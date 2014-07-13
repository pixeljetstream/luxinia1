// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "gl_render.h"
#include "../common/vid.h"
#include "../common/3dmath.h"
#include "gl_camlight.h"
#include "gl_window.h"
#include "gl_drawmesh.h"
#include "gl_draw2d.h"
#include "gl_draw3d.h"
#include "gl_print.h"
#include "gl_list2d.h"
#include "gl_list3d.h"
#include "gl_particle.h"
#include "../scene/vistest.h"

#include "../common/console.h"
#include "../controls/controls.h"
#include "../main/main.h"
#include "../common/perfgraph.h"

// GLOBALS
RenderHelpers_t   g_Draws;
RenderBackground_t  *g_Background;


// LOCALS
static RenderData_t l_rdata;
static enum32 l_FBOTargets[FBO_TARGETS];

// FUNCTIONS
void Render(void)
{
  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.render = -getMicros());
  }

  vidWorldScale(NULL);
  vidWorldMatrixIdentity();

  lxObjRefSys_pushNoDelete(GLOBALREFSYS);
  LuaCore_callTableFunction("luxinia","framebegin");

  //g_VID.boneSkip = (g_VID.frameCnt%g_VID.boneFrameSkip < g_VID.boneFrameSkip-1) ? FALSE : TRUE;

  if(rpoolresized()){
    List3D_updateMemPoolRelated();
  }

  rpoolsetbegin(g_List3D.visibleBufferStart);

  Draw_initFrame();


  //g_VID.state.renderflag = VIDRenderFlag_getGL();
  g_VID.state.renderflagforce = 0;
  g_VID.state.renderflagignore = 0;

  vidNoDepthTest(LUX_FALSE);


  if (g_Draws.drawWire)
    g_VID.state.renderflagforce |= RENDER_WIRE;
  if (g_VID.drawsetup.purehwvertex)
    g_VID.state.renderflagignore |= RENDER_LIT;

  List3D_updateProps();

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.vistest = -getMicros());
  }

  VisTest_run();

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.vistest += getMicros());
  }

  //glFinish();
  // or glGetTexImage to a dummy

  LuaCore_callTableFunction("luxinia","framepostvistest");

  vidResetTexture();
  vidTexBlendDefault(VID_TEXBLEND_MOD_MOD);
  vidStateDefault();

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.l3d = -getMicros());
  }

  List3D_draw();

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.l3d += getMicros());
  }

  if(g_Draws.drawVisSpace){

    vidResetTexture();
    vidTexBlendDefault(VID_TEXBLEND_MOD_MOD);
    vidStateDefault();

    vidLoadCameraGL();
    VisTest_draw(g_Draws.drawVisSpace);
  }

  LuaCore_callTableFunction("luxinia","framepostl3d");

  vidViewport(0,0,g_VID.windowWidth,g_VID.windowHeight);

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.l2d = -getMicros());
  }

  if (!g_Draws.drawNoGUI)
    List2D_draw(NULL,VID_REF_WIDTH,VID_REF_HEIGHT,LUX_FALSE,NULL,NULL);

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.l2d += getMicros());
  }

  LuaCore_callTableFunction("luxinia","frameend");

  lxObjRefSys_popNoDelete(GLOBALREFSYS);


  /*
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  */

  Console_draw();

  //glFlush();

  g_VID.frameCnt++;

  vidWorldScale(NULL);
  vidWorldMatrixIdentity();

  //END_FAST_MATH(fastmath);

  if (g_Draws.drawPerfGraph){
    if (g_Draws.statsFinish)
      glFinish();

    LUX_PROFILING_OP (g_Profiling.perframe.timers.render  += getMicros());
    LUX_PROFILING_OP (g_Profiling.perframe.timers.render  -= g_Profiling.perframe.timers.vistest);
    LUX_PROFILING_OP (g_Profiling.perframe.timers.l3d   -= g_Profiling.perframe.timers.particle);
    PGraph_addSample(PGRAPH_PARTICLE, (float)g_Profiling.perframe.timers.particle   *0.001f);
    PGraph_addSample(PGRAPH_L3D,    (float)g_Profiling.perframe.timers.l3d      *0.001f);
    PGraph_addSample(PGRAPH_L2D,    (float)g_Profiling.perframe.timers.l2d      *0.001f);
    PGraph_addSample(PGRAPH_RENDER,   (float)g_Profiling.perframe.timers.render   *0.001f);
    PGraph_addSample(PGRAPH_VISTEST,  (float)g_Profiling.perframe.timers.vistest    *0.001f);
  }
#if _DEBUG
  VIDState_testGL();
#endif
}



///////////////////////////////////////////////////////////////////////////////
// RENDER


void Render_init(int numL3DSets, int numL3DLayers, int numL3DLayerNodes, int numL3DTotalProjectors)
{
  Reference_registerType(LUXI_CLASS_RENDERBUFFER,"renderbuffer",RFBRenderBuffer_free,NULL);
  Reference_registerType(LUXI_CLASS_RENDERFBO,"renderfbo",RFBObject_free,NULL);
  Reference_registerType(LUXI_CLASS_VIDBUFFER,"vidbuffer",RHWBufferObject_free,NULL);
  Reference_registerType(LUXI_CLASS_RENDERMESH,"rendermesh",RRenderMesh_free,NULL);

  Reference_registerType(LUXI_CLASS_RCMD_CLEAR,RenderCmd_toString(LUXI_CLASS_RCMD_CLEAR),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_STENCIL,RenderCmd_toString(LUXI_CLASS_RCMD_STENCIL),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_DEPTH,RenderCmd_toString(LUXI_CLASS_RCMD_DEPTH),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_LOGICOP,RenderCmd_toString(LUXI_CLASS_RCMD_LOGICOP),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_FORCEFLAG,RenderCmd_toString(LUXI_CLASS_RCMD_FORCEFLAG),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_IGNOREFLAG,RenderCmd_toString(LUXI_CLASS_RCMD_IGNOREFLAG),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_BASESHADERS,RenderCmd_toString(LUXI_CLASS_RCMD_BASESHADERS),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_BASESHADERS_OFF,RenderCmd_toString(LUXI_CLASS_RCMD_BASESHADERS_OFF),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_IGNOREEXTENDED,RenderCmd_toString(LUXI_CLASS_RCMD_IGNOREEXTENDED),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_COPYTEX,RenderCmd_toString(LUXI_CLASS_RCMD_COPYTEX),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_DRAWMESH,RenderCmd_toString(LUXI_CLASS_RCMD_DRAWMESH),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_DRAWBACKGROUND,RenderCmd_toString(LUXI_CLASS_RCMD_DRAWBACKGROUND),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_DRAWDEBUG,RenderCmd_toString(LUXI_CLASS_RCMD_DRAWDEBUG),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_DRAWLAYER,RenderCmd_toString(LUXI_CLASS_RCMD_DRAWLAYER),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_DRAWPARTICLES,RenderCmd_toString(LUXI_CLASS_RCMD_DRAWPARTICLES),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_DRAWL2D,RenderCmd_toString(LUXI_CLASS_RCMD_DRAWL2D),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_DRAWL3D,RenderCmd_toString(LUXI_CLASS_RCMD_DRAWL3D),RRenderCmd_free,NULL);

  Reference_registerType(LUXI_CLASS_RCMD_GENMIPMAPS,RenderCmd_toString(LUXI_CLASS_RCMD_GENMIPMAPS),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_READPIXELS,RenderCmd_toString(LUXI_CLASS_RCMD_READPIXELS),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_R2VB,RenderCmd_toString(LUXI_CLASS_RCMD_R2VB),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_UPDATETEX,RenderCmd_toString(LUXI_CLASS_RCMD_UPDATETEX),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_FNCALL,RenderCmd_toString(LUXI_CLASS_RCMD_FNCALL),RRenderCmd_free,NULL);

  Reference_registerType(LUXI_CLASS_RCMD_FBO_BIND,RenderCmd_toString(LUXI_CLASS_RCMD_FBO_BIND),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_FBO_OFF,RenderCmd_toString(LUXI_CLASS_RCMD_FBO_OFF),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_FBO_TEXASSIGN,RenderCmd_toString(LUXI_CLASS_RCMD_FBO_TEXASSIGN),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_FBO_RBASSIGN,RenderCmd_toString(LUXI_CLASS_RCMD_FBO_RBASSIGN),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_FBO_DRAWBUFFERS,RenderCmd_toString(LUXI_CLASS_RCMD_FBO_DRAWBUFFERS),RRenderCmd_free,NULL);
  Reference_registerType(LUXI_CLASS_RCMD_FBO_READBUFFER,RenderCmd_toString(LUXI_CLASS_RCMD_FBO_READBUFFER),RRenderCmd_free,NULL);

  PText_init();
  Render_initGL();
  Draw_init();

  List3D_init(numL3DSets,numL3DLayers,numL3DLayerNodes,numL3DTotalProjectors);
  List2D_init();

  l_FBOTargets[FBO_TARGET_DRAW] = (GLEW_EXT_framebuffer_blit) ? GL_DRAW_FRAMEBUFFER_EXT : GL_FRAMEBUFFER_EXT;
  l_FBOTargets[FBO_TARGET_READ] = GL_READ_FRAMEBUFFER_EXT;
  //l_FBOTargets[FBO_TARGET_SETTING] = GL_FRAMEBUFFER_EXT;

  CameraLightSky_init();

  GLParticleBuffer_init();

  memset(&l_rdata,0,sizeof(RenderData_t));
}

enum32 FBOTarget_getBind(FBOTarget_t target)
{
  return l_FBOTargets[target];
}

void Render_deinit(void){
  CameraLightSky_deinit();
  Draw_deinit();
  GLParticleBuffer_deinit();
  PText_deinit();
  List3D_deinit();
  List2D_deinit();
}


void RenderBackground_setGL(RenderBackground_t *rback)
{
  if (rback->fogmode){
    glFogi(GL_FOG_MODE,rback->fogmode);
    vidFogColor(rback->fogcolor);
    glFogf(GL_FOG_START,rback->fogstart);
    glFogf(GL_FOG_END,rback->fogend);
    glFogf(GL_FOG_DENSITY,rback->fogdensity);
    g_VID.state.renderflagignore &= ~RENDER_FOG;

    cgSetParameter3fv(g_VID.cg.fogcolor,rback->fogcolor);
    cgSetParameter3f(g_VID.cg.fogdistance,rback->fogstart,rback->fogend,rback->fogdensity);
  }
  else
    g_VID.state.renderflagignore |= RENDER_FOG;
}

void RenderBackground_render(RenderBackground_t *rback, enum32 forceflag, enum32 ignoreflag)
{
  if (rback->skybox){
    vidTexMatrixSet(NULL);
    Draw_SkyBox(rback->skybox);
  }
}

void Render_initGL(){
  lxVector4 color;

  glEnableClientState(GL_VERTEX_ARRAY);
  glShadeModel( GL_SMOOTH);
  glClearDepth(1.0f);                   // Depth Buffer Setup
  glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  vidDepthFunc(GL_LEQUAL);                  // The Type Of Depth Test To Do
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);    // Really Nice Perspective Calculations

  lxVector4Set(color,1,1,1,1);
  glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,color);
  glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,color);
  glEnable(GL_COLOR_MATERIAL);
  vidNoCullFace(LUX_FALSE);

  // tightly pack textures
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, color);

  {
    int stack;
    glMatrixMode(GL_MODELVIEW);
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&stack);
    while (stack-- > 1)
      glPopMatrix();
  }
}


void Render_pushGL()
{
  FBRenderBuffer_t  *rb;
  FBObject_t      *fbo;
  HWBufferObject_t  *vbo;

  lxListNode_forEach(l_rdata.fboListHead,fbo)
    glGenFramebuffersEXT(1,&fbo->glID);
    FBObject_reset(fbo);
  lxListNode_forEachEnd(l_rdata.fboListHead,fbo)

  lxListNode_forEach(l_rdata.fbrenderbufferListHead,rb)
    glGenRenderbuffersEXT(1,&rb->glID);
    FBRenderBuffer_set(rb,rb->textype,rb->width,rb->height,rb->winsized,rb->multisamples);
  lxListNode_forEachEnd(l_rdata.fbrenderbufferListHead,rb)

  lxListNode_forEach(l_rdata.vboListHead,vbo)
    VIDBuffer_initGL(&vbo->buffer,vbo->buffer.type,vbo->buffer.hint,vbo->buffer.size,vbo->tempcopy);
    if (vbo->tempcopy){
      lxMemGenFree(vbo->tempcopy,vbo->buffer.size);
      vbo->tempcopy = NULL;
    }
  lxListNode_forEachEnd(l_rdata.vboListHead,vbo)

  GLParticleBuffer_init();
}

void Render_popGL()
{
  FBRenderBuffer_t  *rb;
  FBObject_t      *fbo;
  HWBufferObject_t  *vbo;

  lxListNode_forEach(l_rdata.fboListHead,fbo)
    glDeleteFramebuffersEXT(1,&fbo->glID);
  lxListNode_forEachEnd(l_rdata.fboListHead,fbo)

  lxListNode_forEach(l_rdata.fbrenderbufferListHead,rb)
    glDeleteRenderbuffersEXT(1,&rb->glID);
  lxListNode_forEachEnd(l_rdata.fbrenderbufferListHead,rb)

  lxListNode_forEach(l_rdata.vboListHead,vbo)
    if (vbo->keeponloss){
      vbo->tempcopy = lxMemGenAlloc(vbo->buffer.size);
      VIDBuffer_retrieveGL(&vbo->buffer,0,vbo->buffer.size,vbo->tempcopy);
    }
    VIDBuffer_clearGL(&vbo->buffer);
  lxListNode_forEachEnd(l_rdata.vboListHead,vbo)

  GLParticleBuffer_deinit();
}

void Render_onWindowResize()
{
  FBObject_t  *fbo;
  FBRenderBuffer_t *rb;

  lxListNode_forEach(l_rdata.fboListHead,fbo)
    if (fbo->winsized){
      FBObject_setSize(fbo,0,0,LUX_TRUE);
    }
    FBObject_reset(fbo);
  lxListNode_forEachEnd(l_rdata.fboListHead,fbo)

  lxListNode_forEach(l_rdata.fbrenderbufferListHead,rb)
    if (rb->winsized){
      FBRenderBuffer_set(rb,rb->textype,rb->width,rb->height,rb->winsized,rb->multisamples);
    }
  lxListNode_forEachEnd(l_rdata.fbrenderbufferListHead,rb)
}

///////////////////////////////////////////////////////////////////////////////
// RenderHelpers

void RenderHelpers_init()
{
  RenderHelpers_t *dh;
  dh = &g_Draws;
  // defaults
  memset(dh,0,sizeof(RenderHelpers_t));

  dh->boneLen = 2;
  dh->axisLen = 5;
  dh->normalLen = 1;
  dh->drawVisTo = -1;
  dh->drawVisFrom = 0;
  dh->drawSpecial = -1;
  dh->texsize = 160;
  dh->forceFinish = LUX_TRUE;

  lxVector4Set(dh->statsColor,1,0.3,0.1,1.0);

}

//////////////////////////////////////////////////////////////////////////
// RenderCommand

static const int l_rcmdsizes[] ={
  sizeof(RenderCmd_t),    //  LUXI_CLASS_RCMD
  sizeof(RenderCmdClear_t), //  LUXI_CLASS_RCMD_CLEAR
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_STENCIL
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_DEPTH
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_LOGICOP
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_FORCEFLAG
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_IGNOREFLAG
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_BASESHADERS
  sizeof(RenderCmd_t),    //  LUXI_CLASS_RCMD_BASESHADERS_OFF
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_IGNOREEXTENDED
  sizeof(RenderCmdTexcopy_t), //  LUXI_CLASS_RCMD_COPYTEX
  sizeof(RenderCmdMesh_t),  //  LUXI_CLASS_RCMD_DRAWMESH
  sizeof(RenderCmd_t),    //  LUXI_CLASS_RCMD_DRAWBACKGROUND
  sizeof(RenderCmd_t),    //  LUXI_CLASS_RCMD_DRAWDEBUG
  sizeof(RenderCmdDraw_t),  //  LUXI_CLASS_RCMD_DRAWLAYER
  sizeof(RenderCmdDraw_t),  //  LUXI_CLASS_RCMD_DRAWPARTICLES
  sizeof(RenderCmdDrawL2D_t), //  LUXI_CLASS_RCMD_DRAWL2D
  sizeof(RenderCmdDrawL3D_t), //  LUXI_CLASS_RCMD_DRAWL3D
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_GENMIPMAPS,
  sizeof(RenderCmdRead_t),  //  LUXI_CLASS_RCMD_READPIXELS
  sizeof(RenderCmdR2VB_t),  //  LUXI_CLASS_RCMD_R2VB
  sizeof(RenderCmdTexupd_t),  //  LUXI_CLASS_RCMD_UPDATETEX
  sizeof(RenderCmdFnCall_t),  //  LUXI_CLASS_RCMD_CALL

  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_FBO_BIND,
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_FBO_OFF,
  sizeof(RenderCmdFBTex_t), //  LUXI_CLASS_RCMD_FBO_TEXASSIGN,
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_FBO_RBASSIGN,
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_FBO_DRAWBUFFERS,
  sizeof(RenderCmdFlag_t),  //  LUXI_CLASS_RCMD_FBO_READBUFFER,
  sizeof(RenderCmdFBBlit_t),  //  LUXI_CLASS_RCMD_BUFFER_BLIT,
};


char * RenderCmd_toString(lxClassType type)
{
  switch(type){
    case LUXI_CLASS_RCMD: return "rcmd";
    case LUXI_CLASS_RCMD_CLEAR: return "rcmdclear";
    case LUXI_CLASS_RCMD_STENCIL: return "rcmdstencil";
    case LUXI_CLASS_RCMD_DEPTH: return "rcmddepth";
    case LUXI_CLASS_RCMD_LOGICOP: return "rcmdlogicop";
    case LUXI_CLASS_RCMD_FORCEFLAG: return "rcmdforceflag";
    case LUXI_CLASS_RCMD_IGNOREFLAG: return "rcmdignoreflag";
    case LUXI_CLASS_RCMD_BASESHADERS: return "rcmdshader";
    case LUXI_CLASS_RCMD_BASESHADERS_OFF: return "rcmdshadersoff";
    case LUXI_CLASS_RCMD_IGNOREEXTENDED: return "rcmdignore";
    case LUXI_CLASS_RCMD_COPYTEX: return "rcmdcopytex";
    case LUXI_CLASS_RCMD_DRAWMESH: return "rcmddrawmesh";
    case LUXI_CLASS_RCMD_DRAWBACKGROUND: return "rcmddrawbg";
    case LUXI_CLASS_RCMD_DRAWDEBUG: return "rcmddrawdbg";
    case LUXI_CLASS_RCMD_DRAWLAYER: return "rcmddrawlayer";
    case LUXI_CLASS_RCMD_DRAWPARTICLES: return "rcmddrawprt";
    case LUXI_CLASS_RCMD_DRAWL2D: return "rcmddrawl2d";
    case LUXI_CLASS_RCMD_DRAWL3D: return "rcmddrawl3d";
    case LUXI_CLASS_RCMD_GENMIPMAPS: return "rcmdmipmaps";
    case LUXI_CLASS_RCMD_READPIXELS: return "rcmdreadpixels";
    case LUXI_CLASS_RCMD_R2VB: return "rcmdr2vb";
    case LUXI_CLASS_RCMD_UPDATETEX: return "rcmdupdatetex";
    case LUXI_CLASS_RCMD_FNCALL: return "rcmdfncall";
    case LUXI_CLASS_RCMD_FBO_BIND: return "rcmdfbobind";
    case LUXI_CLASS_RCMD_FBO_OFF: return "rcmdfbooff";
    case LUXI_CLASS_RCMD_FBO_TEXASSIGN: return "rcmdfbotex";
    case LUXI_CLASS_RCMD_FBO_RBASSIGN: return "rcmdfborb";
    case LUXI_CLASS_RCMD_FBO_DRAWBUFFERS: return "rcmdfbodrawto";
    case LUXI_CLASS_RCMD_FBO_READBUFFER: return "rcmdfboreadfrom";
    case LUXI_CLASS_RCMD_BUFFER_BLIT: return "rcmdbufferblit";
    default: return "no rcmd";
  }
}

//////////////////////////////////////////////////////////////////////////


void RenderCmdClear_run(RenderCmdClear_t *clear,
          struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  int clearv;

  clearv =  (clear->cleardepth * GL_DEPTH_BUFFER_BIT) |
        (clear->clearcolor * GL_COLOR_BUFFER_BIT) |
        (clear->clearstencil * GL_STENCIL_BUFFER_BIT);

  if (clearv){
    vidNoColorMask(LUX_FALSE);
    vidNoDepthMask(LUX_FALSE);
    vidStencilMask(LUX_TRUE);

    glClearStencil(clear->stencil);
    glClearDepth(clear->depth);

    switch (clear->clearmode)
    {
    case 0: glClearColor(lxVector4Unpack(clear->color)); break;
    case 1: glClearColorIiEXT(lxVector4Unpack(clear->colori)); break;
    case 2: glClearColorIuiEXT(lxVector4Unpack(clear->colori)); break;
    }

    glClear(clearv);
  }
}

void  RenderCmdFBBlit_run(RenderCmdFBBlit_t *cmd,
            struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  enum32 copyv =  (cmd->copydepth * GL_DEPTH_BUFFER_BIT) |
          (cmd->copycolor * GL_COLOR_BUFFER_BIT) |
          (cmd->copystencil * GL_STENCIL_BUFFER_BIT);
  if (copyv){
    glBlitFramebufferEXT(cmd->srcX,cmd->srcY,cmd->srcX+cmd->srcWidth,
      cmd->srcY+cmd->srcHeight,
      cmd->dstX,cmd->dstY,cmd->dstX+cmd->dstWidth,
      cmd->dstY+cmd->dstHeight,
      copyv,cmd->linear ? GL_LINEAR : GL_NEAREST);
  }
}

void RenderCmdTexupd_run(RenderCmdTexupd_t* cmd,
             struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  if (!cmd->bufferref) return;

  vidBindBufferUnpack(cmd->buffer);
  Texture_resubmitGL(ResData_getTexture(cmd->texRID),cmd->pos[3],LUX_ARRAY3UNPACK(cmd->pos),LUX_ARRAY3UNPACK(cmd->size),(void*)cmd->offset,cmd->dataformat,1,LUX_TRUE);
  vidBindBufferUnpack(NULL);
}

void RenderCmdMesh_run(RenderCmdMesh_t *cmd,
            struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  flags32 forceflags = lstate->forceflag;
  flags32 ignoreflags = ~lstate->ignoreflag;
  LinkObject_t  *lobj;

  Draw_useBaseShaders(0);
  if (cmd->ortho && !lstate->orthoprojection){
    lstate->orthoprojection = LUX_TRUE;
    vidOrthoOn(0,g_VID.state.viewport.bounds.width ,0,g_VID.state.viewport.bounds.height,-128,128);
  }
  else if (!cmd->ortho && lstate->orthoprojection){
    vidOrthoOff();
    lstate->orthoprojection = LUX_FALSE;
  }

  if (lstate->orthoprojection && cmd->autosize){
    float z = cmd->matrix[14];
    lxMatrix44IdentitySIMD(cmd->matrix);
    cmd->matrix[14] = z;
    lxVector3Set(cmd->size,g_VID.state.viewport.bounds.width,g_VID.state.viewport.bounds.height,1.0f);
  }
  else if (!lstate->orthoprojection && cmd->linkref && Reference_get(cmd->linkref,lobj)){
    lxMatrix44CopySIMD(cmd->matrix,lobj->matrix);
  }

  vidWorldMatrix(cmd->matrix);
  vidWorldScale(cmd->size);

  DrawMesh_draw(&cmd->dnode,forceflags,ignoreflags);
}

void RenderCmdR2VB_run(RenderCmdR2VB_t*cmd,
            struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  static GLenum capturetypes[] = {
    GL_POINTS,GL_LINES,GL_LINES,GL_LINES,
    GL_TRIANGLES,GL_TRIANGLES,GL_TRIANGLES,
    GL_TRIANGLES,GL_TRIANGLES,GL_TRIANGLES,
  };

  RenderCmdMesh_t* rmesh = cmd->rcmdmesh;
  booln currentortho = lstate->orthoprojection;
  flags32 forceflags = lstate->forceflag;
  flags32 ignoreflags = ~lstate->ignoreflag;

  if (!rmesh || !rmesh->dnode.mesh || !cmd->numBuffers || !cmd->numAttribs) return;


  //for (i = 0; i < cmd->numBuffers; i++){
  // for now do only interleaved
  {
    R2VBuffer_t *buf = &cmd->buffers[0];
    VIDBuffer_t *vbuf = buf->vbuf;

    if (!buf->offset && !buf->size){
      glBindBufferBaseNV(GL_TRANSFORM_FEEDBACK_BUFFER,0,vbuf->glID);
    }
    else if (!buf->size){
      glBindBufferOffsetNV(GL_TRANSFORM_FEEDBACK_BUFFER,0,vbuf->glID,
        buf->offset);
    }
    else{
      glBindBufferRangeNV(GL_TRANSFORM_FEEDBACK_BUFFER,0,vbuf->glID,
        buf->offset,buf->size);
    }
    // dirty hack
    g_VID.state.activeBuffers[VID_BUFFER_FEEDBACK] = vbuf;
  }
  //}

  glTransformFeedbackAttribsNV(cmd->numAttribs,(int*)cmd->attribs,
    cmd->numBuffers == 1 ? GL_INTERLEAVED_ATTRIBS : GL_SEPARATE_ATTRIBS);


  if (cmd->capture){
    glGetQueryivARB(cmd->queryobj,GL_QUERY_RESULT,&cmd->output);
    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT,cmd->queryobj);
  }

  if (cmd->noraster){
    glEnable(GL_RASTERIZER_DISCARD_EXT);
  }

  glBeginTransformFeedback(capturetypes[rmesh->dnode.mesh->primtype]);

  RenderCmdMesh_run(rmesh,view,lstate);

  glEndTransformFeedback();

  if (cmd->noraster){
    glDisable(GL_RASTERIZER_DISCARD_EXT);
  }

  if (cmd->capture){
    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT);
  }

  //{
  //  R2VBuffer_t *buf = &cmd->buffers[0];
  //  glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,0);
  //}
}

typedef struct R2VBAttribID_s{
  const char* name;
  int     index;
  int     maxcomp;
  int     id;
}R2VBAttribID_t;
/*
POSITION                  1,2,3,4     no       position
PRIMARY_COLOR             1,2,3,4     no       color.front.primary
SECONDARY_COLOR_NV        1,2,3,4     no       color.front.secondary
BACK_PRIMARY_COLOR_NV     1,2,3,4     no       color.back.primary
BACK_SECONDARY_COLOR_NV   1,2,3,4     no       color.back.secondary
FOG_COORDINATE            1           no       fogcoord
POINT_SIZE                1           no       pointsize
TEXTURE_COORD_NV          1,2,3,4     yes      texcoord[index]
CLIP_DISTANCE_NV          1           yes      clip[index]
VERTEX_ID_NV              1           no       vertexid
PRIMITIVE_ID_NV           1           no       primid
GENERIC_ATTRIB_NV         1,2,3,4     yes      attrib[index]
*/


booln RenderCmdR2VB_setAttrib(RenderCmdR2VB_t*cmd,int index, const char* name, int comp)
{
  static const R2VBAttribID_t ids[] = {
    {"POSITION",0,4,GL_POSITION},
    {"HPOS",0,4,GL_POSITION},

    {"DIFFUSE",0,4,GL_PRIMARY_COLOR},
    {"COL0",0,4,GL_PRIMARY_COLOR},
    {"COLOR",0,4,GL_PRIMARY_COLOR},
    {"COLOR0",0,4,GL_PRIMARY_COLOR},
    {"SPECULAR",0,4,GL_SECONDARY_COLOR_NV},
    {"COL1",0,4,GL_SECONDARY_COLOR_NV},
    {"COLOR1",0,4,GL_SECONDARY_COLOR_NV},

    {"BCOL0",0,4,GL_BACK_PRIMARY_COLOR_NV},
    {"BCOL1",0,4,GL_BACK_SECONDARY_COLOR_NV},

    {"FOGCOORD",0,1,GL_FOG_COORDINATE},
    {"FOGC",0,1,GL_FOG_COORDINATE},
    {"FOG",0,1,GL_FOG_COORDINATE},

    {"TESSFACTOR",0,4,GL_FOG_COORDINATE},
    {"PSIZE",0,1,GL_POINT_SIZE},
    {"PSIZ",0,1,GL_POINT_SIZE},

    {"TEXCOORD%d",7,4,GL_TEXTURE_COORD_NV},
    {"TEX%d",7,4,GL_TEXTURE_COORD_NV},

    {"CLP%d",5,1,GL_CLIP_DISTANCE_NV},

    {"VERTEXID",0,1,GL_VERTEX_ID_NV},
    {"PRIMITIVEID",0,1,GL_PRIMITIVE_ID_NV},
    {"ATTR%d",15,4,GL_GENERIC_ATTRIB_NV},

    {"BLENDWEIGHT",-1,4,GL_GENERIC_ATTRIB_NV},
    {"BLENDINDICES",-5,4,GL_GENERIC_ATTRIB_NV},
    {"TANGENT",-14,4,GL_GENERIC_ATTRIB_NV},
    {"BINORMAL",-15,4,GL_GENERIC_ATTRIB_NV},
  };

  int i;
  int n = sizeof(ids)/sizeof(R2VBAttribID_t);

  const R2VBAttribID_t* found = NULL;
  int subindex = 0;

  LUX_ASSERT(index >= 0 && index < R2VB_MAX_ATTRIBS);

  for (i = 0; i < n; i++){
    const R2VBAttribID_t* id = &ids[i];
    if (id->index>0 && sscanf(name,id->name,&subindex)){
      found = id;
      break;
    }
    if (id->index<1 && strcmp(id->name,name)==0){
      found = id;
      subindex = 0;
      break;
    }
  }

  if (!found)
    return LUX_FALSE;
  if (found->index > 0 && (subindex > found->index || subindex < 0))
    return LUX_FALSE;
  if (comp < 1 || comp > found->maxcomp)
    return LUX_FALSE;
  if (found->index < 0)
    subindex = -found->index;

  cmd->attribs[index].id = found->id;
  cmd->attribs[index].components = comp;
  cmd->attribs[index].index = subindex;

  return LUX_TRUE;
}


void RenderCmdFnCall_run(RenderCmdFnCall_t *cmd,
            struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  if (!cmd->fncall) return;

  if (cmd->ortho && !lstate->orthoprojection){
    lstate->orthoprojection = LUX_TRUE;
    vidOrthoOn(0,g_VID.state.viewport.bounds.width ,0,g_VID.state.viewport.bounds.height,-128,128);
  }
  else if (!cmd->ortho && lstate->orthoprojection){
    vidOrthoOff();
    lstate->orthoprojection = LUX_FALSE;
  }

  vidWorldMatrix(cmd->matrix);
  vidWorldScale(cmd->size);

  lxMatrix44PreScale(g_VID.drawsetup.worldMatrixCache,cmd->matrix,cmd->size);

  vidStateDisable();

  cmd->fncall(cmd->upvalue,
    g_VID.drawsetup.projMatrix,
    g_VID.drawsetup.viewMatrix,
    g_VID.drawsetup.worldMatrixCache);

  vidStateReset();
}



static const enum32 l_FBOAssigns[]= {
  GL_COLOR_ATTACHMENT0_EXT,
  GL_COLOR_ATTACHMENT1_EXT,
  GL_COLOR_ATTACHMENT2_EXT,
  GL_COLOR_ATTACHMENT3_EXT,
  GL_DEPTH_ATTACHMENT_EXT,
  GL_STENCIL_ATTACHMENT_EXT};


void  RenderCmdPtr_runFBO(RenderCmdPtr_t pcmd, FBObject_t *fboTargets[FBO_TARGETS], const int *origviewsizes)
{
  enum32 converted[4];

  int n;
  FBObject_t *fbo;

  FBRenderBuffer_t *rb;
  FBTexAssign_t *texassign;
  Texture_t   *tex;

  FBOTarget_t   fbotarget;

  switch(pcmd.cmd->type){
  case LUXI_CLASS_RCMD_FBO_OFF:
    fbotarget = pcmd.flag->fbobind.fbotarget;
    glBindFramebufferEXT(l_FBOTargets[fbotarget],0);

    if (fbotarget == FBO_TARGET_DRAW){
      glDrawBuffer(GL_BACK);

      g_VID.drawbufferWidth = g_VID.windowWidth;
      g_VID.drawbufferHeight = g_VID.windowHeight;

      // set back viewport, and always enforce scissor check due to drawbuffer change
      if(!vidViewport(lxVector4Unpack(origviewsizes))){
        vidViewportScissorCheck();
      }
    }
    else{
      glReadBuffer(GL_BACK);
    }

    fboTargets[fbotarget] = NULL;
    break;
  case LUXI_CLASS_RCMD_FBO_BIND:
    fbotarget = pcmd.flag->fbobind.fbotarget;
    fbo = pcmd.flag->fbobind.fbo;
    LUX_ASSERT(fbo);
    glBindFramebufferEXT(l_FBOTargets[fbotarget],fbo->glID);

    if (fbotarget == FBO_TARGET_DRAW){
      g_VID.drawbufferWidth = fbo->width;
      g_VID.drawbufferHeight = fbo->height;

      if (pcmd.flag->fbobind.viewportchange)
        vidViewport(0,0,fbo->width,fbo->height);
      else
        vidViewportScissorCheck();
    }

    fboTargets[fbotarget] = fbo;
    break;
  case LUXI_CLASS_RCMD_FBO_DRAWBUFFERS:
    if (pcmd.flag->fbobuffers.numBuffers > 1){
      for (n=0; n < pcmd.flag->fbobuffers.numBuffers;n++)
        converted[n] = l_FBOAssigns[pcmd.flag->fbobuffers.buffers[n]];
      glDrawBuffersARB(pcmd.flag->fbobuffers.numBuffers,converted);
    }
    else
      glDrawBuffer(l_FBOAssigns[pcmd.flag->fbobuffers.buffers[0]]);
    break;
  case LUXI_CLASS_RCMD_FBO_READBUFFER:
    glReadBuffer(l_FBOAssigns[pcmd.flag->fboread.buffer]);
    break;
  case LUXI_CLASS_RCMD_FBO_RBASSIGN:
    fbotarget = pcmd.flag->fborb.fbotarget;
    fbo = fboTargets[fbotarget];
    for (n=0; n < FBO_ASSIGNS; n++){
      rb=pcmd.flag->fborb.fborbassigns[n];
      if(fbo->rbassigns[n] != rb){
        fbo->rbassigns[n] = rb;
        if (rb)
          glFramebufferRenderbufferEXT(l_FBOTargets[fbotarget], l_FBOAssigns[n], GL_RENDERBUFFER_EXT,rb->glID);
        else
          glFramebufferRenderbufferEXT(l_FBOTargets[fbotarget], l_FBOAssigns[n], GL_RENDERBUFFER_EXT,0);
      }
    }
    break;
  case LUXI_CLASS_RCMD_FBO_TEXASSIGN:
    fbotarget = pcmd.fbotex->fbotarget;
    fbo = fboTargets[fbotarget];
    texassign = pcmd.fbotex->fbotexassigns;
    for (n=0; n < FBO_ASSIGNS; n++,texassign++){
      if( *((int64*)&fbo->texassigns[n]) != *((int64*)texassign)){
        *((int64*)&fbo->texassigns[n]) = *((int64*)texassign);
        tex = ResData_getTexture(texassign->texRID);
        if (!tex)
          glFramebufferTexture2DEXT(l_FBOTargets[fbotarget], l_FBOAssigns[n], GL_TEXTURE_2D,0,0);
        else if(!texassign->offset)
          glFramebufferTexture2DEXT(l_FBOTargets[fbotarget], l_FBOAssigns[n], tex->target,tex->texID,texassign->mip);
        else if(texassign->offset > 0)
          glFramebufferTexture2DEXT(l_FBOTargets[fbotarget], l_FBOAssigns[n], GL_TEXTURE_CUBE_MAP_POSITIVE_X+texassign->offset-1,tex->texID,texassign->mip);
        else if (tex->format == TEX_FORMAT_3D)
          glFramebufferTexture3DEXT(l_FBOTargets[fbotarget], l_FBOAssigns[n], GL_TEXTURE_3D,tex->texID,texassign->mip,-1-texassign->offset);
        else{
          glFramebufferTextureLayerEXT(l_FBOTargets[fbotarget], l_FBOAssigns[n], tex->texID,texassign->mip,-1-texassign->offset);
        }

      }
    }
    break;
  }

}


//////////////////////////////////////////////////////////////////////////
//
extern void DrawSortNode_radix(void *base, uint num);
extern void Draw_projectors(List3DSet_t *lset);
extern void Draw_lights(List3DSet_t *lset);
extern void Draw_axisView(List3DView_t *view);
extern void List3DSet_drawDebugHelpers();
extern int  List3DView_fillNodeUpdates(List3DNode_t *node,enum32 kickflag, enum32 depthlayer);
extern void List3DLayerData_cameraSortKey(List3DLayerData_t *layerdata,int btf);
extern void List3D_drawParticles(List3DView_t *l3dview, int layer,int forceflags,int ignoreflags);

//////////////////////////////////////////////////////////////////////////
// Sorting PreCheck

static LUX_INLINE void DrawSortNode_sort( void *source, uint size ){
  DrawSortNode_radix(source,size);
}

void RcmdGenMipmap_run(RenderCmdPtr_t pcmd, struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  if (pcmd.flag->texRID >= 0){
    vidSelectTexture(0);
    vidBindTexture(pcmd.flag->texRID);
    vidTexturing(GL_TEXTURE_2D);
    glGenerateMipmapEXT(ResData_getTextureTarget(pcmd.flag->texRID));
    vidCheckError();
  }
}
void RcmdTexCopy_run(RenderCmdPtr_t pcmd, struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  if (pcmd.texcopy->texRID >= 0){
    if (pcmd.texcopy->autosize){
      pcmd.texcopy->screenX = g_VID.state.viewport.bounds.x;
      pcmd.texcopy->screenY = g_VID.state.viewport.bounds.y;
      pcmd.texcopy->screenWidth = g_VID.state.viewport.bounds.width;
      pcmd.texcopy->screenHeight = g_VID.state.viewport.bounds.height;
    }
    Texture_copyFramebufferGL(ResData_getTexture(pcmd.texcopy->texRID),
      pcmd.texcopy->texside,
      pcmd.texcopy->mip,
      pcmd.texcopy->texX,pcmd.texcopy->texY,
      pcmd.texcopy->screenX,pcmd.texcopy->screenY,
      pcmd.texcopy->screenWidth,pcmd.texcopy->screenHeight);
    vidCheckError();
  }
}

void RcmdSimple_run(RenderCmdPtr_t pcmd, struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  switch(*pcmd.type){
  case LUXI_CLASS_RCMD_STENCIL:
    {
      VIDStencil_t* stencil = &pcmd.flag->stencil;
      if (stencil->enabled)
        VIDStencil_setGL(stencil,stencil->twosided);
    }
    break;
  case LUXI_CLASS_RCMD_DEPTH:
    if (pcmd.flag->depth.func)
      vidDepthFunc(pcmd.flag->depth.func);
    break;
  case LUXI_CLASS_RCMD_LOGICOP:
    VIDLogicOp_setGL(&pcmd.flag->logic);
    break;
  case LUXI_CLASS_RCMD_FORCEFLAG:
    lstate->forceflags = pcmd.flag->renderflag;
    lstate->forceflag = view->depthonly ? RENDER_NOCOLORMASK | lstate->forceflags : lstate->forceflags;
    break;
  case LUXI_CLASS_RCMD_IGNOREFLAG:
    lstate->ignoreflags = pcmd.flag->renderflag;
    lstate->ignoreflag = lstate->ignorelights ? (RENDER_FXLIT | RENDER_SUNLIT | RENDER_LIT | lstate->ignoreflags): lstate->ignoreflags;
    break;
  case LUXI_CLASS_RCMD_BASESHADERS:
    memcpy(&g_VID.drawsetup.shaders,&pcmd.flag->shd.shaders,sizeof(VIDShaders_t));
    lstate->useshaders = LUX_TRUE;
    break;
  case LUXI_CLASS_RCMD_BASESHADERS_OFF:
    vidResetShaders();
    lstate->useshaders = LUX_FALSE;
    break;
  case LUXI_CLASS_RCMD_IGNOREEXTENDED:
    lstate->ignoreprojs = pcmd.flag->ignore.projs;
    lstate->ignorelights = pcmd.flag->ignore.lights;
    lstate->ignoreflag = lstate->ignorelights ? (RENDER_FXLIT | RENDER_SUNLIT | RENDER_LIT | lstate->ignoreflags): lstate->ignoreflags;
    break;
  }
}
void RcmdDrawL3D_run(RenderCmdPtr_t pcmd, struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  if (pcmd.drawl3d->node
    //&& Reference_get(pcmd.drawl3d->targetref,pcmd.drawl3d->node)
    && (pcmd.drawl3d->force || pcmd.drawl3d->node->drawnFrame == g_VID.frameCnt))
  {
    DrawSortNode_t *dsnodes = rpoolmalloc(sizeof(DrawSortNode_t)*pcmd.drawl3d->numNodes);
    List3DNode_t *l3dnode = pcmd.drawl3d->node;
    int numnodes = 0;
    int n;

    Draw_useBaseShaders(lstate->useshaders || g_VID.drawsetup.useshaders);
    if (lstate->orthoprojection){
      lstate->orthoprojection = LUX_FALSE;
      vidOrthoOff();
    }

    if (l3dnode->drawnFrame < g_VID.frameCnt){
      l3dnode->enviro.numFxLights = 0;
      l3dnode->enviro.numProjectors = 0;
      List3DView_fillNodeUpdates(l3dnode,0,0);
    }

    for (n = 0; n < pcmd.drawl3d->numNodes; n++){
      DrawNode_t *dnode = &l3dnode->drawNodes[pcmd.drawl3d->startid+n];
      if (!(dnode->draw.state.renderflag & (lstate->kickflag)))
      {
        dsnodes[numnodes++].pType = &dnode->type;
      }

    }

    DrawNodes_drawList(dsnodes,
      numnodes,
      lstate->ignoreprojs,
      ~lstate->ignoreflag,
      lstate->forceflag);

    vidCheckError();

    rpoolsetbegin(dsnodes);
  }
}

void RcmdDrawBackground_run(RenderCmdPtr_t pcmd, struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  if (lstate->orthoprojection){
    lstate->orthoprojection = LUX_FALSE;
    vidOrthoOff();
    //glPopAttrib();
  }
  Draw_useBaseShaders(lstate->useshaders || g_VID.drawsetup.useshaders);
  vidDepthRange(view->viewport.depth[1],view->viewport.depth[1]);
  RenderBackground_render(g_Background,lstate->forceflags,lstate->ignoreflags);
  vidDepthRange(view->viewport.depth[0],view->viewport.depth[1]);
  vidCheckError();
}

void RcmdDrawDebug_run(RenderCmdPtr_t pcmd, struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  // debug drawing
  if ((view->drawcamaxis || (g_Draws.drawCamAxis && lstate->isdefaultcam))){
    if (lstate->orthoprojection){
      lstate->orthoprojection = LUX_FALSE;
      vidOrthoOff();
      //glPopAttrib();
    }
    Draw_axisView(view);
    vidLoadCameraGL();
  }
  if (g_Draws.drawAxis){
    if (lstate->orthoprojection){
      lstate->orthoprojection = LUX_FALSE;
      vidOrthoOff();
      //glPopAttrib();
    }
    vidLoadCameraGL();
    Draw_axis();
    vidLoadCameraGL();
  }
  if (lstate->drawdebug){
    if (lstate->orthoprojection){
      lstate->orthoprojection = LUX_FALSE;
      vidOrthoOff();
      //glPopAttrib();
    }
    List3DSet_drawDebugHelpers();
    vidLoadCameraGL();
  }
  if (g_Draws.drawProj){
    if (lstate->orthoprojection){
      lstate->orthoprojection = LUX_FALSE;
      vidOrthoOff();
      //glPopAttrib();
    }
    Draw_projectors(lstate->lset);
    vidLoadCameraGL();
  }
  if (g_Draws.drawLights){
    if (lstate->orthoprojection){
      lstate->orthoprojection = LUX_FALSE;
      vidOrthoOff();
      //glPopAttrib();
    }
    Draw_lights(lstate->lset);
    vidLoadCameraGL();
  }
}
void RcmdDrawLayer_run(RenderCmdPtr_t pcmd, struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  List3DLayerData_t*layerdata = &lstate->l3ddraw->layerdata[pcmd.draw->normal.layer];
  if (layerdata->numSortNodes){
    Draw_useBaseShaders(lstate->useshaders || g_VID.drawsetup.useshaders);

    if (pcmd.draw->normal.sort > 0){
      DrawSortNode_sort(layerdata->sortNodes,layerdata->numSortNodes);
    }
    else if (pcmd.draw->normal.sort < 0){
      // warning overwrites materialsortkey
      List3DLayerData_cameraSortKey(layerdata,pcmd.draw->normal.sort < -1);
      DrawSortNode_sort(layerdata->sortNodes,layerdata->numSortNodes);
    }

    if (lstate->orthoprojection){
      lstate->orthoprojection = LUX_FALSE;
      vidOrthoOff();
    }

    DrawNodes_drawList(layerdata->sortNodes,
      layerdata->numSortNodes,
      lstate->ignoreprojs,
      ~lstate->ignoreflag,
      lstate->forceflag);

    vidCheckError();
  }
}
void RcmdDrawParticles_run(RenderCmdPtr_t pcmd, struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  List3DLayerData_t* layerdata = &lstate->l3ddraw->layerdata[pcmd.draw->normal.layer];
  if (layerdata->pcloudListHead || layerdata->psysListHead){
    g_VID.shdsetup.lightmapRID = -1;
    Draw_useBaseShaders(LUX_FALSE);

    if (lstate->orthoprojection){
      lstate->orthoprojection = LUX_FALSE;
      vidOrthoOff();
      //glPopAttrib();
    }
    List3D_drawParticles(view,pcmd.draw->normal.layer,lstate->forceflags,lstate->ignoreflags);
    vidLoadCameraGL();
    vidCheckError();
  }
}
void RcmdDrawL2D_run(RenderCmdPtr_t pcmd, struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  if (pcmd.drawl2d->node
    //&& Reference_get(pcmd.drawl2d->targetref,pcmd.drawl2d->node)
    )
  {
    booln n;
    if (!g_Draws.drawNoGUI){
      if (lstate->orthoprojection){
        lstate->orthoprojection = LUX_FALSE;
        vidOrthoOff();
        //glPopAttrib(); // actually this may be buggy!
        // l2d_draw push/pops matrix itself
      }

      Draw_useBaseShaders(LUX_FALSE);

      // check for scissor changes
      if (pcmd.drawl2d->viewportsizes[0] != g_VID.state.viewport.bounds.sizes[0] ||
        pcmd.drawl2d->viewportsizes[1] != g_VID.state.viewport.bounds.sizes[1] ||
        pcmd.drawl2d->viewportsizes[2] != g_VID.state.viewport.bounds.sizes[2] ||
        pcmd.drawl2d->viewportsizes[3] != g_VID.state.viewport.bounds.sizes[3])
      {
        n = LUX_TRUE;
        LUX_ARRAY4COPY(pcmd.drawl2d->viewportsizes,g_VID.state.viewport.bounds.sizes);
        // convert to referencesystem
        pcmd.drawl2d->viewrefsizes[0] = (float)pcmd.drawl2d->viewportsizes[0]*VID_REF_WIDTHSCALE;
        pcmd.drawl2d->viewrefsizes[1] = (float)pcmd.drawl2d->viewportsizes[1]*VID_REF_WIDTHSCALE;
        pcmd.drawl2d->viewrefsizes[2] = (float)pcmd.drawl2d->viewportsizes[2]*VID_REF_HEIGHTSCALE;
        pcmd.drawl2d->viewrefsizes[3] = (float)pcmd.drawl2d->viewportsizes[3]*VID_REF_HEIGHTSCALE;

        pcmd.drawl2d->viewrefsizes[1] = (float)g_VID.windowHeight - pcmd.drawl2d->viewrefsizes[1] - pcmd.drawl2d->viewrefsizes[3];
      }
      else{
        n = LUX_FALSE;
      }

      List2D_draw(pcmd.drawl2d->node,pcmd.drawl2d->refsize[0],pcmd.drawl2d->refsize[1],n,pcmd.drawl2d->viewrefsizes,g_VID.state.viewport.bounds.fullwindow ? NULL : g_VID.state.viewport.bounds.sizes);
      vidCheckError();
      // force resetting camera
      //g_CamLight.camera = NULL; should work without, as 2d_draw push/pops matrix itself
    }
  }
  else
    pcmd.drawl2d->node = NULL;
  vidCheckError();
}

void  RenderCmdFBO_run(RenderCmdPtr_t pcmd, struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  switch(*pcmd.type){
  case LUXI_CLASS_RCMD_FBO_BIND:
    if (lstate->orthoprojection){
      lstate->orthoprojection = LUX_FALSE;
      vidOrthoOff();
      //glPopAttrib();
    }
    RenderCmdPtr_runFBO(pcmd,lstate->fboTargets,view->viewport.bounds.sizes);

    vidCheckError();
    break;
  case LUXI_CLASS_RCMD_FBO_OFF:
    if (lstate->orthoprojection){
      lstate->orthoprojection = LUX_FALSE;
      vidOrthoOff();
      //glPopAttrib();
    }
    RenderCmdPtr_runFBO(pcmd,lstate->fboTargets,view->viewport.bounds.sizes);
    vidCheckError();
    break;
  case LUXI_CLASS_RCMD_FBO_DRAWBUFFERS:
  case LUXI_CLASS_RCMD_FBO_READBUFFER:
  case LUXI_CLASS_RCMD_FBO_RBASSIGN:
  case LUXI_CLASS_RCMD_FBO_TEXASSIGN:
    RenderCmdPtr_runFBO(pcmd,lstate->fboTargets,view->viewport.bounds.sizes);
    vidCheckError();
    break;
  }
}

void RenderCmdRead_run(RenderCmdPtr_t pcmd,
             struct List3DView_s* view, struct List3DDrawState_s *lstate)
{
  if (pcmd.read->bufferref){
    vidBindBufferPack(pcmd.read->buffer);

    glReadPixels(pcmd.read->rect[0],pcmd.read->rect[1],pcmd.read->rect[2],pcmd.read->rect[3],pcmd.read->format,pcmd.read->dataformat,(void*)pcmd.read->offset);
  }
}

static booln l_rcsimd[] =
{
  0,    //  LUXI_CLASS_RCMD
  0,  //  LUXI_CLASS_RCMD_CLEAR
  0,  //  LUXI_CLASS_RCMD_STENCIL
  0,  //  LUXI_CLASS_RCMD_DEPTH
  0,  //  LUXI_CLASS_RCMD_LOGICOP
  0,  //  LUXI_CLASS_RCMD_FORCEFLAG
  0,  //  LUXI_CLASS_RCMD_IGNOREFLAG
  0,  //  LUXI_CLASS_RCMD_BASESHADERS
  0,  //  LUXI_CLASS_RCMD_BASESHADERS_OFF
  0,  //  LUXI_CLASS_RCMD_IGNOREEXTENDED
  0,  //  LUXI_CLASS_RCMD_COPYTEX
  1,  //  LUXI_CLASS_RCMD_DRAWMESH
  0,  //  LUXI_CLASS_RCMD_DRAWBACKGROUND
  0,  //  LUXI_CLASS_RCMD_DRAWDEBUG
  0,  //  LUXI_CLASS_RCMD_DRAWLAYER
  0,  //  LUXI_CLASS_RCMD_DRAWPARTICLES
  0,  //  LUXI_CLASS_RCMD_DRAWL2D
  0,  //  LUXI_CLASS_RCMD_DRAWL3D
  0,  //  LUXI_CLASS_RCMD_GENMIPMAPS,
  0,  //  LUXI_CLASS_RCMD_READPIXELS
  0,  //  LUXI_CLASS_RCMD_R2VB
  0,  //  LUXI_CLASS_RCMD_UPDATETEX
  1,  //  LUXI_CLASS_RCMD_CALL

  0,  //  LUXI_CLASS_RCMD_FBO_BIND,
  0,  //  LUXI_CLASS_RCMD_FBO_OFF,
  0,  //  LUXI_CLASS_RCMD_FBO_TEXASSIGN,
  0,  //  LUXI_CLASS_RCMD_FBO_RBASSIGN,
  0,  //  LUXI_CLASS_RCMD_FBO_DRAWBUFFERS,
  0,  //  LUXI_CLASS_RCMD_FBO_READBUFFER,
  0,  //  LUXI_CLASS_RCMD_BUFFER_BLIT,
};

static Rcmd_run_fn* l_runcmds[] =
{
  NULL,   //  LUXI_CLASS_RCMD
  (Rcmd_run_fn*)RenderCmdClear_run, //  LUXI_CLASS_RCMD_CLEAR
  (Rcmd_run_fn*)RcmdSimple_run, //  LUXI_CLASS_RCMD_STENCIL
  (Rcmd_run_fn*)RcmdSimple_run, //  LUXI_CLASS_RCMD_DEPTH
  (Rcmd_run_fn*)RcmdSimple_run, //  LUXI_CLASS_RCMD_LOGICOP
  (Rcmd_run_fn*)RcmdSimple_run, //  LUXI_CLASS_RCMD_FORCEFLAG
  (Rcmd_run_fn*)RcmdSimple_run, //  LUXI_CLASS_RCMD_IGNOREFLAG
  (Rcmd_run_fn*)RcmdSimple_run, //  LUXI_CLASS_RCMD_BASESHADERS
  (Rcmd_run_fn*)RcmdSimple_run, //  LUXI_CLASS_RCMD_BASESHADERS_OFF
  (Rcmd_run_fn*)RcmdSimple_run, //  LUXI_CLASS_RCMD_IGNOREEXTENDED
  (Rcmd_run_fn*)RcmdTexCopy_run,  //  LUXI_CLASS_RCMD_COPYTEX
  (Rcmd_run_fn*)RenderCmdMesh_run,  //  LUXI_CLASS_RCMD_DRAWMESH
  (Rcmd_run_fn*)RcmdDrawBackground_run,   //  LUXI_CLASS_RCMD_DRAWBACKGROUND
  (Rcmd_run_fn*)RcmdDrawDebug_run,    //  LUXI_CLASS_RCMD_DRAWDEBUG
  (Rcmd_run_fn*)RcmdDrawLayer_run,    //  LUXI_CLASS_RCMD_DRAWLAYER
  (Rcmd_run_fn*)RcmdDrawParticles_run,  //  LUXI_CLASS_RCMD_DRAWPARTICLES
  (Rcmd_run_fn*)RcmdDrawL2D_run,    //  LUXI_CLASS_RCMD_DRAWL2D
  (Rcmd_run_fn*)RcmdDrawL3D_run,    //  LUXI_CLASS_RCMD_DRAWL3D
  (Rcmd_run_fn*)RcmdGenMipmap_run,    //  LUXI_CLASS_RCMD_GENMIPMAPS,
  (Rcmd_run_fn*)RenderCmdRead_run,  //  LUXI_CLASS_RCMD_READPIXELS
  (Rcmd_run_fn*)RenderCmdR2VB_run,  //  LUXI_CLASS_RCMD_R2VB
  (Rcmd_run_fn*)RenderCmdTexupd_run,  //  LUXI_CLASS_RCMD_UPDATETEX
  (Rcmd_run_fn*)RenderCmdFnCall_run,  //  LUXI_CLASS_RCMD_CALL

  (Rcmd_run_fn*)RenderCmdFBO_run, //  LUXI_CLASS_RCMD_FBO_BIND,
  (Rcmd_run_fn*)RenderCmdFBO_run, //  LUXI_CLASS_RCMD_FBO_OFF,
  (Rcmd_run_fn*)RenderCmdFBO_run, //  LUXI_CLASS_RCMD_FBO_TEXASSIGN,
  (Rcmd_run_fn*)RenderCmdFBO_run, //  LUXI_CLASS_RCMD_FBO_RBASSIGN,
  (Rcmd_run_fn*)RenderCmdFBO_run, //  LUXI_CLASS_RCMD_FBO_DRAWBUFFERS,
  (Rcmd_run_fn*)RenderCmdFBO_run, //  LUXI_CLASS_RCMD_FBO_READBUFFER,
  (Rcmd_run_fn*)RenderCmdFBBlit_run,  //  LUXI_CLASS_RCMD_BUFFER_BLIT,
};

RenderCmd_t* RenderCmd_new(lxClassType type)
{
  RenderCmdPtr_t pcmd;
  DrawMesh_t *dnode2d;
  booln simd = l_rcsimd[type-LUXI_CLASS_RCMD];
  int i;

  pcmd.ptr = simd ? genzallocSIMD(l_rcmdsizes[type-LUXI_CLASS_RCMD]) : lxMemGenZalloc(l_rcmdsizes[type-LUXI_CLASS_RCMD]);
  pcmd.cmd->reference = Reference_new(type,pcmd.ptr);
  pcmd.cmd->type = type;
  pcmd.cmd->runflag = 1;
  pcmd.cmd->fnrun = l_runcmds[type-LUXI_CLASS_RCMD];

  switch(type)
  {
  default:
    break;
  case LUXI_CLASS_RCMD_CLEAR:
    pcmd.clear->clearcolor = LUX_TRUE;
    pcmd.clear->clearstencil = LUX_TRUE;
    pcmd.clear->cleardepth = LUX_TRUE;
    pcmd.clear->depth = 1.0f;
    break;
  case LUXI_CLASS_RCMD_DRAWMESH:
    LUX_SIMDASSERT(((size_t)pcmd.mesh->matrix) % 16 == 0);

    pcmd.mesh->autosize = -1;
    pcmd.mesh->ortho = LUX_TRUE;
    lxMatrix44IdentitySIMD(pcmd.mesh->matrix);
    dnode2d = &pcmd.mesh->dnode;
    dnode2d->mesh = g_VID.drw_meshquadffx;
    dnode2d->state.renderflag = RENDER_NOVERTEXCOLOR | RENDER_NODEPTHMASK | RENDER_NODEPTHTEST;
    lxVector3Set(pcmd.mesh->size,1,1,1);
    lxVector4Set(dnode2d->color,1,1,1,1);
    dnode2d->matRID = -1;
    break;
  case LUXI_CLASS_RCMD_COPYTEX:
    pcmd.texcopy->autosize = -1;
    pcmd.texcopy->texRID = -1;
    break;
  case LUXI_CLASS_RCMD_STENCIL:
    VIDStencil_init(&pcmd.flag->stencil);
    break;
  case LUXI_CLASS_RCMD_DEPTH:
    VIDDepth_init(&pcmd.flag->depth);
    break;
  case LUXI_CLASS_RCMD_BASESHADERS:
    for (i = 0; i < VID_SHADERS; i++)
      pcmd.flag->shd.shaders.ids[i] = -1;
    break;
  case LUXI_CLASS_RCMD_FNCALL:
    lxMatrix44IdentitySIMD(pcmd.fncall->matrix);
    lxVector3Set(pcmd.fncall->size,1,1,1);
    break;
  case LUXI_CLASS_RCMD_FBO_TEXASSIGN:
    for (i = 0; i < FBO_ASSIGNS; i++)
      pcmd.fbotex->fbotexassigns[i].texRID = -1;
    break;
  case LUXI_CLASS_RCMD_FBO_DRAWBUFFERS:
    pcmd.flag->fbobuffers.numBuffers = 1;
    pcmd.flag->fbobuffers.buffers[0] = 0;
    break;
  case LUXI_CLASS_RCMD_FBO_READBUFFER:
    pcmd.flag->fboread.buffer = 0;
    break;
  case LUXI_CLASS_RCMD_FBO_BIND:
    pcmd.flag->fbobind.viewportchange = LUX_TRUE;
    break;
  case LUXI_CLASS_RCMD_GENMIPMAPS:
    pcmd.flag->texRID = -1;
    break;
  }

  return pcmd.cmd;
}

static void RenderCmd_free(RenderCmd_t  *cmd)
{
  RenderCmdPtr_t pcmd;
  int i;
  pcmd.cmd = cmd;

  switch(pcmd.cmd->type)
  {
  default:
    break;
  case LUXI_CLASS_RCMD_R2VB:
    {
      for(i=0; i < pcmd.r2vb->numBuffers; i++){
        Reference_release((Reference)pcmd.r2vb->buffers[i].vbuf->host);
      }
      Reference_releaseCheck(pcmd.r2vb->inputmesh);
      if (pcmd.r2vb->queryobj){
        glDeleteQueries(1,&pcmd.r2vb->queryobj);
      }
    }
    break;
  case LUXI_CLASS_RCMD_READPIXELS:
    Reference_releaseCheck(pcmd.read->bufferref);
    break;
  case LUXI_CLASS_RCMD_UPDATETEX:
    Reference_releaseCheck(pcmd.texupd->bufferref);
    break;
  case LUXI_CLASS_RCMD_CLEAR:
    break;
  case LUXI_CLASS_RCMD_DRAWMESH:
    Reference_releaseCheck(pcmd.mesh->usermesh);
    Reference_releaseWeakCheck(pcmd.mesh->linkref);
    DrawMesh_clear(&pcmd.mesh->dnode);
    break;
  case LUXI_CLASS_RCMD_FBO_BIND:

    Reference_releasePtr(pcmd.flag->fbobind.fbo,reference);
    break;
  case LUXI_CLASS_RCMD_FBO_RBASSIGN:
    for (i = 1; i < FBO_ASSIGNS+1; i++){
      Reference_releasePtr(pcmd.flag->fborb.fborbassigns[i],reference);
    }
    break;
  case LUXI_CLASS_RCMD_DRAWL2D:

    Reference_releasePtr(pcmd.drawl2d->node,reference);
    break;
  case LUXI_CLASS_RCMD_DRAWL3D:

    Reference_releasePtr(pcmd.drawl3d->node,reference);
    break;
  }

  Reference_invalidate(cmd->reference);
  if (l_rcsimd[pcmd.cmd->type-LUXI_CLASS_RCMD]){
    genfreeSIMD(cmd,l_rcmdsizes[pcmd.cmd->type-LUXI_CLASS_RCMD]);
  }
  else{
    lxMemGenFree(cmd,l_rcmdsizes[pcmd.cmd->type-LUXI_CLASS_RCMD]);
  }
}
void RRenderCmd_free(Reference ref) {
  RenderCmd_free((RenderCmd_t*)Reference_value(ref));
}

//////////////////////////////////////////////////////////////////////////
// RenderBuffer

FBRenderBuffer_t* FBRenderBuffer_new()
{
  FBRenderBuffer_t* rb = lxMemGenZalloc(sizeof(FBRenderBuffer_t));
  rb->reference = Reference_new(LUXI_CLASS_RENDERBUFFER,rb);

  glGenRenderbuffersEXT(1,&rb->glID);

  lxListNode_init(rb);
  lxListNode_addLast(l_rdata.fbrenderbufferListHead,rb);

  return rb;
}

int   FBRenderBuffer_set(FBRenderBuffer_t* rb,enum32 type,int w,int h, int windowsized, int multisamples)
{
  int oldcosts;
  LUX_PROFILING_OP( oldcosts = rb->width*rb->height*rb->multisamples*TextureType_pixelcost(rb->width));

  if (windowsized){
    w = g_VID.windowWidth;
    h = g_VID.windowHeight;
  }
  multisamples = LUX_MIN(g_VID.capFBOMSAA,multisamples);

  rb->width = w;
  rb->height = h;
  rb->textype = type;
  rb->winsized = windowsized;
  rb->multisamples = multisamples;
  vidCheckError();
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb->glID);

  if (multisamples)
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, multisamples, TextureType_toInternalGL(type), w, h);
  else
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, TextureType_toInternalGL(type), w, h);

  if(vidCheckErrorF("Setting RenderBuffer",0))
    return LUX_FALSE;

  LUX_PROFILING_OP( g_Profiling.global.memory.vidsurfmem -= oldcosts );
  LUX_PROFILING_OP( g_Profiling.global.memory.vidsurfmem += (rb->width*rb->height*rb->multisamples*TextureType_pixelcost(rb->width)));
  return LUX_TRUE;
}

static void FBRenderBuffer_free(FBRenderBuffer_t* rb)
{
  Reference_invalidate(rb->reference);

  glDeleteRenderbuffersEXT(1,&rb->glID);
  lxListNode_remove(l_rdata.fbrenderbufferListHead,rb);

  lxMemGenFree(rb,sizeof(FBRenderBuffer_t));
}

void RFBRenderBuffer_free(lxRrenderbuffer ref) {
  FBRenderBuffer_free((FBRenderBuffer_t*)Reference_value(ref));
}
//////////////////////////////////////////////////////////////////////////
// FBObject_t

const char* FBOTarget_checkCompleteness(FBOTarget_t fbotarget){
  switch(glCheckFramebufferStatusEXT(l_FBOTargets[fbotarget]))
  {
  case GL_FRAMEBUFFER_COMPLETE_EXT:           return NULL;
  case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:      return "incomplete attachment: %s";
  case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:  return "incomplete missing attachment: %s";
  case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:      return "incomplete dimensions: %s";
  case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:       return "incomplete formats: %s";
  case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:     return "incomplete drawbuffer: %s";
  case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:     return "incomplete readbuffer: %s";
  case GL_FRAMEBUFFER_UNSUPPORTED_EXT:          return "unsupported: %s";
    //case GL_FRAMEBUFFER_STATUS_ERROR_EXT:         return "status error";
  default:
    LUX_ASSERT(0);
    break;
  }
  return NULL;
}

FBObject_t* FBObject_new()
{
  FBObject_t* fbo = lxMemGenZalloc(sizeof(FBObject_t));
  fbo->reference = Reference_new(LUXI_CLASS_RENDERFBO,fbo);

  glGenFramebuffersEXT(1,&fbo->glID);

  lxListNode_init(fbo);
  lxListNode_addLast(l_rdata.fboListHead,fbo);
  vidCheckError();
  return fbo;
}
void  FBObject_setSize(FBObject_t* fbo,int w,int h,int windowsized)
{
  fbo->winsized = windowsized;

  if (fbo->winsized){
    w = g_VID.windowWidth;
    h = g_VID.windowHeight;
  }

  if (fbo->width != w || fbo->height != h)
    FBObject_reset(fbo);

  fbo->width = w;
  fbo->height = h;
}

static void FBObject_free(FBObject_t* fbo)
{
  Reference_invalidate(fbo->reference);

  glDeleteFramebuffersEXT(1,&fbo->glID);
  lxListNode_remove(l_rdata.fboListHead,fbo);

  lxMemGenFree(fbo,sizeof(FBObject_t));
}

void  FBObject_reset(FBObject_t *fbo)
{
  FBTexAssign_t *texassign;
  int a;
  if(fbo){
    memset(&fbo->rbassigns,0,sizeof(FBRenderBuffer_t*)*FBO_ASSIGNS);
    // ati doesnt like double 0(-1) assignments if renderbuffer is bound
    texassign = fbo->texassigns;
    for (a=0; a < FBO_ASSIGNS; a++,texassign++)
      texassign->texRID = -1;

  }
}

void  FBObject_applyAssigns(FBObject_t* fbo, RenderCmdPtr_t texassign, RenderCmdPtr_t rbassign)
{
  FBObject_t *fboTargets[FBO_TARGETS] = {fbo,fbo};
  FBOTarget_t oldtarget;

  vidCheckError();
  if (texassign.cmd || rbassign.cmd){
    glBindFramebufferEXT(l_FBOTargets[FBO_TARGET_DRAW],fbo->glID);
  }
  else{
    return;
  }

  if (texassign.cmd){
    oldtarget = texassign.fbotex->fbotarget;
    texassign.fbotex->fbotarget = FBO_TARGET_DRAW;

    RenderCmdPtr_runFBO(texassign,fboTargets,NULL);

    texassign.fbotex->fbotarget = oldtarget;
  }
  if (rbassign.cmd){
    oldtarget = rbassign.flag->fborb.fbotarget;
    rbassign.flag->fborb.fbotarget = FBO_TARGET_DRAW;

    RenderCmdPtr_runFBO(rbassign,fboTargets,NULL);

    rbassign.flag->fborb.fbotarget = oldtarget;
  }
  glBindFramebufferEXT(l_FBOTargets[FBO_TARGET_DRAW],0);
  vidCheckError();
}

const char* FBObject_check(FBObject_t* fbo)
{
  const char* str;

  vidCheckError();
  glBindFramebufferEXT(l_FBOTargets[FBO_TARGET_DRAW],fbo->glID);

  str = FBOTarget_checkCompleteness(FBO_TARGET_DRAW);

  glBindFramebufferEXT(l_FBOTargets[FBO_TARGET_DRAW],0);
  vidCheckError();
  return str;
}

void RFBObject_free(lxRrenderfbo ref)
{
  FBObject_free((FBObject_t*)Reference_value(ref));
}


//////////////////////////////////////////////////////////////////////////
// HWBufferObject

HWBufferObject_t* HWBufferObject_new(VIDBufferType_t type, VIDBufferHint_t hint, uint size, void* data)
{
  HWBufferObject_t* vbo = lxMemGenZalloc(sizeof(HWBufferObject_t));
  vbo->reference = Reference_new(LUXI_CLASS_VIDBUFFER,vbo);

  VIDBuffer_initGL(&vbo->buffer,type,hint,size,data);
  vbo->buffer.host = vbo->reference;

  lxListNode_init(vbo);
  lxListNode_addLast(l_rdata.vboListHead,vbo);

  return vbo;
}

static LUX_INLINE void HWBufferObject_free(HWBufferObject_t* vbo)
{
  VIDBuffer_clearGL(&vbo->buffer);
  lxListNode_remove(l_rdata.vboListHead,vbo);

  lxMemGenFree(vbo,sizeof(HWBufferObject_t));
}

void RHWBufferObject_free(lxRvidbuffer ref){
  HWBufferObject_free((HWBufferObject_t*)Reference_value(ref));
}

void HWBufferObject_listop(HWBufferObject_t* buffer, HWBufferObject_t **list)
{
  HWBufferObject_t *listhead;

  if (buffer->listhead){
    listhead = *buffer->listhead;
    lxListNode_remove(listhead,buffer);
    *buffer->listhead = listhead;
    buffer->listhead = NULL;
  }

  if (list){
    listhead = *list;

    lxListNode_init(buffer);
    lxListNode_pushBack(listhead,buffer);
    *list = listhead;
    buffer->listhead = list;
  }
}

//////////////////////////////////////////////////////////////////////////
// CustomMesh

RenderMesh_t* RenderMesh_new()
{
  RenderMesh_t* um = lxMemGenZalloc(sizeof(RenderMesh_t));
  um->reference = Reference_new(LUXI_CLASS_RENDERMESH,um);
  return um;
}

void RRenderMesh_free(lxRrendermesh ref)
{
  RenderMesh_t* um = Reference_value(ref);

  if (um->freemesh){
    Mesh_free(um->mesh);
  }

  Reference_releaseCheck(um->vboref);
  Reference_releaseCheck(um->iboref);

  lxMemGenFree(um,sizeof(RenderMesh_t));
}

