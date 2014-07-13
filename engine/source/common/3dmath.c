// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "3dmath.h"
#include "../common/common.h"

//////////////////////////////////////////////////////////////////////////
// LOCALS
float g_mathfrandtable[MATH_MAX_FRAND];
static lxFastMathCache_t l_fastmath;

//////////////////////////////////////////////////////////////////////////
// Funcs

void Math_init(){
  int i;
  for (i = 0; i < MATH_MAX_FRAND; i++)
    g_mathfrandtable[i] = lxFrand();

  lxFastMath_init(&l_fastmath);

  frandPointer(NULL);
  frandPointerSeed(NULL,0);
}

struct lxFastMathCache_s* Luxinia_getFastMathCache()
{
  return &l_fastmath;
}

