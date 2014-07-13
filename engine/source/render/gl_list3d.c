// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "gl_list3d.h"
#include "gl_list2d.h"
#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../scene/actorlist.h"
#include "../scene/scenetree.h"
#include "../scene/vistest.h"



// GLOBALS
List3D_t g_List3D;
// LOCALS
static lxVector4    l_vector;
List3DDrawData_t  l_l3ddraw;

static int List3DNode_activatePsys(const int resRID, List3DSet_t *l3dset);
static int List3DNode_activatePCloud(const int resRID, List3DSet_t *l3dset);


#define MAINID(node)  (node->setID)

#define UPDATE_VISOBJECT(node)  \
  if (Reference_get(node->linkReference,node->linkObject)){\
    node->linkVisobject = node->linkObject->visobject;\
  }\
  else{\
    node->linkVisobject = NULL;\
  }

//////////////////////////////////////////////////////////////////////////
// Matrix44Updates

#define Matrix44_runRotationLocks(matrix,lock) \
  if ((lock) & ROTATION_LOCK_ALL)\
    lxMatrix44ClearRot(matrix);\
  else if ((lock) > ROTATION_LOCK_ALL){\
    lxMatrix44ToEulerZYX((matrix),l_vector);\
    if ((lock) & ROTATION_LOCK_X)\
      l_vector[0] = 0;\
    if ((lock) & ROTATION_LOCK_Y)\
      l_vector[1] = 0;\
    if ((lock) & ROTATION_LOCK_Z)\
      l_vector[2] = 0;\
    lxMatrix44FromEulerZYXFast((matrix),l_vector);\
  }

#define UPD_MATIF     lxMatrix44CopySIMD(node->finalMatrix,node->linkObject->matrix);
//#define UPD_MATIF     if (node->linkType) Matrix44Interface_getElementsCopy(node->linkObject->matrixIF,node->finalMatrix); else Matrix44CopySIMD(node->finalMatrix,node->linkObject->matrix);

#define UPD_PARENT      lxMatrix44CopySIMD(node->finalMatrix,node->parent->finalMatrix);
#define UPD_PARENT_LINK   node->linkVisobject = node->parent->linkVisobject; node->linkObject = node->parent->linkObject;

#define UPD_PARENTBONE    lxMatrix44MultiplySIMD(node->finalMatrix,node->parent->finalMatrix,node->boneMatrix);
#define UPD_BONE      lxMatrix44Multiply1SIMD(node->finalMatrix,node->boneMatrix);

#define UPD_INH       Matrix44_runRotationLocks(node->finalMatrix,node->inheritLocks);

#define UPD_MATIFLOCAL    lxMatrix44MultiplySIMD(node->finalMatrix,node->linkObject->matrix,node->localMatrix);
#define UPD_PARENTLOCAL   lxMatrix44MultiplySIMD(node->finalMatrix,node->parent->finalMatrix,node->localMatrix);
#define UPD_LOCAL     lxMatrix44Multiply1SIMD(node->finalMatrix,node->localMatrix);



static int LUX_FASTCALL list3d_upd_unlinked(List3DNode_t *node){
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd(List3DNode_t *node){
  UPD_MATIF;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_inh(List3DNode_t *node){
  UPD_MATIF;
  UPD_INH;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_local(List3DNode_t *node){
  UPD_MATIFLOCAL;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_inh_local(List3DNode_t *node){
  UPD_MATIF;
  UPD_INH;
  UPD_LOCAL;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_parent(List3DNode_t *node){
  UPD_PARENT_LINK;
  UPD_PARENT;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_parent_inh(List3DNode_t *node){
  UPD_PARENT_LINK;
  UPD_PARENT;
  UPD_INH;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_parent_local(List3DNode_t *node){
  UPD_PARENT_LINK;
  UPD_PARENTLOCAL;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_parent_inh_local(List3DNode_t *node){
  UPD_PARENT_LINK;
  UPD_PARENT;
  UPD_INH;
  UPD_LOCAL;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_parent_bone(List3DNode_t *node){
  UPD_PARENT_LINK;
  UPD_PARENTBONE;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_parent_bone_inh(List3DNode_t *node){
  UPD_PARENT_LINK;
  UPD_PARENTBONE;
  UPD_INH;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_parent_bone_local(List3DNode_t *node){
  UPD_PARENT_LINK;
  UPD_PARENTBONE;
  UPD_LOCAL;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_parent_bone_inh_local(List3DNode_t *node){
  UPD_PARENT_LINK;
  UPD_PARENTBONE;
  UPD_INH;
  UPD_LOCAL;
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_manual_world(List3DNode_t *node){
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_upd_parent_manual_world(List3DNode_t *node){
  UPD_PARENT_LINK;
  return LUX_TRUE;
}

#undef UPD_LOCAL
#undef UPD_BONE
#undef UPD_PARENT
#undef UPD_INH
#undef UPD_MATIF

// keep in synch with header
static  const List3DNodeIntFunc_t l_L3DupdateFuncs[] =
  {
    {list3d_upd_unlinked},
    {list3d_upd},
    {list3d_upd_inh},
    {list3d_upd_local},
    {list3d_upd_inh_local},
    {list3d_upd_parent},
    {list3d_upd_parent_inh},
    {list3d_upd_parent_local},
    {list3d_upd_parent_inh_local},
    {list3d_upd_parent_bone},
    {list3d_upd_parent_bone_inh},
    {list3d_upd_parent_bone_local},
    {list3d_upd_parent_bone_inh_local},
    {list3d_upd_manual_world},
    {list3d_upd_parent_manual_world},
  };

//////////////////////////////////////////////////////////////////////////
// Special Updates

static void LUX_FASTCALL list3d_spc_model_anim(List3DNode_t *node)
{
  int i;
  Modelnstance_t *minst = node->minst;
  ModelUpdate_t *mupd = minst->modelupd;

  if (g_LuxTimer.time <= minst->time)
    return;

  if (minst->boneupd)
    BoneSysUpdate_run(minst->boneupd,node->finalMatrix);
  if (mupd){
    ModelUpdate_run(mupd);
    // exchange vertices if needed
    for (i = 0; i < mupd->vertexcont->numVertexDatas; i++){
      if (mupd->dirtyVDatas[i])
        node->drawNodes[i].overrideVertices = mupd->vertexcont->vertexDatas[i];
      else
        node->drawNodes[i].overrideVertices = NULL;
    }
  }
  minst->time = g_LuxTimer.time;
}


static const List3DNodeVoidFunc_t l_L3DupdateSpecialFuncs[]=
{
  {NULL},
  {list3d_spc_model_anim},
};



// helpers start
LUX_INLINE int list3d_proc_drawnodes(List3DNode_t *node){
  DrawNode_t *drawnode = node->drawNodes;
  int i;
  void* ptr;


  if (node->checkptr &&
    ((ptr=ResData_getInfo(node->checkRID,node->checkType)) != node->checkptr)
    )
  { // ATM it is only needed for MODEL
    if (!ptr){
      //for (i = 0; i < node->numDrawNodes; i++,drawnode++)
      //  drawnode->renderflag |= RENDER_NODRAW;
      return LUX_FALSE;
    }
    if (node->nodeType == LUXI_CLASS_L3D_MODEL){
      Model_t *model = (Model_t*)ptr;
      Modelnstance_updatePtrs(node->minst);
      if(node->drawNodes->type!=DRAW_NODE_BATCH_WORLD){
        for (i = 0; i < node->numDrawNodes; i++,drawnode++){
          drawnode->draw.mesh = model->meshObjects[i].mesh;
          // only reference pointers might have changed
          if (drawnode->bonematrix && !node->minst->boneupd)
            drawnode->bonematrix = &model->bonesys.refRelMatricesT[model->meshObjects[i].bone->ID];
        }
        drawnode = node->drawNodes;
      }
    }

    node->checkptr = ptr;
  }

  for (i = 0; i < node->numDrawNodes; i++,drawnode++){
    int lightcnt = (drawnode->draw.state.renderflag & RENDER_SUNLIT) ? 1 : 0;
    lightcnt = (drawnode->draw.state.renderflag & RENDER_FXLIT) ? node->enviro.numFxLights+lightcnt*4 : lightcnt;
    if (lightcnt)
      drawnode->draw.state.renderflag |= RENDER_LIT;
    else
      drawnode->draw.state.renderflag &= ~RENDER_LIT;

    lightcnt <<= DRAW_SORT_SHIFT_LIGHTCNT;

    // we have 12 bits for renderflag, current length is 15, so shift back
    drawnode->sortkey.value = drawnode->sortID + lightcnt + ((drawnode->draw.state.renderflag & (RENDER_LASTFLAG-1))>>DRAW_SORT_SHIFT_RENDERFLAG);
  }

  return LUX_TRUE;
}

// returns number of lights
#define list3d_proc_lights(node)  \
  if (node->enviro.needslight && (node->enviro.numFxLights=node->vislightres->numLights))\
    node->enviro.fxLights = node->vislightres->lights


LUX_INLINE void list3d_proc_proj(List3DNode_t *node){
  List3DSet_t   *l3dset = &g_List3D.drawSets[MAINID(node)];
  int i;

  int bitID = node->linkVisobject->visset.projectorVis & node->enviro.projectorflag;

  node->enviro.numProjectors = 0;
  // must check if inside maximum range
  if (bitID && (g_List3D.projCurrent+LIST3D_SET_PROJECTORS) < g_List3D.projBufferArrayEnd){
    node->enviro.projectors = g_List3D.projCurrent;
    for (i = 0; i < LIST3D_SET_PROJECTORS; i++){
      if (bitID & 1<<i){
        node->enviro.projectors[node->enviro.numProjectors++] = l3dset->projLookUp[i];
        g_List3D.projCurrent++;
      }
    }
  }
}


// helpers end
static int LUX_FASTCALL list3d_proc_none(List3DNode_t *node){
  return LUX_FALSE;
}
static int LUX_FASTCALL list3d_proc_particle_grp(List3DNode_t *node){
  if (!g_Draws.drawNoPrts)
    ParticleGroup_update(node->pgroup,node->finalMatrix,node->renderscale);
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_proc_particle_emit(List3DNode_t *node){
  if (!g_Draws.drawNoPrts)
    ParticleEmitter_update(node->pemitter,node->finalMatrix,node->renderscale);
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_proc_levelmodel(List3DNode_t *node){
  LevelModel_updateL3D(node);
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_proc_always(List3DNode_t *node){
  list3d_proc_drawnodes(node);
  return LUX_TRUE;
}
static int LUX_FASTCALL list3d_proc_simple(List3DNode_t *node){
  return list3d_proc_drawnodes(node);
}
static int LUX_FASTCALL list3d_proc_simple_lights(List3DNode_t *node){
  list3d_proc_lights(node);
  return list3d_proc_drawnodes(node);
}
static int LUX_FASTCALL list3d_proc_simple_proj(List3DNode_t *node){
  list3d_proc_proj(node);
  return list3d_proc_drawnodes(node);
}
static int LUX_FASTCALL list3d_proc_simple_proj_lights(List3DNode_t *node){
  list3d_proc_proj(node);
  list3d_proc_lights(node);
  return list3d_proc_drawnodes(node);
}


static  const List3DNodeIntFunc_t l_L3DprocessFuncs[] =
{
  {list3d_proc_none},
  {list3d_proc_particle_grp},
  {list3d_proc_particle_emit},
  {list3d_proc_levelmodel},
  {list3d_proc_always},
  {list3d_proc_simple},
  {list3d_proc_simple_lights},
  {list3d_proc_simple_proj},
  {list3d_proc_simple_proj_lights},

};

//////////////////////////////////////////////////////////////////////////
// Main Updates

int List3DNode_updateUp_matrix_recursive(List3DNode_t*node)
{ // function is recursive

  // !Reference_isValid(node->linkReference) ||
  if (node->updFrame == g_VID.frameCnt)
    return LUX_FALSE;

  if (node->parent){
    node->update |= List3DNode_updateUp_matrix_recursive(node->parent);
  }
  else
    node->update |= (node->linkType);

  node->updFrame = g_VID.frameCnt;

  if (node->update){
    l_L3DupdateFuncs[node->updateType].func(node);
    if (node->specUpdType)
      l_L3DupdateSpecialFuncs[node->specUpdType].func(node);
    node->update = LUX_FALSE;
    return LUX_TRUE;
  }
  else
    return LUX_FALSE;
}

// updatetype = 0 none 1 force -1 novistest
static void List3DNode_update_recursive(List3DNode_t *node, const enum32 viscameras,  int updatetype)
{
  List3DNode_t *browse;

  node->viscameras = viscameras & node->visflag;
  if (updatetype < 0){
    if (node->drawnFrame == g_VID.frameCnt) return;
    List3DNode_updateUp_matrix_recursive(node);
  }

  updatetype = node->update = (updatetype>0) | node->update;

  if ((node->update || node->boneMatrix) && !l_L3DupdateFuncs[node->updateType].func(node))
    return;
  if (node->specUpdType)
    l_L3DupdateSpecialFuncs[node->specUpdType].func(node);

  if (viscameras && l_L3DprocessFuncs[node->processType].func(node)){
    *(l_l3ddraw.drawCurrent++) = node;
  }
  node->update = LUX_FALSE;
  node->drawnFrame = g_VID.frameCnt;
  if (node->lookat) lxMatrix44GetTranslation(node->finalMatrix,node->finalPos);


  // do the same for children
  lxListNode_forEach(node->childrenListHead,browse)
    LUX_ASSERT(browse);
    List3DNode_update_recursive(browse,viscameras,updatetype);
  lxListNode_forEachEnd(node->childrenListHead,browse);

}

void List3DNode_update(List3DNode_t *node, const enum32 viscameras){
  List3DNode_update_recursive(node,viscameras,1);
}

void  List3DSet_updateNodes(List3DSet_t *lset){
  List3DNode_t *node;
  List3DNode_t **nodeptr;
  enum32  viscameras;

  // bufferalloc
  l_l3ddraw.drawCurrent = l_l3ddraw.drawBufferArray = rpoolmalloc(sizeof(List3DNode_t*)*lset->numNodes);
  LUX_PROFILING_OP_MAX(g_Profiling.perframe.scratch.renderpoolmax,rpoolgetinuse());

  // run thru all visible
  nodeptr = lset->visibleBufferArray;
  while (node = *(nodeptr++)){
    // it must be linked to something
    viscameras = node->linkVisobject->cameraVis;
    if (viscameras & node->visflag)
      List3DNode_update_recursive(node,viscameras,0);
  }
  // run thru all novistests
  lxLN_forEach(lset->novistestListHead,node,visnext,visprev)
    //if (Reference_get(node->linkReference,node->linkObject))
      List3DNode_update_recursive(node,BIT_ID_FULL32,-1);
  lxLN_forEachEnd(lset->novistestListHead,node,visnext,visprev);

  *l_l3ddraw.drawCurrent = NULL;

  rpoolsetbegin(++l_l3ddraw.drawCurrent);

  LUX_PROFILING_OP_MAX(g_Profiling.global.nodes.maxprojptrs,(g_List3D.projCurrent-g_List3D.projBufferArray));
}

void  List3DSet_updateAll(List3DSet_t *lset)
{
  List3DNode_t *node;

  lxListNode_forEach(lset->nodeListHead,node)
    //if (Reference_get(node->linkReference,node->linkObject))
      List3DNode_update_recursive(node,0,1);
  lxListNode_forEachEnd(lset->nodeListHead,node);
}

void  List3D_updateAll()
{
  List3DSet_t *lset;
  int i;

  lset = g_List3D.drawSets;
  for (i = 0; i < LIST3D_SETS; i++,lset++){
    List3DSet_updateAll(lset);
  }

}

//////////////////////////////////////////////////////////////////////////
// LIST3DNodes New/Dest
// --------------

static void List3DNode_free(List3DNode_t *node)
{
  List3DNode_t *browse;
  DrawNode_t  *dnode;
  int i;

  switch(node->nodeType) {
  case LUXI_CLASS_L3D_MODEL:
    Modelnstance_unref(node->minst);
    // so that batchbuffer knows they are unused
    if (node->compileflag & LIST3D_COMPILE_BATCHED){
      dnode = node->drawNodes;
      for (i = 0; i < node->numDrawNodes; i++,dnode++){
        if (dnode->type == DRAW_NODE_BATCH_WORLD)
          dnode->draw.mesh->meshtype = MESH_UNSET;
      }
    }
    break;
  case LUXI_CLASS_L3D_CAMERA:
    Camera_free(node->cam);
    break;
  case LUXI_CLASS_L3D_PEMITTER:
    ParticleEmitter_free(node->pemitter);
    break;
  case LUXI_CLASS_L3D_PGROUP:
    ParticleGroup_free(node->pgroup);
    break;
  case LUXI_CLASS_L3D_PRIMITIVE:
    Reference_releaseCheck(node->primitive->usermesh);
    lxMemGenFree(node->primitive,sizeof(List3DPrimitive_t));
    break;
  case LUXI_CLASS_L3D_LEVELMODEL:
    Reference_release(node->lmdl->scenenodes[0]->link.reference);
    LevelModel_free(node->lmdl);
    break;
  case LUXI_CLASS_L3D_TEXT:
    PText_free(node->ptext);
    break;
  case LUXI_CLASS_L3D_LIGHT:
    Light_free(node->light);
    break;
  case LUXI_CLASS_L3D_PROJECTOR:
    Projector_free(node->proj);
    break;
  case LUXI_CLASS_L3D_TRAIL:
    Trail_free(node->trail);
    break;
  case LUXI_CLASS_L3D_SHADOWMODEL:
    ShadowModel_free(node->smdl);
    break;
  default:
    break;
  }

  if (node->enviro.lmtexmatrix)
    lxMemGenFree(node->enviro.lmtexmatrix,sizeof(lxMatrix44));


  while (node->childrenListHead){
    browse = node->childrenListHead;
    //lxListNode_popBack(node->childrenListHead,browse);
    List3DNode_unlink(browse);
  }


  Reference_invalidate(node->reference);
  node->reference = NULL;

  List3DNode_unlink(node);

  dnode = node->drawNodes;
  for (i = 0; i < node->numDrawNodes; i++,dnode++){
    if (dnode->draw.matobj)
      MaterialObject_free(dnode->draw.matobj);
  }

  if(node->numDrawNodes){
    lxMemGenFree(node->drawNodes,sizeof(DrawNode_t)*node->numDrawNodes);
  }

  if (node->name){
    genfreestr(node->name);
  }

  g_List3D.drawSets[node->setID].numNodes--;
  genfreeSIMD(node,sizeof(List3DNode_t));

  LUX_PROFILING_OP (g_Profiling.global.nodes.l3ds--);
}
void RList3DNode_free (lxRl3dnode ref)
{
  List3DNode_free((List3DNode_t*)Reference_value(ref));
}

static List3DNode_t* List3DNode_new(const char *name, const uint setID)
{
  List3DNode_t *node = genzallocSIMD(sizeof(List3DNode_t));
  LUX_SIMDASSERT((size_t)((List3DNode_t*)node)->finalMatrix % 16 == 0);
  LUX_ASSERT(node);

  memset(node,0,sizeof(List3DNode_t));
  lxListNode_init(node);
  node->unlinked = LUX_TRUE;
  node->update = LUX_TRUE;
  node->setID = setID;
  node->enviro.lightmapRID = -1;
  node->enviro.lmtexmatrix = NULL;
  node->visflag = BIT_ID_1;
  Reference_reset(node->linkReference);
  Reference_reset(node->reference);
  node->linkType = LINK_UNSET;

  g_List3D.drawSets[setID].numNodes++;

  if (name){
    gennewstrcpy(node->name,name);
  }

  lxMatrix44IdentitySIMD(node->localMatrix);
  lxMatrix44IdentitySIMD(node->finalMatrix);

  lxVector3Set(node->renderscale,1,1,1);


  LUX_PROFILING_OP (g_Profiling.global.nodes.l3ds++);

  return node;
}


// LIST3D UPLOADS
// --------------

void List3Node_initDrawNode(List3DNode_t *base,DrawNode_t *lnode, int layer)
{
  lnode->draw.matRID = -1;
  lnode->layerID = layer%LIST3D_LAYERS;
  lnode->sortkey.pType = (DrawNodeType_t*)lnode;
  lnode->env = &base->enviro;
  lnode->matrix = base->finalMatrix;
  lxVector4Set(lnode->draw.color,1,1,1,1);
#ifdef LUX_DEVBUILD
  lnode->l3d = base;
#endif
}

List3DNode_t* List3DNode_newPrimitive(const char *name, int layer, List3DPrimitiveType_t type,const float *size)
{
  lxBoundingBoxCenter_t bctr;
  List3DNode_t *base;
  List3DPrimitive_t *primitive;
  DrawNode_t *lnode;

  base = List3DNode_new(name,layer/LIST3D_LAYERS);
  if (base == NULL){
    return NULL;
  }

  base->nodeType = LUXI_CLASS_L3D_PRIMITIVE;
  base->primitive = primitive = lxMemGenZalloc(sizeof(List3DPrimitive_t));
  primitive->type = type;
  primitive->size[0] = size[0];
  primitive->size[1] = size[1];
  primitive->size[2] = size[2];
  bctr.size[0]=size[0];
  bctr.size[1]=size[1];
  bctr.size[2]=size[2];
  lxVector3Clear(bctr.center);

  base->numDrawNodes = 1;
  lnode = base->drawNodes = lxMemGenZalloc(sizeof(DrawNode_t));
  lxVector4Set(lnode->draw.color,1,1,1,1);
  List3Node_initDrawNode(base,lnode,layer);
  lnode->sortID =  0;
  lnode->type = DRAW_NODE_OBJECT;
  switch (type) {
    default:
    case L3DPRIMITIVE_CUBE:
      lnode->draw.mesh = g_VID.drw_meshbox;
    break;
    case L3DPRIMITIVE_CYLINDER:
      lnode->draw.mesh = g_VID.drw_meshcylinder;
      bctr.size[0] *= 2;
      bctr.size[1] *= 2;
    break;
    case L3DPRIMITIVE_SPHERE:
      lnode->draw.mesh = g_VID.drw_meshsphere;
      bctr.size[0] *= 2;
      bctr.size[1] *= 2;
      bctr.size[2] *= 2;
    break;
    case L3DPRIMITIVE_QUAD:
      lnode->draw.mesh = g_VID.drw_meshquadffx;
    break;
    case L3DPRIMITIVE_QUAD_CENTERED:
      lnode->draw.mesh = g_VID.drw_meshquadcenteredffx;
    break;
  }
  lxBoundingBox_fromCenterBox(&primitive->bbox,&bctr);

  primitive->orgmesh = lnode->draw.mesh;

  lnode->renderscale = base->primitive->size;
  lnode->draw.state.renderflag |= RENDER_NOVERTEXCOLOR;
  lnode->draw.state.renderflag |= RENDER_NORMALIZE;
  // set up funcs
  base->updateType = LIST3D_UPD_UNLINKED;
  base->specUpdType = LIST3D_SPC_NONE;
  base->processType = LIST3D_PROC_SIMPLE;

  base->reference = Reference_new(LUXI_CLASS_L3D_PRIMITIVE,base);

  return base;
}

List3DNode_t* List3DNode_newParticleEmitterRes(const char *name, int layer, int resRID,const int sortid)
{
  List3DNode_t *base;
  List3DSet_t *l3dset;
  ParticleSys_t  *psys;
  int   lset;

  psys = ResData_getParticleSys(resRID);
  LUX_ASSERT(psys);

  lset = layer/LIST3D_LAYERS;
  l3dset = &g_List3D.drawSets[lset];

  if (!List3DNode_activatePsys(resRID, l3dset))
    return NULL;

  base = List3DNode_new(name,lset);
  if (base == NULL || !psys ){
    return NULL;
  }

  base->numDrawNodes = 0;

  base->nodeType = LUXI_CLASS_L3D_PEMITTER;
  base->pemitter = ParticleEmitter_new(psys);

  if (psys->psysflag & PARTICLE_NOVISTEST)
    base->novistest = LUX_TRUE;

  // set up funcs
  base->updateType = LIST3D_UPD_UNLINKED;
  base->specUpdType = LIST3D_SPC_NONE;
  base->processType = LIST3D_PROC_PARTICLE_EMIT;

  base->reference = Reference_new(LUXI_CLASS_L3D_PEMITTER,base);

  return base;
}

List3DNode_t* List3DNode_newParticleGroupRes(const char *name, int layer, int resRID,const int sortid)
{
  List3DNode_t *base;
  List3DSet_t *l3dset;
  ParticleCloud_t  *pcloud;
  int   lset;

  pcloud = ResData_getParticleCloud(resRID);
  lset = layer/LIST3D_LAYERS;
  l3dset = &g_List3D.drawSets[lset];

  LUX_ASSERT(pcloud);

  if (!List3DNode_activatePCloud(resRID, l3dset))
    return NULL;

  base = List3DNode_new(name,lset);
  if (base == NULL || !pcloud){
    return NULL;
  }

  base->numDrawNodes = 0;

  //base->novistest = TRUE;
  base->nodeType = LUXI_CLASS_L3D_PGROUP;
  base->pgroup = ParticleGroup_new(pcloud);

  base->novistest = LUX_TRUE;

  // set up funcs
  base->updateType = LIST3D_UPD_UNLINKED;
  base->specUpdType = LIST3D_SPC_NONE;
  base->processType = LIST3D_PROC_PARTICLE_GRP;

  base->reference = Reference_new(LUXI_CLASS_L3D_PGROUP,base);

  return base;
}

List3DNode_t* List3DNode_newModelRes(const char *name, int layer, int resRID, booln boneupd, booln modelupd)
{
  return  List3DNode_newModelnstance(name,layer,Modelnstance_new(resRID,boneupd,modelupd));
}

List3DNode_t* List3DNode_newModelnstance(const char *name, int layer, Modelnstance_t* minst)
{
  Model_t *model;
  MeshObject_t *meshobj;
  List3DNode_t *base;
  DrawNode_t *lnode;
  DrawNodeType_t drawtype;
  Shader_t *shader = NULL;

  int  texRID;
  int i;

  if (!minst)
    return NULL;

  model = ResData_getModel(minst->modelRID);
  LUX_ASSERT(model);

  base = List3DNode_new(name,layer/LIST3D_LAYERS);
  if (base == NULL){
    return NULL;
  }
  base->minst = minst;
  base->numDrawNodes = model->numMeshObjects;
  lnode = base->drawNodes = lxMemGenZalloc(sizeof(DrawNode_t)*model->numMeshObjects);

  base->nodeType = LUXI_CLASS_L3D_MODEL;

  drawtype = DRAW_NODE_OBJECT;

  for (i = 0; i < base->numDrawNodes; i++,lnode++) {
    meshobj = &model->meshObjects[i];
    texRID = meshobj->texRID;
    List3Node_initDrawNode(base,lnode,layer);

    if (vidMaterial(texRID)){
      lnode->draw.state.renderflag = Material_getDefaults(ResData_getMaterial(texRID),&shader,&lnode->draw.state.alpha,&lnode->draw.state.blend,&lnode->draw.state.line);
    }

    if (meshobj->texname && meshobj->texname[0] == '!')
      lnode->draw.state.renderflag |= VIDRenderFlag_fromString(meshobj->texname);

    lnode->draw.mesh = meshobj->mesh;
    lnode->draw.matRID = texRID;

    if (shader && shader->layer != 0){
      lnode->layerID = shader->layer-1;
    }
    else
      lnode->layerID = layer % LIST3D_LAYERS;

    lnode->type = DRAW_NODE_OBJECT;

    if (meshobj->skinID >= 0){
      lnode->skinobj = &minst->skinobjs[meshobj->skinID];
      lnode->type = DRAW_NODE_SKINNED_OBJECT;
    }

    DrawNode_updateSortID(lnode,DRAW_SORT_TYPE_NORMAL);

    if (meshobj->bone){
      lnode->bonematrix = (minst->boneupd) ? &minst->boneupd->bonesAbs[meshobj->bone->ID] : &model->bonesys.refAbsMatrices[meshobj->bone->ID];
    }

  }

  // set up funcs
  base->updateType = LIST3D_UPD_UNLINKED;
  if (minst->boneupd || minst->modelupd)
    base->specUpdType = LIST3D_SPC_MODEL_ANIM;
  else
    base->specUpdType = LIST3D_SPC_NONE;
  base->processType = LIST3D_PROC_SIMPLE;

  base->checkptr = model;
  base->checkType = RESOURCE_MODEL;
  base->checkRID = model->resinfo.resRID;

  base->reference = Reference_new(LUXI_CLASS_L3D_MODEL,base);

  return base;
}


List3DNode_t* List3DNode_newLight(const char *name, int layer)
{
  List3DNode_t *base;

  base = List3DNode_new(name,layer/LIST3D_LAYERS);
  if (base == NULL){
    return NULL;
  }
  base->novistest = LUX_TRUE;
  base->numDrawNodes = 0;
  base->nodeType = LUXI_CLASS_L3D_LIGHT;
  base->light = Light_new();
  LUX_ASSERT(base->light);
  base->light->l3dnode = base;


  // set up funcs
  base->updateType = LIST3D_UPD_UNLINKED;
  base->specUpdType = LIST3D_SPC_NONE;
  base->processType = LIST3D_PROC_NONE;

  base->reference = Reference_new(LUXI_CLASS_L3D_LIGHT,base);

  return base;
}

List3DNode_t* List3DNode_newProjector(const char *name, int layer)
{
  List3DNode_t *base;

  base = List3DNode_new(name,layer/LIST3D_LAYERS);
  if (base == NULL){
    return NULL;
  }
  base->novistest = LUX_TRUE;
  base->numDrawNodes = 0;
  base->nodeType = LUXI_CLASS_L3D_PROJECTOR;
  base->proj = Projector_new();
  LUX_ASSERT(base->proj);
  base->proj->l3dnode = base;


  // set up funcs
  base->updateType = LIST3D_UPD_UNLINKED;
  base->specUpdType = LIST3D_SPC_NONE;
  base->processType = LIST3D_PROC_NONE;

  base->reference = Reference_new(LUXI_CLASS_L3D_PROJECTOR,base);

  return base;
}

List3DNode_t* List3DNode_newTrail(const char *name, int layer, ushort length)
{
  List3DNode_t *base;
  DrawNode_t *lnode;

  base = List3DNode_new(name,layer/LIST3D_LAYERS);
  if (base == NULL){
    return NULL;
  }
  base->reference = Reference_new(LUXI_CLASS_L3D_TRAIL,base);

  base->novistest = LUX_TRUE;
  base->numDrawNodes = 1;

  base->nodeType = LUXI_CLASS_L3D_TRAIL;
  base->trail = Trail_new(length);
  LUX_ASSERT(base->trail);

  lnode = base->drawNodes = lxMemGenZalloc(sizeof(DrawNode_t));
  List3Node_initDrawNode(base,lnode,layer);
  lnode->type = DRAW_NODE_WORLD;
  lnode->draw.mesh = &base->trail->mesh;
  DrawNode_updateSortID(lnode,DRAW_SORT_TYPE_TRAIL);


  // set up funcs
  base->updateType = LIST3D_UPD_UNLINKED;
  base->specUpdType = LIST3D_SPC_NONE;
  base->processType = LIST3D_PROC_ALWAYS;



  return base;
}

List3DNode_t* List3DNode_newText(const char *name, int layer, const char *text)
{
  List3DNode_t *base;
  DrawNode_t *lnode;

  base = List3DNode_new(name,layer/LIST3D_LAYERS);
  if (base == NULL){
    return NULL;
  }
  base->novistest = LUX_TRUE;
  base->numDrawNodes = 1;

  base->nodeType = LUXI_CLASS_L3D_TEXT;
  base->ptext = PText_new(text);
  LUX_ASSERT(base->trail);

  lnode = base->drawNodes = lxMemGenZalloc(sizeof(DrawNode_t));
  List3Node_initDrawNode(base,lnode,layer);
  lnode->type = DRAW_NODE_TEXT_OBJECT;
  lnode->draw.ptext = base->ptext;
  lnode->draw.state.blend.blendmode = VID_DECAL;
  lnode->draw.state.renderflag = PText_getDefaultRenderFlag() | RENDER_NORMALIZE;
  lnode->draw.matRID = lnode->draw.ptext->texRID;
  DrawNode_updateSortID(lnode,DRAW_SORT_TYPE_NORMAL);


  // set up funcs
  base->updateType = LIST3D_UPD_UNLINKED;
  base->specUpdType = LIST3D_SPC_NONE;
  base->processType = LIST3D_PROC_SIMPLE;

  base->reference = Reference_new(LUXI_CLASS_L3D_TEXT,base);

  return base;
}

List3DNode_t *List3DNode_newShadowModel(const char *name, int layer,const List3DNode_t *target,const List3DNode_t *light, const char *meshnameflag)
{
  List3DNode_t *base;
  DrawNode_t *lnode;
  ShadowModel_t *smdl;
  ShadowMesh_t  *smesh;
  Model_t *mdl;
  int i;

  smdl = ShadowModel_new(target,light,meshnameflag);
  if ( smdl == NULL){
    return NULL;
  }

  base = List3DNode_new(name,layer/LIST3D_LAYERS);
  if (base == NULL){
    return NULL;
  }

  base->smdl = smdl;
  base->novistest = LUX_TRUE;
  base->numDrawNodes = smdl->numShadowmeshes;
  base->nodeType = LUXI_CLASS_L3D_SHADOWMODEL;

  if (target->nodeType == LUXI_CLASS_L3D_MODEL)
    mdl = ResData_getModel(target->minst->modelRID);
  else
    mdl = NULL;

  lnode = base->drawNodes = lxMemGenZalloc(sizeof(DrawNode_t)*base->numDrawNodes);
  smesh = smdl->shadowmeshes;
  for (i = 0; i < base->numDrawNodes; i++,lnode++,smesh++){
    List3Node_initDrawNode(base,lnode,layer);
    lnode->draw.mesh = smesh->mesh;
    if (mdl){
      lnode->bonematrix = target->drawNodes[smesh->refindex].bonematrix;
      lnode->skinobj =  target->drawNodes[smesh->refindex].skinobj;
      lnode->renderscale = target->drawNodes[smesh->refindex].renderscale;
    }
    else
      lnode->renderscale = target->drawNodes->renderscale;
    lnode->type = DRAW_NODE_SHADOW_OBJECT;
    lnode->draw.state.renderflag = 0;
    lnode->draw.matRID = 0;
    lnode->sortkey.value = BIT_ID_FULL32;

  }



  // set up funcs
  base->updateType = LIST3D_UPD_UNLINKED;
  base->specUpdType = LIST3D_SPC_NONE;
  base->processType = LIST3D_PROC_SIMPLE;

  base->reference = Reference_new(LUXI_CLASS_L3D_SHADOWMODEL,base);

  return base;
}

List3DNode_t* List3DNode_newLevelModel(const char *name, int layer, int resRID, int dimension[3], int numpolys,float mergemargin)
{
  List3DNode_t *base;
  DrawNode_t *lnode;
  LevelModel_t  *lmdl;
  LevelSubMesh_t  *smesh;
  Shader_t *shader;
  Model_t *mdl;
  int i;

  mdl = ResData_getModel(resRID);

  base = List3DNode_new(name,layer/LIST3D_LAYERS);
  if (base == NULL){
    return NULL;
  }
  lmdl = LevelModel_new(resRID,dimension,numpolys,mergemargin);
  if ( lmdl == NULL)
    return NULL;

  base->lmdl = lmdl;
  base->numDrawNodes = lmdl->numSubmeshes;
  base->nodeType = LUXI_CLASS_L3D_LEVELMODEL;

  lnode = base->drawNodes = lxMemGenZalloc(sizeof(DrawNode_t)*base->numDrawNodes);
  smesh = lmdl->submeshes;
  for (i = 0; i < base->numDrawNodes; i++,lnode++,smesh++){
    int texRID =  mdl->meshObjects[smesh->origMeshID].texRID;
    List3Node_initDrawNode(base,lnode,layer);
    shader = NULL;
    if (vidMaterial(texRID)){
      lnode->draw.state.renderflag = Material_getDefaults(ResData_getMaterial(texRID),&shader,&lnode->draw.state.alpha,&lnode->draw.state.blend,&lnode->draw.state.line);
    }
    else
      lnode->draw.state.renderflag = 0;

    lnode->draw.mesh = &smesh->mesh;
    lnode->draw.matRID = texRID;

    if (shader && shader->layer != 0){
      lnode->layerID = shader->layer-1;
    }
    else
      lnode->layerID = layer % LIST3D_LAYERS;

    lnode->draw.mesh = &smesh->mesh;
    lnode->type = DRAW_NODE_OBJECT;
    lnode->env = &smesh->subnode->drawenv;

    DrawNode_updateSortID(lnode,DRAW_SORT_TYPE_NORMAL);
  }

  // set up funcs
  base->updateType = LIST3D_UPD_UNLINKED;
  base->specUpdType = LIST3D_SPC_NONE;
  base->processType = LIST3D_PROC_LEVELMODEL;

  LevelModel_updateVis(base->lmdl,base);

  base->checkRID = resRID;
  base->checkType = RESOURCE_MODEL;
  base->checkptr = mdl;

  base->reference = Reference_new(LUXI_CLASS_L3D_LEVELMODEL,base);

  return base;
}

List3DNode_t* List3DNode_newCamera(const char *name, int layer, int bitid)
{
  List3DNode_t *base;

  base = List3DNode_new(name,layer/LIST3D_LAYERS);
  if (base == NULL){
    return NULL;
  }
  base->novistest = LUX_TRUE;
  base->numDrawNodes = 0;
  base->nodeType = LUXI_CLASS_L3D_CAMERA;
  base->cam = Camera_new(bitid);
  LUX_ASSERT(base->cam);
  base->cam->l3dnode = base;


  // set up funcs
  base->updateType = LIST3D_UPD_UNLINKED;
  base->specUpdType = LIST3D_SPC_NONE;
  base->processType = LIST3D_PROC_NONE;

  base->reference = Reference_new(LUXI_CLASS_L3D_CAMERA,base);

  return base;
}

// LIST3D LINK
// -------------

int List3DNode_link(List3DNode_t *node, List3DNode_t *newparent){
  int becamechild = 0;
  booln hadparent = LUX_FALSE;

  // illegal to mix setIDs
  if (node->setID != newparent->setID)
    return LUX_TRUE;


  if (!node->parent){
    Reference_ref(node->reference);
  }

  if (node->parent){
    lxListNode_remove(node->parent->childrenListHead,node);
    hadparent = LUX_TRUE;
    node->parent = NULL;
  }
  else if (node->unlinked){
    becamechild = 1;
  }
  else{
    lxListNode_remove(g_List3D.drawSets[node->setID].nodeListHead,node);
    becamechild = 1;
  }


  lxListNode_addLast(newparent->childrenListHead,node);


  node->parent = newparent;
  node->update = LUX_TRUE;
  node->unlinked = LUX_FALSE;
  node->updateType = LIST3D_UPD;
  // must not change children linktype
  List3DNode_setIF(node,newparent->linkReference,newparent->linkType < 0 ? -2 : newparent->linkType,becamechild);

  return LUX_FALSE;
}


void List3DNode_unlink(List3DNode_t *node){
  lxRl3dnode pref = NULL;
  int becameorphan = 0;

  if (node->unlinked)
    return;

  if (node->parent){
    pref = node->parent->reference;
    lxListNode_remove(node->parent->childrenListHead,node);

    node->parent = NULL;
    becameorphan = -1;
  }
  else{
    lxListNode_remove(g_List3D.drawSets[node->setID].nodeListHead,node);
  }

  List3DNode_setIF(node,NULL,-1,becameorphan);
  node->unlinked = LUX_TRUE;

  if (pref){
    Reference_releaseCheck(node->reference);
  }
}

int List3DNode_linkParentBone(List3DNode_t *node, char *bonename){
  Bone_t *bone;
  if (node->parent && node->parent->nodeType == LUXI_CLASS_L3D_MODEL && node->updateType < LIST3D_UPD_MANUAL_WORLD){
    Model_t *model = ResData_getModel(node->parent->minst->modelRID);
    bone = BoneSys_getBone(&model->bonesys,bonename);

    if (!bone)
      return LUX_FALSE;

    if (!node->boneMatrix)
      node->updateType += LIST3D_UPD_SHIFT_BONE;

    if (node->parent->minst->boneupd)
      node->boneMatrix = &node->parent->minst->boneupd->bonesAbs[bone->ID];
    else
      node->boneMatrix = &model->bonesys.refAbsMatrices[bone->ID];

    LUX_SIMDASSERT((size_t)node->boneMatrix % 16 == 0);

    return LUX_TRUE;
  }
  else
    return LUX_FALSE;

}

void List3DNode_unlinkParentBone(List3DNode_t *node){
  if (node->boneMatrix)
    node->updateType -= LIST3D_UPD_SHIFT_BONE;
  node->boneMatrix = NULL;
}


void List3DNode_setVisflag(List3DNode_t *base,enum32 flag)
{
  if (flag == base->visflag)
    return;

  base->visflag = flag;
  UPDATE_VISOBJECT(base);
  if (!base->novistest && base->linkVisobject)
    base->linkVisobject->cameraFlag |= base->visflag;
}

// needs updated visobject
// childstate = 0 nochange 1 becamechild -1 becameorphan
static void List3DNode_updateVisobject(List3DNode_t *base, VisObject_t *oldvisobj, int childstate)
{
  int i;
  DrawNode_t *dnode;
  lxBoundingBox_t bbox;

  // no change
  if (oldvisobj && oldvisobj == base->linkVisobject) return;

  // unlink from old
  if (oldvisobj){
    for (i = 0; i < base->numDrawNodes; i++){
      if (base->drawNodes[i].draw.state.renderflag & RENDER_FXLIT)
        VisObject_setVissetLight(oldvisobj,LUX_FALSE,base->drawNodes[i].draw.state.renderflag & RENDER_SUNLIT);
    }

    // in case we already were a child we were never linked directly
    // base->parent might became NULL via unlink, in that case we just became an orphan, but werent linked directly before
    if ((childstate>0) || (!base->parent && childstate >= 0)){
      lxLN_remove(oldvisobj->l3dnodesListHead,base,linknext,linkprev);
      if(!base->novistest)
        lxLN_remove(oldvisobj->l3dvisListHead,base,visnext,visprev);
    }
    // allow oldvisobj to link to any l3dset
    if (!oldvisobj->l3dnodesListHead && !oldvisobj->l3dvisListHead)
      oldvisobj->curvisset = -1;
  }
  if (base->novistest && base->visnext){
    lxLN_remove(g_List3D.drawSets[base->setID].novistestListHead,base,visnext,visprev);
    base->visnext = NULL;
  }

  // link to new
  if (base->linkVisobject){
    // update visobject
    dnode = base->drawNodes;
    for (i = 0; i < base->numDrawNodes; i++,dnode++){
      if (dnode->draw.state.renderflag & RENDER_FXLIT)
        VisObject_setVissetLight(base->linkVisobject,LUX_TRUE,dnode->draw.state.renderflag & RENDER_SUNLIT);
    }

    dnode = base->drawNodes;
    if (base->enviro.needslight && !(dnode->draw.state.renderflag & RENDER_SUNLIT)){
      base->vislightres = &base->linkVisobject->visset.vislight->nonsun;
    }
    else{
      base->vislightres  = &base->linkVisobject->visset.vislight->full;
    }

    base->linkVisobject->curvisset = base->setID;

    switch(base->nodeType) {
    case LUXI_CLASS_L3D_MODEL:
      VisObject_setBBox(base->linkVisobject,&ResData_getModel(base->minst->modelRID)->bbox);
      base->linkVisobject->cameraFlag |= base->visflag;
      base->linkVisobject->visset.projectorFlag |= base->enviro.projectorflag;
      break;
    case LUXI_CLASS_L3D_PEMITTER:
      VisObject_setBBox(base->linkVisobject,&ResData_getParticleSys(base->pemitter->psysRID)->bbox);
      base->linkVisobject->cameraFlag |= base->visflag;
      break;
    case LUXI_CLASS_L3D_PRIMITIVE:
      VisObject_setBBox(base->linkVisobject,&base->primitive->bbox);
      base->linkVisobject->cameraFlag |= base->visflag;
      break;
    case LUXI_CLASS_L3D_LEVELMODEL:
      VisObject_setBBox(base->linkVisobject,&ResData_getModel(base->lmdl->modelRID)->bbox);
      base->linkVisobject->cameraFlag |= base->visflag;
      break;
    case LUXI_CLASS_L3D_TEXT:
      PText_getDimensions(base->ptext,NULL,bbox.max);
      lxVector3Set(bbox.min,0.0f,0.0f,0.0f);
      VisObject_setBBox(base->linkVisobject,&bbox);
      base->linkVisobject->cameraFlag |= base->visflag;
      break;
    default:
      break;
    }


    // link to visobject
    if (!base->parent){
      if (!base->linkVisobject->l3dnodesListHead)
        lxLN_init(base,linknext,linkprev);
      lxLN_addLast(base->linkVisobject->l3dnodesListHead,base,linknext,linkprev);

      if (!base->novistest){
        if (!base->linkVisobject->l3dvisListHead)
          lxLN_init(base,visnext,visprev);
        lxLN_addLast(base->linkVisobject->l3dvisListHead,base,visnext,visprev);
      }
    }
    if (base->novistest){
      if (!g_List3D.drawSets[base->setID].novistestListHead)
        lxLN_init(base,visnext,visprev);
      lxLN_addLast(g_List3D.drawSets[base->setID].novistestListHead,base,visnext,visprev);
    }

  }

}


int List3DNode_useNovistest(List3DNode_t *base, int state)
{

  if (state < 0) return base->novistest;

  if (base->novistest == state)
    return state;

  // FIXME
  // unlinked nodes still can be in the novistest list ?

  // we had no vistest, gotta remove from the list and add to
  // visobject list
  //UPDATE_VISOBJECT(base);
  if (base->novistest){
    // remove
    lxLN_remove(g_List3D.drawSets[base->setID].novistestListHead,base,visnext,visprev);

    // add
    if (!base->parent){
    if (!base->linkVisobject->l3dvisListHead)
        lxLN_init(base,visnext,visprev);
    lxLN_addLast(base->linkVisobject->l3dvisListHead,base,visnext,visprev);
    }
  }
  else{
    // we have to remove from visobject
    if (base->linkVisobject && !base->parent){
      // remvoe from visobj
      lxLN_remove(base->linkVisobject->l3dvisListHead,base,visnext,visprev);
    }


    // add to novistest list
    if (!g_List3D.drawSets[base->setID].novistestListHead)
      lxLN_init(base,visnext,visprev);
    lxLN_addLast(g_List3D.drawSets[base->setID].novistestListHead,base,visnext,visprev);
  }

  base->novistest = state;
  return state;
}

int List3DNode_useLocal(List3DNode_t *node, int state)
{
  if (state < 0){
    if (node->lookat) return node->lookatlocal;
    if (node->updateType){
      List3DUpdateType_t upd = node->updateType;
      upd--;
      upd %= LIST3D_UPD_SHIFT_PARENT;
      if (upd >= LIST3D_UPD_SHIFT_LOCAL)
        return LUX_TRUE;
      else
        return LUX_FALSE;
    }
    else
      return LUX_FALSE;
  }
  else if (node->updateType != LIST3D_UPD_UNLINKED){
    if (node->lookat) return (node->lookatlocal=state);
    else{
      int oldstate = List3DNode_useLocal(node,-1);
      if (state == oldstate)
        return state;

      node->update = LUX_TRUE;
      if (state)
        node->updateType += LIST3D_UPD_SHIFT_LOCAL;
      else
        node->updateType -= LIST3D_UPD_SHIFT_LOCAL;

      return state;
    }
  }
  return LUX_FALSE;
}

int List3DNode_useLookAt(List3DNode_t *self,int state){
  if (state < 0) return self->lookat;
  else if (state != self->lookat){
    if (state){
      self->lookatlocal = List3DNode_useLocal(self,-1);
      List3DNode_useLocal(self,LUX_FALSE);
    }
    else{
      List3DNode_useLocal(self,self->lookatlocal);
    }
    self->lookat = state;
  }
  return self->lookat;
}

int  List3DNode_useManualWorld(List3DNode_t *self, int state)
{
  if (state < 0) return (self->updateType >= LIST3D_UPD_MANUAL_WORLD);

  if (state == self->updateType>= LIST3D_UPD_MANUAL_WORLD  || self->updateType == LIST3D_UPD_UNLINKED)
    return state;

  if (state){
    if (self->updateType >= LIST3D_UPD_PARENT)
      self->updateType = LIST3D_UPD_PARENT;
    else
      self->updateType = LIST3D_UPD;

    self->inheritLocks = 0;
    self->boneMatrix = NULL;
    self->updateType = (self->updateType == LIST3D_UPD) ? LIST3D_UPD_MANUAL_WORLD : LIST3D_UPD_PARENT_MANUAL_WORLD;
  }
  else{
    self->updateType = (self->updateType == LIST3D_UPD_MANUAL_WORLD) ? LIST3D_UPD : LIST3D_UPD_PARENT;
  }
  return state;
}

// childstate = 0 no change 1 becamechild -1 becameorphan
int  List3DNode_setIF(List3DNode_t *base, Reference linkReference, int linkType, int childstate){
  List3DNode_t *browse;
  VisObject_t *oldvisobj;
  LinkObject_t *link;
  VisObject_t *newvisobj;

  if (linkType > 0 && Reference_get(linkReference,link) &&
    (newvisobj = link->visobject) &&
    newvisobj->curvisset >= 0 && newvisobj->curvisset != base->setID)
    return LUX_TRUE;


  // tidy up old
  UPDATE_VISOBJECT(base);
  oldvisobj = base->linkVisobject;

  // always unref old
  Reference_reset(base->linkReference);

  /*
  if (base->parent && base->parent->setID != base->setID){
    g_List3D.drawSets[base->setID].numNodes--;
    base->setID = base->parent->setID;
    g_List3D.drawSets[base->setID].numNodes++;
  }
  */

  // we must also tell children the new visobject nodes
  lxListNode_forEach(base->childrenListHead,browse)
    List3DNode_setIF(browse,linkReference,linkType < 0 ? linkType-1 : linkType,0);
  lxListNode_forEachEnd(base->childrenListHead,browse);

  if (linkType == LINK_UNSET){
    // complete unlink for base
    base->updateType = LIST3D_UPD_UNLINKED;
  }

  if (linkType < 0){
    // keep internal updatetype for childrens
    base->linkReference = NULL;
    base->linkVisobject = NULL;

    // but always unlink from old visobj
    List3DNode_updateVisobject(base,oldvisobj,childstate);
    return LUX_FALSE;
  }

  // link to new and update
  base->linkType = linkType;
  base->linkReference = linkReference;

  UPDATE_VISOBJECT(base);
  LUX_ASSERT(base->linkVisobject);

  List3DNode_updateVisobject(base,oldvisobj,childstate);

  if (base->updateType >= LIST3D_UPD_MANUAL_WORLD)
    base->updateType = (base->updateType==LIST3D_UPD_MANUAL_WORLD)  ? LIST3D_UPD : LIST3D_UPD_PARENT;

  // handle position updates
  if (base->parent){
    if (base->updateType && base->updateType < LIST3D_UPD_PARENT )
      base->updateType += LIST3D_UPD_SHIFT_PARENT;
    List3DNode_useLocal(base,LUX_TRUE);
  }
  else{
    if (!base->updateType)
      base->updateType = LIST3D_UPD;
    else if (base->updateType >= LIST3D_UPD_PARENT){
      base->updateType--;
      base->updateType%=LIST3D_UPD_SHIFT_PARENT;
      base->updateType++;
    }
  }

  // make active
  if (base->unlinked && !base->parent){
    lxListNode_addLast(g_List3D.drawSets[base->setID].nodeListHead,base);
    base->unlinked = LUX_FALSE;
  }
  base->update = LUX_TRUE;

  return LUX_FALSE;
}

void List3DNode_setInheritLocks(List3DNode_t *base, int inh){
  if (inh == base->inheritLocks || base->updateType >= LIST3D_UPD_MANUAL_WORLD)
    return;

  if (inh && !base->inheritLocks)
    base->updateType += LIST3D_UPD_SHIFT_INH;
  else if (!inh && base->inheritLocks)
    base->updateType -= LIST3D_UPD_SHIFT_INH;

  base->inheritLocks = inh;
  base->update = LUX_TRUE;
}

void List3DNode_setRF(List3DNode_t *base, enum32 renderflag, int state, int dnodeID){

  int i;
  DrawNode_t *dnode = (dnodeID < 0) ? base->drawNodes : &base->drawNodes[dnodeID];
  int end = (dnodeID < 0) ? base->numDrawNodes : 1;

  //UPDATE_VISOBJECT(base);

  for (i = 0; i < end; i++,dnode++){
    enum32 rf = renderflag;
    if (dnode->draw.state.renderflag & RENDER_UNLIT){
      rf &= ~RENDER_SUNLIT;
      rf &= ~RENDER_FXLIT;
    }
    if (rf & RENDER_FXLIT){
      if (state && !(dnode->draw.state.renderflag & RENDER_FXLIT)){
        if (base->linkVisobject){
          VisObject_setVissetLight(base->linkVisobject,LUX_TRUE,dnode->draw.state.renderflag & RENDER_SUNLIT);
          // update vislightptr
          if (!(dnode->draw.state.renderflag & RENDER_SUNLIT)){
            base->vislightres = &base->linkVisobject->visset.vislight->nonsun;
          }
          else{
            base->vislightres  = &base->linkVisobject->visset.vislight->full;
          }
        }
        if (base->processType >= LIST3D_PROC_SIMPLE && !base->enviro.needslight)
          base->processType += LIST3D_PROC_SHIFT_LIGHTS;
        base->enviro.needslight++;
      }
      else if (!state && (dnode->draw.state.renderflag & RENDER_FXLIT)){
        if (base->linkVisobject)
          VisObject_setVissetLight(base->linkVisobject,LUX_FALSE,dnode->draw.state.renderflag & RENDER_SUNLIT);
        base->enviro.needslight--;
        if (base->processType >= LIST3D_PROC_START_SIMPLE && !base->enviro.needslight){
          base->processType -= LIST3D_PROC_SHIFT_LIGHTS;
          base->enviro.numFxLights = 0;
        }

      }
    }
    if (rf & RENDER_SUNLIT && dnode->draw.state.renderflag & RENDER_FXLIT){
      // nonsun needs to be raised or lowered and full the opposite
      // we perform raises first to prevent a unneeded free/alloc
      if (state && !(dnode->draw.state.renderflag & RENDER_SUNLIT)){
        if (base->linkVisobject){
          // we were nonsun, and are full now
          VisObject_setVissetLight(base->linkVisobject,LUX_TRUE,LUX_TRUE);
          VisObject_setVissetLight(base->linkVisobject,LUX_FALSE,LUX_FALSE);
          base->vislightres  = &base->linkVisobject->visset.vislight->full;

        }
      }
      else if (!state && (dnode->draw.state.renderflag & RENDER_SUNLIT)){
        if (base->linkVisobject){
          // we were full and are nonsun now
          VisObject_setVissetLight(base->linkVisobject,LUX_TRUE,LUX_FALSE);
          VisObject_setVissetLight(base->linkVisobject,LUX_FALSE,LUX_TRUE);
          base->vislightres = &base->linkVisobject->visset.vislight->nonsun;
        }
      }
    }

    if (state)
      dnode->draw.state.renderflag |= rf;
    else
      dnode->draw.state.renderflag &= ~rf;
  }


}

int  List3DNode_setLayer(List3DNode_t *base, int layer, int mid){
  DrawNode_t *dnode;
  int i;
  int setID = layer/ LIST3D_LAYERS;
  int start = (mid < 0) ? 0 : mid;
  int end = (mid < 0) ? base->numDrawNodes : mid+1;

  //UPDATE_VISOBJECT(base);

  if (setID != base->setID && !base->unlinked)
    return LUX_TRUE;

  for (i = start; i < end; i++){
    dnode = &base->drawNodes[i];
    dnode->layerID = layer%LIST3D_LAYERS;
  }

  // allow when unlinked
  if (setID != base->setID){
    /*
    if (!base->parent && !base->unlinked){
      // remove from old
      lxListNode_remove(g_List3D.drawSets[base->setID].nodeListHead,base);
      // add to new
      lxListNode_pushBack(g_List3D.drawSets[setID].nodeListHead,base);
    }
    */
    if (base->novistest){
      // remove from old
      lxLN_remove(g_List3D.drawSets[base->setID].novistestListHead,base,visnext,visprev);
      // add to new
      if (!g_List3D.drawSets[setID].novistestListHead)
        lxLN_init(base,visnext,visprev);
      lxLN_addLast(g_List3D.drawSets[setID].novistestListHead,base,visnext,visprev);
    }


    g_List3D.drawSets[base->setID].numNodes--;
    g_List3D.drawSets[setID].numNodes++;

    /*
    if (base->linkVisobject){
      for (i=0; i < base->numDrawNodes; i++){
        dnode = &base->drawNodes[i];
        if (dnode->renderflag & RENDER_FXLIT){
          VisObject_setVissetLight(base->linkVisobject,MAINID(base),FALSE,dnode->renderflag & RENDER_SUNLIT);
          VisObject_setVissetLight(base->linkVisobject,setID,TRUE,dnode->renderflag & RENDER_SUNLIT);
        }
      }
      if (base->enviro.needslight && !(dnode->renderflag & RENDER_SUNLIT)){
        base->vislightres = &base->linkVisobject->vissets[MAINID(base)].vislight->nonsun;
      }
      else{
        base->vislightres  = &base->linkVisobject->vissets[MAINID(base)].vislight->full;
      }
    }
    */
    base->setID = setID;
  }
  return LUX_FALSE;
}

// MODEL specific

int List3DNode_Model_meshMaterial(List3DNode_t *base,const int mesh, const int resRID){


  if (base){
    DrawNode_t *lnode;

    if (mesh < 0 || mesh > base->numDrawNodes)
      return -1;

    lnode = &base->drawNodes[mesh];

    if (resRID == -2)
      return lnode->draw.matRID;

    lnode->draw.matRID = resRID;
    if (lnode->draw.matobj)
      MaterialObject_free(lnode->draw.matobj);
    lnode->draw.matobj = NULL;

    if (vidMaterial(resRID)){
      Shader_t *shader;
      lnode->draw.state.renderflag = Material_getDefaults(ResData_getMaterial(resRID),&shader,&lnode->draw.state.alpha,&lnode->draw.state.blend,&lnode->draw.state.line);
      if (shader->layer != 0){
        lnode->layerID = shader->layer-1;
      }
    }

    DrawNode_updateSortID(lnode,DRAW_SORT_TYPE_NORMAL);

    return resRID;
  }
  return -2;
}

void   List3DNode_setProjectorFlag(List3DNode_t *base, enum32 projectorflag){
  if (base->processType >= LIST3D_PROC_START_SIMPLE){
    if (!projectorflag && base->enviro.projectorflag){
      base->processType -= LIST3D_PROC_SHIFT_PROJ;
    }
    else if (projectorflag && !base->enviro.projectorflag){
      base->processType += LIST3D_PROC_SHIFT_PROJ;
    }
  }
  base->enviro.projectorflag = projectorflag;
  //UPDATE_VISOBJECT(base);

  if (base->linkVisobject)
    base->linkVisobject->visset.projectorFlag |= projectorflag;

}

void  List3DNode_Model_setWorldBones(List3DNode_t *base, const int state)
{
  DrawNode_t *dnode = base->drawNodes;
  int i;

  if (base->minst->boneupd->inWorld == state)
    return;

  // update all drawnodes
  for (i = 0; i < base->numDrawNodes; i++,dnode++){
    if (dnode->bonematrix)
      dnode->type = (state) ? DRAW_NODE_WORLD : DRAW_NODE_OBJECT;
    else if (dnode->skinobj)
      dnode->type = (state) ? DRAW_NODE_SKINNED_WORLD : DRAW_NODE_SKINNED_OBJECT;
  }


  base->minst->boneupd->inWorld = state;
}


//////////////////////////////////////////////////////////////////////////
// List3DView

List3DView_t *List3DView_new(int rcmdbias)
{
  List3DView_t *view = lxMemGenZalloc(sizeof(List3DView_t)+(sizeof(RenderCmd_t*)*rcmdbias));
  view->reference = Reference_new(LUXI_CLASS_L3D_VIEW,view);
  view->camvisflag = 1;

  lxVector4Set(view->background.fogcolor,1,1,1,0);
  view->background.fogmode = 0;
  view->background.fogstart = 0;
  view->background.fogend = 1024;
  view->background.fogdensity = 1;
  view->background.skybox = NULL;
  view->maxCommands = LIST3D_VIEW_DEFAULT_COMMANDS+rcmdbias;
  view->rcmdflag = 1;
  view->layerfill = LUX_TRUE;
  view->viewport.bounds.fullwindow = LUX_TRUE;
  view->viewport.depth[0] = 0.0f;
  view->viewport.depth[1] = 1.0f;

  LUX_ASSERT(view->maxCommands>0);

  return view;
}

  // are filled in before default view
int List3DView_activate(List3DView_t *view, const int setID, List3DView_t *ref, int after)
{
  List3DView_t **list = &g_List3D.drawSets[setID].viewListHead;

  if (view->isdefault)
    return LUX_FALSE;

  List3DView_deactivate(view);
  if (ref){
    List3DView_t *browse;


    lxListNode_forEach((*list),browse)
      if (browse == ref){
        browse = NULL;
        break;
      }
    lxListNode_forEachEnd((*list),browse);

    if (!browse){
      if (after){
        lxListNode_insertNext(ref,view);
      }
      else{
        lxListNode_insertPrev(ref,view);
        if (ref == *list)
          *list = view;
      }
    }
    else
      return LUX_FALSE;
  }
  else
    lxListNode_pushFront((*list),view);

  view->list = &g_List3D.drawSets[setID].viewListHead;

  return LUX_TRUE;
}

void  List3DView_deactivate(List3DView_t *view)
{
  if (view->isdefault)
    return;

  if (view->list){
    lxListNode_remove((*view->list),view);
  }
  view->list = NULL;
}

static void List3DView_free(List3DView_t *view)
{
  if (view->isdefault)
    return;

  List3DView_deactivate(view);

  List3DView_rcmdEmpty(view);

  Reference_invalidate(view->reference);

  lxMemGenFree(view,sizeof(List3DView_t)+((sizeof(RenderCmd_t*)*(view->maxCommands-LIST3D_VIEW_DEFAULT_COMMANDS))));
}

void  List3DView_rcmdEmpty(List3DView_t *lview)
{
  int i;
  for (i = 0; i < lview->numCommands; i++){
    Reference_release(lview->commands[i]->reference);
  }
  lview->numCommands = 0;
  lview->commands[0] = NULL;
}

booln List3DView_rcmdRemove(List3DView_t *lview,RenderCmd_t *rcmd2, int id)
{
  RenderCmd_t** rcmdptr;
  RenderCmd_t** rcmdptr2;
  RenderCmd_t*  rcmd;
  int i;
  booln state;

  // find first occurance
  id = id == 0 ? 1 : id;
  if (id < 0){
    rcmdptr = &lview->commands[lview->numCommands];
    rcmdptr2 = lview->commands-1;
    i = -1;
    id = -id;
  }
  else{
    rcmdptr = lview->commands;
    rcmdptr2 = &lview->commands[lview->numCommands+1];
    i = 1;
  }
  state = LUX_FALSE;
  for (;rcmdptr != rcmdptr2; rcmdptr+=i){
    if (*rcmdptr == rcmd2 && (--id) == 0){
      // shift old
      i = rcmdptr-&lview->commands[0];
      rcmdptr2 = &lview->commands[lview->numCommands-1];

      while (rcmdptr2 != rcmdptr){
        rcmd = *(rcmdptr+1);
        *rcmdptr = rcmd;

        rcmdptr++;
        i++;
      }

      lview->numCommands--;
      lview->commands[lview->numCommands] = NULL;

      Reference_release(rcmd2->reference);
      state = LUX_TRUE;
      break;
    }
  }
  return state;
}

booln List3DView_rcmdAdd(List3DView_t *lview, RenderCmd_t *rcmd, booln before, RenderCmd_t *rcmd2, int id)
{
  RenderCmd_t** rcmdptr;
  RenderCmd_t** rcmdptr2;
  int i;

  booln state;
  if (rcmd2 == NULL && before){
    rcmd2 = lview->commands[0];
    id = 1;
  }

  if (rcmd2 == NULL){
    i = lview->numCommands++;
    lview->commands[i] = rcmd;

    Reference_ref(rcmd->reference);
    lview->commands[lview->numCommands] = NULL;
    state = LUX_TRUE;
  }
  else{
    // find first occurance
    id = id == 0 ? 1 : id;
    if (id < 0){
      rcmdptr = &lview->commands[lview->numCommands];
      rcmdptr2 = lview->commands-1;
      i = -1;
      id = -id;
    }
    else{
      rcmdptr = lview->commands;
      rcmdptr2 = &lview->commands[lview->numCommands+1];
      i = 1;
    }
    state = LUX_FALSE;
    for (;rcmdptr != rcmdptr2; rcmdptr+=i){
      if (*rcmdptr == rcmd2 && (--id) == 0){
        // raise by one
        i = lview->numCommands++;
        lview->commands[lview->numCommands] = NULL;
        // shift old
        rcmdptr+= before ? 0 : 1;
        rcmdptr2 = &lview->commands[i];

        while (rcmdptr2 != rcmdptr){
          rcmd2 = *(rcmdptr2-1);
          *rcmdptr2 = rcmd2;
          rcmdptr2--;
          i--;
        }

        // insert new
        *rcmdptr = rcmd;

        Reference_ref(rcmd->reference);
        state = LUX_TRUE;
        break;
      }
    }
  }

  return state;
}

void RList3DView_free (lxRl3dview ref) {
  List3DView_free((List3DView_t*)Reference_value(ref));
}


//////////////////////////////////////////////////////////////////////////
// LIST3D Administration

void List3D_init(int maxSets, int maxLayers, int maxDrawPerLayer, int maxTotalProjectors)
{
  int i;
  List3DView_t *view;
  char *memory;

  memset(&g_List3D,0,sizeof(List3D_t));

  g_List3D.maxSets = maxSets;
  g_List3D.maxLayers = maxLayers;
  g_List3D.maxDrawPerLayer = maxDrawPerLayer;
  g_List3D.maxDrawPerLayerMask = maxDrawPerLayer-1;
  g_List3D.maxTotalProjectors = maxTotalProjectors;

  Reference_registerType(LUXI_CLASS_L3D_NODE,"l3dnode",RList3DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_PRIMITIVE,"l3dprimitive",RList3DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_MODEL,"l3dmodel",RList3DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_LIGHT,"l3dlight",RList3DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_PROJECTOR,"l3dprojector",RList3DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_TRAIL,"l3dtrail",RList3DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_LEVELMODEL,"l3dlevelmodel",RList3DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_SHADOWMODEL,"l3dshadowmodel",RList3DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_TEXT,"l3dtext",RList3DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_PGROUP,"l3dpgroup",RList3DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_PEMITTER,"l3dpemitter",RList3DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_CAMERA,"l3dcamera",RList3DNode_free,NULL);

  Reference_registerType(LUXI_CLASS_L3D_BATCHBUFFER,"l3dbatchbuffer",RList3DBatchBuffer_free,NULL);
  Reference_registerType(LUXI_CLASS_L3D_VIEW,"l3dview",RList3DView_free,NULL);

  memory = lxMemGenZalloc((sizeof(List3DSet_t)*LIST3D_SETS)+(sizeof(List3DLayerData_t)*LIST3D_LAYERS));

  g_List3D.drawSets = (List3DSet_t*)memory;
  l_l3ddraw.layerdata = (List3DLayerData_t*)(memory+(sizeof(List3DSet_t)*LIST3D_SETS));

  for (i = 0; i < LIST3D_SETS; i++){
    view = &g_List3D.drawSets[i].defaultView;
    memset(view,0,sizeof(List3DView_t));
    view->isdefault = LUX_TRUE;
    view->layerfill = LUX_TRUE;
    view->viewport.bounds.fullwindow = LUX_TRUE;
    view->viewport.bounds.x = 0;
    view->viewport.bounds.y = 0;
    view->viewport.bounds.width = g_VID.windowWidth;
    view->viewport.bounds.height = g_VID.windowHeight;
    view->viewport.depth[0] = 0.0f;
    view->viewport.depth[1] = 1.0f;
    view->maxCommands = LIST3D_VIEW_DEFAULT_COMMANDS;
    view->camvisflag = BIT_ID_1;
    view->reference = Reference_new(LUXI_CLASS_L3D_VIEW,view);
    Reference_makeStrong(view->reference);

    lxVector4Set(view->background.fogcolor,1,1,1,0);
    view->background.fogmode = 0;
    view->background.fogstart = 0;
    view->background.fogend = 1024;
    view->background.fogdensity = 1;
    view->background.skybox = NULL;
    view->rcmdflag = 1;

    lxListNode_init(view);
    g_List3D.drawSets[i].viewListHead = view;
  }

  g_List3D.setdefault = 0;
  g_List3D.layerdefault = 0;

  LUX_PROFILING_OP (g_Profiling.global.nodes.l3ds = 0);

  BoneControl_init();

  ModelControl_init();

  List3D_updateMemPoolRelated();
}

void List3D_deinit()
{
  List3DSet_t *lset;
  ParticleCloud_t *pcloud;
  ParticleSys_t *psys;
  int i;

  lset = g_List3D.drawSets;
  for (i = 0; i < LIST3D_SETS; i++,lset++) {
    List3DView_t* view = &g_List3D.drawSets[i].defaultView;
    Int2Node_t* inbrowse;

    while(lset->pcloudListHead){
      lxListNode_popBack(lset->pcloudListHead,inbrowse);
      LUX_ASSERT(inbrowse);

      pcloud = ResData_getParticleCloud(inbrowse->data);
      if(pcloud)pcloud->l3dset = NULL;
      Int2Node_free(inbrowse);
    }

    while(lset->psysListHead){
      lxListNode_popBack(lset->psysListHead,inbrowse);
      LUX_ASSERT(inbrowse);

      psys = ResData_getParticleSys(inbrowse->data);
      if(psys)psys->l3dset = NULL;
      Int2Node_free(inbrowse);
    }

    List3DView_rcmdEmpty(view);

    Reference_invalidate(view->reference);
    Reference_release(view->reference);
  }

  rpoolsetbegin(g_List3D.projBufferArray);

  lxMemGenFree(g_List3D.drawSets,(sizeof(List3DSet_t)*LIST3D_SETS)+(sizeof(List3DLayerData_t)*LIST3D_LAYERS));
}


void List3D_updateMemPoolRelated()
{
  List3DLayerData_t *ldata;
  int i;

  rpoolclear();
  for (i = 0; i < LIST3D_LAYERS; i++){
    ldata = &l_l3ddraw.layerdata[i];
    ldata->sortNodes = rpoolmalloc(sizeof(DrawSortNode_t)*LIST3D_LAYER_MAX_NODES);
  }

  g_List3D.projBufferArray = rpoolmalloc(sizeof(Projector_t*)*LIST3D_MAX_TOTAL_PROJECTORS);
  g_List3D.projBufferArrayEnd = rpoolcurrent();
  g_List3D.visibleBufferStart = rpoolcurrent();
}

int  List3D_getDefaultLayer()
{
  return (g_List3D.setdefault*LIST3D_LAYERS+g_List3D.layerdefault);
}

// particle handling
int List3DNode_activatePsys(const int resRID, List3DSet_t *l3dset)
{
  Int2Node_t *inbrowse;
  List3DSet_t *setbrowse;

  ParticleSys_t *psys;
  int i;

  psys = ResData_getParticleSys(resRID);

  if (psys->l3dset == l3dset)
    return LUX_TRUE;

  if (!psys->l3dset && ResData_lastReloadFrame()>=g_VID.frameCnt-1){
    // a bit of a special case, in that case the above l3dset would have been killed,
    // and we have to manually search for it
    setbrowse=g_List3D.drawSets;
    for (i = 0; i < LIST3D_SETS; i++,setbrowse++){
      lxListNode_forEach(setbrowse->psysListHead,inbrowse)
        if(inbrowse->data == resRID){
          if (setbrowse != l3dset){
            psys->l3dset = setbrowse;
            break;
          }
          else{
            psys->l3dset = l3dset;
            return LUX_TRUE;
          }
        }
      lxListNode_forEachEnd(setbrowse->psysListHead,inbrowse);
      if (psys->l3dset)
        break;
    }
  }
  if (psys->l3dset){


    if (psys->activeEmitterCnt)
      return LUX_FALSE;

    // remove and clear HACK, since we simply kill all particles in the set before
    // find
    lxListNode_forEach(psys->l3dset->psysListHead,inbrowse)
      if (inbrowse->data == resRID){
        lxListNode_remove(psys->l3dset->psysListHead,inbrowse);
        ParticleContainer_reset(&psys->container);
        bprintf("ERROR: moved particlesys to other set, %s",psys->resinfo.name);
        break;
      }
    lxListNode_forEachEnd(psys->l3dset->psysListHead,inbrowse);

  }
  else{
    // creat new intnode
    inbrowse = Int2Node_new(resRID,psys->sortkey);
  }

  psys->l3dset = l3dset;
  lxListNode_addLast(l3dset->psysListHead,inbrowse);


  return LUX_TRUE;
}

static int List3DNode_activatePCloud(const int resRID, List3DSet_t *l3dset)
{
  Int2Node_t *inbrowse;
  ParticleCloud_t *pcloud;
  List3DSet_t *setbrowse;
  int i;

  pcloud = ResData_getParticleCloud(resRID);

  if (pcloud->l3dset == l3dset)
    return LUX_TRUE;

  if (!pcloud->l3dset && ResData_lastReloadFrame()>=g_VID.frameCnt-1){
    // a bit of a special case, in that case the above l3dset would have been killed,
    // and we have to manually search for it
    setbrowse=g_List3D.drawSets;
    for (i = 0; i < LIST3D_SETS; i++,setbrowse++){
      lxListNode_forEach(setbrowse->pcloudListHead,inbrowse)
        if(inbrowse->data == resRID){
          if (setbrowse != l3dset){
            pcloud->l3dset = setbrowse;
            break;
          }
          else{
            pcloud->l3dset = l3dset;
            return LUX_TRUE;
          }
        }
        lxListNode_forEachEnd(setbrowse->pcloudListHead,inbrowse);
        if (pcloud->l3dset)
          break;
    }
  }
  if (pcloud->l3dset){
    if (pcloud->activeGroupCnt)
      return LUX_FALSE;

    // remove and clear HACK, since we simply kill all particles in the set before
    // find
    lxListNode_forEach(pcloud->l3dset->pcloudListHead,inbrowse)
      if (inbrowse->data == resRID){
        lxListNode_remove(pcloud->l3dset->pcloudListHead,inbrowse);
        ParticleContainer_reset(&pcloud->container);
        bprintf("ERROR: moved particlecloud to other set, %s",pcloud->resinfo.name);
        break;
      }
    lxListNode_forEachEnd(pcloud->l3dset->pcloudListHead,inbrowse);

  }
  else{
    // creat new intnode
    inbrowse = Int2Node_new(resRID,pcloud->sortkey);
  }

  lxListNode_addLast(l3dset->pcloudListHead,inbrowse);
  pcloud->l3dset = l3dset;

  return LUX_TRUE;
}

// updates cameras and lights
// sets g_CamLight.cameraListHead
void List3D_updateProps()
{
  int i;
  List3DSet_t *lset = g_List3D.drawSets;
  Light_t *light;
  Light_t *templights;
  Projector_t *proj;
  List3DView_t *view;
  int camerasInUse = g_CamLight.camera->bitID;

  g_CamLight.cameraListHead = NULL;
  g_CamLight.projectorListHead = NULL;
  g_CamLight.lightListHead = NULL;

  if (g_CamLight.sunlight->l3dnode){
    List3DNode_updateUp_matrix_recursive(g_CamLight.sunlight->l3dnode);
    Light_update(g_CamLight.sunlight,g_CamLight.sunlight->l3dnode->finalMatrix);
  }
  else
    Light_update(g_CamLight.sunlight,NULL);

  List3DNode_updateUp_matrix_recursive(g_CamLight.camera->l3dnode);
  Camera_update(g_CamLight.camera,g_CamLight.camera->l3dnode->finalMatrix);

  lxLN_init(g_CamLight.camera,gnext,gprev);
  lxLN_addFirst(g_CamLight.cameraListHead,g_CamLight.camera,gnext,gprev);


  g_List3D.projCurrent = g_List3D.projBufferArray;
  for (i = 0; i < LIST3D_SETS; i++, lset++ ){
    if (lset->disabled)
      continue;
    memset(lset->projLookUp,0,sizeof(Projector_t*)*LIST3D_SET_PROJECTORS);
    if (lset->sun){
      if (lset->sun->l3dnode){
        List3DNode_updateUp_matrix_recursive(lset->sun->l3dnode);
        Light_update(lset->sun,lset->sun->l3dnode->finalMatrix);
      }
      else
        Light_update(lset->sun,NULL);
    }

    // update cameras
    lxListNode_forEach(lset->viewListHead,view)
      if (view->camera && !(view->camera->bitID & camerasInUse)){
        List3DNode_updateUp_matrix_recursive(view->camera->l3dnode);
        Camera_update(view->camera,view->camera->l3dnode->finalMatrix);
        lxLN_init(view->camera,gnext,gprev);
        lxLN_addLast(g_CamLight.cameraListHead,view->camera,gnext,gprev);
        camerasInUse |= view->camera->bitID;
      }
    lxListNode_forEachEnd(lset->viewListHead,view);


    // we update all lights and projects
    // and throw them in a  global list, this is done for vistesting
    // as projects and lights can be linked to l3dnodes we must update their position
    // as well, we do this with the recursive update func that simply bubbles up the hierarchy

    // iterate thru all lights
    // throw out all that are overtime
    templights = lset->fxlightListHead;
    lset->fxlightListHead = NULL;
    while (templights != NULL){
      lxListNode_popFront(templights,light);
      if (!light->removetime || light->removetime > g_LuxTimer.time){
        lxListNode_addLast(lset->fxlightListHead,light);
        // all fx lights are l3dlights
        List3DNode_updateUp_matrix_recursive(light->l3dnode);
        Light_update(light,light->l3dnode->finalMatrix);

        lxLN_init(light,gnext,gprev);
        lxLN_addLast(g_CamLight.lightListHead,light,gnext,gprev);

        light->setID = i;

        LUX_PROFILING_OP (g_Profiling.perframe.draw.fxlights++);
      }
      else{
        light->removetime = 0;
      }
    }
    // update projectors
    lxListNode_forEach(lset->projectorListHead,proj)
      // all projectors are l3dproj
      List3DNode_updateUp_matrix_recursive(proj->l3dnode);
      Projector_update(proj,proj->l3dnode->finalMatrix);

      lxLN_init(proj,gnext,gprev);
      lset->projLookUp[proj->id] = proj;
      lxLN_addLast(g_CamLight.projectorListHead,proj,gnext,gprev);
      proj->setID = i;

      LUX_PROFILING_OP (g_Profiling.perframe.draw.projectors++);

    lxListNode_forEachEnd(lset->projectorListHead,proj);
  }

}

void List3D_activateProjector(int l3dsetID,Projector_t *proj,const int id)
{
  List3DSet_t *l3dset = &g_List3D.drawSets[l3dsetID];
  LUX_ASSERT(proj);
  // already active
  if (proj->l3dset)
    return;

  lxListNode_init(proj);
  proj->id = id;
  proj->bitID = 1<<id;
  lxListNode_addLast(l3dset->projectorListHead,proj);
  proj->l3dset = l3dset;
  proj->setID = l3dsetID;
}

void List3D_deactivateProjector(Projector_t *proj)
{
  Projector_t **projlist;
  LUX_ASSERT(proj);
  if (proj->l3dset){
    projlist=&proj->l3dset->projectorListHead;
    lxListNode_remove(*projlist,proj);
  }
  proj->bitID = 0;
  proj->l3dset = NULL;
}


void List3D_deactivateLight(Light_t *light)
{
  Light_t **lightlist;
  LUX_ASSERT(light);
  if (light->l3dset){
    lightlist=&light->l3dset->fxlightListHead;
    lxListNode_remove(*lightlist,light);
    //light->l3dset->fxlightListHead = lightlist;
  }
  light->removetime = 0;
  light->l3dset = NULL;
}

void List3D_activateLight(int l3dsetID,Light_t *light, const uchar priority, const float range, const uint duration)
{
  List3DSet_t *l3dset = &g_List3D.drawSets[l3dsetID];
  LUX_ASSERT(light);
  // return if already in list or if light is off
  if (light->l3dset)
    return;
  if (duration)
    light->removetime = g_LuxTimer.time + duration;
  else
    light->removetime = 0;

  light->priority = priority;

  light->range = range;
  light->rangeSq = range * range;

  lxListNode_addLast(l3dset->fxlightListHead,light);
  light->l3dset = l3dset;
}

void List3D_removeCamera(Camera_t *cam)
{
  int i;
  List3DSet_t *lset = g_List3D.drawSets;
  List3DView_t *view;
  LUX_ASSERT(cam);
  for (i = 0; i < LIST3D_SETS; i++, lset++ ){
    lxListNode_forEach(lset->viewListHead,view)
      if (view->camera == cam)
        view->camera = NULL;
    lxListNode_forEachEnd(lset->viewListHead,view);
  }
}
void List3D_removeLight(Light_t *light)
{
  int i;
  List3DSet_t *lset = g_List3D.drawSets;
  LUX_ASSERT(light);
  for (i = 0; i < LIST3D_SETS; i++, lset++ ){
    if (lset->sun == light)
      lset->sun = NULL;
  }
  List3D_deactivateLight(light);
}
void List3D_removeProjector(Projector_t *proj)
{
  List3D_deactivateProjector(proj);
}

void List3D_removeSkyBox(SkyBox_t *skybox)
{
  int i;
  List3DSet_t *lset = g_List3D.drawSets;
  List3DView_t *view;
  LUX_ASSERT(skybox);
  for (i = 0; i < LIST3D_SETS; i++, lset++ ){
    lxListNode_forEach(lset->viewListHead,view)
      if (view->background.skybox == skybox)
        view->background.skybox = NULL;
    lxListNode_forEachEnd(lset->viewListHead,view);
  }
}



List3DNode_t *List3D_getNodeByName(const char *name)
{
  // FIXME
  return NULL;
}


