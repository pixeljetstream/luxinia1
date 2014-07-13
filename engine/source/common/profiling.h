// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __PROFILIING_H__
#define __PROFILIING_H__

#include <luxinia/luxplatform/luxplatform.h>
// Per Frame

typedef struct ProfTimers_s{
  double  render;
  double  l2d;
  double  l3d;
  double  vistest;
  double  particle;
  double  ode;
  double  frame;
  double  think;
} ProfTimers_t;

typedef struct ProfVischeck_s{
  int   frustumChecksCam;
  int   frustumChecksProj;
  int   quickAcceptsCam;
  int   quickAcceptsProj;
}ProfVischeck_t;

typedef struct ProfParticle_s{
  int   active;
  int   rendered;
  int   meshes;
  int   tris;
  int   vertices;
}ProfParticle_t;

typedef struct ProfDraw_s{
  int   fxlights;
  int   projectors;
  int   l3ds;

  unsigned int  TMeshes;
  unsigned int  TDraws;
  unsigned int  TTris;
  unsigned int  TVertices;
  unsigned int  MTTris;
  unsigned int  MTVertices;
  unsigned int  TBatches;
}ProfDraw_t;

typedef struct ProfMemScratch_s{
  unsigned int  renderpoolmax;
}ProfMemScratch_t;

typedef struct ProfPerFrame_s{
  ProfTimers_t    timers;
  ProfVischeck_t    vischeck;
  ProfParticle_t    particle;
  ProfDraw_t      draw;
  ProfMemScratch_t  scratch;
}ProfPerFrame_t;


// Global
typedef struct ProfMemory_s{
  size_t  buffermax;
  size_t  renderpoolmax;

  size_t  luausemem;
  size_t  luaallocmem;

  int   vidtexmem;
  int   vidvbomem;
  int   viddlmem;
  int   vidsurfmem;
}ProfMemory_t;

typedef struct ProfNodes_s{
  int   actors;
  int   scenenodes;
  int   l3ds;

  int   maxprojptrs;
  int   maxlayerdrawnodes;
}ProfNodes_t;

typedef struct PGlobal_s{
  ProfMemory_t memory;
  ProfNodes_t nodes;
}PGlobal_t;


// Combined
typedef struct ProfileData_s{
  ProfPerFrame_t  perframe;
  PGlobal_t   global;
}ProfileData_t;

extern ProfileData_t  g_Profiling;


void      ProfileData_clearAll();
void      ProfileData_clearPerFrame();
const char*   ProfileData_getStatsString();

#endif

