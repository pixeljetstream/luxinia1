// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h

#include "luastate.h"

//#define LUASTATE_DLMALLOC

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>
#include <luxinia/luxcore/memorylist.h>
#include <luxinia/luxcore/memorygeneric.h>


#ifdef __cplusplus
  }
#endif


// LOCALS
static struct{
  lua_State     *L;
  size_t        memUsed;

  lxMemoryListPTR mem;
  lxMemoryGenericPTR memgeneric;
}l_State = {NULL,0,NULL,NULL};


//////////////////////////////////////////////////////////////////////////



static void *alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  LUX_PROFILING_OP( l_State.memUsed += nsize-osize);
  if (nsize == 0) {
    lxMemoryList_freeItem(l_State.mem,ptr,osize);
    return NULL;
  }
  return lxMemoryList_reallocItem(l_State.mem,ptr,osize, nsize);
}

//////////////////////////////////////////////////////////////////////////


struct lua_State* LuaState_get( void )
{
  if (l_State.L) return l_State.L;

  l_State.memgeneric = lxMemoryGeneric_new(lxMemoryGenericDescr_default());
  l_State.mem = lxMemoryList_newBits( lxMemoryGeneric_allocator(l_State.memgeneric),4,19,1<<21,0);
  l_State.L = lua_newstate(alloc,l_State.mem);
  //l_State.L = luaL_newstate();
  luaL_openlibs(l_State.L);

  return l_State.L;
}

void LuaState_freeMem( void )
{
  lxMemoryList_delete(l_State.mem);
  lxMemoryGeneric_delete(l_State.memgeneric);
}

unsigned int  LuaState_getMemUsed( void )
{
  return (unsigned int)l_State.memUsed;
}

unsigned int  LuaState_getMemAllocated( void )
{
  uint allocTotal, allocPaged;
  float pageRatio;
  lxMemoryList_stats(l_State.mem,&allocTotal,&allocPaged,&pageRatio);
  return allocTotal;
}

