// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "gl_list3d.h"
#include "../common/3dmath.h"
#include "gl_list2d.h"
#include "gl_drawmesh.h"
#include "gl_draw2d.h"
#include "gl_draw3d.h"
#include "gl_shader.h"
#include "gl_particle.h"
#include "gl_window.h"
#include "../scene/vistest.h"

extern void List3DSet_updateNodes(List3DSet_t *lset);
extern List3DDrawData_t l_l3ddraw;
// LOCALS
static List3DDrawState_t l_dstate;

void DrawSortNode_radix(void *base, uint num);
void Draw_projectors(List3DSet_t *lset);
void Draw_lights(List3DSet_t *lset);
void Draw_axisView(List3DView_t *view);
void List3DSet_drawDebugHelpers();



//////////////////////////////////////////////////////////////////////////
// LIST3D Particle Handling
// ------------------------
#define cmpParticleSysSortKeyInc(a,b) (a->sortkey > b->sortkey)
#define cmpInt2NodeInc(a,b) (a->data2 > b->data2)

static void List3D_addPsysToLayer_recursive(ParticleSys_t *psys)
{
  ParticleSys_t *psysbrowse;
  ParticleSubSys_t *subsys;
  int i;
  if (!(psysbrowse = l_l3ddraw.layerdata[psys->l3dlayer].psysListHead)){
    lxListNode_init(psys);
    l_l3ddraw.layerdata[psys->l3dlayer].psysListHead = psys;
  }
  else{
    lxListNode_insertPrev(psysbrowse,psys);
  }
  psys->indrawlayer = LUX_TRUE;
  subsys = psys->subsys;
  for (i = 0; i < psys->numSubsys; i++,subsys++)
    List3D_addPsysToLayer_recursive(ResData_getParticleSys(subsys->psysRID));
}

static void List3D_prepParticleSys(List3DSet_t *l3dset)
{
  ParticleSys_t *psys;
  Int2Node_t *inbrowse;
  Int2Node_t *inprev;
  List3DLayerData_t *ldata;
  int   wasfirst;
  int n;

  for (n = 0; n < LIST3D_LAYERS; n++)
    l_l3ddraw.layerdata[n].psysListHead = NULL;

  if (!l3dset->psysListHead)
    return;

  if (g_Draws.drawNoPrts)
    return;

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.particle -= getMicros());
  }

  lxListNode_forEach(l3dset->psysListHead,inbrowse)
  StartList:
    LUX_ASSERT(inbrowse);
    LUX_ASSERT(inbrowse->data >= 0);
    psys = ResData_getParticleSys(inbrowse->data);
    if (!psys || (!psys->active && psys->activeEmitterCnt < 1)){
      inprev = inbrowse->prev;

      if (psys)
        psys->l3dset = NULL;

      if (inbrowse == l3dset->psysListHead)
        wasfirst = LUX_TRUE;
      else
        wasfirst = LUX_FALSE;


      lxListNode_remove(l3dset->psysListHead,inbrowse);
      Int2Node_free(inbrowse);
      if (l3dset->psysListHead && (wasfirst || inprev->next != l3dset->psysListHead)){
        inbrowse = inprev->next;
        goto StartList;
      }
      else
        break;
    }

    if (!psys->indrawlayer)
      List3D_addPsysToLayer_recursive(psys);
    ParticleSys_spawnTrails(psys);
    psys->l3dset = l3dset;

    inprev = inbrowse;
  lxListNode_forEachEnd(l3dset->psysListHead,inbrowse);

  // sort drawlayer
  ldata = l_l3ddraw.layerdata;
  for (n = 0; n < LIST3D_LAYERS; n++,ldata++){
    if (ldata->psysListHead){
      lxListNode_sortTapeMerge(ldata->psysListHead,ParticleSys_t,cmpParticleSysSortKeyInc);
    }
  }

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.particle += getMicros());
  }

}

void List3D_drawParticles(List3DView_t *l3dview, int layer,int forceflags,int ignoreflags)
{
  ParticleSys_t *psys;
  ParticleCloud_t *pcloud;
  List3DLayerData_t *layerdata = &l_l3ddraw.layerdata[layer];

  if (g_Draws.drawNoPrts)
    return;


  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.particle -= getMicros());
  }


  lxListNode_forEach(layerdata->psysListHead,psys)
    Draw_ParticleSys(psys,l3dview,forceflags,ignoreflags);
  lxListNode_forEachEnd(layerdata->psysListHead,psys);

  lxListNode_forEach(layerdata->pcloudListHead,pcloud)
    if (pcloud->container.numAParticles && isFlagSet(pcloud->container.visflag,g_CamLight.camera->bitID) )
      Draw_ParticleCloud(pcloud,l3dview,forceflags,ignoreflags);
  lxListNode_forEachEnd(layerdata->pcloudListHead,pcloud);

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.particle += getMicros());
  }

}

static void List3D_updateParticleSys(Int2Node_t *psyslist)
{
  ParticleSys_t *psys;
  Int2Node_t *inbrowse;

  if (!psyslist)
    return;

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.particle -= getMicros());
  }


  lxListNode_forEach(psyslist,inbrowse)
    // also clears indrawlayer
    psys = ResData_getParticleSys(inbrowse->data);
    ParticleSys_update(psys);
    LUX_PROFILING_OP (g_Profiling.perframe.particle.active += psys->container.numAParticles);
  lxListNode_forEachEnd(psyslist,inbrowse);

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.particle += getMicros());
  }



}

static void List3D_clearParticleCloudDraws(Int2Node_t *pcloudlist)
{
  ParticleCloud_t *pcloud;
  Int2Node_t *inbrowse;

  if (!pcloudlist)
    return;

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.particle -= getMicros());
  }


  lxListNode_forEach(pcloudlist,inbrowse)
    pcloud = ResData_getParticleCloud(inbrowse->data);
    LUX_PROFILING_OP (g_Profiling.perframe.particle.active += pcloud->container.numAParticles);
  lxListNode_forEachEnd(pcloudlist,inbrowse);

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.particle += getMicros());
  }
}


static void List3D_prepParticleCloud(List3DSet_t *l3dset)
{
  ParticleCloud_t *pcloud;
  Int2Node_t *inbrowse;
  Int2Node_t *inprev;
  int   wasfirst;
  int n;

  for (n = 0; n < LIST3D_LAYERS; n++)
    l_l3ddraw.layerdata[n].pcloudListHead = NULL;

  if (g_Draws.drawNoPrts)
    return;


  if (l3dset->pcloudListHead){
    lxListNode_sortTapeMerge(l3dset->pcloudListHead,Int2Node_t,cmpInt2NodeInc);
  }
  else
    return;

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.particle -= getMicros());
  }



  lxListNode_forEach(l3dset->pcloudListHead,inbrowse)
    StartList:
    LUX_ASSERT(inbrowse);
    pcloud = ResData_getParticleCloud(inbrowse->data);
    LUX_ASSERT(pcloud);
    if (!pcloud || (!pcloud->container.numAParticles && pcloud->activeGroupCnt < 1)){
      inprev = inbrowse->prev;

      if (pcloud)
        pcloud->l3dset = NULL;

      if (inbrowse == l3dset->pcloudListHead)
        wasfirst = LUX_TRUE;
      else
        wasfirst = LUX_FALSE;

      lxListNode_remove(l3dset->pcloudListHead,inbrowse);
      Int2Node_free(inbrowse);

      if (l3dset->pcloudListHead && (wasfirst || inprev->next != l3dset->pcloudListHead)){
        inbrowse = inprev->next;
        goto StartList;
      }
      else
        break;
    }
    if (g_LuxTimer.time > pcloud->container.drawtime){
      *pcloud->container.activeStaticCur = NULL;
      pcloud->container.activeStaticCur = pcloud->container.stptrs;
      pcloud->container.drawtime = g_LuxTimer.time;
    }
    lxListNode_addLast(l_l3ddraw.layerdata[pcloud->l3dlayer].pcloudListHead,pcloud);

    pcloud->l3dset = l3dset;
  lxListNode_forEachEnd(l3dset->pcloudListHead,inbrowse);


  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.particle += getMicros());
  }

}

#undef cmpInt2NodeInc



//////////////////////////////////////////////////////////////////////////
// LIST3D Drawing
// --------------

// returns true if rejected
static int LUX_FASTCALL list3d_proc_visbox(List3DNode_t *node){
  lxVector3 vec;
  List3DVis_t *l3dvis = node->l3dvis;
  int inside = LUX_TRUE;

  // on batching we set ignore
  if (l3dvis->ignore){
    l3dvis->ignore = LUX_FALSE;
    return LUX_FALSE;
  }

  if (l3dvis->box){
    lxVector3InvTransform(vec,g_CamLight.camera->pos,node->finalMatrix);

    if (vec[0] < l3dvis->omin[0] ||
      vec[0] > l3dvis->omax[0] ||
      vec[1] < l3dvis->omin[1] ||
      vec[1] > l3dvis->omax[1] ||
      vec[2] < l3dvis->omin[2] ||
      vec[2] > l3dvis->omax[2] ||
      (l3dvis->ranged &&
        (vec[0] > l3dvis->imin[0] ||
        vec[0] < l3dvis->imax[0] ||
        vec[1] > l3dvis->imin[1] ||
        vec[1] < l3dvis->imax[1] ||
        vec[2] > l3dvis->imin[2] ||
        vec[2] < l3dvis->imax[2])))
      inside = LUX_FALSE;

  }
  else{
    vec[0] = lxVector3SqDistance(&node->finalMatrix[12],g_CamLight.camera->pos);

    if (vec[0] > l3dvis->omax[0] || (l3dvis->ranged && vec[0] < l3dvis->imax[0]))
      inside = LUX_FALSE;
  }

  // we draw when we are inside and inside flag is set
  // or when outside flag is set and we are outside
  if (l3dvis->inside != inside){
    return LUX_TRUE;
  }
  else
    return LUX_FALSE;

}

// returns always true
static int LUX_FASTCALL list3d_proc_lookat(List3DNode_t *node){
  lxMatrix44SetTranslation(node->finalMatrix,node->finalPos);

  lxMatrix44CopyRot(node->finalMatrix,g_CamLight.camera->finalMatrix);
  if (node->lookatlocal)
    lxMatrix44Multiply1SIMD(node->finalMatrix,node->localMatrix);

  return LUX_FALSE;
}

int List3DView_fillNodeUpdates(List3DNode_t *node,enum32 kickflag, enum32 depthlayer)
{
  switch(node->nodeType) {
    case LUXI_CLASS_L3D_TRAIL:
      Trail_update(node->trail,node->finalMatrix);
      break;
    case LUXI_CLASS_L3D_SHADOWMODEL:
      if (!ShadowModel_visible(node->smdl,node->finalMatrix,g_CamLight.camera))
        return LUX_TRUE;
      ShadowModel_updateMeshes(node->smdl,node,g_CamLight.camera);
      break;
    case LUXI_CLASS_L3D_LEVELMODEL:
      {
        DrawNode_t      *drawnode;
        List3DLayerData_t *layerdata;
        DrawSortNode_t    *sortnode;
        DrawNode_t  **drawnodeptr = LevelModel_getVisible(node->lmdl,node,g_CamLight.camera);

        while(drawnode = *drawnodeptr++) {
          if (drawnode->draw.state.renderflag & (kickflag))
            continue;

          layerdata = &l_l3ddraw.layerdata[drawnode->layerID];
          sortnode = &layerdata->sortNodes[layerdata->numSortNodes++ & LIST3D_LAYER_MAX_NODES_MASK];
          sortnode->value = drawnode->sortkey.value | depthlayer;
          sortnode->pType = drawnode->sortkey.pType;
        }

        rpoolsetbegin(drawnodeptr);
        LUX_PROFILING_OP (g_Profiling.perframe.draw.l3ds++);
        return LUX_TRUE;
      }
      break;
    default:
      break;
  }
  return LUX_FALSE;
}

static void List3DView_fillLayers(const List3DView_t *view, enum32 kickflag)
{
  List3DNode_t *node;
  List3DNode_t **nodeptr;
  DrawNode_t  *drawnode;
  DrawSortNode_t  *sortnode;
  List3DLayerData_t *layerdata;
  int n;
  enum32 depthlayer;
  float range;


  range = g_CamLight.camera->backplane * g_CamLight.camera->nearpercentage;
  range *= range;

  nodeptr = l_l3ddraw.drawBufferArray;
  while (node = *(nodeptr++)){
    if (!(node->viscameras & g_CamLight.camera->bitID) || (node->lookat && list3d_proc_lookat(node))  || (node->l3dvis && list3d_proc_visbox(node)))
      continue;

    if(lxVector3SqDistance(g_CamLight.camera->pos,node->finalMatrix+12) > range)
      depthlayer = DRAW_SORT_FLAG_FAR;
    else
      depthlayer = 0;

    if (List3DView_fillNodeUpdates(node,kickflag,depthlayer))
      continue;

    drawnode = node->drawNodes;
    for (n = 0; n < node->numDrawNodes; n++,drawnode++) {
      if (drawnode->draw.state.renderflag & (kickflag))
        continue;

      layerdata = &l_l3ddraw.layerdata[drawnode->layerID];
      sortnode = &layerdata->sortNodes[layerdata->numSortNodes++ & LIST3D_LAYER_MAX_NODES_MASK];
      sortnode->value = depthlayer | drawnode->sortkey.value ;
      sortnode->pType = drawnode->sortkey.pType;
    }
    LUX_PROFILING_OP (g_Profiling.perframe.draw.l3ds++);
  }

  // cap layers
  layerdata = l_l3ddraw.layerdata;
  for (n = 0; n < LIST3D_LAYERS; n++,layerdata++){
    LUX_PROFILING_OP_MAX(g_Profiling.global.nodes.maxlayerdrawnodes,layerdata->numSortNodes);
    // cap
    layerdata->numSortNodes = layerdata->numSortNodes & LIST3D_LAYER_MAX_NODES_MASK;
  }
}

  // changes sortkey to distance to camera
void List3DLayerData_cameraSortKey(List3DLayerData_t  *layerdata,int btf)
{
  DrawSortNode_t *sortnode;
  for(sortnode = layerdata->sortNodes; sortnode < layerdata->sortNodes+layerdata->numSortNodes; sortnode++){
    DrawNode_t *dnode  = (DrawNode_t*)sortnode->pType;
    if (dnode->matrix){
      sortnode->value = (uint)LUX_MIN((float)BIT_ID_FULL32,lxVector3SqDistance(g_CamLight.camera->pos,dnode->matrix+12));
      sortnode->value = (btf) ? (uint)((int)BIT_ID_FULL32 - (int)sortnode->value) : sortnode->value;
    }
  }
}


static LUX_INLINE void List3DDrawState_runCommands(List3DDrawState_t *lstate)
{
  RenderCmdPtr_t pcmd;
  List3DView_t* view = lstate->view;
  RenderCmd_t **pcmds = view->commands;

  lstate->ignoreflag = 0;
  lstate->ignoreflags = 0;
  lstate->forceflag = 0;
  lstate->forceflags = 0;
  lstate->ignorelights = LUX_FALSE;
  lstate->ignoreprojs = LUX_FALSE;

  while(pcmd.ptr = *(pcmds++)){
    if (pcmd.cmd->runflag & view->rcmdflag)
      pcmd.cmd->fnrun(pcmd.ptr,view,lstate);
  }

}

void List3D_drawView(List3DView_t *view,Light_t *sun, List3DSet_t *lset)
{
  int n;
  enum32 kickflag = (RENDER_NODRAW | RENDER_NEVER) | ((view->depthonly) ? RENDER_NODEPTHMASK | RENDER_BLEND : 0);

  // all commands disabled
  if (!view->rcmdflag)
    return;

  vidCheckError();
  if (l_dstate.orthoprojection){
    l_dstate.orthoprojection = LUX_FALSE;
    vidOrthoOff();
    //glPopAttrib();
  }

  if (view->viewport.bounds.fullwindow){
    LUX_ARRAY4SET(view->viewport.bounds.sizes,0,0,g_VID.drawbufferWidth,g_VID.drawbufferHeight);
  }
  vidViewport(lxVector4Unpack(view->viewport.bounds.sizes));
  vidDepthRange(view->viewport.depth[0],view->viewport.depth[1]);
  vidCheckError();

  // own defaultcam, and diffierent from current
  if (view->camera && view->camera != g_CamLight.camera){
    g_CamLight.camera = view->camera;
    Camera_setGL(view->camera);
  }//
  else if (!view->camera && g_CamLight.camera != l_dstate.defaultcam){
    g_CamLight.camera = l_dstate.defaultcam;
    Camera_setGL(l_dstate.defaultcam);
  }
  else{
    vidLoadCameraGL();
  }
  if (g_CamLight.camera == l_dstate.defaultcam)
    g_CamLight.camera->bitID = view->camvisflag;

  RenderBackground_setGL(&view->background);
  g_Background = &view->background;


  if (view->depthonly)
    glShadeModel(GL_FLAT);

  if (view->usepolyoffset){
    glPolygonOffset(view->polyoffset[0],view->polyoffset[1]);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glEnable(GL_POLYGON_OFFSET_POINT);
  }


  vidLightClear();

  if (sun){
    g_CamLight.sunlight = sun;
    Light_setGL(sun,0);
  }else{
    Light_setGL(g_CamLight.sunlight,0);
  }


  // clear layers
  if(l_dstate.firstclear || view->layerfill){
    for (n = 0; n < LIST3D_LAYERS; n++)
      l_l3ddraw.layerdata[n].numSortNodes = 0;
    l_dstate.firstclear = LUX_FALSE;
  }

  // fill layers
  if (view->layerfill && *l_l3ddraw.drawBufferArray){
    List3DView_fillLayers(view,kickflag);
  }

  if (l_dstate.useshaders){
    vidResetShaders();
    l_dstate.useshaders = LUX_FALSE;
  }

  l_dstate.isdefaultcam = l_dstate.defaultcam == g_CamLight.camera;
  l_dstate.view = view;
  l_dstate.lset = lset;
  l_dstate.kickflag = kickflag;
  l_dstate.l3ddraw = &l_l3ddraw;

  List3DDrawState_runCommands(&l_dstate);

  if (view->depthonly)
    glShadeModel(GL_SMOOTH);

  if (view->usepolyoffset){
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glDisable(GL_POLYGON_OFFSET_POINT);
  }

  vidCheckError();
}

void List3D_drawDeinitState()
{
  if (l_dstate.orthoprojection){
    vidOrthoOff();
    //glPopAttrib();
  }

  if (l_dstate.fboTargets[FBO_TARGET_DRAW]){
    glBindFramebufferEXT(FBOTarget_getBind(FBO_TARGET_DRAW),0);
    glDrawBuffer(GL_BACK);
    g_VID.drawbufferWidth = g_VID.windowWidth;
    g_VID.drawbufferHeight = g_VID.windowHeight;
    vidViewport(0,0,g_VID.windowWidth,g_VID.windowHeight);
  }
  if (l_dstate.fboTargets[FBO_TARGET_READ]){
    glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT,0);
    glReadBuffer(GL_BACK);
  }

  g_CamLight.camera = l_dstate.defaultcam;
  l_dstate.defaultcam->bitID = l_dstate.defaultcambitid;
  g_CamLight.sunlight = l_dstate.defaultsun;
  vidResetStencil();

  vidResetTexture();
  vidTexBlendDefault(VID_TEXBLEND_MOD_MOD);
  vidStateDefault();
  Draw_useBaseShaders(LUX_FALSE);
  vidResetShaders();
  vidWorldScale(NULL);
  vidWorldMatrixIdentity();
}

void List3D_drawInitState()
{
  vidWorldScale(NULL);
  vidWorldMatrixIdentity();

  l_dstate.defaultcam = g_CamLight.camera;
  l_dstate.defaultsun = g_CamLight.sunlight;

  l_dstate.drawdebug =  g_Draws.drawBBOXwire ||
    g_Draws.drawBBOXsolid ||
    g_Draws.drawAxis ||
    g_Draws.drawBones ||
    g_Draws.drawNormals ||
    g_Draws.drawPath;

  l_dstate.defaultcambitid = g_CamLight.camera->bitID;

  l_dstate.fboTargets[FBO_TARGET_DRAW] = NULL;
  l_dstate.fboTargets[FBO_TARGET_READ] = NULL;
  l_dstate.useshaders = LUX_FALSE;
  l_dstate.orthoprojection = LUX_FALSE;
  l_dstate.firstclear = LUX_TRUE;

  Camera_setGL(g_CamLight.camera);
  Light_setGL(g_CamLight.sunlight,0);
}

void List3D_draw()
{
  int l;

  List3DSet_t   *lset = g_List3D.drawSets;
  List3DView_t  *view;

  List3D_drawInitState();

  vidCheckError();
  for (l = 0; l < LIST3D_SETS; l++,lset++){
    if (lset->disabled){
      continue;
    }

    // method uses bufferstack
    List3DSet_updateNodes(lset);
    List3D_prepParticleSys(lset);
    List3D_prepParticleCloud(lset);

    l_dstate.firstclear = LUX_TRUE;

    lxListNode_forEach(lset->viewListHead,view)
      List3D_drawView(view,lset->sun,lset);
    lxListNode_forEachEnd(lset->viewListHead,view);


    List3D_updateParticleSys(lset->psysListHead);
    List3D_clearParticleCloudDraws(lset->pcloudListHead);

    // reset buffer stack
    rpoolsetbegin(l_l3ddraw.drawBufferArray);
  }

  List3D_drawDeinitState();

  VisTest_resetVisible();

  // reset rpool
  rpoolclear();

  vidCheckError();
}

//////////////////////////////////////////////////////////////////////////
// List3DView Direct Draw

void List3DView_drawDirect(List3DView_t *view, Light_t *sun)
{
  static List3DNode_t *dummy = NULL;
  int n;
  if (view->numCommands){
    List3D_drawInitState();


    // some overrides to allow non lset related drawing
    // clear particle layers
    if(l_dstate.firstclear || view->layerfill){
      for (n = 0; n < LIST3D_LAYERS; n++){
        l_l3ddraw.layerdata[n].pcloudListHead = NULL;
        l_l3ddraw.layerdata[n].psysListHead = NULL;
      }
    }
    l_dstate.drawdebug = LUX_FALSE;
    l_l3ddraw.drawBufferArray = &dummy;

    List3D_drawView(view,sun,NULL);

    List3D_drawDeinitState();
  }
}


//////////////////////////////////////////////////////////////////////////
// List3D FBO Check



static char* fboCheckAll(FBObject_t *fboTargets[FBO_TARGETS])
{
  static char buffer[64];

  FBOTarget_t fbotarget;
  char *errorstr = NULL;
  for (fbotarget = 0; fbotarget < FBO_TARGETS; fbotarget++){
    if (fboTargets[fbotarget]){
      const char* estr = FBOTarget_checkCompleteness(fbotarget);
      if (estr){
        sprintf(buffer,estr,FBO_TARGET_DRAW == fbotarget ? "DRAW" : "READ");
        errorstr = buffer;
      }
    }
  }
  return errorstr;
}

static int  fboErrorString(int set, int view, int cmd, char *cname, const char *errorin, char *errorout, int erroroutlen){
  if (errorin){
    char *c = errorout;
    while (*c)c++;
    if (c+100 > errorout+erroroutlen){
      c[-3] = c[-2] = c[-1] = '.';
    }
    else{
      *c = '|';
      c++;
      sprintf(c,"l3dset:%d,l3dview:%d,rcmd:%d %s,%s",set,view,cmd,cname,errorin);
    }

    return LUX_TRUE;
  }
  return LUX_FALSE;
}

static void fboResetTargets(FBObject_t *fboTargets[FBO_TARGETS])
{
  FBOTarget_t fbotarget;
  FBObject_t  *fbo;
  for (fbotarget = 0; fbotarget < FBO_TARGETS; fbotarget++)
    if(fbo=fboTargets[fbotarget]){
      FBObject_reset(fbo);
      glBindFramebufferEXT(FBOTarget_getBind(fbotarget),0);
    }

}

int List3D_fbotestView(List3DView_t *view,char *outerror, int outerrorlen,
             int n, int v,
             int *hadwarnings,int *runcheck,
             FBObject_t  **fboTargets, FBObject_t **fboinout)
{
  RenderCmdPtr_t pcmd;
  RenderCmd_t **pcmds;
  FBRenderBuffer_t *rbuffer;
  FBTexAssign_t *texassign;
  FBOTarget_t fbotarget;
  FBObject_t *fbo = *fboinout;

  Texture_t *tex;
  int c;
  int a;

  enum32 glerror;


  if (view->viewport.bounds.fullwindow){
    LUX_ARRAY4SET(view->viewport.bounds.sizes,0,0,g_VID.drawbufferWidth,g_VID.drawbufferHeight);
  }
  vidViewport(lxVector4Unpack(view->viewport.bounds.sizes));


  pcmds = view->commands;
  c = 0;
  while(pcmd.ptr = *(pcmds++)){
    c++;
    fbotarget = FBO_TARGETS;
    if (pcmd.cmd->runflag & view->rcmdflag)
      switch(*pcmd.type){
        case LUXI_CLASS_RCMD_FBO_OFF:
          FBObject_reset(fboTargets[pcmd.flag->fbobind.fbotarget]);
        case LUXI_CLASS_RCMD_FBO_DRAWBUFFERS:
          fbotarget = FBO_TARGET_DRAW;
        case LUXI_CLASS_RCMD_FBO_READBUFFER:
          if (fbotarget == FBO_TARGETS)
            fbotarget = FBO_TARGET_READ;
        case LUXI_CLASS_RCMD_FBO_RBASSIGN:
          if (fbotarget == FBO_TARGETS)
            fbotarget = pcmd.flag->fborb.fbotarget;
        case LUXI_CLASS_RCMD_FBO_TEXASSIGN:
          if (fbotarget == FBO_TARGETS)
            fbotarget = pcmd.fbotex->fbotarget;

          if (!fboTargets[fbotarget]){
            *hadwarnings = fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),"no fbo bound",outerror,outerrorlen);
            return LUX_TRUE;
          }
        case LUXI_CLASS_RCMD_FBO_BIND:
          if (fbotarget == FBO_TARGETS)
            fbotarget = pcmd.flag->fbobind.fbotarget;

          // some prechecks that would cause trouble on fbo bind
          if (*pcmd.type == LUXI_CLASS_RCMD_FBO_BIND){
            if(!(fbo=pcmd.flag->fbobind.fbo))
            {
              *hadwarnings = fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),"no fbo bound",outerror,outerrorlen);
              return LUX_TRUE;
            }
            if (!pcmd.flag->fbobind.viewportchange && (
              g_VID.state.viewport.bounds.x+g_VID.state.viewport.bounds.width > fbo->width ||
              g_VID.state.viewport.bounds.y+g_VID.state.viewport.bounds.height > fbo->height))
            {
              *hadwarnings = fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),"viewport out of fbo bounds",outerror,outerrorlen);
              return LUX_TRUE;
            }
          }

          RenderCmdPtr_runFBO(pcmd,fboTargets,view->viewport.bounds.sizes);

          fbo = fboTargets[fbotarget];

          while ( (glerror = glGetError()) != GL_NO_ERROR) {
            *hadwarnings = fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),gluErrorString(glerror),outerror,outerrorlen);
            return LUX_TRUE;
          }
          // runsize check
          if (*pcmd.type == LUXI_CLASS_RCMD_FBO_BIND){
            if (!fbo->height || !fbo->width){
              *hadwarnings = fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),"fbosize not set",outerror,outerrorlen);
              return LUX_TRUE;
            }
          }
          else if (*pcmd.type == LUXI_CLASS_RCMD_FBO_RBASSIGN){
            for (a=0; a < FBO_ASSIGNS; a++){
              if ((rbuffer=pcmd.flag->fborb.fborbassigns[a]) && (rbuffer->width != fbo->width || rbuffer->height != fbo->height)){
                *hadwarnings = fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),"size mismatch of rb assign and fbo",outerror,outerrorlen);
                return LUX_TRUE;
              }
            }
            if (fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),fboCheckAll(fboTargets),outerror,outerrorlen)){
              *hadwarnings = 1;
              return LUX_TRUE;
            }
          }
          else if (*pcmd.type == LUXI_CLASS_RCMD_FBO_TEXASSIGN){
            texassign = pcmd.fbotex->fbotexassigns;
            for (a=0; a < FBO_ASSIGNS; a++,texassign++){
              if (texassign->texRID >= 0){
                if (!(tex=ResData_getTexture(texassign->texRID))){
                  *hadwarnings = fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),"illegal texture found",outerror,outerrorlen);
                  return LUX_TRUE;
                }
                if (tex->sarr3d.sz.width != fbo->width || tex->sarr3d.sz.height != fbo->height){
                  *hadwarnings = fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),"size mismatch of tex assign and fbo",outerror,outerrorlen);
                  return LUX_TRUE;
                }
              }
            }
            if (fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),fboCheckAll(fboTargets),outerror,outerrorlen)){
              *hadwarnings = 1;
              return LUX_TRUE;
            }
          }
          *runcheck = LUX_TRUE;
          break;
        case LUXI_CLASS_RCMD_CLEAR:
        case LUXI_CLASS_RCMD_DRAWL2D:
        case LUXI_CLASS_RCMD_DRAWL3D:
        case LUXI_CLASS_RCMD_DRAWMESH:
        case LUXI_CLASS_RCMD_DRAWBACKGROUND:
        case LUXI_CLASS_RCMD_DRAWDEBUG:
        case LUXI_CLASS_RCMD_DRAWLAYER:
          if (*runcheck){
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            *runcheck = LUX_FALSE;
            if (fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),fboCheckAll(fboTargets),outerror,outerrorlen)){
              *hadwarnings = 1;
              return LUX_TRUE;
            }
          }
          break;
        case LUXI_CLASS_RCMD_BUFFER_BLIT:
          // hard to test, as back buffer blits would ruin our image
          break;
        case LUXI_CLASS_RCMD_GENMIPMAPS:
          if (pcmd.flag->texRID < 0){
            *hadwarnings = -1;
            fboErrorString(n,v,c,RenderCmd_toString(*pcmd.type),"genmipmap has no texture set",outerror,outerrorlen);
          }
          break;
        default:
          break;
    }
  }

  for (a = 0; a < FBO_TARGETS; a++){
    if (fboTargets[a]){
      *hadwarnings = -1;
      fboErrorString(n,v,c,"end","left fbo bound for next view",outerror,outerrorlen);
    }
  }

  *fboinout = fbo;

  return LUX_FALSE;
}

// 0 no error, 1 errors, -1 warnings
int List3D_fbotest(char *outerror, int outerrorlen, List3DView_t *singleview)
{

  List3DSet_t *lset;
  List3DView_t *view;
  FBObject_t  *fbo = NULL;
  FBObject_t  *fboTargets[FBO_TARGETS] = {NULL,NULL};

  int n;
  int v;
  int runcheck = LUX_FALSE;
  int hadwarnings = 0;

  vidClearTexStages(0,n);

  lset = g_List3D.drawSets;

  for (n = 0; n < LIST3D_SETS; n++,lset++){
    if (!singleview && lset->disabled){
      continue;
    }
    v = 0;
    lxListNode_forEach(lset->viewListHead,view)
      if (singleview){
        view = singleview;
        n = -1;
      }
      if (List3D_fbotestView(view,outerror,outerrorlen,
        n, v,&hadwarnings,&runcheck,fboTargets,&fbo)){

        goto FinishCheck;
      }
      v++;
      if (singleview) goto FinishCheck;
    lxListNode_forEachEnd(lset->viewListHead,view)
  }

FinishCheck:
  // in all cases unbind
  fboResetTargets(fboTargets);

  g_VID.drawbufferWidth = g_VID.windowWidth;
  g_VID.drawbufferHeight = g_VID.windowHeight;

  glDrawBuffer(GL_BACK);
  glReadBuffer(GL_BACK);
  vidViewport(0,0,g_VID.windowWidth,g_VID.windowHeight);

  vidCheckError();
  return hadwarnings;
}


//////////////////////////////////////////////////////////////////////////
// Debug

void List3DSet_drawDebugHelpers()
{
  List3DNode_t **nodeptr = l_l3ddraw.drawBufferArray;
  List3DNode_t *node;

  VIDRenderFlag_setGL(0);

  while (node = *(nodeptr++)){
    if ( !(node->viscameras & g_CamLight.camera->bitID) && node->l3dvis && list3d_proc_visbox(node))
      continue;

    vidLoadCameraGL();
    glMultMatrixf(node->finalMatrix);
    glScalef(lxVector3Unpack(node->renderscale));
    if (g_Draws.drawAxis)
      Draw_axis();
    if (node->nodeType == LUXI_CLASS_L3D_MODEL){
      if (g_Draws.drawNormals){
        Draw_Model_normals(ResData_getModel(node->minst->modelRID));
      }
      if (g_Draws.drawBBOXsolid || g_Draws.drawBBOXwire ){
        Draw_Model_bbox(ResData_getModel(node->minst->modelRID));
      }
      if (node->minst->boneupd && node->minst->boneupd->inWorld)
        vidLoadCameraGL();
      ResData_getModel(node->minst->modelRID)->bonesys.absMatrices = node->minst->boneupd ?  node->minst->boneupd->bonesAbs : ResData_getModel(node->minst->modelRID)->bonesys.refAbsMatrices;
      if (g_Draws.drawBones){
        Draw_Model_bones(ResData_getModel(node->minst->modelRID),node->minst->boneupd && node->minst->boneupd->inWorld ? NULL : node->finalMatrix, node->renderscale );
      }
    }

  }
}

void Draw_axisView(List3DView_t *view){
  lxVector3 pos;
  float scale = 0.01;
  int viewport[4];

  if (view->viewport.bounds.fullwindow)
    LUX_ARRAY4SET(viewport,0,0,g_VID.windowWidth,g_VID.windowHeight);
  else
    LUX_ARRAY4COPY(viewport,view->viewport.bounds.sizes);

  // camera orientation
  lxVector3Set(pos,viewport[0]+viewport[2]/8,(viewport[1]+viewport[3]/8),0.2);
  Camera_screenToWorld(g_CamLight.camera,pos,pos,viewport);
  vidLoadCameraGL();
  glPushMatrix();
  glTranslatef(lxVector3Unpack(pos));
  scale *= ((g_CamLight.camera->backplane-g_CamLight.camera->frontplane)*0.2)*0.005;
  scale *= g_CamLight.camera->frontplane;
  scale *= g_CamLight.camera->fov/90;
  scale *= 1024/g_CamLight.camera->backplane;
  glScalef(scale,scale,scale);
  Draw_axis();
  glPopMatrix();
}

void Draw_lights(List3DSet_t *lset)
{
  Light_t   *light;
  int i;
  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);
  VIDRenderFlag_setGL(0);
  glDisable(GL_LINE_STIPPLE);
  vidClearTexStages(0,i);

  if (g_CamLight.sunlight){
    vidLoadCameraGL();
    glPushMatrix();
    glTranslatef(g_CamLight.sunlight->pos[0],g_CamLight.sunlight->pos[1],g_CamLight.sunlight->pos[2]);
    glColor3f(1,1,1);
    Draw_sphere(2,8,8);
    glPopMatrix();
  }
  vidLoadCameraGL();
  lxListNode_forEach(lset->fxlightListHead,light)
    glPushMatrix();
    glTranslatef(light->pos[0],light->pos[1],light->pos[2]);
    glColor3f(1,1,0);
    Draw_sphere(2,8,8);
    vidBlend(VID_ADD,LUX_FALSE);
    vidBlending(LUX_TRUE);
    glColor3f(0.1,0.1,0);
    Draw_sphere(light->range,32,32);
    glPopMatrix();
  lxListNode_forEachEnd(lset->fxlightListHead,light)
}


void Draw_projectors(List3DSet_t *lset)
{
  Projector_t *proj;
  FrustumObject_t *fobj;
  float *from;
  float *to;
  int i;
  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);
  VIDRenderFlag_setGL(0);
  glDisable(GL_LINE_STIPPLE);
  vidClearTexStages(0,i);

  vidLoadCameraGL();
  lxListNode_forEach(lset->projectorListHead,proj)
    glPushMatrix();

    glColor3f(0,1,1);
    glBegin(GL_LINE_LOOP);
    to = proj->frustumBoxCorners[0];
    glVertex3f(to[0],to[1],to[2]);
    to = proj->frustumBoxCorners[1];
    glVertex3f(to[0],to[1],to[2]);
    to = proj->frustumBoxCorners[2];
    glVertex3f(to[0],to[1],to[2]);
    to = proj->frustumBoxCorners[3];
    glVertex3f(to[0],to[1],to[2]);
    glEnd();
    glBegin(GL_LINE_LOOP);
    to = proj->frustumBoxCorners[4];
    glVertex3f(to[0],to[1],to[2]);
    to = proj->frustumBoxCorners[5];
    glVertex3f(to[0],to[1],to[2]);
    to = proj->frustumBoxCorners[6];
    glVertex3f(to[0],to[1],to[2]);
    to = proj->frustumBoxCorners[7];
    glVertex3f(to[0],to[1],to[2]);
    glEnd();
    glBegin(GL_LINES);
    from = proj->frustumBoxCorners[0];
    to = proj->frustumBoxCorners[4];
    glVertex3f(from[0],from[1],from[2]);
    glVertex3f(to[0],to[1],to[2]);
    from = proj->frustumBoxCorners[1];
    to = proj->frustumBoxCorners[5];
    glVertex3f(from[0],from[1],from[2]);
    glVertex3f(to[0],to[1],to[2]);
    from = proj->frustumBoxCorners[2];
    to = proj->frustumBoxCorners[6];
    glVertex3f(from[0],from[1],from[2]);
    glVertex3f(to[0],to[1],to[2]);
    from = proj->frustumBoxCorners[3];
    to = proj->frustumBoxCorners[7];
    glVertex3f(from[0],from[1],from[2]);
    glVertex3f(to[0],to[1],to[2]);
    glEnd();

    glTranslatef(proj->pos[0],proj->pos[1],proj->pos[2]);
    Draw_sphere(2,8,8);
    glPopMatrix();
  lxListNode_forEachEnd(lset->projectorListHead,proj);

  lxListNode_forEach(g_CamLight.camera->frustcontListHead,fobj)
    glPushMatrix();

    glColor3f(0.2f,1.0f,1.0f);
    glBegin(GL_LINE_LOOP);
    to = fobj->frustumBoxCorners[0];
    glVertex3f(to[0],to[1],to[2]);
    to = fobj->frustumBoxCorners[1];
    glVertex3f(to[0],to[1],to[2]);
    to = fobj->frustumBoxCorners[2];
    glVertex3f(to[0],to[1],to[2]);
    to = fobj->frustumBoxCorners[3];
    glVertex3f(to[0],to[1],to[2]);
    glEnd();
    glBegin(GL_LINE_LOOP);
    to = fobj->frustumBoxCorners[4];
    glVertex3f(to[0],to[1],to[2]);
    to = fobj->frustumBoxCorners[5];
    glVertex3f(to[0],to[1],to[2]);
    to = fobj->frustumBoxCorners[6];
    glVertex3f(to[0],to[1],to[2]);
    to = fobj->frustumBoxCorners[7];
    glVertex3f(to[0],to[1],to[2]);
    glEnd();
    glBegin(GL_LINES);
    from = fobj->frustumBoxCorners[0];
    to = fobj->frustumBoxCorners[4];
    glVertex3f(from[0],from[1],from[2]);
    glVertex3f(to[0],to[1],to[2]);
    from = fobj->frustumBoxCorners[1];
    to = fobj->frustumBoxCorners[5];
    glVertex3f(from[0],from[1],from[2]);
    glVertex3f(to[0],to[1],to[2]);
    from = fobj->frustumBoxCorners[2];
    to = fobj->frustumBoxCorners[6];
    glVertex3f(from[0],from[1],from[2]);
    glVertex3f(to[0],to[1],to[2]);
    from = fobj->frustumBoxCorners[3];
    to = fobj->frustumBoxCorners[7];
    glVertex3f(from[0],from[1],from[2]);
    glVertex3f(to[0],to[1],to[2]);
    glEnd();

    glPopMatrix();
  lxListNode_forEachEnd(g_CamLight.camera->frustcontListHead,fobj);
}


//////////////////////////////////////////////////////////////////////////
// Sorting

static LUX_INLINE void DrawSortNode_radixSingle( uint byte, uint size, DrawSortNode_t *source, DrawSortNode_t *dest )
{
  uint           count[ 256 ] = { 0 };
  uint           index[ 256 ];
  uint           i;
  unsigned char *sortKey = NULL;
  unsigned char *end = NULL;

  sortKey = ( (unsigned char *)&source[ 0 ].value ) + byte;
  end = sortKey + ( size * sizeof( DrawSortNode_t ) );
  for( ; sortKey < end; sortKey += sizeof( DrawSortNode_t ) )
    ++count[ *sortKey ];

  index[ 0 ] = 0;

  for( i = 1; i < 256; ++i )
    index[ i ] = index[ i - 1 ] + count[ i - 1 ];

  sortKey = ( (unsigned char *)&source[ 0 ].value ) + byte;
  for( i = 0; i < size; ++i, sortKey += sizeof( DrawSortNode_t ) )
    dest[ index[ *sortKey ]++ ] = source[ i ];
}

void DrawSortNode_radix( void *source, uint size )
{
  DrawSortNode_t *scratch = rpoolmalloc(sizeof(DrawSortNode_t)*LIST3D_LAYER_MAX_NODES);
  LUX_PROFILING_OP_MAX(g_Profiling.perframe.scratch.renderpoolmax,rpoolgetinuse());

#ifdef LUX_ENDIAN_LITTLE
  DrawSortNode_radixSingle( 0, size, source, scratch );
  DrawSortNode_radixSingle( 1, size, scratch, source );
  DrawSortNode_radixSingle( 2, size, source, scratch );
  DrawSortNode_radixSingle( 3, size, scratch, source );
#else
  DrawSortNode_radixSingle( 3, size, source, scratch );
  DrawSortNode_radixSingle( 2, size, scratch, source );
  DrawSortNode_radixSingle( 1, size, source, scratch );
  DrawSortNode_radixSingle( 0, size, scratch, source );
#endif

  rpoolsetbegin(scratch);
}

