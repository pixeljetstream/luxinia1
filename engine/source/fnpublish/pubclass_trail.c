// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../resource/resmanager.h"
#include "../render/gl_list3d.h"
#include "../common/3dmath.h"
#include "../scene/actorlist.h"
#include "../scene/scenetree.h"


// Published here:
// LUXI_CLASS_L3D_TRAIL


static int PubTrail_new (PState pstate,PubFunction_t *fn, int n)
{
  List3DNode_t *l3d;
  char *name;
  int points;
  int layer;

  layer = List3D_getDefaultLayer();


  if (n<2 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void**)(&name)) ||
    !FunctionPublish_getArgOffset(pstate,1+FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID,(void**)&layer),1,LUXI_CLASS_INT,(void**)&points) || points < 1)
    return FunctionPublish_returnError(pstate,"1 string and [1 l3dlayerid] 1 int  required");

  l3d = List3DNode_newTrail(name,layer,points);

  Reference_makeVolatile(l3d->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_TRAIL,REF2VOID(l3d->reference));
}

static int PubTrail_spawn (PState pstate,PubFunction_t *fn, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  lxVector3 pos,vel,normal;
  TrailPoint_t *tpoint;

  lxVector3Clear(vel);
  if ((n<7) ||
    FunctionPublish_getArg(pstate,10,LUXI_CLASS_L3D_TRAIL,(void*)&ref,FNPUB_TOVECTOR3(pos),FNPUB_TOVECTOR3(normal),FNPUB_TOVECTOR3(vel))<7
    || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dtrail 6 floats required, optional 3 floats");

  tpoint = Trail_addPoint(l3d->trail,pos,normal,vel);

  return FunctionPublish_returnType(pstate,LUXI_CLASS_TRAILPOINT,tpoint);

}

static int PubTrail_spawnref(PState pstate,PubFunction_t *f, int n){
  Reference ref;
  Reference refsn;
  Reference refact;
  List3DNode_t *l3d;
  TrailPoint_t *tpoint;
  int axis;
  SceneNode_t* s3d;
  ActorNode_t* act;
  lxVector3 dummy;

  Reference_reset(refsn);
  Reference_reset(refact);
  if ((n!=3) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_TRAIL,(void*)&ref) ||
    !(  FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SCENENODE,(void*)&refsn) ||
    FunctionPublish_getNArg(pstate,1,LUXI_CLASS_ACTORNODE,(void*)&refact)) ||
    !FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&axis) ||
    !Reference_get(ref,l3d) || axis < 0 || axis > 2)
    return FunctionPublish_returnError(pstate,"1 valid l3dtrail 1 valid actor/scenenode 1 int(0-2) required");

  if (l3d->trail->type != TRAIL_WORLD_REF){
    return FunctionPublish_returnError(pstate,"l3dtrail must be of typeworldref");
  }

  if (Reference_get(refsn,s3d)){
    ref = refsn;
  }
  else if (Reference_get(refact,act)) {
    ref = refact;
  } else
    return FunctionPublish_returnError(pstate,"1 valid l3dpgroup 1 valid actor/scenenode 1 int required");


  tpoint = Trail_addPoint(l3d->trail,dummy,dummy,dummy);

  if (tpoint){

    Reference_refWeak(ref);
    tpoint->linkRef = ref;
    if (act)
      tpoint->linkType = LINK_ACT;
    else
      tpoint->linkType = LINK_S3D;

    tpoint->axis = axis;
    return FunctionPublish_returnType(pstate,LUXI_CLASS_TRAILPOINT,tpoint);
  }
  else
    return 0;

  return 0;
}

enum PubTrailPtCmds_e
{
  PTPT_POS,
  PTPT_NORMAL,
  PTPT_VEL,
  PTPT_INDEX,
};


static int PubTrail_point (PState pstate,PubFunction_t *fn, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  TrailPoint_t *tpoint;

  if ((n<2) ||
    FunctionPublish_getArg(pstate,2,LUXI_CLASS_L3D_TRAIL,(void*)&ref,LUXI_CLASS_TRAILPOINT,(void*)&tpoint)!=2
    || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dtrail 1 trailpoint required");

  if (!Trail_checkPoint(l3d->trail,tpoint))
    return FunctionPublish_returnError(pstate,"1 l3dtrail 1 valid trailpoint required");

  if ((int)fn->upvalue < 4 && l3d->trail->type == TRAIL_WORLD_REF)
    return FunctionPublish_returnError(pstate,"change illegal for typeworldref");

  switch((int)fn->upvalue) {
  case PTPT_POS:  // pos
    if (n==2) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(tpoint->pos));
    else if (FunctionPublish_getArgOffset(pstate,2,3,FNPUB_TOVECTOR3(tpoint->pos))!=3)
      return FunctionPublish_returnError(pstate,"3 floats required");
    break;
  case PTPT_NORMAL: // normal
    if (n==2) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(tpoint->normal));
    else if (FunctionPublish_getArgOffset(pstate,2,3,FNPUB_TOVECTOR3(tpoint->normal))!=3)
      return FunctionPublish_returnError(pstate,"3 floats required");
    break;
  case PTPT_VEL:  // vel
    if (n==2) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(tpoint->vel));
    else if (FunctionPublish_getArgOffset(pstate,2,3,FNPUB_TOVECTOR3(tpoint->vel))!=3)
      return FunctionPublish_returnError(pstate,"3 floats required");
    break;
  case PTPT_INDEX:
    return FunctionPublish_returnInt(pstate,(tpoint-l3d->trail->tpoints));
  default:
    LUX_ASSERT(0);
    LUX_ASSUME(0);
    break;
  }

  l3d->trail->recompile = LUX_TRUE;

  return 0;
}


static int PubTrail_color (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  int step;
  List3DNode_t *l3d;
  float *color;
  lxVector4 vec4;
  lxVertex32_t *vert32;

  if ((n!=2 && n!=6) ||
    FunctionPublish_getArg(pstate,2,LUXI_CLASS_L3D_TRAIL,(void*)&ref,LUXI_CLASS_INT,(void*)&step)!=2
    || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dtrail 1 int, optional 4 floats");

  if (f->upvalue){
    if (step >= l3d->trail->length)
      return FunctionPublish_returnError(pstate,"index must be smaller than max length");

    vert32 = &l3d->trail->mesh.vertexData32[step*2];

    if (n==2){
      lxVector4float_FROM_ubyte(vec4,vert32->color);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vec4));
    }
    else if (4!=FunctionPublish_getArgOffset(pstate,2,4,FNPUB_TOVECTOR4(vec4)))
      return FunctionPublish_returnError(pstate,"4 float required");
    lxVector4ubyte_FROM_float(vert32->color,vec4);
    vert32[1].colorc = vert32[0].colorc;
    l3d->trail->compile = LUX_FALSE;
  }
  else{
    if (step < 0)
      step = TRAIL_STEPS;

    color = l3d->trail->colors[step%TRAIL_STEPS];

    if (n==2) return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(color));
    else if (4!=FunctionPublish_getArgOffset(pstate,2,4,FNPUB_TOVECTOR4(color)))
        return FunctionPublish_returnError(pstate,"4 numbers required");

    l3d->trail->compilestart = 0;
    l3d->trail->compile = LUX_TRUE;
    l3d->trail->recompile = LUX_TRUE;
    if (step >= TRAIL_STEPS){
      int i;
      for (i = 1; i < TRAIL_STEPS; i++)
        lxVector4Copy(l3d->trail->colors[i],l3d->trail->colors[0]);
    }
  }
  return 0;
}

static int PubTrail_size (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  int step;
  List3DNode_t *l3d;
  float *color;
  TrailPoint_t *tpoint;
  Trail_t *trail;

  if ((n!=2 && n!=3) ||
    FunctionPublish_getArg(pstate,2,LUXI_CLASS_L3D_TRAIL,(void*)&ref,LUXI_CLASS_INT,(void*)&step)!=2
    || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dtrail 1 int, optional 1 float");

  trail = l3d->trail;


  // manual
  if (f->upvalue){
    if (step >= trail->length)
      return FunctionPublish_returnError(pstate,"index must be smaller than max length");

    tpoint = &trail->tpoints[step];

    if (n==2) return FunctionPublish_returnFloat(pstate,tpoint->_size0);
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_FLOAT, (void*)&tpoint->_size0 ))
      return FunctionPublish_returnError(pstate,"1 float required");
    tpoint->_size1 = tpoint->_size0;
    trail->compile = LUX_FALSE;
  }
  else{
    if (step < 0)
      step = TRAIL_STEPS;

    color = &trail->sizes[step%TRAIL_STEPS];
    if (n==2)return FunctionPublish_returnFloat(pstate,*color);
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_FLOAT, (void*)color ))
        return FunctionPublish_returnError(pstate,"1 float required");
    trail->compilestart = 0;
    trail->compile = LUX_TRUE;
    trail->recompile = LUX_TRUE;
    if (step >= TRAIL_STEPS){
      int i;
      for (i = 1; i < TRAIL_STEPS; i++)
        trail->sizes[i] = trail->sizes[0];
    }
  }

  return 0;
}


static int PubTrail_memoffset (PState pstate,PubFunction_t *f, int n)
{
  return FunctionPublish_returnInt(pstate,(int)f->upvalue);
}

enum PubTrailCmds_e
{
  PTRAIL_DRAWLEN,
  PTRAIL_CAMALIGN,
  PTRAIL_MTL,
  PTRAIL_SPAWNSTEP,
  PTRAIL_RESET,
  PTRAIL_USEVEL,
  PTRAIL_VEL,
  PTRAIL_UVSCALE,
  PTRAIL_UVNORMALIZED,
  PTRAIL_UVNORMALIZESCALE,
  PTRAIL_PLANARMAP,
  PTRAIL_PLANARSCALE,
  PTRAIL_TYPELOCAL,
  PTRAIL_TYPEWORLD,
  PTRAIL_TYPEWORLDREF,
  PTRAIL_CLOSED,
  PTRAIL_POSARRAY,
  PTRAIL_NORMALARRAY,
  PTRAIL_VELARRAY,
  PTRAIL_CLOSESTPOINT,
  PTRAIL_CLOSESTDIST,
  PTRAIL_USELOCALTIME,
  PTRAIL_LOCALTIME,
  PTRAIL_RECOMPILE,
  PTRAIL_CURFIRST,
  PTRAIL_PTRS,
  PTRAIL_VPTRS,
};


static int PubTrail_attributes (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  lxVector3 uvscale;
  StaticArray_t *floatarray;
  TrailPoint_t *tp;
  float *pFlt;
  int i;
  int maxcnt;
  Trail_t *trail;

  if ((n < 1) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_TRAIL,(void*)&ref)
    || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dtrail required");

  trail = l3d->trail;

  switch((int)f->upvalue) {
  case PTRAIL_POSARRAY:
  case PTRAIL_NORMALARRAY:
  case PTRAIL_VELARRAY:
    if (n==1){
      int runner = trail->startidx;
      int len = trail->length;

      floatarray = StaticArray_new(trail->numTpoints*3,LUXI_CLASS_FLOATARRAY,NULL,NULL);
      pFlt = floatarray->floats;
      switch((int)f->upvalue) {
      case PTRAIL_POSARRAY:
        for (i=0; i < trail->length; i++,pFlt+=3,runner++){
          lxVector3Copy(pFlt,trail->tpoints[ runner%len].pos);
        }
        break;
      case PTRAIL_NORMALARRAY:
        for (i=0; i < trail->length; i++,pFlt+=3,runner++){
          lxVector3Copy(pFlt,trail->tpoints[ runner%len].normal);
        }
        break;
      case PTRAIL_VELARRAY:
        for (i=0; i < trail->length; i++,pFlt+=3,runner++){
          lxVector3Copy(pFlt,trail->tpoints[ runner%len].vel);
        }
        break;
      }
      return FunctionPublish_returnType(pstate,LUXI_CLASS_FLOATARRAY,REF2VOID(floatarray->ref));
    }
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOATARRAY, (void*)&ref ) || !Reference_get(ref,floatarray))
      return FunctionPublish_returnError(pstate,"1 floatarry required");

    {
      int runner = trail->startidx;
      int len = trail->length;

      maxcnt = LUX_MIN(trail->numTpoints,floatarray->count/3);
      pFlt = floatarray->floats;
      tp = trail->tpoints;

      switch((int)f->upvalue) {
      case PTRAIL_POSARRAY:
        for (i=0; i < trail->length; i++,pFlt+=3,runner++){
          lxVector3Copy(trail->tpoints[ runner%len].pos,pFlt);
        }
        break;
      case PTRAIL_NORMALARRAY:
        for (i=0; i < trail->length; i++,pFlt+=3,runner++){
          lxVector3Copy(trail->tpoints[ runner%len].normal,pFlt);
        }
        break;
      case PTRAIL_VELARRAY:
        for (i=0; i < trail->length; i++,pFlt+=3,runner++){
          lxVector3Copy(trail->tpoints[ runner%len].vel,pFlt);
        }
        break;
      }
    }


    trail->recompile = LUX_TRUE;
    break;
  case PTRAIL_DRAWLEN:  // drawlength
    if (n==1) return FunctionPublish_setRet(pstate,1,LUXI_CLASS_INT,trail->drawlength);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT, (void*)&trail->drawlength ))
      return FunctionPublish_returnError(pstate,"1 number required");
    trail->drawlength = LUX_MIN(trail->drawlength,trail->length);
    break;
  case PTRAIL_CAMALIGN: //camaligned
    if (n==1) return FunctionPublish_setRet(pstate,1,LUXI_CLASS_BOOLEAN,trail->camaligned);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN, (void*)&trail->camaligned ))
      return FunctionPublish_returnError(pstate,"1 bool required");
    break;
  case PTRAIL_MTL:// material
    if (n==1){
      if (vidMaterial(l3d->drawNodes[0].draw.matRID))
        return FunctionPublish_setRet(pstate,1,LUXI_CLASS_MATERIAL,l3d->drawNodes[0].draw.matRID);
      else if (l3d->drawNodes[0].draw.matRID>=0)
        return FunctionPublish_setRet(pstate,1,LUXI_CLASS_TEXTURE,l3d->drawNodes[0].draw.matRID);
      else
        return 0;
    }
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATSURF, (void*)&l3d->drawNodes[0].draw.matRID ))
      return FunctionPublish_returnError(pstate,"1 matsurface required");
    DrawNode_updateSortID(l3d->drawNodes,DRAW_SORT_TYPE_TRAIL);
    break;
  case PTRAIL_SPAWNSTEP:  // spawnstep
    if (n==1) return FunctionPublish_setRet(pstate,1,LUXI_CLASS_INT,trail->spawnstep);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT, (void*)&trail->spawnstep ))
      return FunctionPublish_returnError(pstate,"1 number required");
    trail->lastspawntime = l3d->trail->uselocaltime ? l3d->trail->localtime : g_LuxTimer.time;
  case PTRAIL_RESET: // reset
    Trail_reset(trail);
    break;
  case PTRAIL_USEVEL: //usevel
    if (n==1) return FunctionPublish_setRet(pstate,1,LUXI_CLASS_BOOLEAN,trail->usevel);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN, (void*)&trail->usevel ))
      return FunctionPublish_returnError(pstate,"1 bool required");
    break;
  case PTRAIL_VEL:  //velocity
    if (n==1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(trail->velocity));
    else if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(trail->velocity))!=3)
      return FunctionPublish_returnError(pstate,"3 floats required");
    break;
  case PTRAIL_UVSCALE:  // uvscale
    if (FunctionPublish_getArgOffset(pstate,1,2,FNPUB_TOVECTOR3(uvscale))!=2)
      return FunctionPublish_returnError(pstate,"2 floats required");
    Trail_setUVs(trail,uvscale[0],uvscale[1]);
    break;
  case PTRAIL_UVNORMALIZESCALE: // uvscale
    if (n==1) return FunctionPublish_setRet(pstate,1,LUXI_CLASS_FLOAT,trail->unormalizescale);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT, (void*)&trail->unormalizescale ))
      return FunctionPublish_returnError(pstate,"1 float required");
    break;
  case PTRAIL_PLANARMAP: // planarmap
    if (n==1) return FunctionPublish_setRet(pstate,1,LUXI_CLASS_INT,trail->planarmap);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT, (void*)&trail->planarmap ))
      return FunctionPublish_returnError(pstate,"1 int required");
    trail->recompile = LUX_TRUE;
    break;
  case PTRAIL_PLANARSCALE: // planarscale
    if (n==1) return FunctionPublish_setRet(pstate,1,LUXI_CLASS_FLOAT,trail->planarscale);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT, (void*)&trail->planarscale ))
      return FunctionPublish_returnError(pstate,"1 float required");
    break;
  case PTRAIL_TYPELOCAL: // typelocal
    if (trail->numTpoints)
      return FunctionPublish_returnError(pstate,"cannot change type if active points exist");
    trail->type = TRAIL_LOCAL;
    break;
  case PTRAIL_TYPEWORLD: // typeworld
    if (trail->numTpoints)
      return FunctionPublish_returnError(pstate,"cannot change type if active points exist");
    trail->type = TRAIL_WORLD;
    break;
  case PTRAIL_TYPEWORLDREF: // typeworldref
    if (trail->numTpoints)
      return FunctionPublish_returnError(pstate,"cannot change type if active points exist");
    trail->type = TRAIL_WORLD_REF;
    break;
  case PTRAIL_UVNORMALIZED: // uvnormalized
    if (n==1) return FunctionPublish_setRet(pstate,1,LUXI_CLASS_BOOLEAN,trail->uvnormalized);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&trail->uvnormalized))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    break;
  case PTRAIL_CLOSED: // closed
    if (n==1) return FunctionPublish_setRet(pstate,1,LUXI_CLASS_BOOLEAN,trail->closed);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&trail->closed))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    break;
  case PTRAIL_CLOSESTDIST:
    if (!FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(uvscale)))
      return FunctionPublish_returnError(pstate,"3 floats required");
    if (trail->type == TRAIL_LOCAL)
      lxVector3InvTransform1(uvscale,l3d->finalMatrix);

    if (Trail_closestDistance(trail,uvscale,&uvscale[0],&tp,&uvscale[1]))
      return FunctionPublish_setRet(pstate,3,LUXI_CLASS_FLOAT,lxfloat2void(uvscale[0]),LUXI_CLASS_TRAILPOINT,(void*)tp,LUXI_CLASS_FLOAT,lxfloat2void(uvscale[1]));
    break;
  case PTRAIL_CLOSESTPOINT:
    if (!FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(uvscale)))
      return FunctionPublish_returnError(pstate,"3 floats required");
    if (trail->type == TRAIL_LOCAL)
      lxVector3InvTransform1(uvscale,l3d->finalMatrix);
    if (Trail_closestPoint(trail,uvscale,&uvscale[0],&tp))
      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_FLOAT,lxfloat2void(uvscale[0]),LUXI_CLASS_TRAILPOINT,(void*)tp);
    break;

  case PTRAIL_USELOCALTIME:
    if (n==1) return FunctionPublish_setRet(pstate,1,LUXI_CLASS_BOOLEAN,trail->uselocaltime);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&trail->uselocaltime))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    break;
  case PTRAIL_LOCALTIME:
    if (n==1) return FunctionPublish_setRet(pstate,1,LUXI_CLASS_INT,trail->localtime);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT, (void*)&trail->localtime))
      return FunctionPublish_returnError(pstate,"1 number required");
    break;
  case PTRAIL_RECOMPILE:
    trail->recompile = LUX_TRUE;
    break;
  case PTRAIL_CURFIRST:
    if (trail->numTpoints)
      return FunctionPublish_returnType(pstate,LUXI_CLASS_TRAILPOINT,&trail->tpoints[trail->startidx]);
    else
      return 0;
  case PTRAIL_PTRS:
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_POINTER,trail->tpoints,LUXI_CLASS_POINTER,trail->tpoints+trail->length);
  case PTRAIL_VPTRS:
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_POINTER,trail->mesh.vertexData32,LUXI_CLASS_POINTER,trail->mesh.vertexData32+(trail->length*2));
  default:
    return 0;
  }

  return 0;
}

void PubClass_Trail_init()
{
  FunctionPublish_initClass(LUXI_CLASS_L3D_TRAIL,"l3dtrail","Contains a collection of points, that are rendered as a quad strip.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_TRAIL,LUXI_CLASS_L3D_NODE);

  FunctionPublish_initClass(LUXI_CLASS_TRAILPOINT,"trailpoint","A index to point of a l3dtrail",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_TRAILPOINT,LUXI_CLASS_L3D_LIST);

  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_memoffset,
    (void*)(size_t)(((TrailPoint_t*)(NULL))->pos),"memtopos",
    "(int bytes):() - byte offset within tpoint to position (float3, SSE aligned, w can be overwritten ");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_memoffset,
    (void*)(size_t)(((TrailPoint_t*)(NULL))->normal) ,"memtonormal",
    "(int bytes):() - byte offset within tpoint to normal (float3, SSE aligned, w can be overwritten) ");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_memoffset,
    (void*)(size_t)(((TrailPoint_t*)(NULL))->vel) ,"memtovel",
    "(int bytes):() - byte offset within tpoint to velocity (float3, SSE aligned, w can be overwritten) ");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_memoffset,
    (void*)(size_t)(&((TrailPoint_t*)(NULL))->_size0) ,"memto_size0",
    "(int bytes):() - byte offset within tpoint to size scaler (will not belong to the trailpoint, but stored interleaved).");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_memoffset,
    (void*)(size_t)(&((TrailPoint_t*)(NULL))->_size1) ,"memto_size1",
    "(int bytes):() - byte offset within tpoint to size scaler (will not belong to the trailpoint, but stored interleaved). Size1 is always multiplied by -1.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_memoffset,
    (void*)(size_t)(sizeof(TrailPoint_t)) ,"memtonext",
    "(int bytes):() - size of the trailpoint structure.");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_new,(void*)0,"new",
    "(trail):(string name,|l3dlayerid|,int length) - a new trail system");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_spawn,(void*)0,"spawn",
    "(trailpoint):(l3dtrail,float pos x,y,z,float normal x,y,z,[float vel x,y,z]) - spawns a new point in world/local coordinates, normal is needed when you dont use facecamera, and velocity when you use velocity. You can automaically spawn points with spawnstep. Illegal if typeworldref is used.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_spawnref,NULL,"spawnworldref",
    "(trailpoint):(l3dtrail,actornode/scenenode,int axis) - spawns a single particle that is autolinked to the given node. Axis is 0(X),1(Y),2(Z) and will mark the axis used for normals. Only works if typeworldref is used.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_point,(void*)PTPT_POS,"tpPos",
    "([float x,y,z]):(l3dtrail,trailpoint,[float x,y,z]) - gets or sets trailpoint's position. Either local or world coordinates, depending on if trail:uselocalpoints is set.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_point,(void*)PTPT_NORMAL,"tpNormal",
    "([float x,y,z]):(l3dtrail,trailpoint,[float x,y,z]) - gets or sets trailpoint's position. Either local or world coordinates, depending on if trail:uselocalpoints is set.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_point,(void*)PTPT_VEL,"tpVel",
    "([float x,y,z]):(l3dtrail,trailpoint,[float x,y,z]) - gets or sets trailpoint's position. Either local or world coordinates, depending on if trail:uselocalpoints is set.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_point,(void*)PTPT_INDEX,"tpIndex",
    "(int index):(l3dtrail,trailpoint) - gets offset within trailpoint memory.");

  /*
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_point,(void*)PTPT_PREV,"tpPrev",
    "([trailpoint]):(l3dtrail,trailpoint) - gets previous trailpoint (or null).");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_point,(void*)PTPT_NEXT,"tpNext",
    "([trailpoint]):(l3dtrail,trailpoint) - gets next trailpoint (or null).");
  */
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_color,(void*)0,"color",
    "([float r,g,b,a]):(l3dtrail,int step,[float r,g,b,a]) - sets or returns color interpolater at step. There is 0-2 steps, step 3 or -1 sets all.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_color,(void*)1,"indexcolor",
    "([float r,g,b,a]):(l3dtrail,int step,[float r,g,b,a]) - sets or returns color at index. There is 0...length-1 steps. The regular color function creates linear interpolated values. This function lets you precisely modify every single value. However if you start using it, the regular color function will be ignored");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_size,(void*)0,"size",
    "([float s]):(l3dtrail,int step,[float size]) - sets or returns size interpolater at step. There is 0-2 steps, step 3 or -1 sets all.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_color,(void*)1,"indexsize",
    "([float r,g,b,a]):(l3dtrail,int step,[float r,g,b,a]) - sets or returns size at index. There is 0...length-1 steps. The regular size function creates linear interpolated values. This function lets you precisely modify every single value. However if you start using it, the regular size function will be ignored.");


  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_POSARRAY,"tpPosarray",
    "([floatarray 3t]):(l3dtrail,[floatarray 3t])- sets or returns position of active trailpoints from floatarray");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_NORMALARRAY,"tpNormalarray",
    "([floatarray 3t]):(l3dtrail,[floatarray 3t])- sets or returns normal of active trailpoints from floatarray");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_VELARRAY,"tpVelarray",
    "([floatarray 3t]):(l3dtrail,[floatarray 3t])- sets or returns velocity of active trailpoints from floatarray");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_DRAWLEN,"drawlength",
    "([int]):(l3dtrail,[int])- sets or return drawlength of trail");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_CAMALIGN,"facecamera",
    "([bool]):(l3dtrail,[bool]) - sets or return facecamera flag");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_MTL,"matsurface",
    "([material/texture]):(l3dtrail,[matsurface]) - sets or returns matsurface");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_SPAWNSTEP,"spawnstep",
    "(int):(l3dtrail,[int])- sets or returns spawnstep in ms, one point will be spawned every spawnstep, if spanwstep is greater 0.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_RESET,"reset",
    "():(l3dtrail) - clears all points");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_RECOMPILE,"recompile",
    "():(l3dtrail) - tells trail mesh to be recompiled. Useful if mounted arrays modify trail data.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_USEVEL,"usevelocity",
    "([boolean]):(l3dtrail,[boolean]) - sets or returns if velocity vector should be added");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_VEL,"velocity",
    "([float x,y,z]):(l3dtrail,[float x,y,z]) - sets or returns velocity vector in local space, units per second.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_UVSCALE,"materialscale",
    "():(l3dtrail,float length, width) - sets how often a material/texture is repeated/minified. length is along x axis ");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_PLANARMAP,"planarmap",
    "([int]) : (l3dtrail,[int]) - sets or returns planar projection axis. -1 is none/disabled, 0 = x, 1 = y, z = 2. Materials will be projected along these axis.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_PLANARSCALE,"planarscale",
    "([float]) : (l3dtrail,[float]) - sets or returns planarscale multiplier. texcoord = worldpos.projected * planarscale.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_TYPELOCAL,"typelocal",
    "() : (l3dtrail) - local transforms are used on the points, ie points are in local coordinates. You should do this after a new trail, or after resetting. It does not allow spawnstep.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_TYPEWORLD,"typeworld",
    "() : (l3dtrail) - local transforms are not used on the points, ie all points are in world coordinates. You should do this after a new trail, or after resetting. (default)");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_TYPEWORLDREF,"typeworldref",
    "() : (l3dtrail) - points take their world coordinates from linked references. You should do this after a new trail, or after resetting. It does not allow spawnstep.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_UVNORMALIZED,"materialnormalized",
    "([boolean]) : (l3dtrail,[boolean]) - sets or returns if the material/texture is stretched in such a fashion that its width is the length of the trail. If materialscale was applied before uscale will be applied, too.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_UVNORMALIZESCALE,"normalizedsize",
    "([float]) : (l3dtrail,[float]) - sets or returns size of an absolute ulength when materialnormalized is used (0 disables).");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_CLOSED,"closed",
    "([boolean]):(l3dtrail,[boolean]) - sets or returns if trail ends where it starts. The last added point will be skipped and the first is used instead, so add one more point than the ones you really need.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_CLOSESTPOINT,"closestpoint",
    "([float dist,trailpoint]):(l3dtrail,[float x,y,z]) - returns closest active trailpoint and distance to it. Last frame's state is used.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_CLOSESTDIST,"closestdistance",
    "([float dist,trailpoint,float fracc ]):(l3dtrail,[float x,y,z]) - returns closest line segment and distance to it. The closest point is fracc*(nexttpoint-tpoint). Last frame's state is used.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_USELOCALTIME,"uselocaltime",
    "([boolean]):(l3dtrail,[boolean]) - sets or returns if trail uses its own localtime (in ms). If false(default) the global time by luxinia is used. All events such as spawstep, usevelocity and spawntime for manually created tpoints depend on this.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_LOCALTIME,"localtime",
    "([int]):(l3dtrail,[int]) - sets or returns trail's own localtime (in ms). Only active when uselocaltime is used.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_PTRS,"datapointer",
    "(pointer start,end):(l3dtrail) - returns first and end address of trailpointer array. Operations must be weithin [start,end[. Trailpoints are saved in cyclic array, the first index can be retrieved with firstpoint. Subsequent points are stored after the first and wrapped around trailpoint array. Within this array the size scalers for the trail sides are stored linearly from start to end pointer.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_VPTRS,"vertexpointer",
    "(pointer start,end):(l3dtrail) - returns first and end address of mesh array. Operations must be weithin [start,end[. The mesh has 2 vertices (vertex32lit) per trailpoint. You can manipulate color and texcoords directly with these pointers.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_TRAIL,PubTrail_attributes,(void*)PTRAIL_USEVEL,"firstpoint",
    "([trailpoint]):(l3dtrail) - returns first trailpoint");
}

