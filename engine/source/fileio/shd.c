// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "shd.h"
#include "../common/3dmath.h"
#include "../render/gl_render.h"
#include "../render/gl_list3d.h"
#include "../resource/resmanager.h"
#include "../fileio/filesystem.h"


void fileLoadSHDRFlag();
void fileLoadSHDNPass();
void fileLoadSHDColor();
void fileLoadSHDTex();
void fileLoadSHDGPU();
void ShaderBuildFromBuffer(Shader_t *shader);

#define BUFFERSTAGES (128)

static struct SHDData_s{
  Shader_t    *shader;
  ShaderStage_t *stage;
  ShaderStage_t *stagesColor;
  ShaderStage_t *stagesTex[BUFFERSTAGES];

  VID_Technique_t tech;
  int       stagesColorInBuffer;
  int       stagesTexInBuffer;
  int       stagesTexGpuInBuffer;
  int       lastVColorStage;
  int       stagesGpuInBuffer;
  int       bumpStage;
  int       colorStage;
  int       version;
  int       cgmode;
  lxFSFile_t    *file;
} l_SHDData;

static int SHD_technique(char* buf)
{
  char src[256];
  int tex4 = LUX_FALSE;

  sscanf(buf,"Technique:%s",&src);

  if (src[strlen(src)-1] == '{')
    src[strlen(src)-1] = 0;

  // already defined before, we are done
  if (l_SHDData.tech)
    return LUX_FALSE;

  if (strcmp(src,"VID_DEFAULT")== 0)
    l_SHDData.tech = VID_T_DEFAULT;
  else if (strcmp(src,"VID_LOWDETAIL")== 0)
    l_SHDData.tech = VID_T_LOWDETAIL;
  else if (strcmp(src,"VID_ARB_TEXCOMB_TEX4")== 0){
    l_SHDData.tech = VID_T_ARB_TEXCOMB; tex4 = LUX_TRUE;}
  else if (strcmp(src,"VID_ARB_V_TEX4")== 0){
    l_SHDData.tech = VID_T_ARB_V; tex4 = LUX_TRUE;}
  else if (strcmp(src,"VID_ARB_VF_TEX4")== 0){
    l_SHDData.tech = VID_T_ARB_VF;  tex4 = LUX_TRUE;}
  else if (strcmp(src,"VID_ARB_VF_TEX8")== 0)
    l_SHDData.tech = VID_T_ARB_VF_TEX8;
  else if (strcmp(src,"VID_CG_SM3_TEX8")== 0)
    l_SHDData.tech = VID_T_CG_SM3;
  else if (strcmp(src,"VID_CG_SM3")== 0)
    l_SHDData.tech = VID_T_CG_SM3;
  else if (strcmp(src,"VID_CG_SM4")== 0)
    l_SHDData.tech = VID_T_CG_SM4;
  else if (strcmp(src,"VID_CG_SM4")== 0)
    l_SHDData.tech = VID_T_CG_SM4_GS;
  else if (strcmp(src,"VID_ARB_VF")== 0)
    l_SHDData.tech = VID_T_ARB_VF;
  else if (strcmp(src,"VID_ARB_V")== 0)
    l_SHDData.tech = VID_T_ARB_V;
  else if (strcmp(src,"VID_ARB_TEXCOMB")== 0)
    l_SHDData.tech = VID_T_ARB_TEXCOMB;

  if (l_SHDData.tech > g_VID.capTech || (tex4 && g_VID.capTexUnits < 4)){
    l_SHDData.tech = VID_T_UNSET;
    return LUX_FALSE;
  }
  else{
    return LUX_TRUE;
  }
}

static int SHD_texture(char *name)
{
  if(l_SHDData.stagesTexInBuffer < VID_MAX_SHADER_STAGES && l_SHDData.stagesTexGpuInBuffer < BUFFERSTAGES){
    fileLoadSHDTex();
    l_SHDData.shader->numStages++;
    l_SHDData.stagesTexGpuInBuffer++;
    l_SHDData.stagesTexInBuffer++;

  }
  else lprintf("WARNING shdload: too many tex/gpu stages\n");

  return LUX_FALSE;
}

static int SHD_gpu(char *name)
{
  if (l_SHDData.tech <= VID_T_DEFAULT || l_SHDData.tech == VID_T_LOWDETAIL)
    return LUX_FALSE;

  if(l_SHDData.stagesTexGpuInBuffer < BUFFERSTAGES){
    int error = LUX_FALSE;
    fileLoadSHDGPU();

    switch (l_SHDData.stage->srcType){
      case GPUPROG_F_CG:
      case GPUPROG_F_ARB:
        if (l_SHDData.tech < VID_T_ARB_VF)
          error = LUX_TRUE;
        break;
      case GPUPROG_G_CG:
        if (l_SHDData.tech < VID_T_CG_SM4_GS)
          error = LUX_TRUE;
        break;
      default:
        break;
    }

    // must not mix ARB and CG
    if ( GPUPROG_IS_CG(l_SHDData.stage->srcType) && l_SHDData.cgmode == SHADER_PASSMODE_FIXEDARB){
      lprintf("ERROR shdload: Must not mix ARB/FIXED and Cg GpuPrograms\n");
      error = LUX_TRUE;
    }
    else if ( !GPUPROG_IS_CG(l_SHDData.stage->srcType) && l_SHDData.cgmode == SHADER_PASSMODE_CG){
      lprintf("ERROR shdload: Must not mix ARB/FIXED and Cg GpuPrograms\n");
      error = LUX_TRUE;
    }

    // set cg mode if unset
    if (l_SHDData.cgmode == -1)
      l_SHDData.cgmode = GPUPROG_IS_CG(l_SHDData.stage->srcType) ? SHADER_PASSMODE_CG : SHADER_PASSMODE_FIXEDARB;


    if ( GPUPROG_IS_CG(l_SHDData.stage->srcType) && l_SHDData.version < SHD_VER_CGLOADER)
    {
      lprintf("ERROR shdload: SHD version too old for Cg, required: %d\n",SHD_VER_CGLOADER);
      error = LUX_TRUE;
    }

    if (error){
      lprintf("ERROR shdload: wrong gpu program for picked technique/version \n");
    }
    else {
      l_SHDData.shader->numStages++;
      l_SHDData.stagesTexGpuInBuffer++;
      l_SHDData.stagesGpuInBuffer++;
    }

  }

  return LUX_FALSE;
}

static int SHD_color(char *name)
{
  if(l_SHDData.stagesColorInBuffer < 1){
    fileLoadSHDColor();
    l_SHDData.shader->numStages++;
    l_SHDData.stagesColorInBuffer++;
  }
  else lprintf("WARNING shdload: too many color stages\n");

  return LUX_FALSE;
}

static int SHD_rflag(char *name)
{
  fileLoadSHDRFlag();

  return LUX_FALSE;
}

static int SHD_newpass(char *name)
{
  if(l_SHDData.stagesTexGpuInBuffer < BUFFERSTAGES){
    fileLoadSHDNPass();

    l_SHDData.shader->numStages++;
    l_SHDData.stagesTexGpuInBuffer++;
  }
  else lprintf("WARNING shdload: too many tex/gpu stages\n");

  return LUX_FALSE;
}


static FileParseDef_t l_defSHD[]=
{
  {"IF:",     LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},
  {"ELSEIF:",   LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},
  {"ELSE",    LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},
  {"<<_",     LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},

  {"Technique:",  LUX_TRUE,LUX_FALSE,LUX_FALSE, SHD_technique},
  {"RenderFlag",  LUX_FALSE,LUX_TRUE,LUX_FALSE, SHD_rflag},
  {"NewPass",   LUX_FALSE,LUX_TRUE,LUX_FALSE, SHD_newpass},
  {"DrawPass",  LUX_FALSE,LUX_TRUE,LUX_FALSE, SHD_newpass},
  {"Color",   LUX_FALSE,LUX_TRUE,LUX_TRUE,  SHD_color},
  {"Texture",   LUX_FALSE,LUX_TRUE,LUX_TRUE,  SHD_texture},
  {"GpuProgram",  LUX_FALSE,LUX_TRUE,LUX_FALSE, SHD_gpu},

  {NULL,      LUX_FALSE,LUX_FALSE,LUX_FALSE,NULL},
};

int fileLoadSHD(const char *filename,Shader_t *shader,void *unused)
{
  lxFSFile_t * fSHD;
  char buf[1024];

  fSHD = FS_open(filename);
  l_SHDData.file = fSHD;
  l_SHDData.shader = shader;
  lprintf("Shader: \t%s\n",filename);

  if(fSHD == NULL)
  {
    lprintf("ERROR shdload: ");
    lnofile(filename);
    return LUX_FALSE;
  }

  lxFS_gets(buf, 255, fSHD);


  if (!sscanf(buf,SHD_HEADER,&l_SHDData.version) || l_SHDData.version < SHD_VER_MINIMUM)
  {
    // wrong header
    lprintf("ERROR shdload: invalid file format or version\n");
    FS_close(fSHD);
    return LUX_FALSE;
  }
  l_SHDData.cgmode = -1;
  shader->numStages = 0;

  l_SHDData.stagesColorInBuffer = 0;
  l_SHDData.stagesTexInBuffer = 0;
  l_SHDData.stagesTexGpuInBuffer = 0;
  l_SHDData.lastVColorStage = 0;
  l_SHDData.stagesGpuInBuffer = 0;
  l_SHDData.bumpStage     = -1;

  l_SHDData.tech        = VID_T_UNSET;

  clearArray(l_SHDData.stagesTex,BUFFERSTAGES);
  l_SHDData.stagesColor = NULL;

  FileParse_setAnnotationList(&l_SHDData.shader->annotationListHead);
  FileParse_start(fSHD,l_defSHD);


  if (!l_SHDData.stagesTexGpuInBuffer && !l_SHDData.stagesColorInBuffer){
    lprintf("WARNING shdload: no stages defined\n");
    FS_close (fSHD);
    return LUX_FALSE;
  }

  shader->tech = l_SHDData.tech;

  ShaderBuildFromBuffer(shader);
  dlprintf("\tColors: %d Textures: %d\n",l_SHDData.stagesColorInBuffer,l_SHDData.stagesTexGpuInBuffer);
  FS_close (fSHD);

  return LUX_TRUE;
}



// RFlag
// -----

void loadRFlag(void * target,char * command, char* rest)
{
  char buffer[1000];
  int bufferpos=0,restpos=0;
  Shader_t *shader = (Shader_t*)target;

  if (strcmp(command,"nocolorarray")==0) {
    shader->renderflag |= RENDER_NOVERTEXCOLOR;
    return;
  }
  if (strcmp(command,"normals")==0) {
    shader->shaderflag |= SHADER_NORMALS;
    return;
  }
  if (strcmp(command,"nocull")==0) {
    shader->renderflag |= RENDER_NOCULL;
    return;
  }
  if (strcmp(command,"sunlit")==0) {
    shader->renderflag |= RENDER_SUNLIT;
    return;
  }
  if (strcmp(command,"unlit")==0) {
    shader->renderflag |= RENDER_UNLIT;
    return;
  }
  if (strcmp(command,"nodepthmask")==0) {
    shader->renderflag |= RENDER_NODEPTHMASK;
    return;
  }
  if (strcmp(command,"nodepthtest")==0) {
    shader->renderflag |= RENDER_NODEPTHTEST;
    return;
  }
  if (strcmp(command,"alphamask")==0) {
    shader->renderflag |= RENDER_ALPHATEST;
    if (shader->alpha == NULL){
      shader->alpha = reszalloc(sizeof(VIDAlpha_t));
      shader->alpha->alphafunc = GL_GREATER;
      shader->alpha->alphaval = 0;
    }
    return;
  }
  if (strcmp(command,"alphaTEX")==0){
    char name[256];
    const char *next = lxStrReadInQuotes(rest, name, 255);
    shader->alphatex = reszalloc(sizeof(ShaderAlphaTex_t));
    shader->alphatex->srcType = TEX_COLOR;
    shader->alphatex->srcAttributes = TEX_ATTR_MIPMAP;

    if (strstr(name,".")==NULL){
      if (sscanf(name,"Texture:%i",&shader->alphatex->id)){
        shader->alphatex->id += MATERIAL_TEX0;
      }
    }
    else{
      resnewstrcpy(shader->alphatex->srcName,name);
    }
    sscanf(next,"%d",&shader->alphatex->texchannel);
    return;
  }
  if (strcmp(command,"layer")==0) {
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    shader->layer = 0;
    if(sscanf(buffer,"%d",&shader->layer))
      shader->layer++;
    return;
  }
  if (strcmp(command,"alphafunc")==0) {
    char src[100];
    float alphaval;

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    if (sscanf(buffer,"%s (%f)",&src,&alphaval) == 2){
      GLenum alphafunc = 0;

      if (strcmp(src,"GL_NEVER")==0)
        alphafunc = GL_NEVER;
      else if (strcmp(src,"GL_ALWAYS")==0)
        alphafunc = GL_ALWAYS;
      else if (strcmp(src,"GL_LESS")==0)
        alphafunc = GL_LESS;
      else if (strcmp(src,"GL_LEQUAL")==0)
        alphafunc = GL_LEQUAL;
      else if (strcmp(src,"GL_GEQUAL")==0)
        alphafunc = GL_GEQUAL;
      else if (strcmp(src,"GL_GREATER")==0)
        alphafunc = GL_GREATER;
      else if (strcmp(src,"GL_EQUAL")==0)
        alphafunc = GL_EQUAL;
      else if (strcmp(src,"GL_NOTEQUAL")==0)
        alphafunc = GL_NOTEQUAL;

      if (alphafunc){
        if (shader->alpha== NULL){
          shader->alpha = reszalloc(sizeof(VIDAlpha_t));
        }
        shader->alpha->alphafunc = alphafunc;
        shader->alpha->alphaval = alphaval;
        shader->renderflag |= RENDER_ALPHATEST;
      }
    }

    return;
  }
  if (strcmp(command,"blendmode")==0) {
    char src[100];
    char dst[100];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    shader->renderflag |= RENDER_BLEND;
    if (sscanf(buffer,"%s",&src,&dst)){
      VIDBlendColor_t blendmode = VID_UNSET;

      if (strcmp(src,"VID_MODULATE")==0)
        blendmode = VID_MODULATE;
      else if (strcmp(src,"VID_MODULATE2")==0)
        blendmode = VID_MODULATE2;
      else if (strcmp(src,"VID_DECAL")==0)
        blendmode = VID_DECAL;
      else if (strcmp(src,"VID_ADD")==0)
        blendmode = VID_ADD;
      else if (strcmp(src,"VID_AMODADD")==0)
        blendmode = VID_AMODADD;

      if (blendmode){
        if (shader->blend== NULL){
          shader->blend = reszalloc(sizeof(VIDBlend_t));
        }
        shader->blend->blendmode = GL_GREATER;
      }
    }
    return;
  }
  if (strcmp(command,"blendinvertalpha")== 0){
    if (shader->blend)
      shader->blend->blendinvert = LUX_TRUE;
    return;
  }
}
void fileLoadSHDRFlag()
{
  static char *words[100]={ "sunlit","unlit","nocull","nodepthmask","nodepthtest","lookat","layer",
                "alphamask","alphafunc","nocolorarray","alphaTEX","normals","blendmode","blendinvertalpha","\0"};
  FileParse_stage(l_SHDData.file,words,loadRFlag,l_SHDData.shader);
}



// Blend
// -----
void loadBlend(void *target,char * command, char* rest){
  char buffer[1024];
  int bufferpos=0,restpos=0;
  ShaderStage_t *stage = l_SHDData.stage;
  if (strcmp(command,"blendmode")==0) {
    char src[256] = {0};

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"%s",&src);

    if (strstr(src,VID_TEXCOMBSTRING))
      stage->blendmode = VIDTexCombiner_genFromString(LUX_FALSE,src);
    else
      stage->blendmode = VIDBlendColor_fromString(src);

    // default/correct
    if (!stage->blendmode)
      stage->blendmode = VID_MODULATE;
    else if (stage->blendmode == VID_AMODADD_CONSTMOD && !GLEW_NV_texture_env_combine4 && !GLEW_ARB_texture_env_crossbar){
      bprintf("WARNING: Blendmode %s not supported, reverting to %s\n",VIDBlendColor_toString(VID_AMODADD_CONSTMOD),VIDBlendColor_toString(VID_AMODADD));
      stage->blendmode = VID_AMODADD;
    }
    else if (stage->blendmode == VID_DECAL_CONSTMOD && !GLEW_NV_texture_env_combine4 && !GLEW_ARB_texture_env_crossbar){
      bprintf("WARNING: Blendmode %s not supported, reverting to %s\n",VIDBlendColor_toString(VID_DECAL_CONSTMOD),VIDBlendColor_toString(VID_DECAL));
      stage->blendmode = VID_DECAL;
    }


    return;
  }
  if (strcmp(command,"alphamode")==0) {
    char src[100];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"%s",&src);

    if (strstr(src,VID_TEXCOMBSTRING))
      stage->alphamode = VIDTexCombiner_genFromString(LUX_TRUE,src);
    else
      stage->alphamode = VIDBlendAlpha_fromString(src);

    return;
  }
  if (strcmp(command,"blendinvertalpha")== 0){
    stage->stageflag |= SHADER_INVALPHA;
  }
}

//////////////////////////////////////////////////////////////////////////
// NEW PASS

void loadNPass(void * target,char * command, char* rest)
{
  char buffer[1000] = {0};
  int bufferpos=0,restpos=0;

  if (strcmp(command,"blendmode")==0 || strcmp(command,"blendinvertalpha")==0){
    loadBlend(target,command,rest);
    return;
  }
  if (strcmp(command,"colorpass")==0){
    l_SHDData.stage->stagetype = SHADER_STAGE_COLORPASS;
    return;
  }
  if (strcmp(command,"stateflag")==0) {
    char src[100] = {0};
    booln state = 0;

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    if(sscanf(buffer,"%s %d",&src,&state)==2){
      enum32 rf = 0;

      if (strcmp(src,"RENDER_BLEND")==0)
        rf = RENDER_BLEND;
      else if (strcmp(src,"RENDER_NOVERTEXCOLOR")==0)
        rf = RENDER_NOVERTEXCOLOR;
      else if (strcmp(src,"RENDER_ALPHATEST")==0)
        rf = RENDER_ALPHATEST;
      else if (strcmp(src,"RENDER_STENCILTEST")==0)
        rf = RENDER_STENCILTEST;
      else if (strcmp(src,"RENDER_NODEPTHTEST")==0)
        rf = RENDER_NODEPTHTEST;
      else if (strcmp(src,"RENDER_NOCULL")==0)
        rf = RENDER_NOCULL;
      else if (strcmp(src,"RENDER_FRONTCULL")==0)
        rf = RENDER_FRONTCULL;
      else if (strcmp(src,"RENDER_NOCOLORMASK")==0)
        rf = RENDER_NOCOLORMASK;
      else if (strcmp(src,"RENDER_NODEPTHMASK")==0)
        rf = RENDER_NODEPTHMASK;
      else if (strcmp(src,"RENDER_STENCILMASK")==0)
        rf = RENDER_STENCILMASK;
      else if (strcmp(src,"RENDER_LIT")==0)
        rf = RENDER_LIT;
      if (state)
        l_SHDData.stage->rflagson |= rf;
      else
        l_SHDData.stage->rflagsoff |= rf;

    }

    return;
  }
}

void fileLoadSHDNPass()
{
  static char *words[100]={ "colorpass",
    "blendmode","blendinvertalpha",
    "stateflag",
    "\0"};
  ShaderStage_t *stage;

  if (!l_SHDData.stagesTex[l_SHDData.stagesTexGpuInBuffer])
    l_SHDData.stagesTex[l_SHDData.stagesTexGpuInBuffer] =reszalloc(sizeof(ShaderStage_t));

  stage = l_SHDData.stagesTex[l_SHDData.stagesTexGpuInBuffer];

  l_SHDData.shader->shaderflag |= SHADER_NOCOMPILE;

  ShaderStage_clear(stage);
  stage->stagetype = SHADER_STAGE_NEWPASS;

  stage->blendmode = VID_UNSET;
  l_SHDData.stage = stage;
  FileParse_stage(l_SHDData.file,words,loadNPass,NULL);
  if (stage->blendmode == VID_UNSET){
    lprintf("WARNING shdload: stageblend UNSET  after first stage, MODULATE instead\n");
    stage->blendmode = VID_MODULATE;
  }
}

// Color
// -----
void loadColor(void * target,char * command, char* rest)
{
  ShaderStage_t *stage = l_SHDData.stage;

  if (strcmp(command,"blendmode")==0 || strcmp(command,"blendinvertalpha")==0){
    loadBlend(target,command,rest);
    return;
  }
  // Generic
  if (strcmp(command,"RGBA")==0) {
    char name[256];
    lxStrReadInQuotes(rest, name, 255);
    if (sscanf(name,"Color:%i",&stage->id)){
      stage->id += MATERIAL_COLOR0;
      stage->colorid = stage->id;
    }
    return;
  }
}
void fileLoadSHDColor()
{
  static char *words[100]={ "RGBA","blendmode","blendinvertalpha","\0"};
  ShaderStage_t *stage;
  if (!l_SHDData.stagesColor)
    l_SHDData.stagesColor =reszalloc(sizeof(ShaderStage_t));
  stage = l_SHDData.stagesColor;
  ShaderStage_clear(stage);
  stage->stagetype = SHADER_STAGE_COLOR;

  l_SHDData.colorStage = l_SHDData.stagesTexGpuInBuffer;

  stage->blendmode = VID_UNSET;
  stage->stageflag |= SHADER_VCOLOR | SHADER_VCOLOR_KEEP;
  l_SHDData.stage = stage;
  FileParse_stage(l_SHDData.file,words,loadColor,NULL);
  switch (stage->blendmode){
    case VID_UNSET:
      if (l_SHDData.stagesTexGpuInBuffer > 0)
        stage->blendmode = VID_MODULATE;
      else
        stage->blendmode = VID_REPLACE;
      break;
  }
}

// Texture
// -------

void parseShaderParam(char *buffer, char *rest)
{
  char name[256];
  char name2[256];
  ShaderParam_t *param;
  ShaderStage_t *stage = l_SHDData.stage;
  int bufferpos=0,restpos=0;

  if (l_SHDData.shader->numTotalParams == VID_MAX_SHADERPARAMS){
    bprintf("ERROR shdload: too many shader parameters, maximum is %d\n",VID_MAX_SHADERPARAMS);
    return;
  }

  param = reszalloc(sizeof(ShaderParam_t));
  memset(param,0,sizeof(ShaderParam_t));
  lxListNode_init(param);

  param->vector[3] = 1.0f;
  param->vectorp = param->vector;
  param->totalid = l_SHDData.shader->numTotalParams;

  l_SHDData.shader->numTotalParams++;

  lxStrReadInQuotes(rest, name, 255);
  while (rest[restpos]!=0 && rest[restpos]!=';')
    buffer[bufferpos++]=rest[restpos++];
  buffer[bufferpos]=0;

  resnewstrcpy(param->name,name);
  name[0] = 0;

  sscanf(buffer,"%s %d %s (%f,%f,%f,%f) %d %d",name2,&param->id,name,&param->vector[0],&param->vector[1],&param->vector[2],&param->vector[3],&param->upvalue,&param->upvalue2);

#define SETPARAM(str,value) \
  if (strcmp(name,str)==0)  \
  param->type = value;

  SETPARAM("VID_VALUE",SHADER_PARAM_VALUE);
  SETPARAM("VID_ARRAY",SHADER_PARAM_ARRAY);
  SETPARAM("VID_CAMPOS",SHADER_PARAM_CAMPOS);
  SETPARAM("VID_CAMDIR",SHADER_PARAM_CAMDIR);
  SETPARAM("VID_CAMUP",SHADER_PARAM_CAMUP);

  SETPARAM("VID_TIME",SHADER_PARAM_TIME);
  SETPARAM("VID_RANDOM",SHADER_PARAM_RANDOM);

  SETPARAM("VID_TEXCONST",SHADER_PARAM_TEXCONST);
  SETPARAM("VID_LIGHTCOLOR",SHADER_PARAM_LIGHTCOLOR);
  SETPARAM("VID_LIGHTAMBIENT",SHADER_PARAM_LIGHTAMBIENT);
  SETPARAM("VID_LIGHTPOS",SHADER_PARAM_LIGHTPOS);
  SETPARAM("VID_LIGHTRANGE",SHADER_PARAM_LIGHTRANGE);
  SETPARAM("VID_LIGHTDIR",SHADER_PARAM_LIGHTDIR);

  SETPARAM("VID_TEXSIZEINV",SHADER_PARAM_TEXSIZEINV);
  SETPARAM("VID_TEXSIZE",SHADER_PARAM_TEXSIZE);
  SETPARAM("VID_TEXLMSCALE",SHADER_PARAM_TEXLMSCALE);

  SETPARAM("VID_TEXGEN0",SHADER_PARAM_TEXGEN0);
  SETPARAM("VID_TEXGEN1",SHADER_PARAM_TEXGEN1);
  SETPARAM("VID_TEXGEN2",SHADER_PARAM_TEXGEN2);
  SETPARAM("VID_TEXGEN3",SHADER_PARAM_TEXGEN3);

  SETPARAM("VID_TEXMAT0",SHADER_PARAM_TEXMAT0);
  SETPARAM("VID_TEXMAT1",SHADER_PARAM_TEXMAT1);
  SETPARAM("VID_TEXMAT2",SHADER_PARAM_TEXMAT2);
  SETPARAM("VID_TEXMAT3",SHADER_PARAM_TEXMAT3);

#undef SETPARAM

  lxListNode_addLast(stage->paramListHead,param);
  if (param->type == SHADER_PARAM_ARRAY){
    param->upvalue = LUX_MAX(param->upvalue,1);
    if (param->upvalue > 1){
      int i;
      param->vectorp = reszalloc(sizeof(lxVector4)*param->upvalue);
      for(i=0; i < param->upvalue; i++)
        memcpy(&param->vectorp[i*4],param->vector,sizeof(lxVector4));
    }
    else{
      param->vectorp = param->vector;
    }

    stage->numArrayParams++;
  }
  else
    stage->numVectorParams++;

}


void loadTex(void * target,char * command, char* rest)
{
  char buffer[1024];
  int bufferpos=0,restpos=0;
  ShaderStage_t *stage = l_SHDData.stage;
  int tex = 0;
  int attribute = 0;

  if (strcmp(command,"alphamode")==0 || strcmp(command,"blendmode")==0 || strcmp(command,"blendinvertalpha")==0){
    loadBlend(target,command,rest);
    return;
  }


  if (strcmp(command,"TEXALPHA")==0 || strcmp(command,"VTEXALPHA")==0)
    tex = TEX_ALPHA;
  else if (strcmp(command,"TEXCUBE")==0 || strcmp(command,"VTEXCUBE")==0){
    tex = TEX_COLOR;
    attribute |= TEX_ATTR_CUBE;
  }
  else if (strcmp(command,"TEXPROJ")==0 || strcmp(command,"VTEXPROJ")==0){
    tex = TEX_COLOR;
    attribute |= TEX_ATTR_PROJ;
  }
  else if (strcmp(command,"TEXDOTZ")==0 || strcmp(command,"VTEXDOTZ")==0){
    tex = TEX_COLOR;
    attribute |= TEX_ATTR_DOTZ;
  }
  else if (strcmp(command,"TEX")==0 || strcmp(command,"VTEX")==0)
    tex = TEX_COLOR;
  if (tex) {
    char name[256];
    lxStrReadInQuotes(rest, name, 255);

    stage->srcType = tex;
    stage->srcAttributes |= attribute;

    if (strstr(name,"Texture:")){
      if (sscanf(name,"Texture:%i",&stage->id)){
        stage->id += MATERIAL_TEX0;
        stage->colorid = stage->id;
      }
    }
    else{
      resnewstrcpy(stage->srcName,name);
    }


    if (strcmp(command,"VTEX")==0 || strcmp(command,"VTEXALPHA")==0 || strcmp(command,"VTEXPROJ")==0 || strcmp(command,"VTEXCUBE")==0 || strcmp(command,"VTEXDOTZ")==0)
      stage->stageflag |= SHADER_VCOLOR | SHADER_VCOLOR_KEEP;
    return;
  }
  if (strcmp(command,"ATTENUATE3D")==0){
    stage->srcType = TEX_3D_ATTENUATE;
    stage->stageflag |= SHADER_SPECIAL;
    return;
  }
  if (strcmp(command,"NORMALIZE")==0){
    stage->srcType = TEX_CUBE_NORMALIZE;
    stage->stageflag |= SHADER_SPECIAL;
    return;
  }
  if (strcmp(command,"SKYBOX")==0){
    stage->srcType = TEX_CUBE_SKYBOX;
    stage->stageflag |= SHADER_SPECIAL;
    return;
  }
  if (strcmp(command,"DUMMY")==0){
    stage->srcType = TEX_DUMMY;
    stage->stageflag |= SHADER_SPECIAL;
    return;
  }
  if (strcmp(command,"SPECULAR")==0){
    stage->srcType = TEX_CUBE_SPECULAR;
    stage->stageflag |= SHADER_SPECIAL;
    return;
  }
  if (strcmp(command,"DIFFUSE")==0){
    stage->srcType = TEX_CUBE_DIFFUSE;
    stage->stageflag |= SHADER_SPECIAL;
    return;
  }
  if (strcmp(command,"LIGHTMAP")==0){
    stage->srcType = TEX_2D_LIGHTMAP;
    stage->stageflag |= SHADER_SPECIAL;
    stage->texchannel = VID_TEXCOORD_LIGHTMAP;
    return;
  }
  if (strcmp(command,"vertexcolored")==0) {
    stage->stageflag |= SHADER_VCOLOR | SHADER_VCOLOR_KEEP;
    return;
  }
  if (strcmp(command,"spheremap")==0) {
    stage->stageflag |= SHADER_SPHEREMAP;
    stage->stageflag |= SHADER_TEXGEN;
    return;
  }
  if (strcmp(command,"screenmap")==0) {
    stage->stageflag |= SHADER_SCREENMAP;
    stage->stageflag |= SHADER_TEXGEN;
    return;
  }
  if (strcmp(command,"reflectmap")==0) {
    stage->stageflag |= SHADER_REFLECTMAP;
    stage->stageflag |= SHADER_TEXGEN;
    return;
  }
  if (strcmp(command,"skyreflectmap")==0) {
    stage->stageflag |= SHADER_SKYMAP;
    stage->stageflag |= SHADER_TEXGEN;
    return;
  }
  if (strstr(command,"lightnormalmap")){
    stage->stageflag |= SHADER_SUNREFMAP;
    stage->stageflag |= SHADER_TEXGEN;
    stage->stageparam = command[14]-'0';
    stage->stageparam %= 4;
    return;
  }
  if (strstr(command,"lightreflectmap")){
    stage->stageflag |= SHADER_SUNREFMAP;
    stage->stageflag |= SHADER_TEXGEN;
    stage->stageparam = command[15]-'0';
    stage->stageparam %= 4;
    return;
  }
  if (strcmp(command,"sunreflectmap")==0) {
    stage->stageflag |= SHADER_SUNREFMAP;
    stage->stageflag |= SHADER_TEXGEN;
    //l_SHDData.shader->shaderflag |= SHADER_NEEDINVMATRIX;
    stage->stageparam = 0;
    return;
  }
  if (strcmp(command,"sunnormalmap")==0) {
    stage->stageflag |= SHADER_SUNNORMALMAP;
    stage->stageflag |= SHADER_TEXGEN;
    //l_SHDData.shader->shaderflag |= SHADER_NEEDINVMATRIX;
    stage->stageparam = 0;
    return;
  }
  if (strcmp(command,"worldlinmap")==0 || strcmp(command,"eyelinmap")==0) { // backwards compatibility
    stage->stageflag |= SHADER_EYELINMAP;
    stage->stageflag |= SHADER_TEXGEN;
    return;
  }
  if (strcmp(command,"normalmap")==0) {
    stage->stageflag |= SHADER_NORMALMAP;
    stage->stageflag |= SHADER_TEXGEN;
    return;
  }
  if (strcmp(command,"depthcompare")==0) {
    stage->stageflag |= SHADER_DEPTHCOMPARE;
    return;
  }
  if (strcmp(command,"depthvalue")==0) {
    stage->stageflag |= SHADER_DEPTHNOCOMPARE;
    return;
  }
  if (strcmp(command,"texcoord")==0) {
    sscanf(rest,"%i",&stage->texchannel);
    if (stage->texchannel >= VID_TEXCOORD_CHANNELS){
      lprintf("WARNING shdload: illegal texcoord channel\n");
      stage->texchannel = 0;
    }
    return;
  }
  if (strcmp(command,"texclamp")==0) {
    int states[3];
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    if (3==sscanf(buffer,"(%d,%d,%d)",&states[0],&states[1],&states[2])){
      int i;
      for (i = 0; i < 3; i++)
        if (states[i])
          stage->texclamp |= 1<<i;
    }
    return;
  }
  if (strcmp(command,"texgenplane")==0){
    int axis;
    lxVector4 plane;
    float *vec4;
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    if (stage->stagetype == SHADER_STAGE_TEX && 5==sscanf(buffer,"%d (%f,%f,%f,%f)",&axis,&plane[0],&plane[1],&plane[2],&plane[3])){
      if (!stage->texgen)
        stage->texgen = reszalloc(sizeof(ShaderTexGen_t));
      axis = LUX_MAX(axis,0);
      axis %= 4;

      stage->texgen->enabledaxis[axis] = LUX_TRUE;
      vec4 =  &stage->texgen->texgenmatrix[4*axis];
      lxVector4Copy(vec4,plane);
    }
  }
  if (strcmp(command,"rgbscale2")==0) {
    stage->rgbscale = 1;
    return;
  }
  if (strcmp(command,"rgbscale4")==0) {
    stage->rgbscale = 2;
    return;
  }
  if (strcmp(command,"alphascale2")==0) {
    stage->alphascale = 1;
    return;
  }
  if (strcmp(command,"alphascale4")==0) {
    stage->alphascale = 2;
    return;
  }
  if (strcmp(command,"lightmapscale")==0) {
    stage->stageflag |= SHADER_LMRGBSCALE;
    return;
  }
  if (strcmp(command,"nomipmap")==0) {
    stage->srcAttributes &= ~TEX_ATTR_MIPMAP;
    return;
  }
  if (strcmp(command,"param")==0) {
    parseShaderParam(buffer,rest);
    return;
  }
}
void fileLoadSHDTex()
{
  static char *words[100]={ "TEX","TEXALPHA","VTEX","VTEXALPHA","TEXCUBE","TEXPROJ","VTEXCUBE","VTEXPROJ","TEXDOTZ",
                "VTEXDOTZ","param",
                "spheremap","screenmap","reflectmap","worldlinmap","eyelinmap","skyreflectmap","normalmap",
                "alphamode","blendmode","blendinvertalpha","vertexcolored","texcoord","sunnormalmap","sunreflectmap",
                "texclamp","texgenplane","alphascale4","alphascale2","rgbscale2","rgbscale4","lightmapscale",
                "lightnormalmap0","lightnormalmap1","lightnormalmap2","lightnormalmap3","lightreflectmap0","lightreflectmap1","lightreflectmap2","lightreflectmap3",
                "depthcompare","depthvalue","nomipmap",
                "ATTENUATE3D","LIGHTMAP","NORMALIZE","SKYBOX","SPECULAR","DIFFUSE","DUMMY",
                "\0"};
  ShaderStage_t *stage;

  if (!l_SHDData.stagesTex[l_SHDData.stagesTexGpuInBuffer])
    l_SHDData.stagesTex[l_SHDData.stagesTexGpuInBuffer] =reszalloc(sizeof(ShaderStage_t));

  stage = l_SHDData.stagesTex[l_SHDData.stagesTexGpuInBuffer];
  ShaderStage_clear(stage);
  stage->stagetype = SHADER_STAGE_TEX;
  stage->srcAttributes = TEX_ATTR_MIPMAP;

  stage->blendmode = VID_UNSET;
  l_SHDData.stage = stage;
  FileParse_stage(l_SHDData.file,words,loadTex,NULL);

  switch (stage->blendmode){
    case VID_UNSET:
      if (!l_SHDData.stagesTexInBuffer && l_SHDData.stagesColorInBuffer > 0 && !(stage->stageflag & SHADER_VCOLOR)){
        stage->blendmode = VID_MODULATE;
      }
      else if (l_SHDData.stagesTexInBuffer > 0)
        stage->blendmode = VID_MODULATE;
      else
        stage->blendmode = VID_REPLACE;
      break;
  }
}

// GPU Program
// -----------

void GPUnameset(char *rest, int isskin, char **srcname, char**entryname)  {
  char name[1024];
  char namebuf[40] = {0};
  int num = 0;
  const char *next = lxStrReadInQuotes(rest, name, 1023);
  sscanf(rest,"%i",&num);
  num%=(VID_MAX_LIGHTS+1);

  resnewstrcpy(srcname[num], name);
  if (next){
    lxStrReadInQuotes(next, name, 1023);
    if (isskin){
      if (strstr(name,RES_CGC_SEPARATOR)==NULL)
        strcat(name,RES_CGC_SEPARATOR);
      sprintf(namebuf,"-DVID_BONES=%d;",g_VID.gensetup.hwbones);
      strcat(name,namebuf);
    }
    resnewstrcpy(entryname[num], name);
  }
  else if (isskin){
    sprintf(namebuf,"%s-DVID_BONES=%d;",RES_CGC_SEPARATOR,g_VID.gensetup.hwbones);
    resnewstrcpy(entryname[num], namebuf);
  }
}


void loadGPU(void * target,char * command, char* rest)
{
  char buffer[1000];
  int bufferpos=0,restpos=0;
  int isskin = LUX_FALSE;
  ShaderStage_t *stage = l_SHDData.stage;

  //"VPROG","FPROG","VCg","FCg","VFixed","FFixed"
  // Generic
  if (strcmp(command,"VPROG")==0) {
    stage->srcType = GPUPROG_V_ARB;
    return;
  }
  if (strcmp(command,"FPROG")==0) {
    stage->srcType = GPUPROG_F_ARB;
    return;
  }
  if (strcmp(command,"VGLSL")==0) {
    stage->srcType = GPUPROG_V_GLSL;
    return;
  }
  if (strcmp(command,"FGLSL")==0) {
    stage->srcType = GPUPROG_F_GLSL;
    return;
  }
  if (strcmp(command,"GGLSL")==0) {
    stage->srcType = GPUPROG_G_GLSL;
    return;
  }
  if (strcmp(command,"VCG")==0) {
    stage->srcType = GPUPROG_V_CG;
    return;
  }
  if (strcmp(command,"FCG")==0) {
    stage->srcType = GPUPROG_F_CG;
    return;
  }
  if (strcmp(command,"GCG")==0) {
    stage->srcType = GPUPROG_G_CG;
    return;
  }
  if (strcmp(command,"VFIXED")==0) {
    stage->srcType = GPUPROG_V_FIXED;
    return;
  }
  if (strcmp(command,"FFIXED")==0) {
    stage->srcType = GPUPROG_F_FIXED;
    return;
  }
  if (strcmp(command,"GFIXED")==0) {
    stage->srcType = GPUPROG_G_FIXED;
    return;
  }
  if (strcmp(command,"lowCgProfile")==0){
    stage->stageflag |= SHADER_CG_LOWPROFILE;
    return;
  }
  if (strcmp(command,"BASE")==0) {
    GPUnameset(rest,LUX_FALSE,stage->gpuinfo->srcNames,stage->gpuinfo->srcEntryNames);
    return;
  }
  if ((isskin=strcmp(command,"SKIN"))==0 || strcmp(command,"FOGGED")==0) {
    GPUnameset(rest,isskin,stage->gpuinfo->srcSkinNames,stage->gpuinfo->srcSkinEntryNames);
    return;
  }
  if (strcmp(command,"tangents")==0) {
    l_SHDData.shader->shaderflag |= SHADER_TANGENTS;
    stage->stageflag |= SHADER_TANGENTS;
    return;
  }
  if (strcmp(command,"normals")==0) {
    l_SHDData.shader->shaderflag |= SHADER_NORMALS;
    stage->stageflag |= SHADER_NORMALS;
    return;
  }
  if (strcmp(command,"texbarrier")==0 && GLEW_NV_texture_barrier) {
    stage->stageflag |= SHADER_TEXBARRIER;
    return;
  }
  if (strcmp(command,"param")==0) {
    parseShaderParam(buffer,rest);
    return;
  }

}

#undef GPUNAMESET

void fileLoadSHDGPU()
{
  static char *words[100]={ "VPROG","FPROG","VCG","FCG","GCG","FFIXED","VFIXED","GFIXED",
                "BASE",
                "SKIN",
                "FOGGED",
                "lowCgProfile",
                "param","tangents","normals","texbarrier","\0"};
  ShaderStage_t *stage;
  if (!l_SHDData.stagesTex[l_SHDData.stagesTexGpuInBuffer])
    l_SHDData.stagesTex[l_SHDData.stagesTexGpuInBuffer] =reszalloc(sizeof(ShaderStage_t));

  l_SHDData.shader->shaderflag |= SHADER_NOCOMPILE;

  stage = l_SHDData.stagesTex[l_SHDData.stagesTexGpuInBuffer];
  ShaderStage_clear(stage);
  stage->stagetype = SHADER_STAGE_GPU;
  stage->gpuinfo = reszalloc(sizeof(ShaderGpuInfo_t));

  stage->blendmode = VID_UNSET;
  stage->stageflag |= SHADER_VCOLOR | SHADER_VCOLOR_KEEP;
  l_SHDData.stage = stage;
  FileParse_stage(l_SHDData.file,words,loadGPU,NULL);
}

void ShaderBuildFromBuffer(Shader_t *shader)
{
  int i,n;
  int offset;
  int needsworldinv = LUX_FALSE;
  ShaderParam_t *parambrowse;
  // copy stages
  offset = 0;
  shader->numStages = l_SHDData.stagesColorInBuffer + l_SHDData.stagesTexGpuInBuffer;
  shader->stages = reszalloc(sizeof(ShaderStage_t*)*shader->numStages);
  shader->totalParams = reszalloc(sizeof(ShaderParam_t*)*shader->numTotalParams);

  // we will need manual matrix paramters
  shader->cgMode = l_SHDData.cgmode == -1 ? SHADER_PASSMODE_FIXEDARB : l_SHDData.cgmode;


  if (!(shader->shaderflag & SHADER_NOCOMPILE))
    l_SHDData.colorStage = 0;

  n = 0;
  for (i = 0; i < l_SHDData.stagesColorInBuffer + l_SHDData.stagesTexGpuInBuffer; i++){
    if (i == l_SHDData.colorStage && l_SHDData.stagesColorInBuffer)
      shader->stages[i]=l_SHDData.stagesColor;
    else{
      ShaderStage_t *stage =  l_SHDData.stagesTex[n];
      shader->stages[i]=stage;
      if (stage->paramListHead){
        lxListNode_forEach(stage->paramListHead,parambrowse)
          shader->totalParams[parambrowse->totalid] = parambrowse;
          needsworldinv |= ShaderParam_needsWorldInv(parambrowse);
        lxListNode_forEachEnd(stage->paramListHead,parambrowse);
      }
      if (stage->stagetype == SHADER_STAGE_TEX){
        if (stage->stageflag & (SHADER_EYELINMAP | SHADER_OBJLINMAP) && !stage->texgen){
          stage->stageflag &= ~(SHADER_EYELINMAP | SHADER_OBJLINMAP);
          stage->texgen = NULL;
          lprintf("WARNING shdload: no texgenplanes defined,%s\n",shader->resinfo.name);
        }
        if (stage->texgen){
          uchar legalcombos[] = { LUX_TRUE,LUX_TRUE,LUX_FALSE,LUX_FALSE,  // XY
                        LUX_TRUE,LUX_TRUE,LUX_TRUE,LUX_FALSE, // XYZ
                        LUX_TRUE,LUX_TRUE,LUX_FALSE,LUX_TRUE, // XYW
                        LUX_TRUE,LUX_TRUE,LUX_TRUE,LUX_TRUE}; // XYZW
          enum32  legalmodes[]  = { VID_TEXCOORD_TEXGEN_ST,
                        VID_TEXCOORD_TEXGEN_STR,
                        VID_TEXCOORD_TEXGEN_STQ,
                        VID_TEXCOORD_TEXGEN_STRQ};

          uint *pIn = (uint*)legalcombos;
          uint *pCur = (uint*)stage->texgen->enabledaxis;
          int c;

          for (c = 0; c < 4; c++,pIn++){
            if (*pIn == *pCur){
              stage->texgen->texgenmode = legalmodes[c];
              break;
            }
          }

          if (!stage->texgen){
            stage->texgen = NULL;
            stage->stageflag &= ~(SHADER_EYELINMAP | SHADER_OBJLINMAP);
            lprintf("WARNING shdload: illegal combination of texgenplanes defined,%s\n",shader->resinfo.name);
          }
        }
      }
      n++;
    }
  }

  if (needsworldinv)
    shader->shaderflag |= SHADER_NEEDINVMATRIX;


  // check if textured
  if (l_SHDData.stagesTexInBuffer)
    shader->shaderflag |= SHADER_TEXTURED;

  // nicer to work with, if first stage = color replace & second stage = tex modulate, we "encode" the modulate via vcolor flag
  if (  l_SHDData.stagesColorInBuffer && l_SHDData.stagesColor->blendmode == VID_REPLACE
    &&  l_SHDData.stagesTexInBuffer)
    for (i = 0; i < shader->numStages; i++){
      if (shader->stages[i]->stagetype == SHADER_STAGE_TEX ){
        if (shader->stages[i]->blendmode == VID_MODULATE && !(isFlagSet(shader->stages[i]->stageflag,SHADER_VCOLOR))){
          shader->stages[i]->blendmode = VID_REPLACE;
          shader->stages[i]->stageflag |= SHADER_VCOLOR | SHADER_VCOLOR_KEEP;
        }
        break;
      }
    }


  // if there is a bumpstage, we will remove lighting info from all other stages
  if (l_SHDData.bumpStage != -1){
    shader->shaderflag |= SHADER_TANGENTS;
    for (i= 0; i < shader->numStages; i++)
      shader->stages[i]->stageflag &= ~(SHADER_VCOLOR | SHADER_VCOLOR_KEEP);
  }
}

#undef BUFFERSTAGES
