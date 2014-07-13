// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "parser.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <luxinia/luxcore/strmisc.h>

static void strunstring(char *buffer, char* rest, int* bufferpos, int* restpos)
{
  while ( rest[*restpos]!=0 &&
      !(rest[*restpos]=='\"' && rest[*restpos-1]!='\\')) {
    if (rest[*restpos]=='\"') (*bufferpos)--;
    if (rest[*restpos]=='n' && rest[*restpos-1]=='\\') {
      rest[*restpos]='\n';
      (*bufferpos)--;
    }
    buffer[(*bufferpos)++]=rest[(*restpos)++];
  }
}
static void strreadfloat(char *buffer,char *rest, int *bufferpos, int *restpos, float *f)
{
  while (rest[*restpos]!=0 && rest[*restpos]<'0' && rest[*restpos]!='-') (*restpos)++;

  while (rest[*restpos]!=0 &&
    (rest[*restpos]<='9' && rest[*restpos]>='0' || rest[*restpos]=='.' || rest[*restpos]=='-'))
  {
    buffer[(*bufferpos)++]=rest[(*restpos)++];
  }
  buffer[*bufferpos]=0;
  *f = (float)atof(buffer);
}
static void strreadint(char *buffer,char *rest, int *bufferpos, int *restpos, int *f)
{
  while (rest[*restpos]!=0 && rest[*restpos]<'0' && rest[*restpos]!='-') (*restpos)++;

  while (rest[*restpos]!=0 &&
    (rest[*restpos]<='9' && rest[*restpos]>='0' || rest[*restpos]=='-'))
  {
    buffer[(*bufferpos)++]=rest[(*restpos)++];
  }
  buffer[*bufferpos]=0;
  *f = atoi(buffer);
}

void strreadvector(char *buffer, char *rest, int* bufferpos, int *restpos,float *vec,int n)
{
  int i;
  while (rest[*restpos]!=0 && rest[*restpos]!='(') (*restpos)++;
  (*restpos)++;
  for (i=0;i<n;i++) {
    *bufferpos=0;
    strreadfloat(buffer,rest,bufferpos,restpos,&vec[i]);
  }
  while (rest[*restpos]!=0 && rest[*restpos]!=')') (*restpos)++;
  (*restpos)++;
}

static int strreadArg(char *start, char *buf)
{
  char *out = buf;

  while (*start && *start != ':')start++;

  if (*start == ':'){
    start++;
    while (*start && *start != '{'){
      *out++ = *start++ ;
    }
    *out = 0;
  }
  return (out > buf);
}
//////////////////////////////////////////////////////////////////////////
// FileParse_stage for SHD/MTL/PRT

static booln alwaysfail(const char *name){
  return LUX_FALSE;
}

static Char2PtrNode_t **l_annoListHead;
static FileParseDefineCheck_fn  *l_defineFunc = alwaysfail;
static FileParseAnnotationAlloc_fn  *l_allocFunc = NULL;


void FileParse_setAnnotationList(Char2PtrNode_t **annotationListHead)
{
  l_annoListHead = annotationListHead;
}

void FileParse_setDefineCheck(FileParseDefineCheck_fn *func){
  l_defineFunc = func ? func : alwaysfail;
}
void FileParse_setAnnotationAlloc(FileParseAnnotationAlloc_fn *func){
  l_allocFunc = func;
}

void FileParse_annotation(lxFSFile_t *fMM,const char *rest)
{
  static char buffer[256];
  char c;
  const char *cptr;
  const char *start;
  size_t  strlength;
  Char2PtrNode_t  *annotation;
  Char2PtrNode_t  *listhead;

  cptr = lxStrReadInQuotes(rest,buffer,256);
  if (!rest || !cptr){
    bprintf("ERROR: annotation name missing\n");
    return;
  }
  rest += strlen(rest);
  while ((c=*cptr++) && c != ';' && c != '\n');
  // find start and end ptrs

  start = lxFS_getCurrent(fMM);
  start -= (rest-cptr);

  cptr = strstr(start,"_>>");
  if (!cptr){
    bprintf("ERROR: annotation end missing\n");
    return;
  }

  strlength = cptr-start;
  if (!l_allocFunc || !(annotation = l_allocFunc(buffer,start,sizeof(char)*(strlength+1)))){
    bprintf("ERROR: annotation alloc failed\n");
    return;
  }


  listhead = *l_annoListHead;
  lxListNode_init(annotation);
  lxListNode_addLast(listhead,annotation);
  *l_annoListHead = listhead;

}

void FileParse_stage(lxFSFile_t *fMM, char **words, void (*targetfunc)(void*,char*,char*), void* target)
{ // fMM    = file pointer
  // words  = pointer to good word list
  // target = what else you want to hand over to the targetfunc
  // targetfunc = if a good word is found, targetfunc(target, buffer, argstart) is called
  //        whereas buffer is goodword + chars until ';' and argstart the pointer to first char
  //        after goodword
  // we parse characters of the file until end of a paragraph { or if end of file is reached

  int i;
  int raiser = 1;
  char c=0;
  char c2=0;
  char buffer[1024];
  char src[256];
  int wcnt=0,rest=0,unknownword;  //wcnt is length of our word in the buffer,
                  //rest is the position in the buffer for the next search
  int blockdepth = 0;
  int ifblockdepth = 0;
  int startblockdepth = 0;
  int activeblockdepth = 0;
  uchar ifblockactive[64];
  uchar ifblockfinished[64];
  memset(ifblockfinished,0,sizeof(uchar)*64);
  memset(ifblockactive,0,sizeof(uchar)*64);


  //if active block
  while ((c=lxFS_getc(fMM))!=EOF) {
    if (c<'!') continue; //ignore spaces at beginning
    if (c=='}'){
      blockdepth--;
      if (blockdepth < startblockdepth)
        break;
      if (blockdepth < activeblockdepth)
        activeblockdepth = blockdepth;
      continue;
    }
    if (c=='{'){
      blockdepth++;

      if (!startblockdepth){
        startblockdepth++;
        activeblockdepth = blockdepth;
      }
      continue;
    }

    wcnt=0;
    if (c=='/') { //ignore comments
      wcnt = 1;
      buffer[0] = c;  // store old

      c = lxFS_getc(fMM);

      // single comment
      if (c == '/'){
        while ((c=lxFS_getc(fMM))!=EOF && c!='\n');
        continue;
      }
      else if (c == '*'){ // block comment, /* .... */
        char pc = c;
        while (pc != EOF && pc != '*'){
          c=lxFS_getc(fMM);
          while (c!=EOF && c!='/'){
            pc = c;
            c=lxFS_getc(fMM);
          }
        }
        continue;
      }
    }
    // now we read a word
    // FIXME comments?
    do
      buffer[wcnt++]=c;
    while ((c=lxFS_getc(fMM))>='!' && c!=';' && c!='{');

    if (c=='{'){
      blockdepth++;
      ifblockdepth = blockdepth;
    }
    else{
      ifblockdepth = blockdepth + 1;
    }

    buffer[wcnt]=0; // terminate it
    rest=wcnt+1; wcnt=0;
    unknownword=1;

    // check for if
    if (strstr(buffer,"IF:") && (ifblockactive[ifblockdepth] = LUX_TRUE) && strreadArg(buffer,src) && l_defineFunc(src))
    {
      activeblockdepth = ifblockdepth;
      ifblockfinished[ifblockdepth] = LUX_TRUE;
    }
    else if (strstr(buffer,"ELSEIF:") && ifblockactive[ifblockdepth] && !ifblockfinished[ifblockdepth] &&
      strreadArg(buffer,src) && l_defineFunc(src))
    {
      activeblockdepth = ifblockdepth;
      ifblockfinished[ifblockdepth] = LUX_TRUE;
    }
    else if (strstr(buffer,"ELSE") && ifblockactive[ifblockdepth] && !ifblockfinished[ifblockdepth]){
      activeblockdepth = ifblockdepth;
      // done with this if
      ifblockfinished[ifblockdepth] = LUX_TRUE;
    }
    else if (blockdepth == activeblockdepth){
      if (ifblockactive[blockdepth] && !ifblockfinished[blockdepth]){
        ifblockactive[blockdepth] = LUX_FALSE;
        ifblockfinished[blockdepth] = LUX_FALSE;
      }
      if (strcmp(buffer,"<<_")== 0){
        FileParse_annotation(fMM,lxFS_gets(buffer,1024,fMM));
        continue;
      }
      i=0; // compare the word with our "good word list"
      while (words[i]!=0) {
        int stringactive = LUX_FALSE;
        if (strcmp(words[i],buffer)!=0) { // no correspondence
          i++; continue;
        }
        // we found a word, read the rest of it
        // need to ignore ; when within ""

        // FIXME comments?
        while ((c=lxFS_getc(fMM))!=EOF && c!='\n'){
          if (c == '\"')
            stringactive = !stringactive;
          if (c==';' && !stringactive)
            break;

          buffer[rest+wcnt++]=c;
        }
        buffer[rest+wcnt]=0;wcnt=0;

        // hand over to targetfunc
        targetfunc(target,buffer,&buffer[rest]);
        unknownword=0;
        break;
      }
    }
    if (unknownword) { // ??
      continue;
    }
  }

}


void FileParse_start(lxFSFile_t *file, FileParseDef_t *defs)
{
  char buffer[1024];
  char *buf;
  char src[256];
  char *stagewords[16];
  int curstageword = -1;
  int blockdepth;
  int activeblockdepth = 1;
  int onceblockdepth = 0;
  uchar ifblockactive[64];
  uchar ifblockfinished[64];
  FileParseDef_t *curdef;
  int onlyonce = 0;
  int numdefs = 0;

  curdef = defs;
  while (curdef->stagename){ numdefs++; curdef++;};

  blockdepth = 0;
  activeblockdepth = 1;
  memset(ifblockfinished,0,sizeof(uchar)*64);
  memset(ifblockactive,0,sizeof(uchar)*64);
  // read block identification
  // we are done when we were within an active technique block and finished it
  while ((buf=lxFS_gets(buffer,1024,file)) != NULL && (!onlyonce || blockdepth != onceblockdepth))
  {
    // check line for comments
    char *c = buf;
    while (*c){
      if (*c == '/'){
        if (*(c+1) == '*'){
          // if block comment traverse till it ends
          if (c=strstr(c,"*/")){  // ends in same line
            c += 2;
            buf = c;
            continue;
          }
          else{         // chase lines
            while ((buf=lxFS_gets(buffer,1024,file)) != NULL && (c=strstr(buf,"*/")) == NULL);
            if (!buf)
              return;     // reached fileend
            else{
              c += 2;
              buf = c;
              continue;
            }
          }
        }
        else if (*(c+1) == '/'){
          // end buffer
          *c = 0;
          // we are done with comment search
          break;
        }
      }
      c++;
    }



    curstageword = -1;
    memset(stagewords,0,sizeof(char*)*16);
    while (curstageword < numdefs-1 && !(stagewords[++curstageword]=strstr(buf, defs[curstageword].stagename)));
    curdef = &defs[curstageword];


    // compare blocks
    // order must match enum
    if (stagewords[curstageword] &&
      (!curdef->checkend || *(stagewords[curstageword]+strlen(defs[curstageword].stagename)) != ':'))
    {
      blockdepth++;

      if (blockdepth < activeblockdepth)
        activeblockdepth = blockdepth;

      //if (strstr (buf,"#") || strstr (buf,"#") < stagewords[curstageword]);
      //else{
      if (ifblockactive[blockdepth] && !ifblockfinished[blockdepth] && curstageword != FPDEF_ELSEIF && curstageword != FPDEF_ELSE){
        ifblockactive[blockdepth] = LUX_FALSE;
        ifblockfinished[blockdepth] = LUX_FALSE;
      }
      if (blockdepth == activeblockdepth){
        switch(curstageword)
        {
        case FPDEF_ANNOTATE:
          FileParse_annotation(file,stagewords[curstageword]+4);
          blockdepth--;
          break;
        case FPDEF_IF:
          if (strreadArg(stagewords[curstageword],src) && l_defineFunc(src))
          {
            activeblockdepth = blockdepth + 1;
            ifblockfinished[blockdepth] = LUX_TRUE;
          }
          ifblockactive[blockdepth] = LUX_TRUE;
          break;
        case FPDEF_ELSEIF:
          if (ifblockactive[blockdepth] && !ifblockfinished[blockdepth] &&
            strreadArg(stagewords[curstageword],src) && l_defineFunc(src))
          {
            activeblockdepth = blockdepth + 1;
            ifblockfinished[blockdepth] = LUX_TRUE;
          }
          break;
        case FPDEF_ELSE:
          if (ifblockactive[blockdepth] && !ifblockfinished[blockdepth])
          {
            activeblockdepth = blockdepth + 1;
            // done with this if
            ifblockfinished[blockdepth] = LUX_TRUE;
          }
          break;
        default:
          if(curdef->fnstage(stagewords[curstageword])){
            activeblockdepth = blockdepth + 1;
            if (curdef->onlyonce){
              onlyonce = LUX_TRUE;
              onceblockdepth = blockdepth-1;
            }
          }
          if (curdef->finishesblock)
            blockdepth--;
          break;
        }
      }
      else if (curstageword == FPDEF_ANNOTATE)
        blockdepth--;
      //}
    }
    else if  (strstr(buf, "}") != NULL)
    {
      blockdepth--;
    }
  }
}


