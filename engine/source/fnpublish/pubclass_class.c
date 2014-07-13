// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/common.h"

typedef enum {
  R_ISCHILD,
  R_ISINTERFACE,
  R_INTERFACE,
  R_HASINTERFACE,
  R_CLASSID,
  R_DESCRIPTION,
  R_FUNCTION,
  R_CLASS,
  R_PARENT,
  R_NAME,
  R_ISINHERITED,
  R_ISLUXCORE,
  R_TOPOINTER,
} ClassAttr_t;

static int Pub_class (PState state,PubFunction_t *fn, int n)
{
  ClassAttr_t attr;
  lxClassType c1,c2;
  int index;
  const char *str;
  lxStrMapPTR fns;

  attr = (ClassAttr_t)fn->upvalue;

  if (attr==R_CLASS){
    if (n==0) return FunctionPublish_returnInt(state,LUXI_CLASS_COUNT);
    c1 = FunctionPublish_getNArgType(state,0);
    if (!FunctionPublish_initialized(c1)) {
      switch (c1) {
        case LUXI_CLASS_STRING:
          FNPUB_CHECKOUT(state,n,0,LUXI_CLASS_STRING,str);
          c1 = FunctionPublish_getClassID(str);
          if (c1==0) return 0;
          break;
        default:
          if (FunctionPublish_getNArg(state,0,LUXI_CLASS_INT,(void*)&c1)!=1)
            return 0;
          if (!FunctionPublish_initialized(c1)) return 0;
      }
    }
    return FunctionPublish_returnType(state,LUXI_CLASS_CLASS,(void*)c1);
  }
  else if (attr==R_TOPOINTER){
    void* data = NULL;
    if (n!=1 || !FunctionPublish_getNArg(state,0,LUXI_CLASS_ANY,&data)){
      return FunctionPublish_returnError(state,"passed handle is not a pointer");
    }
    return FunctionPublish_returnType(state,LUXI_CLASS_POINTER,data);
  }

  FNPUB_CHECKOUT(state,n,0,LUXI_CLASS_CLASS,c1);
  switch (attr)
  {
    case R_ISCHILD:
      FNPUB_CHECKOUT(state,n,1,LUXI_CLASS_CLASS,c2);
      return FunctionPublish_returnBool(state,FunctionPublish_isChildOf(c1,c2));
    case R_ISINTERFACE:
      return FunctionPublish_returnBool(state,FunctionPublish_isInterface(c1));
    case R_INTERFACE:
      if (n==1) return FunctionPublish_returnInt(state,FunctionPublish_getInterfaceCount(c1));
      FNPUB_CHECKOUT(state,n,1,LUXI_CLASS_INT,index);
      c1 = FunctionPublish_getInterface(c1,index);
      if (c1) return FunctionPublish_returnType(state,LUXI_CLASS_CLASS,(void*)c1);
      return 0;
    case R_HASINTERFACE:
      FNPUB_CHECKOUT(state,n,1,LUXI_CLASS_CLASS,c2);
      return FunctionPublish_returnBool(state,FunctionPublish_hasInterface(c1,c2));
    case R_CLASSID:
      return FunctionPublish_returnInt(state,c1);
    case R_DESCRIPTION:
      return FunctionPublish_returnString(state,FunctionPublish_classDescription(c1));
    case R_PARENT:
      c1 = FunctionPublish_getParent(c1);
      return c1?FunctionPublish_returnType(state,LUXI_CLASS_CLASS,(void*)c1):0;
    case R_FUNCTION:
      fns = FunctionPublish_classFunctions(c1);
      if (n==1) return FunctionPublish_returnInt(state,
        lxStrMap_getCount(FunctionPublish_classFunctions(c1)));
      if (n>=2) {
        PubFunction_t *fun;
        switch (FunctionPublish_getNArgType(state,1)) {
          case LUXI_CLASS_STRING:
            FNPUB_CHECKOUT(state,n,1,LUXI_CLASS_STRING,str);
            fun = (PubFunction_t*)lxStrMap_get(fns,str);
            break;
          default:
            FNPUB_CHECKOUT(state,n,1,LUXI_CLASS_INT,index);
            fun = (PubFunction_t*)lxStrMap_getNth(fns,index);
            break;
        }
        if (fun==NULL) return 0;
        return FunctionPublish_setRet(state,2,LUXI_CLASS_STRING,fun->name,
          LUXI_CLASS_STRING,fun->description);
      }
    break;
    case R_ISLUXCORE:
      return FunctionPublish_returnBool(state,c1 <= LUXI_CLASS_COUNT);
    case R_ISINHERITED:
    {
        PubFunction_t *fun;
            fns = FunctionPublish_classFunctions(c1);
            FNPUB_CHECKOUT(state,n,1,LUXI_CLASS_STRING,str);
            fun = (PubFunction_t*)lxStrMap_get(fns,str);
            return FunctionPublish_returnBool(state,fun->inherits);
    }
    break;
    case R_NAME:
      return FunctionPublish_returnString(state,FunctionPublish_className(c1));
    default: break;
  }
  return 0;
}

void PubClass_Class_init()
{
  FunctionPublish_initClass(LUXI_CLASS_CLASS,"fpubclass","The luxinia API is provided \
by a system that we call functionpublishing. The functionpublishing is standalone and \
not bound to a certain scripting language. It also provides basic descriptions \
(like this text) and implements an objectoriented system of inheritance, even though \
Luxinia is written in pure C (which isn't an objectoriented programminglanguage). \
Every object that is created in the Luxinia C core is wrapped in a fpubclass which \
provides information about the class functions, parent classes and interface classes.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_ISCHILD,"ischildof",
    "(boolean):(fpubclass child,fpubclass parent) - checks if the child is derived from parent");
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_ISINTERFACE,"isinterface",
    "(boolean):(fpubclass) - returns true if the given class is an interface class");
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_INTERFACE,"interface",
    "([int/fpubclass]):(fpubclass,[int index]) - returns the number of implemented \
interfaces. If an index is provided (>=0, <interfacecount) it returns the interface of the class");
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_HASINTERFACE,"hasinterface",
    "(boolean):(fpubclass c, fpubclass interface) - checks if the class c implements \
the given interface class");
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_CLASSID,"classid",
    "(int):(fpubclass c) - returns the internal ID number of the class");
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_ISLUXCORE,"isluxnative",
    "(boolean):(fpubclass c) - returns whether the class is part of the base luxinia.");
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_DESCRIPTION,"description",
    "([string]):(fpubclass) - returns description string for the class");
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_FUNCTION,"functioninfo",
    "(int/[string name,string description]):(fpubclass, [int n/string name]) - \
returns the number of functions within \
this class (not within it's parents or interfaces). If n is provided, it returns the name \
and the description of the n'th function, where n is >= 0 and < the number of functions. If \
a string is passed, the return is the same as for n if the functinname is known.");
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_NAME,"name",
    "(string):(fpubclass) - returns the name of the given class");
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_PARENT,"parent",
    "([fpubclass]):(fpubclass) - returns the parent of the class if it has a parent");
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_CLASS,"class",
    "([fpubclass/int]):(string/int/any handle) - returns the maximum number of classes \
that are registered in the functionpublishing (not all the numbers are registered \
classes) if no argument is given. If an int is applied, the class with this classid \
is returned. If a string is given, a class with that name is searched, and if an \
object is given, the class of the given object is returned. If a class was not \
found, nil is returned.");
    FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_ISINHERITED,"inherited",
        "(boolean):(fpubclass,string functionname) - returns true if the given function \
is inherited to child classes");
  FunctionPublish_addFunction(LUXI_CLASS_CLASS,Pub_class,(void*)R_TOPOINTER,"datapointer",
    "(pointer):(any handle) - converts a luxinia core-class userdata to a pointer."
    "This is useful for threading and custom dll extenders, BUT be aware that you loose all garbage collection tracking."
    "Because that is solely based on the lua userdata in the main lua thread.");


}
