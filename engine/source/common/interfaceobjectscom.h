// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __INTERFACEOBJECT_COMMLUX_H__
#define __INTERFACEOBJECT_COMMLUX_H__

////////////////////////////////////////////////////////////////////////////////
// Matrix44Interface
//
// With the interface you can link consumer nodes to custom spatial host.
// Spatial information is represented as a single Matrix44,
// stored in colum major as float[16] (OpenGL like)
//
// The content is provided by the host, which also provides the Matrix44Interface.
// Consumers use "ref" and "unref" to show that the Matrix44Interface is still in use.
// Once refcount became 0 on "unref" the custom free function is called.
// When the host became invalid, it is crucial to invalidate the Matrix44Interface
// as it prevents further access and referencing.

struct Matrix44Interface_s;
typedef struct Matrix44Interface_s* Matrix44InterfacePTR;

typedef struct Matrix44InterfaceDef_s{
  // retrieves content of host, either directly
  // or copied into copybuffer. In all cases returned
  // if LUX_SIMD_SSE is defined copybuffer & return ptr are/must be 16-byte aligned
  float*  (*fnGetElements)  (void *userdata, float *copybuffer);
  // sets host to given content
  void  (*fnSetElements)  (void *userdata, float *f);
  //optional to inform host when ref count became 0
  void  (*fnFree)   (void *userdata);

}Matrix44InterfaceDef_t;

// Creation and maintaining
  // used by the functions that are used as the "source" of the matrix
  // Be aware for cache efficiency each object gets its own copy of the functions
Matrix44InterfacePTR  Matrix44Interface_new(const Matrix44InterfaceDef_t *funcdef,void *userdata);

  // the reference count increases with each link, but if the interface is
  // invalid, it won't increase and returns NULL
Matrix44InterfacePTR  Matrix44Interface_ref(Matrix44InterfacePTR self);

  // decrements the reference count and returns NULL if the interface was
  // freed otherwise returns self
Matrix44InterfacePTR  Matrix44Interface_unref(Matrix44InterfacePTR self);

  // similiar to unlink, except that it is used by the host object if it
  // doesn't provide matrixdata any longer. Ie for consumers this means
  // the data is invalid.
void  Matrix44Interface_invalidate  (Matrix44InterfacePTR self);

// Content Access
// these will use the Matrix44InterfaceDef_t of the Matrix44Interface
// operations will show no effect if the Matrix44Interface was invalidated
// before (return NULL).


void  Matrix44Interface_setElements (Matrix44InterfacePTR self, float *content);
  // returns content directly or copied via buffer
  // when used "directly" the pointer must be valid for the current
  // frame
float*  Matrix44Interface_getElements (Matrix44InterfacePTR self, float *copybuffer);
  // enforces copying
float*  Matrix44Interface_getElementsCopy(Matrix44InterfacePTR self, float *copybuffer);


#endif

