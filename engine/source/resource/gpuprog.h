// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GPUPROG_H__
#define __GPUPROG_H__

/*
GPUPROG
-------

GPU Programs simply contain instructions for programmable GPUs.
Currently ARB Vertex/Fragment Program are supported along with Cg.
They can only be used if apropriate hardware is found, and are normally
loaded by Shaders

Author: Christoph Kubisch

*/

#include "../common/common.h"

typedef enum GPUProgType_e{
  GPUPROG_UNSET = 0,
  GPUPROG_V_FIXED = 1,
  GPUPROG_F_FIXED = 2,
  GPUPROG_G_FIXED = 3,
  GPUPROG_V_CG  = 4,
  GPUPROG_F_CG  = 5,
  GPUPROG_G_CG  = 6,
  GPUPROG_V_GLSL  = GL_VERTEX_SHADER_ARB,
  GPUPROG_F_GLSL  = GL_FRAGMENT_SHADER_ARB,
  GPUPROG_G_GLSL  = GL_GEOMETRY_SHADER_ARB,
  GPUPROG_V_ARB = GL_VERTEX_PROGRAM_ARB,
  GPUPROG_F_ARB = GL_FRAGMENT_PROGRAM_ARB,
}GPUProgType_t;

#define GPUPROG_MAX_COMPILERARG_LENGTH    512
#define GPUPROG_MAX_PARAMNAME_LENGTH    32
#define GPUPROG_CG_ANNO_RES         "ResRID"

#define GPUPROG_IS_CG(test)         ((test) == GPUPROG_V_CG || (test) == GPUPROG_F_CG || (test) == GPUPROG_G_CG)

// for GLSL
typedef struct GpuProgInstanceGLSL_s{
  uint        progID;
  // must contain array of native paramids, which shaderparam->id can index
  int       numParamIDs;
  uint      *paramIDs;
  // issue, how to find length ?
  int       *linkparams;
}GpuProgInstanceGLSL_t;


typedef struct GpuProgParam_s{
  const char    *name;
  CGparameter   cgparam;
}GpuProgParam_t;

typedef struct GpuProg_s{
  ResourceInfo_t  resinfo;

  GPUProgType_t type;
  uint      progID;
  CGprofile   cgProfile;
  CGprogram   cgProgram;
  int       lowCgProfile;
  int       vidtype;

  char      *source;
  size_t      sourcelen;

  // for Cg, shared connect hosts, shared among all gpuprogs of the same "group"
  int       numCgparams;
  GpuProgParam_t  *cgparams;
}GpuProg_t;

void  GpuProg_fixSource(GpuProg_t *prog);

void  GpuProg_clear(GpuProg_t *prog);
void  GpuProg_clearGL(GpuProg_t *prog);
int   GpuProg_initGL(GpuProg_t *prog);
void  GpuProg_getGL(GpuProg_t *prog);

CGparameter CGprogram_GroupAddOrFindParam(CGprogram *usedProgs, const int numUsedProgs, const char *name,CGtype cgtype, const int arraycnt);

#endif
