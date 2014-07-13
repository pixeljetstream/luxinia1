// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/reflib.h"
#include "../render/gl_list3d.h"
#include "../common/3dmath.h"


// Published here:
// LUXI_CLASS_L3D_LIGHT


static int PubLight_new (PState pstate,PubFunction_t *fn, int n)
{
  List3DNode_t *l3d;
  char *name;
  int layer = List3D_getDefaultLayer();

  if (n<1 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_STRING,(void*)&name, LUXI_CLASS_L3D_LAYERID, (void*)&layer)<1 )
    return FunctionPublish_returnError(pstate,"1 string 1 l3dlayerid required");

  l3d = List3DNode_newLight(name,layer);

  Reference_makeVolatile(l3d->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_LIGHT,REF2VOID(l3d->reference));
}
static int PubLight_makesun (PState pstate,PubFunction_t *fn, int n)
{
  List3DNode_t *l3d;
  Reference ref;

  if (n!=1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_LIGHT,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 light required");

  Light_activateSun(l3d->light);

  return 0;
}
static int PubLight_fx (PState pstate,PubFunction_t *fn, int n)
{
  List3DNode_t *l3d;
  Light_t *light;
  Reference ref;
  int prio;
  float range;
  int dur;

  if ((n!=1 && n!=4) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_LIGHT,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 light required");

  light = l3d->light;

  switch((int)fn->upvalue) {
  case 0:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&prio)  ||
      !FunctionPublish_getNArg(pstate,2,LUXI_CLASS_FLOAT,(void*)&range) ||
      !FunctionPublish_getNArg(pstate,3,LUXI_CLASS_INT,(void*)&dur))
      return FunctionPublish_returnError(pstate,"1 light 1 int 1 float 1 int required");

    List3D_activateLight(l3d->setID,light,prio,range,dur);
    break;
  case 1:
    List3D_deactivateLight(light);
    break;
  default:
    break;
  }

  return 0;
}

enum PubLightFuncs_e{
  PLIGHT_ATT0,
  PLIGHT_ATT1,
  PLIGHT_ATT2,
  PLIGHT_RANGEATT,
  PLIGHT_NONSUN,
  PLIGHT_COLORFUNCS,
  PLIGHT_AMBIENT,
  PLIGHT_DIFFUSE,
};


static int PubLight_attributes (PState pstate,PubFunction_t *fn, int n)
{
  List3DNode_t *l3d;
  Light_t *light;
  Reference ref;
  float value;
  lxVector4 color;
  int attr;

  if ((n<1) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_LIGHT,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 light required");

  light = l3d->light;
  attr = (int)fn->upvalue;
  color[0]=0;

  if (attr < PLIGHT_COLORFUNCS){
    if ((n==2) && !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&value))
      return FunctionPublish_returnError(pstate,"1 light required, optional 1 float or 1 boolean");
  }
  else{
    if ((n > 3) && (FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(color))<3))
      return FunctionPublish_returnError(pstate,"1 light required, optional 3/4 float");
  }


  switch(attr) {
  case PLIGHT_ATT0:
    if (n==2) light->attenuate[0] = LUX_MAX(value,0);
    else    return FunctionPublish_returnFloat(pstate,light->attenuate[0]);
    break;
  case PLIGHT_ATT1:
    if (n==2) light->attenuate[1] = LUX_MAX(value,0);
    else    return FunctionPublish_returnFloat(pstate,light->attenuate[1]);
    break;
  case PLIGHT_ATT2:
    if (n==2) light->attenuate[2]  = LUX_MAX(value,0);
    else    return FunctionPublish_returnFloat(pstate,light->attenuate[2]);
    break;
  case PLIGHT_RANGEATT:
    if (n==2) light->rangeAtt  = (value) ? LUX_TRUE : LUX_FALSE;
    else    return FunctionPublish_returnBool(pstate,light->rangeAtt);
    break;
  case PLIGHT_NONSUN:
    if (n==2) light->nonsunlit  = (value) ? LUX_TRUE : LUX_FALSE;
    else    return FunctionPublish_returnBool(pstate,light->nonsunlit);
    break;
  case PLIGHT_AMBIENT:
    if (n > 3)  lxVector4Copy(light->ambient,color);
    else    return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(light->ambient));
    break;
  case PLIGHT_DIFFUSE:
    if (n > 3)  lxVector4Copy(light->diffuse,color);
    else    return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(light->diffuse));
    break;
  default:
    break;
  }

  return 0;
}

void PubClass_Light_init()
{
  FunctionPublish_initClass(LUXI_CLASS_L3D_LIGHT,"l3dlight",
    "Lights illuminate models that have the lit renderflag set. \
There is one sunlight either global default or l3dset specific, just like cameras.\
Additionally there can be multiple FX lights in the world. All lights are pointlights without shadowing.\
The FX lights should have a smaller effective range, so that the engine can pick closest light to a node, because\
the number of lights at the same time is limited to 4 (1 sun, 3 fxlights).<br> Priority of FX lights will make no influence yet.<br>\
Lighting is done in hardware and uses following attenation formula:<br>\
1/(const+linear*distance+quadratic*distance*distance).<br>\
All lights' intensities are summed up and then multiplied with the vertexcolor."
    ,NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_LIGHT,LUXI_CLASS_L3D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIGHT,PubLight_new,NULL,"new",
    "([l3dlight]):(string name,l3dlayerid layer) - creates new light");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIGHT,PubLight_makesun,NULL,"makesun",
    "():(l3dlight) - makes it the default sun light");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIGHT,PubLight_fx,(void*)0,"activate",
    "():(l3dlight,int priority,float range,int duration) - adds light to FX light list, if duration is passed it will get auto deactivated. Priority is unused for now");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIGHT,PubLight_fx,(void*)1,"deactivate",
    "():(l3dlight) - removes light from FX light list");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIGHT,PubLight_attributes,(void*)PLIGHT_ATT0,"attenuateconst",
    "([float]):(l3dlight,[float]) - returns or sets constant attenuation");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIGHT,PubLight_attributes,(void*)PLIGHT_ATT1,"attenuatelinear",
    "([float]):(l3dlight,[float]) - returns or sets linear attenuation");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIGHT,PubLight_attributes,(void*)PLIGHT_ATT2,"attenuatequadratic",
    "([float]):(l3dlight,[float]) - returns or sets quadratic attenuation");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIGHT,PubLight_attributes,(void*)PLIGHT_RANGEATT,"rangeattenuate",
    "([boolean]):(l3dlight,[boolean]) - returns or sets state of automatic range based attenuation");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIGHT,PubLight_attributes,(void*)PLIGHT_AMBIENT,"ambient",
    "([float r,g,b,a]):(l3dlight,[float r,g,b,[a]]) - returns or sets ambient intensity. Intensities can be negative and exceed 0-1. This way you can creater steeper falloffs. ");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIGHT,PubLight_attributes,(void*)PLIGHT_DIFFUSE,"diffuse",
    "([float r,g,b,a]):(l3dlight,[float r,g,b,[a]]) - returns or sets diffuse intensity. Intensities can be negative and exceed 0-1. This way you can creater steeper falloffs.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIGHT,PubLight_attributes,(void*)PLIGHT_NONSUN,"fxnonsunlit",
    "([boolean]):(l3dlight,[boolean]) - returns or sets if the fxlight should affect nodes that are not sunlit (default is true). Can be useful to split lights into 2 categories: one affect all nodes (static scenery & moveable), and the other just fully dynamic lit nodes (moveable nodes). Be aware that l3dnodes will pick the lights per full node, not on a per mesh basis, if the first drawnode is just fxlit, we will use the nonsun lights.");
}
