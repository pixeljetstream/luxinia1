// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __MAIN_H__
#define __MAIN_H__

#include "../common/common.h"
#include <luxinia/luxinia.h>

#define LUX_BASEPATH  "base/"
#define LUX_TASKSTACK 32
#define LUX_AVGTIME 8

typedef enum LuxTask_e{
  LUX_TASK_MAIN,
  LUX_TASK_RENDER,
  LUX_TASK_SOUND,
  LUX_TASK_LUA,
  LUX_TASK_ACTOR,
  LUX_TASK_SCENETREE,
  LUX_TASK_INPUT,
  LUX_TASK_TIMER,
  LUX_TASK_EXIT,
  LUX_TASK_THINK,
  LUX_TASK_RESOURCE,
}LuxTask_t;

// GLOBALS
extern  char        *g_projectpath;
extern  LuxiniaWinMgr_t   g_LuxiniaWinMgr;


// luxinia main
void clearall(void);
int main (int, char**);

void Main_freezeTime();

int  Main_start(struct lua_State *Lstate,const int argc,const char **argv);
void Main_frame(void);

void Main_init();

  // if timediff is greater 0 it will be used, else automatic value
  // autoprocess will not perform luathink nor swapbuffers
  // drawfirst, then update scenenodes and alike, only affects non-auto mode
void Main_think(float timediff, int autoprocess, int drawfirst);

void Main_sleep(int timems);

void Main_startTask(LuxTask_t task);
void Main_endTask();

int  Main_exit(int nocleanup);
void Main_pause(int state);


#endif

