// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../main/main.h"
#include "f3d.h"
#include "../common/3dmath.h"
#include "../fileio/filesystem.h"

/*
vertex64 old

typedef struct Vertex64_s{  // Warning gl_terrain depends on this order and f3d version 240
  float   tex[2];
  union{
    float tex2[2];
    short tangent[4];
  };
  Vector4SIMD user4;
  Vector3   normal;
  union{
    uchar   color[4];
    uint    colorc;
  };
  Vector3SIMD pos;
}Vertex64_t;
*/

static void fileLoadF3DMesh(uint type, uint count, uint offsetRID);
static void fileLoadF3DMaterial(uint count);
static void fileLoadF3DSkin(uint count);

// LOCALS
static struct F3DData_s{
  lxFSFile_t      *file;

  Model_t       *model;

  uint      meshesCount;
  uint      instance_1_Count;
  uint      instance_2_Count;
  uint      instance_3_Count;
  uint      helpersCount;

  uint      materialsCount;
  uint      skinsCount;

  uint      curbone;

  uint      totalMeshCount;
  int       f3dversion;
} l_F3DData;

typedef enum IPrimitiveType_e{
  IPRIM_POINTS,
  IPRIM_TRIANGLE_LIST,
  IPRIM_TRIANGLE_STRIP,
  IPRIM_QUAD_LIST,
  IPRIM_QUAD_STRIP,
  IPRIM_UNSET = 0xFFFF,
}IPrimitiveType;

typedef enum IMeshType_e{
  IMESH_MESH,
  IMESH_INST_SHRD_VERT,
  IMESH_INST_SHRD_VERT_PARENT_MAT,
  IMESH_INST_SHRD_VERT_TRIS,
  IMESH_HELPER,
  IMESH_COLLISION,
  IMESH_UNSET = 0xff
}IMeshType;

static PrimitiveType_t f3dPrimType(uint type){
  switch (type){
    case IPRIM_POINTS:
      return PRIM_POINTS;
    case IPRIM_TRIANGLE_LIST:
      return PRIM_TRIANGLES;
    case IPRIM_TRIANGLE_STRIP:
      return PRIM_TRIANGLE_STRIP;
    case IPRIM_QUAD_LIST:
      return PRIM_QUADS;
    case IPRIM_QUAD_STRIP:
      return PRIM_QUAD_STRIP;
    default:
      return PRIM_TRIANGLES;
  }
}

static InstanceType_t f3dInstanceType(uint type)
{
  switch (type) {
    case IMESH_MESH:
      return INST_NONE;
    case IMESH_INST_SHRD_VERT:
      return INST_SHRD_VERT;
    case IMESH_INST_SHRD_VERT_PARENT_MAT:
      return INST_SHRD_VERT_PARENT_MAT;
    case IMESH_INST_SHRD_VERT_TRIS:
      return INST_SHRD_VERT_TRIS;
    default:
      return INST_NONE;
  }
}

static uint f3dFixBoneID(uint id)
{
  // instance_2_Count has no bones
  if (!id)
    return id;
  if (id > l_F3DData.meshesCount+ l_F3DData.instance_1_Count +l_F3DData.instance_2_Count)
    return id-l_F3DData.instance_2_Count;
  else if (id <= l_F3DData.meshesCount + l_F3DData.instance_1_Count)
    return id;
  else{
    LUX_ASSERT(0);
    return 0;
  }
}


int fileLoadF3D(const char * filename, Model_t *model, void *unused)
{
  // model
  lxFSFile_t *file;
  uint  offsetID;
  ushort  ushort1;

  file = FS_open(filename);
  l_F3DData.file = file;
  if (!file){
    lprintf("ERROR f3dload: ");
    lnofile(filename);
    return LUX_FALSE;
  }
  else
    lprintf("Model:   \t%s\n",filename);

  lxFS_read(&ushort1,sizeof(ushort),1,file);
  if (ushort1 != F3D_HEADER){
    bprintf("ERROR f3dload: wrong header\n");
    FS_close(file);
    return LUX_FALSE;
  }
  // Version
  lxFS_read(&ushort1,sizeof(ushort),1,file);
  if (ushort1 != F3D_VERSION_23 && ushort1 != F3D_VERSION_24 && ushort1 != F3D_VERSION_25 && ushort1 != F3D_VERSION_26){
    bprintf("ERROR f3dload: wrong version\n");
    FS_close(file);
    return LUX_FALSE;
  }

  l_F3DData.f3dversion = ushort1;

  l_F3DData.curbone = 0;
  l_F3DData.model = model;

  // Meshes
  lxFS_read(&l_F3DData.meshesCount,sizeof(ushort),1,file);
  lxFS_read(&l_F3DData.instance_1_Count,sizeof(ushort),1,file);
  lxFS_read(&l_F3DData.instance_2_Count,sizeof(ushort),1,file);
  lxFS_read(&l_F3DData.instance_3_Count,sizeof(ushort),1,file);
  lxFS_read(&l_F3DData.helpersCount,sizeof(ushort),1,file);

  l_F3DData.totalMeshCount =  l_F3DData.meshesCount+
                l_F3DData.instance_1_Count+
                l_F3DData.instance_2_Count+
                l_F3DData.instance_3_Count;

  Model_allocMeshObjs(model,l_F3DData.totalMeshCount);
    // instance 2 doesnt need bones
  Model_allocBones(model, l_F3DData.meshesCount+
              l_F3DData.instance_1_Count+
              l_F3DData.instance_3_Count+
              l_F3DData.helpersCount);

/*  IMESH_MESH,
  IMESH_INST_SHRD_VERT,
  IMESH_INST_SHRD_VERT_PARENT_MAT,
  IMESH_INST_SHRD_VERT_TRIS,
  IMESH_HELPER,
*/

  offsetID = 0;
  fileLoadF3DMesh(IMESH_MESH,l_F3DData.meshesCount,offsetID);
  offsetID += l_F3DData.meshesCount;
  fileLoadF3DMesh(IMESH_INST_SHRD_VERT,l_F3DData.instance_1_Count,offsetID);
  offsetID += l_F3DData.instance_1_Count;
  fileLoadF3DMesh(IMESH_INST_SHRD_VERT_PARENT_MAT, l_F3DData.instance_2_Count,offsetID);
  offsetID += l_F3DData.instance_2_Count;
  fileLoadF3DMesh(IMESH_INST_SHRD_VERT_TRIS, l_F3DData.instance_3_Count,offsetID);
  offsetID += l_F3DData.instance_3_Count;
  fileLoadF3DMesh(IMESH_HELPER,l_F3DData.helpersCount,offsetID);

  // Materials
  lxFS_read(&l_F3DData.materialsCount,sizeof(ushort),1,file);
  fileLoadF3DMaterial(l_F3DData.materialsCount);
  // Skins
  lxFS_read(&l_F3DData.skinsCount,sizeof(ushort),1,file);

  if (model->modeltype == MODEL_DYNAMIC){
    Model_allocSkins(model,l_F3DData.skinsCount);
    fileLoadF3DSkin(l_F3DData.skinsCount);
  }

  FS_close(file);

  return LUX_TRUE;
}


static void fileLoadF3DMesh(uint type, uint count, uint offset)
{
  lxFSFile_t      *file = l_F3DData.file;
  Bone_t        *bone;
  MeshObject_t    *meshobj;
  Mesh_t        *mesh;
  lxVertex64_t      *vert64;
  lxVertex32_t      *vert32;
  float       buffer[16];

  char    name[4096];
  char    *curname;

  uint  i;
  uint  n;
  uint  len;

  uint  uintCnt;
  ushort  ushortCnt;
  ushort  ushort1;
  uchar uchar;

  /*
  <[2] Eltern ID>
  <[2] String länge > [<[1] Name des Objekts>]
  <[4 * 16] 4*4 Float Matrix44, relativ zum Elternteil>
  <[12] RefPose(x,y,z)> <[16] RefRot(x,y,z,w)> <[12] RefScale (x,y,z)>
   */

  for (n = 0; n < count; n++){
    // Generic Info
    // id
    bone = (type != IMESH_INST_SHRD_VERT_PARENT_MAT) ? &l_F3DData.model->bonesys.bones[l_F3DData.curbone++] : NULL;
    meshobj = (type != IMESH_HELPER) ? &l_F3DData.model->meshObjects[n+offset] : NULL;

    lxFS_read(&ushort1,sizeof(ushort),1,file);
    if (bone)
      bone->parentID = f3dFixBoneID(ushort1);

    // name
    lxFS_read(&ushort1,sizeof(ushort),1,file);
    lxFS_read(&name,sizeof(uchar),ushort1,file);
    LUX_ASSERT(ushort1 < 4096);
    name[ushort1]=0;

    resnewstrcpy(curname,name);
    if (meshobj){
      meshobj->name = curname;
      meshobj->bone = bone;
      meshobj->skinID = -1;
    }
    if (bone)
      bone->name = curname;

    // matrix
    lxFS_read(buffer,sizeof(float),16,file);
    if (bone){
      lxMatrix44Copy(bone->refMatrix,buffer);
    }

    // Ref Pos, Rot, Scale
    lxFS_read(buffer,sizeof(float),3,file);
    if (bone){
      lxVector3Copy(bone->refPRS.pos,buffer);
    }
    lxFS_read(buffer,sizeof(float),4,file);
    if (bone){
      lxVector4Copy(bone->refPRS.rot,buffer);
    }
    lxFS_read(buffer,sizeof(float),3,file);
    if (bone){
      lxVector3Copy(bone->refPRS.scale,buffer);

      if (bone->refPRS.scale[0] && bone->refPRS.scale[1] && bone->refPRS.scale[2] &&
        bone->refPRS.scale[0]!=1.0f && bone->refPRS.scale[1]!=1.0f && bone->refPRS.scale[2]!=1.0f){
        bone->refInvScale[0] = 1.0f/bone->refPRS.scale[0];
        bone->refInvScale[1] = 1.0f/bone->refPRS.scale[1];
        bone->refInvScale[2] = 1.0f/bone->refPRS.scale[2];

        // rebuild matrix without scale
        // rot
        lxQuatToMatrixIdentity(bone->refPRS.rot,bone->refMatrix);
        //pos
        lxMatrix44SetTranslation(bone->refMatrix,bone->refPRS.pos);
      }
      else
        lxVector3Set(bone->refInvScale,1,1,1);
    }



    switch (type){
      case IMESH_HELPER:
        break;
      case IMESH_MESH:
      /*  <[2] Material ID>
        <[2] Anzahl der Punkte>
          [ <[16] tx,ty,tz,tw> <[8] (u,v)> <[4] (r,g,b,a)> <[12] (nx,ny,nz)> <[12] (x,y,z)> ]
        <1> Primitve Type
          {
          0: Points
          1: Triangle List
          2: Triangle Strip
          3: Quad List
          4: Quad Strip
          }
        <[2] Total Indices>
        <[2] Primitivelistencount>
          [
          <[2] Material ID> IGNORED
          <[2] Anzahl der Indices>  ## z.b Triangle List : indices = tris * 3, Triangle Strip indices = tris - 2
            [<[2] Vertex ID>]
          ]*/
        mesh = meshobj->mesh = reszalloc(sizeof(Mesh_t));
        mesh->instanceType = f3dInstanceType(type);
        mesh->vertextype = l_F3DData.model->vertextype;
        // material
        lxFS_read(&ushort1,sizeof(ushort),1,file);
        meshobj->texRID = ushort1;

        // number of Vertices
        uintCnt = 0;
        lxFS_read(&uintCnt,(l_F3DData.f3dversion >= F3D_VERSION_26) ? sizeof(uint) : sizeof(ushort),1,file);

        switch (l_F3DData.model->vertextype){
          case VERTEX_64_TEX4:
          case VERTEX_64_SKIN:
            vert64 = mesh->vertexData = reszallocaligned(sizeof(lxVertex64_t)*uintCnt,32);
            mesh->numAllocVertices = mesh->numVertices = uintCnt;
            switch(l_F3DData.f3dversion)
            {
            case F3D_VERSION_23:
              for (i = 0; i < uintCnt; i++){
                lxFS_read(vert64->tex2,sizeof(float),4,file);
                lxFS_read(vert64->tex,sizeof(float),2,file);
                lxFS_read(vert64->color,sizeof(uchar),4,file);
                lxFS_read(buffer,sizeof(float),3,file);
                lxVector3short_FROM_float(vert64->normal,buffer);
                lxFS_read(vert64->pos,sizeof(float),3,file);
                vert64++;
              }
              break;
            case F3D_VERSION_24:
              for (i = 0; i < uintCnt; i++){
                // tex,tex2 user4
                lxFS_read(vert64->tex,sizeof(float),8,file);
                // normal
                lxFS_read(buffer,sizeof(float),3,file);
                lxVector3short_FROM_float(vert64->normal,buffer);
                // color
                lxFS_read(vert64->color,sizeof(uchar),4,file);
                lxFS_read(vert64->pos,sizeof(float),3,file);
                // pad
                lxFS_read(buffer,sizeof(float),1,file);
                vert64++;
              }
              break;
            case F3D_VERSION_25:
            default:
              lxFS_read(vert64,sizeof(lxVertex64_t),mesh->numVertices,file);
              break;

            }
            break;
          case VERTEX_32_TEX2:
            vert32 = mesh->vertexData = reszallocaligned(sizeof(lxVertex32_t)*uintCnt,32);
            mesh->numAllocVertices = mesh->numVertices = uintCnt;
            switch(l_F3DData.f3dversion)
            {
            case F3D_VERSION_23:
              for (i = 0; i < uintCnt; i++){
                lxFS_read(vert32->tex1,sizeof(float),2,file);
                lxFS_read(buffer,sizeof(float),2,file);
                lxFS_read(vert32->tex,sizeof(float),2,file);
                lxFS_read(vert32->color,sizeof(uchar),4,file);
                lxFS_read(buffer,sizeof(float),3,file);
                lxFS_read(vert32->pos,sizeof(float),3,file);
                vert32++;
              }
              break;
            case F3D_VERSION_24:
              for (i = 0; i < uintCnt; i++){
                // tex, tex2
                lxFS_read(vert32->tex,sizeof(float),4,file);
                //FS_read(vert32->tex2,sizeof(float),2,file);
                // user4, normal
                lxFS_read(buffer,sizeof(float),7,file);
                lxFS_read(vert32->color,sizeof(uchar),4,file);
                lxFS_read(vert32->pos,sizeof(float),3,file);
                lxFS_read(buffer,sizeof(float),1,file);
                vert32++;
              }
              break;

            case F3D_VERSION_25:
            default:
              for (i = 0; i < uintCnt; i++){
                // tex, tex2
                lxFS_read(vert32->tex,sizeof(float),4,file);
                //FS_read(vert32->tex2,sizeof(float),2,file);
                // user4, normal,tangent
                lxFS_read(buffer,sizeof(float),8,file);
                lxFS_read(vert32->pos,sizeof(float),3,file);
                lxFS_read(vert32->color,sizeof(uchar),4,file);
                  vert32++;
              }
              break;
            }

            break;
          case VERTEX_32_NRM:
            vert32 = mesh->vertexData = reszallocaligned(sizeof(lxVertex32_t)*uintCnt,32);
            mesh->numAllocVertices = mesh->numVertices = uintCnt;
            switch(l_F3DData.f3dversion)
            {
            case F3D_VERSION_23:
              for (i = 0; i < uintCnt; i++){
                // user4
                lxFS_read(buffer,sizeof(float),4,file);
                lxFS_read(vert32->tex,sizeof(float),2,file);
                lxFS_read(vert32->color,sizeof(uchar),4,file);
                //normal
                lxFS_read(buffer,sizeof(float),3,file);
                lxVector3short_FROM_float(vert32->normal,buffer);
                lxFS_read(vert32->pos,sizeof(float),3,file);
                vert32++;
              }
              break;
            case F3D_VERSION_24:
              for (i = 0; i < uintCnt; i++){
                // tex
                lxFS_read(vert32->tex,sizeof(float),2,file);
                // tex2, user4
                lxFS_read(buffer,sizeof(float),6,file);
                // normal
                lxFS_read(buffer,sizeof(float),3,file);
                lxVector3short_FROM_float(vert32->normal,buffer);
                lxFS_read(vert32->color,sizeof(uchar),4,file);
                lxFS_read(vert32->pos,sizeof(float),3,file);
                lxFS_read(buffer,sizeof(float),1,file);
                vert32++;
              }
              break;
            default:
            case F3D_VERSION_25:
              for (i = 0; i < uintCnt; i++){
                // tex
                lxFS_read(vert32->tex,sizeof(float),2,file);
                // tex2, user4
                lxFS_read(buffer,sizeof(float),6,file);
                // normal
                lxFS_read(vert32->normal,sizeof(short),4,file);
                // tangent
                lxFS_read(buffer,sizeof(short),4,file);
                lxFS_read(vert32->pos,sizeof(float),3,file);
                lxFS_read(vert32->color,sizeof(uchar),4,file);
                vert32++;
              }
              break;
            }

            break;
          default:
            LUX_ASSERT(0);
            break;
        }

        // primitive type
        lxFS_read(&uchar,sizeof(uchar),1,file);
        mesh->primtype = f3dPrimType(uchar);
        // total indices count
        uintCnt = 0;
        lxFS_read(&uintCnt,(l_F3DData.f3dversion >= F3D_VERSION_26) ? sizeof(uint) : sizeof(ushort),1,file);
        mesh->index16 = mesh->numAllocVertices <= BIT_ID_FULL16;
        if (mesh->index16)
          mesh->indicesData16 = reszalloc(sizeof(ushort)*uintCnt);
        else
          mesh->indicesData32 = reszalloc(sizeof(uint)*uintCnt);
        mesh->numIndices = mesh->numAllocIndices = uintCnt;
        // group count
        lxFS_read(&ushortCnt,sizeof(ushort),1,file);
        mesh->indicesGroupLength = reszalloc(sizeof(uint)*ushortCnt);
        mesh->numGroups = ushortCnt;
        len = 0;
        for (i = 0; i < ushortCnt; i++){
          // material id (ignored)
          lxFS_read(&ushort1,sizeof(ushort),1,file);

          // indices
          uintCnt = 0;
          lxFS_read(&uintCnt,(l_F3DData.f3dversion >= F3D_VERSION_26) ? sizeof(uint) : sizeof(ushort),1,file);

          mesh->indicesGroupLength[i] = uintCnt;

          if (mesh->numAllocVertices > BIT_ID_FULL16)
            lxFS_read(&mesh->indicesData32[len],sizeof(uint),uintCnt,file);
          else
            lxFS_read(&mesh->indicesData16[len],sizeof(ushort),uintCnt,file);

          len += uintCnt;
        }
        break;
      case IMESH_INST_SHRD_VERT:
      case IMESH_INST_SHRD_VERT_PARENT_MAT:
      /*  <[2] Objekt ID als instanz>
        <[2] Material ID>
        <[2] Total Indices>
        <[2] Polygonlistencount>
          [
          <[2] Material ID>
          <[2] Anzahl der Indices>
            [<[2] Vertex ID>]*/
        mesh = meshobj->mesh = reszalloc(sizeof(Mesh_t));
        mesh->instanceType = f3dInstanceType(type);
        mesh->vertextype = l_F3DData.model->vertextype;
        // instance
        lxFS_read(&ushort1,sizeof(ushort),1,file);
        if (!ushort1){
          LUX_ASSERT(0);
        }
        else{
          mesh->instance = l_F3DData.model->meshObjects[((int)ushort1)-1].mesh;
          if (type == IMESH_INST_SHRD_VERT_PARENT_MAT){
            meshobj->bone = l_F3DData.model->meshObjects[((int)ushort1)-1].bone;
          }
        }

        uchar = (l_F3DData.model->meshObjects[ushort1-1].mesh->numAllocVertices > BIT_ID_FULL16);

        // material
        lxFS_read(&ushort1,sizeof(ushort),1,file);
        meshobj->texRID = ushort1;

        // total indices count
        uintCnt = 0;
        lxFS_read(&uintCnt,(l_F3DData.f3dversion >= F3D_VERSION_26) ? sizeof(uint) : sizeof(ushort),1,file);
        if (uchar)
          mesh->indicesData32 = reszalloc(sizeof(uint)*uintCnt);
        else
          mesh->indicesData16 = reszalloc(sizeof(ushort)*uintCnt);
        mesh->index16 = !uchar;
        mesh->numIndices = mesh->numAllocIndices = uintCnt;
        // group count
        lxFS_read(&ushortCnt,sizeof(ushort),1,file);
        mesh->indicesGroupLength = reszalloc(sizeof(uint)*ushortCnt);
        mesh->numGroups = ushortCnt;
        len = 0;
        for (i = 0; i < ushortCnt; i++){
          // material id
          lxFS_read(&ushort1,sizeof(ushort),1,file);
          // indices
          uintCnt = 0;
          lxFS_read(&uintCnt,(l_F3DData.f3dversion >= F3D_VERSION_26) ? sizeof(uint) : sizeof(ushort),1,file);

          mesh->indicesGroupLength[i] = uintCnt;
          if (uchar)
            lxFS_read(&mesh->indicesData32[len],sizeof(uint),uintCnt,file);
          else
            lxFS_read(&mesh->indicesData16[len],sizeof(ushort),uintCnt,file);

          len += uintCnt;
        }
        break;
      case IMESH_INST_SHRD_VERT_TRIS:
        mesh = meshobj->mesh = reszalloc(sizeof(Mesh_t));
        mesh->instanceType = f3dInstanceType(type);
        mesh->vertextype = l_F3DData.model->vertextype;
        // instance
        lxFS_read(&ushort1,sizeof(ushort),1,file);
        if (!ushort1){
          // ERROR
        }
        mesh->instance = l_F3DData.model->meshObjects[((int)ushort1)-1].mesh;
        mesh->index16 = mesh->instance->index16;
        // material
        lxFS_read(&ushort1,sizeof(ushort),1,file);
        meshobj->texRID = ushort1;
        break;
    }

  }
}
static void fileLoadF3DMaterial(uint count){
  lxFSFile_t    *file = l_F3DData.file;
  MeshObject_t  *meshobj;

  uint    n;
  int     m;
  char    name[4096];
  char*   curname;

  ushort    ushort1;

  for (n = 0; n < count; n++){
    // Shader name
    lxFS_read(&ushort1,sizeof(ushort),1,file);
    lxFS_read(&name,sizeof(uchar),ushort1,file);
    LUX_ASSERT(ushort1 < 4096);
    name[ushort1]=0;

    // look if any mesh uses this material
    // if yes set its texturename
    curname = NULL;
    if (name[0])
      for (m = 0; m < l_F3DData.model->numMeshObjects; m++){
        meshobj = &l_F3DData.model->meshObjects[m];
        if (meshobj->texRID == n+1){
          if (!curname){
            resnewstrcpy(curname,name);
          }
          meshobj->texRID = 1;
          meshobj->texname = curname;
        }
      }
  }

  // clear ids which werent found
  for (m = 0; m < l_F3DData.model->numMeshObjects; m++){
    meshobj = &l_F3DData.model->meshObjects[m];
    if (meshobj->texRID && !meshobj->texname)
      meshobj->texRID = 0;
  }
}
static void fileLoadF3DSkin(uint count){
  lxFSFile_t      *file = l_F3DData.file;

  Skin_t      *skin;
  SkinVertex_t  *skinvert;

  uint  i,j;
  uint    n;
  uint  uintCnt;
  ushort  ushortCnt2;
  ushort  ushort1;
  ushort  ushort2;
  float   floatv;

  for (n = 0; n < count ; n++){
  /*  <[2] ID des deformierten Meshes>
    <[2] Maximal Anzahl der Gewichtungen pro Punkt
    <[2] Anzahl der Punkte>
      [
      <[2] Anzahl Weights>                         ## Gewichte in Absteigender Reihenfolge
        [<[2] Parent ID> <[2] Weight>]
      ]*/
    skin = &l_F3DData.model->skins[n];
    // deform mesh
    lxFS_read(&ushort1,sizeof(ushort),1,file);
    if (!ushort1){
      // ERROR
      LUX_ASSERT(0);
    }
    else{
      l_F3DData.model->meshObjects[((int)ushort1)-1].skinID = n;
    }
    // max count
    lxFS_read(&ushort1,sizeof(ushort),1,file);

    // vertices
    uintCnt = 0;
    lxFS_read(&uintCnt,l_F3DData.f3dversion >= F3D_VERSION_26 ? sizeof(uint) : sizeof(ushort),1,file);

    skin->numSkinVertices = uintCnt;
    skin->skinvertices = reszalloc(sizeof(SkinVertex_t)*uintCnt);
    skinvert = skin->skinvertices;
    for ( i = 0; i < uintCnt; i++,skinvert++){
      lxFS_read(&ushortCnt2,sizeof(ushort),1,file);
      skinvert->numWeights = LUX_MIN(ushortCnt2,MODEL_MAX_WEIGHTS);
      for ( j = 0; j < ushortCnt2; j++){
        // parent
        lxFS_read(&ushort1,sizeof(ushort),1,file);
        // weight
        lxFS_read(&ushort2,sizeof(ushort),1,file);
        floatv = (float)ushort2 / 0xffff;
        if (ushort1 && j < MODEL_MAX_WEIGHTS){
          skinvert->matrixIndex[j] = f3dFixBoneID(ushort1)-1;
          skinvert->weights[j] = floatv;
        }
      }
    }
  }

}

