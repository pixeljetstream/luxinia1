#ifndef __LINKOBJECT_H__
#define __LINKOBJECT_H__

#include "../common/common.h"

typedef enum LinkType_e{
  // first 3 must stay like this
  LINK_UNSET    = -1,
  LINK_S3D    = 0,
  LINK_ACT    = 1,
  // only for particles so far
  LINK_L3DBONE  = 2,
}LinkType_t;

typedef struct LinkObject_s{
  // 16 DW - 4 DW aligned
  lxMatrix44    matrix;
  // 4 DW
  struct VisObject_s      *visobject;
  struct Matrix44Interface_s  *matrixIF;
  lxRef       reference;
  ulong           updframe;
}LinkObject_t;  // 20 DW

typedef struct MatrixLinkObject_s{
  struct Matrix44Interface_s  *matrixIF;
}MatrixLinkObject_t;  // 2 DW


#endif

