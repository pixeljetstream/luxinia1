// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __PARTICLE_H__
#define __PARTICLE_H__

/*
PARTICLE
--------

There is two ways of handling particles. The ParticleSystem and the ParticleCloud
The System is dynamic with particles changing their appearance based on a script input
while the cloud gets its data from the engine, ie the user places particles where
he wants, and must take care of them himself.
The System has influences such as forces and takes care of removing particles and spawning.

Both dump the actual Particle data in the ParticleContainer which is used on rendering.
Rendering is done in /render/gl_particle

For ParticleSystems you will create/destroy emitters as you need
and for ParticleCloud ParticleGroups do same
ParticleGroups can contain particles in world coordinates,
or object coordinates (then they need a matrix update)
world coordinates can be externally sourced thru references

ParticleEmitter (user,GenMem)
ParticleSys (nested,ResMem)

ParticleGroup (user,GenMem)
ParticleCloud (nested,ResMem)


Author: Christoph Kubisch

*/

#include "../common/common.h"
#include "../common/reflib.h"
#include "../scene/linkobject.h"
#include "model.h"
#include "shader.h"
#include "texture.h"

// for PSYS:
// store pointers to particles, else we use structs directly
//#define LUX_PARTICLE_USEPOINTERITERATOR
// for ALL:
// uses VBO when creating billboards
//#define LUX_PARTICLE_USEBATCHVBO


#define MAX_PARTICLES 8192
#define MAX_PFORCES   32
#define MAX_PSUBSYS   8
#define PARTICLE_COMMANDLENGTH  128
#define PARTICLE_COMMAND_LAST (128-4)
#define PARTICLE_COMMAND_FINISH   (128-2)
#define PARTICLE_FORCE_NAMELENGTH 16
// precalc
#define PARTICLE_STEPS      128
#define PARTICLE_DIV_STEPS    0.0078125f
// code must be rewritten if changed !
#define PARTICLE_UNROLLSIZE 4


// particlesystem types
typedef enum ParticleContType_e{
  PARTICLE_SYSTEM = 1,
  PARTICLE_CLOUD
}ParticleContType_t;

typedef enum ParticleGroupType_e{
  PARTICLE_GROUP_WORLD,
  PARTICLE_GROUP_WORLD_REF_ALL,
  PARTICLE_GROUP_WORLD_REF_EACH,
  PARTICLE_GROUP_LOCAL,
  PARTICLE_GROUP_LOCAL_REF_ALL,
}ParticleGroupType_t;

// emittertypes
typedef enum ParticleEmitterType_e{
  PARTICLE_EMITTER_POINT  = 1,
  PARTICLE_EMITTER_CIRCLE ,
  PARTICLE_EMITTER_SPHERE ,
  PARTICLE_EMITTER_RECTANGLE  ,
  PARTICLE_EMITTER_RECTANGLELOCAL ,
  PARTICLE_EMITTER_MODEL  ,
}ParticleEmitterType_t;

// particletypes
typedef enum ParticleType_e{
  PARTICLE_TYPE_POINT   = 1,
  PARTICLE_TYPE_TRIANGLE  ,
  PARTICLE_TYPE_QUAD    ,
  PARTICLE_TYPE_HSPHERE ,
  PARTICLE_TYPE_SPHERE  ,
  PARTICLE_TYPE_INSTMESH  ,
  PARTICLE_TYPE_MODEL   ,
}ParticleType_t;
// forces
typedef enum ParticleForceType_e{
  PARTICLE_FORCE_NONE   ,
  PARTICLE_FORCE_GRAVITY  ,
  PARTICLE_FORCE_WIND   ,
  PARTICLE_FORCE_TARGET , // not ranged (only via fpub)
  PARTICLE_FORCE_MAGNET , // ranged   (only via fpub)
  PARTICLE_FORCE_INTVEL , // not ranged, interpolated velocity  (only via fpub)
}ParticleForceType_t;
// subsystems
typedef enum ParticleSubType_e{
  PARTICLE_SUB_NORMAL   = 1,
  PARTICLE_SUB_TRANSLATED ,
  PARTICLE_SUB_TRAIL
}ParticleSubType_t;

// particle flags
typedef enum ParticleSysFlag_e {
  PARTICLE_PAUSE =    1<<0,   // not updated
  PARTICLE_COLORED =    1<<1,   // needs color array
  PARTICLE_TEXTURED =   1<<2,   // needs texture array
  PARTICLE_SUBSYS =   1<<3,   // there are subsystems
  PARTICLE_SORT =     1<<4,   // sort on drawing
  PARTICLE_FORCES =   1<<5,   // need to apply forces
  PARTICLE_MAGNET =   1<<6,   // has at least one magnet
  PARTICLE_NOSPAWN =    1<<7,   // no new particles spawned
  PARTICLE_NODRAW =   1<<8,   // wont be rendered
  PARTICLE_COMBDRAW =   1<<9,   // combine drawing of subsystems
  PARTICLE_NOVISTEST =  1<<10,    // always process emitters
  PARTICLE_ISSUBSYS  =  1<<11,
  PARTICLE_VOLUME =   1<<12,    // volume effect (wrap positions)
  PARTICLE_DIRTYFORCES =  1<<13,    // need to recompile forcecommand
  PARTICLE_NODEATH =    1<<14,    // age is reset to 0
} ParticleSysFlag_t;
typedef enum ParticleContFlag_e{
  PARTICLE_POINTSMOOTH =  1<<0,
  PARTICLE_CAMROTFIX =  1<<1,   // rotation is affected by camera angle in xy plane
  PARTICLE_COLORMUL =   1<<2,   // colors are multiplied by user value
  PARTICLE_SIZEMUL =    1<<3,   // size is multiplied by user value
  PARTICLE_PROBABILITY =  1<<4,   // not all particles are drawn but when their frand is below user value
  PARTICLE_GLOBALAXIS = 1<<5,   // globally aligned axis
  PARTICLE_ORIGINOFFSET = 1<<6,   // for quads and tris we will add this vector to the original corner offsets
  PARTICLE_NOGPU  =   1<<7,
  PARTICLE_FORCEINSTANCE =1<<8,   // uses instancing even for billboards
  PARTICLE_SINGLENORMAL = 1<<9,   // uses one normal for all vertices, only for instances
}ParticleContFlag_t;

// particle life flags
typedef enum ParticleLifeFlag_e{
  PARTICLE_NOCOLAGE =   1<<0,
  PARTICLE_NOTEXAGE =   1<<1,
  PARTICLE_INTERPOLATE =  1<<2,
  PARTICLE_COLVAR =   1<<3,
  PARTICLE_SIZEVAR =    1<<4,
  PARTICLE_LIFEVAR =    1<<5,
  PARTICLE_SIZEAGE =    1<<6,
  PARTICLE_ROTATE =   1<<7,
  PARTICLE_ROTVEL =   1<<8,
  PARTICLE_ROTOFFSET =  1<<9,
  PARTICLE_ROTVAR =   1<<10,
  PARTICLE_TEXSEQ =   1<<11,
  PARTICLE_TURB =   1<<12,
  PARTICLE_ROTAGE =   1<<13,
  PARTICLE_DIECAMPLANE =  1<<14,
  PARTICLE_VELAGE =   1<<15,
}ParticleLifeFlag_t;

typedef enum ParticleAgeTex_e{
  PARTICLE_AGETEX_COLOR,
  PARTICLE_AGETEX_TEX,
  PARTICLE_AGETEX_SIZE,
  PARTICLE_AGETEX_VEL,
  PARTICLE_AGETEX_ROT,
  PARTICLE_AGETEXS,
}ParticleAgeTex_t;

typedef enum PrtRotUpdType_e{
  PRT_UPD_ROT,
  PRT_UPD_ROT_ROTAGE,
  PRT_UPD_ROTVAR,
  PRT_UPD_ROTVAR_ROTAGE,
  PRT_UPD_NOROT,
}PrtRotUpdType_t;

typedef enum PrtVelUpdType_e{
  PRT_UPD_NOFORCE,
  PRT_UPD_NOFORCE_VELAGE,
  PRT_UPD_FORCE,      // just a single force
  PRT_UPD_FORCE_VELAGE,
  PRT_UPD_FORCECOMMAND, // process the full command
  PRT_UPD_FORCECOMMAND_VELAGE,
}PrtVelUpdType_t;

enum PrtCommand_e{
  PRT_CMD_NONE = 0,   // must be 0

  PRT_CMD_CHECK_STARTTIME,
  PRT_CMD_CHECK_ENDTIME,
  PRT_CMD_FINISH,
  PRT_CMD_FINISH_VELAGE,

  PRT_CMD_PROC_FORCE,   // must be the last commandtype
};

typedef struct ParticleStatic_s{
  // fancy setup for alignment

  // 3 DW - 4 DW aligned vector
  union{
    lxVector3   pos;      // position of particle in world or object coords
    struct{
      float   *pPos;    // when reference is used we will use these
      float   *pNormal;
      int     axis;
    };
  };
  // 1 DW
  int         texid;

  // 3 DW - 4 DW aligned vector
  union{
    lxVector3   normal;     // velocity/normal for static
    struct{
      LinkType_t  linkType;   // 0 = s3d, 1 = actor
      Reference linkRef;
    };
  };
  // 1 DW
  union{
    uchar     color[4];
    uint      colorc;
  };


  // 8 DW - 4 DW aligned vectors
  lxVector3       viewpos;
  float       rot;
  lxVector3       viewnormal;
  float       size;

}ParticleStatic_t;

typedef struct ParticleDyn_s{
  // 8 DW - 4 DW aligned vectors
  lxVector3       pos;      // position of particle in world coords
  float       rot;      // rotation
  lxVector3       vel;      // velocity
  float       size;

  // 3 DW
  uint        life;     // particle lifetime
  uint        age;      // particle age 0 means particle is inactive
  uint        timeStep;
  // 1 DW
  uint        _pad;
}ParticleDyn_t;

typedef struct ParticleList_s{
  ParticleStatic_t    *prtSt;
  struct ParticleList_s *prev,*next;
}ParticleList_t;

typedef struct ParticlePointParams_s{
  float   min;
  float   max;
  float   alphathresh;
  float   dist[3];
}ParticlePointParams_t;

typedef struct ParticleLife_s{
  enum32        lifeflag;   // particle life flag
  PrtRotUpdType_t   rotupdtype;
  PrtVelUpdType_t   velupdtype;

  uint        lifeTime;
  float       lifeTimeVar;    // lifetime variance

  float       rotate;     // rotate in radians per second
  float       rotateoffset; // randomoffset
  float       rotatevar;    // variance
  float       rotatedir;    // direction 0 random 1 pos -1 neg
  float       rotAgeTimes[3];
  float       rotAge[3];

  float       size;     // size value
  float       sizeVar;    // size variance
  float       sizeAgeTimes[3];// times for the sizeAges
  float       sizeAge[3];   // sizeages


  float       velAgeTimes[3];
  float       velAge[3];    // velocity ages


  float       *colorTimes;  // color times
  float       *colors;    // color values in RGBA
  float       colorVar[4];  // variance for channels
  int         colorVarB[4]; // variance as int
  int         colorVarB2[4];  // var * 2
  int         colorVarMulB[4];  // variance as int
  int         colorVarMulB2[4]; // var * 2
  int         numColors;    // number of colors

  float       turb;     // turbulence

  uint        intColorsMulC[4*PARTICLE_STEPS];  // interpolated life
  uint        intColorsC[PARTICLE_STEPS];
  int         intTex[PARTICLE_STEPS];
  float       intSize[PARTICLE_STEPS];
  float       intTurb[PARTICLE_STEPS];
  float       intRot[PARTICLE_STEPS];
  float       intVel[PARTICLE_STEPS];

  char        *modelname;

  float       *texTimes;    // texture times
  char        **texNames;   // texturenames
  int         numTex;     // number of textures
  TextureType_t   texType;    // texturetype

  char        *agetexNames[PARTICLE_AGETEXS];
  uint        agetexRows[PARTICLE_AGETEXS];
}ParticleLife_t;

typedef struct ParticleContainer_s{
  ParticleContType_t  conttype;
  enum32        contflag;

  struct List3DView_s   *drawl3dview;
  ulong         drawtime;
  enum32          visflag;

  ParticleType_t    particletype; // PARTICLE_POINT...
  ParticlePointParams_t pparams;

  enum32        renderflag;   // for various effects
  VIDBlend_t      blend;      // if specific blending is to be done
  VIDAlpha_t      alpha;
  VIDLine_t     line;

  union{
    uchar       userColor[4];
    uint        userColorC;
  };
  float       userSize;
  float       userProbabilityFadeOut;
  float       userProbability;
  lxMatrix44      userAxisMatrix;
  lxVector3       userOffset;

  float       size;
  lxVector4       color;
  int         lmRID;
  lxMatrix44      lmProjMat;
  int         texRID;   // id in resources
  int         numTex;
  MaterialObject_t  *matobj;

  int         modelRID;     // if type is model
  char        *instmeshname;
  struct GLParticleMesh_s   *instmesh;  // if type is instmesh

  int         check;
  lxVector3       checkMin;
  lxVector3       checkMax;

  ParticleLife_t    *life;      // only for dynamic

  union{
    ParticleDyn_t*    particles;    // particles are stored
    ParticleStatic_t* particlesStatic;
  };

  int       numParticles; // number of total particles
  int       numAParticles;  // number of active particles

#ifdef LUX_PARTICLE_USEPOINTERITERATOR
  int         curCache;

  union{
    ParticleDyn_t   **activePrts; // pointer list of active prts
    ParticleStatic_t  **activeStaticCur;
  };

  union{
    struct{
      ParticleList_t    *list;
      ParticleStatic_t  **stptrs;
      ParticleList_t    *unusedPrt;   // pointer list of open slot
    };
    struct{
      ParticleDyn_t   **caches[2];
      ParticleDyn_t   **unusedPrts;
    };
  };
#else
  // only for static system
  ParticleStatic_t  **activeStaticCur;
  ParticleList_t    *list;
  ParticleStatic_t  **stptrs;
  ParticleList_t    *unusedPrt;   // pointer list of open slot
#endif
}ParticleContainer_t;


typedef struct ParticleForce_s{
  ParticleForceType_t forcetype;    // PARTICLE_FORCE_GRAV...
  char        name[PARTICLE_FORCE_NAMELENGTH];

  Reference     linkRef;
  LinkType_t      linkType;   // 0 = s3d, 1 = actor, 2 = l3dbone
  float       *linkMatrix;  // s3d=final, l3dbone = bonematrix
  lxVector3       *intVec3;   // interpolated velocities for PARTICLE_FORCE_INTVEL
  lxVector3       *intVec3Cache;  // transformed

  float       flt;      // air resistance / strength
  float       range;
  float       rangeDiv;
  float       rangeSq;

  lxVector3       vec3;
  lxVector3       pos;
  float       turb;     // turbulence angle in radians
  uint        starttime;
  uint        endtime;

  struct ParticleForce_s  *prev,*next;
}ParticleForce_t;


typedef struct ParticleSubSys_s{
  ParticleSubType_t subsystype;   // PARTICLE_SUBSYS_NORMAL ...
  uint        delay;      // time delay
  char*       name;     // name of file
  int         psysRID;    // pointer to system
  lxVector3       pos;      // position of subsys, for trail: 0=stride 1=minage 2=dir
  lxVector3       dir;      // direction of subsys
}ParticleSubSys_t;

typedef struct ParticleTrail_s{
  ParticleDyn_t     *trailParticle; // we are a trail sysetem and this is the last used index of our target
  int           parentsysRID;
  uint          minAgeTrail;
  int           stride;
  float         dir;

  struct ParticleTrail_s  LUX_LISTNODEVARS;
}ParticleTrail_t;

typedef struct ParticleTrailEmit_s{
  int       parentID;
  uint      starttime;
  uint      endtime;

  struct ParticleTrailEmit_s  LUX_LISTNODEVARS;
}ParticleTrailEmit_t;

typedef struct ParticleEmitter_s{
  // - 4 DW aligned
  lxMatrix44      matrix;     // current world matrix

  ParticleEmitterType_t emittertype;  // tpye of emitter SHADER_EMITTER_POINT...
  int           psysRID;

  lxVector3       dir;      // starting direction
  union{
    struct{
      uint    minAgeTrail;
      int     stride;
      int     parentsysRID;
    };
    lxVector3       pos;      // position in world coords
  };



  lxVector3       lastpos;
  lxVector3       lastdir;
  int         firststart;

  float       offset;
  lxVector3       offsetVelocity;
  float       velocityVar;  // velocity variance
  float       velocity;   // starting velocity
  float       spreadin;   // inner spread around dir in radians ie spread = spread * pi;
  float       spreadout;    // out spread cone
  float       rate;     // particles per ms
  float       flipdirection;
  float       prtsize;

  float       size;     // size of emitter
  union{
    struct  {
      int     modelRID;
      char    *modelname;
    };
    struct  {
      float   size2;      // for rectangle
      int     axis;
    };
  };


  uint        starttime;    // the time the psys was created
  uint        duration;   // endtime offset
  int         istrail;
  uint        oldduration;
  uint        lastspawntime;  // time when last spawn time was
  uint        lastvisibleframe;

  uint        restartdelay; // time delay after end before restart
  int         restarts;   // number of restarts done 0-254, 255 means inf restarts

  uint        startage;   // age a particle has on creation

  int             numSubsys;
  struct ParticleEmitter_s  **subsysEmitters;

  int             numTrails;
  ParticleTrailEmit_t     **trailNodes;

}ParticleEmitter_t;

typedef struct ParticleSys_s{
  ResourceInfo_t    resinfo;
  Char2PtrNode_t    *annotationListHead;

  int         active;
  int         sortkey;
  enum32        psysflag;

  struct List3DSet_s    *l3dset;
  int           indrawlayer;
  int           l3dlayer;


  ParticleTrail_t     *trailListHead;
  ParticleTrailEmit_t   *trailEmitListHead; // they store the time when they should be removed
  ParticleDyn_t     *trailParticle;   // we are a trail sysetem and this is the last used index of our target

  lxBoundingBox_t     bbox;

  ParticleLife_t      life;
  ParticleContainer_t   container;    // life effects size, material...

  int           activeEmitterCnt;
  ParticleEmitter_t   emitterDefaults;// emitter default values

  ParticleForce_t*    forces[MAX_PFORCES];  // slots that can be used
  int           numForces;        // number of how many we use in total
  ParticleForce_t*    forceListHead;      // sorted by starttime
  char          forceCommands[PARTICLE_COMMANDLENGTH];


  ulong         trailtime;
  ulong         lasttime;   // time when last update was

  ParticleSubSys_t*   subsys;     // subsystems
  int           numSubsys;
  int           numTrails;

  float         timescale;

  lxVector3         volumeSize;
  lxVector3         volumeOffset;
  lxVector3         volumeCamInfluence;
  float         volumeDist;

  struct ParticleSys_s  *prev,*next;
}ParticleSys_t;

typedef struct ParticleGroup_s{
  ParticleStatic_t    prtDefaults;
  ParticleGroupType_t   type;
  int           pcloudRID;
  struct ParticleCloud_s  *pcloud;
  int           numParticles;

  Reference       targetref;
  int           shortnormals;

  ParticleList_t      *particles;

  ParticleList_t      *drawListHead;
  ParticleList_t      *drawListTail;

  int           usedrawlist;
  float         autorot;
  float         autoscale;
  int           autoseq;
  float         seqfracc;

  float         sizeVar;
  float         rotVar;
  float         seqVar;
  int           usecolorVar;
  float         colorVar[4];

  struct ParticleGroup_s  LUX_LISTNODEVARS;
}ParticleGroup_t;

typedef struct ParticleCloud_s{
  ResourceInfo_t    resinfo;
  int         sortkey;

  struct List3DSet_s  *l3dset;
  int         l3dlayer;

  int         activeGroupCnt;
  ParticleGroup_t   *groupListHead;

  int         rotvel;
  int         rot;
  int         colorset;
  int         sort;

  int         insiderange;
  float       range;

  ParticleContainer_t container;

  struct ParticleCloud_s  *prev,*next;
}ParticleCloud_t;

//////////////////////////////////////////////////////////////////////////
// ParticleTrail

ParticleTrail_t*  ParticleTrail_new();


//////////////////////////////////////////////////////////////////////////
// ParticleTrailEmit

ParticleTrailEmit_t* ParticleTrailEmit_new(int parentID, int start);
void        ParticleTrailEmit_free(ParticleTrailEmit_t *pte);

//////////////////////////////////////////////////////////////////////////
// ParticlePointParams

void ParticlePointParams_default(ParticlePointParams_t *pparams);

//////////////////////////////////////////////////////////////////////////
// ParticleContainer
void ParticleContainer_reset(ParticleContainer_t *pcont);
int  ParticleContainer_initInstance(ParticleContainer_t *pcont,const char *name);

///////////////////////////////////////////////////////////////////////////////
// ParticleEmitter

ParticleEmitter_t*  ParticleEmitter_new(ParticleSys_t *psys);
void ParticleEmitter_free(ParticleEmitter_t * pemitter);

void ParticleEmitter_spawn(ParticleEmitter_t *pemitter);
void ParticleEmitter_start(ParticleEmitter_t *pemitter,uint timeoffset);  // start with delay from now
void ParticleEmitter_end(ParticleEmitter_t *pemitter,uint timeoffset, int norestart); // end with delay from now

void ParticleEmitter_update(ParticleEmitter_t *pemit, lxMatrix44 matrix, const lxVector3 renderscale);

///////////////////////////////////////////////////////////////////////////////
// ParticleSys

void ParticleSys_spawnTrails(ParticleSys_t *psys);
void ParticleSys_update(ParticleSys_t *psys);

void ParticleSys_init(ParticleSys_t *psys);

void ParticleSys_initAgeTex(ParticleSys_t *psys, struct Texture_s **textures);

void ParticleSys_clear(ParticleSys_t *psys);

void ParticleSys_precalcTurb(ParticleSys_t *psys, const ParticleForce_t *pf);

void ParticleSys_updateForceCommands(ParticleSys_t *psys);

void ParticleSys_updateColorMul(ParticleSys_t *psys);

  // to make force active in list
void ParticleSys_addForce(ParticleSys_t *psys, ParticleForce_t *pforce);
  // takes force out of list
void ParticleSys_removeForce(ParticleSys_t *psys, ParticleForce_t *pforce);

  // allocs a new force within the psys or returns cleared, if we are within the limits
ParticleForce_t * ParticleSys_newForce(ParticleSys_t *psys, const ParticleForceType_t type, const char *name,int *outid);
  // marks the force for overwriting and removes from active list
void ParticleSys_clearForce(ParticleSys_t *psys, ParticleForce_t *pf);

///////////////////////////////////////////////////////////////////////////////
// ParticleCloud
void ParticleCloud_init(ParticleCloud_t *pcloud, int particlecount);

int ParticleCloud_checkParticle(ParticleCloud_t *pcloud, ParticleList_t *prt);

void ParticleCloud_clear(ParticleCloud_t *pcloud);

//////////////////////////////////////////////////////////////////////////
// ParticleGroup

  // default type is PARTICLE_GROUP_LOCAL
ParticleGroup_t*  ParticleGroup_new(ParticleCloud_t *pcloud);
void  ParticleGroup_free(ParticleGroup_t *pgroup);

  // clears all used particles
void  ParticleGroup_reset(ParticleGroup_t *pgroup);

void  ParticleGroup_update(ParticleGroup_t *pgroup, const lxMatrix44 matrix, const lxVector3 scale);

ParticleList_t* ParticleGroup_addParticle(ParticleGroup_t *pgroup);

void  ParticleGroup_removeParticle(ParticleGroup_t *pgroup, ParticleList_t* prt);

  // what: d = distance (vec3+dist), s = size, t = seqid
  // does not preserve drawlisthead
  // returns kickcount
  // RPOOLUSE
int ParticleGroup_removeParticlesMulti(ParticleGroup_t *pgroup, char *what, enum32 *cmpmode, float *ref);

#endif
