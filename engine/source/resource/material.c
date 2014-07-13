// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../scene/actorlist.h"
#include "../scene/scenetree.h"
#include "../render/gl_list3d.h"
#include "../render/gl_window.h"


//////////////////////////////////////////////////////////////////////////
// MaterialAutoControl

static void MaterialAutoControl_free(MaterialAutoControl_t *mautoctrl)
{
  Reference_releaseWeakCheck(mautoctrl->targetRef);
  lxMemGenFree(mautoctrl,sizeof(MaterialAutoControl_t));
}
void RMaterialAutoControl_free(lxRmatautocontrol ref)
{
  MaterialAutoControl_free((MaterialAutoControl_t*)Reference_value(ref));
}

MaterialAutoControl_t* MaterialAutoControl_new(){
  MaterialAutoControl_t *mautoctrl = lxMemGenZalloc(sizeof(MaterialAutoControl_t));
  mautoctrl->reference = Reference_new(LUXI_CLASS_MATAUTOCONTROL,mautoctrl);
  return mautoctrl;
}


static MaterialAutoControl_t* MaterialAutoControl_runMatrix(MaterialAutoControl_t **mautoctrlptr,MaterialObject_t *matobj,lxMatrix44 inoutmat, int transposed)
{
  static lxMatrix44SIMD mat;
  static lxMatrix44SIMD temp;
  static lxMatrix44SIMD temp2;
  static lxVector4 vectemp;
  static lxVector4 vectemp2;

  union{
    List3DNode_t *l3dnode;
    LinkObject_t *lobj;
    void    *ptr;
  }uni;

  float *pPos;
  const float *pSelfPos;
  MaterialAutoControl_t *mautoctrl = *mautoctrlptr;


  if (!Reference_get(mautoctrl->targetRef,uni.ptr)){
    MaterialObject_setAutoControl(matobj,mautoctrlptr,NULL);
    return NULL;
  }

  if (mautoctrl->targetRefType < 0)
    pPos = ((float*)uni.l3dnode->finalMatrix)+12;
  else
    pPos = ((float*)uni.lobj->matrix)+12;

  switch(mautoctrl->type)
  {
  case MATERIAL_ACTRL_MAT_NODEROT:
    lxVector3Set(vectemp,0,0,1);
    pSelfPos = g_VID.drawsetup.worldMatrix+12;
    lxVector3Sub(vectemp2,pPos,pSelfPos);
    lxVector3Normalized(vectemp2);

    // rotate towards reference position
    lxMatrix44RotateAngle(temp2,vectemp2,vectemp);
    lxMatrix44ClearTranslation(temp2);
    temp2[15]=1.0f;

    // need to premul with worldmatrix (gpuprog) or mviewinv (fixed)
    // Matrix44Multiply1(temp2,g_VID.drawsetup.viewMatrixInv);

    if (!transposed){
      lxMatrix44TransposeSIMD(mat,temp2);
      lxMatrix44Multiply1SIMD(inoutmat,mat);
    }
    else
      lxMatrix44CopySIMD(inoutmat,mat);

    return mautoctrl;
  case MATERIAL_ACTRL_MAT_NODEPOS_PROJ:
    lxVector4Transform(vectemp,pPos,g_CamLight.camera->mviewproj);
    vectemp[0]=((-vectemp[0]/vectemp[3])*0.5f );
    vectemp[1]=((-vectemp[1]/vectemp[3])*0.5f );

    // prevent backprojection
    if (vectemp[2] < 0){
      lxMatrix44Clear(inoutmat);
      inoutmat[15] = 1.0f;
      return mautoctrl;
    }
    vectemp[2]= 0.0f;

    // create matrix
    // matrix needs to move screenspace coords and scale them based on sunscreensize
    // and fov


    // first move so that center matches sunpos in screencoords
    lxMatrix44IdentitySIMD(temp2);
    lxMatrix44SetTranslation(temp2,vectemp);


    // adjust size via fov
    if (g_CamLight.camera->fov > 0)
      vectemp2[0] = mautoctrl->vec4[0] / tan(LUX_DEG2RAD(g_CamLight.camera->fov)*0.5);
    else
      vectemp2[0] = mautoctrl->vec4[0];

    // then scale with aspect
    lxMatrix44IdentitySIMD(temp);
    vectemp2[1] = 1.0f/vectemp2[0];   // dist
    vectemp2[2] = (g_CamLight.camera->aspect < 0) ? g_Window.ratio : g_CamLight.camera->aspect; // aspect

    lxVector3Set(vectemp,vectemp2[1]*vectemp2[2],vectemp2[1],1);
    lxMatrix44SetScale(temp,vectemp);

    lxVector3Set(vectemp,(vectemp2[1]*vectemp2[2]*-0.5f) + 0.5f,(vectemp2[1]*-0.5f)+0.5f,0);
    lxMatrix44SetTranslation(temp,vectemp);

    // compute output matrix
    lxMatrix44MultiplySIMD(mat,temp,temp2);
    if (transposed)
      lxMatrix44TransposeSIMD(inoutmat,mat);
    else
      lxMatrix44Multiply1SIMD(inoutmat,mat);

    return mautoctrl;
  case MATERIAL_ACTRL_MAT_PROJECTOR:
    LUX_ASSERT(uni.l3dnode->nodeType == LUXI_CLASS_L3D_PROJECTOR);
    pPos = transposed ? uni.l3dnode->proj->vprojmatrixT : uni.l3dnode->proj->vprojmatrix;
    lxMatrix44CopySIMD(inoutmat,pPos);

    return mautoctrl;
  default:
    return mautoctrl;
  }
}

static MaterialAutoControl_t* MaterialAutoControl_runVector4(MaterialAutoControl_t **mautoctrlptr,MaterialObject_t *matobj,lxVector4 vec4)
{
  static lxMatrix44SIMD temp;

  union{
    List3DNode_t *l3dnode;
    LinkObject_t  *lobj;
    void    *ptr;
  }uni;
  float *pPos;
  const float *pSelfPos;
  MaterialAutoControl_t *mautoctrl = *mautoctrlptr;

  if (!Reference_get(mautoctrl->targetRef,uni.ptr)){
    MaterialObject_setAutoControl(matobj,mautoctrlptr,NULL);
    return NULL;
  }

  if (mautoctrl->targetRefType < 0)
    pPos = ((float*)uni.l3dnode->finalMatrix)+12;
  else
    pPos = ((float*)uni.lobj->matrix)+12;

  switch(mautoctrl->type)
  {
  case MATERIAL_ACTRL_VEC4_NODEPOS_PROJ:
    lxVector4Transform(vec4,pPos,g_CamLight.camera->mviewproj);
    return mautoctrl;
  case MATERIAL_ACTRL_VEC4_NODEPOS:
    lxVector3Copy(vec4,pPos);
    vec4[3] = 1;
    return mautoctrl;
  case MATERIAL_ACTRL_VEC4_NODEDIR:
    pSelfPos = g_VID.drawsetup.worldMatrix+12;
    lxVector3Sub(vec4,pPos,pSelfPos);
    lxVector3Normalized(vec4);
    vec4[3] = 0;
    return mautoctrl;
  default:
    return mautoctrl;
  }
}

///////////////////////////////////////////////////////////////////////////////
// MaterialObject

MaterialObject_t *MaterialObject_new(Material_t *mat)
{
  MaterialObject_t *matobj;
  int sizecontrols = (mat && mat->numControls ? (sizeof(MaterialAutoControl_t **)*mat->numControls) + (sizeof(float)*mat->numControlValues) : 0);
  int sizestages = (mat && mat->numStages ? sizeof(MaterialAutoControl_t **)*mat->numStages*2 : 0);
  int sizetexcontrols = (mat && mat->numResControls ? sizeof(int)*mat->numResControls*2 : 0);

  matobj = genzallocSIMD(sizeof(MaterialObject_t) + sizecontrols + sizestages + sizetexcontrols);

  LUX_SIMDASSERT((size_t)((MaterialObject_t*)matobj)->texmatrixData  % 16 == 0);

  if (mat){
    memcpy(matobj,&mat->matobjdefault,sizeof(MaterialObject_t));
    matobj->numControls = mat->numControls;
    matobj->numControlValues = mat->numControlValues;

    if (mat->numControls){
      matobj->controlValues = (float*)(matobj+1);
      memcpy(matobj->controlValues,mat->matobjdefault.controlValues,sizeof(float)*mat->numControlValues);
      matobj->mautoctrlControls = (MaterialAutoControl_t **)(((uchar*)(matobj+1))+(sizeof(float)*mat->numControlValues));
    }

    matobj->numStages = mat->numStages;
    if (matobj->numStages){
      matobj->mautoctrlStages = (MaterialAutoControl_t **)(((uchar*)(matobj+1))+sizecontrols);
    }
    matobj->numResControls = mat->numResControls;
    if (matobj->numResControls){
      matobj->resRIDs = (int*)(((uchar*)(matobj+1))+sizecontrols+sizestages);
      memcpy(matobj->resRIDs,mat->matobjdefault.resRIDs,sizeof(int)*mat->numResControls*2);
    }
  }

  return matobj;
}

LUX_INLINE void MaterialObject_setAutoControl(MaterialObject_t *matobj,MaterialAutoControl_t **ptr,MaterialAutoControl_t *autoctrl)
{
  if (*ptr){
    Reference_release(((MaterialAutoControl_t *)*ptr)->reference);
  }
  if (autoctrl){
    Reference_ref(autoctrl->reference);
  }
  *ptr = autoctrl;
}

static void MaterialObject_unrefAutoControls(MaterialObject_t *matobj)
{
  int i;
  MaterialAutoControl_t **mautoctrl;

  mautoctrl = matobj->mautoctrlStages;
  for (i = 0; i < matobj->numStages*2; i++,mautoctrl++){
    MaterialObject_setAutoControl(matobj,mautoctrl,NULL);
  }

  mautoctrl = matobj->mautoctrlControls;
  for (i = 0; i < matobj->numControls; i++,mautoctrl++){
    MaterialObject_setAutoControl(matobj,mautoctrl,NULL);
  }
}

void MaterialObject_free(MaterialObject_t *matobj)
{
  if (matobj){
    MaterialObject_unrefAutoControls(matobj);

    genfreeSIMD(matobj,sizeof(MaterialObject_t)+
        (matobj->numControls ? (sizeof(MaterialAutoControl_t **)*matobj->numControls)+(sizeof(float)*matobj->numControlValues) : 0)+
        (matobj->numStages ? sizeof(MaterialAutoControl_t **)*matobj->numStages*2 : 0)+
        (matobj->numResControls ? sizeof(int)*matobj->numResControls*2 : 0)
        );
  }
}

///////////////////////////////////////////////////////////////////////////////
// MaterialStage

static void MaterialStage_updateSeq(MaterialStage_t *stage,uint time){
  MaterialSeq_t *seq = stage->seq;
  float   diff;
  int diffi;
  int active;
  int next;

  // check if not needed
  if (seq->loops != 255 && seq->numLooped > seq->loops)
    return;

  // update time
  if (!time)
    time = g_LuxTimer.time+seq->delay;
  else
    time--;

  // check if its time for a full update or interpolate
  if (!seq->interpolate && ((time- seq->lastTime) < seq->delay))
    return;
  else if (stage->id == MATERIAL_COLOR0 && seq->interpolate &&((time- seq->lastTime) < seq->delay)){
    diff = (float)(time - seq->lastTime) * seq->delayfrac;
    active = seq->activeFrame;
    if (seq->reverse < 0)
      next = (seq->activeFrame-1 < 0) ? seq->numFrames : seq->activeFrame-1;
    else
      next = (seq->activeFrame+1) % seq->numFrames;

    lxVector3Lerp(stage->color,diff,&seq->colors[active*4],&seq->colors[next*4]);
    if (stage->color[3] >= 0)
      stage->color[3] = LUX_LERP(diff,seq->colors[active*4+3],seq->colors[next*4+3]);

    return;
  }
  // in case we need to wait
  if (seq->wait < 0 && ((time- seq->lastTime -seq->delay) < (uint)(abs(seq->wait))))
    return;

  // do updates
  if (stage->id == MATERIAL_COLOR0){
    float *flt = &seq->colors[seq->activeFrame*4];
    lxVector3Copy(stage->color,flt);
    if (stage->color[3] >= 0)
      stage->color[3]= seq->colors[seq->activeFrame*4+3];
  }
  else{
    stage->texRIDUse = seq->texRID[seq->activeFrame];
  }

  // deactivate wait and use "next" to clean time differences of waits
  if (seq->wait < 0){
    seq->wait = -seq->wait;
    next = seq->wait;
  }
  else
    next = 0;

  // no interpolation just throw in the values and find where we are timewise;
  diffi = (time - seq->lastTime-next)/seq->delay;

  // if we are currently in reverse mode
  if (seq->reverse < 0)
    diffi = -diffi;

  active = seq->activeFrame;
  active += diffi;


  // check if we have run thru the sequence and set wait/reverse status
  if ( active >= (int)seq->numFrames){
    seq->numLooped++;
    active %= seq->numFrames;
    // if we need to wait make wait active ie negative
    if (seq->wait)
      seq->wait = -seq->wait;
    // if we need to reverse
    if (seq->reverse && (seq->numLooped % abs(seq->reverse) == 0)){
      seq->reverse = -seq->reverse;
      active = seq->numFrames-active-1;
    }
  }
  else if (active < 0){
    seq->numLooped++;
    active = abs(active)-1;
    active %= seq->numFrames;
    active = seq->numFrames-active-1;
    if (seq->wait)
      seq->wait = -seq->wait;
    if (seq->reverse && (seq->numLooped % abs(seq->reverse) == 0)){
      seq->reverse = -seq->reverse;
      active= seq->numFrames-active-1;
    }
  }


  // set our status
  seq->activeFrame = active;
  seq->lastTime = time;

}

///////////////////////////////////////////////////////////////////////////////
// MaterialModifier

void MaterialModifier_init(MaterialModifier_t *mod,char *name, int type ,int delay, float value, uchar cap, void *data, int dataID)
{
  if (name)
    resnewstrcpy(mod->name,name)

  mod->value = value;


  if (delay < 0){
    mod->delay = -delay;
    mod->perframe = LUX_FALSE;
  }
  else{
    mod->delay = delay;
    mod->perframe = LUX_TRUE;
  }
  mod->delay = delay;
  mod->delayfrac = 1/(float)delay;
  mod->lastTime = g_LuxTimer.time;
  mod->runner = 0.0;
  mod->modflag = 0;
  mod->data = data;
  mod->dataID = dataID;

  if (cap == 0)
    mod->modflag |= MATERIAL_MOD_CAP0;
  if (cap == 1)
    mod->modflag |= MATERIAL_MOD_CAP1;
  if (cap == 2)
    mod->modflag |= MATERIAL_MOD_CAP2;

  if (type == 0)
    mod->modflag |= MATERIAL_MOD_ADD;
  if (type == 1)
    mod->modflag |= MATERIAL_MOD_SIN;
  if (type == 2)
    mod->modflag |= MATERIAL_MOD_COS;
  if (type == 3)
    mod->modflag |= MATERIAL_MOD_ZIGZAG;

}
void MaterialModifier_initSource(MaterialModifier_t *mod,float *source,int capped,MaterialValue_t target)
{
  mod->sourcep = source;
  mod->sourceval = *source;

  if (capped)
    mod->modflag |= MATERIAL_MOD_CAPPED;
  mod->modtarget = target;
}

void Material_initModifiers(Material_t *material){
  int i;
  MaterialModifier_t *mod;

  for (i=0; i < material->numMods; i++){
    mod = &material->mods[i];
    switch(mod->modtarget) {
      case MATERIAL_MOD_RGBA_1 :
      case MATERIAL_MOD_RGBA_2 :
      case MATERIAL_MOD_RGBA_3 :
      case MATERIAL_MOD_RGBA_4 :
        mod->sourcep = &material->stages[mod->dataID]->color[mod->modtarget - MATERIAL_MOD_RGBA_1];
        break;
      case MATERIAL_MOD_TEXMOVE_1 :
      case MATERIAL_MOD_TEXMOVE_2 :
      case MATERIAL_MOD_TEXMOVE_3 :
        mod->sourcep = &material->stages[mod->dataID]->texcoord->move[mod->modtarget - MATERIAL_MOD_TEXMOVE_1];
        break;
      case MATERIAL_MOD_TEXCENTER_1 :
      case MATERIAL_MOD_TEXCENTER_2 :
      case MATERIAL_MOD_TEXCENTER_3 :
        mod->sourcep = &material->stages[mod->dataID]->texcoord->center[mod->modtarget - MATERIAL_MOD_TEXCENTER_1];
        break;
      case MATERIAL_MOD_TEXSCALE_1 :
      case MATERIAL_MOD_TEXSCALE_2 :
      case MATERIAL_MOD_TEXSCALE_3 :
        mod->sourcep = &material->stages[mod->dataID]->texcoord->scale[mod->modtarget - MATERIAL_MOD_TEXSCALE_1];
        break;
      case MATERIAL_MOD_TEXROT_1 :
      case MATERIAL_MOD_TEXROT_2 :
      case MATERIAL_MOD_TEXROT_3 :
        mod->sourcep = &material->stages[mod->dataID]->texcoord->rotate[mod->modtarget - MATERIAL_MOD_TEXROT_1];
        break;
      case MATERIAL_MOD_TEXMAT0_1 :
      case MATERIAL_MOD_TEXMAT0_2 :
      case MATERIAL_MOD_TEXMAT0_3 :
      case MATERIAL_MOD_TEXMAT0_4 :
      case MATERIAL_MOD_TEXMAT1_1 :
      case MATERIAL_MOD_TEXMAT1_2 :
      case MATERIAL_MOD_TEXMAT1_3 :
      case MATERIAL_MOD_TEXMAT1_4 :
      case MATERIAL_MOD_TEXMAT2_1 :
      case MATERIAL_MOD_TEXMAT2_2 :
      case MATERIAL_MOD_TEXMAT2_3 :
      case MATERIAL_MOD_TEXMAT2_4 :
      case MATERIAL_MOD_TEXMAT3_1 :
      case MATERIAL_MOD_TEXMAT3_2 :
      case MATERIAL_MOD_TEXMAT3_3 :
      case MATERIAL_MOD_TEXMAT3_4 :
        mod->sourcep = &material->stages[mod->dataID]->texcoord->matrix[mod->modtarget - MATERIAL_MOD_TEXMAT0_1];
        break;
      case MATERIAL_MOD_TEXGEN0_1 :
      case MATERIAL_MOD_TEXGEN0_2 :
      case MATERIAL_MOD_TEXGEN0_3 :
      case MATERIAL_MOD_TEXGEN0_4 :
      case MATERIAL_MOD_TEXGEN1_1 :
      case MATERIAL_MOD_TEXGEN1_2 :
      case MATERIAL_MOD_TEXGEN1_3 :
      case MATERIAL_MOD_TEXGEN1_4 :
      case MATERIAL_MOD_TEXGEN2_1 :
      case MATERIAL_MOD_TEXGEN2_2 :
      case MATERIAL_MOD_TEXGEN2_3 :
      case MATERIAL_MOD_TEXGEN2_4 :
      case MATERIAL_MOD_TEXGEN3_1 :
      case MATERIAL_MOD_TEXGEN3_2 :
      case MATERIAL_MOD_TEXGEN3_3 :
      case MATERIAL_MOD_TEXGEN3_4 :
        mod->sourcep = &material->stages[mod->dataID]->texgen[mod->modtarget - MATERIAL_MOD_TEXGEN0_1];
        break;
      case MATERIAL_MOD_PARAM_1 :
      case MATERIAL_MOD_PARAM_2 :
      case MATERIAL_MOD_PARAM_3 :
      case MATERIAL_MOD_PARAM_4 :
        mod->sourcep = &material->shaders[mod->dataID].params[(int)mod->data].vecP[mod->modtarget - MATERIAL_MOD_PARAM_1];
        break;
    }
  }
}

static void MaterialModifier_update(MaterialModifier_t *mod,uint time)
{
  float   diff;
  float   out;
  float   scz;

  if (!time)
    time = g_LuxTimer.time;
  else
    time--;

  if (isFlagSet(mod->modflag,MATERIAL_MOD_OFF) || time == mod->lastTime){
    mod->lastTime = time;
    return;
  }
  if (!mod->perframe && ((time- mod->lastTime) < mod->delay))
    return;
  // how much do we need to add ?
  diff = (float)(time - mod->lastTime)*mod->delayfrac;

  // compute factor for sin/cos/zigzag and cap it if needed
  if (mod->modflag & MATERIAL_MOD_SIN || mod->modflag & MATERIAL_MOD_COS || mod->modflag & MATERIAL_MOD_ZIGZAG){
    mod->runner += diff;
    if (mod->runner > 2)
      mod->runner = (float)fmod(mod->runner,2);

    if (mod->modflag & MATERIAL_MOD_SIN)
      scz = lxFastSin(LUX_MUL_TWOPI * mod->runner);
    else if (mod->modflag & MATERIAL_MOD_COS)
      scz = lxFastCos(LUX_MUL_TWOPI * mod->runner);
    else if (mod->modflag & MATERIAL_MOD_ZIGZAG)
      scz = lxZigzag(mod->runner * 2);

    if (mod->modflag & MATERIAL_MOD_CAP0)
      scz = scz;
    else if (mod->modflag & MATERIAL_MOD_CAP1)
      scz = (scz >= 0) ? scz : -scz;
    else if (mod->modflag & MATERIAL_MOD_CAP2)
      scz = (scz <= 0) ? scz : -scz;
  }

  // compute out value
  if (mod->modflag & MATERIAL_MOD_ADD)
    out = mod->value * diff + mod->sourceval;
  else if (mod->modflag & MATERIAL_MOD_SIN)
    out = scz*mod->sourceval + mod->value;
  else if (mod->modflag & MATERIAL_MOD_COS)
    out = scz*mod->sourceval + mod->value;
  else if (mod->modflag & MATERIAL_MOD_ZIGZAG)
    out = scz*mod->sourceval + mod->value;
  else
    out = mod->sourceval;

  // if the value is clamped we need some extra magic for the ADD modifier
  if (mod->modflag & MATERIAL_MOD_CAPPED && (out < 0 || out > 1.0 )){
    // floats are capped 0-1
    if (out < 0.0f){
      if (mod->modflag & MATERIAL_MOD_ADD){
        if (mod->modflag & MATERIAL_MOD_CAP0)
          // flip to other side
          out = 1.0f + out;
        if (mod->modflag & MATERIAL_MOD_CAP1){
          // reverse
          out = -out;
          mod->value = - mod->value;
        }
        if (mod->modflag & MATERIAL_MOD_CAP2){
          // stop
          out = 0.0f;
          mod->value = 0.0f;
        }
      }
    }
    if (out > 1.0f){
      if (mod->modflag & MATERIAL_MOD_ADD){
        if (mod->modflag & MATERIAL_MOD_CAP0)
          // flip to other side
          out = -1.0f + out;
        if (mod->modflag & MATERIAL_MOD_CAP1){
          // reverse
          out =2.0f -out;
          mod->value = - mod->value;
        }
        if (mod->modflag & MATERIAL_MOD_CAP2){
          // stop
          out = 1.0f;
          mod->value = 0.0f;
        }
      }
    }
    // final check
    if (out < 0.0f)
      out = 0.0f;
    else
      out = 1.0f;
  }
  if (mod->modflag & MATERIAL_MOD_ADD){
      out = fmod(out,1024.0f);
      mod->sourceval = out;
  }

  *mod->sourcep = out;
  mod->lastTime = time;
}

///////////////////////////////////////////////////////////////////////////////
// MaterialControl

void MaterialControl_init(MaterialControl_t *ctrl,char *name, void *data, int dataID, int length)
{
  if (name)
    resnewstrcpy(ctrl->name,name)
  ctrl->data = data;
  ctrl->dataID = dataID;
  ctrl->length = length;
}
void MaterialControl_initSource(MaterialControl_t *ctrl,float *source,MaterialValue_t target)
{
  ctrl->sourcep = source;
  ctrl->modtarget = target;
}

void Material_initControls(Material_t *material){
  int i;
  MaterialControl_t *mod;

  int offset = 0;
  for (i=0; i < material->numControls; i++){
    mod = &material->controls[i];
    mod->offset = offset;
    switch(mod->modtarget) {
      case MATERIAL_MOD_RGBA_1 :
      case MATERIAL_MOD_RGBA_2 :
      case MATERIAL_MOD_RGBA_3 :
      case MATERIAL_MOD_RGBA_4 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_RGBA_4-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->color[mod->modtarget - MATERIAL_MOD_RGBA_1];
        break;
      case MATERIAL_MOD_TEXMOVE_1 :
      case MATERIAL_MOD_TEXMOVE_2 :
      case MATERIAL_MOD_TEXMOVE_3 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXMOVE_3-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texcoord->move[mod->modtarget - MATERIAL_MOD_TEXMOVE_1];
        break;
      case MATERIAL_MOD_TEXCENTER_1 :
      case MATERIAL_MOD_TEXCENTER_2 :
      case MATERIAL_MOD_TEXCENTER_3 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXCENTER_3-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texcoord->center[mod->modtarget - MATERIAL_MOD_TEXCENTER_1];
        break;
      case MATERIAL_MOD_TEXSCALE_1 :
      case MATERIAL_MOD_TEXSCALE_2 :
      case MATERIAL_MOD_TEXSCALE_3 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXSCALE_3-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texcoord->scale[mod->modtarget - MATERIAL_MOD_TEXSCALE_1];
        break;
      case MATERIAL_MOD_TEXROT_1 :
      case MATERIAL_MOD_TEXROT_2 :
      case MATERIAL_MOD_TEXROT_3 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXROT_3-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texcoord->rotate[mod->modtarget - MATERIAL_MOD_TEXROT_1];
        break;
      case MATERIAL_MOD_TEXMAT0_1 :
      case MATERIAL_MOD_TEXMAT0_2 :
      case MATERIAL_MOD_TEXMAT0_3 :
      case MATERIAL_MOD_TEXMAT0_4 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXMAT0_4-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texcoord->matrix[mod->modtarget - MATERIAL_MOD_TEXMAT0_1];
        break;
      case MATERIAL_MOD_TEXMAT1_1 :
      case MATERIAL_MOD_TEXMAT1_2 :
      case MATERIAL_MOD_TEXMAT1_3 :
      case MATERIAL_MOD_TEXMAT1_4 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXMAT1_4-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texcoord->matrix[mod->modtarget - MATERIAL_MOD_TEXMAT0_1];
        break;
      case MATERIAL_MOD_TEXMAT2_1 :
      case MATERIAL_MOD_TEXMAT2_2 :
      case MATERIAL_MOD_TEXMAT2_3 :
      case MATERIAL_MOD_TEXMAT2_4 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXMAT2_4-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texcoord->matrix[mod->modtarget - MATERIAL_MOD_TEXMAT0_1];
        break;
      case MATERIAL_MOD_TEXMAT3_1 :
      case MATERIAL_MOD_TEXMAT3_2 :
      case MATERIAL_MOD_TEXMAT3_3 :
      case MATERIAL_MOD_TEXMAT3_4 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXMAT3_4-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texcoord->matrix[mod->modtarget - MATERIAL_MOD_TEXMAT0_1];
        break;
      case MATERIAL_MOD_TEXGEN0_1 :
      case MATERIAL_MOD_TEXGEN0_2 :
      case MATERIAL_MOD_TEXGEN0_3 :
      case MATERIAL_MOD_TEXGEN0_4 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXGEN0_4-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texgen[mod->modtarget - MATERIAL_MOD_TEXGEN0_1];
        break;
      case MATERIAL_MOD_TEXGEN1_1 :
      case MATERIAL_MOD_TEXGEN1_2 :
      case MATERIAL_MOD_TEXGEN1_3 :
      case MATERIAL_MOD_TEXGEN1_4 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXGEN1_4-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texgen[mod->modtarget - MATERIAL_MOD_TEXGEN0_1];
        break;
      case MATERIAL_MOD_TEXGEN2_1 :
      case MATERIAL_MOD_TEXGEN2_2 :
      case MATERIAL_MOD_TEXGEN2_3 :
      case MATERIAL_MOD_TEXGEN2_4 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXGEN2_4-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texgen[mod->modtarget - MATERIAL_MOD_TEXGEN0_1];
        break;
      case MATERIAL_MOD_TEXGEN3_1 :
      case MATERIAL_MOD_TEXGEN3_2 :
      case MATERIAL_MOD_TEXGEN3_3 :
      case MATERIAL_MOD_TEXGEN3_4 :
        mod->length = LUX_MIN(mod->length,1+MATERIAL_MOD_TEXGEN3_4-mod->modtarget);
        mod->sourcep = &material->stages[mod->dataID]->texgen[mod->modtarget - MATERIAL_MOD_TEXGEN0_1];
        break;
      case MATERIAL_MOD_PARAM_1 :
      case MATERIAL_MOD_PARAM_2 :
      case MATERIAL_MOD_PARAM_3 :
      case MATERIAL_MOD_PARAM_4 :
        //mod->length = mod->length; MIN(mod->length,1+MATERIAL_MOD_PARAM_4-mod->modtarget);
        mod->sourcep = &material->shaders[mod->dataID].params[(int)mod->data].vecP[mod->modtarget - MATERIAL_MOD_PARAM_1];
        break;
    }
    offset += mod->length;
  }

  material->numControlValues = offset;

  mod = material->controls;
  material->matobjdefault.controlValues = reszalloc(sizeof(float)*material->numControlValues);
  for (i = 0; i < material->numControls; i++,mod++){
    memcpy(&material->matobjdefault.controlValues[mod->offset],mod->sourcep,sizeof(float)*mod->length);
  }
}

int Material_getControlIndex(Material_t *mat, const char *name, MaterialControlType_t controltype)
{
  int i;

  if (!mat)
    return -1;

  switch(controltype)
  {
  case MATERIAL_CONTROL_RESTEX:
  case MATERIAL_CONTROL_RESSHD:
    for (i = 0; i < mat->numResControls; i++){
      if (mat->rescontrols[i].type == controltype && strcmp(mat->rescontrols[i].name, name) == 0)
        return i;
    }
    break;
  case MATERIAL_CONTROL_FLOAT:
    for (i = 0; i < mat->numControls; i++){
      if (strcmp(mat->controls[i].name, name) == 0)
        return i;
    }
    break;
  }

  return -1;
}
///////////////////////////////////////////////////////////////////////////////
// MaterialResControl

void Material_initResControls(Material_t *material)
{
  MaterialResControl_t *tctrl;
  int i;

  if (!material->numResControls) return;

  material->matobjdefault.resRIDs = reszalloc(sizeof(int)*material->numResControls*2);

  tctrl = material->rescontrols;
  for (i=0; i < material->numResControls*2; i+=2,tctrl++){
    material->matobjdefault.resRIDs[i] = *tctrl->sourcep;
    material->matobjdefault.resRIDs[i+1] = *tctrl->sourcewinsizedp;
  }

}


///////////////////////////////////////////////////////////////////////////////
// Material

void Material_update(Material_t *material, MaterialObject_t *matobject){
  static lxMatrix44SIMD temp;
  MaterialObject_t *matobj;
  MaterialTexCoord_t *texcoord;
  MaterialControl_t *mcontrol;
  MaterialResControl_t *texcontrol;
  MaterialResControl_t *texcontrolend;
  MaterialAutoControl_t **mautoctrl;

  int i;
  float *val;
  MaterialStage_t **stages;


  matobj = (matobject) ? matobject : &material->matobjdefault;

  if (material->lastmatobj == matobj && material->lastmatobjframe == g_VID.frameCnt){
    return;
  }
  material->lastmatobj = matobj;
  material->lastmatobjframe = g_VID.frameCnt;

  if (!(matobj->materialflag & MATERIAL_MODOFF))
    for (i = 0; i < material->numMods; i++)
      MaterialModifier_update(&material->mods[i], matobj->time);
  if (!(matobj->materialflag & MATERIAL_SEQOFF)){
    stages = material->stages;
    for (i = 0; i < material->numStages; i++,stages++){
      if ((*stages)->seq)
        MaterialStage_updateSeq(*stages,  matobj->time);
    }
  }

  // update texcontrol
  texcontrol = material->rescontrols;
  texcontrolend = texcontrol+material->numResControls;
  for (i=0;texcontrol < texcontrolend; i+=2,texcontrol++){
    *texcontrol->sourcep = matobj->resRIDs[i];
    *texcontrol->sourcewinsizedp = matobj->resRIDs[i+1];
  }


  // update material control values
  mcontrol = material->controls;
  val = matobj->controlValues;
  mautoctrl = matobj->mautoctrlControls;
  for (i = 0; i < material->numControls; i++,mcontrol++,mautoctrl++) {
    *mautoctrl = *mautoctrl ? (mcontrol->length==4 ?
      MaterialAutoControl_runVector4(mautoctrl,matobject,val) :
      MaterialAutoControl_runMatrix(mautoctrl,matobject,val,LUX_TRUE)
      ): NULL;
    memcpy(mcontrol->sourcep,val,sizeof(float)*mcontrol->length);
    val+=mcontrol->length;
  }

  if (material->noTexgenOrMat)
    return;

  stages = material->stages;
  mautoctrl = matobj->mautoctrlStages;
  for (i = 0; i < material->numStages; i++,stages++,mautoctrl++){
    texcoord = (*stages)->texcoord;
    val = (*stages)->texgen;

    if(texcoord){
      if (texcoord->usemanual){
        lxMatrix44CopySIMD(texcoord->matrix,texcoord->usermatrix);
      }
      else{
        lxMatrix44IdentitySIMD(temp);
        lxMatrix44SetScale(temp,texcoord->scale);

        lxMatrix44IdentitySIMD(texcoord->matrix);
        lxMatrix44FromEulerZYXdeg(texcoord->matrix,texcoord->rotate);
        lxMatrix44Multiply1SIMD(texcoord->matrix,temp);
        lxMatrix44SetTranslation(texcoord->matrix,texcoord->move);

        if (texcoord->center[0] || texcoord->center[1] || texcoord->center[2]){
          lxMatrix44IdentitySIMD(temp);
          lxMatrix44SetTranslation(temp,texcoord->center);
          lxMatrix44Multiply2SIMD(temp,texcoord->matrix);

          lxMatrix44IdentitySIMD(temp);
          lxMatrix44SetInvTranslation(temp,texcoord->center);
          lxMatrix44Multiply1SIMD(texcoord->matrix,temp);
        }

      }
      *mautoctrl = *mautoctrl ? MaterialAutoControl_runMatrix(mautoctrl,matobject,texcoord->matrix,LUX_FALSE) : NULL;
    }
    mautoctrl++;
    if (val)
      *mautoctrl = *mautoctrl ? MaterialAutoControl_runMatrix(mautoctrl,matobject,val,LUX_TRUE) : NULL;

  }
}

void Material_setVID(Material_t *mat,const int shader){
  int i;
  MaterialStage_t *stage;
  MaterialParam_t *param;
  MaterialShader_t* mshader;

  mshader = &mat->shaders[shader];

  //g_VID.shdsetup.alpha = mshader->alpha;

  if (mshader->numShaderTotalParams)
    memset(g_VID.shdsetup.shaderparams,0,sizeof(float*)*mshader->numShaderTotalParams);

  if (param = mshader->params){
    for (i = 0; i < mshader->numParams; i++,param++){
      g_VID.shdsetup.shaderparams[param->totalid] = param->vecP;
    }
  }


  for (i = 0; i < mat->numStages; i++){
    stage = mat->stages[i];
    if (stage->id >= MATERIAL_UNUSED)
      continue;

    g_VID.shdsetup.texmatrixStage[stage->id] = stage->pMatrix;
    g_VID.shdsetup.colors[stage->id] = stage->pColor;
    g_VID.shdsetup.textures[stage->id] = stage->texRIDUse;
    g_VID.shdsetup.texgenStage[stage->id] = stage->texgen;


    if (stage->windowsized){
      if(stage->pMatrix){
        lxMatrix44Multiply2SIMD(g_VID.drawsetup.texwindowMatrix,stage->pMatrix);
      }
      else{
        g_VID.shdsetup.texmatrixStage[stage->id] = g_VID.drawsetup.texwindowMatrix;
      }
    }

  }
}

int  Material_getStageRIDTexture(Material_t *material,int stageid){
  int i;
  for (i = 0; i < material->numStages; i++)
    if (material->stages[i]->id == stageid)
      return material->stages[i]->texRID;
  return -1;
}

void  Material_clear(Material_t *material)
{
  int i,n;
  MaterialStage_t *matstage;

  MaterialObject_unrefAutoControls(&material->matobjdefault);

  for (i=0; i < MATERIAL_MAX_SHADERS; i++){
    if (material->shaders[i].name)
      ResData_unrefResource(RESOURCE_SHADER,material->shaders[i].resRID,material);
  }


  for (i = 0; i < material->numStages; i++) {
    matstage = material->stages[i];
    if (matstage->seq){
      for (n=0; n < matstage->seq->numFrames; n++) {
        ResData_unrefResource(RESOURCE_TEXTURE,matstage->seq->texRID[n],material);
      }
    }
    else{
      ResData_unrefResource(RESOURCE_TEXTURE,matstage->texRID,material);
    }
  }
}

int Material_setFromString(Material_t *mat, char *command)
{
  char *names[16] = {0};
  int numnames;
  int sid;
  int numtex;
  int i;
  MaterialStage_t *stage;
  static char *specialtex[5] = {"TEX ","TEXALPHA ", "TEXPROJ ", "TEXDOTZ ", "TEXCUBE "};
  static int specialtexType[5] = {TEX_COLOR,
    TEX_ALPHA, TEX_COLOR, TEX_COLOR, TEX_COLOR};
  static int specialtexAttr[5] = {TEX_ATTR_MIPMAP,
    TEX_ATTR_MIPMAP, TEX_ATTR_MIPMAP | TEX_ATTR_PROJ, TEX_ATTR_MIPMAP | TEX_ATTR_PROJ, TEX_ATTR_MIPMAP | TEX_ATTR_CUBE};


  lprintf("Material: \t%s\n",command);

  numnames = lxStrSplitList(command+MATERIAL_AUTOSTART_LEN,names,'|',16);
  // first is shader rest is textures
  mat->numMods = 0;
  mat->numStages = 0;
  mat->numControls = 0;
  mat->matobj = &mat->matobjdefault;

  for (sid = 0; sid < MATERIAL_MAX_SHADERS; sid++){
    mat->shaders[sid].rfalpha = -2.0f;
    mat->shaders[sid].resRID = -1;
  }

  sid = 0;
  numtex = 0;
  for (i = 0; i < numnames; i++){
    if (strstr(names[i],".shd") || strstr(names[i],".SHD")){
      char *end;
      resnewstrcpy(mat->shaders[i].name,names[i]);
      if (end=strrchr(mat->shaders[i].name,'^')){
        mat->shaders[i].extname = end+1;
        *end = 0;
      }

      names[i][0] = 0;
      sid++;
    }
    else{
      numtex++;
    }
  }

  mat->numStages = numtex;
  mat->stages = reszalloc(sizeof(MaterialStage_t*)*numtex);
  numtex = 0;
  for (i = 0; i < numnames; i++){
    if (names[i][0]){
      char *texname = names[i];
      char *special[5] = {0,0,0,0,0};
      int whichspecial = -1;

      stage = mat->stages[numtex] = reszalloc(sizeof(MaterialStage_t));
      stage->color[3] = -1;
      stage->id = numtex + MATERIAL_TEX0;

      while (++whichspecial < 5 && !(special[whichspecial]=strstr(names[i], specialtex[whichspecial])));


      if (whichspecial < 5){
        stage->texType = specialtexType[whichspecial];
        stage->texAttributes = specialtexAttr[whichspecial];
        texname += strlen(specialtex[whichspecial]);
      }
      else{
        stage->texType = TEX_COLOR;
        stage->texAttributes = TEX_ATTR_MIPMAP;
      }
      resnewstrcpy(stage->texName,texname);
      stage->texRID = -1;

      numtex++;
    }
  }


  return sid > 0;
}

int  Material_getTextureStageIndex(Material_t *material, int texid)
{
  MaterialStage_t **mstage = material->stages;
  int i;

  for (i = 0; i < material->numStages; i++,mstage++){
    if ((*mstage)->texRID == texid)
      return i;
  }

  return -1;
}

void Material_initMatObject(Material_t *material){
  MaterialObject_t *matobj = &material->matobjdefault;

  matobj->numStages = material->numStages;
  matobj->numControls = material->numControls;
  if (matobj->numStages)
    matobj->mautoctrlStages = reszalloc(sizeof(MaterialAutoControl_t *)*matobj->numStages*2);
  if (matobj->numControls)
    matobj->mautoctrlControls = reszalloc(sizeof(MaterialAutoControl_t *)*matobj->numControls);

}

booln Material_initShaders(Material_t *material)
{
  MaterialParam_t *param;
  MaterialShader_t *matshader;
  Shader_t *shader;
  booln ret = LUX_TRUE;
  int i,n;

  matshader = material->shaders;
  for (i = 0 ; i < MATERIAL_MAX_SHADERS; i++,matshader++){
    if (matshader->resRID < 0)
      continue;

    param = matshader->params;
    shader = ResData_getShader(matshader->resRID);
    matshader->numShaderTotalParams = shader->numTotalParams;
    for (n = 0; n < matshader->numParams; n++,param++){
      param->totalid = Shader_searchParam(shader,param->name,param->pass);
      if (param->totalid < 0){
        ret = LUX_FALSE;
        lprintf("ERROR resadd %s: cannot link shaderparameter %s|%d in %s\n",
          g_ResDescriptor[RESOURCE_MATERIAL].name,param->name,param->pass,material->resinfo.name);
      }
    }
  }

  return ret;
}
booln Material_checkShaderLink(Material_t *material, int ctrlid, struct Shader_s *shader)
{
  MaterialResControl_t  *ctrl = &material->rescontrols[ctrlid];
  MaterialShader_t *matshader;
  MaterialParam_t *param;
  int i,n;

  matshader = material->shaders;
  for (i = 0 ; i < MATERIAL_MAX_SHADERS; i++,matshader++){
    if (matshader->resRID < 0)
      continue;

    param = matshader->params;
    shader = ResData_getShader(matshader->resRID);
    for (n = 0; n < matshader->numParams; n++,param++){
      if (param->totalid != Shader_searchParam(shader,param->name,param->pass)){
        return LUX_FALSE;
      }
    }
  }

  return LUX_TRUE;
}

enum32 Material_getDefaults(Material_t *mat, struct Shader_s **oshader, VIDAlpha_t *alpha, VIDBlend_t *blend, VIDLine_t *line)
{
  Shader_t *shader = ResData_getShader(mat->shaders[0].resRID);
  LUX_ASSERT(shader);

  if (alpha && shader->alpha){
    alpha->alphafunc = shader->alpha->alphafunc;
    alpha->alphaval = mat->shaders[0].rfalpha < 0.0f ? shader->alpha->alphaval : mat->shaders[0].rfalpha;
  }
  //if (blend && shader->blend){
  //  blend->alphamode = shader->blend->alphamode;
  //  blend->blendmode = shader->blend->blendmode;
  //  blend->blendinvert = shader->blend->blendinvert;
  //}

  //if (line && shader->line){
  //
  //}

  *oshader = shader;
  return shader->renderflag;
}

//////////////////////////////////////////////////////////////////////////

int MaterialControl_getOffsetsFromID(int ctrlid,int inmatRID, int* offset, int *vlength){
  int matRID = SUBRESID_GETRES(ctrlid);
  Material_t *mat = ResData_getMaterial(matRID);
  if (!mat || inmatRID != matRID) return LUX_FALSE;

  SUBRESID_MAKEPURE(ctrlid);

  *offset = mat->controls[ctrlid].offset;
  *vlength = mat->controls[ctrlid].length;

  return LUX_TRUE;

}

///////////////////////////////////////////////////////////////////////////////
// MaterialTexCoord

void MaterialTexCoord_clear(MaterialTexCoord_t *texcoord){
  memset(texcoord,0,sizeof(MaterialTexCoord_t));
  lxVector3Set(texcoord->scale,1,1,1);
  lxMatrix44Identity(texcoord->matrix);
}

