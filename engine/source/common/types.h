// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __TYPES_H__
#define __TYPES_H__

#include <luxinia/luxmath/basetypes.h>
#include <luxinia/luxplatform/debug.h>

// Simd is supported on MSDEV
#ifdef _MSC_VER
  #define LUX_SIMD
  #pragma warning(disable:4142)   // redefinition of same
  #pragma warning(disable:4244 4305)  // for VC++, no precision loss complaints
#endif

enum TransformType_e{
  POS,
  POS_VELOCITY,
  ROT_RAD,
  ROT_DEG,
  ROT_AXIS,
  ROT_QUAT,
  SCALE,
};

enum BitID_e{
  BIT_ID_ZERO = 0,
  BIT_ID_1 = 1,
  BIT_ID_2 = 1<<1,
  BIT_ID_3 = 1<<2,
  BIT_ID_4 = 1<<3,
  BIT_ID_5 = 1<<4,
  BIT_ID_6 = 1<<5,
  BIT_ID_7 = 1<<6,
  BIT_ID_8 = 1<<7,
  BIT_ID_9 = 1<<8,
  BIT_ID_10 = 1<<9,
  BIT_ID_11 = 1<<10,
  BIT_ID_12 = 1<<11,
  BIT_ID_13 = 1<<12,
  BIT_ID_14 = 1<<13,
  BIT_ID_15 = 1<<14,
  BIT_ID_16 = 1<<15,
  BIT_ID_17 = 1<<16,
  BIT_ID_18 = 1<<17,
  BIT_ID_19 = 1<<18,
  BIT_ID_20 = 1<<19,
  BIT_ID_21 = 1<<20,
  BIT_ID_22 = 1<<21,
  BIT_ID_23 = 1<<22,
  BIT_ID_24 = 1<<23,
  BIT_ID_25 = 1<<24,
  BIT_ID_26 = 1<<25,
  BIT_ID_27 = 1<<26,
  BIT_ID_28 = 1<<27,
  BIT_ID_29 = 1<<28,
  BIT_ID_30 = 1<<29,
  BIT_ID_31 = 1<<30,
  BIT_ID_32 = 1<<31,
  BIT_ID_FULL8 = 0xFF,
  BIT_ID_FULL16 = 0xFFFF,
  BIT_ID_FULL24 = 0xFFFFFF,
  BIT_ID_FULL32 = 0xFFFFFFFF,
};

typedef enum RotationLocks_e
{
  ROTATION_LOCK_ALL   = 1,
  ROTATION_LOCK_X   = 2,
  ROTATION_LOCK_Y   = 4,
  ROTATION_LOCK_Z   = 8
}RotationLocks_t;

typedef float Vector6[6];

// unfortunately this was the best place for this
// it must come first for any resource, resRID must come first
typedef struct ResourceInfo_s{
  union{
    char  nameID[4];
    int   typeID;
  };
  int     resRID;
  char    *name;
  char    *extraname;
  int     outerRefCnt;
  int     userRefCnt;
  int     depth;

  struct ResourceChunk_s* reschunk;
  char          userstring[256];
}ResourceInfo_t;  // 8 DW + 64 DW

typedef enum32 VIDTexBlendHash;

#endif
