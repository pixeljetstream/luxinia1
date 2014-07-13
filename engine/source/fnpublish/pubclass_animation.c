// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../main/main.h"
// Published here:
// LUXI_CLASS_ANIMATION
// LUXI_CLASS_TRACKID

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_ANIMATIOM
static int PubAnimation_load (PState state, PubFunction_t *fn, int n)
{
  char *path;
  int index;
  booln spline = LUX_TRUE;
  booln rotonly = LUX_FALSE;
  int animflag = 0;


  if (n<1 || FunctionPublish_getArg(state,3,LUXI_CLASS_STRING,&path,LUXI_CLASS_BOOLEAN,&spline,LUXI_CLASS_BOOLEAN,&rotonly)<1)
    return FunctionPublish_returnError(state,"1 string [2 booleans] required");


  animflag |= spline  ? ANIM_TYPE_SPLINE : ANIM_TYPE_LINEAR;
  animflag |= rotonly ? ANIM_UPD_ABS_ROTONLY : ANIM_UPD_ABS_ALL;

  Main_startTask(LUX_TASK_RESOURCE);
  index = ResData_addAnimation(path, animflag);
  Main_endTask();
  if (index==-1)
    return 0;

  return FunctionPublish_returnType(state,LUXI_CLASS_ANIMATION,(void*)index);
}

enum PubAnimCmd_e
{
  PANIM_PRESCALE,
  PANIM_CREATE,

  PANIM_NEEDANIM_FUNCS,

  PANIM_LEN,
  PANIM_TRACKS,
  PANIM_SPLINE,
  PANIM_ROTONLY,
  PANIM_CREATEFINISH,
};

static int PubAnimation_attributes(PState pstate, PubFunction_t *fn, int n)
{
  int index;
  booln state = LUX_FALSE;
  Anim_t *anim = NULL;

  if ((int)fn->upvalue > PANIM_NEEDANIM_FUNCS &&
    (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_ANIMATION,(void*)&index) || !(anim=ResData_getAnim(index))))
    return FunctionPublish_returnError(pstate,"1 animation required");

  switch ((int)fn->upvalue){
  case PANIM_PRESCALE:
    {
      lxVector3 prescale;
      if (n==0){
        AnimLoader_getPrescale(prescale);
        return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(prescale));
      }
      else if (n < 3 || 3>FunctionPublish_getArg(pstate,3,FNPUB_TOVECTOR3(prescale)))
        return FunctionPublish_returnError(pstate,"3 floats required");

      AnimLoader_setPrescale(prescale);
      return 0;
    }
  case PANIM_LEN:
    return FunctionPublish_returnInt(pstate,anim->length);
  case PANIM_TRACKS:
    return FunctionPublish_returnInt(pstate,anim->numTracks);
  case PANIM_ROTONLY:
    if (n==1)
      return FunctionPublish_returnBool(pstate,ResData_getAnim(index)->animflag & ANIM_UPD_ABS_ROTONLY);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 animation 1 boolean required");
    if (state){
      anim->animflag |= ANIM_UPD_ABS_ROTONLY;
      anim->animflag &= ~ANIM_UPD_ABS_ALL;
    }
    else{
      anim->animflag &= ~ANIM_UPD_ABS_ROTONLY;
      anim->animflag |= ANIM_UPD_ABS_ALL;
    }
    return 0;
  case PANIM_SPLINE:
    if (n==1)
      return FunctionPublish_returnBool(pstate,anim->animflag & ANIM_TYPE_SPLINE);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 animation 1 boolean required");
    if (state){
      anim->animflag |= ANIM_TYPE_SPLINE;
      anim->animflag &= ~ANIM_TYPE_LINEAR;
    }
    else{
      anim->animflag &= ~ANIM_TYPE_SPLINE;
      anim->animflag |= ANIM_TYPE_LINEAR;
    }
    return 0;
  default:
    return 0;
  }
  return 0;
}


//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_TrackRID
static int PubAnimation_gettrack(PState state, PubFunction_t *fn, int n)
{
  char *name = NULL;
  int index = 0;
  int model;

  if (n!=2 || !FunctionPublish_getNArg(state,0,LUXI_CLASS_ANIMATION,(void*)&model) ||
    ( !FunctionPublish_getNArg(state,1,LUXI_CLASS_STRING,&name) &&
      !FunctionPublish_getNArg(state,1,LUXI_CLASS_INT,(void*)&index)))
    return FunctionPublish_returnError(state,"1 animation 1 string/int required");

  if (name){
    index = Anim_searchTrack(ResData_getAnim(model),name);
    if (index==-1)
      return 0;
  }
  else if (index < 0 || index >= ResData_getAnim(model)->numTracks){
    return FunctionPublish_returnError(state,"index out of bounds");
  }


  SUBRESID_MAKE(index,model);

  return FunctionPublish_returnType(state,LUXI_CLASS_TRACKID,(void*)index);
}

enum PubTrackCmds_e
{
  PTRACK_UPDATEKEY,

  PTRACK_NAME,
  PTRACK_KEYCOUNT,
  PTRACK_KEYPOS,
  PTRACK_KEYSCALE,
  PTRACK_KEYROT,
  PTRACK_KEYTIME,
  PTRACK_NEW,
};

static int PubTrack_func(PState state, PubFunction_t *fn, int n)
{
  static LUX_ALIGNSIMD_V(PRS_t  statickey);

  int track;
  int time;
  Reference ref;
  Anim_t *anim;
  float *matrix;

  bool8 spline;

  if (n<1 || !FunctionPublish_getNArg(state,0,LUXI_CLASS_TRACKID,(void*)&track))
    return FunctionPublish_returnError(state,"1 trackid required");


  switch((int)fn->upvalue)
  {
  case PTRACK_UPDATEKEY:
    if (n<4 || FunctionPublish_getArgOffset(state,1,3,LUXI_CLASS_MATRIX44,(void*)&ref,LUXI_CLASS_INT,(void*)&time,LUXI_CLASS_BOOLEAN,(void*)&spline)!=3 ||
      !Reference_get(ref,matrix))
      return FunctionPublish_returnError(state,"1 trackid 1 matrix4x4 1 int 1 boolean required");

    anim = ResData_getAnim(SUBRESID_GETRES(track));
    track = SUBRESID_MAKEPURE(track);

    Track_setKey(&anim->tracks[track],(float)(time%anim->length),spline ? ANIM_TYPE_SPLINE : ANIM_TYPE_LINEAR, &statickey);
    PRS_setMatrix(&statickey,matrix);
    return 0;
  default:
    break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_PRSKey


void PubClass_Animation_init()
{
  FunctionPublish_initClass(LUXI_CLASS_ANIMATION,"animation",
    "The animation contains tracks which store position, rotation and scale changes over time.\
    Such a tripple is called a prskey. You can use animations on bonesystems found in l3dmodels,\
    or evaluate prskey states for given times manually. The current interpolation and updatetype is copied when assigned to a bonesystem. Therefore same animation can be used with different interpolation or update definitions.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_ANIMATION,LUXI_CLASS_RESOURCE);
  FunctionPublish_addFunction(LUXI_CLASS_ANIMATION,PubAnimation_load,NULL,"load",
    "([animation]):(string filename,[boolean splineinterpolation],[boolean rotationonly]) - loads a animation with given properties. default: spline=true, rotationonly=false");
  FunctionPublish_addFunction(LUXI_CLASS_ANIMATION,PubAnimation_attributes,(void*)PANIM_LEN,"length",
    "(int):(animation) - returns animation length in ms");
  FunctionPublish_addFunction(LUXI_CLASS_ANIMATION,PubAnimation_attributes,(void*)PANIM_TRACKS,"trackcount",
    "(int):(animation) - returns number of tracks within the animation");
  FunctionPublish_addFunction(LUXI_CLASS_ANIMATION,PubAnimation_attributes,(void*)PANIM_SPLINE,"splineinterpolation",
    "(boolean):(animation,[boolean]) - sets or gets if animation uses spline interpolation for position between keys.");
  FunctionPublish_addFunction(LUXI_CLASS_ANIMATION,PubAnimation_attributes,(void*)PANIM_ROTONLY,"rotonlyupdate",
    "(boolean):(animation,[boolean]) - sets or gets if animation calculates only the rotation keys.");
  FunctionPublish_addFunction(LUXI_CLASS_ANIMATION,PubAnimation_attributes,(void*)PANIM_PRESCALE,"loaderprescale",
    "([float x,y,z]):([float x,y,z]) - prescales bone positions with a given vector. The factor remains active for all follwing 'animation.load' calls. Prescale will be reapplied if the anim is reloaded from disk again.");
/*
  FunctionPublish_addFunction(LUXI_CLASS_ANIMATION,PubAnimation_attributes,(void*)PANIM_CREATE,"create",
    "(animation):(string name, int numtracks) - returns empty animation to be filled manually with trackid commands. Call createfinish once done.");
  FunctionPublish_addFunction(LUXI_CLASS_ANIMATION,PubAnimation_attributes,(void*)PANIM_CREATEFINISH,"createfinish",
    "():(animation) - finalizes manually created animations");
*/
  FunctionPublish_addFunction(LUXI_CLASS_ANIMATION,PubAnimation_gettrack,NULL,"gettrack",
    "([trackid]):(animation,string trackname/ int number) - returns trackid of track within animation");


  FunctionPublish_initClass(LUXI_CLASS_TRACKID,"trackid","trackid - a track within animation contains prskey changes",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_TRACKID,PubTrack_func,(void*)PTRACK_UPDATEKEY,"updatekey",
    "():(trackid,[matrix4x4],int time,boolean spline) - computes key for given time");
/*
  FunctionPublish_addFunction(LUXI_CLASS_TRACKID,PubTrack_func,(void*)PTRACK_NAME,"name",
    "([string]):(trackid) - returns name of tracl");
  FunctionPublish_addFunction(LUXI_CLASS_TRACKID,PubTrack_func,(void*)PTRACK_KEYCOUNT,"keycount",
    "(int):(trackid) - returns number of keys in track");
  FunctionPublish_addFunction(LUXI_CLASS_TRACKID,PubTrack_func,(void*)PTRACK_KEYPOS,"keypos",
    "([float x,y,z]):(trackid,int key,[float x,y,z]) - gets or sets key's position");
  FunctionPublish_addFunction(LUXI_CLASS_TRACKID,PubTrack_func,(void*)PTRACK_KEYSCALE,"keyscale",
    "([float x,y,z]):(trackid,int key,[float x,y,z]) - gets or sets key's scale");
  FunctionPublish_addFunction(LUXI_CLASS_TRACKID,PubTrack_func,(void*)PTRACK_KEYROT,"keyrotquat",
    "([float x,y,z,w]):(trackid,int key,[float x,y,z,w]) - gets or sets key's rotation quaternion");
  FunctionPublish_addFunction(LUXI_CLASS_TRACKID,PubTrack_func,(void*)PTRACK_KEYTIME,"keytime",
    "([float]):(trackid,int key,[float]) - gets or sets key's time. Warning time of keys must be ascendingly sorted");
  FunctionPublish_addFunction(LUXI_CLASS_MESHID,PubTrack_func,(void*)PTRACK_NEW,"init",
    "():(trackid,string name, int numkeys) - only works on empty tracks that are hosted in animation.create generated model.");
*/

}
