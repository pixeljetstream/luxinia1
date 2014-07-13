// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/console.h"
#include "../resource/resmanager.h"

// Published here:
// LUXI_CLASS_GPUPROG


enum PubGPUCmds_e{
  PGPU_SOURCE,

  PGPU_NOGPU_SEPARATOR,

  PGPU_CGCOMPILERARGS,
  PGPU_FIND,
};

static int PubGpuProg_attribs (PState pstate,PubFunction_t *fn, int n)
{
  int index;
  GpuProg_t *gpuprog;
  char *path;
  char *entry;

  if ((int)fn->upvalue < PGPU_NOGPU_SEPARATOR){
    if (n<1 || FunctionPublish_getNArg(pstate,0,LUXI_CLASS_GPUPROG,(void*)&index)!=1)
      return FunctionPublish_returnError(pstate,"1 gpuprog required");

    gpuprog = ResData_getGpuProg(index);
  }

  switch((int)fn->upvalue){
  case PGPU_SOURCE:
    if (!gpuprog->source)
      GpuProg_getGL(gpuprog);
    return FunctionPublish_returnString(pstate,gpuprog->source);
    break;
  case PGPU_FIND:
    entry = NULL;
    if (n<1 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_STRING,&path,LUXI_CLASS_STRING,&entry)<1)
      return FunctionPublish_returnError(pstate,"1 string [1 string] required");
    index = ResData_search(RESOURCE_GPUPROG,path,entry);
    if (index==-1)
      return 0;

    return FunctionPublish_returnType(pstate,LUXI_CLASS_GPUPROG,(void*)index);
  }

  return 0;
}

void PubClass_GpuProg_init()
{
  FunctionPublish_initClass(LUXI_CLASS_GPUPROG,"gpuprog",
    "It contains code to be executed on the graphics card. \
Depending on whether your hardware is capable or not shaders will load gpuprogs automatically. \
There is als a set of standard gpuprograms in the base/gpuprogs folder of the application which are used \
internally to speed rendering up."
    ,NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_GPUPROG,LUXI_CLASS_RESOURCE);
  FunctionPublish_addFunction(LUXI_CLASS_GPUPROG,PubGpuProg_attribs,(void*)PGPU_FIND,"find",
    "([gpuprog]):(string file,string entryname) - finds a loaded gpuprog");
  FunctionPublish_addFunction(LUXI_CLASS_GPUPROG,PubGpuProg_attribs,(void*)PGPU_SOURCE,"source",
    "(string):(gpuprog) - returns the source");


}
