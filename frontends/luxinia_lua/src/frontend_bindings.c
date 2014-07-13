// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h

#include "frontend_bindings.h"
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

#include <luxinia/luxinia.h>
#include <stdio.h>

// LOCALS
static LuxiniaCallbacks_t l_luxcallbacks;
static lua_State *l_L;
static int l_Lstack;

//////////////////////////////////////////////////////////////////////////
struct LuxiniaCallbacks_s * LuaBinding_getLuxiniaCallbacks()
{
  return &l_luxcallbacks;
}

//////////////////////////////////////////////////////////////////////////
// Frontend -> luxinia

//void (LUXICALL * LUXIframefunc)(void);
static int LUXIframefunc (lua_State *L)
{
  l_luxcallbacks.LUXIframefunc();

  return 0;
}

//void (LUXICALL * LUXIwindowsizefunc)(int w,int h);
static int LUXIwindowsizefunc (lua_State *L)
{
  int stacksize = lua_gettop(L);
  if (stacksize < 2){
    lua_pushstring(L,"2 int required.");
    lua_error(L);
  }
  else
    l_luxcallbacks.LUXIwindowsizefunc((int)lua_tointeger(L,1),(int)lua_tointeger(L,2));

  return 0;
}


//void (LUXICALL * LUXImousebuttonfunc)(int button, int state);
static int LUXImousebuttonfunc (lua_State *L)
{
  int stacksize = lua_gettop(L);
  if (stacksize < 2){
    lua_pushstring(L,"1 int 1 boolean required.");
    lua_error(L);
  }
  else
    l_luxcallbacks.LUXImousebuttonfunc((int)lua_tointeger(L,1),(int)lua_toboolean(L,2));

  return 0;
}

//void (LUXICALL * LUXImouseposfunc)(int x,int y);
static int LUXImouseposfunc (lua_State *L)
{
  int stacksize = lua_gettop(L);
  if (stacksize < 2){
    lua_pushstring(L,"2 int required.");
    lua_error(L);
  }
  else
    l_luxcallbacks.LUXImouseposfunc((int)lua_tointeger(L,1),(int)lua_tointeger(L,2));

  return 0;
}

//void (LUXICALL * LUXIkeyfunc)(int key, int state, int charCall);
static int LUXIkeyfunc (lua_State *L)
{
  int stacksize = lua_gettop(L);
  if (stacksize < 3){
    lua_pushstring(L,"1 int 2 boolean required.");
    lua_error(L);
  }
  else
    l_luxcallbacks.LUXIkeyfunc((int)lua_tointeger(L,1),(int)lua_toboolean(L,2),(int)lua_toboolean(L,3));

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// Frontend Init


static int setfunction (lua_State *L)
{
  if (lua_gettop(L)>1) {
    lua_pushstring(L,"Only one argument is expected");
    lua_error(L);
  }
  if (!lua_isnil(L,1))
    luaL_checktype(L,1,LUA_TFUNCTION); // error if arg 1 is not a function value or nil
  lua_pushvalue(L,lua_upvalueindex(1)); // get the string to be used as safe location
  lua_pushvalue(L,1); // first arg on top, could use insert, but this is simpler (and probably more efficient)
  lua_settable(L,LUA_REGISTRYINDEX); // store value (nil/function) in the registry
  return 0;
}

static int getfunction (lua_State *L)
{
  lua_pushvalue(L,lua_upvalueindex(1)); // get the function key to be fetched
  lua_gettable(L,LUA_REGISTRYINDEX);
  return 1;
}
static int existsfunction (lua_State *L, const char *key)
{
  int exists;
  lua_getfield(L,LUA_REGISTRYINDEX,key);
  exists = !lua_isnil(L,-1);
  lua_pop(L,1);
  return exists;
}

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

// returns 0 on error
static int callfunction (lua_State *L, int narg, int clear, const char *key)
{
  int status;
  int base = lua_gettop(L) - narg +1;  /* function index */
  lua_getfield(L,LUA_REGISTRYINDEX,key); // get the function key to be fetched
  lua_insert(L, base);  /* put it under chunk and args */

  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
  status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
  lua_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
}

static void pushgetset (lua_State *L,const char *key, int tableidx, const char *get, const char *set)
{
  lua_pushstring(L,key);
  lua_pushcclosure(L,setfunction,1);
  lua_setfield(L,tableidx,set);

  lua_pushstring(L,key);
  lua_pushcclosure(L,getfunction,1);
  lua_setfield(L,tableidx,get);
}

static const struct luaL_reg luxfrontendlib[] = {
  {"lrunframe", LUXIframefunc},
  {"lwindowsize", LUXIwindowsizefunc},
  {"lmousebutton", LUXImousebuttonfunc},
  {"lmousepos", LUXImouseposfunc},
  {"lkeyinput", LUXIkeyfunc},
  {NULL, NULL},
};


#define ADD_ENUM(key) lua_pushnumber(L,key); lua_setfield(L,-2,#key);
#define ADD_GETSET(str) pushgetset(L,"LXF" str ,lua_gettop(L),"get" str ,"set" str);

void LuaBinding_initFrontend(struct lua_State *L) {
  // frontend->luxinia
  luaL_register (L, "luxfrontend", luxfrontendlib);

  /////
  // GET/SET, luxinia->frontend

  // General
  ADD_GETSET("PostInit")

  ADD_GETSET("Print")
  ADD_GETSET("SetCallbacks")
  //ADD_GETSET("Terminate")

  // Window handling
  ADD_GETSET("OpenWindow")
  ADD_GETSET("OpenWindowHint")
  ADD_GETSET("CloseWindow")
  ADD_GETSET("SetWindowTitle")
  ADD_GETSET("GetWindowSize")
  ADD_GETSET("SetWindowSize")
  ADD_GETSET("GetWindowPos")
  ADD_GETSET("SetWindowPos")
  ADD_GETSET("SwapBuffers")
  ADD_GETSET("GetWindowParam")
  ADD_GETSET("SetWindowOnTop")
  ADD_GETSET("ClientToScreen")
  ADD_GETSET("ScreenToClient")
  ADD_GETSET("IconifyWindow")
  ADD_GETSET("RestoreWindow")

  // Input handling
  ADD_GETSET("GetKey")
  ADD_GETSET("GetMouseButton")
  ADD_GETSET("GetMousePos")
  ADD_GETSET("SetMousePos")
  ADD_GETSET("GetMouseWheel")
  ADD_GETSET("SetMouseWheel")

  // Enable/disable functions
  ADD_GETSET("Enable")
  ADD_GETSET("Disable")

  //////
  // ENUMS

  ADD_ENUM(LUXI_FALSE)
  ADD_ENUM(LUXI_TRUE)

  ADD_ENUM(LUXI_RELEASE)
  ADD_ENUM(LUXI_PRESS)

  ADD_ENUM(LUXI_KEY_UNKNOWN)
  ADD_ENUM(LUXI_KEY_SPACE)
  ADD_ENUM(LUXI_KEY_SPECIAL)
  ADD_ENUM(LUXI_KEY_ESC)
  ADD_ENUM(LUXI_KEY_F1)
  ADD_ENUM(LUXI_KEY_F2)
  ADD_ENUM(LUXI_KEY_F3)
  ADD_ENUM(LUXI_KEY_F4)
  ADD_ENUM(LUXI_KEY_F5)
  ADD_ENUM(LUXI_KEY_F6)
  ADD_ENUM(LUXI_KEY_F7)
  ADD_ENUM(LUXI_KEY_F8)
  ADD_ENUM(LUXI_KEY_F9)
  ADD_ENUM(LUXI_KEY_F10)
  ADD_ENUM(LUXI_KEY_F11)
  ADD_ENUM(LUXI_KEY_F12)
  ADD_ENUM(LUXI_KEY_F13)
  ADD_ENUM(LUXI_KEY_F14)
  ADD_ENUM(LUXI_KEY_F15)
  ADD_ENUM(LUXI_KEY_F16)
  ADD_ENUM(LUXI_KEY_F17)
  ADD_ENUM(LUXI_KEY_F18)
  ADD_ENUM(LUXI_KEY_F19)
  ADD_ENUM(LUXI_KEY_F20)
  ADD_ENUM(LUXI_KEY_F21)
  ADD_ENUM(LUXI_KEY_F22)
  ADD_ENUM(LUXI_KEY_F23)
  ADD_ENUM(LUXI_KEY_F24)
  ADD_ENUM(LUXI_KEY_F25)
  ADD_ENUM(LUXI_KEY_UP)
  ADD_ENUM(LUXI_KEY_DOWN)
  ADD_ENUM(LUXI_KEY_LEFT)
  ADD_ENUM(LUXI_KEY_RIGHT)
  ADD_ENUM(LUXI_KEY_LSHIFT)
  ADD_ENUM(LUXI_KEY_RSHIFT)
  ADD_ENUM(LUXI_KEY_LCTRL)
  ADD_ENUM(LUXI_KEY_RCTRL)
  ADD_ENUM(LUXI_KEY_LALT)
  ADD_ENUM(LUXI_KEY_RALT)
  ADD_ENUM(LUXI_KEY_TAB)
  ADD_ENUM(LUXI_KEY_ENTER)
  ADD_ENUM(LUXI_KEY_BACKSPACE)
  ADD_ENUM(LUXI_KEY_INSERT)
  ADD_ENUM(LUXI_KEY_DEL)
  ADD_ENUM(LUXI_KEY_PAGEUP)
  ADD_ENUM(LUXI_KEY_PAGEDOWN)
  ADD_ENUM(LUXI_KEY_HOME)
  ADD_ENUM(LUXI_KEY_END)
  ADD_ENUM(LUXI_KEY_KP_0)
  ADD_ENUM(LUXI_KEY_KP_1)
  ADD_ENUM(LUXI_KEY_KP_2)
  ADD_ENUM(LUXI_KEY_KP_3)
  ADD_ENUM(LUXI_KEY_KP_4)
  ADD_ENUM(LUXI_KEY_KP_5)
  ADD_ENUM(LUXI_KEY_KP_6)
  ADD_ENUM(LUXI_KEY_KP_7)
  ADD_ENUM(LUXI_KEY_KP_8)
  ADD_ENUM(LUXI_KEY_KP_9)
  ADD_ENUM(LUXI_KEY_KP_DIVIDE)
  ADD_ENUM(LUXI_KEY_KP_MULTIPLY)
  ADD_ENUM(LUXI_KEY_KP_SUBTRACT)
  ADD_ENUM(LUXI_KEY_KP_ADD)
  ADD_ENUM(LUXI_KEY_KP_DECIMAL)
  ADD_ENUM(LUXI_KEY_KP_EQUAL)
  ADD_ENUM(LUXI_KEY_KP_ENTER)
  ADD_ENUM(LUXI_KEY_APPEXIT)
  ADD_ENUM(LUXI_KEY_LAST)

  ADD_ENUM(LUXI_MOUSE_BUTTON_0)
  ADD_ENUM(LUXI_MOUSE_BUTTON_1)
  ADD_ENUM(LUXI_MOUSE_BUTTON_2)

  ADD_ENUM(LUXI_MOUSE_BUTTON_LEFT)
  ADD_ENUM(LUXI_MOUSE_BUTTON_RIGHT)
  ADD_ENUM(LUXI_MOUSE_BUTTON_MIDDLE)

  ADD_ENUM(LUXI_JOYSTICK_1)
  ADD_ENUM(LUXI_JOYSTICK_2)
  ADD_ENUM(LUXI_JOYSTICK_3)
  ADD_ENUM(LUXI_JOYSTICK_4)
  ADD_ENUM(LUXI_JOYSTICK_5)
  ADD_ENUM(LUXI_JOYSTICK_6)
  ADD_ENUM(LUXI_JOYSTICK_7)
  ADD_ENUM(LUXI_JOYSTICK_8)
  ADD_ENUM(LUXI_JOYSTICK_9)
  ADD_ENUM(LUXI_JOYSTICK_10)
  ADD_ENUM(LUXI_JOYSTICK_11)
  ADD_ENUM(LUXI_JOYSTICK_12)
  ADD_ENUM(LUXI_JOYSTICK_13)
  ADD_ENUM(LUXI_JOYSTICK_14)
  ADD_ENUM(LUXI_JOYSTICK_15)
  ADD_ENUM(LUXI_JOYSTICK_16)
  ADD_ENUM(LUXI_JOYSTICK_LAST)

    // luxiOpenWindow modes)
  ADD_ENUM(LUXI_WINDOW)
  ADD_ENUM(LUXI_FULLSCREEN)
    // luxiWindowParam)
  ADD_ENUM(LUXI_OPENED)
  ADD_ENUM(LUXI_ACTIVE)
    // luxiWindowHint)
  ADD_ENUM(LUXI_REFRESH_RATE)
  ADD_ENUM(LUXI_STEREO)
  ADD_ENUM(LUXI_WINDOW_NO_RESIZE)
  ADD_ENUM(LUXI_FSAA_SAMPLES)

    // luxiEnable/luxiDisable tokens)
  ADD_ENUM(LUXI_MOUSE_CURSOR)
  ADD_ENUM(LUXI_KEY_REPEAT)
  ADD_ENUM(LUXI_MOUSE_FIX)

    // luxiWaitThread wait modes)
  ADD_ENUM(LUXI_WAIT)
  ADD_ENUM(LUXI_NOWAIT)

    // luxiGetJoystickParam tokens)
  ADD_ENUM(LUXI_PRESENT)
  ADD_ENUM(LUXI_AXES)
  ADD_ENUM(LUXI_BUTTONS)

  lua_pop(L,0);

  l_L = L;
}
#undef ADD_ENUM
#undef ADD_GETSET


void printpoperror()
{
  printf("Lua call error: %s\n", lua_tostring(l_L, -1));
  lua_settop(l_L,l_Lstack);
}

//////////////////////////////////////////////////////////////////////////
// Frontend -> Frontend/Lua

void LuaBinding_PostInit()
{
  l_Lstack = lua_gettop(l_L);
  if (callfunction(l_L,0,LUXI_TRUE,"LXFPostInit")) {
    printpoperror();
  }
}


//////////////////////////////////////////////////////////////////////////
// Luxinia -> Frontend

int  LuaBinding_Print(char*str, va_list args)
{
  static char buffer[512];
  int cnt = vsprintf(buffer,str,args);
  if (!cnt){
    return 0;
  }

  // pass to lua
  l_Lstack = lua_gettop(l_L);
  lua_pushstring(l_L,buffer);
  if (callfunction(l_L,1,LUXI_TRUE,"LXFPrint")) {
    printpoperror();
  }

  return cnt;
}

void LuaBinding_SetCallbacks( void )
{
  l_Lstack = lua_gettop(l_L);
  if (callfunction(l_L,0,LUXI_TRUE,"LXFSetCallbacks")) {
    printpoperror();
  }
}

// Window handling
int  LuaBinding_OpenWindow( int width, int height, int redbits, int greenbits, int bluebits, int alphabits, int depthbits, int stencilbits, int mode )
{
  int stacksize;
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,width);
  lua_pushnumber(l_L,height);
  lua_pushnumber(l_L,redbits);
  lua_pushnumber(l_L,greenbits);
  lua_pushnumber(l_L,bluebits);
  lua_pushnumber(l_L,alphabits);
  lua_pushnumber(l_L,depthbits);
  lua_pushnumber(l_L,stencilbits);
  lua_pushnumber(l_L,mode);
  if (callfunction(l_L,9,LUXI_FALSE,"LXFOpenWindow")) {
    printpoperror();;

    return LUXI_FALSE;
  }
  stacksize = lua_gettop(l_L);
  if (stacksize-l_Lstack == 1){
    int state =  lua_toboolean(l_L,stacksize);
    lua_settop(l_L,l_Lstack);
    return state;
  }
  else{
    lua_settop(l_L,l_Lstack);
    return LUXI_FALSE;
  }
}

void LuaBinding_OpenWindowHint( int target, int hint )
{
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,target);
  lua_pushnumber(l_L,hint);
  if (callfunction(l_L,2,LUXI_TRUE,"LXFOpenWindowHint")) {
    printpoperror();
  }
}

void LuaBinding_CloseWindow( void )
{
  l_Lstack = lua_gettop(l_L);
  if (callfunction(l_L,0,LUXI_TRUE,"LXFCloseWindow")) {
    printpoperror();
  }
}

void LuaBinding_SetWindowTitle( const char *title )
{
  l_Lstack = lua_gettop(l_L);
  lua_pushstring(l_L,title);
  if (callfunction(l_L,1,LUXI_TRUE,"LXFSetWindowTitle")) {
    printpoperror();
  }
}

void LuaBinding_GetWindowSize( int *width, int *height )
{
  int stacksize;
  l_Lstack = lua_gettop(l_L);
  if (callfunction(l_L,0,LUXI_FALSE,"LXFGetWindowSize")) {
    printpoperror();
    return;
  }
  stacksize = lua_gettop(l_L);
  if (stacksize-l_Lstack == 2){
    *width = (int)lua_tointeger(l_L,stacksize-1);
    *height = (int)lua_tointeger(l_L,stacksize);
  }
  lua_settop(l_L,l_Lstack);
}

void LuaBinding_SetWindowSize( int width, int height )
{
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,width);
  lua_pushnumber(l_L,height);
  if (callfunction(l_L,2,LUXI_TRUE,"LXFSetWindowSize")) {
    printpoperror();
  }
}

int LuaBinding_GetWindowPos( int *x, int *y )
{
  int stacksize;
  int success = LUXI_FALSE;
  l_Lstack = lua_gettop(l_L);
  if (callfunction(l_L,0,LUXI_FALSE,"LXFGetWindowPos")) {
    printpoperror();
    return LUXI_FALSE;
  }
  stacksize = lua_gettop(l_L);
  if (stacksize - l_Lstack == 2){
    *x = (int)lua_tointeger(l_L,stacksize-1);
    *y = (int)lua_tointeger(l_L,stacksize);
    success = LUXI_TRUE;
  }
  lua_settop(l_L,l_Lstack);
  return success;
}

void LuaBinding_SetWindowPos( int x, int y )
{
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,x);
  lua_pushnumber(l_L,y);
  if (callfunction(l_L,2,LUXI_TRUE,"LXFSetWindowPos")) {
    printpoperror();
  }
}

void LuaBinding_SwapBuffers( void )
{
  l_Lstack = lua_gettop(l_L);
  if (callfunction(l_L,0,LUXI_TRUE,"LXFSwapBuffers")) {
    printpoperror();
  }
}
int  LuaBinding_GetWindowParam( int key )
{
  int stacksize;
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,key);
  if (callfunction(l_L,1,LUXI_FALSE,"LXFGetWindowParam")) {
    printpoperror();
    return LUXI_TRUE;
  }
  stacksize = lua_gettop(l_L);
  if (stacksize - l_Lstack == 1){
    int pos = lua_type(l_L,stacksize)== LUA_TBOOLEAN ? (int)lua_toboolean(l_L,stacksize) : (int)lua_tointeger(l_L,stacksize);
    lua_settop(l_L,l_Lstack);
    return pos;
  }
  else{
    lua_settop(l_L,l_Lstack);
    return LUXI_TRUE;
  }
}

void LuaBinding_SetWindowOnTop( int state )
{
  l_Lstack = lua_gettop(l_L);
  lua_pushboolean(l_L,state);
  if (callfunction(l_L,1,LUXI_TRUE,"LXFSetWindowOnTop")) {
    printpoperror();
  }
}
int LuaBinding_ClientToScreen( int *x, int *y )
{
  int stacksize;
  int success = LUXI_FALSE;
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,*x);
  lua_pushnumber(l_L,*y);
  if (callfunction(l_L,0,LUXI_FALSE,"LXFClientToScreen")) {
    printpoperror();
    return LUXI_FALSE;
  }
  stacksize = lua_gettop(l_L);
  if (stacksize - l_Lstack == 2){
    *x = (int)lua_tointeger(l_L,stacksize-1);
    *y = (int)lua_tointeger(l_L,stacksize);
    success = LUXI_TRUE;
  }
  lua_settop(l_L,l_Lstack);
  return success;
}
int LuaBinding_ScreenToClient( int *x, int *y )
{
  int stacksize;
  int success = LUXI_FALSE;
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,*x);
  lua_pushnumber(l_L,*y);
  if (callfunction(l_L,0,LUXI_FALSE,"LXFScreenToClient")) {
    printpoperror();
    return LUXI_FALSE;
  }
  stacksize = lua_gettop(l_L);
  if (stacksize - l_Lstack == 2){
    *x = (int)lua_tointeger(l_L,stacksize-1);
    *y = (int)lua_tointeger(l_L,stacksize);
    success = LUXI_TRUE;
  }
  lua_settop(l_L,l_Lstack);
  return success;
}

void LuaBinding_IconifyWindow( void )
{
  l_Lstack = lua_gettop(l_L);
  if (callfunction(l_L,0,LUXI_TRUE,"LXFIconifyWindow")) {
    printpoperror();
  }
}

void LuaBinding_RestoreWindow( void )
{
  l_Lstack = lua_gettop(l_L);
  if (callfunction(l_L,0,LUXI_TRUE,"LXFRestoreWindow")) {
    printpoperror();
  }
}

// Input handling
int  LuaBinding_GetKey( int key )
{
  int stacksize;
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,key);
  if (callfunction(l_L,1,LUXI_FALSE,"LXFGetKey")) {
    printpoperror();
    return LUXI_RELEASE;
  }
  stacksize = lua_gettop(l_L);
  if (stacksize - l_Lstack == 1){
    int pos = (int)lua_toboolean(l_L,stacksize);
    lua_settop(l_L,l_Lstack);
    return pos;
  }
  else{
    lua_settop(l_L,l_Lstack);
    return LUXI_RELEASE;
  }
}

int  LuaBinding_GetMouseButton( int button )
{
  int stacksize;
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,button);
  if (callfunction(l_L,1,LUXI_FALSE,"LXFGetMouseButton")) {
    printpoperror();
    return LUXI_RELEASE;
  }
  stacksize = lua_gettop(l_L);
  if (stacksize-l_Lstack == 1){
    int pos = (int)lua_toboolean(l_L,stacksize);
    lua_settop(l_L,l_Lstack);
    return pos;
  }
  else{
    lua_settop(l_L,l_Lstack);
    return LUXI_RELEASE;
  }
}

void LuaBinding_GetMousePos( int *x, int *y )
{
  int stacksize;
  l_Lstack = lua_gettop(l_L);
  if (callfunction(l_L,0,LUXI_FALSE,"LXFGetMousePos")) {
    printpoperror();
    return;
  }
  stacksize = lua_gettop(l_L);
  if (stacksize-l_Lstack == 2){
    *x = (int)lua_tointeger(l_L,stacksize-1);
    *y = (int)lua_tointeger(l_L,stacksize);
  }
  lua_settop(l_L,l_Lstack);
}

void LuaBinding_SetMousePos( int x, int y )
{
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,x);
  lua_pushnumber(l_L,y);
  if (callfunction(l_L,2,LUXI_TRUE,"LXFSetMousePos")) {
    printpoperror();
  }
}

int  LuaBinding_GetMouseWheel( void )
{
  int stacksize;
  l_Lstack = lua_gettop(l_L);
  if (callfunction(l_L,0,LUXI_FALSE,"LXFGetMousePos")) {
    printpoperror();
    return 0;
  }
  stacksize = lua_gettop(l_L);
  if (stacksize-l_Lstack == 1){
    int pos = (int)lua_tointeger(l_L,stacksize);
    lua_settop(l_L,l_Lstack);
    return pos;
  }
  else{
    lua_settop(l_L,l_Lstack);
    return 0;
  }
}

void LuaBinding_SetMouseWheel( int pos )
{
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,pos);
  if (callfunction(l_L,1,LUXI_TRUE,"LXFSetMouseWheel")) {
    printpoperror();
  }
}


// Enable/disable functions
void LuaBinding_Enable( int token )
{
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,token);
  if (callfunction(l_L,1,LUXI_TRUE,"LXFEnable")) {
    printpoperror();
  }
}

void LuaBinding_Disable( int token )
{
  l_Lstack = lua_gettop(l_L);
  lua_pushnumber(l_L,token);
  if (callfunction(l_L,1,LUXI_TRUE,"LXFDisable")) {
    printpoperror();
  }
}

