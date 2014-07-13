// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __PERFGRAPH_H__
#define __PERFGRAPH_H__

#define PGRAPH_SAMPLESIZE 128
#define PGRAPH_CACHESIZE  128
#define PGRAPH_TIMESTEP   300

#define PGRAPH_POSX     320
#define PGRAPH_POSY     240
#define PGRAPH_POSZ     126
#define PGRAPH_WIDTH    320
#define PGRAPH_HEIGHT   240

typedef enum PGraphType_e{// draw priority
  PGRAPH_FRAME,
  PGRAPH_RENDER,
  PGRAPH_L2D,
  PGRAPH_PARTICLE,
  PGRAPH_L3D,
  PGRAPH_VISTEST,
  PGRAPH_ODESTEP,
  PGRAPH_THINK,
  PGRAPH_USER0,
  PGRAPH_USER1,
  PGRAPH_USER2,
  PGRAPH_USER3,
  PGRAPHS
}PGraphType_t;

typedef struct PGraph_s{
  float   samples[PGRAPH_SAMPLESIZE];
  int     sampleIndex;
  float   caches[PGRAPH_CACHESIZE];
  int     cacheIndex;
  double    time;
  float   max;
  float   color[4];
  int     stipple;
  unsigned long lasttime;
}PGraph_t;

typedef struct PGraphData_s{
  PGraph_t  graphs[PGRAPHS];
  float   pos[3];
  float   size[2];
  unsigned long   lastdraw;
}PGraphData_t;


//////////////////////////////////////////////////////////////////////////
// PGraphs

  // init pgraphs
void  PGraphs_init();

  // reset all graphs
void  PGraphs_reset();

PGraphData_t* PGraphData_get();

//////////////////////////////////////////////////////////////////////////
// PGraph

PGraph_t *  PGraph_get(PGraphType_t type);

  // color and maximum
void  PGraph_attribue(PGraphType_t type, int stipple, float r, float g, float b);

  // caches are turned to samples, depending on time spent and then drawn
  // assumes a orthographic projection matrix
void  PGraph_draw(PGraphType_t type);

  // add sample to the graph cache
  // samples should be positive
void  PGraph_addSample(PGraphType_t type,float sample);


#endif
