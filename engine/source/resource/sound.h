// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __SOUND_H__
#define __SOUND_H__

#include <al/alut.h>
#include "../common/types.h"

typedef struct Sound_s
{
  ResourceInfo_t resinfo;
  ALuint buffer;
}Sound_t;

extern int g_nosound;

void Sound_clear(Sound_t *sound);
int  Sound_loadFile(const char* filename,Sound_t *snd,void *unused);

#endif


