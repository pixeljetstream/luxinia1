// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __VISTEST_H__
#define __VISTEST_H__


/*
VISIBILITY TESTING
------------------

contains a special octree space purely doing the visibility tests.

All Geometry objects are stored in a space the user can define
Cameras and Projectors are stored in another simple space

VisObjects have their linked sources as userdata. There is no
reference checking, because the visobject can only exist
as long as the source does.

*/
#include <luxinia/luxcore/contmacrolinkedlist.h>
#include <luxinia/luxcore/contoctree.h>
#include "../common/types.h"
#include <float.h>


#define VISTEST_FRUSTUM_USE_BBOX
//#define VISTEST_DYNAMIC_VISIBLESPACE


  // at lower depth its too unlikely a full node is fully inside
#define VISTEST_PRECHECK_LIST_MIN 4
#define VISTEST_MAX_DEPTH   4
#define VISTEST_MAX_LIGHTS    1024

  // should be in sync with VID_FXLIGHTS
#define VISTEST_LIGHT_CONTACTS  3
#define VISTEST_LIGHT_DIST    FLT_MAX



typedef enum VisObjectType_e{
  VIS_OBJECT_NONE,
  VIS_OBJECT_DYNAMIC,
  VIS_OBJECT_STATIC,
  VIS_OBJECT_CAMERA,
  VIS_OBJECT_PROJECTOR,
  VIS_OBJECT_LIGHT,
}VisObjectType_t;

typedef enum VisTestTweakable_e{
  VIS_TWEAK_MAXDEPTH_SCT,
  VIS_TWEAK_MAXDEPTH_ACT,
  VIS_TWEAK_MAXDEPTH_CAM,
  VIS_TWEAK_MAXDEPTH_PROJ,
  VIS_TWEAK_MAXDEPTH_LIGHT,
  VIS_TWEAK_MAXDEPTH_LIGHTVIS,

  VIS_TWEAK_VOLCHECKCNT,
  VIS_TWEAKS,
}VisTestTweakable_t;

typedef struct VisLightResult_s{
  int       numLights;
  float     maxlightdist;
  const struct Light_s  *lights[VISTEST_LIGHT_CONTACTS];
  float     lightdists[VISTEST_LIGHT_CONTACTS];
  int       maxlight;
}VisLightResult_t;

typedef struct VisLight_s{
  VisLightResult_t  full;
  VisLightResult_t  nonsun;
}VisLight_t;

typedef struct VisSet_s{
  enum32      projectorFlag;  // id of projector
  enum32      projectorVis; // visible to this projector

  VisLight_t    *vislight;
  int       needslightFull;
  int       needslightNonsun;
}VisSet_t;

typedef struct VisObject_s{
  struct VisObject_s LUX_LISTNODEVARS;
  enum32      cameraVis;    // visible to those vissets/cameras
  enum32      cameraFlag;   // id of vissets/cameras it is visible to
  struct  List3DNode_s *l3dvisListHead; // all visible root-l3dnodes associated with this visobject using visnext/visprev
  struct  List3DNode_s *l3dnodesListHead; // all root-l3dnodes associated with this visobject using linknext,linkprev
  VisSet_t    visset;
  int       curvisset;    // -1 on init, allows only l3dnodes of same l3dset to be bound

  VisObjectType_t type;
  const char      *name;      // mostly for debug

  int         bboxset;
  lxBoundingBox_t   bbox;
#ifdef VISTEST_FRUSTUM_USE_BBOX
  booln       usetbbox;
  lxBoundingBox_t tbbox;
  float       radius;
#else
  lxBoundingSphere_t  bsphere;
#endif
  lxVector3       center;

  lxBoundingCapsule_t bcapsule;   // for light distance check

  lxVector3     tcaporigin;   // transformed capsule
  lxVector3     tcapdir;
  ulong     tcaptime;

  int lastclipplane;

  union {
    void        *userdata;  // for camera/projector/light pointer to self
    struct LinkObject_s *linkobject;
    struct Camera_s   *camera;
    struct Projector_s  *proj;
    struct Light_s    *light;
    struct ActorNode_s  *actor;
    struct SceneNode_s  *scenenode;
  };


}VisObject_t;


typedef struct VisTestFrustum_s{
  lxFrustum_t   frustum;
  int       setID;
  enum32      bitID;
}VisTestFrustum_t;

typedef struct VisTestLight_s{
  uint      setID;
  float     range;
  float     rangeSq;
  booln     nonsunlit;
  struct Light_s  *light;
}VisTestLight_t;

typedef struct VisTestSpatial_s{
  booln visible;
  booln lit;
  uint  curvisset;
  enum32  cameraFlag;
  enum32  projectorFlag;
}VisTestSpatial_t;


struct VisTest_s{
  // - 4 DW aligned
  lxMatrix44  matrix;

  int     tweaks[VIS_TWEAKS];

  lxOcTreePTR dynamicSpace;
  lxOcTreePTR staticSpace;

  lxOcTreePTR lightSpace;
  lxOcTreePTR cameraSpace;
  lxOcTreePTR projectorSpace;

  int     voStaticCnt;
  int     voDynamicCnt;
  VisObject_t *voStaticListHead;
  VisObject_t *voDynamicListHead;
  VisObject_t voLights[VISTEST_MAX_LIGHTS];

  VisObject_t **visibleBufferArray;
  VisObject_t **visCurrent;

  int     staticCompile;
  int     traverseproj;
};

#ifdef LUX_COMPILER_MSC
typedef LUX_ALIGNSIMD_BEGIN struct VisTest_s VisTest_t;
#elif defined(LUX_COMPILER_GCC)
typedef lxMatrix44 struct VisTest_s VisTest_t;
#endif

//////////////////////////////////////////////////////////////////////////
// VisObject

  // constructor, data is pointer to cam/light/proj/actornode/scenenode
VisObject_t*  VisObject_new(VisObjectType_t type,void *data, char *name);

  // destructor
void  VisObject_free(VisObject_t *self);

  // increases/decreases visset light useage based on state
void VisObject_setVissetLight(VisObject_t *self, int state, int full);

void VisObject_setBBox(VisObject_t *vis, lxBoundingBox_t *bbox);

void VisObject_setBBoxForce(VisObject_t *vis, lxVector3 min, lxVector3 max);

///////////////////////////////////////////////////////////////////////////////
// Vis Test

void VisTest_init();

void VisTest_free();

void VisTest_recompileStatic();

  // performs visibility checking between nodes and cameras/projectors
  // results are stored in VisObject
  // RPOOLUSE
void VisTest_run();

  // RPOOLUSE
void VisTest_resetVisible();

void VisTest_draw(VisObjectType_t type);

int  VisTest_getTweak(VisTestTweakable_t type);
void VisTest_setTweak(VisTestTweakable_t type, int val);

#endif
