// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/3dmath.h"
#include "../render/gl_print.h"
#include "../render/gl_draw3d.h"
#include "../render/gl_drawmesh.h"
#include "../render/gl_window.h"
#include "../render/gl_list3d.h"
#include "../scene/vistest.h"

// LOCALS
extern DrawData_t l_DrawData;
extern DrawMeshFunc_t *l_DrawMeshFuncs;

DrawBatch_t   l_batchdata;
DrawLoop_t    l_loopdata;

static int Draw_shadowedMesh(Mesh_t *mesh, int zfail);


//////////////////////////////////////////////////////////////////////////
// DrawNode

void  DrawNode_updateSortID(DrawNode_t *dnode, int sorttype)
{
  Shader_t *shader;

  // shader
  if (vidMaterial(dnode->draw.matRID)){
    shader = ResData_getMaterialShader(dnode->draw.matRID);
    dnode->sortID = ((shader->resinfo.resRID+2)<<DRAW_SORT_SHIFT_SHADER) + ((dnode->draw.matRID-VID_OFFSET_MATERIAL)<<DRAW_SORT_SHIFT_MATERIAL);
  }// texture
  else if (dnode->draw.matRID >= 0){
    dnode->sortID = (1<<DRAW_SORT_SHIFT_SHADER) + ((dnode->draw.matRID)<<DRAW_SORT_SHIFT_MATERIAL);
  }// color
  else{
    dnode->sortID = 0;
  }

  if (dnode->skinobj){
    //sorttype = DRAW_SORT_TYPE_SKINNED;
    dnode->sortID |= DRAW_SORT_FLAG_SKINNED;
  }

  //dnode->sortRID += sorttype<<DRAW_SORT_SHIFT_TYPE;
}



//////////////////////////////////////////////////////////////////////////
// DrawNodes


#define _dnode_bonematrix()       if (dnode->bonematrix){ lxMatrix44MultiplySIMD(g_VID.drawsetup.worldMatrixCache,g_VID.drawsetup.worldMatrix,dnode->bonematrix); vidWorldMatrix(g_VID.drawsetup.worldMatrixCache);  }
#define _dnode_bonematrix_worldset()  if (dnode->bonematrix){ vidWorldMatrix(dnode->bonematrix);} else vidWorldMatrixIdentity()

#define _dnode_overridevertices() if(dnode->overrideVertices) dnode->draw.mesh->vertexData = dnode->overrideVertices

#define _dnode_renderscale()  vidWorldScale(dnode->renderscale)
#define _dnode_copyworldmat() vidWorldMatrix(dnode->matrix)

static LUX_INLINE _dnode_vidstate(const DrawNode_t* dnode, flags32 renderflag)
{
  if (dnode->draw.state.line.linewidth > 0)
    VIDLine_setGL(&dnode->draw.state.line);
  if (dnode->draw.state.alpha.alphafunc)
    vidAlphaFunc(dnode->draw.state.alpha.alphafunc,dnode->draw.state.alpha.alphaval);
  if (dnode->draw.state.blend.blendmode)
    vidBlend(dnode->draw.state.blend.blendmode,dnode->draw.state.blend.blendinvert);
}

static LUX_INLINE _dnode_shading(const DrawNode_t* dnode, flags32 renderflag)
{
  if( renderflag & RENDER_NOVERTEXCOLOR)
    glColor4fv(dnode->draw.color);
  if (vidMaterial(dnode->draw.matRID))
    Material_update(ResData_getMaterial(dnode->draw.matRID),dnode->draw.matobj);
  if (dnode->draw.matobj)
    vidTexMatrixSet(dnode->draw.matobj->texmatrix);
  else
    vidTexMatrixSet(NULL);
  _dnode_vidstate(dnode,renderflag);
}

static LUX_INLINE _dnode_lighting(const DrawNode_t* dnode, flags32 renderflag)
{
  l_DrawData.numFxLights=dnode->env->numFxLights;
  l_DrawData.fxLights = (void**) dnode->env->fxLights;
  g_VID.shdsetup.lightmapRID = (renderflag & RENDER_NOLIGHTMAP) ? -1 : dnode->env->lightmapRID;
  vidLMTexMatrixSet(dnode->env->lmtexmatrix);
}


#define _dnode_DRAW()   \
  type = DRAW_MESH_BASIC;   \
  type += (dnode->draw.matRID >= 0)     * DRAW_MESH_SHIFT_TYPE; \
  type += (vidMaterial(dnode->draw.matRID)) * DRAW_MESH_SHIFT_TYPE; \
  type += (dnode->skinobj != NULL)    * DRAW_MESH_SHIFT_SKIN; \
  type += (dnode->env->numProjectors && !ignoreprojs) * DRAW_MESH_SHIFT_PROJ; \
  l_DrawMeshFuncs[type].func(dnode->draw.mesh,dnode->draw.matRID,dnode->env->projectors,dnode->env->numProjectors,dnode->skinobj)

#define _dnode_DRAW_noskin()    \
  type = DRAW_MESH_BASIC;   \
  type += (dnode->draw.matRID >= 0)     * DRAW_MESH_SHIFT_TYPE; \
  type += (vidMaterial(dnode->draw.matRID)) * DRAW_MESH_SHIFT_TYPE; \
  type += (dnode->env->numProjectors && !ignoreprojs) * DRAW_MESH_SHIFT_PROJ; \
  l_DrawMeshFuncs[type].func(dnode->draw.mesh,dnode->draw.matRID,dnode->env->projectors,dnode->env->numProjectors,NULL)

#define _dnode_pushMatricesGL() Draw_pushMatricesGL()

//////////////////////////////////////////////////////////////////////////
// DrawBatch

static void DrawNode_addToBatch(const DrawNode_t *dnode)
{
  Mesh_t *mesh;
  ushort *ind;
  int i;
  int *grplength;
  int leng;

  l_batchdata.dnode = dnode;
  mesh = dnode->draw.mesh;

  // strips need degenerated indices
  if(mesh->primtype == PRIM_TRIANGLE_STRIP && l_batchdata.curind != l_batchdata.batchmesh.indicesData16){
    // repeat last and add first new
    *l_batchdata.curind = *(l_batchdata.curind-1);
    l_batchdata.curind++;
    *l_batchdata.curind = *mesh->indicesData16;
    l_batchdata.curind++;
  }
  // gotta fill the groups
  if (mesh->numGroups > 1 && mesh->primtype == PRIM_TRIANGLE_STRIP){
    leng = 0;
    // all cept last need to make sure they degenerate strips
    grplength = mesh->indicesGroupLength;
    for (i = 0; i < mesh->numGroups-1; i++,grplength++){
      ind = &mesh->indicesData16[leng];
      leng+= *grplength;
      memcpy(l_batchdata.curind,ind,sizeof(ushort)*(*grplength));
      l_batchdata.curind += *grplength;

      // for next strip
      *l_batchdata.curind = *(l_batchdata.curind-1);
      l_batchdata.curind++;
      *l_batchdata.curind = *ind;
      l_batchdata.curind++;
    }
    memcpy(l_batchdata.curind,ind,sizeof(ushort)*(*grplength));
    l_batchdata.curind += *grplength;
  }
  else{
    // copy over all
    memcpy(l_batchdata.curind,mesh->indicesData16,sizeof(ushort)*mesh->numIndices);
    l_batchdata.curind += mesh->numIndices;
  }

  l_batchdata.batchmesh.numVertices += mesh->numVertices;
  l_batchdata.batchmesh.numTris += mesh->numTris;
  l_batchdata.batchmesh.indexMax = LUX_MAX(mesh->indexMax,l_batchdata.batchmesh.indexMax);
  l_batchdata.batchmesh.indexMin = LUX_MIN(mesh->indexMin,l_batchdata.batchmesh.indexMin);

}

static int DrawNode_rsurfCheck(const DrawNode_t *a, const DrawNode_t *b)
{
  if ((a->draw.state.renderflag & RENDER_BLEND &&
    ( a->draw.state.blend.blendmode != b->draw.state.blend.blendmode ||
      a->draw.state.blend.blendinvert != b->draw.state.blend.blendinvert))
    ||
    (a->draw.state.renderflag & RENDER_ALPHATEST  &&
    ( a->draw.state.alpha.alphafunc != b->draw.state.alpha.alphafunc ||
      a->draw.state.alpha.alphaval  != b->draw.state.alpha.alphaval))
    ||
    (a->draw.state.renderflag & RENDER_WIRE &&
    ( a->draw.state.line.linewidth  !=  b->draw.state.line.linewidth ||
      a->draw.state.line.linestipple  !=  b->draw.state.line.linestipple ||
      a->draw.state.line.linefactor !=  b->draw.state.line.linefactor)))
      return 0;

  return 1;
}

#define _dnode_isequal(check) (lastnode->check == dnode->check)
static int DrawNode_finishBatch();
// return number of passes (0 or 1)
static void DrawNode_checkBatch(const DrawNode_t *dnode,const enum32 renderflag)
{
  const DrawNode_t  *lastnode = l_batchdata.dnode;
  // if no dnode set we can just start new batch
  // else we gotta check if its compatible to last dnode
  if (!lastnode ||
      (
      _dnode_isequal(draw.matRID) &&
      _dnode_isequal(draw.state.renderflag) &&
      _dnode_isequal(batchinfo.batchcontainer)  &&
      _dnode_isequal(draw.mesh->primtype) &&
      _dnode_isequal(env->lightmapRID) &&
      (l_batchdata.curind+dnode->draw.mesh->numIndices) < (l_batchdata.maxind) &&
      DrawNode_rsurfCheck(lastnode,dnode)
      )
    ){
    DrawNode_addToBatch(dnode);
    l_batchdata.renderflag = renderflag;
    LUX_PROFILING_OP (g_Profiling.perframe.draw.TBatches++);
    LUX_PROFILING_OP (g_Profiling.perframe.draw.TMeshes--);
    l_DrawData.numPasses = 0;
    return;
  }
  // not compatible ? finish last batch
  DrawNode_finishBatch();
  // and start new
  DrawNode_addToBatch(dnode);
  l_batchdata.renderflag = renderflag;

}


static int DrawNode_finishBatch()
{
  const DrawNode_t  *dnode;
  DrawMeshType_t  type;
  enum32 renderflag;

  dnode = l_batchdata.dnode;
  renderflag = l_batchdata.renderflag;

  // setup batch mesh
  l_batchdata.batchmesh.numIndices = l_batchdata.curind-l_batchdata.batchmesh.indicesData16;
  l_batchdata.batchmesh.origVertexData = l_batchdata.batchmesh.vertexData = dnode->batchinfo.batchcontainer->vertexData;
  l_batchdata.batchmesh.numVertices = dnode->batchinfo.batchcontainer->numVertices;
  l_batchdata.batchmesh.meshtype = dnode->draw.mesh->meshtype;
  l_batchdata.batchmesh.vid.vbo = dnode->draw.mesh->vid.vbo;
  l_batchdata.batchmesh.primtype = dnode->draw.mesh->primtype;
  l_batchdata.batchmesh.vertextype = dnode->draw.mesh->vertextype;


  vidWorldMatrixIdentity();
  _dnode_lighting(dnode,renderflag);
  VIDRenderFlag_setGL(l_batchdata.renderflag);
  vidTexMatrixSet(NULL);
  _dnode_vidstate(dnode,renderflag);

  type = DRAW_MESH_BASIC;
  if (dnode->draw.matRID >= 0)
    type += DRAW_MESH_SHIFT_TYPE;
  if (vidMaterial(dnode->draw.matRID))
    type += DRAW_MESH_SHIFT_TYPE;

  //if (l_batchdata.batchmesh.vid.ibo){
  //  vidBindBufferIndex(l_batchdata.batchmesh.vid.ibo);
  //  glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, (sizeof(ushort)*l_batchdata.batchmesh.numIndices), l_batchdata.batchmesh.indicesData16, GL_STREAM_DRAW_ARB);
  //}

  l_DrawMeshFuncs[type].func(&l_batchdata.batchmesh,dnode->draw.matRID,NULL,0,NULL);

  l_batchdata.dnode = NULL;
  l_batchdata.curind = l_batchdata.batchmesh.indicesData16;
  l_batchdata.batchmesh.indexMin = BIT_ID_FULL16;
  l_batchdata.batchmesh.indexMax = 0;

  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTVertices += l_batchdata.batchmesh.numVertices*l_DrawData.numPasses);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTTris += l_batchdata.batchmesh.numTris*l_DrawData.numPasses);

  l_batchdata.batchmesh.numVertices = 0;
  l_batchdata.batchmesh.numTris = 0;
  return l_DrawData.numPasses;
}

//////////////////////////////////////////////////////////////////////////
// List

static void _dnode_DrawTextObject(DrawNode_t* dnode, enum32 renderflag, booln ignoreprojs)
{
  _dnode_lighting(dnode,renderflag);
  _dnode_copyworldmat();
  _dnode_bonematrix();
  _dnode_renderscale();

  VIDRenderFlag_setGL(renderflag);
  vidColor4fv(dnode->draw.color);
  _dnode_vidstate(dnode,renderflag);

  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);

  _dnode_pushMatricesGL();
  Draw_PText(0,0,0,NULL,dnode->draw.ptext,NULL);
  glPopMatrix();
  l_DrawData.numPasses = 1;
}

static void _dnode_DrawFunc(DrawNode_t* dnode, enum32 renderflag, booln ignoreprojs)
{
  l_DrawData.numPasses = dnode->drawfunc(dnode,dnode->drawupvalue,renderflag);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.TVertices += dnode->draw.mesh->numVertices);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.TTris += dnode->draw.mesh->numTris);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTVertices += dnode->draw.mesh->numVertices*l_DrawData.numPasses);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTTris += dnode->draw.mesh->numTris*l_DrawData.numPasses);
}


static void _dnode_DrawShadowObject(DrawNode_t* dnode, enum32 renderflag, booln ignoreprojs)
{
  _dnode_copyworldmat();
  _dnode_bonematrix();
  _dnode_renderscale();
  _dnode_pushMatricesGL();
  l_DrawData.numPasses = Draw_shadowedMesh(dnode->draw.mesh,dnode->draw.matRID);
  glPopMatrix();
}

static void _dnode_DrawWorld(DrawNode_t* dnode, enum32 renderflag, booln ignoreprojs)
{
  DrawMeshType_t type;
  _dnode_lighting(dnode,renderflag);
  _dnode_bonematrix_worldset();
  vidWorldScale(NULL);

  VIDRenderFlag_setGL(renderflag);
  _dnode_shading(dnode,renderflag);

  _dnode_DRAW();

  LUX_PROFILING_OP (g_Profiling.perframe.draw.TVertices += dnode->draw.mesh->numVertices);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.TTris += dnode->draw.mesh->numTris);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTVertices += dnode->draw.mesh->numVertices*l_DrawData.numPasses);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTTris += dnode->draw.mesh->numTris*l_DrawData.numPasses);
}

static void _dnode_DrawWorldSkinned(DrawNode_t* dnode, enum32 renderflag, booln ignoreprojs)
{
  DrawMeshType_t type;
  vidWorldMatrixIdentity();
  vidWorldScale(NULL);
  _dnode_lighting(dnode,renderflag);

  VIDRenderFlag_setGL(renderflag);
  _dnode_shading(dnode,renderflag);

  _dnode_overridevertices();
  _dnode_DRAW();

  LUX_PROFILING_OP (g_Profiling.perframe.draw.TVertices += dnode->draw.mesh->numVertices);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.TTris += dnode->draw.mesh->numTris);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTVertices += dnode->draw.mesh->numVertices*l_DrawData.numPasses);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTTris += dnode->draw.mesh->numTris*l_DrawData.numPasses);
}

static void _dnode_DrawObject(DrawNode_t* dnode, enum32 renderflag, booln ignoreprojs)
{
  DrawMeshType_t type;
  _dnode_lighting(dnode,renderflag);
  _dnode_copyworldmat();
  _dnode_bonematrix();
  _dnode_renderscale();

  VIDRenderFlag_setGL(renderflag);
  _dnode_shading(dnode,renderflag);

  _dnode_overridevertices();
  _dnode_DRAW();

  LUX_PROFILING_OP (g_Profiling.perframe.draw.TVertices += dnode->draw.mesh->numVertices);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.TTris += dnode->draw.mesh->numTris);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTVertices += dnode->draw.mesh->numVertices*l_DrawData.numPasses);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTTris += dnode->draw.mesh->numTris*l_DrawData.numPasses);
}

static void _dnode_DrawObjectSkinned(DrawNode_t* dnode, enum32 renderflag, booln ignoreprojs)
{
  DrawMeshType_t type;
  _dnode_lighting(dnode,renderflag);
  _dnode_copyworldmat();
  _dnode_renderscale();

  VIDRenderFlag_setGL(renderflag);
  _dnode_shading(dnode,renderflag);

  _dnode_overridevertices();
  _dnode_DRAW();

  LUX_PROFILING_OP (g_Profiling.perframe.draw.TVertices += dnode->draw.mesh->numVertices);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.TTris += dnode->draw.mesh->numTris);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTVertices += dnode->draw.mesh->numVertices*l_DrawData.numPasses);
  LUX_PROFILING_OP (g_Profiling.perframe.draw.MTTris += dnode->draw.mesh->numTris*l_DrawData.numPasses);
}

static void _dnode_DrawWorldBatched(DrawNode_t* dnode, enum32 renderflag, booln ignoreprojs)
{
  DrawMeshType_t type;
  // material objects or fxlights/projectors spoil the batching
  if (dnode->draw.mesh->numTris > g_VID.gensetup.batchMeshMaxTris || dnode->draw.matobj || ((renderflag & RENDER_FXLIT) && dnode->env->numFxLights)
    ||(dnode->env->numProjectors && !ignoreprojs)){
      _dnode_lighting(dnode,renderflag);

      VIDRenderFlag_setGL(renderflag);
      _dnode_shading(dnode,renderflag);

      _dnode_DRAW_noskin();

      LUX_PROFILING_OP (g_Profiling.perframe.draw.TVertices += dnode->draw.mesh->numVertices);
      LUX_PROFILING_OP (g_Profiling.perframe.draw.TTris += dnode->draw.mesh->numTris);
      LUX_PROFILING_OP (g_Profiling.perframe.draw.MTVertices += dnode->draw.mesh->numVertices*l_DrawData.numPasses);
      LUX_PROFILING_OP (g_Profiling.perframe.draw.MTTris += dnode->draw.mesh->numTris*l_DrawData.numPasses);
  }
  else
    DrawNode_checkBatch(dnode,renderflag);
}

#undef _dnode_bonematrix
#undef _dnode_copyworldmat
#undef _dnode_renderscale


void  DrawNodes_drawList(const DrawSortNode_t *list, const int listlength, const int ignoreprojs, const enum32 ignoreflags, const enum32 forceflags)
{
  int i;
  const DrawSortNode_t  *browse = list;
  DrawNode_t    *dnode;
  enum32 renderflag;

  for (i = 0; i < listlength; i++,browse++){

    g_VID.drawsetup.curnode = dnode = l_DrawData.dnode = (DrawNode_t*)browse->pType;
    renderflag = (dnode->draw.state.renderflag & ignoreflags) | forceflags;

    LUX_DEBUGASSERT(dnode->draw.mesh || (dnode->type==DRAW_NODE_FUNC && dnode->drawfunc));
    switch(dnode->type) {
    case DRAW_NODE_OBJECT:
      _dnode_DrawObject(dnode,renderflag,ignoreprojs);
      break;
    case DRAW_NODE_SKINNED_OBJECT:
      _dnode_DrawObjectSkinned(dnode,renderflag,ignoreprojs);
      break;
    case DRAW_NODE_WORLD:
      _dnode_DrawWorld(dnode,renderflag,ignoreprojs);
      break;
    case DRAW_NODE_SKINNED_WORLD:
      _dnode_DrawWorldSkinned(dnode,renderflag,ignoreprojs);
      break;
    case DRAW_NODE_BATCH_WORLD:
      _dnode_DrawWorldBatched(dnode,renderflag,ignoreprojs);
      break;
    case DRAW_NODE_TEXT_OBJECT:
      _dnode_DrawTextObject(dnode,renderflag,ignoreprojs);
      break;
    case DRAW_NODE_SHADOW_OBJECT:
      _dnode_DrawShadowObject(dnode,renderflag,ignoreprojs);
      break;
    case DRAW_NODE_FUNC:
      _dnode_DrawFunc(dnode,renderflag,ignoreprojs);
      break;


    default:
      LUX_ASSERT(0);
    }

    LUX_PROFILING_OP (g_Profiling.perframe.draw.TMeshes++);
    LUX_PROFILING_OP (g_Profiling.perframe.draw.TDraws += l_DrawData.numPasses);
  }

  if (l_batchdata.dnode){
    l_DrawData.numPasses = DrawNode_finishBatch();
    LUX_PROFILING_OP (g_Profiling.perframe.draw.TDraws += l_DrawData.numPasses);
    LUX_PROFILING_OP (g_Profiling.perframe.draw.TMeshes++);
  }

  vidWorldScale(NULL);
}



//////////////////////////////////////////////////////////////////////////
// Draw_shadowMesh


static int Draw_shadowedMesh(Mesh_t *mesh, int zfail)
{
  VIDStencil_t *stencil;
  ushort    rflags;
  int i;

  if(zfail)
    stencil = &g_VID.gensetup.zfail;
  else
    stencil = &g_VID.gensetup.zpass;


  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);

  vidClearTexStages(0,i);

  rflags = RENDER_NOVERTEXCOLOR |  RENDER_STENCILMASK | RENDER_STENCILTEST | RENDER_NODEPTHMASK;
  if (g_Draws.drawShadowVolumes){
    rflags |= RENDER_BLEND;
    glColor3f(0.3,0.3,0.1);
    vidBlend(VID_ADD,LUX_FALSE);
  }
  else
    rflags |= RENDER_NOCOLORMASK;

  if (g_VID.capTwoSidedStencil){
    rflags |= RENDER_NOCULL ;
    VIDStencil_setGL(stencil,-1);
    VIDRenderFlag_setGL(rflags);
    Draw_Mesh_simple(mesh);
    return 1;
  }
  else{
    rflags |= RENDER_FRONTCULL ;
    VIDStencil_setGL(stencil,1);
    VIDRenderFlag_setGL(rflags);
    Draw_Mesh_simple(mesh);
    rflags &= ~RENDER_FRONTCULL;
    VIDStencil_setGL(stencil,0);
    VIDRenderFlag_setGL(rflags);
    Draw_Mesh_simple(mesh);
    return 2;
  }

}


//////////////////////////////////////////////////////////////////////////
// HELPERS

void Draw_Model_normals(const struct Model_s *model)
{
  static lxMatrix44SIMD matrix;

  MeshObject_t  *meshobj;
  Mesh_t      *mesh;
  int transform;
  int i,j;
  lxVector3 v;
  lxVector3 n;
  lxVector4 t;
  lxVector3 b;
  lxVertex64_t  *vert64;
  lxVertex32_t  *vert32;
  RenderHelpers_t *dh = &g_Draws;

  if(model->vertextype == VERTEX_32_TEX2)
    return;

  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);
  vidClearTexStages(0,i);

  VIDRenderFlag_setGL(0);

  if (dh->normalLen == 0)
    dh->normalLen=1;

  glLineWidth(1);
  glDisable(GL_LINE_STIPPLE);

  for(j=0; j<model->numMeshObjects; j++){
    meshobj = &model->meshObjects[j];
    mesh = meshobj->mesh;

    if (mesh->instanceType == INST_SHRD_VERT_PARENT_MAT)
      continue;

    transform = LUX_FALSE;
    if (meshobj->bone){
      lxMatrix44CopySIMD(matrix,&model->bonesys.absMatrices[meshobj->bone->ID]);
      transform = LUX_TRUE;
    }

    switch(mesh->vertextype)
    {
    case VERTEX_32_NRM:
      vert32 = mesh->vertexData32;
      for(i=0; i<mesh->numVertices; i++,vert32++)
      {
        if (transform){
          lxVector3Transform(v,vert32->pos,matrix);
          lxVector3float_FROM_short(n,vert32->normal);
          lxVector3TransformRot1(n,matrix);
        }
        else{
          lxVector3Copy(v,vert32->pos);
          lxVector3float_FROM_short(n,vert32->normal);
        }

        lxVector3ScaledAdd(n,v,dh->normalLen,n);

        glColor3f(1, 0, 0);

        glBegin(GL_LINES);
        glVertex3fv(v);
        glVertex3fv(n);
        glEnd();
      }
      break;
    case VERTEX_64_TEX4:
    case VERTEX_64_SKIN:
      vert64 = mesh->vertexData64;
      for(i=0; i<mesh->numVertices; i++,vert64++)
      {
        if (transform){
          lxVector3Transform(v,vert64->pos,matrix);
          lxVector3float_FROM_short(n,vert64->normal);
          lxVector3TransformRot1(n,matrix);
          lxVector4float_FROM_short(t,vert64->tangent);
          lxVector3TransformRot1(t,matrix);
        }
        else{
          lxVector3Copy(v,vert64->pos);
          lxVector3float_FROM_short(n,vert64->normal);
          lxVector4float_FROM_short(t,vert64->tangent);
        }

        lxVector3Cross(b,n,t);
        lxVector3Scale(b,b,t[3]);

        lxVector3ScaledAdd(n,v,dh->normalLen,n);
        lxVector3ScaledAdd(t,v,dh->normalLen,t);
        lxVector3ScaledAdd(b,v,dh->normalLen,b);

        glColor3f(1, 0, 0);

        glBegin(GL_LINES);
        glVertex3fv(v);
        glVertex3fv(n);
        glEnd();

        glColor3f(0, 0, 1);

        glBegin(GL_LINES);
        glVertex3fv(v);
        glVertex3fv(t);
        glEnd();

        glColor3f(0, 1, 0);
        glBegin(GL_LINES);
        glVertex3fv(v);
        glVertex3fv(b);
        glEnd();
      }
      break;
    default:
      break;
    }


  }
}
#define DRAW_BONELIMITS_ITER 5

static void Draw_Bone_limits(Bone_t *bone, int axis, const lxMatrix44 absmatix, const lxVector3 base, const lxVector3 angles, const lxVector3 parentangles)
{
  int i;
  float lrp = 0;
  float angle;
  float lrpadd = 1/(float)(DRAW_BONELIMITS_ITER-1);
  const float *dir;
  lxVector3 vec;
  lxQuat qrot;
  const float *rotaxis;

  switch(axis)
  {
  case 0:
    dir = absmatix+4;
    rotaxis = absmatix;
    break;
  case 1:
    dir = absmatix+8;
    rotaxis = absmatix+4;
    break;
  case 2:
    dir = absmatix;
    rotaxis = absmatix+8;
    break;
  default:
    break;
  }

  glBegin(GL_TRIANGLE_FAN);
  glVertex3fv(base);
  for(i = 0; i < DRAW_BONELIMITS_ITER; i++,lrp+=lrpadd)
  {
    angle = LUX_LERP(lrp,bone->boneaxis->minAngleAxis[axis],bone->boneaxis->maxAngleAxis[axis]);
    angle+=parentangles[axis]-angles[axis];

    lxQuatFromRotationAxisFast(qrot,angle,rotaxis);
    lxQuatTransformVector(qrot,(float*)dir,vec);

    lxVector3Add(vec,vec,base);
    glVertex3fv(vec);
  }
  glEnd();

}

void Draw_Model_bones(const struct Model_s *model, const float *localmat,  lxVector3 renderscale)
{
  static uint stipplemask[]={
    0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,
    0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,
    0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,
    0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,0xCCCCCCCC,0x33333333,
  };
  int i;
  Bone_t *bone;
  const float *base;
  const float *basemat;
  const float *parentmat;
  lxVector3 x, y, z;
  lxVector4 screenpos;
  RenderHelpers_t *dh = &g_Draws;
  const float *absmatrices = model->bonesys.absMatrices;

  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);
  vidClearTexStages(0,i);

  vidNoDepthTest(LUX_TRUE);
  vidLighting(LUX_FALSE);
  glLineWidth(1);
  glDisable(GL_LINE_STIPPLE);


  if (dh->drawBonesLimits){
    glEnable(GL_POLYGON_STIPPLE);
    glPolygonStipple((char*)stipplemask);
    vidNoCullFace(LUX_TRUE);
  }

  for(i=0;i<model->bonesys.numBones;i++){
    bone = &model->bonesys.bones[i];
    basemat = &absmatrices[bone->ID];
    base = basemat+12;
    parentmat = NULL;
    if(bone->parentB != NULL)
    {
      parentmat = &absmatrices[bone->parentID];
      lxVector3Set(x,0,0,0);
      lxVector3Transform1(x,parentmat);

      glColor3f(0.7,0.7,0.7);
      glBegin(GL_LINES);
        glVertex3fv(base);
        glVertex3fv(x);
      glEnd();
    }

    lxVector3Set(x,dh->boneLen,0,0);
    lxVector3Transform1(x,basemat);
    lxVector3Set(y,0,dh->boneLen,0);
    lxVector3Transform1(y,basemat);
    lxVector3Set(z,0,0,dh->boneLen);
    lxVector3Transform1(z,basemat);

    glBegin(GL_LINES);
      glColor3f(1,0,0);
      glVertex3fv(base);
      glVertex3fv(x);
      glColor3f(0,1,0);
      glVertex3fv(base);
      glVertex3fv(y);
      glColor3f(0,0,1);
      glVertex3fv(base);
      glVertex3fv(z);
    glEnd();

    if (dh->drawBonesLimits && bone->boneaxis && bone->boneaxis->active){
      lxMatrix44ToEulerXYZ(basemat,x);
      if (parentmat)
        lxMatrix44ToEulerXYZ(parentmat,y);
      else
        lxVector3Set(y,0,0,0);
      if (bone->boneaxis->limitAxis[0]){
        glColor3f(1,0,0);
        Draw_Bone_limits(bone,0,basemat,base,x,y);
      }
      if (bone->boneaxis->limitAxis[1]){
        glColor3f(0,1,0);
        Draw_Bone_limits(bone,1,basemat,base,x,y);
      }
      if (bone->boneaxis->limitAxis[2]){
        glColor3f(0,0,1);
        Draw_Bone_limits(bone,2,basemat,base,x,y);
      }
    }
  }

  if (dh->drawBonesLimits){
    glDisable(GL_POLYGON_STIPPLE);
  }

  if (dh->drawBonesNames){
    PText_t ptext;

    PText_setDefaults(&ptext);
    PText_setWidth(&ptext,12.0f);
    PText_setSize(&ptext,SCREEN_SCALE*12.0f);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glFrontFace(GL_CW);
    glOrtho(0,g_VID.state.viewport.bounds.width, g_VID.state.viewport.bounds.height,0,-128,128);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    VIDRenderFlag_setGL(PText_getDefaultRenderFlag() | RENDER_NODEPTHMASK | RENDER_BLEND);
    vidBlend(VID_DECAL,LUX_FALSE);

    vidColor4f(1,1,1,1);
    for(i=0;i<model->bonesys.numBones;i++){
      bone = &model->bonesys.bones[i];
      basemat = &absmatrices[bone->ID];
      base = basemat+12;

      if (localmat)
        lxVector3Transform(screenpos,base,localmat);
      else
        lxVector3Copy(screenpos,base);

      lxVector3Mul(screenpos,screenpos,renderscale);

      screenpos[3] = 1.0f;

      lxVector4Transform1(screenpos,g_CamLight.camera->mviewproj);
      screenpos[0]=((screenpos[0]/screenpos[3])*0.5f + 0.5f)*g_VID.state.viewport.bounds.width;
      screenpos[1]=((-screenpos[1]/screenpos[3])*0.5f + 0.5f)*g_VID.state.viewport.bounds.height;
      screenpos[2]= 0.0f;
      screenpos[0] = floor(screenpos[0]);
      screenpos[1] = floor(screenpos[1]);
      Draw_PText_shadowedf(screenpos[0],screenpos[1],screenpos[2],NULL,&ptext,bone->name);

    }

    VIDRenderFlag_setGL(0);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glFrontFace(GL_CCW);
  }
}

void Draw_axis()
{
  int i;
  RenderHelpers_t *dh = &g_Draws;
  ushort renderflag = RENDER_NODEPTHTEST;
  glLineWidth(1);
  glDisable(GL_LINE_STIPPLE);

  VIDRenderFlag_setGL(renderflag);

  vidClearTexStages(0,i);
  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);

  //X-Axis is red
  glColor3f(1,0,0);
  glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(dh->axisLen,0,0);
  glEnd();

  //Y-Axis is green
  glColor3f(0,1,0);
  glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(0,dh->axisLen,0);
  glEnd();

  //Z-Axis is blue
  glColor3f(0,0,1);
  glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(0,0,dh->axisLen);
  glEnd();
}

void Draw_sphere(const float size, const int hslices, const int vslices)
{
  int i;
  vidClearTexStages(0,i);
  gluSphere( g_VID.gensetup.quadobj,size,hslices,vslices);
}

void Draw_cube(const float size){
  lxVector3 min,max;
  float half = size*0.5f;

  lxVector3Set(min,-half,-half,-half);
  lxVector3Set(max,half,half,half);

  Draw_box(min,max,LUX_FALSE);
}

void Draw_box(const lxVector3 min,const lxVector3 max,const int drawWireframe)
{
  int i;
  // renders a box at the desired position
  // simply pass the minimum and maximum along
  //
  // yes, this is a copy of cb's vidDrawBBOX, i simplyfied it a bit
  // no, i don't like the vid prefix ;)

  //const float *min = maxa;
  //const float *max = mina;

  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);
  vidClearTexStages(0,i);

  glDisable(GL_LINE_STIPPLE);

  if(drawWireframe)
  {
    glLineWidth(1);
    vidLighting(LUX_FALSE);
    glBegin(GL_LINE_LOOP);
  }
  else
  {
    glBegin(GL_QUADS);
  }
    // bottom
    glNormal3f(0,-1,0);
    glVertex3f(min[0],min[1],min[2]);
    glVertex3f(max[0],min[1],min[2]);
    glVertex3f(max[0],min[1],max[2]);
    glVertex3f(min[0],min[1],max[2]);

  if(drawWireframe)
  {
    glEnd();
    glBegin(GL_LINE_LOOP);
  }
    //top
    glNormal3f(0,1,0);
    glVertex3f(min[0],max[1],min[2]);
    glVertex3f(min[0],max[1],max[2]);
    glVertex3f(max[0],max[1],max[2]);
    glVertex3f(max[0],max[1],min[2]);
  if(drawWireframe)
  {
    glEnd();
    glBegin(GL_LINE_LOOP);
  }
    //front
    glNormal3f(0,0,1);
    glVertex3f(max[0],max[1],max[2]);
    glVertex3f(min[0],max[1],max[2]);
    glVertex3f(min[0],min[1],max[2]);
    glVertex3f(max[0],min[1],max[2]);
  if(drawWireframe)
  {
    glEnd();
    glBegin(GL_LINE_LOOP);
  }
    //back
    glNormal3f(0,0,-1);
    glVertex3f(max[0],max[1],min[2]);
    glVertex3f(max[0],min[1],min[2]);
    glVertex3f(min[0],min[1],min[2]);
    glVertex3f(min[0],max[1],min[2]);
  if(drawWireframe)
  {
    glEnd();
    glBegin(GL_LINE_LOOP);
  }
    //right
    glNormal3f(1,0,0);
    glVertex3f(max[0],max[1],min[2]);
    glVertex3f(max[0],max[1],max[2]);
    glVertex3f(max[0],min[1],max[2]);
    glVertex3f(max[0],min[1],min[2]);
  if(drawWireframe)
  {
    glEnd();
    glBegin(GL_LINE_LOOP);
  }
    //left
    glNormal3f(-1,0,0);
    glVertex3f(min[0],max[1],min[2]);
    glVertex3f(min[0],min[1],min[2]);
    glVertex3f(min[0],min[1],max[2]);
    glVertex3f(min[0],max[1],max[2]);
  glEnd();
}

void Draw_boxWireColor(const lxVector3 min, const lxVector3 max, const lxVector4 color)
{
  glColor4fv(color);

#define Line(a,b,c,d,e,f) glVertex3f(a,b,c); glVertex3f(d,e,f);
  glBegin(GL_LINES);
  Line(min[0],min[1],min[2], max[0],min[1],min[2]);
  Line(min[0],max[1],min[2], max[0],max[1],min[2]);
  Line(min[0],min[1],max[2], max[0],min[1],max[2]);
  Line(min[0],max[1],max[2], max[0],max[1],max[2]);

  Line(min[0],max[1],max[2], min[0],min[1],max[2]);
  Line(max[0],max[1],max[2], max[0],min[1],max[2]);
  Line(min[0],max[1],min[2], min[0],min[1],min[2]);
  Line(max[0],max[1],min[2], max[0],min[1],min[2]);

  Line(min[0],min[1],min[2], min[0],min[1],max[2]);
  Line(min[0],max[1],min[2], min[0],max[1],max[2]);
  Line(max[0],min[1],min[2], max[0],min[1],max[2]);
  Line(max[0],max[1],min[2], max[0],max[1],max[2]);
  glEnd();
}

void Draw_SkyBox(struct SkyBox_s *skybox){
  lxVector3 min,max;
  lxMatrix44 matrix;
  int i;
  float skyboxsize = LUX_LERP(0.5f,g_CamLight.camera->frontplane,g_CamLight.camera->backplane);

  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);


  lxMatrix44CopyRot(matrix,g_VID.drawsetup.viewMatrix);
  matrix[3]=matrix[7]=matrix[11]=0;
  matrix[12]=matrix[13]=matrix[14]=0;
  matrix[15]=1;
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(matrix);
  vidNoDepthTest(LUX_FALSE);
  vidNoDepthMask(LUX_FALSE);

  vidStateDefault();

  if (skybox->fovfactor != 1){
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  if(g_CamLight.camera->fov > 0){
    glLoadIdentity ();
    gluPerspective((g_CamLight.camera->fov*skybox->fovfactor)/g_CamLight.camera->aspect,g_CamLight.camera->aspect,g_CamLight.camera->frontplane,g_CamLight.camera->backplane);
  }
  else{
    //MatrixOrtho(proj,-cam->fov,cam->frontplane,cam->backplane,g_Window.ratio);
    //glLoadMatrixf(proj);
    float size = -g_CamLight.camera->fov*0.5f*skybox->fovfactor;
    glLoadIdentity();
    glOrtho(-size*g_Window.ratio,size*g_Window.ratio,-size,size,g_CamLight.camera->frontplane,g_CamLight.camera->backplane);
  }
  }

  lxVector3Set(max,-skyboxsize,-skyboxsize,-skyboxsize);
  lxVector3Set(min,skyboxsize,skyboxsize,skyboxsize);


  vidClearTexStages(1,i);
  vidSelectTexture(0);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glTranslatef(0,0,0);

  if (skybox->cubeRID != -1){
    //glRotatef(90,1,0,0);

    vidTexBlendDefault(VID_TEXBLEND_REP_REP);
    vidBindTexture(skybox->cubeRID);
    vidTexturing(ResData_getTextureTarget(skybox->cubeRID));

    glMultMatrixf(g_VID.drawsetup.skyrotMatrix);
    glBegin(GL_QUADS);
    // +y
    glTexCoord3f(-1,1,1);
    glVertex3f(max[0],min[1],min[2]);
    glTexCoord3f(-1,1,-1);
    glVertex3f(max[0],min[1],max[2]);
    glTexCoord3f(1,1,-1);
    glVertex3f(min[0],min[1],max[2]);
    glTexCoord3f(1,1,1);
    glVertex3f(min[0],min[1],min[2]);
    // -y
    glTexCoord3f(1,-1,1);
    glVertex3f(min[0],max[1],min[2]);
    glTexCoord3f(1,-1,-1);
    glVertex3f(min[0],max[1],max[2]);
    glTexCoord3f(-1,-1,-1);
    glVertex3f(max[0],max[1],max[2]);
    glTexCoord3f(-1,-1,1);
    glVertex3f(max[0],max[1],min[2]);
    // -z
    glTexCoord3f(-1,-1,-1);
    glVertex3f(max[0],max[1],max[2]);
    glTexCoord3f(1,-1,-1);
    glVertex3f(min[0],max[1],max[2]);
    glTexCoord3f(1,1,-1);
    glVertex3f(min[0],min[1],max[2]);
    glTexCoord3f(-1,1,-1);
    glVertex3f(max[0],min[1],max[2]);
    // +z
    glTexCoord3f(1,-1,1);
    glVertex3f(min[0],max[1],min[2]);
    glTexCoord3f(-1,-1,1);
    glVertex3f(max[0],max[1],min[2]);
    glTexCoord3f(-1,1,1);
    glVertex3f(max[0],min[1],min[2]);
    glTexCoord3f(1,1,1);
    glVertex3f(min[0],min[1],min[2]);
    // -x
    glTexCoord3f(-1,1,1);
    glVertex3f(max[0],min[1],min[2]);
    glTexCoord3f(-1,-1,1);
    glVertex3f(max[0],max[1],min[2]);
    glTexCoord3f(-1,-1,-1);
    glVertex3f(max[0],max[1],max[2]);
    glTexCoord3f(-1,1,-1);
    glVertex3f(max[0],min[1],max[2]);
    // +x
    glTexCoord3f(1,1,-1);
    glVertex3f(min[0],min[1],max[2]);
    glTexCoord3f(1,-1,-1);
    glVertex3f(min[0],max[1],max[2]);
    glTexCoord3f(1,-1,1);
    glVertex3f(min[0],max[1],min[2]);
    glTexCoord3f(1,1,1);
    glVertex3f(min[0],min[1],min[2]);
    glEnd();
  }
  else{

    vidTexBlendDefault(VID_TEXBLEND_REP_REP);
    vidTexturing(GL_TEXTURE_2D);
    // north
    vidBindTexture(skybox->northRID);
    ResData_setTextureClamp(skybox->northRID,TEX_CLAMP_ALL);
    glBegin(GL_QUADS);
    glTexCoord2f(0,1);
    glVertex3f(max[0],min[1],min[2]);
    glTexCoord2f(0,0);
    glVertex3f(max[0],min[1],max[2]);
    glTexCoord2f(1,0);
    glVertex3f(min[0],min[1],max[2]);
    glTexCoord2f(1,1);
    glVertex3f(min[0],min[1],min[2]);
    glEnd();
    // south
    vidBindTexture(skybox->southRID);
    ResData_setTextureClamp(skybox->southRID,TEX_CLAMP_ALL);
    glBegin(GL_QUADS);
    glTexCoord2f(0,1);
    glVertex3f(min[0],max[1],min[2]);
    glTexCoord2f(0,0);
    glVertex3f(min[0],max[1],max[2]);
    glTexCoord2f(1,0);
    glVertex3f(max[0],max[1],max[2]);
    glTexCoord2f(1,1);
    glVertex3f(max[0],max[1],min[2]);
    glEnd();
    // bottom
    vidBindTexture(skybox->bottomRID);
    ResData_setTextureClamp(skybox->bottomRID,TEX_CLAMP_ALL);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0);
    glVertex3f(max[0],max[1],max[2]);
    glTexCoord2f(1,0);
    glVertex3f(min[0],max[1],max[2]);
    glTexCoord2f(1,1);
    glVertex3f(min[0],min[1],max[2]);
    glTexCoord2f(0,1);
    glVertex3f(max[0],min[1],max[2]);
    glEnd();
    // top
    vidBindTexture(skybox->topRID);
    ResData_setTextureClamp(skybox->topRID,TEX_CLAMP_ALL);
    glBegin(GL_QUADS);
    glTexCoord2f(1,1);
    glVertex3f(min[0],max[1],min[2]);
    glTexCoord2f(0,1);
    glVertex3f(max[0],max[1],min[2]);
    glTexCoord2f(0,0);
    glVertex3f(max[0],min[1],min[2]);
    glTexCoord2f(1,0);
    glVertex3f(min[0],min[1],min[2]);
    glEnd();
    //east
    vidBindTexture(skybox->eastRID);
    ResData_setTextureClamp(skybox->eastRID,TEX_CLAMP_ALL);
    glBegin(GL_QUADS);
    glTexCoord2f(0,1);
    glVertex3f(max[0],max[1],min[2]);
    glTexCoord2f(0,0);
    glVertex3f(max[0],max[1],max[2]);
    glTexCoord2f(1,0);
    glVertex3f(max[0],min[1],max[2]);
    glTexCoord2f(1,1);
    glVertex3f(max[0],min[1],min[2]);
    glEnd();
    //west
    vidBindTexture(skybox->westRID);
    ResData_setTextureClamp(skybox->westRID,TEX_CLAMP_ALL);
    glBegin(GL_QUADS);
    glTexCoord2f(1,0);
    glVertex3f(min[0],max[1],max[2]);
    glTexCoord2f(1,1);
    glVertex3f(min[0],max[1],min[2]);
    glTexCoord2f(0,1);
    glVertex3f(min[0],min[1],min[2]);
    glTexCoord2f(0,0);
    glVertex3f(min[0],min[1],max[2]);
    glEnd();
  }

  if (skybox->fovfactor != 1){
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
  }
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(g_VID.drawsetup.viewMatrix);
  vidTexturing(LUX_FALSE);
  vidNoDepthMask(LUX_FALSE);
}

void Draw_SkyL3DNode(List3DNode_t *l3dnode, enum32 forceflags, enum32 ignoreflags, lxVector3 cammoveinfluence)
{
  static LUX_ALIGNSIMD_V (Camera_t cam);
  static lxMatrix44SIMD cammat;

  Camera_t *oldcam = g_CamLight.camera;
  DrawNode_t *lnode;
  int i;

  // make sure we write at max Z
  vidDepthRange(1.0,1.0);

  // should use manual world
  List3DNode_update(l3dnode,LUX_FALSE);


  // change camera a bit
  memcpy(&cam,g_CamLight.camera,sizeof(Camera_t));
  lxVector3Mul(cam.pos,g_CamLight.camera->pos,cammoveinfluence);
  lxMatrix44Copy(cammat,cam.finalMatrix);
  lxMatrix44SetTranslation(cammat,cam.pos);
  Camera_update(&cam,cammat);

  g_CamLight.camera = &cam;
  Camera_setGL(g_CamLight.camera);

  // update trail
  if (l3dnode->nodeType == LUXI_CLASS_L3D_TRAIL)
    Trail_update(l3dnode->trail,l3dnode->finalMatrix);

  lnode = l3dnode->drawNodes;
  vidWorldMatrix(l3dnode->finalMatrix);
  for (i = 0; i < l3dnode->numDrawNodes; i++,lnode++){
    if (lnode->draw.state.renderflag & RENDER_NODRAW)
      continue;
    DrawNodes_drawList(&lnode->sortkey,1,LUX_TRUE,~(RENDER_FXLIT | ignoreflags),forceflags);
  }

  vidDepthRange(0.0,1.0);
  g_CamLight.camera = oldcam;
  Camera_setGL(g_CamLight.camera);
}

// CrazyButcher draw bbox
void Draw_Model_bbox(const struct Model_s *model)
{
  RenderHelpers_t *dh = &g_Draws;

  //solid
  if (dh->drawBBOXsolid){
    glColor3f(1,1,0);
    Draw_box(model->bbox.min, model->bbox.max, 0);
  }
  if (dh->drawBBOXwire){
  //wire
    glColor3f(0,0,1);
    Draw_box(model->bbox.min, model->bbox.max, 1);
  }
  glColor3f(1,1,1);
}
void DrawPathBone( Model_t *model, Bone_t *bone, Track_t *track, const int density, int length, enum32 animflag);
void Draw_ModelAnim_path(struct Model_s *model, struct Anim_s *anim,const int density)
{
  int i;
  uchar found;
  BoneAssign_t *assign;
  Bone_t *bone;

  // do nothing when there is no assign
  if(anim->assignsListHead == NULL)
    return;

  // find proper assign
  found = LUX_FALSE;
  lxListNode_forEach(anim->assignsListHead,assign)
    if (assign && assign->bonesys == &model->bonesys){
      found = LUX_TRUE;
      break;
    }
  lxListNode_forEachEnd(anim->assignsListHead,assign);

  // we had no luck
  if (!found) {
    return;
  }

  // go thru all tracks and draw path for roots
  for (i = 0; i < anim->numTracks; i++){
    bone = (Bone_t*)assign->bones[i];
    if ( bone->parentB == NULL ){
      // this bone has no parent -> is a root
      DrawPathBone(model,bone,&anim->tracks[i],density,anim->length,anim->animflag);
    }
  }

}
void DrawPathBone( Model_t *model, Bone_t *bone, Track_t *track,const int density, int length, enum32 animflag)
{
  int i,points;

  float steps;
  lxVector3 pos;

  vidClearTexStages(0,i);
  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);

  vidLighting(LUX_FALSE);

  // number of points
  points = density * 10;
  //steps is amount of time we raise each point
  steps = (float)length/(float)points;

  glLineWidth(1);
  glBegin(GL_LINE_STRIP);
  glColor3f(0,1,0);
  for (i = 0; i < points; i++){
    Track_setKey(track,(uint)i*steps,animflag, &bone->keyPRS);
    Bone_setFromKey(bone,(unsigned char)isFlagSet(animflag,ANIM_UPD_ABS_ROTONLY));
    lxMatrix44GetTranslation(bone->updateMatrix,pos);
    glVertex3f(pos[0],pos[1],pos[2]);
    if (i == 0 || i == points){
      glEnd();
      glTranslatef(pos[0],pos[1],pos[2]);
      Draw_sphere(0.75,3,3);
      glTranslatef(-pos[0],-pos[1],-pos[2]);
      glBegin(GL_LINE_STRIP);
    }
  }
  glEnd();

  lxMatrix44Clear(bone->updateMatrix);
}

void Draw_frustum(lxFrustum_t *frustum){
  lxVector3 boxcorners[8];
  int i;
  vidVertexProgram(VID_FIXED);
  vidFragmentProgram(VID_FIXED);
  vidGeometryProgram(VID_FIXED);
  vidClearTexStages(0,i);
  vidLighting(LUX_FALSE);
  glColor3f(1,0.5,0.5);
  lxFrustum_getCorners(frustum,boxcorners);
  vidLoadCameraGL();
  for (i = 0; i < 8; i++){
    glPushMatrix();
    glTranslatef(boxcorners[i][0],boxcorners[i][1],boxcorners[i][2]);
    if (i == 4)
      glColor3f(1,0.8,0.8);
    Draw_sphere(4,8,8);
    glPopMatrix();
  }
}
