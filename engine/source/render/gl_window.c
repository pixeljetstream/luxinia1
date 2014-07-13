// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "gl_window.h"
#include "gl_render.h"
#include "../main/luacore.h"
#include "../main/main.h"
#include "../resource/resmanager.h"

extern void PText_initGL();
extern void PText_clearGL();

// GLOBALS
LuxWindow_t g_Window;


// LOCALS
static struct WINData_s{
  int   lastbpp;
  int   lastfullscreen;
  int   lastsbits;
  int   lastsamples;
  int   lastw;
  int   lasth;

  int   pfposx; // pre fullscreen pos
  int   pfposy;
}l_WINData = {0,0,0,0,0,0};


void LuxWindow_init(LuxWindow_t *window)
{
  window->minheight = 1;
  window->minwidth = 1;
  LuxWindow_setFromString(window,"640x480x32:24:8:0");
  window->fullscreen = LUX_FALSE;
  window->posX = 100;
  window->posY = 100;
  window->ontop = LUX_FALSE;
  window->resizemode = LUXWIN_RESIZE_NONE;
  window->resizetime = 0.0;
  window->refratioX = 1920.0/512.0;
  window->refratioY = 1200.0/320.0;
}

void LuxWindow_setFromString(LuxWindow_t *window, char *resstring)
{
  if (sscanf(resstring,"%dx%dx%d:%d:%d:%d",&window->width,&window->height,&window->bpp,&window->bits[LUXWIN_BIT_DEPTH],&window->bits[LUXWIN_BIT_STENCIL],&window->samples)<3){
    window->width = 640;
    window->height = 480;
    window->bpp = 32;
    window->bits[LUXWIN_BIT_DEPTH] = 24;
    window->bits[LUXWIN_BIT_STENCIL] = 8;
    window->samples = 0;
    bprintf("WARNING: windowres illegal resolutionstring %s",resstring);
  }
  if (window->bpp != 32 && window->bpp != 16 && window->bpp != 24){
    bprintf("WARNING: windowres unknown bits per pixel in %s",resstring);
    window->bpp = 32;
  }
  if (window->samples%2 || window->samples >16){
    bprintf("WARNING: windowres unknown AA samples %s",resstring);
    window->samples = 0;
  }
}
char* LuxWindow_getString(LuxWindow_t *window)
{
  static char outstring[20];

  sprintf(outstring,"%dx%dx%d:%d:%d:%d",window->width,window->height,window->bpp,window->bits[LUXWIN_BIT_DEPTH],window->bits[LUXWIN_BIT_STENCIL],window->samples);
  return outstring;
}

void LuxWindow_resized(LuxWindow_t *window)
{
  int w,h,buffers;
  g_LuxiniaWinMgr.luxiGetWindowSize(&w,&h);
  LUX_DEBUG_FRPRINT("Win Resized: %i %i\n",w,h);

  window->width = w;
  window->height = h;

  g_VID.windowWidth = w;
  g_VID.windowHeight = h;

  g_VID.drawbufferWidth = w;
  g_VID.drawbufferHeight = h;

  window->ratio = (float)w / (float)h;

  // assuming 32bit + 32bit depth/stencil

  // front,back,multisample buffers
  buffers = l_WINData.lastsamples ? l_WINData.lastsamples+1 : 2;
  LUX_PROFILING_OP( g_Profiling.global.memory.vidsurfmem -= (l_WINData.lastw*l_WINData.lasth*buffers*8 ));
  LUX_PROFILING_OP( g_Profiling.global.memory.vidsurfmem += (w*h*buffers*8 ));

  l_WINData.lastw = w;
  l_WINData.lasth = h;
}

void LuxWindow_createGL(LuxWindow_t *window)
{
  int red,green,blue,alpha,depth,stencil,mode;

  stencil = window->bits[LUXWIN_BIT_STENCIL];
  depth = window->bits[LUXWIN_BIT_DEPTH];

  if (window->bpp == 32){
    red = green = blue = alpha = 8;
  }
  else if (window->bpp == 24){
    red = blue = green = 8;
    alpha = 0;
  }
  else{
    red = blue = 5;
    green = 6;
    alpha = 0;
  }


  if (window->fullscreen)
    mode = LUXI_FULLSCREEN;
  else
    mode = LUXI_WINDOW;

  if (window->samples){
    g_LuxiniaWinMgr.luxiOpenWindowHint(LUXI_FSAA_SAMPLES,window->samples);
  }

  if (!g_LuxiniaWinMgr.luxiOpenWindow(window->width,window->height,red,green,blue,alpha,depth,stencil,mode)){
    bprintf("ERROR: Window could not be created\n");
    Main_exit(LUX_FALSE);
  }


  if (!window->fullscreen){
    g_LuxiniaWinMgr.luxiSetWindowPos(window->posX,window->posY);
  }
  g_LuxiniaWinMgr.luxiSetWindowTitle("luxinia");
  g_LuxiniaWinMgr.luxiSetSwapInterval(g_VID.swapinterval);

  {
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
      bprintf("ERROR: %s\n", glewGetErrorString(err));
      Main_exit(LUX_FALSE);
    }
  }

  glGetIntegerv(GL_RED_BITS,    &window->bits[LUXWIN_BIT_RED]);
  glGetIntegerv(GL_GREEN_BITS,  &window->bits[LUXWIN_BIT_GREEN]);
  glGetIntegerv(GL_BLUE_BITS,   &window->bits[LUXWIN_BIT_BLUE]);
  glGetIntegerv(GL_ALPHA_BITS,  &window->bits[LUXWIN_BIT_ALPHA]);
  glGetIntegerv(GL_DEPTH_BITS,  &window->bits[LUXWIN_BIT_DEPTH]);
  glGetIntegerv(GL_STENCIL_BITS,  &window->bits[LUXWIN_BIT_STENCIL]);

  if (GLEW_ARB_multisample){
    int samplebuffers = 0;
    int samples = 0;
    glGetIntegerv(GL_SAMPLE_BUFFERS_ARB, &samplebuffers);
    glGetIntegerv(GL_SAMPLES_ARB, &samples);
    window->samples = (samplebuffers > 0) * samples;
  }
  else{
    window->samples = 0;
  }

  if (window->samples){
    glEnable(GL_MULTISAMPLE_ARB);
  }

  window->internalformat = (window->bits[LUXWIN_BIT_ALPHA]) ? GL_RGBA8 : GL_RGB8;

  LuxWindow_resized(window);
}

//#define LUX_DEBUG_NOFULLSCREEN

void LuxWindow_setGL(LuxWindow_t *window)
{
  // this leaves quite some possibilities
  // we either just want to change resolution
  // or change to fullscreen/window

  window->ratio = (float)window->width / (float)window->height;
  if (window->fullscreen != l_WINData.lastfullscreen || window->bpp != l_WINData.lastbpp || window->bits[LUXWIN_BIT_STENCIL] != l_WINData.lastsbits || window->samples != l_WINData.lastsamples){

    if (g_LuxTimer.time > 2){
      LuxWindow_popGL();

      if (!l_WINData.lastfullscreen){
        g_LuxiniaWinMgr.luxiGetWindowPos(&window->posX,&window->posY);
      }

#ifndef LUX_DEBUG_NOFULLSCREEN
      g_LuxiniaWinMgr.luxiCloseWindow();
#endif
    }

    if (window->fullscreen){
      l_WINData.pfposx = window->posX;
      l_WINData.pfposy = window->posY;
    }

#ifdef LUX_DEBUG_NOFULLSCREEN
    if (g_LuxTimer.time < 10)
#endif
      LuxWindow_createGL(window);

    if (g_LuxTimer.time > 2){
      LuxWindow_pushGL();
    }
    // re-register callbacks
    Main_init();
  }
  else{
    LuxWindow_reshape(window->width, window->height);
  }

  l_WINData.lastsamples = window->samples;
  l_WINData.lastbpp = window->bpp;
  l_WINData.lastfullscreen = window->fullscreen;
  l_WINData.lastsbits = window->bits[LUXWIN_BIT_STENCIL];
}
void LuxWindow_pushGL()
{
  if (!g_Window.poppedGL) return;
  glFinish();
  VID_initGL();
  PText_initGL();
  Render_initGL();
  Render_pushGL();
  ResData_pushGL();
  g_Window.poppedGL = LUX_FALSE;
}

void LuxWindow_popGL()
{
  if (g_Window.poppedGL) return;
  Render_popGL();
  ResData_popGL();
  PText_clearGL();
  VID_deinitGL();
  g_Window.poppedGL = LUX_TRUE;
}

int LuxWindow_poppedGL()
{
  return g_Window.poppedGL;
}

void LuxWindow_reshape (int w, int h)
{
  LuxWindow_t *window = &g_Window;
  LUX_DEBUG_FRPRINT("Win Reshape: %i %i\n",w,h); LUX_DEBUG_FRFLUSH;

  // fixed size
  if (window->resizemode == LUXWIN_RESIZE_NONE){
    w = window->width;
    h = window->height;
  }

  w = LUX_MAX(w,window->minwidth);
  h = LUX_MAX(h,window->minheight);

  g_LuxiniaWinMgr.luxiSetWindowSize(w,h);
  LuxWindow_resized(window);
  LuxWindow_updatedResizeRef();
  if (window->resizemode != LUXWIN_RESIZE_NONE){
    window->resizetime = getMicros()+100000.0;
  }
}

void LuxWindow_updatedResizeRef()
{
  LuxWindow_t *window = &g_Window;
  switch(window->resizemode)
  {
  case LUXWIN_RESIZE_WIN_REF:
    g_VID.refScreenWidth = (float)window->width;
    g_VID.refScreenHeight = (float)window->height;
    break;
  case LUXWIN_RESIZE_WIN_REFSCALED:
    {
      int sw,sh;
      int rw,rh;
      float x,y;

      g_LuxiniaWinMgr.luxiGetScreenRes(&rw,&rh);
      g_LuxiniaWinMgr.luxiGetScreenSizeMilliMeters(&sw,&sh);

      x = (float)rw/(float)sw;
      y = (float)rh/(float)sh;

      g_VID.refScreenWidth = (float)window->width * (window->refratioX/x);
      g_VID.refScreenHeight = (float)window->height * (window->refratioY/y);
    }
  default:
    break;
  }

}

void LuxWindow_updatedWindow()
{
  LuxWindow_t *window = &g_Window;
  LUX_PRINTF("Win Upd: %i,%i\n",window->width,window->height);
  if(GLEW_NV_texture_rectangle){
    g_VID.drawsetup.texwindowMatrix[0]= (float)window->width;
    g_VID.drawsetup.texwindowMatrix[5]= (float)window->height;
  }
  else{
    g_VID.drawsetup.texwindowMatrix[0]= (float)window->width/(float)lxNextPowerOf2(window->width);
    g_VID.drawsetup.texwindowMatrix[5]= (float)window->height/(float)lxNextPowerOf2(window->height);
  }
  LUX_DEBUG_FRPRINT("Win Upd Tex\n");
  ResData_resizeWindowTextures();
  LUX_DEBUG_FRPRINT("Win Upd Render\n");
  Render_onWindowResize();
  LUX_DEBUG_FRPRINT("Win Upd Lua\n");
  LuaCore_callTableFunction("luxinia","windowresized");
  LUX_PRINTF("Win Upd: ...done\n");
}

int LuxWindow_getPos(int *x,int *y)
{
  LuxWindow_t *window = &g_Window;
  if (x && y && window->fullscreen){
    *x = l_WINData.pfposx;
    *y = l_WINData.pfposy;
    return LUX_TRUE;
  }
  if (x && y && g_LuxiniaWinMgr.luxiGetWindowPos(&window->posX,&window->posY)){
    *x = window->posX;
    *y = window->posY;
    return LUX_TRUE;
  }
  return LUX_FALSE;
}

void LuxWindow_setPos(int x,int y)
{
  LuxWindow_t *window = &g_Window;
  window->posX = x;
  window->posY = y;
  if (!window->fullscreen)
    g_LuxiniaWinMgr.luxiSetWindowPos(window->posX,window->posY);
  else{
    l_WINData.pfposx = x;
    l_WINData.pfposy = y;
  }
}

//outstanding fullscreen algorithm (zet compatible)
int isFullscreen(){ return g_Window.fullscreen;}
void setFullscreen(int state){ g_Window.fullscreen=state;}
