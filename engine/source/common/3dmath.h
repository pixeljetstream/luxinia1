// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __3DMATH_H__
#define __3DMATH_H__

#include  <luxinia/luxmath/luxmath.h>
#include  "../common/types.h"

//////////////////////////////////////////////////////////////////////////
// Math LIB

#define MATH_MAX_FRAND      4096
extern float  g_mathfrandtable[MATH_MAX_FRAND];

//////////////////////////////////////////////////////////////////////////
void Math_init();

// pseudo random 0-1 float generator
#define randPointerSeed(pt,seed,offset,max) (((((uint)(pt))*(seed))+(offset))%(max))
#define frandPointer(pt)          g_mathfrandtable[(((uint)(pt))*3)%MATH_MAX_FRAND]
#define frandPointerSeed(pt,seed)     g_mathfrandtable[(((uint)(pt))*(seed))%MATH_MAX_FRAND]


#define lxVector3Unpack(a)  ((a)[0]),((a)[1]),((a)[2])
#define lxVector4Unpack(a)  ((a)[0]),((a)[1]),((a)[2]),((a)[3])
#define lxVector3Compare(a,cmp,b) ( ((a)[0]) cmp ((b)[0]) && ((a)[1]) cmp ((b)[1]) && ((a)[2]) cmp ((b)[2]))

#endif
