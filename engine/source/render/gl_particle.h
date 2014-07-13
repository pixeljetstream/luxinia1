// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_PARTICLE_H__
#define __GL_PARTICLE_H__

/*
GL_PARTICLE
-----------

The ParticleBuffer is used to flush the particles to screen.
It will generate the appropriate geometry (normally billboards)
and set texturing. This is done on a per frame basis so quite costly.
Based on hardware the billboard generation can be moved to GPU


Author: Christoph Kubisch

*/

#include "../common/types.h"
#include "../resource/particle.h"

#define GLPARTICLE_MAX_MESHVERTICES   32
#define GLPARTICLE_MAX_MESHINDICES    96

typedef enum GLParticleProcess_e{
  GLPARTICLE_PROCESS_ROT_NONE = 0,  // must stay 0
  GLPARTICLE_PROCESS_ROT_VEL,
  GLPARTICLE_PROCESS_ROT,
  GLPARTICLE_PROCESS_ROT_AND_VEL,
  GLPARTICLE_PROCESS_ROT_CAMFIX,
  GLPARTICLE_PROCESS_COLOR,
  GLPARTICLE_PROCESS_COLOR_VAR,
  GLPARTICLE_PROCESS_COLOR_NOAGE,
  GLPARTICLE_PROCESS_COLOR_NOAGE_VAR,
  GLPARTICLE_PROCESS_COLOR_NONE,
  GLPARTICLE_PROCESS_COLOR_USERMUL,
  GLPARTICLE_PROCESS_SIZE,
  GLPARTICLE_PROCESS_SIZE_VAR,
  GLPARTICLE_PROCESS_SIZE_USERMUL,
  GLPARTICLE_PROCESS_TEX,
  GLPARTICLE_PROCESS_TEX_NOAGE,
  GLPARTICLE_PROCESS_TURBULENCE,
  GLPARTICLE_PROCESS_SORTDIST,
  GLPARTICLE_PROCESS_FADEOUT,
}GLParticleProcess_t;

// we never use the struct, but just need it for proper sizes
typedef struct GLParticleBatchVertex_s{
  lxVector4   pos;
  lxVector2   tex;
  short   normal[4];
}GLParticleBatchVertex_t;

typedef struct GLParticleMesh_s{
  int     hasnormals;   // individual normals or just the first one for all
  VIDBuffer_t vbo;
  VIDBuffer_t ibo;
  Mesh_t    mesh;

  GLParticleBatchVertex_t   *batchverts;  // contains data for following pointer
  lxVector4   *pos;
  lxVector2   *tex;
  short   *normal;
  ushort    *indices;
}GLParticleMesh_t;

typedef struct GLParticle_s{
  union{
    ParticleDyn_t   *prtDyn;
    ParticleStatic_t  *prtSt;
  };
  float   *viewpos;
  union{
    uchar   colorb[4];
    uint    colorc;
  };
  float   *normal;
  float   size;
  float   texoffset;
  uint    timeStep;
  float   relage;
  float   rot;
  float   cosf;
  float   sinf;
  float   speed;
}GLParticle_t;

typedef struct GLParticleBuffer_s{
  lxMatrix44SIMD  axisMatrix;

  ParticleContainer_t* cont;
  int     numCont;

  float   texwidth;

  int     sort;   // if sorting is needed and in which direction

  int     rot;
  int     combdraw;
  int     normalsset;   // 0 none 1 mesh 2 per particle
  int     colorset;
  int     texset;

  int     useGlobalAxis;
  int     usevprog;
  int     usevbo;
  int     useoffset;
  int     lastvprogParticletype;
  int     lastvprogRot;
  int     lastGlobalAxis;
  int     lastUseOffset;
  int     lastLm;

  float     *offset;
  lxVector3     lastOffset;

  GLParticleMesh_t  quadmesh;
  GLParticleMesh_t  trimesh;
  GLParticleMesh_t  hspheremesh;

  float     *pPos;
  float     *pTex;
  short     *pNormal;

  lxVector4     poscache[32]; // for offset

  GLParticle_t  *drawParticles;
  float     drawZ[MAX_PARTICLES];
  uint      drawIndices[MAX_PARTICLES*2];

  uint      *intColorsC;
  int       *colorVarB;
  int       *colorVarB2;

  uint      vboID[2];
  int       curVBO;

  enum32      renderflag;

  int       drawSize;   // number of particles in draw array
}GLParticleBuffer_t;

//////////////////////////////////////////////////////////////////////////
// Drawing

  // RPOOLUSE
void Draw_ParticleSys(ParticleSys_t *psys, struct List3DView_s *l3dview,enum32 forceflags, enum32 ignoreflags);
void Draw_ParticleCloud(ParticleCloud_t *pcloud, struct List3DView_s *l3dview,enum32 forceflags, enum32 ignoreflags);

//////////////////////////////////////////////////////////////////////////
// GLParticleBuffer

void GLParticleBuffer_init();
void GLParticleBuffer_deinit();


//////////////////////////////////////////////////////////////////////////
// GLParticleMesh

GLParticleMesh_t * GLParticleMesh_new(Mesh_t *mesh);

void  GLParticleMesh_initBatchVerts(GLParticleMesh_t *glmesh);
void  GLParticleMesh_free(GLParticleMesh_t *glmesh,int dontfreeself);

#endif
