// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../scene/scenetree.h"
#include "../common/reflib.h"
#include "../render/gl_list3d.h"


// Published here:
// LUXI_CLASS_SCENENODE

static int PubSceneNode_new (PState pstate,PubFunction_t *f, int n)
{
  static lxMatrix44SIMD matrix;

  SceneNode_t *s3d;
  uchar drawable;
  char *name;


  lxMatrix44Identity(matrix);
  drawable = LUX_TRUE;
  if (FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name)!=1)
    return FunctionPublish_returnError(pstate,"1 string required");
  if ((n==2 || n==5) && !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&drawable))
    return FunctionPublish_returnError(pstate,"1 boolean required");
  if ((n==4 || n==5) && 3!=FunctionPublish_getArgOffset(pstate,1+(n==5),3,FNPUB_TOMATRIXPOS(matrix)))
    return FunctionPublish_returnError(pstate,"3 floats required");

  s3d = SceneNode_new(drawable,name);

  SceneNode_setMatrix(s3d,matrix);

  Reference_makeVolatile(s3d->link.reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_SCENENODE,REF2VOID(s3d->link.reference));
}

static int PubSceneNode_getroot(PState pstate,PubFunction_t *f, int n)
{
  SceneNode_t* s3d = SceneTree_getRootNode();
  return FunctionPublish_returnType(pstate,LUXI_CLASS_SCENENODE,REF2VOID(s3d->link.reference));
}

static int PubSceneNode_free(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  SceneNode_t *s3d;

  if (n<1 || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SCENENODE,(void*)&ref) || !Reference_get(ref,s3d)))
    return 0;

  Reference_free(ref);//RSceneNode_free(ref);

  return 0;
}

static int PubSceneNode_link (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  SceneNode_t *s3d,*s3d2;

  if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SCENENODE,(void*)&ref) || !Reference_get(ref,s3d))
    return FunctionPublish_returnError(pstate,"1 scenenode required");

  s3d2 = NULL;
  if (n==1){
    return s3d->parent ? FunctionPublish_returnType(pstate,LUXI_CLASS_SCENENODE,REF2VOID(s3d->parent->link.reference)) : 0;
  }
  else if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SCENENODE,(void*)&ref) && !Reference_get(ref,s3d2))
    return FunctionPublish_returnError(pstate,"2 scenenodes required");

  SceneNode_link(s3d,s3d2);

  return 0;
}

static int PubSceneNode_matrix(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  float *mat;
  SceneNode_t *s3d;

  if (n< 1 || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref) || !Reference_get(ref,s3d)))
    return FunctionPublish_returnError(pstate,"1 scenenode required");

  if (n==1)
    return PubMatrix4x4_return (pstate,s3d->localMatrix);
  else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,mat))
    return FunctionPublish_returnError(pstate,"1 matrix4x4 required");

  SceneNode_setMatrix(s3d,mat);

  return 0;
}

static int PubSceneNode_finalmatrix(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  SceneNode_t *s3d;

  if (n< 1 || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SCENENODE,(void*)&ref) || !Reference_get(ref,s3d)))
    return FunctionPublish_returnError(pstate,"1 scenenode required");

  return PubMatrix4x4_return (pstate,s3d->link.matrix);
}

static int PubSceneNode_localPos(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  SceneNode_t *s3d;

  if ((n!= 1 && n!=4) || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SCENENODE,(void*)&ref) || !Reference_get(ref,s3d)))
    return FunctionPublish_returnError(pstate,"1 scenenode [3 floats] required");

  if (s3d == NULL) return 0;
  switch(n) {
  case 1:
    return FunctionPublish_setRet(pstate,3,FNPUB_FFLOAT(s3d->localMatrix[12]),FNPUB_FFLOAT(s3d->localMatrix[13]),FNPUB_FFLOAT(s3d->localMatrix[14]));
  case 4:
    if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOMATRIXPOS(s3d->localMatrix)) != 3)
      return FunctionPublish_returnError(pstate,"1 scenenode [3 floats] required");

    SceneNode_setMatrix(s3d,NULL);
    return 0;
  }
  return 0;
}

static int PubSceneNode_localRot(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  SceneNode_t *s3d;

  lxVector4 vector;
  float *x,*y,*z;

  if ((n<1) || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SCENENODE,(void*)&ref) || !Reference_get(ref,s3d)))
    return FunctionPublish_returnError(pstate,"1 scenenode required");

  switch((int)f->upvalue) {
  case ROT_AXIS:
    if (n<2){
      x = y = z = s3d->localMatrix;
      y+=4;
      z+=8;
      return FunctionPublish_setRet(pstate,9, FNPUB_FROMVECTOR3(x)
        ,FNPUB_FROMVECTOR3(y)
        ,FNPUB_FROMVECTOR3(z));
    }
    else{
      x = y = z = s3d->localMatrix;
      y+=4;
      z+=8;
      if (FunctionPublish_getArgOffset(pstate,1,9,FNPUB_TOVECTOR3(x),
        FNPUB_TOVECTOR3(y),
        FNPUB_TOVECTOR3(z)) != 9)
        return FunctionPublish_returnError(pstate,"9 floats required");
    }
    break;
  case ROT_QUAT:
    if (n<2){
      lxQuatFromMatrix(vector,s3d->localMatrix);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(vector))  < 4)
        return FunctionPublish_returnError(pstate,"4 numbers required");
      lxQuatToMatrix(vector,s3d->localMatrix);
    }
    break;
  default:
  case ROT_RAD:
    if (n<2){
      lxMatrix44ToEulerZYX(s3d->localMatrix,vector);
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(vector)) != 3)
        return FunctionPublish_returnError(pstate,"3 numbers required");
      lxMatrix44FromEulerZYXFast(s3d->localMatrix,vector);
    }
    break;
  case ROT_DEG:
    if (n<2){
      lxMatrix44ToEulerZYX(s3d->localMatrix,vector);
      lxVector3Rad2Deg(vector,vector);
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(vector)) != 3)
        return FunctionPublish_returnError(pstate,"3 numbers required");
      lxMatrix44FromEulerZYXdeg(s3d->localMatrix,vector);
    }
    break;

  }

  SceneNode_setMatrix(s3d,NULL);

  return 0;
}


static int PubSceneNode_finalPos(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  SceneNode_t *s3d;

  if (n!= 1 || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SCENENODE,(void*)&ref) || !Reference_get(ref,s3d)))
    return FunctionPublish_returnError(pstate,"1 scenenode required");

  return FunctionPublish_setRet(pstate,3,FNPUB_FFLOAT(s3d->link.matrix[12]),FNPUB_FFLOAT(s3d->link.matrix[13]),FNPUB_FFLOAT(s3d->link.matrix[14]));
}

static int PubSceneNode_finalRot(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  SceneNode_t *s3d;
  lxVector4 vector;
  float *x,*y,*z;

  if (n!= 1 || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SCENENODE,(void*)&ref) || !Reference_get(ref,s3d)))
    return FunctionPublish_returnError(pstate,"1 scenenode required");

  switch((int)f->upvalue) {
  case ROT_AXIS:
    x = y = z = s3d->link.matrix;
    y+=4;
    z+=8;
    return FunctionPublish_setRet(pstate,9, FNPUB_FROMVECTOR3(x)
      ,FNPUB_FROMVECTOR3(y)
      ,FNPUB_FROMVECTOR3(z));
  case ROT_QUAT:
    lxQuatFromMatrix(vector,s3d->link.matrix);
    return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vector));
  default:
  case ROT_RAD:
    lxMatrix44ToEulerZYX(s3d->link.matrix,vector);
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector));
  case ROT_DEG:
    lxMatrix44ToEulerZYX(s3d->link.matrix,vector);
    lxVector3Rad2Deg(vector,vector);
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector));

  }

  return 0;
}


static int PubSceneNode_transform(PState pstate,PubFunction_t *fn, int n)
{
  SceneNode_t *s3d;
  Reference ref;
  float f[3];
  float out[3];

  FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_SCENENODE,ref,s3d);
  FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_FLOAT,f[0]);
  FNPUB_CHECKOUT(pstate,n,2,LUXI_CLASS_FLOAT,f[1]);
  FNPUB_CHECKOUT(pstate,n,3,LUXI_CLASS_FLOAT,f[2]);

  lxVector3Transform(out,f,s3d->link.matrix);

  return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(out));
}

static int PubSceneNode_vistest(PState pstate,PubFunction_t *f, int n) {
  SceneNode_t *self;
  Reference ref;
  lxVector3 min,max;

  if (n < 1 || FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SCENENODE,(void*)&ref) != 1 || !Reference_get(ref,self)
    || !self->link.visobject)
    return FunctionPublish_returnError(pstate,"1 drawable scenenode required");

  if (n==1){
    return FunctionPublish_setRet(pstate,6,FNPUB_FROMVECTOR3(self->link.visobject->bbox.min),FNPUB_FROMVECTOR3(self->link.visobject->bbox.max));
  }
  else if (6!=FunctionPublish_getArgOffset(pstate,1,6,FNPUB_TOVECTOR3(min),FNPUB_TOVECTOR3(max)))
    return FunctionPublish_returnError(pstate,"1 scenenode 6 floats required");
  VisObject_setBBoxForce(self->link.visobject,min,max);
  return 0;
}

static int PubSceneNode_updateall(PState pstate,PubFunction_t *fn, int n)
{
  SceneTree_run();
  return 0;
}

int PubLinkObject_return(PState pstate, Reference linkref)
{
  if (linkref && Reference_isValid(linkref)){
    return FunctionPublish_returnType(pstate,Reference_type(linkref),linkref);
  }
  else return 0;
}

void PubClass_SceneTree_init()
{
  FunctionPublish_initClass(LUXI_CLASS_SPATIALNODE,"spatialnode",
    "Spatialnodes contain position/rotation information in the world, and can contain data used for visibility culling. There is two kinds of nodes dynamic actornodes and static scenenodes.\
    The purpose of scenenodes is to provide positional information in space without \
    information about visual appearance or collision behavior. The matrices of the scenenode \
    can be linked against l3dnodes in order to display objects on the screen.<br>\
    Each spatialnode has a name and can be found by using the name. Spatialnodes with the same name are \
    grouped in a ring list that can be traversed.",NULL,LUX_TRUE);

  FunctionPublish_initClass(LUXI_CLASS_SCENENODE,"scenenode",
    "The SceneTree is a hierarchical SceneGraph organised as tree, containing scenenodes. \
    The SceneTree is optimized for static data, for dynamic\
    nodes better use actornodes, however the scenenodes can be changed in position and rotation just fine as well.<br>\
    Retrieving world data can return wrong results, you need to wait one frame until they are uptodate or enforce a full \
    tree updateall.<br>Compared to the other visual node types, scenenodes are automatically linked to the rootnode on creation, however this won't prevent garbagecollection."
    ,NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_SCENENODE,LUXI_CLASS_SPATIALNODE);


  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_new,NULL,"new",
    "([scenenode s3d]):(string name,[boolean drawable],[float x,y,z]) - creates a new scenenode with given position as local pos. By default has root as parent.");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_updateall,NULL,"updateall",
    "():() - done automatically before rendering is done, but if you need uptodate world data, right after setting your scenenodes, call this function.");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_transform,NULL,"transform",
    "(float x,y,z):(scenenode self,float x,y,z) - transforms x,y,z with final matrix of the node into world coordinates");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_getroot,(void*)NULL,"getroot",
    "(scenenode s3d):() - returns the root node. The root node can't be deleted nor modified");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_free,NULL,"delete",
    "():(scenenode) - removes the Node from the SceneTree and deletes it. withchildren defaults to false");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_link,NULL,"parent",
    "([scenenode parent]):(scenenode s3d,[scenenode parent]) - return or sets parent. If parent is not a scenenode, the node becomes unlinked. ==parent will prevent gc of self, unless parent is root.");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_localPos,NULL,"localpos",
    "(float x,y,z):(scenenode s3d,[float x,y,z]) - updates node's local position or returns it");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_localRot,(void*)ROT_RAD,"localrotrad",
    "(float x,y,z):(scenenode s3d,[float x,y,z]) - updates node's local rotation or returns it in radians");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_localRot,(void*)ROT_DEG,"localrotdeg",
    "(float x,y,z):(scenenode s3d,[float x,y,z]) - updates node's local rotation or returns it in degrees");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_localRot,(void*)ROT_QUAT,"localrotquat",
    "(float x,y,z,w):(scenenode s3d,[float x,y,z,w]) - updates node's local rotation or returns it as quaternion");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_localRot,(void*)ROT_AXIS,"localrotaxis",
    "([float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]):(scenenode s3d,[float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]) - returns or sets local rotation axis, make sure they make a orthogonal basis.");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_matrix,NULL,"localmatrix",
    "([matrix4x4]):(scenenode s3d,[matrix4x4]) - updates node's local matrix or returns it");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_finalmatrix,NULL,"worldmatrix",
    "([matrix4x4]):(scenenode s3d) - returns node's world matrix");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_finalPos,NULL,"worldpos",
    "(float x,y,z):(scenenode s3d) - returns world position");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_finalRot,(void*)ROT_DEG,"worldrotdeg",
    "(float x,y,z):(scenenode s3d) - returns world rotation (degrees)");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_finalRot,(void*)ROT_RAD,"worldrotrad",
    "(float x,y,z):(scenenode s3d) - returns world rotation (radians)");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_finalRot,(void*)ROT_AXIS,"worldrotaxis",
    "([float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]):(scenenode s3d,[float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]) - returns or sets last world rotation axis, make sure they make a orthogonal basis.");
  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_finalRot,(void*)ROT_QUAT,"worldrotquat",
    "(float x,y,z,w):(scenenode s3d,[float x,y,z,w]) - returns world rotation as quaternion");

  FunctionPublish_addFunction(LUXI_CLASS_SCENENODE,PubSceneNode_vistest,(void*)0,"vistestbbox",
    "([float minX,minY,minZ,maxX,maxY,maxZ]):(scenenode self, [float minX,minY,minZ,maxX,maxY,maxZ]) - sets/gets visibility testing boundingbox when scenenode is drawable ");


}
