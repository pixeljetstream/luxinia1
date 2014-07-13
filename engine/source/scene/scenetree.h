// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __SCENETREE_H__
#define __SCENETREE_H__

#include "../common/common.h"
#include "../scene/linkobject.h"
#include "../scene/vistest.h"
#include "../common/interfaceobjects.h"
#include "../common/reflib.h"

/*
SCENE TREE
----------

A Scenegraph containing objects that should not change their position
often, ie. mostly being static. A Node can also be a terrain, all
children of it will be placed "on the terrain"+"local matrix"

Each Node has

Matrix44Interface:
has only "getElements" and "setElements" Function
setElements updates the local Matrix44 (same as SceneNode_setMatrix)
getElements returns the world matrix

StateInterface:
handles visibility states

*/

#define SCENE_NODE_ROOTNAME   "_default_root"


enum  SceneNodeFlag_e{
  SCENE_NODE_ROOT     = 1,
  SCENE_NODE_RECOMPILE    = 2,
  SCENE_NODE_UPDATED    = 4,  // set if the local matrix changed
  SCENE_NODE_DRAW     = 8,  // set if it needs a state interface
  SCENE_NODE_COLLIDE    = 16, // will create a geom in physics world
  SCENE_NODE_VIS_UPDATED  = 32, // set if visobject info changed
  SCENE_NODE_LEVELMODEL = 64, // when created via levelmodel
};

typedef struct SceneNode_s{
  // 8 DW
  LinkObject_t    link; // must come first
  // 4 DW - 4 DW aligned
  lxMatrix44      localMatrix;

  // 4 DW
  struct SceneNode_s  *parent;
  struct SceneNode_s  *childrenListHead;
  struct SceneNode_s  *prev,*next;

  enum32        flag;
  char        *name;
}SceneNode_t;

//////////////////////////////////////////////////////////////////////////
// SceneTree

void  SceneTree_init();
void  SceneTree_deinit();

  // if the scene was modified via Matrix44/StateInterfaces or linking/manual matrix set
  // all nodes that were affected will be recalculated in their position
  // also the VisTest info will be updated if needed
void  SceneTree_run();

SceneNode_t* SceneTree_getRootNode();

//////////////////////////////////////////////////////////////////////////
// SceneNode

  // creates a new unlinked node, name is optional
SceneNode_t *SceneNode_new(int drawable,const char *name);


//void  SceneNode_free(SceneNode_t* self);
//void  SceneNode_freeWithChildren(SceneNode_t* sn);
void  RSceneNode_free(lxRscenenode ref);

  // can be done thru matrix Interface as well
  // or manually reset local matrix
  // if matrix is NULL it marks the node as updated
void    SceneNode_setMatrix(SceneNode_t* self,lxMatrix44 matrix);
void    SceneNode_update(SceneNode_t* self);

void SceneNode_link(SceneNode_t* self, SceneNode_t *parent);

Matrix44Interface_t *SceneNode_getMatrixIF(SceneNode_t* self);

#endif

