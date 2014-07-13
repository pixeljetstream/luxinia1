// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __VIDBUFFER_H__
#define __VIDBUFFER_H__

#include "../common/common.h"
#include "../common/reflib.h"


//////////////////////////////////////////////////////////////////////////
// VertexAttrib

typedef enum VertexAttrib_e{
  VERTEX_ATTRIB_POS,
  VERTEX_ATTRIB_BLENDWEIGHTS, // Cg:  blendweights
  VERTEX_ATTRIB_NORMAL,
  VERTEX_ATTRIB_COLOR,
  VERTEX_ATTRIB_ATTR4,
  VERTEX_ATTRIB_ATTR5,    // Cg: fogcoord,tessfactor
  VERTEX_ATTRIB_ATTR6,    // Cg: pointsize
  VERTEX_ATTRIB_BLENDINDICES, // Cg: blendindices
  VERTEX_ATTRIB_TEXCOORD0,
  VERTEX_ATTRIB_TEXCOORD1,
  VERTEX_ATTRIB_TEXCOORD2,
  VERTEX_ATTRIB_TEXCOORD3,
  VERTEX_ATTRIB_ATTR12,
  VERTEX_ATTRIB_ATTR13,
  VERTEX_ATTRIB_TANGENT,
  VERTEX_ATTRIB_ATTR15,   // Cg: binormal
  VERTEX_ATTRIBS,
}VertexAttrib_t;

//////////////////////////////////////////////////////////////////////////
// VertexCustomAttrib

typedef ushort VertexCustomAttrib_t;

#define VertexAttribCustomFlag ((1<<VERTEX_ATTRIB_ATTR4)|(1<<VERTEX_ATTRIB_ATTR5)|(1<<VERTEX_ATTRIB_ATTR6)|(1<<VERTEX_ATTRIB_ATTR12)|(1<<VERTEX_ATTRIB_ATTR13)|(1<<VERTEX_ATTRIB_ATTR15)|(1<<VERTEX_ATTRIB_TANGENT))
#define VertexCustomAttrib_new(cnt,type,normalize,integer) \
  (((cnt)-1) | ((type) << 2) | ((normalize) ? (1<<14) : 0) | ((integer) ? (1<<15) : 0))

LUX_INLINE VertexCustomAttrib_t VertexCustomAttrib_set(uint cnt, lxScalarType_t type, booln normalize, booln integer)
{
  return VertexCustomAttrib_new(cnt,type,normalize,integer);
}


//////////////////////////////////////////////////////////////////////////
// VIDBuffer

typedef enum VIDBufferType_e{
  VID_BUFFER_VERTEX,
  VID_BUFFER_INDEX,
  VID_BUFFER_PIXELINTO,
  VID_BUFFER_PIXELFROM,
  VID_BUFFER_TEXTURE,
  VID_BUFFER_FEEDBACK,
  VID_BUFFER_CPYINTO,
  VID_BUFFER_CPYFROM,
  VID_BUFFERS,
}VIDBufferType_t;

typedef enum VIDBufferHint_e{
  VID_BUFFERHINT_STREAM_DRAW,
  VID_BUFFERHINT_STREAM_READ,
  VID_BUFFERHINT_STREAM_COPY,
  VID_BUFFERHINT_STATIC_DRAW,
  VID_BUFFERHINT_STATIC_READ,
  VID_BUFFERHINT_STATIC_COPY,
  VID_BUFFERHINT_DYNAMIC_DRAW,
  VID_BUFFERHINT_DYNAMIC_READ,
  VID_BUFFERHINT_DYNAMIC_COPY,
  VID_BUFFERHINTS,
}VIDBufferHint_t;

typedef enum VIDBufferMapType_e{
  VID_BUFFERMAP_READ,
  VID_BUFFERMAP_WRITE,
  VID_BUFFERMAP_READWRITE,

  VID_BUFFERMAP_WRITEDISCARD,
  VID_BUFFERMAP_WRITEDISCARDALL,
  VID_BUFFERMAPS,
}VIDBufferMapType_t;

typedef struct VIDBuffer_s{
  uint        glID;
  uint        glType;
  VIDBufferType_t   type;

  uint        size;
  uint        used;

  VIDBufferHint_t   hint;

  VIDBufferMapType_t  maptype;
  void*       mapped;
  void*       mappedend;
  uint        mapstart;
  uint        maplength;

  void        *host;
}VIDBuffer_t;

void VIDBuffer_bindGL(VIDBuffer_t* buffer, VIDBufferType_t type);
void VIDBuffer_unbindGL(VIDBuffer_t* vbuf);
void VIDBuffer_clearGL(VIDBuffer_t* buffer);
void VIDBuffer_resetGL(VIDBuffer_t* buffer, void* data);
void VIDBuffer_initGL(VIDBuffer_t* buffer, VIDBufferType_t type, VIDBufferHint_t hint, uint size, void* data);
  // raises used and returns offset withn padsize from start
uint VIDBuffer_alloc(VIDBuffer_t *buffer, uint needed, uint padsize);

void VIDBuffer_submitGL(VIDBuffer_t *buffer, uint offset, uint size, const void *data);
void VIDBuffer_retrieveGL(VIDBuffer_t *buffer, uint offset, uint size, void *data);

void* VIDBuffer_mapGL(VIDBuffer_t *buffer, VIDBufferMapType_t type);
void* VIDBuffer_mapRangeGL(VIDBuffer_t *buffer, VIDBufferMapType_t type, uint from, uint length, booln manualflush, booln unsynch);
void VIDBuffer_flushRangeGL(VIDBuffer_t *buffer, uint from, uint lenght);
void VIDBuffer_unmapGL(VIDBuffer_t *buffer);

void VIDBuffer_copy(VIDBuffer_t *dst, uint dstoffset, VIDBuffer_t *src, uint srcoffset, uint length);

//////////////////////////////////////////////////////////////////////////
// VIDVertexPointer

typedef struct VIDVertexPointer_s{
  void          *ptr;
  VIDBuffer_t*      buffer;
  ushort          stride;
  VertexCustomAttrib_t  cattr;
}VIDVertexPointer_t;

//////////////////////////////////////////////////////////////////////////
// VIDStreamObject

typedef struct VIDStreamObject_s{
  VIDVertexPointer_t  stream;
  Reference     reference;
}VIDStreamObject_t;



#endif

