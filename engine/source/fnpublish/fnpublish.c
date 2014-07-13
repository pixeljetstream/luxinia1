// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../main/main.h"
#include "../main/luacore.h"
#include "../common/common.h"
#include "../common/3dmath.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"


typedef struct StateInfo_s {
  lua_State *L;
  int args;
} StateInfo_t;

typedef struct FnPubSys_s{
  StateInfo_t   defaultstate;
  lua_State   *defstate;


  PubClass_t    classes[LUXI_CLASS_MAX];
  lxStrMapPTR   classbynames;
  lxStrDictPTR  dictionary;

  int       numFuncs;
  int       numClasses;
}FnPubSys_t;
static FnPubSys_t l_fpub;


#define FPUBSYS_INITIALIZED   0xA2C4E6F8

//////////////////////////////////////////////////////////////////////////
//


PubClass_t* FunctionPublish_getClass(lxClassType type)
{
  return &l_fpub.classes[type];
}
int FunctionPublish_initialized(lxClassType type)
{
  return (type >=0 && type<= LUXI_CLASS_COUNT && l_fpub.classes[type].initialized == FPUBSYS_INITIALIZED);
}

void FunctionPublish_setParent(lxClassType type, lxClassType parent)
{
  l_fpub.classes[type].parent = parent;
  l_fpub.classes[parent].classinfo |= FPUB_CLASSTYPE_CHILDREN;
}

void FunctionPublish_setIsPointer(lxClassType type, booln state)
{
  l_fpub.classes[type].returnsPointer = state | l_fpub.classes[type].returnsReferences;
}

LUX_INLINE booln FunctionPublish_isPointer(lxClassType type)
{
  return l_fpub.classes[type].returnsPointer;
}
LUX_INLINE booln FunctionPublish_isReference(lxClassType type)
{
  return l_fpub.classes[type].returnsReferences;
}


void FunctionPublish_addInterface(lxClassType type, lxClassType interface)
{
  LUX_ASSERT(FunctionPublish_initialized(type)&&FunctionPublish_initialized(interface));
  LUX_ASSERT(l_fpub.classes[type].interfacecount<FNPUB_INTERFACES);
  LUX_ASSERT(!(l_fpub.classes[type].classinfo & FPUB_CLASSTYPE_INTERFACE));
  l_fpub.classes[interface].classinfo |= FPUB_CLASSTYPE_INTERFACE;
  l_fpub.classes[type].interfaces[l_fpub.classes[type].interfacecount++] = interface;
}

int FunctionPublish_isInterface(lxClassType type) {
  return  (l_fpub.classes[type].classinfo & FPUB_CLASSTYPE_INTERFACE) != 0;
}

lxClassType FunctionPublish_getParent(lxClassType type) {
  if (!FunctionPublish_initialized(type)) return LUXI_CLASS_ERROR;
  return l_fpub.classes[type].parent;
}

const char *FunctionPublish_className(lxClassType type)
{
  if (!FunctionPublish_initialized(type)) return "uninitialized";
  return l_fpub.classes[type].name;
}

const char *FunctionPublish_classDescription(lxClassType type)
{
  if (!FunctionPublish_initialized(type)) return "uninitialized";
  return l_fpub.classes[type].description;
}

lxStrMapPTR FunctionPublish_classFunctions(lxClassType type)
{
  if (!FunctionPublish_initialized(type)) return NULL;
  return l_fpub.classes[type].functions;
}

flags32 FunctionPublish_updateClassInfo_recursive(lxClassType type)
{
  PubClass_t *pclass = &l_fpub.classes[type];
  flags32 classinfo = pclass->classinfo ;
  flags32 parent = 0;

  if (pclass->initialized != FPUBSYS_INITIALIZED)
    return 0;

  if (pclass->parent){
    parent = FunctionPublish_updateClassInfo_recursive(pclass->parent);
  }

  if (parent & FPUB_CLASSTYPE_PASSES){
    classinfo |= FPUB_CLASSTYPE_PASSES;
  }

  if (!pclass->numFunc){
    classinfo |= FPUB_CLASSTYPE_GROUP;
  }
  else{
    if (!pclass->numInheritsFunc){
      classinfo |= FPUB_CLASSTYPE_STATIC;
    }
    else{
      classinfo |= FPUB_CLASSTYPE_PASSES;
    }
  }

  if (!(parent & FPUB_CLASSTYPE_PASSES) && (classinfo & FPUB_CLASSTYPE_PASSES) &&
    (classinfo & FPUB_CLASSTYPE_CHILDREN))
  {
    classinfo |= FPUB_CLASSTYPE_BASE;
  }

  pclass->classinfo |= classinfo;

  return classinfo;
}

void FunctionPublish_initClass (lxClassType type, const char *name,
  const char *description, void (*deInit)(), int returnsReferences)
{
  PubClass_t *pclass = &l_fpub.classes[type];

  LUX_ASSERT(type < LUXI_CLASS_MAX);
  LUX_ASSERT(pclass->initialized != FPUBSYS_INITIALIZED);

  //LUX_PRINTF("  registering Class: %s\n",name);
  pclass->functions = lxStrMap_new(GLOBAL_ALLOCATOR,64,0,l_fpub.dictionary);
  pclass->description = description;
  pclass->name = name;
  pclass->initialized = FPUBSYS_INITIALIZED;
  pclass->deInit = deInit;
  pclass->interfacecount = 0;
  pclass->numFunc = 0;
  pclass->numInheritsFunc = 0;

  pclass->returnsReferences = returnsReferences;
  pclass->returnsPointer = returnsReferences;

  l_fpub.numClasses++;
}


static void FunctionPublish_freeFunction (void *data)
{
  gentypefree(data,PubFunction_t,1);
}

void FunctionPublish_deInitClass(lxClassType type)
{
  if (l_fpub.classes[type].initialized != FPUBSYS_INITIALIZED) return;
  if (l_fpub.classes[type].deInit) l_fpub.classes[type].deInit();

  lxStrMap_delete(l_fpub.classes[type].functions,FunctionPublish_freeFunction);
}

void FunctionPublish_addFunction(lxClassType type, PubFunction fn,
  void *upvalue, const char *name, const char *description)
{
  PubFunction_t *function;

  if (l_fpub.classes[type].initialized != FPUBSYS_INITIALIZED) {
    fprintf(stderr,"  You cannot add functions to an uninitalized class, %s\n",name);
    LUX_ASSERT(0);
    return;
  }
  if (lxStrMap_get(l_fpub.classes[type].functions,name)) {
    fprintf(stderr,"  You cannot overwrite functions, %s\n",name);
    LUX_ASSERT(0);
    return;
  }

  //LUX_PRINTF("    adding Function %s\n",name);

  function = gentypezalloc(PubFunction_t,1);
  function->name = name;
  function->classID = type;
  function->description = description;
  function->function = fn;
  function->upvalue = upvalue;
  function->inherits = 1;
  lxStrMap_set(l_fpub.classes[type].functions,name,function);

  l_fpub.classes[type].numFunc++;
  l_fpub.classes[type].numInheritsFunc++;

  l_fpub.numFuncs++;
}

void FunctionPublish_setFunctionInherition (lxClassType type, const char *name, int on)
{
    PubFunction_t *function;
  int bias;

    if (l_fpub.classes[type].initialized != FPUBSYS_INITIALIZED) {
    fprintf(stderr,"  You cannot add functions to an uninitalized class, %s\n",name);
    LUX_ASSERT(0);
    return;
  }
  if (!(function = (PubFunction_t*)lxStrMap_get(l_fpub.classes[type].functions,name))) {
    fprintf(stderr,"  Given function does not exist (yet?): %s\n",name);
    LUX_ASSERT(0);
    return;
  }

  bias = function->inherits - on;

  l_fpub.classes[type].numInheritsFunc += on ? -bias : bias;

  function->inherits = on;
}



lxClassType FunctionPublish_getInterface(lxClassType type, int i)
{
  if (i>l_fpub.classes[type].interfacecount || i<0) return LUXI_CLASS_ERROR;
  return l_fpub.classes[type].interfaces[i];
}

int FunctionPublish_getInterfaceCount(lxClassType type)
{
  return l_fpub.classes[type].interfacecount;
}

LUX_INLINE int FunctionPublish_isChildOf(lxClassType type, lxClassType parent) {
  while (type!=parent && type>0 && type<LUXI_CLASS_COUNT) {
    type = l_fpub.classes[type].parent;
  }
  return type == parent;
}

LUX_INLINE int FunctionPublish_hasInterface(lxClassType type, lxClassType interface)
{
  int i;
  while (type>0 && type<LUXI_CLASS_COUNT) {
    for (i=0;i<l_fpub.classes[type].interfacecount;i++)
      if (FunctionPublish_isChildOf(interface,l_fpub.classes[type].interfaces[i])) return 1;
    type = l_fpub.classes[type].parent;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// Lua Binding Internals


LUX_INLINE PubUData_t*  PubUData_safeGet(PubUData_t* udata)
{
  return (udata->magicvalue == FNPUB_MAGICVALUE) ? udata : NULL;
}

LUX_INLINE lxClassType  PubUData_safeType(PubUData_t* udata)
{
  return (udata->magicvalue == FNPUB_MAGICVALUE) ? (lxClassType)udata->id.type : LUXI_CLASS_ERROR;
}

LUX_INLINE void PubUData_init( PubUData_t* udata, lxClassType type, void * data )
{
  PubClass_t* pclass = &l_fpub.classes[type];

  udata->magicvalue = FNPUB_MAGICVALUE;
  if (pclass->returnsReferences){
    lxObjRef_t* ref = (lxObjRef_t*)data;
    lxObjRef_addUser(ref);
    udata->used = (lxObjId_t*)ref;
    udata->id = *lxObjRef_getId(ref);
    udata->isref = LUX_TRUE;
  }
  else{
    udata->used = &udata->id;
    udata->id.ptr = data;
    udata->id.type = type;
    udata->isref = LUX_FALSE;
#ifdef LUX_ARCH_X64
    udata->isptr = pclass->returnsPointer;
#endif
  }
}

static void printStack(lua_State *L) {
  int i;
  LUX_PRINTF("stack\n");
  for (i=1;i<=lua_gettop(L);i++) {
    lua_pushvalue(L,i);
    LUX_PRINTF(" %i: %s %s\n",i,lua_typename(L,lua_type(L,i)),lua_tostring(L,-1));
    lua_pop(L,1);
  }
}

static int fpub_luacallfunc (lua_State *L)
{
  PubFunction_t function;
  PubFunction_t *func;
  StateInfo_t state;
  int n,r;

  state.L = L;
  n = state.args = lua_gettop(L);

  l_fpub.defaultstate = state;

  if (lua_type(L,lua_upvalueindex(1))!=LUA_TLIGHTUSERDATA) {
    LUX_PRINTF("%i %i\n",lua_type(L,lua_upvalueindex(1)),LUA_TUSERDATA);
    //  return 0;
    lua_pushfstring(L,"Not an userdatavalue in (%s)",lua_typename(L,lua_type(L,lua_upvalueindex(1))));
    lua_error(L);
  }

  func = (PubFunction_t*)lua_touserdata(L,lua_upvalueindex(1));
  if (!func)
    return 0;

  function = *func;

  r = function.function((PState)&state,&function,n);
  if (lua_gettop(L)<n+r)
    luaL_checkstack(L,lua_gettop(L)-n-r,"stack too small");
  lua_settop(L,n+r);

  return r;
}

#define TAB_FUNCS   (((size_t)&l_fpub)+4)


#define TAB_VMODE   (((size_t)&l_fpub)+1)
#define TAB_UDATAS    (((size_t)&l_fpub)+2)
#define TAB_CLASSES   (((size_t)&l_fpub)+3)

#ifdef HEAVYDEBUG

#define checkstackstart(L) int __stacksize = lua_gettop(L)
#define checkstack(L,where,more) if (lua_gettop(L)-__stacksize!=more) LUX_PRINTF(\
  "==== Stacksizediff (%p)! ====== %s : before %i / after %i / wanted dif %i\n",\
  L,where,lua_gettop(L),__stacksize,more)


#else
#define checkstackstart   LUX_NOOP
#define fnl         LUX_NOOP
#define checkstack      LUX_NOOP
#endif

// leaves tab
#define GETTAB(state,which) { \
  lua_pushlightuserdata(state,(void*)which); \
  lua_rawget(state,LUA_REGISTRYINDEX);}

// leaves classtab
#define GETREFTYPE(state,id) \
  GETTAB(state,TAB_CLASSES); \
  lua_pushinteger(state,(int)id);\
  lua_rawget(state,-2);\
  LUX_DEBUGASSERT(!lua_isnil(state,-1));\
  lua_remove(state,-2);




static int fpub_luagc(lua_State* L)
{
  PubUData_t* udata = (PubUData_t*)lua_touserdata(L,-1);
  lua_State *lintstate = l_fpub.defstate;


  lua_pushlightuserdata(lintstate,(void*)TAB_UDATAS);
  lua_rawget(lintstate,LUA_REGISTRYINDEX);
  lua_pushinteger(lintstate,udata->id.type);
  lua_rawget(lintstate,-2);
  lua_pushlightuserdata(lintstate,udata->id.ptr);
  lua_pushnil(lintstate);
  lua_rawset(lintstate,-3);
  lua_pop(lintstate,2);

  if (udata->isref){
    lxObjRef_releaseUser((lxObjRefPTR)udata->used);
  }

  return 0;
}

static int fpub_luaindex(lua_State* L)
{
  PubUData_t* udata = (PubUData_t*)lua_touserdata(L,-2);
  // : udata, key
  lua_getfenv(L,-2);
  // : udata, key, local

  lua_pushvalue(L,-2);
  // : udata, key, local, key

  lua_rawget(L,-2);
  // : udata, key, local, val

  if (!lua_isnil(L,-1)) {
    return 1;
  } else {
    lua_pop(L,2);
  }
  // : udata, key

  lua_getmetatable(L,-2);
  // : udata, key , mt
  lua_pushvalue(L,-2);
  // : udata, key , mt, key
  lua_rawget(L,-2);
  // : udata, key , mt, val

  return 1;
}

static int fpub_luanewindex(lua_State* L)
{
  //PubUData_t* udata = (PubUData_t*)lua_touserdata(L,-3);
  // : udata, key, val
  lua_getfenv(L,-3);
  // : udata, key, val , tab

  LUX_ASSERT(lua_type(L,-1) == LUA_TTABLE);

  lua_pushvalue(L,-3);
  // : udata, key, val , tab, key
  lua_pushvalue(L,-3);
  // : udata, key, val , tab, key, val
  lua_rawset(L,-3);
  // : udata, key, val , tab

  return 0;

}
static int fpub_luatostring(lua_State* L)
{
  PubUData_t* ref = (PubUData_t*)lua_touserdata(L,-3);


  lua_pushstring(L,"[");
  lua_pushstring(L,l_fpub.classes[ref->id.type].name);
  lua_pushfstring(L," (classid: %d, value: %p)@%p]",ref->id.type,ref->id.ptr,(void*)ref);
  lua_concat(L,3);
  return 1;
}

static void* FP_initFuncsLua (void* fnData,const char *name,void *browseData)
{
  PubFunction_t * f = (PubFunction_t*) browseData;
  lxClassType origtype = (lxClassType)fnData;
  lua_State * L = l_fpub.defstate;

  checkstackstart(L);

  if (f->classID != origtype && !f->inherits)
    return fnData;

  // dont overwrite stuff
  lua_getfield(L,-1,f->name);
  if (!lua_isnil(L,-1)){
    lua_pop(L,1);
    return fnData;
  }
  lua_pop(L,1);
  lua_pushlightuserdata(L,f);
  lua_pushcclosure(L,(lua_CFunction)fpub_luacallfunc,1);
  lua_setfield(L,-2,f->name);

  checkstack(L,"FP_initFuncsLua",0);

  return fnData;
}

static lxClassType FP_iterateInitLua_recursive (lxClassType i,lxClassType orig)
{
  lxStrMapPTR map;
  int j;
  if (i==LUXI_CLASS_ERROR) return LUXI_CLASS_ERROR;
  map = FunctionPublish_classFunctions(i);
  lxStrMap_iterate(map, FP_initFuncsLua,(void*)orig);
  for (j=0;j<l_fpub.classes[i].interfacecount;j++)
    FP_iterateInitLua_recursive(l_fpub.classes[i].interfaces[j],orig);
  return FunctionPublish_getParent(i);
}

static void FunctionPublish_initClassLua(lxClassType t)
{
  PubClass_t* classdef = FunctionPublish_getClass(t);
  const char* classname = classdef->name;
  lua_State * L = l_fpub.defstate;

  fnl(FunctionPublish_initClassLua);
  checkstackstart(L);


  GETTAB(L,TAB_CLASSES);
  // : types
  lua_pushinteger (L,(int)t);
  // : types,t
  lua_newtable  (L);
  // : types,t, CLASSTAB
  {
    {
      lxClassType n = t;
      while (n){
        n = FP_iterateInitLua_recursive(n,t);
      }
    }
    lua_pushvalue(L,-1);
    // : types,t, CLASSTAB, CLASSTAB
    // : types,t, CLASSTAB, CLASSTAB
    lua_setglobal(L,classname);
    // : types,t, CLASSTAB

    lua_pushcclosure(L,fpub_luagc,0);
    lua_setfield(L,-2,"__gc");

    lua_pushcclosure(L,fpub_luaindex,0);
    lua_setfield(L,-2,"__index");

    lua_pushcclosure(L,fpub_luanewindex,0);
    lua_setfield(L,-2,"__newindex");

    lua_pushcclosure(L,fpub_luatostring,0);
    lua_setfield(L,-2,"__tostring");
  }
  lua_rawset(L,-3);
  lua_pop(L,1);


  checkstack(L,"FunctionPublish_initClassLua",0);
}

booln FunctionPublish_finalizeClassLua(lxClassType pubclass)
{
  if (!FunctionPublish_initialized(pubclass)) return LUX_FALSE;

  lxStrMap_set(l_fpub.classbynames,FunctionPublish_className(pubclass),(void*)pubclass);
  //FunctionPublish_updateClassInfo_recursive(pclass);

  FunctionPublish_initClassLua(pubclass);
  return LUX_TRUE;
}

static void FunctionPublish_initLua(lua_State *L)
{
  int i;

  l_fpub.defstate = lua_newthread (L);
  lua_setfield(L,LUA_REGISTRYINDEX,"luxregthread");
  L = l_fpub.defstate;

  // value weak table pushing
  lua_pushlightuserdata(L,(void*)TAB_VMODE);
  lua_newtable(L);
  lua_pushstring(L,"kv");
  lua_setfield(L,-2,"__mode");
  lua_rawset(L,LUA_REGISTRYINDEX);

  // create the list for userdata lookup
  lua_pushlightuserdata(L,(void*)TAB_UDATAS);
  lua_newtable(L);
  for (i=1;i<LUXI_CLASS_COUNT;i++) {
    lua_pushinteger(L,i);
    lua_newtable(L);
    lua_pushlightuserdata(L,(void*)TAB_VMODE);
    lua_rawget(L,LUA_REGISTRYINDEX);
    lua_setmetatable(L,-2);
    lua_rawset(L,-3);
  }
  lua_rawset(L,LUA_REGISTRYINDEX);

  lua_pushlightuserdata(L,(void*)TAB_CLASSES);
  lua_newtable(L);
  lua_rawset(L,LUA_REGISTRYINDEX);

  // let's build a lookuplist for the classnames now
  l_fpub.classbynames = lxStrMap_new(GLOBAL_ALLOCATOR,256,0,l_fpub.dictionary);
  for (i=0;i<LUXI_CLASS_COUNT;i++) {
    FunctionPublish_finalizeClassLua((lxClassType)i);
  }
}


void FunctionPublish_pushClassOnStackLua(lua_State *L, lxClassType type, void *data)
{
  PubUData_t* udata;

  checkstackstart(L);

  lua_pushlightuserdata(L,(void*)TAB_UDATAS);
  lua_rawget(L,LUA_REGISTRYINDEX);
  // : tab_ref_luts
  lua_pushinteger(L,type);
  lua_rawget(L,-2);
  // : tab_ref_luts,tab_ref_luts[type]
  lua_remove(L,-2);
  // : tab_ref_luts[type]
  lua_pushlightuserdata(L,data);
  // : tab_ref_luts[type], data
  lua_rawget(L,-2);
  // : tab_ref_luts[type], value
  if (!lua_isnil(L,-1)) {
    lua_remove(L,-2);

    checkstack(L,"pushClassOnStackLua",1);
    return;
  }

  lua_pop(L,1);
  // : tab_ref_luts[type]

  udata = lua_newuserdata(L,sizeof(PubUData_t));
  // : tab_ref_luts[type], udata

  PubUData_init(udata,type,data);

  lua_pushlightuserdata(L,data);
  // : tab_ref_luts[type],udata,data,
  lua_pushvalue(L,-2);
  // : tab_ref_luts[type],udata,data,udata
  lua_rawset(L,-4);
  // : tab_ref_luts[type],udata
  lua_remove(L,-2);
  // : udata
  GETREFTYPE(L,type);
  // : udata,classtab

  lua_setmetatable(L,-2);
  // : udata
  lua_newtable(L);
  // : udata,TABLE
  lua_setfenv(L,-2);
  // : udata


  checkstack(L,"FunctionPublish_pushClassOnStackLua",1);
}




//////////////////////////////////////////////////////////////////////////
// Function Handling

PFunc FunctionPublish_luafunc2pfunc(lua_State *L , int index)
{
  PFunc func;

  if (lua_type(L,index) != LUA_TFUNCTION) return 0;

  func = (PFunc)lua_topointer(L,index);

  lua_pushlightuserdata(L,(void*)TAB_FUNCS);
  lua_rawget(L,LUA_REGISTRYINDEX);

  if (lua_isnil(L,-1))
  {
    lua_pop(L,1);
    lua_newtable(L);
    lua_pushlightuserdata(L,(void*)TAB_FUNCS);
    lua_pushvalue(L,-2);
    lua_rawset(L,LUA_REGISTRYINDEX);
  }
  lua_pushlightuserdata(L,(void*)func);
  lua_pushvalue(L,index);
  lua_settable(L,-3);
  lua_pop(L,1);

  return func;
}

void FunctionPublish_releasePFunc(PState state, PFunc func)
{
  StateInfo_t *info = (StateInfo_t*)state;
  lua_State *L = info->L;

  lua_pushlightuserdata(L,(void*)TAB_FUNCS);
  lua_rawget(L,LUA_REGISTRYINDEX);
  if (lua_isnil(L,-1))
  {
    lua_pop(L,1);
    return;
  }
  lua_pushlightuserdata(L,(void*)func);
  lua_pushnil(L);
  lua_settable(L,-3);
  lua_pop(L,1);
}

int FunctionPublish_pushpfuncLua(lua_State *L, PFunc func)
{
  lua_pushlightuserdata(L,(void*)TAB_FUNCS);
  lua_rawget(L,LUA_REGISTRYINDEX);
  if (lua_isnil(L,-1))
  {
    lua_pop(L,1);
    return 0;
  }
  lua_pushlightuserdata(L,(void*)func);
  lua_gettable(L,-2);
  lua_remove(L,-2);
  if (lua_isnil(L,-1)) {
    lua_pop(L,1);
    return 0;
  }
  return 1;
}


static int traceback (lua_State *L) {
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}

void FunctionPublish_pushonStackLua(lua_State *L, lxClassType type, void *data);
int FunctionPublish_getStackDirect (lua_State *L, int from,lxClassType type, void *data);
int FunctionPublish_callPFunc(PState state, PFunc func, const char **errmsg, ...)
{
  lxClassType type;
  void **data;
  va_list ap;
  int narg = 0,nret,n;
  StateInfo_t *info, alt;
  lua_State *L;
  int top;

  if (state==NULL) {
    alt = l_fpub.defaultstate;
    state = &alt;
  }
  *errmsg = NULL;

  info = (StateInfo_t*)state;
  L = info->L;

  top = lua_gettop(info->L);
  if (!FunctionPublish_pushpfuncLua(L,func)) return -2;


  va_start(ap, errmsg);
  while(1) {
    type = va_arg(ap,lxClassType);
    if (!type) break;
    data = va_arg(ap,void**);
    lua_checkstack (L,1);
    FunctionPublish_pushonStackLua(L, type, (void*)data);
    narg++;
  }
  {
    int status;
    int base = lua_gettop(L) - narg;  /* function index */
    lua_pushcfunction(L, traceback);  /* push traceback function */
    lua_insert(L, base);  /* put it under chunk and args */
    status = lua_pcall(L, narg, LUA_MULTRET, base);
    lua_remove(L, base);  /* remove traceback function */
    /* force a complete garbage collection in case of errors */
    if (status != 0) {
      lua_gc(L, LUA_GCCOLLECT, 0);
      if (errmsg) *errmsg = lua_tostring(L,-1);
      lua_settop(L,top);
      va_end(ap);
      return -1;
    }
  }
  nret = lua_gettop(L)-top;
  n = 0;
  while (n<nret) {
    type = va_arg(ap,lxClassType);
    if (!type) break;
    data = va_arg(ap,void**);
    if (!FunctionPublish_getStackDirect (L, top+ ++n,type, (void*)data))
      break;
  }
  lua_settop(L,top);
  va_end(ap);

  return n;
}

//////////////////////////////////////////////////////////////////////////
// Query Stack



void FunctionPublish_fillArray(PState state,int n, lxClassType type, PubArray_t *arr, void *datadst)
{
  static size_t sizes[6] = {
    sizeof(void*),
    sizeof(char*),
    sizeof(float),
    sizeof(int),
    sizeof(bool8),
  };
  StateInfo_t *info = (StateInfo_t*)state;
  lua_State *L = info->L;
  int from = n+1;
  uint len = arr->length;
  LUX_ASSERT(len);

  LUX_DEBUGASSERT(lua_type(L,from) == LUA_TTABLE &&
          LUXI_CLASS_ARRAY_POINTER+4 == LUXI_CLASS_ARRAY_BOOLEAN &&
          type >= LUXI_CLASS_ARRAY_POINTER && type <= LUXI_CLASS_ARRAY_BOOLEAN);

  if (arr->data.ptr){
    memcpy(datadst,arr->data.ptr,arr->length * sizes[type-LUXI_CLASS_ARRAY_POINTER]);
    arr->data.ptr = datadst;
    return;
  }

  arr->data.ptr = datadst;

  switch (type)
  {
  case LUXI_CLASS_ARRAY_FLOAT:
    while(len--){
      lua_rawgeti(L,from,1+len);
      arr->data.floats[len] = (float)lua_tonumber(L,-1);
      lua_pop(L,1);
    }
    return;
  case LUXI_CLASS_ARRAY_INT:
    while(len--){
      lua_rawgeti(L,from,1+len);
      arr->data.ints[len] = (int)lua_tointeger(L,-1);
      lua_pop(L,1);
    }
    return;
  case LUXI_CLASS_ARRAY_STRING:
    while(len--){
      lua_rawgeti(L,from,1+len);
      arr->data.chars[len] = (char*)lua_tostring(L,-1);
      lua_pop(L,1);
    }
    return;
  case LUXI_CLASS_ARRAY_BOOLEAN:
    while(len--){
      lua_rawgeti(L,from,1+len);
      arr->data.bool8s[len] = (bool8)lua_toboolean(L,-1);
      lua_pop(L,1);
    }
    return;
  case LUXI_CLASS_ARRAY_POINTER:
    while(len--){
      lua_rawgeti(L,from,1+len);
      arr->data.ptrs[len] = (void*)lua_touserdata(L,-1);
      lua_pop(L,1);
    }
    return;
  }

}

static int FunctionPublish_getStackPubArray(lua_State *L,int from,lxClassType type, void *data)
{
  PubArray_t* arr = (PubArray_t*)data;
  int luatype;
  uint len;
  arr->length = 0;
  arr->data.ptr = NULL;

  // check first type
  lua_rawgeti(L,from,1);
  luatype = lua_type(L,-1);
  lua_pop(L,1);

  len = lua_objlen(L,from);
  if (!len) return 0;
  arr->length = len;

  switch (type)
  {
  case LUXI_CLASS_ARRAY_FLOAT:
    if (luatype != LUA_TNUMBER) return 0;
    break;
  case LUXI_CLASS_ARRAY_INT:
    if (luatype != LUA_TNUMBER) return 0;
    break;
  case LUXI_CLASS_ARRAY_STRING:
    if (luatype != LUA_TSTRING) return 0;
    break;
  case LUXI_CLASS_ARRAY_BOOLEAN:
    if (luatype != LUA_TBOOLEAN || type != LUA_TNUMBER) return 0;
    break;
  case LUXI_CLASS_ARRAY_POINTER:
    if (luatype != LUA_TLIGHTUSERDATA) return 0;
    break;
  default:
    return 0;
  }

  return 1;
}

static void FunctionPublish_pushStackPubArray(lua_State *L,lxClassType type, void* data)
{
  PubArray_t* arr = (PubArray_t*)data;
  uint i = 0;
  uint len = arr->length;

  LUX_DEBUGASSERT(type >= LUXI_CLASS_ARRAY_POINTER && type <= LUXI_CLASS_ARRAY_BOOLEAN);

  lua_newtable(L);

  switch (type)
  {
  case LUXI_CLASS_ARRAY_FLOAT:
    while (i++ < len){
      lua_pushnumber(L,(lua_Number)arr->data.floats[i]);
      lua_rawseti(L,-2,i);
    }
    break;
  case LUXI_CLASS_ARRAY_INT:
    while (i++ < len){
      lua_pushinteger(L,(lua_Integer)arr->data.floats[i]);
      lua_rawseti(L,-2,i);
    }
    break;
  case LUXI_CLASS_ARRAY_STRING:
    while (i++ < len){
      lua_pushstring(L,arr->data.chars[i]);
      lua_rawseti(L,-2,i);
    }
    break;
  case LUXI_CLASS_ARRAY_BOOLEAN:
    while (i++ < len){
      lua_pushboolean(L,(int)arr->data.bool8s[i]);
      lua_rawseti(L,-2,i);
    }
    break;
  case LUXI_CLASS_ARRAY_POINTER:
    while (i++ < len){
      lua_pushlightuserdata(L,arr->data.ptrs[i]);
      lua_rawseti(L,-2,i);
    }
    break;
  default:
    return;
  }
}

static int FunctionPublish_getStackDirect (lua_State *L, int from,lxClassType type, void *data)
{
  int val;
  int luatype = lua_type(L,from);

/*
#define LUA_TNIL    0
#define LUA_TBOOLEAN    1
#define LUA_TLIGHTUSERDATA  2
#define LUA_TNUMBER   3
#define LUA_TSTRING   4
#define LUA_TTABLE    5
#define LUA_TFUNCTION   6
#define LUA_TUSERDATA   7
#define LUA_TTHREAD   8
*/
  switch (luatype) {
    case LUA_TNIL:
      return 0;

    case LUA_TBOOLEAN:
      val =(int)lua_toboolean(L,from);
      switch (type) {
        case LUXI_CLASS_INT: *(int*)data = val; return 1;
        case LUXI_CLASS_FLOAT: *(float*)data = (float)val; return 1;
        case LUXI_CLASS_CHAR: *(char*)data = (char)val; return 1;
        case LUXI_CLASS_BOOLEAN: *(char*)data = (char)val; return 1;
        default: return 0;
      }


    case LUA_TLIGHTUSERDATA:
      if (type == LUXI_CLASS_POINTER){
        *(void**)data = lua_touserdata(L,from); return 1;
      }
      return 0;

    case LUA_TNUMBER:
      switch(type) {
        case LUXI_CLASS_INT: *(int*)data = lua_tointeger(L,from); return 1;
        case LUXI_CLASS_FLOAT: *(float*)data = (float)lua_tonumber(L,from); return 1;
        case LUXI_CLASS_CHAR: *(char*)data = (char)lua_tointeger(L,from); return 1;
        case LUXI_CLASS_BOOLEAN: *(char*)data = (char)(int)lua_toboolean(L,from); return 1;
        case LUXI_CLASS_DOUBLE: *(double*)data = (double)lua_tonumber(L,from); return 1;
        default: return 0;
      }

    case LUA_TSTRING: if (type!=LUXI_CLASS_STRING && type!=LUXI_CLASS_BINSTRING) return 0;
      if (type==LUXI_CLASS_STRING) {
        *(char**)data = (char*)lua_tostring(L,from);
      } else {
        PubBinString_t *bin = (PubBinString_t*)data;
        bin->str = lua_tolstring(L,from,&bin->length);
      }
      return 1;

    case LUA_TTABLE:
      if (type < LUXI_CLASS_ARRAY_POINTER || type > LUXI_CLASS_ARRAY_BOOLEAN) return 0;
      return FunctionPublish_getStackPubArray(L,from,type,data);

    case LUA_TFUNCTION:
      if(type!=LUXI_CLASS_FUNCTION) return 0;
      *(PFunc*)data = (PFunc) FunctionPublish_luafunc2pfunc(L, from);
      return 1;

    case LUA_TUSERDATA:
      // I assume it's a  reference
      {
        PubUData_t* udata = PubUData_safeGet((PubUData_t*)lua_touserdata(L,from));
        lxClassType reftype = (lxClassType)(udata->id.type);
        if (!udata) return 0;

        if (!(FunctionPublish_isChildOf(reftype,type) || FunctionPublish_hasInterface(reftype,type))){
          if (type!=LUXI_CLASS_ANY) return 0;
        }

        if (udata->isref){
          (*(Reference*)data) =  (Reference)udata->used;
        }
        else{
#ifdef LUX_ARCH_X64
          if (!udata->isptr)){
            *(uint32*)data = (uint32)udata->id.ptr;
          }
          else
#endif
          {
            (*(void**)data) = udata->id.ptr;
          }
        }
        return 1;
      }
    case LUA_TTHREAD:
      return 0;

  }
  return 0;
}


static void FunctionPublish_pushonStackLua(lua_State *L, lxClassType type, void *data)
{
  if (type >= LUXI_CLASS_CLASS){
    FunctionPublish_pushClassOnStackLua(L,type,data);
    return;
  }

  switch (type) {
    case LUXI_CLASS_FLOAT:
      lua_pushnumber(L,*(float*)&data);
      break;
    case LUXI_CLASS_INT:
      lua_pushinteger(L,((int)data));
      break;
    case LUXI_CLASS_BOOLEAN:
      lua_pushboolean(L,((int)data != 0));
      break;
    case LUXI_CLASS_STRING:
      if (data)
        lua_pushstring(L,(char*)data);
      else
        lua_pushnil(L);
      break;

    case LUXI_CLASS_BINSTRING:
      if (data)
      {
        PubBinString_t *bin = (PubBinString_t*) data;
        lua_pushlstring(L,bin->str,bin->length);
      }
      else
        lua_pushnil(L);
      break;
    case LUXI_CLASS_DOUBLE:
      lua_pushnumber(L,*(double*)&data);
      break;
    case LUXI_CLASS_CHAR:
      lua_pushfstring(L,"%c",*(char*)&data);
      break;
    case LUXI_CLASS_ARRAY_BOOLEAN:
    case LUXI_CLASS_ARRAY_STRING:
    case LUXI_CLASS_ARRAY_FLOAT:
    case LUXI_CLASS_ARRAY_INT:
    case LUXI_CLASS_ARRAY_POINTER:
      FunctionPublish_pushStackPubArray(L,type,data);
      break;
    case LUXI_CLASS_FUNCTION:
      if(!FunctionPublish_pushpfuncLua(L,*(PFunc*)data))
        lua_pushnil(L);
      break;
    case LUXI_CLASS_POINTER:
      lua_pushlightuserdata(L,data);
      break;
    case LUXI_CLASS_ANY:
      lua_pushstring(L,"invalid classtype in FunctionPublish_pushonStackLua");
      lua_error(L);
      break; // never reached anyway
    case LUXI_CLASS_ERROR:
      LUX_ASSERT(data!=NULL);
      lua_pushstring(L,(char*)data);
      lua_error(L);
      break;
    default:
      LUX_ASSERT(0);
      break;
  }
}

int FunctionPublish_getNArg (PState state,int n, lxClassType toType, void **to)
{

  StateInfo_t *info = (StateInfo_t*)state;
  lua_State *L = info->L;
  int top = lua_gettop(L);
  int from = n+1;

  if (top<from) return 0;

  return FunctionPublish_getStackDirect (L, from,toType, to);
}

lxClassType FunctionPublish_getNArgType (PState state,int n)
{
  StateInfo_t *info = (StateInfo_t*)state;
  lua_State *L = info->L;
  int top = lua_gettop(L);
  int from = n+1;

  if (top<from) return LUXI_CLASS_ERROR;

  switch (lua_type(L,from)) {
    case LUA_TNIL:    return LUXI_CLASS_ERROR;
    case LUA_TNUMBER: return LUXI_CLASS_FLOAT;
    case LUA_TBOOLEAN:  return LUXI_CLASS_BOOLEAN;
    case LUA_TSTRING: return LUXI_CLASS_STRING;
    case LUA_TTABLE:
      // check first type
      lua_rawgeti(L,from,1);
      from = lua_type(L,-1);
      lua_pop(L,1);

      switch (from)
      {
      case LUA_TBOOLEAN:return LUXI_CLASS_ARRAY_BOOLEAN;
      case LUA_TNUMBER: return LUXI_CLASS_FLOAT;
      case LUA_TSTRING: return LUXI_CLASS_ARRAY_STRING;
      default:
        return LUXI_CLASS_ERROR;
      }
    case LUA_TFUNCTION: return LUXI_CLASS_FUNCTION;
    case LUA_TUSERDATA:
      return PubUData_safeType((PubUData_t*)lua_touserdata(L,from));
    case LUA_TTHREAD: return LUXI_CLASS_ERROR;
    case LUA_TLIGHTUSERDATA: return LUXI_CLASS_POINTER;
  }
  return LUXI_CLASS_ERROR;
}

int FunctionPublish_getArgOffset(PState state,int offset,int argn, ...)
{
  va_list ap;
  int i;
  StateInfo_t *info = (StateInfo_t*)state;
  lua_State *L = info->L;
  int top = lua_gettop(L);
  int argmax;

  argn*=2;
  va_start(ap, argn);
  argn/=2;

  argmax = LUX_MIN(argn,top-offset);
  for (i=0;i<argmax;i++)
  {
    lxClassType type = va_arg(ap,lxClassType);
    void** data = va_arg(ap,void**);
    if (!FunctionPublish_getStackDirect(L,i+offset+1, type, data))
      return i;
  }
  va_end(ap);

  return i;
}

int FunctionPublish_getArg(PState state,int argn, ...)
{
  va_list ap;
  int i;
  StateInfo_t *info = (StateInfo_t*)state;
  lua_State *L = info->L;
  int top = lua_gettop(L);
  int argmax;

  argn*=2;
  va_start(ap, argn);
  argn/=2;

  argmax = LUX_MIN(argn,top);
  for (i=0;i<argmax;i++)
  {
    lxClassType type = va_arg(ap,lxClassType);
    void **data = va_arg(ap,void**);
    if (!FunctionPublish_getStackDirect(L,i+1, type, data))
      return i;
  }
  va_end(ap);

  return i;
}



//////////////////////////////////////////////////////////////////////////
// Return Stack

// this function writes a value on the stack with offset "cnt" (last arg)
// the stack during fpub calls is divided in two parts: from 1-argcnt
// and arg+1 to returncount
//
void FunctionPublish_setStack (PState state,int n, lxClassType type, void *data, int cnt)
{
  StateInfo_t *info = (StateInfo_t*)state;
  lua_State *L = info->L;
  int top = lua_gettop(L);
  int to = n+cnt+1;

  if (to>top) {
    lua_checkstack (L,to-top);
    lua_settop(L,to);
  }

  FunctionPublish_pushonStackLua(L, type, data);

  lua_insert(L,to);
}

int FunctionPublish_setNRet(PState state,int i, lxClassType type, void *data)
{
  FunctionPublish_setStack (state,i,type,data, ((StateInfo_t*)state)->args);
  return 1;
}

int FunctionPublish_setRet(PState state,int argn, ...)
{
  lxClassType type;
  void *data;
  va_list ap;
  int i;

  if (argn >= FNPUB_STACKSIZE) return 0;

  argn*=2;
  va_start(ap, argn);
  argn/=2;
  for(i = 0; i < argn; i++)
  {
    type = va_arg(ap, lxClassType);
    data = va_arg(ap, void*);
    FunctionPublish_setNRet(state,i,type,data);
  }
  va_end(ap);

  return argn;
}

int FunctionPublish_returnBool(PState state,booln boolean) {
  return FunctionPublish_setRet(state,1,LUXI_CLASS_BOOLEAN,boolean);
}

int FunctionPublish_returnPointer (PState state,void *ptr){
  return FunctionPublish_setRet(state,1,LUXI_CLASS_POINTER,ptr);
}

int FunctionPublish_returnInt(PState state,int i) {
  return FunctionPublish_setRet(state,1,LUXI_CLASS_INT,i);
}

int FunctionPublish_returnFloat(PState state,float f) {
  return FunctionPublish_setRet(state,1,FNPUB_FFLOAT(f));
}

int FunctionPublish_returnChar(PState state,char chr) {
  return FunctionPublish_setRet(state,1,LUXI_CLASS_CHAR,(int)chr);
}

int FunctionPublish_returnString(PState state,const char *chr) {
  return FunctionPublish_setRet(state,1,LUXI_CLASS_STRING,chr);
}

int FunctionPublish_returnFString(PState state,const char *fmt, ...){
  static char   text[1024];
  va_list   ap;
  int textlen;

  if (fmt == NULL)
    return 0;

  va_start(ap, fmt);
  textlen = vsprintf(text, fmt, ap);
  va_end(ap);

  return FunctionPublish_setRet(state,1,LUXI_CLASS_STRING,text);
}

int FunctionPublish_returnErrorf(PState state,const char *fmt, ...) {
  static char text[1024];
  va_list ap;
  int textlen;

  if (fmt == NULL)
    return 0;

  va_start(ap, fmt);
      textlen = vsprintf(text, fmt, ap);
  va_end(ap);

  return FunctionPublish_returnError(state,text);
}

int FunctionPublish_returnError(PState state,char *chr) {
  return FunctionPublish_setRet(state,1,LUXI_CLASS_ERROR,chr);
}

int FunctionPublish_returnType(PState state,lxClassType type, void *ptr) {
  return FunctionPublish_setRet(state,1,type,ptr);
}



//////////////////////////////////////////////////////////////////////////
// System

void FunctionPublish_deinitLua(lua_State *L) {
  lua_pushlightuserdata(L,(void*)TAB_FUNCS);
  lua_pushnil(L);
  lua_rawset(L,LUA_REGISTRYINDEX);

  lua_pushlightuserdata(L,(void*)TAB_UDATAS);
  lua_pushnil(L);
  lua_rawset(L,LUA_REGISTRYINDEX);

  lua_pushlightuserdata(L,(void*)TAB_CLASSES);
  lua_pushnil(L);
  lua_rawset(L,LUA_REGISTRYINDEX);

  lua_pushlightuserdata(L,(void*)TAB_VMODE);
  lua_pushnil(L);
  lua_rawset(L,LUA_REGISTRYINDEX);
}

void FunctionPublish_deinit() {
  int i;
  LUX_PRINTF("- Functionpublishing deinit\n");
  for (i=0;i<LUXI_CLASS_COUNT;i++)
    FunctionPublish_deInitClass(i);
  lxStrMap_delete(l_fpub.classbynames,NULL);
  lxStrDict_delete(l_fpub.dictionary);
}

lxClassType FunctionPublish_getClassID (const char *str)
{
  lxClassType i = (lxClassType) lxStrMap_get(l_fpub.classbynames,str);
  if (FunctionPublish_initialized(i)) return i;
  return 0;
}



extern void PubClass_Class_init();
extern void PubClass_StaticArray_init();
extern void PubClass_ScalarArray_init();

extern void PubClass_Render_init();
extern void PubClass_Console_init();
extern void PubClass_Input_init();
extern void PubClass_System_init();

extern void PubClass_ActorNode_init();
extern void PubClass_SceneTree_init();
extern void PubClass_List3D_init();
extern void PubClass_List2D_init();
extern void PubClass_L3DSet_init();

extern void PubClass_GpuProg_init();
extern void PubClass_Texture_init();
extern void PubClass_Sound_init();
extern void PubClass_Material_init();
extern void PubClass_Model_init();
extern void PubClass_Shader_init();
extern void PubClass_Particle_init();
extern void PubClass_Animation_init();

extern void PubClass_SimpleSpace_init();
extern void PubClass_Light_init();
extern void PubClass_Camera_init();
extern void PubClass_SkyBox_init();
extern void PubClass_Trail_init();
extern void PubClass_Bone_init();
extern void PubClass_Matrix16_init();
extern void PubClass_Collision_init();
extern void PubClass_Projector_init();
extern void PubClass_LuaCore_init();

extern void PubClass_Test_init();



static LuxiniaFPub_t  l_luxfpub = {
  NULL,
  FunctionPublish_pushonStackLua,
  FunctionPublish_getStackDirect,
  FunctionPublish_isPointer,
  FunctionPublish_isReference,
};

LUXIAPI void LuxiniaFPub_pushType(struct lua_State *L,lxClassType type,void *data)
{
  FunctionPublish_pushonStackLua(L,type,data);
}

LUXIAPI int LuxiniaFPub_getType(struct lua_State *L,int n, lxClassType type,void *data)
{
  return FunctionPublish_getStackDirect(L,n,type,data);
}

LUXIAPI unsigned int  LuxiniaFPub_isPointer(lxClassType type)
{
  return FunctionPublish_isPointer(type);
}
LUXIAPI unsigned int  LuxiniaFPub_isReference(lxClassType type)
{
  return FunctionPublish_isReference(type);
}

LUXIAPI LuxiniaFPub_t*  Luxinia_getFPub()
{
  return &l_luxfpub;
}

void FunctionPublish_init()
{
  l_luxfpub.luastate = LuaCore_getMainState();

  memset(&l_fpub,0,sizeof(FnPubSys_t));
  l_fpub.dictionary = lxStrDict_new(GLOBAL_ALLOCATOR,1024);

  bprintf("- Functionpublishing ...\n");
  PubClass_Class_init();
  PubClass_StaticArray_init();
  PubClass_ScalarArray_init();
  PubClass_Console_init();
  PubClass_Input_init();
  PubClass_Matrix16_init();
  PubClass_Collision_init();

  PubClass_List3D_init();
  PubClass_List2D_init();
  PubClass_L3DSet_init();
  // resources
  PubClass_GpuProg_init();
  PubClass_Sound_init();
  PubClass_Texture_init();
  PubClass_Bone_init();
  PubClass_Model_init();
  PubClass_Shader_init();
  PubClass_Particle_init();
  PubClass_Animation_init();

  // scene
  PubClass_SceneTree_init();
  PubClass_ActorNode_init();

  // render enviro
  PubClass_Light_init();
  PubClass_Camera_init();
  PubClass_SkyBox_init();
  PubClass_Trail_init();
  PubClass_Projector_init();

  PubClass_Render_init();
  PubClass_Material_init();

  //PubClass_LuaCore_init();

  PubClass_LuaCore_init();

  // system has to come after all resources
  PubClass_System_init();

  FunctionPublish_initLua(LuaCore_getMainState());

  bprintf("- Functionpublishing initialized\n");
}





