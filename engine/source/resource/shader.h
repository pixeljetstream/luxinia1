// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __SHADER_H__
#define __SHADER_H__

/*
SHADER
------

A Shader defines the way a mesh is rendered. It controls
passes blendmodes and texture environment.
Depending on Hardware a shader effect can use different
amount of passes. The real setup is done
in /render/gl_shader

See SHDScript manual what you can do with them. They
are defined externally in files normally called by
a Material

Shader  (nested,ResMem)

Author: Christoph Kubisch

*/

#include "../common/vid.h"
#include "../render/gl_render.h"
#include "texture.h"
#include "material.h"

typedef enum  ShaderStageType_e{
  SHADER_STAGE_COLOR    ,
  SHADER_STAGE_TEX    ,
  SHADER_STAGE_GPU    ,
  SHADER_STAGE_NEWPASS  ,
  SHADER_STAGE_COLORPASS  ,
}ShaderStageType_t;

// may only be 0 = Fixed/ARB, 1 = Cg
enum ShaderPassMode_e{
  SHADER_PASSMODE_FIXEDARB  = 0,
  SHADER_PASSMODE_CG      = 1,
};

enum ShaderFlag_e{
  // Stages
  SHADER_NONE     = 0,
  SHADER_SPHEREMAP  = 1<<0,
  SHADER_SCREENMAP  = 1<<1,
  SHADER_REFLECTMAP = 1<<2,
  SHADER_EYELINMAP  = 1<<3,
  SHADER_NORMALMAP  = 1<<4,
  SHADER_VCOLOR   = 1<<5,
  SHADER_VCOLOR_KEEP  = 1<<6,
  SHADER_INVALPHA   = 1<<7,
  SHADER_SPECIAL    = 1<<8,
  SHADER_SKYMAP   = 1<<9,
  SHADER_SUNREFMAP    = 1<<10,
  SHADER_SUNNORMALMAP   = 1<<11,
  SHADER_OBJLINMAP    = 1<<12,
  SHADER_LMRGBSCALE   = 1<<13,
  SHADER_ISNORMALMAP    = 1<<14,
  SHADER_DEPTHCOMPARE   = 1<<15,
  SHADER_DEPTHNOCOMPARE = 1<<16,
  SHADER_CG_LOWPROFILE  = 1<<17,
  SHADER_TEXBARRIER =   1<<18,

  // Shader
  SHADER_TEXTURED   = 0,
  SHADER_HWSKIN   = 1<<0,
  SHADER_FGPU     = 1<<1,
  SHADER_VGPU     = 1<<2,
  SHADER_GGPU     = 1<<3,
  SHADER_HWBATCHABLE  = 1<<4,
  SHADER_NOCOMPILE  = 1<<5,
  SHADER_VCG      = 1<<6,
  SHADER_VARB     = 1<<7,
  SHADER_VGLSL    = 1<<8,
  SHADER_NEEDINVMATRIX= 1<<9,

  // both
  SHADER_CUBETEXGEN = 1<<28,
  SHADER_NORMALS    = 1<<29,
  SHADER_TANGENTS   = 1<<30,
  SHADER_TEXGEN   = 1<<31,


}ShaderFlag_e;

typedef enum ShaderParamType_e{
  SHADER_PARAM_UNSET,
  SHADER_PARAM_VALUE,
  SHADER_PARAM_ARRAY,
  SHADER_PARAM_STREAM,

  SHADER_PARAM_CAMUP,
  SHADER_PARAM_CAMPOS,
  SHADER_PARAM_CAMDIR,

  SHADER_PARAM_TIME,
  SHADER_PARAM_RANDOM,
  SHADER_PARAM_LIGHTDIR,
  SHADER_PARAM_LIGHTPOS,
  SHADER_PARAM_LIGHTCOLOR,
  SHADER_PARAM_LIGHTAMBIENT,
  SHADER_PARAM_LIGHTRANGE,
  SHADER_PARAM_TEXCONST,
  SHADER_PARAM_TEXSIZE,
  SHADER_PARAM_TEXSIZEINV,
  SHADER_PARAM_TEXLMSCALE,

  SHADER_PARAM_TEXGEN0,
  SHADER_PARAM_TEXGEN1,
  SHADER_PARAM_TEXGEN2,
  SHADER_PARAM_TEXGEN3,

  SHADER_PARAM_TEXMAT0,
  SHADER_PARAM_TEXMAT1,
  SHADER_PARAM_TEXMAT2,
  SHADER_PARAM_TEXMAT3,
}ShaderParamType_t;

typedef struct ShaderParam_s{
  int     totalid;

  ShaderParamType_t type;
  char    *name;

  CGparameter cgparam;    // for Cg, combo = unique per shader, else it is a copy of shared parameter by gpuprog
  enum32    id;       // for ARB

  int     upvalue;    // OFTEN means which stage, VALUE mul with winv, ARRAY means sizeof array
  int     upvalue2;   // TEXMAT/TEXGEN 1=premul with worldmat, ARRAY
  float   *vectorp;
  float   vector[4];
  struct ShaderParam_s  *prev,*next;
}ShaderParam_t;

typedef struct ShaderGpuInfo_s{
  VID_GPU_t   vidtype;

  int       srcRIDs[VID_MAX_LIGHTS+1];
  char      *srcNames[VID_MAX_LIGHTS+1];
  char      *srcEntryNames[VID_MAX_LIGHTS+1];

  int       srcSkinRIDs[VID_MAX_LIGHTS+1];
  char      *srcSkinNames[VID_MAX_LIGHTS+1];
  char      *srcSkinEntryNames[VID_MAX_LIGHTS+1];
}ShaderGpuInfo_t;

typedef struct ShaderTexGen_s{
  lxMatrix44    texgenmatrix;
  int       texgenmode;
  uchar     enabledaxis[4];
}ShaderTexGen_t;

typedef struct ShaderAlphaTex_s{
  int         id;
  enum32        srcType;
  enum32        srcAttributes;
  int         srcRID;
  char        *srcName;
  VID_TexChannel_t  texchannel;
}ShaderAlphaTex_t;

typedef struct ShaderStage_s{
  ShaderStageType_t stagetype;
  enum32        stageflag;
  enum32        stageparam;

  int         id;     // set to MATERIAL_COLOR0 if srcname is set
  int         colorid;
  int         texclamp;
  int         rgbscale;
  int         alphascale;

  enum32        srcType;  // texturetypes or gputype
  union{
    // STAGE_NEWPASS
    struct  {
      enum32      renderflag;

      enum32      rflagson;
      enum32      rflagsoff;
    };
    // STAGE TEX,GPU
    struct  {
      int         srcRID;
      enum32        srcAttributes;
      char        *srcName;
      VID_TexChannel_t  texchannel;
    };
  };

  VIDBlendColor_t blendmode;
  VIDBlendAlpha_t alphamode;

  union{
    ShaderGpuInfo_t   *gpuinfo;
    ShaderTexGen_t    *texgen;
  };

  ShaderParam_t   *paramListHead;
  int         numVectorParams;
  int         numArrayParams;

  struct ShaderStage_s LUX_LISTNODEVARS;
}ShaderStage_t;

typedef struct Shader_s{
  ResourceInfo_t  resinfo;
  Char2PtrNode_t  *annotationListHead;

  int   cgMode;       // may only be 0 = Fixed/ARB, 1 = Cg
  VID_Technique_t tech;
  enum32  shaderflag;     // shaderflag
  short nolock;
  short layer;        // layer + 1, 0 is unset
  ShaderAlphaTex_t  *alphatex;

  // defaults for all assigned
  enum32    renderflag;
  VIDAlpha_t  *alpha;
  VIDBlend_t  *blend;

  int       numStages;
  ShaderStage_t **stages; // the real content

  int       numTotalParams;
  ShaderParam_t **totalParams;

  int   colorPass;
  int   numMaxTexPass;    // number of maximum stages used per pass
  VID_TexChannel_t  *texchannels; // texchannel setup in that max pass (only if numPasses = 1)
  int   numPasses;      // number of render passes must not be equal to stages
  struct GLShaderPass_s   *glpassListHead;  // this can be lower than stages if we can make use of extensions

}Shader_t;
// BlastaRID

///////////////////////////////////////////////////////////////////////////////
// Shader

#define SHADER_CANCOMPILE(shader) (!isFlagSet(shader->shaderflag,SHADER_NOCOMPILE))

ShaderStage_t*  Shader_searchStageRID(Shader_t *shader,const int id);


int Shader_searchParam(Shader_t *shader, const char *name,  int pass);
  // unrefs resources
void Shader_clear(Shader_t *shader);

int Shader_getTextureStageIndex(Shader_t *shader, int resid);

///////////////////////////////////////////////////////////////////////////////
// ShaderStage

void ShaderStage_clear(ShaderStage_t *stage);

//////////////////////////////////////////////////////////////////////////
// ShaderParam

int ShaderParam_needsWorldInv(ShaderParam_t *param);

#endif
