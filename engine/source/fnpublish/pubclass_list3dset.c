// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../render/gl_list3d.h"
#include "../render/gl_list2d.h"
#include "../common/3dmath.h"
#include "../scene/scenetree.h"
#include "../render/gl_window.h"


// Published here:
// LUXI_CLASS_L3D_SET
// LUXI_CLASS_L3D_VIEW
// LUXI_CLASS_L3D_LAYERID
// LUXI_CLASS_L3D_BATCHBUFFER
// LUXI_CLASS_RCMD


//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_L3D_SET
static int PubL3DSet_get (PState pstate,PubFunction_t *f, int n)
{
  int lset;

  if (!n || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&lset))
    return FunctionPublish_returnError(pstate,"1 int required");

  return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_SET,(void*)lset);
}

static int PubL3DSet_default(PState pstate,PubFunction_t *f, int n)
{
  if (n == 0)
    return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_SET,(void*)g_List3D.setdefault);

  if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_SET,(void*)&g_List3D.setdefault))
    return FunctionPublish_returnError(pstate,"1 l3dset required");

  return 0;
}

static int PubL3DSet_getlayer (PState pstate,PubFunction_t *fn, int n)
{
  int lset = g_List3D.setdefault;
  int layer;
  int read = FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_SET,(void*)&lset);


  if (!FunctionPublish_getNArg(pstate,read,LUXI_CLASS_INT,(void*)&layer))
    return FunctionPublish_returnError(pstate,"[1 l3dset] 1 int required");

  if (!read)
    g_List3D.layerdefault = layer;

  return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_LAYERID,(void*)(lset*LIST3D_LAYERS+layer));
}

enum PubL3DSetCmds_e
{
  PLSET_DISABLED,
  PLSET_SUN,
  PLSET_DEFAULTVIEW,
  PLSET_UPDATEALL,
};

static int PubL3DSet_attributes(PState pstate,PubFunction_t *fn, int n)
{
  Reference ref;
  List3DSet_t *lset;
  int lsetid;

  if ((n!=1 && n!=2) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_SET,(void*)&lsetid))
    return FunctionPublish_returnError(pstate,"1 l3dset required");

  lset = &g_List3D.drawSets[lsetid];

  switch((int)fn->upvalue) {
  case PLSET_DISABLED:
    if (n == 1) return FunctionPublish_returnBool(pstate,lset->disabled ? LUX_TRUE : LUX_FALSE);
    else FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&lset->disabled);
    break;
  case PLSET_SUN:
    if (n==2){
      List3DNode_t *l3d;
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LIGHT,(void*)&ref) || !Reference_get(ref,l3d))
        lset->sun = NULL;
      lset->sun = l3d->light;
    }
    else if (lset->sun && lset->sun->l3dnode){
      return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_LIGHT,REF2VOID(lset->sun->l3dnode->reference));
    }
    break;
  case PLSET_DEFAULTVIEW:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_VIEW,REF2VOID(lset->defaultView.reference));
    break;
  case PLSET_UPDATEALL:
    List3DSet_updateAll(lset);
    break;
  default:
    break;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_L3D_VIEW

static int PubL3DView_new(PState pstate,PubFunction_t *fn, int n)
{
  int rcmdbias = 0;
  List3DView_t *lview;

  if (FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&rcmdbias) && rcmdbias < 1-LIST3D_VIEW_DEFAULT_COMMANDS)
    return FunctionPublish_returnError(pstate,"rcmdbias may not be < -254");

  lview = List3DView_new(rcmdbias);

  Reference_makeVolatile(lview->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_VIEW,REF2VOID(lview->reference));
}

enum VIEWATTR_UP_e{
  VAT_CAMAXIS,
  VAT_FOG,
  VAT_FOGSTART,
  VAT_FOGEND,
  VAT_FOGDENS,
  VAT_FOGCOLOR,
  VAT_SKYBOX,
  VAT_CAMERA,
  VAT_DELETE,
  VAT_ACTIVATE,
  VAT_DEACTIVATE,
  VAT_FULLWINDOW,
  VAT_VIEWPOS,
  VAT_VIEWSIZE,
  VAT_VIEWREFBOUNDS,
  VAT_DEPTHONLY,
  VAT_DEPTHBOUNDS,
  VAT_USEPOLYOFFSET,
  VAT_POLYOFFSET,
  VAT_CAMVIS,
  VAT_FILLLAYERS,
  VAT_RCMDVIS,
  VAT_RCMDADD,
  VAT_RCMDEMPTY,
  VAT_RCMDREMOVE,
  VAT_RCMDGET,
  VAT_DRAW,
};

extern int PubL3D_return(PState pstate,List3DNode_t *l3d);
extern int PubL2D_return(PState pstate,List2DNode_t *l2d);

static int PubL3DView_attributes(PState pstate,PubFunction_t *fn, int n)
{
  Reference ref;
  Reference ref2;
  RenderBackground_t *rback;
  RenderCmd_t *rcmd;
  RenderCmd_t *rcmd2;
  int tex;
  int id;
  List3DView_t *lview;
  List3DView_t *lview2;
  List3DNode_t *l3dnode;
  lxVector2 pos;
  lxVector2 size;

  byte state;
  byte state2;

  if ((n==0) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_VIEW,(void*)&ref) ||
    !Reference_get(ref,lview))
    return FunctionPublish_returnError(pstate,"1 l3dview required");

  rback = &lview->background;


  switch((int)fn->upvalue) {
  case VAT_FOG:
    if (n == 1) return FunctionPublish_returnBool(pstate,rback->fogmode ? LUX_TRUE : LUX_FALSE);
    else {
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
        return 0;
      if (state) rback->fogmode = GL_LINEAR;
      else rback->fogmode = 0;
    }
    break;
  case  VAT_RCMDGET:
    id = 0;
    if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&id) && id < lview->numCommands && id >= 0){
      Reference ref = lview->commands[id]->reference;
      return FunctionPublish_returnType(pstate,Reference_type(ref),REF2VOID(ref));
    }

    return 0;
  case  VAT_RCMDREMOVE:
    id = 0;
    if (n < 2 || 1>(tex=FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_RCMD,(void*)&ref2,LUXI_CLASS_INT,(void*)&id))
      || !Reference_get(ref2,rcmd))
      return FunctionPublish_returnError(pstate,"1 rcmd [1 int] required");

    state = List3DView_rcmdRemove(lview,rcmd,id);

    return FunctionPublish_returnBool(pstate,state);
  case  VAT_RCMDADD:
    if (lview->numCommands >= lview->maxCommands)
      return FunctionPublish_returnError(pstate,"l3dview already has max rcmds");
    id = 0;
    state2 = LUX_FALSE;
    rcmd2 = NULL;

    if (n < 2 || 1>(tex=FunctionPublish_getArgOffset(pstate,1,4,LUXI_CLASS_RCMD,(void*)&ref,LUXI_CLASS_RCMD,(void*)&ref2,LUXI_CLASS_INT,(void*)&id,LUXI_CLASS_BOOLEAN,(void*)&state2))
      || !Reference_get(ref,rcmd) || (tex>1 && !Reference_get(ref2,rcmd2))
      )
      return FunctionPublish_returnError(pstate,"1 rcmd [1 rcmd 1 int]/[1 boolean] required");

    if ( n==3 && tex == 1){
      FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&state2);
    }

    state = List3DView_rcmdAdd(lview,rcmd,state2,rcmd2,id);

    return FunctionPublish_returnBool(pstate,state);
    break;
  case  VAT_RCMDEMPTY:
    List3DView_rcmdEmpty(lview);
    break;
  case VAT_RCMDVIS:
    if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&tex) || tex < 0 || tex > 31)
      return FunctionPublish_returnError(pstate,"1 l3dview 1 (0..31) int required");
    if (n==2) return FunctionPublish_returnBool(pstate,isFlagSet(lview->rcmdflag ,1<<tex));
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 l3dview 1 int (0..31) 1 boolean required");
    if (state)
      lview->rcmdflag |= 1<<tex;
    else
      lview->rcmdflag  &= ~(1<<tex);
    break;
  case VAT_CAMVIS:
    if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&tex) || tex < 0 || tex > 31)
      return FunctionPublish_returnError(pstate,"1 l3dview 1 (0..31) int required");
    if (n==2) return FunctionPublish_returnBool(pstate,isFlagSet(lview->camvisflag ,1<<tex));
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 l3dview 1 int (0..31) 1 boolean required");
    if (state)
      lview->camvisflag |= 1<<tex;
    else
      lview->camvisflag  &= ~(1<<tex);
    break;
  case VAT_FOGSTART:
    if (n == 1) return FunctionPublish_returnFloat(pstate,rback->fogstart);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&rback->fogstart))
      return FunctionPublish_returnError(pstate,"1 l3dview 1 float required");
    break;
  case VAT_FOGEND:
    if (n == 1) return FunctionPublish_returnFloat(pstate,rback->fogend);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&rback->fogend))
      return FunctionPublish_returnError(pstate,"1 l3dview 1 float required");

    break;
  case VAT_FOGDENS:
    if (n == 1) return FunctionPublish_returnFloat(pstate,rback->fogdensity);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&rback->fogdensity))
      return FunctionPublish_returnError(pstate,"1 l3dview 1 float required");
    break;
  case VAT_FOGCOLOR:
    if (n == 1) return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(rback->fogcolor));
    else if (4!=FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(rback->fogcolor)))
      return FunctionPublish_returnError(pstate,"1 l3dview 4 floats required");
    break;
  case VAT_FILLLAYERS:
    if (n == 1) return FunctionPublish_returnBool(pstate,lview->layerfill);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&lview->layerfill))
      return FunctionPublish_returnError(pstate,"1 l3dview 1 boolean required");
    break;
  case VAT_SKYBOX:
    if (n==2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SKYBOX,(void*)&ref) || !Reference_get(ref,rback->skybox))
        rback->skybox = NULL;
    }
    else if (rback->skybox){
      return FunctionPublish_returnType(pstate,LUXI_CLASS_SKYBOX,REF2VOID(rback->skybox->reference));
    }
    break;
  case VAT_CAMERA:
    if (n==2){
      List3DNode_t *l3dnode;

      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_CAMERA,(void*)&ref) || !Reference_get(ref,l3dnode))
        lview->camera = NULL;

      lview->camera = l3dnode->cam;
    }
    else if (lview->camera){
      return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_CAMERA,REF2VOID(lview->camera->l3dnode->reference));
    }
    break;
  case VAT_DELETE:
    Reference_free(ref);//RList3DView_free(ref);
    break;
  case VAT_ACTIVATE:
    state = LUX_FALSE;
    if (FunctionPublish_getArgOffset(pstate,1,3,LUXI_CLASS_L3D_SET,(void*)&tex,LUXI_CLASS_L3D_VIEW,(void*)&ref,LUXI_CLASS_BOOLEAN,(void*)&state)<1)
      return FunctionPublish_returnError(pstate,"1 l3dview 1 l3dset [1 l3dview [1 boolean]] required");
    lview2 = NULL;
    Reference_get(ref,lview2);
    lview2 = lview2 == lview ? NULL : lview2;
    FunctionPublish_returnBool(pstate,List3DView_activate(lview,tex,lview2,state));
    break;
  case VAT_DEACTIVATE:
    List3DView_deactivate(lview);
    break;
  case VAT_FULLWINDOW:
    if (n==2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&lview->viewport.bounds.fullwindow))
        return FunctionPublish_returnError(pstate,"1 l3dview 1 boolean required");
    }
    else
      return FunctionPublish_returnBool(pstate,lview->viewport.bounds.fullwindow);
    break;
  case VAT_VIEWPOS:
    if (n==3){
      if (FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_INT,(void*)&lview->viewport.bounds.x,LUXI_CLASS_INT,(void*)&lview->viewport.bounds.y)!=2)
        return FunctionPublish_returnError(pstate,"1 l3dview 2 int required");
    }
    else
      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,lview->viewport.bounds.x,LUXI_CLASS_INT,lview->viewport.bounds.y);
    break;
  case VAT_VIEWSIZE:
    if (n==3){
      if (FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_INT,(void*)&lview->viewport.bounds.width,LUXI_CLASS_INT,(void*)&lview->viewport.bounds.height)!=2)
        return FunctionPublish_returnError(pstate,"1 l3dview 2 int required");
    }
    else
      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,lview->viewport.bounds.width,LUXI_CLASS_INT,lview->viewport.bounds.height);
    break;
  case VAT_VIEWREFBOUNDS:
    if (n==5){
      if (4!=FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR2(pos),FNPUB_TOVECTOR2(size)))
        return FunctionPublish_returnError(pstate,"1 l3dview 4 float required");
            lview->viewport.bounds.x = (int)(pos[0]/VID_REF_WIDTHSCALE);
      lview->viewport.bounds.y = (int)(pos[1]/VID_REF_HEIGHTSCALE);
      lview->viewport.bounds.width = (int)(size[0]/VID_REF_WIDTHSCALE);
      lview->viewport.bounds.height = (int)(size[1]/VID_REF_HEIGHTSCALE);
      lview->viewport.bounds.y = (int)g_Window.height - (lview->viewport.bounds.y+lview->viewport.bounds.height);
      break;
    }
    else{
      lxVector2Set(pos,(float)lview->viewport.bounds.x*VID_REF_WIDTHSCALE,(float)lview->viewport.bounds.y*VID_REF_HEIGHTSCALE);
      lxVector2Set(size,(float)lview->viewport.bounds.width *VID_REF_WIDTHSCALE,(float)lview->viewport.bounds.height*VID_REF_HEIGHTSCALE);
      // swap y and add size
      pos[1] = VID_REF_HEIGHT - (pos[1]+size[1]);

      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR2(pos),FNPUB_FROMVECTOR2(size));

    }
    break;
  case VAT_CAMAXIS:
    if (n == 1) return FunctionPublish_returnBool(pstate,lview->drawcamaxis);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&lview->drawcamaxis))
      return FunctionPublish_returnError(pstate,"1 l3dview 1 bool required");
    break;
  case VAT_DEPTHONLY:
    if (n== 1) return FunctionPublish_returnBool(pstate,lview->depthonly);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&lview->depthonly))
      return FunctionPublish_returnError(pstate,"1 l3dview 1 bool required");
    break;
  case VAT_DEPTHBOUNDS:
    if (n== 1) return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR2(lview->viewport.depth));
    else if (2!=FunctionPublish_getArgOffset(pstate,1,2,FNPUB_TOVECTOR2(lview->viewport.depth)))
      return FunctionPublish_returnError(pstate,"1 l3dview 2 floats required");
    break;
  case VAT_USEPOLYOFFSET:
    if (n== 1) return FunctionPublish_returnBool(pstate,lview->usepolyoffset);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&lview->usepolyoffset))
      return FunctionPublish_returnError(pstate,"1 l3dview 1 bool required");
    break;
  case VAT_POLYOFFSET:
    if (n==1) return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR2(lview->polyoffset));
    else if (!FunctionPublish_getArgOffset(pstate,1,2,FNPUB_TOVECTOR2(lview->polyoffset)))
      return FunctionPublish_returnError(pstate,"1 l3dview 2 floats required");
    break;
  case VAT_DRAW:
    l3dnode = NULL;
    if (lview->list)
      return FunctionPublish_returnError(pstate,"1 l3dview not activated required");
    if (n==2 && (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LIGHT,(void*)&ref) || !Reference_get(ref,l3dnode)))
      return FunctionPublish_returnError(pstate,"1 l3dview 1 l3dlight required");
    List3DView_drawDirect(lview,l3dnode ? l3dnode->light : NULL);
    break;
  default:
    break;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_L3D_BATCHBUFFER

enum PubL3DBatchFunc_e
{
  PBATCH_NEW,
  PBATCH_NEEDBATCH_FUNCS,
  PBATCH_DELETE,
};

static int PubL3DBatch_func(PState pstate,PubFunction_t *fn, int n)
{
  Reference ref;
  int l3dsetid;
  List3DBatchBuffer_t *bbuffer;
  SceneNode_t *snode;

  if ((int)fn->upvalue > PBATCH_NEEDBATCH_FUNCS)
    if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_BATCHBUFFER,(void*)&ref) || !Reference_get(ref,bbuffer))
      return FunctionPublish_returnError(pstate,"1 l3dbatchbuffer required");

  switch((int)fn->upvalue)
  {
    case PBATCH_NEW:
      if (n<2 || !FunctionPublish_getArg(pstate,2,LUXI_CLASS_L3D_SET,(void*)&l3dsetid,LUXI_CLASS_SCENENODE,(void*)&ref) ||
        !Reference_get(ref,snode))
        return FunctionPublish_returnError(pstate,"1 l3dset 1 scenenode required.");

      bbuffer = List3DBatchBuffer_new(l3dsetid);
      List3DBatchBuffer_init(bbuffer,snode);
      Reference_makeVolatile(bbuffer->reference);
      return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_BATCHBUFFER,REF2VOID(bbuffer->reference));
      break;
    case PBATCH_DELETE:
      Reference_free(ref);//RList3DBatchBuffer_free(ref);
      break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_RCMD

enum PubRCmd_Func_e
{
  // stored in main func
  PRCMD_FLAG,
  PRCMD_FBO_DRAWTO,
  PRCMD_FBO_READFROM,
  PRCMD_FBO_BIND,
  PRCMD_FBO_BIND_TARGET,

  PRCMD_FBO_ASSIGNTEX_TARGET,
  PRCMD_FBO_ASSIGNTEX_COLOR,
  PRCMD_FBO_ASSIGNTEX_DEPTH,
  PRCMD_FBO_ASSIGNTEX_STENCIL,

  PRCMD_FBO_ASSIGNRB_TARGET,
  PRCMD_FBO_ASSIGNRB_COLOR,
  PRCMD_FBO_ASSIGNRB_DEPTH,
  PRCMD_FBO_ASSIGNRB_STENCIL,

  PRCMD_FBO_BLIT_CONTENT,
  PRCMD_FBO_BLIT_LINEAR,
  PRCMD_FBO_BLIT_SRC,
  PRCMD_FBO_BLIT_DST,

  PRCMD_GENMIPMAP,
  PRCMD_DEPTHTEST,
  PRCMD_LOGICOP,

  // own functions
  PRCMD_SHADERS_SHD,
  PRCMD_SHADERS_OVERRIDE,

  PRCMD_IGNORE_LIGHTS,
  PRCMD_IGNORE_PROJECTORS,

  PRCMD_DRAWLAYER_ID,
  PRCMD_DRAWLAYER_SORT,

  PRCMD_DRAWPRT_ID,

  PRCMD_DRAWL2D_L2D,
  PRCMD_DRAWL2D_REFSIZE,

  PRCMD_DRAWL3D_L3D,
  PRCMD_DRAWL3D_SUBMESHES,
  PRCMD_DRAWL3D_FORCE,

  PRCMD_READ_BUFFER,
  PRCMD_READ_RECT,
  PRCMD_READ_WHAT,

  PRCMD_UPDTEX_TEX,
  PRCMD_UPDTEX_BUFFER,
  PRCMD_UPDTEX_SIZE,
  PRCMD_UPDTEX_POS,
  PRCMD_UPDTEX_FORMAT,

  PRCMD_FNCALL_FUNC,
  PRCMD_FNCALL_ORTHO,
  PRCMD_FNCALL_MATRIX,
  PRCMD_FNCALL_SIZE,

  PRCMD_CLEAR_COLOR,
  PRCMD_CLEAR_DEPTH,
  PRCMD_CLEAR_STENCIL,
  PRCMD_CLEAR_COLORVAL,
  PRCMD_CLEAR_DEPTHVAL,
  PRCMD_CLEAR_STENCILVAL,
  PRCMD_CLEAR_MODE,

  PRCMD_COPYTEX_TEX,
  PRCMD_COPYTEX_SIDE,
  PRCMD_COPYTEX_MIP,
  PRCMD_COPYTEX_TEXOFFSET,
  PRCMD_COPYTEX_SCREENBOUNDS,
  PRCMD_COPYTEX_AUTOSIZE,

  PRCMD_DRAWMESH_QUADMESH,
  PRCMD_DRAWMESH_USERMESH,
  PRCMD_DRAWMESH_RENDERMESH,
  PRCMD_DRAWMESH_MATSURFACE,
  PRCMD_DRAWMESH_COLOR,
  PRCMD_DRAWMESH_POS,
  PRCMD_DRAWMESH_SIZE,
  PRCMD_DRAWMESH_AUTOSIZE,
  PRCMD_DRAWMESH_ORTHO,
  PRCMD_DRAWMESH_MATRIX,
  PRCMD_DRAWMESH_LINK,

  PRCMD_RVB_DRAWMESH,
  PRCMD_RVB_BUFFER,
  PRCMD_RVB_ATTRIB,
  PRCMD_RVB_NUMATTRIB,
  PRCMD_RVB_CAPTURE,
  PRCMD_RVB_OUTPUT,
  PRCMD_RVB_NORASTER,
};

static int PubRCmd_new(PState pstate,PubFunction_t *fn, int n){
  Reference ref;

  if (!g_VID.capFBO && (lxClassType)fn->upvalue >= LUXI_CLASS_RCMD_FBO_BIND)
    return FunctionPublish_returnError(pstate,"no capability for framebufferobject!");

  ref = RenderCmd_new((lxClassType)fn->upvalue)->reference;
  Reference_makeVolatile(ref);
  return FunctionPublish_returnType(pstate,(lxClassType)fn->upvalue,REF2VOID(ref));
}

static int PubRCmd_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmd_t *rcmd;
  RenderCmdPtr_t pcmd;
  int ival;
  int offset;
  int mip;
  int tex;
  FBOTarget_t *pTarget;
  FBRenderBuffer_t *rb;
  FBObject_t *fbo;
  int buffer[4];

  int state = LUX_FALSE;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmd required");

  pcmd.cmd = rcmd;

  ival = 0;
  pTarget = NULL;

  switch((int)fn->upvalue){
  case PRCMD_FLAG:
    if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&ival) || ival < 0 || ival > 31)
      return FunctionPublish_returnError(pstate,"1 l3dview 1 (0..31) int required");
    if (n==2) return FunctionPublish_returnBool(pstate,isFlagSet(rcmd->runflag ,1<<ival));
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 l3dview 1 int (0..31) 1 boolean required");
    if (state)
      rcmd->runflag |= 1<<ival;
    else
      rcmd->runflag  &= ~(1<<ival);
    return 0;
  case PRCMD_FBO_BIND:
    state = LUX_TRUE;
    if (rcmd->type != LUXI_CLASS_RCMD_FBO_BIND) return FunctionPublish_returnError(pstate,"1 rcmdfbobind required");
    if (n==1) return FunctionPublish_setRet(pstate,2,LUXI_CLASS_RENDERFBO,REF2VOID(pcmd.flag->fbobind.fbo->reference),LUXI_CLASS_BOOLEAN,pcmd.flag->fbobind.viewportchange);
    else if (n < 2 || 1>FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_RENDERFBO,(void*)&ref,LUXI_CLASS_BOOLEAN,(void*)&state) || !Reference_get(ref,fbo))
      return FunctionPublish_returnError(pstate,"1 fbo [1 boolean] required");

    Reference_releasePtr(pcmd.flag->fbobind.fbo,reference);
    Reference_ref(fbo->reference);

    pcmd.flag->fbobind.fbo = fbo;
    pcmd.flag->fbobind.viewportchange = state;
    return 0;
  case PRCMD_FBO_BIND_TARGET:
    pTarget = &pcmd.flag->fbobind.fbotarget;
  case PRCMD_FBO_ASSIGNRB_TARGET:
    if (!pTarget)
      pTarget = &pcmd.flag->fborb.fbotarget;
  case PRCMD_FBO_ASSIGNTEX_TARGET:
    if (!pTarget)
      pTarget = &pcmd.fbotex->fbotarget;

    if (n == 1) return FunctionPublish_returnBool(pstate,*pTarget == FBO_TARGET_READ);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 fbo 1 boolean required");

    *pTarget = state ? FBO_TARGET_READ : FBO_TARGET_DRAW;
    return 0;
  case PRCMD_FBO_READFROM:
    if (rcmd->type != LUXI_CLASS_RCMD_FBO_READBUFFER) return FunctionPublish_returnError(pstate,"1 rcmdfboreadfrom required");
    if (n==1){
      return FunctionPublish_returnInt(pstate,pcmd.flag->fboread.buffer);
    }
    else{
      int buffer;
      if (!FunctionPublish_getNArg(pstate,1,FNPUB_TINT(buffer)) ||
        buffer<0 || buffer >= FBO_MAX_DRAWBUFFERS)
      {
        return FunctionPublish_returnError(pstate,"1 int 0..3 required");
      }
      pcmd.flag->fboread.buffer = buffer;
      return 0;
    }
    break;
  case PRCMD_FBO_DRAWTO:
    if (rcmd->type != LUXI_CLASS_RCMD_FBO_DRAWBUFFERS) return FunctionPublish_returnError(pstate,"1 rcmdfbodrawto required");
    if (n==1){
      return FunctionPublish_setRet(pstate,pcmd.flag->fbobuffers.numBuffers,
        LUXI_CLASS_INT,(void*)pcmd.flag->fbobuffers.buffers[0],
        LUXI_CLASS_INT,(void*)pcmd.flag->fbobuffers.buffers[1],
        LUXI_CLASS_INT,(void*)pcmd.flag->fbobuffers.buffers[2],
        LUXI_CLASS_INT,(void*)pcmd.flag->fbobuffers.buffers[3]);
    }
    ival = FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4INT(buffer));
    if (!ival)
      return FunctionPublish_returnError(pstate,"no drawbuffer defined");
    if (ival > g_VID.capDrawBuffers || ival > FBO_MAX_DRAWBUFFERS)
      return FunctionPublish_returnError(pstate,"no capability for so many multiple rendertargets");

    for (state = 0; state < ival; state++){
      if (buffer[state] < 0 || buffer[state] >= FBO_MAX_DRAWBUFFERS)
        return FunctionPublish_returnError(pstate,"color draw buffer must be 0..3");
    }
    LUX_ARRAY4COPY(pcmd.flag->fbobuffers.buffers,buffer);
    pcmd.flag->fbobuffers.numBuffers = ival;
    return 0;
  case PRCMD_FBO_ASSIGNTEX_COLOR:
    if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&ival) || ival < 0 || ival >= g_VID.capDrawBuffers)
      return FunctionPublish_returnError(pstate,"1 int 0..drawbuffers-1 required");
    ival++;
  case PRCMD_FBO_ASSIGNTEX_DEPTH:
    if (!ival)
      ival = 5;
  case PRCMD_FBO_ASSIGNTEX_STENCIL:
    if (!ival)
      ival = 6;

    ival--;
    offset = ival < 4 ? 2 : 1;
    if (rcmd->type != LUXI_CLASS_RCMD_FBO_TEXASSIGN) return FunctionPublish_returnError(pstate,"1 rcmdfbotex required");

    state = 0;
    mip = 0;
    if (n==(offset)){
      tex = pcmd.fbotex->fbotexassigns[ival].texRID;
      state = pcmd.fbotex->fbotexassigns[ival].offset;
      mip = pcmd.fbotex->fbotexassigns[ival].mip;
      state = (state < 0 ? -state : state);
      state = (state) ? state-1 : 0;
      return tex < 0 ? 0 : FunctionPublish_setRet(pstate,3,LUXI_CLASS_TEXTURE,(void*)tex,LUXI_CLASS_INT,(void*)state,LUXI_CLASS_INT,(void*)mip);
    }
    else if (n < (offset+1) || !FunctionPublish_getArgOffset(pstate,offset,3,LUXI_CLASS_TEXTURE,(void*)&tex,LUXI_CLASS_INT,(void*)&state,LUXI_CLASS_INT,(void*)&mip)){
      pcmd.fbotex->fbotexassigns[ival].texRID = -1;
      return 0;
    }
    switch(ResData_getTexture(tex)->format)
    {
    case TEX_FORMAT_CUBE:
      state++;
      break;
    case TEX_FORMAT_1D_ARRAY:
    case TEX_FORMAT_2D_ARRAY:
    case TEX_FORMAT_3D:
      state++;
      state = -state;
      break;
    default:
      state = 0;
      break;
    }
    pcmd.fbotex->fbotexassigns[ival].texRID = tex;
    pcmd.fbotex->fbotexassigns[ival].offset = state;
    pcmd.fbotex->fbotexassigns[ival].mip    = mip;
    return 0;
  case PRCMD_FBO_ASSIGNRB_COLOR:
    if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&ival) || ival < 0 || ival >= g_VID.capDrawBuffers)
      return FunctionPublish_returnError(pstate,"1 int 0..drawbuffers-1 required");
    ival++;
  case PRCMD_FBO_ASSIGNRB_DEPTH:
    if (!ival)
      ival = 5;
  case PRCMD_FBO_ASSIGNRB_STENCIL:
    if (!ival)
      ival = 6;

    ival--;
    offset = ival < 4 ? 2 : 1;
    if (rcmd->type != LUXI_CLASS_RCMD_FBO_RBASSIGN) return FunctionPublish_returnError(pstate,"1 rcmdfborb required");

    if (n==(offset)){
      rb = pcmd.flag->fborb.fborbassigns[ival];
      return rb ? FunctionPublish_returnType(pstate,LUXI_CLASS_RENDERBUFFER,REF2VOID(rb->reference)) : 0;
    }
    else if (n < (offset+1) || !FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_RENDERBUFFER,(void*)&ref) || !Reference_get(ref,rb)){

      Reference_releasePtrClear(pcmd.flag->fborb.fborbassigns[ival],reference);
      return 0;
    }
    if (!rb->textype)
      return FunctionPublish_returnError(pstate,"renderbuffer was not setup");


    Reference_ref(ref);
    Reference_releasePtr(pcmd.flag->fborb.fborbassigns[ival],reference);


    pcmd.flag->fborb.fborbassigns[ival] = rb;

    return 0;
  case PRCMD_GENMIPMAP:
    if (rcmd->type != LUXI_CLASS_RCMD_GENMIPMAPS) return FunctionPublish_returnError(pstate,"1 rcmdmipmaps required");
    if (n==1)return pcmd.flag->texRID < 0 ? 0 : FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)pcmd.flag->texRID);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_TEXTURE,(void*)&tex) ||
      ResData_getTexture(tex)->dataformat == TEX_FORMAT_3D && ResData_getTexture(tex)->dataformat == TEX_FORMAT_RECT)
      return FunctionPublish_returnError(pstate,"texture (1d,2d,cube,array) required");
    pcmd.flag->texRID = tex;
    return 0;
  case PRCMD_DEPTHTEST:
    if (rcmd->type != LUXI_CLASS_RCMD_DEPTH) return FunctionPublish_returnError(pstate,"1 rcmddepth required");
    if (n==1)return pcmd.flag->depth.func ? FunctionPublish_returnType(pstate,LUXI_CLASS_COMPAREMODE,(void*)pcmd.flag->depth.func) : FunctionPublish_returnBool(pstate,LUX_FALSE);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_COMPAREMODE,(void*)&pcmd.flag->depth.func))
      pcmd.flag->depth.func = 0;
    return 0;

  case PRCMD_LOGICOP:
    if (rcmd->type != LUXI_CLASS_RCMD_LOGICOP) return FunctionPublish_returnError(pstate,"1 rcmdlogicop required");
    if (n==1)return FunctionPublish_returnType(pstate,LUXI_CLASS_LOGICMODE,(void*)pcmd.flag->logic.op);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_LOGICMODE,(void*)&pcmd.flag->logic.op))
      return FunctionPublish_returnError(pstate,"1 logicmode required");
    return 0;
  case PRCMD_FBO_BLIT_CONTENT:
    if (rcmd->type != LUXI_CLASS_RCMD_BUFFER_BLIT) return FunctionPublish_returnError(pstate,"1 rcmdbufferblit required");
    if (n==1) return FunctionPublish_setRet(pstate,3,LUXI_CLASS_BOOLEAN,(void*)pcmd.fboblit->copycolor,LUXI_CLASS_BOOLEAN,(void*)pcmd.fboblit->copydepth,LUXI_CLASS_BOOLEAN,(void*)pcmd.fboblit->copystencil);
    else if (n < 4 || 3>FunctionPublish_getArgOffset(pstate,1,3,LUXI_CLASS_BOOLEAN,(void*)&pcmd.fboblit->copycolor,LUXI_CLASS_BOOLEAN,(void*)&pcmd.fboblit->copydepth,LUXI_CLASS_BOOLEAN,(void*)&pcmd.fboblit->copystencil))
      return FunctionPublish_returnError(pstate,"3 boolean required");
    return 0;
  case PRCMD_FBO_BLIT_LINEAR:
    if (rcmd->type != LUXI_CLASS_RCMD_BUFFER_BLIT) return FunctionPublish_returnError(pstate,"1 rcmdbufferblit required");
    if (n==1) return FunctionPublish_returnBool(pstate,pcmd.fboblit->linear);
    else if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&pcmd.fboblit->linear))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    return 0;
  case PRCMD_FBO_BLIT_SRC:
    if (rcmd->type != LUXI_CLASS_RCMD_BUFFER_BLIT) return FunctionPublish_returnError(pstate,"1 rcmdbufferblit required");
    if (n==1) return FunctionPublish_setRet(pstate,4,LUXI_CLASS_INT,(void*)pcmd.fboblit->srcX,LUXI_CLASS_INT,(void*)pcmd.fboblit->srcY,LUXI_CLASS_INT,(void*)pcmd.fboblit->srcWidth,LUXI_CLASS_INT,(void*)pcmd.fboblit->srcHeight);
    else if (n < 5 || 4> FunctionPublish_getArgOffset(pstate,1,4,LUXI_CLASS_INT,(void*)&pcmd.fboblit->srcX,LUXI_CLASS_INT,(void*)&pcmd.fboblit->srcY,LUXI_CLASS_INT,(void*)&pcmd.fboblit->srcWidth,LUXI_CLASS_INT,(void*)&pcmd.fboblit->srcHeight))
      return FunctionPublish_returnError(pstate,"4 int required");
    return 0;
  case PRCMD_FBO_BLIT_DST:
    if (rcmd->type != LUXI_CLASS_RCMD_BUFFER_BLIT) return FunctionPublish_returnError(pstate,"1 rcmdbufferblit required");
    if (n==1) return FunctionPublish_setRet(pstate,4,LUXI_CLASS_INT,(void*)pcmd.fboblit->dstX,LUXI_CLASS_INT,(void*)pcmd.fboblit->dstY,LUXI_CLASS_INT,(void*)pcmd.fboblit->dstWidth,LUXI_CLASS_INT,(void*)pcmd.fboblit->dstHeight);
    else if (n < 5 || 4> FunctionPublish_getArgOffset(pstate,1,4,LUXI_CLASS_INT,(void*)&pcmd.fboblit->dstX,LUXI_CLASS_INT,(void*)&pcmd.fboblit->dstY,LUXI_CLASS_INT,(void*)&pcmd.fboblit->dstWidth,LUXI_CLASS_INT,(void*)&pcmd.fboblit->dstHeight))
      return FunctionPublish_returnError(pstate,"4 int required");
    return 0;
  default:
    return 0;
  }
}

static int PubRCmdReadPixels_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdRead_t *rcmd;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_READPIXELS,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmdreadpixels required");

  switch((int)fn->upvalue){
  case PRCMD_READ_BUFFER:
    {
      lxRvidbuffer vref;
      HWBufferObject_t* hwbuf;
      int offset = 0;

      if (n==1){
        if (rcmd->bufferref){
          return FunctionPublish_returnType(pstate,LUXI_CLASS_VIDBUFFER,REF2VOID(rcmd->bufferref));
        }
        else{
          return 0;
        }
      }
      else if (!FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_VIDBUFFER,(void*)&vref,FNPUB_TINT(offset)) || !Reference_get(vref,hwbuf)){
        return FunctionPublish_returnError(pstate,"1 vidbuffer [1 int] required");
      }
      Reference_ref(vref);
      Reference_releaseCheck(rcmd->bufferref);
      rcmd->bufferref = vref;
      rcmd->buffer = &hwbuf->buffer;
      rcmd->offset = offset;
    }

    return 0;
  case PRCMD_READ_RECT:
    {
      int rect[4];

      if (n == 1){
        return FunctionPublish_setRet(pstate,4, FNPUB_FROMVECTOR4INT(rcmd->rect));
      }
      else if (4>FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4INT(rect))){
        return FunctionPublish_returnError(pstate,"4 int required");
      }
      LUX_ARRAY4COPY(rcmd->rect,rect);
      return 0;
    }
  case PRCMD_READ_WHAT:
    {
      TextureType_t type;
      TextureDataType_t tdata;

      if (2>FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_TEXTYPE,(void*)&type,
        LUXI_CLASS_TEXDATATYPE,(void*)&tdata)){
          return FunctionPublish_returnError(pstate,"1 textype 1 texdatatype required.");
      }

      rcmd->format = TextureFormatGL_alterData(TextureType_toInternalGL(type),tdata);
      rcmd->dataformat = ScalarType_toGL(TextureDataType_toScalar(tdata));
      return 0;
    }
  }

  return 0;
}

static int PubRCmdUpdateTex_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdTexupd_t *rcmd;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_UPDATETEX,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmdupdatetex required");

  switch((int)fn->upvalue){
  case PRCMD_UPDTEX_TEX:
    if (n==1) return rcmd->texRID < 0 ? 0 : FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)rcmd->texRID);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_TEXTURE,(void*)&rcmd->texRID))
      return FunctionPublish_returnError(pstate,"1 texture required");
    return 0;
  case PRCMD_UPDTEX_BUFFER:
    {
      lxRvidbuffer vref;
      HWBufferObject_t* hwbuf;
      int offset = 0;

      if (n==1){
        if (rcmd->bufferref){
          return FunctionPublish_returnType(pstate,LUXI_CLASS_VIDBUFFER,REF2VOID(rcmd->bufferref));
        }
        else{
          return 0;
        }
      }
      else if (!FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_VIDBUFFER,(void*)&vref,FNPUB_TINT(offset)) || !Reference_get(vref,hwbuf)){
        return FunctionPublish_returnError(pstate,"1 vidbuffer [1 int] required");
      }
      Reference_ref(vref);
      Reference_releaseCheck(rcmd->bufferref);
      rcmd->bufferref = vref;
      rcmd->buffer = &hwbuf->buffer;
      rcmd->offset = offset;
    }

    return 0;
  case PRCMD_UPDTEX_SIZE:
    {
      int size[3];

      if (n == 1){
        return FunctionPublish_setRet(pstate,3, FNPUB_FROMVECTOR3INT(rcmd->size));
      }
      else if (3>FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3INT(size))){
        return FunctionPublish_returnError(pstate,"3 int required");
      }
      LUX_ARRAY3COPY(rcmd->size,size);
      return 0;
    }
  case PRCMD_UPDTEX_POS:
    {
      int pos[4];
      pos[3] = rcmd->pos[3];

      if (n == 1){
        return FunctionPublish_setRet(pstate,4, FNPUB_FROMVECTOR4INT(rcmd->pos));
      }
      else if (3>FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4INT(pos))){
        return FunctionPublish_returnError(pstate,"3 int [1 int] required");
      }
      LUX_ARRAY4COPY(rcmd->pos,pos);
      return 0;
    }
  case PRCMD_UPDTEX_FORMAT:
    {
      TextureDataType_t tdata;

      if (1>FunctionPublish_getArgOffset(pstate,1,1,LUXI_CLASS_TEXDATATYPE,(void*)&tdata)){
          return FunctionPublish_returnError(pstate,"1 texdatatype required.");
      }

      rcmd->dataformat = ScalarType_toGL(TextureDataType_toScalar(tdata));
      return 0;
    }
  }

  return 0;
}


static int PubRCmdRVB_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdR2VB_t *rcmd;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_R2VB,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmdreadpixels required");

  switch((int)fn->upvalue){
  case PRCMD_RVB_BUFFER:
    {
      lxRvidbuffer vref;
      HWBufferObject_t* hwbuf;
      int offset = 0;
      int size = 0;

      if (n==1){
        if (rcmd->buffers[0].vbuf){
          return FunctionPublish_returnType(pstate,LUXI_CLASS_VIDBUFFER,rcmd->buffers[0].vbuf->host);
        }
        else{
          return 0;
        }
      }
      else if (!FunctionPublish_getArgOffset(pstate,1,3,LUXI_CLASS_VIDBUFFER,(void*)&vref,FNPUB_TINT(offset),FNPUB_TINT(size)) || !Reference_get(vref,hwbuf) ||
        offset + size >= hwbuf->buffer.size ){
        return FunctionPublish_returnError(pstate,"1 vidbuffer [2 int within size range] required");
      }
      Reference_ref(vref);
      Reference_releasePtr(rcmd->buffers[0].vbuf,host);
      rcmd->buffers[0].vbuf = &hwbuf->buffer;
      rcmd->buffers[0].offset = offset;
      rcmd->buffers[0].size = size;
      rcmd->numBuffers = 1;
    }

    return 0;
  case PRCMD_RVB_DRAWMESH:
    {
      RenderCmdMesh_t *rmesh;

      if (n == 1){
        if (!rcmd->inputmesh) return 0;
        return FunctionPublish_returnType(pstate,LUXI_CLASS_RCMD_DRAWMESH,REF2VOID(rcmd->inputmesh));
      }
      else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_RCMD_DRAWMESH,(void*)&ref)
        || !Reference_get(ref,rmesh)){
        return FunctionPublish_returnError(pstate,"1 rcmddrawmesh required");
      }
      Reference_ref(ref);
      Reference_releaseCheck(rcmd->inputmesh);
      rcmd->inputmesh = ref;
      rcmd->rcmdmesh = rmesh;
      return 0;
    }
  case PRCMD_RVB_ATTRIB:
    {
      const char *name;
      int index = 0;
      int comp = 4;

      if (2> FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TINT(index),FNPUB_TSTR(name),FNPUB_TINT(comp))
        || index<0 || index >= R2VB_MAX_ATTRIBS || !RenderCmdR2VB_setAttrib(rcmd,index,name,comp)
        )
      {
        return FunctionPublish_returnError(pstate,"1 int (0..16) 1 valid string [1 int (1..4)] required");
      }

      return 0;
    }
  case PRCMD_RVB_NUMATTRIB:
    {
      int num;
      if (n == 1) return FunctionPublish_returnInt(pstate,rcmd->numAttribs);
      else if (!FunctionPublish_getNArg(pstate,1,FNPUB_TINT(num)) ||
        num < 0 || num >= R2VB_MAX_ATTRIBS)
      {
        return FunctionPublish_returnError(pstate,"1 boolean required");
      }
      rcmd->numAttribs = num;
      return 0;
    }
  case PRCMD_RVB_OUTPUT:
    {
      return FunctionPublish_returnInt(pstate,rcmd->output);
    }
  case PRCMD_RVB_CAPTURE:
    {
      if (n == 1) return FunctionPublish_returnBool(pstate,rcmd->capture);
      else if (!FunctionPublish_getNArg(pstate,1,FNPUB_TBOOL(rcmd->capture))){
        return FunctionPublish_returnError(pstate,"1 boolean required");
      }
      return 0;
    }
  case PRCMD_RVB_NORASTER:
    {
      if (n == 1) return FunctionPublish_returnBool(pstate,rcmd->noraster);
      else if (!FunctionPublish_getNArg(pstate,1,FNPUB_TBOOL(rcmd->noraster))){
        return FunctionPublish_returnError(pstate,"1 boolean required");
      }
      return 0;
    }
  }

  return 0;
}


static int PubRCmdShaders_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdFlag_t *rcmd;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_BASESHADERS,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmdshaders required");

  switch((int)fn->upvalue){
  case PRCMD_SHADERS_SHD:
    if (n == 1) return FunctionPublish_setRet(pstate,4,LUXI_CLASS_SHADER,(void*)rcmd->shd.shaders.ids[VID_SHADER_COLOR],
      LUXI_CLASS_SHADER,(void*)rcmd->shd.shaders.ids[VID_SHADER_COLOR_LM],
      LUXI_CLASS_SHADER,(void*)rcmd->shd.shaders.ids[VID_SHADER_TEX],
      LUXI_CLASS_SHADER,(void*)rcmd->shd.shaders.ids[VID_SHADER_TEX_LM]);
    else if (4!=FunctionPublish_getArgOffset(pstate,1,4,LUXI_CLASS_SHADER,(void*)&rcmd->shd.shaders.ids[VID_SHADER_COLOR],\
      LUXI_CLASS_SHADER,(void*)&rcmd->shd.shaders.ids[VID_SHADER_COLOR_LM],\
      LUXI_CLASS_SHADER,(void*)&rcmd->shd.shaders.ids[VID_SHADER_TEX],\
      LUXI_CLASS_SHADER,(void*)&rcmd->shd.shaders.ids[VID_SHADER_TEX_LM])){
        return FunctionPublish_returnError(pstate,"1 rcmdshaders 4 shader required");
      }
    return 0;
  case PRCMD_SHADERS_OVERRIDE:
    if (n == 1) return FunctionPublish_returnInt(pstate,rcmd->shd.shaders.override);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&rcmd->shd.shaders.override) || rcmd->shd.shaders.override<0 || rcmd->shd.shaders.override > 3){
      rcmd->shd.shaders.override = 0;
      return FunctionPublish_returnError(pstate,"1 rcmdshaders 1 int 0-3 required");
    }
    return 0;
  default:
    return 0;
  }
}

static int PubRCmdIgnore_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdFlag_t *rcmd;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_IGNOREEXTENDED,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmdignore required");

  switch((int)fn->upvalue){
  case PRCMD_IGNORE_LIGHTS:
    if (n == 1) return FunctionPublish_returnBool(pstate,rcmd->ignore.lights);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&rcmd->ignore.lights))
      return FunctionPublish_returnError(pstate,"1 rcmdignore 1 bool required");
    return 0;
  case PRCMD_IGNORE_PROJECTORS:
    if (n == 1) return FunctionPublish_returnBool(pstate,rcmd->ignore.projs);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&rcmd->ignore.projs))
      return FunctionPublish_returnError(pstate,"1 rcmdignore 1 bool required");
    return 0;
  default:
    return 0;
  }
}

static int PubRCmdDrawLayer_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdDraw_t *rcmd;
  int layer;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_DRAWLAYER,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmddrawlayer required");

  switch((int)fn->upvalue){
  case PRCMD_DRAWLAYER_ID:
    if (n==1)return FunctionPublish_returnInt(pstate,rcmd->normal.layer);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&layer) || layer < 0 || layer > 15)
      return FunctionPublish_returnError(pstate,"1 int 0-15 required");
    rcmd->normal.layer = layer;
    return 0;
  case PRCMD_DRAWLAYER_SORT:
    if (n==1)return FunctionPublish_returnInt(pstate,rcmd->normal.sort);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&layer) || layer < -2 || layer > 1)
      return FunctionPublish_returnError(pstate,"1 int -2 - 1 required");
    rcmd->normal.sort = layer;
    return 0;
  default:
    return 0;
  }
}

static int PubRCmdDrawPrt_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdDraw_t *rcmd;
  int layer;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_DRAWPARTICLES,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmddrawprt required");

  switch((int)fn->upvalue){
  case PRCMD_DRAWPRT_ID:
    if (n==1)return FunctionPublish_returnInt(pstate,rcmd->normal.layer);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&layer) || layer < 0 || layer > 15)
      return FunctionPublish_returnError(pstate,"1 int 0-15 required");
    rcmd->normal.layer = layer;
    return 0;
  default:
    return 0;
  }
}

static int PubRCmdDrawL2D_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdDrawL2D_t *rcmd;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_DRAWL2D,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmddrawl2d required");

  switch((int)fn->upvalue){
  case PRCMD_DRAWL2D_L2D:
    if (n == 1) return rcmd->node && Reference_get(rcmd->targetref,rcmd->node) ? PubL2D_return(pstate,rcmd->node) : 0;

    Reference_releaseCheck(rcmd->targetref);

    if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L2D_NODE,(void*)&rcmd->targetref) && Reference_get(rcmd->targetref,rcmd->node)){
      List2DNode_link(rcmd->node,NULL);

      Reference_ref(rcmd->targetref);
    }
    else{
      rcmd->node = NULL;
    }

    return 0;
  case PRCMD_DRAWL2D_REFSIZE:
    if (n == 1){
      return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR2(rcmd->refsize));
    }
    else if (2>FunctionPublish_getArgOffset(pstate,1,2,FNPUB_TOVECTOR2(rcmd->refsize)))
      return FunctionPublish_returnError(pstate,"2 floats required");
    return 0;
  default:
    return 0;
  }
}

static int PubRCmdDrawL3D_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdDrawL3D_t *rcmd;
  int cnt;
  uint mid;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_DRAWL3D,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmddrawl2d required");

  switch((int)fn->upvalue){
  case PRCMD_DRAWL3D_L3D:
    if (n == 1) return rcmd->node && Reference_get(rcmd->targetref,rcmd->node) ? PubL3D_return(pstate,rcmd->node) : 0;


    Reference_releaseCheck(rcmd->targetref);

    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_NODE,(void*)&rcmd->targetref) || !Reference_get(rcmd->targetref,rcmd->node) || Reference_type(rcmd->targetref) == LUXI_CLASS_L3D_LEVELMODEL){
      rcmd->node = NULL;
      return FunctionPublish_returnError(pstate,"1 l3dnode required (l3dlevelmodel not allowed)");
    }
    rcmd->startid  = 0;
    if (Reference_type(rcmd->targetref) == LUXI_CLASS_L3D_MODEL){
      rcmd->numNodes = rcmd->node->numDrawNodes;
    }
    else{
      rcmd->numNodes = 1;
    }
    Reference_ref(rcmd->targetref);

    return 0;
  case PRCMD_DRAWL3D_SUBMESHES:
    if (!rcmd->node || !Reference_get(rcmd->targetref,rcmd->node) ||
      Reference_type(rcmd->targetref) != LUXI_CLASS_L3D_MODEL)
      return 0;

    cnt = 1;
    if (n == 1){
      mid = rcmd->startid;
      SUBRESID_MAKE(mid,rcmd->node->checkRID);

      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_MESHID,(void*)mid,LUXI_CLASS_INT,(void*)rcmd->numNodes);
    }
    else if (!FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_MESHID,(void*)&mid,LUXI_CLASS_INT,(void*)&cnt)
          || SUBRESID_GETRES(mid) != rcmd->node->checkRID)
      return FunctionPublish_returnError(pstate,"1 proper meshid [1 int] required");

    rcmd->numNodes = cnt;
    rcmd->startid = SUBRESID_GETSUB(mid);

    rcmd->startid %= rcmd->node->numDrawNodes;
    rcmd->numNodes = LUX_MIN(rcmd->node->numDrawNodes-rcmd->startid,rcmd->numNodes);

    return 0;
  case PRCMD_DRAWL3D_FORCE:
    if (n == 1) return FunctionPublish_returnBool(pstate,rcmd->force);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&rcmd->force))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    return 0;
  default:
    return 0;
  }
}

static int PubRCmdFnCall_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdFnCall_t *rcmd;
  float* matrix;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_FNCALL,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmdfncall required");

  switch((int)fn->upvalue){
  case PRCMD_FNCALL_FUNC:
    if (n==1) return FunctionPublish_setRet(pstate,2,LUXI_CLASS_POINTER,(void*)rcmd->fncall,LUXI_CLASS_POINTER,(void*)rcmd->upvalue);
    else if (2>FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_POINTER,(void*)&rcmd->fncall,LUXI_CLASS_POINTER,(void*)&rcmd->upvalue))
      return FunctionPublish_returnError(pstate,"2 pointer required required");
    return 0;
  case PRCMD_FNCALL_ORTHO:
    if (n==1) return FunctionPublish_returnBool(pstate,rcmd->ortho);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&rcmd->ortho))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    return 0;
  case PRCMD_FNCALL_MATRIX:
    if (n == 1) return PubMatrix4x4_return(pstate,rcmd->matrix);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) ||
      !Reference_get(ref,matrix))
    {
      return FunctionPublish_returnError(pstate,"1 matrix4x4 required");
    }
    lxMatrix44CopySIMD(rcmd->matrix,matrix);
    return 0;
  case PRCMD_FNCALL_SIZE:
    if (n == 1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(rcmd->size));
    else if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(rcmd->size)))
      return FunctionPublish_returnError(pstate,"3 floats required");
    return 0;

  default:
    return 0;
  }
}

static int PubRCmdClear_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdClear_t *rcmd;
  float pos;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_CLEAR,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmdclear required");

  switch((int)fn->upvalue){
  case PRCMD_CLEAR_COLORVAL:
    if (rcmd->clearmode == 0){
      if (n == 1) return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(rcmd->color));
      else if (4!=FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(rcmd->color)))
        return FunctionPublish_returnError(pstate,"1 rcmdclear 4 floats required");
      return 0;
    }
    else{
      if (n == 1) return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4INT(rcmd->colori));
      else if (4!=FunctionPublish_getArgOffset(pstate,1,4,FNPUB_FROMVECTOR4INT(rcmd->colori)))
        return FunctionPublish_returnError(pstate,"1 rcmdclear 4 int required");
      return 0;
    }
  case PRCMD_CLEAR_STENCILVAL:
    if (n==1) return FunctionPublish_returnInt(pstate,rcmd->stencil);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&rcmd->stencil))
      return FunctionPublish_returnError(pstate,"1 rcmdclear 1 int required");
    return 0;
  case PRCMD_CLEAR_DEPTHVAL:
    if (n==1) return FunctionPublish_returnFloat(pstate,rcmd->depth);
    else if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(pos)) || pos < 0.0f || pos > 1.0f)
      return FunctionPublish_returnError(pstate,"1 rcmdclear 1 float (0-1) required");
    rcmd->depth = pos;
    return 0;
  case PRCMD_CLEAR_STENCIL:
    if (n == 1) return FunctionPublish_returnBool(pstate,rcmd->clearstencil);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&rcmd->clearstencil))
      return FunctionPublish_returnError(pstate,"1 rcmdclear 1 bool required");
    return 0;
  case PRCMD_CLEAR_COLOR:
    if (n == 1) return FunctionPublish_returnBool(pstate,rcmd->clearcolor);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&rcmd->clearcolor))
      return FunctionPublish_returnError(pstate,"1 rcmdclear 1 bool required");
    return 0;
  case PRCMD_CLEAR_DEPTH:
    if (n == 1) return FunctionPublish_returnBool(pstate,rcmd->cleardepth);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&rcmd->cleardepth))
      return FunctionPublish_returnError(pstate,"1 rcmdclear 1 bool required");
    return 0;
  case PRCMD_CLEAR_MODE:
    if (n == 1) return FunctionPublish_returnBool(pstate,rcmd->clearmode);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&rcmd->clearmode))
      return FunctionPublish_returnError(pstate,"1 rcmdclear 1 int required");
    return 0;
  default:
    return 0;
  }
}


static int PubRCmdCopyTex_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdTexcopy_t *rcmd;
  int tex;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_COPYTEX,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmdcopytex required");

  switch((int)fn->upvalue){
  case PRCMD_COPYTEX_TEX:
    if (n==1) return rcmd->texRID < 0 ? 0 : FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)rcmd->texRID);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_TEXTURE,(void*)&rcmd->texRID))
      return FunctionPublish_returnError(pstate,"1 texture required");
    return 0;
  case PRCMD_COPYTEX_SIDE:
    if (n==1) return FunctionPublish_returnInt(pstate,rcmd->texside);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&tex) || tex < 0)
      return FunctionPublish_returnError(pstate,"1 int 0+ required");
    rcmd->texside = tex;
    return 0;
  case PRCMD_COPYTEX_MIP:
    if (n==1) return FunctionPublish_returnInt(pstate,rcmd->mip);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&tex) || tex < 0)
      return FunctionPublish_returnError(pstate,"1 int 0+ required");
    rcmd->mip = tex;
    return 0;
  case PRCMD_COPYTEX_TEXOFFSET:
    if (n==1) return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)rcmd->texX,LUXI_CLASS_INT,(void*)rcmd->texY);
    else if (FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_INT,(void*)&rcmd->texX,LUXI_CLASS_INT,(void*)&rcmd->texX)<2)
      return FunctionPublish_returnError(pstate,"2 int required");
    return 0;
  case PRCMD_COPYTEX_SCREENBOUNDS:
    if (n==1) return FunctionPublish_setRet(pstate,4,LUXI_CLASS_INT,(void*)rcmd->screenX,LUXI_CLASS_INT,(void*)rcmd->screenY,LUXI_CLASS_INT,(void*)rcmd->screenWidth,LUXI_CLASS_INT,(void*)rcmd->screenHeight);
    else if (FunctionPublish_getArgOffset(pstate,1,4,LUXI_CLASS_INT,(void*)&rcmd->screenX,LUXI_CLASS_INT,(void*)&rcmd->screenY,LUXI_CLASS_INT,(void*)&rcmd->screenWidth,LUXI_CLASS_INT,(void*)&rcmd->screenHeight)<4)
      return FunctionPublish_returnError(pstate,"4 int required");
    return 0;
  case PRCMD_COPYTEX_AUTOSIZE:
    if (n==1) return FunctionPublish_returnInt(pstate,rcmd->autosize);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&tex) || tex < -1 || tex > 1)
      return FunctionPublish_returnError(pstate,"1 int -1..1 required");
    rcmd->autosize = tex;
    return 0;
  default:
    return 0;
  }
}

static int PubRCmdDrawMesh_func(PState pstate,PubFunction_t *fn, int n){
  Reference ref;
  RenderCmdMesh_t *rcmd;
  int verts;
  float* matrix;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RCMD_DRAWMESH,(void*)&ref) || !Reference_get(ref,rcmd))
    return FunctionPublish_returnError(pstate,"1 rcmddrawmesh required");

  switch((int)fn->upvalue){
  case PRCMD_DRAWMESH_QUADMESH:
    Reference_releaseClear(rcmd->usermesh);
    rcmd->dnode.mesh = g_VID.drw_meshquadffx;
    return 0;
  case PRCMD_DRAWMESH_USERMESH:
    return PubUserMesh(pstate,1,&rcmd->dnode.mesh,
      &rcmd->usermesh);
  case PRCMD_DRAWMESH_RENDERMESH:
    return PubRenderMesh(pstate,1,&rcmd->dnode.mesh,
      &rcmd->usermesh);
  case PRCMD_DRAWMESH_MATSURFACE:
    if (n==1){
      verts = rcmd->dnode.matRID;
      if (vidMaterial(verts))
        return FunctionPublish_returnType(pstate,LUXI_CLASS_MATERIAL,(void*)verts);
      else if (verts >= 0)
        return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)verts);
      else
        return 0;
    }
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATSURF,(void*)&rcmd->dnode.matRID))
      return FunctionPublish_returnError(pstate,"1 matsurface required");
    if (rcmd->dnode.matobj)
      MaterialObject_free(rcmd->dnode.matobj);

    if (vidMaterial(rcmd->dnode.matRID)){
      Shader_t *shader;
      rcmd->dnode.state.renderflag = Material_getDefaults(ResData_getMaterial(rcmd->dnode.matRID),&shader,&rcmd->dnode.state.alpha,&rcmd->dnode.state.blend,&rcmd->dnode.state.line);
    }

    if (rcmd->ortho){
      rcmd->dnode.state.renderflag |= RENDER_NODEPTHMASK | RENDER_NODEPTHTEST | RENDER_NOVERTEXCOLOR;
    }

    rcmd->dnode.matobj = NULL;
    return 0;
  case PRCMD_DRAWMESH_COLOR:
    if (n == 1) return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(rcmd->dnode.color));
    else if (4!=FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(rcmd->dnode.color)))
      return FunctionPublish_returnError(pstate,"4 floats required");
    return 0;
  case PRCMD_DRAWMESH_POS:
    if (n == 1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(&rcmd->matrix[12]));
    else if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(&rcmd->matrix[12])))
      return FunctionPublish_returnError(pstate,"3 floats required");
    return 0;
  case PRCMD_DRAWMESH_MATRIX:
    if (n == 1) return PubMatrix4x4_return(pstate,rcmd->matrix);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) ||
      !Reference_get(ref,matrix))
    {
      return FunctionPublish_returnError(pstate,"1 matrix4x4 required");
    }
    lxMatrix44CopySIMD(rcmd->matrix,matrix);
    return 0;
  case PRCMD_DRAWMESH_SIZE:
    if (n == 1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(rcmd->size));
    else if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(rcmd->size)))
      return FunctionPublish_returnError(pstate,"3 floats required");
    return 0;
  case PRCMD_DRAWMESH_ORTHO:
    if (n == 1) return FunctionPublish_returnBool(pstate,rcmd->ortho);
    else if (!FunctionPublish_getNArg(pstate,1,FNPUB_TBOOL(rcmd->ortho)))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    if (rcmd->ortho){
      rcmd->dnode.state.renderflag |= RENDER_NODEPTHMASK | RENDER_NODEPTHTEST | RENDER_NOVERTEXCOLOR;
    }
    else{
      rcmd->dnode.state.renderflag &= ~(RENDER_NODEPTHMASK | RENDER_NODEPTHTEST | RENDER_NOVERTEXCOLOR);
    }
    return 0;
  case PRCMD_DRAWMESH_AUTOSIZE:
    if (n==1) return FunctionPublish_returnInt(pstate,rcmd->autosize);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&verts) || verts < -1 || verts > 1)
      return FunctionPublish_returnError(pstate,"1 int -1..1 required");
    rcmd->autosize = verts;
    return 0;
  case PRCMD_DRAWMESH_LINK:
    ref = NULL;
    if (n==1) return PubLinkObject_return(pstate,rcmd->linkref);
    else if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SPATIALNODE,(void*)&ref) &&
          Reference_isValid(ref))
    {
      Reference_refWeak(ref);
    }
    Reference_releaseWeakCheck(rcmd->linkref);
    rcmd->linkref = ref;

  default:
    return 0;
  }
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_RENDERBUFFER

static int PubRenderBuffer_func(PState pstate,PubFunction_t *f,int n)
{
  int width,height,type,windowsized,multisamples;
  Reference ref;
  FBRenderBuffer_t *rb;

  windowsized = LUX_FALSE;
  multisamples = 0;

  if (!f->upvalue){
    if (n > 0 && 3>FunctionPublish_getArg(pstate,5,LUXI_CLASS_TEXTYPE,(void*)&type,LUXI_CLASS_INT,(void*)&width,LUXI_CLASS_INT,(void*)&height,LUXI_CLASS_BOOLEAN,(void*)&windowsized,LUXI_CLASS_INT,(void*)&multisamples))
      return FunctionPublish_returnError(pstate,"1 textype 2 int [1 boolean] [1 int] required");

    rb = FBRenderBuffer_new();

    if (n > 0)
      if (!FBRenderBuffer_set(rb,type,width,height,windowsized,multisamples)){
        Reference_release(rb->reference);
        return 0;
      }

    Reference_makeVolatile(rb->reference);
    return FunctionPublish_returnType(pstate,LUXI_CLASS_RENDERBUFFER,REF2VOID(rb->reference));
  }
  else if (n < 4 || 4>FunctionPublish_getArg(pstate,6,LUXI_CLASS_RENDERBUFFER,(void*)&ref,LUXI_CLASS_TEXTYPE,(void*)&type,LUXI_CLASS_INT,(void*)&width,LUXI_CLASS_INT,(void*)&height,LUXI_CLASS_INT,(void*)&windowsized,LUXI_CLASS_INT,(void*)&multisamples)
    || !Reference_get(ref,rb))
    return FunctionPublish_returnError(pstate,"1 renderbuffer 1 textype 2 int [1 boolean] [1 int] required");

  if (FBRenderBuffer_set(rb,type,width,height,windowsized,multisamples))
    return FunctionPublish_returnType(pstate,LUXI_CLASS_RENDERBUFFER,REF2VOID(rb->reference));
  else
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_RENDERFBO

enum PubFBOCmd_e{
  PFBO_NEW,
  PFBO_SETUP,
  PFBO_RESET,
  PFBO_APPLY,
  PFBO_CHECK,
};

static int PubFrameBuffer_func(PState pstate,PubFunction_t *f,int n)
{

  Reference ref;
  FBObject_t *fbo;
  int width,height,winsized;

  winsized = LUX_FALSE;

  if ((int)f->upvalue != PFBO_NEW &&
    (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RENDERFBO,(void*)&ref) || !Reference_get(ref,fbo)))
      return FunctionPublish_returnError(pstate,"1 fbo required");

  switch((int)f->upvalue){
    case PFBO_NEW:
      if (2>FunctionPublish_getArg(pstate,3,LUXI_CLASS_INT,(void*)&width,LUXI_CLASS_INT,(void*)&height,LUXI_CLASS_BOOLEAN,(void*)&winsized))
        return FunctionPublish_returnError(pstate,"2 int [1 boolean] required");

      fbo = FBObject_new();
      FBObject_setSize(fbo,width,height,winsized);

      Reference_makeVolatile(fbo->reference);
      return FunctionPublish_returnType(pstate,LUXI_CLASS_RENDERFBO,REF2VOID(fbo->reference));
    case PFBO_SETUP:
      if (n == 1) return FunctionPublish_setRet(pstate,3,LUXI_CLASS_INT,(void*)fbo->width,LUXI_CLASS_INT,(void*)fbo->height,LUXI_CLASS_BOOLEAN,(void*)fbo->winsized);
      else if (n < 3 || 2>FunctionPublish_getArgOffset(pstate,1,3,LUXI_CLASS_INT,(void*)&width,LUXI_CLASS_INT,(void*)&height,LUXI_CLASS_BOOLEAN,(void*)&winsized))
        return FunctionPublish_returnError(pstate,"2 int [1 boolean] required");

      FBObject_setSize(fbo,width,height,winsized);

      return 0;
    case PFBO_RESET:
      FBObject_reset(fbo);
      return 0;
    case PFBO_APPLY:
      {
        void* ptr;
        RenderCmdPtr_t tex = {NULL};
        RenderCmdPtr_t rb = {NULL};
        if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_RCMD_FBO_TEXASSIGN,(void*)&ref) &&
          Reference_get(ref,ptr)){
          tex.cmd = ptr;
        }
        if (FunctionPublish_getNArg(pstate,2,LUXI_CLASS_RCMD_FBO_RBASSIGN,(void*)&ref) &&
          Reference_get(ref,ptr)){
          rb.cmd = ptr;
        }
        FBObject_applyAssigns(fbo,tex,rb);
      }
      return 0;
    case PFBO_CHECK:
      return FunctionPublish_returnString(pstate,FBObject_check(fbo));
    default:
      return 0;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
void PubClass_L3DSet_init()
{

  FunctionPublish_initClass(LUXI_CLASS_L3D_SET,"l3dset",
    "The List3D is organised in l3dsets, every set contains multiple layers. You can render the same set from different l3dviews. One default l3dview always exists per l3dset.\
    Each l3dset has its own sun. particlesystems and particleclouds are rendered at the end of every l3dset.\
    Every l3dnode requires a l3dlayerid, directly or indirectly via defaults. l3dsets are independent from each other, and l3dnodes\
    can only be rendered in one l3dset. By default 0-2 are disabled.<br>\
    l3dsets are renderd in this order: 0,1,2,3.<br>",
    NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_SET,LUXI_CLASS_L3D_LIST);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_SET,PubL3DSet_get,(void*)NULL,"get",
    "(l3dset):(int 0-3)- returns l3dset");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_SET,PubL3DSet_default,NULL,"default",
    "([l3dset]):([l3dset])- returns or sets default l3dset.");

  FunctionPublish_initClass(LUXI_CLASS_L3D_LAYERID,"l3dlayerid",
    "l3dlayerid used to assign l3dnodes directly, get it from l3dset. Only needed for manual assign. You can manually multipass render a single layer using the opcode commandstring."
    ,NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_LAYERID,LUXI_CLASS_L3D_LIST);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_SET,PubL3DSet_getlayer,(void*)NULL,"layer",
    "(l3dlayerid):([l3dset],int 0..15) - returns l3dlayerid for l3dset or default l3dset. If default l3dset is used the layer is also set as default layer. So whenever you dont pass l3dlayerid to l3dnodes, the default layer/l3dset combo will be used.");



  FunctionPublish_addFunction(LUXI_CLASS_L3D_SET,PubL3DSet_attributes,(void*)PLSET_DISABLED,"disabled",
    "(boolean):(l3dset,[boolean]) - returns or sets disabled state");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_SET,PubL3DSet_attributes,(void*)PLSET_SUN,"sun",
    "(l3dlight):(l3dset,[l3dlight]) - returns or sets sun, disabled when 2nd arg is not a l3dlight. If disabled we will use default sun");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_SET,PubL3DSet_attributes,(void*)PLSET_DEFAULTVIEW,"getdefaultview",
    "(l3dview):(l3dset) - returns default l3dview of the set");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_SET,PubL3DSet_attributes,(void*)PLSET_UPDATEALL,"updateall",
    "():(l3dset) - updates all nodes, so that world state is uptodate");

  FunctionPublish_initClass(LUXI_CLASS_L3D_VIEW,"l3dview","l3dview is used to render a l3dset. You can specify camera, background and fog for each l3dview.\
Every l3dset has one default l3dview, which cannot be deleted. Additionally you can register more l3dviews which are processed before/after the default views.\
The l3dnodes can be made visible only to certain cameras. By default they are visible to the default camera and windowsized.<br> Another special purpose of l3dviews is rendering to a texture."
  ,NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_VIEW,LUXI_CLASS_L3D_LIST);

  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_new,NULL,"new",
    "(l3dview):([int rcmdbias]) - returns a new l3dview. rcmdbias (default 0) allows you to allow more (>0) or less (<0) than 255 rcmds in this view.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_FOG,"fogstate",
    "([boolean]):(l3dview,[boolean]) - returns or sets");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_FOGSTART,"fogstart",
    "([float]):(l3dview,[float]) - returns or sets");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_FOGEND,"fogend",
    "([float]):(l3dview,[float]) - returns or sets");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_FOGDENS,"fogdensity",
    "([float]):(l3dview,[float]) - returns or sets");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_FOGCOLOR,"fogcolor",
    "([float r,g,b,a]):(l3dview,[float r,g,b,a]) - returns or sets");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_SKYBOX,"bgskybox",
    "([skybox]):(l3dview,[skybox]) - returns or sets skybox, disabled when 2nd arg is not a skybox");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_CAMERA,"camera",
    "(l3dcamera):(l3dview,[l3dcamera]) - returns or sets camera, disabled when 2nd arg is not a camera. If disabled we will use default camera.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_DELETE,"delete",
    "():(l3dview) - deletes the l3dview. Has no effect on defaultviews");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_ACTIVATE,"activate",
    "():(l3dview,l3dset,[l3dview ref,boolean after]) - activates the l3dview in the given set. Activate inserts the l3dview before (or after if set) reference view (or listhead if none given) in the list. Has no effect on defaultviews");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_DEACTIVATE,"deactivate",
    "():(l3dview) - deactivates the l3dview. Has no effect on defaultviews");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_FULLWINDOW,"windowsized",
    "([boolean]):(l3dview,[boolean]) - returns or sets if the viewport should use the full window size (default true).");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_VIEWPOS,"viewpos",
    "([int x,y]):(l3dview,[int x,y]) - returns or sets the viewport starting position in GLpixels. 0,0 is bottom left, which is not as in list2d. only used when windowsized is false");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_VIEWSIZE,"viewsize",
    "([int width,height]):(l3dview,[int width,height]) - returns or sets the viewport size in pixels. only used when windowsized is false. Warning: viewsize must always be smaller than current window resolution (unless FBO rendering is used).");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_VIEWREFBOUNDS,"viewrefbounds",
    "([float refx,refy,refwidth,refheight]):(l3dview,[float  refx,refy,refwidth,refheight]) - returns or sets the viewport size and position in reference coordinates. only used when windowsized is false. Warning: viewsize must always be smaller than current window refsize.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_DEPTHONLY,"depthonly",
    "([boolean]):(l3dview,[boolean]) - returns or sets if a pure Z write should be done.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_DEPTHBOUNDS,"viewdepth",
    "([float min,max]):(l3dview,[float min,max]) - returns or sets range of depthbuffer (0-1).");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_USEPOLYOFFSET,"usepolygonoffset",
    "([boolean]):(l3dview,[boolean]) - returns or sets if depth values are changed depending on slope and constant factors when depth testing occurs.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_POLYOFFSET,"polygonoffset",
    "([float scale,bias]):(l3dview,[float scale,bias]) - returns or sets the polygonoffset values..");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_RCMDVIS,"rcmdflag",
    "([boolean]):(l3dview,int id,[boolean]) - sets rcmd process bit flag. If rcmdflag of view AND the own flag bitwise return true, then the rcmd will be processed. id = 0..31");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_CAMVIS,"defaultcamvisflag",
    "([boolean]):(l3dview,int id,[boolean]) - sets camera visibility bit flag if a default camera is used for this view. It overrides the visflag of the camera itself, however culling is done before and based on the original camera bitid.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_CAMAXIS,"drawcamaxis",
    "([boolean]):(l3dview,[boolean]) - draws camera orientation");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_RCMDEMPTY,"rcmdempty",
    "():(l3dview) - empties the rcmd list of this l3dview.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_RCMDADD,"rcmdadd",
    "(boolean success):(l3dview,rcmd,|rcmd ref,int occurance|,[boolean before]) - adds the rcmd to the rcmd queue list of this l3dview. You can add a maximum of 255 rcmds to default views, and for custom views you can modify this limit. \
They are processed in the order they are added. If there are no commands, the l3dview will do nothing cept \
debug drawing. As long as rcmd is assigned it won't be garbagecollected.<br><br>You can insert after a \
another rcmd, which is searched in the list. When occurance is negative we will search the rcmd from the \
back first. An absolute value of 1 will be first occurance, 2 second and so on. 0 or passing no occurance \
value is same as +1. If before is true we will insert before the given rcmd (default is false).<br><br>\
When only before == true is passed and no ref&occurance, we will insert the command as first.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_RCMDREMOVE,"rcmdremove",
    "(boolean success):(l3dview,rcmd,[int occurance]) - removes the rcmd from the list. Occurance value treated as in rcmdadd.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_RCMDGET,"rcmdget",
    "([rcmd]):(l3dview,int index) -  gets the current rcmd from a specified index, nil if out of bounds. Useful for debugging.");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_FILLLAYERS,"filldrawlayers",
    "([boolean]):(l3dview,[boolean]) - enable/disable filling of drawlayers (l3dtrails, l3dmodels, l3dprimitives, l3dlevelmodel, l3dtext, l3dshadowmodel), particlelayers remain active. Default is true. Useful when a pure fx/l2dnode/particle view is being rendered, or layers are organized to be spread over different l3dviews. When set to false the layerstate of previous l3dview remains active (first l3dview of each l3dset always does a clear). That way you can reuse the data, but it only makes sense if camera visibility stays identical. The layer filling is a rather costly operation, so avoid it when possible.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_VIEW,PubL3DView_attributes,(void*)VAT_DRAW,"drawnow",
    "():(l3dview,[l3dlight sun]) - if the l3dview is not active, it can be drawn directly, ie unrelated of the l3dset. This is mostly meant for immediate/one-time modifications of textures via renderfbo useage. Due to lack of an l3dset layer and particle drawing will show no effect.");

  FunctionPublish_initClass(LUXI_CLASS_L3D_BATCHBUFFER,"l3dbatchbuffer","l3dbatchbuffer contains precompiled models. It is meant for \
efficient rendering of static geometry. Every model affected is transformed using the current hierarchy informatons and its vertices and indices are modified, so \
that the models are stored in big chunks. Meshes then can be batched to minimize drawcalls."
  ,NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_BATCHBUFFER,LUXI_CLASS_L3D_LIST);

  FunctionPublish_addFunction(LUXI_CLASS_L3D_BATCHBUFFER,PubL3DBatch_func,(void*)PBATCH_NEW,"new",
    "(l3dbatchbuffer):(l3dset,scenenode root) - returns a new l3dbatchbuffer for the given l3dset and scenenode. All models, that are \
part of the given l3dset and either directly or indirectly linked to the given scenenode or its children, will be added to the batchbuffer and get their vertex and index data from it. \
Animateable models or those marked as nonbatchable will not be added, neither those with unsupported primitive types (triangle fans,line loop,line strip, polygon). Properties such as color and renderscale will be ignored and reset. Note that the matrices are calculated with the latest information for affected l3dnodes and scenenodes, \
that means make sure the scene is properly set up before. l3dnodes that are linked to bones, will not be added, nor their children, as they are considered animated. You can still modify the resulting worldspace vertexdata via vertexarray/indexarray interface from the affected l3dmodels. <br>\
It is important that you do not change the models position/rotation after batchcompiling, as visibility culling is still based on those.<br> Avoid using matobject interfaces on batchcompiled geometry.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_BATCHBUFFER,PubL3DBatch_func,(void*)PBATCH_DELETE,"delete",
    "():(l3dbatchbuffer) - destroys the batchbuffer and all content in it. All linked drawnodes will revert back to their models' meshes and become fully operational again.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD,"rcmd","The rendercommand mechanism allows you to program the renderer of each l3dview. You can precisely define what you want to render and in which order.",NULL,LUX_TRUE);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD,PubRCmd_func,(void*)PRCMD_FLAG,"flag",
    "([boolean]):(rcmd,int id,[boolean]) - sets rcmd process bit flag. If rcmdflag of view AND the own flag bitwise return true, then the rcmd will be processed. id = 0..31");
  FunctionPublish_setParent(LUXI_CLASS_RCMD,LUXI_CLASS_RENDERINTERFACE);

  FunctionPublish_initClass(LUXI_CLASS_RCMD_CLEAR,"rcmdclear","Clears drawbuffers (stencil,depth,color).",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_CLEAR,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_CLEAR,PubRCmd_new,(void*)LUXI_CLASS_RCMD_CLEAR,"new",
    "(rcmdclear):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_CLEAR,PubRCmdClear_func,(void*)PRCMD_CLEAR_COLORVAL,"colorvalue",
    "([float r,g,b,a]):(rcmdclear,[float r,g,b,a]) - returns or sets");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_CLEAR,PubRCmdClear_func,(void*)PRCMD_CLEAR_DEPTHVAL,"depthvalue",
    "([float]):(rcmdclear,[float 0-1]) - returns or sets");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_CLEAR,PubRCmdClear_func,(void*)PRCMD_CLEAR_STENCILVAL,"stencilvalue",
    "([int]):(rcmdclear,[int]) - returns or sets");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_CLEAR,PubRCmdClear_func,(void*)PRCMD_CLEAR_COLOR,"color",
    "([boolean]):(rcmdclear,[boolean]) - returns or sets");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_CLEAR,PubRCmdClear_func,(void*)PRCMD_CLEAR_DEPTH,"depth",
    "([boolean]):(rcmdclear,[boolean]) - returns or sets");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_CLEAR,PubRCmdClear_func,(void*)PRCMD_CLEAR_STENCIL,"stencil",
    "([boolean]):(rcmdclear,[boolean]) - returns or sets");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_CLEAR,PubRCmdClear_func,(void*)PRCMD_CLEAR_MODE,"colormode",
    "([int]):(rcmdclear,[int 0-2]) - returns or sets mode for color (0 float, 1 int, 2 uint) must have appropriate rendertarget!");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_STENCIL,"rcmdstencil","Sets stenciltest environment.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_STENCIL,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_STENCIL,PubRCmd_new,(void*)LUXI_CLASS_RCMD_STENCIL,"new",
    "(rcmdstencil):() - returns the rcmd");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_DEPTH,"rcmddepth","Sets depthtest environment.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_DEPTH,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DEPTH,PubRCmd_new,(void*)LUXI_CLASS_RCMD_DEPTH,"new",
    "(rcmddepth):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DEPTH,PubRCmd_func,(void*)PRCMD_DEPTHTEST,"compare",
    "([comparemode/false]):(rcmddepth,[comparemode/anything]) - returns or sets comparemode used for depthtest. Passing any non-comparemode will disable setting and current state remains unchanged (default).");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_LOGICOP,"rcmdlogicop","Allows bitwise framebuffer operations (not legal for float rendertargets), overrides blendmodes when active.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_LOGICOP,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_LOGICOP,PubRCmd_new,(void*)LUXI_CLASS_RCMD_LOGICOP,"new",
    "(rcmdlogicop):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_LOGICOP,PubRCmd_func,(void*)PRCMD_LOGICOP,"logic",
    "([logicmode]):(rcmdlogicop,[logicmode]) - returns or sets logicmode used for logicop.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_FORCEFLAG,"rcmdforceflag","Sets enforced renderflag",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_FORCEFLAG,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FORCEFLAG,PubRCmd_new,(void*)LUXI_CLASS_RCMD_FORCEFLAG,"new",
    "(rcmdforceflag):() - returns the rcmd");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_IGNOREFLAG,"rcmdignoreflag","Sets ignored renderflag",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_IGNOREFLAG,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_IGNOREFLAG,PubRCmd_new,(void*)LUXI_CLASS_RCMD_IGNOREFLAG,"new",
    "(rcmdignoreflag):() - returns the rcmd");


    FunctionPublish_initClass(LUXI_CLASS_RCMD_BASESHADERS,"rcmdshaders","Sets use of baseshaders",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_BASESHADERS,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_BASESHADERS,PubRCmd_new,(void*)LUXI_CLASS_RCMD_BASESHADERS,"new",
    "(rcmdshaders):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_BASESHADERS,PubRCmdShaders_func,(void*)PRCMD_SHADERS_SHD,"shaders",
    "([shader color-only,shader color_lightmap,shader 1tex,shader 1tex_lightmap]):(rcmdshaders,[shader color-only,shader color_lightmap,shader 1tex,shader 1tex_lightmap]) - returns or sets baseshaders. Any non-material mesh, ie just having a texture or just having a color, will use those shaders and pass its texture (if exists) as Texture:0 to the shader.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_BASESHADERS,PubRCmdShaders_func,(void*)PRCMD_SHADERS_OVERRIDE,"shaderstage",
    "([int]):(rcmdshaders,[int 0-3]) - returns or sets shaderstage of the material. A material can define up to 4 shaders, the id that you pass here will be used. If a material does not have a shader with the given id, we will use the color-only baseshader. Only active if usebaseshaders is true.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_BASESHADERS_OFF,"rcmdshadersoff","Disables use of l3dview baseshaders",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_BASESHADERS_OFF,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_BASESHADERS_OFF,PubRCmd_new,(void*)LUXI_CLASS_RCMD_BASESHADERS_OFF,"new",
    "(rcmdshadersoff):() - returns the rcmd");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_IGNOREEXTENDED,"rcmdignore","Ignore certain properties",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_IGNOREEXTENDED,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_IGNOREEXTENDED,PubRCmd_new,(void*)LUXI_CLASS_RCMD_IGNOREEXTENDED,"new",
    "(rcmdignore):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_IGNOREEXTENDED,PubRCmdIgnore_func,(void*)PRCMD_IGNORE_LIGHTS,"lights",
    "([boolean]):(rcmdignore,[boolean]) - returns or sets if lights should be turned off.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_IGNOREEXTENDED,PubRCmdIgnore_func,(void*)PRCMD_IGNORE_PROJECTORS,"projectors",
    "([boolean]):(rcmdignore,[boolean]) - returns or sets if projectors should be disabled.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_COPYTEX,"rcmdcopytex","Copys drawbuffer to texture",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_COPYTEX,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_COPYTEX,PubRCmd_new,(void*)LUXI_CLASS_RCMD_COPYTEX,"new",
    "(rcmdcopytex):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_COPYTEX,PubRCmdCopyTex_func,(void*)PRCMD_COPYTEX_TEX,"tex",
    "([texture]):(rcmdcopytex,[texture]) - returns or sets texture");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_COPYTEX,PubRCmdCopyTex_func,(void*)PRCMD_COPYTEX_SIDE,"sideordepth",
    "([int):(rcmdcopytex,[int]) - returns or sets side of texture for cubemaps 0..5, or z slice depth for 3d textures.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_COPYTEX,PubRCmdCopyTex_func,(void*)PRCMD_COPYTEX_MIP,"mipmaplevel",
    "([int):(rcmdcopytex,[int]) - returns or sets mipmap level.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_COPYTEX,PubRCmdCopyTex_func,(void*)PRCMD_COPYTEX_TEXOFFSET,"offset",
    "([int x,y]):(rcmdcopytex,[int x,y]) - returns or sets offset into texture.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_COPYTEX,PubRCmdCopyTex_func,(void*)PRCMD_COPYTEX_SCREENBOUNDS,"screenbounds",
    "([int x,y,w,h]):(rcmdcopytex,[int x,y,w,h]) - returns or sets area of window to be copied into texture. OpenGL coordinates are used.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_COPYTEX,PubRCmdCopyTex_func,(void*)PRCMD_COPYTEX_AUTOSIZE,"autosize",
    "([int]):(rcmdcopytex,[int]) - returns or sets. 0 off -1 viewsized. Default is -1");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_DRAWBACKGROUND,"rcmddrawbg","Draws the background (skybox) of the current l3dview",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_DRAWBACKGROUND,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWBACKGROUND,PubRCmd_new,(void*)LUXI_CLASS_RCMD_DRAWBACKGROUND,"new",
    "(rcmddrawbg):() - returns the rcmd");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_DRAWDEBUG,"rcmddrawdbg","Draws the debug helpers. (see render class)",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_DRAWDEBUG,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWDEBUG,PubRCmd_new,(void*)LUXI_CLASS_RCMD_DRAWDEBUG,"new",
    "(rcmddrawdbg):() - returns the rcmd");


  FunctionPublish_initClass(LUXI_CLASS_RCMD_DRAWLAYER,"rcmddrawlayer","Draws a l3dlayer",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_DRAWLAYER,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWLAYER,PubRCmd_new,(void*)LUXI_CLASS_RCMD_DRAWLAYER,"new",
    "(rcmddrawlayer):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWLAYER,PubRCmdDrawLayer_func,(void*)PRCMD_DRAWLAYER_ID,"layer",
    "([int]):(rcmddrawlayer,[int]) - returns or sets which layer 0..15");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWLAYER,PubRCmdDrawLayer_func,(void*)PRCMD_DRAWLAYER_SORT,"sort",
    "([int]):(rcmddrawlayer,[int]) - returns or sets sorting mode for layer. 0 none 1 material -1 front-to-back -2 back-to-front. Be aware that once camera order is used, the material sortkey is overwritten. Also if you draw the same layer multiple times, you only need to sort it once.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_DRAWPARTICLES,"rcmddrawprt","Draws the particles of an l3dlayer",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_DRAWPARTICLES,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWPARTICLES,PubRCmd_new,(void*)LUXI_CLASS_RCMD_DRAWPARTICLES,"new",
    "(rcmddrawprt):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWPARTICLES,PubRCmdDrawPrt_func,(void*)PRCMD_DRAWPRT_ID,"layer",
    "([int]):(rcmddrawprt,[int]) - returns or sets which layer 0..15");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_DRAWL2D,"rcmddrawl2d","Draws a l2dnode with its children. Useful for render-to-texture GUIs",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_DRAWL2D,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWL2D,PubRCmd_new,(void*)LUXI_CLASS_RCMD_DRAWL2D,"new",
    "(rcmddrawl2d):() - returns the rcmdt");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWL2D,PubRCmdDrawL2D_func,(void*)PRCMD_DRAWL2D_L2D,"root",
    "([l2dnode]):(rcmddrawl2d,[l2dnode]) - allows l2dnode drawing in this viewport. The given node will be unlinked from rest and made the master root for this view. Previous root becomes unlinked. Passing a non-l2dnode disables it.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWL2D,PubRCmdDrawL2D_func,(void*)PRCMD_DRAWL2D_REFSIZE,"refsize",
    "([float w,h]):(rcmddrawl2d,[float w,h]) - The refscreensize width/height while rendering l2droot and its children.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_DRAWL3D,"rcmddrawl3d","Draws a single l3dnode (l3dmodels allow single or sub series of meshids), only when l3dnode is visible. Useful for greater control over mesh rendering.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_DRAWL3D,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWL3D,PubRCmd_new,(void*)LUXI_CLASS_RCMD_DRAWL3D,"new",
    "(rcmddrawl3d):() - returns the rcmd.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWL3D,PubRCmdDrawL3D_func,(void*)PRCMD_DRAWL3D_L3D,"node",
    "([l3dnode]):(rcmddrawl3d,[l3dnode]) - returns or sets which l3dnode is used. l3dmodels default to all meshes being drawn. Passing a non-l3dnode disables it. l3dlevelmodels are not allowed");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWL3D,PubRCmdDrawL3D_func,(void*)PRCMD_DRAWL3D_SUBMESHES,"submeshes",
    "([meshid, int cnt]):(rcmddrawl3d,[meshid,[int cnt]]) - for l3dmodels returns or sets which sub meshid is used as start, as well as how many meshes are drawn (default 1). l3dmodels default to all meshes being drawn. Has no affect on non-l3dmodels.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWL3D,PubRCmdDrawL3D_func,(void*)PRCMD_DRAWL3D_FORCE,"forcedraw",
    "([boolean]):(rcmddrawl3d,[boolean]) - returns or sets whether the l3dnode shall be drawn even if wasnt updated (visible) this frame (default false).");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_DRAWMESH,"rcmddrawmesh","Draws a mesh, be aware that mesh winding is opposite (CCW) of l2dnodes (CW). Positions and sizes are always in OpenGL coordinates (0,0) = bottom left. The l2dnodes' reference size system is not used. By default starts out as fullscreen quad, 2d mesh for orhtographic overlay drawing.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_DRAWMESH,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmd_new,(void*)LUXI_CLASS_RCMD_DRAWMESH,"new",
    "(rcmddrawmesh):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmdDrawMesh_func,(void*)PRCMD_DRAWMESH_AUTOSIZE,"autosize",
    "([int]):(rcmddrawmesh,[int]) - returns or sets. 0 off -1 viewsized. Default is -1");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmdDrawMesh_func,(void*)PRCMD_DRAWMESH_ORTHO,"orthographic",
    "([boolean]):(rcmddrawmesh,[boolean]) - returns or sets whether 2d orhographic overlay mode should be used instead of the current active camera. the renderflags rfNovertexcolor,rfNodepthtest and rfNodepthmask are set to true in orthographic mode (also after matsurface change). (defualt is true).");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmdDrawMesh_func,(void*)PRCMD_DRAWMESH_POS,"pos",
    "([float x,y,z]):(rcmddrawmesh,[float x,y,z]) - returns or sets. Overwritten by matrix. xy Only used when autosize is 0 or non-orthographic. Coordinates in OpenGL (0,0) = bottomleft of current l3dview.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmdDrawMesh_func,(void*)PRCMD_DRAWMESH_MATRIX,"matrix",
    "([matrix4x4]):(rcmddrawmesh,[matrix4x4]) - returns or sets. Overwrites pos and is only used in non-orthographic.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmdDrawMesh_func,(void*)PRCMD_DRAWMESH_SIZE,"size",
    "([float x,y,z]):(rcmddrawmesh,[float x,y,z]) - returns or sets. xy Only used when autosize is 0 or non-orthographic. Coordinates in OpenGL (0,0) = bottomleft of current l3dview");

  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmdDrawMesh_func,(void*)PRCMD_DRAWMESH_COLOR,"color",
    "([float r,g,b,a]):(rcmddrawmesh,[float r,g,b,a]) - returns or sets color");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmdDrawMesh_func,(void*)PRCMD_DRAWMESH_MATSURFACE,"matsurface",
    "([matsurface]):(rcmddrawmesh,[matsurface]) - returns or sets matsurface");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmdDrawMesh_func,(void*)PRCMD_DRAWMESH_QUADMESH,"quadmesh",
    "():(rcmddrawmesh) - deletes usermesh and sets quadmesh again.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmdDrawMesh_func,(void*)PRCMD_DRAWMESH_USERMESH,"usermesh",
    "():(rcmddrawmesh, vertextype, int numverts, numindices, [vidbuffer vbo], [int vbooffset], [vidbuffer ibo], [int ibooffset]) - creates inplace custom rendermesh (see rendermesh for details) Note that polygon winding is CCW.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmdDrawMesh_func,(void*)PRCMD_DRAWMESH_RENDERMESH,
    "rendermesh","([rendermesh]):(rcmddrawmesh,[rendermesh]) - gets or sets rendermesh. Get only works if a usermesh was created before or another rendermesh passed for useage.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_DRAWMESH,PubRCmdDrawMesh_func,(void*)PRCMD_DRAWMESH_LINK,
    "link","([spatialnode]):(rcmddrawmesh,[spatialnode]) - gets or sets if matrix shall be updated from a linked node. This is a weak reference and will not prevent spatialnode from gc. Passing a non-spatialnode disables the link.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_GENMIPMAPS,"rcmdmipmaps","Generates mipmaps for the specified texture",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_GENMIPMAPS,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_GENMIPMAPS,PubRCmd_new,(void*)LUXI_CLASS_RCMD_GENMIPMAPS,"new",
    "(rcmdmipmaps):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_GENMIPMAPS,PubRCmd_func,(void*)PRCMD_GENMIPMAP,"texture",
    "([texture]):(rcmdmipmaps,[texture]) - returns or sets texture to generate mipmaps for.");


  FunctionPublish_initClass(LUXI_CLASS_RCMD_READPIXELS,"rcmdreadpixels","Allows storage of drawn pixels into a vidbuffer.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_READPIXELS,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_READPIXELS,PubRCmd_new,(void*)LUXI_CLASS_RCMD_READPIXELS,"new",
    "(rcmdreadpixels):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_READPIXELS,PubRCmdReadPixels_func,(void*)PRCMD_READ_BUFFER,"buffer",
    "([vidbuffer,offset]):(rcmdreadpixels,[vidbuffer,[int offset]]) - returns or sets vidbuffer used for storage.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_READPIXELS,PubRCmdReadPixels_func,(void*)PRCMD_READ_RECT,"rect",
    "([int x,y,w,h]):(rcmdreadpixels,[int x,y,w,h]) - returns or sets the screen rectangle used to read from.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_READPIXELS,PubRCmdReadPixels_func,(void*)PRCMD_READ_WHAT,"format",
    "():(rcmdreadpixels,textype,texdatatype) - sets whicht data and in what format it is read.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_UPDATETEX,"rcmdupdatetex","Upload the texture data from a vidbuffer.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_UPDATETEX,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_UPDATETEX,PubRCmd_new,(void*)LUXI_CLASS_RCMD_UPDATETEX,"new",
    "(rcmdupdatetex):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_UPDATETEX,PubRCmdUpdateTex_func,(void*)PRCMD_UPDTEX_TEX,"tex",
    "([texture]):(rcmdupdatetex,[texture]) - returns or sets texture used.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_UPDATETEX,PubRCmdUpdateTex_func,(void*)PRCMD_UPDTEX_BUFFER,"buffer",
    "([vidbuffer,offset]):(rcmdupdatetex,[vidbuffer,[int offset]]) - returns or sets vidbuffer used for reading.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_UPDATETEX,PubRCmdUpdateTex_func,(void*)PRCMD_UPDTEX_SIZE,"size",
    "([int w,h,d]):(rcmdupdatetex,[int w,h,d]) - returns or sets the data dimensions.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_UPDATETEX,PubRCmdUpdateTex_func,(void*)PRCMD_UPDTEX_POS,"pos",
    "([int x,y,z,mip]):(rcmdupdatetex,[int x,y,z,[mip]]) - returns or sets the data position.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_UPDATETEX,PubRCmdUpdateTex_func,(void*)PRCMD_UPDTEX_FORMAT,"format",
    "():(rcmdupdatetex,texdatatype) - sets datatype.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_FNCALL,"rcmdfncall","Wraps an external function call lxRcmdcall_func_fn",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_FNCALL,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FNCALL,PubRCmd_new,(void*)LUXI_CLASS_RCMD_FNCALL,"new",
    "(rcmdfncall):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FNCALL,PubRCmdFnCall_func,(void*)PRCMD_FNCALL_FUNC,"func",
    "([pointer fn, upvalue]):(rcmdfncall,[pointer fn, upvalue]) - returns or sets function and upvalue pointers.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FNCALL,PubRCmdFnCall_func,(void*)PRCMD_FNCALL_MATRIX,"matrix",
    "([matrix4x4]):(rcmdfncall,[matrix4x4]) - returns or sets.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FNCALL,PubRCmdFnCall_func,(void*)PRCMD_FNCALL_SIZE,"size",
    "([float x,y,z]):(rcmdfncall,[float x,y,z]) - returns or sets.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FNCALL,PubRCmdFnCall_func,(void*)PRCMD_FNCALL_ORTHO,"orthographic",
    "([boolean]):(rcmdfncall,[boolean]) - returns or sets whether 2d orhographic overlay mode should be used instead of the current active camera.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_R2VB,"rcmdr2vb","Allows storage of processed vertices  into a vidbuffer. All vertices are captured in the order as they are drawn.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_R2VB,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_R2VB,PubRCmd_new,(void*)LUXI_CLASS_RCMD_R2VB,"new",
    "(rcmdr2vb):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_R2VB,PubRCmdRVB_func,(void*)PRCMD_RVB_BUFFER,"buffer",
    "([vidbuffer,offset,size]):(rcmdr2vb,[vidbuffer,[int offset],[size]]) - returns or sets vidbuffer used for storage. size of 0 means rest is used.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_R2VB,PubRCmdRVB_func,(void*)PRCMD_RVB_DRAWMESH,"drawmesh",
    "([rcmddrawmesh]):(rcmdr2vb,[rcmddrawmesh]) - returns or sets the mesh to be drawn.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_R2VB,PubRCmdRVB_func,(void*)PRCMD_RVB_ATTRIB,"attrib",
    "():(rcmdr2vb,int index, string name, int components) - sets which vertex attribute and how many components shall be read. Use the Cg semantic names (eg. 'TEXCOORD3'). Components can be 1-4 for POSITION,TEXCOORD and ATTR and 1 for rest. The index defines the ordered index how you want to store the output. index must be 0-15.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_R2VB,PubRCmdRVB_func,(void*)PRCMD_RVB_NUMATTRIB,"numattrib",
    "([int]):(rcmdr2vb,[int]) - sets or gets how many attribs are read.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_R2VB,PubRCmdRVB_func,(void*)PRCMD_RVB_CAPTURE,"capture",
    "([boolean]):(rcmdr2vb,[boolean]) - sets or gets if the number of captured primitives shall be captured. (default is false).");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_R2VB,PubRCmdRVB_func,(void*)PRCMD_RVB_OUTPUT,"lastcaptured",
    "([int]):(rcmdr2vb) - returns number of captured vertices in last frame (if capture was active).");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_R2VB,PubRCmdRVB_func,(void*)PRCMD_RVB_NORASTER,"noraster",
    "([boolean]):(rcmdr2vb,[boolean]) - sets or gets whether rasterization is disabled. (default true, ie. nothing is drawn to framebuffer.)");


  FunctionPublish_initClass(LUXI_CLASS_RCMD_FBO_BIND,"rcmdfbobind","Binds a renderfbo (if capability exists). Fbos allow enhanced render-to-texture operation, and off-screen rendering to large buffers. Once you start using renderfbos it is heavily recommended to check your setup for the whole l3dlist with l3dlist.fbotest. An fbo setup consists of rcmdfbobind (for binding), rcmdfbotex or rcmdfborb for attaching renderbuffers/textures and finally the rcmdfbodrawto command which specifies which assignment the color is rendered to.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_FBO_BIND,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_BIND,PubRCmd_new,(void*)LUXI_CLASS_RCMD_FBO_BIND,"new",
    "(rcmdfbobind):() - returns the rcmd for fbo binding. Make sure to setup the fbo to bind.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_BIND,PubRCmd_func,(void*)PRCMD_FBO_BIND,"setup",
    "([renderfbo, boolean viewportchange]):(rcmdfbobind,[renderfbo,[boolean viewportchange]]) - returns or sets renderfbo, must be defined before use. Viewportchange means that active viewport dimensions are changed to fbo dimension (0,0, fbowidth, fboheight) and is by default true. l3dviews viewport dimensions become active again once fbo is unbound.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_BIND,PubRCmd_func,(void*)PRCMD_FBO_BIND_TARGET,"readbuffer",
    "([boolean readbuffer]):(rcmdfbobind, [boolean readbuffer]) - returns or sets whether target is readbuffer binding, or drawbuffer. Readbuffer functionality requires extra capability and by default is off. Viewportchange is ignored when readbuffer is true.");


  FunctionPublish_initClass(LUXI_CLASS_RCMD_FBO_OFF,"rcmdfbooff","Unbinds bound fbo, and returns to window backbuffer again.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_FBO_OFF,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_OFF,PubRCmd_new,(void*)LUXI_CLASS_RCMD_FBO_OFF,"new",
    "(rcmdfbooff):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_OFF,PubRCmd_func,(void*)PRCMD_FBO_BIND_TARGET,"readbuffer",
    "([boolean readbuffer]):(rcmdfbobind, [boolean readbuffer]) - returns or sets whether target is readbuffer binding, or drawbuffer. Readbuffer functionality requires extra capability and by default is off.");


  FunctionPublish_initClass(LUXI_CLASS_RCMD_FBO_TEXASSIGN,"rcmdfbotex","Attaches textures to the current bound fbo. By default the drawbuffer is used for assignments.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_FBO_TEXASSIGN,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_TEXASSIGN,PubRCmd_new,(void*)LUXI_CLASS_RCMD_FBO_TEXASSIGN,"new",
    "(rcmdfbotex):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_TEXASSIGN,PubRCmd_func,(void*)PRCMD_FBO_ASSIGNTEX_COLOR,"color",
    "([texture, sideordepth, miplevel]):(rcmdfbotex,int clr,[texture,[int sideordepth],[miplevel]]) - returns or sets renderbuffer attachments for color targets 0..3. You can disable certain assignments by passing 0. sideordepth is the side of a cubemap, or the z slice/layer of a 3d/array texture. Miplevel defaults to 0.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_TEXASSIGN,PubRCmd_func,(void*)PRCMD_FBO_ASSIGNTEX_DEPTH,"depth",
    "([texture, sideordepth, miplevel]):(rcmdfbotex,[texture,[int sideordepth],[int miplevel]]) - returns or sets renderbuffer attachment for depth target. You can disable assignment by passing 0.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_TEXASSIGN,PubRCmd_func,(void*)PRCMD_FBO_ASSIGNTEX_STENCIL,"stencil",
    "([texture, sideordepth, miplevel]):(rcmdfbotex,[texture,[int sideordepth],[int miplevel]]) - returns or sets renderbuffer attachment for stencil target. You can disable assignment by passing 0.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_TEXASSIGN,PubRCmd_func,(void*)PRCMD_FBO_ASSIGNTEX_TARGET,"readbuffer",
    "([boolean readbuffer]):(rcmdfbobind, [boolean readbuffer]) - returns or sets whether target is readbuffer fbo binding, or drawbuffer. Readbuffer functionality requires extra capability and by default is off.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_FBO_RBASSIGN,"rcmdfborb","Attaches renderbuffers to the current bound fbo. By default the drawbuffer is used for assignments.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_FBO_RBASSIGN,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_RBASSIGN,PubRCmd_new,(void*)LUXI_CLASS_RCMD_FBO_RBASSIGN,"new",
    "(rcmdfborb):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_RBASSIGN,PubRCmd_func,(void*)PRCMD_FBO_ASSIGNRB_COLOR,"color",
    "([renderbuffer]):(rcmdfborb,int clr,[renderbuffer]) - returns or sets renderbuffer attachments for color targets 0..3. You can disable certain assignments by passing 0.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_RBASSIGN,PubRCmd_func,(void*)PRCMD_FBO_ASSIGNRB_DEPTH,"depth",
    "([renderbuffer]):(rcmdfborb,[renderbuffer]) - returns or sets renderbuffer attachment for depth target. You can disable assignment by passing 0.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_RBASSIGN,PubRCmd_func,(void*)PRCMD_FBO_ASSIGNRB_STENCIL,"stencil",
    "([renderbuffer]):(rcmdfborb,[renderbuffer]) - returns or sets renderbuffer attachment for stencil target. You can disable assignment by passing 0.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_RBASSIGN,PubRCmd_func,(void*)PRCMD_FBO_ASSIGNRB_TARGET,"readbuffer",
    "([boolean readbuffer]):(rcmdfbobind, [boolean readbuffer]) - returns or sets whether target is readbuffer fbo binding, or drawbuffer. Readbuffer functionality requires extra capability and by default is off.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_FBO_DRAWBUFFERS,"rcmdfbodrawto","Sets to which color attachments it should be rendered to. Using only fragment shaders you can also write to up to four attachments simultaneously.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_FBO_DRAWBUFFERS,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_DRAWBUFFERS,PubRCmd_new,(void*)LUXI_CLASS_RCMD_FBO_DRAWBUFFERS,"new",
    "(rcmdfbodrawto):() - returns the rcmd. By default draws to color attach 0");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_DRAWBUFFERS,PubRCmd_func,(void*)PRCMD_FBO_DRAWTO,"setup",
    "([int buffer0,buffer1..]):(rcmdfbodrawto,[int buffer0,buffer1..]) - returns or sets the active color drawbuffers. Only ints from 0-3 are allowed. You can specify up to 4 ints, if you want multiple rendertargets and capability exists.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_FBO_READBUFFER,"rcmdfboreadfrom","Sets from which color attachments data should be read.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_FBO_READBUFFER,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_READBUFFER,PubRCmd_new,(void*)LUXI_CLASS_RCMD_FBO_READBUFFER,"new",
    "(rcmdfboreadfrom):() - returns the rcmd. By default reads from color attach 0");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_FBO_READBUFFER,PubRCmd_func,(void*)PRCMD_FBO_READFROM,"setup",
    "([int buffer]):(rcmdfboreadfrom,[int buffer]) - returns or sets the active color readbuffer.");

  FunctionPublish_initClass(LUXI_CLASS_RCMD_BUFFER_BLIT,"rcmdbufferblit","Blits a rectangle from current read to drawbuffer. Also allows stretching and flipping the rectangle specified. When no fbo is bound backbuffer is used for read or draw.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RCMD_BUFFER_BLIT,LUXI_CLASS_RCMD);
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_BUFFER_BLIT,PubRCmd_new,(void*)LUXI_CLASS_RCMD_BUFFER_BLIT,"new",
    "(rcmdbufferblit):() - returns the rcmd");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_BUFFER_BLIT,PubRCmd_func,(void*)PRCMD_FBO_BLIT_CONTENT,"content",
    "([boolean color,depth,stencil]):(rcmdbufferblit,[boolean color,depth,stencil]) - returns or sets which buffers are copied.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_BUFFER_BLIT,PubRCmd_func,(void*)PRCMD_FBO_BLIT_LINEAR,"linear",
    "([boolean]):(rcmdbufferblit,[boolean]) - returns or sets whether linear filtering shall be applied when sizes mismatch. Not allowed when depth or stencil are involved in copy.");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_BUFFER_BLIT,PubRCmd_func,(void*)PRCMD_FBO_BLIT_SRC,"from",
    "([int x,y,w,h]):(rcmdbufferblit,[int x,y,w,h]) - returns or sets rectangle to copy from. OpenGL pixel coordinates");
  FunctionPublish_addFunction(LUXI_CLASS_RCMD_BUFFER_BLIT,PubRCmd_func,(void*)PRCMD_FBO_BLIT_DST,"to",
    "([int x,y,w,h]):(rcmdbufferblit,[int x,y,w,h]) - returns or sets rectangle to copy to. OpenGL pixel coordinates. Negative width and height means flipping along these axis.");

  FunctionPublish_initClass(LUXI_CLASS_RENDERBUFFER,"renderbuffer","The renderbuffer is for storing results of rendering processes for fbo attachments using rcmdfborb. It cannot be used as texture directly, but allows faster and more complex (multi-sampled) internal storage. Using rcmdbufferblit you can blit the result of a renderbuffer to a texture or backbuffer.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RENDERBUFFER,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_RENDERBUFFER,PubRenderBuffer_func,(void*)NULL,"new",
    "([renderbuffer]):([textype,int width,height,[boolan windowsized],[int multisamples]]) - returns a new renderbuffer. You must setup its attributes prior use or pass directly. When setting failed no rb is returend.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERBUFFER,PubRenderBuffer_func,(void*)1,"setup",
    "([renderbuffer]):(renderbuffer,textype,int width,height,[boolan windowsized],[int multisamples]) - returns self if the renderbuffer could be set to the given attributes. Windowsized means width and height are ignored and automatically set to window size, also when changing window resolution. Multisamples can be set to a value >0 if hardware capability for multisample renderbuffers exists.");

  FunctionPublish_initClass(LUXI_CLASS_RENDERFBO,"renderfbo","The framebufferobject (fbo) can be used for advanced off-screen rendering effects. For this class you need special capability. When not using fbos rendering is done to the windows's backbuffer. The 'fbo' rcmds can be used to make use of it.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_RENDERFBO,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFBO,PubFrameBuffer_func,(void*)PFBO_NEW,"new",
    "(renderfbo):(int width,height,[boolean windowsized]) - returns the new created fbo. If windowsized is true width and height are ignored and current window resolution is taken and fbo is resized on window.size changes, too.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFBO,PubFrameBuffer_func,(void*)PFBO_SETUP,"size",
    "([int width,height, boolean windowsized]):(renderfbo,[int width,height,[boolean windowsized ]]) - returns or sets fbo size. All attachments must have equal size.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFBO,PubFrameBuffer_func,(void*)PFBO_RESET,"resetattach",
    "():(renderfbo) - resets the attach bind cache. Useful if you reloaded assigned textures. l3dlist.fbotest calls this as well.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFBO,PubFrameBuffer_func,(void*)PFBO_CHECK,"check",
    "([string error]):(renderfbo) - checks for completeness and returns error string if issue was found with current attaches.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERFBO,PubFrameBuffer_func,(void*)PFBO_APPLY,"applyattach",
    "():(renderfbo,[rcmdfbotex],[rcmdfborb]) - runs the rcmds and applies there attach information (run resetattach before to enforce setting). Passing a illegal type will ignore either.");
}

