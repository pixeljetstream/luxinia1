// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_RENDER_H__
#define __GL_RENDER_H__

// vidOpenGL
#include "../common/types.h"
#include "../common/reflib.h"
#include "../common/vid.h"
#include "gl_draw2d.h"

#define FBO_MAX_DRAWBUFFERS 4

#define R2VB_MAX_ATTRIBS  16
#define R2VB_MAX_BUFFERS  1

typedef enum FBOAssignType_e{
  FBO_ASSIGN_COLOR_0,
  FBO_ASSIGN_COLOR_1,
  FBO_ASSIGN_COLOR_2,
  FBO_ASSIGN_COLOR_3,
  FBO_ASSIGN_DEPTH,
  FBO_ASSIGN_STENCIL,
  FBO_ASSIGNS,
}FBOAssignType_t;

typedef enum FBOTarget_e{
  FBO_TARGET_DRAW,
  FBO_TARGET_READ,
  FBO_TARGETS,
}FBOTarget_t;

typedef struct RenderHelpers_s{
  float normalLen;
  float axisLen;
  float boneLen;
  float texsize;

  int   drawVisFrom;
  int   drawVisTo;

  int drawNoPrts;
  int drawNoTexture;
  int drawNormals;
  int drawBBOXwire;

  int drawBBOXsolid;
  int drawBones;
  int drawWire;
  int drawAxis;

  int drawNoGUI;
  int drawCamAxis;

  int drawPath;
  int drawLights;
  int drawProj;

  int drawFPS;
  int drawStats;
  int drawAll;
  int forceFinish;
  int statsFinish;

  int drawBonesNames;
  int drawBonesLimits;
  int drawShadowVolumes;
  int drawStencilBuffer;

  int   drawSpecial;
  int   drawVisSpace;
  int   drawPerfGraph;

  lxVector4 statsColor;
}RenderHelpers_t;


//////////////////////////////////////////////////////////////////////////
// RenderBackground

typedef struct RenderBackground_s
{
  enum32    fogmode;    // if 0 then no foggin is done
  float   fogcolor[4];
  float   fogstart;
  float   fogend;
  float   fogdensity;

  struct SkyBox_s *skybox;

  Reference skyl3dnoderef;
}RenderBackground_t;

//////////////////////////////////////////////////////////////////////////
// CustomMesh

typedef struct RenderMesh_s{
  Mesh_t      *mesh;
  booln     freemesh;

  Reference   reference;

  Reference   vboref;
  Reference   iboref;
}RenderMesh_t;

//////////////////////////////////////////////////////////////////////////
// FBRenderBuffer

typedef struct FBRenderBuffer_s{
  Reference reference;
  uint    glID;
  enum32    textype;    // GL_DEPTH_COMPONENT...
  int     width;
  int     height;
  int     winsized;
  int     multisamples;

  struct FBRenderBuffer_s *prev,*next;  // in allocation
}FBRenderBuffer_t;


//////////////////////////////////////////////////////////////////////////
// FBObject

typedef struct FBTexAssign_s{
  int     texRID;
  short   offset; // negative 3d+1, positive cubeside-1
  short   mip;
}FBTexAssign_t;

typedef struct FBObject_s{
  Reference reference;
  uint    glID;

  int     winsized;
  int     width;
  int     height;

  FBTexAssign_t   texassigns[FBO_ASSIGNS];
  FBRenderBuffer_t  *rbassigns[FBO_ASSIGNS];

  struct FBObject_s *prev,*next;  // in allocation
}FBObject_t;

//////////////////////////////////////////////////////////////////////////
// HWBufferObject

typedef struct HWBufferObject_s{
  VIDBuffer_t       buffer;
  Reference       reference;

  booln         keeponloss;
  void*         tempcopy;

  struct HWBufferObject_s **listhead;
  struct HWBufferObject_s *prev,*next;
}HWBufferObject_t;


//////////////////////////////////////////////////////////////////////////
// RenderCmd

typedef void (Rcmd_run_fn)(void* pcmd, struct List3DView_s* view, struct List3DDrawState_s *lstate);

typedef struct RenderCmd_s{
  lxClassType   type;
  Reference   reference;
  enum32      runflag;
  Rcmd_run_fn   *fnrun;
}RenderCmd_t; // 4 DW

typedef struct RenderCmdMesh_s{
  RenderCmd_t     cmd;
  // 4 DW
  lxMatrix44SIMD    matrix;
  int         autosize; // 0 not, 1 window, -1 view
  Reference     linkref;
  booln       ortho;
  lxVector3     size;
  lxVector4     color;
  DrawMesh_t      dnode;
  Reference     usermesh;
}RenderCmdMesh_t;

typedef struct RenderCmdClear_s{
  RenderCmd_t   cmd;
  enum32    clear;
  int     clearcolor;
  int     cleardepth;
  int     clearstencil;

  int     clearmode;  // 0 = float, 1 = i, 2 = ui

  union{
    float   color[4];
    int     colori[4];
    int     colorui[4];
  };

  float   depth;
  int     stencil;
}RenderCmdClear_t;

typedef struct RenderCmdTexcopy_s{
  RenderCmd_t   cmd;
  int     autosize;
  int     texRID;
  int     texside;
  int     mip;
  int     texX;
  int     texY;
  int     screenX;
  int     screenY;
  int     screenWidth;
  int     screenHeight;
}RenderCmdTexcopy_t;

typedef struct RenderCmdTexupd_s{
  RenderCmd_t   cmd;
  int       texRID;
  int       texside;

  int       offset;
  GLenum      dataformat;

  int       pos[4];
  int       size[3];

  VIDBuffer_t   *buffer;
  lxRvidbuffer  bufferref;
}RenderCmdTexupd_t;

typedef struct RenderCmdRead_s{
  RenderCmd_t   cmd;

  GLenum      format;
  GLenum      dataformat;

  int       rect[4];
  int       offset;

  VIDBuffer_t   *buffer;
  lxRvidbuffer  bufferref;
}RenderCmdRead_t;

typedef struct RenderCmdFBBlit_s{
  RenderCmd_t   cmd;
  int     copycolor;
  int     copydepth;
  int     copystencil;

  int     linear;

  int     srcX;
  int     srcY;
  int     srcWidth;
  int     srcHeight;

  int     dstX;
  int     dstY;
  int     dstWidth;
  int     dstHeight;
}RenderCmdFBBlit_t;


typedef struct R2VBAttrib_s{
  int     id;
  int     components;
  int     index;
}R2VBAttrib_t;


typedef struct R2VBuffer_s{
  VIDBuffer_t   *vbuf;
  uint      offset;
  uint      size;
}R2VBuffer_t;

typedef struct RenderCmdR2VB_s{
  RenderCmd_t     cmd;
  booln       capture;
  booln       noraster;

  lxRrcmddrawmesh inputmesh;
  RenderCmdMesh_t   *rcmdmesh;

  int         numBuffers;
  R2VBuffer_t     buffers[R2VB_MAX_BUFFERS];
  int         numAttribs;
  R2VBAttrib_t    attribs[R2VB_MAX_ATTRIBS];

  int         output;
  GLuint        queryobj;
}RenderCmdR2VB_t;

typedef struct RenderCmdFlag_s{
  RenderCmd_t   cmd;
  union{
    VIDStencil_t    stencil;
    VIDDepth_t      depth;
    VIDLogicOp_t    logic;
    struct{
      int       lights;
      int       projs;
    }ignore;
    union{
      enum32      renderflag;
      int       texRID;
    };
    struct{
      VIDShaders_t    shaders;
    }shd;
    struct{
      FBRenderBuffer_t  *fborbassigns[FBO_ASSIGNS];
      FBOTarget_t     fbotarget;
    }fborb;
    struct{
      FBObject_t    *fbo;
      FBOTarget_t   fbotarget;
      int       viewportchange; // default TRUE
    }fbobind;
    struct{
      int       numBuffers;
      int       buffers[FBO_MAX_DRAWBUFFERS];
    }fbobuffers;
    struct{
      int       buffer;
    }fboread;
  };

}RenderCmdFlag_t;

typedef struct RenderCmdFBTex_s{
  RenderCmd_t   cmd;
  FBTexAssign_t fbotexassigns[FBO_ASSIGNS];
  FBOTarget_t   fbotarget;
}RenderCmdFBTex_t;

typedef struct RenderCmdDraw_s{
  RenderCmd_t   cmd;
  union{
    struct{
      int     layer;
      int     sort;   // 0 no, 1 state -1 ftb -2 btf
    }normal;
  };

}RenderCmdDraw_t;

typedef struct RenderCmdDrawL2D_s{
  RenderCmd_t     cmd;
  Reference     targetref;
  struct List2DNode_s *node;
  lxVector2     refsize;
  int         viewportsizes[4];
  lxVector4     viewrefsizes;
}RenderCmdDrawL2D_t;

typedef struct RenderCmdDrawL3D_s{
  RenderCmd_t     cmd;
  Reference     targetref;
  struct List3DNode_s *node;
  int         startid;
  int         numNodes;
  int         force;
}RenderCmdDrawL3D_t;

typedef struct RenderCmdFnCall_s{
  RenderCmd_t       cmd;
  // 4 DW
  booln         ortho;
  lxMatrix44SIMD      matrix;
  lxVector3       size;
  lxRcmdfncall_func_fn* fncall;
  void*         upvalue;

}RenderCmdFnCall_t;

typedef union{
  void        *ptr;
  RenderCmd_t     *cmd;
  lxClassType   *type;
  RenderCmdDraw_t   *draw;
  RenderCmdDrawL2D_t  *drawl2d;
  RenderCmdDrawL3D_t  *drawl3d;
  RenderCmdFlag_t   *flag;
  RenderCmdTexcopy_t  *texcopy;
  RenderCmdClear_t  *clear;
  RenderCmdMesh_t   *mesh;
  RenderCmdFBTex_t  *fbotex;
  RenderCmdFBBlit_t *fboblit;
  RenderCmdRead_t   *read;
  RenderCmdR2VB_t   *r2vb;
  RenderCmdTexupd_t *texupd;
  RenderCmdFnCall_t *fncall;
}RenderCmdPtr_t;

//////////////////////////////////////////////////////////////////////////

typedef struct RenderData_s{
  FBObject_t      *fboListHead;
  HWBufferObject_t  *vboListHead;
  FBRenderBuffer_t  *fbrenderbufferListHead;
}RenderData_t;

// GLOBALS
extern RenderHelpers_t    g_Draws;
extern RenderBackground_t *g_Background;

void Render(void);

void Render_initGL();

void Render_pushGL();
void Render_popGL();
void Render_onWindowResize();

void Render_init(int numL3DSets, int numL3DLayers, int numL3DLayerNodes, int numL3DTotalProjectors);
void Render_deinit();

void RenderBackground_setGL(RenderBackground_t *rbackg);
void RenderBackground_render(RenderBackground_t *rback, enum32 forceflag, enum32 ignoreflag);

void RenderHelpers_init();


//////////////////////////////////////////////////////////////////////////
// RenderCmd

void  RRenderCmd_free(lxRrcmd cmd);
char *  RenderCmd_toString(lxClassType type);
RenderCmd_t* RenderCmd_new(lxClassType type);

void  RenderCmdClear_run(RenderCmdClear_t *cmd, struct List3DView_s* view, struct List3DDrawState_s *lstate);

  // FBO related
void  RenderCmdPtr_runFBO(RenderCmdPtr_t pcmd, FBObject_t *pfboTargets[FBO_TARGETS], const int *origviewbounds);

//////////////////////////////////////////////////////////////////////////
// FBRenderBuffer

FBRenderBuffer_t* FBRenderBuffer_new();
int   FBRenderBuffer_set(FBRenderBuffer_t* rb,enum32 type,int w,int h, int windowsized, int multisamples);
//void  FBRenderBuffer_free(FBRenderBuffer_t* rb);

void  RFBRenderBuffer_free(lxRrenderbuffer ref);

//////////////////////////////////////////////////////////////////////////
// FBObject
enum32    FBOTarget_getBind(FBOTarget_t target);
const char* FBOTarget_checkCompleteness(FBOTarget_t fbotarget);

FBObject_t* FBObject_new();
void  FBObject_setSize(FBObject_t* fbo,int w,int h,int windowsized);
void  FBObject_reset(FBObject_t* fbo);  // legal to pass NULL
void  FBObject_applyAssigns(FBObject_t* fbo, RenderCmdPtr_t texassign, RenderCmdPtr_t rbassign);
const char* FBObject_check(FBObject_t* fbo);
//void  FBObject_free(FBObject_t* fbo);

void  RFBObject_free(lxRrenderfbo ref);

//////////////////////////////////////////////////////////////////////////
// HWBufferObject

HWBufferObject_t* HWBufferObject_new(
    VIDBufferType_t type, VIDBufferHint_t hint, uint size, void* data);

void HWBufferObject_listop(HWBufferObject_t* buffer, HWBufferObject_t **list);
void RHWBufferObject_free(lxRvidbuffer ref);

//////////////////////////////////////////////////////////////////////////
// CustomMesh

RenderMesh_t* RenderMesh_new();
void RRenderMesh_free(lxRrendermesh ref);


#endif
