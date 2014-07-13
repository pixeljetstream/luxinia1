// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "../main/main.h"

#include "../common/common.h"
#include "../common/3dmath.h"


#include "luacore.h"
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

//#define LUX_LUACORE_TLS

static int LuaCore_call (lua_State *L,int narg, int clear);


//////////////////////////////////////////

typedef struct LCore_s {
  lua_State *L;
  int init;
} LCore_t;


static LCore_t l_core = {NULL,0};

#ifdef LUX_LUACORE_TLS
//////////////////////////////////////////////////
// Threaded Lua State

typedef struct TLS_s {
  LUXImutex mutex;
  LUXIthread thread;
  int running;
  LUXIthread locker;
  lua_State *L;
  lua_State *exchange;

} TLS_t;

static void mlock (LUXImutex mutex, int lock) {
  if (lock) g_LuxiniaWinMgr.luxiLockMutex(mutex);
  else g_LuxiniaWinMgr.luxiUnlockMutex(mutex);
}

static int TLS_delete (lua_State *L)
{
  TLS_t *ext = luaL_checkudata(L,1,"tls");
  if (ext->thread) g_LuxiniaWinMgr.luxiDestroyThread(ext->thread);
  if (ext->L) lua_close(ext->L);
  if (ext->exchange) lua_close(ext->exchange);
  ext->thread = 0;
  ext->L = NULL;
  ext->exchange = NULL;

  if (ext->mutex) g_LuxiniaWinMgr.luxiDestroyMutex(ext->mutex);
  ext->mutex = 0;
  return 0;
}

static int TLS_stop_ (lua_State *L,TLS_t *self)
{
  if (self->running) {
    g_LuxiniaWinMgr.luxiDestroyThread(self->thread);
    self->thread = 0;
    self->running = 0;
  }
  return 0;
}
static int TLS_exlock_ (lua_State *L,TLS_t *self)
{
  if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
    mlock(self->mutex,1);
  self->locker = g_LuxiniaWinMgr.luxiGetThreadID();
  return 0;
}
static int TLS_exunlock_ (lua_State *L,TLS_t *self)
{
  if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker) return 0;
  self->locker = -1;
  mlock(self->mutex,0);
  return 0;
}
static int TLS_exget_ (lua_State *L,TLS_t *self)
{
  int len = 0;
  if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
    mlock(self->mutex,1);

  switch(lua_type(L,2)) {
    case LUA_TNUMBER:
      lua_pushnumber(self->exchange,lua_tonumber(L,2));
      break;
    case LUA_TSTRING:
      lua_pushlstring(self->exchange,lua_tolstring(L,2,&len),len);
      break;
    default:
      if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
        mlock(self->mutex,0);
      lua_pushfstring(L,"Unsupported key: %s",lua_typename(L,lua_type(L,2)));
      lua_error(L);
      break;
  }
  lua_gettable(self->exchange,LUA_GLOBALSINDEX);
  switch(lua_type(self->exchange,-1)) {
    case LUA_TNUMBER:
      lua_pushnumber(L,lua_tonumber(self->exchange,-1));
      break;
    case LUA_TSTRING:
      lua_pushlstring(L,lua_tolstring(self->exchange,-1,&len),len);
      break;
    case LUA_TNIL:
      lua_pushnil(L);
      break;
    default:
      lua_pushnil(L);
      lua_pushstring(L,lua_typename(self->exchange,lua_type(self->exchange,-1)));
      len = -1;
      break;
  }
  lua_settop(self->exchange,0);
  if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
    mlock(self->mutex,0);

  return len==-1?2:1;
}
static int TLS_exset_ (lua_State *L, TLS_t *self)
{
  int len = 0;
  if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
    mlock(self->mutex,1);

  switch(lua_type(L,2)) {
    case LUA_TNUMBER:
      lua_pushnumber(self->exchange,lua_tonumber(L,2));
      break;
    case LUA_TSTRING:
      lua_pushlstring(self->exchange,lua_tolstring(L,2,&len),len);
      break;
    default:
      if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
        mlock(self->mutex,0);
      lua_pushfstring(L,"Unsupported key: %s",lua_typename(L,lua_type(L,2)));
      lua_error(L);
      break;
  }
  switch(lua_type(L,3)) {
    case LUA_TNUMBER:
      lua_pushnumber(self->exchange,lua_tonumber(L,3));
      break;
    case LUA_TSTRING:
      lua_pushlstring(self->exchange,lua_tolstring(L,3,&len),len);
      break;
    case LUA_TNIL:
      lua_pushnil(self->exchange);
      break;
    default:
      lua_settop(self->exchange,0);
      if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
        mlock(self->mutex,0);
      lua_pushfstring(L,"Unsupported value: %s",lua_typename(L,lua_type(L,3)));
      lua_error(L);
      break;
  }
  lua_settable(self->exchange,LUA_GLOBALSINDEX);
  if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
    mlock(self->mutex,0);
  return 0;
}
static int TLS_exexec_ (lua_State *L, TLS_t *self)
{
  const char *str = luaL_checkstring(L,2);
  int i,n,len = 0;

  if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
    mlock(self->mutex,1);
  if (luaL_loadstring(self->exchange,str))
  {
    lua_pushnil(L);
    lua_pushstring(L,lua_tostring(self->exchange,-1));
    lua_settop(self->exchange,0);
    if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
      mlock(self->mutex,0);
    return 2;
  }
  n = lua_gettop(self->exchange);
  if (lua_pcall(self->exchange,0,LUA_MULTRET,0)) {
    lua_pushnil(L);
    lua_pushstring(L,lua_tostring(self->exchange,-1));
    lua_settop(self->exchange,0);
    if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
      mlock(self->mutex,0);
    return 2;
  }

  lua_pushboolean(L,1);
  for (i=n;i<=lua_gettop(self->exchange);i++) {
    switch(lua_type(self->exchange,i)) {
      case LUA_TSTRING:
        lua_pushlstring(L,lua_tolstring(self->exchange,i,&len),len);
      break;
      case LUA_TNUMBER:
        lua_pushnumber(L,lua_tonumber(self->exchange,i));
      break;
      default:
        lua_pushnil(L);
    }
  }
  lua_settop(self->exchange,0);
  if (g_LuxiniaWinMgr.luxiGetThreadID()!=self->locker)
    mlock(self->mutex,0);


  return i-n+1;
}

static int TLS_stop (lua_State *L)
{
  TLS_t *self = (TLS_t*)luaL_checkudata(L,1,"tls");
  lua_pushnil(L);
  lua_insert(L,1);
  return TLS_stop_(L,self);
}

static int TLS_self_killme (lua_State *L)
{
  TLS_t *self = (TLS_t*)lua_touserdata(L,lua_upvalueindex(1));
  lua_pushnil(L);
  lua_insert(L,1);
  return TLS_stop_(L,self);
}

static int TLS_exlock (lua_State *L)
{
  TLS_t *self = (TLS_t*)luaL_checkudata(L,1,"tls");
  lua_pushnil(L);
  lua_insert(L,1);
  return TLS_exlock_(L,self);
}

static int TLS_self_elock (lua_State *L)
{
  TLS_t *self = (TLS_t*)lua_touserdata(L,lua_upvalueindex(1));
  lua_pushnil(L);
  lua_insert(L,1);
  return TLS_exlock_(L,self);
}

static int TLS_exunlock (lua_State *L)
{
  TLS_t *self = (TLS_t*)luaL_checkudata(L,1,"tls");
  return TLS_exunlock_(L,self);
}

static int TLS_self_eunlock (lua_State *L)
{
  TLS_t *self = (TLS_t*)lua_touserdata(L,lua_upvalueindex(1));
  lua_pushnil(L);
  lua_insert(L,1);
  return TLS_exunlock_(L,self);
}

static int TLS_exset (lua_State *L)
{
  TLS_t *self = (TLS_t*)luaL_checkudata(L,1,"tls");
  return TLS_exset_(L,self);
}
static int TLS_self_eset (lua_State *L)
{
  TLS_t *self = (TLS_t*)lua_touserdata(L,lua_upvalueindex(1));
  lua_pushnil(L);
  lua_insert(L,1);
  return TLS_exset_(L,self);
}


static int TLS_exget (lua_State *L)
{
  TLS_t *self = (TLS_t*)luaL_checkudata(L,1,"tls");
  return TLS_exget_(L,self);
}

static int TLS_self_eget (lua_State *L)
{
  TLS_t *self = (TLS_t*)lua_touserdata(L,lua_upvalueindex(1));
  lua_pushnil(L);
  lua_insert(L,1);
  return TLS_exget_(L,self);
}

static int TLS_exexec (lua_State *L)
{
  TLS_t *self = (TLS_t*)luaL_checkudata(L,1,"tls");
  return TLS_exexec_(L,self);
}

static int TLS_self_exexec (lua_State *L)
{
  TLS_t *self = (TLS_t*)lua_touserdata(L,lua_upvalueindex(1));
  lua_pushnil(L);
  lua_insert(L,1);
  return TLS_exexec_(L,self);
}

static int TLS_self_sleep (lua_State *L)
{
  g_LuxiniaWinMgr.luxiSleep(luaL_checknumber(L,-1));
  return 0;
}





static int TLS_running (lua_State *L)
{
  TLS_t *self = luaL_checkudata(L,-1,"tls");
  lua_pushboolean(L,self->running);
  return 1;
}

#define TLS_NEW "new"
#define TLS_START "start"
#define TLS_EXDOSTRING "xdostring"
#define TLS_EXSET "xset"
#define TLS_EXGET "xget"
#define TLS_EXLOCK "xlock"
#define TLS_EXUNLOCK "xunlock"
#define TLS_KILLME "killme"
#define TLS_SLEEP "sleep"
#define TLS_STOP "stop"
#define TLS_RUNNING "running"

static int TLS_create (lua_State *L)
{
  TLS_t *ext;
  ext = (TLS_t*)lua_newuserdata(L,sizeof(TLS_t));
  ext->L = luaL_newstate();
  ext->thread = 0;
  ext->exchange = luaL_newstate();
  ext->mutex = g_LuxiniaWinMgr.luxiCreateMutex();
  ext->locker = -1;
  ext->running = 0;

  luaL_getmetatable(L,"tls");
  lua_setmetatable(L,-2);


  luaL_openlibs(ext->L);

  luaL_openlibs(ext->exchange);

  lua_getfield(L,LUA_GLOBALSINDEX,"package");
  lua_getfield(ext->L,LUA_GLOBALSINDEX,"package");
  lua_getfield(L,-1,"cpath");
  lua_pushstring(ext->L,lua_tostring(L,-1));
  lua_setfield(ext->L,-2,"cpath");
  lua_pop(L,1);
  lua_getfield(L,-1,"path");
  lua_pushstring(ext->L,lua_tostring(L,-1));
  lua_setfield(ext->L,-2,"path");
  lua_pop(ext->L,1);
  lua_pop(L,2);


  lua_pushlightuserdata(ext->L,ext);

  lua_pushvalue(ext->L,-1);
  lua_pushcclosure(ext->L,TLS_self_exexec,1);
  lua_setfield(ext->L,LUA_GLOBALSINDEX,TLS_EXDOSTRING);

  lua_pushvalue(ext->L,-1);
  lua_pushcclosure(ext->L,TLS_self_eset,1);
  lua_setfield(ext->L,LUA_GLOBALSINDEX,TLS_EXSET);

  lua_pushvalue(ext->L,-1);
  lua_pushcclosure(ext->L,TLS_self_eget,1);
  lua_setfield(ext->L,LUA_GLOBALSINDEX,TLS_EXGET);

  lua_pushvalue(ext->L,-1);
  lua_pushcclosure(ext->L,TLS_self_elock,1);
  lua_setfield(ext->L,LUA_GLOBALSINDEX,TLS_EXLOCK);

  lua_pushvalue(ext->L,-1);
  lua_pushcclosure(ext->L,TLS_self_eunlock,1);
  lua_setfield(ext->L,LUA_GLOBALSINDEX,TLS_EXUNLOCK);

  lua_pushvalue(ext->L,-1);
  lua_pushcclosure(ext->L,TLS_self_killme,1);
  lua_setfield(ext->L,LUA_GLOBALSINDEX,TLS_KILLME);

  lua_pushcfunction(ext->L,TLS_self_sleep);
  lua_setfield(ext->L,LUA_GLOBALSINDEX,TLS_SLEEP);

  lua_settop(ext->L,0);

  return 1;
}

static void LUXICALL TLS_kickoff (void *data)
{
  LUXIthread thread;
  TLS_t *self = (TLS_t*)data;

  if (LuaCore_call (self->L,0, 1)) {
    fprintf(stderr, "Lua init error: %s\n", lua_tostring(self->L, -1));
    fflush(stderr);
    lua_pop(self->L, 1);  // pop error message from the stack
  };

  thread = self->thread;
  self->running = 0;
}

static int TLS_start (lua_State *L)
{
  TLS_t *self = luaL_checkudata(L,-2,"tls");
  const char* kickoffcode = luaL_checkstring(L,-1);

  if (self->running) {
    lua_pushnil(L);
    lua_pushstring(L,"Thread is running");
    return 2;
  }
  self->running = 1;
  lua_settop(self->L,0);
  if (luaL_loadstring(self->L,kickoffcode)) {
    lua_pushnil(L);
    lua_pushstring(L,lua_tostring(self->L,-1));
    lua_settop(self->L,0);
    return 2;
  }
  self->thread = g_LuxiniaWinMgr.luxiCreateThread(TLS_kickoff,(void*)self);
  lua_pushboolean(L,1);
  return 1;
}

static void TLS_init (lua_State *L)
{
  luaL_Reg tls[] = {
    {TLS_NEW, TLS_create},
    {TLS_START, TLS_start},
    {TLS_EXDOSTRING, TLS_exexec},
    {TLS_EXSET, TLS_exset},
    {TLS_EXGET, TLS_exget},
    {TLS_EXLOCK, TLS_exlock},
    {TLS_EXUNLOCK, TLS_exunlock},
    {TLS_STOP, TLS_stop},
    {TLS_RUNNING, TLS_running},
    {NULL,NULL}
  };
  luaL_newmetatable(L,"tls");
  luaL_register(L,"tls",tls);
  lua_setfield(L,-2,"__index");
  lua_pushcfunction(L,TLS_delete);
  lua_setfield(L,-2,"__gc");
  lua_pop(L,1);
}
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


static int traceback (lua_State *L) {
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}

static int LuaCore_call (lua_State *L,int narg, int clear)
{

  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
  //signal(SIGINT, laction);
  status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
  //signal(SIGINT, SIG_DFL);
  lua_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
}

void LuaCore_setMainState(lua_State * Lstate)
{
  l_core.L = Lstate;
}

lua_State *LuaCore_getMainState()
{
  return l_core.L;
}

int LuaCore_gcwatch (lua_State *L)
{
  luaL_checktype(L,1,LUA_TFUNCTION);
  lua_newuserdata(L,4);

  lua_newtable(L);
  lua_pushvalue(L,1);
  lua_setfield(L,-2,"__gc");
  lua_setmetatable(L,2);

  return 1;
}

extern int luaopen_bit(lua_State *L);

void LuaCore_init(int argn, const char *arg[])
{
  int i;
  lua_State *L = LuaCore_getMainState();
  l_core.init = 1;

  luaopen_bit(L);
  lua_pop(l_core.L,1);

  LUX_ASSERT(L);

  lua_pushcfunction(L,LuaCore_gcwatch);
  lua_setglobal(L,"gcwatch");
#ifdef LUX_LUACORE_TLS
  TLS_init (l_core.L);
#endif

  {
    // init luxiniaptrs table
    lua_newtable(L);
    lua_pushlightuserdata(L,Luxinia_getFastMathCache());
    lua_setfield(L,-2,"fmcache");

    lua_pushlightuserdata(L,Luxinia_getRefSys());
    lua_setfield(L,-2,"refsys");

    lua_pushlightuserdata(L,Luxinia_getFPub());
    lua_setfield(L,-2,"fpub");

    lua_setglobal(L,"luxiniaptrs");
  }


  if (luaL_loadfile(L,"base/main.lua")) {
    fprintf(stderr, "Lua init error: %s\n", lua_tostring(L, -1));
    fflush(stderr);
    lua_pop(L, 1);  // pop error message from the stack
    return;
  }

  for (i=0;i<argn;i++)
    lua_pushstring(L,arg[i]);

  if (LuaCore_call(L,argn,1)) {
    fprintf(stderr, "Lua init error: %s\n", lua_tostring(L, -1));
    fflush(stderr);
    lua_pop(L, 1);  // pop error message from the stack
  }
}


void LuaCore_deinit()
{
  //lua_close(l_core.L); // done by refsys
  lua_getfield(l_core.L,LUA_GLOBALSINDEX,"luxinia");
  if (lua_type(l_core.L,-1)!=LUA_TTABLE) {
    lua_pop(l_core.L,1);
  } else {
    lua_getfield(l_core.L,-1,"atexit");
    if (lua_type(l_core.L,-1)==LUA_TFUNCTION) {
      LuaCore_call(l_core.L,0,1);
    } else
      lua_pop(l_core.L,1);
    lua_pop(l_core.L,1);
  }
  l_core.init = 0;
}

void LuaCore_think ()
{
  lua_getglobal(l_core.L,"luxinia");
  if (lua_type(l_core.L,-1)!=LUA_TTABLE) {
    lua_pop(l_core.L,1);
    return;
  }
  lua_getfield(l_core.L,-1,"think");
  lua_remove(l_core.L,-2);
  if (!lua_isfunction(l_core.L,-1)) {
    lua_pop(l_core.L,1);
    return;
  }
  if (LuaCore_call(l_core.L,0,1)) {
    fprintf(stderr, "%s\n", lua_tostring(l_core.L, -1));
    lua_pop(l_core.L, 1);  // pop error message from the stack
  }
}

void LuaCore_callTableFunction(const char *table,const char *funcname)
{
  lua_getglobal(l_core.L,table);
  if (lua_type(l_core.L,-1)!=LUA_TTABLE) {
    lua_pop(l_core.L,1);
    return;
  }
  lua_getfield(l_core.L,-1,funcname);
  lua_remove(l_core.L,-2);
  if (!lua_isfunction(l_core.L,-1)) {
    lua_pop(l_core.L,1);
    return;
  }
  if (LuaCore_call(l_core.L,0,1)) {
    fprintf(stderr, "%s\n", lua_tostring(l_core.L, -1));
    lua_pop(l_core.L, 1);  // pop error message from the stack
  }
}

