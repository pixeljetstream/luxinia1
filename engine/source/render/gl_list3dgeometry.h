// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_LIST3DGEO_H__
#define __GL_LIST3DGEO_H__

/*
List3DGeometry
--------------

Some special geometry for L3D Nodes is defined in here.
List3D Primitives are just single meshes.

List3DShadowModel
shadow volume mesh that is created on the fly.
it can either be linked to another l3dmodel
or an l3dprimitive


*/

// Primitives

#include "../common/reflib.h"
#include "../common/vid.h"
#include "gl_camlight.h"
#include "gl_draw3d.h"
#include "../scene/scenetree.h"

typedef enum List3DPrimitiveType_e{
  L3DPRIMITIVE_SPHERE,
  L3DPRIMITIVE_CYLINDER,
  L3DPRIMITIVE_CUBE,
  L3DPRIMITIVE_QUAD,
  L3DPRIMITIVE_QUAD_CENTERED,
}List3DPrimitiveType_t;


typedef struct List3DPrimitive_s{
  List3DPrimitiveType_t type;
  lxBoundingBox_t     bbox;
  Mesh_t          *orgmesh;
  Reference       usermesh;

  float         size[3];
}List3DPrimitive_t;


// ShadowModel

typedef enum ShadowMeshType_e{
  SHADOWMSH_TYPE_UNSET,
  SHADOWMSH_TYPE_CAP,       // silhouette and caps at nearplane/infinity
  SHADOWMSH_TYPE_UNCAP,     // silhouette extruded to infinity
  SHADOWMSH_TYPE_COLLAPSE,    // 1 point at infinity
}ShadowMeshType_t;

typedef struct EdgeVerts_s{
  union{
    struct{
      ushort      vertA;
      ushort      vertB;
    };
    uint        verts;
  };

}EdgeVerts_t;

typedef struct EdgeList_s{
  float*        planeA;
  float*        planeB;
  EdgeVerts_t     edge;
  int         faceA;
  int         faceB;
}EdgeList_t;

typedef struct ShadowMesh_s{
  lxVector4       lightpos;     // last relativ light position

  int         refindex;     // for models
  Mesh_t        *refmesh;

  lxVector4       *faceplanes;    // one normal for every face in refmesh
  int         numFaceplanes;

  EdgeList_t      *edges;
  int         numEdges;

  float       extrusiondistance;
  float       lastextrusion;

  void        *lastvertices;

  ShadowMeshType_t  meshtype;
  Mesh_t        *mesh;        // the volumemesh
}ShadowMesh_t;

typedef struct ShadowModel_s{
  lxClassType     targettype;
  Reference     targetref;      // must be valid l3dmodel or l3dprimitive
  struct List3DNode_s *targetl3d;
  Mesh_t        *targetmesh;    // l3dprimitive
  Reference     lightref;       // the light it uses
  Light_t       *light;

  lxVector3       origcenter;
  lxBoundingSphere_t  bsphere;
  lxVector3       bpoints[8];

  float       extrusiondistance;

  ShadowMesh_t  *shadowmeshes;
  int       numShadowmeshes;
}ShadowModel_t;



// LevelModel

// all split structures use bufferalloc

typedef struct LevelBoundingBox_s{
  lxBoundingBox_t   bbox;
  lxBoundingBoxCenter_t   bctr;
}LevelBoundingBox_t;

typedef struct LevelSplitMesh_s{
  int         origMeshID;
  int         lightmapRID;
  int         activeTris;
  ushort        *indices; // indices for every mesh, MAXSHORT for already split
  MeshObject_t    *meshobj;
}LevelSplitMesh_t;

typedef struct LevelSplitNodeMesh_s{
  LevelSplitMesh_t  *spmesh;
  ushort        *indices;
  int         numIndices;

  struct LevelSplitNodeMesh_s *prev,*next;
}LevelSplitNodeMesh_t;

typedef struct LevelSplitNode_s{
  LevelSplitNodeMesh_t  *meshListHead;
  int         numTris;
  LevelBoundingBox_t    bbox;
  LevelBoundingBox_t    bboxverts;

  struct LevelSplitNode_s *prev,*next;
}LevelSplitNode_t;

typedef struct LevelSplit_s{
  LevelSplitMesh_t  *splitmeshes;
  int         numSplitmeshes;
  LevelSplitNode_t  *nodeListHead;
}LevelSplit_t;

// genalloc
typedef struct LevelSubNode_s{
  DrawEnv_t     drawenv;
  struct VisObject_s  *visobj;
  struct VisSet_s   *visset;
  struct VisLightResult_s *vislightres;
}LevelSubNode_t;

typedef struct LevelSubMesh_s{
  Mesh_t      mesh;
  int       origMeshID;
  LevelSubNode_t  *subnode;
}LevelSubMesh_t;

typedef struct LevelModel_s{
  int       modelRID;

  SceneNode_t   **scenenodes;
  int       numScenenodes;
  LevelSubNode_t  *subnodes;  // at least 1 per scenenode, maximum lightmapcnt per scenenode
  int       numSubnodes;

  int       *lightmapRIDs;
  int       numLightmapRIDs;


  LevelSubMesh_t  *submeshes; // can be display lists, mostly VA with own indices and shared vertices from original model
  int       numSubmeshes; // at least 1 per subnode
}LevelModel_t;

//////////////////////////////////////////////////////////////////////////
// ShadowModel

// sets up all data, returns true if model was closed
// we do a strstr if target is model and only pick meshes wiht the nameflag
// it can return NULL, if meshes were not found or not closed
// also sets boundingsphere
ShadowModel_t* ShadowModel_new(const struct List3DNode_s *l3dtarget,const struct List3DNode_s *lightl3d,const char *meshnameflag);

void ShadowModel_free(ShadowModel_t *smdl);

  // RPOOLUSE
int ShadowModel_updateMeshes(ShadowModel_t *smdl,struct List3DNode_s *l3dself, Camera_t *cam);

int ShadowModel_visible(ShadowModel_t *smdl,lxMatrix44 mat,Camera_t *cam);

void ShadowModel_updateExtrusionLength(ShadowModel_t *smdl);


//////////////////////////////////////////////////////////////////////////
// LevelModel

LevelModel_t* LevelModel_new(int modelresid, int dimension[3],int minpolycount, float margin);

void      LevelModel_free(LevelModel_t *lmdl);

  // returns NULL terminated ptr list
  // RPOOLUSE
DrawNode_t**  LevelModel_getVisible(LevelModel_t *lmdl, struct List3DNode_s *l3dself, Camera_t *cam);

void      LevelModel_updateVis(LevelModel_t *lmdl, struct List3DNode_s *l3dself);

  // preps visdata
void      LevelModel_updateL3D(struct List3DNode_s *l3dself);

#endif

