// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __DRAWMESHINL_H__
#define __DRAWMESHINL_H__

#include "gl_drawmesh.h"

extern DrawData_t l_DrawData;


#define _dset_multipass_start \
  for(l_DrawData.curPassNum = 0; l_DrawData.curPassNum < l_DrawData.numPasses; l_DrawData.curPassNum++){  \
    if ( l_DrawData.curPassNum == 1 ){  \
      vidNoDepthMask(LUX_TRUE); \
    }

#define _dset_multipass_end   }vidNoDepthMask(LUX_FALSE);

LUX_INLINE void _dset_default(Mesh_t *mesh){
  l_DrawData.fogon = (g_VID.state.renderflag & RENDER_FOG);
  l_DrawData.origPasses = 1;
  l_DrawData.hwSkinning = LUX_TRUE;
}

  #define _dset_lock(mesh)
  #define _dset_unlock()

LUX_INLINE void _dset_initmesh(Mesh_t *mesh){
  l_DrawData.omeshtype = mesh->meshtype;
  l_DrawData.ovbo = mesh->vid.vbo;
  l_DrawData.oibo = mesh->vid.ibo;

  if (mesh->vertexData != mesh->origVertexData){
    mesh->meshtype = MESH_VA;
    mesh->vid.vbo = NULL;
    mesh->vid.ibo = NULL;
  }
}

LUX_INLINE void _dset_resetmesh(Mesh_t *mesh){
  mesh->vertexData = mesh->origVertexData;
  mesh->meshtype = l_DrawData.omeshtype;
  mesh->vid.vbo = l_DrawData.ovbo;
  mesh->vid.ibo = l_DrawData.oibo;
}

#define _dset_fixed_end()     glMatrixMode(GL_MODELVIEW);glPopMatrix()
#define _dset_shader_end()      if (!l_DrawData.shader->cgMode){glMatrixMode(GL_MODELVIEW);glPopMatrix();}

LUX_INLINE void _dset_setMatricesGL()
{
  //vidLoadCameraGL();
  vidSetLights(g_VID.state.renderflag,l_DrawData.numFxLights,l_DrawData.fxLights);

  if (g_VID.drawsetup.renderscale){
    lxMatrix44PreScale(g_VID.drawsetup.worldMatrixCache,g_VID.drawsetup.worldMatrix,g_VID.drawsetup.renderscale);

    g_VID.drawsetup.worldMatrix = g_VID.drawsetup.worldMatrixCache;
  }
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMultMatrixf(g_VID.drawsetup.worldMatrix);
}

LUX_INLINE void _dset_setMatricesCg()
{
  vidSetLightsCG(g_VID.state.renderflag,l_DrawData.numFxLights,l_DrawData.fxLights);

  if (g_VID.drawsetup.renderscale){
    lxVector3Scale(&g_VID.drawsetup.worldMatrixCache[0], &g_VID.drawsetup.worldMatrix[0], g_VID.drawsetup.renderscale[0]);
    lxVector3Scale(&g_VID.drawsetup.worldMatrixCache[4], &g_VID.drawsetup.worldMatrix[4], g_VID.drawsetup.renderscale[1]);
    lxVector3Scale(&g_VID.drawsetup.worldMatrixCache[8], &g_VID.drawsetup.worldMatrix[8], g_VID.drawsetup.renderscale[2]);
    lxVector3Copy(&g_VID.drawsetup.worldMatrixCache[12], &g_VID.drawsetup.worldMatrix[12]);

    g_VID.drawsetup.worldMatrix = g_VID.drawsetup.worldMatrixCache;
  }
  vidWorldViewProjMatrixUpdate();
  vidWorldViewMatrixUpdate();
  cgSetMatrixParameterfc(g_VID.cg.worldMatrix,g_VID.drawsetup.worldMatrix);
  cgSetMatrixParameterfc(g_VID.cg.worldViewMatrix,g_VID.drawsetup.worldViewMatrix);
  cgSetMatrixParameterfc(g_VID.cg.worldViewProjMatrix,g_VID.drawsetup.worldViewProjMatrix);
}

// vidFog(true) not necessary as noone can turn fog of
LUX_INLINE void _dset_fog_start(){
  if (l_DrawData.fogon && g_VID.state.renderflag & RENDER_BLEND){
    switch(g_VID.state.blend.blendmode){
        case VID_MODULATE:
          vidFogColor(g_VID.white);
          l_DrawData.resetfog = LUX_TRUE;
          break;
        case VID_AMODADD:
        case VID_ADD:
          vidFogColor(g_VID.black);
          l_DrawData.resetfog = LUX_TRUE;
          break;
        default:
          l_DrawData.resetfog = LUX_FALSE;
          break;
    }
  }
}

LUX_INLINE void _dset_fog_end(){
  if (l_DrawData.resetfog){
    vidFogColor(g_Background->fogcolor);
  }
}

LUX_INLINE void _dset_set_gpu(){
  vidVertexProgram(l_DrawData.vgpu);
  vidFragmentProgram(l_DrawData.fgpu);
  vidGeometryProgram(l_DrawData.ggpu);

  _dset_setMatricesGL();
}

LUX_INLINE  void _dset_color_prep(const Mesh_t *mesh){
  l_DrawData.numPasses = 1;
  // lightmap texture
  if (g_VID.shdsetup.lightmapRID >= 0){
    vidSelectTexture(0);
    vidTexBlendDefault(ResData_getTexture(g_VID.shdsetup.lightmapRID)->lmtexblend);
    vidBindTexture(g_VID.shdsetup.lightmapRID);
    vidTexCoordSource(VID_TEXCOORD_ARRAY,VID_TEXCOORD_LIGHTMAP);
    // texmat
    glMatrixMode(GL_TEXTURE);
    if (g_VID.shdsetup.lmtexmatrix) glLoadMatrixf(g_VID.shdsetup.lmtexmatrix);
    else              glLoadIdentity();
    vidTexturing(GL_TEXTURE_2D);
    l_DrawData.texStages = 1;
  }
  else
    l_DrawData.texStages = 0;
  l_DrawData.fgpu = VID_FIXED;
  l_DrawData.vgpu = VID_FIXED;
#ifdef LUX_VID_GEOSHADER
  l_DrawData.ggpu = VID_FIXED;
#endif
}

LUX_INLINE  void _dset_tex_start(const Mesh_t *mesh, const int texindex){
  // normal texture
  l_DrawData.numPasses = 1;
  //g_VID.activeLightmap = -1;
  vidSelectTexture(0);
  vidTexBlendDefault(VID_TEXBLEND_MOD_MOD);
  vidBindTexture(texindex);
  vidTexCoordSource(VID_TEXCOORD_ARRAY,VID_TEXCOORD_0);

  // texture matrix
  glMatrixMode(GL_TEXTURE);
  if (g_VID.shdsetup.texmatrix) glLoadMatrixf(g_VID.shdsetup.texmatrix);
  else              glLoadIdentity();
  vidTexturing(ResData_getTextureTarget(texindex));


  // lightmap texture
  if (g_VID.shdsetup.lightmapRID >= 0){
    vidSelectTexture(1);
    vidTexBlendDefault(ResData_getTexture(g_VID.shdsetup.lightmapRID)->lmtexblend);
    vidBindTexture(g_VID.shdsetup.lightmapRID);
    vidTexCoordSource(VID_TEXCOORD_ARRAY,VID_TEXCOORD_LIGHTMAP);
    // texmat
    if (g_VID.shdsetup.lmtexmatrix) glLoadMatrixf(g_VID.shdsetup.lmtexmatrix);
    else              glLoadIdentity();
    vidTexturing(GL_TEXTURE_2D);
    l_DrawData.texStages = 2;
  }
  else
    l_DrawData.texStages = 1;

  l_DrawData.fgpu = VID_FIXED;
  l_DrawData.vgpu = VID_FIXED;
#ifdef LUX_VID_GEOSHADER
  l_DrawData.ggpu = VID_FIXED;
#endif
}

LUX_INLINE void _dset_mat_prep(const int texindex)  {
  Material_t *material = ResData_getMaterial(texindex);
  l_DrawData.shader = ResData_getShader(material->shaders[0].resRIDUse);
  Material_setVID(material,0);
}

LUX_INLINE void _dset_shadermat_prep(int shadertype, int texindex){
  Shader_t* shader = ResData_getShader(g_VID.drawsetup.shaders.ids[shadertype]);
  l_DrawData.shader = shader;

  if (shader->numTotalParams)
    memset(g_VID.shdsetup.shaderparams,0,sizeof(float*)*shader->numTotalParams);

  g_VID.shdsetup.texmatrixStage[MATERIAL_TEX0] = NULL;
  g_VID.shdsetup.texgenStage[MATERIAL_TEX0] = NULL;
  g_VID.shdsetup.textures[MATERIAL_TEX0] = texindex;
}

LUX_INLINE void _dset_shadermat_override_prep(const int texindex) {
  Material_t *material = ResData_getMaterial(texindex);
  l_DrawData.shader = ResData_getShader(material->shaders[g_VID.drawsetup.shaders.override].resRIDUse);
  if (!l_DrawData.shader){
    _dset_shadermat_prep(VID_SHADER_COLOR,-1);
  }
  else Material_setVID(material,g_VID.drawsetup.shaders.override);
}



// l_DrawData.shader must be set;
LUX_INLINE  void _dset_shader_prep( Mesh_t *mesh){
  Shader_t *shader = l_DrawData.shader ;
  LUX_ASSERT(shader);

  if (isFlagSet(shader->shaderflag,SHADER_HWSKIN))
    l_DrawData.shaderSkinning = LUX_TRUE;
  else if (isFlagSet(shader->shaderflag,(SHADER_TEXGEN | SHADER_TANGENTS | SHADER_VGPU)))
    l_DrawData.hwSkinning = LUX_FALSE;

  // must do before worldMatrix contains scale
  if (isFlagSet(shader->shaderflag,SHADER_NEEDINVMATRIX)){
    lxMatrix44AffineInvertSIMD(g_VID.drawsetup.worldMatrixInv,g_VID.drawsetup.worldMatrix);
    if (g_VID.drawsetup.renderscale){
      lxVector3 invscale;
      const float *scale = g_VID.drawsetup.renderscale;
      float *wmatinv = g_VID.drawsetup.worldMatrixInv;

      lxVector3Set(invscale,1.0f/scale[0],1.0f/scale[1],1.0f/scale[2]);
      wmatinv[0] *= invscale[0];  wmatinv[4] *= invscale[0];  wmatinv[8] *= invscale[0];  wmatinv[12] *= invscale[0];
      wmatinv[1] *= invscale[1];  wmatinv[5] *= invscale[1];  wmatinv[9] *= invscale[1];  wmatinv[13] *= invscale[1];
      wmatinv[2] *= invscale[2];  wmatinv[6] *= invscale[2];  wmatinv[10] *= invscale[2]; wmatinv[14] *= invscale[2];
    }
  }

  if (shader->cgMode){
    _dset_setMatricesCg();
  }
  else{
    _dset_setMatricesGL();
  }

  l_DrawData.origPasses = l_DrawData.numPasses = shader->numPasses;

  l_DrawData.glpassFirst = l_DrawData.glpass = shader->glpassListHead;
  l_DrawData.vgpu = l_DrawData.glpass->vertexGPU;
  l_DrawData.fgpu = l_DrawData.glpass->fragmentGPU;
#ifdef LUX_VID_GEOSHADER
  l_DrawData.ggpu = l_DrawData.glpass->geometryGPU;
#endif

  l_DrawData.texStages = l_DrawData.glpass->numTexStages;
  if ((g_VID.state.renderflag & RENDER_LIT) && shader->colorPass > 1){
    vidLighting(LUX_FALSE);
    l_DrawData.togglelit = LUX_TRUE;
  }
  else
    l_DrawData.togglelit = LUX_FALSE;

#ifdef LUX_VID_COMPILEDARRAY
  mesh->nolock |= shader->nolock;
#endif
}



LUX_INLINE void  _dset_mat_pass_start(){
  if (l_DrawData.glpass){
    GLShaderPass_activate(l_DrawData.shader,l_DrawData.glpass,&l_DrawData.togglelit);
  }
}

LUX_INLINE void  _dset_mat_pass_end(){
  if (l_DrawData.glpass){
    GLShaderPass_deactivate(l_DrawData.glpass);
    l_DrawData.glpass = l_DrawData.glpass->next;
    if (l_DrawData.glpass == l_DrawData.glpassFirst){
      l_DrawData.texStages = 0;
    }
    else
      l_DrawData.texStages = l_DrawData.glpass->numTexStages;
  }
}

// needs numProjecters > 0, texstages,fpgu
// must come before skin
LUX_INLINE void _dset_proj_prep(const int numProjectors, const Projector_t **projectors){
  int n;

  l_DrawData.projStages = 0;
  l_DrawData.kickedprojectors = 0;
  if (!(g_VID.state.renderflag & RENDER_BLEND || l_DrawData.dnode->draw.state.renderflag & RENDER_PROJPASS) && l_DrawData.origPasses < 2 && !l_DrawData.fgpu){
    // find out how many we can do in first pass
    n = 0;
    while (n < g_VID.capTexUnits - l_DrawData.texStages && n < numProjectors && projectors[n]->nopass)
      n++;

    l_DrawData.projStages = n;
    l_DrawData.numPasses = numProjectors-l_DrawData.projStages;
  }
  else{
    l_DrawData.projStages = 0;
    l_DrawData.numPasses = numProjectors;
  }

  // hw skinning doesnt work with texgen
  l_DrawData.hwSkinning = LUX_FALSE;

  l_DrawData.numPasses += l_DrawData.origPasses;
}

static  int _dset_proj_start(Projector_t **projectors, const Mesh_t *mesh){
  int n;
  int passprep;
  if (l_DrawData.curPassNum > l_DrawData.origPasses-1){
    Projector_t *proj = projectors[l_DrawData.curPassNum-l_DrawData.origPasses+l_DrawData.projStages];
    const VIDBlend_t *blend = &proj->blend;

    vidBlending(LUX_TRUE);
    if (l_DrawData.vgpu){
      vidBindVertexProg(ResData_getGpuProgARB(g_VID.gensetup.vprogRIDs[VID_VPROG_PROJECTOR]));
      vidVertexProgram(VID_ARB);
    }
    else
      vidVertexProgram(VID_FIXED);
    vidFragmentProgram(VID_FIXED);
    vidGeometryProgram(VID_FIXED);

    vidBlend(blend->blendmode,blend->blendinvert);

    l_DrawData.attenuate = 0;
    // deal with alphamask texture
    if (l_DrawData.shader && l_DrawData.shader->alphatex){
      vidSelectTexture(0);
      vidTexBlendDefault(VID_TEXBLEND_PREV_REP);
      if (l_DrawData.shader->alphatex->id)
        l_DrawData.shader->alphatex->srcRID = g_VID.shdsetup.textures[l_DrawData.shader->alphatex->id];
      vidBindTexture(l_DrawData.shader->alphatex->srcRID);
      vidTexCoordSource(VID_TEXCOORD_ARRAY,l_DrawData.shader->alphatex->texchannel);
      l_DrawData.attenuate = 1;
    }
    passprep = Projector_prepScissor(proj,g_VID.state.viewport.bounds.sizes);
    // invisible
    if (passprep < 0){
      l_DrawData.kickedprojectors ++;
      return LUX_TRUE;
    }
    // we had scissor before, now none
    if (l_DrawData.revertscissor && !passprep){
      if (g_VID.state.viewport.bounds.fullwindow){
        vidScissor(LUX_FALSE);}
      else
        glScissor(lxVector4Unpack(g_VID.state.viewport.bounds.sizes));
    }
    l_DrawData.revertscissor = passprep;
    l_DrawData.texStages = Projector_stageGL(proj,l_DrawData.attenuate,LUX_TRUE);
    l_DrawData.glpass = NULL;
  }
  else{
    l_DrawData.projOffset = l_DrawData.texStages;
    for (n = 0; n < l_DrawData.projStages; n++)
      Projector_stageGL(projectors[n],l_DrawData.texStages+n,LUX_FALSE);
    l_DrawData.texStages += l_DrawData.projStages;
  }

  return LUX_FALSE;
}

LUX_INLINE void _dset_proj_end(){
  if (l_DrawData.revertscissor){
    if (g_VID.state.viewport.bounds.fullwindow){
      vidScissor(LUX_FALSE);}
    else
      glScissor(lxVector4Unpack(g_VID.state.viewport.bounds.sizes));
  }
  l_DrawData.numPasses -= l_DrawData.kickedprojectors;
}


// skin must be non NULL, hwSkinning, shaderSkinning, vgpu must be set
static  void _dset_skin_start(Mesh_t *mesh, SkinObject_t *skinobj){
  // do skinning
  // Hardware Skinning

  mesh->vertexData = SkinObject_run(skinobj,mesh->vertexData,mesh->vertextype, l_DrawData.hwSkinning);

  if ( skinobj->skin->hwskinning && l_DrawData.hwSkinning){
    int n;
    float *flt;
    float *inmat;
    float *outmat;
    lxVector4 pos;

    pos[3] = 1.0f;

    Mesh_setBlendGL(mesh);
    glEnableVertexAttribArrayARB(VERTEX_ATTRIB_BLENDINDICES);
    glEnableVertexAttribArrayARB(VERTEX_ATTRIB_BLENDWEIGHTS);

    if (!l_DrawData.shaderSkinning){
      vidBindVertexProg(ResData_getGpuProgARB(g_VID.gensetup.vprogRIDs[VID_VPROG_SKIN_WEIGHT_UNLIT+g_VID.drawsetup.lightCnt]));
      for (n = 0; n < g_VID.drawsetup.lightCnt; n++){
        lxVector3InvTransform(pos,g_VID.drawsetup.lightPt[n]->pos,g_VID.drawsetup.worldMatrix);
        glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB,n,pos);
      }
      l_DrawData.vgpu = VID_ARB;

      for (n = 0; n < skinobj->skin->numBones; n++){
        flt = skinobj->relMatricesT+skinobj->skin->boneIndices[n];
        glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB,(n*3),flt);
        glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB,(n*3)+1,flt+4);
        glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB,(n*3)+2,flt+8);
      }
    }
    else{
      g_VID.drawsetup.hwskin = LUX_TRUE;

      if (l_DrawData.shader->shaderflag & SHADER_VARB){
        for (n = 0; n < skinobj->skin->numBones; n++){
          flt = skinobj->relMatricesT+skinobj->skin->boneIndices[n];
          glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB,n,flt);
          glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB,n,flt+4);
          glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB,n,flt+8);
        }
      }
      else if (l_DrawData.shader->shaderflag & SHADER_VCG){
        // copy all used bones matrices because cg needs them all as one big chunk
        outmat = skinobj->skin->boneMatrices;
        for (n = 0; n < skinobj->skin->numBones; n++,outmat+=12){
          inmat = skinobj->relMatricesT+skinobj->skin->boneIndices[n];
          lxMatrix34CopySIMD(outmat,inmat);
        }
        //cgGLSetParameterArray4f(g_VID.cg.boneMatrices,0,skinobj->skin->numBones*3,skinobj->skin->boneMatrices);
        cgSetParameterValuefr(g_VID.cg.boneMatrices,g_VID.gensetup.hwbones*12,skinobj->skin->boneMatrices);
      }
      else if (l_DrawData.shader->shaderflag & SHADER_VGLSL){
        // copy all used bones matrices because glsl needs them all as one big chunk
        outmat = skinobj->skin->boneMatrices;
        for (n = 0; n < skinobj->skin->numBones; n++,outmat+=12){
          inmat = skinobj->relMatricesT+skinobj->skin->boneIndices[n];
          lxMatrix34CopySIMD(outmat,inmat);
        }
        // GLSL upload happens at shader binding so must tell info for later
        g_VID.drawsetup.skinMatrices = skinobj->skin->boneMatrices;
      }
    }


  } // Software Skinning
  else{
    l_DrawData.hwSkinning = LUX_FALSE;
  }

  vidCheckError();
}

LUX_INLINE void _dset_skin_end(){
  g_VID.drawsetup.hwskin = LUX_FALSE;
  if (l_DrawData.hwSkinning){
    glDisableVertexAttribArrayARB(VERTEX_ATTRIB_BLENDINDICES);
    glDisableVertexAttribArrayARB(VERTEX_ATTRIB_BLENDWEIGHTS);
  }
}

#endif
