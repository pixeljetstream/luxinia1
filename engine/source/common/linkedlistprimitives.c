// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "linkedlistprimitives.h"
#include "memorymanager.h"


//////////////////////////////////////////////////////////////////////////
// Int2Node

Int2Node_t* Int2Node_new(int value, int value2){
  Int2Node_t *node = (Int2Node_t*)lxMemGenZalloc(sizeof(Int2Node_t));
  lxListNode_init(node);
  node->data = value;
  node->data2 = value2;
  return node;
}

void  Int2Node_free(Int2Node_t*node){
  lxMemGenFree(node,sizeof(Int2Node_t));

}

