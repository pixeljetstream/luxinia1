// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include <stdio.h>
#include <luxinia/luxplatform/file.h>

  // uncomment if you want crc checking
  // will exit application if crc is wrong
//#define CHECK_CRC32 /*(set by preprocessor)*/

typedef enum FSFileLoaderType_e{
  FILELOADER_NONE,
  FILELOADER_TGA,
  FILELOADER_JPG,
  FILELOADER_DDS,
  FILELOADER_PNG,
  FILELOADER_MA,
  FILELOADER_F3D,
  FILELOADER_WAV,
  FILELOADER_PRT,
  FILELOADER_SHD,
  FILELOADER_MTL,
  FILELOADERS,
}FSFileLoaderType_t;

typedef int (FSFileLoaderFn)(const char *filename, void *outData0, void *outData1);

typedef struct FSFileLoader_s{
  char  *ext;
  FSFileLoaderFn *loader; // set if the resource needs to dereference other resources
}FSFileLoader_t;


typedef void*   (FSexternalOpen_fn)       (const char *filename, long *outsize, int *freecontent);
typedef int     (FSexternalProjectFilename_fn)  (char *filename,const char *name,const char *dir,const char *altdir, FSFileLoaderType_t *outType);
typedef int     (FSexternalFileExists_fn)   (const char *filename);


typedef struct FSdata_s{
  FSexternalOpen_fn       *openFunc;
  FSexternalProjectFilename_fn  *filenameFunc;
  FSexternalFileExists_fn     *existsFunc;
}FSdata_t;

//////////////////////////////////////////////////////////////////////////
// FS
void FS_init();
void FS_checkcrc();
int  FS_internal_fileExists(const char *filename);
int  FS_fileExists(const char *filename);

  // if external open exists we will use it when opening files
  // else internal FILE* based stuff is used
void  FS_setExternalOpen(FSexternalOpen_fn* func);

void  FS_setExternalFileExists(FSexternalFileExists_fn* func);

  // same as with open but for project filename processing
void  FS_setExternalProjectFilename(FSexternalProjectFilename_fn* func);

FSFileLoaderType_t FS_getLoaderType(const char *filename);

  // creates proper filenames, filename must be already allocated
  // must write proper loadertype !
  // also checks if file exists
int  FS_setProjectFilename(char *filename,const char *name,const char *dir,const char *altdir, FSFileLoaderType_t *outType);

  // runs the loadertype func
int  FS_processLoader(FSFileLoaderType_t type, const char *filename, void *outData0, void *outData1);

  // internal versions
  // filename => reinschreiben (256 bytes)
  // dir => resource path (textures)
  // altdir => resource loader source (models)
  // outtype =>
int FS_internal_setProjectFilename(char *filename, const char *name, const char *dir, const char *altdir,FSFileLoaderType_t *outType);
void *  FS_internal_open(const char *filename, long *size, int *freecontent);

//////////////////////////////////////////////////////////////////////////
//FSFile

lxFSFile_t* FS_open(const char *filename);
void    FS_close(lxFSFile_t *file);



//////////////////////////////////////////////////////////////////////////
// helpers
int cmpfilenamesi(const char *filea, const char *fileb);

#endif

