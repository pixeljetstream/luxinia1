// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __CRCTABLE_H__
#define __CRCTABLE_H__

CRC32entry_t CRC32table[]=
{
  {"base/main.lua", 3391496937},
#ifdef LUX_DEV
  {"base/textures/logo1.tga", 2636858941},
#else
  {"base/textures/logo1.tga", 2367177421},
#endif
  {"base/textures/logo2.tga", 73497786},
  { NULL, 0},
};

#endif

