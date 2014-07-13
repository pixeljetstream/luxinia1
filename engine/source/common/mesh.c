// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "mesh.h"
#include "vid.h"
#include "3dmath.h"

#include <luxinia/luxcore/scalarmisc.h>

/*
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

*/

// this array must be in synch with definitions found in the header !!
const int32 g_VertexSizes[VERTEX_LASTTYPE][VERTEX_LASTVALUE]={
    // Vertex32 Lightmap
  { 32,0,0,8,-1,-1,28,16,16,-1,-1,-1,
    2,VertexCustomAttrib_new(2,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    VertexCustomAttrib_new(4,LUX_SCALAR_UINT8,LUX_TRUE,LUX_FALSE),GL_UNSIGNED_BYTE,
    3,VertexCustomAttrib_new(3,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    -1,-1,
    -1,-1,
    LUX_SCALAR_FLOAT32, LUX_SCALAR_FLOAT32, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL, LUX_SCALAR_UINT8, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL},
    // Vertex32 Lit
  { 32,0,0,-1,-1,-1,28,16,8,-1,-1,-1,
    1,VertexCustomAttrib_new(2,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    VertexCustomAttrib_new(4,LUX_SCALAR_UINT8,LUX_TRUE,LUX_FALSE),GL_UNSIGNED_BYTE,
    3,VertexCustomAttrib_new(3,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    VertexCustomAttrib_new(3,LUX_SCALAR_INT16,LUX_TRUE,LUX_FALSE),GL_SHORT,
    -1,-1,
    LUX_SCALAR_FLOAT32, LUX_SCALAR_FLOAT32, LUX_SCALAR_INT16, LUX_SCALAR_ILLEGAL, LUX_SCALAR_UINT8, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL},
    // Vertex32FN
  { 32,0,24,24,-1,-1,-1,0,12,-1,-1,-1,
    1,VertexCustomAttrib_new(2,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    -1,-1,
    3,VertexCustomAttrib_new(3,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    VertexCustomAttrib_new(3,LUX_SCALAR_INT16,LUX_TRUE,LUX_FALSE),GL_FLOAT,
    -1,-1,
    LUX_SCALAR_FLOAT32, LUX_SCALAR_FLOAT32, LUX_SCALAR_FLOAT32, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL},
    // Vertex32 Terrain
  { 32,0,0,-1,-1,-1,28,16,0,8,-1,-1,
    1,VertexCustomAttrib_new(2,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    VertexCustomAttrib_new(4,LUX_SCALAR_UINT8,LUX_TRUE,LUX_FALSE),GL_UNSIGNED_BYTE,
    3,VertexCustomAttrib_new(3,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    VertexCustomAttrib_new(3,LUX_SCALAR_INT16,LUX_TRUE,LUX_FALSE),GL_SHORT,
    VertexCustomAttrib_new(3,LUX_SCALAR_INT16,LUX_TRUE,LUX_FALSE),GL_SHORT,
    LUX_SCALAR_FLOAT32, LUX_SCALAR_ILLEGAL, LUX_SCALAR_INT16, LUX_SCALAR_INT16, LUX_SCALAR_UINT8, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL},
    // Vertex64 Tex4
  { 64,0,0,8,16,24,60,48,32,40,-1,-1,
    4,VertexCustomAttrib_new(2,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    VertexCustomAttrib_new(4,LUX_SCALAR_UINT8,LUX_TRUE,LUX_FALSE),GL_UNSIGNED_BYTE,
    3,VertexCustomAttrib_new(3,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    VertexCustomAttrib_new(3,LUX_SCALAR_INT16,LUX_TRUE,LUX_FALSE),GL_SHORT,
    VertexCustomAttrib_new(4,LUX_SCALAR_INT16,LUX_TRUE,LUX_FALSE),GL_SHORT,
    LUX_SCALAR_FLOAT32, LUX_SCALAR_FLOAT32, LUX_SCALAR_INT16, LUX_SCALAR_INT16, LUX_SCALAR_UINT8, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL},
    // Vertex64 Skin
  { 64,0,0,8,16,24,60,48,32,40,20,24,
    2,VertexCustomAttrib_new(2,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    VertexCustomAttrib_new(4,LUX_SCALAR_UINT8,LUX_TRUE,LUX_FALSE),GL_UNSIGNED_BYTE,
    3,VertexCustomAttrib_new(3,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    VertexCustomAttrib_new(3,LUX_SCALAR_INT16,LUX_TRUE,LUX_FALSE),GL_SHORT,
    VertexCustomAttrib_new(4,LUX_SCALAR_INT16,LUX_TRUE,LUX_FALSE),GL_SHORT,
    LUX_SCALAR_FLOAT32, LUX_SCALAR_FLOAT32, LUX_SCALAR_INT16, LUX_SCALAR_INT16, LUX_SCALAR_UINT8, LUX_SCALAR_UINT8, LUX_SCALAR_UINT16},
    // Vertex16 Homogenous
  { 16,0,-1,-1,-1,-1,-1,0,-1,-1,-1,-1,
    0,-1,-1,
    -1,-1,
    4,VertexCustomAttrib_new(4,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    -1,-1,
    -1,-1,
    LUX_SCALAR_FLOAT32, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL},
    // Vertex16 Color
  { 16,0,-1,-1,-1,-1,12,0,-1,-1,-1,-1,
    0,-1,-1,
    VertexCustomAttrib_new(4,LUX_SCALAR_UINT8,LUX_TRUE,LUX_FALSE),GL_UNSIGNED_BYTE,
    3,VertexCustomAttrib_new(3,LUX_SCALAR_FLOAT32,LUX_FALSE,LUX_FALSE),GL_FLOAT,
    -1,-1,
    -1,-1,
    LUX_SCALAR_FLOAT32, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL, LUX_SCALAR_UINT8, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL},
    // Vertex16 Terrain
  { 16,0,0,-1,-1,-1,-1,8,4,0,-1,-1,
    1,VertexCustomAttrib_new(2,LUX_SCALAR_INT16,LUX_FALSE,LUX_FALSE),GL_SHORT,
    -1,-1,
    3,VertexCustomAttrib_new(3,LUX_SCALAR_INT16,LUX_FALSE,LUX_FALSE),GL_SHORT,
    VertexCustomAttrib_new(4,LUX_SCALAR_UINT8,LUX_TRUE,LUX_FALSE),GL_UNSIGNED_BYTE,
    VertexCustomAttrib_new(4,LUX_SCALAR_UINT8,LUX_TRUE,LUX_FALSE),GL_UNSIGNED_BYTE,
    LUX_SCALAR_INT16, LUX_SCALAR_ILLEGAL, LUX_SCALAR_UINT8, LUX_SCALAR_UINT8, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL, LUX_SCALAR_ILLEGAL},
};

///////////////////////////////////////////////////////////////////////////////
// Mesh

Mesh_t* Mesh_newUnset(Mesh_t* into,int vertexcount, int indexcount, lxVertexType_t vtype)
{
  Mesh_t *mesh = into ? into : ((Mesh_t *)lxMemGenZalloc(sizeof(Mesh_t)));

  mesh->numIndices = 0;
  mesh->numAllocIndices = indexcount;
  mesh->numVertices = 0;
  mesh->numAllocVertices = vertexcount;
  mesh->vertextype = vtype;
  mesh->primtype = PRIM_TRIANGLES;
  mesh->index16 = vertexcount <= BIT_ID_FULL16;

  return mesh;
}

Mesh_t* Mesh_new(Mesh_t* into,int vertexcount, int indexcount, lxVertexType_t vtype)
{
  Mesh_t* mesh = Mesh_newUnset(into,vertexcount,indexcount,vtype);

  Mesh_setPtrsAllocState(mesh,LUX_TRUE,LUX_TRUE);
  mesh->meshtype = MESH_VA;

  return mesh;
}

Mesh_t* Mesh_newVBO(Mesh_t* into,int vertexcount, int indexcount, lxVertexType_t vtype, VIDBuffer_t *vbo, int vbooffset, VIDBuffer_t *ibo,  int ibooffset)
{
  Mesh_t* mesh = Mesh_newUnset(into,vertexcount,indexcount,vtype);

  if (!Mesh_typeVBO(mesh,vbo,vbooffset,ibo,ibooffset)){
    Mesh_free(mesh);
    return NULL;
  }

  Mesh_setPtrsAllocState(mesh,LUX_FALSE,ibo == NULL);

  return mesh;
}

Mesh_t  *Mesh_newBuffer()
{
  Mesh_t *mesh = (Mesh_t*)lxMemGenZalloc(sizeof(Mesh_t));

  mesh->numIndices = 0;
  mesh->numAllocIndices = VID_MAX_INDICES;
  mesh->indicesData16 = (ushort*)lxMemGenZalloc(sizeof(ushort)*mesh->numAllocIndices );
  mesh->index16 = LUX_TRUE;
  mesh->numVertices = 0;
  mesh->numAllocVertices = VID_MAX_VERTICES;
  mesh->origVertexData = mesh->vertexData = (lxVertex64_t*)lxMemGenZallocAligned(sizeof(lxVertex64_t)*mesh->numAllocVertices,32);
  mesh->meshtype = MESH_VA;
  mesh->vertextype = VERTEX_64_TEX4;
  mesh->primtype = PRIM_TRIANGLES;

  return mesh;
}

Mesh_t  *Mesh_newBox()
{
  Mesh_t *mesh = (Mesh_t*)lxMemGenZalloc(sizeof(Mesh_t));
  lxVertex64_t *vert;
  ushort  *index;
  short norm[3];
  int i;



  mesh->numIndices = 24;
  mesh->numVertices = 24;
  mesh->indexMin = 0;
  mesh->indexMax = 24-1;
  index = mesh->indicesData16 = (ushort*)lxMemGenZalloc(sizeof(ushort)*mesh->numIndices);
  mesh->origVertexData = mesh->vertexData = vert = (lxVertex64_t*)lxMemGenZallocAligned(sizeof(lxVertex64_t)*mesh->numVertices,32);
  mesh->meshtype = MESH_VA;
  mesh->vertextype = VERTEX_64_TEX4;
  mesh->primtype = PRIM_QUADS;
  mesh->numTris = 12;
  mesh->index16 = LUX_TRUE;

  for (i = 0; i < 24; i++,index++){
    *index = i;
  }

#define H 0.5f

#define P0  lxVector3Set(vert->pos,-H, -H, -H)
#define P1  lxVector3Set(vert->pos, H, -H, -H)
#define P2  lxVector3Set(vert->pos, H, -H,  H)
#define P3  lxVector3Set(vert->pos,-H, -H,  H)
#define P4  lxVector3Set(vert->pos, H,  H, -H)
#define P5  lxVector3Set(vert->pos, H,  H,  H)
#define P6  lxVector3Set(vert->pos,-H,  H, -H)
#define P7  lxVector3Set(vert->pos,-H,  H,  H)

#define VERTQ0  vert->colorc = BIT_ID_FULL32; lxVector2Set(vert->tex,0,0);  LUX_ARRAY3COPY(vert->normal,norm); vert++
#define VERTQ1  vert->colorc = BIT_ID_FULL32; lxVector2Set(vert->tex,1,0);  LUX_ARRAY3COPY(vert->normal,norm);  vert++
#define VERTQ2  vert->colorc = BIT_ID_FULL32; lxVector2Set(vert->tex,1,1);  LUX_ARRAY3COPY(vert->normal,norm);  vert++
#define VERTQ3  vert->colorc = BIT_ID_FULL32; lxVector2Set(vert->tex,0,1);  LUX_ARRAY3COPY(vert->normal,norm);  vert++


  // front
  LUX_ARRAY3SET(norm,0,-LUX_SHORT_SIGNEDMAX,0);
  // 0
  P0; VERTQ0;
  // 1
  P1; VERTQ1;
  // 2
  P2; VERTQ2;
  // 3
  P3; VERTQ3;

  // right
  LUX_ARRAY3SET(norm,LUX_SHORT_SIGNEDMAX,0,0);
  // 0
  P1; VERTQ0;
  // 1
  P4; VERTQ1;
  // 2
  P5; VERTQ2;
  // 3
  P2; VERTQ3;

  // back
  LUX_ARRAY3SET(norm,0,LUX_SHORT_SIGNEDMAX,0);
  // 0
  P4; VERTQ0;
  // 1
  P6; VERTQ1;
  // 2
  P7; VERTQ2;
  // 3
  P5; VERTQ3;

  // left
  LUX_ARRAY3SET(norm,-LUX_SHORT_SIGNEDMAX,0,0);
  // 0
  P6; VERTQ0;
  // 1
  P0; VERTQ1;
  // 2
  P3; VERTQ2;
  // 3
  P7; VERTQ3;

  // top
  LUX_ARRAY3SET(norm,0,0,LUX_SHORT_SIGNEDMAX);
  // 0
  P3; VERTQ0;
  // 1
  P2; VERTQ1;
  // 2
  P5; VERTQ2;
  // 3
  P7; VERTQ3;

  // bot
  LUX_ARRAY3SET(norm,0,0,-LUX_SHORT_SIGNEDMAX);
  // 0
  P6; VERTQ0;
  // 1
  P4; VERTQ1;
  // 2
  P1; VERTQ2;
  // 3
  P0; VERTQ3;

#undef H
#undef VERTQ0
#undef VERTQ1
#undef VERTQ2
#undef VERTQ3
#undef P0
#undef P1
#undef P2
#undef P3
#undef P4
#undef P5
#undef P6
#undef P7


  mesh->numAllocIndices = mesh->numIndices;
  mesh->numAllocVertices = mesh->numVertices;

  return mesh;
}

Mesh_t  *Mesh_newQuadCentered(int flip)
{
  Mesh_t *mesh = (Mesh_t*)lxMemGenZalloc(sizeof(Mesh_t));
  lxVertex64_t *vert;

  mesh->numIndices = 4;
  mesh->indicesData16 = (ushort*)lxMemGenZalloc(sizeof(ushort)*mesh->numIndices);
  mesh->numVertices = 4;
  mesh->indexMin = 0;
  mesh->indexMax = 3;
  mesh->origVertexData = mesh->vertexData = lxMemGenZallocAligned(sizeof(lxVertex64_t)*mesh->numVertices,32);
  mesh->meshtype = MESH_VA;
  mesh->vertextype = VERTEX_64_TEX4;
  mesh->primtype = PRIM_QUADS;
  mesh->index16 = LUX_TRUE;
  mesh->numTris = 2;

  mesh->indicesData16[0]=3;
  mesh->indicesData16[1]=2;
  mesh->indicesData16[2]=1;
  mesh->indicesData16[3]=0;

  vert = mesh->vertexData64;

  LUX_ARRAY4SET(vert->color,255,255,255,255);
  lxVector3Set(vert->pos,-0.5f,0.5f,0);
  LUX_ARRAY3SET(vert->normal,0,0,LUX_SHORT_SIGNEDMAX);
  lxVector2Set(vert->tex,0,flip ? 1 : 0);
  vert++;

  LUX_ARRAY4SET(vert->color,255,255,255,255);
  lxVector3Set(vert->pos,0.5f,0.5f,0);
  LUX_ARRAY3SET(vert->normal,0,0,LUX_SHORT_SIGNEDMAX);
  lxVector2Set(vert->tex,1,flip ? 1 : 0);
  vert++;

  LUX_ARRAY4SET(vert->color,255,255,255,255);
  lxVector3Set(vert->pos,0.5f,-0.5f,0);
  LUX_ARRAY3SET(vert->normal,0,0,LUX_SHORT_SIGNEDMAX);
  lxVector2Set(vert->tex,1,flip ? 0 : 1);
  vert++;

  LUX_ARRAY4SET(vert->color,255,255,255,255);
  lxVector3Set(vert->pos,-0.5f,-0.5f,0);
  LUX_ARRAY3SET(vert->normal,0,0,LUX_SHORT_SIGNEDMAX);
  lxVector2Set(vert->tex,0,flip ? 0 : 1);
  vert++;

  mesh->numAllocIndices = mesh->numIndices;
  mesh->numAllocVertices = mesh->numVertices;

  return mesh;
}

Mesh_t  *Mesh_newQuad(int flip)
{
  Mesh_t *mesh = (Mesh_t*)lxMemGenZalloc(sizeof(Mesh_t));
  lxVertex64_t *vert;

  mesh->numIndices = 4;
  mesh->indicesData16 = (ushort*)lxMemGenZalloc(sizeof(ushort)*mesh->numIndices);
  mesh->numVertices = 4;
  mesh->indexMin = 0;
  mesh->indexMax = 3;
  mesh->origVertexData = mesh->vertexData = lxMemGenZallocAligned(sizeof(lxVertex64_t)*mesh->numVertices,32);
  mesh->meshtype = MESH_VA;
  mesh->vertextype = VERTEX_64_TEX4;
  mesh->primtype = PRIM_QUADS;
  mesh->numTris = 2;

  mesh->indicesData16[0]=3;
  mesh->indicesData16[1]=2;
  mesh->indicesData16[2]=1;
  mesh->indicesData16[3]=0;
  mesh->index16 = LUX_TRUE;

  vert = mesh->vertexData64;

  LUX_ARRAY4SET(vert->color,255,255,255,255);
  lxVector3Set(vert->pos,0,1,0);
  LUX_ARRAY3SET(vert->normal,0,0,LUX_SHORT_SIGNEDMAX);
  lxVector2Set(vert->tex,0,flip ? 1 : 0);
  vert++;

  LUX_ARRAY4SET(vert->color,255,255,255,255);
  lxVector3Set(vert->pos,1,1,0);
  LUX_ARRAY3SET(vert->normal,0,0,LUX_SHORT_SIGNEDMAX);
  lxVector2Set(vert->tex,1,flip ? 1 : 0);
  vert++;

  LUX_ARRAY4SET(vert->color,255,255,255,255);
  lxVector3Set(vert->pos,1,0,0);
  LUX_ARRAY3SET(vert->normal,0,0,LUX_SHORT_SIGNEDMAX);
  lxVector2Set(vert->tex,1,flip ? 0 : 1);
  vert++;

  LUX_ARRAY4SET(vert->color,255,255,255,255);
  lxVector3Set(vert->pos,0,0,0);
  LUX_ARRAY3SET(vert->normal,0,0,LUX_SHORT_SIGNEDMAX);
  lxVector2Set(vert->tex,0,flip ? 0 : 1);
  vert++;

  mesh->numAllocIndices = mesh->numIndices;
  mesh->numAllocVertices = mesh->numVertices;

  return mesh;
}

Mesh_t  *Mesh_newSphere()
{
  int pointcount = 118;
  float points[] = {
    0.000000, -1.000000, 0.000000,  0.000000, -1.000000, 0.000000,  0.000000, -1.000000, 0.000000,
      0.000000, -1.000000, 0.000000,  0.000000, -1.000000, 0.000000,  0.000000, -1.000000, 0.000000,
      0.000000, -1.000000, 0.000000,  0.000000, -1.000000, 0.000000,  0.000000, -1.000000, 0.000000,
      0.000000, -1.000000, 0.000000,  0.000000, -1.000000, 0.000000,  0.000000, -1.000000, 0.000000,
      0.000000, -1.000000, 0.000000,  0.000000, -1.000000, 0.000000,  0.000000, 1.000000, 0.000000,
      0.000000, 1.000000, 0.000000,  0.000000, 1.000000, 0.000000,  0.000000, 1.000000, 0.000000,
      0.000000, 1.000000, 0.000000,  0.000000, 1.000000, 0.000000,  0.000000, 1.000000, 0.000000,
      0.000000, 1.000000, 0.000000,  0.000000, 1.000000, 0.000000,  0.000000, 1.000000, 0.000000,
      0.000000, 1.000000, 0.000000,  0.000000, 1.000000, 0.000000,  0.000000, 1.000000, 0.000000,
      0.000000, 1.000000, 0.000000,  0.433884, -0.900969, 0.000000,  0.433884, -0.900969, 0.000000,
      0.390916, -0.900969, 0.188255,  0.270522, -0.900969, 0.339224,  0.096548, -0.900969, 0.423005,
      -0.096548, -0.900969, 0.423005,  -0.270522, -0.900969, 0.339224,  -0.390916, -0.900969, 0.188255,
      -0.433884, -0.900969, 0.000000,  -0.390916, -0.900969, -0.188255,  -0.270522, -0.900969, -0.339224,
      -0.096548, -0.900969, -0.423005,  0.096548, -0.900969, -0.423005,  0.270522, -0.900969, -0.339224,
      0.390916, -0.900969, -0.188255,  0.781832, -0.623490, 0.000000,  0.781832, -0.623490, 0.000000,
      0.704406, -0.623490, 0.339224,  0.487464, -0.623490, 0.611260,  0.173974, -0.623490, 0.762229,
      -0.173974, -0.623490, 0.762229,  -0.487464, -0.623490, 0.611260,  -0.704406, -0.623490, 0.339224,
      -0.781832, -0.623490, 0.000000,  -0.704406, -0.623490, -0.339224,  -0.487464, -0.623490, -0.611260,
      -0.173974, -0.623490, -0.762229,  0.173974, -0.623490, -0.762229,  0.487464, -0.623490, -0.611260,
      0.704406, -0.623490, -0.339224,  0.974928, -0.222521, 0.000000,  0.974928, -0.222521, 0.000000,
      0.878380, -0.222521, 0.423005,  0.607858, -0.222521, 0.762229,  0.216942, -0.222521, 0.950484,
      -0.216942, -0.222521, 0.950484,  -0.607858, -0.222521, 0.762229,  -0.878380, -0.222521, 0.423005,
      -0.974928, -0.222521, 0.000000,  -0.878380, -0.222521, -0.423005,  -0.607858, -0.222521, -0.762229,
      -0.216942, -0.222521, -0.950484,  0.216942, -0.222521, -0.950484,  0.607858, -0.222521, -0.762229,
      0.878380, -0.222521, -0.423005,  0.974928, 0.222521, 0.000000,  0.974928, 0.222521, 0.000000,
      0.878380, 0.222521, 0.423005,  0.607858, 0.222521, 0.762229,  0.216942, 0.222521, 0.950484,
      -0.216942, 0.222521, 0.950484,  -0.607858, 0.222521, 0.762229,  -0.878380, 0.222521, 0.423005,
      -0.974928, 0.222521, 0.000000,  -0.878380, 0.222521, -0.423005,  -0.607858, 0.222521, -0.762229,
      -0.216942, 0.222521, -0.950484,  0.216942, 0.222521, -0.950484,  0.607858, 0.222521, -0.762229,
      0.878380, 0.222521, -0.423005,  0.781832, 0.623490, 0.000000,  0.781832, 0.623490, 0.000000,
      0.704406, 0.623490, 0.339224,  0.487464, 0.623490, 0.611260,  0.173974, 0.623490, 0.762229,
      -0.173974, 0.623490, 0.762229,  -0.487464, 0.623490, 0.611260,  -0.704406, 0.623490, 0.339224,
      -0.781832, 0.623490, 0.000000,  -0.704406, 0.623490, -0.339224,  -0.487464, 0.623490, -0.611260,
      -0.173974, 0.623490, -0.762229,  0.173974, 0.623490, -0.762229,  0.487464, 0.623490, -0.611260,
      0.704406, 0.623490, -0.339224,  0.433884, 0.900969, 0.000000,  0.433884, 0.900969, 0.000000,
      0.390916, 0.900969, 0.188255,  0.270522, 0.900969, 0.339224,  0.096548, 0.900969, 0.423005,
      -0.096548, 0.900969, 0.423005,  -0.270522, 0.900969, 0.339224,  -0.390916, 0.900969, 0.188255,
      -0.433884, 0.900969, 0.000000,  -0.390916, 0.900969, -0.188255,  -0.270522, 0.900969, -0.339224,
      -0.096548, 0.900969, -0.423005,  0.096548, 0.900969, -0.423005,  0.270522, 0.900969, -0.339224,
      0.390916, 0.900969, -0.188255,  };
    float uvs[] = {
      0.928571, 1.000000, 0.000000, 1.000000, 0.071429, 1.000000, 0.142857, 1.000000,
        0.214286, 1.000000, 0.285714, 1.000000, 0.357143, 1.000000, 0.428571, 1.000000,
        0.500000, 1.000000, 0.571429, 1.000000, 0.642857, 1.000000, 0.714286, 1.000000,
        0.785714, 1.000000, 0.857143, 1.000000, 0.928571, 0.000000, 0.000000, 0.000000,
        0.071429, 0.000000, 0.142857, 0.000000, 0.214286, 0.000000, 0.285714, 0.000000,
        0.357143, 0.000000, 0.428571, 0.000000, 0.500000, 0.000000, 0.571429, 0.000000,
        0.642857, 0.000000, 0.714286, 0.000000, 0.785714, 0.000000, 0.857143, 0.000000,
        1.000000, 0.857143, 0.000000, 0.857143, 0.071429, 0.857143, 0.142857, 0.857143,
        0.214286, 0.857143, 0.285714, 0.857143, 0.357143, 0.857143, 0.428571, 0.857143,
        0.500000, 0.857143, 0.571429, 0.857143, 0.642857, 0.857143, 0.714286, 0.857143,
        0.785714, 0.857143, 0.857143, 0.857143, 0.928571, 0.857143, 1.000000, 0.714286,
        0.000000, 0.714286, 0.071429, 0.714286, 0.142857, 0.714286, 0.214286, 0.714286,
        0.285714, 0.714286, 0.357143, 0.714286, 0.428571, 0.714286, 0.500000, 0.714286,
        0.571429, 0.714286, 0.642857, 0.714286, 0.714286, 0.714286, 0.785714, 0.714286,
        0.857143, 0.714286, 0.928571, 0.714286, 1.000000, 0.571429, 0.000000, 0.571429,
        0.071429, 0.571429, 0.142857, 0.571429, 0.214286, 0.571429, 0.285714, 0.571429,
        0.357143, 0.571429, 0.428571, 0.571429, 0.500000, 0.571429, 0.571429, 0.571429,
        0.642857, 0.571429, 0.714286, 0.571429, 0.785714, 0.571429, 0.857143, 0.571429,
        0.928571, 0.571429, 1.000000, 0.428571, 0.000000, 0.428571, 0.071429, 0.428571,
        0.142857, 0.428571, 0.214286, 0.428571, 0.285714, 0.428571, 0.357143, 0.428571,
        0.428571, 0.428571, 0.500000, 0.428571, 0.571429, 0.428571, 0.642857, 0.428571,
        0.714286, 0.428571, 0.785714, 0.428571, 0.857143, 0.428571, 0.928571, 0.428571,
        1.000000, 0.285714, 0.000000, 0.285714, 0.071429, 0.285714, 0.142857, 0.285714,
        0.214286, 0.285714, 0.285714, 0.285714, 0.357143, 0.285714, 0.428571, 0.285714,
        0.500000, 0.285714, 0.571429, 0.285714, 0.642857, 0.285714, 0.714286, 0.285714,
        0.785714, 0.285714, 0.857143, 0.285714, 0.928571, 0.285714, 1.000000, 0.142857,
        0.000000, 0.142857, 0.071429, 0.142857, 0.142857, 0.142857, 0.214286, 0.142857,
        0.285714, 0.142857, 0.357143, 0.142857, 0.428571, 0.142857, 0.500000, 0.142857,
        0.571429, 0.142857, 0.642857, 0.142857, 0.714286, 0.142857, 0.785714, 0.142857,
        0.857143, 0.142857, 0.928571, 0.142857, };
      int indexcnt = 504;
      unsigned short indexarray[] = {
        29, 45, 44,  29, 30, 45,  30, 46, 45,  30, 31, 46,  31, 47, 46,  31, 32, 47,
          32, 48, 47,  32, 33, 48,  33, 49, 48,  33, 34, 49,  34, 50, 49,  34, 35, 50,
          35, 51, 50,  35, 36, 51,  36, 52, 51,  36, 37, 52,  37, 53, 52,  37, 38, 53,
          38, 54, 53,  38, 39, 54,  39, 55, 54,  39, 40, 55,  40, 56, 55,  40, 41, 56,
          41, 57, 56,  41, 42, 57,  42, 43, 57,  42, 28, 43,  44, 60, 59,  44, 45, 60,
          45, 61, 60,  45, 46, 61,  46, 62, 61,  46, 47, 62,  47, 63, 62,  47, 48, 63,
          48, 64, 63,  48, 49, 64,  49, 65, 64,  49, 50, 65,  50, 66, 65,  50, 51, 66,
          51, 67, 66,  51, 52, 67,  52, 68, 67,  52, 53, 68,  53, 69, 68,  53, 54, 69,
          54, 70, 69,  54, 55, 70,  55, 71, 70,  55, 56, 71,  56, 72, 71,  56, 57, 72,
          57, 58, 72,  57, 43, 58,  59, 75, 74,  59, 60, 75,  60, 76, 75,  60, 61, 76,
          61, 77, 76,  61, 62, 77,  62, 78, 77,  62, 63, 78,  63, 79, 78,  63, 64, 79,
          64, 80, 79,  64, 65, 80,  65, 81, 80,  65, 66, 81,  66, 82, 81,  66, 67, 82,
          67, 83, 82,  67, 68, 83,  68, 84, 83,  68, 69, 84,  69, 85, 84,  69, 70, 85,
          70, 86, 85,  70, 71, 86,  71, 87, 86,  71, 72, 87,  72, 73, 87,  72, 58, 73,
          74, 90, 89,  74, 75, 90,  75, 91, 90,  75, 76, 91,  76, 92, 91,  76, 77, 92,
          77, 93, 92,  77, 78, 93,  78, 94, 93,  78, 79, 94,  79, 95, 94,  79, 80, 95,
          80, 96, 95,  80, 81, 96,  81, 97, 96,  81, 82, 97,  82, 98, 97,  82, 83, 98,
          83, 99, 98,  83, 84, 99,  84, 100, 99,  84, 85, 100,  85, 101, 100,  85, 86, 101,
          86, 102, 101,  86, 87, 102,  87, 88, 102,  87, 73, 88,  89, 105, 104,  89, 90, 105,
          90, 106, 105,  90, 91, 106,  91, 107, 106, 91, 92, 107,  92, 108, 107,  92, 93, 108,
          93, 109, 108,  93, 94, 109,  94, 110, 109, 94, 95, 110,  95, 111, 110,  95, 96, 111,
          96, 112, 111,  96, 97, 112,  97, 113, 112, 97, 98, 113,  98, 114, 113,  98, 99, 114,
          99, 115, 114,  99, 100, 115,  100, 116, 115, 100, 101, 116,  101, 117, 116,  101, 102, 117,
          102, 103, 117,  102, 88, 103,  1, 30, 29, 105, 15, 104,  2, 31, 30,  106, 16, 105,
          3, 32, 31,  107, 17, 106,  4, 33, 32, 108, 18, 107,  5, 34, 33,  109, 19, 108,
          6, 35, 34,  110, 20, 109,  7, 36, 35, 111, 21, 110,  8, 37, 36,  112, 22, 111,
          9, 38, 37,  113, 23, 112,  10, 39, 38, 114, 24, 113,  11, 40, 39,  115, 25, 114,
          12, 41, 40,  116, 26, 115,  13, 42, 41, 117, 27, 116,  0, 28, 42,  103, 14, 117,
      };

  Mesh_t *mesh = (Mesh_t*)lxMemGenZalloc(sizeof(Mesh_t));
  lxVertex64_t *vert;
  int i;

  mesh->numIndices = indexcnt;
  mesh->indicesData16 = (ushort*)lxMemGenZalloc(sizeof(ushort)*mesh->numIndices);
  mesh->numVertices = pointcount;
  mesh->origVertexData = mesh->vertexData = lxMemGenZallocAligned(sizeof(lxVertex64_t)*mesh->numVertices,32);
  mesh->meshtype = MESH_VA;
  mesh->vertextype = VERTEX_64_TEX4;
  mesh->primtype = PRIM_TRIANGLES;
  mesh->indexMin = 0;
  mesh->indexMax = pointcount-1;
  mesh->numTris = indexcnt/3;
  mesh->index16 = LUX_TRUE;

  memcpy(mesh->indicesData16,indexarray,sizeof(ushort)*mesh->numIndices);

  vert = mesh->vertexData64;

  for (i=0;i<mesh->numVertices;i++,vert++) {
    LUX_ARRAY4SET(vert->color,255,255,255,255);
    lxVector3Copy(vert->pos,&points[i*3]);
    lxVector3Negated(vert->pos);
    lxVector3short_FROM_float(vert->normal,&points[i*3]);
    vert->normal[0] = -vert->normal[0];
    vert->normal[1] = -vert->normal[1];
    vert->normal[2] = -vert->normal[2];
    lxVector2Copy(vert->tex,&uvs[i*2]);
  }

  mesh->numAllocIndices = mesh->numIndices;
  mesh->numAllocVertices = mesh->numVertices;

  return mesh;
}

Mesh_t  *Mesh_newCylinder()
{
  int pointcount = 100;
  float points[] = {
    0.000000, 0.000000, 0.500000,  0.000000, -0.000000, -0.500000,  1.000000, 0.000000, 0.500000,
      1.000000, 0.000000, 0.500000,  1.000000, 0.000000, 0.500000,  1.000000, -0.000000, -0.500000,
      1.000000, -0.000000, -0.500000,  1.000000, -0.000000, -0.500000,  0.965926, 0.258819, 0.500000,
      0.965926, 0.258819, 0.500000,  0.965926, 0.258819, -0.500000,  0.965926, 0.258819, -0.500000,
      0.866025, 0.500000, 0.500000,  0.866025, 0.500000, 0.500000,  0.866025, 0.500000, -0.500000,
      0.866025, 0.500000, -0.500000,  0.707107, 0.707107, 0.500000,  0.707107, 0.707107, 0.500000,
      0.707107, 0.707107, -0.500000,  0.707107, 0.707107, -0.500000,  0.500000, 0.866025, 0.500000,
      0.500000, 0.866025, 0.500000,  0.500000, 0.866025, -0.500000,  0.500000, 0.866025, -0.500000,
      0.258819, 0.965926, 0.500000,  0.258819, 0.965926, 0.500000,  0.258819, 0.965926, -0.500000,
      0.258819, 0.965926, -0.500000,  -0.000000, 1.000000, 0.500000,  -0.000000, 1.000000, 0.500000,
      -0.000000, 1.000000, -0.500000,  -0.000000, 1.000000, -0.500000,  -0.258819, 0.965926, 0.500000,
      -0.258819, 0.965926, 0.500000,  -0.258819, 0.965926, -0.500000,  -0.258819, 0.965926, -0.500000,
      -0.500000, 0.866025, 0.500000,  -0.500000, 0.866025, 0.500000,  -0.500000, 0.866025, -0.500000,
      -0.500000, 0.866025, -0.500000,  -0.707107, 0.707107, 0.500000,  -0.707107, 0.707107, 0.500000,
      -0.707107, 0.707107, -0.500000,  -0.707107, 0.707107, -0.500000,  -0.866025, 0.500000, 0.500000,
      -0.866025, 0.500000, 0.500000,  -0.866025, 0.500000, -0.500000,  -0.866025, 0.500000, -0.500000,
      -0.965926, 0.258819, 0.500000,  -0.965926, 0.258819, 0.500000,  -0.965926, 0.258819, -0.500000,
      -0.965926, 0.258819, -0.500000,  -1.000000, -0.000000, 0.500000,  -1.000000, -0.000000, 0.500000,
      -1.000000, -0.000000, -0.500000,  -1.000000, -0.000000, -0.500000,  -0.965926, -0.258819, 0.500000,
      -0.965926, -0.258819, 0.500000,  -0.965926, -0.258819, -0.500000,  -0.965926, -0.258819, -0.500000,
      -0.866025, -0.500000, 0.500000,  -0.866025, -0.500000, 0.500000,  -0.866025, -0.500000, -0.500000,
      -0.866025, -0.500000, -0.500000,  -0.707107, -0.707107, 0.500000,  -0.707107, -0.707107, 0.500000,
      -0.707107, -0.707107, -0.500000,  -0.707107, -0.707107, -0.500000,  -0.500000, -0.866025, 0.500000,
      -0.500000, -0.866025, 0.500000,  -0.500000, -0.866025, -0.500000,  -0.500000, -0.866025, -0.500000,
      -0.258819, -0.965926, 0.500000,  -0.258819, -0.965926, 0.500000,  -0.258819, -0.965926, -0.500000,
      -0.258819, -0.965926, -0.500000,  0.000000, -1.000000, 0.500000,  0.000000, -1.000000, 0.500000,
      0.000000, -1.000000, -0.500000,  0.000000, -1.000000, -0.500000,  0.258819, -0.965926, 0.500000,
      0.258819, -0.965926, 0.500000,  0.258819, -0.965926, -0.500000,  0.258819, -0.965926, -0.500000,
      0.500000, -0.866025, 0.500000,  0.500000, -0.866025, 0.500000,  0.500000, -0.866025, -0.500000,
      0.500000, -0.866025, -0.500000,  0.707107, -0.707107, 0.500000,  0.707107, -0.707107, 0.500000,
      0.707107, -0.707107, -0.500000,  0.707107, -0.707107, -0.500000,  0.866026, -0.500000, 0.500000,
      0.866026, -0.500000, 0.500000,  0.866026, -0.500000, -0.500000,  0.866026, -0.500000, -0.500000,
      0.965926, -0.258819, 0.500000,  0.965926, -0.258819, 0.500000,  0.965926, -0.258819, -0.500000,
      0.965926, -0.258819, -0.500000,  };
    float normals[] = {
      0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,  1.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 1.000000,  1.000000, 0.000000, 0.000000,  0.000000, 0.000000, -1.000000,
        1.000000, 0.000000, 0.000000,  1.000000, 0.000000, 0.000000,  0.965926, 0.258819, -0.000000,
        0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,  0.965926, 0.258819, -0.000000,
        0.866025, 0.500000, -0.000000,  0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,
        0.866025, 0.500000, -0.000000,  0.707107, 0.707107, -0.000000,  0.000000, 0.000000, 1.000000,
        0.000000, 0.000000, -1.000000,  0.707107, 0.707107, -0.000000,  0.500000, 0.866025, -0.000000,
        0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,  0.500000, 0.866025, -0.000000,
        0.258819, 0.965926, 0.000000,  0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,
        0.258819, 0.965926, 0.000000,  0.000000, 1.000000, 0.000000,  0.000000, 0.000000, 1.000000,
        0.000000, 0.000000, -1.000000,  0.000000, 1.000000, 0.000000,  -0.258819, 0.965926, 0.000000,
        0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,  -0.258819, 0.965926, 0.000000,
        -0.500000, 0.866025, -0.000000,  0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,
        -0.500000, 0.866025, -0.000000,  -0.707107, 0.707107, -0.000000,  0.000000, 0.000000, 1.000000,
        0.000000, 0.000000, -1.000000,  -0.707107, 0.707107, -0.000000,  -0.866025, 0.500000, -0.000000,
        0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,  -0.866025, 0.500000, -0.000000,
        -0.965926, 0.258819, -0.000000,  0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,
        -0.965926, 0.258819, -0.000000,  -1.000000, -0.000000, -0.000000,  0.000000, 0.000000, 1.000000,
        0.000000, 0.000000, -1.000000,  -1.000000, -0.000000, -0.000000,  -0.965926, -0.258819, 0.000000,
        0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,  -0.965926, -0.258819, 0.000000,
        -0.866025, -0.500000, 0.000000,  0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,
        -0.866025, -0.500000, 0.000000,  -0.707107, -0.707107, 0.000000,  0.000000, 0.000000, 1.000000,
        0.000000, 0.000000, -1.000000,  -0.707107, -0.707107, 0.000000,  -0.500000, -0.866025, 0.000000,
        0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,  -0.500000, -0.866025, 0.000000,
        -0.258819, -0.965926, 0.000000,  0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,
        -0.258819, -0.965926, 0.000000,  0.000000, -1.000000, 0.000000,  0.000000, 0.000000, 1.000000,
        0.000000, 0.000000, -1.000000,  0.000000, -1.000000, 0.000000,  0.258819, -0.965926, 0.000000,
        0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,  0.258819, -0.965926, 0.000000,
        0.500000, -0.866025, 0.000000,  0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,
        0.500000, -0.866025, 0.000000,  0.707107, -0.707107, 0.000000,  0.000000, 0.000000, 1.000000,
        0.000000, 0.000000, -1.000000,  0.707107, -0.707107, 0.000000,  0.866025, -0.500000, 0.000000,
        0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,  0.866025, -0.500000, 0.000000,
        0.965926, -0.258819, 0.000000,  0.000000, 0.000000, 1.000000,  0.000000, 0.000000, -1.000000,
        0.965926, -0.258819, 0.000000,  };
      float uvs[] = {
        0.500000, 0.500000, 0.500000, 0.500000, 1.000000, 1.000000, 0.000000, 0.500000,
          0.000000, 1.000000, 1.000000, 0.500000, 0.000000, 0.000000, 1.000000, 0.000000,
          0.041667, 1.000000, 0.017037, 0.370590, 0.982963, 0.370590, 0.041667, 0.000000,
          0.083333, 1.000000, 0.066987, 0.250000, 0.933013, 0.250000, 0.083333, 0.000000,
          0.125000, 1.000000, 0.146447, 0.146447, 0.853553, 0.146447, 0.125000, 0.000000,
          0.166667, 1.000000, 0.250000, 0.066987, 0.750000, 0.066987, 0.166667, 0.000000,
          0.208333, 1.000000, 0.370591, 0.017037, 0.629409, 0.017037, 0.208333, 0.000000,
          0.250000, 1.000000, 0.500000, 0.000000, 0.500000, 0.000000, 0.250000, 0.000000,
          0.291667, 1.000000, 0.629410, 0.017037, 0.370590, 0.017037, 0.291667, 0.000000,
          0.333333, 1.000000, 0.750000, 0.066987, 0.250000, 0.066987, 0.333333, 0.000000,
          0.375000, 1.000000, 0.853553, 0.146447, 0.146447, 0.146447, 0.375000, 0.000000,
          0.416667, 1.000000, 0.933013, 0.250000, 0.066987, 0.250000, 0.416667, 0.000000,
          0.458333, 1.000000, 0.982963, 0.370591, 0.017037, 0.370591, 0.458333, 0.000000,
          0.500000, 1.000000, 1.000000, 0.500000, 0.000000, 0.500000, 0.500000, 0.000000,
          0.541667, 1.000000, 0.982963, 0.629410, 0.017037, 0.629410, 0.541667, 0.000000,
          0.583333, 1.000000, 0.933013, 0.750000, 0.066987, 0.750000, 0.583333, 0.000000,
          0.625000, 1.000000, 0.853553, 0.853553, 0.146447, 0.853553, 0.625000, 0.000000,
          0.666667, 1.000000, 0.750000, 0.933013, 0.250000, 0.933013, 0.666667, 0.000000,
          0.708333, 1.000000, 0.629409, 0.982963, 0.370591, 0.982963, 0.708333, 0.000000,
          0.750000, 1.000000, 0.500000, 1.000000, 0.500000, 1.000000, 0.750000, 0.000000,
          0.791667, 1.000000, 0.370590, 0.982963, 0.629410, 0.982963, 0.791667, 0.000000,
          0.833333, 1.000000, 0.250000, 0.933013, 0.750000, 0.933013, 0.833333, 0.000000,
          0.875000, 1.000000, 0.146447, 0.853553, 0.853553, 0.853553, 0.875000, 0.000000,
          0.916667, 1.000000, 0.066987, 0.750000, 0.933013, 0.750000, 0.916667, 0.000000,
          0.958333, 1.000000, 0.017037, 0.629409, 0.982963, 0.629409, 0.958333, 0.000000,
      };
      int indexcnt = 288;
      unsigned short indexarray[] = {
        0, 9, 3,  4, 11, 6,  4, 8, 11, 5, 10, 1,  0, 13, 9,  8, 15, 11,
          8, 12, 15,  10, 14, 1,  0, 17, 13,  12, 19, 15,  12, 16, 19,  14, 18, 1,
          0, 21, 17,  16, 23, 19,  16, 20, 23,  18, 22, 1,  0, 25, 21,  20, 27, 23,
          20, 24, 27,  22, 26, 1,  0, 29, 25,  24, 31, 27,  24, 28, 31,  26, 30, 1,
          0, 33, 29,  28, 35, 31,  28, 32, 35,  30, 34, 1,  0, 37, 33,  32, 39, 35,
          32, 36, 39,  34, 38, 1,  0, 41, 37,  36, 43, 39,  36, 40, 43,  38, 42, 1,
          0, 45, 41,  40, 47, 43,  40, 44, 47,  42, 46, 1,  0, 49, 45,  44, 51, 47,
          44, 48, 51,  46, 50, 1,  0, 53, 49,  48, 55, 51,  48, 52, 55,  50, 54, 1,
          0, 57, 53,  52, 59, 55,  52, 56, 59,  54, 58, 1,  0, 61, 57,  56, 63, 59,
          56, 60, 63,  58, 62, 1,  0, 65, 61,  60, 67, 63,  60, 64, 67,  62, 66, 1,
          0, 69, 65,  64, 71, 67,  64, 68, 71,  66, 70, 1,  0, 73, 69,  68, 75, 71,
          68, 72, 75,  70, 74, 1,  0, 77, 73,  72, 79, 75,  72, 76, 79,  74, 78, 1,
          0, 81, 77,  76, 83, 79,  76, 80, 83, 78, 82, 1,  0, 85, 81,  80, 87, 83,
          80, 84, 87,  82, 86, 1,  0, 89, 85,  84, 91, 87,  84, 88, 91,  86, 90, 1,
          0, 93, 89,  88, 95, 91,  88, 92, 95,  90, 94, 1,  0, 97, 93,  92, 99, 95,
          92, 96, 99,  94, 98, 1,  0, 3, 97,  96, 7, 99,  96, 2, 7,  98, 5, 1,
      };

  Mesh_t *mesh = (Mesh_t*)lxMemGenZalloc(sizeof(Mesh_t));
  lxVertex64_t *vert;
  int i;

  mesh->numIndices = indexcnt;
  mesh->indicesData16 = (ushort*)lxMemGenZalloc(sizeof(ushort)*mesh->numIndices);
  mesh->numVertices = pointcount;
  mesh->origVertexData = mesh->vertexData = lxMemGenZallocAligned(sizeof(lxVertex64_t)*mesh->numVertices,32);
  mesh->meshtype = MESH_VA;
  mesh->vertextype = VERTEX_64_TEX4;
  mesh->primtype = PRIM_TRIANGLES;
  mesh->indexMin = 0;
  mesh->indexMax = pointcount-1;
  mesh->numTris = indexcnt/3;
  mesh->index16 = LUX_TRUE;

  memcpy(mesh->indicesData16,indexarray,sizeof(ushort)*mesh->numIndices);

  vert = mesh->vertexData64;

  for (i=0;i<mesh->numVertices;i++,vert++) {
    LUX_ARRAY4SET(vert->color,255,255,255,255);
    lxVector3Copy(vert->pos,&points[i*3]);
    lxVector3Negated(vert->pos);
    lxVector3short_FROM_float(vert->normal,&normals[i*3]);
    vert->normal[0] = -vert->normal[0];
    vert->normal[1] = -vert->normal[1];
    vert->normal[2] = -vert->normal[2];
    lxVector2Copy(vert->tex,&uvs[i*2]);
  }

  mesh->numAllocIndices = mesh->numIndices;
  mesh->numAllocVertices = mesh->numVertices;

  return mesh;
}

void  Mesh_clear(Mesh_t *mesh)
{
  mesh->meshtype = MESH_UNSET;
  Mesh_setPtrsAllocState(mesh,LUX_FALSE,LUX_FALSE);
}

void  Mesh_free(Mesh_t *mesh)
{
  Mesh_clear(mesh);

  lxMemGenFree(mesh,sizeof(Mesh_t));
}


void Mesh_setBlendGL(const Mesh_t *mesh)
{
  VIDBuffer_t *buffer = mesh->vid.vbo;
  byte *ptr = buffer ? NULL : (byte*)mesh->vertexData;
  VIDVertexPointer_t *vpti = &g_VID.drawsetup.attribs.pointers[VERTEX_ATTRIB_BLENDINDICES];
  VIDVertexPointer_t *vptw = &g_VID.drawsetup.attribs.pointers[VERTEX_ATTRIB_BLENDWEIGHTS];
  VertexCustomAttrib_t cattri = (LUX_SCALAR_UINT8<<2) | 3;
  VertexCustomAttrib_t cattrw = (LUX_SCALAR_UINT16<<2) | 3 | (1<<14);

  if (mesh->meshtype == MESH_DL)
    return;

  LUX_ASSERT(mesh->vertextype == VERTEX_64_SKIN);


  {
    lxVertex64_t *vert64 = (lxVertex64_t*) ptr;
    ptr = (byte*)vert64[0].blendi;
  }

  vidBindBufferArray(buffer);

  if (vpti->buffer == buffer && vpti->ptr == ptr && vpti->stride == sizeof(lxVertex64_t) && vpti->cattr == cattri)
    return;

  glVertexAttribPointerARB(VERTEX_ATTRIB_BLENDINDICES,4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(lxVertex64_t) ,ptr);
  glVertexAttribPointerARB(VERTEX_ATTRIB_BLENDWEIGHTS,4, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(lxVertex64_t) ,ptr+4);

  vpti->buffer = buffer;
  vpti->ptr = ptr;
  vpti->stride = sizeof(lxVertex64_t);
  vpti->cattr = cattri;

  vptw->buffer = buffer;
  vptw->ptr = ptr+4;
  vptw->stride = sizeof(lxVertex64_t);
  vptw->cattr = cattrw;
}

// set tangent pointers for model
void Mesh_setTangentGL(const Mesh_t *mesh)
{
  VIDBuffer_t *buffer = mesh->vid.vbo;
  VIDVertexPointer_t *vpt = &g_VID.drawsetup.attribs.pointers[VERTEX_ATTRIB_TANGENT];
  void *ptr = buffer ? NULL : mesh->vertexData;
  VertexCustomAttrib_t cattr = (LUX_SCALAR_INT16<<2) | 3 | (1<<14);

  if ( mesh->meshtype == MESH_DL)
    return;

  LUX_ASSERT(mesh->vertextype == VERTEX_64_TEX4 || mesh->vertextype == VERTEX_64_SKIN);

  if(buffer){
    ptr = NULL;
  }
  else{
    ptr = mesh->vertexData;
  }

  vidBindBufferArray(buffer);

  {
    lxVertex64_t *vert64 = (lxVertex64_t*) ptr;
    ptr = (void*)vert64[0].tangent;
  }

  if (vpt->buffer == buffer && vpt->ptr == ptr && vpt->stride == sizeof(lxVertex64_t) && vpt->cattr == cattr)
    return;

  glVertexAttribPointerARB(VERTEX_ATTRIB_TANGENT,4, GL_SHORT, GL_TRUE, sizeof(lxVertex64_t) ,ptr);

  vpt->ptr = ptr;
  vpt->buffer = buffer;
  vpt->stride = sizeof(lxVertex64_t);
  vpt->cattr = cattr;
}

// set texcoord pointers for model
void Mesh_setTexCoordGL(const Mesh_t *mesh,uint coord, const VID_TexChannel_t channel)
{

  int cnt = 2;
  VIDBuffer_t *buffer = mesh->vid.vbo;
  GLenum datatype = GL_FLOAT;
  size_t stride = VertexSize(mesh->vertextype);
  VIDVertexPointer_t *vpt = &g_VID.drawsetup.attribs.pointers[VERTEX_ATTRIB_TEXCOORD0+coord];
  void *ptr = buffer ? NULL : mesh->vertexData;
  VertexCustomAttrib_t cattr = (LUX_SCALAR_FLOAT32<<2) | 1;

  LUX_DEBUGASSERT(coord < 4);

  vidBindBufferArray(buffer);

  switch(mesh->vertextype)
  {
  case VERTEX_64_TEX4:
  {
    lxVertex64_t* vert64 = (lxVertex64_t*)ptr;
    switch (channel){
    case VID_TEXCOORD_01: cattr |= 3; cnt = 4;
    case VID_TEXCOORD_0:  ptr = vert64[0].tex;    break;
    case VID_TEXCOORD_1:  ptr = vert64[0].tex1;   break;
    case VID_TEXCOORD_23: cattr |= 3; cnt = 4;
    case VID_TEXCOORD_2:  ptr = vert64[0].tex2; break;
    case VID_TEXCOORD_3:  ptr = vert64[0].tex3; break;
    }
  }
    break;
  case VERTEX_32_NRM:
  {
    lxVertex32_t* vert32 = (lxVertex32_t*)ptr;
    ptr = vert32[0].tex;
  }
    break;
  case VERTEX_32_TEX2:
  {
    lxVertex32_t* vert32 = (lxVertex32_t*)ptr;
    if(!channel)
      ptr = vert32[0].tex;
    else if (channel < 2)
      ptr = vert32[0].tex1;
    else{
      ptr = vert32[0].tex;
      cattr |= 3; cnt = 4;
    }
  }
    break;
  case VERTEX_32_TERR:
  {
    lxVertex32Terrain_t*  vert32 = (lxVertex32Terrain_t*)ptr;
    ptr = vert32[0].pos;
  }
    break;
  case VERTEX_32_FN:
  {
    lxVertex32FN_t* vert32FN =  (lxVertex32FN_t*)ptr;
    ptr = vert32FN->tex;
    break;
  }
  default:
    return;
  }

  if (vpt->buffer == buffer && vpt->ptr == ptr && vpt->stride == stride && vpt->cattr == cattr)
    return;

  vidSelectClientTexture(coord);
  glTexCoordPointer(cnt,datatype,stride,ptr);

  vpt->ptr = ptr;
  vpt->buffer = buffer;
  vpt->stride = stride;
  vpt->cattr = cattr;
}


void VertexAttrib_setCustomStreamGL(VertexAttrib_t attr, VIDBuffer_t *buffer, size_t stride, void *ptr,
                  VertexCustomAttrib_t cattr)
{
  lxScalarType_t type = (lxScalarType_t)((cattr>>2) & 15);
  GLenum datatype = ScalarType_toGL(type);
  uint count = ((cattr) & 3) + 1;
  GLenum normalize = (cattr & (1<<14)) ? GL_TRUE : GL_FALSE;
  booln integer = cattr & (1<<15);
  VIDVertexPointer_t *vpt = &g_VID.drawsetup.attribs.pointers[attr];
  LUX_ASSERT(attr < VERTEX_ATTRIBS);

  vidBindBufferArray(buffer);

  if (vpt->buffer == buffer && vpt->ptr == ptr && vpt->stride == stride && vpt->cattr == cattr)
    return;

  if (!integer){
    glVertexAttribPointerARB(attr,count, datatype, normalize, stride, ptr);
  }
  else{
    glVertexAttribIPointer(attr,count, datatype, stride, ptr);
  }

  vpt->ptr = ptr;
  vpt->buffer = buffer;
  vpt->stride = stride;
  vpt->cattr = cattr;
}


void Mesh_setStandardAttribsGL(const Mesh_t *mesh, booln docolor, booln donormals)
{
  lxVertexType_t vtype = mesh->vertextype;

  VIDBuffer_t *buffer = mesh->vid.vbo;
  VIDVertexPointer_t  *vptpos = &g_VID.drawsetup.attribs.pointers[VERTEX_ATTRIB_POS];
  VIDVertexPointer_t  *vptnormal = &g_VID.drawsetup.attribs.pointers[VERTEX_ATTRIB_NORMAL];
  VIDVertexPointer_t  *vptcolor = &g_VID.drawsetup.attribs.pointers[VERTEX_ATTRIB_COLOR];
  void* pointvert = mesh->vid.vbo ? NULL : mesh->vertexData;
  byte* ppos =    (byte*)VertexArrayPtr(pointvert,0,vtype,VERTEX_POS);
  byte* pcolor =  (byte*)VertexArrayPtr(pointvert,0,vtype,VERTEX_COLOR);
  byte* pnormal = (byte*)VertexArrayPtr(pointvert,0,vtype,VERTEX_NORMAL);
  VertexCustomAttrib_t  cattrpos = VertexValue(vtype,VERTEX_POSFORMAT);
  VertexCustomAttrib_t  cattrcolor = VertexValue(vtype,VERTEX_COLORFORMAT);
  VertexCustomAttrib_t  cattrnormal = VertexValue(vtype,VERTEX_NORMALFORMAT);
  ushort  stride = VertexSize(vtype);
  uint  vcnt = 3;

  vidBindBufferArray(mesh->vid.vbo);

  switch(mesh->vertextype)
  {
  case VERTEX_32_NRM:
  case VERTEX_64_TEX4:
  case VERTEX_64_SKIN:
  case VERTEX_32_TERR:
    break;
  case VERTEX_32_TEX2:
    {
      donormals = LUX_FALSE;
    }
    break;
  case VERTEX_16_TERR:
    {
      donormals = LUX_FALSE;
      vidNoVertexColor(LUX_FALSE);
      docolor = LUX_TRUE;
      // TODO seccondarycolor
    }
    break;
  case VERTEX_16_CLR:
    {
      donormals = LUX_FALSE;
    }
    break;
  case VERTEX_16_HOM:
    {
      vidNoVertexColor(LUX_TRUE);
      docolor = LUX_FALSE;
      donormals = LUX_FALSE;
      vcnt = 4;
    }
    break;
  case VERTEX_32_FN:
    {
      vidNoVertexColor(LUX_TRUE);
      docolor = LUX_FALSE;
    }
    break;
  }

  if(docolor){
    if (buffer != vptcolor->buffer || pcolor != vptcolor->ptr || stride != vptcolor->stride || cattrcolor != vptcolor->cattr)
    {
      glColorPointer(4,GL_UNSIGNED_BYTE,stride,pcolor);

      vptcolor->buffer = buffer;
      vptcolor->ptr = pcolor;
      vptcolor->stride = stride;
      vptcolor->cattr = cattrcolor;
    }
  }
  if(donormals){
    if (buffer != vptnormal->buffer || pnormal != vptnormal->ptr || stride != vptnormal->stride || cattrnormal != vptnormal->cattr)
    {
      GLenum  type =  VertexValue(vtype,VERTEX_NORMALNATIVE);
      glNormalPointer(type,stride,pnormal);

      vptnormal->buffer = buffer;
      vptnormal->ptr = pnormal;
      vptnormal->stride = stride;
      vptnormal->cattr = cattrnormal;
    }
    glEnableClientState(GL_NORMAL_ARRAY);
  }
  else{
    glDisableClientState(GL_NORMAL_ARRAY);
  }

  if (buffer != vptpos->buffer || ppos != vptpos->ptr || stride != vptpos->stride || cattrpos != vptpos->cattr)
  {
    GLenum  type =  VertexValue(vtype,VERTEX_POSNATIVE);
    glVertexPointer(vcnt,type,stride,ppos);

    vptpos->buffer = buffer;
    vptpos->ptr = ppos;
    vptpos->stride = stride;
    vptpos->cattr = cattrpos;
  }
}


// set manual texcoords
void Mesh_pushTexCoordGL(const Mesh_t *mesh,const VID_Texpush_t mode, void *data){
  lxMatrix44 eyematrix;
  lxVector3 norm;
  lxVector3 vert;
  lxVector3 reflect;
  lxMatrix44 inveyematrix;
  int i;
  float *pos;
  float *normal;
  float *outcoords = NULL;
  float ratio = (float)g_VID.state.viewport.bounds.width / (float)g_VID.state.viewport.bounds.height;

  glGetFloatv(GL_MODELVIEW_MATRIX,eyematrix);
  /*
  if (!g_VID.drw_texcoords[g_VID.state.activeTex])
  g_VID.drw_texcoords[g_VID.state.activeTex] = malloc(sizeof(float)*VID_MAX_VERTICES*2);
  outcoords = g_VID.drw_texcoords[g_VID.state.activeTex];
  */

  if(mesh->vertextype == VERTEX_64_TEX4){
    pos = mesh->vertexData64[0].pos;
    normal = mesh->vertexData64[0].pos;
  }
  else{
    pos = mesh->vertexData32[0].pos;
    normal = mesh->vertexData32[0].pos;
  }

  switch (mode){
    case VID_TEXPUSH_SCREEN:
      {
        // push polys
        for ( i = 0; i < mesh->numVertices; i++){
          vert[3]=1;
          lxVector3Copy(vert,pos);
          lxVector4Transform1(vert,eyematrix);
          outcoords[i*2] = -vert[0]* ratio / vert[2] + 0.5;
          outcoords[i*2+1] = -vert[1]* ratio / vert[2] + 0.5;
          VertexIncrPtr(mesh->vertextype,float,pos);
        }
      }
      break;

    case VID_TEXPUSH_EYELIN:
      {
        // push polys
        for ( i = 0; i < mesh->numVertices; i++){
          vert[3]=1;
          lxVector3Copy(vert,pos);
          lxVector4Transform1(vert,eyematrix);
          outcoords[i*2] = vert[0];
          outcoords[i*2+1] = vert[1];
          VertexIncrPtr(mesh->vertextype,float,pos);
        }
      }
      break;

    case VID_TEXPUSH_SPHERE:
      {
        float dot;
        float m;

        lxMatrix44Transpose(inveyematrix,eyematrix);
        // push polys
        for ( i = 0; i < mesh->numVertices; i++){
          lxVector3Copy(norm,normal);
          lxVector3Copy(vert,pos);

          lxVector3Transform1(vert,eyematrix);
          lxVector3NormalizedFast(vert);

          lxMatrix44VectorRotate(inveyematrix,norm);

          dot = 2.0*lxVector3Dot(norm,vert);
          reflect[0]= vert[0] - dot * norm[0];
          reflect[1]= vert[1] - dot * norm[1];
          reflect[2]= vert[2] - dot * norm[2];

          m = 2 * sqrt(reflect[0]*reflect[0] + reflect[1]*reflect[1] + (reflect[2]+1)*(reflect[2]+1));
          if (m)
          {
            outcoords[i*2] = reflect[0] / m + 0.5f;
            outcoords[i*2+1] = reflect[1] / m + 0.5f;
          }
          else
          {
            outcoords[i*2] = 0.5f;
            outcoords[i*2+1] = 0.5f;
          }
          VertexIncrPtr(mesh->vertextype,float,pos);
          VertexIncrPtr(mesh->vertextype,float,normal);
        }
      }
      break;

    case VID_TEXPUSH_REFLECT:
      {
        float dot;
        lxMatrix44Transpose(inveyematrix,eyematrix);

        // push polys
        for ( i = 0; i < mesh->numVertices; i++){

          lxVector3Copy(norm,normal);
          lxVector3Copy(vert,pos);

          lxVector3Transform1(vert,eyematrix);
          lxVector3NormalizedFast(vert);
          lxMatrix44VectorRotate(inveyematrix,normal);

          dot = 2.0*lxVector3Dot(norm,vert);
          reflect[0]=vert[0] - dot * norm[0];
          reflect[1]=vert[1] - dot * norm[1];
          reflect[2]=vert[2] - dot * norm[2];

          outcoords[i*2] = reflect[0] + 0.5;
          outcoords[i*2+1] = reflect[1] + 0.5;

          VertexIncrPtr(mesh->vertextype,float,pos);
          VertexIncrPtr(mesh->vertextype,float,normal);
        }
      }
      break;

    case VID_TEXPUSH_NORMAL:
      {
        lxMatrix44Transpose(inveyematrix,eyematrix);
        // push polys
        for ( i = 0; i < mesh->numVertices; i++){
          lxVector3Copy(norm,normal);

          lxVector3TransformRot1(norm,inveyematrix);

          outcoords[i*2] = norm[0] + 0.5;
          outcoords[i*2+1] = norm[1] + 0.5;

          VertexIncrPtr(mesh->vertextype,float,normal);
        }
      }
      break;
  }
  // set coordpointer
  // unlock arrays

  vidBindBufferArray(NULL);
  glTexCoordPointer(2,GL_FLOAT,sizeof(float)*2,outcoords);
}


extern void Draw_Mesh_simple(Mesh_t *mesh);

// vbo = 0, ibo = 1
void  Mesh_submitVBO(const Mesh_t *mesh, booln isindex, int from, int cnt)
{
  if (mesh->meshtype != MESH_VBO)
    return;

  if (isindex && mesh->vid.ibo && (mesh->indicesData)){
    from = LUX_MIN(LUX_MAX(0,from),mesh->numAllocIndices);
    cnt  = cnt < 0 ? mesh->numAllocIndices : cnt;
    cnt  = LUX_MIN(mesh->numAllocIndices-from,cnt);

    if (cnt < 1)
      return;

    if (mesh->index16){
      ushort * LUX_RESTRICT indexIn = mesh->indicesData16+from;
      uint indexsize = sizeof(ushort);

      if (mesh->vid.vbooffset){
        ushort *indexOut = VIDBuffer_mapGL(mesh->vid.ibo,VID_BUFFERMAP_WRITE);
        int i;

        indexOut+=(mesh->vid.ibooffset+from);

        for (i = 0; i < cnt; i++){
          indexOut[i] = indexIn[i] + mesh->vid.vbooffset;
        }

        VIDBuffer_unmapGL(mesh->vid.ibo);
      }
      else{
        VIDBuffer_submitGL(mesh->vid.ibo,
          (mesh->vid.ibooffset+from)*indexsize,
          (indexsize*cnt),
          indexIn);
      }

    }
    else{
      uint * LUX_RESTRICT indexIn = mesh->indicesData32+from;
      uint indexsize = sizeof(uint);

      if (mesh->vid.vbooffset){
        uint *indexOut = VIDBuffer_mapGL(mesh->vid.ibo,VID_BUFFERMAP_WRITE);
        int i;

        indexOut+=(mesh->vid.ibooffset+from);

        for (i = 0; i < cnt; i++){
          indexOut[i] = indexIn[i] + mesh->vid.vbooffset;
        }

        VIDBuffer_unmapGL(mesh->vid.ibo);
      }
      else{
        VIDBuffer_submitGL(mesh->vid.ibo,
          (mesh->vid.ibooffset+from)*indexsize,
          (indexsize*cnt),
          indexIn);
      }
    }


  }
  else if (!isindex && mesh->vid.vbo && mesh->origVertexData){

    from = LUX_MIN(LUX_MAX(0,from),mesh->numAllocVertices);
    cnt  = cnt < 0 ? mesh->numAllocVertices : cnt;
    cnt  = LUX_MIN(mesh->numAllocVertices-from,cnt);

    if (cnt < 1)
      return;

    VIDBuffer_submitGL(mesh->vid.vbo,
      (mesh->vid.vbooffset+from)*VertexSize(mesh->vertextype),
      VertexSize(mesh->vertextype)*cnt,
      VertexArrayPtr(mesh->origVertexData,from,mesh->vertextype,VERTEX_START));
  }
}

void  Mesh_retrieveVBO(Mesh_t *mesh, booln isindex, int from, int cnt)
{
  if (mesh->meshtype != MESH_VBO)
    return;

  if (isindex && mesh->vid.ibo && (mesh->indicesData)){
    booln hasushort = mesh->index16;
    void *data = mesh->indicesData;
    uint indexsize = hasushort ? sizeof(ushort) : sizeof(uint);

    from = LUX_MIN(LUX_MAX(0,from),mesh->numAllocIndices);
    cnt  = cnt < 0 ? mesh->numAllocIndices : cnt;
    cnt  = LUX_MIN(mesh->numAllocIndices-from,cnt);

    if (cnt < 1)
      return;


    VIDBuffer_retrieveGL(mesh->vid.vbo,mesh->vid.ibooffset*indexsize,indexsize*cnt,data);
    // we must subtract vertexoffsets
    if (hasushort){
      int i;
      for (i = from; i < from+cnt; i++){
        mesh->indicesData16[i] = mesh->indicesData16[i]-mesh->vid.vbooffset;
      }
    }
    else{
      int i;
      for (i = from; i < from+cnt; i++){
        mesh->indicesData32[i] = mesh->indicesData32[i]-mesh->vid.vbooffset;
      }
    }
  }
  else if (!isindex && mesh->vid.vbo && mesh->origVertexData){
    uint vertexsize = VertexSize(mesh->vertextype);

    from = LUX_MIN(LUX_MAX(0,from),mesh->numAllocVertices);
    cnt  = cnt < 0 ? mesh->numAllocVertices : cnt;
    cnt  = LUX_MIN(mesh->numAllocVertices-from,cnt);

    if (cnt < 1)
      return;

    VIDBuffer_retrieveGL(mesh->vid.vbo,
      (mesh->vid.vbooffset+from)*vertexsize,
      vertexsize*cnt,
      VertexArrayPtr(mesh->origVertexData,from,mesh->vertextype,VERTEX_START));
  }
}

booln Mesh_typeVA(Mesh_t *mesh)
{
  int vsize;
  int isize;

  if (MESH_VA == mesh->meshtype)
    return LUX_TRUE;


  vsize = (VertexSize(mesh->vertextype)*mesh->numVertices);
  isize = ((mesh->index16 ? sizeof(uint) : sizeof(ushort))*mesh->numIndices);

  // delete previous
  if (mesh->meshtype == MESH_DL){
    glDeleteLists(mesh->vid.glID,1);
    LUX_PROFILING_OP (g_Profiling.global.memory.viddlmem -= vsize + isize);
  }

  memset(&mesh->vid,0,sizeof(MeshGL_t));

  if (!mesh->indicesData || !mesh->vertexData){
    return LUX_FALSE;
  }

  mesh->meshtype = MESH_VA;
  return LUX_TRUE;
}

booln Mesh_typeDL(Mesh_t *mesh, int texcoords, booln novcolor, booln nonormals, booln needtangents)
{
  int i;
  int vsize,isize;


  if (MESH_DL == mesh->meshtype)
    return LUX_TRUE;

  if (!mesh->indicesData || !mesh->vertexData){
    return LUX_FALSE;
  }

  Mesh_typeVA(mesh);

  vsize = (VertexSize(mesh->vertextype)*mesh->numVertices);
  isize = ((mesh->index16 ? sizeof(uint) : sizeof(ushort))*mesh->numIndices);

  LUX_PROFILING_OP (g_Profiling.global.memory.viddlmem += vsize + isize);

  vidCheckError();
  VIDRenderFlag_setGL(0);
  vidBindBufferArray(NULL);
  vidBindBufferIndex(NULL);

  mesh->vid.glID = glGenLists(1);
  g_VID.drawsetup.setnormals = nonormals ? LUX_FALSE :LUX_TRUE;
  glEnableClientState(GL_VERTEX_ARRAY);
  vidNoVertexColor(novcolor);


  if (GLEW_ARB_vertex_program && needtangents){
    g_VID.drawsetup.attribs.needed |= 1<<VERTEX_ATTRIB_TANGENT;
    glEnableVertexAttribArrayARB(VERTEX_ATTRIB_TANGENT);
  }
  for (i = 0; i < texcoords; i++){
    vidSelectTexture(i);
    vidTexCoordSource(VID_TEXCOORD_ARRAY,VID_TEXCOORD_0);
    //Mesh_setTexCoordGL(mesh,i,VID_TEXCOORD_0);
  }
  // draw
  glNewList(mesh->vid.glID, GL_COMPILE);
  Draw_Mesh_simple(mesh);
  glEndList();

  for (i = texcoords-1; i >= 0; i--){
    vidSelectTexture(i);
    vidTexCoordSource(VID_TEXCOORD_NONE,VID_TEXCOORD_NOSET);
  }
  if (GLEW_ARB_vertex_program && needtangents){
    glDisableVertexAttribArrayARB(VERTEX_ATTRIB_TANGENT);
    g_VID.drawsetup.attribs.needed = 0;
  }


  vidNoVertexColor(novcolor==0);
  g_VID.drawsetup.setnormals = LUX_FALSE;


  mesh->meshtype = MESH_DL;
  vidCheckError();

  return LUX_TRUE;
}

void Mesh_getPtrsAllocState(Mesh_t *mesh, booln* vertex, booln* index)
{
  *vertex = mesh->vertexData != NULL;
  *index = mesh->indicesData != NULL;
}

booln Mesh_setPtrsAllocState( Mesh_t *mesh, booln vertex, booln index )
{
  int vertexcount = mesh->numAllocVertices;
  int indexcount = mesh->numAllocIndices;
  booln hasvertex = mesh->vertexData != NULL;
  booln hasindex = mesh->indicesData != NULL;
  booln hasushort = mesh->index16;

  if (MESH_VBO != mesh->meshtype && mesh->meshtype != MESH_UNSET)
    return LUX_FALSE;


  if (vertex != hasvertex){
    if (vertex){
      size_t vertexsize = VertexSize(mesh->vertextype);
      size_t vsize = vertexsize*vertexcount;
      mesh->origVertexData = mesh->vertexData = lxMemGenZallocAligned(vsize,32);

      if (mesh->vid.vbo){
        // fill with content
        VIDBuffer_retrieveGL(mesh->vid.vbo,mesh->vid.vbooffset*vertexsize,vsize,mesh->origVertexData);
      }
    }
    else{
      lxMemGenFreeAligned(mesh->vertexData,VertexSize(mesh->vertextype)*vertexcount);
      mesh->origVertexData = mesh->vertexData = NULL;
    }
  }
  if (index != hasindex){
    if (index){
      if (indexcount){
        size_t isize;
        size_t indexsize;
        if (hasushort){
          indexsize = sizeof(ushort);
          isize = indexsize * indexcount;
          mesh->indicesData16 = (ushort*)lxMemGenZalloc(isize);
        }
        else{
          indexsize = sizeof(uint);
          isize = indexsize * indexcount;
          mesh->indicesData32 = (uint*)lxMemGenZalloc(isize);
        }
      }
    }
    else{
      if (hasushort){
        lxMemGenFree(mesh->indicesData16,indexcount * sizeof(ushort));
        mesh->indicesData16 = NULL;
      }
      else{
        lxMemGenFree(mesh->indicesData32,indexcount * sizeof(uint));
        mesh->indicesData32 = NULL;
      }
    }
  }

  return LUX_TRUE;
}

booln Mesh_typeVBO(Mesh_t *mesh, VIDBuffer_t *vbo,int vbooffset, VIDBuffer_t *ibo, int ibooffset)
{
  int vsize;
  int isize;
  uint voffset;
  uint ioffset;

  size_t vertexsize = VertexSize(mesh->vertextype);
  size_t elemsize = mesh->index16 ? sizeof(ushort) : sizeof(uint);

  if (MESH_VBO == mesh->meshtype)
    return LUX_TRUE;

  vsize = (vertexsize*mesh->numAllocVertices);
  isize = (elemsize*mesh->numAllocIndices);

  Mesh_typeVA(mesh);

  voffset = vbooffset >= 0 ? vbooffset : VIDBuffer_alloc(vbo,vsize,sizeof(lxVertex64_t));
  if (voffset == -1)
    return LUX_FALSE;

  if (ibo){
    ioffset = ibooffset >= 0 ? ibooffset : VIDBuffer_alloc(ibo,isize,sizeof(uint));
    if (ioffset == -1)
      return LUX_FALSE;
  }

  // vertex buffer
  mesh->vid.vbo = vbo;
  mesh->vid.vbooffset = voffset/vertexsize;
  Mesh_submitVBO(mesh,LUX_FALSE,-1,-1);

  // index buffer
  mesh->vid.ibo = ibo;
  if (ibo){
    mesh->vid.ibooffset = ioffset/elemsize;
    Mesh_submitVBO(mesh,LUX_TRUE,-1,-1);
  }

  mesh->meshtype = MESH_VBO;
  return LUX_TRUE;
}

void Mesh_updateMinMax(Mesh_t *mesh)
{

  mesh->indexMax = 0;
  mesh->indexMin = 0xFFFFFFFF;

  if (mesh->index16){
    int i;
    ushort* primstart = mesh->indicesData16;
    for (i = 0; i < mesh->numIndices; i++,primstart++){
      mesh->indexMax = LUX_MAX(mesh->indexMax,*primstart);
      mesh->indexMin = LUX_MIN(mesh->indexMin,*primstart);
    }
  }
  else{
    int i;
    uint* primstartInt = mesh->indicesData32;
    for (i = 0; i < mesh->numIndices; i++,primstartInt++){
      mesh->indexMax = LUX_MAX(mesh->indexMax,*primstartInt);
      mesh->indexMin = LUX_MIN(mesh->indexMin,*primstartInt);
    }
  }
}

void Mesh_updateTrisCnt(Mesh_t *mesh)
{
  switch(mesh->primtype) {
    case PRIM_POINTS:
    case PRIM_LINES:
    case PRIM_LINE_LOOP:
    case PRIM_LINE_STRIP:
      mesh->numTris = 0;  break;
    case PRIM_TRIANGLES:
      mesh->numTris = mesh->numIndices/3; break;
    case PRIM_TRIANGLE_STRIP:
      mesh->numTris = mesh->numIndices-2; break;
    case PRIM_TRIANGLE_FAN:
      mesh->numTris = mesh->numIndices-2; break;
    case PRIM_QUADS:
      mesh->numTris = mesh->numIndices/2; break;
    case PRIM_QUAD_STRIP:
      mesh->numTris = mesh->numIndices-2; break;
    case PRIM_POLYGON:
      mesh->numTris = mesh->numIndices-2; break;
    default:
      LUX_ASSERT(0);
  }
}

int  Mesh_getPrimCnt(const Mesh_t *mesh)
{
  switch(mesh->primtype) {
    case PRIM_POINTS:
      return mesh->numIndices;
    case PRIM_LINES:
      return mesh->numIndices/2;
    case PRIM_LINE_LOOP:
      return mesh->numIndices;
    case PRIM_LINE_STRIP:
      return mesh->numIndices-1;
    case PRIM_TRIANGLES:
      return mesh->numIndices/3;
    case PRIM_TRIANGLE_STRIP:
      return mesh->numIndices-2;
    case PRIM_TRIANGLE_FAN:
      return mesh->numIndices-2;
    case PRIM_QUADS:
      return mesh->numIndices/4;
    case PRIM_QUAD_STRIP:
      return (mesh->numIndices-2)/2;
    case PRIM_POLYGON:
      return 1;
    default:
      return 0;
  }
}



