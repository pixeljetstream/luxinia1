// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include <stdio.h>
#include <stdlib.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <memory.h>
#include <string.h>

#include "../common/common.h"
#include "../common/reflib.h"

lxObjRefSysPTR g_RefSys = NULL;

void Reference_init()
{
  g_RefSys = lxObjRefSys_new(lxMemoryGeneric_allocator(g_GenericMem));
}

static booln Reference_reportLeak(Reference ref)
{
  LUX_PRINTF("Ref Leak: %d %p u:%d w:%d\n",ref->id.type,ref->id.ptr,ref->usecounter,ref->weakcounter);
  return LUX_FALSE;
}

void Reference_deinit()
{
  lxObjRefSys_deinit(GLOBALREFSYS,Reference_reportLeak);
  lxObjRefSys_delete(GLOBALREFSYS);
}

void Reference_registerType(lxClassType type, const char *classname,
              lxObjRefDelete_fn *fn, struct RefClass_s *classinfo)
{
  LUX_ASSERT(classinfo == NULL);
  lxObjRefSys_register(GLOBALREFSYS,type,lxObjTypeInfo_new(fn,classname));
}

int Reference_getTotalCount ()
{
  return lxObjRefSys_refCount(GLOBALREFSYS);
}

LUXIAPI booln LuxiniaRefSys_getSafe(lxRef lref, void **out)
{
  return lxObjRef_getSafe(lref,out);
}
LUXIAPI booln LuxiniaRefSys_addUser(lxRef lref)
{
  return lxObjRef_addUser(lref);
}
LUXIAPI void  LuxiniaRefSys_addWeak(lxRef lref)
{
  lxObjRef_addWeak(lref);
}
LUXIAPI booln LuxiniaRefSys_releaseUser(lxRef lref)
{
  return lxObjRef_releaseUser(lref);
}
LUXIAPI void  LuxiniaRefSys_releaseWeak(lxRef lref)
{
  lxObjRef_releaseWeak(lref);
}


static LuxiniaRefSys_t l_luxrefsys = {
  NULL,
  LuxiniaRefSys_getSafe,
  LuxiniaRefSys_addUser,
  LuxiniaRefSys_addWeak,
  LuxiniaRefSys_releaseUser,
  LuxiniaRefSys_releaseWeak,
};

LUXIAPI LuxiniaRefSys_t*    Luxinia_getRefSys()
{
  l_luxrefsys.refsys = GLOBALREFSYS;
  return &l_luxrefsys;
}

