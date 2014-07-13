// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __PUBLUXI_CLASS_COLLISION_H__
#define __PUBLUXI_CLASS_COLLISION_H__

#include "../common/reflib.h"
#include "../common/interfaceobjects.h"
#include "../scene/linkobject.h"
#include <ode/ode.h>

typedef enum PhysicsType_e {
  PHT_BODY,
  PHT_GEOM,
  PHT_SPACE,
  PHT_JOINT,
  PHT_JOINTGROUP,
  PHT_RADIUS,
  PHT_LENGTH,
  PHT_WIDTH,
  PHT_HEIGHT,
  PHT_DEPTH,
  PHT_ADD,
  PHT_GET,
  PHT_COUNT,
  PHT_REMOVE,
  PHT_QUERY,
  PHT_LEVELS,
  PHT_COLLIDE,
  PHT_MAXCONTACTS,
  PHT_MAXCONTACTINFO,
  PHT_NOCONTACTGENERATOR,
  PHT_GETCONTACTINFO,
  PHT_TRANSFORMER,
  PHT_RAYTRIRESULTS,
  PHT_TRISARRAYRESULTS,
} PhysicsType_t;

typedef struct OdeSpace_s{
  dSpaceID space;
  int collidetest;
  struct PhysicsObject_s *nextspace,*prevspace;
}OdeSpace_t;

typedef struct OdeTriData_s{
  dTriMeshDataID meshdata;
  float *vertices;
  int verticecount;
  int *indices;
  int indicescount;
}OdeTriData_t;

typedef struct OdeJointGroup_s{
  dJointGroupID jointgroup;
  struct PhysicsObject_s *jointlist;
}OdeJointGroup_t;

typedef struct OdeJoint_s{
  dJointID joint;
  Reference geom1,geom2;
  struct PhysicsObject_s *jointowner;
  struct PhysicsObject_s *nextjoint, *prevjoint;
  Reference disabledBody,enablerBody;
  float activationforce;
  float contactpos[3],contactnormal[3],contactdepth;
  dJointFeedback jointfeedback;
}OdeJoint_t;

typedef struct OdeGeom_s{
  dGeomID geom;
  dGeomID transformer; // geomtransformer object - needed for matrixIF-matrixtransformation
  Reference enabledSpace, disabledSpace;
  int positionchange;  // flag for matrixcaching that the position was changed
  int maxcontacts;   // maximum number of contacts on that geom. The minimum value wins
  float *contactinfo;   // x,y,z, nx,ny,nz, depth information from contacts
  struct PhysicsObject_s **contactgeomlist;
  int contactcount;   // actual number of contacts
  int maxcontactinfo; // maximum number of contactinfo
  int contactflag;    // is the contact flag outdated (overwrite)
  int nocontactgenerator; // don't create contacts for this any collision for this geom
  int material;

  float fdirnormal[3];  // normal of fdir
  float fdirpriority;   // priority of fdir
  float motion1,motion2; // motion1 and motion 2 (surfaceprop)
  struct PhysicsObject_s *nextGeomInBody,*prevGeomInBody;
  struct PhysicsObject_s *bodyowner;
}OdeGeom_t;

typedef struct OdeBody_s{
  dBodyID body;
  int lock;      // -1 if no locking, 0=x,1=y,2=z - axis will be set to plane then
  float plane,minenableforce; // axisvalue that is set if lock is pointing to an axis
  float planewidth,planeveldamp;
  float nextminenableforce; // replaces the minenableforce if the body was activated
  int affect,reaffect; // affects other objects and can be affected by other objects
  int nocollissionwithconnected;
  int minactdepth; // activation trace depth
  int actdepth;
  struct PhysicsObject_s *nextbody, *prevbody, *nextEnabledBody, *prevEnabledBody;
  struct PhysicsObject_s *geombodylist;
  int axislock;
  float axislockvec[3];
  float veldrag[3],rotdrag[3];
}OdeBody_t;


typedef struct PhysicsObject_s {
  // 1 DW
  MatrixLinkObject_t  link; // must come first!
  // 3 DW
  Reference   ref;
  PhysicsType_t type;
  lxClassType   classtype;
  // 16 DW - 4 DW aligned
  lxMatrix44    cache;
  //
  int       cachedate;

  union {
    OdeSpace_t    ospace;
    OdeTriData_t  otridata;
    OdeJointGroup_t ojointgrp;
    OdeJoint_t    ojoint;

    struct {
      dGeomID geom;
      dGeomID transformer; // geomtransformer object - needed for matrixIF-matrixtransformation
      Reference enabledSpace, disabledSpace;
      int positionchange;  // flag for matrixcaching that the position was changed
      int maxcontacts;   // maximum number of contacts on that geom. The minimum value wins
      float *contactinfo;   // x,y,z, nx,ny,nz, depth information from contacts
      struct PhysicsObject_s **contactgeomlist;
      int contactcount;   // actual number of contacts
      int maxcontactinfo; // maximum number of contactinfo
      int contactflag;    // is the contact flag outdated (overwrite)
      int nocontactgenerator; // don't create contacts for this any collision for this geom
      int material;
      Reference userref;

      float fdirnormal[3];  // normal of fdir
      float fdirpriority;   // priority of fdir
      float motion1,motion2; // motion1 and motion 2 (surfaceprop)
      struct PhysicsObject_s *nextGeomInBody,*prevGeomInBody;
      struct PhysicsObject_s *bodyowner;
    };

    struct {
      dBodyID body;
      int lock;      // -1 if no locking, 0=x,1=y,2=z - axis will be set to plane then
      float plane,minenableforce; // axisvalue that is set if lock is pointing to an axis
      float planewidth,planeveldamp;
      float nextminenableforce; // replaces the minenableforce if the body was activated
      int affect,reaffect; // affects other objects and can be affected by other objects
      int nocollissionwithconnected;
      int minactdepth; // activation trace depth
      int actdepth;
      struct PhysicsObject_s *nextbody, *prevbody, *nextEnabledBody, *prevEnabledBody;
      struct PhysicsObject_s *geombodylist;
      int axislock;
      float axislockvec[3];
      float veldrag[3],rotdrag[3];
    };

    void *id;
  };
  struct PhysicsObject_s *next,*prev;
} PhysicsObject_t;

typedef struct PhysicsRayTrisHit_s{
  int         idx;
  lxVector2       uv;
}PhysicsRayTrisHit_t;

typedef struct PhysicsContact_s{
  dContactGeom    odata;
  Reference     g1;
  Reference     g2;
  int         raystart;
  int         rayend;
  int         tristart;
  int         triend;
}PhysicsContact_t;

typedef struct PhysicsCollResult_s{
  Reference ref;

  int         count;

  int         maxContacts;
  int         numContacts;
  PhysicsContact_t  *contacts;

  int         maxRay;
  int         numRay;
  PhysicsRayTrisHit_t *rayhits;

  int         maxTri;
  int         numTri;
  int *trihits;

}PhysicsCollResult_t;

#endif
