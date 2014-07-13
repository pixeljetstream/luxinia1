// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "mtl.h"
#include "../fileio/filesystem.h"
#include "../resource/resmanager.h"

void fileLoadMTLColor(uchar id);
void fileLoadMTLTex(uchar id);
void fileLoadMTLShader();
void MaterialBuildFromBuffer(Material_t *material);

struct MTLData_s{
  lxFSFile_t        *file;

  Material_t        *material;
  MaterialStage_t     *matStage;
  MaterialStage_t     *matStagesColor;
  MaterialStage_t     *matStagesTex[VID_MAX_SHADER_STAGES];
  MaterialResControl_t  rescontrol[VID_MAX_SHADER_STAGES];
  MaterialModifier_t    modifiers[MATERIAL_MAX_MODIFIERS];
  MaterialControl_t   controls[MATERIAL_MAX_CONTROLS];
  MaterialParam_t     params[MATERIAL_MAX_SHADERS][VID_MAX_SHADERPARAMS];

  int   matStagesColorInBuffer;
  int   matStagesTexInBuffer;
  int   modifiersInBuffer;
  int   controlsInBuffer;
  int   paramsInBuffer[MATERIAL_MAX_SHADERS];
  int   shaderLoad;
  int   shdcontrolsInBuffer;
  int   texcontrolsInBuffer;
  int   curShader;

  // Stack
  MaterialModifier_t  modifierStack[4];
  booln       modifierStackUsed[4];
  MaterialControl_t controlStack[4];
  booln       controlStackUsed[4];
}l_MTLData;

static int MTL_shader(char *buf)
{
  if (!sscanf(buf,"Shader:%i",&l_MTLData.curShader))
    l_MTLData.curShader = 0;

  l_MTLData.curShader %= MATERIAL_MAX_SHADERS;

  if (l_MTLData.curShader == 0)
    l_MTLData.shaderLoad = LUX_TRUE;

  fileLoadMTLShader();

  return LUX_FALSE;
}

static int MTL_texture(char *buf)
{
  int id;
  if(l_MTLData.matStagesTexInBuffer < VID_MAX_SHADER_STAGES){
    if (sscanf(buf,"Texture:%i",&id)){
      fileLoadMTLTex( id + MATERIAL_TEX0);
      l_MTLData.material->numStages++;
      l_MTLData.matStagesTexInBuffer++;
    }
  }
  else lprintf("WARNING mtlload: too many tex stages\n");

  return LUX_FALSE;
}

static int MTL_color(char *buf)
{
    int id;
  if(l_MTLData.matStagesColorInBuffer < 1){
    if (sscanf(buf,"Color:%i",&id)){
      fileLoadMTLColor(MATERIAL_COLOR0);
      l_MTLData.material->numStages++;
      l_MTLData.matStagesColorInBuffer++;
    }
  }
  else lprintf("WARNING mtlload: too many color stages\n");

  return LUX_FALSE;
}

static FileParseDef_t l_defMTL[]=
{
  {"IF:",     LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},
  {"ELSEIF:",   LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},
  {"ELSE",    LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},
  {"<<_",     LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},

  {"Color",   LUX_FALSE,LUX_TRUE,LUX_FALSE, MTL_color},
  {"Texture",   LUX_FALSE,LUX_TRUE,LUX_FALSE, MTL_texture},
  {"Shader",    LUX_FALSE,LUX_TRUE,LUX_FALSE, MTL_shader},

  {NULL,      LUX_FALSE,LUX_FALSE,LUX_FALSE,NULL},
};

int fileLoadMTL(const char *filename,Material_t *material,void *unused)
{
  lxFSFile_t * fMTL;
  char buf[1024];
  int id;

  fMTL = FS_open(filename);
  l_MTLData.file = fMTL;
  l_MTLData.material = material;
  lprintf("Material: \t%s\n",filename);

  if(fMTL == NULL)
  {
    lprintf("ERROR mtlload: ");
    lnofile(filename);
    return LUX_FALSE;
  }

  lxFS_gets(buf, 255, fMTL);
  if (!sscanf(buf,MTL_HEADER,&id) || id < MTL_VER_MINIMUM)
  {
    // wrong header
    lprintf("ERROR mtlload: invalid file format or version\n");
    FS_close(fMTL);
    return LUX_FALSE;
  }
  material->numMods = 0;
  material->numStages = 0;
  material->numControls = 0;
  material->numResControls = 0;
  material->matobj = &material->matobjdefault;

  for (id = 0; id < MATERIAL_MAX_SHADERS; id++){
    material->shaders[id].rfalpha = -2.0f;
    material->shaders[id].resRID = -1;
  }

  l_MTLData.shdcontrolsInBuffer = 0;
  l_MTLData.texcontrolsInBuffer = 0;
  l_MTLData.matStagesColorInBuffer = 0;
  l_MTLData.matStagesTexInBuffer = 0;
  l_MTLData.modifiersInBuffer = 0;
  l_MTLData.controlsInBuffer = 0;
  l_MTLData.shaderLoad = 0;
  l_MTLData.curShader = 0;

  for (id = 0; id < MATERIAL_MAX_SHADERS; id++){
    l_MTLData.paramsInBuffer[id] = 0;
    clearArray(l_MTLData.params[id],VID_MAX_SHADERPARAMS);
  }

  clearArray(l_MTLData.rescontrol,VID_MAX_SHADER_STAGES);
  clearArray(l_MTLData.matStagesTex,VID_MAX_SHADER_STAGES);
  clearArray(l_MTLData.modifiers,MATERIAL_MAX_MODIFIERS);
  clearArray(l_MTLData.modifierStack,4);
  clearArray(l_MTLData.modifierStackUsed,4);
  clearArray(l_MTLData.controls,MATERIAL_MAX_CONTROLS);
  clearArray(l_MTLData.controlStack,4);
  clearArray(l_MTLData.controlStackUsed,4);

  l_MTLData.matStagesColor = NULL;

  FileParse_setAnnotationList(&l_MTLData.material->annotationListHead);
  FileParse_start(fMTL,l_defMTL);

  if (!l_MTLData.shaderLoad){
    lprintf("WARNING mtlload: no shader 0 defined\n");
    FS_close (fMTL);
    return LUX_FALSE;
  }

  MaterialBuildFromBuffer(material);
  dlprintf("\tColors: %d Textures: %d\n",l_MTLData.matStagesColorInBuffer,l_MTLData.matStagesTexInBuffer);
  FS_close (fMTL);

  return LUX_TRUE;
}

// Modifier & Controls
// -------------------

void addModifiers(void *value0, void *value1, void* value2, void *value3, int capped,MaterialValue_t target)
{
  // for each argument add modifiers if available
  int i;
  void *values[]={value0,value1,value2,value3};

  for(i=0; i < 4; i++){
    if (l_MTLData.modifierStackUsed[i] && values[i] != NULL){
      MaterialModifier_initSource(&l_MTLData.modifierStack[i],values[i],capped, target+i);
      if (l_MTLData.modifiersInBuffer < MATERIAL_MAX_MODIFIERS){
        memcpy(&l_MTLData.modifiers[l_MTLData.modifiersInBuffer],&l_MTLData.modifierStack[i],sizeof(MaterialModifier_t));
        l_MTLData.modifiersInBuffer++;
      }
    }
    else if (l_MTLData.controlStackUsed[i] && values[i] != NULL)
    {
      MaterialControl_initSource(&l_MTLData.controlStack[i],values[i],target+i);
      if (l_MTLData.controlsInBuffer < MATERIAL_MAX_CONTROLS){
        memcpy(&l_MTLData.controls[l_MTLData.controlsInBuffer],&l_MTLData.controlStack[i],sizeof(MaterialControl_t));
        l_MTLData.controlsInBuffer++;
      }
    }
  }

  clearArray(l_MTLData.controlStack,4);
  clearArray(l_MTLData.controlStackUsed,4);

  clearArray(l_MTLData.modifierStack,4);
  clearArray(l_MTLData.modifierStackUsed,4);
}

void loadModifier(void * target,char * command, char* rest)
{
  char buffer[1000];
  char src[100];
  char name[256];
  const char *lastname;
  int bufferpos=0,restpos=0;
  int type = 0;
  int cap = 0;
  int delay = 0;
  float val = 0;
  int  pos = 4;
  void  *stage = NULL;

  lastname = lxStrReadInQuotes(rest, name, 255);
  if (lastname){
    while (&rest[restpos++] != lastname);
  }
  while (rest[restpos]!=0 && rest[restpos]!=';')
    buffer[bufferpos++]=rest[restpos++];
  buffer[bufferpos]=0;

  sscanf(buffer,"%s %d (%f) %d %d",&src,&delay,&val,&cap,&pos);

  type = 255;

  if (strcmp(src,"ADD")==0)
    type = 0;
  if (strcmp(src,"SIN")==0)
    type = 1;
  if (strcmp(src,"COS")==0)
    type = 2;
  if (strcmp(src,"ZIGZAG")==0)
    type = 3;


  // create the modifier in the stack
  if ( type < 4 && delay != 0 && pos < 4 && pos >= 0){
    if (l_MTLData.matStage)
      stage = l_MTLData.matStage;

    MaterialModifier_init(&l_MTLData.modifierStack[pos],name,(unsigned char)type,delay,val,(unsigned char)cap,stage,l_MTLData.curShader);
    l_MTLData.modifierStackUsed[pos] = LUX_TRUE;
  }
}

void loadControl(void * target,char * command, char* rest)
{
  char buffer[1000];
  char name[256];
  const char *lastname;
  int bufferpos=0,restpos=0;
  int  pos = 4;
  int  length = 1;
  void  *stage = NULL;

  lastname = lxStrReadInQuotes(rest, name, 255);
  if (lastname){
    while (&rest[restpos++] != lastname);
  }
  while (rest[restpos]!=0 && rest[restpos]!=';')
    buffer[bufferpos++]=rest[restpos++];
  buffer[bufferpos]=0;

  sscanf(buffer,"%d %d",&pos,&length);

  // create the modifier in the stack
  if (lastname && lastname[0]){
    if (l_MTLData.matStage)
      stage = l_MTLData.matStage;
    else
      stage = target;

    MaterialControl_init(&l_MTLData.controlStack[pos],name,stage,l_MTLData.curShader,length);
    l_MTLData.controlStackUsed[pos] = LUX_TRUE;
  }
}

// Shader
// ------

void loadShader(void * target,char * command, char* rest)
{
  char buffer[1000];
  int bufferpos=0,restpos=0;

  if (strcmp(command,"floatmod")== 0){
    loadModifier((void*)l_MTLData.paramsInBuffer[l_MTLData.curShader],command,rest);
    return;
  }
  if (strcmp(command,"control")== 0){
    loadControl((void*)l_MTLData.paramsInBuffer[l_MTLData.curShader],command,rest);
    return;
  }
  if (strcmp(command,"SHD")==0) {
    char name[256];
    char *end;

    lxStrReadInQuotes(rest, name, 255);
    resnewstrcpy(l_MTLData.material->shaders[l_MTLData.curShader].name,name);
    // split
    if (end=strrchr(l_MTLData.material->shaders[l_MTLData.curShader].name,'^')){
      l_MTLData.material->shaders[l_MTLData.curShader].extname = end+1;
      *end = 0;
    }
  }
  if (strcmp(command,"param")==0){
    char  name[256];
    MaterialParam_t *param;
    const char *endstr = lxStrReadInQuotes(rest,name,255);

    param = &l_MTLData.params[l_MTLData.curShader][l_MTLData.paramsInBuffer[l_MTLData.curShader]++];
    resnewstrcpy(param->name,name);
    param->pass = 0;
    param->arraysize = 0;
    param->vecP = param->vec;
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"%s (%f,%f,%f,%f) %d %d",name,&param->vec[0],&param->vec[1],&param->vec[2],&param->vec[3],&param->pass,&param->arraysize);
    addModifiers(&param->vec[0],&param->vec[1],&param->vec[2],&param->vec[3],0,MATERIAL_MOD_PARAM_1);

  }
  if ((strcmp(command,"rfalpha")==0) || (strcmp(command,"alpha")==0)) {
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"(%f)",&l_MTLData.material->shaders[l_MTLData.curShader].rfalpha);
  }
  if (strcmp(command,"shdcontrol")==0){
    char name[256];
    MaterialResControl_t *texctrl;
    if (l_MTLData.shdcontrolsInBuffer == MATERIAL_MAX_SHADERS){
      bprintf("ERROR matload: too many shdcontrols\n");
      return;
    }

    lxStrReadInQuotes(rest, name, 255);
    texctrl = &l_MTLData.rescontrol[l_MTLData.shdcontrolsInBuffer+l_MTLData.texcontrolsInBuffer];
    l_MTLData.shdcontrolsInBuffer++;

    if(name)
      resnewstrcpy(texctrl->name,name);

    texctrl->type = MATERIAL_CONTROL_RESSHD;
    texctrl->sourcewinsizedp = (int*)&texctrl->sourcewinsizedp; //&l_MTLData.material->shaders[l_MTLData.curShader].numShaderTotalParams;
    texctrl->sourcep = &l_MTLData.material->shaders[l_MTLData.curShader].resRIDUse;
    return;
  }
}
void fileLoadMTLShader()
{
  static char *words[100]={ "SHD","alpha","rfalpha","floatmod","control","param","shdcontrol","\0"};
  l_MTLData.matStage = NULL;
  FileParse_stage(l_MTLData.file,words,loadShader,l_MTLData.material);
}


// Sequence
// --------

void loadSeq(void *target,char * command, char* rest)
{
  char buffer[1000];
  int bufferpos=0,restpos=0;
  MaterialStage_t *stage = (MaterialStage_t*)target;
  MaterialSeq_t *seq = stage->seq;

  if (strcmp(command,"frames")==0) {
    while (rest[restpos]!=0 && rest[restpos]!='\n')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"%d",&seq->numFrames);
    if (stage->seq == NULL ){
      seq = stage->seq = reszalloc(sizeof(MaterialSeq_t));
    }
    if (stage->id == MATERIAL_COLOR0 && seq->colors == NULL){
      seq->colors = reszalloc(sizeof(float)*4*seq->numFrames);
    }
    else if (stage->id != MATERIAL_COLOR0 && seq->textures == NULL){
      seq->textures = reszalloc(sizeof(char*)*seq->numFrames);
      seq->texRID = reszalloc(sizeof(int)*seq->numFrames);
      seq->texTypes = reszalloc(sizeof(TextureType_t)*seq->numFrames);
    }
    return;
  }
  if (!seq)
    return;
  if (strcmp(command,"delay")==0) {
    while (rest[restpos]!=0 && rest[restpos]!='\n')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"%d",&seq->delay);
    seq->delayfrac = 1/(float)seq->delay;
    return;
  }
  if (strcmp(command,"loop")==0) {
    int l;
    while (rest[restpos]!=0 && rest[restpos]!='\n')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"%d",&l);
    if (l > 255)
      seq->loops = 255;
    else if (l < 0)
      seq->loops = 0;
    else
      seq->loops = l;
    return;
  }
  if (strcmp(command,"wait")==0) {
    int l;
    while (rest[restpos]!=0 && rest[restpos]!='\n')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"%d",&l);
    if (l < 0)
      seq->wait = -l;
    else
      seq->wait = l;

    return;
  }
  if (strcmp(command,"reverse")==0) {
    int l;
    while (rest[restpos]!=0 && rest[restpos]!='\n')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"%d",&l);
    if (l < 0)
      seq->reverse = -l;
    else
      seq->reverse = l;

    return;
  }
  if (strcmp(command,"interpolate")==0){
    seq->interpolate = LUX_TRUE;
    return;
  }
}

// Color
// -----
void loadColorMat(void * target,char * command, char* rest)
{
  char buffer[1000];
  int bufferpos=0,restpos=0;
  MaterialStage_t *stage = l_MTLData.matStage;

  if (strcmp(command,"floatmod")== 0 ){
    loadModifier(target,command,rest);
    return;
  }
  if (strcmp(command,"control")== 0){
    loadControl(target,command,rest);
    return;
  }

  // Sequence specific
  loadSeq(target,command,rest);

  if (strcmp(command,"RGBA")==0) {
    while (rest[restpos]!=0 && rest[restpos]!='\n')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    if (stage->seq == NULL){
      sscanf(buffer,"(%f,%f,%f,%f)",&stage->color[0],&stage->color[1],&stage->color[2],&stage->color[3]);
      addModifiers(&stage->color[0],&stage->color[1],&stage->color[2],&stage->color[3],1,MATERIAL_MOD_RGBA_1);
    }
    else{
      // Sequence
      uint a = stage->seq->activeFrame;
      sscanf(buffer,"(%f,%f,%f,%f)",&stage->seq->colors[a*4+0],&stage->seq->colors[a*4+1],&stage->seq->colors[a*4+2],&stage->seq->colors[a*4+3]);
      stage->seq->activeFrame++;
      stage->seq->activeFrame%=stage->seq->numFrames;
    }
    return;
  }
}
void fileLoadMTLColor(uchar id)
{
  static char *words[100]={ "RGBA","frames","delay","loop","wait","reverse","interpolate","floatmod","control","\0"};
  MaterialStage_t *stage;
  if (!l_MTLData.matStagesColor)
    l_MTLData.matStagesColor =reszalloc(sizeof(MaterialStage_t));
  stage = l_MTLData.matStagesColor;
  memset(stage,0,sizeof(MaterialStage_t));
  stage->color[3] = -1;
  stage->id = id;
  l_MTLData.matStage = stage;
  FileParse_stage(l_MTLData.file,words,loadColorMat,stage);

  if (stage->seq){
    stage->color[0] = stage->seq->colors[0];
    stage->color[1] = stage->seq->colors[1];
    stage->color[2] = stage->seq->colors[2];
    if (stage->color[3] > 0)
      stage->color[3] = stage->seq->colors[3];

    if (stage->seq->delay == 0){
      stage->seq->delay = 1000;
      stage->seq->delayfrac =1/1000;
    }
  }
  stage->pColor = stage->color;
}
// Texture
// -------

void loadTexMat(void * target,char * command, char* rest)
{
  char buffer[1024];
  int bufferpos=0,restpos=0;
  int tex = 0;
  int attributes = TEX_ATTR_MIPMAP;
  MaterialStage_t *stage = l_MTLData.matStage;

  if (strcmp(command,"floatmod")== 0){
    loadModifier(target,command,rest);
    return;
  }
  if (strcmp(command,"control")== 0){
    loadControl(target,command,rest);
    return;
  }

  if (target != NULL){
    loadSeq(target,command,rest);
  }

  if (strcmp(command,"TEXALPHA")==0)
    tex = TEX_ALPHA;
  else if (strcmp(command,"TEXCUBE")==0){
    tex = TEX_COLOR;
    attributes |= TEX_ATTR_CUBE;
  }
  else if (strcmp(command,"TEXPROJ")==0){
    tex = TEX_COLOR;
    attributes |= TEX_ATTR_PROJ;
  }
  else if (strcmp(command,"TEXDOTZ")==0){
    tex = TEX_COLOR;
    attributes |= TEX_ATTR_DOTZ;
  }
  else if (strcmp(command,"TEX")==0)
    tex = TEX_COLOR;

  if (tex) {
    char name[256];
    lxStrReadInQuotes(rest, name, 255);

    stage->texType = tex;
    stage->texAttributes = attributes;

    if (!stage->seq){
      resnewstrcpy(stage->texName,name);
    }
    else {
      // Sequence
      uint a = stage->seq->activeFrame;
      resnewstrcpy(stage->seq->textures[a],name);
      stage->seq->texTypes[a] = stage->texType;
      stage->seq->activeFrame++;
      stage->seq->activeFrame%=stage->seq->numFrames;
    }

    return;
  }
  if (strcmp(command,"texmove")==0) {
    if (stage->texcoord == NULL){
      stage->texcoord = reszallocSIMD(sizeof(MaterialTexCoord_t));
      MaterialTexCoord_clear(stage->texcoord);
    }
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"(%f,%f,%f)",&stage->texcoord->move[0],&stage->texcoord->move[1],&stage->texcoord->move[2]);
    addModifiers(&stage->texcoord->move[0],&stage->texcoord->move[1],&stage->texcoord->move[2],NULL,0,MATERIAL_MOD_TEXMOVE_1);
    return;
  }
  if (strcmp(command,"texcenter")==0) {
    if (stage->texcoord == NULL){
      stage->texcoord = reszallocSIMD(sizeof(MaterialTexCoord_t));
      MaterialTexCoord_clear(stage->texcoord);
    }
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"(%f,%f,%f)",&stage->texcoord->center[0],&stage->texcoord->center[1],&stage->texcoord->center[2]);
    addModifiers(&stage->texcoord->center[0],&stage->texcoord->center[1],&stage->texcoord->center[2],NULL,0,MATERIAL_MOD_TEXCENTER_1);
    return;
  }
  if (strcmp(command,"texscale")==0) {
    if (stage->texcoord == NULL){
      stage->texcoord = reszallocSIMD(sizeof(MaterialTexCoord_t));
      MaterialTexCoord_clear(stage->texcoord);
    }
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"(%f,%f,%f)",&stage->texcoord->scale[0],&stage->texcoord->scale[1],&stage->texcoord->scale[2]);
    addModifiers(&stage->texcoord->scale[0],&stage->texcoord->scale[1],&stage->texcoord->scale[2],NULL,0,MATERIAL_MOD_TEXSCALE_1);
    return;
  }
  if (strcmp(command,"texrotate")==0) {
    if (stage->texcoord == NULL){
      stage->texcoord = reszallocSIMD(sizeof(MaterialTexCoord_t));
      MaterialTexCoord_clear(stage->texcoord);
    }
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"(%f,%f,%f)",&stage->texcoord->rotate[0],&stage->texcoord->rotate[1],&stage->texcoord->rotate[2]);
    addModifiers(&stage->texcoord->rotate[0],&stage->texcoord->rotate[1],&stage->texcoord->rotate[2],NULL,0,MATERIAL_MOD_TEXROT_1);
    return;
  }
  if (strcmp(command,"texmatrixc0")==0 || strcmp(command,"texmatrixc1")==0 || strcmp(command,"texmatrixc2")==0 ||strcmp(command,"texmatrixc3")==0) {
    float *pMatrix;
    int read;
    read = (uchar)command[10]-(uchar)'0';

    if (stage->texcoord == NULL){
      stage->texcoord = reszallocSIMD(sizeof(MaterialTexCoord_t));
      MaterialTexCoord_clear(stage->texcoord);
      lxMatrix44Identity(stage->texcoord->usermatrix);
    }
    stage->texcoord->usemanual = LUX_TRUE;
    pMatrix = &stage->texcoord->usermatrix[read*4];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"(%f,%f,%f,%f)",&pMatrix[0],&pMatrix[1],&pMatrix[2],&pMatrix[3]);
    addModifiers(&pMatrix[0],&pMatrix[1],&pMatrix[2],&pMatrix[3],0,MATERIAL_MOD_TEXMAT0_1+read*4);
    return;
  }
  if (strcmp(command,"texgenplane0")==0 || strcmp(command,"texgenplane1")==0 || strcmp(command,"texgenplane2")==0 ||strcmp(command,"texgenplane3")==0) {
    float *pMatrix;
    int read;
    read = (uchar)command[strlen("texgenplane")]-(uchar)'0';

    if (stage->texgen == NULL){
      stage->texgen = reszallocSIMD(sizeof(lxMatrix44));
      lxMatrix44Identity(stage->texgen);
    }
    pMatrix = &stage->texgen[read*4];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"(%f,%f,%f,%f)",&pMatrix[0],&pMatrix[1],&pMatrix[2],&pMatrix[3]);
    addModifiers(&pMatrix[0],&pMatrix[1],&pMatrix[2],&pMatrix[3],0,MATERIAL_MOD_TEXGEN0_1+read*4);
    return;
  }
  if (strcmp(command,"texconst")==0) {
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    lxStrReadVector4(buffer,stage->color);
    addModifiers(&stage->color[0],&stage->color[1],&stage->color[2],&stage->color[3],1,MATERIAL_MOD_RGBA_1);
    return;
  }
  if (strcmp(command,"texclamp")==0) {
    int states[3];
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    if (3==sscanf(buffer,"(%d,%d,%d)",&states[0],&states[1],&states[2])){
      int i;
      for (i = 0; i < 3; i++)
        if (states[i])
          stage->texclamp |= 1<<i;
    }
    return;
  }
  if (strcmp(command,"texcontrol")==0){
    char name[256];
    MaterialResControl_t *texctrl;
    if (l_MTLData.texcontrolsInBuffer == VID_MAX_SHADER_STAGES){
      bprintf("ERROR matload: too many texcontrols\n");
      return;
    }

    lxStrReadInQuotes(rest, name, 255);
    texctrl = &l_MTLData.rescontrol[l_MTLData.shdcontrolsInBuffer+l_MTLData.texcontrolsInBuffer];
    l_MTLData.texcontrolsInBuffer++;

    if(name)
      resnewstrcpy(texctrl->name,name);

    texctrl->type = MATERIAL_CONTROL_RESTEX;
    texctrl->sourcewinsizedp = &stage->windowsized;
    texctrl->sourcep = &stage->texRIDUse;
    return;
  }
}

void fileLoadMTLTex(uchar id)
{
  static char *words[100]={ "TEX","TEXALPHA","TEXCUBE","TEXPROJ","TEXDOTZ","texmove","texrotate","texscale","texcenter","texconst","floatmod",
                "frames","delay","loop","wait","reverse","control","texclamp","texcontrol",
                "texmatrixc0","texmatrixc1","texmatrixc2","texmatrixc3","texgenplane0","texgenplane1","texgenplane2","texgenplane3",
                "\0"};
  MaterialStage_t *stage;
  if (!l_MTLData.matStagesTex[l_MTLData.matStagesTexInBuffer])
    l_MTLData.matStagesTex[l_MTLData.matStagesTexInBuffer] =reszalloc(sizeof(MaterialStage_t));

  stage = l_MTLData.matStagesTex[l_MTLData.matStagesTexInBuffer];
  memset(stage,0,sizeof(MaterialStage_t));
  stage->color[3] = -1;
  stage->id = id;
  l_MTLData.matStage = stage;
  FileParse_stage(l_MTLData.file,words,loadTexMat,stage);
  if (stage->seq && stage->seq->delay == 0){
    stage->seq->delay = 1000;
    stage->seq->delayfrac =1/1000;
  }
  if (stage->color[3] > -1)
    stage->pColor = stage->color;
  if (stage->texcoord)
    stage->pMatrix = stage->texcoord->matrix;
}



void MaterialBuildFromBuffer(Material_t *material)
{
  int i,n;
  int offset;
  // copy stages
  offset = 0;
  material->numStages = l_MTLData.matStagesColorInBuffer + l_MTLData.matStagesTexInBuffer;
  material->stages = reszalloc(sizeof(MaterialStage_t*)*material->numStages);
  n = 0;

  for (i = 0; i < l_MTLData.matStagesColorInBuffer + l_MTLData.matStagesTexInBuffer; i++){
    MaterialStage_t *stage =  l_MTLData.matStagesTex[i-l_MTLData.matStagesColorInBuffer];

    if (i == 0 && l_MTLData.matStagesColorInBuffer)
      material->stages[n]=l_MTLData.matStagesColor;
    else{
      material->stages[n]=stage;
    }
    n++;
  }

  // copy params

  for (i= 0; i < MATERIAL_MAX_SHADERS; i++){
    MaterialShader_t *mshader = &material->shaders[i];

    mshader->numParams = l_MTLData.paramsInBuffer[i];
    if (mshader->numParams)
    {
      MaterialParam_t *mparam;
      mshader->params = (MaterialParam_t*)reszalloc(sizeof(MaterialParam_t)*mshader->numParams);
      memcpy(mshader->params,l_MTLData.params[i],sizeof(MaterialParam_t)*mshader->numParams);

      mparam = mshader->params;
      for (n=0; n < mshader->numParams; n++,mparam++){
        if(mparam->arraysize > 1){
          int s;
          mparam->vecP = reszalloc(sizeof(lxVector4)*mparam->arraysize);
          for (s = 0; s < mparam->arraysize; s++){
            memcpy(&mparam->vecP[s*4],mparam->vecP,sizeof(lxVector4));
          }
        }
        else{
          mparam->vecP = mparam->vec;
        }
      }
    }
  }



  // copy modifiers
  material->numMods = l_MTLData.modifiersInBuffer;
  if (material->numMods)
  {
  material->mods = (MaterialModifier_t*)reszalloc(sizeof(MaterialModifier_t)*material->numMods);
  for (i = 0; i < l_MTLData.modifiersInBuffer; i++){
    memcpy(&material->mods[i],&l_MTLData.modifiers[i],sizeof(MaterialModifier_t));
    // find stage number for modifier target
    for (n = 0; n < material->numStages; n++){
      if (material->stages[n] == material->mods[i].data){
        material->mods[i].dataID = n;
        break;
      }
    }
  }
  }

  // copy controls
  material->numControls = l_MTLData.controlsInBuffer;
  if (l_MTLData.controlsInBuffer){
    material->controls = (MaterialControl_t*)reszalloc(sizeof(MaterialControl_t)*material->numControls);
    material->matobjdefault.numControls = material->numControls;
    for (i = 0; i < l_MTLData.controlsInBuffer; i++){
      memcpy(&material->controls[i],&l_MTLData.controls[i],sizeof(MaterialControl_t));
      // find stage number for modifier target
      for (n = 0; n < material->numStages; n++){
        if (material->stages[n] == material->controls[i].data){
          material->controls[i].dataID = n;
          break;
        }
      }
    }
  }

  // copy rescontrols
  material->numResControls = l_MTLData.texcontrolsInBuffer + l_MTLData.shdcontrolsInBuffer;
  if (material->numResControls)
  {
    material->rescontrols = (MaterialResControl_t*)reszalloc(sizeof(MaterialResControl_t)*material->numResControls);
    for (i = 0; i < material->numResControls; i++){
      memcpy(&material->rescontrols[i],&l_MTLData.rescontrol[i],sizeof(MaterialResControl_t));
    }
  }

  if (material->numMods || material->numControls)
    material->matobj->materialflag |= MATERIAL_ANIMATED;
}

