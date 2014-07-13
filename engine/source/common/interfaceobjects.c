// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include <stdio.h>
#include <stdlib.h>
#include "memorymanager.h"
#include "interfaceobjects.h"
#include "3dmath.h"

Matrix44Interface_t*  Matrix44Interface_new(const Matrix44InterfaceDef_t *funcdef,void *data)
{
  Matrix44Interface_t *self;
  if (!funcdef)
    return NULL;

  self = (Matrix44Interface_t*) lxMemGenAlloc(sizeof(Matrix44Interface_t));
  self->funcdef = *funcdef;
  self->data    = data;

  self->refCount    = 1;
  self->valid     = 1;

  return self;
}

Matrix44Interface_t*  Matrix44Interface_ref(Matrix44Interface_t *self)
{
  if (self->valid == 0) return NULL; // no getMatrix, no link!
  self->refCount++;
  return self;
}

Matrix44Interface_t*  Matrix44Interface_unref(Matrix44Interface_t *self)
{
  self->refCount--;
  if (self->refCount > 0) return self;

  if (self->funcdef.fnFree) self->funcdef.fnFree(self->data);
  lxMemGenFree(self,sizeof(Matrix44Interface_t));

  return NULL;
}

float*  Matrix44Interface_getElements(Matrix44Interface_t *self, float *f)
{ //
  if (self->valid == 0 ) return NULL;
  return self->funcdef.fnGetElements(self->data,f);
}

float* Matrix44Interface_getElementsCopy(Matrix44Interface_t *self, float *f)
{
  float *ret;

  if (self->valid == 0 ) return NULL;
  ret = self->funcdef.fnGetElements(self->data,f);
  if (!ret) return NULL;

  if (ret==f) return f;

  LUX_SIMDASSERT(((size_t)ret) % 16 == 0);
  LUX_SIMDASSERT(((size_t)f) % 16 == 0);
  lxMatrix44Copy(f,ret);

  return f;
}

void  Matrix44Interface_setElements(Matrix44Interface_t *self, float *f)
{ // set matrix values by array
  if (self->valid == 0 ) return;
  self->funcdef.fnSetElements(self->data,f);
}


void  Matrix44Interface_invalidate(Matrix44Interface_t *self)
{ // invalidation of matrix -> refcount--
  if (self->valid == 0) return;
  self->valid = 0;
  Matrix44Interface_unref(self);
//  self->data      = NULL;
//  self->refCount--;
}


////////////////////////////////////////////////////////////////////////////////

static void P4x4MatrixDef_fnFree(void *data)
{

}

static float* P4x4MatrixDef_getElements (void *data, float *f)
{
  return ((float*)data);
}

static void   P4x4MatrixDef_setElements (void *data, float *f)
{
  memcpy(data,f,sizeof(float)*16);
}

Matrix44Interface_t* P4x4Matrix_new(float *matrix)
{
  static Matrix44InterfaceDef_t PMatrixDef = {
    P4x4MatrixDef_getElements,P4x4MatrixDef_setElements,
    P4x4MatrixDef_fnFree
  };

  return Matrix44Interface_new(&PMatrixDef,matrix);
}


