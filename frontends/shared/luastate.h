// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h

#ifndef __LUA_STATE_GETTER_H__
#define __LUA_STATE_GETTER_H__

#ifdef __cplusplus
extern "C" {
#endif

  // creates and returns same
struct lua_State* LuaState_get( void );
void        LuaState_freeMem( void );

  // for profiling stats
unsigned int    LuaState_getMemUsed( void );
unsigned int    LuaState_getMemAllocated( void );

#ifdef __cplusplus
}
#endif


#endif