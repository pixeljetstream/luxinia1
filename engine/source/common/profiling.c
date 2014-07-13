// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include <string.h>
#include <stdio.h>

#include "profiling.h"



// GLOBALS
ProfileData_t g_Profiling;

//////////////////////////////////////////////////////////////////////////
void  ProfileData_clearAll()
{
  memset(&g_Profiling,0,sizeof(ProfileData_t));
}
void  ProfileData_clearPerFrame()
{
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.renderpoolmax,g_Profiling.perframe.scratch.renderpoolmax);

  memset(&g_Profiling.perframe,0,sizeof(ProfPerFrame_t));
}

//////////////////////////////////////////////////////////////////////////
// just for stats string

#include "memorymanager.h"
#include "../render/gl_list3d.h"

extern int    Reference_getTotalCount ();
extern size_t ResData_getTotalMemUse();
extern size_t ResData_getTotalMem();
extern char*  lxStrFormatedNumber(char *buffer,size_t size, int number,int sep);

#define TOPCT(val,max)  ((int)(100.0f*((double)val)/((double)max)))


const char* ProfileData_getStatsString()
{
  static char temp[256];
  static char output[4096];
  static char buffer[16];
  static char buffer1[16];
  static char buffer2[16];
  static char buffer3[16];
  static char buffer4[16];

  ProfTimers_t  *timers = &g_Profiling.perframe.timers;
  ProfVischeck_t  *vischeck = &g_Profiling.perframe.vischeck;
  ProfParticle_t  *particle = &g_Profiling.perframe.particle;
  ProfDraw_t    *draw = &g_Profiling.perframe.draw;
  ProfMemScratch_t *scratch = &g_Profiling.perframe.scratch;

  ProfMemory_t  *memory = &g_Profiling.global.memory;
  ProfNodes_t   *nodes = &g_Profiling.global.nodes;
  lxMemoryGenericInfo_t geninfo = lxMemoryGeneric_getInfo(g_GenericMem);

  temp[0] = 0;
  output[0] = 0;

  sprintf(temp,
    "l3d:%4d,draws:%3d,passes+:%3d,batched:%3d,fxlights:%3d,projs:%3d\n",
    draw->l3ds,draw->TDraws,draw->TDraws-draw->TMeshes,draw->TBatches,draw->fxlights,draw->projectors);
  strcat(output,temp);

  sprintf(temp,
    "l3d:tris:%s,verts:%s|nodes:act:%3d,sct:%3d|vistest:cam:%3d/%3d,proj:%3d/%3d\n",
    lxStrFormatedNumber(buffer,16,draw->TTris,1),lxStrFormatedNumber(buffer1,16,draw->TVertices,2),
    nodes->actors,nodes->scenenodes,
    vischeck->frustumChecksCam,vischeck->quickAcceptsCam,vischeck->frustumChecksProj,vischeck->quickAcceptsProj);
  strcat(output,temp);

  sprintf(temp,
    "l3d:multipass tris:%s,verts:%s\n",
    lxStrFormatedNumber(buffer,16,draw->MTTris-draw->TTris,2),lxStrFormatedNumber(buffer1,16,draw->MTVertices-draw->TVertices,2));
  strcat(output,temp);

  sprintf(temp,
    "l3dmax: perlayer: %s%s totalprojs: %s%s\n",
    lxStrFormatedNumber(buffer,16,nodes->maxlayerdrawnodes,1),
    nodes->maxlayerdrawnodes >= g_List3D.maxDrawPerLayer ? "!! ":"",
    lxStrFormatedNumber(buffer1,16,nodes->maxprojptrs,1),
    nodes->maxprojptrs >= g_List3D.maxTotalProjectors ? "!! ":"");
  strcat(output,temp);

  sprintf(temp,
    "prts:%5d,render:%5d,draws:%3d,tris:%s verts:%s\n",
    particle->active,particle->rendered,particle->meshes,lxStrFormatedNumber(buffer,16,particle->tris,1),lxStrFormatedNumber(buffer1,16,particle->vertices,2));
  strcat(output,temp);

  sprintf(temp,
    "genmem:%s, resmem:%s(%2d) bytes(pct)\n",
    lxStrFormatedNumber(buffer,16,geninfo.mem,2),
    lxStrFormatedNumber(buffer1,16,(int)ResData_getTotalMemUse(),2),
    TOPCT(ResData_getTotalMemUse(),ResData_getTotalMem()));
  strcat(output,temp);

  sprintf(temp,
    "poolrender:%s(%2d), poolbuffer:%s(%2d) bytes(pct)\n",
    lxStrFormatedNumber(buffer,16,scratch->renderpoolmax,2),
    TOPCT(scratch->renderpoolmax,lxMemoryStack_bytesTotal(g_BufferMemStack)),
    lxStrFormatedNumber(buffer1,16,memory->buffermax,2),
    TOPCT(memory->buffermax,lxMemoryStack_bytesTotal(g_BufferMemStack)));
  strcat(output,temp);

  {
    int totalcosts = memory->vidvbomem+memory->vidsurfmem+memory->viddlmem+memory->vidtexmem;
    int pct = g_VID.capVRAM ? TOPCT(totalcosts,g_VID.capVRAM) : -1;

    sprintf(temp,
      "vtex:%s vsurf:%s vbo:%s vdl:%s | %s (%2d) bytes(pct)\n",
      lxStrFormatedNumber(buffer,16,memory->vidtexmem,2),
      lxStrFormatedNumber(buffer1,16,memory->vidsurfmem,2),
      lxStrFormatedNumber(buffer2,16,memory->vidvbomem,2),
      lxStrFormatedNumber(buffer3,16,memory->viddlmem,2),
      lxStrFormatedNumber(buffer4,16,totalcosts,2),
      pct
      );
    strcat(output,temp);
  }


  sprintf(temp,
    "refcnt:%5d,gencnt: %4d,lua:%s / %s(bytes)\n",
      Reference_getTotalCount(),geninfo.allocs,lxStrFormatedNumber(buffer,16,memory->luausemem,2),lxStrFormatedNumber(buffer1,16,memory->luaallocmem,2));
  strcat(output,temp);

  return output;
}

#undef TOPCT
