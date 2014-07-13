// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h

#ifndef __LUA_FRONTEND_BINDINGS_H__
#define __LUA_FRONTEND_BINDINGS_H__

// init funcs
struct LuxiniaCallbacks_s * LuaBinding_getLuxiniaCallbacks();
void LuaBinding_initFrontend(struct lua_State *L);


//////////////////////////////////////////////////////////////////////////
// comm
void LuaBinding_PostInit();

int  LuaBinding_Print(char*str, char* va_listitem);
void LuaBinding_SetCallbacks( void );

// Window handling
int  LuaBinding_OpenWindow( int width, int height, int redbits, int greenbits, int bluebits, int alphabits, int depthbits, int stencilbits, int mode );
void LuaBinding_OpenWindowHint( int target, int hint );
void LuaBinding_CloseWindow( void );
void LuaBinding_SetWindowTitle( const char *title );
void LuaBinding_GetWindowSize( int *width, int *height );
void LuaBinding_SetWindowSize( int width, int height );
int  LuaBinding_GetWindowPos( int *x, int *y );
void LuaBinding_SetWindowPos( int x, int y );
void LuaBinding_SwapBuffers( void );
int  LuaBinding_GetWindowParam( int param );
void LuaBinding_SetWindowOnTop( int state );
int  LuaBinding_ClientToScreen( int *x, int *y );
int  LuaBinding_ScreenToClient( int *x, int *y );
void LuaBinding_IconifyWindow( void );
void LuaBinding_RestoreWindow( void );

// Input handling
int  LuaBinding_GetKey( int key );
int  LuaBinding_GetMouseButton( int button );
void LuaBinding_GetMousePos( int *xpos, int *ypos );
void LuaBinding_SetMousePos( int xpos, int ypos );
int  LuaBinding_GetMouseWheel( void );
void LuaBinding_SetMouseWheel( int pos );

// Enable/disable functions
void LuaBinding_Enable( int token );
void LuaBinding_Disable( int token );

#endif
