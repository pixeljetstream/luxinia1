// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __PARSER_H__
#define __PARSER_H__

#include <luxinia/luxplatform/luxplatform.h>
#include <stdio.h>
#include "../common/linkedlistprimitives.h"
#include "../fileio/filesystem.h"

//////////////////////////////////////////////////////////////////////////
// FileParse

typedef booln (FileParseDefineCheck_fn) (char *arg);
typedef Char2PtrNode_t* (FileParseAnnotationAlloc_fn) (const char *str1, const char *str2, size_t str2len);

typedef struct FileParseDef_s
{
  char *stagename;
  int onlyonce;
  int finishesblock;
  int checkend;
  int (*fnstage) (char *arg);
}FileParseDef_t;

enum FileParseDefType_e
{
  FPDEF_IF,
  FPDEF_ELSEIF,
  FPDEF_ELSE,
  FPDEF_ANNOTATE,

  FPDEF_USERSTART,
};

void FileParse_setDefineCheck(FileParseDefineCheck_fn *func);
void FileParse_setAnnotationAlloc(FileParseAnnotationAlloc_fn *func);
void FileParse_setAnnotationList(Char2PtrNode_t **annotationListHead);

void FileParse_stage(lxFSFile_t *fMM, char **words,void (*targetfunc)(void*,char*,char*), void* target);  // search for goodwords and exec targetfunc


// first defs as presented in FPDef
// last def must have NULL as name
void FileParse_start(lxFSFile_t *fMM, FileParseDef_t *defs);

#endif
