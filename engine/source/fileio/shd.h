// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __SHD_H__
#define __SHD_H__

#include "../common/common.h"
#include "../resource/shader.h"
#include "../fileio/parser.h"

#define SHD_HEADER      "luxinia_Shader_v%d"
#define SHD_VER_MINIMUM   310
#define SHD_VER_CGLOADER  310

int fileLoadSHD(const char *filename,Shader_t *shader,void *unused);

#endif
