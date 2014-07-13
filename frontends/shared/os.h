// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h

#ifndef __OS_HELPERS_H__
#define __OS_HELPERS__H__

#include <luxinia/luxplatform/luxplatform.h>

#ifdef __cplusplus
extern "C" {
#endif

int     osGetDrives(char ** outnames,int maxnames,int maxlength);
const char* osGetDriveLabel(char *drive);
int     osGetDriveSize(char *drive,double *freetocaller, double *total, double *totalfree);
const char* osGetDriveType(char *drive);
const char* osGetOSString( void );
const char* osGetMACaddress( void );
void    osSwapInterval ( int interval );  // needs GL context bound
void    osGetScreenSizeMilliMeters(int *w,int *h);
void    osGetScreenRes(int *w, int *h);
unsigned int osGetVideoRam( void );
void    osSetWorkDir(const char* wpath);

  // "userappdata", "userdocs", "commondocs", "commonappdata","exefile","exepath"
const char* osGetPaths(const char *input);
const char* osGetResourceString(unsigned int uID);

  // performs crash analysis (writes minidump,crashdump..)

typedef void (osErrorPostReport_fn)(const char *reportfilename);

void    osErrorEnableHandler(const char *reportfilename,  osErrorPostReport_fn  *func);

#ifdef __cplusplus
}
#endif

#endif