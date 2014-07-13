// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "actorlist.h"
#include "../common/memorymanager.h"
#include "../common/3dmath.h"
#include "../common/timer.h"
#include "../render/gl_list3d.h"


static ActorNode_t *l_actorList = NULL;


float*  MIF_getElements (void *data, float *f)
{
  return Matrix44Interface_getElements((Matrix44Interface_t*)data,f);
}
void  MIF_setElements (void *data, float *f)
{
  Matrix44Interface_setElements((Matrix44Interface_t*)data,f);
}
void  MIF_fnFree    (void *data)
{
  Matrix44Interface_unref((Matrix44Interface_t*)data);
}

static Matrix44InterfaceDef_t l_mifdef = {
  MIF_getElements, MIF_setElements,
  MIF_fnFree };

int ActorNode_getCount ()
{
  int cnt = 0;
  ActorNode_t *browse;

  lxListNode_countSize(l_actorList,browse,cnt)

  return cnt;
}
ActorNode_t* ActorNode_getNext (ActorNode_t *node)
{
  return node->next;
}

ActorNode_t* ActorNode_getPrev (ActorNode_t *node)
{
  return node->prev;
}

ActorNode_t* ActorNode_getFromIndex (int id)
{
  ActorNode_t *browse;

  lxListNode_getAtOffset(l_actorList,l_actorList, browse, id);

  return browse;
}

ActorNode_t* ActorNode_new (const char *name,int drawable,lxVector3 pos)
{
  ActorNode_t *self = genzallocSIMD(sizeof(ActorNode_t));

  LUX_SIMDASSERT(((size_t)((ActorNode_t*)self)->link.matrix) % 16 == 0);
  LUX_ASSERT(self);

  if (name)
    gennewstrcpy(self->name,name);


  lxMatrix44IdentitySIMD(self->link.matrix);
  lxMatrix44IdentitySIMD(self->internMatrix);
  lxMatrix44SetTranslation(self->internMatrix,pos);
  self->mIFintern = P4x4Matrix_new(self->internMatrix);
  self->mIFextern = NULL;
  Matrix44Interface_ref(self->mIFintern);
  self->link.matrixIF = Matrix44Interface_new(&l_mifdef,(void*)self->mIFintern);

  if (drawable){
    self->link.visobject = VisObject_new(VIS_OBJECT_DYNAMIC,self,self->name);
  }


  lxListNode_init(self);
  lxListNode_insertPrev(l_actorList,self);

  LUX_PROFILING_OP (g_Profiling.global.nodes.actors++);

  self->link.reference = Reference_new(LUXI_CLASS_ACTORNODE,self);

  return self;
}

static void ActorNode_free(ActorNode_t *self)
{
  if (self->name)
    genfreestr(self->name);

  if (self->link.visobject)
    VisObject_free(self->link.visobject);

  Matrix44Interface_invalidate(self->mIFintern);
  Matrix44Interface_invalidate(self->link.matrixIF);

  Reference_invalidate(self->link.reference);

  LUX_PROFILING_OP (g_Profiling.global.nodes.actors--);

  lxListNode_remove(l_actorList,self);
  genfreeSIMD(self,sizeof(ActorNode_t));
}

void RActorNode_free(lxRactornode ref)
{
  ActorNode_free((ActorNode_t*)Reference_value(ref));
}

ActorNode_t* ActorNode_get(const char *name)
{
  // FIXME
  return NULL;
}

void ActorList_init()
{
  Reference_registerType(LUXI_CLASS_ACTORNODE,"actornode",RActorNode_free,NULL);
}

Matrix44Interface_t *ActorNode_getMatrixIF(ActorNode_t* self){
  return self->link.matrixIF;
}

void ActorNode_setMatrixIF(ActorNode_t* self,Matrix44Interface_t* mif)
{
  if (mif==NULL)
    mif = self->mIFintern;
  Matrix44Interface_ref(mif);
  Matrix44Interface_unref((Matrix44Interface_t*)self->link.matrixIF->data);
  self->link.matrixIF->data = (void*)mif;
}


void ActorList_think() {
  ActorNode_t *browse;
  float *m;


  lxListNode_forEach(l_actorList,browse) {
    LUX_ASSERT(browse);
    // check for validity
    m = Matrix44Interface_getElementsCopy(browse->link.matrixIF,browse->link.matrix);
    if (m == NULL ) {
      ActorNode_setMatrixIF(browse,NULL);
      m = Matrix44Interface_getElementsCopy(browse->link.matrixIF,browse->link.matrix);
    }
    LUX_ASSERT(m);
    if (browse->link.visobject){
      List3DNode_t *l3dbrowse;
      lxLN_forEach(browse->link.visobject->l3dvisListHead,l3dbrowse,visnext,visprev)
        List3DNode_markUpdate(l3dbrowse);
      lxLN_forEachEnd(browse->link.visobject->l3dvisListHead,l3dbrowse,visnext,visprev);
    }

  } lxListNode_forEachEnd(l_actorList,browse);
}
