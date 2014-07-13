// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __VIDINL_H__
#define __VIDINL_H__

//////////////////////////////////////////////////////////////////////////
// Error Handling

#if defined(_DEBUG) || 0
  #define vidCheckError()   vidCheckErrorF(__FILE__,__LINE__)
  #define vidCgCheckError() vidCgCheckErrorF(__FILE__,__LINE__)
#else
  #define vidCheckError()   ((void*)0)
  #define vidCgCheckError() ((void*)0)
#endif

#define vidHaltError() \
  LUX_DEBUGASSERT(!glGetError())


//////////////////////////////////////////////////////////////////////////
// Renderflag & State

#define vidSingleRFCmd( flag,  staten,  cmdon, cmdoff)  {\
  if (((g_VID.state.renderflag & flag) != 0) != staten) {\
  if (staten){\
  cmdon;\
  g_VID.state.renderflag |= flag;\
  }\
    else{\
    cmdoff;\
    g_VID.state.renderflag &= ~flag;\
    }\
  }\
}

#define vidSingleRF(flag,  state,  glenum)  vidSingleRFCmd(flag,  state,  glEnable(glenum), glDisable(glenum))

#define vidState(var, state, glenum)  if(state!=var){if(state)glEnable(glenum);else glDisable(glenum); var=state;}

// State
#define vidNormalize(on)  vidSingleRF(RENDER_NORMALIZE, on, GL_NORMALIZE)
#define vidLighting(on)   vidSingleRF(RENDER_LIT, on, GL_LIGHTING)
#define vidBlending(on)   vidSingleRF(RENDER_BLEND, on, GL_BLEND)
#define vidAlphaTest(on)  vidSingleRF(RENDER_ALPHATEST, on, GL_ALPHA_TEST)
#define vidWireframe(on)  vidSingleRFCmd(RENDER_WIRE, on, glPolygonMode (GL_FRONT_AND_BACK, GL_LINE), glPolygonMode (GL_FRONT_AND_BACK, GL_FILL))
#define vidNoDepthMask(on)  vidSingleRFCmd(RENDER_NODEPTHMASK, on, glDepthMask(LUX_FALSE), glDepthMask(LUX_TRUE))
#define vidNoColorMask(on)  vidSingleRFCmd(RENDER_NOCOLORMASK, on, glColorMask(LUX_FALSE,LUX_FALSE,LUX_FALSE,LUX_FALSE), glColorMask(LUX_TRUE,LUX_TRUE,LUX_TRUE,LUX_TRUE))
#define vidNoDepthTest(on)  vidSingleRFCmd(RENDER_NODEPTHTEST, on, glDisable(GL_DEPTH_TEST),glEnable(GL_DEPTH_TEST))
#define vidNoCullFace(on) vidSingleRFCmd(RENDER_NOCULL, on, glDisable(GL_CULL_FACE),glEnable(GL_CULL_FACE))
#define vidNoVertexColor(on) vidSingleRFCmd(RENDER_NOVERTEXCOLOR, on, glDisableClientState(GL_COLOR_ARRAY),glEnableClientState(GL_COLOR_ARRAY))
#define vidFog(on)      vidSingleRF(RENDER_FOG, on, GL_FOG)
#define vidScissor(on)    vidState(g_VID.state.scissor,on,GL_SCISSOR_TEST)
#define vidStencilTest(on)  vidSingleRF(RENDER_STENCILTEST, on, GL_STENCIL_TEST)
#define vidStencilMask(on)  vidSingleRFCmd(RENDER_STENCILMASK, on, g_VID.gensetup.stencilmaskfunc(g_VID.state.stencilmaskpos), g_VID.gensetup.stencilmaskfunc(g_VID.state.stencilmaskneg))
#define vidFrontcull(on)  vidSingleRFCmd(RENDER_FRONTCULL, on, glCullFace(GL_FRONT), glCullFace(GL_BACK))
#define vidResetStencil() {g_VID.state.stencilmaskpos = BIT_ID_FULL32; g_VID.state.stencilmaskneg = 0;}

#define vidColor4fv(vec)    {glColor4fv((vec)); g_VID.drawsetup.rendercolor = (vec);}
#define vidColor4f(r, g, b, a)  {lxVector4Set(g_VID.drawsetup.rendercolorCache, r, g, b, a); g_VID.drawsetup.rendercolor = g_VID.drawsetup.rendercolorCache; glColor4fv(g_VID.drawsetup.rendercolorCache);}

#define vidAlphaFunc(fnc,value) {                 \
  if (g_VID.state.alpha.alphafunc != fnc || g_VID.state.alpha.alphaval != value){ \
  glAlphaFunc(fnc,value); g_VID.state.alpha.alphafunc = fnc; g_VID.state.alpha.alphaval = value;}\
}
#define vidDepthFunc(fnc) {                 \
  if (g_VID.state.depth.func != fnc){ \
  glDepthFunc(fnc); g_VID.state.depth.func = fnc;}\
}

#define vidFogColor(color) {\
  glFogfv(GL_FOG_COLOR,color);  \
  g_VID.state.fogcolor[0] = (color)[0]; \
  g_VID.state.fogcolor[1] = (color)[1]; \
  g_VID.state.fogcolor[2] = (color)[2]; \
  g_VID.state.fogcolor[3] = (color)[3]; \
}



//////////////////////////////////////////////////////////////////////////
// Lighting

#define vidLight(lightnum,on) {                 \
  if (on != g_VID.drawsetup.light[lightnum]){                   \
  if (on) glEnable(GL_LIGHT0+lightnum); else glDisable(GL_LIGHT0+lightnum); \
  g_VID.drawsetup.light[lightnum] = on;               \
  }                               \
}

// must be called right after setting camera matrix
// and before setting object matrix, active number of lights will be stored to
// g_VID.drawsetup.lightCnt

extern void Light_setGL(const struct Light_s *light,const int lightnum);

LUX_INLINE void vidSetLightsCG(int renderflag,int numFxLights, const void** fxLights)
{
  if (isFlagSet(renderflag,RENDER_LIT)){
    int d_n;
    g_VID.drawsetup.lightCnt = 1;
    if(isFlagSet(renderflag,RENDER_FXLIT)){
      g_VID.drawsetup.lightCnt+=numFxLights;
      d_n = 0;
      while(d_n < numFxLights){
        g_VID.drawsetup.lightPt[d_n+1] = (const struct Light_s*) fxLights[d_n];
        d_n++;
      }
    }
  }
  else
    g_VID.drawsetup.lightCnt = 0;
}

LUX_INLINE void vidSetLights(int renderflag,int numFxLights, const void** fxLights)
{
  if (isFlagSet(renderflag,RENDER_LIT)){
    int d_n;
    g_VID.drawsetup.lightCnt = 1;
    if (g_VID.drawsetup.purehwvertex){
      if(isFlagSet(renderflag,RENDER_FXLIT)){
        g_VID.drawsetup.lightCnt+=numFxLights;
        d_n=0;
        while(d_n < numFxLights){
          Light_setGL((const struct Light_s*)fxLights[d_n],d_n+1);
          d_n++;
        }
      }
    }
    else{
      if(isFlagSet(renderflag,RENDER_FXLIT)){
        g_VID.drawsetup.lightCnt+=numFxLights;
        d_n=0;
        while(d_n < numFxLights){
          Light_setGL((const struct Light_s*)fxLights[d_n],d_n+1);
          vidLight(d_n+1,LUX_TRUE);
          d_n++;
        }
        while (d_n < g_VID.drawsetup.lightCntLast){
          vidLight(d_n+1,LUX_FALSE);
          d_n++;
        }
        g_VID.drawsetup.lightCntLast = numFxLights;
      }
      else{
        d_n = 0;
        while (d_n < g_VID.drawsetup.lightCntLast){
          vidLight(d_n+1,LUX_FALSE);
          d_n++;
        }
        g_VID.drawsetup.lightCntLast = 0;
      }
    }
  }
  else{
    g_VID.drawsetup.lightCnt = 0;
  }
}

#define vidLightClear() \
  memset((void*)g_VID.drawsetup.lightPtGL,0,sizeof(struct Light_s *)*VID_MAX_LIGHTS);


//////////////////////////////////////////////////////////////////////////
// Shaders
#define vidGPUVertex(id) ((id == GPUPROG_V_ARB || id == GPUPROG_V_CG || id == GPUPROG_V_FIXED || id == GPUPROG_V_GLSL) ? LUX_TRUE : LUX_FALSE)
#define vidGPUFragment(id) ((id == GPUPROG_F_ARB || id == GPUPROG_F_CG || id == GPUPROG_F_FIXED || id == GPUPROG_F_GLSL) ? LUX_TRUE : LUX_FALSE)
#define vidGPUGeometry(id) ((id == GPUPROG_G_CG || id == GPUPROG_G_FIXED || id == GPUPROG_G_GLSL) ? LUX_TRUE : LUX_FALSE)

#define vidCgProfileisGLSL(profile) ((profile) == g_VID.cg.glslFragmentProfile || (profile) == g_VID.cg.glslVertexProfile)

LUX_INLINE void vidVertexProgram(VID_GPU_t type){
  if (type != g_VID.state.gpustate.vertex) {
    if (g_VID.state.gpustate.vertex == VID_ARB)   glDisable(GL_VERTEX_PROGRAM_ARB);
    else if (g_VID.state.gpustate.vertex == VID_GLSL) glUseProgramObjectARB(0);

    if (type == VID_ARB)          glEnable(GL_VERTEX_PROGRAM_ARB);

    g_VID.state.gpustate.vertex = type;
  }
}

LUX_INLINE void vidFragmentProgram(VID_GPU_t type){
  if (type != g_VID.state.gpustate.fragment) {
    if (g_VID.state.gpustate.fragment == VID_ARB)   glDisable(GL_FRAGMENT_PROGRAM_ARB);
    else if (g_VID.state.gpustate.fragment == VID_GLSL) glUseProgramObjectARB(0);

    if (type == VID_ARB)          glEnable(GL_FRAGMENT_PROGRAM_ARB);

    g_VID.state.gpustate.fragment = type;
  }
}

#ifdef LUX_VID_GEOSHADER

LUX_INLINE void vidGeometryProgram(VID_GPU_t type){
  LUX_DEBUGASSERT(type == VID_FIXED || type == VID_ARB);
  if (type != g_VID.state.gpustate.geometry) {
    if (g_VID.state.gpustate.geometry == VID_ARB) glDisable(GL_GEOMETRY_PROGRAM_NV);
    else if (g_VID.state.gpustate.geometry == VID_GLSL) glUseProgramObjectARB(0);

    if (type == VID_ARB)          glEnable(GL_GEOMETRY_PROGRAM_NV);
    g_VID.state.gpustate.geometry = type;
  }
}

#else
  #define vidGeometryProgram(type) {}
#endif


LUX_INLINE void vidBindVertexProg(const int id)
{
  if (g_VID.state.activeVertex==id)
    return ;

  glBindProgramARB(GL_VERTEX_PROGRAM_ARB, id);
  g_VID.state.activeVertex=id;
}
LUX_INLINE void  vidBindFragmentProg(const int id)
{
  if (g_VID.state.activeFragment==id)
    return ;

  glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, id);
  g_VID.state.activeFragment=id;
}

#define vidUnSetProgs()     {g_VID.state.activeFragment = 0; g_VID.state.activeVertex = 0; }
#define vidUnSetVertexProg()  g_VID.state.activeVertex = 0
#define vidUnSetFragmentProg()  g_VID.state.activeFragment = 0

//////////////////////////////////////////////////////////////////////////
// VertexArray

#define vidResetPointer()   memset(g_VID.drawsetup.attribs.pointers,255,sizeof(VIDVertexPointer_t)*VERTEX_ATTRIBS)
#define vidResetBuffers()   memcpy((void*)g_VID.state.activeBuffers,(void*)g_VID.defaultBuffers,sizeof(VIDBuffer_t*)*VID_BUFFERS)

LUX_INLINE void vidBindBufferArray(VIDBuffer_t* buffer)
{
  if (g_VID.state.activeBuffers[VID_BUFFER_VERTEX]==buffer)
    return ;

  glBindBufferARB(GL_ARRAY_BUFFER_ARB, buffer ? buffer->glID : 0);
  g_VID.state.activeBuffers[VID_BUFFER_VERTEX] = buffer;
}
LUX_INLINE void vidBindBufferIndex(VIDBuffer_t* buffer)
{
  if (g_VID.state.activeBuffers[VID_BUFFER_INDEX]==buffer)
    return ;

  glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, buffer ? buffer->glID : 0);
  g_VID.state.activeBuffers[VID_BUFFER_INDEX] = buffer;
}

LUX_INLINE void vidBindBufferTexture(VIDBuffer_t* buffer)
{
  if (g_VID.state.activeBuffers[VID_BUFFER_TEXTURE]==buffer)
    return ;

  glBindBufferARB(GL_TEXTURE_BUFFER_EXT, buffer ? buffer->glID : 0);
  g_VID.state.activeBuffers[VID_BUFFER_TEXTURE] = buffer;
}

LUX_INLINE void vidBindBufferFeedback(VIDBuffer_t* buffer)
{
  if (g_VID.state.activeBuffers[VID_BUFFER_FEEDBACK]==buffer)
    return;

  glBindBufferARB(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, buffer ? buffer->glID : 0);
  g_VID.state.activeBuffers[VID_BUFFER_FEEDBACK] = buffer;
}

LUX_INLINE void vidBindBufferPack(VIDBuffer_t* buffer)
{
  if (g_VID.state.activeBuffers[VID_BUFFER_PIXELINTO]==buffer)
    return;

  glBindBufferARB(GL_PIXEL_PACK_BUFFER, buffer ? buffer->glID : 0);
  g_VID.state.activeBuffers[VID_BUFFER_PIXELINTO] = buffer;
}

LUX_INLINE void vidBindBufferUnpack(VIDBuffer_t* buffer)
{
  if (g_VID.state.activeBuffers[VID_BUFFER_PIXELFROM]==buffer)
    return;

  glBindBufferARB(GL_PIXEL_UNPACK_BUFFER, buffer ? buffer->glID : 0);
  g_VID.state.activeBuffers[VID_BUFFER_PIXELFROM] = buffer;
}

LUX_INLINE void VIDBuffer_bindGL(VIDBuffer_t* buffer, VIDBufferType_t type)
{
  static const GLenum gltypes[VID_BUFFERS] = {
    GL_ARRAY_BUFFER_ARB,GL_ELEMENT_ARRAY_BUFFER_ARB,
    GL_PIXEL_PACK_BUFFER,GL_PIXEL_UNPACK_BUFFER,
    GL_TEXTURE_BUFFER_EXT,
    GL_TRANSFORM_FEEDBACK_BUFFER_EXT,
    GL_COPY_WRITE_BUFFER,
    GL_COPY_READ_BUFFER,
  };

  if (g_VID.state.activeBuffers[type]==buffer)
    return;

  glBindBufferARB(gltypes[type], buffer ? buffer->glID : 0);

  g_VID.state.activeBuffers[type] = buffer;
}



//////////////////////////////////////////////////////////////////////////
// Texture

#define vidMaterial(id)  ((id >= VID_OFFSET_MATERIAL ))

LUX_INLINE void vidSelectTexture(const int texture)
{
  if (g_VID.state.activeTex == texture){
    return ;
  }
  glActiveTextureARB( texture + GL_TEXTURE0_ARB);
  g_VID.state.activeTex = texture;

}

LUX_INLINE void vidSelectClientTexture(const int texture)
{
  if (g_VID.state.activeClientTex == texture){
    return ;
  }
  glClientActiveTextureARB( texture + GL_TEXTURE0_ARB);
  g_VID.state.activeClientTex = texture;

}

LUX_INLINE void vidTexturing(GLenum mode) {
  if (mode != g_VID.state.textargets[g_VID.state.activeTex] && g_VID.state.activeTex < g_VID.capTexUnits){
  if (g_VID.state.textargets[g_VID.state.activeTex] || !mode )  glDisable(g_VID.state.textargets[g_VID.state.activeTex]);
  if (mode) glEnable(mode);
  g_VID.state.textargets[g_VID.state.activeTex] = mode;}
}


#define vidBindTexture(texRID)\
  if (g_VID.state.textures[g_VID.state.activeTex]!=texRID ){\
  glBindTexture(ResData_getTextureBind(texRID));\
  g_VID.state.textures[g_VID.state.activeTex]=texRID;\
  }

#define vidForceBindTexture(texRID)\
  g_VID.state.textures[g_VID.state.activeTex] = -1;\
  vidBindTexture(texRID);


LUX_INLINE void vidTexCoordSource(VID_TexCoord_t type, VID_TexChannel_t channel)
{
  VIDTexCoordSource_t* LUX_RESTRICT source = &g_VID.state.texcoords[g_VID.state.activeTex];
  if (g_VID.state.activeTex >= g_VID.capTexUnits)
    return;

  if (type != source->type){
    // disable old
    if (source->type < 0){
      vidSelectClientTexture(g_VID.state.activeTex);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    else if (source->type > 0){
      glDisable(GL_TEXTURE_GEN_S);
      glDisable(GL_TEXTURE_GEN_T);
      glDisable(GL_TEXTURE_GEN_R);
      glDisable(GL_TEXTURE_GEN_Q);
    }

    if (type < 0){
      vidSelectClientTexture(g_VID.state.activeTex);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    else if (type && !g_VID.drawsetup.purehwvertex){
      switch(type) {
      case VID_TEXCOORD_TEXGEN_STRQ:
        glEnable(GL_TEXTURE_GEN_Q);
      case VID_TEXCOORD_TEXGEN_STR:
        glEnable(GL_TEXTURE_GEN_R);
      case VID_TEXCOORD_TEXGEN_ST:
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        break;
      case VID_TEXCOORD_TEXGEN_STQ:
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glEnable(GL_TEXTURE_GEN_Q);
      default:
        break;
      }
    }

    source->type = type;
  }
  source->channel = channel;
}

#ifdef _DEBUG
#define vidClearTexStages(needed, __i)  vidClearTexStagesF(needed)
LUX_INLINE void vidClearTexStagesF(int needed){
  int __i;
  for (__i = needed; __i < g_VID.state.lasttexStages; __i++ ){
    vidSelectTexture(__i);
    vidCheckError();
    vidTexturing(LUX_FALSE);
    vidCheckError();
    vidTexCoordSource(VID_TEXCOORD_NONE,VID_TEXCOORD_NOSET);
    vidCheckError();
  }
  g_VID.state.lasttexStages = needed;
}
#else
#define vidClearTexStages(needed, __i)  \
{ \
  for (__i = needed; __i < g_VID.state.lasttexStages; __i++ ){  \
  vidSelectTexture(__i);          \
  vidTexturing(LUX_FALSE);          \
  vidTexCoordSource(VID_TEXCOORD_NONE,VID_TEXCOORD_NOSET);  \
  }                     \
  g_VID.state.lasttexStages = needed;     \
}
#endif



LUX_INLINE void vidTexBlend(const VIDTexBlendHash hash,const VIDBlendColor_t blendmode,  VIDBlendAlpha_t alphamode,
                 const booln blendinvert, const int rgbscale, const int alphascale)
{
  if (g_VID.state.activeTex <= g_VID.capTexUnits &&
    hash != g_VID.state.texhash[g_VID.state.activeTex])
  {
    vidTexBlendSet(hash,blendmode,alphamode,blendinvert,rgbscale,alphascale);
  }
}

LUX_INLINE void vidTexBlendDefault(VIDTexBlendDefault_t mode)
{
  VIDTexBlendHash hash = g_VID.state.texhashdefaults[mode];

  if (g_VID.state.activeTex <= g_VID.capTexUnits &&
     hash != g_VID.state.texhash[g_VID.state.activeTex])
  {
    switch(mode)
    {
    case VID_TEXBLEND_MOD_MOD:
      vidTexBlendSet(hash,VID_MODULATE,VID_A_MODULATE,LUX_FALSE,0,0);
      return;
    case VID_TEXBLEND_MOD_REP:
      vidTexBlendSet(hash,VID_MODULATE,VID_A_REPLACE,LUX_FALSE,0,0);
      return;
    case VID_TEXBLEND_REP_REP:
      vidTexBlendSet(hash,VID_REPLACE,VID_A_REPLACE,LUX_FALSE,0,0);
      return;
    case VID_TEXBLEND_REP_MOD:
      vidTexBlendSet(hash,VID_REPLACE,VID_A_MODULATE,LUX_FALSE,0,0);
      return;
    case VID_TEXBLEND_PREV_REP:
      vidTexBlendSet(hash,VID_REPLACE_PREV,VID_A_REPLACE,LUX_FALSE,0,0);
      return;
    case VID_TEXBLEND_MOD_PREV:
      vidTexBlendSet(hash,VID_MODULATE,VID_A_REPLACE_PREV,LUX_FALSE,0,0);
      return;
    case VID_TEXBLEND_MOD2_PREV:
      vidTexBlendSet(hash,VID_MODULATE,VID_A_REPLACE_PREV,LUX_FALSE,1,0);
      return;
    case VID_TEXBLEND_MOD4_PREV:
      vidTexBlendSet(hash,VID_MODULATE,VID_A_REPLACE_PREV,LUX_FALSE,2,0);
      return;
    default:
      LUX_ASSERT(0);
      return;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
// Matrices
#define vidTexMatrixSet(matrix)     g_VID.shdsetup.texmatrix = matrix
#define vidLMTexMatrixSet(matrix)   g_VID.shdsetup.lmtexmatrix = matrix

#define vidWorldMatrix(matrix)      g_VID.drawsetup.worldMatrix = (matrix)
#define vidWorldMatrixIdentity()    g_VID.drawsetup.worldMatrix = lxMatrix44GetIdentity()
#define vidWorldScale(scale)      g_VID.drawsetup.renderscale = scale
#define vidWorldViewMatrixUpdate()    lxMatrix44MultiplyFull(g_VID.drawsetup.worldViewMatrix,g_VID.drawsetup.viewMatrix,g_VID.drawsetup.worldMatrix)
#define vidWorldViewProjMatrixUpdate()  lxMatrix44MultiplyFull(g_VID.drawsetup.worldViewProjMatrix,g_VID.drawsetup.viewProjMatrix,g_VID.drawsetup.worldMatrix)

#define vidLoadCameraGL()       {glMatrixMode(GL_MODELVIEW);glLoadMatrixf(g_VID.drawsetup.viewMatrix);}


//////////////////////////////////////////////////////////////////////////
// State Funcs

#define vidResetShaders() \
  memcpy(&g_VID.drawsetup.shaders,&g_VID.drawsetup.shadersdefault,sizeof(VIDShaders_t));

#endif
