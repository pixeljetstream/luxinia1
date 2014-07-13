// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __RESMANAGER_H__
#define __RESMANAGER_H__

/*
RESMANAGER
----------

The ResManager is the main resource creator. Thru
ResData_addCLASS you can load most resources from
files. Normally the resources are only loaded once.
You will get a index to the array back. -1 if
the operation failed.
No pointers are returned because I wanted a unified way
for all resources and for some types I need int.
Namely Materials and Textures are separated by their
return value (textures start at 0 and Materials at VID_OFFSET_MATERIAL)
To get pointers use ResData_getCLASS(id)

ResData holds all resources that are supposedly static, like models, textures,
and are often reused, as those use different memory than the dynamic nodes
that are normally created.
ResData is sepearted in ResourceChunks. The user specifies
how many chunks he wants and how big those should be, ie how many
resources of each type they can hold and how much memory they
preallocate.
Resources are then loaded into the current active chunk, because
of the linear memorypool they cannot be destroyed individually
but only a total chunk can be destroyed.
When a chunk is destroyed it will however keep the resources
that are still needed by other chunks. Since the "unique resource"
lookup on loading is done over all resources and not individually
to save memory, this was needed.
The separation in ResourceChunks can be done only once until
the built in maximums are hit.
On startup a default chunk is generated, only to hold
all resources that the engine itself needs. If the user
will not define chunks in his main.lua call, the engine
will create a second chunk.
The user can specify what should happen when a chunk is full
and a resource wants to be loaded in it. you can either throw an
error, or just fill the next chunk.

Resources are classified as "direct" loaded or "indirect"
indirect are those resources that are loaded by other resources.
When a ResourceChunk is reloaded only "directlys" are loaded again,
of course with their dependants
This is done to prevent collecting unused resources, when
e.g. materials or other scripts have changed some textures
might not be needed anymore.

A lot can go wrong when consumers of resource, that
were in a chunk that was cleared, are still active.
So the user should keep resources/consumer managment
in unified blocks himself, to prevent this from
happening, as he is totally responsible for it.
You can always play safe and use only one chunk
(or none, ie. use second default)


ResourceData (nested, FixedMem)
ResourceChunk (user, GenMem)


WHO REFS WHOM
-------------

a list of which resources can be autoloaded
by each other on creation.

model     -> texture/material , collmodel
anim      -> none
texture     -> none
material    -> texture, shader
shader      -> texture, gpuprog
gpuprog     -> none
particlecloud -> none
particlesys   -> texture/material, model
sound     -> none
collmodel   -> none

that means following order
when popping reffed resources:

texture, gpuprog, collmodel
shader,
material,
model,
particlesys,
anim,particlecloud,terrain,sound

Author: Christoph Kubisch

*/

// RESOURCES

#include "../resource/shader.h"
#include "../resource/material.h"
#include "../resource/model.h"
#include "../resource/animation.h"
#include "../resource/texture.h"
#include "../resource/sound.h"
#include "../resource/particle.h"
#include "../resource/gpuprog.h"
#include "../render/gl_render.h"

#define RES_MAXSTRLENGTH  1024
#define RES_MAX_CHUNKNAME 64

#define RES_MAX_MODELS    4096
#define RES_MAX_ANIMATIONS  1024
  // gl list3d sorting shifts depend on the next 3 values
#define RES_MAX_TEXTURES  512   // 9 bits
#define RES_MAX_MATERIALS 512
#define RES_MAX_SHADERS   126   // 7 bits - 2

#define RES_MAX_GPUPROGS  512
#define RES_MAX_PARTICLECLD 128
#define RES_MAX_PARTICLESYS 128
#define RES_MAX_SOUNDS    256

#define RES_MAXS      (RES_MAX_MODELS+RES_MAX_ANIMATIONS+RES_MAX_TEXTURES+\
              RES_MAX_MATERIALS+RES_MAX_SHADERS+RES_MAX_GPUPROGS+\
              RES_MAX_PARTICLECLD+RES_MAX_PARTICLESYS+\
              RES_MAX_SOUNDS)


  // default chunk holds ptrs, a few default resource, and terrain cache
#define RES_CORE_NAME   "?core"
#define RES_DEFAULT_NAME  "?default"

#define RES_VBO_MEM_KB    2048
#define RES_IBO_MEM_KB    1024

#define RES_PTR_RESERVE   (void*)1

#define RES_CGC_SEPARATOR "^"

#define SUBRESID_CHECK(meshid,modelID)    ((meshid>>16)==modelID)
#define SUBRESID_MAKE(meshid,modelID)   (meshid = (modelID<<16) | meshid)
#define SUBRESID_GETRES(meshid)       (meshid>>16)
#define SUBRESID_GETSUB(meshid)       (meshid&0xffff)
#define SUBRESID_MAKEPURE(meshid)     (meshid&=0xffff)

typedef enum ResourceBOType_e{
  RESOURCE_BO_VERTEX,
  RESOURCE_BO_INDEX,
  RESOURCE_BOS,
}ResourceBOType_t;

typedef enum ResourceType_e{
  RESOURCE_NONE = -1,
  RESOURCE_MODEL = 0,
  RESOURCE_ANIM,
  RESOURCE_TEXTURE,
  RESOURCE_MATERIAL,
  RESOURCE_SHADER,
  RESOURCE_GPUPROG,
  RESOURCE_PARTICLECLOUD,
  RESOURCE_PARTICLESYS,
  RESOURCE_SOUND,
  RESOURCES,
}ResourceType_t;

// fill behavior
typedef enum ResourceFill_e{
  RESOURCE_FILL_ONLYSELF, // fills only current chunk
  RESOURCE_FILL_TONEXT, // activates next chunk if current is full resets to original chunk once done
}ResourceFill_t;

typedef void (ResourceClearFn) (void*);

typedef struct ResourceDescriptor_s{
  char  *name;      // name of the resource
  char  path[256];      // default search path
  union{          // identifier of the resource type, for checking pointer validity
    char  nameID[4];
    int   typeID;
  };
  int   max;      // max resources that the engine will allow of this type
  int   core;     // core chunksize for this resource (hardcoded resources will go here)
  size_t  size;     // sizeof(Type)
  ResourceClearFn*  clearfunc;  // set if the resource needs to dereference other resources
              // or special unloading
}ResourceDescriptor_t;


typedef struct ResourceReload_s{
  char  name[RES_MAXSTRLENGTH];
  char  extraname[RES_MAXSTRLENGTH];

  int   resRID;

  int   arg0;   // different for the classes
  int   arg1;
  int   arg2;

  char  customdata[32];

  struct ResourceReload_s LUX_LISTNODEVARS;
}ResourceReload_t;


typedef struct ResourceContainer_s{
  int     resCnt;   // total count for this chunk
  int     resUsed;  // used resources
  int     resOffset;  // offset into the main array
}ResourceContainer_t;


typedef struct ResourceChunk_s{
  Reference reference;
  char    name[RES_MAX_CHUNKNAME];
  int     chunkID;  // just filled linearly

  ResourceContainer_t   containers[RESOURCES];
  lxMemoryStackPTR      mempool;

  HWBufferObject_t    *vbosListHead[RESOURCE_BOS];

  struct ResourceChunk_s  *prev,*next;
}ResourceChunk_t;

typedef union {
  Texture_t   *tex;
  Model_t     *model;
  Anim_t      *anim;
  Material_t    *material;
  Shader_t    *shader;
  ParticleSys_t *psys;
  ParticleCloud_t *pcloud;
  GpuProg_t   *gpuprog;
  Sound_t     *sound;
  ResourceInfo_t  *resinfo;
  void      *mem;
}ResourcePtr_t;

typedef struct ResourceArray_s{
  ResourcePtr_t *ptrs;      // pointers to the resources
  int       chunkOffset;  // if a new chunk is made it will start here
  int       numLoaded;    // number of total resources loaded
  int       numMax;
}ResourceArray_t;

typedef struct ResourceData_s
{
  ResourceFill_t    strategy;   // fill strategy

  ResourceArray_t   resarray[RESOURCES];
  ResourcePtr_t   resptrs[RES_MAXS];

  int         numChunks;
  ResourceChunk_t   *chunkListHead;
  ResourceChunk_t   *chunkActive; // the chunk we want to fill now

  size_t        newIBOsize;   // in KB
  size_t        newVBOsize;

}ResourceData_t;

// GLOBALS
extern ResourceData_t g_ResData;
extern ResourceDescriptor_t g_ResDescriptor[];


//////////////////////////////////////////////////////////////////////////
// SETUP


  // basic initialization creates the default chunk
void  ResData_init(uint defmemsizeKB);

  // resets everything, frees all chunks
void  ResData_deinit();

void  ResData_freeDefault();

void  ResData_strategy(ResourceFill_t type);

  // runs thru all memorypools and sums their inuse values
size_t  ResData_getTotalMemUse();
  // runs thru all memorypools and sums their preallocated sizes
size_t  ResData_getTotalMem();

int   ResData_search(ResourceType_t type ,const char *name, const char *extraname);

  // called from the external clear functions, they deref their indirect resources
void  ResData_unrefResourcePtr(ResourceType_t type, void *resinfoUnref, void *resinfoSrc);
void  ResData_unrefResource(ResourceType_t type, int index, void *resinfoSrc);

  // sets or unsets the core chunk, only use it for internal resources
void  ResData_CoreChunk( int state);

int   ResData_getOpenCnt(ResourceType_t type);

int   ResData_getLoadCnt(ResourceType_t type);

  // returns the framecnt when the last reload happened
ulong ResData_lastReloadFrame();

ResourceChunk_t* ResData_getChunkActive();

ResourceChunk_t* ResData_getChunkFromRID(int id);

  // will not search for this type but load (dont forget to reset to RESOURCE_NONE)
void  ResData_forceLoad(ResourceType_t restype);

void  ResData_overwriteRID(int id);

  // returns previous active chunk, and sets active chunk to the one of resinfo
ResourceChunk_t* ResData_setChunkFromPtr(ResourceInfo_t *resinfo);

ResourceChunk_t* ResData_getChunkDefault();


  // returns vboid and offsetsize
  // type 0 = vbo, 1 = ibo
int   ResData_getBO(ResourceInfo_t *resinfo,ResourceBOType_t type, uint size, HWBufferObject_t **outbuffer, int padsize);

  // name must be < 64
  // maxcnts must be of size: RESOURCES
  // float < 0 is percentage of maximum = -1 get all what is left, -0.5 try to get half of maximum
  // allocates mempool
ResourceChunk_t* ResourceChunk_new(char *name, float *maxcnts, uint memsizeKB);

  // make the chunk the one used for new resources
void  ResourceChunk_activate(ResourceChunk_t *chunk);

  // unloads all resources, resets mempool
  // you can specify which resources should be reloaded
void  ResourceChunk_clear(ResourceChunk_t *chunk, int keepdepth, int reloadoutref);

size_t  ResourceChunk_bytesLeft(ResourceChunk_t *chunk);

//////////////////////////////////////////////////////////////////////////
// GETTERS

#define ResData_isValidPtr(ptr, type) (((ResourceInfo_t*)ptr)->typeID == g_ResDescriptor[type].typeRID)

  // returns GL id
#define ResData_getTextureID(id)   g_ResData.resarray[RESOURCE_TEXTURE].ptrs[id].tex->texID

  // returns GL target
#define ResData_getTextureTarget(id) g_ResData.resarray[RESOURCE_TEXTURE].ptrs[id].tex->target

  // returns GL target, GL id
#define ResData_getTextureBind(id)   ResData_getTextureTarget(id),ResData_getTextureID(id)

  // current bound texture must be the one used
#define ResData_setTextureClamp(id,state)  Texture_clamp((g_ResData.resarray[RESOURCE_TEXTURE].ptrs[id].tex),state)

#define ResData_getGpuProgARB(id)  g_ResData.resarray[RESOURCE_GPUPROG].ptrs[id].gpuprog->progID

#define ResData_getGpuProgCG(id)   g_ResData.resarray[RESOURCE_GPUPROG].ptrs[id].gpuprog->cgProgram

LUX_INLINE ResourceInfo_t*    ResData_getInfo(const int id, const int type){
  return (id > -1) ? g_ResData.resarray[type].ptrs[id].resinfo : NULL;}
LUX_INLINE Texture_t*   ResData_getTexture(const int id){
  return (id > -1) ? g_ResData.resarray[RESOURCE_TEXTURE].ptrs[id].tex : NULL;}
LUX_INLINE Sound_t*     ResData_getSound(const int id){
  return (id > -1) ? g_ResData.resarray[RESOURCE_SOUND].ptrs[id].sound : NULL;}
LUX_INLINE Model_t*     ResData_getModel(const int id){
  return (id > -1) ? g_ResData.resarray[RESOURCE_MODEL].ptrs[id].model : NULL;}
LUX_INLINE float*     ResData_getModelBoneAbs(const int id, const int boneid, const int user){
  return (id > -1 && (int)g_ResData.resarray[RESOURCE_MODEL].ptrs[id].model->bonesys.numBones > boneid) ? (user ? &g_ResData.resarray[RESOURCE_MODEL].ptrs[id].model->bonesys.userAbsMatrices[boneid*16] : &g_ResData.resarray[RESOURCE_MODEL].ptrs[id].model->bonesys.refAbsMatrices[boneid*16] ) : NULL;}
LUX_INLINE Bone_t*      ResData_getModelBone(const int id, const int boneid){
  return (id > -1 && (int)g_ResData.resarray[RESOURCE_MODEL].ptrs[id].model->bonesys.numBones > boneid) ? &g_ResData.resarray[RESOURCE_MODEL].ptrs[id].model->bonesys.bones[boneid] : NULL;}
LUX_INLINE Anim_t*      ResData_getAnim(const int id){
  return (id > -1) ?  g_ResData.resarray[RESOURCE_ANIM].ptrs[id].anim : NULL;}
LUX_INLINE GpuProg_t*   ResData_getGpuProg(const int id){
  return (id > -1) ?  g_ResData.resarray[RESOURCE_GPUPROG].ptrs[id].gpuprog : NULL;}
LUX_INLINE ParticleSys_t* ResData_getParticleSys(const int id){
  return (id > -1) ?  g_ResData.resarray[RESOURCE_PARTICLESYS].ptrs[id].psys : NULL;}
LUX_INLINE ParticleCloud_t* ResData_getParticleCloud(const int id){
  return (id > -1) ?  g_ResData.resarray[RESOURCE_PARTICLECLOUD].ptrs[id].pcloud : NULL;}
LUX_INLINE Material_t*    ResData_getMaterial(const int id){
  return (id - VID_OFFSET_MATERIAL > -1) ? g_ResData.resarray[RESOURCE_MATERIAL].ptrs[id-VID_OFFSET_MATERIAL].material : NULL;}
LUX_INLINE Shader_t*    ResData_getShader(const int id){
  return (id > -1) ?  g_ResData.resarray[RESOURCE_SHADER].ptrs[id].shader : NULL;}
LUX_INLINE Shader_t*    ResData_getMaterialShader(const int id){
  return (id - VID_OFFSET_MATERIAL > -1) ? ResData_getShader(g_ResData.resarray[RESOURCE_MATERIAL].ptrs[id-VID_OFFSET_MATERIAL].material->shaders[0].resRID) : NULL;}
LUX_INLINE Shader_t*    ResData_getMaterialShaderUse(const int id){
  return (id - VID_OFFSET_MATERIAL > -1) ? ResData_getShader(g_ResData.resarray[RESOURCE_MATERIAL].ptrs[id-VID_OFFSET_MATERIAL].material->shaders[0].resRIDUse) : NULL;}


//////////////////////////////////////////////////////////////////////////
// UPLOADS

void ResData_ModelInit(Model_t *model,int onlydata,const int needlm);
int ResData_addModel(char *name,ModelType_t modeltype, const lxVertexType_t vtype, const int needlm);
  // automatically decides if material or texture
int ResData_addTexMat(char *name);
int ResData_addTexture(char *name, TextureType_t type, int attributes);
  // combines images horizontally or as cubemap
int ResData_addTextureCombine(char **names, int num,TextureType_t type, int packsingle, int attributes);
int ResData_addMaterial(char *name,const char *cgcstring);
int ResData_addShader(char *name,const char *cgcstring);
int ResData_addAnimation(char *name, enum32 animflag);
int ResData_addSound(char *name);
int ResData_addParticleSys(char *name, int combdraw, int sortkey);
int ResData_addParticleCloud(char *name,int particlecount, int sortkey);
int ResData_addGpuProg(char *name,GPUProgType_t type, int lowCgProfile,const char *entryname);

// Helpers
int  ResData_getUserTexture(char *name);
void ResData_addUserTexture(int id, char *name);
  // either removes value or name
void ResData_removeUserTexture(char *name, int id);

// CgCompiler String Stack
void  ResData_pushCgCstring(const char *str);
void  ResData_popCgCstring();
const char* ResData_getCgCstring(int idx);
int   ResData_isCgCstringUsed();
int   ResData_getCgCstringCnt();
void  ResData_setCgCstringBase(char *str);
char* ResData_getCgCstringBase();
const char* ResData_concatCgCstring(const char *entryname);
booln ResData_isDefinedCgCstring(const char *check);

void ResData_resizeWindowTextures();
void ResData_popGL();
void ResData_pushGL();
void ResData_print();

#endif

