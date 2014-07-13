// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __JPG_H__
#define __JPG_H__

#include "../resource/texture.h"

int fileLoadJPG(const char *filename, Texture_t *texture, void *unused);

int fileSaveJPG(const char * filename, short int width, short int height, int channels,
  int quality, unsigned char *imageBuffer);

#endif
