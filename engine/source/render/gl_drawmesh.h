// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_DRAWMESH_H__
#define __GL_DRAWMESH_H__

/*
DRAWMESH
--------

Draws Meshes,
a basic and shader version is available.


Author: Christoph Kubisch

*/

#include "../common/vid.h"

typedef enum DrawMeshType_e{
  DRAW_MESH_BASIC,
  DRAW_MESH_BASIC_SKIN,
  DRAW_MESH_BASIC_PROJ,
  DRAW_MESH_BASIC_PROJ_SKIN,
  DRAW_MESH_TEX,
  DRAW_MESH_TEX_SKIN,
  DRAW_MESH_TEX_PROJ,
  DRAW_MESH_TEX_PROJ_SKIN,
  DRAW_MESH_MATERIAL,
  DRAW_MESH_MATERIAL_SKIN,
  DRAW_MESH_MATERIAL_PROJ,
  DRAW_MESH_MATERIAL_PROJ_SKIN,
  DRAW_MESH_AUTO,
  DRAW_MESHS,
}DrawMeshType_t;

enum  DrawMeshShift_e{
  DRAW_MESH_SHIFT_SKIN = 1,
  DRAW_MESH_SHIFT_PROJ = 2,
  DRAW_MESH_SHIFT_TYPE = 4,
};

typedef struct DrawMesh_s{
  int           matRID;
  union{
    struct Mesh_s   *mesh;
    struct PText_s    *ptext;
  };
  struct MaterialObject_s *matobj;
  VIDDrawState_t      state;
  lxVector4       color;
}DrawMesh_t;

typedef struct DrawData_s{
  struct DrawNode_s *dnode;
  int       curPassNum;
  int       numPasses;
  int       origPasses;
  int       texStages;
  int       projStages;
  int       projOffset;

  struct Shader_s     *shader;
  struct GLShaderPass_s *glpass;
  struct GLShaderPass_s *glpassFirst;

  int       noDisplayList;

  int       vgpu;
  int       fgpu;
  int       ggpu;
  int       attenuate;
  int       fogon;
  int       resetfog;
  int       togglelit;
  int       hwSkinning;
  int       shaderSkinning;

  MeshType_t    omeshtype;
  VIDBuffer_t   *ovbo;
  VIDBuffer_t   *oibo;

  int       numLights;

  int       revertscissor;
  int       kickedprojectors;

  int       numFxLights;
  const void**  fxLights;
}DrawData_t;

typedef int (Draw_Mesh_func)(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors, struct SkinObject_s *skinobj);

typedef struct DrawMeshFunc_s{
  Draw_Mesh_func  *func;
}DrawMeshFunc_t;

//////////////////////////////////////////////////////////////////////////
// Draw

void  Draw_init();
void  Draw_deinit();
void  Draw_initFrame();
void  Draw_useBaseShaders(int state);

void  Draw_pushMatricesGL();
void  Draw_setFXLights(int numLights, const struct Light_s **fxlights);

//////////////////////////////////////////////////////////////////////////
// DrawMesh

int DrawMesh_draw(DrawMesh_t *draw, flags32 forceflags, flags32 ignoreflags);
void DrawMesh_clear(DrawMesh_t *dnode);

//////////////////////////////////////////////////////////////////////////
// Draw Mesh

// render funcs
// renders a mesh
void Draw_Mesh_simple(Mesh_t *mesh);

// does shader setup / texture binds, returns number of passes used
int  Draw_Mesh_auto( Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors, struct SkinObject_s *skinobj);

int  Draw_Mesh_type( Mesh_t *mesh, DrawMeshType_t type, const int texindex, struct Projector_s **projectors,const int numProjectors, struct SkinObject_s *skinobj);


//////////////////////////////////////////////////////////////////////////
// Inline Draw


#endif

