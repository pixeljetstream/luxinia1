// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef _ACTORLIST_H_
#define _ACTORLIST_H_

#include "../common/common.h"
#include "../scene/linkobject.h"
#include "../scene/vistest.h"
#include "../common/interfaceobjects.h"
#include "../common/reflib.h"

typedef struct ActorNode_s{
  LinkObject_t    link; // must come first
  lxMatrix44      internMatrix;

  Matrix44Interface_t *mIFintern;
  Matrix44Interface_t *mIFextern;
  char        *name;

  struct ActorNode_s  *prev,*next;
} ActorNode_t;

void ActorList_init();
void ActorList_think();
ActorNode_t* ActorNode_new (const char *name,int drawable,lxVector3 pos);
//void ActorNode_free(ActorNode_t *node);
void RActorNode_free(lxRactornode ref);
Matrix44Interface_t *ActorNode_getMatrixIF(ActorNode_t* self);
void ActorNode_setMatrixIF(ActorNode_t* self,Matrix44Interface_t* mif);
ActorNode_t* ActorNode_get(const char *name);

int ActorNode_getCount ();
ActorNode_t* ActorNode_getNext (ActorNode_t *node);
ActorNode_t* ActorNode_getPrev (ActorNode_t *node);
ActorNode_t* ActorNode_getFromIndex (int index);

#endif //_ACTORLIST_H_
