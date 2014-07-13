// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"

#include "../render/gl_list3d.h"
#include "../render/gl_list2d.h"
#include "../common/reflib.h"
#include "../scene/scenetree.h"
#include "../scene/actorlist.h"

// Published here:
// LUXI_CLASS_L3D_NODE
// LUXI_CLASS_L3D_LIST
// LUXI_CLASS_L3D_PRIMITIVE
// LUXI_CLASS_L3D_TEXT
// LUXI_CLASS_L3D_SHADOWMODEL
// LUXI_CLASS_L3D_LEVELMODEL
// shared functions for LUXI_CLASS_L3D_x






// L3D_LIST

enum PubL3Dfuncs_e{
  PL3D_FBOTEST,
  PL3D_MAXSETS,
  PL3D_MAXLAYERS,

  PL3D_PERLAYER,
  PL3D_TOTALPROJ,
};


static int PubL3D_updateall (PState pstate,PubFunction_t *fn, int n)
{
  List3D_updateAll();
  return 0;
}

static int PubL3D_funcs (PState pstate,PubFunction_t *fn, int n)
{
  int val;
  switch((int)fn->upvalue)
  {
  case PL3D_FBOTEST:
    {
      char *erroroutput;
      int what;
      List3DView_t *view = NULL;
      Reference ref;
      bufferclear();
      erroroutput = buffermalloc(sizeof(char)*2048);
      *erroroutput = 0;

      LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

      if (n==1 && (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_VIEW,(void*)&ref) || !Reference_get(ref,view)))
        return FunctionPublish_returnError(pstate,"1 l3dview required");

      if (what = List3D_fbotest(erroroutput,2048,view)){
        return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)what,LUXI_CLASS_STRING,(void*)erroroutput);
      }
      return 0;
    }
  case PL3D_MAXLAYERS:
    return FunctionPublish_returnInt(pstate,LIST3D_LAYERS);
  case PL3D_MAXSETS:
    return FunctionPublish_returnInt(pstate,LIST3D_SETS);
  case PL3D_PERLAYER:
    if (n==0) return FunctionPublish_returnInt(pstate,LIST3D_LAYER_MAX_NODES);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&val) && val > 1)
      return FunctionPublish_returnError(pstate,"1 integer (>1) required");

    g_List3D.maxDrawPerLayer = val;
    g_List3D.maxDrawPerLayerMask = val-1;

    List3D_updateMemPoolRelated();
    break;
  case PL3D_TOTALPROJ:
    if (n==0) return FunctionPublish_returnInt(pstate,LIST3D_MAX_TOTAL_PROJECTORS);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&val) && val > 1)
      return FunctionPublish_returnError(pstate,"1 integer (>1) required");

    g_List3D.maxTotalProjectors = val;

    List3D_updateMemPoolRelated();
    break;

  }

  return 0;
}





// L3D

int PubL3D_return(PState pstate,List3DNode_t *l3d)
{
  if (!l3d)
    return 0;

  return FunctionPublish_returnType(pstate,l3d->nodeType,REF2VOID(l3d->reference));
}



///////////////////////////////////////////////////////////////////////////////
// Shared functions

static int PubList3D_free (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;

  if ((n<1) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)||
    !Reference_get(ref,l3d))
    return 0;

  if ((l3d->nodeType == LUXI_CLASS_L3D_CAMERA && Camera_isDefault(l3d->cam)) ||
    (l3d->nodeType == LUXI_CLASS_L3D_LIGHT && Light_isDefault(l3d->light)))
    return 0;

  Reference_free(ref);//RList3DNode_free(ref);

  return 0;
}

static int PubList3D_name(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;

  if ((n<1) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)||
    !Reference_get(ref,l3d))
    return 0;

  return FunctionPublish_returnString(pstate,l3d->name);
}


enum PubL3dStates_e{
  PL3D_NOVISTEST,
  PL3D_USELOCAL,
  PL3D_USEMANUAL,
  PL3D_USELOOKAT,
  PL3D_LASTFRAME,
};


static int PubList3D_states(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;

  byte state;

  if ( n < 1 || !FunctionPublish_getArg(pstate,2,LUXI_CLASS_L3D_NODE,(void*)&ref,LUXI_CLASS_BOOLEAN,(void*)&state)  ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dnode required");

  switch((int)f->upvalue){
  case PL3D_NOVISTEST:
    if (n==1) return FunctionPublish_returnBool(pstate,List3DNode_useNovistest(l3d,-1));
    List3DNode_useNovistest(l3d,state);
    return 0;
  case PL3D_USEMANUAL:
    if (n==1) return FunctionPublish_returnBool(pstate,List3DNode_useManualWorld(l3d,-1));
    List3DNode_useManualWorld(l3d,state);
    return 0;
  case PL3D_USELOCAL:
    if (l3d->nodeType == LIST3D_UPD_UNLINKED) return FunctionPublish_returnError(pstate,"l3dnode must be linked");
    if (n==1) return FunctionPublish_returnBool(pstate,List3DNode_useLocal(l3d,-1));
    List3DNode_useLocal(l3d,state);
    return 0;
  case PL3D_USELOOKAT:
    if (n==1) return FunctionPublish_returnBool(pstate,List3DNode_useLookAt(l3d,-1));
    List3DNode_useLookAt(l3d,state);
    return 0;
  case PL3D_LASTFRAME:
    return FunctionPublish_returnInt(pstate,l3d->drawnFrame);
  }

  return 0;
}

static int PubList3D_linkInterfaces(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  Reference refsn;
  Reference refact;
  List3DNode_t *l3d;
  SceneNode_t *s3d;
  ActorNode_t *act;

  Reference_reset(refsn);
  Reference_reset(refact);

  if ((n<1) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 valid l3dnode required");

  if (n==1){
    if (l3d->linkType == LINK_UNSET || l3d->updateType == LIST3D_UPD_UNLINKED || l3d->unlinked || l3d->parent)
      return 0;
    return FunctionPublish_returnType(pstate,l3d->linkType ? LUXI_CLASS_ACTORNODE : LUXI_CLASS_SCENENODE,REF2VOID(l3d->linkReference));
  }

  if (!(  FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SCENENODE,(void*)&refsn) ||
    FunctionPublish_getNArg(pstate,1,LUXI_CLASS_ACTORNODE,(void*)&refact)))
    return FunctionPublish_returnError(pstate,"1 valid l3dnode 1 valid actor/scenenode required");

  // always unlink before
  List3DNode_unlink(l3d);

  if (Reference_get(refsn,s3d)){
    if (!s3d->link.visobject)
      return FunctionPublish_returnError(pstate,"scenenode is not drawable");
    if(List3DNode_setIF(l3d,refsn,0,0))
      return FunctionPublish_returnError(pstate,"scenenode already has l3dnode with different l3dset bound");

  }
  else if (Reference_get(refact,act)) {
    if (!act->link.visobject)
      return FunctionPublish_returnError(pstate,"actornode is not drawable");
    if(List3DNode_setIF(l3d,refact,1,0))
      return FunctionPublish_returnError(pstate,"actornode already has l3dnode with different l3dset bound");
  } else
    return FunctionPublish_returnError(pstate,"1 valid l3dnode 1 valid actor/scenenode required");

  return 0;
}

static int PubList3D_lightmap(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  float *mat;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref) ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dnode required");

  if (f->upvalue){
    if (n==2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,mat)){
        if (l3d->enviro.lmtexmatrix)
          genfreeSIMD(l3d->enviro.lmtexmatrix,sizeof(lxMatrix44));
        l3d->enviro.lmtexmatrix = NULL;
        return 0;
      }
      else if (!l3d->enviro.lmtexmatrix)
        l3d->enviro.lmtexmatrix = genzallocSIMD(sizeof(lxMatrix44));

      lxMatrix44CopySIMD(l3d->enviro.lmtexmatrix,mat);
    }
    else if (l3d->enviro.lmtexmatrix){
      return PubMatrix4x4_return(pstate,l3d->enviro.lmtexmatrix);
    }
  }
  else{
    if (n==2){
      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_TEXTURE,(void*)&l3d->enviro.lightmapRID))
        l3d->enviro.lightmapRID = -1;
    }
    else if (l3d->enviro.lightmapRID >= 0){
      return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)l3d->enviro.lightmapRID);
    }
  }



  return 0;
}

static int PubList3D_link (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  Reference ref2;
  List3DNode_t *a,*b;

  if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref) || !Reference_get(ref,a))
    return FunctionPublish_returnError(pstate,"1 valid l3dnode required");

  if (n == 1){
    if (a->parent)
      return PubL3D_return(pstate,a->parent);
    else
      return 0;
  }
  else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_NODE,(void*)&ref2)   || !Reference_get(ref2,b))
    return FunctionPublish_returnError(pstate,"2 valid l3dnodes required");

  if(List3DNode_link(a,b))
    return FunctionPublish_returnError(pstate,"l3dnodes must have same l3dset");
  return 0;
}

static int PubList3D_animateable (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;

  FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_L3D_NODE,ref,l3d)

  return FunctionPublish_returnBool(pstate,
    l3d->nodeType==LUXI_CLASS_L3D_MODEL && l3d->minst->boneupd!=NULL);
}

static int PubList3D_unlink(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;

  if (n!=1 ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)    ||
    !Reference_get(ref,l3d) )
    return FunctionPublish_returnError(pstate,"1 valid l3dnode required");

  List3DNode_unlink(l3d);
  return 0;
}

static int PubList3D_linkParentBone (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  char *name;
  List3DNode_t *l3d;

  if (n!=2 ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)  ||
    !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&name) ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 valid l3dnode and 1 string required");

  List3DNode_linkParentBone(l3d,name);
  return 0;
}

static int PubList3D_unlinkParentBone (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;

  if (n!=1 ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)    ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 valid l3dnode required");

  List3DNode_unlinkParentBone(l3d);
  return 0;
}

static int PubList3D_layer (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  int layer;
  List3DNode_t *l3d;
  int mid = -1;

  if ((n < 2) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)    ||
    !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID,(void*)&layer) ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 valid l3dnode and 1 l3dlayerid required");

  if (n>2 && l3d->nodeType == LUXI_CLASS_L3D_MODEL && (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_MESHID,(void*)&mid) || !SUBRESID_CHECK(mid,l3d->checkRID)))
    return FunctionPublish_returnError(pstate,"1 meshid required");

  if (List3DNode_setLayer(l3d,layer,mid))
    return FunctionPublish_returnError(pstate,"l3dset change not allowed unless fully unlinked");

  return 0;
}

static int PubList3D_inheritLockRot (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  int lock;
  int curlocks;
  List3DNode_t *l3d;

  byte state;

  if ((n!=2 && n!=3) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)    ||
    !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&lock) ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 valid l3dnode 1 int (0-3) required, optional 1 boolean");

  switch(lock) {
  case 3:   lock = ROTATION_LOCK_ALL;   break;
  case 0:   lock = ROTATION_LOCK_X;   break;
  case 1:   lock = ROTATION_LOCK_Y;   break;
  case 2:   lock = ROTATION_LOCK_Z;   break;
  default:
    return 0;
  }

  if (n!=3 || !FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&state)){
    state = isFlagSet(l3d->inheritLocks,lock);
    return FunctionPublish_returnBool(pstate,state);
  }

  curlocks = l3d->inheritLocks;

  if (state)
    curlocks |= lock;
  else
    curlocks &= ~lock;

  List3DNode_setInheritLocks(l3d,curlocks);

  return 0;
}

static int PubList3D_visvolume(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  lxVector3 min;
  lxVector3 max;
  List3DNode_t *l3d;

  bool8 inside;

  if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)  ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 valid l3dnode required");


  switch((int)f->upvalue) {
  case 0: // sphere
    if ((n<3) ||  !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&inside) ||
            !FunctionPublish_getNArg(pstate,2,LUXI_CLASS_FLOAT,(void*)&min[0]))
      return FunctionPublish_returnError(pstate,"1 valid l3dnode 1 boolean 1 float required");
    if (!l3d->l3dvis){
      l3d->l3dvis = lxMemGenZalloc(sizeof(List3DVis_t));
    }
    l3d->l3dvis->inside = inside;
    l3d->l3dvis->omax[0] = min[0]*min[0];
    l3d->l3dvis->box = LUX_FALSE;
    if (FunctionPublish_getNArg(pstate,3,LUXI_CLASS_FLOAT,(void*)&min[0])){
      l3d->l3dvis->omax[0] = min[0]*min[0];
      l3d->l3dvis->ranged = LUX_TRUE;
    }
    break;
  case 1: // box
    if ((n<8) ||  8!=FunctionPublish_getArg(pstate,8,LUXI_CLASS_L3D_NODE,(void*)&ref,LUXI_CLASS_BOOLEAN,(void*)&inside,
        FNPUB_TOVECTOR3(min),
        FNPUB_TOVECTOR3(max)))
      return FunctionPublish_returnError(pstate,"1 valid l3dnode 1 boolean 6 floats required");
    if (!l3d->l3dvis){
      l3d->l3dvis = lxMemGenZalloc(sizeof(List3DVis_t));
    }
    l3d->l3dvis->inside = inside;
    lxVector3Copy(l3d->l3dvis->omin,min);
    lxVector3Copy(l3d->l3dvis->omax,max);
    l3d->l3dvis->box = LUX_TRUE;
    if (6==FunctionPublish_getArgOffset(pstate,8,6,FNPUB_TOVECTOR3(min),FNPUB_TOVECTOR3(max))){
      lxVector3Copy(l3d->l3dvis->imin,min);
      lxVector3Copy(l3d->l3dvis->imax,max);
      l3d->l3dvis->ranged = LUX_TRUE;
    }
    break;
  case 2: // clear
    if (l3d->l3dvis)
      lxMemGenFree(l3d->l3dvis,sizeof(List3DVis_t));
    l3d->l3dvis = NULL;
    break;
  default:
    break;
  }

  return 0;
}



static int PubList3D_renderScale (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  int i;
  DrawNode_t  *drawnode;
  int userenderscale;

  if ((n!=1 && n!=4) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)    ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dnode [3 floats] required");

  switch(n) {
  case 1:
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(l3d->renderscale));
  case 4:
    if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(l3d->renderscale)) != 3)
      return FunctionPublish_returnError(pstate,"3 numbers required");
    break;
    return 0;
  }

  userenderscale=(l3d->renderscale[0]!=1 || l3d->renderscale[1]!=1 || l3d->renderscale[2]!=1);

  drawnode = l3d->drawNodes;
  for (i = 0; i < l3d->numDrawNodes; i++,drawnode++){
    if (userenderscale){
      drawnode->renderscale = l3d->renderscale;
      drawnode->draw.state.renderflag |= RENDER_NORMALIZE;
    }
    else{
      drawnode->renderscale = NULL;
      drawnode->draw.state.renderflag &= ~ RENDER_NORMALIZE;
    }
  }

  return 0;
}

static int PubList3D_localPos (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;

  if ((n!=1 && n!=4) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dnode [3 floats] required");

  switch(n) {
  case 1:
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMMATRIXPOS(l3d->localMatrix));
  case 4:
    if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOMATRIXPOS(l3d->localMatrix)) != 3)
      return FunctionPublish_returnError(pstate,"3 numbers required");
    List3DNode_markUpdate(l3d);
  }
  return 0;
}

static int PubList3D_localRot (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  float *x,*y,*z;
  List3DNode_t *l3d;
  lxVector4 vector;

  if ((n<1) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)    ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dnode required");


  switch((int)f->upvalue) {
  case ROT_AXIS:
    if (n<2){
      x = y = z = l3d->localMatrix;
      y+=4;
      z+=8;
      return FunctionPublish_setRet(pstate,9, FNPUB_FROMVECTOR3(x)
        ,FNPUB_FROMVECTOR3(y)
        ,FNPUB_FROMVECTOR3(z));
    }
    else{
      x = y = z = l3d->localMatrix;
      y+=4;
      z+=8;
      if (FunctionPublish_getArgOffset(pstate,1,9,FNPUB_TOVECTOR3(x),
        FNPUB_TOVECTOR3(y),
        FNPUB_TOVECTOR3(z)) != 9)
        return FunctionPublish_returnError(pstate,"9 floats required");
      List3DNode_markUpdate(l3d);
      return 0;
    }
    break;
  case ROT_QUAT:
    if (n<2){
      lxQuatFromMatrix(vector,l3d->localMatrix);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(vector)) <4)
        return FunctionPublish_returnError(pstate,"4 numbers required");
      lxQuatToMatrix(vector,l3d->localMatrix);
      List3DNode_markUpdate(l3d);
      return 0;
    }
  default:
  case ROT_RAD:
    if (n<2){
      lxMatrix44ToEulerZYX(l3d->localMatrix,vector);
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(vector)) != 3)
        return FunctionPublish_returnError(pstate,"3 numbers required");
      lxMatrix44FromEulerZYXFast(l3d->localMatrix,vector);
      List3DNode_markUpdate(l3d);
      return 0;
    }
    break;
  case ROT_DEG:
    if (n<2){
      lxMatrix44ToEulerZYX(l3d->localMatrix,vector);
      vector[0] = LUX_RAD2DEG(vector[0]);
      vector[1] = LUX_RAD2DEG(vector[1]);
      vector[2] = LUX_RAD2DEG(vector[2]);
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(vector)) != 3)
        return FunctionPublish_returnError(pstate,"3 numbers required");
      lxMatrix44FromEulerZYXdeg(l3d->localMatrix,vector);
      List3DNode_markUpdate(l3d);
      return 0;
    }
    break;

  }

  return 0;
}

static int PubList3D_finalPos (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;

  if ((n!=1 && n!=4) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dnode [3 floats] required");

  switch(n) {
  case 1:
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMMATRIXPOS(l3d->finalMatrix));
  case 4:
    if (l3d->updateType && l3d->updateType < LIST3D_UPD_MANUAL_WORLD  || FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOMATRIXPOS(l3d->finalMatrix)) != 3)
      return FunctionPublish_returnError(pstate,"1 l3dnode (manual world) 3 floats required");
    List3DNode_markUpdate(l3d);
  }
  return 0;
}

static int PubList3D_finalRot (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  float *x,*y,*z;
  List3DNode_t *l3d;
  lxVector4 vector;

  if ((n<1) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref)    ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dnode required");

  if (n > 1 && l3d->updateType && l3d->updateType < LIST3D_UPD_MANUAL_WORLD)
    return FunctionPublish_returnError(pstate,"1 l3dnode (manual world) required");


  switch((int)f->upvalue) {
  case ROT_AXIS:
    if (n<2){
      x = y = z = l3d->finalMatrix;
      y+=4;
      z+=8;
      return FunctionPublish_setRet(pstate,9, FNPUB_FROMVECTOR3(x)
        ,FNPUB_FROMVECTOR3(y)
        ,FNPUB_FROMVECTOR3(z));
    }
    else{
      x = y = z = l3d->finalMatrix;
      y+=4;
      z+=8;
      if (FunctionPublish_getArgOffset(pstate,1,9,FNPUB_TOVECTOR3(x),
        FNPUB_TOVECTOR3(y),
        FNPUB_TOVECTOR3(z)) != 9)
        return FunctionPublish_returnError(pstate,"1 l3dnode (manual world) 9 floats required");
      List3DNode_markUpdate(l3d);
      return 0;
    }
    break;
  case ROT_QUAT:
    if (n<2){
      lxQuatFromMatrix(l3d->finalMatrix,vector);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR3(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR3(vector)) < 4)
        return FunctionPublish_returnError(pstate,"4 numbers required");
      lxQuatToMatrix(l3d->finalMatrix,vector);
      List3DNode_markUpdate(l3d);
      return 0;
    }
    break;
  default:
  case ROT_RAD:
    if (n<2){
      lxMatrix44ToEulerZYX(l3d->finalMatrix,vector);
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(vector)) != 3)
        return FunctionPublish_returnError(pstate,"1 l3dnode (manual world) 3 floats required");
      lxMatrix44FromEulerZYXFast(l3d->finalMatrix,vector);
      List3DNode_markUpdate(l3d);
      return 0;
    }
    break;
  case ROT_DEG:
    if (n<2){
      lxMatrix44ToEulerZYX(l3d->finalMatrix,vector);
      vector[0] = LUX_RAD2DEG(vector[0]);
      vector[1] = LUX_RAD2DEG(vector[1]);
      vector[2] = LUX_RAD2DEG(vector[2]);
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(vector)) != 3)
        return FunctionPublish_returnError(pstate,"1 l3dnode (manual world) 3 floats required");
      lxMatrix44FromEulerZYXdeg(l3d->finalMatrix,vector);
      List3DNode_markUpdate(l3d);
      return 0;
    }
    break;

  }

  return 0;
}

static int PubList3D_matrix(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  float *mat;
  List3DNode_t *l3d;

  if (n< 1 || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref) || !Reference_get(ref,l3d)))
    return FunctionPublish_returnError(pstate,"1 l3dnode required");

  if (n==1)
    return PubMatrix4x4_return (pstate,l3d->localMatrix);
  else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,mat))
    return FunctionPublish_returnError(pstate,"1 matrix4x4 required");
  lxMatrix44CopySIMD(l3d->localMatrix,mat);
  List3DNode_markUpdate(l3d);

  return 0;
}

static int PubList3D_finalmatrix(PState pstate,PubFunction_t *f, int n)
{
  static lxMatrix44SIMD matrix;

  Reference ref;
  float *mat;
  List3DNode_t *l3d;
  ActorNode_t *act;
  SceneNode_t *scn;

  if (n< 1 || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref) || !Reference_get(ref,l3d)))
    return FunctionPublish_returnError(pstate,"1 l3dnode required");

  if (n==1)
    return PubMatrix4x4_return (pstate,l3d->finalMatrix);

  if (l3d->updateType && l3d->updateType < LIST3D_UPD_MANUAL_WORLD)
    return FunctionPublish_returnError(pstate,"1 l3dnode (manual world)");

  switch(FunctionPublish_getNArgType(pstate,1))
  {
  case LUXI_CLASS_MATRIX44:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,mat))
      return FunctionPublish_returnError(pstate,"1 l3dnode 1 matrix4x4 required");
    break;
  case LUXI_CLASS_ACTORNODE:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_ACTORNODE,(void*)&ref) || !Reference_get(ref,act))
      return FunctionPublish_returnError(pstate,"1 l3dnode 1 matrix4x4 required");
    mat = Matrix44Interface_getElements(act->link.matrixIF,matrix);
    break;
  case LUXI_CLASS_SCENENODE:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SCENENODE,(void*)&ref) || !Reference_get(ref,scn))
      return FunctionPublish_returnError(pstate,"1 l3dnode 1 matrix4x4 required");
    mat = scn->link.matrix;
    break;
  }

  lxMatrix44CopySIMD(l3d->finalMatrix,mat);
  List3DNode_markUpdate(l3d);

  return 0;
}


static int PubList3D_color(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  int mid;
  float *flt;


  if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_NODE,(void*)&ref) || !Reference_get(ref,l3d) || !l3d->numDrawNodes)
    return FunctionPublish_returnError(pstate,"1 renderable l3dnode required, optional 4 floats");

  mid = -1;
  if (l3d->nodeType == LUXI_CLASS_L3D_MODEL && (n==2 || n==6) && (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MESHID,(void*)&mid) || !SUBRESID_CHECK(mid,l3d->checkRID)))
    return FunctionPublish_returnError(pstate,"1 l3dmodel 1 meshid required");

  mid = mid<0 ? mid : SUBRESID_MAKEPURE(mid);

  flt = l3d->drawNodes[(mid<0 ? 0:mid)].draw.color;
  if (n>2){
    if (FunctionPublish_getArgOffset(pstate,mid<0 ? 1 : 2,4,FNPUB_TOVECTOR4(flt))!=4)
      return FunctionPublish_returnError(pstate,"4 floats required");
    if (l3d->nodeType == LUXI_CLASS_L3D_MODEL && mid<0){
      for (mid = 1; mid < l3d->numDrawNodes; mid++)
        lxVector4Copy(l3d->drawNodes[mid].draw.color,flt);
    }

    return 0;
  }

  return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(flt));
}

static int PubList3D_visflag(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  int id;
  int visflag;

  byte state;

  if ((n<2) || FunctionPublish_getArg(pstate,2,LUXI_CLASS_L3D_NODE,(void*)&ref,LUXI_CLASS_INT,(void*)&id)!=2 || !Reference_get(ref,l3d) ||
    id > 31 || id < 0)
    return FunctionPublish_returnError(pstate,"1 l3dnode 1 int 0-31 required, optional 1 boolean");

  visflag = l3d->visflag;
  if (n>2){
    if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 l3dnode 1 int 0-31 required, optional 1 boolean");

    if (state)
      visflag |= 1<<id;
    else
      visflag &= ~(1<<id);

    List3DNode_setVisflag(l3d,visflag);

    return 0;
  }

  return FunctionPublish_returnBool(pstate,(visflag & (1<<id))!=0);
}



// L3D_PRIMITIVE
static int PubL3DPrimitive_new(PState pstate,PubFunction_t *f, int n)
{
  char *name;
  float size[3];
  int layer = List3D_getDefaultLayer();
  int read;
  List3DNode_t *l3d;
  List3DPrimitiveType_t type;

  if (n<2 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name) ||
    (read=FunctionPublish_getArgOffset(pstate,1+FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID,(void*)&layer),3,FNPUB_TOVECTOR3(size)))<1 )
    return FunctionPublish_returnError(pstate,"1 string [1 l3dlayerid] 1/3 float required");

  if (read==1) (size[1]=size[0],size[2]=size[0]);
  if (read==2) (size[2] = 1);

  type = (List3DPrimitiveType_t)f->upvalue;

  l3d = List3DNode_newPrimitive(name,layer,type,size);
  if (!l3d)
    return 0;

  Reference_makeVolatile(l3d->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_PRIMITIVE,REF2VOID(l3d->reference));
}

static int PubL3DPrimitive_material(PState pstate,PubFunction_t *fn, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  int index;

  if ((n<1) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_PRIMITIVE,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dprimitive required, optional 1 matsurface");

  if (n==1){
    if (vidMaterial(l3d->drawNodes->draw.matRID))
      return FunctionPublish_returnType(pstate,LUXI_CLASS_MATERIAL,REF2VOID(l3d->drawNodes->draw.matRID));
    else if (l3d->drawNodes->draw.matRID >= 0)
      return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,REF2VOID(l3d->drawNodes->draw.matRID));
    else
      return 0;
  }
  if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATSURF,(void*)&index))
    return FunctionPublish_returnError(pstate,"1 l3dprimitive required, optional 1 matsurface");
  List3DNode_Model_meshMaterial(l3d,0,index);
  return 0;
}

static int PubL3DPrimitive_size(PState pstate,PubFunction_t *fn, int n)
{
  lxBoundingBoxCenter_t ctrbox;
  Reference ref;
  List3DNode_t *l3d;
  float f[3];
  int read;

  if ((n<1) || (read=FunctionPublish_getArg(pstate,4,LUXI_CLASS_L3D_PRIMITIVE,(void*)&ref,FNPUB_TOVECTOR3(f)))<1
    || !Reference_get(ref,l3d) )
    return FunctionPublish_returnError(pstate,"1 l3dprimitive required, optional 1/3 floats");

  if (n==4 && read>=4) {
    lxVector3Clear(ctrbox.center);
    l3d->primitive->size[0] = f[0];
    l3d->primitive->size[1] = f[1];
    l3d->primitive->size[2] = f[2];
    ctrbox.size[0]=f[0];
    ctrbox.size[1]=f[1];
    ctrbox.size[2]=f[2];

    if (l3d->drawNodes->draw.mesh == l3d->primitive->orgmesh)
    switch (l3d->primitive->type) {
    case L3DPRIMITIVE_CYLINDER:
      ctrbox.size[0] *= 2;
      ctrbox.size[1] *= 2;
      break;
    case L3DPRIMITIVE_SPHERE:
      ctrbox.size[0] *= 2;
      ctrbox.size[1] *= 2;
      ctrbox.size[2] *= 2;
    default:
      break;
    }
    lxBoundingBox_fromCenterBox(&l3d->primitive->bbox,&ctrbox);
    return 0;
  }
  return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(l3d->primitive->size));
}




enum PubL3DPrimCmd_e{
  PL3DPRIM_USERMESH,
  PL3DPRIM_ORGMESH,
  PL3DPRIM_RENDERMESH,
};

static int PubL3DPrimitive_func(PState pstate,PubFunction_t *fn, int n)
{
  Reference ref;
  List3DNode_t *l3d;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_PRIMITIVE,(void*)&ref) ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dprimitive required");

  switch( (int) fn->upvalue)
  {
  case PL3DPRIM_USERMESH:
    return PubUserMesh(pstate,1,&l3d->drawNodes->draw.mesh,
      &l3d->primitive->usermesh);

  case PL3DPRIM_RENDERMESH:
    return PubRenderMesh(pstate,1,&l3d->drawNodes->draw.mesh,
      &l3d->primitive->usermesh);

  case PL3DPRIM_ORGMESH:
    Reference_releaseClear(l3d->primitive->usermesh);
    l3d->drawNodes->draw.mesh = l3d->primitive->orgmesh;
    break;
  }

  return 0;
}
//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_L3D_TEXT

static int PubL3DText_new (PState pstate,PubFunction_t *f, int n)
{
  char *name;
  char *text;
  List3DNode_t *l3d;
  Reference ref;
  int layer,read;

  layer = List3D_getDefaultLayer();

  if (n<2 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name) ||
    (read=FunctionPublish_getArgOffset(pstate,1+FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID,(void*)&layer),2,LUXI_CLASS_STRING,(void*)&text,LUXI_CLASS_FONTSET,(void*)&ref))<1 )
    return FunctionPublish_returnError(pstate,"1 string [1 l3dlayerid] 1 string [1 fontset] required");

  l3d = List3DNode_newText(name,layer,text);

  if (read > 1 && Reference_isValid(ref)){
    PText_setFontSetRef(l3d->ptext,ref);
  }

  Reference_makeVolatile(l3d->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_TEXT,REF2VOID(l3d->reference));
}

enum PubL3DTextCmds_e
{
  PL3DTEXT_FS,
  PL3DTEXT_TEX,
  PL3DTEXT_SPACING,
  PL3DTEXT_TABWIDTH,
  PL3DTEXT_SIZE,
  PL3DTEXT_TEXT,
  PL3DTEXT_DIM,
  PL3DTEXT_CHARAT,
  PL3DTEXT_POSAT,
};

static int PubL3DText_attributes (PState pstate,PubFunction_t *f, int n)
{
  char *name;
  Reference ref;
  List3DNode_t *l3d;
  lxVector3 vec;

  if ((n == 0)|| !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_TEXT,(void*)&ref) ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dtext required");

  switch((int)f->upvalue) {
  case PL3DTEXT_FS: // fontset
    if (n==1) return FunctionPublish_returnType(pstate,LUXI_CLASS_FONTSET,REF2VOID(l3d->ptext->fontsetref));
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FONTSET,(void*)&ref) || !Reference_isValid(ref))
      return FunctionPublish_returnError(pstate,"1 l3dtext required, optional 1 fontset");
    PText_setFontSetRef(l3d->ptext,ref);
    break;
  case PL3DTEXT_TEX:  // texture
    if (n==1) return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)l3d->ptext->texRID);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_TEXTURE,(void*)&l3d->ptext->texRID))
      return FunctionPublish_returnError(pstate,"1 l3dtext required, optional 1 texture");
    l3d->drawNodes->draw.matRID = l3d->ptext->texRID;
    DrawNode_updateSortID(l3d->drawNodes,DRAW_SORT_TYPE_NORMAL);
    break;
  case PL3DTEXT_TABWIDTH: // tabwidth
    if (n==1) return FunctionPublish_returnFloat(pstate,l3d->ptext->tabwidth);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(l3d->ptext->tabwidth)))
      return FunctionPublish_returnError(pstate,"1 l3dtext required, optional 1 float");
    break;
  case PL3DTEXT_SPACING:  // spacing
    if (n==1) return FunctionPublish_returnFloat(pstate,l3d->ptext->width);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(l3d->ptext->width)))
      return FunctionPublish_returnError(pstate,"1 l3dtext required, optional 1 float");
    break;
  case PL3DTEXT_SIZE: // size
    if (n==1) return FunctionPublish_returnFloat(pstate,l3d->ptext->size);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(l3d->ptext->size)))
      return FunctionPublish_returnError(pstate,"1 l3dtext required, optional 1 float");
    break;
  case PL3DTEXT_TEXT: // text
    if (n==1)
      return FunctionPublish_returnString(pstate,l3d->ptext->text);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&name))
      return FunctionPublish_returnError(pstate,"1 l2dtext required, optional 1 string");
    PText_setText(l3d->ptext,name);
    break;
  case PL3DTEXT_DIM: // dimension
    PText_getDimensions(l3d->ptext,NULL,vec);
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vec));
  case PL3DTEXT_CHARAT: {
    int suc,pos;
    FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_FLOAT,vec[1]);
    FNPUB_CHECKOUT(pstate,n,2,LUXI_CLASS_FLOAT,vec[2]);
    suc = PText_getCharindexAt(l3d->ptext,vec[1], vec[2], &pos);
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_BOOLEAN,suc,LUXI_CLASS_INT,pos);
              }
  case PL3DTEXT_POSAT: {
    int pos;
    FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_INT,pos);
    PText_getCharPoint(l3d->ptext, pos,vec);
    return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR3(vec));
             }
  default:
    break;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_L3D_SHADOWMODEL

static int PubL3DShadowModel_new(PState pstate,PubFunction_t *f,int n)
{
  Reference ref;
  Reference ref2;
  List3DNode_t *l3d;
  List3DNode_t *l3dlight;
  char *name;
  int layer;
  char *meshname;

  layer = List3D_getDefaultLayer();
  meshname = NULL;
  if (n<3 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name) ||
    FunctionPublish_getArgOffset(pstate,1+FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID,(void*)&layer),3,LUXI_CLASS_L3D_LIGHT,(void*)&ref,LUXI_CLASS_L3D_NODE,(void*)&ref2,LUXI_CLASS_STRING,(void*)&meshname)<2
    || !Reference_get(ref,l3dlight) || !Reference_get(ref2,l3d))
    return FunctionPublish_returnError(pstate,"1 string [1 l3dlayerid] 1 l3dlight 1 l3dprimitive/l3dmodel [1 string] required");

  l3d = List3DNode_newShadowModel(name,layer,l3d,l3dlight,meshname);

  if (!l3d)
    return 0;

  Reference_makeVolatile(l3d->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_SHADOWMODEL,REF2VOID(l3d->reference));
}

enum PubL3DShadowModelCmd_e{
  PSMDL_LIGHT,
  PSMDL_REFVALID,
  PSMDL_LIGHTVALID,
  PSMDL_EXTRUSION,
};

static int PubL3DShadowModel_funcs(PState pstate,PubFunction_t *f,int n)
{
  Reference ref;
  List3DNode_t *l3d;

  if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_SHADOWMODEL,(void*)&ref) ||
    !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dshadowmodel required");

  switch((int)f->upvalue)
  {
  case PSMDL_LIGHT:
    if (n==1) return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_LIGHT,REF2VOID(l3d->smdl->lightref));
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LIGHT,(void*)&ref) || !Reference_isValid(ref))
      return FunctionPublish_returnError(pstate,"1 l3dshadowmodel 1 l3dlight required");

    Reference_releaseWeakCheck(l3d->smdl->lightref);
    Reference_refWeak(ref);

    l3d->smdl->lightref = ref;
    break;
  case PSMDL_REFVALID:
    return FunctionPublish_returnBool(pstate,Reference_isValid(l3d->smdl->targetref));
    break;
  case PSMDL_LIGHTVALID:
    return FunctionPublish_returnBool(pstate,Reference_isValid(l3d->smdl->lightref));
    break;
  case PSMDL_EXTRUSION:
    if (n==1) return FunctionPublish_returnFloat(pstate,l3d->smdl->extrusiondistance);
    else if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(l3d->smdl->extrusiondistance)))
      return FunctionPublish_returnError(pstate,"1 l3dshadowmodel 1 float required");
    //ShadowModel_updateExtrusionLength(l3d->smdl);
    break;
  default:
    break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_L3D_LEVELMODEL



static int PubL3DLevelModel_new(PState pstate,PubFunction_t *f,int n)
{
  int idx;
  int dimension[3];
  int minpolys = 128;
  List3DNode_t *l3d;
  char *name;
  float margin = 0.0f;
  int layer = List3D_getDefaultLayer();

  if (n<5 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name) ||
    FunctionPublish_getArgOffset(pstate,1+FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID,(void*)&layer),6,LUXI_CLASS_MODEL,(void*)&idx,LUXI_CLASS_INT,(void*)&dimension[0],LUXI_CLASS_INT,(void*)&dimension[1],LUXI_CLASS_INT,(void*)&dimension[2],LUXI_CLASS_INT,(void*)&minpolys,LUXI_CLASS_FLOAT,(void*)&margin)<4
    || dimension[0]<1 || dimension[1]<1 || dimension[2]<1)
    return FunctionPublish_returnError(pstate,"1 string [1 l3dlayerid] 1 model 3 int (>0) [1 int] required");

  l3d = List3DNode_newLevelModel(name,layer,idx,dimension,minpolys,margin);

  if (!l3d)
    return 0;

  Reference_makeVolatile(l3d->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_LEVELMODEL,REF2VOID(l3d->reference));
}

enum PubLevelModelCmds_e{
  PLMDL_LIGHTMAPCNT,
  PLMDL_LIGHTMAP,
};

static int PubL3DLevelModel_attr(PState pstate,PubFunction_t *f, int n)
{
  List3DNode_t *l3d;
  Reference ref;
  int id;

  if ( n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_LEVELMODEL,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dlevelmodel required");

  switch((int)f->upvalue)
  {
  case PLMDL_LIGHTMAPCNT:
    return FunctionPublish_returnInt(pstate,l3d->lmdl->numLightmapRIDs);
  case PLMDL_LIGHTMAP:
    if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&id) || id < 0 || id >= l3d->lmdl->numLightmapRIDs)
      return FunctionPublish_returnError(pstate,"1 l3dlevelmodel 1 int (>0, <lightmapcount) required");
    return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)l3d->lmdl->lightmapRIDs[id]);
  default:
    break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////

void PubClass_List3D_init()
{
  FunctionPublish_initClass(LUXI_CLASS_L3D_LIST,"l3dlist",
    "The List3D is the main rendering list, it contains l3dnodes that represent visual items.\
    It is organised in l3dsets l3dlayerids and l3dviews. l3dviews can render l2dnodes thru\
    special commands as well.<br><br>\
    The l3dlist the buffermempool for per-frame results. Various limits such as l3dset,\
    layer and perlayer draws influence its useage. You can alter some of these limits and the poolsize at runtime.\
    However these are critical operations and should only be looked into if you exceed limits, or had\
    quits with error messages regarding buffermempool.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIST,PubL3D_funcs,(void*)PL3D_FBOTEST,"fbotest","([int stringtype, string message]):([l3dview]) - performs a test run on current rcmdfbo related setups. For testing l3dview:drawnow you can pass the view directly. It takes enable flags of rcmds into account. Returns an error (+1) or warning(-1) string, or nothing if all is working.");
  FunctionPublish_setFunctionInherition(LUXI_CLASS_L3D_LIST,"fbotest",LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIST,PubL3D_funcs,(void*)PL3D_MAXSETS,"l3dsetcount",
    "(int):() - returns how many l3dsets exist");
  FunctionPublish_setFunctionInherition(LUXI_CLASS_L3D_LIST,"l3dsetcount",LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIST,PubL3D_funcs,(void*)PL3D_MAXLAYERS,"l3dlayercount",
    "(int):() - returns how many l3dlayers exist");
  FunctionPublish_setFunctionInherition(LUXI_CLASS_L3D_LIST,"l3dlayercount",LUX_FALSE);

  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIST,PubL3D_funcs,(void*)PL3D_PERLAYER,"maxdrawsperlayer",
    "([int]):([int]) - gets or sets how many meshes per l3dlayer in total can ever be rendered.\
    Changing this value influences runtime limits and rendermempool consumption (8 bytes per l3dset*l3dlayer).");
  FunctionPublish_setFunctionInherition(LUXI_CLASS_L3D_LIST,"maxdrawsperlayer",LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LIST,PubL3D_funcs,(void*)PL3D_TOTALPROJ,"maxtotalprojectors",
    "([int]):([int]) - gets or sets how many total projectors per-frame can be active. Say you have two meshes\
    that each are affected by the same projector, this would result into two total projectors. It is\
    the sum of used projectors per mesh, over all meshes. Changing this value influences runtime limits\
    and rendermempool consumption (4 bytes per cnt).");
  FunctionPublish_setFunctionInherition(LUXI_CLASS_L3D_LIST,"maxtotalprojectors",LUX_FALSE);

  FunctionPublish_initClass(LUXI_CLASS_L3D_NODE,"l3dnode",
"l3dnodes of different classes represent any visual item\
  you want to draw, models, particles etc. Each l3dnode can have a subtree of l3dnodes, however visibilty is only\
  tested for the root l3dnode, so if the root is not visible, childs won't be either. For a l3dnode to become active\
  it must be linked with a actornode or scenenode, from which it will get its world position information.<br>\
  for more info on the order nodes are rendered see l3dset.<br>\
  Retrieving world positions and rotations can return wrong results. Because world updates are only done when the node\
  was visible. So they lag one frame behind. Alternatively you can use updateall to enforce uptodate world data."
    ,NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_NODE,LUXI_CLASS_L3D_LIST);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubL3D_updateall,NULL,"updateall","():() - updates world positions of all l3d nodes, visible or not. Useful if you need that data after you created and linked l3ds.");
  FunctionPublish_setFunctionInherition(LUXI_CLASS_L3D_NODE,"updateall",LUX_FALSE);

  // inherited l3d funcs
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_layer,NULL,"setlayer",
    "():(l3dnode,l3dlayerid,[meshid]) - sets the nodes layer/set. l3dmodels can pass meshid. An l3dset change always affects all");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_free,NULL,"delete",
    "():(l3dnode node) - deletes l3dnode, by default children are also deleted");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_name,NULL,"name",
    "(string name):(l3dnode node) - returns name");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_linkInterfaces,NULL,"linkinterface",
    "([scenenode]/[actornode]):(l3dnode node,[scenenode]/[actornode]) - links l3dnode to interfaces of scenenode or actornode (or returns current)");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_lightmap,(void*)0,"lightmap",
    "([texture]):(l3dnode node,[texture]) - sets or returns node's lightmap, if not a texture is passed as argument, it will be disabled.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_lightmap,(void*)1,"lightmaptexmatrix",
    "([matrix4x4]):(l3dnode node,[matrix4x4]) - sets or returns node's lightmap texmatrix, if not a matrix4x4 is passed as argument, it will be disabled.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_link,NULL,"parent",
    "([l3dnode parent]):(l3dnode node,[l3dnode parent]) - links node to a parent. ==parent prevents gc of self");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_unlink,NULL,"unlink",
    "():(l3dnode node) - fully unlinks l3dnode, wont be drawn until linked again");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_linkParentBone,NULL,"parentbone",
    "():(l3dnode node,string bonename) - tries to link node to parent's bonesystem");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_unlinkParentBone,NULL,"unparentbone",
    "():(l3dnode node) - unlinks l3dnode from bone");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_inheritLockRot,NULL,"rotationlock",
    "(boolean):(l3dnode node,int axis,[boolean state]) - returns or sets rotation lock. axis 0=X 1=Y 2=Z 3=All. Rotation lock is applied before local transforms.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_visvolume,(void*)1,"cambox",
    "():(l3dnode node,boolean inside,float outer min x,y,z,float outer max x,y,z,[float inner min x,y,z, float inner max x,y,z]) - sets visibility box, if camera is inside (inside set to true) / outside (inside set to false) the box it will be drawn. If you pass the second box,which must be smaller than the first, we will perform a ranged check. This means the node will be marked as inside if camera is within outer box, and outside of innerbox.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_visvolume,(void*)0,"camsphere",
    "():(l3dnode node,boolean inside,float outer range, [float inner range]) - sets visibilty sphere, if camera is inside (inside set to true) / outside (inside set to false) the sphere it will be drawn. If you pass the inner range we will perform a ranged check. This means the node will be marked as inside if camera is within outer sphere, and outside of inner sphere.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_visvolume,(void*)2,"camvolumedelete",
    "():(l3dnode node) - removes the extra visibility test based on camera position");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_states,(void*)PL3D_LASTFRAME,"lastframe",
    "([int]):(l3dnode node) - returns the last frame the node was rendered in.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_color,NULL,"color",
    "([float r,g,b,a]):(l3dnode node,[meshid],[float r,g,b,a]) - returns or sets node's color. Only used when rfNovertexcolor is set. Optionally can pass meshid for l3dmodels");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_states,(void*)PL3D_USELOCAL,"uselocal",
    "([boolean]):(l3dnode node,[boolean]) - enables local matrices, automatically on when you link to another l3dnode. You must not call it before the l3dnode was linked.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_states,(void*)PL3D_USEMANUAL,"usemanualworld",
    "([boolean]):(l3dnode node,[boolean]) - enables manual world matrices. Bonelinking, inheritlocks and local matrices will be disabled. You must not call it before the l3dnode was linked.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_renderScale,NULL,"renderscale",
    "([float x,y,z]):(l3dnode node,[float x,y,z]) - returns or sets render scale factor. (1,1,1) will disable renderscale, which is cheaper to render. For l3dpemitters it applies scaling to subsystem offsets. For l3dprimitives this matches size on start, however setting renderscale overrides size, and size changes will only affect bbox.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_matrix,NULL,"localmatrix",
    "([matrix4x4]):(l3dnode node,[matrix4x4]) - returns or sets local matrix. As visibility culling is based on the scene/actornode using too much localposition offsets can create problems. You should rather create new actor/scenenodes in that case.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_finalmatrix,NULL,"worldmatrix",
    "([matrix4x4]):(l3dnode node,[matrix4x4/ actornode / scenenode]) - returns or sets world matrix. Setting only allowed if 'usemanualworld' is true.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_localPos,NULL,"localpos",
    "([float x,y,z]):(l3dnode node,[float x,y,z]) - returns or sets local position. As visibility culling is based on the scene/actornode using too much localposition offsets can create problems. You should rather create new actor/scenenodes in that case.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_localRot,(void*)ROT_DEG,"localrotdeg",
    "([float x,y,z]):(l3dnode node,[float x,y,z]) - returns or sets local rotation in degrees");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_localRot,(void*)ROT_RAD,"localrotrad",
    "([float x,y,z]):(l3dnode node,[float x,y,z]) - returns or sets local rotation in radians");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_localRot,(void*)ROT_QUAT,"localrotquat",
    "([float x,y,z,w]):(l3dnode node,[float x,y,z,w]) - returns or sets local rotation as quaternion");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_localRot,(void*)ROT_AXIS,"localrotaxis",
    "([float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]):(l3dnode node,[float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]) - returns or sets local rotation axis, make sure they make a orthogonal basis.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_finalPos,NULL,"worldpos",
    "([float x,y,z]):(l3dnode node,[float x,y,z]) - returns or sets last world position. Setting only allowed if 'usemanualworld' is true.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_finalRot,(void*)ROT_RAD,"worldrotrad",
    "([float x,y,z]):(l3dnode node,[float x,y,z]) - returns or sets last world rotation in radians. Setting only allowed if 'usemanualworld' is true.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_finalRot,(void*)ROT_DEG,"worldrotdeg",
    "([float x,y,z]):(l3dnode node,[float x,y,z]) - returns or sets last world rotation in degrees. Setting only allowed if 'usemanualworld' is true.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_finalRot,(void*)ROT_QUAT,"worldrotquat",
    "([float x,y,z,w]):(l3dnode node,[float x,y,z,w]) - returns or sets local rotation as quaternion. Setting only allowed if 'usemanualworld' is true.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_finalRot,(void*)ROT_AXIS,"worldrotaxis",
    "([float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]):(l3dnode node,[float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]) - returns or sets last world rotation axis, make sure they make a orthogonal basis. Setting only allowed if 'usemanualworld' is true.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_animateable,NULL,"isanimateable",
    "(boolean):(l3dnode) - checks if the l3dnode is a l3dmodel and if yes, if it can be animated. Won't throw an error if a valid l3d is passed.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_states,(void*)PL3D_NOVISTEST,"novistest",
    "([boolean]):(l3dnode,[boolean]) - gets or sets novistest state. If set to true, node will always be drawn, as well as all its children.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_visflag,NULL,"visflag",
    "([boolean]):(l3dnode,int id 0-31,[boolean]) - gets or sets visiblity status for each id. If a camera's id is set to true, the l3dnode will be visible to it. By default all are visible to id 0, which is the default camera.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_NODE,PubList3D_states,(void*)PL3D_USELOOKAT,"uselookat",
    "([boolean]):(l3dnode,[boolean]) - gets or sets if node should face camera. Local updates are done post align.");


  FunctionPublish_initClass(LUXI_CLASS_L3D_PRIMITIVE,"l3dprimitive","Primitives such as cubes, boxes and user meshes.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_PRIMITIVE,LUXI_CLASS_L3D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PRIMITIVE,PubL3DPrimitive_new,(void*)L3DPRIMITIVE_CUBE,"newbox",
    "([l3dprimitive]):(string name,|l3dlayerid|,float w,d,h) - creates new l3dnode");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PRIMITIVE,PubL3DPrimitive_new,(void*)L3DPRIMITIVE_SPHERE,"newsphere",
    "([l3dprimitive]):(string name,|l3dlayerid|,float size) - creates new l3dnode");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PRIMITIVE,PubL3DPrimitive_new,(void*)L3DPRIMITIVE_CYLINDER,"newcylinder",
    "([l3dprimitive]):(string name,|l3dlayerid|,float size) - creates new l3dnode");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PRIMITIVE,PubL3DPrimitive_new,(void*)L3DPRIMITIVE_QUAD,"newquad",
    "([l3dprimitive]):(string name,|l3dlayerid|,float w,d) - creates new l3dnode. origin of the quad is lower left corner");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PRIMITIVE,PubL3DPrimitive_new,(void*)L3DPRIMITIVE_QUAD_CENTERED,"newquadcentered",
    "([l3dprimitive]):(string name,|l3dlayerid|,float w,d) - creates new l3dnode. origin of the quad is center point");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PRIMITIVE,PubL3DPrimitive_func,(void*)PL3DPRIM_ORGMESH,"originalmesh",
    "():(l3dprimitive)  - deletes the usermesh and uses the original mesh again.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PRIMITIVE,PubL3DPrimitive_func,(void*)PL3DPRIM_USERMESH,"usermesh",
        "():(l3dprimitive, vertextype, int numverts, numindices, [vidbuffer vbo], [int vbooffset], [vidbuffer ibo], [int ibooffset]) - creates inplace custom rendermesh (see rendermesh for details) Note that polygon winding is CCW.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PRIMITIVE,PubL3DPrimitive_func,(void*)PL3DPRIM_RENDERMESH,
    "rendermesh","([rendermesh]):(l3dprimitive,[rendermesh]) - gets or sets rendermesh. Get only works if a usermesh was created before or another rendermesh passed for useage.");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_PRIMITIVE,PubL3DPrimitive_material,NULL,"matsurface",
    "([matsurface]):(l3dprimitive,[matsurface]) - returns or sets material");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PRIMITIVE,PubL3DPrimitive_size,NULL,"size",
    "([float radius/float x,y,z]):(l3dprimitive,[float radius/float x,y,z]) - returns or sets size (modifies l3d's visbbox as well). Visual size will be overriden by calls to renderscale.");

  FunctionPublish_initClass(LUXI_CLASS_L3D_LEVELMODEL,"l3dlevelmodel","A model that is very large and contains static geometry. \
The model will be split into a subdivided visibility cubes. Meshes that contain 'lightmap_' in their texture string will get \
those auto-loaded and applied.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_LEVELMODEL,LUXI_CLASS_L3D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LEVELMODEL,PubL3DLevelModel_new,(void*)NULL,"new",
    "([l3dlevelmodel]):(string name,|l3dlayerid|,model,int separatorsx,y,z, [int minpolys],[float mergemargin]) - creates new l3dlevelmodel. Visibility cubes are generated with the given separations (>=1).");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LEVELMODEL,PubL3DLevelModel_attr,(void*)PLMDL_LIGHTMAPCNT,"lightmapcount",
    "(int):(l3dlevelmodel) - returns how many lightmaps are used.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_LEVELMODEL,PubL3DLevelModel_attr,(void*)PLMDL_LIGHTMAP,"lightmap",
    "(texture):(l3dlevelmodel,int i) - returns ith lightmap.");

  FunctionPublish_initClass(LUXI_CLASS_L3D_TEXT,"l3dtext","A text node",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_TEXT,LUXI_CLASS_L3D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TEXT,PubL3DText_new,NULL,"new",
    "([l3dtext]):(string name,|l3dlayerid|,string text,[fontset]) - creates new l3dtext");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TEXT,PubL3DText_attributes,(void*)PL3DTEXT_FS,"fontset",
    "([fontset]):(l3dtext,[fontset])  - returns or sets fontset.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TEXT,PubL3DText_attributes,(void*)PL3DTEXT_TEX,"font",
    "([texture]):(l3dtext,[texture])  - returns or sets font texture");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TEXT,PubL3DText_attributes,(void*)PL3DTEXT_SPACING,"spacing",
    "([float]):(l3dtext,[float])  - returns or sets font spacing, default is 16");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TEXT,PubL3DText_attributes,(void*)PL3DTEXT_SIZE,"size",
    "([float]):(l3dtext,[float])  - returns or sets font size, default is 16");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TEXT,PubL3DText_attributes,(void*)PL3DTEXT_TABWIDTH,"tabwidth",
    "(float):(l3dtext,[float])  - returns or sets tab width spacing, default is 0. 0 means spacing * 4 is used, otherwise values will be directly applied.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TEXT,PubL3DText_attributes,(void*)PL3DTEXT_TEXT,"text",
    "([string]):(l3dtext,[string])  - returns or sets text");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TEXT,PubL3DText_attributes,(void*)PL3DTEXT_DIM,"dimensions",
    "(float x,y,z):(l3dtext)  - returns dimension of space it would take when printed");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TEXT,PubL3DText_attributes,(void*)PL3DTEXT_CHARAT,"charatpos",
    "(boolean inside, int charpos):(l3dtext, float x, float y)  - returns character index at given x,y. Returns true / false wether it x,y was inside or not.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TEXT,PubL3DText_attributes,(void*)PL3DTEXT_POSAT,"posatchar",
    "(float x,y):(l3dtext, int pos)  - returns the character's position offset. Behaviour undefined if pos>length of text.");


  FunctionPublish_initClass(LUXI_CLASS_L3D_SHADOWMODEL,"l3dshadowmodel","A volume model for stencil shadows. It takes meshdata from the original model, and renders only into the stencil buffer for the given light. Models will takeover bonelinks and skinobjects. You must use camera infinitebackplane in the view that contains the shadowmodels.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_SHADOWMODEL,LUXI_CLASS_L3D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_SHADOWMODEL,PubL3DShadowModel_new,NULL,"new",
    "([l3dshadowmodel]):(string name,|l3dlayerid|,l3dlight,l3dprimitive/l3dmodel,[string meshname]) -\
 creates a new l3dshadowmodel for the given nodes. Can return nil, if sourcemesh is too complex or not closed. When a l3dmodel is used you can specify a substring that is searched for in the l3dmodel's meshes.\
 That way you can mark some meshes with nodraw/neverdraw in the original model, but use them as shadowmeshes and they still use the original animation/skin data.\
 However when the original model is not visible, the animation data is not further updated. For l3dprimitives have in mind that you must not change the mesh after the shadowmodel was created.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_SHADOWMODEL,PubL3DShadowModel_funcs,(void*)PSMDL_LIGHT,"light",
    "([l3dlight]):(l3dshadowmodel,[l3dlight])  - returns or sets the light that is used. The light should be activated or sun, else its position is not updated.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_SHADOWMODEL,PubL3DShadowModel_funcs,(void*)PSMDL_REFVALID,"istargetvalid",
    "(boolean):(l3dshadowmodel)  - is target model/primitive still valid, if not you should delete the node.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_SHADOWMODEL,PubL3DShadowModel_funcs,(void*)PSMDL_LIGHTVALID,"islightvalid",
    "(boolean):(l3dshadowmodel)  - is light still valid, if not you should delete the node, or use another light");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_SHADOWMODEL,PubL3DShadowModel_funcs,(void*)PSMDL_EXTRUSION,"extrusionlength",
    "([float]):(l3dshadowmodel,[float])  - returns or sets volume extrusion length. A value of 0 (default) means infinite length.");
}

