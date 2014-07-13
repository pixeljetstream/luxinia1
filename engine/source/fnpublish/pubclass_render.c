// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../resource/resmanager.h"
#include "../common/vid.h"
#include "../render/gl_render.h"
#include "../render/gl_print.h"
#include "../render/gl_list2d.h"
#include "../render/gl_list3d.h"
#include "../common/3dmath.h"
#include "../scene/vistest.h"
#include "../common/perfgraph.h"



// published here:
// LUXI_CLASS_RENDERDEBUG
// LUXI_CLASS_RENDERSURF
// LUXI_CLASS_RENDERFLAG
// LUXI_CLASS_BLENDMODE
// LUXI_CLASS_COMPAREMODE
// LUXI_CLASS_TECHNIQUE
// LUXI_CLASS_FONTSET
// LUXI_CLASS_RENDERSTENCIL
// LUXI_CLASS_OPERATIONMODE
// LUXI_CLASS_TEXCOMBINER
// LUXI_CLASS_TEXCOMBINER_FUNC
// LUXI_CLASS_TEXCOMBINER_SRC
// LUXI_CLASS_TEXCOMBINER_OP
// LUXI_CLASS_VERTEXTYPE
// LUXI_CLASS_TEXTYPE
// LUXI_CLASS_VISTEST
// LUXI_CLASS_BUFFERTYPE
// LUXI_CLASS_BUFFERHINT
// LUXI_CLASS_BUFFERMAPPING
// LUXI_CLASS_VIDBUFFER
// LUXI_CLASS_RENDERMESH

enum PubDrawCmd_e{
  PUBDRAW_FPS,
  PUBDRAW_STATS,
  PUBDRAW_WIRE,
  PUBDRAW_AXIS,
  PUBDRAW_BONES,
  PUBDRAW_BONESNAMES,
  PUBDRAW_BONESLIMITS,
  PUBDRAW_NORMALS,
  PUBDRAW_LIGHTS,
  PUBDRAW_PROJ,
  PUBDRAW_NOTEX,
  PUBDRAW_NOPRTS,
  PUBDRAW_DRAWALL,
  PUBDRAW_VISSPACE,
  PUBDRAW_STATSCOLOR,
  PUBDRAW_VISFROM,
  PUBDRAW_VISTO,
  PUBDRAW_PAUSEPARTICLES,
  PUBDRAW_NORMALLEN,
  PUBDRAW_DRAWSPECIAL,
  PUBDRAW_SHADOWVOL,
  PUBDRAW_STENCILBUFFER,
  PUBDRAW_DRAWSPECIALSIZE,
  PUBDRAW_NOGUI,
  PUBDRAW_CAMAXIS,
};

static int PubRender_drawhelpers(PState pstate,PubFunction_t *fn, int n)
{
  byte state;

  switch((int)fn->upvalue) {
  case PUBDRAW_CAMAXIS:
    if (n==0) return FunctionPublish_returnBool(pstate,g_Draws.drawCamAxis);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawCamAxis);
    break;
  case PUBDRAW_NOGUI:
    if (n==0) return FunctionPublish_returnBool(pstate,g_Draws.drawNoGUI);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawNoGUI);
    break;
  case PUBDRAW_SHADOWVOL:
    if (n==0) return FunctionPublish_returnBool(pstate,g_Draws.drawShadowVolumes);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawShadowVolumes);
    break;
  case PUBDRAW_STENCILBUFFER:
    if (n==0) return FunctionPublish_returnBool(pstate,g_Draws.drawStencilBuffer);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawStencilBuffer);
    break;
  case PUBDRAW_DRAWSPECIAL:
    if (n==0) g_Draws.drawSpecial = -1;
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_TEXTURE,(void*)&g_Draws.drawSpecial)){
      return FunctionPublish_returnError(pstate,"1 texture required");
    }
    break;
  case PUBDRAW_DRAWSPECIALSIZE:
    if (n == 0) return FunctionPublish_returnFloat(pstate,g_Draws.texsize);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_FLOAT,(void*)&g_Draws.texsize);
    break;
  case PUBDRAW_FPS:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawFPS);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawFPS);
    break;
  case PUBDRAW_STATS:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawStats);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawStats);
    break;
  case PUBDRAW_WIRE:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawWire);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawWire);
    break;
  case PUBDRAW_AXIS:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawAxis);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawAxis);
    break;
  case PUBDRAW_BONES:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawBones);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawBones);
    break;
  case PUBDRAW_BONESNAMES:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawBonesNames);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawBonesNames);
    break;
  case PUBDRAW_BONESLIMITS:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawBonesLimits);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawBonesLimits);
    break;
  case PUBDRAW_NORMALS:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawNormals);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawNormals);
    break;
  case PUBDRAW_LIGHTS:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawLights);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawLights);
    break;
  case PUBDRAW_PROJ:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawProj);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawProj);
    break;
  case PUBDRAW_NOPRTS:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawNoPrts);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawNoPrts);
    break;
  case PUBDRAW_DRAWALL:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.drawAll);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.drawAll);
    break;
  case PUBDRAW_VISSPACE:
    if (n == 0) return FunctionPublish_returnInt(pstate,g_Draws.drawVisSpace);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_Draws.drawVisSpace);
    break;
  case PUBDRAW_STATSCOLOR:
    if (n == 0) return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(g_Draws.statsColor));
    else FunctionPublish_getArg(pstate,4,FNPUB_TOVECTOR4(g_Draws.statsColor));
    break;
  case PUBDRAW_VISFROM:
    if (n == 0) return FunctionPublish_returnInt(pstate,g_Draws.drawVisFrom);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_Draws.drawVisFrom);
    break;
  case PUBDRAW_VISTO:
    if (n == 0) return FunctionPublish_returnInt(pstate,g_Draws.drawVisTo);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_Draws.drawVisTo);
    break;
  case PUBDRAW_PAUSEPARTICLES:
    if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    else{
      int i;
      ParticleSys_t *psys;
      for (i = 0; i < RES_MAX_PARTICLESYS; i++){
        if (psys = g_ResData.resarray[RESOURCE_PARTICLESYS].ptrs[i].psys){
          if (state){
            psys->psysflag |= PARTICLE_PAUSE;
          }
          else
            psys->psysflag &= ~PARTICLE_PAUSE;
        }

      }
    }
    break;
  case PUBDRAW_NORMALLEN:
    if (n==0) return FunctionPublish_returnFloat(pstate,g_Draws.normalLen);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_FLOAT,(void*)&g_Draws.normalLen);
    break;
  default:
    break;

  }

  return 0;
}


//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_RENDERSURFCE

enum SurfValue_e{
  SURF_LINEWIDTH,
  SURF_LINESTIPPLE,
  SURF_LINEFACTOR,
  SURF_ALPHAMODE,
  SURF_ALPHAVALUE,
  SURF_BLENDMODE,
  SURF_BLENDINVERT,

};

static int PubRenderSurf_set(PState pstate,PubFunction_t *fn, int n)
{
  static char   patternout[16];

  int applytoall;
  int upvalue;

  void      *data,*d2;
  VIDBlend_t  *blend;
  VIDAlpha_t  *alpha;
  VIDLine_t *line;
  char* pattern;


  int meshid;
  int offset;

  if (n==0)
    return FunctionPublish_returnError(pstate,"1 l2dimage/l2dtext/l3dmodel,[meshid]/l3dtrail/l3dprimitive/particlecloud/rcmdmesh required");

  FunctionPublish_getNArg(pstate,0,LUXI_CLASS_ANY,(void*)&data);

  offset = 1;
  meshid = 0;
  applytoall = LUX_FALSE;
  upvalue = (int)fn->upvalue;
  switch(FunctionPublish_getNArgType(pstate,0)) {
  case LUXI_CLASS_L3D_MODEL:
    if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MESHID,(void*)&meshid))
      offset++;
    else
      applytoall = LUX_TRUE;
  case LUXI_CLASS_L3D_LEVELMODEL:
    if (offset < 2)
      applytoall = LUX_TRUE;
  case LUXI_CLASS_L3D_TEXT:
  case LUXI_CLASS_L3D_TRAIL:
  case LUXI_CLASS_L3D_PRIMITIVE:
    if (!Reference_get(*(Reference*)&data,d2))
      return FunctionPublish_returnError(pstate,"reference node not valid");
    data = d2;
    if (offset > 1){
      if (!SUBRESID_CHECK(meshid,((List3DNode_t*)data)->minst->modelRID))
        return FunctionPublish_returnError(pstate,"illegal meshid");
      SUBRESID_MAKEPURE(meshid);
    }

    if (upvalue < SURF_ALPHAMODE)
      line = &((List3DNode_t*)data)->drawNodes[meshid].draw.state.line;
    else if (upvalue > SURF_ALPHAVALUE)
      blend = &((List3DNode_t*)data)->drawNodes[meshid].draw.state.blend;
    else
      alpha = &((List3DNode_t*)data)->drawNodes[meshid].draw.state.alpha;
    break;
  case LUXI_CLASS_RCMD_DRAWMESH:
    if (!Reference_get(*(Reference*)&data,d2))
      return FunctionPublish_returnError(pstate,"reference node not valid or not rcmdmesh");
    data = d2;
    if (upvalue < SURF_ALPHAMODE)
      line = &((RenderCmdMesh_t*)data)->dnode.state.line;
    else if (upvalue > SURF_ALPHAVALUE)
      blend = &((RenderCmdMesh_t*)data)->dnode.state.blend;
    else
      alpha = &((RenderCmdMesh_t*)data)->dnode.state.alpha;
    break;
  case LUXI_CLASS_L2D_TEXT:
  case LUXI_CLASS_L2D_IMAGE:
    if (!Reference_get(*(Reference*)&data,d2))
      return FunctionPublish_returnError(pstate,"reference node not valid");
    data = d2;
    if (upvalue < SURF_ALPHAMODE)
      line = &((List2DNode_t*)data)->line;
    else if (upvalue > SURF_ALPHAVALUE)
      blend = &((List2DNode_t*)data)->blend;
    else
      alpha = &((List2DNode_t*)data)->alpha;
    break;
  case LUXI_CLASS_PARTICLECLOUD:
    data = ResData_getParticleCloud((int)data);
    if (upvalue < SURF_ALPHAMODE)
      line = &((ParticleCloud_t*)data)->container.line;
    else if (upvalue > SURF_ALPHAVALUE)
      blend = &((ParticleCloud_t*)data)->container.blend;
    else
      alpha = &((ParticleCloud_t*)data)->container.alpha;
    break;
  default:
    return FunctionPublish_returnError(pstate,"1 l2dimage/l2dtext/l3dmodel,[meshid]/l3dtrail/l3dprimitive/particlecloud/rcmdmesh required");
  }

  switch(upvalue) {
  case SURF_ALPHAMODE:
    if (n == offset)  return FunctionPublish_returnType(pstate,LUXI_CLASS_COMPAREMODE,(void*)alpha->alphafunc);
    if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_COMPAREMODE,(void*)&alpha->alphafunc))
      return FunctionPublish_returnError(pstate,"1 comparemode required");
    if (applytoall){
      for (offset = 0; offset < ((List3DNode_t*)data)->numDrawNodes; offset++){
        ((List3DNode_t*)data)->drawNodes[offset].draw.state.alpha.alphafunc = alpha->alphafunc;
      }
    }
    break;
  case SURF_ALPHAVALUE:
    if (n == offset)  return FunctionPublish_returnFloat(pstate,alpha->alphaval);
    if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_FLOAT,(void*)&alpha->alphaval))
      return FunctionPublish_returnError(pstate,"1 float required");
    if (applytoall){
      for (offset = 0; offset < ((List3DNode_t*)data)->numDrawNodes; offset++){
        ((List3DNode_t*)data)->drawNodes[offset].draw.state.alpha.alphaval = alpha->alphaval;
      }
    }
    break;
  case SURF_BLENDMODE:
    if (n == offset)  return FunctionPublish_returnType(pstate,LUXI_CLASS_BLENDMODE,(void*)blend->blendmode);
    if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_BLENDMODE,(void*)&blend->blendmode))
      return FunctionPublish_returnError(pstate,"1 blendmode required");

    if (applytoall){
      for (offset = 0; offset < ((List3DNode_t*)data)->numDrawNodes; offset++){
        ((List3DNode_t*)data)->drawNodes[offset].draw.state.blend.blendmode = blend->blendmode;
      }
    }
    break;
  case SURF_BLENDINVERT:
    if (n == offset)  return FunctionPublish_returnBool(pstate,blend->blendinvert);
    if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_BOOLEAN,(void*)&blend->blendinvert))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    if (applytoall){
      for (offset = 0; offset < ((List3DNode_t*)data)->numDrawNodes; offset++){
        ((List3DNode_t*)data)->drawNodes[offset].draw.state.blend.blendinvert = blend->blendinvert;
      }
    }
    break;
  case SURF_LINEWIDTH:
    if (n == offset)  return FunctionPublish_returnFloat(pstate,line->linewidth);
    if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_FLOAT,(void*)&line->linewidth))
      return FunctionPublish_returnError(pstate,"1 float required");
    if (applytoall){
      for (offset = 0; offset < ((List3DNode_t*)data)->numDrawNodes; offset++){
        ((List3DNode_t*)data)->drawNodes[offset].draw.state.line.linewidth = line->linewidth;
      }
    }
    break;
  case SURF_LINEFACTOR:
    if (n == offset)  return FunctionPublish_returnInt(pstate,line->linefactor);
    if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_INT,(void*)&upvalue))
      return FunctionPublish_returnError(pstate,"1 int required");
    line->linefactor = upvalue;
    if (applytoall){
      for (offset = 0; offset < ((List3DNode_t*)data)->numDrawNodes; offset++){
        ((List3DNode_t*)data)->drawNodes[offset].draw.state.line.linefactor = line->linefactor;
      }
    }
    break;
  case SURF_LINESTIPPLE:
    if (n == offset){
      pattern = patternout;
      for (offset = 0; offset < 16; offset++,pattern++){
        if (line->linestipple & 1<<offset)
          *pattern = '1';
        else
          *pattern = '0';
      }
      return FunctionPublish_returnString(pstate,patternout);
    }
    if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_STRING,(void*)&pattern))
      return FunctionPublish_returnError(pstate,"1 string required");
    meshid = strlen(pattern);
    for (offset = 0; offset < meshid; offset++,pattern++){
      if (*pattern == '1')
        line->linestipple |= 1<<offset;
      else
        line->linestipple &= ~(1<<offset);
    }
    if (applytoall){
      for (offset = 0; offset < ((List3DNode_t*)data)->numDrawNodes; offset++){
        ((List3DNode_t*)data)->drawNodes[offset].draw.state.line.linestipple = line->linestipple;
      }
    }
    break;
  default:
    break;
  }



  return 0;

}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_TECHNIQUE

static int PubTechnique_supported(PState pstate,PubFunction_t *fn, int n)
{
  int tech;
  if (n!=1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_TECHNIQUE,(void*)&tech))
    return FunctionPublish_returnError(pstate,"1 technique required");

  if (fn->upvalue)
    return FunctionPublish_returnBool(pstate,tech <= g_VID.capTech && g_VID.capTexUnits >= 4);
  else
    return FunctionPublish_returnBool(pstate,tech <= g_VID.capTech);
}

static int PubTechnique_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_TECHNIQUE,fn->upvalue);
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_BLENDMODE

static int PubBlendMode_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_BLENDMODE,fn->upvalue);
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_COMPAREMODE

static int PubCompareMode_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_COMPAREMODE,fn->upvalue);
}

static int PubCompareMode_run(PState pstate,PubFunction_t *fn, int n)
{
  int mode;
  float ref;
  float val;
  if (n < 3 || 3!= FunctionPublish_getArg(pstate,3,LUXI_CLASS_COMPAREMODE,(void*)&mode,LUXI_CLASS_FLOAT,(void*)&val,LUXI_CLASS_FLOAT,(void*)&ref))
    return FunctionPublish_returnError (pstate,"1 comparemode 2 float required");

  return FunctionPublish_returnBool(pstate,VIDCompareMode_run(mode,val,ref));
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_OPERATIONMODE

static int PubOpMode_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_OPERATIONMODE,fn->upvalue);
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_LOGICMODE
static int PubLogicMode_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_LOGICMODE,fn->upvalue);
}


//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_VERTEXTYPE

static int PubVertexType_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_VERTEXTYPE,fn->upvalue);
}

static int PubVertexType_getScalarType(PState pstate,PubFunction_t *fn, int n)
{
  lxVertexType_t vtype;
  lxScalarType_t stype;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_VERTEXTYPE,(void*)&vtype))
    return FunctionPublish_returnError(pstate,"1 vertextype required");

  stype = g_VertexSizes[vtype][(int)(fn->upvalue)];
  if (stype == LUX_SCALAR_ILLEGAL)
    return 0;

  return FunctionPublish_returnType(pstate,LUXI_CLASS_SCALARTYPE,(void*)stype);
}

static int PubVertexType_getOffset(PState pstate,PubFunction_t *fn, int n)
{
  lxVertexType_t vtype;
  int val;
  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_VERTEXTYPE,(void*)&vtype))
    return FunctionPublish_returnError(pstate,"1 vertextype required");

  val = g_VertexSizes[vtype][(int)(fn->upvalue)];
  if (val < 0)
    return 0;

  return FunctionPublish_returnInt(pstate,val);
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_MESHTYPE

static int PubMeshType_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_MESHTYPE,fn->upvalue);
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_TEXTYPE

static int PubTexType_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTYPE,fn->upvalue);
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_TEXDATATYPE

static int PubTexDataType_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXDATATYPE,fn->upvalue);
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_PRIMITIVETYPE

static int PubRendermode_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_PRIMITIVETYPE,fn->upvalue);
}



//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_RENDERFLAG


static int PubRenderflag_set(PState pstate,PubFunction_t *fn, int n)
{
  void *data,*d2;
  int meshid;
  int offset;
  enum32 rfcopy;
  enum32 *rf;

  byte state;

  if (n == 0 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_ANY,&data))
    return FunctionPublish_returnError(pstate,"1 renderflag/l2dnode/l3dmodel,[meshid]/l3dtrail/l3dprimitive/particlecloud/rcmdmesh required");

  offset = 0;

  switch(FunctionPublish_getNArgType(pstate,0)) {
  case LUXI_CLASS_RCMD_DRAWMESH:
    if (!Reference_get(*(Reference*)&data,d2))
      return FunctionPublish_returnError(pstate,"1 rcmdmesh  required");
    rf = &((RenderCmdMesh_t*)d2)->dnode.state.renderflag;
    data =NULL;
    break;
  case LUXI_CLASS_PARTICLECLOUD:
    rf = &ResData_getParticleCloud((int)data)->container.renderflag;
    data = NULL;
    break;
  case LUXI_CLASS_RCMD_IGNOREFLAG:
  case LUXI_CLASS_RCMD_FORCEFLAG:
    if (!Reference_get(*(Reference*)&data,d2))
      return FunctionPublish_returnError(pstate,"1 rcmd required");
    rf = &((RenderCmdFlag_t*)d2)->renderflag;
    data =NULL;
    break;
  case LUXI_CLASS_L2D_NODE:
  case LUXI_CLASS_L2D_FLAG:
  case LUXI_CLASS_L2D_IMAGE:
  case LUXI_CLASS_L2D_TEXT:
  case LUXI_CLASS_L2D_L3DLINK:
    if (!Reference_get(*(Reference*)&data,d2))
      return FunctionPublish_returnError(pstate,"1 l2dnode required");
    data = d2;
    rf = &((List2DNode_t*)data)->renderflag;
    data = NULL;

    break;
  case LUXI_CLASS_L3D_MODEL:
    if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MESHID,(void*)&meshid))
      offset++;
  case LUXI_CLASS_L3D_LEVELMODEL:
  case LUXI_CLASS_L3D_TEXT:
  case LUXI_CLASS_L3D_TRAIL:
  case LUXI_CLASS_L3D_PRIMITIVE:
    if (!Reference_get(*(Reference*)&data,d2))
      return FunctionPublish_returnError(pstate,"1 l3dnode required");
    data = d2;

    if (offset){
      if (!SUBRESID_CHECK(meshid,((List3DNode_t*)data)->minst->modelRID))
        return FunctionPublish_returnError(pstate,"illegal meshid");
      SUBRESID_MAKEPURE(meshid);
      rf = &((List3DNode_t*)data)->drawNodes[meshid].draw.state.renderflag;
    }
    else{
      rf = &((List3DNode_t*)data)->drawNodes[0].draw.state.renderflag;
      rfcopy = *rf;
      rf = &rfcopy;
    }

    break;
  default:
    return FunctionPublish_returnError(pstate,"1 renderflag/l2dnode/l3dmodel,[meshid]/l3dtrail/l3dprimitive/particlecloud/rcmdmesh required");
  }

  offset++;

  if (n < offset+1) return FunctionPublish_returnBool(pstate,isFlagSet(*rf,(enum32)fn->upvalue));


  if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_BOOLEAN,(void*)&state))
    return FunctionPublish_returnError(pstate,"1 renderflag 1 boolean required");



  if (state)
    *rf |= (enum32)fn->upvalue;
  else
    *rf &= ~(enum32)fn->upvalue;

  if (data){
    // l3dnodes need special treatment
    List3DNode_setRF(data,(enum32)fn->upvalue,state,(offset < 2) ? -1 : meshid);
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_FONTSET

static int PubFontSet_new(PState pstate,PubFunction_t *f,int n)
{
  lxRfontset ref;
  if (f->upvalue == (void*)-1) {
    int size;
    if (n==0) {
      ref = FontSet_new(FONTSET_SIZE_16X16)->reference;
      Reference_makeVolatile(ref);
      return FunctionPublish_returnType(pstate,LUXI_CLASS_FONTSET,REF2VOID(ref));
    }
    FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_INT,size);
    if (size<2 || size>16) FunctionPublish_returnError(pstate,"Passed size must be >1 and <17");

    ref = FontSet_new(size)->reference;
    Reference_makeVolatile(ref);
    return FunctionPublish_returnType(pstate,LUXI_CLASS_FONTSET,REF2VOID(ref));
  }
  ref = FontSet_new((FontSetSize_t)f->upvalue)->reference;
  Reference_makeVolatile(ref);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_FONTSET,REF2VOID(ref));
}



enum PubFontSetCmds_e
{
  PFS_DEFAULT,
  PFS_DELETEALL,

  PFS_FS_FUNCS,
  PFS_LINE,
  PFS_MAX,
  PFS_DELETE,
  PFS_IGSPECIALS,
  PFS_OFFSETX,
  PFS_OFFSETY,

  PFS_FS_1ARG_FUNCS,
  PFS_WIDTH,
  PFS_LOOKUP,
};

static int PubFontSet_attr(PState pstate,PubFunction_t *f,int n)
{
  Reference ref;
  FontSet_t *fs;
  int index;
  int chr = 0;

  if (((int)f->upvalue > PFS_FS_FUNCS) &&
      (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_FONTSET,(void*)&ref) ||
      !Reference_get(ref,fs)) )
    return FunctionPublish_returnError(pstate,"1 fontset required");

  if ((int)f->upvalue > PFS_FS_1ARG_FUNCS && (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&chr)))
    return FunctionPublish_returnError(pstate,"1 fontset 1 int required");

  chr %= 256;

  switch((int)f->upvalue)
  {
  case PFS_DEFAULT:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_FONTSET,REF2VOID(PText_getDefaultFontSet()->reference));
  case PFS_DELETE:
    Reference_free(ref);//RFontSet_free(ref);
    return 0;
  case PFS_IGSPECIALS:
    if (n==1)
      return FunctionPublish_returnBool(pstate,fs->ignorespecials);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&fs->ignorespecials))
      return FunctionPublish_returnError(pstate,"1 fontset 1 boolean required");
    break;
  case PFS_LINE:
    if (n==1)
      return FunctionPublish_returnFloat(pstate,fs->linespacing);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&fs->linespacing))
      return FunctionPublish_returnError(pstate,"1 fontset 1 float required");
    break;
    case PFS_OFFSETX:
    if (n==1)
      return FunctionPublish_returnFloat(pstate,fs->offsetX);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&fs->offsetX))
      return FunctionPublish_returnError(pstate,"1 fontset 1 float required");
    break;
    case PFS_OFFSETY:
    if (n==1)
      return FunctionPublish_returnFloat(pstate,fs->offsetY);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&fs->offsetY))
      return FunctionPublish_returnError(pstate,"1 fontset 1 float required");
    break;
  case PFS_MAX:
    return FunctionPublish_returnInt(pstate,fs->numchars);
  case PFS_WIDTH:
    if (n==2)
      return FunctionPublish_returnFloat(pstate,fs->charwidth[chr]);
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_FLOAT,(void*)&fs->charwidth[chr]))
      return FunctionPublish_returnError(pstate,"1 fontset 1 int 1 float required");
    break;
  case PFS_LOOKUP:
    if (n==2)
      return FunctionPublish_returnInt(pstate,fs->charlookup[chr]);
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&index) || index < 0 || index >= fs->numchars)
      return FunctionPublish_returnError(pstate,"1 fontset 1 int 1 int (0- maxcharacters-1) required");
    fs->charlookup[chr] = index;
    break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_RENDERSTENCIL

enum PubStencilCmd_e{
  STENCIL_SET,
  STENCIL_FUNC,
  STENCIL_OP,
};

static int PubRenderStencil_cmd(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List2DNode_t *l2d;
  RenderCmdFlag_t *ffx;
  int id;
  void *data;
  VIDStencil_t *vstencil;

  int state;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RENDERSTENCIL,(void*)&data))
    return FunctionPublish_returnError(pstate,"1 stencilcommand required");

  switch(FunctionPublish_getNArgType(pstate,0))
  {
  case LUXI_CLASS_RCMD_STENCIL:
    ref = *(Reference*)&data;
    if (!Reference_get(ref,ffx))
      return FunctionPublish_returnError(pstate,"1 rcmdstencil required ");
    vstencil = &ffx->stencil;
    break;
  case LUXI_CLASS_L2D_FLAG:
    ref = *(Reference*)&data;
    if (!Reference_get(ref,l2d))
      return FunctionPublish_returnError(pstate,"1 l2dflag required ");
    vstencil = &l2d->l2dflag->stencil;
  default:
    return FunctionPublish_returnError(pstate,"no interface found for class");
  }
  switch((int)f->upvalue)
  {
  case STENCIL_SET:
    state = LUX_FALSE;
    if (n==1) return FunctionPublish_setRet(pstate,2,LUXI_CLASS_BOOLEAN,(void*)vstencil->enabled,LUXI_CLASS_INT,(void*)((int)vstencil->twosided));
    else if (FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_BOOLEAN,(void*)&vstencil->enabled,LUXI_CLASS_INT,(void*)&state)<1)
      return FunctionPublish_returnError(pstate,"1 stencilcommand 1 boolean [1 int] required");
    vstencil->twosided = state;
    break;
  case STENCIL_FUNC:
    if (n==1) return FunctionPublish_setRet(pstate,4,LUXI_CLASS_COMPAREMODE,(void*)vstencil->funcF,
                          LUXI_CLASS_COMPAREMODE,(void*)vstencil->funcB,
                          LUXI_CLASS_INT,(void*)vstencil->refvalue,
                          LUXI_CLASS_INT,(void*)vstencil->mask);
    else if (n!=5 || 4!=FunctionPublish_getArgOffset(pstate,1,4,LUXI_CLASS_COMPAREMODE,(void*)&vstencil->funcF,
                        LUXI_CLASS_COMPAREMODE,(void*)&vstencil->funcB,
                        LUXI_CLASS_INT,(void*)&vstencil->refvalue,
                        LUXI_CLASS_INT,(void*)&vstencil->mask))
      return FunctionPublish_returnError(pstate,"1 stencilcommand 2 comparemodes 2 int required");
    break;
  case STENCIL_OP:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&id) || id < 0 || id > 1)
      return FunctionPublish_returnError(pstate,"1 stencilcommand 1 int (0/1) required");
    if (n==2) return FunctionPublish_setRet(pstate,3,LUXI_CLASS_OPERATIONMODE,(void*)vstencil->ops[id].fail,
                          LUXI_CLASS_OPERATIONMODE,(void*)vstencil->ops[id].zfail,
                          LUXI_CLASS_OPERATIONMODE,(void*)vstencil->ops[id].zpass);
    else if (n!=5 || 3!=FunctionPublish_getArgOffset(pstate,2,3,  LUXI_CLASS_OPERATIONMODE,(void*)&vstencil->ops[id].fail,
                          LUXI_CLASS_OPERATIONMODE,(void*)&vstencil->ops[id].zfail,
                          LUXI_CLASS_OPERATIONMODE,(void*)&vstencil->ops[id].zpass))
      return FunctionPublish_returnError(pstate,"1 stencilcommand 3 operationmodes required");
    break;
  default:
    break;
  }

  return 0;
}
//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_TEXCOMBINER

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_TEXCOMBINER_CFUNC

static int PubTexCombinerCFunc_new(PState pstate,PubFunction_t *fn, int n)
{
  char *name;
  VIDTexCombiner_t *comb;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name))
    return FunctionPublish_returnError(pstate,"1 string required");

  comb = VIDTexCombiner_color(name);
  if (!comb)
    return FunctionPublish_returnError(pstate,"must not overwrite default combiners");

  VIDTexCombiner_prepCmd(comb,(enum32)fn->upvalue,LUX_FALSE);

  return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXCOMBINER_CFUNC,(void*)comb);
}

static int PubTexCombinerAFunc_new(PState pstate,PubFunction_t *fn, int n)
{
  char *name;
  VIDTexCombiner_t *comb;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name))
    return FunctionPublish_returnError(pstate,"1 string required");

  comb = VIDTexCombiner_alpha(name);
  if (!comb)
    return FunctionPublish_returnError(pstate,"must not overwrite default combiners");

  VIDTexCombiner_prepCmd(comb,(enum32)fn->upvalue,LUX_TRUE);

  return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXCOMBINER_AFUNC,(void*)comb);
}

enum PubTexCombCmds_e
{
  TEXC_SETARG,
  TEXC_TEST,
};

static int PubTexCombinerFunc_attr(PState pstate,PubFunction_t *fn, int n)
{
  VIDTexCombiner_t *comb;
  static char errorbuffer[2048];
  int cfunc = 0;
  int id;
  enum32 op,src;

  if (n < 1 || (!(cfunc=FunctionPublish_getNArg(pstate,0,LUXI_CLASS_TEXCOMBINER_CFUNC,(void*)&comb)) &&
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_TEXCOMBINER_AFUNC,(void*)&comb)))
    return FunctionPublish_returnError(pstate,"1 texcombcolor/texcombalpha required");

  switch((int)fn->upvalue)
  {
  case TEXC_SETARG:
    if (n < 4 ||  3>FunctionPublish_getArgOffset(pstate,1,3,LUXI_CLASS_INT,(void*)&id,LUXI_CLASS_TEXCOMBINER_SRC,(void*)&src,LUXI_CLASS_TEXCOMBINER_OP,(void*)&op))
      return FunctionPublish_returnError(pstate,"1 texcombcolor/texcombalpha 1 texcombsrc 1 texcombop required");
    // modulate add is different for combine 4
    // ati = 0 * 2 + 1
    // nvcombine4 = 0 * 1 + 2 * 3
    if (comb->combine4==1 && id)
      id = id%2 + 1;

    comb->srcs[id] = src;
    comb->ops[id] = op;
    return 0;
  case TEXC_TEST:
    if (!cfunc){
      VIDTexBlendHash hash =
        VIDTexBlendHash_get(VID_MODULATE,VID_ALPHA_USER+(comb-g_VID.gensetup.tcalpha),LUX_FALSE,0,0);
      vidTexBlendSet(hash,VID_MODULATE,VID_ALPHA_USER+(comb-g_VID.gensetup.tcalpha),LUX_FALSE,0,0);
    }
    else{
      VIDTexBlendHash hash =
        VIDTexBlendHash_get(VID_COLOR_USER+(comb-g_VID.gensetup.tccolor),VID_A_UNSET,LUX_FALSE,0,0);
      vidTexBlendSet(hash,VID_COLOR_USER+(comb-g_VID.gensetup.tccolor),VID_A_UNSET,LUX_FALSE,0,0);
    }
    {
      GLenum error = 0;
      errorbuffer[0] = 0;
      while ( (error = glGetError()) != GL_NO_ERROR) {
        strcat(errorbuffer,gluErrorString(error));
      }

      if (errorbuffer[0])
        return FunctionPublish_returnString(pstate,errorbuffer);
    }
    break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_TEXCOMBINER_SRC

static int PubTexCombinerSrc_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXCOMBINER_SRC,fn->upvalue);
}

// LUXI_CLASS_TEXCOMBINER_OP

static int PubTexCombinerOp_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXCOMBINER_OP,fn->upvalue);
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_VISTEST


static int PubVisTest_tweak(PState pstate,PubFunction_t *fn, int n)
{
  int val;
  if (n==0) return FunctionPublish_returnInt(pstate,VisTest_getTweak((VisTestTweakable_t)fn->upvalue));
  if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&val))
    return FunctionPublish_returnError(pstate,"1 boolean required");
  VisTest_setTweak((VisTestTweakable_t)fn->upvalue,val);

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_BUFFERTYPE,LUXI_CLASS_BUFFERHINT,LUXI_CLASS_BUFFERMAPPING

static int PubBufferType_new(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_BUFFERTYPE,fn->upvalue);
}
static int PubBufferHint_new(PState pstate,PubFunction_t *fn, int n)
{
  uint type = (uint)fn->upvalue;
  int freq = 0;
  if(!FunctionPublish_getNArg(pstate,0,FNPUB_TINT(freq)) || freq < 0 || freq > 2)
    return FunctionPublish_returnError(pstate,"1 int required.");

  type += freq*3;
  return FunctionPublish_returnType(pstate,LUXI_CLASS_BUFFERHINT,(void*)type);
}
static int PubBufferMapping_new(PState pstate,PubFunction_t *fn, int n)
{

  return FunctionPublish_returnType(pstate,LUXI_CLASS_BUFFERMAPPING,fn->upvalue);
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_VIDBUFFER

static int PubVidBuffer_new(PState pstate,PubFunction_t *fn, int n)
{
  HWBufferObject_t* vo;
  VIDBufferType_t type;
  VIDBufferHint_t hint;
  int size;
  void* data = NULL;

  if (n < 3 || 3>FunctionPublish_getArg(pstate,4,LUXI_CLASS_BUFFERTYPE,(void*)&type,
    LUXI_CLASS_BUFFERHINT,(void*)&hint,LUXI_CLASS_INT,(void*)&size,
    LUXI_CLASS_POINTER,(void*)&data)){
      return FunctionPublish_returnError(pstate,"1 vidtype 1 vidhint 1 int [1 pointer] required.");
  }

  vo = HWBufferObject_new(type,hint,size,data);
  Reference_makeVolatile(vo->reference);

  return FunctionPublish_returnType(pstate,LUXI_CLASS_VIDBUFFER,REF2VOID(vo->reference));
}

enum PubVBufCmd_e{
  PVBUF_MAPGL,
  PVBUF_MAPRANGEGL,
  PVBUF_UNMAPGL,
  PVBUF_MAPPED,
  PVBUF_ALLOC,
  PVBUF_SUBMITGL,
  PVBUF_RETRIEVEGL,
  PVBUF_RELEASEGL,
  PVBUF_FLUSHGL,
  PVBUF_CPYGL,
  PVBUF_KEEP,
  PVBUF_GLID,
};

static int PubVidBuffer_attr(PState pstate, PubFunction_t *fn, int n)
{
  lxRvidbuffer    ref;
  HWBufferObject_t* vo;
  VIDBuffer_t*    vbuf;

  if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_VIDBUFFER,(void*)&ref) || !Reference_get(ref,vo))
    return FunctionPublish_returnError(pstate,"1 vidbuffer required");

  vbuf = &vo->buffer;

  switch((uint)fn->upvalue){
  case PVBUF_MAPGL:
    {
      VIDBufferMapType_t maptype;

      if(!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BUFFERMAPPING,(void*)&maptype)){
        return FunctionPublish_returnError(pstate,"1 vidmapping required");
      }

      VIDBuffer_mapGL(vbuf,maptype);
      return FunctionPublish_setRet(pstate,2,FNPUB_FPTR(vbuf->mapped),
        FNPUB_FPTR(vbuf->mappedend));
    }
    return 0;

  case PVBUF_MAPRANGEGL:
    {
      VIDBufferMapType_t maptype;
      booln manflush = LUX_FALSE;
      booln unsynch = LUX_FALSE;
      int from = -1;
      int cnt = -1;

      if(3>FunctionPublish_getArgOffset(pstate,1,5,LUXI_CLASS_BUFFERMAPPING,(void*)&maptype,FNPUB_TINT(from),FNPUB_TINT(cnt),FNPUB_TBOOL(manflush),FNPUB_TBOOL(unsynch))){
        return FunctionPublish_returnError(pstate,"1 vidmapping 2 int [1 boolean] [1 boolean] required");
      }

      if (!g_VID.capRangeBuffer && (manflush || unsynch)){
        return FunctionPublish_returnError(pstate,"hardware capability insufficient");
      }

      from = LUX_MIN(LUX_MAX(0,from),vbuf->size);
      cnt  = cnt < 0 ? vbuf->size : cnt;
      cnt  = LUX_MIN(vbuf->size-from,cnt);

      VIDBuffer_mapRangeGL(vbuf,maptype,from,cnt,manflush,unsynch);

      return FunctionPublish_setRet(pstate,2,FNPUB_FPTR(vbuf->mapped),
        FNPUB_FPTR(vbuf->mappedend));
    }
    return 0;
  case PVBUF_UNMAPGL:
    VIDBuffer_unmapGL(vbuf);
    return 0;
  case PVBUF_SUBMITGL:
  case PVBUF_RETRIEVEGL:
    {
      void* data = NULL;
      int from = -1;
      int cnt = -1;

      if(3>FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TINT(from),FNPUB_TINT(cnt),FNPUB_TPTR(data))){
        return FunctionPublish_returnError(pstate,"2 int 1 pointer required");
      }

      from = LUX_MIN(LUX_MAX(0,from),vbuf->size);
      cnt  = cnt < 0 ? vbuf->size : cnt;
      cnt  = LUX_MIN(vbuf->size-from,cnt);

      if (cnt){
        if ((uint)fn->upvalue == PVBUF_SUBMITGL){
          VIDBuffer_submitGL(vbuf,from,cnt,data);
        }
        else{
          VIDBuffer_retrieveGL(vbuf,from,cnt,data);
        }
      }
      return FunctionPublish_returnInt(pstate,cnt);
    }
    return 0;

  case PVBUF_RELEASEGL:
    VIDBuffer_clearGL(vbuf);
    return 0;
  case PVBUF_FLUSHGL:
    {
      int from = -1;
      int cnt = -1;

      if(2>FunctionPublish_getArgOffset(pstate,1,2,FNPUB_TINT(from),FNPUB_TINT(cnt))){
        return FunctionPublish_returnError(pstate,"2 int required");
      }

      from = LUX_MIN(LUX_MAX(0,from),vbuf->size);
      cnt  = cnt < 0 ? vbuf->size : cnt;
      cnt  = LUX_MIN(vbuf->size-from,cnt);

      VIDBuffer_flushRangeGL(vbuf,from,cnt);
    }

    return 0;
  case PVBUF_CPYGL:
    {
      int from = 0;
      int to = 0;
      int cnt = 0;

      if(4>FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TINT(to),LUXI_CLASS_VIDBUFFER,(void*)&ref,FNPUB_TINT(from),FNPUB_TINT(cnt)) || !Reference_get(ref,vo))
      {
        return FunctionPublish_returnError(pstate,"1 int 1 vidbuffer 2 int required");
      }

      VIDBuffer_copy(vbuf,to,&vo->buffer,from,cnt);
    }
    return 0;
  case PVBUF_GLID:
    {

      if (n==1){
        VIDBufferType_t type;
        if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BUFFERTYPE,(void*)&type)){
          VIDBuffer_bindGL(vbuf,type);
        }
        else{
          VIDBuffer_unbindGL(vbuf);
        }
      }
      return FunctionPublish_returnPointer(pstate,(void*)vbuf->glID);
    }

  case PVBUF_ALLOC:
    {
      uint size;
      uint padsize = 4;
      void *ptr = NULL;

      if (1>FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TINT(size),FNPUB_TINT(padsize),LUXI_CLASS_POINTER,(void*)&ptr)){
        return FunctionPublish_returnError(pstate,"1 int [1 int] [1 ptr] required.");
      }

      padsize = VIDBuffer_alloc(vbuf,size,padsize);
      if (padsize == (uint)-1)
        return 0;
      if (ptr){
        VIDBuffer_submitGL(vbuf,padsize,size,ptr);
      }
      return FunctionPublish_returnInt(pstate,padsize);
    }
    return 0;
  case PVBUF_MAPPED:
    if (vbuf->mapped){
      return FunctionPublish_setRet(pstate,2,FNPUB_FPTR(vbuf->mapped),
                          FNPUB_FPTR(vbuf->mappedend));
    }
    return 0;
  case PVBUF_KEEP:
    if (n==1)
      return FunctionPublish_returnBool(pstate,vo->keeponloss);
    else if (!FunctionPublish_getNArg(pstate,1,FNPUB_TBOOL(vo->keeponloss))){
      return FunctionPublish_returnError(pstate,"1 boolean required.");
    }
    return 0;
  default:
    return 0;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// RenderMesh

int PubUserMesh(PState pstate, int from, Mesh_t **outmesh, lxRrendermesh *outuser)
{
  HWBufferObject_t* vbo = NULL;
  HWBufferObject_t* ibo = NULL;
  RenderMesh_t* umesh;
  lxRvidbuffer  iboref = NULL;
  lxRvidbuffer  vboref = NULL;
  int vbooffset = -1;
  int ibooffset = -1;
  lxVertexType_t  vtype;
  int verts;
  int indices;
  Mesh_t* newmesh;

  if (3>FunctionPublish_getArgOffset(pstate,from,7,LUXI_CLASS_VERTEXTYPE,(void*)&vtype,
    FNPUB_TINT(verts),FNPUB_TINT(indices),LUXI_CLASS_VIDBUFFER,(void*)&vboref,FNPUB_TINT(vbooffset),
    LUXI_CLASS_VIDBUFFER,(void*)&iboref,FNPUB_TINT(ibooffset)) ||
    (vboref && !Reference_get(vboref,vbo)) || (iboref && !Reference_get(iboref,ibo)))
    return FunctionPublish_returnError(pstate,"1 l3dprimitive 1 vertextype 2 int [1 vidbuffer] [1 int] [1 vidbuffer] [1 int] required");

  if (vbo){
    newmesh = Mesh_newVBO(NULL,verts,indices,vtype,&vbo->buffer,vbooffset,
      ibo ? &ibo->buffer : NULL,ibooffset);
  }
  else{
    newmesh = Mesh_new(NULL,verts,indices,vtype);
  }

  if (!newmesh){
    return FunctionPublish_returnError(pstate,"could not create new mesh");
  }

  if (*outuser){
    Reference_release(*outuser);
  }
  if (vboref){
    Reference_ref(vboref);
  }
  if (iboref){
    Reference_ref(iboref);
  }

  umesh = RenderMesh_new();
  umesh->freemesh = LUX_TRUE;
  umesh->mesh = newmesh;
  umesh->iboref = iboref;
  umesh->vboref = vboref;

  *outmesh = newmesh;
  *outuser = umesh->reference;


  return 0;
}

int PubRenderMesh(PState pstate, int from, struct Mesh_s **outmesh, lxRrendermesh *outuser)
{
  lxRrendermesh ref;
  RenderMesh_t *rmesh;

  if (FunctionPublish_getNArg(pstate,from,LUXI_CLASS_RENDERMESH,(void*)&ref) &&
    Reference_get(ref,rmesh))
  {
    Reference_releaseCheck(*outuser);
    Reference_ref(ref);
    *outuser = ref;
    *outmesh = rmesh->mesh;
  }
  else if ((ref = *outuser)){
    return FunctionPublish_returnType(pstate,LUXI_CLASS_RENDERMESH,REF2VOID(ref));
  }
  return 0;
}

static int PubRenderMesh_new(PState pstate, PubFunction_t *fn, int n)
{
  lxRrendermesh rmesh = NULL;
  Mesh_t*     mesh = NULL;

  int ret = PubUserMesh(pstate,0,&mesh,&rmesh);
  if (ret)  return ret;

  Reference_makeVolatile(rmesh);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_RENDERMESH,REF2VOID(rmesh));
}


//////////////////////////////////////////////////////////////////////////
// INIT ALL
void PubClass_Render_init()
{
    FunctionPublish_initClass(LUXI_CLASS_RENDERINTERFACE,"renderinterface",
    "Most renderable surfaces allow a detailed control of how they are rendered. Several interfaces exist for greater control. Also some other related classes are stored here.",NULL,LUX_FALSE);

  FunctionPublish_initClass(LUXI_CLASS_RENDERDEBUG,"render","Rendering statistics and other data. Mostly just debugging helpers.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_FPS,"drawfps",
    "([boolean]):([boolean]) - draws FPS on top right of the screen");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_STATS,"drawstats",
    "([boolean]):([boolean]) - draws various stats");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_WIRE,"drawwire",
    "([boolean]):([boolean]) - draws everthing in wiremesh");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_AXIS,"drawaxis",
    "([boolean]):([boolean]) - draws axis of objects");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_BONES,"drawbones",
    "([boolean]):([boolean]) - draws bones");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_BONESNAMES,"drawbonenames",
    "([boolean]):([boolean]) - draws bonenames");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_BONESLIMITS,"drawbonelimits",
    "([boolean]):([boolean]) - draws boneaxislimits");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_NORMALS,"drawnormals",
    "([boolean]):([boolean]) - draws red normals (blue tangents + green binormals) of the triangles");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_LIGHTS,"drawlights",
    "([boolean]):([boolean]) - draws lightsource origins");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_PROJ,"drawprojectors",
    "([boolean]):([boolean]) - draw projectors");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_SHADOWVOL,"drawshadowvolumes",
    "([boolean]):([boolean]) - draw drawshadowvolumes");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_STENCILBUFFER,"drawstencilbuffer",
    "([boolean]):([boolean]) - draws stencilbuffer");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_NOPRTS,"noparticles",
    "([boolean]):([boolean]) - deactivates particles");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_DRAWALL,"drawall",
    "([boolean]):([boolean]) - draws every node independent of vistest result");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_VISSPACE,"drawvisspace",
    "([int]):([int]) - draws the octree of the visspace, 0 disables, 1 dynamic, 2 is static, 3 camera, 4 projector, 5 light");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_STATSCOLOR,"statscolor",
    "(float r,g,b,a):(float r,g,b,a) - sets color of stats");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_VISFROM,"visfrom",
    "(int):(int) - min depth, if you draw a visspace, you can set the min (visfrom) and max (visto) depth of the octree that should be drawn");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_VISTO,"visto",
    "(int):(int) - max depth (-1 will use octree's maxdepth), if you draw a visspace, you can set the min (visfrom) and max (visto) depth of the octree that should be drawn");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_PAUSEPARTICLES,"pauseparticles",
    "([boolean]):([boolean]) - if true then no new spawns, and no age updates");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_NORMALLEN,"normallength",
    "([float]):([float]) - sets/gets length of drawn normals if normals are drawn.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_DRAWSPECIAL,"drawtexture",
    "():([texture]) - draws special texture test (useful for depth or cubemap texture), if no arg is passed then drawing is disabled.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_DRAWSPECIALSIZE,"drawtexturesize",
    "([float]):([float]) - sets or gets size of the special texture draw");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_NOGUI,"drawnogui",
    "([boolean]):([boolean]) - disables l2dnodes and other gui items");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERDEBUG,PubRender_drawhelpers,(void*)PUBDRAW_CAMAXIS,"drawcamaxis",
    "([boolean]):([boolean]) - draws axis orientation for cameras");

  FunctionPublish_initClass(LUXI_CLASS_RENDERSURF,"rendersurface","For most renderable nodes you can define how their surface are blend with the framebuffer, or how alpha-based rejection should be done. Enabling/disabling those effects is done with renderflags. l3dmodel is treated similar as in renderflag",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_RENDERSURF,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_PRIMITIVE,LUXI_CLASS_RENDERSURF);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_TRAIL,LUXI_CLASS_RENDERSURF);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_TEXT,LUXI_CLASS_RENDERSURF);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_MODEL,LUXI_CLASS_RENDERSURF);
  FunctionPublish_addInterface(LUXI_CLASS_L2D_IMAGE,LUXI_CLASS_RENDERSURF);
  FunctionPublish_addInterface(LUXI_CLASS_L2D_TEXT,LUXI_CLASS_RENDERSURF);
  FunctionPublish_addInterface(LUXI_CLASS_PARTICLECLOUD,LUXI_CLASS_RENDERSURF);
  FunctionPublish_addInterface(LUXI_CLASS_RCMD_DRAWMESH,LUXI_CLASS_RENDERSURF);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_LEVELMODEL,LUXI_CLASS_RENDERSURF);

  FunctionPublish_addFunction(LUXI_CLASS_RENDERSURF,PubRenderSurf_set,(void*)SURF_ALPHAMODE,"rsAlphacompare",
    "([comparemode]):(rendersurface,[comparemode]) - returns or sets alpha compare mode - only effective if rfAlphatest is set");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSURF,PubRenderSurf_set,(void*)SURF_BLENDMODE,"rsBlendmode",
    "([blendmode]):(rendersurface,[blendmode]) - only effective if rfBlend is set");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSURF,PubRenderSurf_set,(void*)SURF_ALPHAVALUE,"rsAlphavalue",
    "([float]):(rendersurface,[float]) - returns or sets alpha threshold");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSURF,PubRenderSurf_set,(void*)SURF_BLENDINVERT,"rsBlendinvert",
    "([boolean]):(rendersurface,[boolean]) - returns or sets blend invert of alpha-based blend types");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSURF,PubRenderSurf_set,(void*)SURF_LINEWIDTH,"rsLinewidth",
    "([float]):(rendersurface,[float]) - returns or sets linewidth for wireframes. When not set it will use whatever linewidth is currently active.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSURF,PubRenderSurf_set,(void*)SURF_LINESTIPPLE,"rsLinestipple",
    "([string]):(rendersurface,[string]) - returns or sets stipple pattern, is used if rfWireframe is on and rsLinefactor is greater 0. The string should contain 16 0s or 1s which define the linestyle.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSURF,PubRenderSurf_set,(void*)SURF_LINEFACTOR,"rsLinefactor",
    "([int]):(rendersurface,[int 0-255]) - returns or sets stipple factor, is used if rfWireframe and is value is greater 0. Linestyle is define in rsLinestipple.");

  FunctionPublish_initClass(LUXI_CLASS_RENDERFLAG,"renderflag",
    "For most renderable nodes you can define under what state conditions they are rendered. l3dmodels also allow renderflag to be meshid specific, if no meshid is given, we apply it to all.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RENDERFLAG,LUXI_CLASS_RENDERINTERFACE);

  FunctionPublish_addInterface(LUXI_CLASS_L3D_PRIMITIVE,LUXI_CLASS_RENDERFLAG);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_TRAIL,LUXI_CLASS_RENDERFLAG);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_TEXT,LUXI_CLASS_RENDERFLAG);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_MODEL,LUXI_CLASS_RENDERFLAG);
  FunctionPublish_addInterface(LUXI_CLASS_L2D_NODE,LUXI_CLASS_RENDERFLAG);
  FunctionPublish_addInterface(LUXI_CLASS_PARTICLECLOUD,LUXI_CLASS_RENDERFLAG);
  FunctionPublish_addInterface(LUXI_CLASS_RCMD_DRAWMESH,LUXI_CLASS_RENDERFLAG);
  FunctionPublish_addInterface(LUXI_CLASS_RCMD_FORCEFLAG,LUXI_CLASS_RENDERFLAG);
  FunctionPublish_addInterface(LUXI_CLASS_RCMD_IGNOREFLAG,LUXI_CLASS_RENDERFLAG);
  FunctionPublish_addInterface(LUXI_CLASS_L3D_LEVELMODEL,LUXI_CLASS_RENDERFLAG);

  //FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_new,NULL,"newrf",
  //  "(renderflag):() - returns empty renderflag, rarely needed. For most nodes you must directly operate on their own renderflaq.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_SUNLIT,"rfLitSun",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. Lit by sun");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_FXLIT,"rfLitFX",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state, only really affects l3dmodels. closest 3 l3dlights in the same l3dset will be used. If not lit by sun, but lit by fx lights we will turn ambientterm to 1, so you can use baked lighting in vertexcolors.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_FOG,"rfFog",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_NOCULL,"rfNocull",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. We will draw backfacing triangles too");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_NOVERTEXCOLOR,"rfNovertexcolor",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. Vertex colors arent used, instead a single color for all.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_NODEPTHMASK,"rfNodepthmask",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. Output pixels will not write into Z-Buffer");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_NODEPTHTEST,"rfNodepthtest",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. When disabled only pixels that have closer or equal distance than previous pixels will be drawn.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_NODRAW,"rfNodraw",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. We will not draw the node");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_PROJPASS,"rfProjectorpass",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state, forces surfaces affected by projectors into a new pass. Singlepass projector outcome sometimes might differ from multipass, especially when fog is envolved. If you need same treatment you can enforce it.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_WIRE,"rfWireframe",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state, specify linewidth in rendersurface");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_NORMALIZE,"rfNormalize",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state, normalization fixes lighting problems if your models are scaled, but costs a bit more. Flag is autoset if renderscale is not 1");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_BLEND,"rfBlend",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. Blending will compute outpixel as result of previous pixel and new pixel in the framebuffer (see rendersurface).");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_ALPHATEST,"rfAlphatest",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. Alphatest performs a test on the pixel's alpha value. Depending on the user set comparemode and alpha threshold (see rendersurface) the pixel will be drawn or not.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_NOCOLORMASK,"rfNocolormask",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. Pixel wont write into framebuffer ");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_FRONTCULL,"rfFrontcull",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. Frontfaces are culled (default is off which means backfaces are culled), makes no difference when rfNocull is set");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_STENCILMASK,"rfStencilmask",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. If set pixel will write into stencil buffer");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_STENCILTEST,"rfStenciltest",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. Stenciltest provides a mechanism to draw or discard pixel based on stencil buffer value. Detailed operations can be set with stencilcommand interface");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_NOLIGHTMAP,"rfNolightmap",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. Lightmap if active for this node will not be used. Useful for disabling lightmap on permesh basis.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFLAG,PubRenderflag_set,(void*)RENDER_UNLIT,"rfUnlit",
    "([boolean]):(renderflag,[boolean]) - returns or sets renderflag state. LitSun and LitFX will be ignored, this is mostly used in renderflags set by shaders.");

  FunctionPublish_initClass(LUXI_CLASS_OPERATIONMODE,"operationmode","Some functions need this information on what to do if certain tests fail or pass",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_OPERATIONMODE,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_OPERATIONMODE,PubOpMode_new,(void*)GL_KEEP,"keep",
    "(operationmode):() - keeps old value");
  FunctionPublish_addFunction(LUXI_CLASS_OPERATIONMODE,PubOpMode_new,(void*)GL_ZERO,"zero",
    "(operationmode):() - sets value to zero");
  FunctionPublish_addFunction(LUXI_CLASS_OPERATIONMODE,PubOpMode_new,(void*)GL_REPLACE,"replace",
    "(operationmode):() - replaces old with new value");
  FunctionPublish_addFunction(LUXI_CLASS_OPERATIONMODE,PubOpMode_new,(void*)GL_INCR,"increment",
    "(operationmode):() - increments old by one");
  FunctionPublish_addFunction(LUXI_CLASS_OPERATIONMODE,PubOpMode_new,(void*)GL_DECR,"decrement",
    "(operationmode):() - decrements old by one");
  FunctionPublish_addFunction(LUXI_CLASS_OPERATIONMODE,PubOpMode_new,(void*)GL_INVERT,"invert",
    "(operationmode):() - inverts old");

  FunctionPublish_initClass(LUXI_CLASS_LOGICMODE,"logicmode","bitwise logical operations between source and destination.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_LOGICMODE,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)0,"disable",
    "(logicmode):() - disables logic ops");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_CLEAR,"clear",
    "(logicmode):() - sets to 0");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_SET,"set",
    "(logicmode):() - sets to 1");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_COPY,"copy",
    "(logicmode):() - sets to source");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_COPY_INVERTED,"copyinv",
    "(logicmode):() - sets to negated source");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_NOOP,"noop",
    "(logicmode):() - keeps destination");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_INVERT,"invert",
    "(logicmode):() - inverts destination");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_AND,"and",
    "(logicmode):() - s & d");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_NAND,"nand",
    "(logicmode):() - ~(s & d)");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_OR,"or",
    "(logicmode):() -  s | d");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_NOR,"nor",
    "(logicmode):() - ~(s | d)");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_XOR,"xor",
    "(logicmode):() - s ^ d");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_EQUIV,"equivalent",
    "(logicmode):() - ~(s ^ d)");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_AND_REVERSE,"andrev",
    "(logicmode):() - s & ~d");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_AND_INVERTED,"andinv",
    "(logicmode):() - ~s & d");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_OR_REVERSE,"orrev",
    "(logicmode):() - s | ~d");
  FunctionPublish_addFunction(LUXI_CLASS_LOGICMODE,PubLogicMode_new,(void*)GL_OR_INVERTED,"orinv",
    "(logicmode):() - ~s | d");

  FunctionPublish_initClass(LUXI_CLASS_RENDERSTENCIL,"stencilcommand",
"Stencilcommand allows discarding pixels based on stencil buffer.\
 Here you can define the test and the write operations.\
 l2dflag will ignore twosided stenciling and use front for both, else twosided stenciling is only done when the rfNoCull flag is forced.\
 Other than that we use same operations (front) for front and back faces. Be aware that if system does not support twosided stenciling, we need to multipass, and we cannot guarantee that the result is exactly like with twosided support.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_RENDERSTENCIL,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addInterface(LUXI_CLASS_L2D_FLAG,LUXI_CLASS_RENDERSTENCIL);
  FunctionPublish_addInterface(LUXI_CLASS_RCMD_STENCIL,LUXI_CLASS_RENDERSTENCIL);
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSTENCIL,PubRenderStencil_cmd,(void*)STENCIL_FUNC,"scFunction",
    "([comparemode front,comparemode back,int threshold, int mask]):(stencilcommand,[comparemode front,comparemode back,int threshold, int mask]) - returns or sets how the stencil test should be done, and the binary mask for before writing is done.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSTENCIL,PubRenderStencil_cmd,(void*)STENCIL_OP,"scOperation",
    "([operationmode fail, operationmode zfail, operationmode zpass]):(stencilcommand,int side,[operationmode fail, operationmode zfail, operationmode zpass]) - returns or sets what to do when depth/stencil fails or passes. side 0 = frontface, 1 = backface. It defines how the value is combined with the old stencilbuffer value.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSTENCIL,PubRenderStencil_cmd,(void*)STENCIL_SET,"scEnabled",
    "([boolean]):(stencilcommand,[boolean state,[int side]]) - returns or sets if command should be enabled. Which side should be affected can also be set 0 = front, 1 = back, -1 both modes active (needs capability).");


  FunctionPublish_initClass(LUXI_CLASS_BLENDMODE,"blendmode","They define how texels or pixels are blend with the destination. Sometimes you might be able to use inverted blends, those just invert the self.alpha value",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_BLENDMODE,LUXI_CLASS_RENDERINTERFACE);

  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_ADD,"add",
    "(blendmode):() - out = destination + self");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_AMODADD,"amodadd",
    "(blendmode):() - out = destination + (self * self.alpha)");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_DECAL,"decal",
    "(blendmode):() - out = destination * (1-self.alpha) + self * (self.alpha)");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_MODULATE,"modulate",
    "(blendmode):() - out = destination*self");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_MODULATE2,"modulate2",
    "(blendmode):() - out = destination*self*2. Allows self to brigthen (>0.5) or darken (<0.5) at the same time.");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_MIN,"min",
    "(blendmode):() - out = min(destination,self).");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_MAX,"max",
    "(blendmode):() - out = max(destination,self).");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_SUB,"sub",
    "(blendmode):() - out = self - destination.");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_SUB_REV,"subrev",
    "(blendmode):() - out = destination - self.");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_AMODSUB,"amodsub",
    "(blendmode):() - out = (self * self.alpha) - destination.");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_AMODSUB_REV,"amodsubrev",
    "(blendmode):() - out = destination - (self * self.alpha).");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_REPLACE,"replace",
    "(blendmode):() - out = self");
  FunctionPublish_addFunction(LUXI_CLASS_BLENDMODE,PubBlendMode_new,(void*)VID_UNSET,"disable",
    "(blendmode):() - removes setting the blendmode, warning if rfBlend is still true you will get undefined behavior");

  FunctionPublish_initClass(LUXI_CLASS_TECHNIQUE,"technique","Techniques are used in shaders and depend on the graphics hardware capabilities.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_TECHNIQUE,LUXI_CLASS_RENDERINTERFACE);

  FunctionPublish_addFunction(LUXI_CLASS_TECHNIQUE,PubTechnique_new,(void*)VID_T_LOWDETAIL,"lowdetail",
    "(technique):() - lowdetail, overrides all other techniques");
  FunctionPublish_addFunction(LUXI_CLASS_TECHNIQUE,PubTechnique_new,(void*)VID_T_ARB_TEXCOMB,"arb_texcomb",
    "(technique):() - arbitrary texture access in combiners, cubemap textures, dot3 combiner.");
  FunctionPublish_addFunction(LUXI_CLASS_TECHNIQUE,PubTechnique_new,(void*)VID_T_ARB_V,"arb_v",
    "(technique):() - arb_vertexprograms support and arb_texcomb. Basic cg vertex programs are allowed too.");
  FunctionPublish_addFunction(LUXI_CLASS_TECHNIQUE,PubTechnique_new,(void*)VID_T_ARB_VF,"arb_vf",
    "(technique):() - same as arb_v but with arb_fragmentprograms support, also allows cg useage for better fragmentshaders.");
  FunctionPublish_addFunction(LUXI_CLASS_TECHNIQUE,PubTechnique_new,(void*)VID_T_ARB_VF_TEX8,"arb_vf_tex8",
    "(technique):() - same as arb_vf with 8 texture images");
  FunctionPublish_addFunction(LUXI_CLASS_TECHNIQUE,PubTechnique_new,(void*)VID_T_CG_SM3,"cg_sm3",
    "(technique):() - advanced cg support, shader model 3.0 and 16 texture images");
  FunctionPublish_addFunction(LUXI_CLASS_TECHNIQUE,PubTechnique_new,(void*)VID_T_CG_SM3,"cg_sm3_tex8",
    "(technique):() - advanced cg support, shader model 3.0 and 16 texture images");
  FunctionPublish_addFunction(LUXI_CLASS_TECHNIQUE,PubTechnique_new,(void*)VID_T_CG_SM4,"cg_sm4",
    "(technique):() - advanced cg support, shader model 4.0 and 32 texture images");
#ifdef LUX_VID_GEOSHADER
  FunctionPublish_addFunction(LUXI_CLASS_TECHNIQUE,PubTechnique_new,(void*)VID_T_CG_SM4_GS,"cg_sm4_gs",
    "(technique):() - advanced cg support, shader model 4.0 and 32 texture images");
#endif
  FunctionPublish_addFunction(LUXI_CLASS_TECHNIQUE,PubTechnique_supported,(void*)0,"supported",
    "(boolean):(technique) - checks if given technique is supported.");
  FunctionPublish_addFunction(LUXI_CLASS_TECHNIQUE,PubTechnique_supported,(void*)1,"supported_tex4",
    "(boolean):(technique) - checks if given technique is supported with 4 textures.");

  FunctionPublish_initClass(LUXI_CLASS_COMPAREMODE,"comparemode","For some rendering operations you can set compare behavior. eg. GREATER means a value passes when greater than a given user threshold.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_COMPAREMODE,LUXI_CLASS_RENDERINTERFACE);

  FunctionPublish_addFunction(LUXI_CLASS_COMPAREMODE,PubCompareMode_new,(void*)GL_GREATER,"greater",
    "(comparemode):() - ");
  FunctionPublish_addFunction(LUXI_CLASS_COMPAREMODE,PubCompareMode_new,(void*)GL_LESS,"less",
    "(comparemode):() - ");
  FunctionPublish_addFunction(LUXI_CLASS_COMPAREMODE,PubCompareMode_new,(void*)GL_EQUAL,"equal",
    "(comparemode):() - ");
  FunctionPublish_addFunction(LUXI_CLASS_COMPAREMODE,PubCompareMode_new,(void*)GL_NOTEQUAL,"notequal",
    "(comparemode):() - ");
  FunctionPublish_addFunction(LUXI_CLASS_COMPAREMODE,PubCompareMode_new,(void*)GL_GEQUAL,"gequal",
    "(comparemode):() - greater or equal");
  FunctionPublish_addFunction(LUXI_CLASS_COMPAREMODE,PubCompareMode_new,(void*)GL_LEQUAL,"lequal",
    "(comparemode):() - less or equal");
  FunctionPublish_addFunction(LUXI_CLASS_COMPAREMODE,PubCompareMode_new,(void*)GL_ALWAYS,"always",
    "(comparemode):() - any value passes");
  FunctionPublish_addFunction(LUXI_CLASS_COMPAREMODE,PubCompareMode_new,(void*)GL_NEVER,"never",
    "(comparemode):() - no value passes");
  FunctionPublish_addFunction(LUXI_CLASS_COMPAREMODE,PubCompareMode_new,(void*)0,"disable",
    "(comparemode):() - removes setting the compare, warning if rfAlphatest is still true you will get undefined behavior");
  FunctionPublish_addFunction(LUXI_CLASS_COMPAREMODE,PubCompareMode_run,(void*)0,"run",
    "(boolean):(comparemode,float value, float ref) - runs the comparemode and returns result");

  FunctionPublish_initClass(LUXI_CLASS_VERTEXTYPE,"vertextype","For usercreated meshes it is necessary to specify what vertextype you want to have. Different vertextypes have different attributes and sizes.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_VERTEXTYPE,LUXI_CLASS_RENDERINTERFACE);

  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_new,(void*)VERTEX_64_TEX4,"vertex64tex4",
    "(vertextype):() - size 64 bytes, normals (3 shorts), position (3 floats), color (4 unsigned bytes), 4 texcoord channels (4*2 floats), tangents (4 shorts). ");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_new,(void*)VERTEX_64_SKIN,"vertex64skin",
    "(vertextype):() - size 64 bytes, normals (3 shorts), position (3 floats), color (4 unsigned bytes), 2 texcoord channels (2*2 floats), tangents (4 shorts). Supports 4-weight hardware skinning using 4 unsigned bytes for indices and 4 unsigned shorts for weights.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_new,(void*)VERTEX_32_NRM,"vertex32normals",
    "(vertextype):() - size 32 bytes, normals (3 shorts), position (3 floats), color (4 unsigned bytes), 1 texcoord channel (2 floats).");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_new,(void*)VERTEX_32_TEX2,"vertex32tex2",
    "(vertextype):() - size 32 bytes, position (3 floats), color (4 unsigned byte), 2 texcoord channels (2*2 floats).");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_new,(void*)VERTEX_16_CLR,"vertex16color",
    "(vertextype):() - size 16 bytes, position (3 floats), color (4 unsigned byte).");

  FunctionPublish_initClass(LUXI_CLASS_MESHTYPE,"meshtype","The meshtype defines how a mesh's vertex and indexdata is stored for display. Depending on system different capabilities exist.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_MESHTYPE,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_MESHTYPE,PubMeshType_new,(void*)MESH_UNSET,"auto",
    "(meshtype):() - Either unset or pick 'best' depending on current material assignments.");
  FunctionPublish_addFunction(LUXI_CLASS_MESHTYPE,PubMeshType_new,(void*)MESH_VA,"ram",
    "(meshtype):() - Stored in regular ram, submitted with each drawcall. Slow for lots of data, but allows easy changes.");
  FunctionPublish_addFunction(LUXI_CLASS_MESHTYPE,PubMeshType_new,(void*)MESH_VBO,"vbo",
    "(meshtype):() - Store in bufferobjects in video/driver ram. Updates to local data");
  FunctionPublish_addFunction(LUXI_CLASS_MESHTYPE,PubMeshType_new,(void*)MESH_DL,"displaylist",
    "(meshtype):() - Store in displaylist in video ram. All needed attributes are stored along. Local copy is kept in ram as well.");

  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getOffset,(void*)VERTEX_SIZE,"memsize",
    "(int bytes):(vertextype) - returns size in bytes.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getOffset,(void*)VERTEX_TEX0,"memtotex0",
    "([int bytes]):(vertextype) - returns byte offset from start of the vertex to this component. Returns nil if component not part of vertex.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getOffset,(void*)VERTEX_TEX1,"memtotex1",
    "([int bytes]):(vertextype) - returns byte offset from start of the vertex to this component. Returns nil if component not part of vertex.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getOffset,(void*)VERTEX_TEX2,"memtotex2",
    "([int bytes]):(vertextype) - returns byte offset from start of the vertex to this component. Returns nil if component not part of vertex.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getOffset,(void*)VERTEX_TEX3,"memtotex3",
    "([int bytes]):(vertextype) - returns byte offset from start of the vertex to this component. Returns nil if component not part of vertex.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getOffset,(void*)VERTEX_POS,"memtopos",
    "([int bytes]):(vertextype) - returns byte offset from start of the vertex to this component. Returns nil if component not part of vertex.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getOffset,(void*)VERTEX_NORMAL,"memtonormal",
    "([int bytes]):(vertextype) - returns byte offset from start of the vertex to this component. Returns nil if component not part of vertex.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getOffset,(void*)VERTEX_TANGENT,"memtotangent",
    "([int bytes]):(vertextype) - returns byte offset from start of the vertex to this component. Returns nil if component not part of vertex.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getOffset,(void*)VERTEX_COLOR,"memtocolor",
    "([int bytes]):(vertextype) - returns byte offset from start of the vertex to this component. Returns nil if component not part of vertex.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getOffset,(void*)VERTEX_BLENDI,"memtoblendi",
    "([int bytes]):(vertextype) - returns byte offset from start of the vertex to this component. Returns nil if component not part of vertex.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getOffset,(void*)VERTEX_BLENDW,"memtoblendw",
    "([int bytes]):(vertextype) - returns byte offset from start of the vertex to this component. Returns nil if component not part of vertex.");

  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getScalarType,(void*)VERTEX_SCALARPOS,"postype",
    "([scalartype]):(vertextype) - returns scalartype of the attribute or nil if not found.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getScalarType,(void*)VERTEX_SCALARTEX,"textype",
    "([scalartype]):(vertextype) - returns scalartype of the attribute or nil if not found.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getScalarType,(void*)VERTEX_SCALARNORMAL,"normaltype",
    "([scalartype]):(vertextype) - returns scalartype of the attribute or nil if not found.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getScalarType,(void*)VERTEX_SCALARTANGENT,"tangenttype",
    "([scalartype]):(vertextype) - returns scalartype of the attribute or nil if not found.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getScalarType,(void*)VERTEX_SCALARCOLOR,"colortype",
    "([scalartype]):(vertextype) - returns scalartype of the attribute or nil if not found.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getScalarType,(void*)VERTEX_SCALARBLENDI,"blenditype",
    "([scalartype]):(vertextype) - returns scalartype of the attribute or nil if not found.");
  FunctionPublish_addFunction(LUXI_CLASS_VERTEXTYPE,PubVertexType_getScalarType,(void*)VERTEX_SCALARBLENDW,"blendwtype",
    "([scalartype]):(vertextype) - returns scalartype of the attribute or nil if not found.");

  FunctionPublish_initClass(LUXI_CLASS_TEXTYPE,"textype","For creating textures or renderbuffers this defines the internal storage. Be aware that the rendersystem might not always be able to provide the format you asked for. Some types are baseformats which will leave it up to driver on which internalmode to pick. Formats such as depth and depth_stencil require special capabilities",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_TEXTYPE,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_RGB,"rgb",
    "(textype):() - baseformat for RGB");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_RGBA,"rgba",
    "(textype):() - baseformat for RGBA");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_BGR,"bgr",
    "(textype):() - reverses order of data for RGB, internal remains baseformat");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_BGRA,"bgra",
    "(textype):() - reverses order of data for RGBA, internal remains baseformat");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_ABGR,"abgr",
    "(textype):() - reverses order of data for RGBA, internal remains baseformat");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_LUM,"lum",
    "(textype):() - baseformat for LUMINANCE");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_LUMALPHA,"lumalpha",
    "(textype):() - baseformat for LUMINANCE_ALPHA");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_R,"r",
    "(textype):() - baseformat for R (needs capability.texrg)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_RG,"rg",
    "(textype):() - baseformat for RG (needs capability.texrg)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_ALPHA,"alpha",
    "(textype):() - baseformat for ALPHA");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_STENCIL,"stencil",
    "(textype):() - baseformat for STENCIL_INDEX. only for renderbuffers");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_DEPTH,"depth",
    "(textype):() - baseformat for DEPTH_COMPONENT");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_DEPTH16,"depth16",
    "(textype):() - 16 bit DEPTH_COMPONENT");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_DEPTH24,"depth24",
    "(textype):() - 24 bit DEPTH_COMPONENT");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_DEPTH32,"depth32",
    "(textype):() - 32 bit DEPTH_COMPONENT");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTYPE,PubTexType_new,(void*)TEX_DEPTH_STENCIL,"depthstencil",
    "(textype):() - baseformat for DEPTH_STENCIL");

  FunctionPublish_initClass(LUXI_CLASS_TEXDATATYPE,"texdatatype","For creating textures this defines the internal storage precision. Some may require special capabilities",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_TEXDATATYPE,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_TEXDATATYPE,PubTexDataType_new,(void*)TEX_DATA_FIXED8,
    "fixed8",
    "(texdatatype):() - unsigned byte. normalized 0-1. 8-bit precision, local data storage as unsigned byte.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXDATATYPE,PubTexDataType_new,(void*)TEX_DATA_FIXED16,
    "fixed16",
    "(texdatatype):() - unsigned byte. normalized 0-1. 16-bit precision, local data storage as unsigned short.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXDATATYPE,PubTexDataType_new,(void*)TEX_DATA_FLOAT16,
    "float16",
    "(texdatatype):() - float16. arbitrary, 16-bit precision, local data storage as float32.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXDATATYPE,PubTexDataType_new,(void*)TEX_DATA_FLOAT32,
    "float32",
    "(texdatatype):() - float32. arbitrary, 32-bit precision, local same.");

  FunctionPublish_addFunction(LUXI_CLASS_TEXDATATYPE,PubTexDataType_new,(void*)TEX_DATA_INT,
    "int32",
    "(texdatatype):() - int32. -/+2147483647, 32-bit precision, local same. requires integer texture capability.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXDATATYPE,PubTexDataType_new,(void*)TEX_DATA_UINT,
    "uint32",
    "(texdatatype):() - unsigned int32. 0-4294967296, 32-bit precision, local same. requires integer texture capability.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXDATATYPE,PubTexDataType_new,(void*)TEX_DATA_SHORT,
    "int16",
    "(texdatatype):() - int16. -/+32767, 16-bit precision, local same. requires integer texture capability.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXDATATYPE,PubTexDataType_new,(void*)TEX_DATA_USHORT,
    "uint16",
    "(texdatatype):() - unsigned int16. 0-65535, 16-bit precision, local same. requires integer texture capability.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXDATATYPE,PubTexDataType_new,(void*)TEX_DATA_BYTE,
    "int8",
    "(texdatatype):() - int8. -/+127, 8-bit precision, local same. requires integer texture capability.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXDATATYPE,PubTexDataType_new,(void*)TEX_DATA_UBYTE,
    "uint8",
    "(texdatatype):() - unsigned int8. 0-255, 8-bit precision, local same. requires integer texture capability.");

  FunctionPublish_initClass(LUXI_CLASS_PRIMITIVETYPE,"primitivetype","Graphics hardware supports rendering of different primitive types. The way indices are interpreted will depend on the indexprimitivetype.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_PRIMITIVETYPE,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_PRIMITIVETYPE,PubRendermode_new,(void*)PRIM_POINTS,"points",
    "(primitivetype):() - points. Each index will be a point. Using vertexshaders pointsize can be influenced.");
  FunctionPublish_addFunction(LUXI_CLASS_PRIMITIVETYPE,PubRendermode_new,(void*)PRIM_LINES,"lines",
    "(primitivetype):() - line list. Every two indices make a line. Rendersurface interface can be used to influence appearance.");
  FunctionPublish_addFunction(LUXI_CLASS_PRIMITIVETYPE,PubRendermode_new,(void*)PRIM_LINE_LOOP,"lineloop",
    "(primitivetype):() - closed line loop. Each index is a line point connected to previous index, a last line segment is added automatically to first index. Rendersurface interface can be used to influence appearance.");
  FunctionPublish_addFunction(LUXI_CLASS_PRIMITIVETYPE,PubRendermode_new,(void*)PRIM_LINE_STRIP,"linestrip",
    "(primitivetype):() - line strip. Each index is a line point connected to previous index, not closed. Rendersurface interface can be used to influence appearance.");
  FunctionPublish_addFunction(LUXI_CLASS_PRIMITIVETYPE,PubRendermode_new,(void*)PRIM_TRIANGLES,"triangles",
    "(primitivetype):() - triangle list. Every 3 indices make a triangle.");
  FunctionPublish_addFunction(LUXI_CLASS_PRIMITIVETYPE,PubRendermode_new,(void*)PRIM_TRIANGLE_STRIP,"trianglestrip",
    "(primitivetype):() - triangle strip. After the first 2 indices, each new index spans a triangle with last 3 indices.");
  FunctionPublish_addFunction(LUXI_CLASS_PRIMITIVETYPE,PubRendermode_new,(void*)PRIM_TRIANGLE_FAN,"trianglefan",
    "(primitivetype):() - triangle fan. First index becomes center, all others are connected to it and previous index.");
  FunctionPublish_addFunction(LUXI_CLASS_PRIMITIVETYPE,PubRendermode_new,(void*)PRIM_QUADS,"quads",
    "(primitivetype):() - quad list. Every four indices make a quad.");
  FunctionPublish_addFunction(LUXI_CLASS_PRIMITIVETYPE,PubRendermode_new,(void*)PRIM_QUAD_STRIP,"quadstrip",
    "(primitivetype):() - quad strip. After the first 2 indices, each 2 new indices creat a quad with last 4 indices.");
  FunctionPublish_addFunction(LUXI_CLASS_PRIMITIVETYPE,PubRendermode_new,(void*)PRIM_POLYGON,"polygon",
    "(primitivetype):() - polygon. All indices create the outer closed line of a polygon, which becomes triangulated by the driver internally, undefined behavior for non-convex polygons.");


  FunctionPublish_initClass(LUXI_CLASS_FONTSET,"fontset","It defines how each character of a string is rendered. You can define what grid the font texture has, and which quad of the grid each character takes.",NULL,LUX_TRUE);
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_new,(void*)-1,"new",
    "(fontset):([int size=16])  - returns a new fontset with default values. Allowed values for size are 6, 8 and 16. Size denotes the number of rows and columns in the fonttexture, which results in the number of glyphs for the font (36, 64 or 256).");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_new,(void*)FONTSET_SIZE_16X16,"new16x16",
    "(fontset):()  - returns a new fontset with default values.");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_new,(void*)FONTSET_SIZE_8X8,"new8x8",
    "(fontset):()  - returns a new fontset with default values.");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_new,(void*)FONTSET_SIZE_6X6,"new6x6",
    "(fontset):()  - returns a new fontset with default values.");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_attr,(void*)PFS_LOOKUP,"lookup",
    "(int):(fontset,int character,[int gridfield])  - returns or sets which grid field should be used for this character. gridfield must be between 0 and maxcharacters");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_attr,(void*)PFS_WIDTH,"width",
    "([int]):(fontset,int character,[float width])  - returns or sets character width (0-1).");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_attr,(void*)PFS_LINE,"linespacing",
    "([float]):(fontset,[float])  - returns or sets size of a line, default is 16");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_attr,(void*)PFS_OFFSETX,"offsetx",
    "([float]):(fontset,[float])  - returns or sets glyphoffset from topleft");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_attr,(void*)PFS_OFFSETY,"offsety",
    "([float]):(fontset,[float])  - returns or sets glyphoffset from topleft");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_attr,(void*)PFS_IGSPECIALS,"ignorespecials",
    "([boolean]):(fontset,[boolean])  - returns or sets if special command characters suchs as \\n should be ignored.");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_attr,(void*)PFS_MAX,"maxcharacters",
    "(int):(fontset)  - returns how many gridfields exist.");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_attr,(void*)PFS_DELETE,"delete",
    "():(fontset)  - deletes the fontset.");
  //FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_attr,(void*)PFS_DELETE,"deleteall",
  //  "():(fontset)  - deletes all fontsets except default.");
  FunctionPublish_addFunction(LUXI_CLASS_FONTSET,PubFontSet_attr,(void*)PFS_DEFAULT,"getdefault",
    "(fontset):()  - returns default fontset, you shouldnt change it as console and other drawings rely on it.");


  FunctionPublish_initClass(LUXI_CLASS_TEXCOMBINER,"texcombiner","A texcombiner defines how textures are blend together / what color/alpha values they return. Luxinia comes with a bunch of predefined combiners that the shadercompiler uses and which you can access in the shaderscript. If those are not enough or you need specific operations to be done you can define your own combiners. You must make sure they run well, as there wont be error checking. GL_ARB_texture_env_combine is the main extension this system builds on.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_TEXCOMBINER,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_initClass(LUXI_CLASS_TEXCOMBINER_CFUNC,"texcombcolor","The texcombiner for rgb values. The functions return a texcombiner or overwrite the combiner with same name. They may return an error when a conflict with default names exists.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_TEXCOMBINER_CFUNC,LUXI_CLASS_TEXCOMBINER);
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)GL_REPLACE,"replace",
    "([texcombcolor]):(string name) -  OUT = arg0");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)GL_ADD,"add",
    "([texcombcolor]):(string name) -  OUT = arg0 + arg1");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)GL_MODULATE,"modulate",
    "([texcombcolor]):(string name) -  OUT = arg0 * arg1");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)GL_ADD_SIGNED,"addsigned",
    "([texcombcolor]):(string name) -  OUT = arg0 + arg1 - 0.5");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)GL_INTERPOLATE,"interpolate",
    "([texcombcolor]):(string name) -  OUT = arg0 * arg2 + arg1 * (1-arg2)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)GL_MODULATE_SIGNED_ADD_ATI,"modaddsigned",
    "([texcombcolor]):(string name) -  OUT = arg0 * arg2 + arg1 - 0.5. You should query for capability.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)GL_MODULATE_ADD_ATI,"modadd",
    "([texcombcolor]):(string name) -  OUT = arg0 * arg2 + arg1. You should query for capability.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)GL_SUBTRACT,"subtract",
    "([texcombcolor]):(string name) -  OUT = arg0 - arg1. You should query for capability. When used modadd/combine4 for alpha at the same time is not allowed for cards with combine4 support");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)(GL_COMBINE4_NV+GL_ADD),"combine4",
    "([texcombcolor]):(string name) -  OUT = arg0*arg1 + arg2*arg3. You should query for capability.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)(GL_COMBINE4_NV+GL_ADD_SIGNED),"combine4signed",
    "([texcombcolor]):(string name) -  OUT = arg0*arg1 + arg2*arg3 - 0.5  You should query for capability.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)GL_DOT3_RGB,"dot3",
    "([texcombcolor]):(string name) -  OUT = arg0 dotproduct arg1. (args as signed vector -1,+1). You should query for capability");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerCFunc_new,(void*)GL_DOT3_RGBA,"dot3alpha",
    "([texcombcolor]):(string name) -  OUT = arg0 dotproduct arg1. (args as signed vector -1,+1). Also sets alpha. You should query for capability");

  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerFunc_attr,(void*)TEXC_SETARG,"setarg",
    "():(texcombcolor,int arg,texcombsrc,texcombop) -  sets argument of the function. Check the function descriptor for which arg index does what.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_CFUNC,PubTexCombinerFunc_attr,(void*)TEXC_TEST,"test",
    "([string]):(texcombcolor) - tests the combiner (binds it). Returns GL Error string (might have other errors not bound to this problem)");

  FunctionPublish_initClass(LUXI_CLASS_TEXCOMBINER_AFUNC,"texcombalpha","The texcombiner for alpha values. Make sure that texcombop are only ALPHA values.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_TEXCOMBINER_AFUNC,LUXI_CLASS_TEXCOMBINER);
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerAFunc_new,(void*)GL_REPLACE,"replace",
    "([texcombalpha]):(string name) -  OUT = arg0");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerAFunc_new,(void*)GL_ADD,"add",
    "([texcombalpha]):(string name) -  OUT = arg0 + arg1");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerAFunc_new,(void*)GL_MODULATE,"modulate",
    "([texcombalpha]):(string name) -  OUT = arg0 * arg1");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerAFunc_new,(void*)GL_ADD_SIGNED,"addsigned",
    "([texcombalpha]):(string name) -  OUT = arg0 + arg1 - 0.5");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerAFunc_new,(void*)GL_INTERPOLATE,"interpolate",
    "([texcombalpha]):(string name) -  OUT = arg0 * arg2 + arg1 * (1-arg2)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerAFunc_new,(void*)GL_MODULATE_SIGNED_ADD_ATI,"modaddsigned",
    "([texcombalpha]):(string name) -  OUT = arg0 * arg2 + arg1 - 0.5. You should query for capability. When used color mode must be modadd/combine4 for cards with combine4 support as well.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerAFunc_new,(void*)GL_MODULATE_ADD_ATI,"modadd",
    "([texcombalpha]):(string name) -  OUT = arg0 * arg2 + arg1. You should query for capability. When used color mode must be modadd/combine4 for cards with combine4 support as well.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerAFunc_new,(void*)GL_SUBTRACT,"subtract",
    "([texcombalpha]):(string name) -  OUT = arg0 - arg1. You should query for capability. When used modadd/combine4 for color at the same time is not allowed for cards with combine4 support");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerAFunc_new,(void*)(GL_COMBINE4_NV+GL_ADD),"combine4",
    "([texcombalpha]):(string name) -  OUT = arg0*arg1 + arg2*arg3. You should query for capability. Color mode must be combine4/modadd at the same time");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerAFunc_new,(void*)(GL_COMBINE4_NV+GL_ADD_SIGNED),"combine4signed",
    "([texcombalpha]):(string name) -  OUT = arg0*arg1 + arg2*arg3 - 0.5  You should query for capability. Color mode must be combine4/modadd at the same time");

  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerFunc_attr,(void*)TEXC_SETARG,"setarg",
    "():(texcombalpha,int arg,texcombsrc,texcombop) -  sets argument of the function. Check the function descriptor for which arg index does what. Make sure operand is always alpha or invalpha.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_AFUNC,PubTexCombinerFunc_attr,(void*)TEXC_TEST,"test",
    "([string]):(texcombalpha) - tests the combiner (binds it). Returns GL Error string (might have other errors not bound to this problem)");

  FunctionPublish_initClass(LUXI_CLASS_TEXCOMBINER_SRC,"texcombsrc","The texcombiner source for the function specifies where values should be taken from.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_TEXCOMBINER_SRC,LUXI_CLASS_TEXCOMBINER);
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_SRC,PubTexCombinerSrc_new,(void*)GL_CONSTANT,"constant",
    "(texcombsrc):() - constant value (texture env color)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_SRC,PubTexCombinerSrc_new,(void*)GL_PRIMARY_COLOR,"vertex",
    "(texcombsrc):() - vertex color/alpha");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_SRC,PubTexCombinerSrc_new,(void*)GL_TEXTURE,"texture",
    "(texcombsrc):() - current texture");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_SRC,PubTexCombinerSrc_new,(void*)GL_PREVIOUS,"previous",
    "(texcombsrc):() - previous result");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_SRC,PubTexCombinerSrc_new,(void*)GL_TEXTURE0,"texture0",
    "(texcombsrc):() - texture at unit 0 (needs crossbar capability / higher technique than default)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_SRC,PubTexCombinerSrc_new,(void*)GL_TEXTURE1,"texture1",
    "(texcombsrc):() - texture at unit 1 (needs crossbar capability / higher technique than default)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_SRC,PubTexCombinerSrc_new,(void*)GL_TEXTURE2,"texture2",
    "(texcombsrc):() - texture at unit 2 (needs crossbar capability / higher technique than default)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_SRC,PubTexCombinerSrc_new,(void*)GL_TEXTURE3,"texture3",
    "(texcombsrc):() - texture at unit 3 (needs crossbar capability / higher technique than default)");

  FunctionPublish_initClass(LUXI_CLASS_TEXCOMBINER_OP,"texcombop","The texcombiner operand are the values that are used from the source.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_TEXCOMBINER_OP,LUXI_CLASS_TEXCOMBINER);
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_OP,PubTexCombinerOp_new,(void*)GL_SRC_COLOR,"color",
    "(texcombop):() - RGB value");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_OP,PubTexCombinerOp_new,(void*)GL_ONE_MINUS_SRC_COLOR,"invcolor",
    "(texcombop):() - (1 - RGB) value. (Inverted colors)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_OP,PubTexCombinerOp_new,(void*)GL_SRC_ALPHA,"alpha",
    "(texcombop):() - Alpha value");
  FunctionPublish_addFunction(LUXI_CLASS_TEXCOMBINER_OP,PubTexCombinerOp_new,(void*)GL_ONE_MINUS_SRC_ALPHA,"invalpha",
    "(texcombop):() - (1 - Alpha) value");

  FunctionPublish_initClass(LUXI_CLASS_VISTEST,"vistest",
    "The Vistest system is responsible for culling of spatialnodes, which can be drawable. It also takes care of assigning l3dlights and l3dprojects to such nodes. With render.drawvisspace you can make several of the octrees used visible.",NULL,LUX_FALSE);

  FunctionPublish_addFunction(LUXI_CLASS_VISTEST,PubVisTest_tweak,(void*)VIS_TWEAK_MAXDEPTH_SCT,"maxdepthscene",
    "([int]) : ([int]) - sets/gets maximum octree depth for scenenode octree.");
  FunctionPublish_addFunction(LUXI_CLASS_VISTEST,PubVisTest_tweak,(void*)VIS_TWEAK_MAXDEPTH_ACT,"maxdepthactor",
    "([int]) : ([int]) - sets/gets maximum octree depth for actornode octree.");
  FunctionPublish_addFunction(LUXI_CLASS_VISTEST,PubVisTest_tweak,(void*)VIS_TWEAK_MAXDEPTH_CAM,"maxdepthcamera",
    "([int]) : ([int]) - sets/gets maximum octree depth for l3dcamera octree.");
  FunctionPublish_addFunction(LUXI_CLASS_VISTEST,PubVisTest_tweak,(void*)VIS_TWEAK_MAXDEPTH_PROJ,"maxdepthproj",
    "([int]) : ([int]) - sets/gets maximum octree depth for l3dprojector octree.");
  FunctionPublish_addFunction(LUXI_CLASS_VISTEST,PubVisTest_tweak,(void*)VIS_TWEAK_MAXDEPTH_LIGHT,"maxdepthlight",
    "([int]) : ([int]) - sets/gets maximum octree depth for l3dlight octree.");
  FunctionPublish_addFunction(LUXI_CLASS_VISTEST,PubVisTest_tweak,(void*)VIS_TWEAK_MAXDEPTH_LIGHTVIS,"maxdepthvislight",
    "([int]) : ([int]) - sets/gets maximum octree depth for visible l3dlight octree, used for light source finding.");
  FunctionPublish_addFunction(LUXI_CLASS_VISTEST,PubVisTest_tweak,(void*)VIS_TWEAK_VOLCHECKCNT,"volcheckcnt",
    "([int]) : ([int]) - sets/gets how many items an octree node must have to do an additional check on its subvolume, which might be smaller than the ocnodes outer bounds.");



  FunctionPublish_initClass(LUXI_CLASS_RENDERMESH,"rendermesh","This class allows creation of custom meshes with all supported primitivetypes. Typically usermeshes are created 'in place' for l3dprimitive, l2dimage and rcmddrawmesh. But you can generate them externally and share as well.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RENDERMESH,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addInterface(LUXI_CLASS_RENDERMESH,LUXI_CLASS_INDEXARRAY);
  FunctionPublish_addInterface(LUXI_CLASS_RENDERMESH,LUXI_CLASS_VERTEXARRAY);
  FunctionPublish_addFunction(LUXI_CLASS_RENDERMESH,PubRenderMesh_new,
    (void*)NULL,"new",
    "():(vertextype, int vertices, indices, [vidbuffer vbo], [int offset], [vidbuffer ibo],[int offset]) - used for vertex attribute streams. Needs VBO capability"
    "():(l3dprimitive, vertextype, int numverts, int numindices, [vidbuffer vbo], [int vbooffset], [vidbuffer ibo], [int ibooffset]) - gives the l3dprimitive a unique mesh."
    "The new rendermesh is completely empty, so make sure you fill it with content before it "
    "gets rendered the first time (use indexarray and vertexarray interfaces). "
    "You can pass vidbuffers for content as well. Uses the byte offset passed to the buffers or (-1) "
    "allocates from the buffer. Allocation is useful if multiple meshes shall reside in same vidbuffers, "
    "manual offset is needed if you want to make use of instancing the same buffer content multiple time."
    "Don't forget to set indexCount and vertexCount and at the end indexMinmax, if you use indices! When VBOs are passed then the corresponding memory will not exist in system memory, which means accessing individual indices and vertices via indexarray and vertexarray will not work directly."
    );

  FunctionPublish_initClass(LUXI_CLASS_BUFFERTYPE,"vidtype","The main purpose of the vidbuffer is described here. OpenGL allows iterchanging the useage of the buffer.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_BUFFERTYPE,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERTYPE,PubBufferType_new,
    (void*)VID_BUFFER_VERTEX,"vertex",
    "(vidtype):() - used for vertex attribute streams. Needs VBO capability");
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERTYPE,PubBufferType_new,
    (void*)VID_BUFFER_INDEX,"index",
    "(vidtype):() - used for primitive index streams. Needs VBO capability");
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERTYPE,PubBufferType_new,
    (void*)VID_BUFFER_PIXELINTO,"pixelinto",
    "(vidtype):() - used for reading pixel data into the buffer. Needs PBO capability");
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERTYPE,PubBufferType_new,
    (void*)VID_BUFFER_PIXELFROM,"pixelfrom",
    "(vidtype):() - used for writing pixel data from the buffer. Needs PBO capability");
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERTYPE,PubBufferType_new,
    (void*)VID_BUFFER_TEXTURE,"pixelto",
    "(vidtype):() - used for writing pixel data from the buffer. Needs TBO capability");
  /*
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERTYPE,PubBufferType_new,
    (void*)VID_BUFFER_UNIFORM,"uniform",
    "(vidtype):() - used for storing uniforms in buffers. Needs UBO capability");
    */
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERTYPE,PubBufferType_new,
    (void*)VID_BUFFER_FEEDBACK,"feedback",
    "(vidtype):() - used for storing results of vertex transformations. Needs XFBO capability");

  FunctionPublish_initClass(LUXI_CLASS_BUFFERHINT,"vidhint","The dominant memory update behavior of a vidbuffer. Frequency: "
    "* 0 content update very rarely and only used a few times (stream).\n"
    "* 1 content update very rarely but used often. (static)\n"
    "* 2 content update frequently and used often. (dynamic)\n\n"
    "A typical vertexbuffer hint would be draw.1"
    ,NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_BUFFERHINT,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERHINT,PubBufferHint_new,
    (void*)VID_BUFFERHINT_STREAM_DRAW,"draw",
    "(vidhint):(int frequency 0-2) - content from application, read by graphics. 0 update rare used rare, 1 update rare, used often, 2 update often used often.");
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERHINT,PubBufferHint_new,
    (void*)VID_BUFFERHINT_STREAM_READ,"read",
    "(vidhint):(int frequency 0-2) - content from graphics, read by application. 0 update rare used rare, 1 update rare, used often, 2 update often used often.");
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERHINT,PubBufferHint_new,
    (void*)VID_BUFFERHINT_STREAM_COPY,"copy",
    "(vidhint):(int frequency 0-2) - content from graphics, ready by graphics. 0 update rare used rare, 1 update rare, used often, 2 update often used often.");

  FunctionPublish_initClass(LUXI_CLASS_BUFFERMAPPING,"vidmapping",
    "Defines the access type of a pointer that is mapped into application memory."
    ,NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_BUFFERMAPPING,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERMAPPING,PubBufferMapping_new,
    (void*)VID_BUFFERMAP_READ,"read",
    "(vidmapping):() - content only read");
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERMAPPING,PubBufferMapping_new,
    (void*)VID_BUFFERMAP_WRITE,"write",
    "(vidmapping):() - content only written");
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERMAPPING,PubBufferMapping_new,
    (void*)VID_BUFFERMAP_READWRITE,"readwrite",
    "(vidmapping):() - content written and read");
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERMAPPING,PubBufferMapping_new,
    (void*)VID_BUFFERMAP_WRITEDISCARD,"discard",
    "(vidmapping):() - content within mapping will be overwritten.");
  FunctionPublish_addFunction(LUXI_CLASS_BUFFERMAPPING,PubBufferMapping_new,
    (void*)VID_BUFFERMAP_WRITEDISCARDALL,"discardall",
    "(vidmapping):() - complete buffer content will be overwritten.");

  FunctionPublish_initClass(LUXI_CLASS_VIDBUFFER,"vidbuffer",
    "Allows allocation of memory mostly resident on graphics card, or managed by driver for fast data exchange. Several vidbuffertypes exist, OpenGL allows changing use of same buffer during a lifetime."
    ,NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_VIDBUFFER,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_new,
    (void*)NULL,"new",
    "(vidbuffer):(vidtype,vidhint,int sizebytes,[pointer]) - creates a new buffer with the given size. Optionally copies content from given pointer.");
  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_attr,
    (void*)PVBUF_MAPGL,"map",
    "(pointer start,end):(vidbuffer, vidmapping) - maps the full buffer into application memory. "
    "While mapped other content operations (retreve or submit) are not allowed.");
  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_attr,
    (void*)PVBUF_MAPRANGEGL,"maprange",
    "(pointer start,end):(vidbuffer, vidmapping, int from, sizebytes, [bool manualflush], [bool unsync]) - maps the buffer into application memory. "
    "Only the specified range is mapped if maprange capability exists, "
    "otherwise full is mapped and pointers offset to match range. The last arguments are only "
    "valid for maprange capability."
    "Manual flush means that the data is not marked 'valid' after unmap, but requires calls to flush."
    "Unsynchronized access means data can be accessed even if still modified by graphics."
    "While mapped other content operations (retreve or submit) are not allowed.");
  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_attr,
    (void*)PVBUF_MAPPED,"mapped",
    "([pointer start,end]):(vidbuffer) - is the buffer mapped, if yes returns pointers");
  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_attr,
    (void*)PVBUF_UNMAPGL,"unmap",
    "():(vidbuffer) - unmaps the buffer");
  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_attr,
    (void*)PVBUF_ALLOC,"localalloc",
    "(int offset):(vidbuffer, int sizebytes, [int alignpad=4], [pointer data]) - advances the internal allocation state. "
    "To aid storing multiple content in same buffer (speed benefit) as well as mapping safety, "
    "the returned offset can be used to identify the region of current allocation. "
    "The allocation can be aligned to a given size. For example storing multiple vertextypes "
    "in same buffer, works if they all allocation starts are aligned to the greatest vertexsize."
    "If a pointer is passed, then its content is submitted as well."
);
  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_attr,
    (void*)PVBUF_RETRIEVEGL,"retrieve",
    "():(vidbuffer,int frombyte, sizebytes, pointer output) - stores the data into the given pointer.");
  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_attr,
    (void*)PVBUF_SUBMITGL,"submit",
    "(int bytes):(vidbuffer,int frombyte, sizebytes, pointer output) - submits data from the given pointer.");
  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_attr,
    (void*)PVBUF_RELEASEGL,"release",
    "(int bytes):(vidbuffer) - deletes the buffer (driver might delay resource release, if still in use).");
  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_attr,
    (void*)PVBUF_CPYGL,"copy",
    "():(vidbuffer,int dstoffset, vidbuffer src, int srcoffset, size) - copies data from one to another buffer (or same, if interval doesnt overlap). Buffers may not be mapped.");

  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_attr,
    (void*)PVBUF_KEEP,"keeponloss",
    "([bool]):(vidbuffer,[bool]) - whether to keep a temporary local copy of the data in case of context loss, such as fullscreen toggle. Default is false.");

  FunctionPublish_addFunction(LUXI_CLASS_VIDBUFFER,PubVidBuffer_attr,
    (void*)PVBUF_GLID,"glid",
    "(pointer):(vidbuffer,[vidtype/other]) - returns the native graphics buffer identifier stored in a pointer. Cast the (void*) to (GLuint). If you pass a vidtype, the buffer is also bound, if you pass any none vidtype, the buffer is unbound from all possible types. As OpenGL uses reference counting itself, the buffer is kept alive, even if luxinia flags it for deletion. Useful for interoperability with CUDA/OpenCL.");
}
