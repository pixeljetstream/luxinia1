// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/3dmath.h"
#include "controls.h"
#include "../resource/resmanager.h"
#include "../render/gl_camlight.h"
#include "../main/main.h"
#include "../common/console.h"

// GLOBALS
CtrlInput_t g_CtrlInput = {0,0};


// LOCALS
static struct CTRLData_s{
  ulong sshottime;
  int   ctrlkeys[CTRLKEYS];
  int   oldmaxfps;

  void  (*mouseMotion)(float x, float y);
  void  (*mouseClick)(float x, float y, int button, int state);
  void  (*keycallback)( int key, int state, int charcallback);
  float mouseRoundOff[2]; // roundoff factor, simulating a
                // windowsmouse with floating point precissision
}l_CTRLData = {0,{1,1,1,1},0,NULL,NULL,NULL,{0,0}};

///////////////////////////////////////////////////////////////////////////////
// Controls

void Controls_setCtrlKey(CtrlKey_t key, int state)
{
  l_CTRLData.ctrlkeys[key] = state;
}
int  Controls_getCtrlKey(CtrlKey_t key)
{
  return l_CTRLData.ctrlkeys[key];
}


//////////////////////////////////////////////////////////////////////////
// CallBacks
void Controls_setKeyCallback( void( *func)( int key, int state, int charcallback))
{
  l_CTRLData.keycallback = func;
}

void Controls_setMouseClickCallback(void (*click)(float x, float y, int button, int state))
{
  l_CTRLData.mouseClick = click;
}
void Controls_setMouseMotionCallback(void (*motion)(float x, float y))
{
  l_CTRLData.mouseMotion = motion;
}


void Controls_keyInput(int key, int state, int charcall)
{
  //LUX_PRINTF("KEY %d, %d, %d",key,state,charcall);

  if (l_CTRLData.keycallback)
    l_CTRLData.keycallback(key,state,charcall);

  if (key < LUXI_KEY_SPECIAL && !charcall)
    return;

  if(key == LUXI_KEY_F1 && l_CTRLData.ctrlkeys[CTRLKEY_CONSOLE])
  {
    if(state == LUXI_PRESS)
      Console_setActive(!Console_isActive());
    return;
  }
  if (key == LUXI_KEY_F3 && l_CTRLData.ctrlkeys[CTRLKEY_SCREENSHOTS]){
    if (state == LUXI_PRESS && g_LuxTimer.time+1000>l_CTRLData.sshottime){
      vidScreenShot(0);
      l_CTRLData.sshottime = g_LuxTimer.time;
      return;
    }
  }
  if (key == LUXI_KEY_F4 && l_CTRLData.ctrlkeys[CTRLKEY_SCREENSHOTS]){
    if (state == LUXI_PRESS && g_LuxTimer.time+1000>l_CTRLData.sshottime){
      vidScreenShot(1);
      l_CTRLData.sshottime = g_LuxTimer.time;
      return;
    }
  }
  if (key == LUXI_KEY_F11 && l_CTRLData.ctrlkeys[CTRLKEY_RECORD]){
    if (state == LUXI_PRESS && g_LuxTimer.time+1000>l_CTRLData.sshottime){
      g_VID.autodump = !g_VID.autodump;
      if (g_VID.autodump){
        l_CTRLData.oldmaxfps = g_VID.maxFps;
        g_VID.maxFps = 30;
      }
      else{
        g_VID.maxFps = l_CTRLData.oldmaxfps;
      }
      l_CTRLData.sshottime = g_LuxTimer.time;
      return;
    }
  }
  if (key == LUXI_KEY_ESC && state == LUXI_PRESS && (l_CTRLData.ctrlkeys[CTRLKEY_QUIT] || Console_isActive())){
    bprintf("closed by user\n");
    Main_exit(LUX_FALSE);
  }

  if(Console_isActive())
  {
    if(state == LUXI_PRESS)
      Console_keyIn(key);
    return;
  }

}

void Controls_setMousePosition(float x, float y)
{
  int outx = (int)(x/VID_REF_WIDTHSCALE);
  int outy = (int)(y/VID_REF_HEIGHTSCALE);
  g_LuxiniaWinMgr.luxiSetMousePos(outx,outy);
  g_CtrlInput.mousePosition[0] = x;
  g_CtrlInput.mousePosition[1] = y;
  l_CTRLData.mouseRoundOff[0] = (outx*VID_REF_WIDTHSCALE-x);
  l_CTRLData.mouseRoundOff[1] = (outy*VID_REF_HEIGHTSCALE-y);
  //LUX_PRINTF("%.2f %.2f\n",l_CTRLData.mouseRoundOff[0],l_CTRLData.mouseRoundOff[1]);
}

void Controls_mouseClick(int button, int state)
{
  int x,y;

  g_LuxiniaWinMgr.luxiGetMousePos( &x, &y );

  g_CtrlInput.mousePosition[0]= ((float)x - l_CTRLData.mouseRoundOff[0]) * VID_REF_WIDTHSCALE;
  g_CtrlInput.mousePosition[1]= ((float)y - l_CTRLData.mouseRoundOff[1]) * VID_REF_HEIGHTSCALE;

  if (l_CTRLData.mouseClick!=NULL)
    l_CTRLData.mouseClick(g_CtrlInput.mousePosition[0],g_CtrlInput.mousePosition[1],button,state!=LUXI_RELEASE);
}

void Controls_mouseInput(int x,int y)
{
  g_CtrlInput.mousePosition[0]= (float)x - l_CTRLData.mouseRoundOff[0];
  g_CtrlInput.mousePosition[1]= (float)y - l_CTRLData.mouseRoundOff[1];
  g_CtrlInput.mousePosition[0]*=VID_REF_WIDTHSCALE;
  g_CtrlInput.mousePosition[1]*=VID_REF_HEIGHTSCALE;

  if (l_CTRLData.mouseMotion != NULL)
    l_CTRLData.mouseMotion(g_CtrlInput.mousePosition[0],g_CtrlInput.mousePosition[1]);
}
