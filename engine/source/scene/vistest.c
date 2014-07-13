// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "vistest.h"
#include "../common/interfaceobjects.h"
#include "../common/common.h"
#include "../common/3dmath.h"
#include "../render/gl_camlight.h"
#include "../render/gl_render.h"
#include "../render/gl_list3d.h"
#include "../render/gl_draw3d.h"
#include "../scene/actorlist.h"
#include "../scene/scenetree.h"

// LOCALS
static VisTest_t  l_VisTest;

#define CAMBIT(key)   (1<<key)


////////////////////////////////////////////////////////////////////////////////
// VisObject

// constructor
VisObject_t*  VisObject_new(VisObjectType_t type,void *data,char *name)
{
  VisObject_t* self;
  self = lxMemGenZalloc(sizeof(VisObject_t));
  lxListNode_init(self);

  self->type = type;
  self->userdata = data;
  self->name = name;
  self->curvisset = -1;

  switch(self->type) {
  case VIS_OBJECT_STATIC:
    lxListNode_addLast(l_VisTest.voStaticListHead,self);
    l_VisTest.voStaticCnt++;
    l_VisTest.staticCompile = LUX_TRUE;
#ifdef VISTEST_FRUSTUM_USE_BBOX
    self->usetbbox = LUX_TRUE;
#endif
    break;
  case VIS_OBJECT_DYNAMIC:
    lxListNode_addLast(l_VisTest.voDynamicListHead,self);
    l_VisTest.voDynamicCnt++;
    break;
  default:
    LUX_ASSERT(0);
    break;
  }

  return self;
}

// destructor
void  VisObject_free(VisObject_t *self)
{
  VisSet_t *visset;
  List3DNode_t *l3dnode;

  switch(self->type) {
  case VIS_OBJECT_STATIC:
    lxListNode_remove(l_VisTest.voStaticListHead,self);
    l_VisTest.voStaticCnt--;
    l_VisTest.staticCompile = LUX_TRUE;
    break;
  case VIS_OBJECT_DYNAMIC:
    lxListNode_remove(l_VisTest.voDynamicListHead,self);
    l_VisTest.voDynamicCnt--;
    break;
  default:
    break;
  }

  while (self->l3dnodesListHead){
    lxLN_popFront(self->l3dnodesListHead,l3dnode,linknext,linkprev);
    List3DNode_unlink(l3dnode);
  }

  visset = &self->visset;
  if (visset->vislight)
    lxMemGenFree(visset->vislight,sizeof(VisLight_t));

  lxMemGenFree(self,sizeof(VisObject_t));
}

// only allowed to be called for scenenode/actor
static void VisObject_updateMatrix(VisObject_t *self, lxBoundingBox_t *transformedbox)
{
  float *mat;

  if (self->type == VIS_OBJECT_STATIC){
    mat = self->scenenode->link.matrix;
    // reset on recompile
    lxVector3Transform(self->tcaporigin,self->bcapsule.origin,mat);
    lxVector3TransformRot(self->tcapdir,self->bcapsule.toEnd,mat);
    self->tcaptime = 0xFFFFFFFF;
  }
  else{
    mat = Matrix44Interface_getElements(self->actor->link.matrixIF,l_VisTest.matrix);
  }

  if (transformedbox){
    lxBoundingBox_transform(transformedbox,&self->bbox,mat);
  }
  else{
    lxVector3 ctr;
    lxVector3Add(ctr,self->bbox.max,self->bbox.min);
    lxVector3Scale(ctr,ctr,0.5f);
    lxVector3Transform(self->center,ctr,mat);
  }
}


void VisObject_setVissetLight(VisObject_t *self, int state, int full)
{
  VisSet_t  *visset = &self->visset;
  if (state){
    if (full)
      visset->needslightFull++;
    else
      visset->needslightNonsun++;

    if (!visset->vislight){
      visset->vislight = lxMemGenZalloc(sizeof(VisLight_t));
      visset->vislight->full.maxlightdist = VISTEST_LIGHT_DIST;
      visset->vislight->nonsun.maxlightdist = VISTEST_LIGHT_DIST;
    }

  }
  else{
    if (full)
      visset->needslightFull--;
    else
      visset->needslightNonsun--;

    if (visset->needslightFull == 0 && visset->needslightNonsun == 0 && visset->vislight){
      lxMemGenFree(visset->vislight,sizeof(VisLight_t));
      visset->vislight = NULL;
    }
  }
}

void VisObject_setBBox(VisObject_t *vis, lxBoundingBox_t *bbox)
{
  lxVector3 size;
  if(vis->bboxset){
    if (vis->type == VIS_OBJECT_STATIC){
      l_VisTest.staticCompile = lxBoundingBox_mergeChange(&vis->bbox,&vis->bbox,bbox);
    }
    else
      lxBoundingBox_merge(&vis->bbox,&vis->bbox,bbox);
  }
  else{
    if (vis->type == VIS_OBJECT_STATIC){
      l_VisTest.staticCompile = !(lxVector3Compare(bbox->min,==,vis->bbox.min) && lxVector3Compare(bbox->max,==,vis->bbox.max));
    }
    lxBoundingBox_copy(&vis->bbox,bbox);
    vis->bboxset = LUX_TRUE;
  }
  lxVector3Sub(size, vis->bbox.max, vis->bbox.min);
#ifdef VISTEST_FRUSTUM_USE_BBOX
  vis->radius = lxVector3LengthFast(size) * 0.5f;
#else
  lxBoundingBox_toSphere(&vis->bbox,&vis->bsphere);
#endif
  lxBoundingBox_toCapsule(&vis->bbox,&vis->bcapsule);
}

void VisObject_setBBoxForce(VisObject_t *vis, lxVector3 min, lxVector3 max)
{
  lxVector3 size;
  if (vis->type == VIS_OBJECT_STATIC){
    l_VisTest.staticCompile = !(lxVector3Compare(min,==,vis->bbox.min) && lxVector3Compare(max,==,vis->bbox.max));
  }

  lxVector3Copy(vis->bbox.min,min);
  lxVector3Copy(vis->bbox.max,max);
  lxVector3Sub(size, max, min);

#ifdef VISTEST_FRUSTUM_USE_BBOX
  vis->radius = lxVector3LengthFast(size) * 0.5f;
#else
  lxBoundingBox_toSphere(&vis->bbox,&vis->bsphere);
#endif
  lxBoundingBox_toCapsule(&vis->bbox,&vis->bcapsule);
  vis->bboxset = LUX_TRUE;

}


static void VisObject_testFrustum(VisObject_t *visobject,const lxFrustum_t *frustum, enum32 bitID)
{
  List3DNode_t *browse;
  // perspective so we can do Cone check
  if (l_VisTest.traverseproj < 0)
    LUX_PROFILING_OP (g_Profiling.perframe.vischeck.frustumChecksCam++);
  else
    LUX_PROFILING_OP (g_Profiling.perframe.vischeck.frustumChecksProj++);

#ifndef VISTEST_FRUSTUM_USE_BBOX
  {
    lxBoundingSphere_t tsphere;
    lxVector3Copy(tsphere.center,visobject->center);
    tsphere.radius = visobject->radius;
    if (lxFrustum_checkSphereCoherent(frustum,&tsphere,&visobject->lastclipplane))
      return;
  }
#else
  if (visobject->usetbbox){
    if (lxFrustum_checkBoundingBoxCoherent(frustum,&visobject->tbbox,&visobject->lastclipplane))
      return;
  }
  else{
    lxBoundingSphere_t tsphere;
    lxVector3Copy(tsphere.center,visobject->center);
    tsphere.radius = visobject->radius;
    if (lxFrustum_checkSphereCoherent(frustum,&tsphere,&visobject->lastclipplane))
      return;
  }
#endif

  if (l_VisTest.traverseproj < 0){
    if (!visobject->cameraVis){
      // it became visible now add l3dnodes to visibles
      lxLN_forEach(visobject->l3dvisListHead,browse,visnext,visprev)
          *(g_List3D.drawSets[browse->setID].visCurrent++) = browse;
      lxLN_forEachEnd(visobject->l3dvisListHead,browse,visnext,visprev);
      *(l_VisTest.visCurrent++) = visobject;
    }
    visobject->cameraVis |= bitID;
  }
  else if (l_VisTest.traverseproj == visobject->curvisset) {
    visobject->visset.projectorVis |= bitID;
  }
}

static void VisLightResult_update(VisLightResult_t *res, const float dist, const Light_t *light)
{
  float *pDist;
  int i;
  // replace the last maximum distant light
  if (res->numLights >= VISTEST_LIGHT_CONTACTS){
    res->lights[res->maxlight] = light;
    res->lightdists[res->maxlight] = dist;
  }
  else{ // add it to the lights
    res->lights[res->numLights] = light;
    res->lightdists[res->numLights++] = dist;
  }

  // find new maximum
  res->maxlightdist = 0;
  pDist = res->lightdists;
  i = 0;
  while (i < res->numLights){
    if (*pDist > res->maxlightdist){
      res->maxlightdist = *pDist;
      res->maxlight = i;
    }
    pDist++;
    i++;
  }
}

static void VisObject_testLight(VisObject_t *visobject,const VisSet_t *visset,const VisTestLight_t *vlight, const lxVector3 pos)
{
  float dist;
  float *mat;
  VisLight_t  *vislight;

  vislight = visset->vislight;


  if (visobject->tcaptime < g_LuxTimer.time){
    //mat = (visobject->type == VIS_OBJECT_STATIC) ? visobject->scenenode->link.matrix : Matrix44Interface_getElements(visobject->actor->link.matrixIF,l_VisTest.matrix);
    mat = visobject->linkobject->matrix;
    lxVector3Transform(visobject->tcaporigin,visobject->bcapsule.origin,mat);
    lxVector3TransformRot(visobject->tcapdir,visobject->bcapsule.toEnd,mat);
    visobject->tcaptime = g_LuxTimer.time;
  }

  dist = lxVector3LineDistanceSq(pos,visobject->tcaporigin,visobject->tcapdir);

  if (dist - visobject->bcapsule.radiusSqr > vlight->rangeSq)
    return;

  if (vlight->nonsunlit && visset->needslightNonsun && dist < vislight->nonsun.maxlightdist && vislight->nonsun.numLights < VISTEST_LIGHT_CONTACTS){
    VisLightResult_update(&vislight->nonsun,dist,vlight->light);
  }

  if (!visset->needslightFull && dist > vislight->full.maxlightdist && vislight->full.numLights >= VISTEST_LIGHT_CONTACTS)
    return;

  VisLightResult_update(&vislight->full,dist,vlight->light);
}


//////////////////////////////////////////////////////////////////////////
// Vis Test Collision

static void VisTest_traverse (uint listcount, lxOcContainerBox_t** list, void **dataList, void *upvalue)
{
  VisObject_t *visobj;
  List3DNode_t *l3dbrowse;
  ulong bits;
  uint i;

  bits = (ulong)upvalue;

  if (l_VisTest.traverseproj < 0){
    for (i = 0; i < listcount; i++){
      visobj = (VisObject_t*)dataList[i];

      LUX_DEBUGASSERT(visobj);
      if (!visobj->cameraVis){
        // it became visible now add l3dnodes to visibles
        lxLN_forEach(visobj->l3dvisListHead,l3dbrowse,visnext,visprev)
          *(g_List3D.drawSets[l3dbrowse->setID].visCurrent++) = l3dbrowse;
        lxLN_forEachEnd(visobj->l3dvisListHead,l3dbrowse,visnext,visprev);
        *(l_VisTest.visCurrent++) = visobj;
      }
      visobj->cameraVis |= bits;
      LUX_PROFILING_OP (g_Profiling.perframe.vischeck.quickAcceptsCam++);
    }
  }
  else{
    for (i = 0; i < listcount; i++){
      visobj = (VisObject_t*)dataList[i];

      LUX_DEBUGASSERT(visobj);
      if (visobj->curvisset == l_VisTest.traverseproj)
        visobj->visset.projectorVis |= bits;
      LUX_PROFILING_OP (g_Profiling.perframe.vischeck.quickAcceptsProj++);
    }
  }

}


static int VisTest_contactFrustum (lxOcContainerBox_t* incontbox, lxOcNode_t* node, int depth, void *upvalue)
{
  VisTestFrustum_t *vfrustum = (VisTestFrustum_t*)(incontbox+1);
  lxCullType_t ctype = lxFrustum_cullBoundingBox(&vfrustum->frustum, (lxBoundingBox_t*)&node->bounds);

  // fully outside
  if (ctype == LUX_CULL_OUTSIDE){
    return LUX_FALSE;
  }

  l_VisTest.traverseproj = vfrustum->setID;
  if (ctype == LUX_CULL_INSIDE){

    // full node lies in the frustum we set all children as visible and stop branching
    lxOcNode_getDeepLists(node,VisTest_traverse,(void*)vfrustum->bitID);
    return LUX_FALSE;
  }

  // if the list contains few elements, a precheck is overhead
  if (node->listCount
    && (node->listCount < l_VisTest.tweaks[VIS_TWEAK_VOLCHECKCNT] ||
    lxFrustum_cullBoundingBox(&vfrustum->frustum, (lxBoundingBox_t*)&node->volume))
    )
  {
    uint i;
    for (i = 0; i < node->listCount; i++){
      VisObject_t *visobj = node->dataList[i];

      // two cases, camera or projector testing
      if ((l_VisTest.traverseproj < 0 && visobj->cameraFlag & vfrustum->bitID)
        || (
#ifndef VISTEST_DYNAMIC_VISIBLESPACE
        visobj->cameraVis &&
#endif
        l_VisTest.traverseproj == visobj->curvisset && visobj->visset.projectorFlag & vfrustum->bitID))
      {
        VisObject_testFrustum(visobj, &vfrustum->frustum,vfrustum->bitID);
      }
    }
  }

  return LUX_TRUE; // continue the branching
}


static int VisTest_contactLight (lxOcContainerBox_t* incontbox, lxOcNode_t* node, int depth, void *upvalue)
{
  // if the list contains few elements, a precheck is overhead
  // if the light doesn't intersect
  // the bounds of the list,  I don't need to check the list elements

  if (node->listCount && (node->listCount < l_VisTest.tweaks[VIS_TWEAK_VOLCHECKCNT] ||
    (lxMinMax_intersectsV(incontbox->box.bounds.min,node->volume.min)))
    )
  {
    VisTestLight_t *vlight = (VisTestLight_t *)(incontbox+1);
    uint i;
    LUX_ASSERT(vlight);

    for (i = 0; i < node->listCount; i++){
      VisObject_t *visobj = node->dataList[i];
      VisSet_t *visset = &visobj->visset;


      // we only test nodes that are visible
      if (
#ifndef VISTEST_DYNAMIC_VISIBLESPACE
        visobj->cameraVis &&
#endif
        visobj->curvisset == vlight->setID && visset->vislight)
      {
        lxVector3 sub;
#ifdef VISTEST_FRUSTUM_USE_BBOX
        float dist = visobj->radius + vlight->range;
#else
        float dist = visobj->bsphere.radius + vlight->range;
#endif
        lxVector3Sub(sub,incontbox->box.center,visobj->center);

        if (lxVector3Dot(sub,sub) < dist*dist){
          VisObject_testLight(visobj,visset,vlight,incontbox->box.center);
        }
      }

    }
  }

  return 1; // continue the branching
}

////////////////////////////////////////////////////////////////////////////////
// Vis Test

void VisTest_init()
{
  int i;
  VisObject_t *visobject;

  LUX_SIMDASSERT(((size_t)l_VisTest.matrix) % 16 == 0);

  memset(&l_VisTest,0,sizeof(VisTest_t));

  for (i = 0,visobject = l_VisTest.voLights; i < VISTEST_MAX_LIGHTS; i++,visobject++){
    visobject->type = VIS_OBJECT_LIGHT;
#ifdef VISTEST_FRUSTUM_USE_BBOX
    visobject->usetbbox = LUX_FALSE;
#endif
  }

  l_VisTest.tweaks[VIS_TWEAK_VOLCHECKCNT] = VISTEST_PRECHECK_LIST_MIN;
  for (i = 0; i < VIS_TWEAK_VOLCHECKCNT; i++)
    l_VisTest.tweaks[i] = VISTEST_MAX_DEPTH;

  l_VisTest.cameraSpace = lxOcTree_new(GLOBAL_ALLOCATOR,1<<9);
  l_VisTest.projectorSpace = lxOcTree_new(GLOBAL_ALLOCATOR,1<<9);
  l_VisTest.lightSpace = lxOcTree_new(GLOBAL_ALLOCATOR,1<<9);

  l_VisTest.dynamicSpace = lxOcTree_new(GLOBAL_ALLOCATOR,1<<9);
  l_VisTest.staticSpace = lxOcTree_new(GLOBAL_ALLOCATOR,1<<9);
}

void VisTest_free()
{
  lxOcTree_delete(l_VisTest.projectorSpace);
  lxOcTree_delete(l_VisTest.cameraSpace);
  lxOcTree_delete(l_VisTest.lightSpace);

  lxOcTree_delete(l_VisTest.staticSpace);
  lxOcTree_delete(l_VisTest.dynamicSpace);
}

void VisTest_recompileStatic()
{
  l_VisTest.staticCompile = LUX_TRUE;
}

void VisTest_addFrustum(lxOcTreePTR camspace, flags32 bitID, int setID, lxFrustum_t *frustum, lxVector3 box[LUX_FRUSTUM_CORNERS])
{
  VisTestFrustum_t *vfrustum;
  lxBoundingBox_t bbox;
  lxBoundingBoxCenter_t ctr;

  lxBoundingBox_fromCorners(&bbox,box);
  lxBoundingBox_toCenterBox(&bbox,&ctr);
  vfrustum = lxOcTree_add(l_VisTest.cameraSpace,NULL,lxVector3Unpack(ctr.center),lxVector3Unpack(ctr.size));
  vfrustum->bitID = bitID;
  vfrustum->frustum = *frustum;
  vfrustum->setID = setID;
}

void VisTest_run()
{
  Camera_t *cam;
  Projector_t *proj;
  Light_t   *light;
  lxBoundingBox_t bbox;
  lxBoundingBoxCenter_t ctr;
  List3DSet_t *l3dset;
  List3DNode_t *l3dbrowse;
  VisObject_t *visobject;
  List3DNode_t  **l3dptr;
  VisObject_t   **visptr;
  size_t    shiftlen;
  int i;


  int   usedLight = 0;
  int   usedProj = 0;
  int   usedCam = 0;

  float size;

  enum32  camRID;

  if (!g_CamLight.cameraListHead)
    return;

  if (g_Draws.drawAll)
    camRID = BIT_ID_FULL32;
  else
    camRID = 0;


  // create our visnode arrays, note that although we wont have
  // that many visnodes, its a worst case approximation

  l3dset = g_List3D.drawSets;
  for (i = 0; i < LIST3D_SETS; i++,l3dset++){
    l3dset->visCurrent = l3dset->visibleBufferArray = rpoolmalloc(sizeof(List3DNode_t*)*(l3dset->numNodes+1));
  }
  LUX_PROFILING_OP_MAX(g_Profiling.perframe.scratch.renderpoolmax,rpoolgetinuse());

  // recompile static tree if needed
  if (l_VisTest.staticCompile){
    lxOcTree_reset(l_VisTest.staticSpace);
    lxListNode_forEach(l_VisTest.voStaticListHead,visobject)

      VisObject_updateMatrix(visobject,&bbox);
#ifdef VISTEST_FRUSTUM_USE_BBOX
      lxBoundingBox_copy(&visobject->tbbox,&bbox);
#endif
      lxBoundingBox_toCenterBox(&visobject->tbbox, &ctr);
      lxOcTree_add(l_VisTest.staticSpace,visobject,lxVector3Unpack(ctr.center),lxVector3Unpack(ctr.size));
    lxListNode_forEachEnd(l_VisTest.voStaticListHead,visobject);
    l_VisTest.staticCompile = LUX_FALSE;
    // todo add customdata
    lxOcTree_build(l_VisTest.staticSpace,l_VisTest.tweaks[VIS_TWEAK_MAXDEPTH_SCT],0,NULL,NULL);
  }

  // build dynamic tree
  lxOcTree_reset(l_VisTest.dynamicSpace);
  lxListNode_forEach(l_VisTest.voDynamicListHead,visobject)

    VisObject_updateMatrix(visobject,NULL);
#ifdef VISTEST_FRUSTUM_USE_BBOX
    size = visobject->radius * 2.0f;
#else
    size = visobject->bsphere.radius*2.0f;
#endif
    lxOcTree_add(l_VisTest.dynamicSpace,visobject,lxVector3Unpack(visobject->center),size,size,size);

  lxListNode_forEachEnd(l_VisTest.voDynamicListHead,visobject);
  lxOcTree_build(l_VisTest.dynamicSpace,l_VisTest.tweaks[VIS_TWEAK_MAXDEPTH_ACT],0,NULL,NULL);

  // add cameras
  lxOcTree_reset(l_VisTest.cameraSpace);
  lxOcTree_containerExtraMem(l_VisTest.cameraSpace,sizeof(VisTestFrustum_t));
  lxLN_forEach (g_CamLight.cameraListHead,cam,gnext,gprev)

    LUX_ASSERT(cam);

    if (cam->frustcontListHead){
      FrustumObject_t *fcont;
      lxListNode_forEach(cam->frustcontListHead,fcont)
        VisTest_addFrustum(l_VisTest.cameraSpace,cam->bitID,-1,&fcont->frustum,fcont->frustumBoxCorners);
      lxListNode_forEachEnd(cam->frustcontListHead,fcont);
    }
    else{
      VisTest_addFrustum(l_VisTest.cameraSpace,cam->bitID,-1,&cam->frustum,cam->frustumBoxCorners);
    }


    usedCam++;
  lxLN_forEachEnd(g_CamLight.cameraListHead,cam,gnext,gprev);
  lxOcTree_build(l_VisTest.cameraSpace,l_VisTest.tweaks[VIS_TWEAK_MAXDEPTH_CAM],0,NULL,NULL);

  // add projectors
  lxOcTree_reset(l_VisTest.projectorSpace);
  lxOcTree_containerExtraMem(l_VisTest.projectorSpace,sizeof(VisTestFrustum_t));
  lxLN_forEach (g_CamLight.projectorListHead,proj,gnext,gprev)
    VisTestFrustum_t *vfrustum;
    LUX_ASSERT(proj);

    lxBoundingBox_fromCorners(&bbox,proj->frustumBoxCorners);
    lxBoundingBox_toCenterBox(&bbox, &ctr);
    vfrustum = lxOcTree_add(l_VisTest.projectorSpace,NULL,lxVector3Unpack(ctr.center),lxVector3Unpack(ctr.size));
    vfrustum->bitID = proj->bitID;
    vfrustum->frustum = proj->frustum;
    vfrustum->setID = proj->setID;

    usedProj++;
  lxLN_forEachEnd(g_CamLight.projectorListHead,proj,gnext,gprev);
  lxOcTree_build(l_VisTest.projectorSpace,l_VisTest.tweaks[VIS_TWEAK_MAXDEPTH_PROJ],0,NULL,NULL);

  // add lights
  lxOcTree_reset(l_VisTest.lightSpace);
  visobject = l_VisTest.voLights;
  lxLN_forEach (g_CamLight.lightListHead,light,gnext,gprev)
    LUX_ASSERT(light);
    if (usedLight >= VISTEST_MAX_LIGHTS)
      break;

    visobject->userdata = light;
    visobject->cameraVis = camRID;
    visobject->cameraFlag = BIT_ID_FULL32;
    lxVector3Copy(visobject->center,light->pos);

#ifdef VISTEST_FRUSTUM_USE_BBOX
    visobject->radius = light->range;
    visobject->tbbox.min[0] = visobject->center[0] - visobject->radius;
    visobject->tbbox.min[1] = visobject->center[1] - visobject->radius;
    visobject->tbbox.min[2] = visobject->center[2] - visobject->radius;
    visobject->tbbox.max[0] = visobject->center[0] + visobject->radius;
    visobject->tbbox.max[1] = visobject->center[1] + visobject->radius;
    visobject->tbbox.max[2] = visobject->center[2] + visobject->radius;
#else
    visobject->bsphere.radius = light->range;
#endif
    size = light->range*2;
    lxOcTree_add(l_VisTest.lightSpace,visobject,lxVector3Unpack(visobject->center),size,size,size);

    usedLight++;
    visobject++;
  lxLN_forEachEnd(g_CamLight.lightListHead,light,gnext,gprev);
  lxOcTree_build(l_VisTest.lightSpace,l_VisTest.tweaks[VIS_TWEAK_MAXDEPTH_LIGHT],0,NULL,NULL);

  // worst case approx
  l_VisTest.visCurrent = l_VisTest.visibleBufferArray = rpoolmalloc(sizeof(VisObject_t*)*(l_VisTest.voStaticCnt + usedLight + l_VisTest.voDynamicCnt+1));
  LUX_PROFILING_OP_MAX(g_Profiling.perframe.scratch.renderpoolmax,rpoolgetinuse());

  // all setup is done now test everything

  // first test camers
  // dont bother if we should draw everything
  l_VisTest.traverseproj = -1;
  if (!g_Draws.drawAll ){
    lxOcTree_collide(l_VisTest.staticSpace,NULL,l_VisTest.cameraSpace,VisTest_contactFrustum,(void*)VIS_OBJECT_CAMERA);
    if (usedLight)
      lxOcTree_collide(l_VisTest.lightSpace,NULL,l_VisTest.cameraSpace,VisTest_contactFrustum,(void*)VIS_OBJECT_CAMERA);
    lxOcTree_collide(l_VisTest.dynamicSpace,NULL,l_VisTest.cameraSpace,VisTest_contactFrustum,(void*)VIS_OBJECT_CAMERA);
  }
  else{
    lxListNode_forEach(l_VisTest.voDynamicListHead,visobject)
      visobject->cameraVis = camRID;
      lxLN_forEach(visobject->l3dvisListHead,l3dbrowse,visnext,visprev)
          *(g_List3D.drawSets[l3dbrowse->setID].visCurrent++) = l3dbrowse;
      lxLN_forEachEnd(visobject->l3dvisListHead,l3dbrowse,visnext,visprev);
      *(l_VisTest.visCurrent++) = visobject;
    lxListNode_forEachEnd(l_VisTest.voDynamicListHead,visobject);

    lxListNode_forEach(l_VisTest.voStaticListHead,visobject)
      visobject->cameraVis = camRID;
      lxLN_forEach(visobject->l3dvisListHead,l3dbrowse,visnext,visprev)
        *(g_List3D.drawSets[l3dbrowse->setID].visCurrent++) = l3dbrowse;
      lxLN_forEachEnd(visobject->l3dvisListHead,l3dbrowse,visnext,visprev);
      *(l_VisTest.visCurrent++) = visobject;
    lxListNode_forEachEnd(l_VisTest.voStaticListHead,visobject);
  }
  *l_VisTest.visCurrent = NULL;

#ifdef VISTEST_DYNAMIC_VISIBLESPACE
  {
    VisObject_t **pvisobject = l_VisTest.visibleBufferArray;

    lxOcTree_reset(l_VisTest.dynamicSpace);
    if (usedLight ){
      lxOcTree_reset(l_VisTest.lightSpace);
      lxOcTree_containerExtraMem(l_VisTest.lightSpace,sizeof(VisTestLight_t));
    }

    while ((visobject=*(pvisobject++))){
      if (visobject->type != VIS_OBJECT_LIGHT){
        lxOcTree_add(l_VisTest.dynamicSpace,visobject,lxVector3Unpack(visobject->center),
          visobject->tbbox.max[0]-visobject->tbbox.min[0],
          visobject->tbbox.max[1]-visobject->tbbox.min[1],
          visobject->tbbox.max[2]-visobject->tbbox.min[2]);
      }
      else if (usedLight){
        VisTestLight_t *vlight;
        Light_t     *light = visobject->light;
        size = 2.0f * light->range;
        vlight = lxOcTree_add(l_VisTest.lightSpace,NULL,lxVector3Unpack(light->pos),size,size,size);
        vlight->light = light;
        vlight->range = light->range;
        vlight->rangeSq = light->rangeSq;
        vlight->nonsunlit = light->nonsunlit;
        vlight->setID = light->setID;
      }
    }


    lxOcTree_build(l_VisTest.dynamicSpace,l_VisTest.tweaks[VIS_TWEAK_MAXDEPTH_LIGHTVIS],0,NULL,NULL);

    // test projectors
    if (usedProj ){
      lxOcTree_collide(l_VisTest.dynamicSpace,NULL,l_VisTest.projectorSpace,VisTest_contactFrustum,(void*)VIS_OBJECT_PROJECTOR);
    }
    // test lights
    if (usedLight ){
      lxOcTree_build(l_VisTest.lightSpace,l_VisTest.tweaks[VIS_TWEAK_MAXDEPTH_LIGHTVIS],0,NULL,NULL);
      lxOcTree_collide(l_VisTest.dynamicSpace,NULL,l_VisTest.lightSpace,VisTest_contactLight,NULL);
    }
  }

#else

  // test projectors
  if (usedProj ){
    lxOcTree_collide(l_VisTest.staticSpace,NULL,l_VisTest.projectorSpace,VisTest_contactFrustum,(void*)VIS_OBJECT_PROJECTOR);
    lxOcTree_collide(l_VisTest.dynamicSpace,NULL,l_VisTest.projectorSpace,VisTest_contactFrustum,(void*)VIS_OBJECT_PROJECTOR);
  }
  // test lights
  if (usedLight ){
    // rebuild light space (only insert lights that intersect with camra)
    lxOcTree_reset(l_VisTest.lightSpace);
    lxOcTree_containerExtraMem(l_VisTest.lightSpace,sizeof(VisTestLight_t));

    visobject = l_VisTest.voLights;
    for (usedLight = usedLight; usedLight > 0; usedLight--,visobject++){
      if (visobject->cameraVis){
        VisTestLight_t *vlight;
        Light_t     *light = visobject->light;
        size = 2.0f * light->range;
        vlight = lxOcTree_add(l_VisTest.lightSpace,NULL,lxVector3Unpack(light->pos),size,size,size);
        vlight->light = light;
        vlight->range = light->range;
        vlight->rangeSq = light->rangeSq;
        vlight->nonsunlit = light->nonsunlit;
        vlight->setID = light->setID;
      }
    }
    lxOcTree_build(l_VisTest.lightSpace,l_VisTest.tweaks[VIS_TWEAK_MAXDEPTH_LIGHTVIS],0,NULL,NULL);

    // test lights, must come after camera testing, cause we only check drawable nodes
    lxOcTree_collide(l_VisTest.staticSpace,NULL,l_VisTest.lightSpace,VisTest_contactLight,NULL);
    lxOcTree_collide(l_VisTest.dynamicSpace,NULL,l_VisTest.lightSpace,VisTest_contactLight,NULL);
  }
#endif


#ifdef LUX_MEMPOOL_SAVE
  // compress useage down

  // first l3dsets, can skip first
  l3dset = g_List3D.drawSets;
  *l3dset->visCurrent = NULL;
  l3dptr = l3dset->visCurrent+1;

  l3dset++;
  for (i = 1; i < LIST3D_SETS; i++,l3dset++){
    *l3dset->visCurrent = NULL;
    shiftlen = (l3dset->visCurrent-l3dset->visibleBufferArray)+1;
    if (l3dptr+shiftlen > l3dset->visibleBufferArray)
      memmove(l3dptr,l3dset->visibleBufferArray,shiftlen*sizeof(List3DNode_t*));
    else
      memcpy(l3dptr,l3dset->visibleBufferArray,shiftlen*sizeof(List3DNode_t*));
    l3dset->visibleBufferArray = l3dptr;
    l3dptr += shiftlen;
  }
  // then shift visobjs back
  shiftlen = (l_VisTest.visCurrent - l_VisTest.visibleBufferArray)+1;
  visptr = (VisObject_t**)l3dptr;
  if (visptr+shiftlen > l_VisTest.visibleBufferArray)
    memmove(visptr,l_VisTest.visibleBufferArray,shiftlen*sizeof(VisObject_t*));
  else
    memcpy(visptr,l_VisTest.visibleBufferArray,shiftlen*sizeof(VisObject_t*));

  l_VisTest.visibleBufferArray = visptr;
  visptr += shiftlen;

  rpoolsetbegin(visptr);
#else
  l3dset = g_List3D.drawSets;
  for (i = 0; i < LIST3D_SETS; i++,l3dset++){
    *l3dset->visCurrent = NULL;
  }
  rpoolsetbegin(++l_VisTest.visCurrent);
#endif
}

void VisTest_resetVisible()
{
  VisObject_t** visobjptr = l_VisTest.visibleBufferArray;
  VisObject_t *visobject;
  VisSet_t *visset;

  while (visobject = *(visobjptr++)){
    visobject->cameraVis = 0;
    visset = &visobject->visset;
    visset->projectorVis = 0;
    if (visset->vislight){
      visset->vislight->full.numLights = 0;
      visset->vislight->full.maxlightdist = VISTEST_LIGHT_DIST;
      visset->vislight->nonsun.numLights = 0;
      visset->vislight->nonsun.maxlightdist = VISTEST_LIGHT_DIST;
    }
  }

  rpoolsetbegin(l_VisTest.visibleBufferArray);

}


void VisTest_draw(VisObjectType_t type)
{
  int i;
  vidClearTexStages(0,i);
  vidStateDefault();

  glDisable(GL_LINE_STIPPLE);

  switch(type) {
  case VIS_OBJECT_CAMERA:
    lxOcTree_draw(l_VisTest.cameraSpace,g_Draws.drawVisFrom,g_Draws.drawVisTo,Draw_boxWireColor);
    break;
  case VIS_OBJECT_LIGHT:
    lxOcTree_draw(l_VisTest.lightSpace,g_Draws.drawVisFrom,g_Draws.drawVisTo,Draw_boxWireColor);
    break;
  case VIS_OBJECT_PROJECTOR:
    lxOcTree_draw(l_VisTest.projectorSpace,g_Draws.drawVisFrom,g_Draws.drawVisTo,Draw_boxWireColor);
    break;
  case VIS_OBJECT_STATIC:
    lxOcTree_draw(l_VisTest.staticSpace,g_Draws.drawVisFrom,g_Draws.drawVisTo,Draw_boxWireColor);
    break;
  case VIS_OBJECT_DYNAMIC:
    lxOcTree_draw(l_VisTest.dynamicSpace,g_Draws.drawVisFrom,g_Draws.drawVisTo,Draw_boxWireColor);
    break;
  default:
    break;
  }
}

int VisTest_getTweak(VisTestTweakable_t type)
{
  LUX_ASSERT(type < VIS_TWEAKS);
  return l_VisTest.tweaks[type];
}

void VisTest_setTweak(VisTestTweakable_t type, int val)
{
  if (val < 0)
    return;

  LUX_ASSERT(type < VIS_TWEAKS);
  l_VisTest.tweaks[type] = val;

  if (type == VIS_TWEAK_MAXDEPTH_SCT)
    l_VisTest.staticCompile = LUX_TRUE;
}


