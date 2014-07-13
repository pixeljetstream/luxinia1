// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __INTERFACEOBJECT_H__
#define __INTERFACEOBJECT_H__

#include "types.h"
#include "interfaceobjectscom.h"

typedef struct Matrix44Interface_s
{
  int           valid;
  void          *data;
  Matrix44InterfaceDef_t  funcdef;

  int           refCount;
} Matrix44Interface_t;


  // A Matrix44 Interface pointing to given matrix
  // as long as MIF is in use pointer must be valid
  // pointer won't be freed on free
Matrix44Interface_t* P4x4Matrix_new(float *matrix);



#endif

