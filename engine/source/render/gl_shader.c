// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "gl_shader.h"
#include "gl_render.h"
#include "gl_list3d.h"
#include "gl_window.h"
#include "gl_camlight.h"

// local
static ShaderStage_t l_ColorPass;
static ShaderStage_t l_SpecCubePass;
static ShaderStage_t l_SpecColorPass;

static enum32   l_texgenflag = SHADER_REFLECTMAP |  SHADER_NORMALMAP | SHADER_SPHEREMAP | SHADER_SCREENMAP | SHADER_EYELINMAP | SHADER_SKYMAP | SHADER_SUNREFMAP | SHADER_SUNNORMALMAP | SHADER_OBJLINMAP;

// Runtime setup of SHADER
// -----------------------
static float l_half[] = {0.5,0.5,0.5,0.5};




static void GLShaderPass_runTexGen(ShaderStage_t *stage){
  //light = NULL;
  static lxMatrix44SIMD mat;
  static lxMatrix44SIMD mat2;
  ShaderTexGen_t  *texgen;
  const Light_t *light;
  float *pFlt;
  int n;

  switch(stage->stageflag & l_texgenflag) {
  case SHADER_SCREENMAP:
    {
    lxMatrix44TransposeSIMD(mat,g_CamLight.camera->proj);


    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

    glTexGenfv(GL_S, GL_EYE_PLANE, &mat[0]);
    glTexGenfv(GL_T, GL_EYE_PLANE, &mat[4]);
    glTexGenfv(GL_Q, GL_EYE_PLANE, &mat[12]);

    vidTexCoordSource(VID_TEXCOORD_TEXGEN_STQ,VID_TEXCOORD_NOSET);

    glPopMatrix();

    glMatrixMode(GL_TEXTURE);
    glTranslatef(0.5,0.5,0.5);
    glScalef(0.5,0.5,0.5);
    glMatrixMode(GL_MODELVIEW);
    }
    break;

  case SHADER_OBJLINMAP:
    texgen = stage->texgen;
    pFlt = g_VID.shdsetup.texgenStage[stage->id];
    pFlt = pFlt ? pFlt : texgen->texgenmatrix;
    for(n = 0; n < 4; n++){
      if (texgen->enabledaxis[n]){
        glTexGeni(GL_S+n, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glTexGenfv(GL_S+n, GL_OBJECT_PLANE, &pFlt[4*n]);
      }
    }
    vidTexCoordSource(texgen->texgenmode,VID_TEXCOORD_NOSET);
    break;
  case SHADER_EYELINMAP:
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(g_VID.drawsetup.viewMatrix);

    texgen = stage->texgen;
    pFlt = g_VID.shdsetup.texgenStage[stage->id];
    pFlt = pFlt ? pFlt : texgen->texgenmatrix;
    for(n = 0; n < 4; n++){
      if (texgen->enabledaxis[n]){
        glTexGeni(GL_S+n, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
        glTexGenfv(GL_S+n, GL_EYE_PLANE, &pFlt[4*n]);
      }
    }
    vidTexCoordSource(texgen->texgenmode,VID_TEXCOORD_NOSET);

    glPopMatrix();
    break;
  case SHADER_SPHEREMAP:
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    vidTexCoordSource(VID_TEXCOORD_TEXGEN_ST,VID_TEXCOORD_NOSET);
    break;
  case SHADER_SUNREFMAP:
  case SHADER_SUNNORMALMAP:
    // mat = reference position
    light = g_VID.drawsetup.lightPt[stage->stageparam];
    if (g_VID.drawsetup.lightCnt && light){
      lxVector3Set(mat,0,0,1);
      lxVector3Copy(mat+3,light->pos);
      // TODO could need a subtraction to get direction ?
      lxVector3Normalized(mat+3);
      // rotate towards reference position
      lxMatrix44RotateAngle(mat2,mat+3,mat);
      lxMatrix44ClearTranslation(mat2);
      mat2[15]=1.0f;
      lxMatrix44TransposeSIMD(mat,mat2);

      // first get into worldspace
      lxMatrix44CopySIMD(mat2,g_VID.drawsetup.viewMatrixInv);
      lxMatrix44ClearTranslation(mat2);

      lxMatrix44Multiply1SIMD(mat,mat2);
    }else{
      lxMatrix44IdentitySIMD(mat);
      lxVector3Set(mat2,0.0f,0.0f, 1.0f - (2.0f*((stage->stageflag & SHADER_SUNREFMAP)!=0)));

      lxMatrix44SetTranslation(mat,mat2);
      mat2[2] = 0.0f;
      lxMatrix44SetScale(mat,mat2);
    }
    glMultMatrixf(mat);
    if (stage->stageflag & SHADER_SUNREFMAP)
      goto GLSHD_setRefMap;
    else
      goto GLSHD_setNrmlMap;
    break;
  case SHADER_SKYMAP:
    glMultMatrixf(g_VID.drawsetup.viewSkyrotMatrix);

  case SHADER_REFLECTMAP:
GLSHD_setRefMap:
    g_VID.drawsetup.setnormals = LUX_TRUE;
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    vidTexCoordSource(VID_TEXCOORD_TEXGEN_STR,VID_TEXCOORD_NOSET);
    break;
  case SHADER_NORMALMAP:
GLSHD_setNrmlMap:
    g_VID.drawsetup.setnormals = LUX_TRUE;
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
    vidTexCoordSource(VID_TEXCOORD_TEXGEN_STR,VID_TEXCOORD_NOSET);
    break;
  default:
    break;
  }
}

static void GLShaderPass_runTexSpecialCG(ShaderStage_t *stage){
  Texture_t *tex;

  switch(stage->srcType) {
    case TEX_CUBE_SKYBOX:
      if (g_Background->skybox){
        vidBindTexture(g_Background->skybox->cubeRID);
      }
      break;
    case TEX_3D_ATTENUATE:
      vidBindTexture(g_VID.gensetup.attenuate3dRID);
      break;
    case TEX_CUBE_NORMALIZE:
      vidBindTexture(g_VID.gensetup.normalizeRID);
      break;
    case TEX_2D_LIGHTMAP:
      if (g_VID.shdsetup.lightmapRID < 0){
        vidBindTexture(g_VID.gensetup.whiteRID);}
      else{
        vidBindTexture(g_VID.shdsetup.lightmapRID);
        if (stage->stageflag & SHADER_LMRGBSCALE)
          stage->rgbscale = ResData_getTexture(g_VID.shdsetup.lightmapRID)->lmrgbscale;
      }
      break;
    case TEX_CUBE_SPECULAR:
      vidBindTexture(g_VID.gensetup.specularRID);
      break;
    case TEX_CUBE_DIFFUSE:
      vidBindTexture(g_VID.gensetup.diffuseRID);
      break;
    case TEX_DEPTH:
      tex = ResData_getTexture(stage->srcRID);
      vidBindTexture(stage->srcRID);

      if ((stage->stageflag & SHADER_DEPTHCOMPARE) && !(tex->attributes & TEX_ATTR_DEPTHCOMPARE)){
        glTexParameteri(tex->target,GL_TEXTURE_COMPARE_MODE_ARB,GL_COMPARE_R_TO_TEXTURE);
        tex->attributes  |= TEX_ATTR_DEPTHCOMPARE;
      }
      else if ((stage->stageflag & SHADER_DEPTHNOCOMPARE) && (tex->attributes & TEX_ATTR_DEPTHCOMPARE)){
        glTexParameteri(tex->target,GL_TEXTURE_COMPARE_MODE_ARB,GL_NONE);
        tex->attributes &= ~TEX_ATTR_DEPTHCOMPARE;
      }

      break;
    default:
      break;
  }
}

static void GLShaderPass_runTexSpecial(ShaderStage_t *stage, int *texmatrix, int *lmtexmat){
  static lxMatrix44 mat;
  Texture_t *tex;

  switch(stage->srcType) {
    case TEX_CUBE_SKYBOX:
      if (g_Background->skybox){
        vidBindTexture(g_Background->skybox->cubeRID);
        vidTexturing(GL_TEXTURE_CUBE_MAP_ARB);
      }
      *texmatrix = LUX_FALSE;
      break;
    case TEX_3D_ATTENUATE:
      vidBindTexture(g_VID.gensetup.attenuate3dRID);
      vidTexturing(GL_TEXTURE_3D);
      *texmatrix = LUX_FALSE;
      break;
    case TEX_CUBE_NORMALIZE:
      vidBindTexture(g_VID.gensetup.normalizeRID);
      vidTexturing(GL_TEXTURE_CUBE_MAP_ARB);
      *texmatrix = LUX_FALSE;
      break;
    case TEX_2D_LIGHTMAP:
      if (g_VID.shdsetup.lightmapRID < 0){
        vidBindTexture(g_VID.gensetup.whiteRID);}
      else{
        vidBindTexture(g_VID.shdsetup.lightmapRID);
        if (stage->stageflag & SHADER_LMRGBSCALE)
          stage->rgbscale = ResData_getTexture(g_VID.shdsetup.lightmapRID)->lmrgbscale;
      }
      vidTexturing(GL_TEXTURE_2D);
      *texmatrix = LUX_FALSE;
      *lmtexmat = LUX_TRUE;
      break;
    case TEX_CUBE_SPECULAR:
      vidBindTexture(g_VID.gensetup.specularRID);
      vidTexturing(GL_TEXTURE_CUBE_MAP_ARB);
      break;
    case TEX_CUBE_DIFFUSE:
      vidBindTexture(g_VID.gensetup.diffuseRID);
      vidTexturing(GL_TEXTURE_CUBE_MAP_ARB);
      break;
    case TEX_DEPTH:
      tex = ResData_getTexture(stage->srcRID);
      vidBindTexture(stage->srcRID);
      vidTexturing(tex->target);

      if ((stage->stageflag & SHADER_DEPTHCOMPARE) && !(tex->attributes & TEX_ATTR_DEPTHCOMPARE)){
        glTexParameteri(tex->target,GL_TEXTURE_COMPARE_MODE_ARB,GL_COMPARE_R_TO_TEXTURE);
        tex->attributes  |= TEX_ATTR_DEPTHCOMPARE;
      }
      else if ((stage->stageflag & SHADER_DEPTHNOCOMPARE) && (tex->attributes & TEX_ATTR_DEPTHCOMPARE)){
        glTexParameteri(tex->target,GL_TEXTURE_COMPARE_MODE_ARB,GL_NONE);
        tex->attributes &= ~TEX_ATTR_DEPTHCOMPARE;
      }

      break;
    default:
      break;
  }
}

static void GLShaderPass_runTexParam(ShaderStage_t *stage)
{
  const Light_t *light;
  ShaderParam_t *param;

  lxListNode_forEach(stage->paramListHead,param)
    switch(param->type){
    case SHADER_PARAM_CAMDIR :
      lxVector3Transform(param->vector,g_CamLight.camera->fwd,g_VID.drawsetup.worldMatrixInv);
      lxVector3Normalized(param->vector);
      lxVector3ScaledAdd(param->vector,l_half,0.5f,param->vector);
      break;
    case SHADER_PARAM_LIGHTDIR :
      light = g_VID.drawsetup.lightPt[param->upvalue];
      if (light){
        lxVector3Transform(param->vector,light->pos,g_VID.drawsetup.worldMatrixInv);
        lxVector3Normalized(param->vector);
        lxVector3ScaledAdd(param->vector,l_half,0.5f,param->vector);
      }
      break;
    case SHADER_PARAM_LIGHTCOLOR:
      light = g_VID.drawsetup.lightPt[param->upvalue];
      if (light)
        lxVector4Copy(param->vector,light->diffuse);
      break;
    case SHADER_PARAM_LIGHTAMBIENT:
      light = g_VID.drawsetup.lightPt[param->upvalue];
      if (light)
        lxVector4Copy(param->vector,light->ambient);
      break;
    default:
      break;
    }

    g_VID.shdsetup.colors[stage->id] = param->vector;
  lxListNode_forEachEnd(stage->paramListHead,param);
}


LUX_INLINE void GpuParam_MaxtrixVector(float* outvec, int mode, float x, float y, float z, float w)
{
  lxVector4 temp;

  switch(mode)
  {
  default:
  case 0: // straight output
    lxVector4Set(outvec,x,y,z,w);
    break;
  case 1: // worldmatrix
    lxVector4Set(temp,x,y,z,w);
    lxVector4TransformT(outvec,temp,g_VID.drawsetup.worldMatrix);
    break;
  case 2: // texmatrix
    if (g_VID.shdsetup.texmatrix){
      lxVector4Set(temp,x,y,z,w);
      lxVector4TransformT(outvec,temp,g_VID.shdsetup.texmatrix);
    }
    else{
      lxVector4Set(outvec,x,y,z,w);
    }
    break;
  case 3: // lmtexmatrix
    if (g_VID.shdsetup.lmtexmatrix){
      lxVector4Set(temp,x,y,z,w);
      lxVector4TransformT(outvec,temp,g_VID.shdsetup.lmtexmatrix);
    }
    else{
      lxVector4Set(outvec,x,y,z,w);
    }
    break;
  }
}



static void GLShaderPass_runGpuParam(ShaderStage_t *stage, VID_GPUprogram_t mode){
  int i;
  const Light_t *light;
  ShaderParam_t *param;
  Texture_t   *tex;
  float *pFlt;
  float *bufferstart = rpoolcurrent();
  float *temp = NULL;

  static ShaderParam_t  *vectorparams[VID_MAX_SHADERPARAMS+1];
  static ShaderParam_t  *arrayparams[VID_MAX_SHADERPARAMS+1];
  static float      *arrayvecs[VID_MAX_SHADERPARAMS+1];

  ShaderParam_t **curvec = vectorparams;
  ShaderParam_t **curarray = arrayparams;
  float   **curarrayvec = arrayvecs;

  lxListNode_forEach(stage->paramListHead,param)
    switch(param->type){
    default:
    case SHADER_PARAM_VALUE:

      pFlt = g_VID.shdsetup.shaderparams[param->totalid];
      param->vectorp = (pFlt) ? pFlt : param->vector;

      // turn to objectspace
      if (param->upvalue){
        temp = rpoolmalloc(sizeof(lxVector4));
        // transforms into temporary cache
        lxVector4Transform(temp,param->vectorp,g_VID.drawsetup.worldMatrixInv);
        param->vectorp = temp;
      }
      *(curvec++) = param;
      break;
    case SHADER_PARAM_CAMUP :
      lxVector3Transform(param->vector,g_CamLight.camera->up,g_VID.drawsetup.worldMatrixInv);
      *(curvec++) = param;
      break;
    case SHADER_PARAM_CAMPOS :
      lxVector3Transform(param->vector,g_CamLight.camera->pos,g_VID.drawsetup.worldMatrixInv);
      *(curvec++) = param;
      break;
    case SHADER_PARAM_CAMDIR :
      lxVector3TransformRot(param->vector,g_CamLight.camera->fwd,g_VID.drawsetup.worldMatrixInv);
      lxVector3Normalized(param->vector);
      *(curvec++) = param;
      break;
    case SHADER_PARAM_RANDOM:
      param->vector[0]=frandPointerSeed(g_VID.drawsetup.curnode,123)*param->vector[3];
      param->vector[2]=frandPointerSeed(g_VID.drawsetup.curnode,573)*param->vector[3];
      param->vector[1]=frandPointerSeed(g_VID.drawsetup.curnode,17)*param->vector[3];
      *(curvec++) = param;
      break;
    case SHADER_PARAM_TIME:
      param->vector[0]=param->vector[3]*g_LuxTimer.timef;
      param->vector[1]=lxFastSin(param->vector[0]);
      param->vector[2]=lxFastCos(param->vector[0]);
      *(curvec++) = param;
      break;
    case SHADER_PARAM_LIGHTRANGE:
      light = g_VID.drawsetup.lightPt[param->upvalue];
      if (light){
        lxVector4Set(param->vector,light->range,1.0f/light->range,light->range*0.5f,1/light->rangeSq);
        *(curvec++) = param;
      }
      break;
    case SHADER_PARAM_LIGHTDIR :
      light = g_VID.drawsetup.lightPt[param->upvalue];
      if (light){
        lxVector3Transform(param->vector,light->pos,g_VID.drawsetup.worldMatrixInv);
        lxVector3Normalized(param->vector);
        *(curvec++) = param;
      }
      break;
    case SHADER_PARAM_LIGHTPOS :
      light = g_VID.drawsetup.lightPt[param->upvalue];
      if (light){
        lxVector3Transform(param->vector,light->pos,g_VID.drawsetup.worldMatrixInv);
        *(curvec++) = param;
      }
      break;
    case SHADER_PARAM_LIGHTCOLOR :
      light = g_VID.drawsetup.lightPt[param->upvalue];
      if (light){
        lxVector4Copy(param->vector,light->diffuse);
        *(curvec++) = param;
      }
      break;
    case SHADER_PARAM_LIGHTAMBIENT :
      light = g_VID.drawsetup.lightPt[param->upvalue];
      if (light){
        lxVector4Copy(param->vector,light->ambient);
        *(curvec++) = param;
      }
      break;
    case SHADER_PARAM_TEXCONST :
      pFlt = g_VID.shdsetup.colors[MATERIAL_TEX0+param->upvalue];
      if (pFlt){
        lxVector4Copy(param->vector,pFlt);
        *(curvec++) = param;
      }
      break;
    case SHADER_PARAM_TEXSIZE :
      tex = ResData_getTexture(g_VID.state.textures[param->upvalue]);
      if (tex){
        lxVector3Set(param->vector,tex->sarr3d.sz.width*param->vector[3],tex->sarr3d.sz.height*param->vector[3],tex->sarr3d.sz.depth*param->vector[3]);
        *(curvec++) = param;
      }
      break;
    case SHADER_PARAM_TEXSIZEINV :
      tex = ResData_getTexture(g_VID.state.textures[param->upvalue]);
      if (tex){
        lxVector3Set(param->vector,1.0f/(tex->sarr3d.sz.width*param->vector[3]),1.0f/(tex->sarr3d.sz.height*param->vector[3]),1.0f/(tex->sarr3d.sz.depth*param->vector[3]));
        *(curvec++) = param;
      }
      break;
    case SHADER_PARAM_TEXLMSCALE:
      tex = ResData_getTexture(g_VID.state.textures[param->upvalue]);
      if (tex){
        float scales[3] = {1.0f,2.0f,4.0f};
        param->vector[0] = scales[tex->lmrgbscale];
        *(curvec++) = param;
      }
      break;
    case SHADER_PARAM_TEXMAT0 :
    case SHADER_PARAM_TEXMAT1 :
    case SHADER_PARAM_TEXMAT2 :
    case SHADER_PARAM_TEXMAT3 :
      pFlt = g_VID.shdsetup.texgenStage[MATERIAL_TEX0+param->upvalue];
      if (pFlt){
        pFlt += (param->type-SHADER_PARAM_TEXMAT0);

        GpuParam_MaxtrixVector(param->vector,param->upvalue2,pFlt[0],pFlt[4],pFlt[8],pFlt[12]);

        *(curvec++) = param;
      }
      break;
    case SHADER_PARAM_TEXGEN0 :
    case SHADER_PARAM_TEXGEN1 :
    case SHADER_PARAM_TEXGEN2 :
    case SHADER_PARAM_TEXGEN3 :
      pFlt = g_VID.shdsetup.texgenStage[SHADER_PARAM_TEXGEN0+param->upvalue];
      if (pFlt){
        pFlt += (param->type-SHADER_PARAM_TEXGEN0)*4;

        GpuParam_MaxtrixVector(param->vector,param->upvalue2,lxVector4Unpack(pFlt));

        *(curvec++) = param;
      }
      break;
    case SHADER_PARAM_ARRAY:
      pFlt = g_VID.shdsetup.shaderparams[param->totalid];
      pFlt = pFlt ? pFlt : param->vectorp;

      *(curarrayvec++) = pFlt;
      *(curarray++) = param;
      break;

    }
  lxListNode_forEachEnd(stage->paramListHead,param);


  *curvec = NULL;
  *curarray = NULL;

  curvec = vectorparams;
  curarray = arrayparams;
  curarrayvec = arrayvecs;

  switch(mode){
  case VID_GPU_CG:
    while(param = *(curvec++)){
      //cgSetParameter4fv(param->cgparam, param->vectorp);
      cgSetParameter4fv(param->cgparam, param->vectorp);
      vidCgCheckError();
    }
    while(param = *(curarray++)){
      pFlt =  *(curarrayvec++);
      cgSetParameterValuefr(param->cgparam,param->upvalue*4,pFlt);
      //cgSetParameterArray4f(param->cgparam,0,param->upvalue,pFlt);
      vidCgCheckError();
    }
    vidCgCheckError();
    break;
  case VID_GPU_ARB:
    while(param = *(curvec++)){
      glProgramLocalParameter4fvARB(stage->srcType, param->id, param->vectorp);
    }
    while(param = *(curarray++)){
      pFlt =  *(curarrayvec++);
      for (i=0; i < param->upvalue; i++,pFlt+=4){
        glProgramLocalParameter4fvARB(stage->srcType, param->id+i,pFlt);
      }
    }
    break;
  case VID_GPU_GLSL:
    break;
  }

  rpoolsetbegin(bufferstart);
}

LUX_INLINE void GLShaderPass_setState(const GLShaderPass_t *pass)
{
  enum32 rf = g_VID.state.renderflag;

  if (pass->tangents){
    glEnableVertexAttribArrayARB(VERTEX_ATTRIB_TANGENT);
    g_VID.drawsetup.attribs.needed |= 1<<VERTEX_ATTRIB_TANGENT;
  }
  if (pass->blend.blendmode){
    vidBlend(pass->blend.blendmode,pass->blend.blendinvert);
  }
  /*
  if (pass->custattribs){
    if (pass->custattribs & (1<<VERTEX_ATTRIB_ATTR5)){
      glEnableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR5);
    }
    if (pass->custattribs & (1<<VERTEX_ATTRIB_ATTR6)){
      glEnableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR6);
    }
    if (pass->custattribs & (1<<VERTEX_ATTRIB_ATTR12)){
      glEnableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR12);
    }
    if (pass->custattribs & (1<<VERTEX_ATTRIB_ATTR13)){
      glEnableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR13);
    }
    if (pass->custattribs & (1<<VERTEX_ATTRIB_ATTR15)){
      glEnableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR15);
    }
  }
  */

  rf |= pass->rflagson;
  rf &= pass->rflagsoff;
  if (rf != g_VID.state.renderflag){
    VIDRenderFlag_setGL(rf);
  }
}

void GLShaderPass_activateCG(const GLShaderPass_t *pass, int *togglelit)
{
  int i;
  int clr,texunit;
  ShaderStage_t *stage;
  GLShaderStage_t *glstage;

  vidCheckError();

  clr = 0;

  GLShaderPass_setState(pass);
  vidCheckError();


  glstage = pass->glstageListHead;
  for (i = 0; i < pass->numStages; i++){
    stage = glstage->stage;
    switch (stage->stagetype){
      /* TEXTURE */case SHADER_STAGE_TEX:
      { // texture:
        texunit = i+clr;
        vidSelectTexture(  texunit);
        if (stage->id)
          stage->srcRID = g_VID.shdsetup.textures[stage->colorid];

        if (stage->stageflag & SHADER_SPECIAL)
        {
          GLShaderPass_runTexSpecialCG(stage);
        }
        else{
          vidBindTexture(stage->srcRID);
        }

        vidCheckError();


        // do texcoords/texmatrix
        if (texunit < g_VID.capTexUnits){
          if (stage->texchannel > VID_TEXCOORD_NOSET) {
            vidTexCoordSource(VID_TEXCOORD_ARRAY,stage->texchannel);
          }
          else{
            vidTexCoordSource(VID_TEXCOORD_NONE,VID_TEXCOORD_NOSET);
          }
        }// end texcoords/texmatrix

        vidCheckError();

      }
      break;
      /* COLOR */ case SHADER_STAGE_COLOR:
      { // color:
        if (g_VID.shdsetup.colors[stage->id])
          glColor4fv( g_VID.shdsetup.colors[stage->id] );
        clr--;
      }
      break;
      /* GPU */ case SHADER_STAGE_GPU:
      {
        CGprogram cgprog = NULL;

        if (isFlagSet(stage->stageflag,SHADER_NORMALS)){
          g_VID.drawsetup.setnormals = LUX_TRUE;
        }
        if (isFlagSet(stage->stageflag,SHADER_TEXBARRIER)){
          glTextureBarrierNV();
        }

        if (!pass->combo)
          switch (stage->srcType){
          case GPUPROG_V_CG:
            vidUnSetVertexProg();

            if (g_VID.drawsetup.hwskin)
              cgprog = ResData_getGpuProgCG(stage->gpuinfo->srcSkinRIDs[g_VID.drawsetup.lightCnt]);
            else
              cgprog = ResData_getGpuProgCG(stage->gpuinfo->srcRIDs[g_VID.drawsetup.lightCnt]);

            cgGLBindProgram(cgprog);
            break;
          case GPUPROG_F_CG:
            vidUnSetFragmentProg();

            if (g_VID.state.renderflag & RENDER_FOG)
              cgprog = ResData_getGpuProgCG(stage->gpuinfo->srcSkinRIDs[g_VID.drawsetup.lightCnt]);
            else
              cgprog = ResData_getGpuProgCG(stage->gpuinfo->srcRIDs[g_VID.drawsetup.lightCnt]);

            cgGLBindProgram(cgprog);
            break;
          case GPUPROG_V_FIXED:
            break;
          case GPUPROG_F_FIXED:
            break;
          case GPUPROG_G_FIXED:
            break;
          }

        if (stage->paramListHead)
          GLShaderPass_runGpuParam(stage,VID_GPU_CG);

        if (cgprog)
          cgUpdateProgramParameters(cgprog);

        vidCgCheckError();
        clr--;
      }
    }

    glstage = glstage->next;
  }

  if(pass->combo){
    CGprogram cgprog = pass->cgcomboprogs[
      g_VID.drawsetup.lightCnt +
        (isFlagSet(g_VID.state.renderflag,RENDER_FOG)*GL_GPUCOMBO_FOGGED) +
        (g_VID.drawsetup.hwskin * GL_GPUCOMBO_SKINNED)];

      cgGLBindProgram(cgprog);
      cgUpdateProgramParameters(cgprog);
  }

  vidVertexProgram(pass->vertexGPU);
  vidFragmentProgram(pass->fragmentGPU);
  vidGeometryProgram(pass->geometryGPU);
  vidCheckError();

  vidClearTexStages(pass->numStages+clr,i);

  vidCheckError();
}



void GLShaderPass_activateFIXEDARB(const GLShaderPass_t *pass, int *togglelit)
{
  int i;
  int clr,texunit;
  int texmatrix,lmtexmat;
  ShaderStage_t *stage;
  GLShaderStage_t *glstage;

  vidCheckError();
  clr = 0;

  GLShaderPass_setState(pass);
  vidCheckError();

  glstage = pass->glstageListHead;
  for (i = 0; i < pass->numStages; i++){
    stage = glstage->stage;
    switch (stage->stagetype){
/* TEXTURE */case SHADER_STAGE_TEX:
      { // texture:
        texunit = i+clr;
        vidSelectTexture(  texunit);
        if (stage->id)
          stage->srcRID = g_VID.shdsetup.textures[stage->colorid];

        if (stage->paramListHead)
          GLShaderPass_runTexParam(stage);

        texmatrix = LUX_TRUE;
        lmtexmat = LUX_FALSE;
        if (stage->stageflag & SHADER_SPECIAL)
        {
          GLShaderPass_runTexSpecial(stage,&texmatrix,&lmtexmat);
        }
        else{
          vidBindTexture(stage->srcRID);
          if (pass->fragmentGPU == VID_FIXED)
            vidTexturing(ResData_getTextureTarget(stage->srcRID));
        }

        vidCheckError();


        // do texcoords/texmatrix
        if (texunit < g_VID.capTexUnits){
        if (g_VID.shdsetup.texmatrixStage[stage->id]){
          glMatrixMode(GL_TEXTURE);
          glLoadMatrixf(g_VID.shdsetup.texmatrixStage[stage->id]);
        }
        else if (!g_VID.capIsCrap){
          glMatrixMode(GL_TEXTURE);
          glLoadIdentity();
        }
        else{
          // on crap hardware...
          glMatrixMode(GL_TEXTURE);
          glLoadIdentity();
          glScalef(1,1,1);
          glTranslatef(0,0,0);
          glRotatef(0,0,0,0);
        }
        if (texmatrix &&  g_VID.shdsetup.texmatrix)
          glMultMatrixf(g_VID.shdsetup.texmatrix);
        else if (lmtexmat && g_VID.shdsetup.lmtexmatrix)
          glMultMatrixf(g_VID.shdsetup.lmtexmatrix);

        vidCheckError();

        if (stage->stageflag & SHADER_TEXGEN){
          GLShaderPass_runTexGen(stage);
        }
        else if (stage->texchannel > VID_TEXCOORD_NOSET) {
          vidTexCoordSource(VID_TEXCOORD_ARRAY,stage->texchannel);
        }
        else
          vidTexCoordSource(VID_TEXCOORD_NONE,VID_TEXCOORD_NOSET);
        }// end texcoords/texmatrix

        // Texture_t Environment

        if (g_VID.shdsetup.colors[stage->colorid]){
          glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,g_VID.shdsetup.colors[stage->colorid]);
        }

        // do texenv default is modulate
        vidTexBlend(glstage->texblend,glstage->blend.blendmode,glstage->blend.alphamode,glstage->blend.blendinvert,stage->rgbscale,stage->alphascale);

        vidCheckError();

      }
      break;
/* COLOR */ case SHADER_STAGE_COLOR:
      { // color:
        if (g_VID.shdsetup.colors[stage->id])
          glColor4fv( g_VID.shdsetup.colors[stage->id] );
        clr--;
      }
      break;
/* GPU */ case SHADER_STAGE_GPU:
      {

        if (isFlagSet(stage->stageflag,SHADER_NORMALS)){
          g_VID.drawsetup.setnormals = LUX_TRUE;
        }


        if (!pass->combo)
          switch (stage->srcType){
          case GPUPROG_V_ARB:
            if (g_VID.drawsetup.hwskin)
              vidBindVertexProg(ResData_getGpuProgARB(stage->gpuinfo->srcSkinRIDs[g_VID.drawsetup.lightCnt]));
            else
              vidBindVertexProg(ResData_getGpuProgARB(stage->gpuinfo->srcRIDs[g_VID.drawsetup.lightCnt]));
            break;
          case GPUPROG_F_ARB:
            if (g_VID.state.renderflag & RENDER_FOG)
              vidBindFragmentProg(ResData_getGpuProgARB(stage->gpuinfo->srcSkinRIDs[g_VID.drawsetup.lightCnt]));
            else
              vidBindFragmentProg(ResData_getGpuProgARB(stage->gpuinfo->srcRIDs[g_VID.drawsetup.lightCnt]));
            break;
          case GPUPROG_V_FIXED:
            break;
          case GPUPROG_F_FIXED:
            break;
          case GPUPROG_G_FIXED:
            break;
          }

        if (stage->paramListHead)
          GLShaderPass_runGpuParam(stage,pass->combo ? VID_GPU_GLSL : VID_GPU_ARB);

        clr--;
      }
      //for colorpass we do nothing just rerender the whole thing
      // using its primary color
      case SHADER_STAGE_COLORPASS:
        if (*togglelit){
          vidLighting(LUX_TRUE);
          *togglelit = LUX_FALSE;
        }
      break;
    }

    glstage = glstage->next;
  }

  vidVertexProgram(pass->vertexGPU);
  vidFragmentProgram(pass->fragmentGPU);
  vidGeometryProgram(pass->geometryGPU);
  vidCheckError();

  vidClearTexStages(pass->numStages+clr,i);

  vidCheckError();
};

// Preprocessing of SHADER
// -----------------------


//////////////////////////////////////////////////////////////////////////
// Shader Stage Setup

int Shader_prepareStages(Shader_t *shader, int *hasaddpassfirst)
{
  ShaderStage_t *stage,*prevstage,*nextstage;
  ShaderStage_t *stagesListHead = NULL;
  int i,n=0;
  int tofirst = 0;
  int colorstages = 0;
  int texstages = 0;
  int colorpassoffset = 0;
  int gpuprogs = 0;
  int addpassfirst = 0;

  int colormodall = 0;


  // as we are in compile mode, we allow only one pair of gpu programs, so bring them to the front
  for (i = 0; i < shader->numStages; i++){
    stage = shader->stages[i];
    lxListNode_init(stage);
    if (stage->stagetype == SHADER_STAGE_GPU){
      lxListNode_addFirst(stagesListHead,stage);
      gpuprogs++;
    }
    else
      lxListNode_addLast(stagesListHead,stage);
  }
  stage = stagesListHead;
  // reordered with gpu in front
  for (i = 0; i < shader->numStages; i++){
    shader->stages[i] = stage;
    stage = stage->next;
  }


  // all stages/passes (a OP b OP c ..)*v WARNING when firstblend is not REPLACE you cant do a "colormodpass"
  if (shader->stages[gpuprogs]->stagetype == SHADER_STAGE_TEX)
    switch(shader->stages[0]->blendmode){
    case VID_REPLACE:
      tofirst = LUX_TRUE;
      break;
    case VID_ADD:
      tofirst = LUX_TRUE;
      addpassfirst = LUX_TRUE;
      break;
    default:
      tofirst = LUX_FALSE;
      break;
    }
  else if( shader->numStages > gpuprogs+1 && shader->stages[gpuprogs+1]->stagetype == SHADER_STAGE_TEX )
    switch(shader->stages[gpuprogs+1]->blendmode){
    case VID_REPLACE:
      tofirst = 2;
      break;
    default:
      tofirst = LUX_FALSE;
      break;
    }
  // modadds become first when we dont have these two extensions, else
  // they can stay where they are
  if ((GLEW_NV_texture_env_combine4 || GLEW_ATI_texture_env_combine3))
    tofirst = LUX_FALSE;

  // now start the show
  // we store into a new list without the gpuprogams
  stagesListHead = NULL;
  colormodall = 0;
  prevstage = NULL;
  for ( i = gpuprogs; i < shader->numStages; i++){
    // stage
    stage = shader->stages[i];

    // prevstage, ignore gpu
    if (i > gpuprogs)
      prevstage = shader->stages[i-1];

    // nextstage
    if (i < shader->numStages -1){
      nextstage = shader->stages[i+1];
    }
    else
      nextstage = NULL;

    lxListNode_init(stage);

    // colormodall only works when previous was a colormodall and no modulate
    if (isFlagSet(stage->stageflag,SHADER_VCOLOR) && stage->blendmode != VID_MODULATE && colormodall == (int)i){
      colormodall = i+1;
    }// modulate without vcolors works however ((x OP y)*v) *tex OP ... = ((x OP y)*tex)  *v OP...
    else if (!isFlagSet(stage->stageflag,SHADER_VCOLOR) && stage->blendmode == VID_MODULATE && colormodall == (int)i){
      colormodall = i+1;
    }// swap in favor of getting all vcolors in a row
    else if (nextstage && isFlagSet(nextstage->stageflag,SHADER_VCOLOR)){
      switch (nextstage->blendmode){
        // dont allow us to change order
      case VID_MODULATE:
      case VID_DECAL:
      case VID_DECAL_CONSTMOD:
      case VID_DECAL_PREV:
      case VID_DECAL_VERTEX:
      case VID_DECAL_CONST:
      case VID_AMODADD_PREV:
      case VID_AMODADD_CONSTMOD:
        break;
      default:
        // when there is another stage relying on next
        // we cant swap either
        if (i+2 < shader->numStages &&
          (shader->stages[i+2]->blendmode == VID_DECAL_PREV ||
          shader->stages[i+2]->blendmode == VID_AMODADD_PREV)){
            break;
          }
          // else we can safely swap
          shader->stages[i] = nextstage;
          shader->stages[i+1] = stage;
          // and rerun this stage (after swap)
          i--;
          continue;
          break;
      }
    }

    // these here dont allow us to change the order
    switch(stage->blendmode){
    case VID_MODULATE:
    case VID_DECAL:
    case VID_DECAL_PREV:
    case VID_DECAL_VERTEX:
    case VID_DECAL_CONST:
    case VID_DECAL_CONSTMOD:
      tofirst = LUX_FALSE;
      break;
    }
    if (stage->stagetype == SHADER_STAGE_GPU){
      lxListNode_addLast(stagesListHead,stage);
      tofirst = LUX_FALSE;
    }

    switch(stage->blendmode){
    case VID_MODULATE:
    case VID_REPLACE:
    case VID_DECAL:
    case VID_DECAL_PREV:
    case VID_DECAL_VERTEX:
    case VID_DECAL_CONST:
    case VID_DECAL_CONSTMOD:
    case VID_ADD:
    case VID_AMODADD_PREV:
      lxListNode_addLast(stagesListHead,stage);
      break;
    case VID_AMODADD:
    case VID_AMODADD_CONSTMOD:
    case VID_AMODADD_VERTEX:
    case VID_AMODADD_CONST:
      // we can make it first, when there are no colormods at all, or if we are the current "last colormod"
      // and when  noone relies on us
      if (tofirst && (colormodall == 0 || colormodall == (int)i+1)
        && !(nextstage && (nextstage->blendmode == VID_DECAL_PREV || nextstage->blendmode == VID_AMODADD_PREV))){
          // change the first one to ADD
          if (tofirst == 1){
            stagesListHead->blendmode = VID_ADD;
            lxListNode_addFirst(stagesListHead,stage);
          }
          else
          {
            stagesListHead->next->blendmode = VID_ADD;
            lxListNode_insertPrev(stagesListHead->next,stage);
          }
          switch(stage->blendmode){
    case VID_AMODADD:
      stage->blendmode = VID_AMOD;
      break;
    case VID_AMODADD_VERTEX:
      stage->blendmode = VID_AMOD_VERTEX;
      break;
    case VID_AMODADD_CONST:
      stage->blendmode = VID_AMOD_CONST;
      break;
          }


          // it only works once
          tofirst = LUX_FALSE;
        }
      else
        lxListNode_addLast(stagesListHead,stage);
      break;
    }
  }

  // setup new pointers using the list we just created
  stage = stagesListHead;
  for (i = gpuprogs; i < shader->numStages; i++){
    shader->stages[i] = stage;
    stage = stage->next;
  }

  *hasaddpassfirst = addpassfirst;
  return colormodall;
}

//////////////////////////////////////////////////////////////////////////
// GLShaderPass/Stage Constructors

GLShaderPass_t  *GLShaderPass_new(Shader_t *shader, GLShaderStage_t **glstagelastpass){
  GLShaderPass_t *out;
  out = reszalloc(sizeof(GLShaderPass_t));
  out->blend.blendmode = VID_REPLACE;
  out->vertexGPU = VID_FIXED;
  out->fragmentGPU = VID_FIXED;
  out->geometryGPU = VID_FIXED;
  out->shader = shader;
  shader->numPasses++;
  lxListNode_init(out);
  if (shader->glpassListHead)
    *glstagelastpass = shader->glpassListHead->prev->glstageListHead->prev;

  lxListNode_addLast(shader->glpassListHead,out);
  return out;
}
GLShaderStage_t *GLShaderStage_new(GLShaderPass_t *glpass,ShaderStage_t *stage,int first){
  GLShaderStage_t *out;
  out = reszalloc(sizeof(GLShaderStage_t));
  out->blend.blendmode = VID_MODULATE;
  out->stage = stage;
  lxListNode_init(out);

  if (stage == &l_ColorPass)
    glpass->blend.blendmode = VID_MODULATE;

  glpass->numStages++;
  if (first){
    lxListNode_addFirst(glpass->glstageListHead,out);
  }
  else{
    lxListNode_addLast(glpass->glstageListHead,out);
  }
  return out;
}

//////////////////////////////////////////////////////////////////////////
// Conversion

void GLShaderStage_blend(GLShaderStage_t *glstage, GLShaderPass_t *glpass, VIDBlendColor_t  nextmode)
{
  int   skip = LUX_FALSE;
  int   invert = LUX_FALSE;
  int   vcolor = LUX_FALSE;

  VIDBlendColor_t DEFAULT;
  int   amodadd;
  VIDBlendColor_t blendmode;
  VIDBlendColor_t alphamodeStage = VID_A_UNSET;
  VIDBlendColor_t blendmodeStage;

  // how to handle vertex colors
  if (isFlagSet(glstage->stage->stageflag,SHADER_VCOLOR)){
    DEFAULT = VID_MODULATE;
    vcolor = LUX_TRUE;
  }
  else{
    DEFAULT = VID_REPLACE;
    vcolor = LUX_FALSE;
  }

  // invert alpha ?
  if (isFlagSet(glstage->stage->stageflag,SHADER_INVALPHA)){
    invert = LUX_TRUE;
  }

  // find out how AMODADDs should be blended, if the stage needs its own single stage pass
  // then its better to use AMODADD blend, else we use ADD blend and do AMOD in the combiner
  amodadd = LUX_TRUE;
  switch (glstage->stage->blendmode){
    case VID_AMODADD_VERTEX:
      if (nextmode == VID_AMODADD_VERTEX){
        amodadd = LUX_TRUE;
        break;
      }
    case VID_AMODADD:
    case VID_AMODADD_PREV:
    case VID_AMODADD_CONST:
      switch(nextmode){
        case VID_ADD:
        case VID_AMODADD:
        case VID_AMODADD_PREV:
        case VID_AMODADD_VERTEX:
        case VID_AMODADD_CONST:
          amodadd = LUX_FALSE;
          break;
      }
  }

  skip = LUX_FALSE;
  switch (glstage->stage->blendmode){
    case VID_REPLACE:
      blendmodeStage = DEFAULT;
      blendmode = VID_REPLACE;
      break;
    case VID_MODULATE:
      blendmodeStage = DEFAULT;
      blendmode = VID_MODULATE;
      break;
    case VID_DECAL_PREV:
      alphamodeStage = VID_A_REPLACE_PREV;
      blendmodeStage = VID_REPLACE;
      if (vcolor)
        blendmodeStage = VID_CMOD;
      skip = LUX_TRUE;
    case VID_DECAL_CONSTMOD:
      if (!skip){
        alphamodeStage = VID_A_CONSTMOD_C;
        blendmodeStage = VID_REPLACE;
        if (vcolor)
          blendmodeStage = VID_CMOD;
        skip = LUX_TRUE;
      }
    case VID_DECAL:
      if (!skip){
        alphamodeStage = VID_A_REPLACE;
        blendmodeStage = VID_REPLACE;
        if (vcolor)
          blendmodeStage = VID_CMOD;
        skip = LUX_TRUE;
      }
    case VID_DECAL_VERTEX:
      if (!skip){
        alphamodeStage = VID_A_REPLACE_VERTEX;
        blendmodeStage = VID_REPLACE;
        if (vcolor)
          blendmodeStage = VID_CMOD;
        skip = LUX_TRUE;
      }
    case VID_DECAL_CONST:
      if (!skip){
        alphamodeStage = VID_A_REPLACE_CONST;
        blendmodeStage = VID_REPLACE;
        if (vcolor)
          blendmodeStage = VID_CMOD;
        skip = LUX_TRUE;
      }
      blendmode = VID_DECAL;
      break;
    case VID_ADD:
      blendmodeStage = DEFAULT;
      blendmode = VID_ADD;
      break;
    case VID_AMODADD_CONSTMOD:
      alphamodeStage = VID_A_CONSTMOD_C;
      if(amodadd){
        blendmodeStage = VID_REPLACE;
        if (vcolor)
          blendmodeStage = VID_CMOD;
      }
      else
        blendmodeStage = VID_AMOD_PREV;
      skip = LUX_TRUE;
    case VID_AMODADD_PREV:
      if (!skip){
        alphamodeStage = VID_A_REPLACE_PREV;
        if(amodadd){
          blendmodeStage = VID_REPLACE;
          if (vcolor)
            blendmodeStage = VID_CMOD;
        }
        else
          blendmodeStage = VID_AMOD_PREV;
        skip = LUX_TRUE;
      }
    case VID_AMODADD:
      if (!skip){
        alphamodeStage = VID_A_REPLACE;
        if(amodadd){
          blendmodeStage = VID_REPLACE;
          if (vcolor)
            blendmodeStage = VID_CMOD;
        }
        else
          blendmodeStage = VID_AMOD;
        skip = LUX_TRUE;
      }
    case VID_AMODADD_VERTEX:
      if (!skip){
        alphamodeStage = VID_A_REPLACE_VERTEX;
        if(amodadd){
          blendmodeStage = VID_REPLACE;
          if (vcolor)
            blendmodeStage = VID_CMOD;
        }
        else
          blendmodeStage = VID_AMOD_VERTEX;
        skip = LUX_TRUE;
      }
    case VID_AMODADD_CONST:
      if (!skip){
        alphamodeStage = VID_A_REPLACE_CONST;
        if(amodadd){
          blendmodeStage = VID_REPLACE;
          if (vcolor)
            blendmodeStage = VID_CMOD;
        }
        else
          blendmodeStage = VID_AMOD_CONST;
        skip = LUX_TRUE;
      }
      // we dont do MODADD to leave more space for mixing MODADD with ADD
      if (amodadd){
        blendmode = VID_AMODADD;
      }
      else{
        blendmode = VID_ADD;
      }
      break;
    default:
      blendmodeStage = glstage->stage->blendmode;
      blendmode = VID_REPLACE;
      break;
  }
  glpass->blend.blendmode = blendmode;
  glpass->blend.blendinvert = invert;


  glstage->blend.alphamode = alphamodeStage;
  glstage->blend.blendmode = blendmodeStage;
  glstage->blend.blendinvert = LUX_FALSE;

  clearFlag(glstage->stage->stageflag,SHADER_INVALPHA);
  clearFlag(glstage->stage->stageflag,SHADER_VCOLOR);


}

GLShaderConvert_t ShaderStage_convert(ShaderStage_t *stage,
                    ShaderStage_t *prevstage,
                    ShaderStage_t *nextstage,
                    int colorpassoffset,
                    GLShaderPass_t *glpass,
                    VIDBlendColor_t firstblend,int texleft)
{
  // this here more or less evaluates if we can put it into the same pass
  uchar vcolor;
  VIDBlendColor_t blendpassmode = glpass->blend.blendmode;

  // vcolor : stage * v
  // colorpassoffset = 0 (no colorpass)  < 0 (colorpass was already) == 1 (we are the pass) > 1 (colorpass will come)

  vcolor = (isFlagSet(stage->stageflag, SHADER_VCOLOR)) ? LUX_TRUE : LUX_FALSE;

  // three cases for vcolor handling
  //  colorpass = now
  //  colorpass = was already, and we have vcolor
  //  colorpass = will happen later or not at all, ie we dont need to take vcolors into account


  // first we check under what conditions the current stage is being built
  // only replace really gives us most variety
  switch(blendpassmode){
    case VID_REPLACE:
      switch(stage->blendmode){
        // BASICS
        case VID_ADD:
          if (colorpassoffset == 1 ){
            if (prevstage->stagetype == SHADER_STAGE_COLOR && (GLEW_NV_texture_env_combine4 || GLEW_ATI_texture_env_combine3))
              return GL_SHADER_NOPASS;
            else if (GLEW_NV_texture_env_combine4)
              return GL_SHADER_NOPASS;
            else if (prevstage && prevstage->stagetype == SHADER_STAGE_TEX && prevstage->blendmode == VID_REPLACE
              && (GLEW_NV_texture_env_combine4 || GLEW_ARB_texture_env_crossbar) )
              return GL_SHADER_NOPASS_CROSSBAR;
            else if (texleft > 0)
              return GL_SHADER_NOPASS_COLORMOD_NEXT;
            else
              return GL_SHADER_PASS_COLORMOD_NOW;
          }
          else if (vcolor && colorpassoffset < 0){
            if (GLEW_NV_texture_env_combine4 || GLEW_ATI_texture_env_combine3)
              return GL_SHADER_NOPASS;
            else
              return GL_SHADER_PASS;
          }
          else
            return GL_SHADER_NOPASS;
        case VID_REPLACE:
          return GL_SHADER_NOPASS;
        case VID_MODULATE:
          if (colorpassoffset == 1 ){
            if (prevstage && prevstage->stagetype == SHADER_STAGE_TEX && prevstage->blendmode == VID_REPLACE)
              return GL_SHADER_NOPASS;
            else if (texleft > 0)
              return GL_SHADER_NOPASS_COLORMOD_NEXT;
            else
              return GL_SHADER_PASS_COLORMOD_NOW;
          }
          else if (vcolor){
            if (texleft > 0)
              return GL_SHADER_NOPASS_COLORMOD_NEXT;
            else
              return GL_SHADER_PASS;
          }
          else
            return GL_SHADER_NOPASS;
        // DECAL
        case VID_DECAL:
        case VID_DECAL_PREV:
        case VID_DECAL_VERTEX:
        case VID_DECAL_CONST:
          if (colorpassoffset > 1 &&
            (GLEW_NV_texture_env_combine4 || GLEW_ARB_texture_env_crossbar) ){
            if (prevstage && prevstage->stagetype == SHADER_STAGE_TEX && prevstage->blendmode == VID_REPLACE)
              return GL_SHADER_NOPASS_CROSSBAR;
            else if (prevstage && prevstage->stagetype == SHADER_STAGE_TEX && (prevstage->blendmode == VID_DECAL) &&
              (glpass->glstageListHead->prev->blend.blendmode == VID_CMOD_PREV ||
              glpass->glstageListHead->prev->blend.blendmode == VID_PASSTHRU))
              return GL_SHADER_NOPASS_CROSSBAR_CHAIN;
            else
              return GL_SHADER_NOPASS;
          }
          else if (colorpassoffset == 1 ){
            if (prevstage && prevstage->stagetype == SHADER_STAGE_TEX && prevstage->blendmode == VID_REPLACE
              && (GLEW_NV_texture_env_combine4 || GLEW_ARB_texture_env_crossbar))
              return GL_SHADER_NOPASS_CROSSBAR;
            else if (glpass->glstageListHead->prev->blend.blendmode == VID_PASSTHRU)
              return GL_SHADER_NOPASS_CROSSBAR_CHAIN;
            else if (texleft > 0)
              return GL_SHADER_NOPASS_COLORMOD_NEXT;
            else
              return GL_SHADER_PASS_COLOR;
          }
          else if (vcolor && colorpassoffset < 0){
              return GL_SHADER_PASS;
          }
          else
            return GL_SHADER_NOPASS;

        case VID_DECAL_CONSTMOD:
          if (colorpassoffset == 1){
            if (texleft > 0)
              return GL_SHADER_NOPASS_COLORMOD_NEXT;
            else
              return GL_SHADER_PASS_COLOR;
          }
          else if (vcolor && colorpassoffset < 0){
            return GL_SHADER_PASS;
          }
          else
            return GL_SHADER_NOPASS;


        // MODADD  can be done with a single combine if ati or nv combine are available
        case VID_AMODADD:
        case VID_AMODADD_CONSTMOD:
        case VID_AMODADD_PREV:
        case VID_AMODADD_VERTEX:
        case VID_AMODADD_CONST:
          if (colorpassoffset == 1 ){
            if (texleft > 0 && (GLEW_NV_texture_env_combine4 || GLEW_ATI_texture_env_combine3))
              return GL_SHADER_NOPASS_COLORMOD_NEXT;
            else
              return GL_SHADER_PASS_COLORMOD_NOW;
          }
          else if (vcolor && colorpassoffset < 0){
              return GL_SHADER_PASS;
          }
          else if ((GLEW_NV_texture_env_combine4 || GLEW_ATI_texture_env_combine3))
            return GL_SHADER_NOPASS;
          else
            return GL_SHADER_PASS;
      }
      break;
    // now we have a lot less space to work with, cause we are under a blend
    // that means any color info stuff must be taken care of within the pass
    case VID_MODULATE:
      switch(stage->blendmode){
        case VID_MODULATE:
          if (vcolor && texleft > 0)
            return GL_SHADER_NOPASS_COLORMOD_NEXT;
          else
            return GL_SHADER_NOPASS;
        default:
          return GL_SHADER_PASS;
      }
    case VID_ADD:
      switch(stage->blendmode){
        case VID_ADD:
          if (colorpassoffset == 1){
            if (GLEW_NV_texture_env_combine4)
              return GL_SHADER_NOPASS;
            else if (texleft > 0)
              return GL_SHADER_NOPASS_COLORMOD_NEXT;
            else
              return GL_SHADER_PASS_COLORMOD_NOW;
          }
          else if (colorpassoffset < 0 && vcolor){
            if ((GLEW_NV_texture_env_combine4 || GLEW_ATI_texture_env_combine3))
              return GL_SHADER_NOPASS;
            else
              return GL_SHADER_PASS;
          } // last stage of this pass, all previous "thought" we would fix the color for it
          else if (colorpassoffset > 1 && texleft == 0){
            if (GLEW_NV_texture_env_combine4 )
              return GL_SHADER_NOPASS;
            else
              return GL_SHADER_PASS_COLORMOD_NOW;
          }
          else
            return GL_SHADER_NOPASS;
          break;
        case VID_AMODADD:
        case VID_AMODADD_PREV:
        case VID_AMODADD_VERTEX:
        case VID_AMODADD_CONSTMOD:
        case VID_AMODADD_CONST:
          if (colorpassoffset == 1){
            if ((GLEW_NV_texture_env_combine4 || GLEW_ATI_texture_env_combine3) && texleft > 0 )
              return GL_SHADER_NOPASS_COLORMOD_NEXT;
            else
              return GL_SHADER_PASS_COLORMOD_NOW;
          }
          else if (colorpassoffset < 0 && vcolor){
              return GL_SHADER_PASS;
          } // last stage of this pass, all previous "thought" we would fix the color for it
          else if (colorpassoffset > 1 && texleft == 0){
              return GL_SHADER_PASS_COLORMOD_NOW;
          }
          else if ((GLEW_NV_texture_env_combine4 || GLEW_ATI_texture_env_combine3))
            return GL_SHADER_NOPASS;
          else
            return GL_SHADER_PASS;

          break;
        default:
          return GL_SHADER_PASS;
      }
    case VID_AMODADD:
      if (VID_AMODADD_VERTEX == stage->blendmode && prevstage->blendmode == stage->blendmode){
        if (colorpassoffset == 1){
          if (GLEW_NV_texture_env_combine4 )
            return GL_SHADER_NOPASS;
          else if (GLEW_ARB_texture_env_crossbar && prevstage->stagetype == SHADER_STAGE_TEX && prevstage == glpass->glstageListHead->stage)
            return GL_SHADER_NOPASS_CROSSBAR;
          else if (texleft > 0)
            GL_SHADER_NOPASS_COLORMOD_NEXT;
          else
            return GL_SHADER_PASS_COLORMOD_NOW;
        }
        else if (colorpassoffset < 0 && vcolor){
          if ((GLEW_NV_texture_env_combine4 || GLEW_ATI_texture_env_combine3) )
            return GL_SHADER_NOPASS;
          else
            return GL_SHADER_PASS;
        }
        else if (colorpassoffset > 1 && texleft == 0){
            return GL_SHADER_PASS_COLORMOD_NOW;
        }
        else
          return GL_SHADER_NOPASS;
      }
      else
        return GL_SHADER_PASS;
    case VID_DECAL:
      switch (stage->blendmode){
        // DECAL
        case VID_DECAL:
        case VID_DECAL_PREV:
        case VID_DECAL_VERTEX:
        case VID_DECAL_CONST:
          if (colorpassoffset > 1
            && (GLEW_NV_texture_env_combine4 || GLEW_ARB_texture_env_crossbar) ){
            if (prevstage && prevstage->stagetype == SHADER_STAGE_TEX && prevstage->blendmode == VID_DECAL)
              return GL_SHADER_NOPASS_CROSSBAR;
            else if (prevstage && prevstage->stagetype == SHADER_STAGE_TEX && (prevstage->blendmode == VID_DECAL) &&
              (glpass->glstageListHead->prev->blend.blendmode == VID_CMOD_PREV ||
              glpass->glstageListHead->prev->blend.blendmode == VID_PASSTHRU))
              return GL_SHADER_NOPASS_CROSSBAR_CHAIN;
            else
              return GL_SHADER_NOPASS;
          }
          else if (colorpassoffset == 1 ){
            if (prevstage && prevstage->stagetype == SHADER_STAGE_TEX && prevstage->blendmode == VID_DECAL
              && (GLEW_NV_texture_env_combine4 || GLEW_ARB_texture_env_crossbar) )
              return GL_SHADER_NOPASS_CROSSBAR;
            else if (glpass->glstageListHead->prev->blend.blendmode == VID_PASSTHRU)
              return GL_SHADER_NOPASS_CROSSBAR_CHAIN;
            else if (texleft > 0)
              return GL_SHADER_NOPASS_COLORMOD_NEXT;
            else
              return GL_SHADER_PASS_COLOR;
          }
          else if (vcolor && colorpassoffset < 0){
            return GL_SHADER_PASS;
          }
          else
            return GL_SHADER_NOPASS;
        default:
          return GL_SHADER_PASS;
      }
    default:
      return GL_SHADER_PASS;
  }



  lprintf("WARNING: shdcompile: unhandled case\n");
  // in case we are here things went wrong
  return GL_SHADER_PASS;
}



int GLShaderConvert_setup(GLShaderConvert_t glconv,Shader_t *shader,
            int texleft,int *colormodall,int *newpass,int *i,int *texstages,
            ShaderStage_t *stage,
              ShaderStage_t *prevstage,
              ShaderStage_t *nextstage,
              int colorpassoffset,
              GLShaderPass_t *glpass,
              VIDBlendColor_t firstblend,
              GLShaderStage_t **glstagelastpass)
{
  GLShaderStage_t *glstage;

  switch (glconv){
  case GL_SHADER_NOPASS:
    // GLStage
    glstage = GLShaderStage_new(glpass,stage,LUX_FALSE);

    glstage->blend.blendmode = stage->blendmode;
    glstage->blend.blendinvert = ((stage->stageflag & SHADER_INVALPHA) != 0);
    if (stage->blendmode == VID_ADD){
      if (colorpassoffset == 1 && prevstage->stagetype == SHADER_STAGE_COLOR)
        glstage->blend.blendmode = VID_CMODADD_VERTEX;
      else if (colorpassoffset == 1)
        glstage->blend.blendmode = VID_ADDCMOD_VERTEX;
      else if (colorpassoffset < 0 && isFlagSet(stage->stageflag, SHADER_VCOLOR))
        glstage->blend.blendmode = VID_CMODADD_VERTEX;
      else if (colorpassoffset == 2 && texleft == 0
        && (GLEW_NV_texture_env_combine4) ){
          glstage->blend.blendmode = VID_ADDCMOD_VERTEX;
          *colormodall--;
        }
    }
    // FIXME some combo may not like this
    if (glpass->blend.blendmode == VID_REPLACE &&
      stage->blendmode == VID_REPLACE && colorpassoffset >= 1 &&
      (!nextstage || nextstage->blendmode == VID_MODULATE))
    {
      glstage->blend.blendmode = VID_MODULATE;
      if (nextstage)
        *colormodall--;
    }
    if (glpass->blend.blendmode == VID_ADD && colorpassoffset >= 1){
      prevstage->stageflag &= ~SHADER_VCOLOR;
      if (glpass->glstageListHead->stage == prevstage
        && glpass->glstageListHead->blend.blendmode == VID_MODULATE)
        glpass->glstageListHead->blend.blendmode = VID_REPLACE;
      if (stage->blendmode == VID_ADD)
        *colormodall--;
    }
    if (glpass->blend.blendmode == VID_REPLACE && colorpassoffset >= 1){
      if (glpass->glstageListHead->stage == prevstage
        && glpass->glstageListHead->blend.blendmode == VID_MODULATE
        && stage->blendmode != VID_MODULATE)
        glpass->glstageListHead->blend.blendmode = VID_REPLACE;
      if (!prevstage && colorpassoffset == 1 && stage->blendmode == VID_REPLACE)
        glstage->blend.blendmode = VID_MODULATE;
    }
    if (glpass->blend.blendmode == VID_AMODADD && prevstage
      && prevstage->blendmode == stage->blendmode && stage->blendmode == VID_AMODADD_VERTEX)
    {
      if (colorpassoffset == 1)
        glstage->blend.blendmode = VID_ADDCMOD_VERTEX;
      else if (colorpassoffset < 0 && isFlagSet(stage->stageflag, SHADER_VCOLOR))
        glstage->blend.blendmode = VID_CMODADD_VERTEX;
      else
        glstage->blend.blendmode = VID_ADD;

      if (colorpassoffset >= 1 && glpass->glstageListHead->stage == prevstage)
        glpass->glstageListHead->blend.blendmode = VID_REPLACE;
    }
    if (stage->blendmode == VID_DECAL_CONSTMOD ||
      stage->blendmode == VID_AMODADD_CONSTMOD)
    {
      if (stage->blendmode == VID_DECAL_CONSTMOD)
        glstage->blend.blendmode = VID_DECAL_PREV;
      else
        glstage->blend.blendmode = VID_AMODADD_PREV;

      if (*texstages == 2)
        glstage->prev->blend.alphamode = VID_A_CONSTMOD_C;
      else if (*texstages == 3)
        glstage->prev->blend.alphamode = VID_A_CONSTMOD_C2;
      else if (*texstages == 4)
        glstage->prev->blend.alphamode = VID_A_CONSTMOD_C3;

      // NOTE this is a bit ugly
      // but in case vertexcolors were stored in parameters
      // they need to move as well
      if (stage->paramListHead){
        if (!prevstage->paramListHead){
          prevstage->paramListHead = stage->paramListHead;
          stage->paramListHead = NULL;
        }
      }
      else
        prevstage->colorid = stage->colorid;
    }
    break;
  case GL_SHADER_NOPASS_COLORMOD_NEXT:
    // GLStage
    glstage = GLShaderStage_new(glpass,stage,LUX_FALSE);

    glstage->blend.blendmode = stage->blendmode;
    glstage->blend.blendinvert = ((stage->stageflag & SHADER_INVALPHA) != 0);

    if (stage->blendmode == VID_DECAL_CONSTMOD ||
      stage->blendmode == VID_AMODADD_CONSTMOD)
    {
      if (stage->blendmode == VID_DECAL_CONSTMOD)
        glstage->blend.blendmode = VID_DECAL_PREV;
      else
        glstage->blend.blendmode = VID_AMODADD_PREV;

      if (*texstages == 2)
        glstage->prev->blend.alphamode = VID_A_CONSTMOD_C;
      else if (*texstages == 3)
        glstage->prev->blend.alphamode = VID_A_CONSTMOD_C2;
      else if (*texstages == 4)
        glstage->prev->blend.alphamode = VID_A_CONSTMOD_C3;


      // but in case vertexcolors were stored in parameters
      // they need to move as well
      if (stage->paramListHead){
        if (!prevstage->paramListHead){
          prevstage->paramListHead = stage->paramListHead;
          stage->paramListHead = NULL;
        }
      }
      else
        prevstage->colorid = stage->colorid;
    }

    // GLStage
    glstage = GLShaderStage_new(glpass,stage,LUX_FALSE);

    glstage->blend.blendmode = VID_CMOD_PREV;

    if (glpass->blend.blendmode == VID_REPLACE && colorpassoffset >= 1)
    {
      if (glpass->glstageListHead->stage
        && glpass->glstageListHead->blend.blendmode == VID_MODULATE)
        glpass->glstageListHead->blend.blendmode = VID_REPLACE;
      if (glpass->glstageListHead->stage  &&
        ( glpass->glstageListHead->blend.blendmode == VID_AMOD ||
        glpass->glstageListHead->blend.blendmode == VID_AMOD_VERTEX ||
        glpass->glstageListHead->blend.blendmode == VID_AMOD_PREV ||
        glpass->glstageListHead->blend.blendmode == VID_AMOD_CONST))
        glpass->glstageListHead->stage->stageflag &= ~SHADER_VCOLOR;
    }
    if (glpass->blend.blendmode == VID_AMODADD && prevstage
      && prevstage->blendmode == stage->blendmode
      && stage->blendmode == VID_AMODADD_VERTEX)
    {
      if (colorpassoffset >= 1 && glpass->glstageListHead->stage == prevstage)
        glpass->glstageListHead->blend.blendmode = VID_REPLACE;
    }
        break;
  case GL_SHADER_NOPASS_CROSSBAR:
    if (glpass->blend.blendmode == VID_REPLACE && colorpassoffset >= 1)
    {
      if (glpass->glstageListHead->stage == prevstage
        && glpass->glstageListHead->blend.blendmode == VID_MODULATE)
        glpass->glstageListHead->blend.blendmode = VID_REPLACE;
    }
    if (glpass->blend.blendmode == VID_AMODADD && prevstage
      && prevstage->blendmode == stage->blendmode
      && stage->blendmode == VID_AMODADD_VERTEX)
    {
      if (colorpassoffset >= 1 && glpass->glstageListHead->stage == prevstage)
        glpass->glstageListHead->blend.blendmode = VID_REPLACE;
    }
    // GLStage
    glstage = GLShaderStage_new(glpass,stage,LUX_FALSE);

    if (colorpassoffset == 1)
      glstage->blend.blendmode = VID_CMOD_PREV;
    else if (colorpassoffset > 1 && texleft == 0 && (!nextstage || (nextstage && nextstage->blendmode != VID_MODULATE)))  // next stage is colorpass and a newpass
      glstage->blend.blendmode = VID_CMOD_PREV;
    else
      glstage->blend.blendmode = VID_PASSTHRU;

    if (glpass->blend.blendmode == VID_DECAL && glstage->blend.blendmode == VID_CMOD_PREV){
      glstage->blend.blendmode = VID_CMOD_PREV;
      glstage->blend.alphamode = VID_A_REPLACE_PREV;
    }

    switch(stage->blendmode){
    case VID_ADD:
      glstage->prev->blend.blendmode = VID_ADD_C;
      break;
    case VID_DECAL:
      glstage->prev->blend.blendmode = VID_DECAL_C;
      if (stage->stageflag & SHADER_INVALPHA)
        glstage->prev->blend.blendinvert = LUX_TRUE;
      break;
    case VID_DECAL_PREV:
      glstage->prev->blend.blendmode = VID_DECAL_PREV_C;
      if (stage->stageflag & SHADER_INVALPHA)
        glstage->prev->blend.blendinvert = LUX_TRUE;
      break;
    case VID_DECAL_VERTEX:
      glstage->prev->blend.blendmode = VID_DECAL_VERTEX_C;
      if (stage->stageflag & SHADER_INVALPHA)
        glstage->prev->blend.blendinvert = LUX_TRUE;
      break;
    case VID_DECAL_CONST:
      glstage->prev->blend.blendmode = VID_DECAL_CONST_C;
      if (stage->stageflag & SHADER_INVALPHA)
        glstage->prev->blend.blendinvert = LUX_TRUE;

      // but in case vertexcolors were stored in parameters
      // they need to move as well
      if (stage->paramListHead){
        if (!prevstage->paramListHead){
          prevstage->paramListHead = stage->paramListHead;
          stage->paramListHead = NULL;
        }
      }
      else
        prevstage->colorid = stage->colorid;

      break;
    case VID_AMODADD_VERTEX:
      glstage->prev->blend.blendmode = VID_ADD_C;
      if (stage->stageflag & SHADER_INVALPHA)
        glstage->prev->blend.blendinvert = LUX_TRUE;
      break;
    default:
      glstage->blend.blendmode = VID_CMOD_PREV;
    }
    break;
  case GL_SHADER_NOPASS_CROSSBAR_CHAIN:
    // GLStage
    glstage = GLShaderStage_new(glpass,stage,LUX_FALSE);

    if (colorpassoffset == 1)
      glstage->blend.blendmode = VID_CMOD_PREV;
    else
      glstage->blend.blendmode = VID_PASSTHRU;

    if (glpass->blend.blendmode == VID_DECAL && glstage->blend.blendmode == VID_CMOD_PREV){
      glstage->blend.blendmode = VID_CMOD_PREV;
      glstage->blend.alphamode = VID_A_REPLACE_PREV;
    }

    // we are here because either previous is crossbar -> ie cmod
    // or because it was passhtru
    if (*texstages == 2)
      glstage->prev->blend.blendmode = VID_DECAL_C;
    else if (*texstages == 3)
      glstage->prev->blend.blendmode = VID_DECAL_C2;
    else if (*texstages == 4)
      glstage->prev->blend.blendmode = VID_DECAL_C3;

    break;
  case GL_SHADER_PASS_COLORMOD_NOW:

    if (glpass->blend.blendmode == VID_REPLACE && colorpassoffset >= 1)
    {
      if (glpass->glstageListHead->stage == prevstage
        && glpass->glstageListHead->blend.blendmode == VID_MODULATE)
        glpass->glstageListHead->blend.blendmode = VID_REPLACE;
    }
    if (glpass->blend.blendmode == VID_AMODADD && prevstage
      && prevstage->blendmode == stage->blendmode
      && stage->blendmode == VID_AMODADD_VERTEX)
    {
      if (colorpassoffset >= 1 && glpass->glstageListHead->stage == prevstage)
        glpass->glstageListHead->blend.blendmode = VID_REPLACE;
    }

    // GLStage
    glstage = GLShaderStage_new(glpass,stage,LUX_FALSE);

    glstage->blend.blendmode = VID_CMOD_PREV;

    // this served as colorpass
    *colormodall--;

    *newpass = LUX_TRUE;
    *texstages = 0;
    glpass = GLShaderPass_new(shader,glstagelastpass);
    *i--;
    return LUX_TRUE;
  case GL_SHADER_PASS:
    *newpass = LUX_TRUE;
    *texstages = 0;
    glpass = GLShaderPass_new(shader,glstagelastpass);
    *i--;
    return LUX_TRUE;
  case GL_SHADER_PASS_COLOR:
    // GLStage
    glstage = GLShaderStage_new(glpass,stage,LUX_FALSE);

    glstage->blend.blendmode = stage->blendmode;
    glstage->blend.blendinvert = ((stage->stageflag & SHADER_INVALPHA) != 0);

    if (glpass->blend.blendmode == VID_REPLACE && colorpassoffset >= 1)
    {
      if (glpass->glstageListHead->stage == prevstage
        && glpass->glstageListHead->blend.blendmode == VID_MODULATE)
        glpass->glstageListHead->blend.blendmode = VID_REPLACE;
    }

    // create a new color pass
    glpass = GLShaderPass_new(shader,glstagelastpass);
    GLShaderStage_new(glpass,&l_ColorPass,LUX_FALSE);


    if (nextstage){
      glpass = GLShaderPass_new(shader,glstagelastpass);
    }

    *newpass = LUX_TRUE;
    *texstages = 0;
    return LUX_TRUE;
  }

  // we might need better alpha stage
  if (glstage->prev != glstage && ( stage->blendmode == VID_DECAL_PREV ||
    stage->blendmode == VID_AMODADD_PREV) &&
    glstage->prev->blend.alphamode == VID_A_UNSET)
    glstage->prev->blend.alphamode = VID_A_REPLACE;

  return LUX_FALSE;
}

void GLPass_setGPU(GLShaderPass_t *glpass, ShaderStage_t *stage, VID_GPU_t *vertex, VID_GPU_t *fragment, VID_GPU_t *geometry){
  switch(stage->srcType){
    case GPUPROG_V_ARB:
      glpass->vertexGPU = *vertex = VID_ARB;
      break;
    case GPUPROG_F_ARB:
      glpass->fragmentGPU = *fragment = VID_ARB;
      break;
    case GPUPROG_V_CG:
      glpass->vertexGPU = *vertex = stage->gpuinfo->vidtype;
      break;
    case GPUPROG_F_CG:
      glpass->fragmentGPU = *fragment = stage->gpuinfo->vidtype;
      break;
    case GPUPROG_G_CG:
      glpass->geometryGPU = *geometry = stage->gpuinfo->vidtype;
      break;
    case GPUPROG_V_FIXED:
      glpass->vertexGPU = *vertex = VID_FIXED;
      break;
    case GPUPROG_F_FIXED:
      glpass->fragmentGPU = *fragment = VID_FIXED;
      break;
    case GPUPROG_G_FIXED:
      glpass->geometryGPU = *geometry = VID_FIXED;
      break;
    default:
      LUX_ASSERT(0);
      break;
  }
}


//////////////////////////////////////////////////////////////////////////
// Compiler

void Shader_initGL(Shader_t *shader)
{
  int i,n=0;
  int colorstages = 0;
  int texstages = 0;
  int colorpassoffset = 0;

  int colormodall = 0;
  // all stages/passes (a OP b OP c ..)*v WARNING when firstblend is not REPLACE you cant do a "colormodpass"

  int newpass = 0;
  int istex,isgpu = 0;

  int addpassfirst = 0;
  VID_GPU_t vertex;
  VID_GPU_t fragment;
  VID_GPU_t geometry;

  ShaderStage_t *stage,*prevstage,*nextstage;
  ShaderStage_t *prevgpu = NULL;
  ShaderStage_t *stagesListHead = NULL;

  GLShaderPass_t  *glpassListHead = NULL;
  GLShaderPass_t  *glpass = NULL;
  GLShaderStage_t *glstage = NULL;
  GLShaderStage_t *glstagespec = NULL;
  GLShaderStage_t *glstagelastpass = NULL;

  VIDBlendColor_t firstblend;
  VID_TexChannel_t  *texchannels;

  // general preparation
  l_ColorPass.stagetype = SHADER_STAGE_COLORPASS;

  // first lets try to smartly swap stages so that MODADDs come first
  // and get all vcolors in a row and find the colormodall stage
  // also brings all gpu programs first

  if (SHADER_CANCOMPILE(shader))
    colormodall = Shader_prepareStages(shader,&addpassfirst);



  // up to this point the stages are properly sorted for REPLACE
  // and almost fine for the other starting blends
  // colormodall = is the last stage until all previous are *v
  // addpassfirst = on when first was a addpass (cause that info would be lost thru a "tofirst" swap of modadd)


  // create our passes
  firstblend = 0;
  newpass = LUX_TRUE;
  shader->numPasses = 0;
  glpass = glpassListHead = GLShaderPass_new(shader,&glstagelastpass);

  vertex    = VID_FIXED;
  fragment  = VID_FIXED;
  geometry  = VID_FIXED;

  prevstage = NULL;
  for ( i = 0; i < shader->numStages; i++){
    // first some basic setup
    // prevstage, ignore gpu
    if (i > 0 && shader->stages[i-1]->stagetype != SHADER_STAGE_GPU)
      prevstage = shader->stages[i-1];
    if (i > 0 && shader->stages[i-1]->stagetype == SHADER_STAGE_GPU)
      prevgpu = shader->stages[i-1];
    else
      prevgpu = NULL;

    // nextstage
    if (i < shader->numStages -1){
      nextstage = shader->stages[i+1];
    }
    else
      nextstage = NULL;
    // stage
    stage = shader->stages[i];

    switch (stage->stagetype){
      case SHADER_STAGE_COLOR:
        colorstages++;
        istex = LUX_FALSE;
        isgpu = LUX_FALSE;
        break;
      case SHADER_STAGE_NEWPASS:
      case SHADER_STAGE_COLORPASS:

        if (!newpass){
          glpass = GLShaderPass_new(shader,&glstagelastpass);
          newpass = LUX_TRUE;
          texstages = 0;
        }

        glpass->blend.blendmode = stage->blendmode;
        glpass->blend.blendinvert = isFlagSet(stage->stageflag,SHADER_INVALPHA);

        glpass->rflagson  = stage->rflagson;
        glpass->rflagsoff = stage->rflagsoff;

        if (stage->stagetype == SHADER_STAGE_COLORPASS){
          GLShaderStage_new(glpass,&l_ColorPass,LUX_FALSE);
        }
        continue;
      case SHADER_STAGE_TEX:
        texstages++;
        istex = LUX_TRUE;
        isgpu = LUX_FALSE;
        break;
      case SHADER_STAGE_GPU:
        isgpu = LUX_TRUE;
        istex = LUX_FALSE;
        break;
    }

    if (SHADER_CANCOMPILE(shader)){
      if (colormodall){
        colorpassoffset = (colormodall-1) - i;
        if (colorpassoffset >= 0)
          colorpassoffset++;
      }
      else colorpassoffset = 0;

      // what to do when we are in a new pass
      if (newpass){
        GLenum  nextmode;

        // the ugly case, we are supposed to be the colorpass
        // but right now a new pass started so all previous stages
        // in the previous passes dont have color info, we must change this !
        // as this is a full surface modulate, it only works when firstblend was replace
        // however if the last glstage had already done the color work for prev blend
        // then we dont need to do this
        // simpler when we are already modulating now, we can just make it vcolored
        // v v v | vc ->  v v v |* vc
        if (colorpassoffset == 1 && firstblend == VID_REPLACE && stage->blendmode == VID_MODULATE){
          stage->stageflag |= SHADER_VCOLOR;
        }
        else if (colorpassoffset == 1 && firstblend == VID_REPLACE && shader->numPasses > 1 ){
          // v v v | vc  -> v v v | * c | v
          if (glstagelastpass->blend.blendmode != VID_CMOD_PREV &&
            glstagelastpass->blend.blendmode != VID_CMOD &&
            glstagelastpass->blend.blendmode != VID_MODULATE)
          {
            glpass->glstageListHead = GLShaderStage_new(glpass,&l_ColorPass,LUX_FALSE);

            // this is our new actual work pass
            glpass = GLShaderPass_new(shader,&glstagelastpass);

          }// v v v | vc -> v v vc | vc
          else{
            stage->stageflag |= SHADER_VCOLOR;
          }
        }

        // GLStage
        glstage = GLShaderStage_new(glpass,stage,0);

        if (nextstage)  nextmode = nextstage->blendmode;
        else      nextmode = 0;

        if (!prevstage || ( prevstage->stagetype != SHADER_STAGE_NEWPASS &&
                  prevstage->stagetype != SHADER_STAGE_COLORPASS))
          GLShaderStage_blend(glstage,glpass,nextmode);
        else{
          // manual compile preserve all as it was
          glstage->blend.alphamode = glstage->stage->alphamode;
          glstage->blend.blendmode = glstage->stage->blendmode;
          glstage->blend.blendinvert = isFlagSet(glstage->stage->stageflag,SHADER_INVALPHA);
        }

        if (addpassfirst){
          glpass->blend.blendmode = VID_ADD;
          addpassfirst = LUX_FALSE;
        }
        if (glstage->blend.alphamode == VID_A_REPLACE_PREV){
          // prevstage functions as passthru for its alpha
          glstagespec = GLShaderStage_new(glpass,prevstage,LUX_TRUE);
          glstagespec->blend.blendmode = VID_REPLACE;
        }
        else if (glstage->blend.alphamode == VID_A_CONSTMOD_C){
          // prevstage functions as passthru for its alpha
          glstagespec = GLShaderStage_new(glpass,prevstage,LUX_TRUE);
          glstagespec->blend.blendmode = VID_REPLACE;
          glstagespec->blend.alphamode = VID_A_CONSTMOD_C;
          glstage->blend.alphamode = VID_A_REPLACE_PREV;
        }

        if (glpass == glpassListHead){
          firstblend = glpass->blend.blendmode;
        }

        newpass = LUX_FALSE;
      }
      else if (istex){
        // THE EVIL STUFF !!
        // Trying to find out if this stage fits into current pass
        // And if how
        // dont care if we have a fragment program active
        int texleft = g_VID.capTexUnits - texstages;
        GLShaderConvert_t glconv;

        glconv = ShaderStage_convert(stage,prevstage,nextstage,colorpassoffset,glpass,firstblend,texleft);

        if (GLShaderConvert_setup(glconv,shader,texleft,&colormodall,&newpass,&i,&texstages,stage,prevstage,nextstage,colorpassoffset,glpass,firstblend,&glstagelastpass))
          continue;
      }
    }
    else{
      if (newpass){
        glpass->fragmentGPU = fragment;
        glpass->vertexGPU = vertex;
        glpass->geometryGPU = geometry;

        prevgpu = NULL;
      }
      if (istex){
        // now we have a texture stage and a fragment program running
        // we simply add it cause frag program assumes them in the current modes
        // GLStage
        glstage = GLShaderStage_new(glpass,stage,LUX_FALSE);

        glstage->blend.blendmode = stage->blendmode;
        glstage->blend.alphamode = stage->alphamode;
        glstage->blend.blendinvert = isFlagSet(stage->stageflag,SHADER_INVALPHA);

        newpass = LUX_FALSE;
      }
      else if (isgpu){
        // gpu stages start always a new pass, unless prev stage was a gpu program too
        if (prevgpu && vidGPUVertex(prevgpu->srcType) == vidGPUVertex(stage->srcType)){
          newpass = LUX_TRUE;
          texstages = 0;
          i--;
          glpass->glstageListHead = NULL;
          lxListNode_popBack(glpass->glstageListHead,glstage);
          glpass->numStages --;
          lprintf("WARNING: shdcompile: same domain GpuPrograms in same pass, %s\n",shader->resinfo.name);
          continue;
        }
        else if (prevgpu || newpass)
        { // newpass or different kind of prog, they can be part of the same pass
          // GLStage
          glstage = GLShaderStage_new(glpass,stage,LUX_FALSE);
          GLPass_setGPU(glpass, stage, &vertex, &fragment, &geometry);

          newpass = LUX_FALSE;
        }
        else
        {
          // previous was no prog, then we must start a new pass
          // we throw an error
          lprintf("WARNING: shdcompile: multiple GpuPrograms in same pass, %s\n",shader->resinfo.name);
        }
      }
    }

    // if we are at capacity limit start a new pass
    if (texstages && texstages%g_VID.capTexUnits == 0 && nextstage && fragment == VID_FIXED){
      newpass = LUX_TRUE;
      texstages = 0;

      glpass = GLShaderPass_new(shader,&glstagelastpass);
    }
    else
      newpass = LUX_FALSE;
  }

  // find maximum texture stages we need,
  // also find out when colorpass happens

  shader->numMaxTexPass = 0;
  shader->colorPass = 0;
  n = 0;
  lxListNode_forEach(glpassListHead,glpass)
    glpass->numTexStages = glpass->numStages;
    lxListNode_forEach(glpass->glstageListHead,glstage)
      if (glstage->stage->stagetype != SHADER_STAGE_TEX)
        glpass->numTexStages--;
      if (glstage->stage == &l_ColorPass && !shader->colorPass)
        shader->colorPass = n+1;
      if (glstage->stage->srcType == TEX_2D_LIGHTMAP && !glstage->blend.alphamode)
        glstage->blend.alphamode = VID_A_REPLACE_PREV;
      if (isFlagSet(glstage->stage->stageflag,SHADER_TANGENTS)){
        glpass->tangents = LUX_TRUE;
      }

      if (glstage->stage->stagetype == SHADER_STAGE_TEX){
        glstage->texblend = VIDTexBlendHash_get(glstage->blend.blendmode,glstage->blend.alphamode,
            glstage->blend.blendinvert,glstage->stage->rgbscale,glstage->stage->alphascale);
      }
    lxListNode_forEachEnd(glpass->glstageListHead,glstage);

    // fix renderflags for each pass
    if (glpass->blend.blendmode == VID_REPLACE)
      glpass->blend.blendmode = VID_UNSET;

    if (glpass->blend.blendmode){
      glpass->rflagson |= RENDER_BLEND;
    }

    glpass->rflagsoff = ~glpass->rflagsoff;

    shader->numMaxTexPass = LUX_MAX(shader->numMaxTexPass,glpass->numTexStages);
    n++;
  lxListNode_forEachEnd(glpassListHead,glpass);

  // make sure default renderflag matches first pass
  // means less double state changes
  shader->renderflag |= glpassListHead->rflagson;
  shader->renderflag &= glpassListHead->rflagsoff;

  // we can lock a shader when texturechannels dont change over all passes
  shader->nolock = LUX_FALSE;
  texchannels = lxMemGenAlloc(sizeof(VID_TexChannel_t)*shader->numMaxTexPass );
  for (n = 0; n < shader->numMaxTexPass; n++ )
    texchannels[n] = -1;

  n = 0;

  lprintf("\tShaderInit Results\n");
  lxListNode_forEach(glpassListHead,glpass)
    i = 0;
    lprintf("\t Pass:\t%s\n",VIDBlendColor_toString(glpass->blend.blendmode));
    {
      // change order, GPU last
      GLShaderStage_t *glstagelist = glpass->glstageListHead;
      GLShaderStage_t *glstagelistGPU = NULL;
      glpass->glstageListHead = NULL;

      while (glstagelist){
        lxListNode_popFront(glstagelist,glstage);
        if (glstage->stage->stagetype == SHADER_STAGE_GPU){
          lxListNode_addLast(glstagelistGPU,glstage);
        }
        else{
          lxListNode_addLast(glpass->glstageListHead,glstage);
        }
      }

      while (glstagelistGPU){
        lxListNode_popFront(glstagelistGPU,glstage);
        lxListNode_addLast(glpass->glstageListHead,glstage);
      }
    }

    lxListNode_forEach(glpass->glstageListHead,glstage)
      if (glstage->stage->stagetype != SHADER_STAGE_TEX)
        i--;
      else if (texchannels[i] == -1)
        texchannels[i]=glstage->stage->texchannel;
      else if (texchannels[i] != glstage->stage->texchannel)
        shader->nolock = LUX_TRUE;

      switch(glstage->stage->stagetype) {
      case SHADER_STAGE_TEX:
        lprintf("\t  Tex:\t%s,\t%s\n",VIDBlendColor_toString(glstage->blend.blendmode),VIDBlendAlpha_toString(glstage->blend.alphamode));
        break;
      case SHADER_STAGE_GPU:
        lprintf("\t  Gpu\n");
        break;
      case SHADER_STAGE_COLOR:
        lprintf("\t  Color\n");
        break;
      case SHADER_STAGE_COLORPASS:
        lprintf("\t  ColorPass\n");
        break;
      }
      i++;
    lxListNode_forEachEnd(glpass->glstageListHead,glstage);
    n++;
  lxListNode_forEachEnd(glpassListHead,glpass);

  if (!shader->nolock){
    shader->texchannels = reszalloc(sizeof(VID_TexChannel_t)*shader->numMaxTexPass);
    memcpy(shader->texchannels,texchannels,sizeof(VID_TexChannel_t)*shader->numMaxTexPass);
  }
  lxMemGenFree(texchannels,sizeof(VID_TexChannel_t)*shader->numMaxTexPass );

  lprintf("\tTotal Passes: %d\n\n",shader->numPasses);
}

typedef struct GPUPair_s{
  CGprogram vert;
  CGprogram frag;
  CGprogram combo;
  int     vertRID;
  int     fragRID;
}GPUPair_t;

void Shader_initGpuPrograms(Shader_t *shader)
{
  ShaderParam_t *param;
  ShaderStage_t *stage;
  ShaderGpuInfo_t *gpuinfovert = NULL;
  ShaderGpuInfo_t *gpuinfofrag = NULL;
  GLShaderPass_t *glpass;
  GLShaderStage_t *glstage;
  CGprogram program;
  CGparameter cgparam;
  int vglsl;
  int fglsl;

  int n;
  int i;

  GPUPair_t *pair;
  GPUPair_t *cpair;
  GPUPair_t *combinations;
  int **usedprogs;
  int usedcnt = 0;
  int hasvprog;
  CGprogram gpuv;
  CGprogram gpuvskin;
  CGprogram gpuf;
  CGprogram gpuffog;

  bufferclear();
  combinations = bufferzalloc(sizeof(GPUPair_t)*GL_GPUCOMBO_MAX);
  usedprogs = buffermalloc(sizeof(int*)*GL_GPUCOMBO_MAX);
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

  // find vertex and fragment info
  lxListNode_forEach(shader->glpassListHead,glpass)
    hasvprog = LUX_FALSE;
    lxListNode_forEach(glpass->glstageListHead,glstage)
      stage = glstage->stage;
      if (stage->srcType != GPUPROG_F_CG && stage->srcType != GPUPROG_V_CG )
        lxListNode_forEachContinue(glpass->glstageListHead,glstage);

      if (stage->srcType == GPUPROG_V_CG)
        gpuinfovert = stage->gpuinfo;
      else
        gpuinfofrag = stage->gpuinfo;

    lxListNode_forEachEnd(glpass->glstageListHead,glstage);

    if (!gpuinfofrag && !gpuinfovert)
      lxListNode_forEachContinue(shader->glpassListHead,glpass);

    vglsl = gpuinfovert && (ResData_getGpuProg(gpuinfovert->srcRIDs[0])->cgProfile == g_VID.cg.glslVertexProfile);
    fglsl = gpuinfofrag && (ResData_getGpuProg(gpuinfofrag->srcRIDs[0])->cgProfile == g_VID.cg.glslFragmentProfile);

    // first pass, find out all programs that are around
    for (n = 0; n < (VID_MAX_LIGHTS+1); n++){
      gpuv = NULL;
      gpuf = NULL;
      gpuvskin = NULL;
      gpuffog = NULL;
      // vertex
      if (gpuinfovert){
        program = gpuv = ResData_getGpuProg(gpuinfovert->srcRIDs[n])->cgProgram;
        lxArrayFindOrAddPtr(usedprogs,&usedcnt,program,GL_GPUCOMBO_MAX);

        if (shader->shaderflag & SHADER_HWSKIN){
          program = gpuvskin = ResData_getGpuProg(gpuinfovert->srcSkinRIDs[n])->cgProgram;
          lxArrayFindOrAddPtr(usedprogs,&usedcnt,program,GL_GPUCOMBO_MAX);
        }
      }

      // fragment
      if (gpuinfofrag){
        program = gpuf = ResData_getGpuProg(gpuinfofrag->srcRIDs[n])->cgProgram;
        lxArrayFindOrAddPtr(usedprogs,&usedcnt,program,GL_GPUCOMBO_MAX);

        program = gpuffog = ResData_getGpuProg(gpuinfofrag->srcSkinRIDs[n])->cgProgram;
        lxArrayFindOrAddPtr(usedprogs,&usedcnt,program,GL_GPUCOMBO_MAX);
      }
      pair = &combinations[n];
      pair->vert = gpuv;
      pair->frag = gpuf;
      pair->vertRID = gpuinfovert ? gpuinfovert->srcRIDs[n] : -1;
      pair->fragRID = gpuinfofrag ? gpuinfofrag->srcRIDs[n] : -1;

      pair = &combinations[n+GL_GPUCOMBO_FOGGED];
      pair->vert = gpuv;
      pair->frag = gpuffog;
      pair->vertRID = gpuinfovert ? gpuinfovert->srcRIDs[n] : -1;
      pair->fragRID = gpuinfofrag ? gpuinfofrag->srcSkinRIDs[n] : -1;

      if (shader->shaderflag & SHADER_HWSKIN){
        pair = &combinations[n+GL_GPUCOMBO_SKINNED];
        pair->vert = gpuvskin;
        pair->frag = gpuf;
        pair->vertRID = gpuinfovert ? gpuinfovert->srcSkinRIDs[n] : -1;
        pair->fragRID = gpuinfofrag ? gpuinfofrag->srcRIDs[n] : -1;

        pair = &combinations[n+GL_GPUCOMBO_FOGGED+GL_GPUCOMBO_SKINNED];
        pair->vert = gpuvskin;
        pair->frag = gpuffog;
        pair->vertRID = gpuinfovert ? gpuinfovert->srcSkinRIDs[n] : -1;
        pair->fragRID = gpuinfofrag ? gpuinfofrag->srcSkinRIDs[n] : -1;
      }
    }

    // combine programs
    if (vglsl && fglsl){
      // find unique and combine them
      usedcnt = 0;
      i = shader->shaderflag & SHADER_HWSKIN ? GL_GPUCOMBO_MAX : GL_GPUCOMBO_SKINNED;
      i--;
      pair = &combinations[i];
      for (; i >= 0; i--,pair--){
        if (!pair->combo){
          CGprogram progs[3] = {pair->vert,pair->frag,NULL};
          vidCheckErrorF(__FILE__,__LINE__);
          pair->combo = cgCombinePrograms(2,progs);
          if (vidCgCheckErrorF(__FILE__,__LINE__))break;
          cgGLLoadProgram(pair->combo);
          if (vidCgCheckErrorF(__FILE__,__LINE__) ||
            vidCheckErrorF(__FILE__,__LINE__))break;

          // -1 for comboprogs
          cgSetIntAnnotation(cgCreateProgramAnnotation(pair->combo,GPUPROG_CG_ANNO_RES,CG_INT),-1);

          usedprogs[usedcnt++]=(int*)pair->combo;

          cpair = pair-1;
          for (n = i-1; n >= 0; n--,cpair--){
            if (cpair->vert == pair->vert && cpair->frag == pair->frag)
              cpair->combo = pair->combo;
          }
        }
      }
      if (usedcnt){
        // output combos
        pair = combinations;
        for (i = 0; i < GL_GPUCOMBO_MAX; i++,pair++){
          glpass->cgcomboprogs[i] = pair->combo;
        }
        glpass->combo = LUX_TRUE;
      }
    }

    // offset usedprogs, which currently is in buffer
    buffersetbegin(usedprogs+usedcnt);

    // now link all parameters
    lxListNode_forEach(glpass->glstageListHead,glstage)
      stage = glstage->stage;
      if (! GPUPROG_IS_CG(stage->srcType) )
        lxListNode_forEachContinue(glpass->glstageListHead,glstage);

      // create parameters
      lxListNode_forEach(stage->paramListHead,param)
        if (!param->cgparam){

          // for GLSL always
          if (glpass->combo){
            param->cgparam = (param->type == SHADER_PARAM_ARRAY) ?
                  cgCreateParameterArray(g_VID.cg.context,CG_FLOAT4,param->upvalue) :
                  cgCreateParameter(g_VID.cg.context,CG_FLOAT4);
          } // for others make sure linked parameter is only created once
          else{
            // first search in one of the usedprogs
            // and associate with others
            param->cgparam = CGprogram_GroupAddOrFindParam(
                (CGprogram*)usedprogs, usedcnt, param->name,
                CG_FLOAT4, (param->type == SHADER_PARAM_ARRAY) ? param->upvalue : 0);
          }

        }

        // link parameters with all active progs
        for (n = 0; n < usedcnt; n++){
          program = (CGprogram)usedprogs[n];
          cgparam = cgGetNamedProgramParameter(program, CG_PROGRAM, param->name);
          if (cgparam)
            cgConnectParameter(param->cgparam,cgparam);
          vidCgCheckError();
        }
      lxListNode_forEachEnd(stage->paramListHead,param);

    lxListNode_forEachEnd(glpass->glstageListHead,glstage);

    // link special parameters with active progs
    for (n = 0; n < usedcnt; n++){
      program = (CGprogram)usedprogs[n];
      {
        int domains = cgGetNumProgramDomains(program);
        for (i=0; i<domains; i++) {
          CGprogram subprog = cgGetProgramDomainProgram(program, i);
          VIDCg_linkProgramConnectors(subprog);
          VIDCg_linkProgramSemantics(subprog, CG_PROGRAM);
          VIDCg_linkProgramSemantics(subprog, CG_GLOBAL);
        }
      }

      vidCgCheckError();
      vidCheckError();
    }

  lxListNode_forEachEnd(shader->glpassListHead,glpass);

}

void Shader_clearGpuPrograms(Shader_t *shader)
{
  GLShaderPass_t *glpass;
  ShaderStage_t *stage;
  GLShaderStage_t *glstage;
  ShaderParam_t *param;
  int i;

  lxListNode_forEach(shader->glpassListHead,glpass)

    if (glpass->combo){

      int **buffer;
      int cnt = 0;
      bufferclear();
      buffer = buffermalloc(sizeof(int*)*GL_GPUCOMBO_MAX);
      LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

      for (i=0; i < GL_GPUCOMBO_MAX; i++){
        lxArrayFindOrAddPtr(buffer,&cnt,glpass->cgcomboprogs[i],GL_GPUCOMBO_MAX);
      }
      for (i=0; i < cnt; i++){
        cgDestroyProgram((CGprogram)buffer[i]);
      }

    }


    lxListNode_forEach(glpass->glstageListHead,glstage)
      stage = glstage->stage;

      lxListNode_forEach(stage->paramListHead,param)
        // delete parameters, as they were unique to this cgcombo, and their source programs are now dead
        if (glpass->combo && param->cgparam && cgGetNumConnectedToParameters(param->cgparam)==0)
          cgDestroyParameter(param->cgparam);
        param->cgparam = NULL;
      lxListNode_forEachEnd(stage->paramListHead,param);

    lxListNode_forEachEnd(glpass->glstageListHead,glstage);

  lxListNode_forEachEnd(shader->glpassListHead,glpass);
}
