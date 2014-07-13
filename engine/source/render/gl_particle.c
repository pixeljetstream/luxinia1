// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h




#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"

#include "gl_particle.h"
#include "gl_camlight.h"
#include "gl_shader.h"
#include "gl_render.h"
#include "gl_list3d.h"
#include "gl_drawmesh.h"

#include <luxinia/luxcore/sortradix.h>

#include <float.h>
// the draw buffer
static GLParticleBuffer_t l_PrtBuffer;
void  GLParticleMesh_InstFromBatch(GLParticleMesh_t *glmesh);


//extern double l_particletime;

//////////////////////////////////////////////////////////////////////////
// GLParticleBuffer

void  GLParticleBuffer_init()
{
  // set-up base meshes
  GLParticleMesh_t *glmesh;
  int i;
  float normhsphere[]={0.0,0.0,1.0, 0.0,0.575968,0.817472, -0.547778,0.177984,0.817472, -0.338546,-0.465968,0.817472, 0.338546,-0.465968,0.817472, 0.547778,0.177984,0.817472};

  memset(&l_PrtBuffer,0,sizeof(GLParticleBuffer_t));


  // setup hsphere
  glmesh = &l_PrtBuffer.hspheremesh;
  glmesh->mesh.primtype = PRIM_TRIANGLES;
  glmesh->mesh.numAllocIndices = glmesh->mesh.numIndices = 15;
  glmesh->mesh.numVertices = 6;
  glmesh->hasnormals = LUX_TRUE;
  glmesh->mesh.indicesData16 = lxMemGenZalloc(sizeof(ushort)*glmesh->mesh.numAllocIndices);
  GLParticleMesh_initBatchVerts(glmesh);
  LUX_ARRAY3SET(&glmesh->indices[0], 0, 1, 2);
  LUX_ARRAY3SET(&glmesh->indices[3], 0, 2, 3);
  LUX_ARRAY3SET(&glmesh->indices[6], 0, 3, 4);
  LUX_ARRAY3SET(&glmesh->indices[9], 0, 4, 5);
  LUX_ARRAY3SET(&glmesh->indices[12], 0, 5, 1);
  // center, up, upleft,dwnleft,dwnright,upright
  lxVector4Set(glmesh->pos[0],  0.0,0.0,0.0,1.0);
  lxVector4Set(glmesh->pos[1],  0.0,1.0,-0.727615,1.0);
  lxVector4Set(glmesh->pos[2],  -0.982160,0.319123,-0.727615,1.0);
  lxVector4Set(glmesh->pos[3],  -0.607008,-0.835474,-0.727615,1.0);
  lxVector4Set(glmesh->pos[4],  0.607008,-0.835474,-0.727615,1.0);
  lxVector4Set(glmesh->pos[5],  0.982160,0.319123,-0.727615,1.0);
  lxVector2Set(glmesh->tex[0],  0.5,0.5);
  lxVector2Set(glmesh->tex[1],  0.5,0.988647);
  lxVector2Set(glmesh->tex[2],  0.006689,0.652711);
  lxVector2Set(glmesh->tex[3],  0.195117,0.109156);
  lxVector2Set(glmesh->tex[4],  0.804883,0.109156);
  lxVector2Set(glmesh->tex[5],  0.993311,0.652711);
  for (i=0; i < 6; i++){
    lxVector3short_FROM_float(&glmesh->normal[i*4],&normhsphere[i*3]);
    glmesh->normal[3+i*4] = 0;
  }
  GLParticleMesh_InstFromBatch(glmesh);


  // setup tris
  glmesh = &l_PrtBuffer.trimesh;
  glmesh->mesh.primtype = PRIM_TRIANGLES;
  glmesh->mesh.numIndices = 0;
  glmesh->mesh.numVertices = 3;
  glmesh->hasnormals = LUX_FALSE;
  GLParticleMesh_initBatchVerts(glmesh);
  lxVector4Set(glmesh->pos[0], -0.66,-0.33,0,1.0);
  lxVector4Set(glmesh->pos[1],  0.33,-0.33,0,1.0);
  lxVector4Set(glmesh->pos[2],  0.33, 0.66,0,1.0);
  lxVector2Set(glmesh->tex[0],  0,0);
  lxVector2Set(glmesh->tex[1],  1,0);
  lxVector2Set(glmesh->tex[2],  1,1);
  GLParticleMesh_InstFromBatch(glmesh);

  // setup quad
  glmesh = &l_PrtBuffer.quadmesh;
  glmesh->mesh.primtype = PRIM_QUADS;
  glmesh->mesh.numIndices = 0;
  glmesh->mesh.numVertices = 4;
  glmesh->hasnormals = LUX_FALSE;
  GLParticleMesh_initBatchVerts(glmesh);
  lxVector4Set(glmesh->pos[0], -0.5,-0.5,0,1.0);
  lxVector4Set(glmesh->pos[1],  0.5,-0.5,0,1.0);
  lxVector4Set(glmesh->pos[2],  0.5, 0.5,0,1.0);
  lxVector4Set(glmesh->pos[3], -0.5, 0.5,0,1.0);
  lxVector2Set(glmesh->tex[0],  0,0);
  lxVector2Set(glmesh->tex[1],  1,0);
  lxVector2Set(glmesh->tex[2],  1,1);
  lxVector2Set(glmesh->tex[3],  0,1);
  GLParticleMesh_InstFromBatch(glmesh);

  for (i = 0; i < GLPARTICLE_MAX_MESHVERTICES; i++){
    l_PrtBuffer.poscache[i][3] = 1.0f;
  }
#ifdef LUX_PARTICLE_USEBATCHVBO
  if (GLEW_ARB_vertex_buffer_object && g_VID.usevbos){
    glGenBuffersARB(1,l_PrtBuffer.vboID);
    l_PrtBuffer.curVBO = 0;
  }
#endif
}

void GLParticleBuffer_deinit()
{
  GLParticleMesh_free(&l_PrtBuffer.trimesh,LUX_TRUE);
  GLParticleMesh_free(&l_PrtBuffer.quadmesh,LUX_TRUE);
  GLParticleMesh_free(&l_PrtBuffer.hspheremesh,LUX_TRUE);

  if (l_PrtBuffer.vboID[0]){
    glDeleteBuffersARB(1,l_PrtBuffer.vboID);
    l_PrtBuffer.vboID[0] = 0;
  }
}

static int  GLParticleBuffer_maxparticles(ParticleType_t type,GLParticleMesh_t *glmesh)
{
  int maxparticles;
  switch (type){
  case PARTICLE_TYPE_POINT:
    maxparticles = VID_MAX_VERTICES;
    break;
  case PARTICLE_TYPE_TRIANGLE:
    maxparticles = VID_MAX_VERTICES/3;
    break;
  case PARTICLE_TYPE_QUAD:
    maxparticles = VID_MAX_VERTICES/4;
    break;
  case PARTICLE_TYPE_HSPHERE:
    maxparticles = VID_MAX_VERTICES/6;
    maxparticles = LUX_MIN(maxparticles,VID_MAX_INDICES/15);
    break;
  case PARTICLE_TYPE_INSTMESH:
    if (!glmesh->batchverts || l_PrtBuffer.cont->contflag & PARTICLE_FORCEINSTANCE){
      maxparticles = MAX_PARTICLES;
    }
    else{
      maxparticles = VID_MAX_VERTICES/glmesh->mesh.numVertices;
      maxparticles = LUX_MIN(maxparticles,VID_MAX_INDICES/glmesh->mesh.numIndices);
    }
    break;
  default:
    maxparticles = MAX_PARTICLES;
  }

  return maxparticles;
}

//////////////////////////////////////////////////////////////////////////
// GLParticleMesh

void GLParticleMesh_initBatchVerts(GLParticleMesh_t *glmesh)
{
  if ((glmesh->mesh.primtype != PRIM_QUADS && glmesh->mesh.primtype != PRIM_TRIANGLES) ||
    glmesh->mesh.numVertices > GLPARTICLE_MAX_MESHVERTICES ||
    glmesh->mesh.numIndices > GLPARTICLE_MAX_MESHINDICES)
    return;
  glmesh->indices = glmesh->mesh.indicesData16;
  glmesh->batchverts = lxMemGenZalloc(sizeof(GLParticleBatchVertex_t)*glmesh->mesh.numVertices);
  glmesh->normal  = (short*)(((uchar*)glmesh->batchverts)+((sizeof(lxVector2)+sizeof(lxVector4))*glmesh->mesh.numVertices));
  glmesh->pos =   (lxVector4*)((uchar*)glmesh->batchverts);
  glmesh->tex =   (lxVector2*)(((uchar*)glmesh->batchverts)+(sizeof(lxVector4)*glmesh->mesh.numVertices));
}

void  GLParticleMesh_BatchFromInst(GLParticleMesh_t *glmesh)
{
  lxVertex32FN_t  *glvert;
  int i;

  glvert = glmesh->mesh.vertexData32FN;
  for(i = 0; i < glmesh->mesh.numVertices; i++,glvert++){
    lxVector3Copy(glmesh->pos[i],glvert->pos);
    glmesh->pos[i][3] = 1.0f;
    lxVector2Copy(glmesh->tex[i],glvert->tex);
    lxVector3short_FROM_float(&glmesh->normal[i*4],glvert->normal);
  }
}

void GLParticleMesh_setVBO(GLParticleMesh_t *glmesh){
  VIDBuffer_initGL(&glmesh->vbo,VID_BUFFER_VERTEX,VID_BUFFERHINT_STATIC_DRAW,glmesh->mesh.numVertices*sizeof(lxVertex32FN_t),
    glmesh->mesh.vertexData);
  VIDBuffer_initGL(&glmesh->ibo,VID_BUFFER_INDEX,VID_BUFFERHINT_STATIC_DRAW,glmesh->mesh.numIndices*sizeof(ushort),
    glmesh->mesh.indicesData);

  glmesh->mesh.vid.vbo = &glmesh->vbo;
  glmesh->mesh.vid.ibo = &glmesh->ibo;
  glmesh->mesh.vid.vbooffset = 0;
  glmesh->mesh.vid.ibooffset = 0;
  glmesh->mesh.meshtype = MESH_VBO;
}

GLParticleMesh_t * GLParticleMesh_new(Mesh_t *mesh){
  GLParticleMesh_t *glmesh;
  lxVertex32FN_t  *glvert;
  int i;
  float *vec;
  short *normal;


  glmesh = lxMemGenZalloc(sizeof(GLParticleMesh_t));
  glmesh->mesh.vertexData = lxMemGenZallocAligned(sizeof(lxVertex32FN_t)*mesh->numVertices,32);
  glmesh->mesh.indicesData16 = lxMemGenZalloc(sizeof(ushort)*mesh->numIndices);
  glmesh->mesh.index16 = LUX_TRUE;
  glmesh->mesh.vertextype = VERTEX_32_FN;
  glmesh->mesh.indexMin = mesh->indexMin;
  glmesh->mesh.indexMax = mesh->indexMax;
  glmesh->mesh.numAllocVertices = glmesh->mesh.numVertices = mesh->numVertices;
  glmesh->mesh.numAllocIndices = glmesh->mesh.numIndices = mesh->numIndices;
  glmesh->mesh.primtype = mesh->primtype;
  glmesh->mesh.meshtype = MESH_VA;
  glmesh->mesh.numTris = mesh->numIndices/((mesh->primtype == PRIM_TRIANGLES) ? 3 : 4);

  LUX_ASSERT(VertexValue(mesh->vertextype,VERTEX_SCALARPOS) == LUX_SCALAR_FLOAT32);
  LUX_ASSERT(VertexValue(mesh->vertextype,VERTEX_SCALARTEX) == LUX_SCALAR_FLOAT32);

  glvert = glmesh->mesh.vertexData32FN;
  for (i = 0; i < mesh->numVertices; i++,glvert++){
    vec = VertexArrayPtr(mesh->vertexData,i,mesh->vertextype,VERTEX_POS);
    lxVector3Copy(glvert->pos,vec);

    vec = VertexArrayPtr(mesh->vertexData,i,mesh->vertextype,VERTEX_TEX0);
    lxVector2Copy(glvert->tex,vec);
  }

  if (mesh->vertextype == VERTEX_32_NRM || mesh->vertextype == VERTEX_64_TEX4 || mesh->vertextype == VERTEX_64_SKIN){
    glvert = glmesh->mesh.vertexData32FN;
    for (i = 0; i < mesh->numVertices; i++,glvert++){
      normal = VertexArrayPtr(mesh->vertexData,i,mesh->vertextype,VERTEX_NORMAL);
      lxVector3float_FROM_short(glvert->normal,normal);
    }
    glmesh->hasnormals = LUX_TRUE;
  }

  for (i = 0; i < mesh->numIndices; i++)
    glmesh->mesh.indicesData16[i] = mesh->indicesData16[i];



  if (GLEW_ARB_vertex_buffer_object && g_VID.usevbos){
    GLParticleMesh_setVBO(glmesh);
  }
  if (glmesh->mesh.numVertices < 8 ){
    GLParticleMesh_initBatchVerts(glmesh);
    GLParticleMesh_BatchFromInst(glmesh);
  }

  return glmesh;
}


void  GLParticleMesh_InstFromBatch(GLParticleMesh_t *glmesh)
{
  int i;
  lxVertex32FN_t  *glvert;

  glvert = glmesh->mesh.vertexData = lxMemGenZallocAligned(sizeof(lxVertex32FN_t)*glmesh->mesh.numVertices,32);
  if(!glmesh->mesh.numIndices){
    glmesh->mesh.indicesData16 = lxMemGenZalloc(sizeof(ushort)*glmesh->mesh.numVertices);
    glmesh->mesh.numIndices = glmesh->mesh.numVertices;
    for (i=0; i < glmesh->mesh.numVertices; i++)
      glmesh->mesh.indicesData16[i]=i;
  }
  glmesh->mesh.vertextype = VERTEX_32_FN;
  glmesh->mesh.indexMin = 0;
  glmesh->mesh.indexMax = glmesh->mesh.numVertices;
  glmesh->mesh.index16 = LUX_TRUE;
  glmesh->mesh.numAllocVertices = glmesh->mesh.numVertices;
  glmesh->mesh.numAllocIndices = glmesh->mesh.numIndices;
  glmesh->mesh.meshtype = MESH_VA;
  glmesh->mesh.numTris = glmesh->mesh.numIndices/((glmesh->mesh.primtype == PRIM_TRIANGLES) ? 3 : 4);

  for (i= 0; i < glmesh->mesh.numVertices; i++,glvert++){
    lxVector3Copy(glvert->pos,glmesh->pos[i]);
    if (glmesh->hasnormals)
      lxVector3float_FROM_short(glvert->normal,&glmesh->normal[i*4]);
    else
      lxVector3Set(glvert->normal,0,0,1);
    lxVector2Copy(glvert->tex,glmesh->tex[i]);
  }


  if (GLEW_ARB_vertex_buffer_object && g_VID.usevbos){
    GLParticleMesh_setVBO(glmesh);
  }


}





void  GLParticleMesh_free(GLParticleMesh_t *glmesh,int dontfreeself)
{
  if (glmesh->batchverts)
    lxMemGenFree(glmesh->batchverts,sizeof(GLParticleBatchVertex_t)*glmesh->mesh.numVertices);
  if (glmesh->mesh.vertexData)
    lxMemGenFreeAligned(glmesh->mesh.vertexData,sizeof(lxVertex32FN_t)*glmesh->mesh.numAllocVertices);
  if (glmesh->mesh.indicesData16)
    lxMemGenFree(glmesh->mesh.indicesData16,sizeof(ushort)*glmesh->mesh.numAllocIndices);

  // delete buffers
  if (glmesh->mesh.meshtype == MESH_VBO){
    VIDBuffer_clearGL(&glmesh->vbo);
    VIDBuffer_clearGL(&glmesh->ibo);
  }

  Mesh_typeVA(&glmesh->mesh);



  if (!dontfreeself){
    lxMemGenFree(glmesh,sizeof(GLParticleMesh_t));
  }
}

//////////////////////////////////////////////////////////////////////////
// Particle Vertex Processors

#define PARTICLE_CHECKAXIS(i) (prt->pos[i] < pcont->checkMin[i] || prt->pos[i] > pcont->checkMax[i])

#define PARTICLE_GPUVERTEX()  \
  lxVector3Copy(out64->pos,drawPrt->viewpos); \
  out64->colorc = drawPrt->colorc;  \
  out64->tex[0] = n;  \
  out64->tex[1] = drawPrt->size;  \
  out64->tex[2] = drawPrt->texoffset; \
  out64->tex2[0] = drawPrt->cosf; \
  out64->tex2[1] = -drawPrt->sinf;  \
  out64->tex2[2] = drawPrt->relage; \
  out64->tex2[3] = drawPrt->speed; \
  out64++;  \
  g_VID.drw_meshbuffer->numVertices++;

#define PARTICLE_SOFTVERTEX() \
  /* texcoords */   \
  out32->tex[0]=(*tex+drawPrt->texoffset)*l_PrtBuffer.texwidth; \
  out32->tex[1]=*(tex+1); \
  /* position */  \
  lxVector3Copy(out32->pos,drawPrt->viewpos); \
  lxVector3RotateZcossin(vec,pos,drawPrt->cosf,drawPrt->sinf);  \
  lxVector3ScaledAdd(out32->pos,out32->pos,drawPrt->size,vec);  \
  /* colors */  \
  out32->colorc = drawPrt->colorc;  \
  \
  g_VID.drw_meshbuffer->numVertices++;  \
  out32++;

#define PARTICLE_SOFTVERTEX_GLOBALALIGN() \
  /* texcoords */   \
  out32->tex[0]=(*tex+drawPrt->texoffset)*l_PrtBuffer.texwidth; \
  out32->tex[1]=*(tex+1); \
  /* position */  \
  lxVector3RotateZcossin(vec,pos,drawPrt->cosf,drawPrt->sinf);  \
  lxVector3TransformRot(temp,vec,l_PrtBuffer.axisMatrix); \
  lxVector3Copy(vec,drawPrt->viewpos);  \
  lxVector3ScaledAdd(out32->pos,vec,drawPrt->size,temp);  \
  /* colors */  \
  out32->colorc = drawPrt->colorc;  \
  \
  g_VID.drw_meshbuffer->numVertices++;  \
  out32++;

#define GLPARTICLE_START  \
  order = (l_PrtBuffer.sort < 0) ? (l_PrtBuffer.drawIndices+l_PrtBuffer.drawSize-1) : l_PrtBuffer.drawIndices;\
  for (i= 0; i < l_PrtBuffer.drawSize; i++,order+=l_PrtBuffer.sort){\
    drawPrt = &l_PrtBuffer.drawParticles[*order];

#define GLPARTICLE_END    }


//////////////////////////////////////////////////////////////////////////
// GLParticles Upload

static void GLParticles_pushPoint()
{
  int i;
  GLParticle_t *drawPrt;
  lxVertex16_t *out16;
  ushort *index;
  uint *order;

  out16 = g_VID.drw_meshbuffer->vertexData16;
  index = g_VID.drw_meshbuffer->indicesData16;
  g_VID.drw_meshbuffer->vertextype = VERTEX_16_CLR;


  GLPARTICLE_START

    // position
    lxVector3Copy(out16->pos,drawPrt->viewpos);

    // color
    out16->colorc = drawPrt->colorc;

    *index = i;
    index++;
    out16++;
    g_VID.drw_meshbuffer->numVertices++;
  GLPARTICLE_END;
}

static void GLParticles_pushGeo(GLParticleMesh_t *glmesh)
{
  int i;
  int n;
  lxVertex64_t *out64;
  lxVertex64_t *outsrc64;
  lxVertex32_t *out32;
  lxVertex32_t *outsrc32;
  ushort *index;
  GLParticle_t *drawPrt;
  int   *order;
  lxVector3 vec;
  lxVector3 temp;
  double *normalin;
  float *tex;
  float *pos;


  g_VID.drw_meshbuffer->vertextype = l_PrtBuffer.usevprog ? VERTEX_64_TEX4 : VERTEX_32_NRM;

#ifdef LUX_PARTICLE_USEBATCHVBO
  vidBindBufferArray(l_PrtBuffer.vboID[l_PrtBuffer.curVBO]);
  glBufferData(GL_ARRAY_BUFFER_ARB,VertexSize(g_VID.drw_meshbuffer->vertextype)*glmesh->mesh.numVertices*l_PrtBuffer.drawSize,NULL,GL_STREAM_DRAW);

  out32 = (void*)outsrc32 = (void*)out64 = (void*)outsrc64 = g_VID.drw_meshbuffer->vertexData;
#else
  out64 = outsrc64 = g_VID.drw_meshbuffer->vertexData64;
  out32 = outsrc32 = g_VID.drw_meshbuffer->vertexData32;
#endif

  index = g_VID.drw_meshbuffer->indicesData16;

  if(l_PrtBuffer.useoffset){
    for (n = 0; n < glmesh->mesh.numVertices; n++){
      lxVector3Add(l_PrtBuffer.poscache[n],l_PrtBuffer.offset,glmesh->pos[n]);
    }
  }

  l_PrtBuffer.pPos = (l_PrtBuffer.useoffset) ? l_PrtBuffer.poscache[0] : glmesh->pos[0];
  l_PrtBuffer.pTex = glmesh->tex[0];


  //g_VID.drw_meshbuffer->numVertices++;
  //out++;


  // unique indices
  if(glmesh->indices){

    if (l_PrtBuffer.usevprog){
      GLPARTICLE_START
        for (n = 0; n < glmesh->mesh.numVertices; n++){
          PARTICLE_GPUVERTEX();
        }
      GLPARTICLE_END;
    }
    else{
      if (l_PrtBuffer.useGlobalAxis){
        GLPARTICLE_START
          tex = l_PrtBuffer.pTex;
          pos = l_PrtBuffer.pPos;
          for (n = 0; n < glmesh->mesh.numVertices; n++,tex+=2,pos+=4){
            PARTICLE_SOFTVERTEX_GLOBALALIGN();
          }
        GLPARTICLE_END;
      }
      else{
        GLPARTICLE_START
          tex = l_PrtBuffer.pTex;
          pos = l_PrtBuffer.pPos;
          for (n = 0; n < glmesh->mesh.numVertices; n++,tex+=2,pos+=4){
            PARTICLE_SOFTVERTEX();
          }
        GLPARTICLE_END;
      }
    }

    { // offset indices
      ushort offset = 0;
      for (i =0; i < l_PrtBuffer.drawSize; i++){
        for (n = 0; n < glmesh->mesh.numIndices; n++)
          index[n] = offset + glmesh->indices[n];
        index+=glmesh->mesh.numIndices;
        offset += glmesh->mesh.numVertices;
      }
    }


  }// simple indices
  else{
    if (l_PrtBuffer.usevprog){
      GLPARTICLE_START
        for (n = 0; n < glmesh->mesh.numVertices; n++){
          *index = g_VID.drw_meshbuffer->numVertices;
          index++;
          PARTICLE_GPUVERTEX();
        }
      GLPARTICLE_END;
    }
    else{
      if (l_PrtBuffer.useGlobalAxis){
        GLPARTICLE_START
          tex = l_PrtBuffer.pTex;
          pos = l_PrtBuffer.pPos;
          for (n = 0; n < glmesh->mesh.numVertices; n++,tex+=2,pos+=4){
            *index = g_VID.drw_meshbuffer->numVertices;
            index++;
            PARTICLE_SOFTVERTEX_GLOBALALIGN();
          }
        GLPARTICLE_END;
      }
      else{
        GLPARTICLE_START
          tex = l_PrtBuffer.pTex;
          pos = l_PrtBuffer.pPos;
          for (n = 0; n < glmesh->mesh.numVertices; n++,tex+=2,pos+=4){
            *index = g_VID.drw_meshbuffer->numVertices;
            index++;
            PARTICLE_SOFTVERTEX();
          }
        GLPARTICLE_END;
      }
    }

    for (i = 0; i < glmesh->mesh.numVertices%8; i++,index++)
      *index = i;
    n = glmesh->mesh.numVertices/8;
    for (; i < n; i++){
      *index++ = i++;
      *index++ = i++;
      *index++ = i++;
      *index++ = i++;
      *index++ = i++;
      *index++ = i++;
      *index++ = i++;
      *index++ = i++;
    }
  }

  if(l_PrtBuffer.normalsset){
    normalin = (double*)glmesh->normal;

    if (l_PrtBuffer.usevprog){
      out64 = outsrc64;
      for (n = 0; n < g_VID.drw_meshbuffer->numVertices; n++,out64++){
        *(double*)out64->normal = normalin[n%glmesh->mesh.numVertices];
      }
    }
    else{
      out32 = outsrc32;
      for (n = 0; n < g_VID.drw_meshbuffer->numVertices; n++,out32++){
        *(double*)out32->normal = normalin[n%glmesh->mesh.numVertices];
      }
    }

  }

#ifdef LUX_PARTICLE_USEBATCHVBO
  glBufferSubDataARB(GL_ARRAY_BUFFER_ARB,0,VertexSize(g_VID.drw_meshbuffer->vertextype)*glmesh->mesh.numVertices*l_PrtBuffer.drawSize,g_VID.drw_meshbuffer->vertexData);
#endif

  vidCheckError();

}

//////////////////////////////////////////////////////////////////////////
// GLParticle Instancing

static void GLParticle_prepMatrix(GLParticle_t *drawPrt,lxMatrix44 mat)
{
  lxMatrix44SetRotRows(mat,
    drawPrt->size*drawPrt->cosf,  -drawPrt->size*drawPrt->sinf,         0,
    drawPrt->size*drawPrt->sinf,          drawPrt->size*drawPrt->cosf,  0,
    0,                0,                drawPrt->size);
  mat[12] = drawPrt->viewpos[0];
  mat[13] = drawPrt->viewpos[1];
  mat[14] = drawPrt->viewpos[2];
}

static void GLParticle_prepMatrix_globalaxis(GLParticle_t *drawPrt,lxMatrix44 mat)
{
  static lxMatrix44SIMD  temp = {1,0,0,0, 0,1,0,0,  0,0,1,0,  0,0,0,1};
  lxMatrix44SetRotRows(mat,
    drawPrt->size*drawPrt->cosf,  -drawPrt->size*drawPrt->sinf,         0,
    drawPrt->size*drawPrt->sinf,          drawPrt->size*drawPrt->cosf,  0,
    0,                0,                drawPrt->size);
  mat[12] = 0;
  mat[13] = 0;
  mat[14] = 0;
  temp[12] = drawPrt->viewpos[0];
  temp[13] = drawPrt->viewpos[1];
  temp[14] = drawPrt->viewpos[2];
  lxMatrix44MultiplyRot2(l_PrtBuffer.axisMatrix,mat);
  lxMatrix44Multiply2SIMD(temp,mat);
}

#ifdef LUX_VID_FORCE_LIMITPRIMS
#define USED_INDICES(ind) LUX_MIN(ind,4)
#else
#define USED_INDICES(ind) (ind)
#endif

static void GLParticles_drawInstanceMesh(Mesh_t *mesh)
{
  static lxMatrix44SIMD mat =  {1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1};

  GLParticle_t *drawPrt;
  int   *order;
  ushort  *ind;
  int i;
  lxVector4 color;


  void (*fn_mat) (GLParticle_t *drawPrt,lxMatrix44 mat) = l_PrtBuffer.useGlobalAxis ? GLParticle_prepMatrix_globalaxis : GLParticle_prepMatrix;


  lxMatrix44IdentitySIMD(mat);

  if (mesh->meshtype == MESH_VBO){
    vidBindBufferIndex(mesh->vid.ibo);
    ind = NULL;
  }
  else{
    ind = mesh->indicesData16;
  }

  if (l_PrtBuffer.usevprog){
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GLPARTICLE_START
      glVertexAttrib4fARB(9,  drawPrt->size*drawPrt->cosf,  -drawPrt->size*drawPrt->sinf,         0,          drawPrt->viewpos[0]);
      glVertexAttrib4fARB(10, drawPrt->size*drawPrt->sinf,  drawPrt->size*drawPrt->cosf,          0,          drawPrt->viewpos[1]);
      glVertexAttrib4fARB(11, 0,                0,                  drawPrt->size,          drawPrt->viewpos[2]);
      glVertexAttrib4fARB(12, drawPrt->texoffset,drawPrt->relage,drawPrt->speed,1);
      lxVector4float_FROM_ubyte(color,drawPrt->colorb);
      glVertexAttrib4fvARB(13,color);
      if(l_PrtBuffer.normalsset > 1){
        lxVector3TransformRot(color,drawPrt->normal,g_CamLight.camera->mview);
        glNormal3fv(color);
      }
      if(ind && GLEW_EXT_draw_range_elements){ // no rangecall for VBO
        glDrawRangeElementsEXT(mesh->primtype,0,mesh->numVertices-1,USED_INDICES(mesh->numIndices),GL_UNSIGNED_SHORT,ind);
      }
      else
        glDrawElements(mesh->primtype,USED_INDICES(mesh->numIndices),GL_UNSIGNED_SHORT,ind);
      //glVertexAttrib4fARB
    GLPARTICLE_END;

    glVertexAttrib4fARB(9,  0,0,0,1);
    glVertexAttrib4fARB(10, 0,0,0,1);
    glVertexAttrib4fARB(11, 0,0,0,1);
    glVertexAttrib4fARB(12, 0,0,0,1);
    glVertexAttrib4fARB(13, 0,0,0,1);
  }
  else{

    if (g_VID.state.renderflag & RENDER_LIT)
      vidNormalize(LUX_TRUE);

    if (ind && GLEW_EXT_compiled_vertex_array)
      glLockArraysEXT(0,mesh->numVertices);

    vidSelectTexture(0);
    GLPARTICLE_START
      glMatrixMode(GL_MODELVIEW);
      fn_mat(drawPrt,mat);
      glLoadMatrixf(mat);

      if (l_PrtBuffer.cont->numTex > 1){
        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
        glScalef(l_PrtBuffer.texwidth,1,1);
        glTranslatef(drawPrt->texoffset,0,0);

        //vidSelectTexture(1);
        //dirty speed hack
        glActiveTextureARB(GL_TEXTURE1_ARB);
        glPushMatrix();
        glScalef(l_PrtBuffer.texwidth,1,1);
        glTranslatef(drawPrt->texoffset,0,0);
      }

      if(l_PrtBuffer.normalsset > 1){
        lxVector3RotateZcossin(color,drawPrt->normal,drawPrt->cosf,-drawPrt->sinf);
        glNormal3fv(color);
      }
      glColor4ubv(drawPrt->colorb);

      if(ind && GLEW_EXT_draw_range_elements)// no rangecall for VBO
        glDrawRangeElementsEXT(mesh->primtype,0,mesh->numVertices-1,USED_INDICES(mesh->numIndices),GL_UNSIGNED_SHORT,ind);
      else
        glDrawElements(mesh->primtype,USED_INDICES(mesh->numIndices),GL_UNSIGNED_SHORT,ind);

      if (l_PrtBuffer.cont->numTex > 1){
        glPopMatrix();
        glActiveTextureARB(GL_TEXTURE0_ARB);
        glPopMatrix();
      }

    GLPARTICLE_END;

    if (ind && GLEW_EXT_compiled_vertex_array)
      glUnlockArraysEXT();
  }

  LUX_PROFILING_OP (g_Profiling.perframe.particle.meshes +=l_PrtBuffer.drawSize);
  LUX_PROFILING_OP (g_Profiling.perframe.particle.vertices += mesh->numVertices*l_PrtBuffer.drawSize);
  LUX_PROFILING_OP (g_Profiling.perframe.particle.tris += mesh->numTris*l_PrtBuffer.drawSize);

}

//////////////////////////////////////////////////////////////////////////
// GLParticle Model

static void GLParticles_drawModel(int type)
{
  static lxMatrix44SIMD mat =  {1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1};

  int i;
  GLParticle_t *drawPrt;
  Model_t *model;
  MeshObject_t  *meshobj;
  Mesh_t    *mesh;
  int   *order;
  ParticleLife_t *life;

  void (*fn_mat) (GLParticle_t *drawPrt,lxMatrix44 mat) = l_PrtBuffer.useGlobalAxis ? GLParticle_prepMatrix_globalaxis : GLParticle_prepMatrix;

  // type 0 = model, 1 = sphere

  life = l_PrtBuffer.cont->life;

  if (type){
    mesh = g_VID.drw_meshsphere;
    LUX_PROFILING_OP (g_Profiling.perframe.particle.meshes+= l_PrtBuffer.drawSize);
    LUX_PROFILING_OP (g_Profiling.perframe.particle.vertices += l_PrtBuffer.drawSize*mesh->numVertices);
    LUX_PROFILING_OP (g_Profiling.perframe.particle.tris +=  mesh->numTris*l_PrtBuffer.drawSize);
  }
  else{
    model = ResData_getModel(l_PrtBuffer.cont->modelRID);
    LUX_PROFILING_OP (g_Profiling.perframe.particle.meshes+= l_PrtBuffer.drawSize);
  }

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  vidWorldScale(NULL);

  if (l_PrtBuffer.colorset && !type){
    vidNoVertexColor(LUX_TRUE);
  }

  GLPARTICLE_START

    // render geometry
    if (type){
      // Sphere
      glColor4ubv(drawPrt->colorb);
      fn_mat(drawPrt,mat);
      vidWorldMatrix(mat);
      Draw_Mesh_auto(mesh,-1,NULL,0,NULL);
    }
    else{
      // Mesh_t
      glColor4ubv(drawPrt->colorb);
      fn_mat(drawPrt,mat);
      vidWorldMatrix(mat);
      meshobj = &model->meshObjects[(((uint)drawPrt->prtSt>1)*13) % model->numMeshObjects];
      mesh = meshobj->mesh;
      Draw_Mesh_auto(mesh,meshobj->texRID,NULL,0,NULL);

      LUX_PROFILING_OP (g_Profiling.perframe.particle.vertices += mesh->numVertices);
      LUX_PROFILING_OP (g_Profiling.perframe.particle.tris +=  mesh->numTris);
    }
  GLPARTICLE_END;

}

//////////////////////////////////////////////////////////////////////////
// ParticleBufferDraw

typedef struct GLParticleState_s{
  // shader
  enum32        oldflag;
  GLShaderStage_t   *lmglstage;
  ShaderTexGen_t    *oldtexgen;

  // shader & texture
  int         lightmapunit;

  // state
  Material_t  *material;
  Shader_t  *shader;
  int     arb;
  int     fgpu;
  int     vgpu;
  int     ggpu;
} GLParticleState_t;

static GLParticleState_t  l_PState;

static void ParticleBufferDraw_setVertexParameters(int arb)
{
  int numtex;
  int i;

  if (l_PrtBuffer.usevprog==2 && (l_PrtBuffer.cont->particletype != l_PrtBuffer.lastvprogParticletype ||
    l_PrtBuffer.rot != l_PrtBuffer.lastvprogRot ||
    l_PrtBuffer.lastGlobalAxis != l_PrtBuffer.useGlobalAxis ||
    l_PrtBuffer.lastUseOffset != l_PrtBuffer.useoffset ||
    l_PrtBuffer.lastLm != (g_VID.shdsetup.lightmapRID >= 0) ||
    (l_PrtBuffer.offset && !lxVector3Compare(l_PrtBuffer.offset,==,l_PrtBuffer.lastOffset))))
  {
    switch(l_PrtBuffer.cont->particletype) {
    case PARTICLE_TYPE_INSTMESH:
      l_PrtBuffer.pPos = l_PrtBuffer.offset ? l_PrtBuffer.poscache[0] : l_PrtBuffer.cont->instmesh->pos[0];
      l_PrtBuffer.pTex = l_PrtBuffer.cont->instmesh->tex[0];
      numtex = l_PrtBuffer.cont->instmesh->mesh.numVertices;
      break;
    case PARTICLE_TYPE_QUAD:
      l_PrtBuffer.pPos = l_PrtBuffer.offset ? l_PrtBuffer.poscache[0] : l_PrtBuffer.quadmesh.pos[0];
      l_PrtBuffer.pTex = l_PrtBuffer.quadmesh.tex[0];
      numtex = 4;
      break;
    case PARTICLE_TYPE_HSPHERE:
      l_PrtBuffer.pPos = l_PrtBuffer.offset ? l_PrtBuffer.poscache[0] : l_PrtBuffer.hspheremesh.pos[0];
      l_PrtBuffer.pTex = l_PrtBuffer.hspheremesh.tex[0];
      numtex = 6;
      break;
    case PARTICLE_TYPE_TRIANGLE:
      l_PrtBuffer.pPos = l_PrtBuffer.offset ? l_PrtBuffer.poscache[0] : l_PrtBuffer.trimesh.pos[0];
      l_PrtBuffer.pTex = l_PrtBuffer.trimesh.tex[0];
      numtex = 3;
      break;
    default:
      numtex = 0;
      break;
    }
    if (arb){
      for (i = 0; i < numtex; i++,l_PrtBuffer.pPos+=4,l_PrtBuffer.pTex+=2){
        //offsets
        glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, i+1, l_PrtBuffer.pPos);
        //texoffsets
        glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, i+1+GLPARTICLE_MAX_MESHVERTICES, l_PrtBuffer.pTex[0],l_PrtBuffer.pTex[1],0.0,1.0);
      }
    }
    else{
      cgSetParameterValuefc(g_VID.cg.bbvertexOffsets,numtex*4,l_PrtBuffer.pPos);
      cgSetParameterValuefc(g_VID.cg.bbtexOffsets,numtex*2,l_PrtBuffer.pTex);
    }
    l_PrtBuffer.lastvprogParticletype = l_PrtBuffer.cont->particletype;
    l_PrtBuffer.lastvprogRot = l_PrtBuffer.rot;
    l_PrtBuffer.lastGlobalAxis = l_PrtBuffer.useGlobalAxis;
    l_PrtBuffer.lastUseOffset = l_PrtBuffer.useoffset;
    l_PrtBuffer.lastLm = (g_VID.shdsetup.lightmapRID >= 0);
    if(l_PrtBuffer.offset)
      lxVector3Copy(l_PrtBuffer.lastOffset,l_PrtBuffer.offset);
  }


  if (l_PrtBuffer.useGlobalAxis){
    lxMatrix44Transpose1(l_PrtBuffer.axisMatrix);
    if (arb){
      glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 65, &l_PrtBuffer.axisMatrix[0]);
      glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 66, &l_PrtBuffer.axisMatrix[4]);
      glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 67, &l_PrtBuffer.axisMatrix[8]);       }
    else
      cgSetMatrixParameterfr(g_VID.cg.bbglobalAxis,l_PrtBuffer.axisMatrix);
  }

  if (arb)
    glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 0, l_PrtBuffer.texwidth, 1, 0, 1);
  else
    cgSetParameter2f(g_VID.cg.bbtexwidth,l_PrtBuffer.texwidth,1);
  vidCgCheckError();
}


static int ParticleBufferDrawGL_State()
{
  static lxVector4 black = {0.0f,0.0f,0.0f,0.0f};

  GLParticleState_t *glps  = &l_PState;
  enum32    renderflag;
  int     resetfog = LUX_FALSE;



  g_VID.shdsetup.lightmapRID = l_PrtBuffer.cont->lmRID;
  g_VID.shdsetup.lmtexmatrix = NULL;

  if (vidMaterial(l_PrtBuffer.cont->texRID)){
    int sid = g_VID.drawsetup.shadersdefault.override;

    glps->material = ResData_getMaterial(l_PrtBuffer.cont->texRID);
    glps->shader = ResData_getShader(glps->material->shaders[sid].resRIDUse);
    if(!glps->shader){
      sid = 0;
      glps->shader = ResData_getShader(glps->material->shaders[sid].resRIDUse);
    }
    Material_update(glps->material,l_PrtBuffer.cont->matobj);

    Material_setVID(glps->material,sid);
    glps->vgpu = isFlagSet(glps->shader->shaderflag,SHADER_VGPU);
    glps->fgpu = isFlagSet(glps->shader->shaderflag,SHADER_FGPU);
    glps->ggpu = isFlagSet(glps->shader->shaderflag,SHADER_GGPU);

    if (isFlagSet(glps->shader->shaderflag,SHADER_NEEDINVMATRIX))
      lxMatrix44Copy(g_VID.drawsetup.worldMatrixInv,g_VID.drawsetup.viewMatrix);

    glps->arb = !glps->vgpu || glps->shader->shaderflag & SHADER_VARB;
  }
  else{
    glps->shader = NULL;

    glps->vgpu = LUX_FALSE;
    glps->fgpu = LUX_FALSE;
    glps->ggpu = LUX_FALSE;

    glps->arb = LUX_TRUE;
  }

  // global axis
  if (l_PrtBuffer.cont->contflag & PARTICLE_GLOBALAXIS){
    lxMatrix44Multiply(l_PrtBuffer.axisMatrix,g_CamLight.camera->mview,l_PrtBuffer.cont->userAxisMatrix);
    lxMatrix44ClearTranslation(l_PrtBuffer.axisMatrix);
    l_PrtBuffer.useGlobalAxis = LUX_TRUE;
  }
  else
    l_PrtBuffer.useGlobalAxis = LUX_FALSE;


  // origin offset
  if (l_PrtBuffer.cont->contflag & PARTICLE_ORIGINOFFSET){
    l_PrtBuffer.offset = l_PrtBuffer.cont->userOffset;
    l_PrtBuffer.useoffset = LUX_TRUE;
  }
  else
    l_PrtBuffer.useoffset = LUX_FALSE;


  // blend/alphatest override shader's
  if (l_PrtBuffer.cont->blend.blendmode && l_PrtBuffer.cont->blend.blendmode != VID_REPLACE){
    vidBlend(l_PrtBuffer.cont->blend.blendmode,l_PrtBuffer.cont->blend.blendinvert);
    l_PrtBuffer.renderflag |= RENDER_BLEND;
  }
  else{
    l_PrtBuffer.renderflag &= ~RENDER_BLEND;
  }
  if (l_PrtBuffer.cont->alpha.alphafunc && l_PrtBuffer.cont->alpha.alphafunc != GL_ALWAYS){
    vidAlphaFunc(l_PrtBuffer.cont->alpha.alphafunc,l_PrtBuffer.cont->alpha.alphaval);
    l_PrtBuffer.renderflag |= RENDER_ALPHATEST;
  }
  else{
    l_PrtBuffer.renderflag &= ~RENDER_ALPHATEST;
  }

  // RenderFlags
  renderflag = l_PrtBuffer.renderflag;
  if (renderflag & RENDER_SUNLIT || renderflag & RENDER_FXLIT){
    renderflag |= RENDER_LIT;
  }

  if (!g_Background->fogmode)
    renderflag &= ~ RENDER_FOG;
  VIDRenderFlag_setGL(renderflag);

  // lights
  vidSetLights(renderflag,0,NULL);

  // fog
  if (g_VID.state.renderflag & RENDER_FOG && g_VID.state.renderflag & RENDER_BLEND){
    switch(g_VID.state.blend.blendmode){
        case VID_MODULATE:
          vidFog(LUX_FALSE);
          break;
        case VID_AMODADD:
        case VID_ADD:
          vidFogColor(black);
          resetfog = LUX_TRUE;
          break;
    }
  }

  return resetfog;
}

static int  ParticleBufferDrawGL_Textured(Mesh_t *drawmesh)
{
  GLParticleState_t *glps  = &l_PState;

  int i;
  int numTexStages;
  ShaderTexGen_t    lmtexgen;

  vidSelectTexture(0);
  vidTexBlendDefault(VID_TEXBLEND_MOD_MOD);
  vidBindTexture(l_PrtBuffer.cont->texRID);
  glMatrixMode(GL_TEXTURE);
  if (l_PrtBuffer.cont->matobj && l_PrtBuffer.cont->matobj->texmatrix)
    glLoadMatrixf(l_PrtBuffer.cont->matobj->texmatrix);
  else
    glLoadIdentity();

  //if (GLEW_EXT_texture_filter_anisotropic)
  //  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0);

  vidTexturing(ResData_getTextureTarget(l_PrtBuffer.cont->texRID));
  numTexStages = 1;

  if (l_PrtBuffer.usevprog < 2){
    Mesh_setTexCoordGL(drawmesh,0,VID_TEXCOORD_0);
    vidTexCoordSource(VID_TEXCOORD_ARRAY,VID_TEXCOORD_0);
  }

  if (g_VID.shdsetup.lightmapRID >= 0){
    lxMatrix44Transpose(lmtexgen.texgenmatrix,l_PrtBuffer.cont->lmProjMat);

    vidSelectTexture(1);
    vidBindTexture(g_VID.shdsetup.lightmapRID);
    vidTexturing(ResData_getTextureTarget(g_VID.shdsetup.lightmapRID));
    vidTexBlendDefault(ResData_getTexture(g_VID.shdsetup.lightmapRID)->lmtexblend);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(g_CamLight.camera->mview);

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

    glTexGenfv(GL_S, GL_EYE_PLANE, lmtexgen.texgenmatrix);
    glTexGenfv(GL_T, GL_EYE_PLANE, &lmtexgen.texgenmatrix[4]);

    if (!l_PrtBuffer.usevprog)
      vidTexCoordSource(VID_TEXCOORD_TEXGEN_ST,VID_TEXCOORD_NOSET);
    else
      vidTexCoordSource(VID_TEXCOORD_NONE,VID_TEXCOORD_NOSET);

    vidClearTexStages(2,i);

    glps->lightmapunit = 1;
  }
  else{
    vidClearTexStages(1,i);
  }

  return numTexStages;
}

static int  ParticleBufferDrawGL_ShaderInit(Mesh_t *drawmesh,Shader_t *shader,int arb)
{ static ShaderTexGen_t   lmtexgen;

  GLParticleState_t *glps  = &l_PState;

  //VIDAlpha_t*   oldalpha = shader->alpha;
  int     numTexStages = shader->numMaxTexPass;
  int i;

  //shader->alpha = &l_PrtBuffer.cont->alpha;
  glps->lmglstage = NULL;

  if (g_VID.shdsetup.lightmapRID >= 0){
    if (shader->cgMode){
      lxMatrix44Transpose(lmtexgen.texgenmatrix,l_PrtBuffer.cont->lmProjMat);
      cgSetMatrixParameterfr(g_VID.cg.prtlmTexgen,lmtexgen.texgenmatrix);
    }
    else{
      GLShaderStage_t *glstage = shader->glpassListHead->glstageListHead;
      for (i = 0 ; i < shader->glpassListHead->numStages; i++){
        if (glstage->stage->stagetype == SHADER_STAGE_TEX &&
          glstage->stage->srcType == TEX_2D_LIGHTMAP){
            glps->oldtexgen = glstage->stage->texgen;
            glstage->stage->texgen = &lmtexgen;
            lmtexgen.texgenmode = VID_TEXCOORD_TEXGEN_ST;
            *(uint*)lmtexgen.enabledaxis = 0x0000FFFF;

            lxMatrix44Transpose(lmtexgen.texgenmatrix,l_PrtBuffer.cont->lmProjMat);

            glps->oldflag = glstage->stage->stageflag;
            glstage->stage->stageflag |= SHADER_EYELINMAP | SHADER_TEXGEN;
            glps->lmglstage = glstage;

            // FIXME might be wrong
            glps->lightmapunit = i;
            break;
        }
        glstage = glstage->next;
      }
    }
  }

  i = LUX_FALSE;
  // make sure pass doesnt screw up alpha
  if (!arb)
    ParticleBufferDraw_setVertexParameters(LUX_FALSE);

  GLShaderPass_activate(shader,shader->glpassListHead,&i);

  {
    int t;
    for (t = 0; t < VID_MAX_TEXTURE_UNITS; t++){
      if (g_VID.state.texcoords[t].type < 0){
        Mesh_setTexCoordGL(drawmesh,t,g_VID.state.texcoords[t].channel);
      }
    }
  }
  if (g_VID.drawsetup.attribs.needed & (1<<VERTEX_ATTRIB_TANGENT)){
    Mesh_setTangentGL(drawmesh);
  }

  return numTexStages;
}

static void ParticleBufferDrawGL_ShaderDeinit(Shader_t *shader)
{
  GLParticleState_t *glps  = &l_PState;
  GLShaderStage_t *lmglstage = glps->lmglstage;

  GLShaderPass_deactivate(shader->glpassListHead);
  if (lmglstage){
    lmglstage->stage->stageflag = glps->oldflag;
    lmglstage->stage->texgen = glps->oldtexgen;
    lmglstage->stage->stageflag &= ~ SHADER_OBJLINMAP;
  }
}

static void ParticleBufferDrawGL_GPUProgs(Mesh_t* drawmesh,void *vertdata, int arb, int vgpu)
{
  int i;
  if (l_PrtBuffer.usevprog > 1){
    // we misuse some arrays for non indexed
    if ((l_PrtBuffer.rot || l_PrtBuffer.useGlobalAxis || vgpu)){
      VertexAttrib_setCustomStreamGL(VERTEX_ATTRIB_ATTR15,drawmesh->vid.vbo,
        VertexSize(drawmesh->vertextype),
        VertexArrayPtr(vertdata,0,drawmesh->vertextype,VERTEX_TEX2),
        VertexCustomAttrib_set(4,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE));
      glEnableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR15);
    }
    // disable all texcoords
    for (i = 1; i < g_VID.state.lasttexStages; i++ ){
      vidSelectTexture(i);
      vidTexCoordSource(VID_TEXCOORD_NONE,VID_TEXCOORD_NOSET);
    }
    vidSelectTexture(0);
    vidTexCoordSource(VID_TEXCOORD_ARRAY,VID_TEXCOORD_NOSET);
    vidSelectClientTexture(0);
    glTexCoordPointer(3,GL_FLOAT,VertexSize(drawmesh->vertextype),VertexArrayPtr(vertdata,0,drawmesh->vertextype,VERTEX_TEX0));
    g_VID.drawsetup.attribs.pointers[VERTEX_ATTRIB_TEXCOORD0].ptr = (void*)-1;


    if (!vgpu){
      if (l_PrtBuffer.useGlobalAxis)
        vidBindVertexProg(ResData_getGpuProgARB(g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_BATCH_ROT_GLOBALAXIS + ((g_VID.shdsetup.lightmapRID >= 0) ? 1 : 0)]));
      else if (l_PrtBuffer.rot)
        vidBindVertexProg(ResData_getGpuProgARB(g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_BATCH_ROT + ((g_VID.shdsetup.lightmapRID >= 0) ? 1 : 0)]));
      else
        vidBindVertexProg(ResData_getGpuProgARB(g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_BATCH + ((g_VID.shdsetup.lightmapRID >= 0) ? 1 : 0)]));
      vidVertexProgram(VID_ARB);
    }

  }
  else{
    // disable all texcoords
    for (i = 1; i < g_VID.state.lasttexStages; i++ ){
      vidSelectTexture(i);
      vidTexCoordSource(VID_TEXCOORD_NONE,VID_TEXCOORD_NOSET);
    }

    vidSelectTexture(0);
    if (!vgpu){
      i = (l_PrtBuffer.renderflag & RENDER_SUNLIT) ? 2 : 0;

      if (l_PrtBuffer.useGlobalAxis)
        vidBindVertexProg(ResData_getGpuProgARB(g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_INSTANCE_GLOBALAXIS+i+((g_VID.shdsetup.lightmapRID >= 0) ? 1 : 0)]));
      else
        vidBindVertexProg(ResData_getGpuProgARB(g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_INSTANCE+i+((g_VID.shdsetup.lightmapRID >= 0) ? 1 : 0)]));
      vidVertexProgram(VID_ARB);
    }
  }

  if(arb)
    ParticleBufferDraw_setVertexParameters(LUX_TRUE);
}

static void ParticleBufferDrawGL_Mesh(Mesh_t *drawmesh,GLParticleMesh_t *glmesh,int flush, int primcnt)
{
  GLParticleState_t *glps  = &l_PState;

  void  *vertdata;
  int   numTexStages;
  int   i;
  booln color;
  booln normals;

#ifdef LUX_PARTICLE_USEBATCHVBO
  if (l_PrtBuffer.vboID[0] && primcnt){
    drawmesh->meshtype = MESH_VBO;
    drawmesh->vid.vbo = l_PrtBuffer.vboID[l_PrtBuffer.curVBO];
    drawmesh->vid.ibo = 0;
    drawmesh->vid.vbooffset = 0;
  }
#endif

  vertdata = drawmesh->vid.vbo ? NULL : drawmesh->vertexData;
  vidBindBufferIndex(NULL);

  // Vertex Attributes
  glEnableClientState(GL_VERTEX_ARRAY);

  if (l_PrtBuffer.colorset && primcnt){
    vidNoVertexColor(LUX_FALSE);
    color = LUX_TRUE;
  }
  else{
    glColor4fv(l_PrtBuffer.cont->color);
    vidNoVertexColor(LUX_TRUE);
    color = LUX_FALSE;

  }
  if (l_PrtBuffer.normalsset && l_PrtBuffer.normalsset < 2){
    normals = LUX_TRUE;
  }
  else{
    glNormal3f(0,0,1);
    normals = LUX_FALSE;
  }

  Mesh_setStandardAttribsGL(drawmesh,color,normals);

  // Texture  & Shader Setup
  if (l_PrtBuffer.texset){
    glps->lightmapunit = -1;
    if (glps->shader){
      numTexStages = ParticleBufferDrawGL_ShaderInit(drawmesh,glps->shader,glps->arb);
    }
    else{
      numTexStages = ParticleBufferDrawGL_Textured(drawmesh);
    }

    if (drawmesh->primtype == PRIM_POINTS){
      for (i = 0; i < numTexStages; i++){
        vidSelectTexture(i);
        vidTexCoordSource(VID_TEXCOORD_NONE,VID_TEXCOORD_NOSET);
        if (i != glps->lightmapunit)
          glTexEnvi(GL_POINT_SPRITE_ARB,GL_COORD_REPLACE_ARB,GL_TRUE);
      }
      glEnable(GL_POINT_SPRITE_ARB);
    }
  }
  else
    vidClearTexStages(0,i);

  // special overrides
  if (g_Draws.drawWire){
    vidBlending(LUX_FALSE);
    vidTexturing(LUX_FALSE);
    vidNoVertexColor(LUX_TRUE);
    glColor4f(1,1,1,1);
    glLineWidth(1);
  }
  vidCheckError();


  // push geo
  if (l_PrtBuffer.usevprog){
    ParticleBufferDrawGL_GPUProgs(drawmesh,vertdata,glps->arb,glps->vgpu);
  }
  else if (!glps->vgpu) vidVertexProgram(VID_FIXED);
  if (!glps->fgpu)    vidFragmentProgram(VID_FIXED);
  if (!glps->ggpu)    vidGeometryProgram(VID_FIXED);

  // draw single call
  if(primcnt){
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    primcnt = USED_INDICES(primcnt);

    if (flush){
      glDrawArrays(drawmesh->primtype, 0,primcnt);
    }
    else{
      if (vertdata && GLEW_EXT_draw_range_elements) // no range for VBO
        glDrawRangeElementsEXT(drawmesh->primtype,0, drawmesh->numVertices-1, primcnt,  GL_UNSIGNED_SHORT ,drawmesh->indicesData16);
      else{
        glDrawElements(drawmesh->primtype, primcnt,  GL_UNSIGNED_SHORT, drawmesh->indicesData16);
      }
    }

    if (l_PrtBuffer.texset && drawmesh->primtype == PRIM_POINTS){
      glDisable(GL_POINT_SPRITE_ARB);

      for (i = 0; i < numTexStages; i++){
        vidSelectTexture(i);
        glTexEnvi(GL_POINT_SPRITE_ARB,GL_COORD_REPLACE_ARB,GL_FALSE);
      }
    }
    if (l_PrtBuffer.cont->contflag & PARTICLE_POINTSMOOTH)
      glDisable(GL_POINT_SMOOTH);

    LUX_PROFILING_OP (g_Profiling.perframe.particle.meshes++);
    LUX_PROFILING_OP (g_Profiling.perframe.particle.vertices += g_VID.drw_meshbuffer->numVertices);
    switch(drawmesh->primtype)
    {
    case PRIM_QUADS:
      LUX_PROFILING_OP (g_Profiling.perframe.particle.tris +=  primcnt/2);
      break;
    case PRIM_TRIANGLES:
      LUX_PROFILING_OP (g_Profiling.perframe.particle.tris +=  primcnt/3);
      break;
    case PRIM_POINTS:
      LUX_PROFILING_OP (g_Profiling.perframe.particle.tris +=  primcnt);
      break;
    }
  }
  else {
    // draw instanced calls
    GLParticles_drawInstanceMesh(drawmesh);
  }

  if(normals){
    glDisableClientState(GL_NORMAL_ARRAY);
  }

  if (glps->shader){
    ParticleBufferDrawGL_ShaderDeinit(glps->shader);
  }

  if (l_PrtBuffer.usevprog && l_PrtBuffer.rot){
    glDisableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR15);
  }

}

// PARTICLE BUFFER DRAW
static void ParticleBufferDrawGL(){
  GLParticleState_t *glps  = &l_PState;

  Mesh_t        *drawmesh = NULL;
  GLParticleMesh_t  *glmesh = NULL;
  int         flush = LUX_FALSE;
  int         primcnt;
  int         resetfog;

  // sort list by z value if blending is done
  if (l_PrtBuffer.sort){
    uint *useindices = lxSortRadixArrayFloat(l_PrtBuffer.drawZ,l_PrtBuffer.drawSize,l_PrtBuffer.drawIndices,&l_PrtBuffer.drawIndices[MAX_PARTICLES]);
    if (useindices != l_PrtBuffer.drawIndices){
      memcpy(l_PrtBuffer.drawIndices,useindices,sizeof(uint)*l_PrtBuffer.drawSize);
    }
  }
  else
    l_PrtBuffer.sort = 1;

  g_VID.drw_meshbuffer->numVertices = 0;
  g_VID.drawsetup.curnode = l_PrtBuffer.cont;

  resetfog = ParticleBufferDrawGL_State();

  // create geometry or do direct drawing
  switch(l_PrtBuffer.cont->particletype){
    case PARTICLE_TYPE_POINT:
      glPointSize(l_PrtBuffer.cont->size*(((float)g_VID.windowWidth/640.0f)+0.4f));
      l_PrtBuffer.normalsset = LUX_FALSE;
      GLParticles_pushPoint();
      primcnt = g_VID.drw_meshbuffer->numVertices;
      flush = LUX_TRUE;
      if (l_PrtBuffer.cont->contflag & PARTICLE_POINTSMOOTH)
        glEnable(GL_POINT_SMOOTH);
      if (GLEW_ARB_point_parameters){
        ParticlePointParams_t *params = &l_PrtBuffer.cont->pparams;
        glPointParameterfARB(GL_POINT_SIZE_MIN_ARB,params->min);
        glPointParameterfARB(GL_POINT_SIZE_MAX_ARB,params->max);
        glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE_ARB,params->alphathresh);
        glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB,params->dist);
      }

      drawmesh = g_VID.drw_meshbuffer;
      drawmesh->meshtype = MESH_VA;
      drawmesh->primtype = PRIM_POINTS;

      break;
    case PARTICLE_TYPE_INSTMESH:
    case PARTICLE_TYPE_TRIANGLE:
    case PARTICLE_TYPE_QUAD:
    case PARTICLE_TYPE_HSPHERE:

      switch(l_PrtBuffer.cont->particletype){
        case PARTICLE_TYPE_INSTMESH:
          glmesh = l_PrtBuffer.cont->instmesh;
          flush = LUX_FALSE;
          break;
        case PARTICLE_TYPE_TRIANGLE:
          glmesh = &l_PrtBuffer.trimesh;
          flush = LUX_TRUE;
          break;
        case PARTICLE_TYPE_QUAD:
          glmesh = &l_PrtBuffer.quadmesh;
          flush = LUX_TRUE;
          break;
        case PARTICLE_TYPE_HSPHERE:
          glmesh = &l_PrtBuffer.hspheremesh;
          flush = LUX_FALSE;
          break;
      }
      if ((glps->shader && glps->shader->shaderflag & SHADER_NORMALS) || l_PrtBuffer.renderflag & RENDER_SUNLIT){
          l_PrtBuffer.normalsset = (l_PrtBuffer.cont->contflag & PARTICLE_SINGLENORMAL) ?  2 : glmesh->hasnormals;      }
      else  l_PrtBuffer.normalsset = LUX_FALSE;

      // we only allow world transformed vprog normals
      if (glps->vgpu || (g_VID.gensetup.hasVprogs && (!l_PrtBuffer.normalsset || l_PrtBuffer.normalsset == 2) && !(l_PrtBuffer.cont->contflag & PARTICLE_NOGPU)))
        l_PrtBuffer.usevprog = 2;

      if (!glmesh->batchverts || l_PrtBuffer.cont->contflag & PARTICLE_FORCEINSTANCE){
        primcnt = 0;
        drawmesh = &glmesh->mesh;
        l_PrtBuffer.usevprog = (l_PrtBuffer.usevprog ) ? 1 : 0;
        break;
      }

      // for the moment we dont have lit batch vprogs
      if (l_PrtBuffer.usevprog && (!glps->vgpu && l_PrtBuffer.renderflag & RENDER_SUNLIT))
        l_PrtBuffer.usevprog = 0;

      GLParticles_pushGeo(glmesh);
      primcnt = l_PrtBuffer.drawSize * ((glmesh->mesh.numIndices) ? glmesh->mesh.numIndices : glmesh->mesh.numVertices);
      drawmesh = g_VID.drw_meshbuffer;
      drawmesh->meshtype = MESH_VA;
      drawmesh->primtype = glmesh->mesh.primtype;
      break;
    case PARTICLE_TYPE_SPHERE:
      GLParticles_drawModel(1);
      break;
    case PARTICLE_TYPE_MODEL:
      GLParticles_drawModel(0);
      break;
    default:
      return;
  }

  if (g_VID.gpuvendor != VID_VENDOR_UNKNOWN)
    flush = LUX_FALSE;
  // draw the created geometry
  if (drawmesh)
    ParticleBufferDrawGL_Mesh(drawmesh,glmesh,flush,primcnt);


  if (resetfog){
    vidFogColor(g_Background->fogcolor);
  }
  g_VID.shdsetup.lightmapRID = -1;
  LUX_PROFILING_OP (g_Profiling.perframe.particle.rendered += l_PrtBuffer.drawSize);

  vidCheckError();
}

//////////////////////////////////////////////////////////////////////////
// GLParticles PreProcessing

static void GLParticles_process(GLParticleProcess_t proctype, int start, int end, ParticleContainer_t *pcont){
  lxVector4     vec;
  int       color[4];

  float     temp;
  float     temp2;
  float     sinf;
  float     cosf;
  uchar     *pColorB;
  GLParticle_t  *drawPrt;
  float     *drawDist;
  int i;
  int unrollend;
  int prob;
  int fademin;
  int fadesize;
  int fademax;
  ParticleLife_t  *life;


#define PRT_RAND(seed)    ((uint)drawPrt->prtSt)/seed


  life = (pcont) ? pcont->life : NULL;

  drawPrt = &l_PrtBuffer.drawParticles[start];
  unrollend = start + ((end-start)%PARTICLE_UNROLLSIZE);
  switch(proctype) {
  case GLPARTICLE_PROCESS_ROT_VEL:
    for (i = start; i < end; i++,drawPrt++){
      lxVector3TransformRot(vec,drawPrt->normal,g_VID.drawsetup.viewMatrix);
      //rotate angle by -90 hence swap sin/cos and invert sin
      drawPrt->speed = lxVector3GetRotateZcossin(vec,&drawPrt->sinf,&drawPrt->cosf);
      drawPrt->sinf = -drawPrt->sinf;
    }
    break;
  case GLPARTICLE_PROCESS_ROT_AND_VEL:
    for (i = start; i < end; i++,drawPrt++){
      lxVector3TransformRot(vec,drawPrt->normal,g_VID.drawsetup.viewMatrix);
      //rotate angle by -90 hence swap sin/cos and invert sin
      drawPrt->speed = lxVector3GetRotateZcossin(vec,&sinf,&cosf);
      sinf = -sinf;
      // add original rotation
      // sin (a+b) = sin (a) cos (b) + cos (a) sin (b)
      // cos (a+b) = cos (a) cos (b) - sin (a) sin (b)
      temp  = lxFastSin(drawPrt->rot);
      temp2 = lxFastCos(drawPrt->rot);
      drawPrt->sinf = (sinf * temp2) + (cosf * temp);
      drawPrt->cosf = (cosf * temp2) - (sinf * temp);
    }
    break;
  case GLPARTICLE_PROCESS_ROT:
    for (i = start; i < unrollend; i++){
      drawPrt->cosf = lxFastCos(drawPrt->rot);
      drawPrt->sinf = lxFastSin(drawPrt->rot);
      drawPrt->speed = 1;
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      drawPrt->cosf = lxFastCos(drawPrt->rot);
      drawPrt->sinf = lxFastSin(drawPrt->rot);
      drawPrt->speed = 1;
      drawPrt++;
      drawPrt->cosf = lxFastCos(drawPrt->rot);
      drawPrt->sinf = lxFastSin(drawPrt->rot);
      drawPrt->speed = 1;
      drawPrt++;
      drawPrt->cosf = lxFastCos(drawPrt->rot);
      drawPrt->sinf = lxFastSin(drawPrt->rot);
      drawPrt->speed = 1;
      drawPrt++;
      drawPrt->cosf = lxFastCos(drawPrt->rot);
      drawPrt->sinf = lxFastSin(drawPrt->rot);
      drawPrt->speed = 1;
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_ROT_NONE:
    for (i = start; i < unrollend; i++){
      drawPrt->cosf = 1;
      drawPrt->sinf = 0;
      drawPrt->speed = 1;
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      drawPrt->cosf = 1;
      drawPrt->sinf = 0;
      drawPrt->speed = 1;
      drawPrt++;
      drawPrt->cosf = 1;
      drawPrt->sinf = 0;
      drawPrt->speed = 1;
      drawPrt++;
      drawPrt->cosf = 1;
      drawPrt->sinf = 0;
      drawPrt->speed = 1;
      drawPrt++;
      drawPrt->cosf = 1;
      drawPrt->sinf = 0;
      drawPrt->speed = 1;
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_ROT_CAMFIX:
    lxVector3GetRotateZcossin(g_CamLight.camera->fwd,&cosf,&sinf);
    for (i = start; i < unrollend; i++){
      temp  = lxFastSin(drawPrt->rot);
      temp2 = lxFastCos(drawPrt->rot);
      drawPrt->sinf = (temp * cosf) - (temp2 * sinf);
      drawPrt->cosf = (temp2* cosf) + (temp  * sinf);
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      temp  = lxFastSin(drawPrt->rot);
      temp2 = lxFastCos(drawPrt->rot);
      drawPrt->sinf = (temp * cosf) - (temp2 * sinf);
      drawPrt->cosf = (temp2* cosf) + (temp  * sinf);
      drawPrt++;
      temp  = lxFastSin(drawPrt->rot);
      temp2 = lxFastCos(drawPrt->rot);
      drawPrt->sinf = (temp * cosf) - (temp2 * sinf);
      drawPrt->cosf = (temp2* cosf) + (temp  * sinf);
      drawPrt++;
      temp  = lxFastSin(drawPrt->rot);
      temp2 = lxFastCos(drawPrt->rot);
      drawPrt->sinf = (temp * cosf) - (temp2 * sinf);
      drawPrt->cosf = (temp2* cosf) + (temp  * sinf);
      drawPrt++;
      temp  = lxFastSin(drawPrt->rot);
      temp2 = lxFastCos(drawPrt->rot);
      drawPrt->sinf = (temp * cosf) - (temp2 * sinf);
      drawPrt->cosf = (temp2* cosf) + (temp  * sinf);
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_COLOR_NONE:
    pColorB = (uchar*)color;
    if (pcont->contflag & PARTICLE_COLORMUL){
      lxVector4float_FROM_ubyte(vec,pcont->userColor);
      lxVector4Mul(vec,pcont->color,vec);
      lxVector4ubyte_FROM_float(pColorB,vec);
    }
    else
      lxVector4ubyte_FROM_float(pColorB,pcont->color);


    for (i = start; i < unrollend; i++){
      drawPrt->colorc = color[0];
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      drawPrt->colorc = color[0];
      drawPrt++;
      drawPrt->colorc = color[0];
      drawPrt++;
      drawPrt->colorc = color[0];
      drawPrt++;
      drawPrt->colorc = color[0];
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_COLOR:
    for (i = start; i < unrollend; i++){
      drawPrt->colorc = l_PrtBuffer.intColorsC[drawPrt->timeStep];
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      drawPrt->colorc = l_PrtBuffer.intColorsC[drawPrt->timeStep];
      drawPrt++;
      drawPrt->colorc = l_PrtBuffer.intColorsC[drawPrt->timeStep];
      drawPrt++;
      drawPrt->colorc = l_PrtBuffer.intColorsC[drawPrt->timeStep];
      drawPrt++;
      drawPrt->colorc = l_PrtBuffer.intColorsC[drawPrt->timeStep];
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_COLOR_VAR:
    for (i = start; i < end; i++,drawPrt++){
      /*
      flt = &life->intColors[4*drawPrt->timeStep];
      vec[0] = variance(flt[0],life->colorVar[0],frandPointer(drawPrt->prtSt));
      vec[1] = variance(flt[1],life->colorVar[1],frandPointer(drawPrt->prtSt+1));
      vec[2] = variance(flt[2],life->colorVar[2],frandPointer(drawPrt->prtSt+2));
      vec[3] = variance(flt[3],life->colorVar[3],frandPointer(drawPrt->prtSt+3));
      Vector4ubyte_FROM_float(drawPrt->colorb,vec,temp);
      */
      pColorB = (uchar*)&l_PrtBuffer.intColorsC[drawPrt->timeStep];
      color[0] = (int)pColorB[0] - l_PrtBuffer.colorVarB[0] + randPointerSeed(drawPrt->prtSt,0,3,l_PrtBuffer.colorVarB2[0]);
      color[1] = (int)pColorB[1] - l_PrtBuffer.colorVarB[1] + randPointerSeed(drawPrt->prtSt,1,3,l_PrtBuffer.colorVarB2[1]);
      color[2] = (int)pColorB[2] - l_PrtBuffer.colorVarB[2] + randPointerSeed(drawPrt->prtSt,2,3,l_PrtBuffer.colorVarB2[2]);
      color[3] = (int)pColorB[3] - l_PrtBuffer.colorVarB[3] + randPointerSeed(drawPrt->prtSt,3,3,l_PrtBuffer.colorVarB2[3]);
      drawPrt->colorb[0] = LUX_MIN(255,LUX_MAX(0,color[0]));
      drawPrt->colorb[1] = LUX_MIN(255,LUX_MAX(0,color[1]));
      drawPrt->colorb[2] = LUX_MIN(255,LUX_MAX(0,color[2]));
      drawPrt->colorb[3] = LUX_MIN(255,LUX_MAX(0,color[3]));

    }
    break;
  case GLPARTICLE_PROCESS_COLOR_NOAGE:
    for (i = start; i < unrollend; i++){
      drawPrt->colorc  = l_PrtBuffer.intColorsC[((PRT_RAND(7)) % life->numColors)];
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      drawPrt->colorc  = l_PrtBuffer.intColorsC[((PRT_RAND(7)) % life->numColors)];
      drawPrt++;
      drawPrt->colorc  = l_PrtBuffer.intColorsC[((PRT_RAND(7)) % life->numColors)];
      drawPrt++;
      drawPrt->colorc  = l_PrtBuffer.intColorsC[((PRT_RAND(7)) % life->numColors)];
      drawPrt++;
      drawPrt->colorc  = l_PrtBuffer.intColorsC[((PRT_RAND(7)) % life->numColors)];
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_COLOR_NOAGE_VAR:
    for (i = start; i < end; i++,drawPrt++){
      pColorB = (uchar*)&l_PrtBuffer.intColorsC[((PRT_RAND(7)) % life->numColors)];
      color[0] = (int)pColorB[0] - l_PrtBuffer.colorVarB[0] + randPointerSeed(drawPrt->prtSt,0,3,l_PrtBuffer.colorVarB2[0]);
      color[1] = (int)pColorB[1] - l_PrtBuffer.colorVarB[1] + randPointerSeed(drawPrt->prtSt,1,3,l_PrtBuffer.colorVarB2[1]);
      color[2] = (int)pColorB[2] - l_PrtBuffer.colorVarB[2] + randPointerSeed(drawPrt->prtSt,2,3,l_PrtBuffer.colorVarB2[2]);
      color[3] = (int)pColorB[3] - l_PrtBuffer.colorVarB[3] + randPointerSeed(drawPrt->prtSt,3,3,l_PrtBuffer.colorVarB2[3]);
      drawPrt->colorb[0] = LUX_MIN(255,LUX_MAX(0,color[0]));
      drawPrt->colorb[1] = LUX_MIN(255,LUX_MAX(0,color[1]));
      drawPrt->colorb[2] = LUX_MIN(255,LUX_MAX(0,color[2]));
      drawPrt->colorb[3] = LUX_MIN(255,LUX_MAX(0,color[3]));
      /*
      flt = &life->colors[((((uint)drawPrt->prtDyn)*7) % life->numColors)*4];
      vec[0] = variance(flt[0],life->colorVar[0],frandPointer(drawPrt->prtSt));
      vec[1] = variance(flt[1],life->colorVar[1],frandPointer(drawPrt->prtSt+1));
      vec[2] = variance(flt[2],life->colorVar[2],frandPointer(drawPrt->prtSt+2));
      vec[3] = variance(flt[3],life->colorVar[3],frandPointer(drawPrt->prtSt+3));
      Vector4ubyte_FROM_float(drawPrt->colorb,vec,temp);
      */
    }
    break;
  case GLPARTICLE_PROCESS_COLOR_USERMUL:
    for (i = start; i < unrollend; i++){
      drawPrt->colorb[0] = (drawPrt->colorb[0]*pcont->userColor[0])/255;
      drawPrt->colorb[1] = (drawPrt->colorb[1]*pcont->userColor[1])/255;
      drawPrt->colorb[2] = (drawPrt->colorb[2]*pcont->userColor[2])/255;
      drawPrt->colorb[3] = (drawPrt->colorb[3]*pcont->userColor[3])/255;
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      drawPrt->colorb[0] = (drawPrt->colorb[0]*pcont->userColor[0])/255;
      drawPrt->colorb[1] = (drawPrt->colorb[1]*pcont->userColor[1])/255;
      drawPrt->colorb[2] = (drawPrt->colorb[2]*pcont->userColor[2])/255;
      drawPrt->colorb[3] = (drawPrt->colorb[3]*pcont->userColor[3])/255;
      drawPrt++;
      drawPrt->colorb[0] = (drawPrt->colorb[0]*pcont->userColor[0])/255;
      drawPrt->colorb[1] = (drawPrt->colorb[1]*pcont->userColor[1])/255;
      drawPrt->colorb[2] = (drawPrt->colorb[2]*pcont->userColor[2])/255;
      drawPrt->colorb[3] = (drawPrt->colorb[3]*pcont->userColor[3])/255;
      drawPrt++;
      drawPrt->colorb[0] = (drawPrt->colorb[0]*pcont->userColor[0])/255;
      drawPrt->colorb[1] = (drawPrt->colorb[1]*pcont->userColor[1])/255;
      drawPrt->colorb[2] = (drawPrt->colorb[2]*pcont->userColor[2])/255;
      drawPrt->colorb[3] = (drawPrt->colorb[3]*pcont->userColor[3])/255;
      drawPrt++;
      drawPrt->colorb[0] = (drawPrt->colorb[0]*pcont->userColor[0])/255;
      drawPrt->colorb[1] = (drawPrt->colorb[1]*pcont->userColor[1])/255;
      drawPrt->colorb[2] = (drawPrt->colorb[2]*pcont->userColor[2])/255;
      drawPrt->colorb[3] = (drawPrt->colorb[3]*pcont->userColor[3])/255;
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_FADEOUT:
    fademin = (int)((pcont->userProbability-pcont->userProbabilityFadeOut)*255.0f);
    fadesize = (int)(pcont->userProbabilityFadeOut*255.0f);
    fademax = (int)(pcont->userProbability*255.0f);
    for (i = start; i < unrollend; i++){
      prob = (int)(frandPointer(drawPrt->prtDyn)*255.0f);
      drawPrt->colorb[3] = (prob > fademin) ? (((int)drawPrt->colorb[3]*(fademax-prob))/fadesize) : drawPrt->colorb[3];
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      prob = (int)(frandPointer(drawPrt->prtDyn)*255.0f);
      drawPrt->colorb[3] = (prob > fademin) ? (((int)drawPrt->colorb[3]*(fademax-prob))/fadesize) : drawPrt->colorb[3];
      drawPrt++;
      prob = (int)(frandPointer(drawPrt->prtDyn)*255.0f);
      drawPrt->colorb[3] = (prob > fademin) ? (((int)drawPrt->colorb[3]*(fademax-prob))/fadesize) : drawPrt->colorb[3];
      drawPrt++;
      prob = (int)(frandPointer(drawPrt->prtDyn)*255.0f);
      drawPrt->colorb[3] = (prob > fademin) ? (((int)drawPrt->colorb[3]*(fademax-prob))/fadesize) : drawPrt->colorb[3];
      drawPrt++;
      prob = (int)(frandPointer(drawPrt->prtDyn)*255.0f);
      drawPrt->colorb[3] = (prob > fademin) ? (((int)drawPrt->colorb[3]*(fademax-prob))/fadesize) : drawPrt->colorb[3];
      drawPrt++;
    }

    break;
  case GLPARTICLE_PROCESS_SIZE:
    for (i = start; i < unrollend; i++){
      drawPrt->size = life->intSize[drawPrt->timeStep]*drawPrt->size;
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      drawPrt->size = life->intSize[drawPrt->timeStep]*drawPrt->size;
      drawPrt++;
      drawPrt->size = life->intSize[drawPrt->timeStep]*drawPrt->size;
      drawPrt++;
      drawPrt->size = life->intSize[drawPrt->timeStep]*drawPrt->size;
      drawPrt++;
      drawPrt->size = life->intSize[drawPrt->timeStep]*drawPrt->size;
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_SIZE_VAR:
    for (i = start; i < unrollend; i++){
      drawPrt->size = lxVariance(life->intSize[drawPrt->timeStep]*drawPrt->size,life->sizeVar,frandPointer(drawPrt->prtDyn));
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      drawPrt->size = lxVariance(life->intSize[drawPrt->timeStep]*drawPrt->size,life->sizeVar,frandPointer(drawPrt->prtDyn));
      drawPrt++;
      drawPrt->size = lxVariance(life->intSize[drawPrt->timeStep]*drawPrt->size,life->sizeVar,frandPointer(drawPrt->prtDyn));
      drawPrt++;
      drawPrt->size = lxVariance(life->intSize[drawPrt->timeStep]*drawPrt->size,life->sizeVar,frandPointer(drawPrt->prtDyn));
      drawPrt++;
      drawPrt->size = lxVariance(life->intSize[drawPrt->timeStep]*drawPrt->size,life->sizeVar,frandPointer(drawPrt->prtDyn));
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_SIZE_USERMUL:
    for (i = start; i < unrollend; i++){
      drawPrt->size *= pcont->userSize;
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      drawPrt->size *= pcont->userSize;
      drawPrt++;
      drawPrt->size *= pcont->userSize;
      drawPrt++;
      drawPrt->size *= pcont->userSize;
      drawPrt++;
      drawPrt->size *= pcont->userSize;
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_TEX:
    for (i = start; i < unrollend; i++){
      drawPrt->texoffset += life->intTex[drawPrt->timeStep];
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      drawPrt->texoffset += life->intTex[drawPrt->timeStep];
      drawPrt++;
      drawPrt->texoffset += life->intTex[drawPrt->timeStep];
      drawPrt++;
      drawPrt->texoffset += life->intTex[drawPrt->timeStep];
      drawPrt++;
      drawPrt->texoffset += life->intTex[drawPrt->timeStep];
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_TEX_NOAGE:
    for (i = start; i < unrollend; i++){
      drawPrt->texoffset += (PRT_RAND(13)) % life->numTex;
      drawPrt++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      drawPrt->texoffset += (PRT_RAND(13)) % life->numTex;
      drawPrt++;
      drawPrt->texoffset += (PRT_RAND(13)) % life->numTex;
      drawPrt++;
      drawPrt->texoffset += (PRT_RAND(13)) % life->numTex;
      drawPrt++;
      drawPrt->texoffset += (PRT_RAND(13)) % life->numTex;
      drawPrt++;
    }
    break;
  case GLPARTICLE_PROCESS_TURBULENCE:
    for (i = start; i < end; i++,drawPrt++){
      vec[0] = lxFastCos(g_LuxTimer.time*0.01+frandPointerSeed(drawPrt,17)*5)*life->intTurb[drawPrt->timeStep];
      vec[1] = lxFastSin(g_LuxTimer.time*0.01+frandPointerSeed(drawPrt,5)*5)*life->intTurb[drawPrt->timeStep];
      vec[2] = lxFastSin(g_LuxTimer.time*0.01+frandPointerSeed(drawPrt,13)*5)*life->intTurb[drawPrt->timeStep];
      lxVector3Add(drawPrt->viewpos,drawPrt->viewpos,vec);
    }
    break;
  case GLPARTICLE_PROCESS_SORTDIST:
    drawDist = l_PrtBuffer.drawZ+start;
    for (i = start; i < unrollend; i++){
      *drawDist = drawPrt->viewpos[2];
      drawPrt++;drawDist++;
    }
    for (; i < end; i+=PARTICLE_UNROLLSIZE){
      *drawDist = drawPrt->viewpos[2];
      drawPrt++;drawDist++;
      *drawDist = drawPrt->viewpos[2];
      drawPrt++;drawDist++;
      *drawDist = drawPrt->viewpos[2];
      drawPrt++;drawDist++;
      *drawDist = drawPrt->viewpos[2];
      drawPrt++;drawDist++;
    }
    break;
  default:
    break;
  }

#undef PRT_RAND
}



//////////////////////////////////////////////////////////////////////////
// ParticleSys Preparation

// returns last used viewpos
static float* Draw_ParticleSys_fillDrawPrts(ParticleSys_t *psys, float *viewposarray, const float numtex)
{
  ParticleDyn_t *prt;
  ParticleContainer_t *pcont;
  GLParticle_t  *drawPrt;
  float     *curviewpos;
  int       *drawIndex;
  int       start;
#ifdef LUX_PARTICLE_USEPOINTERITERATOR
  ParticleDyn_t **prtptr;
#endif
  ParticleLife_t  *life = &psys->life;
  lxVector3     pos;
  lxVector3     camThresh;
  lxVector3     camMul;
  lxVector3     camDir;
  lxVector3     camShift;

  GLParticleProcess_t proc;
  int camdeath;


  pcont = &psys->container;
  curviewpos = viewposarray;
  start =  l_PrtBuffer.drawSize;

#ifdef LUX_PARTICLE_USEPOINTERITERATOR
  #define PLIST_RESET   prtptr = pcont->activePrts;
  #define PLIST_START   while(  (prt = *(prtptr++)) && curviewpos < (float*)l_PrtBuffer.drawParticles){
  #define PLIST_END   curviewpos+=3;}
#else
  #define PLIST_RESET   prt = pcont->particles;
  #define PLIST_START   while(  curviewpos < (float*)l_PrtBuffer.drawParticles){
  #define PLIST_END   curviewpos+=3;prt++;}
#endif
#define PLIST_VIEWPOS   curviewpos

  // transform to viewspace
  PLIST_RESET
  if (psys->psysflag & PARTICLE_VOLUME){
    lxVector3Scale(camThresh,psys->volumeSize,0.5f);

    lxVector3ScaledAdd(camMul,g_CamLight.camera->pos,psys->volumeDist,g_CamLight.camera->fwd);

    camMul[0] = LUX_LERP(psys->volumeCamInfluence[0],psys->volumeOffset[0],camMul[0]);
    camMul[1] = LUX_LERP(psys->volumeCamInfluence[1],psys->volumeOffset[1],camMul[1]);
    camMul[2] = LUX_LERP(psys->volumeCamInfluence[2],psys->volumeOffset[2],camMul[2]);

    lxVector3Sub(camThresh,camMul,camThresh);
    lxVector3Copy(camDir,camThresh);
    // cam = vol center -> frustum center

    camThresh[0] = fmod(camThresh[0],psys->volumeSize[0]);
    camThresh[1] = fmod(camThresh[1],psys->volumeSize[1]);
    camThresh[2] = fmod(camThresh[2],psys->volumeSize[2]);
    lxVector3Sub(camDir,camDir,camThresh);

    camMul[0] = (camThresh[0] < 0 ) ? -1.0f  : 1.0f;
    camMul[1] = (camThresh[1] < 0 ) ? -1.0f  : 1.0f;
    camMul[2] = (camThresh[2] < 0 ) ? -1.0f  : 1.0f;
    lxVector3Mul(camShift,camMul,psys->volumeSize);

    camThresh[0] = (camThresh[0] < 0 ) ? camThresh[0]+psys->volumeSize[0] : camThresh[0];
    camThresh[1] = (camThresh[1] < 0 ) ? camThresh[1]+psys->volumeSize[1] : camThresh[1];
    camThresh[2] = (camThresh[2] < 0 ) ? camThresh[2]+psys->volumeSize[2] : camThresh[2];

    PLIST_START
#define POSWRAP(i)  pos[i] = (prt->pos[i]*camMul[i] < camThresh[i]*camMul[i]) ? prt->pos[i]+camShift[i] : prt->pos[i]
      POSWRAP(0);
      POSWRAP(1);
      POSWRAP(2);
#undef  POSWRAP

      lxVector3Add(pos,camDir,pos);
      lxVector3Transform(PLIST_VIEWPOS,pos,g_VID.drawsetup.viewMatrix);
    PLIST_END
  }
  else{
    prt = pcont->particles;
    switch(pcont->check)
    {
    case 1: //x
      PLIST_START
        // set Z to positive so it wont be drawn
        if (PARTICLE_CHECKAXIS(0) )
          PLIST_VIEWPOS[2] = 1.0f;
        else
          lxVector3Transform(PLIST_VIEWPOS,prt->pos,g_VID.drawsetup.viewMatrix);
      PLIST_END
      break;
    case 2: //y
      PLIST_START
        // set Z to positive so it wont be drawn
        if (PARTICLE_CHECKAXIS(1) )
          curviewpos[2] = 1.0f;
        else
          lxVector3Transform(PLIST_VIEWPOS,prt->pos,g_VID.drawsetup.viewMatrix);
      PLIST_END
      break;
    case 3: //xy
      PLIST_START
        // set Z to positive so it wont be drawn
        if (PARTICLE_CHECKAXIS(0) || PARTICLE_CHECKAXIS(1))
          PLIST_VIEWPOS[2] = 1.0f;
        else
          lxVector3Transform(PLIST_VIEWPOS,prt->pos,g_VID.drawsetup.viewMatrix);
      PLIST_END
      break;
    case 4: //z
      PLIST_START
        // set Z to positive so it wont be drawn
        if (PARTICLE_CHECKAXIS(2))
          PLIST_VIEWPOS[2] = 1.0f;
        else
          lxVector3Transform(PLIST_VIEWPOS,prt->pos,g_VID.drawsetup.viewMatrix);
      PLIST_END
      break;
    case 5: //zx
      PLIST_START
        // set Z to positive so it wont be drawn
        if (PARTICLE_CHECKAXIS(0) || PARTICLE_CHECKAXIS(2))
          PLIST_VIEWPOS[2] = 1.0f;
        else
          lxVector3Transform(PLIST_VIEWPOS,prt->pos,g_VID.drawsetup.viewMatrix);
      PLIST_END
      break;
    case 6: //zy
      PLIST_START
        // set Z to positive so it wont be drawn
        if (PARTICLE_CHECKAXIS(1) || PARTICLE_CHECKAXIS(2))
          PLIST_VIEWPOS[2] = 1.0f;
        else
          lxVector3Transform(PLIST_VIEWPOS,prt->pos,g_VID.drawsetup.viewMatrix);
      PLIST_END
      break;
    case 7: //xyz
      PLIST_START
        // set Z to positive so it wont be drawn
        if (PARTICLE_CHECKAXIS(1) || PARTICLE_CHECKAXIS(2) || PARTICLE_CHECKAXIS(3))
          PLIST_VIEWPOS[2] = 1.0f;
        else
          lxVector3Transform(PLIST_VIEWPOS,prt->pos,g_VID.drawsetup.viewMatrix);
      PLIST_END
      break;
    default:
      PLIST_START
        lxVector3Transform(PLIST_VIEWPOS,prt->pos,g_VID.drawsetup.viewMatrix);
      PLIST_END
    }
  }

  if (pcont->contflag & PARTICLE_PROBABILITY){
    curviewpos = viewposarray;
    PLIST_RESET
    PLIST_START
      // set Z to positive so it wont be drawn
      PLIST_VIEWPOS[2] = (LUX_FP_LESS_ZERO(PLIST_VIEWPOS[2]) && frandPointer(prt) < pcont->userProbability) ? PLIST_VIEWPOS[2] : 1.0f;
    PLIST_END
  }


  // set pointers to begin
  curviewpos = viewposarray;
  drawPrt = l_PrtBuffer.drawParticles + start;
  drawIndex = l_PrtBuffer.drawIndices + start;

  camdeath = life->lifeflag & PARTICLE_DIECAMPLANE;
  // build drawlist
  PLIST_RESET
  PLIST_START

    LUX_ASSERT(prt->age >= 1 && prt->age < prt->life);
    LUX_ASSERT(prt->timeStep >= 0 && prt->timeStep < PARTICLE_STEPS);

    // add it to draw
    if (PLIST_VIEWPOS[2] < 0 && PLIST_VIEWPOS[2] > -g_CamLight.camera->backplane){
      drawPrt->prtDyn = prt;
      drawPrt->viewpos=PLIST_VIEWPOS;
      drawPrt->normal = prt->vel;
      drawPrt->rot = prt->rot;
      drawPrt->size = prt->size;
      drawPrt->texoffset = numtex;
      drawPrt->timeStep = prt->timeStep;
      drawPrt->relage = (float)drawPrt->timeStep * PARTICLE_DIV_STEPS;

      *drawIndex = l_PrtBuffer.drawSize;

      l_PrtBuffer.drawSize++;
      drawIndex++;
      drawPrt++;
    }
    else if (camdeath){
      prt->age = prt->life+1;
    }
  PLIST_END

#undef PLIST_RESET
#undef PLIST_START
#undef PLIST_END

  if (l_PrtBuffer.drawSize != start)
  {
    if (life->lifeflag & PARTICLE_TURB)
      GLParticles_process(GLPARTICLE_PROCESS_TURBULENCE,start,l_PrtBuffer.drawSize,pcont);

    // rotation
    if ((psys->container.contflag & PARTICLE_CAMROTFIX) && !(life->lifeflag & PARTICLE_ROTVEL)){
      GLParticles_process(GLPARTICLE_PROCESS_ROT_CAMFIX,start,l_PrtBuffer.drawSize,NULL);
      l_PrtBuffer.rot = LUX_TRUE;
    }
    else{
      proc = ((life->lifeflag & PARTICLE_ROTATE || life->lifeflag & PARTICLE_ROTOFFSET) ? GLPARTICLE_PROCESS_ROT : GLPARTICLE_PROCESS_ROT_NONE);
      if (life->lifeflag & PARTICLE_ROTVEL )
        proc +=1;
      GLParticles_process(proc,start,l_PrtBuffer.drawSize,pcont);
      l_PrtBuffer.rot |= proc;
    }


    proc = (life->lifeflag & PARTICLE_SIZEVAR) ? GLPARTICLE_PROCESS_SIZE_VAR : GLPARTICLE_PROCESS_SIZE;
    GLParticles_process(proc,start,l_PrtBuffer.drawSize,pcont);

    proc = (life->lifeflag & PARTICLE_NOTEXAGE) ? GLPARTICLE_PROCESS_TEX_NOAGE : GLPARTICLE_PROCESS_TEX;
    GLParticles_process(proc,start,l_PrtBuffer.drawSize,pcont);

    if (pcont->contflag & PARTICLE_COLORMUL){
      l_PrtBuffer.intColorsC = life->intColorsMulC;
      l_PrtBuffer.colorVarB =  life->colorVarMulB;
      l_PrtBuffer.colorVarB2 = life->colorVarMulB2;
    }
    else{
      l_PrtBuffer.intColorsC = life->intColorsC;
      l_PrtBuffer.colorVarB =  life->colorVarB;
      l_PrtBuffer.colorVarB2 = life->colorVarB2;
    }

    proc = (life->lifeflag & PARTICLE_NOCOLAGE) ? GLPARTICLE_PROCESS_COLOR_NOAGE : GLPARTICLE_PROCESS_COLOR;
    proc += (life->lifeflag & PARTICLE_COLVAR) ? 1 : 0;
    GLParticles_process(proc,start,l_PrtBuffer.drawSize,pcont);


    if (pcont->contflag & PARTICLE_SIZEMUL){
      GLParticles_process(GLPARTICLE_PROCESS_SIZE_USERMUL,start,l_PrtBuffer.drawSize,pcont);
    }
    if (pcont->contflag & PARTICLE_PROBABILITY && pcont->userProbabilityFadeOut){
      GLParticles_process(GLPARTICLE_PROCESS_FADEOUT,start,l_PrtBuffer.drawSize,pcont);
    }

  }

  return curviewpos;
}

void Draw_ParticleSys(ParticleSys_t *psys, struct List3DView_s *l3dview, enum32 forceflags, enum32 ignoreflags)
{ // this func draws a particle system
  // first we compute viewpos of particles and add them to drawlist
  // if there is blending, we will sort the list for z
  // then we push to draw it stuff is dfferent for modeltype which we ignore atm
  float     *viewposarray;
  float     *curviewpos;
  ParticleLife_t *life = &psys->life;
  int i,maxparticles,numtex;

  if ((psys->container.numAParticles > 0 || psys->psysflag & PARTICLE_COMBDRAW) && !(psys->psysflag & PARTICLE_NODRAW) && (g_LuxTimer.time > psys->container.drawtime || l3dview != psys->container.drawl3dview) && isFlagSet(psys->container.visflag,g_CamLight.camera->bitID))
  {
    // setup basics
    l_PrtBuffer.cont = &psys->container;
    l_PrtBuffer.rot = LUX_FALSE;

    l_PrtBuffer.numCont = (psys->psysflag & PARTICLE_COMBDRAW) ? psys->numSubsys + 1 : 1;
    l_PrtBuffer.colorset = (psys->psysflag & PARTICLE_COLORED) ? LUX_TRUE : LUX_FALSE;
    l_PrtBuffer.texset = (psys->psysflag & PARTICLE_TEXTURED) ? LUX_TRUE : LUX_FALSE;
    l_PrtBuffer.sort = (psys->psysflag & PARTICLE_SORT) ? LUX_TRUE : LUX_FALSE;

    l_PrtBuffer.usevbo = LUX_FALSE;
    l_PrtBuffer.usevprog = LUX_FALSE;
    l_PrtBuffer.combdraw = LUX_FALSE;
    l_PrtBuffer.normalsset = LUX_FALSE;

    l_PrtBuffer.renderflag = (l_PrtBuffer.cont->renderflag & (~ignoreflags)) | forceflags;

#ifdef LUX_PARTICLE_USEPOINTERITERATOR
    maxparticles = GLParticleBuffer_maxparticles(psys->container.particletype,psys->container.instmesh);
#else
    maxparticles = LUX_MIN(psys->container.numAParticles,GLParticleBuffer_maxparticles(psys->container.particletype,psys->container.instmesh));
#endif
    // sorting by z
    // if only alphatest we draw front to back
    // if blending we do back to front
    if (l_PrtBuffer.sort){
      if (psys->container.blend.blendmode){
        l_PrtBuffer.sort = 1;
      }
      if (psys->container.alpha.alphafunc && psys->container.blend.blendmode != VID_DECAL){
        l_PrtBuffer.sort = -1;
      }
    }

    // alloc particles from buffer
    viewposarray = rpoolmalloc(sizeof(lxVector3)*maxparticles);
    l_PrtBuffer.drawParticles = rpoolmalloc(sizeof(GLParticle_t)*maxparticles);
    l_PrtBuffer.drawSize = 0;
    LUX_PROFILING_OP_MAX(g_Profiling.perframe.scratch.renderpoolmax,rpoolgetinuse());

    curviewpos = Draw_ParticleSys_fillDrawPrts(psys,viewposarray,0);

    numtex = psys->container.numTex;

    if (psys->psysflag & PARTICLE_COMBDRAW){
      ParticleSubSys_t  *subsys;
      ParticleSys_t   *psysSub;

      subsys = psys->subsys;
      for( i = 0; i < psys->numSubsys; i++,subsys++){
        psysSub = ResData_getParticleSys(subsys->psysRID);
        if (psysSub->container.numAParticles){
          curviewpos = Draw_ParticleSys_fillDrawPrts(psysSub,curviewpos,(float)numtex);
          numtex += psys->container.numTex;
          // we reached maximum particles
          if (curviewpos >= (float*)l_PrtBuffer.drawParticles)
            break;
        }
      }
    }

    if (l_PrtBuffer.drawSize > 0){
      l_PrtBuffer.texwidth = 1/(float)lxNextPowerOf2(numtex);
      if (l_PrtBuffer.sort)
        GLParticles_process(GLPARTICLE_PROCESS_SORTDIST,0,l_PrtBuffer.drawSize,NULL);
      ParticleBufferDrawGL();
    }

    psys->container.drawtime = g_LuxTimer.time;
    psys->container.drawl3dview = l3dview;

    // reset buffer
    rpoolsetbegin(viewposarray);
  }

}

//////////////////////////////////////////////////////////////////////////
// ParticleCloud Preparation

void Draw_ParticleCloud(ParticleCloud_t *pcloud,struct List3DView_s *l3dview,enum32 forceflags, enum32 ignoreflags)
{
  ParticleStatic_t  *prt;
  ParticleStatic_t  **prtptr;
  GLParticle_t    *drawPrt;
  ParticleContainer_t *pcont;
  GLParticleProcess_t rot;
  int       *drawIndex;
  float     maxSqRange;
  int       inside;
  float     renderprobability;
  float     *viewposarray;
  float     *curviewpos;
  int maxparticles;

  // setup basics
  pcont = l_PrtBuffer.cont = &pcloud->container;
  l_PrtBuffer.rot = rot =  (pcloud->rotvel) ? GLPARTICLE_PROCESS_ROT_VEL : ((pcloud->rot) ? GLPARTICLE_PROCESS_ROT : GLPARTICLE_PROCESS_ROT_NONE);
  l_PrtBuffer.numCont = 1;
  l_PrtBuffer.colorset = (pcloud->colorset) ? LUX_TRUE : LUX_FALSE;
  l_PrtBuffer.texset = (pcont->numTex && pcont->texRID >= 0) ? LUX_TRUE : LUX_FALSE;
  l_PrtBuffer.texwidth = 1/(float)pcont->numTex;

  if (!GLEW_ARB_point_sprite && pcont->numTex && pcont->particletype == PARTICLE_TYPE_POINT)
    l_PrtBuffer.texset = LUX_FALSE;

  l_PrtBuffer.sort = (pcloud->sort) ? LUX_TRUE : LUX_FALSE;

  if (l_PrtBuffer.sort){
    if (pcont->blend.blendmode){
      l_PrtBuffer.sort = -1;
    }
    if (pcont->alpha.alphafunc && pcont->blend.blendmode != VID_DECAL){
      l_PrtBuffer.sort = 1;
    }
  }

  if (!l_PrtBuffer.colorset)
    pcont->renderflag |= RENDER_NOVERTEXCOLOR;
  else
    pcont->renderflag &= ~RENDER_NOVERTEXCOLOR;

  l_PrtBuffer.usevbo = LUX_FALSE;
  l_PrtBuffer.usevprog = LUX_FALSE;
  l_PrtBuffer.combdraw = LUX_FALSE;
  l_PrtBuffer.normalsset = LUX_FALSE;

  l_PrtBuffer.renderflag = (l_PrtBuffer.cont->renderflag & (~ignoreflags)) | forceflags;


  maxparticles = GLParticleBuffer_maxparticles(pcont->particletype,pcont->instmesh);

  // lets try something different for iteration here
  prtptr = pcloud->container.activeStaticCur;
  drawIndex = l_PrtBuffer.drawIndices;

  // alloc particles from buffer
  viewposarray = rpoolmalloc(sizeof(lxVector3)*pcloud->container.numAParticles);
  drawPrt = l_PrtBuffer.drawParticles = rpoolmalloc(sizeof(GLParticle_t)*maxparticles);
  l_PrtBuffer.drawSize = 0;
  LUX_PROFILING_OP_MAX(g_Profiling.perframe.scratch.renderpoolmax,rpoolgetinuse());

  // transform to viewspace and perform user clipping
  curviewpos = viewposarray;
  switch(pcont->check)
  {
  case 1: //x
    while (prt=*prtptr){
      // set Z to positive so it wont be drawn
      if (PARTICLE_CHECKAXIS(0) )
        curviewpos[2] = 1.0f;
      else
        lxVector3Transform(curviewpos,prt->viewpos,g_VID.drawsetup.viewMatrix);
      curviewpos+=3;
      prtptr++;
    }
    break;
  case 2: //y
    while (prt=*prtptr){
      // set Z to positive so it wont be drawn
      if (PARTICLE_CHECKAXIS(1) )
        curviewpos[2] = 1.0f;
      else
        lxVector3Transform(curviewpos,prt->viewpos,g_VID.drawsetup.viewMatrix);
      curviewpos+=3;
      prtptr++;
    }
    break;
  case 3: //xy
    while (prt=*prtptr){
      // set Z to positive so it wont be drawn
      if (PARTICLE_CHECKAXIS(0) || PARTICLE_CHECKAXIS(1))
        curviewpos[2] = 1.0f;
      else
        lxVector3Transform(curviewpos,prt->viewpos,g_VID.drawsetup.viewMatrix);
      curviewpos+=3;
      prtptr++;
    }
    break;
  case 4: //z
    while (prt=*prtptr){
      // set Z to positive so it wont be drawn
      if (PARTICLE_CHECKAXIS(2))
        curviewpos[2] = 1.0f;
      else
        lxVector3Transform(curviewpos,prt->viewpos,g_VID.drawsetup.viewMatrix);
      curviewpos+=3;
      prtptr++;
    }
    break;
  case 5: //zx
    while (prt=*prtptr){
      // set Z to positive so it wont be drawn
      if (PARTICLE_CHECKAXIS(0) || PARTICLE_CHECKAXIS(2))
        curviewpos[2] = 1.0f;
      else
        lxVector3Transform(curviewpos,prt->viewpos,g_VID.drawsetup.viewMatrix);
      curviewpos+=3;
      prtptr++;
    }
    break;
  case 6: //zy
    while (prt=*prtptr){
      // set Z to positive so it wont be drawn
      if (PARTICLE_CHECKAXIS(1) || PARTICLE_CHECKAXIS(2))
        curviewpos[2] = 1.0f;
      else
        lxVector3Transform(curviewpos,prt->viewpos,g_VID.drawsetup.viewMatrix);
      curviewpos+=3;
      prtptr++;
    }
    break;
  case 7: //xyz
    while (prt=*prtptr){
      // set Z to positive so it wont be drawn
      if (PARTICLE_CHECKAXIS(1) || PARTICLE_CHECKAXIS(2) || PARTICLE_CHECKAXIS(3))
        curviewpos[2] = 1.0f;
      else
        lxVector3Transform(curviewpos,prt->viewpos,g_VID.drawsetup.viewMatrix);
      curviewpos+=3;
      prtptr++;
    }
    break;
  default:
    while (prt=*prtptr){
      lxVector3Transform(curviewpos,prt->viewpos,g_VID.drawsetup.viewMatrix);
      curviewpos+=3;
      prtptr++;
    }
  }

  // process ranging
  if (pcloud->range){
    maxSqRange = pcloud->range*pcloud->range;
    inside = pcloud->insiderange;

    curviewpos = viewposarray;
    prtptr = pcont->activeStaticCur;
    while (prt=*prtptr){
      if (LUX_FP_LESS_ZERO(curviewpos[2]) &&
        ((inside && lxVector3SqLength(curviewpos) > maxSqRange) ||
        !inside && lxVector3SqLength(curviewpos) < maxSqRange))
        curviewpos[2] = 1.0f;
      curviewpos+=3;
      prtptr++;
    }
  }

  // process probability
  if (pcont->contflag & PARTICLE_PROBABILITY){
    renderprobability = pcont->userProbability;

    curviewpos = viewposarray;
    prtptr = pcont->activeStaticCur;
    while (prt=*prtptr){
      // set Z to positive so it wont be drawn
      curviewpos[2] = (LUX_FP_LESS_ZERO(curviewpos[2]) && frandPointer(prt) < renderprobability) ? curviewpos[2] : 1.0f;
      curviewpos+=3;
      prtptr++;
    }
  }

  // process all active and ignore those outside of view
  curviewpos = viewposarray;
  prtptr = pcont->activeStaticCur;
  for (; (prt=*prtptr) && l_PrtBuffer.drawSize < maxparticles; prtptr++){
    if (curviewpos[2] > 0 || curviewpos[2] < -g_CamLight.camera->backplane){
      curviewpos+=3;
      continue;
    }

    drawPrt->prtSt = prt;
    drawPrt->viewpos = curviewpos;

    drawPrt->colorc = prt->colorc;
    drawPrt->normal = prt->normal;
    drawPrt->rot = prt->rot;
    drawPrt->size = prt->size;
    drawPrt->texoffset = (float)prt->texid;
    drawPrt->relage = 0.0f;


    *drawIndex = l_PrtBuffer.drawSize;

    l_PrtBuffer.drawSize++;
    drawIndex++;
    drawPrt++;
    curviewpos+=3;
  }


  if (l_PrtBuffer.sort)
    GLParticles_process(GLPARTICLE_PROCESS_SORTDIST,0,l_PrtBuffer.drawSize,NULL);

  GLParticles_process(rot,0,l_PrtBuffer.drawSize,NULL);

  if (!pcloud->colorset){
    GLParticles_process(GLPARTICLE_PROCESS_COLOR_NONE,0,l_PrtBuffer.drawSize,pcont);
  }
  else if (pcont->contflag & PARTICLE_COLORMUL){
    GLParticles_process(GLPARTICLE_PROCESS_COLOR_USERMUL,0,l_PrtBuffer.drawSize,pcont);
  }
  if (pcont->contflag & PARTICLE_SIZEMUL){
    GLParticles_process(GLPARTICLE_PROCESS_SIZE_USERMUL,0,l_PrtBuffer.drawSize,pcont);
  }
  if (pcont->contflag & PARTICLE_PROBABILITY && pcont->userProbabilityFadeOut){
    GLParticles_process(GLPARTICLE_PROCESS_FADEOUT,0,l_PrtBuffer.drawSize,pcont);
  }

  ParticleBufferDrawGL();

  // reset buffer
  rpoolsetbegin(viewposarray);
}
