// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/reflib.h"

#include "../render/gl_camlight.h"
#include "../scene/scenetree.h"
#include "../scene/actorlist.h"
#include "../common/3dmath.h"
#include "../render/gl_window.h"
#include "../render/gl_list3d.h"

// Published here:
// LUXI_CLASS_L3D_CAMERA
// LUXI_CLASS_FRUSTUMOBJECT


static int PubCamera_new (PState state, PubFunction_t *fn, int n)
{
  List3DNode_t *cam;
  int shift;
  int layer = List3D_getDefaultLayer();
  char *name = NULL;

  if (n<2 || (shift=FunctionPublish_getArg(state,2,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_L3D_LAYERID,(void*)&layer))<1 ||
    !FunctionPublish_getNArg(state,shift,LUXI_CLASS_INT,(void*)&shift)
    || shift<1 || shift>31)
    return FunctionPublish_returnError(state,"1 string [1 l3dlayerid] 1 int 1-31 required");

  if (Camera_bitInUse(shift))
    return FunctionPublish_returnError(state,"l3dcamera id already in use");

  cam = List3DNode_newCamera(name,layer,shift);
  if (!cam)
    return 0;

  Reference_makeVolatile(cam->reference);
  return FunctionPublish_returnType(state,LUXI_CLASS_L3D_CAMERA,REF2VOID(cam->reference));
}


static int PubCamera_default (PState state,PubFunction_t *fn, int n)
{
  List3DNode_t *cam;
  Reference ref;

  if (n==0){
    return FunctionPublish_returnType(state,LUXI_CLASS_L3D_CAMERA,REF2VOID(g_CamLight.camera->l3dnode->reference));
  }

  if (n!=1 || !FunctionPublish_getNArg(state,0,LUXI_CLASS_L3D_CAMERA,(void*)&ref) || !Reference_get(ref,cam))
    return FunctionPublish_returnError(state,"1 l3dcamera or no arg required");

  Camera_activateMain(cam->cam);

  return 0;
}

enum PubCameraCmd_e
{
  PUBCAM_FOV,
  PUBCAM_NEARPLANE,
  PUBCAM_FARPLANE,
  PUBCAM_ASPECT,
  PUBCAM_NEARPERCENTAGE,
  PUBCAM_FUNC_OWNARGS, // <- ...
  PUBCAM_VISFLAGID,
  PUBCAM_REFLECT,
  PUBCAM_REFLECTPLANE,
  PUBCAM_CLIP,
  PUBCAM_CLIPPLANE,
  PUBCAM_USEINF,
  PUBCAM_CW,
  PUBCAM_UPDATE,
  PUBCAM_MANPROJ,
  PUBCAM_ADDFOBJ,
  PUBCAM_REMFOBJ,

  PUBCAM_AXISUP,
  PUBCAM_AXISALL,

  PUBCAM_VIEWMATRIX,
  PUBCAM_PROJMATRIX,
  PUBCAM_INVPROJMATRIX,
  PUBCAM_VIEWPROJMATRIX,
};

static int PubCamera_attributes (PState state,PubFunction_t *fn, int n)
{
  Camera_t *cam;
  List3DNode_t *l3dcam;
  Reference ref;
  float value;
  int attr;
  float *matrix;

  //ref = NULL;

  if (n < 1 || !FunctionPublish_getNArg(state,0,LUXI_CLASS_L3D_CAMERA,(void*)&ref) || !Reference_get(ref,l3dcam)) {
    return FunctionPublish_returnError(state,"1 l3dcamera required");
  }
  cam = l3dcam->cam;


  attr = (int)fn->upvalue;

  if (attr < PUBCAM_FUNC_OWNARGS && n==2 && !FunctionPublish_getNArg(state,1,LUXI_CLASS_FLOAT,(void*)&value))
    return FunctionPublish_returnError(state,"1 l3dcamera 1 float required");

  switch(attr) {
  case PUBCAM_VISFLAGID:
    return FunctionPublish_returnInt(state,cam->bit);
  case PUBCAM_USEINF:
    if (n==1) return FunctionPublish_returnBool(state,cam->infinitebackplane);
    else if (!FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&cam->infinitebackplane))
      return FunctionPublish_returnError(state,"1 l3dcamera 1 boolean required");
    break;
  case PUBCAM_NEARPERCENTAGE:
    if (n==2) cam->nearpercentage = value;
    else    return FunctionPublish_returnFloat(state,cam->nearpercentage);
    break;
  case PUBCAM_FOV:
    if (n==2) cam->fov = value;
    else    return FunctionPublish_returnFloat(state,cam->fov);
    break;
  case PUBCAM_NEARPLANE:
    if (n==2) cam->frontplane = LUX_MAX(value,0);
    else    return FunctionPublish_returnFloat(state,cam->frontplane);
    break;
  case PUBCAM_FARPLANE:
    if (n==2) cam->backplane  = LUX_MAX(value,0);
    else    return FunctionPublish_returnFloat(state,cam->backplane);
    break;
  case PUBCAM_ASPECT:
    if (n==2) cam->aspect = value;
    else    return FunctionPublish_returnFloat(state,cam->aspect);
    break;
  case PUBCAM_CLIPPLANE:
    if (n==5 && 4!= FunctionPublish_getArgOffset(state,1,4,FNPUB_TOVECTOR4(cam->clipplane)))
      return FunctionPublish_returnError(state,"1 l3dcamera 4 float required");
    else    return FunctionPublish_setRet(state,4,FNPUB_FROMVECTOR4(cam->clipplane));
    lxVector3NormalizedFast(cam->clipplane);
    break;
  case PUBCAM_CLIP:
    if (n==2 && !FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&cam->useclip))
      return FunctionPublish_returnError(state,"1 l3dcamera 1 boolean required");
    else    return FunctionPublish_returnBool(state,cam->useclip);
    break;
  case PUBCAM_REFLECT:
    if (n==2 && !FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&cam->usereflect))
      return FunctionPublish_returnError(state,"1 l3dcamera 1 boolean required");
    else    return FunctionPublish_returnBool(state,cam->usereflect);
    break;
  case PUBCAM_REFLECTPLANE:
    if (n==5 && 4!= FunctionPublish_getArgOffset(state,1,4,FNPUB_TOVECTOR4(cam->reflectplane)))
      return FunctionPublish_returnError(state,"1 l3dcamera 4 float required");
    else    return FunctionPublish_setRet(state,4,FNPUB_FROMVECTOR4(cam->reflectplane));
    lxVector3Normalized(cam->reflectplane);
    break;
  case PUBCAM_CW:
    if (n==2 && !FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&cam->frontcw))
      return FunctionPublish_returnError(state,"1 l3dcamera 1 boolean required");
    else    return FunctionPublish_returnBool(state,cam->frontcw);
    break;
  case PUBCAM_MANPROJ:
    if (n==2 && !FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&cam->userproj))
      return FunctionPublish_returnError(state,"1 l3dcamera 1 boolean required");
    else    return FunctionPublish_returnBool(state,cam->userproj);
    break;
  case PUBCAM_UPDATE:
    cam->dirtyframe = -1;
    List3DNode_updateUp_matrix_recursive(cam->l3dnode);
    Camera_update(cam,cam->l3dnode->finalMatrix);
    attr = LUX_FALSE;
    if (n==2 && FunctionPublish_getNArg(state,1,FNPUB_TBOOL(attr)) && attr){
      cam->l3dnode->updFrame = -1;
      cam->dirtyframe = -1;
    }
    break;
  case PUBCAM_PROJMATRIX:
    if (n==1)   return PubMatrix4x4_return(state,cam->proj);
    else if (n < 2 || !FunctionPublish_getNArg(state,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,matrix))
      return FunctionPublish_returnError(state,"1 l3dcamera 1 matrix4x4 required");
    lxMatrix44CopySIMD(cam->proj,matrix);
    return 0;
  case PUBCAM_INVPROJMATRIX:
    return PubMatrix4x4_return(state,cam->projinv);
  case PUBCAM_VIEWMATRIX:
    return PubMatrix4x4_return(state,cam->mview);
  case PUBCAM_VIEWPROJMATRIX:
    return PubMatrix4x4_return(state,cam->mviewproj);
  case PUBCAM_ADDFOBJ:
    {
      FrustumObject_t *fobj;
      if (!FunctionPublish_getNArg(state,1,LUXI_CLASS_FRUSTUMOBJECT,(void*)&ref) || !Reference_get(ref,fobj))
        return FunctionPublish_returnError(state,"1 frustumobject required");

      return FunctionPublish_returnBool(state,Camera_addFrustumObject(cam,fobj));
    }

  case PUBCAM_REMFOBJ:
    {
      FrustumObject_t *fobj = NULL;
      if (FunctionPublish_getNArg(state,1,LUXI_CLASS_FRUSTUMOBJECT,(void*)&ref) && !Reference_get(ref,fobj))
        return FunctionPublish_returnError(state,"1 frustumobject required");

      if (fobj){
        return FunctionPublish_returnBool(state,Camera_remFrustumObject(cam,fobj));
      }
      else{
        Camera_remFrustumObjects(cam);
        return FunctionPublish_returnBool(state,LUX_TRUE);
      }
    }
  default:
    LUX_ASSUME(0);
    break;
  }

  return 0;
}

static int  PubCamera_viewrect(PState state,PubFunction_t *f, int n)
{
  static lxMatrix44SIMD mat;

  Reference ref;
  int   axis;
  float dist;
  float t;
  lxVector3 rays[4];

  Camera_t *cam;
  List3DNode_t *l3dcam;
  int i;


  if (n!=3 || 3!=FunctionPublish_getArg(state,3,LUXI_CLASS_L3D_CAMERA,&ref,LUXI_CLASS_INT,&axis,LUXI_CLASS_FLOAT,&dist) ||
    !Reference_get(ref,l3dcam))
    return FunctionPublish_returnError(state,"1 l3dcamera 1 int(0-2) 1 float required.");

  cam = l3dcam->cam;

  lxVector3Set(rays[0],0,1,0);
  lxMatrix44Identity(mat);
  lxVector3Set(rays[1],cam->fov/g_Window.ratio,0,-cam->fov);
  lxMatrix44FromEulerZYXdeg(mat,rays[1]);
  lxMatrix44VectorRotate(mat,rays[0]);
  lxVector3Set(rays[1],-rays[0][0],rays[0][1], rays[0][2]);
  lxVector3Set(rays[2],-rays[0][0],rays[0][1],-rays[0][2]);
  lxVector3Set(rays[3], rays[0][0],rays[0][1],-rays[0][2]);

  // rays set, now transform
  for (i = 0; i < 4; i++)
    lxMatrix44VectorRotate(cam->finalMatrix,rays[i]);

  // hittest with plane
  for (i = 0; i < 4; i++){
    if (!rays[i][axis])
      return 0;

    // t * ray + cam.pos = axis*dist
    t = (dist - cam->pos[axis])/rays[i][axis];
    if (t < 0)
      return 0;
    lxVector3ScaledAdd(rays[i],cam->pos,t,rays[i]);
  }

  return FunctionPublish_setRet(state,12,FNPUB_FROMVECTOR3(rays[0]),FNPUB_FROMVECTOR3(rays[1]),FNPUB_FROMVECTOR3(rays[2]),FNPUB_FROMVECTOR3(rays[3]));
}


static int PubCamera_screen (PState state,PubFunction_t *fn, int n)
{
  Camera_t *cam;
  List3DNode_t *l3dcam;
  Reference ref;
  lxVector4 pos;
  float w,h;

  w = VID_REF_WIDTH;
  h = VID_REF_HEIGHT;

  if (n<4 || FunctionPublish_getArg(state,6,LUXI_CLASS_L3D_CAMERA,(void*)&ref,FNPUB_TOVECTOR3(pos),LUXI_CLASS_FLOAT,(void*)&w,LUXI_CLASS_FLOAT,(void*)&h)<4 ||
    !Reference_get(ref,l3dcam))
    return FunctionPublish_returnError(state,"1 l3dcamera 3 floats [2 floats] required");

  cam = l3dcam->cam;

  pos[3]=1.0f;
  lxVector4Transform1(pos,cam->mviewproj);
  pos[0]=((pos[0]/pos[3])*0.5f + 0.5f)*w;
  pos[1]=((-pos[1]/pos[3])*0.5f + 0.5f)*h;

  return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(pos));
}

static int PubCamera_view (PState state,PubFunction_t *fn, int n)
{
  Camera_t *cam;
  List3DNode_t *l3dcam;
  Reference ref;
  lxVector4 pos;

  pos[3] = 1.0;
  if (n<4 || FunctionPublish_getArg(state,5,LUXI_CLASS_L3D_CAMERA,(void*)&ref,FNPUB_TOVECTOR4(pos))<4 ||
    !Reference_get(ref,l3dcam))
    return FunctionPublish_returnError(state,"1 l3dcamera 3/4 floats required");

  cam = l3dcam->cam;

  lxVector4Transform1(pos,cam->mview);

  return FunctionPublish_setRet(state,4,FNPUB_FROMVECTOR4(pos));
}

static int PubCamera_world (PState state,PubFunction_t *fn, int n)
{
  Camera_t *cam;
  Reference ref;
  List3DView_t *l3dview;
  lxVector4 pos;
  List3DNode_t *l3dcam;
  booln inview = LUX_FALSE;

  int viewport[] = {0,0,(int)VID_REF_WIDTH,(int)VID_REF_HEIGHT};


  if (n<4 || FunctionPublish_getArg(state,4,LUXI_CLASS_L3D_CAMERA,(void*)&ref,FNPUB_TOVECTOR3(pos))!=4||
    !Reference_get(ref,l3dcam))
    return FunctionPublish_returnError(state,"1 l3dcamera 3 floats required");

  cam = l3dcam->cam;

  l3dview = NULL;
  if (n >= 5 && (!FunctionPublish_getArgOffset(state,4,2,LUXI_CLASS_L3D_VIEW,(void*)&ref,LUXI_CLASS_BOOLEAN,(void*)&inview) ||
    !Reference_get(ref,l3dview))){
    return FunctionPublish_returnError(state,"1 l3dview [1 boolean] required");
  }

  if (inview && l3dview){
    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = l3dview->viewport.bounds.sizes[2];
    viewport[3] = l3dview->viewport.bounds.sizes[3];

    pos[1] = (float)viewport[3] - pos[1];
  }
  else{
    if (l3dview && !l3dview->viewport.bounds.fullwindow){
      viewport[0] = (int)(VID_REF_WIDTHSCALE*(l3dview->viewport.bounds.sizes[0]));
      viewport[1] = (int)(VID_REF_HEIGHTSCALE*(l3dview->viewport.bounds.sizes[1]));
      viewport[2] = (int)(VID_REF_WIDTHSCALE*(l3dview->viewport.bounds.sizes[2]));
      viewport[3] = (int)(VID_REF_HEIGHTSCALE*(l3dview->viewport.bounds.sizes[3]));
    }

    pos[1] = (float)VID_REF_HEIGHT - pos[1];
  }

  Camera_screenToWorld(cam,pos,pos,viewport);
  return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(pos));
}

enum PubFrust_e{
  PUBFRUST_NEW,
  PUBFRUST_CORNER,
  PUBFRUST_PLANE,
  PUBFRUST_UPDATE,
  PUBFRUST_PROJMAT,
};

static int PubFrustum_attributes(PState pstate,PubFunction_t *fn, int n)
{
  Reference ref;
  FrustumObject_t *fobj;

  if ((int)fn->upvalue > PUBFRUST_NEW){
    if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_FRUSTUMOBJECT,(void*)&ref) || !Reference_get(ref,fobj))
      return FunctionPublish_returnError(pstate,"1 frustumobject required.");
  }

  switch((int)fn->upvalue)
  {
    case PUBFRUST_NEW:
      ref = FrustumObject_new()->ref;
      Reference_makeVolatile(ref);
      return FunctionPublish_returnType(pstate,LUXI_CLASS_FRUSTUMOBJECT,REF2VOID(ref));

    case PUBFRUST_CORNER:
      {
        int idx;
        if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&idx) || idx < 0 || idx >= LUX_FRUSTUM_CORNERS)
          return FunctionPublish_returnError(pstate,"1 valid index 0-7 required.");

        if (n==2){
          return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(fobj->frustumBoxCorners[idx]));
        }
        else if (!FunctionPublish_getArgOffset(pstate,2,3,FNPUB_TOVECTOR3(fobj->frustumBoxCorners[idx])))
          return FunctionPublish_returnError(pstate,"3 floats required");

        return 0;
      }
      break;
    case PUBFRUST_PLANE:
      {
        int idx;
        if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&idx) || idx < 0 || idx >= LUX_FRUSTUM_PLANES)
          return FunctionPublish_returnError(pstate,"1 valid index 0-5 required.");


        if (n==2){
          return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(fobj->frustum.fplanes[idx].pvec));
        }
        else if (!FunctionPublish_getArgOffset(pstate,2,4,FNPUB_TOVECTOR4(fobj->frustum.fplanes[idx].pvec)))
          return FunctionPublish_returnError(pstate,"4 floats required");

        return 0;
      }
      break;
    case PUBFRUST_UPDATE:
      {
        booln fromcorners = LUX_TRUE;
        FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&fromcorners);

        if (fromcorners){
          lxFrustum_fromCorners(&fobj->frustum,fobj->frustumBoxCorners);
        }
        else{
          lxFrustum_getCorners(&fobj->frustum,fobj->frustumBoxCorners);
        }

        lxFrustum_updateSigns(&fobj->frustum);
        return 0;
      }
      break;
    case PUBFRUST_PROJMAT:
      {
        float *mat;
        if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,mat))
          return FunctionPublish_returnError(pstate,"1 matrix44 required");

        lxFrustum_update(&fobj->frustum,mat);
        return 0;
      }
    default:
      LUX_ASSUME(0);
      return 0;
  }
}


void PubClass_Camera_init()
{

  FunctionPublish_initClass(LUXI_CLASS_FRUSTUMOBJECT,"frustumobject",
    "l3dcameras can use multiple user defined frustumobjects instead of the automatically created frustum. "
    "The latter is based on the projection settings. "
    "A frusutmobject is user defined and consists of 6 planes defining the frustum and the 8 points of the corners. "
    "At runtime all assoicated frustumobjects are used for the visibility test of that camera. frustumobjects are stored in world space."
    ,NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_FRUSTUMOBJECT,LUXI_CLASS_L3D_LIST);

  FunctionPublish_addFunction(LUXI_CLASS_FRUSTUMOBJECT,PubFrustum_attributes,(void*)PUBFRUST_NEW,"new",
    "(frustumobject):() - returns self");
  FunctionPublish_addFunction(LUXI_CLASS_FRUSTUMOBJECT,PubFrustum_attributes,(void*)PUBFRUST_CORNER,"corner",
    "([float x,y,z]):(frustumobject,int index,[float x,y,z]) - returns or sets ith 0-7 corner.");
  FunctionPublish_addFunction(LUXI_CLASS_FRUSTUMOBJECT,PubFrustum_attributes,(void*)PUBFRUST_PLANE,"plane",
    "([float x,y,z,-d]):(frustumobject,int index,[float x,y,z,-d]) - returns or sets ith 0-5 plane. Plane normals point to inside.");
  FunctionPublish_addFunction(LUXI_CLASS_FRUSTUMOBJECT,PubFrustum_attributes,(void*)PUBFRUST_UPDATE,"update",
    "():(frustumobject, [boolean fromcorners]) - updates vice versa from planes or corners (default is true).");
  FunctionPublish_addFunction(LUXI_CLASS_FRUSTUMOBJECT,PubFrustum_attributes,(void*)PUBFRUST_PROJMAT,"frommatrix",
    "():(frustumobject, matrix44 mat) - updates planes from given projection matrix.");


  FunctionPublish_initClass(LUXI_CLASS_L3D_CAMERA,"l3dcamera",
    "Cameras are used to render the frame."
  "You can link them with actornodes or scenenodes. And activate them in specific l3dviews, "
  "or globally by making it the default camera. However there is always a default camera at startup."
    ,NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_CAMERA,LUXI_CLASS_L3D_NODE);

  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_new,NULL,"new",
    "([l3dcamera]):(string name,[l3dlayerid], int id 1-31) - creates new camera. The id must be unique and a l3dnode must get an update to its visflag, if it should be visible to it.");
  //FunctionPublish_addFunction(LUXI_CLASS_CAMERA,PubCamera_freeall,NULL,"deleteall",
  //  "():() - deletes all existing cameras");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_default,NULL,"default",
    "([l3dcamera]):([l3dcamera]) - returns or sets current global default camera");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_USEINF,"useinfinitebackplane",
    "([boolean]):(l3dcamera,[boolean]) - returns or sets if this camera needs shadow projection and therefore infinite backplane should be used. (only works for perspective projection)");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_FOV,"fov",
    "([float]):(l3dcamera,[float]) - returns or sets fov horizontal angle. Negative values are used for orthographic projection and their absolute value will become width of view.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_NEARPLANE,"frontplane",
    "([float]):(l3dcamera,[float]) - returns or sets frontplane");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_NEARPERCENTAGE,"nearpercentage",
    "([float]):(l3dcamera,[float]) - returns or sets percentage of what is considered near. range = backplane * nearpercentage. If a drawnode is closer than its drawn first. This helps making use of zbuffer better.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_FARPLANE,"backplane",
    "([float]):(l3dcamera,[float]) - returns or sets backplane");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_CLIP,"useclipping",
    "([boolean]):(l3dcamera,[boolean]) - returns or sets if clipping plane should be used");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_REFLECT,"usereflection",
    "([boolean]):(l3dcamera,[boolean]) - returns or sets if camera is reflected on the reflectplane. Useful for reflection rendering");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_CLIPPLANE,"clipplane",
    "([float x,y,z,w]):(l3dcamera,[float x,y,z,w]) - returns or sets clipping plane (worldspace). To make clipping useful the camera must be on the negative side of the plane.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_REFLECTPLANE,"reflectplane",
    "([float x,y,z,w]):(l3dcamera,[float x,y,z,w]) - returns or sets reflection plane (worldspace).");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_CW,"cwfront",
    "([boolean]):(l3dcamera,[boolean]) - returns or sets if front faces are clockwise, default off. Is needed when reflection plane creates artefacts");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_UPDATE,"forceupdate",
    "():(l3dcamera,[boolean leavedirty]) - forces the camera matrix to be updated (synchronized with its linked interface). If leavedirty is true, then the node will be updated in the next drawcall, otherwise results of this operation might be cached."
    "This is neccessary if you use the toscreen and toworld functions and changed the position of the camera.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_ASPECT,"aspect",
    "([float]):(l3dcamera,[float]) - returns or sets aspect ratio = width/height, if negative then current window aspect will be used.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_PROJMATRIX,"projmatrix",
    "([matrix4x4]):(l3dcamera,[matrix4x4]) - returns or sets current projection matrix.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_VIEWMATRIX,"viewmatrix",
    "([matrix4x4]):(l3dcamera) - returns viewmatrix, viewmatrixinv is worldmatrix of l3dnode.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_INVPROJMATRIX,"projmatrixinv",
    "([matrix4x4]):(l3dcamera) - returns current inverse of projection matrix.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_VIEWPROJMATRIX,"viewprojmatrix",
    "([matrix4x4]):(l3dcamera) - returns viewprojmatrix. Can be used to transform vectors into clipspace");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_VISFLAGID,"visflagid",
    "(int):(l3dcamera) - returns the id the camera was intialized with");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_MANPROJ,"usemanualprojection",
    "([boolean]):(l3dcamera,[boolean]) - returns or sets if manual projection matrix is used. That means aspect/frontplane.. will not modify projection matrix (settable via projmatrix).");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_ADDFOBJ,"addfobj",
    "(boolean success):(l3dcamera, frustumobject) - adds frustumobject to the camera, this automatically disables useage of the standard frustum for vistesting. ==prevents gc of frustumobject");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_attributes,(void*)PUBCAM_REMFOBJ,"remfobj",
    "(boolean success):(l3dcamera, [frustumobject]) - removes single frustumobject or all.");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_view,NULL,"toview",
    "(float x,y,z,w):(l3dcamera,float x,y,z,[w]) - computes position in modelview matrix. (w=1.0 default)");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_screen,NULL,"toscreen",
    "(float x,y,z):(l3dcamera,float x,y,z,[width,height]) - computes screen pixel position. Uses reference width/height or custom.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_world,NULL,"toworld",
    "(float x,y,z):(l3dcamera,float x,y,z,[l3dview,[viewlocal]]) - computes screen pixel position (reference coordinates or local to l3dview) to world coordinates."
    "The camera returns the coordinates seen from 0,0,0. You will need to transform the coordinate with a "
    "matrix, if you link the camera against an actornode/scenenode. If l3dview is not passed, we will use window.refsize.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_CAMERA,PubCamera_viewrect,NULL,"getrect",
    "(4 float x,y,z):(l3dcamera,int axis,float distance) - computes viewable rectangle on the given plane.\
    axis is the plane normal 0 = X, 1 = Y, 2 = Z. Method returns 4 Vectors that lie on the plane, or nothing if plane\
    wasnt intersected by frustum limiting rays. hitpoint order is, top left, top right, right bottom, left bottom");

}
