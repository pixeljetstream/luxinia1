// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_TRAIL_H__
#define __GL_TRAIL_H__

#include "../common/common.h"
#include "../resource/texture.h"
#include "../common/reflib.h"
#include "../scene/linkobject.h"

#define TRAIL_STEPS 3

typedef struct TrailPoint_s{
  lxVector4   pos;
  lxVector4   normal;

  union{
    struct  {
      lxVector4   vel;
    };
    struct {
      Reference linkRef;
      LinkType_t  linkType;   // 0 s3d 1 act
      int     axis;
    };
  };
  ulong   updtime;
  float   _size0;
  float   _size1;
  uint    _pad;
}TrailPoint_t;

typedef enum TrailType_e{
  TRAIL_LOCAL,
  TRAIL_WORLD,
  TRAIL_WORLD_REF,
  TRAIL_WORLD_FORCE_DWORD = 0x7FFFFFFF,
}TrailType_t;

typedef struct Trail_s{
  TrailType_t type;
  int     recompile;
  int     uvnormalized;
  int     planarmap;
  float   planarscale;
  float   unormalizescale;
  float   uscale;
  int     camaligned;
  int     usevel;
  int     uselocaltime;
  ulong   localtime;
  int     closed;
  lxVector3   velocity;

  int     compile;
  int     compilestart;
  lxVector4   colors[TRAIL_STEPS];
  float   sizes[TRAIL_STEPS];

  int     drawlength;
  int     length;
  int     numTpoints;
  int     startidx;

  int     spawnstep;

  ulong   lastspawntime;

  lxVector3   lastpos;
  lxVector3   lastvel;
  lxVector3   lastnormal;

  Mesh_t    mesh;
  TrailPoint_t *tpoints;
}Trail_t;


//////////////////////////////////////////////////////////////////////////
// Trail

void  Trail_update(Trail_t* LUX_RESTRICT trail,lxMatrix44 mat);
TrailPoint_t* Trail_addPoint(Trail_t *trail,lxVector3 pos,lxVector3 normal,lxVector3 vel);
void  Trail_reset(Trail_t *trail);
void  Trail_setUVs(Trail_t *trail, float uscale, float vscale);
int   Trail_checkPoint(Trail_t *trail, TrailPoint_t *tpoint);

Trail_t*  Trail_new(int length);
void    Trail_free(Trail_t *trail);

int   Trail_closestDistance(Trail_t *trail, lxVector3 pos, float *odistSq, TrailPoint_t **otpointClosest, float *ofracc);
int   Trail_closestPoint(Trail_t *trail,lxVector3 pos,float *odistSq,TrailPoint_t **otpointClosest);

#endif

