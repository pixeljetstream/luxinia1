// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "timer.h"
#include <time.h>
#include "../common/common.h"
#include "../main/main.h"

// GLOBALS
extern LuxTimer_t g_LuxTimer = {0,0,0,0,0};

int getFPS() {
  return g_LuxTimer.fps;
}

int getFPSAvg() {
  return (int)(1000.0f/(float)g_LuxTimer.avgtimediff);
}

double getMicros()
{
  return (g_LuxiniaWinMgr.luxiGetTime()*1000000.0);
}

void stopwatch()
{
  static double l_timeSW    = 0;  // time for stopswatch

  if (l_timeSW != 0){
    l_timeSW += getMicros();
    LUX_PRINTF("Stopwatch: %d\n",l_timeSW);
    l_timeSW = 0;
  }
  else
    l_timeSW = -getMicros();

}


