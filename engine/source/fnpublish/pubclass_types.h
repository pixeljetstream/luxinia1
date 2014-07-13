// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef PUBLUXI_CLASS_STRUCTS_H_
#define PUBLUXI_CLASS_STRUCTS_H_

#include "fnpublish.h"
#include "../common/reflib.h"
#include "../common/interfaceobjects.h"


typedef enum MeshArrayType_e{
  MESHARRAY_L3DMODEL_MESHID,
  MESHARRAY_POINTERS,
}MeshArrayType_t;

typedef struct MeshArrayHandle_s{
  MeshArrayType_t   type;
  Reference     srcref;
  union{
    int       meshid;
    struct{
      void*       begin;
      int         num;
      enum VertexType_e vtype;
    }ptrs;
  };
}MeshArrayHandle_t;

Reference MeshArrayHandle_new(MeshArrayHandle_t **out,lxClassType type);

typedef int (*FnPubFunctionCall_t) (Reference fnref, int upparam1, void *upparam2, int stackargs);
typedef struct FnPubFunction_s {
  Reference ref;
  FnPubFunctionCall_t fn;
  int param1;
  void *param2;
} FnPubFunction_t;

int PubRenderflag_return(enum32 rflag);

int PubUserMesh(PState pstate, int from, struct Mesh_s **outmesh, lxRrendermesh *outuser);
int PubRenderMesh(PState pstate, int from, struct Mesh_s **outmesh, lxRrendermesh *outuser);

#endif /*PUBLUXI_CLASS_STRUCTS_H_*/
