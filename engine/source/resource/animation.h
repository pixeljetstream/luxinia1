// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __ANIMATION_H__
#define __ANIMATION_H__

/*
ANIMATION
---------

Animation information consists of tracks for each animated object.
A track holds multiple keys which represent Position Scale Rotation (PRS) state
of the object at a given time.
All timecodes are in milliseconds.

Animation can control a bonesystem, between keys there is either linear
or spline interpolation for the position (CatmullRom Spline)

All functions are accessed directly by the bonesystem itself

An Animation can affect all PRS states or Rotation only of the client bonesystem.
Every Animation will automatically create a lookup table (assign) to associate tracks
with bones. It is crucial that tracke names are identical to bonenames

Anim  (nested, ResMem)
Track (nested, ResMem)

Author: Christoph Kubisch

*/

#include "../resource/bone.h"

#define ANIM_MAX_TRACKS   128

#define ANIM_USERSTART    "USER_ANIM("
#define ANIM_USERSTART_LEN  10

enum AnimFlags_e{
  ANIM_TYPE_LINEAR    =1<<0,
  ANIM_TYPE_SPLINE    =1<<1,
  ANIM_TYPE_SPLINELOOP  =1<<2,

  ANIM_UPD_ABS_ALL    =1<<8,
  ANIM_UPD_ABS_ROTONLY  =1<<9,
};

typedef struct Track_s{
  char  *name;      // name of the object it affects

  int     numKeys;
  PRS_t   *keys;
} Track_t;

typedef struct Anim_s{
  ResourceInfo_t resinfo;

  uint  length;

  int   numTracks;    // number of objects that are changed
  Track_t *tracks;

  enum32    animflag;

  float   prescale[3];

  BoneAssign_t  *assignsListHead; // fasten processing of tracks updating the model
} Anim_t;

typedef struct AnimBlend_s
{
  Anim_t  *anim;
  uint  inTime;
  enum32  animflag;
  //int   *loopStart;
  //int   *loopEnd;
}AnimBlend_t;


//////////////////////////////////////////////////////////////////////////
// AnimLoader

void  AnimLoader_setPrescale(float prescale[3]);
void  AnimLoader_getPrescale(float prescale[3]);

void  AnimLoader_applyScale(Anim_t *anim);

///////////////////////////////////////////////////////////////////////////////
// Anim

// main funcs
void AnimBlend_updateBoneSys(AnimBlend_t * LUX_RESTRICT ablend1, AnimBlend_t * LUX_RESTRICT ablend2,const float factor, BoneSys_t * LUX_RESTRICT bonesys);

// helpers
BoneAssign_t* AnimAssign_get(Anim_t *anim, BoneSys_t *bonesys);
int   Anim_searchTrack(Anim_t * anim, const char *name);

void  Anim_removeAssign(Anim_t *anim, BoneSys_t * bonesys);


///////////////////////////////////////////////////////////////////////////////
// Track
// also set the "time" value of the key to TRUE
// output PRS will have invalid time and usedflags
void Track_setKey(const Track_t *LUX_RESTRICT track,const float inTime, const enum32 animflag, PRS_t * LUX_RESTRICT lerpkey);

#endif

