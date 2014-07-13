// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "gl_render.h"
#include "gl_shader.h"
#include "gl_draw3d.h"
#include "gl_list3d.h"
#include "gl_camlight.h"
#include "gl_window.h"
#include <GL/glu.h>

// GLOBALS
CamLightProj_t g_CamLight = {NULL,NULL,NULL,NULL,NULL};

// LOCALS
static struct CLData_s{
  List3DNode_t* defaultCam;
  List3DNode_t* defaultSun;
  enum32      camInUse;
}l_CLData = {NULL,NULL,0};

void  Camera_defaults(Camera_t *cam, const enum32 bitid);
void  Light_defaults(Light_t *light);


///////////////////////////////////////////////////////////////////////////////
// Garbage Collection

void CameraLightSky_init(){

  l_CLData.defaultCam = List3DNode_newCamera("_dcam",0,0);
  List3DNode_useManualWorld(l_CLData.defaultCam,LUX_TRUE);
  Reference_makeStrong(l_CLData.defaultCam->reference);
  l_CLData.camInUse |= 1;

  g_CamLight.camera = l_CLData.defaultCam->cam;

  l_CLData.defaultSun = List3DNode_newLight("_dsun",0);
  List3DNode_useManualWorld(l_CLData.defaultSun,LUX_TRUE);
  Reference_makeStrong(l_CLData.defaultSun->reference);
  g_CamLight.sunlight = l_CLData.defaultSun->light;

  Reference_registerType(LUXI_CLASS_SKYBOX,"skybox",RSkyBox_free,NULL);
  Reference_registerType(LUXI_CLASS_FRUSTUMOBJECT,"frustumobject",RFrustumObject_free,NULL);
}

void CameraLightSky_deinit()
{

  Reference_release(l_CLData.defaultCam->reference);
  Reference_release(l_CLData.defaultSun->reference);

  l_CLData.defaultCam = NULL;
  l_CLData.defaultSun = NULL;
}


//////////////////////////////////////////////////////////////////////////
// CAMERA
// ------

int Camera_isDefault(Camera_t *cam){
  return cam == l_CLData.defaultCam->cam;
}


int Camera_bitInUse(const int id){
  // cam bit already in use
  return ((1<<id) & l_CLData.camInUse);
}

void Camera_defaults(Camera_t*cam, const enum32 bitid)
{
  static lxMatrix44SIMD matrix;
  lxMatrix44IdentitySIMD(matrix);
  lxMatrix44IdentitySIMD(cam->mviewproj);

  cam->backplane = 1024;
  cam->frontplane = 1;
  cam->fov = 45.0;
  cam->bitID = bitid;
  cam->aspect = -1;
  cam->nearpercentage = 0.05;
  lxVector3Set(cam->clipplane,0,0,1);
  lxVector3Set(cam->reflectplane,0,0,1);

  Camera_update(cam,matrix);
}

Camera_t* Camera_new(const int id)
{
  Camera_t *cam;
  enum32 bitid = 1<<id;

  cam = genzallocSIMD(sizeof(Camera_t));
  cam->bit = id;

  LUX_SIMDASSERT(((size_t)((Camera_t*)cam)->finalMatrix) % 16 == 0);
  LUX_SIMDASSERT(((size_t)((Camera_t*)cam)->proj) % 16 == 0);

  l_CLData.camInUse |= bitid;

  Camera_defaults(cam,bitid);

  return cam;
}

void Camera_activateMain(Camera_t *cam)
{
  g_CamLight.camera = cam;
}

void Camera_free(Camera_t *cam){
  if (cam == g_CamLight.camera && l_CLData.defaultCam){
    g_CamLight.camera = l_CLData.defaultCam->cam;
  }

  List3D_removeCamera(cam);
  Camera_remFrustumObjects(cam);

  clearFlag(l_CLData.camInUse,cam->bitID);
  genfreeSIMD(cam,sizeof(Camera_t));
}

void Camera_setGL(Camera_t *cam)
{
  glMatrixMode (GL_PROJECTION);
  glLoadMatrixf(cam->proj);
  g_VID.drawsetup.projMatrix = cam->proj;
  g_VID.drawsetup.projMatrixInv = cam->projinv;
  g_VID.drawsetup.viewMatrix = cam->mview;
  g_VID.drawsetup.viewMatrixInv = cam->mviewinv;
  g_VID.drawsetup.viewProjMatrix = cam->mviewproj;
  lxMatrix44MultiplyRot(g_VID.drawsetup.viewSkyrotMatrix,g_VID.drawsetup.skyrotMatrix,cam->mviewinv);
  //MatrixTranspose(g_VID.drawsetup.viewSkyrotMatrix,g_VID.drawsetup.skyrotMatrix);
  //MatrixMultiplyRot1(g_VID.drawsetup.viewSkyrotMatrix,cam->mviewinv);

  if (cam->frontcw) glFrontFace(GL_CW);
  else        glFrontFace(GL_CCW);

  vidLoadCameraGL();
  vidCgViewProjUpdate();
}

void CameraViewMatrix(lxMatrix44 viewMatrix, const lxMatrix44 finalMatrix)
{
  static lxMatrix44SIMD mat;
  lxVector3 pos = {finalMatrix[12],finalMatrix[13],finalMatrix[14]};

  mat[0] = finalMatrix[0];
  mat[4] = finalMatrix[1];
  mat[8] = finalMatrix[2];
  mat[12] = 0.0f;

  mat[1] = finalMatrix[8];
  mat[5] = finalMatrix[9];
  mat[9] = finalMatrix[10];
  mat[13] = 0.0f;

  mat[2] = -finalMatrix[4];
  mat[6] = -finalMatrix[5];
  mat[10] = -finalMatrix[6];
  mat[14] = 0.0f;

  mat[3] = 0.0f;
  mat[7] = 0.0f;
  mat[11] = 0.0f;
  mat[15] = 1.0f;

  lxMatrix44IdentitySIMD(viewMatrix);
  lxMatrix44SetInvTranslation(viewMatrix,pos);

  lxMatrix44Multiply2SIMD(mat,viewMatrix);
}

void Camera_update(Camera_t *cam, const lxMatrix44 override)
{
  static lxMatrix44SIMD mat;

  float size;
  float aspect;

  if (cam->dirtyframe == g_VID.frameCnt) return;
  cam->dirtyframe = g_VID.frameCnt;

  aspect = (cam->aspect < 0) ? g_Window.ratio : cam->aspect;

  LUX_DEBUGASSERT(override);

  lxMatrix44CopySIMD(cam->finalMatrix,override);

  if (cam->usereflect){
    lxMatrix44Reflection(mat,cam->reflectplane);
    lxMatrix44Multiply2SIMD(mat,cam->finalMatrix);
  }

  // position vector
  lxVector3Set(cam->pos, cam->finalMatrix[12], cam->finalMatrix[13], cam->finalMatrix[14]);
  lxVector3Set(cam->up,cam->finalMatrix[8],cam->finalMatrix[9],cam->finalMatrix[10]);
  lxVector3Set(cam->fwd,cam->finalMatrix[4],cam->finalMatrix[5],cam->finalMatrix[6]);
  lxVector3Set(cam->right,cam->finalMatrix[0],cam->finalMatrix[1],cam->finalMatrix[2]);

  CameraViewMatrix(cam->mview,cam->finalMatrix);
  lxMatrix44AffineInvertSIMD(cam->mviewinv,cam->mview);

  if (!cam->userproj)
  if(cam->fov > 0){
    //glLoadIdentity ();
    //gluPerspective(cam->fov/aspect,aspect,cam->frontplane,cam->backplane);
    lxMatrix44Perspective(cam->proj,cam->fov/aspect,cam->frontplane,cam->backplane,aspect);
    //MatrixPerspective(cam->projaspect1,cam->fov,cam->frontplane,cam->backplane,1);
  }
  else{
    size = -cam->fov*0.5f;
    //glLoadIdentity();
    //glOrtho(-size*aspect,size*aspect,-size,size,cam->frontplane,cam->backplane);
    lxMatrix44Ortho(cam->proj,-cam->fov/aspect,cam->frontplane,cam->backplane,aspect);
    //MatrixOrtho(cam->projaspect1,size,cam->frontplane,cam->backplane,1);
  }

  if (cam->useclip){
    lxMatrix44ModifyProjectionClipplane(cam->proj,cam->mview,cam->mviewinv,cam->clipplane);
  }

  lxMatrix44MultiplyFullSIMD(cam->mviewproj,cam->proj,cam->mview);
  lxFrustum_update(&cam->frustum,cam->mviewproj);
  lxFrustum_getCorners(&cam->frustum,cam->frustumBoxCorners);

  if (cam->infinitebackplane && cam->fov > 0){
    lxMatrix44PerspectiveInf(cam->proj,cam->fov/aspect,cam->frontplane,aspect);
    lxMatrix44MultiplyFullSIMD(cam->mviewproj,cam->proj,cam->mview);
  }

  lxMatrix44Invert(cam->projinv,cam->proj);
  lxMatrix44Invert(cam->mviewprojinv,cam->mviewproj);


#if 0
  {
    BoundingVectors_fromCamera(cam->frustumBoxCorners,cam->finalMatrix,cam->fov,cam->frontplane,cam->backplane,aspect);
    Vector3 corners[8];
    Frustum_getCorners(&cam->frustum,corners);
    if (1)
    {
      Vector3 vec;
      Vector3Set(vec,1,1,1);
    }
  }
#endif
}


void  Camera_getCorners(const Camera_t *cam, const float range, lxVector3 outpoints[6])
{
  static lxMatrix44SIMD mat;
  lxVector3 vec;

  float aspect = cam->aspect < 0 ? g_Window.ratio : cam->aspect;
  // first is origin
  lxVector3Copy(outpoints[0],cam->pos);
  // last is target
  lxVector3ScaledAdd(outpoints[5],cam->pos,range,cam->fwd);

  // ortho
  if (cam->fov < 0){
    // top right
    lxVector3ScaledAdd(outpoints[1],cam->pos,-cam->fov,cam->right);
    lxVector3ScaledAdd(outpoints[1],outpoints[1],-cam->fov,cam->up);
    lxVector3ScaledAdd(outpoints[1],outpoints[1],range,cam->fwd);

    // top left
    lxVector3ScaledAdd(outpoints[2],cam->pos,cam->fov,cam->right);
    lxVector3ScaledAdd(outpoints[2],outpoints[2],-cam->fov,cam->up);
    lxVector3ScaledAdd(outpoints[2],outpoints[2],range,cam->fwd);

    // low right
    lxVector3ScaledAdd(outpoints[3],cam->pos,-cam->fov,cam->right);
    lxVector3ScaledAdd(outpoints[3],outpoints[3],cam->fov,cam->up);
    lxVector3ScaledAdd(outpoints[3],outpoints[3],range,cam->fwd);

    // low left
    lxVector3ScaledAdd(outpoints[4],cam->pos,cam->fov,cam->right);
    lxVector3ScaledAdd(outpoints[4],outpoints[4],cam->fov,cam->up);
    lxVector3ScaledAdd(outpoints[4],outpoints[4],range,cam->fwd);
  }
  else{
  // perspective
    // more complex
    lxVector3Set(vec,cam->fov/aspect,0,cam->fov);
    lxMatrix44IdentitySIMD(mat);
    lxMatrix44FromEulerZYXdeg(mat,vec);
    lxVector3Set(vec,0,range,0);
    lxVector3TransformRot1(vec,mat);
    // now create the subversions of this vector
    // top right
    lxVector3Copy(outpoints[1],vec);
    // top left
    lxVector3Copy(outpoints[2],vec);
    outpoints[2][0] = -outpoints[2][0];
    // low right
    lxVector3Copy(outpoints[3],outpoints[1]);
    outpoints[3][2] = -outpoints[1][2];
    // low left
    lxVector3Copy(outpoints[4],outpoints[2]);
    outpoints[4][2] = -outpoints[2][2];

    // transform into worldspace
    lxVector3Transform1(outpoints[1],cam->finalMatrix);
    lxVector3Transform1(outpoints[2],cam->finalMatrix);
    lxVector3Transform1(outpoints[3],cam->finalMatrix);
    lxVector3Transform1(outpoints[4],cam->finalMatrix);

  }

}
void Camera_screenToWorld(const Camera_t *cam, const lxVector3 screen, lxVector3 world, const int viewport[4])
{
  static double mm[16];
  static double pm[16];
  double x,y,z;

  LUX_ARRAY16COPY(mm,cam->mview);
  LUX_ARRAY16COPY(pm,cam->proj);
  gluUnProject((double)screen[0], (double)screen[1], (double)screen[2], mm, pm,viewport,&x,&y,&z);
  world[0] = (float)x; world[1] = (float)y; world[2] = (float)z;
}

booln Camera_addFrustumObject(Camera_t *cam, FrustumObject_t *fobj)
{
  if (fobj->listHead != NULL)
    return LUX_FALSE;

  lxListNode_addLast(cam->frustcontListHead,fobj);

  Reference_ref(fobj->ref);
  fobj->listHead = &cam->frustcontListHead;

  return LUX_TRUE;
}

booln Camera_remFrustumObject(Camera_t *cam, FrustumObject_t *fobj)
{
  if (fobj->listHead != &cam->frustcontListHead)
    return LUX_FALSE;

  lxListNode_remove(cam->frustcontListHead,fobj);
  fobj->listHead = NULL;
  Reference_release(fobj->ref);


  return LUX_TRUE;
}

void Camera_remFrustumObjects(Camera_t *cam)
{
  FrustumObject_t *fobj;

  while (fobj = cam->frustcontListHead){
    Camera_remFrustumObject(cam,fobj);
  }

}

//////////////////////////////////////////////////////////////////////////
// LIGHT
// -----

void Light_defaults(Light_t *light)
{
  lxVector4Set(light->ambient, 0.0f, 0.0f, 0.0f, 1.0f);
  lxVector4Set(light->diffuse, 1.0f, 1.0f, 1.0f, 1.0f);
  lxVector4Set(light->specular, 0.0f, 0.0f, 0.0f, 0.0f);  // Keep off costs a lot performance

  lxVector4Set(light->pos, -30.0f, -30.0f, 30.0f, 1.0f);

  lxVector3Set(light->attenuate,1,0,0);
  light->nonsunlit = LUX_TRUE;
}

int Light_isDefault(Light_t *cam){  return cam == l_CLData.defaultSun->light;}

Light_t *Light_new()
{
  Light_t *light;

  light = lxMemGenZalloc(sizeof(Light_t));
  lxListNode_init(light);

  Light_defaults(light);

  return light;
}

void Light_free(Light_t *light){
  if (light == g_CamLight.sunlight && l_CLData.defaultSun){
    g_CamLight.sunlight = l_CLData.defaultSun->light;
  }
  List3D_removeLight(light);

  lxMemGenFree(light,sizeof(Light_t));
}


void Light_update(Light_t *light,const lxMatrix44 override){

  if (override){
    lxVector3Copy(light->pos,&override[12]);
    return;
  }
  else
    return;

}

void Light_setGL(const struct Light_s *light,const int lightnum)
{
  GLenum  lightid;
  lxVector3 attenuate;

  if (lightnum >= VID_MAX_LIGHTS)
    return;
  lightid = GL_LIGHT0+lightnum;

  // we dont need to resubmit data because a light cannot change within a draw
  // and cannot change its camera
  // except the sunlight which can be used on different cameras at the same time
  if(g_VID.drawsetup.lightPtGL[lightnum]==light)
    return;

  glLightfv(lightid, GL_AMBIENT, light->ambient);
  glLightfv(lightid, GL_DIFFUSE, light->diffuse);
  glLightfv(lightid, GL_SPECULAR,light->specular);

  glLightfv(lightid, GL_POSITION,light->pos);

  // compute distance factors on a "self made" formula ;)
  // if we operate with ranges, no attenuation if sun
  if (light == g_CamLight.sunlight){
    attenuate[0]=1;
    attenuate[1]=0;
    attenuate[2]=0;
    glLightf(lightid, GL_CONSTANT_ATTENUATION,attenuate[0]);
    glLightf(lightid, GL_LINEAR_ATTENUATION,attenuate[1]);
    glLightf(lightid, GL_QUADRATIC_ATTENUATION,attenuate[2]);
  }
  else if (light->rangeAtt){
    attenuate[0]=1;
    attenuate[1]=0;
    attenuate[2]=(254.0f/(light->rangeSq));
    glLightf(lightid, GL_CONSTANT_ATTENUATION,attenuate[0]);
    glLightf(lightid, GL_LINEAR_ATTENUATION,attenuate[1]);
    glLightf(lightid, GL_QUADRATIC_ATTENUATION,attenuate[2]);
  }
  else
  {
    glLightf(lightid, GL_CONSTANT_ATTENUATION,light->attenuate[0]);
    glLightf(lightid, GL_LINEAR_ATTENUATION,light->attenuate[1]);
    glLightf(lightid, GL_QUADRATIC_ATTENUATION,light->attenuate[2]);
  }

  g_VID.drawsetup.lightPtGL[lightnum]=(Light_t*)light;
  g_VID.drawsetup.lightPt[lightnum]=(Light_t*)light;
}
void Light_activateSun(Light_t *light){
  g_CamLight.sunlight = light;
}


//////////////////////////////////////////////////////////////////////////
// PROJECTOR
// ---------
Projector_t *Projector_new()
{
  Projector_t *proj = genzallocSIMD(sizeof(Projector_t));
  LUX_SIMDASSERT(((size_t)((Projector_t*)proj)->projmatrix) % 16 == 0);
  lxListNode_init(proj);

  proj->aspect = 1.0f;
  proj->frontplane = 1.0f;
  proj->backplane = 1024.0f;
  proj->fov = 60.0f;

  proj->changed |= PROJECTOR_CHANGE_PROJ;

  return proj;
}

void    Projector_free(Projector_t *proj)
{
  List3D_removeProjector(proj);
  genfreeSIMD(proj,sizeof(Projector_t));
}


void Projector_update(Projector_t *proj, const lxMatrix44 overridemat){
  static lxMatrix44SIMD matrix;
  static lxMatrix44SIMD matrix2;
  lxVector3 temp;
  float *mat;

  if (proj->changed & PROJECTOR_CHANGE_TEX)
    Projector_setTex(proj,proj->texRID,proj->blend.blendmode,proj->attenuate);
  if (proj->changed & PROJECTOR_CHANGE_PROJ)
    Projector_setProjection(proj,proj->fov,proj->frontplane,proj->backplane,proj->aspect);

  // position vector
  lxVector3Set(proj->pos, overridemat[12], overridemat[13], overridemat[14]);
  lxVector3Set(proj->up,overridemat[8],overridemat[9],overridemat[10]);
  lxVector3Set(proj->fwd,overridemat[4],overridemat[5],overridemat[6]);
  lxVector3Set(temp,overridemat[0],overridemat[1],overridemat[2]);

  mat = matrix;
  mat[0] = temp[0];
  mat[4] = temp[1];
  mat[8] = temp[2];
  mat[12] = 0;

  mat[1] = proj->up[0];
  mat[5] = proj->up[1];
  mat[9] = proj->up[2];
  mat[13] = 0;

  mat[2] = -proj->fwd[0];
  mat[6] = -proj->fwd[1];
  mat[10] = -proj->fwd[2];
  mat[14] = 0;

  mat[3] = 0;
  mat[7] = 0;
  mat[11] = 0;
  mat[15] = 1;

  lxMatrix44IdentitySIMD(matrix2);
  lxMatrix44SetInvTranslation(matrix2,proj->pos);
  lxMatrix44MultiplySIMD(proj->viewmatrixI,mat,matrix2);

  lxMatrix44MultiplyFullSIMD(matrix2,proj->projmatrix,proj->viewmatrixI);
  lxFrustum_update(&proj->frustum,matrix2);
  lxFrustum_getCorners(&proj->frustum,proj->frustumBoxCorners);

  // create modified GL matrix
  lxMatrix44IdentitySIMD(matrix);
  if (proj->cubemap)
    matrix[5] = -1.0f;
  else// First scale and bias into [0..1] range.
    matrix[0] = matrix[5] = matrix[10] = matrix[12] = matrix[13] = matrix[14] = 0.5;
  lxMatrix44MultiplyFullSIMD(proj->vprojmatrix,matrix,matrix2);
  lxMatrix44TransposeSIMD(proj->vprojmatrixT,proj->vprojmatrix);



  proj->lastcam = NULL;
}

void Projector_setTex(Projector_t *proj,const int texindex,const VIDBlendColor_t blendmode,const int attenuate){
  uchar black[3];

  Texture_t *tex = ResData_getTexture(texindex);
  proj->texRID = texindex;

  LUX_ARRAY3SET(black,5,5,5);
  if (tex->format == TEX_FORMAT_CUBE){
    proj->cubemap = LUX_TRUE;
  }
  else{
    //vidBindTexture(texindex);
    proj->cubemap = LUX_FALSE;
  }

  switch (blendmode){
    case VID_AMODADD:
      proj->blend.blendmode = VID_AMODADD;
      proj->blend.blendinvert = LUX_FALSE;
      if (!GLEW_NV_texture_env_combine4 && !GLEW_ATI_texture_env_combine3)
        proj->nopass = LUX_FALSE;
      else
        proj->nopass = LUX_TRUE;
      proj->attmode = VID_MODULATE;
      break;
    case VID_ADD:
      proj->blend.blendmode = VID_ADD;
      proj->blend.blendinvert = LUX_FALSE;
      proj->nopass = LUX_TRUE;
      proj->attmode = VID_MODULATE;
      break;
    case VID_DECAL:
      proj->blend.blendmode = VID_DECAL;
      proj->blend.blendinvert = LUX_FALSE;
      proj->nopass = LUX_TRUE;
      proj->attmode = VID_MODULATE;
      break;
    case VID_MODULATE:
    default:
      proj->blend.blendmode = VID_MODULATE;
      proj->blend.blendinvert = LUX_FALSE;
      proj->nopass = LUX_TRUE;
      if (lxVector3Compare(tex->border,<,black))
        proj->attmode = VID_MODULATE;
      else
        proj->attmode = VID_ADD_INV;
      break;
  }
  if (attenuate && g_VID.gensetup.attenuate3dRID >= 0){
    proj->nopass = LUX_FALSE;
    proj->attenuate = LUX_TRUE;
  }
  else
    proj->attenuate = LUX_FALSE;

  proj->thnopass = VIDTexBlendHash_get(proj->blend.blendmode,VID_A_REPLACE_PREV,LUX_FALSE,0,0);
  proj->thnopassatt = VIDTexBlendHash_get(proj->attmode,VID_A_REPLACE_PREV,LUX_FALSE,0,0);

  clearFlag(proj->changed,PROJECTOR_CHANGE_TEX);
}
void Projector_setProjection(Projector_t *proj,const float fov,const float front,const float back,const float aspect){
  if (fov > 0){
    lxMatrix44Perspective(proj->projmatrix,fov,front,back,aspect);
  }
  else{
    lxMatrix44Ortho(proj->projmatrix,-fov,front,back,aspect);
  }
  proj->range = back;
  proj->scalef =  1.0f / (2 * back);
  proj->fov = fov;
  proj->aspect = aspect;

  clearFlag(proj->changed,PROJECTOR_CHANGE_PROJ);
}

  // returns 1 for scissor -1 for invisible and 0 for no scissor
int Projector_prepScissor(Projector_t *proj, int oldscissor[4])
{
  lxVector4 test;
  int   newscissor[4];

  int i;
  if (proj->lastcam != g_CamLight.camera){
    proj->lastcam = g_CamLight.camera;
    // compute 2d rectangle

    lxVector3Copy(test,proj->frustumBoxCorners[0]);
    test[3] = 1.0f;
    lxVector4Transform1(test,g_CamLight.camera->mviewproj);
    proj->camRectangle[0]=((test[0]/test[3])*0.5f + 0.5f);
    proj->camRectangle[1]=((test[1]/test[3])*0.5f + 0.5f);
    proj->camRectangle[2] = proj->camRectangle[0];
    proj->camRectangle[3] = proj->camRectangle[1];

    for (i = 1; i < 8; i ++){
      lxVector3Copy(test,proj->frustumBoxCorners[i]);
      test[3] = 1.0f;
      lxVector4Transform1(test,g_CamLight.camera->mviewproj);
      test[0]=((test[0]/test[3])*0.5f + 0.5f);
      test[1]=((test[1]/test[3])*0.5f + 0.5f);
      // minmax
      proj->camRectangle[0] = LUX_MIN(test[0],proj->camRectangle[0]);
      proj->camRectangle[1] = LUX_MIN(test[1],proj->camRectangle[1]);
      proj->camRectangle[2] = LUX_MAX(test[0],proj->camRectangle[2]);
      proj->camRectangle[3] = LUX_MAX(test[1],proj->camRectangle[3]);
    }

    // rectangle should be within 0-1
    proj->camRectangle[0] = LUX_MIN(1,LUX_MAX(0,proj->camRectangle[0]));
    proj->camRectangle[1] = LUX_MIN(1,LUX_MAX(0,proj->camRectangle[1]));
    proj->camRectangle[2] = LUX_MIN(1,LUX_MAX(0,proj->camRectangle[2]));
    proj->camRectangle[3] = LUX_MIN(1,LUX_MAX(0,proj->camRectangle[3]));

    // rectangle width/height
    proj->camRectangle[2]-=proj->camRectangle[0];
    proj->camRectangle[3]-=proj->camRectangle[1];
    // if the rectangle area is relative big dont scissor
    test[0] = proj->camRectangle[2]*proj->camRectangle[3];
    proj->camScissor = test[0] < PROJECTOR_SCISSOR_AREA ? LUX_TRUE : LUX_FALSE;
    proj->camInvisible = test[0] < LUX_FLOAT_EPSILON*LUX_FLOAT_EPSILON ? LUX_TRUE : LUX_FALSE;
  }
  if (proj->camInvisible)
    return -1;
  if (!proj->camScissor)
    return 0;

  // new scissor sizes
  newscissor[0] = oldscissor[0] + (int)((float)oldscissor[2]*proj->camRectangle[0]);
  newscissor[1] = oldscissor[1] + (int)((float)oldscissor[3]*proj->camRectangle[1]);
  newscissor[2] = (int)((float)oldscissor[2]*proj->camRectangle[2]);
  newscissor[3] = (int)((float)oldscissor[3]*proj->camRectangle[3]);

  glScissor(lxVector4Unpack(newscissor));
  vidScissor(LUX_TRUE);

  return 1;
}

int Projector_stageGL(const Projector_t *proj,const int texunit,const int pass){
  static lxMatrix44SIMD identity;

  const float *planeS = &proj->vprojmatrixT[0];
  const float *planeT = &proj->vprojmatrixT[4];
  const float *planeR = &proj->vprojmatrixT[8];
  const float *planeQ = &proj->vprojmatrixT[12];



  vidSelectTexture(texunit);
  vidBindTexture(proj->texRID);
  vidTexturing(ResData_getTextureTarget(proj->texRID));
  ResData_setTextureClamp(proj->texRID,TEX_CLAMP_ALL);


  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadMatrixf(g_VID.drawsetup.viewMatrix);

  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
  glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);


  glTexGenfv(GL_S, GL_EYE_PLANE, planeS);
  glTexGenfv(GL_T, GL_EYE_PLANE, planeT);
  glTexGenfv(GL_R, GL_EYE_PLANE, planeR);
  glTexGenfv(GL_Q, GL_EYE_PLANE, planeQ);


  glPopMatrix();

  vidTexCoordSource(VID_TEXCOORD_TEXGEN_STRQ,VID_TEXCOORD_NOSET);

  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);


  if (!pass)
    vidTexBlend(proj->thnopass,proj->blend.blendmode,VID_A_REPLACE_PREV,LUX_FALSE,0,0);
  else{
    vidTexBlendDefault(texunit ? VID_TEXBLEND_REP_MOD : VID_TEXBLEND_REP_REP);
  }

  if (proj->attenuate && texunit+1 < g_VID.capTexCoords){
    lxMatrix44IdentitySIMD(identity);
    identity[3] = -proj->pos[0];
    identity[7] = -proj->pos[1];
    identity[11] = -proj->pos[2];
    planeS = &identity[0];
    planeT = &identity[4];
    planeR = &identity[8];

    vidSelectTexture(texunit +1);
    vidBindTexture(g_VID.gensetup.attenuate3dRID);
    vidTexBlend(proj->thnopassatt,proj->attmode,VID_A_REPLACE_PREV,LUX_FALSE,0,0);

    vidTexturing(GL_TEXTURE_3D);

    glPushMatrix();
    glLoadMatrixf(g_VID.drawsetup.viewMatrix);

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

    glTexGenfv(GL_S, GL_EYE_PLANE, planeS);
    glTexGenfv(GL_T, GL_EYE_PLANE, planeT);
    glTexGenfv(GL_R, GL_EYE_PLANE, planeR);

    glPopMatrix();

    vidTexCoordSource(VID_TEXCOORD_TEXGEN_STR,VID_TEXCOORD_NOSET);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(0.5,0.5,0.5);
    glScalef(proj->scalef,proj->scalef,proj->scalef);

    glMatrixMode(GL_MODELVIEW);

    return texunit + 2;
  }
  return texunit+1;
}
//////////////////////////////////////////////////////////////////////////
// SKYBOX
// ------

SkyBox_t *SkyBox_new(int cubetexRID,char *northname,  char *southname, char *westname, char* eastname, char* topname, char *bottomname){
  char *names[6];
  SkyBox_t *skybox;

  if ((cubetexRID >= 0 && ResData_getTexture(cubetexRID)->format != TEX_FORMAT_CUBE) ||
    (cubetexRID < 0 && (!northname || !southname || !eastname || !westname || !topname || !bottomname)))
    return NULL;

  skybox = lxMemGenZalloc(sizeof(SkyBox_t));
  memset(skybox,-1,sizeof(SkyBox_t));

  skybox->fovfactor = 1.0f;

  if (cubetexRID >= 0){
    skybox->cubeRID = cubetexRID;
  }
  else if (GLEW_ARB_texture_cube_map){
    names[0] = westname;
    names[1] = eastname;
    names[2] = bottomname;
    names[3] = topname;
    names[4] = northname;
    names[5] = southname;

    skybox->cubeRID = ResData_addTextureCombine(names,6,TEX_COLOR,LUX_FALSE,TEX_ATTR_MIPMAP | TEX_ATTR_CUBE);
  }
  else{
    skybox->bottomRID = ResData_addTexture(bottomname,TEX_COLOR,TEX_ATTR_MIPMAP);
    skybox->topRID    = ResData_addTexture(topname,TEX_COLOR,TEX_ATTR_MIPMAP);
    skybox->eastRID   = ResData_addTexture(eastname,TEX_COLOR,TEX_ATTR_MIPMAP);
    skybox->westRID   = ResData_addTexture(westname,TEX_COLOR,TEX_ATTR_MIPMAP);
    skybox->northRID  = ResData_addTexture(northname,TEX_COLOR,TEX_ATTR_MIPMAP);
    skybox->southRID  = ResData_addTexture(southname,TEX_COLOR,TEX_ATTR_MIPMAP);
  }

  skybox->reference = Reference_new(LUXI_CLASS_SKYBOX,skybox);

  return skybox;
}

static void SkyBox_free(SkyBox_t *skybox){

  List3D_removeSkyBox(skybox);
  Reference_invalidate(skybox->reference);
  lxMemGenFree(skybox,sizeof(SkyBox_t));
}
void RSkyBox_free (lxRskybox ref)
{
  SkyBox_free((SkyBox_t*)Reference_value(ref));
}
//////////////////////////////////////////////////////////////////////////
// SkyDome

/*
void SkyDome_createMesh(SkyDome_t *skydome)
{

  //  x - x - x - x
  //   \f/ \ / \ /
  //    x - x - x   toptris first
  //   /f\ / \ / \
  //  x - x - x - x bottomtris first
  //   \f/ \ / \ /
  //    x - x - x   toptris first

  //skymesh
  //vertices:
  //  per odd row = 1 + density
  //  per even row = 2 + density
  //  total = (1 + density)*(1+density) + (density+1)/2
  //tris:
  //  quad per field = density * density * 2
  //  plus one tris per row



  Mesh_t *mesh;
  Vertex64_t *vertex;
  Vector3 normal;
  float anglestart;
  float angleincr;
  float yawangle;
  float maxangle;
  float divfov;
  float pitchangle;
  float pitchincr;
  float ystart;
  float ystep;
  ushort *index;
  int baserow;
  int toprow;
  int topfirst;
  int x,y;
  float stretchz = 1;

  Matrix44 rotmat;
  Vector3 angles;

  if (skydome->mesh && skydome->modelRID < 0)
    Mesh_free(skydome->mesh);



  mesh = skydome->mesh = Mesh_new((1+skydome->density)*(skydome->density+1) + ((skydome->density+1)/2),
    ((skydome->density*skydome->density*2) + skydome->density)*3,
    VERTEX_64_TEX4);

  mesh->numIndices = mesh->numAllocIndices;
  mesh->numVertices = mesh->numAllocVertices;
  mesh->indexMin = 0;
  mesh->indexMax = mesh->numVertices-1;
  mesh->numTris = mesh->numIndices/3;
  mesh->primtype = GL_TRIANGLES;

  anglestart = -LUX_DEG2RAD(skydome->maxfov*0.5f);
  divfov = 1.0f/LUX_DEG2RAD(skydome->maxfov);
  pitchincr = angleincr = LUX_DEG2RAD(skydome->maxfov/(float)skydome->density);
  maxangle = LUX_DEG2RAD(skydome->maxfov*0.5f);

  pitchangle = anglestart;
  // tan a = y/dist
  ystart = -(tan(-anglestart)*skydome->distance);
  ystep = -ystart*2.0 / (float)skydome->density;


  Matrix44Identity(rotmat);
  angles[2] = 0.0f;


  // generate vertices
  vertex = mesh->vertexData64;
  for (y = 0; y < skydome->density+1 ; y++){
    int rowlength;
    if ((y+1)%2){
      rowlength = skydome->density + 1;
      yawangle = anglestart;
    }
    else{
      rowlength = skydome->density + 2;
      yawangle = anglestart - (angleincr*0.5f);
    }
    for (x = 0; x < rowlength; x++,vertex++){
      // spherical texcoords
      angles[0] = pitchangle;
      angles[1] = -yawangle;
      Matrix44FromEulerZYX(rotmat,angles);
      Vector3Set(normal,0,0,1);
      Vector3TransformRot(vertex->user4,normal,rotmat);
      vertex->user4[1] *=-1.0f;
      vertex->user4[3] = 1.0f;

      // cylindrical position

      angles[0] = 0.0f;
      Matrix44FromEulerZYX(rotmat,angles);
      Vector3Set(normal,0,0,-skydome->distance);
      Vector3TransformRot1(normal,rotmat);

      Vector3Copy(vertex->pos,normal);
      Vector3Negated(normal);
      Vector3Normalized(normal);
      Vector3short_FROM_float(vertex->normal,normal);
      vertex->pos[1] = ystart;



      Vector2Set(vertex->tex,(yawangle-anglestart)*divfov,(pitchangle-anglestart)*divfov);
      Vector2Set(vertex->tex2,0,vertex->pos[2]);
      vertex->colorc = BIT_ID_FULL32;

      yawangle += angleincr;
    }
    pitchangle += pitchincr;
    ystart+=ystep;
  }

  index = mesh->indicesData16;

  baserow = 0;

  for (y = 0; y < skydome->density ; y++){
    toprow = baserow + (skydome->density+1);
    if ((y+1)%2){
      topfirst = 1;
    }
    else{
      toprow++;
      topfirst = 0;
    }
    // start with low
    // topfirst means if we start with top tris first
    // then top one
    for (x = 0; x < (skydome->density*2)+1; x++){
      int cur = x/2;

      if ((x+1)%2 == topfirst){
        *index = baserow + cur +1 -topfirst;  index++;
        *index = toprow + cur + 1;        index++;
        *index = toprow + cur;          index++;
      }
      else{
        *index = baserow + cur ;      index++;
        *index = baserow + cur +1;      index++;
        *index = toprow + cur + topfirst; index++;
      }
    }

    baserow = toprow;
  }

}
*/

//////////////////////////////////////////////////////////////////////////
// FrustumObject

FrustumObject_t* FrustumObject_new()
{
  FrustumObject_t* fcont = lxMemGenZalloc(sizeof(FrustumObject_t));

  lxListNode_init(fcont);
  fcont->listHead = LUX_FALSE;

  fcont->ref = Reference_new(LUXI_CLASS_FRUSTUMOBJECT,fcont);

  return fcont;
}

void RFrustumObject_free(lxRfrustumobject ref)
{
  FrustumObject_t* fcont;
  Reference_get(ref,fcont);

  lxMemGenFree(fcont,sizeof(FrustumObject_t));
}
