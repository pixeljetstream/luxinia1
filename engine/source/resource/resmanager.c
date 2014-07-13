// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include <luxinia/luxcore/contstringmap.h>

#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../main/main.h"
#include "resmanager.h"

// File Input/Output
#include "../fileio/filesystem.h"

#include "../render/gl_draw3d.h"
#include "../render/gl_shader.h"

// GLOBALS
ResourceData_t  g_ResData;
ResourceDescriptor_t g_ResDescriptor[]=
{
  {"Model   ",  "models/",    "mdl",  RES_MAX_MODELS,   0,      sizeof(Model_t),      (ResourceClearFn*)Model_clear},
  {"Anim    ",  "anims/",   "anm",  RES_MAX_ANIMATIONS, 0,      sizeof(Anim_t),       NULL},
  {"Texture ",  "textures/",  "tex",  RES_MAX_TEXTURES, 32,     sizeof(Texture_t),      (ResourceClearFn*)Texture_clear},
  {"Material",  "materials/", "mtl",  RES_MAX_MATERIALS,  0,      sizeof(Material_t),     (ResourceClearFn*)Material_clear},
  {"Shader  ",  "shaders/",   "shd",  RES_MAX_SHADERS,  0,      sizeof(Shader_t),     (ResourceClearFn*)Shader_clear},
  {"GpuProg ",  "gpuprogs/",  "gpu",  RES_MAX_GPUPROGS, VID_VPROGS, sizeof(GpuProg_t),      (ResourceClearFn*)GpuProg_clear},
  {"PCloud  ",  "",       "pcl",  RES_MAX_PARTICLECLD,0,      sizeof(ParticleCloud_t),  (ResourceClearFn*)ParticleCloud_clear},
  {"PSystem ",  "particles/", "psy",  RES_MAX_PARTICLESYS,0,      sizeof(ParticleSys_t),    (ResourceClearFn*)ParticleSys_clear},
  {"Sound   ",  "sounds/",    "snd",  RES_MAX_SOUNDS,   0,      sizeof(Sound_t),      (ResourceClearFn*)Sound_clear},
};

// default textures:
//          vid = 7
//            normalize,att,specular,diffuse, logo (2), white
//          lua = 6
//            fonts (2), gui (2), mouse, colorpicker(2)
//          font 1,


// LOCALS
static struct RESData_s{
  int         exitclear;
  int         forceload;
  int         forcedRID;
  int         forcedOverwrite;
  int         indirectlevel;
  ulong       lastreload;
  ResourceChunk_t * fillchunk;
  ResourceReload_t* reloadListHead[RESOURCES];
  ResourceChunk_t * actchunk;
  ResourceChunk_t * chunks[256];
  lxStrMapPTR   userTextures;
  char        curpath[6][1024];
  FSFileLoaderType_t  loadertype;
  int         cgcstack;
  char        compilerargs[GPUPROG_MAX_COMPILERARG_LENGTH];
  const char      *cgcstackptrs[8];
  const char      *curcgcfullstr;
}l_RESData;

//////////////////////////////////////////////////////////////////////////
// SETUP

void  ResourceReload_resetAll(){
  ResourceType_t type;

  for (type = 0; type < RESOURCES; type ++)
    l_RESData.reloadListHead[type]=NULL;

}

//////////////////////////////////////////////////////////////////////////
// ResourceReload

int ResourceReload_push(ResourceType_t type, ResourceInfo_t *resinfo)
{
  ResourcePtr_t resptr;
  ResourceReload_t  *resload = lxMemGenZalloc(sizeof(ResourceReload_t));
  float *prescale;
  lxListNode_init(resload);

  resptr.resinfo = resinfo;

  if (resinfo->name)
    strcpy(resload->name,resinfo->name);
  if (resinfo->extraname)
    strcpy(resload->extraname,resinfo->extraname);

  resload->resRID = resinfo->resRID;

  switch(type) {
  case RESOURCE_MODEL:
    resload->arg0 = resptr.model->orgtype;
    resload->arg1 = resptr.model->vertextype;
    resload->arg2 = resptr.model->lmcoords;
    prescale = (float*)resload->customdata;
    lxVector3Copy(prescale,resptr.model->prescale);
    break;
  case RESOURCE_TEXTURE:
    resload->arg0 = resptr.tex->type;
    resload->arg1 = resptr.tex->attributes;
    break;
  case RESOURCE_GPUPROG:
    resload->arg0 = resptr.gpuprog->type;
    resload->arg1 = resptr.gpuprog->lowCgProfile;
    break;
  case RESOURCE_PARTICLECLOUD:
    resload->arg0 = resptr.pcloud->container.numParticles;
    resload->arg1 = resptr.pcloud->sortkey;
    resload->arg2 = resptr.pcloud->activeGroupCnt;
    break;
  case RESOURCE_ANIM:
    resload->arg0 = resptr.anim->animflag;
    prescale = (float*)resload->customdata;
    lxVector3Copy(prescale,resptr.anim->prescale);
    break;
  case RESOURCE_PARTICLESYS:
    resload->arg0 = resptr.psys->sortkey;
    resload->arg2 = resptr.psys->activeEmitterCnt;
    break;
  case RESOURCE_SOUND:
  case RESOURCE_SHADER:
  case RESOURCE_MATERIAL:
    // no extra info needed
    break;
  default:
    break;
  }

  lxListNode_addLast(l_RESData.reloadListHead[type],resload);

  return LUX_TRUE;
}

void  ResourceReload_popAll(ResourceType_t type)
{
  ResourceReload_t  *resload;
  int id = -1;
  int numtex = 0;
  int packsingle = 0;
  char  *letter;
  char  **names;
  lxVector3 prescale;

  while (l_RESData.reloadListHead[type]){
    lxListNode_popBack(l_RESData.reloadListHead[type],resload);

    l_RESData.forcedRID = resload->resRID;
    switch(type) {
    case RESOURCE_MODEL:
      ModelLoader_getPrescale(prescale);
      ModelLoader_setPrescale((float*)resload->customdata);
      id = ResData_addModel(resload->name,resload->arg0,resload->arg1,resload->arg2);
      ModelLoader_setPrescale(prescale);
      break;
    case RESOURCE_ANIM:
      AnimLoader_getPrescale(prescale);
      AnimLoader_setPrescale((float*)resload->customdata);
      id = ResData_addAnimation(resload->name,resload->arg0);
      AnimLoader_setPrescale(prescale);
      break;
    case RESOURCE_TEXTURE:
      // combined texture
      if (strstr(resload->name,"||")!=NULL) {
        sscanf(resload->name,"%d|%d||",&numtex,&packsingle);
        names = (char**)lxMemGenAlloc(sizeof(char*)*numtex);

        letter = resload->name;
        while (*letter && *letter != '|')
          letter++;
        letter++;
        letter++;

        numtex = lxStrSplitList(letter,names,'|',numtex);

        id = ResData_addTextureCombine(names,numtex,resload->arg0,packsingle,resload->arg1);

        lxMemGenFree(names,sizeof(char*)*numtex);
      }
      else
        id = ResData_addTexture(resload->name,resload->arg0,resload->arg1);
      break;
    case RESOURCE_MATERIAL:
      id = ResData_addMaterial(resload->name,resload->extraname) - VID_OFFSET_MATERIAL;
      break;
    case RESOURCE_SHADER:
      id = ResData_addShader(resload->name,resload->extraname);
      break;
    case RESOURCE_GPUPROG:
      id = ResData_addGpuProg(resload->name,resload->arg0,resload->arg1,(resload->extraname[0]) ? resload->extraname : NULL);
      break;
    case RESOURCE_PARTICLECLOUD:
      id = ResData_addParticleCloud(resload->name,resload->arg0,resload->arg1);
      ResData_getParticleCloud(id)->activeGroupCnt = resload->arg2;
      break;
    case RESOURCE_PARTICLESYS:
      id = ResData_addParticleSys(resload->name, LUX_FALSE, resload->arg0);
      ResData_getParticleSys(id)->activeEmitterCnt = resload->arg2;
      break;
    case RESOURCE_SOUND:
      id = ResData_addSound(resload->name);
      break;
    default:
      break;
    }

    if (id != resload->resRID){
      lprintf("ERROR resloadpop: wrong resRID returned %s\n",resload->name);
    }

    lxMemGenFree(resload,sizeof(ResourceReload_t));
  }

}

//////////////////////////////////////////////////////////////////////////
// ResChunk

// maxcnts must be of size: RESOURCES
// allocates mempool
ResourceChunk_t* ResourceChunk_new(char *name, float *incnts, uint memsizeKB)
{
  static int maxcnts[RESOURCES];

  int i;
  int totalres;
  int memminsize = 0;
  ResourceChunk_t*chunk;
  ResourceArray_t *resarray;
  ResourceContainer_t *rescont;
  lxMemoryStackPTR  mempool;

  if (g_ResData.numChunks>=255){
    bprintf("ERROR: reschunk: maximum reached 255, %s\n",name);
    return NULL;
  }

  totalres = 0;
  resarray = g_ResData.resarray;
  for (i= 0; i < RESOURCES; i++,resarray++){
    if (incnts[i]<0)
      maxcnts[i] = (-incnts[i]*(float)g_ResDescriptor[i].max);
    else
      maxcnts[i] = incnts[i];

    totalres += maxcnts[i] = LUX_MIN(g_ResDescriptor[i].max-resarray->chunkOffset,maxcnts[i]);
    memminsize += maxcnts[i]*(g_ResDescriptor[i].size+2048);
  }

  if (!totalres){
    bprintf("ERROR: reschunk: no space for resources, %s\n",name);
    return NULL;
  }
/*  // TOO critical, current "resources" would require ~ 18 MEGs
  if (LUX_MEMORY_KBS(memsizeKB) < memminsize){
    bprintf("ERROR: reschunk: mempool too small: %d < %d,  %s\n",LUX_MEMORY_KBS(memsizeKB),memminsize,name);
    return NULL;
  }
*/
  mempool = lxMemoryStack_new(GLOBAL_ALLOCATOR,name,LUX_MEMORY_KBS(memsizeKB));
  if (!mempool){
    bprintf("ERROR: reschunk: no mempool, %s\n",name);
    return NULL;
  }

  chunk = lxMemGenZalloc(sizeof(ResourceChunk_t));
  chunk->mempool = mempool;
  chunk->chunkID = g_ResData.numChunks;
  g_ResData.numChunks++;
  strncpy(chunk->name,name,RES_MAX_CHUNKNAME-1);
  lxListNode_init(chunk);
  lxListNode_addLast(g_ResData.chunkListHead,chunk);

  resarray = g_ResData.resarray;
  rescont = chunk->containers;
  for (i= 0; i < RESOURCES; i++,resarray++,rescont++){
    rescont->resCnt = maxcnts[i];
    rescont->resOffset = resarray->chunkOffset;
    resarray->chunkOffset += rescont->resCnt;
  }

  if (g_ResData.chunkActive == g_ResData.chunkListHead){
    // auto activate the first "none" core
    ResourceChunk_activate(chunk);
  }

  return chunk;
}

// unloads all resources and frees mempool
void  ResourceChunk_free(ResourceChunk_t *chunk)
{
  ResourceChunk_t *nextchunk;

  ResourceChunk_clear(chunk,-1,LUX_FALSE);
  lxMemoryStack_delete(chunk->mempool);

  nextchunk = chunk->next;
  lxListNode_remove(g_ResData.chunkListHead,chunk);
  lxMemGenFree(chunk,sizeof(ResourceChunk_t));

  g_ResData.numChunks--;

  if (g_ResData.chunkActive == chunk && !l_RESData.exitclear){
     ResourceChunk_activate((chunk == nextchunk ) ? NULL : nextchunk);
  }
  vidResetTexture();
}

size_t  ResourceChunk_bytesLeft(ResourceChunk_t *chunk)
{
  return lxMemoryStack_bytesLeft(chunk->mempool);
}

// make the chunk the one used for new resources
void  ResourceChunk_activate(ResourceChunk_t *chunk)
{
  if (!chunk){
    bprintf("ERROR: illegal reschunk activated\n");
    Main_exit(LUX_FALSE);
  }
  g_ResMemStack = chunk->mempool;
  g_ResData.chunkActive = chunk;
}



//pushes all 'direct' or 'outref' to resreloadstack
void  ResourceChunk_push(ResourceChunk_t *chunk, int depth, int outref)
{
  int i;
  ResourceType_t type;

  ResourceArray_t *resarray;
  ResourceContainer_t *rescont;
  ResourcePtr_t   *resptr;

  rescont = chunk->containers;
  resarray = g_ResData.resarray;

  // FIXME, must pump with decreasing depth
  // cause that way referenced resources stay at same place first

  for (type = 0; type < RESOURCES; type++,resarray++,rescont++) {
    resptr = &resarray->ptrs[rescont->resOffset];
    for (i = 0; i < rescont->resCnt; i++,resptr++) {
      if (resptr->resinfo){
        rescont->resUsed--;
        resarray->numLoaded--;
        if ((depth >= 0 && resptr->resinfo->depth <= depth) ||
          (outref && resptr->resinfo->outerRefCnt))
        {
          if(!ResourceReload_push(type,resptr->resinfo))
            resptr->resinfo = NULL;
          else{
            lprintf(" - %s\n",resptr->resinfo->name);
            resptr->resinfo = RES_PTR_RESERVE;
          }
        }
        else
          resptr->resinfo = NULL; // released the resRID
      }
    }
  }
}

void ResourceChunk_clearBOs(ResourceChunk_t *chunk)
{
  HWBufferObject_t *resbo;
  int i;

  for (i = 0; i < RESOURCE_BOS; i++){
    HWBufferObject_t *resboLH = chunk->vbosListHead[i];
    lxListNode_forEach(resboLH,resbo)
      VIDBuffer_clearGL(&resbo->buffer);
    lxListNode_forEachEnd(resboLH,resbo);
  }
}

void ResourceChunk_deleteBOs(ResourceChunk_t *chunk)
{
  int i;

  for (i = 0; i < RESOURCE_BOS; i++){
    while (chunk->vbosListHead[i]){
      //lxListNode_popFront(chunk->vbosListHead[i],resbo);
      HWBufferObject_t* resbo = chunk->vbosListHead[i];
      HWBufferObject_listop(resbo,NULL);
      VIDBuffer_clearGL(&resbo->buffer);
      // prevent GC call
      Reference_invalidate(resbo->reference);
      Reference_release(resbo->reference);
    }
  }
}

// clears all data within the chunk, if resources exist that where referenced from outside
// they will be reloaded with same id
void  ResourceChunk_clear(ResourceChunk_t *chunk, int keepdepth, int reloadoutref)
{
  int i;
  ResourceType_t type;

  ResourceArray_t *resarray;
  ResourceContainer_t *rescont;
  ResourcePtr_t   *resptr;


  rescont = chunk->containers;
  resarray = g_ResData.resarray;

  for (type = 0; type < RESOURCES; type++,resarray++,rescont++) {
    resptr = &resarray->ptrs[rescont->resOffset];
    if (g_ResDescriptor[type].clearfunc){
      for (i = 0; i < rescont->resCnt; i++,resptr++) {
        if (resptr->resinfo && resptr->resinfo->typeID != g_ResDescriptor[type].typeID){
          bprintf("ERROR: illegal resource in chunk %s\n",chunk->name);
          Main_exit(LUX_FALSE);
        }

        if (resptr->resinfo)
          g_ResDescriptor[type].clearfunc(resptr->resinfo);
      }
    }
  }
  ResourceReload_resetAll();
  lprintf("RESCHUNK: push\n");
  ResourceChunk_push(chunk,keepdepth,reloadoutref);

  // reset BOs must be done before mempool is cleared !
  ResourceChunk_deleteBOs(chunk);

  lxMemoryStack_clear(chunk->mempool);


  l_RESData.actchunk = chunk;
  ResourceChunk_activate(chunk);
  l_RESData.fillchunk = NULL;
  lprintf("RESCHUNK: pop\n");
  // the order is important
  ResourceReload_popAll(RESOURCE_TEXTURE);
  ResourceReload_popAll(RESOURCE_GPUPROG);
  ResourceReload_popAll(RESOURCE_SHADER);
  ResourceReload_popAll(RESOURCE_MATERIAL);
  ResourceReload_popAll(RESOURCE_MODEL);
  ResourceReload_popAll(RESOURCE_PARTICLESYS);
  ResourceReload_popAll(RESOURCE_PARTICLECLOUD);
  ResourceReload_popAll(RESOURCE_SOUND);
  ResourceReload_popAll(RESOURCE_ANIM);
  ResourceReload_resetAll();

  ResourceChunk_activate(l_RESData.actchunk);
  Main_freezeTime();
  l_RESData.lastreload = g_VID.frameCnt;
}

int ResourceChunk_getFreeRID(ResourceChunk_t *chunk,ResourceType_t type)
{
  int i;
  ResourceArray_t *resarray = &g_ResData.resarray[type];
  ResourceContainer_t *rescont = &chunk->containers[type];

  if (rescont->resUsed >= rescont->resCnt)
    return -1;

  for (i = rescont->resOffset; i < rescont->resOffset+rescont->resCnt; i++) {
    if (!resarray->ptrs[i].resinfo)
      return i;
  }

  return -1;
}


//////////////////////////////////////////////////////////////////////////
// ResData

// basic initialization creates the default chunk
void  ResData_clear()
{
  ResourceArray_t *resarray;
  ResourceType_t  i;
  int used = 0;
  memset(&g_ResData,0,sizeof(ResourceData_t));

  // the default chunk also sets up all pointer arrays
  resarray = g_ResData.resarray;
  for (i = 0; i < RESOURCES; i++,resarray++){
    resarray->ptrs = &g_ResData.resptrs[used];
    resarray->numMax = g_ResDescriptor[i].max;
    used += g_ResDescriptor[i].max;
  }

  g_ResData.newIBOsize = RES_IBO_MEM_KB;
  g_ResData.newVBOsize = RES_VBO_MEM_KB;
}
void  ResData_init(uint defmemsizeKB)
{
  float defaultcnts[RESOURCES];
  ResourceType_t  i;
  size_t corememsizeKB = 0;

  ResData_clear();

  for (i = 0; i < RESOURCES; i++){
    defaultcnts[i] = g_ResDescriptor[i].core;
    corememsizeKB += g_ResDescriptor[i].core*(g_ResDescriptor[i].size+2048);
    corememsizeKB += sizeof(ResourcePtr_t)*g_ResDescriptor[i].max;
  }

  corememsizeKB /= 1024;
  corememsizeKB += 16;  // add a few KBs buffer


  memset(&l_RESData,0,sizeof(struct RESData_s));

  l_RESData.exitclear = LUX_FALSE;
  l_RESData.forcedOverwrite = LUX_FALSE;
  l_RESData.forceload = RESOURCE_NONE;
  l_RESData.forcedRID = -1;
  l_RESData.indirectlevel = 0;
  l_RESData.lastreload = 0;
  l_RESData.fillchunk = NULL;
  l_RESData.actchunk = NULL;

  l_RESData.cgcstack = 1;
  memset((void*)l_RESData.cgcstackptrs,0,sizeof(l_RESData.cgcstackptrs));
  l_RESData.cgcstackptrs[0] = l_RESData.compilerargs;
  l_RESData.curcgcfullstr = NULL;

  ResourceChunk_activate(ResourceChunk_new(RES_CORE_NAME,defaultcnts,corememsizeKB));

  for (i = 0; i < RESOURCES; i++)
    defaultcnts[i] = g_ResDescriptor[i].max;

  ResourceChunk_activate(ResourceChunk_new(RES_DEFAULT_NAME,defaultcnts,defmemsizeKB));

  memset(l_RESData.curpath,0,sizeof(char)*6*1024);

  l_RESData.userTextures = lxStrMap_new(GLOBAL_ALLOCATOR,128,0,NULL);

  ResData_setCgCstringBase(NULL);
}

// resets everything, frees all chunks
void  ResData_deinit()
{
  ResourceChunk_t *chunk;
  ResourceChunk_t *firstchunk;

  l_RESData.exitclear = LUX_TRUE;

  lxListNode_popFront(g_ResData.chunkListHead,firstchunk);

  while (g_ResData.chunkListHead){
    lxListNode_popBack(g_ResData.chunkListHead,chunk);
    ResourceChunk_free(chunk);
  }

  // we skip the first chunk, as it is the default one
  // and holds also the array ptr memory
  ResourceChunk_free(firstchunk);

  ResData_clear();
  g_ResMemStack = NULL;

  lxStrMap_delete(l_RESData.userTextures,NULL);
}

ResourceChunk_t* ResData_getChunkActive()
{
  return g_ResData.chunkActive;
}

ResourceChunk_t* ResData_getChunkDefault(){
  if (strcmp(g_ResData.chunkListHead->next->name,RES_DEFAULT_NAME)==0)
    return g_ResData.chunkListHead->next;
  else
    return NULL;

}

// returns previous active chunk, and sets active chunk to the one of resinfo
ResourceChunk_t* ResData_setChunkFromPtr(ResourceInfo_t *resinfo)
{
  ResourceChunk_t *old = g_ResData.chunkActive;

  ResourceChunk_activate(resinfo->reschunk);

  return old;
}

void  ResData_freeDefault()
{
  ResourceType_t type;
  ResourceArray_t *resarray;

  if (strcmp(g_ResData.chunkListHead->next->name,RES_DEFAULT_NAME)==0) {
    ResourceChunk_free(g_ResData.chunkListHead->next);

    resarray = g_ResData.resarray;
    for (type = 0; type < RESOURCES; type++,resarray++){
      resarray->chunkOffset = g_ResDescriptor[type].core;
    }
  }
}

ulong ResData_lastReloadFrame()
{
  return l_RESData.lastreload;
}

void  ResData_strategy(ResourceFill_t type)
{
  g_ResData.strategy = type;
}

// runs thru all memorypools and sums their inuse values
size_t    ResData_getTotalMemUse()
{
  size_t sum = 0;
  ResourceChunk_t *chunk;

  lxListNode_forEach(g_ResData.chunkListHead,chunk)
    sum += lxMemoryStack_bytesUsed(chunk->mempool);
  lxListNode_forEachEnd(g_ResData.chunkListHead,chunk);

  return sum;
}
// runs thru all memorypools and sums their preallocated sizes
size_t    ResData_getTotalMem()
{
  size_t sum = 0;
  ResourceChunk_t *chunk;

  lxListNode_forEach(g_ResData.chunkListHead,chunk)
    sum += lxMemoryStack_bytesTotal(chunk->mempool);
  lxListNode_forEachEnd(g_ResData.chunkListHead,chunk);

  return sum;
}

void  ResData_forceLoad(ResourceType_t restype)
{
  l_RESData.forceload = restype;
}

int ResData_search(ResourceType_t type ,const char *name, const char *extraname){
  int i;
  ResourcePtr_t *resptr;

  if (type == l_RESData.forceload)
    return -1;

  resptr = g_ResData.resarray[type].ptrs;
  for (i = 0; i < g_ResData.resarray[type].numMax; i++,resptr++){
    if (resptr->resinfo &&  resptr->resinfo != RES_PTR_RESERVE && cmpfilenamesi(resptr->resinfo->name, name) == 0 &&
      ((extraname == resptr->resinfo->extraname)
      || (extraname && resptr->resinfo->extraname
      && strcmp(resptr->resinfo->extraname,extraname)==0)))
      return i;
  }

  return -1;
}

int   ResData_getFreeRID(ResourceType_t type)
{
  int i;
  ResourceChunk_t *reschunk;

  if (g_ResData.resarray[type].numLoaded >= g_ResData.resarray[type].numMax){
    lprintf("ERROR resadd %s: maximum reached !\n",g_ResDescriptor[type].name);
    return -1;
  }

  // we came from an auto fill
  if (l_RESData.fillchunk && g_ResData.chunkActive!=g_ResData.chunkListHead){
    ResourceChunk_activate(l_RESData.fillchunk);
    l_RESData.fillchunk = NULL;
  }

  if (l_RESData.forcedRID >= 0){
    i = l_RESData.forcedRID;
    // only the first resource is affected
    l_RESData.forcedRID = -1;
    return  i;
  }

  i = ResourceChunk_getFreeRID(g_ResData.chunkActive,type);

  if (i < 0)
  switch(g_ResData.strategy) {
  case RESOURCE_FILL_TONEXT:
    // we just try the next chunks to the end of the list
    reschunk = g_ResData.chunkActive;
    while(reschunk != g_ResData.chunkListHead){
      i =  ResourceChunk_getFreeRID(reschunk,type);
      if (i >= 0){
        l_RESData.fillchunk = g_ResData.chunkActive;
        ResourceChunk_activate(reschunk);
        break;
      }
      reschunk = reschunk->next;
    }
    break;
  default:
    i = -1;
  }

  if (i < 0)
    lprintf("ERROR resadd %s: chunks full !\n",g_ResDescriptor[type].name);

  return i;
}
  // only handles referenceing/marking as direct
void  ResData_refResource(ResourceType_t type, int index)
{
  ResourceInfo_t *resinfo = ResData_getInfo(index,type);
  LUX_ASSERT(resinfo);

  if (l_RESData.indirectlevel){
    resinfo->userRefCnt++;
  }

  resinfo->depth = l_RESData.indirectlevel;

  if (resinfo->reschunk != g_ResData.chunkActive)
    resinfo->outerRefCnt++;
}

  // called from the external clear functions, they deref their indirect resources
void  ResData_unrefResourcePtr(ResourceType_t type,void *resinfoUnref, void *resinfoSrc)
{
  ResourceInfo_t *resinfo = (ResourceInfo_t*)resinfoUnref;

  if (!resinfo || resinfo->typeID!=g_ResDescriptor[type].typeID)
    return;

  if (resinfo->reschunk != ((ResourceInfo_t*)resinfoSrc)->reschunk)
    resinfo->outerRefCnt--;
  resinfo->userRefCnt--;
}
void  ResData_unrefResource(ResourceType_t type, int index, void *resinfoSrc)
{
  if (index < 0)
    return;

  ResData_unrefResourcePtr(type,ResData_getInfo(index,type),resinfoSrc);
}

  // clears resourceptr /leaks mem
void  ResData_clearResource(ResourceType_t type,int index){
  ResourceArray_t *resarray = &g_ResData.resarray[type];
  resarray->ptrs[index].resinfo = NULL;
}


  // adds resource, allocates it and referenes/marks as direct if needed
void* ResData_addResource(ResourceType_t type,int index, const char *name, const char *extraname){
  ResourceArray_t *resarray = &g_ResData.resarray[type];
  ResourceInfo_t  *resinfo;

  if (index < 0)
    return NULL;

  if (l_RESData.forcedOverwrite){
    l_RESData.forcedOverwrite = LUX_FALSE;
    LUX_ASSERT(resarray->ptrs[index].resinfo);
    return (void*)resarray->ptrs[index].resinfo ;
  }

  resarray->numLoaded++;
  g_ResData.chunkActive->containers[type].resUsed++;

  resinfo = resarray->ptrs[index].resinfo = reszallocSIMD(g_ResDescriptor[type].size);
  LUX_ASSERT(resinfo);
  resinfo->resRID = index;
  resinfo->typeID = g_ResDescriptor[type].typeID;
  resinfo->reschunk = g_ResData.chunkActive;

  if (name)
    resnewstrcpy(resinfo->name,name);
  if (extraname)
    resnewstrcpy(resinfo->extraname,extraname);

  if (l_RESData.indirectlevel)
    resinfo->userRefCnt++;

  resinfo->depth = l_RESData.indirectlevel;

  return (void*)resinfo;
}


void  ResData_CoreChunk( int state)
{
  static  ResourceChunk_t *activechunk = NULL;
  if (state && g_ResData.chunkActive != g_ResData.chunkListHead){
    activechunk = g_ResData.chunkActive;
    ResourceChunk_activate(g_ResData.chunkListHead);
  }
  else if (activechunk){
    ResourceChunk_activate(activechunk);
  }
}

int   ResData_getOpenCnt(ResourceType_t type)
{
  ResourceArray_t *resarray = &g_ResData.resarray[type];
  return (resarray->numMax-resarray->chunkOffset);
}

int   ResData_getLoadCnt(ResourceType_t type)
{
  ResourceArray_t *resarray = &g_ResData.resarray[type];
  return (resarray->numLoaded);
}

// if a resource references others it should mark the passage when it happens
void ResData_IndirectStart(char *filename){
  l_RESData.indirectlevel++;
  lxStrGetPath(l_RESData.curpath[l_RESData.indirectlevel],filename);
}
void ResData_IndirectEnd(){
  l_RESData.indirectlevel--;
}

char* ResData_getCurPath(){
  return &l_RESData.curpath[l_RESData.indirectlevel][0];
}

void  ResData_overwriteRID(int id)
{
  l_RESData.forcedRID = id;
  l_RESData.forcedOverwrite = LUX_TRUE;
}

//////////////////////////////////////////////////////////////////////////
// ResourceBO


// returns vboid and offsetsize
// type 0 = vbo, 1 = ibo
int   ResData_getBO(ResourceInfo_t *resinfo,ResourceBOType_t type, uint size,HWBufferObject_t **outbo,int padsize)
{
  ResourceChunk_t *oldchunk = ResData_setChunkFromPtr(resinfo);
  ResourceChunk_t *reschunk = g_ResData.chunkActive;
  HWBufferObject_t *resbo;
  HWBufferObject_t *resboLH = reschunk->vbosListHead[type];
  size_t defaultsize = (type ? g_ResData.newIBOsize : g_ResData.newVBOsize)*1024;
  //size_t defaultsize = (type ? g_ResData.newVBOsize : g_ResData.newIBOsize)*1024;

  int found = LUX_FALSE;
  int offset;

  // pad to biggest
  offset = (size%padsize);
  size += offset ? (padsize-offset) : 0;


  // check if space is left, if not create new vbo
  lxListNode_forEach(resboLH,resbo)
    if (resbo->buffer.size-resbo->buffer.used > size){
      found = LUX_TRUE;
      break;
    }
  lxListNode_forEachEnd(resboLH,resbo);

  if (!found){
    resbo = reszalloc(sizeof(HWBufferObject_t));
    resbo->reference = Reference_new(LUXI_CLASS_VIDBUFFER,resbo);

    VIDBuffer_initGL(&resbo->buffer,type,VID_BUFFERHINT_STATIC_DRAW,LUX_MAX(size,defaultsize),NULL);
    resbo->buffer.host = resbo->reference;
    HWBufferObject_listop(resbo,&reschunk->vbosListHead[type]);
  }
  else if (!resbo->buffer.glID){
    VIDBuffer_initGL(&resbo->buffer,type,VID_BUFFERHINT_STATIC_DRAW,LUX_MAX(size,defaultsize),NULL);
  }

  ResourceChunk_activate(oldchunk);

  *outbo = resbo;
  offset = resbo->buffer.used;
  resbo->buffer.used+=size;

  return offset;
}




/*
=================================================================
BUILDING RESOURCE DATA
=================================================================
*/


// MAIN FUNCTION to upload models into memory
// calls visualUploadTexture with texture found in model
// returns index of the model in visdata

void ResData_ModelInit(Model_t *model,int onlydata, const int needlm)
{
  uchar     *bumpmap;
  uchar     *shader;
  MeshObject_t  *tempM;
  int i;

  // load texture of meshes
  shader = NULL;
  bumpmap = NULL;


  // raise indirection cause we load other resources
  ResData_IndirectStart(model->resinfo.name);

  // ignore material data
  if (!onlydata)
    for (i = 0; i < model->numMeshObjects;i++){
      tempM = &model->meshObjects[i];
      if (tempM->texRID){
        char *str = strstr(model->meshObjects[i].texname,";");
        if (!str)
          str = model->meshObjects[i].texname;
        else
          str++;

        if (*str){
          tempM->texRID = ResData_addTexMat(str);

          if (vidMaterial(tempM->texRID)){
            if (!shader){
              shader = lxMemGenAlloc(sizeof(uchar)*model->numMeshObjects);
              clearArray(shader,model->numMeshObjects);
            }
            shader[i] = LUX_TRUE;
            if (ResData_getMaterialShader(tempM->texRID)->shaderflag & SHADER_TANGENTS && (model->vertextype == VERTEX_64_TEX4 || model->vertextype == VERTEX_64_SKIN)){
              if (!bumpmap){
                bumpmap = lxMemGenAlloc(sizeof(uchar)*model->numMeshObjects);
                clearArray(bumpmap,model->numMeshObjects);
              }
              bumpmap[i] = LUX_TRUE;
            }
          }
        }
        else
          tempM->texRID = -1;
      }
      else
        tempM->texRID = -1;
    }
    ResData_IndirectEnd();

    // omg we have bumpmapping
    if (bumpmap){
      // find out which meshes and compute
      for (i = 0; i < model->numMeshObjects;i++){
        tempM=&model->meshObjects[i];
        if (bumpmap[i]){
          Mesh_createTangents(tempM->mesh);
        }
      }
    }

    // create the proper skinweights and relative vertices
    // we deferred this cause before the tangent space was computed
    Model_initSkin(model);

    // do conversions of modeltypes
    // if we dont have nvi nor ati, I got no clue if their display lists are properly supporting mtex
    // also we need to manually do relfectmap if ext not given


    Model_initGL(model,MESH_UNSET,needlm,LUX_TRUE);

    if (shader)
      lxMemGenFree(shader,sizeof(uchar)*model->numMeshObjects);
    shader = NULL;

    if (bumpmap)
      lxMemGenFree(bumpmap,sizeof(uchar)*model->numMeshObjects);
    bumpmap = NULL;

}
int ResData_addModel(char *name, ModelType_t modeltype,const lxVertexType_t vtype, const int needlm)
{
  int resRID;
  uchar     onlydata;
  int       reflect;

  Model_t     *model;

  uchar   nodl;
  uchar   novbo;
  uchar   nofile = LUX_FALSE;

  char      filename[RES_MAXSTRLENGTH] = {0};
  char      *dir = g_ResDescriptor[RESOURCE_MODEL].path;

  if (!name)
    return -1;

  if (strstr(name,MODEL_USERSTART)!=NULL){
    nofile = LUX_TRUE;
    name+=MODEL_USERSTART_LEN;
    strcpy(filename,name);
  }// process filename
  else if (!FS_setProjectFilename(filename,name,dir,ResData_getCurPath(),&l_RESData.loadertype)){
    lprintf("ERROR resadd %s: file not found %s\n",g_ResDescriptor[RESOURCE_MODEL].name,filename);
    return -1;
  }

  // check if loaded, return
  resRID = ResData_search(RESOURCE_MODEL,filename,NULL);
  if(resRID > -1) {
    ResData_refResource(RESOURCE_MODEL,resRID);
    return resRID;
  }
  // create new
  resRID = ResData_getFreeRID(RESOURCE_MODEL);
  if (resRID < 0)
    return -1;
  model = ResData_addResource(RESOURCE_MODEL,resRID,filename,NULL);
  model->orgtype = model->modeltype = modeltype;
  model->vertextype = vtype;

  if (nofile){
    lxVector3Set(model->prescale,1,1,1);
    return resRID;
  }

  if (modeltype == MODEL_ONLYDATA){
    onlydata = LUX_TRUE;
    modeltype = MODEL_STATIC;
  }
  else
    onlydata = LUX_FALSE;


  switch(modeltype) {
  case MODEL_STATIC_NODL_NOVBO:
  case MODEL_DYNAMIC_NODL_NOVBO:
    novbo = LUX_TRUE;
    nodl = LUX_TRUE;
    modeltype = modeltype-3;
    break;
  case MODEL_STATIC_NOVBO:
  case MODEL_DYNAMIC_NOVBO:
    novbo = LUX_TRUE;
    nodl = LUX_FALSE;
    modeltype = modeltype-2;
    break;
  case MODEL_STATIC_NODL:
  case MODEL_DYNAMIC_NODL:
    nodl = LUX_TRUE;
    novbo = LUX_FALSE;
    modeltype = modeltype-1;
    break;
  default:
    nodl = LUX_FALSE;
    novbo = LUX_FALSE;
    break;
  }

  model->modeltype = modeltype;

  // load model
  // create imodel
  reflect = FS_processLoader(l_RESData.loadertype,model->resinfo.name,model,NULL);

  if (!reflect)
  {
    lprintf("ERROR resmodel: %s failed on load\n",name);
    bufferclear();
    ResData_clearResource(RESOURCE_MODEL,resRID);
    return -1;
  }
  reflect = 0;


  // major conversion
  if (!ModelLoader_postInit(model))
  {
    lprintf("ERROR model post load: %s\n",name);
    bufferclear();
    ResData_clearResource(RESOURCE_MODEL,resRID);
    return -1;
  }

  // kill buffer
  bufferclear();

  // all the following can be skipped when no rendercontext available
  ResData_ModelInit(model,onlydata,needlm);

  // we are done

  return resRID;
}

int ResData_addTexMat(char *name)
{
  char      filename[RES_MAXSTRLENGTH] = {0};
  char      extname[RES_MAXSTRLENGTH] = {0};

  if (!name)
    return -1;
  if (strstr(name,MATERIAL_AUTOSTART))
    return ResData_addMaterial(name,NULL);

  // use shader if assigned directly or detail setting suggest to look for shaders
  if (strstr(name, ".mtl") != NULL || strstr(name, ".MTL") != NULL || g_VID.details != VID_DETAIL_LOW){
    char *dir = g_ResDescriptor[RESOURCE_MATERIAL].path;
    char matext[]=".mtl";
    int out;

    strcpy(extname,name);

    // add extension string if needed
    if (strstr(name, ".mtl") == NULL && strstr(name, ".MTL") == NULL)
      strcat(extname,matext);

    // check if file exists if yes upload material
    out = (FS_setProjectFilename(filename,extname,dir,ResData_getCurPath(),&l_RESData.loadertype)) ? ResData_addMaterial(extname,NULL) : -1;
    if (out > -1)
      return out; // return the material id
  }
  return ResData_addTexture(name,TEX_COLOR,TEX_ATTR_MIPMAP);
}

int ResData_addMaterial(char *name,const char *cgcstring)
{
  int resRID;
  int nofile;
  int i,n;
  Material_t  *material;
  MaterialStage_t *stage;
  Shader_t  *shader;
  char    filename[RES_MAXSTRLENGTH] = {0};
  const char  *extname;
  char    *dir = g_ResDescriptor[RESOURCE_MATERIAL].path;

  if (!name)
    return -1;

  nofile = LUX_FALSE;
  if (strstr(name,MATERIAL_AUTOSTART)){
    strcpy(filename,name);
    nofile = LUX_TRUE;
  }


  // process filename
  if (!nofile &&! FS_setProjectFilename(filename,name,dir,ResData_getCurPath(),&l_RESData.loadertype)){
    lprintf("ERROR resadd %s: file not found %s\n",g_ResDescriptor[RESOURCE_MATERIAL].name,filename);
    return -1;
  }

  // cgcstring needs to be concat
  extname = ResData_concatCgCstring(cgcstring);
  l_RESData.curcgcfullstr = extname;


  // check if loaded, return
  resRID = ResData_search(RESOURCE_MATERIAL,filename,extname);
  if(resRID != -1) {
    ResData_refResource(RESOURCE_MATERIAL,resRID);
    return resRID + VID_OFFSET_MATERIAL;
  }
  // create new
  resRID = ResData_getFreeRID(RESOURCE_MATERIAL);
  if (resRID < 0)
    return -1;
  material = ResData_addResource(RESOURCE_MATERIAL,resRID,filename,extname);

  LUX_SIMDASSERT((size_t)((Material_t*)NULL)->matobjdefault.texmatrixData  % 16 == 0);



  // load texture
  if (!nofile && !FS_processLoader(l_RESData.loadertype,filename,material,NULL))
  {
    return ResData_addTexture("?default",TEX_COLOR,0);
  }
  else if (nofile && !Material_setFromString(material,filename)){
    return ResData_addTexture("?default",TEX_COLOR,0);
  }

  lprintf("\tCg Compiler: %s\n",extname);

  ResData_IndirectStart(filename);
  ResData_pushCgCstring(cgcstring);

  if (nofile && l_RESData.indirectlevel)
    strcpy(l_RESData.curpath[l_RESData.indirectlevel],l_RESData.curpath[l_RESData.indirectlevel-1]);
  else if (nofile)
    strcpy(l_RESData.curpath[l_RESData.indirectlevel],"./");

  material->shaders[0].resRID = material->shaders[0].resRIDUse = ResData_addShader(material->shaders[0].name,material->shaders[0].extname);

  if (material->shaders[0].resRID == -1){
    ResData_clearResource(RESOURCE_MATERIAL,resRID);
    ResData_IndirectEnd();
    ResData_popCgCstring();
    return -1;
  }
  for (i = 1 ; i < MATERIAL_MAX_SHADERS; i++){
    if (material->shaders[i].name)
      material->shaders[i].resRID = material->shaders[i].resRIDUse = ResData_addShader(material->shaders[i].name,material->shaders[i].extname);
  }
  shader = ResData_getShader(material->shaders[0].resRID);

  material->noTexgenOrMat = BIT_ID_FULL32;
  for (i = 0; i < material->numStages; i++){
    ShaderStage_t *sstage;
    stage = material->stages[i];
    n = 0;
    for (n= 0; n < MATERIAL_MAX_SHADERS; n++){
      if ((shader = ResData_getShader(material->shaders[n].resRID)) &&
        (sstage=Shader_searchStageRID(shader,stage->id)))
        break;
    }
    if (n >= MATERIAL_MAX_SHADERS){
      stage->id = MATERIAL_UNUSED;
      continue;
    }
    if (stage->texName){
      char *texname = stage->texName;

      stage->texRID = ResData_addTexture(texname,stage->texType,stage->texAttributes);
      stage->texRIDUse = stage->texRID;
      if (stage->texclamp)
        ResData_setTextureClamp(stage->texRID,stage->texclamp);
    }

    if (ResData_getTexture(stage->texRID))
      stage->windowsized = ResData_getTexture(stage->texRID)->windowsized;

    if (stage->seq){
      for (n = 0; n < stage->seq->numFrames; n++){
        stage->seq->texRID[n] = ResData_addTexture(stage->seq->textures[n],stage->seq->texTypes[n],0);
      }
    }
    if (stage->texgen || stage->texcoord)
      material->noTexgenOrMat = LUX_FALSE;
  }

  if (!Material_initShaders(material)){
    ResData_IndirectEnd();
    ResData_popCgCstring();
    return -1;
  }

  // because shaderparams got new pointers
  Material_initControls(material);
  Material_initModifiers(material);
  Material_initResControls(material);
  Material_initMatObject(material);

  ResData_popCgCstring();
  ResData_IndirectEnd();
  return resRID + VID_OFFSET_MATERIAL;
}

int ResData_addTexture(char *name,TextureType_t type,int attributes)
{
  int resRID;
  static char   typeattrib[RES_MAXSTRLENGTH] = {0};
  static char   username[RES_MAXSTRLENGTH] = {0};
  static char   userinstruct[RES_MAXSTRLENGTH] = {0};
  static char   filename[RES_MAXSTRLENGTH] = {0};
  static char   buffer[RES_MAXSTRLENGTH] = {0};
  char    *names[6] = {0,0,0,0,0,0};

  Texture_t *texture;
  char    *user = NULL;
  char    *out = NULL;
  char    *dir = g_ResDescriptor[RESOURCE_TEXTURE].path;
  int nofile;
  int nocompress = LUX_FALSE;
  int normalize = LUX_FALSE;
  int normal2light = LUX_FALSE;

  typeattrib[0] = 0;
  username[0] = 0;
  userinstruct[0] = 0;
  filename[0]= 0;
  buffer[0] = 0;

  if (!name || type == TEX_UNSET)
    return -1;

  if ((user=strstr(name,TEX_USERSTART)) || name[0] == (char)'?')
    nofile = LUX_TRUE;
  else
    nofile = LUX_FALSE;

  if (strstr(name,"lightmap"))
    nocompress = LUX_TRUE;

  if (!user && (out = strrchr(name,';'))){  // was strstr
    char *origname = name;
    name = out+1;

    if ((out=strstr(origname,"nocompress")) && out < name)
    {
      nocompress = LUX_TRUE;
    }
    if ((out=strstr(origname,"normal2light")) && out < name)
    {
      normal2light = LUX_TRUE;
    }
    if ((out=strstr(origname,"normalize")) && out < name)
    {
      normalize = LUX_TRUE;
    }

    if ((out=strstr(origname,"dxt1")) && out < name)
    {
      attributes |= TEX_ATTR_COMPR_DXT1;
    }
    if ((out=strstr(origname,"dxt3")) && out < name)
    {
      attributes |= TEX_ATTR_COMPR_DXT3;
    }
    if ((out=strstr(origname,"dxt5")) && out < name)
    {
      attributes |= TEX_ATTR_COMPR_DXT5;
    }
    out = NULL;
  }

  // comma separated name assumes cubemap
  if (!user && strstr(name,",") && (attributes & TEX_ATTR_CUBE)){
    strcpy(buffer,name);
    return ResData_addTextureCombine(names,lxStrSplitList(buffer,names,',',6),type,LUX_FALSE,attributes);
  }




  // process filename
  if (user){
    Texture_splitUserString(name,username,userinstruct);

    if (resRID = ResData_getUserTexture(username))
      return resRID;
    else if (!*userinstruct){
      bprintf("WARNING restex: user texture not found %s\n",username);

      nofile = LUX_FALSE;
      l_RESData.loadertype = FILELOADER_NONE;
    }

    out = userinstruct;
    strcpy(filename,name);
  }
  else if (nofile){
    strcpy(filename,name);

    resRID = VID_CheckSpecialTexs(name);
    if(resRID != -1) {
      ResData_refResource(RESOURCE_TEXTURE,resRID);
      return resRID;
    }
  }
  else if ((user=strstr(name,".any"))|| (user=strstr(name,".ANY"))){
    char *autlookup[] = {"dds","png","jpg","tga",NULL};
    char **lookup = autlookup;
    // try dds, jpg, then tga
    user++;
    do{
      strcpy(user,*lookup);
    }while(!FS_setProjectFilename(filename,name,dir,ResData_getCurPath(),&l_RESData.loadertype) && *++lookup);
  }
  else
    FS_setProjectFilename(filename,name,dir,ResData_getCurPath(),&l_RESData.loadertype);

  if (!nofile && (l_RESData.loadertype != FILELOADER_DDS && l_RESData.loadertype != FILELOADER_PNG &&
    l_RESData.loadertype != FILELOADER_JPG && l_RESData.loadertype != FILELOADER_TGA))
  {
    lprintf("WARNING restex: returning default, illegal image lodadertype / file not found: %s\n",name);

    return g_VID.gensetup.texdefaultRID;
  }

  // put attrib and type into string
  strcpy(typeattrib,TextureType_toString(type));


  // check if loaded, return
  resRID = ResData_search(RESOURCE_TEXTURE,filename,typeattrib);
  if(resRID != -1) {
    ResData_refResource(RESOURCE_TEXTURE,resRID);
    return resRID;
  }
  // create new
  resRID = ResData_getFreeRID(RESOURCE_TEXTURE);
  if (resRID < 0)
    return -1;
  texture = ResData_addResource(RESOURCE_TEXTURE,resRID,filename,typeattrib);
  texture->attributes = attributes;

  // load texture
  if (!nofile && !FS_processLoader(l_RESData.loadertype,filename,texture,NULL))
  {
    lprintf("WARNING restex: load failed, creating default texture %s\n",name);

    Texture_makeDefault(texture);
    return resRID;
  }

  /*
  if ((type == TEX_CUBE_COLOR_PROJ || type == TEX_CUBE_COLOR_PROJ_MIPMAP ||
    type == TEX_CUBE_COLOR_DOTZ || type == TEX_CUBE_COLOR_DOTZ_MIPMAP) && !GLEW_ARB_texture_cube_map){
    if (type == TEX_CUBE_COLOR_PROJ || type == TEX_CUBE_COLOR_DOTZ)
      type = TEX_2D_COLOR;
    else
      type = TEX_2D_COLOR_MIPMAP;
    lprintf("WARNING: restex: no cubemapping support for TEX_PROJ/TEX_DOTZ\n");
  }
  */

  if (nocompress){
    nocompress = g_VID.useTexCompress;
    g_VID.useTexCompress = LUX_FALSE;
  }
  else
    nocompress = g_VID.useTexCompress;

  if (out){
    type = Texture_makeFromString(texture,out);
    ResData_addUserTexture(resRID,username);
  }


  Texture_init(texture,type);
  if (normal2light){
    //Texture_normal2light(texture);
  }
  else if (normalize){
    //Texture_makeMipMaps(texture);
    //Texture_normalize(texture);
  }


  Texture_initGL(texture,LUX_FALSE);

  g_VID.useTexCompress = nocompress;

  return resRID;
}
int ResData_addTextureCombine(char **names, int num,TextureType_t type,int packchannel, int attributes)
{
  int resRID;
  Texture_t *texture;
  Texture_t* temptex;
  int i;
  char    combinename[RES_MAXSTRLENGTH] = {0};
  char    filename[RES_MAXSTRLENGTH] = {0};
  char    *dir = g_ResDescriptor[RESOURCE_TEXTURE].path;
  TextureCombineModel_t mode;

  if (!names)
    return -1;

  if (num < 2)
    return ResData_addTexture(names[0],type,attributes);

  // we give combined a special flag
  sprintf(combinename,"%d|%d||",num,packchannel);
  for (i = 0; i < num; i++){
    if(!FS_setProjectFilename(filename,names[i],dir,ResData_getCurPath(),&l_RESData.loadertype)){
      lprintf("ERROR restexcomb: sequence member failed %s\n",names[i]);
      return ResData_addTexture("?default",TEX_COLOR,0);
    }
    strcat(combinename,filename);
    strcat(combinename,"|");
  }


  // check if loaded, return
  resRID = ResData_search(RESOURCE_TEXTURE,combinename,NULL);
  if(resRID != -1) {
    ResData_refResource(RESOURCE_TEXTURE,resRID);
    return resRID;
  }

  temptex = bufferzalloc(sizeof(Texture_t)*num);
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

  if (packchannel)
    mode = TEX_COMBINE_PACK;
  else if ((attributes & TEX_ATTR_CUBE) && num == 6)
    mode = TEX_COMBINE_CONCAT;
  else
    mode = TEX_COMBINE_INTERLEAVE;

  // load textures
  for (i = 0; i < num; i++){
    FS_setProjectFilename(filename,names[i],dir,ResData_getCurPath(),&l_RESData.loadertype);
    if (!FS_processLoader(l_RESData.loadertype,filename,&temptex[i],NULL) || temptex[i].mipmaps)
    {
      lprintf("ERROR restexcomb: sequence member failed %s (load failed or texture had own mipmaps)\n",names[i]);
      bufferclear();

      return ResData_addTexture("?default",TEX_COLOR,0);
    }
  }
  resRID = ResData_getFreeRID(RESOURCE_TEXTURE);
  if (resRID < 0)
    return -1;
  // create new
  texture = ResData_addResource(RESOURCE_TEXTURE,resRID,combinename,NULL);

  if (!Texture_combine(temptex,num,texture,mode)){
    lprintf("ERROR restexcomb: combine failed %s\n",names[i]);
    bufferclear();

    for (i = 0; i < num; i++){
      Texture_clearData(&temptex[i]);
    }

    return ResData_addTexture("?default",TEX_COLOR,0);
  }

  // upload GL_TEXTURE with mipmap
  texture->attributes = attributes;

  Texture_init(texture,type);
  Texture_initGL(texture,LUX_FALSE);

  for (i = 0; i < num; i++){
    Texture_clearData(&temptex[i]);
  }

  bufferclear();

  return resRID;
}


int  ResData_addShader(char *name,const char *cgcstring)
{
  int resRID;
  int i;
  Shader_t  *shader;
  char    filename[RES_MAXSTRLENGTH] = {0};
  const char  *extname;
  char    *dir = g_ResDescriptor[RESOURCE_SHADER].path;
  uchar hwskin;

  if (!name)
    return -1;

  // process filename
  if (!FS_setProjectFilename(filename,name,dir,ResData_getCurPath(),&l_RESData.loadertype)){
    lprintf("ERROR resadd %s: file not found %s\n",g_ResDescriptor[RESOURCE_SHADER].name,filename);
    return -1;
  }

  // cgcstring needs to be concat
  extname = ResData_concatCgCstring(cgcstring);
  l_RESData.curcgcfullstr = extname;

  // check if loaded, return
  resRID = ResData_search(RESOURCE_SHADER,filename,extname);
  if(resRID != -1) {
    ResData_refResource(RESOURCE_SHADER,resRID);
    return resRID;
  }
  // create new
  resRID = ResData_getFreeRID(RESOURCE_SHADER);
  if (resRID < 0)
    return -1;
  shader = ResData_addResource(RESOURCE_SHADER,resRID,filename,extname);

  if (!FS_processLoader(l_RESData.loadertype,filename,shader,NULL))
  {
    ResData_clearResource(RESOURCE_SHADER,resRID);
    return -1;
  }
  lprintf("\tCg Compiler: %s\n",extname);

  // upload proper textures/gpu stages
  ResData_IndirectStart(filename);
  ResData_pushCgCstring(cgcstring);

  if (shader->alphatex){
    if (shader->alphatex->srcName){
      shader->alphatex->srcRID = ResData_addTexture(shader->alphatex->srcName,shader->alphatex->srcType,shader->alphatex->srcAttributes);
      shader->alphatex->id = MATERIAL_NONE;
    }
  }

  for (i = 0; i < shader->numStages; i++){
    ShaderStage_t *stage = shader->stages[i];
    if (stage->stagetype == SHADER_STAGE_TEX && stage->srcName){
      char *texname = stage->srcName;

      stage->srcRID = ResData_addTexture(texname,stage->srcType,stage->srcAttributes);
      if (stage->texclamp)
        ResData_setTextureClamp(stage->srcRID,stage->texclamp);
      stage->id = MATERIAL_NONE;
    }
    if (stage->stagetype == SHADER_STAGE_TEX && (stage->stageflag & SHADER_DEPTHNOCOMPARE ||
      stage->stageflag & SHADER_DEPTHCOMPARE)){
      stage->srcType = TEX_DEPTH;
      stage->stageflag |= SHADER_SPECIAL;
    }
    if (stage->stagetype == SHADER_STAGE_GPU && stage->srcType != GPUPROG_F_FIXED && stage->srcType != GPUPROG_V_FIXED){
      int n;
      int failed;
      int lowprofile = (shader->tech <= VID_T_ARB_VF_TEX8) || isFlagSet(stage->stageflag,SHADER_CG_LOWPROFILE);
      ShaderGpuInfo_t *gpuinfo = stage->gpuinfo;

      failed = LUX_FALSE;
      for (n = 0; n < (VID_MAX_LIGHTS+1); n++){
        if (gpuinfo->srcNames[n])
          gpuinfo->srcRIDs[n] = ResData_addGpuProg(gpuinfo->srcNames[n],stage->srcType,lowprofile,gpuinfo->srcEntryNames[n]);
        else if (n > 0)
          gpuinfo->srcRIDs[n] = gpuinfo->srcRIDs[n-1];
        else
          gpuinfo->srcRIDs[n] = -1;

        if (gpuinfo->srcSkinNames[n])
          gpuinfo->srcSkinRIDs[n] = ResData_addGpuProg(gpuinfo->srcSkinNames[n],stage->srcType,lowprofile,gpuinfo->srcSkinEntryNames[n]);
        else if (!vidGPUVertex(stage->srcType))
          gpuinfo->srcSkinRIDs[n] = gpuinfo->srcRIDs[n];
        else if (n > 0)
          gpuinfo->srcSkinRIDs[n] = gpuinfo->srcSkinRIDs[n-1];
        else
          gpuinfo->srcSkinRIDs[n] = -1;

        if (gpuinfo->srcRIDs[n] == -1){

          if (vidGPUVertex(stage->srcType)){
            stage->srcType = GPUPROG_V_FIXED;
          }
          else if (vidGPUFragment(stage->srcType))
            stage->srcType = GPUPROG_F_FIXED;
          else
            stage->srcType = GPUPROG_G_FIXED;

          failed = LUX_TRUE;
          break;
        }
        else{
          gpuinfo->vidtype = ResData_getGpuProg(gpuinfo->srcRIDs[n])->vidtype;
        }
      }
      if (failed){
        bprintf("ERROR resadd shader: GPU Program failed, reverting to fixed function. Stage %d, %s\n",i,filename);
        continue;
      }

      if  (stage->srcType == GPUPROG_V_CG)
        shader->shaderflag |= SHADER_VCG;
      else if (stage->srcType == GPUPROG_V_ARB)
        shader->shaderflag |= SHADER_VARB;


      if (vidGPUVertex(stage->srcType))
        shader->shaderflag |= SHADER_VGPU;
      else if (vidGPUFragment(stage->srcType))
        shader->shaderflag |= SHADER_FGPU;
      else
        shader->shaderflag |= SHADER_GGPU;
    }

    if (stage->stageflag & SHADER_REFLECTMAP || stage->stageflag & SHADER_SKYMAP || stage->stageflag & SHADER_NORMALMAP)
      shader->shaderflag |= SHADER_CUBETEXGEN;
  }


  if (shader->shaderflag & SHADER_VGPU){
    hwskin = LUX_TRUE;
    for (i = 0; i < shader->numStages; i++){
      ShaderStage_t *stage = shader->stages[i];
      if (stage->stagetype == SHADER_STAGE_GPU && vidGPUVertex(stage->srcType)){
        if (stage->gpuinfo->srcSkinRIDs[0] == -1)
          hwskin = LUX_FALSE;
      }

    }
    if (hwskin)
      shader->shaderflag |= SHADER_HWSKIN;
  }

  // we can batch when there is no texgen or no vgpu, or if vgpu then supports skinning
  if (!(shader->shaderflag & SHADER_TEXGEN) &&
    (!(shader->shaderflag & SHADER_VGPU) ||
      (shader->shaderflag & SHADER_VGPU && shader->shaderflag & SHADER_HWSKIN)))
    shader->shaderflag |= SHADER_HWBATCHABLE;

  Shader_initGL(shader);

  Shader_initGpuPrograms(shader);

  ResData_popCgCstring();
  ResData_IndirectEnd();

  vidCheckError();
  return resRID;
}

int  ResData_addAnimation(char *name, enum32 animflag)
{
  int resRID;
  Anim_t  *anim;
  char  filename[RES_MAXSTRLENGTH] = {0};
  char  *dir = g_ResDescriptor[RESOURCE_ANIM].path;

  if (!name)
    return -1;

  // process filename
  if (!FS_setProjectFilename(filename,name,dir,ResData_getCurPath(),&l_RESData.loadertype)){
    lprintf("ERROR resadd %s: file not found %s\n",g_ResDescriptor[RESOURCE_ANIM].name,filename);
    return -1;
  }
  // check if loaded, return
  resRID = ResData_search(RESOURCE_ANIM,filename,NULL);
  if(resRID != -1) {
    ResData_refResource(RESOURCE_ANIM,resRID);
    return resRID;
  }
  // create new
  resRID = ResData_getFreeRID(RESOURCE_ANIM);
  if (resRID < 0)
    return -1;
  anim = ResData_addResource(RESOURCE_ANIM,resRID,filename,NULL);

  // load texture
  if (!FS_processLoader(l_RESData.loadertype,filename,anim,NULL))
  {
    lprintf("ERROR resanim: animation file load failed");
    ResData_clearResource(RESOURCE_ANIM,resRID);
    return -1;
  }
  anim->animflag = animflag;

  if (!anim->animflag ){
    anim->animflag |= ANIM_TYPE_LINEAR;
    anim->animflag |= ANIM_UPD_ABS_ALL;
  }
  anim->length = LUX_MAX(1,anim->length);

  return resRID;
}
int  ResData_addSound(char *name)
{
  char  filename[RES_MAXSTRLENGTH] = {0};

  int resRID;
  Sound_t *sound;
  char  *dir = g_ResDescriptor[RESOURCE_SOUND].path;

  if (!name)
    return -1;

  // process filename
  if (!FS_setProjectFilename(filename,name,dir,ResData_getCurPath(),&l_RESData.loadertype)){
    lprintf("ERROR resadd %s: file not found %s\n",g_ResDescriptor[RESOURCE_SOUND].name,filename);
    return -1;
  }
  // check if loaded, return
  resRID = ResData_search(RESOURCE_SOUND,filename,NULL);
  if(resRID != -1) {
    ResData_refResource(RESOURCE_SOUND,resRID);
    return resRID;
  }
  // create new
  resRID = ResData_getFreeRID(RESOURCE_SOUND);
  if (resRID < 0)
    return -1;
  sound = ResData_addResource(RESOURCE_SOUND,resRID,filename,NULL);

  if (!FS_processLoader(l_RESData.loadertype,filename,sound,NULL))
  {
    lprintf("ERROR ressound: sound file load failed");
    ResData_clearResource(RESOURCE_SOUND,resRID);
    return -1;
  }

  return resRID;
}

int  ResData_addParticleSys(char *name, int combdraw, int sortkey)
{
  int resRID;
  int i;
  int n;
  ParticleSys_t *psys;
  ParticleSys_t *psub;
  ParticleLife_t *life;
  Texture_t   *agetexs[PARTICLE_AGETEXS];
  char    filename[RES_MAXSTRLENGTH] = {0};
  char    *dir = g_ResDescriptor[RESOURCE_PARTICLESYS].path;

  if (!name)
    return -1;

  bufferclear();

  l_RESData.curcgcfullstr = NULL;

  // process filename
  if (!FS_setProjectFilename(filename,name,dir,ResData_getCurPath(),&l_RESData.loadertype)){
    lprintf("ERROR resadd %s: file not found %s\n",g_ResDescriptor[RESOURCE_PARTICLESYS].name,filename);
    return -1;
  }
  // check if loaded, return
  resRID = ResData_search(RESOURCE_PARTICLESYS,filename,NULL);
  if(resRID != -1) {
    ResData_refResource(RESOURCE_PARTICLESYS,resRID);
    return resRID;
  }
  // create new
  resRID = ResData_getFreeRID(RESOURCE_PARTICLESYS);
  if (resRID < 0)
    return -1;
  psys = ResData_addResource(RESOURCE_PARTICLESYS,resRID,filename,NULL);
  psys->sortkey = sortkey;

  if (!FS_processLoader(l_RESData.loadertype,filename,psys,NULL))
  {
    lprintf("ERROR resparticle: particlesys file load failed");
    ResData_clearResource(RESOURCE_PARTICLESYS,resRID);
    return -1;
  }
  ResData_IndirectStart(filename);
  // load models
  if (psys->emitterDefaults.emittertype == PARTICLE_EMITTER_MODEL){
    psys->emitterDefaults.modelRID = ResData_addModel(psys->emitterDefaults.modelname,MODEL_STATIC,VERTEX_32_NRM,LUX_FALSE);
    if (psys->emitterDefaults.modelRID < 0){
      lprintf("WARNING resparticle: model file load failed");
      psys->emitterDefaults.emittertype = PARTICLE_EMITTER_POINT;
    }

  }
  if (psys->container.particletype == PARTICLE_TYPE_MODEL){
    psys->container.modelRID = ResData_addModel(psys->life.modelname,MODEL_STATIC,VERTEX_32_NRM,LUX_FALSE);
    if (psys->container.modelRID < 0){
      lprintf("WARNING resparticle: model file load failed\n");
      psys->container.particletype = PARTICLE_TYPE_POINT;
    }
  }

  psys->timescale = 1.0f;

  for (i = 0; i < psys->numSubsys; i++){
    psys->subsys[i].psysRID = ResData_addParticleSys(psys->subsys[i].name,isFlagSet(psys->psysflag,PARTICLE_COMBDRAW),sortkey+i);
    psub = ResData_getParticleSys(psys->subsys[i].psysRID);
    if (!psub){
      lprintf("ERROR resparticle: particlesubsys file load failed\n");
      return -1;
    }
    if (psub->numSubsys > 0){
      combdraw = LUX_FALSE;
      psys->psysflag &= ~PARTICLE_COMBDRAW;
    }
    psub->psysflag |= PARTICLE_ISSUBSYS;
    if (psys->subsys[i].subsystype == PARTICLE_SUB_TRAIL){
      ParticleTrail_t *ptrail = ParticleTrail_new();
      ptrail->dir = psys->subsys[i].pos[2];
      ptrail->parentsysRID = psys->resinfo.resRID;
      ptrail->stride = psys->subsys[i].pos[0];
      ptrail->minAgeTrail = psys->subsys[i].pos[1];
      ptrail->trailParticle = psys->container.particles;

      lxListNode_addLast(psub->trailListHead,ptrail);

      // FIXME ?
      // what was sorting for, reschunks dont guarantee "first served" has lower id
#define cmpTrail(a,b) (a->parentsysRID < b->parentsysRID)
      lxListNode_sortBubble(psub->trailListHead,ParticleTrail_t,cmpTrail);
#undef cmpTrail

      psub->emitterDefaults.psysRID = psub->resinfo.resRID;
      psys->numTrails++;
    }
    // for combined draw we store all data in psys
    if (psys->psysflag & PARTICLE_COMBDRAW){
      psub->container.particletype = psys->container.particletype;
      psub->container.texRID = -1;

      //if (psys->container.particletype == PARTICLE_TYPE_POINT)
      //  psub->psysflag &= ~PARTICLE_TEXTURED;

      if (psub->psysflag & PARTICLE_COLORED)
        psys->psysflag |= PARTICLE_COLORED;
      if (psub->psysflag & PARTICLE_TEXTURED)
        psys->psysflag |= PARTICLE_TEXTURED;
    }
  }
  // textures for psys
  if (psys->psysflag & PARTICLE_COMBDRAW && psys->psysflag & PARTICLE_TEXTURED){
    // now we throw ALL textures of all psys and psub into a single tex
    char **combtexnames;
    int n;
    int index;
    int numtex;
    numtex = psys->life.numTex;
    index = numtex;

    for (i = 0; i < psys->numSubsys; i++){
      numtex +=  ResData_getParticleSys(psys->subsys[i].psysRID)->life.numTex;
    }
    combtexnames = lxMemGenAlloc(sizeof(char*)*numtex);
    for (i = 0; i < psys->life.numTex; i++){
      combtexnames[i] = lxMemGenZalloc(strlen(psys->life.texNames[i])+1);
      strcpy(combtexnames[i],psys->life.texNames[i]);
    }

    for (i = 0; i < psys->numSubsys; i++){
      psub = ResData_getParticleSys(psys->subsys[i].psysRID);
      for (n = 0; n < psub->life.numTex; n++){
        combtexnames[index] = lxMemGenZalloc(strlen(psys->life.texNames[i])+1);
        strcpy(combtexnames[index],psys->life.texNames[i]);
        index++;
      }
    }

    if (numtex == 1){
      if (psys->life.texType == TEX_MATERIAL)
        psys->container.texRID = ResData_addMaterial(psys->life.texNames[0],NULL);
      else
        psys->container.texRID = ResData_addTexture(combtexnames[0],psys->life.texType,TEX_ATTR_MIPMAP);
    }
    if (numtex > 1){
      if (psys->life.texType == TEX_MATERIAL)
        psys->container.texRID = ResData_addMaterial(psys->life.texNames[0],NULL);
      else if(psys->life.texType == TEX_2D_COMBINE1D || psys->life.texType == TEX_2D_COMBINE2D_64 || psys->life.texType == TEX_2D_COMBINE2D_16)
        psys->container.texRID = ResData_addTexture(psys->life.texNames[0],psys->life.texType,TEX_ATTR_MIPMAP);
      else
        psys->container.texRID = ResData_addTextureCombine(combtexnames,numtex,psys->life.texType,LUX_FALSE,TEX_ATTR_MIPMAP);
    }


    for (i = 0; i < numtex; i++){
      lxMemGenFree(combtexnames[i],strlen(combtexnames[i])+1);
    }
    lxMemGenFree(combtexnames,sizeof(char*)*numtex);
  }
  else if (!combdraw){
    if (psys->life.numTex == 1){
      if (psys->life.texType == TEX_MATERIAL)
        psys->container.texRID = ResData_addMaterial(psys->life.texNames[0],NULL);
      else
        psys->container.texRID = ResData_addTexture(psys->life.texNames[0],psys->life.texType,TEX_ATTR_MIPMAP);
    }
    if (psys->life.numTex > 1){
      if (psys->life.texType == TEX_MATERIAL)
        psys->container.texRID = ResData_addMaterial(psys->life.texNames[0],NULL);
      else if(psys->life.texType == TEX_2D_COMBINE1D || psys->life.texType == TEX_2D_COMBINE2D_64 || psys->life.texType == TEX_2D_COMBINE2D_16)
        psys->container.texRID = ResData_addTexture(psys->life.texNames[0],psys->life.texType,TEX_ATTR_MIPMAP);
      else
        psys->container.texRID = ResData_addTextureCombine(psys->life.texNames,psys->life.numTex,psys->life.texType,LUX_FALSE,LUX_FALSE);
    }
  }

  // buffer load interpolated textures
  life = &psys->life;
  for(i = 0; i < PARTICLE_AGETEXS; i++){
    agetexs[i]=NULL;
    if (life->agetexNames[i]){
      // check if already loaded
      for (n = 0; n < i; n++)
        if (life->agetexNames[n] && strcmp(life->agetexNames[i],life->agetexNames[n])==0){
          agetexs[i] = agetexs[n];
          break;
        }
      if (!agetexs[i]){
        agetexs[i] = bufferzalloc(sizeof(Texture_t));
        LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

        FS_setProjectFilename(filename,life->agetexNames[n],g_ResDescriptor[RESOURCE_TEXTURE].path,ResData_getCurPath(),&l_RESData.loadertype);
        if (!FS_processLoader(l_RESData.loadertype,filename,agetexs[i],NULL))
        {
          lprintf("WARNING resparticle: agetex load failed\n");
          life->agetexNames[i] = NULL;
          agetexs[i] = NULL;
        }
        else if (agetexs[i]->width != PARTICLE_STEPS )
        {
          lprintf("WARNING resparticle: agetex not %d pixels wide\n",PARTICLE_STEPS);
          life->agetexNames[i] = NULL;
          Texture_clearData(agetexs[i]);
          agetexs[i] = NULL;
        }
      }
    }
  }
  ParticleSys_initAgeTex(psys,agetexs);
  if (!ParticleContainer_initInstance(&psys->container,psys->container.instmeshname))
    lprintf("WARNING particlecontainer: mesh instancing not supported, %s\n",psys->resinfo.name);

  for (i = 0; i < PARTICLE_AGETEXS; i++)
    if (agetexs[i])
      Texture_clearData(agetexs[i]);


  ResData_IndirectEnd();
  return resRID;
}

int ResData_addParticleCloud(char *name, int particlecount, int sortkey){
  ParticleCloud_t *pcloud;
  int resRID;


  // check if loaded, return
  resRID = ResData_search(RESOURCE_PARTICLECLOUD,name,NULL);
  if(resRID != -1) {
    ResData_refResource(RESOURCE_PARTICLECLOUD,resRID);
    return resRID;
  }
  // create new
  resRID = ResData_getFreeRID(RESOURCE_PARTICLECLOUD);
  if (resRID < 0)
    return -1;
  pcloud = ResData_addResource(RESOURCE_PARTICLECLOUD,resRID,name,NULL);
  pcloud->sortkey = sortkey;

  ParticleCloud_init(pcloud,particlecount);


  return resRID;
}

int  ResData_addGpuProg(char *name, GPUProgType_t type, int lowCgProfile,const char *entryname)
{
  int resRID;
  int len;
  int checkuncompiled = LUX_TRUE;
  GpuProg_t *prog;
  CGprofile cgprofile;
  lxFSFile_t  *file;
  char    filename[RES_MAXSTRLENGTH] = {0};
  char    *dir = g_ResDescriptor[RESOURCE_GPUPROG].path;

  if (!name)
    return -1;

  if (!type){
    bprintf("ERROR: GpuProg %s: no type defined\n",name);
    return -1;
  }

  entryname = ResData_concatCgCstring(entryname);

  if (type == GPUPROG_V_CG)
    cgprofile = lowCgProfile ?  g_VID.cg.arbVertexProfile : g_VID.cg.vertexProfile;
  else if (type == GPUPROG_F_CG)
    cgprofile = lowCgProfile ?  g_VID.cg.arbFragmentProfile : g_VID.cg.fragmentProfile;
  else if (type == GPUPROG_G_CG)
    cgprofile = g_VID.cg.geometryProfile;
  else
    cgprofile = CG_PROFILE_UNKNOWN;


  // check precompiled file
  if (g_VID.cg.prefercompiled && GPUPROG_IS_CG(type)){
    if (strlen(entryname)+strlen(name)+10 > RES_MAXSTRLENGTH)
      lprintf("WARNING: compiled filename too long\n");
    else{
      char * newname = bufferzalloc(sizeof(char)*RES_MAXSTRLENGTH+1);
      LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

      // try to find pre-compiled file
      sprintf(newname,"%s.%s^%s.gl%s",name,entryname,cgGetProfileString(cgprofile),vidCgProfileisGLSL(cgprofile) ? "sl" : "p");
      if (FS_setProjectFilename(filename,newname,dir,ResData_getCurPath(),&l_RESData.loadertype))
        checkuncompiled = LUX_FALSE;
    }
  }

  // process filename
  if (checkuncompiled && !FS_setProjectFilename(filename,name,dir,ResData_getCurPath(),&l_RESData.loadertype)){
    lprintf("ERROR resadd %s: file not found %s\n",g_ResDescriptor[RESOURCE_GPUPROG].name,filename);
    return -1;
  }
  // check if loaded, return,
  resRID = ResData_search(RESOURCE_GPUPROG,filename,entryname);
  if(resRID != -1) {
    ResData_refResource(RESOURCE_GPUPROG,resRID);
    return resRID;
  }
  // create new
  resRID = ResData_getFreeRID(RESOURCE_GPUPROG);
  if (resRID < 0)
    return -1;
  prog = ResData_addResource(RESOURCE_GPUPROG,resRID,filename,entryname);



  if (entryname)
    lprintf("GpuProg: \t%s\t-%s\n",filename,entryname);
  else
    lprintf("GpuProg: \t%s\n",filename);

  if (cgprofile != CG_PROFILE_UNKNOWN){
    lprintf("\t%s\n",cgGetProfileString(cgprofile));
  }

  prog->cgProfile = cgprofile;
  prog->lowCgProfile = lowCgProfile;
  prog->type = type;

  if (prog->type == GPUPROG_V_ARB || prog->type == GPUPROG_F_ARB || !checkuncompiled){
    char *src;

    file = FS_open(filename);
    src  = lxFS_getContent(file);
    len = lxFS_getSize(file);
    prog->source = lxMemGenAlloc((1+len)*sizeof(char));
    prog->source[len]=0;
    prog->sourcelen = len+1;
    memcpy(prog->source,src,len);
    FS_close(file);
    if (prog->type == GPUPROG_V_ARB || prog->type == GPUPROG_F_ARB)
      GpuProg_fixSource(prog);
  }

  // prog source freed in here
  if (!GpuProg_initGL(prog)){
    ResData_clearResource(RESOURCE_GPUPROG,resRID);
    return -1;
  }

  return resRID;
}



//////////////////////////////////////////////////////////////////////////
// Shader Compiler String

void ResData_pushCgCstring(const char *str)
{
  l_RESData.cgcstackptrs[l_RESData.cgcstack] = str;
  l_RESData.cgcstack++;
}
void ResData_popCgCstring(){
  l_RESData.cgcstack--;
  l_RESData.cgcstackptrs[l_RESData.cgcstack] = NULL;
  LUX_ASSERT(l_RESData.cgcstack >= 1);
}

const char *ResData_getCgCstring(int idx){
  return l_RESData.cgcstackptrs[idx];
}
int ResData_getCgCstringCnt(){
  return l_RESData.cgcstack;
}
int ResData_isCgCstringUsed(){
  int used = 0;
  int i;
  for (i = 0; i < l_RESData.cgcstack; i++)
    used += (l_RESData.cgcstackptrs[i] && l_RESData.cgcstackptrs[i][0]) ? 1 : 0;

  return used > 0;
}

void  ResData_setCgCstringBase(char *str)
{
  if (str)
    strncpy(l_RESData.compilerargs,str,sizeof(char)*(GPUPROG_MAX_COMPILERARG_LENGTH-1));
  else
    memset(l_RESData.compilerargs,0,sizeof(char)*(GPUPROG_MAX_COMPILERARG_LENGTH));
}

char* ResData_getCgCstringBase()
{
  return l_RESData.compilerargs;
}

const char* ResData_concatCgCstring(const char *entryname)
{
  // concat cgcstring
  // gloabal compiler string is not part of this process
  if (ResData_isCgCstringUsed()){
    static char newentryname[RES_MAXSTRLENGTH+1];
    const char *cgcstring;
    int curlength = entryname ? strlen(entryname) : 0;
    int cgcCnt = ResData_getCgCstringCnt();

    *newentryname = 0;
    if (entryname){
      strcpy(newentryname,entryname);
      // find end
      entryname = strstr(newentryname,RES_CGC_SEPARATOR);
      if (!entryname){
        strcat(newentryname,RES_CGC_SEPARATOR);
        curlength += 1;
      }
      else if (newentryname[curlength-1] != ';'){
        strcat(newentryname,";");
        curlength += 1;
      }
    }
    while (cgcCnt >= 0){
      cgcstring = ResData_getCgCstring(cgcCnt);
      if (cgcstring){
        curlength += strlen(cgcstring);
        if (curlength >= RES_MAXSTRLENGTH-2){
          lprintf("WARNING: entryname with compilerstring too long\n");
          break;
        }
        strcat(newentryname,cgcstring);
        // add ; to end
        if (newentryname[curlength-1] != ';'){
          newentryname[curlength] = ';';
          newentryname[++curlength] = 0;
        }
      }

      cgcCnt--;
    }

    return newentryname;
  }
  return entryname;
}

booln ResData_isDefinedCgCstring(const char *check)
{
  char *found;
  if (!l_RESData.curcgcfullstr) return LUX_FALSE;

  found = strstr(l_RESData.curcgcfullstr,check);
  if (found > l_RESData.curcgcfullstr+1){
    int len = strlen(check);
    if (found[-1] == 'D' && found[-2] == '-' && found[len] == ';')
      return LUX_TRUE;
  }
  return LUX_FALSE;
}

//////////////////////////////////////////////////////////////////////////
// User Textures

int  ResData_getUserTexture(char *name)
{
  return (int)lxStrMap_get(l_RESData.userTextures,name);
}
void ResData_addUserTexture(int id, char *name)
{
  lxStrMap_set(l_RESData.userTextures,name,(void*)id);
}

typedef struct TexIter_s{
  char *names;
  int  numnames;
  int  texRID;
}TexIter_t;

void* ResData_userTextureIterator(void *upvalue,const char *name,void *browse)
{
  TexIter_t *iter = upvalue;

  if ((int)browse == iter->texRID){
    int len = strlen(name);

    iter->numnames++;
    iter->names = buffermalloc(len*sizeof(char));
    LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());
    iter->names = strcpy(iter->names,name);
  }

  return upvalue;
}

void ResData_removeUserTexture( char *name, int id)
{
  if (name)
    lxStrMap_remove(l_RESData.userTextures,name);
  else if (id >= 0){
    TexIter_t iter;
    char *namesstart;
    bufferclear();

    iter.texRID = id;
    namesstart = iter.names = buffercurrent();
    iter.numnames = 0;
    lxStrMap_iterate(l_RESData.userTextures,ResData_userTextureIterator,&iter);

    while(iter.numnames--){
      lxStrMap_remove(l_RESData.userTextures,namesstart);
      while(*namesstart++);
    }
  }
}
//////////////////////////////////////////////////////////////////////////
// GL POP / PUSH

void ResData_resizeWindowTextures()
{
  Texture_t *tex;
  int i;

  for (i=0; i < g_ResData.resarray[RESOURCE_TEXTURE].numMax; i++){
    tex = g_ResData.resarray[RESOURCE_TEXTURE].ptrs[i].tex;
    if (tex && tex->windowsized){
      Texture_clearData(tex);
      Texture_init(tex,tex->type);
      Texture_initGL(tex,LUX_TRUE);
    }

  }
}

void ResData_popGL(){
  int i;
  ResourceChunk_t *reschunk;
  Texture_t *tex;
  GpuProg_t *gpuprog;
  Model_t *model;
  Shader_t *shader;

  lxListNode_forEach(g_ResData.chunkListHead,reschunk)
    ResourceChunk_clearBOs(reschunk);
  lxListNode_forEachEnd(g_ResData.chunkListHead,reschunk)


  // things we lost on loading
  vidBindBufferPack(NULL);
  for (i = 0; i < g_ResData.resarray[RESOURCE_TEXTURE].numMax; i++){
    tex = g_ResData.resarray[RESOURCE_TEXTURE].ptrs[i].tex;
    if (tex){
      Texture_getGL(tex,NULL,NULL,0);
      Texture_clearGL(tex);
    }
  }
  for (i = 0; i < g_ResData.resarray[RESOURCE_GPUPROG].numMax; i++){
    gpuprog = g_ResData.resarray[RESOURCE_GPUPROG].ptrs[i].gpuprog;
    if (gpuprog){
      GpuProg_getGL(gpuprog);
      GpuProg_clearGL(gpuprog);
    }
  }
  for (i = 0; i < g_ResData.resarray[RESOURCE_MODEL].numMax; i++){
    model = g_ResData.resarray[RESOURCE_MODEL].ptrs[i].model;
    if (model){
      Model_clearGL(model,LUX_TRUE);
    }
  }

  for (i = 0; i < g_ResData.resarray[RESOURCE_SHADER].numMax; i++){
    shader = g_ResData.resarray[RESOURCE_SHADER].ptrs[i].shader;
    if (!shader)
      continue;
    Shader_clearGpuPrograms(shader);
  }

}
void ResData_pushGL(){
  int i,n;
  uchar meshtype;
  Model_t *model;
  Texture_t *tex;
  Mesh_t  *mesh;
  MeshObject_t *meshobj;
  GpuProg_t *gpuprog;
  Shader_t  *shader;

  for (i = 0; i < g_ResData.resarray[RESOURCE_GPUPROG].numMax; i++){
    gpuprog = g_ResData.resarray[RESOURCE_GPUPROG].ptrs[i].gpuprog;
    if (gpuprog){
      GpuProg_initGL(gpuprog);
    }
  }

  for (i = 0; i < g_ResData.resarray[RESOURCE_TEXTURE].numMax; i++){
    tex = g_ResData.resarray[RESOURCE_TEXTURE].ptrs[i].tex;
    if (tex)
      Texture_initGL(tex,LUX_FALSE);
  }


  for (i = 0; i < g_ResData.resarray[RESOURCE_MODEL].numMax; i++){
    model = g_ResData.resarray[RESOURCE_MODEL].ptrs[i].model;
    if (!model)
      continue;

    for (n = 0; n < model->numMeshObjects;n++){
      meshobj = &model->meshObjects[n];
      mesh = meshobj->mesh;
      meshtype = mesh->meshtype;
    }

    Model_initGL(model,meshtype,model->lmcoords,model->vcolors);

  }


  for (i = 0; i < g_ResData.resarray[RESOURCE_SHADER].numMax; i++){
    shader = g_ResData.resarray[RESOURCE_SHADER].ptrs[i].shader;
    if (!shader)
      continue;
    Shader_initGpuPrograms(shader);
  }
}


void ResData_print(){
  int i,type;
  ResourcePtr_t *resptr;
  ResourceChunk_t *reschunk;

  LUX_PRINTF("\n-------------------\nRESOURCE CHUNKS\n-------------------\n");
  lxListNode_forEach(g_ResData.chunkListHead,reschunk)
    LUX_PRINTF("%d: %s memory %d/%d\n",reschunk->chunkID,reschunk->name,lxMemoryStack_bytesUsed(reschunk->mempool),lxMemoryStack_bytesTotal(reschunk->mempool));
  lxListNode_forEachEnd(g_ResData.chunkListHead,reschunk);

  LUX_PRINTF("\n-------------------\nRESOURCES USED\n-------------------\n");
  LUX_PRINTF("Resource: Depth,ResChunk - name - extraname\n");
  for (type = 0; type < RESOURCES; type++){
    resptr = g_ResData.resarray[type].ptrs;
    for (i = 0; i < g_ResData.resarray[type].numMax; i++,resptr++){
      if (resptr->resinfo){
        LUX_PRINTF("%s: %d,%p - %s - %s\n",g_ResDescriptor[type].name,resptr->resinfo->depth,resptr->resinfo->reschunk,resptr->resinfo->name,(resptr->resinfo->extraname) ? resptr->resinfo->extraname : "");
      }
    }
  }

  LUX_PRINTF("-------------------\n");
}

