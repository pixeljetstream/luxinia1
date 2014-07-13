// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __TIMER_H__
#define __TIMER_H__



typedef struct LuxTimer_s{
  unsigned long time;
  float     timef;
  float     timediff;
  double      avgtimediff;
  int       fps;
}LuxTimer_t;

extern LuxTimer_t g_LuxTimer;

int  getFPS();
int getFPSAvg();
double getMicros(); // return nanosecond time of board, could be wrong at times...
void stopwatch();     // simple stopwatch will print time passed by every second execution

#endif
