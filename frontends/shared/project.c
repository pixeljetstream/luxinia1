// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h

#include "project.h"
#include <assert.h>
#include <string.h>
#include "os.h"
#include "resource.h"
#include <resourcecustom.h>

#ifdef __unix__
#include <unistd.h>
#elif __WIN32__ || _MS_DOS_
#include <dir.h>
#else
#include <direct.h>
#endif

#define MAX_PATHLENGTH  512

static struct ProjInfo_s{
  char PathExe[MAX_PATHLENGTH];
  char PathWork[MAX_PATHLENGTH];
  char PathScreenshot[MAX_PATHLENGTH];
  char PathLog[MAX_PATHLENGTH];
  char PathConfig[MAX_PATHLENGTH];
}l_ProjData;

static void checkorcreatedir(char *dir)
{
  if (chdir(dir) == -1){
    mkdir(dir);
  }
}


void ProjectInit()
{
  static char pathname[MAX_PATHLENGTH];
  const char *osbasepath;
  const char *path;

  strncpy(l_ProjData.PathWork,osGetPaths("exepath"),sizeof(char)*MAX_PATHLENGTH);
  strncpy(l_ProjData.PathExe,osGetPaths("exefile"),sizeof(char)*MAX_PATHLENGTH);


  if ((osbasepath=osGetResourceString(IDS_PATH_OSUSER)) && (osbasepath=osGetPaths(osbasepath)))
  {
    strncpy(pathname,osbasepath,sizeof(char)*MAX_PATHLENGTH);
  }
  else{
    strncpy(pathname,l_ProjData.PathWork,sizeof(char)*MAX_PATHLENGTH);
  }

  path = osGetResourceString(IDS_PATH_USERSUB);
  strncat(pathname,path ? path : (osbasepath ? "/luxinia" : ""),(sizeof(char)*MAX_PATHLENGTH)-strlen(pathname));

  strncpy(l_ProjData.PathScreenshot,pathname,sizeof(char)*MAX_PATHLENGTH);
  strncpy(l_ProjData.PathConfig,pathname,sizeof(char)*MAX_PATHLENGTH);
  strncpy(l_ProjData.PathLog,pathname,sizeof(char)*MAX_PATHLENGTH);

  path = osGetResourceString(IDS_PATH_SCREENSHOTS);
  strncat(l_ProjData.PathScreenshot,path ? path : "/screenshots",(sizeof(char)*MAX_PATHLENGTH)-strlen(l_ProjData.PathScreenshot));
  path = osGetResourceString(IDS_PATH_CONFIG);
  strncat(l_ProjData.PathConfig,path ? path : "/base",(sizeof(char)*MAX_PATHLENGTH)-strlen(l_ProjData.PathConfig));
  path = osGetResourceString(IDS_PATH_LOG);
  strncat(l_ProjData.PathLog,path ? path : "",(sizeof(char)*MAX_PATHLENGTH)-strlen(l_ProjData.PathLog));

  // check for dirs
  checkorcreatedir(l_ProjData.PathScreenshot);
  checkorcreatedir(l_ProjData.PathConfig);
  checkorcreatedir(l_ProjData.PathLog);

  // back to workdir
  osSetWorkDir(l_ProjData.PathWork);

  {
    char PathCrashReport[MAX_PATHLENGTH];
    strncpy(PathCrashReport,l_ProjData.PathLog,sizeof(char)*MAX_PATHLENGTH);
    strncat(PathCrashReport,"/crashlog.dmp",(sizeof(char)*MAX_PATHLENGTH)-strlen(PathCrashReport));
#ifndef _DEBUG
    osErrorEnableHandler(PathCrashReport,ProjectPostErrorDump);
#endif
  }

}

const char *ProjectCustomStrings(const char *input)
{
  uint id = 0;
  if (strcmp(input,"pathconfig")==0)
    return l_ProjData.PathConfig;
  else if (strcmp(input,"pathscreenshot")==0)
    return l_ProjData.PathScreenshot;
  else if (strcmp(input,"pathlog")==0)
    return l_ProjData.PathLog;
  else if (strcmp(input,"pathwork")==0)
    return l_ProjData.PathWork;
  else if (strcmp(input,"pathexe")==0)
    return l_ProjData.PathExe;
  else if (strcmp(input,"logobig")==0)
    return osGetResourceString(IDS_CUSTOM_LOGOBIG);
  else if (strcmp(input,"logosmall")==0)
    return osGetResourceString(IDS_CUSTOM_LOGOSMALL);
  else if (strcmp(input,"logoclearcolor")==0)
    return osGetResourceString(IDS_CUSTOM_LOGOCLEARCOLOR);
  else if (strcmp(input,"logotime")==0)
    return osGetResourceString(IDS_CUSTOM_LOGOTIME);
  else if (sscanf(input,"resstr:%d",&id) ==1)
    return osGetResourceString(id);
#ifdef LUXINIA_FRONTEND_TYPE
  else if (strcmp(input,"frontendtype")==0)
    return LUXINIA_FRONTEND_TYPE;
#endif
  else
    return "";
}

void ProjectForcedArgs(int *pargc, char ***pargv)
{
  static const char *outargs[64];
  static char buffer[4096];

  const char **argv = (const char **)*pargv;
  int argc = *pargc;
  int i;
  const char *forced = osGetResourceString(IDS_FORCEDARGS);
  char *cur;
  char *last;
  int instring;

  if (!forced)
    return;

  for (i = 0; i < argc; i++)
    outargs[i] = argv[i];

  // parse forced string
  strncpy(buffer,forced,sizeof(char)*4096);
  cur = buffer;
  last = cur;

  instring = 0;
  while (*cur && i < 64){
    if (*cur == '"')
      instring = !instring;

    if (*cur == ' ' && !instring){
      *cur = 0;
      if (last < cur)
        outargs[i++] = last;
      last = cur+1;
    }
    cur++;
  }
  if (last < cur)
    outargs[i++] = last;

  *pargc = i;
  *pargv = outargs;
}