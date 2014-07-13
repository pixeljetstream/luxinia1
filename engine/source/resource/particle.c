// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../scene/actorlist.h"
#include "particle.h"
#include "../render/gl_list3d.h"
#include "../render/gl_particle.h"

//LOCALS
static void ParticleSys_updatePrts(ParticleSys_t *psys,uint difftime);
static float  l_origin[3]={0,0,0};

//////////////////////////////////////////////////////////////////////////
// ParticleTrail

ParticleTrail_t*  ParticleTrail_new(){
  ParticleTrail_t* ptrail = reszalloc(sizeof(ParticleTrail_t));
  lxListNode_init(ptrail);

  return ptrail;
}

//////////////////////////////////////////////////////////////////////////
// ParticleTrailEmit

ParticleTrailEmit_t* ParticleTrailEmit_new(int parentID, int start){
  ParticleTrailEmit_t* pte = lxMemGenZalloc(sizeof(ParticleTrailEmit_t));
  lxListNode_init(pte);

  pte->parentID = parentID;
  pte->starttime = start;

  return pte;
}
void        ParticleTrailEmit_free(ParticleTrailEmit_t *pte){
  lxMemGenFree(pte,sizeof(ParticleTrailEmit_t));
}
//////////////////////////////////////////////////////////////////////////
// ParticlePointParams

void ParticlePointParams_default(ParticlePointParams_t *pparams)
{
  pparams->alphathresh = 1.0f;
  pparams->min = 0.0f;
  pparams->max = g_VID.capPointSize;
  lxVector3Set(pparams->dist,1,0,0);
}

//////////////////////////////////////////////////////////////////////////
// ParticleContainer

void ParticleContainer_reset(ParticleContainer_t *pcont)
{
  int n;
  ParticleList_t *plist;
  ParticleDyn_t *prt;
  ParticleStatic_t *prtst;
#ifdef LUX_PARTICLE_USEPOINTERITERATOR
  ParticleDyn_t **prtptr;
#endif



  switch(pcont->conttype) {
  case PARTICLE_CLOUD:
    plist = pcont->list;
    prtst= pcont->particlesStatic;
    for (n = 0; n < pcont->numParticles; n++,plist++,prtst++){
      plist->prtSt = prtst;
      plist->prev = plist-1;
      plist->next = plist+1;
    }
    plist--;plist->next = NULL;
    pcont->list->prev= NULL;
    pcont->unusedPrt = pcont->list;
    pcont->activeStaticCur = pcont->stptrs;
    *pcont->activeStaticCur = NULL;
    break;
  case PARTICLE_SYSTEM:

#ifdef LUX_PARTICLE_USEPOINTERITERATOR
    pcont->curCache = 0;
    pcont->activePrts = pcont->caches[0];
    prt= pcont->particles;
    pcont->activePrts[0] = NULL;
    prtptr = pcont->unusedPrts+pcont->numParticles-1;
    for (n = 0; n < pcont->numParticles; n++,prtptr--,prt++){
      *prtptr = prt;
      prt->age = 0;
    }
#else
    prt = pcont->particles;
    for (n = 0; n < pcont->numParticles+1; n++,prt++){
      prt->age = 0;
    }
#endif
    break;
  default:
    LUX_ASSERT(0);
  }



  pcont->numAParticles = 0;
}

int ParticleContainer_initInstance(ParticleContainer_t *pcont,const char *name)
{
  int i;
  GLParticleMesh_t *newmesh;
  if((pcont->particletype != PARTICLE_TYPE_MODEL && pcont->particletype != PARTICLE_TYPE_INSTMESH) || !name)
    return LUX_TRUE;

  if(pcont->instmesh)
    GLParticleMesh_free(pcont->instmesh,LUX_FALSE);
  pcont->instmesh = NULL;
  pcont->particletype = PARTICLE_TYPE_MODEL;

  i = Model_searchMesh(ResData_getModel(pcont->modelRID),name);
  if (i < 0){
    return LUX_FALSE;
  }

  newmesh = GLParticleMesh_new(ResData_getModel(pcont->modelRID)->meshObjects[i].mesh);
  if (newmesh)
    pcont->particletype = PARTICLE_TYPE_INSTMESH;

  pcont->instmesh = newmesh;

  return (newmesh != NULL);
}

///////////////////////////////////////////////////////////////////////////////
// ParticleEmitter

ParticleEmitter_t* ParticleEmitter_new(ParticleSys_t *psys)
{
  ParticleEmitter_t *pemitter;
  int i;

  if (!psys)
    return NULL;

  pemitter = genzallocSIMD(sizeof(ParticleEmitter_t));
  memcpy(pemitter,&psys->emitterDefaults,sizeof(ParticleEmitter_t));
  pemitter->psysRID = psys->resinfo.resRID;
  pemitter->prtsize = psys->life.size;
  pemitter->oldduration = pemitter->duration;

  pemitter->numSubsys = psys->numSubsys;
  if (psys->numSubsys)
    pemitter->subsysEmitters = lxMemGenZalloc(sizeof(ParticleEmitter_t*)*psys->numSubsys);

  pemitter->numTrails = psys->numTrails;
  if (psys->numTrails)
    pemitter->trailNodes = lxMemGenZalloc(sizeof(ParticleTrailEmit_t*)*psys->numTrails);

  for (i = 0; i < psys->numSubsys; i++){
    if (psys->subsys[i].subsystype != PARTICLE_SUB_TRAIL)
      pemitter->subsysEmitters[i]= ParticleEmitter_new(ResData_getParticleSys(psys->subsys[i].psysRID));
  }
  psys->activeEmitterCnt++;

  return pemitter;
}

void ParticleEmitter_free(ParticleEmitter_t * pemitter)
{
  int i;
  ParticleSys_t *psys = ResData_getParticleSys(pemitter->psysRID);

  for (i = 0; i < pemitter->numSubsys; i++){
    if (pemitter->subsysEmitters[i])
      ParticleEmitter_free(pemitter->subsysEmitters[i]);
  }
  // end trails
  if (psys){
    psys->activeEmitterCnt--;

    for (i = 0; i < pemitter->numTrails; i++){
      if (pemitter->trailNodes[i])
        pemitter->trailNodes[i]->endtime = g_LuxTimer.time + psys->container.life->lifeTime + (float)psys->container.life->lifeTime*psys->container.life->lifeTimeVar ;
    }
  }

  if (pemitter->numSubsys)
    lxMemGenFree(pemitter->subsysEmitters,sizeof(ParticleEmitter_t*)*pemitter->numSubsys);
  if (pemitter->numTrails)
    lxMemGenFree(pemitter->trailNodes,sizeof(ParticleTrailEmit_t*)*pemitter->numTrails);

  genfreeSIMD(pemitter,sizeof(ParticleEmitter_t));
}

void ParticleEmitter_start(ParticleEmitter_t *pemitter,uint timeoffset){
  int i;
  ParticleSys_t *psys = ResData_getParticleSys(pemitter->psysRID);

  if (timeoffset + g_LuxTimer.time == 0)
    timeoffset = 1;
  pemitter->starttime = g_LuxTimer.time + timeoffset;
  if (!psys->lasttime)
    psys->lasttime = pemitter->starttime;
  pemitter->lastspawntime = pemitter->starttime;

  pemitter->firststart = LUX_TRUE;

  pemitter->numTrails = 0;
  for (i = 0; i < pemitter->numSubsys; i++){
    if (pemitter->subsysEmitters[i])
      ParticleEmitter_start(pemitter->subsysEmitters[i],psys->subsys[i].delay + timeoffset);
    else{
      ParticleSys_t *subsys;
      ParticleTrailEmit_t *intnode;

      intnode = ParticleTrailEmit_new(psys->resinfo.resRID,pemitter->starttime);
      // if there used to be an old disable it
      if (pemitter->trailNodes[pemitter->numTrails])
        pemitter->trailNodes[pemitter->numTrails]->endtime = g_LuxTimer.time - 1;
      pemitter->trailNodes[pemitter->numTrails] = intnode;
      subsys = ResData_getParticleSys(psys->subsys[i].psysRID);
      lxListNode_addFirst(subsys->trailEmitListHead,intnode);
      pemitter->numTrails++;
    }
  }
}

void ParticleEmitter_end(ParticleEmitter_t *pemitter,uint timeoffset, int norestart){
  int i;
  ParticleSys_t *psys = ResData_getParticleSys(pemitter->psysRID);

  pemitter->duration = g_LuxTimer.time - pemitter->starttime + timeoffset;
  if (norestart)
    pemitter->restarts = 0;

  for (i = 0; i < pemitter->numSubsys; i++){
    if (pemitter->subsysEmitters[i])
      ParticleEmitter_end(pemitter->subsysEmitters[i],psys->subsys[i].delay + timeoffset,norestart);
  }
  for (i = 0; i < pemitter->numTrails; i++){
    pemitter->trailNodes[i]->endtime = g_LuxTimer.time + timeoffset;
    pemitter->trailNodes[i] = NULL;
  }
}

// spawn particles is recursive
void ParticleEmitter_update(ParticleEmitter_t *pemit, lxMatrix44 matrix,const lxVector3 renderscale)
{
  int i;
  ParticleEmitter_t *subsysemitter;
  ParticleSubSys_t *subsys;
  ParticleSys_t *psys = ResData_getParticleSys(pemit->psysRID);

  int count;
  float difftime;
  ParticleDyn_t*  prt;
  float lerpfactor;
  float fracc;
  int noadd;
  float flt;
  int index;
  Mesh_t  *mesh;
  Model_t *model;
  float*  pos;
  short *normal;

  float dist;
  ParticleDyn_t *trailprt;
  ParticleDyn_t *lastprt;
  ParticleSys_t *parentsys;
  float *offsetvec;
  float *xdir;
  float *zdir;

  lxVector3 emitpos;
  lxVector3 emitdir;
  lxVector3 temp;
  lxVector3 xvec;
  lxVector3 zvec;


  noadd = LUX_FALSE;

  if (pemit->lastvisibleframe+1!=g_VID.frameCnt && pemit->lastspawntime != pemit->starttime){
    pemit->lastspawntime = g_LuxTimer.time;
  }
  pemit->lastvisibleframe = g_VID.frameCnt;

  if (!pemit->istrail){
    // update position and direction
    lxMatrix44GetTranslation(matrix,pemit->pos);
    lxVector3Set(pemit->dir,0,1,0);
    lxVector3TransformRot1(pemit->dir,matrix);

    if (pemit->firststart){
      lxVector3Copy(pemit->lastpos,pemit->pos);
      lxVector3Copy(pemit->lastdir,pemit->dir);
      pemit->firststart = LUX_FALSE;
    }
    // update subsys
    subsys = psys->subsys;
    for (i = 0 ; i < (int)pemit->numSubsys; i++,subsys++){
      subsysemitter = pemit->subsysEmitters[i];
      switch(subsys->subsystype){
        case PARTICLE_SUB_NORMAL:
          ParticleEmitter_update(subsysemitter,matrix,renderscale);
          break;
        case PARTICLE_SUB_TRANSLATED:
          lxMatrix44CopySIMD(subsysemitter->matrix,matrix);
          lxVector3Mul(xvec,subsys->pos,renderscale);
          lxVector3TransformRot(subsysemitter->pos,xvec,subsysemitter->matrix);
          lxMatrix44SetTranslation(subsysemitter->matrix,subsysemitter->pos);
          ParticleEmitter_update(subsysemitter,subsysemitter->matrix,renderscale);
          break;
      }
    }

    xdir = matrix;
    zdir = matrix+8;

    // first check if we actually need to do anything
    if (!pemit->starttime || g_LuxTimer.time < pemit->starttime || psys->psysflag & PARTICLE_PAUSE || psys->psysflag & PARTICLE_NOSPAWN){
      pemit->lastspawntime = g_LuxTimer.time;
      return;
    }

    // now check if we are beyond our endtime, if so check if we have to do any restarts
    if (pemit->duration != 0 && g_LuxTimer.time > pemit->starttime + pemit->duration){
      if (pemit->restarts > 0){
        if (pemit->restarts != 255)
          pemit->restarts--;
        pemit->starttime = pemit->starttime + pemit->duration + pemit->restartdelay;
        return;
      }
      else{
        pemit->duration = pemit->oldduration;
        pemit->starttime = 0;
        return;
      }
    }


  }
  else{
    trailprt = psys->trailParticle;
    parentsys = ResData_getParticleSys(pemit->parentsysRID);
    lastprt = &parentsys->container.particles[parentsys->container.numParticles];

    // trails always fire
    if (parentsys->container.numAParticles < 1)
      return;
  }


  // first check out if we can afford new particles, and if how many
  difftime = g_LuxTimer.timef - (float)pemit->lastspawntime;
  difftime *= psys->timescale;

  count = (difftime * pemit->rate);
  if (count > ((g_LuxTimer.timediff+30) * pemit->rate)+1)
    count = (g_LuxTimer.timediff+30) * pemit->rate;
  if (count + psys->container.numAParticles > psys->container.numParticles)
    count = psys->container.numParticles - psys->container.numAParticles;
  if (count < 1){
    return;
  }


  noadd = LUX_FALSE;
  // now create particles
  lerpfactor = 0;
  fracc = 1.0f/(count);

  for (i = 0; i < count; i++){
    // get list node
#ifdef LUX_PARTICLE_USEPOINTERITERATOR
    prt = psys->container.unusedPrts[psys->container.numParticles-psys->container.numAParticles-1];
#else
    prt = &psys->container.particles[psys->container.numAParticles];
#endif

    // set up age
    prt->rot = 0.001;
    if (psys->life.lifeflag & PARTICLE_LIFEVAR)
      prt->life = (uint)lxVariance(psys->life.lifeTime,psys->life.lifeTimeVar,frandPointer(prt));
    else
      prt->life = psys->life.lifeTime;
    // make rand rotoffset
    if (psys->life.lifeflag & PARTICLE_ROTOFFSET)
      prt->rot = lxFrand()*psys->life.rotateoffset;
    if (psys->life.lifeflag & PARTICLE_ROTATE){
      if (!psys->life.rotatedir){
        prt->rot *= (lxFrand() < 0.5) ? 1 :  -1;
      }
      else
        prt->rot *= (float)psys->life.rotatedir;
    }

    // LERP position and direction
    if (!pemit->istrail){
      lerpfactor += fracc;
      lxVector3Lerp(emitpos,lerpfactor,pemit->lastpos,pemit->pos);
      lxVector3Lerp(emitdir,lerpfactor,pemit->lastdir,pemit->dir);
    }
    else{
      if (trailprt >= lastprt)
        trailprt = parentsys->container.particles;
      psys->trailParticle = trailprt;
      while ( trailprt->age < pemit->minAgeTrail){
        trailprt += pemit->stride;
        if (trailprt >= lastprt)
          trailprt = parentsys->container.particles;
        // we are at beginning again
        if (trailprt == psys->trailParticle){
          noadd = LUX_TRUE;
          break;
        }
      }

      if (!trailprt->age){
        noadd = LUX_TRUE;
        break;
      }

      // we found one
      lxVector3Copy(emitpos,trailprt->pos);
      lxVector3Copy(emitdir,trailprt->vel);
      lxVector3Normalized(emitdir);
      lxVector3Scale(emitdir,emitdir,pemit->dir[0]);

      lxVector3PerpendicularFast(xvec,emitdir);
      lxVector3Cross(zvec,emitdir,xvec);

      xdir = xvec;
      zdir = zvec;

      trailprt += pemit->stride;
    }

    // get position and velocity
    switch(pemit->emittertype){
      case PARTICLE_EMITTER_RECTANGLE:
        prt->pos[pemit->axis] = 0;
        prt->pos[(pemit->axis+1)%3] = pemit->size*(lxFrand() - 0.5f);
        prt->pos[(pemit->axis+2)%3] = pemit->size2*(lxFrand() - 0.5f);
        lxVector3Copy(prt->vel,emitdir);
        lxVector3Add(prt->pos,prt->pos,emitpos);
        offsetvec = prt->vel;
        break;
      case PARTICLE_EMITTER_RECTANGLELOCAL:
        // offset in directions
        flt = pemit->size*(lxFrand() - 0.5f);
        lxVector3ScaledAdd(prt->pos,emitpos,flt,xdir);
        flt = pemit->size2*(lxFrand() - 0.5f);
        lxVector3ScaledAdd(prt->pos,prt->pos,flt,zdir);

        lxVector3Copy(prt->vel,emitdir);
        offsetvec = prt->vel;
        break;
      case PARTICLE_EMITTER_POINT:
        lxVector3Copy(prt->pos,emitpos);
        lxVector3Copy(prt->vel,emitdir);
        offsetvec = prt->vel;
        break;
      case PARTICLE_EMITTER_CIRCLE:
        lxVector3PerpendicularFast(prt->pos,emitdir);
        lxVector3Scale(prt->pos,prt->pos,lxFrand()*pemit->size);
        lxVector3Add(prt->pos,prt->pos,emitpos);
        lxVector3Copy(prt->vel,pemit->dir);
        offsetvec = emitdir;
        break;
      case PARTICLE_EMITTER_SPHERE:
        lxVector3Random(prt->pos);
        lxVector3Copy(prt->vel,prt->pos);
        lxVector3Scale(prt->pos,prt->pos,pemit->size);
        lxVector3Add(prt->pos,prt->pos,emitpos);
        offsetvec = prt->vel;
        break;
      case PARTICLE_EMITTER_MODEL:
        // get a mesh of the model then a vertex
        model = ResData_getModel(pemit->modelRID);
        index = (uint)(lxFrand()*(float)model->numMeshObjects+0.5);
        mesh = model->meshObjects[index].mesh;
        index = (uint)(lxFrand()*(float)mesh->numVertices+0.5);

        pos    = VertexArrayPtr(mesh->vertexData,index,mesh->vertextype,VERTEX_POS);
        normal = VertexArrayPtr(mesh->vertexData,index,mesh->vertextype,VERTEX_NORMAL);

        LUX_ASSERT(VertexValue(mesh->vertextype,VERTEX_SCALARPOS) == LUX_SCALAR_FLOAT32);

        lxVector3float_FROM_short(temp,normal);
        lxVector3TransformRot1(temp,matrix);
        lxVector3Copy(prt->vel,temp);

        lxVector3Transform(temp,pos,matrix);
        lxVector3Copy(prt->pos,temp);
        lxVector3Scale(prt->pos,prt->pos,pemit->size);
        lxVector3Add(prt->pos,prt->pos,pemit->pos);

        offsetvec = prt->vel;
        break;
      default:
        LUX_ASSERT(0);
    }

    if (pemit->offset){
      dist = lxFrand()*pemit->offset;
      lxVector3ScaledAdd(prt->pos,prt->pos,dist,offsetvec);
    }

    flt = pemit->velocityVar ? lxVariance(pemit->velocity,pemit->velocityVar,lxFrand()) : pemit->velocity;
    lxVector3Scale(prt->vel,prt->vel,flt);

    if (pemit->spreadout && flt)
      lxVector3SpreadFast(prt->vel,prt->vel,pemit->spreadin,pemit->spreadout);
    if (pemit->flipdirection && lxFrand() < pemit->flipdirection)
      lxVector3Negated(prt->vel);

    // we had no luck
    if (noadd){
      // just reset age
      //prt->age = 0;
      //prt->timeStep = PARTICLE_STEPS;
      // we get out of the loop, cause we likely wont find any other
      // spots to create particles within this update
      break;
    }
    else{
      prt->age = LUX_MIN(pemit->startage+((float)difftime*(1.0f-lerpfactor)),prt->life-1);
      prt->age = LUX_MAX(prt->age,1);
      lxVector3Add(prt->vel,prt->vel,pemit->offsetVelocity);
      prt->size = pemit->prtsize;
      prt->timeStep = (prt->age*PARTICLE_STEPS)/prt->life;

      // we make it active
#ifdef LUX_PARTICLE_USEPOINTERITERATOR
      psys->container.activePrts[psys->container.numAParticles] = prt;
      psys->container.numAParticles++;
      psys->container.activePrts[psys->container.numAParticles] = NULL;
#else
      psys->container.numAParticles++;
#endif

    }
  }
  if (!noadd){
    if (pemit->istrail)
      psys->trailParticle = trailprt;
    pemit->lastspawntime = (uint)g_LuxTimer.time;
    lxVector3Copy(pemit->lastpos,pemit->pos);
    lxVector3Copy(pemit->lastdir,pemit->dir);
  }


}

///////////////////////////////////////////////////////////////////////////////
// ParticleSystem

void ParticleSys_spawnTrails(ParticleSys_t *psys){
  float scale[3] = {1.0f,1.0f,1.0f};
  int   i;

  if (psys->trailEmitListHead && psys->trailtime < g_LuxTimer.time){
    static lxMatrix44SIMD mat;

    ParticleTrail_t   *trail;
    ParticleTrailEmit_t *list;
    ParticleTrailEmit_t *browse;
    ParticleTrailEmit_t *usedlist;
    float origrate;
    int   activeCnt;

    list = psys->trailEmitListHead;
    psys->trailEmitListHead = NULL;
    usedlist = NULL;

    // first we remove all that are overtime
    // and we put back those who are not active
    // the active ones get into a separate list
    while (list){
      lxListNode_popBack(list,browse);
      if (browse->endtime && browse->endtime < g_LuxTimer.time){
        ParticleTrailEmit_free(browse);
        continue;
      }
      if (browse->starttime < g_LuxTimer.time){
        lxListNode_addFirst(usedlist,browse);}
      else{
        lxListNode_addFirst(psys->trailEmitListHead,browse);}
    }

    lxListNode_forEach(psys->trailListHead,trail)
      activeCnt = 0;

      // find the ones that belong to this trail
      lxListNode_forEach(usedlist,browse)
        if (browse->parentID == trail->parentsysRID)
          activeCnt++;
      lxListNode_forEachEnd(usedlist,browse);


      if (activeCnt){
        lxMatrix44IdentitySIMD(mat);

        origrate = psys->emitterDefaults.rate;
        psys->emitterDefaults.rate *= activeCnt;
        psys->emitterDefaults.istrail = LUX_TRUE;

        // constant rate updates for trails
        psys->emitterDefaults.lastspawntime =  g_LuxTimer.time - g_LuxTimer.timediff;

        // trail values
        psys->emitterDefaults.stride = trail->stride;
        psys->emitterDefaults.minAgeTrail = trail->minAgeTrail;
        psys->emitterDefaults.parentsysRID = trail->parentsysRID;
        psys->emitterDefaults.dir[0] = trail->dir;

        psys->trailParticle = trail->trailParticle;
        ParticleEmitter_update(&psys->emitterDefaults,mat,scale);
        trail->trailParticle = psys->trailParticle;

        psys->emitterDefaults.istrail = LUX_FALSE;
        psys->emitterDefaults.rate = origrate;
      }

    lxListNode_forEachEnd(psys->trailListHead,trail);

    while (usedlist){
      lxListNode_popBack(usedlist,browse);
      lxListNode_addFirst(psys->trailEmitListHead,browse);
    }

    psys->trailtime = g_LuxTimer.time;
  }

  for (i=0; i < psys->numSubsys; i++)
    ParticleSys_spawnTrails(ResData_getParticleSys(psys->subsys[i].psysRID) );

}

void ParticleSys_update(ParticleSys_t *psys)
{
  uint  difftime;
  int i;

  if (!(psys->psysflag & PARTICLE_PAUSE)){

    // now check how much time has gone by and do updates
    difftime = g_LuxTimer.time - psys->lasttime;

    // update life this checks if particles are too old and removes them
    // and apply forces
    if (difftime > 0)
      ParticleSys_updatePrts(psys,(uint)((double)difftime*(double)psys->timescale));
  }

  psys->active = psys->container.numAParticles;
  psys->indrawlayer = LUX_FALSE;


  // update subsystems
  for (i=0;i < psys->numSubsys; i++)
  {
    ParticleSys_t *subsys = ResData_getParticleSys(psys->subsys[i].psysRID);
    ParticleSys_update(subsys);

    psys->active |= subsys->active;
  }

  // we are done
  psys->lasttime = g_LuxTimer.time;
}

#define PRTPROC_NOFORCE_VELAGE  \
  velscale = psys->life.intVel[prt->timeStep];  \
  lxVector3Scale(vel,prt->vel,velscale);      \
  lxVector3ScaledAdd(prt->pos,prt->pos,fracc,vel);

#define PRTPROC_NOFORCE \
  lxVector3ScaledAdd(prt->pos,prt->pos,fracc,prt->vel);

// vel = oldvel
#define PRTPROC_FORCE_START \
  lxVector3Copy(vel,prt->vel);

// vel += grav * t
#define PRTPROC_FORCE_GRAVITY \
  vel[0] += pforce->vec3[0]*fracc;  \
  vel[1] += pforce->vec3[1]*fracc;  \
  vel[2] += pforce->vec3[2]*fracc;

// vel += (wind - oldvel)* airres * t
#define PRTPROC_FORCE_WIND    \
  vel[0] += (pforce->vec3[0]-prt->vel[0])*pforce->flt*fracc;  \
  vel[1] += (pforce->vec3[1]-prt->vel[1])*pforce->flt*fracc;  \
  vel[2] += (pforce->vec3[2]-prt->vel[2])*pforce->flt*fracc;

// vel = normalized(pos to vec3) * strength
#define PRTPROC_FORCE_TARGET  \
  lxVector3Sub(pos,pforce->pos,prt->pos); \
  lxVector3NormalizedFast(pos);         \
  vel[0] += pos[0]*pforce->flt;     \
  vel[1] += pos[1]*pforce->flt;     \
  vel[2] += pos[2]*pforce->flt;

// if within range
// dropoff = ((rangeSq-distSq)/rangeSq)^2
// vel = normalized(pos to vec3) * strength * dropoff
#define PRTPROC_FORCE_MAGNET    \
  lxVector3Sub(pos,pforce->pos,prt->pos);   \
  dist = lxVector3Dot(pos,pos);         \
  if (dist < pforce->rangeSq){                \
    velscale = (pforce->rangeSq-dist)*pforce->rangeDiv; \
    velscale *=velscale;                \
    dist = 1.0f/lxFastSqrt(dist);             \
    lxVector3Scale(pos,pos,dist);             \
    vel[0] += pos[0]*pforce->flt*velscale;        \
    vel[1] += pos[1]*pforce->flt*velscale;        \
    vel[2] += pos[2]*pforce->flt*velscale;        \
  }

#define PRTPROC_FORCE_INTVEL    \
  pMatrix = pforce->intVec3Cache[prt->timeStep];\
  velscale = pforce->flt*fracc;\
  lxVector3Lerp(vel,velscale,prt->vel,pMatrix);


#define PRTPROC_FORCE_FINISH_VELAGE \
  velscale = psys->life.intVel[prt->timeStep];        \
  prt->pos[0] += fracc * (prt->vel[0] + vel[0])*0.5*velscale; \
  prt->pos[1] += fracc * (prt->vel[1] + vel[1])*0.5*velscale; \
  prt->pos[2] += fracc * (prt->vel[2] + vel[2])*0.5*velscale; \
  lxVector3Copy(prt->vel,vel);

#define PRTPROC_FORCE_FINISH  \
  prt->pos[0] += fracc * (prt->vel[0] + vel[0])*0.5;  \
  prt->pos[1] += fracc * (prt->vel[1] + vel[1])*0.5;  \
  prt->pos[2] += fracc * (prt->vel[2] + vel[2])*0.5;  \
  lxVector3Copy(prt->vel,vel);

#define TESTSTATIC

// Update particle position/age ...
static void ParticleSys_updatePrts(ParticleSys_t *psys,uint difftime)
{
  TESTSTATIC lxMatrix44SIMD matrix;

  TESTSTATIC int i;
  TESTSTATIC int activecnt;
  TESTSTATIC float fracc;
  TESTSTATIC lxVector3 vel;
  TESTSTATIC lxVector3 pos;
  TESTSTATIC float rot;
  TESTSTATIC float rot2;
  TESTSTATIC ParticleDyn_t *prt;
  TESTSTATIC ParticleDyn_t *prtstart;
#ifdef LUX_PARTICLE_USEPOINTERITERATOR
  TESTSTATIC ParticleDyn_t **prtptr;
  TESTSTATIC ParticleDyn_t **newactive;
#else
  TESTSTATIC ParticleDyn_t *prtold;
#endif
  TESTSTATIC ParticleForce_t *pforce;
  TESTSTATIC ParticleForce_t *prevforce;
  TESTSTATIC float velscale;
  TESTSTATIC float dist;
  TESTSTATIC float *pMatrix;
  TESTSTATIC float *pIn;
  TESTSTATIC float *pOut;
  TESTSTATIC char *cmd;
  TESTSTATIC int wasfirst;
  TESTSTATIC int dir;
  TESTSTATIC List3DNode_t *l3d;
  TESTSTATIC ActorNode_t *actor;
  TESTSTATIC int unrollend;

  //float *max = psys->visboxMaxs;
  //float *min = psys->visboxMins;
  //Vector3Copy(max,psys->emitter.pos);
  //Vector3Copy(min,psys->emitter.pos);

#ifdef LUX_PARTICLE_USEPOINTERITERATOR
  #define PLIST_START   prtptr = psys->container.activePrts; while( prt = *(prtptr++)){
  #define PLIST_END   }
#else
  #define PLIST_START   prt= psys->container.particles; while(  prt < &psys->container.particles[activecnt]){
  #define PLIST_END   prt++;}
#endif

  activecnt = psys->container.numAParticles;
  if (psys->psysflag & PARTICLE_NODEATH){
    PLIST_START
      prt->age %= prt->life;
      prt->timeStep = (prt->age*PARTICLE_STEPS)/prt->life;
    PLIST_END;
  }
  else{
#ifdef LUX_PARTICLE_USEPOINTERITERATOR
    psys->container.curCache = (psys->container.curCache+1)%2;
    newactive = psys->container.caches[psys->container.curCache];
    PLIST_START
      // create lifetime
      prt->age += difftime;

    // check age and remove particle if too old
    if (prt->age >= prt->life){
      // kill it !
      activecnt--;
      psys->container.unusedPrts[psys->container.numParticles - activecnt -1] = prt;
      prt->age = 0;
      prt->timeStep = PARTICLE_STEPS;
    }
    else{
      *newactive = prt;
      newactive++;
      prt->timeStep = (prt->age*PARTICLE_STEPS)/prt->life;
    }
    PLIST_END;
    // after last set to NULL
    *newactive = NULL;
#else
    PLIST_START
      // create lifetime
      prt->age += difftime;

      // check age and remove particle if too old
      if (prt->age >= prt->life){
        // kill it !
        activecnt--;
        prtold = &psys->container.particles[activecnt];
        *prt = *prtold;
        prtold->age = 0;
        continue;
      }
      else{
        //*newactive = prt;
        //newactive++;
        prt->timeStep = (prt->age*PARTICLE_STEPS)/prt->life;
      }
    PLIST_END;
#endif
    // after last set to NULL
    //*newactive = NULL;
  }

  psys->container.numAParticles = activecnt;
#ifdef LUX_PARTICLE_USEPOINTERITERATOR
  psys->container.activePrts = psys->container.caches[psys->container.curCache];
#endif

  if (!activecnt)
    return;


#ifdef LUX_PARTICLE_USEPOINTERITERATOR
  #define PLIST_UNROLL_ADVANCE prt = *(prtptr++)
  #define PLIST_UNROLL_START  prtptr = psys->container.activePrts;for (i = 0; i < unrollend; i++){prt = *(prtptr++);
  #define PLIST_UNROLL_REST }while( prt = *(prtptr++)){
#else
  #define PLIST_UNROLL_ADVANCE prt++
  #define PLIST_UNROLL_START  prt=psys->container.particles;for (i = 0; i < unrollend; i++){
  #define PLIST_UNROLL_REST }while( prt < &psys->container.particles[psys->container.numAParticles]){
#endif
  // update position based forces
  // and remove them from active if targets are dead

  lxListNode_forEach(psys->forceListHead,pforce)
  StartList:
    unrollend = LUX_FALSE; // we abuse unrollend to check if we need to transform stuff
    switch(pforce->forcetype){
    case PARTICLE_FORCE_INTVEL:
      unrollend = LUX_TRUE;
    case PARTICLE_FORCE_TARGET:
    case PARTICLE_FORCE_MAGNET:
      if (pforce->linkType < 0 || !Reference_isValid(pforce->linkRef)){
        prevforce = pforce->prev;
        if (pforce == psys->forceListHead)
          wasfirst = LUX_TRUE;
        else
          wasfirst = LUX_FALSE;
        ParticleSys_removeForce(psys,pforce);
                            // if we were last, we would restart
        if (psys->forceListHead && (wasfirst || prevforce->next != psys->forceListHead)){
          pforce = prevforce->next;
          goto StartList;
        }
        else
          goto EndList;
        pforce = prevforce;
        break;
      }

      switch(pforce->linkType){
      case 0:// s3d
        pMatrix = pforce->linkMatrix;
        break;
      case 1:// actor
        Reference_get(pforce->linkRef,actor);
        pMatrix = Matrix44Interface_getElements(actor->link.matrixIF,matrix);
        break;
      case 2:// l3d/bone combo
        Reference_get(pforce->linkRef,l3d);
        pMatrix = matrix;
        lxMatrix44Multiply(matrix,l3d->finalMatrix,pforce->linkMatrix);
        break;
      default:
        break;
      }
      lxVector3Transform(pforce->pos,pforce->vec3,pMatrix);
      // abused to check if we need to transform the vertices
      if (unrollend){
        pOut = pforce->intVec3Cache[0];
        pIn = pforce->intVec3[0];
        for (i = 0; i < PARTICLE_STEPS; i++,pIn+=3, pOut +=3){
          lxVector3TransformRot(pOut,pIn,pMatrix);
        }
      }

      break;
    default:
      break;
    }
  lxListNode_forEachEnd(psys->forceListHead,pforce);
  EndList:

  unrollend = activecnt%PARTICLE_UNROLLSIZE;

  if (psys->psysflag & PARTICLE_DIRTYFORCES)
    ParticleSys_updateForceCommands(psys);


  // Apply Forces / Update Position
  fracc = (float)difftime*0.001;
  velscale = 1.0f;
  pforce = psys->forceListHead;

  switch(psys->life.velupdtype) {
  case PRT_UPD_NOFORCE_VELAGE:
    PLIST_START
      PRTPROC_NOFORCE_VELAGE
    PLIST_END;
    break;
  case PRT_UPD_NOFORCE:
    PLIST_UNROLL_START
      PRTPROC_NOFORCE
    PLIST_UNROLL_REST
      PRTPROC_NOFORCE
      PLIST_UNROLL_ADVANCE;
      PRTPROC_NOFORCE
      PLIST_UNROLL_ADVANCE;
      PRTPROC_NOFORCE
      PLIST_UNROLL_ADVANCE;
      PRTPROC_NOFORCE
    PLIST_END;
    break;
  // single pforce with velage
  case PRT_UPD_FORCE_VELAGE:
    switch(pforce->forcetype){
    case PARTICLE_FORCE_GRAVITY:
      PLIST_START
        PRTPROC_FORCE_START
        PRTPROC_FORCE_GRAVITY
        PRTPROC_FORCE_FINISH_VELAGE
      PLIST_END;
      break;
    case PARTICLE_FORCE_WIND:
      PLIST_START
        PRTPROC_FORCE_START
        PRTPROC_FORCE_WIND
        PRTPROC_FORCE_FINISH_VELAGE
      PLIST_END;
      break;
    case PARTICLE_FORCE_MAGNET:
      PLIST_START
        lxVector3Sub(pos,pforce->pos,prt->pos);
        dist = lxVector3Dot(pos,pos);
        if (dist < pforce->rangeSq){
          PRTPROC_FORCE_START
          velscale = (pforce->rangeSq-dist)*pforce->rangeDiv;
          velscale *=velscale;
          // now normalize pos
          dist = 1.0f/lxFastSqrt(dist);
          lxVector3Scale(pos,pos,dist);
          vel[0] += pos[0]*pforce->flt*velscale;
          vel[1] += pos[1]*pforce->flt*velscale;
          vel[2] += pos[2]*pforce->flt*velscale;
          PRTPROC_FORCE_FINISH_VELAGE
        }
        else{
          PRTPROC_NOFORCE_VELAGE
        }
      PLIST_END;
      break;
    case PARTICLE_FORCE_TARGET:
      PLIST_START
        PRTPROC_FORCE_START
        PRTPROC_FORCE_TARGET
        PRTPROC_FORCE_FINISH_VELAGE
      PLIST_END;
      break;
    case PARTICLE_FORCE_INTVEL:
      PLIST_START
        PRTPROC_FORCE_INTVEL
        PRTPROC_FORCE_FINISH_VELAGE
      PLIST_END;
      break;
    default:
      break;
    }
    break;
  // single pforce
  case PRT_UPD_FORCE:
    switch(pforce->forcetype){
    case PARTICLE_FORCE_GRAVITY:
      PLIST_START
        PRTPROC_FORCE_START
        PRTPROC_FORCE_GRAVITY
        PRTPROC_FORCE_FINISH
      PLIST_END;
      break;
    case PARTICLE_FORCE_WIND:
      PLIST_START
        PRTPROC_FORCE_START
        PRTPROC_FORCE_WIND
        PRTPROC_FORCE_FINISH
      PLIST_END;
      break;
    case PARTICLE_FORCE_MAGNET:
      PLIST_START
        lxVector3Sub(pos,pforce->pos,prt->pos);
        dist = lxVector3Dot(pos,pos);
        if (dist < pforce->rangeSq){
          PRTPROC_FORCE_START
          velscale = (pforce->rangeSq-dist)*pforce->rangeDiv;
          velscale *=velscale;
          // now normalize pos
          dist = 1.0f/lxFastSqrt(dist);
          lxVector3Scale(pos,pos,dist);
          vel[0] += pos[0]*pforce->flt*velscale;
          vel[1] += pos[1]*pforce->flt*velscale;
          vel[2] += pos[2]*pforce->flt*velscale;
          PRTPROC_FORCE_FINISH
        }
        else{
          PRTPROC_NOFORCE
        }
      PLIST_END;
      break;
    case PARTICLE_FORCE_TARGET:
      PLIST_START
        PRTPROC_FORCE_START
        PRTPROC_FORCE_TARGET
        PRTPROC_FORCE_FINISH
      PLIST_END;
      break;
    case PARTICLE_FORCE_INTVEL:
      PLIST_START
        PRTPROC_FORCE_INTVEL
        PRTPROC_FORCE_FINISH
      PLIST_END;
      break;
    default:
      break;
    }
    break;
  case PRT_UPD_FORCECOMMAND:
  case PRT_UPD_FORCECOMMAND_VELAGE:
    PLIST_START
      cmd = psys->forceCommands;
      pforce = psys->forceListHead;
      PRTPROC_FORCE_START
      while(*cmd){
        switch(*cmd){
        case PRT_CMD_CHECK_STARTTIME:
          // as we sort forces by starttime we can stop once a particle is too young
          if (prt->age < pforce->starttime)
            cmd= & psys->forceCommands[PARTICLE_COMMAND_FINISH];
          break;
        case PRT_CMD_CHECK_ENDTIME:
          // skip next command and use next pforce
          if (prt->age > pforce->endtime){
            pforce = pforce->next;
            cmd+=2;
          }
          break;
        case PRT_CMD_FINISH:
          PRTPROC_FORCE_FINISH
          cmd++;
          break;
        case PRT_CMD_FINISH_VELAGE:
          PRTPROC_FORCE_FINISH_VELAGE
          cmd++;
          break;
        case PRT_CMD_PROC_FORCE+PARTICLE_FORCE_GRAVITY:
          PRTPROC_FORCE_GRAVITY
          pforce = pforce->next;
          cmd++;
          break;
        case PRT_CMD_PROC_FORCE+PARTICLE_FORCE_WIND:
          PRTPROC_FORCE_WIND
          pforce = pforce->next;
          cmd++;
          break;
        case PRT_CMD_PROC_FORCE+PARTICLE_FORCE_MAGNET:
          PRTPROC_FORCE_MAGNET
          pforce = pforce->next;
          cmd++;
          break;
        case PRT_CMD_PROC_FORCE+PARTICLE_FORCE_TARGET:
          PRTPROC_FORCE_TARGET
          pforce = pforce->next;
          cmd++;
          break;
        case PRT_CMD_PROC_FORCE+PARTICLE_FORCE_INTVEL:
          PRTPROC_FORCE_INTVEL
          pforce = pforce->next;
          cmd++;
          break;
        default:
          break;
        }
      }
    PLIST_END;
    break;
  default:
    break;
  }


  // rotation
  prtstart = psys->container.particles;
  rot = psys->life.rotate * fracc;
  dir = psys->life.rotatedir;
  if (psys->life.rotatedir){
    rot *= psys->life.rotatedir;
    dir = 0;
  }
  else
    dir = 1;
  switch(psys->life.rotupdtype) {
    case PRT_UPD_ROT:
      PLIST_UNROLL_START
        prt->rot += (dir*(prt-prtstart)%2) ? rot : -rot;
      PLIST_UNROLL_REST
        prt->rot += (dir*(prt-prtstart)%2) ? rot : -rot;
        PLIST_UNROLL_ADVANCE;
        prt->rot += (dir*(prt-prtstart)%2) ? rot : -rot;
        PLIST_UNROLL_ADVANCE;
        prt->rot += (dir*(prt-prtstart)%2) ? rot : -rot;
        PLIST_UNROLL_ADVANCE;
        prt->rot += (dir*(prt-prtstart)%2) ? rot : -rot;

      PLIST_END;
      break;
    case PRT_UPD_ROTVAR:
      PLIST_START
        rot2 = lxVariance(psys->life.rotate,psys->life.rotatevar,frandPointer(prt)) * fracc;
        if (prt->rot > 0) prt->rot += rot2;else prt->rot -= rot2;
      PLIST_END;
      break;
    case PRT_UPD_ROT_ROTAGE:
      PLIST_UNROLL_START
        rot2 = psys->life.intRot[prt->timeStep] * rot;
        prt->rot += (dir*(prt-prtstart)%2) ? rot2 : -rot2;
      PLIST_UNROLL_REST
        rot2 = psys->life.intRot[prt->timeStep] * rot;
        prt->rot += (dir*(prt-prtstart)%2) ? rot2 : -rot2;
        PLIST_UNROLL_ADVANCE;
        rot2 = psys->life.intRot[prt->timeStep] * rot;
        prt->rot += (dir*(prt-prtstart)%2) ? rot2 : -rot2;
        PLIST_UNROLL_ADVANCE;
        rot2 = psys->life.intRot[prt->timeStep] * rot;
        prt->rot += (dir*(prt-prtstart)%2) ? rot2 : -rot2;
        PLIST_UNROLL_ADVANCE;
        rot2 = psys->life.intRot[prt->timeStep] * rot;
        prt->rot += (dir*(prt-prtstart)%2) ? rot2 : -rot2;
      PLIST_END;
      break;
    case PRT_UPD_ROTVAR_ROTAGE:
      PLIST_START
        rot2 = lxVariance(psys->life.rotate,psys->life.rotatevar,frandPointer(prt)) * fracc;
        rot2 *= psys->life.intRot[prt->timeStep];
        prt->rot += (dir*(prt-prtstart)%2) ? rot2 : -rot2;
      PLIST_END;
      break;
    case PRT_UPD_NOROT:
    default:
      break;
  }

  if (psys->psysflag & PARTICLE_VOLUME){
    PLIST_START
      if (prt->pos[0] < 0)
        prt->pos[0]+=psys->volumeSize[0];
      else if (prt->pos[0] > psys->volumeSize[0])
        prt->pos[0]-=psys->volumeSize[0];

      if (prt->pos[1] < 0)
        prt->pos[1]+=psys->volumeSize[1];
      else if (prt->pos[1] > psys->volumeSize[1])
        prt->pos[1]-=psys->volumeSize[1];

      if (prt->pos[2] < 0)
        prt->pos[2]+=psys->volumeSize[2];
      else if (prt->pos[2] > psys->volumeSize[2])
        prt->pos[2]-=psys->volumeSize[2];
    PLIST_END;
  }

#undef PLIST_END
#undef PLIST_START

}

//////////////////////////////////////////////////////////////////////////
/// ParticleSys forces related stuff

void ParticleSys_precalcTurb(ParticleSys_t *psys, const ParticleForce_t *pf)
{
  ParticleLife_t *life = &psys->life;
  int startstep;
  int endstep;

  if (!pf->turb)
    return;

  startstep = pf->starttime*PARTICLE_STEPS/life->lifeTime;
  endstep = pf->endtime*PARTICLE_STEPS/life->lifeTime;

  startstep = LUX_MIN(startstep,PARTICLE_STEPS);
  endstep = LUX_MIN(endstep,PARTICLE_STEPS);

  for (startstep; startstep < endstep; startstep++){
    life->intTurb[startstep] += pf->turb;
  }

}

void ParticleSys_addForce(ParticleSys_t *psys, ParticleForce_t *pforce)
{
  ParticleForce_t *pfbrowse;
  lxListNode_init(pforce);

  psys->psysflag |= PARTICLE_DIRTYFORCES;

  lxListNode_forEach(psys->forceListHead,pfbrowse)
    if (pfbrowse->starttime > pforce->starttime){
      lxListNode_insertPrev(pfbrowse,pforce);
      return;
    }
  lxListNode_forEachEnd(psys->forceListHead,pfbrowse);

  lxListNode_addLast(psys->forceListHead,pforce);
}

// takes pforce out of list
void ParticleSys_removeForce(ParticleSys_t *psys, ParticleForce_t *pforce)
{
  psys->psysflag |= PARTICLE_DIRTYFORCES;
  lxListNode_remove(psys->forceListHead,pforce);
}

// allocs a new pforce within the psys, if we are within the limits
ParticleForce_t *ParticleSys_newForce(ParticleSys_t *psys, const ParticleForceType_t type, const char *name, int *outid)
{
  int i;
  ParticleForce_t *pf;
  ResourceChunk_t *old;

  if (!type || psys->numForces >= MAX_PFORCES){
    *outid = -1;
    return NULL;
  }

  for (i = 0; i < MAX_PFORCES; i++){
    if ((pf=psys->forces[i]) && !pf->forcetype){
      pf->forcetype = type;
      strcpy(pf->name,name);
      pf->linkType = LINK_UNSET;
      pf->linkRef = NULL;
      if (type == PARTICLE_FORCE_INTVEL){
        pf->intVec3 = lxMemGenZalloc(sizeof(lxVector3)*PARTICLE_STEPS*2);
        pf->intVec3Cache = pf->intVec3+PARTICLE_STEPS;
        pf->flt = 1.0f;
      }
      *outid = i;
      return pf;
    }
  }

  psys->numForces++;

  for (i = 0; i < MAX_PFORCES; i++){
    if (!psys->forces[i]){
      old = ResData_setChunkFromPtr(&psys->resinfo);
      pf = psys->forces[i] = reszalloc(sizeof(ParticleForce_t));
      pf->linkRef = NULL;
      pf->linkType = LINK_UNSET;
      pf->forcetype = type;
      if (type == PARTICLE_FORCE_INTVEL){
        pf->intVec3 = lxMemGenZalloc(sizeof(lxVector3)*PARTICLE_STEPS*2);
        pf->intVec3Cache = pf->intVec3+PARTICLE_STEPS;
        pf->flt = 1.0f;
      }
      strcpy(pf->name,name);
      ResourceChunk_activate(old);
      *outid = i;
      return pf;
    }
  }
  // should never get here
  LUX_ASSERT(0);
  *outid = -1;
  return NULL;
}

// marks the pforce for overwriting and removes from active list
void ParticleSys_clearForce(ParticleSys_t *psys, ParticleForce_t *pf)
{
  ParticleSys_removeForce(psys,pf);
  Reference_releaseWeakCheck(pf->linkRef);

  if (pf->intVec3){
    lxMemGenFree(pf->intVec3,sizeof(lxVector3)*PARTICLE_STEPS*2);
    pf->intVec3 = pf->intVec3Cache = NULL;
  }
  memset(pf,0,sizeof(ParticleForce_t));
  psys->numForces--;
}

void ParticleSys_updateForceCommands(ParticleSys_t *psys)
{
  ParticleForce_t *pf;
  char      *cmd;
  char      *lastcmd;
  char      finish;


  if (!psys->forceListHead)
    psys->life.velupdtype = PRT_UPD_NOFORCE;
  else if (psys->forceListHead->next == psys->forceListHead)
    psys->life.velupdtype = PRT_UPD_FORCE;
  else
    psys->life.velupdtype = PRT_UPD_FORCECOMMAND;

  if (psys->life.lifeflag & PARTICLE_VELAGE){
    finish = PRT_CMD_FINISH_VELAGE;
    psys->life.velupdtype++;
  }
  else
    finish = PRT_CMD_FINISH;

  cmd = psys->forceCommands;
  lastcmd = &psys->forceCommands[PARTICLE_COMMAND_LAST];
  lxListNode_forEach(psys->forceListHead,pf)
    if (cmd >= lastcmd){
      bprintf("ERROR: ParticleSys Force Command too long, use less forces. %s \n",psys->resinfo.name);
      break;
    }
    if (pf->starttime){
      *cmd = PRT_CMD_CHECK_STARTTIME;
      cmd++;
    }
    if (pf->endtime){
      *cmd = PRT_CMD_CHECK_ENDTIME;
      cmd++;
    }
    *cmd = PRT_CMD_PROC_FORCE + pf->forcetype;
    cmd++;
  lxListNode_forEachEnd(psys->forceListHead,pf);
  *cmd = finish;
  cmd++;
  *cmd = PRT_CMD_NONE;


  psys->forceCommands[PARTICLE_COMMAND_FINISH-1] = PRT_CMD_NONE;
  psys->forceCommands[PARTICLE_COMMAND_FINISH] = finish;
  psys->forceCommands[PARTICLE_COMMAND_FINISH+1] = PRT_CMD_NONE;

  clearFlag(psys->psysflag,PARTICLE_DIRTYFORCES);
}

//////////////////////////////////////////////////////////////////////////
/// ParticleSys Initialization

  // fix things that wouldnt work, setup flags
void ParticleSys_fix(ParticleSys_t *psys)
{
  ParticleLife_t *life = &psys->life;
  ParticleContainer_t *cont = &psys->container;

  if (!psys->emitterDefaults.emittertype){
    lprintf("WARNING prtload: missing emittertype\n");
    psys->emitterDefaults.emittertype = PARTICLE_EMITTER_POINT;
  }
  if (!cont->particletype){
    lprintf("WARNING prtload: missing particletype\n");
    cont->particletype = PARTICLE_TYPE_POINT;
  }

  if (life->numColors && !life->agetexNames[PARTICLE_AGETEX_COLOR])
    lxVector4Copy(cont->color,life->colors);
  else
    lxVector4Set(cont->color,1,1,1,1);

  life->colorVarB[0] = life->colorVar[0]*255.0f;
  life->colorVarB[1] = life->colorVar[1]*255.0f;
  life->colorVarB[2] = life->colorVar[2]*255.0f;
  life->colorVarB[3] = life->colorVar[3]*255.0f;
  life->colorVarB2[0] = life->colorVarB[0]<<1;
  life->colorVarB2[1] = life->colorVarB[1]<<1;
  life->colorVarB2[2] = life->colorVarB[2]<<1;
  life->colorVarB2[3] = life->colorVarB[3]<<1;
  life->colorVarB2[0] = LUX_MAX(1,life->colorVarB2[0]);
  life->colorVarB2[1] = LUX_MAX(1,life->colorVarB2[1]);
  life->colorVarB2[2] = LUX_MAX(1,life->colorVarB2[2]);
  life->colorVarB2[3] = LUX_MAX(1,life->colorVarB2[3]);

  cont->size = life->size;

  // fix stuff that wouldnt work
  if (psys->psysflag & PARTICLE_COMBDRAW && cont->particletype == PARTICLE_TYPE_MODEL)
    psys->psysflag &= ~PARTICLE_COMBDRAW;
  if (psys->life.lifeflag & PARTICLE_COLVAR && !life->numColors)
    psys->life.lifeflag &= ~PARTICLE_COLVAR;
  if (cont->particletype == PARTICLE_TYPE_MODEL && !cont->instmeshname){
    life->numTex = 0;
    psys->psysflag &= ~PARTICLE_TEXTURED;
  }
  if (cont->particletype == PARTICLE_TYPE_POINT && life->numTex){
    if (!GLEW_ARB_point_sprite){
      life->numTex = 0;
      psys->psysflag &= ~PARTICLE_TEXTURED;
    }
    else{
      life->numTex = 1;
      cont->contflag &= ~PARTICLE_POINTSMOOTH;
    }
  }
  if (cont->particletype != PARTICLE_TYPE_POINT)
    cont->contflag &= ~PARTICLE_POINTSMOOTH;
  if (life->numColors > 1 || life->lifeflag & PARTICLE_COLVAR)
    psys->psysflag |= PARTICLE_COLORED;
  if (life->numTex)
    psys->psysflag |= PARTICLE_TEXTURED;
  if (life->numColors < 2 &&  life->lifeflag & PARTICLE_NOCOLAGE)
    life->lifeflag &= ~PARTICLE_NOCOLAGE;
  if (life->numTex < 2 &&  life->lifeflag & PARTICLE_NOTEXAGE)
    life->lifeflag &= ~PARTICLE_NOTEXAGE;
  /*
  if (life->lifeflag & PARTICLE_ROTVEL){
    life->lifeflag &= ~PARTICLE_ROTOFFSET;
    life->lifeflag &= ~PARTICLE_ROTVAR;
    life->lifeflag &= ~PARTICLE_ROTATE;
  }*/

  if (life->numTex == 1 && life->lifeflag & PARTICLE_TEXSEQ)
    life->lifeflag &= ~PARTICLE_TEXSEQ;
  if (life->lifeflag & PARTICLE_ROTVAR && !(life->lifeflag & PARTICLE_ROTATE))
    life->lifeflag &= ~PARTICLE_ROTVAR;
  if (life->lifeflag & PARTICLE_ROTAGE && !(life->lifeflag & PARTICLE_ROTATE))
    life->lifeflag &= ~PARTICLE_ROTAGE;

  if (psys->emitterDefaults.emittertype == PARTICLE_EMITTER_POINT)
    psys->emitterDefaults.size = life->size*5;
  if (psys->emitterDefaults.emittertype == PARTICLE_EMITTER_MODEL)
    psys->emitterDefaults.size = 100;
  if (!(psys->psysflag & PARTICLE_COLORED))
    cont->renderflag |= RENDER_NOVERTEXCOLOR;

  cont->numTex = life->numTex;

  if (psys->life.lifeflag & PARTICLE_ROTVAR)
    psys->life.rotupdtype = PRT_UPD_ROTVAR;
  else if (psys->life.lifeflag & PARTICLE_ROTATE)
    psys->life.rotupdtype = PRT_UPD_ROT;
  else
    psys->life.rotupdtype = PRT_UPD_NOROT;

  if (psys->life.lifeflag & PARTICLE_ROTAGE)
    psys->life.rotupdtype++;

  if (psys->numForces)
    psys->life.velupdtype = PRT_UPD_FORCE;
  else
    psys->life.velupdtype = PRT_UPD_NOFORCE;

  if (psys->life.lifeflag & PARTICLE_VELAGE)
    psys->life.velupdtype++;

  ParticleSys_updateColorMul(psys);
}

void ParticleSys_precalc(ParticleSys_t *psys)
{
  int i;
  uint n;
  int texnum;
  uint time;
  lxVector4 color;
  float size;
  float sizem;
  float velm;
  float rotm;
  ParticleLife_t *life = &psys->life;

  // Precalculate Life
  lxVector4Set(color,1,1,1,1);
  size = life->size;
  for (n = 0; n < PARTICLE_STEPS; n++){
    sizem = 1;
    rotm = 1;
    velm = 1;
    // color
    if (!life->agetexNames[PARTICLE_AGETEX_COLOR])
      if (life->numColors > 1 && !(life->lifeflag & PARTICLE_NOCOLAGE)){
        for (i = life->numColors-1; i >= 0; i--){
          time = (uint)(PARTICLE_STEPS*life->colorTimes[i]);
          if (n > time){
            lxVector4Copy(color,&life->colors[i*4]);
            if (life->lifeflag & PARTICLE_INTERPOLATE && i < (int)life->numColors-1){
              float t;
              t = PARTICLE_STEPS*life->colorTimes[i+1] - time;
              t = (n - time) / t;
              lxVector4Lerp(color,t,color,&life->colors[(i+1)*4]);
            }
            break;
          }
          else if (i == 0)
            lxVector4Copy(color,&life->colors[0]);
        }
      }
      else if (life->numColors > 1 && (life->lifeflag & PARTICLE_NOCOLAGE)){
        lxVector4Copy(color,&life->colors[(n%life->numColors)*4]);
      }
      else if (psys->psysflag & PARTICLE_COLORED)
        lxVector4Copy(color,&life->colors[0]);

    //Vector4Copy(&life->intColors[n*4],color);
    lxVector4ubyte_FROM_float(((uchar*)&life->intColorsC[n]),color);

    // size
    if (life->lifeflag & PARTICLE_SIZEAGE && !life->agetexNames[PARTICLE_AGETEX_SIZE]){
      for (i = 2; i >= 0; i--){
        time = (uint)(life->sizeAgeTimes[i]*PARTICLE_STEPS);
        if (n > time){
          sizem = life->sizeAge[i];
          if (i < 2){
            float t;
            t = PARTICLE_STEPS*life->sizeAgeTimes[i+1] - time;
            t = (n - time) / t;
            sizem = LUX_LERP(t,sizem,life->sizeAge[i+1]);
          }
          break;
        }
        else if (i == 0)
          sizem = life->sizeAge[0];
      }
    }
    life->intSize[n]=sizem;
    // rotation
    if (life->lifeflag & PARTICLE_ROTAGE && !life->agetexNames[PARTICLE_AGETEX_ROT]){
      for (i = 2; i >= 0; i--){
        time = (uint)(life->rotAgeTimes[i]*PARTICLE_STEPS);
        if (n > time){
          rotm = life->rotAge[i];
          if (i < 2){
            float t;
            t = PARTICLE_STEPS*life->rotAgeTimes[i+1] - time;
            t = (n - time) / t;
            rotm = LUX_LERP(t,rotm,life->rotAge[i+1]);
          }
          break;
        }
        else if (i == 0)
          rotm = life->rotAge[0];
      }
    }
    life->intRot[n]=rotm;
    // velm
    if (life->lifeflag & PARTICLE_VELAGE && !life->agetexNames[PARTICLE_AGETEX_VEL]){
      for (i = 2; i >= 0; i--){
        time = (uint)(life->velAgeTimes[i]*PARTICLE_STEPS);
        if (n > time){
          velm = life->velAge[i];
          if (i < 2){
            float t;
            t = PARTICLE_STEPS*life->velAgeTimes[i+1] - time;
            t = (n - time) / t;
            velm = LUX_LERP(t,velm,life->velAge[i+1]);
          }
          break;
        }
        else if (i == 0)
          velm = life->velAge[0];
      }
    }
    life->intVel[n]=velm;
    // texture
    if (life->numTex > 1 && !life->agetexNames[PARTICLE_AGETEX_TEX]){
      if (!(life->lifeflag & PARTICLE_TEXSEQ)){
        if (!(life->lifeflag & PARTICLE_NOTEXAGE))
          for (i = life->numTex-1; i >=0; i--){
            time = (uint)(life->texTimes[i]*PARTICLE_STEPS);
            if (n > time){
              texnum = i;
              break;
            }
            else if (i==0)
              texnum = 0;
          }
        else
          texnum = n%life->numTex;
      }
      else{
        for (i = life->numTex-1; i >=0; i--){
          time = (uint)(life->texTimes[i]*PARTICLE_STEPS);
          if (n%((uint)((2*life->texTimes[life->numTex-1]-life->texTimes[life->numTex-2])*PARTICLE_STEPS)) > time){
            texnum = i;
            break;
          }
          else if (i==0)
            texnum = 0;
        }
      }
    }
    else
      texnum = 0;
    life->intTex[n] = texnum;
  }
}

void ParticleSys_initAgeTex(ParticleSys_t *psys, struct Texture_s **textures)
{
  int i;
  int n;
  Texture_t *tex;
  float *pFlt;
  uint    *pUintIn;
  uint    *pUInt;
  uchar   *pUByte;
  size_t stride;
  size_t rowoffset;

  for (i = 0; i < PARTICLE_AGETEXS; i++){
    tex = textures[i];
    if (tex){
      stride = tex->bpp/8;
      rowoffset = ((int)tex->height-(int)psys->life.agetexRows[i]-1)*tex->width;
      switch(i){
      case PARTICLE_AGETEX_COLOR:
        pUInt = psys->life.intColorsC;
        for (n=0; n < PARTICLE_STEPS; n++,pUInt++){
          pUintIn = (void*)&tex->imageData[(n+rowoffset)*stride];
          pUByte = (void*)pUInt;
          switch(stride)
          {
          case 4:
            *pUInt = *pUintIn;
            break;
          case 3:
            pUByte[0] = ((uchar*)pUintIn)[0];
            pUByte[1] = ((uchar*)pUintIn)[1];
            pUByte[2] = ((uchar*)pUintIn)[2];
            pUByte[3] = 255;
            break;
          case 1:
            pUByte[0] = pUByte[1] = pUByte[2] =((uchar*)pUintIn)[0];
            pUByte[3] = 255;
            break;
          }
        }
        break;
      case PARTICLE_AGETEX_TEX:
        pUInt = psys->life.intTex;
        for (n=0; n < PARTICLE_STEPS; n++,pUInt++){
          *pUInt = tex->imageData[(n+rowoffset)*stride]%psys->life.numTex;
        }
        break;
      case PARTICLE_AGETEX_ROT:
        pFlt = psys->life.intRot;
        for (n=0; n < PARTICLE_STEPS; n++,pFlt++){
          *pFlt = ((float)tex->imageData[(n+rowoffset)*stride]*psys->life.rotAge[0]*LUX_DIV_255) -psys->life.rotAge[1];
        }
        break;
      case PARTICLE_AGETEX_VEL:
        pFlt = psys->life.intVel;
        for (n=0; n < PARTICLE_STEPS; n++,pFlt++){
          *pFlt = ((float)tex->imageData[(n+rowoffset)*stride]*psys->life.velAge[0]*LUX_DIV_255) -psys->life.velAge[1];
        }
        break;
      case PARTICLE_AGETEX_SIZE:
        pFlt = psys->life.intSize;
        for (n=0; n < PARTICLE_STEPS; n++,pFlt++){
          *pFlt = ((float)tex->imageData[(n+rowoffset)*stride]*psys->life.sizeAge[0]*LUX_DIV_255) -psys->life.sizeAge[1];
        }
        break;
      default:
        break;
      }
    }
  }
}

#define ARRAY4MULDIV(c,a,b,divisor)   ((c)[0]=((a)[0]*(b)[0])/divisor,(c)[1]=((a)[1]*(b)[1])/divisor,(c)[2]=((a)[2]*(b)[2])/divisor,(c)[3]=((a)[3]*(b)[3])/divisor)

void ParticleSys_updateColorMul(ParticleSys_t *psys)
{
  ParticleLife_t *life = &psys->life;
  ParticleContainer_t *pcont = &psys->container;
  int ps;
  uchar *pIn =(uchar *) life->intColorsC;
  uchar *pOut = (uchar *) life->intColorsMulC;
  int steps = (life->lifeflag & PARTICLE_NOCOLAGE) ? life->numColors : PARTICLE_STEPS;

  ARRAY4MULDIV(life->colorVarMulB,life->colorVarB,pcont->userColor,255);
  ARRAY4MULDIV(life->colorVarMulB2,life->colorVarB2,pcont->userColor,255);

  life->colorVarMulB2[0] = LUX_MAX(1,life->colorVarMulB2[0]);
  life->colorVarMulB2[1] = LUX_MAX(1,life->colorVarMulB2[1]);
  life->colorVarMulB2[2] = LUX_MAX(1,life->colorVarMulB2[2]);
  life->colorVarMulB2[3] = LUX_MAX(1,life->colorVarMulB2[3]);

  for (ps = 0; ps < steps;ps++,pIn+=4,pOut+=4){
    ARRAY4MULDIV(pOut,pIn,pcont->userColor,255);
  }
}

#undef ARRAY4MULDIV

void ParticleSys_init(ParticleSys_t *psys)
{
  ParticleLife_t *life = &psys->life;
  ParticleContainer_t *cont = &psys->container;
  int i;
  float boundingsize;


  // okay time for some stuff to be setup
  if (g_VID.details == VID_DETAIL_LOW){
    cont->numParticles /= 2;
    psys->emitterDefaults.rate *=0.5;
  }

  psys->emitterDefaults.startage = 1;
  boundingsize = psys->emitterDefaults.size + psys->emitterDefaults.velocity * 100;
  boundingsize = LUX_MAX(5,boundingsize);
  lxVector3Set(psys->bbox.max,boundingsize,boundingsize,boundingsize);
  lxVector3Set(psys->bbox.min,-boundingsize,-boundingsize,-boundingsize);
  lxMatrix44Identity(psys->container.userAxisMatrix);
  lxMatrix44Identity(psys->container.lmProjMat);
  lxVector3Set(psys->volumeCamInfluence,1,1,1);
  LUX_ARRAY4SET(psys->container.userColor,255,255,255,255);
  psys->container.userSize = 1.0f;
  psys->container.userProbability = 1.01f;
  psys->l3dset = NULL;

  cont->lmRID = -1;
  cont->visflag = BIT_ID_1;

  LUX_SIMDASSERT(sizeof(ParticleDyn_t) % 16 == 0);

  // first create particles:
#ifdef LUX_PARTICLE_USEPOINTERITERATOR
  cont->particles = reszallocSIMD(sizeof(ParticleDyn_t)*cont->numParticles);
  cont->activePrts = cont->caches[0];
  cont->caches[0] =  reszalloc(sizeof(ParticleDyn_t*)*(cont->numParticles+1));
  cont->caches[1] =  reszalloc(sizeof(ParticleDyn_t*)*(cont->numParticles+1));
  cont->unusedPrts = reszalloc(sizeof(ParticleDyn_t*)*cont->numParticles);
#else
  cont->particles = reszallocSIMD(sizeof(ParticleDyn_t)*(cont->numParticles+1));
#endif
  // now create the active/free slotlist
  cont->conttype = PARTICLE_SYSTEM;
  cont->life = life;
  ParticleContainer_reset(cont);
  psys->lasttime = 0;

  ParticleSys_fix(psys);

  if (psys->numForces){
    for (i = 0; i < psys->numForces; i++){
      ParticleSys_precalcTurb(psys,psys->forces[i]);
    }

    ParticleSys_updateForceCommands(psys);
  }

  ParticleSys_precalc(psys);

  lxListNode_init(psys);
}

void ParticleSys_clear(ParticleSys_t *psys)
{
  ParticleContainer_t *cont = &psys->container;
  ParticleForce_t *pforce;
  int i;

  if (cont->particletype == PARTICLE_TYPE_MODEL)
    ResData_unrefResource(RESOURCE_MODEL,cont->modelRID,psys);

  if (psys->emitterDefaults.emittertype == PARTICLE_EMITTER_MODEL)
    ResData_unrefResource(RESOURCE_MODEL,psys->emitterDefaults.modelRID,psys);

  if (cont->matobj)
    MaterialObject_free(cont->matobj);

  if (vidMaterial(cont->texRID)){
    ResData_unrefResource(RESOURCE_MATERIAL,cont->texRID-VID_OFFSET_MATERIAL,psys);
  }
  else
    ResData_unrefResource(RESOURCE_TEXTURE,cont->texRID,psys);

  for(i = 0; i < psys->numSubsys; i++){
    ResData_unrefResource(RESOURCE_PARTICLESYS,psys->subsys[i].psysRID,psys);
  }

  for (i =0; i < MAX_PFORCES; i++){
    if (!(pforce=psys->forces[i])) continue;

    Reference_releaseWeakCheck(pforce->linkRef);

    if (pforce->intVec3){
      lxMemGenFree(pforce->intVec3,sizeof(lxVector3)*PARTICLE_STEPS*2);
      pforce->intVec3 = pforce->intVec3Cache = NULL;
    }
  }

  if (cont->instmesh)
    GLParticleMesh_free(cont->instmesh,LUX_FALSE);

  // kill all remaining trails
  lxListNode_destroyList(psys->trailEmitListHead,ParticleTrailEmit_t,ParticleTrailEmit_free);
}

//////////////////////////////////////////////////////////////////////////
// ParticleCloud

void ParticleCloud_init(ParticleCloud_t *pcloud, int particlecount)
{
  ParticleContainer_t *cont = &pcloud->container;

  memset((char*)pcloud+sizeof(ResourceInfo_t),0,sizeof(ParticleCloud_t)-sizeof(ResourceInfo_t));
  cont->numParticles = particlecount;
  cont->visflag = BIT_ID_1;

  LUX_SIMDASSERT(sizeof(ParticleStatic_t) % 16 == 0);

  // first create particles:
  cont->particles = reszallocSIMD(sizeof(ParticleStatic_t)*cont->numParticles);
  cont->list = reszalloc(sizeof(ParticleList_t)*cont->numParticles);
  cont->stptrs = reszalloc(sizeof(ParticleStatic_t*)*(cont->numParticles+1));
  clearArray(cont->list,cont->numParticles);
  clearArray(cont->particles,cont->numParticles);
  clearArray(cont->stptrs,cont->numParticles+1);
  cont->activeStaticCur = cont->stptrs;

  lxMatrix44Identity(pcloud->container.userAxisMatrix);
  lxMatrix44Identity(pcloud->container.lmProjMat);
  LUX_ARRAY4SET(pcloud->container.userColor,255,255,255,255);
  pcloud->container.userSize = 1.0f;
  pcloud->container.userProbability = 1.01f;

  // now create the active/free slotlist
  cont->conttype = PARTICLE_CLOUD;
  ParticleContainer_reset(&pcloud->container);
  cont->modelRID = -1;
  cont->texRID = -1;
  cont->lmRID = -1;
  cont->size = 32.0f;

  cont->renderflag |= RENDER_NODEPTHMASK;
  ParticlePointParams_default(&cont->pparams);
  pcloud->l3dlayer = LIST3D_LAYERS-1;
  lxListNode_init(pcloud);
}

int ParticleCloud_checkParticle(ParticleCloud_t *pcloud, ParticleList_t *prt)
{
  if (prt >= pcloud->container.list && prt < pcloud->container.list + pcloud->container.numParticles)
    return LUX_TRUE;
  else
    return LUX_FALSE;
}

void ParticleCloud_clear(ParticleCloud_t *pcloud)
{
  if (pcloud->container.matobj)
    MaterialObject_free(pcloud->container.matobj);

  if (pcloud->container.instmesh)
    GLParticleMesh_free(pcloud->container.instmesh,LUX_FALSE);
}

//////////////////////////////////////////////////////////////////////////
// ParticleGroup

ParticleGroup_t *ParticleGroup_new(ParticleCloud_t *pcloud)
{
  ParticleGroup_t *pgroup;

  if (!pcloud)
    return NULL;

  pgroup = lxMemGenZalloc(sizeof(ParticleGroup_t));
  pgroup->pcloudRID = pcloud->resinfo.resRID;
  pgroup->pcloud = pcloud;
  pgroup->type = PARTICLE_GROUP_LOCAL;
  pgroup->targetref = NULL;
  lxVector4ubyte_FROM_float(pgroup->prtDefaults.color,pcloud->container.color);

  lxListNode_init(pgroup);
  lxListNode_addLast(pcloud->groupListHead,pgroup);
  pcloud->activeGroupCnt++;

  return pgroup;
}

void  ParticleGroup_free(ParticleGroup_t *pgroup)
{
  ParticleCloud_t *pcloud = ResData_getParticleCloud(pgroup->pcloudRID);
  ParticleGroup_reset(pgroup);

  // remove from active emitter list
  if (pcloud){
    lxListNode_remove(pcloud->groupListHead,pgroup);
    pcloud->activeGroupCnt--;
  }

  lxMemGenFree(pgroup,sizeof(ParticleGroup_t));
}

void  ParticleGroup_reset(ParticleGroup_t *pgroup)
{
  ParticleCloud_t *pcloud = ResData_getParticleCloud(pgroup->pcloudRID);
  ParticleList_t *plist;


  if (pgroup->type == PARTICLE_GROUP_LOCAL_REF_ALL){
    Reference_releaseWeakCheck(pgroup->targetref);
  }


  // return all used particles
  plist = pgroup->particles;
  if (plist){
    // run to end of list, and concat the unused prtlist
    if (pgroup->type == PARTICLE_GROUP_WORLD_REF_EACH){
      while(plist->next){
        plist = plist->next;

        Reference_releaseWeak(plist->prtSt->linkRef);
        plist->prtSt->linkRef = NULL;
      }
    }
    else{
      while(plist->next)
        plist = plist->next;
    }

    if (!pcloud){
      return;
    }

    plist->next = pcloud->container.unusedPrt;
    if (plist->next)
      plist->next->prev = plist;

    pcloud->container.unusedPrt = pgroup->particles;
    pgroup->particles->prev = NULL;
  }

  pcloud->container.numAParticles -= pgroup->numParticles;
}

void  ParticleGroup_update(ParticleGroup_t *pgroup, const lxMatrix44 tmatrix, const lxVector3 scale)
{
  TESTSTATIC lxMatrix44SIMD matrix;
  TESTSTATIC lxMatrix44SIMD refmat;

  TESTSTATIC ParticleList_t *plist;
  TESTSTATIC ParticleList_t *plistend;
  TESTSTATIC ParticleList_t *oldplist;
  TESTSTATIC ParticleStatic_t *prt;
  TESTSTATIC float rotdiff;
  TESTSTATIC float sizediff;
  TESTSTATIC float seqdiff;
  TESTSTATIC int  seqstep;
  TESTSTATIC int  seqclamp;
  TESTSTATIC ActorNode_t *act;
  TESTSTATIC ParticleContainer_t *pcont;
  TESTSTATIC short *pShort;
  ParticleCloud_t*  pcloud = ResData_getParticleCloud(pgroup->pcloudRID);
  plist = pgroup->particles;
  if (!plist)
    return;

  if (scale){
    lxMatrix44IdentitySIMD(matrix);
    lxMatrix44SetScale(matrix,scale);
    lxMatrix44Multiply2SIMD(tmatrix,matrix);
  }
  else
    lxMatrix44CopySIMD(matrix,tmatrix);

  if (pgroup->usedrawlist){
    plistend = pgroup->drawListTail;
    plist = pgroup->drawListHead;
  }
  else
    plistend = NULL;

  pcont = &pcloud->container;

  switch(pgroup->type) {
  case PARTICLE_GROUP_LOCAL:
    while (plist != plistend){
      prt = plist->prtSt;
      lxVector3Transform(prt->viewpos,prt->pos,matrix);
      lxVector3TransformRot(prt->viewnormal,prt->normal,matrix);
      *(pcont->activeStaticCur++) = prt;
      plist = plist->next;
    }
    break;
  case PARTICLE_GROUP_LOCAL_REF_ALL:
    if (Reference_isValid(pgroup->targetref)){
      if (pgroup->shortnormals)
        while (plist != plistend){
          prt = plist->prtSt;
          lxVector3Transform(prt->viewpos,prt->pPos,matrix);
          pShort = (short*)prt->pNormal;
          lxVector3float_FROM_short(prt->viewnormal,pShort);
          lxVector3TransformRot1(prt->viewnormal,matrix);
          *(pcont->activeStaticCur++) = prt;
          plist = plist->next;
        }
      else
        while (plist != plistend){
          prt = plist->prtSt;
          lxVector3Transform(prt->viewpos,prt->pPos,matrix);
          lxVector3TransformRot(prt->viewnormal,prt->pNormal,matrix);
          *(pcont->activeStaticCur++) = prt;
          plist = plist->next;
        }
    }
    else{
      ParticleGroup_reset(pgroup);
    }
    break;
  case PARTICLE_GROUP_WORLD:
    while (plist != plistend){
      prt = plist->prtSt;
      lxVector3Copy(prt->viewpos,prt->pos);
      lxVector3Copy(prt->viewnormal,prt->normal);
      *(pcont->activeStaticCur++) = prt;
      plist = plist->next;
    }
    break;
  case PARTICLE_GROUP_WORLD_REF_ALL:
    if (Reference_isValid(pgroup->targetref)){
      while (plist != plistend){
        prt = plist->prtSt;
        lxVector3Copy(prt->viewpos,prt->pPos);
        lxVector3Copy(prt->viewnormal,prt->pNormal);
        *(pcont->activeStaticCur++) = prt;
        plist = plist->next;
      }
    }
    else{
      ParticleGroup_reset(pgroup);
    }
    break;
  case PARTICLE_GROUP_WORLD_REF_EACH:
    while (plist && plist != plistend){
      prt = plist->prtSt;
      if (Reference_get(prt->linkRef,act)){
        // update pointers for actors
        if (prt->linkType){
          prt->pPos = prt->pNormal = Matrix44Interface_getElements(act->link.matrixIF,refmat);
          prt->pPos += 12;
          prt->pNormal += (prt->axis*4);
        }
        lxVector3Copy(prt->viewpos,prt->pPos);
        lxVector3Copy(prt->viewnormal,prt->pNormal);
        *(pcont->activeStaticCur++) = prt;
        plist = plist->next;
        //if (Vector3isZero(prt->viewpos))
        //  prt->draw = FALSE;
      }
      else{
        // remove particle
        oldplist = plist;
        plist = plist->next;


        Reference_releaseWeak(oldplist->prtSt->linkRef);
        oldplist->prtSt->linkRef = NULL;
        ParticleGroup_removeParticle(pgroup,oldplist);
      }

    }
    break;
  default:
    break;
  }

  if (pgroup->autorot || pgroup->autoscale || pgroup->autoseq){
    plist = pgroup->particles;
    rotdiff = g_LuxTimer.timediff * 0.001;
    seqclamp = pcloud->container.numTex;

    if (pgroup->autoscale)
      sizediff = rotdiff * pgroup->autoscale;
    else
      sizediff = 0.0f;

    if (pgroup->autorot)
      rotdiff *= pgroup->autorot;
    else
      rotdiff = 0.0f;

    if (pgroup->autoseq){
      seqdiff = g_LuxTimer.timediff + pgroup->seqfracc;
      seqdiff /= pgroup->autoseq;
      pgroup->seqfracc = seqdiff - floor(seqdiff);
      seqstep = seqdiff;
    }
    else
      seqstep = 0.0f;

    while (plist){
      prt = plist->prtSt;
      prt->rot += rotdiff;
      prt->size += sizediff;
      prt->texid += seqstep;
      prt->texid %= seqclamp;
      plist = plist->next;
    }
  }
}


ParticleList_t* ParticleGroup_addParticle(ParticleGroup_t *pgroup)
{
  ParticleList_t *plist;
  ParticleStatic_t *prtSt;
  ParticleCloud_t *pcloud = ResData_getParticleCloud(pgroup->pcloudRID);
  // remove a plist from unused
  plist = pcloud->container.unusedPrt;
  if (!plist)
    return NULL;

  pcloud->container.unusedPrt = plist->next;
  if (plist->next)
    plist->next->prev = NULL;

  prtSt = plist->prtSt;
  memcpy(prtSt,&pgroup->prtDefaults,sizeof(ParticleStatic_t));

  if (pgroup->rotVar){
    prtSt->rot = LUX_LERP(lxFrand(),prtSt->rot-pgroup->rotVar,prtSt->rot+pgroup->rotVar);
  }
  if (pgroup->sizeVar){
    prtSt->size = LUX_LERP(lxFrand(),prtSt->size-pgroup->sizeVar,prtSt->size+pgroup->sizeVar);
  }
  if (pgroup->usecolorVar){
    prtSt->color[0] = (uchar)(lxVariance(((float)prtSt->color[0])*LUX_DIV_255,pgroup->colorVar[0],lxFrand())*255.0f);
    prtSt->color[1] = (uchar)(lxVariance(((float)prtSt->color[1])*LUX_DIV_255,pgroup->colorVar[1],lxFrand())*255.0f);
    prtSt->color[2] = (uchar)(lxVariance(((float)prtSt->color[2])*LUX_DIV_255,pgroup->colorVar[2],lxFrand())*255.0f);
    prtSt->color[3] = (uchar)(lxVariance(((float)prtSt->color[3])*LUX_DIV_255,pgroup->colorVar[3],lxFrand())*255.0f);
    LUX_ARRAY4CLAMP(prtSt->color,0,255);
  }

  // add plist to the pgroup
  plist->next = pgroup->particles;
  if (plist->next)
    plist->next->prev = plist;
  plist->prev = NULL;
  pgroup->particles = plist;

  pgroup->numParticles++;
  pcloud->container.numAParticles++;


  return plist;
}

void  ParticleGroup_removeParticle(ParticleGroup_t *pgroup, ParticleList_t* plist)
{
  ParticleCloud_t* pcloud = ResData_getParticleCloud(pgroup->pcloudRID);
  // remove from group
  if (plist->prev)
    plist->prev->next = plist->next;

  if (plist->next)
    plist->next->prev = plist->prev;

  if (pgroup->particles == plist)
    pgroup->particles = plist->next;

  if (pgroup->drawListTail == plist)
    pgroup->drawListTail = plist->prev;
  if (pgroup->drawListHead == plist)
    pgroup->drawListHead = plist->next;

  // add to unused
  plist->prev = NULL;
  plist->next = pcloud->container.unusedPrt;
  if (plist->next)
    plist->next->prev = plist;

  pcloud->container.unusedPrt = plist;

  pgroup->numParticles--;
  pcloud->container.numAParticles--;
}

// what: d = distance (vec3+distsq), s = size, t = texid
int ParticleGroup_removeParticlesMulti(ParticleGroup_t *pgroup, char *what, enum32 *cmpmode, float *ref)
{
  ParticleList_t **keeplist;
  ParticleList_t **keepcur;
  ParticleList_t **kicklist;
  ParticleList_t **kickcur;
  ParticleList_t *plist;

  char  *cmd;
  float *curref;
  enum32  *curcmp;
  int kickedcnt = 0;
  int kick;

  ParticleCloud_t* pcloud = ResData_getParticleCloud(pgroup->pcloudRID);

  if (!what || !what[0]) return 0;

  keepcur = keeplist = rpoolmalloc((pgroup->numParticles*2+2)*sizeof(ParticleList_t*));
  kickcur = kicklist = keeplist+pgroup->numParticles+1;

  plist = pgroup->particles;
  while (plist){
    kick = LUX_TRUE;
    cmd = what;
    curref = ref;
    curcmp = cmpmode;
    while(*cmd && kick){
      switch(*cmd)
      {
      case 'd':
        kick = VIDCompareMode_run(*curcmp,lxVector3SqDistance(curref,plist->prtSt->viewpos),curref[3]);
        curref += 4; break;
      case 's':
        kick = VIDCompareMode_run(*curcmp,plist->prtSt->size,*curref);
        curref++; break;
      case 't':
        kick = VIDCompareMode_run(*curcmp,plist->prtSt->texid,*curref);
        curref++; break;
      }
      curcmp++;
      cmd++;
    }
    if (!kick)
      *keepcur++ = plist;
    else{
      *kickcur++ = plist;
      kickedcnt++;
    }

    plist = plist->next;
  }

  *kickcur = *keepcur = NULL;

  pgroup->numParticles-=kickedcnt;
  pcloud->container.numAParticles-=kickedcnt;

  // rebuild group list
  if (*keeplist){
    keepcur = keeplist;
    keepcur++;
    while (*keepcur){
      (*keepcur)->prev =  *(keepcur-1);
      (*(keepcur-1))->next = *keepcur;
      keepcur++;
    }
    keepcur--;
    (*keepcur)->next = NULL;

    pgroup->particles = *keeplist;
    pgroup->particles->prev = NULL;
  }
  else{
    pgroup->particles = NULL;
  }

  // add to unused
  if (*kicklist){
    kickcur = kicklist;
    kickcur++;
    while (*kickcur){
      (*kickcur)->prev =  *(kickcur-1);
      (*(kickcur-1))->next = *kickcur;
      kickcur++;
    }
    kickcur--;
    (*kickcur)->next =  pcloud->container.unusedPrt;
    if ((*kickcur)->next)
      (*kickcur)->next->prev = *kickcur;

    (*kicklist)->prev = NULL;
    pcloud->container.unusedPrt = *kicklist;
  }

  pgroup->drawListTail = NULL;
  pgroup->drawListHead = NULL;

  return kickedcnt;
}
