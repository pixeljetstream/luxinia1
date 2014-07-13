// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h" // needed for GL_RGB and GL_RGBA
#include "../common/common.h"
#include "../fileio/dds.h"
#include <ddslib/ddslib.h>
#include "../fileio/filesystem.h"

int fileLoadDDS(const char * filename,Texture_t *texture, void *unused)
{
  lxFSFile_t *fDDS;
  CDDSImage_t dds;
  CDDSSurface_t surf;
  char *data;
  int i,n,size,sides;
  int mipoffsets[TEX_MAXMIPMAPS];
  int datasize;

  if (!g_VID.capTexDDS){
    lprintf("ERROR ddsload: No DDS capability, %s\n",filename);
    return LUX_FALSE;
  }

  fDDS = FS_open(filename);
  if(fDDS == NULL)
  {
    lprintf("ERROR ddsload: ");
    lnofile(filename);
    return LUX_FALSE;
  }

  data = (char*)lxFS_getContent(fDDS);

  lprintf("Texture: \t%s\n",filename);
  if ((dds = CDDSImage_load((unsigned char*)data,LUX_TRUE)) == NULL)
  {
    lprintf("ERROR ddsload: invalid file format\n");
    FS_close(fDDS);
    return LUX_FALSE;
  }

  texture->target = CDDSImage_getTarget(dds);
  texture->bpp = CDDSImage_getComponents(dds);
  texture->components = texture->bpp;
  texture->dataformat = CDDSImage_getFormat(dds);
  texture->datatype = CDDSImage_getDatatype(dds);
  texture->sctype = ScalarType_fromGL(texture->datatype);



  datasize = (texture->datatype == GL_UNSIGNED_BYTE ? 8 : (texture->datatype == GL_FLOAT ? 32 : 16));
  texture->bpp *= datasize;

  texture->storedcompressed = CDDSImage_isCompressed(dds);
  if (texture->storedcompressed){
    enum32 dataformat = CDDSImage_getFormat(dds);
    switch(dataformat){
      case GL_COMPRESSED_RED_RGTC1_EXT:
      case GL_COMPRESSED_RED_GREEN_RGTC2_EXT:
        texture->attributes |= TEX_ATTR_COMPR_RGTC;

        break;
      case GL_COMPRESSED_SIGNED_RED_RGTC1_EXT:
      case GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT:
        texture->attributes |= TEX_ATTR_COMPR_SIGNED_RGTC;
        texture->sarr3d.sarr.type = LUX_SCALAR_INT8;
        break;
      case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
      case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
        texture->attributes |= TEX_ATTR_COMPR_LATC;
        break;
      case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
      case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
        texture->attributes |= TEX_ATTR_COMPR_LATC;
        texture->sarr3d.sarr.type = LUX_SCALAR_INT8;
        break;
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        texture->attributes |= TEX_ATTR_COMPR_DXT1;
        break;

      case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        texture->attributes |= TEX_ATTR_COMPR_LATC;
        break;

      case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        texture->attributes |= TEX_ATTR_COMPR_LATC;
        break;
    }
  }

  // base surface
  surf = CDDSImage_getSurface(dds,0,0);
  texture->width = CDDSSurface_getWidth(surf);
  texture->height = CDDSSurface_getHeight(surf);
  texture->depth = CDDSSurface_getDepth(surf);
  texture->mipmaps = CDDSImage_getNumMipmaps(dds);
  if (texture->target == GL_TEXTURE_CUBE_MAP){
    texture->format = TEX_FORMAT_CUBE;
    texture->depth = 1;
    texture->attributes |= TEX_ATTR_CUBE;
    sides = 6;
  }else{
    sides = 1;
  }

  size = 0;
  for (i=0; i < texture->mipmaps+1; i++){
    size += CDDSSurface_getSize(CDDSImage_getSurface(dds,i,0));
  }

  // check size/mipmap gen
  if (size != Texture_getSizes(texture,mipoffsets,LUX_FALSE,NULL)){
    lprintf("ERROR ddsload: mipmapsizes wrong\n");
    CDDSImage_free(dds);
    FS_close(fDDS);
    return LUX_FALSE;
  }


  // get data
  texture->imageData = (byte*)lxMemGenZalloc(size*sides);
  for (i=0; i < sides; i++){
    for(n = 0; n < texture->mipmaps+1; n++){
      surf = CDDSImage_getSurface(dds,n,i);
      memcpy(&texture->imageData[mipoffsets[n]+(i*size)],CDDSSurface_getData(surf),CDDSSurface_getSize(surf));
    }
  }


  CDDSImage_free(dds);

  Texture_fixFloat(texture,(size*sides)/(datasize));

  FS_close(fDDS);
  return LUX_TRUE;
}
