// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/memorymanager.h"

//////////////////////////////////////////////////////////////////////////
// DEFS

lxMemoryStackPTR  g_ResMemStack;
lxMemoryStackPTR  g_BufferMemStack;
lxMemoryListPTR   g_DynamicMemList;
lxMemoryGenericPTR    g_GenericMem;
lxMemoryAllocatorPTR  g_GenericMemAlloc;

#include <luxinia/luxcore/conthash.h>

typedef struct MemTrace_s{
  const char*   src;
  int       line;
  size_t      size;
  size_t      n;
}MemTrace_t;

static lxContPtrHashPTR l_allocTracer = NULL;
static size_t     l_allocCnt = 0;

//////////////////////////////////////////////////////////////////////////

void GenMemory_init()
{
  g_GenericMem = lxMemoryGeneric_new(lxMemoryGenericDescr_default());
  g_GenericMemAlloc = lxMemoryGeneric_allocator(g_GenericMem);
}

void GenMemory_deinit()
{
  lxMemoryGeneric_deinitStats(g_GenericMem);
  lxMemoryGeneric_delete(g_GenericMem);
}

void GenMemory_dumpStats(const char* fname)
{
  lxMemoryGeneric_dumpStats(g_GenericMem,fname);
}

void DynMemory_init()
{
  uint sizes[16] = {16,32,48,64,128,160,208,224,272,352,512,768,1024,2048,4096,6144};
  g_DynamicMemList = lxMemoryList_new(GLOBAL_ALLOCATOR,16,sizes,1<<22,16);
#ifdef MEMORY_STATS
  l_allocTracer = lxContPtrHash_new(GLOBAL_ALLOCATOR,4096,sizeof(MemTrace_t));
#endif
}

static void*  DynMemory_leakIterator(void* fnData, void* key, void *val)
{
  MemTrace_t* browse = (MemTrace_t*)val;
  LUX_PRINTF("    %5.i: %p (%5.i): %s (l:%i)\n",browse->n,key,browse->size,browse->src,browse->line);
  return fnData;
}

void DynMemory_deinit()
{
#ifdef MEMORY_STATS
  if (l_allocTracer){
    if (!lxContPtrHash_isEmpty(l_allocTracer)){
      LUX_PRINTF("DynMemory Unfreed blocks:\n");
      lxContPtrHash_iterate(l_allocTracer,DynMemory_leakIterator,NULL);
    }
    lxContPtrHash_delete(l_allocTracer);
  }
#endif
}

static void*  DynMemory_statsIterator(void* fnData, void* key, void *val)
{
  MemTrace_t* browse = (MemTrace_t*)val;
  FILE* of = (FILE*)fnData;
  fprintf(of,"{file=[[%s]],\tline=%d,\tsize=%d,n=%d},\n",browse->src,browse->line,browse->size,browse->n);
  return fnData;
}

void DynMemory_dumpStats(const char* fname)
{
  FILE* of = fopen(fname,"wb");
  fprintf(of,"return {\n");
  lxContPtrHash_iterate(l_allocTracer,DynMemory_statsIterator,of);
  fprintf(of,"}\n");
  fclose(of);
}

void* DynMemory_mallocStats(size_t s, const char* src, int line)
{
  void* ptr = lxMemoryList_allocItem(g_DynamicMemList,s);
  MemTrace_t  trace = {src,line,s,++l_allocCnt};

  lxContPtrHash_set(l_allocTracer,ptr,&trace);

  return ptr;
}

void* DynMemory_zallocStats(size_t s, const char* src, int line)
{
  void* ptr = lxMemoryList_zallocItem(g_DynamicMemList,s);
  MemTrace_t  trace = {src,line,s,++l_allocCnt};

  lxContPtrHash_set(l_allocTracer,ptr,&trace);

  return ptr;
}

void  DynMemory_freeStats(void* ptr, size_t s, const char* src, int line)
{
  MemTrace_t* trace;
  LUX_ASSERT(lxContPtrHash_get(l_allocTracer,ptr,&trace) && trace->size == s);
  lxContPtrHash_remove(l_allocTracer,ptr);
}