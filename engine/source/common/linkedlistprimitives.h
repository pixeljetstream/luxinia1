// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __LINKEDLISTPRIMITIVES_H__
#define __LINKEDLISTPRIMITIVES_H__
#include <luxinia/luxcore/contmacrolinkedlist.h>

typedef struct Int2Node_s {
  int data;
  int data2;
  struct Int2Node_s LUX_LISTNODEVARS;
} Int2Node_t;

typedef struct Char2PtrNode_s {
  char *name;
  char *str;
  struct Char2PtrNode_s LUX_LISTNODEVARS;
} Char2PtrNode_t;

//////////////////////////////////////////////////////////////////////////
//

Int2Node_t* Int2Node_new(int value, int value2);
void    Int2Node_free(Int2Node_t*node);

#endif

