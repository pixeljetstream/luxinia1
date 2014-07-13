// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include <math.h>
#include "../common/vid.h"
#include "../common/3dmath.h"
#include "shader.h"
#include "resmanager.h"
#include "../render/gl_shader.h"

//////////////////////////////////////////////////////////////////////////
// SHADER

ShaderStage_t*  Shader_searchStageRID(Shader_t *shader,const int id){
  int i;
  for (i = 0; i < shader->numStages; i++){
    if (shader->stages[i]->id == id)
      return shader->stages[i];
  }

  return NULL;
}

int Shader_searchParam(Shader_t *shader, const char *name, int pass){
  int i;
  int n;
  ShaderParam_t **param;
  if (!name)
    return -1;

  param = shader->totalParams;
  n = -1;
  for (i = 0; i < shader->numTotalParams; i++,param++){
    if((*param) && strcmp((*param)->name,name)==0){
      if (pass==0)
        return i;
      pass--;
      n = i;
    }
  }
  return n;
}

int Shader_getTextureStageIndex(Shader_t *shader, int resid)
{
  int i;
  ShaderStage_t **sstage = shader->stages;

  for (i = 0; i < shader->numStages; i++,sstage++){
    if ((*sstage)->stagetype == SHADER_STAGE_TEX && (*sstage)->srcRID == resid)
      return i;
  }
  return -1;
}


void Shader_clear(Shader_t *shader){
  ShaderStage_t *stage;
  int i,n;
  int lastRID;

  Shader_clearGpuPrograms(shader);

  if (shader->alphatex){
    if(shader->alphatex->srcName)
      ResData_unrefResource(RESOURCE_TEXTURE,shader->alphatex->srcRID,shader);
  }

  for (i=0; i < shader->numStages; i++){
    stage = shader->stages[i];
    if (stage->stagetype == SHADER_STAGE_GPU){
      lastRID = -1;
      for (n = 0; n < VID_MAX_LIGHTS+1; n++) {
        if (stage->gpuinfo->srcRIDs[n] == lastRID)
          break;
        ResData_unrefResource(RESOURCE_GPUPROG,lastRID = stage->gpuinfo->srcRIDs[n],shader);
      }
      lastRID = -1;
      for (n = 0; n < VID_MAX_LIGHTS+1; n++) {
        if (stage->gpuinfo->srcSkinRIDs[n] == lastRID)
          break;
        ResData_unrefResource(RESOURCE_GPUPROG,lastRID = stage->gpuinfo->srcSkinRIDs[n],shader);
      }

    }
    else if (stage->stagetype == SHADER_STAGE_TEX && stage->srcName){
      ResData_unrefResource(RESOURCE_TEXTURE,stage->srcRID,shader);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
// ShaderStage

void ShaderStage_clear(ShaderStage_t *stage){
  memset(stage,0,sizeof(ShaderStage_t));
  stage->rgbscale = 0;
  stage->alphascale = 0;
}

//////////////////////////////////////////////////////////////////////////
// ShaderParam

int ShaderParam_needsWorldInv(ShaderParam_t *param){
  int needsworldinv[]={
    SHADER_PARAM_CAMUP,
    SHADER_PARAM_CAMPOS,
    SHADER_PARAM_CAMDIR,
    SHADER_PARAM_LIGHTDIR,
    SHADER_PARAM_LIGHTPOS,
    -1,
  };
  int *type = needsworldinv;
  while (*type >= 0){
    if (*type == param->type)
      return LUX_TRUE;
    type++;
  }
  /*
  ptype = (int)param->type - (int)SHADER_PARAM_TEXGEN0 ;
  if (ptype >= 0 && ptype < 4 && param->upvalue2)
    return TRUE;

  ptype = (int)param->type - (int)SHADER_PARAM_TEXMAT0 ;
  if (ptype >= 0 && ptype < 4 && param->upvalue2)
    return TRUE;
  */

  if (param->type == SHADER_PARAM_VALUE && param->upvalue)
    return LUX_TRUE;

  return LUX_FALSE;
}


