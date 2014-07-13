// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __TEXTURE_H__
#define __TEXTURE_H__

/*
TEXTURE
-------

A Texture is a bitmap file containing color information.
It will normally be uploaded to GL memory to be used for texturing ingame.
When creating a Texture you can specify its type.
Some special types are only used internally.

Render to screen with /render/gl_draw2d


Texture (user/nested,ResMem)

Author: Christoph Kubisch

*/

#include "../common/common.h"
#include <luxinia/luxcore/contscalararray.h>

#define TEX_TILESPERROW 8
#define TEX_TILEDEFSIZE 24

#define TEX_MAXMIPMAPS  16

#define TEX_USERSTART   "USER_TEX("
#define TEX_USERSTART_LEN 9

// order must match with attribute flag
typedef enum TextureDataType_e
{
  TEX_DATA_FIXED8,
  TEX_DATA_FIXED16,

  TEX_DATA_FLOAT16,
  TEX_DATA_FLOAT32,

  TEX_DATA_BYTE,
  TEX_DATA_UBYTE,

  TEX_DATA_SHORT,
  TEX_DATA_USHORT,

  TEX_DATA_INT,
  TEX_DATA_UINT,
  TEX_DATAS,
}TextureDataType_t;

typedef enum TextureCompressType_e
{
  TEX_COMPRESS_GENERIC,
  TEX_COMPRESS_DXT1,
  TEX_COMPRESS_DXT3,
  TEX_COMPRESS_DXT5,
  TEX_COMPRESS_RGTC,
  TEX_COMPRESS_SIGNED_RGTC,
  TEX_COMPRESS_LATC,
  TEX_COMPRESS_SIGNED_LATC,
  TEX_COMPRESS,
}TextureCompressType_t;

enum TextureFilterFlag_e
{
  TEX_FILTER_MAG = 1,
  TEX_FILTER_MIN = 2,
  TEX_FILTERS = 3,
};

enum TextureClampFlag_e
{
  TEX_CLAMP_NONE = 0,
  TEX_CLAMP_X = 1,
  TEX_CLAMP_Y = 2,
  TEX_CLAMP_Z = 4,
  TEX_CLAMP_ALL = 7,
};


enum TextureAttributeFlag_e
{
  TEX_ATTR_NONE = 0,
  TEX_ATTR_KEEPDATA = 1<<0,
  TEX_ATTR_MIPMAP = 1<<1,

  TEX_ATTR_DOTZ = 1<<2,
  TEX_ATTR_PROJ = 1<<3,

  TEX_ATTR_RECT = 1<<4,
  TEX_ATTR_CUBE = 1<<5,
  TEX_ATTR_3D   = 1<<6,
  TEX_ATTR_ARRAY = 1<<7,
  TEX_ATTR_BUFFER = 1<<8,

  TEX_ATTR_DATATYPE0  = 1<<10,
  TEX_ATTR_DATATYPE1  = 1<<11,
  TEX_ATTR_DATATYPE2  = 1<<12,
  TEX_ATTR_DATATYPE3  = 1<<13,

  TEX_ATTR_COMPR_GEN  = 1<<20,
  TEX_ATTR_COMPR_DXT1 = 1<<21,
  TEX_ATTR_COMPR_DXT3 = 1<<22,
  TEX_ATTR_COMPR_DXT5 = 1<<23,
  TEX_ATTR_COMPR_RGTC = 1<<24,
  TEX_ATTR_COMPR_SIGNED_RGTC = 1<<25,
  TEX_ATTR_COMPR_LATC = 1<<26,
  TEX_ATTR_COMPR_SIGNED_LATC = 1<<27,

  TEX_ATTR_NOTFILTERED  = 1<<29,
  TEX_ATTR_WINDOWSIZED  = 1<<30,
  TEX_ATTR_DEPTHCOMPARE = 1<<31,

  TEX_ATTR_DATATYPESHIFT = 10,
  TEX_ATTR_DATATYPEMASK = TEX_ATTR_DATATYPE0|TEX_ATTR_DATATYPE1|TEX_ATTR_DATATYPE2|TEX_ATTR_DATATYPE3,
};

typedef enum TextureFormat_e
{
  TEX_FORMAT_NONE,
  TEX_FORMAT_1D,
  TEX_FORMAT_2D,
  TEX_FORMAT_3D,
  TEX_FORMAT_RECT,
  TEX_FORMAT_CUBE,
  TEX_FORMAT_1D_ARRAY,
  TEX_FORMAT_2D_ARRAY,
  TEX_FORMAT_BUFFER,
}TextureFormat_t;

typedef enum TextureType_e{
  TEX_UNSET,

  TEX_RGB,
  TEX_RGBA,
  TEX_ALPHA,
  TEX_LUM,
  TEX_INTENSE,
  TEX_DEPTH,
  TEX_DEPTH16,
  TEX_DEPTH24,
  TEX_DEPTH32,
  TEX_BGR,
  TEX_BGRA,
  TEX_ABGR,
  TEX_LUMALPHA,
  TEX_R,
  TEX_RG,
  TEX_STENCIL,
  TEX_DEPTH_STENCIL,
  TEX_SHARED_EXPONENT,

  TEX_TYPES,

  TEX_COLOR,
  TEX_NOUPLOAD,       // will keep its imagedata and not create a gl texture
  TEX_BY_NAMESTRING,        // user textures encoded into a string
  TEX_3D_ATTENUATE,       // indirectly called by VID
  TEX_CUBE_NORMALIZE,       // indirectly called by VID
  TEX_CUBE_SPECULAR,        // indirectly called by VID
  TEX_CUBE_DIFFUSE,       // indirectly called by VID
  TEX_2D_WHITE,         // indirectly called by VID
  TEX_2D_DEFAULT,         // indirectly called by VID
  TEX_2D_COMBINE1D,       // indirectly called by prt
  TEX_2D_COMBINE2D_16,
  TEX_2D_COMBINE2D_64,

  // following are no upload targets
  TEX_CUBE_SKYBOX,
  TEX_2D_LIGHTMAP,
  TEX_MATERIAL,
  TEX_DUMMY,
}TextureType_t;

typedef enum TextureCombineModel_e
{
  TEX_COMBINE_INTERLEAVE,
  TEX_COMBINE_CONCAT,
  TEX_COMBINE_PACK,
}TextureCombineModel_t;


typedef struct Texture_s
{
  ResourceInfo_t resinfo;

  enum32  target;           // GL texture target (GL_TEXTURE_2D,...)
  uint  texID;            // Texture ID Used To Select A Texture

  union{
    byte  *imageData;
    ushort  *imageDataUShort;
    float *imageDataFloat;
  };

  // set on load
  TextureType_t type;
  int       mipmaps;
  flags32     attributes;
  int       bpp;
  lxScalarType_t  sctype;


  // must not change order
  int width;              // Image Width
  int height;             // Image Height
  int depth;              // Image Depth

  int components;

  // derived
  TextureFormat_t format;
  TextureType_t uptype;

  enum32  internalfomat;
  enum32  dataformat;         // Image format (GL_RGB, GL_RGBA, ...)
  enum32  datatype;         // data type (GL_UNSIGNED_BYTE, ...)


  // internal
  lxScalarArray3D_t sarr3d;

  int   origheight;
  int   origwidth;

  uchar border[4];          // fragment at 0,0
  int   clamped;
  int   windowsized;

  uchar trycompress;
  uchar notcompress;
  uchar iscompressed;
  uchar storedcompressed;

  uchar integer;
  uchar notfiltered;
  uchar isuser;
  uchar isregistered;
  int   mipmapfiltered;

  int   lmrgbscale;       // when bound as lightmap in non shader enviro this is rgbscale
  uint  lmtexblend;

  int   memcosts;
} Texture_t;

typedef struct TextureUpInfo_s{
  TextureType_t type;
  enum32      attribute;
}TextureUpInfo_t;


//////////////////////////////////////////////////////////////////////////
// TextureType
enum32  TextureType_toInternalGL(TextureType_t type);
enum32  TextureType_toDataGL(TextureType_t type);
enum32  TextureFormatGL_alterData(enum32 format, TextureDataType_t data);
size_t  TextureType_pixelcost(TextureType_t type);
const char* TextureType_toString(TextureType_t type);
lxScalarType_t TextureDataType_toScalar(enum32 type);
TextureDataType_t TextureDataType_fromFlag(flags32 attributes);
flags32 TextureDataType_toFlag(TextureDataType_t data, flags32 attributes);

enum32  TextureInternalGL_alterData(enum32 type, TextureDataType_t data);

//////////////////////////////////////////////////////////////////////////
// TextureFormat

const char* TextureFormat_toString(TextureFormat_t format);

//////////////////////////////////////////////////////////////////////////
// Texture

  // prepares texture data in a special way
void Texture_init(Texture_t *texture, TextureType_t type);

  // uploads to GL only special upload modes are allowed
void Texture_initGL (Texture_t *texture, booln keepGLID);

  // clear
void Texture_clear(Texture_t *texture);
void Texture_clearGL(Texture_t *texture);
void Texture_clearData(Texture_t *texture);

//------
// special generators
  // concat just stores imagedata next to each other
int  Texture_combine(Texture_t* textures, int num,Texture_t* target,TextureCombineModel_t mode);
TextureType_t Texture_makeFromString(Texture_t *tex,char *instruction);
void Texture_splitUserString(char *name, char *username, char *userinstruct);
// creates default 2d tex
void Texture_makeDefault(Texture_t *texture);

//------
// GL related
  // retrieve data from GL memory
booln Texture_getGL(Texture_t *texture, lxScalarArray_t *sarr, void* into, size_t intosize);
  // copy content of framebuffer into tex
void Texture_copyFramebufferGL(Texture_t *texture, int side, int mip,
  int xtex, int ytex, int xscreen, int yscreen,
  int width, int height);

  // if x == -1 then full submit is done
  // does no error checking
  // componentsize in bytes
void Texture_resubmitGL(Texture_t *texture, int mipmap,
  int x,int y,int z,
  int w, int h, int d,
  void *data, enum32 datatype, size_t componentsize, booln datapartial);

  // state
void Texture_clamp(Texture_t * texture, int state);
void Texture_filter(Texture_t * texture, int minstate, int magstate, int force);
void Texture_MipMapping(Texture_t* texture,int state);

//------
// DATA related
void Texture_getPixel(Texture_t *tex, int x, int y, int z,int wrap, lxVector4 outcolor);
void Texture_setPixel(Texture_t *tex, int x, int y, int z,lxVector4 color);
void Texture_getPixelUB(Texture_t *tex, int x, int y, int z,int wrap, uchar* outcolor);
void Texture_setPixelUB(Texture_t *tex, int x, int y, int z,uchar* color);
  // sample (bilinear filter) texture with keepdata, else crash
void Texture_sample(Texture_t *texture,lxVector3 coords,int clamp, lxVector4 outcolor);

  // convert hfloat to float and set FLOAT/HFLOAT attrib
void Texture_fixFloat(Texture_t *texture,int valuecnt);

  // returns size for imagedata (must be multiplied by 6 for cubemaps)
size_t Texture_getSizes(Texture_t *texture,int mipmapoffsets[16],int runtimecosts, int* mipmaps);
void Texture_resize(Texture_t *tex,int w,int h, int d);
  // forces given sizes for the next texture loaded
void Texture_forceSize(int width,int height);

  // returns if was uploaded with mipmapping
int  Texture_hasMipMaps(Texture_t *texture);
int  Texture_saveToFile(Texture_t *texture, char *filename, int quality, int side);


#endif

