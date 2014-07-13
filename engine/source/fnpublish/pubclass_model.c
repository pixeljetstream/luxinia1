// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../render/gl_list3d.h"
#include "../render/gl_list2d.h"
#include "../common/reflib.h"
#include "../fnpublish/pubclass_types.h"
#include "../main/main.h"

// Published here:
// LUXI_CLASS_MODEL
// LUXI_CLASS_MESHID
// LUXI_CLASS_SKINID
// LUXI_CLASS_VERTEXARRAY
// LUXI_CLASS_INDEXARRAY
// LUXI_CLASS_MORPHCONTROL
// LUXI_CLASS_L3D_MODEL


//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_MODEL

static int PubModel_load (PState pstate,PubFunction_t *fn, int n)
{
  char *path;
  booln animateable,neednormals,fatvertex,lmcoords;
  int index;

  fatvertex = LUX_FALSE;
  animateable = LUX_FALSE;
  neednormals = LUX_TRUE;


  if (n<1 || FunctionPublish_getArg(pstate,LUX_MIN(n,4),
      LUXI_CLASS_STRING,(void*)&path,
      LUXI_CLASS_BOOLEAN,(void*)&animateable,
      LUXI_CLASS_BOOLEAN,(void*)&neednormals,
      LUXI_CLASS_BOOLEAN,(void*)&fatvertex)<1)
    return FunctionPublish_returnError(pstate,"1 string [3 booleans] required");
  index = animateable ? MODEL_DYNAMIC : MODEL_STATIC;
  if (n > 4){
    animateable = LUX_FALSE;
    FunctionPublish_getNArg(pstate,4,LUXI_CLASS_BOOLEAN,(void*)&animateable);
    if (animateable)
      index++;
  }
  if (n > 5){
    animateable = LUX_FALSE;
    FunctionPublish_getNArg(pstate,5,LUXI_CLASS_BOOLEAN,(void*)&animateable);
    if (animateable)
      index+=2;
  }
  lmcoords = LUX_FALSE;
  if (n>6)
    FunctionPublish_getNArg(pstate,6,LUXI_CLASS_BOOLEAN,(void*)&lmcoords);

  Main_startTask(LUX_TASK_RESOURCE);

  if (fn->upvalue)
    ResData_forceLoad(RESOURCE_MODEL);
  index = ResData_addModel(path,index, fatvertex ? VERTEX_64_TEX4 : (neednormals ? VERTEX_32_NRM : VERTEX_32_TEX2),lmcoords);

  if (fn->upvalue)
    ResData_forceLoad(RESOURCE_NONE);

  Main_endTask();

  if (index==-1)
    return 0;

  return FunctionPublish_returnType(pstate,LUXI_CLASS_MODEL,(void*)index);
}

static int PubModel_new (PState pstate,PubFunction_t *fn, int n)
{
  static char texname[256] = {0};
  char *name;
  int vertextype;
  int index;
  int mcount,hcount,scount;
  Model_t *model;

  scount = 0;
  if (n<4 || 4>FunctionPublish_getArg(pstate,4,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_VERTEXTYPE,(void*)&vertextype,LUXI_CLASS_INT,(void*)&mcount,LUXI_CLASS_INT,(void*)&hcount,LUXI_CLASS_INT,(void*)&scount))
    return FunctionPublish_returnError(pstate,"1 string 1 vertextype 2 int [+1 int] required");

  sprintf(texname,"%s%s",MODEL_USERSTART,name);
  Main_startTask(LUX_TASK_RESOURCE);
  index = ResData_addModel(texname,MODEL_UNSET,vertextype,LUX_FALSE);
  Main_endTask();

  if (index==-1)
    return 0;

  model = ResData_getModel(index);
  Model_allocBones(model,hcount+mcount);
  Model_allocMeshObjs(model,mcount);
  Model_allocSkins(model,scount);
  {
    int i;
    Bone_t *bone;
    for (i=0; i < model->numMeshObjects;i++){
      bone = &model->bonesys.bones[i];
      model->meshObjects[i].bone = bone;
      bone->parentID = 0;
      lxMatrix44Identity(bone->refMatrix);
    }
  }

  return FunctionPublish_returnType(pstate,LUXI_CLASS_MODEL,(void*)index);
}

static int PubModel_newfinish (PState pstate,PubFunction_t *fn, int n)
{
  int modeltype;
  int index;
  int nodl,novbo,lmcoords;
  Model_t *model;
  ResourceChunk_t *oldchunk;

  bool8 animateable;

  if (n<2 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_MODEL,(void*)&index,LUXI_CLASS_BOOLEAN,(void*)&animateable)<2
    || !(model=ResData_getModel(index)) || model->modeltype!=MODEL_UNSET)
    return FunctionPublish_returnError(pstate,"1 unfinished model 1 boolean required");




  modeltype = animateable ? MODEL_DYNAMIC : MODEL_STATIC;
  lmcoords = nodl = novbo = LUX_FALSE;
  FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&nodl);
  FunctionPublish_getNArg(pstate,3,LUXI_CLASS_BOOLEAN,(void*)&novbo);
  FunctionPublish_getNArg(pstate,4,LUXI_CLASS_BOOLEAN,(void*)&lmcoords);


  if (nodl && novbo)
    modeltype += 3;
  else if (nodl)
    modeltype += 1;
  else if (novbo)
    modeltype += 2;

  model->modeltype = model->orgtype = modeltype;

  // link our bones
  if (!Model_linkBones(model))
    return FunctionPublish_returnError(pstate,"bone linking failed");
  Model_initBones(model); // time to Initialize our bones' relative matrices

  // pre transform
  Model_initVertices(model);
  Model_initStats(model);
  Model_updateBoundingBox(model);

  // do skin
  Model_initSkin(model);

  // upload resources
  oldchunk = ResData_setChunkFromPtr(&model->resinfo);
  ResData_ModelInit(model,LUX_FALSE,lmcoords);
  ResourceChunk_activate(oldchunk);

  return FunctionPublish_returnType(pstate,LUXI_CLASS_MODEL,(void*)index);
}

enum PubModelFunc_e{
  PMDL_PRESCALE,

  PMDL_NEEDMODEL_FUNCS,

  PMDL_BBOX,
  PMDL_BBOXMANUAL,
  PMDL_GLUPDATE,
  PMDL_REMANIMASSIGN,
  PMDL_RUNANIM,
};

static int PubModel_func(PState pstate,PubFunction_t *fn,int n)
{
  AnimBlend_t ablend2 = {NULL,0,0};
  Model_t *model;
  int index;

  byte state;
  byte clr;

  if ((int)fn->upvalue > PMDL_NEEDMODEL_FUNCS && (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MODEL,(void*)&index) || !(model=ResData_getModel(index))))
    return FunctionPublish_returnError(pstate,"1 model required");

  switch((int)fn->upvalue)
  {
  case PMDL_PRESCALE:
    {
      lxVector3 prescale;
      if (n==0){
        ModelLoader_getPrescale(prescale);
        return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(prescale));
      }
      else if (n < 3 || 3>FunctionPublish_getArg(pstate,3,FNPUB_TOVECTOR3(prescale)))
        return FunctionPublish_returnError(pstate,"3 floats required");

      ModelLoader_setPrescale(prescale);
      return 0;
    }
  case PMDL_RUNANIM:
    {
      AnimBlend_t ablend;
      Reference ref;
      float *matrix = NULL;

      if ((2>(ablend2.inTime=FunctionPublish_getArgOffset(pstate,1,3,LUXI_CLASS_ANIMATION,(void*)&index,LUXI_CLASS_INT,(void*)&ablend.inTime, LUXI_CLASS_MATRIX44, (void*)&ref)))
        || !(ablend.anim=ResData_getAnim(index)) || ablend2.inTime < 0 || ablend2.inTime > ablend.anim->length
        || (ablend2.inTime==3 && !Reference_get(ref,matrix)))
        return FunctionPublish_returnError(pstate,"1 model 1 anim 1 int (within animlength) [1 matrix4x4] required");
      model->bonesys.absMatrices = model->bonesys.userAbsMatrices;
      model->bonesys.relMatricesT = model->bonesys.userRelMatricesT;

      ablend.animflag = ablend.anim->animflag;
      AnimBlend_updateBoneSys(&ablend,&ablend2,0,&model->bonesys);
      BoneSys_update(&model->bonesys,BONE_UPDATE_ABS, matrix);

      model->bonesys.absMatrices = model->bonesys.refAbsMatrices;
      model->bonesys.relMatricesT = model->bonesys.relMatricesT;
      return 0;
    }
  case PMDL_REMANIMASSIGN:
    if (!FunctionPublish_getArg(pstate,1,LUXI_CLASS_ANIMATION,(void*)&index))
      return FunctionPublish_returnError(pstate,"1 model 1 animation required");
    Anim_removeAssign(ResData_getAnim(index),&model->bonesys);
    return 0;
  case PMDL_BBOX:
    Model_updateBoundingBox(model);
    return 0;
  case PMDL_BBOXMANUAL:
    if (n==1){
      return FunctionPublish_setRet(pstate,6,FNPUB_FROMVECTOR3(model->bbox.min),FNPUB_FROMVECTOR3(model->bbox.max));
    }
    else if (n==2){
      Reference ref;
      lxVector3 min,max;
      float *matrix;
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,matrix))
        return FunctionPublish_returnError(pstate,"1 model 1 matrix4x4 required");

      lxBoundingBox_transformV(min,max,model->bbox.min,model->bbox.max,matrix);

      return FunctionPublish_setRet(pstate,6,FNPUB_FROMVECTOR3(min),FNPUB_FROMVECTOR3(max));
    }
    if(6!=FunctionPublish_getArgOffset(pstate,1,6,FNPUB_TOVECTOR3(model->bbox.min),FNPUB_TOVECTOR3(model->bbox.max)))
      return FunctionPublish_returnError(pstate,"1 model 6 floats required");
    return 0;
  case PMDL_GLUPDATE:
    state = LUX_FALSE;
    clr = LUX_TRUE;
    if (FunctionPublish_getArgOffset(pstate,1,3,LUXI_CLASS_MESHTYPE,(void*)&index,LUXI_CLASS_BOOLEAN,(void*)&state,LUXI_CLASS_BOOLEAN,(void*)&clr)<1)
      return FunctionPublish_returnError(pstate,"1 valid meshtype required");
    Model_initGL(model,index,state,clr);
    return 0;
  default:
    return 0;
  }
}


static int PubModel_getmesh (PState pstate,PubFunction_t *fn, int n)
{
  char *name;
  int index;
  int model;
  lxClassType type;

  if (n<1 || FunctionPublish_getArg(pstate,1,LUXI_CLASS_MODEL,(void*)&model)!=1)
    return FunctionPublish_returnError(pstate,"1 model required");
  if (ResData_getModel(model)==NULL)
    return FunctionPublish_returnError(pstate,"invalid model");

  if (fn->upvalue)
    return FunctionPublish_returnInt(pstate,ResData_getModel(model)->numMeshObjects);

  type = FunctionPublish_getNArgType(pstate,1);
  if (type == LUXI_CLASS_INT || type == LUXI_CLASS_FLOAT) {
    FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&index);

    if (index<0 || index>=ResData_getModel(model)->numMeshObjects)
      return FunctionPublish_returnError(pstate,"meshindex out of range (must be 0..<meshcnt");
    SUBRESID_MAKE(index,model);
    return FunctionPublish_returnType(pstate,LUXI_CLASS_MESHID,(void*)index);
  }
  if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&name)!=1)
    return FunctionPublish_returnError(pstate,"1 model 1 string or 1 int required");
  index = Model_searchMesh(ResData_getModel(model),name);
  if (index==-1)
    return 0;
  SUBRESID_MAKE(index,model);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_MESHID,(void*)index);
}

static int PubModel_getskin (PState pstate,PubFunction_t *fn, int n)
{
  int index;
  int model;

  if (n<1 || FunctionPublish_getArg(pstate,1,LUXI_CLASS_MODEL,(void*)&model)!=1)
    return FunctionPublish_returnError(pstate,"1 model required");
  if (ResData_getModel(model)==NULL)
    return FunctionPublish_returnError(pstate,"invalid model");

  if (fn->upvalue)
    return FunctionPublish_returnInt(pstate,ResData_getModel(model)->numSkins);

  if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&index)!=1)
    return FunctionPublish_returnError(pstate,"1 model 1 int required");
  if (index<0 || index>=ResData_getModel(model)->numSkins)
    return FunctionPublish_returnError(pstate,"skinindex out of range (must be 0..<skincnt");
  SUBRESID_MAKE(index,model);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_SKINID,(void*)index);

}

static int PubModel_getbone (PState pstate,PubFunction_t *fn, int n)
{
  char *name;
  int index;
  int model;
  lxClassType type;

  if (n<1 || FunctionPublish_getArg(pstate,1,LUXI_CLASS_MODEL,(void*)&model)!=1)
    return FunctionPublish_returnError(pstate,"1 model required");
  if (ResData_getModel(model)==NULL)
    return FunctionPublish_returnError(pstate,"invalid model");

  if (fn->upvalue)
    return FunctionPublish_returnInt(pstate,ResData_getModel(model)->bonesys.numBones);

  type = FunctionPublish_getNArgType(pstate,1);
  if (type == LUXI_CLASS_INT || type == LUXI_CLASS_FLOAT) {
    FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&index);

    if (index<0 || index>=ResData_getModel(model)->bonesys.numBones)
      return FunctionPublish_returnError(pstate,"meshindex out of range (must be 0..<bonecnt");
    SUBRESID_MAKE(index,model);
    return FunctionPublish_returnType(pstate,LUXI_CLASS_BONEID,(void*)index);
  }
  if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&name)!=1)
    return FunctionPublish_returnError(pstate,"1 model 1 string or 1 int required");
  index = BoneSys_searchBone(&ResData_getModel(model)->bonesys,name);
  if (index==-1)
    return 0;
  SUBRESID_MAKE(index,model);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_BONEID,(void*)index);
}

static int PubModel_getboneparents(PState pstate,PubFunction_t *f,int n)
{
  int id;
  Model_t *model;
  Bone_t *bone;
  BoneSys_t *bsys;
  PubArray_t  names;

  if (n<1 || FunctionPublish_getArg(pstate,1,LUXI_CLASS_MODEL,(void*)&id)!=1)
    return FunctionPublish_returnError(pstate,"1 model required");
  if ((model=ResData_getModel(id))==NULL)
    return FunctionPublish_returnError(pstate,"invalid model");


  bsys = &model->bonesys;
  bone = bsys->bones;

  bufferclear();
  names.data.ints = buffermalloc(sizeof(int)*bsys->numBones);
  names.length = bsys->numBones;
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

  for (id = 0; id < bsys->numBones; id++,bone++){
    names.data.ints[id] =(bone->parentB) ? bone->parentID/16 : -1;
  }

  FunctionPublish_returnType(pstate,LUXI_CLASS_ARRAY_INT,(void*)&names);

  bufferclear();
  return 1;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_SKINID

enum SKIND_PROP {
  SID_HWSUPPORT,
  SID_NEED_1INDEX_FUNCS,
  SID_VERTEXWEIGHTCOUNT,
  SID_BONEID,
  SID_NEED_2INDEX_FUNCS,
  SID_NEW,
  SID_VERTEXBONE,
  SID_VERTEXWEIGHT,
};

static int PubSkinID_prop (PState pstate,PubFunction_t *fn, int n)
{
  Model_t *model;
  Skin_t  *skin;
  SkinVertex_t *svert;
  int   skinid;
  int   modelid;
  int   meshid;

  if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SKINID,(void*)&skinid))
    return FunctionPublish_returnError(pstate,"1 skinid required");
  modelid = SUBRESID_GETRES(skinid);
  SUBRESID_MAKEPURE(skinid);
  if ((model = ResData_getModel(modelid))==NULL)
    return FunctionPublish_returnError(pstate,"model is invalid");

  skin = &model->skins[skinid];

  if ((int)fn->upvalue > SID_NEED_1INDEX_FUNCS && (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&meshid) || meshid < 0))
    return FunctionPublish_returnError(pstate,"1 skinid 1 int required");

  if ((int)fn->upvalue > SID_NEED_2INDEX_FUNCS && (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&modelid) || modelid < 0))
    return FunctionPublish_returnError(pstate,"1 skinid 2 int required");

  // index0 = meshid
  // index1 = modelid

  switch((int)fn->upvalue)
  {
  case SID_NEW:
    if (skin->boneIndices)
      return FunctionPublish_returnError(pstate,"skinid already defined");

    Model_setSkin(model,skinid,meshid,modelid);
    break;
  case SID_HWSUPPORT:
    return FunctionPublish_returnBool(pstate,skin->hwskinning);
    break;
  case SID_BONEID:
    if (n==2){
      skinid = skin->boneIndices[meshid];
      skinid /=12;
      SUBRESID_MAKE(skinid,model->resinfo.resRID);
      return FunctionPublish_returnType(pstate,LUXI_CLASS_BONEID,(void*)skinid);
    }
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BONEID,(void*)&skinid) || SUBRESID_GETRES(skinid)!=model->resinfo.resRID)
      return FunctionPublish_returnError(pstate,"1 skinid 1 int 1 proper boneid required");
    SUBRESID_MAKEPURE(skinid);
    skin->boneIndices[meshid] = skinid * 12;
    break;
  case SID_VERTEXWEIGHTCOUNT:
    svert = &skin->skinvertices[meshid];
    if (n==2) return FunctionPublish_returnInt(pstate,svert->numWeights);
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&modelid) || modelid < 0 || modelid >= MODEL_MAX_WEIGHTS)
      return FunctionPublish_returnError(pstate,"1 skinid 1 int 1 int(0..3) required");
    svert->numWeights = modelid;
    break;
  case SID_VERTEXBONE:
    svert = &skin->skinvertices[meshid];
    modelid %= MODEL_MAX_WEIGHTS;
    if (n==3) return FunctionPublish_returnInt(pstate,svert->matrixIndex[modelid]);
    else if (!FunctionPublish_getNArg(pstate,3,LUXI_CLASS_INT,(void*)&meshid) || meshid < 0 || meshid >= skin->numBones)
      return FunctionPublish_returnError(pstate,"1 skinid 2 int 1 int(0...bonecnt-1) required");
    svert->matrixIndex[modelid] = meshid;
    break;
  case SID_VERTEXWEIGHT:
    svert = &skin->skinvertices[meshid];
    modelid %= MODEL_MAX_WEIGHTS;
    if (n==3) return FunctionPublish_returnFloat(pstate,svert->weights[modelid]);
    else if (!FunctionPublish_getNArg(pstate,3,FNPUB_TFLOAT(svert->weights[modelid])))
      return FunctionPublish_returnError(pstate,"1 skinid 2 int 1 float required");
    break;
  default:
    break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_MESHID

enum MESHID_PROP {
  MID_NAME,
  MID_NEW,
  MID_TEXTURE,
  MID_TEXNAME,
  MID_MATERIAL,
  MID_MATSURF,
  MID_BONEID,
  MID_SKINID,
  MID_RMESH,
};
static int PubMeshID_prop (PState pstate,PubFunction_t *fn, int n)
{
  int meshid,modelid;
  Model_t *model;
  MeshObject_t *meshobj;
  char *name;
  char *texname;
  int icount,vcount;

  if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MESHID,(void*)&meshid))
    return FunctionPublish_returnError(pstate,"1 meshid required");
  modelid = SUBRESID_GETRES(meshid);
  SUBRESID_MAKEPURE(meshid);
  if ((model = ResData_getModel(modelid))==NULL)
    return FunctionPublish_returnError(pstate,"model is invalid");

  meshobj = &model->meshObjects[meshid];

  switch ((int)fn->upvalue)
  {
  case MID_NAME:
    return FunctionPublish_returnString(pstate,meshobj->name);
  case MID_NEW:
    if (meshobj->name)
      return FunctionPublish_returnError(pstate,"meshobj data already written");
    texname = NULL;
    if (n<3 || FunctionPublish_getArgOffset(pstate,1,4,LUXI_CLASS_STRING,&name,LUXI_CLASS_INT,&vcount,LUXI_CLASS_INT,&icount,LUXI_CLASS_STRING,&texname)<3)
      return FunctionPublish_returnError(pstate,"1 meshid 1 string 2 ints [1 string] required");
    Model_setMesh(model,meshid,name,vcount,icount,texname);
    break;
  case MID_TEXTURE:
    if (meshobj->texRID<0) return 0;
    if (vidMaterial(meshobj->texRID)==LUX_FALSE)
      return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)meshobj->texRID);
    return 0;
  case MID_MATERIAL:
    if (meshobj->texRID<0) return 0;
    if (vidMaterial(meshobj->texRID)==LUX_TRUE)
      return FunctionPublish_returnType(pstate,LUXI_CLASS_MATERIAL,(void*)meshobj->texRID);
    return 0;
  case MID_MATSURF:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_MATSURF,(void*)meshobj->texRID);
  case MID_TEXNAME:
    return FunctionPublish_returnString(pstate,meshobj->texname);
  case MID_BONEID:
    if (n==1 && meshobj->bone){
      meshid = meshobj->bone->ID/16;
      SUBRESID_MAKE(meshid,model->resinfo.resRID);
      return FunctionPublish_returnType(pstate,LUXI_CLASS_BONEID,(void*)meshid);
    }
    else if (n==2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BONEID,(void*)&meshid) || SUBRESID_GETRES(meshid)!=modelid )
        return FunctionPublish_returnError(pstate,"1 meshid 1 proper boneid required");
      SUBRESID_MAKEPURE(meshid);
      meshobj->bone = &model->bonesys.bones[meshid];
    }
    break;
  case MID_SKINID:
    if (n==1 && meshobj->skinID >= 0){
      meshid = meshobj->skinID;
      SUBRESID_MAKE(meshid,model->resinfo.resRID);
      return FunctionPublish_returnType(pstate,LUXI_CLASS_SKINID,(void*)meshid);
    }
    else if (n==2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SKINID,(void*)&meshid) || SUBRESID_GETRES(meshid)!=modelid )
        return FunctionPublish_returnError(pstate,"1 meshid 1 proper skinid required");
      SUBRESID_MAKEPURE(meshid);
      meshobj->skinID = meshid;
      return 0;
    }
  case MID_RMESH:
    {
      RenderMesh_t *rmesh = RenderMesh_new();
      rmesh->mesh = meshobj->mesh;
      rmesh->freemesh = LUX_FALSE;
      rmesh->vboref = NULL;
      rmesh->iboref = NULL;

      Reference_makeVolatile(rmesh->reference);
      return FunctionPublish_returnType(pstate,LUXI_CLASS_RENDERMESH,REF2VOID(rmesh->reference));
    }
  default:
    break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_VERTEXARRAY

static void MeshArrayHandle_free (MeshArrayHandle_t *rptr)
{
  Reference_releaseWeakCheck(rptr->srcref);
  lxMemGenFree(rptr,sizeof(MeshArrayHandle_t));
}

Reference MeshArrayHandle_new(MeshArrayHandle_t **out,lxClassType type)
{
  MeshArrayHandle_t *vah = lxMemGenZalloc(sizeof(MeshArrayHandle_t));
  Reference ref = Reference_new(type,vah);

  *out = vah;
  return ref;
}

static int PubVertexArray_new(PState pstate,PubFunction_t *fn, int n)
{
  lxVertexType_t vtype;
  byte* from;
  byte* to;
  MeshArrayHandle_t*  vah;
  Reference lref;

  if (n<3 || 3>FunctionPublish_getArg(pstate,3,LUXI_CLASS_VERTEXTYPE,(void*)&vtype,
    LUXI_CLASS_POINTER,(void*)&from,LUXI_CLASS_POINTER,(void*)&to))
  {
      return FunctionPublish_returnError(pstate,"1 vertextype 2 pointer required");
  }

  lref = MeshArrayHandle_new(&vah,LUXI_CLASS_VERTEXARRAY);
  vah->ptrs.begin = from;
  vah->ptrs.num = (to-from)/VertexSize(vtype);
  vah->ptrs.vtype = vtype;
  vah->type = MESHARRAY_POINTERS;
  Reference_makeVolatile(lref);

  return FunctionPublish_returnType(pstate,LUXI_CLASS_VERTEXARRAY,REF2VOID(lref));
}


enum VertexArrayProp_e{
  VA_POS,
  VA_NORMAL,
  VA_TANGENT,
  VA_TEX,
  VA_COLOR,
  VA_COPY,

  VA_FUNC_SEPARATOR,

  VA_TRANSFORM,
  VA_VCOUNT,
  VA_VALLOC,
  VA_POS2TEX,
  VA_TEXINV,
  VA_TEXTRANSFORM,
  VA_COLARRAY,
  VA_POSARRAY,
  VA_NORMALARRAY,

  VA_BOUNDS,
  VA_UVTRIS,
  VA_NORMALTRIS,
  VA_POSTRIS,
  VA_COLORTRIS,
  VA_PTRS,

  VA_SUBMITGL,
  VA_RETRIEVEGL,
  VA_PTRSTATE,
  VA_MESHTYPE,
  VA_VIDBUFFER,
  VA_TYPE,
};

typedef struct VAMeshInfo_s{
  Mesh_t    *mesh;
  int     meshtype;
  VIDBuffer_t *bo;
  int     vtype;
  union{
    lxVertex64_t  *vert64;
    lxVertex32_t  *vert32;
    lxVertex16_t  *vert16;
    void  *verts;
    char  *vertsdata;
  };

  int vcountalloc;
  int *vcountptr;
}VAMeshInfo_t;

static char* VAMeshInfo_init(PState pstate,VAMeshInfo_t *info, int stack)
{
  Reference ref;
  Mesh_t  *mesh = NULL;

  if (!FunctionPublish_getNArg(pstate,stack,LUXI_CLASS_VERTEXARRAY,(void*)&ref))
    return "1 vertexarray required";

  info->meshtype = MESH_UNSET;
  switch(FunctionPublish_getNArgType(pstate,stack)) {
  case LUXI_CLASS_RENDERMESH:
  {
    RenderMesh_t *rmesh;
    if (!Reference_get(ref,rmesh))
      return "1 rendermesh required";

    mesh = info->mesh = rmesh->mesh;
    info->bo = mesh->vid.vbo;
    info->meshtype = mesh->meshtype;
    info->verts = mesh->vertexData;
    info->vtype = mesh->vertextype;
    info->vcountalloc = mesh->numAllocVertices;
    info->vcountptr = &mesh->numVertices;
  }
  break;

  case LUXI_CLASS_VERTEXARRAY:
  {
    MeshArrayHandle_t *vahandle;

    if (!Reference_get(ref,vahandle))
      return "1 vertexarray required";

    switch(vahandle->type){
    case MESHARRAY_L3DMODEL_MESHID:
    {

      VertexContainer_t *vcont;
      List3DNode_t *l3d;
      int meshid;

      if (!Reference_get(vahandle->srcref,l3d))
        return "1 vertexarray from valid l3dmodel required";
      meshid = vahandle->meshid;
      SUBRESID_MAKEPURE(meshid);
      if (l3d->compileflag & LIST3D_COMPILE_BATCHED){
        mesh = info->mesh = l3d->drawNodes[meshid].draw.mesh;
        info->vtype = mesh->vertextype;
        info->verts = (void*) VertexArrayPtr(mesh->vertexData,l3d->drawNodes[meshid].batchinfo.vertoffset,info->vtype,VERTEX_START) ;
        info->vcountalloc = mesh->numAllocVertices;
        info->vcountptr = &mesh->numVertices;
        info->bo = mesh->vid.vbo;

        info->meshtype = mesh->meshtype;
      }
      else{
        vcont = l3d->minst->modelupd->vertexcont;
        info->verts = vcont->vertexDatas[meshid];
        info->vcountalloc = vcont->numVertices[meshid];
        info->vcountptr = NULL;
        info->bo = NULL;
        info->vtype = vcont->vertextype;
        info->mesh = NULL;
      }
    }
    break;
    case MESHARRAY_POINTERS:
    {
      info->vtype = vahandle->ptrs.vtype;
      info->verts = vahandle->ptrs.begin;
      info->vcountalloc = vahandle->ptrs.num;
      info->vcountptr = NULL;
      info->bo = NULL;
      info->mesh = NULL;
    }
    break;
    }
  }

    break;
  case LUXI_CLASS_MESHID:
  {
    Model_t *model;
    int meshid;

    meshid = *(int*)&ref;
    if ((model = ResData_getModel(SUBRESID_GETRES(meshid)))==NULL)
      return "model is invalid";
    SUBRESID_MAKEPURE(meshid);
    info->mesh = mesh = model->meshObjects[meshid].mesh;
    info->bo = mesh->vid.vbo;
    info->meshtype = mesh->meshtype;
    info->verts = mesh->vertexData;
    info->vtype = mesh->vertextype;
    info->vcountalloc = mesh->numAllocVertices;
    info->vcountptr = &mesh->numVertices;
  }
    break;
  case LUXI_CLASS_L2D_IMAGE:
  {
    List2DNode_t  *l2d;

    if (!Reference_get(ref,l2d) || l2d->l2dimage->mesh == g_VID.drw_meshquad || l2d->l2dimage->mesh == g_VID.drw_meshquadcentered)
      return "1 l2dimage with usermesh required";
    info->mesh = mesh = l2d->l2dimage->mesh;
    info->meshtype = mesh->meshtype;
    info->verts =  mesh->vertexData;
    info->bo = mesh->vid.vbo;
    info->vtype = mesh->vertextype;
    info->vcountalloc = mesh->numAllocVertices;
    info->vcountptr = &mesh->numVertices;
  }
    break;
  case LUXI_CLASS_RCMD_DRAWMESH:
  {
    RenderCmdMesh_t *ffx;

    if (!Reference_get(ref,ffx) || ffx->dnode.mesh == g_VID.drw_meshquadffx)
      return "1 rcmdmesh with usermesh required";
    info->mesh = mesh = ffx->dnode.mesh;
    info->meshtype = mesh->meshtype;
    info->verts =  mesh->vertexData;
    info->bo = mesh->vid.vbo;
    info->vtype = mesh->vertextype;
    info->vcountalloc = mesh->numAllocVertices;
    info->vcountptr = &mesh->numVertices;
  }
    break;
  case LUXI_CLASS_L3D_PRIMITIVE:
  {
    List3DNode_t *l3d;

    if (!Reference_get(ref,l3d) || l3d->drawNodes->draw.mesh == l3d->primitive->orgmesh)
      return "1 l3dprimitive with usermesh required";
    info->mesh = mesh = l3d->drawNodes->draw.mesh;
    info->meshtype = mesh->meshtype;
    info->verts = mesh->vertexData;
    info->bo = mesh->vid.vbo;
    info->vtype = mesh->vertextype;
    info->vcountalloc = mesh->numAllocVertices;
    info->vcountptr = &mesh->numVertices;
  }
    break;
  default:
    return "1 vertexarray required";
  }
  return NULL;
}

static int PubVertexArray_prop (PState pstate,PubFunction_t *fn, int n)
{
  VAMeshInfo_t  info;
  VAMeshInfo_t  infofrom;
  Reference ref;
  lxMatrix44 mat;
  float *f;
  float *posptr;
  float *base;
  uchar *ub;
  uchar *ub1;
  uchar *ub2;
  short *pNormal;
  short *pNormal1;
  short *pNormal2;
  lxVector4 vector;
  lxVector4 vector1;
  lxVector4 vector2;
  lxVector4 vector3;
  lxVector4 vector4;
  int index;
  int i;
  int a;
  int b;
  int c;
  int texchannel;
  StaticArray_t *starr;
  char *errorstring;

  if (errorstring = VAMeshInfo_init(pstate,&info,0))
    return FunctionPublish_returnError(pstate,errorstring);


  if ((int)fn->upvalue < VA_FUNC_SEPARATOR && (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&index) || index < 0 || index >= info.vcountalloc || !info.verts))
    return FunctionPublish_returnError(pstate,"index required or index out of bounds");

  switch((int)fn->upvalue) {
  case VA_POS:
    switch(info.vtype) {
    case VERTEX_32_TEX2:
    case VERTEX_32_NRM:
      f = info.vert32[index].pos;
      break;
    case VERTEX_16_CLR:
      f = info.vert16[index].pos;
      break;
    case VERTEX_64_TEX4:
    case VERTEX_64_SKIN:
      f = info.vert64[index].pos;
      break;
    default:
      return 0;
    }
    posptr = NULL;
    if (n>2 && FunctionPublish_getNArg(pstate,2,LUXI_CLASS_MATRIX44,(void*)&ref) &&
        !Reference_get(ref,posptr))
      return FunctionPublish_returnError(pstate,"1 matrix4x4 required");
    if (posptr){
      lxVector3Transform(vector,f,posptr);
      f = vector;
    }

    if (n==2 || n==3)
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(f));
    if (3!=FunctionPublish_getArgOffset(pstate,posptr ? 3 : 2,3,FNPUB_TOVECTOR3(vector)))
      return FunctionPublish_returnError(pstate,"3 floats required");
    if (posptr)
      lxVector3Transform(f,vector,posptr);
    else
      lxVector3Copy(f,vector);
    break;
  case VA_PTRS:
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_POINTER,(void*)info.vertsdata,LUXI_CLASS_POINTER,(void*)(info.vertsdata+(VertexSize(info.vtype)*info.vcountalloc)));
  case VA_NORMAL:

    posptr = NULL;
    if (n>2 && FunctionPublish_getNArg(pstate,2,LUXI_CLASS_MATRIX44,(void*)&ref) &&
      !Reference_get(ref,posptr))
      return FunctionPublish_returnError(pstate,"1 matrix4x4 required");

    switch(info.vtype) {
    case VERTEX_32_NRM:
      pNormal = info.vert32[index].normal;
      break;
    case VERTEX_64_TEX4:
    case VERTEX_64_SKIN:
      pNormal = info.vert64[index].normal;
      break;
    default:
      return 0;
    }

    posptr = NULL;
    if (n>2 && FunctionPublish_getNArg(pstate,2,LUXI_CLASS_MATRIX44,(void*)&ref) &&
      !Reference_get(ref,posptr))
      return FunctionPublish_returnError(pstate,"1 matrix4x4 required");


    f = vector;
    if (n==2 || n==3){
      lxVector3float_FROM_short(vector,pNormal);
      if (posptr){
        lxVector3TransformRot(vector1,vector,posptr);
        f = vector1;
      }
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(f));
    }
    if (3!=FunctionPublish_getArgOffset(pstate,posptr ? 3 : 2,3,FNPUB_TOVECTOR3(vector)))
      return FunctionPublish_returnError(pstate,"3 floats required");

    if (posptr){
      lxVector3TransformRot(vector1,vector,posptr);
      f = vector1;
    }

    lxVector3short_FROM_float(pNormal,f);

    break;
  case VA_TEX:
    texchannel= 0;
    // last arg is always texchannel
    if ((n==3 || n==5) && !FunctionPublish_getNArg(pstate,n-1,LUXI_CLASS_INT,(void*)&texchannel))
      return FunctionPublish_returnError(pstate,"1 int required");
    switch(info.vtype) {
    case VERTEX_32_NRM:
      texchannel %= 1;
    case VERTEX_32_TEX2:
      texchannel %= 2;
      f = info.vert32[index].tex;
      break;
    case VERTEX_64_TEX4:
    case VERTEX_64_SKIN:
      texchannel %= 4;
      f = info.vert64[index].tex;
      break;
    default:
      return 0;
    }
    f += 2*texchannel;
    if (n<4)
      return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR2(f));
    if (2!=FunctionPublish_getArgOffset(pstate,2,2,FNPUB_TOVECTOR2(f)))
      return FunctionPublish_returnError(pstate,"2 floats required");
    break;
  case VA_COLOR:
    switch(info.vtype) {
    default:
      return 0;
    case VERTEX_32_TEX2:
    case VERTEX_32_NRM:
      ub = info.vert32[index].color;
      break;
    case VERTEX_16_CLR:
      ub = info.vert16[index].color;
      break;
    case VERTEX_64_TEX4:
    case VERTEX_64_SKIN:
      ub = info.vert64[index].color;
      break;
    }
    if (n==2){
      lxVector4float_FROM_ubyte(vector,ub);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vector));
    }
    if (4!=FunctionPublish_getArgOffset(pstate,2,4,FNPUB_TOVECTOR4(vector)))
      return FunctionPublish_returnError(pstate,"4 floats required");
    lxVector4ubyte_FROM_float(ub,vector);
    break;
    /*
  case VA_USER4:
    switch(info.vtype) {
    default:
      return 0;
    case VERTEX_64_TEX4:
      f = info.vert64[index].user4;
      break;
    }
    if (n==2){
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(f));
    }
    if (4!=FunctionPublish_getArgOffset(pstate,2,4,FNPUB_TOVECTOR4(f)))
      return FunctionPublish_returnError(pstate,"4 floats required");
    break;
    */
  case VA_TRANSFORM:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,f))
      return FunctionPublish_returnError(pstate,"matrix4x4 required");

    VertexArray_transform(info.verts,info.vtype,info.vcountalloc,f);
    break;
  case VA_BOUNDS:
    VertexArray_minmax(info.verts,info.vtype,info.vcountalloc,vector,vector2);
    return FunctionPublish_setRet(pstate,6,FNPUB_FROMVECTOR3(vector),FNPUB_FROMVECTOR3(vector2));
    break;
  case VA_TEXTRANSFORM:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,posptr))
      return FunctionPublish_returnError(pstate,"matrix4x4 required");
    texchannel = 0;
    if ( (n==3) && !FunctionPublish_getNArg(pstate,n-1,LUXI_CLASS_INT,(void*)&texchannel))
      return FunctionPublish_returnError(pstate,"1 int required");

    switch(info.vtype) {
    case VERTEX_32_NRM:
      texchannel %= 1;
    case VERTEX_32_TEX2:
      texchannel %= 2;
      f = info.vert32[0].tex;
      break;
    case VERTEX_64_TEX4:
    case VERTEX_64_SKIN:
      texchannel %= 4;
      f = info.vert64[0].tex;
      break;
    default:
      return 0;
    }
    f += 2*texchannel;

    index = VertexSize(info.vtype)/sizeof(float);
    // index is stride of vertex in floats
    for (i = 0; i < info.vcountalloc; i++,f+=index){
      lxVector2Transform1(f,posptr);
    }
    break;
  case VA_POS2TEX:
    texchannel = 0;
    if ( (n==2 || n == 5) && !FunctionPublish_getNArg(pstate,n-1,LUXI_CLASS_INT,(void*)&texchannel))
      return FunctionPublish_returnError(pstate,"1 int required");

    if (n > 3 && 3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(vector)))
      return FunctionPublish_returnError(pstate,"3 float required");

    switch(info.vtype) {
    case VERTEX_32_NRM:
      texchannel %= 1;
    case VERTEX_32_TEX2:
      texchannel %= 2;
      f = info.vert32[0].tex;
      posptr = info.vert32[0].pos;
      break;
    case VERTEX_64_SKIN:
      texchannel %= 2;
    case VERTEX_64_TEX4:
      texchannel %= 4;
      f = info.vert64[0].tex;
      posptr = info.vert64[0].pos;
      break;
    default:
      return 0;
    }
    f += 2*texchannel;

    index = VertexSize(info.vtype)/sizeof(float);
    // index is stride of vertex in floats
    if (n > 4){
      lxMatrix44PlaneProjection(mat,vector);
      for (i = 0; i < info.vcountalloc; i++,posptr+=index,f+=index){
        lxVector3Transform(vector2,posptr,mat);
        lxVector2Copy(f,vector2);
      }
    }
    else
      for (i = 0; i < info.vcountalloc; i++,posptr+=index,f+=index){
        lxVector2Copy(f,posptr);
      }
    break;
  case VA_TEXINV:
    texchannel = 0;
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&index))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    if ( n==3 && !FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&texchannel))
      return FunctionPublish_returnError(pstate,"1 int required");

    switch(info.vtype) {
    case VERTEX_32_NRM:
      texchannel %= 1;
    case VERTEX_32_TEX2:
      texchannel %= 2;
      f = info.vert32[0].tex;
      break;
    case VERTEX_64_SKIN:
      texchannel %= 2;
    case VERTEX_64_TEX4:
      texchannel %= 4;
      f = info.vert64[0].tex;
      break;
    default:
      return 0;
    }
    f += 2*texchannel;
    f += index;

    index = VertexSize(info.vtype)/sizeof(float);
    // index is stride of vertex in floats
    for (i = 0; i < info.vcountalloc; i++,f+=index){
      *f= 1.0-(*f);
    }
    break;
  case VA_VALLOC:
    return FunctionPublish_returnInt(pstate,info.vcountalloc);
  case VA_VCOUNT:
    if (n==1){
      if (info.vcountptr)
        info.vcountalloc = *info.vcountptr;
      return FunctionPublish_returnInt(pstate,info.vcountalloc);
    }
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&info.vcountalloc) || !info.vcountptr)
      return FunctionPublish_returnError(pstate,"1 int required and no l3dmodel");
    *info.vcountptr = info.vcountalloc;
    break;
  case VA_POSARRAY:
    {
      switch(info.vtype) {
      case VERTEX_32_NRM:
      case VERTEX_32_TEX2:
        posptr = info.vert32[0].pos;
        break;
      case VERTEX_16_CLR:
        posptr = info.vert16[0].pos;
        break;
      case VERTEX_64_SKIN:
      case VERTEX_64_TEX4:
        posptr = info.vert64[0].pos;
        break;
      default:
        return 0;
      }

      // index = stride
      index = VertexSize(info.vtype)/sizeof(float);
      if (n==1){
        starr = StaticArray_new(info.vcountalloc*3,LUXI_CLASS_FLOATARRAY,NULL,NULL);
        f = starr->floats;
        for (i = 0; i < info.vcountalloc; i++,posptr+=index,f+=3){
          lxVector3Copy(f,posptr);
        }
        return StaticArray_return(pstate,starr);
      }
      if (!StaticArray_getNArg(pstate,1,&starr) || starr->count/3 != info.vcountalloc)
        return FunctionPublish_returnError(pstate,"1 floatarray (size= 3*vcountalloc) required");

      f = starr->floats;
      for (i = 0; i < info.vcountalloc; i++,posptr+=index,f+=3){
        lxVector3Copy(posptr,f);
      }
    }
  case VA_COLARRAY:
    {
      switch(info.vtype) {
      case VERTEX_32_NRM:
      case VERTEX_32_TEX2:
        ub = info.vert32[0].color;
        break;
      case VERTEX_16_CLR:
        ub = info.vert16[0].color;
        break;
      case VERTEX_64_SKIN:
      case VERTEX_64_TEX4:
        ub = info.vert64[0].color;
        break;
      default:
        return 0;
      }

      // index = stride
      index = VertexSize(info.vtype)/sizeof(uchar);
      if (n==1){
        starr = StaticArray_new(info.vcountalloc*4,LUXI_CLASS_FLOATARRAY,NULL,NULL);
        f = starr->floats;
        for (i = 0; i < info.vcountalloc; i++,ub+=index,f+=4){
          lxVector4float_FROM_ubyte(f,ub);
        }
        return StaticArray_return(pstate,starr);
      }
      if (!StaticArray_getNArg(pstate,1,&starr) || starr->count/4 != info.vcountalloc)
        return FunctionPublish_returnError(pstate,"1 floatarray (size= 4*vcountalloc) required");

      f = starr->floats;
      for (i = 0; i < info.vcountalloc; i++,ub+=index,f+=4){
        lxVector4ubyte_FROM_float(ub,f);
      }
    }
  case VA_NORMALARRAY:
    {
      switch(info.vtype) {
      case VERTEX_32_NRM:
      case VERTEX_64_TEX4:
      case VERTEX_64_SKIN:
        break;
      default:
        return FunctionPublish_returnError(pstate,"vertextype supports no normals");
      }

      // index = stride
      if (n==1){
        starr = StaticArray_new(info.vcountalloc*3,LUXI_CLASS_FLOATARRAY,NULL,NULL);
        f = starr->floats;
        if (info.vtype == VERTEX_64_TEX4 || info.vtype == VERTEX_64_SKIN)
          for (i = 0; i < info.vcountalloc; i++,info.vert64++,f+=3){
            lxVector3float_FROM_short(f,info.vert64->normal);
          }
        else
          for (i = 0; i < info.vcountalloc; i++,info.vert32++,f+=3){
            lxVector3float_FROM_short(f,info.vert32->normal);
          }

        return StaticArray_return(pstate,starr);
      }
      if (!StaticArray_getNArg(pstate,1,&starr) || starr->count/3 != info.vcountalloc)
        return FunctionPublish_returnError(pstate,"1 floatarray (size= 3*vcountalloc) required");

      f = starr->floats;
      if (info.vtype == VERTEX_64_TEX4 || info.vtype == VERTEX_64_SKIN)
        for (i = 0; i < info.vcountalloc; i++,info.vert64++,f+=3){
          lxVector3short_FROM_float(info.vert64->normal,f);
        }
      else
        for (i = 0; i < info.vcountalloc; i++,info.vert32++,f+=3){
          lxVector3short_FROM_float(info.vert32->normal,f);
        }
    }
    break;
  case VA_SUBMITGL:
    if (info.meshtype==MESH_VBO && info.bo && info.verts){
      int from = -1;
      int cnt = -1;
      FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_INT,(void*)&from, LUXI_CLASS_INT, (void*)&cnt);

      Mesh_submitVBO(info.mesh,LUX_FALSE,from,cnt);
      return FunctionPublish_returnBool(pstate,LUX_TRUE);
    }
    else return FunctionPublish_returnBool(pstate,LUX_FALSE);
    break;
  case VA_RETRIEVEGL:
    if (info.meshtype==MESH_VBO && info.bo && info.verts){
      int from = -1;
      int to = -1;
      FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_INT,(void*)&from, LUXI_CLASS_INT, (void*)&to);

      Mesh_retrieveVBO(info.mesh,LUX_FALSE,from,to);
      return FunctionPublish_returnBool(pstate,LUX_TRUE);
    }
    else return FunctionPublish_returnBool(pstate,LUX_FALSE);
    break;
  case VA_PTRSTATE:
    if (info.meshtype==MESH_VBO){
      booln hasvertex,hasindex;
      Mesh_getPtrsAllocState(info.mesh,&hasvertex,&hasindex);
      if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&hasvertex)){
        Mesh_setPtrsAllocState(info.mesh,hasvertex,hasindex);
        return 0;
      }
      else{
        return FunctionPublish_returnBool(pstate,hasvertex);
      }
    }
    break;
  case VA_TYPE:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_VERTEXTYPE,(void*)info.vtype);
    break;
  case VA_MESHTYPE:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_MESHTYPE,(void*)info.meshtype);
  case VA_VIDBUFFER:
    if (info.meshtype == MESH_VBO){
      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_VIDBUFFER,info.mesh->vid.vbo->host, LUXI_CLASS_INT,(void*)(info.mesh->vid.vbooffset*VertexValue(info.vtype,VERTEX_SIZE)));
    }
    else{
      return 0;
    }
    break;
  case VA_UVTRIS:
    texchannel = 0;
    if (n < 6 || 5>FunctionPublish_getArgOffset(pstate,1,6,LUXI_CLASS_INT,(void*)&a,LUXI_CLASS_INT,(void*)&b,LUXI_CLASS_INT,(void*)&c,FNPUB_TOVECTOR2(vector),LUXI_CLASS_INT,(void*)&texchannel))
      return FunctionPublish_returnError(pstate,"1 vertexarray 3 int 2 float required");

    if (a >= info.vcountalloc || b >= info.vcountalloc || c >= info.vcountalloc)
      return FunctionPublish_returnError(pstate,"index out of bounds");

    switch(info.vtype) {
    case VERTEX_32_NRM:
      texchannel %= 1;
    case VERTEX_32_TEX2:
      texchannel %= 2;
      f = info.vert32[0].tex;
      break;
    case VERTEX_64_SKIN:
      texchannel %= 2;
    case VERTEX_64_TEX4:
      texchannel %= 4;
      f = info.vert64[0].tex;
      break;
    default:
      return 0;
    }
    f += 2*texchannel;
    index = VertexSize(info.vtype)/sizeof(float);
    base = f+(index*b);
    posptr = f+(index*c);
    f += (index*a);

    vector[2] = 1.0f-vector[0]-vector[1];
    vector2[0] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],f[0],base[0],posptr[0]);
    vector2[1] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],f[1],base[1],posptr[1]);

    return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR2(vector2));

    break;
  case VA_POSTRIS:
    if (n < 6 || 5>FunctionPublish_getArgOffset(pstate,1,5,LUXI_CLASS_INT,(void*)&a,LUXI_CLASS_INT,(void*)&b,LUXI_CLASS_INT,(void*)&c,FNPUB_TOVECTOR2(vector)))
      return FunctionPublish_returnError(pstate,"1 vertexarray 3 int 2 float required");

    if (a >= info.vcountalloc || b >= info.vcountalloc || c >= info.vcountalloc)
      return FunctionPublish_returnError(pstate,"index out of bounds");

    switch(info.vtype) {
    case VERTEX_32_NRM:
    case VERTEX_32_TEX2:
      f = info.vert32[0].pos;
      break;
    case VERTEX_64_TEX4:
    case VERTEX_64_SKIN:
      f = info.vert64[0].pos;
      break;
    default:
      return 0;
    }
    index = VertexSize(info.vtype)/sizeof(float);
    base = f+(index*b);
    posptr = f+(index*c);
    f += (index*a);

    vector[2] = 1-vector[0]-vector[1];
    vector2[0] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],f[0],base[0],posptr[0]);
    vector2[1] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],f[1],base[1],posptr[1]);
    vector2[2] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],f[2],base[2],posptr[2]);

    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector2));
    break;
  case VA_NORMALTRIS:
    switch(info.vtype) {
    case VERTEX_32_NRM:
    case VERTEX_64_TEX4:
    case VERTEX_64_SKIN:
      break;
    default:
      return FunctionPublish_returnError(pstate,"vertextype supports no normals");
    }
    if (n < 6 || 5>FunctionPublish_getArgOffset(pstate,1,5,LUXI_CLASS_INT,(void*)&a,LUXI_CLASS_INT,(void*)&b,LUXI_CLASS_INT,(void*)&c,FNPUB_TOVECTOR2(vector)))
      return FunctionPublish_returnError(pstate,"1 vertexarray 3 int 2 float required");

    if (a >= info.vcountalloc || b >= info.vcountalloc || c >= info.vcountalloc)
      return FunctionPublish_returnError(pstate,"index out of bounds");

    pNormal =( info.vtype == VERTEX_32_NRM) ? info.vert32[0].normal : info.vert64[0].normal;
    index = VertexSize(info.vtype)/sizeof(short);
    pNormal1 = pNormal+(index*b);
    pNormal2 = pNormal+(index*c);
    pNormal += (index*a);

    lxVector3float_FROM_short(vector1,pNormal);
    lxVector3float_FROM_short(vector2,pNormal1);
    lxVector3float_FROM_short(vector3,pNormal2);

    vector[2] = 1-vector[0]-vector[1];
    vector4[0] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],vector1[0],vector2[0],vector3[0]);
    vector4[1] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],vector1[1],vector2[1],vector3[1]);
    vector4[2] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],vector1[2],vector2[2],vector3[2]);
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector4));

    break;
  case VA_COLORTRIS:
    if (n < 6 || 5>FunctionPublish_getArgOffset(pstate,1,5,LUXI_CLASS_INT,(void*)&a,LUXI_CLASS_INT,(void*)&b,LUXI_CLASS_INT,(void*)&c,FNPUB_TOVECTOR2(vector)))
      return FunctionPublish_returnError(pstate,"1 vertexarray 3 int 2 float required");

    if (a >= info.vcountalloc || b >= info.vcountalloc || c >= info.vcountalloc)
      return FunctionPublish_returnError(pstate,"index out of bounds");

    ub =( info.vtype == VERTEX_64_TEX4 || info.vtype == VERTEX_64_SKIN) ?  info.vert64[0].color : info.vert32[0].color;
    index = VertexSize(info.vtype)/sizeof(uchar);
    ub1 = ub+(index*b);
    ub2 = ub+(index*c);
    ub += (index*a);

    lxVector4float_FROM_ubyte(vector1,ub);
    lxVector4float_FROM_ubyte(vector2,ub1);
    lxVector4float_FROM_ubyte(vector3,ub2);

    vector[2] = 1-vector[0]-vector[1];
    vector4[0] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],vector1[0],vector2[0],vector3[0]);
    vector4[1] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],vector1[1],vector2[1],vector3[1]);
    vector4[2] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],vector1[2],vector2[2],vector3[2]);
    vector4[3] = LUX_BARYCENTRIC(vector[2],vector[0],vector[1],vector1[3],vector2[3],vector3[3]);
    return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vector4));

    break;
  case VA_COPY:
    // a = fromstart
    // b = size
    {
      posptr=NULL;
      if (errorstring = VAMeshInfo_init(pstate,&infofrom,2))
        return FunctionPublish_returnError(pstate,errorstring);
      if (n < 5 || 2>FunctionPublish_getArgOffset(pstate,3,3,LUXI_CLASS_INT,(void*)&a,LUXI_CLASS_INT,(void*)&b,LUXI_CLASS_MATRIX44,(void*)&ref) ||
        (n==6 && !Reference_get(ref,posptr)))
        return FunctionPublish_returnError(pstate,"1 vertexarray 1 int 1 vertexarray 2 int [1 matrix4x4] required");
      if (index+b > info.vcountalloc || a+b > infofrom.vcountalloc)
        return FunctionPublish_returnError(pstate,"index+size out of bounds");


      ub = (uchar*)info.verts;
      ub+= (VertexSize(info.vtype)*index);

      if (VertexSize(info.vtype) == VertexSize(infofrom.vtype)){
        memcpy(VertexArrayPtr(info.verts,index,info.vtype,VERTEX_START),VertexArrayPtr(infofrom.verts,a,infofrom.vtype,VERTEX_START),VertexSize(info.vtype)*b);
        if (info.vtype != infofrom.vtype){
          // vertex32 has same size but may have 2 texcoords or 1 normal
          if (info.vtype == VERTEX_32_NRM){
            // means we have illegal normals
            info.vert32+=index;
            for (i=0; i < b; i++,info.vert32++){
              LUX_ARRAY4SET(info.vert32->normal,0,0,1,1);
            }
          }
          else{
            // means we have illegal texcoord2
            info.vert32+=index;
            for (i=0; i < b; i++,info.vert32++){
              LUX_ARRAY2COPY(info.vert32->tex1,info.vert32->tex);
            }
          }
        }
      }
      else{
        if (info.vtype == VERTEX_64_TEX4 || info.vtype == VERTEX_64_SKIN){
          info.vert64 += index;
          infofrom.vert32 += a;
          if (infofrom.vtype == VERTEX_32_NRM){
            for (i=0; i < b; i++,infofrom.vert32++,info.vert64++){
              lxVector2Copy(info.vert64->tex,infofrom.vert32->tex);
              lxVector2Copy(info.vert64->tex1,infofrom.vert32->tex);
              lxVector4Clear(info.vert64->tex2);
              LUX_ARRAY4COPY(info.vert64->normal,infofrom.vert32->normal);
              LUX_ARRAY4CLEAR(info.vert64->tangent);
              lxVector4Copy(info.vert64->pos,infofrom.vert32->pos);
            }
          }
          else{
            for (i=0; i < b; i++,infofrom.vert32++,info.vert64++){
              lxVector4Copy(info.vert64->tex,infofrom.vert32->tex);
              lxVector4Clear(info.vert64->tex2);
              LUX_ARRAY4SET(info.vert64->normal,0,0,1,1);
              LUX_ARRAY4CLEAR(info.vert64->tangent);
              lxVector4Copy(info.vert64->pos,infofrom.vert32->pos);
            }
          }
        }
        else{
          info.vert32 += index;
          infofrom.vert64 += a;
          if (info.vtype == VERTEX_32_NRM){
            for (i=0; i < b; i++,infofrom.vert64++,info.vert32++){
              lxVector2Copy(info.vert32->tex,infofrom.vert64->tex);
              LUX_ARRAY4COPY(info.vert32->normal,infofrom.vert32->normal);
              lxVector4Copy(info.vert32->pos,infofrom.vert64->pos);
            }
          }
          else{
            for (i=0; i < b; i++,infofrom.vert64++,info.vert32++){
              lxVector4Copy(info.vert32->tex,infofrom.vert64->tex);
              lxVector4Copy(info.vert32->pos,infofrom.vert64->pos);
            }
          }
        }
      }
      if (posptr)
        VertexArray_transform((void*)ub,info.vtype,b,posptr);
    }
    break;
  default:
    break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_INDEXARRAY

enum IndexArrayProp_e{
  IA_VALUE,
  IA_PRIM,
  IA_COPY,

  IA_FUNC_SEPARATOR,

  IA_PRIMTYPE,
  IA_ICOUNT,
  IA_IALLOC,
  IA_PRIMCOUNT,
  IA_TRISCOUNT,
  IA_MINMAX,

  IA_MESHTYPE,
  IA_VIDBUFFER,
  IA_TYPE,
  IA_PTRS,

  IA_SUBMITGL,
  IA_RETRIEVEGL,
  IA_PTRSTATE,
};

static char*  IAMesh_init(PState pstate,Mesh_t **meshptr, int stack)
{

  Mesh_t  *mesh;
  Reference ref;


  if (!FunctionPublish_getNArg(pstate,stack,LUXI_CLASS_INDEXARRAY,(void*)&ref))
    return "1 indexarray required";

  switch(FunctionPublish_getNArgType(pstate,stack)) {
  case LUXI_CLASS_INDEXARRAY:
  {
    MeshArrayHandle_t *vahandle;
    int index;
    List3DNode_t  *l3d;

    if (!Reference_get(ref,vahandle) || !Reference_get(vahandle->srcref,l3d))
      return "1 vertexarray from valid l3dmodel required";

    index = vahandle->meshid;
    SUBRESID_MAKEPURE(index);
    mesh = l3d->drawNodes[index].draw.mesh;

  }
    break;

  case LUXI_CLASS_L2D_IMAGE:
  {
    List2DNode_t  *l2d;

    if (!Reference_get(ref,l2d) || l2d->l2dimage->mesh == g_VID.drw_meshquad || l2d->l2dimage->mesh == g_VID.drw_meshquadcentered)
      return "1 l2dimage with user mesh required";

    mesh = l2d->l2dimage->mesh;
  }
    break;
  case LUXI_CLASS_L3D_PRIMITIVE:
  {
    List3DNode_t  *l3d;

    if (!Reference_get(ref,l3d) || l3d->drawNodes->draw.mesh == l3d->primitive->orgmesh)
      return "1 l3dprimitive with user mesh required";

    mesh = l3d->drawNodes->draw.mesh;
  }
    break;
  case LUXI_CLASS_RCMD_DRAWMESH:
  {
    RenderCmdMesh_t *ffx;

    if (!Reference_get(ref,ffx) || ffx->dnode.mesh == g_VID.drw_meshquadffx)
      return "1 rcmdmesh with user mesh required";

    mesh = ffx->dnode.mesh;
  }
    break;
  case LUXI_CLASS_MESHID:
  {
    Model_t *model;
    int index;

    index = *(int*)&ref;
    if ((model = ResData_getModel(SUBRESID_GETRES(index)))==NULL)
      return "model is invalid";
    SUBRESID_MAKEPURE(index);
    mesh = model->meshObjects[index].mesh;
  }
    break;
  case LUXI_CLASS_RENDERMESH:
  {
    RenderMesh_t *rmesh;
    if (!Reference_get(ref,rmesh))
      return "1 rendermesh required";

    mesh = rmesh->mesh;
  }
  break;
  default:
    return "unknown indexarray type";
  }

  *meshptr = mesh;
  return NULL;
}

static int PubIndexArray_prop (PState pstate,PubFunction_t *fn, int n)
{
  int index;
  int data;
  int i;
  ushort *primstart;
  ushort *primstartfrom;
  uint *primstartInt;
  uint *primstartfromInt;
  int indices[4];
  Mesh_t  *mesh;
  Mesh_t  *meshfrom;
  char *errorstring;
  int intindices;

  if (errorstring=IAMesh_init(pstate,&mesh,0))
    return FunctionPublish_returnError(pstate,errorstring);

  intindices = !mesh->index16;

  if ((int)fn->upvalue < IA_FUNC_SEPARATOR && (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&index) || index >= mesh->numAllocIndices || !(mesh->indicesData)))
    return FunctionPublish_returnError(pstate,"index required or index out of bounds");


  switch((int)fn->upvalue) {
  case IA_VALUE:
    if (n==2) return FunctionPublish_returnInt(pstate,intindices ? mesh->indicesData32[index] : mesh->indicesData16[index]);
    if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&data) || data<0 || data > mesh->numAllocVertices)
      return  FunctionPublish_returnError(pstate,"1 indexarray 1 int 1 int 0-allocated vertices required");
    if (intindices)   mesh->indicesData32[index] = data;
    else        mesh->indicesData16[index] = data;
    break;
  case IA_PTRS:
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_POINTER,(intindices ? (void*)mesh->indicesData32 : (void*)mesh->indicesData16),
      LUXI_CLASS_POINTER, (intindices ? (void*)(mesh->indicesData32+mesh->numAllocIndices) : (void*)(mesh->indicesData16+mesh->numAllocIndices)));
  case IA_PRIM:

    if(!mesh->index16){
      // set index to -1 if we cannot write into array
      switch(mesh->primtype) {
      case PRIM_POINTS:
        primstartInt = &mesh->indicesData32[index];
        data =1; break;
        break;
      case PRIM_LINES:
        primstartInt = &mesh->indicesData32[index*2];
        data =2; break;
      case PRIM_LINE_LOOP:
        primstartInt = &mesh->indicesData32[index];
        if (index == mesh->numIndices-1 && n==2)
          return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,primstartInt[0],LUXI_CLASS_INT,mesh->indicesData32[0]);
        index = -1;
        data =2; break;
      case PRIM_LINE_STRIP:
        primstartInt = &mesh->indicesData32[index];
        index = -1;
        data =2; break;
      case PRIM_TRIANGLES:
        primstartInt = &mesh->indicesData32[index*3];
        data =3; break;
      case PRIM_TRIANGLE_STRIP:
        primstartInt = &mesh->indicesData32[index];
        index = -1;
        data =3; break;
      case PRIM_TRIANGLE_FAN:
        primstartInt = &mesh->indicesData32[index+1];
        if (n==2){
          return FunctionPublish_setRet(pstate,3,LUXI_CLASS_INT,mesh->indicesData32[0],LUXI_CLASS_INT,primstartInt[0],LUXI_CLASS_INT,primstartInt[1]);
        }
        index = -1;
        data =3; break;
      case PRIM_QUADS:
        primstartInt = &mesh->indicesData32[index*4];
        data =4; break;
      case PRIM_QUAD_STRIP:
        primstartInt = &mesh->indicesData32[index*2];
        index = -1;
        data =4; break;
      case PRIM_POLYGON:
        return FunctionPublish_returnError(pstate,"polygon not supported in this function, all indices within array make one polygon");
      default:
      return 0;
      }
      if (n==2){
        for (i = 0; i < data; i++){
          FunctionPublish_setNRet(pstate,i,LUXI_CLASS_INT,(void*)primstartInt[i]);
        }
        return data;
      }
      if (index >= 0){
        index = 0;
        if (n+1<(int)data) return FunctionPublish_returnError(pstate,"unsufficent indices for this primitivetype given");
        for (i = 0; i < data; i++){
          if (!FunctionPublish_getNArg(pstate,i+2,LUXI_CLASS_INT,(void*)&indices[i]))
            return FunctionPublish_returnErrorf(pstate,"Parameter %i could not be casted to int",i);
        }
        for (i = 0; i < data; i++){
          primstartInt[i] = indices[i];
        }
      }
      else
        return FunctionPublish_returnError(pstate,"illegal primitivetype for write operation");
    }
    else{
      // set index to -1 if we cannot write into array
      switch(mesh->primtype) {
      case PRIM_POINTS:
        primstart = &mesh->indicesData16[index];
        data =1; break;
        break;
      case PRIM_LINES:
        primstart = &mesh->indicesData16[index*2];
        data =2; break;
      case PRIM_LINE_LOOP:
        primstart = &mesh->indicesData16[index];
        if (index == mesh->numIndices-1 && n==2)
          return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,primstart[0],LUXI_CLASS_INT,mesh->indicesData16[0]);
        index = -1;
        data =2; break;
      case PRIM_LINE_STRIP:
        primstart = &mesh->indicesData16[index];
        index = -1;
        data =2; break;
      case PRIM_TRIANGLES:
        primstart = &mesh->indicesData16[index*3];
        data =3; break;
      case PRIM_TRIANGLE_STRIP:
        primstart = &mesh->indicesData16[index];
        index = -1;
        data =3; break;
      case PRIM_TRIANGLE_FAN:
        primstart = &mesh->indicesData16[index+1];
        if (n==2){
          return FunctionPublish_setRet(pstate,3,LUXI_CLASS_INT,mesh->indicesData16[0],LUXI_CLASS_INT,primstart[0],LUXI_CLASS_INT,primstart[1]);
        }
        index = -1;
        data =3; break;
      case PRIM_QUADS:
        primstart = &mesh->indicesData16[index*4];
        data =4; break;
      case PRIM_QUAD_STRIP:
        primstart = &mesh->indicesData16[index*2];
        index = -1;
        data =4; break;
      case PRIM_POLYGON:
        return FunctionPublish_returnError(pstate,"polygon not supported in this function, all indices within array make one polygon");
      default:
        return 0;
      }
      if (n==2){
        for (i = 0; i < data; i++){
          FunctionPublish_setNRet(pstate,i,LUXI_CLASS_INT,(void*)primstart[i]);
        }
        return data;
      }
      if (index >= 0){
        index = 0;
        if (n+1<(int)data) return FunctionPublish_returnError(pstate,"unsufficent indices for this primitivetype given");
        for (i = 0; i < data; i++){
          if (!FunctionPublish_getNArg(pstate,i+2,LUXI_CLASS_INT,(void*)&indices[i]))
            return FunctionPublish_returnErrorf(pstate,"Parameter %i could not be casted to int",i);
        }
        for (i = 0; i < data; i++){
          primstart[i] = indices[i];
        }
      }
      else
        return FunctionPublish_returnError(pstate,"illegal primitivetype for write operation");
    }
    break;
  case IA_PRIMTYPE:
    if (n==1) return FunctionPublish_returnType(pstate,LUXI_CLASS_PRIMITIVETYPE,(void*)mesh->primtype);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_PRIMITIVETYPE,(void*)&data))
      return FunctionPublish_returnError(pstate,"1 indexarray 1 indexprimitivetype required");
    mesh->primtype = data;
    break;
  case IA_ICOUNT:
    if (n==1) return FunctionPublish_returnInt(pstate,mesh->numIndices);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&data) || data < 0 || data > mesh->numAllocIndices)
      return FunctionPublish_returnError(pstate,"1 indexarray 1 int 0-allocated indices required");
    mesh->numIndices = data;
    break;
  case IA_IALLOC:
    return FunctionPublish_returnInt(pstate,mesh->numAllocIndices);
    break;
  case IA_PRIMCOUNT:
    return FunctionPublish_returnInt(pstate,Mesh_getPrimCnt(mesh));
    break;
  case IA_TRISCOUNT:
    Mesh_updateTrisCnt(mesh);
    return FunctionPublish_returnInt(pstate,mesh->numTris);
    break;
  case IA_MINMAX:
    if (n==1){
      Mesh_updateMinMax(mesh);

      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)mesh->indexMin,LUXI_CLASS_INT,(void*)mesh->indexMax);
    }
    else if(FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_INT,(void*)&index,LUXI_CLASS_INT,(void*)&data)<2 || index >= mesh->numAllocIndices || data >= mesh->numAllocIndices || index > data)
      return FunctionPublish_returnError(pstate,"indices out of bounds, or min > max");
        mesh->indexMin = index;
    mesh->indexMax = data;
    return 0;
  case IA_SUBMITGL:
    if (mesh->meshtype==MESH_VBO && mesh->vid.ibo && (mesh->indicesData16 || mesh->indicesData32)){
      int from = -1;
      int cnt = -1;
      FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_INT,(void*)&from, LUXI_CLASS_INT, (void*)&cnt);

      Mesh_submitVBO(mesh,LUX_TRUE,from,cnt);
      return FunctionPublish_returnBool(pstate,LUX_TRUE);
    }
    else return FunctionPublish_returnBool(pstate,LUX_FALSE);
    break;
  case IA_RETRIEVEGL:
    if (mesh->meshtype==MESH_VBO && mesh->vid.ibo && (mesh->indicesData16 || mesh->indicesData32)){
      int from = -1;
      int to = -1;
      FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_INT,(void*)&from, LUXI_CLASS_INT, (void*)&to);

      Mesh_retrieveVBO(mesh,LUX_TRUE,from,to);
      return FunctionPublish_returnBool(pstate,LUX_TRUE);
    }
    else return FunctionPublish_returnBool(pstate,LUX_FALSE);
    break;
  case IA_PTRSTATE:
    if (mesh->meshtype == MESH_VBO){
      booln hasvertex,hasindex;
      Mesh_getPtrsAllocState(mesh,&hasvertex,&hasindex);
      if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&hasindex)){
        Mesh_setPtrsAllocState(mesh,hasvertex,hasindex);
        return 0;
      }
      else{
        return FunctionPublish_returnBool(pstate,hasindex);
      }
    }
    break;
  case IA_MESHTYPE:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_MESHTYPE,(void*)mesh->meshtype);
  case IA_VIDBUFFER:
    if (mesh->meshtype == MESH_VBO){
      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_VIDBUFFER,mesh->vid.ibo->host,LUXI_CLASS_INT,(void*)mesh->vid.ibooffset);
    }
    else{
      return 0;
    }
    break;
  case IA_TYPE:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_SCALARTYPE,(void*)(mesh->indicesData16 ? LUX_SCALAR_UINT16 : LUX_SCALAR_UINT32));

  case IA_COPY:
    {
      indices[1] = 0;
      if (errorstring = IAMesh_init(pstate,&meshfrom,2))
        return FunctionPublish_returnError(pstate,errorstring);
      if (n < 5 || 2>FunctionPublish_getArgOffset(pstate,3,3,LUXI_CLASS_INT,(void*)&data,LUXI_CLASS_INT,(void*)&indices[0],LUXI_CLASS_INT,(void*)&indices[1]))
        return FunctionPublish_returnError(pstate,"1 indexarray 1 int 1 indexarray 2 int [1 int] required");
      if (index + indices[0] > mesh->numAllocIndices || data + indices[0] > meshfrom->numAllocIndices)
        return FunctionPublish_returnError(pstate,"index + size out of bounds");


      if (mesh->index16 == meshfrom->index16){
        if (n==6){
          if (!meshfrom->index16){
            primstartInt = &mesh->indicesData32[index];
            primstartfromInt = &meshfrom->indicesData32[data];
            for (i=0; i < indices[0]; i++,primstartInt++,primstartfromInt++){
              *primstartInt = *primstartfromInt + indices[1];
            }
          }
          else{
            primstart = &mesh->indicesData16[index];
            primstartfrom = &meshfrom->indicesData16[data];
            for (i=0; i < indices[0]; i++,primstart++,primstartfrom++){
              *primstart = *primstartfrom + indices[1];
            }
          }
        }
        else{
          if(!meshfrom->index16)
            memcpy(&mesh->indicesData32[index],&meshfrom->indicesData32[data],sizeof(uint)*indices[0]);
          else
            memcpy(&mesh->indicesData16[index],&meshfrom->indicesData16[data],sizeof(ushort)*indices[0]);
        }
      }
      else if (!mesh->index16){
        primstartInt = &mesh->indicesData32[index];
        primstartfrom = &meshfrom->indicesData16[data];
        for (i=0; i < indices[0]; i++,primstartInt++,primstartfrom++){
          *primstartInt = *primstartfrom + indices[1];
        }
      }
      else{
        primstart = &mesh->indicesData16[index];
        primstartfromInt = &meshfrom->indicesData32[data];
        for (i=0; i < indices[0]; i++,primstart++,primstartfromInt++){
          *primstart = *primstartfromInt + indices[1];
        }
      }


    }
    return 0;
  default:
    break;
  }

  return 0;
}


//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_MORPHCONTROL

static int PubMorphControl_new(PState pstate,PubFunction_t *f, int n)
{
  ModelControl_t *mcontrol = ModelControl_new();
  mcontrol->type = (int)f->upvalue;



  return FunctionPublish_returnType(pstate,LUXI_CLASS_MORPHCONTROL,REF2VOID(mcontrol->reference));
}

static int PubMorphControl_del(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  ModelControl_t *mcontrol;

  if (n!=1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MORPHCONTROL,(void*)&ref) || !Reference_get(ref,mcontrol))
    return FunctionPublish_returnError(pstate,"1 morphcontrol required");
  Reference_free(ref);//RModelControl_free(ref);
  return 0;
}

static int PubMorphControl_activate(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  Reference refl3d;
  List3DNode_t *l3d;
  ModelControl_t *mcontrol;

  if (n!=2 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_MORPHCONTROL,(void*)&ref,LUXI_CLASS_L3D_MODEL,(void*)&refl3d)!=2
    || !Reference_get(ref,mcontrol) || !Reference_get(refl3d,l3d))
    return FunctionPublish_returnError(pstate,"1 morphcontrol 1 l3dmodel required");

  if (!l3d->minst->modelupd)
    return FunctionPublish_returnError(pstate,"l3dmodel does not allow morphing");

  ModelControl_activate(mcontrol,l3d->minst->modelupd);
  return 0;
}

static int PubMorphControl_deactivate(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  ModelControl_t *mcontrol;

  if (n!=1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MORPHCONTROL,(void*)&ref) || !Reference_get(ref,mcontrol))
    return FunctionPublish_returnError(pstate,"1 bonecontrol required");

  ModelControl_deactivate(mcontrol);
  return 0;
}

enum PubMorphCmds_e
{
  PMORPH_FACTOR,
  PMORPH_MDLTO,
  PMORPH_MDLFROM,
  PMORPH_MTO,
  PMORPH_MFROM,
  PMORPH_MTARGET,
};

static int PubMorphControl_attributes(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  float factor;
  int index;
  ModelControl_t *mcontrol;

  if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MORPHCONTROL,(void*)&ref) || !Reference_get(ref,mcontrol))
    return FunctionPublish_returnError(pstate,"1 morphcontrol required");

  switch((int)f->upvalue) {
  case PMORPH_FACTOR: // morphfactor
    if (n == 2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&factor))
        return FunctionPublish_returnError(pstate,"1 morphcontrol 1 float required");
      mcontrol->morphFactor = factor;
    }
    else{
      return FunctionPublish_returnFloat(pstate,mcontrol->morphFactor);
    }
    break;
  case PMORPH_MDLFROM: // modelfrom
    if (n != 2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MODEL,(void*)&index))
        mcontrol->modelMorphFrom = MODEL_CONTROL_SELF;
      else mcontrol->modelMorphFrom = ResData_getModel(index);
    }
    else
      return FunctionPublish_returnError(pstate,"1 morphcontrol 1 model/non model required");
    break;
  case PMORPH_MDLTO: // modelto
    if (n != 2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MODEL,(void*)&index))
        mcontrol->modelMorphTo = MODEL_CONTROL_SELF;
      else mcontrol->modelMorphTo = ResData_getModel(index);
    }
    else
      return FunctionPublish_returnError(pstate,"1 morphcontrol 1 model/non model required");
    break;
  case PMORPH_MFROM:  // meshfrom
    if (n != 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MESHID,(void*)&index))
      return FunctionPublish_returnError(pstate,"1 morphcontrol 1 meshid required");
    mcontrol->meshFrom = index&0xffff;
    break;
  case PMORPH_MTO:  // meshto
    if (n != 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MESHID,(void*)&index))
      return FunctionPublish_returnError(pstate,"1 morphcontrol 1 meshid required");
    mcontrol->meshTo = index&0xffff;
    break;
  case PMORPH_MTARGET:  // meshtarget
    if (n != 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MESHID,(void*)&index))
      return FunctionPublish_returnError(pstate,"1 morphcontrol 1 meshid required");
    mcontrol->mesh = index&0xffff;
    break;
  default:
    break;
  }

  return 0;
}


//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_L3D_MODEL

static int PubL3DModel_getmodel(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;

  if (n!=1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_MODEL,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate," 1 l3dnode required");
  return FunctionPublish_returnType(pstate,LUXI_CLASS_MODEL,(void*)l3d->minst->modelRID);
}


static int PubL3DModel_new (PState pstate,PubFunction_t *f, int n)
{
  int layer;
  int model;
  booln boneupd,meshupd;
  char *name;
  List3DNode_t *l3d;

  layer = List3D_getDefaultLayer();

  boneupd = LUX_FALSE;
  meshupd = LUX_FALSE;

  if (!f->upvalue){
    if (n<2 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name) ||
      FunctionPublish_getArgOffset(pstate,FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID, (void*)&layer)+1,3,LUXI_CLASS_MODEL,(void*)&model,LUXI_CLASS_BOOLEAN,(void*)&boneupd,LUXI_CLASS_BOOLEAN,(void*)&meshupd)<1)
      return FunctionPublish_returnError(pstate," 1 string [1 l3dlayerid] 1 model [2 booleans] required");
  }
  else{
    if (n<2 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name) || !FunctionPublish_getNArg(pstate,FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID, (void*)&layer)+1,LUXI_CLASS_MODEL,(void*)&model))
      return FunctionPublish_returnError(pstate," 1 string [1 l3dlayerid] 1 model  required");
  }


  l3d = List3DNode_newModelRes(name,layer,model,boneupd,meshupd);
  if (l3d==NULL)
    return FunctionPublish_returnError(pstate,"could not create l3dnode");

  Reference_makeVolatile(l3d->reference);
  return FunctionPublish_setRet(pstate,1,LUXI_CLASS_L3D_MODEL,REF2VOID(l3d->reference));
}

static int PubL3DModel_newinstance (PState pstate,PubFunction_t *f, int n)
{
  int layer;
  char *name;
  List3DNode_t *l3d;
  List3DNode_t *l3dsource;
  Reference ref;

  layer = List3D_getDefaultLayer();

  if (n<2 || FunctionPublish_getArg(pstate,3,LUXI_CLASS_STRING,&name,LUXI_CLASS_L3D_MODEL,&ref, LUXI_CLASS_L3D_LAYERID, &layer)<2 ||
    !Reference_get(ref,l3dsource) || (l3dsource->minst->boneupd && !l3dsource->minst->boneupd->inWorld))
    return FunctionPublish_returnError(pstate," 1 string 1 l3dmodel [1 l3dlayerid] required");

  l3d = List3DNode_newModelnstance(name,layer,Modelnstance_ref(l3dsource->minst));
  if (l3d==NULL)
    return FunctionPublish_returnError(pstate,"could not create l3dnode");

  Reference_makeVolatile(l3d->reference);
  return FunctionPublish_setRet(pstate,1,LUXI_CLASS_L3D_MODEL,REF2VOID(l3d->reference));
}

static int PubL3DModel_material (PState pstate,PubFunction_t *f, int n)
{
  int material;
  Reference ref;
  int mesh;
  List3DNode_t *l3d;

  if ((n!=2 && n!=3)|| !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_MODEL,(void*)&ref) ||
             !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MESHID,(void*)&mesh) ||
             !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dmodel 1 meshid [1 matsurface] required");

  mesh&=0xffff;

  switch(n) {
  case 2:
    material = List3DNode_Model_meshMaterial(l3d,mesh,-2);
    if (vidMaterial(material))
      return FunctionPublish_returnType(pstate,LUXI_CLASS_MATERIAL,(void*)material);
    else if (material >= 0)
      return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)material);
    else
      return 0;
    break;
  case 3:
    if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_MATSURF,(void*)&material))
      return FunctionPublish_returnError(pstate,"1 l3dmodel 1 meshid [1 matsurface] required");
    List3DNode_Model_meshMaterial(l3d,mesh,material);
    break;
  default:
    break;
  }
  return 0;
}

enum PubSeqCmds_e
{
  PSEQ_STATE,
  PSEQ_SPEED,
  PSEQ_TIME,
  PSEQ_OFFSET,
  PSEQ_LOOP,
  PSEQ_BLENDMANUAL,
  PSEQ_BLEND,
  PSEQ_DURATION,

  PSEQ_ANIM,
  PSEQ_START,
  PSEQ_LEN,
};

static int PubL3DModel_sequence (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  int seq;
  int intvalue;
  float floatvalue;
  BoneSysUpdate_t *boneupd;
  BoneSequence_t *boneseq;
  List3DNode_t *l3d;

  byte state;

  if ((n<2) || (FunctionPublish_getArg(pstate,2,LUXI_CLASS_L3D_MODEL,(void*)&ref,LUXI_CLASS_INT,(void*)&seq)!=2
      || !Reference_get(ref,l3d)))
    return FunctionPublish_returnError(pstate,"1 l3dmodel 1 int required, optional float/boolean/int");

  boneupd = l3d->minst->boneupd;
  if (!boneupd)
    return FunctionPublish_returnError(pstate,"l3dmodel doesnt allow boneanimation");

  seq %= BONE_MAX_SEQ;
  boneseq = &boneupd->seq[seq];

  switch((int)f->upvalue){
  case PSEQ_STATE:  // state
    if (n==2) return FunctionPublish_returnInt(pstate,boneseq->state);
    else{
      if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&intvalue))
        return FunctionPublish_returnError(pstate,"1 l3dmodel 1 int required, optional float/boolean/int");
      boneseq->state = intvalue;
    }
    break;
  case PSEQ_SPEED: // speed
    if (n==2) return FunctionPublish_returnFloat(pstate,boneseq->animspeed);
    else{
      if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_FLOAT,(void*)&floatvalue))
        return FunctionPublish_returnError(pstate,"1 l3dmodel 1 int required, optional float/boolean/int");
      boneseq->animspeed = floatvalue;
    }
    break;
  case PSEQ_TIME: // time
    if (n==2) return FunctionPublish_returnInt(pstate,boneseq->runtime);
    else{
      if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&intvalue))
        return FunctionPublish_returnError(pstate,"1 l3dmodel 1 int required, optional float/boolean/int");
      boneseq->runtime = intvalue;
      boneseq->flag |= BONE_TIME_SET;
    }
    break;
  case PSEQ_OFFSET: // offset
    if (n==2) return FunctionPublish_returnInt(pstate,boneseq->anim1Start-boneseq->animLengths[0]);
    else{
      if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&intvalue))
        return FunctionPublish_returnError(pstate,"1 l3dmodel 1 int required, optional float/boolean/int");
      boneseq->anim1Start = boneseq->animLengths[0] + intvalue;
      boneseq->duration = boneseq->anim1Start + boneseq->animLengths[1];
    }
    break;
  case PSEQ_LOOP: // loop
    if (n==2) return FunctionPublish_returnBool(pstate,(boneseq->flag & BONE_LOOP) ? LUX_TRUE : LUX_FALSE);
    else{
      if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&state))
        return FunctionPublish_returnError(pstate,"1 l3dmodel 1 int required, optional float/boolean/int");
      if (state)
        boneseq->flag |= BONE_LOOP;
      else
        boneseq->flag &= ~BONE_LOOP;
    }
    break;
  case PSEQ_BLENDMANUAL:  // blendmanual
    if (n==2) return FunctionPublish_returnBool(pstate,(boneseq->flag & BONE_MANUAL_BLEND) ? LUX_TRUE : LUX_FALSE);
    else{
      if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&state))
        return FunctionPublish_returnError(pstate,"1 l3dmodel 1 int required, optional float/boolean/int");
      if (state)
        boneseq->flag |= BONE_MANUAL_BLEND;
      else
        boneseq->flag &= ~BONE_MANUAL_BLEND;
    }
    break;
  case PSEQ_BLEND:  // blendfactor
    if (n==2) return FunctionPublish_returnFloat(pstate,boneseq->blendfactor);
    else{
      if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_FLOAT,(void*)&floatvalue))
        return FunctionPublish_returnError(pstate,"1 l3dmodel 1 int required, optional float/boolean/int");
      boneseq->blendfactor = floatvalue;
    }
    break;
  case PSEQ_DURATION: // duration
    if (n==2) return FunctionPublish_returnInt(pstate,boneseq->duration);
    /*
    if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&intvalue))
      return FunctionPublish_returnError(pstate,"1 l3dmodel 1 int required, optional float/boolean/int");
      boneseq->duration = intvalue;*/
    break;
  default:
    break;
  }

  return 0;
}

static int PubL3DModel_sequenceanim (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  int seq;
  int anim;
  int intvalue;
  BoneSysUpdate_t *boneupd;
  BoneSequence_t *boneseq;
  List3DNode_t *l3d;

  if ((n!=3 && n!=4) || (FunctionPublish_getArg(pstate,3,LUXI_CLASS_L3D_MODEL,(void*)&ref,LUXI_CLASS_INT,(void*)&seq,LUXI_CLASS_INT,(void*)&anim)!=3
    || !Reference_get(ref,l3d)))
    return FunctionPublish_returnError(pstate,"1 l3dmodel 2 int required, optional animation/int");

  boneupd = l3d->minst->boneupd;
  if (!boneupd)
    return FunctionPublish_returnError(pstate,"l3dmodel doesnt allow boneanimation");

  seq %= BONE_MAX_SEQ;
  anim %= BONE_MAX_SEQANIMS;
  boneseq = &boneupd->seq[seq];

  switch((int)f->upvalue) {
  case PSEQ_ANIM: // anim
    if (n==3) {
      if (boneseq->anims[anim]<0) return 0;
      else return FunctionPublish_returnType(pstate,LUXI_CLASS_ANIMATION,(void*)boneseq->anims[anim]);
    }
    else{
      if (!FunctionPublish_getNArg(pstate,3,LUXI_CLASS_ANIMATION,(void*)&intvalue))
        return FunctionPublish_returnError(pstate,"1 l3dmodel 2 int required, optional animation/int");
      boneseq->anims[anim] = intvalue;
      boneseq->animflags[anim] = ResData_getAnim(intvalue)->animflag;
    }
    break;
  case PSEQ_START:  // start
    if (n==3) {
      return FunctionPublish_returnInt(pstate,boneseq->animStartTimes[anim]);
    }
    else{
      if (!FunctionPublish_getNArg(pstate,3,LUXI_CLASS_INT,(void*)&intvalue))
        return FunctionPublish_returnError(pstate,"1 l3dmodel 2 int required, optional animation/int");
      boneseq->animStartTimes[anim] = intvalue;
    }
    break;
  case PSEQ_LEN:  // length
    if (n==3) {
      return FunctionPublish_returnInt(pstate,boneseq->animLengths[anim]);
    }
    else{
      if (!FunctionPublish_getNArg(pstate,3,LUXI_CLASS_INT,(void*)&intvalue))
        return FunctionPublish_returnError(pstate,"1 l3dmodel 2 int required, optional animation/int");
      boneseq->animLengths[anim] = intvalue;
    }
    break;
  default:
    break;
  }
  return 0;
}



enum PubL3DModelFuncs_e{
  PL3DM_SEQANIMMAX,
  PL3DM_SEQMAX,

  PL3DM_ARG_FUNCS,

  PL3DM_BONERESET,
  PL3DM_MORPHRESET,
  PL3DM_NONBATCHABLE,
  PL3DM_VERTEXARRAY,
  PL3DM_INDEXARRAY,
  PL3DM_BATCHBUFFER,
  PL3DM_WORLDBONES,

  PL3DM_BONE_FUNCS,
  PL3DM_BONEMATRIX,
  PL3DM_BONEPOS,
  PL3DM_BONEROTDEG,
  PL3DM_BONEROTRAD,
  PL3DM_BONEROTAXIS,
};

static int PubL3DModel_exec (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  int data;
  char *name;
  float *bonematrix;
  float *y,*z;
  lxVector3 vec;
  MeshArrayHandle_t *vah;

  byte state;

  if ((int)f->upvalue > PL3DM_ARG_FUNCS && (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_MODEL,(void*)&ref) || !Reference_get(ref,l3d)))
    return FunctionPublish_returnError(pstate,"1 l3dmodel required");

  if ((int)f->upvalue > PL3DM_BONE_FUNCS){
    name = NULL;
    data = -1;
    switch(FunctionPublish_getNArgType(pstate,1))
    {
    case LUXI_CLASS_BONEID:
      FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BONEID,(void*)&data);
      if (SUBRESID_GETRES(data) != l3d->minst->modelRID)
        return FunctionPublish_returnError(pstate,"inappropriate boneid");
      SUBRESID_MAKEPURE(data);
      break;
    case LUXI_CLASS_STRING:
      FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&name);
      data = BoneSys_searchBone(&ResData_getModel(l3d->minst->modelRID)->bonesys,name);
      if (data < 0)
        return FunctionPublish_returnError(pstate,"bone not found");
      break;
    default:
      return FunctionPublish_returnError(pstate,"1 l3dmode 1 boneid/string required");
    }
    bonematrix = (l3d->minst->boneupd) ? &l3d->minst->boneupd->bonesAbs[data*16] : ResData_getModelBoneAbs(l3d->minst->modelRID,data,LUX_FALSE);
  }

  switch((int)f->upvalue) {
  case PL3DM_SEQANIMMAX:
    return FunctionPublish_returnInt(pstate,BONE_MAX_SEQANIMS);
  case PL3DM_SEQMAX:
    return FunctionPublish_returnInt(pstate,BONE_MAX_SEQ);
  case PL3DM_BONEMATRIX:
    return PubMatrix4x4_return(pstate,bonematrix);
  case PL3DM_BONEPOS:
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMMATRIXPOS(bonematrix));
  case PL3DM_BONEROTRAD:
    lxMatrix44ToEulerZYX(bonematrix,vec);
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vec));
  case PL3DM_BONEROTDEG:
    lxMatrix44ToEulerZYX(bonematrix,vec);
    lxVector3Rad2Deg(vec,vec);
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vec));
  case PL3DM_BONEROTAXIS:
    y = bonematrix+4;
    z = bonematrix+8;
    return FunctionPublish_setRet(pstate,9,FNPUB_FROMVECTOR3(bonematrix),FNPUB_FROMVECTOR3(y),FNPUB_FROMVECTOR3(z));
  case PL3DM_BONERESET: // bonereset
    if (l3d->minst->boneupd)
      BoneSysUpdate_reset(l3d->minst->boneupd);
    break;
  case PL3DM_MORPHRESET:  // morphreset
    if (l3d->minst->modelupd)
      ModelUpdate_reset(l3d->minst->modelupd);
    break;
  case PL3DM_WORLDBONES:
    if (!l3d->minst->boneupd)
      return FunctionPublish_returnError(pstate,"l3dmodel has no bone-support");
    if (n==1) return FunctionPublish_returnBool(pstate,l3d->minst->boneupd->inWorld);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&data))
      return FunctionPublish_returnError(pstate,"1 l3dmodel 1 boolean required");
    List3DNode_Model_setWorldBones(l3d,data);
    break;
  case PL3DM_BATCHBUFFER:
    if (l3d->drawNodes->type == DRAW_NODE_BATCH_WORLD)
      return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_BATCHBUFFER,REF2VOID(((DrawBatchInfo_t*)l3d->drawNodes->draw.mesh->vid.vbo)->batchbuffer));
    break;
  case PL3DM_NONBATCHABLE:  // compile
    if (n==1)
      return FunctionPublish_returnBool(pstate,isFlagSet(l3d->compileflag,LIST3D_COMPILE_NOT));
    else if (n<2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 l3dmodel 1 boolean required");
    if (state)
      l3d->compileflag |= LIST3D_COMPILE_NOT;
    else
      l3d->compileflag &= ~LIST3D_COMPILE_NOT;
    break;
  case PL3DM_VERTEXARRAY: // vertexarray
    if ((!l3d->minst->modelupd && l3d->compileflag & LIST3D_COMPILE_BATCHED)|| !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MESHID,(void*)&data) || !SUBRESID_CHECK(data,l3d->minst->modelRID))
      return FunctionPublish_returnError(pstate,"1 l3dmodel (batched/morphable) 1 corresponding meshid required");
    ref = MeshArrayHandle_new(&vah,LUXI_CLASS_VERTEXARRAY);
    vah->type = MESHARRAY_L3DMODEL_MESHID;
    vah->meshid = data;
    vah->srcref = l3d->reference;
    Reference_refWeak(vah->srcref);
    Reference_makeVolatile(ref);
    return FunctionPublish_returnType(pstate,LUXI_CLASS_VERTEXARRAY,REF2VOID(ref));
    break;
  case PL3DM_INDEXARRAY:
    if (!(l3d->compileflag & LIST3D_COMPILE_BATCHED)|| !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MESHID,(void*)&data) || !SUBRESID_CHECK(data,l3d->minst->modelRID))
      return FunctionPublish_returnError(pstate,"1 l3dmodel (batched) 1 corresponding meshid required");
    ref = MeshArrayHandle_new(&vah,LUXI_CLASS_INDEXARRAY);
    vah->type = MESHARRAY_L3DMODEL_MESHID;
    vah->meshid = data;
    vah->srcref = l3d->reference;
    Reference_refWeak(vah->srcref);
    Reference_makeVolatile(ref);
    return FunctionPublish_returnType(pstate,LUXI_CLASS_INDEXARRAY,REF2VOID(ref));
    break;
  default:
    break;
  }
  return 0;
}

static int PubL3DModel_projector (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  int id;
  enum32 projcopy;

  byte state;

  if (n<2 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_L3D_MODEL,(void*)&ref,LUXI_CLASS_INT,(void*)&id)!=2 || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dmodel 1 int required");

  if (n==2){
    return FunctionPublish_returnBool(pstate,l3d->enviro.projectorflag & 1<<id);
  }
  else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&state))
    return FunctionPublish_returnError(pstate,"1 l3dmodel 1 int 1 boolean required");

  projcopy = l3d->enviro.projectorflag;

  if (state)
    projcopy |= 1<<id;
  else
    projcopy &= ~(1<<id);

  List3DNode_setProjectorFlag(l3d,projcopy);

  return 0;
}


void RMeshArrayHandle_free(Reference ref)
{
  MeshArrayHandle_free(Reference_value(ref));
}


void PubClass_Model_init()
{

  FunctionPublish_initClass(LUXI_CLASS_MODEL,"model",
    "A model contains triangle meshes and bonesystems, it can also have skin \
information for weighted vertex deformation based on bones. Models can be animateable or static, \
they can also have normals for lighting computation or simple vertex data. The information is needed \
for optimized storage and rendering of the model.<br> Every model can be loaded only once, so if one \
instance needs lighting use neednormals.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_MODEL,LUXI_CLASS_RESOURCE);
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_load,(void*)0,"load",
    "(model):(string filename,[boolean animateable],[boolean neednormals],[boolean bigvertex],[boolean nodisplaylist],[boolean novbo],[boolean lmcoords]) \
- loads a model. Bigvertex means that multiple texcoords, hardware skinning, normal mapping.. can be used. If you pass the novbo and nodisplaylist with true, then the model will not get special GL memory \
processing for faster rendering, it is supposedly used for compiled l3dmodels. \
Normally the engine will favor vbo if avaliable over displaylists. However as displaylists \
allow only few changes (need to use same shader, cant disable vertexcolors) depending on your \
needs it is useful to disable them. VBOs dont have those disadvantages, but can be costly for older cards if the model has just very few vertices.<br> When you should need lightmap texcoords use lmcoords with true.<br>(defaults: neednormals:true, rest:false)<");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_load,(void*)1,"forceload",
    "(model):(string filename,[boolean animateable],[boolean neednormals],[boolean bigvertex],[boolean nodisplaylist],[boolean novbo] - same as load but enforces to load a copy if resource was loaded before.)");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_new,NULL,"create",
    "(model):(string name, vertextype, int meshcount, int bonecount,[int skincount]) - returns an empty model, using boneid,meshid,skinid.init as well as vertex-/indexarrays you can fill it with content. To make it useable you must run the \
createfinish command after you set all data. There will be bonecount+meshcount bones at start, and the first bones are linked with the meshes.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_newfinish,NULL,"createfinish",
    "(model):(model,boolean animatable,[boolean nodisplaylist],[boolean novbo],[boolean lmcoords]) - returns itself after successful compiling and errorchecking. To make the model useable you must call this function after all data was set. It will load all needed textures and materials as well as creating proper bone hierarchy. \
If it's not animateable we will precompute all vertices and remove bonelinks from meshids, as well as compute the internal bounding box. For last two parameters see load command (set both false if you know that you will often change vertex/index data). If skins are set and model is animateable, those will be initialized as well.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_getbone,NULL,"boneid",
    "(boneid):(model,[string bonename/int index]) - returns boneid within model. If a string is applied, a bone with that name is seeked and if \
it doesn't exist, nothing is returned. If a number is passed, the bone with this index is returned, or an \
error is thrown to a index out of bounds exception. The index must be >=0 and <bonecount.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_getbone,(void*)1,"bonecount",
    "(int):(model) - returns number of bones within model.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_getboneparents,NULL,"getboneparents",
    "([array int]):(model) - returns an array of the parent index for every bone. We start at 0 not 1. -1 means it has no parent. With model.boneid(index) command you can get more info about the bone.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_getskin,NULL,"skinid",
    "(boneid):(model,[int index]) - returns skinid within model. The skin with this index is returned, or an \
    error is thrown to a index out of bounds exception. The index must be >=0 and < skincount.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_getskin,(void*)1,"skincount",
    "(int):(model) - returns number of skins within model.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_getmesh,NULL,"meshid",
    "(meshid):(model,[string meshname/int index]) - returns meshid within model. If a string is applied, a mesh with that name is seeked and if \
it doesn't exist, nothing is returned. If a number is passed, the mesh with this index is returned, or an \
error is thrown to a index out of bounds exception. The index must be >=0 and <meshcount.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_getmesh,(void*)1,"meshcount",
    "(int):(model) - returns number of meshes within model.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_func,(void*)PMDL_BBOX,"updatebbox",
    "():(model) - updates bounding box based on current vertices and reference bones.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_func,(void*)PMDL_PRESCALE,"loaderprescale",
    "([float x,y,z]):([float x,y,z]) - prescales vertices and bone positions with a given vector. The factor remains active for all follwing 'model.load' calls. Animations based on original size may cause problems. Prescale will be reapplied if the model is reloaded from disk again.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_func,(void*)PMDL_BBOXMANUAL,"bbox",
    "([float minx,miny,minz, maxx,maxy,maxz]):(model,[matrix4x4/float minx,miny,minz, maxx,maxy,maxz]) - returns or sets bounding box. Returning can be done with matrix transformation applied (computes new AABB based)");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_func,(void*)PMDL_GLUPDATE,"setmeshtype",
    "():(model,meshtype,[boolean lmcoords],[boolean vertexcolors]) - updates all meshes with given type. defaults for displaylist: lmcoords = false, vertexcolors = true");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_func,(void*)PMDL_REMANIMASSIGN,"removefromanim",
    "():(model,animation) - when animations are applied to models, they create an assignlist for each model to speed up track/bone matching. If you know this model will not use the given animation again, you can remove it from that list for speed's sake.");
  FunctionPublish_addFunction(LUXI_CLASS_MODEL,PubModel_func,(void*)PMDL_RUNANIM,"setanimcache",
    "():(model,animation, int time,[matrix4x4 rootmatrix]) - updates the animcache of a model with the given data. There is no visual effect of this operation, but only to be able to compute animation results without visual representation and query results via the boneid functions.");

  FunctionPublish_initClass(LUXI_CLASS_SKINID,"skinid","Skin within a model. Skins allow weighted vertex transforms based on the model's bones. Changing skin data after model.createfinish is not recommended.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_SKINID,PubSkinID_prop,(void*)SID_NEW,"init",
    "():(skinid,int vertexcount,int bonecount) - sets skindata, only works on empty skins in model.create generated models. vertexcount must match the mesh's count the skin is for. bonecount should be the minimum number of bones you need.");
  FunctionPublish_addFunction(LUXI_CLASS_SKINID,PubSkinID_prop,(void*)SID_HWSUPPORT,"hwsupport",
    "([boolean]):(skinid,[boolean]) - returns if skinning can be done in hardware. Only valid after model.initskin was called.");
  FunctionPublish_addFunction(LUXI_CLASS_SKINID,PubSkinID_prop,(void*)SID_BONEID,"boneid",
    "([boneid]):(skinid,int bindex,[boneid]) - returns or sets boneid of internal bonelist.");
  FunctionPublish_addFunction(LUXI_CLASS_SKINID,PubSkinID_prop,(void*)SID_VERTEXWEIGHTCOUNT,"skinvertexWeights",
    "([int]):(skinid,int vindex,[int numweights]) - returns or sets how many weights are used for this vertex. Hardware support will use 2 weights, lowdetail 1 weight, else a maximum of 4 is allowed.");
  FunctionPublish_addFunction(LUXI_CLASS_SKINID,PubSkinID_prop,(void*)SID_VERTEXWEIGHT,"skinvertexInfluence",
    "([float]):(skinid,int vindex,int weight,[float influence]) - returns or sets how much the influence of the bone is. You must sort weights by descending influences and make sure they sum to 1");
  FunctionPublish_addFunction(LUXI_CLASS_SKINID,PubSkinID_prop,(void*)SID_VERTEXBONE,"skinvertexBonelookup",
    "([int]):(skinid,int vindex,int weight,[int lookup]) - returns or sets index to internal bonelist.");

  FunctionPublish_initClass(LUXI_CLASS_MESHID,"meshid","Mesh within a model. Typically contains vertices and indices, as well as material information.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_MESHID,PubMeshID_prop,(void*)MID_NAME,"name",
    "([string]):(meshid) - returns name of mesh");
  FunctionPublish_addFunction(LUXI_CLASS_MESHID,PubMeshID_prop,(void*)MID_NEW,"init",
    "():(meshid,string name, int vertexcount, int indicescount, [texture/material name]) - only works on empty meshes that are hosted in model.create generated model. Fill with content via indexarray and vertexarray interfaces (don't forget to set indexCount and vertexCount and at the end indexMinmax!).");
  FunctionPublish_addFunction(LUXI_CLASS_MESHID,PubMeshID_prop,(void*)MID_MATSURF,"matsurface",
    "([matsurface]):(meshid) - returns matsurface");
  FunctionPublish_addFunction(LUXI_CLASS_MESHID,PubMeshID_prop,(void*)MID_MATERIAL,"material",
    "([material]):(meshid) - returns material of the mesh or nothing if there's no material");
  FunctionPublish_addFunction(LUXI_CLASS_MESHID,PubMeshID_prop,(void*)MID_TEXTURE,"texture",
    "([texture]):(meshid) - returns texture of the mesh or nothing if there's no texture");
  FunctionPublish_addFunction(LUXI_CLASS_MESHID,PubMeshID_prop,(void*)MID_TEXNAME,"texname",
    "([string]):(meshid) - returns original texturename (material / texture).");
  FunctionPublish_addFunction(LUXI_CLASS_MESHID,PubMeshID_prop,(void*)MID_BONEID,"boneid",
    "([boneid]):(meshid,[boneid]) - returns or sets boneid it is linked to. Only makes sense if model is animateable).");
  FunctionPublish_addFunction(LUXI_CLASS_MESHID,PubMeshID_prop,(void*)MID_SKINID,"skinid",
    "([skinid]):(meshid,[skinid]) - returns or sets skinid it is linked to. Only makes sense if model is animateable).");
  FunctionPublish_addFunction(LUXI_CLASS_MESHID,PubMeshID_prop,(void*)MID_RMESH,"rendermesh",
    "(rendermesh):(meshid) - returns rendermesh (be aware that it is only valid as long as the model is kept alive. No error checking is done.");


  FunctionPublish_initClass(LUXI_CLASS_MORPHCONTROL,"morphcontrol",
    "To add morph effects between models or meshes.\
    Morphing is done between vertex normal, position and color info. Activate it on l3dmodels.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_MORPHCONTROL,LUXI_CLASS_L3D_LIST);

  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_new,(void*)MODEL_CONTROL_MORPH_MODEL,"newfullmorph",
    "(morphcontrol):()- all meshes are affected by the morph");
  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_new,(void*)MODEL_CONTROL_MORPH_MESH,"newmeshmorph",
    "(morphcontrol):()- only a single mesh will get morphed");
  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_new,(void*)MODEL_CONTROL_FORCEDIRTY,"newforceall",
    "(morphcontrol):()- we will always use the vertices in the modeulupdate container.");
  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_del,NULL,"delete",
    "():(morphcontrol) - deactivates and deletes the morphcontrol");
  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_attributes,(void*)PMORPH_FACTOR,"factor",
    "(float):(morphcontrol,float) - sets or returns the blend factor of the morph");
  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_attributes,(void*)PMORPH_MDLFROM,"modelfrom",
    "():(morphcontrol,model) - sets model to morph from, if a none-model is passed we will use previous morph results");
  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_attributes,(void*)PMORPH_MDLTO,"modelto",
    "():(morphcontrol,model) - sets model to morph to, if a none-model is passed we will use previous morph results");
  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_attributes,(void*)PMORPH_MFROM,"meshfrom",
    "():(morphcontrol,meshid) - sets mesh to morph from");
  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_attributes,(void*)PMORPH_MTO,"meshto",
    "():(morphcontrol,meshid) - sets mesh to morph to");
  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_attributes,(void*)PMORPH_MTARGET,"meshtarget",
    "():(morphcontrol,meshid) - sets mesh that will get morphed");
  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_activate,NULL,"activate",
    "():(morphcontrol,l3dmodel) - activates the morph effect on the given l3dmodel");
  FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_deactivate,NULL,"deactivate",
    "():(morphcontrol) - deactivates the morph effect");
  //FunctionPublish_addFunction(LUXI_CLASS_MORPHCONTROL,PubMorphControl_del,(void*)1,"deleteall",
  //  "():() - deletes all");

  FunctionPublish_initClass(LUXI_CLASS_L3D_MODEL,"l3dmodel",
    "l3dnode of a model instance. Every l3dmodel has unique materials, bone and morph data, if specified. \
You can apply boneanimation with the sequence commands, control bones with bonecontrol or run morphing \
with morphcontrol. On creation the l3dmodel will copy all material and renderflags (if material provides a shader) data from the source model.",
    NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_MODEL,LUXI_CLASS_L3D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_new,(void*)0,"new",
    "(l3dmodel):(string name,|l3dlayerid|,model,[boolean boneanim],[boolean meshanim]) - creates l3dmodel. If you want to be able to apply animations on it set boneanim to true. If you want it to have unique vertices or allow morphing set meshanim to true. (both default to false)");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_newinstance,NULL,"newinstance",
    "(l3dmodel):(string name,|l3dlayerid|,l3dmodel) - creates instance of another l3dmodel, they will share bone and morph data. Only works if bone data is not set to worldspace.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_getmodel,NULL,"getmodel",
    "(model):(l3dnode) - returns model of this l3dnode");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_material,NULL,"matsurface",
    "([material/texture]):(l3dnode,meshid,[matsurface]) - sets or returns material of specific mesh");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_projector,NULL,"projectorflag",
    "([boolean]):(l3dnode,int id,[boolean]) - returns or sets to which projectors the model reacts to. id is 0-31.");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_BONERESET,"bonereset",
    "():(l3dmodel) - sets bones to reference positions");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_BONEMATRIX,"bonematrix",
    "(matrix4x4):(l3dmodel,boneid/string bonename) - gets bone matrix");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_BONEPOS,"bonepos",
    "(float x,y,z):(l3dmodel,boneid/string bonename) - gets bone position");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_BONEROTRAD,"bonerotrad",
    "(float x,y,z):(l3dmodel,boneid/string bonename) - gets bone rotation radians");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_BONEROTDEG,"bonerotdeg",
    "(float x,y,z):(l3dmodel,boneid/string bonename) - gets bone rotation degrees");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_BONEROTAXIS,"bonerotaxis",
    "(float Xx,Xy,Xz,Yx,Yy,Yz,Zx,Zy,Zz):(l3dmodel,boneid/string bonename) - gets bone rotation axis");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_MORPHRESET,"morphreset",
    "():(l3dmodel) - sets morphdata to reference positions");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_NONBATCHABLE,"nonbatchable",
    "([boolean]):(l3dmodel,[boolean]) - returns or sets state. If a batchbuffer affects this model it will not be included.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_VERTEXARRAY,"getvertexarray",
    "(vertexarray):(l3dmodel,meshid) - returns vertexarray of a batched or morphable l3dmodel's mesh.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_INDEXARRAY,"getindexarray",
    "(vertexarray):(l3dmodel,meshid) - returns vertexarray of a batched l3dmodel's mesh.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_BATCHBUFFER,"getl3dbatchbuffer",
    "([l3dbatchbuffer]):(l3dmodel) - returns batchbuffer of the model, if it was batched.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_WORLDBONES,"bonesinworldspace",
    "([boolean]):(l3dmodel,[boolean]) - returns or sets if bones should be calculated in world space. If set in worldspace you cannot instance this l3dmodel, and you will get problems when others were already instanced.");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_sequenceanim,(void*)PSEQ_ANIM,"seqAnimsource",
    "([animation]):(l3dmodel,int sequence,int anim,[animation]) - sets or returns sequence:anim animation");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_sequenceanim,(void*)PSEQ_START,"seqAnimstart",
    "([int]):(l3dmodel,int sequence,int anim,[int]) - sets or returns sequence:anim start time");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_sequenceanim,(void*)PSEQ_LEN,"seqAnimlength",
    "([int]):(l3dmodel,int sequence,int anim,[int]) - sets or returns sequence:anim length");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_SEQANIMMAX,"seqAnimMax",
    "([int]):() - returns how many anims per sequence can be set");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_sequence,(void*)PSEQ_STATE,"seqState",
    "([int]):(l3dmodel,int sequence,[int state]) - sets or returns state of sequence. 0 inactive, 1 run, 2 pause, 3 singleframe");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_sequence,(void*)PSEQ_SPEED,"seqSpeed",
    "([float]):(l3dmodel,int sequence,[float]) - sets or returns speed of sequence, default is 1");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_sequence,(void*)PSEQ_TIME,"seqTime",
    "([int]):(l3dmodel,int sequence,[int]) - sets or returns current time of sequence");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_sequence,(void*)PSEQ_OFFSET,"seqOffset",
    "([int]):(l3dmodel,int sequence,[int]) - sets or returns current offset between anims of sequence, negative means overlapping. Dont call before anims were set. It will also set the proper duration for the sequence.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_sequence,(void*)PSEQ_LOOP,"seqLoop",
    "([boolean]):(l3dmodel,int sequence,[boolean]) - sets or returns if sequence loops, default is true");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_sequence,(void*)PSEQ_BLENDMANUAL,"seqBlendmanual",
    "([boolean]):(l3dmodel,int sequence,[boolean]) - sets or returns if sequence is blended manually");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_sequence,(void*)PSEQ_BLEND,"seqBlendfactor",
    "([float]):(l3dmodel,int sequence,[float]) - sets or returns blendfactor of sequence");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_sequence,(void*)PSEQ_DURATION,"seqDuration",
    "(int):(l3dmodel,int sequence) - returns duration of sequence");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_MODEL,PubL3DModel_exec,(void*)PL3DM_SEQMAX,"seqMax",
    "([int]):() - returns how many sequences can be set.");

  FunctionPublish_initClass(LUXI_CLASS_VERTEXARRAY,"vertexarray",
    "meshes, l3dmodels (morphable or compiled) or l2dimage usermeshes contain vertices you can modify."
"This interface allows access to it. For models this only has effect if they are not rendered via display list, "
"nor vbo, else those need to be updated as well.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_VERTEXARRAY,LUXI_CLASS_RENDERINTERFACE);
  Reference_registerType(LUXI_CLASS_VERTEXARRAY,"vertexarray",RMeshArrayHandle_free,NULL);
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_new,(void*)NULL,"newfromptrs",
    "(vertexarray):(vertextype,pointer begin,end) - creates a vertexarray handle from the pointers. Make sure they stay valid as long as you use the handle.");

  FunctionPublish_setFunctionInherition(LUXI_CLASS_VERTEXARRAY,"newfromptrs",LUX_FALSE);

  FunctionPublish_addInterface(LUXI_CLASS_MESHID,LUXI_CLASS_VERTEXARRAY);
  FunctionPublish_addInterface(LUXI_CLASS_L2D_IMAGE,LUXI_CLASS_VERTEXARRAY);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_PRIMITIVE,LUXI_CLASS_VERTEXARRAY);
  FunctionPublish_addInterface(LUXI_CLASS_RCMD_DRAWMESH,LUXI_CLASS_VERTEXARRAY);
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_VALLOC,"vertexAllocatedcount",
    "([int cnt]):(vertexarray) - return vertex allocated count. This is the number of vertices that can be used in this vertexarray, index must always be smaller than this.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_VCOUNT,"vertexCount",
    "([int cnt]):(vertexarray, [int cnt]) - set or return vertex count. This count is used for statistics and for optimizing, it can be smaller than allocated count and should be equal to the last vertex you use. Morphable l3dmodels will not allow setting it.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_POS,"vertexPos",
    "([float x,y,z]):(vertexarray, int index,[matrix4x4],[float x,y,z]) - set or return position. Transformed with by matrix if given.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_NORMAL,"vertexNormal",
    "([float x,y,z]):(vertexarray, int index,[matrix4x4],[float x,y,z]) - set or return normal, some vertexarrays do not have normals");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_COLOR,"vertexColor",
    "([float r,g,b,a]):(vertexarray, int index,[float r,g,b,a]) - set or return color");
  //FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_USER4,"vertexUser4",
  //  "([float x,y,z,w]):(vertexarray, int index,[float x,y,z,w]) - set or return user4 vector, only bigvertex vertexarrays will allow this. It overwrites texcoord channel 2 and 3.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_TEX,"vertexTexcoord",
    "([float u,v]):(vertexarray, int index,[float u,v],[int texchannel 0-3]) - set or return texture coordinates for given channel. All vertices have 1 channel, without normals 2 and bigvertex 4. channel 0 is used for textures, channel 1 is used for lightmaps.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_TRANSFORM,"vertexTransform",
    "():(vertexarray, matrix4x4) - transform all vertices with given matrix");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_COPY,"vertexCopy",
    "():(vertexarray, int start, vertexarray from,int fromstart, int size, [matrix4x4]) - copies vertices from other vertexarray, optionally with transfrom matrix.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_BOUNDS,"vertexBBox",
    "(float minx,miny,minz,maxx,maxy,maxz):(vertexarray) - gets bounding box of vertices");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_POS2TEX,"vertexPos2texcoord",
    "():(vertexarray,[float planex,y,z], [int texchannel 0-3]) - copy xy coordinates from positions to texcoord channel for all vertices, or project them on given plane normal. ");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_TEXINV,"vertexInverttexcoord",
    "():(vertexarray, boolean y,[int texchannel 0-3]) - inverts y (or x) coordinates of texcoord channel for all vertices by doing 1.0f-coord. ");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_TEXTRANSFORM,"vertexTexcoordtransform",
    "():(vertexarray, matrix4x4,[int texchannel 0-3]) - transforms texcoords by given matrix ");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_COLARRAY,"vertexColorarray",
    "([floatarray]):(vertexarray,[floatarray]) - returns or sets vertex colors from given array (4 times the size of vertexAllocatedcount).");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_POSARRAY,"vertexPosarray",
    "([floatarray]):(vertexarray,[floatarray]) - returns or sets vertex position from given array (3 times the size of vertexAllocatedcount).");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_NORMALARRAY,"vertexNormalarray",
    "([floatarray]):(vertexarray,[floatarray]) - returns or sets vertex position from given array (3 times the size of vertexAllocatedcount).");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_UVTRIS,"vertexTexcoordTris",
    "([float u,v]):(vertexarray, int a, int b, int c, float ba, float ca,[int texchannel 0-3]) - returns texture coordinates for given triangle indices, using the barycentric coordinates.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_NORMALTRIS,"vertexNormalTris",
    "([float x,y,z]):(vertexarray, int a, int b, int c, float ba, float ca) - returns interpolated normal for given triangle indices, using the barycentric coordinates.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_POSTRIS,"vertexPosTris",
    "([float x,y,z]):(vertexarray, int a, int b, int c, float ba, float ca) - returns interpolated position for given triangle indices, using the barycentric coordinates.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_COLORTRIS,"vertexColorTris",
    "([float r,g,b,a]):(vertexarray, int a, int b, int c, float ba, float ca) - returns interpolated color for given triangle indices, using the barycentric coordinates.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_PTRS,"vertexPtrs",
    "(pointer start,end]):(vertexarray) - returns C pointers for begin and end of array.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_MESHTYPE,"vertexMeshtype",
    "([meshtype]):(vertexarray) - returns what type of mesh the indices/vertices are stored for display.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_VIDBUFFER,"vertexVidbuffer",
    "([vidbuffer,int vbooffset]):(vertexarray) - if stored as VBO returns vidbuffer and offset bytes.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_TYPE,"vertexType",
    "(vertextype):(vertexarray) - returns the vertextype.");

  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_SUBMITGL,"vertexUpdateVBO",
    "(boolean):(vertexarray,[int fromidx, count]) - if possible submits array using vertexAllocatedcount or [from,to] range to VBO, returns true on success else false (no VBO or non compatible vertexarray).");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_RETRIEVEGL,"vertexFromVBO",
    "(boolean):(vertexarray,[int fromidx, count]) - if possible retrieves array using vertexAllocatedcount or [from,to] range from VBO, returns true on success else false (no VBO or non compatible vertexarray).");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXARRAY,PubVertexArray_prop,(void*)VA_PTRSTATE,"vertexAllocState",
    "([boolean]):(vertexarray,[boolean]) - can create or remove the local data copy of the vertices. Only allowed if mesh is a VBO usermesh.");

  FunctionPublish_initClass(LUXI_CLASS_INDEXARRAY,"indexarray",
    "meshes contain indices for rendering their primitives, with this interface you can modify them. Always make sure that the corresponding vertices exist and always run indexMinmax once you set all indices. If you use display lists or vbos, changes to a loaded model will not be visible.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_INDEXARRAY,LUXI_CLASS_RENDERINTERFACE);
  Reference_registerType(LUXI_CLASS_INDEXARRAY,"indexarray",RMeshArrayHandle_free,NULL);

  FunctionPublish_addInterface(LUXI_CLASS_MESHID,LUXI_CLASS_INDEXARRAY);
  FunctionPublish_addInterface(LUXI_CLASS_L2D_IMAGE,LUXI_CLASS_INDEXARRAY);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_PRIMITIVE,LUXI_CLASS_INDEXARRAY);
  FunctionPublish_addInterface(LUXI_CLASS_RCMD_DRAWMESH,LUXI_CLASS_INDEXARRAY);

  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_IALLOC,"indexAllocatedcount",
    "([int cnt]):(indexarray) - return indices count. This is the number of indices that can be used in this indexarray, index must always be smaller than this.");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_VALUE,"indexValue",
    "([int value]):(indexarray,index,[int value]) - set or returns value at the given index. Make sure the corresponding vertex exists");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_PRIM,"indexPrimitive",
    "([int a,b,c...]):(indexarray,primindex,[int a,b,c...]) - set or returns the primitive at given position. The function will return/need 1 index for points, 2 for lines, 3 indices for triangles, 4 indices for quads and can only return indices for any strips/loops/fan. Polygons are not supported, since the whole indexarray always contains only one.");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_ICOUNT,"indexCount",
    "([int]):(indexarray,[int cnt]) - set or return the number of indices to be used on rendering. This value must always be smaller than Allocatedcount.");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_PRIMCOUNT,"indexPrimitivecount",
    "([int]):(indexarray) - returns the number of primitives based on indexCount");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_TRISCOUNT,"indexTrianglecount",
    "([int]):(indexarray) - returns and updates the internal number of triangles based on current indexCount. This is only needed for statistics and should be called at least once after all indices were written.");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_PRIMTYPE,"indexPrimitivetype",
    "([primitivetype]):(indexarray, [primitivetype]) - set or return the primitive type.");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_MINMAX,"indexMinmax",
    "(int min,max):(indexarray,[min,max]) - computes minimum and maximum index used within indexCount, or you can manually set them. This is required for an indexarray to work, call this after all indices are set.");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_SUBMITGL,"indexUpdateVBO",
    "(boolean):(indexarray,[int fromidx, count]) - if possible resubmits array using indexAllocatedcount or [from,to] range to VBO, returns true on success else false (no VBO or non compatible indexarray).");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubVertexArray_prop,(void*)IA_RETRIEVEGL,"vertexFromVBO",
    "(boolean):(indexarray,[int fromidx, count]) - if possible retrieves array using vertexAllocatedcount or [from,to] range from VBO, returns true on success else false (no VBO or non compatible vertexarray).");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubVertexArray_prop,(void*)IA_PTRSTATE,"vertexAllocState",
    "([boolean]):(indexarray,[boolean]) - can create or remove the local data copy of the vertices. Only allowed if mesh is a VBO usermesh.");

  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_MESHTYPE,"indexMeshtype",
    "([meshtype]):(indexarray) - returns what type of mesh the indices/vertices are stored for display.");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_VIDBUFFER,"indexVidbuffer",
    "([vidbuffer,int ibooffset]):(indexarray) - if stored as VBO returns vidbuffer and offset bytes.");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_TYPE,"indexType",
    "(scalartype):(indexarray) - returns the scalartype of the indices.");


  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_COPY,"indexCopy",
    "([int]):(indexarray,int start,indexarray from,int fromstart, int size,[int vertexoffset]) - copies indices from one indexarray into self, starting at given index and optionally offsetting the copied.");
  FunctionPublish_addFunction(LUXI_CLASS_INDEXARRAY,PubIndexArray_prop,(void*)IA_PTRS,"indexPtrs",
    "(pointer start,end]):(indexarray) - returns C pointers for begin and end of array. [start,end[");
}
