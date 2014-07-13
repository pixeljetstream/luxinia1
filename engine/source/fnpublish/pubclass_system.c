// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/common.h"
#include "fnpublish.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../common/memorymanager.h"
#include "../common/reflib.h"
#include "../common/vid.h"
#include "../common/console.h"
#include "../render/gl_window.h"
#include "../render/gl_list3d.h"
#include "../main/main.h"
#include "../common/perfgraph.h"
#include <stdio.h>
#include "../fileio/tga.h"
#include "../fileio/jpg.h"
#include "../common/timer.h"
#include "../fileio/filesystem.h"

#include <stdio.h>
#include <stdlib.h>
// LUXI_CLASS_SYSTEM
// LUXI_CLASS_RESDATA,
// LUXI_CLASS_RESCHUNK,
// LUXI_CLASS_WINDOW
// LUXI_CLASS_FILESYSTEM,
// LUXI_CLASS_PGRAPH
// LUXI_CLASS_CAPABILITY
// LUXI_CLASS_RENDERSYS

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_SYSTEM, WINDOW

enum PubSys_Cmds_e{
  SYSCMD_BUFFERPOOLSIZE,
  SYSCMD_EXIT,
  SYSCMD_DETAIL,

  SYSCMD_WINRES,
  SYSCMD_WINREFSIZE,
  SYSCMD_WINSIZE,
  SYSCMD_WINMINSIZE,
  SYSCMD_WINRESSTRING,
  SYSCMD_WINFULLSCREEN,
  SYSCMD_WINPOS,
  SYSCMD_WINHEIGHT,
  SYSCMD_WINWIDTH,
  SYSCMD_WINUPDATE,
  SYSCMD_WINONTOP,
  SYSCMD_WINCOLORBITS,
  SYSCMD_WINSTENCILBITS,
  SYSCMD_WINDEPTHBITS,
  SYSCMD_WINSAMPLES,
  SYSCMD_WINBITS,
  SYSCMD_WINTITLE,
  SYSCMD_WINREADCOLOR,
  SYSCMD_WINREADSTENCIL,
  SYSCMD_WINREADDEPTH,
  SYSCMD_WINTEXMATRIX,
  SYSCMD_WINRESIZE,
  SYSCMD_WINRESIZERATIO,
  SYSCMD_WINCLIENT2SCREEN,
  SYSCMD_WINSCREEN2CLIENT,
  SYSCMD_WINSCREEN2REF,
  SYSCMD_WINREF2SCREEN,
  SYSCMD_WINICONIFY,
  SYSCMD_WINRESTORE,

  SYSCMD_PROJECTPATH,
  SYSCMD_MAXFPS,

  SYSCMD_SWAPBUFFERS,
  SYSCMD_SWAPINTERVAL,
  SYSCMD_PROCESSFRAME,
  SYSCMD_TIME,
  SYSCMD_TIMECUR,
  SYSCMD_REFCOUNT,
  SYSCMD_DUMPMEMSTATS,
  SYSCMD_MILLIS,
  SYSCMD_FRAME,

  SYSCMD_NOSOUND,

  SYSCMD_SCREENSHOTQUALITY,
  SYSCMD_AUTOSCREENSHOT,
  SYSCMD_SLEEP,

  SYSCMD_OSSTRING,
  SYSCMD_DRIVELIST,
  SYSCMD_DRIVETYPE,
  SYSCMD_DRIVESIZES,
  SYSCMD_DRIVELABEL,
  SYSCMD_MACADDRESS,
  SYSCMD_CUSTOMSTRING,
  SYSCMD_SCREENSIZE,
  SYSCMD_SCREENRES,
  SYSCMD_NUMPROC,

  SYSCMD_NODEFAULTGPUPROGS,
  SYSCMD_NOGLEXTENSIONS,
  SYSCMD_FORCE2TEX,
  SYSCMD_GLSTRING,
  SYSCMD_BATCHMAXTRIS,
  SYSCMD_BATCHMAXINDICES,
  SYSCMD_BONESPARAMS,
  SYSCMD_VBOS,
  SYSCMD_TEXANISO,
  SYSCMD_TEXANISOLEVEL,
  SYSCMD_TEXCOMPRESS,
  SYSCMD_BASESHADERS,
  SYSCMD_USESHADERS,
  SYSCMD_PUREVERTEX,
  SYSCMD_FORCEFINISH,
  SYSCMD_PUSHGL,
  SYSCMD_POPGL,
  SYSCMD_POPPEDGL,
  SYSCMD_RENDERVENDOR,
  SYSCMD_CHECKSTATE,
  SYSCMD_EXTSTRING,
  SYSCMD_FINISHGL,
  SYSCMD_DRIVERGL,

  SYSCMD_CGFORCEGLSLLOW,
  SYSCMD_CGHIGHPROFILE,
  SYSCMD_CGCOMPILERARGS,
  SYSCMD_CGPREFERCOMPILED,
  SYSCMD_CGDUMPCOMPILED,
  SYSCMD_CGDUMPBROKEN,
  SYSCMD_CGUSEOPTIMAL,
  SYSCMD_CGALLOWGLSL,
};
static int PubSystem_attributes (PState pstate,PubFunction_t *fn, int n)
{
  int  detail;
  int  pixels[2];
  float flt,w,h;
  char *out;
  lxVector4 vec4;

  switch((int)fn->upvalue) {
  case SYSCMD_NUMPROC:
    return FunctionPublish_returnInt(pstate,g_LuxiniaWinMgr.luxiGetNumberOfProcessors());
  case SYSCMD_FRAME:
    return FunctionPublish_returnInt(pstate,g_VID.frameCnt);
  case SYSCMD_RENDERVENDOR:
    return FunctionPublish_returnString(pstate,VID_GPUVendor_toString(g_VID.gpuvendor));
  case SYSCMD_CHECKSTATE:
    return FunctionPublish_returnBool(pstate,VIDState_testGL());
  case SYSCMD_EXTSTRING:
    return FunctionPublish_returnString(pstate,glGetString(GL_EXTENSIONS));
  case SYSCMD_DRIVERGL:
    FunctionPublish_setNRet(pstate,0,LUXI_CLASS_STRING,glGetString(GL_VENDOR));
    FunctionPublish_setNRet(pstate,1,LUXI_CLASS_STRING,glGetString(GL_RENDERER));
    FunctionPublish_setNRet(pstate,2,LUXI_CLASS_STRING,glGetString(GL_VERSION));
    return 3;
  case SYSCMD_DUMPMEMSTATS:
    GenMemory_dumpStats("source/memgeneric_stats.lua");
    DynMemory_dumpStats("source/memdynamic_stats.lua");
    return 0;
  case SYSCMD_FINISHGL:
    glFinish();
    return 0;
  case SYSCMD_PUSHGL:
    LuxWindow_pushGL();
    return 0;
  case SYSCMD_POPGL:
    LuxWindow_popGL();
    return 0;
  case SYSCMD_POPPEDGL:
    return FunctionPublish_returnBool(pstate,LuxWindow_poppedGL());
  case SYSCMD_CGALLOWGLSL:
    if (n==0) return FunctionPublish_returnBool(pstate,g_VID.cg.allowglsl);
    if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_VID.cg.allowglsl))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    break;
  case SYSCMD_CGCOMPILERARGS:
    if (n==0) return FunctionPublish_returnString(pstate,ResData_getCgCstringBase());
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&out))
      return FunctionPublish_returnError(pstate,"1 string required");
    ResData_setCgCstringBase(out);
    break;
  case SYSCMD_CGPREFERCOMPILED:
    if (n==0) return FunctionPublish_returnBool(pstate,g_VID.cg.prefercompiled);
    if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_VID.cg.prefercompiled))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    break;
  case SYSCMD_CGDUMPCOMPILED:
    if (n==0) return FunctionPublish_returnBool(pstate,g_VID.cg.dumpallcompiled);
    if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_VID.cg.dumpallcompiled))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    break;
  case SYSCMD_CGUSEOPTIMAL:
    if (n==0) return FunctionPublish_returnBool(pstate,g_VID.cg.useoptimal);
    if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_VID.cg.useoptimal))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    break;
  case SYSCMD_CGDUMPBROKEN:
    if (n==0) return FunctionPublish_returnBool(pstate,g_VID.cg.dumpbroken);
    if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_VID.cg.dumpbroken))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    break;
  case SYSCMD_CGFORCEGLSLLOW:
    detail = 0;
    if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&detail))
      return FunctionPublish_returnError(pstate,"1 boolean required");

    if (detail){
      g_VID.cg.arbVertexProfile =   g_VID.cg.glslVertexProfile;
      g_VID.cg.arbFragmentProfile = g_VID.cg.glslFragmentProfile;

      g_VID.cg.GLSL = LUX_TRUE;
    }
    else{
      g_VID.cg.arbVertexProfile =   cgGetProfile("arbvp1");
      g_VID.cg.arbFragmentProfile = cgGetProfile("arbfp1");

      g_VID.cg.GLSL = vidCgProfileisGLSL(g_VID.cg.vertexProfile) || vidCgProfileisGLSL(g_VID.cg.fragmentProfile);
    }
    return 0;
  case SYSCMD_CGHIGHPROFILE:
    {
      const char *vname,*fname;
      VID_Technique_t tech = g_VID.capTech;
      CGprofile vprof,fprof;

      if (FunctionPublish_getArg(pstate,3,LUXI_CLASS_STRING,(void*)&vname,LUXI_CLASS_STRING,(void*)&fname,LUXI_CLASS_TECHNIQUE,(void*)&tech)<2)
        return FunctionPublish_returnError(pstate,"2 strings [1 technique] required");

      vprof = cgGetProfile(vname);
      fprof = cgGetProfile(fname);

      if (tech > g_VID.origTech || !cgGLIsProfileSupported(vprof) || !cgGLIsProfileSupported(fprof))
        return FunctionPublish_returnError(pstate,"technique or profiles not supported");

      cgGLSetOptimalOptions(vprof);
      cgGLSetOptimalOptions(fprof);

      g_VID.cg.optimalinited = LUX_FALSE;
      g_VID.cg.vertexProfile = vprof;
      g_VID.cg.fragmentProfile = fprof;
      g_VID.capTech = tech;
      g_VID.cg.GLSL = vidCgProfileisGLSL(g_VID.cg.vertexProfile) || vidCgProfileisGLSL(g_VID.cg.fragmentProfile);
    }

    return 0;
  case SYSCMD_BASESHADERS:
    if (n == 0) return FunctionPublish_setRet(pstate,4,LUXI_CLASS_SHADER,(void*)g_VID.drawsetup.shadersdefault.ids[VID_SHADER_COLOR],
      LUXI_CLASS_SHADER,(void*)g_VID.drawsetup.shadersdefault.ids[VID_SHADER_COLOR_LM],
      LUXI_CLASS_SHADER,(void*)g_VID.drawsetup.shadersdefault.ids[VID_SHADER_TEX],
      LUXI_CLASS_SHADER,(void*)g_VID.drawsetup.shadersdefault.ids[VID_SHADER_TEX_LM]);
    else if (4!=FunctionPublish_getArg(pstate,4,LUXI_CLASS_SHADER,(void*)&g_VID.drawsetup.shadersdefault.ids[VID_SHADER_COLOR],\
      LUXI_CLASS_SHADER,(void*)&g_VID.drawsetup.shadersdefault.ids[VID_SHADER_COLOR_LM],\
      LUXI_CLASS_SHADER,(void*)&g_VID.drawsetup.shadersdefault.ids[VID_SHADER_TEX],\
      LUXI_CLASS_SHADER,(void*)&g_VID.drawsetup.shadersdefault.ids[VID_SHADER_TEX_LM]) ){
        return FunctionPublish_returnError(pstate,"4 shader required");
      }
      break;
  case SYSCMD_USESHADERS:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_VID.drawsetup.useshaders);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_VID.drawsetup.useshaders) || g_VID.drawsetup.shadersdefault.ids[0] < 0 || g_VID.drawsetup.shadersdefault.ids[1] < 0 || g_VID.drawsetup.shadersdefault.ids[2] < 0)
      return FunctionPublish_returnError(pstate,"1 bool required and shaders set");
    break;
  case SYSCMD_PUREVERTEX:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_VID.drawsetup.purehwvertex);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_VID.drawsetup.purehwvertex) || !g_VID.drawsetup.useshaders)
      return FunctionPublish_returnError(pstate,"1 bool required and useshaders active");
    break;
  case SYSCMD_FORCEFINISH:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_Draws.forceFinish);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.forceFinish);
    break;
  case SYSCMD_BATCHMAXTRIS:
    if (n == 0) return FunctionPublish_returnInt(pstate,g_VID.gensetup.batchMeshMaxTris);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_VID.gensetup.batchMeshMaxTris);
    break;
  case SYSCMD_BATCHMAXINDICES:
    if (n == 0) return FunctionPublish_returnInt(pstate,g_VID.gensetup.batchMaxIndices);
    else FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_VID.gensetup.batchMaxIndices);
    break;
  case SYSCMD_OSSTRING:
    return FunctionPublish_returnString(pstate,VID_GetOSString());
    break;
  case SYSCMD_GLSTRING:
    {
      const char *renderer = glGetString(GL_RENDERER);
      const char *version = glGetString(GL_VERSION);
      const char *vendor = glGetString(GL_VENDOR);

      return FunctionPublish_setRet(pstate,3,LUXI_CLASS_STRING,(void*)renderer,LUXI_CLASS_STRING,(void*)version,LUXI_CLASS_STRING,(void*)vendor);
    }
    break;
  case SYSCMD_EXIT:
    detail = 0;
    FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&detail);
    Main_exit(detail);
    break;
  case SYSCMD_SLEEP:
    if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&detail))
      return FunctionPublish_returnError(pstate,"1 int required");
    Main_sleep(detail);
    break;
  case SYSCMD_SCREENSHOTQUALITY:
    if (n==0) return FunctionPublish_returnInt(pstate,g_VID.screenshotquality);
    else if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_VID.screenshotquality))
      return FunctionPublish_returnError(pstate,"1 int required");
    break;
  case SYSCMD_AUTOSCREENSHOT:
    if (n==0) return FunctionPublish_setRet(pstate,2,LUXI_CLASS_BOOLEAN,(void*)g_VID.autodump,LUXI_CLASS_BOOLEAN,(void*)g_VID.autodumptga);
    else if (n<1 || 1>FunctionPublish_getArg(pstate,2,LUXI_CLASS_BOOLEAN,(void*)&g_VID.autodump,LUXI_CLASS_BOOLEAN,(void*)&g_VID.autodumptga))
      return FunctionPublish_returnError(pstate,"1 boolean [1 boolean] required");
    break;
  case SYSCMD_MILLIS:
    return FunctionPublish_returnFloat(pstate,(float)(getMicros()*0.001));
  case SYSCMD_DETAIL: // detail
    if (n==1){
      if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&detail))
        return FunctionPublish_returnError(pstate,"1 int required as arg");
      detail = LUX_MAX(VID_DETAIL_LOW,detail);
      detail = LUX_MIN(VID_DETAIL_HI,detail);
      g_VID.details = detail;
    }
    else
      return FunctionPublish_returnInt(pstate,g_VID.details);
    break;
  case SYSCMD_WINRESIZE:
    if (n==0) return FunctionPublish_returnInt(pstate,g_Window.resizemode);
    else if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&detail) || detail<0 || detail > 3)
      return FunctionPublish_returnError(pstate,"1 int [0..3] required");
    g_Window.resizemode = detail;
    LuxWindow_updatedResizeRef();
    break;
  case SYSCMD_WINSCREEN2REF:
    if (2 > FunctionPublish_getArg(pstate,2,FNPUB_TOVECTOR2(vec4)))
      return FunctionPublish_returnError(pstate,"2 float required");
    vec4[0] *= VID_REF_WIDTHSCALE;
    vec4[1] *= VID_REF_HEIGHTSCALE;
    return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR2(vec4));
  case SYSCMD_WINREF2SCREEN:
    if (2 > FunctionPublish_getArg(pstate,2,FNPUB_TOVECTOR2(vec4)))
      return FunctionPublish_returnError(pstate,"2 float required");
    vec4[0] = (float)VID_REF_WIDTH_TOPIXEL(vec4[0]);
    vec4[1] = (float)VID_REF_HEIGHT_TOPIXEL(vec4[1]);
    return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR2(vec4));
  case SYSCMD_WINRESIZERATIO:
    if (n==0)
      return FunctionPublish_setRet(pstate,2, FNPUB_FFLOAT(g_Window.refratioX),
                          FNPUB_FFLOAT(g_Window.refratioY));
    FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_FLOAT,w);
    FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_FLOAT,h);

    g_Window.refratioX = w;
    g_Window.refratioY = h;
    LuxWindow_updatedResizeRef();
    break;
  case SYSCMD_WINTEXMATRIX:
    return PubMatrix4x4_return(pstate,g_VID.drawsetup.texwindowMatrix);
  case SYSCMD_WINREFSIZE:
    if (n==0)
      return FunctionPublish_setRet(pstate,2, FNPUB_FFLOAT(g_VID.refScreenWidth),
                          FNPUB_FFLOAT(g_VID.refScreenHeight));
    FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_FLOAT,w);
    FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_FLOAT,h);

    g_VID.refScreenWidth = w;
    g_VID.refScreenHeight = h;
    break;
  case SYSCMD_WINMINSIZE:
    if (n==0)
      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)g_Window.minwidth,
      LUXI_CLASS_INT,(void*)g_Window.minheight);
    FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_INT,pixels[0]);
    FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_INT,pixels[1]);
    g_Window.minwidth = pixels[0];
    g_Window.minheight = pixels[1];
    break;
  case SYSCMD_WINSIZE:
    if (n==0)
      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)g_Window.width,
      LUXI_CLASS_INT,(void*)g_Window.height);
    FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_INT,pixels[0]);
    FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_INT,pixels[1]);
    g_Window.width = pixels[0];
    g_Window.height = pixels[1];
    break;
  case SYSCMD_WINREADCOLOR:
    if (n<2 || 2!=FunctionPublish_getArg(pstate,2,FNPUB_TOVECTOR2(vec4)))
      return FunctionPublish_returnError(pstate,"2 float required");
    pixels[0] = VID_REF_WIDTH_TOPIXEL(vec4[0]);
    pixels[1] = g_Window.height - (VID_REF_HEIGHT_TOPIXEL(vec4[1]));
    vidBindBufferPack(NULL);
    glReadPixels(pixels[0],pixels[1],1,1,GL_RGBA,GL_FLOAT,vec4);
    return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(vec4));
  case SYSCMD_WINREADSTENCIL:
    if (n<2 || 2!=FunctionPublish_getArg(pstate,2,FNPUB_TOVECTOR2(vec4)))
      return FunctionPublish_returnError(pstate,"2 float required");
    pixels[0] = VID_REF_WIDTH_TOPIXEL(vec4[0]);
    pixels[1] = g_Window.height - (VID_REF_HEIGHT_TOPIXEL(vec4[1]));
    vidBindBufferPack(NULL);
    glReadPixels(pixels[0],pixels[1],1,1,GL_STENCIL_INDEX,GL_INT,&detail);
    return FunctionPublish_returnInt(pstate,detail);
  case SYSCMD_WINREADDEPTH:
    if (n<2 || 2!=FunctionPublish_getArg(pstate,2,FNPUB_TOVECTOR2(vec4)))
      return FunctionPublish_returnError(pstate,"2 float required");
    pixels[0] = VID_REF_WIDTH_TOPIXEL(vec4[0]);
    pixels[1] = g_Window.height - (VID_REF_HEIGHT_TOPIXEL(vec4[1]));
    vidBindBufferPack(NULL);
    glReadPixels(pixels[0],pixels[1],1,1,GL_DEPTH_COMPONENT,GL_FLOAT,vec4);
    return FunctionPublish_returnFloat(pstate,vec4[0]);
  case SYSCMD_WINBITS:
    if (n==0)return FunctionPublish_returnInt(pstate,g_Window.bpp);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_Window.bpp) ||
      (g_Window.bpp != 16 && g_Window.bpp != 24 && g_Window.bpp != 32))
      return FunctionPublish_returnError(pstate,"1 int (16,24,32) required");
    break;
  case SYSCMD_WINSAMPLES:
    detail = 0;
    if (n==0)return FunctionPublish_returnInt(pstate,g_Window.samples);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&detail)  || detail%2 || detail > 16)
      return FunctionPublish_returnError(pstate,"1 int (<=16, even or 0) required");
    g_Window.samples = detail;
    break;
  case SYSCMD_WINSTENCILBITS:
    if (n==0)return FunctionPublish_returnInt(pstate,g_Window.bits[LUXWIN_BIT_STENCIL]);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_Window.bits[LUXWIN_BIT_STENCIL]))
      return FunctionPublish_returnError(pstate,"1 int required");
    break;
  case SYSCMD_WINDEPTHBITS:
    if (n==0)return FunctionPublish_returnInt(pstate,g_Window.bits[LUXWIN_BIT_DEPTH]);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_Window.bits[LUXWIN_BIT_DEPTH]))
      return FunctionPublish_returnError(pstate,"1 int required");
    break;
  case SYSCMD_WINHEIGHT:
    if (n==0)return FunctionPublish_returnInt(pstate,g_Window.height);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_Window.height))
      return FunctionPublish_returnError(pstate,"1 int required");
    break;
  case SYSCMD_WINWIDTH:
    if (n==0)return FunctionPublish_returnInt(pstate,g_Window.width);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_Window.width))
      return FunctionPublish_returnError(pstate,"1 int required");
    break;
  case SYSCMD_WINRES: // winres
    if (n==1){
      if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&detail))
        return FunctionPublish_returnError(pstate,"1 int required");
      detail = LUX_MIN (5,detail);
      detail = LUX_MAX (0,detail);

      switch(detail) {
      case 0: out = "640x480x16"; break;
      case 1: out = "800x600x16"; break;
      case 2: out = "1024x768x16";  break;
      case 3: out = "640x480x32"; break;
      case 4: out = "800x600x32"; break;
      case 5: out = "1024x768x32";  break;
      default:
        break;
      }

      LuxWindow_setFromString(&g_Window,out);
    }
    else{
      out = LuxWindow_getString(&g_Window);
      if    (strstr(out,"640x480x16")) return FunctionPublish_returnInt(pstate,0);
      else if (strstr(out,"800x600x16")) return FunctionPublish_returnInt(pstate,1);
      else if (strstr(out,"1024x768x16")) return FunctionPublish_returnInt(pstate,2);
      else if (strstr(out,"640x480x32")) return FunctionPublish_returnInt(pstate,3);
      else if (strstr(out,"800x600x32")) return FunctionPublish_returnInt(pstate,4);
      else if (strstr(out,"1024x768x32")) return FunctionPublish_returnInt(pstate,5);

    }
    break;
  case SYSCMD_WINRESSTRING:
    if (n==1){
      if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&out))
        return FunctionPublish_returnError(pstate,"1 string required");

      LuxWindow_setFromString(&g_Window,out);
    }
    else{
      return FunctionPublish_returnString(pstate,LuxWindow_getString(&g_Window));
    }
    break;
    case SYSCMD_WINPOS: // window position
        {
            int x,y;
      if (n==0){
        if(LuxWindow_getPos(&x,&y));
          return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)x,LUXI_CLASS_INT,(void*)y);
      }
            if (n<1 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_INT,&x,LUXI_CLASS_INT,&y)!=2)
                return FunctionPublish_returnError(pstate,"2 ints required");
            LuxWindow_setPos(x,y);
        }
        break;

  case SYSCMD_WINCLIENT2SCREEN:
    {
      int x,y;
      if (n<1 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_INT,&x,LUXI_CLASS_INT,&y)!=2)
        return FunctionPublish_returnError(pstate,"2 ints required");
      if(g_LuxiniaWinMgr.luxiClientToScreen(&x,&y))
        return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)x,LUXI_CLASS_INT,(void*)y);
    }
    break;
  case SYSCMD_WINSCREEN2CLIENT:
    {
      int x,y;
      if (n<1 || FunctionPublish_getArg(pstate,2,LUXI_CLASS_INT,&x,LUXI_CLASS_INT,&y)!=2)
        return FunctionPublish_returnError(pstate,"2 ints required");
      if(g_LuxiniaWinMgr.luxiScreenToClient(&x,&y))
        return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)x,LUXI_CLASS_INT,(void*)y);
    }
    break;
  case SYSCMD_WINFULLSCREEN:  // winfullscreen
    if (n==1){
      if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Window.fullscreen))
        return FunctionPublish_returnError(pstate,"1 boolean required");
    }
    else
      return FunctionPublish_returnBool(pstate,g_Window.fullscreen);
    break;
  case SYSCMD_WINONTOP:
    if (n==0)return FunctionPublish_returnInt(pstate,g_Window.ontop);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&detail))
      return FunctionPublish_returnError(pstate,"1 int required");
    if (detail != g_Window.ontop){
      g_Window.ontop = detail;
      g_LuxiniaWinMgr.luxiSetWindowOnTop(detail);
    }
    break;
  case SYSCMD_WINUPDATE:  // update
    /*if (g_Window.fullscreen && !strstr(g_Window.resstring,"640x480") &&
      !strstr(g_Window.resstring,"800x600") &&
      !strstr(g_Window.resstring,"1024x768") &&
      !strstr(g_Window.resstring,"1152x864") &&
      !strstr(g_Window.resstring,"1280x768") &&
      !strstr(g_Window.resstring,"1280x1024") &&
      !strstr(g_Window.resstring,"1400x1050") &&
      !strstr(g_Window.resstring,"1600x1200"))
      return FunctionPublish_returnError(pstate,"untypical fullscreen resolution");*/
    //LuxWindow_updateRes(&g_Window,g_Window.resstring);
    LuxWindow_setGL(&g_Window);
    break;
  case SYSCMD_WINTITLE: // title
    {
      const char *str;
      FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_STRING,str);
      g_LuxiniaWinMgr.luxiSetWindowTitle(str);
    }
    break;
  case SYSCMD_WINCOLORBITS: // bits

    return FunctionPublish_setRet(pstate,4,
      LUXI_CLASS_INT,(void*)g_Window.bits[LUXWIN_BIT_RED],
      LUXI_CLASS_INT,(void*)g_Window.bits[LUXWIN_BIT_GREEN],
      LUXI_CLASS_INT, (void*)g_Window.bits[LUXWIN_BIT_BLUE],
      LUXI_CLASS_INT, (void*)g_Window.bits[LUXWIN_BIT_ALPHA]);

    break;

  case SYSCMD_WINICONIFY:
    {
      g_LuxiniaWinMgr.luxiIconifyWindow();
    }
    break;
  case SYSCMD_WINRESTORE:
    {
      g_LuxiniaWinMgr.lxRestoreWindow();
    }
    break;
  case SYSCMD_TEXCOMPRESS:  // texcompress
    if (n==1){
      if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_VID.useTexCompress))
        return FunctionPublish_returnError(pstate,"1 boolean required");
    }
    else
      return FunctionPublish_returnBool(pstate,g_VID.useTexCompress);
    break;
  case SYSCMD_TEXANISO: // texansio
    if (n==1){
      if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_VID.useTexAnisotropic))
        return FunctionPublish_returnError(pstate,"1 boolean required");
    }
    else
      return FunctionPublish_returnBool(pstate,g_VID.useTexAnisotropic);
    break;
  case SYSCMD_TEXANISOLEVEL:  // texanisolevel
    if (n==1){
      if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_FLOAT,(void*)&flt) || (g_VID.capTexAnisotropic && flt > g_VID.capTexAnisotropic) || flt < 1.0f)
        return FunctionPublish_returnError(pstate,"1 float [1,maxanisotropic] required");
      g_VID.lvlTexAnisotropic = flt;
    }
    else
      return FunctionPublish_returnFloat(pstate,g_VID.lvlTexAnisotropic);
    break;
  case SYSCMD_PROJECTPATH:
    {
      const char *str;
      if (n==0) return FunctionPublish_returnString(pstate,g_projectpath);
      FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_STRING,str);
      n = strlen(str);
      lxMemGenFree(g_projectpath,strlen(g_projectpath)+1);
      g_projectpath = lxMemGenZalloc(sizeof(char)*(n+2));
      strcpy(g_projectpath,str);
      g_projectpath[n]='/';
    }
    break;
  case SYSCMD_MAXFPS: // maxfps
    if (n==1){
      if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&detail))
        return FunctionPublish_returnError(pstate,"1 int required as arg");
      detail = LUX_MAX(0,detail);
      g_VID.maxFps = detail;
      if (g_VID.maxFps  < 15 && g_VID.maxFps != 0)
        g_VID.maxFps = 15;
    }
    else
      return FunctionPublish_returnInt(pstate,g_VID.maxFps);
    break;
  case SYSCMD_BONESPARAMS: // bonesparams
    if (n==1){
      if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_VID.gensetup.hwbonesparams))
        return FunctionPublish_returnError(pstate,"1 int required as arg");

      // extreme HACK, only update shd_hwbones when we have a gl context
      // we abuse the fact that drw_meshs are initialized in initGL
      if (g_VID.drw_meshquad){
        glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_MAX_PROGRAM_PARAMETERS_ARB,&g_VID.gensetup.hwbones);
        g_VID.gensetup.hwbones -= g_VID.gensetup.hwbonesparams;
        g_VID.gensetup.hwbones/=3;
      }
    }
    else
      return FunctionPublish_returnInt(pstate,g_VID.gensetup.hwbonesparams);
  case SYSCMD_VBOS:
    if (n == 0) return FunctionPublish_returnBool(pstate,g_VID.usevbos);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_VID.usevbos))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    g_VID.configset = LUX_TRUE;
    break;
  case SYSCMD_SWAPBUFFERS:
    g_LuxiniaWinMgr.luxiSwapBuffers();
    if (g_Draws.forceFinish)
      glFinish();
    break;
  case SYSCMD_SWAPINTERVAL:
    if (n == 0) return FunctionPublish_returnInt(pstate,g_VID.swapinterval);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&g_VID.swapinterval))
      return FunctionPublish_returnError(pstate,"1 int required");

    g_LuxiniaWinMgr.luxiSetSwapInterval(g_VID.swapinterval);
    break;
  case SYSCMD_PROCESSFRAME:
    // negative timediff means autocompute
    detail = LUX_FALSE;
    flt = -1.0f;
    FunctionPublish_getArg(pstate,2,FNPUB_TFLOAT(flt),LUXI_CLASS_INT,(void*)&detail);

    Main_think(flt,LUX_FALSE,detail);
    break;
  case SYSCMD_TIMECUR:
    return FunctionPublish_returnFloat(pstate,(float)g_LuxiniaWinMgr.luxiGetTime());
  case SYSCMD_TIME:
    return FunctionPublish_setRet(pstate,3,
      LUXI_CLASS_INT,g_LuxTimer.time,FNPUB_FFLOAT(g_LuxTimer.timediff),FNPUB_FFLOAT((float)g_LuxTimer.avgtimediff));
  case SYSCMD_REFCOUNT:
    return FunctionPublish_returnInt(pstate,Reference_getTotalCount());

  case SYSCMD_NODEFAULTGPUPROGS:
    if (n==0) return FunctionPublish_returnBool(pstate,g_VID.novprogs);
    FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_BOOLEAN,g_VID.novprogs);
    g_VID.configset = LUX_TRUE;
    break;
  case SYSCMD_NOGLEXTENSIONS:
    if (n==0) return FunctionPublish_returnBool(pstate,g_VID.useworst);
    FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_BOOLEAN,g_VID.useworst);
    break;
  case SYSCMD_NOSOUND:
    if (n==0) return FunctionPublish_returnBool(pstate,g_nosound);
    FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_BOOLEAN,g_nosound);
    break;
  case SYSCMD_FORCE2TEX:
    if (n==0) return FunctionPublish_returnBool(pstate,g_VID.force2tex);
    FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_BOOLEAN,g_VID.force2tex);
    break;
  case SYSCMD_CUSTOMSTRING:
    {
      const char *str = NULL;
      FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_STRING,str);
      return FunctionPublish_returnString(pstate,g_LuxiniaWinMgr.luxiGetCustomStrings(str));
    }
  case SYSCMD_MACADDRESS:
    return FunctionPublish_returnString(pstate,g_LuxiniaWinMgr.luxiGetMACaddress());
  case SYSCMD_DRIVELIST:
    {
      char drives[64][32];
      char *drivelookup[64];
      for (n=0; n < 64; n++)
        drivelookup[n] = drives[n];

      detail = g_LuxiniaWinMgr.luxiGetDrives(drivelookup,64,32);
      for(n=0; n < detail; n++)
      {
        FunctionPublish_setNRet (pstate,n, LUXI_CLASS_STRING, drives[n]);
      }
      return detail;
    }
    break;
  case SYSCMD_DRIVETYPE:
    {
      char *drv;
      const char *label;

      FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_STRING,drv);
      label = g_LuxiniaWinMgr.luxiGetDriveType(drv);
      if (label) {
        return FunctionPublish_returnString(pstate,label);
      }
    }
    break;
  case SYSCMD_DRIVESIZES:
    {
      char *drv;
      int found;
      double da,db,dc;
      float a,b,c;

      FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_STRING,drv);
      found = g_LuxiniaWinMgr.luxiGetDriveSize(drv,&da,&db,&dc);
      if (found) {
        a=(float)da;
        b=(float)db;
        c=(float)dc;
        return FunctionPublish_setRet(pstate,3,FNPUB_TFLOAT(a),FNPUB_TFLOAT(b),FNPUB_TFLOAT(c));
      }
    }
    break;
  case SYSCMD_DRIVELABEL:
    {
      char *drv;
      const char *label;

      FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_STRING,drv);
      label = g_LuxiniaWinMgr.luxiGetDriveLabel(drv);
      if (label) {
        return FunctionPublish_returnString(pstate,label);
      }
    }
    break;
  case SYSCMD_SCREENSIZE:
    {
      int x,y;
      g_LuxiniaWinMgr.luxiGetScreenSizeMilliMeters(&x,&y);
      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)x,LUXI_CLASS_INT,(void*)y);
    }
    break;
  case SYSCMD_SCREENRES:
    {
      int x,y;
      g_LuxiniaWinMgr.luxiGetScreenRes(&x,&y);
      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)x,LUXI_CLASS_INT,(void*)y);
    }
    break;
  case SYSCMD_BUFFERPOOLSIZE:
    if (n==0) return FunctionPublish_returnInt(pstate,lxMemoryStack_bytesTotal(g_BufferMemStack));
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&detail) && detail > 262144)
      return FunctionPublish_returnError(pstate,"1 integer (>262144) required");

    lxMemoryStack_initMin(g_BufferMemStack,detail);

    List3D_updateMemPoolRelated();
    break;
  default:
    break;
  }

  return 0;
}
static int PubSystem_vid (PState pstate,PubFunction_t *fn, int n)
{
  VID_info(0,Console_printf);
  VID_info(1,Console_printf);
  VID_info(2,Console_printf);
  return 0;
}

static int PubSystem_pause(PState pstate,PubFunction_t *fn, int n){
  byte state;

  if (n!= 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&state))
    return FunctionPublish_returnError(pstate,"1 boolean required");
  Main_pause(state);

  return 0;
}

static int PubSystem_version(PState pstate,PubFunction_t *fn, int n) {
  return FunctionPublish_setRet(pstate,3,LUXI_CLASS_STRING,LUX_VERSION,LUXI_CLASS_STRING,__DATE__,LUXI_CLASS_STRING,__TIME__);
}

static int PubSystem_screenshot(PState pstate,PubFunction_t *fn, int n)
{
  static char filename[1024];

  GLubyte *imageBuffer = NULL;
  char *name;
  int width;
  int height,res,i,quality,j,q;
  int snapw,snaph,snapx,snapy;
  size_t snapsize;

  g_LuxiniaWinMgr.luxiGetWindowSize(&width,&height);
  snapw = width;
  snaph = height;
  snapx = 0;
  snapy = 0;

  res = 0;
  quality = 85;

  if (n < 1 || 1>FunctionPublish_getArg(pstate,7,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_INT,(void*)&res,LUXI_CLASS_INT,(void*)&quality,LUXI_CLASS_INT,(void*)&snapx,LUXI_CLASS_INT,(void*)&snapy,LUXI_CLASS_INT,(void*)&snapw,LUXI_CLASS_INT,(void*)&snaph) ||
      res<0||res>3 || quality<1||quality>100)
      return FunctionPublish_returnError(pstate,"1 string [1 int (0..4)] [1 int (1..100)] [4 int 0..winwidth, 0..winheight] required.");
  res = 1<<res;


  // cap snaps
  snapx = LUX_MAX(LUX_MIN(snapx,width-4),0);
  snapy = LUX_MAX(LUX_MIN(snapy,height-4),0);

  snapw = LUX_MAX(LUX_MIN(snapw,width-snapx),4);
  snaph = LUX_MAX(LUX_MIN(snaph,height-snapy),4);


  snapsize = sizeof(uchar)*4*snapw*snaph;

  bufferminsize(snapsize);
  imageBuffer = buffermalloc(snapsize);
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

  if(imageBuffer == NULL)
  {
    return 0;
  }
  glFinish();
  vidBindBufferPack(NULL);

  strcpy(filename, name);
  name = filename;
  while (*name) name++;

  width = snapw;
  height = snaph;

  if (*(int*)&name[-4] == *(int*)".tga")
  {
    glReadPixels(snapx, snapy, snapw, snaph, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
    if (res!=1) {
      uchar *out = imageBuffer;

      snapw /= res;
      snaph /= res;

      for (j=0;j < snaph; j++) {
        for (i=0; i<snapw; i++,out+=4) {
          q = (j*res*width)+(i*res);
          q *= 4;
          out[0] = imageBuffer[q+0];
          out[1] = imageBuffer[q+1];
          out[2] = imageBuffer[q+2];
          out[3] = imageBuffer[q+3];
        }
      }
    }
  }
  else{
    glReadPixels(snapx, snapy, snapw, snaph, GL_RGB, GL_UNSIGNED_BYTE, imageBuffer);
    if (res!=1) {
      uchar *out = imageBuffer;

      snapw /= res;
      snaph /= res;

      for (j=0;j < snaph; j++) {
        for (i=0; i<snapw; i++,out+=3) {
          q = (j*res*width)+(i*res);
          q *= 3;
          out[0] = imageBuffer[q+0];
          out[1] = imageBuffer[q+1];
          out[2] = imageBuffer[q+2];
        }
      }
    }
  }


  if (*(int*)&name[-4] == *(int*)".tga")
    fileSaveTGA(filename, snapw, snaph, 4, imageBuffer,LUX_FALSE);
  else if (*(int*)&name[-4] == *(int*)".jpg")
    fileSaveJPG(filename, snapw, snaph, 3, quality, imageBuffer);
  else return FunctionPublish_returnError(pstate,"suffix of file must be either .jpg or .tga (case sensitive!)");

  return 0;
}


//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_RESDATA

static int PubResources_print(PState pstate,PubFunction_t *fn, int n)
{
  ResData_print();
  return 0;
}

static int PubResources_getCnt(PState pstate,PubFunction_t *fn, int n)
{
  ResourceType_t type;
  int up = (int)fn->upvalue;

  if (up<2 && (n!=1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_INT,(void*)&type)))
    return FunctionPublish_returnError(pstate,"1 int required");

  switch(up) {
  case 0: // load
    return FunctionPublish_returnInt(pstate,ResData_getLoadCnt(type));
  case 1: // open
    return FunctionPublish_returnInt(pstate,ResData_getOpenCnt(type));
  case 2: // resCnt
    return FunctionPublish_returnInt(pstate,RESOURCES);

  default:
    return 0;
  }


}

static int PubResources_printDescr(PState pstate,PubFunction_t *fn, int n)
{
  ResourceType_t type;

  type = (ResourceType_t)fn->upvalue;

  Console_printf("%s: defaultpath: %s maxCnt: %d CoreCnt:%d\n",g_ResDescriptor[type].name,g_ResDescriptor[type].path,g_ResDescriptor[type].max,g_ResDescriptor[type].core);

  return 0;
}

static int PubResources_strategy(PState pstate,PubFunction_t *fn, int n)
{
  ResData_strategy((int)fn->upvalue);
  return 0;
}

static int PubResources_getAnnotation(PState pstate,PubFunction_t *fn, int n)
{
  int resRID;
  char *name;
  Char2PtrNode_t *anno;
  Char2PtrNode_t *listhead;

  if (n < 2 || 2>FunctionPublish_getArg(pstate,2,(lxClassType)fn->upvalue,(void*)&resRID,LUXI_CLASS_STRING,(void*)&name))
    return FunctionPublish_returnError(pstate,"1 resource 1 string required");

  switch((lxClassType)fn->upvalue){
  case LUXI_CLASS_MATERIAL:
    listhead = ResData_getMaterial(resRID)->annotationListHead;
    break;
  case LUXI_CLASS_SHADER:
    listhead = ResData_getShader(resRID)->annotationListHead;
    break;
  case LUXI_CLASS_PARTICLESYS:
    listhead = ResData_getParticleSys(resRID)->annotationListHead;
    break;
  }

  lxListNode_forEach(listhead,anno)
    if (strcmp(anno->name,name)==0){
      return FunctionPublish_returnString(pstate,anno->str);
    }
  lxListNode_forEachEnd(listhead,anno);

  return 0;
}

static int PubResources_getType(PState pstate,PubFunction_t *fn, int n)
{
  return FunctionPublish_returnInt(pstate,(int)fn->upvalue);
}

static int PubResources_path(PState pstate,PubFunction_t *fn, int n)
{
  char *path;

  if (n){
    if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&path))
      return FunctionPublish_returnError(pstate,"1 string required");
    strcpy(g_ResDescriptor[(int)fn->upvalue].path,path);
    return 0;
  }

  return FunctionPublish_returnString(pstate,g_ResDescriptor[(int)fn->upvalue].path);

}

static int PubResourceChunk_new(PState pstate,PubFunction_t *fn, int n)
{
  float cnts[RESOURCES];
  int kbs = 0;
  int read;
  int megs;
  char *name;
  int i;
  ResourceChunk_t *reschunk;


  if (n<(2+RESOURCES) || FunctionPublish_getArg(pstate,3,LUXI_CLASS_STRING,&name,LUXI_CLASS_INT,&megs,LUXI_CLASS_INT,&kbs)<2 ||
    strlen(name)>= RES_MAX_CHUNKNAME)
    return FunctionPublish_returnError(pstate,"1 string 1/2 int and 'restypes'-many floats required. name must be shorter than 64");

  if (n == 2+RESOURCES){
    read = 2;
    kbs = 0;
  }
  else{
    read = 3;
  }

  for (i=0; i < RESOURCES; i++){
    if (!FunctionPublish_getNArg(pstate,i+read,FNPUB_TFLOAT(cnts[i])))
      return FunctionPublish_returnError(pstate,"1 string 1 or 2 int and 'restypes'-many floats required");
  }

  reschunk = ResourceChunk_new(name,cnts,(megs*1024) + kbs);

  if (!reschunk)
    return 0;

  return FunctionPublish_returnType(pstate,LUXI_CLASS_RESCHUNK,reschunk);
}

enum PubResCmd_e
{
  PUBRES_DESTROYDEFAULT,
  PUBRES_USECORE,
  PUBRES_GETDEFAULT,
  PUBRES_CURRENT,
  PUBRES_NEWVBOSIZE,

  PUBRES_NEEDARG_FUNCS,

  PUBRES_ACTIVATE,
  PUBRES_CLEAR,
  PUBRES_VBOSTATS,
  PUBRES_GETLOADED,
};

static int PubResourceChunk_func(PState pstate,PubFunction_t *fn, int n)
{
  static char buffer[2][256];

  ResourceChunk_t *chunk;
  HWBufferObject_t *resbo;
  int depth;
  int intval;
  char state;


  if ((int)fn->upvalue > PUBRES_NEEDARG_FUNCS && !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RESCHUNK,(void*)&chunk))
    return FunctionPublish_returnError(pstate,"1 reschunk required");


  switch((int)fn->upvalue) {
  case PUBRES_VBOSTATS:
    {
      for (depth = 0; depth < 2; depth++)
      {

        char buffernum[12];
        char buffernum2[12];
        int used = 0;
        int usedsize = 0;
        int allocsize = 0;

        lxListNode_forEach(chunk->vbosListHead[depth],resbo)
          used++;
          usedsize += resbo->buffer.used;
          allocsize += resbo->buffer.size;
        lxListNode_forEachEnd(chunk->vbosListHead[depth],resbo);

        sprintf(buffer[depth],"%s: used %d, accum.space: %s/%s (used/total)\n",
          (depth) ? "indexbuffer" : "vertexbuffer",used,
          lxStrFormatedNumber(buffernum,12,usedsize,2),
          lxStrFormatedNumber(buffernum2,12,allocsize,2));
      }

      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_STRING,(void*)buffer[0],LUXI_CLASS_STRING,(void*)buffer[1]);
    }


    break;
  case PUBRES_NEWVBOSIZE:
    if (n==0) return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)g_ResData.newVBOsize,LUXI_CLASS_INT,(void*)g_ResData.newIBOsize);
    else if (n != 2 || 2!= FunctionPublish_getArg(pstate,2,LUXI_CLASS_INT,(void*)&depth,LUXI_CLASS_INT,(void*)&intval) ||
      depth < 1 || intval < 1)
      return FunctionPublish_returnError(pstate,"2 int (>0) required");
    g_ResData.newIBOsize = intval;
    g_ResData.newVBOsize = depth;
    break;
  case PUBRES_DESTROYDEFAULT:
    ResData_freeDefault();
    break;
  case PUBRES_GETDEFAULT:
    chunk = ResData_getChunkDefault();
    if (chunk)
      return FunctionPublish_returnType(pstate,LUXI_CLASS_RESCHUNK,(void*)chunk);
    else
      return 0;
    break;
  case PUBRES_CURRENT:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_RESCHUNK,(void*)ResData_getChunkActive());
    break;
  case PUBRES_USECORE:
    if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&state))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    ResData_CoreChunk(state);
    break;
  case PUBRES_ACTIVATE: // activate
    ResourceChunk_activate(chunk);
    break;
  case PUBRES_CLEAR:  // clear
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&depth))
      return FunctionPublish_returnError(pstate,"1 reschunk 1 int required");
    ResourceChunk_clear(chunk,depth,LUX_TRUE);
    break;
  case PUBRES_GETLOADED:
    {
      int i = -1;
      int n;

      ResourceArray_t *resarray;
      ResourcePtr_t *resptr;
      ResourceContainer_t *rescont;

      if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&depth))
        return FunctionPublish_returnError(pstate,"1 reschunk 1 int required");

      switch(depth) {
      case RESOURCE_MODEL:  intval = LUXI_CLASS_MODEL; break;
      case RESOURCE_ANIM:  intval = LUXI_CLASS_ANIMATION; break;
      case RESOURCE_TEXTURE:  intval = LUXI_CLASS_TEXTURE; break;
      case RESOURCE_MATERIAL:  intval = LUXI_CLASS_MATERIAL; break;
      case RESOURCE_SHADER:  intval = LUXI_CLASS_SHADER; break;
      case RESOURCE_GPUPROG:  intval = LUXI_CLASS_GPUPROG; break;
      case RESOURCE_PARTICLECLOUD:  intval = LUXI_CLASS_PARTICLECLOUD; break;
      case RESOURCE_PARTICLESYS:  intval = LUXI_CLASS_PARTICLESYS; break;
      case RESOURCE_SOUND:  intval = LUXI_CLASS_SOUND; break;
      default:
        return FunctionPublish_returnError(pstate,"illegal resourcetype");
      }

      resarray = &g_ResData.resarray[depth];
      rescont = &chunk->containers[depth];
      resptr = &resarray->ptrs[rescont->resOffset];

      if (FunctionPublish_getNArg(pstate,2,LUXI_CLASS_INT,(void*)&i)
        && (i < 0 || i >= rescont->resUsed))
      {
        return FunctionPublish_returnError(pstate,"index out of bounds");
      }

      if (i < 0){
        return FunctionPublish_returnInt(pstate,rescont->resUsed);
      }

      if (rescont->resUsed < 1)
        return FunctionPublish_returnInt(pstate,0);

      for (n = 0; n < rescont->resCnt; n++,resptr++){
        if (resptr->resinfo){
          if (i==0){
            return FunctionPublish_returnType(pstate,intval,(void*)resptr->resinfo->resRID);
          }
          i--;
        }
      }

      return 0;
    }
  default:
    break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_FILETYPE
/*
static int PubFileType_get(PubFunction_t *f, int n)
{
  char *name;
  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name))
    return FunctionPublish_returnError(pstate,"1 string required");


  return FunctionPublish_returnType(pstate,LUXI_CLASS_FILETYPE,(void*)FS_getLoaderType(name));
}

static int PubFileType_new(PubFunction_t *f, int n)
{
  return FunctionPublish_returnType(pstate,LUXI_CLASS_FILETYPE,f->upvalue);
}
*/
//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_FILESYSTEM

enum PubFileCmd_e{
  PUBFILE_FILEEXISTS,
  PUBFILE_OPENFILE,
};
static PFunc l_fileexists = NULL;
static PFunc l_popenfile = NULL;

static int PubFileSys_externalfileexists (const char *filename)
{
  char *err;
  int n,res=0;

  if (l_fileexists == NULL)
    return FS_internal_fileExists(filename);

  n = FunctionPublish_callPFunc(NULL, l_fileexists, &err,
    LUXI_CLASS_STRING,filename,NULL,LUXI_CLASS_BOOLEAN,&res,NULL);

  if (n<1) {
    if (n<0)
      printf("Error: %s\n",err);
    return 0;
  }

  return res;
}

static void* PubFileSys_externalOpenFile(const char *filename, long *outsize, int *freecontent)
{
  if (l_popenfile != NULL) {
    PubBinString_t strread;
    int n;
    const char *err;
    char *data;

    n = FunctionPublish_callPFunc(NULL, l_popenfile, &err,LUXI_CLASS_STRING,filename,NULL,LUXI_CLASS_BINSTRING,&strread,NULL);
    if (n<0) {
      printf("Error: %s\n",err);
      return NULL;
    }
    if (n==0) {
      *freecontent = LUX_FALSE;
      *outsize = 0;
      return NULL;
    }
    *outsize = strread.length;
    data = MEMORY_GLOBAL_MALLOC(strread.length);
    memcpy(data,strread.str,strread.length);
    *freecontent = LUX_TRUE;
    return (void*)data;
  } else {
    return FS_internal_open(filename, outsize, freecontent);
  }
  return NULL;
}

static int PubFileSys_setfunc(PState state,PubFunction_t *f, int n)
{
  PFunc *apply;
  switch((int)f->upvalue) {
    case PUBFILE_FILEEXISTS:
      apply = &l_fileexists;
      FS_setExternalFileExists(PubFileSys_externalfileexists);
    break;
    case PUBFILE_OPENFILE:
      apply = &l_popenfile;
      FS_setExternalOpen(PubFileSys_externalOpenFile);
    break;
  }
  if (n==0) {
    FunctionPublish_releasePFunc(state,*apply);
    *apply = NULL;
    return 0;
  }
  if (!FunctionPublish_getNArg(state,0,LUXI_CLASS_FUNCTION,(void**)apply)) {
    return FunctionPublish_returnError(state,"Function or nil expected as arg1");
  }

  //const char *err;
  //int ival;

  //if (FunctionPublish_callPFunc(state, *apply, &err,LUXI_CLASS_STRING,"test",NULL,LUXI_CLASS_INT,&ival,NULL)<0)
  //  return FunctionPublish_returnError(state,err);

  //printf("result %i\n",ival);

  return 0;
}


//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_PGRAPH
enum PubPGraphCmd_e{
  PUBPG_MAX,
  PUBPG_SIZE,
  PUBPG_POS,
  PUBPG_DRAW,
  PUBPG_RENDERFINISH,
  PUBPG_PGRAPH_FUNC,
  PUBPG_COLOR,
  PUBPG_SAMPLE,
  PUBPG_STIPPLE,
  PUBPG_START,
  PUBPG_END,
};
static int PubPGraph_get(PState pstate,PubFunction_t *f, int n){
  return FunctionPublish_returnType(pstate,LUXI_CLASS_PGRAPH,f->upvalue);
}

static int PubPGraph_func(PState pstate,PubFunction_t *f, int n)
{
  PGraphType_t pg;
  PGraphData_t *pgdata;
  PGraph_t *pgraph;
  float flt;

  if ((int)f->upvalue > PUBPG_PGRAPH_FUNC){
    if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_PGRAPH,(void*)&pg))
      return FunctionPublish_returnError(pstate,"1 pgraph required");
    pgraph = PGraph_get(pg);
  }

  pgdata = PGraphData_get();

  switch((int)f->upvalue)
  {
  case PUBPG_MAX:
    if (n <1)
      return FunctionPublish_returnError(pstate,"1 pgraph or 1 float required");

    if (FunctionPublish_getNArgType(pstate,0) == LUXI_CLASS_FLOAT){
      FunctionPublish_getNArg(pstate,0,FNPUB_TFLOAT(flt));
      pgraph = pgdata->graphs;
      for (pg = 0; pg < PGRAPHS; pg++,pgraph++){
        pgraph->max = flt;
      }
      return 0;
    }

    if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_PGRAPH,(void*)&pg))
      return FunctionPublish_returnError(pstate,"1 pgraph required");

    pgraph = PGraph_get(pg);
    if (n==1) return FunctionPublish_returnFloat(pstate,pgraph->max);
    else if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(pgraph->max)))
      return FunctionPublish_returnError(pstate,"1 pgraph 1 float required");

    break;
  case PUBPG_RENDERFINISH:
    if (n==0) return FunctionPublish_returnBool(pstate,g_Draws.statsFinish);
    else if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&g_Draws.statsFinish))
      return FunctionPublish_returnError(pstate,"1 boolean required");
    break;
  case PUBPG_SIZE:
    if (n==0) return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR2(pgdata->size));
    else if (2!=FunctionPublish_getArg(pstate,2,FNPUB_TOVECTOR2(pgdata->size)))
      return FunctionPublish_returnError(pstate,"2 float required");
    break;
  case PUBPG_POS:
    if (n==0) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(pgdata->pos));
    else if (3!=FunctionPublish_getArg(pstate,3,FNPUB_TOVECTOR3(pgdata->pos)))
      return FunctionPublish_returnError(pstate,"3 float required");
    break;
  case PUBPG_COLOR:
    if (n==1) return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(pgraph->color));
    else if (3!=FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(pgraph->color)))
      return FunctionPublish_returnError(pstate,"1 pgraph 3 float required");
    break;
  case PUBPG_SAMPLE:
    if (n < 2 || !FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(flt)))
      return FunctionPublish_returnError(pstate,"1 pgraph 1 float required");
    PGraph_addSample(pg,flt);
    break;
  case PUBPG_STIPPLE:
    if (n==1) return FunctionPublish_returnBool(pstate,pgraph->stipple);
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&pgraph->stipple))
      return FunctionPublish_returnError(pstate,"1 pgraph 1 boolean required");
    break;
  case PUBPG_START:
    pgraph->time = -getMicros();
    break;
  case PUBPG_END:
    pgraph->time += getMicros();
    PGraph_addSample(pg,0.001f*(float)pgraph->time);
    break;
  case PUBPG_DRAW:
    {
      byte state;

      if (n <1)
        return FunctionPublish_returnError(pstate,"1 pgraph or 1 boolean required");

      if (FunctionPublish_getNArgType(pstate,0) == LUXI_CLASS_BOOLEAN){
        FunctionPublish_getNArg(pstate,0,LUXI_CLASS_BOOLEAN,(void*)&state);

        if (state)
          for (pg = 0; pg < PGRAPHS; pg++){
            g_Draws.drawPerfGraph |= 1<<pg;
          }
        else
          g_Draws.drawPerfGraph = 0;

        return 0;
      }

      if (!FunctionPublish_getNArg(pstate,0,LUXI_CLASS_PGRAPH,(void*)&pg))
        return FunctionPublish_returnError(pstate,"1 int or 1 boolean required");

      if (n == 1){
        state = (g_Draws.drawPerfGraph & 1<<pg) ? LUX_TRUE : LUX_FALSE;
        return FunctionPublish_returnBool(pstate,state);
      }
      else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
        return FunctionPublish_returnError(pstate,"1 int required, optional 1 boolean");

      if (state)
        g_Draws.drawPerfGraph |= 1<<pg;
      else
        g_Draws.drawPerfGraph &= ~(1<<pg);
    }
    break;
  default :
    break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_CAPABILITY

enum PubCapaCmd_e
{
  PCAP_HWSHADOW,
  PCAP_DOT3NORMAL,
  PCAP_TWOSIDEDSTENCIL,
  PCAP_VBO,
  PCAP_TEXMIPMAP,
  PCAP_TEXCOMPRESS,
  PCAP_TEXANISO,
  PCAP_TEXDDS,
  PCAP_TEX3D,
  PCAP_TEXCUBE,
  PCAP_STENCILWRAP,
  PCAP_TEXCOMB_CROSSBAR,
  PCAP_TEXCOMB_MODADD,
  PCAP_TEXCOMB_DOT3,
  PCAP_TEXCOMB_SUBTRACT,
  PCAP_TEXCOMB_COMBINE4,
  PCAP_TEXBGR,
  PCAP_TEXRECT,
  PCAP_TEXABGR,
  PCAP_TEXFLOAT,
  PCAP_TEXINT,
  PCAP_TEXBUFFER,
  PCAP_PBO,
  PCAP_TEXVERTEX,
  PCAP_TEXDEPTHSTENCIL,
  PCAP_TEX_NONPOWEROF2,
  PCAP_TEXSIZE3D,
  PCAP_TEXARRAY,
  PCAP_TEXARRAYLAYERS,
  PCAP_FBO,
  PCAP_FBOBLIT,
  PCAP_FBOMSAA,
  PCAP_MRT,
  PCAP_VRAM,
  PCAP_POINTSPRITES,
  PCAP_R2VB,
  PCAP_TEXBARRIER,
  PCAP_TEXRG,
  PCAP_TEXSIZE,
  PCAP_TEXCOPY,
};

static int PubCapability_query(PState pstate,PubFunction_t *f, int n)
{
  switch((int)f->upvalue)
  {
  case PCAP_HWSHADOW:     return FunctionPublish_returnBool(pstate,g_VID.capHWShadow);
  case PCAP_DOT3NORMAL:   return FunctionPublish_returnBool(pstate,g_VID.capBump);
  case PCAP_TWOSIDEDSTENCIL:  return FunctionPublish_returnBool(pstate,g_VID.capTwoSidedStencil);
  case PCAP_VBO:        return FunctionPublish_returnBool(pstate,GLEW_ARB_vertex_buffer_object);
  case PCAP_TEXMIPMAP:    return FunctionPublish_returnBool(pstate,GLEW_SGIS_generate_mipmap);
  case PCAP_TEXCOMPRESS:    return FunctionPublish_returnBool(pstate,g_VID.capTexCompress);
  case PCAP_TEXANISO:     return g_VID.capTexAnisotropic ? FunctionPublish_returnFloat(pstate,g_VID.capTexAnisotropic) : 0;
  case PCAP_TEXDDS:     return FunctionPublish_returnBool(pstate,g_VID.capTexDDS);
  case PCAP_TEX3D:      return FunctionPublish_returnBool(pstate,GLEW_EXT_texture3D);
  case PCAP_TEXCUBE:      return FunctionPublish_returnBool(pstate,GLEW_ARB_texture_cube_map);
  case PCAP_STENCILWRAP:    return FunctionPublish_returnBool(pstate,GLEW_EXT_stencil_wrap);
  case PCAP_TEXCOMB_CROSSBAR: return FunctionPublish_returnBool(pstate,GLEW_ARB_texture_env_crossbar || GLEW_NV_texture_env_combine4);
  case PCAP_TEXCOMB_MODADD: return FunctionPublish_returnBool(pstate,GLEW_ATI_texture_env_combine3 || GLEW_NV_texture_env_combine4);
  case PCAP_TEXCOMB_DOT3:   return FunctionPublish_returnBool(pstate,GLEW_ARB_texture_env_dot3);
  case PCAP_TEXCOMB_COMBINE4: return FunctionPublish_returnBool(pstate,GLEW_NV_texture_env_combine4);
  case PCAP_TEXCOMB_SUBTRACT: return FunctionPublish_returnBool(pstate,GLEW_ARB_texture_env_combine);
  case PCAP_TEXBGR:     return FunctionPublish_returnBool(pstate,GLEW_EXT_bgra);
  case PCAP_TEXRECT:      return FunctionPublish_returnBool(pstate,GLEW_NV_texture_rectangle);
  case PCAP_TEXABGR:      return FunctionPublish_returnBool(pstate,GLEW_EXT_abgr);
  case PCAP_TEXFLOAT:     return FunctionPublish_returnBool(pstate,g_VID.capTexFloat);
  case PCAP_TEXINT:     return FunctionPublish_returnBool(pstate,g_VID.capTexInt);
  case PCAP_TEXBUFFER:    return FunctionPublish_returnBool(pstate,g_VID.capTexBuffer);
  case PCAP_PBO:        return FunctionPublish_returnBool(pstate,g_VID.capPBO);
  case PCAP_TEXVERTEX:    return FunctionPublish_returnInt(pstate,g_VID.capTexVertexImages);
  case PCAP_TEXDEPTHSTENCIL:  return FunctionPublish_returnBool(pstate,GLEW_EXT_packed_depth_stencil);
  case PCAP_TEX_NONPOWEROF2:  return FunctionPublish_returnBool(pstate,GLEW_ARB_texture_non_power_of_two);
  case PCAP_MRT:        return FunctionPublish_returnInt(pstate,g_VID.capDrawBuffers);
  case PCAP_TEXSIZE3D:    return FunctionPublish_returnInt(pstate,g_VID.capTex3DSize);
  case PCAP_FBO:        return FunctionPublish_returnBool(pstate,g_VID.capFBO);
  case PCAP_FBOBLIT:      return FunctionPublish_returnBool(pstate,g_VID.capFBOBlit);
  case PCAP_FBOMSAA:      return g_VID.capFBOMSAA ? FunctionPublish_returnInt(pstate,g_VID.capFBOMSAA) : 0;
  case PCAP_VRAM:       return FunctionPublish_returnInt(pstate,g_VID.capVRAM);
  case PCAP_POINTSPRITES:   return FunctionPublish_returnBool(pstate,GLEW_ARB_point_sprite);
  case PCAP_TEXARRAY:     return FunctionPublish_returnBool(pstate,g_VID.capTexArray > 0);
  case PCAP_TEXARRAYLAYERS: return FunctionPublish_returnInt(pstate,g_VID.capTexArray);
  case PCAP_R2VB:       return FunctionPublish_returnBool(pstate,g_VID.capFeedback);
  case PCAP_TEXBARRIER:   return FunctionPublish_returnBool(pstate,GLEW_NV_texture_barrier);
  case PCAP_TEXRG:      return FunctionPublish_returnBool(pstate,GLEW_ARB_texture_rg);
  case PCAP_TEXSIZE:      return FunctionPublish_returnInt(pstate,g_VID.capTexSize);
  case PCAP_TEXCOPY:      return FunctionPublish_returnBool(pstate,GLEW_NV_copy_image);
  default:
    return 0;
  }

}

static int PubResSubRID_getParts(PState pstate,PubFunction_t *f,int n)
{
  int id;
  int hostid;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RESSUBID,(void*)&id))
    return FunctionPublish_returnError(pstate,"1 ressubid required");

  hostid = SUBRESID_GETRES(id);
  id = SUBRESID_GETSUB(id);

  switch(FunctionPublish_getNArgType(pstate,0))
  {
  case LUXI_CLASS_MESHID:
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)id,LUXI_CLASS_MODEL,(void*)hostid);
  case LUXI_CLASS_BONEID:
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)id,LUXI_CLASS_MODEL,(void*)hostid);
  case LUXI_CLASS_SKINID:
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)id,LUXI_CLASS_MODEL,(void*)hostid);
  case LUXI_CLASS_PARTICLEFORCEID:
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)id,LUXI_CLASS_PARTICLESYS,(void*)hostid);
  case LUXI_CLASS_TRACKID:
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)id,LUXI_CLASS_ANIMATION,(void*)hostid);
  case LUXI_CLASS_MATTEXCONTROLID:
  case LUXI_CLASS_MATCONTROLID:
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)id,LUXI_CLASS_MATERIAL,(void*)hostid);
  case LUXI_CLASS_SHADERPARAMID:
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_INT,(void*)id,LUXI_CLASS_SHADER,(void*)hostid);
  }
  return 0;
}

enum PubResNameType_e{
  PUB_RES_NAME,
  PUB_RES_NAMESHORT,
  PUB_RES_NAMEUSER,
};

static int PubResources_getName(PState pstate,PubFunction_t *f,int n)
{
  int id;
  char *name = NULL;
  char *userstring;
  char *extname =NULL;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_RESOURCE,(void*)&id))
    return FunctionPublish_returnError(pstate,"1 resource required");


  switch(FunctionPublish_getNArgType(pstate,0))
  {
  case LUXI_CLASS_MODEL:
    name = ResData_getModel(id)->resinfo.name;
    userstring = ResData_getModel(id)->resinfo.userstring;
    break;
  case LUXI_CLASS_PARTICLECLOUD:
    name = ResData_getParticleCloud(id)->resinfo.name;
    userstring = ResData_getParticleCloud(id)->resinfo.userstring;
    break;
  case LUXI_CLASS_PARTICLESYS:
    name = ResData_getParticleSys(id)->resinfo.name;
    userstring = ResData_getParticleSys(id)->resinfo.userstring;
    break;
  case LUXI_CLASS_SHADER:
    name = ResData_getShader(id)->resinfo.name;
    userstring = ResData_getShader(id)->resinfo.userstring;
    break;
  case LUXI_CLASS_MATERIAL:
    name = ResData_getMaterial(id)->resinfo.name;
    userstring = ResData_getMaterial(id)->resinfo.userstring;
    break;
  case LUXI_CLASS_ANIMATION:
    name = ResData_getAnim(id)->resinfo.name;
    userstring = ResData_getAnim(id)->resinfo.userstring;
    break;
  case LUXI_CLASS_GPUPROG:
    name = ResData_getGpuProg(id)->resinfo.name;
    extname = ResData_getGpuProg(id)->resinfo.extraname;
    userstring = ResData_getGpuProg(id)->resinfo.userstring;
    break;
  case LUXI_CLASS_SOUND:
    name = ResData_getSound(id)->resinfo.name;
    userstring = ResData_getSound(id)->resinfo.userstring;
    break;
  case LUXI_CLASS_TEXTURE:
    name = ResData_getTexture(id)->resinfo.name;
    userstring = ResData_getTexture(id)->resinfo.userstring;
    break;
  default:
    return 0;
  }

  switch((int)f->upvalue)
  {
  case PUB_RES_NAME:
    {
      if (extname)
        return FunctionPublish_setRet(pstate,2,LUXI_CLASS_STRING,(void*)name,LUXI_CLASS_STRING,(void*)extname);
      else
        return FunctionPublish_returnString(pstate,name);
    }
  case PUB_RES_NAMESHORT:
    {
      // strip name from path
      char *last = LUX_MAX(strrchr(name,'/'),strrchr(name,'\\'));
      name = last ? last+1 : name;

      if (extname)
        return FunctionPublish_setRet(pstate,2,LUXI_CLASS_STRING,(void*)name,LUXI_CLASS_STRING,(void*)extname);
      else
        return FunctionPublish_returnString(pstate,name);
    }
  case PUB_RES_NAMEUSER:
    {
      char *username;

      if(n < 2) return FunctionPublish_returnString(pstate,userstring);
      else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&username))
        return FunctionPublish_returnError(pstate,"1 resource 1 string [length < 256] required");

      strncpy(userstring,username,sizeof(char)*255);
      return 0;
    }
  default:
    return 0;
  }

}

static int PubResources_condition(PState pstate,PubFunction_t * f, int n)
{
  char *name;
  byte state;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name))
    return FunctionPublish_returnError(pstate,"1 string required");

  if (n < 2)
    return FunctionPublish_returnBool(pstate,(char)VIDUserDefine_get(name));

  if (n==2 && !FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&state))
    return FunctionPublish_returnError(pstate,"1 string 1 boolean required");

  if (state)
    VIDUserDefine_add(name);
  else
    VIDUserDefine_remove(name);

  return 0;
}

//////////////////////////////////////////////////////////////////////////

void PubClass_System_init()
{
  FunctionPublish_initClass(LUXI_CLASS_CAPABILITY,"capability","Certain system (hardware related) capabilites can be queried for support here.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_HWSHADOW,"hwshadow",
    "(boolean):() - returns support for shadow map comparison and depth textures");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_DOT3NORMAL,"dot3normalmap",
    "(boolean):() - returns support for VID_NORMAL combiner in shader system.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TWOSIDEDSTENCIL,"stenciltwosided",
    "(boolean):() - returns support for two-sided stencil testing");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_VBO,"vbo",
    "(boolean):() - returns support for vbo");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXMIPMAP,"texautomipmap",
    "(boolean):() - returns support for automatic mipmaps");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXCOMPRESS,"texcompress",
    "(boolean):() - returns support for texture compression");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXANISO,"texanisotropic",
    "([float]):() - returns maximum anisotropic value for texture filtering, if supported.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXDDS,"texdds",
    "(boolean):() - returns support for dds texture loading");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEX3D,"tex3d",
    "(boolean):() - returns support for 3d textures");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXCUBE,"texcube",
    "(boolean):() - returns support for cubemap textures");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_STENCILWRAP,"stencilwrap",
    "(boolean):() - returns support for wrapped increment and decrement");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXCOMB_CROSSBAR,"texcomb_crossbar",
    "(boolean):() - returns support for access to all texture units in texcombiners");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXCOMB_MODADD,"texcomb_modadd",
    "(boolean):() - returns support for modulated add functions in texcombiners");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXCOMB_DOT3,"texcomb_dot3",
    "(boolean):() - returns support for dot3 in texcombiners");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXCOMB_SUBTRACT,"texcomb_subtract",
    "(boolean):() - returns support for subtract in texcombiners");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXCOMB_COMBINE4,"texcomb_combine4",
    "(boolean):() - returns support for combine4 in texcombiners");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXBGR,"texbgr",
    "(boolean):() - returns support for BGR,BGRA texture data");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXRECT,"texrectangle",
    "(boolean):() - returns support for rectangle textures");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXABGR,"texabgr",
    "(boolean):() - returns support for ABGR texture data");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXFLOAT,"texfloat",
    "(boolean):() - returns support for float16 and float32 textures");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXVERTEX,"texvertex",
    "(int):() - returns number of textures that can be accessed in vertex shader");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXDEPTHSTENCIL,"texdepthstencil",
    "(boolean):() - returns support for a packed format of 24bit depth and 8bit stencil texture.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEX_NONPOWEROF2,"texnp2",
    "(boolean):() - returns support for non power of two texture dimensions. Currently only supported for 3d textures. For 2d textures use rectangle capability.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_FBO,"fbo",
    "(boolean):() - returns support for framebufferobjects (fbo), allow enhanced rendertotexture. Especially rendering to multiple drawbuffers or textures with larger size than window. renderfbo, renderbuffer, rcmdfbobind ... need this capability.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_FBOBLIT,"bufferblit",
    "(boolean):() - returns support for blitting from one to another renderfbo/backbuffer via rcmdbufferblit");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_FBOMSAA,"renderbuffermsaa",
    "([int]):() - returns number of maximum samples of multisampled renderbuffers, or nil if no support is given.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_MRT,"drawbuffers",
    "(int):() - returns maximum number of multiple render targets for color drawbuffers");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query, (void*)PCAP_TEXSIZE3D,"tex3dsize",
    "(int):() - returns maximum dimension of 3d texture");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query, (void*)PCAP_VRAM,"vramsize",
    "(int):() - returns video ram (can be 0 if query did not work)");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query, (void*)PCAP_POINTSPRITES,"pointsprites",
    "(boolean):() - returns if particle points can be rendered as sprites (quad-like textured).");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXARRAY,"texarray",
    "(boolean):() - returns support for array textures, typically requires capability.cg_sm4 as well.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXARRAYLAYERS,"texarraylayers",
    "(int):() - returns how many texture layers in array textures are available.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXBUFFER,"texbuffer",
    "(boolean):() - returns support for buffer textures, typically requires capability.cg_sm4 as well.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXINT,"texinteger",
    "(boolean):() - returns support for integer textures, typically requires capability.cg_sm4 as well.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_PBO,"pbo",
    "(boolean):() - returns support for pixel buffer objects, useful for loading/retrieven image data asynchronously.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_R2VB,"r2vb",
    "(boolean):() - returns support for render 2 vertexbuffer functionality, useful to capture vertex stream output.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXBARRIER,"texbarrir",
    "(boolean):() - returns support texbarrier functionality (read and write same texture once, ie. custom blending). allows 'texbarrier' keyword in shader files.");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXRG,"texrg",
    "(boolean):() - returns support for R and RG textures.");

  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXSIZE,"texsize",
    "(int):() - returns maximum texture side length (1d or 2d).");
  FunctionPublish_addFunction(LUXI_CLASS_CAPABILITY,PubCapability_query,(void*)PCAP_TEXCOPY,"texcopy",
    "(boolean):() - returns support for texture.copy.");

  FunctionPublish_initClass(LUXI_CLASS_SYSTEM,"system","system information and settings",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_vid,NULL,"vidinfo",
    "():() - prints info on renderer");
  //FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_pause,NULL,"pause",
  //  "pause (boolean) - pauses server, g_LuxTimer.time stays same as before");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_version,NULL,"version",
    "(string version,string data,string time):() - returns luxinia version, builddate and buildtime.");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_EXIT,"exit",
    "() : ([boolean nocleanup]) - closes application, if nocleanup is true, this will perform no proper cleanup but leave to os.");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_AUTOSCREENSHOT,"autoscreenshot",
    "([boolean]) : ([boolean],[boolean tga]) - makes a screenshot at end of every frame.");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_SCREENSHOTQUALITY,"screenshotquality",
    "([int]) : ([int]) - quality of jpg screenshots (0-100).");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_PROCESSFRAME,"processframe",
    "() : ([float timediff],[boolean drawfirst]) - allows manuall processing of frames. timediff is in milliseconds, if not passed or -1, we will compute from the last time this function was called. \
this will run all internal thinks except the lua think. You should never start this \
unless you want to do frame processing from this time on for the rest of the runtime. \
Ideally this function is part of a loop which you start right in your initialisation script. Using this kind of setup \
will not return the same results as the automatic think and maxfps will show no effect. For one the performance graphs will not measure the luathink correctly, as well as \
ode. Also the order of the thinkcycle is slightly different. Be aware that you must run swapbuffers yourself to see the result of rendering. \
A good order of your think would be: processframe,your changes, swapbuffers. This would be close to the internal order, just that swapbuffers is called differently, but still would allow some cpu/gpu parallelism. \
Internally the order of a think in automatic mode is in one frame: render, luathink, sound, actors/scenenodes,swapbuffers. <br>\
Compared to the manual mode of this function: sound,actors/scenenodes,render. However when setting drawfirst to true, sound/actor etc. stuff is handled after rendering.");

  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_screenshot,NULL,"screenshot",
    "() : (string filename,[int scaling],[int quality],[int x,y,w,h]) - writes an jpg or tga image file in a file with the given name, \
depending on the suffix of the string (.jpg or .tga - case sensitive!). The scaling should be a number \
>=0 and <=3 and tells luxinia the scaling factor of the screenshot. 0 means full resolution, 3 means 1/8th \
of the resolution. The scaling is done with a simple and fast but lowquality pixelresize. The quality integer \
describes the quality of the jpg compression: lower means less quality but smaller filesize. Note: Smaller files \
can be flushed much faster to the HD, so if you want to do a little imagesequence, you may prefer a lower \
resolution mode with low quality.<br>\
You can also make a snapshot of a screenarea by defining x,y,w,h. Be aware those coordinates are in GL coords meaning 0,0 is bottom left.");

  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_SWAPBUFFERS,"swapbuffers",
    "() : () - should only be used in manual frame processing. swaps backbuffer, which we normally render to, and front buffer, which is what you see.");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_MAXFPS,"maxfps",
    "([int]) : ([int]) - gets or sets if fps of the app should be capped, 0 is uncapped, minimum will be 15.");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_PROJECTPATH,"projectpath",
    "([string]) : ([string]) - gets or sets used projectpath.");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_TIME,"time",
    "(int milisecs, float frametimediff, float avgdiff) : () - returns internal luxinia time in milliseconds since start and precise difference since the last frame, plus average of last 8 frames");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_TIMECUR,"currenttime",
    "(float seconds) : () - returns time");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_FRAME,"frame",
    "(int frame) : () - returns current render frame");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_MILLIS,"getmillis",
    "(float millis) : () - returns time in milliseconds, time is taken from motherboard high precision timer. Useful for profiling");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_REFCOUNT,"refcount",
    "(int refcount) : () - returns current number of references in the system. There's (currently) a \
    limit of 65000 references. Although Luxinia is using the garbage collection system of Lua, it \
    permanent data outside of Lua can be created which results in memory leaks if this is not managed.");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_SLEEP,"sleep",
    "() : (int millis) - trys to sleep for given time. The sleeptime will be subtracted from the next thinktime difference");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_NOSOUND,"nosound",
      "([boolean]) : ([boolean]) - sets/gets if sound is disabled. Affects system only during startup.");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_BUFFERPOOLSIZE,"buffermempoolsize",
    "([int]):([int]) - gets or sets maximum size in bytes of bufferpool used primarily in l3d rendering and various load buffers.\
    This is a scratch memory in which visible nodes, particles... are written into. It uses various\
    worst-case approximations, hence the maximum size it needs at runtime depends both on total l3dnodes\
    and per-frame situation. Don't change this unless you know what you are doing and have used luxinias's\
    profiling info (render.drawstats -> rpoolmax) and know the upper limits. Only try to lower the pool if\
    you either exceed limits or want to save every byte.<br>Large memory operations such as screenshot and texture saving may resize the buffer to prevent an allocation error, all other operations however will quit with error message written to log.");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_NUMPROC,"processors",
    "(int) : () - returns number of processors in system");

  // os stuff
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_OSSTRING,"osinfo",
    "(string) : () - returns information about OS");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_DRIVELIST,"drivelist",
    "(string drive, ...):() - returns available drives on windows");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_DRIVETYPE,"drivetype",
    "(string type):(string drive) - returns type for given drive. A trailing backslash is required. Returns either 'unkown', 'removable', 'norootdir', 'fixed', 'cdrom', 'ramdisk' or 'remote' (network)");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_DRIVESIZES,"drivesizes",
    "([float freetocaller,totalbytes,freebytes]):(string drive) - returns information on drive sizes or nil if not succesful");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_DRIVELABEL,"drivelabel",
    "([string name]):(string drive) - returns drivelabel or nil if not succesful");
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_MACADDRESS,"macaddress",
    "(string) : () - returns MAC adress of first network adapter.");
#ifdef MEMORY_GENERIC_STATS
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_DUMPMEMSTATS,"memstatsdump",
    "() : () - dumps all generic allocations to a csv file.");
#endif

  // frontend
  FunctionPublish_addFunction(LUXI_CLASS_SYSTEM,PubSystem_attributes,(void*)SYSCMD_CUSTOMSTRING,"queryfrontend",
    "(string) : ([string]) - queries the frontend, in which luxinia is embedded");

  // rendersystem
  FunctionPublish_initClass(LUXI_CLASS_RENDERSYS,"rendersystem","Rendering system information and settings",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_USESHADERS,"usebaseshaders",
    "([boolean]):([boolean]) - returns or sets if baseshaders should be used. Make sure they are specified before.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_RENDERVENDOR,"vendor",
    "(string):() - returns vendor of rendering system (ati,nvidia,intel,unknown)");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_CHECKSTATE,"checkstate",
    "(boolean):() - returns whether there are errors in the state tracker (prints to console).");

  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_BASESHADERS,"baseshaders",
    "([shader color_only, color_lightmap, 1tex, 1tex_lightmap]):([shader color_only, color_lightmap, 1tex, 1tex_lightmap]) - returns or sets baseshaders. Any non-material mesh, ie just having a texture or just having a color, will use those shaders and pass its texture (if exists) as Texture:0 to the shader. All other materials will use the Shader:0 they come with.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_PUREVERTEX,"purehwvertices",
    "([boolean]):([boolean]) - returns or sets if you can guarantee that all vertices (cept for debug helpers or font) are processed by your shaders which all use programmable pipeline. In that case we can ignore a few costly state changes that are only needed for fixed function pipeline.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_FORCEFINISH,"forcefinish",
    "([boolean]):([boolean]) - if true then renderfunction will wait until all rendering is finished after swapbuffers. This results into smoother fps especially with vsynch enabled, but reduces the theoretical maximum thruough-put.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_VBOS,"usevbos",
    "([boolean]):([boolean]) - sets/gets if vertex buffer objects should be enabled. This generally turns them on/off (on will have no effect if your hardware cant do it), the other option is to disable them per model.load. By default it is on if system technique is higher than VID_ARB_V.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_TEXCOMPRESS,"texcompression",
    "([boolean]) : ([boolean]) - gets or sets if texture compression should be used");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_TEXANISO,"texanisotropic",
    "([boolean]) : ([boolean]) - gets or sets if texture anisotropic filtering should be used");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_TEXANISOLEVEL,"texanisolevel",
    "([float]) : ([float]) - sets value for anisotropic filtering. Must be <= maximum capable");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_BONESPARAMS,"hwbonesparams",
    "([int]) : ([int]) - gets or sets how many parameters you use additionally to matrices in your gpuskin programs");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_NODEFAULTGPUPROGS,"nodefaultgpuprogs",
    "([boolean]) : ([boolean]) - sets/gets if defaultgpuprogs are disabled. Affects system only during startup.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_NOGLEXTENSIONS,"noglextensions",
    "([boolean]) : ([boolean]) - sets/gets if 'nice-to-have' GLextensions are disabled. Affects system only during startup.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_FORCE2TEX,"force2tex",
    "([boolean]) : ([boolean]) - sets/gets if system should be forced to have only two textureunits, useful for debugging. Affects system only during startup.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_GLSTRING,"glinfo",
    "(string renderer,string version, string vendor) : () - returns information about OpenGL Driver");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_BATCHMAXTRIS,"batchmeshmaxtris",
    "([int]):([int]) - l3dbatchbuffers will immediately render meshes with less triangles.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_BATCHMAXINDICES,"batchmaxindices",
    "([int]):([int]) - l3dbatchbuffers will finish batch after indices are reached.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_DETAIL,"detail",
    "([int]) : ([int]) - gets or sets users detail setting 1 - 3, 3 = highest detail. 1 = half texture size, 16 bit. 2 = 16 bit texture, 3 = 32 bit textures.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_CGCOMPILERARGS,"cgcompilerargs",
    "([string]):([string]) - returns the cg compiler args. Args must be separated by ;");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_CGFORCEGLSLLOW,"cglowprofileglsl",
    "() : ([boolean]) - sets if GLSL is used in the low Cg profiles. Setting will enforce GLSL useage for lowCgProfile (normally arbfp1/arbvp1) and highCgProfile, disabling will revert to original state. Be aware that setting should be done on startup, changing between modes so that shaders will be in mixed types, will result into errors. The commands main purpose is to test GLSL codepaths profiles.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_CGHIGHPROFILE,"cghighprofile",
    "() : (string vertexprofile, fragmentprofile, [technique]) - sets the Cg high profiles, used in higher techniques, also allows downgrade of max capable technique. Strings are profilenames of Cg, there is no error checking if profilename actually correspons to maximum tehniques. Be aware that setting should be done on startup, changing between modes so that shaders will be in mixed types, will result into errors. The commands main purpose is to test profile codepaths.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_CGDUMPBROKEN,"cgdumpbroken",
    "([boolean]) : ([boolean]) - gets or sets if Cg programs, who failed to load and were compiled at runtime, should be saved. Their filename is like pre-compiled version just with a BROKEN added.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_CGPREFERCOMPILED,"cgprefercompiled",
    "([boolean]) : ([boolean]) - gets or sets if Cg should check for compiled programs first.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_CGDUMPCOMPILED,"cgdumpcompiled",
    "([boolean]) : ([boolean]) - gets or sets if all Cg programs will saved to their corresponding compiled filename, will overwrite files.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_CGUSEOPTIMAL,"cguseoptimal",
    "([boolean]) : ([boolean]) - gets or sets if Cg should set optimal profile compiler arguments, using driver and GPU info. Be aware that there is no way to disable it, hence once the first Cg gpuprogram is loaded from that time on all will use compile flags for the local machine. When you want to precompile your shaders using generic profile options, ie like the offline Cg compiler, you should set this value to false before loading your shaders. The only machine dependent flag will be use of ATI_draw_buffers over ARB_draw_buffers.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_CGALLOWGLSL,"cgallowglslsm3up",
    "([boolean]) : ([boolean]) - gets or sets if Cg on startup should allow use of GLSL for non-nvidia cards, that can compile a test vertex- and fragment shader with SM3 features. From Cg 2.1 on this defaults to true.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_SWAPINTERVAL,"swapinterval",
    "([int]) : ([int]) - gets or sets swapbuffer interval behavior 0 = vsynch off, 1 on, 2 double buffered on ...");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_PUSHGL,"pushgl",
    "() : () - pushes all GL resources back (ie restores active data). Should be done after a major GLContext change. This function is also only for use with custom window managers. It is automatically called when switching from windowed to fullscreen mode. While GL resources are pushed rendering will be ignored, and many OpenGL calls will result into errors.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_POPGL,"popgl",
    "() : () - pops GL resources from useage (ie retrieves data). Should be done before a major GLContext change. Only relative if cusom window managers are used.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_POPPEDGL,"poppedgl",
    "(boolean) : () - returns true if GL resources are just popped, ie rendering is disabled.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_EXTSTRING,"glextensions",
    "(string) : () - returns the GL extension string");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_FINISHGL,"glfinish",
    "() : () - waits until all pending GL processes are finished.");
  FunctionPublish_addFunction(LUXI_CLASS_RENDERSYS,PubSystem_attributes,(void*)SYSCMD_DRIVERGL,"gldriverinfo",
    "(string vendor, renderer, version) : () - returns current driver info.");

  FunctionPublish_initClass(LUXI_CLASS_WINDOW,"window","The graphics window luxinia runs in. Setters only have effect after update is called. Do not call setters without ever calling update.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINRES,"res",
    "([int]) : ([int]) - sets window resolution 0 = 640x480x16, 1 = 800x600x16, 2 = 1024x768x16, 3 = 640x480x32, 4 = 800x600x32, 5 = 1024x768x32");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINRESSTRING,"resstring",
    "([string]) : ([string]) - gets or sets window resolution string=WIDTHxHEIGHTxBPP:DEPTH:STENCIL:FSAASAMPLES  BPP should be 16,24 or 32. Only 32 has forced alpha buffer support. You can check colorbits what kind of bits per pixel are used.");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINREFSIZE,"refsize",
    "([int w,h]) : ([int w,h]) - sets reference size of screen (default=640,480). Reference size is mostly important for list2d rendering and mouse positions. It allows the window not use pixel positions but custom dimensions which gui will be stretched to.");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINSIZE,"size",
    "([int w,h]) : ([int w,h]) - returns or sets size of window. Needs window.update to show effect");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINFULLSCREEN,"fullscreen",
    "([boolean]) : ([boolean]) - gets or sets if window will be fullscreen");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINPOS,"pos",
    "([x,y]) : ([x,y]) - sets or returns window position");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINSCREEN2REF,"screen2ref",
    "(x,y) : (x,y) - converts screen pixel to reference coords (origin top left in both).");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINREF2SCREEN,"ref2screen",
    "(x,y) : (x,y) - converts reference to screen pixel coords (origin top left in both).");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINUPDATE,"update",
    "() : () - if you have changed resolution or fullscreen state this function makes changes active");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINRESIZE,"resizemode",
    "([int]) : ([int mode]) - allows to get or set how the window should be resized if a user changes window size in windowed mode.\
    * 0 size is not changed (default)<br>\
    * 1 size is changed<br>\
    * 2 size and refsize are changed to new window size.<br>\
    * 3 size is changed and refsize will be changed according to window.resizeratio.");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINRESIZERATIO,"resizepixelscale",
    "([x,y]) : ([x,y]) - sets or returns pixelsize ratio (pixels/mm) for resizemode 3. outref = scale * screenpixels_per_mm. Queries screenpixels_per_mm automatically via window.screensizemm and window.screensize in the resize event.");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINMINSIZE,"minsize",
    "([int w,h]) : ([int w,h]) - returns or sets minimum size of window. Only has effect in windowed mode.");

  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINTITLE,"title",
    "() : (string) - Set the title of the window");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINONTOP,"ontop",
    "([boolean]) : ([boolean]) - returns or sets if window is always on top");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINCOLORBITS,"colorbits",
    "(int r,g,b,a) : () - returns actual bits used for each channel");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINHEIGHT,"height",
    "(int) : ([int]) - returns or sets height");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINWIDTH,"width",
    "(int) : ([int]) - returns or sets width");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINBITS,"bpp",
    "(int) : ([int]) - returns or sets bits per pixel (16,24,32)");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINSTENCILBITS,"stencilbits",
    "(int) : ([int]) - returns or sets stencilbits. After update this reflects the actual window value");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINDEPTHBITS,"depthbits",
    "(int) : ([int]) - returns or sets depthbits. After update this reflects the actual window value");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINSAMPLES,"multisamples",
    "(int) : ([int]) - returns or sets samples for fullscreen anti-aliasing. After update this reflects the actual window value");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINREADCOLOR,"readcolor",
    "(float r,g,b,a) : (float x,y) - returns color value at window coordinate (in refsystem)");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINREADDEPTH,"readdepth",
    "(float) : (float x,y) - returns depth value at window coordinate (in refsystem)");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINREADSTENCIL,"readstencil",
    "(int) : (float x,y) - returns stencil value at window coordinate (in refsystem)");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINTEXMATRIX,"texmatrix",
    "(matrix4x4) : () - returns texture matrix for 'fullwindow' textures.");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINCLIENT2SCREEN,"client2screen",
    "([x,y]) : (x,y) - converts coordinates from client (luxinia) window to screen (desktop) window.");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINSCREEN2CLIENT,"screen2client",
    "([x,y]) : (x,y) - converts coordinates from screen (desktop) window to client (luxinia) window.");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_SCREENSIZE,"screensizemm",
    "(int w,h) : () - returns size in millimeters for current primary display (e.g monitor).");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_SCREENRES,"screensize",
    "(int w,h) : () - returns primary display resolution.");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINICONIFY,"iconify",
    "() : () - iconifies the window.");
  FunctionPublish_addFunction(LUXI_CLASS_WINDOW,PubSystem_attributes,(void*)SYSCMD_WINRESTORE,"restore",
    "() : () - restores the window if iconified.");

// copied from  resmanger.h
  FunctionPublish_initClass(LUXI_CLASS_RESDATA,"resdata",
"resdata holds all resources that are supposedly static, like models, textures,\
 and are often reused, as those use different memory than the dynamic nodes\
 that are normally created.<br>\
 ResData is sepearted in ResourceChunks. The user specifies\
 how many chunks he wants and how big those should be, ie how many\
 resources of each type they can hold and how much memory they\
 preallocate.<br>\
 Resources are then loaded into the current active chunk, because\
 of the linear memorypool they cannot be destroyed individually\
 but only a total chunk can be destroyed.\
 <br>\
 A lot can go wrong when consumers of resource, that\
 were in a chunk that was cleared, are still active.\
 So the user should keep resources/consumer managment\
 in unified blocks himself, to prevent this from\
 happening, as he is totally responsible for it.\
 You can always play safe and use only the main chunk that is automatically created.<br>\
 <br>\
Overview on the restypes, and their int values:<br>\
 0  MODEL<br>\
 1  ANIMATION<br>\
 2  TEXTURE<br>\
 3  MATERIAL<br>\
 4  SHADER<br>\
 5  GPUPROG<br>\
 6  PARTICLECLOUD<br>\
 7  PARTICLESYS<br>\
 8  SOUND<br>"
    ,NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_RESDATA,PubResources_print,NULL,"print",
    "():() - prints loaded resources, and reschunks to os console");
  FunctionPublish_addFunction(LUXI_CLASS_RESDATA,PubResources_strategy,(void*)RESOURCE_FILL_ONLYSELF,"fillonlyself",
    "():() - when the active reschunk is full, resources will not be created. (default)");
  FunctionPublish_addFunction(LUXI_CLASS_RESDATA,PubResources_strategy,(void*)RESOURCE_FILL_TONEXT,"filltonext",
    "():() - when the active reschunk is full, we will load the resource in the next one, and make that active.");
  FunctionPublish_addFunction(LUXI_CLASS_RESDATA,PubResources_getCnt,(void*)0,"getloadcount",
    "(int):(int restype) - returns number of loaded resources.");
  FunctionPublish_addFunction(LUXI_CLASS_RESDATA,PubResources_getCnt,(void*)1,"getopencount",
    "(int):(int restype) - returns number of open slots available for this resource type. Useful for generating reschunks.");
  FunctionPublish_addFunction(LUXI_CLASS_RESDATA,PubResources_getCnt,(void*)2,"gettypescount",
    "(int):() - returns number of types that exist. You need to pass this many counts to reschunk new.");
  FunctionPublish_addFunction(LUXI_CLASS_RESDATA,PubResources_printDescr,(void*)NULL,"info",
    "():(int restype) - prints info about the given resource type, any resource implements the 'getrestype' function");

#define resFuncs(fclass,type) \
  FunctionPublish_addFunction(fclass,PubResources_getType,(void*)type,"getrestype","(int restype):() - returns the resource type as int value, useful for resdata or reschunk functions");\
  FunctionPublish_addFunction(fclass,PubResources_path,(void*)type,"defaultpath","([string]):([string]) - returns or sets the default resource path. Luxinia will search in those when resources are not found.")

    resFuncs(LUXI_CLASS_MODEL,RESOURCE_MODEL);
    resFuncs(LUXI_CLASS_ANIMATION,RESOURCE_ANIM);
    resFuncs(LUXI_CLASS_TEXTURE,RESOURCE_TEXTURE);
    resFuncs(LUXI_CLASS_MATERIAL,RESOURCE_MATERIAL);
    resFuncs(LUXI_CLASS_SHADER,RESOURCE_SHADER);
    resFuncs(LUXI_CLASS_GPUPROG,RESOURCE_GPUPROG);
    resFuncs(LUXI_CLASS_PARTICLECLOUD,RESOURCE_PARTICLECLOUD);
    resFuncs(LUXI_CLASS_PARTICLESYS,RESOURCE_PARTICLESYS);
    resFuncs(LUXI_CLASS_SOUND,RESOURCE_SOUND);
#undef resFuncs

#define resAnnotation(fclass,name)\
    FunctionPublish_addFunction(fclass,PubResources_getAnnotation,(void*)fclass,"annotation",\
      "([string]):(" name ", string name) - searches and returns annotation string")

    resAnnotation(LUXI_CLASS_SHADER,"shader");
    resAnnotation(LUXI_CLASS_MATERIAL,"material");
    resAnnotation(LUXI_CLASS_PARTICLESYS,"particlesys");

#undef resAnnotation


  FunctionPublish_initClass(LUXI_CLASS_RESOURCE,"resource",
    "Resources are files that are typically loaded from the harddisk and are not "
"managed with dynamic memory management, but stored within a reschunk. Unloading of a loaded resources is "
"not trivial, but possible. Unloading of resources is done by the reschunk class.<br><br>"
"The load function will try to load the file or return the first instance of the same name. "
"The forceload function will make sure a new copy of this resourcetype (not other resourcetypes) will be loaded."
"All create functions behave similar to load, they first look for the instance name being loaded/created before, and return that resource, or run the creation process. Therefore make sure to have unique identifier names when you want create multiple unique resources."
,
    NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_RESOURCE,PubResources_condition,(void*)NULL,"condition",
    "([boolean]):(string name,[boolean]) - Some resources allow sort of preprocessor settings for setups. Here you can set the values that you can query in shader,particlesys,material scripts.");
  FunctionPublish_addFunction(LUXI_CLASS_RESOURCE,PubResources_getName,(void*)PUB_RES_NAME,"getresname",
    "(string,[string]):(resource) - returns the resource filename and optional second argname");
  FunctionPublish_addFunction(LUXI_CLASS_RESOURCE,PubResources_getName,(void*)PUB_RES_NAMESHORT,"getresshortname",
    "(string,[string]):(resource) - returns the resource filename without paths and optional second argname");
  FunctionPublish_addFunction(LUXI_CLASS_RESOURCE,PubResources_getName,(void*)PUB_RES_NAMEUSER,"resuserstring",
    "([string]):(resource,[string]) - returns or sets a user string that is permanent for each resource. String must be shorter than 256 characters.");

  FunctionPublish_initClass(LUXI_CLASS_RESSUBID,"ressubid",
    "Some resources contain sub resources, such as a model is made of meshes. Ressubids allow access to those.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_RESSUBID,PubResSubRID_getParts,NULL,"hostinfo",
    "(int index,[resource]):(ressubid) - returns the index in the host and the host resource");

  FunctionPublish_setParent(LUXI_CLASS_SKINID,LUXI_CLASS_RESSUBID);
  FunctionPublish_setParent(LUXI_CLASS_MESHID,LUXI_CLASS_RESSUBID);
  FunctionPublish_setParent(LUXI_CLASS_PARTICLEFORCEID,LUXI_CLASS_RESSUBID);
  FunctionPublish_setParent(LUXI_CLASS_BONEID,LUXI_CLASS_RESSUBID);
  FunctionPublish_setParent(LUXI_CLASS_MATCONTROLID,LUXI_CLASS_RESSUBID);
  FunctionPublish_setParent(LUXI_CLASS_MATTEXCONTROLID,LUXI_CLASS_RESSUBID);
  FunctionPublish_setParent(LUXI_CLASS_MATSHDCONTROLID,LUXI_CLASS_RESSUBID);
  FunctionPublish_setParent(LUXI_CLASS_TRACKID,LUXI_CLASS_RESSUBID);
  FunctionPublish_setParent(LUXI_CLASS_SHADERPARAMID,LUXI_CLASS_RESSUBID);

  FunctionPublish_initClass(LUXI_CLASS_RESCHUNK,"reschunk",
    "resdata is separated into resource chunks, each chunk has a memory pool, \
and can hold a specific amount of resources. Every chunk also generates Vertex/IndexBufferObjects for the models if needed.<br><br>\
If you want to clear resources and \
keep others active during runtime, you will need multiple resource chunks. \
When a chunk is destroyed it will however keep the resources \
that are still needed by other chunks. Since the 'unique resource' \
lookup on loading is done over all resources and not individually \
to save memory, this was needed. \
The separation in ResourceChunks can be done only once until \
the built in maximums are hit. \
On startup a core chunk is generated, only to hold\
all resources that the engine itself needs. A default chunk is allocated (8 megs) afterwards. \
If you destroy the default, you must set up all yourself.<br> \
The user can specify what should happen when a chunk is full \
and a resource wants to be loaded in it. you can either throw an \
error, or just fill the next chunk.<br> \
<br> \
Resources are classified as 'direct' loaded or 'indirect' \
indirect are those resources that are loaded by other resources. \
When a ResourceChunk is reloaded only 'directlys' are loaded again, \
of course with their dependants \
This is done to prevent collecting unused resources, when \
e.g. materials or other scripts have changed some textures \
might not be needed anymore.<br> \
<br>"
    ,NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_RESCHUNK,PubResourceChunk_new,(void*)0,"create",
    "(reschunk):(string name,int mbsize,|int kbsize|, float restypecnt...) -\
creates a new resource chunk, with given sized memory pool (in megabytes, kbsize optional),\
and given number of openslots per restype. The slotcount comes at the 'restyped' position. \
When a count is < zero it is used as multiplied of the total slots. So -1 tries to use all slots, -0.5 half and so on. Of course if more slots are already in use, than queried for, we will use the maximum of those still available.<br>\
Make sure you have called destroydefault() before you use your own chunks.<br>");
  FunctionPublish_addFunction(LUXI_CLASS_RESCHUNK,PubResourceChunk_func,(void*)PUBRES_ACTIVATE,"activate",
    "():(reschunk) - makes this the active reschunk, all new resources will be loaded into it.");
  FunctionPublish_addFunction(LUXI_CLASS_RESCHUNK,PubResourceChunk_func,(void*)PUBRES_CLEAR,"clearload",
    "():(reschunk,int indirection) - clears all resources in this chunk.\
Then reloads resources with a smaller or equal indirection depth than the given, or none if indirection is negative.\
About indirection: eg. a model loads a texture and a material, both have a ind. depth of 1, the material loads a \
texture and shader, both with depth of 2. The shader finally loads another texture with depth of 3. So we have \
3 textures with depths of 1,2,3 and 1 material with depth of 1, and 1 shader with depth of 1. The model has level 0, and to do a complete clear you pass -1.<br>\
Additionally we will always try to reload resources that were referenced from other reschunks.");
  FunctionPublish_addFunction(LUXI_CLASS_RESCHUNK,PubResourceChunk_func,(void*)PUBRES_DESTROYDEFAULT,"destroydefault",
    "():() - destroys the default chunk. If you want to do your own handling you must call this right at start, before you create new reschunks.");
  FunctionPublish_addFunction(LUXI_CLASS_RESCHUNK,PubResourceChunk_func,(void*)PUBRES_USECORE,"usecore",
    "():(boolean) - uses core memory chunk (very small and reserved for luxinia core modules, you should never use this).");
  FunctionPublish_addFunction(LUXI_CLASS_RESCHUNK,PubResourceChunk_func,(void*)PUBRES_GETDEFAULT,"getdefault",
    "(reschunk):() - returns default chunk (or first if default was destroyed).");
  FunctionPublish_addFunction(LUXI_CLASS_RESCHUNK,PubResourceChunk_func,(void*)PUBRES_CURRENT,"getcurrent",
    "(reschunk):() - returns current active chunk.");
  FunctionPublish_addFunction(LUXI_CLASS_RESCHUNK,PubResourceChunk_func,(void*)PUBRES_GETLOADED,"getloaded",
    "(int count/resource):(reschunk, int restype, [int nth]) - returns either number of all loaded resources or the nth resource of that type.");
  FunctionPublish_addFunction(LUXI_CLASS_RESCHUNK,PubResourceChunk_func,(void*)PUBRES_VBOSTATS,"vbostats",
    "(string,string):(reschunk) - returns strings on vertexbufferobjects useage statistics for this chunk.");
  FunctionPublish_addFunction(LUXI_CLASS_RESCHUNK,PubResourceChunk_func,(void*)PUBRES_NEWVBOSIZE,"vbonewsize",
    "([int vbo,int ibo]):([int vbo,int ibo]) - returns or sets the size of new allocated Vertex/IndexBufferObjects in KiloBytes. Be aware that using a size greater 2048, means that only vertex64 can be addressed via shorts.");

  FunctionPublish_initClass(LUXI_CLASS_FILELOADER,"fileloader","",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_FILELOADER,PubFileSys_setfunc,(void*)PUBFILE_OPENFILE,"setfileopen",
    "():() -");
  FunctionPublish_addFunction(LUXI_CLASS_FILELOADER,PubFileSys_setfunc,(void*)PUBFILE_FILEEXISTS,"setfileexists",
    "():() -");

  FunctionPublish_initClass(LUXI_CLASS_PGRAPH,"pgraph","profiling graph. If render.drawpgraph is enabled sampling is done per frame and rendered as line graph at the end.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_get, (void*)PGRAPH_PARTICLE, "getthink",
    "(pgraph) : () - think pgraph (lua mostly).");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_get, (void*)PGRAPH_ODESTEP, "getphysics",
    "(pgraph) : () - ode simulation pgraph.");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_get, (void*)PGRAPH_RENDER, "getrender",
    "(pgraph) : () - rendering pgraph without full accurate driver time (unless pgraph.renderfinish is set)");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_get, (void*)PGRAPH_PARTICLE, "getparticle",
    "(pgraph) : () - particle pgraph is subpart of render pgraph.");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_get, (void*)PGRAPH_L2D, "getl2d",
    "(pgraph) : () - l2d pgraph is subpart of render pgraph");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_get, (void*)PGRAPH_L3D, "getl3d",
    "(pgraph) : () - l3d pgraph is subpart of render pgraph)");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_get, (void*)PGRAPH_FRAME, "getvistest",
    "(pgraph) : () - vistest frame pgraph");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_get, (void*)PGRAPH_USER0, "getuser0",
    "(pgraph) : () - user pgraph, the only ones you should be adding samples to");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_get, (void*)PGRAPH_USER1, "getuser1",
    "(pgraph) : () - user pgraph, the only ones you should be adding samples to");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_get, (void*)PGRAPH_USER2, "getuser2",
    "(pgraph) : () - user pgraph, the only ones you should be adding samples to");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_get, (void*)PGRAPH_USER3, "getuser3",
    "(pgraph) : () - user pgraph, the only ones you should be adding samples to");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_func, (void*)PUBPG_POS, "drawpos",
    "([float x,y,z]) : ([float x,y,z]) - returns or sets position of the pgraphs");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_func, (void*)PUBPG_SIZE, "drawsize",
    "([float x,y]) : ([float x,y]) - returns or sets size of the pgraphs");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_func, (void*)PUBPG_COLOR, "color",
    "([float r,g,b]) : (pgraph,[float r,g,b]) - returns or sets color of the pgraph");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_func, (void*)PUBPG_STIPPLE, "stipple",
    "([float r,g,b]) : (pgraph,[float r,g,b]) - returns or sets if stippling should be used");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_func, (void*)PUBPG_SAMPLE, "addsample",
    "() : (pgraph,float sample) - adds a sample to the cached values.");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_func,(void*)PUBPG_DRAW,"draw",
    "([boolean]):([pgraph],[boolean]) - draw pgraph individually or all, also enables time sampling.");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_func, (void*)PUBPG_MAX, "drawmax",
    "([float]) : ([pgraph],[float]) - returns or sets maximum value,individually or for all. (default is 16.667 for 60 fps as internal graphs sample in milliseconds). When drawing the values are scaled and clamped accordingly.");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_func,(void*)PUBPG_START,"starttimesample",
    "():(pgraph) - starts a time sample. (in milliseconds)");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_func,(void*)PUBPG_END,"endtimesample",
    "():(pgraph) - ends the time sample and adds it to graph.");
  FunctionPublish_addFunction(LUXI_CLASS_PGRAPH,PubPGraph_func,(void*)PUBPG_RENDERFINISH,"renderfinish",
    "([boolean]):([boolean]) - renderpgraph will show full rendering time, it will wait until all rendering commands are executed by driver. This will reduce performance greatly as it kills all parallelism.");
}
