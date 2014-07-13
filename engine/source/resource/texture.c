// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../render/gl_window.h"
#include "../resource/resmanager.h"
#include "../fileio/tga.h"
#include "../fileio/jpg.h"


// LOCALS
TextureType_t Texture_makeAlpha( Texture_t *texture);
TextureType_t Texture_makeWhite(Texture_t *texture);
TextureType_t Texture_makeNormalize(Texture_t *tex);
TextureType_t Texture_makeSpecular(Texture_t *tex);
TextureType_t Texture_makeDiffuse(Texture_t *tex);
TextureType_t Texture_makeAttenuate(Texture_t *tex);
TextureType_t Texture_makeCombine1D(Texture_t *texture);
TextureType_t Texture_makeProjCube(Texture_t *texture);
TextureType_t Texture_makeDotZ(Texture_t *texture);

void Texture_makeWindowSized(Texture_t *texture);

void Texture_size(Texture_t *texture, int makesquare, int ignorecaps);
int  Texture_scale( Texture_t *texture, int outwidth, int outheight);

static struct TEXData_s{
  int         forcesize;
  int         forcedsize[2];
}l_TEXData = {0};



//////////////////////////////////////////////////////////////////////////
// TextureFormat

const char* TextureFormat_toString(TextureFormat_t format)
{
  switch(format)
  {
  case TEX_FORMAT_NONE:
    return "none";
  case TEX_FORMAT_1D:
    return "1d";
  case TEX_FORMAT_2D:
    return "2d";
  case TEX_FORMAT_3D:
    return "3d";
  case TEX_FORMAT_RECT:
    return "rect";
  case TEX_FORMAT_CUBE:
    return "cube";
  case TEX_FORMAT_1D_ARRAY:
    return "1darray";
  case TEX_FORMAT_2D_ARRAY:
    return "2darray";
  case TEX_FORMAT_BUFFER:
    return "buffer";
  default:
    return NULL;
  }
}

//////////////////////////////////////////////////////////////////////////
// User Textures

void  Texture_splitUserString(char *name, char *username, char *userinstruct)
{
  char *out;
  char *user = name;
  size_t len;

  user+=TEX_USERSTART_LEN;
  out = username;

  out = strrchr(name,')');
  len = out ? out-user : strlen(user);
  strncpy(username,user,len);
  username[len] = 0;

  user += out ? len+1 : len;
  if (*user){
    out = userinstruct;
    while (*user){
      *out = *user;
      out++;
      user++;
    }
    *out = 0;
  }
}

//////////////////////////////////////////////////////////////////////////
// Clear texture
void Texture_clearGL(Texture_t *texture)
{
  if (texture->texID){
    glDeleteTextures(1, &texture->texID);
    LUX_PROFILING_OP (g_Profiling.global.memory.vidtexmem -= texture->memcosts);
  }
  texture->texID = 0;

}

void Texture_clearData(Texture_t *texture)
{
  int size = Texture_getSizes(texture,NULL,LUX_FALSE,NULL);
  int side = (texture->format == TEX_FORMAT_CUBE) ? 6 : 1;

  if (texture->imageData)
    lxMemGenFree(texture->imageData,size*side);
  texture->imageData = NULL;
  texture->sarr3d.sarr.data.tvoid = NULL;
  //texture->attributes &= ~TEX_ATTR_KEEPDATA;
}

void Texture_clear(Texture_t *texture)
{
  char username[256] = {0};
  char userinstruct[256] = {0};
  char *user;

  Texture_clearData(texture);
  Texture_clearGL(texture);

  if ((user=strstr(texture->resinfo.name,TEX_USERSTART))){
    Texture_splitUserString(user,username,userinstruct);
    ResData_removeUserTexture(username,-1);
  }
  else if (texture->isregistered)
    ResData_removeUserTexture(NULL,texture->resinfo.resRID);
}

size_t  Texture_getSizes(Texture_t *texture,int mipmapoffsets[TEX_MAXMIPMAPS],int runtimecosts, int* outmipmaps)
{
  int mipwidth = texture->width;
  int mipheight = texture->height;
  int mipdepth = texture->depth;

  size_t mipsize = 0;
  size_t pixelsize = texture->bpp/8;
  int n;
  int mipmaps = runtimecosts ? (Texture_hasMipMaps(texture) ? 20 : 1) : texture->mipmaps+1;


  // cubemap
  //if (texture->format == TEX_FORMAT_CUBE)
  //  mipdepth = 1;

  for (n = 0; n < mipmaps; n++){
    if (mipmapoffsets)
      mipmapoffsets[n] = mipsize;
    if ((!runtimecosts && !texture->storedcompressed) || (runtimecosts && !texture->iscompressed))
      mipsize += mipwidth * mipheight * mipdepth * pixelsize ;
    else
      mipsize += ((mipwidth+3)/4)*((mipheight+3)/4)*  (texture->dataformat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);

    if (runtimecosts && mipdepth == 1 && mipheight == 1 && mipwidth == 1){
      n++;
      break;
    }

    mipwidth /= 2;
    mipheight/= 2;
    mipdepth /= 2;
    // as long as there are mipmaps they are at least 1
    mipwidth = LUX_MAX(mipwidth,1);
    mipheight = LUX_MAX(mipheight,1);
    mipdepth = LUX_MAX(mipdepth,1);

  }
  if (mipmapoffsets){
    mipmapoffsets[n] = mipsize;
  }

  if (outmipmaps){
    *outmipmaps = n;
  }

  return runtimecosts && texture->depth==6 ? mipsize*6 : mipsize;
}




//////////////////////////////////////////////////////////////////////////
// TexUpload
typedef struct TexUpload_s{
  enum32 target;
  int level;
  enum32 internal;
  int w;
  int h;
  int d;
  int border;
  enum32 format;
  enum32 type;
  uchar *data;
  int size;
}TexUpload_t;

typedef enum TexUploadFuncType_e
{
  TEXUPLOAD_1D,
  TEXUPLOAD_1D_COMPRESSED,
  TEXUPLOAD_1D_MIPMAP,      // manual mipmap generation
  TEXUPLOAD_1D_MIPMAP_COMPRESSED, // doesnt exist

  TEXUPLOAD_2D,
  TEXUPLOAD_2D_COMPRESSED,
  TEXUPLOAD_2D_MIPMAP,
  TEXUPLOAD_2D_MIPMAP_COMPRESSED, // doesnt exist

  TEXUPLOAD_3D,
  TEXUPLOAD_3D_COMPRESSED,
  TEXUPLOAD_3D_MIPMAP,      // doesnt exist
  TEXUPLOAD_3D_MIPMAP_COMPRESSED, // doesnt exist

  TEXUPLOAD_NONE,
}TexUploadFuncType_t;

typedef void (TexUpload_fn) (TexUpload_t *tu);

static void TexUpload_1D(TexUpload_t *tu)
{
  glTexImage1D(tu->target,tu->level,tu->internal,tu->w,tu->border,tu->format,tu->type,tu->data);
}
static void TexUpload_1DCompressed(TexUpload_t *tu)
{
  glCompressedTexImage1DARB(tu->target,tu->level,tu->format,tu->w,tu->border,tu->size,tu->data);
}
static void TexUpload_1DMipMap(TexUpload_t *tu)
{
  gluBuild1DMipmaps(tu->target,tu->internal,tu->w,tu->format,tu->type,tu->data);
}

static void TexUpload_2D(TexUpload_t *tu)
{
  glTexImage2D(tu->target,tu->level,tu->internal,tu->w,tu->h,tu->border,tu->format,tu->type,tu->data);
}
static void TexUpload_2DCompressed(TexUpload_t *tu)
{
  glCompressedTexImage2DARB(tu->target,tu->level,tu->format,tu->w,tu->h,tu->border,tu->size,tu->data);
}
static void TexUpload_2DMipMap(TexUpload_t *tu)
{
  gluBuild2DMipmaps(tu->target,tu->internal,tu->w,tu->h,tu->format,tu->type,tu->data);
}

static void TexUpload_3D(TexUpload_t *tu)
{
  glTexImage3D(tu->target,tu->level,tu->internal,tu->w,tu->h,tu->d,tu->border,tu->format,tu->type,tu->data);
}
static void TexUpload_3DCompressed(TexUpload_t *tu)
{
  glCompressedTexImage3DARB(tu->target,tu->level,tu->format,tu->w,tu->h,tu->d,tu->border,tu->size,tu->data);
}


static TexUpload_fn* l_uploadfuncs[] =
{
  TexUpload_1D,
  TexUpload_1DCompressed,
  TexUpload_1DMipMap,
  NULL,
  TexUpload_2D,
  TexUpload_2DCompressed,
  TexUpload_2DMipMap,
  NULL,
  TexUpload_3D,
  TexUpload_3DCompressed,
  NULL,
  NULL,
};

void TexUpload_run(TexUpload_t *base, int withmipmap, int numMipMaps, int *mipoffsets, TexUploadFuncType_t uploadfunctype)
{
  TexUpload_t up;
  int mipwidth = base->w;
  int mipheight = base->h;
  int mipdepth = base->d;

  int mipsize;
  int n;

  if (uploadfunctype == TEXUPLOAD_NONE)
    return;

  vidCheckError();
  if (!withmipmap)
    l_uploadfuncs[uploadfunctype](base);
  else if (numMipMaps){
    memcpy(&up,base,sizeof(TexUpload_t));
    for (n = 0; n < numMipMaps+1; n++){
      mipsize = mipoffsets[n+1] - mipoffsets[n];
      up.level = n;
      up.w = mipwidth;
      up.h = mipheight;
      up.d = mipdepth;
      up.size = mipsize;
      up.data = &base->data[mipoffsets[n]];
      l_uploadfuncs[uploadfunctype](&up);
      mipwidth /= 2;
      mipheight/= 2;
      mipdepth /= 2;
      mipwidth = LUX_MAX(1,mipwidth);
      mipheight = LUX_MAX(1,mipheight);
      mipdepth = LUX_MAX(1,mipdepth);
    }
  }
  else if (GLEW_SGIS_generate_mipmap){
    l_uploadfuncs[uploadfunctype](base);
  }
  else if (l_uploadfuncs[uploadfunctype+2])
    l_uploadfuncs[uploadfunctype+2](base);
  else
    l_uploadfuncs[uploadfunctype](base);

  vidCheckError();
}

//////////////////////////////////////////////////////////////////////////
// Upload

void  Texture_forceSize(int width,int height)
{
  l_TEXData.forcedsize[0] = width;
  l_TEXData.forcedsize[1] = height;
  l_TEXData.forcesize = LUX_TRUE;
}

void Texture_size( Texture_t *texture, int makesquare, int ignorecaps)
{
  int     scaleW, scaleH;

  scaleW = lxNearestPowerOf2(texture->width);
  scaleH = lxNearestPowerOf2(texture->height);

  if (l_TEXData.forcesize){
    scaleW = l_TEXData.forcedsize[0];
    scaleH = l_TEXData.forcedsize[1];
    l_TEXData.forcesize = LUX_FALSE;
  }

  if (! ignorecaps){
    if (scaleW > g_VID.capTexSize)
      scaleW = g_VID.capTexSize;
    if (scaleH > g_VID.capTexSize)
      scaleH = g_VID.capTexSize;
  }
  if (makesquare && scaleW != scaleH)
    scaleH = (scaleW < scaleH) ? scaleW : scaleH;

    if (scaleW != texture->width || scaleH != texture->height)
      lprintf("WARNING texload: texture size change\n");

  if (g_VID.details == VID_DETAIL_LOW && !texture->mipmaps){
    if(scaleW > 32)
      scaleW /=2;
    if(scaleH > 32)
      scaleH /=2;
  }

  if (scaleW != texture->width || scaleH != texture->height)
  {
    Texture_scale(texture,scaleW,scaleH);
  }
}

TextureFormat_t Texture_getFormat(Texture_t *texture)
{
  if (texture->attributes & (TEX_ATTR_RECT))
    return TEX_FORMAT_RECT;
  else if (texture->attributes & (TEX_ATTR_BUFFER))
    return TEX_FORMAT_BUFFER;
  else if (texture->attributes & TEX_ATTR_3D)
    return TEX_FORMAT_3D;
  else if (texture->attributes & TEX_ATTR_ARRAY)
    return texture->depth >  1 ? TEX_FORMAT_2D_ARRAY : TEX_FORMAT_1D_ARRAY;
  else if (texture->attributes & (TEX_ATTR_CUBE | TEX_ATTR_PROJ | TEX_ATTR_DOTZ))
    return TEX_FORMAT_CUBE;
  else if (texture->height <= 1)
    return TEX_FORMAT_1D;
  else if (texture->depth <= 1 && texture->width)
    return TEX_FORMAT_2D;
  else if (texture->depth >  1){
    texture->attributes |= TEX_ATTR_3D;
    return TEX_FORMAT_3D;
  }
  else
    return TEX_FORMAT_NONE;
}
lxScalarType_t TextureDataType_toScalar(TextureDataType_t type)
{
  switch(type)
  {
  case TEX_DATA_FIXED8:
    return LUX_SCALAR_UINT8;
  case TEX_DATA_FIXED16:
    return LUX_SCALAR_UINT16;
  case TEX_DATA_UBYTE:
    return LUX_SCALAR_UINT8;
  case TEX_DATA_BYTE:
    return LUX_SCALAR_INT8;
  case TEX_DATA_USHORT:
    return LUX_SCALAR_UINT16;
  case TEX_DATA_SHORT:
    return LUX_SCALAR_INT16;
  case TEX_DATA_UINT:
    return LUX_SCALAR_UINT32;
  case TEX_DATA_INT:
    return LUX_SCALAR_INT32;
  case TEX_DATA_FLOAT32:
  case TEX_DATA_FLOAT16:
    return LUX_SCALAR_FLOAT32;
  default:
    return LUX_SCALAR_ILLEGAL;
  }

}


TextureCompressType_t TextureCompressType_fromFlag(flags32 attributes)
{

  TextureCompressType_t data = 0;
  for (; data < TEX_COMPRESS; data++){
    if (attributes & 1<<(20+data))
      return data;
  }

  return 0;
}

enum32  TextureInternalGL_alterCompression(enum32 type, TextureCompressType_t data)
{

#define GL_COMPRESSED_ALPHA                                0x84E9
#define GL_COMPRESSED_LUMINANCE                            0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA                      0x84EB
#define GL_COMPRESSED_INTENSITY                            0x84EC
#define GL_COMPRESSED_RGB                                  0x84ED
#define GL_COMPRESSED_RGBA                                 0x84EE

  switch(type){
  case GL_RGB:
    if (data == TEX_COMPRESS_DXT1)
      return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    return GL_COMPRESSED_RGB;

  case GL_RGBA:
    if (data == TEX_COMPRESS_DXT1)
      return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    else if (data == TEX_COMPRESS_DXT3)
      return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    else if (data == TEX_COMPRESS_DXT5)
      return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    return GL_COMPRESSED_RGBA;

  case GL_ALPHA:
    return GL_COMPRESSED_ALPHA;

  case GL_INTENSITY:
    return GL_COMPRESSED_INTENSITY;

  case GL_LUMINANCE:
    if (data == TEX_COMPRESS_LATC)
      return GL_COMPRESSED_LUMINANCE_LATC1_EXT;
    else if (data == TEX_COMPRESS_SIGNED_LATC)
      return GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT;

    return GL_COMPRESSED_LUMINANCE;

  case GL_LUMINANCE_ALPHA:
    if (data == TEX_COMPRESS_LATC)
      return GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT;
    else if (data == TEX_COMPRESS_SIGNED_LATC)
      return GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT;

    return GL_COMPRESSED_LUMINANCE_ALPHA;

  case GL_RED:
    if (data == TEX_COMPRESS_RGTC)
      return GL_COMPRESSED_RED_RGTC1_EXT;
    else if (data == TEX_COMPRESS_SIGNED_RGTC)
      return GL_COMPRESSED_SIGNED_RED_RGTC1_EXT;

    return GL_COMPRESSED_RED;

  case GL_RG:
    if (data == TEX_COMPRESS_RGTC)
      return GL_COMPRESSED_RED_GREEN_RGTC2_EXT;
    else if (data == TEX_COMPRESS_SIGNED_RGTC)
      return GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT;

    return GL_COMPRESSED_RG;
  default:
    return type;
  }
}

TextureDataType_t TextureDataType_fromFlag(flags32 attributes)
{

  TextureDataType_t data = 0;
  data = (attributes&TEX_ATTR_DATATYPEMASK) >> TEX_ATTR_DATATYPESHIFT;

  return data;
}

flags32 TextureDataType_toFlag(TextureDataType_t data, flags32 attributes)
{
  return (attributes&(~TEX_ATTR_DATATYPEMASK)) | (data << TEX_ATTR_DATATYPESHIFT);
}

enum32  TextureInternalGL_alterData(enum32 type, TextureDataType_t data)
{
  static enum32 rgb[TEX_DATAS] = {
    GL_RGB8, GL_RGB16, GL_RGB_FLOAT16_ATI, GL_RGB_FLOAT32_ATI,
    GL_RGB8I_EXT, GL_RGB8UI_EXT, GL_RGB16I_EXT, GL_RGB16UI_EXT,
    GL_RGB32I_EXT, GL_RGB32UI_EXT,
  };
  static enum32 rgba[TEX_DATAS] = {
    GL_RGBA8, GL_RGBA16, GL_RGBA_FLOAT16_ATI, GL_RGBA_FLOAT32_ATI,
    GL_RGBA8I_EXT, GL_RGBA8UI_EXT, GL_RGBA16I_EXT, GL_RGBA16UI_EXT,
    GL_RGBA32I_EXT, GL_RGBA32UI_EXT,
  };
  static enum32 alpha[TEX_DATAS] = {
    GL_ALPHA8, GL_ALPHA16, GL_ALPHA_FLOAT16_ATI, GL_ALPHA_FLOAT32_ATI,
    GL_ALPHA8I_EXT, GL_ALPHA8UI_EXT, GL_ALPHA16I_EXT, GL_ALPHA16UI_EXT,
    GL_ALPHA32I_EXT, GL_ALPHA32UI_EXT,
  };
  static enum32 intensity[TEX_DATAS] = {
    GL_INTENSITY8, GL_INTENSITY16, GL_INTENSITY_FLOAT16_ATI, GL_INTENSITY_FLOAT32_ATI,
    GL_INTENSITY8I_EXT, GL_INTENSITY8UI_EXT, GL_INTENSITY16I_EXT, GL_INTENSITY16UI_EXT,
    GL_INTENSITY32I_EXT, GL_INTENSITY32UI_EXT,
  };
  static enum32 lum[TEX_DATAS] = {
    GL_LUMINANCE8, GL_LUMINANCE16, GL_LUMINANCE_FLOAT16_ATI, GL_LUMINANCE_FLOAT32_ATI,
    GL_LUMINANCE8I_EXT, GL_LUMINANCE8UI_EXT, GL_LUMINANCE16I_EXT, GL_LUMINANCE16UI_EXT,
    GL_LUMINANCE32I_EXT, GL_LUMINANCE32UI_EXT,
  };
  static enum32 lumalpha[TEX_DATAS] = {
    GL_LUMINANCE8_ALPHA8, GL_LUMINANCE16_ALPHA16, GL_LUMINANCE_ALPHA_FLOAT16_ATI, GL_LUMINANCE_ALPHA_FLOAT32_ATI,
    GL_LUMINANCE_ALPHA8I_EXT, GL_LUMINANCE_ALPHA8UI_EXT, GL_LUMINANCE_ALPHA16I_EXT, GL_LUMINANCE_ALPHA16UI_EXT,
    GL_LUMINANCE_ALPHA32I_EXT, GL_LUMINANCE_ALPHA32UI_EXT,
  };
  static enum32 red[TEX_DATAS] = {
    GL_R8, GL_R16, GL_R16F, GL_R32F,
    GL_R8I, GL_R8UI, GL_R16I, GL_R16UI,
    GL_R32I, GL_R32UI,
  };
  static enum32 redgreen[TEX_DATAS] = {
    GL_RG8, GL_RG16, GL_RG16F, GL_RG32F,
    GL_RG8I, GL_RG8UI, GL_RG16I, GL_RG16UI,
    GL_RG32I, GL_RG32UI,
  };

  switch(type){
  case GL_RGB:
    return rgb[data];

  case GL_RGBA:
    return rgba[data];

  case GL_ALPHA:
    return alpha[data];

  case GL_INTENSITY:
    return intensity[data];

  case GL_LUMINANCE:
    return intensity[data];

  case GL_LUMINANCE_ALPHA:
    return lumalpha[data];

  case GL_RED:
    return red[data];

  case GL_RG:
    return redgreen[data];
  default:
    return type;
  }

}

enum32  TextureType_toInternalGL(TextureType_t type){
  switch(type)
  {
  case TEX_RGB:     return GL_RGB;
  case TEX_RGBA:      return GL_RGBA;
  case TEX_ALPHA:     return GL_ALPHA;
  case TEX_LUM:     return GL_LUMINANCE;
  case TEX_INTENSE:   return GL_INTENSITY;
  case TEX_DEPTH:     return GL_DEPTH_COMPONENT;
  case TEX_DEPTH16:   return GL_DEPTH_COMPONENT16;
  case TEX_DEPTH24:   return GL_DEPTH_COMPONENT24;
  case TEX_DEPTH32:   return GL_DEPTH_COMPONENT32;
  case TEX_BGR:     return GL_RGB;
  case TEX_BGRA:      return GL_RGBA;
  case TEX_ABGR:      return GL_RGBA;
  case TEX_LUMALPHA:    return GL_LUMINANCE_ALPHA;
  case TEX_R:       return GL_RED;
  case TEX_RG:      return GL_RG;
  case TEX_STENCIL:   return GL_STENCIL_INDEX;
  case TEX_DEPTH_STENCIL: return GL_DEPTH24_STENCIL8_EXT;
  default:
    return 0;
  }

}

enum32 TextureFormatGL_alterData(enum32 format, TextureDataType_t data)
{
  if (data < TEX_DATA_BYTE)
    return format;

  switch(format)
  {
  case GL_RGB:
    return GL_RGB_INTEGER_EXT;
  case GL_RGBA:
    return GL_RGBA_INTEGER_EXT;
  case GL_LUMINANCE:
    return GL_LUMINANCE_INTEGER_EXT;
  case GL_DEPTH_COMPONENT:
    return GL_DEPTH_COMPONENT;
  case GL_BGR:
    return GL_BGR_INTEGER_EXT;
  case GL_BGRA:
    return GL_BGRA_INTEGER_EXT;
  case GL_ABGR_EXT:
    return GL_ABGR_EXT;
  case GL_LUMINANCE_ALPHA:
    return GL_LUMINANCE_ALPHA_INTEGER_EXT;
  case GL_RED:
    return GL_RED_INTEGER_EXT;
  case GL_STENCIL_INDEX:
    return GL_STENCIL_INDEX;
  case GL_DEPTH_STENCIL_EXT:
    return GL_DEPTH_STENCIL_EXT;
  default:
    return 0;
  }
}

enum32  TextureType_toDataGL(TextureType_t type){
  switch(type)
  {
  case TEX_RGB:     return GL_RGB;
  case TEX_RGBA:      return GL_RGBA;
  case TEX_ALPHA:     return GL_ALPHA;
  case TEX_LUM:     return GL_LUMINANCE;
  case TEX_INTENSE:   return GL_LUMINANCE;
  case TEX_DEPTH:     return GL_DEPTH_COMPONENT;
  case TEX_DEPTH16:   return GL_DEPTH_COMPONENT;
  case TEX_DEPTH24:   return GL_DEPTH_COMPONENT;
  case TEX_DEPTH32:   return GL_DEPTH_COMPONENT;
  case TEX_BGR:     return GL_BGR;
  case TEX_BGRA:      return GL_BGRA;
  case TEX_ABGR:      return GL_ABGR_EXT;
  case TEX_LUMALPHA:    return GL_LUMINANCE_ALPHA;
  case TEX_R:       return GL_RED;
  case TEX_RG:      return GL_RG;
  case TEX_STENCIL:   return GL_STENCIL_INDEX;
  case TEX_DEPTH_STENCIL: return GL_DEPTH_STENCIL_EXT;
  default:
    return 0;
  }
}

size_t  TextureType_pixelcost(TextureType_t type){
  switch(type)
  {
  case TEX_RGB:     return 3;
  case TEX_RGBA:      return 4;
  case TEX_ALPHA:     return 1;
  case TEX_LUM:     return 1;
  case TEX_DEPTH:     return 3;
  case TEX_DEPTH16:   return 2;
  case TEX_DEPTH24:   return 3;
  case TEX_DEPTH32:   return 4;
  case TEX_BGR:     return 3;
  case TEX_BGRA:      return 4;
  case TEX_ABGR:      return 4;
  case TEX_LUMALPHA:    return 2;
  case TEX_STENCIL:   return 1;
  case TEX_DEPTH_STENCIL: return 4;
  default:
    return 0;
  }
}

TextureType_t Texture_getAutoUptype(Texture_t *texture)
{
  switch(texture->components)
  {
  case 1:
    return TEX_LUM;
  case 2:
    if (texture->attributes & (TEX_ATTR_COMPR_RGTC | TEX_ATTR_COMPR_SIGNED_RGTC))
      return TEX_RG;
    else
      return TEX_LUMALPHA;
  case 3:
    return TEX_RGB;
  case 4:
    return TEX_RGBA;
  }
  LUX_ASSERT(0);
  return 0;
}

void Texture_init(Texture_t *texture, TextureType_t type)
{
  TextureType_t     uptype = type;
  TextureDataType_t   datatype;
  TextureCompressType_t comptype;

  // prepare size and format
  texture->origheight = texture->height;
  texture->origwidth = texture->width;

  texture->format = Texture_getFormat(texture);
  texture->type = type;

  if (texture->attributes & TEX_ATTR_WINDOWSIZED){
    Texture_makeWindowSized(texture);
  }
  else if (texture->attributes & TEX_ATTR_BUFFER){
    texture->width = 0;
    texture->height = 0;
    texture->depth = 0;
  }
  else if (texture->attributes & TEX_ATTR_CUBE && texture->width != texture->height){
    int w = lxNearestPowerOf2(texture->width);
    Texture_forceSize(w,w*6);
    Texture_size(texture, LUX_FALSE,LUX_TRUE);
    texture->height = w;
    texture->depth = 1;
    texture->format = TEX_FORMAT_CUBE;
  }
  else if (texture->attributes & (TEX_ATTR_3D|TEX_ATTR_ARRAY) && texture->depth < 2){
    int w,h,d;
    if (!GLEW_ARB_texture_non_power_of_two){
      w = lxNearestPowerOf2(texture->width);
      d = lxNearestPowerOf2(texture->height/w);
      h = w*d;
      Texture_forceSize(w,h*d);
      Texture_size(texture, LUX_FALSE,LUX_TRUE);
    }
    else{
      w = texture->width;
      d = texture->height/w;
    }

    texture->depth = d;
    texture->height = w;
  }
  else if (texture->attributes & TEX_ATTR_DOTZ){
    Texture_size(texture, LUX_TRUE,LUX_FALSE);
    Texture_makeDotZ(texture);
  }
  else if (texture->attributes & TEX_ATTR_PROJ){
    Texture_size(texture, LUX_TRUE,LUX_FALSE);
    Texture_makeProjCube(texture);
  }
  else if (type < TEX_TYPES && !(texture->attributes & (TEX_ATTR_RECT | TEX_ATTR_3D | TEX_ATTR_CUBE | TEX_ATTR_ARRAY))){
    if (!GLEW_ARB_texture_non_power_of_two){
      Texture_size(texture, LUX_FALSE,LUX_FALSE);
    }
  }
  if (texture->attributes & (TEX_ATTR_RECT )){
    texture->attributes &= ~TEX_ATTR_MIPMAP;
  }
  if (texture->attributes & TEX_ATTR_NOTFILTERED){
    texture->notfiltered = TEX_FILTER_MIN | TEX_FILTER_MAG;
  }

  // prepare uptype

  if (type > TEX_TYPES){
    switch(type)
    {
    case TEX_COLOR:
      uptype = Texture_getAutoUptype(texture);
      break;
      // SPECIALS
    case TEX_2D_COMBINE1D:
      uptype = Texture_getAutoUptype(texture);
      break;
    case TEX_2D_COMBINE2D_16:
    case TEX_2D_COMBINE2D_64:
      uptype = Texture_makeCombine1D(texture);
      break;
    case TEX_3D_ATTENUATE:
      uptype = Texture_makeAttenuate(texture);
      break;
    case TEX_2D_WHITE:
      uptype = Texture_makeWhite(texture);
      break;
    case TEX_2D_DEFAULT:
      Texture_makeDefault(texture);
      return;
      break;
    case TEX_CUBE_SPECULAR:
      uptype = Texture_makeSpecular(texture);
      break;
    case TEX_CUBE_DIFFUSE:
      uptype = Texture_makeDiffuse(texture);
      break;
    case TEX_CUBE_NORMALIZE:
      uptype = Texture_makeNormalize(texture);
      break;
    case TEX_NOUPLOAD:
      texture->attributes |= TEX_ATTR_KEEPDATA;
      texture->format = TEX_FORMAT_NONE;
      break;
    }
  }

  // prepare internal & dataformats
  texture->uptype = uptype;
  texture->datatype      = ScalarType_toGL(texture->sctype);
  texture->dataformat    = TextureType_toDataGL(uptype);
  texture->internalfomat = TextureType_toInternalGL(uptype);

  switch(uptype){
    case TEX_STENCIL:
      bprintf("WARNING: texGLinit: type is no valid target, %s\n",texture->resinfo.name);
      return;
    case TEX_ALPHA:
      Texture_makeAlpha(texture);
      texture->notcompress = LUX_TRUE;
      break;
    case TEX_LUM:
      Texture_makeAlpha(texture);
      texture->dataformat = GL_LUMINANCE;
      texture->notcompress = LUX_TRUE;
      break;
    case TEX_INTENSE:
      Texture_makeAlpha(texture);
      texture->dataformat = GL_LUMINANCE;
      texture->notcompress = LUX_TRUE;
      break;
    case TEX_DEPTH:
    case TEX_DEPTH16:
    case TEX_DEPTH24:
    case TEX_DEPTH32:
      texture->notcompress = LUX_TRUE;
      break;
    case TEX_SHARED_EXPONENT:
      texture->dataformat = GL_RGB;
      texture->datatype = GL_UNSIGNED_INT_5_9_9_9_REV_EXT;
      texture->internalfomat = GL_RGB9_E5_EXT;
      break;
    case TEX_DEPTH_STENCIL:
      texture->datatype = GL_UNSIGNED_INT_24_8_EXT;
      texture->notcompress = LUX_TRUE;
      break;
    case TEX_NOUPLOAD:
      texture->notcompress = LUX_TRUE;
      break;
  }

  LUX_ASSERT(texture->datatype && texture->dataformat && texture->internalfomat);


  texture->notcompress = (texture->notcompress || texture->format == TEX_FORMAT_3D || texture->format == TEX_FORMAT_BUFFER);
  texture->target = 0;
  texture->trycompress = (g_VID.useTexCompress && g_VID.capTexCompress);
  if (texture->notcompress || texture->storedcompressed)
    texture->trycompress = LUX_FALSE;

  // internal type conversions
  datatype = TextureDataType_fromFlag(texture->attributes);
  comptype = TextureCompressType_fromFlag(texture->attributes);

  LUX_ASSERT(!texture->storedcompressed || (texture->storedcompressed && comptype));


  if (datatype == TEX_DATA_FIXED8 && (texture->storedcompressed || texture->trycompress)){
    texture->internalfomat = TextureInternalGL_alterCompression(texture->internalfomat,comptype);
  }
  else{
    texture->internalfomat = TextureInternalGL_alterData(texture->internalfomat,datatype);
    texture->dataformat    = TextureFormatGL_alterData(texture->dataformat,datatype);
    texture->integer = datatype >= TEX_DATA_UBYTE;
  }

}

static void Texture_updateScalarArray(Texture_t *texture)
{
  lxScalarArray3D_t *sarr3d = &texture->sarr3d;
  LUX_ARRAY3COPY(sarr3d->size,&texture->width);
  sarr3d->offset[0] = sarr3d->offset[1] = sarr3d->offset[2] = 0;
  sarr3d->sarr.stride    = texture->components;
  sarr3d->sarr.vectordim = texture->components;
  sarr3d->sarr.data.tvoid = texture->imageData;
  sarr3d->sarr.count = sarr3d->size[0] * sarr3d->size[1] * sarr3d->size[2];
  sarr3d->sarr.type = texture->sctype;
  sarr3d->sarr.data.tvoid = texture->imageData;
}


void Texture_initGL (Texture_t *texture, booln keepGLID)
{
  int i;
  size_t  size;
  int   mipoffsets[TEX_MAXMIPMAPS];
  TexUploadFuncType_t uploadfunctype;
  TexUpload_t up;

  if (texture->format == TEX_FORMAT_NONE){
    Texture_updateScalarArray(texture);
    return;
  }
  vidCheckError();
  vidBindBufferUnpack(NULL);
  vidSelectTexture(0);
  size = Texture_getSizes(texture,mipoffsets,LUX_FALSE,NULL);

  if (!keepGLID)
    Texture_clearGL(texture);
  if (!texture->texID)
    glGenTextures(1, &texture->texID);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT,1);

  switch(texture->format){
    case TEX_FORMAT_1D:
      texture->target = GL_TEXTURE_1D;
      uploadfunctype = TEXUPLOAD_1D;
      break;
    case TEX_FORMAT_2D:
      texture->target = GL_TEXTURE_2D;
    case TEX_FORMAT_RECT:
      if (!texture->target){
        texture->target = GL_TEXTURE_RECTANGLE_NV;
        texture->attributes &= ~TEX_ATTR_MIPMAP;
      }
      uploadfunctype = TEXUPLOAD_2D;
      break;
    case TEX_FORMAT_1D_ARRAY:
      texture->target = GL_TEXTURE_1D_ARRAY_EXT;
      uploadfunctype = TEXUPLOAD_2D;
      break;
    case TEX_FORMAT_3D:
      texture->target = GL_TEXTURE_3D_EXT;
      uploadfunctype = TEXUPLOAD_3D;
      break;
    case TEX_FORMAT_2D_ARRAY:
      texture->target = GL_TEXTURE_2D_ARRAY_EXT;
      uploadfunctype = TEXUPLOAD_3D;
      break;
    case TEX_FORMAT_CUBE:
      texture->target = GL_TEXTURE_CUBE_MAP_ARB;
      texture->clamped = TEX_CLAMP_ALL;
      uploadfunctype = TEXUPLOAD_2D;
      break;
    case TEX_FORMAT_BUFFER:
      texture->target = GL_TEXTURE_BUFFER_ARB;
      uploadfunctype = TEXUPLOAD_NONE;
      break;
    default:
      bprintf("WARNING: texGLinit: types are no valid uploads, %s\n",texture->resinfo.name);
  }
  vidCheckError();
  if (texture->imageData)
    LUX_ARRAY4COPY(texture->border,texture->imageData);
  vidForceBindTexture(texture->resinfo.resRID);

  up.target = texture->target;
  up.level = 0;
  up.internal = texture->internalfomat;
  up.w = texture->width;
  up.h = texture->height;
  up.d = texture->depth;
  up.border = 0;
  up.format = texture->dataformat;
  up.type = texture->datatype;
  up.data = texture->imageData;
  up.size = texture->mipmaps ? mipoffsets[1]-mipoffsets[0] : size;

  uploadfunctype += (texture->storedcompressed) ? 1 : 0;
  vidCheckError();

  if (texture->format != TEX_FORMAT_BUFFER && Texture_hasMipMaps(texture) && GLEW_SGIS_generate_mipmap && !texture->mipmaps){
    glTexParameteri(texture->target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
  }

  if (texture->format == TEX_FORMAT_BUFFER);
  else if (texture->format == TEX_FORMAT_CUBE){
    vidCheckError();
    for (i = 0; i < 6; i++){
      up.target = GL_TEXTURE_CUBE_MAP_POSITIVE_X+i;
      up.data = texture->imageData ? &texture->imageData[i*size] : NULL;
      up.d = 1;
      TexUpload_run(&up,GLEW_SGIS_generate_mipmap && !texture->mipmaps ? 0 : Texture_hasMipMaps(texture),texture->mipmaps,mipoffsets,uploadfunctype);
    }
  }
  else
    TexUpload_run(&up,Texture_hasMipMaps(texture),texture->mipmaps,mipoffsets,uploadfunctype);


  if (texture->format == TEX_FORMAT_BUFFER){
    // do nothing
    return;
  }
  else{
    if (Texture_hasMipMaps(texture)){
      glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, (g_VID.details == VID_DETAIL_HI) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST);
      texture->mipmapfiltered = LUX_TRUE;
    }
    else
      glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }


  vidCheckError();

  if (texture->trycompress) {
    if (texture->target == GL_TEXTURE_CUBE_MAP_ARB)
      glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_COMPRESSED_ARB, &i);
    else
      glGetTexLevelParameteriv(texture->target, 0, GL_TEXTURE_COMPRESSED_ARB, &i);
    if (i != GL_TRUE)
      lprintf("WARNING: texGLinit: could not compress, %s\n",texture->resinfo.name);
    texture->iscompressed = i;
  }
  if (texture->clamped){
    glTexParameteri(texture->target,GL_TEXTURE_WRAP_S,texture->clamped & TEX_CLAMP_X ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(texture->target,GL_TEXTURE_WRAP_T,texture->clamped & TEX_CLAMP_Y ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(texture->target,GL_TEXTURE_WRAP_R,texture->clamped & TEX_CLAMP_Z ? GL_CLAMP_TO_EDGE : GL_REPEAT);
  }
  else if (texture->format != TEX_FORMAT_RECT){
    glTexParameteri(texture->target,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(texture->target,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(texture->target,GL_TEXTURE_WRAP_R,GL_REPEAT);
  }

  if (texture->notfiltered){
    Texture_filter(texture,!isFlagSet(texture->notfiltered,TEX_FILTER_MIN),!isFlagSet(texture->notfiltered,TEX_FILTER_MAG),LUX_TRUE);
  }
  if (texture->dataformat == GL_DEPTH_COMPONENT){
    lxVector4 vec;
    lxVector4Set(vec,1,0,0,0);
    LUX_ARRAY4SET(texture->border,255,255,255,255);
    // always clamp depth textures and set border to 1
    glTexParameterfv(texture->target,GL_TEXTURE_BORDER_COLOR,vec);
    glTexParameteri(texture->target,GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(texture->target,GL_TEXTURE_WRAP_T, GL_CLAMP);
    if (GLEW_ARB_shadow){
      glTexParameteri(texture->target,GL_TEXTURE_COMPARE_MODE_ARB,GL_COMPARE_R_TO_TEXTURE);//
      glTexParameteri(texture->target,GL_TEXTURE_COMPARE_FUNC_ARB,GL_LEQUAL);
      glTexParameteri(texture->target,GL_DEPTH_TEXTURE_MODE,GL_LUMINANCE);
      texture->attributes |= TEX_ATTR_DEPTHCOMPARE;
    }
    texture->clamped = TEX_CLAMP_X | TEX_CLAMP_Y;
  }

  if (g_VID.useTexAnisotropic && g_VID.capTexAnisotropic && !texture->notfiltered){
    glTexParameterf(texture->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, g_VID.lvlTexAnisotropic);
  }

  texture->lmrgbscale = 0;
  texture->lmtexblend = VID_TEXBLEND_MOD_PREV;

  // we dont need the image data in the RAM anymore, it is in GL memory now
  if (!(texture->attributes & TEX_ATTR_KEEPDATA))
    Texture_clearData(texture);

  texture->memcosts = Texture_getSizes(texture,NULL,LUX_TRUE,NULL);
  LUX_PROFILING_OP (g_Profiling.global.memory.vidtexmem += texture->memcosts);

  Texture_updateScalarArray(texture);

  lprintf("Texture Init GL: [%s] format %d internal %d data %d type %d\n",
    texture->resinfo.name,texture->format,
    texture->internalfomat, texture->dataformat, texture->type);

  // force this one
  if(vidCheckErrorF(__FILE__,__LINE__)){
    bprintf("WARNING: Texture Init GL fail\n");
  }
}

booln Texture_getGL(Texture_t *tex, lxScalarArray_t *sarr, void* into, size_t intosize)
{
  size_t size,i,n;
  int mipoffsets[TEX_MAXMIPMAPS];
  enum32 datatype = tex->datatype;
  int level = 0;
  booln getcompressed = tex->storedcompressed;
  booln mipmaps = tex->mipmaps;


  byte *data;

  if (!tex->texID || (tex->format == TEX_FORMAT_BUFFER))
    return LUX_FALSE;

  vidSelectTexture(0);
  vidBindTexture(tex->resinfo.resRID);
  size = Texture_getSizes(tex,mipoffsets,LUX_FALSE,NULL);

  if (!tex->imageData && !sarr && !intosize){
    switch(tex->format) {
    case TEX_FORMAT_CUBE:
      tex->imageData = lxMemGenZalloc(size*6);
      break;
    default:
      tex->imageData = lxMemGenZalloc(size);
      break;
    }
    tex->sarr3d.sarr.data.tvoid = tex->imageData;
  }

  if (!sarr){
    data = intosize ? into : tex->imageData;
    if (intosize){
      getcompressed = LUX_FALSE;
      if (intosize < mipoffsets[0])
        return LUX_FALSE;
    }
  }
  else{
    size_t pixelstride = tex->bpp/8;
    size_t ocompsize = pixelstride/tex->components;
    size_t componentsize = lxScalarType_getSize(sarr->type);

    size =  mipoffsets[0] * (componentsize/ocompsize) * ((tex->format==TEX_FORMAT_CUBE)*6);

    if ( sarr->vectordim != tex->components
      || sarr->vectordim != sarr->stride || sarr->count*componentsize < mipoffsets[0])
      return LUX_FALSE;

    data = sarr->data.tvoid;
    datatype = ScalarType_toGL(sarr->type);

    mipmaps = LUX_FALSE;
    getcompressed = LUX_FALSE;
  }


  switch(tex->format) {
  case TEX_FORMAT_CUBE:
    for (i = 0; i < 6; i++)
    {
      if (getcompressed)
        glGetCompressedTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,level,data);
      else
        glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,level,tex->dataformat,datatype,&data[i*size]);
      if (mipmaps){
        if (getcompressed)
          for (n=1; n<tex->mipmaps; n++){
            glGetCompressedTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,n,&data[mipoffsets[n]+i*size]);
          }
        else
          for (n=1; n<tex->mipmaps; n++){
            glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,n,tex->dataformat,datatype,&data[mipoffsets[n]+i*size]);
          }
      }
    }
    break;
  default:
    if (getcompressed)
      glGetCompressedTexImage(tex->target,level,data);
    else
      glGetTexImage(tex->target,level,tex->dataformat,datatype,data);

    if (mipmaps){
      if (tex->storedcompressed)
        for (n=1; n<tex->mipmaps; n++){
          glGetCompressedTexImage(tex->target,n,&data[mipoffsets[n]]);
        }
      else
        for (n=1; n<tex->mipmaps; n++){
          glGetTexImage(tex->target,n,tex->dataformat,datatype,&data[mipoffsets[n]]);
        }
    }
    break;
  }

  return LUX_TRUE;
}


// copy content of framebuffer into tex
void Texture_copyFramebufferGL(Texture_t *texture,int side,int mip, int xtex, int ytex, int xscreen, int yscreen, int width, int height)
{
  enum32 target = texture->target;

  vidSelectTexture(0);
  vidBindTexture(texture->resinfo.resRID);

  switch(texture->format){
    case TEX_FORMAT_1D:
      if (texture->width >= xtex+width)
        glCopyTexSubImage1D(target,mip,xtex,xscreen,yscreen,width);
      break;
    case TEX_FORMAT_1D_ARRAY:
    case TEX_FORMAT_2D:
    case TEX_FORMAT_RECT:
    case TEX_FORMAT_CUBE:
      if (target == GL_TEXTURE_CUBE_MAP_ARB)
        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X+side;
      if (texture->width >= xtex+width && texture->height >= ytex+height)
        glCopyTexSubImage2D(target,mip,xtex,ytex,xscreen,yscreen,width,height);
      break;
    case TEX_FORMAT_3D:
    case TEX_FORMAT_2D_ARRAY:
      if (texture->width >= xtex+width && texture->height >= ytex+height && texture->depth >= side)
        glCopyTexSubImage3D(target,mip,xtex,ytex,side,xscreen,yscreen,width,height);
      break;
  }

  vidCheckError();
}

void Texture_resubmitGL(Texture_t *texture, int mipmap, int x,int y,int z, int w, int h, int d, void *pdata, enum32 datatype, size_t componentsize, booln partialdata)
{
  enum32 target = texture->target;
  int side = 0;
  uchar *data;
  size_t size = Texture_getSizes(texture,NULL,LUX_FALSE,NULL);
  size_t pixelstride = texture->bpp/8;
  size_t ocompsize = pixelstride/texture->components;
  booln externalmem = g_VID.state.activeBuffers[VID_BUFFER_PIXELFROM] || pdata;

  // change size and stride to match format of externaldata
  if (externalmem){
    size *=     (componentsize/ocompsize);
    pixelstride *=  (componentsize/ocompsize);
  }

  if (texture->iscompressed || (texture->format == TEX_FORMAT_BUFFER))return;

  vidSelectTexture(0);
  vidBindTexture(texture->resinfo.resRID);

  if (target == GL_TEXTURE_CUBE_MAP_ARB){
    target = GL_TEXTURE_CUBE_MAP_POSITIVE_X+z;
    side = z;
    z = 0;
  }

  data = externalmem ? ((char*)pdata) : &texture->imageData[side*size];
  if (x == -1){
    x = 0;
    y = 0;
    z = 0;
    w = texture->width;
    h = texture->height;
    d = texture->depth;
  }
  else{
    x = LUX_MIN(LUX_MAX(0,x),texture->width-1);
    y = LUX_MIN(LUX_MAX(0,y),texture->height-1);
    z = LUX_MIN(LUX_MAX(0,z),texture->depth-1);
    w = LUX_MIN(x+w,texture->width)  - x;
    h = LUX_MIN(y+h,texture->height) - y;
    d = LUX_MIN(z+d,texture->depth)  - z;
    w = LUX_MAX(1,w);
    h = LUX_MAX(1,h);
    d = LUX_MAX(1,d);

    if (externalmem && partialdata){
      data = (uchar*)pdata;
    }
    else{
      if (texture->height == 1){
        data += (x*pixelstride);
      }
      else
      {
        uchar *pout;
        uchar *datastart;
        uchar *buffer;
        int ny,nz;
        int tex2dsize = texture->width*texture->height;
        // ugly case we need to copy the data into buffer

        if (texture->depth == 1 || texture->format == TEX_FORMAT_CUBE){
          z = 0; d = 1;
        }


        bufferminsize(pixelstride*w*d*h);
        buffer = buffermalloc(pixelstride*w*d*h);
        LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

        pout = buffer;

        data+= ((x) + (y*texture->width) + (z*tex2dsize))*pixelstride;
        datastart = data;
        for (nz = 0; nz < d; nz++){
          data = datastart;
          for (ny = 0; ny < h; ny++){
            memcpy(pout,data,pixelstride*w);
            pout+=pixelstride*w;
            data+=pixelstride*texture->width;
          }
          datastart += pixelstride*tex2dsize;
        }
        data = buffer;
      }
    }
  }
  datatype = externalmem ? datatype : texture->datatype;
  vidCheckError();

  switch(texture->format)
  {
  case TEX_FORMAT_2D:
  case TEX_FORMAT_RECT:
  case TEX_FORMAT_CUBE:
  case TEX_FORMAT_1D_ARRAY:
    glTexSubImage2D(target,mipmap,x,y,w,h,texture->dataformat,datatype,data);
    break;

  case TEX_FORMAT_3D:
  case TEX_FORMAT_2D_ARRAY:
    glTexSubImage3D(target,mipmap,x,y,z,w,h,d,texture->dataformat,datatype,data);
    break;

  case TEX_FORMAT_1D:
    glTexSubImage1D(target,mipmap,x,w,texture->dataformat,datatype,data);
    break;
  default:
    LUX_ASSUME(0);
  }

  vidCheckError();
}

void Texture_clamp(Texture_t *texture,int state)
{
  if (state != texture->clamped){
    vidSelectTexture(0);
    vidBindTexture(texture->resinfo.resRID);
    texture->clamped = state;
    if (texture->clamped){
      glTexParameteri(texture->target,GL_TEXTURE_WRAP_S,texture->clamped & TEX_CLAMP_X ? GL_CLAMP_TO_EDGE : GL_REPEAT);
      glTexParameteri(texture->target,GL_TEXTURE_WRAP_T,texture->clamped & TEX_CLAMP_Y ? GL_CLAMP_TO_EDGE : GL_REPEAT);
      glTexParameteri(texture->target,GL_TEXTURE_WRAP_R,texture->clamped & TEX_CLAMP_Z ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    }
    else if (texture->format != TEX_FORMAT_RECT){
      glTexParameteri(texture->target,GL_TEXTURE_WRAP_S,GL_REPEAT);
      glTexParameteri(texture->target,GL_TEXTURE_WRAP_T,GL_REPEAT);
      glTexParameteri(texture->target,GL_TEXTURE_WRAP_R,GL_REPEAT);
    }
  }
}

int  Texture_hasMipMaps(Texture_t *texture)
{
  return ((texture->attributes & TEX_ATTR_MIPMAP)!=0 || texture->mipmaps);
}

void Texture_MipMapping(Texture_t* texture,int state){
  if (state == texture->mipmapfiltered || (!Texture_hasMipMaps(texture) && !GLEW_SGIS_generate_mipmap))
    return;
  vidCheckError();
  vidSelectTexture(0);
  vidBindTexture(texture->resinfo.resRID);

  if (state){
    // enable
    if (!isFlagSet(texture->notfiltered,TEX_FILTER_MIN))
      glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, (g_VID.details == VID_DETAIL_HI) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST);

    if (GLEW_SGIS_generate_mipmap && !texture->mipmaps)
      glTexParameteri(texture->target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
    texture->attributes |= TEX_ATTR_MIPMAP;
  }
  else{
    // disable
    if (!isFlagSet(texture->notfiltered,TEX_FILTER_MIN))
      glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


    if (GLEW_SGIS_generate_mipmap && !texture->mipmaps){
      glTexParameteri(texture->target, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);
      texture->attributes &= ~TEX_ATTR_MIPMAP;
    }

  }
  vidCheckError();
  texture->mipmapfiltered = state;
}

void Texture_filterType(Texture_t *texture, enum32 filter, int state)
{
  if (!state){
    glTexParameteri(texture->target, filter, GL_NEAREST);
  }
  else{
    if (texture->mipmapfiltered){
      if (filter == GL_TEXTURE_MAG_FILTER)
        glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      else{
        if (g_VID.details == VID_DETAIL_HI)
          glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        else
          glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
      }
    }
    else{
      glTexParameteri(texture->target, filter, GL_LINEAR);
    }
  }
}

void Texture_filter(Texture_t * texture, int min, int mag, int force)
{
  int flag = ((min) ? 0 : TEX_FILTER_MIN) | ((mag) ? 0 : TEX_FILTER_MAG);
  if (flag == texture->notfiltered && !force)
    return;

  vidSelectTexture(0);
  vidBindTexture(texture->resinfo.resRID);
  Texture_filterType(texture,GL_TEXTURE_MAG_FILTER,mag);
  Texture_filterType(texture,GL_TEXTURE_MIN_FILTER,min);

  if (flag && g_VID.useTexAnisotropic && g_VID.capTexAnisotropic){
    glTexParameterf(texture->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
  }
  else if (g_VID.useTexAnisotropic && g_VID.capTexAnisotropic && !texture->notfiltered){
    glTexParameterf(texture->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, g_VID.lvlTexAnisotropic);
  }
  texture->notfiltered = flag;
}

int Texture_saveToFile(Texture_t *texture, char *filename, int quality, int side)
{
  char *name = filename;
  char *imageBuffer;
  enum32 target = texture->target;
  int outtype;
  size_t outsize;
  int tga = LUX_FALSE;

  if (texture->format != TEX_FORMAT_RECT && texture->format != TEX_FORMAT_1D && texture->format != TEX_FORMAT_2D)
    return LUX_FALSE;

  while (*name) name++;
  if (*(int*)&name[-4] == *(int*)".tga"){
    tga = LUX_TRUE;
    outtype = texture->dataformat;
  }
  else if (*(int*)&name[-4] == *(int*)".jpg")
    outtype = GL_RGB;
  else
    return LUX_FALSE;

  outsize = (texture->bpp/8)*texture->width*texture->height;

  bufferminsize(outsize);
  imageBuffer = buffermalloc(outsize);
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());


  vidSelectTexture(0);
  vidBindTexture(texture->resinfo.resRID);

  if (target == GL_TEXTURE_CUBE_MAP_ARB)
    target = GL_TEXTURE_CUBE_MAP_POSITIVE_X+side;

  glGetTexImage(target,0,outtype,GL_UNSIGNED_BYTE,imageBuffer);

  if (tga)
    fileSaveTGA(filename, texture->width, texture->height, texture->bpp/8, imageBuffer,LUX_FALSE);
  else
    fileSaveJPG(filename, texture->width, texture->height, texture->bpp/8, quality, imageBuffer);

  return LUX_TRUE;
}

#define bcstr(type) \
  case type:  return #type;

const char* TextureType_toString(TextureType_t type)
{
  switch(type){
    bcstr(TEX_UNSET)

    bcstr(TEX_COLOR)
    bcstr(TEX_RGB)
    bcstr(TEX_RGBA)
    bcstr(TEX_ALPHA)
    bcstr(TEX_LUM)
    bcstr(TEX_DEPTH)
    bcstr(TEX_DEPTH16)
    bcstr(TEX_DEPTH24)
    bcstr(TEX_DEPTH32)
    bcstr(TEX_BGR)
    bcstr(TEX_BGRA)
    bcstr(TEX_ABGR)
    bcstr(TEX_LUMALPHA)
    bcstr(TEX_STENCIL)
    bcstr(TEX_DEPTH_STENCIL)
    bcstr(TEX_R)
    bcstr(TEX_RG)
    bcstr(TEX_SHARED_EXPONENT)

    bcstr(TEX_TYPES)

    bcstr(TEX_NOUPLOAD)
    bcstr(TEX_BY_NAMESTRING)
    bcstr(TEX_3D_ATTENUATE)
    bcstr(TEX_CUBE_NORMALIZE)
    bcstr(TEX_CUBE_SPECULAR)
    bcstr(TEX_CUBE_DIFFUSE)
    bcstr(TEX_2D_WHITE)
    bcstr(TEX_2D_DEFAULT)
    bcstr(TEX_2D_COMBINE1D)
    bcstr(TEX_2D_COMBINE2D_16)
    bcstr(TEX_2D_COMBINE2D_64)

    bcstr(TEX_CUBE_SKYBOX)
    bcstr(TEX_2D_LIGHTMAP)
    bcstr(TEX_MATERIAL)
    bcstr(TEX_DUMMY)
  }
  return "TEX_ERROR";
}

#undef bcstr

//////////////////////////////////////////////////////////////////////////
// Special Makes

TextureType_t Texture_makeFromString(Texture_t *tex,char *instruction)
{
  // string is formated: width,height,depth,type,winsized,notfiltered,mipmap,datatype
  // depth == -6: cube
  // negative height/depth: array

  TextureType_t type;
  int mipmap;
  TextureDataType_t datatype;

  tex->notcompress = LUX_TRUE;

  sscanf(instruction,"%d,%d,%d,%d,%d,%d,%d,%d",&tex->width,&tex->height,&tex->depth,&type,&tex->windowsized,&tex->notfiltered,&mipmap,&datatype);

  if (!g_VID.capTexFloat && (datatype == TEX_DATA_FLOAT16 || datatype == TEX_DATA_FLOAT16))
    datatype = TEX_DATA_FIXED8;

  if (!g_VID.capTexInt && (datatype >= TEX_DATA_BYTE && datatype <= TEX_DATA_UINT))
    datatype = TEX_DATA_FIXED8;

  tex->sctype = TextureDataType_toScalar(datatype);

  switch(type) {
  case TEX_RGB:
    tex->components = 3;
    break;
  case TEX_RGBA:
    tex->components = 4;
    break;
  case TEX_ALPHA:
    tex->components = 1;
    break;
  case TEX_LUM:
    tex->components = 1;
    break;
  case TEX_DEPTH:
  case TEX_DEPTH16:
  case TEX_DEPTH24:
  case TEX_DEPTH32:
    tex->components = 1;
    break;
  case TEX_DEPTH_STENCIL:
    tex->components = 4;
    break;
  case TEX_BGR:
    tex->components = 3;
    break;
  case TEX_BGRA:
    tex->components = 4;
    break;
  case TEX_ABGR:
    tex->components = 4;
    break;
  case TEX_LUMALPHA:
    tex->components = 2;
    break;
  case TEX_RG:
    tex->components = 2;
    break;
  case TEX_R:
    tex->components = 1;
    break;
  default:
    break;
  }

  tex->bpp = tex->components * lxScalarType_getSize(tex->sctype)*8;
  tex->attributes = TextureDataType_toFlag(datatype,tex->attributes);

  if (!(tex->attributes & (TEX_ATTR_RECT | TEX_ATTR_BUFFER | TEX_ATTR_CUBE | TEX_ATTR_3D | TEX_ATTR_ARRAY)) &&
    tex->depth < 2 && GLEW_NV_texture_rectangle && (tex->width != lxNearestPowerOf2(tex->width) || tex->height != lxNearestPowerOf2(tex->height) ))
  {
    //tex->attributes |= TEX_ATTR_RECT;
    //mipmap = FALSE;
  }

  tex->notcompress = LUX_TRUE;

  if (tex->notfiltered)
    tex->attributes |= TEX_ATTR_NOTFILTERED;

  if (tex->windowsized){
    tex->attributes |= TEX_ATTR_WINDOWSIZED;
    tex->attributes &= ~TEX_ATTR_KEEPDATA;
  }
  else if (tex->attributes & TEX_ATTR_KEEPDATA){
    int d = (tex->attributes & TEX_ATTR_CUBE) ? 6 : tex->depth;
    tex->imageData = lxMemGenZalloc(sizeof(char)*tex->width*d*tex->height*tex->bpp/8);
  }

  tex->isuser = LUX_TRUE;

  if (mipmap)
    tex->attributes |= TEX_ATTR_MIPMAP;

  return type;
}


int Texture_scale( Texture_t *texture, int outwidth, int outheight)
{
  uchar   *out;

  if (!texture->imageData)
    return LUX_FALSE;

  if (texture->height == outheight && texture->width == outwidth)
    return LUX_FALSE;

  out = (uchar *)lxMemGenZalloc(outheight*outwidth*(texture->bpp/8));

  gluScaleImage(  texture->dataformat, texture->width, texture->height, texture->datatype, texture->imageData,
    outwidth, outheight, texture->datatype, out);

  Texture_clearData(texture);

  texture->width = outwidth;
  texture->height = outheight;
  texture->imageData = out;
  return LUX_TRUE;
}

int Texture_combine(Texture_t* textures, int num,Texture_t* target, TextureCombineModel_t mode)
{
  int i,height,width;
  int pad;
  char* out;
  char* in;

  // texes should be power of 2 and equal size
  // if the types are different we wont bother with conversion
  Texture_size(&textures[0], LUX_FALSE,LUX_FALSE);
  for (i = 0; i < num-1; i++){
    if (textures[i].bpp != textures[i+1].bpp){
      lprintf("ERROR texcombine: textures not same type\n");
      return LUX_FALSE;
    }
    if (textures[i].height != textures[i+1].height || textures[i].width != textures[i+1].width ){
      Texture_scale(&textures[i+1],textures[0].height,textures[0].width);
    }
  }
  target->bpp = textures[0].bpp;
  target->components = textures[0].components;
  target->dataformat = textures[0].dataformat;
  target->datatype = textures[0].datatype;
  target->height = textures[0].height;
  target->width = textures[0].width;
  target->datatype = textures[0].datatype;
  target->sctype = textures[0].sctype;
  target->depth = 1;


  switch(mode){
    case TEX_COMBINE_CONCAT:
    {
      pad = target->height * target->width * target->components *sizeof(char);
      target->imageData = lxMemGenZalloc(pad * num);
      for (i = 0; i < num; i++)
      {
        memcpy(&target->imageData[i*pad],textures[i].imageData,pad);
      }
      target->height *= num;
    }
    break;
  case TEX_COMBINE_PACK:
    {
      int n;
      int size = target->height * target->width;

      pad = target->height * target->width * sizeof(char);
      target->bpp = num*8;
      target->imageData = lxMemGenZalloc(pad * num);
      target->dataformat = (num == 3) ? GL_RGB : GL_RGBA;
      for (i = 0; i < num; i++)
      {
        Texture_makeAlpha(&textures[i]);
        out = target->imageData+i;
        in = textures[i].imageData;
        for (n = 0; n < size; n++,out+=num,in++ ){
          *out = *in;
        }
      }
    }
    break;
  default:
  case TEX_COMBINE_INTERLEAVE:
    target->width = lxNearestPowerOf2(num*textures[0].width);
    pad = target->width - num*textures[0].width;
    if ((int)target->width > g_VID.capTexSize){
      lprintf("ERROR texcombine: sequence width too big\n");
      return LUX_FALSE;
    }

    target->imageData = lxMemGenZalloc(target->height * target->width * target->bpp/8 *sizeof(char));

    out = target->imageData;
    // append data
    for(height = 0; height < target->height; height++){
      for (i = 0; i < num; i++){
        in = &textures[i].imageData[height * textures[0].width * target->bpp/8];
        for (width = 0; width < textures[0].width; width++){
          switch(target->dataformat){
          case GL_RGB:
            out[0] = in[0];
            out[1] = in[1];
            out[2] = in[2];
            out += 3;
            in  += 3;
            break;
          case GL_RGBA:
            out[0] = in[0];
            out[1] = in[1];
            out[2] = in[2];
            out[3] = in[3];
            out += 4;
            in  += 4;
            break;
          }
        }
      }
      for (i = 0; i < pad; i++){
        switch(target->dataformat){
        case GL_RGB:
          out[0] =out[1] =out[2] = 0;
          out += 3;
          break;
        case GL_RGBA:
          out[0] = out[1] = out[2] = out[3] = 0;
          out += 4;
          break;
        }
      }
    }
    break;
  }

  return LUX_TRUE;
}


void Texture_sample(Texture_t *tex,lxVector3 coords,int clamp, lxVector4 coloroutvec)
{
  booln notclamped[3] = {!clamp,!clamp,!clamp};

  if (!tex->imageData || tex->sarr3d.sarr.type == LUX_SCALAR_ILLEGAL)
    return;


  if (tex->format == TEX_FORMAT_CUBE){
    uint shift = lxSampleCubeTo2DCoord(coords,coords);
    uint offset[3] = {0,0,shift * tex->width * tex->height};
    lxScalarArray3D_setDataOffset(&tex->sarr3d,offset,tex->imageData);
  }
  else{
    lxScalarArray3D_setData(&tex->sarr3d,tex->imageData);
  }

  lxScalarArray3D_sampleLinear(coloroutvec,&tex->sarr3d,coords,notclamped);
  if (tex->sarr3d.sarr.type != LUX_SCALAR_FLOAT32)
    lxScalarType_normalizedFloat(coloroutvec,tex->sarr3d.sarr.type,coloroutvec,tex->sarr3d.sarr.vectordim);


  switch(tex->components)
  {
  case 1: coloroutvec[1] = coloroutvec[2] = coloroutvec[0]; coloroutvec[3] = 1.0f; break;
  case 2: coloroutvec[2] = coloroutvec[3] = 0.0f; break;
  case 3: coloroutvec[3] = 1.0f; break;
  default:
    break;
  }
}

static int Array3D_index(int x, int y, int z, int w, int h, int d)
{
  int outx = LUX_MIN(LUX_MAX(0,x),w-1);
  int outy = LUX_MIN(LUX_MAX(0,y),h-1);
  int outz = LUX_MIN(LUX_MAX(0,z),d-1);
  return ((outy*w) + outx)+(outz*(w*h));
}

static int Array3D_indexWrap(int x,int y,int z, int w, int h, int d)
{
  int outx = (x < 0 ? w+x : x) % w;
  int outy = (y < 0 ? h+y : y) % h;
  int outz = (z < 0 ? d+z : z) % d;

  return ((outy*w) + outx)+(outz*(w*h));
}

void Texture_getPixel(Texture_t *tex, int x, int y, int z,int wrap, lxVector4 outcolor)
{
  int index;
  int stride;
  uchar *ptr;
  uint  depth = (tex->format == TEX_FORMAT_CUBE) ? 6 : tex->depth;

  if (!tex->imageData)
    return;

  stride = tex->bpp/8;
  index = wrap ? Array3D_indexWrap(x,y,z,tex->width,tex->height,depth) :  Array3D_index(x,y,z,tex->width,tex->height,depth);

  ptr= &tex->imageData[(index)*stride];

  if (tex->sctype != LUX_SCALAR_FLOAT32){
    lxScalarType_toFloatNormalized(outcolor,tex->sctype,ptr,tex->components);
  }
  else{
    lxVector4Copy(outcolor,(float*)ptr);
  }


  switch(tex->components)
  {
  case 1: outcolor[1] = outcolor[2] = outcolor[0]; outcolor[3] = 1.0f; break;
  case 2: outcolor[2] = outcolor[3] = 0.0f; break;
  case 3: outcolor[3] = 1.0f; break;
  default:
    break;
  }
}

void Texture_setPixel(Texture_t *tex, int x, int y, int z,lxVector4 incolor)
{
  int index;
  int stride;
  uchar *ptr;
  float *pFloat;
  uint  depth = (tex->format == TEX_FORMAT_CUBE) ? 6 : tex->depth;

  if (!tex->imageData)
    return;

  stride = tex->bpp/8;

  index = Array3D_index(x,y,z,tex->width,tex->height,depth);

  ptr= &tex->imageData[(index)*stride];

  if (tex->sctype != LUX_SCALAR_FLOAT32){
    lxScalarType_fromFloatNormalized(ptr,tex->sctype,incolor,tex->components);
  }
  else{
    pFloat = (float*)ptr;
    switch(tex->components)
    {
    case 1: *pFloat = incolor[0]; return;
    case 2: LUX_ARRAY2COPY(pFloat,incolor); return;
    case 3: LUX_ARRAY3COPY(pFloat,incolor); return;
    case 4: LUX_ARRAY4COPY(pFloat,incolor); return;
    }
  }


}


void Texture_getPixelUB(Texture_t *tex, int x, int y, int z,int wrap, uchar *outcolor)
{
  int index;
  int stride;
  uchar *ptr;
  uint  depth = (tex->format == TEX_FORMAT_CUBE) ? 6 : tex->depth;

  if (!tex->imageData)
    return;

  stride = tex->bpp/8;
  index = wrap ? Array3D_indexWrap(x,y,z,tex->width,tex->height,depth) :  Array3D_index(x,y,z,tex->width,tex->height,depth);

  ptr= &tex->imageData[(index)*stride];

  switch(tex->datatype)
  {
  case GL_FLOAT:
    lxVector4ubyte_FROM_float(outcolor,(float*)ptr);
    break;
  case GL_UNSIGNED_BYTE:
    LUX_ARRAY4COPY(outcolor,ptr);
    break;
  case GL_UNSIGNED_SHORT:
    lxVector4ubyte_FROM_ushort(outcolor,(ushort*)ptr);
    break;
  default:
    LUX_ASSERT(0);
    break;

  }

  switch(tex->components)
  {
  case 1: outcolor[1] = outcolor[2] = outcolor[0]; outcolor[3] = 255; break;
  case 2: outcolor[2] = outcolor[3] = 0; break;
  case 3: outcolor[3] = 255; break;
  default:
    break;
  }
}

void Texture_setPixelUB(Texture_t *tex, int x, int y, int z,uchar *incolor)
{
  float colorf[4];
  ushort colorus[4];
  int index;
  int stride;
  uchar *ptr;
  float *pFloat;
  ushort *pUshort;
  uint  depth = (tex->format == TEX_FORMAT_CUBE) ? 6 : tex->depth;

  if (!tex->imageData)
    return;

  stride = tex->bpp/8;

  index = Array3D_index(x,y,z,tex->width,tex->height,depth);

  ptr= &tex->imageData[(index)*stride];
  switch(tex->datatype)
  {
  case GL_FLOAT:
    lxVector4float_FROM_ubyte(colorf,incolor);
    pFloat = (float*)ptr;
    switch(tex->components)
    {
    case 1: *pFloat = colorf[0]; return;
    case 2: LUX_ARRAY2COPY(pFloat,colorf); return;
    case 3: LUX_ARRAY3COPY(pFloat,colorf); return;
    case 4: LUX_ARRAY4COPY(pFloat,colorf); return;
    }
    return;
  case GL_UNSIGNED_BYTE:
    switch(tex->components)
    {
    case 1: *ptr = incolor[0]; return;
    case 2: LUX_ARRAY2COPY(ptr,incolor); return;
    case 3: LUX_ARRAY3COPY(ptr,incolor); return;
    case 4: LUX_ARRAY4COPY(ptr,incolor); return;
    }
    return;
  case GL_UNSIGNED_SHORT:
    pUshort = (ushort*)ptr;
    lxVector4ushort_FROM_ubyte(colorus,incolor);
    switch(tex->components)
    {
    case 1: *pUshort = colorus[0]; return;
    case 2: LUX_ARRAY2COPY(pUshort,colorus); return;
    case 3: LUX_ARRAY3COPY(pUshort,colorus); return;
    case 4: LUX_ARRAY4COPY(pUshort,colorus); return;
    }
    return;
  default:
    LUX_ASSERT(0);
  }

}

void  Texture_resize(Texture_t *tex,int w,int h, int d)
{
  if (!tex->isuser || !tex->texID)
    return;

  Texture_clearData(tex);
  LUX_PROFILING_OP (g_Profiling.global.memory.vidtexmem -= tex->memcosts);
  //Texture_clearGL(tex);

  if (tex->attributes & TEX_ATTR_KEEPDATA)
    tex->imageData = lxMemGenZalloc(w*h*d*sizeof(uchar)*(tex->bpp/8));

  tex->width = w;
  tex->height = h;
  tex->depth = d;

  Texture_initGL(tex,LUX_TRUE);
}
void Texture_fixFloat(Texture_t *texture,int values)
{
  float *newdata;
  float *outPtr;
  ushort *inPtr;
  int i;

  if (texture->datatype == GL_UNSIGNED_BYTE)
    return;

  if (g_VID.capTexFloat){
    if (texture->datatype == GL_FLOAT){
      texture->attributes = TextureDataType_toFlag(TEX_DATA_FLOAT32,texture->attributes);
      return;
    }
    texture->attributes = TextureDataType_toFlag(TEX_DATA_FLOAT16,texture->attributes);
  }

  // convert halfs into floats
    outPtr = newdata = lxMemGenZalloc(sizeof(float)*values);
  inPtr = (ushort*)texture->imageData;

  for (i = 0; i < values; i++,inPtr++,outPtr++){
    *outPtr = lxFloat16To32(*inPtr);
  }
  texture->datatype = GL_FLOAT;

  lxMemGenFree(texture->imageData,sizeof(ushort)*values);
  texture->imageDataFloat = newdata;
}


void Texture_makeDefault(Texture_t *texture)
{
  char name[]="default.tex";
  uchar image[48] =
  {
    255,255,255,  225,225,225,  255,255,255,  215,215,215,
    225,225,225,  255,255,255,  225,225,225,  255,255,255,
    255,255,255,  225,225,225,  255,255,255,  225,225,225,
    215,215,215,  255,255,255,  225,225,225,  255,255,255,
  };

  Texture_clearData(texture);

  texture->imageData = (uchar *)lxMemGenZalloc(sizeof(uchar)*3*16);
  memcpy(texture->imageData, image, sizeof(uchar)*3*16);
  texture->dataformat = GL_RGB;
  texture->datatype = GL_UNSIGNED_BYTE;
  texture->sctype = LUX_SCALAR_UINT8;
  texture->internalfomat = GL_RGB;
  texture->bpp = 24;
  texture->components = 3;
  texture->width = 4;
  texture->height = 4;
  texture->depth = 1;
  texture->uptype = TEX_RGB;
  texture->type = TEX_COLOR;
  texture->format = TEX_FORMAT_2D;
  texture->notfiltered = TEX_FILTERS;
  Texture_initGL(texture,LUX_FALSE);//no mipmap
  //texture->resinfo.name = (char *)reszalloc(sizeof(char) * (strlen(name)+1));
  //strcpy(texture->resinfo.name,name);
}


void getCubeVector(lxVector3 vector,GLenum side, float cubesize, int x, int y){
  float s,t,sc,tc;
  s = (x + 0.5) / cubesize;
  t = (y + 0.5) / cubesize;
  sc = s*2 - 1;
  tc = t*2 - 1;

  switch(side){
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      vector[0] = 1;
      vector[1] = -tc;
      vector[2] = -sc;
      break;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      vector[0] = -1;
      vector[1] = -tc;
      vector[2] = sc;
      break;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      vector[0] = sc;
      vector[1] = 1;
      vector[2] = tc;
      break;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      vector[0] = sc;
      vector[1] = -1;
      vector[2] = -tc;
      break;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      vector[0] = sc;
      vector[1] = -tc;
      vector[2] = 1;
      break;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      vector[0] = -sc;
      vector[1] = -tc;
      vector[2] = -1;
      break;
  }

  lxVector3Normalized(vector);
}
TextureType_t Texture_makeNormalize(Texture_t *tex)
{
  int i,y,x;
  lxVector3 vector;
  uchar *pixels;

  tex->imageData = pixels = lxMemGenZalloc(VID_CUBEMAP_SIZE*VID_CUBEMAP_SIZE*3*6*sizeof(uchar));

  for (i = GL_TEXTURE_CUBE_MAP_POSITIVE_X; i <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;i++)
  {
    for (y = 0; y < VID_CUBEMAP_SIZE; y++)
    {
      for (x = 0; x < VID_CUBEMAP_SIZE; x++)
      {
        getCubeVector(vector,i, VID_CUBEMAP_SIZE, x, y);
        pixels[(y*VID_CUBEMAP_SIZE + x)*3 + 0] = (128 + (vector[0]*127));
        pixels[(y*VID_CUBEMAP_SIZE + x)*3 + 1] = (128 + (vector[1]*127));
        pixels[(y*VID_CUBEMAP_SIZE + x)*3 + 2] = (128 + (vector[2]*127));
      }
    }
    pixels+=(VID_CUBEMAP_SIZE*VID_CUBEMAP_SIZE*3);
  }

  tex->bpp = 24;
  tex->components = 3;
  tex->internalfomat = GL_RGB;
  tex->dataformat = GL_RGB;
  tex->datatype = GL_UNSIGNED_BYTE;
  tex->sctype = LUX_SCALAR_UINT8;
  tex->height = VID_CUBEMAP_SIZE;
  tex->width = VID_CUBEMAP_SIZE;
  tex->depth = 1;
  tex->format = TEX_FORMAT_CUBE;
  tex->attributes |= TEX_ATTR_CUBE;
  tex->clamped = TEX_CLAMP_ALL;
  tex->notcompress = LUX_TRUE;

  return TEX_RGB;
}
TextureType_t Texture_makeSpecular(Texture_t *tex)
{
  int i,y,x;
  lxVector3 vector;
  lxVector3 up;
  uchar *pixels;
  float ndotl;

  tex->imageData = pixels = lxMemGenZalloc(VID_CUBEMAP_SIZE*VID_CUBEMAP_SIZE*6*sizeof(uchar));

  lxVector3Set(up,0,0,1);
  for (i = GL_TEXTURE_CUBE_MAP_POSITIVE_X; i <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;i++)
  {
    for (y = 0; y < VID_CUBEMAP_SIZE; y++)
    {
      for (x = 0; x < VID_CUBEMAP_SIZE; x++)
      {
        getCubeVector(vector,i, VID_CUBEMAP_SIZE, x, y);
        ndotl = lxVector3Dot(up,vector);
        ndotl = LUX_MAX(0,ndotl);
        if (ndotl > 0)
          ndotl = pow(ndotl,8);
        pixels[y*VID_CUBEMAP_SIZE + x] = ndotl * 255.0;
      }
    }
    pixels+=(VID_CUBEMAP_SIZE*VID_CUBEMAP_SIZE);
  }

  tex->bpp = 8;
  tex->components = 1;
  tex->internalfomat = GL_INTENSITY;
  tex->dataformat = GL_LUMINANCE;
  tex->datatype = GL_UNSIGNED_BYTE;
  tex->sctype = LUX_SCALAR_UINT8;
  tex->height = VID_CUBEMAP_SIZE;
  tex->width = VID_CUBEMAP_SIZE;
  tex->depth = 1;
  tex->format = TEX_FORMAT_CUBE;
  tex->attributes |= TEX_ATTR_CUBE;
  tex->clamped = TEX_CLAMP_ALL;
  tex->notcompress = LUX_TRUE;

  return TEX_INTENSE;
}

TextureType_t Texture_makeDiffuse(Texture_t *tex)
{
  int i,y,x;
  lxVector3 vector;
  lxVector3 up;
  uchar *pixels;
  float ndotl;

  tex->imageData = pixels = lxMemGenZalloc(VID_CUBEMAP_SIZE*VID_CUBEMAP_SIZE*6*sizeof(uchar));

  lxVector3Set(up,0,0,1);
  for (i = GL_TEXTURE_CUBE_MAP_POSITIVE_X; i <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;i++)
  {
    for (y = 0; y < VID_CUBEMAP_SIZE; y++)
    {
      for (x = 0; x < VID_CUBEMAP_SIZE; x++)
      {
        getCubeVector(vector,i, VID_CUBEMAP_SIZE, x, y);
        ndotl = lxVector3Dot(up,vector);
        ndotl = LUX_MAX(0,ndotl);
        pixels[y*VID_CUBEMAP_SIZE + x] = ndotl * 255.0;
      }
    }
    pixels+=(VID_CUBEMAP_SIZE*VID_CUBEMAP_SIZE);
  }

  tex->bpp = 8;
  tex->components = 1;
  tex->internalfomat = GL_INTENSITY;
  tex->dataformat = GL_LUMINANCE;
  tex->datatype = GL_UNSIGNED_BYTE;
  tex->sctype = LUX_SCALAR_UINT8;
  tex->height = VID_CUBEMAP_SIZE;
  tex->width = VID_CUBEMAP_SIZE;
  tex->depth = 1;
  tex->format = TEX_FORMAT_CUBE;
  tex->attributes |= TEX_ATTR_CUBE;
  tex->clamped = TEX_CLAMP_ALL;
  tex->notcompress = LUX_TRUE;

  return TEX_INTENSE;
}

// by ATI  RadeonDot3Bump.cpp
TextureType_t Texture_makeAttenuate(Texture_t *tex)
{
  // Generate the lightmap texture
  uchar *lightmap;
  int r,s,t,half;
  float range_sq = 3*3;

  tex->imageData = lightmap = lxMemGenZalloc(VID_LIGHTMAP_3D_SIZE*VID_LIGHTMAP_3D_SIZE*VID_LIGHTMAP_3D_SIZE*sizeof(uchar));
  /*
  #define SQ(A) ((A)*(A))
  float rad = (VID_LIGHTMAP_3D_SIZE/2) -1;
  float rad_sq = rad*rad;
  // here we had square distance
  half = VID_LIGHTMAP_3D_SIZE/2;
  for ( r = 0; r < VID_LIGHTMAP_3D_SIZE; r++) {
  for ( t = 0; t < VID_LIGHTMAP_3D_SIZE; t++) {
  for ( s = 0; s < VID_LIGHTMAP_3D_SIZE; s++) {
  float d_sq = SQ(r-half) + SQ(s-half) + SQ(t-half);
  if (d_sq < rad_sq) {
  float falloff = SQ((rad_sq - d_sq)/rad_sq);
  lightmap[r*VID_LIGHTMAP_3D_SIZE*VID_LIGHTMAP_3D_SIZE +
  t*VID_LIGHTMAP_3D_SIZE + s] = (255.0f * falloff);
  } else {
  lightmap[r*VID_LIGHTMAP_3D_SIZE*VID_LIGHTMAP_3D_SIZE +
  t*VID_LIGHTMAP_3D_SIZE + s] = 0;
  }
  }
  }
  #undef SQ
  }*/

  half = VID_LIGHTMAP_3D_SIZE/2;
  for ( r = 0; r < VID_LIGHTMAP_3D_SIZE; r++) {
    for ( t = 0; t < VID_LIGHTMAP_3D_SIZE; t++) {
      for ( s = 0; s < VID_LIGHTMAP_3D_SIZE; s++) {
        float fr = -1.0f + (2.0f*r+1.0f)/(float)VID_LIGHTMAP_3D_SIZE;
        float fs = -1.0f + (2.0f*s+1.0f)/(float)VID_LIGHTMAP_3D_SIZE;
        float ft = -1.0f + (2.0f*t+1.0f)/(float)VID_LIGHTMAP_3D_SIZE;

        fr = exp(-(fr*fr*range_sq));
        fs = exp(-(fs*fs*range_sq));
        ft = exp(-(ft*ft*range_sq));

        lightmap[r*VID_LIGHTMAP_3D_SIZE*VID_LIGHTMAP_3D_SIZE +  t*VID_LIGHTMAP_3D_SIZE + s] = 255.0f*(fr*fs*ft);

      }
    }
  }

  tex->bpp = 8;
  tex->components = 1;
  tex->clamped = LUX_TRUE;
  tex->internalfomat = GL_INTENSITY;
  tex->dataformat = GL_LUMINANCE;
  tex->datatype = GL_UNSIGNED_BYTE;
  tex->sctype = LUX_SCALAR_UINT8;
  tex->depth = VID_LIGHTMAP_3D_SIZE;
  tex->height = VID_LIGHTMAP_3D_SIZE;
  tex->width = VID_LIGHTMAP_3D_SIZE;
  tex->notcompress = LUX_TRUE;
  tex->format = TEX_FORMAT_3D;
  return TEX_INTENSE;
}

TextureType_t Texture_makeDotZ(Texture_t *texture)
{
  uchar *cubedata;
  int i,y,x;
  lxVector3 vector;
  lxVector3 up;
  uchar *pixels;
  uchar *out;
  float ndotl;
  int cubesize;
  int pixelsize;
  lxVector3 coords;
  lxVector4 color;
  uchar colorb[4];

  cubesize = texture->width/2;
  pixelsize = texture->bpp/8;

  cubedata = pixels = lxMemGenZalloc(cubesize*cubesize*6*pixelsize);
  Texture_updateScalarArray(texture);

  // FIXME sampling before scalararray is inited!
  lxVector3Set(up,0,0,1);
  for (i = GL_TEXTURE_CUBE_MAP_POSITIVE_X; i <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;i++)
  {
    for (y = 0; y < cubesize; y++)
    {
      for (x = 0; x < cubesize; x++)
      {
        getCubeVector(vector,i, cubesize, x, y);
        ndotl = lxVector3Dot(up,vector);
        ndotl *= 0.5;
        ndotl += 0.5;
        lxVector2Set(coords,ndotl,0);
        Texture_sample(texture,coords,LUX_TRUE,color);
        lxVector4ubyte_FROM_float(colorb,color);

        out = &pixels[(y*cubesize + x)*pixelsize];
        switch(pixelsize){
        case 1:
          *out = *colorb;
          break;
        case 3:
          LUX_ARRAY3COPY(out,colorb);
          break;
        case 4:
          LUX_ARRAY4COPY(out,colorb);
          break;
        }
      }
    }
    pixels+=(cubesize*cubesize*pixelsize);
  }
  Texture_clearData(texture);
  texture->imageData = cubedata;
  texture->width = texture->width/2;
  texture->height = texture->width;
  texture->depth = 1;
  texture->format = TEX_FORMAT_CUBE;
  texture->attributes |= TEX_ATTR_CUBE;
  return Texture_getAutoUptype(texture);
}

void Texture_makeWindowSized(Texture_t *texture)
{
  texture->windowsized = LUX_TRUE;
  texture->notcompress = LUX_TRUE;

  if (GLEW_NV_texture_rectangle){
    texture->format = TEX_FORMAT_RECT;
    texture->attributes |= TEX_ATTR_RECT;

    texture->width = g_VID.windowWidth;
    texture->height = g_VID.windowHeight;

  }
  else{
    texture->format = TEX_FORMAT_2D;

    texture->width = lxNextPowerOf2(g_VID.windowWidth);
    texture->height = lxNextPowerOf2(g_VID.windowHeight);

  }
  texture->depth = 1;
  texture->imageData = NULL;
}

TextureType_t Texture_makeCombine1D(Texture_t *texture)
{
  uchar *in;
  uchar *out;
  uchar *newpixels;
  int x,y,block;
  int   div;
  int   newwidth;
  int   newheight;
  int   channels;

  if (texture->type == TEX_2D_COMBINE2D_16)
    div = 4;
  else
    div = 8;
  channels = texture->bpp/8;

  out = newpixels = lxMemGenZalloc(texture->width*texture->height*sizeof(uchar)*channels);

  newwidth = (texture->width/div)*div*div;
  newheight = (texture->height/div);

  for (y = 0; y < newheight; y++) {
    for (block = div-1; block >= 0; block--) {
      in = &texture->imageData[(block*newheight+y)*texture->width*channels];
      for (x = 0; x < (int)texture->width*channels; x++,out++,in++) {
        *out = *in;
      }
    }
  }

  Texture_clearData(texture);
  texture->height = newheight;
  texture->width = newwidth;
  texture->depth = 1;
  texture->imageData = newpixels;
  texture->format = TEX_FORMAT_2D;
  return Texture_getAutoUptype(texture);
}


TextureType_t Texture_makeProjCube(Texture_t *texture)
{
  uchar   *cubeData;
  int i;
  size_t    pixelsize = texture->bpp/8;
  size_t    imgsize = sizeof(uchar)*texture->width*texture->height*pixelsize;

  LUX_ASSERT(texture->imageData);

  LUX_ARRAY4COPY(texture->border,texture->imageData);
  texture->target = GL_TEXTURE_CUBE_MAP_ARB;

  cubeData = (uchar*)lxMemGenZalloc(imgsize*6);

  // fill cubeData with bordercolor for simplicity now
  if (pixelsize == 3)
    for (i = 0; i < texture->width*texture->height; i++)
      LUX_ARRAY4COPY(&cubeData[i*3],texture->border);
  else
    for (i = 0; i < texture->width*texture->height; i++)
      LUX_ARRAY4COPY(&cubeData[i*4],texture->border);

  // -x
  memcpy(&cubeData[imgsize],cubeData,imgsize);
  // +/- y
  memcpy(&cubeData[imgsize*2],cubeData,imgsize);
  memcpy(&cubeData[imgsize*3],cubeData,imgsize);
  // +/- z
  memcpy(&cubeData[imgsize*5],texture->imageData,imgsize);
  memcpy(&cubeData[imgsize*5],cubeData,imgsize);

  Texture_clearData(texture);
  texture->attributes |= TEX_ATTR_CUBE;
  texture->format = TEX_FORMAT_CUBE;
  texture->depth = 1;
  texture->imageData = cubeData;

  return Texture_getAutoUptype(texture);
}

TextureType_t Texture_makeWhite(Texture_t *tex)
{
  tex->width = 1;
  tex->height = 1;
  tex->depth = 1;
  tex->bpp = 32;
  tex->components = 4;
  tex->format = TEX_FORMAT_2D;
  tex->internalfomat = GL_RGBA;
  tex->dataformat = GL_RGBA;
  tex->datatype = GL_UNSIGNED_BYTE;
  tex->sctype = LUX_SCALAR_UINT8;
  tex->notcompress = LUX_TRUE;
  tex->imageData = lxMemGenZalloc(sizeof(uchar)*(tex->bpp/8)*tex->width*tex->height);
  memset(tex->imageData,255,sizeof(uchar)*(tex->bpp/8)*tex->width*tex->height);

  return TEX_RGBA;
}

TextureType_t Texture_makeAlpha( Texture_t *texture)
{
  int   i;
  uchar   *in;
  uchar   *newdata;

  if (!texture->imageData) return TEX_ALPHA;

  // all others are single channel
  if (texture->components != 3 && texture->components != 4)
    return TEX_ALPHA;

  newdata = (uchar*) lxMemGenZalloc(texture->width * texture->height * sizeof(uchar));

  if (texture->components == 3)
    for (i = 0; i < texture->width * texture->height; i++){
      in = &texture->imageData[i*3];
      newdata[i] = ((int)in[0]+(int)in[1]+(int)in[2])/3;
    }
  else
    for (i = 0; i < texture->width * texture->height; i++){
      in = &texture->imageData[i*4];
      newdata[i] = in[3];
    }

  Texture_clearData(texture);
  texture->dataformat = GL_ALPHA;
  texture->bpp = 8;
  texture->components = 1;
  texture->imageData = newdata;
  texture->sctype = LUX_SCALAR_UINT8;

  return TEX_ALPHA;
}

