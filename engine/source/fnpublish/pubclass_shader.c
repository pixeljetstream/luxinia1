// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../resource/resmanager.h"
#include "../common/3dmath.h"
#include "../main/main.h"
// Published here:
// LUXI_CLASS_SHADER
// LUXI_CLASS_SHADERPARAMID
// LUXI_CLASS_CGPARAM

static int PubShader_load (PState pstate,PubFunction_t *fn, int n)
{
  char *path;
  char *cflags = NULL;
  int index;

  if (n<1 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_STRING,&path,LUXI_CLASS_STRING,&cflags)<1)
    return FunctionPublish_returnError(pstate,"1 string required");
  Main_startTask(LUX_TASK_RESOURCE);
  index = ResData_addShader(path,cflags);
  Main_endTask();
  if (index==-1)
    return 0;

  return FunctionPublish_setRet(pstate,1,LUXI_CLASS_SHADER,index);
}

static int PubShader_findparam(PState pstate,PubFunction_t *f, int n){
  char *name;
  int id;
  int pass;

  pass = 0;
  if (n<2 ||  FunctionPublish_getArg(pstate,3,LUXI_CLASS_SHADER,(void*)&id,LUXI_CLASS_STRING,&name,LUXI_CLASS_INT,(void*)&pass)<2)
    return FunctionPublish_returnError(pstate,"1 shader 1 string [1 int] required");

  pass = Shader_searchParam(ResData_getShader(id),name,pass);

  if (pass < 0){
    return 0;
  }
  else{
    SUBRESID_MAKE(pass,id);
    return FunctionPublish_returnType(pstate,LUXI_CLASS_SHADERPARAMID,(void*)pass);
  }
}

static int PubShader_param (PState pstate,PubFunction_t *f, int n)
{
  int index;
  int id;
  ShaderParam_t *sparam;
  Shader_t *shader;
  float *vec;

  int offset = 0;

  if (n<1 ||
    FunctionPublish_getArg(pstate,3,LUXI_CLASS_SHADER,(void*)&index,LUXI_CLASS_SHADERPARAMID,(void*)&id,LUXI_CLASS_INT,(void*)&offset)<2)
    return FunctionPublish_returnError(pstate,"1 shader 1 shaderparamid [1 int] required, optional 4 floats");

  shader = ResData_getShader(index);
  if (SUBRESID_GETRES(id) != index)
    return FunctionPublish_returnError(pstate,"illegal shaderparamid");

  SUBRESID_MAKEPURE(id);
  sparam  = shader->totalParams[id];

  if (sparam->type == SHADER_PARAM_ARRAY)
    vec = (n==3 || n==7) ? &sparam->vectorp[offset*4] : sparam->vectorp;
  else
    vec = sparam->vector;

  if (sparam)
  switch(n) {
  case 2:
  case 3:
    return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vec));
  case 6:
  case 7:
    if (FunctionPublish_getArgOffset(pstate,(n==7) ? 3 : 2,4,FNPUB_TOVECTOR4(vec))<4)
      return FunctionPublish_returnError(pstate,"4 numbers required");
  default:
    return 0;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_CGPARAM

enum PubCG_Cmds_e{
  PCG_NEW,
  PCG_GET,

  PCG_DELETE,
  PCG_VALUE,
  PCG_VALUEINTO,
  PCG_SIZES,
};

extern float *PubMatrix4x4_alloc();
static int PubCGparam_func (PState pstate,PubFunction_t *fn, int n)
{
  CGparameter cgparam = NULL;

  if ((int)fn->upvalue > PCG_GET && (n<1 || FunctionPublish_getArg(pstate,1,LUXI_CLASS_CGPARAM,(void*)&cgparam)<1))
    return FunctionPublish_returnError(pstate,"1 cgparam required");

  switch((int)fn->upvalue){
    case PCG_NEW:
      {
        char *name;
        char *nametype;
        CGtype cgtype;
        CGtype basetype;
        int cnt = 1;

        if (n < 2 || FunctionPublish_getArg(pstate,3,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_STRING,(void*)&nametype,LUXI_CLASS_INT,(void*)&cnt)<2)
          return FunctionPublish_returnError(pstate,"2 strings required");

        cgtype = cgGetType(nametype);
        basetype = cgGetTypeBase(cgtype);

        if (VIDCg_getConnector(name) || cgtype==CG_UNKNOWN_TYPE ||
          (basetype != CG_FLOAT && basetype != CG_INT && basetype != CG_BOOL))
          return FunctionPublish_returnError(pstate,"name already in use, or illegal cg base type (float, bool, int)");

        if (cgGetTypeClass(cgtype) == CG_PARAMETERCLASS_MATRIX && basetype != CG_FLOAT)
          return FunctionPublish_returnError(pstate,"only float matrix parameters allowed");

        cgparam = cnt > 1 ? cgCreateParameterArray(g_VID.cg.context,cgtype,cnt) : cgCreateParameter(g_VID.cg.context,cgtype);
        VIDCg_setConnector(name,cgparam);

        return FunctionPublish_returnType(pstate,LUXI_CLASS_CGPARAM,(void*)cgparam);
      }

      break;
    case PCG_DELETE:
      {
        int linked = cgGetNumConnectedToParameters(cgparam);
        if (linked == 0){
          VIDCg_setConnector(cgGetParameterName(cgparam),NULL);
          cgDestroyParameter(cgparam);
        }
        return FunctionPublish_returnInt(pstate,linked);
      }
      break;
    case PCG_GET:
      {
        char *name;
        if (n < 1 || !FunctionPublish_getArg(pstate,1,LUXI_CLASS_STRING,(void*)&name))
          return FunctionPublish_returnError(pstate,"1 string required");
        cgparam = VIDCg_getConnector(name);

        if (cgparam)
          return FunctionPublish_returnType(pstate,LUXI_CLASS_CGPARAM,(void*)cgparam);
        else
          return 0;
      }
      break;
    case PCG_VALUE:
      {
      CGtype cgtype = cgGetParameterType(cgparam);
      CGtype basetype = cgGetTypeBase(cgtype);
      CGtype classtype = cgGetTypeClass(cgtype);
      lxClassType ctype;

      Reference ref;
      float *matrix;
      StaticArray_t *starray;
      int read = 0;
      int offset = 0;
      int size;

      int into = (int)fn->upvalue == PCG_VALUEINTO;

      if (n==1 || into){
        int vector[4];


        switch(classtype){
        case CG_PARAMETERCLASS_SCALAR:
        case CG_PARAMETERCLASS_VECTOR:

          switch(basetype){
          case CG_FLOAT:
            ctype = LUXI_CLASS_FLOAT;
            read = cgGetParameterValuefc(cgparam,4,(float*)vector);
            break;
          case CG_BOOL:
            ctype = LUXI_CLASS_BOOLEAN;
            read = cgGetParameterValueic(cgparam,4,vector);
            break;
          case CG_INT:
            ctype = LUXI_CLASS_INT;
            read = cgGetParameterValueic(cgparam,4,vector);
            break;
          default:
            return 0;
          }
          return FunctionPublish_setRet(pstate,read,ctype,(void*)vector[0],ctype,(void*)vector[1],ctype,(void*)vector[2],ctype,(void*)vector[3]);
          break;
        case CG_PARAMETERCLASS_MATRIX:
          if (into){
            // get matrix first
            if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,matrix))
              return FunctionPublish_returnError(pstate,"1 matrix4x4 required");
          }
          else{
            // create matrix
            matrix = PubMatrix4x4_alloc();
            ref = Reference_new(LUXI_CLASS_MATRIX44,matrix);
            Reference_makeVolatile(ref);
          }

          cgGetMatrixParameterfc(cgparam,matrix);
          return FunctionPublish_returnType(pstate,LUXI_CLASS_MATRIX44,REF2VOID(ref));
          break;
        case CG_PARAMETERCLASS_ARRAY:
          ctype = basetype == CG_FLOAT ? LUXI_CLASS_FLOATARRAY : LUXI_CLASS_INTARRAY;

          size = cgGetParameterColumns(cgparam)*cgGetParameterRows(cgparam)*cgGetArrayTotalSize(cgparam);
          if (into){
            // get array first
            if (FunctionPublish_getArgOffset(pstate,1,3,ctype,(void*)&ref,LUXI_CLASS_INT,(void*)&offset)<1 || !Reference_get(ref,starray) ||
              offset+size > starray->count)
              return FunctionPublish_returnError(pstate,"1 staticarray [1 int] required (with enough space)");
          }
          else{
            // create array
            starray = StaticArray_new(size,ctype,NULL,NULL);
          }

          offset = LUX_MIN(offset,(starray->count-1));
          read = LUX_MIN(read,(starray->count-offset));

          if (ctype == LUXI_CLASS_FLOATARRAY)
            cgGetParameterValuefc(cgparam,read,starray->floats);
          else
            cgGetParameterValueic(cgparam,read,starray->ints);

          return FunctionPublish_returnType(pstate,ctype,REF2VOID(starray->ref));
        }
      }
      else {
        // set
        lxVector4 vector;

        switch(classtype){
        case CG_PARAMETERCLASS_SCALAR:
        case CG_PARAMETERCLASS_VECTOR:
          ctype = basetype == CG_FLOAT ? LUXI_CLASS_FLOAT : LUXI_CLASS_INT;
          read = FunctionPublish_getArgOffset(pstate,1,n-1,ctype,(void*)&vector[0],ctype,(void*)&vector[1],ctype,(void*)&vector[2],ctype,(void*)&vector[3]);
          if (read > 0){
            switch(basetype){
            case CG_FLOAT:
              cgSetParameterValuefc(cgparam,n-1,vector);
              return 0;
            case CG_BOOL:
            case CG_INT:
              cgSetParameterValueic(cgparam,n-1,(int*)vector);
              return 0;
            default:
              return 0;
            }
          }
          break;
        case CG_PARAMETERCLASS_MATRIX:
          if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,matrix))
            return FunctionPublish_returnError(pstate,"1 matrix4x4 required");
          cgSetMatrixParameterfc(cgparam,matrix);
          return 0;
        case CG_PARAMETERCLASS_ARRAY:
          ctype = basetype == CG_FLOAT ? LUXI_CLASS_FLOATARRAY : LUXI_CLASS_INTARRAY;
          size = cgGetParameterColumns(cgparam)*cgGetParameterRows(cgparam)*cgGetArrayTotalSize(cgparam);

          if (FunctionPublish_getArgOffset(pstate,1,3,ctype,(void*)&ref,LUXI_CLASS_INT,(void*)&offset) || !Reference_get(ref,starray)
            || offset+size > starray->count)
            return FunctionPublish_returnError(pstate,"1 staticarray [2 ints] required, with enough space");

          offset = LUX_MIN(offset,(starray->count-1));

          if (ctype == LUXI_CLASS_FLOATARRAY)
            cgSetParameterValuefc(cgparam,size,&starray->floats[offset]);
          else
            cgSetParameterValueic(cgparam,size,&starray->ints[offset]);
          return 0;
        default:
          return 0;
        }
      }
      }
    case PCG_SIZES:
      return FunctionPublish_setRet(pstate,3,LUXI_CLASS_INT,(void*)cgGetParameterColumns(cgparam),LUXI_CLASS_INT,(void*)cgGetParameterRows(cgparam),LUXI_CLASS_INT,(void*)cgGetArrayTotalSize(cgparam));
    default:
      break;
  }

  return 0;
}

void PubClass_Shader_init()
{
  FunctionPublish_initClass(LUXI_CLASS_SHADERPARAMID,"shaderparamid","A simple vector4 within shaders, that can control gpuprogram values",NULL,LUX_FALSE);
  FunctionPublish_initClass(LUXI_CLASS_SHADER,"shader","shader to setup blend effects between textures.<br> See the shdscript manual for more infos.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_SHADER,LUXI_CLASS_RESOURCE);
  FunctionPublish_addFunction(LUXI_CLASS_SHADER,PubShader_load,NULL,"load",
    "(shader shd):(string filename, [compilerargs]) - loads a shader. Optionally can pass compilerstrings, eg. \"-DSOMETHING;\".");
  FunctionPublish_addFunction(LUXI_CLASS_SHADER,PubShader_findparam,NULL,"getparamid",
    "([shaderparamid]):(shader,string name,[int pass]) - returns id. In case param is used in multiple passes define which. (default first used).");
  FunctionPublish_addFunction(LUXI_CLASS_SHADER,PubShader_param,NULL,"param",
    "(float x,y,z,w):(shader,shaderparamid,[arrayoffset],[float x,y,z,w]) - returns or sets a shaderparam value, does nothing when param wasnt found. Arrayoffset only used for array parameters");

  FunctionPublish_initClass(LUXI_CLASS_CGPARAM,"shadercgparamhost","A Cg Parameter that serves as host to all parameters with the same name. Use it to make it easier to control many parameters thru one with optimal efficiency. Directly creates a CGparam and connects the ones found in gpuprograms. Such parameters may not be controlled individually in a shader. Be aware that there is not much error checking done on parameter value get/set. If arrays passed are too small or length is set incorrect there will be no error warning.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_CGPARAM,LUXI_CLASS_RENDERINTERFACE);
  FunctionPublish_addFunction(LUXI_CLASS_CGPARAM,PubCGparam_func,(void*)PCG_NEW,"create",
    "([shadercgparamhost]):(string name, string cgtypename, [int size]) - creates a parameter with the given name (or returns nil if name is already in use). Typename can be found in the Cg documentary. For array types the size value is taken.");
  FunctionPublish_addFunction(LUXI_CLASS_CGPARAM,PubCGparam_func,(void*)PCG_DELETE,"destroy",
    "(int users):(string name) - tries to destroy the parameter, will return the number of still dependent parameters when parameter could not be destroyed, as its still in use.");
  FunctionPublish_addFunction(LUXI_CLASS_CGPARAM,PubCGparam_func,(void*)PCG_GET,"get",
    "([shadercgparamhost]):(string name) - gets parameter or returns nil if noone with the name is registered");
  FunctionPublish_addFunction(LUXI_CLASS_CGPARAM,PubCGparam_func,(void*)PCG_VALUE,"value",
    "([float x,y,z,w / matrix4x4 / staticarray]):(shadercgparamhost, [float x,y,z,w / matrix4x4 / staticarray], [int offset, length]) - sets/gets the value of a parameter, depending on type it will return/need different types. Arrays will alwas need corresponding staticarray (float/int), whilse matrices use matrix4x4 and vectors or scalars are directly pushed/popped on stack. When arrays are accessed you can fill them with an offset value. Counters for those are always single scalars. Return will always return the full array");
  FunctionPublish_addFunction(LUXI_CLASS_CGPARAM,PubCGparam_func,(void*)PCG_VALUEINTO,"valueinto",
    "(matrix4x4 / staticarray):(shadercgparamhost, matrix4x4 / staticarray, [int offset]) - similar to value, but performs a get into the given parameters. Will not work for single vectors.");
  FunctionPublish_addFunction(LUXI_CLASS_CGPARAM,PubCGparam_func,(void*)PCG_SIZES,"sizes",
    "(int columns, int rows, int totalarraysize):(shadercgparamhost) - returns various dimensions of parameter");
}
