// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef _LUAREF_H_
#define _LUAREF_H_

#include <luxinia/reftypes.h>
#include <luxinia/reference.h>
typedef lxRef Reference;

#define REF2VOID(ptr) ((void*)ptr)

#include <luxinia/luxcore/refsys.h>

extern lxObjRefSysPTR g_RefSys;
#define GLOBALREFSYS  g_RefSys

#define Reference_new(type,ptr)   (lxObjRefSys_newRef(GLOBALREFSYS,(type),(ptr)))
#define Reference_get(ref,ptr)    (lxObjRef_getSafe((ref),&(ptr)))
#define Reference_reset(ptr)    ((ptr) = NULL)
#define Reference_value(ref)    ((ref)->id.ptr)
#define Reference_type(ref)     ((ref)->id.type)
#define Reference_isValid(ref)    (lxObjRef_getId(ref)!=NULL)
#define Reference_invalidate(ref) (lxObjRefSys_invalidateRef(GLOBALREFSYS,(ref),LUX_FALSE))
#define Reference_free(ref)     (lxObjRefSys_invalidateRef(GLOBALREFSYS,(ref),LUX_TRUE))
#define Reference_release(ref)    lxObjRef_releaseUser((ref))
#define Reference_releaseWeak(ref)  lxObjRef_releaseWeak((ref))
#define Reference_ref(ref)      lxObjRef_addUser(ref)
#define Reference_refWeak(ref)    lxObjRef_addWeak(ref)


#define Reference_releaseWeakCheck(ref) \
  if ((ref)){\
    lxObjRef_releaseWeak((ref));\
  }

#define Reference_releaseCheck(ref) \
  if ((ref)){\
    lxObjRef_releaseUser((ref));\
  }

#define Reference_releaseClear(ref) \
  if ((ref)){\
    lxObjRef_releaseUser((ref));\
    ref = NULL;\
  }

#define Reference_releasePtr(ptr,ref) \
  if (ptr){ \
    lxObjRef_releaseUser((ptr->ref));\
  }

#define Reference_releasePtrClear(ptr,ref)  \
  if (ptr){\
    lxObjRef_releaseUser((ptr->ref));\
    ptr = NULL;\
  }

void Reference_init();
void Reference_deinit();

void Reference_registerType(lxClassType type, const char *classname,
              lxObjRefDelete_fn *fn, struct RefClass_s *classinfo);

int Reference_getTotalCount ();
#define Reference_makeStrong    LUX_NOOP

#define Reference_makeVolatile(ref) {booln succ=lxObjRef_makeVolatile(ref); LUX_DEBUGASSERT(succ);}


#endif


