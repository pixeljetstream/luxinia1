// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __MUSICFILE_H__
#define __MUSICFILE_H__

void Music_load(const char *oggfile);
void Music_play();
void Music_stop();

double Music_getTime();
void Music_setTime(double t);

void Music_setGain(double g);
int Music_isPlaying();

/*
  returns number of available comment fields, char pointer is set to point
  to that string (where each string is null terminated). The returned
  string is valid until the next call.
*/
int Music_getFileInfo (const char *file, const char ** to);

#endif
