// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include <luxinia/luxcore/contstringmap.h>
#include <time.h>

#include "vid.h"
#include "../common/3dmath.h"
#include "../main/main.h"
#include "../resource/resmanager.h"
#include "../common/memorymanager.h"
#include "../render/gl_render.h"
#include "../render/gl_particle.h"

#ifdef LUX_SIMD
//#include "_cpuid.h"
#endif


// GLOBALS
VID_t   g_VID;

// LOCALS
#define EXTENSION_MAX_COUNT   128

static struct VIDData_s{
  char  extensionList[EXTENSION_MAX_COUNT][80];
  int   extensionNum;
  lxStrMapPTR alphamodes;
  lxStrMapPTR colormodes;
  lxStrMapPTR userdefines;
  lxStrMapPTR cgconnectors;
  lxStrMapPTR cgconnectorsemantics;
  char  screenshotname[MAX_PATHLENGTH];
}l_VIDData;


extern int fileSaveTGA(char * filename, short int width, short int height, int channels, unsigned char *imageBuffer, int isbgra);
extern int fileSaveJPG(char * filename, short int width, short int height, int channels, int quality, unsigned char *imageBuffer);
extern void Console_printf (char *key,...);


///////////////////////////////////////////////////////////////////////////////
// VID
static void _addExtension(const char *s){
  strcpy(l_VIDData.extensionList[l_VIDData.extensionNum++],s);
  LUX_ASSERT(l_VIDData.extensionNum < EXTENSION_MAX_COUNT);
}


//////////////////////////////////////////////////////////////////////////
// Error Handling

int vidCgCheckErrorF(char *filename,int line){
  CGerror error;
  const char *str;
  int haderror = 0;
  while ( (error = cgGetError()) != GL_NO_ERROR) {
    str = cgGetErrorString(error);
    haderror = LUX_TRUE;
    LUX_PRINTF( "ERROR: Cg [%s:%d] failed with %s\n", filename,line, str );
  }
  return haderror;
}
int vidCheckErrorF(char *filename,int line){
  GLenum error;
  const GLubyte *str;
  int haderror = 0;
  while ( (error = glGetError()) != GL_NO_ERROR) {
    str = gluErrorString(error);
    haderror = LUX_TRUE;
    LUX_PRINTF( "ERROR: OpenGL [%s:%d] failed with %s\n", filename,line, str );
  }
  return haderror;
}

//////////////////////////////////////////////////////////////////////////
// Helpers
static void vidTwoSideStencilMask (uint mask)
{
  glActiveStencilFaceEXT(GL_FRONT);
  glStencilMask(mask);
  glActiveStencilFaceEXT(GL_BACK);
  glStencilMask(mask);
  glActiveStencilFaceEXT(GL_FRONT);
}
static void vidSimpleStencilMask ( uint mask)
{
  glStencilMask(mask);
}

//////////////////////////////////////////////////////////////////////////
// VID

void VID_clear()
{
  int i;
  int w = g_VID.windowWidth;
  int h = g_VID.windowHeight;

  memset(&g_VID,0,sizeof(VID_t));
  memset(&l_VIDData,0,sizeof(struct VIDData_s));

  for (i = 0; i < VID_VPROGS; i++){
    g_VID.gensetup.vprogRIDs[i] = -1;
  }
  for( i = 0; i < VID_LOGOS; i++)
    g_VID.gensetup.logoRIDs[i] = -1;
  // shader
  g_VID.shdsetup.lightmapRID = -1;
  g_VID.gensetup.normalizeRID = -1;
  g_VID.gensetup.attenuate3dRID = -1;
  g_VID.gensetup.specularRID = -1;
  g_VID.gensetup.diffuseRID = -1;
  g_VID.gensetup.whiteRID = -1;

  vidResetStencil();

  g_VID.gensetup.hwbonesparams = 24;
  g_VID.gensetup.hwbones = 0;
  g_VID.windowWidth = w;
  g_VID.windowHeight = h;
  g_VID.drawbufferWidth = w;
  g_VID.drawbufferHeight = h;
  g_VID.refScreenWidth = 640.0f;
  g_VID.refScreenHeight = 480.0f;

  g_VID.useTexCompress = LUX_TRUE;
  g_VID.gensetup.batchMaxIndices = 1024;
  g_VID.gensetup.batchMeshMaxTris = 32;

  lxMatrix44Identity(g_VID.drawsetup.worldMatrixCache);

  lxVector4Set(g_VID.black,0,0,0,0);
  lxVector4Set(g_VID.white,1,1,1,1);

  g_VID.cg.allowglsl = LUX_TRUE;
  g_VID.swapinterval = 0;
  g_VID.maxFps = 60;
}
void VID_deinitGL(){
  int i;

  vidBindBufferArray(NULL);
  vidBindBufferIndex(NULL);
  vidUnSetProgs();
  vidStateDefault();
  {
    int stack;
    glMatrixMode(GL_MODELVIEW);
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&stack);
    while (stack-- > 1){
      glScalef(1,1,1);
      glPopMatrix();
    }
    glLoadMatrixf(lxMatrix44GetIdentity());
    glScalef(1,1,1);
    glLoadIdentity();
  }

  Mesh_typeVA(g_VID.drw_meshquad);
  Mesh_typeVA(g_VID.drw_meshquadffx);
  Mesh_typeVA(g_VID.drw_meshquadcentered);
  Mesh_typeVA(g_VID.drw_meshquadcenteredffx);
  Mesh_typeVA(g_VID.drw_meshbuffer);
  Mesh_typeVA(g_VID.drw_meshbox);
  Mesh_typeVA(g_VID.drw_meshsphere);
  Mesh_typeVA(g_VID.drw_meshcylinder);

  g_VID.state.activeFragment = -1;
  g_VID.state.activeVertex = -1;

  g_VID.gensetup.hasVprogs = LUX_FALSE;
  // shader
  for( i = 0; i < VID_LOGOS; i++)
    g_VID.gensetup.logoRIDs[i] = -1;
  g_VID.shdsetup.lightmapRID = -1;
  g_VID.gensetup.normalizeRID = -1;
  g_VID.gensetup.attenuate3dRID = -1;
  g_VID.gensetup.specularRID = -1;
  g_VID.gensetup.whiteRID = -1;
  g_VID.gensetup.diffuseRID = -1;

  g_VID.drawsetup.lightCntLast = 0;
  for (i = 0; i < VID_MAX_LIGHTS; i++){
    g_VID.drawsetup.light[i] = 0;
    g_VID.drawsetup.lightPt[i] = NULL;
    g_VID.drawsetup.lightPtGL[i] = NULL;
  }



  // particle
  if (g_VID.gensetup.quadobj)
    gluDeleteQuadric(g_VID.gensetup.quadobj);
  g_VID.gensetup.quadobj = NULL;
  for (i = 0; i < VID_VPROGS; i++)
    g_VID.gensetup.vprogRIDs[i] = -1;
  /*
  for (i = 0; i < VID_MAX_TEXTURE_UNITS;i++){
    if (g_VID.drw_texcoords[i])
      free(g_VID.drw_texcoords[i]);
    g_VID.drw_texcoords[i] = NULL;
  }
  */


  /*
  if (g_VID.cg.context)
    cgDestroyContext(g_VID.cg.context);
  g_VID.cg.context = NULL;
  */

  vidBindBufferArray(NULL);
  vidBindBufferIndex(NULL);
  vidUnSetProgs();
  vidStateDefault();

}

int VID_CheckSpecialTexs(const char *str){
  Texture_t *tex;
  int texs[6] = {
    g_VID.gensetup.normalizeRID,
    g_VID.gensetup.specularRID,
    g_VID.gensetup.diffuseRID,
    g_VID.gensetup.attenuate3dRID,
    g_VID.gensetup.whiteRID,
    g_VID.gensetup.texdefaultRID,
  };
  int i;
  for (i = 0; i < 6; i++){
    if ((tex=ResData_getTexture(texs[i])) && strcmp(str,tex->resinfo.name)==0){
      return texs[i];
    }
  }
  return -1;
}

void VID_initGL()
{
  int i;
  int texcompress;
  VID_deinitGL();
  vidResetTexture();
  vidResetPointer();

  // PARTICLE SPECIFC
  if (!g_VID.gensetup.quadobj)
    g_VID.gensetup.quadobj = gluNewQuadric();

  // SHADER SPECIFIC
  // atlhoguh we dont use vbo here, its just to find out how "new" the hardware is
  // to prevent software emulation of vertex programs

  ResData_CoreChunk(LUX_TRUE);
  if (GLEW_ARB_vertex_program && GLEW_ARB_vertex_buffer_object && GLEW_ARB_texture_cube_map){
    bprintf("... load gpuprogs\n");
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_MAX_PROGRAM_PARAMETERS_ARB,&g_VID.gensetup.hwbones);
    // fix for latest cards
    g_VID.gensetup.hwbones = g_VID.gensetup.hwbones < 256 ? 96 : 256;
    g_VID.gensetup.hwbones -= g_VID.gensetup.hwbonesparams;
    g_VID.gensetup.hwbones/=3;


    g_VID.gensetup.hasVprogs = !g_VID.novprogs;
    if (!g_VID.novprogs){

      g_VID.gensetup.vprogRIDs[VID_VPROG_SKIN_WEIGHT_UNLIT]=ResData_addGpuProg("skin_weight_unlit.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_SKIN_WEIGHT_LIT1] =ResData_addGpuProg("skin_weight_lit1.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_SKIN_WEIGHT_LIT2] =ResData_addGpuProg("skin_weight_lit2.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_SKIN_WEIGHT_LIT3] =ResData_addGpuProg("skin_weight_lit3.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_SKIN_WEIGHT_LIT4] =ResData_addGpuProg("skin_weight_lit4.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);

      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_BATCH] =    ResData_addGpuProg("particle_batch.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_BATCH_ROT] =  ResData_addGpuProg("particle_batch_rotated.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_BATCH_ROT_GLOBALAXIS] =ResData_addGpuProg("particle_batch_rotated_globalaxis.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_BATCH_LM] =   ResData_addGpuProg("particle_batch_lm.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_BATCH_ROT_LM] = ResData_addGpuProg("particle_batch_rotated_lm.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_BATCH_ROT_GLOBALAXIS_LM] =ResData_addGpuProg("particle_batch_rotated_globalaxis_lm.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);

      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_INSTANCE]         = ResData_addGpuProg("particle_instance.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_INSTANCE_LIT]       = ResData_addGpuProg("particle_instance_lit.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_INSTANCE_GLOBALAXIS]    = ResData_addGpuProg("particle_instance_globalaxis.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_INSTANCE_GLOBALAXIS_LIT]  = ResData_addGpuProg("particle_instance_globalaxis_lit.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);

      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_INSTANCE_LM]        = ResData_addGpuProg("particle_instance_lm.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_INSTANCE_LIT_LM]      = ResData_addGpuProg("particle_instance_lit_lm.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_INSTANCE_GLOBALAXIS_LM]   = ResData_addGpuProg("particle_instance_globalaxis_lm.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
      g_VID.gensetup.vprogRIDs[VID_VPROG_PARTICLE_INSTANCE_GLOBALAXIS_LIT_LM] = ResData_addGpuProg("particle_instance_globalaxis_lit_lm.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);

      for (i = 0; i < VID_VPROG_OPTIONAL_PROGS; i++){
        if (g_VID.gensetup.vprogRIDs[i] == -1){
          g_VID.gensetup.hasVprogs = LUX_FALSE;
          break;
        }
      }
    }

    g_VID.gensetup.vprogRIDs[VID_VPROG_BUMP_POSINV_UNLIT] =   ResData_addGpuProg("bump_posinv_unlit.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
    g_VID.gensetup.vprogRIDs[VID_VPROG_BUMP_POSINV_LIT1] =    ResData_addGpuProg("bump_posinv_lit1.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
    g_VID.gensetup.vprogRIDs[VID_VPROG_BUMP_POSINV_LIT2] =    ResData_addGpuProg("bump_posinv_lit2.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
    g_VID.gensetup.vprogRIDs[VID_VPROG_BUMP_POSINV_LIT3] =    ResData_addGpuProg("bump_posinv_lit3.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
    g_VID.gensetup.vprogRIDs[VID_VPROG_BUMP_POSINV_LIT4] =    ResData_addGpuProg("bump_posinv_lit4.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);

    g_VID.gensetup.vprogRIDs[VID_VPROG_PROJECTOR] =   ResData_addGpuProg("projector.vp",GPUPROG_V_ARB,LUX_TRUE,NULL);
  }
  else{
    g_VID.gensetup.hasVprogs = LUX_FALSE;
  }

  vidCheckErrorF(__FILE__,__LINE__);

  texcompress = g_VID.useTexCompress;
  g_VID.useTexCompress = LUX_FALSE;
  bprintf("... load textures\n");
  if(GLEW_ARB_texture_cube_map){
    g_VID.gensetup.normalizeRID = ResData_addTexture("?normalizeCUBE",TEX_CUBE_NORMALIZE,0);
    g_VID.gensetup.specularRID = ResData_addTexture("?specularCUBE",TEX_CUBE_SPECULAR,0);
    g_VID.gensetup.diffuseRID = ResData_addTexture("?diffuseCUBE",TEX_CUBE_DIFFUSE,0);
  }
  if ( GLEW_ARB_texture_cube_map && GLEW_EXT_texture3D)
    g_VID.gensetup.attenuate3dRID = ResData_addTexture("?attenuate3D",TEX_3D_ATTENUATE,0);

  g_VID.gensetup.logoRIDs[0] = ResData_addTexture((char*)g_LuxiniaWinMgr.luxiGetCustomStrings("logobig"),TEX_COLOR,TEX_ATTR_MIPMAP);
  g_VID.gensetup.logoRIDs[1] = ResData_addTexture((char*)g_LuxiniaWinMgr.luxiGetCustomStrings("logosmall"),TEX_COLOR,TEX_ATTR_MIPMAP);
  g_VID.gensetup.whiteRID = ResData_addTexture("?white2D",TEX_2D_WHITE,0);
  g_VID.gensetup.texdefaultRID = ResData_addTexture("?default2D",TEX_2D_DEFAULT,0);
  ResData_CoreChunk(LUX_FALSE);

  g_VID.useTexCompress = texcompress;
  g_VID.state.renderflag = VIDRenderFlag_getGL();

  // DRAW SPECIFIC

  bprintf("... prep state\n");
  vidBindBufferArray(NULL);
  vidBindBufferIndex(NULL);
  vidUnSetProgs();
  vidStateDefault();

  {
    int stack;
    glMatrixMode(GL_MODELVIEW);
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&stack);
    while (stack-- > 1){
      glScalef(1,1,1);
      glPopMatrix();
    }
    glLoadMatrixf(lxMatrix44GetIdentity());
    glScalef(1,1,1);
    glLoadIdentity();
  }

  bprintf("... create dls\n");
  Mesh_typeDL(g_VID.drw_meshquad,g_VID.capTexUnits,LUX_TRUE,0,LUX_TRUE);
  Mesh_typeDL(g_VID.drw_meshquadffx,g_VID.capTexUnits,LUX_TRUE,0,LUX_TRUE);
  Mesh_typeDL(g_VID.drw_meshbox,g_VID.capTexUnits,LUX_TRUE,0,LUX_TRUE);
  Mesh_typeDL(g_VID.drw_meshsphere,g_VID.capTexUnits,LUX_TRUE,0,LUX_TRUE);
  Mesh_typeDL(g_VID.drw_meshcylinder,g_VID.capTexUnits,LUX_TRUE,0,LUX_TRUE);
  Mesh_typeDL(g_VID.drw_meshquadcentered,g_VID.capTexUnits,LUX_TRUE,0,LUX_TRUE);
  Mesh_typeDL(g_VID.drw_meshquadcenteredffx,g_VID.capTexUnits,LUX_TRUE,0,LUX_TRUE);

  bprintf("... reset state\n");
  vidBindBufferArray(NULL);
  vidBindBufferIndex(NULL);
  vidUnSetProgs();
  vidStateDefault();
  bprintf("... end\n");
}

void VIDCg_setConnector(const char *name, CGparameter host)
{
  if (!host){
    lxStrMap_remove(l_VIDData.cgconnectors,name);
  }
  else
    lxStrMap_set(l_VIDData.cgconnectors,name,(void*)host);
}

CGparameter VIDCg_getConnector(const char *name)
{
  return (CGparameter)lxStrMap_get(l_VIDData.cgconnectors,name);
}

static void* VIDCg_ConnectorIterator(void *data, const char *name, void* browse)
{
  CGprogram cgprog = (CGprogram)data;
  CGparameter cgconnector = (CGparameter) browse;
  CGparameter cgparam = cgGetNamedParameter(cgprog,name);
  if (cgparam)
    cgConnectParameter(cgconnector,cgparam);

  vidCgCheckErrorF(__FILE__,__LINE__);

  return data;
}


void VIDCg_linkProgramConnectors(CGprogram cgprog)
{
  lxStrMap_iterate(l_VIDData.cgconnectors, VIDCg_ConnectorIterator , (void*)cgprog);
}

void VIDCg_setSemantic(CGparameter host,const char *semantics)
{
  char namebuffer[256];
  const char *c = semantics;
  char *o = namebuffer;
  char *name = o;

  while (c[0])
  {
    booln zero = LUX_FALSE;
    if (name && ((zero=(c[0] == ',' || c[0] == ' ')) || c[1] == 0)){
      o[0] = zero ? 0 : c[0];
      o[1] = 0;

      lxStrToLower(namebuffer,namebuffer);
      lxStrMap_set(l_VIDData.cgconnectorsemantics,namebuffer,(void*)host);
      name = NULL;
    }
    else if (c[0] != ' ' && !name){
      name = namebuffer;
      o = name;
    }
    if (name){
      o[0] = c[0];
      o++;
    }
    c++;
  }
}

void VIDCg_linkProgramSemantics(CGprogram cgprog, CGenum name_space)
{
  static char namebuffer[256];
  CGparameter cgparam = cgGetFirstParameter( cgprog, name_space);
  CGparameter cgconnector;
  CGparameter cgparamleaf = cgGetFirstLeafParameter(cgprog, name_space);

  while ( cgparam )
  {
    if (!cgGetConnectedParameter(cgparam)){
      const char *name = cgGetParameterName(cgparam);
      const char *sem = cgGetParameterSemantic(cgparam);
      if (sem && sem[0] && (cgconnector=(CGparameter)lxStrMap_get(l_VIDData.cgconnectorsemantics,lxStrToLower(namebuffer,sem)))
        && cgGetParameterType(cgparam) == cgGetParameterType(cgconnector))
      {
        cgConnectParameter(cgconnector,cgparam);
      }
    }
    cgparam = cgGetNextParameter( cgparam );
  }
  vidCgCheckErrorF(__FILE__,__LINE__);
}

void VIDCg_initProfiles()
{
  cgGLSetOptimalOptions(g_VID.cg.arbFragmentProfile);
  cgGLSetOptimalOptions(g_VID.cg.arbVertexProfile );

  if (cgGLIsProfileSupported(g_VID.cg.glslFragmentProfile)){
    cgGLSetOptimalOptions(g_VID.cg.glslFragmentProfile);
    cgGLSetOptimalOptions(g_VID.cg.glslVertexProfile);
  }


  cgGLSetOptimalOptions(g_VID.cg.vertexProfile);
  cgGLSetOptimalOptions(g_VID.cg.fragmentProfile);

  if(g_VID.cg.geometryProfile != CG_PROFILE_UNKNOWN)
    cgGLSetOptimalOptions(g_VID.cg.geometryProfile);

  g_VID.cg.optimalinited = LUX_TRUE;
}


void VID_initCg(booln glslsm3, booln glslsm4)
{
  // Setup Cg
  g_VID.cg.context = cgCreateContext();

  // Validate Our Context Generation Was Successful
  if (g_VID.cg.context && GLEW_ARB_vertex_program)
  {
#ifdef LUX_VID_CG_DEBUG
    cgGLSetDebugMode(CG_TRUE);
#else
    cgGLSetDebugMode(CG_FALSE);
#endif
    cgSetLockingPolicy(CG_NO_LOCKS_POLICY); // single-threaded
    cgSetParameterSettingMode(g_VID.cg.context, CG_DEFERRED_PARAMETER_SETTING);

    g_VID.cg.glslFragmentProfile = cgGetProfile("glslf");
    g_VID.cg.glslVertexProfile = cgGetProfile("glslv");

    g_VID.cg.arbFragmentProfile = cgGetProfile("arbfp1");
    g_VID.cg.arbVertexProfile = cgGetProfile("arbvp1");

    g_VID.cg.vertexProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
    g_VID.cg.fragmentProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
    g_VID.cg.geometryProfile = cgGLGetLatestProfile(CG_GL_GEOMETRY);

    g_VID.cg.useoptimal = LUX_TRUE;

    // we dont like NV extensions that are non ARB compatible
    // nor GLSL
    if (g_VID.cg.vertexProfile == cgGetProfile("vp20")
      || g_VID.cg.vertexProfile == cgGetProfile("vp30")
      || g_VID.cg.vertexProfile == cgGetProfile("glslv"))
    {
        g_VID.cg.vertexProfile = cgGetProfile("arbvp1");
    }
    if (g_VID.cg.fragmentProfile == cgGetProfile("fp30") ||
      g_VID.cg.fragmentProfile == cgGetProfile("glslf"))
    {
      g_VID.cg.vertexProfile = cgGetProfile("arbfp1");
    }

    // allow GLSL for sm3+
    if (g_VID.gpuvendor != VID_VENDOR_NVIDIA &&
      (glslsm3 || glslsm4))
    {
      g_VID.cg.vertexProfile = cgGetProfile("glslv");
      g_VID.cg.fragmentProfile = cgGetProfile("glslf");
    }

    if (  g_VID.cg.fragmentProfile == cgGetProfile("glslf")
      ||  g_VID.cg.vertexProfile == cgGetProfile("glslv"))
      g_VID.cg.GLSL = LUX_TRUE;
    else
      g_VID.cg.GLSL = LUX_FALSE;


    // Validate Our Profile Determination Was Successful
    if (g_VID.cg.vertexProfile != CG_PROFILE_UNKNOWN)
    {
      _addExtension("CG_SHADER_SUPPORT");
      _addExtension(cgGetProfileString(g_VID.cg.vertexProfile));


      if (g_VID.cg.fragmentProfile != CG_PROFILE_UNKNOWN && (GLEW_ARB_fragment_program || GLEW_ARB_fragment_shader)){
        _addExtension(cgGetProfileString(g_VID.cg.fragmentProfile));
      }
      if (g_VID.cg.geometryProfile != CG_PROFILE_UNKNOWN){
        _addExtension(cgGetProfileString(g_VID.cg.geometryProfile));
      }

    }

    g_VID.cg.optFragmentProfile = g_VID.cg.fragmentProfile;
    g_VID.cg.optVertexProfile =   g_VID.cg.vertexProfile;
    g_VID.cg.optGeometryProfile = g_VID.cg.geometryProfile;


    vidCgCheckError();


    l_VIDData.cgconnectors = lxStrMap_new(GLOBAL_ALLOCATOR,128,0,NULL);
    l_VIDData.cgconnectorsemantics = lxStrMap_new(GLOBAL_ALLOCATOR,128,0,NULL);

#define VIDCg_createParamArray(var,strname,cgtype,arraycnt,sem)  var = cgCreateParameterArray(g_VID.cg.context, cgtype, arraycnt); VIDCg_setConnector(strname , var); VIDCg_setSemantic(var,sem)
#define VIDCg_createParam(var,strname,cgtype,sem)        var = cgCreateParameter(g_VID.cg.context, cgtype); VIDCg_setConnector(strname , var); VIDCg_setSemantic(var,sem)


    VIDCg_createParamArray(g_VID.cg.boneMatrices,   "Bonesmatrices",  CG_FLOAT3x4,  g_VID.gensetup.hwbones, "BONESMATRICES, BONES");

    VIDCg_createParamArray(g_VID.cg.bbtexOffsets,   "PRTtexoffsets",  CG_FLOAT2,    GLPARTICLE_MAX_MESHVERTICES, "PRTTEXOFFSETS");
    VIDCg_createParamArray(g_VID.cg.bbvertexOffsets,  "PRTvertexoffsets", CG_FLOAT4,    GLPARTICLE_MAX_MESHVERTICES, "PRTVERTEXOFFSETS");

    VIDCg_createParam(g_VID.cg.prtlmTexgen,   "PRTlmtexgen",    CG_FLOAT3x4, "PRTLMTEXGEN");
    VIDCg_createParam(g_VID.cg.bbglobalAxis,  "PRTglobalaxis",  CG_FLOAT3x4, "PRTGLOBALAXIS");
    VIDCg_createParam(g_VID.cg.bbtexwidth,    "PRTtexwidth",    CG_FLOAT2,   "PRTTEXWIDTH");

    VIDCg_createParam(g_VID.cg.projMatrix,    "ProjMatrix",       CG_FLOAT4x4, "PROJ, PROJECTION, PROJECTIONMATRIX, PROJMATRIX");
    VIDCg_createParam(g_VID.cg.projMatrixInv, "ProjMatrixInv",      CG_FLOAT4x4, "PROJMATRIXINV, PROJINV, PROJECTIONINV, PROJINVERSE, PROJECTIONINVERSE, PROJINVMATRIX, PROJECTIONINVMATRIX, PROJINVERSEMATRIX, PROJECTIONINVERSEMATRIX");
    VIDCg_createParam(g_VID.cg.viewMatrix,    "ViewMatrix",       CG_FLOAT4x4, "VIEW, VIEWMATRIX");
    VIDCg_createParam(g_VID.cg.viewMatrixInv, "ViewMatrixInv",      CG_FLOAT4x4, "VIEWMATRIXINV, VIEWINV, VIEWINVERSE, VIEWINVERSEMATRIX, VIEWINVMATRIX");
    VIDCg_createParam(g_VID.cg.viewProjMatrix,  "ViewProjMatrix",     CG_FLOAT4x4, "VIEWPROJECTION, VIEWPROJ, VIEWPROJMATRIX, VIEWPROJECTIONMATRIX");

    VIDCg_createParam(g_VID.cg.worldMatrix,     "WorldMatrix",      CG_FLOAT4x4, "WORLD, WORLDMATRIX");
    VIDCg_createParam(g_VID.cg.worldViewMatrix,   "WorldViewMatrix",    CG_FLOAT4x4, "WORLDVIEW, WORLDVIEWMATRIX");
    VIDCg_createParam(g_VID.cg.worldViewProjMatrix, "WorldViewProjMatrix",  CG_FLOAT4x4, "WORLDVIEWPROJ, WORLDVIEWPROJECTION, WORLDVIEWPROJMATRIX, WORLDVIEWPROJECTIONMATRIX");

    VIDCg_createParam(g_VID.cg.fogcolor,  "FogColor",     CG_FLOAT3, "FOGCOLOR");
    VIDCg_createParam(g_VID.cg.fogdistance, "FogDistance",    CG_FLOAT3, "FOGDISTANCE");
    VIDCg_createParam(g_VID.cg.camwpos,   "CameraWorldPos", CG_FLOAT3, "CAMERAWORLDPOS, VIEWPOSITION, VIEWPOS");
    VIDCg_createParam(g_VID.cg.camwdir,   "CameraWorldDir", CG_FLOAT3, "CAMERAWORLDDIR, VIEWDIRECTION, VIEWDIR");

    VIDCg_createParam(g_VID.cg.viewsize,  "ViewportSize",   CG_FLOAT2, "VIEWPORTSIZE, VIEWPORTDIMENSION");
    VIDCg_createParam(g_VID.cg.viewsizeinv, "ViewportSizeInv",  CG_FLOAT2, "VIEWPORTSIZEINV, VIEWPORTSIZEINVERSE, VIEWPORTDIMENSIONINV, VIEWPORTDIMENSIONINVERSE, INVERSEVIEWPORTDIMENSIONS");

#undef VIDCg_createParam
#undef VIDCg_createParamArray


    vidCgCheckError();
  }
  else if (GLEW_ARB_vertex_program){
    bprintf("WARNING: No Cg Context\n");
  }
}

void VID_info(int start, void (*fnprint)(char *string, ...))
{
  int i=0;
  int num,num2;

  switch(start) {
  case 0:
    fnprint("  OS       : %s\n", VID_GetOSString());
    fnprint("  Renderer : %s\n", glGetString(GL_RENDERER));
    fnprint("  Vendor   : %s\n", glGetString(GL_VENDOR));
    fnprint("  Version  : %s\n", glGetString(GL_VERSION));
    fnprint("  VIDpath  : %s\n", VID_GPUVendor_toString(g_VID.gpuvendor));
    return;

  case 1:
    fnprint("  Extensions:\n");
    while (l_VIDData.extensionList[i][0]){
      fnprint("   %s\n",l_VIDData.extensionList[i]);
      i++;
    }
    return;
  default:
    break;
  }


  fnprint("  Capabilites:\n");
  fnprint("   Texture units : %d (of %d)\n",g_VID.capTexUnits,VID_MAX_TEXTURE_UNITS);
  fnprint("   Texture coords : %d \n",g_VID.capTexCoords);
  fnprint("   Texture images : %d (of %d)\n",g_VID.capTexImages,VID_MAX_TEXTURE_IMAGES);
  if (GLEW_ARB_vertex_program){
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_MAX_PROGRAM_INSTRUCTIONS_ARB,&num);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB,&num2);
    fnprint("   VertexProgram Instructions : %d, %d (native)\n",num,num2);
  }
  if (GLEW_ARB_fragment_program){
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,GL_MAX_PROGRAM_INSTRUCTIONS_ARB,&num);
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB,&num2);
    fnprint("   FragmentProgram Instructions : %d, %d (native)\n",num,num2);
  }

  if (g_VID.capBump) fnprint("   Dot3 Normal mapping\n");
  if (g_VID.capHWShadow) fnprint("   HW Shadow\n");
  if (g_VID.capTexAnisotropic) fnprint("   Anisotropy : %f\n",g_VID.capTexAnisotropic);
  fnprint("   Pointsize: %f\n",g_VID.capPointSize);
  fnprint("   Technique : %s\n",VIDTechnique_toString(g_VID.capTech));
  fnprint("  Useage Extras:\n");
  fnprint("   Extensions %s\n",       (!g_VID.useworst) ? "YES" : "NO");
  fnprint("   Default VertexPrograms %s\n", (!g_VID.novprogs) ? "YES" : "NO");
  fnprint("   VBOs %s\n",           (g_VID.usevbos) ? "YES" : "NO");
  fnprint("   TexCompress %s\n",        (g_VID.useTexCompress) ? "YES" : "NO");
  fnprint("   TexAnisotropic %s\n",     (g_VID.useTexAnisotropic) ? "YES" : "NO");
  fnprint("\n");
}


int VID_init()
{
  int i;
  const char *glext;
  const char *glrenderer;
  int ismesa = LUX_FALSE;
  int combiners = LUX_FALSE;
  float vec[4];
  int glslsm3;
  int glslsm4;

  for (i=0;i<EXTENSION_MAX_COUNT;i++)
    l_VIDData.extensionList[i][0] = 0;
  l_VIDData.extensionNum = 0;

  if (g_VID.maxFps  < 15 && g_VID.maxFps != 0)
    g_VID.maxFps = 15;

  glrenderer = (const char *)glGetString(GL_RENDERER);

  if (!glrenderer){
    bprintf("ERROR: renderer NOT FOUND\n" );
    bprintf("Sorry no GLContext bound, or incompatible hardware.\n");
    return LUX_FALSE;
  }
  // vendor paths
  {
    const char* vendor = glGetString(GL_VENDOR);
    if (lxStrFindAsLower((const char *)vendor,"nvidia")!= NULL)
      g_VID.gpuvendor = VID_VENDOR_NVIDIA;
    else if (lxStrFindAsLower((const char *)vendor,"intel")!= NULL)
      g_VID.gpuvendor = VID_VENDOR_INTEL;
    else if (lxStrFindAsLower((const char *)vendor,"ati ")!= NULL ||
      strcmp(vendor,"ATI")==0)
      g_VID.gpuvendor = VID_VENDOR_ATI;
    else
      g_VID.gpuvendor = VID_VENDOR_UNKNOWN;
  }

  g_VID.capIsCrap = g_VID.gpuvendor == VID_VENDOR_UNKNOWN || g_VID.gpuvendor == VID_VENDOR_INTEL;

  VID_info(0,LogPrintf);

  ismesa = strstr(glrenderer,"Mesa") != NULL;

  if (strstr((const char *)glGetString(GL_RENDERER),"GDI") && !ismesa) {
    bprintf("ERROR: proper renderer NOT FOUND\n" );
    bprintf("Graphics card OpenGL driver is not installed or you are running in 16-bit desktop mode. Found 'GDI' in renderer string.\n");
    return LUX_FALSE;
  }

  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&g_VID.capTexSize);
  glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE,&g_VID.capTex3DSize);

  // must-haves extensions
  _addExtension("Requirements:");
  if (GLEW_ARB_multitexture){
    _addExtension("GL_ARB_multitexture");
    glGetIntegerv( GL_MAX_TEXTURE_UNITS, &g_VID.capTexUnits );
  }
  else{
    bprintf("ERROR: GL_ARB_multitexture NOT FOUND\n" );
    bprintf("Sorry, your graphics card is either too old, or the drivers not up-to-date to run luxinia.\n");
    return LUX_FALSE;
  }

  if (GLEW_ARB_texture_env_combine)
    _addExtension("GL_ARB_texture_env_combine");
  else if (GLEW_EXT_texture_env_combine)
    _addExtension("GL_EXT_texture_env_combine");
  else{
    bprintf("ERROR: GL__texture_env_combine NOT FOUND\n" );
    bprintf("Sorry, your graphics card is either too old, or the drivers not up-to-date to run luxinia\n");
    return LUX_FALSE;
  }

  if (GLEW_ARB_texture_env_add)
    _addExtension("GL_ARB_texture_env_add");
  else if (GLEW_EXT_texture_env_add)
    _addExtension("GL_EXT_texture_env_add");
  else {
    bprintf("ERROR: GL__texture_env_add NOT FOUND\n" );
    bprintf("Sorry, your graphics card is either too old, or the drivers not up-to-date to run luxinia\n");
    return LUX_FALSE;
  }

  if (GLEW_EXT_blend_minmax){
    _addExtension("GL_EXT_blend_minmax");
  }
  else{
    bprintf("ERROR: GL_EXT_blend_minmax NOT FOUND\n" );
    bprintf("Sorry, your graphics card is either too old, or the drivers not up-to-date to run luxinia.\n");
    return LUX_FALSE;
  }

  if (GLEW_EXT_blend_subtract){
    _addExtension("GL_EXT_blend_subtract");
  }
  else{
    bprintf("ERROR: GL_EXT_blend_subtract NOT FOUND\n" );
    bprintf("Sorry, your graphics card is either too old, or the drivers not up-to-date to run luxinia.\n");
    return LUX_FALSE;
  }


  // nice-to-have extensions
  _addExtension("Enhancements:");
  if (GLEW_ARB_vertex_buffer_object && !g_VID.useworst){
    _addExtension("GL_ARB_vertex_buffer_object");
    g_VID.capVBO = LUX_TRUE;
  }
  else
    __GLEW_ARB_vertex_buffer_object = LUX_FALSE;
  if (GLEW_SGIS_generate_mipmap && !g_VID.useworst)
    _addExtension("GL_SGIS_generate_mipmap");
  else
    __GLEW_SGIS_generate_mipmap = LUX_FALSE;
  if (GLEW_NV_texture_env_combine4 && !g_VID.useworst)
    _addExtension("GL_NV_texture_env_combine4");
  else
    __GLEW_NV_texture_env_combine4 = LUX_FALSE;
  if (GLEW_ATI_texture_env_combine3 && !g_VID.useworst)
    _addExtension("GL_ATI_texture_env_combine3");
  else
    __GLEW_ATI_texture_env_combine3 = LUX_FALSE;
  if (GLEW_EXT_compiled_vertex_array && !g_VID.useworst)
    _addExtension("GL_EXT_compiled_vertex_array");
  else
    __GLEW_EXT_compiled_vertex_array = LUX_FALSE;
  if (GLEW_EXT_draw_range_elements && !g_VID.useworst)
    _addExtension("GL_EXT_draw_range_elements");
  else
    __GLEW_EXT_draw_range_elements = LUX_FALSE;
  if (GLEW_ARB_texture_cube_map && !g_VID.useworst)
    _addExtension("GL_ARB_texture_cube_map");
  else
    __GLEW_ARB_texture_cube_map = LUX_FALSE;
  if (GLEW_ARB_vertex_program && !g_VID.useworst)
    _addExtension("GL_ARB_vertex_program");
  else
    __GLEW_ARB_vertex_program = LUX_FALSE;
  if (GLEW_ARB_texture_env_dot3 && !g_VID.useworst)
    _addExtension("GL_ARB_texture_env_dot3");
  else
    __GLEW_ARB_texture_env_dot3 = LUX_FALSE;
  if (GLEW_ARB_texture_env_crossbar && !g_VID.useworst)
    _addExtension("GL_ARB_texture_env_crossbar");
  else
    __GLEW_ARB_texture_env_crossbar = LUX_FALSE;
  if (GLEW_EXT_texture_filter_anisotropic && !g_VID.useworst)
    _addExtension("GL_EXT_texture_filter_anisotropic");
  else
    __GLEW_EXT_texture_filter_anisotropic = LUX_FALSE;
  if (GLEW_ARB_point_sprite && !g_VID.useworst)
    _addExtension("GL_ARB_point_sprite");
  else
    __GLEW_ARB_point_sprite = LUX_FALSE;
  if (GLEW_ARB_point_parameters && !g_VID.useworst)
    _addExtension("GL_ARB_point_parameters");
  else
    __GLEW_ARB_point_parameters = LUX_FALSE;
  if (GLEW_ARB_fragment_program && !g_VID.useworst)
    _addExtension("GL_ARB_fragment_program");
  else
    __GLEW_ARB_fragment_program = LUX_FALSE;
  if (GLEW_ARB_texture_compression && !g_VID.useworst)
    _addExtension("GL_ARB_texture_compression");
  else
    __GLEW_ARB_texture_compression = LUX_FALSE;
  if (GLEW_EXT_texture_compression_s3tc && !g_VID.useworst)
    _addExtension("GL_EXT_texture_compression_s3tc");
  else
    __GLEW_EXT_texture_compression_s3tc = LUX_FALSE;
  if (GLEW_ARB_depth_texture && !g_VID.useworst)
    _addExtension("GL_ARB_depth_texture");
  else
    __GLEW_ARB_depth_texture = LUX_FALSE;
  if (GLEW_ARB_shadow && !g_VID.useworst)
    _addExtension("GL_ARB_shadow");
  else
    __GLEW_ARB_shadow = LUX_FALSE;
  if (GLEW_ARB_texture_border_clamp && !g_VID.useworst)
    _addExtension("GL_ARB_texture_border_clamp");
  else
    __GLEW_ARB_texture_border_clamp = LUX_FALSE;
  if (GLEW_EXT_bgra && !g_VID.useworst)
    _addExtension("GL_EXT_bgra");
  else
    __GLEW_EXT_bgra = LUX_FALSE;
  if (GLEW_EXT_abgr && !g_VID.useworst)
    _addExtension("GL_EXT_abgr");
  else
    __GLEW_EXT_abgr = LUX_FALSE;
  if (GLEW_EXT_stencil_two_side && !g_VID.useworst)
    _addExtension("GL_EXT_stencil_two_side");
  else
    __GLEW_EXT_stencil_two_side = LUX_FALSE;
  if (GLEW_ATI_separate_stencil && !g_VID.useworst)
    _addExtension("GL_ATI_separate_stencil");
  else
    __GLEW_ATI_separate_stencil = LUX_FALSE;
  if (GLEW_EXT_stencil_wrap && !g_VID.useworst)
    _addExtension("GL_EXT_stencil_wrap");
  else
    __GLEW_EXT_stencil_wrap = LUX_FALSE;
  if (GLEW_NV_depth_clamp && !g_VID.useworst)
    _addExtension("GL_NV_depth_clamp");
  else
    __GLEW_NV_depth_clamp = LUX_FALSE;

  if (GLEW_ARB_half_float_pixel && !g_VID.useworst)
    _addExtension("GL_ARB_half_float_pixel");
  else
    __GLEW_ARB_half_float_pixel = LUX_FALSE;

  if ((GLEW_ATI_texture_float || GLEW_ARB_texture_float ) && !g_VID.useworst){
    _addExtension("GL__texture_float");
    g_VID.capTexFloat = LUX_TRUE;
  }

  if ((GLEW_EXT_texture_buffer_object ) && !g_VID.useworst){
    _addExtension("GL_EXT_texture_buffer_object");
    g_VID.capTexBuffer = LUX_TRUE;
  }

  if ((GLEW_ARB_pixel_buffer_object ) && !g_VID.useworst){
    _addExtension("GL_ARB_pixel_buffer_object");
    g_VID.capPBO = LUX_TRUE;
  }

  if ((GLEW_EXT_texture_integer ) && !g_VID.useworst){
    _addExtension("GL_EXT_texture_integer");
    g_VID.capTexInt = LUX_TRUE;
  }

  if (GLEW_ARB_shader_objects ){
    _addExtension("GL_ARB_shader_objects");
  }
  if (GLEW_EXT_framebuffer_object && !g_VID.useworst){
    _addExtension("GL_EXT_framebuffer_object");
    g_VID.capFBO = LUX_TRUE;
  }
  else
    __GLEW_EXT_framebuffer_object = LUX_FALSE;

  if (GLEW_EXT_framebuffer_blit && !g_VID.useworst){
    _addExtension("GL_EXT_framebuffer_blit");
    g_VID.capFBOBlit = LUX_TRUE;
  }
  else
    __GLEW_EXT_framebuffer_blit = LUX_FALSE;

  if (GLEW_EXT_framebuffer_multisample && !g_VID.useworst){
    _addExtension("GL_EXT_framebuffer_multisample");
    glGetIntegerv(GL_MAX_SAMPLES_EXT,&g_VID.capFBOMSAA);
  }
  else
    __GLEW_EXT_framebuffer_multisample = LUX_FALSE;

  if (GLEW_EXT_packed_depth_stencil && !g_VID.useworst){
    _addExtension("GL_EXT_packed_depth_stencil");
  }
  else
    __GLEW_EXT_packed_depth_stencil = LUX_FALSE;

  if (GLEW_ARB_texture_non_power_of_two  && !g_VID.useworst)
    _addExtension("GL_ARB_texture_non_power_of_two");
  else
    __GLEW_ARB_texture_non_power_of_two = LUX_FALSE;

  if (GLEW_ARB_draw_buffers && !g_VID.useworst){
    _addExtension("GL_ARB_draw_buffers");
    glGetIntegerv(GL_MAX_DRAW_BUFFERS,&g_VID.capDrawBuffers);
  }
  else{
    g_VID.capDrawBuffers = 1;
  }

  if (GLEW_EXT_texture_array && !g_VID.useworst){
    _addExtension("GL_EXT_texture_array");
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS_EXT,&g_VID.capTexArray);
  }
  else{
    g_VID.capTexArray = 0;
  }

  if (GLEW_NV_transform_feedback && !g_VID.useworst){
    _addExtension("GL_NV_transform_feedback");
    g_VID.capFeedback = LUX_TRUE;
  }
  if (GLEW_ARB_copy_buffer && !g_VID.useworst){
    _addExtension("GL_ARB_copy_buffer");
    g_VID.capCPYBO = LUX_TRUE;
  }

  g_VID.defaultBuffers[VID_BUFFER_VERTEX] =
  g_VID.defaultBuffers[VID_BUFFER_INDEX] =
    g_VID.capVBO ? (void*)-1 : 0;
  g_VID.defaultBuffers[VID_BUFFER_PIXELINTO] =
  g_VID.defaultBuffers[VID_BUFFER_PIXELFROM] =
    g_VID.capPBO ? (void*)-1 : 0;
  g_VID.defaultBuffers[VID_BUFFER_TEXTURE] =
    g_VID.capTexBuffer ? (void*)-1 : 0;
  g_VID.defaultBuffers[VID_BUFFER_FEEDBACK] =
    g_VID.capFeedback? (void*)-1 : 0;
  g_VID.defaultBuffers[VID_BUFFER_CPYINTO] =
  g_VID.defaultBuffers[VID_BUFFER_CPYFROM] =
    g_VID.capCPYBO? (void*)-1 : 0;

  vidResetBuffers();

  glslsm3 = LUX_FALSE;
  if (GLEW_ARB_fragment_shader && GLEW_ARB_vertex_shader){
    char buffer[1024];
    const char *vsource =
      "uniform mat4 in_matrix;\n"
      "uniform vec4 in_offsets[16];\n"
      "attribute vec4 in_pos;\n"
      "attribute float in_cnt;\n"
      "void main()\n"
      "{  vec4 pos = in_pos;\n"
      " for (int i=0; i < int(in_cnt) && i < 16; i++){\n"
      "   pos += in_offsets[i];\n"
      " }\n"
      " gl_Position = in_matrix *  pos;\n"
      "}"
      ;
    const char *fsource =
      "uniform vec4 offsets[64];\n"
      "varying vec4 in_pos;\n"
      "varying float in_cnt;\n"
      "void main()\n"
      "{  vec4 pos = in_pos;\n"
      " for (int i=0; i < int(in_cnt); i++){\n"
      "   pos += offsets[i];\n"
      " }\n"
      " gl_FragColor = pos;\n"
      "}"
      ;

    GLuint vshd = glCreateShader(GL_VERTEX_SHADER);
    GLuint fshd = glCreateShader(GL_FRAGMENT_SHADER);
    GLint vstate;
    GLint fstate;
    GLsizei outlen;

    glShaderSource(vshd,1,&vsource,NULL);
    glShaderSource(fshd,1,&fsource,NULL);

    glCompileShader(vshd);
    glCompileShader(fshd);

    glGetShaderInfoLog(vshd,1024,&outlen,buffer);
    glGetShaderInfoLog(fshd,1024,&outlen,buffer);

    glGetShaderiv(vshd,GL_COMPILE_STATUS,&vstate);
    glGetShaderiv(fshd,GL_COMPILE_STATUS,&fstate);

    glDeleteShader(vshd);
    glDeleteShader(fshd);

    glslsm3 = vstate && fstate;
  }

  glslsm3 = g_VID.cg.allowglsl ? glslsm3 : LUX_FALSE;
  glslsm4 = GLEW_EXT_gpu_shader4 ;
  glslsm4 = g_VID.cg.allowglsl ? glslsm4 : LUX_FALSE;

  // fix software emulated vertex programs
  // RADEON 7xxx
  // Radeon
  // GeForce 256
  // GeForce2
  // GeForce4 MX
  // GeForce4 4xx Go
  if  (GLEW_ARB_vertex_program){
    const char *name = (const char *)glGetString(GL_RENDERER);
    const char *search = NULL;
    int software = LUX_FALSE;
    GLint num;

    if (((search=strstr(name,"Radeon")) || (search=strstr(name,"RADEON"))) && search[7] == '7')
      software = LUX_TRUE;
    else if (strstr(name,"GeForce 256") || strstr(name,"GeForce2") || strstr(name,"GeForce4 MX"))
      software = LUX_TRUE;
    else if ((search=strstr(name,"GeForce4 4")) && search[13] == 'G')
      software = LUX_TRUE;

    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB,&num);

    if (software || !num){
      if (!g_VID.configset)
        g_VID.novprogs = LUX_TRUE;
      lprintf("WARNING: Vertex GpuPrograms will run in software mode\n");
    }
  }


  if (GLEW_ATI_separate_stencil || GLEW_EXT_stencil_two_side)
    g_VID.capTwoSidedStencil = LUX_TRUE;

  if (GLEW_ARB_shadow && GLEW_ARB_depth_texture)
    g_VID.capHWShadow = LUX_TRUE;

  if (GLEW_EXT_texture_compression_s3tc && GLEW_ARB_texture_compression)
    g_VID.capTexCompress = LUX_TRUE;

  if (g_VID.capTexCompress && GLEW_EXT_bgra && GLEW_EXT_abgr)
    g_VID.capTexDDS = LUX_TRUE;

  // ATI doesnt support NV_texture_rectangle ...
  glext = (const char *)glGetString (GL_EXTENSIONS);
  if (strstr(glext,"GL_EXT_texture_rectangle") || strstr(glext,"GL_ARB_texture_rectangle"))
    __GLEW_NV_texture_rectangle = LUX_TRUE;

  if (GLEW_NV_texture_rectangle)
    _addExtension("GL_NV_texture_rectangle");

  g_VID.gensetup.stencilmaskfunc = vidSimpleStencilMask;
  if (GLEW_EXT_stencil_two_side)
    g_VID.gensetup.stencilmaskfunc = vidTwoSideStencilMask;



  VID_initCg(glslsm3,glslsm4);

  VID_info(1,LogPrintf);

  // OVERRIDES FOR TESTING
  if (g_VID.useworst){
    g_VID.capTexUnits = g_VID.capTexImages = 2;
    g_VID.gpuvendor = VID_VENDOR_UNKNOWN;
    g_VID.capIsCrap = LUX_TRUE;
  }



  // check if we can do bump mapping
  if (GLEW_ARB_texture_env_dot3 && GLEW_ARB_vertex_program && GLEW_ARB_texture_cube_map && (GLEW_ARB_texture_env_crossbar || GLEW_NV_texture_env_combine4))
    g_VID.capBump = 1;

  if (GLEW_ARB_fragment_program || GLEW_ARB_fragment_shader){
    glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &g_VID.capTexImages );
    glGetIntegerv( GL_MAX_TEXTURE_COORDS_ARB, &g_VID.capTexCoords );
  }
  else
    g_VID.capTexImages = g_VID.capTexCoords =  g_VID.capTexUnits;

  if (GLEW_ARB_vertex_shader){
    glGetIntegerv( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &g_VID.capTexVertexImages );
  }
  else
    g_VID.capTexVertexImages = 0;


  if (g_VID.force2tex)
    g_VID.capTexImages = g_VID.capTexCoords =  g_VID.capTexUnits = 2;


  if (g_VID.capTexUnits > VID_MAX_TEXTURE_UNITS)
    g_VID.capTexUnits = VID_MAX_TEXTURE_UNITS;
  if (g_VID.capTexImages > VID_MAX_TEXTURE_IMAGES)
    g_VID.capTexImages = VID_MAX_TEXTURE_IMAGES;

  if (GLEW_ARB_texture_cube_map && GLEW_ARB_texture_env_combine && (GLEW_ARB_texture_env_crossbar || GLEW_NV_texture_env_combine4) && GLEW_ARB_texture_env_dot3)
    combiners = LUX_TRUE;

  if (GLEW_ARB_vertex_program && GLEW_ARB_fragment_program)
    g_VID.capTech = VID_T_ARB_VF;
  else if (GLEW_ARB_vertex_program && combiners)
    g_VID.capTech = VID_T_ARB_V;
  else if (combiners)
    g_VID.capTech = VID_T_ARB_TEXCOMB;
  else
    g_VID.capTech = VID_T_DEFAULT;

  // yummy shader model 3
  if (g_VID.capTexCoords >=8){
    if (g_VID.capTech == VID_T_ARB_VF)
      g_VID.capTech = VID_T_ARB_VF_TEX8;
    if (g_VID.capTexImages >= 16 && (GLEW_NV_vertex_program3 || glslsm3))
      g_VID.capTech = VID_T_CG_SM3;
    if (g_VID.capTech == VID_T_CG_SM3 && (g_VID.capTexImages >= 32 || g_VID.capTexImages == VID_MAX_TEXTURE_IMAGES) && (g_VID.capTexArray && (glslsm4 || GLEW_NV_gpu_program4)))
      g_VID.capTech = VID_T_CG_SM4;
#ifdef LUX_VID_GEOSHADER
    if (g_VID.capTech == VID_T_CG_SM4 && (GLEW_NV_gpu_program4 || GLEW_EXT_geometry_shader4))
      g_VID.capTech = VID_T_CG_SM4_GS;
#endif
  }

  g_VID.origTech = g_VID.capTech;


  switch (g_VID.details){
    case VID_DETAIL_HI:
    case VID_DETAIL_MED:
      break;
    case VID_DETAIL_LOW:
      g_VID.capTech = VID_T_LOWDETAIL;
      break;
    default:
      g_VID.details = VID_DETAIL_HI;
  }

  // default to false for non ati/nvi
  if (GLEW_ARB_vertex_buffer_object && !g_VID.configset)
    g_VID.usevbos = (g_VID.capTech > VID_T_ARB_VF && g_VID.capTech != VID_T_LOWDETAIL
      && (g_VID.gpuvendor == VID_VENDOR_ATI || g_VID.gpuvendor == VID_VENDOR_NVIDIA)) ? LUX_TRUE : LUX_FALSE;
  else if (!GLEW_ARB_vertex_buffer_object)
    g_VID.usevbos = LUX_FALSE;

//  Variable_setSaveMode(
//    Environment_registerVar("system_useVBOs", PVariable_newBool((char*)&g_VID.usevbos)),1);

  g_VID.capTexAnisotropic = 0;
  if (GLEW_EXT_texture_filter_anisotropic && g_VID.details >= VID_DETAIL_MED){
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&g_VID.capTexAnisotropic);
    if (g_VID.lvlTexAnisotropic < 1){
      if (g_VID.details == VID_DETAIL_MED)
        g_VID.lvlTexAnisotropic = (g_VID.capTexAnisotropic+1)*0.5;
      else
        g_VID.lvlTexAnisotropic = g_VID.capTexAnisotropic;
    }
    g_VID.lvlTexAnisotropic = LUX_MIN(g_VID.capTexAnisotropic,g_VID.lvlTexAnisotropic);
  }

  glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE,vec);
  g_VID.capPointSize = vec[1];
  glGetFloatv(GL_SMOOTH_POINT_SIZE_RANGE,vec);
  g_VID.capPointSize = LUX_MAX(vec[1],g_VID.capPointSize);

  VID_info(2,LogPrintf);

  bprintf("... base meshes\n");
  lxVector4Set(g_VID.drawsetup.skyrot,90,1,0,0);
  lxMatrix44Identity(g_VID.drawsetup.skyrotMatrix);
  lxMatrix44FromAngleAxisFast(g_VID.drawsetup.skyrotMatrix,LUX_DEG2RAD(g_VID.drawsetup.skyrot[0]),&g_VID.drawsetup.skyrot[1]);
  lxMatrix44Identity(g_VID.drawsetup.viewSkyrotMatrix);

  g_VID.drw_meshquad = Mesh_newQuad(0);
  g_VID.drw_meshquadffx = Mesh_newQuad(1);
  g_VID.drw_meshbuffer = Mesh_newBuffer();
  g_VID.drw_meshbox = Mesh_newBox();
  g_VID.drw_meshsphere = Mesh_newSphere();
  g_VID.drw_meshcylinder = Mesh_newCylinder();
  g_VID.drw_meshquadcentered = Mesh_newQuadCentered(0);
  g_VID.drw_meshquadcenteredffx = Mesh_newQuadCentered(1);
  Mesh_createTangents(g_VID.drw_meshquad);
  Mesh_createTangents(g_VID.drw_meshquadffx);
  Mesh_createTangents(g_VID.drw_meshbuffer);
  Mesh_createTangents(g_VID.drw_meshbox);
  Mesh_createTangents(g_VID.drw_meshsphere);
  Mesh_createTangents(g_VID.drw_meshcylinder);
  Mesh_createTangents(g_VID.drw_meshquadcentered);

  bprintf("... blendmodes\n");
  l_VIDData.alphamodes = lxStrMap_new(GLOBAL_ALLOCATOR,128,0,NULL);
  l_VIDData.colormodes = lxStrMap_new(GLOBAL_ALLOCATOR,128,0,NULL);
  // fill maps with names
  for (i = 0; i < VID_COLOR_USER; i++)
    lxStrMap_set(l_VIDData.colormodes,VIDBlendColor_toString((VIDBlendColor_t)i),(void*)i);
  for (i = 0; i < VID_ALPHA_USER; i++)
    lxStrMap_set(l_VIDData.alphamodes,VIDBlendAlpha_toString((VIDBlendAlpha_t)i),(void*)i);

  l_VIDData.userdefines = lxStrMap_new(GLOBAL_ALLOCATOR,128,0,NULL);

  // capibilities
  if (GLEW_EXT_texture3D)
    VIDUserDefine_add("CAP_TEX3D");
  if (GLEW_NV_texture_env_combine4)
    VIDUserDefine_add("CAP_COMBINE4");
  if (GLEW_NV_texture_env_combine4 || GLEW_ATI_texture_env_combine3)
    VIDUserDefine_add("CAP_MODADD");
  if (g_VID.capTexFloat)
    VIDUserDefine_add("CAP_TEXFLOAT");
  if (GLEW_ARB_point_sprite)
    VIDUserDefine_add("CAP_POINTSPRITES");

  bprintf("... hashes\n");
  g_VID.state.texhashdefaults[VID_TEXBLEND_MOD_MOD] =
    VIDTexBlendHash_get(VID_MODULATE,VID_A_MODULATE,LUX_FALSE,0,0);
  g_VID.state.texhashdefaults[VID_TEXBLEND_MOD_REP] =
    VIDTexBlendHash_get(VID_MODULATE,VID_A_REPLACE,LUX_FALSE,0,0);
  g_VID.state.texhashdefaults[VID_TEXBLEND_REP_REP] =
    VIDTexBlendHash_get(VID_REPLACE,VID_A_REPLACE,LUX_FALSE,0,0);
  g_VID.state.texhashdefaults[VID_TEXBLEND_REP_MOD] =
    VIDTexBlendHash_get(VID_REPLACE,VID_A_MODULATE,LUX_FALSE,0,0);
  g_VID.state.texhashdefaults[VID_TEXBLEND_PREV_REP] =
    VIDTexBlendHash_get(VID_REPLACE_PREV,VID_A_REPLACE,LUX_FALSE,0,0);
  g_VID.state.texhashdefaults[VID_TEXBLEND_MOD_PREV] =
    VIDTexBlendHash_get(VID_MODULATE,VID_A_REPLACE_PREV,LUX_FALSE,0,0);
  g_VID.state.texhashdefaults[VID_TEXBLEND_MOD2_PREV] =
    VIDTexBlendHash_get(VID_MODULATE,VID_A_REPLACE_PREV,LUX_FALSE,1,0);
  g_VID.state.texhashdefaults[VID_TEXBLEND_MOD4_PREV] =
    VIDTexBlendHash_get(VID_MODULATE,VID_A_REPLACE_PREV,LUX_FALSE,2,0);

  bprintf("... state\n");
  VIDState_getGL();
  vidCheckErrorF(__FILE__,__LINE__);
  bprintf("... GL resources\n");
  VID_initGL();
  vidCheckErrorF(__FILE__,__LINE__);

  lxMatrix44Identity(g_VID.drawsetup.texwindowMatrix);

  g_VID.gensetup.zpass.enabled = LUX_TRUE;
  g_VID.gensetup.zfail.enabled = LUX_TRUE;
  g_VID.gensetup.zpass.refvalue = g_VID.gensetup.zfail.refvalue = 0;
  g_VID.gensetup.zpass.funcB = g_VID.gensetup.zpass.funcF = g_VID.gensetup.zfail.funcB = g_VID.gensetup.zfail.funcF = GL_ALWAYS;
  g_VID.gensetup.zpass.mask = g_VID.gensetup.zfail.mask= 255;
  g_VID.gensetup.zpass.ops[0].zpass = (GLEW_EXT_stencil_wrap) ? GL_INCR_WRAP : GL_INCR ;
  g_VID.gensetup.zpass.ops[0].fail =  GL_KEEP;
  g_VID.gensetup.zpass.ops[0].zfail = GL_KEEP;
  g_VID.gensetup.zpass.ops[1].zpass = (GLEW_EXT_stencil_wrap) ? GL_DECR_WRAP : GL_DECR ;
  g_VID.gensetup.zpass.ops[1].fail =  GL_KEEP;
  g_VID.gensetup.zpass.ops[1].zfail = GL_KEEP;

  g_VID.gensetup.zfail.ops[0].fail =  GL_KEEP;
  g_VID.gensetup.zfail.ops[0].zfail = (GLEW_EXT_stencil_wrap) ? GL_DECR_WRAP : GL_DECR ;
  g_VID.gensetup.zfail.ops[0].zpass =  GL_KEEP;
  g_VID.gensetup.zfail.ops[1].fail =  GL_KEEP;
  g_VID.gensetup.zfail.ops[1].zfail = (GLEW_EXT_stencil_wrap) ? GL_INCR_WRAP : GL_INCR ;
  g_VID.gensetup.zfail.ops[1].zpass =  GL_KEEP;

  g_VID.gensetup.shadowvalue = 0;

  // setup screenshot
  bprintf("... setup screenshot\n");
  {
    time_t aclock;
    char sshotname[64];
    time(&aclock);
    strftime(sshotname,64,"%Y%m%d%H%M%S",localtime(&aclock));
    strncpy(l_VIDData.screenshotname,g_LuxiniaWinMgr.luxiGetCustomStrings("pathscreenshot"),sizeof(char)*MAX_PATHLENGTH);
    strncat(l_VIDData.screenshotname,"/",(sizeof(char)*MAX_PATHLENGTH)-strlen(l_VIDData.screenshotname));
    strncat(l_VIDData.screenshotname,sshotname,(sizeof(char)*MAX_PATHLENGTH)-strlen(l_VIDData.screenshotname));
  }


  g_VID.screenshotquality = 95;

  bprintf("... get VRAM\n");
  g_VID.capVRAM = g_LuxiniaWinMgr.luxiGetVideoRam();

  bprintf("... end\n");
  return LUX_TRUE;
}

void VID_deinit()
{
  g_VID.drw_meshbuffer->vertextype = VERTEX_64_TEX4;
  Mesh_free(g_VID.drw_meshquad);
  Mesh_free(g_VID.drw_meshquadffx);
  Mesh_free(g_VID.drw_meshbuffer);
  Mesh_free(g_VID.drw_meshbox);
  Mesh_free(g_VID.drw_meshsphere);
  Mesh_free(g_VID.drw_meshcylinder);
  Mesh_free(g_VID.drw_meshquadcentered);
  Mesh_free(g_VID.drw_meshquadcenteredffx);

  lxStrMap_delete(l_VIDData.alphamodes,NULL);
  lxStrMap_delete(l_VIDData.colormodes,NULL);
  lxStrMap_delete(l_VIDData.userdefines,NULL);
  lxStrMap_delete(l_VIDData.cgconnectors,NULL);
  lxStrMap_delete(l_VIDData.cgconnectorsemantics,NULL);

  if (g_VID.cg.context){
    cgDestroyContext(g_VID.cg.context);
  }
}

const char* VID_GetOSString()
{
  return g_LuxiniaWinMgr.luxiGetOSString();
}

const char* VID_GPUVendor_toString(VID_GPUVendor_t gpuvendor){
  switch(gpuvendor){
    case VID_VENDOR_ATI:    return "ati";
    case VID_VENDOR_NVIDIA:   return "nvidia";
    case VID_VENDOR_INTEL:    return "intel";
    default:
    case VID_VENDOR_UNKNOWN:  return "unknown";
  }
}


///////////////////////////////////////////////////////////////////////////////
// vid Commands


void vidOrthoOn(float left, float right, float bottom, float top, float front, float back)
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  lxMatrix44OrthoDirect(g_VID.drawsetup.orthoProjMatrix,left, right, bottom, top, front, back);
  glLoadMatrixf(g_VID.drawsetup.orthoProjMatrix);

  g_VID.drawsetup.oprojMatrix =   g_VID.drawsetup.projMatrix;
  g_VID.drawsetup.oprojMatrixInv =  g_VID.drawsetup.projMatrixInv;
  g_VID.drawsetup.oviewMatrix =   g_VID.drawsetup.viewMatrix;
  g_VID.drawsetup.oviewMatrixInv =  g_VID.drawsetup.viewMatrixInv;
  g_VID.drawsetup.oviewProjMatrix = g_VID.drawsetup.viewProjMatrix;

  g_VID.drawsetup.projMatrix =    g_VID.drawsetup.orthoProjMatrix;
  g_VID.drawsetup.projMatrixInv =   g_VID.drawsetup.orthoProjMatrixInv;
  g_VID.drawsetup.viewMatrix =    lxMatrix44GetIdentity();
  g_VID.drawsetup.viewMatrixInv =   lxMatrix44GetIdentity();
  g_VID.drawsetup.viewProjMatrix =  g_VID.drawsetup.projMatrix;

  lxMatrix44Invert(g_VID.drawsetup.orthoProjMatrixInv,g_VID.drawsetup.projMatrix);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  vidCgViewProjUpdate();
}
void vidOrthoOff()
{
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  g_VID.drawsetup.projMatrix =    g_VID.drawsetup.oprojMatrix;
  g_VID.drawsetup.projMatrixInv =   g_VID.drawsetup.oprojMatrixInv;
  g_VID.drawsetup.viewMatrix =    g_VID.drawsetup.oviewMatrix;
  g_VID.drawsetup.viewMatrixInv =   g_VID.drawsetup.oviewMatrixInv;
  g_VID.drawsetup.viewProjMatrix =  g_VID.drawsetup.oviewProjMatrix;

  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(g_VID.drawsetup.viewMatrix);

  vidCgViewProjUpdate();
}

void vidCgViewProjUpdate()
{
  lxVector3 camwdir = { -g_VID.drawsetup.viewMatrixInv[8],
            -g_VID.drawsetup.viewMatrixInv[9],
            -g_VID.drawsetup.viewMatrixInv[10]};

  cgSetMatrixParameterfc(g_VID.cg.projMatrix,g_VID.drawsetup.projMatrix);
  cgSetMatrixParameterfc(g_VID.cg.projMatrixInv,g_VID.drawsetup.projMatrixInv);
  cgSetMatrixParameterfc(g_VID.cg.viewMatrix,g_VID.drawsetup.viewMatrix);
  cgSetMatrixParameterfc(g_VID.cg.viewMatrixInv,g_VID.drawsetup.viewMatrixInv);
  cgSetMatrixParameterfc(g_VID.cg.viewProjMatrix,g_VID.drawsetup.viewProjMatrix);

  cgSetParameter3fv(g_VID.cg.camwpos,&g_VID.drawsetup.viewMatrixInv[12]);
  cgSetParameter3fv(g_VID.cg.camwdir,camwdir);


}

void vidViewportScissorCheck()
{
  if (g_VID.state.viewport.bounds.width != g_VID.drawbufferWidth || g_VID.state.viewport.bounds.height != g_VID.drawbufferHeight || g_VID.state.viewport.bounds.x != 0 || g_VID.state.viewport.bounds.y != 0){
    glScissor(g_VID.state.viewport.bounds.x,g_VID.state.viewport.bounds.y,g_VID.state.viewport.bounds.width,g_VID.state.viewport.bounds.height);
    vidScissor(LUX_TRUE);
    g_VID.state.viewport.bounds.fullwindow = LUX_FALSE;
  }
  else{
    vidScissor(LUX_FALSE);
    g_VID.state.viewport.bounds.fullwindow = LUX_TRUE;
  }
}

int vidViewport(int x,int y, int width, int height)
{

  if (width != g_VID.state.viewport.bounds.width || height != g_VID.state.viewport.bounds.height ||  x != g_VID.state.viewport.bounds.x || y != g_VID.state.viewport.bounds.y){
    glViewport (x, y, width, height);

    g_VID.state.viewport.bounds.width = width;
    g_VID.state.viewport.bounds.height = height;
    g_VID.state.viewport.bounds.x = x;
    g_VID.state.viewport.bounds.y = y;

    vidViewportScissorCheck();

    cgSetParameter2f(g_VID.cg.viewsize, (float)g_VID.state.viewport.bounds.width, (float)g_VID.state.viewport.bounds.height);
    cgSetParameter2f(g_VID.cg.viewsizeinv, 1.0f/(float)g_VID.state.viewport.bounds.width, 1.0f/(float)g_VID.state.viewport.bounds.height);

    return LUX_TRUE;
  }
  return LUX_FALSE;
}

void vidDepthRange(float from, float to)
{
  VIDViewPort_t*  view = &g_VID.state.viewport;
  view->depth[0] = from;
  view->depth[1] = to;
  glDepthRange(from,to);
}

void vidResetTexture(){
  int i;
  glMatrixMode(GL_TEXTURE);
  for (i = g_VID.capTexImages-1; i >= 0; i--){
    g_VID.state.textures[i] = -1;
    vidSelectTexture(i);
    //glActiveTextureARB(GL_TEXTURE0_ARB+i);
    if (i < g_VID.capTexCoords){
      vidTexCoordSource(VID_TEXCOORD_NONE,VID_TEXCOORD_NOSET);
      glLoadIdentity();
    }
    if (i < g_VID.capTexUnits){
      vidTexturing(LUX_FALSE);
      vidTexBlendDefault(VID_TEXBLEND_MOD_MOD);
    }
  }
  glMatrixMode(GL_MODELVIEW);
  //g_VID.state.activeTex = 0;
}

LUX_INLINE void vidBlendNative(const VIDBlendColor_t blendmode,const booln blendinvert,
                 GLenum* src, GLenum *dst, GLenum *equ)
{
  GLenum  ALPHA = GL_SRC_ALPHA;
  GLenum  MINUSALPHA = GL_ONE_MINUS_SRC_ALPHA;

  if (blendinvert){
    ALPHA = GL_ONE_MINUS_SRC_ALPHA;
    MINUSALPHA = GL_SRC_ALPHA;
  }

  switch (blendmode){
    case VID_REPLACE:
      *src = GL_ONE;
      *dst = GL_ZERO;
      *equ = GL_FUNC_ADD;
      return;
    case VID_MODULATE:
      *src = GL_DST_COLOR;
      *dst = GL_ZERO;
      *equ = GL_FUNC_ADD;
      return;
    case VID_MODULATE2:
      *src = GL_DST_COLOR;
      *dst = GL_SRC_COLOR;
      *equ = GL_FUNC_ADD;
      return;
    case VID_MIN:
      *src = GL_ONE;
      *dst = GL_ONE;
      *equ = GL_MIN;
      return;
    case VID_MAX:
      *src = GL_ONE;
      *dst = GL_ONE;
      *equ = GL_MAX;
      return;
    case VID_SUB:
      *src = GL_ONE;
      *dst = GL_ONE;
      *equ = GL_FUNC_SUBTRACT;
      return;
    case VID_AMODSUB:
      *src = ALPHA;
      *dst = GL_ONE;
      *equ = GL_FUNC_SUBTRACT;
      return;
    case VID_SUB_REV:
      *src = GL_ONE;
      *dst = GL_ONE;
      *equ = GL_FUNC_REVERSE_SUBTRACT;
      return;
    case VID_AMODSUB_REV:
      *src = ALPHA;
      *dst = GL_ONE;
      *equ = GL_FUNC_REVERSE_SUBTRACT;
      return;
    case VID_DECAL:
      *src = ALPHA;
      *dst = MINUSALPHA;
      *equ = GL_FUNC_ADD;
      return;
    case VID_ADD:
      *src = GL_ONE;
      *dst = GL_ONE;
      *equ = GL_FUNC_ADD;
      return;
    case VID_AMODADD:
      *src = ALPHA;
      *dst = GL_ONE;
      *equ = GL_FUNC_ADD;
      return;
    default:
      *src = GL_ONE;
      *dst = GL_ONE;
      *equ = GL_FUNC_ADD;
      return;
  }
}


//////////////////////////////////////////////////////////////////////////
// VIDDepth

void VIDDepth_init(VIDDepth_t *viddepth)
{
  viddepth->func = 0;
}

//////////////////////////////////////////////////////////////////////////
// VIDLine

void VIDLine_init(VIDLine_t *vidline)
{
  vidline->linefactor = 0;
  vidline->linewidth = 1;
  vidline->linestipple = 0;
}

void VIDLine_setGL(const VIDLine_t *line)
{
  glLineWidth(line->linewidth);
  if (line->linestipple){
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(line->linefactor,line->linestipple);
  }
  else{
    glDisable(GL_LINE_STIPPLE);
  }
}

//////////////////////////////////////////////////////////////////////////
// VIDStencil

// side = -1 both, 0 front, 1 back
void VIDStencil_setGL(const VIDStencil_t *vidstencil, int side)
{
  const VIDStencilOp_t *op;
  VIDStencilOp_t *opcmp;
  int face;

  if (side < 0){
    if (g_VID.state.stencil.twosided != side ||
      g_VID.state.stencil.funcF != vidstencil->funcF ||
      g_VID.state.stencil.funcB != vidstencil->funcB ||
      vidstencil->refvalue != g_VID.state.stencil.refvalue ||
      vidstencil->mask != g_VID.state.stencil.mask)
    {
      g_VID.state.stencil.funcF = vidstencil->funcF;
      g_VID.state.stencil.funcB = vidstencil->funcB;
      g_VID.state.stencil.refvalue = vidstencil->refvalue;
      g_VID.state.stencil.mask = vidstencil->mask;

      if (GLEW_ATI_separate_stencil)
        glStencilFuncSeparateATI(vidstencil->funcF,vidstencil->funcB,vidstencil->refvalue,vidstencil->mask);
      else{
        glActiveStencilFaceEXT(GL_FRONT);
        glStencilFunc(vidstencil->funcF,vidstencil->refvalue,vidstencil->mask);
        glActiveStencilFaceEXT(GL_BACK);
        glStencilFunc(vidstencil->funcB,vidstencil->refvalue,vidstencil->mask);
      }

    }

    op = &vidstencil->ops[0];
    opcmp = &g_VID.state.stencil.ops[0];
    face = GL_FRONT;
    if (g_VID.state.stencil.twosided != side ||
      opcmp->fail != op->fail ||
      opcmp->zfail != op->zfail  ||
      opcmp->zpass != op->zpass)
    {
      opcmp->fail = op->fail;
      opcmp->zfail = op->zfail;
      opcmp->zpass = op->zpass;
      if (GLEW_ATI_separate_stencil)
        glStencilOpSeparateATI(face,op->fail,op->zfail,op->zpass);
      else{
        glActiveStencilFaceEXT(face);
        glStencilOp(op->fail,op->zfail,op->zpass);
      }
    }
    op = &vidstencil->ops[1];
    opcmp = &g_VID.state.stencil.ops[1];
    face = GL_BACK;
    if (g_VID.state.stencil.twosided != side ||
      opcmp->fail != op->fail ||
      opcmp->zfail != op->zfail  ||
      opcmp->zpass != op->zpass)
    {
      opcmp->fail = op->fail;
      opcmp->zfail = op->zfail;
      opcmp->zpass = op->zpass;
      if (GLEW_ATI_separate_stencil)
        glStencilOpSeparateATI(face,op->fail,op->zfail,op->zpass);
      else{
        glActiveStencilFaceEXT(face);
        glStencilOp(op->fail,op->zfail,op->zpass);
      }
    }
    if (GLEW_EXT_stencil_two_side)
      glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
  }
  else{
    int func = (side) ? vidstencil->funcB : vidstencil->funcF;
    op = (side) ? op = &vidstencil->ops[1] : &vidstencil->ops[0];

    if (GLEW_EXT_stencil_two_side){
      glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
      glActiveStencilFaceEXT(GL_FRONT);
    }

    if (g_VID.state.stencil.twosided != side || func != g_VID.state.stencil.funcF ||
      vidstencil->refvalue != g_VID.state.stencil.refvalue ||
      vidstencil->mask != g_VID.state.stencil.mask)
    {
      g_VID.state.stencil.funcF = func;
      g_VID.state.stencil.refvalue = vidstencil->refvalue;
      g_VID.state.stencil.mask = vidstencil->mask;
      glStencilFunc(vidstencil->funcF,vidstencil->refvalue,vidstencil->mask);
    }

    if (g_VID.state.stencil.twosided != side ||g_VID.state.stencil.ops[0].fail != op->fail ||
      g_VID.state.stencil.ops[0].zfail != op->zfail ||
      g_VID.state.stencil.ops[0].zpass != op->zpass)
    {
      g_VID.state.stencil.ops[0].fail = op->fail;
      g_VID.state.stencil.ops[0].zfail = op->zfail;
      g_VID.state.stencil.ops[0].zpass = op->zpass;
      glStencilOp(op->fail,op->zfail,op->zpass);
    }


  }

  g_VID.state.stencil.twosided = side;
}

void VIDStencil_init(VIDStencil_t *vidstencil)
{
  vidstencil->enabled = LUX_FALSE;
  vidstencil->twosided = 0;
  vidstencil->ops[0].fail = GL_KEEP;
  vidstencil->ops[0].zfail  = GL_KEEP;
  vidstencil->ops[0].zpass  = GL_KEEP;
  vidstencil->ops[1].fail = GL_KEEP;
  vidstencil->ops[1].zfail  = GL_KEEP;
  vidstencil->ops[1].zpass  = GL_KEEP;
  vidstencil->funcF = GL_ALWAYS;
  vidstencil->funcB = GL_ALWAYS;
  vidstencil->refvalue = 0;
  vidstencil->mask = BIT_ID_FULL32;
}

//////////////////////////////////////////////////////////////////////////
// RenderFlag

enum32 VIDRenderFlag_getGL(){
  enum32 renderflag;
  GLboolean state[4];
  GLint   blah[4];
  renderflag = 0;
  if (glIsEnabled(GL_BLEND))
    renderflag |= RENDER_BLEND;
  if (glIsEnabled(GL_NORMALIZE))
    renderflag |= RENDER_BLEND;
  if (glIsEnabled(GL_ALPHA_TEST))
    renderflag |= RENDER_ALPHATEST;
  if (glIsEnabled(GL_LIGHTING))
    renderflag |= RENDER_LIT;
  if (glIsEnabled(GL_FOG))
    renderflag |= RENDER_FOG;
  if (!glIsEnabled(GL_DEPTH_TEST))
    renderflag |= RENDER_NODEPTHTEST;
  if (!glIsEnabled(GL_CULL_FACE))
    renderflag |= RENDER_NOCULL;
  if (!glIsEnabled(GL_COLOR_ARRAY))
    renderflag |= RENDER_NOVERTEXCOLOR;
  if (glIsEnabled(GL_STENCIL_TEST))
    renderflag |= RENDER_STENCILTEST;
  if (glIsEnabled(GL_LIGHT0))
    renderflag |= RENDER_SUNLIT;
  glGetBooleanv(GL_DEPTH_WRITEMASK,state);
  if (!state[0])
    renderflag |= RENDER_NODEPTHMASK;
  glGetBooleanv(GL_COLOR_WRITEMASK,state);
  if (!state[0])
    renderflag |= RENDER_NOCOLORMASK;
  glGetBooleanv(GL_STENCIL_WRITEMASK,state);
  if (state[0])
    renderflag |= RENDER_STENCILMASK;

  glGetIntegerv(GL_POLYGON_MODE, blah);
  if (blah[1] != GL_FILL)
    renderflag |= RENDER_WIRE;

  glGetIntegerv(GL_CULL_FACE,blah);
  if (blah[0] == GL_FRONT)
    renderflag |= RENDER_FRONTCULL;

  return renderflag;

}

enum32 VIDRenderFlag_fromString(char *names)
{
  enum32 rf = 0;

  if (strstr(names,"rfLit"))
    rf |= RENDER_LIT;
  if (strstr(names,"rfUnlit"))
    rf |= RENDER_UNLIT;
  if (strstr(names,"rfNeverdraw"))
    rf |= RENDER_NEVER;
  if (strstr(names,"rfFog"))
    rf |= RENDER_FOG;
  if (strstr(names,"rfNovertexcolor"))
    rf |= RENDER_NOVERTEXCOLOR;
  if (strstr(names,"rfNodepthmask"))
    rf |= RENDER_NODEPTHMASK;
  if (strstr(names,"rfNodepthtest"))
    rf |= RENDER_NODEPTHTEST;
  if (strstr(names,"rfNodraw"))
    rf |= RENDER_NODRAW;
  if (strstr(names,"rfProjectorpass"))
    rf |= RENDER_PROJPASS;
  if (strstr(names,"rfNormalize"))
    rf |= RENDER_NORMALIZE;
  if (strstr(names,"rfNocolormask"))
    rf |= RENDER_NOCOLORMASK;
  if (strstr(names,"rfFrontcull"))
    rf |= RENDER_FRONTCULL;

  return rf;
}

#define CASE_TEST(what) if( changed & what){
#define CASE_END    }

booln VIDRenderFlag_testGL()
{
  booln error = LUX_FALSE;
  flags32 flags = g_VID.state.renderflag & (RENDER_LASTFLAG-1);
  flags32 current = VIDRenderFlag_getGL();
  flags32 changed = current ^ flags;

  CASE_TEST( RENDER_LIT)
    LUX_PRINTF("RenderFlag Missmatch, RENDER_LIT \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_NOVERTEXCOLOR )
    LUX_PRINTF("RenderFlag Missmatch, RENDER_NOVERTEXCOLOR \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_NOCULL )
    LUX_PRINTF("RenderFlag Missmatch, RENDER_NOCULL \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_NODEPTHMASK )
    LUX_PRINTF("RenderFlag Missmatch, RENDER_NODEPTHMASK \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_NODEPTHTEST )
    LUX_PRINTF("RenderFlag Missmatch, RENDER_NODEPTHTEST \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_BLEND )
    LUX_PRINTF("RenderFlag Missmatch, RENDER_BLEND \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_ALPHATEST )
    LUX_PRINTF("RenderFlag Missmatch, RENDER_ALPHATEST \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_NORMALIZE)
    LUX_PRINTF("RenderFlag Missmatch, RENDER_NORMALIZE \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_FOG)
    LUX_PRINTF("RenderFlag Missmatch, RENDER_FOG \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_SUNLIT)
    LUX_PRINTF("RenderFlag Missmatch, RENDER_SUNLIT \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_WIRE)
    LUX_PRINTF("RenderFlag Missmatch, RENDER_WIRE \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_NOCOLORMASK)
    LUX_PRINTF("RenderFlag Missmatch, RENDER_NOCOLORMASK \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_FRONTCULL)
    LUX_PRINTF("RenderFlag Missmatch, RENDER_FRONTCULL \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_STENCILMASK)
    LUX_PRINTF("RenderFlag Missmatch, RENDER_STENCILMASK \n");
    error = LUX_TRUE;
  CASE_END;
  CASE_TEST( RENDER_STENCILTEST)
    LUX_PRINTF("RenderFlag Missmatch, RENDER_STENCILTEST \n");
    error = LUX_TRUE;
  CASE_END;

  return error;
}

void  VIDDepth_getGL(VIDDepth_t* state)
{
  glGetIntegerv(GL_DEPTH_FUNC,&state->func);
}

void  VIDAlpha_getGL(VIDAlpha_t* state)
{
  glGetIntegerv(GL_ALPHA_TEST_FUNC,&state->alphafunc);
  glGetFloatv(GL_ALPHA_TEST_REF,&state->alphaval);
}

void VIDLogicOp_setGL(VIDLogicOp_t *logic)
{
  if (logic->op){
    glEnable(GL_LOGIC_OP);
    glLogicOp(logic->op);
  }
  else{
    glDisable(GL_LOGIC_OP);
  }
}

void VIDLogicOp_getGL(VIDLogicOp_t *logic)
{
  if (glIsEnabled(GL_LOGIC_OP))
    glGetIntegerv(GL_LOGIC_OP_MODE,&logic->op);
  else
    logic->op = 0;
}

void  VIDBlend_getGL(VIDBlend_t* blend)
{
  VIDBlendColor_t modes[12] = {
    VID_REPLACE,VID_MODULATE,VID_MODULATE2,
    VID_MIN,VID_MAX,VID_SUB,VID_AMODSUB,VID_SUB_REV,VID_AMODSUB_REV,
    VID_DECAL,VID_ADD,VID_AMODADD
  };
  int i;

  GLenum ssrc;
  GLenum sdst;
  GLenum sequ = GL_FUNC_ADD_EXT;

  GLenum src;
  GLenum dst;
  GLenum equ;

  glGetIntegerv(GL_BLEND_SRC_RGB,&src);
  glGetIntegerv(GL_BLEND_DST_RGB,&dst);
  glGetIntegerv(GL_BLEND_EQUATION_RGB,&equ);

  if (equ == sequ){
    for (i = 0; i < 24; i++){
      vidBlendNative(modes[i%12],i >=12 ,&ssrc,&sdst,&sequ);
      if (ssrc == src && sdst == dst && sequ == equ){
        blend->alphamode = modes[i];
        blend->blendmode = modes[i];
        blend->blendinvert = i >=12;
        return;
      }
    }
  }

  blend->alphamode = VID_UNSET;
  blend->blendmode = VID_UNSET;
  blend->blendinvert = LUX_FALSE;
}

void VIDStencil_getGL(VIDStencil_t *stencil)
{

}

void VIDViewPort_getGL(VIDViewPort_t* view)
{
  glGetIntegerv(GL_VIEWPORT,view->bounds.sizes);
  glGetFloatv(GL_DEPTH_RANGE,view->depth);
  view->bounds.fullwindow  = (
    view->bounds.sizes[0] == 0 &&
    view->bounds.sizes[1] == 0 &&
    view->bounds.sizes[2] == g_VID.drawbufferWidth &&
    view->bounds.sizes[3] == g_VID.drawbufferHeight);
}

void VIDState_getGL()
{
  GLenum tex;
  g_VID.state.renderflag = VIDRenderFlag_getGL();

  VIDDepth_getGL(&g_VID.state.depth);
  VIDAlpha_getGL(&g_VID.state.alpha);
  VIDBlend_getGL(&g_VID.state.blend);
  VIDStencil_getGL(&g_VID.state.stencil);
  VIDViewPort_getGL(&g_VID.state.viewport);

  g_VID.state.scissor = glIsEnabled(GL_SCISSOR_TEST);

  glGetIntegerv(GL_ACTIVE_TEXTURE_ARB,&tex);
  g_VID.state.activeTex = tex-GL_TEXTURE0;

  glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE_ARB,&tex);
  g_VID.state.activeClientTex = tex-GL_TEXTURE0;
}

booln VIDState_testGL()
{
  booln error = VIDRenderFlag_testGL();

  {
    VIDDepth_t* state = &g_VID.state.depth;
    VIDDepth_t test;
    VIDDepth_getGL(&test);

    if (test.func != state->func){
      LUX_PRINTF("State Missmatch, GL_DEPTH_FUNC \n");
      error = LUX_TRUE;
    }
  }

  {
    VIDAlpha_t* state = &g_VID.state.alpha;
    VIDAlpha_t test;
    VIDAlpha_getGL(&test);

    if (test.alphafunc != state->alphafunc){
      LUX_PRINTF("State Missmatch, GL_ALPHA_TEST_FUNC \n");
      error = LUX_TRUE;
    }

    if (test.alphaval != state->alphaval){
      LUX_PRINTF("State Missmatch, GL_ALPHA_TEST_REF \n");
      error = LUX_TRUE;
    }
  }

  {
    VIDBlend_t* state = &g_VID.state.blend;
    VIDBlend_t test;
    VIDBlend_getGL(&test);

    if (test.blendmode != state->blendmode){
      LUX_PRINTF("State Missmatch, BLEND_COLORMODE \n");
      error = LUX_TRUE;
    }
    /*
    if (test.alphamode != state->alphamode){
      LUX_PRINTF("State Missmatch, BLEND_ALPHAMODE \n");
    }
    */
    if (test.blendinvert != state->blendinvert){
      LUX_PRINTF("State Missmatch, BLEND_INVERT \n");
      error = LUX_TRUE;
    }
  }

  {
    VIDStencil_t* state = &g_VID.state.stencil;
    VIDStencil_t test;
    VIDStencil_getGL(&test);
  }

  {

    VIDViewPort_t* state = &g_VID.state.viewport;
    VIDViewPort_t test;
    VIDViewPort_getGL(&test);

    if (state->bounds.fullwindow != test.bounds.fullwindow){
      LUX_PRINTF("State Missmatch, VIEWPORT_FULLWINDOW \n");
      error = LUX_TRUE;
    }
    else{
      int i;
      for (i=0; i < 4; i++){
        if (state->bounds.sizes[i] != test.bounds.sizes[i]){
          LUX_PRINTF("State Missmatch, VIEWPORT_BOUNDS %d\n",i);
          error = LUX_TRUE;
        }
      }
    }

    if (test.depth[0] != state->depth[0] ||
      test.depth[1] != state->depth[1])
    {
      LUX_PRINTF("State Missmatch, GL_DEPTH_RANGE \n");
      error = LUX_TRUE;
    }
  }

  {
    GLenum tex;

    if (g_VID.state.scissor != glIsEnabled(GL_SCISSOR_TEST)){
      LUX_PRINTF("State Missmatch, GL_SCISSOR_TEST \n");
      error = LUX_TRUE;
    }


    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB,&tex);
    if (tex-GL_TEXTURE0 != g_VID.state.activeTex){
      LUX_PRINTF("State Missmatch, GL_ACTIVE_TEXTURE_ARB \n");
      error = LUX_TRUE;
    }
    glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE_ARB,&tex);
    if (tex-GL_TEXTURE0 != g_VID.state.activeTex){
      LUX_PRINTF("State Missmatch, GL_CLIENT_ACTIVE_TEXTURE_ARB \n");
      error = LUX_TRUE;
    }
  }

  return error;
}

void VIDRenderFlag_setGL(enum32 flags){
  enum32 changed;

  flags = (flags | g_VID.state.renderflagforce) & (~g_VID.state.renderflagignore);
  changed = (flags  ^ g_VID.state.renderflag) & (RENDER_LASTFLAG-1);

  if (!changed)
    return;

  CASE_TEST( RENDER_LIT)
    if (RENDER_LIT & flags) glEnable(GL_LIGHTING);
    else      glDisable(GL_LIGHTING);
  CASE_END;
  CASE_TEST( RENDER_NOVERTEXCOLOR )
    if (RENDER_NOVERTEXCOLOR & flags) glDisableClientState(GL_COLOR_ARRAY);
    else      glEnableClientState(GL_COLOR_ARRAY);
  CASE_END;
  CASE_TEST( RENDER_NOCULL )
    if (RENDER_NOCULL & flags)  glDisable(GL_CULL_FACE);
    else      glEnable(GL_CULL_FACE);
  CASE_END;
  CASE_TEST( RENDER_NODEPTHMASK )
    if (RENDER_NODEPTHMASK & flags) glDepthMask(GL_FALSE);
    else      glDepthMask(GL_TRUE);
  CASE_END;
  CASE_TEST( RENDER_NODEPTHTEST )
    if (RENDER_NODEPTHTEST & flags) glDisable(GL_DEPTH_TEST);
    else      glEnable(GL_DEPTH_TEST);
  CASE_END;
  CASE_TEST( RENDER_BLEND )
    if (RENDER_BLEND & flags) glEnable(GL_BLEND);
    else      glDisable(GL_BLEND);
  CASE_END;
  CASE_TEST( RENDER_ALPHATEST )
    if (RENDER_ALPHATEST & flags) glEnable(GL_ALPHA_TEST);
    else      glDisable(GL_ALPHA_TEST);
  CASE_END;
  CASE_TEST( RENDER_NORMALIZE)
    if (RENDER_NORMALIZE & flags) glEnable(GL_NORMALIZE);
    else      glDisable(GL_NORMALIZE);
  CASE_END;
  CASE_TEST( RENDER_FOG)
    if (RENDER_FOG & flags) glEnable(GL_FOG);
    else      glDisable(GL_FOG);
  CASE_END;
  CASE_TEST( RENDER_SUNLIT)
    if (RENDER_SUNLIT & flags)  {glEnable(GL_LIGHT0);glLightModelfv(GL_LIGHT_MODEL_AMBIENT, g_VID.black);}
    else      {glDisable(GL_LIGHT0);glLightModelfv(GL_LIGHT_MODEL_AMBIENT, g_VID.white);}
  CASE_END;
  CASE_TEST( RENDER_WIRE)
    if (RENDER_WIRE & flags)  glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    else      glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  CASE_END;
  CASE_TEST( RENDER_NOCOLORMASK)
    if (RENDER_NOCOLORMASK & flags) glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    else      glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  CASE_END;
  CASE_TEST( RENDER_FRONTCULL)
    if (RENDER_FRONTCULL & flags) glCullFace(GL_FRONT);
    else      glCullFace(GL_BACK);
  CASE_END;
  CASE_TEST( RENDER_STENCILMASK)
    if (RENDER_STENCILMASK & flags) g_VID.gensetup.stencilmaskfunc(g_VID.state.stencilmaskpos);
    else      g_VID.gensetup.stencilmaskfunc(g_VID.state.stencilmaskneg);
  CASE_END;
  CASE_TEST( RENDER_STENCILTEST)
    if (RENDER_STENCILTEST & flags) glEnable(GL_STENCIL_TEST);
    else      glDisable(GL_STENCIL_TEST);
  CASE_END;


  g_VID.state.renderflag = flags;
}

void VIDRenderFlag_disable()
{
  VIDRenderFlag_setGL(RENDER_NODEPTHTEST | RENDER_NOVERTEXCOLOR);
}

#undef CASE_TEST
#undef CASE_END

//////////////////////////////////////////////////////////////////////////
// vidTex


LUX_INLINE void vidTexBlendSet(
  const VIDTexBlendHash hash,
  const VIDBlendColor_t blendmode,  VIDBlendAlpha_t alphamode,
  const booln blendinvert, const int rgbscale, const int alphascale)
{
#define vidTexEnvMode(mode)         glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode )
#define vidTexEnvf(what,val)        glTexEnvf( GL_TEXTURE_ENV,what,val )
#define vidComb_COMB_RGB(mode)        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, mode)
#define vidComb_SRC_RGB(unit,target)    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB + unit, target)
#define vidComb_OP_RGB(unit,target)     glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB + unit, target)
#define vidComb_COMB_ALPHA(mode)      glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, mode)
#define vidComb_SRC_ALPHA(unit,target)    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA + unit, target)
#define vidComb_OP_ALPHA(unit,target)   glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA + unit, target)

  GLenum    ALPHA = blendinvert ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA;
  GLenum    COLOR = blendinvert ? GL_ONE_MINUS_SRC_COLOR : GL_SRC_COLOR;
  int iscomb4 = LUX_FALSE;
  float scales[3] = {1.0f,2.0f,4.0f};

  //if (VIDTexBlendHash_get(blendmode,alphamode,blendinvert,rgbscale,alphascale) != hash)
  //  return;


  g_VID.state.texhash[g_VID.state.activeTex] = hash;

  vidTexEnvf(   GL_RGB_SCALE, scales[rgbscale]);
  vidTexEnvf(   GL_ALPHA_SCALE, scales[alphascale]);

  switch  (blendmode){
    default:
      {
        VIDTexCombiner_t *texcomb = &g_VID.gensetup.tccolor[blendmode-VID_COLOR_USER];
        int i;
        vidTexEnvMode ( texcomb->combine4 ? GL_COMBINE4_NV : GL_COMBINE );
        vidComb_COMB_RGB  (texcomb->mode);
        for(i=0; i < texcomb->numCmds; i++){
          vidComb_SRC_RGB   (i, texcomb->srcs[i]);
          vidComb_OP_RGB    (i, texcomb->ops[i]);
        }
        iscomb4 = texcomb->combine4;
      }
      break;
    case VID_REPLACE:
      if ((alphamode == VID_A_UNSET || alphamode == VID_A_REPLACE) && rgbscale == 0 && alphascale == 0){
        vidTexEnvMode ( GL_REPLACE );
        alphamode = VID_A_SETBYCOLOR;
        return;
      }
      else{
        vidTexEnvMode (  GL_COMBINE );
        vidComb_COMB_RGB  (GL_REPLACE);
        vidComb_SRC_RGB   (0, GL_TEXTURE);
        vidComb_OP_RGB    (0, GL_SRC_COLOR);
      }
      break;
    case VID_PASSTHRU:
      alphamode = VID_A_REPLACE_PREV;
    case VID_REPLACE_PREV:
      vidTexEnvMode (  GL_COMBINE );
      vidComb_COMB_RGB  (GL_REPLACE);
      vidComb_SRC_RGB   (0, GL_PREVIOUS);
      vidComb_OP_RGB    (0, GL_SRC_COLOR);
      break;
    case VID_REPLACE_VERTEX:
      vidTexEnvMode (  GL_COMBINE );
      vidComb_COMB_RGB  (GL_REPLACE);
      vidComb_SRC_RGB   (0, GL_PRIMARY_COLOR);
      vidComb_OP_RGB    (0, GL_SRC_COLOR);
      break;
    case VID_REPLACE_CONST:
      vidTexEnvMode (  GL_COMBINE );
      vidComb_COMB_RGB  (GL_REPLACE);
      vidComb_SRC_RGB   (0, GL_CONSTANT);
      vidComb_OP_RGB    (0, GL_SRC_COLOR);
      break;
    case VID_MODULATE:
      if ((alphamode == VID_A_UNSET || alphamode == VID_A_MODULATE) && rgbscale == 0 && alphascale == 0){
        vidTexEnvMode ( GL_MODULATE );
        alphamode = VID_A_SETBYCOLOR;
        return;
      }
      else{
        vidTexEnvMode ( GL_COMBINE );
        vidComb_COMB_RGB  (GL_MODULATE );
        vidComb_SRC_RGB   (0, GL_PREVIOUS );
        vidComb_OP_RGB    (0, GL_SRC_COLOR );
        vidComb_SRC_RGB   (1, GL_TEXTURE);
        vidComb_OP_RGB    (1, GL_SRC_COLOR);
      }
      break;
    case VID_ADD:
      vidTexEnvMode (  GL_COMBINE );
      vidComb_COMB_RGB  ( GL_ADD );
      vidComb_SRC_RGB   (0 , GL_TEXTURE );
      vidComb_SRC_RGB   (1 , GL_PREVIOUS );
      vidComb_OP_RGB    (0 , GL_SRC_COLOR );
      vidComb_OP_RGB    (1 , GL_SRC_COLOR );
      /*
      if (alphamode == VID_A_UNSET || alphamode == VID_A_MODULATE){
        vidTexEnvMode ( GL_ADD );
        alphamode = VID_A_SETBYCOLOR;
      }
      else{
        vidTexEnvMode ( GL_COMBINE );
        vidComb_COMB_RGB  (GL_ADD );
        vidComb_SRC_RGB   (0, GL_PREVIOUS );
        vidComb_OP_RGB    (0, GL_SRC_COLOR );
        vidComb_SRC_RGB   (1, GL_TEXTURE);
        vidComb_OP_RGB    (1, GL_SRC_COLOR);
      }*/
      break;
    case VID_ADD_INV:
      vidTexEnvMode (  GL_COMBINE );
      vidComb_COMB_RGB  ( GL_ADD );
      vidComb_SRC_RGB   (0 , GL_TEXTURE );
      vidComb_SRC_RGB   (1 , GL_PREVIOUS );
      vidComb_OP_RGB    (0 , GL_ONE_MINUS_SRC_COLOR );
      vidComb_OP_RGB    (1 , GL_SRC_COLOR );
      break;
    case VID_ADD_VERTEX:
      vidTexEnvMode (  GL_COMBINE );
      vidComb_COMB_RGB  ( GL_ADD );
      vidComb_SRC_RGB   (0 , GL_PRIMARY_COLOR );
      vidComb_SRC_RGB   (1 , GL_PREVIOUS );
      vidComb_OP_RGB    (0 , GL_SRC_COLOR );
      vidComb_OP_RGB    (1 , GL_SRC_COLOR );
      break;
    case VID_ADD_CONST:
      vidTexEnvMode (  GL_COMBINE );
      vidComb_COMB_RGB  ( GL_ADD );
      vidComb_SRC_RGB (0 , GL_CONSTANT );
      vidComb_SRC_RGB (1 , GL_PREVIOUS );
      vidComb_OP_RGB    (0 , GL_SRC_COLOR );
      vidComb_OP_RGB    (1 , GL_SRC_COLOR );
      break ;
    case VID_SQUARE:
      vidTexEnvMode ( GL_COMBINE );
      vidComb_COMB_RGB  (GL_MODULATE );
      vidComb_SRC_RGB   (0, GL_PREVIOUS );
      vidComb_OP_RGB    (0, GL_SRC_COLOR );
      vidComb_SRC_RGB   (1, GL_PREVIOUS);
      vidComb_OP_RGB    (1, GL_SRC_COLOR);
      break;

    case VID_CMOD:
      vidTexEnvMode (  GL_COMBINE );
      vidComb_COMB_RGB  (GL_MODULATE );
      vidComb_SRC_RGB   (0 , GL_TEXTURE );
      vidComb_SRC_RGB   (1 , GL_PRIMARY_COLOR );
      vidComb_OP_RGB    (0, GL_SRC_COLOR);
      vidComb_OP_RGB    (1, GL_SRC_COLOR);
      break;

    case VID_CMOD_PREV:
      vidTexEnvMode (  GL_COMBINE );
      vidComb_COMB_RGB  (GL_MODULATE );
      vidComb_SRC_RGB (0, GL_PREVIOUS );
      vidComb_SRC_RGB (1, GL_PRIMARY_COLOR );
      vidComb_OP_RGB    (0, GL_SRC_COLOR);
      vidComb_OP_RGB    (1, GL_SRC_COLOR);
      break;


      // DECAL
#define vidCombDECAL()        \
  vidTexEnvMode (  GL_COMBINE );        \
  vidComb_COMB_RGB  ( GL_INTERPOLATE );   \
  vidComb_SRC_RGB   (0 , GL_TEXTURE );    \
  vidComb_SRC_RGB   (1 , GL_PREVIOUS );   \
  vidComb_OP_RGB    (0 , GL_SRC_COLOR );  \
  vidComb_OP_RGB    (1 , GL_SRC_COLOR );  \
  vidComb_OP_RGB    (2 , ALPHA );

    case VID_DECAL:
      vidCombDECAL();
      vidComb_SRC_RGB (2 , GL_TEXTURE );
      break;
    case VID_DECAL_VERTEX:
      vidCombDECAL();
      vidComb_SRC_RGB (2 , GL_PRIMARY_COLOR );
      break;
    case VID_DECAL_PREV:
      vidCombDECAL();
      vidComb_SRC_RGB (2 , GL_PREVIOUS );
      break;
    case VID_DECAL_CONST:
      vidCombDECAL();
      vidComb_SRC_RGB (2 , GL_CONSTANT );
      break;

#undef vidCombDECAL

#define vidCombDECALCLR()       \
  vidTexEnvMode (  GL_COMBINE );        \
  vidComb_COMB_RGB  ( GL_INTERPOLATE );   \
  vidComb_SRC_RGB   (0 , GL_TEXTURE );    \
  vidComb_SRC_RGB   (1 , GL_PREVIOUS );   \
  vidComb_OP_RGB    (0 , GL_SRC_COLOR );  \
  vidComb_OP_RGB    (1 , GL_SRC_COLOR );  \
  vidComb_OP_RGB    (2 , COLOR );

    case VID_DECALCLR:
      vidCombDECALCLR();
      vidComb_SRC_RGB (2 , GL_TEXTURE );
      break;
    case VID_DECALCLR_VERTEX:
      vidCombDECALCLR();
      vidComb_SRC_RGB (2 , GL_PRIMARY_COLOR );
      break;
    case VID_DECALCLR_PREV:
      vidCombDECALCLR();
      vidComb_SRC_RGB (2 , GL_PREVIOUS );
      break;
    case VID_DECALCLR_CONST:
      vidCombDECALCLR();
      vidComb_SRC_RGB (2 , GL_CONSTANT );
      break;

#undef vidCombDECAL

      //  Require Special EXTENSIONS
      //  --------------------------

      //  Crossbar:
    case VID_ADD_C:
      vidTexEnvMode (  GL_COMBINE );
      vidComb_COMB_RGB  ( GL_ADD );
      vidComb_SRC_RGB (0 , GL_TEXTURE1_ARB );
      vidComb_SRC_RGB (1 , GL_TEXTURE );
      vidComb_OP_RGB    (0 , GL_SRC_COLOR );
      vidComb_OP_RGB    (1 , GL_SRC_COLOR );
      break ;

#define vidCombDECAL_C(SRC0,SRC1)       \
  vidTexEnvMode (  GL_COMBINE );        \
  vidComb_COMB_RGB  ( GL_INTERPOLATE );   \
  vidComb_SRC_RGB   (0 , SRC0 );  \
  vidComb_SRC_RGB   (1 , SRC1);   \
  vidComb_OP_RGB    (0 , GL_SRC_COLOR );  \
  vidComb_OP_RGB    (1 , GL_SRC_COLOR );  \
  vidComb_OP_RGB    (2 , ALPHA );

    case VID_DECAL_C:
      vidCombDECAL_C(GL_TEXTURE1_ARB,GL_TEXTURE);
      vidComb_SRC_RGB (2 , GL_TEXTURE1_ARB );
      break;
    case VID_DECAL_VERTEX_C:
      vidCombDECAL_C(GL_TEXTURE1_ARB,GL_TEXTURE);
      vidComb_SRC_RGB (2 , GL_PRIMARY_COLOR );
      break;
    case VID_DECAL_PREV_C:
      vidCombDECAL_C(GL_TEXTURE1_ARB,GL_TEXTURE);
      vidComb_SRC_RGB (2 , GL_PREVIOUS );
      break;
    case VID_DECAL_CONST_C:
      vidCombDECAL_C(GL_TEXTURE1_ARB,GL_TEXTURE);
      vidComb_SRC_RGB (2 , GL_CONSTANT );
      break ;

    case VID_DECAL_C2:
      vidCombDECAL_C(GL_TEXTURE2_ARB,GL_PREVIOUS);
      vidComb_SRC_RGB (2 , GL_TEXTURE2_ARB );
      break;
    case VID_DECAL_C3:
      vidCombDECAL_C(GL_TEXTURE3_ARB,GL_PREVIOUS);
      vidComb_SRC_RGB (2 , GL_TEXTURE3_ARB );
      break;


#undef vidCombDECAL_C

#define vidCombDECALCLR_C(SRC0,SRC1)        \
  vidTexEnvMode (  GL_COMBINE );        \
  vidComb_COMB_RGB  ( GL_INTERPOLATE );   \
  vidComb_SRC_RGB   (0 , SRC0 );  \
  vidComb_SRC_RGB   (1 , SRC1);   \
  vidComb_OP_RGB    (0 , GL_SRC_COLOR );  \
  vidComb_OP_RGB    (1 , GL_SRC_COLOR );  \
  vidComb_OP_RGB    (2 , COLOR );

    case VID_DECALCLR_C:
      vidCombDECALCLR_C(GL_TEXTURE1_ARB,GL_TEXTURE);
      vidComb_SRC_RGB (2 , GL_TEXTURE1_ARB );
      break;
    case VID_DECALCLR_VERTEX_C:
      vidCombDECALCLR_C(GL_TEXTURE1_ARB,GL_TEXTURE);
      vidComb_SRC_RGB (2 , GL_PRIMARY_COLOR );
      break;
    case VID_DECALCLR_PREV_C:
      vidCombDECALCLR_C(GL_TEXTURE1_ARB,GL_TEXTURE);
      vidComb_SRC_RGB (2 , GL_PREVIOUS );
      break;
    case VID_DECALCLR_CONST_C:
      vidCombDECALCLR_C(GL_TEXTURE1_ARB,GL_TEXTURE);
      vidComb_SRC_RGB (2 , GL_CONSTANT );
      break ;

    case VID_DECALCLR_C2:
      vidCombDECALCLR_C(GL_TEXTURE2_ARB,GL_PREVIOUS);
      vidComb_SRC_RGB (2 , GL_TEXTURE2_ARB );
      break;
    case VID_DECALCLR_C3:
      vidCombDECALCLR_C(GL_TEXTURE3_ARB,GL_PREVIOUS);
      vidComb_SRC_RGB (2 , GL_TEXTURE3_ARB );
      break;


#undef vidCombDECALCLR_C
      // ADDCMOD
    case VID_ADDCMOD_VERTEX:
      if  (GLEW_NV_texture_env_combine4){
        vidTexEnvMode     (  GL_COMBINE4_NV );
        vidComb_COMB_RGB  ( GL_ADD );
        vidComb_SRC_RGB   (2 , GL_PRIMARY_COLOR );
        vidComb_OP_RGB    (2 , GL_SRC_COLOR );
        vidComb_SRC_RGB   (3 , GL_TEXTURE );
        vidComb_OP_RGB    (3  , GL_SRC_COLOR );
        vidComb_SRC_RGB   (0 , GL_PRIMARY_COLOR );
        vidComb_OP_RGB    (0  , GL_SRC_COLOR );
        vidComb_SRC_RGB   (1 , GL_PREVIOUS );
        vidComb_OP_RGB    (1  , GL_SRC_COLOR );
        iscomb4 = LUX_TRUE;
      }
      break;
      // CMODADD
    case VID_CMODADD_VERTEX:
      vidComb_SRC_RGB (2 , GL_PRIMARY_COLOR );
      vidComb_OP_RGB    (2 , GL_SRC_COLOR );
      vidComb_SRC_ALPHA (2 , GL_PRIMARY_COLOR );
      vidComb_OP_ALPHA  (2 , GL_SRC_ALPHA );
      if  (GLEW_NV_texture_env_combine4){
        vidTexEnvMode (  GL_COMBINE4_NV );
        vidComb_COMB_RGB  ( GL_ADD );
        vidComb_SRC_RGB (3 , GL_TEXTURE );
        vidComb_OP_RGB    (3  , GL_SRC_COLOR );
        vidComb_SRC_RGB (0 , GL_ZERO );
        vidComb_OP_RGB    (0  , GL_ONE_MINUS_SRC_COLOR );
        vidComb_SRC_RGB (1 , GL_PREVIOUS );
        vidComb_OP_RGB    (1  , GL_SRC_COLOR );
        iscomb4 = LUX_TRUE;
      }
      else if   (GLEW_ATI_texture_env_combine3){
        vidTexEnvMode (  GL_COMBINE );
        vidComb_COMB_RGB  ( GL_MODULATE_ADD_ATI );
        vidComb_SRC_RGB (0 , GL_TEXTURE );
        vidComb_SRC_RGB (1 , GL_PREVIOUS );
        vidComb_OP_RGB    (0 , GL_SRC_COLOR );
        vidComb_OP_RGB    (1 , GL_SRC_COLOR );
      }
      break;
      // AMODADD

#define vidCombAMODADD()    \
  if  (GLEW_NV_texture_env_combine4){\
    vidTexEnvMode (  GL_COMBINE4_NV );\
    vidComb_COMB_RGB  ( GL_ADD );\
    vidComb_SRC_RGB (3 , GL_TEXTURE );\
    vidComb_OP_RGB    (3  , GL_SRC_COLOR );\
    vidComb_SRC_RGB (0 , GL_ZERO );\
    vidComb_OP_RGB    (0  , GL_ONE_MINUS_SRC_COLOR );\
    vidComb_SRC_RGB (1 , GL_PREVIOUS );\
    vidComb_OP_RGB    (1  , GL_SRC_COLOR );\
    iscomb4 = LUX_TRUE;\
  }\
  else if   (GLEW_ATI_texture_env_combine3){\
    vidTexEnvMode (  GL_COMBINE );\
    vidComb_COMB_RGB  ( GL_MODULATE_ADD_ATI );\
    vidComb_SRC_RGB (0 , GL_TEXTURE );\
    vidComb_SRC_RGB (1 , GL_PREVIOUS );\
    vidComb_OP_RGB    (0  , GL_SRC_COLOR );\
    vidComb_OP_RGB    (1 , GL_SRC_COLOR );\
  }
    case VID_AMODADD:
      vidCombAMODADD();
      vidComb_SRC_RGB (2 , GL_TEXTURE );
      vidComb_OP_RGB    (2 , GL_SRC_ALPHA );
      break;
    case VID_AMODADD_PREV:
      vidCombAMODADD();
      vidComb_SRC_RGB (2 , GL_PREVIOUS );
      vidComb_OP_RGB    (2 , GL_SRC_ALPHA );
      break;
    case VID_AMODADD_VERTEX:
      vidCombAMODADD();
      vidComb_SRC_RGB (2 , GL_PRIMARY_COLOR );
      vidComb_OP_RGB    (2 , GL_SRC_ALPHA );
      break;
    case VID_AMODADD_CONST:
      vidCombAMODADD();
      vidComb_SRC_RGB (2 , GL_CONSTANT );
      vidComb_OP_RGB    (2 , GL_SRC_ALPHA );
      break;
#undef vidCombAMODADD


#define vidCombAMOD()       \
  vidTexEnvMode (  GL_COMBINE );        \
  vidComb_COMB_RGB  (GL_MODULATE );     \
  vidComb_SRC_RGB   (0 , GL_TEXTURE );    \
  vidComb_OP_RGB    (0 , GL_SRC_COLOR );  \
  vidComb_OP_RGB    (1 , GL_SRC_ALPHA );

    case VID_AMOD:
      vidCombAMOD();
      vidComb_SRC_RGB (1 , GL_TEXTURE );
      break;
    case VID_AMOD_PREV:
      vidCombAMOD();
      vidComb_SRC_RGB (1 , GL_PREVIOUS );
      break;
    case VID_AMOD_VERTEX:
      vidCombAMOD();
      vidComb_SRC_RGB (1 , GL_PRIMARY_COLOR );
      break ;
    case VID_AMOD_CONST:
      vidCombAMOD();
      vidComb_SRC_RGB (1 , GL_CONSTANT);
      break ;
#undef vidCombAMOD

  }


  // combine4 needs a special case
  // because it only works with its own ADD mode
  // 0 * 1 + 2 * 3
  // so we must remap the commands
  if(iscomb4)
    switch(alphamode) {
#define vidCombA_REPLACE()            \
  vidComb_COMB_ALPHA  ( GL_ADD );\
  vidComb_OP_ALPHA  (0 , GL_SRC_ALPHA );\
  vidComb_SRC_ALPHA (1 , GL_ZERO );\
  vidComb_SRC_ALPHA (2 , GL_ZERO );\
  vidComb_SRC_ALPHA (3 , GL_ZERO );\
  vidComb_OP_ALPHA  (1 , GL_ONE_MINUS_SRC_ALPHA );\
  vidComb_OP_ALPHA  (2 , GL_SRC_ALPHA );\
  vidComb_OP_ALPHA  (3 , GL_SRC_ALPHA );\

    case VID_A_UNSET:
    case VID_A_REPLACE:
      vidCombA_REPLACE();
      vidComb_SRC_ALPHA (0, GL_TEXTURE);
      break;
    case VID_A_REPLACE_PREV:
      vidCombA_REPLACE();
      vidComb_SRC_ALPHA (0, GL_PREVIOUS);
      break;
    case VID_A_REPLACE_VERTEX:
      vidCombA_REPLACE();
      vidComb_SRC_ALPHA (0, GL_PRIMARY_COLOR);
      break;
    case VID_A_REPLACE_CONST:
      vidCombA_REPLACE();
      vidComb_SRC_ALPHA (0, GL_CONSTANT);
      break;


#define vidCombA_MODULATE()           \
  vidComb_COMB_ALPHA  ( GL_ADD );\
  vidComb_SRC_ALPHA (2 , GL_ZERO );\
  vidComb_SRC_ALPHA (3 , GL_ZERO );\
  vidComb_OP_ALPHA  (2 , GL_SRC_ALPHA );\
  vidComb_OP_ALPHA  (3 , GL_SRC_ALPHA );\

    case VID_A_MODULATE:
      vidCombA_MODULATE();
      vidComb_SRC_ALPHA (0, GL_PREVIOUS);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_SRC_ALPHA (1, GL_TEXTURE);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;
    case VID_A_CMOD:
      vidCombA_MODULATE();
      vidComb_SRC_ALPHA (0, GL_TEXTURE);
      vidComb_SRC_ALPHA (1, GL_PRIMARY_COLOR);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;
    case VID_A_CMOD_PREV:
      vidCombA_MODULATE();
      vidComb_SRC_ALPHA (0, GL_PREVIOUS);
      vidComb_SRC_ALPHA (1, GL_PRIMARY_COLOR);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;
    case VID_A_CONSTMOD_C:
      vidCombA_MODULATE();
      vidComb_SRC_ALPHA (0, GL_CONSTANT);
      vidComb_SRC_ALPHA (1, GL_TEXTURE1_ARB);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;
    case VID_A_CONSTMOD_C2:
      vidCombA_MODULATE();
      vidComb_SRC_ALPHA (0, GL_CONSTANT);
      vidComb_SRC_ALPHA (1, GL_TEXTURE2_ARB);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;
    case VID_A_CONSTMOD_C3:
      vidCombA_MODULATE();
      vidComb_SRC_ALPHA (0, GL_CONSTANT);
      vidComb_SRC_ALPHA (1, GL_TEXTURE3_ARB);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;

    case VID_A_SETBYCOLOR:
      break;
    default:
      {
        VIDTexCombiner_t *texcomb = &g_VID.gensetup.tcalpha[alphamode-VID_ALPHA_USER];
        int i;

        if (iscomb4 && !texcomb->combine4){
          switch(texcomb->mode){
          case GL_ADD:
            vidComb_COMB_ALPHA  ( GL_ADD );
            vidComb_SRC_ALPHA   (0, texcomb->srcs[0]);
            vidComb_OP_ALPHA    (0, texcomb->ops[0]);
            vidComb_SRC_ALPHA   (1, GL_ZERO);
            vidComb_OP_ALPHA    (1, GL_ONE_MINUS_SRC_ALPHA);

            vidComb_SRC_ALPHA   (2, texcomb->srcs[1]);
            vidComb_OP_ALPHA    (2, texcomb->ops[1]);
            vidComb_SRC_ALPHA   (3, GL_ZERO);
            vidComb_OP_ALPHA    (3, GL_ONE_MINUS_SRC_ALPHA);
            break;
          case GL_MODULATE:
            vidCombA_MODULATE();
            for(i=0; i < texcomb->numCmds; i++){
              vidComb_SRC_ALPHA   (i, texcomb->srcs[i]);
              vidComb_OP_ALPHA    (i, texcomb->ops[i]);
            }
            break;
          case GL_REPLACE:
            vidCombA_REPLACE();
            vidComb_SRC_ALPHA   (0, texcomb->srcs[0]);
            vidComb_OP_ALPHA    (0, texcomb->ops[0]);
            break;
          case GL_INTERPOLATE:
            vidComb_COMB_ALPHA  ( GL_ADD );
            vidComb_SRC_ALPHA   (0, texcomb->srcs[0]);
            vidComb_OP_ALPHA    (0, texcomb->ops[0]);
            vidComb_SRC_ALPHA   (1, texcomb->srcs[2]);
            vidComb_OP_ALPHA    (1, texcomb->ops[2]);

            vidComb_SRC_ALPHA   (2, texcomb->srcs[1]);
            vidComb_OP_ALPHA    (2, texcomb->ops[1]);
            vidComb_SRC_ALPHA   (3, texcomb->srcs[2]);
            vidComb_OP_ALPHA    (3, texcomb->ops[2] == GL_SRC_ALPHA ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA );
            break;
          case GL_ADD_SIGNED:
            vidComb_COMB_ALPHA  ( GL_ADD_SIGNED );
            vidComb_SRC_ALPHA   (0, texcomb->srcs[0]);
            vidComb_OP_ALPHA    (0, texcomb->ops[0]);
            vidComb_SRC_ALPHA   (1, GL_ZERO);
            vidComb_OP_ALPHA    (1, GL_ONE_MINUS_SRC_ALPHA);

            vidComb_SRC_ALPHA   (2, texcomb->srcs[1]);
            vidComb_OP_ALPHA    (2, texcomb->ops[1]);
            vidComb_SRC_ALPHA   (3, GL_ZERO);
            vidComb_OP_ALPHA    (3, GL_ONE_MINUS_SRC_ALPHA);
            break;
          default :
            break;
          }
        }
        else{
          vidComb_COMB_ALPHA  (texcomb->mode);
          for(i=0; i < texcomb->numCmds; i++){
            vidComb_SRC_ALPHA   (i, texcomb->srcs[i]);
            vidComb_OP_ALPHA    (i, texcomb->ops[i]);
          }
        }
      }
      break;
#undef vidCombA_REPLACE
#undef vidCombA_MODULATE
    }
  else
  switch(alphamode) {
#define vidCombA_REPLACE()            \
  vidComb_COMB_ALPHA  (GL_REPLACE);     \
  vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);

    case VID_A_UNSET:
    case VID_A_REPLACE:
      vidCombA_REPLACE();
      vidComb_SRC_ALPHA (0, GL_TEXTURE);
      break;
    case VID_A_REPLACE_PREV:
      vidCombA_REPLACE();
      vidComb_SRC_ALPHA (0, GL_PREVIOUS);
      break;
    case VID_A_REPLACE_VERTEX:
      vidCombA_REPLACE();
      vidComb_SRC_ALPHA (0, GL_PRIMARY_COLOR);
      break;
    case VID_A_REPLACE_CONST:
      vidCombA_REPLACE();
      vidComb_SRC_ALPHA (0, GL_CONSTANT);
      break;
#undef vidCombA_REPLACE
    case VID_A_MODULATE:
      vidComb_COMB_ALPHA  (GL_MODULATE);
      vidComb_SRC_ALPHA (0, GL_PREVIOUS);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_SRC_ALPHA (1, GL_TEXTURE);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;
    case VID_A_CMOD:
      vidComb_COMB_ALPHA  (GL_MODULATE );
      vidComb_SRC_ALPHA (0, GL_TEXTURE);
      vidComb_SRC_ALPHA (1, GL_PRIMARY_COLOR);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;
    case VID_A_CMOD_PREV:
      vidComb_COMB_ALPHA  (GL_MODULATE );
      vidComb_SRC_ALPHA (0, GL_PREVIOUS);
      vidComb_SRC_ALPHA (1, GL_PRIMARY_COLOR);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;
    case VID_A_CONSTMOD_C:
      vidComb_COMB_ALPHA  (GL_MODULATE );
      vidComb_SRC_ALPHA (0, GL_CONSTANT);
      vidComb_SRC_ALPHA (1, GL_TEXTURE1_ARB);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;
    case VID_A_CONSTMOD_C2:
      vidComb_COMB_ALPHA  (GL_MODULATE );
      vidComb_SRC_ALPHA (0, GL_CONSTANT);
      vidComb_SRC_ALPHA (1, GL_TEXTURE2_ARB);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;
    case VID_A_CONSTMOD_C3:
      vidComb_COMB_ALPHA  (GL_MODULATE );
      vidComb_SRC_ALPHA (0, GL_CONSTANT);
      vidComb_SRC_ALPHA (1, GL_TEXTURE3_ARB);
      vidComb_OP_ALPHA  (0, GL_SRC_ALPHA);
      vidComb_OP_ALPHA  (1, GL_SRC_ALPHA);
      break;
    case VID_A_SETBYCOLOR:
      break;
    default:
      {
        VIDTexCombiner_t *texcomb = &g_VID.gensetup.tcalpha[alphamode-VID_ALPHA_USER];
        int i;
        vidComb_COMB_ALPHA  (texcomb->mode);
        for(i=0; i < texcomb->numCmds; i++){
          vidComb_SRC_ALPHA   (i, texcomb->srcs[i]);
          vidComb_OP_ALPHA    (i, texcomb->ops[i]);
        }
      }
      break;
  }


  #undef vidComb_COMB_RGB
  #undef vidComb_SRC_RGB
  #undef vidComb_OP_RGB
  #undef vidComb_COMB_ALPHA
  #undef vidComb_SRC_ALPHA
  #undef vidComb_OP_ALPHA
  #undef vidTexEnvMode
}

VIDTexBlendHash VIDTexBlendHash_get(const VIDBlendColor_t blendmode,  VIDBlendAlpha_t alphamode,
                  const booln blendinvert, const int rgbscale, const int alphascale)
{
  enum32 hash = 0;
  hash = (((int)rgbscale)<<30) | (((int)alphascale) << 28) | (blendinvert << 27);
  hash |= blendmode | (alphamode << 16);

  LUX_ASSERT(alphamode < 2048 && blendmode < LUX_SHORT_UNSIGNEDMAX);

  return hash;
}

void vidTexBlendSetFromHash(const VIDTexBlendHash hash)
{
  VIDBlendColor_t blendmode = (VIDBlendColor_t)(hash & BIT_ID_FULL16);
  VIDBlendAlpha_t alphamode = (VIDBlendAlpha_t)((hash>>16) & 2047);
  int blendinvert = (hash>>27) & 1;
  int rgbscale = (hash>>28) & 3;
  int alphascale = (hash>>30) & 3;

  vidTexBlendSet(hash, blendmode,   alphamode,  blendinvert,  rgbscale, alphascale);
}


void vidBlend(const VIDBlendColor_t blendmode,const booln blendinvert)
{
  GLenum  src;
  GLenum  dst;
  GLenum  equ;

  if (blendmode == g_VID.state.blend.blendmode && blendinvert == g_VID.state.blend.blendinvert)
    return;

  vidBlendNative(blendmode,blendinvert,&src,&dst,&equ);
  glBlendFunc(src,dst);
  if (equ != g_VID.state.blendlastequ){
    glBlendEquation(equ);
  }
  g_VID.state.blend.blendinvert = blendinvert;
  g_VID.state.blend.blendmode = blendmode;
  g_VID.state.blendlastequ = equ;
}



//////////////////////////////////////////////////////////////////////////
// VIDBlendAlpha


#define bcstr(type) \
  case type:  return #type;

typedef struct VIDUserSearch_s{
  char name[256];
  int searchkey;
}VIDUserSearch_t;

void *VIDUserSearch_run(void *upvalue,const char *name,void *browse)
{
  VIDUserSearch_t*  srch = (VIDUserSearch_t*)upvalue;

  if (srch->searchkey == (int)browse){
    strcpy(srch->name,name);
  }

  return upvalue;
}

void  VIDUserDefine_add(const char *name)
{
  lxStrMap_set(l_VIDData.userdefines,name,(void*)LUX_TRUE);
}
void  VIDUserDefine_remove(const char *name)
{
  lxStrMap_remove(l_VIDData.userdefines,name);
}
void* VIDUserDefine_get(const char *name)
{
  if (name[0] == '!')
    return (void*)!lxStrMap_get(l_VIDData.userdefines,++name);
  else
    return lxStrMap_get(l_VIDData.userdefines,name);
}


const char * VIDBlendAlpha_toString(VIDBlendAlpha_t type){
  static VIDUserSearch_t srch;

  switch(type) {
  bcstr( VID_A_UNSET)
  bcstr( VID_A_SETBYCOLOR)
  bcstr( VID_A_REPLACE)
  // tex
  bcstr( VID_A_REPLACE_CONST)
  // const
  bcstr( VID_A_REPLACE_VERTEX)
  // primary
  bcstr( VID_A_REPLACE_PREV)
  // prev
  bcstr( VID_A_MODULATE)
  // prev*tex
  bcstr( VID_A_CMOD)
  // tex*prim
  bcstr( VID_A_CMOD_PREV)
  // prev*prim

  // crossbar
  bcstr( VID_A_CONSTMOD_C)
  // const*tex1
  bcstr( VID_A_CONSTMOD_C2)
  bcstr( VID_A_CONSTMOD_C3)
  default:
    {

      srch.name[0] = 0;
      srch.searchkey = type;
      lxStrMap_iterate(l_VIDData.alphamodes,VIDUserSearch_run,&srch);

      return srch.name;
    }
  }
}

const char * VIDBlendColor_toString(VIDBlendColor_t type){
  static VIDUserSearch_t srch;

  switch(type) {
    bcstr( VID_UNSET        )
    bcstr( VID_REPLACE      )
    // tex
    bcstr( VID_REPLACE_PREV     )
    //  prev
    bcstr( VID_REPLACE_VERTEX     )
    //  vertex
    bcstr( VID_REPLACE_CONST    )
    //  const
    bcstr( VID_MODULATE     )
    // prev*tex
    bcstr( VID_MODULATE2      )
    // prev*tex*2
    bcstr( VID_PASSTHRU)
    // prev

    bcstr(VID_MIN)
    bcstr(VID_MAX)
    bcstr(VID_SUB)
    bcstr(VID_AMODSUB)
    bcstr(VID_SUB_REV)
    bcstr(VID_AMODSUB_REV)

    bcstr( VID_DECAL        )
    // prev LERP(tex.alpha) tex
    bcstr( VID_DECAL_PREV     )
    // prev LERP(prev.alpha) tex
    bcstr( VID_DECAL_VERTEX     )
    // prev LERP(primary.alpha) tex
    bcstr( VID_DECAL_CONST    )
    // prev LERP(const.alpha) tex

    bcstr( VID_DECAL_CONSTMOD)
    // crossbar needed
    // will turn to bcstr( VID_DECAL_PREV
    // prev alpha = tex*const.alpha

    bcstr( VID_DECALCLR       )
    // prev LERP(tex.color) tex
    bcstr( VID_DECALCLR_PREV    )
    // prev LERP(prev.color) tex
    bcstr( VID_DECALCLR_VERTEX    )
    // prev LERP(vertex.color) tex
    bcstr( VID_DECALCLR_CONST   )
    // prev LERP(const.color) tex

    bcstr( VID_ADD        )
    // prev + tex
    bcstr( VID_ADD_INV      )
    // prev + tex.invert
    bcstr( VID_ADD_VERTEX   )
    // prev + vertex
    bcstr( VID_ADD_CONST    )
    // prev + tex.const
    bcstr( VID_AMODADD      )
    // prev + tex*tex.alpha
    bcstr( VID_AMODADD_PREV   )
    // prev + tex*prev.alpha
    bcstr( VID_AMODADD_VERTEX   )
    // prev + tex*primary.alpha
    bcstr( VID_AMODADD_CONST    )
    // prev + tex*const.alpha

    bcstr( VID_AMODADD_CONSTMOD)
    // crossbar needed
    // will turn to bcstr( VID_AMODADD_PREV
    // prev alpha = tex*const.alpha

    // internal helpers
    bcstr( VID_AMOD       )
    // tex*tex.alpha
    bcstr( VID_AMOD_PREV      )
    // tex*prev.alpha
    bcstr( VID_AMOD_VERTEX      )
    // tex*primary.alpha
    bcstr( VID_AMOD_CONST     )
    // tex*const.alpha

    bcstr( VID_CMOD     )
    // tex*primary
    bcstr( VID_CMOD_PREV        )
    //  prev*primary
    bcstr( VID_SQUARE     )
    //  color*color

    bcstr( VID_CMODADD_VERTEX   )
    // prev + tex*primary.color
    bcstr( VID_ADDCMOD_VERTEX   )
    // prev*primary.color + tex*primary.color

    // crossbar functionality
    // same as normal ones just that action
    // is between tex0 & tex1
    bcstr( VID_ADD_C        )
    bcstr( VID_DECAL_C          )
    bcstr( VID_DECAL_PREV_C     )
    bcstr( VID_DECAL_VERTEX_C   )
    bcstr( VID_DECAL_CONST_C    )
    // operation is between
    // texX & prev
    bcstr( VID_DECAL_C2     )
    bcstr( VID_DECAL_C3     )

    bcstr( VID_DECALCLR_C     )
    bcstr( VID_DECALCLR_PREV_C    )
    bcstr( VID_DECALCLR_VERTEX_C  )
    bcstr( VID_DECALCLR_CONST_C   )
    bcstr( VID_DECALCLR_C2      )
    bcstr( VID_DECALCLR_C3      )

  default:
    {
      srch.name[0] = 0;
      srch.searchkey = type;
      lxStrMap_iterate(l_VIDData.colormodes,VIDUserSearch_run,&srch);

      return srch.name;
    }
  }
}

#undef bcstr


VIDBlendAlpha_t VIDBlendAlpha_fromString(const char *name)
{
  return (VIDBlendAlpha_t)(int32)lxStrMap_get(l_VIDData.alphamodes,name);
}

VIDBlendColor_t VIDBlendColor_fromString(const char *name)
{
  return (VIDBlendColor_t)(int32)lxStrMap_get(l_VIDData.colormodes,name);
}


//////////////////////////////////////////////////////////////////////////
// VIDTexCombiner

VIDTexCombiner_t* VIDTexCombiner_alpha(const char *name)
{
  int id;
  if (id = (int)lxStrMap_get(l_VIDData.alphamodes,name)){
    if (id < VID_ALPHA_USER)
      return NULL;
    else
      return &g_VID.gensetup.tcalpha[id];
  }

  if (g_VID.gensetup.tcalphaCnt >= VID_MAX_USERCOMBINERS)
    return NULL;

  lxStrMap_set(l_VIDData.alphamodes,name,(void*)(g_VID.gensetup.tcalphaCnt+VID_ALPHA_USER));

  return &g_VID.gensetup.tcalpha[g_VID.gensetup.tcalphaCnt++];
}

VIDTexCombiner_t* VIDTexCombiner_color(const char *name)
{
  int id;
  if (id = (int)lxStrMap_get(l_VIDData.colormodes,name)){
    if (id < VID_COLOR_USER)
      return NULL;
    else
      return &g_VID.gensetup.tccolor[id];
  }

  if (g_VID.gensetup.tccolorCnt >= VID_MAX_USERCOMBINERS)
    return NULL;

  lxStrMap_set(l_VIDData.colormodes,name,(void*)(g_VID.gensetup.tccolorCnt+VID_COLOR_USER));

  return &g_VID.gensetup.tccolor[g_VID.gensetup.tccolorCnt++];
}


int VIDTexCombiner_prepCmd(VIDTexCombiner_t* comb, const GLenum cmd, const booln isalpha)
{
  memset(comb,0,sizeof(VIDTexCombiner_t));

  comb->mode = cmd;
  comb->combine4 = LUX_FALSE;

  switch(comb->mode)
  {
  case GL_REPLACE:
    comb->numCmds = 1;
    break;
  case GL_DOT3_RGBA:
  case GL_DOT3_RGB:
    if (isalpha)
      return 1;
  case GL_ADD:
  case GL_ADD_SIGNED:
  case GL_MODULATE:

  case GL_SUBTRACT:
    comb->numCmds = 2;

    break;
  case GL_INTERPOLATE:
    comb->numCmds = 3;

    break;
  case GL_MODULATE_ADD_ATI:
  case GL_MODULATE_SIGNED_ADD_ATI:
    comb->numCmds = 3;
    // special case in case we are not ATI

    if (GLEW_NV_texture_env_combine4){
      comb->combine4 = LUX_TRUE;
      if (comb->mode == GL_MODULATE_ADD_ATI)
        comb->mode = GL_ADD;
      else
        comb->mode = GL_ADD_SIGNED;

      comb->numCmds = 4;
      comb->ops[3] = isalpha ? GL_ONE_MINUS_SRC_ALPHA :  GL_ONE_MINUS_SRC_COLOR;
      comb->srcs[3] = GL_ZERO;
    }
    break;
  case (GL_COMBINE4_NV+GL_ADD):
    comb->combine4 = 2;
    comb->mode = GL_ADD;
    comb->numCmds = 4;
    break;
  case (GL_COMBINE4_NV+GL_ADD_SIGNED):
    comb->combine4 = 2;
    comb->mode = GL_ADD_SIGNED;
    comb->numCmds = 4;
    break;
  }

  return 0;
}

enum32 VIDTexCombiner_genFromString(int isalpha,const char *name)
{
  VIDTexCombiner_t *comb = isalpha ? VIDTexCombiner_alpha(name) : VIDTexCombiner_color(name);
  char buffer[256];
  char *cpy;
  const char *cmd;
  int arg;
  int fixarg;
  int tex;

  if (!comb)
    return VID_UNSET;

  cmd = name + strlen(VID_TEXCOMBSTRING);



  arg = -1;
  while (*cmd){
    cpy = buffer;
    while (*cmd && *cmd != ':' && *cmd != '|'){
      *cpy = *cmd;
      cpy++;
      cmd++;
    }
    *cpy = 0;

    if (arg < 0){
      enum32 cmdcode;

      if (!strcmp(buffer,"MOD"))
        cmdcode = GL_MODULATE;
      else if (!strcmp(buffer,"REP"))
        cmdcode = GL_REPLACE;
      else if (!strcmp(buffer,"SUB"))
        cmdcode = GL_SUBTRACT;
      else if (!strcmp(buffer,"ADD"))
        cmdcode = GL_ADD;
      else if (!strcmp(buffer,"ADDS"))
        cmdcode = GL_ADD_SIGNED;
      else if (!strcmp(buffer,"DOT3"))
        cmdcode = GL_DOT3_RGB;
      else if (!strcmp(buffer,"DOT3A"))
        cmdcode = GL_DOT3_RGBA;
      else if (!strcmp(buffer,"LERP"))
        cmdcode = GL_INTERPOLATE;
      else if (!strcmp(buffer,"MODADD"))
        cmdcode = GL_MODULATE_ADD_ATI;
      else if (!strcmp(buffer,"MODADDS"))
        cmdcode = GL_MODULATE_SIGNED_ADD_ATI;
      else if (!strcmp(buffer,"COMB4"))
        cmdcode = (GL_COMBINE4_NV+GL_ADD);
      else if (!strcmp(buffer,"COMB4S"))
        cmdcode = (GL_COMBINE4_NV+GL_ADD_SIGNED);
      else{
        lprintf("ERROR: Texcombinerstring: %s\n\toperationmode not set\n",name);
        return VID_UNSET;
      }

      if(VIDTexCombiner_prepCmd(comb,cmdcode,isalpha)){
        lprintf("ERROR: Texcombinerstring: %s\n\tillegal operationmode\n",name);
        return VID_UNSET;
      }
    }
    else{
      // modulate add is different for combine 4
      // ati = 0 * 2 + 1
      // nvcombine4 = 0 * 1 + 2 * 3
      if (comb->combine4==1 && arg)
        fixarg = arg%2 + 1;
      else
        fixarg = arg;

      if (strstr(buffer,"CONST"))
        comb->srcs[fixarg] = GL_CONSTANT;
      else if (strstr(buffer,"VERTEX"))
        comb->srcs[fixarg] = GL_PRIMARY_COLOR;
      else if (strstr(buffer,"PREV"))
        comb->srcs[fixarg] = GL_PREVIOUS;
      else if (sscanf(buffer,"TEX%d.",&tex))
        comb->srcs[fixarg] = GL_TEXTURE0_ARB + tex;
      else
        comb->srcs[fixarg] = GL_TEXTURE;

      cpy = strchr(buffer,'.');

      if (!cpy){
        lprintf("ERROR: Texcombinerstring: %s\n\toperands not found\n",name);
        return VID_UNSET;
      }
      cpy++;

      if (*cpy == 'C'){
        if (isalpha){
          lprintf("ERROR: Texcombinerstring: %s\n\tcolor not allowed in alphamode",name);
          return VID_UNSET;
        }
        comb->ops[fixarg] = strstr(cpy,"INV") ? GL_ONE_MINUS_SRC_COLOR : GL_SRC_COLOR;
      }
      else if (*cpy == 'A'){
        comb->ops[fixarg] = strstr(cpy,"INV") ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA;
      }
      else {
        lprintf("ERROR: Texcombinerstring: %s\n\toperands not found\n",name);
        return VID_UNSET;
      }

    }
    arg++;
    if (*cmd)
      cmd++;
  }

  if (arg != comb->numCmds && !comb->combine4){
    lprintf("ERROR: Texcombinerstring: %s\n\tinsufficient arguments\n",name);
    return VID_UNSET;
  }
  if (!comb->mode){
    lprintf("ERROR: Texcombinerstring: %s\n\tno operationmode defined\n",name);
    return VID_UNSET;
  }

  return isalpha ? VIDBlendAlpha_fromString(name) :  VIDBlendColor_fromString(name);
}

//////////////////////////////////////////////////////////////////////////
// Helpers


void vidScreenShot(int tga)
{
  static char filename[2048];

  GLubyte *imageBuffer = NULL;
  int width;
  int height;

  width = g_VID.windowWidth;
  height = g_VID.windowHeight;

  bufferminsize(sizeof(uchar)*4*width*height);
  imageBuffer = (GLubyte *)buffermalloc(sizeof(uchar)*4*width*height);
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

  glFinish();
  vidBindBufferPack(NULL);
  if(tga)
  {
    glReadPixels(0, 0, width, height, GLEW_EXT_bgra ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
    sprintf(filename, "%s_%05d.tga",l_VIDData.screenshotname, g_VID.screenshots);
    fileSaveTGA(filename, width, height, 4, imageBuffer,GLEW_EXT_bgra ? LUX_TRUE : LUX_FALSE);
  }
  else
  {
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, imageBuffer);
    sprintf(filename, "%s_%05d.jpg",l_VIDData.screenshotname, g_VID.screenshots);
    fileSaveJPG(filename, width, height,3, g_VID.screenshotquality, imageBuffer);
  }

  if (!g_VID.autodump)
    Console_printf("Wrote %s\n", filename);
  g_VID.screenshots++;
}

const char * VIDTechnique_toString(VID_Technique_t type)
{

  switch(type){
    case VID_T_DEFAULT :
      return "VID_DEFAULT";

    case VID_T_ARB_TEXCOMB:
      return "VID_ARB_TEXCOMB";

    case VID_T_ARB_V :
      return "VID_ARB_V";

    case VID_T_ARB_VF :
      return "VID_ARB_VF";

    case VID_T_ARB_VF_TEX8 :
      return "VID_ARB_VF_TEX8";

    case VID_T_CG_SM3 :
      return "VID_CG_SM3";

    case VID_T_CG_SM4 :
      return "VID_CG_SM4";

    case VID_T_CG_SM4_GS :
      return "VID_CG_SM4_GS";

    case VID_T_LOWDETAIL :
      return "VID_LOWDETAIL";
    default:
      return NULL;
  }
}


int VIDCompareMode_run(GLenum cmpmode, float val, float ref)
{
  switch(cmpmode)
  {
  case GL_GREATER:  return (val > ref);
  case GL_LESS:   return (val < ref);
  case GL_LEQUAL:   return (val <= ref);
  case GL_GEQUAL:   return (val >= ref);
  case GL_EQUAL:    return (val == ref);
  case GL_NOTEQUAL: return (val != ref);
  case GL_NEVER:    return (LUX_FALSE);
  case GL_ALWAYS:   return (LUX_TRUE);
  default:
    return 0;
  }
}

void    VIDViewBounds_set(VIDViewBounds_t *vbounds,booln fullwindow, int x, int y, int w, int h)
{
  vbounds->fullwindow = fullwindow;
  vbounds->x = x;
  vbounds->y = y;
  vbounds->width = w;
  vbounds->height = h;
}

//////////////////////////////////////////////////////////////////////////
// VIDBuffer

void VIDBuffer_clearGL(VIDBuffer_t* buffer)
{
  LUX_ASSERT(buffer->glID);

  if (buffer->mapped){
    VIDBuffer_bindGL(buffer,buffer->type);
    glUnmapBuffer(buffer->glType);
    VIDBuffer_bindGL(NULL,buffer->type);
  }

  glDeleteBuffers(1,&buffer->glID);

  LUX_PROFILING_OP (g_Profiling.global.memory.vidvbomem -= (int)(buffer->size));

  buffer->mapped = NULL;
  buffer->glID = 0;
}


void VIDBuffer_resetGL(VIDBuffer_t* buffer, void *data)
{
  static const GLenum hinttoGL[VID_BUFFERHINTS] = {
    GL_STREAM_DRAW,
    GL_STREAM_READ,
    GL_STREAM_COPY,
    GL_STATIC_DRAW,
    GL_STATIC_READ,
    GL_STATIC_COPY,
    GL_DYNAMIC_DRAW,
    GL_DYNAMIC_READ,
    GL_DYNAMIC_COPY,
  };
  void* buf = NULL;

  LUX_ASSERT(buffer->glID);

  VIDBuffer_bindGL(buffer,buffer->type);
  if (g_VID.capIsCrap && !data){
    data = buffermalloc(buffer->size);
    buf = data;
  }
  glBufferData(buffer->glType,buffer->size,data,hinttoGL[buffer->hint]);
  if (buf){
    buffersetbegin(buf);
    data = NULL;
  }

  buffer->used = data ? buffer->size : 0;
}

void VIDBuffer_initGL( VIDBuffer_t* buffer, VIDBufferType_t type, VIDBufferHint_t hint, uint size, void* data )
{
  static const GLenum gltypes[VID_BUFFERS] = {
    GL_ARRAY_BUFFER_ARB,GL_ELEMENT_ARRAY_BUFFER_ARB,
    GL_PIXEL_PACK_BUFFER,GL_PIXEL_UNPACK_BUFFER,
    GL_TEXTURE_BUFFER_EXT,
    GL_TRANSFORM_FEEDBACK_BUFFER_EXT,
    GL_COPY_WRITE_BUFFER,
    GL_COPY_READ_BUFFER,
  };

  LUX_ASSERT(buffer->glID == 0);

  buffer->glType = gltypes[type];
  buffer->type = type;
  buffer->hint = hint;
  buffer->mapped = NULL;
  buffer->size = size;
  buffer->used = 0;

  LUX_PROFILING_OP (g_Profiling.global.memory.vidvbomem += size);

  glGenBuffers(1,&buffer->glID);
  VIDBuffer_resetGL(buffer,data);
}

uint VIDBuffer_alloc(VIDBuffer_t *buffer, uint size, uint padsize)
{
  // pad to biggest
  uint oldused = buffer->used;
  uint oldsize = size;
  uint offset = (size%padsize);
  size += offset ? (padsize-offset) : 0;

  if (buffer->used + size > buffer->size)
    return (uint)-1;

  buffer->used += size;


  return oldused;
}

void* VIDBuffer_mapGL(VIDBuffer_t *buffer, VIDBufferMapType_t type)
{
  static const GLenum typetoGL[VID_BUFFERMAPS] = {
    GL_READ_ONLY,
    GL_WRITE_ONLY,
    GL_READ_WRITE,
    GL_WRITE_ONLY,
    GL_WRITE_ONLY,
  };

  LUX_ASSERT(!buffer->mapped);

  VIDBuffer_bindGL(buffer,buffer->type);

  if (type >= VID_BUFFERMAP_WRITEDISCARD && g_VID.capRangeBuffer){
    buffer->mapped = glMapBufferRange(buffer->glType,0,buffer->size,GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
  }
  else{
    buffer->mapped = glMapBuffer(buffer->glType,typetoGL[type]);
  }


  buffer->mapstart = 0;
  buffer->maplength = buffer->size;
  buffer->mappedend = ((byte*)buffer->mapped)+buffer->size;
  buffer->maptype = type;

  vidCheckError();

  return buffer->mapped;
}

void VIDBuffer_copy(VIDBuffer_t *dst, uint dstoffset, VIDBuffer_t *src, uint srcoffset, uint size)
{
  LUX_ASSERT(!dst->mapped && !src->mapped);
  LUX_ASSERT(dstoffset+size <= dst->size);
  LUX_ASSERT(srcoffset+size <= src->size);
  if (src == dst){
    LUX_ASSERT(dstoffset+size <= srcoffset || dstoffset >= srcoffset+size);
  }

  if (g_VID.capCPYBO){
    VIDBuffer_bindGL(dst,VID_BUFFER_CPYINTO);
    VIDBuffer_bindGL(src,VID_BUFFER_CPYFROM);

    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
      srcoffset, dstoffset,
      size);
  }
  else if (g_VID.capRangeBuffer || src != dst){
    // temporarily map both
    void* psrc = VIDBuffer_mapRangeGL(src,VID_BUFFERMAP_READ,srcoffset,size,LUX_FALSE,LUX_FALSE);
    void* pdst = VIDBuffer_mapRangeGL(dst,VID_BUFFERMAP_WRITEDISCARD,dstoffset,size,LUX_FALSE,LUX_FALSE);

    memcpy(pdst,psrc,size);

    // unmap
    VIDBuffer_unmapGL(src);
    VIDBuffer_unmapGL(dst);
  }
  else{
    // temporarily map
    byte* p = VIDBuffer_mapGL(src,VID_BUFFERMAP_READWRITE);
    byte* psrc = p+srcoffset;
    byte* pdst = p+dstoffset;

    memcpy(pdst,psrc,size);

    // unmap
    VIDBuffer_unmapGL(src);
  }
}

void VIDBuffer_submitGL(VIDBuffer_t *buffer, uint offset, uint size, void *data)
{
  LUX_ASSERT(!buffer->mapped);
  LUX_ASSERT(offset+size <= buffer->size);

  VIDBuffer_bindGL(buffer,buffer->type);
  glBufferSubDataARB(buffer->glType,offset,size,data);
}

void VIDBuffer_retrieveGL(VIDBuffer_t *buffer, uint offset, uint size, void *data)
{
  LUX_ASSERT(!buffer->mapped);
  LUX_ASSERT(offset+size <= buffer->size);

  VIDBuffer_bindGL(buffer,buffer->type);
  glGetBufferSubDataARB(buffer->glType,offset,size,data);
}

void* VIDBuffer_mapRangeGL(VIDBuffer_t *buffer, VIDBufferMapType_t type, uint from, uint length,  booln manualflush, booln unsynch)
{
  LUX_ASSERT(!buffer->mapped);

  VIDBuffer_bindGL(buffer,buffer->type);

  if (g_VID.capRangeBuffer)
  {
    static const GLbitfield bitfieldsGL[VID_BUFFERMAPS] = {
      GL_MAP_READ_BIT,
      GL_MAP_WRITE_BIT,
      GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
      GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT,
      GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
    };

    GLbitfield bitfield = bitfieldsGL[type];
    bitfield |= (manualflush ? GL_MAP_FLUSH_EXPLICIT_BIT : 0) | (unsynch ? GL_MAP_UNSYNCHRONIZED_BIT : 0);

    buffer->mapped = glMapBufferRange(buffer->glType,from,length,bitfield);
  }
  else{
    static const GLenum typetoGL[VID_BUFFERMAPS] = {
      GL_READ_ONLY,
      GL_WRITE_ONLY,
      GL_READ_WRITE,
      GL_WRITE_ONLY,
      GL_WRITE_ONLY,
    };

    buffer->mapped = ((byte*)glMapBuffer(buffer->glType,typetoGL[type])) + from;
  }

  buffer->mapstart = from;
  buffer->maplength = length;
  buffer->mappedend = ((byte*)buffer->mapped)+length;
  buffer->maptype = type;

  vidCheckError();

  return buffer->mapped;
}

void VIDBuffer_flushRangeGL(VIDBuffer_t *buffer, uint from, uint length)
{
  VIDBuffer_bindGL(buffer,buffer->type);

  LUX_ASSERT(buffer->mapped);

  glFlushMappedBufferRange(buffer->glType,from,length);

  vidCheckError();
}
void VIDBuffer_unmapGL(VIDBuffer_t *buffer)
{
  if(!buffer->mapped);

  VIDBuffer_bindGL(buffer,buffer->type);
  glUnmapBuffer(buffer->glType);

  buffer->maplength = buffer->mapstart = 0;
  buffer->mapped = buffer->mappedend = NULL;
  buffer->maptype = -1;
}

void VIDBuffer_unbindGL(VIDBuffer_t* vbuf)
{
  int i;
  for (i = 0; i < VID_BUFFERS; i++){
    if (g_VID.state.activeBuffers[i] == vbuf){
      VIDBuffer_bindGL(NULL,i);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//


lxScalarType_t ScalarType_fromGL(GLenum type)
{
  switch(type)
  {
  case GL_UNSIGNED_BYTE:
    return LUX_SCALAR_UINT8;
  case GL_BYTE:
    return LUX_SCALAR_INT8;
  case GL_UNSIGNED_SHORT:
    return LUX_SCALAR_UINT16;
  case GL_SHORT:
    return LUX_SCALAR_INT16;
  case GL_UNSIGNED_INT:
    return LUX_SCALAR_UINT8;
  case GL_INT:
    return LUX_SCALAR_INT8;
  case GL_FLOAT:
    return LUX_SCALAR_FLOAT32;
  case GL_HALF_FLOAT_ARB:
    return LUX_SCALAR_FLOAT16;
  case GL_DOUBLE:
    return LUX_SCALAR_FLOAT64;
  default:
    return (lxScalarType_t)0;
  }

}

GLenum ScalarType_toGL(lxScalarType_t type)
{
  static const GLenum types[LUX_SCALARS] = {
    GL_FLOAT,GL_BYTE,GL_UNSIGNED_BYTE,GL_SHORT,GL_UNSIGNED_SHORT,GL_INT,GL_UNSIGNED_INT,
    GL_HALF_FLOAT_ARB,GL_DOUBLE,0,
  };

  LUX_DEBUGASSERT(type < LUX_SCALARS);

  return types[type];
}


//////////////////////////////////////////////////////////////////////////

void vidStateDefault()  {
  flags32 defaultflag = 0;
  VIDLogicOp_t def = {0};
  VIDRenderFlag_setGL(defaultflag);

  /*
  g_VID.state.blend.blendinvert = 0;
  g_VID.state.blend.blendmode = (VIDBlendColor_t)0;
  g_VID.state.alpha.alphafunc = 0xffffffff; g_VID.state.alpha.alphaval = 0xffffffff;
  g_VID.state.depth.func = 0xffffffff;
  */
  memset(&g_VID.state.stencil,255,sizeof(VIDStencil_t));

  vidDepthFunc(GL_LEQUAL);
  VIDLogicOp_setGL(&def);

  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);

  g_VID.shdsetup.lightmapRID = -1;
}

void vidStateDisable()
{
  int i;

  // disable program state
  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);

  // disable texture state
  vidClearTexStages(0,i);

  // disable rest of state
  VIDRenderFlag_disable();
  glDisable(GL_LOGIC_OP);
  glDisable(GL_LINE_STIPPLE);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
}

void vidStateReset()
{
  int i;

  // general
  VIDRenderFlag_setGL(~g_VID.state.renderflag);
  glEnableClientState(GL_VERTEX_ARRAY);

  // viewport
  i = g_VID.state.viewport.bounds.x;
  g_VID.state.viewport.bounds.x = ~0;

  vidViewport(i,g_VID.state.viewport.bounds.y,g_VID.state.viewport.bounds.width,g_VID.state.viewport.bounds.height);
  vidDepthRange(g_VID.state.viewport.depth[0],g_VID.state.viewport.depth[1]);

  // blend & friends
  memset(&g_VID.state.alpha,~0, sizeof(g_VID.state.alpha));
  memset(&g_VID.state.depth,~0, sizeof(g_VID.state.depth));
  memset(&g_VID.state.blend,~0, sizeof(g_VID.state.blend));
  memset(&g_VID.state.blendlastequ,~0, sizeof(g_VID.state.blendlastequ));
  memset(&g_VID.state.stencil,~0, sizeof(g_VID.state.stencil));

  // programs
  vidUnSetProgs();

  // vertex state
  vidResetPointer();
  vidResetBuffers();
  // texture state
  g_VID.state.activeTex = -1;
  g_VID.state.activeClientTex = -1;
  for (i = 0; i < g_VID.capTexImages; i++){
    g_VID.state.textures[i] = -1;
  }
}



