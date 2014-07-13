// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_SHADER_H__
#define __GL_SHADER_H__

/*
GL_SHADER
---------

The Shader compiling is done here, also the setup of texture stages and passes.
The compilation is fairly complex, but tries to get the best out of the hardware,
with a focus on OGL 1.3

Shader "Compilation":

VColorMod:
when there is consecutive stages that all contain the VColor flag
the last one is marked as colormod
expl: a * v OP b * v OP c * v = (a OP b OP c) * v

Swapping Stages:
we will swap ADD/MODADDs to get MODADD next to a REPLACE stage, or to pack VColorMod
expl: a + b*ALPHA = b*ALPHA + a
expl: a*v + b + c*v*ALPHA + d*v = (a + b + c*ALPHA)*v + b = (c*ALPHA + a + b)*v + b

Prefer ADD blend over AMODADD:
when a ADD blendpass is used we can still use AMODADDs in the same pass, however we cant do the opposite
expl: x + a + b*ALPHA  = x + (a + b*ALPHA)
expl: x + a*ALPHA + b != x + (a + b)*ALPHA
however we will combine AMODADD_PRIMs and not favor ADD blend in that case

Crossbar:
similar to swapping we will try to get as much done before VColor is applied


Special Combiners explanation:


Bump:
primary.color = light1-3.color + light0.ambient

Texunit         Combiner
tex0 normalmap      dot3( tex0.tex1 )
tex1 cubemap      add( primary.color, prev ) || modulate(light_color,prev) + primary.color
texunit > 2 (no mod_add combiner):
 tex1 cubemap     modulate( light_color, prev )
 tex2 cubemap     add(prev,primary.color)


Author: Christoph Kubisch

*/

#include <luxinia/luxcore/contmacrolinkedlist.h>
#include "../resource/shader.h"

typedef enum GLShaderConvert_e{
  GL_SHADER_NOPASS,       // no extra pass needed
  GL_SHADER_NOPASS_COLORMOD_NEXT, // an extra stage is needed for color mod
  GL_SHADER_NOPASS_CROSSBAR,    // making use of crossbar functionality
  GL_SHADER_NOPASS_CROSSBAR_CHAIN,// multiple crossbars as chain
  GL_SHADER_PASS_COLORMOD_NOW,  // colormod is done now, the operation in a new pass
  GL_SHADER_PASS,         // at least one more pass
  GL_SHADER_PASS_COLOR      // a pass just doing color mod
}GLShaderConvert_t;

typedef enum GLGPUComboOffset_e{
  GL_GPUCOMBO_FOGGED = 5,   // lights+1
  GL_GPUCOMBO_SKINNED = 10,
  GL_GPUCOMBO_MAX = 20,
}GLGPUComboOffset_t;

typedef struct GLShaderStage_s{
  ShaderStage_t *stage;

  VIDBlend_t    blend;
  VIDTexBlendHash texblend;

  struct GLShaderStage_s LUX_LISTNODEVARS;
}GLShaderStage_t;

typedef struct GLShaderPass_s{
  GLShaderStage_t   *glstageListHead;
  int         numStages;
  int         numTexStages;

  VID_GPU_t     vertexGPU;
  VID_GPU_t     fragmentGPU;
  VID_GPU_t     geometryGPU;

  ushort        tangents;
  ushort        custattribs;

  VIDBlend_t      blend;
  flags32       rflagson;
  flags32       rflagsoff;

  Shader_t      *shader;

  int         combo;  // uses comboprogs
  CGprogram     cgcomboprogs[GL_GPUCOMBO_MAX];  // 5 lights, x2 for skinned/normal x2 for fog/non fog

  struct GLShaderPass_s *next,*prev;
}GLShaderPass_t;

///////////////////////////////////////////////////////////////////////////////
// Functions


LUX_INLINE void GLShaderPass_deactivate(GLShaderPass_t *pass)
{
  if (pass->tangents){
    glDisableVertexAttribArrayARB(VERTEX_ATTRIB_TANGENT);
  }
  /*
  if (pass->custattribs){
    if (pass->custattribs & (1<<VERTEX_ATTRIB_ATTR5)){
      glDisableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR5);
    }
    if (pass->custattribs & (1<<VERTEX_ATTRIB_ATTR6)){
      glDisableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR6);
    }
    if (pass->custattribs & (1<<VERTEX_ATTRIB_ATTR12)){
      glDisableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR12);
    }
    if (pass->custattribs & (1<<VERTEX_ATTRIB_ATTR13)){
      glDisableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR13);
    }
    if (pass->custattribs & (1<<VERTEX_ATTRIB_ATTR15)){
      glDisableVertexAttribArrayARB(VERTEX_ATTRIB_ATTR15);
    }
  }
  */
  g_VID.drawsetup.attribs.needed = 0;
  g_VID.drawsetup.setnormals = LUX_FALSE;
}

#define GLShaderPass_activate(shader,pass,togglelit) {\
  if (shader->cgMode) GLShaderPass_activateCG(pass,togglelit);\
  else        GLShaderPass_activateFIXEDARB(pass,togglelit);\
}


  // create appropriate GL environment
  // RPOOLUSE
void GLShaderPass_activateFIXEDARB(const GLShaderPass_t *pass, int *togglelit);
void GLShaderPass_activateCG(const GLShaderPass_t *pass, int *togglelit);


  // Shader compile
void Shader_initGL(Shader_t *shader);

void Shader_initGpuPrograms(Shader_t *shader);
void Shader_clearGpuPrograms(Shader_t *shader);

#endif

