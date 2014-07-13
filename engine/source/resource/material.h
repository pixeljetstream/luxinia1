// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __MATERIAL_H__
#define __MATERIAL_H__

/*
MATERIAL
--------

The Material handles texture setup for the shader it uses.
For further detail see the MTLScript reference, as Materials
are defined externally
MaterialObject is a instance of a material's animation parameters.

Material  (nested, ResMem)
MaterialObject (user,GenMem)
MaterialAutoCtrl (user RefCtrled,GenMem)

Author: Christoph Kubisch

*/

#include "../common/types.h"
#include "../resource/texture.h"
#include "../common/linkedlistprimitives.h"
#include "../common/reflib.h"

#define MATERIAL_MAX_CONTROLS 64
#define MATERIAL_MAX_MODIFIERS  64

#define MATERIAL_MAX_SHADERS  4
#define VID_MAX_SHADER_STAGES 16

#define MATERIAL_AUTOSTART    "MATERIAL_AUTO:"
#define MATERIAL_AUTOSTART_LEN  14

enum MaterialFlag_e{
  MATERIAL_MODOFF = 1,
  MATERIAL_SEQOFF = 2,
  MATERIAL_ANIMATED = 4,
};

typedef enum MaterialControlType_e{
  MATERIAL_CONTROL_FLOAT,
  MATERIAL_CONTROL_RESTEX,
  MATERIAL_CONTROL_RESSHD,
  MATERIAL_CONTROLS,
}MaterialControlType_t;

typedef enum MaterialStageRID_e{
  MATERIAL_NONE,
  MATERIAL_TEX0,
  MATERIAL_TEX1,
  MATERIAL_TEX2,
  MATERIAL_TEX3,
  MATERIAL_TEX4,
  MATERIAL_TEX5,
  MATERIAL_TEX6,
  MATERIAL_TEX7,
  MATERIAL_TEX8,
  MATERIAL_TEX9,
  MATERIAL_TEX10,
  MATERIAL_TEX11,
  MATERIAL_TEX12,
  MATERIAL_TEX13,
  MATERIAL_TEX14,
  MATERIAL_TEX15,
  MATERIAL_COLOR0,
  MATERIAL_UNUSED,
}MaterialStageRID_t;

enum MaterialModifierFlag_e {
  MATERIAL_MOD_NONE = 0,
  MATERIAL_MOD_ADD = 1,
  MATERIAL_MOD_SIN = 2,
  MATERIAL_MOD_COS = 4,
  MATERIAL_MOD_ZIGZAG = 8,

  MATERIAL_MOD_CAPPED = 16,

  MATERIAL_MOD_CAP0 = 32,
  MATERIAL_MOD_CAP1 = 64,
  MATERIAL_MOD_CAP2 = 128,
  MATERIAL_MOD_OFF  = 256,
};

typedef enum MaterialValue_e{
  MATERIAL_MOD_RGBA_1,
  MATERIAL_MOD_RGBA_2,
  MATERIAL_MOD_RGBA_3,
  MATERIAL_MOD_RGBA_4,
  MATERIAL_MOD_TEXMOVE_1,
  MATERIAL_MOD_TEXMOVE_2,
  MATERIAL_MOD_TEXMOVE_3,
  MATERIAL_MOD_TEXCENTER_1,
  MATERIAL_MOD_TEXCENTER_2,
  MATERIAL_MOD_TEXCENTER_3,
  MATERIAL_MOD_TEXSCALE_1,
  MATERIAL_MOD_TEXSCALE_2,
  MATERIAL_MOD_TEXSCALE_3,
  MATERIAL_MOD_TEXROT_1,
  MATERIAL_MOD_TEXROT_2,
  MATERIAL_MOD_TEXROT_3,
  MATERIAL_MOD_TEXMAT0_1,
  MATERIAL_MOD_TEXMAT0_2,
  MATERIAL_MOD_TEXMAT0_3,
  MATERIAL_MOD_TEXMAT0_4,
  MATERIAL_MOD_TEXMAT1_1,
  MATERIAL_MOD_TEXMAT1_2,
  MATERIAL_MOD_TEXMAT1_3,
  MATERIAL_MOD_TEXMAT1_4,
  MATERIAL_MOD_TEXMAT2_1,
  MATERIAL_MOD_TEXMAT2_2,
  MATERIAL_MOD_TEXMAT2_3,
  MATERIAL_MOD_TEXMAT2_4,
  MATERIAL_MOD_TEXMAT3_1,
  MATERIAL_MOD_TEXMAT3_2,
  MATERIAL_MOD_TEXMAT3_3,
  MATERIAL_MOD_TEXMAT3_4,
  MATERIAL_MOD_TEXGEN0_1,
  MATERIAL_MOD_TEXGEN0_2,
  MATERIAL_MOD_TEXGEN0_3,
  MATERIAL_MOD_TEXGEN0_4,
  MATERIAL_MOD_TEXGEN1_1,
  MATERIAL_MOD_TEXGEN1_2,
  MATERIAL_MOD_TEXGEN1_3,
  MATERIAL_MOD_TEXGEN1_4,
  MATERIAL_MOD_TEXGEN2_1,
  MATERIAL_MOD_TEXGEN2_2,
  MATERIAL_MOD_TEXGEN2_3,
  MATERIAL_MOD_TEXGEN2_4,
  MATERIAL_MOD_TEXGEN3_1,
  MATERIAL_MOD_TEXGEN3_2,
  MATERIAL_MOD_TEXGEN3_3,
  MATERIAL_MOD_TEXGEN3_4,
  MATERIAL_MOD_PARAM_1, // nothing allowed afterwards
  MATERIAL_MOD_PARAM_2,
  MATERIAL_MOD_PARAM_3,
  MATERIAL_MOD_PARAM_4,
}MaterialValue_t;

typedef enum MaterialACtrlType_e{
  // single vector4
  MATERIAL_ACTRL_VEC4_NODEPOS,
  MATERIAL_ACTRL_VEC4_NODEDIR,
  MATERIAL_ACTRL_VEC4_NODEPOS_PROJ,

  MATERIAL_ACTRL_VEC4MAT_SEP,

  // matrix or texgen
  MATERIAL_ACTRL_MAT_NODEROT,
  MATERIAL_ACTRL_MAT_NODEPOS_PROJ,
  MATERIAL_ACTRL_MAT_PROJECTOR,
}MaterialACtrlType_t;

typedef struct MaterialAutoControl_s{
  Reference       reference;

  MaterialACtrlType_t   type;

  int           targetRefType;    // -1 l3d, 0 s3d, 1 act
  Reference       targetRef;      // weak reference

  lxVector4         vec4;
}MaterialAutoControl_t;

// MATERIAL_t Modifier
typedef struct MaterialModifier_s{
  char*   name;
  MaterialValue_t   modtarget;    // value

  void*       data;
  int         dataID;

  enum32    modflag;    // what kind of mod and caps
  uchar   perframe;   // update per frame ?

  uint    delay;      // delay in ms
  float   delayfrac;    // 1/delay
  uint    lastTime;   // when last update occured
  float   runner;     // for sin,cos,zigzag 0 - 2

  float   sourceval;    // source value
  float   value;      // mod value
  float   *sourcep;
} MaterialModifier_t;

typedef struct MaterialControl_s{
  char        *name;
  MaterialValue_t   modtarget;

  void*       data;
  int         dataID;

  int         offset;
  int         length;
  void*       upvalue;

  float       *sourcep;
}MaterialControl_t;

typedef struct MaterialResControl_s{
  MaterialControlType_t type;
  char          *name;
  int           *sourcewinsizedp; // points to dumy for shd
  int           *sourcep;
}MaterialResControl_t;

typedef struct MaterialTexCoord_s{
  // - 4 DW aligned
  lxMatrix44    matrix;
  union{
    struct{
      lxVector3   center;
      lxVector3   rotate;
      lxVector3   move;
      lxVector3   scale;
    };
    lxMatrix44    usermatrix;
  };
  int     usemanual;
}MaterialTexCoord_t;

// Extra Information for sequences
typedef struct MaterialSeq_s{
  int       numFrames;    // number of items in seq

  uint      delay;      // delay between items in ms
  float     delayfrac;    // 1/delay

  uchar     loops;      // number of loops 0 = played once, 255 = played infinetely
  uchar     interpolate;  // interpolate per frame

  int       wait;     // wait time between loops  if - it will be used as flag to perform wait
  int       reverse;    // reverse Anim_t every nth loop
  int       numLooped;    // number already played

  uint      activeFrame;  // active list item
  uint      lastTime;   // when last update occured

  float     *colors;
  char      **textures;
  int       *texRID;
  TextureType_t *texTypes;
}MaterialSeq_t;

typedef struct MaterialStage_s{
  MaterialStageRID_t  id;


  float     *pMatrix;
  float     *pColor;
  int       texRID;
  int       texRIDUse;
  int       texclamp;

  float     color[4];
  TextureType_t texType;
  enum32      texAttributes;
  char      *texName;
  int       windowsized;

  MaterialSeq_t   *seq;
  MaterialTexCoord_t  *texcoord;
  float       *texgen;
}MaterialStage_t;

typedef struct MaterialParam_s{
  char    *name;
  int     pass;
  int     arraysize;
  int     totalid;
  lxVector4   vec;
  float   *vecP;
}MaterialParam_t;

typedef struct MaterialAlpha_s{
  int     pass;
  float   val;
  float   *valP;
}MaterialAlpha_t;

typedef struct MaterialObject_s{
  // 4 DW
  uint    _pad;
  uint    time;
  enum32    materialflag;
  float   *texmatrix;
  // - 4 DW aligned
  lxMatrix44  texmatrixData;

  int           numControls;
  int           numControlValues;
  MaterialAutoControl_t **mautoctrlControls;
  float         *controlValues;         // holds values of all controllables

  int           numStages;
  MaterialAutoControl_t **mautoctrlStages;    // numStages*2 size, matrix or texgen autoctrl

  int     numResControls;
  int     *resRIDs;     // 2 values per ResControl
}MaterialObject_t;

typedef struct MaterialShader_s{
  int         resRID;   // resRID of shader
  int         resRIDUse;  // modified by ResControl
  char        *name;    // shader
  char        *extname;

  float       rfalpha;

  MaterialAlpha_t   *alphas;
  int         numAlphas;

  MaterialParam_t   *params;
  int         numParams;
  int         numShaderTotalParams; // modified by ResControl
}MaterialShader_t;

typedef struct Material_s{
  ResourceInfo_t    resinfo;
  // - 4 DW aligned
  MaterialObject_t  matobjdefault;
  MaterialObject_t  *matobj;
  MaterialObject_t  *lastmatobj;
  ulong       lastmatobjframe;

  int         noTexgenOrMat;

  MaterialShader_t    shaders[MATERIAL_MAX_SHADERS];

  MaterialStage_t     **stages;
  int           numStages;

  MaterialModifier_t    *mods;
  int           numMods;

  MaterialControl_t   *controls;
  int           numControls;
  int           numControlValues;

  MaterialResControl_t  *rescontrols;
  int           numResControls;

  Char2PtrNode_t    *annotationListHead;
}Material_t;


///////////////////////////////////////////////////////////////////////////////
// Material

//sets default states and returns renderflag
enum32 Material_getDefaults(Material_t *mat, struct Shader_s **oshader, VIDAlpha_t *alpha, VIDBlend_t *blend, VIDLine_t *line);

int  Material_setFromString(Material_t *mat,char *command);

  // need proper drawsetup
void Material_setVID(Material_t *mat, const int shader);
void Material_update(Material_t *material, MaterialObject_t *matobject);

  // unrefs own resources
void Material_clear(Material_t *material);

void Material_initMatObject(Material_t *material);
void Material_initModifiers(Material_t *material);
void Material_initControls(Material_t *material);
void Material_initResControls(Material_t *material);
booln Material_initShaders(Material_t *material);
booln Material_checkShaderLink(Material_t *material, int ctrlid, struct Shader_s *shader);

int  Material_getStageRIDTexture(Material_t *material,int stageid);

int  Material_getTextureStageIndex(Material_t *material, int texid);

///////////////////////////////////////////////////////////////////////////////
// MaterialObject

  // if NULL is passed all data is 0
MaterialObject_t *MaterialObject_new(Material_t *mat);
void MaterialObject_free(MaterialObject_t *matobj);
void MaterialObject_unrefAutoControls(MaterialObject_t *matobj);
void MaterialObject_setAutoControl(MaterialObject_t *matobj,MaterialAutoControl_t **ptr,MaterialAutoControl_t *autoctrl);

///////////////////////////////////////////////////////////////////////////////
// MaterialAutoControl
MaterialAutoControl_t* MaterialAutoControl_new();

//void MaterialAutoControl_free(MaterialAutoControl_t *mautoctrl);
void RMaterialAutoControl_free(lxRmatautocontrol ref);

///////////////////////////////////////////////////////////////////////////////
// MaterialTexCoord

void MaterialTexCoord_clear(MaterialTexCoord_t *texcoord);

///////////////////////////////////////////////////////////////////////////////
// MaterialModifier

void MaterialModifier_init(MaterialModifier_t *mod,char *name, int type ,int delay, float value, uchar cap, void* data, int dataID);
void MaterialModifier_initSource(MaterialModifier_t *mod,float *source,int capped,MaterialValue_t target);

//////////////////////////////////////////////////////////////////////////
// MaterialControl
void MaterialControl_init(MaterialControl_t *ctrl,char *name, void *data, int dataID, int length);
void MaterialControl_initSource(MaterialControl_t *ctrl,float *source,MaterialValue_t target);

  // controltype= 0: floatcontrol, 1 texontrol, -1 shdcontrol
int Material_getControlIndex(Material_t *mat, const char *name, MaterialControlType_t controltype);
int MaterialControl_getOffsetsFromID(int ctrlid,int inmatRID, int* offset, int *vlength);

#endif
