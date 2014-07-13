// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __DRAW2D_H__
#define __DRAW2D_H__

#include "gl_drawmesh.h"

/*
DRAW2D
------

just simple functions to draw textured Quad on screen


Author: Christoph Kubisch

*/


  // draws a screen image
void Draw_Texture_screen( float x, float y, float z, float w, float h, int texid, int blend, int setproj, enum32 forceflag, enum32 ignoreflag);
void Draw_Texture_cubemap(int texid, float x, float y, float size);


#endif

