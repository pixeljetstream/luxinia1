// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../render/gl_list3d.h"
#include "../render/gl_list2d.h"
#include "../main/main.h"
// Published here:
// LUXI_CLASS_MATCONTROLID
// LUXI_CLASS_MATTEXCONTROLID
// LUXI_CLASS_MATSHDCONTROLID
// LUXI_CLASS_MATOBJECT
// LUXI_CLASS_MATERIAL
// LUXI_CLASS_MATAUTOCONTROL
// LUXI_CLASS_MATSURF

extern int PubMatrix4x4_return (PState pstate,float *m);

static int PubMaterial_load (PState pstate,PubFunction_t *fn, int n)
{
  char *path;
  char *cgstring = NULL;
  int index;

  if (n<1 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_STRING,&path,LUXI_CLASS_STRING,&cgstring)<1)
    return FunctionPublish_returnError(pstate,"1 string [1 string] required");
  Main_startTask(LUX_TASK_RESOURCE);
  index = ResData_addMaterial(path,cgstring);
  Main_endTask();
  if (index==-1)
    return 0;

  return FunctionPublish_returnType(pstate,LUXI_CLASS_MATERIAL,(void*)index);
}

enum PubMaterialCmd_e{
  PMTL_SHADER,
  PMTL_NAME,
  PMTL_TEXCONTROL,
  PMTL_SHDCONTROL,
  PMTL_CONTROL,
  PMTL_ALPHATEX,
};

static int PubMaterial_cmd (PState pstate,PubFunction_t *fn, int n)
{
  Shader_t *shader;
  Material_t *mtl;
  char *name;
  int index;
  int sid;
  float refvalue;
  MaterialControlType_t type;
  lxClassType outclass;

  if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATERIAL,(void*)&index))
    return FunctionPublish_returnError(pstate,"1 material required");

  mtl = ResData_getMaterial(index);

  sid = 0;
  switch((int)fn->upvalue)
  {
  case PMTL_NAME:
    return FunctionPublish_returnString(pstate,mtl->resinfo.name);
  case PMTL_SHDCONTROL:
  case PMTL_TEXCONTROL:
  case PMTL_CONTROL:
    if (n!=2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&name))
      return FunctionPublish_returnError(pstate,"1 material 1 string required");


    switch((int)fn->upvalue)
    {
    case PMTL_SHDCONTROL:
      type = MATERIAL_CONTROL_RESSHD;
      outclass = LUXI_CLASS_MATSHDCONTROLID;
      break;
    case PMTL_TEXCONTROL:
      type = MATERIAL_CONTROL_RESTEX;
      outclass = LUXI_CLASS_MATTEXCONTROLID;
      break;
    case PMTL_CONTROL:
      type = MATERIAL_CONTROL_FLOAT;
      outclass = LUXI_CLASS_MATCONTROLID;
      break;
    }
    sid = Material_getControlIndex(mtl,name,type);

    if (sid != -1){
      SUBRESID_MAKE(sid,index);
      return FunctionPublish_returnType(pstate,outclass,(void*)sid);
    }
    break;
  case PMTL_SHADER:
    sid = 0;
    if (n>1 && !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&sid))
      return FunctionPublish_returnError(pstate,"1 material 1 int required");
    sid %= MATERIAL_MAX_SHADERS;
    return FunctionPublish_returnType(pstate,LUXI_CLASS_SHADER,(void*)mtl->shaders[sid].resRID);
  case PMTL_ALPHATEX:
    sid = 0;
    if (n>1 && !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&sid))
      return FunctionPublish_returnError(pstate,"1 material 1 int required");
    sid %= MATERIAL_MAX_SHADERS;
    shader = ResData_getShader(mtl->shaders[sid].resRID);
    if (shader->alpha && shader->alphatex){
      if (shader->alphatex->id)
        index = Material_getStageRIDTexture(mtl,shader->alphatex->id);
      else
        index = shader->alphatex->srcRID;
      sid = shader->alpha->alphafunc;
      refvalue = (mtl->shaders[sid].rfalpha >= 0) ? mtl->shaders[sid].rfalpha : shader->alpha->alphaval;
      return FunctionPublish_setRet(pstate,4,LUXI_CLASS_TEXTURE,(void*)index,LUXI_CLASS_COMPAREMODE,(void*)sid,FNPUB_FFLOAT(refvalue),LUXI_CLASS_INT,(void*)shader->alphatex->texchannel);
    }
    break;
  default:
    break;
  }


  return 0;

}




enum {
  MOA_SEQOFF = 1,
  MOA_MODOFF,
  MOA_TIME,
  MOA_MODCONTROL,
  MOA_TEXCONTROL,
  MOA_SHDCONTROL,
  MOA_MATRIX,
  MOA_POS,
  MOA_ROTAXIS,
  MOA_ROTRAD,
  MOA_ROTDEG,
  MOA_AUTOCTRL,
  MOA_AUTOSTAGE,
};

static int PubMaterialObject_attributes (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  lxClassType type;
  int ctrlid;
  void *data,*d2;
  int meshid;
  int offset;
  int matRID;
  MaterialObject_t **matobjptr;
  MaterialObject_t *matobj;
  float *fltPtr;
  MaterialAutoControl_t *matautoctrl;
  MaterialAutoControl_t **matautoctrlptr;
  MaterialControl_t *mcontrol;

  byte state;

  if (n<1)
    return FunctionPublish_returnError(pstate,"no argument passed");

  FunctionPublish_getNArg(pstate,0,LUXI_CLASS_ANY,(void*)&data);

  meshid = 0;
  offset = 1;
  switch(type = FunctionPublish_getNArgType(pstate,0)) {
  case LUXI_CLASS_MATERIAL:
    matobjptr = NULL;
    matobj = &ResData_getMaterial((int)data)->matobjdefault;
    matRID = (int)data;
    break;

  case LUXI_CLASS_L3D_MODEL:
    if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MESHID,(void*)&meshid))
      offset++;
  case LUXI_CLASS_L3D_TRAIL:
  case LUXI_CLASS_L3D_PRIMITIVE:
    if (!Reference_get(*(Reference*)&data,d2))
      return FunctionPublish_returnError(pstate,"reference node not valid");
    data = d2;
    if (offset>1){
      if (!SUBRESID_CHECK(meshid,((List3DNode_t*)data)->minst->modelRID))
        return FunctionPublish_returnError(pstate,"illegal meshid");
      SUBRESID_MAKEPURE(meshid);
    }
    matobjptr = &((List3DNode_t*)data)->drawNodes[meshid].draw.matobj;
    matobj = *matobjptr;
    matRID = ((List3DNode_t*)data)->drawNodes[meshid].draw.matRID;
    break;

  case LUXI_CLASS_L2D_IMAGE:
    if (!Reference_get(*(Reference*)&data,d2))
      return FunctionPublish_returnError(pstate,"1 l2dimage required");
    data = d2;
    matobjptr = &((List2DNode_t*)data)->l2dimage->matobj;
    matobj = *matobjptr;
    matRID = ((List2DNode_t*)data)->l2dimage->texid;
    break;

  case LUXI_CLASS_RCMD_DRAWMESH:
    if (!Reference_get(*(Reference*)&data,d2))
      return FunctionPublish_returnError(pstate,"1 rcmdmesh required");
    data = d2;
    matobjptr = &((RenderCmdMesh_t*)data)->dnode.matobj;
    matobj = *matobjptr;
    matRID = ((RenderCmdMesh_t*)data)->dnode.matRID;
    break;
/*
  case LUXI_CLASS_L3D_VIEW:
    if (!Reference_get(*(Reference*)&data,d2))
      return FunctionPublish_returnError(pstate,"1 l3dview required");
    matobjptr = &((List3DView_t*)d2)->background.matobj;
    matobj = *matobjptr;
    matRID = ((List3DView_t*)d2)->background.texRID;
    break;
    */
  case LUXI_CLASS_PARTICLECLOUD:
    data = ResData_getParticleCloud((int)data);
    matobjptr = &((ParticleCloud_t*)data)->container.matobj;
    matobj = *matobjptr;
    matRID = ((ParticleCloud_t*)data)->container.texRID;
    break;
  case LUXI_CLASS_PARTICLESYS:
    data = ResData_getParticleSys((int)data);
    matobjptr = &((ParticleSys_t*)data)->container.matobj;
    matobj = *matobjptr;
    matRID = ((ParticleSys_t*)data)->container.texRID;
    break;
  default:
    return FunctionPublish_returnError(pstate,"material/l3dtrail/l3dmodel,[meshid]/l2dimage/l3dview/particlesys/particlecloud/rcmdmesh required");
  }

  if (matRID < 0 || (n < offset+1 && !matobj))
    return 0;

  if (n > offset && !matobj && matobjptr)
    matobj = *matobjptr = MaterialObject_new(ResData_getMaterial(matRID));

  switch((int)f->upvalue) {
  case MOA_SEQOFF:  // seqoff
    if (n==offset) return FunctionPublish_returnBool(pstate,matobj->materialflag & MATERIAL_SEQOFF);
    else if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 matobject 1 boolean required");
    if (state)
      matobj->materialflag |= MATERIAL_SEQOFF;
    else
      matobj->materialflag &= MATERIAL_SEQOFF;
    break;
  case MOA_MODOFF:  // modoff
    if (n==offset) return FunctionPublish_returnBool(pstate,matobj->materialflag & MATERIAL_MODOFF);
    else if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 matobject 1 boolean required");
    if (state)
      matobj->materialflag |= MATERIAL_MODOFF;
    else
      matobj->materialflag &= MATERIAL_MODOFF;
    break;
  case MOA_TIME:  // time
    if (n==offset) return FunctionPublish_returnInt(pstate,matobj->time);
    else if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_INT,(void*)&matobj->time))
      return FunctionPublish_returnError(pstate,"1 matobject 1 int required");
    break;
  case MOA_TEXCONTROL:
    if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_MATTEXCONTROLID,(void*)&ctrlid) || SUBRESID_GETRES(ctrlid) != matRID)
      return FunctionPublish_returnError(pstate,"1 matobject 1 matching mattexcontrolid required");
    offset++;
    SUBRESID_MAKEPURE(ctrlid);

    if (n==offset) return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)matobj->resRIDs[ctrlid*2]);
    else if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_TEXTURE,(void*)&matobj->resRIDs[ctrlid*2]))
      return FunctionPublish_returnError(pstate,"1 matobject 1 texture required");
    matobj->resRIDs[(ctrlid*2)+1] = ResData_getTexture(matobj->resRIDs[ctrlid*2])->windowsized;
    break;
  case MOA_SHDCONTROL:
    if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_MATSHDCONTROLID,(void*)&ctrlid) || SUBRESID_GETRES(ctrlid) != matRID)
      return FunctionPublish_returnError(pstate,"1 matobject 1 matching matshdcontrolid required");
    offset++;
    SUBRESID_MAKEPURE(ctrlid);

    if (n==offset) return FunctionPublish_returnType(pstate,LUXI_CLASS_SHADER,(void*)matobj->resRIDs[ctrlid*2]);
    else if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_SHADER,(void*)&meshid) || !Material_checkShaderLink(ResData_getMaterial(matRID),ctrlid,ResData_getShader(meshid)))
      return FunctionPublish_returnError(pstate,"1 matobject 1 shader required");

    matobj->resRIDs[ctrlid*2] = meshid;
    matobj->resRIDs[(ctrlid*2)+1] = ResData_getShader(meshid)->numTotalParams;
    break;
  case MOA_MODCONTROL:  // matcontrol
    if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_MATCONTROLID,(void*)&ctrlid))
      return FunctionPublish_returnError(pstate,"1 matobject 1 matcontrolid required");
    offset++;

    if(!MaterialControl_getOffsetsFromID(ctrlid,matRID,&meshid,&matRID))
      return FunctionPublish_returnError(pstate,"matcontrolid or material invalid");
    // meshid  = offset in values
    // matRID = length
    ctrlid = 0; // extraoffset
    if (matRID > 4){
      if(!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_INT,(void*)&ctrlid) ||
        ctrlid*4 >= matRID)
        return FunctionPublish_returnError(pstate,"offset parameter required, or offset too big");

      ctrlid *= 4;
      offset++;
    }

    fltPtr = &matobj->controlValues[meshid+ctrlid];
    if (n==offset) return FunctionPublish_setRet(pstate,LUX_MIN(4,matRID-ctrlid),FNPUB_FROMVECTOR4(fltPtr));
    else if (1>FunctionPublish_getArgOffset(pstate,offset,LUX_MIN(4,matRID-ctrlid),FNPUB_TOVECTOR4(fltPtr)))
      return FunctionPublish_returnError(pstate,"1 matobject 1 matcontrolid 1-4 float required");
    break;
  case MOA_MATRIX:  // texmatrix
    if (n==offset) {
      if (matobj->texmatrix){
        return PubMatrix4x4_return(pstate,matobj->texmatrix);
      }
      else
        return 0;
    }
    else if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_MATRIX44,(void*)&data) ||
      !Reference_get(*(Reference*)&data,d2)){
      matobj->texmatrix = NULL;
      return 0;
    }
    matobj->texmatrix = matobj->texmatrixData;
    data = d2;
    lxMatrix44CopySIMD(matobj->texmatrix,(float*)data);
    break;
  case MOA_POS:
    if (n==offset) {
      if (matobj->texmatrix) {
        return FunctionPublish_setRet(pstate,3,FNPUB_TOMATRIXPOS(matobj->texmatrix));
      }
      return FunctionPublish_setRet(pstate,3,FNPUB_FFLOAT(0),FNPUB_FFLOAT(0),FNPUB_FFLOAT(0));
    }
    {
      float vec[3];
      if (n<4 || FunctionPublish_getArgOffset(pstate,offset,3,FNPUB_TOVECTOR3(vec))<3)
        return FunctionPublish_returnError(pstate,"Requires 3 floats as arguments");
      if (matobj->texmatrix != matobj->texmatrixData) {
        matobj->texmatrix = matobj->texmatrixData;
        lxMatrix44Identity(matobj->texmatrix);
      }
      matobj->texmatrix[12] = vec[0];
      matobj->texmatrix[13] = vec[1];
      matobj->texmatrix[14] = vec[2];
    }
    break;
  case MOA_ROTAXIS:
    if (n==offset) {
      if (matobj->texmatrix) {
        return FunctionPublish_setRet(pstate,9,FNPUB_FROMVECTOR3((&matobj->texmatrix[0])),
          FNPUB_FROMVECTOR3((&matobj->texmatrix[4])),
          FNPUB_FROMVECTOR3((&matobj->texmatrix[8])));
      }
      return FunctionPublish_setRet(pstate,9,FNPUB_FFLOAT(1),FNPUB_FFLOAT(0),FNPUB_FFLOAT(0),
        FNPUB_FFLOAT(0),FNPUB_FFLOAT(1),FNPUB_FFLOAT(0),
        FNPUB_FFLOAT(0),FNPUB_FFLOAT(0),FNPUB_FFLOAT(1));
    }
    {
      float vec[9];
      if (n<10 || FunctionPublish_getArgOffset(pstate,offset,9,FNPUB_TOVECTOR3(vec),
            FNPUB_TOVECTOR3((&vec[3])),FNPUB_TOVECTOR3((&vec[6])))<9)
        return FunctionPublish_returnError(pstate,"Requires 3 floats as arguments");
      if (matobj->texmatrix != matobj->texmatrixData) {
        matobj->texmatrix = matobj->texmatrixData;
        lxMatrix44Identity(matobj->texmatrix);
      }
      matobj->texmatrix[0] = vec[0];
      matobj->texmatrix[1] = vec[1];
      matobj->texmatrix[2] = vec[2];
      matobj->texmatrix[4] = vec[3];
      matobj->texmatrix[5] = vec[4];
      matobj->texmatrix[6] = vec[5];
      matobj->texmatrix[8] = vec[6];
      matobj->texmatrix[9] = vec[7];
      matobj->texmatrix[10] = vec[8];
    }
    break;
  case MOA_AUTOCTRL:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATCONTROLID,(void*)&meshid) || SUBRESID_GETRES(meshid) != matRID)
      return FunctionPublish_returnError(pstate,"1 matcontrolid required");

    SUBRESID_MAKEPURE(meshid);

    matautoctrlptr = &matobj->mautoctrlControls[meshid];
    mcontrol = ResData_getMaterial(matRID)->controls + meshid;

    if (n>2){
      if (FunctionPublish_getNArg(pstate,2,LUXI_CLASS_MATAUTOCONTROL,(void*)&ref) && Reference_get(ref,matautoctrl)){
        if (mcontrol->length != 4 && mcontrol->length != 16)
          return FunctionPublish_returnError(pstate,"matcontrolid must be of length 4 or 16");
        else if (mcontrol->length==4 && matautoctrl->type > MATERIAL_ACTRL_VEC4MAT_SEP)
          return FunctionPublish_returnError(pstate,"vec4 matautocontrol required");
        else if (mcontrol->length==16 && matautoctrl->type < MATERIAL_ACTRL_VEC4MAT_SEP)
          return FunctionPublish_returnError(pstate,"matrix matautocontrol required");

        MaterialObject_setAutoControl(matobj,matautoctrlptr,matautoctrl);
      }
      else{
        MaterialObject_setAutoControl(matobj,matautoctrlptr,NULL);
      }
    }
    else{
      matautoctrl = *matautoctrlptr;
      if (matautoctrl)
        return FunctionPublish_returnType(pstate,LUXI_CLASS_MATAUTOCONTROL,REF2VOID(matautoctrl->reference));
      else
        return 0;
    }
    break;
  case MOA_AUTOSTAGE:
    if (FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_INT,(void*)&meshid,LUXI_CLASS_BOOLEAN,(void*)&offset) < 2)
      return FunctionPublish_returnError(pstate,"1 stagenumber 1 boolean required");
    meshid %= matobj->numStages;
    matautoctrlptr = &matobj->mautoctrlStages[offset+(meshid*2)];

    if (n > 3){
      if (FunctionPublish_getNArg(pstate,3,LUXI_CLASS_MATAUTOCONTROL,(void*)&ref) && Reference_get(ref,matautoctrl)){
        if (matautoctrl->type < MATERIAL_ACTRL_VEC4MAT_SEP)
          return FunctionPublish_returnError(pstate,"matrix matautocontrol required");
        MaterialObject_setAutoControl(matobj,matautoctrlptr,matautoctrl);
      }
      else{
        MaterialObject_setAutoControl(matobj,matautoctrlptr,NULL);
      }
    }
    else{
      matautoctrl = *matautoctrlptr;
      if (matautoctrl)
        return FunctionPublish_returnType(pstate,LUXI_CLASS_MATAUTOCONTROL,REF2VOID(matautoctrl->reference));
      else
        return 0;
    }
    break;
  default:
    break;
  }

  return 0;
}

enum PubMatSurf_e
{
  PMS_VCOLOR,
  PMS_CONTAINS,
};

static int PubMatSurf_funcs(PState pstate,PubFunction_t *f, int n)
{
  switch((int)f->upvalue){
  case PMS_VCOLOR:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_MATSURF,(void*)-1);
  case PMS_CONTAINS:
    {
      Material_t*mat;
      Shader_t *shd;
      int i;
      int resid0,resid1;

      if (n < 2 || 2>FunctionPublish_getArg(pstate,2,LUXI_CLASS_MATSURF,(void*)&resid0,LUXI_CLASS_MATSURF,(void*)&resid1))
        return FunctionPublish_returnError(pstate,"2 matsurfaces required");

      // same
      if (resid0 == resid1)
        return FunctionPublish_returnBool(pstate,LUX_TRUE);
      else if (!vidMaterial(resid0)) // first is no material, ie cannot contain others
        return FunctionPublish_returnBool(pstate,LUX_FALSE);

      // first is material
      mat=ResData_getMaterial(resid0);
      if (Material_getTextureStageIndex(mat,resid1)>=0)
        return FunctionPublish_returnBool(pstate,LUX_TRUE);
      // check shaders of material
      for (i = 0; i < MATERIAL_MAX_SHADERS; i++)
        if ((shd=ResData_getShader(mat->shaders[i].resRID)) && Shader_getTextureStageIndex(shd,resid1)>=0)
          return FunctionPublish_returnBool(pstate,LUX_TRUE);

      return FunctionPublish_returnBool(pstate,LUX_FALSE);
    }
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_MATAUTOCONTROL

extern int PubL3D_return(PState pstate,List3DNode_t *l3d);

enum PubMatAutoCmd_e{
  PMAC_VECTOR,
  PMAC_TARGET,
};

static int PubMatAuto_funcs(PState pstate,PubFunction_t *f,int n)
{
  Reference ref;
  MaterialAutoControl_t *matautctrl;
  List3DNode_t *l3dnode;
  void *actscn;

  if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATAUTOCONTROL,(void*)&ref) || !Reference_get(ref,matautctrl))
    return FunctionPublish_returnError(pstate,"1 matautocontrol required");

  switch((int)f->upvalue)
  {
  case PMAC_VECTOR:
    if (n==1) return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(matautctrl->vec4));
    else if (!FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(matautctrl->vec4)))
      return FunctionPublish_returnError(pstate,"1-4 floats required");
    break;
  case PMAC_TARGET:
    if (n==1){
      if (!Reference_isValid(matautctrl->targetRef))
        return 0;

      if (matautctrl->targetRefType > 0)
        return FunctionPublish_returnType(pstate,LUXI_CLASS_ACTORNODE,REF2VOID(matautctrl->targetRef));
      else if (matautctrl->targetRefType < 0){
        Reference_get(matautctrl->targetRef,l3dnode);
        return PubL3D_return(pstate,l3dnode);
      }
      else
        return FunctionPublish_returnType(pstate,LUXI_CLASS_SCENENODE,REF2VOID(matautctrl->targetRef));
    }
    else if (n >1){
      if (matautctrl->type == MATERIAL_ACTRL_MAT_PROJECTOR && FunctionPublish_getNArgType(pstate,1)!=LUXI_CLASS_L3D_PROJECTOR)
        return FunctionPublish_returnError(pstate,"1 l3dprojector required");

      // unref old
      Reference_releaseWeakCheck(matautctrl->targetRef);

      if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_ACTORNODE,(void*)&ref) && Reference_get(ref,actscn)) {
        matautctrl->targetRef = ref;
        matautctrl->targetRefType = 1;
      }
      else if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SCENENODE,(void*)&ref) && Reference_get(ref,actscn)){
        matautctrl->targetRef = ref;
        matautctrl->targetRefType = 0;
      }
      else if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_NODE,(void*)&ref) && Reference_get(ref,l3dnode)){
        matautctrl->targetRef = ref;
        matautctrl->targetRefType = -1;
      }
      else
        return FunctionPublish_returnError(pstate,"1 spatial/l3dnode required");

      Reference_refWeak(matautctrl->targetRef);
    }
    break;
  default:
    return 0;
  }

  return 0;
}

static int PubMatAuto_new(PState pstate,PubFunction_t *f,int n)
{
  MaterialAutoControl_t *matautctrl;
  int type;
  Reference ref;

  if (n < 1)  return FunctionPublish_returnError(pstate,"1 targetnode required");

  type = (MaterialACtrlType_t)f->upvalue;

  if (type == MATERIAL_ACTRL_MAT_PROJECTOR && FunctionPublish_getNArgType(pstate,0)!=LUXI_CLASS_L3D_PROJECTOR)
    return FunctionPublish_returnError(pstate,"1 l3dprojector required");

  if (FunctionPublish_getNArg(pstate,0,LUXI_CLASS_ACTORNODE,(void*)&ref) && Reference_isValid(ref)) {
    type = 1;
  }
  else if (FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SCENENODE,(void*)&ref) && Reference_isValid(ref)){
    type = 0;
  }
  else if (FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref) && Reference_isValid(ref)){
    type =  -1;
  }
  else
    return FunctionPublish_returnError(pstate,"1 spatial/l3dnode required");

  matautctrl = MaterialAutoControl_new();
  matautctrl->targetRefType = type;
  matautctrl->targetRef = ref;
  matautctrl->type = (MaterialACtrlType_t)f->upvalue;

  Reference_makeVolatile(matautctrl->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_MATAUTOCONTROL,REF2VOID(matautctrl->reference));
}


void PubClass_Material_init()
{
  FunctionPublish_initClass(LUXI_CLASS_MATERIAL,"material",
    "To allow more complex surface detail than just simple textures, materials\
    can be used to have more control over the surface. Every material consists of a shader\
    which defines how textures are blend, and textures or color definitions.<br>\
    See the mtlscript manual for more details.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_MATERIAL,LUXI_CLASS_RESOURCE);
  FunctionPublish_addFunction(LUXI_CLASS_MATERIAL,PubMaterial_load,NULL,"load",
    "(Material mtl):(string filename,[compilerargs]) - loads a material. Optionally can pass compilerstrings, eg. \"-DSOMETHING;\". You can create a simple material with a special filename when it starts with 'MATERIAL_AUTO:'. What follows in the string should be a pipe '|' separated list of .shd or texture filenames that make the shader/texture stages. Their ids are linearly generated. For textures you can put the special texturetype before, as defined in mtl-format.\
    <br><br>For example: \"MATERIAL_AUTO:myshader.shd|mytexture.jpg|TEXDOTZ anothertex.tga\" generates a simple material using the given shader and the 2 textures from which one is a special texture.");
  FunctionPublish_addFunction(LUXI_CLASS_MATERIAL,PubMaterial_cmd,(void*)PMTL_NAME,"name",
    "(string):(material) - returns the name of the loaded material.");
  FunctionPublish_addFunction(LUXI_CLASS_MATERIAL,PubMaterial_cmd,(void*)PMTL_SHADER,"getshader",
    "(Shader):(material,[int sid]) - returns a shader, optional shader id can be given, defaults to 0");
  FunctionPublish_addFunction(LUXI_CLASS_MATERIAL,PubMaterial_cmd,(void*)PMTL_CONTROL,"getcontrol",
    "([matcontrolid]):(material,string name) - returns matcontrolid, if found.");
  FunctionPublish_addFunction(LUXI_CLASS_MATERIAL,PubMaterial_cmd,(void*)PMTL_TEXCONTROL,"gettexcontrol",
    "([mattexcontrolid]):(material,string name) - returns mattexcontrolid, if found.");
  FunctionPublish_addFunction(LUXI_CLASS_MATERIAL,PubMaterial_cmd,(void*)PMTL_SHDCONTROL,"getshdcontrol",
    "([matshdcontrolid]):(material,string name) - returns matshdcontrolid, if found.");
  FunctionPublish_addFunction(LUXI_CLASS_MATERIAL,PubMaterial_cmd,(void*)PMTL_ALPHATEX,"getalphatex",
    "([texture,comparemode,float ref,int texchannel]):(material,[int sid]) - returns texture and alphatesting info if the shader supports it (alphaTEX defined).");

  FunctionPublish_initClass(LUXI_CLASS_MATCONTROLID,"matcontrolid","matcontrolid to access material control values in matobjects",NULL,LUX_FALSE);
  FunctionPublish_initClass(LUXI_CLASS_MATTEXCONTROLID,"mattexcontrolid","mattexcontrolid to access material texcontrol values in matobjects",NULL,LUX_FALSE);
  FunctionPublish_initClass(LUXI_CLASS_MATSHDCONTROLID,"matshdcontrolid","mattexcontrolid to access material shdcontrol values in matobjects",NULL,LUX_FALSE);

  FunctionPublish_initClass(LUXI_CLASS_MATOBJECT,"matobject","Several classes which materials can be applied to, allow the detailed control of some material values. l3dmodels need to pass 2 arguments as matobject: l3dmodel,meshid. If you pass materials, then you will change values for the default matobject. If the node's material is changed, its old materialobject will be lost.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_MATOBJECT,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_PRIMITIVE,LUXI_CLASS_MATOBJECT);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_TRAIL,LUXI_CLASS_MATOBJECT);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_MODEL,LUXI_CLASS_MATOBJECT);
  FunctionPublish_addInterface(LUXI_CLASS_L2D_IMAGE,LUXI_CLASS_MATOBJECT);
  FunctionPublish_addInterface(LUXI_CLASS_PARTICLECLOUD,LUXI_CLASS_MATOBJECT);
  FunctionPublish_addInterface(LUXI_CLASS_PARTICLESYS,LUXI_CLASS_MATOBJECT);
  FunctionPublish_addInterface(LUXI_CLASS_MATERIAL,LUXI_CLASS_MATOBJECT);
  FunctionPublish_addInterface(LUXI_CLASS_RCMD_DRAWMESH,LUXI_CLASS_MATOBJECT);
  FunctionPublish_addFunction(LUXI_CLASS_MATOBJECT,PubMaterialObject_attributes,(void*)MOA_SEQOFF,"moSeqoff",
    "(boolean):(matobject,[boolean]) - sets or returns sequence play state");
  FunctionPublish_addFunction(LUXI_CLASS_MATOBJECT,PubMaterialObject_attributes,(void*)MOA_MODOFF,"moModoff",
    "(boolean):(matobject,[boolean]) - sets or returns modifiers active state");
  FunctionPublish_addFunction(LUXI_CLASS_MATOBJECT,PubMaterialObject_attributes,(void*)MOA_TIME,"moTime",
    "(time):(matobject,[int]) - sets or returns time value, for automatic material values. 0 means systemtime is used.");
  FunctionPublish_addFunction(LUXI_CLASS_MATOBJECT,PubMaterialObject_attributes,(void*)MOA_MODCONTROL,"moControl",
    "([float ...]):(matobject,matcontrolid,[offset],[float ...]) - sets or returns materialcontrol value. Depending on length of control, 1..4 values are required. If the control value has length > 4, as in array values, the array offset is required. Offsets are multiplied by 4.");
  FunctionPublish_addFunction(LUXI_CLASS_MATOBJECT,PubMaterialObject_attributes,(void*)MOA_TEXCONTROL,"moTexcontrol",
    "([texture]):(matobject,mattexcontrolid,[texture]) - sets or returns material texcontrol value.");
  FunctionPublish_addFunction(LUXI_CLASS_MATOBJECT,PubMaterialObject_attributes,(void*)MOA_SHDCONTROL,"moShdcontrol",
    "([shader]):(matobject,matshdcontrolid,[shader]) - sets or returns material shdcontrol value. If material has shader parameters, the passed shader must have matching parameterids as original");
  FunctionPublish_addFunction(LUXI_CLASS_MATOBJECT,PubMaterialObject_attributes,(void*)MOA_MATRIX,"moTexmatrix",
    "(matrix4x4):(matobject,[matrix4x4]) - sets or returns texturematrix, pass 0 as matrix4x4 to disable.");
  FunctionPublish_addFunction(LUXI_CLASS_MATOBJECT,PubMaterialObject_attributes,(void*)MOA_POS,"moPos",
    "([float x,y,z]):(matobject,[float x,y,z]) - sets or returns position of texture matrix. Returns 0,0,0 if no matrix is set");
  FunctionPublish_addFunction(LUXI_CLASS_MATOBJECT,PubMaterialObject_attributes,(void*)MOA_ROTAXIS,"moRotaxis",
    "([float x1,y1,z1,x2,y2,z2,x3,y3,z3]):(matobject,[float x1,y1,z1,x2,y2,z2,x3,y3,z3]) - \
    sets or returns rotaxis of texture matrix. Returns 1,0,0, 0,1,0, 0,0,1 if no matrix is set. May contain scaling!");
  FunctionPublish_addFunction(LUXI_CLASS_MATOBJECT,PubMaterialObject_attributes,(void*)MOA_AUTOCTRL,"moAutocontrol",
    "([matautocontrol]):(matobject,matcontrolid,[matautocontrol]) - sets or returns matautocontrol for this control value. Note that only vector controllers are allowed. Passing a non-matautocontrol will disable it.");
  FunctionPublish_addFunction(LUXI_CLASS_MATOBJECT,PubMaterialObject_attributes,(void*)MOA_AUTOSTAGE,"moAutotexstage",
    "([matautocontrol]):(matobject,int stage,boolean texgen,[matautocontrol]) - sets or returns matautocontrol for a texture stage. Use texgen=true if you want to modify texgenplanes. Note that only matrix controllers are allowed. Passing a non-matautocontrol will disable it.");

  FunctionPublish_initClass(LUXI_CLASS_MATSURF,"matsurface","Most renderable items allow their material to be changed. A materialsurface can either be just color, a texture or a material.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_MATSURF,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addInterface(LUXI_CLASS_MATERIAL,LUXI_CLASS_MATSURF);
  FunctionPublish_addInterface(LUXI_CLASS_TEXTURE,LUXI_CLASS_MATSURF);
  FunctionPublish_addFunction(LUXI_CLASS_MATSURF,PubMatSurf_funcs,(void*)PMS_VCOLOR,"vertexcolor",
    "(matsurface):() - removes any previous materialinfo and makes the surface just vertexcolored");
  FunctionPublish_addFunction(LUXI_CLASS_MATSURF,PubMatSurf_funcs,(void*)PMS_CONTAINS,"contains",
    "(boolean):(matsurface self, matsurface other) - checks if one matsurface contains another or is equal");

  FunctionPublish_initClass(LUXI_CLASS_MATAUTOCONTROL,"matautocontrol","Some matcontrolids or matobject properties allow auotmatic value generation, such as tracking node positions. You can use a matautocontrol to do so. It is GCed by Lua, and will survive as long as it is used or referenced by lua.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_MATAUTOCONTROL,LUXI_CLASS_RENDERINTERFACE);
  Reference_registerType(LUXI_CLASS_MATAUTOCONTROL,"matautocontrol",RMaterialAutoControl_free,NULL);

  FunctionPublish_addFunction(LUXI_CLASS_MATAUTOCONTROL,PubMatAuto_funcs,(void*)PMAC_TARGET,"target",
    "([spatialnode/l3dnode]):(matautocontrol,[spatialnode/l3dnode]) - returns or sets target to be tracked. After the matautocontrol has been assigned and the target becomes invalid, the matautocontrol is destroyed as well.");
  FunctionPublish_addFunction(LUXI_CLASS_MATAUTOCONTROL,PubMatAuto_funcs,(void*)PMAC_VECTOR,"vector",
    "([float x,y,z,w]):(matautocontrol,[float x,y,z,w]) - returns or sets a custom vector that may be used by some controllers.");

  FunctionPublish_addFunction(LUXI_CLASS_MATAUTOCONTROL,PubMatAuto_new,(void*)MATERIAL_ACTRL_VEC4_NODEPOS,"newvec4pos",
    "(matautocontrol):([spatialnode/l3dnode]) - autocontroller that transfers target position as vector.");
  FunctionPublish_addFunction(LUXI_CLASS_MATAUTOCONTROL,PubMatAuto_new,(void*)MATERIAL_ACTRL_VEC4_NODEDIR,"newvec4dir",
    "(matautocontrol):([spatialnode/l3dnode]) - autocontroller that transfers direction from current to target as vector.");
  FunctionPublish_addFunction(LUXI_CLASS_MATAUTOCONTROL,PubMatAuto_new,(void*)MATERIAL_ACTRL_VEC4_NODEPOS_PROJ,"newvec4posproj",
    "(matautocontrol):([spatialnode/l3dnode]) - autocontroller that transfers position of target in screenspace as vector.");
  FunctionPublish_addFunction(LUXI_CLASS_MATAUTOCONTROL,PubMatAuto_new,(void*)MATERIAL_ACTRL_MAT_NODEPOS_PROJ,"newmatposproj",
    "(matautocontrol):([spatialnode/l3dnode]) - autocontroller that creates matrix for translation of target in screenspace. vector.x is scale factor");
  FunctionPublish_addFunction(LUXI_CLASS_MATAUTOCONTROL,PubMatAuto_new,(void*)MATERIAL_ACTRL_MAT_NODEROT,"newmatrot",
    "(matautocontrol):([spatialnode/l3dnode]) - autocontroller that creates matrix for rotation towards target from current node (so that 0,0,1 points to target).");
  FunctionPublish_addFunction(LUXI_CLASS_MATAUTOCONTROL,PubMatAuto_new,(void*)MATERIAL_ACTRL_MAT_PROJECTOR,"newmatprojector",
    "(matautocontrol):([l3dprojector]) - autocontroller that creates matrix for translation of target in screenspace.");
}
