// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __MEMORYMANAGER_H__
#define __MEMORYMANAGER_H__

#include <luxinia/luxcore/memory.h>

//////////////////////////////////////////////////////////////////////////
//  Memory Management

extern lxMemoryStackPTR g_ResMemStack;
extern lxMemoryStackPTR g_BufferMemStack;
extern lxMemoryListPTR  g_DynamicMemList;
extern lxMemoryGenericPTR g_GenericMem;
extern lxMemoryAllocatorPTR g_GenericMemAlloc;

//////////////////////////////////////////////////////////////////////////
// Common Dynamic Allocations

#define MEMORY_GLOBAL_FREE            free
#define MEMORY_GLOBAL_MALLOC          malloc

#define GLOBAL_ALLOCATOR              g_GenericMemAlloc

#define lxMemGenZalloc(a)         lxMemoryAllocator_calloc(GLOBAL_ALLOCATOR, a, 1)
#define lxMemGenAlloc(a)          lxMemoryAllocator_malloc(GLOBAL_ALLOCATOR, a)
#define lxMemGenCalloc(a,b)         lxMemoryAllocator_calloc(GLOBAL_ALLOCATOR, a, b)
#define lxMemGenRealloc(ptr,size,oldsize) lxMemoryAllocator_realloc(GLOBAL_ALLOCATOR, ptr,size,oldsize)
#define lxMemGenFree(ptr,size)        lxMemoryAllocator_free(GLOBAL_ALLOCATOR, ptr,size)

#define lxMemGenReallocAligned(ptr,size,aligned,oldsize)  lxMemoryAllocator_reallocAligned(GLOBAL_ALLOCATOR, ptr,size,oldsize,aligned)
#define lxMemGenAllocAligned(a,aligned)   lxMemoryAllocator_mallocAligned(GLOBAL_ALLOCATOR, a,aligned)
#define lxMemGenZallocAligned(a,aligned)    lxMemoryAllocator_callocAligned(GLOBAL_ALLOCATOR, a,1,aligned)
#define lxMemGenFreeAligned(ptr,size)   lxMemoryAllocator_freeAligned(GLOBAL_ALLOCATOR, ptr,size)


  void GenMemory_init();
  void GenMemory_deinit();
  void GenMemory_dumpStats(const char* fname);

  void DynMemory_init();
  void DynMemory_deinit();
  void DynMemory_dumpStats(const char* fname);
#if 0
#ifdef MEMORY_STATS
  void* DynMemory_mallocStats(size_t s, const char* src, int line);
  void* DynMemory_zallocStats(size_t s, const char* src, int line);
  void  DynMemory_freeStats(void* ptr, size_t s, const char* src, int line);

  #define dynmalloc(s)        DynMemory_mallocStats(s,__FILE__,__LINE__)
  #define dynzalloc(s)        DynMemory_zallocStats(s,__FILE__,__LINE__)
  #define dynfree(p,s)        DynMemory_freeStats(p,s,__FILE__,__LINE__)
#else
  #define dynmalloc(s)        lxMemoryList_allocItem(g_DynamicMemList,s)
  #define dynzalloc(s)        lxMemoryList_zallocItem(g_DynamicMemList,s)
  #define dynfree(p,s)        lxMemoryList_freeItem(g_DynamicMemList,p,s)
#endif
#else
  #define dynmalloc         lxMemGenAlloc
  #define dynzalloc         lxMemGenZalloc
  #define dynfree           lxMemGenFree
#endif
//////////////////////////////////////////////////////////////////////////
// Conveniance Functions


// used for resource allocations
#define reszalloc(a)        lxMemoryStack_zalloc(g_ResMemStack,a)
#define reszallocaligned(a,align) lxMemoryStack_zallocAligned(g_ResMemStack,a,align)

#define resnewstrcpy(s1,s2)      {s1=(char*)reszalloc((strlen(s2)+1)*sizeof(char));strcpy(s1,s2);}

// use buffer commands in one shot stuff
#define buffercurrent()       lxMemoryStack_current(g_BufferMemStack)
#define bufferzalloc(a)       lxMemoryStack_zalloc(g_BufferMemStack, a)
#define buffermalloc(a)       lxMemoryStack_alloc(g_BufferMemStack, a)
#define bufferclear()         lxMemoryStack_clear(g_BufferMemStack)
#define buffersetbegin(a)     lxMemoryStack_setBegin(g_BufferMemStack, a)
#define buffercheckptr(a)     lxMemoryStack_checkPtr(g_BufferMemStack, a)
#define buffergetinuse()      lxMemoryStack_bytesUsed(g_BufferMemStack)
#define bufferminsize(a)      lxMemoryStack_initMin(g_BufferMemStack, a)

// use rpool commands inside rendering func
#define rpoolcurrent()        lxMemoryStack_current(g_BufferMemStack)
#define rpoolzalloc(a)        lxMemoryStack_zalloc(g_BufferMemStack, a)
#define rpoolmalloc(a)        lxMemoryStack_alloc(g_BufferMemStack, a)
#define rpoolclear()          lxMemoryStack_clear(g_BufferMemStack)
#define rpoolsetbegin(a)      lxMemoryStack_setBegin(g_BufferMemStack, a)
#define rpoolcheckptr(a)      lxMemoryStack_checkPtr(g_BufferMemStack, a)
#define rpoolgetinuse()       lxMemoryStack_bytesUsed(g_BufferMemStack)
#define rpoolresized()        lxMemoryStack_popResized(g_BufferMemStack)

// generic
#define gentypezalloc(type,count)   (type*)lxMemGenZalloc(sizeof(type)*(count))
#define gentypefree(data,type,count)  lxMemGenFree(data,sizeof(type)*(count))
#define genfreestr(str)         {lxMemGenFree(str,(strlen(str)+1)*sizeof(char));str = NULL;}
#define gennewstrcpy(s1,s2)       {s1=(char*)lxMemGenZalloc((strlen(s2)+1)*sizeof(char));strcpy(s1,s2);}

// internal advancing
#define pointeralloc(ptr,size)        lxPointerAdvanceAlloc(ptr,size)
#define pointerallocaligned(ptr,size,align) lxPointerAdvanceAllocAligned(ptr,size,align)

// XMM SSE
#ifdef LUX_SIMD_SSE
#define reszallocSIMD(a)        lxMemoryStack_zallocAligned(g_ResMemStack,a,16)
#define genzallocSIMD(a)        lxMemGenZallocAligned(a,16)
#define genfreeSIMD(a,size)     lxMemGenFreeAligned(a,size)
#else
#define reszallocSIMD(a)        lxMemoryStack_zalloc(g_ResMemStack,a)
#define genzallocSIMD(a)        lxMemGenZalloc(a)
#define genfreeSIMD(a,size)     lxMemGenFree(a,size)
#endif

#endif
