// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "project.h"
#include "os.h"
#include "resource.h"

#ifndef LUX_PLATFORM_WINDOWS
#error "Wrong Platform"
#endif


#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Winbase.h>
#include <tchar.h>


void    ProjectPostErrorDump(const char *filename)
{
  //const char *resstring = osGetResourceString();
  // we either have a filename or not (success of write)

  /*

  MessageBox(NULL,_T(""),_T(""), MB_OK | MB_ICONERROR | MB_TASKMODAL);
  ShellExecute(0,
  "open",
  "mailto:" + lcEmailAddress + "?Subject=" + lcSubject +
  "&CC=" + lcCCAddress + "&body=" + lcMessageBody,
  "",
  "",1);
  */
}