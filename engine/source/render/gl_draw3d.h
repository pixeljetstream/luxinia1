// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_DRAW3D_H__
#define __GL_DRAW3D_H__

/*
DRAW3D
------

Drawing of 3d DrawNodes

There is also various helper functions to draw boxes, bone axis and so on

Author: Christoph Kubisch

*/

#include "../common/vid.h"
#include "../resource/resmanager.h"
#include "../render/gl_camlight.h"
#include "../render/gl_shader.h"
#include "../render/gl_drawmesh.h"

typedef enum DrawNodeType_e{
  DRAW_NODE_OBJECT,
  DRAW_NODE_SKINNED_OBJECT,
  DRAW_NODE_WORLD,
  DRAW_NODE_SKINNED_WORLD,
  DRAW_NODE_BATCH_WORLD,
  DRAW_NODE_TEXT_OBJECT,
  DRAW_NODE_SHADOW_OBJECT,
  DRAW_NODE_FUNC,
}DrawNodeType_t;

enum DrawSortTypes_e{
  DRAW_SORT_TYPE_NORMAL,
  DRAW_SORT_TYPE_SKINNED,
  DRAW_SORT_TYPE_TERRAIN,
  DRAW_SORT_TYPE_TRAIL,
};

enum DrawSortFlags_e{
  DRAW_SORT_FLAG_SKINNED  = 1<<30,
  DRAW_SORT_FLAG_FAR    = 1<<31,
};

// bit shifts
// the sortkey is made of
// 2BITs : TYPES | 3 BITs : LIGHTCNT | 7 BITs: SHADER | 9 BITs: Textures
// | 11 BITs: Renderflag => 32 BITs
enum DrawSortShifts_e{
  DRAW_SORT_SHIFT_RENDERFLAG = 4, // this one is negative
  DRAW_SORT_SHIFT_MATERIAL = 11,
  DRAW_SORT_SHIFT_SHADER = 20,
  DRAW_SORT_SHIFT_LIGHTCNT = 27,
  DRAW_SORT_SHIFT_TYPE = 30,
};

typedef struct DrawEnv_s{
  int         needslight;
  int         numFxLights;
  const Light_t   **fxLights;

  enum32        projectorflag;
  int         numProjectors;  // projectors its visible to
  Projector_t     **projectors;

  int         lightmapRID;    // lightmap texture
  float       *lmtexmatrix;
}DrawEnv_t;

typedef struct DrawSortNode_s{
  uint          value;
  DrawNodeType_t      *pType;
}DrawSortNode_t;

typedef struct DrawBatchInfo_s{
  Reference   batchbuffer;
  struct List3DBatchContainer_s *batchcontainer;
  uint        vertoffset;
  uint        indoffset;
}DrawBatchInfo_t;

typedef struct DrawNode_s{
  DrawNodeType_t      type;
  DrawSortNode_t      sortkey;
  DrawMesh_t        draw;

  float         *matrix;
  float         *bonematrix;

  DrawEnv_t       *env;
  void          *overrideVertices;  // to be used instead of mesh's

  int           layerID;
  int           sortID;

  union{
    struct  // used in most types
    {
      float     *renderscale;
      SkinObject_t  *skinobj;
    };    // used in batchnodes
    DrawBatchInfo_t   batchinfo;
    // used in func must return number of passes
    struct{
      int (*drawfunc) (struct DrawNode_s* drawnode, void *upvalue, enum32 renderflag);
      void*     drawupvalue;
    };

  };


#ifdef LUX_DEVBUILD
  struct List3DNode_s   *l3d;
#endif
}DrawNode_t;

typedef struct DrawBatch_s{
  Mesh_t        batchmesh;
  ushort        *curind;
  ushort        *maxind;
  enum32        renderflag;
  const DrawNode_t  *dnode;
}DrawBatch_t;

typedef struct DrawLoop_s{
  DrawNode_t      **nodes;
  int         numNodes;
  enum32        renderflag;
  const DrawNode_t  *dnode;
}DrawLoop_t;



//////////////////////////////////////////////////////////////////////////
// DrawNode

  // sorttype might be altered for batchnodes and skinned nodes
void  DrawNode_updateSortID(DrawNode_t *dnode, int sorttype);
void  DrawNodes_drawList(const DrawSortNode_t *list, const int listlength, const int ignoreprojs,const enum32 ignoreflags, const enum32 forceflags);


//////////////////////////////////////////////////////////////////////////
// Draw Special

void Draw_SkyBox(struct SkyBox_s *skybox);
void Draw_SkyL3DNode(struct List3DNode_s *skydome, enum32 forceflag, enum32 ignoreflag,lxVector3 cammoveinfluence);

//////////////////////////////////////////////////////////////////////////
// Helpers

void Draw_Model_normals(const struct Model_s *model);
void Draw_Model_bones(const struct Model_s *model, const float *localmat,  lxVector3 renderscale);
void Draw_Model_bbox(const struct Model_s *model);
void Draw_ModelAnim_path( struct Model_s *model, struct Anim_s *anim,const int density);

void Draw_axis();
void Draw_box(const lxVector3 min,const lxVector3 max,const int drawWireframe);
void Draw_boxWireColor(const lxVector3 min, const lxVector3 max, const lxVector4 color);
void Draw_sphere(const float size, const int hslices, const int vslices);
void Draw_cube(const float size);
void Draw_frustum(lxFrustum_t *frustum);



#endif

