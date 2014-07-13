// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/perfgraph.h"
#include "../common/3dmath.h"
#include "../main/main.h"
#include "gl_camlight.h"
#include "gl_list2d.h"
#include "gl_list3d.h"
#include "gl_draw2d.h"
#include "gl_draw3d.h"
#include "gl_drawmesh.h"
#include "gl_print.h"
#include "gl_window.h"
#include "gl_render.h"

#include "../scene/vistest.h"

extern DrawData_t l_DrawData;

// LOCALS
static List2D_t l_List2D;

static void List2DNode_updateMatrix(List2DNode_t *node, const lxVector3 pos);
static void List2DNode_updateScissor(List2DNode_t *node, const float *parentscissor);
static void List2DText_updateLocalBounds(List2DText_t *l2dtext, const float *scissorin, const lxMatrix44 mat);


///////////////////////////////////////////////////////////////////////////////
// List2D
void List2D_init()
{
  Reference_registerType(LUXI_CLASS_L2D_NODE,"l2dnode",RList2DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L2D_TEXT,"l2dtext",RList2DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L2D_IMAGE,"l2dimage",RList2DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L2D_L3DLINK,"l2dnode3d",RList2DNode_free,NULL);
  Reference_registerType(LUXI_CLASS_L2D_FLAG,"l2dflag",RList2DNode_free,NULL);

  l_List2D.fps = l_List2D.fpsaccum = 0;
  l_List2D.root = List2DNode_new("_root");
  l_List2D.root->reference = Reference_new(LUXI_CLASS_L2D_NODE,l_List2D.root);
  Reference_makeStrong(l_List2D.root->reference);

  l_List2D.cmdclear = (RenderCmdClear_t*)RenderCmd_new(LUXI_CLASS_RCMD_CLEAR);
  Reference_makeStrong(l_List2D.cmdclear->cmd.reference);
  l_List2D.cmdclear->depth = 1.0f;
  l_List2D.cmdclear->clearcolor = LUX_FALSE;
  l_List2D.cmdclear->cleardepth = LUX_TRUE;
  l_List2D.cmdclear->clearstencil = LUX_TRUE;

}

void List2D_deinit()
{
  Reference_release(l_List2D.root->reference);
  Reference_release(l_List2D.cmdclear->cmd.reference);
}

#define cmpL2DNodeInc(a,b) ((a)->sortkey > (b)->sortkey)

LUX_INLINE static const float* List2DNode_scissorSetup(List2DNode_t *node, booln haddirtymat,
                         booln *haddirtyscissor, const booln parentdirtyscissor, const float *parentscissor)
{
  if (node->usescissor){
    if (node->dirtyscissor || (node->localscissor && haddirtymat) || (node->parentscissor && parentdirtyscissor)){
      *haddirtyscissor = LUX_TRUE;
      List2DNode_updateScissor(node,parentscissor);
    }


    // we have to invert y because scissor origin is bottom left
    // and we have to offset it by height as the rectangle goes to top in gl and for us downwards

    vidScissor(LUX_TRUE);

    glScissor(
      VID_REF_WIDTH_TOPIXEL(node->finalscissorstart[0]),
      g_VID.drawbufferHeight - VID_REF_HEIGHT_TOPIXEL(node->finalscissorstart[1]) - VID_REF_HEIGHT_TOPIXEL(node->finalscissorsize[1]),
      VID_REF_WIDTH_TOPIXEL(node->finalscissorsize[0]),
      VID_REF_HEIGHT_TOPIXEL(node->finalscissorsize[1]));

    return node->finalscissorstart;
  }
  else if (!node->parentscissor){
    if (l_List2D.globalscissor){
      vidScissor(LUX_TRUE);
      glScissor(l_List2D.globalscissor[0],l_List2D.globalscissor[1],l_List2D.globalscissor[2],l_List2D.globalscissor[3]);
    }
    else
      vidScissor(LUX_FALSE);

    return NULL;
  }
  else{
    return parentscissor;
  }
}

static void List2DNode_draw_recursive(List2DNode_t *node,const booln parentdirtymat, const booln parentdirtyscissor, const float *parentscissor)
{
  // must not be static
  int haddirtymat = LUX_FALSE;
  int haddirtyscissor = LUX_FALSE;
  List2DNode_t *browse = NULL;
  const float *outscissor = NULL;

  // check if we are visible
  LUX_DEBUGASSERT(node);

  if (node->renderflag & RENDER_NODRAW) return;
  g_VID.drawsetup.curnode = node;

  if (node->dirtymat || parentdirtymat){
    haddirtymat = LUX_TRUE;
    List2DNode_updateMatrix(node,node->position);
  }


  // we need to sort our children
  if (node->childrenListHead && node->sortchildren){
    lxListNode_sortTapeMerge(node->childrenListHead,List2DNode_t,cmpL2DNodeInc);
    node->sortchildren = LUX_FALSE;
  }


  // scissor setup
  outscissor = List2DNode_scissorSetup(node, haddirtymat,
    &haddirtyscissor, parentdirtyscissor, parentscissor);

  vidCheckError();

  // DRAWING
  switch(node->type){
  case LUXI_CLASS_L2D_FLAG:
    if (node->l2dflag->stencil.enabled)
      VIDStencil_setGL(&node->l2dflag->stencil,node->l2dflag->stencil.twosided);
    break;
  case LUXI_CLASS_L2D_L3DLINK:
  {
    DrawNode_t *lnode;
    int i;

    LUX_ASSERT(node->l3dnode);

    if (node->l3dnode->update && haddirtymat)
      lxMatrix44MultiplySIMD(node->l3dnode->finalMatrix,node->finalMatrix,node->l3dnode->localMatrix);
    else if (haddirtymat)
      lxMatrix44CopySIMD(node->l3dnode->finalMatrix,node->finalMatrix);

    List3DNode_update(node->l3dnode,LUX_FALSE);

    // update trail
    if (node->l3dnode->nodeType == LUXI_CLASS_L3D_TRAIL)
      Trail_update(node->l3dnode->trail,node->finalMatrix);

    lnode = node->l3dnode->drawNodes;
    vidWorldMatrix(node->l3dnode->finalMatrix);
    vidWorldScale(node->scale);
    for (i = 0; i < node->l3dnode->numDrawNodes; i++,lnode++){
      if (lnode->draw.state.renderflag & RENDER_NODRAW)
        continue;
      DrawNodes_drawList(&lnode->sortkey,1,LUX_TRUE,~(RENDER_FXLIT | RENDER_SUNLIT | RENDER_LIT),0);
    }
    // revert to fixed function which is the majority here
    vidVertexProgram(VID_FIXED);
    vidFragmentProgram(VID_FIXED);
    vidGeometryProgram(VID_FIXED);
  }
    break;
  case LUXI_CLASS_L2D_IMAGE:
  {
    List2DImage_t *l2dimage = node->l2dimage;
    enum32 renderflag = node->renderflag;

    vidWorldMatrix(node->finalMatrix);
    vidWorldScale(node->scale);

    if (vidMaterial(l2dimage->texid)){
      Shader_t *shader;
      Material_update(ResData_getMaterial(l2dimage->texid),l2dimage->matobj);
      shader = ResData_getMaterialShaderUse(l2dimage->texid);
      renderflag |= shader->renderflag;
    }

    if (l2dimage->matobj)
      vidTexMatrixSet(l2dimage->matobj->texmatrix);
    else
      vidTexMatrixSet(NULL);

    VIDRenderFlag_setGL(renderflag);

    if (node->line.linewidth > 0){
      VIDLine_setGL(&node->line);
    }


    if (node->blend.blendmode)
      vidBlend(node->blend.blendmode,node->blend.blendinvert);
    if (node->alpha.alphafunc)
      vidAlphaFunc(node->alpha.alphafunc,node->alpha.alphaval);


    // draw
    glColor4fv(node->color);
    Draw_Mesh_auto(l2dimage->mesh,l2dimage->texid,NULL,0,NULL);

    /*
    if(l2dimage->texid < 0){
      // HACK for old nvi driver
      vidSelectTexture(0);
      vidBindTexture(PText_getDefaultFontTexRID());
    }
    */
  }
    break;
  case LUXI_CLASS_L2D_TEXT:
  {
    List2DText_t   *l2dtext = node->l2dtext;
    VIDRenderFlag_setGL(node->renderflag);

    if (node->blend.blendmode)
      vidBlend(node->blend.blendmode,node->blend.blendinvert);
    if (node->alpha.alphafunc)
      vidAlphaFunc(node->alpha.alphafunc,node->alpha.alphaval);

    vidColor4fv(node->color);
    LUX_DEBUGASSERT(l2dtext);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(node->finalScaled);
    // draw text optimized to view or current scissor bounds

    // FIXME warning finalScaled would throw off text bounds
    // but as scissoring would break with scales & rotation anyway
    // it's negligable
    if (!outscissor)
      List2DText_updateLocalBounds(l2dtext,l_List2D.orthobounds,node->finalMatrix);
    else if (haddirtymat || haddirtyscissor)
      List2DText_updateLocalBounds(l2dtext,outscissor,node->finalMatrix);

    Draw_PText(0,0,0, l2dtext->localscissorstart,&l2dtext->ptext,NULL);
    glPopMatrix();
    break;
  }
  case LUXI_CLASS_L2D_L3DVIEW:
  {
    List3DView_t  *lview = NULL;
    List3DNode_t  *lsun = NULL;

    if (Reference_isValid(node->l2dview->viewref) &&
      Reference_get(node->l2dview->viewref,lview))
    {
      if (Reference_isValid(node->l2dview->sunref)){
        Reference_get(node->l2dview->sunref,lsun);
      }

      // prep viewbounds

      VIDViewBounds_set(&lview->viewport.bounds,LUX_FALSE,
        VID_REF_WIDTH_TOPIXEL(node->finalMatrix[12]),
        g_VID.drawbufferHeight - VID_REF_HEIGHT_TOPIXEL(node->finalMatrix[13]) - VID_REF_HEIGHT_TOPIXEL(node->scale[1]),
        VID_REF_WIDTH_TOPIXEL(node->scale[0]),
        VID_REF_HEIGHT_TOPIXEL(node->scale[1]));

      vidOrthoOff();
      List3DView_drawDirect(lview,lsun->light);
      vidOrthoOn(0,VID_REF_WIDTH, VID_REF_HEIGHT,0,-128,128);

      // must reset scissor and viewport, as l3dview messes with it
      vidViewport(0,0,g_VID.drawbufferWidth,g_VID.drawbufferHeight);
      outscissor = List2DNode_scissorSetup(node, haddirtymat,
        &haddirtyscissor, parentdirtyscissor, parentscissor);

      vidDepthRange(0.0f,1.0f);
    }
  }
    break;
  default:
    break;
  }


  if (node->childrenListHead){
    lxListNode_forEach(node->childrenListHead,browse)
      List2DNode_draw_recursive(browse,haddirtymat,haddirtyscissor,outscissor);
    lxListNode_forEachEnd(node->childrenListHead,browse);
  }

}

static void List2D_drawDebug()
{
  static lxMatrix44SIMD texmat = {1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1};
  static PText_t ptext;
  Texture_t *tex;

  PText_setDefaults(&ptext);

  if (g_Draws.drawSpecial >= 0 && (tex=ResData_getTexture(g_Draws.drawSpecial)) &&
    tex->format != TEX_FORMAT_1D_ARRAY && tex->format != TEX_FORMAT_2D_ARRAY){
    vidSelectTexture(0);

    if (tex->format == TEX_FORMAT_RECT){
      texmat[0] = (float)tex->sarr3d.sz.width;
      texmat[5] = (float)tex->sarr3d.sz.height;
      vidTexMatrixSet(texmat);
    }
    else{
      vidTexMatrixSet(NULL);
    }

    if (tex->dataformat == GL_DEPTH_COMPONENT && GLEW_ARB_shadow){
      vidBindTexture(g_Draws.drawSpecial);
      glTexParameteri(tex->target,GL_TEXTURE_COMPARE_MODE_ARB,GL_NONE);
    }

    if (tex->format == TEX_FORMAT_CUBE)
      Draw_Texture_cubemap(g_Draws.drawSpecial,VID_REF_WIDTH-(g_Draws.texsize*VID_REF_WIDTHSCALE),0,(g_Draws.texsize*VID_REF_WIDTHSCALE));
    else
      Draw_Texture_screen(VID_REF_WIDTH-(g_Draws.texsize*VID_REF_WIDTHSCALE),0,-121,(g_Draws.texsize*VID_REF_WIDTHSCALE),(g_Draws.texsize*VID_REF_WIDTHSCALE),g_Draws.drawSpecial,LUX_FALSE,LUX_FALSE,0,~0);

    if (tex->dataformat == GL_DEPTH_COMPONENT && GLEW_ARB_shadow){
      vidSelectTexture(0);
      vidBindTexture(g_Draws.drawSpecial);
      glTexParameteri(tex->target,GL_TEXTURE_COMPARE_MODE_ARB,GL_COMPARE_R_TO_TEXTURE);
    }
  }
  else
    g_Draws.drawSpecial = -1;

  if (g_Draws.drawStencilBuffer){
    GLboolean state;
    char *pixels;

    bufferminsize(sizeof(char)*g_Window.width*g_Window.height);
    pixels = buffermalloc(sizeof(char)*g_Window.width*g_Window.height);
    LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

    glFinish();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    vidBindBufferPack(NULL);
    glReadPixels(0,0,g_Window.width,g_Window.height,GL_STENCIL_INDEX,GL_UNSIGNED_BYTE,pixels);
    glRasterPos3f(-1,-1,0);
    glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID,&state);
    if (state){
      glDrawPixels(g_Window.width,g_Window.height,GL_LUMINANCE,GL_UNSIGNED_BYTE,pixels);
    }
    glPopMatrix();
    vidCheckError();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }


  VIDRenderFlag_setGL(PText_getDefaultRenderFlag());
  vidBlend(VID_DECAL,LUX_FALSE);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  if (g_Draws.drawFPS)
  {
    l_List2D.fpsaccum += getFPSAvg();
    if (!(g_VID.frameCnt % 32)){
      l_List2D.fps = l_List2D.fpsaccum/32;
      l_List2D.fpsaccum = 0;
    }
    vidColor4f(1,1,1,1);
    Draw_PText_shadowedf(VID_REF_WIDTH-60,10,-122,NULL,&ptext,"%3d",l_List2D.fps);
  }

  if (g_Draws.drawStats)
  {
    float height = 10*SCREEN_SCALE*12;
    float alpha = g_Draws.statsColor[3];
    int i;
    vidClearTexStages(0,i);

    glColor4f(0,0,0,alpha);
    glBegin(GL_QUADS);
    glVertex3f(VID_REF_WIDTH,0,0);
    glVertex3f(VID_REF_WIDTH,height,0);
    glVertex3f(0,height,0);
    glVertex3f(0,0,0);
    glEnd();

    PText_setWidth(&ptext,12.0f);
    PText_setSize(&ptext,SCREEN_SCALE*12);
    g_Draws.statsColor[3] = 1.0f;
    vidColor4fv(g_Draws.statsColor);
    Draw_PText(1,1,-124,NULL,&ptext,ProfileData_getStatsString());
    g_Draws.statsColor[3] = alpha;
  }
  if (g_Draws.drawPerfGraph){
    int i;
    for (i = 0; i < PGRAPHS; i++){
      if (g_Draws.drawPerfGraph & (1<<i))
        PGraph_draw(i);
    }
  }
}


void List2D_draw(List2DNode_t *root,float refw,float refh,int dirtyscissor,float *parentscissor, int *globalscissor)
{
  float oldrefw = VID_REF_WIDTH;
  float oldrefh = VID_REF_HEIGHT;

  VID_REF_WIDTH = refw;
  VID_REF_HEIGHT = refh;

  // needed as top/bottom is swapped
  glFrontFace(GL_CW);

  vidOrthoOn(0,VID_REF_WIDTH, VID_REF_HEIGHT,0,-128,128);

  lxVector4Set(l_List2D.orthobounds,0,0,refw,refh);

  if (!root){
    RenderCmdClear_run(l_List2D.cmdclear,NULL,NULL);
    vidDepthRange(0.0f,1.0f);
  }

  l_List2D.globalscissor = globalscissor;


  // Start drawing at root node
  List2DNode_draw_recursive(root ? root : l_List2D.root,LUX_FALSE,dirtyscissor,parentscissor);

  vidScissor(LUX_FALSE);
  VIDRenderFlag_setGL(0);
  vidResetTexture();

  if (!root)
    List2D_drawDebug();


  VID_REF_WIDTH = oldrefw;
  VID_REF_HEIGHT = oldrefh;

  vidOrthoOff();
}

#undef cmpL2DNodeInc

///////////////////////////////////////////////////////////////////////////////
// List2DImage

List2DImage_t*  List2DImage_new()
{
  return lxMemGenZalloc(sizeof(List2DImage_t));
}

void List2DImage_free(List2DImage_t *l2dimage)
{
  if (l2dimage->matobj)
    MaterialObject_free(l2dimage->matobj);
  Reference_releaseCheck(l2dimage->usermesh);
  lxMemGenFree(l2dimage,sizeof(List2DImage_t));
}

///////////////////////////////////////////////////////////////////////////////
// List2DText

List2DText_t* List2DText_new(const char *text)
{
  List2DText_t* l2dtext = lxMemGenZalloc(sizeof(List2DText_t));
  PText_setDefaults(&l2dtext->ptext);
  PText_setText(&l2dtext->ptext,text);
  return l2dtext;
}


void List2DText_free(List2DText_t *l2dtext)
{
  PText_setText(&l2dtext->ptext,NULL);

  Reference_releaseWeak(l2dtext->ptext.fontsetref);
  lxMemGenFree(l2dtext,sizeof(List2DText_t));
}


static void List2DText_updateLocalBounds(List2DText_t *l2dtext, const float *scissorin, const lxMatrix44 mat)
{
  lxVector2InvTransform(l2dtext->localscissorstart,scissorin,mat);
  lxVector2Set(l2dtext->localscissorsize,scissorin[2],scissorin[3]);
}
//////////////////////////////////////////////////////////////////////////
// List2DFlag

List2DFlag_t* List2DFlag_new()
{
  List2DFlag_t *flag;
  flag = lxMemGenZalloc(sizeof(List2DFlag_t));
  VIDStencil_init(&flag->stencil);
  return flag;
}

void List2DFlag_free(List2DFlag_t *flag)
{
  lxMemGenFree(flag,sizeof(List2DFlag_t));
}

///////////////////////////////////////////////////////////////////////////////
// List2DNode

List2DNode_t *List2DNode_new(const char *name)
{
  List2DNode_t *node = genzallocSIMD(sizeof(List2DNode_t));

  LUX_SIMDASSERT(((size_t)((List2DNode_t*)node)->finalMatrix) % 16 == 0);

  lxVector4Set(node->color,1,1,1,1);
  lxVector3Set(node->scale, 1, 1, 1);
  lxListNode_init(node);
  node->type = LUXI_CLASS_L2D_NODE;
  node->parent = NULL;

  if (name)
    gennewstrcpy(node->name,name);

  lxMatrix44IdentitySIMD(node->finalScaled);
  lxMatrix44IdentitySIMD(node->finalMatrix);

  return node;
}

List2DNode_t *List2DNode_newFlag(const char *name)
{
  List2DNode_t *node = List2DNode_new(name);
  node->type = LUXI_CLASS_L2D_FLAG;
  node->l2dflag = List2DFlag_new();

  node->reference = Reference_new(LUXI_CLASS_L2D_FLAG,node);

  return node;
}

List2DNode_t *List2DNode_newImage(const char *name, int texid)
{
  List2DNode_t *node = List2DNode_new(name);
  node->type = LUXI_CLASS_L2D_IMAGE;
  node->l2dimage = List2DImage_new();
  node->l2dimage->mesh = g_VID.drw_meshquad;
  node->l2dimage->texid = texid;
  node->renderflag |= RENDER_NOVERTEXCOLOR;
  node->scale[0]=32;
  node->scale[1]=32;

  node->reference = Reference_new(LUXI_CLASS_L2D_IMAGE,node);

  return node;
}


List2DNode_t *List2DNode_newL3DNode(const char *name, List3DNode_t *l3dnode)
{
  List2DNode_t *node;
  int i;
  if (!l3dnode || ( l3dnode->nodeType != LUXI_CLASS_L3D_MODEL
          &&  l3dnode->nodeType != LUXI_CLASS_L3D_PRIMITIVE
          &&  l3dnode->nodeType != LUXI_CLASS_L3D_TRAIL))
    return NULL;


  node = List2DNode_new(name);
  node->type = LUXI_CLASS_L2D_L3DLINK;
  node->l3dnode = l3dnode;

  for (i = 0; i < l3dnode->numDrawNodes; i++)
    l3dnode->drawNodes[i].renderscale = node->scale;

  node->reference = Reference_new(LUXI_CLASS_L2D_L3DLINK,node);

  Reference_ref(l3dnode->reference);

  List3DNode_unlink(l3dnode);


  return node;
}

List2DNode_t *List2DNode_newText(const char *name, char *text)
{
  List2DNode_t *node = List2DNode_new(name);
  node->type = LUXI_CLASS_L2D_TEXT;
  node->l2dtext = List2DText_new(text);
  node->renderflag = PText_getDefaultRenderFlag();
  node->blend.blendmode = VID_DECAL;

  node->reference = Reference_new(LUXI_CLASS_L2D_TEXT,node);

  return node;
}

List2DNode_t *List2DNode_newL3DView(const char *name, List3DView_t *lview)
{
  List2DNode_t *node;

  if (lview->isdefault || lview->list)
    return NULL;

  node = List2DNode_new(name);
  node->type = LUXI_CLASS_L2D_L3DVIEW;
  node->l2dview = lxMemGenZalloc(sizeof(List2DView_t));
  node->l2dview->viewref = lview->reference;
  node->scale[0]=32;
  node->scale[1]=32;

  node->reference = Reference_new(LUXI_CLASS_L2D_L3DVIEW,node);

  Reference_ref(lview->reference);
  return node;
}

static void List2DNode_free(List2DNode_t* node)
{
  List2DNode_t *browse;

#ifdef _DEBUG
  static List2DNode_t *prev=NULL;
  if (prev==node && node != l_List2D.root){
    LUX_PRINTF("List2DNode_free SELF FREE: %p %s %p %p\n",node,node->name,node->parent,prev);
  }
  prev = node;
#endif


  Reference_invalidate(node->reference);
  node->reference = NULL;

  switch(node->type) {
  case LUXI_CLASS_L2D_L3DLINK:

    Reference_releasePtr(node->l3dnode,reference);
    break;
  case LUXI_CLASS_L2D_L3DVIEW:

    Reference_releaseCheck(node->l2dview->sunref);
    Reference_releaseCheck(node->l2dview->viewref);
    break;
  case LUXI_CLASS_L2D_TEXT:
    List2DText_free(node->l2dtext);
    break;
  case LUXI_CLASS_L2D_IMAGE:
    List2DImage_free(node->l2dimage);
    break;
  case LUXI_CLASS_L2D_FLAG:
    List2DFlag_free(node->l2dflag);
    break;
  default:
    break;
  } // I would prefer that the l3d node is not freed, instead of that
    // the user should decide that - zet
  if (node->name)
    genfreestr(node->name);


  List2DNode_link(node,NULL);

  while(node->childrenListHead){
    lxListNode_popFront(node->childrenListHead,browse);
    browse->parent = NULL;

    Reference_release(browse->reference);
  }

  genfreeSIMD(node,sizeof(List2DNode_t));
}
void RList2DNode_free(lxRl2dnode ref)
{
  List2DNode_free((List2DNode_t*)Reference_value(ref));
}

void List2DNode_setSortKey(List2DNode_t *node, int sortkey)
{
  if (node->sortkey == sortkey) return;
  if (node->parent)
    node->parent->sortchildren = LUX_TRUE;

  node->sortkey = sortkey;
}

void List2DNode_updateFinalMatrix_recursive(List2DNode_t *node)
{
  if (node->parent && node->parent != l_List2D.root)
    List2DNode_updateFinalMatrix_recursive(node->parent);

  if (node->dirtymat)
    List2DNode_updateMatrix(node,node->position);
}

static void List2DNode_updateMatrix(List2DNode_t *node, const lxVector3 pos){
  static lxMatrix44SIMD mat;
  lxVector3 trans;

  lxMatrix44IdentitySIMD(node->finalMatrix);
  if (node->rotationcenter[0] || node->rotationcenter[1] || node->rotationcenter[2]){
    // translation + rottrans
    lxVector3Add(trans,pos,node->rotationcenter);
    lxMatrix44SetTranslation(node->finalMatrix,trans);

    // rot
    lxMatrix44IdentitySIMD(mat);
    lxMatrix44FromEulerZYXFast(mat,node->rotation);
    lxMatrix44Multiply1SIMD(node->finalMatrix,mat);

    // inv rottrans
    lxMatrix44IdentitySIMD(mat);
    lxMatrix44SetInvTranslation(mat,node->rotationcenter);
    lxMatrix44Multiply1SIMD(node->finalMatrix,mat);
  }
  else{
    // translation
    lxMatrix44SetTranslation(node->finalMatrix,pos);
    // rotation
    lxMatrix44IdentitySIMD(mat);
    lxMatrix44FromEulerZYXFast(mat,node->rotation);
    lxMatrix44Multiply1SIMD(node->finalMatrix,mat);
  }



  if (node->parent != l_List2D.root)
    lxMatrix44Multiply2SIMD(node->parent->finalMatrix,node->finalMatrix);

  // scale
  lxMatrix44IdentitySIMD(mat);
  lxMatrix44SetScale(mat,node->scale);
  lxMatrix44MultiplySIMD(node->finalScaled,node->finalMatrix,mat);

  node->dirtymat = LUX_FALSE;
}


void List2DNode_link(List2DNode_t *node,List2DNode_t *parent)
{
  List2DNode_t* oldparent = node->parent;
  if (parent == node->parent) return;

  if (oldparent){
    lxListNode_remove(oldparent->childrenListHead,node);
  }

  if (parent){
    lxListNode_addLast(parent->childrenListHead,node);
    if (parent != l_List2D.root){
      if (!oldparent){
        Reference_ref(node->reference);
      }
    }
    parent->sortchildren = LUX_TRUE;
    node->parent = parent;
  }
  else if (oldparent){
    node->parent = NULL;

    Reference_releaseCheck(node->reference);
  }
}



static void List2DNode_updateScissor(List2DNode_t *node, const float *parentscissor)
{
  lxVector4 scmax;

  if (node->localscissor){
    lxVector2Transform(node->finalscissorstart,node->scissorstart,node->finalMatrix);
    lxVector2Copy(node->finalscissorsize,node->scissorsize);
  }
  else{
    lxVector4Copy(node->finalscissorstart,node->scissorstart);
  }

  if (node->parentscissor && parentscissor){
    // cap
    scmax[0] = parentscissor[0]+parentscissor[2];
    scmax[1] = parentscissor[1]+parentscissor[3];
    scmax[2] = node->finalscissorstart[0]+node->finalscissorsize[0];
    scmax[3] = node->finalscissorstart[1]+node->finalscissorsize[1];

    node->finalscissorstart[0] = LUX_MAX(parentscissor[0],node->finalscissorstart[0]);
    node->finalscissorstart[1] = LUX_MAX(parentscissor[1],node->finalscissorstart[1]);

    node->finalscissorsize[0] = LUX_MAX(0,LUX_MIN(scmax[0],scmax[2]) - node->finalscissorstart[0]);
    node->finalscissorsize[1] = LUX_MAX(0,LUX_MIN(scmax[1],scmax[3]) - node->finalscissorstart[1]);
  }

  node->dirtyscissor = LUX_FALSE;
}

List2DNode_t* List2DNode_getRoot()
{
  return l_List2D.root;
}

List2D_t* List2D_get(){
  return &l_List2D;
}

void  List2D_setGlobalscissor(int *scissor)
{
  LUX_ARRAY4COPY(l_List2D.globalscissor,scissor);
}
