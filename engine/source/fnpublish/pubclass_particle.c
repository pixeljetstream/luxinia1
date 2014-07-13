// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../resource/resmanager.h"
#include "../render/gl_list3d.h"
#include "../common/reflib.h"
#include "../common/3dmath.h"
#include "../scene/actorlist.h"
#include "../scene/scenetree.h"
#include "../main/main.h"

// Published here:
// LUXI_CLASS_PARTICLESYS
// LUXI_CLASS_PARTICLEFORCEID
// LUXI_CLASS_L3D_PEMITTER

// LUXI_CLASS_PARTICLE
// LUXI_CLASS_PARTICLECLOUD
// LUXI_CLASS_L3D_PGROUP

// LUXI_CLASS_PARTICLESYS
static int PubParticleSys_load (PState pstate,PubFunction_t *fn, int n)
{
  char *path;
  int index;
  int sortkey;

  if (n < 1 || FunctionPublish_getArg(pstate,1,LUXI_CLASS_STRING,&path)!=1)
    return FunctionPublish_returnError(pstate,"Argument 1 required and must be string");

  sortkey = 0;
  FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&sortkey);

  Main_startTask(LUX_TASK_RESOURCE);
  if (fn->upvalue){
    ResData_forceLoad(RESOURCE_PARTICLESYS);
    index = ResData_addParticleSys(path,LUX_FALSE,sortkey);
    ResData_forceLoad(RESOURCE_NONE);
  }
  else
    index = ResData_addParticleSys(path,LUX_FALSE,sortkey);
  Main_endTask();

  if (index==-1)
    return 0;

  return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLESYS,(void*)index);
}

enum PubPrtSysCmds_e{
  PUBPSYS_SUBSYSNAMED,
  PUBPSYS_USEVOL,
  PUBPSYS_VOLSIZE,
  PUBPSYS_VOLDIST,
  PUBPSYS_VOLCAMINFLUENCE,
  PUBPSYS_VOLOFFSET,
  PUBPSYS_TIMESCALE,
  PUBPSYS_CLEAR,
  PUBPSYS_GETFORCE,
  PUBPSYS_PAUSE,
  PUBPSYS_NODRAW,
  PUBPSYS_COLORMUL,
  PUBPSYS_USECOLORMUL,
  PUBPSYS_SIZEMUL,
  PUBPSYS_USESIZEMUL,
  PUBPSYS_PROBABILITY,
  PUBPSYS_USEPROBABILITY,
  PUBPSYS_PROBFADEOUT,
  PUBPSYS_USEGLOBALAXIS,
  PUBPSYS_GLOBALAXIS,
  PUBPSYS_CHECK,
  PUBPSYS_CHECKMIN,
  PUBPSYS_CHECKMAX,
  PUBPSYS_NOGPU,
  PUBPSYS_SINGLENORMAL,
  PUBPSYS_FORCEINSTANCE,
  PUBPSYS_BBOX,
  PUBPSYS_LM,
  PUBPSYS_LMPROJMAT,
  PUBPSYS_VISFLAG,
  PUBPSYS_LAYER,

  PUBPSYS_INTERPOLATOR_FUNCS,

  PUBPSYS_INTERCOLOR,
  PUBPSYS_INTERTEX,
  PUBPSYS_INTERSIZE,
  PUBPSYS_INTERVEL,
  PUBPSYS_INTERROT,
};

int ParticleSys_subsysString(ParticleSys_t *psys,char *name,char *outcode,int istrail)
{
  ParticleSubSys_t *subsys;
  char *runner = name;
  char cur;
  int i;

  if (istrail || !psys || !psys->numSubsys) return LUX_FALSE;

  while ((cur=*(runner++)) && cur != ',');

  runner--;
  *runner = 0;
  // name now contains subsys name

  subsys = psys->subsys;
  for (i = 0; i < psys->numSubsys; i++,subsys++){
    if(strstr(subsys->name,name)){
      outcode[0] = '0'+i;
      outcode[1] = 0;

      runner++;
      if (*runner)
        return ParticleSys_subsysString(ResData_getParticleSys(subsys->psysRID),runner,outcode+1,subsys->subsystype == PARTICLE_SUB_TRAIL);
      else
        return LUX_TRUE;
    }
  }

  return LUX_FALSE;

}

static int PubParticleSys_attributes(PState pstate,PubFunction_t *fn, int n)
{
  int resRID;
  int index;
  ParticleSys_t *psys;
  char *name;
  lxVector4 vec;
  float temp;
  float *matrix;
  uchar *pUB;
  int *pI;
  Reference ref;

  byte state;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_PARTICLESYS,(void*)&resRID))
    return FunctionPublish_returnError(pstate,"1 particlesys required");

  if ((int)fn->upvalue > PUBPSYS_INTERPOLATOR_FUNCS && (
    !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&index) || index < 0 || index >= PARTICLE_STEPS))
    return FunctionPublish_returnError(pstate,"1 particlesys 1 int (0..127) required");

  psys = ResData_getParticleSys(resRID);

  switch((int)fn->upvalue) {
  case PUBPSYS_SUBSYSNAMED:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&name))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 string required");
    {
      static char longname[1024];
      static char subname[256];

      strcpy(longname,name);

      if(ParticleSys_subsysString(psys,name,subname,LUX_FALSE))
        return FunctionPublish_returnString(pstate,subname);
    }
    return 0;
    break;
  case PUBPSYS_LAYER:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID,(void*)&index))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 l3dlayerid required");
    psys->l3dlayer = index%LIST3D_LAYERS;
    break;
  case PUBPSYS_INTERCOLOR:
    pUB = (void*)&psys->life.intColorsC[index];
    if (n==2){
      lxVector4float_FROM_ubyte(vec,pUB);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vec));
    }
    else if (4!=FunctionPublish_getArgOffset(pstate,2,4,FNPUB_TOVECTOR4(vec)))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 int 4 floats required");
    lxVector4ubyte_FROM_float(pUB,vec);
    break;
  case PUBPSYS_INTERTEX:
    pI = &psys->life.intTex[index];
    if (n==2) return FunctionPublish_returnInt(pstate,*pI);
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&index))
      return FunctionPublish_returnError(pstate,"1 particlesys 2 int required");
    *pI = index;
    break;
  case PUBPSYS_INTERVEL:
    matrix = &psys->life.intVel[index];
    if (n==2) return FunctionPublish_returnFloat(pstate,*matrix);
    else if (!FunctionPublish_getNArg(pstate,2,FNPUB_TFLOAT(temp)))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 int 1 float required");
    *matrix = temp;
    break;
  case PUBPSYS_INTERSIZE:
    matrix = &psys->life.intSize[index];
    if (n==2) return FunctionPublish_returnFloat(pstate,*matrix);
    else if (!FunctionPublish_getNArg(pstate,2,FNPUB_TFLOAT(temp)))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 int 1 float required");
    *matrix = temp;
    break;
  case PUBPSYS_INTERROT:
    matrix = &psys->life.intRot[index];
    if (n==2) return FunctionPublish_returnFloat(pstate,*matrix);
    else if (!FunctionPublish_getNArg(pstate,2,FNPUB_TFLOAT(temp)))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 int 1 float required");
    *matrix = temp;
    break;
  // enable/disable vis volume
  case PUBPSYS_USEVOL:
    if (n==1) return FunctionPublish_returnBool(pstate,(psys->psysflag & PARTICLE_VOLUME)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 boolean required");
    if (state)
      psys->psysflag |= PARTICLE_VOLUME;
    else
      psys->psysflag &= ~PARTICLE_VOLUME;
    break;
  case PUBPSYS_SINGLENORMAL:
    if (n==1) return FunctionPublish_returnBool(pstate,(psys->container.contflag & PARTICLE_SINGLENORMAL)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 boolean required");
    if (state)
      psys->container.contflag |= PARTICLE_SINGLENORMAL;
    else
      psys->container.contflag &= ~PARTICLE_SINGLENORMAL;
    break;
  case PUBPSYS_FORCEINSTANCE:
    if (n==1) return FunctionPublish_returnBool(pstate,(psys->container.contflag & PARTICLE_FORCEINSTANCE)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 boolean required");
    if (state)
      psys->container.contflag |= PARTICLE_FORCEINSTANCE;
    else
      psys->container.contflag &= ~PARTICLE_FORCEINSTANCE;
    break;
  case PUBPSYS_USECOLORMUL:
    if (n==1) return FunctionPublish_returnBool(pstate,(psys->container.contflag & PARTICLE_COLORMUL)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 boolean required");
    if (state)
      psys->container.contflag |= PARTICLE_COLORMUL;
    else
      psys->container.contflag &= ~PARTICLE_COLORMUL;
    break;
  case PUBPSYS_COLORMUL:
    if (n==1){
      lxVector4float_FROM_ubyte(vec,psys->container.userColor);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vec));
    }
    if (4!=FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(vec)))
      return FunctionPublish_returnError(pstate,"1 particlesys 4 floats required");
    lxVector4ubyte_FROM_float(psys->container.userColor,vec);
    ParticleSys_updateColorMul(psys);
    break;
  case PUBPSYS_USESIZEMUL:
    if (n==1) return FunctionPublish_returnBool(pstate,(psys->container.contflag & PARTICLE_SIZEMUL)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 boolean required");
    if (state)
      psys->container.contflag |= PARTICLE_SIZEMUL;
    else
      psys->container.contflag &= ~PARTICLE_SIZEMUL;
    break;
  case PUBPSYS_SIZEMUL:
    if (n==1) return FunctionPublish_returnFloat(pstate,psys->container.userSize);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(psys->container.userSize)))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 floats required");
    break;
  case PUBPSYS_USEPROBABILITY:
    if (n==1) return FunctionPublish_returnBool(pstate,(psys->container.contflag & PARTICLE_PROBABILITY)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 boolean required");
    if (state)
      psys->container.contflag |= PARTICLE_PROBABILITY;
    else
      psys->container.contflag &= ~PARTICLE_PROBABILITY;
    break;
  case PUBPSYS_PROBFADEOUT:
    if (n==1) return FunctionPublish_returnFloat(pstate,psys->container.userProbabilityFadeOut);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(psys->container.userProbabilityFadeOut)))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 floats required");
    break;
  case PUBPSYS_PROBABILITY:
    if (n==1) return FunctionPublish_returnFloat(pstate,psys->container.userProbability);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(psys->container.userProbability)))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 floats required");
    break;
  case PUBPSYS_VOLSIZE:
    if (n==1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(psys->volumeSize));
    if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(psys->volumeSize)))
      return FunctionPublish_returnError(pstate,"1 particlesys 3 floats required");
    break;
  case PUBPSYS_VOLCAMINFLUENCE:
    if (n==1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(psys->volumeCamInfluence));
    if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(psys->volumeCamInfluence)))
      return FunctionPublish_returnError(pstate,"1 particlesys 3 floats required");
    break;
  case PUBPSYS_VOLOFFSET:
    if (n==1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(psys->volumeOffset));
    if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(psys->volumeOffset)))
      return FunctionPublish_returnError(pstate,"1 particlesys 3 floats required");
    break;
  case PUBPSYS_VOLDIST:
    if (n==1) return FunctionPublish_returnFloat(pstate,psys->volumeDist);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(psys->volumeDist)))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 floats required");
    break;
  case PUBPSYS_TIMESCALE:
    if (n==1) return FunctionPublish_returnFloat(pstate,psys->timescale);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(psys->timescale)))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 floats required");
    break;
  case PUBPSYS_CLEAR: // clear
    ParticleContainer_reset(&psys->container);
    break;
  case PUBPSYS_GETFORCE:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&name))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 string required");
    if (psys->numForces){
      for (resRID = 0; resRID < MAX_PFORCES; resRID++){
        if (psys->forces[resRID] && strcmp(psys->forces[resRID]->name,name)==0){
          SUBRESID_MAKE(resRID,psys->resinfo.resRID);
          return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLEFORCEID,(void*)resRID);
        }
      }
    }
    break;
  case PUBPSYS_PAUSE:
    if (n==1) return FunctionPublish_returnBool(pstate,(psys->psysflag & PARTICLE_PAUSE)!=0);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 boolean required");
    if (state)
      psys->psysflag |= PARTICLE_PAUSE;
    else
      psys->psysflag &= ~PARTICLE_PAUSE;
    break;
  case PUBPSYS_NODRAW:
    if (n==1) return FunctionPublish_returnBool(pstate,(psys->psysflag & PARTICLE_NODRAW)!=0);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 boolean required");
    if (state)
      psys->psysflag |= PARTICLE_NODRAW;
    else
      psys->psysflag &= ~PARTICLE_NODRAW;
    break;
  case PUBPSYS_USEGLOBALAXIS:
    if (n==1) return FunctionPublish_returnBool(pstate,(psys->container.contflag & PARTICLE_GLOBALAXIS)!=0);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 boolean required");
    if (state){
      psys->container.contflag |= PARTICLE_GLOBALAXIS;
      psys->container.contflag &= ~PARTICLE_CAMROTFIX;
    }
    else
      psys->container.contflag &= ~PARTICLE_GLOBALAXIS;
    break;
  case PUBPSYS_GLOBALAXIS:
    if (n==1) return PubMatrix4x4_return(pstate,psys->container.userAxisMatrix);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,matrix))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 matrix4x4 required");
    lxMatrix44Copy(psys->container.userAxisMatrix,matrix);
    break;
  case PUBPSYS_CHECK:
    if (n==1) return FunctionPublish_returnInt(pstate,psys->container.check);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&psys->container.check))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 int required");
    break;
  case PUBPSYS_CHECKMIN:
    if (n==1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(psys->container.checkMin));
    else if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(psys->container.checkMin)))
      return FunctionPublish_returnError(pstate,"1 particlesys 3 floats required");
    break;
  case PUBPSYS_CHECKMAX:
    if (n==1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(psys->container.checkMax));
    else if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(psys->container.checkMax)))
      return FunctionPublish_returnError(pstate,"1 particlesys 3 floats required");
    break;
  case PUBPSYS_NOGPU:
    if (n==1) return FunctionPublish_returnBool(pstate,(psys->container.contflag & PARTICLE_NOGPU)!=0);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 boolean required");
    if (state)
      psys->container.contflag |= PARTICLE_NOGPU;
    else
      psys->container.contflag &= ~PARTICLE_NOGPU;
    break;
  case PUBPSYS_BBOX:
    if (n==1){
      return FunctionPublish_setRet(pstate,6,FNPUB_FROMVECTOR3(psys->bbox.min),FNPUB_FROMVECTOR3(psys->bbox.max));
    }
    else if(6!=FunctionPublish_getArgOffset(pstate,1,6,FNPUB_TOVECTOR3(psys->bbox.min),FNPUB_TOVECTOR3(psys->bbox.max)))
      return FunctionPublish_returnError(pstate,"1 model 6 floats required");
    break;
  case  PUBPSYS_LM:
    if (n==1){
      if (psys->container.lmRID >= 0)return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)psys->container.lmRID);
    }
    else if(!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_TEXTURE,(void*)&psys->container.lmRID))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 texture required");
    break;
  case PUBPSYS_LMPROJMAT:
    if (n==1) return PubMatrix4x4_return(pstate,psys->container.lmProjMat);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,matrix))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 matrix4x4 required");
    lxMatrix44Copy(psys->container.lmProjMat,matrix);
    break;
  case PUBPSYS_VISFLAG:
    if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&index) || index < 0 || index > 31)
      return FunctionPublish_returnError(pstate,"1 particlesys 1 (0..31) int required");
    if (n==2) return FunctionPublish_returnBool(pstate,isFlagSet(psys->container.visflag ,1<<index));
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlesys 1 int (0..31) 1 boolean required");
    if (state)
      psys->container.visflag |= 1<<index;
    else
      psys->container.visflag  &= ~(1<<index);
    break;
  default:
    break;
  }

  return 0;
}

// LUXI_CLASS_PARTICLECLOUD
static int PubParticleCloud_load (PState pstate,PubFunction_t *fn, int n)
{
  int cnt;
  int index;
  char *name;
  int sortkey;

  if (n < 2|| FunctionPublish_getArg(pstate,2,LUXI_CLASS_STRING, &name,LUXI_CLASS_INT,&cnt)!=2)
    return FunctionPublish_returnError(pstate,"1 string 1 int required");

  sortkey = 0;
  FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&sortkey);
  Main_startTask(LUX_TASK_RESOURCE);
  index = ResData_addParticleCloud(name,cnt,sortkey);
  Main_endTask();
  if (index==-1)
    return 0;

  return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLECLOUD,(void*)index);
}

static int PubParticleCloud_renderflag (PState pstate,PubFunction_t *f, int n)
{
  int resRID;
  ParticleCloud_t *pcloud;
  int rf;

  byte state;

  if ((n!=1 && n!=2) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_PARTICLECLOUD,(void*)&resRID))
    return FunctionPublish_returnError(pstate,"1 particlecloud required, optional 1 boolean");

  pcloud = ResData_getParticleCloud(resRID);

  rf = (int)f->upvalue;

  if (n!=2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state)){
    state = isFlagSet(pcloud->container.renderflag,rf);
    return FunctionPublish_returnBool(pstate,state);
  }

  if (state)
    pcloud->container.renderflag |= rf;
  else
    pcloud->container.renderflag &= ~rf;

  return 0;
}

static int PubParticleCloud_type (PState pstate,PubFunction_t *f, int n)
{
  int resRID;
  ParticleCloud_t *pcloud;

  if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_PARTICLECLOUD,(void*)&resRID))
    return FunctionPublish_returnError(pstate,"1 particlecloud required");

  pcloud = ResData_getParticleCloud(resRID);
  pcloud->container.particletype = (int)f->upvalue;

  return 0;
}

enum PubPrtCloudCmd_e{
  PUBPCLD_RANGE,
  PUBPCLD_INSIDE,
  PUBPCLD_ROT,
  PUBPCLD_ROTVEL,
  PUBPCLD_SORT,
  PUBPCLD_COLORSET,
  PUBPCLD_COLOR,
  PUBPCLD_TEX,
  PUBPCLD_NUMTEX,
  PUBPCLD_SMOOTHPOINT,
  PUBPCLD_PPARAMSIZE,
  PUBPCLD_PPARAMDIST,
  PUBPCLD_MODEL,
  PUBPCLD_PSIZE,
  PUBPCLD_CLEAR,
  PUBPCLD_CAMROTFIX,
  PUBPCLD_COLORMUL,
  PUBPCLD_USECOLORMUL,
  PUBPCLD_SIZEMUL,
  PUBPCLD_USESIZEMUL,
  PUBPCLD_PROBABILITY,
  PUBPCLD_USEPROBABILITY,
  PUBPCLD_PROBFADEOUT,
  PUBPCLD_USEGLOBALAXIS,
  PUBPCLD_GLOBALAXIS,
  PUBPCLD_USEORIGINOFFSET,
  PUBPCLD_ORIGINOFFSET,
  PUBPCLD_CHECK,
  PUBPCLD_CHECKMIN,
  PUBPCLD_CHECKMAX,
  PUBPCLD_INSTMESH,
  PUBPCLD_NOGPU,
  PUBPCLD_SINGLENORMAL,
  PUBPCLD_FORCEINSTANCE,
  PUBPCLD_LM,
  PUBPCLD_LMPROJMAT,
  PUBPCLD_VISFLAG,
  PUBPCLD_LAYER,
};

static int PubParticleCloud_attributes(PState pstate,PubFunction_t *f, int n)
{
  int index;
  ParticleCloud_t *pcloud;
  ParticleContainer_t *pcont;
  Reference ref;
  float *matrix;
  lxVector4 vec;
  float temp;
  char *str;

  byte state;

  if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_PARTICLECLOUD,(void*)&index))
    return FunctionPublish_returnError(pstate,"1 particlecloud required");

  pcloud = ResData_getParticleCloud(index);
  pcont = &pcloud->container;

  switch((int)f->upvalue) {
  case PUBPCLD_LAYER:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID,(void*)&index))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 l3dlayerid required");
    pcloud->l3dlayer = index%LIST3D_LAYERS;
    break;
  case PUBPCLD_SINGLENORMAL:
    if (n==1) return FunctionPublish_returnBool(pstate,(pcont->contflag & PARTICLE_SINGLENORMAL)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    if (state)
      pcont->contflag |= PARTICLE_SINGLENORMAL;
    else
      pcont->contflag &= ~PARTICLE_SINGLENORMAL;
    break;
  case PUBPCLD_FORCEINSTANCE:
    if (n==1) return FunctionPublish_returnBool(pstate,(pcont->contflag & PARTICLE_FORCEINSTANCE)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    if (state)
      pcont->contflag |= PARTICLE_FORCEINSTANCE;
    else
      pcont->contflag &= ~PARTICLE_FORCEINSTANCE;
    break;
  case PUBPCLD_NOGPU:
    if (n==1) return FunctionPublish_returnBool(pstate,(pcont->contflag & PARTICLE_NOGPU)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    if (state)
      pcont->contflag |= PARTICLE_NOGPU;
    else
      pcont->contflag &= ~PARTICLE_NOGPU;
    break;
  case PUBPCLD_INSTMESH:
    if (pcloud->container.particletype != PARTICLE_TYPE_MODEL)
      return 0;
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&str))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 string required");

    return FunctionPublish_returnBool(pstate,ParticleContainer_initInstance(pcont,str));
    break;
  case PUBPCLD_RANGE: // range
    if (n==1) return FunctionPublish_returnFloat(pstate,pcloud->range);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&pcloud->range))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 float required");
    break;
  case PUBPCLD_INSIDE:  // inside
    if (n==1) return FunctionPublish_returnBool(pstate,pcloud->insiderange);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&pcloud->insiderange))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    break;
  case PUBPCLD_ROT: // rot
    if (n==1) return FunctionPublish_returnBool(pstate,pcloud->rot);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&pcloud->rot))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    break;
  case PUBPCLD_ROTVEL:  // rotvel
    if (n==1) return FunctionPublish_returnBool(pstate,pcloud->rotvel);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&pcloud->rotvel))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    break;
  case PUBPCLD_SORT:  // sort
    if (n==1) return FunctionPublish_returnBool(pstate,pcloud->sort);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&pcloud->sort))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    break;
  case PUBPCLD_COLORSET:  // colorset
    if (n==1) return FunctionPublish_returnBool(pstate,pcloud->colorset);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&pcloud->colorset))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    break;
  case PUBPCLD_COLOR: // color
    if (n==1) return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(pcont->color));
    else if (FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(pcont->color))!=4)
      return FunctionPublish_returnError(pstate,"1 particlecloud 4 floats required");
    break;
  case PUBPCLD_TEX: // tex
    if (n==1){
      if (vidMaterial(pcont->texRID))
        return FunctionPublish_returnType(pstate,LUXI_CLASS_MATERIAL,(void*)pcont->texRID);
      else if (pcont->texRID >= 0)
        return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)pcont->texRID);
      else
        return 0;
    }
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATSURF,(void*)&pcont->texRID)){
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 matsurface required");
    }
    if (pcont->matobj){
      MaterialObject_free(pcont->matobj);
      pcont->matobj;
    }
    pcont->numTex = 1;
    break;
  case PUBPCLD_NUMTEX:  // numtex
    if (n==1) return FunctionPublish_returnInt(pstate,pcont->numTex);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&pcont->numTex))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 int required");
    break;
  case PUBPCLD_SMOOTHPOINT: // smoothpoint
    if (n==1) return FunctionPublish_returnBool(pstate,isFlagSet(pcont->contflag,PARTICLE_POINTSMOOTH));
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    if (state)
      setFlag(pcont->contflag,PARTICLE_POINTSMOOTH);
    else
      clearFlag(pcont->contflag,PARTICLE_POINTSMOOTH);
    break;
  case PUBPCLD_PPARAMSIZE:  // pointparamssize
    if (n==1){
      return FunctionPublish_setRet(pstate,3,FNPUB_FFLOAT(pcont->pparams.min),FNPUB_FFLOAT(pcont->pparams.max),FNPUB_FFLOAT(pcont->pparams.alphathresh));
    }
    else if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TFLOAT(pcont->pparams.min),FNPUB_TFLOAT(pcont->pparams.max),FNPUB_TFLOAT(pcont->pparams.alphathresh)))
      return FunctionPublish_returnError(pstate,"1 particlecloud 3 floats required");
    break;
  case PUBPCLD_PPARAMDIST:  // pointparamsdist
    if (n==1){
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(pcont->pparams.dist));
    }
    else if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(pcont->pparams.dist)))
      return FunctionPublish_returnError(pstate,"1 particlecloud 3 floats required");
    break;
  case PUBPCLD_MODEL: // model
    if (n==1){
      if (pcont->modelRID >= 0)
        return FunctionPublish_setRet(pstate,1,LUXI_CLASS_MODEL,pcont->modelRID);
    }
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MODEL,(void*)&pcont->modelRID))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 model required");
    break;
  case PUBPCLD_PSIZE: // pointsize
    if (n==1){
      return FunctionPublish_returnFloat(pstate,pcont->size);
    }
    else if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(temp)) || temp < 0.1)
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 float (>= 0.1) required");
    pcont->size = temp;
    break;
  case PUBPCLD_CLEAR: // clear
    ParticleContainer_reset(pcont);
    break;
  case PUBPCLD_CAMROTFIX: // camrotfix
    if (n==1){
      return FunctionPublish_returnBool(pstate,isFlagSet(pcont->contflag,PARTICLE_CAMROTFIX));
    }
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    if (state)
      pcont->contflag |= PARTICLE_CAMROTFIX;
    else
      pcont->contflag &= ~PARTICLE_CAMROTFIX;
    break;
  case PUBPCLD_USEGLOBALAXIS:
    if (n==1) return FunctionPublish_returnBool(pstate,(pcont->contflag & PARTICLE_GLOBALAXIS)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    if (state){
      pcont->contflag |= PARTICLE_GLOBALAXIS;
      pcont->contflag &= ~PARTICLE_CAMROTFIX;
    }
    else
      pcont->contflag &= ~PARTICLE_GLOBALAXIS;
    break;
  case PUBPCLD_GLOBALAXIS:
    if (n==1) return PubMatrix4x4_return(pstate,pcont->userAxisMatrix);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,matrix))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 matrix4x4 required");
    lxMatrix44Copy(pcont->userAxisMatrix,matrix);
    break;
  case PUBPCLD_USEORIGINOFFSET:
    if (n==1) return FunctionPublish_returnBool(pstate,(pcont->contflag & PARTICLE_ORIGINOFFSET)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    if (state)
      pcont->contflag |= PARTICLE_ORIGINOFFSET;
    else
      pcont->contflag &= ~PARTICLE_ORIGINOFFSET;
    break;
  case PUBPCLD_ORIGINOFFSET:
    if (n==1){
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(pcont->userOffset));
    }
    if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(vec)))
      return FunctionPublish_returnError(pstate,"1 particlecloud 3 floats required");
    lxVector3Copy(pcont->userOffset,vec);
    break;
  case PUBPCLD_USECOLORMUL:
    if (n==1) return FunctionPublish_returnBool(pstate,(pcont->contflag & PARTICLE_COLORMUL)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    if (state)
      pcont->contflag |= PARTICLE_COLORMUL;
    else
      pcont->contflag &= ~PARTICLE_COLORMUL;
    break;
  case PUBPCLD_COLORMUL:
    if (n==1){
      lxVector4float_FROM_ubyte(vec,pcont->userColor);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vec));
    }
    if (4!=FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(vec)))
      return FunctionPublish_returnError(pstate,"1 particlecloud 4 floats required");
    lxVector4ubyte_FROM_float(pcont->userColor,vec);
    break;
  case PUBPCLD_USESIZEMUL:
    if (n==1) return FunctionPublish_returnBool(pstate,(pcont->contflag & PARTICLE_SIZEMUL)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    if (state)
      pcont->contflag |= PARTICLE_SIZEMUL;
    else
      pcont->contflag &= ~PARTICLE_SIZEMUL;
    break;
  case PUBPCLD_SIZEMUL:
    if (n==1) return FunctionPublish_returnFloat(pstate,pcont->userSize);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(pcont->userSize)))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 floats required");
    break;
  case PUBPCLD_USEPROBABILITY:
    if (n==1) return FunctionPublish_returnBool(pstate,(pcont->contflag & PARTICLE_PROBABILITY)!=0);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 boolean required");
    if (state)
      pcont->contflag |= PARTICLE_PROBABILITY;
    else
      pcont->contflag &= ~PARTICLE_PROBABILITY;
    break;
  case PUBPCLD_PROBFADEOUT:
    if (n==1) return FunctionPublish_returnFloat(pstate,pcont->userProbabilityFadeOut);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(pcont->userProbabilityFadeOut)))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 floats required");
    break;
  case PUBPCLD_PROBABILITY:
    if (n==1) return FunctionPublish_returnFloat(pstate,pcont->userProbability);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(pcont->userProbability)))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 floats required");
    break;
  case PUBPCLD_CHECK:
    if (n==1) return FunctionPublish_returnInt(pstate,pcont->check);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&pcont->check))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 int required");
    break;
  case PUBPCLD_CHECKMIN:
    if (n==1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(pcont->checkMin));
    if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(pcont->checkMin)))
      return FunctionPublish_returnError(pstate,"1 particlecloud 3 floats required");
    break;
  case PUBPCLD_CHECKMAX:
    if (n==1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(pcont->checkMax));
    if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(pcont->checkMax)))
      return FunctionPublish_returnError(pstate,"1 particlecloud 3 floats required");
    break;
  case  PUBPCLD_LM:
    if (n==1) {
      if (pcont->lmRID >= 0)return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)pcont->lmRID);
    }
    else if(!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_TEXTURE,(void*)&pcont->lmRID))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 texture required");
    break;
  case PUBPCLD_LMPROJMAT:
    if (n==1) return PubMatrix4x4_return(pstate,pcont->lmProjMat);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,matrix))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 matrix4x4 required");
    lxMatrix44Copy(pcont->lmProjMat,matrix);
    break;
  case PUBPCLD_VISFLAG:
    if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&index) || index < 0 || index > 31)
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 (0..31) int required");
    if (n==2) return FunctionPublish_returnBool(pstate,isFlagSet(pcloud->container.visflag ,1<<index));
    else if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 particlecloud 1 int (0..31) 1 boolean required");
    if (state)
      pcloud->container.visflag |= 1<<index;
    else
      pcloud->container.visflag  &= ~(1<<index);
    break;
  default:
    break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_L3D_PEMITTER

static int PubL3DPEmitter_new (PState pstate,PubFunction_t *f, int n)
{
  int layer;
  int psys;
  char *name;
  List3DNode_t *l3d;

  layer = List3D_getDefaultLayer();


  if (n<2 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,&name) || !FunctionPublish_getNArg(pstate,1+FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID,(void*)&layer),LUXI_CLASS_PARTICLESYS,(void*)&psys))
    return FunctionPublish_returnError(pstate,"1 string [1 l3dlayerid] 1 particlesys required");

  l3d = List3DNode_newParticleEmitterRes(name,layer,psys,0);
  if (l3d==NULL)
    return FunctionPublish_returnError(pstate,"could not create l3dnode (make sure particlesys has no active emitters in another l3dset)");

  Reference_makeVolatile(l3d->reference);
  return FunctionPublish_setRet(pstate,1,LUXI_CLASS_L3D_PEMITTER,REF2VOID(l3d->reference));
}

ParticleEmitter_t* ParticleEmitter_fromSubString(ParticleEmitter_t *startemitter,char *str)
{
  ParticleEmitter_t* cur;
  int index = isdigit(str[0]) ? str[0]-'0' : -1;

  if (!startemitter || index < 0 || index >= startemitter->numSubsys)
    return NULL;

  cur = startemitter->subsysEmitters[index];

  // mega ugly, trails
  if (!cur){
    ParticleSys_t *psys = ResData_getParticleSys(ResData_getParticleSys(startemitter->psysRID)->subsys[index].psysRID);
    cur = &psys->emitterDefaults;
  }

  if (!str[1])
    return cur;
  else
    return ParticleEmitter_fromSubString(cur,str+1);
}

static int PubL3DPEmitter_type (PState pstate,PubFunction_t *f, int n)
{
  static char subname[256];

  Reference ref;
  List3DNode_t *l3d;
  ParticleEmitter_t *pemitter;
  int attr;
  int offset;
  int intval;
  lxVector2 vec;


  if ((n<1) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_PEMITTER,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dpemitter required");


  pemitter = l3d->pemitter;
  offset = 1;
  if (n>1 && FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&subname) && (offset=2) && !(pemitter=ParticleEmitter_fromSubString(l3d->pemitter,subname)))
    return FunctionPublish_returnError(pstate,"invalid subsys string, must only contain digits within subsys counts of each emitter");


  attr = (int)f->upvalue;
  intval = 2;
  switch(attr)
  {
  case PARTICLE_EMITTER_CIRCLE:
  case PARTICLE_EMITTER_SPHERE:
    if (n < offset+1 || !FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_FLOAT,(void*)&vec[0]))
      return FunctionPublish_returnError(pstate,"1 l3dpemitter [string] 1 float required");
    pemitter->size = vec[0];
    break;
  case PARTICLE_EMITTER_RECTANGLE:
  case PARTICLE_EMITTER_RECTANGLELOCAL:
    if (n < offset+1 || 2>FunctionPublish_getArgOffset(pstate,offset,3,FNPUB_TOVECTOR2(vec),LUXI_CLASS_INT,(void*)&intval) ||
      intval < 0 || intval > 2)
      return FunctionPublish_returnError(pstate,"1 l3dpemitter [string] 2 floats [1 int (0-2)] required");
    pemitter->size = vec[0];
    pemitter->size2 = vec[1];
    pemitter->axis = intval;
    break;
  case PARTICLE_EMITTER_MODEL:
    if (n < offset+2 || 2!=FunctionPublish_getArgOffset(pstate,offset,2,LUXI_CLASS_MODEL,(void*)&intval,LUXI_CLASS_FLOAT,(void*)&vec[0]))
      return FunctionPublish_returnError(pstate,"1 l3dpemitter [string] 1 model 1 float required");
    pemitter->modelRID = intval;
    pemitter->size = vec[0];
    break;
  default:
    break;
  }
  pemitter->emittertype = attr;

  return 0;
}

enum PubPEmCmds_e
{
  PUBEM_SPREADIN,
  PUBEM_SPREADOUT,
  PUBEM_VELOCITY,
  PUBEM_VELOCITYVAR,
  PUBEM_RATE,
  PUBEM_SIZE,
  PUBEM_SIZE2,
  PUBEM_FLIP,
  PUBEM_PRTSIZE,
  PUBEM_OFFSET,
  PUBEM_PSYS,

  PUBEM_INT_FUNCS,

  PUBEM_GETSUBCNT,
  PUBEM_AXIS,
  PUBEM_STARTAGE,

  PUBEM_VECTOR_FUNCS,
  PUBEM_STARTVEL,
};

static int PubL3DPEmitter_attributes(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  float value;
  int valueint;
  List3DNode_t *l3d;
  int attr;
  char *subname;
  ParticleEmitter_t *pemitter;
  int offset;
  lxVector4 vec;

  if ((n<1) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_PEMITTER,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dpemitter required, optional 1 float");


  pemitter = l3d->pemitter;
  offset = 1;
  if (n>1 && FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&subname) && (offset=2) && !(pemitter=ParticleEmitter_fromSubString(l3d->pemitter,subname)))
    return FunctionPublish_returnError(pstate,"invalid subsys string, must only contain digits within subsys counts of each emitter");

  attr = (int)f->upvalue;

  if ((n == offset+1) && (  (attr < PUBEM_INT_FUNCS && !(FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_FLOAT,(void*)&value))) ||
          (attr > PUBEM_INT_FUNCS && !(FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_INT,(void*)&valueint)))
        ))
    return FunctionPublish_returnError(pstate,"1 l3dpemitter [string] [1 float/int] required");

  if (attr > PUBEM_VECTOR_FUNCS && n > (offset+3) && FunctionPublish_getArgOffset(pstate,offset,4,FNPUB_TOVECTOR4(vec))<3)
    return FunctionPublish_returnError(pstate,"1 l3dpemitter [string] [3/4 floats] required");

  switch(attr) {
  case PUBEM_STARTVEL:
    if (n==offset) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(pemitter->offsetVelocity));
    lxVector3Copy(pemitter->offsetVelocity,vec);
    break;
  case PUBEM_GETSUBCNT:
    return FunctionPublish_returnInt(pstate,pemitter->numSubsys);
  case PUBEM_PSYS:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLESYS,(void*)pemitter->psysRID);
  case PUBEM_SPREADIN:
    if (n==offset)  return FunctionPublish_returnFloat(pstate,pemitter->spreadin);
    else      pemitter->spreadin = value;
    break;
  case PUBEM_SPREADOUT:
    if (n==offset)  return FunctionPublish_returnFloat(pstate,pemitter->spreadout);
    else      pemitter->spreadout = value;
    break;
  case PUBEM_VELOCITY:
    if (n==offset)  return FunctionPublish_returnFloat(pstate,pemitter->velocity);
    else      pemitter->velocity = value;
    break;
  case PUBEM_VELOCITYVAR:
    if (n==offset)  return FunctionPublish_returnFloat(pstate,pemitter->velocityVar);
    else      pemitter->velocityVar = value;
    break;
  case PUBEM_RATE:
    if (n==offset)  return FunctionPublish_returnFloat(pstate,pemitter->rate*1000);
    else      pemitter->rate = value*0.001;
    break;
  case PUBEM_SIZE:
    if (n==offset)  return FunctionPublish_returnFloat(pstate,pemitter->size);
    else      pemitter->size = value;
    break;
  case PUBEM_SIZE2:
    if (n==offset)  return FunctionPublish_returnFloat(pstate,pemitter->size2);
    else      pemitter->size2 = value;
    break;
  case PUBEM_AXIS:
    if (n==offset)  return FunctionPublish_returnInt(pstate,pemitter->axis);
    else      pemitter->axis = valueint;
    break;
  case PUBEM_FLIP:
    if (n==offset)  return FunctionPublish_returnFloat(pstate,pemitter->flipdirection);
    else      pemitter->flipdirection = value;
    break;
  case PUBEM_STARTAGE:
    if (n==offset)  return FunctionPublish_returnInt(pstate,pemitter->startage);
    else      pemitter->startage = LUX_MAX(valueint,1);
    break;
  case PUBEM_PRTSIZE:
    if (n==offset)  return FunctionPublish_returnFloat(pstate,pemitter->prtsize);
    else      pemitter->prtsize = value;
    break;
  case PUBEM_OFFSET:
    if (n==offset)  return FunctionPublish_returnFloat(pstate,pemitter->offset);
    else      pemitter->offset = value;
    break;
  default:
    break;
  }

  return 0;
}

static int PubL3DPEmitter_start(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  int value;
  List3DNode_t *l3d;

  if (n!=2 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_L3D_PEMITTER,(void*)&ref,LUXI_CLASS_INT,(void*)&value)!=2 || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dpemitter 1 int required");

  ParticleEmitter_start(l3d->pemitter,value);
  return 0;
}

static int PubL3DPEmitter_end(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  int value;
  List3DNode_t *l3d;

  bool8 bol;

  if (n!=3 || FunctionPublish_getArg(pstate,3,LUXI_CLASS_L3D_PEMITTER,(void*)&ref,LUXI_CLASS_INT,(void*)&value,LUXI_CLASS_BOOLEAN,(void*)&bol)!=3 || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dpemitter 1 int 1 boolean required");

  ParticleEmitter_end(l3d->pemitter,value,bol);
  return 0;
}

// LUXI_CLASS_L3D_PGROUP
static int PubL3DPGroup_new (PState pstate,PubFunction_t *f, int n)
{
  int layer;
  int pcloud;
  char *name;
  List3DNode_t *l3d;

  layer = List3D_getDefaultLayer();

  if (n<2 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,&name) || !FunctionPublish_getNArg(pstate,1+FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LAYERID,(void*)&layer),LUXI_CLASS_PARTICLECLOUD,(void*)&pcloud))
    return FunctionPublish_returnError(pstate,"1 string [1 l3dlayerid] 1 particlecloud required");

  l3d = List3DNode_newParticleGroupRes(name,layer,pcloud,0);
  if (l3d==NULL)
    return FunctionPublish_returnError(pstate,"could not create l3dnode (make sure particlecloud has no active groups in another l3dset)");

  Reference_makeVolatile(l3d->reference);
  return FunctionPublish_setRet(pstate,1,LUXI_CLASS_L3D_PGROUP,REF2VOID(l3d->reference));
}

static int PubL3DPGroup_type(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  int attr;

  if ((n!=1) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_PGROUP,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dpgroup required");

  attr = (int)f->upvalue;

  ParticleGroup_reset(l3d->pgroup);

  l3d->pgroup->type = attr;
  l3d->pgroup->shortnormals = LUX_FALSE;

  return 0;
}


static int PubL3DPGroup_typebones(PState pstate,PubFunction_t *f, int n)
{
  static char* names[256];
  Reference ref;
  Reference ref2;
  List3DNode_t *l3d;
  List3DNode_t *l3dmodel;
  int axis;
  int i;
  int boneid;
  int numbones;
  ParticleStatic_t *prt;
  ParticleList_t *plist;
  float *bonematrix;
  Model_t *model;

  PubArray_t strstack;

  if ( FunctionPublish_getArg(pstate,4,LUXI_CLASS_L3D_PGROUP,(void*)&ref,LUXI_CLASS_L3D_PGROUP,(void*)&ref2,LUXI_CLASS_INT,(void*)&axis,LUXI_CLASS_ARRAY_STRING,(void*)&strstack)!=4
    || !Reference_get(ref,l3d) || !Reference_get(ref2,l3dmodel) || axis < 0 || axis > 2)
    return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 l3dmodel 1 int(0-2) 1 stringarray required");

  ParticleGroup_reset(l3d->pgroup);

  l3d->pgroup->type = PARTICLE_GROUP_LOCAL_REF_ALL;
  l3d->pgroup->targetref = ref2;
  l3d->pgroup->shortnormals = LUX_FALSE;

  model = ResData_getModel(l3dmodel->minst->modelRID);

  strstack.length = LUX_MIN(256,strstack.length);
  FunctionPublish_fillArray(pstate,3,LUXI_CLASS_ARRAY_STRING,&strstack,names);

  numbones = strstack.length;
  for (i = 0; i < numbones; i++){
    if (!names[i])
      return FunctionPublish_returnError(pstate,"table must only contain strings");

    boneid = BoneSys_searchBone(&model->bonesys,names[i]);
    if (boneid == -1)
      continue;

    plist = ParticleGroup_addParticle(l3d->pgroup);
    if (!plist)
      break;

    prt = plist->prtSt;

    if (l3dmodel->minst->boneupd)
      bonematrix = &l3dmodel->minst->boneupd->bonesAbs[boneid*16];
    else
      bonematrix = &model->bonesys.absMatrices[boneid*16];

    prt->pPos = &bonematrix[12];
    prt->pNormal = &bonematrix[axis*4];
  }

  return 0;
}

static int PubL3DPGroup_typeverts(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  Reference ref2;
  List3DNode_t *l3d;
  List3DNode_t *l3dmodel;
  int i;
  int meshid;
  int spawncnt;
  int vertexcnt;
  int *permtable;
  int *pInt;
  float percent;
  ModelUpdate_t *modelupd;
  ParticleList_t *plist;
  lxVertexType_t vtype;
  Mesh_t *mesh;
  lxVertex64_t *vert64;
  lxVertex32_t *vert32;
  Model_t *model;

  if ( FunctionPublish_getArg(pstate,3,LUXI_CLASS_L3D_PGROUP,(void*)&ref,LUXI_CLASS_L3D_PGROUP,(void*)&ref2,LUXI_CLASS_MESHID,(void*)&meshid, LUXI_CLASS_FLOAT,(void*)&percent)!=4
    || !Reference_get(ref,l3d) || !Reference_get(ref2,l3dmodel) || percent < 0 || percent > 1)
    return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 l3dmodel 1 meshid 1 float(0-1) required");

  SUBRESID_MAKEPURE(meshid);
  ParticleGroup_reset(l3d->pgroup);

  l3d->pgroup->type = PARTICLE_GROUP_LOCAL_REF_ALL;
  l3d->pgroup->targetref = ref2;
  l3d->pgroup->shortnormals = LUX_FALSE;

  Reference_refWeak(ref2);


  model = ResData_getModel(l3dmodel->minst->modelRID);

  vtype = model->vertextype;


  // HACK vertex_32 has no normals use object's z axis
  if (l3dmodel->minst->modelupd){
    modelupd = l3dmodel->minst->modelupd;

    vertexcnt = modelupd->vertexcont->numVertices[meshid];
    vert32 = (lxVertex32_t*)( vert64 = modelupd->vertexcont->vertexDatas[meshid]);
  }
  else{
    mesh = model->meshObjects[meshid].mesh;

    vertexcnt = mesh->numVertices;
    vert32 = (lxVertex32_t*) (vert64 = mesh->vertexData);
  }


  if (percent > 0.99){

    if (vtype == VERTEX_64_TEX4 || vtype == VERTEX_64_SKIN){
      l3d->pgroup->shortnormals = LUX_TRUE;
      for (i=0; i < vertexcnt; i++,vert64++){
        plist = ParticleGroup_addParticle(l3d->pgroup);
        if (!plist)
          break;
        plist->prtSt->pPos = vert64->pos;
        plist->prtSt->pNormal = (float*)vert64->normal;
      }
    }
    else
      for (i=0; i < vertexcnt; i++,vert32++){
        plist = ParticleGroup_addParticle(l3d->pgroup);
        if (!plist)
          break;
        plist->prtSt->pPos = vert32->pos;
        plist->prtSt->pNormal = &l3d->finalMatrix[8];
      }

  }
  else{
    bufferclear();
    pInt = permtable = buffermalloc(sizeof(int)*vertexcnt);
    lxPermutation(permtable,vertexcnt);
    spawncnt = (int)((float)vertexcnt*percent);

    if (vtype == VERTEX_64_TEX4 || vtype == VERTEX_64_SKIN)
      for (i=0; i < spawncnt; i++,pInt++){
        plist = ParticleGroup_addParticle(l3d->pgroup);
        if (!plist)
          break;
        plist->prtSt->pPos = vert64[*pInt].pos;
        plist->prtSt->pNormal = (float*)vert64[*pInt].normal;
      }
    else
      for (i=0; i < spawncnt; i++,pInt++){
        plist = ParticleGroup_addParticle(l3d->pgroup);
        if (!plist)
          break;
        plist->prtSt->pPos = vert32[*pInt].pos;
        plist->prtSt->pNormal = &l3d->finalMatrix[8];
      }

  }


  return 0;
}

static int PubL3DPGroup_spawn(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  ParticleList_t *plist;
  lxVector3 pos;
  lxVector3 normal;

  lxVector3Set(normal,0,0,1);
  if ((n<4) || FunctionPublish_getArg(pstate,7,LUXI_CLASS_L3D_PGROUP,(void*)&ref,FNPUB_TOVECTOR3(pos),FNPUB_TOVECTOR3(normal))<4 || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dpgroup 3 floats required");

  switch(l3d->pgroup->type) {
  case PARTICLE_GROUP_LOCAL:
  case PARTICLE_GROUP_WORLD:
    break;
  default:
    return FunctionPublish_returnError(pstate,"l3dgroup must be local/world type.");
  }

  plist = ParticleGroup_addParticle(l3d->pgroup);

  if (plist){
    lxVector3Copy(plist->prtSt->pos,pos);
    lxVector3Copy(plist->prtSt->normal,normal);
    return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLE,plist);
  }
  else
    return 0;
}


static int PubL3DPGroup_spawnref(PState pstate,PubFunction_t *f, int n){
  Reference ref;
  Reference refsn;
  Reference refact;
  List3DNode_t *l3d;
  ParticleList_t *plist;
  float *matrix;
  int axis;
  SceneNode_t* s3d;
  ActorNode_t* act;
  ParticleStatic_t *prt;

  Reference_reset(refsn);
  Reference_reset(refact);
  if ((n!=3) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_PGROUP,(void*)&ref) ||
    !(  FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SCENENODE,(void*)&refsn) ||
    FunctionPublish_getNArg(pstate,1,LUXI_CLASS_ACTORNODE,(void*)&refact)) ||
    !FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&axis) ||
    !Reference_get(ref,l3d) || axis < 0 || axis > 2)
    return FunctionPublish_returnError(pstate,"1 valid l3dpgroup 1 valid actor/scenenode 1 int(0-2) required");

  if (!l3d->pgroup->type == PARTICLE_GROUP_WORLD_REF_EACH){
    return FunctionPublish_returnError(pstate,"l3dpgroup must be of typeworldref");
  }

  matrix = NULL;
  s3d = NULL;
  act = NULL;
  if (Reference_get(refsn,s3d)){
    matrix = s3d->link.matrix;
    ref = refsn;
  }
  else if (Reference_get(refact,act)) {
    matrix = act->link.matrix;
    ref = refact;
  } else
    return FunctionPublish_returnError(pstate,"1 valid l3dpgroup 1 valid actor/scenenode 1 int required");


  plist = ParticleGroup_addParticle(l3d->pgroup);

  if (plist){
    prt = plist->prtSt;

    Reference_refWeak(ref);

    prt->linkRef = ref;
    if (act)
      prt->linkType = LINK_ACT;
    else
      prt->linkType = LINK_S3D;

    prt->axis = axis;
    prt->pPos = &matrix[12];
    prt->pNormal = &matrix[axis*4];
    return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLE,plist);
  }
  else
    return 0;

  return 0;
}

static int PubL3DPGroup_first(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;

  if ((n!=1) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L3D_PGROUP,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dpgroup required");

  if (l3d->pgroup->particles)
    return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLE,l3d->pgroup->particles);
  return 0;
}

static int PubL3DPGroup_remove(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  ParticleList_t *plist;

  if ((n!=2) || !FunctionPublish_getArg(pstate,2,LUXI_CLASS_L3D_PGROUP,(void*)&ref,LUXI_CLASS_PARTICLE,(void*)&plist) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 particle required");

  if (!ParticleCloud_checkParticle(l3d->pgroup->pcloud,plist))
    return FunctionPublish_returnError(pstate,"particle not valid");

  ParticleGroup_removeParticle(l3d->pgroup,plist);
  return 0;
}

enum PubPGrpCmds_e{
  PUBPGRP_CNT,
  PUBPGRP_ROT,
  PUBPGRP_SCALE,
  PUBPGRP_SEQ,
  PUBPGRP_SIZEVAR,
  PUBPGRP_ROTVAR,
  PUBPGRP_USECOLOR,
  PUBPGRP_COLORVAR,
  PUBPGRP_USEDRAWLIST,
  PUBPGRP_DRAWHEAD,
  PUBPGRP_DRAWTAIL,
  PUBPGRP_REMOVEMULTI,
  PUBPGRP_UPDATEPTS,
  PUBPGRP_PCLD,
};

#define PARTICLE_MULTIDELETE_MAX    6

static int PubL3DPGroup_auto(PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List3DNode_t *l3d;
  ParticleList_t *plist;
  char *cmds;
  int cur;
  enum32 cmpmodes[PARTICLE_MULTIDELETE_MAX];
  int curref;
  float cmprefs[PARTICLE_MULTIDELETE_MAX*4];

  if ((n==0) || !FunctionPublish_getArg(pstate,1,LUXI_CLASS_L3D_PGROUP,(void*)&ref) || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dpgroup required");

  switch((int)f->upvalue) {
  case PUBPGRP_PCLD:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLECLOUD,(void*)l3d->pgroup->pcloudRID);
  case PUBPGRP_CNT:
    return FunctionPublish_returnInt(pstate,l3d->pgroup->numParticles);
    break;
  case PUBPGRP_USEDRAWLIST:
    if (n==1) return FunctionPublish_returnBool(pstate,l3d->pgroup->usedrawlist);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&l3d->pgroup->usedrawlist))
      return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 boolean required");

    break;
  case PUBPGRP_ROT: // autorot
    if (n==1) return FunctionPublish_returnFloat(pstate,l3d->pgroup->autorot);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&l3d->pgroup->autorot))
      return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 float required");

    break;
  case PUBPGRP_SCALE: // autoscale
    if (n==1) return FunctionPublish_returnFloat(pstate,l3d->pgroup->autoscale);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&l3d->pgroup->autoscale))
      return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 float required");

    break;
  case PUBPGRP_SEQ: // autseq
    if (n==1) return FunctionPublish_returnInt(pstate,l3d->pgroup->autoseq);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&l3d->pgroup->autoseq))
      return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 int required");

    break;
  case PUBPGRP_SIZEVAR: // sizevar
    if (n==1) return FunctionPublish_returnFloat(pstate,l3d->pgroup->sizeVar);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&l3d->pgroup->sizeVar))
      return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 float required");

    break;
  case PUBPGRP_ROTVAR:  // rotvar
    if (n==1) return FunctionPublish_returnFloat(pstate,l3d->pgroup->rotVar);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&l3d->pgroup->rotVar))
      return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 float required");
    break;
  case PUBPGRP_USECOLOR:  // usecolor
    if (n==1) return FunctionPublish_returnBool(pstate,l3d->pgroup->usecolorVar);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&l3d->pgroup->usecolorVar))
      return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 boolean required");

    break;
  case PUBPGRP_COLORVAR:  // colorvar
    if (n==1) return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(l3d->pgroup->colorVar));
    else if (4!=FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(l3d->pgroup->colorVar)))
      return FunctionPublish_returnError(pstate,"1 l3dpgroup 4 floats required");

    break;
  case PUBPGRP_DRAWHEAD:  // colorvar
    if (n==1) return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLE,(void*)l3d->pgroup->drawListHead);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_PARTICLE,(void*)&plist) || !ParticleCloud_checkParticle(l3d->pgroup->pcloud,plist))
      return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 particle required");

    l3d->pgroup->drawListHead = plist;

    break;
  case PUBPGRP_DRAWTAIL:  // colorvar
    if (n==1) return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLE,(void*)l3d->pgroup->drawListTail);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_PARTICLE,(void*)&plist) || !ParticleCloud_checkParticle(l3d->pgroup->pcloud,plist))
      return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 particle required");

    l3d->pgroup->drawListTail = plist;
    break;
  case PUBPGRP_REMOVEMULTI:
    if (n < 4 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&cmds))
      return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 string + ...comparemode/refvalue required ");

    cur = 0;
    curref = 0;
    while (cmds[cur]){
      switch(cmds[cur])
      {
      case 'd':
        if (5>FunctionPublish_getArgOffset(pstate,cur+curref+2,5,LUXI_CLASS_COMPAREMODE,(void*)&cmpmodes[cur],FNPUB_TOVECTOR4((&cmprefs[curref]))))
          return FunctionPublish_returnError(pstate,"1 comparemode 4 floats required");
        cmprefs[curref+3]*=cmprefs[curref+3];
        curref+=4;
        break;
      case 's':
        if (2>FunctionPublish_getArgOffset(pstate,cur+curref+2,2,LUXI_CLASS_COMPAREMODE,(void*)&cmpmodes[cur],FNPUB_TFLOAT(cmprefs[curref])))
          return FunctionPublish_returnError(pstate,"1 comparemode 1 floats required");
        curref++;
        break;
      case 't':
        if (2>FunctionPublish_getArgOffset(pstate,cur+curref+2,2,LUXI_CLASS_COMPAREMODE,(void*)&cmpmodes[cur],FNPUB_TFLOAT(cmprefs[curref])))
          return FunctionPublish_returnError(pstate,"1 comparemode 1 floats required");
        curref++;
        break;
      default:
        return FunctionPublish_returnError(pstate,"illegal remove flag in command string");
      }
      cur++;
    }

    return FunctionPublish_returnInt(pstate,ParticleGroup_removeParticlesMulti(l3d->pgroup,cmds,cmpmodes,cmprefs));
    break;
  case PUBPGRP_UPDATEPTS:
    l3d->pgroup->pcloud->container.activeStaticCur = l3d->pgroup->pcloud->container.stptrs;
    *l3d->pgroup->pcloud->container.activeStaticCur = NULL;
    ParticleGroup_update(l3d->pgroup,l3d->finalMatrix,l3d->renderscale);
    l3d->pgroup->pcloud->container.activeStaticCur = l3d->pgroup->pcloud->container.stptrs;
    *l3d->pgroup->pcloud->container.activeStaticCur = NULL;
    break;
  default:
    break;
  }

  return 0;
}

// LUXI_CLASS_PARTICLE

enum PubPrtCmd_e{
  PUBPRT_POS,
  PUBPRT_NORMAL,
  PUBPRT_WPOS,
  PUBPRT_WNORMAL,
  PUBPRT_COLOR,
  PUBPRT_SIZE,
  PUBPRT_SEQ,
  PUBPRT_ROT,
  PUBPRT_NEXT,
  PUBPRT_PREV,
  PUBPRT_SWAP,
};

static int PubParticle_attributes(PState pstate,PubFunction_t *f, int n)
{
  ParticleList_t *plist;
  ParticleList_t *plist2;
  ParticleStatic_t  *prt;
  Reference ref;
  List3DNode_t *l3d;
  lxVector4 color;
  int texid;
  int attr;
  int mode;
  ParticleStatic_t prtupd;

  if (FunctionPublish_getArg(pstate,2,LUXI_CLASS_L3D_PGROUP,(void*)&ref,LUXI_CLASS_ANY,(void*)&plist)!=2 || !Reference_get(ref,l3d))
    return FunctionPublish_returnError(pstate,"1 l3dpgroup 1 particle/int(0,1) required");

  mode = (int)plist;

  if (!(mode == 0 || mode == 1) && !ParticleCloud_checkParticle(l3d->pgroup->pcloud,plist))
    return FunctionPublish_returnError(pstate,"particle not valid");

  if (mode == 0){
    prt = &l3d->pgroup->prtDefaults;
  }
  else if (mode == 1){
    prt = &prtupd;
  }
  else
    prt = plist->prtSt;

  attr = (int)f->upvalue;

  if (attr <= PUBPRT_NORMAL && l3d->pgroup->type != PARTICLE_GROUP_LOCAL &&
          l3d->pgroup->type != PARTICLE_GROUP_WORLD)
    return FunctionPublish_returnError(pstate,"pos and normal not allowed for referenced particles");

  switch(attr) {
  case PUBPRT_POS:  // pos
    if (n ==5){
      if( FunctionPublish_getArgOffset(pstate,2,3,FNPUB_TOVECTOR3(prt->pos))!=3)
        return FunctionPublish_returnError(pstate,"3 floats needed");
    }
    else  return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(prt->pos));
    break;
  case PUBPRT_WPOS: // pos
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(prt->viewpos));
    break;
  case PUBPRT_WNORMAL:  // normal
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(prt->viewnormal));
    break;
  case PUBPRT_NORMAL: // normal
    if (n ==5){
      if( FunctionPublish_getArgOffset(pstate,2,3,FNPUB_TOVECTOR3(prt->normal))!=3)
        return FunctionPublish_returnError(pstate,"3 floats needed");
    }
    else  return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(prt->normal));
    break;
  case PUBPRT_COLOR:  // color
    if (n > 5){
      if(FunctionPublish_getArgOffset(pstate,2,4,FNPUB_TOVECTOR4(color))<4)
        return FunctionPublish_returnError(pstate,"3 floats needed");
      lxVector4ubyte_FROM_float(prt->color,color);
      if (mode == 1){
        plist = l3d->pgroup->particles;
        while (plist){
          LUX_ARRAY4COPY(plist->prtSt->color,prt->color);
          plist = plist->next;
        }
      }
    }
    else{
      lxVector4float_FROM_ubyte(color,prt->color);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(color));
    }
    break;
  case PUBPRT_SIZE: // size
    if (n ==3){
      if( FunctionPublish_getArgOffset(pstate,2,1,LUXI_CLASS_FLOAT,&prt->size)!=1)
        return FunctionPublish_returnError(pstate,"1 float needed");
      if (mode == 1){
        plist = l3d->pgroup->particles;
        while (plist){
          plist->prtSt->size = prt->size;
          plist = plist->next;
        }
      }
    }
    else  return FunctionPublish_setRet(pstate,1,FNPUB_FFLOAT(prt->size));
    break;
  case PUBPRT_SEQ: // texid
    if (n ==3){
      if( FunctionPublish_getArgOffset(pstate,2,1,LUXI_CLASS_INT,&texid)!=1)
        return FunctionPublish_returnError(pstate,"1 float needed");
      prt->texid = texid;
      if (mode == 1){
        plist = l3d->pgroup->particles;
        while (plist){
          plist->prtSt->texid = prt->texid;
          plist = plist->next;
        }
      }
    }
    else{
      texid = prt->texid;
      return FunctionPublish_setRet(pstate,1,LUXI_CLASS_INT,(void*)texid);
    }
    break;
  case PUBPRT_ROT: // rotation
    if (n ==3){
      if( FunctionPublish_getArgOffset(pstate,2,1,LUXI_CLASS_FLOAT,&prt->rot)!=1)
        return FunctionPublish_returnError(pstate,"1 float needed");
      if (mode == 1){
        plist = l3d->pgroup->particles;
        while (plist){
          plist->prtSt->rot = prt->rot;
          plist = plist->next;
        }
      }
    }
    else  return FunctionPublish_setRet(pstate,1,FNPUB_FFLOAT(prt->rot));
  case PUBPRT_NEXT: // next
    if (mode > 1 && plist->next)
      return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLE,(void*)plist->next);
    return 0;
  case PUBPRT_PREV: // next
    if (mode > 1 && plist->prev)
      return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLE,(void*)plist->prev);
    return 0;
  case PUBPRT_SWAP:
    if (!FunctionPublish_getNArg(pstate,2,LUXI_CLASS_PARTICLE,(void*)&plist2) || !ParticleCloud_checkParticle(l3d->pgroup->pcloud,plist2))
      return FunctionPublish_returnError(pstate,"1 pgroup 2 particles of that group required");
    prt = plist->prtSt;
    plist->prtSt = plist2->prtSt;
    plist2->prtSt = prt;
    return 0;
  default:
    break;
  }



  return 0;
}

enum PubPForceCmds_e{
  PUBPFORCE_ACTIVATE,
  PUBPFORCE_DEACTIVATE,
  PUBPFORCE_DELETE,

  PUBPFORCE_TIME,
  PUBPFORCE_TURB,
  PUBPFORCE_VECTOR,
  PUBPFORCE_FLT,
  PUBPFORCE_RANGE,
  PUBPFORCE_LINKWORLD,
  PUBPFORCE_LINKBONE,
  PUBPFORCE_INTVEC,
};

static int PubParticleForce_new(PState pstate,PubFunction_t *f, int n)
{
  ParticleSys_t *psys;
  int id;
  char *name;

  if (n < 1 || 2!=FunctionPublish_getArg(pstate,2,LUXI_CLASS_PARTICLESYS,(void*)&id,LUXI_CLASS_STRING,(void*)&name))
    return FunctionPublish_returnError(pstate,"1 particlesys 1 string required");

  psys = ResData_getParticleSys(id);
  ParticleSys_newForce(psys,(ParticleForceType_t)f->upvalue,name,&id);
  if (id < 0)
    return 0;

  SUBRESID_MAKE(id,psys->resinfo.resRID);

  return FunctionPublish_returnType(pstate,LUXI_CLASS_PARTICLEFORCEID,(void*)id);
}

// LUXI_CLASS_PARTICLEFORCEID
static int PubParticleForce_funcs(PState pstate,PubFunction_t *f, int n)
{
  ParticleSys_t *psys;
  ParticleForce_t *pforce;
  int id;
  float flt;
  int start,end;
  float *pVec;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_PARTICLEFORCEID,(void*)&id))
    return FunctionPublish_returnError(pstate,"1 particleforceid required");

  psys = ResData_getParticleSys(SUBRESID_GETRES(id));
  SUBRESID_MAKEPURE(id);
  pforce = psys->forces[id];

  switch((int)f->upvalue){
  case PUBPFORCE_ACTIVATE:
    ParticleSys_addForce(psys,pforce);
    break;
  case PUBPFORCE_DEACTIVATE:
    ParticleSys_removeForce(psys,pforce);
    break;
  case PUBPFORCE_DELETE:
    ParticleSys_clearForce(psys,pforce);
    break;
  case PUBPFORCE_INTVEC:
    if (n < 2 || !pforce->intVec3 ||!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&start) || start < 0 || start >= PARTICLE_STEPS )
      return FunctionPublish_returnError(pstate,"1 particleforceid 1 int (0-127) and one intvel force required");
    pVec = pforce->intVec3[start];
    if (n<3){
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(pVec));
    }
    if (3!=FunctionPublish_getArgOffset(pstate,2,3,FNPUB_TOVECTOR3(pVec)))
      return FunctionPublish_returnError(pstate,"1 particleforceid 1 int 3 float required");
    break;
  case PUBPFORCE_TIME:
    if (n<2){
      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)pforce->starttime,LUXI_CLASS_INT,(void*)pforce->endtime);
    }
    if (2!=FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_INT,(void*)&start,LUXI_CLASS_INT,(void*)&end))
      return FunctionPublish_returnError(pstate,"1 particleforceid 2 int required");

    // with turbulences we have to remove the turbulence from old timeline
    // and add to new
    if (pforce->turb){
      pforce->turb = -pforce->turb;
      ParticleSys_precalcTurb(psys,pforce);
      pforce->starttime = start;
      pforce->endtime = end;
      pforce->turb = -pforce->turb;
      ParticleSys_precalcTurb(psys,pforce);
    }
    else{
      pforce->starttime = start;
      pforce->endtime = end;
    }
    break;
  case PUBPFORCE_TURB:
    if (n<2){
      return FunctionPublish_setRet(pstate,1,FNPUB_FFLOAT(pforce->turb));
    }
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&flt))
      return FunctionPublish_returnError(pstate,"1 particleforceid 1 float required");

    // add difference of old and new
    pforce->turb = flt - pforce->turb;
    ParticleSys_precalcTurb(psys,pforce);
    break;
  case PUBPFORCE_VECTOR:
    if (n<2){
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(pforce->vec3));
    }
    if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(pforce->vec3)))
      return FunctionPublish_returnError(pstate,"1 particleforceid 3 float required");
    break;
  case PUBPFORCE_FLT:
    if (n<2){
      return FunctionPublish_setRet(pstate,1,FNPUB_FFLOAT(pforce->flt));
    }
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&pforce->flt))
      return FunctionPublish_returnError(pstate,"1 particleforceid 1 float required");
    break;
  case PUBPFORCE_RANGE:
    if (n<2){
      return FunctionPublish_setRet(pstate,1,FNPUB_FFLOAT(pforce->range));
    }
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&pforce->range))
      return FunctionPublish_returnError(pstate,"1 particleforceid 1 float required");
    pforce->rangeSq = pforce->range*pforce->range;
    pforce->rangeDiv = 1.0f/pforce->rangeSq;
    break;
  case PUBPFORCE_LINKWORLD:
    {
      Reference refsn;
      Reference refact;
      SceneNode_t* s3d;
      ActorNode_t* act;

      Reference_reset(refsn);
      Reference_reset(refact);
      if (!(  FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SCENENODE,(void*)&refsn) ||
        FunctionPublish_getNArg(pstate,1,LUXI_CLASS_ACTORNODE,(void*)&refact))  )
        return FunctionPublish_returnError(pstate,"1 particleforceid 1 valid actor/scenenode required");

      if (pforce->forcetype != PARTICLE_FORCE_MAGNET &&
        pforce->forcetype != PARTICLE_FORCE_TARGET &&
        pforce->forcetype != PARTICLE_FORCE_INTVEL){
        return FunctionPublish_returnError(pstate,"particleforceid must be of type magnet, target or intvel");
      }

      Reference_releaseWeakCheck(pforce->linkRef);

      s3d = NULL;
      act = NULL;
      if (Reference_get(refsn,s3d)){
        pforce->linkRef = refsn;
        pforce->linkMatrix = s3d->link.matrix;
        pforce->linkType = LINK_S3D;
      }
      else if (Reference_get(refact,act)) {
        pforce->linkRef = refact;
        pforce->linkType = LINK_ACT;
      } else
        return FunctionPublish_returnError(pstate,"1 particleforceid 1 valid actor/scenenode required");


      Reference_refWeak(pforce->linkRef);
    }

    break;
  case PUBPFORCE_LINKBONE:
    {
      Reference ref;
      List3DNode_t *l3d;

      if (2!=FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_L3D_MODEL,(void*)&ref,LUXI_CLASS_BONEID,(void*)&id) || Reference_get(ref,l3d)
        || l3d->minst->modelRID != SUBRESID_GETRES(id))
        return FunctionPublish_returnError(pstate,"1 particleforceid 1 l3dmodel 1 proper boneid required");

      if (pforce->forcetype != PARTICLE_FORCE_MAGNET &&
        pforce->forcetype != PARTICLE_FORCE_TARGET ){
          return FunctionPublish_returnError(pstate,"particleforceid must be of type magnet or target");
      }
      SUBRESID_MAKEPURE(id);

      Reference_releaseWeakCheck(pforce->linkRef);

      pforce->linkType = LINK_L3DBONE;
      if (l3d->minst->boneupd){
        pforce->linkMatrix = &l3d->minst->boneupd->bonesAbs[id*16];
      }
      else
        pforce->linkMatrix = &ResData_getModel(l3d->minst->modelRID)->bonesys.refAbsMatrices[id*16];

      Reference_refWeak(ref);
      pforce->linkRef = ref;


    }
    break;
  }

  return 0;
}

void PubClass_Particle_init()
{
  FunctionPublish_initClass(LUXI_CLASS_PARTICLESYS,"particlesys",
"ParticleSystem contains of dynamic particles, which spawn at emitters and die after a given time. \
 The way they move and appear is controlled by the particlescript, which has its own detailed manual. \
 All particles of a system are rendered at the end of a l3dset and a particlesystem can be used in only one l3dst. \
 If you want to use the same particlesystem in multiple l3dsets, use one forceload per l3dset.<br>\
 Compared to ParticleClouds you cannot control particles individually."
    ,NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_PARTICLESYS,LUXI_CLASS_RESOURCE);
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_load,(void*)0,"load",
    "(Particlesys psys):(string filename,[int sortkey]) - loads a particlesystem");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_load,(void*)1,"forceload",
    "(Particlesys psys):(string filename,[int sortkey]) - forces load of a particlesystem. If the same file was already loaded before, this will create another copy of it.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_USEVOL,"usevolume",
    "([boolean]):(particlesys,[boolean]) - particles are kept in a box volume, and treated as infinite distribution");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_VOLSIZE,"volumesize",
    "([float x,y,z]):(particlesys,[float x,y,z]) - size of the volume");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_VOLCAMINFLUENCE,"volumecaminfluence",
    "([float x,y,z]):(particlesys,[float x,y,z]) - how much the camera position affects the volumecenter (default is 1,1,1).");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_VOLDIST,"volumedistance",
    "([float]):(particlesys,[float]) - distance from volume center to camera");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_VOLOFFSET,"volumeoffset",
    "([float x,y,z]):(particlesys,[float x,y,z]) - the position that is used when volumecaminfluence is 0");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_TIMESCALE,"timescale",
    "([float]):(particlesys,[float]) - you can slow down or speed up particle update and aging. 1.0 is default");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_CLEAR,"clearparticles",
    "():(particlesys) - deletes all active particles");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_GETFORCE,"getforceid",
    "([particleforceid]):(particlesys,string name) - returns particleforceid if found.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_PAUSE,"pause",
    "([boolean]):(particlesys,[boolean]) - will not perform any updates to the particles");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_NODRAW,"nodraw",
    "([boolean]):(particlesys,[boolean]) - wont draw the system");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_USECOLORMUL,"usecolormul",
    "([boolean]):(particlesys,[boolean]) - if set output colors are multipiled with colormul");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_USESIZEMUL,"usesizemul",
    "([boolean]):(particlesys,[boolean]) - if set output size is multiplied with sizemul");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_USEPROBABILITY,"useprobability",
    "([boolean]):(particlesys,[boolean]) - if set not all particles are drawn but user given percentage");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_SIZEMUL,"sizemul",
    "([float]):(particlesys,[float]) - size multiplier applied when usesizemul is true");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_PROBABILITY,"probability",
    "([float]):(particlesys,[float]) - percentage of how many particles are rendered when useprobability is true");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_PROBFADEOUT,"probabilityfadeout",
    "([float]):(particlesys,[float]) - fadeout threshold. when particle's render probability is between probability-thresh its color alpha will be interpolated accordingly. The more it closes the probability value the less its alpha will be.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_COLORMUL,"colormul",
    "([float r,g,b,a]):(particlesys,[float r,g,b,a]) - color multiplier applied when usecolormul is true. Make sure to call this after interpolatercolors are set.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_USEGLOBALAXIS,"useworldaxis",
    "([boolean]):(particlesys,[boolean]) - when set the billboards will be aligned with the user given world axis and not face camera.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_GLOBALAXIS,"worldaxis",
    "([matrix4x4]):(particlesys,[matrix4x4]) - the user given align matrix for all billboards when useworldaxis is true. It will remove the camrotfix flag as it is unneeded.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_INTERCOLOR,"intercolor",
    "([float r,g,b,a]):(particlesys,int step (0..127),[float r,g,b,a]) - returns or sets the color at the particle interpolator step.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_INTERTEX,"intertex",
    "([int]):(particlesys,int step (0..127),[int]) - returns or sets the texturenumber at the particle interpolator step.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_INTERSIZE,"intersize",
    "([float]):(particlesys,int step (0..127),[float]) - returns or sets the size multiplier at the particle interpolator step.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_INTERROT,"interrot",
    "([float]):(particlesys,int step (0..127),[float]) - returns or sets the rotation multiplier at the particle interpolator step.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_INTERVEL,"intervelocity",
    "([float]):(particlesys,int step (0..127),[float]) - returns or sets the velocity multiplier at the particle interpolator step.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_CHECK,"checktype",
    "([int]):(particlesys,[int]) - returns or sets what kind of axis aligned bounding box check should be done. Only particles within the box (defined by checkmin/max) are drawn. Checks performed are:<br> 1 = x-axis <br> 2 = y-axis <br> 3 = x,y-axis <br> 4 = z-axis <br> 5 = x,z-axis <br> 6 = y,z-axis <br> 7 = all axis <br> any other value = no check (default)");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_CHECKMIN,"checkmin",
    "([float x,y,z]):(particlesys,[float x,y,z]) - returns or sets minimum of the check boundingbox.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_CHECKMAX,"checkmax",
    "([float x,y,z]):(particlesys,[float x,y,z]) - returns or sets maximum of the check boundingbox.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_NOGPU,"nogpu",
    "([boolean]):(particlesys,[boolean]) - wont use the default gpu programs");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_FORCEINSTANCE,"forceinstance",
    "([boolean]):(particlesys,[boolean]) - will use instancing rendering even for the small normally batched billboards");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_SINGLENORMAL,"particlenormal",
    "([boolean]):(particlesys,[boolean]) - will use particle's normal. Only applied when instancing renderpath is used.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_BBOX,"bbox",
    "([float minx,miny,minz, maxx,maxy,maxz]):(model,[float minx,miny,minz, maxx,maxy,maxz]) - returns or sets bounding box");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_LM,"lightmap",
    "([texture]):(particlesys,[texture]) - lightmap texture to be used. for texture coordinates the world space positions are transformed with lightmapmatrix");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_LMPROJMAT,"lightmapmatrix",
    "([matrix4x4]):(particlesys,[matrix4x4]) - generates texture coordinates when lightmap is used.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_VISFLAG,"visflag",
    "([boolean]):(particlesys,int id,[boolean]) - sets visibility bit flag, default is true for id 1 and false for rest. It is bitwise 'and'ed with camera bitid to decide if particles should be drawn.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_LAYER,"drawlayer",
    "():(particlesys,[l3dlayerid]) - at the end of which layer it will be drawn, l3dset info is ignored.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLESYS,PubParticleSys_attributes,(void*)PUBPSYS_SUBSYSNAMED,"getsubsysdigits",
    "([string]):(particlesys,string)  - returns the special digit subsys string, to access subsys emitters in l3dpemitter, based on particle resource names (substring search is done, the preciser the resname the less ambigous). E.g \"bomb,sparks\" will search bomb subsys in this psys, then spark in bomb and so on. Returns nothing when not found.");

  FunctionPublish_initClass(LUXI_CLASS_PARTICLEFORCEID,"particleforceid",
"Particle forces within particlesys allow greater control of the dynamic beahvior. You can create general effects such as \
 wind and gravity within the prt-script itself. However for location based effects you must add particle forces with this \
 class. You can also get access to the orginal prt scripts. The number of total forces per particlesys is limited to 32 and name must be shorter than 16 characters. \
 The more you add, the slower it will be.", NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_new,(void*)PARTICLE_FORCE_GRAVITY,"creategravity",
"([particleforceid]):(particlesys,string name) - returns a new particleforceid if particlesys had an empty slot.\
 Gravity is constant acceleration.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_new,(void*)PARTICLE_FORCE_INTVEL,"createagedvelocity",
"([particleforceid]):(particlesys,string name) - returns a new particleforceid if particlesys had an empty slot.\
 Aged velocity means that velocity of the particle will be interpolated to based on its age. The age velocities must be set by the user and will be rotated by the node it is linked to");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_new,(void*)PARTICLE_FORCE_WIND,"createwind",
    "([particleforceid]):(particlesys,string name) - returns a new particleforceid if particlesys had an empty slot.\
    Wind is acceleration if slower than wind, or deceleration if faster.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_new,(void*)PARTICLE_FORCE_TARGET,"createtarget",
"([particleforceid]):(particlesys,string name) - returns a new particleforceid if particlesys had an empty slot.\
 Target means that particles will be attracted or repulsed by target.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_new,(void*)PARTICLE_FORCE_MAGNET,"createmagnet",
"([particleforceid]):(particlesys,string name) - returns a new particleforceid if particlesys had an empty slot.\
 Magnet is similar to target but a ranged effect, only within range particles are affected and the strenght falloff is quadratic with distance");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_ACTIVATE,"activate",
    "():(particleforceid) - force is activated.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_DEACTIVATE,"deactivate",
    "():(particleforceid) - force is deactivated.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_DELETE,"delete",
    "():(particleforceid) - force is deleted.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_INTVEC,"agevec3",
    "([float x,y,z]):(particleforceid,int index,[float x,y,z]) - returns or sets the vector3 at the particle interpolator step. (for aged forces)");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_VECTOR,"vector",
    "([float x,y,z]):(particleforceid,[float x,y,z]) - returns or sets the vector (must not be normalized). For wind and gravity this serves as direction and strength. For magnet and target it is an offset to the linked node.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_FLT,"airresistance",
    "([float]):(particleforceid,[float]) - returns or sets airresistance for wind.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_FLT,"strength",
    "([float]):(particleforceid,[float]) - returns or sets strength (positive attraction, negative repulsion) for target,magnet, agedvelocity.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_RANGE,"range",
    "([float]):(particleforceid,[float]) - returns or sets range for magnet.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_TIME,"time",
    "([int start,end]):(particleforceid,[int start,end]) - returns or sets start- and endtime for force. A particle must be at least starttime (ms) old and younger than endtime to be affected. Set either to 0 to disable the check for start/endtime.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_TURB,"turbulence",
    "([float]):(particleforceid,[float]) - returns or sets turbulence for the effect.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_LINKWORLD,"linkworldref",
    "():(particleforceid,[actornode/scenenode]) - sets the reference for magnet,target or agedvelocity.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLEFORCEID,PubParticleForce_funcs,(void*)PUBPFORCE_LINKBONE,"linkbone",
    "():(particleforceid,[l3dmodel, boneid]) - sets the reference for magnet,target or agedvelocity. Be aware that if the linked l3dmodel is not visible then the used boneposition will be the last when it was visible.");

  FunctionPublish_initClass(LUXI_CLASS_L3D_PEMITTER,"l3dpemitter","l3dnode of a particle emitter. Every emitter starts \
with the default values from the particle system. You can change the emittertype afterwards.<br><br>Important note about accessing subsystems: \
The l3dpemitter also hosts all subsystem emitters (all but trail, trail's common shared emitter is referenced), you can access them with a special ''subsys string'', that is made of the subsystem indices (0-based).<br>\
For example \"0\" accesses first subemitter, \"12\" the 2nd emitter of l3dpemitter and the 3rd of that one. An error is thrown when indexing is done out of bounds or on trail. You can use particlesys.getsubsysdigits to generate the string based on normal names. Be aware that if a trail subsystem is indexed, its default emitter (as used by trail effects) is modified. Also when a trail is used it wont report subsystems, as those are ignored in rendering as well.<br> Subsystem translation offsets can be scaled via renderscale.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_PEMITTER,LUXI_CLASS_L3D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_new,NULL,"new",
    "(l3dpemitter):(string name,|l3dlayerid layer|,particlesys)  - new particle emitter l3dnode (default type). The layer itself isnt important but the l3dset is extracted from it. Particlesys has a command to define in which layer it should be rendered.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_type,(void*)PARTICLE_EMITTER_POINT,"typepoint",
    "():(l3dpemitter,[subsys string])  - sets emitter type to POINT");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_type,(void*)PARTICLE_EMITTER_SPHERE,"typesphere",
    "():(l3dpemitter,[subsys string], float radius)  - sets emitter type to SPHERE");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_type,(void*)PARTICLE_EMITTER_CIRCLE,"typecircle",
    "():(l3dpemitter,[subsys string], float radius)  - sets emitter type to CIRCLE");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_type,(void*)PARTICLE_EMITTER_MODEL,"typemodel",
    "():(l3dpemitter,[subsys string], model, float scale)  - sets emitter type to MODEL");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_type,(void*)PARTICLE_EMITTER_RECTANGLE,"typerectangle",
    "():(l3dpemitter,[subsys string], float width, float height, [int axis])  - sets emitter type to RECTANGLE. Default plane normal is Z (2)");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_type,(void*)PARTICLE_EMITTER_RECTANGLE,"typerectanglelocal",
    "():(l3dpemitter,[subsys string], float width, float height)  - sets emitter type to RECTANGLELOCAL. width = along X axis, height = along Z axis");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_SPREADIN,"spreadin",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets inner spread (radians)");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_SPREADOUT,"spreadout",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets outer spread (radians)");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_VELOCITY,"velocity",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets velocity");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_VELOCITYVAR,"velocityvar",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets velocityvar");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_RATE,"rate",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets rate");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_SIZE,"size",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets emitter size (radius,width)");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_SIZE,"radius",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets emitter radius for typecircle/typeshere. equivalent to size");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_SIZE,"width",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets emitter width for typerectangles. equivalent to size");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_SIZE2,"height",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets emitter height for typerectangle(local)");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_AXIS,"axis",
    "([float]):(l3dpemitter,[subsys string],[int 0..2])  - returns or sets emitter plane normal axis for typerectangle");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_FLIP,"flipdirection",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets percent of flip direction");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_STARTAGE,"startage",
    "([int]):(l3dpemitter,[subsys string],[int])  - returns or sets starting age of a particle");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_PRTSIZE,"prtsize",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets particle size");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_OFFSET,"maxoffsetdist",
    "([float]):(l3dpemitter,[subsys string],[float])  - returns or sets max random offset from original spawnposition");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_PSYS,"getparticlesys",
    "(particlesys):(l3dpemitter,[subsys string])  - returns particlesys emitter was created from");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_GETSUBCNT,"subsyscount",
    "(int):(l3dpemitter,[subsys string])  - returns how many subsys emitters are nested");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_attributes,(void*)PUBEM_STARTVEL,"offsetvelocity",
    "([float x,y,z]):(l3dpemitter,[subsys string],[float x,y,z])  - returns or sets offset velocity");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_start,NULL,"start",
    "():(l3dpemitter,int timeoffset)  - start with timeoffset(ms) from now");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PEMITTER,PubL3DPEmitter_end,NULL,"stop",
    "():(l3dpemitter,int timeoffset,boolean norestarts)  - end with offset from now, disallow restarts");




  FunctionPublish_initClass(LUXI_CLASS_PARTICLECLOUD,
"particlecloud","A ParticleCloud contains many static particles, which are organised as groups. \
The user specifies all particle properties, the groups are used to handle automatic particle \
position updates. The fastest types to render are quad,triangle and point.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_PARTICLECLOUD,LUXI_CLASS_RESOURCE);
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_load,NULL,"create",
    "(particlecloud):(string name,int particlecount,[int sortkey]) - creates a particlecloud with the number of given particles, the name is just a identifier. However make sure to pick a unique name, else a old cloud will be returned.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_type,(void*)PARTICLE_TYPE_POINT,"typepoint",
    "():(particlecloud) - just simple points, some systems may have capability for simple textured sprites. particle sizes will be ignored. but using pointsize (default 32) and attpointsize/dist you can influence the sizes.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_type,(void*)PARTICLE_TYPE_HSPHERE,"typehemisphere",
    "():(particlecloud) - hemisphere facing towards camera.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_type,(void*)PARTICLE_TYPE_QUAD,"typequad",
    "():(particlecloud) - quad facing towards camera. origin at center");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_type,(void*)PARTICLE_TYPE_TRIANGLE,"typetriangle",
    "():(particlecloud) - a single triangle facing camera");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_type,(void*)PARTICLE_TYPE_SPHERE,"typesphere",
    "():(particlecloud) - spheres, that can be lit but are yet untextured");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_type,(void*)PARTICLE_TYPE_MODEL,"typemodel",
    "():(particlecloud) - meshes taken from the given model, very expensive. Set the model with 'model' command. Faster if instancemesh is used.");

  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_INSTMESH,"instancemesh",
    "(boolean):(particlecloud,string meshname) - sets the instance mesh. Trys to lookup the mesh in the model that is used for rendering. If the mesh is small (only triangles/quads, max 32 vertices, 96 indices) we can instance it. Returns true on success");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_RANGE,"range",
    "([float]):(particlecloud,[float]) - gets or sets range for particles, depending on the 'inside' flag, particles are either only drawn when inside range or outside.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_INSIDE,"inside",
    "([boolean]):(particlecloud,[boolean]) - gets or sets insideflag, if it is set and range is specified particles need to be within the range to be rendered.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_ROT,"particlerotate",
    "([boolean]):(particlecloud,[boolean]) - gets or sets if particles can have rotation.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_ROTVEL,"rotatedirection",
    "([boolean]):(particlecloud,[boolean]) - automaticall rotates into the direction their normals point.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_SORT,"sort",
    "([boolean]):(particlecloud,[boolean]) - gets or sets z-sort state, setting can be good for some blendtypes.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_COLORSET,"particlecolor",
    "([boolean]):(particlecloud,[boolean]) - gets or sets if particles have their own colors.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_COLOR,"color",
    "([float r,g,b,a]):(particlecloud,[float r,g,b,a]) - gets or sets common color for all particles, if particlecolor is false.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_TEX,"matsurface",
    "([material/texture]):(particlecloud,[matsurface]) - gets or sets what kind of material/texture should be used. If your textures contain combined sequences use sequencecount to set how many.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_NUMTEX,"sequencecount",
    "([int]):(particlecloud,[int]) - gets or sets number of sequence items that the material holds.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_SMOOTHPOINT,"smoothpoints",
    "([boolean]):(particlecloud,[boolean]) - gets or sets if points should be smoothed.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_PPARAMSIZE,"attpointsize",
    "([float min,max,alpha]):(particlecloud,[float min,max,alpha]) - gets or sets the automatic point parameter sizes. Combined with pointdist you can define how point particles should change size depending on distance.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_PPARAMDIST,"attpointdist",
    "([float const,lin,quad]):(particlecloud,[float const,lin,quad]) - gets or sets automatic point size attenuation along with the min and max values from pointsize.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_MODEL,"model",
    "([model]):(particlecloud,[model]) - gets or sets model, only will be used if pcloud type is also set to model.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_PSIZE,"pointsize",
    "([float]):(particlecloud,[float]) - gets or sets particle pointsize for typepoint, size is in pixels at 640x480.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_CLEAR,"clearparticles",
    "():(particlecloud) - delets all active particles");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_CAMROTFIX,"camrotfix",
    "([boolean]):(particlecloud,[boolean]) - gets or sets if particle rotation is influenced by the camera orientation. Fixes billboard rotation issues when we look 'down' on particles, but will rotate them if we are the same plane as well.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_USECOLORMUL,"usecolormul",
    "([boolean]):(particlecloud,[boolean]) - if set output colors are multipiled with colormul");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_USESIZEMUL,"usesizemul",
    "([boolean]):(particlecloud,[boolean]) - if set output size is multiplied with sizemul");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_USEORIGINOFFSET,"useoriginoffset",
    "([boolean]):(particlecloud,[boolean]) - if set the quads/triangles center will be moved by the originoffset vector. Useful to create particles that dont have their center in the middle.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_USEPROBABILITY,"useprobability",
    "([boolean]):(particlecloud,[boolean]) - if set not all particles are drawn but user given percentage");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_SIZEMUL,"sizemul",
    "([float]):(particlecloud,[float]) - size multiplier applied when usesizemul is true");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_ORIGINOFFSET,"originoffset",
    "([float x,y,z]):(particlecloud,[float x,y,z]) - returns or sets vector that is added to the generic quad/triangle center. Only applied if useoriginoffset is true.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_PROBABILITY,"probability",
    "([float]):(particlecloud,[float]) - percentage of how many particles are rendered when useprobability is true");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_PROBFADEOUT,"probabilityfadeout",
    "([float]):(particlecloud,[float]) - fadeout threshold. when particle's render probability is between probability-thresh its color alpha will be interpolated accordingly. The more it closes the probability value the less its alpha will be.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_COLORMUL,"colormul",
    "([float r,g,b,a]):(particlecloud,[float r,g,b,a]) - color multiplier applied when usecolormul is true");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_USEGLOBALAXIS,"useworldaxis",
    "([boolean]):(particlecloud,[boolean]) - when set the billboards will be aligned with the user given world axis and not face camera.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_GLOBALAXIS,"worldaxis",
    "([matrix4x4]):(particlecloud,[matrix4x4]) - the user given align matrix for all billboards when useworldaxis is true. It will remove the camrotfix flag as it is unneeded.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_CHECK,"checktype",
    "([int]):(particlecloud,[int]) - returns or sets what kind of axis aligned bounding box check should be done. Only particles within the box (defined by checkmin/max) are drawn. Checks performed are:<br> 1 = x-axis <br> 2 = y-axis <br> 3 = x,y-axis <br> 4 = z-axis <br> 5 = x,z-axis <br> 6 = y,z-axis <br> 7 = all axis <br> any other value = no check (default)");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_CHECKMIN,"checkmin",
    "([float x,y,z]):(particlecloud,[float x,y,z]) - returns or sets minimum of the check boundingbox.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_CHECKMAX,"checkmax",
    "([float x,y,z]):(particlecloud,[float x,y,z]) - returns or sets maximum of the check boundingbox.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_NOGPU,"nogpu",
    "([boolean]):(particlecloud,[boolean]) - wont use the default gpu programs");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_FORCEINSTANCE,"forceinstance",
    "([boolean]):(particlecloud,[boolean]) - will use instancing rendering even for the small normally batched billboards");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_SINGLENORMAL,"particlenormal",
    "([boolean]):(particlecloud,[boolean]) - will use particle's normal. Only applied when instancing renderpath is used.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_VISFLAG,"visflag",
    "([boolean]):(particlecloud,int id,[boolean]) - sets visibility bit flag, default is true for id 1 and false for rest. It is bitwise 'and'ed with camera bitid to decide if particles should be drawn.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_LAYER,"drawlayer",
    "():(particlecloud,[l3dlayerid]) - at the end of which layer it will be drawn, l3dset info is ignored.");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_LM,"lightmap",
    "([texture]):(particlecloud,[texture]) - lightmap texture to be used. for texture coordinates the world space positions are transformed with lightmapmatrix");
  FunctionPublish_addFunction(LUXI_CLASS_PARTICLECLOUD,PubParticleCloud_attributes,(void*)PUBPCLD_LMPROJMAT,"lightmapmatrix",
    "([matrix4x4]):(particlecloud,[matrix4x4]) - generates texture coordinates when lightmap is used.");

  FunctionPublish_initClass(LUXI_CLASS_PARTICLE,"particle","A single static particle part of a l3dpgroup",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_PARTICLE,LUXI_CLASS_L3D_LIST);

  FunctionPublish_initClass(LUXI_CLASS_L3D_PGROUP,"l3dpgroup",
"list3d node of a particle group. The group can contain manually spawned particles, \
which will not perform any additional movement unless user changes them. The particles \
are transformed with the groups matrix in local types, also the renderscale is applied. There is 4 \
different types of particlegroups, only local/world allow manual spawning. All particles of the same \
particlecloud will be rendered at once, to optimize performance you should use as little particleclouds as possible. \
You can tweak particles appearance individually or update them all in one batch. \
When they spawn they will get default values, to assign those use 0 as particle in the prt??? functions, \
to update all particles' attributes you can pass 1 as particle.<br> \
Automatic particlegroups, such as typeworldref,typemodelvertices,typemodelbones remove their particles when references become invalid."
    ,NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L3D_PGROUP,LUXI_CLASS_L3D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_new,NULL,"new",
    "(l3dgroup):(string name,|l3dlayerid layer|,particlecloud)  - new particle group l3dnode. The layer itself isnt important but the l3dset is extracted from it. Particlecloud has a command to define in which layer it should be rendered.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_type,(void*)PARTICLE_GROUP_LOCAL,"typelocal",
    "():(l3dgroup) - sets the grouptype to LOCAL, particle coordinates are in localspace");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_type,(void*)PARTICLE_GROUP_WORLD,"typeworld",
    "():(l3dgroup) - sets the grouptype to WORLD, particle coordinates are in worldspace");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_type,(void*)PARTICLE_GROUP_WORLD_REF_EACH,"typeworldref",
    "():(l3dgroup) - sets the grouptype to WORLD_REFERENCE, particle coordinates are in worldspace, and auto linked to actornode or scenenode.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_typebones,NULL,"typemodelbones",
    "():(l3dgroup,l3dmodel,int axis,stringstack) - \
sets the grouptype to LOCAL, particle coordinates are in localspace and taken from the l3dmodel's bonesystem. \
Axis is 0(X),1(Y),2(Z) and will mark the axis used for normals, stringstack allows up to 128 items. To make sure the bones are updated before the particle group is activated and that their positions match, link the group as children to the l3dmodel.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_typeverts,NULL,"typemodelvertices",
    "():(l3dgroup,l3dmodel,meshid,float percent) - \
    sets the grouptype to LOCAL, particle coordinates are in localspace and taken from the l3dmodel's vertices. \
    You can specify how many percent of the mesh's surface you want to put particles on.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_spawn,NULL,"spawn",
    "(particle):(l3dgroup,float pos x,y,z,[float normal x,y,z]) - spawns a single particle, depending on the l3dpgroup type either in object coords or local coords. It will get the default particle values, to set those just pass 0 as particle in the prt??? functions.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_remove,NULL,"remove",
    "():(l3dgroup,particle) - removes the particle, but performs no error checking, so any l3dpgroup could have spawned it belonging to the same particlecloud.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_first,NULL,"getfirst",
    "(particle):(l3dgroup) - returns first particle");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_spawnref,NULL,"spawnworldref",
    "(particle):(l3dgroup,[actornode]/[scenenode],int axis) - spawns a single particle that is autolinked to the given node. Axis is 0(X),1(Y),2(Z) and will mark the axis used for normals. Particle uses default values for the rest.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_DRAWHEAD,"drawlisthead",
    "([particle]):(l3dgroup,[particle]) - gets or sets particle that serves as head of the drawlist when usedrawlist is enabled. Be aware that there is no error checking if the particle really belongs to this group, you will get undefined behavior if you assign particles from other groups.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_DRAWTAIL,"drawlisttail",
    "([particle]):(l3dgroup,[particle]) - gets or sets particle that serves as trail of the drawlist when usedrawlist is enabled. Be aware that there is no error checking if the particle really belongs to this group, you will get undefined behavior if you assign particles from other groups.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_USEDRAWLIST,"usedrawlist",
    "([particle]):(l3dgroup,[particle]) - gets or sets if the manual drawlist should be used. You specify the list of particles that should be drawn with the drawlisthead/tail commands.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_ROT,"autorot",
    "([float]):(l3dgroup,[float]) - gets or sets automatic rotation in radians per second. Set to 0 to disable.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_CNT,"particlecount",
    "(int):(l3dgroup) - returns current particle count of the group.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_PCLD,"getparticlecloud",
    "(particlecloud):(l3dgroup) - returns particlecloud of group.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_UPDATEPTS,"updatetoworld",
    "():(l3dgroup) - transforms all particles to world state, using the current worldmatrix of the l3dpgroup. Can be useful if manual manipulation of the particles needs to be done.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_SCALE,"autoscale",
    "([float]):(l3dgroup,[float]) - gets or sets automatic scaling add per second. scale = scale + (autoscale/second)*time");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_SEQ,"autoseq",
    "([float]):(l3dgroup,[float]) - gets or sets automatic sequencing, time in ms until the next texid is used, simply loops thru seqeunces in the texture. 0 to disable");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_SIZEVAR,"sizevar",
    "([float]):(l3dgroup,[float]) - gets or sets size variance for spawned particles. outvalue = random [in-variance,in+variance]");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_ROTVAR,"rotvar",
    "([float]):(l3dgroup,[float]) - gets or sets rotationangle variance for spawned particles. outvalue = random [in-variance,in+variance]");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_USECOLOR,"usecolorvar",
    "([float]):(l3dgroup,[float]) - gets or sets if colorvariance is used for spawned particles.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_COLORVAR,"colorvar",
    "([float r,g,b,a]):(l3dgroup,[float r,g,b,a]) - gets or sets color variances. outvalue = random [in-variance,in+variance]");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubL3DPGroup_auto,(void*)PUBPGRP_REMOVEMULTI,"removemulti",
    "([int count]):(l3dgroup,string cmds, ...) - removes multiple particles that fulfill all commands.<br>\
    The command string specifies what should be compared, and after the string each compare function has a different set of arguments: \n\n\
    * 't': seqeunce texid (comparemode,int) \n\n\
    * 'd': distance to point (comparemode,float x,y,z,distance) \n\n\
    * 's': size (comparemode,float) \n\n\
    e.g. grp:(\"td\",comparemode.greater(),1,comparemode.less(),1,2,3,100)<br>\
    kicks all particles with texseqid greater than one and with distance to point(1,2,3) less than 100.");

  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubParticle_attributes,(void*)PUBPRT_POS,"prtPos",
    "([float x,y,z]):(l3dpgroup,particle,[float x,y,z]) - gets or sets particle position, does not allow 'set all'");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubParticle_attributes,(void*)PUBPRT_NORMAL,"prtNormal",
    "([float x,y,z]):(l3dpgroup,particle,[float x,y,z]) - gets or sets particle normal, does not allow 'set all'.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubParticle_attributes,(void*)PUBPRT_WPOS,"prtPosworld",
    "([float x,y,z]):(l3dpgroup,particle) - gets or sets particle world position. Make sure to call l3dpgroup.updatetoworld, else positions might not be uptodate.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubParticle_attributes,(void*)PUBPRT_WNORMAL,"prtNormalworld",
    "([float x,y,z]):(l3dpgroup,particle) - gets or sets particle normal. Make sure to call l3dpgroup.updatetoworld, else positions might not be uptodate.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubParticle_attributes,(void*)PUBPRT_COLOR,"prtColor",
    "([float r,g,b,a]):(l3dpgroup,particle,[float r,g,b,a]) - gets or sets particle color. particlecloud must have particlecolor flag set to make it use the value.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubParticle_attributes,(void*)PUBPRT_SIZE,"prtSize",
    "([float]):(l3dpgroup,particle,[float]) - gets or sets particle size.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubParticle_attributes,(void*)PUBPRT_SEQ,"prtSeq",
    "([int]):(l3dpgroup,particle,[int]) - gets or sets particle sequence number, the subtexture to be used if we have a sequencecount greater 1.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubParticle_attributes,(void*)PUBPRT_ROT,"prtRotangle",
    "([float]):(l3dpgroup,particle,[float]) - gets or sets particle rotation in radians. particlecloud must have rotate flag set to make it use the value.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubParticle_attributes,(void*)PUBPRT_NEXT,"prtNext",
    "(particle):(l3dpgroup,particle) - returns next particle or null if last. useful for manual iteration.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubParticle_attributes,(void*)PUBPRT_NEXT,"prtPrev",
    "(particle):(l3dpgroup,particle) - returns prev particle or null if first. useful for manual iteration.");
  FunctionPublish_addFunction(LUXI_CLASS_L3D_PGROUP,PubParticle_attributes,(void*)PUBPRT_SWAP,"prtSwap",
    "():(l3dpgroup,particle,particle) - swaps content of the two particles.");
}

