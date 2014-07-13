// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "animation.h"
#include "../common/timer.h"
#include "../common/common.h"
#include "../resource/resmanager.h"


//////////////////////////////////////////////////////////////////////////
// Anim Loader
static struct AnimData_s{
  float scale[3];
  int   usescale;
}l_AnimData = {{1.0f,1.0f,1.0f},LUX_FALSE};

void  AnimLoader_setPrescale(float scale[3])
{
  lxVector3Copy(l_AnimData.scale,scale);
  l_AnimData.usescale = scale[0] != 1.0f || scale[1] != 1.0f || scale[2] != 1.0f;
}
void  AnimLoader_getPrescale(float scale[3])
{
  lxVector3Copy(scale,l_AnimData.scale);
}

void  AnimLoader_applyScale(Anim_t *anim)
{
  Track_t* track;
  PRS_t*  key;
  int t,k;

  if (!l_AnimData.usescale) return;

  track = anim->tracks;
  for (t=0; t < anim->numTracks; t++,track++){
    key = track->keys;
    for (k = 0; k < track->numKeys; k++,key++){
      lxVector3Mul(key->pos,key->pos,l_AnimData.scale);
    }
  }

}

//////////////////////////////////////////////////////////////////////////
// Anim

// UPDATES A MODEL WITH GIVEN ANIM AND TIME
//------------------------------------------

void AnimBlend_updateBoneSys( AnimBlend_t * LUX_RESTRICT ablend1, AnimBlend_t * LUX_RESTRICT ablend2, const float factor, BoneSys_t * LUX_RESTRICT bonesys )
{
  int i;
  //ListNode *browse;
  BoneAssign_t *assign=NULL;
  BoneAssign_t *assign2=NULL;
  Bone_t  **bonep;
  Bone_t  *bone;
  Track_t *track;

  booln found = LUX_FALSE;
  booln rotonly;
  booln rotonly2;

  if (ablend2->anim && factor > 0.99){
    ablend1->anim = ablend2->anim;
    ablend2->anim = NULL;
    ablend1->inTime = ablend2->inTime;
  }

  if (ablend1->anim == NULL ||
    bonesys == NULL)
    return;

  if (ablend2->anim && factor < 0.01)
    ablend2->anim = NULL;

  if (!(assign = AnimAssign_get(ablend1->anim,bonesys)))
    return;

  rotonly =  isFlagSet(ablend1->animflag,ANIM_UPD_ABS_ROTONLY);

  // if someone goes too far at this point of time, bring him down to earth
  ablend1->inTime %= ablend1->anim->length;

  // do the same for second anim
  if (ablend2->anim){
    if (!(assign2 = AnimAssign_get(ablend2->anim,bonesys)))
      return;
    rotonly2 =  isFlagSet(ablend2->animflag,ANIM_UPD_ABS_ROTONLY);
    ablend2->inTime %= ablend2->anim->length;
  }

  // we are getting closer to her now let each track take care of itself
  // cause the track actually knows whats going on at this time
  if (ablend2->anim){
    track = ablend1->anim->tracks;
    bonep = assign->bones;
    for (i = 0; i < ablend1->anim->numTracks; i++,track++,bonep++){
      // just compute what exists in the model
      if (assign->bones[i]){
        Track_setKey(track,(float)ablend1->inTime,ablend1->animflag, &(*bonep)->keyPRS);
      }
    }

    track = ablend2->anim->tracks;
    bonep = assign2->bones;
    for (i = 0; i < ablend2->anim->numTracks; i++,track++,bonep){
      // just compute what exists in the model
      if (assign2->bones[i]){
        Track_setKey(track,(float)ablend2->inTime,ablend2->animflag, &(*bonep)->blendPRS);
      }
    }
    bone = bonesys->bones;
    // run thru all bones, the ones that are affected have either in key or blendkey time=1
    for (i = 0; i < bonesys->numBones; i++,bone++){
      Bone_setFromBlendKey(bone,factor,rotonly,rotonly2);
    }
  }else{
    track = ablend1->anim->tracks;
    bonep = assign->bones;
    for (i = 0; i < ablend1->anim->numTracks; i++,track++,bonep++){
      // just compute what exists in the model
      if (assign->bones[i]){
        Track_setKey(track,(float)ablend1->inTime,ablend1->animflag, &(*bonep)->keyPRS);
        Bone_setFromKey(*bonep,rotonly);
      }
    }
  }

}


// TRACK UPDATES KEY AT TIME
//------------------------------
//    parts taken from SceneManager.cpp RCMANIA
//
void Track_setKey(const Track_t *LUX_RESTRICT track,const float inTime, const enum32 animflag, PRS_t * LUX_RESTRICT lerpkey)
{
  // the things we want to change so bad in our lives
  float *rotLerp;
  float *posLerp;
  float *scaleLerp;

  // the two keys we interpolate between
  PRS_t   *before;
  PRS_t   *after;
  // the time we need for this
  float timefrac;
  int numkeys = track->numKeys;

  LUX_SIMDASSERT(LUX_IS_ALIGNED(lerpkey,16));

  rotLerp = lerpkey->rot;
  posLerp = lerpkey->pos;
  scaleLerp = lerpkey->scale;

  lerpkey->time = 1;
  // okay easy we got only one key
  if(numkeys == 1){
    before = &track->keys[0];
    lxVector3Copy(posLerp,before->pos);
    lxQuatCopy(rotLerp,before->rot);
    lxVector3Copy(scaleLerp,before->scale);
  }
  else{
    // find keys to be interpolated
    int key = 0, key1, key2;
    int minbnd = 0;
    int maxbnd = numkeys-1;

    while(maxbnd-minbnd > 1){
      key = (maxbnd+minbnd)/2;
      if(track->keys[key].time > inTime)
        maxbnd = key;
      else
        minbnd = key;
    }
    key = (track->keys[minbnd].time > inTime) ? minbnd : maxbnd;

    //while (track->keys[key].time < inTime && key < (int)track->numKeys)
    //  key++;

    key1 = key- 1;
    key2 = key;

    if (key1 < 0)
    {
      key1++;
      key2++;
    }

    before = &track->keys[key1];
    after  = &track->keys[key2];

    // if the time is below our first key then just use before's data
    if (inTime < before->time){
      lxVector3Copy(posLerp,before->pos);
      lxVector3Copy(scaleLerp,before->scale);
      lxQuatCopy(rotLerp,before->rot);
    }// same story with last key
    else if (inTime > after->time){
      lxVector3Copy(posLerp,after->pos);
      lxVector3Copy(scaleLerp,after->scale);
      lxQuatCopy(rotLerp,after->rot);
    }
    else{
      // find interpolation timefrac value
      timefrac = (inTime- before->time) / (after->time - before->time);
      timefrac = LUX_CLAMP(timefrac,0.0f,1.0f);

      // skip if we want rotation only
      if (!isFlagSet(animflag,ANIM_UPD_ABS_ROTONLY))
      {
        // do linear interpolation for position (is default on file load)
        if (isFlagSet(animflag,ANIM_TYPE_LINEAR)){
          lxVector3Lerp(posLerp,timefrac,before->pos,after->pos);
        }
        else
        { // do Catmull-Rom Spline Interpolation for position
          lxVector3SIMD firstvec, lastvec;
          float *last,*first;
          float test;

          // do a test if interpolation for position is needed
          // we skip if the change is too small, else we get subtle
          // motion of things that actually dont move

          lxVector3Sub(firstvec,before->pos,after->pos);
          test = lxVector3Dot(firstvec,firstvec);

          if (test < 0.00001){
            lxVector3Copy(posLerp,before->pos);
          }
          else if (isFlagSet(animflag,ANIM_TYPE_SPLINE))
          {
            if (key1 == 0){
              lxVector3Copy( firstvec,before->pos);
              lxVector3Scale(firstvec,firstvec,2);
              lxVector3Sub(firstvec,firstvec,after->pos);
              first = firstvec;
            }
            else
              first = track->keys[key1-1].pos;

            if (key2 == numkeys-1){
              lxVector3Copy( lastvec,after->pos);
              lxVector3Scale(lastvec,lastvec,2);
              lxVector3Sub(lastvec,lastvec,before->pos);
              last = lastvec;
            }
            else
              last = track->keys[key2+1].pos;

            LUX_SIMDASSERT(LUX_IS_ALIGNED(first,16));
            LUX_SIMDASSERT(LUX_IS_ALIGNED(last,16));

            lxVector4CatmullRomSIMD(posLerp,timefrac,first,before->pos,after->pos,last);
          }
          else{
            // SPLINE LOOP
            first = track->keys[(key1-1+numkeys)%numkeys].pos;
            last  = track->keys[(key2+1)%numkeys].pos;
            lxVector4CatmullRomSIMD(posLerp,timefrac,first,before->pos,after->pos,last);
          }

        }
        lxVector4LerpSIMD(scaleLerp,timefrac,before->scale,after->scale);
      }
      lxQuatSlerp(rotLerp,timefrac,before->rot,after->rot);
    }
  }
}


BoneAssign_t*  AnimAssign_get(Anim_t *anim, BoneSys_t *bonesys)
{
  int found = LUX_FALSE;
  BoneAssign_t  *assign;


  // start with a little foreplay
  // look if there exists an assignment for the given model already
  lxListNode_forEach(anim->assignsListHead,assign)
    if (assign->bonesys == bonesys){
      found = LUX_TRUE;
      break;
    }
  lxListNode_forEachEnd(anim->assignsListHead,assign);

  // we had luck, no need to create an assignment
  if (found)
    return assign;
  else
  {
    ResourceChunk_t *reschunk;
    Track_t *track;
    int i;
    int index;

    // we must make sure that anim stores its stuff always in the same chunk it is in
    reschunk = ResData_setChunkFromPtr(&anim->resinfo);
    assign = (BoneAssign_t*)  reszalloc(sizeof(BoneAssign_t));
    assign->bonesys= bonesys;
    // create proper lookups
    assign->bones = (Bone_t**)  reszalloc(sizeof(Bone_t*)*anim->numTracks);
    ResourceChunk_activate(reschunk);

    index = -1;
    found = LUX_FALSE;
    for (i = 0; i < anim->numTracks; i++){
      track = &anim->tracks[i];

      index = BoneSys_searchBone(bonesys,track->name);
      if (index >= 0){
        assign->bones[i] = &bonesys->bones[index];
        found = LUX_TRUE;
      }
      else{
        assign->bones[i] = NULL;
      }
    }
  }

  if (found){
    lxListNode_init(assign);
    lxListNode_addLast(anim->assignsListHead,assign);
    return assign;
  }
  // WARNING assign lost then
  else{
    return NULL;
  }

}

void  Anim_removeAssign(Anim_t *anim, BoneSys_t *bonesys)
{
  BoneAssign_t *assign;
  int remove = LUX_FALSE;

  lxListNode_forEach(anim->assignsListHead,assign)
    if (assign->bonesys == bonesys){
      remove = LUX_TRUE;
      break;
    }
  lxListNode_forEachEnd(anim->assignsListHead,assign);

  if (remove){
    lxListNode_remove(anim->assignsListHead,assign);
  }
}

int Anim_searchTrack(Anim_t * anim, const char *name)
{
  int i;
  Track_t *track;
  if (!anim || !name)
    return -1;

  track = anim->tracks;
  for (i = 0 ; i < (int)anim->numTracks; i++,track++){
    if (strcmp(track->name,name)==0)
      return i;
  }
  return -1;
}

