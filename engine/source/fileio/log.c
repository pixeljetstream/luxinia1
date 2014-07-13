// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "log.h"
#include "../main/main.h"
#include "../common/common.h"
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

static FILE *l_logfile = NULL;

void LogInit()
{
  char logfilename[MAX_PATHLENGTH];
  FILE *file;
  time_t aclock;

  strncpy(logfilename,g_LuxiniaWinMgr.luxiGetCustomStrings("pathlog"),sizeof(char)*MAX_PATHLENGTH);
  strncat(logfilename,"/luxinia.log",(sizeof(char)*MAX_PATHLENGTH)-strlen(logfilename));


  file = fopen(logfilename, "w");

  time(&aclock);          // get timestamp

  fprintf(file,"=======================================\n");
  fprintf(file,"============= LUXINIA LOG =============\n");
  fprintf(file,"=======================================\n");
  fprintf(file," %s: %s", LUX_VERSION,asctime(localtime(&aclock))); // convert time to string
  fprintf(file,"=======================================\n");
  fprintf(file,"\n");

  fclose(file);

  l_logfile = fopen(logfilename, "a");

  fflush(NULL);
}

void LogDeinit()
{
  fclose(l_logfile);
}

void LogPrintf(char *string, ...)
{
  va_list va;

  va_start (va, string);
    vfprintf(l_logfile, string, va);
  va_end (va);

  fflush(NULL);
}

void LogVPrintf(char *string, va_list va)
{
  vfprintf(l_logfile, string, va);

  fflush(NULL);
}

void LogPrintf2(char *string, ...)
{
  va_list va;

  va_start (va, string);
    // to screen
    g_LuxiniaWinMgr.luxiPrint(string, va);
    // to file
    vfprintf(l_logfile, string, va);
  va_end (va);

  fflush(NULL);
}

void LogVPrintf2(char *string, va_list va)
{
  // to screen
  g_LuxiniaWinMgr.luxiPrint(string, va);

  // to file
  vfprintf(l_logfile, string, va);

  fflush(NULL);
}

