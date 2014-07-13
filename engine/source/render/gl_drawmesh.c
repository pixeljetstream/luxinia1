// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../render/gl_camlight.h"
#include "../render/gl_shader.h"
#include "gl_drawmesh.h"
#include "gl_draw3d.h"

// LOCALS
DrawData_t l_DrawData;

extern DrawBatch_t l_batchdata;


//////////////////////////////////////////////////////////////////////////
// DRAW

void  Draw_init(){
  memset(&l_batchdata,0,sizeof(DrawBatch_t));
  //if (g_VID.usevbos && GLEW_ARB_vertex_buffer_object){
  //  glGenBuffersARB(1, &l_DrawData.batchdata.batchmesh.vid.ibo);
  //}
}

void  Draw_deinit(){
  //if (l_batchdata.batchmesh.vid.ibo){
  //  glDeleteBuffersARB(1, &l_batchdata.batchmesh.vid.ibo);
  //}
}

void  Draw_initFrame()
{
  l_batchdata.dnode = NULL;
  l_batchdata.curind = g_VID.drw_meshbuffer->indicesData16;
  l_batchdata.maxind = g_VID.drw_meshbuffer->indicesData16+(LUX_MIN(VID_MAX_INDICES,g_VID.gensetup.batchMaxIndices));
  l_batchdata.batchmesh.indexMin = BIT_ID_FULL16;
  l_batchdata.batchmesh.indexMax = 0;
  l_batchdata.batchmesh.numIndices = 0;
  l_batchdata.batchmesh.indicesData16 = g_VID.drw_meshbuffer->indicesData16;
  l_batchdata.batchmesh.numTris = 0;
  l_batchdata.batchmesh.numVertices = 0;
}

void Draw_setFXLights(int numLights, const struct Light_s **fxlights)
{
  l_DrawData.numFxLights = numLights;
  l_DrawData.fxLights = (void**)fxlights;
}

void Draw_pushMatricesGL()
{
  vidSetLights(g_VID.state.renderflag,l_DrawData.numFxLights,l_DrawData.fxLights);

  if (g_VID.drawsetup.renderscale){
    lxVector3Scale(&g_VID.drawsetup.worldMatrixCache[0], &g_VID.drawsetup.worldMatrix[0], g_VID.drawsetup.renderscale[0]);
    lxVector3Scale(&g_VID.drawsetup.worldMatrixCache[4], &g_VID.drawsetup.worldMatrix[4], g_VID.drawsetup.renderscale[1]);
    lxVector3Scale(&g_VID.drawsetup.worldMatrixCache[8], &g_VID.drawsetup.worldMatrix[8], g_VID.drawsetup.renderscale[2]);
    lxVector3Copy(&g_VID.drawsetup.worldMatrixCache[12], &g_VID.drawsetup.worldMatrix[12]);

    g_VID.drawsetup.worldMatrix = g_VID.drawsetup.worldMatrixCache;
  }
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMultMatrixf(g_VID.drawsetup.worldMatrix);
}



//////////////////////////////////////////////////////////////////////////
// DrawMesh

int DrawMesh_draw(DrawMesh_t *dnode, flags32 forceflags, flags32 ignoreflags)
{
  flags32 renderflag = (dnode->state.renderflag & ignoreflags) | forceflags;
  VIDRenderFlag_setGL(renderflag);

  if (dnode->state.blend.blendmode)
    vidBlend(dnode->state.blend.blendmode,dnode->state.blend.blendinvert);
  if (dnode->state.alpha.alphafunc)
    vidAlphaFunc(dnode->state.alpha.alphafunc,dnode->state.alpha.alphaval);
  if (dnode->state.line.linewidth > 0){
    glLineWidth(dnode->state.line.linewidth);
    if (dnode->state.line.linestipple){
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(dnode->state.line.linefactor,dnode->state.line.linestipple);
    }
    else
      glDisable(GL_LINE_STIPPLE);
  }

  if (g_VID.state.renderflag & RENDER_NOVERTEXCOLOR){
    glColor4fv(dnode->color);
  }

  if (vidMaterial(dnode->matRID)){
    Material_update(ResData_getMaterial(dnode->matRID),dnode->matobj);
  }

  if (dnode->matobj)
    vidTexMatrixSet(dnode->matobj->texmatrix);
  else
    vidTexMatrixSet(NULL);

  return Draw_Mesh_auto(dnode->mesh,dnode->matRID,NULL,0,NULL);
}

void DrawMesh_clear(DrawMesh_t *dnode)
{
  if (dnode->matobj){
    MaterialObject_free(dnode->matobj);
    dnode->matobj = NULL;
  }
}

//////////////////////////////////////////////////////////////////////////
// DRAWING MESH



#ifdef LUX_VID_FORCE_LIMITPRIMS
#define USED_INDICES(ind) LUX_MIN(ind,4)
#else
#define USED_INDICES(ind) (ind)
#endif


void Draw_Mesh_simple(Mesh_t *mesh)
{
  int meshtype = mesh->meshtype;

  LUX_DEBUGASSERT(mesh);

  if (!mesh->numVertices)
    return;

  // draw mesh
  if (meshtype != MESH_DL){
    int n;
    ushort  *pointtri;
    uint  *pointtriI;
    uint  offsetTri;
    uint  vertexoffset;
    GLenum  rendertype = mesh->primtype;
    GLenum  indicestype = mesh->index16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

    LUX_DEBUGASSERT(!(mesh->meshtype == MESH_VBO && !mesh->vid.vbo));

    if (mesh->vid.ibo){
      pointtri  = NULL;
      pointtriI = NULL;
      offsetTri  = mesh->vid.ibooffset;
    }
    else{
      offsetTri = 0;
      pointtri  = mesh->indicesData16;
      pointtriI = mesh->indicesData32;
    }
    vidBindBufferIndex(mesh->vid.ibo);

    vertexoffset = mesh->vid.vbo ? mesh->vid.vbooffset : 0;


    // feed vertex attribs
    {
      int t;
      Mesh_setStandardAttribsGL(mesh,
        !(g_VID.state.renderflag & RENDER_NOVERTEXCOLOR),
        g_VID.drawsetup.lightCnt || g_VID.drawsetup.setnormals);

      for (t = 0; t < VID_MAX_TEXTURE_UNITS; t++){
        if (g_VID.state.texcoords[t].type < 0){
          Mesh_setTexCoordGL(mesh,t,g_VID.state.texcoords[t].channel);
        }
      }

      if (g_VID.drawsetup.attribs.needed & (1<<VERTEX_ATTRIB_TANGENT)){
        Mesh_setTangentGL(mesh);
      }
    }

    if (mesh->vid.ibo || mesh->indicesData){
      if (mesh->numGroups < 2){
        if (GLEW_EXT_draw_range_elements)
          glDrawRangeElementsEXT(rendertype,mesh->indexMin + vertexoffset,mesh->indexMax + vertexoffset, USED_INDICES(mesh->numIndices),  indicestype ,mesh->index16 ? (void*)(&pointtri[offsetTri]) : (void*)(&pointtriI[offsetTri]));
        else
          glDrawElements(rendertype, USED_INDICES(mesh->numIndices),  indicestype, mesh->index16 ? (void*)(&pointtri[offsetTri]) : (void*)(&pointtriI[offsetTri]));
      }
      else {
        for (n = 0; n < mesh->numGroups; n++){
          if (GLEW_EXT_draw_range_elements)
            glDrawRangeElementsEXT(rendertype,mesh->indexMin + vertexoffset,mesh->indexMax + vertexoffset, USED_INDICES(mesh->indicesGroupLength[n]),  indicestype ,mesh->index16 ? (void*)(&pointtri[offsetTri]) : (void*)(&pointtriI[offsetTri]));
          else
            glDrawElements(rendertype, USED_INDICES(mesh->indicesGroupLength[n]),  indicestype, mesh->index16 ? (void*)(&pointtri[offsetTri]) : (void*)(&pointtriI[offsetTri]));

          offsetTri += mesh->indicesGroupLength[n];
        }
      }
    }
    else{
      glDrawArrays(rendertype,vertexoffset,mesh->numVertices);
    }
  }
  else{
    vidBindBufferArray(NULL);
    vidBindBufferIndex(NULL);
    if (g_VID.gpuvendor == VID_VENDOR_ATI)
      if((g_VID.drawsetup.lightCnt || g_VID.drawsetup.setnormals) && (mesh->vertextype == VERTEX_32_NRM || mesh->vertextype == VERTEX_64_TEX4 || mesh->vertextype == VERTEX_64_SKIN || mesh->vertextype == VERTEX_32_FN))
        glEnableClientState(GL_NORMAL_ARRAY);
      else
        glDisableClientState(GL_NORMAL_ARRAY);

    glCallList(mesh->vid.glID);
    g_VID.state.activeTex = -1;
    g_VID.state.activeClientTex = -1;

    LUX_DEBUGASSERT(!glIsEnabled(GL_COLOR_ARRAY)==isFlagSet(g_VID.state.renderflag,RENDER_NOVERTEXCOLOR));

    //if (!glIsEnabled(GL_COLOR_ARRAY)) g_VID.state.renderflag |= RENDER_NOVERTEXCOLOR;
    //else                g_VID.state.renderflag &= ~RENDER_NOVERTEXCOLOR;

    vidResetPointer();
  }


#if 0
  glEnableClientState(GL_VERTEX_ARRAY);
  vidResetPointer();
  vidResetBuffers();
#endif

}

#undef USED_INDICES

//////////////////////////////////////////////////////////////////////////
// helpers

#include "gl_drawmeshinl.h"

//////////////////////////////////////////////////////////////////////////


static int Draw_Mesh_basic(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors, struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_color_prep(mesh);


  vidCheckError();
  vidClearTexStages(l_DrawData.texStages,i);
  vidCheckError();
  _dset_set_gpu();
  _dset_initmesh(mesh);

  _dset_fog_start();
  Draw_Mesh_simple(mesh);
  _dset_fog_end();

  _dset_fixed_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return 1;
}

static int Draw_Mesh_basic_skin(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_color_prep(mesh);
  _dset_skin_start(mesh,skinobj);
  _dset_set_gpu();
  vidClearTexStages(l_DrawData.texStages,i);
  _dset_initmesh(mesh);

  _dset_fog_start();
  Draw_Mesh_simple(mesh);
  _dset_fog_end();

  _dset_skin_end();
  _dset_fixed_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return 1;
}

static int Draw_Mesh_basic_proj(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_color_prep(mesh);
  _dset_proj_prep(numProjectors,projectors);
  _dset_lock(mesh);
  _dset_set_gpu();
  _dset_initmesh(mesh);

  _dset_multipass_start
    if(_dset_proj_start(projectors,mesh))continue;
    _dset_fog_start();
    vidClearTexStages(l_DrawData.texStages,i);
    Draw_Mesh_simple(mesh);

    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_proj_end();

  _dset_fixed_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_basic_proj_skin(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_color_prep(mesh);
  _dset_proj_prep(numProjectors,projectors);
  _dset_skin_start(mesh,skinobj);
  _dset_lock(mesh);
  _dset_set_gpu();
  _dset_initmesh(mesh);

  _dset_multipass_start
    if(_dset_proj_start(projectors,mesh))continue;
    _dset_fog_start();
    vidClearTexStages(l_DrawData.texStages,i);
    Draw_Mesh_simple(mesh);

    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_skin_end();
  _dset_proj_end();
  _dset_fixed_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_tex(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_tex_start(mesh,texindex);
  _dset_set_gpu();
  _dset_initmesh(mesh);

  _dset_fog_start();
  vidClearTexStages(l_DrawData.texStages,i);
  Draw_Mesh_simple(mesh);
  _dset_fog_end();

  _dset_fixed_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return 1;
}
static int Draw_Mesh_tex_skin(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_tex_start(mesh,texindex);
  _dset_skin_start(mesh,skinobj);
  _dset_set_gpu();
  _dset_initmesh(mesh);

  _dset_fog_start();
  vidClearTexStages(l_DrawData.texStages,i);
  Draw_Mesh_simple(mesh);
  _dset_fog_end();

  _dset_skin_end();
  _dset_fixed_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return 1;
}
static int Draw_Mesh_tex_proj(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_tex_start(mesh,texindex);
  _dset_proj_prep(numProjectors,projectors);
  _dset_lock(mesh);
  _dset_set_gpu();
  _dset_initmesh(mesh);

  _dset_multipass_start
    if(_dset_proj_start(projectors,mesh))continue;
    _dset_fog_start();
    vidClearTexStages(l_DrawData.texStages,i);
    Draw_Mesh_simple(mesh);

    _dset_fog_end();

  _dset_multipass_end;

  _dset_unlock();
  _dset_proj_end();
  _dset_fixed_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_tex_proj_skin(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_tex_start(mesh,texindex);
  _dset_proj_prep(numProjectors,projectors);
  _dset_skin_start(mesh,skinobj);
  _dset_lock(mesh);
  _dset_set_gpu();
  _dset_initmesh(mesh);

  _dset_multipass_start
    if(_dset_proj_start(projectors,mesh))continue;
    _dset_fog_start();
    vidClearTexStages(l_DrawData.texStages,i);
    Draw_Mesh_simple(mesh);

    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_skin_end();
  _dset_proj_end();
  _dset_fixed_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}

//material
static int Draw_Mesh_material(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_mat_prep(texindex);
  _dset_shader_prep(mesh);
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_material_skin(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_mat_prep(texindex);
  _dset_shader_prep(mesh);
  _dset_skin_start(mesh,skinobj);
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_skin_end();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_material_proj(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_mat_prep(texindex);
  _dset_shader_prep(mesh);
  _dset_proj_prep(numProjectors,projectors);
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    if(_dset_proj_start(projectors,mesh))continue;
    vidClearTexStages(l_DrawData.texStages,i);
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_proj_end();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_material_proj_skin(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_mat_prep(texindex);
  _dset_shader_prep(mesh);
  _dset_proj_prep(numProjectors,projectors);
  _dset_skin_start(mesh,skinobj);
  _dset_set_gpu();
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    if(_dset_proj_start(projectors,mesh))continue;
    vidClearTexStages(l_DrawData.texStages,i);
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_skin_end();
  _dset_proj_end();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}

// still in-use main render func
// renders given model
// mesh must be specified !

static DrawMeshFunc_t l_DrawMeshFuncsFull[]=
{
  {Draw_Mesh_basic},
  {Draw_Mesh_basic_skin},
  {Draw_Mesh_basic_proj},
  {Draw_Mesh_basic_proj_skin},
  {Draw_Mesh_tex},
  {Draw_Mesh_tex_skin},
  {Draw_Mesh_tex_proj},
  {Draw_Mesh_tex_proj_skin},
  {Draw_Mesh_material},
  {Draw_Mesh_material_skin},
  {Draw_Mesh_material_proj},
  {Draw_Mesh_material_proj_skin},
  {Draw_Mesh_auto},
};
// USING BASE SHADERS

static int Draw_Mesh_shader_basictex(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors, struct SkinObject_s *skinobj)
{
  int type;

  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  type = (texindex < 0) ? VID_SHADER_COLOR :  VID_SHADER_TEX;
  type += (g_VID.shdsetup.lightmapRID >= 0) ? 1 : 0;
  _dset_shadermat_prep(type,texindex);
  _dset_shader_prep(mesh);
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_shader_basictex_skin(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors, struct SkinObject_s *skinobj)
{
  int type;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  type = (texindex < 0) ? VID_SHADER_COLOR :  VID_SHADER_TEX;
  type += (g_VID.shdsetup.lightmapRID >= 0) ? 1 : 0;
  _dset_shadermat_prep(type,texindex);
  _dset_shader_prep(mesh);
  _dset_skin_start(mesh,skinobj);
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_skin_end();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_shader_basictex_proj(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors, struct SkinObject_s *skinobj)
{
  int i;
  int type;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  type = (texindex < 0) ? VID_SHADER_COLOR :  VID_SHADER_TEX;
  type += (g_VID.shdsetup.lightmapRID >= 0) ? 1 : 0;
  _dset_shadermat_prep(type,texindex);
  _dset_shader_prep(mesh);
  _dset_proj_prep(numProjectors,projectors);
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    if(_dset_proj_start(projectors,mesh))continue;
    vidClearTexStages(l_DrawData.texStages,i);
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_proj_end();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_shader_basictex_proj_skin(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors, struct SkinObject_s *skinobj)
{
  int type;
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  type = (texindex < 0) ? VID_SHADER_COLOR :  VID_SHADER_TEX;
  type += (g_VID.shdsetup.lightmapRID >= 0) ? 1 : 0;
  _dset_shadermat_prep(type,texindex);
  _dset_shader_prep(mesh);
  _dset_proj_prep(numProjectors,projectors);
  _dset_skin_start(mesh,skinobj);
  _dset_set_gpu();
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    if(_dset_proj_start(projectors,mesh))continue;
    vidClearTexStages(l_DrawData.texStages,i);
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_skin_end();
  _dset_proj_end();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}

static int Draw_Mesh_shader_material(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_shadermat_override_prep(texindex);
  _dset_shader_prep(mesh);
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_shader_material_skin(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_shadermat_override_prep(texindex);
  _dset_shader_prep(mesh);
  _dset_skin_start(mesh,skinobj);
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_skin_end();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_shader_material_proj(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_shadermat_override_prep(texindex);
  _dset_shader_prep(mesh);
  _dset_proj_prep(numProjectors,projectors);
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    if(_dset_proj_start(projectors,mesh))continue;
    vidClearTexStages(l_DrawData.texStages,i);
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_proj_end();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static int Draw_Mesh_shader_material_proj_skin(Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj)
{
  int i;
  // error check
  vidCheckError();
  LUX_DEBUGASSERT(mesh);

  _dset_default(mesh);
  _dset_shadermat_override_prep(texindex);
  _dset_shader_prep(mesh);
  _dset_proj_prep(numProjectors,projectors);
  _dset_skin_start(mesh,skinobj);
  _dset_lock(mesh);
  _dset_initmesh(mesh);

  _dset_multipass_start
    _dset_mat_pass_start();
    if(_dset_proj_start(projectors,mesh))continue;
    vidClearTexStages(l_DrawData.texStages,i);
    _dset_fog_start();
    Draw_Mesh_simple(mesh);

    _dset_mat_pass_end();
    _dset_fog_end();
  _dset_multipass_end;

  _dset_unlock();
  _dset_skin_end();
  _dset_proj_end();
  _dset_shader_end();
  _dset_resetmesh(mesh);

  vidCheckError();
  return l_DrawData.numPasses;
}
static DrawMeshFunc_t l_DrawMeshFuncsShaders[]=
{
  {Draw_Mesh_shader_basictex},
  {Draw_Mesh_shader_basictex_skin},
  {Draw_Mesh_shader_basictex_proj},
  {Draw_Mesh_shader_basictex_proj_skin},
  {Draw_Mesh_shader_basictex},
  {Draw_Mesh_shader_basictex_skin},
  {Draw_Mesh_shader_basictex_proj},
  {Draw_Mesh_shader_basictex_proj_skin},
  {Draw_Mesh_shader_material},
  {Draw_Mesh_shader_material_skin},
  {Draw_Mesh_shader_material_proj},
  {Draw_Mesh_shader_material_proj_skin},
  {Draw_Mesh_auto},
};

DrawMeshFunc_t  *l_DrawMeshFuncs = l_DrawMeshFuncsFull;

// returns numpasses
int  Draw_Mesh_auto( Mesh_t *mesh,const int texindex, struct Projector_s **projectors,const int numProjectors,  struct SkinObject_s *skinobj){
  DrawMeshType_t type = DRAW_MESH_BASIC;
  type += (texindex >= 0) * DRAW_MESH_SHIFT_TYPE;
  type += (vidMaterial(texindex)) * DRAW_MESH_SHIFT_TYPE;
  type += (skinobj != NULL) * DRAW_MESH_SHIFT_SKIN;
  type += (numProjectors > 0) * DRAW_MESH_SHIFT_PROJ;

  l_DrawData.numFxLights = 0;
  return l_DrawMeshFuncs[type].func(mesh,texindex,projectors,numProjectors,skinobj);
}

int  Draw_Mesh_type( Mesh_t *mesh, DrawMeshType_t type, const int texindex, struct Projector_s **projectors,const int numProjectors, struct SkinObject_s *skinobj)
{
  return l_DrawMeshFuncs[type].func(mesh,texindex,projectors,numProjectors,skinobj);
}


#undef _dset_multipass_start
#undef _dset_multipass_end
/*
#undef _dset_mat_pass_start
#undef _dset_mat_pass_end
#undef _dset_shadermat_prep
#undef _dset_mat_prep
#undef _dset_color_prep
#undef _dset_lock
#undef _dset_unlock
#undef _dset_skin_end
#undef _dset_default
#undef _dset_fog_end
#undef _dset_fog_start
#undef _dset_set_gpu
*/

#ifndef LUX_VID_COMPILEDARRAY
  #undef _dset_lock
  #undef _dset_unlock
#endif

//////////////////////////////////////////////////////////////////////////
// DRAW Mode Toggle
void  Draw_useBaseShaders(int state){
  if (state)
    l_DrawMeshFuncs = l_DrawMeshFuncsShaders;
  else
    l_DrawMeshFuncs = l_DrawMeshFuncsFull;
}

