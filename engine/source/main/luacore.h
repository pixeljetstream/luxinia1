// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __LUACORE_H__
#define __LUACORE_H__

#include "../fnpublish/fnpublish.h"
#include <lua/lua.h>

typedef enum LuaLuxFunctionState_e {
  LLF_FINISHED,
  LLF_STARTING,
  LLF_ARGPUSHING,
  LLF_CALLING,
  LLF_ARGPOPPING
} LuaLuxFunctionState_t;

void LuaCore_init(int argn, const char *arg[]);
void LuaCore_think ();
void LuaCore_deinit();
void LuaCore_setMainState(lua_State* state);
lua_State* LuaCore_getMainState ();

void LuaCore_callTableFunction(const char *table,const char *funcname);
//void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize);
#endif
