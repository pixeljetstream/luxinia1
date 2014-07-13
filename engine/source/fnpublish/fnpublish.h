// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef _FNPUBLISH_H_
#define _FNPUBLISH_H_

#include <luxinia/luxcore/contstringmap.h>
#include <luxinia/luxcore/contscalararray.h>
#include "../common/reflib.h"

#define FNPUB_MAGICVALUE  (1442968193ul)
#define FNPUB_STACKSIZE   64
#define FNPUB_INTERFACES  8

//////////////////////////////////////////////////////////////////////////
// Casting Helpers

#define FNPUB_FINT(flt)   LUXI_CLASS_INT,(void*)flt
#define FNPUB_TINT(flt)   LUXI_CLASS_INT,(void*)&(flt)

#define FNPUB_FSTR(flt)   LUXI_CLASS_STRING,(void*)flt
#define FNPUB_TSTR(flt)   LUXI_CLASS_STRING,(void*)&(flt)

#define FNPUB_FBOOL(flt)  LUXI_CLASS_BOOLEAN,(void*)flt
#define FNPUB_TBOOL(flt)  LUXI_CLASS_BOOLEAN,(void*)&(flt)

#define FNPUB_FPTR(flt)   LUXI_CLASS_POINTER,(void*)flt
#define FNPUB_TPTR(flt)   LUXI_CLASS_POINTER,(void*)&(flt)

#define FNPUB_FFLOAT(flt) LUXI_CLASS_FLOAT,lxfloat2void((float)flt)
#define FNPUB_TFLOAT(flt) LUXI_CLASS_FLOAT,(void*)&(flt)

#define FNPUB_TOVECTOR2(vector)   LUXI_CLASS_FLOAT,(void*)&(vector)[0],LUXI_CLASS_FLOAT,(void*)&(vector)[1]
#define FNPUB_FROMVECTOR2(vector) FNPUB_FFLOAT((vector)[0]),FNPUB_FFLOAT((vector)[1])
#define FNPUB_TOVECTOR3(vector)   LUXI_CLASS_FLOAT,(void*)&(vector)[0],LUXI_CLASS_FLOAT,(void*)&(vector)[1],LUXI_CLASS_FLOAT,(void*)&(vector)[2]
#define FNPUB_FROMVECTOR3(vector) FNPUB_FFLOAT((vector)[0]),FNPUB_FFLOAT((vector)[1]),FNPUB_FFLOAT((vector)[2])
#define FNPUB_TOVECTOR4(vector)   LUXI_CLASS_FLOAT,(void*)&(vector)[0],LUXI_CLASS_FLOAT,(void*)&(vector)[1],LUXI_CLASS_FLOAT,(void*)&(vector)[2],LUXI_CLASS_FLOAT,(void*)&(vector)[3]
#define FNPUB_FROMVECTOR4(vector) FNPUB_FFLOAT((vector)[0]),FNPUB_FFLOAT((vector)[1]),FNPUB_FFLOAT((vector)[2]),FNPUB_FFLOAT((vector)[3])

#define FNPUB_TOVECTOR3INT(vector)    LUXI_CLASS_INT,(void*)&(vector)[0],LUXI_CLASS_INT,(void*)&(vector)[1],LUXI_CLASS_INT,(void*)&(vector)[2]
#define FNPUB_FROMVECTOR3INT(vector)  FNPUB_FINT((vector)[0]),FNPUB_FINT((vector)[1]),FNPUB_FINT((vector)[2])

#define FNPUB_TOVECTOR4INT(vector)    LUXI_CLASS_INT,(void*)&(vector)[0],LUXI_CLASS_INT,(void*)&(vector)[1],LUXI_CLASS_INT,(void*)&(vector)[2],LUXI_CLASS_INT,(void*)&(vector)[3]
#define FNPUB_FROMVECTOR4INT(vector)  FNPUB_FINT((vector)[0]),FNPUB_FINT((vector)[1]),FNPUB_FINT((vector)[2]),FNPUB_FINT((vector)[3])

#define FNPUB_TOMATRIXPOS(vector)   LUXI_CLASS_FLOAT,(void*)&(vector)[12],LUXI_CLASS_FLOAT,(void*)&(vector)[13],LUXI_CLASS_FLOAT,(void*)&(vector)[14]
#define FNPUB_FROMMATRIXPOS(vector)   FNPUB_FFLOAT((vector)[12]),FNPUB_FFLOAT((vector)[13]),FNPUB_FFLOAT((vector)[14])


typedef void* PState;
typedef void* PFunc;

typedef struct PubFunction_s {
  void *upvalue;
  lxClassType classID;

  int (*function)(PState state, struct PubFunction_s* fn, int argCount);
  const char *description;
  const char *name;
  int inherits;
} PubFunction_t;


typedef struct PubBinString_s {
  int length;
  const char *str;
} PubBinString_t;

typedef struct PubArray_s{
  union{
    void* ptr;
    void**  ptrs;
    float*  floats;
    int*  ints;
    bool8*  bool8s;
    const char**  chars;
  }data;
  uint    length;
}PubArray_t;

typedef struct PubUData_s{
  uint32    magicvalue;
  lxObjId_t   id;
  lxObjId_t   *used;
#ifdef LUX_ARCH_X64
  bool16    isref;
  bool16    isptr;
#else
  bool32    isref;
#endif
}PubUData_t;


typedef int (*PubFunction) (PState state, struct PubFunction_s* fn,int argCount);

enum PubClassInfo_e{
  FPUB_CLASSTYPE_INTERFACE = 1<<0,  // applied to various objects
  FPUB_CLASSTYPE_BASE   = 1<<1,   // first class to passes functions to children
  FPUB_CLASSTYPE_STATIC = 1<<2,   // passes no functions
  FPUB_CLASSTYPE_GROUP  = 1<<3,   // has no functions
  FPUB_CLASSTYPE_CHILDREN = 1<<4,
  FPUB_CLASSTYPE_PASSES = 1<<5,
};

typedef struct PubClass_s {
  short returnsReferences;
  short returnsPointer;
  lxClassType parent;
  lxClassType interfaces[FNPUB_INTERFACES];
  int interfacecount;

  const char *description;
  const char *name;
  lxStrMapPTR functions;
  int   initialized;
  flags32 classinfo;

  int   numFunc;
  int   numInheritsFunc;

  void (*deInit)();
} PubClass_t;

//////////////////////////////////////////////////////////////////////////
// System

void FunctionPublish_init();
void FunctionPublish_deinit();
void FunctionPublish_deinitLua(struct lua_State *L);

//////////////////////////////////////////////////////////////////////////
// PubClass related

void FunctionPublish_initClass(lxClassType type, const char *name,
  const char *description, void (*deinit)(), int returnsreference);
void FunctionPublish_setIsPointer(lxClassType type, booln state);

void FunctionPublish_addFunction(lxClassType type, PubFunction fn,
  void *upvalue, const char *name, const char *description);
void FunctionPublish_setFunctionInherition (lxClassType type, const char *name, int on);

int FunctionPublish_isChildOf(lxClassType type, lxClassType parent);
void FunctionPublish_addInterface(lxClassType type, lxClassType interface);
int FunctionPublish_hasInterface(lxClassType type, lxClassType interface);
int FunctionPublish_isInterface(lxClassType type);

lxClassType FunctionPublish_getInterface(lxClassType type, int i);
int FunctionPublish_getInterfaceCount(lxClassType type);
void FunctionPublish_setParent(lxClassType type, lxClassType parent);
lxClassType FunctionPublish_getParent(lxClassType type);
int FunctionPublish_initialized(lxClassType type);
const char *FunctionPublish_className(lxClassType type);
const char *FunctionPublish_classDescription(lxClassType type);
lxStrMapPTR FunctionPublish_classFunctions(lxClassType type);
lxClassType FunctionPublish_getClassID (const char *str);



//////////////////////////////////////////////////////////////////////////
// Function Handling
  // this set of functions is to be used for handling lua functions that are passed
  // as arguments to pubfunctions. The lua functions are stored in a lua table
  // and lua functions are stored inside that table if a pfunc value is requested.
  // Remember to release PFunc values once they are obsolete!

  // sets the lua function in the registry to nil, ready to be GCed
  // (cannot be called anymore)
void FunctionPublish_releasePFunc(PState state, PFunc func);

  // calls a func. state may be NULL (will use main thread), func should be
  // valid. Returns -1 on error and sets errmsg to the returned error message
  // (if not NULL). -2 is returned if func is invalid. Otherwise it
  // returns the number of succesful set values on the return stack.
  // First the arguments (class_type+value) are passed, followed by a NULL
  // value, followed by the pointers to the return values (classtypes+pointers)
  // followed by a terminating NULL. Calling a "void" lua function would
  // look like this: FunctionPublish_call(NULL,func,NULL,NULL,NULL);
  // the luastack is immeadiatly freed after the lua function returned.
int FunctionPublish_callPFunc(PState state, PFunc func, const char **errmsg, ...);

//////////////////////////////////////////////////////////////////////////
// Stack Manipulation

  // assumes getArg already called on PubArray_t and pointers are either null
  // or filled with locations to copy from
void FunctionPublish_fillArray(PState state,int n, lxClassType type, PubArray_t *arr, void *datadst);

int FunctionPublish_getArgOffset(PState state,int offset,int n, ...);
int FunctionPublish_getArg(PState state,int n, ...);

int FunctionPublish_setRet(PState state,int n, ...);
int FunctionPublish_setNRet(PState state,int i, lxClassType type, void *data);

lxClassType FunctionPublish_getNArgType (PState state,int n);
int FunctionPublish_getNArg (PState state,int n, lxClassType toType, void **to);

// helpers
int FunctionPublish_returnBool    (PState state,booln boolean);
int FunctionPublish_returnPointer (PState state,void *ptr);
int FunctionPublish_returnInt     (PState state,int i);
int FunctionPublish_returnFloat   (PState state,float f);
int FunctionPublish_returnChar    (PState state,char chr);
int FunctionPublish_returnString  (PState state,const char *chr);
int FunctionPublish_returnFString (PState state,const char *fmt, ...);
int FunctionPublish_returnError   (PState state,char *chr);
int FunctionPublish_returnErrorf  (PState state,const char *str, ...);
int FunctionPublish_returnType    (PState state,lxClassType type, void *ptr);

int PubMatrix4x4_return (PState state,float *m);

#define FNPUB_CHECKOUT(state,argcnt,n,t,var) \
  if (argcnt<=n || FunctionPublish_getNArg(state,n,t,(void*)&var)!=1) \
    return FunctionPublish_returnErrorf(state,"arg%i: must be %s",n+1,\
      FunctionPublish_initialized(t)?FunctionPublish_className(t):#t);
#define FNPUB_CHECKOUTREF(state,argcnt,n,t,ref,var) \
  {if (argcnt<=n || FunctionPublish_getNArg(state,n,t,(void*)&ref)!=1) \
    return FunctionPublish_returnErrorf(state,"arg%i: must be %s",n+1,\
      FunctionPublish_initialized(t)?FunctionPublish_className(t):#t);\
  if (!Reference_get(ref,var)) \
    return FunctionPublish_returnErrorf(state,"arg%i: reference is invalid",n+1);}
////////////////////////////////////////////////////////////////////////////////
// external structs & functions

typedef struct StaticArray_s {
  lxClassType type;
  Reference ref;
  Reference refmounted;

  booln   mounted;
  int     count;

  union {
    float *floats;
    int   *ints;
    void  *data;
    byte  *bytes;
  };
} StaticArray_t;

typedef struct PScalarArray3D_s{
  Reference ref;
  Reference refmounted;
  booln   mounted;

  byte      *origdata;
  size_t      origsize;
  size_t      datasize; // origsize-dataoffset
  size_t      dataoffset;
  uint      sizecount;
  lxScalarArray3D_t sarr3d;
}PScalarArray3D_t;

  //mounteddata will not be freed, but the pointer must always be valid !
StaticArray_t* StaticArray_new (int count,lxClassType type, void *mounteddata, Reference mountedref);
int StaticArray_getNArg(PState pstate,int n,StaticArray_t **to);
int StaticArray_return(PState pstate,StaticArray_t *arr);


PScalarArray3D_t* PScalarArray3D_new(lxScalarType_t type, uint count, uint vector);
PScalarArray3D_t* PScalarArray3D_newMounted(lxScalarType_t type, uint count, uint vector, uint stride, void *data, Reference mountedref);
int PScalarArray3D_getNArg(PState pstate,int n,PScalarArray3D_t **to);
int PScalarArray3D_return(PState pstate,PScalarArray3D_t *arr);

#endif //_FNPUBLISH_H_
