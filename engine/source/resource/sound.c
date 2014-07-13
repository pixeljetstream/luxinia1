// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/3dmath.h"
#include "sound.h"
#include "resmanager.h"
#include "../fileio/filesystem.h"

// GLOBALS
int g_nosound = LUX_FALSE;


void Sound_clear(Sound_t *sound)
{

  if (sound->buffer)
    alDeleteBuffers(1, &sound->buffer);
}

int Sound_loadFile(const char* filename,Sound_t *tempsnd,void *unused)
{
  lxFSFile_t *file;
  ALbyte *memory;

  if (g_nosound)
    return LUX_TRUE;

  file = FS_open(filename);
  if(file == NULL)
  {
    lprintf("ERROR wavload: ");
    lnofile(filename);
    return LUX_FALSE;
  }
  memory = lxFS_getContent(file);

  tempsnd->buffer = alutCreateBufferFromFileImage (lxFS_getContent(file),lxFS_getSize(file));
  FS_close(file);

  if(alGetError() != AL_NO_ERROR){
    return LUX_FALSE;
  }

  return LUX_TRUE;
}

