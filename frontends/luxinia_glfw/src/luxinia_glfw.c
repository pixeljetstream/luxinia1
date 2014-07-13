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

#include "../../shared/os.h"
#include "../../shared/project.h"
#include "../../shared/luastate.h"

static LuxiniaCallbacks_t l_luxcallbacks;

void Key_CharCallback( int key, int action )
{
  l_luxcallbacks.LUXIkeyfunc(key,action,1);
}

void Key_SpecCallback( int key, int action )
{
  l_luxcallbacks.LUXIkeyfunc(key,action,0);
}

int WindowCloseFunc(void)
{
  // fire event
  l_luxcallbacks.LUXIkeyfunc(LUXI_KEY_APPEXIT,LUXI_PRESS,0);
  l_luxcallbacks.LUXIkeyfunc(LUXI_KEY_APPEXIT,LUXI_RELEASE,0);

  return LUX_FALSE;
}

int PrintFunc(char *str,va_list arg)
{
  int ret = vfprintf(stdout,str,arg);
  fflush(stdout);
  return ret;
}

void SetCallbacks(void)
{
  glfwSetCharCallback(Key_CharCallback);
  glfwSetKeyCallback(Key_SpecCallback);

  glfwSetMousePosCallback(l_luxcallbacks.LUXImouseposfunc);
  glfwSetMouseButtonCallback(l_luxcallbacks.LUXImousebuttonfunc);

  glfwSetWindowCloseCallback(WindowCloseFunc);
  glfwSetWindowSizeCallback(l_luxcallbacks.LUXIwindowsizefunc);
}

void Terminate(void)
{
  LuaState_freeMem();
  glfwTerminate();
}

static LuxiniaWinMgr_t l_glfwmgr = {
    PrintFunc,      //int (LUXICALL * luxiPrint)( char*,char*);
    SetCallbacks,   //void (LUXICALL * luxiSetCallbacks)( void );
    Terminate,      //void (LUXICALL * luxiTerminate)( void );

    // Window handling
    glfwOpenWindow,     //int  (LUXICALL * luxiOpenWindow)( int width, int height, int redbits, int greenbits, int bluebits, int alphabits, int depthbits, int stencilbits, int mode );
    glfwOpenWindowHint,   //void (LUXICALL * luxiOpenWindowHint)( int target, int hint );
    glfwCloseWindow,    //void (LUXICALL * luxiCloseWindow)( void );
    glfwSetWindowTitle,   //void (LUXICALL * luxiSetWindowTitle)( const char *title );
    glfwGetWindowSize,    //void (LUXICALL * luxiGetWindowSize)( int *width, int *height );
    glfwSetWindowSize,    //void (LUXICALL * luxiSetWindowSize)( int width, int height );
    glfwGetWindowPos,   //int  (LUXICALL * luxiGetWindowPos)( int *x, int *y );
    glfwSetWindowPos,   //void (LUXICALL * luxiSetWindowPos)( int x, int y );
    glfwSwapBuffers,    //void (LUXICALL * luxiSwapBuffers)( void );
    glfwSwapInterval,   //void (LUXICALL * luxiSetSwapInterval)( int interval );
    glfwGetWindowParam,   //int  (LUXICALL * luxiGetWindowParam)( int param );
    glfwSetWindowOnTop,   //void (LUXICALL * luxiSetWindowOnTop)( int state );
    glfwClientToScreen,   //int  (LUXICALL * luxiClientToScreen)( int *x, int *y );
    glfwScreenToClient,   //int  (LUXICALL * luxiScreenToClient)( int *x, int *y );
    glfwIconifyWindow,    //void (LUXICALL * luxiIconifyWindow)( void );
    glfwRestoreWindow,    //void (LUXICALL * lxRestoreWindow)( void );

    // Input handling
    glfwGetKey,       //int  (LUXICALL * luxiGetKey)( int key );
    glfwGetMouseButton,   //int  (LUXICALL * luxiGetMouseButton)( int button );
    glfwGetMousePos,    //void (LUXICALL * luxiGetMousePos)( int *xpos, int *ypos );
    glfwSetMousePos,    //void (LUXICALL * luxiSetMousePos)( int xpos, int ypos );
    glfwGetMouseWheel,    //int  (LUXICALL * luxiGetMouseWheel)( void );
    glfwSetMouseWheel,    //void (LUXICALL * luxiSetMouseWheel)( int pos );

    // Enable/disable functions
    glfwEnable,         //void (LUXICALL * luxiEnable)( int token );
    glfwDisable,        //void (LUXICALL * luxiDisable)( int token );

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
    ProjectCustomStrings,   // const char*  (LUXICALL * luxiGetCustomStrings) ( const char *input );
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

  ProjectInit();
  ProjectForcedArgs(&argc,&argv);

  // int glfw
  glfwInit();


  // get lux callbacks
  Luxina_getCallbacks(&l_luxcallbacks);
  SetCallbacks();

  // set lux winmgr funcs
  Luxina_setWinMgr(&l_glfwmgr);


  // startup app
  atexit(l_luxcallbacks.LUXIatexitfunc);
  if (error=Luxina_start(LuaState_get(),argc,argv))
    return error;

  // this kind of polling is not accurately representing
  // systems under load, but it's beneficial for capping fps
  // and maintaining smooth inputs
  glfwDisable(GLFW_AUTO_POLL_EVENTS);
  //l_luxcallbacks.LUXIframefun();
//  glfwCreateThread(eventhandling,NULL);
  while(1){
    l_luxcallbacks.LUXIframefunc();
    glfwPollEvents();
  }
  return 0;
}

