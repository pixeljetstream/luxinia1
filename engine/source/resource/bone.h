// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __BONE_H__
#define __BONE_H__

/*
BONESYSTEM
----------

The Bonesystem contains various bones which are linked to each other and create a hierarchy.

A BoneUpdate contains information about how a bonsystem should be modified and if, it will
store the results of such operation, leaving the host bonesystem at the original state.
All final matrices of the Bones are stored in a single interleaved array, so that they can
be exchanged easily.

The BoneUpdate will allow Animations to be mixed and played simultanously blended and so on.
There are currently 2 sequences played sequentially. And within a sequence 2 animation can
be overlapped or played after another.
There is also 3 kind of individual BoneControllers, RELATIVE, LOOKAT, IK which can further
change the bones' state. IK does only affect the bone's parents direclty not its siblings
or other children of the its parents.

Ideally the BoneSystem is nested in a model or somewhere else, then for every instance of that
system you create a boneupdate. After that the BoneUpdate is changed depending on which controllers,
animations the user sets it to.

Atm a Bone parent does not know its children.

Bone    (nested,ResMem)
BoneSystem  (nested,ResMem)
BoneUpdate  (nested,GenMem)
BoneControl (user, GenMem)

Author: Christoph Kubisch

*/

#include "../common/common.h"
#include "../common/reflib.h"

#define BONE_MAX_SEQ      2
#define BONE_MAX_SEQANIMS   2
#define BONE_DEFAULT_IK_THRESHOLD 2
#define BONE_DEFAULT_IK_ITERATIONS  10
#define BONE_MAX_IK_DEPTH   8

typedef enum BoneControlType_e{
  BONE_CONTROL_RELATIVE,
  BONE_CONTROL_ABS,
  BONE_CONTROL_LOOKAT,
  BONE_CONTROL_IK,
}BoneControlType_t;

typedef enum BoneUpdateType_e{
  BONE_UPDATE_NONE,         // no update performed
  BONE_UPDATE_ABS,          // takes update matrix as final (non hierarchical)
  BONE_UPDATE_RELREF,         // relative change to original reference matrix
}BoneUpdateType_t;

enum BoneSequenceState_e{
  BONE_SEQ_INACTIVE,
  BONE_SEQ_RUN,       // runs it once and then sets to stop
  BONE_SEQ_STOP,        // halts at current frame
  BONE_SEQ_SINGLE,
};

enum BoneFlag_e{
  BONE_LOOP     = 1<<0,
  BONE_MANUAL_BLEND = 1<<1,
  BONE_TIME_SET   = 1<<2,
  BONE_SINGLE_FRAME = 1<<3,
};

enum PRSFlag_e{
  PRS_NONE      = 0,
  PRS_POS       = 1<<0,
  PRS_ROT       = 1<<1,
  PRS_SCALE     = 1<<2,
  PRS_ALL       = PRS_POS | PRS_ROT | PRS_SCALE,
};

typedef struct PRS_s{
  // 4 DW
  lxVector3   pos;
  float   time;
  // 4 DW
  lxQuat    rot;
  // 4 DW
  lxVector3   scale;
  enum32    usedflags;
} PRS_t;

typedef struct Bone_s
{
  // 2 DW
  int   ID;         // his own index*16
  int   parentID;     // 0 if non else index+1, then after init index*16
  // 2 DW
  struct Bone_s     *parentB;
  struct BoneControl_s  *bcontrol;    // lookat/IK controller

  // 32 DW - 4 DW aligned
  lxMatrix44    refMatrix;    // at init/rest position relative to parent
  lxMatrix44    updateMatrix; // changes for the frame

  // 36 DW
  PRS_t   keyPRS;
  PRS_t   blendPRS;
  PRS_t   refPRS;
  // 3 DW
  lxVector3   refInvScale;

  // 3 DW
  struct BoneAxis_s   *boneaxis;
  struct BoneSys_s    *bonesys;   // system it is part of
  char          *name;
  // number of DW must be divadable by 4
  // 2 DW
  uint    _pad[2];
}Bone_t;

typedef struct BoneNode_s{
  Bone_t        *bone;
  struct BoneNode_s *childrenListHead;
  struct BoneNode_s LUX_LISTNODEVARS;
}BoneNode_t;

typedef struct BoneAxis_s{
  ushort    active;
  uchar   allowAxis[3]; // 3 axis
  uchar   limitAxis[3];
  float   minAngleAxis[3];
  float   maxAngleAxis[3];
}BoneAxis_t;

typedef struct BoneControl_s{
  // 2 DW
  BoneControlType_t type;
  Reference reference;
  // 2 DW
  Bone_t    *bone;
  struct BoneSysUpdate_s *boneupd;

  // RELATIVE, LOOKAT
  // 16 DW
  lxMatrix44    matrix;         // the local update matrix

  // ALL
  float   influence;

  // LOOKAT
  Matrix44Interface_t *targetMatrixIF;// object to be tracked

  uchar   axis;         // the axis which should aim at the goal
  uchar   invAxis;        // invert the axis
  uchar   ikDepth;        // depth we step up in hierarchy
  uchar   ikLookAt;       // run look at for main node
  int     ikAllChildren;      // updates all affected children
  float   ikThresh;
  int     ikIterations;

  // FIXME cant reuse..
  struct BoneControl_s *prev,*next; // within boneupd
}BoneControl_t;

typedef struct BoneSequence_s{
  int         state;
  int         flag;

  uint        time;     // the g_LuxTimer.time of last evaluation
  uint        duration;
  uint        runtime;
  float       runtimefracc;
  float       blendfactor;  // manual blend, needs special flag to be set

  float       animspeed;    // speed at which sequence is played
  int         anims[BONE_MAX_SEQANIMS];
  uint        animStartTimes[BONE_MAX_SEQANIMS];  // the animtime at which it will start when playing starts
  uint        animLengths[BONE_MAX_SEQANIMS];   // duration of anim
  uint        anim1Start;   // the runtime at which anim1 will start

  int         lastLoopAnims[BONE_MAX_SEQANIMS];
  enum32        animflags[BONE_MAX_SEQANIMS];
  //int         *loopKeyStart[BONE_MAX_SEQANIMS];
  //int         *loopKeyEnd[BONE_MAX_SEQANIMS];
}BoneSequence_t;

typedef struct BoneSysUpdate_s{
  struct BoneSys_s  *bonesys;

  BoneSequence_t  seq[BONE_MAX_SEQ];

  int       inWorld;    // bones abs matrices are computed in world coordinates
  float     *worldMatrix;

  int       numBones;
  float     *bonesAbs;    // pointer to bones absolute matrix
  float     *bonesRelT;   // pointer to the bones relative matrix transposed
  float     *bonesUpd;    // the last update anims we used

  BoneControl_t *controlListHead;

  uint      updateTime;
}BoneSysUpdate_t;


typedef struct BoneSys_s
{
  Bone_t    *bones;
  Bone_t    **bonestraverse;
  int     numBones;
  //Bone_t    *rootListHead;


  // targets and "to be used" pointers
  // just pointers, no allocs
  float   *absMatrices;
  float   *relMatricesT;    //transposed Matrix34

  // allocs
  // user cache
  float   *userAbsMatrices;
  float   *userRelMatricesT;

  // on first init
  float   *refAbsMatrices;
  float   *refRelMatricesT; //transposed Matrix34
  float   *refInvMatrices;  // inverse matrix of initial state after hierarchy, only exists if we have skinning

}BoneSys_t;

typedef struct BoneAssign_s
{
  BoneSys_t     *bonesys;
  Bone_t        **bones;    // bones pointer
  struct BoneAssign_s LUX_LISTNODEVARS;
}BoneAssign_t;
///////////////////////////////////////////////////////////////////////////////
// PRS

void PRS_setMatrix(PRS_t *prskey,lxMatrix44 matrix);

///////////////////////////////////////////////////////////////////////////////
// BONE

void  Bone_setFromKey(Bone_t *bone, int rotonly);
void  Bone_setFromBlendKey(Bone_t *bone, float fracc, int rotonlyKey, int rotonlyBlend);

// limit axis when BoneControl is active
// this func allows user to manually enable axis allowed for rotation
// axis = 0 - 2 (XYZ)
// limit = boolean if rotation should be limited
void Bone_enableAxis(Bone_t *bone, int axis, int limit, float minAngleDEG, float maxAngleDEG);

void Bone_disableAxis(Bone_t *bone,int axis);

///////////////////////////////////////////////////////////////////////////////
// BONE SYS


  // link our bones for hierarchy
int   BoneSys_link(BoneSys_t  *bonesys);

  // initializes bones relative matrix
void  BoneSys_init(BoneSys_t  *bonesys);

void  BoneSys_update(BoneSys_t *bonesys,const BoneUpdateType_t update,  const float *worldmatrix);

int   BoneSys_searchBone(BoneSys_t  *bonesys, const char *name);

Bone_t* BoneSys_getBone(BoneSys_t *bonesys, const char *name);

///////////////////////////////////////////////////////////////////////////////
// BONESYS UPDATE

BoneSysUpdate_t *BoneSysUpdate_new(BoneSys_t  *bonesys);
void  BoneSysUpdate_free(BoneSysUpdate_t *boneupd);

  // resets all bone data to reference
void  BoneSysUpdate_reset(BoneSysUpdate_t *boneupd);

  // updates all BoneSquences sequentially
  // and performs BoneController updates
  // worldpos can be NULL, only needed for lookat controller
void  BoneSysUpdate_run(BoneSysUpdate_t *boneupd, lxMatrix44 worldpos);


///////////////////////////////////////////////////////////////////////////////
// BONE CONTROL

  // new IK controller
  // depth = how many steps up in hierarchy to be taken
  // axis = target axis
  // invAxis = invert target axis
  // matrixIF or relativemat are used to define how the goal should be aimed at
  // lookat is the world position of target
  // default all axis are allowed and have no limits
  // use "Bone_enableAxis" to limit axis

  // new lookat controller
  // axis = target axis
  // invAxis = invert target axis
  // matrixIF or relativemat are used to define how the goal should be aimed at
  // lookat is the world position of target
  // default all axis are allowed and have no limits
  // use "Bone_enableAxis" to limit axis

  // relative
  // just changes local matrix with the given
BoneControl_t * BoneControl_new(Bone_t *bone);
//void BoneControl_free(BoneControl_t *bcontrol);
void  RBoneControl_free(lxRbonecontrol ref);

void  BoneControl_setMatrixIF(BoneControl_t *bcontrol, Matrix44Interface_t *matrixif);

void  BoneControl_activate(BoneControl_t *bcontrol,BoneSysUpdate_t *boneupd);

void  BoneControl_deactivate(BoneControl_t *bcontrol);

void  BoneControl_init();


#endif
