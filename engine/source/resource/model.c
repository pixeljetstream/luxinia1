// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "resmanager.h"
#include "../render/gl_draw3d.h"
#include "../render/gl_drawmesh.h"

/*
=================================================================
MODEL,MESH,MATERIAL,BONE FUNCS
=================================================================
*/

// LOCALS
static struct ModelData_s{
  lxVector3 scale;
  int   usescale;
} l_ModelData;

//////////////////////////////////////////////////////////////////////////
// VERTEX CONTAINER

VertexContainer_t*  VertexContainer_new(Model_t *model)
{
  int i;
  VertexContainer_t *vcont;

  vcont = lxMemGenZalloc(sizeof(VertexContainer_t));

  vcont->vertextype = model->vertextype;
  vcont->numVertexDatas = model->numMeshObjects;
  vcont->vertexDatas = lxMemGenZalloc(sizeof(void*)*model->numMeshObjects);
  vcont->numVertices = lxMemGenZalloc(sizeof(int)*model->numMeshObjects);
  for (i = 0; i < model->numMeshObjects; i++){
    vcont->numVertices[i] = model->meshObjects[i].mesh->numVertices;
    vcont->vertexDatas[i] = lxMemGenZallocAligned(VertexSize(model->vertextype)*model->meshObjects[i].mesh->numVertices,32);
    memcpy(vcont->vertexDatas[i],model->meshObjects[i].mesh->vertexData,VertexSize(model->vertextype)*model->meshObjects[i].mesh->numVertices);
  }

  return vcont;
}

void        VertexContainer_free(VertexContainer_t *vcont)
{
  int i;
  for (i = 0; i < vcont->numVertexDatas; i++)
    lxMemGenFreeAligned(vcont->vertexDatas[i],VertexSize(vcont->vertextype)*vcont->numVertices[i]);
  lxMemGenFree(vcont->vertexDatas,sizeof(void*)*vcont->numVertexDatas);
  lxMemGenFree(vcont->numVertices,sizeof(int)*vcont->numVertexDatas);

  lxMemGenFree(vcont,sizeof(VertexContainer_t));
}
void  VertexArray_transformValue(void *vertices, enum VertexValue_e what,enum32 vtype, int numVertices, lxMatrix44 matrix)
{
  float *out;
  short *sout;
  lxVector3 normal;
  int   stride;
  int   shortnormal;
  int i;

  LUX_ASSERT(g_VertexSizes[vtype][what] != -1);

  // FIXME take datatypes into acount

  stride = VertexSize(vtype)/sizeof(float);
  if (what == VERTEX_NORMAL || what == VERTEX_TANGENT){
    switch(vtype)
    {
    case VERTEX_64_TEX4:
    case VERTEX_64_SKIN:
    case VERTEX_32_TEX2:
      shortnormal = LUX_TRUE;
      stride = VertexSize(vtype)/sizeof(short);
    default:
      break;
    }
  }
  sout = (void*)out = VertexArrayPtr(vertices,0,vtype,what);

  if (what == VERTEX_TEX0 || what == VERTEX_TEX1){
    for (i = 0 ; i < numVertices; i++,out+=stride){
      lxVector2Transform1(out,matrix);
    }
  }
  else if (what == VERTEX_POS && vtype == VERTEX_16_HOM){
    for (i = 0 ; i < numVertices; i++,out+=stride){
      lxVector4Transform1(out,matrix);
    }
  }
  else if (shortnormal){
    for (i = 0 ; i < numVertices; i++,sout+=stride){
      lxVector3float_FROM_short(normal,sout);
      lxVector3TransformRot1(normal,matrix);
      lxVector3short_FROM_float(sout,normal);
    }
  }
  else{
    for (i = 0 ; i < numVertices; i++,out+=stride){
      lxVector3Transform1(out,matrix);
    }
  }
}

void  VertexArray_transform(void *outvertices, enum32 vtype, int numVertices, lxMatrix44 matrix)
{
  int i;
  lxVertex64_t *outvert64;
  lxVertex32_t *outvert32;
  lxVertex32FN_t *outvert32fn;
  lxVertex16_t  *outvert16;

  lxVector3 normal;

  outvert32fn = (void*)  outvert16 = (void*) outvert64 = (void*)( outvert32 = outvertices);

  switch(vtype) {
  case VERTEX_64_SKIN:
  case VERTEX_64_TEX4:
    for (i = 0; i < numVertices; i++,outvert64++){
      lxVector3Transform1(outvert64->pos,matrix);
      lxVector3float_FROM_short(normal,outvert64->normal);
      lxVector3TransformRot1(normal,matrix);
      lxVector3Normalized(normal);
      lxVector3short_FROM_float(outvert64->normal,normal);
      lxVector3float_FROM_short(normal,outvert64->tangent);
      lxVector3TransformRot1(normal,matrix);
      lxVector3Normalized(normal);
      lxVector3short_FROM_float(outvert64->tangent,normal);
    }
    break;
  case VERTEX_32_TEX2:
    for (i = 0; i < numVertices; i++,outvert32++){
      lxVector3Transform1(outvert32->pos,matrix);
    }
    break;
  case VERTEX_32_NRM:
    for (i = 0; i < numVertices; i++,outvert32++){
      lxVector3Transform1(outvert32->pos,matrix);
      lxVector3float_FROM_short(normal,outvert32->normal);
      lxVector3TransformRot1(normal,matrix);
      lxVector3Normalized(normal);
      lxVector3short_FROM_float(outvert32->normal,normal);
    }
    break;
  case VERTEX_32_FN:
    for (i = 0; i < numVertices; i++,outvert32fn++){
      lxVector3Transform1(outvert32fn->pos,matrix);
      lxVector3TransformRot1(outvert32fn->normal,matrix);
    }
    break;
  case VERTEX_16_HOM:
    for (i = 0; i < numVertices; i++,outvert16++){
      lxVector4Transform1(outvert16->pos,matrix);
    }
    break;
  case VERTEX_16_CLR:
    for (i = 0; i < numVertices; i++,outvert16++){
      lxVector3Transform1(outvert16->pos,matrix);
    }
    break;
  default:
    break;
  }
}

void  VertexArray_minmax(void *vertices, enum32 vtype, int numVertices, lxVector3 min, lxVector3 max)
{
  int i;
  float *pos;
  int stride = VertexSize(vtype)/sizeof(float);

  pos = VertexArrayPtr(vertices,0,vtype,VERTEX_POS);
  LUX_ASSERT(VertexValue(vtype,VERTEX_SCALARPOS) == LUX_SCALAR_FLOAT32);

  lxVector3Copy(min,pos);
  lxVector3Copy(max,pos);

  pos+=stride;
  for (i = 1; i < numVertices; i++,pos+=stride){
    min[0] = LUX_MIN(min[0],pos[0]);
    min[1] = LUX_MIN(min[1],pos[1]);
    min[2] = LUX_MIN(min[2],pos[2]);

    max[0] = LUX_MAX(max[0],pos[0]);
    max[1] = LUX_MAX(max[1],pos[1]);
    max[2] = LUX_MAX(max[2],pos[2]);
  }
}

void  VertexArray_markWeld(void *vertices, enum32 vtype, int numVertices, int *marks, float rangeSq)
{
  int i,n;
  float *pos1,*pos2;
  int stride = VertexSize(vtype)/sizeof(float);
  lxVector3 diff;

  pos1 = VertexArrayPtr(vertices,0,vtype,VERTEX_POS);
  pos2 = pos1;

  LUX_ASSERT(VertexValue(vtype,VERTEX_SCALARPOS) == LUX_SCALAR_FLOAT32);

  for (i=0;i < numVertices; i++)
    marks[i] = i;

  for (i = 0; i < numVertices; i++,pos1+=stride){
    pos2 = pos1+stride;
    for (n = i+1; n < numVertices; n++,pos2+=stride){
      if (marks[n] != n)
        continue;
      lxVector3Sub(diff,pos1,pos2);
      if(lxVector3SqLength(diff) < rangeSq){
        marks[n] = i;
      }
    }
  }
}

void  VertexContainer_transform(VertexContainer_t *vcont, Model_t *model, lxMatrix44 matrix)
{
  int i, n;
  lxVertex64_t *invert64, *outvert64;
  lxVertex32_t *invert32, *outvert32;
  lxVector3 normal;

  switch(vcont->vertextype) {
  case VERTEX_64_SKIN:
  case VERTEX_64_TEX4:
    for (n = 0; n < vcont->numVertexDatas; n++){
      invert64 = model->meshObjects[n].mesh->vertexData64;
      outvert64 = vcont->vertexDatas64[n];
      for (i = 0; i < vcont->numVertices[n]; i++,invert64++,outvert64++){
        lxVector3Transform(outvert64->pos,invert64->pos,matrix);
        lxVector3float_FROM_short(normal,invert64->normal);
        lxVector3TransformRot1(normal,matrix);
        lxVector3short_FROM_float(outvert64->normal,normal);
        lxVector3float_FROM_short(normal,invert64->tangent);
        lxVector3TransformRot1(normal,matrix);
        lxVector3short_FROM_float(outvert64->tangent,normal);
      }
    }
    break;
  case VERTEX_32_TEX2:
    for (n = 0; n < vcont->numVertexDatas; n++){
      invert32 = model->meshObjects[n].mesh->vertexData32;
      outvert32 = vcont->vertexDatas32[n];
      for (i = 0; i < vcont->numVertices[n]; i++,invert32++,outvert32++){
        lxVector3Transform(outvert32->pos,invert32->pos,matrix);
      }
    }
    break;
  case VERTEX_32_NRM:
    for (n = 0; n < vcont->numVertexDatas; n++){
      invert32 = model->meshObjects[n].mesh->vertexData32;
      outvert32 = vcont->vertexDatas32[n];
      for (i = 0; i < vcont->numVertices[n]; i++,invert32++,outvert32++){
        lxVector3Transform(outvert32->pos,invert32->pos,matrix);
        lxVector3float_FROM_short(normal,invert32->normal);
        lxVector3TransformRot1(normal,matrix);
        lxVector3short_FROM_float(outvert32->normal,normal);
      }
    }
    break;
  default:
    break;
  }

}

//////////////////////////////////////////////////////////////////////////
// MODEL UPDATE


ModelUpdate_t *ModelUpdate_new(Model_t *model){
  ModelUpdate_t *modelupd;

  modelupd = lxMemGenZalloc(sizeof(ModelUpdate_t)+(model->numMeshObjects*sizeof(int)));
  modelupd->vertexcont = VertexContainer_new(model);
  modelupd->dirtyVDatas = (int*)(((char*)modelupd)+sizeof(ModelUpdate_t));

  return modelupd;
}
static void ModelControl_free(ModelControl_t *mcontrol);
void ModelUpdate_free(ModelUpdate_t *modelupd){
  int vcnts = modelupd->vertexcont->numVertexDatas;
  VertexContainer_free(modelupd->vertexcont);
  lxListNode_destroyList(modelupd->controlListHead,ModelControl_t,ModelControl_free);

  lxMemGenFree(modelupd,sizeof(ModelUpdate_t)+(sizeof(int)*vcnts));
}

void ModelUpdate_morphMesh(ModelUpdate_t *modelupd,ModelControl_t *browse, int n){
  float blend;
  int i;
  void    *from;
  void    *to;

  lxVertex32_t *from32;
  lxVertex32_t *to32;
  lxVertex32_t *out32;

  lxVertex64_t *from64;
  lxVertex64_t *to64;
  lxVertex64_t *out64;

  blend = browse->morphFactor;

  // ignore instances
  if (browse->modelMorphFrom->meshObjects[n].mesh->instanceType)
    return;

  modelupd->dirtyVDatas[n] = LUX_TRUE;

  if (browse->modelMorphFrom == MODEL_CONTROL_SELF)
    from = modelupd->vertexcont->vertexDatas[n];
  else
    from = browse->modelMorphFrom->meshObjects[browse->meshFrom].mesh->vertexData;

  if (browse->modelMorphTo == MODEL_CONTROL_SELF)
    to = modelupd->vertexcont->vertexDatas[n];
  else
    to = browse->modelMorphTo->meshObjects[browse->meshTo].mesh->vertexData;


  switch(modelupd->model->vertextype) {
  case VERTEX_64_SKIN:
  case VERTEX_64_TEX4:
    from64 = from;
    to64 = to;
    out64 = modelupd->vertexcont->vertexDatas[n];
    for (i = 0; i < modelupd->vertexcont->numVertices[n]; i++,from64++,to64++,out64++){
      lxVector3Lerp(out64->pos,blend,from64->pos,to64->pos);
      LUX_ARRAY3LERPTYPE(short,out64->normal,blend,from64->normal,to64->normal);
      LUX_ARRAY4LERPTYPE(uchar,out64->color,blend,from64->color,to64->color);
    }
    break;
  case VERTEX_32_TEX2:
    from32 = from;
    to32 = to;
    out32 = modelupd->vertexcont->vertexDatas[n];
    for (i = 0; i < modelupd->vertexcont->numVertices[n]; i++,from32++,to32++,out32++){
      lxVector3Lerp(out32->pos,blend,from32->pos,to32->pos);
      LUX_ARRAY4LERPTYPE(uchar,out32->color,blend,from32->color,to32->color);
    }
    break;
  case VERTEX_32_NRM:
    from32 = from;
    to32 = to;
    out32 = modelupd->vertexcont->vertexDatas[n];
    for (i = 0; i < modelupd->vertexcont->numVertices[n]; i++,from32++,to32++,out32++){
      lxVector3Lerp(out32->pos,blend,from32->pos,to32->pos);
      LUX_ARRAY4LERPTYPE(ushort,out32->normal,blend,from32->normal,to32->normal);
      LUX_ARRAY4LERPTYPE(uchar,out32->color,blend,from32->color,to32->color);
    }
    break;
  default:
    break;
  }
}

void ModelUpdate_reset(ModelUpdate_t *modelupd)
{
  int *pNumVertices;
  VertexContainer_t *vcont = modelupd->vertexcont;
  int *pDirty;
  int n;

  pDirty = modelupd->dirtyVDatas;
  pNumVertices = vcont->numVertices;
  for (n = 0; n < vcont->numVertexDatas; n++,pDirty++,pNumVertices++){
    if (*pDirty){
      memcpy(vcont->vertexDatas[n],modelupd->model->meshObjects[n].mesh->vertexData,VertexSize(modelupd->model->vertextype)*(*pNumVertices));
      *pDirty = LUX_FALSE;
    }
  }
  modelupd->dirty = LUX_FALSE;
}

void ModelUpdate_run(ModelUpdate_t *modelupd){
  ModelControl_t  *browse;
  int n;


  if (!modelupd->controlListHead){
    return;
  }

  lxListNode_forEach(modelupd->controlListHead,browse)
    switch(browse->type){
    case MODEL_CONTROL_MORPH_MODEL:
      for (n = 0; n < modelupd->vertexcont->numVertexDatas; n++){
        ModelUpdate_morphMesh(modelupd,browse,n);
      }
      break;
    case MODEL_CONTROL_MORPH_MESH:
      ModelUpdate_morphMesh(modelupd,browse,browse->mesh);
      break;
    case MODEL_CONTROL_FORCEDIRTY:
      for (n = 0; n < modelupd->vertexcont->numVertexDatas; n++){
        modelupd->dirtyVDatas[n] = LUX_TRUE;
      }
    default:
      break;
    }
  lxListNode_forEachEnd(modelupd->controlListHead,browse);

  modelupd->dirty = LUX_TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// ModelControl

void      ModelControl_activate(ModelControl_t *mcontrol,ModelUpdate_t *modelupd)
{
  ModelControl_deactivate(mcontrol);
  lxListNode_addLast(modelupd->controlListHead,mcontrol);
  mcontrol->modelupd = modelupd;
}
void      ModelControl_deactivate(ModelControl_t *mcontrol)
{
  if (!mcontrol->modelupd)
    return;
  lxListNode_remove(mcontrol->modelupd->controlListHead,mcontrol);
}

ModelControl_t * ModelControl_new()
{
  ModelControl_t *mcontrol;

  mcontrol = lxMemGenZalloc(sizeof(ModelControl_t));
  lxListNode_init(mcontrol);

  mcontrol->type = MODEL_CONTROL_UNSET;
  mcontrol->reference = Reference_new(LUXI_CLASS_MORPHCONTROL,mcontrol);

  return mcontrol;
}

static void ModelControl_free(ModelControl_t *mcontrol){
  ModelControl_deactivate(mcontrol);
  Reference_invalidate(mcontrol->reference);
  lxMemGenFree(mcontrol,sizeof(ModelControl_t));
}

void RModelControl_free(lxRmorphcontrol ref)
{
  ModelControl_free((ModelControl_t*)Reference_value(ref));
}
void  ModelControl_init()
{
  Reference_registerType(LUXI_CLASS_MORPHCONTROL,"morphcontrol",RModelControl_free,NULL);
}

//////////////////////////////////////////////////////////////////////////
// MODEL OBJECT

void  Modelnstance_updatePtrs(Modelnstance_t * minst)
{
  int i;
  static const int dummy = 0;
  Model_t *model = ResData_getModel(minst->modelRID);

  if (minst->numSkinObjs){
    for (i = 0 ; i < minst->numSkinObjs; i++){
      minst->skinobjs[i].relMatricesT = (minst->boneupd) ? minst->boneupd->bonesRelT : model->bonesys.refRelMatricesT;
      minst->skinobjs[i].skin = &model->skins[i];
      minst->skinobjs[i].skinID = i;
      minst->skinobjs[i].time = (minst->boneupd) ? &minst->boneupd->updateTime : &dummy;
    }
  }
  if (minst->boneupd)
    minst->boneupd->bonesys = &model->bonesys;
}

Modelnstance_t *Modelnstance_new(int modelRID,const booln boneupd,const booln modelupd){
  Modelnstance_t *minst;
  Model_t *model = ResData_getModel(modelRID);

  if (!model)
    return NULL;

  minst = lxMemGenZalloc(sizeof(Modelnstance_t) + (sizeof(SkinObject_t)*model->numSkins));
  minst->modelRID = modelRID;

  if (isModelDynamic(model)){
    if (boneupd)
      minst->boneupd = BoneSysUpdate_new(&model->bonesys);
    if (modelupd)
      minst->modelupd = ModelUpdate_new(model);
    minst->numSkinObjs = model->numSkins;
    if (minst->numSkinObjs)
      minst->skinobjs = (void*)(minst+1);
  }
  Modelnstance_updatePtrs(minst);
  minst->references = 1;

  return minst;
}

Modelnstance_t* Modelnstance_ref(Modelnstance_t* minst){
  minst->references++;
  return minst;
}

Modelnstance_t* Modelnstance_unref(Modelnstance_t *minst)
{
  minst->references--;

  if (minst->references == 0){
    if (minst->boneupd)
      BoneSysUpdate_free(minst->boneupd);
    if (minst->modelupd)
      ModelUpdate_free(minst->modelupd);

    lxMemGenFree(minst,sizeof(Modelnstance_t) + (sizeof(SkinObject_t)*minst->numSkinObjs));
    return NULL;
  }
  return minst;
}

// MODEL
//-------

int   Model_searchMesh(Model_t  *model, const char *name){
  int i;

  for (i = 0; i < model->numMeshObjects; i++){
    if (strcmp(model->meshObjects[i].name, name) == 0)
      return i;
  }
  return -1;
}

int   Model_linkBones(Model_t *model){
  return BoneSys_link(&model->bonesys);
}
void  Model_updateBones(Model_t *model,BoneUpdateType_t update){
  BoneSys_update(&model->bonesys,update, NULL);
}
void  Model_initBones(Model_t *model){
  BoneSys_init(&model->bonesys);
}

Mesh_t  *Model_getSkinMesh(Model_t *model, int skinID)
{
  int i;
  for (i = 0; i < model->numMeshObjects; i++) {
    if (model->meshObjects[i].skinID == skinID)
      return model->meshObjects[i].mesh;
  }

  return NULL;
}


void Model_initSkin(Model_t *model)
{
  int i,n,b,maxw;
  ResourceChunk_t *oldchunk = ResData_setChunkFromPtr(&model->resinfo);

  // go thru all skin modifiers
  if (model->modeltype != MODEL_DYNAMIC){
    // this leaks a bit of memory, as we always allocate this for better caching
    model->bonesys.refInvMatrices = NULL;
  }
  else
  for (n = 0; n< model->numSkins; n++){
    Skin_t *skin = &model->skins[n];
    Mesh_t *mesh = Model_getSkinMesh(model,n);
    skin->maxweights = 0;

    // we can use hardware skinning if we have enough space for matrices
    if ((model->vertextype==VERTEX_64_TEX4||model->vertextype==VERTEX_64_SKIN) && g_VID.gensetup.hasVprogs && skin->numBones < g_VID.gensetup.hwbones ){
      skin->hwskinning = LUX_TRUE;
      skin->boneMatrices = reszallocSIMD(sizeof(lxMatrix34)*g_VID.gensetup.hwbones);  // too much space?
      clearArray(skin->boneMatrices,g_VID.gensetup.hwbones);
      for (i = 0; i < g_VID.gensetup.hwbones; i++){
        skin->boneMatrices[i*12+0] = 1.0f;
        skin->boneMatrices[i*12+5] = 1.0f;
        skin->boneMatrices[i*12+10] = 1.0f;
      }
      model->vertextype = VERTEX_64_SKIN;
      mesh->vertextype = VERTEX_64_SKIN;
    }
    else{
      skin->hwskinning = LUX_FALSE;
    }

    // fix weights
    maxw = MODEL_MAX_WEIGHTS;
    if (g_VID.details == VID_DETAIL_LOW)
      maxw = 1;
    if (g_VID.details == VID_DETAIL_MED || skin->hwskinning)
      maxw = 2;

    // go thru all weighted vertices
    for (i = 0; i < skin->numSkinVertices; i++){
      SkinVertex_t *sv = &skin->skinvertices[i];
      float sumweight = 0;
      float sumsplit;
      int usedw = LUX_MIN(sv->numWeights,maxw);
      skin->maxweights = LUX_MAX(sv->numWeights,skin->maxweights);

      // first we check how much weight is used
      for (b = 0; b < usedw; b++){
        sumweight += sv->weights[b];
      }
      //then we split what is left
      sumsplit =  (1.0f - sumweight);

#if 1
      // proportionally
      for (b = 0; b < usedw; b++){
        sv->weights[b] += (sv->weights[b]/sumweight) *  sumsplit;
      }
#else
      // equally
      sumsplit /= (float)usedw;
      for (b = 0; b < usedw; b++){
        sv->weights[b] += sumsplit;
      }
#endif

      if (sv->numWeights > usedw)
        sv->numWeights = usedw;

      if (skin->hwskinning){
        // tangent holds info about indices & weights
        byte*   indices = mesh->vertexData64[i].blendi;
        ushort* weights = mesh->vertexData64[i].blendw;

        // zero rest
        for (b = usedw; b < MODEL_MAX_WEIGHTS; b++){
          sv->weights[b] = 0;
          sv->matrixIndex[b] = sv->matrixIndex[b-1];
        }

        for (b = 0; b < MODEL_MAX_WEIGHTS; b++){
          indices[b] = sv->matrixIndex[b]*3;
          weights[b] = sv->weights[b]*LUX_MUL_USHORT;
        }

      }
    }
    // copy refvertices;
    skin->outvertices = reszallocaligned(VertexSize(model->vertextype)*skin->numSkinVertices,32);
    memcpy(skin->outvertices,mesh->vertexData,VertexSize(model->vertextype)*skin->numSkinVertices);
  }
  ResourceChunk_activate(oldchunk);
}


// UPDATES THE VERTEX DATA BASED ON BONE TRANSFORMATIONS
//-------------------------------------------------------
void* SkinObject_run(SkinObject_t *skinobj, void *inVertices, int vtype,int HWskinning)
{
  int i,b;
  Skin_t  *skin;

  lxVector3 vecIn;
  lxVector3 vecOut;
  float *matrix;
  float *inPos;
  float *outPos;

  lxVertex64_t *in64;
  lxVertex64_t *out64;

  lxVertex32_t *in32;
  lxVertex32_t *out32;

  void *outVertices;

  float scale;
  int   calcTangent;

  SkinVertex_t *sv;
  float   *bonesSkinMatrix;
  float   *outmat;
  int     *matindex;

  skin = skinobj->skin;
  bonesSkinMatrix = skinobj->relMatricesT;

  // check if its the same state like last time
  if (skin->lastMatrices == bonesSkinMatrix &&
    skin->lastTime == *skinobj->time &&
    skin->lastVertices == inVertices){
    return skin->outvertices;
  }
  else{
    skin->lastMatrices = bonesSkinMatrix;
    skin->lastTime = *skinobj->time;
    skin->lastVertices = inVertices;
  }

  outVertices = skin->outvertices;


  if (HWskinning && skin->hwskinning){
    return inVertices;
  }

  sv = skin->skinvertices;

  // go thru all weighted vertices
#ifdef LUX_SIMD_SSE
  //TODO SSE
  if (LUX_FALSE  && !g_VID.useworst){
    // Use SIMD

  }
  else
#endif
  {
  if (vtype == VERTEX_64_TEX4 || vtype == VERTEX_64_SKIN)
  {
    in64 = inVertices;
    out64 = outVertices;
    if (!skin->hwskinning && in64->tangent[3])
      calcTangent = LUX_TRUE;
    else
      calcTangent = LUX_FALSE;
    for (i = 0; i < skin->numSkinVertices; i++,sv++, in64++, out64++){
      if (sv->numWeights <= 0)
        continue;

      inPos = in64->pos;
      outPos = out64->pos;

      // clear
      lxVector3Clear(vecOut);
      lxVector3Clear(outPos);

      // go thru all weights for this vertex  //MODEL_MAX_WEIGHTS
      // matrices are transposed !!
      lxVector3float_FROM_short(vecIn,in64->normal);

      matindex = sv->matrixIndex;
      outmat = sv->weights;
      for (b = 0; b < sv->numWeights; b++,matindex++,outmat++){
        scale = *outmat;
        matrix = &bonesSkinMatrix[skin->boneIndices[*matindex]];

        // normal
        vecOut[0] += (vecIn[0]*matrix[0]+vecIn[1]*matrix[1]+vecIn[2]*matrix[2])*scale;
        vecOut[1] += (vecIn[0]*matrix[4]+vecIn[1]*matrix[5]+vecIn[2]*matrix[6])*scale;
        vecOut[2] += (vecIn[0]*matrix[8]+vecIn[1]*matrix[9]+vecIn[2]*matrix[10])*scale;

        // pos
        outPos[0] += (inPos[0]*matrix[0]+inPos[1]*matrix[1]+inPos[2]*matrix[2]+matrix[3])*scale;
        outPos[1] += (inPos[0]*matrix[4]+inPos[1]*matrix[5]+inPos[2]*matrix[6]+matrix[7])*scale;
        outPos[2] += (inPos[0]*matrix[8]+inPos[1]*matrix[9]+inPos[2]*matrix[10]+matrix[11])*scale;
      }
      lxVector3short_FROM_float(out64->normal,vecOut);

      if (calcTangent){
        lxVector3float_FROM_short(vecIn,in64->tangent);
        lxVector3Clear(vecOut);
        matindex = sv->matrixIndex;
        outmat = sv->weights;
        for (b = 0; b < sv->numWeights; b++,matindex++,outmat++){
          //tangent
          vecOut[0] += (vecIn[0]*matrix[0]+vecIn[1]*matrix[1]+vecIn[2]*matrix[2])*scale;
          vecOut[1] += (vecIn[0]*matrix[4]+vecIn[1]*matrix[5]+vecIn[2]*matrix[6])*scale;
          vecOut[2] += (vecIn[0]*matrix[8]+vecIn[1]*matrix[9]+vecIn[2]*matrix[10])*scale;
        }
        lxVector3short_FROM_float(out64->tangent,vecOut);
      }

    }
  }
  else if (vtype == VERTEX_32_TEX2){
    in32 = inVertices;
    out32 = outVertices;
    for (i = 0; i < skin->numSkinVertices; i++,sv++, in32++, out32++){
      if (sv->numWeights <= 0)
        continue;

      inPos = in32->pos;
      outPos = out32->pos;

      // clear
      lxVector3Clear(outPos);

      // go thru all weights for this vertex  //MODEL_MAX_WEIGHTS
      matindex = sv->matrixIndex;
      outmat = sv->weights;
      for (b = 0; b < sv->numWeights; b++,matindex++,outmat++){
        scale = *outmat;
        matrix = &bonesSkinMatrix[skin->boneIndices[*matindex]];

        // pos
        outPos[0] += (inPos[0]*matrix[0]+inPos[1]*matrix[1]+inPos[2]*matrix[2]+matrix[3])*scale;
        outPos[1] += (inPos[0]*matrix[4]+inPos[1]*matrix[5]+inPos[2]*matrix[6]+matrix[7])*scale;
        outPos[2] += (inPos[0]*matrix[8]+inPos[1]*matrix[9]+inPos[2]*matrix[10]+matrix[11])*scale;
      }
    }
  }// VERTEX32_LIT
  else {
    in32 = inVertices;
    out32 = outVertices;
    for (i = 0; i < skin->numSkinVertices; i++,sv++, in32++, out32++){
      if (sv->numWeights <= 0)
        continue;


      inPos = in32->pos;
      outPos = out32->pos;

      // clear
      lxVector3Clear(outPos);
      lxVector3Clear(vecOut);

      // go thru all weights for this vertex  //MODEL_MAX_WEIGHTS
      lxVector3float_FROM_short(vecIn,in32->normal);

      matindex = sv->matrixIndex;
      outmat = sv->weights;
      for (b = 0; b < sv->numWeights; b++,matindex++,outmat++){
        scale = *outmat;
        matrix = &bonesSkinMatrix[skin->boneIndices[*matindex]];
        // normal
        vecOut[0] += (vecIn[0]*matrix[0]+vecIn[1]*matrix[1]+vecIn[2]*matrix[2])*scale;
        vecOut[1] += (vecIn[0]*matrix[4]+vecIn[1]*matrix[5]+vecIn[2]*matrix[6])*scale;
        vecOut[2] += (vecIn[0]*matrix[8]+vecIn[1]*matrix[9]+vecIn[2]*matrix[10])*scale;

        // pos
        outPos[0] += (inPos[0]*matrix[0]+inPos[1]*matrix[1]+inPos[2]*matrix[2]+matrix[3])*scale;
        outPos[1] += (inPos[0]*matrix[4]+inPos[1]*matrix[5]+inPos[2]*matrix[6]+matrix[7])*scale;
        outPos[2] += (inPos[0]*matrix[8]+inPos[1]*matrix[9]+inPos[2]*matrix[10]+matrix[11])*scale;
      }
      lxVector3short_FROM_float(out32->normal,vecOut);
    }
  }
  }


  return outVertices;
}

#undef Matrix44SkinCopy


int   Model_optimalMeshType(Model_t *model)
{
  int i;
  int nolock = LUX_FALSE;
  int reflect = LUX_FALSE;
  int shader = LUX_FALSE;
  int bumpmap = LUX_FALSE;
  int novbo;
  int nodl;
  int meshtype = MESH_VA;

  MeshObject_t *tempM;

  switch(model->orgtype) {
  case MODEL_STATIC_NODL_NOVBO:
  case MODEL_DYNAMIC_NODL_NOVBO:
    novbo = LUX_TRUE;
    nodl = LUX_TRUE;
    break;
  case MODEL_STATIC_NOVBO:
  case MODEL_DYNAMIC_NOVBO:
    novbo = LUX_TRUE;
    nodl = LUX_FALSE;
    break;
  case MODEL_STATIC_NODL:
  case MODEL_DYNAMIC_NODL:
    nodl = LUX_TRUE;
    novbo = LUX_FALSE;
    break;
  default:
    nodl = LUX_FALSE;
    novbo = LUX_FALSE;
    break;
  }

  for (i = 0; i < model->numMeshObjects;i++){
    tempM = &model->meshObjects[i];
    if (vidMaterial(tempM->texRID)){
      shader = LUX_TRUE;
      if (ResData_getMaterialShader(tempM->texRID)->shaderflag & SHADER_CUBETEXGEN )
        reflect = LUX_TRUE;
      if (ResData_getMaterialShader(tempM->texRID)->shaderflag & SHADER_TANGENTS && (model->vertextype == VERTEX_64_TEX4 || model->vertextype == VERTEX_64_SKIN)){
        bumpmap = LUX_TRUE;
      }
      if (ResData_getMaterialShader(tempM->texRID)->nolock )
        nolock = LUX_TRUE;
    }
  }


  // upgrade to the best
  if(!g_VID.useworst && model->orgtype != MODEL_ONLYDATA){
    // upgrade to VBO if we can
    if(g_VID.usevbos && !novbo && GLEW_ARB_vertex_buffer_object)
      meshtype = MESH_VBO;
    else if (model->modeltype == MODEL_STATIC || (model->modeltype == MODEL_DYNAMIC && !model->numSkins))
      meshtype = MESH_DL;

    // downgrade DL
    if (meshtype == MESH_DL){
      if (nodl || (g_VID.capIsCrap && shader) || (reflect && !GLEW_ARB_texture_cube_map) || (g_VID.gpuvendor == VID_VENDOR_ATI && bumpmap) ||(nolock)){
        meshtype = MESH_VA;
      }
    }
  }

  return meshtype;
}

void  Model_initGL(Model_t *model, int meshtype, int lmcoords, int vcolors){
  int i,n;
  Mesh_t  *mesh;
  MeshObject_t *meshobj;
  ModelType_t modeltype = model->modeltype;

  if (!meshtype)
    meshtype = Model_optimalMeshType(model);

  mesh = model->meshObjects[0].mesh;

  Model_clearGL(model,LUX_FALSE);

  vidBindBufferArray(NULL);
  vidBindBufferIndex(NULL);

  if (meshtype == MESH_VBO){
    uint  numTexIndex;
    uint  offsetTri;
    uint  offsetVert;

    // now we use the listID to store where our buffer is for each mesh
    // Vertex_Triangle row
    numTexIndex = 0;
    offsetTri = 0;
    offsetVert  = 0;


    // submit data
    for (i = 0; i < model->numMeshObjects; i++){
      mesh= model->meshObjects[i].mesh;
      mesh->meshtype = meshtype;

      // no instance, we commit, else we tell where to look for
      if (!mesh->instanceType){
        // vertex buffer
        if (!mesh->vid.vbo){
          HWBufferObject_t  *bo;
          offsetVert = ResData_getBO(&model->resinfo,
            RESOURCE_BO_VERTEX, (VertexSize(model->vertextype)*mesh->numAllocVertices),&bo,
            sizeof(lxVertex64_t));
          offsetVert/= VertexSize(model->vertextype);

          mesh->vid.vbo = &bo->buffer;

          //LUX_ASSERT(offsetVert + mesh->indexMax < 0xFFFF);
          mesh->vid.vbooffset = offsetVert;
          offsetVert += mesh->numVertices;
        }
        Mesh_submitVBO(mesh,LUX_FALSE,-1,-1);
      }
      else{
        mesh->vid.vbo = mesh->instance->vid.vbo;
        mesh->vid.vbooffset = mesh->instance->vid.vbooffset;
      }


      // no instance of tris, send as well
      if ( mesh->instanceType != INST_SHRD_VERT_TRIS){
        // index buffer
        if (!mesh->vid.ibo){
          HWBufferObject_t  *bo;

          offsetTri  = ResData_getBO(&model->resinfo,
            RESOURCE_BO_INDEX,  ((mesh->index16 ? sizeof(ushort) : sizeof(uint))*mesh->numAllocIndices),&bo,
            sizeof(uint));
          offsetTri /= (mesh->index16 ? sizeof(ushort) : sizeof(uint));

          mesh->vid.ibo = &bo->buffer;

          mesh->vid.ibooffset = offsetTri;
          offsetTri += mesh->numIndices;
        }
        Mesh_submitVBO(mesh,LUX_TRUE,-1,-1);
      }
      else{
        mesh->vid.ibo = mesh->instance->vid.ibo;
        mesh->vid.ibooffset = mesh->instance->vid.ibooffset;
      }


      vidCheckError();
    }
  }
  else if (meshtype == MESH_DL){
    // we gotta turn it off being STATIC_GL else it would try
    // to render from a non-existing display list

    // the display lists contain textures if the texid isnt a shader
    // in case there is a shader we set all texcoord pointers
    vidStateDefault();
    for (i = 0; i < model->numMeshObjects;i++){
      lxVertex64_t *vert64 = model->meshObjects[i].mesh->vertexData;
      lxVertex32_t *vert32 = model->meshObjects[i].mesh->vertexData;
      int vsize,isize;
      int numMaxStage;

      meshobj =&model->meshObjects[i];
      mesh = meshobj->mesh;
      mesh->vid.glID = glGenLists (1);

      vsize = (VertexSize(mesh->vertextype)*mesh->numVertices);
      isize = ((mesh->index16 ? sizeof(ushort) : sizeof(uint))*mesh->numIndices);

      glEnableClientState(GL_VERTEX_ARRAY);
      g_VID.drawsetup.setnormals = LUX_TRUE;

      if( !vcolors || (vidMaterial(meshobj->texRID) && ResData_getMaterialShader(meshobj->texRID)->renderflag & RENDER_NOVERTEXCOLOR)){
        //g_VID.state.renderflag &= ~RENDER_NOVERTEXCOLOR;
        vidNoVertexColor(LUX_TRUE);}
      else{
        //g_VID.state.renderflag |= RENDER_NOVERTEXCOLOR;
        vidNoVertexColor(LUX_FALSE);}

      glNewList(mesh->vid.glID, GL_COMPILE);

      numMaxStage = (vidMaterial(meshobj->texRID)) ? ResData_getMaterialShader(meshobj->texRID)->numMaxTexPass : (lmcoords ? 2 : 1);
      if (!meshobj->texRID)
        numMaxStage -= 1;

      if (vidMaterial(meshobj->texRID) && ResData_getMaterialShader(meshobj->texRID)->shaderflag & SHADER_TANGENTS){
        glVertexAttribPointerARB(VERTEX_ATTRIB_TANGENT,4, GL_SHORT, GL_TRUE, sizeof(lxVertex64_t),&vert64[0].tangent);
        glEnableVertexAttribArrayARB(VERTEX_ATTRIB_TANGENT);
      }
      for (n = 0; n < numMaxStage; n++){//g_VID.capTexUnits
        vidSelectTexture(n);
        vidTexCoordSource(VID_TEXCOORD_ARRAY,(vidMaterial(meshobj->texRID)) ?
          ResData_getMaterialShader(meshobj->texRID)->texchannels[n] : (lmcoords  && n == numMaxStage-1 ? VID_TEXCOORD_LIGHTMAP : VID_TEXCOORD_0));

      }
      mesh->meshtype = MESH_VA;
      Draw_Mesh_simple(mesh);
      mesh->meshtype = MESH_DL;

      if (vidMaterial(meshobj->texRID) && ResData_getMaterialShader(meshobj->texRID)->shaderflag & SHADER_TANGENTS){
        glDisableVertexAttribArrayARB(VERTEX_ATTRIB_TANGENT);
      }
      for (n = 0; n < numMaxStage; n++){
        vidSelectTexture(n);
        vidTexCoordSource(VID_TEXCOORD_NONE,VID_TEXCOORD_NOSET);
      }

      g_VID.drawsetup.setnormals = LUX_FALSE;

      glEndList();

      vidNoVertexColor(LUX_TRUE);

      vidCheckError();

      LUX_PROFILING_OP (g_Profiling.global.memory.viddlmem += vsize + isize);
    }
  }
  else if (meshtype == MESH_VA){
    for (i = 0; i < model->numMeshObjects;i++){
      meshobj =&model->meshObjects[i];
      mesh = meshobj->mesh;

      mesh->meshtype = MESH_VA;
    }
  }
  model->vcolors = vcolors;
  model->lmcoords = lmcoords;
}
/*
=================================================================
HELPERS
=================================================================
*/



static void Vertex64CreateTangentSpace(lxVertex64_t *a, lxVertex64_t *b, lxVertex64_t *c, lxVector3 T, lxVector3 B)
{
  lxVector3 p,q,normal;
  lxVector2 s,t;
  float r;

  // Calculate triangle's edges:
  lxVector3Sub(p,b->pos,a->pos);
  lxVector3Sub(q,c->pos,a->pos);

  s[0] = b->tex[0] - a->tex[0];
  s[1] = c->tex[0] - a->tex[0];

  t[0] = b->tex[1] - a->tex[1];
  t[1] = c->tex[1] - a->tex[1];


  r = 1.0F / (s[0] * t[1] - s[1] * t[0]);

  // tdir
  lxVector3Scale(B,q,t[0]);
  lxVector3Scale(T,p,t[1]);
  lxVector3Sub(T,T,B);
  lxVector3Scale(T,T,r);

  // sdir
  lxVector3Scale(normal,p,s[1]);
  lxVector3Scale(B,q,s[0]);
  lxVector3Sub(B,B,normal);
  lxVector3Scale(B,B,r);
}

void  Mesh_createTangents(struct Mesh_s *mesh)
{
  int a,b,c,n;

  lxVertex64_t *vert64 = NULL;

  lxVector3 tan,bin,normal;
  lxVector4 tangent;
  lxVector3 binormal;
  ushort *curindex;
  float *tanbin;
  short *pNormal;
  short *pTangent;
  float *pTan;
  float *pBin;
  float scale;
  int   indexstride;
  int   quad = LUX_FALSE;
  // so far only care for TRIANGLES
  if ((mesh->primtype != PRIM_TRIANGLES && mesh->primtype != PRIM_TRIANGLE_STRIP && mesh->primtype != PRIM_QUADS) || mesh->numGroups > 1)
    return;

  // if they were computed before dont care
  vert64 = (lxVertex64_t*)mesh->vertexData;
  if (vert64[0].tangent[3])
    return;


  tanbin = lxMemGenZalloc(sizeof(float)*6*mesh->numVertices);

  switch(mesh->primtype)
  {
  case PRIM_TRIANGLES:
    indexstride = 3;
    break;
  case PRIM_QUADS:
    indexstride = 2;quad = LUX_TRUE;
    break;
  case PRIM_TRIANGLE_STRIP:
    indexstride = 1;
    break;
  }

  curindex = mesh->indicesData16;
  for (n = 0; n < mesh->numTris; n++, curindex += indexstride){
    if (quad && n%2){
      a = *(curindex-2);
      b = *(curindex);
      c = *(curindex+1);
    }
    else{
      a = *curindex;
      b = *(curindex+1);
      c = *(curindex+2);
    }

    Vertex64CreateTangentSpace(&vert64[a],&vert64[b],&vert64[c],tan,bin);

    tanbin[a*6] += tan[0];    tanbin[b*6] += tan[0];    tanbin[c*6] += tan[0];
    tanbin[a*6+1] += tan[1];  tanbin[b*6+1] += tan[1];  tanbin[c*6+1] += tan[1];
    tanbin[a*6+2] += tan[2];  tanbin[b*6+2] += tan[2];  tanbin[c*6+2] += tan[2];

    tanbin[a*6+3] += bin[0];  tanbin[b*6+3] += bin[0];  tanbin[c*6+3] += bin[0];
    tanbin[a*6+4] += bin[1];  tanbin[b*6+4] += bin[1];  tanbin[c*6+4] += bin[1];
    tanbin[a*6+5] += bin[2];  tanbin[b*6+5] += bin[2];  tanbin[c*6+5] += bin[2];
  }
  // now normalize them all
  // tan1 = b
  // tan2 = t
  // we use tan and bin now for temp purposes
  for (n = 0; n < mesh->numVertices; n++){
    pNormal  = vert64[n].normal;
    pTangent = vert64[n].tangent;

    lxVector3float_FROM_short(normal,pNormal);

    pTan = &tanbin[n*6];
    pBin = &tanbin[n*6+3];


    //tangent = (tan - n * (n . tan)).Normalize();
    scale = lxVector3Dot(normal,pTan);
    lxVector3Scale(tan,normal,scale);
    lxVector3Sub(tangent,pTan,tan);
    lxVector3Normalized(tangent);
    //binormal = (bin - n * (n . bin) - tanNew * (tanNew . bin).Normalize();
    scale = lxVector3Dot(normal,pBin);
    lxVector3Scale(tan,normal,scale);
    lxVector3Sub(binormal,pBin,tan);
    scale = lxVector3Dot(tangent,pBin);
    lxVector3Scale(tan,tangent,scale);
    lxVector3Sub(binormal,binormal,tan);
    lxVector3Normalized(binormal);

    //tangent[a].w = ((n % tan) . bin < 0.0F) ? -1.0F : 1.0F;
    lxVector3Cross(tan,normal,pTan);
    tangent[3] = (lxVector3Dot(tan,binormal) < 0.0f) ? -1.0f : 1.0f;
    lxVector4short_FROM_float(pTangent,tangent);
  }
  lxMemGenFree(tanbin,sizeof(float)*6*mesh->numVertices);

}

void  Mesh_fixVertices(Mesh_t *mesh, lxMatrix44 matrix){
  lxVertex64_t *outvert64;
  lxVertex32_t *outvert32;
  lxVector3 normal;
  int i;

  switch(mesh->vertextype) {
  case VERTEX_64_TEX4:
  case VERTEX_64_SKIN:
    outvert64 = mesh->vertexData64;
    for (i = 0; i < mesh->numVertices; i++,outvert64++){
      lxVector3Transform1(outvert64->pos,matrix);
      lxVector3float_FROM_short(normal,outvert64->normal);
      lxVector3TransformRot1(normal,matrix);
      lxVector3Normalized(normal);
      lxVector3short_FROM_float(outvert64->normal,normal);
      lxVector3float_FROM_short(normal,outvert64->tangent);
      lxVector3TransformRot1(normal,matrix);
      lxVector3Normalized(normal);
      lxVector3short_FROM_float(outvert64->tangent,normal);
    }

    break;
  case VERTEX_32_TEX2:

    outvert32 = mesh->vertexData32;
    for (i = 0; i < mesh->numVertices; i++,outvert32++){
      lxVector3Transform1(outvert32->pos,matrix);
    }

    break;
  case VERTEX_32_NRM:

    outvert32 = mesh->vertexData32;
    for (i = 0; i < mesh->numVertices; i++,outvert32++){
      lxVector3Transform1(outvert32->pos,matrix);
      lxVector3float_FROM_short(normal,outvert32->normal);
      lxVector3TransformRot1(normal,matrix);
      lxVector3Normalized(normal);
      lxVector3short_FROM_float(outvert32->normal,normal);
    }

    break;
  default:
    break;
  }

}

//////////////////////////////////////////////////////////////////////////
// Clear

void Model_clearGL(Model_t *model, int full){
  Mesh_t *mesh;
  MeshObject_t *meshobj;
  int i;

  meshobj = model->meshObjects;
  for (i = 0; i < model->numMeshObjects; i++,meshobj++){
    mesh = meshobj->mesh;
    if (mesh->meshtype == MESH_DL){
      int vsize,isize;

      vsize = (VertexSize(mesh->vertextype)*mesh->numVertices);
      isize = ((mesh->index16 ? sizeof(ushort) : sizeof(uint))*mesh->numIndices);

      glDeleteLists(mesh->vid.glID,1);
      mesh->vid.glID = 0;

      LUX_PROFILING_OP (g_Profiling.global.memory.viddlmem += vsize + isize);
    }
    if (full){
      mesh->vid.vbo = mesh->vid.ibo = NULL;
      mesh->vid.vbooffset = mesh->vid.ibooffset = 0;
    }
  }
}

void  Model_clear(Model_t *model)
{
  Mesh_t *mesh;
  MeshObject_t *meshobj;
  int i;

  Model_clearGL(model,LUX_TRUE);

  meshobj = model->meshObjects;
  for (i = 0; i < model->numMeshObjects; i++,meshobj++){
    mesh = meshobj->mesh;
    if (vidMaterial(meshobj->texRID)){
      ResData_unrefResource(RESOURCE_MATERIAL,meshobj->texRID-VID_OFFSET_MATERIAL,model);
    }
    else if (meshobj->texRID > 0){
      ResData_unrefResource(RESOURCE_TEXTURE,meshobj->texRID,model);
    }
  }

  if (model->modeltype == MODEL_DYNAMIC){
    // by chance some animations were applied on us, remove us from the assigns list
    // from animations
    // TODO (unsure if needed)
  }
}

// user created mesh
void  Model_setMesh(Model_t*model, int meshid, char *name, int vertexcount, int indicescount ,char *texname)
{
  ResourceChunk_t *oldchunk = ResData_setChunkFromPtr(&model->resinfo);
  MeshObject_t *meshobj = &model->meshObjects[meshid];
  Mesh_t  *mesh;

  meshobj->skinID = -1;
  resnewstrcpy(meshobj->name,name);
  mesh = meshobj->mesh = reszalloc(sizeof(Mesh_t));
  mesh->meshtype = MESH_VA;
  mesh->vertextype = model->vertextype;
  mesh->origVertexData = mesh->vertexData = reszallocaligned(VertexSize(model->vertextype)*vertexcount,32);
  mesh->index16 = vertexcount <= BIT_ID_FULL16;
  mesh->indicesData = reszalloc((mesh->index16 ? sizeof(ushort) : sizeof(uint))*indicescount);
  mesh->numAllocIndices = indicescount;
  mesh->numAllocVertices = vertexcount;
  mesh->primtype = PRIM_TRIANGLES;
  if (texname){
    resnewstrcpy(meshobj->texname,name);
  }

  ResourceChunk_activate(oldchunk);
}

void  Model_setBone(Model_t *model, int boneid, char *name, float *refmatrix, int parentid)
{
  ResourceChunk_t *oldchunk = ResData_setChunkFromPtr(&model->resinfo);
  Bone_t      *bone = &model->bonesys.bones[boneid];

  resnewstrcpy(bone->name,name);
  lxMatrix44Copy(bone->refMatrix,refmatrix);
  lxVector3Set(bone->refInvScale,1,1,1);
  lxVector3Copy(bone->refPRS.scale,bone->refInvScale);
  lxMatrix44GetTranslation(refmatrix,bone->refPRS.pos);
  lxQuatFromMatrix(bone->refPRS.rot,refmatrix);
  bone->parentID = parentid;

  ResourceChunk_activate(oldchunk);
}

void  Model_setSkin(Model_t *model, int skinid, int skinverts, int bones)
{
  ResourceChunk_t *oldchunk = ResData_setChunkFromPtr(&model->resinfo);
  Skin_t      *skin = &model->skins[skinid];

  skin->numBones = bones;
  skin->numSkinVertices = skinverts;

  skin->skinvertices = reszalloc(sizeof(SkinVertex_t)*skinverts);
  skin->boneIndices = reszalloc(sizeof(uint)*bones);

  ResourceChunk_activate(oldchunk);
}

void  Model_allocBones(Model_t *model, int bonecount)
{
  model->bonesys.numBones = bonecount;
  if (!bonecount)
    return;
  model->bonesys.bones = reszallocSIMD(sizeof(Bone_t)*bonecount);
  model->bonesys.bonestraverse = reszalloc(sizeof(Bone_t*)*(bonecount));
  model->bonesys.absMatrices = reszallocSIMD(sizeof(lxMatrix44)*bonecount);
  model->bonesys.relMatricesT = reszallocSIMD(sizeof(lxMatrix34)*bonecount);
  model->bonesys.userAbsMatrices = reszallocSIMD(sizeof(lxMatrix44)*bonecount);
  model->bonesys.userRelMatricesT = reszallocSIMD(sizeof(lxMatrix34)*bonecount);
  model->bonesys.refInvMatrices = reszallocSIMD(sizeof(lxMatrix44)*bonecount);
}

void  Model_allocMeshObjs(Model_t* model, int meshcount)
{


  model->numMeshObjects = meshcount;
  if (!meshcount)
    return;
  model->meshObjects = reszalloc(sizeof(MeshObject_t)*meshcount);
}

void  Model_allocSkins(Model_t *model, int skincount)
{
  model->numSkins = skincount;
  if (!skincount)
    return;
  model->skins = reszalloc(sizeof(Skin_t)*skincount);
}

void  Model_updateBoundingBox(Model_t *model)
{
  lxVector3 temp;
  lxVector3 min,max;
  float *matrix;
  MeshObject_t *meshobj;
  Mesh_t  *mesh;
  int i,n;

  lxVector3Set(min,-1,-1,-1);
  lxVector3Set(max,1,1,1);

  // run thru all vertices set min/max and count tris/vert
  for (i = 0; i < model->numMeshObjects; i++){
    meshobj = &model->meshObjects[i];
    mesh = meshobj->mesh;
    if (!meshobj->bone)
      matrix = NULL;
    else
      matrix = &model->bonesys.absMatrices[meshobj->bone->ID];

    for (n = 0; n < mesh->numVertices; n++){
      if (matrix)
        if (model->vertextype == VERTEX_64_TEX4 || model->vertextype == VERTEX_64_SKIN)
          lxVector3Transform(temp,mesh->vertexData64[n].pos,matrix);
        else
          lxVector3Transform(temp,mesh->vertexData32[n].pos,matrix);
      else
        if (model->vertextype == VERTEX_64_TEX4 || model->vertextype == VERTEX_64_SKIN)
          lxVector3Copy(temp,mesh->vertexData64[n].pos);
        else
          lxVector3Copy(temp,mesh->vertexData32[n].pos);
      if(n == 0 && i ==0){
        lxVector3Copy(max,temp);
        lxVector3Copy(min,max);
      }
      max[0] = (max[0]> temp[0])? max[0] : temp[0];
      max[1] = (max[1]> temp[1])? max[1] : temp[1];
      max[2] = (max[2]> temp[2])? max[2] : temp[2];

      min[0] = (min[0]< temp[0])? min[0] : temp[0];
      min[1] = (min[1]< temp[1])? min[1] : temp[1];
      min[2] = (min[2]< temp[2])? min[2] : temp[2];
    }
  }
  memset(&model->bbox,0,sizeof(lxBoundingBox_t));
  lxVector3Copy(model->bbox.min,min);
  lxVector3Copy(model->bbox.max,max);
}

int   Model_searchMeshPtr(Model_t*model,struct Mesh_s  *mesh)
{
  int i;
  MeshObject_t *meshobj = model->meshObjects;
  for (i = 0; i<model->numMeshObjects; i++,meshobj++){
    if (meshobj->mesh==mesh)
      return i;
  }
  return -1;
}

void  Model_initVertices(Model_t*model)
{
  static lxMatrix44SIMD mat;

  MeshObject_t *meshobj;
  Mesh_t  *mesh;
  int i;
  int meshid;
  uchar *instanced;

  instanced = bufferzalloc(sizeof(uchar)*(model->numMeshObjects+1));
  instanced++;
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

  if (model->modeltype == MODEL_STATIC){
    meshobj = model->meshObjects;
    for (i=0; i < model->numMeshObjects; i++,meshobj++){
      mesh = meshobj->mesh;
      if (mesh->instanceType){
        // we dont set the instance flag for this one, as we can precompile it
        // because matrices are shared too
        meshid = Model_searchMeshPtr(model,mesh->instance);
        LUX_ASSERT(meshid>=0);
        if (mesh->instanceType == INST_SHRD_VERT_PARENT_MAT)
          meshobj->bone = model->meshObjects[meshid].bone;
        else
          instanced[meshid] = LUX_TRUE;
      }
    }
    meshobj = model->meshObjects;
    for (i=0; i < model->numMeshObjects; i++,meshobj++){
      mesh = meshobj->mesh;

      // not a instance and not instanced we can precompute all
      if (!mesh->instanceType && !instanced[i]){
        lxMatrix44IdentitySIMD(mat);
        if (meshobj->bone->refPRS.scale[0])
          lxMatrix44SetScale(mat,meshobj->bone->refPRS.scale);
        lxMatrix44Multiply2SIMD(&model->bonesys.absMatrices[meshobj->bone->ID],mat);
        Mesh_fixVertices(mesh,mat);
        meshobj->bone = NULL;
      }
      else if (mesh->instanceType == INST_SHRD_VERT_PARENT_MAT && !instanced[Model_searchMeshPtr(model,mesh->instance)])
        meshobj->bone = NULL;
      else if (!mesh->instanceType && model->bonesys.bones[meshobj->bone->ID].refPRS.scale[0]){
        // remove scale
        lxMatrix44IdentitySIMD(mat);
        lxMatrix44SetScale(mat,meshobj->bone->refPRS.scale);
        Mesh_fixVertices(mesh,mat);
      }
    }
  }
  else if (model->modeltype == MODEL_DYNAMIC)
  {
    for (i=0; i < model->numMeshObjects; i++){
      meshobj = &model->meshObjects[i];
      mesh = meshobj->mesh;


      // actually this is a HACK because we just take local scale...
      if (!mesh->instanceType && meshobj->bone && meshobj->bone->refPRS.scale[0]){
        // remove scale
        lxMatrix44IdentitySIMD(mat);
        lxMatrix44SetScale(mat,meshobj->bone->refPRS.scale);
        Mesh_fixVertices(mesh,mat);
      }
      if (meshobj->skinID>=0){
        // skin takes care of all deformation
        // however we need to precompute all vertices
        Mesh_fixVertices(meshobj->mesh,&model->bonesys.absMatrices[meshobj->bone->ID]);
        meshobj->bone = NULL;
      }

    }
  }

  bufferclear();
}

int Model_initStats(Model_t*model)
{
  MeshObject_t *meshobj;
  Mesh_t  *mesh;
  int i,tri,vert,ind;

  tri = vert = ind =0;
  // run thru all vertices set min/max and count tris/vert
  for (i = 0; i < model->numMeshObjects; i++){
    meshobj = &model->meshObjects[i];
    mesh = meshobj->mesh;

    if (!mesh->instanceType)
      vert+= mesh->numAllocVertices;
    if (mesh->instanceType != INST_SHRD_VERT_TRIS)
      ind += mesh->numAllocIndices;

    tri += mesh->numTris;
  }
  model->numIndicesTotal = ind;
  model->numVerticesTotal = vert;

  return tri;
}

//////////////////////////////////////////////////////////////////////////
// LOADING

void ModelLoader_setPrescale(float scale[3])
{
  lxVector3Copy(l_ModelData.scale,scale);
  l_ModelData.usescale = scale[0] != 1.0f || scale[1] != 1.0f || scale[2] != 1.0f;
}

int  ModelLoader_getPrescale(float scale[3])
{
  lxVector3Copy(scale,l_ModelData.scale);
  return l_ModelData.usescale;
}

int ModelLoader_postInit(Model_t *model)
{
  int i,n,tri;
  ushort    *index;
  uint    *indexInt;

  // prescale bones
  if (l_ModelData.usescale){
    for (i = 0; i < model->bonesys.numBones; i++){
      Bone_t *bone = &model->bonesys.bones[i];
      lxVector3Mul(&bone->refMatrix[12],&bone->refMatrix[12],l_ModelData.scale);
      lxVector3Mul(bone->refPRS.pos,bone->refPRS.pos,l_ModelData.scale);
    }
  }


  // link our bones
  if (Model_linkBones(model) == LUX_FALSE)
    lprintf("WARNING model post load: bones not linked correctly\n");
  else
    Model_initBones(model); // time to Initialize our bones' relative matrices


  // create mesh instances
  // and precompile if we can
  for (i=0; i < model->numMeshObjects; i++){
    MeshObject_t  *meshobj = &model->meshObjects[i];
    Mesh_t      *mesh = meshobj->mesh;
    int *verts;
    int numverts = 0;

    dlprintf("\tMesh: %s\n",meshobj->name);
    if (meshobj->texRID){
      dlprintf("\t Material: %s\n",meshobj->texname);
    }
    meshobj = &model->meshObjects[i];
    mesh = meshobj->mesh;
    meshobj->skinID = model->modeltype == MODEL_DYNAMIC ? meshobj->skinID : -1;
    if (meshobj->skinID >= 0){
      dlprintf("\t Skinned\n");
    }
    if (mesh->instanceType){
      mesh->numVertices = mesh->instance->numVertices;
      mesh->numAllocVertices =  mesh->instance->numAllocVertices;
      mesh->vertexData = mesh->instance->vertexData;
      mesh->primtype = mesh->instance->primtype;

      dlprintf("\t Vertices:   \tshared");

      if (mesh->instanceType == INST_SHRD_VERT_TRIS){
        mesh->indicesData16 = mesh->instance->indicesData16;
        mesh->numGroups = mesh->instance->numGroups;
        mesh->indicesGroupLength = mesh->instance->indicesGroupLength;
        mesh->numIndices = mesh->instance->numIndices;
        dlprintf("\tIndices: shared\n");
      }
      else{
        dlprintf("\tIndices: %d\n",mesh->numIndices);
      }

    }
    else{
      // prescale verts
      if (l_ModelData.usescale){
        static lxMatrix44SIMD scalemat;
        lxMatrix44IdentitySIMD(scalemat);
        lxMatrix44SetScale(scalemat,l_ModelData.scale);
        VertexArray_transform(mesh->vertexData, mesh->vertextype, mesh->numVertices,scalemat);
      }
      dlprintf("\t Vertices: %d",mesh->numVertices);
      dlprintf("\tIndices: %d\n",mesh->numIndices);
    }



    mesh->origVertexData = mesh->vertexData;


    mesh->indexMax = 0;
    mesh->indexMin = BIT_ID_FULL32;
    bufferclear();
    verts = buffermalloc(sizeof(int)*mesh->numIndices);
    LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

    if (mesh->instanceType == INST_SHRD_VERT_PARENT_MAT ||
      mesh->instanceType == INST_SHRD_VERT)
    {
      if (mesh->index16){
        index = mesh->indicesData16;
        for (n = 0; n < mesh->numIndices; n++,index++){
          mesh->indexMin = LUX_MIN(mesh->indexMin,*index);
          mesh->indexMax = LUX_MAX(mesh->indexMax,*index);
          lxArrayFindOrAdd32(verts,&numverts,*index,mesh->numVertices);
        }
      }
      else{
        indexInt = mesh->indicesData32;
        for (n = 0; n < mesh->numIndices; n++,indexInt++){
          mesh->indexMin = LUX_MIN(mesh->indexMin,*indexInt);
          mesh->indexMax = LUX_MAX(mesh->indexMax,*indexInt);
          lxArrayFindOrAdd32(verts,&numverts,*indexInt,mesh->numVertices);
        }
      }
    }
    else{
      mesh->indexMin = 0;
      mesh->indexMax = mesh->numVertices-1;
    }

    if (mesh->instanceType)
      mesh->numVertices = numverts;

    switch(mesh->primtype){
      case PRIM_TRIANGLES:
        mesh->numTris = mesh->numIndices/3;
        break;
      case PRIM_TRIANGLE_STRIP:
        mesh->numTris = mesh->numIndices - (mesh->numGroups*2);
        break;
      case PRIM_QUADS:
        mesh->numTris = mesh->numIndices/2;
        break;
      case PRIM_QUAD_STRIP:
        mesh->numTris = mesh->numIndices - (mesh->numGroups*3);
        break;
    }
  }

  // create minimal parent lists for skins
  for (i=0; i < model->numSkins; i++){
    Skin_t  *skin = &model->skins[i];
    int *bones;
    int numbones = 0;
    int w;

    bufferclear();
    bones = bufferzalloc(sizeof(int)*model->bonesys.numBones);
    LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

    // mark used
    for (n = 0; n < skin->numSkinVertices; n++){
      SkinVertex_t *svert = &skin->skinvertices[n];
      for (w = 0 ; w < svert->numWeights; w++){
        bones[svert->matrixIndex[w]] = LUX_TRUE;
      }
    }
    // check used
    for (n = 0; n < model->bonesys.numBones; n++){
      if (bones[n]){
        bones[n] = numbones++;
      }
      else{
        bones[n] = -1;
      }
    }
    // and fill boneindex
    skin->boneIndices = reszalloc(sizeof(uint)*numbones);
    skin->numBones = numbones;
    for (n = 0; n < model->bonesys.numBones; n++){
      if (bones[n] >= 0){
        skin->boneIndices[bones[n]] = n*12;
      }
    }
    // reassign matrixIndex
    for (n = 0; n < skin->numSkinVertices; n++){
      SkinVertex_t *svert = &skin->skinvertices[n];
      for (w = 0 ; w < svert->numWeights; w++){
        svert->matrixIndex[w] = bones[svert->matrixIndex[w]];
      }
    }
  }


  Model_initVertices(model);

  // stats
  tri = Model_initStats(model);

  Model_updateBoundingBox(model);

  lxVector3Copy(model->prescale,l_ModelData.scale);

  dlprintf("\tVertices: %d\tTris: %d\n",model->numVerticesTotal,tri);

  return LUX_TRUE;

}
