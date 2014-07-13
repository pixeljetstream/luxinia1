// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __COMMON_H__
#define __COMMON_H__

//#define LUX_DEBUG_FRAMEPRINT
  //  will print state per-frame (lots of text), useful for oldschool debugging
  //

//#define LUX_VID_GEOSHADER
  //  this will allow geometry shader

#define LUX_MEMPOOL_SAVE
  // less memory consumption in pools, sacrificing speed a bit

//#define LUX_VID_CG_DEBUG
  // Debug Checking for Cg Runtime

//#define LUX_VID_FORCE_LIMITPRIMS
  // draws only the first primitive (cept for text)

#ifdef _DEBUG
#define LUX_VID_CG_DEBUG
#endif


//////////////////////////////////////////////////////////////////////////

// --------
// INCLUDES
// --------

// External
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

// Common
#include "../common/types.h"
#include "../common/memorymanager.h"
#include "../common/linkedlistprimitives.h"
#include "../common/interfaceobjects.h"
#include "../common/timer.h"
#include "../fileio/log.h"
#include "../common/profiling.h"

#include <luxinia/luxinia.h>
#include <luxinia/luxcore/arraymisc.h>
#include <luxinia/luxcore/strmisc.h>
#include <luxinia/luxcore/scalarmisc.h>

#include "../version.h"

// -------
// DEFINES
// -------


#ifdef LUX_DEBUG_FRAMEPRINT
  #define LUX_DEBUG_FRPRINT LUX_PRINTF
  #define LUX_DEBUG_FRFLUSH fflush(NULL)
#else
  #define LUX_DEBUG_FRPRINT LUX_NOOP
  #define LUX_DEBUG_FRFLUSH
#endif


#define MAX_PATHLENGTH  512


#define clearArray(array,size) memset( (array), 0, sizeof((array)[0])*(size)); //clear the array


#define isFlagSet(num, flag)  (((num) & (flag))!=0)
#define setFlag(num, flag)    ((num) |= (flag))
#define clearFlag(num, flag)  ((num) &= ~(flag))
#define toggleFlag(num, flag) ((num) ^= (flag))

#define prd(a) LUX_PRINTF("%s: %d\n",#a,a)
#define prf(a) LUX_PRINTF("%s: %f\n",#a,a)
#define prs(a) LUX_PRINTF("%s: %s\n",#a,a)
#define pr(a)  LUX_PRINTF("%s\n",#a)


#endif

