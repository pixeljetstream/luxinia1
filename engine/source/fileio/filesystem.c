// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include <ctype.h>
#include <string.h>

#include "filesystem.h"
#include "../main/main.h"
#include "../common/common.h"

#include "f3d.h"
#include "jpg.h"
#include "tga.h"
#include "ma.h"
#include "prt.h"
#include "mtl.h"
#include "shd.h"
#include "dds.h"
#include "png.h"
#include "../resource/sound.h"

// LOCALS
FSdata_t  l_FSdata;

  //must match enums
  // ext must be in lower case
FSFileLoader_t l_FSFileLoaders[]=
{
  {".",   NULL}, // cast to (void*) avoids warning in mingw
  {".tga",  (FSFileLoaderFn*)fileLoadTGA},
  {".jpg",  (FSFileLoaderFn*)fileLoadJPG},
  {".dds",  (FSFileLoaderFn*)fileLoadDDS},
  {".png",  (FSFileLoaderFn*)fileLoadPNG},
  {".ma",   (FSFileLoaderFn*)fileLoadMA},
  {".f3d",  (FSFileLoaderFn*)fileLoadF3D},
  {".wav",  (FSFileLoaderFn*)Sound_loadFile},
  {".prt",  (FSFileLoaderFn*)fileLoadPRT},
  {".shd",  (FSFileLoaderFn*)fileLoadSHD},
  {".mtl",  (FSFileLoaderFn*)fileLoadMTL},
};

//////////////////////////////////////////////////////////////////////////
// FS


void FS_init()
{
  l_FSdata.filenameFunc = FS_internal_setProjectFilename;
  l_FSdata.openFunc = FS_internal_open;
  l_FSdata.existsFunc = FS_internal_fileExists;
}

void FS_deinit()
{

}

void  FS_setExternalFileExists(FSexternalFileExists_fn* func)
{
  l_FSdata.existsFunc = (func) ? func : FS_internal_fileExists;
}

void  FS_setExternalOpen(FSexternalOpen_fn* func)
{
  l_FSdata.openFunc = (func) ? func : FS_internal_open;
}


void  FS_setExternalProjectFilename(FSexternalProjectFilename_fn* func)
{
  l_FSdata.filenameFunc = (func) ? func : FS_internal_setProjectFilename;
}

int  FS_processLoader(FSFileLoaderType_t type, const char *filename, void *outData0, void *outData1)
{
  if (l_FSFileLoaders[type].loader)
    return l_FSFileLoaders[type].loader(filename,outData0,outData1);
  else{
    lprintf("WARNING fileloader unknown: %s\n",filename);
    return LUX_FALSE;
  }
}

int FS_setProjectFilename(char *filename,const char *name, const char *dir, const char *altdir,FSFileLoaderType_t *outType)
{
  return l_FSdata.filenameFunc(filename,name,dir,altdir,outType);
}

FSFileLoaderType_t FS_getLoaderType(const char *filename)
{
  FSFileLoaderType_t type;
  FSFileLoader_t *loader;
  const uchar *lastext;
  const uchar *lastname;
  uchar cmp;

  if (!filename || !strrchr(filename,'.'))
    return FILELOADER_NONE;



  loader = l_FSFileLoaders;
  for (type = FILELOADER_NONE; type < FILELOADERS; type++,loader++){
    lastext = loader->ext;
    lastname = filename;
    while (*(lastext++));
    while (*(lastname++));
    lastext--;
    lastname--;

    while (lastext > loader->ext && lastname > filename){
      // lower case
      cmp = *lastname;
      cmp = (cmp >= (uchar)'A' && cmp <= (uchar)'Z') ? cmp+32 : cmp;
      if (*lastext != cmp)
        break;
      lastext--;
      lastname--;
    }
    if (*lastext == '.' && *lastname == '.')
      return type;
  }

  return FILELOADER_NONE;
}

int FS_internal_setProjectFilename(char *filename, const char *name, const char *dir, const char *altdir,FSFileLoaderType_t *outType)
{
  char *noproj = NULL;
  *outType = FS_getLoaderType(name);
  // try as is
  if (FS_fileExists(name)){
    strcpy(filename,name);
    return LUX_TRUE;
  }

  if (altdir && *altdir){
    strcpy(filename,altdir);
    strcat(filename,name);
    if (FS_fileExists(filename))
      return LUX_TRUE;
  }

  // try projectpath
  if (!lxStrBeginsWith(name,g_projectpath) && !lxStrBeginsWith(name,LUX_BASEPATH)){
    noproj = strcpy(filename,g_projectpath);
  }

  strcat(filename,name);
  if (FS_fileExists(filename))
    return LUX_TRUE;

  // try defaultpath
  if (noproj)
    strcpy(filename,g_projectpath);
  if (strstr(name,dir)==NULL){
    strcat(filename,dir);
    strcat(filename,name);
    if (FS_fileExists(filename))
      return LUX_TRUE;
  }


  // repeat same for basepath
  strcpy(filename,LUX_BASEPATH);

  strcat(filename,name);
  if (FS_fileExists(filename))
    return LUX_TRUE;

  // try defaultpath
  strcpy(filename,LUX_BASEPATH);
  if (strstr(name,dir)==NULL){
    strcat(filename,dir);
    strcat(filename,name);
    return FS_fileExists(filename);
  }
  else
    return LUX_FALSE;
}

//////////////////////////////////////////////////////////////////////////
// FSFile

unsigned long fsize(FILE *file)
{
  long current, length;

  // backup the curren file offset
  current = ftell(file);
  // seek to end of file
  fseek(file, 0, SEEK_END);
  // get offset
  length = ftell(file);
  // go back to old location
  fseek(file, current, SEEK_SET);
  return length;
}

int FS_fileExists(const char *filename)
{
  return l_FSdata.existsFunc(filename);
}
int FS_internal_fileExists(const char *filename)
{
  FILE *file;
  file=fopen(filename, "rb");
  if(!file){
    return LUX_FALSE;
  }
  fclose(file);
  return LUX_TRUE;
}

lxFSFile_t *FS_open(const char *filename){
  lxFSFile_t *file;
  void  *data;
  int   size;
  int   freecontent;


  data = l_FSdata.openFunc(filename,&size,&freecontent);

  if (!data)
    return NULL;

  file = lxMemGenZalloc(sizeof(lxFSFile_t));
  file->contents = data;
  file->size = size;
  file->freecontent = freecontent;

  return file;
}

void *FS_internal_open(const char *filename, long *size, int *freecontent)
{

  FILE *normalfile;
  long filesize;
  void *data;

  // look if the file is available
  if(!FS_fileExists(filename))
    return NULL;



  normalfile = fopen(filename, "rb");
  filesize = fsize(normalfile);
  filesize++;

  data = MEMORY_GLOBAL_MALLOC(filesize);
  fread(data, filesize-1, 1, normalfile);
  ((char*)data)[filesize-1]=0;

  fclose(normalfile);

  *freecontent = LUX_TRUE;
  *size = filesize;

  return data;
}

void FS_close(lxFSFile_t *file)
{
  if(!file) return;

  if (file->freecontent)
    MEMORY_GLOBAL_FREE(file->contents);

  lxMemGenFree(file,sizeof(lxFSFile_t));
}



//////////////////////////////////////////////////////////////////////////
// Helpers

int cmpfilenamesi(register const char *filea, register const char *fileb)
{
  if (filea==NULL || fileb==NULL) return -1;
  while(*filea != 0 && *fileb != 0)
  {
    if(*filea != *fileb)
    {
      if(!(*filea >= 'A'  && *filea <= 'Z' && (*fileb == *filea + 32)) && // big vs. small
        !(*filea >= 'a'  && *filea <= 'z' && (*fileb == *filea - 32)) && // small vs.big
        !((*filea == '/'  && *fileb == '\\') || (*filea == '\\' && *fileb == '/'))) // slashes
        return (*filea - *fileb);
    }

    filea++;
    fileb++;
  }

  return (*filea - *fileb);
}

//////////////////////////////////////////////////////////////////////////
// CRC32
#ifdef CHECK_CRC32

typedef struct CRC32entry_s
{
  char *name;
  unsigned long crc32;
}CRC32entry_t;

#include "filesystemcrc32.h"


#define POLYNOMIAL      0x04C11DB7
#define INITIAL_REMAINDER 0xFFFFFFFF
#define FINAL_XOR_VALUE   0xFFFFFFFF

#define REFLECT_DATA(X)     ((unsigned char) reflect((X), 8))
#define REFLECT_REMAINDER(X)  ((unsigned long) reflect((X), WIDTH))

#define WIDTH    (8 * sizeof(unsigned long))
#define TOPBIT   (1 << (WIDTH - 1))

static unsigned long reflect(unsigned long data, unsigned char nBits)
{
  unsigned long  reflection = 0x00000000;
  unsigned char  bit;

  for (bit = 0; bit < nBits; ++bit)
  {
    if (data & 0x01)
    {
      reflection |= (1 << ((nBits - 1) - bit));
    }

    data = (data >> 1);
  }

  return (reflection);
}

unsigned long calculatecrc32(unsigned char const message[], int nBytes)
{
  unsigned long remainder = INITIAL_REMAINDER;
  int       byte;
  unsigned char bit;

  for (byte = 0; byte < nBytes; ++byte)
  {
    remainder ^= (REFLECT_DATA(message[byte]) << (WIDTH - 8));

    for (bit = 8; bit > 0; --bit)
    {
      if (remainder & TOPBIT)
      {
        remainder = (remainder << 1) ^ POLYNOMIAL;
      }
      else
      {
        remainder = (remainder << 1);
      }
    }
  }

  return (REFLECT_REMAINDER(remainder) ^ FINAL_XOR_VALUE);
}

void FS_crcfailed()
{
  bprintf(
"\nSome files are not in their original state or could not be found.\
 Make sure you run luxinia from its own directory as working-directory.\n\
 You must not change the base/texture/logo.tga(s), nor the base/main.lua files.");
  system("pause");
  exit(0);
}

#endif //CHECK_CRC32

void FS_checkcrc()
{
  /*
#ifdef CHECK_CRC32
  CRC32entry_t *entry;
  FSFile_t *file;

  entry = CRC32table;

  while (entry->name){
    file = FS_open(entry->name);

    if (!file){
      bprintf("ERROR: crc check failed, file not found %s\n",entry->name);
      FS_crcfailed();
    }

    if (calculatecrc32((uchar*)file->contents,file->size) != entry->crc32){
      bprintf("ERROR: crc check failed %s\n",entry->name);
      FS_close(file);
      FS_crcfailed();
    }

    FS_close(file);

    entry++;
  }
#endif
  */
  return;
}

