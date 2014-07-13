// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/reflib.h"
#include "../render/gl_list3d.h"
#include "../common/3dmath.h"


// Published here:
// LUXI_CLASS_L3D_PROJECTOR


static int PubProjector_new (PState pstate, PubFunction_t *fn, int n)
{
  List3DNode_t *l3d;
  char *name;
  int layer;

  layer = List3D_getDefaultLayer();

  if (n<1 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_STRING,(void*)&name, LUXI_CLASS_L3D_LAYERID, (void*)&layer)<1 )
    return FunctionPublish_returnError(pstate,"1 string [1 l3dlayerid] required");

  l3d = List3DNode_newProjector(name,layer);

  Reference_makeVolatile(l3d->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_PROJECTOR,REF2VOID(l3d->reference));
}
static int PubProjector_fx (PState pstate,PubFunction_t *fn, int n)
{
  List3DNode_t *l3d;
  Reference ref;
  int id;

  if ((n!=1 && n!=2) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_PROJECTOR,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 projector required");

  switch((int)fn->upvalue) {
  case 0:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&id))
      return FunctionPublish_returnError(pstate,"1 projector 1 int(0-31) required");

    List3D_activateProjector(l3d->setID,l3d->proj,id);
    break;
  case 1:
    List3D_deactivateProjector(l3d->proj);
    break;
  default:
    break;
  }

  return 0;
}

enum PubProjCmds_e
{
  PPROJ_TEX,
  PPROJ_BLEND,
  PPROJ_ATT,
  PPROJ_FOV,
  PPROJ_FPLANE,
  PPROJ_BPLANE,
  PPROJ_ASPECT,
  PPROJ_PROJMATRIX,
};

static int PubProjector_attributes (PState pstate,PubFunction_t *fn, int n)
{
  List3DNode_t *l3d;
  Projector_t *proj;
  Reference ref;
  float *matrix;

  if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_PROJECTOR,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dprojector required");

  proj = l3d->proj;

  switch((int)fn->upvalue) {
  case PPROJ_TEX: // texture
    if (n == 2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_TEXTURE,(void*)&proj->texRID))
        return FunctionPublish_returnError(pstate,"1 l3dprojector 1 texture required");

      proj->changed |= PROJECTOR_CHANGE_TEX;
    }
    else  return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)proj->texRID);
    break;
  case PPROJ_BLEND: // blendmode
    if (n == 2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BLENDMODE,(void*)&proj->blend.blendmode))
        return FunctionPublish_returnError(pstate,"1 l3dprojector 1 blendmode required");

      proj->changed |= PROJECTOR_CHANGE_TEX;
    }
    else  return FunctionPublish_returnType(pstate,LUXI_CLASS_BLENDMODE,(void*)proj->blend.blendmode);
    break;
  case PPROJ_ATT: // attenuate
    if (n == 2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&proj->attenuate))
        return FunctionPublish_returnError(pstate,"1 l3dprojector 1 boolean required");

      proj->changed |= PROJECTOR_CHANGE_TEX;
    }
    else  return FunctionPublish_returnBool(pstate,proj->attenuate);
    break;
  case PPROJ_FOV: // fov
    if (n == 2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&proj->fov))
        return FunctionPublish_returnError(pstate,"1 l3dprojector 1 float required");

      proj->changed |= PROJECTOR_CHANGE_PROJ;
    }
    else  return FunctionPublish_returnFloat(pstate,proj->fov);
    break;
  case PPROJ_FPLANE: // frontplane
    if (n == 2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&proj->frontplane))
        return FunctionPublish_returnError(pstate,"1 l3dprojector 1 float required");

      proj->changed |= PROJECTOR_CHANGE_PROJ;
    }
    else  return FunctionPublish_returnFloat(pstate,proj->frontplane);
    break;
  case PPROJ_BPLANE: // backplane
    if (n == 2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&proj->backplane))
        return FunctionPublish_returnError(pstate,"1 l3dprojector 1 float required");

      proj->changed |= PROJECTOR_CHANGE_PROJ;
    }
    else  return FunctionPublish_returnFloat(pstate,proj->backplane);
    break;
  case PPROJ_ASPECT: // aspect
    if (n == 2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&proj->aspect))
        return FunctionPublish_returnError(pstate,"1 l3dprojector 1 float required");

      proj->changed |= PROJECTOR_CHANGE_PROJ;
    }
    else  return FunctionPublish_returnFloat(pstate,proj->aspect);
    break;
  case PPROJ_PROJMATRIX:
    if (n == 1) return PubMatrix4x4_return(pstate,proj->projmatrix);
    else if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,matrix))
      return FunctionPublish_returnError(pstate,"1 l3dprojector 1 matrix4x4 required");
    lxMatrix44CopySIMD(proj->projmatrix,matrix);
    break;
  default:
    break;
  }

  return 0;
}

void PubClass_Projector_init()
{
  FunctionPublish_initClass(LUXI_CLASS_L3D_PROJECTOR,"l3dprojector","a projector allows textures to be\
    projected on models, orthogonally and perspectively. There is a special texture type that can fix \
    backprojection errors for perpective projection, but shouldnt be used on orthogonal, or if no errors with normal textures appear.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_PROJECTOR,LUXI_CLASS_L3D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PROJECTOR,PubProjector_new,NULL,"new",
    "(l3dprojector):(string name,|l3dlayerid layer|) - creates new projector");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_PROJECTOR,PubProjector_fx,(void*)0,"activate",
    "():(l3dprojector,int id) - adds projector to the active list with given id (0-31)");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PROJECTOR,PubProjector_fx,(void*)1,"deactivate",
    "():(l3dprojector) - removes projector from active list");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PROJECTOR,PubProjector_attributes,(void*)PPROJ_TEX,"textureid",
    "(texture):(l3dprojector,[texture]) - returns or sets projected texture");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PROJECTOR,PubProjector_attributes,(void*)PPROJ_BLEND,"blend",
    "(blendmode):(l3dprojector,[blendmode]) - returns or sets blendmode of projector");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PROJECTOR,PubProjector_attributes,(void*)PPROJ_ATT,"attenuate",
    "(boolean):(l3dprojector,[boolean]) - returns or sets attenuation, this is quite heavy effect but will perform a per pixel distance attenuation that causes the projector to fade out towards its backplane.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PROJECTOR,PubProjector_attributes,(void*)PPROJ_FOV,"fov",
    "(float):(l3dprojector,[float]) - returns or sets projection fov, when negative it will be orthogonal projection with abs(fov) rectangular size");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PROJECTOR,PubProjector_attributes,(void*)PPROJ_FPLANE,"frontplane",
    "(float):(l3dprojector,[float]) - returns or sets projection frontplane distance");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PROJECTOR,PubProjector_attributes,(void*)PPROJ_BPLANE,"backplane",
    "(float):(l3dprojector,[float]) - returns or sets projection backplane distance");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PROJECTOR,PubProjector_attributes,(void*)PPROJ_ASPECT,"aspect",
    "(float):(l3dprojector,[float]) - returns or sets projection aspect ratio = width/height");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PROJECTOR,PubProjector_attributes,(void*)PPROJ_PROJMATRIX,"projmatrix",
    "([matrix4x4]):(l3dprojector,[matrix4x4]) - returns or sets projection matrix. Using frontplane,aspect and so on will override manually set matrices.");

}
