// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __CONTROLS_H__
#define __CONTROLS_H__

#include "../main/main.h"

typedef enum CtrlKey_e{
  CTRLKEY_QUIT,
  CTRLKEY_SCREENSHOTS,
  CTRLKEY_CONSOLE,
  CTRLKEY_RECORD,
  CTRLKEYS
}CtrlKey_t;

// GLOBALS
typedef struct CtrlInput_s{
  float mousePosition[2];
}CtrlInput_t;

extern CtrlInput_t  g_CtrlInput;

void Controls_keyInput(int key, int state,int charcallback);
void Controls_mouseInput(int x, int y);
void Controls_mouseClick(int button, int state);

void Controls_setCtrlKey(CtrlKey_t key, int state);
int  Controls_getCtrlKey(CtrlKey_t key);

void Controls_setMouseClickCallback(void (*click)(float x, float y, int button, int state));
void Controls_setMouseMotionCallback(void (*motion)(float x, float y));
void Controls_setMousePosition(float x, float y);
void Controls_setKeyCallback( void( *func)( int key, int state, int charcallback));

#endif

