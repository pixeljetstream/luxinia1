// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __F3D_H__
#define __F3D_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#define F3D_HEADER 0x1F3D


#define F3D_VERSION_23    230
  // vertex 52

#define F3D_VERSION_24    240
  // vertex 64_old

#define F3D_VERSION_25    250
  // vertex 64_new

#define F3D_VERSION_26    260
  // ints for (index,vertex)counters, int indices for meshes with vertex count > USHORT




#include "../common/common.h"
#include "../resource/model.h"

int fileLoadF3D(const char * filename,Model_t *model, void *unused);


#if defined(__cplusplus)
}
#endif

#endif
