// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include <stdio.h>
#include "fnpublish.h"
#include "../common/3dmath.h"
#include "../common/memorymanager.h"
#include "../common/reflib.h"
#include "../render/gl_camlight.h"
#include <luxinia/luxinia.h>

// Published here:
// LUXI_CLASS_MATH
// LUXI_CLASS_MATRIX44


float *PubMatrix4x4_alloc()
{
  float* ptr = genzallocSIMD(sizeof(float)*16);
  return ptr;
}

static void PubMatrix4x4_free (void *rptr)
{
  genfreeSIMD(rptr,sizeof(float)*16);
}

int PubMatrix4x4_return (PState pstate,float *m) {
  float *matrix;
  Reference ref;

  matrix = PubMatrix4x4_alloc();
  lxMatrix44CopySIMD(matrix,m);
  ref = Reference_new(LUXI_CLASS_MATRIX44,matrix);

  Reference_makeVolatile(ref);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_MATRIX44,REF2VOID(ref));
}


static int PubMatrix4x4_new (PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  float *cpy;
  Reference ref;

  matrix = PubMatrix4x4_alloc();



  if (n>0){
    PubArray_t map;

    if (FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATRIX44,&ref) && Reference_get(ref,cpy)){
      lxMatrix44CopySIMD(matrix,cpy);
    }
    else{
      if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_ARRAY_FLOAT,(void*)&map) ||
        map.length < 16)
      {
        return FunctionPublish_returnError(pstate,
          "an array with 16 elements must be passed");
      }

      FunctionPublish_fillArray(pstate,0,LUXI_CLASS_ARRAY_FLOAT,&map,matrix);
    }
  }
  else{
    lxMatrix44IdentitySIMD(matrix);
  }

  ref = Reference_new(LUXI_CLASS_MATRIX44,matrix);
  Reference_makeVolatile(ref);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_MATRIX44,REF2VOID(ref));
}

static int PubMatrix4x4_toString (PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  Reference ref;

  if((n == 0) || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATRIX44,(void*)&ref) || !Reference_get(ref,matrix)))
    return FunctionPublish_returnError(pstate,"matrix required");

  return FunctionPublish_returnFString(pstate,"(%f,%f,%f,%f\n  %f,%f,%f,%f\n  %f,%f,%f,%f,\n  %f,%f,%f,%f)",
    lxVector4Unpack(matrix),lxVector4Unpack(matrix+4),lxVector4Unpack(matrix+8),lxVector4Unpack(matrix+12));

}

static int PubMatrix4x4_component(PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  Reference ref;
  int comp;

  if((n < 2 ) || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATRIX44,(void*)&ref) || ! Reference_get(ref,matrix))
    || FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&comp)==0 || comp < 0 || comp > 15)
    return FunctionPublish_returnError(pstate,"1 matrix4x4 and 1 int (0-15) required");

  if (n == 2)
    return FunctionPublish_returnFloat(pstate,matrix[comp]);
  else if (!FunctionPublish_getNArg(pstate,2,FNPUB_TFLOAT(matrix[comp])))
    return FunctionPublish_returnError(pstate,"1 float required");

  return 0;
}

static int PubMatrix4x4_identity(PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  Reference ref;

  if((n != 1) || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATRIX44,(void*)&ref) || ! Reference_get(ref,matrix)))
    return FunctionPublish_returnError(pstate,"matrix4x4 required");

  lxMatrix44IdentitySIMD(matrix);

  return FunctionPublish_returnType(pstate,LUXI_CLASS_MATRIX44,REF2VOID(ref));
}

static int PubMatrix4x4_rot (PState pstate, PubFunction_t *f, int n)
{
  Reference ref;
  float *x,*y,*z;
  float *matrix;
  lxVector4 vector;

  if ((n<1) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATRIX44,(void*)&ref)    ||
    !Reference_get(ref,matrix))
    return FunctionPublish_returnError(pstate,"1 matrix4x4 required");


  switch((int)f->upvalue) {
  case ROT_AXIS:
    if (n<2){
      x = y = z = matrix;
      y+=4;
      z+=8;
      return FunctionPublish_setRet(pstate,9, FNPUB_FROMVECTOR3(x)
        ,FNPUB_FROMVECTOR3(y)
        ,FNPUB_FROMVECTOR3(z));
    }
    else{
      x = y = z = matrix;
      y+=4;
      z+=8;
      if (FunctionPublish_getArgOffset(pstate,1,9,FNPUB_TOVECTOR3(x),
        FNPUB_TOVECTOR3(y),
        FNPUB_TOVECTOR3(z)) != 9)
        return FunctionPublish_returnError(pstate,"9 floats required");
      return 0;
    }
    break;
  case ROT_QUAT:
    if (n<2){
      lxQuatFromMatrix(vector,matrix);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(vector)) < 4)
        return FunctionPublish_returnError(pstate,"4 numbers required");
      lxQuatToMatrix(vector,matrix);
      return 0;
    }
  default:
  case ROT_RAD:
    if (n<2){
      lxMatrix44ToEulerZYX(matrix,vector);
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(vector)) != 3)
        return FunctionPublish_returnError(pstate,"3 numbers required");
      lxMatrix44FromEulerZYXFast(matrix,vector);
      return 0;
    }
    break;
  case ROT_DEG:
    if (n<2){
      lxMatrix44ToEulerZYX(matrix,vector);
      vector[0] = LUX_RAD2DEG(vector[0]);
      vector[1] = LUX_RAD2DEG(vector[1]);
      vector[2] = LUX_RAD2DEG(vector[2]);
      return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vector));
    }
    else{
      if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(vector)) != 3)
        return FunctionPublish_returnError(pstate,"3 numbers required");
      lxMatrix44FromEulerZYXdeg(matrix,vector);
      return 0;
    }
    break;

  }

  return 0;
}

static int PubMatrix4x4_transform(PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  Reference ref;
  float vec[3];

  if((n != 4) || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATRIX44,(void*)&ref) || ! Reference_get(ref,matrix)))
    return FunctionPublish_returnError(pstate,"matrix4x4 and 3 floats required");

  if (FunctionPublish_getArg(pstate,4,LUXI_CLASS_MATRIX44,(void*)&ref,LUXI_CLASS_FLOAT,vec,LUXI_CLASS_FLOAT,vec+1,LUXI_CLASS_FLOAT,vec+2)!=4)
    return FunctionPublish_returnError(pstate,"matrix4x4 and 3 floats required");

  switch ((int)f->upvalue) {
    default: lxVector3Transform1(vec,matrix); break;
    case 1: lxMatrix44VectorRotate(matrix,vec); break;
  }

  return FunctionPublish_setRet(pstate,3,FNPUB_FFLOAT(vec[0]),FNPUB_FFLOAT(vec[1]),
    FNPUB_FFLOAT(vec[2]));
}

static int PubMatrix4x4_position(PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  Reference ref;
  float vec[3];

  if((!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATRIX44,(void*)&ref) || ! Reference_get(ref,matrix)))
    return FunctionPublish_returnError(pstate,"matrix4x4 required");

  switch (n) {
    default:
      return 0;
    case 1:
      return FunctionPublish_setRet(pstate,3,FNPUB_FFLOAT(matrix[12]),
        FNPUB_FFLOAT(matrix[13]),
        FNPUB_FFLOAT(matrix[14]));
    case 4:
      if (FunctionPublish_getArgOffset(pstate,1,3,LUXI_CLASS_FLOAT,vec,LUXI_CLASS_FLOAT,vec+1,LUXI_CLASS_FLOAT,vec+2)!=3)
        return FunctionPublish_returnError(pstate,"matrix4x4 and 3 floats required");

      matrix[12] = vec[0];
      matrix[13] = vec[1];
      matrix[14] = vec[2];

      return 0;
  }
  return 0;
}

static int PubMatrix4x4_viewmat(PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  Reference ref;

  if((!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATRIX44,(void*)&ref) || ! Reference_get(ref,matrix)))
    return FunctionPublish_returnError(pstate,"matrix4x4 required");

  CameraViewMatrix(matrix,matrix);

  return FunctionPublish_returnType(pstate,LUXI_CLASS_MATRIX44,REF2VOID(ref));
}

static int PubMatrix4x4_copy(PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  Reference ref;
  float *matrix2;
  Reference ref2;

  if(n!=2 || 2!=FunctionPublish_getArg(pstate,2,LUXI_CLASS_MATRIX44,(void*)&ref,LUXI_CLASS_MATRIX44,(void*)&ref2) || ! Reference_get(ref,matrix) || ! Reference_get(ref2,matrix2))
    return FunctionPublish_returnError(pstate,"2 matrix4x4 required");
  lxMatrix44CopySIMD(matrix,matrix2);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_MATRIX44,REF2VOID(ref));
}

static int PubMatrix4x4_scale(PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  Reference ref;
  float vec[3];

  if((!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATRIX44,(void*)&ref) || ! Reference_get(ref,matrix)))
    return FunctionPublish_returnError(pstate,"matrix4x4 required");

  switch (n) {
    default:
      return 0;
    case 1:
      return FunctionPublish_setRet(pstate,3,FNPUB_FFLOAT(matrix[0]),
        FNPUB_FFLOAT(matrix[5]),
        FNPUB_FFLOAT(matrix[10]));
    case 4:
      if (FunctionPublish_getArgOffset(pstate,1,3,LUXI_CLASS_FLOAT,vec,LUXI_CLASS_FLOAT,vec+1,LUXI_CLASS_FLOAT,vec+2)!=3)
        return FunctionPublish_returnError(pstate,"matrix4x4 and 3 floats required");

      matrix[0] = vec[0];
      matrix[5] = vec[1];
      matrix[10] = vec[2];

      return 0;
  }
  return 0;
}


static int PubMatrix4x4_column(PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  Reference ref;
  int axis;

  if(FunctionPublish_getArg(pstate,2,LUXI_CLASS_MATRIX44,(void*)&ref,LUXI_CLASS_INT,(void*)&axis)!=2
    || ! Reference_get(ref,matrix) || axis > 3 || axis < 0)
    return FunctionPublish_returnError(pstate,"matrix4x4, 1 int (0-3) required");

  switch (n) {
    default:
      return FunctionPublish_returnError(pstate,"matrix4x4, 1 int (0-3) required");
    case 2:
        return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4((matrix+axis*4)));
    case 5:
      if (FunctionPublish_getArgOffset(pstate,2,4, FNPUB_TOVECTOR4((matrix+axis*4)))!=4)
        return FunctionPublish_returnError(pstate,"matrix4x4, 1 int (0-3) and 4 floats required");

      return 0;
  }
  return 0;
}

static int PubMatrix4x4_row(PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  Reference ref;
  lxVector4 vec;
  int axis;

  if(FunctionPublish_getArg(pstate,2,LUXI_CLASS_MATRIX44,(void*)&ref,LUXI_CLASS_INT,(void*)&axis)!=2
    || ! Reference_get(ref,matrix) || axis > 3 || axis < 0)
    return FunctionPublish_returnError(pstate,"matrix4x4, 1 int (0-3) required");

  matrix+=axis;
  switch (n) {
    default:
      return FunctionPublish_returnError(pstate,"matrix4x4, 1 int (0-3) required");
    case 2:
      lxVector4Set(vec,matrix[0],matrix[4],matrix[8],matrix[12]);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vec));
    case 5:
      if (FunctionPublish_getArgOffset(pstate,2,4, FNPUB_TOVECTOR4(vec))!=4)
        return FunctionPublish_returnError(pstate,"matrix4x4, 1 int (0-3) and 4 floats required");
      matrix[0]=vec[0];
      matrix[4]=vec[1];
      matrix[8]=vec[2];
      matrix[12]=vec[3];
      return 0;
  }
  return 0;
}

static int PubMatrix4x4_compress(PState pstate, PubFunction_t *f, int n)
{
  static lxMatrix44SIMD matrix;

  float *matsrc;
  Reference ref;


  if(!FunctionPublish_getArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&ref)
    || ! Reference_get(ref,matsrc))
    return FunctionPublish_returnError(pstate,"matrix4x4 required");

  lxMatrix44CopySIMD(matrix,matsrc);
    // 4x3
  if(f->upvalue){
    matrix[3] = matrix[4];
    matrix[4] = matrix[5];
    matrix[5] = matrix[6];

    matrix[6] = matrix[8];
    matrix[7] = matrix[9];
    matrix[8] = matrix[10];

    matrix[9]  = matrix[12];
    matrix[10] = matrix[13];
    matrix[11] = matrix[14];
  } // 3x3
  else{
    matrix[3] = matrix[4];
    matrix[4] = matrix[5];
    matrix[5] = matrix[6];

    matrix[6] = matrix[8];
    matrix[7] = matrix[9];
    matrix[8] = matrix[10];
  }

  return PubMatrix4x4_return(pstate,matrix);
}

static int PubMatrix4x4_setAxisAngle(PState pstate, PubFunction_t *f, int n)
{
  float *matrix;
  Reference ref;
  float rot[3],angleRad;

  if((n != 5) || (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_MATRIX44,(void*)&ref) || ! Reference_get(ref,matrix)))
    return FunctionPublish_returnError(pstate,"matrix4x4 and 3+1 floats required");

  lxMatrix44Identity(matrix);
  if (FunctionPublish_getArg(pstate,5,LUXI_CLASS_MATRIX44,(void*)&ref,LUXI_CLASS_FLOAT,rot,LUXI_CLASS_FLOAT,rot+1,LUXI_CLASS_FLOAT,rot+2,LUXI_CLASS_FLOAT,&angleRad)!=5)
    return FunctionPublish_returnError(pstate,"matrix4x4 and 3+1 floats required");

  lxMatrix44RotateAroundVectorFast(matrix, rot, angleRad);

  return 0;
}

static int PubMatrix4x4_mul(PState pstate, PubFunction_t *f, int n)
{
  static lxMatrix44SIMD cache;
  float *a,*b,*c;
  Reference A,B,C;


  switch(n) {
    default:
      return FunctionPublish_returnError(pstate,"2 or 3 matrix4x4 required");

    case 2:
      if ((!FunctionPublish_getArg(pstate,2,LUXI_CLASS_MATRIX44,(void*)&A,LUXI_CLASS_MATRIX44,(void*)&B)
        || ! Reference_get(A,a) || ! Reference_get(B,b)))
      return FunctionPublish_returnError(pstate,"2 matrix4x4 required");

      if (f->upvalue){
        lxMatrix44MultiplyFullSIMD(cache,a,b);
        lxMatrix44CopySIMD(a,cache);
      }
      else
        lxMatrix44Multiply1SIMD(a,b);
      return FunctionPublish_returnType(pstate,LUXI_CLASS_MATRIX44,REF2VOID(A));
    case 3:
      if ((!FunctionPublish_getArg(pstate,3,LUXI_CLASS_MATRIX44,(void*)&A,LUXI_CLASS_MATRIX44,(void*)&B,LUXI_CLASS_MATRIX44,(void*)&C)
        || ! Reference_get(A,a) || ! Reference_get(B,b) || ! Reference_get(C,c)))
      return FunctionPublish_returnError(pstate,"3 matrix4x4 required");

      if (a == b || c == a){
        if (f->upvalue)
          lxMatrix44MultiplyFullSIMD(cache,b,c);
        else
          lxMatrix44MultiplySIMD(cache,b,c);
        lxMatrix44CopySIMD(a,cache);
      }
      else if (!f->upvalue)
        lxMatrix44MultiplySIMD(a,b,c);
      else
        lxMatrix44MultiplyFullSIMD(a,b,c);

      return FunctionPublish_returnType(pstate,LUXI_CLASS_MATRIX44,REF2VOID(A));
  }
}

static int PubMatrix4x4_lookat(PState pstate, PubFunction_t *f, int n)
{
  float *mat;
  lxVector3 to,up;
  Reference ref;
  int axis;

  axis = 1;
  if (n < 7 || FunctionPublish_getArg(pstate,8,LUXI_CLASS_MATRIX44,(void*)&ref,FNPUB_TOVECTOR3(to),FNPUB_TOVECTOR3(up),LUXI_CLASS_INT,(void*)&axis) < 7
    || !Reference_get(ref,mat) || axis > 2 || axis < 0)
    return FunctionPublish_returnError(pstate," 1 matrix4x4 6 floats [1 int 0-2] required");

  lxVector3Sub(to,to,&mat[12]);
  lxVector3Normalized(to);
  lxMatrix44Orient(mat,to,up,axis);

  return 0;
}

static int PubMatrix4x4_affinvert(PState pstate, PubFunction_t *f,int n)
{
  static lxMatrix44SIMD mat;

  float *a,*b;
  Reference A,B;


  switch(n) {
    default:
      return FunctionPublish_returnError(pstate,"1 or 2 matrix4x4 required");

    case 1:
      if ((!FunctionPublish_getArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&A)
        || ! Reference_get(A,a)))
      return FunctionPublish_returnError(pstate,"1 matrix4x4 required");

      lxMatrix44CopySIMD(mat,a);
      lxMatrix44AffineInvertSIMD(a,mat);

      return 0;
    case 2:
      if ((!FunctionPublish_getArg(pstate,2,LUXI_CLASS_MATRIX44,(void*)&A,LUXI_CLASS_MATRIX44,(void*)&B)
        || ! Reference_get(A,a) || ! Reference_get(B,b)))
      return FunctionPublish_returnError(pstate,"3 matrix4x4 required");

      lxMatrix44AffineInvertSIMD(a,b);
      return 0;
  }
  return FunctionPublish_returnType(pstate,LUXI_CLASS_MATRIX44,REF2VOID(A));
}

static int PubMatrix4x4_invert(PState pstate, PubFunction_t *f,int n)
{
  static lxMatrix44SIMD mat;

  float *a,*b;
  Reference A,B;


  switch(n) {
    default:
      return FunctionPublish_returnError(pstate,"1 or 2 matrix4x4 required");

    case 1:
      if ((!FunctionPublish_getArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&A)
        || ! Reference_get(A,a)))
        return FunctionPublish_returnError(pstate,"1 matrix4x4 required");

      lxMatrix44CopySIMD(mat,a);
      lxMatrix44InvertSIMD(a,mat);

      return 0;
    case 2:
      if ((!FunctionPublish_getArg(pstate,2,LUXI_CLASS_MATRIX44,(void*)&A,LUXI_CLASS_MATRIX44,(void*)&B)
        || ! Reference_get(A,a) || ! Reference_get(B,b)))
        return FunctionPublish_returnError(pstate,"3 matrix4x4 required");

      lxMatrix44InvertSIMD(a,b);
      return 0;
  }
  return FunctionPublish_returnType(pstate,LUXI_CLASS_MATRIX44,REF2VOID(A));
}

static int PubMatrix4x4_transpose(PState pstate, PubFunction_t *f,int n)
{
  static lxMatrix44SIMD mat;
  float *a,*b;
  Reference A,B;

  switch(n) {
    default:
      return FunctionPublish_returnError(pstate,"1 or 2 matrix4x4 required");

    case 1:
      if ((!FunctionPublish_getArg(pstate,1,LUXI_CLASS_MATRIX44,(void*)&A)
        || ! Reference_get(A,a)))
        return FunctionPublish_returnError(pstate,"1 matrix4x4 required");

      lxMatrix44CopySIMD(mat,a);
      lxMatrix44TransposeSIMD(a,mat);

      return 0;
    case 2:
      if ((!FunctionPublish_getArg(pstate,2,LUXI_CLASS_MATRIX44,(void*)&A,LUXI_CLASS_MATRIX44,(void*)&B)
        || ! Reference_get(A,a) || ! Reference_get(B,b)))
        return FunctionPublish_returnError(pstate,"3 matrix4x4 required");

      lxMatrix44TransposeSIMD(a,b);
      return 0;
  }
  return FunctionPublish_returnType(pstate,LUXI_CLASS_MATRIX44,REF2VOID(A));
}

static int PubMatrix4x4_proj(PState pstate, PubFunction_t *f,int n)
{
  float* mat;
  lxRmatrix4x4 matref;
  lxVector4 vec;

  if (5>FunctionPublish_getArg(pstate,5,LUXI_CLASS_MATRIX44,&matref,FNPUB_TOVECTOR4(vec)) ||
    !Reference_get(matref,mat)){
    return FunctionPublish_returnError(pstate,"1 matrix4x4 4 floats required");
  }

  if(vec[2] >= 0.0){
    //glLoadIdentity ();
    //gluPerspective(cam->fov/aspect,aspect,cam->frontplane,cam->backplane);
    lxMatrix44Perspective(mat,vec[2]/vec[3],vec[0],vec[1],vec[3]);
    //MatrixPerspective(cam->projaspect1,cam->fov,cam->frontplane,cam->backplane,1);
  }
  else{
    //glLoadIdentity();
    //glOrtho(-size*aspect,size*aspect,-size,size,cam->frontplane,cam->backplane);
    lxMatrix44Ortho(mat,-vec[2]/vec[3],vec[0],vec[1],vec[3]);
    //MatrixOrtho(cam->projaspect1,size,cam->frontplane,cam->backplane,1);
  }
  return 0;
}

void RPubMatrix4x4_free(Reference ref)
{
  PubMatrix4x4_free(Reference_value(ref));
}

enum PubMathFuncs_e{
  PMATH_QUATSLERP,
  PMATH_QUATSLERPQUAD,
  PMATH_QUATSLERPQUADTANGENT,
  PMATH_QUAT2EULERRAD,
  PMATH_QUAT2EULERDEG,
  PMATH_EULERRAD2QUAT,
  PMATH_EULERDEG2QUAT,
  PMATH_FMCACHE,
};

static int PubMath_func(PState *state, PubFunction_t *f,int n)
{
  static lxMatrix44SIMD mat;

  lxVector4 vecA;
  lxVector4 vecB;
  lxVector4 vecC;
  lxVector4 vecD;


  float flt;

  switch((int)f->upvalue){
  case PMATH_QUATSLERP:
    if (n < 9 || 9>FunctionPublish_getArg(state,9,FNPUB_TFLOAT(flt),FNPUB_TOVECTOR4(vecA),FNPUB_TOVECTOR4(vecB)))
      return FunctionPublish_returnError(state,"9 floats required");

    lxQuatSlerp(vecC,flt,vecA,vecB);
    return FunctionPublish_setRet(state,4,FNPUB_FROMVECTOR4(vecC));
  case PMATH_QUATSLERPQUAD:
    if (n < 17 || 17>FunctionPublish_getArg(state,17,FNPUB_TFLOAT(flt),FNPUB_TOVECTOR4(vecA),FNPUB_TOVECTOR4(vecB),FNPUB_TOVECTOR4(vecC),FNPUB_TOVECTOR4(vecD)))
      return FunctionPublish_returnError(state,"17 floats required");

    lxQuatSlerpQuadratic(mat,flt,vecA,vecB,vecC,vecD);
    return FunctionPublish_setRet(state,4,FNPUB_FROMVECTOR4(mat));
  case PMATH_QUATSLERPQUADTANGENT:
    if (n < 17 || 17>FunctionPublish_getArg(state,17,FNPUB_TFLOAT(flt),FNPUB_TOVECTOR4(vecA),FNPUB_TOVECTOR4(vecB),FNPUB_TOVECTOR4(vecC),FNPUB_TOVECTOR4(vecD)))
      return FunctionPublish_returnError(state,"17 floats required");

    lxQuatSlerpQuadTangents(mat,flt,vecA,vecB,vecC,vecD);
    return FunctionPublish_setRet(state,4,FNPUB_FROMVECTOR4(mat));
  case PMATH_QUAT2EULERRAD:
    if (n < 4 || 4>FunctionPublish_getArg(state,4,FNPUB_TOVECTOR4(vecA)))
      return FunctionPublish_returnError(state,"4 floats required");
    lxQuatToMatrixIdentity(vecA,mat);
    lxMatrix44ToEulerZYX(mat,vecA);
    return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(vecA));
  case PMATH_QUAT2EULERDEG:
    if (n < 4 || 4>FunctionPublish_getArg(state,4,FNPUB_TOVECTOR4(vecA)))
      return FunctionPublish_returnError(state,"4 floats required");
    lxQuatToMatrixIdentity(vecA,mat);
    lxMatrix44ToEulerZYX(mat,vecA);
    vecA[0] = LUX_RAD2DEG(vecA[0]);
    vecA[1] = LUX_RAD2DEG(vecA[1]);
    vecA[2] = LUX_RAD2DEG(vecA[2]);
    return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(vecA));
  case PMATH_EULERRAD2QUAT:
    if (n < 3 || 3>FunctionPublish_getArg(state,3,FNPUB_TOVECTOR4(vecA)))
      return FunctionPublish_returnError(state,"3 floats required");
    lxMatrix44FromEulerZYXFast(mat,vecA);
    lxQuatFromMatrix(vecA,mat);
    return FunctionPublish_setRet(state,4,FNPUB_FROMVECTOR4(vecA));
  case PMATH_EULERDEG2QUAT:
    if (n < 3 || 3>FunctionPublish_getArg(state,3,FNPUB_TOVECTOR4(vecA)))
      return FunctionPublish_returnError(state,"3 floats required");
    vecA[0] = LUX_DEG2RAD(vecA[0]);
    vecA[1] = LUX_DEG2RAD(vecA[1]);
    vecA[2] = LUX_DEG2RAD(vecA[2]);
    lxMatrix44FromEulerZYXFast(mat,vecA);
    lxQuatFromMatrix(vecA,mat);
    return FunctionPublish_setRet(state,4,FNPUB_FROMVECTOR4(vecA));
  case PMATH_FMCACHE:
    return FunctionPublish_returnPointer(state,Luxinia_getFastMathCache());
  default:
    return 0;
  }

  return 0;
}

void PubClass_Matrix16_init()
{
  FunctionPublish_initClass(LUXI_CLASS_MATH,"mathlib","provides some accelerated functions dealing with vector/matrix math or array operations",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_MATH,PubMath_func,(void*)PMATH_QUATSLERP,"quatslerp",
    "(float qx,qy,qz,qw):(float fracc,ax,ay,az,aw,bx,by,bz,bw) - returns quaternion spherical interpolation bettween a and b");
  FunctionPublish_addFunction(LUXI_CLASS_MATH,PubMath_func,(void*)PMATH_QUATSLERPQUAD,"quatslerpq",
    "(float qx,qy,qz,qw):(float fracc,px,py,pz,pw,ax,ay,az,aw,bx,by,bz,bw,nx,ny,nz,nw) - returns quaternion spherical quadratic interpolation bettween a and b");
  FunctionPublish_addFunction(LUXI_CLASS_MATH,PubMath_func,(void*)PMATH_QUATSLERPQUADTANGENT,"quatslerpqt",
    "(float qx,qy,qz,qw):(float fracc,px,py,pz,pw,ax,ay,az,aw,bx,by,bz,bw,nx,ny,nz,nw) - returns quaternion spherical quadratic tangent interpolation bettween a and b");
  FunctionPublish_addFunction(LUXI_CLASS_MATH,PubMath_func,(void*)PMATH_EULERRAD2QUAT,"rotrad2quat",
    "(float qx,qy,qz,qw):(float ax,ay,az) - converts euler angles (radians) to quaternion");
  FunctionPublish_addFunction(LUXI_CLASS_MATH,PubMath_func,(void*)PMATH_EULERDEG2QUAT,"rotdeg2quat",
    "(float qx,qy,qz,qw):(float ax,ay,az) - converts euler angles (degrees) to quaternion");
  FunctionPublish_addFunction(LUXI_CLASS_MATH,PubMath_func,(void*)PMATH_QUAT2EULERRAD,"quat2rotrad",
    "(float ax,ay,az):(float qx,qy,qz,qw) - converts quaternion to euler angles (radians)");
  FunctionPublish_addFunction(LUXI_CLASS_MATH,PubMath_func,(void*)PMATH_QUAT2EULERDEG,"quat2rotdeg",
    "(float ax,ay,az):(float qx,qy,qz,qw) - converts quaternion to euler angles (degrees)");

  FunctionPublish_setFunctionInherition(LUXI_CLASS_MATH,"quatslerp",LUX_FALSE);
  FunctionPublish_setFunctionInherition(LUXI_CLASS_MATH,"rotrad2quat",LUX_FALSE);
  FunctionPublish_setFunctionInherition(LUXI_CLASS_MATH,"rotdeg2quat",LUX_FALSE);
  FunctionPublish_setFunctionInherition(LUXI_CLASS_MATH,"quat2rotrad",LUX_FALSE);
  FunctionPublish_setFunctionInherition(LUXI_CLASS_MATH,"quat2rotdeg",LUX_FALSE);

    Reference_registerType(LUXI_CLASS_MATRIX44,"matrix4x4",RPubMatrix4x4_free,NULL);
  FunctionPublish_initClass(LUXI_CLASS_MATRIX44,"matrix4x4","4x4 matrices are used\
 for position and rotational transformations. They are saved column-major.",
    NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_MATRIX44,LUXI_CLASS_MATH);

  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_new,NULL,"new",
    "(matrix4x4 m):([table float16]/[matrix4x4]) - constructs identity matrix or uses lua table or other matrix4x4 to fill values. If a table \
    with 16 elements is passed, the values are interpreted as a matrix and will \
    The OpenGL matrixstyle (column-major) is used so the x,y,z translation is found at element index 12,13,14 (or 13,14,15 as lua arrays start at \
    index 1, while C arrays start at index 0)");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_toString,NULL,"tostring",
    "(string str):(matrix4x4 vec) - returns matrix as string");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_component,NULL,"component",
    "([float]):(matrix4x4,int index [0..15],[float]) - returns or sets single matrix entry.");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_identity,NULL,"identity",
    "(matrix4x4):(matrix4x4) - sets diagonal of matrix to 1, rest to 0");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_mul,NULL,"mul",
    "(matrix4x4 a):(matrix4x4 a, b ,[c]) - multiplys: a = b*c or a = a*b. faster but sets W coordinate of each colum to 0, cept last to 1");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_mul,(void*)1,"mulfull",
    "(matrix4x4 a):(matrix4x4 a, b ,[c]) - multiplys: a = b*c or a = a*b. accurately takes all 16 values into account.");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_lookat,NULL,"lookat",
    "():(matrix4x4 a, float toX,toY,toZ, upX,upY,upZ, [int axis 0..2]) - sets rotation axis, so that axis (0:x 1:y 2:z default is y) aims at given target pos from current matrix position.");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_setAxisAngle,NULL,"setaxisangle",
    "():(matrix4x4 a, float axisX,axisY,axisZ ,rotation) - sets rotation around angle");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_transform,(void*)0,"transform",
    "(float x,y,z):(matrix4x4 m,float x,y,z) - transforms the three given coordinates");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_transform,(void*)1,"transformrotate",
    "(float x,y,z):(matrix4x4 m,float x,y,z) - rotate the three given coordinates");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_position,NULL,"pos",
    "([float x,y,z]):(matrix4x4 m,[float x,y,z]) - sets/gets position of matrix");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_viewmat,NULL,"viewmatrix",
    "(matrix4x4):(matrix4x4) - turns self into a viewmatrix usable for cameras");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_proj,NULL,"projmatrix",
    "(matrix4x4):(matrix4x4, nearplane, farplane, fov, aspect) - turns self into a projection matrix usable for cameras and projectors. negative fov means 'size' for orthographic projection. aspect is width/height.");

  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_rot,(void*)ROT_DEG,"rotdeg",
    "([float x,y,z]):(matrix4x4 m,[float x,y,z]) - returns or sets rotation in degrees");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_rot,(void*)ROT_RAD,"rotrad",
    "([float x,y,z]):(matrix4x4 m,[float x,y,z]) - returns or sets rotation in radians");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_rot,(void*)ROT_AXIS,"rotaxis",
    "([float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]):(matrix4x4 m,[float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]) - returns or sets rotation axis, make sure they make a orthogonal basis.");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_scale,NULL,"scale",
    "([float x,y,z]):(matrix4x4 m,[float x,y,z]) - sets/gets scale factors of matrix");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_rot,(void*)ROT_QUAT,"rotquat",
    "([float x,y,z,w]):(matrix4x4 m,[float x,y,z,w]) - sets/gets rotation as quaternion");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_copy,NULL,"copy",
    "(matrix4x4 to):(matrix4x4 to,matrix4x4 from) - copys from other to itself");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_column,NULL,"column",
    "([float x,y,z,w]):(matrix4x4 m,int column,[float x,y,z,w]) - sets/gets column (0-3) vector.");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_row,NULL,"row",
    "([float x,y,z,w]):(matrix4x4 m,int column,[float x,y,z,w]) - sets/gets row (0-3) vector.");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_affinvert,NULL,"affineinvert",
    "(matrix4x4 m):(matrix4x4 m,[matrix4x4 inv]) - inverts m, or m = invert of inv. Affine invert is faster and more robust than regular invert, but will only work on non-scaled orthogonal matrices.");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_invert,NULL,"invert",
    "(matrix4x4 m):(matrix4x4 m,[matrix4x4 inv]) - inverts m, or m = invert of inv");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_transpose,NULL,"transpose",
    "(matrix4x4 m):(matrix4x4 m,[matrix4x4 inv]) - transposes m, or m = transpose of inv");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_compress,(void*)0,"compressed4x3",
    "(matrix4x4):(matrix4x4) - copies and compresses the storage and returns new, for shadercgparamhost:value useage.");
  FunctionPublish_addFunction(LUXI_CLASS_MATRIX44,PubMatrix4x4_compress,(void*)1,"compressed3x3",
    "(matrix4x4):(matrix4x4) - copies and compresses the storage and returns new, for shadercgparamhost:value useage.");
}
