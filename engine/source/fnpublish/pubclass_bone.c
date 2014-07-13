// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../render/gl_list3d.h"
#include "../common/reflib.h"
#include "../scene/actorlist.h"
#include "../scene/scenetree.h"


// Published here:
// LUXI_CLASS_BONECONTROL
// LUXI_CLASS_BONEID


//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_BONECONTROL
static int PubBoneControl_new(PState state, PubFunction_t *f, int n)
{
  int modelindex;
  int boneid;
  BoneControl_t *bctrl;

  if (n<1 || !FunctionPublish_getNArg(state,0,LUXI_CLASS_BONEID,(void*)&boneid) )
    return FunctionPublish_returnError(state,"1 boneid required");

  modelindex = SUBRESID_GETRES(boneid);
  SUBRESID_MAKEPURE(boneid);

  bctrl = BoneControl_new(ResData_getModelBone(modelindex,boneid));
  if (!bctrl) return 0;

  bctrl->type = (int)f->upvalue;

  Reference_makeVolatile(bctrl->reference);
  return FunctionPublish_returnType(state,LUXI_CLASS_BONECONTROL,REF2VOID(bctrl->reference));
}

static int PubBoneControl_del(PState state, PubFunction_t *f, int n)
{
  Reference ref;
  BoneControl_t *bc;
  if (n!=1 || !FunctionPublish_getNArg(state,0,LUXI_CLASS_BONECONTROL,(void*)&ref) ||
    !Reference_get(ref,bc))
    return FunctionPublish_returnError(state,"1 bonecontrol required");
  RBoneControl_free(ref);
  return 0;
}

static int PubBoneControl_activate(PState state, PubFunction_t *f, int n)
{
  Reference ref;
  Reference refl3d;
  List3DNode_t *l3d;
  BoneControl_t *bc;
  if (n!=2 || FunctionPublish_getArg(state,2,LUXI_CLASS_BONECONTROL,(void*)&ref,LUXI_CLASS_L3D_MODEL,(void*)&refl3d)!=2
    || !Reference_get(ref,bc) || !Reference_get(refl3d,l3d))
    return FunctionPublish_returnError(state,"1 bonecontrol 1 l3dmodel required");

  if (!l3d->minst->boneupd)
    return FunctionPublish_returnError(state,"l3dmodel does not allow boneanimtion");

  BoneControl_activate(bc,l3d->minst->boneupd);
  return 0;
}

static int PubBoneControl_deactivate(PState state, PubFunction_t *f, int n)
{
  Reference ref;
  BoneControl_t *bc;
  if (n!=1 || !FunctionPublish_getNArg(state,0,LUXI_CLASS_BONECONTROL,(void*)&ref) || !Reference_get(ref,bc))
    return FunctionPublish_returnError(state,"1 bonecontrol required");
  BoneControl_deactivate(bc);
  return 0;
}

enum PubBControlCmd_e{
  PBCTRL_NEED_ARG_FUNCS,

  PBCTRL_AXIS,
  PBCTRL_INVERT,
  PBCTRL_TARGET,
  PBCTRL_IKDEPTH,
  PBCTRL_IKITER,
  PBCTRL_IKTHRESH,
  PBCTRL_IKLOOKAT,
  PBCTRL_IKCHILDREN,
};

static int PubBoneControl_attribute(PState pstate, PubFunction_t *f, int n)
{
  Reference ref;
  int intvalue;
  float fltvalue;
  BoneControl_t *bctrl;
  Reference refact;
  Reference refs3d;

  byte state;

  if ((int)f->upvalue > PBCTRL_NEED_ARG_FUNCS && ((n<1) || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BONECONTROL,(void*)&ref)
    || !Reference_get(ref,bctrl))))
    return FunctionPublish_returnError(pstate,"1 bonecontrol required");

  switch((int)f->upvalue){
  case PBCTRL_AXIS:
    if (n==1) return FunctionPublish_returnInt(pstate,bctrl->axis);
    else{
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&intvalue))
        return FunctionPublish_returnError(pstate,"1 bonecontrol 1 int required");
      bctrl->axis = intvalue;
    }
    break;
  case PBCTRL_IKDEPTH:
    if (n==1) return FunctionPublish_returnInt(pstate,bctrl->ikDepth );
    else{
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&intvalue) || intvalue >= BONE_MAX_IK_DEPTH)
        return FunctionPublish_returnError(pstate,"1 bonecontrol 1 int required");
      bctrl->ikDepth = intvalue;
    }
    break;
  case PBCTRL_IKITER:
    if (n==1) return FunctionPublish_returnInt(pstate,bctrl->ikIterations );
    else{
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&intvalue))
        return FunctionPublish_returnError(pstate,"1 bonecontrol 1 int required");
      bctrl->ikIterations = intvalue;
    }
    break;
  case PBCTRL_IKTHRESH:
    if (n==1) return FunctionPublish_returnFloat(pstate,bctrl->ikThresh );
    else{
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&fltvalue) || fltvalue < 0)
        return FunctionPublish_returnError(pstate,"1 bonecontrol 1 float required");
      bctrl->ikThresh = fltvalue;
    }
    break;
  case PBCTRL_IKCHILDREN:
    if (n==1) return FunctionPublish_returnBool(pstate,bctrl->ikAllChildren);
    else{
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
        return FunctionPublish_returnError(pstate,"1 bonecontrol 1 boolean required");
      bctrl->ikAllChildren = state;
    }
    break;
  case PBCTRL_IKLOOKAT:
    if (n==1) return FunctionPublish_returnBool(pstate,bctrl->ikLookAt);
    else{
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
        return FunctionPublish_returnError(pstate,"1 bonecontrol 1 boolean required");
      bctrl->ikLookAt = state;
    }
    break;
  case PBCTRL_INVERT: // invert
    if (n==1) return FunctionPublish_returnBool(pstate,bctrl->invAxis);
    else{
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
        return FunctionPublish_returnError(pstate,"1 bonecontrol 1 boolean required");
      bctrl->invAxis = state;
    }
    break;
  case PBCTRL_TARGET: // target
    if (n==1){  BoneControl_setMatrixIF(bctrl,NULL); }
    else{
      void *ret;
      Reference_reset(refact);
      Reference_reset(refs3d);
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SCENENODE,(void*)&refs3d) && !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_ACTORNODE,(void*)&refact))
        return FunctionPublish_returnError(pstate,"1 bonecontrol 1 actornode/scenenode required");
      if (Reference_get(refact,ret)){
        BoneControl_setMatrixIF(bctrl,ActorNode_getMatrixIF((ActorNode_t*)ret));
      }
      else if (Reference_get(refs3d,ret)){
        BoneControl_setMatrixIF(bctrl,SceneNode_getMatrixIF((SceneNode_t*)ret));
      }
    }
    break;
  default:
    break;
  }
  return 0;
}

static int PubBoneControl_pos (PState state,PubFunction_t *f, int n)
{
  Reference ref;
  BoneControl_t *bctrl;

  if ((n!=1 && n!=4) ||
    !FunctionPublish_getNArg(state,0,LUXI_CLASS_BONECONTROL,(void*)&ref)    ||
    !Reference_get(ref,bctrl))
    return FunctionPublish_returnError(state,"1 bonecontrol [3 floats] required");

  switch(n) {
  case 1:
    return FunctionPublish_setRet(state,3,FNPUB_FROMMATRIXPOS(bctrl->matrix));
  case 4:
    if (FunctionPublish_getArgOffset(state,1,3,FNPUB_TOMATRIXPOS(bctrl->matrix)) != 3)
      return FunctionPublish_returnError(state,"3 numbers required");
    return 0;
  }
  return 0;
}

static int PubBoneControl_rot (PState state,PubFunction_t *f, int n)
{
  Reference ref;
  BoneControl_t *bctrl;
  lxVector3 vector;
  float *x,*y,*z;

  if ((n<1) ||
    !FunctionPublish_getNArg(state,0,LUXI_CLASS_BONECONTROL,(void*)&ref)||
    !Reference_get(ref,bctrl))
    return FunctionPublish_returnError(state,"1 bonecontrol [3 floats] required");

  switch((int)f->upvalue) {
  case ROT_AXIS:
    if (n<2){
      x = y = z = bctrl->matrix;
      y+=4;
      z+=8;
      return FunctionPublish_setRet(state,9, FNPUB_FROMVECTOR3(x)
        ,FNPUB_FROMVECTOR3(y)
        ,FNPUB_FROMVECTOR3(z));
    }
    else{
      x = y = z = bctrl->matrix;
      y+=4;
      z+=8;
      if (FunctionPublish_getArgOffset(state,1,9,FNPUB_TOVECTOR3(x),
        FNPUB_TOVECTOR3(y),
        FNPUB_TOVECTOR3(z)) != 9)
        return FunctionPublish_returnError(state,"9 floats required");
      return 0;
    }
    break;
  default:
  case ROT_RAD:
    if (n<2){
      lxMatrix44ToEulerZYX(bctrl->matrix,vector);
      return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(state,1,3,FNPUB_TOVECTOR3(vector)) != 3)
        return FunctionPublish_returnError(state,"3 numbers required");
      lxMatrix44FromEulerZYXFast(bctrl->matrix,vector);
      return 0;
    }
    break;
  case ROT_DEG:
    if (n<2){
      lxMatrix44ToEulerZYX(bctrl->matrix,vector);
      lxVector3Rad2Deg(vector,vector);
      return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(state,1,3,FNPUB_TOVECTOR3(vector)) != 3)
        return FunctionPublish_returnError(state,"3 numbers required");
      lxMatrix44FromEulerZYXdeg(bctrl->matrix,vector);
      return 0;
    }
    break;

  }
  return 0;
}

static int PubBoneControl_matrix(PState state, PubFunction_t *f, int n)
{
  Reference ref;
  float *mat;
  BoneControl_t *bctrl;

  if (n< 1 || (!FunctionPublish_getNArg(state,0,LUXI_CLASS_L3D_NODE,(void*)&ref) || !Reference_get(ref,bctrl)))
    return FunctionPublish_returnError(state,"1 bonecontrol required");

  if (n==1)
    return PubMatrix4x4_return (state,bctrl->matrix);
  else if (!FunctionPublish_getNArg(state,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,mat))
    return FunctionPublish_returnError(state,"1 matrix4x4 required");
  lxMatrix44CopySIMD(bctrl->matrix,mat);

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_BONEID

enum PubBoneCmds_e{
  PBONE_ENABLEAXIS,
  PBONE_DISABLEAXIS,
  PBONE_PARENT,
  PBONE_NAME,
};

static int PubBone_cmd(PState state,PubFunction_t *f, int n)
{
  int modelindex;
  int boneindex;
  Bone_t *bone;
  int axis;
  float minangle,maxangle;
  ResourceChunk_t *oldchunk;

  bool8 limit;

  if (n < 1 || !FunctionPublish_getNArg(state,0,LUXI_CLASS_BONEID,(void*)&boneindex))
    return FunctionPublish_returnError(state,"1 boneid required");

  modelindex = SUBRESID_GETRES(boneindex);
  SUBRESID_MAKEPURE(boneindex);

  switch((int)f->upvalue)
  {
  case PBONE_ENABLEAXIS:
    if (n<5 || FunctionPublish_getArgOffset(state,1,4,
      LUXI_CLASS_INT,(void*)&axis,LUXI_CLASS_BOOLEAN,(void*)&limit,LUXI_CLASS_FLOAT,(void*)&minangle,LUXI_CLASS_FLOAT,(void*)&maxangle)<4)
      return FunctionPublish_returnError(state,"1 boneid 1 int 1 boolean 2 floats required");
    // calls reszalloc so we need to protect it
    oldchunk = ResData_setChunkFromPtr((ResourceInfo_t*)ResData_getModel(modelindex));
    Bone_enableAxis(ResData_getModelBone(modelindex,boneindex),axis,limit,minangle,maxangle);
    ResourceChunk_activate(oldchunk);
    break;
  case PBONE_DISABLEAXIS:
    if (n<2 || !FunctionPublish_getNArg(state,1,LUXI_CLASS_INT,(void*)&axis) )
      return FunctionPublish_returnError(state,"1 boneid 1 int required");
    Bone_disableAxis(ResData_getModelBone(modelindex,boneindex),axis);
    break;
  case PBONE_PARENT:
    bone = ResData_getModelBone(modelindex,boneindex);
    if (!bone->parentB)
      return 0;
    boneindex = bone->parentID/16;
    SUBRESID_MAKE(boneindex,modelindex);
    return FunctionPublish_returnType(state,LUXI_CLASS_BONEID,(void*)boneindex);
  case PBONE_NAME:
    return FunctionPublish_returnString(state,ResData_getModelBone(modelindex,boneindex)->name);

  default:
    break;
  }

  return 0;
}


static int PubBone_bonePos(PState state,PubFunction_t *f, int n)
{
  int modelindex;
  int boneindex;
  float *boneabs;
  booln user = LUX_FALSE;

  if (n<1 || 1>FunctionPublish_getArg(state,2,LUXI_CLASS_BONEID,(void*)&boneindex,LUXI_CLASS_BOOLEAN,(void*)&user))
    return FunctionPublish_returnError(state,"1 boneid required");

  modelindex = SUBRESID_GETRES(boneindex);
  SUBRESID_MAKEPURE(boneindex);
  boneabs = ResData_getModelBoneAbs(modelindex,boneindex,user);
  if (!boneabs)
    return 0;

  return FunctionPublish_setRet(state,3,FNPUB_FROMMATRIXPOS(boneabs));
}

static int PubBone_matrix(PState state,PubFunction_t *f, int n)
{
  int modelindex;
  int boneindex;
  float *boneabs;
  booln user = LUX_FALSE;

  if (n<1 || 1>FunctionPublish_getArg(state,2,LUXI_CLASS_BONEID,(void*)&boneindex,LUXI_CLASS_BOOLEAN,(void*)&user))
    return FunctionPublish_returnError(state,"1 boneid required");

  modelindex = SUBRESID_GETRES(boneindex);
  SUBRESID_MAKEPURE(boneindex);
  boneabs = ResData_getModelBoneAbs(modelindex,boneindex,user);
  if (!boneabs)
    return 0;

  return PubMatrix4x4_return(state,boneabs);
}



static int PubBone_boneRot(PState state,PubFunction_t *f, int n)
{
  int modelindex;
  int boneindex;
  float *boneabs;
  Bone_t *bone;
  lxVector3 vector;
  float *x,*y,*z;
  booln user = LUX_FALSE;

  if (n<1 || (1>FunctionPublish_getArg(state,2,LUXI_CLASS_BONEID,(void*)&boneindex,LUXI_CLASS_BOOLEAN,(void*)&user)))
    return FunctionPublish_returnError(state,"1 boneid required");

  modelindex = SUBRESID_GETRES(boneindex);
  SUBRESID_MAKEPURE(boneindex);
  boneabs = ResData_getModelBoneAbs(modelindex,boneindex,user);
  bone = ResData_getModelBone(modelindex,boneindex);
  if (!boneabs)
    return 0;

  switch((int)f->upvalue)
  {
  case ROT_RAD:
    lxMatrix44ToEulerZYX(boneabs,vector);
    return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(vector));
  case ROT_DEG:
    lxMatrix44ToEulerZYX(boneabs,vector);
    lxVector3Rad2Deg(vector,vector);
    return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(vector));
  case ROT_AXIS:
    x = boneabs;
    y = boneabs+4;
    z = boneabs+8;
    return FunctionPublish_setRet(state,9,FNPUB_FROMVECTOR3(x),FNPUB_FROMVECTOR3(y),FNPUB_FROMVECTOR3(z));
  default:
    break;
  }
  return 0;
}

static int PubBone_new(PState state,PubFunction_t *f,int n)
{
  int boneid;
  int parentid;
  int modelid;
  Model_t*model;
  char *name;
  Reference ref;
  float *matrix;

  if (n<1 || !FunctionPublish_getNArg(state,0,LUXI_CLASS_BONEID,(void*)&boneid))
    return FunctionPublish_returnError(state,"1 boneid required");
  modelid = SUBRESID_GETRES(boneid);
  SUBRESID_MAKEPURE(boneid);
  if ((model = ResData_getModel(modelid))==NULL)
    return FunctionPublish_returnError(state,"model is invalid");

  parentid = -1;
  if (FunctionPublish_getArgOffset(state,1,3,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_MATRIX44,(void*)&ref,LUXI_CLASS_BONEID,(void*)&parentid)<2
    || !Reference_get(ref,matrix))
    return FunctionPublish_returnError(state,"1 string 1 matrix4x4 [1 boneid] required");


  if (parentid >= 0){
    if (SUBRESID_GETRES(parentid)!= modelid)
      return FunctionPublish_returnError(state,"parent boneid doesnt match model");
    SUBRESID_MAKEPURE(parentid);
  }
  parentid++;

  Model_setBone(model,boneid,name,matrix,parentid);

  return 0;
}


void PubClass_Bone_init()
{
  FunctionPublish_initClass(LUXI_CLASS_BONEID,"boneid","Bone within a model. Normally reference state is returned but its also possible to retrieve animcache values.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_BONEID,PubBone_cmd,(void*)PBONE_ENABLEAXIS,"enableaxis",
    "():(boneid,int axis,boolean limit,float minangle,float maxangle) - enables a bone's rotation axis when its controlled by bonecontrol and allows limited rotation. limit is relative to reference position. ");
  FunctionPublish_addFunction(LUXI_CLASS_BONEID,PubBone_cmd,(void*)PBONE_DISABLEAXIS,"disableaxis",
    "():(boneid,int axis) - disable bone's rotation axis");
  FunctionPublish_addFunction(LUXI_CLASS_BONEID,PubBone_cmd,(void*)PBONE_PARENT,"parent",
    "([boneid]):(boneid) - returns parent if exists");
  FunctionPublish_addFunction(LUXI_CLASS_BONEID,PubBone_cmd,(void*)PBONE_PARENT,"name",
    "(string):(boneid) - returns name");
  FunctionPublish_addFunction(LUXI_CLASS_BONEID,PubBone_bonePos,NULL,"pos",
    "(float x,y,z):(boneid,[boolean animcache]) - returns bone's reference position in object space");
  FunctionPublish_addFunction(LUXI_CLASS_BONEID,PubBone_boneRot,(void*)ROT_RAD,"rotrad",
    "(float x,y,z):(boneid,[boolean animcache]) - returns bone's reference rotation in object space. radians");
  FunctionPublish_addFunction(LUXI_CLASS_BONEID,PubBone_boneRot,(void*)ROT_DEG,"rotdeg",
    "(float x,y,z):(boneid,[boolean animcache]) - returns bone's reference rotation in object space. degrees");
  FunctionPublish_addFunction(LUXI_CLASS_BONEID,PubBone_boneRot,(void*)ROT_AXIS,"rotaxis",
    "(float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz):(boneid,[boolean animcache]) - returns bone's reference rotation in object space. axis");
  FunctionPublish_addFunction(LUXI_CLASS_BONEID,PubBone_matrix,NULL,"matrix",
    "(matrix4x4):(boneid,[boolean animcache]) - returns bone's reference matrix in object space.");
  FunctionPublish_addFunction(LUXI_CLASS_BONEID,PubBone_new,NULL,"init",
    "():(boneid,string name,matrix4x4 refmatrix,[boneid parent]) - sets bonedata, only works on empty bones in model.create generated models. matrix should not have scaling.");


  FunctionPublish_initClass(LUXI_CLASS_BONECONTROL,"bonecontrol",
    "Allows manual control of bones in bonesystems found in l3dmodels."
    ,NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_BONECONTROL,LUXI_CLASS_L3D_LIST);

  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_new,(void*)BONE_CONTROL_RELATIVE,"newrelative",
    "(bonecontrol):(boneid) - a new relative bonecontroller, use pos/rot to manually rotate or move a bone");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_new,(void*)BONE_CONTROL_IK,"newik",
    "(bonecontrol):(boneid) - a new ik bonecontroller. The bone will try to reach the target location. The bone must have one parent.");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_new,(void*)BONE_CONTROL_ABS,"newabs",
    "(bonecontrol):(boneid) - a new abs bonecontroller. The bone will takeover the target matrix or local matrix (pos/rot)");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_new,(void*)BONE_CONTROL_LOOKAT,"newlookat",
    "(bonecontrol):(boneid) - a new lookat bonecontroller. The bone will orient the defined axis in such a way that it aims at the target");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_del,NULL,"delete",
    "():(bonecontrol) - deletes the controller");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_activate,NULL,"activate",
    "():(bonecontrol,l3dmodel) - activates bonecontrol on given l3dmodel");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_deactivate,NULL,"deactivate",
    "():(bonecontrol) - deactivates bonecontrol");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_matrix,NULL,"matrix",
    "(matrix4x4):(bonecontrol,[matrix4x4]) - returns or sets matrix");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_pos,NULL,"pos",
    "(float x,y,z):(bonecontrol,[float x,y,z]) - returns or sets position");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_rot,(void*)ROT_DEG,"rotdeg",
    "(float x,y,z):(bonecontrol,[float x,y,z]) - returns or sets rotation in degrees");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_rot,(void*)ROT_RAD,"rotrad",
    "(float x,y,z):(bonecontrol,[float x,y,z]) - returns or sets rotation in radians");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_rot,(void*)ROT_AXIS,"rotaxis",
    "([float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]):(bonecontrol,[float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]) - returns or sets rotation axis, make sure they make a orthogonal basis.");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_attribute,(void*)PBCTRL_AXIS,"axis",
    "([int]):(bonecontrol,[int]) - returns or sets axis along we aim");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_attribute,(void*)PBCTRL_IKDEPTH,"ikdepth",
    "([int]):(bonecontrol,[int]) - returns or sets depth of IK chain. means how many steps up in hierarchy we go. number must be smaller than the parent chain this bone has.");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_attribute,(void*)PBCTRL_IKITER,"ikiterations",
    "([int]):(bonecontrol,[int]) - returns or sets iterations of IK chain. how many iterations of the full IK chain we will do if we are above threshold");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_attribute,(void*)PBCTRL_IKTHRESH,"ikthreshold",
    "([float]):(bonecontrol,[float]) - returns or sets threshold of IK chain. if square distance is below threshold we stop");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_attribute,(void*)PBCTRL_IKLOOKAT,"iklookat",
    "([boolean]):(bonecontrol,[boolean]) - returns or sets if a lookat effect should be applied on the ik root as well (default is false)");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_attribute,(void*)PBCTRL_IKCHILDREN,"ikallchildren",
    "([boolean]):(bonecontrol,[boolean]) - returns or sets if all affected children should be updated. By default (false) we only update the bone's parents.");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_attribute,(void*)PBCTRL_INVERT,"invertaxis",
    "([boolean]):(bonecontrol,[boolean]) - returns or sets if aim axis should be inverted");
  FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_attribute,(void*)PBCTRL_TARGET,"target",
    "():(bonecontrol,[actnornode]/[scenenode]) - sets target for ik/abs/lookat, for abs it will overwrite the matrix. If no argument is passed then it will be unlinked.");
  //FunctionPublish_addFunction(LUXI_CLASS_BONECONTROL,PubBoneControl_attribute,(void*)PBCTRL_DELETALL,"deleteall",
  //  "():() - deletes all bonecontrollers (active or not)");
}
