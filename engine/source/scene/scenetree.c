// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/3dmath.h"
#include "scenetree.h"
#include "../render/gl_list3d.h"


// LOCALS
static struct SceneTreeData_s{
  SceneNode_t *root;
}l_SceneTree = {NULL};

static void SceneTree_updateNode_recursive(SceneNode_t *sn);
void RSceneNode_free (Reference ref);

//////////////////////////////////////////////////////////////////////////
// SceneTree
void  SceneTree_init()
{
  Reference_registerType(LUXI_CLASS_SCENENODE,"scenenode",RSceneNode_free,NULL);

  if (!l_SceneTree.root){
    l_SceneTree.root = SceneNode_new(LUX_TRUE,SCENE_NODE_ROOTNAME);
    Reference_makeStrong(l_SceneTree.root->link.reference);
    l_SceneTree.root->flag |= SCENE_NODE_ROOT;
  }

  lxVector3Set(l_SceneTree.root->link.visobject->bbox.min,-1,-1,-1);
  lxVector3Set(l_SceneTree.root->link.visobject->bbox.max,1,1,1);


}

void  SceneTree_deinit()
{
  lxRscenenode ref = l_SceneTree.root->link.reference;
  // free everything
  l_SceneTree.root = NULL;
  Reference_release(ref);
}


void  SceneTree_run()
{
  SceneNode_t *browse;
  // just to play safe root never changes
  clearFlag(l_SceneTree.root->flag,SCENE_NODE_UPDATED);

  // update world
  if (l_SceneTree.root->flag & SCENE_NODE_RECOMPILE){
    VisTest_recompileStatic();
    lxListNode_forEach (l_SceneTree.root->childrenListHead,browse)
      LUX_DEBUGASSERT(browse);
      SceneTree_updateNode_recursive(browse);
    lxListNode_forEachEnd(l_SceneTree.root->childrenListHead,browse);
  }
  clearFlag(l_SceneTree.root->flag,SCENE_NODE_RECOMPILE);
}

static void SceneNode_updateMatrix(SceneNode_t *sn)
{
  List3DNode_t *l3dbrowse;
  if (sn->parent->flag & SCENE_NODE_ROOT)
    lxMatrix44CopySIMD(sn->link.matrix,sn->localMatrix);
  else
    lxMatrix44MultiplySIMD(sn->link.matrix,sn->parent->link.matrix,sn->localMatrix);

  // tell the world my matrix changed
  if (sn->link.visobject){
    lxLN_forEach(sn->link.visobject->l3dvisListHead,l3dbrowse,visnext,visprev)
      List3DNode_markUpdate(l3dbrowse);
    lxLN_forEachEnd(sn->link.visobject->l3dvisListHead,l3dbrowse,visnext,visprev);
  }
}

static void SceneTree_updateNode_recursive(SceneNode_t *sn)
{ // recursive
  SceneNode_t *browse;

  // when parent changed or my local matrix was changed, we gotta update our position
  if((sn->parent->flag & SCENE_NODE_UPDATED || sn->flag & SCENE_NODE_UPDATED)){
    SceneNode_updateMatrix(sn);
  }

  // do same with children
  lxListNode_forEach (sn->childrenListHead,browse)
    LUX_DEBUGASSERT(browse);
    SceneTree_updateNode_recursive(browse);
  lxListNode_forEachEnd(sn->childrenListHead,browse);

  clearFlag(sn->flag,SCENE_NODE_UPDATED);
}

void  SceneNode_update(SceneNode_t* self){
  SceneTree_updateNode_recursive(self);
}

int Scene_getNodeCount()
{
  return 0;
}

SceneNode_t* SceneTree_getRootNode()
{
  return l_SceneTree.root;
}


//////////////////////////////////////////////////////////////////////////
// SceneNode Interfaces


float *SceneNode_matIFGetElements(void *data,float*mat)
{
  return ((SceneNode_t*)data)->link.matrix;
}

void  SceneNode_matIFSetElements(void *data,float*mat)
{
  SceneNode_setMatrix((SceneNode_t*)data,mat);
}

static Matrix44InterfaceDef_t l_SceneTreeMatrixDef ={
  SceneNode_matIFGetElements,SceneNode_matIFSetElements,NULL
};

Matrix44Interface_t*  SceneNode_newMatrix44Interface(SceneNode_t  *sn)
{
  return Matrix44Interface_new(&l_SceneTreeMatrixDef,sn);
}

//////////////////////////////////////////////////////////////////////////
// SceneNode

SceneNode_t *SceneNode_new(int drawable,const char *name)
{
  SceneNode_t *sn = genzallocSIMD(sizeof(SceneNode_t));

  LUX_SIMDASSERT((size_t)((SceneNode_t*)sn)->link.matrix % 16 == 0);

  lxListNode_init(sn);
  lxMatrix44IdentitySIMD(sn->localMatrix);
  lxMatrix44IdentitySIMD(sn->link.matrix);

  if (name)
    gennewstrcpy(sn->name,name);

  sn->link.matrixIF = SceneNode_newMatrix44Interface(sn);

  if (drawable) {
    sn->link.visobject = VisObject_new(VIS_OBJECT_STATIC,sn,sn->name);
  }
  if (l_SceneTree.root)
    SceneNode_link(sn,l_SceneTree.root);
  else
    sn->parent = sn;

  sn->link.reference = Reference_new(LUXI_CLASS_SCENENODE,sn);

  LUX_PROFILING_OP (g_Profiling.global.nodes.scenenodes++);
  return sn;
}



static void SceneNode_free(SceneNode_t* sn)
{
  SceneNode_t *browse;

  // link children to parent
  // build new local matrix for children

  // cant free root
  if (sn == l_SceneTree.root)
    return;

  // Unlink children
  while (sn->childrenListHead != NULL){
    lxListNode_popFront(sn->childrenListHead,browse);
    browse->parent = NULL;

    Reference_release(browse->link.reference);
  }

  if (sn->parent)
    lxListNode_remove(sn->parent->childrenListHead,sn);

  if (sn->name)
    genfreestr(sn->name);

  if (sn->link.matrixIF)
    Matrix44Interface_invalidate(sn->link.matrixIF);

  if(sn->link.visobject)
    VisObject_free(sn->link.visobject);

  Reference_invalidate(sn->link.reference);

  LUX_PROFILING_OP (g_Profiling.global.nodes.scenenodes--);

  genfreeSIMD(sn,sizeof(SceneNode_t));
}
void SceneNode_freeWithChildren(SceneNode_t* sn)
{
  SceneNode_t *browse;

  while (sn->childrenListHead != NULL){
    lxListNode_popFront(sn->childrenListHead,browse);
    browse->parent = NULL;
    SceneNode_freeWithChildren(browse);
  }
  SceneNode_free(sn);
}

void RSceneNode_free (lxRscenenode ref) {
  SceneNode_free((SceneNode_t*)Reference_value(ref));
}
Matrix44Interface_t *SceneNode_getMatrixIF(SceneNode_t* self){
  return self->link.matrixIF;
}

void SceneNode_setMatrix(SceneNode_t* sn,lxMatrix44 matrix)
{
  if (matrix)
    lxMatrix44CopySIMD(sn->localMatrix,matrix);
  // if there are children we need a full recompile
  if (sn->childrenListHead){
    sn->flag |= SCENE_NODE_UPDATED;
    // a matrix update occured recompile tree
    l_SceneTree.root->flag |= SCENE_NODE_RECOMPILE;
  }
  else{// not then we can directly perform the update here
    SceneNode_updateMatrix(sn);
    VisTest_recompileStatic();
  }

}



void SceneNode_link(SceneNode_t* sn, SceneNode_t *parent)
{
  SceneNode_t* oldparent = sn->parent;

  if (oldparent){
    lxListNode_remove(oldparent->childrenListHead,sn);
    oldparent = oldparent != l_SceneTree.root ? oldparent : NULL;
  }

  if (parent){
    lxListNode_addLast(parent->childrenListHead,sn);
    if (parent != l_SceneTree.root){

      if (!oldparent){
        Reference_ref(sn->link.reference);
      }
    }

    sn->flag |= SCENE_NODE_UPDATED;
    l_SceneTree.root->flag |= SCENE_NODE_RECOMPILE;
    sn->parent = parent;
  }
  else if (oldparent){
    sn->parent = NULL;
    Reference_releaseCheck(sn->link.reference);
  }
}



