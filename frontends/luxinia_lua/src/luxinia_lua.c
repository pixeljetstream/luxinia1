// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h

#include <luxinia/luxinia.h>
#include <assert.h>

#ifdef _WIN32
  #include <windows.h>
  #include <winbase.h>
#else
  #include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <GL/glfw.h>
#include "frontend_bindings.h"

#include "../../shared/os.h"
#include "../../shared/project.h"
#include "../../shared/luastate.h"

#include <lua/lua.h>


//////////////////////////////////////////////////////////////////////////
void Terminate(void)
{
  LuaState_freeMem();
  glfwTerminate();
}


static LuxiniaWinMgr_t l_glfwmgr = {
    LuaBinding_Print,
    LuaBinding_SetCallbacks,
    Terminate,  // operates after lua_close, glfw also needed for threads..

    // Window handling
    LuaBinding_OpenWindow,
    LuaBinding_OpenWindowHint,
    LuaBinding_CloseWindow,
    LuaBinding_SetWindowTitle,
    LuaBinding_GetWindowSize,
    LuaBinding_SetWindowSize,
    LuaBinding_GetWindowPos,
    LuaBinding_SetWindowPos,
    LuaBinding_SwapBuffers,
    osSwapInterval,
    LuaBinding_GetWindowParam,
    LuaBinding_SetWindowOnTop,
    LuaBinding_ClientToScreen,
    LuaBinding_ScreenToClient,
    LuaBinding_IconifyWindow,
    LuaBinding_RestoreWindow,

    // Input handling
    LuaBinding_GetKey,
    LuaBinding_GetMouseButton,
    LuaBinding_GetMousePos,
    LuaBinding_SetMousePos,
    LuaBinding_GetMouseWheel,
    LuaBinding_SetMouseWheel,

    // Enable/disable functions
    LuaBinding_Enable,
    LuaBinding_Disable,

    // Joystick input
    glfwGetJoystickParam, //int (LUXICALL * luxiGetJoystickParam)( int joy, int param );
    glfwGetJoystickPos,   //int (LUXICALL * luxiGetJoystickPos)( int joy, float *pos, int numaxes );
    glfwGetJoystickButtons, //int (LUXICALL * luxiGetJoystickButtons)( int joy, unsigned char *buttons, int numbuttons );

    // Time
    glfwGetTime,        //double (LUXICALL * luxiGetTime)( void );
    glfwSetTime,        //void   (LUXICALL * luxiSetTime)( double time );
    glfwSleep,          //void   (LUXICALL * luxiSleep)( double time );

    // Threading support
    glfwCreateThread,     //LUXIthread (LUXICALL * luxiCreateThread)( LUXIthreadfun fun, void *arg );
    glfwDestroyThread,      //void (LUXICALL * luxiDestroyThread)( LUXIthread ID );
    glfwWaitThread,       //int  (LUXICALL * luxiWaitThread)( LUXIthread ID, int waitmode );
    glfwGetThreadID,      //LUXIthread (LUXICALL * luxiGetThreadID)( void );
    glfwCreateMutex,      //LUXImutex (LUXICALL * luxiCreateMutex)( void );
    glfwDestroyMutex,     //void (LUXICALL * luxiDestroyMutex)( LUXImutex mutex );
    glfwLockMutex,        //void (LUXICALL * luxiLockMutex)( LUXImutex mutex );
    glfwUnlockMutex,      //void (LUXICALL * luxiUnlockMutex)( LUXImutex mutex );
    glfwCreateCond,       //LUXIcond (LUXICALL * luxiCreateCond)( void );
    glfwDestroyCond,      //void (LUXICALL * luxiDestroyCond)( LUXIcond cond );
    glfwWaitCond,       //void (LUXICALL * luxiWaitCond)( LUXIcond cond, LUXImutex mutex, double timeout );
    glfwSignalCond,       //void (LUXICALL * luxiSignalCond)( LUXIcond cond );
    glfwBroadcastCond,      //void (LUXICALL * luxiBroadcastCond)( LUXIcond cond );
    glfwGetNumberOfProcessors,  //int  (LUXICALL * luxiGetNumberOfProcessors)( void );

    // some more OS calls
    osGetDrives,        //int     (LUXICALL * luxiGetDrives)(char ** outnames,int maxnames,int maxlength);
    osGetDriveLabel,      //const char* (LUXICALL * luxiGetDriveLabel)(char *drive);
    osGetDriveSize,       //int     (LUXICALL * luxiGetDriveSize)(char *drive,double *freetocaller, double *total, double *totalfree);
    osGetDriveType,       //const char* (LUXICALL * luxiGetDriveType)(char *drive);
    osGetOSString,        //const char* (LUXICALL * luxiGetOSString)( void );
    osGetMACaddress,      //const char*   (LUXICALL * luxiGetMACaddress) ( void );
    osGetScreenSizeMilliMeters, //void      (LUXICALL * luxiGetScreenSizeMilliMeters)(int *w,int *h);
    osGetScreenRes,       //void      (LUXICALL * luxiGetScreenRes)(int *w, int *h);
    osGetVideoRam,        //unsigned int  (LUXICALL * luxiGetVideoRam)( void );

    // Lua Related
    LuaState_getMemUsed,    // int  (LUXICALL * luxiLuaGetMemUsed)( void );
    LuaState_getMemAllocated, // int  (LUXICALL * luxiLuaGetMemAllocated)( void );

    // custom string
    ProjectCustomStrings,       // const char*  (LUXICALL * luxiGetCustomStrings) ( const char *input );
};


static void eventhandling (void *arg)
{
  while (1) {
    //l_luxcallbacks.LUXIframefun();
    printf ("event...\n");
    glfwWaitEvents();
  }
}

#if defined(LUXINIA_NOCONSOLE) && defined(_WIN32) && defined (_MSC_VER)


extern PCHAR* CommandLineToArgvA(PCHAR CmdLine,int* _argc);

int WINAPI WinMain(HINSTANCE hInstance,  HINSTANCE hPrevInstance,  LPSTR lpCmdLine, int nCmdShow)
{
  int argc;
  char ** argv = CommandLineToArgvA(GetCommandLineA(),&argc);

#else
int main(int argc, char **argv)
{
#endif
  int error;
  lua_State *L;
  LuxiniaCallbacks_t *luxcallbacks;

  ProjectInit();
  ProjectForcedArgs(&argc,&argv);

  // init glfw for thread & joystick
  glfwInit();


  // get lux callbacks
  luxcallbacks = LuaBinding_getLuxiniaCallbacks();
  Luxina_getCallbacks(luxcallbacks);
  atexit(luxcallbacks->LUXIatexitfunc);

  // set lux winmgr funcs
  Luxina_setWinMgr(&l_glfwmgr);

  // init frontend lua
  L = LuaState_get();
  LuaBinding_initFrontend(L);

  // startup luxinia
  if (error=Luxina_start(L,argc,argv))
    return error;

  LuaBinding_PostInit();

  return 0;
}

