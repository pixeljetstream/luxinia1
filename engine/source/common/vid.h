// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __VID_H__
#define __VID_H__

#include <gl/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <Cg/cg.h>
#include <Cg/cgGL.h>

#pragma comment( lib, "cg.lib" )
#pragma comment( lib, "cggl.lib" )

#include "../common/common.h"
#include "../common/mesh.h"
#include "../common/vidbuffer.h"

// luxiniaGL
#define VID_MAX_TEXTURE_IMAGES  16
#define VID_MAX_TEXTURE_UNITS 4
#define VID_MAX_SHADER_STAGES 16
#define VID_MAX_BUFFERS     2048

#define VID_MAX_LIGHTS      4
#define VID_FXLIGHTS      (VID_MAX_LIGHTS-1)
#define VID_MAX_VERTICES    32768
#define VID_MAX_INDICES     65535
#define VID_MAX_SHADERPARAMS  128
#define VID_MAX_USERCOMBINERS 256

#define VID_LOGOS       4
#define VID_CUBEMAP_SIZE    64
#define VID_LIGHTMAP_3D_SIZE  32

// SCREEN REFERENCE coordinate system (for 2d works)
#define VID_REF_WIDTH     g_VID.refScreenWidth
#define VID_REF_HEIGHT      g_VID.refScreenHeight
#define VID_REF_WIDTHSCALE    (g_VID.refScreenWidth/((float)g_VID.drawbufferWidth+0.001f))
#define VID_REF_HEIGHTSCALE   (g_VID.refScreenHeight/((float)g_VID.drawbufferHeight+0.001f))
#define VID_REF_WIDTH_TOPIXEL(w)  ((int)(((float)g_VID.drawbufferWidth*(float)w)/g_VID.refScreenWidth))
#define VID_REF_HEIGHT_TOPIXEL(h) ((int)(((float)g_VID.drawbufferHeight*(float)h)/g_VID.refScreenHeight))

  // keep in synch with RESDATA_MAX_TEXTURES
#define VID_OFFSET_MATERIAL   512

#define VID_TEXCOMBSTRING "VIDTC_"

typedef enum VID_GPUVendor_e{
  VID_VENDOR_UNKNOWN,
  VID_VENDOR_NVIDIA,
  VID_VENDOR_ATI,
  VID_VENDOR_INTEL,
  VID_VENDOR_FORCE_DWORD = 0x7FFFFFFF
}VID_GPUVendor_t;

typedef enum VID_TexCoord_e{
  VID_TEXCOORD_ARRAY = -1,
  VID_TEXCOORD_NONE = 0,
  VID_TEXCOORD_TEXGEN_ST ,
  VID_TEXCOORD_TEXGEN_STR ,
  VID_TEXCOORD_TEXGEN_STQ ,
  VID_TEXCOORD_TEXGEN_STRQ,
  VID_TEXCOORD_FORCE_DWORD = 0x7FFFFFFF
}VID_TexCoord_t;

// detail
typedef enum VID_Detail_e{
  VID_DETAIL_UNSET,
  VID_DETAIL_LOW,
  VID_DETAIL_MED,
  VID_DETAIL_HI,
  VID_DETAIL_FORCE_DWORD = 0x7FFFFFFF
}VID_Detail_t;
// GPU order must not change
typedef enum VID_GPU_e{
  VID_FIXED,
  VID_ARB,
  VID_GLSL,
  VID_GPUMODE_FORCE_DWORD = 0x7FFFFFFF
}VID_GPU_t;

typedef enum VID_GPUprogram_e{
  VID_GPU_ARB,
  VID_GPU_GLSL,
  VID_GPU_CG,
  VID_GPU_FORCE_DWORD = 0x7FFFFFFF
}VID_GPUprogram_t;

typedef enum VID_GPUdomain_e{
  VID_GPU_VERTEX,
  VID_GPU_FRAGMENT,
  VID_GPU_GEOMETRY,
  VID_GPU_DOMAINS,
  VID_TECHNIQUE_FORCE_DWORD = 0x7FFFFFFF
}VID_GPUdomain_t;

// BASE SHADERS
enum VID_Shaders_e{
  VID_SHADER_COLOR,
  VID_SHADER_COLOR_LM,
  VID_SHADER_TEX,
  VID_SHADER_TEX_LM,
  VID_SHADERS,
  VID_SHADERS_FORCE_DWORD = 0x7FFFFFFF
};

// Techniques sorted by ascending priority
typedef enum VID_Technique_e{
  VID_T_UNSET,
  VID_T_DEFAULT,    // should work with most hardware
  VID_T_ARB_TEXCOMB,  // texcombiners
  VID_T_ARB_V,    // ARB_vertex_program
  VID_T_ARB_VF,   // ARB_vertex_program and ARB_fragment_program
  VID_T_ARB_VF_TEX8,
  VID_T_CG_SM3,
  VID_T_CG_SM4,
  VID_T_CG_SM4_GS,  // geometry shader
  VID_T_LOWDETAIL,  // user set details to LOW
  VID_TECHNIQUES_FORCE_DWORD = 0x7FFFFFFF,
}VID_Technique_t;


// special combiners/blendmodes
typedef enum  VIDBlendColor_e{
  VID_UNSET       ,
  VID_REPLACE     ,
  // tex
  VID_REPLACE_PREV      ,
  // prev
  VID_REPLACE_VERTEX      ,
  // vertex
  VID_REPLACE_CONST     ,
  // const
  VID_MODULATE      ,
  // prev*tex
  VID_MODULATE2     ,
  // prev*tex*2
  VID_PASSTHRU,
  // prev (also for alpha)

  VID_MIN,
  VID_MAX,
  VID_SUB,
  VID_AMODSUB,
  VID_SUB_REV,
  VID_AMODSUB_REV,

  VID_DECAL       ,
  // prev LERP(tex.alpha) tex
  VID_DECAL_PREV      ,
  // prev LERP(prev.alpha) tex
  VID_DECAL_VERTEX      ,
  // prev LERP(vertex.alpha) tex
  VID_DECAL_CONST   ,
  // prev LERP(const.alpha) tex


  VID_DECAL_CONSTMOD,
  // crossbar needed
  // will turn to VID_DECAL_PREV
  // prev alpha = tex*const.alpha


  VID_DECALCLR        ,
  // prev LERP(tex.color) tex
  VID_DECALCLR_PREV     ,
  // prev LERP(prev.color) tex
  VID_DECALCLR_VERTEX     ,
  // prev LERP(vertex.color) tex
  VID_DECALCLR_CONST    ,
  // prev LERP(const.color) tex


  VID_ADD       ,
  // prev + tex
  VID_ADD_INV     ,
  // prev + tex.invert
  VID_ADD_VERTEX    ,
  // prev + vertex
  VID_ADD_CONST     ,
  // prev + tex.const
  VID_AMODADD     ,
  // prev + tex*tex.alpha
  VID_AMODADD_PREV    ,
  // prev + tex*prev.alpha
  VID_AMODADD_VERTEX    ,
  // prev + tex*vertex.alpha
  VID_AMODADD_CONST   ,
  // prev + tex*const.alpha

  VID_AMODADD_CONSTMOD,
  // crossbar needed
  // will turn to VID_AMODADD_PREV
  // prev alpha = tex*const.alpha

// internal helpers
  VID_AMOD        ,
  // tex*tex.alpha
  VID_AMOD_PREV     ,
  // tex*prev.alpha
  VID_AMOD_VERTEX     ,
  // tex*vertex.alpha
  VID_AMOD_CONST      ,
  // tex*const.alpha

  VID_CMOD      ,
  // tex*vertex
  VID_CMOD_PREV       ,
  //  prev*vertex
  VID_SQUARE      ,
  //  color*color

  VID_CMODADD_VERTEX    ,
  // prev + tex*vertex.color
  VID_ADDCMOD_VERTEX    ,
  // prev*vertex.color + tex*vertex.color

// crossbar functionality
  // same as normal ones just that action
  // is between tex0 & tex1
  VID_ADD_C       ,
  VID_DECAL_C         ,
  VID_DECAL_PREV_C    ,
  VID_DECAL_VERTEX_C    ,
  VID_DECAL_CONST_C   ,
  // operation is between
  // texX & prev
  VID_DECAL_C2      ,
  VID_DECAL_C3      ,

  VID_DECALCLR_C          ,
  VID_DECALCLR_PREV_C   ,
  VID_DECALCLR_VERTEX_C   ,
  VID_DECALCLR_CONST_C    ,
  VID_DECALCLR_C2     ,
  VID_DECALCLR_C3     ,

  VID_COLOR_USER,
  VID_COLOR_FORCE_DWORD = 0x7FFFFFFF,
}VIDBlendColor_t;

typedef enum VIDBlendAlpha_e{
  VID_A_UNSET,
  VID_A_SETBYCOLOR,
  VID_A_REPLACE,
  // tex
  VID_A_REPLACE_CONST,
  // const
  VID_A_REPLACE_VERTEX,
  // vertex
  VID_A_REPLACE_PREV,
  // prev
  VID_A_MODULATE,
  // prev*tex
  VID_A_CMOD,
  // tex*vertex
  VID_A_CMOD_PREV,
  // prev*vertex

// crossbar
  VID_A_CONSTMOD_C,
  // const*tex1
  VID_A_CONSTMOD_C2,
  VID_A_CONSTMOD_C3,

  VID_ALPHA_USER,

  VID_ALPHA_FORCE_DWORD = 0x7FFFFFFF,
}VIDBlendAlpha_t;

// hardcoded vertex programs
typedef enum  VIDVertexPrograms_e{
  VID_VPROG_PARTICLE_BATCH,
  VID_VPROG_PARTICLE_BATCH_LM,
  VID_VPROG_PARTICLE_BATCH_ROT,
  VID_VPROG_PARTICLE_BATCH_ROT_LM,
  VID_VPROG_PARTICLE_BATCH_ROT_GLOBALAXIS,
  VID_VPROG_PARTICLE_BATCH_ROT_GLOBALAXIS_LM,

  VID_VPROG_PARTICLE_INSTANCE,
  VID_VPROG_PARTICLE_INSTANCE_LM,
  VID_VPROG_PARTICLE_INSTANCE_LIT,
  VID_VPROG_PARTICLE_INSTANCE_LIT_LM,
  VID_VPROG_PARTICLE_INSTANCE_GLOBALAXIS,
  VID_VPROG_PARTICLE_INSTANCE_GLOBALAXIS_LM,
  VID_VPROG_PARTICLE_INSTANCE_GLOBALAXIS_LIT,
  VID_VPROG_PARTICLE_INSTANCE_GLOBALAXIS_LIT_LM,

  VID_VPROG_SKIN_WEIGHT_UNLIT,
  VID_VPROG_SKIN_WEIGHT_LIT1,
  VID_VPROG_SKIN_WEIGHT_LIT2,
  VID_VPROG_SKIN_WEIGHT_LIT3,
  VID_VPROG_SKIN_WEIGHT_LIT4,

  VID_VPROG_OPTIONAL_PROGS,

  VID_VPROG_BUMP_POSINV_UNLIT,
  VID_VPROG_BUMP_POSINV_LIT1,
  VID_VPROG_BUMP_POSINV_LIT2,
  VID_VPROG_BUMP_POSINV_LIT3,
  VID_VPROG_BUMP_POSINV_LIT4,

  VID_VPROG_PROJECTOR,

  VID_VPROGS,
  VID_VPROGS_FORCE_DWORD = 0x7FFFFFFF,
}VIDVertexPrograms_t;

typedef enum VIDTexBlendDefault_e{
  VID_TEXBLEND_MOD_MOD,
  VID_TEXBLEND_MOD_REP,
  VID_TEXBLEND_REP_REP,
  VID_TEXBLEND_REP_MOD,
  VID_TEXBLEND_PREV_REP,
  VID_TEXBLEND_MOD_PREV,
  VID_TEXBLEND_MOD2_PREV,
  VID_TEXBLEND_MOD4_PREV,
  VID_TEXBLENDS,
  VID_TEXBLENDS_FORCE_DWORD = 0x7FFFFFFF,
}VIDTexBlendDefault_t;

typedef struct VIDBlend_s{
  VIDBlendColor_t blendmode;
  VIDBlendAlpha_t alphamode;

  booln     blendinvert;
}VIDBlend_t;

typedef struct VIDAlpha_s{
  GLenum      alphafunc;
  GLfloat     alphaval;
}VIDAlpha_t;

typedef struct VIDLine_s{
  float     linewidth;
  ushort      linefactor;
  ushort      linestipple;
}VIDLine_t;

typedef struct VIDDrawState_s{
  uint32      hash;
  flags32     renderflag;
  VIDBlend_t    blend;
  VIDAlpha_t    alpha;
  VIDLine_t   line;
}VIDDrawState_t;

typedef struct VIDStencilOp_s
{
  GLenum      fail;
  GLenum      zfail;
  GLenum      zpass;
}VIDStencilOp_t;

typedef struct VIDStencil_s{
  short     enabled;
  short     twosided;
  GLenum      funcF;
  GLenum      funcB;
  int       refvalue;
  enum32      mask;
  VIDStencilOp_t  ops[2]; // 0 = front 1 = back
}VIDStencil_t;

typedef struct VIDDepth_s{
  GLenum      func;
}VIDDepth_t;

typedef struct VIDLogicOp_s{
  GLenum      op;
}VIDLogicOp_t;

typedef struct VIDViewBounds_s{
  int         fullwindow;
  union{
    struct{
      int     x;
      int     y;
      int     width;
      int     height;
    };
    int       sizes[4];
  };
}VIDViewBounds_t;

typedef struct VIDViewPort_s{
  VIDViewBounds_t bounds;
  GLfloat     depth[2];
}VIDViewPort_t;

typedef struct VIDTexCombiner_s{
  GLenum      mode;
  short     combine4;
  short     numCmds;
  GLenum      srcs[4];
  GLenum      ops[4];
}VIDTexCombiner_t;

// must stay 16bit wide !!
typedef enum RenderFlag {
  RENDER_NONE=             0,
  RENDER_FRONTCULL  =   1<<0,     // front faces are culled
  RENDER_NOVERTEXCOLOR =1<<1,     // disable color array (only works for VA or VBO, or if part of shader)
  RENDER_NODEPTHMASK =  1<<2,     // dont write in depthmask
  RENDER_STENCILMASK =  1<<3,
  RENDER_FOG =          1<<4,     // is affected by fog
  RENDER_ALPHATEST =    1<<5,     // use alphatest
  RENDER_NOCULL =       1<<6,     // draw the backfaces or not
  RENDER_NORMALIZE =    1<<7,     // normals renormalized
  RENDER_NOCOLORMASK =  1<<8,     // wont write into depth mask
  RENDER_BLEND =        1<<9,     // blending
  RENDER_STENCILTEST =  1<<10,
  RENDER_NODEPTHTEST =  1<<11,    // dont make depthtest
  RENDER_LIT =          1<<12,    // is being lit
  RENDER_SUNLIT =       1<<13,
  RENDER_WIRE =         1<<14,
  RENDER_UNUSED =       1<<15,
  RENDER_LASTFLAG =     1<<15,    // last flag of state changes

  // non state changes start outside of short
  RENDER_NODRAW =     1<<26,      // wont be drawn
  RENDER_FXLIT =      1<<27,      // if set surface is affected by effectlights
  RENDER_PROJPASS =   1<<28,      // forces projectors into new passes
  RENDER_NEVER =      1<<29,      // special user flag
  RENDER_UNLIT =      1<<30,      // override (for materials)
  RENDER_NOLIGHTMAP =   1<<31,      // override
} RenderFlag;

typedef struct VIDShaderSetup_s{
  int     lightmapRID;        // id for current lightmap
  int     textures[VID_MAX_SHADER_STAGES+2];
  float   *colors[VID_MAX_SHADER_STAGES+2];
  float   *texmatrixStage[VID_MAX_SHADER_STAGES+2];
  float   *texgenStage[VID_MAX_SHADER_STAGES+2];
  float   *shaderparams[VID_MAX_SHADERPARAMS];
  float   *texmatrix;
  float   *lmtexmatrix;
}VIDShaderSetup_t;

typedef struct VIDShaders_s{
  int     override;
  int     ids[VID_SHADERS];
}VIDShaders_t;

typedef struct VIDVertexAttribs_s{
  VIDVertexPointer_t  pointers[VERTEX_ATTRIBS];
  flags32       used;
  flags32       needed;
}VIDVertexAttribs_t;

typedef struct VIDTexCoordSource_e{
  VID_TexCoord_t    type;
  VID_TexChannel_t  channel;
}VIDTexCoordSource_t;

typedef struct VIDGPUState_s{
  VID_GPU_t vertex;           // is a vertex program bound if what kind (ARB,CG,FIXED)
  VID_GPU_t fragment;           // is a fragment program bound if what kind (ARB,CG,FIXED)
  VID_GPU_t geometry;           // geometry shader
}VIDGPUState_t;


typedef struct VIDState_s{
  int       activeTex;      // active texture unit
  int       lasttexStages;
  int       activeClientTex;

  int       textures[VID_MAX_TEXTURE_IMAGES];   // currently bound texture
  VIDTexBlendHash texhash[VID_MAX_TEXTURE_IMAGES];    // currently bound texenv

  GLenum      textargets[VID_MAX_TEXTURE_IMAGES]; // currently active target (tex2d, 3d, cube..)
  VIDTexCoordSource_t texcoords[VID_MAX_TEXTURE_IMAGES];  // currenntly used texcoord

  VIDTexBlendHash texhashdefaults[VID_TEXBLENDS];

  const VIDBuffer_t*  activeBuffers[VID_BUFFERS];

  int activeVertex;
  int activeFragment;

  // state
  VIDGPUState_t gpustate;

  flags32 renderflag;
  flags32 renderflagforce;
  flags32 renderflagignore;
  GLenum  stencilmaskpos;
  GLenum  stencilmaskneg;

  int   scissor;

  VIDViewPort_t viewport;
  VIDAlpha_t    alpha;
  VIDDepth_t    depth;
  VIDBlend_t    blend;
  GLenum        blendlastequ;
  VIDStencil_t  stencil;
  float         fogcolor[4];

}VIDState_t;

typedef struct VIDDrawSetup_s{
  const float   *projMatrix;
  const float   *projMatrixInv;
  const float   *viewMatrix;
  const float   *viewMatrixInv;
  const float   *viewProjMatrix;

  const float   *worldMatrix;
  const float   *renderscale;
  const float   *rendercolor;

  // ortho mode
  const float   *oprojMatrix;
  const float   *oprojMatrixInv;
  const float   *oviewMatrix;
  const float   *oviewMatrixInv;
  const float   *oviewProjMatrix;

  lxMatrix44SIMD  worldViewProjMatrix;
  lxMatrix44SIMD  worldViewMatrix;
  lxMatrix44SIMD  worldMatrixInv;
  lxMatrix44SIMD  worldMatrixCache;
  lxMatrix44SIMD  texwindowMatrix;
  lxMatrix44SIMD  viewSkyrotMatrix;
  lxMatrix44SIMD  orthoProjMatrix;
  lxMatrix44SIMD  orthoProjMatrixInv;

  lxMatrix44SIMD  skyrotMatrix;

  float     skyrot[4];
  float     rendercolorCache[4];

  // hwskinning
  int       hwskin;         // use hwskinning
  const float   *skinMatrices;
  int       numSkinMatrices;

  // draw specifc
  VIDVertexAttribs_t  attribs;
  int         useshaders;
  VIDShaders_t    shaders;
  VIDShaders_t    shadersdefault;
  int         setnormals;
  int         purehwvertex; // if set we will turn off any costly state changes ie lighting and texgen

  void    *curnode;
  //float   *drw_texcoords[VID_MAX_TEXTURE_IMAGES]; // vertex space for manual texcoords


  int     lightCG;
  int     lightCnt;
  int     lightCntLast;
  int     light[VID_MAX_LIGHTS];
  const struct Light_s  *lightPt[VID_MAX_LIGHTS];
  const struct Light_s  *lightPtGL[VID_MAX_LIGHTS];

  uint    boneFrameSkip;        // frames we skip for bone updates
  uint    boneSkip;
}VIDDrawSetup_t;

typedef struct VIDGenSetup_s{
  // particle specific
  GLUquadricObj *quadobj;

  // shader specific
  int     vprogRIDs[VID_VPROGS];  // resRIDs for hardcoded gpuprogs
  int     logoRIDs[4];
  int     normalizeRID;       // resid for cubemap
  int     attenuate3dRID;     // resid for 3d attenuate
  int     specularRID;
  int     diffuseRID;
  int     whiteRID;
  int     texdefaultRID;

  int     hwbones;
  int     hwbonesparams;


  int     hasVprogs;

  int         tccolorCnt;
  int         tcalphaCnt;
  VIDTexCombiner_t  tcalpha[VID_MAX_USERCOMBINERS];
  VIDTexCombiner_t  tccolor[VID_MAX_USERCOMBINERS];


  VIDStencil_t zfail;
  VIDStencil_t zpass;
  uint    shadowvalue;
  void (*stencilmaskfunc) (GLuint mask);

  int     batchMaxIndices;
  int     batchMeshMaxTris;
}VIDGenSetup_t;

typedef struct VIDCgState_s{
  // Cg
  CGcontext   context;
  int       prefercompiled;
  int       dumpallcompiled;
  int       dumpbroken;
  int       useoptimal;
  int       allowglsl;
  int       optimalinited;

  // GLSL stuff
  int GLSL; // in case the system might use one GLSL profile
  CGprofile glslFragmentProfile;
  CGprofile glslVertexProfile;

  // ARB prfiles
  CGprofile arbVertexProfile;
  CGprofile arbFragmentProfile;

  // optimal profiles
  CGprofile optVertexProfile;
  CGprofile optFragmentProfile;
  CGprofile optGeometryProfile;

  // active profiles
  CGprofile vertexProfile;
  CGprofile fragmentProfile;
  CGprofile geometryProfile;

  CGparameter boneMatrices;

  CGparameter bbvertexOffsets;
  CGparameter bbtexOffsets;
  CGparameter bbtexwidth;
  CGparameter bbglobalAxis;
  CGparameter prtlmTexgen;

  CGparameter projMatrix;
  CGparameter projMatrixInv;
  CGparameter viewMatrix;
  CGparameter viewMatrixInv;
  CGparameter viewProjMatrix;
  CGparameter worldMatrix;
  CGparameter worldViewMatrix;
  CGparameter worldViewProjMatrix;

  CGparameter fogcolor;
  CGparameter fogdistance;
  CGparameter camwpos;
  CGparameter camwdir;

  CGparameter viewsize;
  CGparameter viewsizeinv;
}VIDCgState_t;

typedef struct VID_s{
  float refScreenWidth;
  float refScreenHeight;
  int windowWidth;
  int windowHeight;
  int drawbufferWidth;
  int drawbufferHeight;

  VIDState_t      state;
  VIDShaderSetup_t  shdsetup;
  VIDDrawSetup_t    drawsetup;
  VIDGenSetup_t   gensetup;
  VIDCgState_t    cg;

  // capabilities
  const VIDBuffer_t*  defaultBuffers[VID_BUFFERS];
  int   usevbos;
  int   capVBO;
  int   capTexSize;           // max tex size
  int   capTex3DSize;
  int   capTexUnits;          // max tex classic units
  int   capTexCoords;         // max tex coord units
  int   capTexImages;         // max tex image units
  int   capTexVertexImages;
  int   capIsCrap;
  int   capDrawBuffers;
  int   capTexArray;
  int   capFBOMSAA;
  float capTexAnisotropic;
  float capPointSize;
  uint  capVRAM;

  byte  capHWShadow;
  byte  capBump;
  byte  capTexCompress;
  byte  capTwoSidedStencil;

  byte  capTexDDS;
  byte  capTexFloat;
  byte  capTexInt;
  byte  capTexBuffer;

  byte  capPBO;
  byte  capRangeBuffer;
  byte  capFeedback;
  byte  capFBO;

  short capCPYBO;
  short capFBOBlit;


  VID_Technique_t capTech;
  VID_Technique_t origTech;

  // renderpath
  VID_GPUVendor_t gpuvendor;

  float lvlTexAnisotropic;
  int useTexCompress;
  int useTexAnisotropic;
  int     useworst;
  int     force2tex;
  int     novprogs;

  enum32    details;          // detail setting

  // meshes
  Mesh_t    *drw_meshbuffer;      // used for particles, terrain decals, batchindices
  Mesh_t    *drw_meshquad;
  Mesh_t    *drw_meshquadffx;
  Mesh_t    *drw_meshquadcentered;
  Mesh_t    *drw_meshquadcenteredffx;
  Mesh_t    *drw_meshbox;
  Mesh_t    *drw_meshsphere;
  Mesh_t    *drw_meshcylinder;



  // stats / misc
  int     configset;
  unsigned long frameCnt;
  int       maxFps;
  int     swapinterval;
  int     screenshots;
  int     autodump;
  int     autodumptga;
  int     screenshotquality;
  float   white[4];
  float   black[4];
}VID_t;

extern VID_t    g_VID;

//////////////////////////////////////////////////////////////////////////
// Error Checking

int vidCgCheckErrorF(char *filename,int line);
int vidCheckErrorF(char *filename,int line);

//////////////////////////////////////////////////////////////////////////
// VID System

void VID_clear();   // clear all
void VID_deinitGL();  // destroys used GL data
void VID_initGL();    // reset GL data used for drawing
int  VID_init();    // fills glState with proper capacities and queries extensions
void VID_deinit();    // shuts down non GL data
void VID_info(int start, void (*fnprint)(char *string, ...));
const char*   VID_GetOSString();
int VID_CheckSpecialTexs(const char *str);

void VIDState_getGL();
booln VIDState_testGL();
void vidScreenShot(int tga);

const char* VID_GPUVendor_toString(VID_GPUVendor_t gpuvendor);

//////////////////////////////////////////////////////////////////////////
// Viewport

int  vidViewport(int x, int y, int width, int height);  // returns TRUE on change, also calls ScissorCheck then
void vidViewportScissorCheck();

void VIDViewBounds_set(VIDViewBounds_t *vbounds,booln fullwindow, int x, int y, int w, int h);
void vidDepthRange(float from, float to);

void vidCgViewProjUpdate();
void vidOrthoOn(float left, float right, float bottom, float top, float front, float back);
void vidOrthoOff();

//////////////////////////////////////////////////////////////////////////
// Texture

void vidResetTexture();

// rgb/alphascale 0= 1.0, 1 = 2.0f, 2 = 4.0f
void vidTexBlend(const VIDTexBlendHash hash, const VIDBlendColor_t blendmode,  VIDBlendAlpha_t alphamode,
         const booln blendinvert, const int rgbscale, const int alphascale);
void vidTexBlendDefault(VIDTexBlendDefault_t mode);

void vidTexBlendSet(const VIDTexBlendHash hash, const VIDBlendColor_t blendmode,  VIDBlendAlpha_t alphamode,
          const booln blendinvert, const int rgbscale, const int alphascale);

VIDTexBlendHash VIDTexBlendHash_get(const VIDBlendColor_t blendmode,  VIDBlendAlpha_t alphamode,
                  const booln blendinvert, const int rgbscale, const int alphascale);
void vidTexBlendSetFromHash(const VIDTexBlendHash hash);

void vidBlend(const VIDBlendColor_t blendmode,const booln blendinvert);

//////////////////////////////////////////////////////////////////////////
// VIDState

void vidStateDefault();
void vidStateReset();
void vidStateDisable();

void VIDDepth_init(VIDDepth_t *viddepth);
void VIDDepth_getGL(VIDDepth_t *viddepth);
void VIDLine_init(VIDLine_t *vidline);
void VIDLine_setGL(const VIDLine_t *vidline);
void VIDStencil_init(VIDStencil_t *vidstencil);
  // side = -1 both, 0 front, 1 back
void VIDStencil_setGL(const VIDStencil_t *vidstencil,int side);
void VIDStencil_getGL(VIDStencil_t *vidstencil);
void VIDBlend_getGL(VIDBlend_t *blend);
void VIDAlpha_getGL(VIDAlpha_t* alpha);
void VIDViewPort_getGL(VIDViewPort_t *viewport);
void VIDLogicOp_setGL(VIDLogicOp_t *logic);
void VIDLogicOp_getGL(VIDLogicOp_t *logic);
//////////////////////////////////////////////////////////////////////////
// VIDRenderFlag

enum32  VIDRenderFlag_fromString(char *names);
void    VIDRenderFlag_setGL(enum32 flags);
enum32  VIDRenderFlag_getGL();
void    VIDRenderFlag_disable();
//////////////////////////////////////////////////////////////////////////

const char * VIDBlendAlpha_toString(VIDBlendAlpha_t type);
const char * VIDBlendColor_toString(VIDBlendColor_t type);
const char * VIDTechnique_toString(VID_Technique_t type);

VIDBlendAlpha_t   VIDBlendAlpha_fromString(const char *name);
VIDBlendColor_t   VIDBlendColor_fromString(const char *name);
VIDTexCombiner_t* VIDTexCombiner_alpha(const char *name);
VIDTexCombiner_t* VIDTexCombiner_color(const char *name);
int         VIDTexCombiner_prepCmd(VIDTexCombiner_t* comb, const GLenum cmd, const booln isalpha);
enum32        VIDTexCombiner_genFromString(int isalpha,const char *name);

int     VIDCompareMode_run(GLenum cmpmode, float val, float ref);


//////////////////////////////////////////////////////////////////////////
// VIDUserDefine

void    VIDUserDefine_add(const char *name);
void    VIDUserDefine_remove(const char *name);
void*   VIDUserDefine_get(const char *name);


//////////////////////////////////////////////////////////////////////////
// VIDCg

void    VIDCg_initProfiles();
void    VIDCg_linkProgramConnectors(CGprogram cgprog);
void    VIDCg_linkProgramSemantics(CGprogram cgprog, CGenum name_space);
void    VIDCg_setConnector(const char *name, CGparameter host);
CGparameter VIDCg_getConnector(const char *name);


//////////////////////////////////////////////////////////////////////////
// Misc

lxScalarType_t ScalarType_fromGL(GLenum type);
GLenum ScalarType_toGL(lxScalarType_t type);

#include "vidinl.h"

#endif
