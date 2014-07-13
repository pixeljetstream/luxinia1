// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../scene/actorlist.h"
#include "gl_trail.h"
#include "gl_camlight.h"

//////////////////////////////////////////////////////////////////////////
// Trail


static void Trail_spawn(Trail_t* LUX_RESTRICT trail, ulong curtime, lxMatrix44 mat)
{
  ulong time = (curtime - trail->lastspawntime);
  int cnt = time/trail->spawnstep;

  if (cnt>0){
    lxVector3 cnormal;
    lxVector3 cvel;
    lxVector3 pos;
    lxVector3 normal;
    lxVector3 vel;

    float* LUX_RESTRICT cpos = &mat[12];
    float fracc = 1.0f/((float)cnt);
    float factor = fracc;
    int i;

    lxVector3Set(cnormal,0,0,1);
    lxVector3TransformRot1(cnormal,mat);
    lxVector3TransformRot(cvel,trail->velocity,mat);

    if (trail->numTpoints < 1){
      lxVector3Copy(trail->lastnormal,cnormal);
      lxVector3Copy(trail->lastvel,cvel);
      lxVector3Copy(trail->lastpos,cpos);
    }

    for (i = 0; i < cnt; i++,factor+=fracc){
      lxVector3Lerp(pos,factor,trail->lastpos,cpos);
      lxVector3Lerp(normal,factor,trail->lastnormal,cnormal);
      lxVector3Lerp(vel,factor,trail->lastvel,cvel);

      Trail_addPoint(trail,pos,normal,vel)->updtime = trail->lastspawntime + ((i+1)*trail->spawnstep);
    }

    lxVector3Copy(trail->lastnormal,cnormal);
    lxVector3Copy(trail->lastvel,cvel);
    lxVector3Copy(trail->lastpos,cpos);

    trail->lastspawntime += cnt*trail->spawnstep;
  }
}

static void Trail_updateLinks(Trail_t* trail, int drawlength)
{
  int runner = trail->startidx;
  int i;

  for (i = 0; i < drawlength; i++,runner++){
    TrailPoint_t *tpoint = &trail->tpoints[runner % trail->length];
    LinkObject_t *linkobj;

    if (Reference_isValid(tpoint->linkRef) && Reference_get(tpoint->linkRef,linkobj)){
      lxVector3Copy(tpoint->pos,&linkobj->matrix[12]);
      lxVector3Copy(tpoint->normal,&linkobj->matrix[tpoint->axis*4]);
    }
  }
  trail->recompile = LUX_TRUE;
}

static void Trail_updateVelocity(Trail_t*  trail, ulong curtime,int drawlength)
{
  int runner = trail->startidx;
  int i;

  if (drawlength == trail->length){
    TrailPoint_t *tpoint = trail->tpoints;
    for (i = 0; i < trail->length; i++,tpoint++){
      float fracc = ((float)(curtime - tpoint->updtime)) * 0.001f;
      lxVector3ScaledAdd(tpoint->pos,tpoint->pos,fracc,tpoint->vel);
      tpoint->updtime = curtime;
    }
  }
  else{
    for (i = 0; i < drawlength; i++,runner++){
      TrailPoint_t *tpoint = &trail->tpoints[runner % trail->length];
      float fracc = ((float)(curtime - tpoint->updtime)) * 0.001f;
      lxVector3ScaledAdd(tpoint->pos,tpoint->pos,fracc,tpoint->vel);
      tpoint->updtime = curtime;
    }
  }
}

static void Trail_updateUVnormalize(Trail_t* trail,int drawlength)
{
  lxVertex32_t* LUX_RESTRICT out = trail->mesh.vertexData32;
  float size0 = 0.0f;
  float size1 = 0.0f;
  int len = drawlength;
  int i;

  // first two and last two will get 0,1
  out->tex[0] = 0.0f;
  out++;
  out->tex[0] = 0.0f;
  out++;

  for (i = 1; i < len; i++,out+=2){
    float dist0 = lxVector3DistanceFast(out->pos,(out-2)->pos);
    float dist1 = lxVector3DistanceFast((out+1)->pos,(out-1)->pos);

    out->tex[0] = dist0 + size0;
    size0 += dist0;

    (out+1)->tex[0] = dist1 + size1;
    size1 += dist1;
  }
  size0 = (trail->unormalizescale) ? trail->unormalizescale : size0;
  size1 = (trail->unormalizescale) ? trail->unormalizescale : size1;
  size0 = trail->uscale * 1.0f/size0 ;
  size1 = trail->uscale * 1.0f/size1 ;

  out  = trail->mesh.vertexData32+2;
  for (i = 1; i < len; i++,out+=2){
    out->tex[0] *= size0;
    (out+1)->tex[0] *= size1;
  }
}

static void Trail_updateUVplanarmap(Trail_t* trail,int drawlength)
{
  lxVertex32_t* LUX_RESTRICT out  = trail->mesh.vertexData32;
  float pscale = trail->planarscale;
  int i;

#define PLANARMAP(s,t)  \
  out->tex[0] = out->pos[s]*pscale;\
  out->tex[1] = out->pos[t]*pscale


  switch(trail->planarmap) {
  case 0: // x axis
    for(i = 0; i < drawlength*2; i++,out++){
      PLANARMAP(1,2);
    }
    break;
  case 1:
    for(i = 0; i < drawlength*2; i++,out++){
      PLANARMAP(0,2);
    }
    break;
  case 2:
    for(i = 0; i < drawlength*2; i++,out++){
      PLANARMAP(0,1);
    }
    break;
  default:
    LUX_ASSUME(0);
    break;
  }
#undef PLANARMAP
}


static void Trail_compileColorSize(Trail_t* trail, int drawlength)
{
  float* LUX_RESTRICT colorfrom = trail->colors[0];
  float* LUX_RESTRICT sizefrom  = &trail->sizes[0];
  lxVertex32_t* LUX_RESTRICT out  = &trail->mesh.vertexData32[trail->compilestart*2];
  float*    LUX_RESTRICT size = &trail->tpoints[trail->compilestart]._size0;
  size_t    sizestride = sizeof(TrailPoint_t)/sizeof(float);

  lxVector4 color;
  int i;

  float interfracc = 1.0f/(float)(TRAIL_STEPS-1);
  float intersteps = 0.0f;
  float fracc = 1.0f/(trail->length-1);
  float runner = fracc * (float)trail->compilestart;
  float factor;

  for (i = trail->compilestart; i < drawlength; i++, out += 2, size += sizestride){
    if (runner > intersteps+interfracc){
      colorfrom+=4;
      sizefrom++;
      intersteps+=interfracc;
    }
    factor = (runner-intersteps)*(TRAIL_STEPS-1);

    lxVector4Lerp(color,factor,colorfrom,colorfrom+4);
    lxVector4ubyte_FROM_float(out->color,color);
    out[1].colorc = out[0].colorc;

    size[0] = size[1] = LUX_LERP(factor,*sizefrom,*(sizefrom+1));

    runner += fracc;
  }
}


static void Trail_recompileWorld(Trail_t* trail, int drawlength)
{
  lxVertex32_t* LUX_RESTRICT out  = trail->mesh.vertexData32;
  float*    LUX_RESTRICT size = &trail->tpoints[0]._size0;
  size_t    sizestride = sizeof(TrailPoint_t)/sizeof(float);

  int i;
  int idx = trail->startidx;
  int lasti = drawlength-1;
  int lastlen = trail->length-1;

  lxVector3 side;
  lxVector3 normal;
  lxVector3 next;
  lxVector3 lastside = {0.0f,0.0f,0.0f};

  for (i = 0; i < drawlength; i++,idx++,out += 2, size += sizestride){
    TrailPoint_t *tpoint = &trail->tpoints[(idx % trail->length)];

    if (trail->closed && i == lasti){
      tpoint = &trail->tpoints[trail->startidx];
    }

    // normal
    if (trail->camaligned){
      lxVector3Sub(normal,g_CamLight.camera->pos,tpoint->pos);
      lxVector3NormalizedFast(normal);
    }
    else{
      lxVector3Copy(normal,tpoint->normal);
    }
    // next
    if (i < lasti){
      lxVector3Sub(next,(tpoint+1)->pos,tpoint->pos);
    }

    // side vector
    lxVector3Cross(side,normal,next);
    if (lxVector3Dot(next,next) < 0.001f){
      lxVector3Copy(side,lastside);
    }
    else{
      lxVector3NormalizedFast(side);
      lxVector3Copy(lastside,side);
    }

    // output vertex
    lxVector3ScaledAdd(out[0].pos,tpoint->pos,size[0],side);
    lxVector3short_FROM_float(out[0].normal,normal);

    lxVector3ScaledAdd(out[1].pos,tpoint->pos,-size[1],side);
    *(long long*)out[1].normal = *(long long*)out[0].normal;
    //ARRAY4COPY(out[1].normal,out[0].normal);
  }
}

static void Trail_recompileLocal(Trail_t* trail, int drawlength, float* LUX_RESTRICT mat)
{
  lxVertex32_t* LUX_RESTRICT out  = trail->mesh.vertexData32;
  float*    LUX_RESTRICT size = &trail->tpoints[0]._size0;
  size_t    sizestride = sizeof(TrailPoint_t)/sizeof(float);

  int i;
  int idx = trail->startidx;
  int lasti = drawlength-1;
  int lastlen = trail->length-1;

  lxVector3 side;
  lxVector3 normal;
  lxVector3 next;
  lxVector3 lastside = {0.0f,0.0f,0.0f};
  lxVector3 temp;

  for (i = 0; i < drawlength; i++,idx++,out += 2, size += sizestride){
    TrailPoint_t *tpoint = &trail->tpoints[(idx % trail->length)];

    if (trail->closed && i == lasti){
      tpoint = &trail->tpoints[trail->startidx];
    }

    // next
    if (i < lasti){
      lxVector3Sub(next,(tpoint+1)->pos,tpoint->pos);
      lxVector3TransformRot1(next,mat);
    }

    lxVector3Transform(temp,tpoint->pos,mat);
    // normal
    if (trail->camaligned){
      lxVector3Sub(normal,g_CamLight.camera->pos,temp);
      lxVector3NormalizedFast(normal);
    }
    else
      lxVector3TransformRot(normal,tpoint->normal,mat);

    // side vector
    lxVector3Cross(side,normal,next);
    if (lxVector3Dot(next,next) < 0.001f){
      lxVector3Copy(side,lastside);
    }
    else{
      lxVector3NormalizedFast(side);
      lxVector3Copy(lastside,side);
    }

    // output vertex
    lxVector3ScaledAdd(out[0].pos,temp,size[0],side);
    lxVector3short_FROM_float(out[0].normal,normal);

    lxVector3ScaledAdd(out[1].pos,temp,-size[1],side);
    *(long long*)out[1].normal = *(long long*)out[0].normal;
    //ARRAY4COPY(out[1].normal,out[0].normal);
  }
}

static void Trail_recompile(Trail_t* LUX_RESTRICT trail, int drawlength, float* LUX_RESTRICT mat)
{
  booln uselocal = trail->type == TRAIL_LOCAL;

  if (trail->compile){
    Trail_compileColorSize(trail,drawlength);
  }

  lxVector3Copy(trail->tpoints[trail->length].pos,trail->tpoints[0].pos);

  if (trail->type == TRAIL_LOCAL){
    Trail_recompileLocal(trail,drawlength,mat);
  }
  else{
    Trail_recompileWorld(trail,drawlength);
  }


  if (trail->uvnormalized){
    Trail_updateUVnormalize(trail,drawlength);
  }
  else if (trail->planarmap >= 0){
    Trail_updateUVplanarmap(trail,drawlength);
  }

  trail->recompile = LUX_FALSE;

  trail->mesh.numVertices = drawlength*2;
  trail->mesh.numIndices = trail->mesh.numVertices;
  trail->mesh.indexMax =  trail->mesh.numVertices-1;
  trail->mesh.numTris = trail->mesh.numIndices - 2;

}


void  Trail_update(Trail_t* LUX_RESTRICT trail, lxMatrix44 mat)
{

  ulong time = trail->uselocaltime ? trail->localtime : g_LuxTimer.time;
  int drawlength;;


  if (trail->spawnstep && trail->type == TRAIL_WORLD){
    Trail_spawn(trail,time,mat);
  }
  else{
    trail->lastspawntime = time;
  }
  drawlength = LUX_MIN(trail->drawlength,trail->numTpoints);

  if (drawlength < 2){
    trail->mesh.numVertices = 0;
    trail->mesh.numIndices = 0;
    trail->mesh.numTris = 0;
    return;
  }


  if (trail->type == TRAIL_WORLD_REF){
    Trail_updateLinks(trail,drawlength);
  }
  else if (trail->usevel){
    Trail_updateVelocity(trail,time,drawlength);
  }

  if(trail->recompile || trail->camaligned || trail->type == TRAIL_LOCAL)
  {
    Trail_recompile(trail,drawlength,mat);
  }

  trail->compilestart = drawlength;
  if (trail->compile && trail->compilestart >= trail->length)
    trail->compile = LUX_FALSE;
}


TrailPoint_t* Trail_addPoint(Trail_t *trail,lxVector3 pos,lxVector3 normal, lxVector3 vel)
{
  int newstartidx = (trail->startidx+trail->length-1)%trail->length;
  TrailPoint_t *tpoint = &trail->tpoints[newstartidx];

  if(trail->numTpoints != trail->length){
    trail->numTpoints++;
  }
  trail->startidx = newstartidx;

  lxVector3Copy(tpoint->normal,normal);
  lxVector3Copy(tpoint->vel,vel);
  lxVector3NormalizedFast(tpoint->normal);
  lxVector3Copy(tpoint->pos,pos);
  tpoint->updtime = trail->uselocaltime ? trail->localtime : g_LuxTimer.time;

  trail->recompile = LUX_TRUE;

  return tpoint;
}


Trail_t*  Trail_new(int length)
{
  ushort i;
  ushort *ind;
  Trail_t *trail = lxMemGenZalloc(sizeof(Trail_t));
  lxVertex32_t *vert;
  float fracc;
  float runner;
  byte  *data;
  size_t vsize = sizeof(lxVertex32_t)*length*2;
  size_t tsize = sizeof(TrailPoint_t)*(length+1);
  size_t isize = sizeof(ushort)*(length*2);

  LUX_ASSERT(length < LUX_SHORT_UNSIGNEDMAX);

  length = LUX_MAX(length,2);

  trail->startidx = length;
  trail->length = length;
  trail->drawlength = length;
  trail->planarscale = 1.0f;
  trail->planarmap = -1;

  for (i = 0; i < TRAIL_STEPS; i++){
    lxVector4Set(trail->colors[i],1,1,1,1);
    trail->sizes[i]=0.5;
  }

  trail->compile = LUX_TRUE;
  trail->compilestart = 0;
  trail->type = TRAIL_WORLD;
  trail->uscale = 1.0f;

  trail->mesh.indexMin = 0;
  trail->mesh.meshtype = MESH_VA;
  trail->mesh.primtype = PRIM_TRIANGLE_STRIP;
  trail->mesh.vertextype = VERTEX_32_NRM;
  trail->mesh.numAllocIndices = length*2;
  trail->mesh.numAllocVertices = length*2;
  trail->mesh.index16 = LUX_TRUE;

  data = lxMemGenZallocAligned(vsize+tsize+isize,32);

  vert = trail->mesh.origVertexData = trail->mesh.vertexData32 = (void*)data;
  ind  = trail->mesh.indicesData16 = (void*)&data[vsize+tsize];
  trail->tpoints = (void*)&data[vsize];


  fracc = 1.0f/(float)(length-1);
  runner = 0.0f;
  for (i = 0; i < length*2; i++,ind++,vert++){
    *ind = i;
    vert->tex[0]=runner;
    vert->tex[1]= (float)(i%2);
    if (i%2)
      runner += fracc;
  }

  return trail;
}

void  Trail_free(Trail_t *trail)
{
  int length = trail->length;
  size_t vsize = sizeof(lxVertex32_t)*length*2;
  size_t tsize = sizeof(TrailPoint_t)*(length+1);
  size_t isize = sizeof(ushort)*(length*2);

  lxMemGenFreeAligned(trail->mesh.vertexData32,(vsize+tsize+isize));

  lxMemGenFree(trail,sizeof(Trail_t));
}

void  Trail_reset(Trail_t *trail)
{
  TrailPoint_t *tpoint = trail->tpoints;


  if (LUX_FALSE && trail->type == TRAIL_WORLD_REF){
    int i;
    int len = trail->length;
    for (i = 0; i < len; i++,tpoint++){
      Reference_releaseWeak(tpoint->linkRef);
      tpoint->linkRef = NULL;
    }
  }

  trail->startidx = trail->length;
  trail->numTpoints = 0;
  trail->mesh.numVertices = 0;
  trail->mesh.numIndices = 0;
  trail->lastspawntime = trail->uselocaltime ? trail->localtime : g_LuxTimer.time;
}

int Trail_checkPoint(Trail_t *trail, TrailPoint_t *tpoint){
  return (tpoint < trail->tpoints+trail->length && tpoint >= trail->tpoints);
}

void  Trail_setUVs(Trail_t *trail, float uscale, float vscale)
{
  int i;
  float fracc;
  float runner;
  lxVertex32_t *vert;

  fracc = 1.0f/(float)(trail->length-1);
  runner = 0;

  trail->uscale = uscale;
  vert = trail->mesh.vertexData32;
  for (i = 0; i < trail->length*2; i++,vert++){
    vert->tex[0]= runner * uscale;
    vert->tex[1]=  (i%2) * vscale;
    if (i%2)
      runner += fracc;
  }
}



int Trail_closestDistance(Trail_t *trail, lxVector3 pos, float *odistSq, TrailPoint_t **otpointClosest, float *ofracc)
{
  if (trail->numTpoints<2 || trail->drawlength<2) return LUX_FALSE;
  else{
    lxVector3 linedir;
    TrailPoint_t *tpoints = trail->tpoints;
    TrailPoint_t *tpointClosest = NULL;
    float closestSq = FLT_MAX;
    float closestFracc = 0;
    float fracc = 0;
    int i;
    int dlen = trail->drawlength-1;
    int len = trail->length;
    int runner = trail->startidx;

    for (i = 0; i < dlen; i++,runner++){
      TrailPoint_t *tcur  = &tpoints[ runner % len];
      TrailPoint_t *tnext = &tpoints[(runner+1) % len];
      float distSq;

      lxVector3Sub(linedir,tnext->pos,tcur->pos);
      distSq = lxVector3LineDistanceSqFracc(pos,tcur->pos,linedir,&fracc);
      if (distSq < closestSq){
        closestFracc = fracc;
        closestSq = distSq;
        tpointClosest = tcur;
      }
    }

    *otpointClosest = tpointClosest;
    *ofracc = closestFracc;
    *odistSq = closestSq;
    return LUX_TRUE;
  }
}

booln Trail_closestPoint(Trail_t *trail,lxVector3 pos,float *odistSq, TrailPoint_t **otpointClosest)
{
  if (!trail->numTpoints || !trail->drawlength) return LUX_FALSE;
  else{
    TrailPoint_t *tpoints = trail->tpoints;
    TrailPoint_t *tpointClosest = NULL;
    float closestSq = FLT_MAX;
    int i;
    int dlen = trail->drawlength;
    int len = trail->length;
    int runner = trail->startidx;

    for (i = 0; i < dlen; i++,runner++){
      TrailPoint_t *tcur = &tpoints[runner % len];
      float dist = lxVector3SqDistance(pos,tcur->pos);
      if (dist < closestSq){
        closestSq = dist;
        tpointClosest = tcur;
      }
    }

    *otpointClosest = tpointClosest;
    *odistSq = closestSq;
    return LUX_TRUE;
  }
}

