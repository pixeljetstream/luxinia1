// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>

// LOG
#define lnofile(a) lprintf("Could not open file '%s'\n",a)

// Debug helpers
#ifdef LUX_DEVBUILD
  #define dprintf LUX_PRINTF
  #define dlprintf lprintf
#else
  #define dprintf //
  #define dlprintf //
#endif

// these two print the string to the log
#define lprintf LogPrintf
#define lvprintf LogVPrintf
// these two print the string additionally to the screen
#define bprintf LogPrintf2
#define bvprintf LogVPrintf2

void LogInit();
void LogDeinit();

// these two print the string to the log
void LogPrintf(char *string, ...);
void LogVPrint(char *string, va_list va);
// these two print the string additionally to the screen
void LogPrintf2(char *string, ...);
void LogVPrintf2(char *string, va_list va);

#endif
