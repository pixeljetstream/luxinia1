// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "fnpublish.h"
#include "../common/memorymanager.h"


typedef enum StaticArrayUp_e {
// stored in main function
  VA_MOUNTED,
  VA_MOUNTSCALAR,
  VA_RESIZE,
  VA_COUNT,
  VA_SETGET,
  VA_COPYFROMSTRING,
  VA_OPCLAMP,
  VA_PTRS,
// INT & FLOAT functions
  VA_OPSET,
  VA_OPADD,
  VA_OPSUB,
  VA_OPMUL,
  VA_OPDIV,
  VA_OPMIN,
  VA_OPMAX,
// FLOAT functions
  VA_VEC3,
  VA_VEC4,
  VA_VEC3_ALL,
  VA_VEC4_ALL,
  VA_LERP,
  VA_LERP3,
  VA_LERP4,
  VA_OPLIT,
  VA_SPLINE3,
  VA_SPLINE4,
  VA_TONEXT3,
} StaticArrayUp_t;

static size_t StaticArray_sizeFromClass(lxClassType type)
{
  switch(type)
  {
  case LUXI_CLASS_FLOATARRAY: return sizeof(float);
  case LUXI_CLASS_INTARRAY: return sizeof(int);
  default:
    LUX_ASSERT(0); return 0;
  }
}


void StaticArray_free(void *rptr)
{
  StaticArray_t * self;

  self = (StaticArray_t*)rptr;
  if (!self->mounted){
    lxMemGenFreeAligned(self->floats,self->count*StaticArray_sizeFromClass(self->type));
  }
  Reference_releaseCheck(self->refmounted);

  gentypefree(self,StaticArray_t,1);
}
StaticArray_t* StaticArray_new (int count, lxClassType type, void *mounteddata, Reference mountedref)
{
  StaticArray_t *self;

  self = gentypezalloc(StaticArray_t,1);
  self->ref = Reference_new(type,self);
  self->count = count;
  self->type = type;
  // SSE needs other alloc
  if (mounteddata){
    self->data = mounteddata;
    self->refmounted = mountedref;
    if (mountedref){
      Reference_ref(mountedref);
    }
  }
  else{
    self->data = lxMemGenZallocAligned(count*StaticArray_sizeFromClass(self->type),16);
  }
  self->mounted = mounteddata ? LUX_TRUE : LUX_FALSE;

  Reference_makeVolatile(self->ref);
  return self;
}

int StaticArray_getNArg(PState pstate,int n,StaticArray_t **to)
{
  Reference ref;

  if (!FunctionPublish_getNArg(pstate,n,LUXI_CLASS_STATICARRAY,(void*)&ref)) return 0;
  return Reference_get(ref,*to);
}

int StaticArray_return(PState pstate,StaticArray_t *arr)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_STATICARRAY,REF2VOID(arr->ref));
}


// TODO a second function using SSE
static int PubStaticFloatArray_prop_FPU(PState pstate,PubFunction_t *fn, int n)
{
  StaticArray_t *self,*a,*b;
  Reference ref;
  int i;
  int index;
  float *pOut;
  float *pPos;
  float f;
  int valint;
  lxVector4 vec4;

  switch ((StaticArrayUp_t)fn->upvalue)
  {
  case VA_VEC3:
    {
      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);
      FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_INT,index);

      if (index<0 || index>=self->count/3)
        return FunctionPublish_returnErrorf(pstate,"index out of bounds (%i/%i)",
        index,self->count/3);

      pOut = &self->floats[3*index];

      if (n==2){
        return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(pOut));
      }
      else if (3!= FunctionPublish_getArgOffset(pstate,2,3,FNPUB_TOVECTOR3(pOut)))
        return FunctionPublish_returnError(pstate,"1 floatarray 1 index 3 floats required");
      return 0;
    }
  case VA_VEC3_ALL:
    {
      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);
      index = 0;
      valint = 0;
      i = 65536;
      if (3>FunctionPublish_getArgOffset(pstate,1,6,FNPUB_TOVECTOR3(vec4),LUXI_CLASS_INT,(void*)&index,LUXI_CLASS_INT,(void*)&i,LUXI_CLASS_INT,(void*)&valint))
        return FunctionPublish_returnError(pstate,"1 floatarray 3 floats [1 int] required");

      pOut = self->floats + index;
      pPos = LUX_MIN(self->floats + self->count,pOut+(i*(3+ valint)));

      for (;pOut < pPos; pOut+=(3+valint)){
        lxVector3Copy(pOut,vec4);
      }
      return 0;
    }
  case VA_VEC4_ALL:
    {
      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);
      index = 0;
      valint = 0;
      i = 65536;
      if (4>FunctionPublish_getArgOffset(pstate,1,7,FNPUB_TOVECTOR4(vec4),LUXI_CLASS_INT,(void*)&index,LUXI_CLASS_INT,(void*)&i,LUXI_CLASS_INT,(void*)&valint))
        return FunctionPublish_returnError(pstate,"1 floatarray 4 floats [1 int] required");

      pOut = self->floats + index;
      pPos = LUX_MIN(self->floats + self->count,pOut+(i*(4+ valint)));

      for (;pOut < pPos; pOut+=(4+valint)){
        lxVector4Copy(pOut,vec4);
      }
      return 0;
    }
  case VA_VEC4:
    {
      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);
      FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_INT,index);

      if (index<0 || index>=self->count/4)
        return FunctionPublish_returnErrorf(pstate,"index out of bounds (%i/%i)",
        index,self->count/4);

      pOut = &self->floats[4*index];

      if (n==2){
        return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(pOut));
      }
      else if (4!= FunctionPublish_getArgOffset(pstate,2,4,FNPUB_TOVECTOR4(pOut)))
        return FunctionPublish_returnError(pstate,"1 floatarray 1 index 4 floats required");
      return 0;
    }
  case VA_LERP:
    {
      float fracc;
      float step;
      float lrp;
      int outcount;
      int incount;

      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);
      FNPUB_CHECKOUTREF(pstate,n,1,LUXI_CLASS_FLOATARRAY,ref,a);
      incount = outcount = a->count;
      FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&incount);
      incount = LUX_MIN(outcount,incount);
      outcount = self->count;
      if (outcount <= incount || incount < 2)
        return FunctionPublish_returnError(pstate,"not enough floats given, need at least 2");


      step = 1.0f/(float)(incount-1);
      fracc = 1.0f/(float)(outcount-1);

      pOut = self->floats;
      pPos = a->floats;
      f = 0;
      for (i = 0; i < outcount; i++, pOut++,f+=fracc){
        if (f > step){
          f-=step;
          pPos++;
        }
        lrp = f/step;
        *pOut = LUX_LERP(lrp,*pPos,*(pPos+1));
      }
      return 0;
    }
  case VA_LERP3:
    {
      float fracc;
      float step;
      float lrp;
      int outcount;
      int incount;

      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);
      FNPUB_CHECKOUTREF(pstate,n,1,LUXI_CLASS_FLOATARRAY,ref,a);
      incount = outcount = a->count/3;
      FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&incount);
      incount = LUX_MIN(outcount,incount);
      outcount = self->count/3;

      if (outcount <= incount || incount < 2)
        return FunctionPublish_returnError(pstate,"not enough floats given, need at least 6");


      step = 1.0f/(float)(incount-1);
      fracc = 1.0f/(float)(outcount-1);

      pOut = self->floats;
      pPos = a->floats;
      f = 0;
      for (i = 0; i < outcount; i++, pOut+=3,f+=fracc){
        if (f > step){
          f-=step;
          pPos+=3;
        }
        lrp = f/step;
        lxVector3Lerp(pOut,lrp,pPos,pPos+3);
      }
      return 0;
    }
  case VA_LERP4:
    {
      float fracc;
      float step;
      float lrp;
      int outcount;
      int incount;

      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);
      FNPUB_CHECKOUTREF(pstate,n,1,LUXI_CLASS_FLOATARRAY,ref,a);
      incount = outcount = a->count/4;
      FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&incount);
      incount = LUX_MIN(outcount,incount);
      outcount = self->count/4;

      if (outcount <= incount || incount < 2)
        return FunctionPublish_returnError(pstate,"not enough floats given, need at least 8");


      step = 1.0f/(float)(incount-1);
      fracc = 1.0f/(float)(outcount-1);

      pOut = self->floats;
      pPos = a->floats;
      f = 0;
      for (i = 0; i < outcount; i++, pOut+=4,f+=fracc){
        if (f > step){
          f-=step;
          pPos+=4;
        }
        lrp = f/step;
        lxVector4Lerp(pOut,lrp,pPos,pPos+4);
      }
      return 0;
    }
  case VA_SPLINE4:
    {
      lxVector4 startvec,endvec;
      float fracc;
      float step;
      float lrp;
      float *endpos;
      float *startpos;
      int outcount;
      int incount;

      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);
      FNPUB_CHECKOUTREF(pstate,n,1,LUXI_CLASS_FLOATARRAY,ref,a);
      incount = outcount = a->count/4;
      FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&incount);
      incount = LUX_MIN(outcount,incount);

      outcount = self->count/4;
      if (outcount <= incount || incount < 2)
        return FunctionPublish_returnError(pstate,"not enough vectors given, need at least 2");

      pPos = &a->floats[4];
      lxVector4Sub(startvec,pPos,pPos-4);
      lxVector4Sub(startvec,pPos-4,startvec);

      pPos = &a->floats[4*(incount-1)];
      lxVector4Sub(endvec,pPos,pPos-4);
      lxVector4Add(endvec,pPos,endvec);

      step = 1.0f/(float)(incount-1);
      fracc = 1.0f/(float)(outcount-1);

      pOut = self->floats;
      pPos = a->floats;
      index = 0;
      f = 0;
      startpos = startvec;
      endpos = (index < incount-1) ? pPos+8 : endvec;
      for (i = 0; i < outcount-1; i++, pOut+=4,f+=fracc){
        if (f > step){
          f-=step;
          pPos+=3;
          index++;
          endpos = (index < incount-2) ? pPos+8 : endvec;
          startpos = pPos-4;
        }
        lrp = f/step;
        lxVector4CatmullRom(pOut,lrp,startpos,pPos,pPos+4,endpos);
      }
      lxVector4Copy(pOut,pPos+4);
      return 0;
    }
  case VA_SPLINE3:
    {
      lxVector3 startvec,endvec;
      float fracc;
      float step;
      float lrp;
      float *endpos;
      float *startpos;
      int outcount;
      int incount;

      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);
      FNPUB_CHECKOUTREF(pstate,n,1,LUXI_CLASS_FLOATARRAY,ref,a);
      incount = outcount = a->count/3;
      FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&incount);
      incount = LUX_MIN(outcount,incount);

      outcount = self->count/3;
      if (outcount <= incount || incount < 2)
        return FunctionPublish_returnError(pstate,"not enough vectors given, need at least 2");

      pPos = &a->floats[3];
      lxVector3Sub(startvec,pPos,pPos-3);
      lxVector3Sub(startvec,pPos-3,startvec);

      pPos = &a->floats[3*(incount-1)];
      lxVector3Sub(endvec,pPos,pPos-3);
      lxVector3Add(endvec,pPos,endvec);

      step = 1.0f/(float)(incount-1);
      fracc = 1.0f/(float)(outcount-1);

      pOut = self->floats;
      pPos = a->floats;
      index = 0;
      f = 0;
      startpos = startvec;
      endpos = (index < incount-1) ? pPos+6 : endvec;
      for (i = 0; i < outcount-1; i++, pOut+=3,f+=fracc){
        if (f > step){
          f-=step;
          pPos+=3;
          index++;
          endpos = (index < incount-2) ? pPos+6 : endvec;
          startpos = pPos-3;
        }
        lrp = f/step;
        lxVector3CatmullRom(pOut,lrp,startpos,pPos,pPos+3,endpos);
      }
      lxVector3Copy(pOut,pPos+3);
      return 0;
    }
  case VA_TONEXT3:
    {
      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);
      index = self->count/3;
      pOut = self->floats;
      for (i = 0; i < index-1; i++,pOut+=3)
      {
        lxVector3Sub(pOut,pOut+3,pOut);
      }
      lxVector3Set(pOut,0,0,0);
      return 0;
    }
  case VA_OPLIT:
    {
      float *pNormal;
      lxVector3 latt;
      lxVector3 lambi;
      lxVector3 lpos;
      lxVector3 ldiff;

      lxVector3 intens;
      lxVector3 ldir;
      lxVector3 dist;
      float dot;
      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);
      i = self->count/4;
      pOut = self->floats;
      FNPUB_CHECKOUTREF(pstate,n,1,LUXI_CLASS_FLOATARRAY,ref,self);
      if (i != self->count/3)
        return FunctionPublish_returnError(pstate,"position array does not match size");
      pPos = self->floats;
      FNPUB_CHECKOUTREF(pstate,n,2,LUXI_CLASS_FLOATARRAY,ref,self);
      if (i != self->count/3)
        return FunctionPublish_returnError(pstate,"normal array does not match size");
      pNormal = self->floats;
      if (n < 15 || 12>FunctionPublish_getArgOffset(pstate,3,12,FNPUB_TOVECTOR3(lpos),FNPUB_TOVECTOR3(ldiff),FNPUB_TOVECTOR3(lambi),FNPUB_TOVECTOR3(latt)))
        return FunctionPublish_returnError(pstate,"3 floatarrays 12 floats required.");

      dist[0] = 1.0f;
      for (i = 0; i < self->count/3; i++,pOut+=4,pPos+=3,pNormal+=3)
      { // compute per vertex lighting
        // pos 2 light
        lxVector3Sub(ldir,lpos,pPos);
        dist[1] = lxVector3Normalized(ldir);
        dist[2] = dist[1]*dist[1];
        // N.L
        dot = LUX_MAX(0,lxVector3Dot(ldir,pNormal));
        // lightintesities
        lxVector3ScaledAdd(intens,lambi,dot,ldiff);
        // attenuate
        dot = 1.0f/(lxVector3Dot(dist,latt));
        lxVector3Scale(pOut,intens,dot);
        pOut[3] = 1.0f;
      }
      return 0;
    }
  case VA_OPSET:
  case VA_OPSUB:
  case VA_OPMUL:
  case VA_OPADD:
  case VA_OPDIV:
  case VA_OPMIN:
  case VA_OPMAX:
    {

      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_FLOATARRAY,ref,self);


      if (n==2 && FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&f)==1) {

        switch ((StaticArrayUp_t)fn->upvalue) {
          case VA_OPSET: for (i=0;i<self->count;i++) self->floats[i] = f; break;
          case VA_OPSUB: for (i=0;i<self->count;i++) self->floats[i] -= f; break;
          case VA_OPDIV: for (i=0;i<self->count;i++) self->floats[i] /= f; break;
          case VA_OPMUL: for (i=0;i<self->count;i++) self->floats[i] *= f; break;
          case VA_OPADD: for (i=0;i<self->count;i++) self->floats[i] += f; break;
          case VA_OPMIN:
            for (i=0;i<self->count;i++) self->floats[i] = LUX_MIN(self->floats[i],f); break;
          case VA_OPMAX:
            for (i=0;i<self->count;i++) self->floats[i] = LUX_MAX(self->floats[i],f); break;
          default:
            return 0;
          }
        }
        return 0;
      }


      FNPUB_CHECKOUTREF(pstate,n,1,LUXI_CLASS_FLOATARRAY,ref,a);
      if (n>=3){
        FNPUB_CHECKOUTREF(pstate,n,2,LUXI_CLASS_FLOATARRAY,ref,b)
      }
      else {
        b = a;
        a = self;
      }

      if (self->count>a->count || self->count>b->count)
        return FunctionPublish_returnError(pstate,"the arrays must be larger then destination");

      switch ((StaticArrayUp_t)fn->upvalue) {
      case VA_OPSET:
        if (n==3) return FunctionPublish_returnError(pstate,"accepts only 2 arguments");
        for (i=0;i<self->count;i++) self->floats[i] = a->floats[i];
        break;
      case VA_OPSUB:
        for (i=0;i<self->count;i++) self->floats[i] = a->floats[i]-b->floats[i];
        break;
      case VA_OPMUL:
        for (i=0;i<self->count;i++) self->floats[i] = a->floats[i]*b->floats[i];
        break;
      case VA_OPADD:
        for (i=0;i<self->count;i++) self->floats[i] = a->floats[i]+b->floats[i];
        break;
      case VA_OPDIV:
        for (i=0;i<self->count;i++) self->floats[i] = a->floats[i]/b->floats[i];
        break;
      case VA_OPMIN:
        for (i=0;i<self->count;i++) self->floats[i] = LUX_MIN(a->floats[i],b->floats[i]);
        break;
      case VA_OPMAX:
        for (i=0;i<self->count;i++) self->floats[i] = LUX_MAX(a->floats[i],b->floats[i]);
        break;
      }

      return 0;
  }
  return 0;
}


static int PubStaticArray_new (PState pstate,PubFunction_t *fn, int n)
{
  int count;
  StaticArrayUp_t type = (StaticArrayUp_t)fn->upvalue;

  FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_INT,count);
  if (count<=0)
    return FunctionPublish_returnError(pstate,"Array size must be >0");

  return FunctionPublish_returnType(pstate,type,REF2VOID(StaticArray_new(count,type,NULL,NULL)->ref));
}
static int PubStaticArray_prop (PState pstate,PubFunction_t *fn, int n)
{
  StaticArray_t *self;
  Reference ref;
  int i;
  int index;
  float *pOut;
  int *pInt;
  float f;
  int valint;


  switch ((StaticArrayUp_t)fn->upvalue)
  {
  case VA_MOUNTED:
    FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_STATICARRAY,ref,self);
    return FunctionPublish_returnBool(pstate,self->mounted);
  case VA_PTRS:
    FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_STATICARRAY,ref,self);
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_POINTER,(void*)self->bytes,LUXI_CLASS_POINTER,(void*)(self->bytes+ (self->count * StaticArray_sizeFromClass(self->type))));
  case VA_MOUNTSCALAR:
    {
      int vectordim = 1;
      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_STATICARRAY,ref,self);
      if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&vectordim) && (vectordim < 1 || vectordim > 4))
        return FunctionPublish_returnError(pstate,"illegal vectorsize");

      if (self->mounted)
        return FunctionPublish_returnError(pstate,"cannot mount mounted staticarray");

      return PScalarArray3D_return(pstate,
        PScalarArray3D_newMounted(self->type == LUXI_CLASS_FLOATARRAY ? LUX_SCALAR_FLOAT32 : LUX_SCALAR_INT32,
            self->count/vectordim,vectordim,vectordim,self->data,self->ref));
    }
  case VA_COPYFROMSTRING:
  {
    PubBinString_t pbs;
    FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_STATICARRAY,ref,self);
    FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_BINSTRING,pbs);

    i = 0;
    FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&i);

    index = (self->count-i) * StaticArray_sizeFromClass(self->type);
    index = LUX_MIN(index,pbs.length);

    memcpy(&self->bytes[i*StaticArray_sizeFromClass(self->type)],pbs.str,index);

    return 0;
  }
  case VA_SETGET:
  {
    FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_STATICARRAY,ref,self);
    FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_INT,index);

    if (index<0 || index>=self->count)
      return FunctionPublish_returnErrorf(pstate,"Index out of bounds (%i/%i)",
        index,self->count);


    switch(self->type){
    case LUXI_CLASS_INTARRAY:
      if (n==3)         {
        FNPUB_CHECKOUT(pstate,n,2,LUXI_CLASS_INT,valint);
        self->ints[index] = valint;
        return 0;
      }
      return FunctionPublish_returnInt(pstate,self->ints[index]);
    case LUXI_CLASS_FLOATARRAY:
      if (n==3)         {
        FNPUB_CHECKOUT(pstate,n,2,LUXI_CLASS_FLOAT,f);
        self->floats[index] = f;
        return 0;
      }
      return FunctionPublish_returnFloat(pstate,self->floats[index]);
    }
  }
  case VA_COUNT:
  {
    FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_STATICARRAY,ref,self);
    return FunctionPublish_returnInt(pstate,self->count);
  }
  case VA_OPCLAMP:
  {
    lxVector2 clamp;
    int clampint[2];


    FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_STATICARRAY,ref,self);
    if (n < 3 || !FunctionPublish_getArgOffset(pstate,1,2,FNPUB_TOVECTOR2(clamp)))
      return FunctionPublish_returnError(pstate,"1 staticarray 2 floats required.");

    switch(self->type){
    case LUXI_CLASS_INTARRAY:
      LUX_ARRAY2COPY(clampint,clamp);
      pInt = self->ints;
      for (i=0; i < self->count; i++,pInt++){
        *pInt = LUX_CLAMP(*pInt,clampint[0],clampint[1]);
      }
      return 0;
    case LUXI_CLASS_FLOATARRAY:
      pOut = self->floats;
      for (i=0; i < self->count; i++,pOut++){
        *pOut = LUX_CLAMP(*pOut,clamp[0],clamp[1]);
      }
      return 0;
    default:
      LUX_ASSERT(0);
    }


    }
  case VA_RESIZE:
  {
    int *cp;
    if (n<2) return FunctionPublish_returnError(pstate,"Staticarray and integer required");
        FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_STATICARRAY,ref,self);
        FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_INT,i);
    if (self->mounted)
      return FunctionPublish_returnError(pstate,"mounted arrays cannot be resized");

    // TODO SSE needs other alloc
    index = i*StaticArray_sizeFromClass(self->type);
    valint = self->count*StaticArray_sizeFromClass(self->type);
        cp = lxMemGenZallocAligned(index,16);
    // copy old if it fits in
    if (valint < index)
      memcpy(cp,self->data,valint);
        lxMemGenFreeAligned(self->data,valint);
        self->count = i;
        self->data = cp;
  }
  break;
  default: return 0;
  }
  return 0;
}

static int PubStaticIntArray_prop (PState pstate,PubFunction_t *fn, int n)
{
  StaticArray_t *self,*a,*b;
  Reference ref;
  int i;
  int valint;

  switch((int)fn->upvalue){
  case VA_OPSET:
  case VA_OPSUB:
  case VA_OPMUL:
  case VA_OPADD:
  case VA_OPDIV:
  case VA_OPMIN:
  case VA_OPMAX:
    {

      FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_INTARRAY,ref,self);


      if (n==2 && FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&valint)==1) {
        switch ((StaticArrayUp_t)fn->upvalue) {
        case VA_OPSET: for (i=0;i<self->count;i++) self->ints[i] = valint; break;
        case VA_OPSUB: for (i=0;i<self->count;i++) self->ints[i] -= valint; break;
        case VA_OPDIV: for (i=0;i<self->count;i++) self->ints[i] /= valint; break;
        case VA_OPMUL: for (i=0;i<self->count;i++) self->ints[i] *= valint; break;
        case VA_OPADD: for (i=0;i<self->count;i++) self->ints[i] += valint; break;
        case VA_OPMIN:
          for (i=0;i<self->count;i++) self->ints[i] = LUX_MIN(self->ints[i],valint); break;
        case VA_OPMAX:
          for (i=0;i<self->count;i++) self->ints[i] = LUX_MAX(self->ints[i],valint); break;
        default:
          return 0;
        }
        return 0;
      }


      FNPUB_CHECKOUTREF(pstate,n,1,LUXI_CLASS_INTARRAY,ref,a);
      if (n>=3)
      {FNPUB_CHECKOUTREF(pstate,n,2,LUXI_CLASS_INTARRAY,ref,b)}
      else {
        b = a;
        a = self;
      }

      if (self->count>a->count || self->count>b->count)
        return FunctionPublish_returnError(pstate,"the arrays must be larger then destination");

      switch ((StaticArrayUp_t)fn->upvalue) {
      case VA_OPSET:
        if (n==3) return FunctionPublish_returnError(pstate,"accepts only 2 arguments");
        for (i=0;i<self->count;i++) self->ints[i] = a->ints[i];
        break;
      case VA_OPSUB:
        for (i=0;i<self->count;i++) self->ints[i] = a->ints[i]-b->ints[i];
        break;
      case VA_OPMUL:
        for (i=0;i<self->count;i++) self->ints[i] = a->ints[i]*b->ints[i];
        break;
      case VA_OPADD:
        for (i=0;i<self->count;i++) self->ints[i] = a->ints[i]+b->ints[i];
        break;
      case VA_OPDIV:
        for (i=0;i<self->count;i++) self->ints[i] = a->ints[i]/b->ints[i];
        break;
      case VA_OPMIN:
        for (i=0;i<self->count;i++) self->ints[i] = LUX_MIN(a->ints[i],b->ints[i]);
        break;
      case VA_OPMAX:
        for (i=0;i<self->count;i++) self->ints[i] = LUX_MAX(a->ints[i],b->ints[i]);
        break;
      }

      return 0;
    }
  }

  return 0;
}

void RStaticArray_free (Reference ref)
{
  StaticArray_free(Reference_value(ref));
}

void PubClass_StaticArray_init()
{
  FunctionPublish_initClass(LUXI_CLASS_STATICARRAY,"staticarray",
    "Staticarrays are created as plain C arrays within luxinia. \
They can be used for operations that require lot's of \
calculations or are used by other C functions.<br><br>'mounted' arrays do not have their own allocated data, but directly \
operate on their host data. Be aware that there is no mechanism to check whether host is still vaild, you will \
need to do this yourself.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_STATICARRAY,LUXI_CLASS_MATH);

  FunctionPublish_addFunction(LUXI_CLASS_STATICARRAY,PubStaticArray_prop,
    (void*)VA_SETGET,"index",
    "([value]):(staticarray,index,[float/int value]) - assigns index the value if \
given or simply returns the value at the index. Returns error in case of invalid indices.");
  FunctionPublish_addFunction(LUXI_CLASS_STATICARRAY,PubStaticArray_prop,
    (void*)VA_COUNT,"count","(int):(staticarray) - returns maximum number of elements");
  FunctionPublish_addFunction(LUXI_CLASS_STATICARRAY,PubStaticArray_prop,
    (void*)VA_OPCLAMP,"clamp","():(staticarray self, value min, value max) - self = (self with values clamped to min,max)");
  FunctionPublish_addFunction(LUXI_CLASS_STATICARRAY,PubStaticArray_prop,
    (void*)VA_MOUNTED,"mounted","(boolean):(staticarray) - returns whether array is mounted, i.e. points to external memory.");
  FunctionPublish_addFunction(LUXI_CLASS_STATICARRAY,PubStaticArray_prop,
    (void*)VA_RESIZE,"resize","():(staticarray self, int size) - Changes size of array. If new size is greater old size, old content is preserved.");
  FunctionPublish_addFunction(LUXI_CLASS_STATICARRAY,PubStaticArray_prop,
    (void*)VA_PTRS,"datapointer","(pointer start, int end):(staticarray self) - you can access the memory directly in other lua dlls. Be aware that you must make sure to not corrupt memory, make sure you are always smaller than the 'end' pointer.");
  FunctionPublish_addFunction(LUXI_CLASS_STATICARRAY,PubStaticArray_prop,
    (void*)VA_COPYFROMSTRING,"fromstring","():(staticarray self, binstring, [int offset]) - copies the data from a binary string. Copies as much as possible.");
  FunctionPublish_addFunction(LUXI_CLASS_STATICARRAY,PubStaticArray_prop,
    (void*)VA_MOUNTSCALAR,"mountscalararray","(scalararray):(staticarray,[int vectorsize]) - returns a mounted scalararray from current array.");


Reference_registerType(LUXI_CLASS_FLOATARRAY,"floatarray",RStaticArray_free,NULL);
FunctionPublish_initClass(LUXI_CLASS_FLOATARRAY,"floatarray",
    "Floatarray in Luxinia for array operations.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_FLOATARRAY,LUXI_CLASS_STATICARRAY);
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticArray_new,(void*)LUXI_CLASS_FLOATARRAY,"new",
    "(floatarray):(int count) - creates a new staticarray. Count must be >0.");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,(void*)VA_OPLIT,"v4lit",
    "():(floatarray outputintensities(4n), floatarray vertexpos(3n), floatarray vertexnormal(3n), float lightpos x,y,z ,float diffuse r,g,b ,float ambient r,g,b ,float attconst, float attlin, float attsq) - \
computes per vertex lighting intensities for n vertices into the given array. The given light position, attenuation and color values are used. Note that light position must be in same coordinate system as vertices (local).");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,(void*)VA_SPLINE3,"v3spline",
    "():(floatarray interpolated(3i), floatarray splinepos(3n),[int noverride]) - \
does spline interpolation based on the n splinepos vectors (eventimed), and interpolates using i steps.");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,(void*)VA_SPLINE4,"v4spline",
    "():(floatarray interpolated(4i), floatarray splinepos(4n),[int noverride]) - \
does spline interpolation based on the n splinepos vectors (eventimed), and interpolates using i steps.");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,(void*)VA_LERP,"lerp",
    "():(floatarray interpolated(i), floatarray splinepos(n),[int noverride]) - \
does linear interpolation based on the n floats (eventimed), and interpolates using i steps.");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,(void*)VA_LERP3,"v3lerp",
    "():(floatarray interpolated(3i), floatarray splinepos(3n),[int noverride]) - \
does linear interpolation based on the n vectors (eventimed), and interpolates using i steps.");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,(void*)VA_LERP4,"v4lerp",
    "():(floatarray interpolated(4i), floatarray splinepos(4n),[int noverride]) - \
does linear interpolation based on the n vectors (eventimed), and interpolates using i steps.");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,(void*)VA_TONEXT3,"v3tonext",
    "():(floatarray out(3n)) - for every vector we do (next-self)");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,
    (void*)VA_OPADD,"add","():(floatarray self,floatarray / value a, [floatarray b]) - self+=a or self = a + b");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,
    (void*)VA_OPSUB,"sub","():(floatarray self,floatarray / value a, [floatarray b]) - self-=a or self = a - b");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,
    (void*)VA_OPMUL,"mul","():(floatarray self,floatarray / value a, [floatarray b]) - self*=a or self = a * b");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,
    (void*)VA_OPDIV,"div","():(floatarray self,floatarray / value a, [floatarray b]) - self/=a or self = a / b");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,
    (void*)VA_OPMIN,"min","():(floatarray self,floatarray / value a, [floatarray b]) - self = min(self,a) or self = min(a,b)");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,
    (void*)VA_OPMAX,"max","():(floatarray self,floatarray / value a, [floatarray b]) - self = max(self,a) or self = max(a,b)");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,
    (void*)VA_OPSET,"set","():(floatarray self,floatarray / value a) - self = a (copy an array or set value to all)");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,(void*)VA_VEC3_ALL,"v3all",
    "():(floatarray,float x,y,z,[int startfloat, vectorcount, stride]) - returns or sets all values of the floatarray, optionally can set from which float to start and how many vectors. Vectorsize is 3+stride.");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,(void*)VA_VEC4_ALL,"v4all",
    "():(floatarray,float x,y,z,w,[int startfloat, vectorcount, stride]) - returns or sets all values of the floatarray, optionally can set from which float to start and how many vectors. Vectorsize is 4+stride.");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,(void*)VA_VEC3,"v3",
    "([float x,y,z]):(floatarray,int index, [float x,y,z]) - returns or sets ith vector3, make sure count is 3*maxindex.");
  FunctionPublish_addFunction(LUXI_CLASS_FLOATARRAY,PubStaticFloatArray_prop_FPU,(void*)VA_VEC4,"v4",
    "([float x,y,z,w]):(floatarray,int index, [float x,y,z,w]) - returns or sets ith vector4, make sure count is 4*maxindex.");

  Reference_registerType(LUXI_CLASS_INTARRAY,"intarray",RStaticArray_free,NULL);
  FunctionPublish_initClass(LUXI_CLASS_INTARRAY,"intarray",
    "Intarray in Luxinia for array operations.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_INTARRAY,LUXI_CLASS_STATICARRAY);
  FunctionPublish_addFunction(LUXI_CLASS_INTARRAY,PubStaticArray_new,(void*)LUXI_CLASS_INTARRAY,"new",
    "(intarray):(int count) - creates a new staticarray. Count must be >0.");
  FunctionPublish_addFunction(LUXI_CLASS_INTARRAY,PubStaticIntArray_prop,
    (void*)VA_OPADD,"add","():(intarray self,intarray / value a, [intarray b]) - self+=a or self = a + b");
  FunctionPublish_addFunction(LUXI_CLASS_INTARRAY,PubStaticIntArray_prop,
    (void*)VA_OPSUB,"sub","():(intarray self,intarray / value a, [intarray b]) - self-=a or self = a - b");
  FunctionPublish_addFunction(LUXI_CLASS_INTARRAY,PubStaticIntArray_prop,
    (void*)VA_OPMUL,"mul","():(intarray self,intarray / value a, [intarray b]) - self*=a or self = a * b");
  FunctionPublish_addFunction(LUXI_CLASS_INTARRAY,PubStaticIntArray_prop,
    (void*)VA_OPDIV,"div","():(intarray self,intarray / value a, [intarray b]) - self/=a or self = a / b");
  FunctionPublish_addFunction(LUXI_CLASS_INTARRAY,PubStaticIntArray_prop,
    (void*)VA_OPMIN,"min","():(intarray self,intarray / value a, [intarray b]) - self = min(self,a) or self = min(a,b)");
  FunctionPublish_addFunction(LUXI_CLASS_INTARRAY,PubStaticIntArray_prop,
    (void*)VA_OPMAX,"max","():(intarray self,intarray / value a, [intarray b]) - self = max(self,a) or self = max(a,b)");
  FunctionPublish_addFunction(LUXI_CLASS_INTARRAY,PubStaticIntArray_prop,
    (void*)VA_OPSET,"set","():(intarray self,intarray / value a) - self = a (copy an array or set value to all)");


}
