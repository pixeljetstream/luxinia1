// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "perfgraph.h"
#include "common.h"
#include "3dmath.h"
#include "timer.h"
#include "vid.h"
#include "../render/gl_render.h"
#include "../render/gl_window.h"

// LOCALS
static PGraphData_t l_graphdata;

//////////////////////////////////////////////////////////////////////////
// PGraphs

PGraphData_t* PGraphData_get()
{
  return &l_graphdata;
}

void  PGraphs_init()
{
  int i;

  memset(l_graphdata.graphs,0,sizeof(PGraph_t)*PGRAPHS);
  for (i = 0; i < PGRAPHS; i++)
    l_graphdata.graphs[i].max = 16.667f;

  l_graphdata.lastdraw = 0;

  lxVector3Set(l_graphdata.pos,PGRAPH_POSX,PGRAPH_POSY,PGRAPH_POSZ);
  lxVector2Set(l_graphdata.size,PGRAPH_WIDTH,PGRAPH_HEIGHT);

  PGraph_attribue(PGRAPH_RENDER,  LUX_FALSE,  0,1,0);
  PGraph_attribue(PGRAPH_L3D,   LUX_TRUE, 0,1,0);
  PGraph_attribue(PGRAPH_L2D,   LUX_TRUE, 0,0.7,0);
  PGraph_attribue(PGRAPH_PARTICLE,LUX_TRUE, 0.8,1,0);

  PGraph_attribue(PGRAPH_VISTEST, LUX_FALSE,  0,1,1);

  PGraph_attribue(PGRAPH_THINK, LUX_FALSE,  1,0,0);
  PGraph_attribue(PGRAPH_ODESTEP, LUX_FALSE,  0,0,1);

  PGraph_attribue(PGRAPH_FRAME, LUX_FALSE,  1,1,1);

  PGraph_attribue(PGRAPH_USER0, LUX_FALSE,  0,0,0);
  PGraph_attribue(PGRAPH_USER1, LUX_FALSE,  0,0,0);
  PGraph_attribue(PGRAPH_USER2, LUX_FALSE,  0,0,0);
  PGraph_attribue(PGRAPH_USER3, LUX_FALSE,  0,0,0);

}

  // reset all graphs
void  PGraphs_reset()
{
  int i;
  PGraph_t *graph = l_graphdata.graphs;

  for (i=0; i < PGRAPHS; i++,graph++){
    memset(graph->samples,0,sizeof(float)*PGRAPH_SAMPLESIZE);
    memset(graph->caches,0,sizeof(float)*PGRAPH_CACHESIZE);
    graph->cacheIndex = 0;
    graph->lasttime = 0;
    graph->sampleIndex = 0;
  }

}

//////////////////////////////////////////////////////////////////////////
// PGraph

PGraph_t *  PGraph_get(PGraphType_t type){
  return &l_graphdata.graphs[type];
}

  // color and maximum
void  PGraph_attribue(PGraphType_t type,int stipple, float r, float g, float b )
{
  l_graphdata.graphs[type].stipple = stipple;
  l_graphdata.graphs[type].color[0] = r;
  l_graphdata.graphs[type].color[1] = g;
  l_graphdata.graphs[type].color[2] = b;
}


void  PGraph_cache(PGraphType_t type)
{
  PGraph_t *graph = &l_graphdata.graphs[type];
  int i;
  float *flt;
  float *flttarget;

  if (!graph->cacheIndex)
    return;

  flt = graph->caches;
  flttarget = &graph->samples[graph->sampleIndex];
  *flttarget = 0.0f;
  for (i = 0; i < graph->cacheIndex; i++, flt++)
    *flttarget += *flt;

  *flttarget /= graph->cacheIndex;

  graph->cacheIndex = 0;

  graph->sampleIndex++;
  graph->sampleIndex%= PGRAPH_SAMPLESIZE;
}

  // max is the horizontal max value, min is 0
  // assumes a orthographic projection matrix

void  PGraph_draw(PGraphType_t type)
{
  ushort stipple = 3 | (3<<4) | (3<<8) | (3<<12);
  PGraph_t *graph = &l_graphdata.graphs[type];
  float flt;
  int i;

  vidClearTexStages(0,i);
  vidNoDepthTest(LUX_TRUE);
  vidNoDepthMask(LUX_TRUE);

  // if its been too long since anything was drawn just reset samples
  if (g_LuxTimer.time > l_graphdata.lastdraw + PGRAPH_SAMPLESIZE*PGRAPH_TIMESTEP){
    PGraphs_reset();
  }

  // update samples from cache after a bit of time
  if (g_LuxTimer.time > graph->lasttime+PGRAPH_TIMESTEP){
    PGraph_cache(type);
    graph->lasttime = g_LuxTimer.time;
  }

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // ,
  glTranslatef(VID_REF_WIDTH-640+ l_graphdata.pos[0],VID_REF_HEIGHT-480+l_graphdata.pos[1],l_graphdata.pos[2]);
  glScalef((float)l_graphdata.size[0]/(float)PGRAPH_SAMPLESIZE,(float)l_graphdata.size[1]/graph->max,1.0f);

  // we draw the lines from the "back", so that most recent is on the right
  glLineWidth(3);
  glDisable(GL_LINE_STIPPLE);
  // draw once in black non stippled
  glColor3f(0.1,0.1,0.1);
  glBegin(GL_LINE_STRIP);
  for (i = 0; i < PGRAPH_SAMPLESIZE; i++){
    flt = graph->samples[(graph->sampleIndex-i-1+PGRAPH_SAMPLESIZE)%PGRAPH_SAMPLESIZE];
    flt = LUX_MIN (flt,graph->max);
    flt = graph->max - flt;
    glVertex3f(PGRAPH_SAMPLESIZE-1.0f-i,flt,1.0f);
  }
  glEnd();
  glLineWidth(1);
  if (graph->stipple){
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1,stipple);
  }
  else
    glDisable(GL_LINE_STIPPLE);

  glColor3f(graph->color[0],graph->color[1],graph->color[2]);
  glBegin(GL_LINE_STRIP);
  for (i = 0; i < PGRAPH_SAMPLESIZE; i++){
    flt = graph->samples[(graph->sampleIndex-i-1+PGRAPH_SAMPLESIZE)%PGRAPH_SAMPLESIZE];
    flt = LUX_MIN (flt,graph->max);
    flt = graph->max - flt;
    glVertex3f(PGRAPH_SAMPLESIZE-1.0f-i,flt,0.0f);
  }
  glEnd();
  glDisable(GL_LINE_STIPPLE);
  l_graphdata.lastdraw = g_LuxTimer.time;
}

  // add sample to the graph cache
  // samples should be positive
void  PGraph_addSample(PGraphType_t type,float sample)
{
  PGraph_t *graph = &l_graphdata.graphs[type];
  graph->caches[graph->cacheIndex] = sample;
  graph->cacheIndex++;
  // not perfect but well normally noone should ever run over the cache
  if (graph->cacheIndex >= PGRAPH_CACHESIZE)
    l_graphdata.graphs[type].cacheIndex = PGRAPH_CACHESIZE-1;
}
