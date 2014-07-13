// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../main/main.h"

// Published here:
// LUXI_CLASS_TEXTURE


static int PubTexture_load (PState pstate,PubFunction_t *fn, int n)
{
  char *path;
  int mipmap;
  int index;
  int keepdata;
  TextureUpInfo_t *info = (TextureUpInfo_t*)fn->upvalue;

  if ((n<1) || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&path))
    return FunctionPublish_returnError(pstate,"1 string required, optional 1 boolean");

  mipmap = (info->attribute & TEX_ATTR_3D) ? LUX_FALSE : LUX_TRUE;
  keepdata = LUX_FALSE;

  if (n==2)
    FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&mipmap);

  if (n==3)
    FunctionPublish_getNArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&keepdata);

  if ((int)fn->upvalue==TEX_NOUPLOAD){
    mipmap = LUX_FALSE;
    keepdata = LUX_TRUE;
  }

  keepdata = keepdata ? TEX_ATTR_KEEPDATA : 0;
  keepdata |= mipmap ? TEX_ATTR_MIPMAP : 0;
  keepdata |= info->attribute;

  Main_startTask(LUX_TASK_RESOURCE);
  index = ResData_addTexture(path,info->type,keepdata);
  Main_endTask();
  if (index==-1)
    return 0;

  return FunctionPublish_setRet(pstate,1,LUXI_CLASS_TEXTURE,index);
}

static int PubTexture_combine (PState pstate,PubFunction_t *fn, int n)
{
  static char *names[64];
  int mipmap;
  int keepdata;
  int index;
  uint i;
  PubArray_t stack;
  TextureUpInfo_t *info = (TextureUpInfo_t*)fn->upvalue;

  if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_ARRAY_STRING,(void*)&stack))
    return FunctionPublish_returnError(pstate,"1 luatable required optional 2 boolean");

  mipmap = LUX_TRUE;
  keepdata = LUX_FALSE;

  FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_BOOLEAN,(void*)&mipmap,LUXI_CLASS_BOOLEAN,(void*)&keepdata);
  keepdata = keepdata ? TEX_ATTR_KEEPDATA : 0;
  keepdata |= mipmap ? TEX_ATTR_MIPMAP : 0;
  keepdata |= info->attribute;

  stack.length = LUX_MIN(stack.length,64);
  FunctionPublish_fillArray(pstate,0,LUXI_CLASS_ARRAY_STRING,&stack,names);

  index = 0;
  for (i = 0; i < stack.length; i++){
    if (!stack.data.chars[i])
      break;
    index++;
  }

  if (keepdata & TEX_ATTR_CUBE && index !=6)
    return FunctionPublish_returnError(pstate,"cubemaps require 6 strings");

  Main_startTask(LUX_TASK_RESOURCE);
  index = ResData_addTextureCombine(names,index,info->type,LUX_FALSE,keepdata);
  Main_endTask();
  if (index==-1)
    return 0;

  return FunctionPublish_setRet(pstate,1,LUXI_CLASS_TEXTURE,index);
}

static int PubTexture_pack (PState pstate,PubFunction_t *fn, int n)
{
  char *names[4];
  int mipmap;
  int keepdata;
  int index;

  if (n < 3 || 3>(index=FunctionPublish_getArg(pstate,4,LUXI_CLASS_STRING,&names[0],LUXI_CLASS_STRING,&names[1],LUXI_CLASS_STRING,&names[2],LUXI_CLASS_STRING,&names[3])))
    return FunctionPublish_returnError(pstate,"3-4 strings required optional 2 boolean");


  mipmap = LUX_TRUE;
  keepdata = LUX_FALSE;

  FunctionPublish_getArgOffset(pstate,index,2,LUXI_CLASS_BOOLEAN,(void*)&mipmap,LUXI_CLASS_BOOLEAN,(void*)&keepdata);
  keepdata = keepdata ? TEX_ATTR_KEEPDATA : 0;
  keepdata |= mipmap ? TEX_ATTR_MIPMAP : 0;


  Main_startTask(LUX_TASK_RESOURCE);
  index = ResData_addTextureCombine(names,index,(int)fn->upvalue,LUX_TRUE,keepdata);
  Main_endTask();
  if (index==-1)
    return 0;

  return FunctionPublish_setRet(pstate,1,LUXI_CLASS_TEXTURE,index);
}

static int PubTexture_sample(PState pstate,PubFunction_t *fn, int n)
{
  int texRID;
  int coordsi[3];
  int colorint[4];
  lxVector3 coords;
  lxVector4 color;
  uchar colorb[4];
  Texture_t *tex;
  int clamped;

  coords[2] = 0.0f;

  if (n < 3 || FunctionPublish_getArg(pstate,4,LUXI_CLASS_TEXTURE,(void*)&texRID,FNPUB_TOVECTOR3(coords))<3 || !(tex=ResData_getTexture(texRID)) || !tex->imageData || tex->storedcompressed)
    return FunctionPublish_returnError(pstate,"1 uncompressed texture with data and 2 numbers required");


  if (!fn->upvalue){
    clamped = LUX_FALSE;


    if (n == 4 && FunctionPublish_getNArgType(pstate,3) == LUXI_CLASS_BOOLEAN){
      clamped = (int)coords[2];
      coords[2] = 0.0f;
    }
    else if (n > 4)
      FunctionPublish_getNArg(pstate,4,LUXI_CLASS_BOOLEAN,(void*)&clamped);

    Texture_sample(ResData_getTexture(texRID),coords,clamped,color);
    return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(color));
  }
  else{
    LUX_ARRAY3COPY(coordsi,coords);
    if (n >= 7){
      if ((int)fn->upvalue > 1){
        if (FunctionPublish_getArgOffset(pstate,(n==8) + 3,4,FNPUB_TOVECTOR4INT(colorint))<4 )
          return FunctionPublish_returnError(pstate,"1 keepdata texture 2 numbers 4 int required");
        LUX_ARRAY4COPY(colorb,colorint);
        Texture_setPixelUB(tex,coordsi[0],coordsi[1],coordsi[2],colorb);
        return 0;
      }
      else{
        if (FunctionPublish_getArgOffset(pstate,(n==8) + 3,4,FNPUB_TOVECTOR4(color))<4 )
          return FunctionPublish_returnError(pstate,"1 keepdata texture 2 numbers 4 floats required");

        Texture_setPixel(tex,coordsi[0],coordsi[1],coordsi[2],color);
        return 0;
      }
    }


    if ((int)fn->upvalue > 1){
      Texture_getPixelUB(tex,coordsi[0],coordsi[1],coordsi[2],LUX_FALSE,colorb);
      LUX_ARRAY4COPY(colorint,colorb);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4INT(colorint));
    }
    else{
      Texture_getPixel(tex,coordsi[0],coordsi[1],coordsi[2],LUX_FALSE,color);
      return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(color));
    }
  }



}


enum TEXATTR_e{
  TA_CLAMP,
  TA_CLEARGL,
  TA_CLEARDATA,
  TA_RELOADDATA,
  TA_GETDATA,
  TA_GETDATABUF,
  TA_SETUSER,
  TA_RESUBMIT,
  TA_RESUBMITBUF,
  TA_FILTER,
  TA_SIZE,
  TA_SAVE,
  TA_CONVGAUSS,
  TA_CONVBOX,
  TA_RESIZE,
  TA_COPY,
  TA_LMRGBSCALE,
  TA_RELOAD,
  TA_SWAP,
  TA_DATAPTR,
  TA_GLID,
  TA_MIPMAP,
  TA_FORMATTYPESTRING,

  TA_VIDBUFFER,

  TA_SCALARTYPE,
  TA_MOUNT,
};

static int PubTexture_attr(PState pstate,PubFunction_t *f, int n)
{
  int texRID;
  int size;
  int read;
  int depth;
  float flt;
  char *str;
  Texture_t *tex;
  ResourceChunk_t *reschunk;

  byte state;
  byte state1;

  if ( n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_TEXTURE,(void*)&texRID))
    return FunctionPublish_returnError(pstate,"1 texture required");

  tex = ResData_getTexture(texRID);

  switch((int)f->upvalue) {
  case TA_FORMATTYPESTRING:
    return FunctionPublish_returnString(pstate,TextureFormat_toString(tex->format));

  case TA_SCALARTYPE:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_SCALARTYPE,(void*)tex->sarr3d.sarr.type);

  case TA_CLAMP:  // clamp
    if (n>1){
      if (n==2){
        if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
          return FunctionPublish_returnError(pstate,"1 texture 1 boolean required");
        Texture_clamp(tex,(state ? TEX_CLAMP_ALL : TEX_CLAMP_NONE));
      }
      else{
        texRID = tex->clamped;
        for (read = 0; read < n-1; read++){
          if (!FunctionPublish_getNArg(pstate,read+1,LUXI_CLASS_BOOLEAN,(void*)&state))
            return FunctionPublish_returnError(pstate,"1 texture 1..3 boolean required");
          if (state)
            texRID |= 1<<read;
          else
            texRID &= ~(1<<read);
        }
        Texture_clamp(tex,texRID);
      }
    }
    else return FunctionPublish_setRet(pstate,3,  LUXI_CLASS_BOOLEAN,(void*)isFlagSet(tex->clamped,TEX_CLAMP_X),
                        LUXI_CLASS_BOOLEAN,(void*)isFlagSet(tex->clamped,TEX_CLAMP_Y),
                        LUXI_CLASS_BOOLEAN,(void*)isFlagSet(tex->clamped,TEX_CLAMP_Z));
    break;
  case TA_CLEARGL:// freegl
    Texture_clearGL(tex);
    break;
  case TA_SIZE:
    return  FunctionPublish_setRet(pstate,7,  LUXI_CLASS_INT,(void*)tex->width,
                      LUXI_CLASS_INT,(void*)tex->height,
                      LUXI_CLASS_INT,(void*)tex->depth,
                      LUXI_CLASS_INT,(void*)tex->bpp,
                      LUXI_CLASS_INT,(void*)tex->origwidth,
                      LUXI_CLASS_INT,(void*)tex->origheight,
                      LUXI_CLASS_INT,(void*)tex->components);
    break;
  case TA_CLEARDATA:
    Texture_clearData(tex);
    break;
  case TA_GETDATABUF:
    {
      Reference ref;
      HWBufferObject_t  *vbo;
      int offset = 0;

      if (n > 0 && 0<FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_VIDBUFFER,(void*)&ref,FNPUB_TINT(offset)) && Reference_get(ref,vbo) && offset <= vbo->buffer.size && offset >= 0)
      {
        booln succ = LUX_FALSE;
        byte* ptr = NULL;
        ptr += offset;
        vidBindBufferPack(&vbo->buffer);

        succ = Texture_getGL(tex,NULL,ptr,(vbo->buffer.size-offset));

        vidBindBufferPack(NULL);
        return FunctionPublish_returnBool(pstate,succ);
      }
      else{
        return FunctionPublish_returnError(pstate,"1 vidbuffer [1 int] required.");
      }
    }
  case TA_GETDATA:
    {
      Reference ref;
      PScalarArray3D_t *psarray = NULL;
      booln succ;

      if (n == 2 && FunctionPublish_getNArg(pstate,1,LUXI_CLASS_SCALARARRAY,(void*)&ref) && Reference_get(ref,psarray))
      {
        lxScalarArray_t sarr = psarray->sarr3d.sarr;
        sarr.count = psarray->sizecount;
        sarr.data.tvoid = (void*)&psarray->origdata[psarray->dataoffset];

        succ = Texture_getGL(tex,&sarr,NULL,0);
      }
      else{
        succ = Texture_getGL(tex,NULL,NULL,LUX_FALSE);
        tex->attributes |= TEX_ATTR_KEEPDATA;
      }

      return FunctionPublish_returnBool(pstate,succ);
    }

    break;
  case TA_RELOADDATA:
    if (!tex->imageData)
      return FunctionPublish_returnError(pstate,"texture with keepdata required");
    Texture_initGL(tex,LUX_TRUE);
    break;
  case TA_RESIZE:
    depth = tex->depth;
    if (2>FunctionPublish_getArgOffset(pstate,1,tex->format == TEX_FORMAT_3D || tex->format == TEX_FORMAT_2D_ARRAY ? 3 : 2,LUXI_CLASS_INT,(void*)&size,LUXI_CLASS_INT,(void*)&read,LUXI_CLASS_INT,(void*)&depth) ||
      read < 0 || size < 0 || depth < 0)
      return FunctionPublish_returnError(pstate,"2/3 positive int required");
    Texture_resize(tex,size,read,depth);
    break;
  case TA_COPY:
    {
      int texsrc;
      Texture_t *stex;
      int dst[4];
      int src[4];
      int sz[3];

      if (!GLEW_NV_copy_image)
        return FunctionPublish_returnError(pstate,"driver/hw not capable for texcopy");

      if (12>FunctionPublish_getArgOffset(pstate,1,12,
        FNPUB_TOVECTOR4INT(dst),LUXI_CLASS_TEXTURE,(void*)&texsrc,FNPUB_TOVECTOR4INT(src),FNPUB_TOVECTOR3INT(sz))
        || !(stex=ResData_getTexture(texsrc))
        )
      {
        return FunctionPublish_returnError(pstate,"4 int 1 texture 4 int 3 int required");
      }

      glCopyImageSubDataNV(
        stex->texID,stex->target,
        LUX_ARRAY4UNPACK(src),
        tex->texID,tex->target,
        LUX_ARRAY4UNPACK(dst),
        LUX_ARRAY3UNPACK(sz)
        );
      return 0;
    }

  case TA_SETUSER:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&str))
      return FunctionPublish_returnError(pstate,"1 texture 1 string required");
    Main_startTask(LUX_TASK_RESOURCE);
    ResData_addUserTexture(texRID,str);
    tex->isregistered = LUX_TRUE;
    Main_endTask();
    break;
  case TA_RESUBMITBUF:
    {
      Reference ref;
      HWBufferObject_t* hwbuf;
      lxScalarType_t    type = LUX_SCALAR_FLOAT32;
      int componentsize = 4;
      int offset = 0;
      int pos[3];
      int size[3];
      int mip;


      if (7>FunctionPublish_getArgOffset(pstate,1,11,
        FNPUB_TOVECTOR3INT(pos),
        FNPUB_TOVECTOR3INT(size),
        LUXI_CLASS_VIDBUFFER,(void*)&ref,
        FNPUB_TINT(offset),
        LUXI_CLASS_SCALARTYPE,(void*)&type,
        FNPUB_TINT(componentsize),
        FNPUB_TINT(mip)
        ) ||
        !Reference_get(ref,hwbuf) ||
        offset < 0 || offset >= hwbuf->buffer.size ||
        ((hwbuf->buffer.size-offset) < size[0]*size[1]*size[2]*componentsize*lxScalarType_getSize(type)))
      {
        return FunctionPublish_returnError(pstate,
        "6 int 1 vidbuffer [1 int] [1 scalartype] [1 int] [1 int] within ranges required");
      }

      componentsize = LUX_CLAMP(componentsize,1,4);
      vidBindBufferUnpack(&hwbuf->buffer);
      Texture_resubmitGL(tex,mip,LUX_ARRAY3UNPACK(pos),LUX_ARRAY3UNPACK(size),(void*)offset,ScalarType_toGL(type),componentsize,LUX_TRUE);

    }
  case TA_RESUBMIT:
    {
    Reference ref;
    StaticArray_t *starray = NULL;
    PScalarArray3D_t *psarray = NULL;
    GLenum type = 0;
    uint componentsize = 0;
    void  *data = NULL;
    uint  elemcount = 0;
    booln full = LUX_FALSE;

    if (FunctionPublish_getNArg(pstate,n-1,LUXI_CLASS_STATICARRAY,(void*)&ref) && Reference_get(ref,starray)){
      switch(starray->type){
      case LUXI_CLASS_FLOATARRAY: type = GL_FLOAT; componentsize = 4; break;
      case LUXI_CLASS_INTARRAY: type = GL_INT; componentsize = 4; break;
      default:
        break;
      }

      data = starray->data;
      elemcount = starray->count;
    }
    else if (FunctionPublish_getNArg(pstate,n-1,LUXI_CLASS_SCALARARRAY,(void*)&ref) && Reference_get(ref,psarray) &&
      psarray->sarr3d.sarr.stride == psarray->sarr3d.sarr.vectordim)
    {
      type = ScalarType_toGL(psarray->sarr3d.sarr.type);
      componentsize = lxScalarType_getSize(psarray->sarr3d.sarr.type);

      data = (void*)&psarray->origdata[psarray->dataoffset];
      elemcount = psarray->sarr3d.size[0] * psarray->sarr3d.size[1] * psarray->sarr3d.size[2] * psarray->sarr3d.sarr.vectordim;
    }

    if (!tex->imageData && !data)
      return FunctionPublish_returnError(pstate,"1 texture with data or dataarray required");

    read = 0;
    full = elemcount == tex->width*tex->height*tex->depth*tex->components;

    if (n > 3){
      int x,y,z,w,h,d = 0;

      if (n < 7 || 6>FunctionPublish_getArgOffset(pstate,1,7,LUXI_CLASS_INT,(void*)&x,LUXI_CLASS_INT,(void*)&y,LUXI_CLASS_INT,(void*)&z,LUXI_CLASS_INT,(void*)&w,LUXI_CLASS_INT,(void*)&h,LUXI_CLASS_INT,(void*)&d)
        || w < 1 || h < 1 || d < 1
        )
        return FunctionPublish_returnError(pstate,"1 texture 6 int [1 static array] required, or update area out of bounds");

      if (data && (!full && elemcount < (uint)(w*h*d))){
        return FunctionPublish_returnError(pstate,"array has too few elements for fullsize update.");
      }
      vidBindBufferUnpack(NULL);
      Texture_resubmitGL(tex,0,x,y,z,w,h,d,data,type,componentsize,!full);
      return 0;
    }
    else if (n < 4 && (tex->format==TEX_FORMAT_CUBE && !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&read)))
      return FunctionPublish_returnError(pstate,"1 texture [1 int  only for cubemaps] required");

    if (data && !full){
      return FunctionPublish_returnError(pstate,"elementcount");
    }

    vidBindBufferUnpack(NULL);
    Texture_resubmitGL(tex,0,-1,-1,read,-1,-1,-1,data,type,componentsize,LUX_FALSE);
    }
    break;
  case TA_FILTER:
    if (n == 1) return FunctionPublish_setRet(pstate,2,
      LUXI_CLASS_BOOLEAN,(void*)(!isFlagSet(tex->notfiltered,TEX_FILTER_MIN)),
      LUXI_CLASS_BOOLEAN,(void*)(!isFlagSet(tex->notfiltered,TEX_FILTER_MAG)));
    else if ((read=FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_BOOLEAN,(void*)&state1,LUXI_CLASS_BOOLEAN,(void*)&state))<1)
      return FunctionPublish_returnError(pstate,"1 texture 1 boolean required");
    state = (read > 1) ? state : state1;
    Texture_filter(tex,state1,state,LUX_FALSE);
    break;
  case TA_SAVE:
    texRID = 0;
    size = 85;

    if (n < 2 || FunctionPublish_getArgOffset(pstate,1,3,LUXI_CLASS_STRING,(void*)&str,LUXI_CLASS_INT,(void*)&size,LUXI_CLASS_INT,(void*)&texRID)<1
      || size < 0 || size > 100 || texRID < 0 || texRID > 5)
      return FunctionPublish_returnError(pstate,"1 texture 1 string [1 int 1-100] [1 int 0-5] required");

    if (!Texture_saveToFile(tex,str,size,texRID))
      return FunctionPublish_returnError(pstate,"unkown file extension for save, must be .jpg or .tga (case sensitive)");
    break;
  case TA_LMRGBSCALE:
    {
      float scales[3] = {1.0f,2.0f,4.0f};

      if (n == 1) return FunctionPublish_returnFloat(pstate,scales[tex->lmrgbscale]);
      else if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(flt)) ||
        (flt != 1.0f && flt != 2.0f && flt != 4.0f))
        return FunctionPublish_returnError(pstate,"1 texture 1 float (1,2 or 4) required");
      tex->lmrgbscale = flt == 1.0f ? 0 : (flt == 2.0f ? 1 : 2);
      tex->lmtexblend = VID_TEXBLEND_MOD_PREV + tex->lmrgbscale;
    }

    break;
  case TA_RELOAD:
    str = tex->resinfo.name;
    state = LUX_FALSE;
    if (n > 1 && 1>FunctionPublish_getArgOffset(pstate,1,2,LUXI_CLASS_STRING,(void*)&str,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 texture 1 string [1 boolean] required");

    if (state){
      Texture_forceSize(tex->width,tex->height);
    }
    reschunk = ResData_getChunkActive();

    ResData_setChunkFromPtr(&tex->resinfo);

    Texture_clearData(tex);
    ResData_overwriteRID(tex->resinfo.resRID);
    ResData_forceLoad(RESOURCE_TEXTURE);
    ResData_addTexture(str,tex->type,tex->attributes);
    ResData_forceLoad(RESOURCE_NONE);

    ResourceChunk_activate(reschunk);
    break;
  case TA_SWAP:
    if (n < 2 || !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_TEXTURE,(void*)&texRID))
      return FunctionPublish_returnError(pstate,"2 textures required");
    {
      Texture_t swap;
      // swap all but not the resourceinfo which is at beginning
      memcpy(
        ((char*)&swap)+sizeof(ResourceInfo_t),
        ((char*)ResData_getTexture(texRID))+sizeof(ResourceInfo_t),
            sizeof(Texture_t)-sizeof(ResourceInfo_t));
      memcpy(
        ((char*)ResData_getTexture(texRID))+sizeof(ResourceInfo_t),
        ((char*)tex)+sizeof(ResourceInfo_t),
            sizeof(Texture_t)-sizeof(ResourceInfo_t));
      memcpy(
        ((char*)tex)+sizeof(ResourceInfo_t),
        ((char*)&swap)+sizeof(ResourceInfo_t),
            sizeof(Texture_t)-sizeof(ResourceInfo_t));

    }
    break;
  case TA_DATAPTR:
    str = (char*) tex->imageData;
    // size * 6 for cubemaps
    size = (Texture_getSizes(tex,NULL,LUX_FALSE,NULL) * (((tex->format == TEX_FORMAT_CUBE)*5) +1));
    return FunctionPublish_setRet(pstate,3,LUXI_CLASS_POINTER,(void*)str,LUXI_CLASS_POINTER,(void*)(str+size),LUXI_CLASS_POINTER,&tex->sarr3d);
  case TA_GLID:
    return FunctionPublish_setRet(pstate,1,LUXI_CLASS_POINTER,(void*)tex->texID);
  case TA_MIPMAP:
    if (n==1) return FunctionPublish_returnBool(pstate,tex->mipmapfiltered);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    Texture_MipMapping(tex,state);
    break;
  case TA_VIDBUFFER:
    if (tex->format == TEX_FORMAT_BUFFER)
    {
      lxRvidbuffer rvbuf;
      HWBufferObject_t* vbuf = NULL;
      if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_VIDBUFFER,(void*)&rvbuf) &&
        !Reference_get(rvbuf,vbuf)){
        return FunctionPublish_returnError(pstate,"1 vidbuffer required");
      }
      vidBindTexture(texRID);
      glTexBufferARB(GL_TEXTURE_BUFFER_ARB,tex->internalfomat,vbuf ? vbuf->buffer.glID : 0);

      vidBindTexture(g_VID.gensetup.whiteRID);
      return 0;
    }
    else{
      return FunctionPublish_returnError(pstate,"1 texbuffer texture required");
    }
  case TA_MOUNT:
    {
      int cubeside = 0;
      byte *data = tex->imageData;
      PScalarArray3D_t *psa;

      if (!data)
        return 0;

      if (tex->format == TEX_FORMAT_CUBE && FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&cubeside)){
        int mipoffsets[TEX_MAXMIPMAPS];
        size_t size = Texture_getSizes(tex,mipoffsets,LUX_FALSE,NULL);

        data += cubeside*size;
      }

      psa = PScalarArray3D_newMounted(tex->sctype,tex->width * tex->height * tex->depth,
        tex->components,tex->components,data,NULL);
      LUX_ARRAY3COPY(psa->sarr3d.size,tex->sarr3d.size);
      return PScalarArray3D_return(pstate,psa);
    }
    break;
  default:
    return 0;
  }
  return 0;
}

static int PubTexture_name (PState pstate,PubFunction_t *fn, int n)
{
  int index;
  Texture_t *tex;

  FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_TEXTURE,index);

  tex = ResData_getTexture(index);
  if (tex)
    return FunctionPublish_returnString(pstate,tex->resinfo.name);
  else return 0;
}

static int PubTexture_same(PState pstate,PubFunction_t *fn, int n)
{
  char texname[256] = {0};
  char* name;
  int index;
  Texture_t *tex;
  int w,h,d,notfiltered,windowsized,type,resid,mipmap,datatype;
  int attr = 0;

  FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_STRING,name);
  FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_TEXTURE,index);

  tex = ResData_getTexture(index);
  if (!tex)
    return FunctionPublish_returnError(pstate,"texture invalid");

  attr = tex->attributes;
  w = tex->width;
  h = tex->height;
  d = tex->depth;
  notfiltered = tex->notfiltered;
  windowsized = tex->windowsized;
  type = tex->type;
  mipmap = tex->mipmapfiltered;
  datatype = TextureDataType_fromFlag(tex->attributes);

  sprintf(texname,"%s%s)%d,%d,%d,%d,%d,%d,%d,%d",TEX_USERSTART,name,w,h,d,type,windowsized,notfiltered,mipmap,datatype);

  Main_startTask(LUX_TASK_RESOURCE);
  resid = ResData_addTexture(texname,TEX_BY_NAMESTRING,attr);
  Main_endTask();
  if (resid >= 0)
    return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)resid);
  else
    return 0;
}

enum TEXNEW_e{
  TN_2D,
  TN_2DRECT,
  TN_3D,
  TN_CUBE,
  TN_WINDOW,
  TN_1D_ARRAY,
  TN_2D_ARRAY,
  TN_BUFFER,
};

static int PubTexture_new(PState pstate,PubFunction_t *fn, int n)
{
  char texname[256] = {0};

  char *name;
  int w,h,d,notfiltered,windowsized,type,resid,keep,mipmap,datatype;
  int attr = 0;

  // string is formated: width,height,depth,type,winsized,notfiltered,mipmap

  d = 1;
  keep = LUX_FALSE;
  notfiltered = LUX_FALSE;
  mipmap = LUX_FALSE;
  datatype = TEX_DATA_FIXED8;
  switch((int)fn->upvalue) {
  case TN_1D_ARRAY:
    if (!g_VID.capTexArray)
      FunctionPublish_returnError(pstate,"no support for array textures");
    attr |= TEX_ATTR_ARRAY;
  case TN_2D:
    if (n < 4 ||  FunctionPublish_getArg(pstate,8,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_INT,(void*)&w,LUXI_CLASS_INT,(void*)&h,LUXI_CLASS_TEXTYPE,(void*)&type,LUXI_CLASS_BOOLEAN,(void*)&keep,LUXI_CLASS_BOOLEAN,(void*)&notfiltered,LUXI_CLASS_BOOLEAN,(void*)&mipmap,LUXI_CLASS_TEXDATATYPE,(void*)&datatype)<4)
      return FunctionPublish_returnError(pstate,"1 string 2 int 1 textype [3 boolean 1 texdatatype] required");
    if (type >= TEX_DEPTH && type <= TEX_DEPTH32 && (!GLEW_ARB_depth_texture || mipmap))
      return FunctionPublish_returnError(pstate,"no depth texture support / no mipmaps for depth texture");
    windowsized = 0;
    break;
  case TN_2DRECT:
    if (!GLEW_NV_texture_rectangle) return FunctionPublish_returnError(pstate,"no rectangle textures supported.");
    if (n < 4 ||  FunctionPublish_getArg(pstate,7,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_INT,(void*)&w,LUXI_CLASS_INT,(void*)&h,LUXI_CLASS_TEXTYPE,(void*)&type,LUXI_CLASS_BOOLEAN,(void*)&keep,LUXI_CLASS_BOOLEAN,(void*)&notfiltered,LUXI_CLASS_TEXDATATYPE,(void*)&datatype)<4)
      return FunctionPublish_returnError(pstate,"1 string 2 int 1 textype [2 boolean 1 texdatatype] required");
    if (type >= TEX_DEPTH && type <= TEX_DEPTH32 && (!GLEW_ARB_depth_texture || mipmap))
      return FunctionPublish_returnError(pstate,"no depth texture support / no mipmaps for depth texture");
    windowsized = 0;
    attr |= TEX_ATTR_RECT;
    break;
  case TN_CUBE:
    if (n < 3 ||  FunctionPublish_getArg(pstate,7,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_INT,(void*)&w,LUXI_CLASS_TEXTYPE,(void*)&type,LUXI_CLASS_BOOLEAN,(void*)&keep,LUXI_CLASS_BOOLEAN,(void*)&notfiltered,LUXI_CLASS_BOOLEAN,(void*)&mipmap,LUXI_CLASS_TEXDATATYPE,(void*)&datatype)<3)
      return FunctionPublish_returnError(pstate,"1 string 1 int 1 textype [3 boolean 1 texdatatype] required");
    if (!GLEW_ARB_texture_cube_map)
      return FunctionPublish_returnError(pstate,"no cubemap support");
    d = 1;
    h = w;
    attr |= TEX_ATTR_CUBE;
    windowsized = 0;
    break;
  case TN_2D_ARRAY:
    if (!g_VID.capTexArray)
      FunctionPublish_returnError(pstate,"no support for array textures");
    attr |= TEX_ATTR_ARRAY;
  case TN_3D:
    if (n < 5 ||  FunctionPublish_getArg(pstate,9,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_INT,(void*)&w,LUXI_CLASS_INT,(void*)&h,LUXI_CLASS_INT,(void*)&d,LUXI_CLASS_TEXTYPE,(void*)&type,LUXI_CLASS_BOOLEAN,(void*)&keep,LUXI_CLASS_BOOLEAN,(void*)&notfiltered,LUXI_CLASS_BOOLEAN,(void*)&mipmap,LUXI_CLASS_TEXDATATYPE,(void*)&datatype)<5)
      return FunctionPublish_returnError(pstate,"1 string 3 int 1 textype [3 boolean 1 texdatatype] required");
    windowsized = 0;
    if (w > g_VID.capTex3DSize || h > g_VID.capTex3DSize || d > g_VID.capTex3DSize)
      return FunctionPublish_returnError(pstate,"texture too big");
    if (!GLEW_ARB_texture_non_power_of_two && (lxNearestPowerOf2(w)!=w || lxNearestPowerOf2(h)!=h || lxNearestPowerOf2(d)!=d))
      return FunctionPublish_returnError(pstate,"System not capable for np2 textures.");

    break;
  case TN_WINDOW:
    type = TEX_RGBA;
    notfiltered = LUX_TRUE;
    if (n < 1 || 1>FunctionPublish_getArg(pstate,4,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_TEXTYPE,(void*)&type,LUXI_CLASS_BOOLEAN,(void*)&notfiltered,LUXI_CLASS_TEXDATATYPE,(void*)&datatype))
      return FunctionPublish_returnError(pstate,"1 string required");
    w=h=d=windowsized = 1;
    break;
  case TN_BUFFER:
    if (!g_VID.capTexBuffer)
      FunctionPublish_returnError(pstate,"no support for buffer textures");

    if (3>FunctionPublish_getArg(pstate,3,LUXI_CLASS_STRING,(void*)&name,
      LUXI_CLASS_TEXTYPE,(void*)&type,LUXI_CLASS_TEXDATATYPE,(void*)&datatype) ||
      (type != TEX_RGBA && type != TEX_LUM && type != TEX_ALPHA && type != TEX_LUMALPHA))
    {
      return FunctionPublish_returnError(pstate,"1 string 1 textype 1 texdatatype required.");
    }
    w=h=d=windowsized = 0;
    attr |= TEX_ATTR_BUFFER;
    break;
  default:
    break;
  }

  if (keep)
    attr |= TEX_ATTR_KEEPDATA;

  /*
  if (!GLEW_SGIS_generate_mipmap && mipmap)
    return FunctionPublish_returnError(pstate,"mipmap only allowed if hardware mipmapping exists");
  */
  sprintf(texname,"%s%s)%d,%d,%d,%d,%d,%d,%d,%d",TEX_USERSTART,name,w,h,d,type,windowsized,notfiltered,mipmap,datatype);

  Main_startTask(LUX_TASK_RESOURCE);
  resid = ResData_addTexture(texname,TEX_BY_NAMESTRING,attr);
  Main_endTask();
  if (resid >= 0)
    return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)resid);
  else
    return 0;
}

void PubClass_Texture_init()
{
  FunctionPublish_initClass(LUXI_CLASS_TEXTURE,"texture","Bitmaps that store color information.<br> There is two kind of textures: \
loaded from file and created in memory. Filebased textures are 'load'ed while pure memory textures are created with 'create'. Loading defaults to mipmap true and keepdata false.\
When a texturename looks like 'USER_TEX(myname)' then luxinia will check if a loaded texture was registered as usertex with 'myname' or if a user created texture with that name exists.",NULL,LUX_FALSE);
  FunctionPublish_setParent(LUXI_CLASS_TEXTURE,LUXI_CLASS_RESOURCE);
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_CLAMP,"clamp",
    "([boolean x,y,z]) : (texture,[boolean all]/[boolean x,y,z]) - sets or gets clamping of the texture. If not set texture will repeat, default is off. By passing one boolean you will toggle all, else you can specify in detail which axis to repeat/clamp.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_CLEARGL,"deletevid",
    "() : (texture) - unloads the texture from video memory, can result into errors when you free a still in use texture.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_CLEARDATA,"deletedata",
    "() : (texture) - deletes the imagedata in system memory and removes keepdata flag. You can no longer change or sample it.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_RELOADDATA,"reloaddata",
    "() : (texture) - deletes old image in videomemory, and uploads a new one with the current imagedata. this is slower than uploaddata but supports all textures.");
  /*
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_CONVBOX,"convfilterboxdata",
    "() : (texture,int kernelsize,[boolean wrap]) - runs convolution boxcar filter on the imagedata. wrap defaults to true");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_CONVGAUSS,"convfiltergaussdata",
    "() : (texture,int kernelsize,[boolean wrap]) - runs convolution gauss filter on the imagedata. wrap defaults to true");
  */
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_SETUSER,"registerusertex",
    "() : (texture,string username) - registers the texture as USER_TEX. If anyone access a texture with the name 'USER_TEX(username)' this texture will be used. The name will be unavailable once texture was deleted.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_RESUBMIT,"uploaddata",
    "() : (texture,[int x,y,z,w,h,d]/[int side],[staticarray/scalararray uploaddata]) - will upload the current data to video memory. Only works on user-created textures with data, or if dataarray is passed. x,y,z are the starting points, w,h,d the dimension of the updated area. If arraycount does not match full texture dimension, then dataarray is assumed to be only the update rectangle. Scalararrays offsets are ignored. Side is for cubemaps (0-5), else cubemaps take Z as side. All numbers passed must be positive");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_RESUBMITBUF,"uploadbuf",
    "() : (texture,int x,y,z,w,h,d, vidbuffer,[int offset],[scalartype], [int offset], [mipmaplevel]) - similar to uploaddata but reads from vidbuffer. Offset must be positive and within size range. Data by default is 4 component float32 and mipmaplevel is 0.");

  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_FILTER,"filter",
    "([boolean min,mag]) : (texture,[boolean both]/[boolean min,mag]) - returns or sets if texture filtering is used for minfication or/and magnification.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_SAVE,"savetofile",
    "() : (texture,string filename,[int quality],[int cubeside]) - saves texture to file. quality must be within 1 and 100 and is used for .jpg (default is 85). cubeside can be 0-5 for cubemap textures (default 0)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_SIZE,"dimension",
    "(int width,height,depth,bpp,origwidth,origheight,components) : (texture) - will return infos about the texture. It returns the size in memory\
    and the original size. If system.detail() is <2, the loaded textures are resized at half of their size! If \
    you do any calculations on the image depending on its size, you have to take this into account!");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_RESIZE,"resize",
    "() : (texture,int width, int height, [int depth]) - resizing textures. previous data is lost. depth only for 3d textures");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_GETDATA,"downloaddata",
    "(boolean success) : (texture,[scalararray]) - if texture was not loaded with keepdata, we can retrieve imagedata this way. Also allows updating local data from vidmem if keepdata was set on init, but texture content has changed. Note that without keepdata set, you must refetch data everytime major visual changes occur (fullscreen/res toggle). Optionally downloads uncompressed 1st mipmap into given scalararray");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_GETDATABUF,"downloadbuf",
    "(boolean success) : (texture,vidbuffer, [int offset]) - similar to downloaddata but stored into the vidbuffer. Offset (bytes) must be positive and within size of vidbuffer.");

  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_LMRGBSCALE,"lightmapscale",
    "([float]) : (texture,[float]) - returns or sets how much the output is scaled, when this texture is used as lightmap outside of shaders (or when lightmapscale is used in shader stage)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_RELOAD,"reload",
    "() : (texture,[string filename,boolean keepoldsize] - reloads the texture with original (or new filename) with same specifications as on first load. If the texture was bound to an renderfbo, make sure to call renderfbo:resetattach. The internal filename remains to be the original filename. Optionally the texture can be forced to the current texturesize.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_SWAP,"swap",
    "() : (texture,texture) - swaps both texture contents, all images who already referenced one of either, now uses the other data. Names stay as they were.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_DATAPTR,"datapointer",
    "(pointer start, end, s3darray) : (texture) - if the texture has imagedata availabe, you can access the memory directly in other lua dlls. Be aware that you must make sure to not corrupt memory, make sure you are always smaller than the 'end' pointer. Start may be NULL if no image data present.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_GLID,"glid",
    "(pointer) : (texture) - returns OpenGL ID (GLuint) as pointer.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_MIPMAP,"mipmapping",
    "([boolean]) : (texture,[boolean]) - turn on/off mipmapping for the texture. Only works when texture was created with mipmap support.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_FORMATTYPESTRING,"formattype",
    "(string) : (texture) - the format in which the texture is stored as string, e.g. 1d, 2d, 3d, rect, cube, 1darray, 2darray.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_SCALARTYPE,"getscalartype",
    "(scalartype) : (texture) - the format of the local data.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_MOUNT,"mountscalararray",
    "([scalararray]) : (texture, [int cubeside]) - returns a mounted array if data is available.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_VIDBUFFER,"settexbuffer",
    "() : (texture, vidbuffer) - assigns the given vidbuffer to the texture. texture must be of 'buffer' format. Internalformat as defined by texture creation is used. Passing a none vidbuffer disables binding.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_attr,(void*)TA_COPY,"copy",
    "() : (texture, int lvl, x,y,z, texture src, int lvl, x,y,z, dimx,dimy,dimz) - copies content of one texture to another. lvl being the mipmap level. Requires texcopy capability.");

  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_sample,(void*)0,"sample",
    "([float r,g,b,a]):(texture,float u,v,[w],[boolean clamp]) - gets color from data texture, does clamped,bilinear filtering. More details under pixel function.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_sample,(void*)1,"pixel",
    "([float r,g,b,a]):(texture,int x,y,[z],[float r,g,b,a]) - same as sample but works with discrete coordinates and can set pixels. Writing to texture is not visible until uploaddata or reloaddata was called. Z can be 0-5 for cubemap side, and 0-depth for 3d textures. If internal format is BGR/BGRA/ABGR values must be manually reordered, LUMALPHA uses R and G channels. ALPHA/LUM use R channel.)");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_sample,(void*)2,"pixelbyte",
    "([int r,g,b,a]):(texture,int x,y,[z],[int r,g,b,a]) - same as pixel but assumes channels as byte values (0-255).");

  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_same,(void*)NULL,"createsame",
    "(texture):(string name, texture) - creates a new empty texture in memory with same size and format description as the texture passed.");

  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_new,(void*)TN_WINDOW,"createfullwindow",
    "(texture):(string name,[textype],[nofilter],[texdatatype]) - creates a new empty windowsized texture. Should only be used within materials or as rendertargets, else you might need to set texture matrices manually (needed if capability for rectangle textures exists). Automatically registered as USER_TEX. No mipmapping nor compression supported. By default nofilter is true and textype is rgba (internally whatever is closest to window is used). The speciality about windowsized textures is that if you change window resolution they are automatically resized.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_new,(void*)TN_2DRECT,"create2drect",
    "(texture):(string name, int width, int height,textype, [boolean keepdata], [boolean nofilter],[texdatatype]) - creates a new empty rectangle texture in memory. Width and height can be arbitrary. texdatatype is uchar (Unsigned Byte) by default. Automatically registered as USER_TEX. No compression supported.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_new,(void*)TN_2D,"create2d",
    "(texture):(string name, int width, int height,textype, [boolean keepdata], [boolean nofilter],[boolean mipmap],[texdatatype]) - creates a new empty texture in memory. Width and height must be power of two (unless capability for np2 textures exists). texdatatype is uchar (Unsigned Byte) by default. Automatically registered as USER_TEX. No compression supported. Mipmapping will only work when keepdata + reloaddata is used, or when hardware mipmapping exists. If height is 1 it will be a 1D texture and stay like that (ie. resizing will not change this).");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_new,(void*)TN_1D_ARRAY,"create1darray",
    "(texture):(string name, int width, int height,textype, [boolean keepdata], [boolean nofilter],[boolean mipmap],[texdatatype]) - creates a new empty texture in memory. Width must be power of two, height <= capability.texarraylayers. Resizing will not change type.");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_new,(void*)TN_CUBE,"createcube",
    "(texture):(string name, int size, textype, [boolean keepdata], [boolean nofilter],[boolean mipmap],[texdatatype]) - creates a new empty texture in memory. Size must be power of two. Automatically registered as USER_TEX. Same creation, compression and mipmap rules as for create2d");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_new,(void*)TN_3D,"create3d",
    "(texture):(string name, int width, int height, int depth, textype, [boolean keepdata], [boolean nofilter],[boolean mipmap],[texdatatype]) - creates a new empty texture in memory. Sizes must be power of two (unless np2 supported). Automatically registered as USER_TEX. Same creation, compression and mipmap rules as for new2d");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_new,(void*)TN_2D_ARRAY,"create2darray",
    "(texture):(string name, int width, int height, int depth, textype, [boolean keepdata], [boolean nofilter],[boolean mipmap],[texdatatype]) - creates a new empty texture in memory. Width and height must be power of two (unless np2 supported), depth <= capability.texarraylayers. Automatically registered as USER_TEX. Same creation, compression and mipmap rules as for create2d");
  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_new,(void*)TN_BUFFER,"createbuffer",
    "(texture):(string name, textype, texdatatype) - creates a texture that allows binding a vidbuffer as texture. The vidbuffer must be specified with setvidbuffer command. Only alpha,lumalpha,intensity and rgba are allowed as textypes.");

  {
    static TextureUpInfo_t texloader[]= {
      // load
      {TEX_NOUPLOAD,0},

      {TEX_COLOR,0},
      {TEX_ALPHA,0},
      {TEX_LUM,0},

      {TEX_COLOR, TEX_ATTR_3D},
      {TEX_ALPHA, TEX_ATTR_3D},
      {TEX_LUM, TEX_ATTR_3D},

      {TEX_COLOR, TEX_ATTR_CUBE},
      {TEX_ALPHA, TEX_ATTR_CUBE},
      {TEX_LUM, TEX_ATTR_CUBE},

      {TEX_COLOR, TEX_ATTR_PROJ},
      {TEX_COLOR, TEX_ATTR_DOTZ},
      // combine
      {TEX_COLOR,0},
      {TEX_COLOR, TEX_ATTR_CUBE},

      {TEX_COLOR, TEX_ATTR_RECT},
      };

    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[0],"loaddata",
      "(Texture tex):(string file) - loads a texture, but it will not be renderable. (autosets keepdata)");
    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[1],"load",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a color 1d/2d texture (RGB or RGBA depending on file). Dimension depends on file.<br>\
      There are some special commands that you can attach to your load filename. \"nocompress;filename.ext\" will disable compression. 'normalize' will normalize the texture and its mipmaps. 'normal2light' will create a greyscale version of the normalmap using a fixed light direction vector. 'dxt1','dxt3' or 'dxt5' enforce specifc compression (if enabled), otherwise let driver choose compression format. If the filename contains 'lightmap' compression will be disabled, too. Note that compressed-stored textures will not work with the commands.");
    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[14],"loadrect",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a color 2d rect texture (check capability) MipMap is ignored. Dimension depends on file.<br>\
      There are some special commands that you can attach to your load filename. \"nocompress;filename.ext\" will disable compression. 'normalize' will normalize the texture and its mipmaps. 'normal2light' will create a greyscale version of the normalmap using a fixed light direction vector. 'dxt1','dxt3' or 'dxt5' enforce specifc compression (if enabled), otherwise let driver choose compression format. If the filename contains 'lightmap' compression will be disabled, too. Note that compressed-stored textures will not work with the commands.");
    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[2],"loadalpha",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a alpha 1d/2d texture.");
    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[3],"loadlum",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a luminance 1d/2d texture.");

    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[4],"load3d",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a color 3d texture (RGB or RGBA depending on file). 3D textures are either .dds or a 2d image whose slices are stored vertically along image height. So that depth = height/width, height = width");
    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[5],"load3dalpha",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a alpha 3d texture");
    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[6],"load3dlum",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a luminance 3d texture.");

    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[7],"loadcube",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a color cube texture (RGB or RGBA depending on file). Cube texture are either .dds or 2d image whose sides are stored (+x,-x,+y,-y,+z,-z) vertically. So that height = width*6.");
    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[8],"loadcubealpha",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a alpha cube texture");
    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[9],"loadcubelum",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a luminance cube texture.");

    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[10],"loadcubeproj",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a special color cubemap texture, generated from a single 2d image to fix backprojection problems, if nothing is specified we will use mipmapping.");
    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_load,(void*)&texloader[11],"loadcubedotz",
      "(Texture tex):(string file,[boolean mipmap],[boolean keepdata]) - loads a special color cubemap texture, generated from a single 1d image to create a vector.dot.+Z cubemap, cube.size is half of width. -1 is left and +1 is right. if nothing is specified we will use mipmapping.");

    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_combine,(void*)&texloader[12],"combine2d",
      "(Texture tex):(stringstack,[boolean mipmap],[boolean keepdata]) - combines all 2d textures to a single row, ideally number of textures should be power of 2, max 64 textures");
    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_combine,(void*)&texloader[13],"combinecube",
      "(Texture tex):(stringstack,[boolean mipmap],[boolean keepdata]) - combines all 2d textures to a single cubemap, 6 textures are required");

    FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_pack,(void*)TEX_COLOR,"pack2d",
      "(Texture tex):(string filenameR, string filenameG, string filenameB,[string filenameA],[boolean mipmap],[boolean keepdata]) - combines all 2d textures to a single image where each image file becomes one color channel. 3 or 4 filenames required.");
  }

  FunctionPublish_addFunction(LUXI_CLASS_TEXTURE,PubTexture_name,NULL,"name",
    "(string):(texture tex) - returns name of texture");
}
