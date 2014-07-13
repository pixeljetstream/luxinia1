// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "fnpublish.h"
#include "../common/memorymanager.h"

//LUXI_CLASS_SCALARARRAY
//LUXI_CLASS_SCALARTYPE
//LUXI_CLASS_SCALAROP

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_SCALARARRAY

PScalarArray3D_t* PScalarArray3D_new(lxScalarType_t type, uint count, uint vectordim)
{
  PScalarArray3D_t* psarr = lxMemGenAlloc(sizeof(PScalarArray3D_t));
  psarr->ref = Reference_new(LUXI_CLASS_SCALARARRAY,psarr);
  psarr->origsize = count * vectordim * lxScalarType_getSize(type);
  psarr->origdata = lxMemGenZallocAligned(psarr->origsize,16);
  psarr->dataoffset = 0;
  psarr->datasize = psarr->origsize;
  psarr->refmounted = NULL;
  psarr->mounted = LUX_FALSE;
  psarr->sizecount = count;

  psarr->sarr3d.size[0] = count;
  psarr->sarr3d.size[1] = 1;
  psarr->sarr3d.size[2] = 1;
  LUX_ARRAY3SET(psarr->sarr3d.offset,0,0,0);

  psarr->sarr3d.sarr.type = type;
  psarr->sarr3d.sarr.count = count;
  psarr->sarr3d.sarr.stride = vectordim;
  psarr->sarr3d.sarr.vectordim = vectordim;

  psarr->sarr3d.sarr.data.tvoid = psarr->origdata;

  Reference_makeVolatile(psarr->ref);
  return psarr;
}

PScalarArray3D_t* PScalarArray3D_newMounted(lxScalarType_t type, uint count,
    uint vectordim, uint stride, void *data, Reference mountedref)
{
  PScalarArray3D_t* psarr = lxMemGenAlloc(sizeof(PScalarArray3D_t));

  LUX_ASSERT(data);

  psarr->ref = Reference_new(LUXI_CLASS_SCALARARRAY,psarr);
  psarr->origsize = count * lxScalarType_getSize(type) * stride;
  psarr->origdata = data;
  psarr->dataoffset = 0;
  psarr->datasize = psarr->origsize;
  psarr->refmounted = mountedref;
  psarr->mounted = LUX_TRUE;
  psarr->sizecount = count;

  if (mountedref){
    Reference_ref(mountedref);
  }

  psarr->sarr3d.size[0] = count;
  psarr->sarr3d.size[1] = 1;
  psarr->sarr3d.size[2] = 1;
  LUX_ARRAY3SET(psarr->sarr3d.offset,0,0,0);

  psarr->sarr3d.sarr.type = type;
  psarr->sarr3d.sarr.count = count;
  psarr->sarr3d.sarr.stride = stride;
  psarr->sarr3d.sarr.vectordim = vectordim;

  psarr->sarr3d.sarr.data.tvoid = psarr->origdata;

  Reference_makeVolatile(psarr->ref);
  return psarr;
}

void RPScalarArray3D_free(Reference ref)
{
  PScalarArray3D_t *self = (PScalarArray3D_t*)Reference_value(ref);

  Reference_releaseCheck(self->refmounted);

  if (!self->mounted)
    lxMemGenFreeAligned(self->origdata,self->origsize);
  lxMemGenFree(self,sizeof(PScalarArray3D_t));
}

LUX_INLINE int PScalarArray3D_getNArg(PState pstate,int n,PScalarArray3D_t **to)
{
  Reference ref;

  if (!FunctionPublish_getNArg(pstate,n,LUXI_CLASS_SCALARARRAY,(void*)&ref)) return 0;
  return Reference_get(ref,*to);
}
LUX_INLINE int PScalarArray3D_return(PState pstate,PScalarArray3D_t *arr)
{
  if (!arr) return 0;
  return FunctionPublish_returnType(pstate,LUXI_CLASS_SCALARARRAY,REF2VOID(arr->ref));
}

enum PubScalarProp_e{
  PSA_MOUNTED,
  PSA_TYPE,
  PSA_DATASIZE,
  PSA_DATASTART,
  PSA_VECTORSIZE,
  PSA_VECTORSTRIDE,
  PSA_SIZE,
  PSA_COUNT,
  PSA_OFFSET,

  PSA_MOUNT,
  PSA_MOUNTSTATIC,

  PSA_CONVERT,
  PSA_CONVERTRANGED,
  PSA_CURVE_LINEAR,
  PSA_CURVE_SPLINE,

  PSA_VECTOR,
  PSA_VECTORALL,
  PSA_VECTORSAMPLE,
  PSA_VECTORDIRECT,

  PSA_PTRS,
  PSA_COPYFROMSTRING,
  PSA_ASSTRING,
};

LUX_INLINE static int PScalarVector_get(PState pstate, int offset, lxScalarVector_t *pval, lxScalarType_t type, int vectordim)
{
  if (type == LUX_SCALAR_FLOAT32){
    return FunctionPublish_getArgOffset(pstate,offset,vectordim,FNPUB_TOVECTOR4(pval->tfloat));
  }
  else{
    int cache[4];
    int read = FunctionPublish_getArgOffset(pstate,offset,vectordim,FNPUB_TOVECTOR4INT(cache));
    lxScalarType_from32(pval, type, cache, read);

    return read;
  }
}

LUX_INLINE static int PScalarVector_return(PState pstate, int offset, lxScalarType_t type, int vectordim, void *pin)
{

  if (type == LUX_SCALAR_FLOAT32){
    float *flt = (float*)pin;
    return FunctionPublish_setRet(pstate,vectordim,FNPUB_FROMVECTOR4(flt));
  }
  else{
    lxScalarVector_t  cache;
    lxScalarType_to32(&cache, type, pin, vectordim);
    return FunctionPublish_setRet(pstate,vectordim,FNPUB_FROMVECTOR4INT(cache.tint32));;
  }
}

static int PubScalarArray_new(PState pstate,PubFunction_t *fn, int n)
{
  int vectorsize = 1;
  int count = 0;
  lxScalarType_t type;

  if (n < 2 || 2>FunctionPublish_getArg(pstate,3,LUXI_CLASS_SCALARTYPE,(void*)&type,
          LUXI_CLASS_INT,(void*)&count,LUXI_CLASS_INT,(void*)&vectorsize) ||
          count < 0 || vectorsize < 1 || vectorsize > 4)
  {
    return FunctionPublish_returnError(pstate,"1 scalartype 1 int (>0) [1 int 1..4] required.");
  }

  return PScalarArray3D_return(pstate,PScalarArray3D_new(type,count,vectorsize));
}

static int PubScalarArray_newPtr(PState pstate,PubFunction_t *fn, int n)
{
  int vectorsize = 1;
  int vectorstride = 1;
  int count = 0;
  lxScalarType_t type;
  void  *data = NULL;
  void  *dataend = NULL;

  if (n < 3 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_POINTER,(void*)&data)
    || !( FunctionPublish_getNArg(pstate,1,LUXI_CLASS_POINTER,(void*)&dataend) ||
        FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&count))
    || 1>FunctionPublish_getArgOffset(pstate,2,3,LUXI_CLASS_SCALARTYPE,(void*)&type,
        LUXI_CLASS_INT,(void*)&vectorsize,LUXI_CLASS_INT,(void*)&vectorstride) ||
    count < 0 || vectorsize < 1 || vectorsize > 4 || !data)
  {
    return FunctionPublish_returnError(pstate,"1 pointer [1 int (>0) / 1 pointer] 1 scalartype [1 int 1..4] [1 int] required.");
  }

  vectorstride = LUX_MAX(vectorstride,vectorsize);

  if (!count){
    count = ((size_t)dataend - (size_t)data)/(lxScalarType_getSize(type)*vectorstride);
  }

  return PScalarArray3D_return(pstate,PScalarArray3D_newMounted(type,count,vectorsize,vectorstride,data,NULL));
}

static int PubScalarArray_prop (PState pstate,PubFunction_t *fn, int n)
{
  PScalarArray3D_t *psa;
  PScalarArray3D_t *psb;

  lxScalarArray_t *sarr;
  lxScalarArray3D_t *sarr3d;


  if (n < 1 || !PScalarArray3D_getNArg(pstate,0,&psa))
  {
    return FunctionPublish_returnError(pstate,"1 scalararray required");
  }

  sarr3d = &psa->sarr3d;
  sarr   = &psa->sarr3d.sarr;

  switch((int)fn->upvalue)
  {
  case PSA_MOUNTED:
    return FunctionPublish_returnBool(pstate,psa->mounted);
  case PSA_TYPE:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_SCALARTYPE,(void*)sarr->type);
  case PSA_DATASIZE:
    {
      int val;
      booln keep = LUX_FALSE;

      if (n == 1) return FunctionPublish_returnInt(pstate,psa->origsize);
      else if (psa->mounted || 1>FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_INT,(void*)&val,FNPUB_TBOOL(keep)) ||
        val < 0 || sarr->stride * psa->sizecount * lxScalarType_getSize(sarr->type) > (uint)val)
      {
        return FunctionPublish_returnError(pstate,"1 int >=0 required and totalsize may not exceed memory. Illegal for mounted arrays.");
      }

      if (keep){
        psa->origdata = lxMemGenReallocAligned(psa->origdata,val,16,psa->origsize);
      }
      else{
        lxMemGenFreeAligned(psa->origdata,psa->origsize);
        psa->origdata = lxMemGenZallocAligned(val,16);
      }


      psa->origsize = val;
      psa->datasize = val;
      psa->dataoffset = 0;
      lxScalarArray3D_setData(sarr3d,psa->origdata);

      return FunctionPublish_returnBool(pstate,keep);
    }
    break;
  case PSA_DATASTART:
    {
      size_t neededspace = sarr->stride * psa->sizecount * lxScalarType_getSize(sarr->type);
      size_t effspace;
      int start;

      if (n==1) return FunctionPublish_returnInt(pstate,psa->dataoffset);
      else if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&start) ||
        start < 0 || (size_t)start < psa->origsize || (effspace=psa->origsize-(size_t)start) < neededspace)
      {
        return FunctionPublish_returnError(pstate,"1 int >=0 required, and totalsize may not exceed memory.");
      }

      psa->datasize = effspace;
      psa->dataoffset = start;
      lxScalarArray3D_setDataOffset(sarr3d,sarr3d->offset,&psa->origdata[psa->dataoffset]);

      return 0;
    }
    break;
  case PSA_VECTORSIZE:
    {
      int val;
      if (n == 1) return FunctionPublish_returnInt(pstate,sarr->vectordim);
      else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&val) ||
        val < 1 || val > 4 || (sarr->stride && LUX_MAX((uint)val,sarr->stride) * psa->sizecount * lxScalarType_getSize(sarr->type) > psa->datasize))
      {
        return FunctionPublish_returnError(pstate,"1 int [1-4] required and totalsize may not exceed memory");
      }

      sarr->stride = sarr->stride ? LUX_MAX((uint)val,sarr->stride) : 0;
      sarr->vectordim = val;
    }
    break;
  case PSA_VECTORSTRIDE:
    {
      int val;
      if (n == 1) return FunctionPublish_returnInt(pstate,sarr->stride);
      else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&val) ||
        val < 0 || (val && (uint)val < sarr->vectordim) || val * psa->sizecount * lxScalarType_getSize(sarr->type) > psa->datasize)
      {
        return FunctionPublish_returnError(pstate,"1 int 0 / >vectorsize required and totalsize may not exceed memory");
      }
      sarr->stride = val;
    }
    break;
  case PSA_SIZE:
    {
      int newsize[3];
      int newcount;

      if (n == 1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3INT(sarr3d->size));
      else if (n < 4 || 3>FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3INT(newsize)) ||
        ((newcount = newsize[0]*newsize[1]*newsize[2]) * (int)sarr->stride * (int)lxScalarType_getSize(sarr->type) >  (int)psa->datasize))
      {
        return FunctionPublish_returnError(pstate,"3 int [>0] required and size with current offset may not exceed memory");
      }

      LUX_ARRAY3COPY(sarr3d->size,newsize);
      psa->sizecount = newcount;
      lxScalarArray3D_setData(sarr3d,&psa->origdata[psa->dataoffset]);
    }
    break;
  case PSA_OFFSET:
    {
      int offset[3];
      if (n == 1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3INT(sarr3d->offset));
      if (n < 4 || 3>FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3INT(offset)) ||
        offset[0] < 0 || offset[1] < 0 || offset[2] < 0 ||
        lxScalarArray3D_setDataOffset(sarr3d,offset,&psa->origdata[psa->dataoffset]))
      {
        return FunctionPublish_returnError(pstate,"3 int [0,size] required");
      }
    }
    break;
  case PSA_COUNT:
    return FunctionPublish_returnInt(pstate,sarr->count);
    break;
  case PSA_MOUNT:
    return PScalarArray3D_return(pstate,PScalarArray3D_newMounted(sarr->type,sarr->count,sarr->vectordim,sarr->stride,sarr->data.tvoid,psa->ref));
    break;
  case PSA_MOUNTSTATIC:
    if ((sarr->type != LUX_SCALAR_FLOAT32 && sarr->type != LUX_SCALAR_INT32) || sarr->vectordim != sarr->stride)
      return FunctionPublish_returnError(pstate,"vectorsize and stride must match and only float32 and int32 allowed");

    return StaticArray_return(pstate,
        StaticArray_new(sarr->count*sarr->vectordim,
          sarr->type == LUX_SCALAR_FLOAT32 ? LUXI_CLASS_FLOATARRAY : LUXI_CLASS_INTARRAY,
          sarr->data.tvoid,psa->ref));
    break;
  case PSA_CONVERT:
    if (!PScalarArray3D_getNArg(pstate,1,&psb))
      return FunctionPublish_returnError(pstate,"2 scalararrays required");

    if (lxScalarArray_convert(&psa->sarr3d.sarr,&psb->sarr3d.sarr))
      return FunctionPublish_returnError(pstate,"conversion failed, vectorsize mismatch or other problem.");
    break;
  case PSA_CONVERTRANGED:
    {
      lxScalarVector_t avec;
      lxScalarVector_t bvec;
      if (2!=PScalarVector_get(pstate,1,&avec,psa->sarr3d.sarr.type,2) || !PScalarArray3D_getNArg(pstate,3,&psb) ||
        2!=PScalarVector_get(pstate,4,&bvec,psb->sarr3d.sarr.type,2) )
        return FunctionPublish_returnError(pstate,"1 scalararray 2 values 1 sacalararray 2 values required");

      if (lxScalarArray_convertRanged(&psa->sarr3d.sarr,&avec,&psb->sarr3d.sarr,&bvec))
        return FunctionPublish_returnError(pstate,"conversion failed, vectorsize mismatch or other problem.");
    }
    break;
  case PSA_CURVE_LINEAR:
    {
      booln closed = LUX_FALSE;
      if (!PScalarArray3D_getNArg(pstate,1,&psb))
        return FunctionPublish_returnError(pstate,"2 scalararrays required");

      FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&closed);

      if (lxScalarArray_curveLinear(sarr,&psb->sarr3d.sarr,closed))
        FunctionPublish_returnError(pstate,"scalararray curve interpolation failed, make sure vectorsizes are equal and out array has higher count than key array.");

      return PScalarArray3D_return(pstate,psa);
    }
    break;
  case PSA_CURVE_SPLINE:
    {
      booln closed = LUX_FALSE;
      if (!PScalarArray3D_getNArg(pstate,1,&psb))
        return FunctionPublish_returnError(pstate,"2 scalararrays required");

      FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&closed);

      if (lxScalarArray_curveSpline(sarr,&psb->sarr3d.sarr,closed))
        FunctionPublish_returnError(pstate,"scalararray curve interpolation failed, make sure vectorsizes are equal and out array has higher count than key array.");

      return PScalarArray3D_return(pstate,psa);
    }
    break;
  case PSA_VECTOR:
    {
      int idx;
      void *vec;
      if (n<2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&idx) || idx < 0 ||
        !(vec=lxScalarArray_getPtr(sarr,idx)))
      {
        return FunctionPublish_returnError(pstate,"1 scalararry 1 int within count required");
      }

      if (n < 3){
        return PScalarVector_return(pstate,0,sarr->type,sarr->vectordim,vec);
      }
      else if (!PScalarVector_get(pstate,2,vec,sarr->type,sarr->vectordim))
        return FunctionPublish_returnError(pstate,"1 scalararry 1-4 values required");
    }
    break;
  case PSA_VECTORALL:
    {
      lxScalarVector_t avec;
      lxScalarArray_t  asarr = lxScalarArray_newSingle(sarr->type,&avec,sarr->vectordim);
      int read;
      if (n < 2 || !(read=PScalarVector_get(pstate,1,&avec,sarr->type,sarr->vectordim)) ||
        (read != sarr->vectordim && read != 1) )
      {
        return FunctionPublish_returnError(pstate,"1 scalararry 1 or vectordim values required");
      }
      asarr.count = sarr->count;
      asarr.vectordim = read;

      lxScalarArray_Op1(sarr,LUX_SCALAR_OP1_COPY,&asarr);
    }
    break;
  case PSA_VECTORSAMPLE:
    {
      lxVector3 coords;
      lxVector4 outvals;
      booln notclamped[3] = {LUX_TRUE,LUX_TRUE,LUX_TRUE};

      if (n < 4 || 3>FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR3(coords),LUXI_CLASS_BOOLEAN,(void*)&notclamped[0]))
        return FunctionPublish_returnError(pstate,"1 scalararry 3 floats required");

      notclamped[1] = notclamped[2] = notclamped[0];
      if (lxScalarArray3D_sampleLinear(outvals,sarr3d,coords,notclamped))
        return FunctionPublish_returnError(pstate,"error on scalararry sampling size/count mismatch");
    }
    break;
  case PSA_VECTORDIRECT:
    {
      uint  pos[3];
      void *vec;
      if (n < 4 || 3>FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3INT(pos)) ||
        !(vec=lxScalarArray3D_getPtr(sarr3d,pos)))
      {
        return FunctionPublish_returnError(pstate,"3 int coords within size required");
      }

      if (n < 5){
        return PScalarVector_return(pstate,0,sarr->type,sarr->vectordim,vec);
      }
      else if (!PScalarVector_get(pstate,4,vec,sarr->type,sarr->vectordim))
        return FunctionPublish_returnError(pstate,"1 scalararry 1-4 values required");
    }
    break;

  case PSA_PTRS:
    return FunctionPublish_setRet(pstate,3,
      LUXI_CLASS_POINTER,psa->origdata,
      LUXI_CLASS_POINTER,psa->origdata + (psa->origsize),
      LUXI_CLASS_POINTER,sarr3d);
  case PSA_COPYFROMSTRING:
    {
      PubBinString_t pbs;
      int offset;
      FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_BINSTRING,pbs);

      if (sarr->vectordim != sarr->stride)
        return FunctionPublish_returnError(pstate,"scalararray must be compact, stride and vectorsize must match.");

      offset = 0;
      FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&offset);
      offset = LUX_MAX(offset,0);

      pbs.length = LUX_MIN(pbs.length, ((int)psa->origsize - offset));

      memcpy(&psa->origdata[offset],pbs.str,pbs.length);
    }
    break;
  case PSA_ASSTRING:
    {
      PubBinString_t pbs;
      pbs.str = sarr->data.tvoid;
      pbs.length = sarr->count*lxScalarType_getSize(sarr->type)*sarr->stride;
      return FunctionPublish_returnType(pstate,LUXI_CLASS_BINSTRING,(void*)&pbs);
    }
  }

  return 0;
}

enum PubScalarOp_e{
  PSO_OP0,
  PSO_OP1,
  PSO_OP2,
  PSO_OP3,

  PSO_OP0_REGION,
  PSO_OP1_REGION,
  PSO_OP2_REGION,
  PSO_OP3_REGION,
};

static int PubScalarArray_op (PState pstate,PubFunction_t *fn, int n){
  LUX_ALIGNSIMD_V(lxScalarVector_t    svec);
  lxScalarArray3D_t   sarr3d;

  PScalarArray3D_t  *psarrs[4];
  lxScalarArray3D_t   *salast;
  lxScalarArrayOp_t   sop;

  enum32 pso = (enum32)fn->upvalue;

  booln fail = LUX_FALSE;
  int region[3] = {0,0,0};
  int read = 0;
  int psoop = (pso % 4);
  int maxread = psoop+1;
  int offset = pso > PSO_OP3 ? 4 : 1;


  for (read = 0; read < maxread; read++){
    if(!PScalarArray3D_getNArg(pstate, read + (offset * (read!=0)),&psarrs[read]))
      break;
    salast = &psarrs[read]->sarr3d;
  }
  if (!FunctionPublish_getNArg(pstate,offset,LUXI_CLASS_SCALAROP,(void*)&sop)){
    return FunctionPublish_returnError(pstate,"scalarop required.");
  }
  if (offset > 1 && 3>FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3INT(region))){
    return FunctionPublish_returnError(pstate,"3 int for region required.");
  }
  if (!read || read < LUX_MAX(0, psoop)){
    return FunctionPublish_returnError(pstate,"too few scalararrays given.");
  }
  if (read != maxread){
    lxScalarArray3D_t *osarr3d = &psarrs[0]->sarr3d;
    lxScalarArray_t *osarr   = &psarrs[0]->sarr3d.sarr;

    if (1> (read=PScalarVector_get(pstate, offset+read, &svec, osarr3d->sarr.type, osarr3d->sarr.vectordim))){
      return FunctionPublish_returnError(pstate,"last arg must be vector values or scalararray");
    }

    salast = &sarr3d;
    lxScalarArray_initSingle(&sarr3d.sarr,osarr->type,&svec,read);
    sarr3d.sarr.count = osarr->count;
    LUX_ARRAY3COPY(sarr3d.size,osarr3d->size);
    LUX_ARRAY3SET(sarr3d.offset,0,0,0);
  }

  if (offset > 1){
    switch(psoop)
    {
    case 0:
      fail = lxScalarArray3D_Op0(&psarrs[0]->sarr3d,region,sop);
      break;
    case 1:
      fail = lxScalarArray3D_Op1(&psarrs[0]->sarr3d,region,sop,salast);
      break;
    case 2:
      fail = lxScalarArray3D_Op2(&psarrs[0]->sarr3d,region,sop,&psarrs[1]->sarr3d,salast);
      break;
    case 3:
      fail = lxScalarArray3D_Op3(&psarrs[0]->sarr3d,region,sop,&psarrs[1]->sarr3d,&psarrs[2]->sarr3d,salast);
      break;
    default:
      LUX_ASSERT(0);
      break;
    }
  }
  else{
    switch(pso)
    {
    case 0:
      fail = lxScalarArray_Op0(&psarrs[0]->sarr3d.sarr,sop);
      break;
    case 1:
      fail = lxScalarArray_Op1(&psarrs[0]->sarr3d.sarr,sop,&salast->sarr);
      break;
    case 2:
      fail = lxScalarArray_Op2(&psarrs[0]->sarr3d.sarr,sop,&psarrs[1]->sarr3d.sarr,&salast->sarr);
      break;
    case 3:
      fail = lxScalarArray_Op3(&psarrs[0]->sarr3d.sarr,sop,&psarrs[1]->sarr3d.sarr,&psarrs[2]->sarr3d.sarr,&salast->sarr);
      break;
    default:
      LUX_ASSERT(0);
      break;
    }
  }

  if (fail)
    return FunctionPublish_returnError(pstate,"scalararray operation failed, likely illegal sizes, vectorsizes or region + offset out of bounds.");

  return PScalarArray3D_return(pstate,psarrs[0]);
}

static int PubScalarArray_fop (PState pstate,PubFunction_t *fn, int n){
  PScalarArray3D_t *psa;
  PScalarArray3D_t *psb;
  Reference ref;
  float *matrix44 = NULL;
  lxFScalarArrayOp_t  fop = (lxFScalarArrayOp_t)fn->upvalue;

  if (n < 2 || !PScalarArray3D_getNArg(pstate,0,&psa) || !PScalarArray3D_getNArg(pstate,0,&psb) ||
    psa->sarr3d.sarr.type != LUX_SCALAR_FLOAT32 || psb->sarr3d.sarr.type != LUX_SCALAR_FLOAT32)
  {
    return FunctionPublish_returnError(pstate,"2 scalararrays required (float)");
  }
  if (fop <= LUX_FSCALAR_OP1_TRANSFORMFULL &&
    (n < 3 || !FunctionPublish_getNArg(pstate,2,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,matrix44)))
  {
    return FunctionPublish_returnError(pstate,"1 matrix4x4 required");
  }

  if (lxFScalarArray_op1(&psa->sarr3d.sarr,fop,&psb->sarr3d.sarr,matrix44))
    return FunctionPublish_returnError(pstate,"error in float scalararray operation (likely illegal vectorsize [2,4])");

  return PScalarArray3D_return(pstate,psa);
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_SCALARTYPE

static int PubScalarType_new (PState pstate,PubFunction_t *fn, int n){

  return FunctionPublish_returnType(pstate,LUXI_CLASS_SCALARTYPE,fn->upvalue);
}

enum PubSType_e{
  PST_SIZE,
  PST_TOBIN,
  PST_FROMBIN,
};

static int PubScalarType_attr (PState pstate,PubFunction_t *fn, int n){
  lxScalarType_t type;
  int count = 1;
  if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_SCALARTYPE,(void*)&type))
    return FunctionPublish_returnError(pstate,"1 scalartype required");



  switch((int)fn->upvalue){
  case PST_SIZE:
    {
      FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&count);
      return FunctionPublish_returnInt(pstate,lxScalarType_getSize(type)*count);
    }
  case PST_TOBIN:
    {
      double val;
      PubBinString_t pbs;

      if (type == LUX_SCALAR_FLOAT32){
        if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FLOAT,(void*)&val)){
          return FunctionPublish_returnError(pstate,"1 float required");
        }
      }
      else if (type == LUX_SCALAR_FLOAT64){
        if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_DOUBLE,(void*)&val)){
          return FunctionPublish_returnError(pstate,"1 double required");
        }
      }
      else{
        if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&val)){
          return FunctionPublish_returnError(pstate,"1 int required");
        }
      }

      pbs.str = (const char*)&val;
      pbs.length = lxScalarType_getSize(type);

      return FunctionPublish_returnType(pstate,LUXI_CLASS_BINSTRING,(void*)&pbs);
    }
  case PST_FROMBIN:
    {
      PubBinString_t pbs;
      lxClassType rettype;

      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BINSTRING,(void*)&pbs) || pbs.length < lxScalarType_getSize(type))
        return FunctionPublish_returnError(pstate,"1 proper binary string required");

      switch (type)
      {
      case LUX_SCALAR_FLOAT32: rettype = LUXI_CLASS_FLOAT; break;
      case LUX_SCALAR_FLOAT64: rettype = LUXI_CLASS_DOUBLE; break;
      default: rettype = LUXI_CLASS_INT; break;
      }

      return FunctionPublish_returnType(pstate,rettype,(void*)pbs.str);
    }
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_SCALAROP

static int PubScalarOp_new (PState pstate,PubFunction_t *fn, int n){

  return FunctionPublish_returnType(pstate,LUXI_CLASS_SCALAROP,fn->upvalue);
}


//////////////////////////////////////////////////////////////////////////
void PubClass_ScalarArray_init()
{
  FunctionPublish_initClass(LUXI_CLASS_SCALARTYPE,"scalartype",
    "Defines what type of scalar value is used in native C.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_SCALARTYPE,LUXI_CLASS_MATH);
  FunctionPublish_addFunction(LUXI_CLASS_SCALARTYPE,PubScalarType_new,(void*)LUX_SCALAR_FLOAT32,"float32",
    "([scalartype]):() - 32-bit signed float. No range limit. Saturate: [0,1]");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARTYPE,PubScalarType_new,(void*)LUX_SCALAR_INT8,"int8",
    "([scalartype]):() - 8-bit signed integer. Range: [-127,127]");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARTYPE,PubScalarType_new,(void*)LUX_SCALAR_INT16,"int16",
    "([scalartype]):() - 16-bit signed integer. Range: [-32767,32767]");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARTYPE,PubScalarType_new,(void*)LUX_SCALAR_INT32,"int32",
    "([scalartype]):() - 32-bit signed integer. Range: [-2147483647,2147483647] Saturate/Normalize: same as int16");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARTYPE,PubScalarType_new,(void*)LUX_SCALAR_UINT8,"uint8",
    "([scalartype]):() - 8-bit unsigned integer. Range: [0,255]");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARTYPE,PubScalarType_new,(void*)LUX_SCALAR_UINT16,"uint16",
    "([scalartype]):() - 16-bit unsigned integer. Range: [0,65535]");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARTYPE,PubScalarType_new,(void*)LUX_SCALAR_UINT32,"uint32",
    "([scalartype]):() - 32-bit unsigned integer. Range: [0,4294967295] Saturate/Normalize: same as uint16");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARTYPE,PubScalarType_new,(void*)LUX_SCALAR_FLOAT64,"float64",
    "([scalartype]):() - 64-bit signed float. WARNING: not useable in scalararray yet! No range limit. Saturate: [0,1]");

  FunctionPublish_addFunction(LUXI_CLASS_SCALARTYPE,PubScalarType_attr,(void*)PST_SIZE,"size",
    "(int bytes):(scalartype, [int count]) returns size in bytes.");

  FunctionPublish_addFunction(LUXI_CLASS_SCALARTYPE,PubScalarType_attr,(void*)PST_TOBIN,"value2bin",
    "(string):(scalartype, value) converts value into binary string.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARTYPE,PubScalarType_attr,(void*)PST_FROMBIN,"bin2value",
    "(value):(scalartype, string) converts binary string into value.");


/*
LUX_SCALAR_OP2_ADD,
LUX_SCALAR_OP2_SUB,
LUX_SCALAR_OP2_MUL,
LUX_SCALAR_OP2_DIV,
LUX_SCALAR_OP2_MIN,
LUX_SCALAR_OP2_MAX,
// saturated, performs clamp to natural range post-op
LUX_SCALAR_OP2_ADD_SAT,
LUX_SCALAR_OP2_SUB_SAT,
LUX_SCALAR_OP2_MUL_SAT,
LUX_SCALAR_OP2_DIV_SAT,

// outarray = op(arg0array, arg1array, arg2array )
// arg2 has weights, integers are interpreted as 0-1 float
LUX_SCALAR_OP3_LERP,
// 1-fracc is used
LUX_SCALAR_OP3_LERPINV,
// arg0 + arg1*arg2
LUX_SCALAR_OP3_MADD,
// saturated, performs clamp to natural range post-op
LUX_SCALAR_OP3_MADD_SAT,
*/
  FunctionPublish_initClass(LUXI_CLASS_SCALAROP,"scalarop",
    "Defines operations between scalararrays / values. Operations are performed componentwise per \
    vector. That means output array and input can be same. Scalartypes must match. Vectorsizes must match, except that last operand may have vectorsize of 1, as well. Non-saturated operations may cause 'wrap' \
    values, ie truncation happening post operation. For example a uint8 operation may yield 300 as result, \
    which means 44 (300 mod 256) is written. The saturated operations would clamp to 255 in that case.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_SCALAROP,LUXI_CLASS_MATH);
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP0_CLEAR,"clear0",
    "([scalarop]):() - out = 0 (zero)");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP1_COPY,"copy1",
    "([scalarop]):() - out = arg1.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP2_ADD,"add2",
    "([scalarop]):() - out = arg1 + arg2.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP2_SUB,"sub2",
    "([scalarop]):() - out = arg1 - arg2.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP2_MUL,"mul2",
    "([scalarop]):() - out = arg1 * arg2.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP2_DIV,"div2",
    "([scalarop]):() - out = arg1 / arg2.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP2_MIN,"min2",
    "([scalarop]):() - out = min(arg1,arg2).");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP2_MAX,"max2",
    "([scalarop]):() - out = max(arg1,arg2).");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP2_ADD_SAT,"add2sat",
    "([scalarop]):() - out = saturate(arg1 + arg2).");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP2_SUB_SAT,"sub2sat",
    "([scalarop]):() - out = saturate(arg1 - arg2).");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP2_MUL_SAT,"mul2sat",
    "([scalarop]):() - out = saturate(arg1 * arg2).");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP2_DIV_SAT,"div2sat",
    "([scalarop]):() - out = saturate(arg1 / arg2).");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP3_LERP,"lerp3",
    "([scalarop]):() - out = lerp(arg1 to arg2 via arg3).");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP3_LERPINV,"lerpinv3",
    "([scalarop]):() - out = lerp(arg1 to arg2 via 1-arg3).");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP3_MADD,"madd3",
    "([scalarop]):() - out = arg1 + (arg2 * arg3).");
  FunctionPublish_addFunction(LUXI_CLASS_SCALAROP,PubScalarOp_new,(void*)LUX_SCALAR_OP3_MADD_SAT,"madd3sat",
    "([scalarop]):() - out = saturate(arg1 + (arg2 * arg3)).");

  FunctionPublish_initClass(LUXI_CLASS_SCALARARRAY,"scalararray",
    "Scalarrays are created as plain C arrays within luxinia. \
    They can be used for operations that require lot's of \
    calculations or are used by other native C functions. They can be 1D,2D or 3D similar to textures and contain 1-4 component vectors. \
    <br><br>Normally the arrays have a memorychunk and you can set the 'region' of operation within this chunk via offset, size and stride. \
    <br><br>'Mounted' arrays, however do not have their own allocated data, but directly \
    operate on their host data. Be aware that there is no mechanism to check whether internal host is still valid, you will \
    need to do this yourself.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_SCALARARRAY,LUXI_CLASS_MATH);
  Reference_registerType(LUXI_CLASS_SCALARARRAY,"scalararray",RPScalarArray3D_free,NULL);

  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_new,(void*)NULL,"new",
    "([scalararray]):(scalartype, int count, [vectorsize]) - returns a new compact scalar array. vectorsize defaults to 1 and stride equals vectorsize.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_newPtr,(void*)NULL,"newfrompointer",
    "([scalararray]):(pointer start, [int count / pointer end], scalartype, [vectorsize], [vecotrstride]) - returns a new mounted scalar array. vectorsize defaults to 1 and stride equals vectorsize. You must make sure the pointer is and stays valid. If endpointer is given count is derived from the remaining attributes.!");

  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_DATASIZE,"datasize",
    "([int bytes]/[boolean kept]):(scalararray, [int bytes],[boolean keep]) - gets or sets maximum number of bytes. If keep is true (default false) then old data is preserved (can be slower). Decreasing can result into error if current size and stride settings are beyond new datasize. Offset and datashift will be reseted.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_DATASTART,"datastart",
    "([int bytes]):(scalararray, [int bytes]) - gets or sets startpoint in bytes. The startpoint is the base from which offsets are applied. Useful for accessing individual components. Will keep size and reapply offset from new startpoint.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_MOUNTED,"mounted",
    "(boolean):(scalararray) - returns whether array is mounted, i.e. points to external memory. When mounted to internal data, its rather unsafe to change sizes or strides.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_MOUNT,"mountscalararray",
    "(scalararray):(scalararray) - returns a new mounted array on the current's content. Mounting a scalararray from another will make sure the data stays valid, as it prevents host's gc. The maximum size you can operate with, is taken at the moment the mount happens using active offset,stride and count. Resizing host via maxscalars will cause crashes!");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_MOUNTSTATIC,"mountstaticarray",
    "(floatarray/intarray):(scalararray) - returns a new mounted array on the current's content. Mounting a staticarray from another will make sure the data stays valid, as it prevents host's gc. The maximum size you can operate with, is taken at the moment the mount happens using active offset,stride and count. Resizing host via maxscalars will cause crashes!");

  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_PTRS,"datapointer",
    "(pointer start, end, sarray3dstruct):(staticarray self) - you can access the memory directly in other lua dlls. Be aware that you must make sure not to corrupt memory, make sure you are always smaller than the 'end' pointer.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_COPYFROMSTRING,"fromstring",
    "():(scalararray, binstring, [byteoffset]) - copies the data from a binary string. Copies as much as possible, ignores dataoffset. Will throw error if stride does not match vectordim.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_ASSTRING,"asstring",
    "(binstring):(scalararray) - returns the current content (size,type,offset...) as binary string.");

  // Following functions are only needed for lua
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_TYPE,"scalartype",
    "([scalartype]):(scalararray, [scalartype]) - gets or sets current scalartype. Changing can influence size as types have different memcosts.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_VECTORSIZE,"vectorsize",
    "([int]):(scalararray, [int]) - gets or sets current vector dimension. Raising may cause vectorstride to change.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_VECTORSTRIDE,"vectorstride",
    "([int]):(scalararray, [int]) - gets or sets current vector stride. This must be zero or vectorsize as minimum, but can be greater for interleaved data. Using zero allows to create virtually any sizes from a single vector.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_SIZE,"size",
    "([int w,h,d]):(scalararray, [int w,h,d]) - gets or sets current total dimension of used elements. Will throw error if datasize isnt sufficient for current vectorsize and vectorstride. Setting will reset offset to 0,0,0.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_OFFSET,"offset",
    "([int w,h,d]):(scalararray, [int w,h,d]) - gets or sets sets access pointer from current memchunk start. Must be within current size.");

  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_COUNT,"count",
    "(int):(scalararray) - returns maximum number of current affected vectors (takes size and offset into account)");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_CONVERT,"convert",
    "(scalararray to):(scalararray to, scalararray from) - Straight converts current vector count from one scalararray (different types allowed) to another");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_CONVERTRANGED,"convertranged",
    "(scalararray to):(scalararray to, value tomin, tomax, scalararray from, value frommin, frommax) - Straight converts current vector count from one scalararray (different types allowed) to another. And ranges input and output values accordingly. values are int/float.");

  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_VECTOR,"vector",
    "([values...]):(scalararray,index,[values...]) - assigns i-th vector the float/int value if \
    given or simply returns the value at the index. Returns error in case of invalid indices (< 0 or > count).");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_VECTORALL,"vectorall",
    "():(scalararray,[values...]) - assigns all vectors the float/int values");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_VECTORDIRECT,"vectorat",
    "([values...]):(scalararray,x,y,z,[values...]) - assigns vector at the given position the float/int value if \
    given or simply returns the value at the position. Returns error in case of invalid position address (< 0 or > size).");

  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_VECTORSAMPLE,"sample",
    "([float values...):(scalararray,float x,y,z, [boolean unclamped]) - samples data with given texcoord, always returns float interpolated data. Unclamped is true by default.");

  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_CURVE_LINEAR,"curvelinear",
    "(scalararray):(scalararray out, scalararray keys, [boolean closed]) - creates linear interpolated curve/line based from the key segment points. Closed is false by default. Make sure vectorsizes are equal and out array has higher count than key array.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_prop,(void*)PSA_CURVE_SPLINE,"curvespline",
    "(scalararray):(scalararray out, scalararray keys, [boolean closed]) - creates catmull rom spline interpolated curve/line based from the key segment points. Closed is false by default. Make sure vectorsizes are equal and out array has higher count than key array.");

  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_op,(void*)PSO_OP0,"op0",
    "(scalararray):(scalararray,scalarop) - performs a single operation. Returns self on success.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_op,(void*)PSO_OP1,"op1",
    "(scalararray):(scalararray out,scalarop,scalararray/values) - performs a single operation into out. Returns self on success.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_op,(void*)PSO_OP2,"op2",
    "(scalararray):(scalararray out,scalarop,scalararray,scalararray/values) - performs a dual operation into out. Returns self on success.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_op,(void*)PSO_OP3,"op3",
    "(scalararray):(scalararray out,scalarop,scalararray,scalararray,scalararray/values) - performs a triple operation int out. Returns self on success.");

  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_op,(void*)PSO_OP0_REGION,"op0region",
    "(scalararray):(scalararray,scalarop, int w,h,d) - performs a single operation. Returns self on success.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_op,(void*)PSO_OP1_REGION,"op1region",
    "(scalararray):(scalararray out, int w,h,d, scalarop, scalararray/values) - performs a single operation into out. Returns self on success.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_op,(void*)PSO_OP2_REGION,"op2region",
    "(scalararray):(scalararray out,int w,h,d, scalarop, scalararray,scalararray/values) - performs a dual operation into out. Returns self on success.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_op,(void*)PSO_OP3_REGION,"op3region",
    "(scalararray):(scalararray out,int w,h,d, scalarop, scalararray,scalararray,scalararray/values) - performs a triple operation int out. Returns self on success.");

  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_fop,(void*)LUX_FSCALAR_OP1_TRANSFORM,"ftransform",
    "(scalararray):(scalararray outfloats, scalararray floats, matrix44) - transforms the vectors (size must 2,3 or 4) with given matrix. For Vector4 ignores W. Input and output array can be same.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_fop,(void*)LUX_FSCALAR_OP1_TRANSFORMROT,"ftransformrot",
    "(scalararray):(scalararray outfloats, scalararray floats, matrix44) - rotates the vectors (size must 2,3 or 4) with given matrix. Input and output array can be same.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_fop,(void*)LUX_FSCALAR_OP1_TRANSFORMFULL,"ftransformfull",
    "(scalararray):(scalararray outfloats, scalararray floats, matrix44) - homogeneously transforms the vectors (size must 2,3 or 4) with given matrix. Vectorsize of 2 and 3 will be divided by w coordinate. Input and output array can be same.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_fop,(void*)LUX_FSCALAR_OP1_NORMALIZE,"fnormalize",
    "(scalararray):(scalararray outfloats, scalararray floats) - normalizes the vectors to unit-length. Fast version less accurate, output length may not exactly be 1. Input and output array can be same.");
  FunctionPublish_addFunction(LUXI_CLASS_SCALARARRAY,PubScalarArray_fop,(void*)LUX_FSCALAR_OP1_NORMALIZEACC,"fnormalizeacc",
    "(scalararray):(scalararray outfloats, scalararray floats) - normalizes the vectors to unit-length. Accurate version. Input and output array can be same.");
}
