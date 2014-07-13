// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_CAMLIGHT_H__
#define __GL_CAMLIGHT_H__

#include "../common/common.h"
#include "../common/reflib.h"
#include "gl_render.h"

/*
CAMERA, LIGHT, PROJECTOR, SKYBOX,
------------------------------------------

RenderInit will create the main Camera and the SunLight

Camera:
Atm only the g_CamLight.camera is used when rendering (future will allow more cameras,
such as water reflection camera..)

Light:
The SunLight is parallel (infinite light), while FxLight are pointlights.
FxLights are range based (closest + highest priority is taken) they can also
have distance attenuation.

Projector:
Projectors project a texture on a surface. If hardware support exists they will
use cubemaps to prevent backprojection (for perspective projectors) and can have
per pixel attenuation (3d texture) on proper hardware.

Skybox:
consist of 6 textures (N,S,E,W,TOP,BOTTOM) which can be set as background

SkyDome:
a hemisphere around the camera

List3D Nodes must be set by user if they are affected by Lights or Projectors.

Projectors when activated will become part of a list, which is used for drawing
Same goes for FxLights. User must activate/deactivate manually.

Camera,Light and Projector look along the positive Y Axis, they can be linked
to Matrix44Interfaces. You must call their Updates before visibility testing is
done, so that all information is on same timelevel.



They are all collected in MallocLists to prevent memory leaks, and will be cleared
when CamLightProjSkyBox_freeAll() is called.


Camera    (user,GenMem)
Projector (user,GenMem)
Light   (user,GenMem)
SkyBox    (user,GenMem)


Author: Christoph Kubisch

*/

#define LIGHT_ATT_QUADFACTOR  0.015f
#define PROJECTOR_SCISSOR_AREA  0.75f

enum ProjectorChange_e{
  PROJECTOR_CHANGE_TEX  = 1,
  PROJECTOR_CHANGE_PROJ = 2,
};

typedef struct FrustumObject_s
{
  Reference         ref;
  struct FrustumObject_s  *prev,*next;
  struct FrustumObject_s  **listHead;

  lxFrustum_t     frustum;
  lxVector3       frustumBoxCorners[LUX_FRUSTUM_CORNERS];
}FrustumObject_t;

typedef struct Camera_s
{
  // 2 DW
  struct Camera_s   *gnext,*gprev;  // global list

  // 2 DW
  enum32  bitID;
  int   bit;

  // aligned to 4 DW
  lxMatrix44  finalMatrix;
  lxMatrix44  mviewproj;
  lxMatrix44  mviewinv;
  lxMatrix44  mview;
  lxMatrix44  proj;
  lxMatrix44  projinv;
  lxMatrix44  mviewprojinv;

  lxFrustum_t     frustum;
  lxVector3       frustumBoxCorners[LUX_FRUSTUM_CORNERS];
  FrustumObject_t   *frustcontListHead;

  float fov;    //  Field of View
  float frontplane; //
  float backplane;  //
  float aspect;   // < 0 means "window" aspect
  float nearpercentage;
  int   userproj;
  int   infinitebackplane;  // for stencilshadows this might be needed
  unsigned long dirtyframe;

  lxVector3 pos;    //  Position of Camera_t
  lxVector3 up;     //  Up Vector
  lxVector3 right;    //  Right Vector
  lxVector3 fwd;

  uchar useclip;
  uchar usereflect;
  short frontcw;
  lxVector4 clipplane;
  lxVector4 reflectplane;

  struct List3DNode_s *l3dnode;

} Camera_t;

typedef struct Light_s
{
  struct Light_s *gprev,*gnext; // global list
  struct Light_s LUX_LISTNODEVARS;  // local l3dset list

  ushort setID;
  ushort priority;
  struct List3DNode_s *l3dnode;
  struct List3DSet_s  *l3dset;

  uint  removetime;
  float range;      // fx lights max range
  float rangeSq;
  int   rangeAtt;   // special range attenuation is done when set
  int   nonsunlit;    // wil affect baked nodes

  lxVector4 pos;      // Position

  // Colors
  lxVector4 ambient;    // Ambient color
  lxVector4 diffuse;    // Diffuse color
  lxVector4 specular;   // Specular

  // Attenuation
  float attenuate[3];   // const, lin, quadric
} Light_t;

typedef struct Projector_s
{
  // 4 DW
  struct Projector_s *gprev,*gnext; // global list
  struct Projector_s *prev,*next; // local l3dset list

  // aligned to 4 DW
  lxMatrix44      projmatrix;
  lxMatrix44      viewmatrixI;
  lxMatrix44      vprojmatrix;
  lxMatrix44      vprojmatrixT;

  lxFrustum_t     frustum;
  lxVector3       frustumBoxCorners[LUX_FRUSTUM_CORNERS];

  short       camScissor;
  short       camInvisible;
  lxVector4       camRectangle; // transformed box corners in camera space, visible rectangle
  Camera_t      *lastcam;

  uint    setID;
  enum32    bitID;

  int     id;
  struct List3DSet_s  *l3dset;
  struct List3DNode_s *l3dnode;

  float   fov;
  float   frontplane;
  float   backplane;
  float   aspect;

  lxVector3     pos;
  lxVector3     target;
  lxVector3     fwd;
  lxVector3     up;

  float     range;
  float     scalef;

  int       texRID;
  VIDBlendColor_t attmode;
  VIDBlend_t    blend;

  VIDTexBlendHash thnopass;
  VIDTexBlendHash thnopassatt;

  uchar     nopass;
  uchar     cubemap;
  uchar     attenuate;
  uchar     changed;

}Projector_t;

typedef struct SkyBox_s{
  int     northRID;
  int     southRID;
  int     eastRID;
  int     westRID;
  int     topRID;
  int     bottomRID;
  int     cubeRID;

  float     fovfactor;

  Reference   reference;
}SkyBox_t;

// GLOBALS
typedef struct CamLightProj_s{
  // the current active camera
  Camera_t  *camera;
  // the current active sun
  Light_t   *sunlight;

  // list of virtual cameras
  Camera_t  *virtualcameraListHead;

  // global list of active cameras
  Camera_t  *cameraListHead;
  // global list of active extra lights
  Light_t   *lightListHead;
  // global list of active projectors
  Projector_t *projectorListHead;
}CamLightProj_t;

extern CamLightProj_t g_CamLight;


///////////////////////////////////////////////////////////////////////////////
// Defaults

void CameraLightSky_init();
void CameraLightSky_deinit();

//////////////////////////////////////////////////////////////////////////
// Camera
Camera_t* Camera_new(const int id);
void Camera_free(Camera_t *cam);

int  Camera_bitInUse(const int id);
int  Camera_isDefault(Camera_t *cam);

void Camera_activateMain(Camera_t *cam);
void Camera_setGL(Camera_t *cam);

void Camera_setMatrixIF(Camera_t *cam,Matrix44Interface_t *matrixIF);
void Camera_update(Camera_t *cam, const lxMatrix44 override);
void Camera_getCorners(const Camera_t *cam, const float range, lxVector3 outpoints[6]);
void Camera_screenToWorld(const Camera_t *cam, const lxVector3 screen, lxVector3 world, const int viewport[4]);

booln Camera_addFrustumObject(Camera_t *cam, FrustumObject_t *fobj);
booln Camera_remFrustumObject(Camera_t *cam, FrustumObject_t *fobj);
void  Camera_remFrustumObjects(Camera_t *cam);

void CameraViewMatrix(lxMatrix44 viewMatrix, const lxMatrix44 finalMatrix);

//////////////////////////////////////////////////////////////////////////
// Light
Light_t* Light_new();
void Light_free(Light_t *light);

int  Light_isDefault(Light_t *cam);

void Light_setGL( const struct Light_s *light,const int lightnum);
void Light_activateSun(Light_t *light);

void Light_update(Light_t *light, const lxMatrix44 override);

//////////////////////////////////////////////////////////////////////////
// Projector


Projector_t *Projector_new();
void    Projector_free(Projector_t *proj);

  // computes final vprojmatrixT & Frustum
void Projector_update(Projector_t *proj, const lxMatrix44 overridemat);

  // set the texture and its blendmode
void Projector_setTex(Projector_t *proj,
            const int texindex,
            const VIDBlendColor_t blendmode,
            const int attenuate);

  // computes proj matrix, normally you do this only once
void Projector_setProjection(Projector_t *proj,
               const float fov,
               const float front,
               const float back,
               const float aspect);

  // returns texstages used
int Projector_stageGL(const Projector_t *proj, const int texunit, const int pass);

  // returns 1 for scissor -1 for invisible and 0 for no scissor
int Projector_prepScissor(Projector_t *proj, int oldscissor[4]);

//////////////////////////////////////////////////////////////////////////
// SkyBox
SkyBox_t* SkyBox_new(int cubetexRID,
           char *southname,
           char *northname,
           char* eastname,
           char *westname,
           char* topname,
           char *bottomname);
//void    SkyBox_free(SkyBox_t *skybox);
void    RSkyBox_free(lxRskybox ref);

//////////////////////////////////////////////////////////////////////////
// FrustumObject

FrustumObject_t* FrustumObject_new();
void RFrustumObject_free(lxRfrustumobject ref);

#endif
