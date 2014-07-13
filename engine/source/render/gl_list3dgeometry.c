// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "gl_list3dgeometry.h"
#include "gl_list3d.h"
#include "../common/3dmath.h"
#include <float.h>


//////////////////////////////////////////////////////////////////////////
// ShadowMesh_t


static void ShadowMesh_updateFaceplanes(ShadowMesh_t *smesh,void *vertices)
{
  int i;
  ushort *primindex;
  int primstride;
  int  quad;
  int triscnt;
  ushort a,b,c;
  float *outnormal;
  float *vertpos;
  lxVector3 toB,toC;
  int vertstride;
  float *pVert0;
  float *pVert1;

  LUX_ASSERT(VertexValue(smesh->refmesh->vertextype,VERTEX_SCALARPOS) == LUX_SCALAR_FLOAT32);

  triscnt = smesh->refmesh->numTris;
  vertpos = VertexArrayPtr(vertices,0,smesh->refmesh->vertextype,VERTEX_POS);
  vertstride = VertexSize(smesh->refmesh->vertextype)/sizeof(float);
  primindex = smesh->refmesh->indicesData16;
  quad = LUX_FALSE;
  switch(smesh->refmesh->primtype){
  case PRIM_TRIANGLES:
    primstride = 3; break;
  case PRIM_TRIANGLE_STRIP:
    primstride = 1; break;
  case PRIM_QUADS:
    quad = LUX_TRUE; primstride = 2; break;
  default:
    return;
  }
  outnormal = smesh->faceplanes[0];
  for(i = 0; i < smesh->numFaceplanes; i++,primindex+=primstride,outnormal+=4){
    if (quad && i%2){
      a = *(primindex-2);
      b = *(primindex);
      c = *(primindex+1);
    }
    else{
      a = *primindex;
      b = *(primindex+1);
      c = *(primindex+2);
    }

    pVert0 = vertpos+(vertstride*a);
    pVert1 = vertpos+(vertstride*b);
    lxVector3Sub(toB,pVert1,pVert0);
    pVert1 = vertpos+(vertstride*c);
    lxVector3Sub(toC,pVert1,pVert0);
    lxVector3Cross(outnormal,toB,toC);
    lxVector3NormalizedFast(outnormal);
    outnormal[3] = -lxVector3Dot(outnormal,pVert0);
  }
}

// computes edges and faceplanes
int ShadowMesh_init(ShadowMesh_t *smesh, Mesh_t *refmesh)
{
  int triscnt;
  int i,n;
  ushort *primindex;
  int primstride;
  int edgecount = 0;
  int *sharedvert;
  int quad = LUX_FALSE;
  uint a,b,c;
  EdgeList_t *edgecache;
  EdgeList_t *edge;

  int outvertexcount;
  int outindexcount;

  smesh->refmesh = refmesh;

  if (!smesh->refmesh->index16)
    return LUX_FALSE;
  else if (smesh->refmesh->numGroups > 1)
    return LUX_FALSE;
  else if(smesh->refmesh->primtype == PRIM_TRIANGLES)
    primstride = 3;
  else if (smesh->refmesh->primtype == PRIM_TRIANGLE_STRIP)
    primstride = 1;
  else if (smesh->refmesh->primtype == PRIM_QUADS){
    primstride = 2; quad = LUX_TRUE;
  }
  else
    return LUX_FALSE;

  triscnt = smesh->refmesh->numTris;

  bufferclear();
  sharedvert = bufferzalloc(sizeof(int)*refmesh->numVertices);

  VertexArray_markWeld(refmesh->vertexData,refmesh->vertextype,refmesh->numVertices,sharedvert,0.00001);

  // first pass , a < b ccw
  // worst case alloc
  edge = edgecache = buffermalloc(sizeof(EdgeList_t)*triscnt*3);
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

  primindex = smesh->refmesh->indicesData16;
  for(i = 0; i < triscnt; i++,primindex+=primstride){
    if (quad && i%2){
      a = sharedvert[*(primindex-2)];
      b = sharedvert[*(primindex)];
      c = sharedvert[*(primindex+1)];
    }
    else{
      a = sharedvert[*(primindex)];
      b = sharedvert[*(primindex+1)];
      c = sharedvert[*(primindex+2)];
    }

    if (a < b){
      edge->edge.vertA = a;
      edge->edge.vertB = b;
      edge->faceA = i;
      edge->faceB = -1;
      edge++;
      edgecount++;
    }
    if (b < c){
      edge->edge.vertA = b;
      edge->edge.vertB = c;
      edge->faceA = i;
      edge->faceB = -1;
      edge++;
      edgecount++;
    }
    if (c < a){
      edge->edge.vertA = c;
      edge->edge.vertB = a;
      edge->faceA = i;
      edge->faceB = -1;
      edge++;
      edgecount++;
    }
  }

  // second pass, the other winding a > b cw
  // put ignore duplicates
  primindex = smesh->refmesh->indicesData16;
  for(i = 0; i < triscnt; i++,primindex+=primstride){
    if (quad && i%2){
      a = sharedvert[*(primindex-2)];
      b = sharedvert[*(primindex)];
      c = sharedvert[*(primindex+1)];
    }
    else{
      a = sharedvert[*(primindex)];
      b = sharedvert[*(primindex+1)];
      c = sharedvert[*(primindex+2)];
    }

    if (a > b){
      edge = edgecache;
      for (n = 0; n < edgecount; n++,edge++){
        if (edge->edge.vertA == b && edge->edge.vertB == a && edge->faceB < 0){
          edge->faceB = i;
          break;
        }
      }
    }
    if (b > c){
      edge = edgecache;
      for (n = 0; n < edgecount; n++,edge++){
        if (edge->edge.vertA == c && edge->edge.vertB == b && edge->faceB < 0){
          edge->faceB = i;
          break;
        }
      }
    }
    if (c > a){
      edge = edgecache;
      for (n = 0; n < edgecount; n++,edge++){
        if (edge->edge.vertA == a && edge->edge.vertB == c && edge->faceB < 0){
          edge->faceB = i;
          break;
        }
      }
    }
  }

  // third pass, check for closed edges
  edge = edgecache;

  for (i = 0; i < edgecount; i++,edge++){
    if (edge->faceA < 0 || edge->faceB < 0)
      return LUX_FALSE;
  }

  // we cant do uint meshes atm
  // we need the mesh verts twice (near and far)
  outvertexcount = refmesh->numVertices*2;
  // we may have caps + sides
  outindexcount = (refmesh->numTris*2 + (refmesh->numTris/4))*3;

  if (outvertexcount >= BIT_ID_FULL16)
    return LUX_FALSE;


  // copy contents
  smesh->edges = lxMemGenZalloc(sizeof(EdgeList_t)*edgecount);
  memcpy(smesh->edges,edgecache,sizeof(EdgeList_t)*edgecount);
  smesh->numEdges = edgecount;

  // compute normals
  smesh->faceplanes = lxMemGenZalloc(sizeof(lxVector4)*triscnt);
  smesh->numFaceplanes = triscnt;
  ShadowMesh_updateFaceplanes(smesh,smesh->refmesh->vertexData);

  // setup edge normal pointers
  edge = smesh->edges;
  for(i = 0; i < edgecount; i++,edge++){
    edge->planeA = (edge->faceA >= 0) ? smesh->faceplanes[edge->faceA] : NULL;
    edge->planeB = (edge->faceB >= 0) ? smesh->faceplanes[edge->faceB] : NULL;
  }

  // create meshes
  smesh->mesh = Mesh_new(NULL,outvertexcount,outindexcount,VERTEX_16_HOM);
  smesh->meshtype = SHADOWMSH_TYPE_UNSET;

  { // copy original verts

    float *outvert = smesh->mesh->vertexData;
    int refvcount = refmesh->numVertices;
    int invertstride;
    float *invertsarray;
    float *invert;

    invertstride = VertexSize(smesh->refmesh->vertextype)/sizeof(float);
    invertsarray = VertexArrayPtr(smesh->refmesh->vertexData,0,smesh->refmesh->vertextype,VERTEX_POS);

    LUX_ASSERT(VertexValue(smesh->refmesh->vertextype,VERTEX_SCALARPOS)==LUX_SCALAR_FLOAT32);

    for (i = 0 ; i < refvcount; i++,outvert+=4){
      invert = invertsarray+(invertstride*(i));
      lxVector3Copy(outvert,invert);
      outvert[3] = 1.0f;
    }
    smesh->lastvertices = NULL;
  }


  // done
  return LUX_TRUE;
}



// lightpos in obj.space
// pointer is volatile in renderbuffer
static EdgeVerts_t* ShadowMesh_getSilhouette(ShadowMesh_t *smesh,int *outcount)
{
  EdgeVerts_t* siledges;
  EdgeVerts_t* edgevert;
  EdgeList_t *edge;
  float dotA,dotB;
  float *lightpos;

  int i;

  lightpos = smesh->lightpos;

  edgevert = siledges = rpoolmalloc(sizeof(EdgeVerts_t)*smesh->numEdges);
  LUX_PROFILING_OP_MAX(g_Profiling.perframe.scratch.renderpoolmax,rpoolgetinuse());

  edge = smesh->edges;
  for (i = 0; i < smesh->numEdges; i++,edge++){
    int dotcmp;
    // check for silhouette
    dotA = lxVector4Dot(edge->planeA,lightpos);
    dotB = lxVector4Dot(edge->planeB,lightpos);
    dotcmp = LUX_FP_LESS_ZERO(dotA);
    if ( (dotcmp) ^ (LUX_FP_LESS_ZERO(dotB)) ){
      if (dotcmp){
        edgevert->vertA = edge->edge.vertB;
        edgevert->vertB = edge->edge.vertA;
      }
      else{
        edgevert->verts = edge->edge.verts;
      }
      edgevert++;
    }
  }

  *outcount = edgevert-siledges;

  return siledges;
}

static void ShadowMesh_buildVolumeCPU(ShadowMesh_t *smesh, EdgeVerts_t *everts, int numEVerts, ShadowMeshType_t type,void *outvertices)
{
  ushort  *outindices;
  uint a,b,c;
  float *invertsarray;
  float *outvertsarray;
  float *invert;
  float *outvert;
  int   invertstride;
  ushort  *primindex;
  int   primstride;
  int   i;
  int   index;
  int   quad;
  lxVector3 lposinv;
  int   triscnt;
  float *faceplane;
  float *lightpos = smesh->lightpos;
  int   allverticesextruded = LUX_FALSE;
  int   refvcount = smesh->refmesh->numVertices;
  uint  imin = 0xFFFFFFFF;
  uint  imax = 0;

  void  *vertices = outvertices ? outvertices : smesh->refmesh->origVertexData;

  invertstride = VertexSize(smesh->refmesh->vertextype)/sizeof(float);
  invertsarray = VertexArrayPtr(vertices,0,smesh->refmesh->vertextype,VERTEX_POS);

  LUX_ASSERT(VertexValue(smesh->refmesh->vertextype,VERTEX_SCALARPOS)==LUX_SCALAR_FLOAT32);

  outvert = outvertsarray = (float*)smesh->mesh->vertexData + (refvcount*4);
  outindices = smesh->mesh->indicesData16;

  lxVector3Negate(lposinv,lightpos);

  // zpass = only extrude needed (uncap or collapse depending on light)
  // zfail =  capped

  if (outvertices || smesh->lastvertices != vertices){
    // we have to refill original stuff
    for (i = 0 ; i < refvcount; i++,outvert+=4){
      invert = invertsarray+(invertstride*(i));
      lxVector3Copy(outvert,invert);
      outvert[3] = 1.0f;
    }
  }

  smesh->meshtype = type;
  smesh->lastvertices = vertices;

  smesh->mesh->primtype = GL_TRIANGLES;
  smesh->mesh->numIndices = 0;
  smesh->mesh->numTris = 0;
  smesh->mesh->numVertices = 0;

  index = 0;
  switch(type)
  {
  case SHADOWMSH_TYPE_CAP:
    quad = LUX_FALSE;
    if(smesh->refmesh->primtype == PRIM_TRIANGLES)
      primstride = 3;
    else if (smesh->refmesh->primtype == PRIM_TRIANGLE_STRIP)
      primstride = 1;
    else if (smesh->refmesh->primtype == PRIM_QUADS){
      primstride = 2; quad = LUX_TRUE;
    }

    primindex = smesh->refmesh->indicesData16;
    triscnt = smesh->refmesh->numTris;
    faceplane = smesh->faceplanes[0];

    allverticesextruded = LUX_TRUE;

    // OPTIMIZE back not needed for directional light, as all proj to one point
    // ie collapse

    // do the back at infinity
    if(smesh->extrusiondistance)
      for (i = 0 ; i < refvcount; i++,outvert+=4){
        invert = invertsarray+(invertstride*(i));
        lxVector3Sub(outvert,invert,lightpos);
        lxVector3NormalizedFast(outvert);
        lxVector3ScaledAdd(outvert,invert,smesh->extrusiondistance,outvert);
        outvert[3] = 1.0f;
      }
    else
      for (i = 0 ; i < refvcount; i++,outvert+=4){
        invert = invertsarray+(invertstride*(i));
        lxVector3ScaledAdd(outvert,lposinv,lightpos[3],invert);
        outvert[3] = 0.0f;
      }

    // fill in faces, reversed order
    for (i = 0; i < triscnt; i++,faceplane+=4,primindex+=primstride,outindices+=3){
      if (quad && i%2){
        a = *(primindex-2);
        b = *(primindex);
        c = *(primindex+1);
      }
      else{
        a = *(primindex);
        b = *(primindex+1);
        c = *(primindex+2);
      }
      if (lxVector4Dot(lightpos,faceplane) < 0 ){
        *(outindices) = c + refvcount;
        *(outindices+1) = b + refvcount;
        *(outindices+2) = a + refvcount;
      }
      else{
        *(outindices) = c;
        *(outindices+1) = b;
        *(outindices+2) = a;
      }
    }
    smesh->mesh->numTris = triscnt;
    smesh->mesh->numIndices = triscnt*3;
    smesh->mesh->numVertices = refvcount*2;
    // now do the uncapped stuff
  case SHADOWMSH_TYPE_UNCAP:
    // internal error prevention of buffer overflow
    if (smesh->mesh->numIndices + numEVerts*6 > smesh->mesh->numAllocIndices)
      break;
    if (allverticesextruded){
      // we had caps before, which means we have
      // transformed all vertices
      for(i = 0; i < numEVerts; i++,everts++,outindices+=6){
        *(outindices) = everts->vertA;
        *(outindices+1) = everts->vertB;
        *(outindices+2) = everts->vertB + refvcount;
        *(outindices+3) = everts->vertB + refvcount;
        *(outindices+4) = everts->vertA + refvcount;
        *(outindices+5) = everts->vertA;
      }
    }
    else{
      // pump edge tris individually
      // only calc "far" vertices (we will double calc, but doenst matter)

      if(smesh->extrusiondistance){
        // TODO version for directional lights
        for(i = 0; i < numEVerts; i++,everts++,outindices+=6){
          invert = invertsarray+(invertstride*everts->vertA);
          outvert = outvertsarray + (everts->vertA*4);

          lxVector3Sub(outvert,invert,lightpos);
          lxVector3NormalizedFast(outvert);
          lxVector3ScaledAdd(outvert,invert,smesh->extrusiondistance,outvert);
          outvert[3] = 1.0f;

          invert = invertsarray+(invertstride*everts->vertB);
          outvert = outvertsarray + (everts->vertB*4);

          lxVector3Sub(outvert,invert,lightpos);
          lxVector3NormalizedFast(outvert);
          lxVector3ScaledAdd(outvert,invert,smesh->extrusiondistance,outvert);
          outvert[3] = 1.0f;


          *(outindices) = everts->vertA;
          *(outindices+1) = everts->vertB;
          *(outindices+2) = everts->vertB + refvcount;
          *(outindices+3) = everts->vertB + refvcount;
          *(outindices+4) = everts->vertA + refvcount;
          *(outindices+5) = everts->vertA;
        }
      }
      else{
        for(i = 0; i < numEVerts; i++,everts++,outindices+=6){
          invert = invertsarray+(invertstride*everts->vertA);
          outvert = outvertsarray + (everts->vertA*4);

          lxVector3ScaledAdd(outvert,lposinv,lightpos[3],invert);
          outvert[3] = 0.0f;

          invert = invertsarray+(invertstride*everts->vertB);
          outvert = outvertsarray + (everts->vertB*4);

          lxVector3ScaledAdd(outvert,lposinv,lightpos[3],invert);
          outvert[3] = 0.0f;


          *(outindices) = everts->vertA;
          *(outindices+1) = everts->vertB;
          *(outindices+2) = everts->vertB + refvcount;
          *(outindices+3) = everts->vertB + refvcount;
          *(outindices+4) = everts->vertA + refvcount;
          *(outindices+5) = everts->vertA;
        }
      }

      smesh->mesh->numTris += numEVerts*2;
      smesh->mesh->numIndices = smesh->mesh->numTris*3;
    }
    break;
  case SHADOWMSH_TYPE_COLLAPSE:
    lxVector4Scale(outvert,lightpos,-1.0f);
    index = 1;
    for(i = 0; i < numEVerts; i++,everts++,outindices+=3){
      *outindices = refvcount;
      *(outindices+1) = everts->vertA;
      *(outindices+2) = everts->vertB;
    }

    smesh->mesh->numIndices = numEVerts*3;
    smesh->mesh->numTris = numEVerts;
    break;
  }

  outindices = smesh->mesh->indicesData16;
  triscnt = smesh->mesh->numTris;
  for (i = 0; i < triscnt; i++,outindices+=3){
    ushort index = outindices[0];
    imax = LUX_MAX(imax,index);
    imin = LUX_MIN(imin,index);

    index = outindices[1];
    imax = LUX_MAX(imax,index);
    imin = LUX_MIN(imin,index);

    index = outindices[2];
    imax = LUX_MAX(imax,index);
    imin = LUX_MIN(imin,index);
  }

  smesh->mesh->indexMin = imin;
  smesh->mesh->indexMax = imax;
  smesh->mesh->numVertices = imax+1;

}

  // vertices may be NULL, if original are used
static void ShadowMesh_update(ShadowMesh_t *smesh, Light_t *light, lxMatrix44 objmatrix, ShadowMeshType_t type, void *vertices, lxVector3 renderscale)
{
  // check if light moved or if it needs zfail
  // build volume
  int edgecount;
  EdgeVerts_t *everts;
  lxVector4 lpos;
  lxVector3 diff;

  lxVector3InvTransform(lpos,light->pos,objmatrix);
  lpos[3] = light->pos[3];
  //invscale
  if (renderscale){
    lxVector3Div(lpos,lpos,renderscale);
    if (smesh->extrusiondistance){
      smesh->extrusiondistance/=lxVector3LengthFast(renderscale);
    }
  }


  lxVector3Sub(diff,lpos,smesh->lightpos);


  // we can keep all as it was
  if (type == smesh->meshtype &&  lxVector3SqLength(diff) < 0.000001 && fabs(smesh->extrusiondistance-smesh->lastextrusion) < 0.00001)
    return;

  lxVector4Copy(smesh->lightpos,lpos);
  smesh->lastextrusion = smesh->extrusiondistance;

  everts = ShadowMesh_getSilhouette(smesh,&edgecount);
  ShadowMesh_buildVolumeCPU(smesh,everts,edgecount,type,vertices);
  rpoolsetbegin(everts);
}

//////////////////////////////////////////////////////////////////////////
// ShadowModel

static int  ShadowModel_needCap(ShadowModel_t *smdl, Camera_t *cam)
{
  // we need cap if we are within the pyramid that is enclosed by light and nearplane
  lxVector4 obj;
  lxVector4 light;
  lxVector4 nearp;
  float temp;
  lxVector4 corners[4];
  lxVector4 planes[5];
  float disttonear;
  int i;

  // first check if camera is on farside of plane obj->light
  lxVector3Sub(obj,smdl->light->pos,smdl->bsphere.center);
  lxVector3NormalizedFast(obj);
  obj[3] = -lxVector3Dot(obj,smdl->bsphere.center);
  lxVector3Copy(light,cam->pos);
  light[3] = 1.0f;
  if (lxVector4Dot(obj,light) > 0.0f){
    return LUX_FALSE;
  }

  lxVector3Copy(obj,smdl->bsphere.center);
  obj[3] = 1.0f;
  lxVector4Copy(light,smdl->light->pos);



  /*
  lxPlaneSet(planes[0],light, corners[0], corners[1]);
  lxPlaneSet(planes[1],light, corners[1], corners[2]);
  lxPlaneSet(planes[2],light, corners[2], corners[3]);
  lxPlaneSet(planes[3],light, corners[3], corners[0]);
  lxPlaneSet(planes[4],corners[0], corners[2], corners[1]);
  */
  lxVector4Copy(nearp,cam->fwd);
  nearp[3] = lxVector3Dot(nearp,cam->pos);

  disttonear = lxVector3Dot(light,nearp);

  if (disttonear > -LUX_FLOAT_EPSILON && disttonear < LUX_FLOAT_EPSILON){
    return fabs(lxVector4Dot(nearp,obj))  <= smdl->bsphere.radius;
  }
  else{
    lxVector4Set(corners[0],1,1,-1,1);
    lxVector4Transform1(corners[0],cam->mviewprojinv);
    temp = 1/corners[0][3];
    lxVector3Scale(corners[0],corners[0],temp);

    lxVector4Set(corners[1],-1,1,-1,1);
    lxVector4Transform1(corners[1],cam->mviewprojinv);
    temp = 1/corners[1][3];
    lxVector3Scale(corners[1],corners[1],temp);

    lxVector4Set(corners[2],-1,-1,-1,1);
    lxVector4Transform1(corners[2],cam->mviewprojinv);
    temp = 1/corners[2][3];
    lxVector3Scale(corners[2],corners[2],temp);

    lxVector4Set(corners[3],1,-1,-1,1);
    lxVector4Transform1(corners[3],cam->mviewprojinv);
    temp = 1/corners[3][3];
    lxVector3Scale(corners[3],corners[3],temp);

    if (disttonear >= LUX_FLOAT_EPSILON){
      lxPlaneSet(planes[0],light, corners[0], corners[1]);
      lxPlaneSet(planes[1],light, corners[1], corners[2]);
      lxPlaneSet(planes[2],light, corners[2], corners[3]);
      lxPlaneSet(planes[3],light, corners[3], corners[0]);
      lxPlaneSet(planes[4],corners[0], corners[2], corners[1]);
    }
    else{
      lxPlaneSet(planes[0],light, corners[1], corners[0]);
      lxPlaneSet(planes[1],light, corners[2], corners[1]);
      lxPlaneSet(planes[2],light, corners[3], corners[2]);
      lxPlaneSet(planes[3],light, corners[0], corners[3]);
      lxPlaneSet(planes[4],corners[0], corners[1], corners[2]);
    }
  }



  // check if object intersects volume
  for (i = 0; i < 5; i++){
    if (lxVector4Dot(planes[i],obj) < -smdl->bsphere.radius)
      return LUX_FALSE;
  }
  //prd(g_VID.frameCnt);
  return LUX_TRUE;

}
/*
static int  ShadowModel_needCap(ShadowModel_t *smdl, Camera_t *cam)
{
  // we need cap if we are within the pyramid that is enclosed by light and nearplane
  Vector4 obj;
  Vector4 light;
  Vector4 nearp;
  float disttonear;
  Vector3 corners[4];
  Vector4 planes[5];
  int i;

  // first check if camera is on farside of plane obj->light
  Vector3Sub(obj,smdl->light->pos,smdl->bsphere.center);
  Vector3Normalized(obj);
  obj[3] = -Vector3Dot(obj,smdl->bsphere.center);
  Vector3Copy(light,cam->pos);
  light[3] = 1.0f;
  if (Vector4Dot(obj,light) > 0.0f){
    return FALSE;
  }

  obj[3] = 1.0f;
  Vector4Transform(obj,smdl->bsphere.center,cam->mviewproj);

  Vector4Transform(light,smdl->light->pos,cam->mviewproj);
//  light[3] = smdl->light->pos[3];

  Vector4Set(nearp,0,0,-1,-1);

  obj[3] = 1/obj[3];
  Vector3Scale(obj,obj,obj[3]);
  obj[3] = 1;

  light[3] = 1/light[3];
  Vector3Scale(light,light,light[3]);
  light[3] = 1;
  disttonear = Vector4Dot(nearp,light);


  if (disttonear > -LUX_FLOAT_EPSILON && disttonear < LUX_FLOAT_EPSILON){
    return fabs(Vector4Dot(nearp,obj))  <= smdl->bsphere.radius;
  }
  else{
    Vector3Set(corners[0],1,1,-1);
    Vector3Set(corners[1],-1,1,-1);
    Vector3Set(corners[2],-1,-1,-1);
    Vector3Set(corners[3],1,-1,-1);

    if (disttonear >= LUX_FLOAT_EPSILON){
      lxPlaneSet(planes[0],light, corners[0], corners[1]);
      lxPlaneSet(planes[1],light, corners[1], corners[2]);
      lxPlaneSet(planes[2],light, corners[2], corners[3]);
      lxPlaneSet(planes[3],light, corners[3], corners[0]);
      lxPlaneSet(planes[4],corners[0], corners[2], corners[1]);
    }
    else{
      lxPlaneSet(planes[0],light, corners[1], corners[0]);
      lxPlaneSet(planes[1],light, corners[2], corners[1]);
      lxPlaneSet(planes[2],light, corners[3], corners[2]);
      lxPlaneSet(planes[3],light, corners[0], corners[3]);
      lxPlaneSet(planes[4],corners[0], corners[1], corners[2]);
    }
  }


  // check if object intersects volume
  for (i = 0; i < 5; i++){
    if (Vector4Dot(planes[i],obj) < -smdl->bsphere.radius)
      return FALSE;
  }
  return TRUE;

}
*/
int ShadowModel_visible(ShadowModel_t *smdl,lxMatrix44 mat,Camera_t *cam)
{
  lxVector3 ldir;
  lxVector4 volpoints[16];
  List3DNode_t *l3d;
  float *vecOut;
  float *vecIn;
  int i;

  if(!Reference_get(smdl->lightref,l3d) || !Reference_get(smdl->targetref,smdl->targetl3d))
    return LUX_FALSE;

  if (smdl->targettype == LUXI_CLASS_L3D_PRIMITIVE && smdl->targetmesh != smdl->targetl3d->drawNodes->draw.mesh)
    return LUX_FALSE;

  smdl->light = l3d->light;

  // if model is outside of lightrange or extrusion volume does not intersect frustum
  lxVector3Transform(smdl->bsphere.center,smdl->origcenter,mat);
  lxVector3Sub(ldir,smdl->light->pos,smdl->bsphere.center);
  if (smdl->light->range && lxVector3Dot(ldir,ldir) > smdl->light->rangeSq)
    return LUX_FALSE;

  // check extruded bbox against frustum
  // fill with bbox corners
  vecOut = volpoints[0];
  vecIn = smdl->bpoints[0];
  for (i = 0; i < 8; i++,vecOut+=4, vecIn+=3){
    lxVector3Add(vecOut,vecIn,smdl->bsphere.center);
    vecOut[3] = 1.0f;
  }
  // do extrusion
  vecIn = volpoints[0];
  if (smdl->extrusiondistance){
    for (i = 0; i < 8; i++,vecOut+=4, vecIn+=4){
      lxVector3Sub(vecOut,vecIn,smdl->light->pos);
      lxVector3NormalizedFast(vecOut);
      lxVector3ScaledAdd(vecOut,vecIn,smdl->extrusiondistance,vecOut);
      vecOut[3] = 1.0f;
    }
  }
  else{
    for (i = 0; i < 8; i++,vecOut+=4, vecIn+=4){
      if (smdl->light->pos[3] > 0)
        lxVector3Sub(vecOut,vecIn,smdl->light->pos);
      else
        lxVector3Negate(vecOut,smdl->light->pos);

      vecOut[3] = 0.0f;
    }
  }


  // was defaulted to TRUE
  return lxFrustum_cullPoints(&cam->frustum,volpoints,16);
}

// need to find out where we are and what type of shadowmeshes to use
// dont call if invisible
int ShadowModel_updateMeshes(ShadowModel_t *smdl,struct List3DNode_s *l3dself, Camera_t *cam)
{
  int i;
  List3DNode_t *l3d;
  ShadowMeshType_t type;
  ShadowMesh_t *smesh;
  DrawNode_t  *dnode;
  DrawNode_t  *dnodeself;
  void    *vertexdata;
  float   *pMat;
  lxMatrix44  mat;

  l3d = smdl->targetl3d;



  // find out if we need capping else do uncapped
  if (ShadowModel_needCap(smdl,cam)){
    type = SHADOWMSH_TYPE_CAP;
  }
  else
    type = SHADOWMSH_TYPE_UNCAP;

  smesh = smdl->shadowmeshes;
  dnodeself = l3dself->drawNodes;

  switch(smdl->targettype){
  case LUXI_CLASS_L3D_MODEL:

    for (i = 0; i < smdl->numShadowmeshes; i++,smesh++,dnodeself++){
      dnode = &l3d->drawNodes[smesh->refindex];
      // if skinned we gotta make sure we get proper vertexdata
      if (dnode->skinobj)
        vertexdata = SkinObject_run(dnode->skinobj,dnode->overrideVertices ? dnode->overrideVertices : dnode->draw.mesh->vertexData,dnode->draw.mesh->vertextype,LUX_FALSE);
      else if (dnode->overrideVertices)
        vertexdata = dnode->overrideVertices;
      else
        vertexdata = NULL;

      if (vertexdata && vertexdata != dnode->draw.mesh->origVertexData)
        ShadowMesh_updateFaceplanes(smesh,vertexdata);

      if (dnode->bonematrix){
        lxMatrix44MultiplySIMD(mat,l3dself->finalMatrix,dnode->bonematrix);
        pMat = mat;
      }
      else
        pMat = l3dself->finalMatrix;

      smesh->extrusiondistance = smdl->extrusiondistance;
      ShadowMesh_update(smesh,smdl->light,pMat,type,vertexdata,dnodeself->renderscale);

      if (type == SHADOWMSH_TYPE_CAP)
        dnodeself->draw.matRID = LUX_TRUE;
      else
        dnodeself->draw.matRID = LUX_FALSE;
      dnodeself->sortkey.value = BIT_ID_FULL32 - dnodeself->draw.matRID;
    }
    break;
  case LUXI_CLASS_L3D_PRIMITIVE:
    smesh->extrusiondistance = smdl->extrusiondistance;
    ShadowMesh_update(smesh,smdl->light,l3dself->finalMatrix,type,NULL,dnodeself->renderscale);

    if (type == SHADOWMSH_TYPE_CAP)
      dnodeself->draw.matRID = LUX_TRUE;
    else
      dnodeself->draw.matRID = LUX_FALSE;
    dnodeself->sortkey.value = BIT_ID_FULL32 - dnodeself->draw.matRID;
    break;
  default:
    return LUX_FALSE;
  }


  return LUX_TRUE;
}

void ShadowModel_updateExtrusionLength(ShadowModel_t *smdl)
{
  int i;
  ShadowMesh_t *smesh;

  smesh = smdl->shadowmeshes;
  for (i = 0; i < smdl->numShadowmeshes; i++,smesh++){
    smesh->extrusiondistance = smdl->extrusiondistance;
  }
}


void ShadowModel_free(ShadowModel_t *smdl)
{
  int i;
  ShadowMesh_t *smesh;

  smesh = smdl->shadowmeshes;
  for (i = 0; i < smdl->numShadowmeshes; i++,smesh++){
    if (smesh->edges)
      lxMemGenFree(smesh->edges,smesh->numEdges*sizeof(EdgeList_t));
    if (smesh->faceplanes)
      lxMemGenFree(smesh->faceplanes,smesh->numFaceplanes*sizeof(lxVector4));
    if (smesh->mesh)
      Mesh_free(smesh->mesh);
  }


  Reference_releaseWeakCheck(smdl->lightref);
  Reference_releaseWeakCheck(smdl->targetref);

  lxMemGenFree(smdl->shadowmeshes,sizeof(ShadowMesh_t)*smdl->numShadowmeshes);
  lxMemGenFree(smdl,sizeof(ShadowModel_t));
}


// sets up all data, returns true if model was closed
// we do a strstr if target is model and only pick meshes wiht the nameflag
// it can return NULL, if meshes were not found or not closed
// also sets boundingsphere
ShadowModel_t* ShadowModel_new(const struct List3DNode_s *l3dtarget,const struct List3DNode_s *lightl3d,const char *meshnameflag)
{
  ShadowModel_t* smdl;
  ShadowMesh_t *smesh;
  lxBoundingBox_t bbox;
  Model_t *mdl;
  int i;

  bufferclear();

  switch(l3dtarget->nodeType)
  {
  case LUXI_CLASS_L3D_MODEL:
    smdl = lxMemGenZalloc(sizeof(ShadowModel_t));
    mdl = ResData_getModel(l3dtarget->minst->modelRID);

    for (i=0; i < mdl->numMeshObjects; i++){
      if (!meshnameflag || strstr(meshnameflag,mdl->meshObjects[i].name))
        smdl->numShadowmeshes++;
    }
    smdl->shadowmeshes = lxMemGenZalloc(sizeof(ShadowMesh_t)*smdl->numShadowmeshes);
    smesh = smdl->shadowmeshes;
    for (i=0; i < mdl->numMeshObjects; i++){
      if (!meshnameflag || strstr(meshnameflag,mdl->meshObjects[i].name)){
        if (!ShadowMesh_init(smesh,mdl->meshObjects[i].mesh)){
          ShadowModel_free(smdl);
          return NULL;
        }
        smesh->refindex = i;
        smesh++;
      }
    }
    smdl->lightref = lightl3d->reference;
    smdl->targetref = l3dtarget->reference;

    Reference_refWeak(smdl->lightref);
    Reference_refWeak(smdl->targetref);

    smdl->targettype = LUXI_CLASS_L3D_MODEL;
    lxBoundingBox_toSphere(&ResData_getModel(l3dtarget->minst->modelRID)->bbox,&smdl->bsphere);
    lxVector3Copy(smdl->origcenter,smdl->bsphere.center);
    lxVector3Set(smdl->bpoints[0],-smdl->bsphere.radius,-smdl->bsphere.radius,-smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[1],-smdl->bsphere.radius,-smdl->bsphere.radius,smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[2],-smdl->bsphere.radius,smdl->bsphere.radius,-smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[3],-smdl->bsphere.radius,smdl->bsphere.radius,smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[4],smdl->bsphere.radius,-smdl->bsphere.radius,-smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[5],smdl->bsphere.radius,-smdl->bsphere.radius,smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[6],smdl->bsphere.radius,smdl->bsphere.radius,-smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[7],smdl->bsphere.radius,smdl->bsphere.radius,smdl->bsphere.radius);
    return smdl;
    break;
  case LUXI_CLASS_L3D_PRIMITIVE:
    smdl = lxMemGenZalloc(sizeof(ShadowModel_t));
    smdl->targetref = l3dtarget->reference;
    smdl->targetmesh = l3dtarget->drawNodes->draw.mesh;
    smdl->numShadowmeshes = 1;
    smdl->shadowmeshes = lxMemGenZalloc(sizeof(ShadowMesh_t));
    if (!ShadowMesh_init(smdl->shadowmeshes,smdl->targetmesh)){
      ShadowModel_free(smdl);
      return NULL;
    }
    smdl->lightref = lightl3d->reference;

    Reference_refWeak(smdl->lightref);
    Reference_refWeak(smdl->targetref);

    smdl->targettype = LUXI_CLASS_L3D_PRIMITIVE;
    VertexArray_minmax(smdl->targetmesh->vertexData,smdl->targetmesh->vertextype,smdl->targetmesh->numVertices,bbox.min,bbox.max);
    if (l3dtarget->drawNodes->renderscale){
      lxVector3Mul(bbox.min,bbox.min,l3dtarget->drawNodes->renderscale);
      lxVector3Mul(bbox.max,bbox.max,l3dtarget->drawNodes->renderscale);
    }

    lxBoundingBox_toSphere(&bbox,&smdl->bsphere);
    lxVector3Copy(smdl->origcenter,smdl->bsphere.center);
    lxVector3Set(smdl->bpoints[0],-smdl->bsphere.radius,-smdl->bsphere.radius,-smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[1],-smdl->bsphere.radius,-smdl->bsphere.radius,smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[2],-smdl->bsphere.radius,smdl->bsphere.radius,-smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[3],-smdl->bsphere.radius,smdl->bsphere.radius,smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[4],smdl->bsphere.radius,-smdl->bsphere.radius,-smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[5],smdl->bsphere.radius,-smdl->bsphere.radius,smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[6],smdl->bsphere.radius,smdl->bsphere.radius,-smdl->bsphere.radius);
    lxVector3Set(smdl->bpoints[7],smdl->bsphere.radius,smdl->bsphere.radius,smdl->bsphere.radius);
    return smdl;
    break;
  default:
    return NULL;
  }
}

//////////////////////////////////////////////////////////////////////////
// LevelSplit

static void LevelSplitNode_merge(LevelSplitNode_t *keep, LevelSplitNode_t *merge)
{
  LevelSplitNodeMesh_t *spnodemesh;
  LevelSplitNodeMesh_t *spnodemeshkeep;
  LevelSplitNodeMesh_t *addlist;
  ushort *newindices;
  int found;

  // the basics
  lxBoundingBox_merge(&keep->bbox.bbox,&keep->bbox.bbox,&merge->bbox.bbox);
  lxBoundingBox_merge(&keep->bboxverts.bbox,&keep->bboxverts.bbox,&merge->bboxverts.bbox);
  keep->numTris += merge->numTris;

  addlist = NULL;
  // run thru all merge's spnodemeshes
  while (merge->meshListHead){
    lxListNode_popFront(merge->meshListHead,spnodemesh);

    found = LUX_FALSE;
    // check if same source mesh is already in keep
    lxListNode_forEach(keep->meshListHead,spnodemeshkeep)
      if (spnodemeshkeep->spmesh == spnodemesh->spmesh){
        found = LUX_TRUE;
        break;
      }
    lxListNode_forEachEnd(keep->meshListHead,spnodemeshkeep);

    if (found){
      lxListNode_remove(keep->meshListHead,spnodemeshkeep);
      // merge
      newindices = buffermalloc(sizeof(ushort)*(spnodemesh->numIndices+spnodemeshkeep->numIndices));
      memcpy(newindices, spnodemesh->indices, sizeof(ushort)*spnodemesh->numIndices);
      memcpy(newindices+spnodemesh->numIndices,spnodemeshkeep->indices,sizeof(ushort)*spnodemeshkeep->numIndices);
      spnodemesh->indices = newindices;
      spnodemesh->numIndices += spnodemeshkeep->numIndices;

    }

    lxListNode_addLast(addlist,spnodemesh);
  }
  if (!keep->meshListHead)
    keep->meshListHead = addlist;
  else if(addlist){
    lxListNode_append(keep->meshListHead,addlist);
  }
}

static void LevelSplit_run(LevelSplit_t *split,
               int startdimension[3],int minpolycount,
               LevelBoundingBox_t *origbbox,
               float mergemargin)
{
  int x,y,z,i,n;
  int dimension[3];
  LevelSplitNode_t *spnode;
  LevelSplitNode_t *spnodefind;
  LevelSplitNodeMesh_t *spnodemesh;
  LevelSplitMesh_t *spmesh;
  LevelSplitNode_t *spnodelist;
  lxVector3 scale;
  lxVector3 curscale;
  lxVector3 min,max;
  float *posstart;
  int posstride;
  Mesh_t *mesh;
  ushort *curOut;
  ushort *curIn;
  int start = LUX_TRUE;

  LUX_ARRAY3COPY(dimension,startdimension);
  do{
    if (!start){
      dimension[0]/=2;
      dimension[0] = LUX_MAX(dimension[0],1);
      dimension[1]/=2;
      dimension[1] = LUX_MAX(dimension[1],1);
      dimension[2]/=2;
      dimension[2] = LUX_MAX(dimension[2],1);
    }
    else{
      start = LUX_FALSE;
    }

    scale[0] = origbbox->bctr.size[0]/(float)dimension[0];
    scale[1] = origbbox->bctr.size[1]/(float)dimension[1];
    scale[2] = origbbox->bctr.size[2]/(float)dimension[2];

    x = y = z = 0;

    while ((x < dimension[0] || y < dimension[1]) && z < dimension[2]){
      LevelBoundingBox_t checkbox;
      lxVector3 margin = {mergemargin,mergemargin,mergemargin};

      // create node
      spnode = buffermalloc(sizeof(LevelSplitNode_t));
      lxListNode_init(spnode);
      lxListNode_addLast(split->nodeListHead,spnode);
      spnode->meshListHead = NULL;
      spnode->numTris = 0;

      // set up bbox
      lxVector3Set(curscale,x,y,z);
      lxVector3MulAdd(spnode->bbox.bbox.min,origbbox->bbox.min,curscale,scale);
      lxVector3Add(spnode->bbox.bbox.max,spnode->bbox.bbox.min,scale);

      checkbox = spnode->bbox;
      lxBoundingBox_toCenterBox(&checkbox.bbox,&checkbox.bctr);
      lxVector3Add(checkbox.bctr.size,checkbox.bctr.size,margin);
      lxBoundingBox_fromCenterBox(&checkbox.bbox,&checkbox.bctr);

      lxVector3Set(max,-FLT_MAX,-FLT_MAX,-FLT_MAX);
      lxVector3Set(min,FLT_MAX,FLT_MAX,FLT_MAX);

      // now fill with all meshes that lie within
      spmesh = split->splitmeshes;
      for (n = 0; n < split->numSplitmeshes; n++,spmesh++){
        mesh = spmesh->meshobj->mesh;

        spnodemesh = buffermalloc(sizeof(LevelSplitNodeMesh_t));
        spnodemesh->numIndices = 0;
        spnodemesh->indices = buffermalloc(sizeof(ushort)*mesh->numIndices);

        LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

        posstride = VertexSize(mesh->vertextype) / sizeof(float);
        posstart = VertexArrayPtr(mesh->vertexData,0,mesh->vertextype,VERTEX_POS);

        LUX_ASSERT(VertexValue(mesh->vertextype,VERTEX_SCALARPOS)==LUX_SCALAR_FLOAT32);

        // check if triangles are within
        curIn = spmesh->indices;
        curOut = spnodemesh->indices;
        for (i = 0; i < mesh->numIndices/3; i++,curIn+=3){
          if (curIn[0] != LUX_SHORT_UNSIGNEDMAX){
            float* va = posstart+(posstride*curIn[0]);
            float* vb = posstart+(posstride*curIn[1]);
            float* vc = posstart+(posstride*curIn[2]);
            lxVector3 ctr = {va[0],va[1],va[2]};

            lxVector3Add(ctr,ctr,vb);
            lxVector3Add(ctr,ctr,vc);
            lxVector3Scale(ctr,ctr,1.0f/3.0f);

            if (lxBoundingBox_checkPoint(&spnode->bbox.bbox,vc) &&
              lxBoundingBox_checkPoint(&checkbox.bbox,va) &&
              lxBoundingBox_checkPoint(&checkbox.bbox,vb) &&
              lxBoundingBox_checkPoint(&checkbox.bbox,vc)){

              lxVector3MinMax(min,max,va);
              lxVector3MinMax(min,max,vb);
              lxVector3MinMax(min,max,vc);

              LUX_ARRAY3COPY(curOut,curIn);
              LUX_ARRAY3SET(curIn,LUX_SHORT_UNSIGNEDMAX,LUX_SHORT_UNSIGNEDMAX,LUX_SHORT_UNSIGNEDMAX);
              curOut+=3;
            }
          }
        }

        spnodemesh->numIndices = curOut - spnodemesh->indices ;
        spnodemesh->spmesh = spmesh;

        if (!spnodemesh->numIndices){
          buffersetbegin(spnodemesh);
        }
        else{
          spnode->numTris += (spnodemesh->numIndices/3);
          buffersetbegin(spnodemesh->indices+spnodemesh->numIndices);
          lxListNode_init(spnodemesh);
          lxListNode_addLast(spnode->meshListHead,spnodemesh);
        }
      }

      lxVector3Copy(spnode->bboxverts.bbox.min,min);
      lxVector3Copy(spnode->bboxverts.bbox.max,max);

      x++;
      if (x >= dimension[0]){
        x = 0;
        y++;
      }
      if (y >= dimension[1]){
        y = 0;
        z++;
      }
    }

  }while (dimension[0] > 1 || dimension[1] > 1 || dimension[2] > 1);
  // now we need to optimize / compress
  start = LUX_TRUE;
  while (start){
    start = LUX_FALSE;
    spnodelist = split->nodeListHead;
    split->nodeListHead = NULL;
    // if the node has too few indices merge it with next
    while (spnodelist){
      lxListNode_popFront(spnodelist,spnode);
      if (spnode->numTris < minpolycount){
        // find a child we can merge with
        lxListNode_forEach(spnodelist,spnodefind)
          if (spnodefind->numTris < minpolycount && lxBoundingBox_intersect(&spnode->bboxverts.bbox,&spnodefind->bbox.bbox)){
            start = LUX_TRUE;
            break;
          }
        lxListNode_forEachEnd(spnodelist,spnodefind);
        // merge with find and remove find
        if (start){
          lxListNode_remove(spnodelist,spnodefind);
          LevelSplitNode_merge(spnode,spnodefind);
        }

      }
      lxListNode_addLast(split->nodeListHead,spnode);

    }
  }
}



static void LevelSplit_storeLevelModel(LevelSplit_t *split,LevelModel_t* lm)
{
  LevelSubNode_t  *subnodes;
  LevelSubMesh_t  *submeshes;
  LevelSplitNode_t *curspnode;
  LevelSplitNodeMesh_t *curspnodemesh;
  Mesh_t *mesh;
  SceneNode_t *curscn;
  int *nodelmcnt;

  int **usedLightmaps;
  int numUsedLightmaps;
  int i;
  int n;
  int l;
  int cnt;
  ushort *index;
  int *verts;
  int numverts;
  int offsetverts;



  // 1 root and 1 scenenode per splitnode if it has meshes
  lm->numScenenodes = 1;
  lxListNode_forEach(split->nodeListHead,curspnode)
    if (curspnode->meshListHead){
      lm->numScenenodes++;
    }
  lxListNode_forEachEnd(split->nodeListHead,curspnode);
  lm->scenenodes = lxMemGenZalloc(sizeof(SceneNode_t*)*lm->numScenenodes);
  // first is the rootnode
  lm->scenenodes[0] = SceneNode_new(LUX_FALSE,"_lux_lvlrt");

  // to check how many lightmaps are used per scenenode
  usedLightmaps = bufferzalloc(sizeof(int*)*lm->numScenenodes);

  for (i = 0; i < lm->numScenenodes; i++){
    usedLightmaps[i] = bufferzalloc(sizeof(int)*split->numSplitmeshes);
    usedLightmaps[i][0] = -1;
  }

  nodelmcnt = bufferzalloc(sizeof(int)*lm->numScenenodes);

  i = 0;
  lxListNode_forEach(split->nodeListHead,curspnode)
    if (curspnode->meshListHead){
      // create scenenode
      curscn = lm->scenenodes[i+1] = SceneNode_new(LUX_TRUE,"_lux_lvl");
      Reference_makeVolatile(curscn->link.reference);

      VisObject_setBBoxForce(curscn->link.visobject,curspnode->bboxverts.bbox.min,curspnode->bboxverts.bbox.max);
      // link all to root
      SceneNode_link(curscn,lm->scenenodes[0]);

      // find out how many submeshes / subnodes we need
      // one subnode per lightmap
      numUsedLightmaps = 0;
      lxListNode_forEach(curspnode->meshListHead,curspnodemesh)
        lxArrayFindOrAdd32(usedLightmaps[i],&numUsedLightmaps,curspnodemesh->spmesh->lightmapRID,split->numSplitmeshes);
        lm->numSubmeshes++;
      lxListNode_forEachEnd(curspnode->meshListHead,curspnodemesh);
      lm->numSubnodes += numUsedLightmaps > 1 ? numUsedLightmaps : 1;
      nodelmcnt[i] = numUsedLightmaps;
      i++;
    }
  lxListNode_forEachEnd(split->nodeListHead,curspnode);

  lm->subnodes = lxMemGenZalloc(sizeof(LevelSubNode_t)*lm->numSubnodes);
  lm->submeshes = lxMemGenZalloc(sizeof(LevelSubMesh_t)*lm->numSubmeshes);

  i = 0;
  subnodes = lm->subnodes;
  submeshes = lm->submeshes;
  lxListNode_forEach(split->nodeListHead,curspnode)
    if (!curspnode->meshListHead)
      lxListNode_forEachContinue(split->nodeListHead,curspnode);

    cnt = nodelmcnt[i] ? nodelmcnt[i] : 1;
    curscn = lm->scenenodes[i+1];
    for (l = 0; l < cnt ; l++,subnodes++){
      // even if nodelmcnt was 0 -1 is at [0]
      subnodes->drawenv.lightmapRID = usedLightmaps[i][l];
      subnodes->visobj = curscn->link.visobject;
    }
    lxListNode_forEach(curspnode->meshListHead,curspnodemesh)
      n = lxArrayFindOrAdd32(usedLightmaps[i],&cnt,curspnodemesh->spmesh->lightmapRID,split->numSplitmeshes);
      LUX_ASSERT(n >=0 );
      submeshes->subnode = subnodes-l+n;

      // copy mesh and indices
      mesh = &submeshes->mesh;
      memcpy(mesh,curspnodemesh->spmesh->meshobj->mesh,sizeof(Mesh_t));
      if (mesh->meshtype == MESH_VBO){
        // dont use VBO indices but our own
        mesh->vid.ibo = 0;
        mesh->vid.ibooffset = 0;
        offsetverts = mesh->vid.vbooffset;
      }
      else{
        mesh->meshtype = MESH_VA;
        offsetverts = 0;
      }
      mesh->numIndices = mesh->numAllocIndices = curspnodemesh->numIndices;
      mesh->indicesData16 = lxMemGenZalloc(sizeof(ushort)*mesh->numAllocIndices);
      memcpy(mesh->indicesData16,curspnodemesh->indices,sizeof(ushort)*curspnodemesh->numIndices);
      mesh->indexMax = 0;
      mesh->indexMin = BIT_ID_FULL16;
      mesh->index16 = LUX_TRUE;
      index = mesh->indicesData16;
      verts = buffermalloc(sizeof(int)*mesh->numIndices);
      LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

      numverts = 0;
      for (n = 0; n < mesh->numIndices; n++,index++){
        mesh->indexMin = LUX_MIN(mesh->indexMin,*index);
        mesh->indexMax = LUX_MAX(mesh->indexMax,*index);
        *index = *index + offsetverts;
        lxArrayFindOrAdd32(verts,&numverts,*index,mesh->numAllocVertices);
      }
      buffersetbegin(verts);
      mesh->numVertices = numverts;
      mesh->numTris = mesh->numIndices/3;
      submeshes->origMeshID = curspnodemesh->spmesh->origMeshID;
      submeshes++;
    lxListNode_forEachEnd(curspnode->meshListHead,curspnodemesh);
    i++;
  lxListNode_forEachEnd(split->nodeListHead,curspnode);
}

//////////////////////////////////////////////////////////////////////////
// LevelModel



void LevelModel_free(LevelModel_t *lmdl)
{
  int i;
  LevelSubMesh_t *submesh;

  //for (i = 0; i < lmdl->numScenenodes; i++){
  //  lmdl->scenenodes[i]->flag &= ~SCENE_NODE_LEVELMODEL;
  //}
  //RSceneNode_free(lmdl->scenenodes[0]->link.reference);


  if (lmdl->numScenenodes)
    lxMemGenFree(lmdl->scenenodes,sizeof(SceneNode_t*)*lmdl->numScenenodes);

  submesh = lmdl->submeshes;
  for (i =0; i < lmdl->numSubmeshes; i++,submesh++){
    lxMemGenFree(submesh->mesh.indicesData16,sizeof(ushort)*submesh->mesh.numAllocIndices);
  }
  if (lmdl->numSubmeshes)
    lxMemGenFree(lmdl->submeshes,sizeof(LevelSubMesh_t)*lmdl->numSubmeshes);

  if (lmdl->numSubnodes)
    lxMemGenFree(lmdl->subnodes,sizeof(LevelSubNode_t)*lmdl->numSubnodes);

  if (lmdl->numLightmapRIDs)
    lxMemGenFree(lmdl->lightmapRIDs,sizeof(int)*lmdl->numLightmapRIDs);

  lxMemGenFree(lmdl,sizeof(LevelModel_t));
}


LevelModel_t* LevelModel_new(
  int modelresid, int dimension[3],
  int minpolycount, float mergemargin)
{
  static char namebuffer[256];
  LevelBoundingBox_t lbbox;
  ResourceChunk_t *reschunk;
  Model_t *mdl;
  LevelSplit_t levelsplit;
  LevelSplitMesh_t *spmesh;
  LevelModel_t* lmdl;
  MeshObject_t *meshobj;
  char *pLightmapname;
  char *pBuffer;

  int *totalUsedLightmaps;
  int numTotalUsedLightmaps;
  int i;

  bufferclear();
  memset(&levelsplit,0,sizeof(LevelSplit_t));


  mdl = ResData_getModel(modelresid);
  meshobj = mdl->meshObjects;


  // only allow triangle lists
  for (i = 0; i < mdl->numMeshObjects; i++,meshobj++){
    if (meshobj->mesh->primtype != GL_TRIANGLES || !meshobj->mesh->index16)
      return NULL;
  }
  lmdl = lxMemGenZalloc(sizeof(LevelModel_t));
  lmdl->modelRID = modelresid;

  // when loading lightmaps make sure they are in same pool as model;
  reschunk = ResData_setChunkFromPtr(&mdl->resinfo);
  levelsplit.numSplitmeshes = mdl->numMeshObjects;

  totalUsedLightmaps = bufferzalloc(sizeof(int)*mdl->numMeshObjects);
  numTotalUsedLightmaps = 0;

  spmesh = levelsplit.splitmeshes = bufferzalloc(sizeof(LevelSplitMesh_t)*mdl->numMeshObjects);
  meshobj = mdl->meshObjects;

  // load lightmaps
  // and create indices copies
  for (i = 0; i < mdl->numMeshObjects; i++,meshobj++,spmesh++){
    spmesh->meshobj = meshobj;
    if (meshobj->texname && (pLightmapname=strstr(meshobj->texname,"lightmap_"))){
      pBuffer = namebuffer;
      while(*pLightmapname && *pLightmapname != ',' && *pLightmapname!= ';'){
        *pBuffer++ = *pLightmapname++;
      }
      *pBuffer = 0;
      spmesh->lightmapRID = ResData_addTexture(namebuffer,TEX_COLOR,TEX_ATTR_MIPMAP);
      lxArrayFindOrAdd32(totalUsedLightmaps,&numTotalUsedLightmaps,spmesh->lightmapRID,mdl->numMeshObjects);
    }
    else
      spmesh->lightmapRID = -1;

    if (meshobj->mesh->numAllocIndices){
      spmesh->indices = buffermalloc(sizeof(ushort)*meshobj->mesh->numAllocIndices);
      memcpy(spmesh->indices,meshobj->mesh->indicesData16,sizeof(ushort)*meshobj->mesh->numAllocIndices);
    }
    else
      spmesh->indices = NULL;
    spmesh->origMeshID = i;

  }
  ResourceChunk_activate(reschunk);
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

  if (numTotalUsedLightmaps){
    lmdl->lightmapRIDs = lxMemGenZalloc(sizeof(int)*numTotalUsedLightmaps);
    for (i = 0; i < numTotalUsedLightmaps; i++)
      lmdl->lightmapRIDs[i] = totalUsedLightmaps[i];
  }
  lmdl->numLightmapRIDs = numTotalUsedLightmaps;

  // now generate the base splitnodes and throw all indices in them that lie within
  lbbox.bbox = mdl->bbox;
  lxBoundingBox_toCenterBox(&mdl->bbox,&lbbox.bctr );
  LevelSplit_run(&levelsplit,dimension,minpolycount,&lbbox,mergemargin);


  // create outputs
  LevelSplit_storeLevelModel(&levelsplit,lmdl);

  return lmdl;
}

// preps visdata
void      LevelModel_updateL3D(struct List3DNode_s *l3dself)
{
  LevelModel_t *lmdl;
  LevelSubNode_t *subnode;
  LevelSubNode_t *lastsub;
  int bitID;
  List3DSet_t   *l3dset = &g_List3D.drawSets[l3dself->setID];
  int i;

  lmdl = l3dself->lmdl;
  subnode = lmdl->subnodes;
  lastsub = lmdl->subnodes+lmdl->numSubnodes;
  while (subnode < lastsub)
  {
    if (l3dself->enviro.needslight && (subnode->drawenv.numFxLights=subnode->vislightres->numLights))
      subnode->drawenv.fxLights = subnode->vislightres->lights;

    bitID = subnode->visset->projectorVis & l3dself->enviro.projectorflag;

    subnode->drawenv.numProjectors = 0;
    if (bitID){
      subnode->drawenv.projectors = g_List3D.projCurrent;
      for (i = 0; i < LIST3D_SET_PROJECTORS; i++){
        if (bitID & 1<<i){
          LUX_ASSERT(subnode->drawenv.numProjectors <= LIST3D_SET_PROJECTORS);
          subnode->drawenv.projectors[subnode->drawenv.numProjectors++] = l3dset->projLookUp[i];
          g_List3D.projCurrent++;
        }
      }
    }
    subnode++;
  }
  if (l3dself->update){
    SceneNode_setMatrix(lmdl->scenenodes[0],l3dself->finalMatrix);
  }

}

void LevelModel_updateVis(LevelModel_t *lmdl, struct List3DNode_s *l3dself)
{
  LevelSubNode_t *subnode = lmdl->subnodes;
  LevelSubNode_t *lastsubnode = subnode+lmdl->numSubnodes;
  SceneNode_t **pscn = lmdl->scenenodes+1;
  SceneNode_t **lastscn = lmdl->scenenodes + lmdl->numScenenodes;
  SceneNode_t *scn;

  VisSet_t *visset;

  while (pscn < lastscn){
    scn = *pscn;
    scn->link.visobject->cameraFlag = l3dself->visflag;
    visset = &scn->link.visobject->visset;
    visset->projectorFlag = l3dself->enviro.projectorflag;
    pscn++;
  }

  while (subnode < lastsubnode){
    subnode->visset = &subnode->visobj->visset;
    if (l3dself->enviro.needslight && !(l3dself->drawNodes->draw.state.renderflag & RENDER_SUNLIT)){
      subnode->vislightres = &subnode->visset->vislight->nonsun;
    }
    else{
      subnode->vislightres  = &subnode->visset->vislight->full;
    }

    subnode++;
  }
}

// returns NULL terminated ptr list
DrawNode_t**  LevelModel_getVisible(LevelModel_t *lmdl, struct List3DNode_s *l3dself ,Camera_t *cam)
{
  int i;
  int lightcnt;
  DrawNode_t  **dnodeslist;
  DrawNode_t  **dnodesout;
  DrawNode_t  *dnode;
  LevelSubMesh_t *submesh = lmdl->submeshes;
  float range = cam->backplane*cam->nearpercentage;
  range *= range;

  dnodeslist = dnodesout = rpoolmalloc(sizeof(DrawNode_t*)*(lmdl->numSubmeshes+1));
  LUX_PROFILING_OP_MAX(g_Profiling.perframe.scratch.renderpoolmax,rpoolgetinuse());

  dnode = l3dself->drawNodes;
  for (i = 0; i < lmdl->numSubmeshes; i++,dnode++,submesh++){
    if (submesh->subnode->visobj->cameraVis & cam->bitID){
      lightcnt = (dnode->draw.state.renderflag & RENDER_SUNLIT) ? 1 : 0;
      lightcnt = (dnode->draw.state.renderflag & RENDER_FXLIT) ? dnode->env->numFxLights+lightcnt*4 : lightcnt;
      if (lightcnt)
        dnode->draw.state.renderflag |= RENDER_LIT;
      else
        dnode->draw.state.renderflag &= ~RENDER_LIT;

      lightcnt <<= DRAW_SORT_SHIFT_LIGHTCNT;


      // we have 12 bits for renderflag, current length is 15, so shift back
      dnode->sortkey.value = dnode->sortID + lightcnt + ((dnode->draw.state.renderflag & (RENDER_LASTFLAG-1))>>3);

      if (lxVector3SqDistance(cam->pos,submesh->subnode->visobj->center) > range)
        dnode->sortkey.value |= DRAW_SORT_FLAG_FAR;

      *dnodesout++ = dnode;
    }
  }

  *dnodesout = NULL;

  return dnodeslist;
}

