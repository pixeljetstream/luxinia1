// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/common.h"
#include "../render/gl_camlight.h"
#include "../common/3dmath.h"
#include "../common/interfaceobjects.h"
#include "../common/reflib.h"


// Published here:
// LUXI_CLASS_SKYBOX

static int PubSkyBox_new(PState pstate,PubFunction_t *f, int n)
{
  SkyBox_t *skybox;
  char *name[6];
  int id;

  id = -1;

  if (n < 1 || (FunctionPublish_getNArgType(pstate,0)==LUXI_CLASS_TEXTURE && !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_TEXTURE,(void*)&id)) ||
    (FunctionPublish_getNArgType(pstate,0)!=LUXI_CLASS_TEXTURE && (n!=6 || 6!=FunctionPublish_getArg(pstate,6,LUXI_CLASS_STRING,(void*)&name[0],LUXI_CLASS_STRING,(void*)&name[1],LUXI_CLASS_STRING,(void*)&name[2],LUXI_CLASS_STRING,(void*)&name[3],LUXI_CLASS_STRING,(void*)&name[4],LUXI_CLASS_STRING,(void*)&name[5]))))
    return FunctionPublish_returnError(pstate,"6 strings or 1 cubemap texture required");

  skybox = SkyBox_new(id,name[0],name[1],name[2],name[3],name[4],name[5]);
  if (skybox == NULL)
    return 0;

  Reference_makeVolatile(skybox->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_SKYBOX,REF2VOID(skybox->reference));
}

static int PubSkyBox_free(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  SkyBox_t *sky;

  if (n!=1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SKYBOX,(void*)&ref) || !Reference_get(ref,sky))
    return FunctionPublish_returnError(pstate,"1 skybox required");

  Reference_free(ref);//RSkyBox_free(ref);
  return 0;
}

static int PubSkyBox_fov(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  SkyBox_t *skybox;

  if ((n!=1 && n!=2) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SKYBOX,(void*)&ref) || !Reference_get(ref,skybox))
    return FunctionPublish_returnError(pstate,"1 skybox required, optional 1 float ");

  if (n == 1) {
    return FunctionPublish_returnFloat(pstate,skybox->fovfactor);
  }
  else{
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&skybox->fovfactor))
      return FunctionPublish_returnError(pstate,"1 skybox required, optional 1 float ");
  }
  return 0;
}
static int PubSkyBox_globalrot(PState pstate,PubFunction_t *f, int n)
{
  if (n==0) return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(g_VID.drawsetup.skyrot));
  else if (n<4 || 4!=FunctionPublish_getArg(pstate,4,FNPUB_TOVECTOR4(g_VID.drawsetup.skyrot)))
    return FunctionPublish_returnError(pstate,"4 floats required");

  lxMatrix44FromAngleAxisFast(g_VID.drawsetup.skyrotMatrix,LUX_DEG2RAD(g_VID.drawsetup.skyrot[0]),&g_VID.drawsetup.skyrot[1]);
  return 0;
}

void PubClass_SkyBox_init()
{
  FunctionPublish_initClass(LUXI_CLASS_SKYBOX,"skybox",
    "6 2d images make a cube and can be set as background of a scene.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_SKYBOX,LUXI_CLASS_L3D_LIST);

  //FunctionPublish_addFunction(LUXI_CLASS_SKYBOX,PubSkyBox_freeall,NULL,"deleteall",
  //  "():() - deletes all existing skyboxes");
  FunctionPublish_addFunction(LUXI_CLASS_SKYBOX,PubSkyBox_new,NULL,"new",
    "(skybox):(texture cubemap / string north,south,west,east,top,bottom) - creates a new skybox from given texture filenames or uses the given texture (must be cubemap)");
  FunctionPublish_addFunction(LUXI_CLASS_SKYBOX,PubSkyBox_free,NULL,"delete",
    "():(skybox) - deletes skybox");
  FunctionPublish_addFunction(LUXI_CLASS_SKYBOX,PubSkyBox_globalrot,NULL,"worldrotation",
    "([float angle,x,y,z]):([float angle,x,y,z]) - sets world angle/axis for skybox cubemap rotations. Also used in shaders when skyreflectmap texgen is used.");
  FunctionPublish_addFunction(LUXI_CLASS_SKYBOX,PubSkyBox_fov,NULL,"fovfactor",
    "(float):(skybox,[float size]) - returns or sets fovfactor, renderfov = camera.fov * fovfactor");

}
