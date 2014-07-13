// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../fileio/filesystem.h"
#include "../resource/resmanager.h"
#include "gpuprog.h"



void GpuProg_clear(GpuProg_t *prog)
{
  if (prog->source){
    lxMemGenFree(prog->source,prog->sourcelen);
  }
  prog->source = NULL;
  prog->sourcelen = 0;

  GpuProg_clearGL(prog);
}

void GpuProg_clearGL(GpuProg_t *prog)
{
  int i;
  GpuProgParam_t *param;

  if (prog->progID)
    glDeleteProgramsARB(1,&prog->progID);
  else if (prog->cgProgram)
    cgDestroyProgram(prog->cgProgram);
  // FIXME use cgGLUnLoad instead

  prog->progID = 0;
  prog->cgProgram = NULL;

  vidCgCheckError();

  // actually if cgGLUnLoad was used in clearGL
  // this isnt needed
  param = prog->cgparams;
  for (i=0; i < prog->numCgparams; i++,param++){
    if (param->cgparam && cgGetNumConnectedToParameters(param->cgparam)==0)
      cgDestroyParameter(param->cgparam);
    vidCgCheckError();
    param->cgparam = NULL;
  }
}

void GpuProg_fixSource(GpuProg_t *prog)
{
  char  buffer[16];
  int   boneids = g_VID.gensetup.hwbones*3;
  char* src = prog->source;


  sprintf(buffer,"%d",boneids);
  lxStrReplace(src,"VID_BONESMAX",buffer,' ');

  sprintf(buffer,"%d",boneids-1);
  lxStrReplace(src,"VID_BONESLAST",buffer,' ');


}
void GpuProg_writeCgCompiled(GpuProg_t *prog,int broken)
{
  const char *src = cgGetProgramString(prog->cgProgram,CG_COMPILED_PROGRAM);
  if (src){
    char *outname = buffermalloc(sizeof(char)*1024);
    int length = strlen(src);
    FILE *fout;

    LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

    outname[0] = 0;
    sprintf(outname,"%s.%s^%s.%s%s",prog->resinfo.name,prog->resinfo.extraname,cgGetProfileString(prog->cgProfile),broken ? "BROKEN.gl" : "gl",vidCgProfileisGLSL(prog->cgProfile) ? "sl" : "p");

    fout = fopen(outname,"wb");
    if (fout){
      fwrite(src,sizeof(char)*(length+1),1,fout);
      fclose(fout);
    }
    bufferclear();
  }
}

int GpuProg_initGL(GpuProg_t *prog){
  // NOTE warning this func is called at runtime on resreload
  if (prog->type == GPUPROG_V_ARB || prog->type == GPUPROG_F_ARB)
  {
    char *err;
    int i;
    int length = strlen(prog->source);

    glGenProgramsARB(1, &prog->progID);
    glBindProgramARB(prog->type, prog->progID);
    glProgramStringARB(prog->type, GL_PROGRAM_FORMAT_ASCII_ARB, length, prog->source);
    lxMemGenFree(prog->source,prog->sourcelen);
    prog->source = NULL;

    err = (char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
    if (err && strcmp(err,"")!=0 && !strstr(err,"warning")){
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &i);
      bprintf("ERROR gpuprog: %s\n\t%s (pos %d)\n",prog->resinfo.name, err,i);
      return LUX_FALSE;
    }
    prog->vidtype = VID_ARB;
  }
  else if (prog->type == GPUPROG_V_GLSL || prog->type == GPUPROG_F_GLSL)
  {
    // TODO
    //  custom load for include and "entry" function
    //  extraction of attrib and texunit assigns
  }
  else if (prog->type == GPUPROG_V_CG || prog->type == GPUPROG_F_CG || prog->type == GPUPROG_G_CG)
  {
    char *buffer;
    char *compilerargs;
    char *cgcstring;
    char **args = NULL;
    char *entryname = prog->resinfo.extraname;
    CGerror error;
    char *entryfunc;
    int curlength = 0;
    ResourceChunk_t *oldchunk;

    int compiled = prog->source != NULL;

    bufferclear();
    buffer = buffermalloc(sizeof(char)*1024);

    // split compilerargs/entry
    if (entryname){
      strcpy(buffer,entryname);
      cgcstring = strstr(buffer,RES_CGC_SEPARATOR);
      if (cgcstring){
        *cgcstring = 0;
        cgcstring++;
      }
    }

    entryfunc = (entryname) ? buffer : "main";

    {
      char *c;
      char *first;
      int a = 0;

      compilerargs =  buffermalloc(sizeof(char)*GPUPROG_MAX_COMPILERARG_LENGTH+100);
      args =      buffermalloc(sizeof(char)*GPUPROG_MAX_COMPILERARG_LENGTH+100);
      LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

      compilerargs[0] = 0;

      if (cgcstring && *cgcstring)
        strcpy(compilerargs,cgcstring);
      // bugfix for ATI drawbuffers option
      if (GLEW_ATI_draw_buffers)
        strcat(compilerargs,"-po ATI_draw_buffers;");


      c = compilerargs;
      first = compilerargs;


      while (*c){
        if (*c == ';' || *c == ' '){
          *c = 0;
          args[a] = first;
          a++;
          first = c+1;
        }
        c++;
      }
      args[a] = NULL;
    }

    vidCgCheckErrorF("gpuprogprecheck",0);

    // FIXME
    // on "reinit" simply cgGLLoad could be used
    // instead of recompile

    if (compiled){
      prog->cgProgram = cgCreateProgram(g_VID.cg.context,CG_OBJECT,prog->source,prog->cgProfile,entryfunc,args);
      lxMemGenFree(prog->source,prog->sourcelen);
      prog->source = NULL;
    }
    else{
      if (g_VID.cg.useoptimal && !g_VID.cg.optimalinited)
        VIDCg_initProfiles();
      prog->cgProgram = cgCreateProgramFromFile(g_VID.cg.context,CG_SOURCE,prog->resinfo.name,prog->cgProfile,entryfunc,args);
    }

    error = cgGetError();
    if (error != CG_NO_ERROR){
      bprintf("ERROR gpuprog: %s - %s\n\t%s\n",prog->resinfo.name,entryfunc,cgGetErrorString(error));
      if (error == CG_COMPILER_ERROR) {
        bprintf("ERROR gpuprog: Error: %s\n", cgGetLastListing(g_VID.cg.context));
      }
      return LUX_FALSE;
    }
    prog->vidtype = ( prog->cgProfile == g_VID.cg.glslFragmentProfile ||
              prog->cgProfile == g_VID.cg.glslVertexProfile /*||
              prog->cgProfile == g_VID.cg.glslGeometryProfile*/) ? VID_GLSL : VID_ARB;

    if (prog->vidtype != VID_GLSL){
      cgGLLoadProgram(prog->cgProgram);
      error = cgGetError();
      if (error != CG_NO_ERROR){
        bprintf("ERROR gpuprog: %s - %s\n\t%s\n",prog->resinfo.name,entryfunc,cgGetErrorString(error));

        if (g_VID.cg.dumpbroken && !compiled){
          GpuProg_writeCgCompiled(prog,LUX_TRUE);
        }

        return LUX_FALSE;
      }
    }

    if (g_VID.cg.dumpallcompiled && !compiled){
      GpuProg_writeCgCompiled(prog,LUX_FALSE);
    }


    oldchunk = ResData_setChunkFromPtr(&prog->resinfo);

    // create parameter array
    //if(prog->vidtype != VID_GLSL){

    // Tell Cg programs which resinfo they have
    cgSetIntAnnotation(cgCreateProgramAnnotation(prog->cgProgram,GPUPROG_CG_ANNO_RES,CG_INT),prog->resinfo.resRID);
    if (!prog->cgparams){
      CGparameter  cgparam = cgGetFirstParameter(prog->cgProgram,CG_PROGRAM);
      const char **paramnames;

      paramnames = bufferzalloc(sizeof(char*)*256);
      LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

      prog->numCgparams = 0;

      while(cgparam && prog->numCgparams < 256){
        CGenum cgvar = cgGetParameterVariability(cgparam);
        if (cgvar == CG_UNIFORM){
          CGtype cgtype = cgGetParameterType(cgparam);
          // only store FLOAT4s and ARRAYS, as those can be accessed by Shader
          if (cgtype== CG_FLOAT4 || (cgtype == CG_ARRAY && cgGetParameterBaseType(cgparam) == CG_FLOAT)){
            paramnames[prog->numCgparams++] = cgGetParameterName(cgparam);
          }
        }

        cgparam = cgGetNextParameter( cgparam );
      }

      if (prog->numCgparams > 0){
        int i;
        prog->cgparams = reszalloc(sizeof(GpuProgParam_t)*prog->numCgparams);
        for (i = 0; i < prog->numCgparams; i++){
          prog->cgparams[i].cgparam = NULL;
          prog->cgparams[i].name = paramnames[i];
        }
      }
    }
    else if (!prog->numCgparams){
      // prevent reinit of cgparams
      prog->cgparams = (void*)1;
    }
    //}

    vidCgCheckError();

    // autoconnect a few parameters
    ResourceChunk_activate(oldchunk);

  }

  vidCheckError();

  return LUX_TRUE;
}

void GpuProg_getGL(GpuProg_t *prog)
{
  int length;
  if (prog->type == GPUPROG_V_ARB || prog->type == GPUPROG_F_ARB){
    glBindProgramARB(prog->type,prog->progID);

    glGetProgramivARB(prog->type, GL_PROGRAM_LENGTH_ARB, &length);
    length++;
    prog->source = lxMemGenAlloc(sizeof(char)*length);
    prog->sourcelen = length;
    glGetProgramStringARB(prog->type,GL_PROGRAM_STRING_ARB,prog->source);
    prog->source[length-1] = 0;
  }
  else if (prog->type == GPUPROG_V_GLSL || prog->type == GPUPROG_F_GLSL){
  }
  else if (prog->type == GPUPROG_V_CG || prog->type == GPUPROG_F_CG || prog->type == GPUPROG_G_CG){
    // retreive compiled string and use it, is faster
    const char *source = cgGetProgramString( prog->cgProgram,CG_COMPILED_PROGRAM);
    length =1 + strlen(source);
    prog->source = lxMemGenAlloc(sizeof(char)*length);
    prog->sourcelen = length;
    strcpy(prog->source,source);
    prog->source[length-1] = 0;
    vidCgCheckError();
  }
}

CGparameter CGprogram_GroupAddOrFindParam(CGprogram *usedProgs, const int numUsedProgs, const char *name,CGtype cgtype, const int arraycnt)
{
  int i = 0,n = 0;
  int resRID;
  int numVals;
  CGparameter cgparam = NULL;

  GpuProg_t *prog;
  GpuProgParam_t *progparam;

  int     numAllgpuprogs = 0;
  GpuProg_t **allgpuprogs;

  allgpuprogs = buffermalloc(sizeof(GpuProg_t*)*numUsedProgs);
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

  // search all progs for local GpuProgParam already set
  while (i < numUsedProgs){
    // retrieve GpuProg via annotation
    CGannotation cgann = cgGetNamedProgramAnnotation(usedProgs[i],GPUPROG_CG_ANNO_RES);
    resRID = cgann ? *(cgGetIntAnnotationValues(cgann,&numVals)) : -1;
    prog = ResData_getGpuProg(resRID);
    if (prog){
      allgpuprogs[numAllgpuprogs++] = prog;

      if (!cgparam){
        progparam = prog->cgparams;
        for (n = 0; n < prog->numCgparams; n++,progparam++){
          if (progparam->cgparam && strcmp(progparam->name,name)==0){
            cgparam = progparam->cgparam;
            break;
          }
        }
      }
    }
    vidCgCheckError();
    i++;
  }
  // if none hase it, create
  if (!cgparam)
    cgparam = arraycnt ? cgCreateParameterArray(g_VID.cg.context,cgtype,arraycnt) : cgCreateParameter(g_VID.cg.context,cgtype);

  // duplicate the param for all
  for (i=0; i < numAllgpuprogs; i++){
    prog = allgpuprogs[i];
    progparam = prog->cgparams;
    for (n = 0; n < prog->numCgparams; n++,progparam++){
      if (strcmp(progparam->name,name)==0){
        progparam->cgparam = cgparam;
      }
    }
  }

  buffersetbegin(allgpuprogs);

  vidCgCheckError();
  return cgparam;
}


