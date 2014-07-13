// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../main/main.h"
#include "../controls/controls.h"
#include "../common/3dmath.h"
#include "../common/console.h"
#include "../common/timer.h"
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////
// offline key history tracking (instead of using callbacks, which would
// make everything so very complicated)
#define QUEUE_SIZE 128
typedef enum KeyInfoType_e {
  KeyInfoUp = 1, KeyInfoDown = 2
} KeyInfoType_t;

typedef struct KeyInfo_s {
  KeyInfoType_t type;
  int key;
  int keycode;
} KeyInfo_t;

static KeyInfo_t l_keyHistory[QUEUE_SIZE];
static unsigned int l_keyHistory_start=0,l_keyHistory_end = 0;

static void PubInput_callback(int key, int state, int ischar)
{
  int pos;
  LUX_DEBUG_FRPRINT("Key: %i %i %i\n",key,state,ischar); LUX_DEBUG_FRFLUSH;
  if (ischar==0 && key<256) return;
  pos = l_keyHistory_end++%QUEUE_SIZE;
  l_keyHistory[pos].keycode = key;
  l_keyHistory[pos].key = key;
  if (ischar==0)
  {
    switch(key) {
      case LUXI_KEY_BACKSPACE: l_keyHistory[pos].key = '\b'; break;
      case LUXI_KEY_TAB: l_keyHistory[pos].key = '\t'; break;
      case LUXI_KEY_ENTER: l_keyHistory[pos].key = '\n'; break;
      default: l_keyHistory[pos].key = 0; break;
    }
  }
  l_keyHistory[pos].type = (state)?KeyInfoDown:KeyInfoUp;
}

////////////////////////////////////////////////////////////////////////////////
// offline mouse history tracking
typedef struct MouseInput_s {
  int button;
  int state;
  float x, y;
  float time;
} MouseInput_t;

static MouseInput_t l_mouseHistory[QUEUE_SIZE];
static unsigned int l_mouseHistory_start = 0, l_mouseHistory_end = 0;

static void PubInput_mouseClickCallback (float x, float y, int button, int state)
{
  int pos;
  LUX_DEBUG_FRPRINT("Mouse Click: %f %f %i %i\n",x, y, button, state); LUX_DEBUG_FRFLUSH;
  pos = l_mouseHistory_end++%QUEUE_SIZE;
  if (l_mouseHistory_start == l_mouseHistory_end) l_mouseHistory_end++;
  l_mouseHistory[pos].button = button;
  l_mouseHistory[pos].state = state;
  l_mouseHistory[pos].x = x;
  l_mouseHistory[pos].y = y;
  l_mouseHistory[pos].time = (float)(getMicros()/1000000.0);
}

static void PubInput_mouseMotionCallback (float x, float y)
{
  int pos;
  LUX_DEBUG_FRPRINT("Mouse Move: %f %f\n",x,y); LUX_DEBUG_FRFLUSH;
  pos = l_mouseHistory_end++%QUEUE_SIZE;
  if (l_mouseHistory_start == l_mouseHistory_end) l_mouseHistory_end++;
  l_mouseHistory[pos].button = -1;
  l_mouseHistory[pos].state = 0;
  l_mouseHistory[pos].x = x;
  l_mouseHistory[pos].y = y;
  l_mouseHistory[pos].time = (float)(getMicros()/1000000.0);
}

////////////////////////////////////////////////////////////////////////////////
// the public interfaces
static int PubInput_popKey (PState pstate, PubFunction_t *f, int n)
{
  int pos;

  if (l_keyHistory_start==l_keyHistory_end) return 0;
  pos = l_keyHistory_start++%QUEUE_SIZE;
//  printf("%d %d\n",l_keyHistory_start,l_keyHistory_end);
  return FunctionPublish_setRet(pstate,3,
    LUXI_CLASS_INT,   l_keyHistory[pos].key,
    LUXI_CLASS_INT,   l_keyHistory[pos].keycode,
    LUXI_CLASS_BOOLEAN, l_keyHistory[pos].type!=KeyInfoUp);
}

static int PubInput_popMouse (PState pstate, PubFunction_t *f, int n)
{
  int pos;
//  LUX_PRINTF("%i %i\n",l_mouseHistory_start, l_mouseHistory_end);
  if (l_mouseHistory_start==l_mouseHistory_end) return 0;
  pos = l_mouseHistory_start++%QUEUE_SIZE;

  return FunctionPublish_setRet(pstate,5,
    LUXI_CLASS_FLOAT, lxfloat2void(l_mouseHistory[pos].x),
    LUXI_CLASS_FLOAT, lxfloat2void(l_mouseHistory[pos].y),
    LUXI_CLASS_INT,   l_mouseHistory[pos].button,
    LUXI_CLASS_BOOLEAN, l_mouseHistory[pos].state!=0,
    LUXI_CLASS_FLOAT, lxfloat2void(l_mouseHistory[pos].time));
}

static int PubInput_mousePos(PState pstate, PubFunction_t *f, int n)
{
  float x,y;
  if (n!=0 && n!=2) return FunctionPublish_returnError(pstate,"requires 0 or 2 arguments");
  if (n == 0)
    return FunctionPublish_setRet(pstate,2,
      LUXI_CLASS_FLOAT, lxfloat2void(g_CtrlInput.mousePosition[0]),
      LUXI_CLASS_FLOAT, lxfloat2void(g_CtrlInput.mousePosition[1]));
  if (FunctionPublish_getArg(pstate,2,LUXI_CLASS_FLOAT,&x,LUXI_CLASS_FLOAT,&y)!=2)
    return FunctionPublish_returnError(pstate,"arguments must be numbers");
  Controls_setMousePosition(x,y);
  return 0;
}

static int PubInput_mouseWheel(PState pstate, PubFunction_t *f, int n)
{
  int i;
  if (n!=0 && n!=2) return FunctionPublish_returnError(pstate,"requires 0 or 1 argument");
  if (n == 0)
    return FunctionPublish_returnInt(pstate,g_LuxiniaWinMgr.luxiGetMouseWheel());
  FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_INT,i);
  g_LuxiniaWinMgr.luxiSetMouseWheel(i);
  return 0;
}

static int PubInput_isKeyDown(PState pstate, PubFunction_t *f, int n)
{
  int i;
  FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_INT,i);
  return FunctionPublish_returnBool(pstate,Console_isActive() ? LUX_FALSE : g_LuxiniaWinMgr.luxiGetKey(i)==LUXI_PRESS);
}

static int PubInput_isMouseDown(PState pstate, PubFunction_t *f, int n)
{
  int i;
  int btn[] = {LUXI_MOUSE_BUTTON_0,LUXI_MOUSE_BUTTON_1,LUXI_MOUSE_BUTTON_2};
  FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_INT,i);
  if (i<0 || i>2)
    return FunctionPublish_returnError(pstate,"Mousebutton must be >= 0 and <3");
  return FunctionPublish_returnBool(pstate,g_LuxiniaWinMgr.luxiGetMouseButton(btn[i])==LUXI_PRESS);
}

typedef struct JoystickState_s {
  int queried;
  int present;
}JoystickState_t;

static JoystickState_t l_joystickState[LUXI_JOYSTICK_LAST];

static int PubInput_joystick(PState pstate, PubFunction_t *f, int n)
{
  float pos[8];
  int nr,btncnt,axiscnt,i;
  unsigned char buttons[32];

  if (n<1 || FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&nr)!=1)
    nr = 0;
  nr = (nr<0)?0:((nr>LUXI_JOYSTICK_LAST)?LUXI_JOYSTICK_LAST:nr);
  if ( !l_joystickState[nr].queried ){
    l_joystickState[nr].present = g_LuxiniaWinMgr.luxiGetJoystickParam(nr,LUXI_PRESENT);
    l_joystickState[nr].queried = LUXI_TRUE;
  }

  if ( l_joystickState[nr].present == LUXI_FALSE)
    return FunctionPublish_returnBool(pstate,0);
  btncnt = g_LuxiniaWinMgr.luxiGetJoystickParam(nr,LUXI_BUTTONS);
  axiscnt = g_LuxiniaWinMgr.luxiGetJoystickParam(nr,LUXI_AXES);

  if (n>1) FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&btncnt);
  if (n>1) FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&axiscnt);
  btncnt = (btncnt<0)?0:(btncnt>32)?32:btncnt;
  axiscnt = (axiscnt<0)?0:(axiscnt>8)?8:axiscnt;

  g_LuxiniaWinMgr.luxiGetJoystickPos(nr,pos,axiscnt);
  g_LuxiniaWinMgr.luxiGetJoystickButtons(nr,buttons,btncnt);
  FunctionPublish_setRet(pstate,4,LUXI_CLASS_BOOLEAN,1,LUXI_CLASS_INT,nr,LUXI_CLASS_INT,axiscnt,LUXI_CLASS_INT,btncnt);
  for (i=0;i<axiscnt;i++)
    FunctionPublish_setNRet(pstate,i+4,FNPUB_FFLOAT(pos[i]));
  for (i=0;i<btncnt;i++)
    FunctionPublish_setNRet(pstate,i+4+axiscnt,LUXI_CLASS_BOOLEAN,(void*)(int)buttons[i]);
  return 4+axiscnt+btncnt;
}

static int PubInput_nativecontrol(PState pstate, PubFunction_t *f, int n)
{
  booln on = 0;
  CtrlKey_t key = (CtrlKey_t)f->upvalue;


  if (n==1){
    if (FunctionPublish_getArg(pstate,1,LUXI_CLASS_BOOLEAN,&on)!=1)
      return FunctionPublish_returnError(pstate,"argument must be boolean");
    Controls_setCtrlKey(key,on);
  }
  else
    return FunctionPublish_returnBool(pstate,(char)Controls_getCtrlKey(key));

  return 0;
}

static int PubInput_showmouse (PState pstate, PubFunction_t *fn, int n)
{
  static booln show = 0;
  if (n==0) return FunctionPublish_returnBool(pstate,show);
  FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_BOOLEAN,show);
  if (show) g_LuxiniaWinMgr.luxiEnable(LUXI_MOUSE_CURSOR);
  else g_LuxiniaWinMgr.luxiDisable(LUXI_MOUSE_CURSOR);
  return 0;
}

static int PubInput_fixmouse (PState pstate, PubFunction_t *fn, int n)
{
  static booln fix = 1;
  if (n==0) return FunctionPublish_returnBool(pstate,fix);
  FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_BOOLEAN,fix);
  if (fix) g_LuxiniaWinMgr.luxiEnable(LUXI_MOUSE_FIX);
  else g_LuxiniaWinMgr.luxiDisable(LUXI_MOUSE_FIX);
  return 0;
}


void PubClass_Input_init()
{
  FunctionPublish_initClass(LUXI_CLASS_INPUT,"input","Luxinia's function publishing \
 does not offer direct calling of external functions if an event arises, such as key or\
 mouseinputs. Instead, Luxinia saves event occurances in queues that can be processed\
 when the scripting language is running.",
    NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_isKeyDown,NULL,"iskeydown",
    "(boolean isDown):(int keycode) - returns true if the key is pressed.");
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_isMouseDown,NULL,"ismousedown",
    "(boolean isDown):(int mousebutton) - returns true if the mousebutton is \
currently hold down. The mousebutton must be >=0 and <3 where 0 is the left, 1 the \
right and 2 the middle mousebutton. ");
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_popKey,NULL,"popkey",
    "([int key, int keycode, boolean isDown]):() - pops a key from keyhistoryqueue. \
The key is the resulted char of the keyboard. The keycode is the number on the keyboard, \
which doesn't have to be a printable letter. In case of unprintable letters, key is 0.");
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_popMouse,NULL,"popmouse",
    "([float x,y, int button, boolean isDown,float time_seconds]):() - pops a mouseevent from queue. -1 button means no button changes (only motion).");
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_mousePos,NULL,"mousepos",
    "([float x,y]):([float x,y]) - sets or gets mouseposition in reference coordinates (see window.refsize)");
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_mouseWheel,NULL,"mousewheel",
    "([int]):([int]) - sets or gets mousewheelposition");

  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_nativecontrol,(void*)CTRLKEY_QUIT,"escquit",
    "([boolean on]):([boolean on]) - sets or gets quit when esc is pressed");
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_nativecontrol,(void*)CTRLKEY_SCREENSHOTS,"intscreenshot",
    "([boolean on]):([boolean on]) - sets or gets if internal screenshot is allowed (F3 jpg, F4 tga).");
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_nativecontrol,(void*)CTRLKEY_CONSOLE,"intconsole",
    "([boolean on]):([boolean on]) - sets or gets if internal console is allowed (F1).");
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_nativecontrol,(void*)CTRLKEY_RECORD,"intrecord",
    "([boolean on]):([boolean on]) - sets or gets if internal record via auto screenshot is allowed (F11).");

  memset(l_joystickState,0,sizeof(l_joystickState));
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_joystick,NULL,"joystick",
    "(boolean exists,[int joysticknr,int axiscount,int btncnt, float pos1,..., boolean btn1,...])\
:([int nr]) - returns joystick position and currently pressed buttons or false, if \
the given number does not exist. If no number is passed, joystick 0 is used. nr must be \
smaller than 16 and >= 0. Luxinia limits the number of axis to 8 and the number \
of buttons to 32, which should be enough.");
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_fixmouse,NULL,"fixmouse",
    "([boolean fixed]):(boolean fix) - If the OS mousecursor is not shown, this flag is "
    "either locking the mousecursor at the center of the window or disallows it. "
    "Per default, this value is set to true.");
  FunctionPublish_addFunction(LUXI_CLASS_INPUT,PubInput_showmouse,NULL,"showmouse",
    "():(boolean visible) - sets mouse visibility of the operating system. \
Only if the mouse is visible, it can be moved out from the window. The invisible \
mouse may have coordinates that are outside the window. Initially, the mouse is \
invisible. If the console is activated and deactivated, the visibilitystatus is \
set to invisible again.");

  // register callbacks to catch all userinputs
  Controls_setKeyCallback(PubInput_callback);
  Controls_setMouseClickCallback(PubInput_mouseClickCallback);
  Controls_setMouseMotionCallback(PubInput_mouseMotionCallback);
}
