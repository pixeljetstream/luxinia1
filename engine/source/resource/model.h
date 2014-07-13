// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __MODEL_H__
#define __MODEL_H__

/*
MODEL
-----

The Model contains Meshes that represent some geometry.
A BoneSystem is used to control the hierarchy within the model.
Models can be animated (dynamic) or (static).
For extra info per Mesh a MeshObject is used, which can link the Mesh with
a Bone and contains its materialinfo.
When its dynamic a "skin" can be defined which deforms the mesh based
on bone's motion and vertex weighting. Additionally you can do morphing.

To make use of the same Model at different states (morphing,animation)
Modelnstances are used. A Modelnstance contains BoneUpdate (bone animation)
and ModelUpdate (morphing).

Depending on hardware the meshes for the models may be represented as
regular VertexArrays, DisplayLists (per mesh) or VertexBufferObject (one for the whole model)

Rendering is done in /render/gl_draw3d

Model   (nested,ResMem)
Modelnstances(user,GenMem)
ModelUpdate (nested,GenMem)
ModelControl(user,GenMem)

Author: Christoph Kubisch

*/

#include "bone.h"
#include "../common/common.h"
#include "../common/reflib.h"

#define MODEL_MAX_WEIGHTS   4
#define MODEL_MAX_MESHES    128
#define MODEL_CONTROL_SELF (Model_t*)1

#define MODEL_USERSTART   "USER_MODEL"
#define MODEL_USERSTART_LEN 10

typedef enum ModelType_e{
  MODEL_UNSET,
  MODEL_DYNAMIC,          // a model that is built on the fly for rendering (animated)
  MODEL_DYNAMIC_NODL,       // no display list
  MODEL_DYNAMIC_NOVBO,      // no vbo
  MODEL_DYNAMIC_NODL_NOVBO,
  MODEL_STATIC,         // no further changes allowed to the model
  MODEL_STATIC_NODL,        // static as well and no display list
  MODEL_STATIC_NOVBO,       // no vbo
  MODEL_STATIC_NODL_NOVBO,
  MODEL_ONLYDATA,         // a static model loaded not supposed to be rendered directly
}ModelType_t;

typedef enum ModelControlType_e{
  MODEL_CONTROL_UNSET,
  MODEL_CONTROL_FORCEDIRTY,
  MODEL_CONTROL_MORPH_MODEL,
  MODEL_CONTROL_MORPH_MESH
}ModelControlType_t;

typedef struct VertexContainer_s{
  int         vertextype;
  int         numVertexDatas;
  int         *numVertices;
  union{
    void        **vertexDatas;
    struct lxVertex64_s **vertexDatas64;
    struct lxVertex32_s **vertexDatas32;
  };
}VertexContainer_t;

typedef struct MeshObject_s
{
  char    *name;

  char    *texname;
  int     texRID;
    // preload: 0 no material set, 1 material or texture string
    // postload: -1 no texture, else use vidMaterial(int) to check if material or not

  Bone_t      *bone;
    // bone reference to, if we use its final matrix for drawing

  int       skinID;
  struct Mesh_s *mesh;
}MeshObject_t;

typedef struct SkinVertex_s
{
  int     numWeights;         // Maximum of 4 weights per vertex
  int     matrixIndex[MODEL_MAX_WEIGHTS]; // Index to matrix in skin->bonesIndices no pointers because of hardware skinning
  float   weights[MODEL_MAX_WEIGHTS];

}SkinVertex_t;


typedef struct Skin_s
{
  float     *lastMatrices;
  uint      lastTime;
  void      *lastVertices;  // last input vertices

  int     hwskinning;
  int     maxweights;

  int     numBones;     // number of used bones
  uint    *boneIndices;
  float   *boneMatrices;

  int       numSkinVertices;  // number of weighted vertices
  SkinVertex_t  *skinvertices;    // the indices/length of this array match the mesh->vertexData


  void  *outvertices; // here the results are computed to

}Skin_t;

typedef struct SkinObject_s{
  float   *relMatricesT;
  uint    *time;
  Skin_t    *skin;
  int     skinID;
}SkinObject_t;

typedef struct Model_s
{
  ResourceInfo_t resinfo;

  ModelType_t modeltype;
  ModelType_t orgtype;

  short   lmcoords;
  short   vcolors;
  enum32    vertextype;
  uint    vboID[2];

  // geometry
  int       numMeshObjects;
  MeshObject_t  *meshObjects;

  int   numVerticesTotal;
  int   numIndicesTotal;

  // bounding box
  lxBoundingBox_t bbox;

  // hierarchy
  BoneSys_t   bonesys;

  // dynamic
  int     numSkins;
  Skin_t    *skins;

  float   prescale[3];
}Model_t;

typedef struct ModelControl_s
{
  ModelControlType_t    type;
  Reference       reference;
  struct ModelUpdate_s  *modelupd;

  Model_t         *modelMorphFrom;
  Model_t         *modelMorphTo;

  float       morphFactor;

  int       meshFrom;
  int       meshTo;
  int       mesh;

  struct ModelControl_s *allocprev,*allocnext;
  struct ModelControl_s LUX_LISTNODEVARS;
}ModelControl_t;

typedef struct ModelUpdate_s
{
  Model_t     *model;

  VertexContainer_t *vertexcont;

  ModelControl_t  *controlListHead;
  int       *dirtyVDatas;
  int       dirty;
}ModelUpdate_t;

typedef struct Modelnstance_s
{
  int         references; // how often the object is referenced

  int         modelRID;   // the model
  BoneSysUpdate_t   *boneupd; // data to be used for bone updates
  ModelUpdate_t   *modelupd;  // data for morphs

  int         numSkinObjs;
  SkinObject_t    *skinobjs;

  uint        time;   // g_LuxTimer.time at which it was last updated

}Modelnstance_t;

#define isModelDynamic(model) (model->modeltype == MODEL_DYNAMIC )



//////////////////////////////////////////////////////////////////////////
// VERTEX CONTAINER

VertexContainer_t*  VertexContainer_new(Model_t *model);
void        VertexContainer_free(VertexContainer_t *vcont);

void  VertexContainer_transform(VertexContainer_t *vcont, Model_t *model, lxMatrix44 matrix);

//////////////////////////////////////////////////////////////////////////
// VertexArray

void  VertexArray_transform(void *vertices,enum32 vtype, int numVertices, lxMatrix44 matrix);

  // there is no error checking if "what" is supported by vertextype so be careful
void  VertexArray_transformValue(void *vertices, enum VertexValue_e what,enum32 vtype, int numVertices, lxMatrix44 matrix);

void  VertexArray_minmax(void *vertices, enum32 vtype, int numVertices, lxVector3 min, lxVector3 max);

  // marks is filled with the index the vertex is within range to (smallest index first)
void  VertexArray_markWeld(void *vertices, enum32 vtype, int numVertices, int *marks, float rangeSq);

///////////////////////////////////////////////////////////////////////////////
// MODEL UPDATE


ModelUpdate_t * ModelUpdate_new(Model_t *model);
void      ModelUpdate_free(ModelUpdate_t *modelupd);
void      ModelUpdate_run(ModelUpdate_t *modelupd);

  // resets all morphs to reference data
void      ModelUpdate_reset(ModelUpdate_t *modelupd);


///////////////////////////////////////////////////////////////////////////////
// MODEL CONTROL


ModelControl_t *ModelControl_new();
//void      ModelControl_free(ModelControl_t *mcontrol);
void      ModelControl_activate(ModelControl_t *mcontrol,ModelUpdate_t *modelupd);
void      ModelControl_deactivate(ModelControl_t *mcontrol);
void      ModelControl_init();

void RModelControl_free(lxRmorphcontrol ref);

///////////////////////////////////////////////////////////////////////////////
// MODEL OBJECT


Modelnstance_t* Modelnstance_new(int modelRID,const booln boneupd,const booln modelupd);
void      Modelnstance_updatePtrs(Modelnstance_t * minst);
Modelnstance_t* Modelnstance_ref(Modelnstance_t*);
Modelnstance_t* Modelnstance_unref(Modelnstance_t *modelobj);


//////////////////////////////////////////////////////////////////////////
// SkinObject

  // returns pointer to computed vertices
void* SkinObject_run(SkinObject_t *skinobj, void *vertices, int vtype, int HWskinning);


///////////////////////////////////////////////////////////////////////////////
// MODEL

int   Model_searchMesh(Model_t  *model, const char *name);
int   Model_linkBones(Model_t *model);
void  Model_updateBones(Model_t *model,BoneUpdateType_t update);
void  Model_initBones(Model_t *model);

  // initializes relative vertex data
void  Model_initSkin(Model_t *model);

  // precomputes vertices if possible
void  Model_initVertices(Model_t*model);
  // counts total indices/vertexcounts
int   Model_initStats(Model_t*model);

void  Model_allocMeshObjs(Model_t*model, int meshcount);
void  Model_allocBones(Model_t *model, int bonecount);
void  Model_allocSkins(Model_t *model, int skincount);

void  Model_updateBoundingBox(Model_t *model);

int   Model_optimalMeshType(Model_t *model);

void  Model_initGL(Model_t *model, int meshtype, int lmcoords, int vcolors);
  // destroys GL resources if needed
  // full also clears vbo references
void  Model_clearGL(Model_t *model, int full);

  // clears GL and unrefs
void  Model_clear(Model_t *model);

  // user created mesh
void  Model_setMesh(Model_t*model, int mesh, char *name, int vertexcount, int indicescount ,char *texname);

  // user create bone
void  Model_setBone(Model_t *model, int bone, char *name, float *refmatrix, int parentid);

  // user create skin
void  Model_setSkin(Model_t *model, int skin, int skinverts, int bones);

int   Model_searchMeshPtr(Model_t*model, struct Mesh_s *mesh);

///////////////////////////////////////////////////////////////////////////////
// HELPERS

void  Mesh_fixVertices(struct Mesh_s  *mesh, lxMatrix44 matrix);    // precomputes vertices and unlinks the mesh bone
void  Mesh_createTangents(struct Mesh_s *mesh);


//////////////////////////////////////////////////////////////////////////
// LOADING

void ModelLoader_setPrescale(lxVector3 scale);
int  ModelLoader_getPrescale(lxVector3 scale);
int  ModelLoader_postInit(Model_t *model);

#endif

