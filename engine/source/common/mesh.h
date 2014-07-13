// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __MESH_H__
#define __MESH_H__

#include "../common/common.h"
#include "../common/vidbuffer.h"
#include <luxinia/meshvertex.h>

typedef enum VertexValue_e{
  VERTEX_SIZE,
  VERTEX_START,
  VERTEX_TEX0,
  VERTEX_TEX1,
  VERTEX_TEX2,
  VERTEX_TEX3,
  VERTEX_COLOR,
  VERTEX_POS,
  VERTEX_NORMAL,
  VERTEX_TANGENT,
  VERTEX_BLENDI,
  VERTEX_BLENDW,

  VERTEX_MAXTEX,
  VERTEX_TEXFORMAT,
  VERTEX_TEXNATIVE,
  VERTEX_COLORFORMAT,
  VERTEX_COLORNATIVE,
  VERTEX_MAXPOS,
  VERTEX_POSFORMAT,
  VERTEX_POSNATIVE,
  VERTEX_NORMALFORMAT,
  VERTEX_NORMALNATIVE,
  VERTEX_TANGENTFORMAT,
  VERTEX_TANGENTNATIVE,


  VERTEX_SCALARPOS,
  VERTEX_SCALARTEX,
  VERTEX_SCALARNORMAL,
  VERTEX_SCALARTANGENT,
  VERTEX_SCALARCOLOR,
  VERTEX_SCALARBLENDI,
  VERTEX_SCALARBLENDW,

  VERTEX_LASTVALUE
}VertexValue_t;

typedef enum VID_TexChannel_e{
  VID_TEXCOORD_NOSET = -1,
  VID_TEXCOORD_0 = 0,
  VID_TEXCOORD_1 = 1,
  VID_TEXCOORD_LIGHTMAP = 1,
  VID_TEXCOORD_2 = 2,
  VID_TEXCOORD_3 = 3,
  VID_TEXCOORD_01 = 4,
  VID_TEXCOORD_23 = 5,
  VID_TEXCOORD_CHANNELS,
}VID_TexChannel_t;

// manual texcoords
typedef enum  VID_Texpush_e{
  VID_TEXPUSH_SCREEN  = 1,
  VID_TEXPUSH_SPHERE  ,
  VID_TEXPUSH_REFLECT ,
  VID_TEXPUSH_MATRIX  ,
  VID_TEXPUSH_EYELIN  ,
  VID_TEXPUSH_NORMAL  ,
}VID_Texpush_t;

typedef enum MeshType_e{
  MESH_UNSET,
  MESH_DL,    // display list
  MESH_VA,    // vertex array
  MESH_VBO,   // vertex buffer object, may not have index array
  MESH_VBO_SPLIT,
}MeshType_t;

typedef enum InstanceType_e{
  INST_NONE,              // no instance
  INST_SHRD_VERT,           // shared vertices
  INST_SHRD_VERT_PARENT_MAT,      // shared vertices and hierarchical position
  INST_SHRD_VERT_TRIS         // shared vertices & tris but own position
}InstanceType_t;

typedef enum PrimitiveType_e{
  PRIM_POINTS,
  PRIM_LINES,
  PRIM_LINE_LOOP,
  PRIM_LINE_STRIP,
  PRIM_TRIANGLES,
  PRIM_TRIANGLE_STRIP,
  PRIM_TRIANGLE_FAN,
  PRIM_QUADS,
  PRIM_QUAD_STRIP,
  PRIM_POLYGON,
}PrimitiveType_t;

typedef struct MeshGL_s{
  VIDBuffer_t   *vbo;
  VIDBuffer_t   *ibo;
  uint      vbooffset;
  uint      ibooffset;
  uint      glID;
}MeshGL_t;

typedef struct Mesh_s{
  MeshType_t    meshtype;
  int       nolock;
  lxVertexType_t  vertextype;
  InstanceType_t  instanceType;

  struct Mesh_s *instance;

  MeshGL_t    vid;

  PrimitiveType_t primtype;     // rendertype TRIANGLES, TRIANGLE_STRIP

  int     numIndices;     // number of primitive indices
  int     numAllocIndices;

  int     numGroups;      // groups
  int     numTris;

  uint    indexMin;
  uint    indexMax;

  int     *indicesGroupLength;  // length of each group
  booln   index16;
  union{
    void    *indicesData;
    ushort    *indicesData16;     // indices for triangles
    uint    *indicesData32;
  };


  int     numVertices;      // number of vertices
  int     numAllocVertices;
  union {
    void        *vertexData;  // vertex data
    lxVertex64_t      *vertexData64;
    lxVertex32_t      *vertexData32;
    lxVertex16_t      *vertexData16;

    lxVertex32FN_t    *vertexData32FN;
  };

  void    *origVertexData;      // sometimes changes might occur on rendering

}Mesh_t;

extern const int32      g_VertexSizes[VERTEX_LASTTYPE][VERTEX_LASTVALUE];


//////////////////////////////////////////////////////////////////////////
// Vertex

#define VertexArrayPtr(array,index,vertextype,vertexvalue)      (void*)((char*)array+(g_VertexSizes[vertextype][VERTEX_SIZE]*index) + g_VertexSizes[vertextype][vertexvalue])
#define VertexSize(vertextype)                    g_VertexSizes[vertextype][VERTEX_SIZE]
#define VertexIncrPtr(vertextype,ptrtype,ptr)           ptr = (ptrtype*)((char*)ptr+g_VertexSizes[vertextype][VERTEX_SIZE])
#define VertexValue(vertextype,val)                 g_VertexSizes[vertextype][val]

//////////////////////////////////////////////////////////////////////////
// Mesh

Mesh_t* Mesh_new(Mesh_t* into,int vertexcount, int indexcount, lxVertexType_t vtype);

// ibo is optional
Mesh_t* Mesh_newVBO(Mesh_t* into,int vertexcount, int indexcount, lxVertexType_t vtype,
          VIDBuffer_t *vbo, int vbooffset,
          VIDBuffer_t *ibo, int ibooffset);

void Mesh_clear(Mesh_t *mesh);
void Mesh_free(Mesh_t *mesh);

// vbo = 0, ibo = 1
void Mesh_submitVBO(const Mesh_t *mesh, booln isindex, int from, int cnt);
void Mesh_retrieveVBO(Mesh_t *mesh, booln isindex, int from, int cnt);

booln Mesh_typeVA(Mesh_t *mesh);
booln Mesh_typeDL(Mesh_t *mesh, int texcoords,booln novcolor, booln nonormals, booln needtangents);
booln Mesh_typeVBO(Mesh_t *mesh, VIDBuffer_t *vbo, int vbooffset, VIDBuffer_t *ibo, int ibooffset);
booln Mesh_setPtrsAllocState(Mesh_t *mesh, booln vertex, booln index);
void Mesh_getPtrsAllocState(Mesh_t *mesh, booln* vertex, booln* index);

void Mesh_setStandardAttribsGL(const Mesh_t *mesh, booln color, booln normals);
void Mesh_setBlendGL(const Mesh_t *mesh);
void Mesh_setTangentGL(const Mesh_t *mesh);
void Mesh_setTexCoordGL(const Mesh_t *mesh, uint coord, const VID_TexChannel_t channel);
void VertexAttrib_setCustomStreamGL(VertexAttrib_t attr, VIDBuffer_t *buffer, size_t stride, void *ptr,
                  VertexCustomAttrib_t cattr);
void Mesh_pushTexCoordGL(const Mesh_t *mesh, const VID_Texpush_t mode, void *data);

void Mesh_updateMinMax(Mesh_t *mesh);
void Mesh_updateTrisCnt(Mesh_t *mesh);
int  Mesh_getPrimCnt(const Mesh_t *mesh);

Mesh_t  *Mesh_newQuad(int flip);
Mesh_t  *Mesh_newQuadCentered(int flip);
Mesh_t  *Mesh_newBuffer();
Mesh_t  *Mesh_newBox();
Mesh_t  *Mesh_newSphere();
Mesh_t  *Mesh_newCylinder();

#endif
