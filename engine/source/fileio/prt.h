// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __PRT_H__
#define __PRT_H__

#include "../common/common.h"
#include "../resource/particle.h"
#include "../fileio/parser.h"

#define PRT_HEADER      "luxinia_ParticleSys_v%d"
#define PRT_VER_MINIMUM   120

int fileLoadPRT(const char *filename,ParticleSys_t *Psys,void *unused);

#endif
