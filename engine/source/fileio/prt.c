// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "prt.h"
#include "../resource/shader.h"
#include "../render/gl_list3d.h"
#include "../render/gl_render.h"
#include "../fileio/filesystem.h"

void fileLoadPRTRFlag();
void fileLoadPRTParticle();
void fileLoadPRTEmitter();
void fileLoadPRTColor();
void fileLoadPRTTexture();
void fileLoadPRTForce();
void fileLoadPRTSubSys();
void ParticleSysBuildFromBuffer(ParticleSys_t *Psys);

static struct PRTData_s{
  lxFSFile_t    *file;

  ParticleSys_t *psys;
  uchar   rflag;
  uchar   emitter;
  uchar   particle;
  uchar   color;
  uchar   tex;
  uchar   force;
  uchar   subsys;
  uchar   visbox;

  int   activeColor;
  int   activeTex;
  int   activeForce;
  int   activeSubsys;

  ParticleForce_t*  PForces[MAX_PFORCES];
  ParticleSubSys_t  PSubSys[MAX_PSUBSYS];

  int     eventimed;
} l_PRTData;


static int PRT_rflag(char *buf)
{
  if(!l_PRTData.rflag){
    fileLoadPRTRFlag();
    l_PRTData.rflag = 1;
  }
  else lprintf("WARNING shdload: too many renderflags\n");

  return LUX_FALSE;
}

static int PRT_emitter(char *buf)
{
  if(!l_PRTData.emitter){
    fileLoadPRTEmitter();
    l_PRTData.emitter = 1;
  }
  else lprintf("WARNING shdload: too many emitters\n");

  return LUX_FALSE;
}

static int PRT_color(char *buf)
{
  if(!l_PRTData.color){
    fileLoadPRTColor();
    l_PRTData.color = 1;
  }
  else lprintf("WARNING shdload: too many colordefs\n");

  return LUX_FALSE;
}

static int PRT_particle(char *buf)
{
  if(!l_PRTData.particle){
    fileLoadPRTParticle();
    l_PRTData.particle = 1;
  }
  else lprintf("WARNING shdload: too many particledefs\n");

  return LUX_FALSE;
}

static int PRT_forces(char *buf)
{
  if(!l_PRTData.force){
    fileLoadPRTForce();
    l_PRTData.force = 1;
  }
  else lprintf("WARNING shdload: too many forcedefs\n");

  return LUX_FALSE;
}
static int PRT_subsys(char *buf)
{
  if(!l_PRTData.subsys){
    fileLoadPRTSubSys();
    l_PRTData.subsys= 1;
  }
  else lprintf("WARNING shdload: too many subsysdefs\n");

  return LUX_FALSE;
}

static int PRT_texture (char *buf)
{
  if(!l_PRTData.tex){
    fileLoadPRTTexture();
    l_PRTData.tex = 1;
  }
  else lprintf("WARNING shdload: too many texturedefs\n");

  return LUX_FALSE;
}

static FileParseDef_t l_defPRT[]=
{
  {"IF:",     LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},
  {"ELSEIF:",   LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},
  {"ELSE",    LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},
  {"<<_",     LUX_FALSE,LUX_FALSE,LUX_FALSE,  NULL},

  {"Forces",    LUX_FALSE,LUX_TRUE,LUX_FALSE, PRT_forces},
  {"RenderFlag",  LUX_FALSE,LUX_TRUE,LUX_FALSE, PRT_rflag},
  {"Emitter",   LUX_FALSE,LUX_TRUE,LUX_FALSE, PRT_emitter},
  {"Color",   LUX_FALSE,LUX_TRUE,LUX_FALSE, PRT_color},
  {"Texture",   LUX_FALSE,LUX_TRUE,LUX_FALSE, PRT_texture},
  {"SubSystem", LUX_FALSE,LUX_TRUE,LUX_FALSE, PRT_subsys},
  {"Particle",  LUX_FALSE,LUX_TRUE,LUX_FALSE, PRT_particle},

  {NULL,      LUX_FALSE,LUX_FALSE,LUX_FALSE,NULL},
};

int fileLoadPRT(const char *filename,ParticleSys_t *psys,void *unused)
{
  lxFSFile_t * fPRT;
  char buf[256];
  int version;

  fPRT = FS_open(filename);
  l_PRTData.file = fPRT;

  lprintf("Particle: \t%s\n",filename);

  if(fPRT == NULL)
  {
    lprintf("ERROR prtload: ");
    lnofile(filename);
    return LUX_FALSE;
  }

  lxFS_gets(buf, 255, fPRT);
  if (!sscanf(buf,PRT_HEADER,&version) || version < PRT_VER_MINIMUM)
  {
    // wrong header
    lprintf("ERROR prtload: invalid file format or version\n");
    FS_close(fPRT);
    return LUX_FALSE;
  }

  l_PRTData.psys = psys;

  clearArray(l_PRTData.PForces,MAX_PFORCES);
  clearArray(l_PRTData.PSubSys,MAX_PSUBSYS);

  l_PRTData.rflag = LUX_FALSE;
  l_PRTData.emitter = LUX_FALSE;
  l_PRTData.particle = LUX_FALSE;
  l_PRTData.color = LUX_FALSE;
  l_PRTData.tex = LUX_FALSE;
  l_PRTData.force = LUX_FALSE;
  l_PRTData.subsys = LUX_FALSE;
  l_PRTData.visbox = LUX_FALSE;

  l_PRTData.activeColor = 0;
  l_PRTData.activeTex = 0;
  l_PRTData.activeForce = 0;
  l_PRTData.activeSubsys = 0;

  // default rflag
  psys->container.renderflag |= RENDER_NODEPTHMASK;
  psys->l3dlayer = LIST3D_LAYERS-1;
  ParticlePointParams_default(&psys->container.pparams);

  FileParse_setAnnotationList(&l_PRTData.psys->annotationListHead);
  FileParse_start(fPRT,l_defPRT);

  ParticleSysBuildFromBuffer(psys);

  dlprintf("\tParticleCount: %d\n\tColors: %d Textures: %d\n",psys->container.numParticles,psys->life.numColors,psys->life.numTex);
  dlprintf("\tForces: %d SubSystems: %d\n\n",psys->numForces,psys->numSubsys);
  FS_close (fPRT);

  return LUX_TRUE;
}

// RFlag
// -----

void loadPRFlag(void * target,char * command, char* rest)
{
  char buffer[1000];
  int bufferpos=0,restpos=0;
  ParticleSys_t *psys = (ParticleSys_t*)target;

  if (strcmp(command,"fog")==0) {
    psys->container.renderflag |= RENDER_FOG;
    return;
  }
  if (strcmp(command,"nodraw")==0) {
    psys->psysflag |= PARTICLE_NODRAW;
    return;
  }
  if (strcmp(command,"nocull")==0) {
    psys->container.renderflag |= RENDER_NOCULL;
    return;
  }
  if (strcmp(command,"lit")==0) {
    psys->container.renderflag |= RENDER_LIT;
    return;
  }
  if (strcmp(command,"depthmask")==0) {
    psys->container.renderflag &= ~RENDER_NODEPTHMASK;
    return;
  }
  if (strcmp(command,"nodepthtest")==0) {
    psys->container.renderflag |= RENDER_NODEPTHTEST;
    return;
  }
  if (strcmp(command,"alphamask")==0) {
    psys->container.renderflag |= RENDER_ALPHATEST;
    psys->container.alpha.alphaval = 0.0f;
    psys->container.alpha.alphafunc = GL_GREATER;
    return;
  }
  if (strcmp(command,"sort")==0){
    psys->psysflag |= PARTICLE_SORT;
    return;
  }
  if (strcmp(command,"novistest")==0){
    psys->psysflag |= PARTICLE_NOVISTEST;
    return;
  }
  if (strcmp(command,"blendmode")==0) {
    char src[100];
    char dst[100];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    if(sscanf(buffer,"%s",&src,&dst))
      if (strcmp(src,"VID_MODULATE")==0)
        psys->container.blend.blendmode = VID_MODULATE;
      else if (strcmp(src,"VID_MODULATE2")==0)
        psys->container.blend.blendmode = VID_MODULATE2;
      else if (strcmp(src,"VID_DECAL")==0)
        psys->container.blend.blendmode = VID_DECAL;
      else if (strcmp(src,"VID_ADD")==0)
        psys->container.blend.blendmode = VID_ADD;
      else if (strcmp(src,"VID_AMODADD")==0)
        psys->container.blend.blendmode = VID_AMODADD;

    if (psys->container.blend.blendmode)
      psys->container.renderflag |= RENDER_BLEND;

    return;
  }
  if (strcmp(command,"blendinvertalpha")== 0){
    psys->container.blend.blendinvert = LUX_TRUE;
    return;
  }

  if (strcmp(command,"layer")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%d",&psys->l3dlayer);
    psys->l3dlayer %= LIST3D_LAYERS;
    return;
  }
  if (strcmp(command,"alphafunc")==0) {
    char src[100] = {0};

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;


    if (sscanf(buffer,"%s (%f)",&src,&psys->container.alpha.alphaval) == 2)
      if (strcmp(src,"GL_NEVER")==0)
        psys->container.alpha.alphafunc = GL_NEVER;
      else if (strcmp(src,"GL_ALWAYS")==0)
        psys->container.alpha.alphafunc = GL_ALWAYS;
      else if (strcmp(src,"GL_LESS")==0)
        psys->container.alpha.alphafunc = GL_LESS;
      else if (strcmp(src,"GL_LEQUAL")==0)
        psys->container.alpha.alphafunc = GL_LEQUAL;
      else if (strcmp(src,"GL_GEQUAL")==0)
        psys->container.alpha.alphafunc = GL_GEQUAL;
      else if (strcmp(src,"GL_GREATER")==0)
        psys->container.alpha.alphafunc = GL_GREATER;
      else if (strcmp(src,"GL_EQUAL")==0)
        psys->container.alpha.alphafunc = GL_EQUAL;
      else if (strcmp(src,"GL_NOTEQUAL")==0)
        psys->container.alpha.alphafunc = GL_NOTEQUAL;

    if (psys->container.alpha.alphafunc)
      psys->container.renderflag |= RENDER_ALPHATEST;

    return;
  }
}
void fileLoadPRTRFlag()
{
  lxFSFile_t  *file = l_PRTData.file;
  static char *words[100]={ "lit","depthmask","nodepthtest","alphamask","sort","novistest","alphafunc","blendmode","fog","nodraw","nocull","layer","\0"};
  FileParse_stage(file,words,loadPRFlag,l_PRTData.psys);
}


// Emitter
// -------

void loadPEmitter(void * target,char * command, char* rest)
{
  char buffer[1000];
  int bufferpos=0,restpos=0;
  ParticleSys_t *psys = (ParticleSys_t*)target;

  if (strcmp(command,"type")==0) {
    char src[100];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%s",&src);


    if (strcmp(src,"VID_POINT")==0)
      psys->emitterDefaults.emittertype = PARTICLE_EMITTER_POINT;
    else if (strcmp(src,"VID_CIRCLE")==0)
      psys->emitterDefaults.emittertype = PARTICLE_EMITTER_CIRCLE;
    else if (strcmp(src,"VID_SPHERE")==0)
      psys->emitterDefaults.emittertype = PARTICLE_EMITTER_SPHERE;
    else if (strcmp(src,"VID_MODEL")==0)
      psys->emitterDefaults.emittertype = PARTICLE_EMITTER_MODEL;
    else if (strcmp(src,"VID_RECTANGLE")==0)
      psys->emitterDefaults.emittertype = PARTICLE_EMITTER_RECTANGLE;
    else if (strcmp(src,"VID_RECTANGLELOCAL")==0)
      psys->emitterDefaults.emittertype = PARTICLE_EMITTER_RECTANGLELOCAL;

    return;
  }


  if (strcmp(command,"size")==0 || strcmp(command,"width")==0 ) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f",&psys->emitterDefaults.size);
    return;
  }
  if (strcmp(command,"height")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f",&psys->emitterDefaults.size2);
    return;
  }
  if (strcmp(command,"axis")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%d",&psys->emitterDefaults.axis);
    psys->emitterDefaults.axis %= 3;
    return;
  }
  if (strcmp(command,"velocityvar")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f",&psys->emitterDefaults.velocityVar);
    return;
  }
  if (strcmp(command,"velocity")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f",&psys->emitterDefaults.velocity);
    return;
  }
  if (strcmp(command,"flipdirection")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f",&psys->emitterDefaults.flipdirection);
    return;
  }
  if (strcmp(command,"spread")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"(%f,%f)",&psys->emitterDefaults.spreadin,&psys->emitterDefaults.spreadout);
    if (psys->emitterDefaults.spreadout < psys->emitterDefaults.spreadout)
      lxSwapFloat(&psys->emitterDefaults.spreadout,&psys->emitterDefaults.spreadin);
    // turn them into radians
    psys->emitterDefaults.spreadin = LUX_DEG2RAD(psys->emitterDefaults.spreadin);
    psys->emitterDefaults.spreadout = LUX_DEG2RAD(psys->emitterDefaults.spreadout);
    return;
  }

  if (strcmp(command,"endtime")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%d",&psys->emitterDefaults.duration);
    return;
  }
  if (strcmp(command,"restarttime")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%d",&psys->emitterDefaults.restartdelay);
    return;
  }
  if (strcmp(command,"restarts")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%d",&psys->emitterDefaults.restarts);
    return;
  }
  if (strcmp(command,"rate")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f",&psys->emitterDefaults.rate);
    psys->emitterDefaults.rate /= 1000;
    return;
  }
  if (strcmp(command,"maxoffsetdist")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f",&psys->emitterDefaults.offset);
    return;
  }
  if (strcmp(command,"count")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%d",&psys->container.numParticles);
    if (psys->container.numParticles > MAX_PARTICLES)
      psys->container.numParticles = MAX_PARTICLES;
    return;
  }

  if (strcmp(command,"model")==0) {
    char src[256];
    char name[256];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%s",&src);
    lxStrReadInQuotes(src, name, 255);
    resnewstrcpy(psys->emitterDefaults.modelname,src);
    return;
  }
}

void fileLoadPRTEmitter()
{
  lxFSFile_t  *file = l_PRTData.file;
  static char *words[100]={ "type","size","endtime","restarttime","restarts","rate","model","count",
                "maxoffsetdist","spread","velocityvar","velocity","flipdirection,",
                "axis","width","height","\0"};
  FileParse_stage(file,words,loadPEmitter,l_PRTData.psys);
}

// ParticleDyn_t
// --------

void loadParticle(void * target,char * command, char* rest)
{
  char buffer[1000];
  int bufferpos=0,restpos=0;
  ParticleSys_t *psys = (ParticleSys_t*)target;

  if (strcmp(command,"rotatevelocity")==0) {
    psys->life.lifeflag |= PARTICLE_ROTVEL;
    return;
  }
  if (strcmp(command,"noagedeath")==0){
    psys->psysflag |= PARTICLE_NODEATH;
    return;
  }
  if (strcmp(command,"dieonfrontplane")==0){
    psys->life.lifeflag |= PARTICLE_DIECAMPLANE;
    return;
  }
  if (strcmp(command,"pointsmooth")==0){
    psys->container.contflag |= PARTICLE_POINTSMOOTH;
    return;
  }
  if (strcmp(command,"camrotfix")==0) {
    psys->container.contflag |= PARTICLE_CAMROTFIX;
    return;
  }
  if (strcmp(command,"turbulence")==0) {
    psys->life.lifeflag |= PARTICLE_TURB;
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f",&psys->life.turb);
    return;
  }
  if (strcmp(command,"type")==0) {
    char src[100];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%s",&src);

    if (strcmp(src,"VID_POINT")==0)
      psys->container.particletype = PARTICLE_TYPE_POINT;
    else if (strcmp(src,"VID_QUAD")==0)
      psys->container.particletype = PARTICLE_TYPE_QUAD;
    else if (strcmp(src,"VID_HSPHERE")==0)
      psys->container.particletype = PARTICLE_TYPE_HSPHERE;
    else if (strcmp(src,"VID_SPHERE")==0)
      psys->container.particletype = PARTICLE_TYPE_SPHERE;
    else if (strcmp(src,"VID_MODEL")==0)
      psys->container.particletype = PARTICLE_TYPE_MODEL;
    else if (strcmp(src,"VID_TRIANGLE")==0)
      psys->container.particletype = PARTICLE_TYPE_TRIANGLE;

    return;
  }
  if (strcmp(command,"originoffset")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;


    if (3 == sscanf(buffer,"(%f,%f,%f)",  &psys->container.userOffset[0],&psys->container.userOffset[1],&psys->container.userOffset[2]))
      psys->container.contflag |= PARTICLE_ORIGINOFFSET;
    return;
  }
  if (strcmp(command,"sizevar")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    psys->life.lifeflag |= PARTICLE_SIZEVAR;

    sscanf(buffer,"%f",&psys->life.sizeVar);
    return;
  }
  if (strcmp(command,"sizeage3")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f (%f) %f (%f) %f (%f)",&psys->life.sizeAgeTimes[0],&psys->life.sizeAge[0],&psys->life.sizeAgeTimes[1],&psys->life.sizeAge[1],&psys->life.sizeAgeTimes[2],&psys->life.sizeAge[2]);
    psys->life.sizeAgeTimes[0]/=100;
    psys->life.sizeAgeTimes[1]/=100;
    psys->life.sizeAgeTimes[2]/=100;
    psys->life.lifeflag |= PARTICLE_SIZEAGE;
    return;
  }
  if (strcmp(command,"sizeagetex")==0) {
    char name[256];
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;


    sscanf(lxStrReadInQuotes(buffer, name, 255),"%d (%f) %f",&psys->life.agetexRows[PARTICLE_AGETEX_SIZE],&psys->life.sizeAge[0],&psys->life.sizeAge[1]);
    resnewstrcpy(psys->life.agetexNames[PARTICLE_AGETEX_SIZE],name);
    psys->life.lifeflag |= PARTICLE_SIZEAGE;
    return;
  }
  if (strcmp(command,"size")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f",&psys->life.size);
    return;
  }
  if (strcmp(command,"rotateage3")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f (%f) %f (%f) %f (%f)",&psys->life.rotAgeTimes[0],&psys->life.rotAge[0],&psys->life.rotAgeTimes[1],&psys->life.rotAge[1],&psys->life.rotAgeTimes[2],&psys->life.rotAge[2]);
    psys->life.rotAgeTimes[0]/=100;
    psys->life.rotAgeTimes[1]/=100;
    psys->life.rotAgeTimes[2]/=100;
    psys->life.lifeflag |= PARTICLE_ROTAGE;
    return;
  }
  if (strcmp(command,"rotateagetex")==0) {
    char name[256];
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;


    sscanf(lxStrReadInQuotes(buffer, name, 255),"%d (%f) %f",&psys->life.agetexRows[PARTICLE_AGETEX_ROT],&psys->life.rotAge[0],&psys->life.rotAge[1]);
    resnewstrcpy(psys->life.agetexNames[PARTICLE_AGETEX_ROT],name);
    psys->life.lifeflag |= PARTICLE_ROTAGE;
    return;
  }
  if (strcmp(command,"speedage3")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f (%f) %f (%f) %f (%f)",&psys->life.velAgeTimes[0],&psys->life.velAge[0],&psys->life.velAgeTimes[1],&psys->life.velAge[1],&psys->life.velAgeTimes[2],&psys->life.velAge[2]);
    psys->life.velAgeTimes[0]/=100;
    psys->life.velAgeTimes[1]/=100;
    psys->life.velAgeTimes[2]/=100;
    psys->life.lifeflag |= PARTICLE_VELAGE;
    return;
  }
  if (strcmp(command,"speedagetex")==0) {
    char name[256];
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;


    sscanf(lxStrReadInQuotes(buffer, name, 255),"%d (%f) %f",&psys->life.agetexRows[PARTICLE_AGETEX_VEL],&psys->life.velAge[0],&psys->life.velAge[1]);
    resnewstrcpy(psys->life.agetexNames[PARTICLE_AGETEX_VEL],name);
    psys->life.lifeflag |= PARTICLE_VELAGE;
    return;
  }
  if (strcmp(command,"instancemesh")==0) {
    char name[256];
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    lxStrReadInQuotes(buffer, name, 255);
    resnewstrcpy(psys->container.instmeshname,name);

    return;
  }
  if (strcmp(command,"lifevar")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    psys->life.lifeflag |= PARTICLE_LIFEVAR;

    sscanf(buffer,"%f",&psys->life.lifeTimeVar);
    return;
  }
  if (strcmp(command,"rotateoffset")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    psys->life.lifeflag |= PARTICLE_ROTOFFSET;

    sscanf(buffer,"%f",&psys->life.rotateoffset);
    psys->life.rotateoffset = LUX_DEG2RAD(psys->life.rotateoffset);
    return;
  }
  if (strcmp(command,"rotatevar")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    psys->life.lifeflag |= PARTICLE_ROTVAR;

    sscanf(buffer,"%f",&psys->life.rotatevar);
    return;
  }
  if (strcmp(command,"rotate")==0) {
    char src[100];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%f %s",&psys->life.rotate,&src);
    psys->life.rotate = LUX_DEG2RAD(psys->life.rotate);
    psys->life.lifeflag |= PARTICLE_ROTATE;

    if (strcmp(src,"VID_POS")==0)
      psys->life.rotatedir = 1;
    if (strcmp(src,"VID_NEG")==0)
      psys->life.rotatedir = -1;
    if (strcmp(src,"VID_RAND")==0)
      psys->life.rotatedir = 0;

    return;
  }
  if (strcmp(command,"life")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%d",&psys->life.lifeTime);
    return;
  }

  if (strcmp(command,"model")==0) {
    char src[256];
    char name[256];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    sscanf(buffer,"%s",&src);
    lxStrReadInQuotes(src, name, 255);
    resnewstrcpy(psys->life.modelname,name);
    return;
  }

  if (strcmp(command,"pointparams")==0) {
    ParticlePointParams_t *params;
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;


    params = &psys->container.pparams;
    sscanf(buffer,"(%f,%f)(%f)(%f,%f,%f)",&params->min,&params->max,&params->alphathresh,&params->dist[0],&params->dist[1],&params->dist[2]);
  }



}
void fileLoadPRTParticle()
{
  lxFSFile_t  *file = l_PRTData.file;
  static char *words[100]={ "type","size","sizevar","sizeage3","count","life","lifevar","model",
          "pointsmooth","dieonfrontplane","rotate","rotatevar","rotateoffset","rotatevelocity","rotateage3",
          "pointparams","speedage3","noagedeath","camrotfix","rotateagetex","sizeagetex","speedagetex","originoffset",
          "instancemesh","\0"};
  FileParse_stage(file,words,loadParticle,l_PRTData.psys);
}


// Color
// -----

void loadPColor(void * target,char * command, char* rest)
{
  char buffer[1000];
  int bufferpos=0,restpos=0;
  ParticleSys_t *psys = (ParticleSys_t*)target;

  if (strcmp(command,"RGBA")==0) {
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    if (psys->life.numColors && l_PRTData.activeColor < psys->life.numColors){

      sscanf(buffer,"%f (%f,%f,%f,%f)",&psys->life.colorTimes[l_PRTData.activeColor],
        &psys->life.colors[l_PRTData.activeColor*4],
        &psys->life.colors[l_PRTData.activeColor*4+1],
        &psys->life.colors[l_PRTData.activeColor*4+2],
        &psys->life.colors[l_PRTData.activeColor*4+3]);

      if (l_PRTData.eventimed){
        psys->life.colorTimes[l_PRTData.activeColor] = (float)((100/psys->life.numColors)*l_PRTData.activeColor);
      }

      psys->life.colorTimes[l_PRTData.activeColor]*=0.01f;

      l_PRTData.activeColor++;
    }
    else
      lprintf("WARNING prtload: missing numcolor or all colors set\n");

    return;
  }
  if (strcmp(command,"RGBAagetex")==0) {
    char name[256];
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;


    sscanf(lxStrReadInQuotes(buffer, name, 255),"%d",&psys->life.agetexRows[PARTICLE_AGETEX_COLOR]);
    resnewstrcpy(psys->life.agetexNames[PARTICLE_AGETEX_COLOR],name);
    psys->life.numColors = 128;
    l_PRTData.activeColor = 128;
    return;
  }

  if (strcmp(command,"interpolate")==0) {
    psys->life.lifeflag |= PARTICLE_INTERPOLATE;
    return;
  }
  if (strcmp(command,"noage")==0) {
    psys->life.lifeflag |= PARTICLE_NOCOLAGE;
    return;
  }
  if (strcmp(command,"eventimed")==0){
    l_PRTData.eventimed = LUX_TRUE;
    return;
  }
  if (strcmp(command,"RGBAvar")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    psys->life.lifeflag |= PARTICLE_COLVAR;
    sscanf(buffer,"(%f,%f,%f,%f)",&psys->life.colorVar[0],&psys->life.colorVar[1],&psys->life.colorVar[2],&psys->life.colorVar[3]);
    return;
  }

  if (strcmp(command,"numcolor")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"%d",&psys->life.numColors);
    psys->life.colorTimes = reszalloc(sizeof(uint)*psys->life.numColors);
    psys->life.colors = reszalloc(sizeof(float)*4*psys->life.numColors);
    return;
  }
}
void fileLoadPRTColor()
{
  lxFSFile_t  *file = l_PRTData.file;
  static char *words[100]={ "RGBA","numcolor","RGBAvar","RGBAagetex","interpolate","noage","eventimed","\0"};
  l_PRTData.eventimed = LUX_FALSE;
  FileParse_stage(file,words,loadPColor,l_PRTData.psys);
}


// Texture_t
// -------

void loadPTex(void * target,char * command, char* rest)
{
  char buffer[1000];
  int bufferpos=0,restpos=0;
  ParticleSys_t *psys = (ParticleSys_t*)target;
  int tex;
  int etime;

  tex = 0;

  if (strcmp(command,"TEX")==0){
    tex = TEX_COLOR;
    etime = 0;
  }
  else if (strcmp(command,"TEXALPHA")==0){
    tex = TEX_ALPHA;
    etime = 0;
  }
  else if (strcmp(command,"MATERIAL")==0){
    tex = TEX_MATERIAL;
    etime = 1;
  }
  else if (strcmp(command,"TEXCOMBINE1D")==0){
    tex = TEX_2D_COMBINE1D;
    etime = 1;
  }
  else if (strcmp(command,"TEXCOMBINE2D_16")==0){
    tex = TEX_2D_COMBINE2D_16;
    etime = 1;
  }
  else if (strcmp(command,"TEXCOMBINE2D_64")==0){
    tex = TEX_2D_COMBINE2D_64;
    etime = 1;
  }

  if (tex) {
    char src[256];
    char name[256];

    psys->life.texType = tex;

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    if (psys->life.numTex && l_PRTData.activeTex < psys->life.numTex){

      sscanf(buffer,"%f %s",&psys->life.texTimes[l_PRTData.activeTex],&src);

      if (l_PRTData.eventimed){
        psys->life.texTimes[l_PRTData.activeTex] = (float)((100/psys->life.numTex)*l_PRTData.activeTex);
      }

      psys->life.texTimes[l_PRTData.activeTex]*=0.01;
      lxStrReadInQuotes(src, name, 255);
      resnewstrcpy(psys->life.texNames[l_PRTData.activeTex],name);
      l_PRTData.activeTex++;
    }
    else
      lprintf("WARNING prtload: missing numtex\n");

    if (l_PRTData.eventimed && etime){
      for (;l_PRTData.activeTex < psys->life.numTex; l_PRTData.activeTex++){
        psys->life.texTimes[l_PRTData.activeTex] = (float)((100/psys->life.numTex)*l_PRTData.activeTex);
        psys->life.texTimes[l_PRTData.activeTex]*=0.01f;
      }
    }

    return;
  }
  if (strcmp(command,"eventimed")==0){
    l_PRTData.eventimed = LUX_TRUE;
    return;
  }
  if (strcmp(command,"noage")==0) {
    psys->life.lifeflag |= PARTICLE_NOTEXAGE;
    return;
  }
  if (strcmp(command,"sequence")==0) {
    psys->life.lifeflag |= PARTICLE_TEXSEQ;
    return;
  }
  if (strcmp(command,"numtex")==0) {

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;
    sscanf(buffer,"%d",&psys->life.numTex);
    psys->life.texTimes = reszalloc(sizeof(uint)*psys->life.numTex);
    psys->life.texNames = reszalloc(sizeof(char*)*psys->life.numTex);
    return;
  }
  if (strcmp(command,"agetex")==0) {
    char name[256];
    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;


    sscanf(lxStrReadInQuotes(buffer, name, 255),"%d",&psys->life.agetexRows[PARTICLE_AGETEX_TEX]);
    resnewstrcpy(psys->life.agetexNames[PARTICLE_AGETEX_TEX],name);
    return;
  }
}

void fileLoadPRTTexture()
{
  lxFSFile_t  *file = l_PRTData.file;
  static char *words[100]={ "TEX","TEXALPHA","MATERIAL","TEXCOMBINE1D","TEXCOMBINE2D_16","TEXCOMBINE2D_64","numtex","noage","sequence","eventimed","agetex","\0"};

  l_PRTData.eventimed = LUX_FALSE;
  FileParse_stage(file,words,loadPTex,l_PRTData.psys);

  if (l_PRTData.activeTex < l_PRTData.psys->life.numTex)
    l_PRTData.psys->life.numTex = l_PRTData.activeTex;
}

// Force
// -----
void loadPForce(void * target,char * command, char* rest)
{
  char buffer[1024]= {0};
  int bufferpos=0,restpos=0;
  ParticleForce_t *pf;

  if (strcmp(command,"gravity")==0) {

    if (l_PRTData.activeForce == MAX_PFORCES){
      lprintf("WARNING prtload: too many forces\n");
      return;
    }

    pf = l_PRTData.PForces[l_PRTData.activeForce] = reszalloc(sizeof(ParticleForce_t));
    pf->linkType = LINK_UNSET;

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    pf->forcetype = PARTICLE_FORCE_GRAVITY;
    sscanf(buffer,"(%f,%f,%f) %f %d %d",&pf->vec3[0],&pf->vec3[1],&pf->vec3[2],&pf->turb,&pf->starttime,&pf->endtime);
    lxStrReadInQuotes(buffer,pf->name,PARTICLE_FORCE_NAMELENGTH);


    ParticleSys_addForce(l_PRTData.psys,pf);

    l_PRTData.activeForce++;
    return;
  }
  if (strcmp(command,"wind")==0) {

    if (l_PRTData.activeForce == MAX_PFORCES){
      lprintf("WARNING prtload: too many forces\n");
      return;
    }

    pf = l_PRTData.PForces[l_PRTData.activeForce] = reszalloc(sizeof(ParticleForce_t));
    pf->linkType = LINK_UNSET;

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    pf->forcetype = PARTICLE_FORCE_WIND;
    sscanf(buffer,"(%f,%f,%f) %f %f %d %d",&pf->vec3[0],&pf->vec3[1],&pf->vec3[2],&pf->turb,&pf->flt,&pf->starttime,&pf->endtime);
    lxStrReadInQuotes(buffer,pf->name,PARTICLE_FORCE_NAMELENGTH);


    ParticleSys_addForce(l_PRTData.psys,pf);

    l_PRTData.activeForce++;
    return;
  }
}
void fileLoadPRTForce()
{
  lxFSFile_t  *file = l_PRTData.file;
  static char *words[100]={ "gravity","wind","\0"};
  FileParse_stage(file,words,loadPForce,l_PRTData.psys);
}

// SubSystem
// ---------
void loadPSubsys(void * target,char * command, char* rest)
{
  char buffer[1000];
  int bufferpos=0,restpos=0;
  ParticleSubSys_t *psub;

  if (strcmp(command,"combinedraw")==0){
    l_PRTData.psys->psysflag |= PARTICLE_COMBDRAW;
    return;
  }
  if (strcmp(command,"trail")==0) {
    char src[256];
    char name[256];
    char dst[100];
    if (l_PRTData.activeSubsys == MAX_PSUBSYS){
      lprintf("WARNING prtload: too many subsystems\n");
      return;
    }

    psub = &l_PRTData.PSubSys[l_PRTData.activeSubsys];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    //subpos  0=stride 1=minage 2=dir


    psub->subsystype = PARTICLE_SUB_TRAIL;
    sscanf(buffer,"%s %d %s %f %f ",&src,&psub->delay,&dst,&psub->pos[1],&psub->pos[0]);
    lxStrReadInQuotes(src, name, 255);
    resnewstrcpy(psub->name,name);

    // default to opposite direction
    psub->pos[2] = -1;
    if (strcmp(dst,"VID_DIR")==0)
      psub->pos[2] = 1;
    if (strcmp(dst,"VID_ODIR")==0)
      psub->pos[2] = -1;

    l_PRTData.activeSubsys++;
    return;
  }
  if (strcmp(command,"normal")==0) {
    char src[256];
    char name[256];
    if (l_PRTData.activeSubsys == MAX_PSUBSYS){
      lprintf("WARNING prtload: too many subsystems\n");
      return;
    }

    psub = &l_PRTData.PSubSys[l_PRTData.activeSubsys];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    psub->subsystype = PARTICLE_SUB_NORMAL;
    sscanf(buffer,"%s %d",&src,&psub->delay);
    lxStrReadInQuotes(src, name, 255);
    resnewstrcpy(psub->name,name);

    l_PRTData.activeSubsys++;
    return;
  }
  if (strcmp(command,"translated")==0) {
    char src[256];
    char name[256];
    if (l_PRTData.activeSubsys == MAX_PSUBSYS){
      lprintf("WARNING prtload: too many subsystems\n");
      return;
    }

    psub = &l_PRTData.PSubSys[l_PRTData.activeSubsys];

    while (rest[restpos]!=0 && rest[restpos]!=';')
      buffer[bufferpos++]=rest[restpos++];
    buffer[bufferpos]=0;

    psub->subsystype = PARTICLE_SUB_TRANSLATED;
    sscanf(buffer,"%s %d (%f,%f,%f) (%f,%f,%f)",
      &src,
      &psub->delay,
      &psub->pos[0],&psub->pos[1],&psub->pos[2],
      &psub->dir[0],&psub->dir[1],&psub->dir[2]);
    lxStrReadInQuotes(src, name, 255);
    resnewstrcpy(psub->name,name);

    l_PRTData.activeSubsys++;
    return;
  }

}

void fileLoadPRTSubSys()
{
  lxFSFile_t  *file = l_PRTData.file;
  static char *words[100]={ "trail","normal","translated","combinedraw","\0"};
  FileParse_stage(file,words,loadPSubsys,l_PRTData.psys);
}


// Build from Buffer
// -----------------

void ParticleSysBuildFromBuffer(ParticleSys_t *psys)
{
  int i;


  if (l_PRTData.activeForce){
    psys->psysflag |= PARTICLE_FORCES;
    psys->numForces = l_PRTData.activeForce;
    for (i = 0; i < l_PRTData.activeForce; i++)
      psys->forces[i] = l_PRTData.PForces[i];
  }
  if (l_PRTData.activeSubsys){
    psys->psysflag |= PARTICLE_SUBSYS;
    psys->subsys = reszalloc(sizeof(ParticleSubSys_t)*l_PRTData.activeSubsys);
    psys->numSubsys = l_PRTData.activeSubsys;
    for ( i = 0; i < l_PRTData.activeSubsys; i++)
      memcpy(&psys->subsys[i],&l_PRTData.PSubSys[i],sizeof(ParticleSubSys_t));
  }
  if (!(psys->psysflag & PARTICLE_SUBSYS) && psys->psysflag & PARTICLE_COMBDRAW)
    psys->psysflag &= ~PARTICLE_COMBDRAW;

  if (l_PRTData.activeTex)
    psys->psysflag |= PARTICLE_TEXTURED;

  ParticleSys_init(psys);
}
