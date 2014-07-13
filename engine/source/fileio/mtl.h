// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __MTL_H__
#define __MTL_H__

#include "../common/common.h"
#include "../fileio/parser.h"
#include "../resource/material.h"

#define MTL_HEADER      "luxinia_Material_v%d"
#define MTL_VER_MINIMUM   110

int fileLoadMTL(const char *filename,Material_t *material,void *unused);

#endif
