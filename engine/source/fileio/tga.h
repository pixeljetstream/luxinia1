// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __TGA_H__
#define __TGA_H__

#include "../resource/texture.h"

int fileLoadTGA(const char * filename,Texture_t *texture, void *unused);

int fileSaveTGA(const char * filename, short int width, short int height, int channels, unsigned char *imageBuffer, int isbgra);

#endif

