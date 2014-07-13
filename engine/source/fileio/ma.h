// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __MA_H__
#define __MA_H__

#include "../common/common.h"
#include "../resource/animation.h"

#define MA_SIGNATURE ("MotionAnimationASCII")
#define MA_VERSION   (100)

int   fileLoadMA(const char *filename,Anim_t *anim, void *unused);

#endif

