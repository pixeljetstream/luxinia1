/*
Luxinia 1 Engine SDK License
=============================================================================
Copyright: 2004-2011 Christoph Kubisch and Eike Decker. 
All rights reserved. 
http://www.luxinia.de

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this 
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this 
  list of conditions and the following disclaimer in the documentation and/or 
  other materials provided with the distribution.
* Neither the name of LUXINIA nor the names of its contributors may
  be used to endorse or promote products derived from this software without 
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __LUXINIA_H__
#define __LUXINIA_H__

#ifdef __cplusplus
extern "C" {
#endif

	// BUILD RELATED
#if defined(_WIN32) && defined(LUXINIA_BUILD_DLL)

	// We are building a Win32 DLL
#define LUXIAPI      __declspec(dllexport)

#elif defined(_WIN32) && defined(LUXINIA_USE_DLL)

	// We are calling a Win32 DLL
#define LUXIAPI      __declspec(dllimport)

#else

	// We are either building/calling a static lib or we are non-win32
#define LUXIAPI

#endif


	// values
#define LUXI_FALSE				0
#define LUXI_TRUE				1

#define LUXI_RELEASE            0
#define LUXI_PRESS              1

#define LUXI_KEY_UNKNOWN      -1
#define LUXI_KEY_SPACE        32
#define LUXI_KEY_SPECIAL      256
#define LUXI_KEY_ESC          (LUXI_KEY_SPECIAL+1)
#define LUXI_KEY_F1           (LUXI_KEY_SPECIAL+2)
#define LUXI_KEY_F2           (LUXI_KEY_SPECIAL+3)
#define LUXI_KEY_F3           (LUXI_KEY_SPECIAL+4)
#define LUXI_KEY_F4           (LUXI_KEY_SPECIAL+5)
#define LUXI_KEY_F5           (LUXI_KEY_SPECIAL+6)
#define LUXI_KEY_F6           (LUXI_KEY_SPECIAL+7)
#define LUXI_KEY_F7           (LUXI_KEY_SPECIAL+8)
#define LUXI_KEY_F8           (LUXI_KEY_SPECIAL+9)
#define LUXI_KEY_F9           (LUXI_KEY_SPECIAL+10)
#define LUXI_KEY_F10          (LUXI_KEY_SPECIAL+11)
#define LUXI_KEY_F11          (LUXI_KEY_SPECIAL+12)
#define LUXI_KEY_F12          (LUXI_KEY_SPECIAL+13)
#define LUXI_KEY_F13          (LUXI_KEY_SPECIAL+14)
#define LUXI_KEY_F14          (LUXI_KEY_SPECIAL+15)
#define LUXI_KEY_F15          (LUXI_KEY_SPECIAL+16)
#define LUXI_KEY_F16          (LUXI_KEY_SPECIAL+17)
#define LUXI_KEY_F17          (LUXI_KEY_SPECIAL+18)
#define LUXI_KEY_F18          (LUXI_KEY_SPECIAL+19)
#define LUXI_KEY_F19          (LUXI_KEY_SPECIAL+20)
#define LUXI_KEY_F20          (LUXI_KEY_SPECIAL+21)
#define LUXI_KEY_F21          (LUXI_KEY_SPECIAL+22)
#define LUXI_KEY_F22          (LUXI_KEY_SPECIAL+23)
#define LUXI_KEY_F23          (LUXI_KEY_SPECIAL+24)
#define LUXI_KEY_F24          (LUXI_KEY_SPECIAL+25)
#define LUXI_KEY_F25          (LUXI_KEY_SPECIAL+26)
#define LUXI_KEY_UP           (LUXI_KEY_SPECIAL+27)
#define LUXI_KEY_DOWN         (LUXI_KEY_SPECIAL+28)
#define LUXI_KEY_LEFT         (LUXI_KEY_SPECIAL+29)
#define LUXI_KEY_RIGHT        (LUXI_KEY_SPECIAL+30)
#define LUXI_KEY_LSHIFT       (LUXI_KEY_SPECIAL+31)
#define LUXI_KEY_RSHIFT       (LUXI_KEY_SPECIAL+32)
#define LUXI_KEY_LCTRL        (LUXI_KEY_SPECIAL+33)
#define LUXI_KEY_RCTRL        (LUXI_KEY_SPECIAL+34)
#define LUXI_KEY_LALT         (LUXI_KEY_SPECIAL+35)
#define LUXI_KEY_RALT         (LUXI_KEY_SPECIAL+36)
#define LUXI_KEY_TAB          (LUXI_KEY_SPECIAL+37)
#define LUXI_KEY_ENTER        (LUXI_KEY_SPECIAL+38)
#define LUXI_KEY_BACKSPACE    (LUXI_KEY_SPECIAL+39)
#define LUXI_KEY_INSERT       (LUXI_KEY_SPECIAL+40)
#define LUXI_KEY_DEL          (LUXI_KEY_SPECIAL+41)
#define LUXI_KEY_PAGEUP       (LUXI_KEY_SPECIAL+42)
#define LUXI_KEY_PAGEDOWN     (LUXI_KEY_SPECIAL+43)
#define LUXI_KEY_HOME         (LUXI_KEY_SPECIAL+44)
#define LUXI_KEY_END          (LUXI_KEY_SPECIAL+45)
#define LUXI_KEY_KP_0         (LUXI_KEY_SPECIAL+46)
#define LUXI_KEY_KP_1         (LUXI_KEY_SPECIAL+47)
#define LUXI_KEY_KP_2         (LUXI_KEY_SPECIAL+48)
#define LUXI_KEY_KP_3         (LUXI_KEY_SPECIAL+49)
#define LUXI_KEY_KP_4         (LUXI_KEY_SPECIAL+50)
#define LUXI_KEY_KP_5         (LUXI_KEY_SPECIAL+51)
#define LUXI_KEY_KP_6         (LUXI_KEY_SPECIAL+52)
#define LUXI_KEY_KP_7         (LUXI_KEY_SPECIAL+53)
#define LUXI_KEY_KP_8         (LUXI_KEY_SPECIAL+54)
#define LUXI_KEY_KP_9         (LUXI_KEY_SPECIAL+55)
#define LUXI_KEY_KP_DIVIDE    (LUXI_KEY_SPECIAL+56)
#define LUXI_KEY_KP_MULTIPLY  (LUXI_KEY_SPECIAL+57)
#define LUXI_KEY_KP_SUBTRACT  (LUXI_KEY_SPECIAL+58)
#define LUXI_KEY_KP_ADD       (LUXI_KEY_SPECIAL+59)
#define LUXI_KEY_KP_DECIMAL   (LUXI_KEY_SPECIAL+60)
#define LUXI_KEY_KP_EQUAL     (LUXI_KEY_SPECIAL+61)
#define LUXI_KEY_KP_ENTER     (LUXI_KEY_SPECIAL+62)
#define LUXI_KEY_APPEXIT	  (LUXI_KEY_SPECIAL+63)	// custom to luxinia, above like GLFW
#define LUXI_KEY_LAST         LUXI_KEY_APPEXIT

#define LUXI_MOUSE_BUTTON_0      0
#define LUXI_MOUSE_BUTTON_1      1
#define LUXI_MOUSE_BUTTON_2      2

#define LUXI_MOUSE_BUTTON_LEFT   LUXI_MOUSE_BUTTON_0
#define LUXI_MOUSE_BUTTON_RIGHT  LUXI_MOUSE_BUTTON_1
#define LUXI_MOUSE_BUTTON_MIDDLE LUXI_MOUSE_BUTTON_2

#define LUXI_JOYSTICK_1          0
#define LUXI_JOYSTICK_2          1
#define LUXI_JOYSTICK_3          2
#define LUXI_JOYSTICK_4          3
#define LUXI_JOYSTICK_5          4
#define LUXI_JOYSTICK_6          5
#define LUXI_JOYSTICK_7          6
#define LUXI_JOYSTICK_8          7
#define LUXI_JOYSTICK_9          8
#define LUXI_JOYSTICK_10         9
#define LUXI_JOYSTICK_11         10
#define LUXI_JOYSTICK_12         11
#define LUXI_JOYSTICK_13         12
#define LUXI_JOYSTICK_14         13
#define LUXI_JOYSTICK_15         14
#define LUXI_JOYSTICK_16         15
#define LUXI_JOYSTICK_LAST       LUXI_JOYSTICK_16


	// luxiOpenWindow modes
#define LUXI_WINDOW               0x00010001
#define LUXI_FULLSCREEN           0x00010002
	// luxiWindowParam
#define LUXI_OPENED               0x00020001
#define LUXI_ACTIVE               0x00020002
	// luxiWindowHint
#define LUXI_REFRESH_RATE         0x0002000B
#define LUXI_STEREO               0x00020011
#define LUXI_WINDOW_NO_RESIZE     0x00020012
#define LUXI_FSAA_SAMPLES         0x00020013

	// luxiEnable/luxiDisable tokens
#define LUXI_MOUSE_CURSOR         0x00030001
#define LUXI_KEY_REPEAT           0x00030005
#define LUXI_MOUSE_FIX            0x00030007

	// luxiWaitThread wait modes
#define LUXI_WAIT                 0x00040001
#define LUXI_NOWAIT               0x00040002

	// luxiGetJoystickParam tokens
#define LUXI_PRESENT              0x00050001
#define LUXI_AXES                 0x00050002
#define LUXI_BUTTONS              0x00050003

	// Thread ID
	typedef int LUXIthread;

	// Mutex object
	typedef void * LUXImutex;

	// Condition variable object
	typedef void * LUXIcond;

	// functions
	typedef void (* LUXIthreadfun)(void *);

	// functions from lux
	// frontend -> luxinia
	typedef struct LuxiniaCallbacks_s{
		void ( * LUXIframefunc)(void);		// needs GLCONTEXT bound
		void ( * LUXIwindowsizefunc)(int w,int h);
		void ( * LUXIatexitfunc)(void);
		void ( * LUXImousebuttonfunc)(int button, int state);
		void ( * LUXImouseposfunc)(int x,int y);
		void ( * LUXIkeyfunc)(int key, int state, int charCall);
		//void ( * LUXImousewheelfunc)(int);		// unused
	}LuxiniaCallbacks_t;

	// functions from api
	// luxinia -> frontend
	// LUXI initialization, termination and version querying
	typedef struct LuxiniaWinMgr_s{

		int	 ( * luxiPrint)(char*str, char* va_listitem);
		void ( * luxiSetCallbacks)( void );
		void ( * luxiTerminate)( void );

		// Window handling
		int  ( * luxiOpenWindow)( int width, int height, int redbits, int greenbits, int bluebits, int alphabits, int depthbits, int stencilbits, int mode );
		void ( * luxiOpenWindowHint)( int target, int hint );
		void ( * luxiCloseWindow)( void ); // used in fullscreen toggle
		void ( * luxiSetWindowTitle)( const char *title );
		void ( * luxiGetWindowSize)( int *width, int *height );
		void ( * luxiSetWindowSize)( int width, int height );
		int  ( * luxiGetWindowPos)( int *x, int *y );
		void ( * luxiSetWindowPos)( int x, int y );
		void ( * luxiSwapBuffers)( void );
		void ( * luxiSetSwapInterval)( int interval );
		int  ( * luxiGetWindowParam)( int param );
		void ( * luxiSetWindowOnTop)( int state );
		int  ( * luxiClientToScreen)( int *x, int *y );
		int  ( * luxiScreenToClient)( int *x, int *y );
		void ( * luxiIconifyWindow)( void );
		void ( * lxRestoreWindow)( void );

		// Input handling
		int  ( * luxiGetKey)( int key );
		int  ( * luxiGetMouseButton)( int button );
		void ( * luxiGetMousePos)( int *xpos, int *ypos );
		void ( * luxiSetMousePos)( int xpos, int ypos );
		int  ( * luxiGetMouseWheel)( void );
		void ( * luxiSetMouseWheel)( int pos );

		// Enable/disable functions
		void ( * luxiEnable)( int token );
		void ( * luxiDisable)( int token );

		// Joystick input
		int ( * luxiGetJoystickParam)( int joy, int param );
		int ( * luxiGetJoystickPos)( int joy, float *pos, int numaxes );
		int ( * luxiGetJoystickButtons)( int joy, unsigned char *buttons, int numbuttons );

		// Time
		double ( * luxiGetTime)( void );
		void   ( * luxiSetTime)( double time );
		void   ( * luxiSleep)( double time );

		// Threading support
		LUXIthread ( * luxiCreateThread)( LUXIthreadfun fun, void *arg );
		void ( * luxiDestroyThread)( LUXIthread ID );
		int  ( * luxiWaitThread)( LUXIthread ID, int waitmode );
		LUXIthread ( * luxiGetThreadID)( void );
		LUXImutex ( * luxiCreateMutex)( void );
		void ( * luxiDestroyMutex)( LUXImutex mutex );
		void ( * luxiLockMutex)( LUXImutex mutex );
		void ( * luxiUnlockMutex)( LUXImutex mutex );
		LUXIcond ( * luxiCreateCond)( void );
		void ( * luxiDestroyCond)( LUXIcond cond );
		void ( * luxiWaitCond)( LUXIcond cond, LUXImutex mutex, double timeout );
		void ( * luxiSignalCond)( LUXIcond cond );
		void ( * luxiBroadcastCond)( LUXIcond cond );
		int  ( * luxiGetNumberOfProcessors)( void );

		// some more OS calls
		int				( * luxiGetDrives)(char ** outnames,int maxnames,int maxlength);
		const char*		( * luxiGetDriveLabel)(char *drive);
		int				( * luxiGetDriveSize)(char *drive,double *freetocaller, double *total, double *totalfree);
		const char*		( * luxiGetDriveType)(char *drive);
		const char*		( * luxiGetOSString)( void );
		const char*		( * luxiGetMACaddress) ( void );
		void			( * luxiGetScreenSizeMilliMeters)(int *w,int *h);
		void			( * luxiGetScreenRes)(int *w, int *h);
		unsigned int	( * luxiGetVideoRam)( void );

		// Lua related
		unsigned int ( * luxiLuaGetMemUsed)( void );
		unsigned int ( * luxiLuaGetMemAllocated)( void );

		// custom call
		const char* ( * luxiGetCustomStrings) ( const char *input );
			//	pathconfig
			//	pathscreenshot
			//	pathlog
			//	pathwork (path of exefile)
			//	pathexe (path + exefile)
			//	logobig
			//	logosmall
			//	logoclearcolor
			//	logotime
			//	resstr:%d
	}LuxiniaWinMgr_t;

	// Frontend Setup
	LUXIAPI void Luxina_getCallbacks(LuxiniaCallbacks_t *destinationcopy);
	LUXIAPI int	 Luxina_setWinMgr(const LuxiniaWinMgr_t *sourcecopy);
	LUXIAPI int	 Luxina_start(struct lua_State *lstate,const int argc, const char **argv);

#include "refclasses.h"
#include "reftypes.h"

	//////////////////////////////////////////////////////////////////////////
	// Runtime DLL extenders

	// Math
	LUXIAPI struct lxFastMathCache_s*	Luxinia_getFastMathCache();

	// FPub
	LUXIAPI void LuxiniaFPub_pushType(struct lua_State *L,lxClassType type,void *data);
	LUXIAPI int  LuxiniaFPub_getType(struct lua_State *L,int n, lxClassType type,void *saveto);
	LUXIAPI unsigned int	LuxiniaFPub_isPointer(lxClassType type);
	LUXIAPI unsigned int	LuxiniaFPub_isReference(lxClassType type);

	typedef struct LuxiniaFPub_s{
		struct lua_State*	luastate;
		void (*pushType)(struct lua_State *L,lxClassType type,void *data);
		int  (*getType)(struct lua_State *L,int n, lxClassType type,void *saveto);
		unsigned int	(*isPointer)(lxClassType type);
		unsigned int	(*isReference)(lxClassType type);
	}LuxiniaFPub_t;

	LUXIAPI	LuxiniaFPub_t*			Luxinia_getFPub();

	// RefSys
	LUXIAPI unsigned int	LuxiniaRefSys_getSafe(lxRef, void **out);
	LUXIAPI unsigned int	LuxiniaRefSys_addUser(lxRef );
	LUXIAPI void			LuxiniaRefSys_addWeak(lxRef );
	LUXIAPI unsigned int	LuxiniaRefSys_releaseUser(lxRef );
	LUXIAPI void			LuxiniaRefSys_releaseWeak(lxRef );

	typedef struct LuxiniaRefSys_s{
		struct lxObjRefSys_s*	refsys;
		unsigned int (*getSafe)(lxRef, void **out);
		unsigned int (*addUser)(lxRef );
		void		(*addWeak)(lxRef );
		unsigned int (*releaseUser)(lxRef );
		void		(*releaseWeak)(lxRef );
	}LuxiniaRefSys_t;
	
	LUXIAPI LuxiniaRefSys_t*		Luxinia_getRefSys();
	

	

#ifdef __cplusplus
}
#endif

#endif

