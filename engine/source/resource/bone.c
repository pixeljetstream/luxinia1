// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "bone.h"
#include "../common/3dmath.h"
#include "animation.h"
#include "resmanager.h"

///////////////////////////////////////////////////////////////////////////////
// PRS

void PRS_setMatrix(PRS_t *prskey,lxMatrix44 matrix){
  static lxMatrix44SIMD rotposM;

  // build final matrix
  lxMatrix44IdentitySIMD(matrix);
  // scale
  lxMatrix44SetScale(matrix,prskey->scale);
  // rot
  lxQuatToMatrixIdentity(prskey->rot,rotposM);
  lxMatrix44Multiply1SIMD(matrix,rotposM);
  //pos
  lxMatrix44SetTranslation(matrix,prskey->pos);
}


// BONE SYS UPDATE
// ---------------

void  BoneSysUpdate_run(BoneSysUpdate_t *boneupd, lxMatrix44 worldmatrix){
  BoneControl_t *bcontrol;
  BoneSys_t *bonesys = boneupd->bonesys;
  BoneSequence_t *boneseq = boneupd->seq;
  int i;
  int change = LUX_FALSE;
  int abs   = LUX_FALSE;
  int lastanim = LUX_FALSE;
  int active  = LUX_FALSE;
  int run   = LUX_FALSE;

  float   blendfactor;
  uint    timediff;
  float   timediffreal;

  float *outmat;
  Bone_t *bone;
  AnimBlend_t ablend1;
  AnimBlend_t ablend2;

  boneupd->worldMatrix = worldmatrix;

  bonesys->absMatrices = boneupd->bonesAbs;
  bonesys->relMatricesT = boneupd->bonesRelT;

  // update bones depending on each sequence
  for (i = 0; i < BONE_MAX_SEQ; i++, boneseq++)
  {

    switch(boneseq->state) {
    case BONE_SEQ_SINGLE:
      if (boneseq->flag & BONE_TIME_SET){
        active = LUX_TRUE;
        run = LUX_TRUE;
        boneseq->flag &= ~BONE_TIME_SET;
      }
      break;
    case BONE_SEQ_RUN:
      active = LUX_TRUE;
      // compute time that has passed and raise our runner
      timediffreal = ((float)(g_LuxTimer.time - boneseq->time))*boneseq->animspeed;
      boneseq->runtimefracc += timediffreal;
      timediff = (uint)boneseq->runtimefracc;
      boneseq->runtimefracc -= timediff;
      if (boneseq->flag & BONE_TIME_SET){
        timediff = 0;
        boneseq->flag &= ~BONE_TIME_SET;
      }

      boneseq->runtime += timediff;
      // if sequence is over its total duration and there is no loop set we halt
      if (!isFlagSet(boneseq->flag,BONE_LOOP) && (boneseq->runtime >= boneseq->duration || boneseq->runtime < 0)){
        boneseq->state = BONE_SEQ_STOP;
        continue;
      }

      run = LUX_TRUE;
      break;
    case BONE_SEQ_STOP:
      active = LUX_TRUE;
      boneseq->time = g_LuxTimer.time;
      break;
    default:
      continue;
    }

    if (run){
      boneseq->runtime %= boneseq->duration;
      boneseq->runtime = (boneseq->runtime < 0) ? boneseq->duration + boneseq->runtime : boneseq->runtime;
      // check if we need to blend
      if (boneseq->flag & BONE_MANUAL_BLEND)
        blendfactor = boneseq->blendfactor;
      else if (boneseq->runtime < boneseq->anim1Start){
        blendfactor = 0;
        // if anim1 isnt starting yet and 0 already done do nothing
        if (boneseq->runtime > boneseq->animLengths[0])
          continue;
      }// we are fully in anim1
      else if (boneseq->runtime > boneseq->animLengths[0])
        blendfactor = 1;
      else {
        // we need to LERP
        blendfactor = (float)((boneseq->runtime - boneseq->anim1Start)/(boneseq->animLengths[0] - boneseq->anim1Start));
      }


      // compute times for anims
      ablend1.inTime = boneseq->runtime + boneseq->animStartTimes[0];
      ablend1.anim = ResData_getAnim(boneseq->anims[0]);
      ablend1.animflag = boneseq->animflags[0];
      //ablend1.loopStart= boneseq->loopKeyStart[0];
      //ablend1.loopEnd= boneseq->loopKeyEnd[0];

      ablend2.inTime = boneseq->runtime + boneseq->animStartTimes[1] - boneseq->anim1Start ;
      ablend2.anim = ResData_getAnim(boneseq->anims[1]);
      ablend2.animflag = boneseq->animflags[1];
      //ablend2.loopStart= boneseq->loopKeyStart[1];
      //ablend2.loopEnd= boneseq->loopKeyEnd[1];
      AnimBlend_updateBoneSys(&ablend1,&ablend2,blendfactor,bonesys);
      // update our time and state
      boneseq->time = g_LuxTimer.time;
      abs = LUX_TRUE;
      change = LUX_TRUE;
    }
  }
  // if there was a change
  // cache the lastanim update bones;
  if (change){
    outmat = boneupd->bonesUpd;
    bone = bonesys->bones;
    for (i = 0; i < boneupd->numBones; i++,outmat+=16,bone++){
      lxMatrix44CopySIMD(outmat,bone->updateMatrix);
    }
  }// else check if the anims are active but havent changed then we could use their last update
  else if (active)
    lastanim = LUX_TRUE;

  // do controller stuff
  lxListNode_forEach(boneupd->controlListHead,bcontrol)
    change = LUX_TRUE;
    if (lastanim){
      outmat = boneupd->bonesUpd;
      bone = bonesys->bones;
      // stream back the old states
      for (i = 0; i < boneupd->numBones; i++,outmat+=16,bone++){
        lxMatrix44CopySIMD(bone->updateMatrix,outmat);
      }
      lastanim = LUX_FALSE;
      abs = LUX_TRUE;
    }
    switch ( bcontrol->type){
      case BONE_CONTROL_RELATIVE:
        if (abs)
          lxMatrix44Multiply1SIMD(bcontrol->bone->updateMatrix,bcontrol->matrix);
        else
          lxMatrix44CopySIMD(bcontrol->bone->updateMatrix,bcontrol->matrix);
        break;
      case BONE_CONTROL_ABS:
        if (bcontrol->targetMatrixIF)
          Matrix44Interface_getElementsCopy(bcontrol->targetMatrixIF,bcontrol->bone->updateMatrix);
        else
          lxMatrix44CopySIMD(bcontrol->bone->updateMatrix,bcontrol->matrix);
        bcontrol->bone->bcontrol = bcontrol;
        break;
      case BONE_CONTROL_LOOKAT:
      case BONE_CONTROL_IK:
        if (worldmatrix){
          bcontrol->bone->bcontrol = bcontrol;
        }
        break;
    }
  lxListNode_forEachEnd(boneupd->controlListHead,bcontrol);



  // update the bones but not every frame
  // HACK removed boneskip, creates problems with seqTime
  if (change){// && !g_VID.boneSkip){
    if (abs)
      BoneSys_update(bonesys,BONE_UPDATE_ABS,  (boneupd->inWorld) ? worldmatrix : NULL);
    else
      BoneSys_update(bonesys,BONE_UPDATE_RELREF,  (boneupd->inWorld) ? worldmatrix : NULL);
  }

  boneupd->updateTime = g_LuxTimer.time;
}

BoneSysUpdate_t *BoneSysUpdate_new(BoneSys_t *bonesys){
  BoneSysUpdate_t *boneupd;
  int i,n;

  boneupd = lxMemGenZalloc(sizeof(BoneSysUpdate_t));

  boneupd->bonesys = bonesys;
  boneupd->numBones = bonesys->numBones;
  boneupd->bonesAbs = genzallocSIMD(sizeof(lxMatrix44) * boneupd->numBones * 3);
  boneupd->bonesRelT = boneupd->bonesAbs+(16 * boneupd->numBones);
  boneupd->bonesUpd = boneupd->bonesAbs+(16 * boneupd->numBones * 2);

  memcpy(boneupd->bonesAbs,bonesys->refAbsMatrices,sizeof(lxMatrix44) *  boneupd->numBones);
  memcpy(boneupd->bonesRelT,bonesys->refRelMatricesT,sizeof(lxMatrix44) *  boneupd->numBones);

  for (i = 0 ; i < BONE_MAX_SEQ; i++){
    boneupd->seq[i].animspeed = 1.0;
    for (n = 0; n < BONE_MAX_SEQANIMS; n++)
      boneupd->seq[i].anims[n] = -1;
  }

  return boneupd;
}

void  BoneSysUpdate_free(BoneSysUpdate_t *boneupd){
  BoneControl_t *bctrl;
  if (boneupd->numBones && boneupd->bonesAbs && boneupd->bonesRelT){
    genfreeSIMD(boneupd->bonesAbs,sizeof(lxMatrix44) * boneupd->numBones * 3);
  }
  // deactivate bonecontrollers
  while (boneupd->controlListHead){
    lxListNode_popBack(boneupd->controlListHead,bctrl);
    bctrl->boneupd = NULL;
  }
  lxMemGenFree(boneupd,sizeof(BoneSysUpdate_t));
}


void  BoneSysUpdate_reset(BoneSysUpdate_t *boneupd)
{

  memcpy(boneupd->bonesAbs,boneupd->bonesys->refAbsMatrices,sizeof(float) * 16 * boneupd->numBones);
  memcpy(boneupd->bonesRelT,boneupd->bonesys->refRelMatricesT,sizeof(float) * 16 * boneupd->numBones);

}

//////////////////////////////////////////////////////////////////////////
// BONE CONTROL


void  BoneControl_activate(BoneControl_t *bcontrol,BoneSysUpdate_t *boneupd){
  BoneControl_deactivate(bcontrol);
  if ((bcontrol->type == BONE_CONTROL_IK || bcontrol->type == BONE_CONTROL_LOOKAT) && !bcontrol->targetMatrixIF)
    return;
  lxListNode_addLast(boneupd->controlListHead,bcontrol);
  bcontrol->boneupd = boneupd;
}
void  BoneControl_deactivate(BoneControl_t *bcontrol){
  if (!bcontrol->boneupd)
    return;
  lxListNode_remove(bcontrol->boneupd->controlListHead,bcontrol);
  bcontrol->boneupd = NULL;
}


BoneControl_t * BoneControl_new(Bone_t *bone)
{
  BoneControl_t *bcontrol;
  if (!bone)
    return NULL;
  bcontrol = genzallocSIMD(sizeof(BoneControl_t));
  lxMatrix44IdentitySIMD(bcontrol->matrix);
  lxListNode_init(bcontrol);

  bcontrol->influence = 1.0f;
  bcontrol->bone = bone;
  bcontrol->type = BONE_CONTROL_RELATIVE;
  bcontrol->ikDepth = 2;
  bcontrol->ikIterations = BONE_DEFAULT_IK_ITERATIONS;
  bcontrol->ikThresh = BONE_DEFAULT_IK_THRESHOLD;

  bcontrol->reference = Reference_new(LUXI_CLASS_BONECONTROL,bcontrol);

  return bcontrol;
}

void  BoneControl_setMatrixIF(BoneControl_t *bcontrol, Matrix44Interface_t *matrixif)
{
  if (bcontrol->targetMatrixIF)
    Matrix44Interface_unref(bcontrol->targetMatrixIF);
  bcontrol->targetMatrixIF = NULL;
  if (matrixif)
    bcontrol->targetMatrixIF = Matrix44Interface_ref(matrixif);
}

void BoneControl_free(BoneControl_t *bcontrol){
  if (bcontrol->targetMatrixIF)
    Matrix44Interface_unref(bcontrol->targetMatrixIF);
  BoneControl_deactivate(bcontrol);
  Reference_invalidate(bcontrol->reference);
  genfreeSIMD(bcontrol,sizeof(BoneControl_t));
}

void RBoneControl_free(lxRbonecontrol ref)
{
  BoneControl_free((BoneControl_t*)Reference_value(ref));
}

void BoneControl_init()
{
  Reference_registerType(LUXI_CLASS_BONECONTROL,"bonecontrol",RBoneControl_free,NULL);
}


//////////////////////////////////////////////////////////////////////////
// Bone

void Bone_setFromKey(Bone_t *bone,int rotonly){
  static lxMatrix44SIMD rotposM;
  static lxVector3  scale;

  // build final matrix

  if (!rotonly){
    // scale
    lxMatrix44IdentitySIMD(bone->updateMatrix);
    lxVector3Mul(scale,bone->keyPRS.scale,bone->refInvScale);
    lxMatrix44SetScale(bone->updateMatrix,scale);
    // rot
    lxQuatToMatrixIdentity(bone->keyPRS.rot,rotposM);
    lxMatrix44Multiply1SIMD(bone->updateMatrix,rotposM);
    // translation
    lxMatrix44SetTranslation(bone->updateMatrix,bone->keyPRS.pos);
  }
  else{
    // rot
    lxQuatToMatrix(bone->keyPRS.rot,bone->updateMatrix);
    // translation
    lxMatrix44SetTranslation(bone->updateMatrix,bone->refPRS.pos);
  }

  bone->keyPRS.time = 0;
}

void  Bone_setFromBlendKey(Bone_t *bone, float fracc, int rotonlyKey, int rotonlyBlend){
  static lxMatrix44SIMD rotposM;
  static lxVector3  posLerp;
  static lxVector3  scaleLerp;
  static lxQuat rotLerp;

  PRS_t *from;
  PRS_t *to;

  from = &bone->keyPRS;
  to = &bone->blendPRS;

  if (from->time && to->time){
    if (rotonlyKey && rotonlyBlend){
      lxVector3Copy(posLerp,bone->refPRS.pos);
      lxVector3Copy(scaleLerp,bone->refPRS.scale);
    }
    else if (!rotonlyKey && !rotonlyBlend){
      lxVector3Lerp(posLerp,fracc,from->pos,to->pos);
      lxVector3Lerp(scaleLerp,fracc,from->scale,to->scale);
    }
    else if (rotonlyKey && !rotonlyBlend){
      lxVector3Lerp(posLerp,fracc,bone->refPRS.pos,to->pos);
      lxVector3Lerp(scaleLerp,fracc,bone->refPRS.scale,to->scale);
    }
    else {
      lxVector3Lerp(posLerp,fracc,from->pos,bone->refPRS.pos);
      lxVector3Lerp(scaleLerp,fracc,from->scale,bone->refPRS.scale);
    }

    lxQuatSlerp(rotLerp,fracc,from->rot,to->rot);
  }
  else if (!from->time && to->time){
    // use second
    lxVector3Copy(posLerp,to->pos);
    lxVector3Copy(scaleLerp,to->scale);
    lxQuatCopy(rotLerp,to->rot);
  }
  else if (!to->time && from->time){
    // use first
    lxVector3Copy(posLerp,from->pos);
    lxVector3Copy(scaleLerp,from->scale);
    lxQuatCopy(rotLerp,from->rot);
  }
  else {
    // do nothing
    return;
  }

  // build final matrix
  lxMatrix44IdentitySIMD(bone->updateMatrix);
  // scale
  lxVector3Mul(scaleLerp,scaleLerp,bone->refInvScale);
  lxMatrix44SetScale(bone->updateMatrix,scaleLerp);
  // rot
  lxQuatToMatrixIdentity(rotLerp,rotposM);
  lxMatrix44Multiply1SIMD(bone->updateMatrix,rotposM);
  //pos
  lxMatrix44SetTranslation(bone->updateMatrix,posLerp);

  bone->keyPRS.time = 0;
  bone->blendPRS.time = 0;

}

void Bone_enableAxis(Bone_t *bone, int axis, int limit, float minAngleDEG, float maxAngleDEG)
{
  BoneAxis_t *boneaxis = bone->boneaxis;
  lxVector3 origrot;

  lxMatrix44ToEulerZYX(bone->refMatrix,origrot);

  if (!bone)
    return;
  if (!boneaxis){
    boneaxis = bone->boneaxis = reszalloc(sizeof(BoneAxis_t));
  }
  boneaxis->active = LUX_TRUE;
  boneaxis->allowAxis[axis] = LUX_TRUE;
  boneaxis->limitAxis[axis] = limit;
  boneaxis->minAngleAxis[axis] = LUX_DEG2RAD(minAngleDEG) + origrot[axis];
  boneaxis->minAngleAxis[axis] = LUX_MAX(LUX_MIN(boneaxis->minAngleAxis[axis],LUX_MUL_PI),-LUX_MUL_PI);
  boneaxis->maxAngleAxis[axis] = LUX_DEG2RAD(maxAngleDEG) + origrot[axis];
  boneaxis->maxAngleAxis[axis] = LUX_MAX(LUX_MIN(boneaxis->maxAngleAxis[axis],LUX_MUL_PI),-LUX_MUL_PI);
}

void Bone_disableAxis(Bone_t *bone,int axis)
{
  if (!bone)
    return;
  if (bone->boneaxis){
    bone->boneaxis->allowAxis[axis] = LUX_FALSE;
    if (bone->boneaxis->allowAxis[0] || bone->boneaxis->allowAxis[1] || bone->boneaxis->allowAxis[2]);
    else bone->boneaxis->active = LUX_FALSE;
  }
}


static void Bone_orientUpdateMatrix(Bone_t *bone,const lxMatrix44 absmatrix,const lxVector3 pTarget,const lxVector3 pEffector)
{
  static lxVector3 side, up;
  static lxVector3 toTarget;
  static lxVector3 toEffector;

  static lxVector3 anglesNew;
  static lxVector3 anglesOld;

  const float *absaxis[3];
  int i;
  float fNumer;
  float fDenom;
  float fDelta;
  int useAllAxis;
  float *localmatrix;
  BoneAxis_t *boneaxis;


  lxVector3Copy(toTarget,pTarget);
  lxVector3Copy(toEffector,pEffector);

  boneaxis = bone->boneaxis;

  if (boneaxis && boneaxis->active)
    useAllAxis = LUX_FALSE;
  else
    useAllAxis = LUX_TRUE;

  if (lxVector3Dot(toEffector,toTarget) > 0.99999)
    return;

  for (i = 0; i < 3; i++)
    absaxis[i] = &absmatrix[i*4];

  localmatrix = bone->updateMatrix;

  for (i = 0; i < 3; i++){
    if (useAllAxis || boneaxis->allowAxis[i]){
      lxVector3Cross(side,absaxis[i],toEffector);
      lxVector3Cross(up,absaxis[i],side);
      fNumer = lxVector3Dot(toTarget,side);
      fDenom = lxVector3Dot(toTarget,up);

      if ( fNumer*fNumer + fDenom*fDenom <= LUX_FLOAT_EPSILON )
      {
        // undefined atan2, no rotation
        continue;
      }
      lxMatrix44ToEulerZYX(localmatrix,anglesOld);

      // desired angle to rotate about axis(i)

      fDelta = atan2(fNumer,fDenom);

      anglesNew[i] = anglesOld[i] - fDelta;

      if (!useAllAxis && boneaxis->limitAxis[i]){
        //anglesNew[i] = MAX(MIN(anglesNew[i],boneaxis->maxAngleAxis[i]),boneaxis->minAngleAxis[i]);
        if (anglesNew[i] > boneaxis->minAngleAxis[i]){
          if (anglesNew[i]> boneaxis->maxAngleAxis[i])
            anglesNew[i]= boneaxis->maxAngleAxis[i];
        }
        else
          anglesNew[i] = boneaxis->minAngleAxis[i];
      }
      anglesOld[i] = anglesNew[i];

      lxMatrix44FromEulerZYXFast(localmatrix,anglesOld);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
// BoneControl


static void BoneControl_runIK(BoneControl_t *bcontrol,Bone_t *bone){
  static lxMatrix44SIMD matrix;
  static lxVector3 target,current,effector,toEffector,toTarget;
  static Bone_t *bonerecalcs[BONE_MAX_IK_DEPTH+1];

  float *absmatrixBrowse;
  float *absmatrixBone;
  int iter;
  int steps;
  int stepsRecalc;
  Bone_t *bonebrowse;
  float distance;
  Bone_t **bonetraverse;

  Matrix44Interface_getElementsCopy(bcontrol->targetMatrixIF,matrix);
  lxVector3Set(target,matrix[12],matrix[13],matrix[14]);

  // transform targetpos into bonesys coords
  if (!bcontrol->boneupd->inWorld)
    lxVector3InvTransform1(target,bcontrol->boneupd->worldMatrix);

  absmatrixBone = &bone->bonesys->absMatrices[bone->ID];
  bonerecalcs[0] = bone;
  bonebrowse = bone->parentB;
  iter = 0;
  steps = 1;
  do{
    lxVector3Set(effector,absmatrixBone[12],absmatrixBone[13],absmatrixBone[14]);
    distance = lxVector3SqDistance(effector,target);
    if (distance < bcontrol->ikThresh)
      continue;

    absmatrixBrowse = &bonebrowse->bonesys->absMatrices[bonebrowse->ID];
    lxVector3Set(current,absmatrixBrowse[12],absmatrixBrowse[13],absmatrixBrowse[14]);

    lxVector3Sub(toTarget,current,target);
    lxVector3Normalized(toTarget);

    lxVector3Sub(toEffector,effector,current);
    lxVector3Normalized(toEffector);



    Bone_orientUpdateMatrix(bonebrowse,absmatrixBrowse,toTarget,toEffector);
    bonerecalcs[steps] = bonebrowse;
    // renew abs-matrix
    // update all bones that are affected by current bonebrowse
    // naturally that are all steps below us
    // HACK their children however are not updated :/
    for (stepsRecalc = steps; stepsRecalc >= 0; stepsRecalc--){
      // update
      bonebrowse = bonerecalcs[stepsRecalc];
      lxMatrix44MultiplySIMD(&bonebrowse->bonesys->absMatrices[bonebrowse->ID],bonebrowse->parentB ? &bonebrowse->bonesys->absMatrices[bonebrowse->parentID] : lxMatrix44GetIdentity(),bonebrowse->updateMatrix);
    }

    if (steps < bcontrol->ikDepth){
      bonebrowse = bonerecalcs[steps]->parentB;
      steps++;
    }
    else{
      bonebrowse = bone->parentB;
      steps = 1;
      iter++;
    }
  }while (iter < bcontrol->ikIterations && distance > bcontrol->ikThresh);

  if (bcontrol->ikAllChildren){
    // we must traverse all "previous" children of all affected parents
    if (iter > 0)
      steps = bcontrol->ikDepth;
    bonetraverse = bcontrol->boneupd->bonesys->bonestraverse;
    // as we traverse the bonetree depth first, all affected other children are stored "before us"
    // in the array
    // so we need to find the topmost, node in the traverse list first
    while (*bonetraverse != bonerecalcs[steps]) bonetraverse++;
    // the new list starts after the topmost parent
    bonetraverse++;
    steps--;
    // we start to update all children now, but skip the ones we already have

    while (steps--){
      // skip the ones we already have
      while(*bonetraverse != bonerecalcs[steps]){
        // update the child
        bonebrowse = *bonetraverse;
        lxMatrix44MultiplySIMD(&bonebrowse->bonesys->absMatrices[bonebrowse->ID],bonebrowse->parentB ? &bonebrowse->bonesys->absMatrices[bonebrowse->parentID] : lxMatrix44GetIdentity(),bonebrowse->updateMatrix);
        bonetraverse++;
      }
      //do the skip
      bonetraverse++;
    }

  }

}

static void BoneControl_runLookAt(BoneControl_t *bcontrol,lxMatrix44 absmatrix, Bone_t *bone,const lxMatrix44 parentMatrix){
  static lxMatrix44SIMD targetmatrix;
  static lxVector3 forward,targetpos,selfpos, up,side;

  Matrix44Interface_getElementsCopy(bcontrol->targetMatrixIF,targetmatrix);
  lxVector3Set(targetpos,targetmatrix[12],targetmatrix[13],targetmatrix[14]);

  // transform targetpos into bonesys coords
  if (!bcontrol->boneupd->inWorld)
    lxVector3InvTransform1(targetpos,bcontrol->boneupd->worldMatrix);

  lxVector3Set(selfpos,absmatrix[12],absmatrix[13],absmatrix[14]);

  lxVector3Sub(forward,selfpos,targetpos);
  lxVector3Normalized(forward);
  // now transform with local matrix (in case user wants some sort of rotationfixes)
  lxVector3TransformRot1(forward,bcontrol->matrix);

  if (bcontrol->invAxis)
    lxVector3Negated(forward);


  if (bone->boneaxis && bone->boneaxis->active){
    switch(bcontrol->axis){
      case 0: lxVector3Set(up,absmatrix[0],absmatrix[1],absmatrix[2]); break; // 1,0,0
      case 1: lxVector3Set(up,absmatrix[4],absmatrix[5],absmatrix[6]); break; // 0,1,0
      case 2: lxVector3Set(up,absmatrix[8],absmatrix[9],absmatrix[10]); break;  // 0,0,1
    }

    Bone_orientUpdateMatrix(bone,absmatrix,forward,up);
    // renew absmatrix
    if (parentMatrix)
      lxMatrix44MultiplySIMD(absmatrix,parentMatrix,bone->updateMatrix);
    else
      lxMatrix44CopySIMD(absmatrix,bone->updateMatrix);
  }
  else{
    switch (bcontrol->axis){
      case 0:
        lxVector3Set(up,absmatrix[4],absmatrix[5],absmatrix[6]);  // 0,1,0
        lxVector3Cross(side,up, forward);
        lxVector3Normalized(side);
        lxVector3Cross(up, forward, side);

        absmatrix[0] = -forward[0];
        absmatrix[1] = -forward[1];
        absmatrix[2] = -forward[2];

        absmatrix[4] = up[0];
        absmatrix[5] = up[1];
        absmatrix[6] = up[2];

        absmatrix[8] = side[0];
        absmatrix[9] = side[1];
        absmatrix[10] = side[2];
        break;
      case 1:
        lxVector3Set(up,absmatrix[8],absmatrix[9],absmatrix[10]);
        lxVector3Cross(side,up, forward);
        lxVector3Normalized(side);
        lxVector3Cross(up,forward, side);

        absmatrix[0] = side[0];
        absmatrix[1] = side[1];
        absmatrix[2] = side[2];

        absmatrix[4] = -forward[0];
        absmatrix[5] = -forward[1];
        absmatrix[6] = -forward[2];

        absmatrix[8] = up[0];
        absmatrix[9] = up[1];
        absmatrix[10] = up[2];
        break;
      case 2:
        lxVector3Set(up,absmatrix[4],absmatrix[5],absmatrix[6]);  // 0,1,0
        lxVector3Cross(side,up, forward);
        lxVector3Normalized(side);
        lxVector3Cross(up,forward, side);

        absmatrix[0] = side[0];
        absmatrix[1] = side[1];
        absmatrix[2] = side[2];

        absmatrix[4] = up[0];
        absmatrix[5] = up[1];
        absmatrix[6] = up[2];

        absmatrix[8] = -forward[0];
        absmatrix[9] = -forward[1];
        absmatrix[10] = -forward[2];
        break;
    }
  }
}



// BONE SYS
// ---------
// CREATES FINAL MATRIX FOR EACH BONE
//------------------------------------
void BoneSys_updateBone(BoneSys_t *bonesys,Bone_t *bone)
{
  float *finalAbsMatrix;

  finalAbsMatrix = &bonesys->absMatrices[bone->ID];

  if (!bone->parentB || (bone->bcontrol && bone->bcontrol->type == BONE_CONTROL_ABS)){
    lxMatrix44CopySIMD(finalAbsMatrix,bone->updateMatrix);
  }
  else {
    lxMatrix44MultiplySIMD(finalAbsMatrix,&bonesys->absMatrices[bone->parentID],bone->updateMatrix);
  }

  if (bone->bcontrol){
    switch (bone->bcontrol->type){
      case BONE_CONTROL_LOOKAT:
        BoneControl_runLookAt(bone->bcontrol,finalAbsMatrix,bone,(bone->parentB) ? &bonesys->absMatrices[bone->parentID] : NULL);
        break;
      case BONE_CONTROL_IK:
        BoneControl_runIK(bone->bcontrol,bone);
        if (bone->bcontrol->ikLookAt)
          BoneControl_runLookAt(bone->bcontrol,finalAbsMatrix,bone,(bone->parentB) ? &bonesys->absMatrices[bone->parentID] : NULL);
        break;
    }
    bone->bcontrol = NULL;
  }

}

void BoneSys_updateBone_world(BoneSys_t *bonesys,Bone_t *bone, const float *worldmatrix)
{
  float *finalAbsMatrix;
  const float *parentmat;

  finalAbsMatrix = &bonesys->absMatrices[bone->ID];


  if (bone->bcontrol && bone->bcontrol->type == BONE_CONTROL_ABS)
  {
    lxMatrix44CopySIMD(finalAbsMatrix,bone->updateMatrix);
    parentmat = NULL;
  }
  else if (bone->parentB){
    parentmat = &bonesys->absMatrices[bone->parentID];
    lxMatrix44MultiplySIMD(finalAbsMatrix,parentmat,bone->updateMatrix);
  }
  else{
    parentmat = worldmatrix;
    lxMatrix44MultiplySIMD(finalAbsMatrix,worldmatrix,bone->updateMatrix);
  }

  if (bone->bcontrol){
    switch (bone->bcontrol->type){
      case BONE_CONTROL_LOOKAT:
        BoneControl_runLookAt(bone->bcontrol,finalAbsMatrix,bone,parentmat);
        break;
      case BONE_CONTROL_IK:
        BoneControl_runIK(bone->bcontrol,bone);
        if (bone->bcontrol->ikLookAt)
          BoneControl_runLookAt(bone->bcontrol,finalAbsMatrix,bone,parentmat);
        break;
    }
    bone->bcontrol = NULL;
  }

}

// creates proper traverse order
void BoneNode_initTraverse_recursive(BoneNode_t *node,BoneSys_t *bonesys)
{
  BoneNode_t *browse;
  static int cnt;

  if (node->bone)
    bonesys->bonestraverse[cnt++] = node->bone;
  else  // root node
    cnt = 0;

  lxListNode_forEach(node->childrenListHead,browse)
     BoneNode_initTraverse_recursive(browse,bonesys);
  lxListNode_forEachEnd(node->childrenListHead,browse);
}


// CREATES BONE HIERARCHY
//------------------------
int  BoneSys_link(BoneSys_t *bonesys)
{
  Bone_t *bone;
  BoneNode_t *bonenode;
  BoneNode_t  *treenodes;
  BoneNode_t  *root;
  int i;

  // create temp tree
  treenodes = lxMemGenZalloc(sizeof(BoneNode_t)*(bonesys->numBones+1));
  root = &treenodes[bonesys->numBones];

  // prep nodes
  bone = bonesys->bones;
  bonenode = treenodes;
  for (i = 0; i < bonesys->numBones; i++, bone++,bonenode++){
    bone->ID = i*16;
    lxListNode_init(bonenode);
    bonenode->bone = bone;
  }
  // Link children to parent
  bone = bonesys->bones;
  bonenode = treenodes;
  for (i = 0; i < bonesys->numBones; i++, bone++,bonenode++){
    if( bone->parentID){
      if (bone->parentID > bonesys->numBones){
        lxMemGenFree(treenodes,sizeof(BoneNode_t)*(bonesys->numBones+1));
        return LUX_FALSE;
      }
      bone->parentID--;
      bone->parentB = &bonesys->bones[bone->parentID];
      lxListNode_addLast(treenodes[bone->parentID].childrenListHead,bonenode);
      bone->parentID *=16;
    }
    else{
      lxListNode_addLast(root->childrenListHead,bonenode);
    }
  }

  BoneNode_initTraverse_recursive(root,bonesys);

  lxMemGenFree(treenodes,sizeof(BoneNode_t)*(bonesys->numBones+1));

  return LUX_TRUE;
}

// CREATES FINAL INIT MATRICES FOR ALL BONES
//-------------------------------------------
void BoneSys_init(BoneSys_t *bonesys)
{
  int i;
  Bone_t *bone;
  Bone_t  **bonestraverse;

  LUX_SIMDASSERT(((size_t)((Bone_t*)NULL)->refMatrix) % 16 == 0
          && sizeof(Bone_t) % 16 == 0);
  LUX_SIMDASSERT(((size_t)((BoneControl_t*)NULL)->matrix) % 16 == 0);

  for (i = 0; i < bonesys->numBones; i++){
    bone = &bonesys->bones[i];
    bone->bonesys = bonesys;
    // boneinit uses always the relative matrix for applying changes
    // our initial "change" is to bring the former world position in reference to the
    // relative position of our bones
    lxMatrix44CopySIMD(bone->updateMatrix,bone->refMatrix);
  }

  // traverse
  bonestraverse = bonesys->bonestraverse;
  for (i = 0; i < bonesys->numBones; i++,bonestraverse++){
    BoneSys_updateBone(bonesys,*bonestraverse);
  }


  // create the refInvMatrix
  if (bonesys->refInvMatrices){
    for (i = 0; i < bonesys->numBones; i++){
      bone = &bonesys->bones[i];
      lxMatrix44AffineInvertSIMD(&bonesys->refInvMatrices[i*16],&bonesys->absMatrices[i*16]);
      lxMatrix34IdentitySIMD(&bonesys->relMatricesT[i*12]);
    }
  }
  bonesys->refAbsMatrices = bonesys->absMatrices;
  bonesys->refRelMatricesT = bonesys->relMatricesT;
}
// CREATES FINAL UPDATED MATRICES FOR ALL BONES
//----------------------------------------------
void BoneSys_update(BoneSys_t *bonesys,const BoneUpdateType_t update, const float *worldmatrix)
{
  int i;
  Bone_t  *bone;
  int   relative;
  float *in;
  float *out;
  float *inv;
  Bone_t  **bonestraverse;


  if (update == BONE_UPDATE_NONE)
    return;

  relative = (update == BONE_UPDATE_RELREF) ? LUX_TRUE : LUX_FALSE;
  bone = bonesys->bones;
  for (i = 0; i < bonesys->numBones; i++,bone++){
    // do our transforms
    // in case relative was never inited take ref pose
    if(!bone->updateMatrix[15]){
      lxMatrix44CopySIMD(bone->updateMatrix,bone->refMatrix);
    }
    else if (relative){
      lxMatrix44Multiply2SIMD(bone->refMatrix,bone->updateMatrix);
    }
  }

  // traverse
  bonestraverse = bonesys->bonestraverse;
  if (worldmatrix)
    for (i = 0; i < bonesys->numBones; i++,bonestraverse++){
      BoneSys_updateBone_world(bonesys,*bonestraverse,worldmatrix);
    }
  else
    for (i = 0; i < bonesys->numBones; i++,bonestraverse++){
      BoneSys_updateBone(bonesys,*bonestraverse);
    }

  // compute relative matrices for skinning
  if (bonesys->refInvMatrices){
    bone = bonesys->bones;
    in = bonesys->absMatrices;
    out = bonesys->relMatricesT;
    inv = bonesys->refInvMatrices;
    for (i = 0; i < bonesys->numBones; i++,bone++,in+=16,out+=12,inv+=16){
      lxMatrix34TMultiply44SIMD(out,in,inv);
    }
  }
  // done making our finals now clear the updates
  bone = bonesys->bones;
  for (i = 0; i < bonesys->numBones; i++,bone++){
    bone->updateMatrix[15] = 0;
  }

}

int   BoneSys_searchBone(BoneSys_t  *bonesys, const char *name){
  int i;

  for (i = 0; i < bonesys->numBones; i++){
    if (strcmp(bonesys->bones[i].name, name) == 0)
      return i;
  }
  return -1;
}
Bone_t* BoneSys_getBone(BoneSys_t *bonesys, const char *name){
  int i;

  for (i = 0; i < bonesys->numBones; i++){
    if (strcmp(bonesys->bones[i].name, name) == 0)
      return &bonesys->bones[i];
  }
  return NULL;
}

