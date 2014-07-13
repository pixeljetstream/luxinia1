// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


// Q: Why are the name of variables (pgeom, pjoint, etc.) different from
//    the published names (dgeom,djoint, etc.)?
// A: The API changed during development. The p means physical, but as more and
//    more ODE-API calls were directly used, it appeared better to me, to use
//    the native names of ode for the sake of clarity. First this is simpler to
//    understand for somebody who knows ode, second it makes clear that we are
//    using pure ode and that we don't want to look like that we pretend that
//    we made this stuff

#include <luxinia/luxcore/contmacrolinkedlist.h>

#include "fnpublish.h"
#include "../main/main.h"
#include "../common/reflib.h"
#include "../render/gl_camlight.h"
#include "../scene/scenetree.h"
#include "../scene/actorlist.h"
#include "../common/3dmath.h"
#include "../common/interfaceobjects.h"
#include "../common/memorymanager.h"
#include "../common/timer.h"
#include "../render/gl_render.h"
#include "pubclass_collision.h"


#include <ode/ode.h>


enum WORLDPROP_UPV_e {
  WP_GRAVITY,
  WP_RANDOMSEED,
  WP_ERP,
  WP_CFM,
  WP_STEP,
  WP_QUICKSTEP,
  WP_QUICKSTEPITER,
  WP_GET,
  WP_COUNT,
  WP_COLBUFSIZE,
  WP_COLLISIONCOUNT,
  WP_COLBUFGET,
  WP_COLLIDE,
  WP_JOINTEMPTY,
  WP_CONTACTCOUNT,
  WP_MAKECONTACTS,
  WP_AUTODISABLE,
  WP_AUTODISABLELINEARTHRESHOLD,
  WP_AUTODISABLEANGULARTHRESHOLD,
  WP_AUTODISABLESTEPS,
  WP_AUTODISABLETIME,
  WP_SURFACELAYER,
  WP_MAXCORVEL,
  WP_DINFINITY,

  WP_MATERIALCOMB,

  WP_MU,
  WP_MU2,
  WP_BOUNCE,
  WP_BOUNCEVEL,
  WP_SOFTERP,
  WP_SOFTCFM,
  WP_MOTION1,
  WP_MOTION2,
  WP_SLIP1,
  WP_SLIP2,

  WP_FDIRMIXMODE,
  WP_FDIID,

  WP_BITMU2,
  WP_BITBOUNCE,
  WP_BITSOFTERP,
  WP_BITSOFTCFM,
  WP_BITMOTION1,
  WP_BITMOTION2,
  WP_BITSLIP1,
  WP_BITSLIP2,
  WP_BITAPPROX1,
  WP_BITAPPROX2,
  WP_BITFDIR,

  WP_ACTIVEBODIES,
  WP_GETACTIVEBODY,
};

typedef enum SurfaceMixMode_e {
  SFM_FDIRUSEHIGHEID,
  SFM_FDIRUSELOWEID,
  SFM_FDIRUSEHIGHERPRIO,
  SFM_FDIRUSELOWERPRIO,
  SFM_FDIRUSEID,
  SFM_AVERAGEFDIR,
  SFM_MIXPRIOBASED,

  SFM_FDIRUSEHIGHEIDWORLD,
  SFM_FDIRUSELOWEIDWORLD,
  SFM_FDIRUSEHIGHERPRIOWORLD,
  SFM_FDIRUSELOWERPRIOWORLD,
  SFM_FDIRUSEIDWORLD,
  SFM_AVERAGEFDIRWORLD,
  SFM_MIXPRIOBASEDWORLD,
} SurfaceMixMode_t;

typedef struct SurfaceParameters_s {
  SurfaceMixMode_t mode;
  int id;
  dSurfaceParameters param;
} SurfaceParameters_t;

static SurfaceParameters_t l_materialdata[0xff];
static unsigned char l_materialcombinations[0xffff];


static int l_contactcount=0;

static dWorldID l_world = NULL;
static Reference  l_worldref = NULL;
static Reference  l_baseref = NULL;
static PhysicsObject_t *l_pobjectList = NULL;
static PhysicsObject_t *l_spaceList = NULL;
static PhysicsObject_t *l_bodyList = NULL, *l_enabledbodies = NULL;
static PhysicsObject_t *l_contactjoints = NULL;
static int l_cachedate = 1,l_activeBodyCount;
static ulong l_steptime = 0;
static float l_timestep = 0;
static int l_contactflag = 0; // increment counter for contactinfo setting (don't need a global reset for all geoms)

/*
typedef struct GeomData_s {
  Reference ref;
} GeomData_t;
*/

#define GETODEDATA(pobj,obj,fn) if (!Reference_get( ((Reference)fn(obj)),pobj))

typedef struct CollisionBuffer_s {
  dGeomID *buffer;
  int collisions,size;
} CollisionBuffer_t;

typedef struct CollideNode_s {
  dContact contact;
  dBodyID a,b;

} CollideNode_t;


CollisionBuffer_t l_collisions = {NULL,0,0};

///////////////////////////////////////////////////////////////////////////////
// Matrix44 interface functions

static float* ODE_getElements (void *data, float *f)
{
  PhysicsObject_t *obj;
  const dReal *pos, *rot;
  PhysicsObject_t *tobj;
  int b;


  obj = (PhysicsObject_t*) data;
  if (obj->type == PHT_GEOM && obj->transformer) {
    GETODEDATA(tobj,obj->transformer,dGeomGetData) tobj=NULL;
    b = tobj!=NULL;
  } else {
    tobj = NULL;
    if (obj->type==PHT_GEOM)
      b =(dGeomGetBody(obj->geom)!=NULL);
    else b = 1;
  }

  if (obj->positionchange||
    (obj->type == PHT_GEOM && b && obj->cachedate!=l_cachedate) ||
    (obj->type==PHT_BODY && obj->cachedate!=l_cachedate)) {
    switch (obj->type) {
      default: return NULL;
      case PHT_BODY:
        pos = dBodyGetPosition(obj->body);
        rot = dBodyGetRotation(obj->body);
      break;
      case PHT_GEOM:
        pos = dGeomGetPosition(obj->geom);
        rot = dGeomGetRotation(obj->geom);
        obj->positionchange = 0;
      break;
    }
    f[0] = (float)rot[0]; f[4] = (float)rot[1]; f[8 ] = (float)rot[2 ]; f[12] = (float)pos[0];
    f[1] = (float)rot[4]; f[5] = (float)rot[5]; f[9 ] = (float)rot[6 ]; f[13] = (float)pos[1];
    f[2] = (float)rot[8]; f[6] = (float)rot[9]; f[10] = (float)rot[10]; f[14] = (float)pos[2];
    f[3] = 0; f[7] = 0; f[11] = 0; f[15] = 1;
    //MatrixTransposeRot1(f);


    if (tobj!=NULL) {
      static lxMatrix44SIMD transf;

      float *t = transf;
      //PhysicsObject_t *o;
      //tobj = (PhysicsObject_t*)Reference_get((Reference)dGeomGetData(obj->transformer));
      t = Matrix44Interface_getElements(tobj->link.matrixIF,t);
      lxMatrix44Multiply1SIMD(t,f);
      lxMatrix44CopySIMD(f,t);
      lxMatrix44CopySIMD(obj->cache,f);
    }
    obj->cachedate = l_cachedate;

    lxMatrix44CopySIMD(obj->cache,f);
  } else {
    lxMatrix44CopySIMD(f,obj->cache);
  }
  return f;
}
static void   ODE_setElements (void *data, float *f)
{
  static PhysicsObject_t *obj;
  static dReal pos[3], rot[12];

  obj = (PhysicsObject_t*) data;

  rot[0] = (dReal)f[0]; rot[4] = (dReal)f[4]; rot[8 ] = (dReal)f[8 ]; pos[0] = (dReal)f[12];
  rot[1] = (dReal)f[1]; rot[5] = (dReal)f[5]; rot[9 ] = (dReal)f[9 ]; pos[1] = (dReal)f[13];
  rot[2] = (dReal)f[2]; rot[6] = (dReal)f[6]; rot[10] = (dReal)f[10]; pos[2] = (dReal)f[14];
  switch (obj->type) {
    default: return;
    case PHT_BODY:
      dBodySetPosition(obj->body,pos[0],pos[1],pos[2]);
      dBodySetRotation(obj->body,rot);
    break;
    case PHT_GEOM:
      obj->positionchange = 1;
      dGeomSetPosition(obj->geom,pos[0],pos[1],pos[2]);
      dGeomSetRotation(obj->geom,rot);
    break;
  }
}
static void ODE_fnFree    (void *data){}


///////////////////////////////////////////////////////////////////////////////
// stuff

static void PhysicsObject_jointGroupEmpty(PhysicsObject_t *self)
{
  if (self->classtype!=LUXI_CLASS_PJOINTGROUP) return;
  //LUX_PRINTF("freeing\n");
  while (self->ojointgrp.jointlist) {
  //  LUX_PRINTF("%p - %p\n",self->jointlist,self->jointlist->prevjoint);
    //if (!Reference_free(self->ojointgrp.jointlist->ojoint.prevjoint->ref)) {
    //  LUX_PRINTF("the reference is invalid: %p\n",self->ojointgrp.jointlist->ojoint.prevjoint->ref);
    //  LUX_ASSERT(0);
    //}
    /*
    PhysicsObject_t *joint;
    lxLN_popFront(self->ojointgrp.jointlist,joint,ojoint.nextjoint,ojoint.prevjoint);
    joint->ojoint.jointowner = NULL;
    joint->ojoint.joint = NULL; // deleted by group

    //Reference_releaseClear(joint->ref);
    */
    Reference lref = self->ojointgrp.jointlist->ojoint.prevjoint->ref;
    Reference_free(lref);
    Reference_release(lref);
  }
  //LUX_PRINTF("empty\n");
  dJointGroupEmpty(self->ojointgrp.jointgroup);
  l_contactcount=0;
}

static void PhysicsObject_disableBody(PhysicsObject_t *self)
{
  static PhysicsObject_t *browse,*space;
  LUX_ASSERT(self->type==PHT_BODY);
  if (!self->nextEnabledBody) return;
  dBodyDisable(self->body);
  lxLN_remove(l_enabledbodies,self,nextEnabledBody,prevEnabledBody);
  self->nextEnabledBody = NULL;
  self->prevEnabledBody = NULL;
  browse = self->geombodylist;
  if (browse) do {
    if (Reference_get(browse->disabledSpace,space)) {
      if (dGeomGetSpace(browse->geom)) dSpaceRemove(dGeomGetSpace(browse->geom),browse->geom);
  //    LUX_PRINTF("%p %p\n",space,space->space);
      dSpaceAdd(space->ospace.space,browse->geom);
//      LUX_PRINTF("reput\n");
    }
    browse = browse->nextGeomInBody;
  } while (browse!=self->geombodylist);
}

static void PhysicsObject_enableBody (PhysicsObject_t *self)
{
  static PhysicsObject_t *browse,*space;
  LUX_ASSERT(self->type==PHT_BODY);
  if (self->nextEnabledBody) return;
  dBodyEnable(self->body);
  //lxLN_remove(l_enabledbodies,self,nextEnabledBody,prevEnabledBody);
  lxLN_init(self,nextEnabledBody,prevEnabledBody);
  lxLN_addLast(l_enabledbodies,self,nextEnabledBody,prevEnabledBody);
  browse = self->geombodylist;
  if (browse) do {
    if (Reference_get(browse->enabledSpace,space)) {
      if (dGeomGetSpace(browse->geom)) dSpaceRemove(dGeomGetSpace(browse->geom),browse->geom);
      dSpaceAdd(space->ospace.space,browse->geom);
    }
    browse = browse->nextGeomInBody;
  } while (browse!=self->geombodylist);
}

/*static void PhysicsObject_removeDisabledBodies ()
{
  static PhysicsObject_t *browse;
  browse = l_enabledbodies;
  l_activeBodyCount = 0;
  if (browse) {
    do {
      if (!dBodyIsEnabled(browse->body)) {
        browse = browse->nextEnabledBody;
        LUX_ASSERT(browse->type==PHT_BODY);
        PhysicsObject_disableBody(browse->prevEnabledBody);
        continue;
      }
      browse = browse->nextEnabledBody;
      l_activeBodyCount++;
    } while (browse!=l_enabledbodies && l_enabledbodies);
  }
}*/

static void PhysicsObject_jointGroupAdd(PhysicsObject_t *self,PhysicsObject_t *joint)
{
  if (self->classtype!=LUXI_CLASS_PJOINTGROUP) return;

  joint->ojoint.jointowner = self;

  Reference_ref(joint->ref);
  lxLN_addLast(self->ojointgrp.jointlist,joint,ojoint.nextjoint,ojoint.prevjoint);
}

static void PhysicsObject_free (Reference ref)
{
  PhysicsObject_t *self,*browse;

  void *ptr = Reference_value(ref);

  self = (PhysicsObject_t*)ptr;


  lxLN_remove(l_pobjectList,self,next,prev);
  if (FunctionPublish_isChildOf(self->classtype,LUXI_CLASS_PSPACE)) {
    lxLN_remove(l_spaceList,self,ospace.nextspace,ospace.prevspace);
    dSpaceDestroy(self->ospace.space);
  } else
  if (FunctionPublish_isChildOf(self->classtype,LUXI_CLASS_PJOINT)) {
    if (self->ojoint.joint && self->ojoint.activationforce>0 &&
      self->ojoint.activationforce<(lxVector3LengthFast(self->ojoint.jointfeedback.f1)+lxVector3LengthFast(self->ojoint.jointfeedback.f2)))
    {// check if the joint
      PhysicsObject_t *act,*enabler;
      if (Reference_get(self->ojoint.disabledBody,act) && Reference_get(self->ojoint.enablerBody,enabler) &&
        enabler->actdepth!=0)
      {
        float *f,*t, s=1.0f, *p;// (1-dWorldGetERP(l_world))/l_timestep*0.25;
        PhysicsObject_enableBody(act);
        act->minenableforce = act->nextminenableforce;
        if (enabler->actdepth>0) act->actdepth = enabler->actdepth-1;
        else act->actdepth = enabler->minactdepth;
        if (dJointGetBody(self->ojoint.joint,0)==act->body) {
            p = self->ojoint.contactpos;
          f = self->ojoint.jointfeedback.f1;  t = self->ojoint.jointfeedback.t1;
          //dBodyAddForceAtPos(act->body,-f[0]*s,-f[1]*s,-f[2]*s, p[0],p[1],p[2]);
          //dBodyAddTorque(act->body,-t[0]*s,-t[1]*s,-t[2]*s);

          f = self->ojoint.jointfeedback.f2;  t = self->ojoint.jointfeedback.t2;
          //dBodyAddForceAtPos(enabler->body,-f[0]*s,-f[1]*s,-f[2]*s, p[0],p[1],p[2]);
          //dBodyAddTorque(enabler->body,-t[0]*s,-t[1]*s,-t[2]*s);
        }
        else {
            p = self->ojoint.contactpos;
          f = self->ojoint.jointfeedback.f2;  t = self->ojoint.jointfeedback.t2;
          dBodyAddForceAtPos(act->body,-f[0],-f[1],-f[2], p[0],p[1],p[2]);
          //dBodyAddTorque(act->body,-t[0],-t[1],-t[2]);

          f = self->ojoint.jointfeedback.f1;  t = self->ojoint.jointfeedback.t1;
          dBodyAddForceAtPos(enabler->body,-f[0]*s,-f[1]*s,-f[2]*s, p[0],p[1],p[2]);
          //dBodyAddTorque(enabler->body,-t[0]*s,-t[1]*s,-t[2]*s);
        }
      }
    }
    if (self->ojoint.jointowner!=NULL)
      {lxLN_remove(self->ojoint.jointowner->ojointgrp.jointlist,self,ojoint.nextjoint,ojoint.prevjoint);}
    dJointDestroy(self->ojoint.joint);
  }
  else
  if (self->classtype==LUXI_CLASS_PBODY) {
    if (self->nextEnabledBody)
      lxLN_remove(l_enabledbodies,self,nextEnabledBody,prevEnabledBody);
    //LUX_PRINTF("bodykill %p\n",self);
    while (self->geombodylist) {
      //LUX_PRINTF("killing spree: %p %p\n",self,self->geombodylist);
      LUX_ASSERT(self==self->geombodylist->bodyowner);
      self->geombodylist->bodyowner = NULL;
      browse = self->geombodylist;
      dGeomSetBody(self->geombodylist->geom,NULL);
      lxLN_remove(self->geombodylist,browse,nextGeomInBody,prevGeomInBody);

    }
//    LUX_PRINTF("killing spree: %p %p\n",self,self->geombodylist);
    lxLN_remove(l_bodyList,self,nextbody,prevbody);
    dBodyDestroy(self->body);
  } else if (self->classtype == LUXI_CLASS_PJOINTGROUP) {
    PhysicsObject_jointGroupEmpty(self);
    dJointGroupDestroy(self->ojointgrp.jointgroup);
  } else if (self->classtype == LUXI_CLASS_PGEOMTRIMESHDATA) {
    dGeomTriMeshDataDestroy(self->otridata.meshdata);
    lxMemGenFree(self->otridata.vertices,sizeof(float)*self->otridata.verticecount*3);
    lxMemGenFree(self->otridata.indices,sizeof(int)*self->otridata.indicescount);
  }
  else {
//    LUX_PRINTF("freeing %s\n",FunctionPublish_getClassName(self->classtype));
    //LUX_PRINTF("geomkill %p\n",self);
    if (self->bodyowner!=NULL)
    {
      //LUX_PRINTF("killing spree2: %p %p %p %p %p %p\n",self,self->bodyowner,self->bodyowner->geombodylist,self->nextGeomInBody,self->prevGeomInBody,&self->nextGeomInBody);
      //LUX_ASSERT(self==self->bodyowner->geombodylist); // seems to be wrong
      //if (self==self->bodyowner->geombodylist) // member in list does not have to be head
        lxLN_remove(self->bodyowner->geombodylist,self,nextGeomInBody,prevGeomInBody);
      //LUX_PRINTF("val %p\n",self->bodyowner->geombodylist);
    }
    if (self->transformer!=NULL) {
      dGeomTransformSetGeom(self->transformer,NULL);
    }
    if (self->classtype == LUXI_CLASS_PGEOMTRANSFORM) {
      static dGeomID trans;
      trans = dGeomTransformGetGeom(self->id);
      if (trans) {
        static PhysicsObject_t *t;
        GETODEDATA(t,trans,dGeomGetData);
        else t->transformer = NULL;
      }
    }
    else if (self->classtype == LUXI_CLASS_PGEOMTRIMESH){
      Reference_release(self->userref);
    }
    dGeomDestroy(self->geom);//*/
  }
  //LUX_PRINTF("freeing end\n");

  Matrix44Interface_invalidate(self->link.matrixIF);
  genfreeSIMD(ptr,sizeof(PhysicsObject_t));
}

static PhysicsObject_t* PhysicsObject_new(PhysicsType_t type,void *id, lxClassType classtype)
{
  static Matrix44InterfaceDef_t def ={
      ODE_getElements, ODE_setElements,
      ODE_fnFree };

  PhysicsObject_t *self;

  self = genzallocSIMD(sizeof(PhysicsObject_t));
  self->link.matrixIF = Matrix44Interface_new(&def,(void*)self);
  self->ref = Reference_new(classtype,(void*)self);

  LUX_SIMDASSERT((size_t)((PhysicsObject_t*)self)->cache % 16 == 0);

  self->id = id;
  self->type = type;
  self->classtype = classtype;
  self->positionchange = 1;

  lxLN_init(self,next,prev);
  lxLN_addLast(l_pobjectList,self,next,prev);
  if (FunctionPublish_isChildOf(classtype,LUXI_CLASS_PSPACE)) {
    lxLN_init(self,ospace.nextspace,ospace.prevspace);
    lxLN_addLast(l_spaceList,self,ospace.nextspace,ospace.prevspace);
    self->type=PHT_SPACE;
    self->ospace.collidetest=2;
    dGeomSetData(id,(void*)self->ref);
  } else
  if (FunctionPublish_isChildOf(classtype,LUXI_CLASS_PJOINT)) {
    lxLN_init(self,ojoint.nextjoint,ojoint.prevjoint);
    self->ojoint.jointowner = NULL;
    Reference_reset(self->ojoint.disabledBody);
    Reference_reset(self->ojoint.enablerBody);
    dJointSetData(self->ojoint.joint,self->ref);
    dJointSetFeedback (self->ojoint.joint,&self->ojoint.jointfeedback);
  } else
  if (classtype == LUXI_CLASS_PJOINTGROUP) {
    self->ojointgrp.jointlist = NULL;
  } else
  if (classtype == LUXI_CLASS_PBODY) {
    self->type = PHT_BODY;
    self->plane = 0;
    self->planewidth = 0;
    self->veldrag[0] = 1;
    self->veldrag[1] = 1;
    self->veldrag[2] = 1;
    self->rotdrag[0] = 1;
    self->rotdrag[1] = 1;
    self->rotdrag[2] = 1;
    self->planeveldamp = 0;
    self->lock = -1;
    self->axislock = -1;
    self->affect = 0xffffff;
    self->reaffect = 0xffffff;
    dBodySetData(id,(void*)self->ref);
    lxLN_init(self,nextbody,prevbody);
    //lxLN_init(self,nextEnabledBody,prevEnabledBody);
    self->nextEnabledBody = NULL;
    self->prevEnabledBody = NULL;
    self->geombodylist = NULL;
    lxLN_addLast(l_bodyList,self,nextbody,prevbody);
    self->minactdepth=-1;
    self->actdepth=-1;
    PhysicsObject_enableBody(self);
    //lxLN_addLast(l_enabledbodies,self,nextEnabledBody,prevEnabledBody);
  } else if (FunctionPublish_isChildOf(classtype,LUXI_CLASS_PGEOM)) {
    dGeomSetData(self->geom,(void*)self->ref);
    self->material = 0;
    self->maxcontacts = 0;
    self->nocontactgenerator = 0;
    self->maxcontactinfo = 0;
    self->fdirnormal[0] = 1;
    self->fdirnormal[1] = 0;
    self->fdirnormal[2] = 0;
    self->fdirpriority = 1;
    Reference_reset(self->enabledSpace);
    Reference_reset(self->disabledSpace);
    lxLN_init(self,nextGeomInBody,prevGeomInBody);
    self->bodyowner = NULL;
  }
//  LUX_PRINTF("created %p %p\n",dGeomGetData(self->geom),gd);
  return self;
}

static int PubGeomTransform_geom(PState state,PubFunction_t *fn, int n)
{
  static Reference ref,refg;
  static PhysicsObject_t *transform,*geom;
  static dGeomID id;

  if (n<1 || n>2 || FunctionPublish_getArg(state,2,LUXI_CLASS_PGEOMTRANSFORM,(void*)&ref,
    LUXI_CLASS_PGEOM,(void*)&refg) < n)
    return FunctionPublish_returnError(state,
      "first argument has to be a dgeomtransform while arg2 must be a \
dgeom if it is given");

  if (!Reference_get(ref,transform))
    return FunctionPublish_returnError(state,"given dbody (arg1) is not valid");

  if (n==1) {
    id = dGeomTransformGetGeom(transform->geom);
    if (id) {
      GETODEDATA(geom,id,dGeomGetData)
        return FunctionPublish_returnError(state,"the transformed geom seems \
to have forgotten that it should exist in lua... how mysterious (report that)");
      return FunctionPublish_returnType(state,geom->classtype,REF2VOID(geom->ref));
    }
    return 0;
  }

  if (!Reference_get(refg,geom))
    return FunctionPublish_returnError(state,"given dgeom (arg2) is not valid");
  if (dGeomGetSpace(geom->geom))
    return FunctionPublish_returnError(state,"given dgeom (arg2) must not be in a space");
  if (geom->transformer)
    return FunctionPublish_returnError(state,"given dgeom (arg2) is already \
transformed by a dgeomtransform");

  dGeomTransformSetGeom(transform->geom,geom->geom);
  geom->transformer = transform->geom;
  return 0;
}

enum PJ_UPS_e {
  PJ_FEEDBACK,
  PJ_ATTACH,
  PJ_BODY1,
  PJ_BODY2,
  PJ_GEOM1,
  PJ_GEOM2,
  PJB_ANCHOR,
  PJH_ANCHOR,
  PJH_AXIS,
  PJH_ANGLE,
  PJS_AXIS,
  PJS_SLIDERPOS,
  PJF_FIX,

  PJH2_ANCHOR,
  PJH2_AXIS1,
  PJH2_AXIS2,
  PJH2_ANGLE1,
  PJH2_ANGLE2,
  PJH2_ANGLERATE1,
  PJH2_ANGLERATE2,

  PJU_ANCHOR,
  PJU_ANGLE1,
  PJU_ANGLE2,
  PJU_ANGLERATE1,
  PJU_ANGLERATE2,
  PJU_AXIS1,
  PJU_AXIS2,

  PJA_MOTORMODEUSER,
  PJA_AXES,
  PJA_AXIS,
  PJA_ANGLE,
  PJA_ANGLERATE,

  PJC_FDIR,
  PJC_MU1,
  PJC_MU2,
  PJC_BOUNCE,
  PJC_BOUNCEVEL,
  PJC_SOFTERP,
  PJC_SOFTCFM,
  PJC_MOTION1,
  PJC_MOTION2,
  PJC_SLIP1,
  PJC_SLIP2,
  PJC_APPROX1_1,
  PJC_APPROX1_2,
  PJC_NORMAL,
  PJC_DEPTH,
  PJC_POS,

  PJP_VEL,      PJP_VEL2,     PJP_VEL3,
  PJP_FMAX,     PJP_FMAX2,      PJP_FMAX3,
  PJP_FUDGEFAC,   PJP_FUDGEFAC2,    PJP_FUDGEFAC3,
  PJP_LOSTOP,     PJP_LOSTOP2,    PJP_LOSTOP3,
  PJP_HISTOP,     PJP_HISTOP2,    PJP_HISTOP3,
  PJP_BOUNCE,     PJP_BOUNCE2,    PJP_BOUNCE3,
  PJP_CFM,      PJP_CFM2,     PJP_CFM3,
  PJP_STOPERP,    PJP_STOPERP2,   PJP_STOPERP3,
  PJP_STOPCFM,    PJP_STOPCFM2,   PJP_STOPCFM3,
  PJP_SUSPENSIONERP,  PJP_SUSPENSIONERP2, PJP_SUSPENSIONERP3,
  PJP_SUSPENSIONCFM,  PJP_SUSPENSIONCFM2, PJP_SUSPENSIONCFM3
};

static int PubJoint_prop(PState state, PubFunction_t *fn, int n)
{
  static Reference ref;
  static PhysicsObject_t *joint,*body1,*body2;
  static float f[12];
  static dReal r[12];
  static int read,b,i;
  static dMass mass;
  static dBodyID body;

  if (n==0 || (read=FunctionPublish_getArg(state,9,LUXI_CLASS_PJOINT,(void*)&ref,
      FNPUB_TOVECTOR4(f),FNPUB_TOVECTOR4((&f[4])),FNPUB_TOVECTOR4((&f[8])))) < 1)
    return FunctionPublish_returnError(state,"first argument has to be a djoint");
  if (!Reference_get(ref,joint))
    return FunctionPublish_returnError(state,"given djoint (arg1) is not valid");

  switch ((int)fn->upvalue) {
    default: return 0;
    case PJ_GEOM1:
      if (!Reference_get(joint->ojoint.geom1,body1)) return 0;
      return FunctionPublish_returnType(state,body1->classtype,REF2VOID(body1->ref));
    case PJ_GEOM2:
      if (!Reference_get(joint->ojoint.geom2,body1)) return 0;
      return FunctionPublish_returnType(state,body1->classtype,REF2VOID(body1->ref));
    case PJ_FEEDBACK:
    {
      static float feedback[12];
      for (i=0;i<3;i++) {
        feedback[i] = (float)joint->ojoint.jointfeedback.f1[i];
        feedback[i+3] = (float)joint->ojoint.jointfeedback.t1[i];
        feedback[i+6] = (float)joint->ojoint.jointfeedback.f2[i];
        feedback[i+9] = (float)joint->ojoint.jointfeedback.t2[i];
      }
      if (n==2 && read>=2) {
        i = (int)f[0];
        i = (i<0)?0:(i>0)?6:0;
        return FunctionPublish_setRet(state,6,
          FNPUB_FROMVECTOR3((&feedback[0+i])),
          FNPUB_FROMVECTOR3((&feedback[3+i])));
      }
      return FunctionPublish_setRet(state,12,
        FNPUB_FROMVECTOR3(feedback),
        FNPUB_FROMVECTOR3((&feedback[3])),
        FNPUB_FROMVECTOR3((&feedback[6])),
        FNPUB_FROMVECTOR3((&feedback[9])));
    }
    case PJ_ATTACH:
      if (n==1) {
        dJointAttach(joint->ojoint.joint,NULL,NULL);
        return 0;
      }
      if (FunctionPublish_getNArg(state,1,LUXI_CLASS_PBODY,(void*)&ref)!=1)
        return FunctionPublish_returnError(state,"arg2 must be dbody");

      if (!Reference_get(ref,body1))
        return FunctionPublish_returnError(state,"arg2 must is not valid");
      if (n==2) {
        dJointAttach(joint->ojoint.joint,body1->body,NULL);
        return 0;
      }
      if (FunctionPublish_getNArg(state,2,LUXI_CLASS_PBODY,(void*)&ref)!=1)
        return FunctionPublish_returnError(state,"arg3 must be dbody");

      if (!Reference_get(ref,body2))
        return FunctionPublish_returnError(state,"arg3 must is not valid");
      if (n==3) {
        dJointAttach(joint->ojoint.joint,body1->body,body2->body);
        return 0;
      }
    break;
    case PJ_BODY1:
      body = dJointGetBody(joint->ojoint.joint,0);
      if (body==NULL) return 0;
      GETODEDATA(body1,body,dBodyGetData)
        return FunctionPublish_returnError(state,"oh no! the body is invalid (report that)");
      return FunctionPublish_returnType(state,body1->classtype,REF2VOID(body1->ref));
    case PJ_BODY2:
      body = dJointGetBody(joint->ojoint.joint,1);
      if (body==NULL) return 0;
      GETODEDATA(body1,body,dBodyGetData)
        return FunctionPublish_returnError(state,"oh no! the body is invalid (report that)");
      return FunctionPublish_returnType(state,body1->classtype,REF2VOID(body1->ref));
    break;
    case PJB_ANCHOR:
      if(joint->classtype!=LUXI_CLASS_PJOINTBALL)
        return FunctionPublish_returnError(state,"given joint is not a balljoint");
      if (n==4 && read>=4) {
        dJointSetBallAnchor(joint->ojoint.joint,f[0],f[1],f[2]);
        return 0;
      }
      if (n==1) {
        static dReal vec[3];
        dJointGetBallAnchor(joint->ojoint.joint,vec);
        f[0] = (float)vec[0];f[1] = (float)vec[1];f[2] = (float)vec[2];
        dJointGetBallAnchor2(joint->ojoint.joint,vec);
        f[3] = (float)vec[0];f[4] = (float)vec[1];f[5] = (float)vec[2];
        return FunctionPublish_setRet(state,6,FNPUB_FROMVECTOR3(f),FNPUB_FROMVECTOR3((&f[3])));
      }
      return FunctionPublish_returnError(state,"either 0 or 3 floats must be passed");
    break;

  /// Hinge
    case PJH_ANCHOR:
      if(joint->classtype!=LUXI_CLASS_PJOINTHINGE)
        return FunctionPublish_returnError(state,"given joint is not a hingejoint");
      if (n==4 && read>=4) {
        dJointSetHingeAnchor(joint->ojoint.joint,f[0],f[1],f[2]);
        return 0;
      }
      if (n==1) {
        static dReal vec[3];
        dJointGetHingeAnchor(joint->ojoint.joint,vec);
        f[0] = (float)vec[0];f[1] = (float)vec[1];f[2] = (float)vec[2];
        dJointGetHingeAnchor2(joint->ojoint.joint,vec);
        f[3] = (float)vec[0];f[4] = (float)vec[1];f[5] = (float)vec[2];
        return FunctionPublish_setRet(state,6,FNPUB_FROMVECTOR3(f),FNPUB_FROMVECTOR3((&f[3])));
      }
      return FunctionPublish_returnError(state,"either 0 or 3 floats must be passed");
    case PJH_AXIS:
      if(joint->classtype!=LUXI_CLASS_PJOINTHINGE)
        return FunctionPublish_returnError(state,"given joint is not a hingejoint");
      if (n==4 && read>=4) {
        dJointSetHingeAxis(joint->ojoint.joint,f[0],f[1],f[2]);
        return 0;
      }
      if (n==0) {
        static dReal vec[3];
        dJointGetHingeAxis(joint->ojoint.joint,vec);
        f[0] = (float)vec[0];f[1] = (float)vec[1];f[2] = (float)vec[2];
        return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(f));
      }
      return FunctionPublish_returnError(state,"either 0 or 3 floats must be passed");
    case PJH_ANGLE:
      if(joint->classtype!=LUXI_CLASS_PJOINTHINGE)
        return FunctionPublish_returnError(state,"given joint is not a hingejoint");
      f[0] = (float)dJointGetHingeAngle(joint->ojoint.joint);
      f[1] = (float)dJointGetHingeAngleRate(joint->ojoint.joint);
      return FunctionPublish_setRet(state,2,FNPUB_FROMVECTOR2(f));

  /// fixed
    case PJF_FIX:
      if(joint->classtype!=LUXI_CLASS_PJOINTFIXED)
        return FunctionPublish_returnError(state,"given joint is not a fixedjoint");
      dJointSetFixed(joint->ojoint.joint);
      return 0;

  /// slider
    case PJS_AXIS:
      if(joint->classtype!=LUXI_CLASS_PJOINTSLIDER)
        return FunctionPublish_returnError(state,"given joint is not a sliderjoint");
      if (n==4 && read>=4) {
        dJointSetSliderAxis(joint->ojoint.joint,f[0],f[1],f[2]);
        return 0;
      }
      if (n==1 && read==1) {
        static dReal vec[3];
        dJointGetSliderAxis(joint->ojoint.joint,vec);
        f[0] = (float)vec[0];f[1] = (float)vec[1];f[2] = (float)vec[2];
        return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(f));
      }
      return FunctionPublish_returnError(state,"either 0 or 3 floats must be passed");
    case PJS_SLIDERPOS:
      if(joint->classtype!=LUXI_CLASS_PJOINTSLIDER)
        return FunctionPublish_returnError(state,"given joint is not a hingejoint");
      f[0] = (float)dJointGetSliderPosition(joint->ojoint.joint);
      f[1] = (float)dJointGetSliderPositionRate(joint->ojoint.joint);
      return FunctionPublish_setRet(state,2,FNPUB_FROMVECTOR2(f));

  /// Amotor
    case PJA_MOTORMODEUSER:
      if(joint->classtype!=LUXI_CLASS_PJOINTAMOTOR)
        return FunctionPublish_returnError(state,"given joint is not a amotorjoint");
      if (n==1)
        return FunctionPublish_returnBool(state,dJointGetAMotorMode(joint->ojoint.joint)==dAMotorUser);
      if (!(n==2 && read>=2))
        return FunctionPublish_returnError(state,"dgeomamotor and optional boolean required");
      dJointSetAMotorMode(joint->ojoint.joint,(f[0]!=0)?dAMotorUser:dAMotorEuler);
      return 0;

    case PJA_AXES:
      if(joint->classtype!=LUXI_CLASS_PJOINTAMOTOR)
        return FunctionPublish_returnError(state,"given joint is not a amotorjoint");
      if (n==1)
        return FunctionPublish_returnInt(state,dJointGetAMotorNumAxes(joint->ojoint.joint));
      if (!(n==2 && read>=2))
        return FunctionPublish_returnError(state,"djointamotor and optional int required");
      dJointSetAMotorNumAxes(joint->ojoint.joint,(int)f[0]);
      break;
    case PJA_AXIS:
      if(joint->classtype!=LUXI_CLASS_PJOINTAMOTOR)
        return FunctionPublish_returnError(state,"given joint is not a amotorjoint");
      if (n==2 && read>=2) {
        f[0] = (f[0]<0)?0:(f[0]>3)?3:f[0];
        dJointGetAMotorAxis(joint->ojoint.joint,(int)f[0],r);
        f[0] = (float) dJointGetAMotorAxisRel(joint->ojoint.joint,(int)f[0]);
        f[1] = (float) r[0]; f[2] = (float)r[1]; f[3] = (float)r[2];
        return FunctionPublish_setRet(state,4,FNPUB_FROMVECTOR4(f));
      }
      if (n==6 && read>=6) {
        f[0] = (f[0]<0)?0:(f[0]>2)?2:f[0];
        f[1] = (f[1]<0)?0:(f[1]>2)?2:f[1];
        dJointSetAMotorAxis(joint->ojoint.joint,(int)f[0],(int)f[1],f[2],f[3],f[4]);
        return 0;
      }
      return FunctionPublish_returnError(state,"djointamotor and optional 2 ints + 3 floats required");

    case PJA_ANGLE:
      if(joint->classtype!=LUXI_CLASS_PJOINTAMOTOR)
        return FunctionPublish_returnError(state,"given joint is not a amotorjoint");
      if (n==2 && read>=2) {
        f[0] = (f[0]<0)?0:(f[0]>2)?2:f[0];
        return FunctionPublish_returnFloat(state,(float)dJointGetAMotorAngle(joint->ojoint.joint,(int)f[0]));
      }
      if (n==3 && read>=3) {
        f[0] = (f[0]<0)?0:(f[0]>2)?2:f[0];
        dJointSetAMotorAngle(joint->ojoint.joint,(int)f[0],f[1]);
        return 0;
      }
      return FunctionPublish_returnError(state,"djointamotor,int and optional float required");

    case PJA_ANGLERATE:
      if(joint->classtype!=LUXI_CLASS_PJOINTAMOTOR)
        return FunctionPublish_returnError(state,"given joint is not a amotorjoint");
      if (n==2 && read>=2) {
        f[0] = (f[0]<0)?0:(f[0]>2)?2:f[0];
        return FunctionPublish_returnFloat(state,(float)
          dJointGetAMotorAngleRate(joint->ojoint.joint,(int)f[0]));
      }
      return FunctionPublish_returnError(state,"djointamotor and int + float required");


    case PJH2_ANCHOR:
    case PJH2_AXIS1:
    case PJH2_AXIS2:
      if(joint->classtype!=LUXI_CLASS_PJOINTHINGE2)
        return FunctionPublish_returnError(state,"given joint is not a hinge2joint");
      if (n==4 && read>=4) {
        switch ((int)fn->upvalue) {
          default: break;
          case PJH2_ANCHOR:
            dJointSetHinge2Anchor(joint->ojoint.joint,f[0],f[1],f[2]); break;
          case PJH2_AXIS1:
            dJointSetHinge2Axis1(joint->ojoint.joint,f[0],f[1],f[2]); break;
          case PJH2_AXIS2:
            dJointSetHinge2Axis2(joint->ojoint.joint,f[0],f[1],f[2]); break;
        }
        return 0;
      }
      if (n==1) {
        static dReal vec[3];

        switch ((int)fn->upvalue) {
          case PJH2_ANCHOR:
            dJointGetHinge2Anchor(joint->ojoint.joint,vec);
            f[0] = (float)vec[0];f[1] = (float)vec[1];f[2] = (float)vec[2];
            dJointGetHinge2Anchor2(joint->ojoint.joint,vec);
            f[3] = (float)vec[0];f[4] = (float)vec[1];f[5] = (float)vec[2];
            return FunctionPublish_setRet(state,6,FNPUB_FROMVECTOR3(f),
              FNPUB_FROMVECTOR3((&f[3])));
          case PJH2_AXIS1:
            dJointGetHinge2Axis1(joint->ojoint.joint,vec); break;
          case PJH2_AXIS2:
            dJointGetHinge2Axis2(joint->ojoint.joint,vec); break;
          default: return 0;
        }
        f[0] = (float)vec[0];f[1] = (float)vec[1];f[2] = (float)vec[2];
        return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(f));
      }
      return FunctionPublish_returnError(state,"either 0 or 3 floats must be passed");
    case PJH2_ANGLE1:
      if(joint->classtype!=LUXI_CLASS_PJOINTHINGE2)
        return FunctionPublish_returnError(state,"given joint is not a hinge2joint");
      return FunctionPublish_returnFloat(state,(float)
          dJointGetHinge2Angle1(joint->ojoint.joint));
    case PJH2_ANGLE2:
      if(joint->classtype!=LUXI_CLASS_PJOINTHINGE2)
        return FunctionPublish_returnError(state,"given joint is not a hinge2joint");
      return FunctionPublish_returnFloat(state,(float)
          dJointGetHingeAngle(joint->ojoint.joint));
    case PJH2_ANGLERATE1:
      if(joint->classtype!=LUXI_CLASS_PJOINTHINGE2)
        return FunctionPublish_returnError(state,"given joint is not a hinge2joint");
      return FunctionPublish_returnFloat(state,(float)dJointGetHinge2Angle1Rate (joint->ojoint.joint));

    case PJH2_ANGLERATE2:
      if(joint->classtype!=LUXI_CLASS_PJOINTHINGE2)
        return FunctionPublish_returnError(state,"given joint is not a hinge2joint");
      return FunctionPublish_returnFloat(state,(float)dJointGetHinge2Angle2Rate (joint->ojoint.joint));

    case PJU_ANCHOR:
      if(joint->classtype!=LUXI_CLASS_PJOINTUNIVERSAL)
        return FunctionPublish_returnError(state,"given joint is not a universaljoint");
      if (n==1) {
        dJointGetUniversalAnchor(joint->ojoint.joint,r);
        dJointGetUniversalAnchor(joint->ojoint.joint,&r[3]);
        f[0] = (float)r[0]; f[1] = (float)r[1]; f[2] = (float)r[2];
        f[3] = (float)r[3]; f[4] = (float)r[4]; f[5] = (float)r[5];
        return FunctionPublish_setRet(state,6,FNPUB_FROMVECTOR3(f),FNPUB_FROMVECTOR3((&f[3])));
      }
    case PJU_ANGLE1:
    case PJU_ANGLE2:
    case PJU_ANGLERATE1:
    case PJU_ANGLERATE2:
    case PJU_AXIS1:
    case PJU_AXIS2:
      if(joint->classtype!=LUXI_CLASS_PJOINTUNIVERSAL)
        return FunctionPublish_returnError(state,"given joint is not a universaljoint");
      if (n==1) {
        switch((int)fn->upvalue) {
          case PJU_AXIS1: dJointGetUniversalAxis1(joint->ojoint.joint,r); break;
          case PJU_AXIS2: dJointGetUniversalAxis2(joint->ojoint.joint,r); break;
          case PJU_ANGLE1:
            return FunctionPublish_returnFloat(state,(float)dJointGetUniversalAngle1(joint->ojoint.joint));
          case PJU_ANGLE2:
            return FunctionPublish_returnFloat(state,(float)dJointGetUniversalAngle2(joint->ojoint.joint));
          case PJU_ANGLERATE1:
            return FunctionPublish_returnFloat(state,(float)dJointGetUniversalAngle1Rate(joint->ojoint.joint));
          case PJU_ANGLERATE2:
            return FunctionPublish_returnFloat(state,(float)dJointGetUniversalAngle2Rate(joint->ojoint.joint));

        }
        f[0] = (float)r[0]; f[1] = (float)r[1]; f[2] = (float)r[2];
        return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(f));
      }
      if (n==4 && read>=4) {
        switch ((int)fn->upvalue) {
          default: dJointSetUniversalAnchor(joint->ojoint.joint,f[0],f[1],f[2]); break;
          case PJU_AXIS1: dJointSetUniversalAxis1(joint->ojoint.joint,f[0],f[1],f[2]); break;
          case PJU_AXIS2: dJointSetUniversalAxis2(joint->ojoint.joint,f[0],f[1],f[2]); break;
        }
        return 0;
      }
      return FunctionPublish_returnError(state,"joint + 3 floats required");

    case PJC_NORMAL:
      if(joint->classtype!=LUXI_CLASS_PJOINTCONTACT)
        return FunctionPublish_returnError(state,"given joint is not a contactjoint");
      /*if (n==4 && read>=4) {
        dContact *con = (dContact*)joint->ojoint.joint;
        con->geom.normal[0]=joint->contactnormal[0]=f[0];
        con->geom.normal[1]=joint->contactnormal[1]=f[1];
        con->geom.normal[2]=joint->contactnormal[2]=f[2];

        return 0;
      }*/
      return FunctionPublish_setRet(state,3, FNPUB_FROMVECTOR3(joint->ojoint.contactnormal));
    case PJC_POS:
      if(joint->classtype!=LUXI_CLASS_PJOINTCONTACT)
        return FunctionPublish_returnError(state,"given joint is not a contactjoint");
      /*if (n==4 && read>=4) {
        dContact *con = (dContact*)((void*)joint->ojoint.joint+sizeof(int));
        con->geom.pos[0]=joint->contactpos[0]=f[0];
        con->geom.pos[1]=joint->contactpos[1]=f[1];
        con->geom.pos[2]=joint->contactpos[2]=f[2];
        return 0;
      }*/
      return FunctionPublish_setRet(state,3, FNPUB_FROMVECTOR3(joint->ojoint.contactpos));
    case PJC_DEPTH:
      if(joint->classtype!=LUXI_CLASS_PJOINTCONTACT)
        return FunctionPublish_returnError(state,"given joint is not a contactjoint");
      /*if (n==2 && read>=2) {
        dContact *con = (dContact*)joint->ojoint.joint;
        con->geom.depth=joint->contactdepth=f[0];

        return 0;
      }*/
      return FunctionPublish_returnFloat(state,joint->ojoint.contactdepth);
    case PJC_FDIR:
      if(joint->classtype!=LUXI_CLASS_PJOINTCONTACT)
        return FunctionPublish_returnError(state,"given joint is not a contactjoint");
      break;
    case PJC_MU1:
    case PJC_MU2:
    case PJC_BOUNCE:
    case PJC_BOUNCEVEL:
    case PJC_SOFTERP:
    case PJC_SOFTCFM:
    case PJC_MOTION1:
    case PJC_MOTION2:
    case PJC_SLIP1:
    case PJC_SLIP2:
    case PJC_APPROX1_1:
    case PJC_APPROX1_2:
      if(joint->classtype!=LUXI_CLASS_PJOINTCONTACT)
        return FunctionPublish_returnError(state,"given joint is not a contactjoint");
    {
      static dReal *values;
// TODO: fix this
      return 0;
    }

    case PJP_VEL:     case PJP_VEL2:    case PJP_VEL3:
    case PJP_FMAX:    case PJP_FMAX2:   case PJP_FMAX3:
    case PJP_FUDGEFAC:  case PJP_FUDGEFAC2: case PJP_FUDGEFAC3:
    case PJP_LOSTOP:  case PJP_LOSTOP2:   case PJP_LOSTOP3:
    case PJP_HISTOP:  case PJP_HISTOP2: case PJP_HISTOP3:
    case PJP_BOUNCE:  case PJP_BOUNCE2: case PJP_BOUNCE3:
    case PJP_CFM:     case PJP_CFM2:    case PJP_CFM3:
    case PJP_STOPERP:   case PJP_STOPERP2:  case PJP_STOPERP3:
    case PJP_STOPCFM:   case PJP_STOPCFM2:  case PJP_STOPCFM3:
    case PJP_SUSPENSIONERP: case PJP_SUSPENSIONERP2: case PJP_SUSPENSIONERP3:
    case PJP_SUSPENSIONCFM: case PJP_SUSPENSIONCFM2: case PJP_SUSPENSIONCFM3:
    {
      static int param[] = {
        dParamVel,dParamVel2,dParamVel3,
        dParamFMax,dParamFMax2,dParamFMax3,
        dParamFudgeFactor,dParamFudgeFactor2,dParamFudgeFactor3,
        dParamLoStop,dParamLoStop2,dParamLoStop3,
        dParamHiStop,dParamHiStop2,dParamHiStop3,
        dParamBounce,dParamBounce2,dParamBounce3,
        dParamCFM,dParamCFM2,dParamCFM3,
        dParamStopERP,dParamStopERP2,dParamStopERP3,
        dParamStopCFM,dParamStopCFM2,dParamStopCFM3,
        dParamSuspensionERP,dParamSuspensionERP2,dParamSuspensionERP3,
        dParamSuspensionCFM,dParamSuspensionCFM2,dParamSuspensionCFM3
        };
      static int pa;
      pa = param[(int)fn->upvalue-PJP_VEL];
      if (n==2 && read>=2) {
        switch (joint->classtype) {
          default: return FunctionPublish_returnError(state,"Incompatible jointtype");
          //case LUXI_CLASS_PJOINTBALL: dJointSetHingeParam(joint->ojoint.joint,pa,f[0]); break;
          case LUXI_CLASS_PJOINTHINGE: dJointSetHingeParam(joint->ojoint.joint,pa,f[0]); break;
          case LUXI_CLASS_PJOINTHINGE2: dJointSetHinge2Param(joint->ojoint.joint,pa,f[0]); break;
          case LUXI_CLASS_PJOINTSLIDER: dJointSetSliderParam(joint->ojoint.joint,pa,f[0]); break;
          case LUXI_CLASS_PJOINTUNIVERSAL:dJointSetUniversalParam(joint->ojoint.joint,pa,f[0]); break;
          case LUXI_CLASS_PJOINTAMOTOR: dJointSetAMotorParam(joint->ojoint.joint,pa,f[0]); break;
        }
        return 0;
      }
      switch (joint->classtype) {
        default: return FunctionPublish_returnError(state,"Incompatible jointtype");
        case LUXI_CLASS_PJOINTBALL: r[0] = dJointGetHingeParam(joint->ojoint.joint,pa); break;
        case LUXI_CLASS_PJOINTHINGE:  r[0] = dJointGetHingeParam(joint->ojoint.joint,pa); break;
        case LUXI_CLASS_PJOINTHINGE2:r[0] = dJointGetHinge2Param(joint->ojoint.joint,pa); break;
        case LUXI_CLASS_PJOINTSLIDER:r[0] = dJointGetSliderParam(joint->ojoint.joint,pa); break;
        case LUXI_CLASS_PJOINTUNIVERSAL:r[0] = dJointGetUniversalParam(joint->ojoint.joint,pa); break;
        case LUXI_CLASS_PJOINTAMOTOR: r[0] = dJointGetAMotorParam(joint->ojoint.joint,pa); break;
      }
      return FunctionPublish_returnFloat(state,(float)r[0]);
    }
  }
  return 0;
}

enum PB_UPS_e {
  PB_VELOCITY,
  PB_ANGVEL,
  PB_GRAVITYMODE,
  PB_ADDFORCE,
  PB_ADDTORQUE,
  PB_ADDRELFORCE,
  PB_ADDRELTORQUE,
  PB_FORCE,
  PB_TORQUE,
  PB_STATE,
  PB_AUTODISABLE,
  PB_VELTHRESHOLD,
  PB_ANGTHRESHOLD,
  PB_DISABLESTEPS,
  PB_DISABLETIME,
  PB_DISABLEDEFAULT,
  PB_SETMASSZERO,
  PB_MASSPARAM,
  PB_ROTDRAG,
  PB_VELDRAG,
  PB_SETMASSSPHERE,
  PB_SETMASSCCYL,
  PB_SETMASSCYL,
  PB_SETMASSBOX,
  PB_ADDMASSSPHERE,
  PB_ADDMASSCCYL,
  PB_ADDMASSCYL,
  PB_ADDMASSBOX,

  PB_ADJUST,
  PB_MASSROT,
  PB_FINITEMODE,
  PB_JOINTGET,
  PB_JOINTCOUNT,
  PB_CONNECTED,
  PB_LOCK,
  PB_AXISLOCK,
  PB_AFFECT,
  PB_REAFFECT,
  PB_RELPOINT,
  PB_RELPOINTVEL,
  PB_POINTVEL,
  PB_RELPOSPOINT,
  PB_WORLDVECTOR,
  PB_LOCALVECTOR,
  PB_MINENABLEFORCE,
  PB_NEXTMINENABLEFORCE,
  PB_NOCOLLISSIONWITHCONNECTED,
  PB_MINACTDEPTH,
  PB_ACTDEPTH,
  PB_JOINTFORCES,
  PB_RELVELOCITIES
};
static int PubBody_prop(PState state, PubFunction_t *fn, int n)
{
  static Reference ref;
  static PhysicsObject_t *body,*body2;
  static float f[12];
  static int read,b;
  static dMass mass,massadd;
  static dReal r[12];

  if (n<1 || (read=FunctionPublish_getArg(state,13,LUXI_CLASS_PBODY,(void*)&ref,
      FNPUB_TOVECTOR4(f),FNPUB_TOVECTOR4((&f[4])),FNPUB_TOVECTOR4((&f[8])))) < 1)
    return FunctionPublish_returnError(state,"first argument has to be a dbody");

  if (!Reference_get(ref,body) || body->classtype!=LUXI_CLASS_PBODY)
    return FunctionPublish_returnError(state,"given dbody (arg1) is not valid or not a body");

  switch ((int)fn->upvalue) {
    default: return 0;
    case PB_VELOCITY:
      if (n==4 && read>=4)
        dBodySetLinearVel(body->body,f[0],f[1],f[2]);
      else {
        static const dReal *vec;
        vec = dBodyGetLinearVel(body->body);
        f[0] = (float)vec[0]; f[1] = (float)vec[1]; f[2] = (float)vec[2];
        return FunctionPublish_setRet(state,3, FNPUB_FROMVECTOR3(f));
      }
    break;
    case PB_ANGVEL:
      if (n==4 && read>=4)
        dBodySetAngularVel(body->body,f[0],f[1],f[2]);
      else {
        static const dReal *vec;
        vec = dBodyGetAngularVel(body->body);
        f[0] = (float)vec[0]; f[1] = (float)vec[1]; f[2] = (float)vec[2];
        return FunctionPublish_setRet(state,3, FNPUB_FROMVECTOR3(f));
      }
    break;
    case PB_GRAVITYMODE:
      if (FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&b)!=1)
        return FunctionPublish_returnBool(state,dBodyGetGravityMode(body->body));
      dBodySetGravityMode(body->body,b&0xff);
    break;
    case PB_MINENABLEFORCE:
      if (n>=1 && read==0)
        return FunctionPublish_returnFloat(state,body->minenableforce);
      body->minenableforce = f[0];
      break;
    case PB_NEXTMINENABLEFORCE:
      if (n>=1 && read==0)
        return FunctionPublish_returnFloat(state,body->nextminenableforce);
      body->nextminenableforce = f[0];
      break;
    case PB_MINACTDEPTH:
      if (n>=1 && read==0)
        return FunctionPublish_returnInt(state,body->minactdepth);
      body->minactdepth = (int)f[0];
      break;
    case PB_ACTDEPTH:
      return FunctionPublish_returnInt(state,body->actdepth);
    case PB_ADDFORCE:
      if (n==7 && read>=7) {dBodyAddForceAtPos(body->body,f[0],f[1],f[2],f[3],f[4],f[5]); return 0;}
      if (n==8 && read>=8) {dBodyAddForceAtRelPos(body->body,f[0],f[1],f[2],f[3],f[4],f[5]); return 0;}
      if (n==4 && read>=4) {dBodyAddForce(body->body,f[0],f[1],f[2]); return 0;}
    case PB_ADDTORQUE:
//      if (n==7 && read>=7) {dBodyAddTorqueAtPos(body->body,f[0],f[1],f[2],f[3],f[4],f[5]); return 0;}
      if (n==4 && read>=4) {dBodyAddTorque(body->body,f[0],f[1],f[2]); return 0;}
    case PB_ADDRELFORCE:
      if (n==7 && read>=7) {dBodyAddRelForceAtPos(body->body,f[0],f[1],f[2],f[3],f[4],f[5]); return 0;}
      if (n==8 && read>=8) {dBodyAddRelForceAtRelPos(body->body,f[0],f[1],f[2],f[3],f[4],f[5]); return 0;}
      if (n==4 && read>=4) {dBodyAddRelForce(body->body,f[0],f[1],f[2]); return 0;}
    case PB_ADDRELTORQUE:
//      if (n==7 && read>=7) {dBodyAddRelTorqueAt(body->body,f[0],f[1],f[2],f[3],f[4],f[5]); return 0;}
      if (n==4 && read>=4) {dBodyAddRelTorque(body->body,f[0],f[1],f[2]); return 0;}
      return FunctionPublish_returnError(state,"1 dbody and 3,6 or 7 floats required");
    case PB_FORCE:
      if (n==4 && read>=4) {dBodySetForce(body->body,f[0],f[1],f[2]); return 0;}
      else {
        static const dReal *vec;
        vec = dBodyGetForce(body->body);
        f[0] = (float)vec[0]; f[1] = (float)vec[1]; f[2] = (float)vec[2];
        return FunctionPublish_setRet(state,3, FNPUB_FROMVECTOR3(f));
      }
    case PB_TORQUE:
      if (n==4 && read>=4) {dBodySetTorque(body->body,f[0],f[1],f[2]); return 0;}
      else {
        static const dReal *vec;
        vec = dBodyGetTorque(body->body);
        f[0] = (float)vec[0]; f[1] = (float)vec[1]; f[2] = (float)vec[2];
        return FunctionPublish_setRet(state,3, FNPUB_FROMVECTOR3(f));
      }
    break;
    case PB_STATE:
      if (n==1 || FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&b)!=1)
        return FunctionPublish_returnBool(state,dBodyIsEnabled(body->body));
      if (b&0xff) {
        PhysicsObject_enableBody(body);
      }
      else {
        PhysicsObject_disableBody(body);
      }
    break;
    case PB_NOCOLLISSIONWITHCONNECTED:
      if (n==1 || FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&b)!=1)
        return FunctionPublish_returnBool(state,body->nocollissionwithconnected);
      else body->nocollissionwithconnected = b&0xff;
    break;
    case PB_AUTODISABLE:
      if (n==1 || FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&b)!=1)
        return FunctionPublish_returnBool(state,dBodyGetAutoDisableFlag(body->body));
      dBodySetAutoDisableFlag(body->body,b&0xff);
    break;
    case PB_VELTHRESHOLD:
      if (n==2 && read>=2) dBodySetAutoDisableLinearThreshold(body->body,f[0]);
      else return FunctionPublish_returnFloat(state,(float)dBodyGetAutoDisableLinearThreshold(body->body));
    break;
    case PB_ANGTHRESHOLD:
      if (n==2 && read>=2) dBodySetAutoDisableAngularThreshold(body->body,f[0]);
      else return FunctionPublish_returnFloat(state,(float)dBodyGetAutoDisableAngularThreshold(body->body));
    break;
    case PB_DISABLESTEPS:
      if (n==2 && read>=2) dBodySetAutoDisableSteps(body->body,(int)f[0]);
      else return FunctionPublish_returnInt(state,dBodyGetAutoDisableSteps(body->body));
    break;
    case PB_DISABLETIME:
      if (n==2 && read>=2) dBodySetAutoDisableTime(body->body,f[0]);
      else return FunctionPublish_returnFloat(state,(float)dBodyGetAutoDisableTime(body->body));
    break;
    case PB_DISABLEDEFAULT:
      dBodySetAutoDisableDefaults(body->body);
    break;

    case PB_SETMASSZERO:
      dBodyGetMass(body->body,&mass);
      dMassSetZero(&mass);
      dBodySetMass(body->body,&mass);
    break;
    case PB_MASSPARAM:
      dBodyGetMass(body->body,&mass);
      if (n==11 && read>=11) {
        dMassSetParameters(&mass,f[0],f[1],f[2],f[3],f[4],f[5],f[6],f[7],
          f[8],f[9]);
        dBodySetMass(body->body,&mass);
      } else {
        f[0] = (float)mass.mass;
        f[1] = (float)mass.c[0];
        f[2] = (float)mass.c[1];
        f[3] = (float)mass.c[2];
        f[4] = (float)mass.I[0];
        f[5] = (float)mass.I[5];
        f[6] = (float)mass.I[10];
        f[7] = (float)mass.I[4];
        f[8] = (float)mass.I[8];
        f[9] = (float)mass.I[9];
        return FunctionPublish_setRet(state,10,FNPUB_FROMVECTOR4(f),
          FNPUB_FROMVECTOR4((&f[4])),FNPUB_FROMVECTOR2((&f[8])));
      }
    break;
    case PB_AXISLOCK: {
      if (n>=5 && read>=5) {
        body->axislockvec[0] = f[1];
        body->axislockvec[1] = f[2];
        body->axislockvec[2] = f[3];
        body->axislock = (int)f[0]-1;
        body->axislock = (body->axislock<0 || body->axislock>2)?-1:body->axislock;
        return 0;
      }
      return FunctionPublish_setRet(state,4,LUXI_CLASS_INT,body->axislock,FNPUB_FROMVECTOR3(body->axislockvec));
    }
    break;
    case PB_LOCK:
      if (n>=3 && read>=3) {
        body->plane = f[1];
        if (n>3 && read>3)
          body->planewidth = f[2];
        else
          body->planewidth = 0;
        if (n>4 && read>4)
          body->planeveldamp = f[3];
        else
          body->planeveldamp = 0;
        body->lock = (unsigned int)f[0]-1;
        body->lock = (body->lock<0 || body->lock>2)?-1:body->lock;
        return 0;
      }
      return FunctionPublish_setRet(state,2,LUXI_CLASS_INT,body->lock+1,FNPUB_FFLOAT(body->plane));
    break;
    case PB_VELDRAG:
      if (n==4 && read>=4) {
        body->veldrag[0] = f[0];
        body->veldrag[1] = f[1];
        body->veldrag[2] = f[2];
        return 0;
      }
      else {
        return FunctionPublish_setRet(state,3, FNPUB_FROMVECTOR3(body->veldrag));
      }
    break;
    case PB_ROTDRAG:
      if (n==4 && read>=4) {
        body->rotdrag[0] = f[0];
        body->rotdrag[1] = f[1];
        body->rotdrag[2] = f[2];
        return 0;
      }
      else {
        return FunctionPublish_setRet(state,3, FNPUB_FROMVECTOR3(body->rotdrag));
      }
    break;
    case PB_ADDMASSSPHERE:
    case PB_SETMASSSPHERE:
      if (n<3 || read<3)
        return FunctionPublish_returnError(state,"two floats required");
      if (n==3) dMassSetSphere(&massadd,f[0],f[1]);
      else dMassSetSphereTotal(&massadd,f[0],f[1]);
      if ((int)fn->upvalue == PB_ADDMASSSPHERE)
      {
                dBodyGetMass(body->body,&mass);
                dMassAdd(&massadd,&mass);
            }
            dBodySetMass(body->body,&massadd);
    break;
    case PB_ADDMASSCCYL:
    case PB_SETMASSCCYL:
      dBodyGetMass(body->body,&mass);
      if (n<5 || read<5)
        return FunctionPublish_returnError(state,"4 floats required");
//      f[1] = (f[1]==2)?3:(f[1]==3)?2:1;
      if (n==5) dMassSetCylinder(&massadd,f[0],(int)f[1],f[2],f[3]);
      else dMassSetCylinderTotal(&massadd,f[0],(int)f[1],f[2],f[4]);
      if ((int)fn->upvalue == PB_ADDMASSCCYL)
      {
                dBodyGetMass(body->body,&mass);
                dMassAdd(&massadd,&mass);
            }
      dBodySetMass(body->body,&massadd);
    break;
    case PB_ADDMASSCYL:
    case PB_SETMASSCYL:
      dBodyGetMass(body->body,&mass);
      if (n<5 || read<5)
        return FunctionPublish_returnError(state,"4 floats required");
//      f[1] = (f[1]==2)?3:(f[1]==3)?2:1;
      if (n==5) dMassSetCapsule(&massadd,f[0],(int)f[1],f[2],f[3]);
      else dMassSetCapsuleTotal(&massadd,f[0],(int)f[1],f[2],f[4]);
      if ((int)fn->upvalue == PB_ADDMASSCYL)
      {
                dBodyGetMass(body->body,&mass);
                dMassAdd(&massadd,&mass);
            }
      dBodySetMass(body->body,&massadd);
    break;
    case PB_ADDMASSBOX:
    case PB_SETMASSBOX:
      dBodyGetMass(body->body,&mass);
      if (n<5 || read<5)
        return FunctionPublish_returnError(state,"4 floats required");
      if (n==5) dMassSetBox(&massadd,f[0],f[1],f[2],f[3]);
      else dMassSetBoxTotal(&massadd,f[0],f[1],f[2],f[4]);
      if ((int)fn->upvalue == PB_ADDMASSBOX)
      {
                dBodyGetMass(body->body,&mass);
                dMassAdd(&massadd,&mass);
            }
      dBodySetMass(body->body,&massadd);
    break;
    case PB_ADJUST:
      dBodyGetMass(body->body,&mass);
      if (n<2 || read<2)
        return FunctionPublish_returnError(state,"1 float required");
      dMassAdjust(&mass,f[0]);
      dBodySetMass(body->body,&mass);
    break;
    case PB_MASSROT:
      dBodyGetMass(body->body,&mass);
      if (n<4 || read<4)
        return FunctionPublish_returnError(state,"3 floats required");
      lxMatrix44FromEulerZYXFast(f,f);
      lxMatrix44TransposeRot1(f);
      dMassRotate(&mass,f);
      dBodySetMass(body->body,&mass);
    break;
    case PB_FINITEMODE:
      if (n==1) {
        static dReal res[3];
        b = dBodyGetFiniteRotationMode(body->body);
        dBodyGetFiniteRotationAxis(body->body,res);
        f[0] = (float)res[0]; f[1] = (float)res[1]; f[2] = (float) res[2];
        return FunctionPublish_setRet(state,4,LUXI_CLASS_BOOLEAN,&b,FNPUB_FROMVECTOR3(f));
      }
      if (n==2 && FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&b)==1) {
        dBodySetFiniteRotationMode(body->body,b&0xff);
        return 0;
      }
      if (n==5 && read>=5) {
        dBodySetFiniteRotationAxis(body->body,f[1],f[2],f[3]);
        return 0;
      }
      return FunctionPublish_returnError(state,"1,2 or 5 args required: body,boolean,3 floats");
    case PB_JOINTGET: {
      static dJointID j;
      static PhysicsObject_t *joint;
      if (n!=2 || read<2)
        return FunctionPublish_returnError(state,"2 args (body and index) required");
      b = (int)f[0];
      if (b>=dBodyGetNumJoints(body->body) || b<0) return 0;
      j = dBodyGetJoint(body->body,b);
      if (j==0) return 0;
      GETODEDATA(joint,j,dJointGetData)
        return FunctionPublish_returnError(state,"Brainless joint found, developers \
surprised (report that)");
      return FunctionPublish_returnType(state,joint->classtype,REF2VOID(joint->ref));
    }
    case PB_JOINTCOUNT:
      return FunctionPublish_returnInt(state,dBodyGetNumJoints(body->body));
    case PB_RELVELOCITIES: {
      dJointID j;
      dBodyID b2;
      const dReal *vec,*ovec;
      dMass mass;
      PhysicsObject_t *joint;


      dBodyGetMass(body->body,&mass);


      if (read<1 && n<1) f[0] = 1;
      f[1] = 0;
      b = dBodyGetNumJoints(body->body);
      ovec = dBodyGetLinearVel(body->body);
      f[2] = lxVector3SqLength(ovec)*f[0]*mass.mass;
      while (b-->0) {
        j = dBodyGetJoint(body->body,b);
        GETODEDATA(joint,j,dJointGetData);
        if ((b2=dJointGetBody(j,0))!=body->body) {}
        else  if ((b2=dJointGetBody(j,1))!=body->body) {}
        if (b2 && b2!=body->body) { // b2 = other body
          dBodyGetMass(b2,&mass);
          vec = dBodyGetLinearVel(b2);
          lxVector3Sub(&f[9],vec,ovec);
          f[1]+=lxVector3SqLength((&f[9]))*f[0]*mass.mass;
        } else f[1]+=f[2];
      }

      return FunctionPublish_returnFloat(state,f[1]);
    }
    case PB_JOINTFORCES: {
      dJointID j;
      PhysicsObject_t *joint;
      f[0] = 0; f[1] = 0; f[2] = 0; f[3] = 0; f[4] = 0;


      b = dBodyGetNumJoints(body->body);
      while (b-->0) {
        j = dBodyGetJoint(body->body,b);
        GETODEDATA(joint,j,dJointGetData);
        if (dJointGetBody(j,0)==body->body) {
          f[4]+=lxVector3LengthFast(joint->ojoint.jointfeedback.f1);
          lxVector3Add(f,f,joint->ojoint.jointfeedback.f1);
        } else {
          f[4]+=lxVector3LengthFast(joint->ojoint.jointfeedback.f2);
          lxVector3Add(f,f,joint->ojoint.jointfeedback.f2);
        }
      }
      f[3] = lxVector3LengthFast(f);

      return FunctionPublish_setRet(state,5,FNPUB_FROMVECTOR3(f),FNPUB_FROMVECTOR2((&f[3])));
    }
    case PB_CONNECTED:
      if (n!=2 || FunctionPublish_getNArg(state,1,LUXI_CLASS_PBODY,(void*)&ref)!=1)
        return FunctionPublish_returnError(state,"arg2 dbody required");
      if (!Reference_get(ref,body2))
        return FunctionPublish_returnError(state,"arg2 is not valid");


      b = dAreConnected(body->body,body2->body);


      return FunctionPublish_returnBool(state,b);

    case PB_AFFECT:
      if (n==2 && read>=2) body->affect = (int)f[0];
      else return FunctionPublish_returnInt(state,body->affect);
    break;
    case PB_REAFFECT:
      if (n==2 && read>=2) body->reaffect = (int)f[0];
      else return FunctionPublish_returnInt(state,body->reaffect);
    break;

    case PB_RELPOINT:
    case PB_RELPOINTVEL:
    case PB_POINTVEL:
    case PB_RELPOSPOINT:
    case PB_WORLDVECTOR:
    case PB_LOCALVECTOR:
      if (n!=4 || read<4) return FunctionPublish_returnError(state,"arg2,3,4 must be floats");

      switch ((int)fn->upvalue) {
        case PB_RELPOINT: dBodyGetRelPointPos(body->body,f[0],f[1],f[2],r); break;
        case PB_RELPOINTVEL: dBodyGetRelPointVel (body->body,f[0],f[1],f[2],r); break;
        case PB_POINTVEL: dBodyGetPointVel(body->body,f[0],f[1],f[2],r); break;
        case PB_RELPOSPOINT: dBodyGetPosRelPoint(body->body,f[0],f[1],f[2],r); break;
        case PB_WORLDVECTOR: dBodyVectorToWorld(body->body,f[0],f[1],f[2],r); break;
        case PB_LOCALVECTOR: dBodyVectorFromWorld(body->body,f[0],f[1],f[2],r); break;
      }

      f[0] = (float)r[0];f[1] = (float)r[1];f[2] = (float)r[2];
      return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(f));
    break;
  }
  return 0;
}

static int PubSpace_prop(PState state,PubFunction_t *fn, int n)
{
  Reference ref;
  PhysicsObject_t *space,*geom;

  if (FunctionPublish_getNArg(state,0,LUXI_CLASS_PSPACE,(void*)&ref) != 1)
    return FunctionPublish_returnError(state,"first argument has to be a space");

  if (!Reference_get(ref,space))
    return FunctionPublish_returnError(state,"given space (arg1) is not valid");

  switch ((int)fn->upvalue) {
    default: return 0;
    case PHT_COLLIDE:
      if (n==2)
      {
        static int col;
        if (FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&col)!=1)
          return FunctionPublish_returnError(state,"arg2 must be integer");
        space->ospace.collidetest = col&0xff;
        return 0;
      }
      return FunctionPublish_returnBool(state,space->ospace.collidetest);
    break;
    case PHT_REMOVE:
    case PHT_QUERY:
    case PHT_ADD:
      if (FunctionPublish_getNArg(state,1,LUXI_CLASS_PCOLLIDER,(void*)&ref) != 1)
        return FunctionPublish_returnError(state,"geom required");

      if (!Reference_get(ref,geom))
        return FunctionPublish_returnError(state,"geom is invalid");
      if (geom->classtype == LUXI_CLASS_PBODY)
        return FunctionPublish_returnError(state,"dbody as arg2 is not allowed here");
      switch ((int)fn->upvalue) {
        case PHT_REMOVE: dSpaceRemove(space->ospace.space,geom->geom); return 0;
        case PHT_ADD: dSpaceAdd(space->ospace.space,geom->geom);
          if (geom->type==PHT_GEOM)
            geom->enabledSpace=geom->disabledSpace=space->ref;
          return 0;
        case PHT_QUERY: return FunctionPublish_returnBool(state,dSpaceQuery(space->ospace.space,geom->geom));
      }
      break;
    case PHT_GET:
    {
      static int i;
      static dGeomID id;
      if (FunctionPublish_getNArg(state,1,LUXI_CLASS_INT,(void*)&i)!=1)
        return FunctionPublish_returnError(state,"arg2 must be integer");
      id = dSpaceGetGeom(space->ospace.space,i);
      if (id==0) return 0;
      GETODEDATA(geom,id,dGeomGetData)
        return FunctionPublish_returnError(state,
          "mysterious error... the geom is there, but is said to be invalid...");
      return FunctionPublish_returnType(state,geom->classtype,REF2VOID(geom->ref));
    } break;
    case PHT_COUNT:
      return FunctionPublish_returnInt(state,dSpaceGetNumGeoms(space->ospace.space));
  }

  return 0;
}

enum PUBGEOM_PARAMPROPS {
  PGPP_CCYLRADIUS, //
  PGPP_SPHERERADIUS, //
  PGPP_CCYLLENGTH, //
  PGPP_CCYLLENGTHRAD, //
  PGPP_BOXSIZE, //
  PGPP_RAYSET,
  PGPP_RAYGET,
  PGPP_RAYLENGTH,
  PGPP_PLANENORMAL,
  PGPP_PLANEDISTANCE,
  PGPP_CATEGORY,
  PGPP_SETCATEGORY,
  PGPP_TRANSFORMER,
  PGPP_MATERIALID,
  PGPP_MATRIXTRANSFORM,
  PGPP_MATRIXINVTRANSFORM,
  PGPP_DISABLEDSPACE,
  PGPP_ENABLEDSPACE,
  PGPP_FDIRNORMAL,
  PGPP_MOTION1,
  PGPP_MOTION2,
  PGPP_FDIRPRIO,
};

static int PubGeom_paramProp(PState state,PubFunction_t *fn, int n)
{
  static lxMatrix44SIMD f;
  static lxMatrix44SIMD r;

    static Reference ref;
  static PhysicsObject_t *geom,*space;
  static int up,read;

  if ((read=FunctionPublish_getArg(state,9,LUXI_CLASS_PGEOM,(void*)&ref,
      FNPUB_TOVECTOR4(f),FNPUB_TOVECTOR4((&f[4])))) < 1)
    return FunctionPublish_returnError(state,"first argument has to be a dgeom");
//  LUX_PRINTF("%i %i %f %f %f %f %f\n",n,read,f[0],f[1],f[2],f[3],f[4]);
  if (!Reference_get(ref,geom))
    return FunctionPublish_returnError(state,"given geom (arg1) is not valid");

  up = (int)fn->upvalue;
  switch (up) {
    case PGPP_BOXSIZE:
      if (geom->classtype!=LUXI_CLASS_PGEOMBOX)
        return FunctionPublish_returnError(state,"arg1 must be dgeombox");
      if (n==4 && read>=4) {
        if (read<3) return FunctionPublish_returnError(state,"arg 1,2,3 must be floats");
        dGeomBoxSetLengths(geom->geom,f[0],f[1],f[2]);
        return 0;
      }
      if (n==1) {
        dGeomBoxGetLengths(geom->geom,r);
        f[0] = (float)r[0]; f[1] = (float)r[1]; f[2] = (float)r[2];
        return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(f));
      }
      return FunctionPublish_returnError(state,"dgeomboy and optional 3 floats required");

    case PGPP_CCYLLENGTHRAD:
      if (geom->classtype!=LUXI_CLASS_PGEOMCCYLINDER)
        return FunctionPublish_returnError(state,"arg1 must be dgeomccylinder");
      if (n==3 && read>=3) {
        dGeomCylinderSetParams(geom->geom,f[0],f[1]);
        return 0;
      }
      if (n==1) {
        dGeomCylinderGetParams(geom->geom,&r[0],&r[1]);
        f[0] = (float)r[0]; f[1] = (float)r[1];
        return FunctionPublish_setRet(state,2,FNPUB_FROMVECTOR2(f));
      }
      return FunctionPublish_returnError(state,"dgeomccylinder and optional 2 float required");

    case PGPP_CCYLRADIUS:
    case PGPP_CCYLLENGTH:
      if (geom->classtype!=LUXI_CLASS_PGEOMCCYLINDER && geom->classtype!=LUXI_CLASS_PGEOMCYLINDER)
        return FunctionPublish_returnError(state,"arg1 must be dgeomccylinder");
      if (n==2 && read>=2) {
        dGeomCylinderGetParams(geom->geom,&r[0],&r[1]);
        if (up==PGPP_CCYLRADIUS) r[0] = f[0];
        else r[1] = f[0];
        dGeomCylinderSetParams(geom->geom,r[0],r[1]);
        return 0;
      }
      if (n==1) {
        dGeomCylinderGetParams(geom->geom,&r[0],&r[1]);
        if (up==PGPP_CCYLRADIUS) f[0] = (float)r[0];
        else f[0] = (float)r[1];
        return FunctionPublish_returnFloat(state,f[0]);
      }
      return FunctionPublish_returnError(state,"dgeomccylinder and optional float required");

    case PGPP_SPHERERADIUS:
      if (geom->classtype!=LUXI_CLASS_PGEOMSPHERE)
        return FunctionPublish_returnError(state,"arg1 must be dgeomsphere");
      if (n==2 && read>=2) {
        dGeomSphereSetRadius(geom->geom,f[0]);
        return 0;
      }
      if (n==1)
        return FunctionPublish_returnFloat(state,(float)dGeomSphereGetRadius(geom->geom));
      return FunctionPublish_returnError(state,"dgeomsphere and optional float required");

    case PGPP_RAYSET:
      if (geom->classtype!=LUXI_CLASS_PGEOMRAY)
        return FunctionPublish_returnError(state,"arg1 must be dgeomray");
      if (n!=7 || read<7)
        return FunctionPublish_returnError(state,"arg1 must be dgeomray + 6 floats");
    //  LUX_PRINTF("-> %f %f %f %f %f %f\n",f[0],f[1],f[2],f[3],f[4],f[5]);
    //  Vector3Normalized(&f[3]);
      dGeomRaySet(geom->geom,f[0],f[1],f[2],f[3],f[4],f[5]);
      return 0;
    case PGPP_RAYGET:
      if (geom->classtype!=LUXI_CLASS_PGEOMRAY)
        return FunctionPublish_returnError(state,"arg1 must be dgeomray");
      dGeomRayGet(geom->geom,&r[0],&r[3]);
      f[0] = (float)r[0]; f[1] = (float)r[1]; f[2] = (float)r[2];
      f[3] = (float)r[3]; f[4] = (float)r[4]; f[5] = (float)r[5];
      return FunctionPublish_setRet(state,6,FNPUB_FROMVECTOR3(f),FNPUB_FROMVECTOR3((&f[3])));
    case PGPP_RAYLENGTH:
      if (geom->classtype!=LUXI_CLASS_PGEOMRAY)
        return FunctionPublish_returnError(state,"arg1 must be dgeomray");
      if (n==1)   return FunctionPublish_returnFloat(state,(float)dGeomRayGetLength(geom->geom));
      if (n==2 && read>=2) {dGeomRaySetLength(geom->geom,f[0]); return 0;}
      return FunctionPublish_returnError(state,"arg1 must be dgeomray, opt. arg2 must be float");

    case PGPP_PLANEDISTANCE:
    case PGPP_PLANENORMAL:
      if (geom->classtype!=LUXI_CLASS_PGEOMPLANE)
        return FunctionPublish_returnError(state,"arg1 must be dgeomplane");
      if (n==1) {
        dGeomPlaneGetParams(geom->geom,r);
        f[0] = (float)r[0]; f[1] = (float)r[1]; f[2] = (float)r[2];
        if (up==PGPP_PLANEDISTANCE)
          return FunctionPublish_returnFloat(state,f[3]);
        return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(f));
      }
      if (n==2 && read>=2 && up==PGPP_PLANEDISTANCE) {
        dGeomPlaneGetParams(geom->geom,r);
        dGeomPlaneSetParams(geom->geom,r[0],r[1],r[2],f[0]);
        return 0;
      }
      if (n==4 && read>=4 && up==PGPP_PLANENORMAL) {
        dGeomPlaneGetParams(geom->geom,r);
        lxVector3Normalized(f);
        dGeomPlaneSetParams(geom->geom,f[0],f[1],f[2],r[3]);
        return 0;
      }
      if (up==PGPP_PLANEDISTANCE)
        return FunctionPublish_returnError(state,"dgeomplane and opt. 1 float required");
      return FunctionPublish_returnError(state,"dgeomplane and opt. 3 float required");
    case PGPP_FDIRNORMAL:
      if (n>=4 && read>=4) {
        lxVector3Normalized(f);
        lxVector3Copy(geom->fdirnormal,f);
      } else
        return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(geom->fdirnormal));
      return 0;
    case PGPP_MOTION1:
      if (n>=2 && read>=2) {
        geom->motion1 = f[0];
      } else
        return FunctionPublish_returnFloat(state,geom->motion1);
      return 0;
    case PGPP_MOTION2:
      if (n>=2 && read>=2) {
        geom->motion2 = f[0];
      } else
        return FunctionPublish_returnFloat(state,geom->motion2);
      return 0;
    case PGPP_FDIRPRIO:
      if (n>=2 && read>=2) {
        geom->fdirpriority = f[0];
      } else
        return FunctionPublish_returnFloat(state,geom->fdirpriority);
      return 0;
    case PGPP_CATEGORY:
    if (geom->classtype != LUXI_CLASS_PBODY)
    {
      static int set,bit,value;
      if (n<3 || read<3)
        return FunctionPublish_returnError(state,"need geom + 2 ints + opt bool");
      set = (f[0]!=0);
      bit = (int)f[1];
      value = (f[2]!=0);
      bit = (bit<0)?0:(bit>31)?31:bit;
      if (n==4 && read>=4) {
        if (set) {
          set = dGeomGetCategoryBits(geom->geom);
          set = value?(set|(1<<bit)):(set&(~(1<<bit)));
          dGeomSetCategoryBits(geom->geom,set);
        } else {
          set = dGeomGetCollideBits(geom->geom);
          set = value?(set|(1<<bit)):(set&(~(1<<bit)));
          dGeomSetCollideBits(geom->geom,set);
        }
        return 0;
      }
      if (set) value = dGeomGetCategoryBits(geom->geom);
      else   value = dGeomGetCollideBits(geom->geom);
      return FunctionPublish_returnBool(state,
          (value&(1<<bit))!=0);
    }
    return FunctionPublish_returnError(state,"arg1 must be geom");

    case PGPP_SETCATEGORY:
    if (geom->classtype != LUXI_CLASS_PBODY)
    {
      static int set,bit,value;
      if (n<2 || read<2)
        return FunctionPublish_returnError(state,"need geom + ints + opt int");
      set = (f[0]!=0);
      value = (int)f[1];
      if (n==3 && read>=3) {
        if (set) {
          dGeomSetCategoryBits(geom->geom,value);
        } else {
          dGeomSetCollideBits(geom->geom,value);
        }
        return 0;
      }
      if (set) value = dGeomGetCategoryBits(geom->geom);
      else   value = dGeomGetCollideBits(geom->geom);
      return FunctionPublish_returnInt(state,value);
    }
    return FunctionPublish_returnError(state,"arg1 must be geom");

    case PGPP_TRANSFORMER:
      if (geom->classtype == LUXI_CLASS_PBODY)
        return FunctionPublish_returnError(state,"arg1 must be geom");
      if (geom->transformer==NULL) return 0;
      GETODEDATA(geom,geom->transformer,dGeomGetData) return 0;
      return FunctionPublish_returnType(state,geom->classtype,REF2VOID(geom->ref));

    case PGPP_MATERIALID:
      if (geom->classtype == LUXI_CLASS_PBODY)
        return FunctionPublish_returnError(state,"arg1 must be geom");
      if (n==1) return FunctionPublish_returnInt(state,geom->material);
      if (n==2 && read>=2) {
        geom->material = (int)f[0];
        return 0;
      }
      return FunctionPublish_returnError(state,"dgeom and optional int as args required");
    case PGPP_MATRIXTRANSFORM: {
      static lxMatrix44SIMD m;
      float out[3],*matrix;

      if (n<4 || read<4)
        return FunctionPublish_returnError(state,"dgeom and 4 floats required");
      matrix = Matrix44Interface_getElements(geom->link.matrixIF,m);
      lxVector3Transform(out,f,matrix);
      return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(out));
    }
    case PGPP_MATRIXINVTRANSFORM: {
      static lxMatrix44SIMD m;
      static lxMatrix44SIMD inv;
      float out[3],*matrix;


      if (n<4 || read<4)
        return FunctionPublish_returnError(state,"dgeom and 4 floats required");
      matrix = Matrix44Interface_getElements(geom->link.matrixIF,m);
      lxMatrix44AffineInvertSIMD(inv,matrix);
      lxVector3Transform(out,f,inv);
      return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(out));
    }
    case PGPP_DISABLEDSPACE:
    case PGPP_ENABLEDSPACE: {
      dBodyID body;
      if (geom->classtype == LUXI_CLASS_PBODY || geom->type==PHT_SPACE)
        return FunctionPublish_returnError(state,"arg1 must be geom");
      if (n==1)
        if (up==PGPP_DISABLEDSPACE) {
          if (Reference_get(geom->disabledSpace,space))
            return FunctionPublish_returnType(state,space->classtype,REF2VOID(space->ref));
          else return 0;
        } else
          if (Reference_get(geom->enabledSpace,space))
            return FunctionPublish_returnType(state,space->classtype,REF2VOID(space->ref));
          else return 0;
      FNPUB_CHECKOUTREF(state,n,1,LUXI_CLASS_PSPACE,ref,space);
      if (up==PGPP_DISABLEDSPACE) geom->disabledSpace = space->ref;
      else            geom->enabledSpace = space->ref;
      body = dGeomGetBody(geom->geom);
      if (body==NULL || dBodyIsEnabled(body))
        {if (!Reference_get(geom->enabledSpace,space)) return 0;}//arghl
      else if (!Reference_get(geom->disabledSpace,space)) return 0; // arghl
      if (dGeomGetSpace(geom->geom)) dSpaceRemove(dGeomGetSpace(geom->geom),geom->geom);
      dSpaceAdd(space->ospace.space,geom->geom);
    }
    break;
  }
  return 0;
}

static int PubGeom_destroy(PState state,PubFunction_t *fn, int n) {
  Reference ref;
  PhysicsObject_t *geom;

  if (FunctionPublish_getNArg(state,0,LUXI_CLASS_PCOLLIDER,(void*)&ref) != 1 &&
    FunctionPublish_getNArg(state,0,LUXI_CLASS_PBODY,(void*)&ref) != 1 &&
    FunctionPublish_getNArg(state,0,LUXI_CLASS_PJOINT,(void*)&ref) != 1)
    return FunctionPublish_returnError(state,"first argument has to be a dgeom");

  if (!Reference_get(ref,geom))
    return FunctionPublish_returnError(state,"given (arg1) is not valid");

  Reference_free(ref);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// Collision

static int PhysicsCollResult_new(PState state,PubFunction_t *fn, int n) {
  int cnt = 128;
  int raytricnt = 128;
  int tricnt = 128;
  PhysicsCollResult_t *collres;

  FunctionPublish_getArg(state,3,LUXI_CLASS_INT,(void*)&cnt,LUXI_CLASS_INT,(void*)&raytricnt,LUXI_CLASS_INT,(void*)&tricnt);

  collres = lxMemGenZalloc( sizeof(PhysicsCollResult_t) +
            (sizeof(PhysicsContact_t)*cnt) +
            (sizeof(PhysicsRayTrisHit_t)*raytricnt) +
            (sizeof(int)*tricnt));

  collres->contacts = (void*)(collres+1);
  collres->rayhits = (void*)(collres->contacts+cnt);
  collres->trihits = (void*)(collres->rayhits+raytricnt);
  collres->ref = Reference_new(LUXI_CLASS_PCOLLRESULT,collres);
  collres->maxContacts = cnt;
  collres->maxRay = raytricnt;
  collres->maxTri = tricnt;

  Reference_makeVolatile(collres->ref);
  return FunctionPublish_returnType(state,LUXI_CLASS_PCOLLRESULT,REF2VOID(collres->ref));
}

static void PhysicsCollResult_clear(PhysicsCollResult_t *collres)
{
  if (collres->numContacts){
    int i;

    for (i = 0; i < collres->numContacts; i++){
      Reference_release(collres->contacts[i].g1);
      Reference_release(collres->contacts[i].g2);
    }

    memset(collres->contacts,0,sizeof(PhysicsContact_t)*collres->numContacts);
  }

  collres->numRay = 0;
  collres->numTri = 0;
  collres->numContacts = 0;
  collres->count = 0;
}

static void PhysicsCollResult_free (Reference ref)
{
  PhysicsCollResult_t *collres;

  Reference_get(ref,collres);

  PhysicsCollResult_clear(collres);

  lxMemGenFree(collres, sizeof(PhysicsCollResult_t) +
        (sizeof(PhysicsContact_t)*collres->maxContacts) +
        (sizeof(PhysicsRayTrisHit_t)*collres->maxRay) +
        (sizeof(int)*collres->maxTri));
}

enum PubCollRes_e{
  PCOLL_MAX,
  PCOLL_NUM,
  PCOLL_CLEAR,
  PCOLL_COMPLETE,
  PCOLL_TRIARRAY,

  PCOLL_POS,
  PCOLL_NORMAL,

  PCOLL_GEOM,
  PCOLL_RAYTRI,
  PCOLL_TRIINFO,

};

static int PhysicsCollResult_detail(PState state,PubFunction_t *fn, int n) {
  PhysicsCollResult_t *collres;
  PhysicsContact_t *con;
  Reference ref;
  int i = 0;
  int needed = (int)fn->upvalue > PCOLL_COMPLETE ? 2 : 1;

  if (n < needed || FunctionPublish_getArg(state,needed,LUXI_CLASS_PCOLLRESULT,(void*)&ref, LUXI_CLASS_INT, (void*)&i)<needed ||
    !Reference_get(ref,collres) || (needed>1 && (i < 0 || i >= collres->numContacts)))
  {
    return FunctionPublish_returnError(state,"1 dcollresult [1 int < numContacts} required");
  }

  con = &collres->contacts[i];

  switch((int)fn->upvalue){
  case PCOLL_MAX:
    return FunctionPublish_setRet(state,3,LUXI_CLASS_INT,(void*)collres->maxContacts,
      LUXI_CLASS_INT,(void*)collres->maxRay,LUXI_CLASS_INT,(void*)collres->maxTri);
  case PCOLL_NUM:
    return FunctionPublish_returnInt(state,collres->numContacts);
  case PCOLL_CLEAR:
    PhysicsCollResult_clear(collres);
    return 0;
  case PCOLL_COMPLETE:
    return FunctionPublish_returnBool(state,collres->numContacts == collres->count);
  case PCOLL_TRIARRAY:
    {
      PubArray_t tris;
      if (!collres->numTri) return 0;

      tris.data.ints = collres->trihits;
      tris.length = collres->numTri;

      return FunctionPublish_returnType(state,LUXI_CLASS_ARRAY_INT,(void*)&tris);
    }

  case PCOLL_POS:
    return FunctionPublish_setRet(state,3,  FNPUB_FROMVECTOR3(con->odata.pos));
  case PCOLL_NORMAL:
    return FunctionPublish_setRet(state,4,  FNPUB_FROMVECTOR3(con->odata.normal),
                        FNPUB_FFLOAT(con->odata.depth));
  case PCOLL_GEOM:
  {
    Reference ref1 = con->g1;
    Reference ref2 = con->g2;

    return FunctionPublish_setRet(state,2,Reference_type(ref1),REF2VOID(ref1),Reference_type(ref1),REF2VOID(ref2));
  }
  case PCOLL_TRIINFO:
  {
    if (con->tristart != con->triend){
      return FunctionPublish_setRet(state,2,LUXI_CLASS_INT, (void*)(con->triend-con->tristart),LUXI_CLASS_INT, (void*)(con->tristart + 1));
    }
    else{
      return 0;
    }
  }
  case PCOLL_RAYTRI:
    if (con->rayend != con->raystart){
      PhysicsRayTrisHit_t *ray = &collres->rayhits[con->raystart];
      return FunctionPublish_setRet(state,3,LUXI_CLASS_INT, (void*)ray->idx, FNPUB_FROMVECTOR2(ray->uv));
    }
    else{
      return 0;
    }
  }

  return 0;
}

static PhysicsCollResult_t* l_pRayTest = NULL;  // must be NULL when PubGeom_testGenerator is not running
static PhysicsCollResult_t* l_pTriTest = NULL;  // must be NULL when PubGeom_trimeshArrayCollect is not running

static int  PubGeom_trimeshRayCallback(dGeomID TriMesh, dGeomID Ray, int TriangleIndex,dReal u, dReal v);
static void PubGeom_testGenerator(void *data, dGeomID o1, dGeomID o2);

static void PubGeom_trimeshArrayCollider(dGeomID TriMesh, dGeomID RefObject, const int* TriIndices, int TriCount);
static void PubGeom_trimeshArrayTestGenerator(void *data,dGeomID o1, dGeomID o2);

// FIXME
// dump all tris somewhere
static void PubGeom_trimeshArrayCallback (dGeomID TriMesh, dGeomID RefObject, const int* TriIndices, int TriCount)
{
  int contactidx;
  if (!l_pTriTest){
    return;
  }

  contactidx = l_pTriTest->numContacts;
  if (l_pTriTest->numContacts >= l_pTriTest->maxContacts) {
    return;
  }


  l_pTriTest->contacts[contactidx].g1 = ((Reference)dGeomGetData(TriMesh));
  l_pTriTest->contacts[contactidx].g2 = ((Reference)dGeomGetData(RefObject));
  Reference_ref(l_pTriTest->contacts[contactidx].g1);
  Reference_ref(l_pTriTest->contacts[contactidx].g2);

  if (l_pTriTest->numTri + TriCount <= l_pTriTest->maxTri ){
    l_pTriTest->contacts[contactidx].tristart = l_pTriTest->numTri;
    l_pTriTest->contacts[contactidx].triend = l_pTriTest->numTri + TriCount;
    memcpy(&l_pTriTest->trihits[l_pTriTest->numTri],TriIndices,sizeof(int)*TriCount);
    l_pTriTest->numTri += TriCount;
  }
  else {
    l_pTriTest->contacts[contactidx].tristart = 0;
    l_pTriTest->contacts[contactidx].triend = 0;
  }

  l_pTriTest->numContacts++;
}

static void PubGeom_trimeshArrayCollect(void *data,dGeomID o1, dGeomID o2)
{
  static dContactGeom  contact[16];
  PhysicsCollResult_t *collres = (PhysicsCollResult_t*) data;

  if (dGeomIsSpace(o1) || dGeomIsSpace(o2)) {
    dSpaceCollide2(o1,o2,data,PubGeom_trimeshArrayCollect);
    return;
  }
  if (collres->numContacts >= collres->maxContacts) return;

  dCollide(o1,o2,16,&contact[0], sizeof(dContactGeom));
}

static int PhysicsCollResult_testtriarray(PState state,PubFunction_t *fn, int n)
{
  Reference rcoll, r1,r2;
  PhysicsCollResult_t *collres;
  PhysicsObject_t *g1,*g2;

  if (n<3 || FunctionPublish_getArg(state,3,LUXI_CLASS_PCOLLRESULT, &rcoll,LUXI_CLASS_PCOLLIDER,&r1,LUXI_CLASS_PCOLLIDER,&r2) < 3 ||
    !Reference_get(rcoll,collres) || !Reference_get(r1,g1) || !Reference_get(r2,g2))
    return FunctionPublish_returnError(state,"1 dcollresult 2 dcollider required");

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.ode -= getMicros());
  }

  PhysicsCollResult_clear(collres);
  l_pTriTest = collres;
  dSpaceCollide2(g1->geom,g2->geom,collres,PubGeom_trimeshArrayCollect);
  l_pTriTest = NULL;

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.ode += getMicros());
  }

  return FunctionPublish_returnInt(state,collres->numContacts);
}



static void PubGeom_collectnear(void *data, dGeomID o1, dGeomID o2)
{
  PhysicsCollResult_t *collres = (PhysicsCollResult_t*) data;
  int curcontact = collres->numContacts;

  if (dGeomIsSpace(o1) || dGeomIsSpace(o2)) {
    dSpaceCollide2(o1,o2,collres,PubGeom_collectnear);
    return;
  }

  if (curcontact < collres->maxContacts){
    collres->contacts[curcontact].odata.g1 = o1;
    collres->contacts[curcontact].odata.g2 = o2;
    collres->contacts[curcontact].g1 = (Reference)dGeomGetData(o1);
    collres->contacts[curcontact].g2 = (Reference)dGeomGetData(o2);

    Reference_ref(collres->contacts[curcontact].g1);
    Reference_ref(collres->contacts[curcontact].g2);
    collres->numContacts++;
  }

  collres->count++;
}


static int PhysicsCollResult_testnear(PState state,PubFunction_t *fn, int n)
{
  Reference rcoll, r1,r2;
  PhysicsCollResult_t *collres;
  PhysicsObject_t *g1,*g2;

  if (n<3 || FunctionPublish_getArg(state,3,LUXI_CLASS_PCOLLRESULT, &rcoll,LUXI_CLASS_PCOLLIDER,&r1,LUXI_CLASS_PCOLLIDER,&r2) < 3 ||
    !Reference_get(rcoll,collres) || !Reference_get(r1,g1) || !Reference_get(r2,g2))
    return FunctionPublish_returnError(state,"1 dcollresult 2 dcollider required");

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.ode -= getMicros());
  }

  PhysicsCollResult_clear(collres);
  dSpaceCollide2(g1->geom,g2->geom,collres,PubGeom_collectnear);

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.ode += getMicros());
  }

  return FunctionPublish_returnInt(state,collres->numContacts);
}

static int PhysicsCollResult_test(PState state,PubFunction_t *fn, int n)
{
  Reference rcoll, r1,r2;
  PhysicsCollResult_t *collres;
  PhysicsObject_t *g1,*g2;


  if (n<3 || FunctionPublish_getArg(state,3,LUXI_CLASS_PCOLLRESULT, &rcoll,LUXI_CLASS_PCOLLIDER,&r1,LUXI_CLASS_PCOLLIDER,&r2) < 3 ||
    !Reference_get(rcoll,collres) || !Reference_get(r1,g1) || !Reference_get(r2,g2))
    return FunctionPublish_returnError(state,"1 dcollresult 2 dcollider required");

  PhysicsCollResult_clear(collres);

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.ode -= getMicros());
  }

  // needed for ray callback
  l_pRayTest = collres;
  dSpaceCollide2(g1->geom,g2->geom,collres,PubGeom_testGenerator);
  l_pRayTest = NULL;

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.ode += getMicros());
  }

  return FunctionPublish_returnInt(state,collres->numContacts);
}


  // must always generate contacts, ie return TRUE
static int  PubGeom_trimeshRayCallback(dGeomID TriMesh, dGeomID Ray, int TriangleIndex,dReal u, dReal v)
{

  // we came from outside of the PubGeom_testGenerator, which means we dont need the results
  if (!l_pRayTest)  return LUX_TRUE;

  if (l_pRayTest->numRay < l_pRayTest->maxRay){
    PhysicsRayTrisHit_t *trihit = &l_pRayTest->rayhits[l_pRayTest->numRay];
    trihit->idx = TriangleIndex;
    lxVector2Set(trihit->uv,u,v);

    l_pRayTest->numRay++;
  }

  return LUX_TRUE;
}

static void PubGeom_testGenerator(void *data, dGeomID o1, dGeomID o2)
{
  PhysicsCollResult_t *collres = (PhysicsCollResult_t*) data;
  int offset = collres->numContacts;
  PhysicsContact_t *collcon = &collres->contacts[offset];
  int i;
  int count;
  int raystart;
  int numRay;

  if (dGeomIsSpace(o1) || dGeomIsSpace(o2)) {
    dSpaceCollide2(o1,o2,collres,PubGeom_testGenerator);
    return;
  }

  if (offset == collres->maxContacts){
    collres->count ++;
    return;
  }

  raystart = collres->numRay;
  count = dCollide(o1,o2,collres->maxContacts-collres->numContacts,&collcon->odata, sizeof(PhysicsContact_t));
  numRay = collres->numRay - raystart;

  for (i = 0; i < count; i++) {
    Reference g1 = (Reference)dGeomGetData(collcon[i].odata.g1);
    Reference g2 = (Reference)dGeomGetData(collcon[i].odata.g2);
    collcon[i].g1 = g1;
    collcon[i].g2 = g2;
    collcon[i].raystart = raystart;
    collcon[i].rayend   = raystart + (i < numRay);

    Reference_ref(g1);
    Reference_ref(g2);
  }

  collres->numContacts += count;
  collres->count += count;

}

//////////////////////////////////////////////////////////////////////////
// Properties

static int PubGeom_prop(PState pstate,PubFunction_t *fn, int n)
{
  static Reference ref;
  static PhysicsObject_t *geom,*body;
  byte state;

  if (n<1 || FunctionPublish_getNArg(pstate,0,LUXI_CLASS_PGEOM,(void*)&ref) != 1)
    return FunctionPublish_returnError(pstate,"first argument has to be a dgeom");

  if (!Reference_get(ref,geom))
    return FunctionPublish_returnError(pstate,"given geom (arg1) is not valid");

  if (FunctionPublish_isChildOf(geom->classtype,LUXI_CLASS_PSPACE) || geom->classtype == LUXI_CLASS_PBODY)
    return FunctionPublish_returnString(pstate,"only geoms can be assigned to bodies");

  switch ((int)fn->upvalue)
  {
    case PHT_BODY:
      while (geom->transformer) {
        GETODEDATA(geom,geom->transformer,dGeomGetData)
          return FunctionPublish_returnError(pstate,"something is broken and I don't know what");
      }
      if (n==2) {
        if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_PBODY,(void*)&ref) != 1) {
          dGeomSetBody(geom->geom,0);
          return 0;
        }
        if (!Reference_get(ref,body))
          return FunctionPublish_returnError(pstate,"given body (arg2) is not valid");
        dGeomSetBody(geom->geom,body->body);
        if (geom->bodyowner)
          lxLN_remove(geom->bodyowner->geombodylist,geom,nextGeomInBody,prevGeomInBody);
        //LUX_PRINTF("adding1: %p %p %p %p %p\n",body,body->geombodylist,geom,geom->nextGeomInBody,geom->nextGeomInBody);
        lxLN_addLast(body->geombodylist,geom,nextGeomInBody,prevGeomInBody);
        //LUX_PRINTF("adding2: %p %p %p %p %p\n",body,body->geombodylist,geom,geom->nextGeomInBody,geom->nextGeomInBody);
        geom->bodyowner = body;
        return 0;
      } else {
        static dBodyID id;
        id = dGeomGetBody(geom->geom);
        if (id == 0) return 0;
        GETODEDATA(body,id,dBodyGetData)
          return FunctionPublish_returnError(pstate,
            "very strange - the body is there but is said to be invalid");
        return FunctionPublish_returnType(pstate,body->classtype,REF2VOID(body->ref));
      }
    break;
    case PHT_GETCONTACTINFO:
            if (geom->maxcontactinfo==0) break;
            if (n==1)
                return FunctionPublish_returnInt(pstate,
                    geom->contactflag==l_contactflag?geom->contactcount:0);
            {
                static int i;
                if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&i)!=1)
                    return FunctionPublish_returnError(pstate,"arg2 must be int");
                if (i<0 || i>=geom->contactcount || geom->contactflag!=l_contactflag) break;
                return FunctionPublish_setRet(pstate,8,
                    FNPUB_FROMVECTOR3((&geom->contactinfo[i*7])),
                    FNPUB_FROMVECTOR4((&geom->contactinfo[i*7+3])),
                    geom->contactgeomlist[i]->classtype,
                    REF2VOID(geom->contactgeomlist[i]->ref));
            }
    case PHT_NOCONTACTGENERATOR:
            if (n==2) {
                FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&geom->nocontactgenerator);
            } else
                return FunctionPublish_returnBool(pstate,geom->nocontactgenerator);
            break;
    case PHT_MAXCONTACTINFO:
            if (n==2) {
                static int i;
                if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&i)!=1)
                    return FunctionPublish_returnError(pstate,"arg2 must be int");
                if (geom->maxcontactinfo>0) {
                    lxMemGenFree(geom->contactinfo,sizeof(float)*7*geom->maxcontactinfo);
                    lxMemGenFree(geom->contactgeomlist,sizeof(PhysicsObject_t*)*geom->maxcontactinfo);
                }
                geom->maxcontactinfo = i;
                if (i>0) {
                    geom->contactflag = l_contactflag;
                    geom->contactcount = 0;
                    geom->contactinfo = (float*)lxMemGenZalloc(sizeof(float)*7*i);
                    geom->contactgeomlist = (PhysicsObject_t**)lxMemGenZalloc(
                        sizeof(PhysicsObject_t*)*i);
                }
            } else
                return FunctionPublish_returnInt(pstate,geom->maxcontactinfo);
            break;
    case PHT_MAXCONTACTS:
      if (n==2) {
        static int i;
        if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&i)!=1)
          return FunctionPublish_returnError(pstate,"arg2 must be int");
        geom->maxcontacts = i;
      } else {
        return FunctionPublish_returnInt(pstate,geom->maxcontacts);
      }
      break;
    case PHT_RAYTRIRESULTS:
      if (geom->classtype != LUXI_CLASS_PGEOMTRIMESH)
        return FunctionPublish_returnError(pstate,"1 dgeomtrimesh required");

      if (n==1) return FunctionPublish_returnBool(pstate,dGeomTriMeshGetRayCallback(geom->geom) ? LUX_TRUE : LUX_FALSE);
      else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
        return FunctionPublish_returnError(pstate,"arg2 must be boolean");
      if (state)
        dGeomTriMeshSetRayCallback(geom->geom,PubGeom_trimeshRayCallback);
      else
        dGeomTriMeshSetRayCallback(geom->geom,NULL);
      break;

    case PHT_TRISARRAYRESULTS:
      if (geom->classtype != LUXI_CLASS_PGEOMTRIMESH)
        return FunctionPublish_returnError(pstate,"1 dgeomtrimesh required");

      if (n==1) return FunctionPublish_returnBool(pstate,dGeomTriMeshGetArrayCallback(geom->geom) ? LUX_TRUE : LUX_FALSE);
      else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
        return FunctionPublish_returnError(pstate,"arg2 must be boolean");
      if (state)
        dGeomTriMeshSetArrayCallback(geom->geom,PubGeom_trimeshArrayCallback);
      else
        dGeomTriMeshSetArrayCallback(geom->geom,NULL);
      break;
  }
  return 0;
}
static int PubGeomBody_matrix(PState state,PubFunction_t *fn,int n){
  static float *m;
  static Reference ref,matrix;
  static PhysicsObject_t *self;
  static lxVector3 pos;
  static lxMatrix44 matrot;
  static const float *p;

  switch (n)
  {
  default:
    return FunctionPublish_returnError(state,"use: (dgeom/dbody self,[matrix4x4 m])");
  case 1:
    if (1>FunctionPublish_getArg(state,1,fn->classID,(void*)&ref) || !Reference_get(ref,self))
      return FunctionPublish_returnError(state,"1 dgeom/dbody required");

    if (self->classtype==LUXI_CLASS_PBODY){
      lxMatrix44TransposeRotIdentity(matrot,dBodyGetRotation(self->body));
      p = dBodyGetPosition(self->body);
    }
    else {
      lxMatrix44TransposeRotIdentity(matrot,dGeomGetRotation(self->geom));
      p = dGeomGetPosition(self->geom);
    }
    lxMatrix44SetTranslation(matrot,p);
    return PubMatrix4x4_return (state,matrot);
  case 2:
    if (2>FunctionPublish_getArg(state,2,fn->classID,(void*)&ref,LUXI_CLASS_MATRIX44,(void*)&matrix) ||
      !Reference_get(ref,self) || !Reference_get(matrix,m))
      return FunctionPublish_returnError(state,"1 dgeom/dbody 1 matrix4x4 required");
    lxMatrix44GetTranslation(m,pos);
    lxMatrix44TransposeRotIdentity(matrot,m);

    if (self->classtype==LUXI_CLASS_PBODY){
      dBodySetRotation(self->body,matrot);
      dBodySetPosition(self->body,lxVector3Unpack(pos));
    }
    else {
      dGeomSetRotation(self->geom,matrot);
      dGeomSetPosition(self->geom,lxVector3Unpack(pos));
      self->positionchange = 1;
    }
    return 0;
  }
}

static int PubGeomBody_pos(PState state,PubFunction_t *fn, int n)
{
  static Reference ref;
  static PhysicsObject_t *geom;
  static float f[3];
  static int read;

  if (n<1 || (read=FunctionPublish_getArg(state,4,fn->classID,&ref,FNPUB_TOVECTOR3(f))) < 1)
    return FunctionPublish_returnError(state,"first argument has to be a dgeom/dbody");

  if (!Reference_get(ref,geom))
    return FunctionPublish_returnError(state,"given (arg1) is not valid");

  if (n==4) {
    if (read!=4)
      return FunctionPublish_returnError(state,"1 geom and 3 floats required");
    if (geom->classtype==LUXI_CLASS_PBODY) {
      dBodySetPosition(geom->body,f[0],f[1],f[2]);
      geom->positionchange = 1;
    }
    else {
      dGeomSetPosition(geom->geom,f[0],f[1],f[2]);
      geom->positionchange = 1;
    }
    return 0;
  } else {
    static const dReal *pos;
    if (geom->classtype==LUXI_CLASS_PBODY)
        pos = dBodyGetPosition(geom->body);
    else  pos = dGeomGetPosition(geom->geom);

    f[0] = (float)pos[0]; f[1] = (float)pos[1]; f[2] = (float)pos[2];
    return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(f));
  }
  return 0;
}

static int PubGeomBody_rot(PState state,PubFunction_t *fn, int n)
{
  static Reference ref;
  static PhysicsObject_t *geom;

  if (FunctionPublish_getNArg(state,0,fn->classID,(void*)&ref) != 1)
    return FunctionPublish_returnError(state,"first argument has to be a dgeom/dbody");

  if (!Reference_get(ref,geom))
    return FunctionPublish_returnError(state,"given (arg1) is not valid");

  if (n>=4) {
    static float pos[9];
    static float f[16];
    int read;
    //static dReal rot[12]={0,0,1, 0,1,0, 1,0,0, 0,0,0};
    if ((read=FunctionPublish_getArg(state,10,fn->classID,(void*)&ref,
      FNPUB_TOVECTOR3(pos),FNPUB_TOVECTOR3((&f[4])),FNPUB_TOVECTOR3((&f[8]))))<4)
      return FunctionPublish_returnError(state,"1 geom and 3/4/9 floats required");

    switch((int)fn->upvalue){
      case ROT_DEG:
        pos[0] = LUX_DEG2RAD(pos[0]);
        pos[1] = LUX_DEG2RAD(pos[1]);
        pos[2] = LUX_DEG2RAD(pos[2]);
        lxMatrix44FromEulerZYXFast(f,pos);
        break;
      case ROT_AXIS:
        f[0] = pos[0];
        f[1] = pos[1];
        f[2] = pos[2];
        break;
      default:
      case ROT_RAD:
        lxMatrix44FromEulerZYXFast(f,pos);
        break;
      case ROT_QUAT:
        pos[3] = f[0];
        lxQuatToMatrix(pos,f);
        break;
    }


    lxMatrix44TransposeRot1(f);
//    rot[2] = (dReal)f[0]; rot[5] = (dReal)f[4]; rot[8 ] = (dReal)f[8 ];
//    rot[1] = (dReal)f[1]; rot[4] = (dReal)f[5]; rot[7 ] = (dReal)f[9 ];
//    rot[0] = (dReal)f[2]; rot[3] = (dReal)f[6]; rot[6 ] = (dReal)f[10];
    //Euler2Quat(m,pos[0],pos[1],pos[2]);
    //q[0] = m[0]; q[1] = m[1]; q[2] = m[2]; q[3] = m[3];
    if (geom->classtype==LUXI_CLASS_PBODY) {
      dBodySetRotation(geom->body,f);
      geom->positionchange = 1;
    }
    else {
      dGeomSetRotation(geom->geom,f);
      geom->positionchange = 1;
    }
    return 0;
  } else {
    static const dReal *R;
    static float f[4],r[16];
    if (geom->classtype==LUXI_CLASS_PBODY)
      R = dBodyGetRotation(geom->body);
    else
      R = dGeomGetRotation(geom->geom);
    r[0] = (float)R[0]; r[4] = (float)R[1]; r[8] = (float)R[2];
    r[1] = (float)R[4]; r[5] = (float)R[5]; r[9] = (float)R[6];
    r[2] = (float)R[8]; r[6] = (float)R[9]; r[10] = (float)R[10];

    switch((int)fn->upvalue)
    {
      case ROT_DEG:
        lxMatrix44ToEulerZYX(r,f);
        f[0] = LUX_RAD2DEG(f[0]);
        f[1] = LUX_RAD2DEG(f[1]);
        f[2] = LUX_RAD2DEG(f[2]);
        return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(f));
        break;
      default:
      case ROT_RAD:
        lxMatrix44ToEulerZYX(r,f);
        return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(f));
        break;
      case ROT_AXIS:
        return FunctionPublish_setRet(state,9, FNPUB_FROMVECTOR3(r)
          ,FNPUB_FROMVECTOR3((&r[4]))
          ,FNPUB_FROMVECTOR3((&r[8])));
      case ROT_QUAT:
        lxQuatFromMatrix(f,r);
        return FunctionPublish_setRet(state,4,FNPUB_FROMVECTOR4(f));
        break;
    }
  }
  return 0;
}

static int PubGeom_new(PState state,PubFunction_t *fn, int n)
{
  static PhysicsType_t type;
  static lxClassType geomType;
  static void *id;
  static dSpaceID space;
  static float f[4];
  static PhysicsObject_t *self;
  static dTriMeshDataID dataID;
  static int floatcnt = 0;
  static float *vertices;
  static int verticecount;
  static int *indices;
  static int indicescount;
  static lxClassType stype;
  static Reference trimeshdataref;


  floatcnt = FunctionPublish_getArg(state,4,FNPUB_TOVECTOR4(f));
  geomType = (lxClassType) fn->upvalue;
  space = NULL;

  if (n>0) {

    stype = FunctionPublish_getNArgType(state,n-1);
    if (stype==LUXI_CLASS_PSPACE || stype == LUXI_CLASS_PSPACEHASH ||
      stype == LUXI_CLASS_PSPACESIMPLE || stype == LUXI_CLASS_PSPACEQUAD) {
      static PhysicsObject_t *s;
      static Reference ref;
      FunctionPublish_getNArg(state,n-1,stype,(void*)&ref);
      if (Reference_get(ref,s))
        space = s->ospace.space;
    }
  }
  type=PHT_GEOM;

  switch (geomType) {
    default: return 0;
    case LUXI_CLASS_PGEOMSPHERE:
      if (n<1 || floatcnt<1) f[0] = 1;
      id = (void*)dCreateSphere(space,f[0]);
    break;
    case LUXI_CLASS_PGEOMBOX:
      if (n<3 || floatcnt<3) (f[0]=1,f[1]=1,f[2]=1);
      id = (void*)dCreateBox(space,(dReal)f[0],(dReal)f[1],(dReal)f[2]);
    break;
    case LUXI_CLASS_PGEOMPLANE:
      if (n<4 || floatcnt<4) (f[0]=0,f[1]=0,f[2]=1,f[3]=0);
      lxVector3Normalized(f);
      id = (void*)dCreatePlane(space,f[0],f[1],f[2],f[3]);
    break;
    case LUXI_CLASS_PGEOMCONE:
    case LUXI_CLASS_PGEOMCYLINDER:
    case LUXI_CLASS_PGEOMCCYLINDER:
      if (n<2 || floatcnt<2) (f[0]=1,f[1]=1);
      id = (void*)(
        geomType==LUXI_CLASS_PGEOMCCYLINDER ?dCreateCylinder(space,f[0],f[1]):
        geomType==LUXI_CLASS_PGEOMCYLINDER  ?dCreateCapsule(space,f[0],f[1]):
        NULL);//dCreateCone(space,f[0],f[1]));
    break;
    case LUXI_CLASS_PGEOMRAY:
      if (n<1 || floatcnt<1) f[0] = 1;
      id = (void*)dCreateRay(space,f[0]);
      dGeomRaySetClosestHit((dGeomID)id,1);
    break;
    case LUXI_CLASS_PSPACEQUAD: {
      float center[]= {0,0,0},size[] = {1,1,1};
      id = (void*)dQuadTreeSpaceCreate (space,center,size,4);
      dSpaceSetCleanup((dSpaceID)id,0);
    }break;
    case LUXI_CLASS_PSPACESIMPLE:
      id = (void*)dSimpleSpaceCreate(space);
      dSpaceSetCleanup((dSpaceID)id,0);
    break;
    case LUXI_CLASS_PSPACEHASH:
      id = (void*)dHashSpaceCreate(space);
      dSpaceSetCleanup((dSpaceID)id,0);
    break;
    case LUXI_CLASS_PBODY:
      id = (void*)dBodyCreate(l_world);
    break;
    case LUXI_CLASS_PGEOMTRANSFORM:
      id = (void*)dCreateGeomTransform(space);
    break;
    case LUXI_CLASS_PGEOMTRIMESHDATA: {
      PubArray_t ind,verts;
      int i;

      if (n!=2 || FunctionPublish_getArg(state,2,LUXI_CLASS_ARRAY_INT,&ind,LUXI_CLASS_ARRAY_FLOAT,&verts)!=2)
        return FunctionPublish_returnError(state,"at least 2 tables expected");

      verticecount = verts.length;
      indicescount = ind.length;
      //LUX_PRINTF("vc:%i ic:%i\n",verticecount,indicescount);
      if (indicescount%3!=0 || verticecount%3!=0)
        return FunctionPublish_returnError(state,
            "the size of arrays the must be a multiple of 3 (since it describes 3d triangles)");

      verticecount/=3;

      vertices = (dReal*)lxMemGenZalloc(sizeof(float)*verticecount*3);
      indices = (int*)lxMemGenZalloc(sizeof(int)*indicescount);


      FunctionPublish_fillArray(state,0,LUXI_CLASS_ARRAY_INT,&ind,indices);

      for (i=0;i<indicescount;i++) {
        if (indices[i]<0 || indices[i]>=verticecount) {
          lxMemGenFree(vertices,sizeof(float)*verticecount*3);
          lxMemGenFree(indices, sizeof(int)*indicescount);
          return FunctionPublish_returnError(state,
            "an index of a triangle exceeds the number of given vertices or is <0");
        }
      }

      FunctionPublish_fillArray(state,1,LUXI_CLASS_ARRAY_FLOAT,&verts,vertices);

      dataID = dGeomTriMeshDataCreate();
      dGeomTriMeshDataBuildSingle (dataID, vertices, sizeof(float)*3,
        verticecount,indices, indicescount, 3*sizeof(int));
      id = (void*)dataID;
      //LUX_PRINTF("%p\n",id);
    }break;
    case LUXI_CLASS_PGEOMTRIMESH: {
      PhysicsObject_t *obj;
      Reference ref;
      if (n<1 || FunctionPublish_getArg(state,1,LUXI_CLASS_PGEOMTRIMESHDATA,&ref)!=1 || !Reference_get(ref,obj))
        return FunctionPublish_returnError(state,"dgeomtrimeshdata expected as arg1");
      trimeshdataref = ref;
      id = (void*)dCreateTriMesh (space,obj->otridata.meshdata,NULL,NULL,NULL);
    }
    break;

    case LUXI_CLASS_PJOINTUNIVERSAL:
    case LUXI_CLASS_PJOINTAMOTOR:
    case LUXI_CLASS_PJOINTSLIDER:
    case LUXI_CLASS_PJOINTHINGE2:
    case LUXI_CLASS_PJOINTFIXED:
    case LUXI_CLASS_PJOINTHINGE:
    case LUXI_CLASS_PJOINTBALL:
    {
      dJointGroupID jgroup;
      if (FunctionPublish_getNArgType(state,n-1)==LUXI_CLASS_PJOINTGROUP) {
        PhysicsObject_t *s;
        Reference ref;
        FunctionPublish_getNArg(state,n-1,LUXI_CLASS_PJOINTGROUP,(void*)&ref);

        if (!Reference_get(ref,s)) jgroup = s->ojointgrp.jointgroup;
      } else jgroup = NULL;
      switch (geomType) {
        default: id = (void*)dJointCreateBall(l_world,jgroup); break;
        case LUXI_CLASS_PJOINTHINGE: id=(void*)dJointCreateHinge(l_world,jgroup); break;
        case LUXI_CLASS_PJOINTHINGE2: id=(void*)dJointCreateHinge2(l_world,jgroup); break;
        case LUXI_CLASS_PJOINTSLIDER: id=(void*)dJointCreateSlider(l_world,jgroup); break;
        case LUXI_CLASS_PJOINTUNIVERSAL: id=(void*)dJointCreateUniversal(l_world,jgroup); break;
        case LUXI_CLASS_PJOINTFIXED: id=(void*)dJointCreateFixed(l_world,jgroup); break;
        case LUXI_CLASS_PJOINTAMOTOR: id=(void*)dJointCreateAMotor(l_world,jgroup); break;
      }
    }
    break;
  }
  self = PhysicsObject_new(type,id,geomType);
  if (geomType == LUXI_CLASS_PGEOMTRIMESHDATA) {
    self->otridata.vertices = vertices;
    self->otridata.verticecount = verticecount;
    self->otridata.indices = indices;
    self->otridata.indicescount = indicescount;

  }
  if (geomType == LUXI_CLASS_PBODY) {
    self->minenableforce = 0;
    self->nextminenableforce = 0;
    self->nocollissionwithconnected = 1;
  }
  if (geomType == LUXI_CLASS_PGEOMTRIMESH){
    self->userref = trimeshdataref;
    Reference_ref(trimeshdataref);
  }
  //Reference_ref(self->ref);

  Reference_makeVolatile(self->ref);
  return FunctionPublish_returnType(state,geomType,REF2VOID(self->ref));
}

static void CollideCallback (void *data, dGeomID o1, dGeomID o2)
{
  static dBodyID b1,b2;
  b1 = dGeomGetBody(o1);
  b2 = dGeomGetBody(o2);
  if (b1 && b2 && !dBodyIsEnabled(b1) && !dBodyIsEnabled(b2)) return;
  if (dGeomIsSpace(o1) || dGeomIsSpace(o2) )
    dSpaceCollide2(o1,o2,NULL,CollideCallback);
  else
  {
    if (l_collisions.collisions>=l_collisions.size) {
      LUX_PRINTF("Error: Collissionbuffer is full\n");
    }
    l_collisions.buffer[l_collisions.collisions++] = o1;
    l_collisions.buffer[l_collisions.collisions++] = o2;

  }
}

static void ContactGenerator (void *data, dGeomID o1, dGeomID o2)
{
  static int count,j;
  static struct dContact contact[128];
  static PhysicsObject_t *a,*b,*joint,*bd1,*bd2;
  static dBodyID b1,b2;
  static Reference act,enb;
  static dJointID c;
  static int min,m1,m2;
  static float f,vec[4];

  b1 = dGeomGetBody(o1);
  b2 = dGeomGetBody(o2);
  if (b1==NULL && b2==NULL) return;
  if (!((b1 && dBodyIsEnabled(b1)) || (b2 && dBodyIsEnabled(b2)))) return;

  min = 128;
  GETODEDATA(a,o1,dGeomGetData);
  GETODEDATA(b,o2,dGeomGetData);
  if (b1) GETODEDATA(bd1,b1,dBodyGetData);
  if (b2) GETODEDATA(bd2,b2,dBodyGetData);
  if (a==NULL || b == NULL || (b1 && bd1==NULL) || (b2&&bd2==NULL)) return; ///ARGHL

  if (b1 && b2 && (bd1->nocollissionwithconnected || bd2->nocollissionwithconnected) && dAreConnected(b1,b2)) {
    if (bd1->nocollissionwithconnected && bd2->nocollissionwithconnected) return;
    if (bd1->nocollissionwithconnected) {
      b1=NULL;
      bd1=NULL;
    } else {
      b2=NULL;
      bd2=NULL;
    }
  }

  if (a->maxcontacts>0)
    min = LUX_MIN(min, a->maxcontacts);
  if (b->maxcontacts>0)
    min = LUX_MIN(min, b->maxcontacts);
//LUX_PRINTF("%i %i\n",b1,b2);
  if (!(count=dCollide(o1,o2,min,&contact[0].geom, sizeof(dContact))))
    return;
//    LUX_PRINTF("%i %i\n",min,count);

    if (a->maxcontactinfo || b->maxcontactinfo)
    {
        if (a->contactflag!=l_contactflag)
            (a->contactflag = l_contactflag, a->contactcount = 0);
        if (b->contactflag!=l_contactflag)
            (b->contactflag = l_contactflag, b->contactcount = 0);
        for (j=0;j<count;j++)
        {
            if (a->maxcontactinfo && a->maxcontactinfo>a->contactcount) {
                lxVector3Copy(&a->contactinfo[a->contactcount*7],contact[j].geom.pos);
                lxVector3Copy(&a->contactinfo[a->contactcount*7+3],contact[j].geom.normal);
                a->contactinfo[a->contactcount*7+6] = contact[j].geom.depth;
                a->contactgeomlist[a->contactcount] = b;
                a->contactcount++;
            }
            if (b->maxcontactinfo && b->maxcontactinfo>b->contactcount) {
                lxVector3Copy(&b->contactinfo[b->contactcount*7],contact[j].geom.pos);
                lxVector3Copy(&b->contactinfo[b->contactcount*7+3],contact[j].geom.normal);
                b->contactinfo[b->contactcount*7+6] = contact[j].geom.depth;
                b->contactgeomlist[b->contactcount] = a;
                b->contactcount++;
            }
        }
    }
    if (a->nocontactgenerator || b->nocontactgenerator)
    {
        return;
    }

//LUX_PRINTF("->count %i\n",count);
  for (j=0;j<count;j++)
  {
    int matid;
    Reference_reset(act);
    Reference_reset(enb);
    f = 0;
    GETODEDATA(a,contact[j].geom.g1,dGeomGetData);
    GETODEDATA(b,contact[j].geom.g2,dGeomGetData);

    if (a->material<b->material)
      (m1 = a->material,m2 = b->material);
    else (m1 = b->material,m2 = a->material);

    contact[j].surface = l_materialdata[matid = l_materialcombinations[m1<<8|m2]].param;

    if (contact[j].surface.mode & dContactFDir1) {
      static lxMatrix44SIMD rotmat;
      float *rot;
      float fdir1[3];
      lxMatrix44Identity(rotmat);
//      dGeomGetQuaternion
#define COPYMOTIONS(from) contact[j].surface.motion1 = from->motion1;contact[j].surface.motion2 = from->motion2;
      switch (l_materialdata[matid].mode) {
        default: LUX_ASSERT(0); break;
        case SFM_FDIRUSEHIGHEIDWORLD:
        case SFM_FDIRUSEHIGHEID:
          if (a->material>b->material) {
            rot = (float*)dGeomGetRotation(a->geom);
            lxVector3Copy(fdir1,a->fdirnormal);
            COPYMOTIONS(a)
          }
          else {rot = (float*)dGeomGetRotation(b->geom);
            lxVector3Copy(fdir1,b->fdirnormal);
            COPYMOTIONS(b)}break;
        case SFM_FDIRUSELOWEID:
        case SFM_FDIRUSELOWEIDWORLD:
          if (a->material<b->material) {rot = (float*)dGeomGetRotation(a->geom);
            lxVector3Copy(fdir1,a->fdirnormal);
            COPYMOTIONS(a)}
          else {rot = (float*)dGeomGetRotation(b->geom);
            lxVector3Copy(fdir1,b->fdirnormal);
            COPYMOTIONS(b)}break;
        case SFM_FDIRUSEHIGHERPRIOWORLD:
        case SFM_FDIRUSEHIGHERPRIO:
          if (a->fdirpriority>b->fdirpriority) {rot = (float*)dGeomGetRotation(a->geom);
            lxVector3Copy(fdir1,a->fdirnormal);
            COPYMOTIONS(a)}
          else {rot = (float*)dGeomGetRotation(b->geom);
            lxVector3Copy(fdir1,b->fdirnormal);
            COPYMOTIONS(b)}break;
        case SFM_FDIRUSELOWERPRIOWORLD:
        case SFM_FDIRUSELOWERPRIO:
          if (a->fdirpriority<b->fdirpriority) {rot = (float*)dGeomGetRotation(a->geom);
            lxVector3Copy(fdir1,a->fdirnormal);
            COPYMOTIONS(a)}
          else {rot = (float*)dGeomGetRotation(b->geom);
            lxVector3Copy(fdir1,b->fdirnormal);
            COPYMOTIONS(b)}break;
        case SFM_FDIRUSEID:
        case SFM_FDIRUSEIDWORLD:
          if (a->material == l_materialdata[matid].id) {rot = (float*)dGeomGetRotation(a->geom);
            lxVector3Copy(fdir1,a->fdirnormal);
            COPYMOTIONS(a)}
          else {rot = (float*)dGeomGetRotation(b->geom);
            lxVector3Copy(fdir1,b->fdirnormal);
            COPYMOTIONS(b)}break;
        case SFM_AVERAGEFDIRWORLD:
        case SFM_AVERAGEFDIR:
          lxVector3Add(fdir1,b->fdirnormal,a->fdirnormal);
          if (lxVector3Normalized(fdir1)==0) lxVector3Set(fdir1,0,0,0);
          contact[j].surface.motion1 = (a->motion1 + b->motion1)*0.5f;
          contact[j].surface.motion2 = (a->motion2 + b->motion2)*0.5f;
        case SFM_MIXPRIOBASEDWORLD:
        case SFM_MIXPRIOBASED:
          lxVector3Set(fdir1,0,0,0);
          lxVector3ScaledAdd(fdir1,fdir1,a->fdirpriority,a->fdirnormal);
          lxVector3ScaledAdd(fdir1,fdir1,b->fdirpriority,b->fdirnormal);
          if (lxVector3Normalized(fdir1)==0) lxVector3Set(fdir1,0,0,0);
          contact[j].surface.motion1 = (a->motion1 + b->motion1)*0.5f;
          contact[j].surface.motion2 = (a->motion2 + b->motion2)*0.5f;
        break;
      }
      switch (l_materialdata[matid].mode) {
        case SFM_FDIRUSELOWEID:
        case SFM_FDIRUSEHIGHERPRIO:
        case SFM_FDIRUSELOWERPRIO:
        case SFM_FDIRUSEID:
        case SFM_AVERAGEFDIR:
        case SFM_MIXPRIOBASED:
        case SFM_FDIRUSEHIGHEID:
          lxMatrix44TransposeRot1(rot);
          lxMatrix44VectorRotate(rot,fdir1);
          lxMatrix44TransposeRot1(rot);
          lxVector3Copy(vec,fdir1);
          lxVector3Cross(fdir1,vec,contact[j].geom.normal);
          if (lxVector3Normalized(fdir1)==0) lxVector3Set(fdir1,0,0,0);
          break;
      }
      if (fdir1[0]==0 && fdir1[1]==0 && fdir1[2]==0) {
        contact[j].surface.mode&=~dContactFDir1;
      } else {
        lxVector3Copy(contact[j].fdir1,fdir1);
      }
    }
    //LUX_PRINTF("creating contact %i\n",j);

    c = dJointCreateContact(l_world,l_contactjoints->ojointgrp.jointgroup,&contact[j]);
    l_contactcount++;
    if (b1 && !dBodyIsEnabled(b1) && bd1->minenableforce>0) {
      act = bd1->ref;
      enb = bd2->ref;
      b1 = NULL;
      f = bd1->minenableforce;
    } else if (b2 && !dBodyIsEnabled(b2) && bd2->minenableforce>0) {
      act = bd2->ref;
      enb = bd1->ref;
      b2 = NULL;
      f = bd2->minenableforce;
    }
    if (b1 && !dBodyIsEnabled(b1) && b2) {
      if (bd2->actdepth>0)    bd1->actdepth = bd2->actdepth - 1;
      else if(bd2->actdepth==0) b1 = NULL;
      else bd1->actdepth = bd2->minactdepth;
    } else if (b2 && !dBodyIsEnabled(b2) && b1) {
      if (bd1->actdepth>0)    bd2->actdepth = bd1->actdepth - 1;
      else if(bd1->actdepth==0) b2 = NULL;
      else bd2->actdepth = bd2->minactdepth;
    }

    //if (b1 && !dBodyIsEnabled(b1)) PhysicsObject_enableBody(bd1);
    //if (b2 && !dBodyIsEnabled(b2)) PhysicsObject_enableBody(bd2);

    dJointAttach (c,
      ((!b2 && (b1 && bd1->reaffect)) || (b1 && b2 && bd1->reaffect&&bd2->affect))?b1:NULL,
      ((!b1 && (b2 && bd2->reaffect)) || (b2 && b2 && bd2->reaffect&&bd1->affect))?b2:NULL);

    joint = PhysicsObject_new(PHT_JOINT,(void*)c,LUXI_CLASS_PJOINTCONTACT);
    Reference_makeVolatile(joint->ref);
    joint->ojoint.geom1 = a->ref;
    joint->ojoint.geom2 = b->ref;
    joint->ojoint.disabledBody = act;
    joint->ojoint.enablerBody = enb;
    joint->ojoint.activationforce = f;
    lxVector3Copy(joint->ojoint.contactpos,contact[j].geom.pos);
    lxVector3Copy(joint->ojoint.contactnormal,contact[j].geom.normal);
    joint->ojoint.contactdepth = contact[j].geom.depth;

    PhysicsObject_jointGroupAdd(l_contactjoints,joint);

  }

}

static void Collide (dSpaceID space)
{
  int i;
  dSpaceCollide(space,NULL,CollideCallback);
  for (i=0;i<dSpaceGetNumGeoms(space); i++) {
    static dGeomID geom;
    static PhysicsObject_t *obj;
    geom = dSpaceGetGeom(space,i);
    if (!geom || !dGeomIsSpace(geom)) continue;
    GETODEDATA(obj,geom,dGeomGetData);
    if (obj->ospace.collidetest)
      Collide((dSpaceID)geom);
  }
}
static void PubClass_world_free (Reference ref)
{
  //printf("FREE WORLD: %p (%p)\n",ref->value,l_world);

  dWorldID w = Reference_value(ref);
  LUX_ASSERT(w != l_world);
}
static int PubClass_world_active (PState state,PubFunction_t *fn, int n)
{
  Reference ref;
  if (n>0) {
    if (!FunctionPublish_getArg(state,1,LUXI_CLASS_PWORLD,&ref))
      return FunctionPublish_returnError(state,"Expected dworld as arg 1");

    Reference_ref(ref);
    Reference_release(l_worldref);

    l_world = Reference_value(ref);
    l_worldref = ref;
    //printf("WORLD: %p\n",l_world);
    return 0;
  }
  // FIXME
  return FunctionPublish_returnType(state,LUXI_CLASS_PWORLD,REF2VOID(l_worldref));
}
static int PubClass_world_new (PState state,PubFunction_t *fn, int n)
{
  dWorldID w = dWorldCreate();
  Reference ref = Reference_new(LUXI_CLASS_PWORLD,w);
  //printf("NEW WORLD: %p\n",w);
  Reference_makeVolatile(ref);
  return FunctionPublish_returnType(state,LUXI_CLASS_PWORLD,ref);
}

static int PubCollision_worldprop (PState state,PubFunction_t *fn, int n)
{
  float f3[3];
  int read,b;

  read = FunctionPublish_getArg(state,3,FNPUB_TFLOAT(f3[0]),FNPUB_TFLOAT(f3[1]),FNPUB_TFLOAT(f3[2]));
  switch ((int)fn->upvalue) {
    case WP_RANDOMSEED:
      if (n==1 && FunctionPublish_getArg(state,1,LUXI_CLASS_INT,&b)==1)
      {
        dRandSetSeed(b);
        return 0;
      }
      return FunctionPublish_returnInt(state,dRandGetSeed());
    case WP_GRAVITY:
      if (n!=3 && read!=3) {
        dVector3 g;
        dWorldGetGravity(l_world,g);
        return FunctionPublish_setRet(state,3,FNPUB_FFLOAT(g[0]),FNPUB_FFLOAT(g[1]),FNPUB_FFLOAT(g[2]));
      } else {
        dWorldSetGravity(l_world,f3[0],f3[1],f3[2]);
        return 0;
      }
    break;
    case WP_ERP:
      if (n!=1 && read<1) {
        return FunctionPublish_returnFloat(state,(float)dWorldGetERP(l_world));
      } else {
        dWorldSetERP(l_world,f3[0]);
        return 0;
      }
    break;
    case WP_CFM:
      if (n!=1 && read<1) {
        return FunctionPublish_returnFloat(state,(float)dWorldGetCFM(l_world));
      } else {
        dWorldSetCFM(l_world,f3[0]);
        return 0;
      }
    break;
    case WP_STEP:
    case WP_QUICKSTEP: {
      PhysicsObject_t *browse;

      if (g_Draws.drawPerfGraph){
        LUX_PROFILING_OP (g_Profiling.perframe.timers.ode -= getMicros());
      }

      l_cachedate++;
      if (n!=1 && read<1)
        return FunctionPublish_returnError(state,"step needs one float argument");

      l_timestep = f3[0];


      {
        static dReal R[16];
        dReal *vec,*pos,*rot;


        l_activeBodyCount = 0;
        //l_enabledbodies = NULL;
        lxLN_forEach(l_bodyList,browse,nextbody,prevbody)
        {
            LUX_ASSERT(browse->nextbody->prevbody == browse);
          if (dBodyIsEnabled(browse->body)) {
            PhysicsObject_enableBody(browse);
            //l_enabledbodies = browse;
            l_activeBodyCount++;
            vec = (dReal*)dBodyGetLinearVel(browse->body);
            vec[0]*=(float)pow(browse->veldrag[0],f3[0]);
            vec[1]*=(float)pow(browse->veldrag[1],f3[0]);
            vec[2]*=(float)pow(browse->veldrag[2],f3[0]);
            if (browse->lock!=-1) {
              int l = browse->lock;
              pos = (dReal*)dBodyGetPosition(browse->body);
              pos[l] = LUX_MAX(browse->plane-browse->planewidth,
                    LUX_MIN(browse->plane+browse->planewidth,pos[l]));
              dBodySetPosition(browse->body,pos[0],pos[1],pos[2]);
              vec[l] *= browse->planeveldamp;
            }
            if (browse->axislock!=-1) {
              rot = (dReal*)dBodyGetRotation(browse->body);
              R[0] = rot[0]; R[4] = rot[1]; R[8] = rot[2];
              R[1] = rot[4]; R[5] = rot[5]; R[9] = rot[6];
              R[2] = rot[8]; R[6] = rot[9]; R[10] = rot[10];

              switch(browse->axislock) {
                case 0:
                  R[0] = browse->axislockvec[0];
                  R[1] = browse->axislockvec[1];
                  R[2] = browse->axislockvec[2];
                  lxVector3Cross((&R[8]),(&R[0]),(&R[4]));
                  lxVector3Cross((&R[4]),(&R[8]),(&R[0]));
                break;
                case 2:
                  R[4] = browse->axislockvec[0];
                  R[5] = browse->axislockvec[1];
                  R[6] = browse->axislockvec[2];
                  lxVector3Cross((&R[0]),(&R[4]),(&R[8]));
                  lxVector3Cross((&R[8]),(&R[0]),(&R[4]));
                break;
                case 1:
                  R[8] = browse->axislockvec[0];
                  R[9] = browse->axislockvec[1];
                  R[10] = browse->axislockvec[2];
                  lxVector3Cross((&R[4]),(&R[8]),(&R[0]));
                  lxVector3Cross((&R[0]),(&R[4]),(&R[8]));
                break;
              }

              rot[0] = R[0]; rot[4] = R[1]; rot[8] = R[2];
              rot[1] = R[4]; rot[5] = R[5]; rot[9] = R[6];
              rot[2] = R[8]; rot[6] = R[9]; rot[10] = R[10];
              dBodySetRotation(browse->body,rot);
            }
            dBodySetLinearVel(browse->body,vec[0],vec[1],vec[2]);
            vec = (dReal*)dBodyGetAngularVel(browse->body);
            vec[0]*=(float)pow(browse->rotdrag[0],f3[0]);
            vec[1]*=(float)pow(browse->rotdrag[1],f3[0]);
            vec[2]*=(float)pow(browse->rotdrag[2],f3[0]);
            dBodySetAngularVel(browse->body,vec[0],vec[1],vec[2]);
          }
          else
          {
            PhysicsObject_disableBody(browse);
          }
        }
        lxLN_forEachEnd(l_bodyList,browse,nextbody,prevbody);
      }
      if ((int)fn->upvalue==WP_STEP) dWorldStep(l_world,f3[0]);
      else dWorldQuickStep(l_world,f3[0]);

      if (g_Draws.drawPerfGraph){
        LUX_PROFILING_OP (g_Profiling.perframe.timers.ode += getMicros());
      }
      //PhysicsObject_removeDisabledBodies();

    }
      break;
    case WP_QUICKSTEPITER:
      if (n==1 && read>=1)
        dWorldSetQuickStepNumIterations(l_world,(int)f3[0]);
      else
        return FunctionPublish_returnInt(state,dWorldGetQuickStepNumIterations(l_world));
      break;
    case WP_COUNT:
      {
        int size;
        PhysicsObject_t *browse;

        lxLN_countSize(l_pobjectList,browse,size,next,prev);

        return FunctionPublish_returnInt(state,size);
      }
    case WP_GET:
      if (n==1 && read>=1) {
        int index;
        PhysicsObject_t *browse;
        index = (int)f3[0];
        if (l_pobjectList == NULL) return 0;
        lxLN_getAtIndex(l_pobjectList, browse, index,next,prev);
        if (browse == NULL) return 0;
        return FunctionPublish_returnType(state,browse->classtype,REF2VOID(browse->ref));
      }
      return FunctionPublish_returnError(state,"int index required");
    case WP_COLBUFSIZE:
      if (n==1 && read>=1) {
        int size;
        size = (int)f3[0]*2;
        if (size<=0||size>200000)
          return FunctionPublish_returnError(state,"collisionbuffersize must be >0 and <=100000");
        gentypefree(l_collisions.buffer,dGeomID,l_collisions.size);
        l_collisions.buffer = gentypezalloc(dGeomID,size);
        l_collisions.size = size;
        l_collisions.collisions = 0;
        return 0;
      }
      return FunctionPublish_returnInt(state,l_collisions.size/2);
    case WP_COLLISIONCOUNT:
      return FunctionPublish_returnInt(state,l_collisions.collisions);
    case WP_COLBUFGET:
      if (n==1 && read>=1) {
        int index;
        PhysicsObject_t *a,*b;
        index = (int)f3[0]*2;
        if (index>=l_collisions.collisions) return 0;
        GETODEDATA(a,l_collisions.buffer[index],dGeomGetData);
        GETODEDATA(b,l_collisions.buffer[index+1],dGeomGetData);
        if (a==NULL || b==NULL)
          return FunctionPublish_returnError(state,
            "something is messed up - the collisionpartner(s) are invalid");
        return FunctionPublish_setRet(state,2,a->classtype,REF2VOID(a->ref),b->classtype,REF2VOID(b->ref));
      }
      return FunctionPublish_returnError(state,"index as arg1 required");
    case WP_COLLIDE:
      {
        Reference ref;
        PhysicsObject_t *space;

//        LUX_PRINTF("collide\n");
        if (g_Draws.drawPerfGraph){
          LUX_PROFILING_OP (g_Profiling.perframe.timers.ode -= getMicros());
        }

        l_collisions.collisions = 0;
        if (n!=1 || FunctionPublish_getNArg(state,0,LUXI_CLASS_PSPACE,(void*)&ref)!=1)
          return FunctionPublish_returnError(state,"1 dspace required");

        if (!Reference_get(ref,space))
          return FunctionPublish_returnError(state,"space is invalid");

        Collide(space->ospace.space);

        /*lxLN_forEach(l_spaceList,browse,nextspace,prevspace)
        {
//          LUX_PRINTF(".> %i %i\n",browse->space, browse->collidetest);
          if (browse->collidetest!=0)
            dSpaceCollide(browse->space,NULL,CollideCallback);
        }
        lxLN_forEachEnd(l_spaceList,browse,nextspace,prevspace);*/

        if (g_Draws.drawPerfGraph){
          LUX_PROFILING_OP (g_Profiling.perframe.timers.ode += getMicros());
        }
        return FunctionPublish_returnInt(state,l_collisions.collisions);
      }
      break;
    case WP_JOINTEMPTY:
      PhysicsObject_jointGroupEmpty(l_contactjoints);
      break;
    case WP_CONTACTCOUNT:
      return FunctionPublish_returnInt(state,l_contactcount);
    case WP_MAKECONTACTS:
      if (g_Draws.drawPerfGraph)
        LUX_PROFILING_OP (g_Profiling.perframe.timers.ode -= getMicros());

            l_contactflag++;
      PhysicsObject_jointGroupEmpty(l_contactjoints);
      {
        int i;
        dGeomID o1,o2;
        i = l_collisions.collisions;
        while (i-->0) {
          o1 = l_collisions.buffer[i--];
          o2 = l_collisions.buffer[i];

          dSpaceCollide2(o1,o2,NULL,ContactGenerator);
        }
      }

      if (g_Draws.drawPerfGraph){
        LUX_PROFILING_OP (g_Profiling.perframe.timers.ode += getMicros());
      }
    break;
    case WP_AUTODISABLE:
      if (n==1 && FunctionPublish_getNArg(state,0,LUXI_CLASS_BOOLEAN,(void*)&b)==1)
        dWorldSetAutoDisableFlag(l_world,b&0xff);
      else return FunctionPublish_returnBool(state,dWorldGetAutoDisableFlag(l_world));
    break;
    case WP_AUTODISABLELINEARTHRESHOLD:
      if (n==1 && read>=1)
        dWorldSetAutoDisableLinearThreshold(l_world,f3[0]);
      else return FunctionPublish_returnFloat(state,
        (float)dWorldGetAutoDisableLinearThreshold(l_world));
    break;
    case WP_AUTODISABLEANGULARTHRESHOLD:
      if (n==1 && read>=1)
        dWorldSetAutoDisableAngularThreshold(l_world,f3[0]);
      else return FunctionPublish_returnFloat(state,
        (float)dWorldGetAutoDisableAngularThreshold(l_world));
    break;
    case WP_AUTODISABLESTEPS:
      if (n==1 && read>=1)
        dWorldSetAutoDisableSteps(l_world,(int)f3[0]);
      else return FunctionPublish_returnInt(state,dWorldGetAutoDisableSteps(l_world));
    break;
    case WP_AUTODISABLETIME:
      if (n==1 && read>=1)
        dWorldSetAutoDisableTime(l_world,f3[0]);
      else return FunctionPublish_returnFloat(state,
        (float)dWorldGetAutoDisableTime(l_world));
    break;
    case WP_SURFACELAYER:
      if (n==1 && read>=1)
        dWorldSetContactSurfaceLayer(l_world,f3[0]);
      else return FunctionPublish_returnFloat(state,
        (float)dWorldGetContactSurfaceLayer(l_world));
    break;
    case WP_MAXCORVEL:
      if (n==1 && read>=1)
        dWorldSetContactMaxCorrectingVel (l_world,f3[0]);
      else return FunctionPublish_returnFloat(state,
        (float)dWorldGetContactMaxCorrectingVel(l_world));
    break;
    case WP_DINFINITY:
      return FunctionPublish_returnFloat(state,(float)dInfinity);

    case WP_MATERIALCOMB: {
      int m1,m2,comb,tmp;
      m1 = (int) f3[0]; m2 = (int) f3[1]; comb = (int) f3[2];
      if (m1>m2) {
        tmp = m1; m1 = m2; m2 = tmp;
      }
      tmp = m1<<8|m2;
      if (n==2 && read>=2 && m1>=0 && m1<256 && m2>=0 && m2<256)
        return FunctionPublish_returnInt(state,l_materialcombinations[tmp]);
      if (n==3 && read>=3 && m1>=0 && m1<256 && m2>=0 && m2<256 && comb>=0 && comb<256) {
        l_materialcombinations[tmp] = comb;
        return 0;
      }
      return FunctionPublish_returnError(state,"two ints (0-255) and option int (0-255) expected");
    }
    break;

    case WP_MU:
    case WP_MU2:
    case WP_BOUNCE:
    case WP_BOUNCEVEL:
    case WP_SOFTERP:
    case WP_SOFTCFM:
    case WP_MOTION1:
    case WP_MOTION2:
    case WP_SLIP1:
    case WP_SLIP2: {
      if (n==0) return FunctionPublish_returnError(state,"int as arg1 expected");
      b = (int)f3[0];
      if (b<0 || b>255)
        return FunctionPublish_returnError(state,"arg1 must be >=0 and <256");
      if (n==1 && read>=1) {
        switch ((int)fn->upvalue) {
          case WP_MU: f3[0] = l_materialdata[b].param.mu; break;
          case WP_MU2: f3[0] = l_materialdata[b].param.mu2; break;
          case WP_BOUNCE: f3[0] = l_materialdata[b].param.bounce; break;
          case WP_BOUNCEVEL: f3[0] = l_materialdata[b].param.bounce_vel; break;
          case WP_SOFTERP: f3[0] = l_materialdata[b].param.soft_erp; break;
          case WP_SOFTCFM: f3[0] = l_materialdata[b].param.soft_cfm; break;
          case WP_MOTION1: f3[0] = l_materialdata[b].param.motion1; break;
          case WP_MOTION2: f3[0] = l_materialdata[b].param.motion2; break;
          case WP_SLIP1: f3[0] = l_materialdata[b].param.slip1; break;
          case WP_SLIP2: f3[0] = l_materialdata[b].param.slip2; break;
          default: break;
        }
        return FunctionPublish_returnFloat(state,f3[0]);
      }
      if (n==2 && read>=2) {
        switch ((int)fn->upvalue) {
          case WP_MU: l_materialdata[b].param.mu=f3[1]; break;
          case WP_MU2: l_materialdata[b].param.mu2=f3[1]; break;
          case WP_BOUNCE: l_materialdata[b].param.bounce=f3[1]; break;
          case WP_BOUNCEVEL: l_materialdata[b].param.bounce_vel=f3[1]; break;
          case WP_SOFTERP: l_materialdata[b].param.soft_erp=f3[1]; break;
          case WP_SOFTCFM: l_materialdata[b].param.soft_cfm=f3[1]; break;
          case WP_MOTION1: l_materialdata[b].param.motion1=f3[1]; break;
          case WP_MOTION2: l_materialdata[b].param.motion2=f3[1]; break;
          case WP_SLIP1: l_materialdata[b].param.slip1=f3[1]; break;
          case WP_SLIP2: l_materialdata[b].param.slip2=f3[1]; break;
          default: break;
        }
        return 0;
      }
      return FunctionPublish_returnError(state,"one int and optional float required");
    } break;

    case WP_FDIRMIXMODE:
      if (n==0) return FunctionPublish_returnError(state,"int as arg1 expected");
      b = (int)f3[0];
      if (n==1 && read>=1) {
        return FunctionPublish_returnInt(state,l_materialdata[b].mode);
      } else
      if (n==2 && read>=2) {
        if (f3[1]<0 || f3[1]>SFM_MIXPRIOBASEDWORLD)
          return FunctionPublish_returnError(state,"Arg2: Invalid int value passed");
        l_materialdata[b].mode = (int)f3[1];
        return 0;
      }
      return FunctionPublish_returnError(state,"one int and optional float required");

    case WP_FDIID:
      if (n==0) return FunctionPublish_returnError(state,"int as arg1 expected");
      b = (int)f3[0];
      if (n==1 && read>=1) {
        return FunctionPublish_returnInt(state,l_materialdata[b].id);
      } else
      if (n==2 && read>=2) {
        l_materialdata[b].id = (int)f3[1];
        return 0;
      }
      return FunctionPublish_returnError(state,"one int and optional int required");

    case WP_BITMU2:
    case WP_BITBOUNCE:
    case WP_BITSOFTERP:
    case WP_BITSOFTCFM:
    case WP_BITMOTION1:
    case WP_BITMOTION2:
    case WP_BITSLIP1:
    case WP_BITSLIP2:
    case WP_BITAPPROX1:
    case WP_BITAPPROX2:
    case WP_BITFDIR:{
      const int params[] = {dContactMu2,dContactBounce,dContactSoftERP,
        dContactSoftCFM,dContactMotion1,dContactMotion2,dContactSlip1,
        dContactSlip2,dContactApprox1_1,dContactApprox1_2,dContactFDir1};

      if (n==0) return FunctionPublish_returnError(state,"int as arg1 expected");
      b = (int)f3[0];
      if (b<0 || b>255)
        return FunctionPublish_returnError(state,"arg1 must be >=0 and <256");
      if (n==1 && read>=1) {
        return FunctionPublish_returnBool(state,
          (l_materialdata[b].param.mode & params[(int)fn->upvalue-WP_BITMU2])!=0);
      }
      if (n==2 && read>=2) {
        if (f3[1]==0)
          l_materialdata[b].param.mode&=~params[(int)fn->upvalue-WP_BITMU2];
        else
          l_materialdata[b].param.mode|= params[(int)fn->upvalue-WP_BITMU2];
        return 0;
      }
      return FunctionPublish_returnError(state,"one int and optional boolean required");
    }
    break;
    case WP_ACTIVEBODIES:
      return FunctionPublish_returnInt(state,l_activeBodyCount);
    case WP_GETACTIVEBODY:
      if (n<1 || read<1) return FunctionPublish_returnError(state,"one int required");
      {
        int i=(int)f3[0];
        PhysicsObject_t *browse;
        if (l_enabledbodies==NULL || i>=l_activeBodyCount) return 0;
        lxLN_getAtIndex(l_enabledbodies, browse, i,nextEnabledBody,prevEnabledBody);
        return FunctionPublish_setRet(state,1,browse->classtype,REF2VOID(browse->ref));
      }
    break;
  }
  return 0;
}

void PubClass_Collision_clear() {
  Reference ref = l_contactjoints->ref;
  Reference wref = l_worldref;
  l_contactjoints = NULL;
  l_world = NULL;
  l_worldref = NULL;


  Reference_release(wref);
  Reference_release(ref);

  gentypefree(l_collisions.buffer,dGeomID,l_collisions.size);
  l_collisions.buffer=NULL;
  l_collisions.size=0;
  l_collisions.collisions = 0;
}

void PubClass_Collision_deinit() {
  PubClass_Collision_clear();
  dCloseODE();
}

static int PubClass_Collision_deleteall (PState state,PubFunction_t *fn, int n) {
  PubClass_Collision_clear();
  l_world = dWorldCreate();
  l_contactjoints = PhysicsObject_new(PHT_JOINTGROUP,(void*)dJointGroupCreate(0),LUXI_CLASS_PJOINTGROUP);
    Reference_makeStrong(l_contactjoints->ref);
  l_collisions.buffer = gentypezalloc(dGeomID,4000);
  l_collisions.size = 4000;
  return 0;
}

static int PubClass_Collision_getnative(PState state,PubFunction_t *fn, int n)
{
  Reference ref;
  PhysicsObject_t *self;
  lxClassType classtype = FunctionPublish_getNArgType(state,0);
  if (!FunctionPublish_getNArg(state,0,LUXI_CLASS_ODE,&ref) || !Reference_get(ref,self))
    return 0;

  if (FunctionPublish_isChildOf(classtype,LUXI_CLASS_PSPACE)){
    FunctionPublish_returnPointer(state,(void*)self->ospace.space);
  }
  else if (classtype == LUXI_CLASS_PBODY){
    FunctionPublish_returnPointer(state,(void*)self->body);
  }
  else if (FunctionPublish_isChildOf(self->classtype,LUXI_CLASS_PGEOM)){
    FunctionPublish_returnPointer(state,(void*)self->geom);
  }
  else if (FunctionPublish_isChildOf(self->classtype,LUXI_CLASS_PJOINT)){
    FunctionPublish_returnPointer(state,(void*)self->ojoint.joint);
  }
  else if (classtype == LUXI_CLASS_PJOINTGROUP) {
    FunctionPublish_returnPointer(state,(void*)self->ojointgrp.jointgroup);
  }
  else if (classtype == LUXI_CLASS_PGEOMTRIMESHDATA) {
    FunctionPublish_returnPointer(state,(void*)self->otridata.meshdata);
    //  dGeomTriMeshDataDestroy(self->meshdata);
  }
  else{
    return FunctionPublish_returnError(state,"no ode physics object given.");
  }

  return 0;
}


void PubClass_Collision_init() {
  int i;

  //dInitODE();

  Reference_registerType(LUXI_CLASS_ODE,"ode",NULL,NULL);
  Reference_registerType(LUXI_CLASS_PWORLD,"dworld",PubClass_world_free,NULL);
  Reference_registerType(LUXI_CLASS_PCOLLRESULT,"dcollresult",PhysicsCollResult_free,NULL);
  Reference_registerType(LUXI_CLASS_PGEOM,"dgeom",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PGEOMTRIMESH,"dgeomtrimesh",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PGEOMTRIMESHDATA,"dgeomtrimeshdata",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PGEOMBOX,"dgeombox",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PGEOMSPHERE,"dgeomsphere",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PGEOMCCYLINDER,"dgeomccylinder",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PGEOMPLANE,"dgeomplane",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PGEOMCYLINDER,"dgeomcylinder",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PGEOMRAY,"dgeomray",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PGEOMTRANSFORM,"dgeomtransform",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PJOINTGROUP,"djointgroup",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PJOINT,"djoint",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PJOINTCONTACT,"djointcontact",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PJOINTBALL,"djointball",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PJOINTHINGE,"djointhinge",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PJOINTHINGE2,"djointhinge2",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PJOINTSLIDER,"djointslider",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PJOINTUNIVERSAL,"djointuniversal",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PJOINTFIXED,"djointfixed",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PJOINTAMOTOR,"djointamotor",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PSPACE,"dspace",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PSPACESIMPLE,"dspacesimple",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PSPACEHASH,"dspacehash",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PSPACEQUAD,"dspacequad",PhysicsObject_free,NULL);
  Reference_registerType(LUXI_CLASS_PBODY,"dbody",PhysicsObject_free,NULL);

  for (i=0;i<0xff;i++)
  {
    l_materialdata[i].param.mode = dContactSlip1 | dContactSlip2 |
        dContactSoftERP | dContactSoftCFM | dContactApprox1;
    l_materialdata[i].param.mu = dInfinity;
    l_materialdata[i].param.slip1 = 0.1;
    l_materialdata[i].param.slip2 = 0.1;
    l_materialdata[i].param.soft_erp = 0.3;
    l_materialdata[i].param.soft_cfm = 0.1;
  }

  for (i=0;i<0xffff;i++)
    l_materialcombinations[i]=0;

  l_world = dWorldCreate();
  l_worldref = Reference_new(LUXI_CLASS_PWORLD,l_world);

  l_contactjoints = PhysicsObject_new(PHT_JOINTGROUP,(void*)dJointGroupCreate(0),LUXI_CLASS_PJOINTGROUP);
  l_collisions.buffer = gentypezalloc(dGeomID,4000);
  l_collisions.size = 4000;

  FunctionPublish_initClass(LUXI_CLASS_ODE,"ode",
    "Physical simulation using the ODE Lib.Use its childclasses for simulating \
physical behaviour of objects (@see http://www.ode.org). \
Read the ODE API documentation for better understanding, parts of it are included in this documentation. \
The luxinia API is simplifies some calls or renames them, but basicly, the functions are the same.\n\
A physical simulation can be divided in two object types: Static and dynamic objects. In ode, simulated \
objects that are staticly are called geoms, and objects that can interact and move dynamicly are \
called bodies. Geoms are solid (except trimeshes, which is the source of most problems when using them) \
objects, socalled primitives - cubes, spheres, cylinders and so on. \
Bodies are built using multiple geoms, allowing more complex objects for your simulation. \
The collision detection is done by using socalled spaces.",NULL,LUX_FALSE);

  FunctionPublish_initClass(LUXI_CLASS_PWORLD,"dworld",
    "Luxinia uses only one world (ode can simulate multiple worlds) for the sake \
of simplicity. The dworld class controls the parameters of the ode object. But it also \
has some special functions like a surfacemanagement system. You can specify 256 different \
types of surface parametersets that directly set how contacts will react at each other. \
Each dgeom has a surfaceid and the combination of the two surfaceids will tell the \
automatic contactgenerator what set of surfaceparameters should be used.",
    PubClass_Collision_deinit,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_PWORLD, LUXI_CLASS_ODE);
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubClass_world_new,NULL,
    "new","(dworld):() - creates a new world. Can be activated using dworld.active(world)."
    " Worlds share surfaceid data.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubClass_world_active,NULL,
    "active","([dworld]):([dworld]) - Sets or gets active dworld. Will be used for all following operations for that world.");
  //FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubClass_Collision_deleteall,0,
  //  "deleteall","():() - delete all collision stuff");

  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_RANDOMSEED,
    "randomseed","([int seed]):([int seed]) - set/get random seed (for all worlds)");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_GRAVITY,
    "gravity","(double x,y,z):([double x,y,z]) - set/get gravity");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_ERP,
    "erp","(double erp) : ([double erp]) - set/get Error Reduction Parameter (should be 0\
< erp < 1, default=0.2)");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_CFM,
    "cfm","(double erp) : ([double cfm]) - set/get Constraint force mixing parameter \
(should be around 10e-9 to 1, default=10e-5)");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_STEP,
    "step","() : (float stepsize) - make a step in simulation. We disadvise \
to use this function unless you are sure about its consequences. Simulations \
tend to produce strange error messages plus it is slower than quickstep.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_QUICKSTEP,
    "quickstep","() : (float stepsize) - make a quickstep in simulation");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_QUICKSTEPITER,
    "iterations","([int]) : ([int]) - sets/gets number of iterations for quickstep");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_COUNT,
    "count","(int) : () - returns sum of spaces,geoms,bodies,joints,jointgroups currently in memory");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_GET,
    "get","([dgeom/djoint/djointgroup]) : (int index) - \
each geom,joint and jointgroup (and all child classes) are stored in a global list that can \
be traversed.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_COLBUFSIZE,
    "maxcollisions","([int]) : (int size) - sets/gets maximum number of traceable collisions. \
Per default this is 4000.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_COLLISIONCOUNT,
    "collisioncount","([int]) : () - gets number of calculated collisions");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_COLBUFGET,
    "collisionget","([dgeom a,dgeom b]) : (int index) - gets \
two geoms that may are colliding");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_COLLIDE,
    "collidetest","() : (dspace space) - runs collisiondetection on space \
(and spaces in that space), resets collisionbuffer");
FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_CONTACTCOUNT,
    "contactcount","(n) : () - returns the current number of contacts in the world");
FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_JOINTEMPTY,
    "jointempty","() : () - Deletes all contact joints, automaticly done by makecontacts");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_MAKECONTACTS,
    "makecontacts","() : () - run this after collidetest if you \
want to autogenerate the contact joints. This will automaticly reset the default contactgroup.\
\n\n\
This method may crash if geoms has been deleted between the collidetest and the call on makecontacts. \
This can happen for example if the garbage collector collects a geom in between. Make sure that if an \
object is to be deleted, it is gone before you do the collidetest (by calling the :delete() function \
or enforcing the garbage collection (which is costly))");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_AUTODISABLE,
    "autodisable","([boolean]) : ([boolean on]) - if switched off, \
bodies won't be disabled. Disabled bodies are no longer consuming CPU power.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_AUTODISABLELINEARTHRESHOLD,
    "autodisablevel","([float]) : ([float]) - velocity at which \
the body is disabled.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_AUTODISABLEANGULARTHRESHOLD,
    "autodisableang","([float]) : ([float]) - angular velocity \
at which the body is disabled.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_AUTODISABLESTEPS,
    "autodisablesteps","([int]) : ([int]) - trace depth of \
activation if an enabled body hits a group of disabled bodies.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_AUTODISABLETIME,
    "autodisabletime","([float]) : ([float]) - minimum time \
until a body can be disabled after exceeding one of the thresholds.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_SURFACELAYER,
    "contactsurfacelayer","([float]):([float depth]) - Set and get the depth of the surface \
layer around all geometry objects. Contacts are allowed to sink into the surface layer up \
to the given depth before coming to rest. The default value is zero. Increasing this to \
some small value (e.g. 0.001) can help prevent jittering problems due to contacts being \
repeatedly made and broken.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_MAXCORVEL,
    "contactmaxcorrectingvel","([float]):([float]) - Set and get the maximum correcting \
velocity that contacts are allowed to generate. The default value is infinity (i.e. no limit). \
Reducing this value can help prevent \"popping\" of deeply embedded objects.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_DINFINITY,
    "dinfinity","(float):() - ODE Infinity value, used by some joints.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_MATERIALCOMB,
    "surfacecombination","([int]):(int s1,int s2,[int matid]) - sets/gets the result for the \
surfacecombination s1 and s2 (then the surface with surfaceid is selected for the contact parameters. \
The parameters s1 and s2 are symmetric, so the result for s2 and s1 is the same as for s1 and s2.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_MU,
    "surfacemu","([float]):(int surfaceid,[float]) - ");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_MU2,
    "surfacemu2","([float]):(int surfaceid,[float]) - ");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BOUNCE,
    "surfacebounce","([float]):(int surfaceid,[float]) - ");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_SOFTERP,
    "surfacesofterp","([float]):(int surfaceid,[float]) - ");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_SOFTCFM,
    "surfacesoftcfm","([float]):(int surfaceid,[float]) - ");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_MOTION1,
    "surfacemotion1","([float]):(int surfaceid,[float]) - ");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_MOTION2,
    "surfacemotion2","([float]):(int surfaceid,[float]) - ");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_SLIP1,
    "surfaceslip1","([float]):(int surfaceid,[float]) - ");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_SLIP2,
    "surfaceslip2","([float]):(int surfaceid,[float]) - ");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BOUNCEVEL,
    "surfacebouncevel","([float]):(int surfaceid,[float]) - ");

  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_FDIRMIXMODE,
    "surfacefdirmixmode","([int]):(int surfaceid,[int mode]) - The fdirmixmode specifies how \
the fdir vectors of different geoms should be handled. The mode values are: \n\
* use higher surfaceid = 0\n\
* use lower surfaceid = 1\n\
* use higher fdirpriority of geom = 2\n\
* use lower fdirpriority of geom = 3\n\
* use specified ID of surface = 4\n\
* mix the fdirs by average = 5\n\
* mix the fdirs using the priorities (prio1+prio2)/prio1*normal1 + (prio1+prio2)/prio2*normal2 = normal = 6\n\
\n\
* use higher surfaceid as world coordinates = 7\n\
* use lower surfaceid as world coordinates = 8\n\
* use higher priority as world coordinates = 9\n\
* use lower priority as world coordinates = 10\n\
* use specified ID of surface = 11\n\
* use average fdirs as world coordinates = 12\n\
* mix the fdirs as world coordinates based on priorities = 13\n");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_FDIID,
    "surfacefdirid","([int]):(int surfaceid,[int]) - specify the surfaceid of the geom that's \
fdir normal should be used. If both have the same surface id, the first geom found is used and \
the outcoming is undefined. The fdirid is only used if the fdirmixmode is set to 4 or 11.");


  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BITMU2,
    "surfacebitmu2","([boolean]):(int surfaceid,[boolean]) - \
If not set, use mu for both friction directions. If set, use mu for friction direction 1, \
use mu2 for friction direction 2.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BITBOUNCE,
    "surfacebitbounce","([boolean]):(int surfaceid,[boolean]) - \
If set, the contact surface is bouncy, in other words the bodies will bounce \
off each other. The exact amount of bouncyness is controlled by the bounce parameter.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BITSOFTERP,
    "surfacebitsofterp","([boolean]):(int surfaceid,[boolean]) - \
If set, the error reduction parameter of the contact normal can be set with the soft_erp \
parameter. This is useful to make surfaces soft.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BITSOFTCFM,
    "surfacebitsoftcfm","([boolean]):(int surfaceid,[boolean]) - \
If set, the constraint force mixing parameter of the contact normal can be set with the \
soft_cfm parameter. This is useful to make surfaces soft.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BITMOTION1,
    "surfacebitmotion1","([boolean]):(int surfaceid,[boolean]) - \
If set, the contact surface is assumed to be moving independently of the motion of the bodies. \
This is kind of like a conveyor belt running over the surface. When this flag is set, motion1 \
defines the surface velocity in friction direction 1.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BITMOTION2,
    "surfacebitmotion2","([boolean]):(int surfaceid,[boolean]) - The same thing as above, \
but for friction direction 2.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BITSLIP1,
    "surfacebitslip1","([boolean]):(int surfaceid,[boolean]) - Force-dependent-slip (FDS) \
in friction direction 1.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BITSLIP2,
    "surfacebitslip2","([boolean]):(int surfaceid,[boolean]) - Force-dependent-slip (FDS) \
in friction direction 2.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BITAPPROX1,
    "surfacebitapprox1","([boolean]):(int surfaceid,[boolean]) - Use the friction pyramid \
approximation for friction direction 1. If this is not specified then the constant-force-limit \
approximation is used (and mu is a force limit).");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BITAPPROX2,
    "surfacebitapprox2","([boolean]):(int surfaceid,[boolean]) - Use the friction pyramid \
approximation for friction direction 2. If this is not specified then the constant-force-limit \
approximation is used (and mu is a force limit).");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_BITFDIR,
    "surfacebitfdir","([boolean]):(int surfaceid,[boolean]) - Compute the fdir vector using the \
given normals of the geoms.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_ACTIVEBODIES,
    "activebodies","(int):() - Returns the number of currently active bodies.");
  FunctionPublish_addFunction(LUXI_CLASS_PWORLD,PubCollision_worldprop,(void*)WP_GETACTIVEBODY,
    "getactivebody","([dbody]):(int index) - Returns active (enabled) body of given index. \
    If the index is larger than the active body count, nil is returned.");
///////// GEOMS ////////////////////////////////////////////////////////////////
///////// GEOMS ////////////////////////////////////////////////////////////////
///////// GEOMS ////////////////////////////////////////////////////////////////

  FunctionPublish_initClass(LUXI_CLASS_PCOLLRESULT,"dcollresult",
    "The tesult of manual collision results are stored here. The returned count can be higher than"
    "the detailed contact info stored within and prevents gc of stored dgeoms.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_PCOLLRESULT, LUXI_CLASS_ODE);
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_new,NULL,
    "new","(dcollresult):([int maxcontacts],[int maxraytriangles],[int maxcollecttri]) - "
    "returns new dcollresult with the given number of maximum contact info. (default is 128)");

  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_test,NULL,
    "test","(int count):(dcollresult,dcollider,dcollider) - "
         "tests two dcolliders against each other and returns the number of contacts.");

  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_testnear,NULL,
    "testnear","(int count):(dcollresult, dcollider, dcollider) - "
    "tests two dcolliders if they are (have subnodes) near each other (aaboxes overlap).");
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_detail,(void*)PCOLL_MAX,
    "max","(int contacts, raytri, triarray):(dcollresult) - returns maximum contacts");
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_detail,(void*)PCOLL_COMPLETE,
    "complete","(bool):(dcollresult) - returns whether all contacts were stored.");
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_detail,(void*)PCOLL_CLEAR,
    "clear","():(dcollresult) - clears contact info, releases dgeoms for gc. (automatically called before each test)");
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_detail,(void*)PCOLL_NUM,
    "contacts","(int):(dcollresult) - returns current contact count (may be smaller than returned hit counts).");
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_detail,(void*)PCOLL_GEOM,
    "geoms","(dgeom,dgeom):(dcollresult, int index) - returns ith contact geoms.");
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_detail,(void*)PCOLL_POS,
    "pos","(float x,y,z):(dcollresult, int index) - returns ith contact position");
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_detail,(void*)PCOLL_NORMAL,
    "normal","(float x,y,z, d):(dcollresult, int index) - returns ith contact normal and depth of penetration.");
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_detail,(void*)PCOLL_TRIINFO,
    "triinfo","(int num, int offset):(dcollresult, int index) - returns number and offset of ith triangle contact. Used to index triaarray. Only generated during testtriarray, might be 0 if ran out of space for triangle indices");
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_detail,(void*)PCOLL_TRIARRAY,
    "triarray","(table int):(dcollresult) - returns table of all recorded triangle array collisions.");
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_detail,(void*)PCOLL_RAYTRI,
    "raytri","(int triidx, float u, v):(dcollresult, int index) - returns ith triangle sub info, if present. Needs ray callback activated for trimesh.");


  FunctionPublish_addFunction(LUXI_CLASS_PCOLLRESULT,PhysicsCollResult_testtriarray,NULL,
  "testtriarray","(int count):(dcollresult, dcollider a, dcollider b) -"
  "tests two geoms for intersecting triangles. Only works if triarrayresult is activated for the included trimeshgeoms. "
  "Each collision pair results into trigeom/geom contact. Use triinfo and triarray to query for details. Position and normal info will be undefined, as well as raytri.");


  FunctionPublish_initClass(LUXI_CLASS_PCOLLIDER,"dcollider",
    "Subclasses can perform collision detection.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_PCOLLIDER, LUXI_CLASS_ODE);
  FunctionPublish_addFunction(LUXI_CLASS_PCOLLIDER,PubGeom_destroy, NULL,
    "delete","() : (dcollider) - deletes a dcollider");

  FunctionPublish_initClass(LUXI_CLASS_PGEOM,"dgeom",
    "Geoms are solid objects that can collide with other geoms.", NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_PGEOM, LUXI_CLASS_PCOLLIDER);
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_FDIRNORMAL,
      "fdirnormal","([float x,y,z]) : (dgeom geom,[float x,y,z]) - sets/gets \
normal vector to the friction direction vector that defines a direction along \
which frictional force is applied. The friction direction vector is computed as the \
crossproduct of the contactnormal and the normal vector that is defined here.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_FDIRPRIO,
      "fdirpriority","([float]) : (dgeom geom,[float]) - sets/gets \
priority of the fdirnormal. The priority can be used in different ways.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_MOTION1,
      "motion1","([float]) : (dgeom geom,[float]) - sets/gets \
motion values of surface. This means that the contact will look like in \
motion for other objects that are in contact with this geom. Behavior is \
undefined if the fdirmixing is set to priority or average.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_MOTION2,
      "motion2","([float]) : (dgeom geom,[float]) - sets/gets \
motion values of surface. This means that the contact will look like in \
motion for other objects that are in contact with this geom. Behavior is \
undefined if the fdirmixing is set to priority or average.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeomBody_pos,NULL,
      "pos","([float x,y,z]) : (dgeom geom,[float x,y,z]) - sets/gets position of geom");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeomBody_matrix,NULL,
      "matrix","([matrix4x4]) : (dgeom geom,[matrix4x4]) - sets/gets matrix of geom");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeomBody_rot,(void*)ROT_RAD,
      "rotrad","([float x,y,z]) : (dgeom geom,[float x,y,z]) - sets/gets \
rotation of geom. The angles must be calculated from a matrix, so the results \
can differ from what you set. These matrix calculations are quite expensive, \
so use it with care. This functions uses x,y,z as angles in radians (180°==PI)");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeomBody_rot,(void*)ROT_DEG,
      "rotdeg","([float x,y,z]) : (dgeom geom,[float x,y,z]) - sets/gets \
rotation of geom. The angles must be calculated from a matrix, so the results \
can differ from what you set. These matrix calculations are quite expensive, \
so use it with care. This function uses x,y,z as angles in degrees (180°==PI).");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeomBody_rot,(void*)ROT_AXIS,
      "rotaxis","([float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]) : \
(dgeom geom,[float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz])) - \
returns or sets local rotation axis, make sure they make a orthogonal basis.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeomBody_rot,(void*)ROT_QUAT,
      "rotquat","([float x,y,z,w]) : (dgeom geom,[float x,y,z,w]) - sets/gets \
           rotation of geom as quaternion.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_MATRIXTRANSFORM,
      "local2world","(float x,y,z) : (dgeom geom,float x,y,z) - transforms the given \
coordinates with the matrix of the geom, as if x,y,z are local coordinates of the geom and \
world coordinates are returned.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_MATRIXINVTRANSFORM,
      "world2local","(float x,y,z) : (dgeom geom,float x,y,z) - transforms the given \
coordinates from world coordinates into local coordinates of the geom.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_ENABLEDSPACE,
      "enabledspace","([dspace]) : (dgeom geom,[dspace]) - The enabledspace is the space a \
geom is put into if it has no body or if its body is enabled. This is usefull if you have a large \
amount of bodies that should collide with each other but that are disabled most of the time.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_DISABLEDSPACE,
      "disabledspace","([dspace]) : (dgeom geom,[dspace]) - The disabledspace is the space a \
geom is put into if it has a body that is disabled. This is usefull if you have a large \
amount of bodies that should collide with each other but that are disabled most of the time.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_prop, (void*)PHT_BODY,
      "body","(dbody body) : (dgeom geom, [dbody body/other]) - sets/gets the geom - if 2nd arg \
is not a dbody, it removes the geom from the body. The geom must not be transformed \
if a body is set. If the geom is transformed and the body is requested, the function \
will look look up the body of the transformer in a loop.");
        FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_prop,(void*)PHT_MAXCONTACTINFO,
            "maxcontactinfo","([int]) : (dgeom geom, [int]) - \
sets/gets the maximum number of generated contact info. The contactinfo is a structure \
that stores the coordinates, the normal and the depth of a contact. This is normaly also \
stored in a contactjoint that is generated from a contact. However, if you want to \
create a geom that only works as a sensor for collision (i.e. if a door should open if \
someone stands in front of it), and turn nocontactgeneration on, you can retrieve \
contactinformation from such geoms.");
        FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_prop,(void*)PHT_NOCONTACTGENERATOR,
            "nocontactgenerator","([boolean]) : (dgeom geom, [boolean]) - \
sets/gets if contacts should be generated for this geom. You might want to deactivate \
contactgeneration for geoms that only should act as sensors in your physical environment.");
        FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_prop,(void*)PHT_GETCONTACTINFO,
            "getcontactinfo", "(int / float x,y,z, nx,ny,nz, depth, dgeom othergeom):(dgeom geom, [int index])\
If contacts have been generated in your simulation and you have set the maxcontactinfo \
value for this geom to a value>0, a collision generation that creates contacts will also \
store the contact results in the contactinfo of the geom. You can read out the values \
for contact generation with this function. If no index is given as argument, the \
number of stored contactinformation are returned for this geom. If an index is passed, \
the contactinformation for this index is returned. The index must be >=0 and <contactcount \
in order to return valid values. The returned values are the position, the normalvector \
and the depth (distance) of the penetraction.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_prop,(void*)PHT_MAXCONTACTS,
      "maxcontacts","([int]) : (dgeom geom,[int]) - \
sets/gets maximum number of contacts for this geom. If two geoms are tested \
for collision, multiple contacts can be generated for this case. If a geom has a limited \
number of possible contacts (set <=0 for deactivation of maximum), the minimum \
of maxcontacts of both geoms is chosen. Reducing the number of contacts max \
boost up the simulation, but can cause jittering of bodies, making autodisabling \
useless. Defaultvalue is 0 (no limit), but the general limit \
of contacts per geom is 128. The effect depends on the geom type. For example, \
two colliding spheres will never generate more than one contact, while \
two colliding boxes can generate not more than 4 contacts. Only if a geom collides \
with a complex trimesh, the limit can be exceeded.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_CATEGORY,
      "category","([boolean]):(dgeom,int set {0-1}, int bit {0-31}, [boolean on]) - \
Sets ODE's collide and category bits. A geom can gollidewith other geoms if cat1&col2 or cat2&col1 are \
not zero.\n\n\
ODE uses collide and categorybits to reject geoms for collision. They can be used \
to prevent collision between certain geoms. A collision is rejected if both the bitset 1 \
of geom A doesn't have bits in common with the bitset 2 of geom B, and if the bitset 2 of \
geom A doesn't have bits in common with bitset 1 of geom B.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_SETCATEGORY,
      "setcategory","([int]):(dgeom,int set {0,1}) - sets/gets raw category bits as number");


    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_TRANSFORMER,
      "transformer","([dgeomtransform]):(dgeom) - returns the geomtransform \
that transforms the geom or returns nothing if the geom is not transformed. Note \
that you might get the 'final' geom in the transformhierarchy if you get a collision \
feedback, what may not be what you want / need.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOM,PubGeom_paramProp,(void*)PGPP_MATERIALID,
      "surfaceid","([int]):(dgeom,[int]) - sets/gets surfaceid of geom (0-255). If two geoms \
generate a contact, the surfaceproperties must be specified. These properties can be \
rather complicated and may depend from the type of the surface (rubber on glas won't slide, \
wood on glas slides, both slides on ice, but both don't slide at each other very well, etc.). \
In order to give you as much control as possible, yet keeping it as simple as possible, you can \
specify every surfacecombination that you wish by defining a list of contactparameters and defining \
the surface to be used by a combination of surfaceids.");


    FunctionPublish_initClass(LUXI_CLASS_PGEOMTRIMESH,"dgeomtrimesh",
    "Triangle mesh collision.", NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PGEOMTRIMESH, LUXI_CLASS_PGEOM);
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMTRIMESH,PubGeom_new,(void*)LUXI_CLASS_PGEOMTRIMESH,
      "new","(dgeomtrimesh):(dgeomtrimeshdata,[dspace space]) - \
creates a trimesh geom, ready for collision. ==new prevents gc of dgeomtrimeshdata");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMTRIMESH,PubGeom_prop,(void*)PHT_RAYTRIRESULTS,
      "raytrisresult","([boolean]):(dgeomtrimesh,[boolean]) - \
You can get precise ray test results, such as index of which triangle was hit and which barycentric coordinates the hitpoint had.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMTRIMESH,PubGeom_prop,(void*)PHT_TRISARRAYRESULTS,
      "triarrayresult","([boolean]):(dgeomtrimesh,[boolean]) - \
You can get all intersecting triangles with the geoms when using triarraycollide command. Outside of calling that function you should disable manually, else there will be a slight slowdown.");


    FunctionPublish_initClass(LUXI_CLASS_PGEOMTRIMESHDATA,"dgeomtrimeshdata",
    "Trimesh data that is used for dgeomtrimesh. You can instantiate one trimeshdata and \
reuse it multiple times.",NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PGEOMTRIMESHDATA, LUXI_CLASS_ODE);
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMTRIMESHDATA,PubGeom_new,(void*)LUXI_CLASS_PGEOMTRIMESHDATA,
      "new","(dgeomtrimeshdata):(table triangleindices,table vertexpositions) - \
Creates a trimeshdataset. Indices and positions are stored as continous streams. For example ind{0,1,2, 3,4,5, ...} pos={x,y,z, x1,y1,z1, ...}");


    FunctionPublish_initClass(LUXI_CLASS_PGEOMBOX,"dgeombox",
    "A solid box object.", NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PGEOMBOX, LUXI_CLASS_PGEOM);
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMBOX,PubGeom_new,(void*)LUXI_CLASS_PGEOMBOX,
      "new","(dgeombox box) : ([float w,h,l],[dspace space]) - \
creates a box with width=w, height=h and length=l");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMBOX,PubGeom_paramProp,(void*)PGPP_BOXSIZE,
      "size","([float x,y,z]):(dgeombox,[float x,y,z]) - sets/gets size of the box");


    FunctionPublish_initClass(LUXI_CLASS_PGEOMSPHERE,"dgeomsphere",
    "A sphere with a radius, the fastest and most simple collission solid.",
    NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PGEOMSPHERE, LUXI_CLASS_PGEOM);
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMSPHERE,PubGeom_new,(void*)LUXI_CLASS_PGEOMSPHERE,
      "new","(dgeomsphere sphere) : ([float r],[dspace space]) - \
creates a new sphere with radius r. Inserts the sphere in space, if given");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMSPHERE,PubGeom_paramProp,(void*)PGPP_SPHERERADIUS,
      "r","([float r]) : (dgeomsphere sphere,[float r]) - sets/gets radius of sphere");


    FunctionPublish_initClass(LUXI_CLASS_PGEOMCCYLINDER,"dgeomccylinder",
    "A capped cylinder is like a normal cylinder except it has half-sphere caps at its ends. \
This feature makes the internal collision detection code particularly fast and accurate. \
The cylinder's length, not counting the caps, is given by length. The cylinder is aligned \
along the geom's local Z axis. The radius of the caps, and of the cylinder itself, is \
given by radius.", NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PGEOMCCYLINDER, LUXI_CLASS_PGEOM);
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMCCYLINDER,PubGeom_new,(void*)LUXI_CLASS_PGEOMCCYLINDER,
      "new","(dgeomccylinder) : ([float r,l],[dspace space]) - Create a capped \
cylinder geom of the given parameters, and return its ID. If space is nonzero, insert it \
into that space.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMCCYLINDER,PubGeom_paramProp,(void*)PGPP_CCYLLENGTH,
      "length","([float]):(dgeomccylinder,[float]) - sets/gets the length of the dgeomccylinder");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMCCYLINDER,PubGeom_paramProp,(void*)PGPP_CCYLRADIUS,
      "radius","([float]):(dgeomccylinder,[float]) - sets/gets the radius of the dgeomccylinder");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMCCYLINDER,PubGeom_paramProp,(void*)PGPP_CCYLLENGTHRAD,
      "lenrad","([float rad,float len]):(dgeomccylinder,[float rad, float len]) - \
sets/gets the radius and length of the dgeomccylinder");


    FunctionPublish_initClass(LUXI_CLASS_PGEOMPLANE,"dgeomplane",
    "Planes are infinite and so non-placeable \
geoms. This means that, unlike placeable geoms, planes do not have an assigned position \
and rotation. This means that the parameters (a,b,c,d) are always in world coordinates. \
In other words it is assumed that the plane is always part of the static environment and \
not tied to any movable object.", NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PGEOMPLANE, LUXI_CLASS_PGEOM);
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMPLANE,PubGeom_new,(void*)LUXI_CLASS_PGEOMPLANE,
      "new","(dgeomplane) : ([float a,b,c,d],[dspace]) - Create a plane geom of the given \
parameters, and return its ID. If space is given, insert it into that space. The plane \
equation is a*x+b*y+c*z = d \n\
The plane's normal vector is (a,b,c), is normalized automaticly. ");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMPLANE,PubGeom_paramProp,(void*)PGPP_PLANENORMAL,
      "normal","([float x,y,z]):(dgeomplane,[float x,y,z]) - sets the normal vector of \
the plane. Luxinia normalizes this vector automaticly for you.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMPLANE,PubGeom_paramProp,(void*)PGPP_PLANEDISTANCE,
      "distance","([float d]):(dgeomplane,[float d]) - sets the distance of the plane to the \
origin (0,0,0)");


    FunctionPublish_initClass(LUXI_CLASS_PGEOMCYLINDER,"dgeomcylinder",
    "Flatended cylinder. The cylinder is in the unstable branch of ODE. It doesn't collide with \
other cylinders or planes. It also seems to react unstable in certain positions.", NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PGEOMCYLINDER, LUXI_CLASS_PGEOM);
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMCYLINDER,PubGeom_new,(void*)LUXI_CLASS_PGEOMCYLINDER,
      "new","(dgeomcylinder) : (float radius,length) - creates a flatended cylinder");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMCYLINDER,PubGeom_paramProp,(void*)PGPP_CCYLLENGTH,
      "length","([float]):(dgeomccylinder,[float]) - sets/gets the length of the dgeomccylinder");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMCYLINDER,PubGeom_paramProp,(void*)PGPP_CCYLRADIUS,
      "radius","([float]):(dgeomccylinder,[float]) - sets/gets the radius of the dgeomccylinder");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMCYLINDER,PubGeom_paramProp,(void*)PGPP_CCYLLENGTHRAD,
      "lenrad","([float rad,float len]):(dgeomccylinder,[float rad, float len]) - \
sets/gets the radius and length of the dgeomccylinder");

/*  FunctionPublish_initClass(LUXI_CLASS_PGEOMCONE,"dgeomcone",
    "Flatended cone.", NULL,NULL,NULL);
    FunctionPublish_setParent(LUXI_CLASS_PGEOMCONE, LUXI_CLASS_PGEOM);
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMCONE,PubGeom_new,(void*)LUXI_CLASS_PGEOMCONE,
      "new","(dgeomcone) : (float radius,length) - creates a flatended cone");*/



    FunctionPublish_initClass(LUXI_CLASS_PGEOMRAY,"dgeomray",
    "A ray is different from all the other geom classes in that it does not represent \
a solid object. It is an infinitely thin line that starts from the geom's position and extends \
in the direction of the geom's local Z-axis.\n\
Calling dCollide between a ray and another geom will result in at most one contact point. \
Rays have their own conventions for the contact information in the dContactGeom structure \
(thus it is not useful to create contact joints from this information):\n\
* pos - This is the point at which the ray intersects the surface of the other geom, \
regardless of whether the ray starts from inside or outside the geom.\n\
* normal - This is the surface normal of the other geom at the contact point. \
if dCollide is passed the ray as its first geom then the normal will be oriented \
correctly for ray reflection from that surface (otherwise it will have the opposite sign).\n\
* depth - This is the distance from the start of the ray to the contact point. \n\
Rays are useful for things like visibility testing, determining the path of projectiles \n\
or light rays, and for object placement.", NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PGEOMRAY, LUXI_CLASS_PGEOM);
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMRAY,PubGeom_new,(void*)LUXI_CLASS_PGEOMRAY,
      "new","(dgeomray) : ([float length],[dspace]) - Create a ray geom of the \
given length, and return its ID. If space is given, insert it into that space.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMRAY,PubGeom_paramProp,(void*)PGPP_RAYSET,
      "set","():(dgeomray,float x,y,z, dx,dy,dz) - set position and direction of ray in space. \
The length of the ray will NOT be changed by using this function.");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMRAY,PubGeom_paramProp,(void*)PGPP_RAYGET,
      "get","(float x,y,z, dx,dy,dz):(dgeomray) - returns position and normalized \
direction of the ray");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMRAY,PubGeom_paramProp,(void*)PGPP_RAYLENGTH,
      "length","([float len]):(dgeomray,[float len]) - sets/gets length of ray");


    FunctionPublish_initClass(LUXI_CLASS_PGEOMTRANSFORM,"dgeomtransform",
    "A geometry transform `T' is a geom that encapsulates another geom `E', \
allowing E to be positioned and rotated arbitrarily with respect to its point \
of reference.\n\
Most placeable geoms (like the sphere and box) have their point of reference \
corresponding to their center of mass, allowing them to be easily connected to \
dynamics objects. Transform objects give you more flexibility - for example, \
you can offset the center of a sphere, or rotate a cylinder so that its axis is \
something other than the default.\n\
T mimics the object E that it encapsulates: T is inserted into a space and \
attached to a body as though it was E. E itself must not be inserted into a \
space or attached to a body. E's position and rotation are set to constant \
values that say how it is transformed relative to T. If E's position and \
rotation are left at their default values, T will behave exactly like E would \
have if you had used it directly.", NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PGEOMTRANSFORM, LUXI_CLASS_PGEOM);
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMTRANSFORM,PubGeom_new,(void*)LUXI_CLASS_PGEOMTRANSFORM,
      "new","(dgeomtranform) : ([dspace space]) - \
creates a new transforminst");
    FunctionPublish_addFunction(LUXI_CLASS_PGEOMTRANSFORM,PubGeomTransform_geom,NULL,
      "geom","([dgeom]) : (dgeomtranform self, [dgeom]) - \
sets/gets the geom to be transformed. The passed geom must not be inserted in a \
space or must not be transformed.");

///////// JOINTS ///////////////////////////////////////////////////////////////
///////// JOINTS ///////////////////////////////////////////////////////////////
///////// JOINTS ///////////////////////////////////////////////////////////////

  FunctionPublish_initClass(LUXI_CLASS_PJOINTGROUP,"djointgroup",
    "djoints can be organized in groups", NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_PJOINTGROUP, LUXI_CLASS_ODE);


    FunctionPublish_initClass(LUXI_CLASS_PJOINT,"djoint",
    "Joints are connecting bodies. They simulate contacts, varius hinges and \
slider joints. For example if a chair stands on the floor, each of his foots are \
connected with a djointcontact to the floor or if a car steers with the wheels, \
ODE uses a jointhinge2 to simulate this. Ragdoll physics is also a typical example.", NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_PJOINT, LUXI_CLASS_ODE);
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJ_FEEDBACK,
    "feedback","([float xf1,yf1,zf1, xt1,yt1,zt1, xf2,yf2,zf2, xt2,yt2,zt2):(djoint)\
 - returns force and torque applied to the connected bodies of the joint. \
Since some joints must not be connected to two bodies, either the force and torque \
of body1 or body2 may not be valid. ");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJ_ATTACH,
    "attach","():(djoint,[dbody],[dbody]) - connects the joint to one or two \
bodies. Some joints may need two bodies while other joints will merely accept anything. \
If nothing is passed, the joint is removed from the bodies it was attached to.");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJ_GEOM1,
    "geom1","([dgeom]):(djoint) - returns geom1 of the joint it has collided with");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJ_GEOM2,
    "geom2","([dgeom]):(djoint) - returns geom2 of the joint it has collided with");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJ_BODY1,
    "body1","([dbody]):(djoint) - returns body1 of the joint or nil if not connected");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJ_BODY2,
    "body2","([dbody]):(djoint) - returns body2 of the joint or nil if not connected");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubGeom_destroy, NULL,
    "delete","() : (djoint) - deletes the joint, making it invalid.");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_LOSTOP,
    "lostop","([float]):(djoint,[float]) - Low stop angle or position. \
Setting this to -dInfinity (the default value) turns off the low stop. For rotational \
joints, this stop must be greater than - pi to be effective.");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_LOSTOP2,
    "lostop2","([float]):(djoint,[float]) - same as lostop but for the 2nd axis (if exists)");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_LOSTOP3,
    "lostop3","([float]):(djoint,[float]) - same as lostop but for the 3rd axis (if exists)");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_HISTOP,
    "histop","([float]):(djoint,[float]) - High stop angle or position. Setting this to \
dInfinity (the default value) turns off the high stop. For rotational joints, this stop must \
be less than pi to be effective. If the high stop is less than the low stop then both stops \
will be ineffective.");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_HISTOP2,
    "histop2","([float]):(djoint,[float]) - same as histop but for the 2nd axis (if exists)");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_HISTOP3,
    "histop3","([float]):(djoint,[float]) - same as histop but for the 3rd axis (if exists)");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_BOUNCE,
    "bounce","([float]):(djoint,[float]) - The bouncyness of the stops. This is a restitution \
parameter in the range 0..1. 0 means the stops are not bouncy at all, 1 means maximum bouncyness.");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_BOUNCE2,
    "bounce2","([float]):(djoint,[float]) - same as bounce but for the 2nd axis (if exits)");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_BOUNCE3,
    "bounce3","([float]):(djoint,[float]) - same as bounce but for the 3rd axis (if exits)");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_CFM,
    "cfm","([float]):(djoint,[float]) - The constraint force mixing (CFM) value used when not at a stop.");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_CFM2,
    "cfm2","([float]):(djoint,[float]) - same as cfm but for the 2nd axis (if exists)");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_CFM3,
    "cfm3","([float]):(djoint,[float]) - same as cfm but for the 3rd axis (if exists)");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_STOPERP,
    "stoperp","([float]):(djoint,[float]) - The error reduction parameter (ERP) used by the stops.");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_STOPERP2,
    "stoperp2","([float]):(djoint,[float]) - same as stoperp");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_STOPERP3,
    "stoperp3","([float]):(djoint,[float]) - same as stoperp");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_STOPCFM,
    "stopcfm","([float]):(djoint,[float]) - The constraint force mixing (CFM) value used by \
the stops. Together with the ERP value this can be used to get spongy or soft stops. Note that \
this is intended for unpowered joints, it does not really work as expected when a powered joint \
reaches its limit.");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_STOPCFM2,
    "stopcfm2","([float]):(djoint,[float]) - same as stopcfm");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_STOPCFM3,
    "stopcfm3","([float]):(djoint,[float]) - same as stopcfm");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_VEL,
    "velocity","([float]):(djoint,[float]) - Desired motor velocity axis 1\
(this will be an angular or linear velocity).");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_VEL2,
    "velocity2","([float]):(djoint,[float]) - Desired motor velocity axis 2");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_VEL3,
    "velocity3","([float]):(djoint,[float]) - Desired motor velocity axis 3");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_FMAX,
    "fmax","([float]):(djoint,[float]) - The maximum force or torque \
that the motor will use to achieve the desired velocity. This must always be greater than or \
equal to zero. Setting this to zero (the default value) turns off the motor.");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_FMAX2,
    "fmax2","([float]):(djoint,[float]) - same as fmax but for axis 2");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_FMAX3,
    "fmax3","([float]):(djoint,[float]) - same as fmax but for axis 3");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_FUDGEFAC,
    "fudgefactor","([float]):(djoint,[float]) - The current joint \
stop/motor implementation has a small problem: when the joint is at one stop and the motor \
is set to move it away from the stop, too much force may be applied for one time step, causing \
a 'jumping' motion. This fudge factor is used to scale this excess force. It should have a \
value between zero and one (the default value). If the jumping motion is too visible in a joint, \
the value can be reduced. Making this value too small can prevent the motor from being able to \
move the joint away from a stop.");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_FUDGEFAC2,
    "fudgefactor2","([float]):(djoint,[float]) - same as fudgefactor but for axis 2");
  FunctionPublish_addFunction(LUXI_CLASS_PJOINT,PubJoint_prop,(void*)PJP_FUDGEFAC3,
    "fudgefactor3","([float]):(djoint,[float]) - same as fudgefactor but for axis 3");


    FunctionPublish_initClass(LUXI_CLASS_PJOINTCONTACT,"djointcontact",
    "The contact joint prevents body 1 and body 2 from inter-penetrating at \
the contact point. It does this by only allowing the bodies to have an 'outgoing' \
velocity in the direction of the contact normal. Contact joints typically have a \
lifetime of one time step. They are created and deleted in response to collision detection.\n\
Contact joints can simulate friction at the contact by applying special forces in \
the two friction directions that are perpendicular to the normal.",
    NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PJOINTCONTACT, LUXI_CLASS_PJOINT);
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_POS,
      "pos","(float x,y,z):(djointcontact) - returns point of contact");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_NORMAL,
      "normal","(float x,y,z):(djointcontact) - returns normal of contact");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_DEPTH,
      "depth","(float x,y,z):(djointcontact) - returns depth of contact");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_MU1,
      "mu","([boolean,float]):(djointcontact,[float]) - \
Coulomb friction coefficient. This must be in the range 0 to dInfinity. 0 results \
in a frictionless contact, and dInfinity results in a contact that never slips. Note \
that frictionless contacts are less time consuming to compute than ones with friction, \
and infinite friction contacts can be cheaper than contacts with finite friction.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_MU2,
      "mu2","([boolean,float]):(djointcontact,[boolean on],[float]) - \
Coulomb friction coefficient for friction direction 2 (0..dInfinity).");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_FDIR,
      "fdir","([boolean,float x,y,z]):(djointcontact,[boolean on],[float x,y,z]) -\
\"first friction direction\" vector that defines a direction along which frictional force \
is applied. It must be of unit length and perpendicular to the contact normal (so it \
is typically tangential to the contact surface). The \"second friction direction\" \
is a vector computed to be perpendicular to both the contact normal and fdir1.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_BOUNCE,
      "bounce","([boolean,float]):(djointcontact,[boolean on],[float]) - \
Restitution parameter (0..1). 0 means the surfaces are not bouncy at all, 1 is maximum bouncyness.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_BOUNCEVEL,
      "bouncevel","([boolean,float]):(djointcontact,[boolean on],[float]) - \
The minimum incoming velocity necessary for bounce (in m/s). Incoming velocities below this \
will effectively have a bounce parameter of 0.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_SOFTERP,
      "softerp","([boolean,float]):(djointcontact,[boolean on],[float]) - \
Contact normal 'softness' parameter.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_SOFTCFM,
      "softcfm","([boolean,float]):(djointcontact,[boolean on],[float]) - \
Contact normal 'softness' parameter.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_MOTION1,
      "motion1","([boolean,float]):(djointcontact,[boolean on],[float]) - \
Surface velocity in friction directions 1 (in m/s).");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_MOTION2,
      "motion2","([boolean,float]):(djointcontact,[boolean on],[float]) - \
Surface velocity in friction directions 2 (in m/s).");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_SLIP1,
      "slip1","([boolean,float]):(djointcontact,[boolean on],[float]) - \
The coefficients of force-dependent-slip (FDS) for friction directions 1 (read the ode-doc).");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_SLIP2,
      "slip2","([boolean,float]):(djointcontact,[boolean on],[float]) - \
The coefficients of force-dependent-slip (FDS) for friction directions 2 (read the ode-doc).");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_APPROX1_1,
      "approx1","([boolean,float]):(djointcontact,[boolean on],[float]) - \
Use the friction pyramid approximation for friction direction 1. If this is not specified \
then the constant-force-limit approximation is used (and mu is a force limit).");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTCONTACT,PubJoint_prop,(void*)PJC_APPROX1_2,
      "approx2","([boolean,float]):(djointcontact,[boolean on],[float]) - \
Use the friction pyramid approximation for friction direction 2. If this is not specified \
then the constant-force-limit approximation is used (and mu is a force limit).");


    FunctionPublish_initClass(LUXI_CLASS_PJOINTBALL,"djointball",
    "A ball-joint that rotates without limits. Can be used to simulate ropes for example",
    NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PJOINTBALL, LUXI_CLASS_PJOINT);
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTBALL,PubGeom_new,(void*)LUXI_CLASS_PJOINTBALL,
      "new","(djointball) : ([djointgroup group]) - creates a new balljoint");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTBALL,PubJoint_prop,(void*)PJB_ANCHOR,
      "anchor","([float x1,y1,z1,x2,y2,z2]):(djointball,[float x,y,z]) - \
sets anchor point of joint if x,y,z is given (world coordinates) \
or returns the current anchors of the bodies if no argument is passed. \
Since the simulation is imperfect, there's a gap between the anchor that is set \
and the actual anchors of the bodies.");


    FunctionPublish_initClass(LUXI_CLASS_PJOINTHINGE,"djointhinge",
    "Hinges are limited in their rotation at one axis. Can be used for carwheel simulation.",
    NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PJOINTHINGE, LUXI_CLASS_PJOINT);
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE,PubGeom_new,(void*)LUXI_CLASS_PJOINTHINGE,
      "new","(djointhinge) : ([float x,y,z],[djointgroup group]) - creates a new hingejoint");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE,PubJoint_prop,(void*)PJH_ANCHOR,
      "anchor","([float x1,y1,z1,x2,y2,z2]):(djointhinge,[float x,y,z]) - sets anchor point \
in worldcoordinates if x,y,z is given. If not, the two anchorpoints are returned that \
both bodies apply. ");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE,PubJoint_prop,(void*)PJH_AXIS,
      "axis","([float x,y,z]):(djointhinge,[float x,y,z]) - sets/gets axis orientation of the hinge ");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE,PubJoint_prop,(void*)PJH_ANGLE,
      "angle","([float angle,rate]):(djointhinge) - sets/gets axis orientation of the hinge.\
The angle is measured between the two bodies, or between the body and the static \
environment. The angle will be between -pi..pi.\n\
When the hinge anchor or axis is set, the current position of the attached bodies \
is examined and that position will be the zero angle. ");


    FunctionPublish_initClass(LUXI_CLASS_PJOINTHINGE2,"djointhinge2",
    "The hinge-2 joint is the same as two hinges connected in series, with different \
hinge axes. An example, shown in the above picture is the steering wheel of a car, where \
one axis allows the wheel to be steered and the other axis allows the wheel to rotate.\n\
The hinge-2 joint has an anchor point and two hinge axes. Axis 1 is specified relative to \
body 1 (this would be the steering axis if body 1 is the chassis). Axis 2 is specified \
relative to body 2 (this would be the wheel axis if body 2 is the wheel).\n\
Axis 1 can have joint limits and a motor, axis 2 can only have a motor.\n\
Axis 1 can function as a suspension axis, i.e. the constraint can be compressible along that axis.\n\
The hinge-2 joint where axis1 is perpendicular to axis 2 is equivalent to a universal \
joint with added suspension.",
    NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PJOINTHINGE2, LUXI_CLASS_PJOINT);
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE2,PubGeom_new,(void*)LUXI_CLASS_PJOINTHINGE2,
      "new","(djointhinge2) : ([djointgroup group]) - creates a new hinge2joint");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE2,PubJoint_prop,(void*)PJH2_ANCHOR,
      "anchor","([float x,y,z]):(djointhinge2,[float x,y,z]) - Sets/gets hinge-2 anchor.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE2,PubJoint_prop,(void*)PJH2_AXIS1,
      "axis1","([float x,y,z]):(djointhinge2,[float x,y,z]) - \
Sets/gets hinge-2 axis1. Axis 1 and axis 2 must not lie along the same line.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE2,PubJoint_prop,(void*)PJH2_AXIS2,
      "axis2","([float x,y,z]):(djointhinge2,[float x,y,z]) - \
Sets/gets hinge-2 axis2. Axis 1 and axis 2 must not lie along the same line.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE2,PubJoint_prop,(void*)PJH2_ANGLE1,
      "angle1","([float]):(djointhinge2) - ");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE2,PubJoint_prop,(void*)PJH2_ANGLERATE1,
      "anglerate1","(float):(djointhinge2) - ");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE2,PubJoint_prop,(void*)PJH2_ANGLERATE2,
      "anglerate2","(float):(djointhinge2) - ");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE2,PubJoint_prop,(void*)PJP_SUSPENSIONERP,
      "suspensionerp","([float]):(djointhinge2,[float]) - Suspension error reduction parameter (ERP)");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTHINGE2,PubJoint_prop,(void*)PJP_SUSPENSIONCFM,
      "suspensioncfm","([float]):(djointhinge2,[float]) - Suspension constraint force mixing (CFM) value");


    FunctionPublish_initClass(LUXI_CLASS_PJOINTSLIDER,"djointslider",
    "Slider joint will limit the movement between two bodies to a axis.",
    NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PJOINTSLIDER, LUXI_CLASS_PJOINT);
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTSLIDER,PubGeom_new,(void*)LUXI_CLASS_PJOINTSLIDER,
      "new","(djointslider):([djointgroup]) - When the axis is set, the current \
position of the attached bodies is examined and that position will be the zero position.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTSLIDER,PubJoint_prop,(void*)PJS_AXIS,
      "axis","([float x,y,z]):([float x,y,z]) - sets/gets axis of slider. Must be attached first, else crash is likely");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTSLIDER,PubJoint_prop,(void*)PJS_SLIDERPOS,
      "position","([float position,positionrate]):(djointslider) - Get the slider \
linear position (i.e. the slider's 'extension') and the time derivative of this value.");


    FunctionPublish_initClass(LUXI_CLASS_PJOINTUNIVERSAL,"djointuniversal",
    "A universal joint is like a ball and socket joint that constrains an extra degree \
of rotational freedom. Given axis 1 on body 1, and axis 2 on body 2 that is perpendicular \
to axis 1, it keeps them perpendicular. In other words, rotation of the two bodies about \
the direction perpendicular to the two axes will be equal.\n\
In the picture, the two bodies are joined together by a cross. Axis 1 is attached to body 1, \
and axis 2 is attached to body 2. The cross keeps these axes at 90 degrees, so if you grab \
body 1 and twist it, body 2 will twist as well.\n\
A Universal joint is equivalent to a hinge-2 joint where the hinge-2's axes are perpendicular \
to each other, and with a perfectly rigid connection in place of the suspension.\n\
Universal joints show up in cars, where the engine causes a shaft, the drive shaft, to \
rotate along its own axis. At some point you'd like to change the direction of the shaft. \
The problem is, if you just bend the shaft, then the part after the bend won't rotate about \
its own axis. So if you cut it at the bend location and insert a universal joint, you can \
use the constraint to force the second shaft to rotate about the same angle as the first shaft.\n\
Another use of this joint is to attach the arms of a simple virtual creature to its body. \
Imagine a person holding their arms straight out. You may want the arm to be able to \
move up and down, and forward and back, but not to rotate about its own axis.",
    NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PJOINTUNIVERSAL, LUXI_CLASS_PJOINT);
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTUNIVERSAL,PubGeom_new,(void*)LUXI_CLASS_PJOINTUNIVERSAL,
      "new","(djointuniversal) : ([djointgroup group]) - creates a new universaljoint");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTUNIVERSAL,PubJoint_prop,(void*)PJU_ANCHOR,
      "anchor","([float x1,y1,z1,x2,y2,z2]):(djointuniversal,[float x,y,z]) - Sets/gets anchor.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTUNIVERSAL,PubJoint_prop,(void*)PJU_AXIS2,
      "axis1","([float x,y,z]):(djointuniversal,[float x,y,z]) - \
Set/gets Axis1. Axis 1 and axis 2 should be perpendicular to each other.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTUNIVERSAL,PubJoint_prop,(void*)PJU_AXIS1,
      "axis2","([float x,y,z]):(djointuniversal,[float x,y,z]) - "
      "Set/gets Axis2. Axis 1 and axis 2 should be perpendicular to each other.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTUNIVERSAL,PubJoint_prop,(void*)PJU_ANGLE1,
      "angle1","(float):(djointuniversal) - Gets Angle1");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTUNIVERSAL,PubJoint_prop,(void*)PJU_ANGLE2,
      "angle2","(float):(djointuniversal) - Gets Angle2");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTUNIVERSAL,PubJoint_prop,(void*)PJU_ANGLERATE1,
      "anglerate1","(float):(djointuniversal) - Gets Anglerate1");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTUNIVERSAL,PubJoint_prop,(void*)PJU_ANGLERATE2,
      "anglerate2","(float):(djointuniversal) - Gets Anglerate2");

    FunctionPublish_initClass(LUXI_CLASS_PJOINTFIXED,"djointfixed",
    "The fixed joint maintains a fixed relative position and orientation between two \
bodies, or between a body and the static environment. Using this joint is almost never a \
good idea in practice, except when debugging. If you need two bodies to be glued together \
it is better to represent that as a single body.",
    NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PJOINTFIXED, LUXI_CLASS_PJOINT);
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTFIXED,PubGeom_new,(void*)LUXI_CLASS_PJOINTFIXED,
      "new","(djointfixed) : ([djointgroup group]) - creates a new fixedjoint");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTFIXED,PubJoint_prop,(void*)PJF_FIX,
      "setfixed","():(djointfixed) - Call this on the fixed joint after it has been \
attached to remember the current desired relative offset and desired relative rotation \
between the bodies.");


    FunctionPublish_initClass(LUXI_CLASS_PJOINTAMOTOR,"djointamotor",
    "An angular motor (AMotor) allows the relative angular velocities of two bodies \
to be controlled. The angular velocity can be controlled on up to three axes, allowing \
torque motors and stops to be set for rotation about those axes. This is mainly useful \
in conjunction will ball joints (which do not constrain the angular degrees of freedom \
at all), but it can be used in any situation where angular control is needed. To use an \
AMotor with a ball joint, simply attach it to the same two bodies that the ball joint \
is attached to.",
    NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PJOINTAMOTOR, LUXI_CLASS_PJOINT);
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTAMOTOR,PubGeom_new,(void*)LUXI_CLASS_PJOINTAMOTOR,
      "new","(djointamotor) : ([djointgroup group]) - creates a new amotorjoint");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTAMOTOR,PubJoint_prop,(void*)PJA_MOTORMODEUSER,
      "usermode","([boolean]):(djointamotor,[boolean]) - If usermode is true, \
the user directly sets the axes that the AMotor controls. If it is false, eulermode is active \
which means that the AMotor computes the euler angles corresponding to the relative rotation, \
allowing euler angle torque motors and stops to be set.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTAMOTOR,PubJoint_prop,(void*)PJA_AXES,
      "numaxes","([int]):([int]) - Sets/gets the number of angular axes that will be \
controlled by the AMotor. The argument num can range from 0 (which effectively deactivates \
the joint) to 3. This is automatically set to 3 in eulermode");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTAMOTOR,PubJoint_prop,(void*)PJA_AXIS,
      "axis","([int,float x,y,z]):(djointamotor,[int anum,rel, float x,y,z]) - \
Sets/gets the AMotor axes. The anum argument selects the axis to change (0,1 or 2). \
Each axis can have one of three 'relative orientation' modes, selected by rel:\n\
* 0: The axis is anchored to the world frame.\n\
* 1: The axis is anchored to the first body.\n\
* 2: The axis is anchored to the second body. \n\
The axis vector (x,y,z) is always specified in world coordinates regardless of the \
setting of rel. There are two GetAMotorAxis functions, one to return the axis and one to \
return the relative mode.\n\
For dAMotorEuler mode:\n\
* Only axes 0 and 2 need to be set. Axis 1 will be determined automatically at each time step.\n\
* Axes 0 and 2 must be perpendicular to each other.\n\
* Axis 0 must be anchored to the first body, axis 2 must be anchored to the second body. \n");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTAMOTOR,PubJoint_prop,(void*)PJA_ANGLE,
      "angle","([float angle]):(djointamotor,int anum, [float angle]) - Tell the AMotor \
what the current angle is along axis anum. This function should only be called in user \
mode, because in this mode the AMotor has no other way of knowing the joint angles. The angle \
information is needed if stops have been set along the axis, but it is not needed for axis motors.\n\
If no angle is passed, it return the current angle for axis anum. In user mode this is simply \
the value that was set with. In euler mode this is the corresponding euler angle.");
    FunctionPublish_addFunction(LUXI_CLASS_PJOINTAMOTOR,PubJoint_prop,(void*)PJA_ANGLERATE,
      "anglerate","(float):(djointamotor,int anum) - Return the current angle rate for axis \
anum. In usermode this is always zero, as not enough information is available. In euler mode this \
is the corresponding euler angle rate.");

///////// SPACES ///////////////////////////////////////////////////////////////
///////// SPACES ///////////////////////////////////////////////////////////////
///////// SPACES ///////////////////////////////////////////////////////////////

    FunctionPublish_initClass(LUXI_CLASS_PSPACE,"dspace",
    "Spaces are grouping geoms and are doing the collisiondetection on the \
geoms inside of them. You can insert space into spaces, which should be done for \
performance reasons. For example, you may want to create a space that does not test \
it's geoms inside with each other, while it should create with the rest of the environment.", NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_PSPACE, LUXI_CLASS_PCOLLIDER);
  FunctionPublish_addFunction(LUXI_CLASS_PSPACE,PubGeom_destroy, NULL,
      "delete","() : (dspace space,[boolean cleanup]) - destroys a space. If cleanup is true \
it will delete the geoms that are in the space.");
  FunctionPublish_addFunction(LUXI_CLASS_PSPACE,PubSpace_prop, (void*)PHT_COUNT,
      "geomcount","(int count) : (dspace space) - Return the number of geoms \
contained within a space.");
  FunctionPublish_addFunction(LUXI_CLASS_PSPACE,PubSpace_prop, (void*)PHT_COLLIDE,
      "collidetest","([int]) : (dspace space,[int]) - if collidetest is \
true, the space's objects are tested against each other. It is usefull to \
create a 'static' space without collidetest that contains no bodies and inserting \
this space into a 'dynamic' space that contains bodies. That way, a lot of \
collisiontesting can be avoided and the simulation runs faster.");
  FunctionPublish_addFunction(LUXI_CLASS_PSPACE,PubSpace_prop, (void*)PHT_ADD,
      "add","() : (dspace space, dcollider geom) - adds a geom to the space");
  FunctionPublish_addFunction(LUXI_CLASS_PSPACE,PubSpace_prop, (void*)PHT_QUERY,
      "query","(boolean isin) : (pscape space, dcollider geom) - returns true if the geom is in the space");
  FunctionPublish_addFunction(LUXI_CLASS_PSPACE,PubSpace_prop, (void*)PHT_REMOVE,
      "remove","() : (dspace space, dcollider geom) - removes the geom from the space");
  FunctionPublish_addFunction(LUXI_CLASS_PSPACE,PubSpace_prop, (void*)PHT_GET,
      "get","([dcollider]) : (dspace space, int i) - \
Return the i'th geom contained within the space. i must range from 0 to dSpaceGetNumGeoms()-1.\n\
If any change is made to the space (including adding and deleting geoms) then no guarantee can \
be made about how the index number of any particular geom will change. Thus no space changes \
should be made while enumerating the geoms.\n\
This function is guaranteed to be fastest when the geoms are accessed in the order 0,1,2,etc.\
Other non-sequential orders may result in slower access, depending on the internal implementation.");


    FunctionPublish_initClass(LUXI_CLASS_PSPACESIMPLE,"dspacesimple",
      "Simple space. This does not do any collision culling - it simply checks every \
possible pair of geoms for intersection, and reports the pairs whose AABBs overlap. The time \
required to do intersection testing for n objects is O(n2). This should not be used for large \
numbers of objects, but it can be the preferred algorithm for a small number of objects. \
This is also useful for debugging potential problems with the collision system.",
      NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PSPACESIMPLE, LUXI_CLASS_PSPACE);
    FunctionPublish_addFunction(LUXI_CLASS_PSPACESIMPLE,PubGeom_new,(void*)LUXI_CLASS_PSPACESIMPLE,
      "new","(dspacesimple) : ([dspace space]) - creates a simple space if space is given, \
the new space is inserted in the given space");


    FunctionPublish_initClass(LUXI_CLASS_PSPACEHASH,"dspacehash",
      "Multi-resolution hash table space. This uses an internal data structure that \
records how each geom overlaps cells in one of several three dimensional grids. Each grid has \
cubical cells of side lengths 2^i, where i  is an integer that ranges from a minimum to a \
maximum value. The time required to do intersection testing for n objects is O(n) \
(as long as those objects are not clustered together too closely), as each object can be quickly \
paired with the objects around it.", NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PSPACEHASH, LUXI_CLASS_PSPACE);
    FunctionPublish_addFunction(LUXI_CLASS_PSPACEHASH,PubGeom_new,(void*)LUXI_CLASS_PSPACEHASH,
      "new","(dspacehash) : ([dspace space]) - creates a hashspace, if space is given, \
the new space is inserted in the given space");


    FunctionPublish_initClass(LUXI_CLASS_PSPACEQUAD,"dspacequad",
      "The quadtree is suited for collision that is mostly planar. However \
the use of this class is not recommend.", NULL,LUX_TRUE);
    FunctionPublish_setParent(LUXI_CLASS_PSPACEQUAD, LUXI_CLASS_PSPACE);
    FunctionPublish_addFunction(LUXI_CLASS_PSPACEQUAD,PubGeom_new,(void*)LUXI_CLASS_PSPACEQUAD,
      "new","(dspacequad) : ([dspace space],[float cx,cy,cz, float w,h,d, int depth]) - \
Creates a quadtree space. center and extents define the size of the \
root block. depth sets the depth of the tree - the number of blocks that are \
created is 4^depth");
///////// BODY /////////////////////////////////////////////////////////////////
///////// BODY /////////////////////////////////////////////////////////////////
///////// BODY /////////////////////////////////////////////////////////////////

    FunctionPublish_initClass(LUXI_CLASS_PBODY,"dbody",
    "A rigid body has various properties from the point of view of the simulation. \
Some properties change over time:\n\
* Position vector (x,y,z) of the body's point of reference. Currently the point of reference \
must correspond to the body's center of mass.\n\
* Linear velocity of the point of reference, a vector (vx,vy,vz).\n\
* Orientation of a body, represented by a quaternion (qs,qx,qy,qz) or a 3x3 rotation matrix.\n\
* Angular velocity vector (wx,wy,wz) which describes how the orientation changes over time.\n\
Other body properties are usually constant over time:\n\
* Mass of the body.\n\
* Position of the center of mass with respect to the point of reference. In the current\
implementation the center of mass and the point of reference must coincide.\n\
* Inertia matrix. This is a 3x3 matrix that describes how the body's mass is distributed \
around the center of mass. ", NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_PBODY, LUXI_CLASS_ODE);
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubGeom_new,(void*)LUXI_CLASS_PBODY,
      "new","(dbody) : () - creates a new body");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_VELOCITY,
      "velocity","([float vx,vy,vz]) : (dbody self,[float fx,fy,fz])\
 - sets/gets linear velocity of the body");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_MINENABLEFORCE,
      "minenableforce","([float f]) : (dbody self,[float f])\
 - sets/gets the required force to enable the body (default=0).");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_NEXTMINENABLEFORCE,
      "nextminenableforce","([float f]) : (dbody self,[float f])\
 - sets/gets the required force to enable the body (default=0), once it was activated - \
it overwrites the minenableforce then. \
This is usefull if you have a brickwall and the bricks need lot's of force to be loosed. \
But once loosed, it shouldn't need much effort anymore to reenable them.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_MINACTDEPTH,
      "minactdepth","([int]) : (dbody self,[int])\
 - sets/gets minimum activation depth, if ==-1 => disabled (default). Use this to reduce \
the number of activated bodys outgoing from a activator source.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ACTDEPTH,
      "actdepth","(int) : (dbody self)\
 - returns -1 if it was activated by a body that has a minactdepth >= 0");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ANGVEL,
      "angvel","([float vx,vy,vz]) : (dbody self,[float fx,fy,fz])\
 - sets/gets angular velocity of the body");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_GRAVITYMODE,
      "gravity","([boolean on]) : ([boolean on]) - sets/gets \
if gravity is on for this body");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_NOCOLLISSIONWITHCONNECTED,
      "nocollissionwithconnected","([boolean on]) : ([boolean on]) - sets/gets \
boolean, default=true. If it is true for two bodies that are connected, the collissioncheck is \
rejected if the two bodies of the collission are connected by a joint, before the complex \
collissioncheck is done - so it saves performance. If only one of the bodies has this flag set, it will be not \
affected by the other body. If the flag is false for both of them, they will collide normally \
with each other.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ADDFORCE,
      "addforce","() : ([float fx,fy,fz],[float x,y,z],[rel]) - \
adds force to the body. If coordinates are given, \
the force is added at this point, which will add some force at the torque. \
If a 7th parameter (boolean) is given, the point is in relative coordinates.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ADDTORQUE,
      "addtorque","() : ([float fx,fy,fz]) - \
adds torque force to the body");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ADDRELFORCE,
      "addrelforce","() : ([float fx,fy,fz],[float x,y,z],[rel]) - \
adds force to the body in the body's orientation. If coordinates are given, \
the force is added at this point, which will add some force at the torque. \
If a 7th parameter (boolean) is given, the point is in relative coordinates.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ADDRELTORQUE,
      "addreltorque","() : ([float fx,fy,fz]) - \
adds torque force to the body in the body's orientation");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_FORCE,
      "force","([float x,y,z]) : () - sets gets power of force");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_TORQUE,
      "torque","([float x,y,z]) : ([float x,y,z]) - sets/gets power \
of torque force");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_STATE,
      "state","([boolean]) : ([boolean enable]) - returns true if the \
body is enabled. Disables/Enables body if boolean value is passed. \
Disabled bodies don't use CPU power.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_AUTODISABLE,
      "autodisable","([boolean]) : ([boolean on]) - if true, \
the body can become disabled if it doesn't do much. This will speed up the \
simulation alot if you have lot's of objects that are just lieing around.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_VELTHRESHOLD,
      "velthreshold","([float]) : ([float]) - when the \
velocity drops below this value, the body becomes autodisabled.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ANGTHRESHOLD,
      "angularthreshold","([float]) : ([float]) - \
rotational minimum limit when the body should become disabled.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_DISABLESTEPS,
      "disablesteps","([int]) : ([int]) - trace depth when an \
enabled body touches a group of disabled bodies.\n");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_DISABLETIME,
      "disabletime","([float]) : ([float]) - amount of time to \
be waited until the body becomes autodisabled.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_DISABLEDEFAULT,
      "defaults","() : (dbody) - resets the autodisable properties to \
the settings of the world.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ROTDRAG,
    "rotdrag","([float x,y,z]):(body,[float x,y,z]) - this vector is multiplied with the \
    angular velocity vector of the body each step. Don't set the vector to values >1 or <0, \
    otherwise your body will either flicker around or accelerate to infinity. This is \
    a useful value to let the system lose energy which is stabilizing the simulation quite well.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_VELDRAG,
    "veldrag","([float x,y,z]):(body,[float x,y,z]) - this vector is multiplied with the \
    velocity vector of the body each step. Don't set the vector to values >1 or <0, \
    otherwise your body will either flicker around or accelerate to infinity. This is \
    a useful value to let the system lose energy which is stabilizing the simulation quite well.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_SETMASSZERO,
    "masszero","():() - sets mass of body to zero");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_MASSPARAM,
    "massparam","([float mass,cx,cy,cz, i11,i22,i33,i12,i13,i23]):\
(dbody self,[float mass,cx,cy,cz, i11,i22,i33,i12,i13,i23]) - \
sets/gets massproperties of the body. Refer to the ode docs here, this \
function is for advanced people who know what they want.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_SETMASSSPHERE,
    "masssphere","():(dbody self, float density,radius,[any total]) - sets the mass to \
the mass of a sphere with density and radius. If a third argument is applied, \
the density will be calculated to the total mass of the sphere.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_SETMASSCCYL,
    "masscappedcyl","():(dbody self, float density,int axis, float radius,\
length, [any total]) - sets mass of a cappedcylinder with density, radius and \
length. The axis indicates the orientation of the cylinder: 1=x, 2=y, 3=z. \
A capped cylinder is a cylinder that has spheres at its end.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_SETMASSCYL,
    "masscylinder","():(dbody self, float density,int axis, float radius,\
length, [any total]) - sets mass of a cylinder with density, radius and \
length. The axis indicates the orientation of the cylinder: 1=x, 2=y, 3=z.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_SETMASSBOX,
    "massbox","():(dbody self,float density,lx,ly,lz,[any total]) - \
sets mass of a box. If a 6th argument is passed, the density is calculated to \
total mass");

  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ADDMASSSPHERE,
    "massaddsphere","():(dbody self, float density,radius,[any total]) - adds the mass to \
the mass of a sphere with density and radius. If a third argument is applied, \
the density will be calculated to the total mass of the sphere.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ADDMASSCCYL,
    "massaddcappedcyl","():(dbody self, float density,int axis, float radius,\
length, [any total]) - adds mass of a cappedcylinder with density, radius and \
length. The axis indicates the orientation of the cylinder: 1=x, 2=y, 3=z. \
A capped cylinder is a cylinder that has spheres at its end.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ADDMASSCYL,
    "massaddcylinder","():(dbody self, float density,int axis, float radius,\
length, [any total]) - adds mass of a cylinder with density, radius and \
length. The axis indicates the orientation of the cylinder: 1=x, 2=y, 3=z.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ADDMASSBOX,
    "massaddbox","():(dbody self,float density,lx,ly,lz,[any total]) - \
adds mass of a box. If a 6th argument is passed, the density is calculated to \
total mass");

  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_ADJUST,
    "massadjust","():(dbody self, float newmass) - sets new mass");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_MASSROT,
    "massrotate","():(dbody self, float x,y,z) - rotates (radians) the \
mass of the body.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_FINITEMODE,
    "finitemode","([boolean on,float x,y,z]):([boolean on],[float x,y,z]) - \
Sets/gets mode and axis of the finite rotation\n\
The mode controls the way a body's orientation is updated at each time step. \n\
* true: An 'infinitesimal' orientation update is used. This is fast to compute, \
but it can occasionally cause inaccuracies for bodies that are rotating at high \
speed, especially when those bodies are joined to other bodies. This is the \
default for every new body that is created.\n\
* false: A 'finite' orientation update is used. This is more costly to compute, \
but will be more accurate for high speed rotations. Note however that high speed \
rotations can result in many types of error in a simulation, and this mode will \
only fix one of those sources of error.\n\
The three floats set the finite rotation axis for a body. This is axis only has \
meaning when the finite rotation mode is set.\n\
If this axis is zero (0,0,0), full finite rotations are performed on the body.\
If this axis is nonzero, the body is rotated by performing a partial finite \
rotation along the axis direction followed by an infinitesimal rotation along an \
orthogonal direction.\n\
This can be useful to alleviate certain sources of error caused by quickly \
spinning bodies. For example, if a car wheel is rotating at high speed you can \
call this function with the wheel's hinge axis as the argument to try and \
improve its behavior. ");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_JOINTGET,
    "joint","([djoint]):(dbody,int i) - returns the i-th joint the body is connected to.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_JOINTCOUNT,
    "jointcount","(int):(dbody) - the number of joints that the body is connected to.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_RELVELOCITIES,
    "jointenergy","(float kineticenergy):(dbody,[float multiplier]) - \
returns the kineticenergy applied to the body by connected bodies. Since the sum can become too \
small, an additional multiplier can be applied.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_JOINTFORCES,
    "jointforces","(float fx,fy,fz,forcesumlength,forcelengthsum):(dbody) - \
returns the sum of all force vectors that is contributed by all joints that are \
attached to the body, the total length of the forcevector and the sum \
of all lengths of forces together. Note that the sum of all lengths is never 0 if a force \
is applied, while all other values can be 0.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_CONNECTED,
    "connected","(boolean):(dbody,dbody) - true if both bodies are connected through a joint");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_LOCK,
    "lock","([int axis,float value,planewidth,planeveldamp]):(dbody,[int axis,float value],[planewidth],[planeveldamp]) - \
sets/gets locking for body. One axis can be set to be locked at a certain value. \
x=1,y=2,z=3,0=no locking. Planewidth is the width in which the body can move. \
Planeveldamp is the damping applied to the velocity in the locking axis. \
The damping should be >=0 and <=1. Default values are for unused arguments is 0.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_AXISLOCK,
    "lockrotaxis","([int axis,float x,y,z]):(dbody,[int axis,float x,y,z]) - \
sets/gets locking rotation axis for body. Chose the axis with the axis number \
(x=1,y=2,z=3,0=no lock) and set the direction where this axis should look to. ");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_AFFECT,
    "affect","([int affect]):(dbody,[int affect]) - \
sets/gets body affection. If two bodies collide and at least one reaffect bit of the \
other body is set that is set in affect, the other body is affected by the body.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_REAFFECT,
    "reaffect","([int reaffect]):(dbody,[int reaffect]) - \
sets/gets body reaffection. If two bodies collide and at least one affect bit of the \
other body is set that is set in reaffect, the body is affected by the other body.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_RELPOINT,
    "local2world","(float x,y,z):(dbody,float x,y,z) - converts local to world coordinates");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_RELPOINTVEL,
    "localpointvel","(float x,y,z):(dbody,float x,y,z) - retrieve velocity at relative point x,y,z");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_POINTVEL,
    "worldpointvel","(float x,y,z):(dbody,float x,y,z) - retrieve velocity at world point x,y,z");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_RELPOSPOINT,
    "world2local","(float x,y,z):(dbody,float x,y,z) - converts world to local coordinates");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_WORLDVECTOR,
    "vectortoworld","(float x,y,z):(dbody,float x,y,z) - rotate a vector into world coordinates");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubBody_prop,(void*)PB_LOCALVECTOR,
    "vectorfromworld","(float x,y,z):(dbody,float x,y,z) - converts world rotation into local");

  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubGeom_destroy, NULL,
    "delete","() : (dbody) - deletes the dbody");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubGeomBody_pos,NULL,
    "pos","([float x,y,z]) : (dgeom geom,[float x,y,z]) - sets/gets position of geom");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubGeomBody_matrix,NULL,
    "matrix","([matrix4x4]) : (dgeom geom,[matrix4x4]) - sets/gets matrix of geom");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubGeomBody_rot,(void*)ROT_RAD,
    "rotrad","([float x,y,z]) : (dgeom geom,[float x,y,z]) - sets/gets \
         rotation of geom. The angles must be calculated from a matrix, so the results \
         can differ from what you set. These matrix calculations are quite expensive, \
         so use it with care. This functions uses x,y,z as angles in radians (180°==PI)");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubGeomBody_rot,(void*)ROT_DEG,
    "rotdeg","([float x,y,z]) : (dgeom geom,[float x,y,z]) - sets/gets \
         rotation of geom. The angles must be calculated from a matrix, so the results \
         can differ from what you set. These matrix calculations are quite expensive, \
         so use it with care. This function uses x,y,z as angles in degrees (180°==PI).");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubGeomBody_rot,(void*)ROT_AXIS,
    "rotaxis","([float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]) : \
          (dgeom geom,[float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz])) - \
          returns or sets local rotation axis, make sure they make a orthogonal basis.");
  FunctionPublish_addFunction(LUXI_CLASS_PBODY,PubGeomBody_rot,(void*)ROT_QUAT,
    "rotquat","([float x,y,z,w]) : (dgeom geom,[float x,y,z,w]) - sets/gets \
          rotation of geom as quaternion.");
}
