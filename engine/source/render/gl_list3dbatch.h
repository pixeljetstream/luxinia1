// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_LIST3DBATCH_H__
#define __GL_LIST3DBATCH_H__

/*
DRAW3DBATCH
-----------

List3DBatchContainer
contain all vertices and indices as well as proper vboIDs. Because indices are limited
to short size, we can need multiple BatchContainers per BatchBuffer
Meshes will point towards proper vertices/indices and store an id of the container they use.

List3DBatchBuffer
contains many batched meshes along with their batchcontainers

Author: Christoph Kubisch

*/


#include "../render/gl_draw3d.h"

typedef struct DrawL3DPairNode_s
{
  struct DrawNode_s     *dnode;
  struct List3DNode_s     *l3dnode;
  struct DrawL3DPairNode_s  *prev,*next;
}DrawL3DPairNode_t;

typedef struct List3DBatchMaterialNode_s
{
  DrawL3DPairNode_t   *drawnodes;
  int       matRID;
  int       vcount;
  int       icount;

  struct List3DBatchMaterialNode_s  *prev,*next;
}List3DBatchMaterialNode_t;

typedef struct List3DBatchContainerNode_s
{
  List3DBatchMaterialNode_t     *matListHead;
  int                 vcount;
  int                 icount;
  lxVertexType_t            vtype;
  struct List3DBatchContainerNode_s *prev,*next;
}List3DBatchContainerNode_t;

typedef struct List3DBatchVertexNode_s
{
  DrawL3DPairNode_t     *vertListHead;
  List3DBatchContainerNode_t  *containerListHead;
}List3DBatchVertexNode_t;

typedef struct List3DBatchContainer_s
{
  lxVertexType_t  vertextype;
  VIDBuffer_t   vbo;

  union{
    void        *vertexData;
    struct lxVertex64_s *vertexData64;
    struct lxVertex32_s *vertexData32;
  };
  int     numVertices;

  ushort    *indicesData16;
  int     numIndices;
}List3DBatchContainer_t;


typedef struct List3DBatchBuffer_s
{
  Reference       reference;
  lxMemoryStackPTR      mempool;
  int           l3dsetID;

  struct List3DNode_s   **meshl3ds;   // use mempool
  Mesh_t          *meshes;    // use mempool
  int           numMeshes;


  List3DBatchContainer_t  *containers;  // use mempool
  int           numContainers;
}List3DBatchBuffer_t;

  // makes use of bufferzalloc
  // gets all valid l3dmodels drawnodes from linked scenenode
void  List3DBatchBuffer_init(List3DBatchBuffer_t *buffer,  struct SceneNode_s *snroot);

  // empty buffer
List3DBatchBuffer_t*  List3DBatchBuffer_new(int l3dsetID);

  // deletes buffer and all data within it
  // luxinia will crash if meshes still point in it
//void  List3DBatchBuffer_free(List3DBatchBuffer_t *buffer);
void  RList3DBatchBuffer_free(lxRl3dbatchbuffer ref);

#endif

