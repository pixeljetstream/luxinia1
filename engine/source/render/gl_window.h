// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_WINDOW_H__
#define __GL_WINDOW_H__

/*
WINDOW
------

Just some info about the current's Window size/colordepth and if it is run fullscreen.
GLUT takes care of the actual WindowManagement.

Toggling to/from fullscreen requires all GL-Memory resident resources to be reloaded
ResData_push/pop does it and will be called automatically.

Author: Christoph Kubisch

Window  (nested, FixedMem)

*/

#include "../common/common.h"

// current screen information
#define SCREEN_WIDTH  g_Window.width
#define SCREEN_HEIGHT g_Window.height
#define SCREEN_SCALE (float)VID_REF_HEIGHT/(float)SCREEN_HEIGHT

  // resstring
  //   WIDTHxHEIGHTxBPP:DEPTH:STENCIL
  //   depth,stencil are optional and default to 24,8

enum LuxWindowBits_e
{
  LUXWIN_BIT_RED,
  LUXWIN_BIT_GREEN,
  LUXWIN_BIT_BLUE,
  LUXWIN_BIT_ALPHA,
  LUXWIN_BIT_DEPTH,
  LUXWIN_BIT_STENCIL,
  LUXWIN_BITS,
};

typedef enum LuxWindowResize_e{
  LUXWIN_RESIZE_NONE,
  LUXWIN_RESIZE_WIN,
  LUXWIN_RESIZE_WIN_REF,
  LUXWIN_RESIZE_WIN_REFSCALED,
}LuxWindowResize_t;

typedef struct LuxWindow_s
{
  LuxWindowResize_t resizemode;
  double        resizetime;

  float       refratioX;  // pixels per mm
  float       refratioY;  // pixels per mm

  int   poppedGL;

  int   width;
  int   height;
  int   minwidth;
  int   minheight;
  float ratio;
  int   bpp;
  int   posX;
  int   posY;
  int   fullscreen;
  int   bits[LUXWIN_BITS];
  enum32  internalformat;
  int   samples;
  int   ontop;
} LuxWindow_t;


// GLOBALS
extern LuxWindow_t g_Window;


void LuxWindow_init(LuxWindow_t *window);
void LuxWindow_setFromString(LuxWindow_t *window, char *resstring);
char* LuxWindow_getString(LuxWindow_t *window);

void LuxWindow_setGL(LuxWindow_t *window);

void LuxWindow_reshape (int w, int h);

void LuxWindow_updatedWindow();

int  LuxWindow_getPos(int *x,int *y);
void LuxWindow_setPos(int x,int y);

void LuxWindow_updatedResizeRef();

// push/pop GL resourced, if pushed, no OpenGL calls should be made
void LuxWindow_pushGL();
void LuxWindow_popGL();
int  LuxWindow_poppedGL();

int   isFullscreen();
void  setFullscreen(int);

#endif

