// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../resource/resmanager.h"
#include "../render/gl_list2d.h"
#include "../common/reflib.h"
#include "../common/3dmath.h"


// Published here:
// LUXI_CLASS_L2D_LIST
// LUXI_CLASS_L2D_NODE
// LUXI_CLASS_L2D_TEXT
// LUXI_CLASS_L2D_IMAGE
// LUXI_CLASS_L2D_L3DLINK
// LUXI_CLASS_L2D_FLAG
// LUXI_CLASS_L2D_L3DVIEW

//////////////////////////////////////////////////////////////////////////
// L2D


int PubL2D_return(PState pstate,List2DNode_t *l2d)
{
  if (!l2d) return 0;

  return FunctionPublish_returnType(pstate,l2d->type,REF2VOID(l2d->reference));
}

static int PubList2D_free (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List2DNode_t *l2d;

  if (n<1 || 1>FunctionPublish_getArg(pstate,1,LUXI_CLASS_L2D_NODE,(void*)&ref) ||
    !Reference_get(ref,l2d))
    return FunctionPublish_returnError(pstate,"1 l2dnode required");

  Reference_free(ref);//RList2DNode_free(ref);
  return 0;
}

static int PubL2DNode_new(PState pstate,PubFunction_t *f, int n)
{
  List2DNode_t *l2d;
  char *name;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name))
    return FunctionPublish_returnError(pstate,"1 string required");

  l2d = List2DNode_new(name);
  l2d->reference = Reference_new(LUXI_CLASS_L2D_NODE,l2d);
  Reference_makeVolatile(l2d->reference);

  return FunctionPublish_returnType(pstate,LUXI_CLASS_L2D_NODE,REF2VOID(l2d->reference));
}

static int PubList2D_getroot (PState pstate,PubFunction_t *f, int n)
{
  return PubL2D_return(pstate,List2DNode_getRoot());
}

enum PubList2DFuncs_e
{
  PL2D_SORTID,
  PL2D_COLOR,
  PL2D_SCISSOR,
  PL2D_SCISSORSTART,
  PL2D_SCISSORSIZE,
  PL2D_SCISSORLOCAL,
  PL2D_SCISSORPARENT,
  PL2D_LINK,
  PL2D_SWAPCHILDS,
  PL2D_FIRSTCHILD,

  PL2D_2WORLD,
  PL2D_2LOCAL,

  // PRS
  PL2D_POS,
  PL2D_ROTRAD,
  PL2D_ROTDEG,
  PL2D_ROTCENTER,
  PL2D_SCALE,
};

static int PubList2D_prs (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List2DNode_t *l2d;
  lxVector3 vector;
  float *target;

  if ((n!=1 && n!=4) ||
    !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L2D_NODE,(void*)&ref)    ||
    !Reference_get(ref,l2d))
    return FunctionPublish_returnError(pstate,"1 valid l2dnode required, optional 1 vector or 3 floats");

  switch((int)f->upvalue) {
  case PL2D_POS:
    target = l2d->position;
    break;
  case PL2D_ROTRAD:
    target = l2d->rotation;
    break;
  case PL2D_SCALE:
    target = l2d->scale;
    break;
  case PL2D_ROTCENTER:
    target = l2d->rotationcenter;
    break;
  case PL2D_ROTDEG:
    lxVector3Rad2Deg(vector,l2d->rotation);
    target = vector;
    break;
  default:
    return 0;
  }

  switch(n) {
  case 1:
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(target));
  case 4:
    if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(target)) != 3)
      return FunctionPublish_returnError(pstate,"3 numbers required");

    l2d->dirtymat = LUX_TRUE;

    if ((int)f->upvalue == PL2D_ROTDEG)
      lxVector3Deg2Rad(l2d->rotation,vector);
    return 0;
  }
  return 0;
}



static int PubList2D_attributes (PState pstate,PubFunction_t *f, int n)
{
  int sort;
  lxVector4 vec;
  Reference ref;
  List2DNode_t *l2d;
  List2DNode_t *l2d2;
  List2DNode_t *pTemp;

  if ((n<1)|| !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L2D_NODE,(void*)&ref) ||
    !Reference_get(ref,l2d)){
    return FunctionPublish_returnError(pstate,"1 l2dnode required");
  }

  switch((int)f->upvalue) {
  case PL2D_SORTID: // sortid
    if (n==1) return FunctionPublish_returnInt(pstate,l2d->sortkey);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_INT,(void*)&sort) )
      return FunctionPublish_returnError(pstate,"1 l2dnode required, optional 1 int ");
    if (sort != l2d->sortkey)
      List2DNode_setSortKey(l2d,sort);
    break;
  case PL2D_COLOR:
    if (n==1) return FunctionPublish_setRet(pstate,4,FNPUB_FROMVECTOR4(l2d->color));
    if (FunctionPublish_getArgOffset(pstate,1,4,FNPUB_TOVECTOR4(l2d->color))!=4)
      return FunctionPublish_returnError(pstate,"1 l2dnode required, optional 4 floats");
    break;
  case PL2D_SCISSORSIZE:
    if (n==1) return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR2(l2d->scissorsize));
    if (FunctionPublish_getArgOffset(pstate,1,2,FNPUB_TOVECTOR2(l2d->scissorsize))!=2)
      return FunctionPublish_returnError(pstate,"1 l2dnode required, optional 2 floats");
    l2d->dirtyscissor = LUX_TRUE;
    break;
  case PL2D_SCISSORSTART:
    if (n==1) return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR2(l2d->scissorstart));
    if (FunctionPublish_getArgOffset(pstate,1,2,FNPUB_TOVECTOR2(l2d->scissorstart))!=2)
      return FunctionPublish_returnError(pstate,"1 l2dnode required, optional 2 floats");
    l2d->dirtyscissor = LUX_TRUE;
    break;
  case PL2D_SCISSOR:
    if (n==1) return FunctionPublish_returnBool(pstate,l2d->usescissor);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&l2d->usescissor))
      return FunctionPublish_returnError(pstate,"1 l2dnode required, optional 1 boolean");
    break;
  case PL2D_SCISSORLOCAL:
    if (n==1) return FunctionPublish_returnBool(pstate,l2d->localscissor);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&l2d->localscissor))
      return FunctionPublish_returnError(pstate,"1 l2dnode required, optional 1 boolean");
    break;
  case PL2D_SCISSORPARENT:
    if (n==1) return FunctionPublish_returnBool(pstate,l2d->parentscissor);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_BOOLEAN,(void*)&l2d->parentscissor))
      return FunctionPublish_returnError(pstate,"1 l2dnode required, optional 1 boolean");
    break;
  case PL2D_LINK:
    if (n==1) {
      return l2d->parent ? PubL2D_return(pstate,l2d->parent) : 0;
    }
    l2d2 = NULL;
    if (FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L2D_NODE,(void*)&ref) && (!Reference_get(ref,l2d2) || l2d==l2d2))
      return FunctionPublish_returnError(pstate,"1 l2dnode required, optional 1 l2dnode/boolean. Parent must not be self");
    List2DNode_link(l2d,l2d2);
    break;
  case PL2D_SWAPCHILDS:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L2D_NODE,(void*)&ref) || !Reference_get(ref,l2d2))
      return FunctionPublish_returnError(pstate,"1 l2dnode required, optional 1 l2dnode boolean");
    if (l2d2->parent != l2d->parent)
      return FunctionPublish_returnError(pstate,"l2dnodes must have same parent");

    pTemp = l2d2->next;
    l2d2->next = l2d->next;
    l2d->next = pTemp;

    pTemp = l2d2->prev;
    l2d2->prev = l2d->prev;
    l2d->prev = pTemp;
    break;
  case PL2D_FIRSTCHILD:
    if (n==1) {
      return PubL2D_return(pstate,l2d->childrenListHead);
    }

    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L2D_NODE,(void*)&ref) || !Reference_get(ref,l2d2))
      return FunctionPublish_returnError(pstate,"1 l2dnode required, optional 1 l2dnode boolean");

    if (l2d2->parent != l2d)
      return FunctionPublish_returnError(pstate,"second l2dnode must be child of first");

    l2d->childrenListHead = l2d2;
    break;
  case PL2D_2WORLD:
  case PL2D_2LOCAL:
    if (n < 4) return FunctionPublish_returnError(pstate,"1 l2dnode 3 floats required");
    if (FunctionPublish_getArgOffset(pstate,1,3,FNPUB_TOVECTOR3(vec))!=3)
      return FunctionPublish_returnError(pstate,"1 l2dnode 3 floats required");

    List2DNode_updateFinalMatrix_recursive(l2d);

    if ((int)f->upvalue == PL2D_2LOCAL){
      lxVector3Transform1(vec,l2d->finalMatrix);
    }
    else{
      lxVector3InvTransform1(vec,l2d->finalMatrix);
    }

    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vec));
  default:
    break;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// L2D TEXT
static int PubL2DText_new (PState pstate,PubFunction_t *f, int n)
{
  char *name;
  char *text;
  List2DNode_t * l2d;
  Reference ref;

  if (n<2|| FunctionPublish_getArg(pstate,2,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_STRING,(void*)&text)!=2)
    return FunctionPublish_returnError(pstate,"2 string required");


  l2d = List2DNode_newText(name,text);

  if (n==3 && FunctionPublish_getNArg(pstate,2,LUXI_CLASS_FONTSET,(void*)&ref) && Reference_isValid(ref))
        PText_setFontSetRef (&l2d->l2dtext->ptext,ref);

  Reference_makeVolatile(l2d->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_L2D_TEXT,REF2VOID(l2d->reference));
}

enum PubL2DTextCmds_e
{
  PL2DTEXT_FS,
  PL2DTEXT_TEX,
  PL2DTEXT_SPACING,
  PL2DTEXT_TABWIDTH,
  PL2DTEXT_SIZE,
  PL2DTEXT_TEXT,
  PL2DTEXT_DIM,
  PL2DTEXT_CHARAT,
  PL2DTEXT_POSAT,
};

static int PubL2DText_attributes (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  char *name;
  List2DNode_t *l2d;
  lxVector3 vec;

  if ((n == 0)|| !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L2D_TEXT,(void*)&ref) ||
    !Reference_get(ref,l2d))
    return FunctionPublish_returnError(pstate,"1 l2dtext required");

  switch((int)f->upvalue) {
  case PL2DTEXT_FS: // fontset
    if (n==1) return FunctionPublish_returnType(pstate,LUXI_CLASS_FONTSET,REF2VOID(l2d->l2dtext->ptext.fontsetref));
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_FONTSET,(void*)&ref) || !Reference_isValid(ref))
      return FunctionPublish_returnError(pstate,"1 l2dtext required, optional 1 fontset");

    PText_setFontSetRef(&l2d->l2dtext->ptext,ref);
    break;
  case PL2DTEXT_TEX:  // texture
    if (n==1) return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)l2d->l2dtext->ptext.texRID);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_TEXTURE,(void*)&l2d->l2dtext->ptext.texRID))
      return FunctionPublish_returnError(pstate,"1 l2dtext required, optional 1 texture");
    break;
  case PL2DTEXT_SPACING:  // spacing
    if (n==1) return FunctionPublish_returnFloat(pstate,l2d->l2dtext->ptext.width);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(l2d->l2dtext->ptext.width)))
      return FunctionPublish_returnError(pstate,"1 l2dtext required, optional 1 float");
    break;
  case PL2DTEXT_TABWIDTH: // tabwidth
    if (n==1) return FunctionPublish_returnFloat(pstate,l2d->l2dtext->ptext.tabwidth);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(l2d->l2dtext->ptext.tabwidth)))
      return FunctionPublish_returnError(pstate,"1 l2dtext required, optional 1 float");
    break;
  case PL2DTEXT_SIZE: // size
    if (n==1) return FunctionPublish_returnFloat(pstate,l2d->l2dtext->ptext.size);
    if (!FunctionPublish_getNArg(pstate,1,FNPUB_TFLOAT(l2d->l2dtext->ptext.size)))
      return FunctionPublish_returnError(pstate,"1 l2dtext required, optional 1 float");
    break;
  case PL2DTEXT_TEXT: // text
    if (n==1)
      return FunctionPublish_returnString(pstate,l2d->l2dtext->ptext.text);
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_STRING,(void*)&name))
      return FunctionPublish_returnError(pstate,"1 l2dtext required, optional 1 string");
    PText_setText(&l2d->l2dtext->ptext,name);
    break;
  case PL2DTEXT_DIM: // dimension
    PText_getDimensions(&l2d->l2dtext->ptext,NULL,vec);
    return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(vec));
  case PL2DTEXT_CHARAT: {
    int suc,pos;
    FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_FLOAT,vec[1]);
    FNPUB_CHECKOUT(pstate,n,2,LUXI_CLASS_FLOAT,vec[2]);
    suc = PText_getCharindexAt(&l2d->l2dtext->ptext,vec[1], vec[2], &pos);
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_BOOLEAN,suc,LUXI_CLASS_INT,pos);
  }
  case PL2DTEXT_POSAT: {
    int pos;
    FNPUB_CHECKOUT(pstate,n,1,LUXI_CLASS_INT,pos);
    PText_getCharPoint(&l2d->l2dtext->ptext, pos,vec);
    return FunctionPublish_setRet(pstate,2,FNPUB_FROMVECTOR3(vec));
  }
  default:
    break;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// L2D IMAGE

static int PubL2DImage_new (PState pstate,PubFunction_t *f, int n)
{
  int style;
  char *name;
  List2DNode_t * l2d;

  style = -1;
  if (n<2|| FunctionPublish_getArg(pstate,2,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_MATSURF,(void*)&style)<2)
    return FunctionPublish_returnError(pstate,"1 string 1 matsurface required");

  l2d = List2DNode_newImage(name,style);
  Reference_makeVolatile(l2d->reference);

  return FunctionPublish_returnType(pstate,LUXI_CLASS_L2D_IMAGE,REF2VOID(l2d->reference));
}



enum L2DImageProps_e{
  L2DIMAGE_FULLSCREEN,
  L2DIMAGE_USERMESH,
  L2DIMAGE_QUADMESH,
  L2DIMAGE_QUADMESHCENTERED,
  L2DIMAGE_RENDERMESH,
};


static int PubL2DImage_prop (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  List2DNode_t *l2d;
  List2DImage_t *l2dimage;

  if (n<1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L2D_IMAGE,(void*)&ref) || !Reference_get(ref,l2d))
    return FunctionPublish_returnError(pstate,"1 l2dimage required");

  l2dimage = l2d->l2dimage;

  switch((int)f->upvalue) {
  case L2DIMAGE_FULLSCREEN:
    l2d->position[0]=0;
    l2d->position[1]=0;
    l2d->scale[0]=VID_REF_WIDTH;
    l2d->scale[1]=VID_REF_HEIGHT;
    l2d->dirtymat = LUX_TRUE;
    break;
  case L2DIMAGE_QUADMESH:
    Reference_releaseClear(l2dimage->usermesh);
    l2dimage->mesh = g_VID.drw_meshquad;
    break;
  case L2DIMAGE_QUADMESHCENTERED:
    Reference_releaseClear(l2dimage->usermesh);
    l2dimage->mesh = g_VID.drw_meshquadcentered;
    break;
  case L2DIMAGE_USERMESH:
    return PubUserMesh(pstate,1,&l2dimage->mesh,
      &l2dimage->usermesh);
  case L2DIMAGE_RENDERMESH:
    return PubRenderMesh(pstate,1,&l2dimage->mesh,
      &l2dimage->usermesh);
  default:
    break;
  }


  return 0;
}

static int PubL2DImage_material (PState pstate,PubFunction_t *f, int n)
{
  int material;
  Reference ref;
  List2DNode_t *l2d;

  if ((n!=1 && n!=2)|| !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L2D_IMAGE,(void*)&ref) ||
    !Reference_get(ref,l2d))
    return FunctionPublish_returnError(pstate,"1 l2dimage required, optional 1 material");

  switch(n) {
  case 1:
    material = l2d->l2dimage->texid;
    if (vidMaterial(material))
      return FunctionPublish_returnType(pstate,LUXI_CLASS_MATERIAL,(void*)material);
    else if (material >= 0)
      return FunctionPublish_returnType(pstate,LUXI_CLASS_TEXTURE,(void*)material);
    else
      return 0;
    break;
  case 2:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_MATSURF,(void*)&material))
      return FunctionPublish_returnError(pstate,"1 l2dimage required, optional 1 matsurface");
    l2d->l2dimage->texid = material;
    if (l2d->l2dimage->matobj)
      MaterialObject_free(l2d->l2dimage->matobj);
    l2d->l2dimage->matobj = NULL;
    break;
  default:
    break;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// L2D L3D LINK
static int PubL3DLinkL3D_new (PState pstate,PubFunction_t *f, int n)
{
  char *name;
  Reference refl3d;
  List2DNode_t * l2d;
  List3DNode_t * l3d;

  if (n!=2|| FunctionPublish_getArg(pstate,2,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_L3D_NODE,(void*)&refl3d)!=2 ||
    !Reference_get(refl3d,l3d))
    return FunctionPublish_returnError(pstate,"1 string 1 l3dnode required");

  l2d = List2DNode_newL3DNode(name,l3d);

  if (!l2d)
    return FunctionPublish_returnError(pstate,"only l3dmodel, l3dprimitive, l3dtrail can become l2dnodes");

  Reference_makeVolatile(l2d->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_L2D_L3DLINK,REF2VOID(l2d->reference));
}
static int PubL3DLinkL3D_node (PState pstate,PubFunction_t *f, int n)
{
  Reference ref;
  Reference refl3d;
  List2DNode_t *l2d;
  List3DNode_t *l3d;

  if ((n!=1 && n!=2)|| !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L2D_L3DLINK,(void*)&ref) ||
    !Reference_get(ref,l2d))
    return FunctionPublish_returnError(pstate,"1 l2dnode3d required, optional 1 l3dnode");

  switch(n) {
  case 1:
    return FunctionPublish_returnType(pstate,l2d->l3dnode->nodeType,REF2VOID(l2d->l3dnode->reference));
    break;
  case 2:
    if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_NODE,(void*)&refl3d) || !Reference_get(refl3d,l3d))
      return FunctionPublish_returnError(pstate,"1 l2dnode3d required, optional 1 l3dnode");

    if (l3d->nodeType != LUXI_CLASS_L3D_MODEL && l3d->nodeType != LUXI_CLASS_L3D_PRIMITIVE && l3d->nodeType != LUXI_CLASS_L3D_TRAIL)
      return FunctionPublish_returnError(pstate,"only l3dmodel,l3dprimitive and l3dtrail can become l2dnodes");

    Reference_ref(l3d->reference);

    List3DNode_unlink(l3d);

    Reference_releasePtr(l2d->l3dnode,reference);
    l2d->l3dnode = l3d;
    l2d->type = l3d->nodeType;

    break;
  default:
    break;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_L2D_L3DVIEW

enum PubL2DView_e{
  PLVIEW_NEW,

  PLVIEW_VIEW,
  PLVIEW_SUN,
};

static int PubL3DLinkL3D_funcs (PState pstate,PubFunction_t *f, int n)
{
  char *name;
  Reference ref2d;
  Reference ref3d;
  List2DNode_t * l2d;
  List3DView_t * l3dview;
  List3DNode_t * l3d;

  if ((int)f->upvalue > PLVIEW_NEW){
    if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_L2D_L3DVIEW,(void*)&ref2d) || !Reference_get(ref2d,l2d))
      return FunctionPublish_returnError(pstate,"1 l2dview3d required");
  }

  switch((int)f->upvalue)
  {
  case PLVIEW_NEW:
    if (n!=2|| FunctionPublish_getArg(pstate,2,LUXI_CLASS_STRING,(void*)&name,LUXI_CLASS_L3D_VIEW,(void*)&ref3d)!=2 ||
      !Reference_get(ref3d,l3dview) )
      return FunctionPublish_returnError(pstate,"1 string 1 l3dview required");

    l2d = List2DNode_newL3DView(name,l3dview);
    if (!l2d)
      return FunctionPublish_returnError(pstate,"only unlinked l3dview allowed");

    return FunctionPublish_returnType(pstate,LUXI_CLASS_L2D_L3DLINK,REF2VOID(l2d->reference));

  case PLVIEW_VIEW:
    if (n==1) return FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_VIEW,REF2VOID(l2d->l2dview->viewref));
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_VIEW,(void*)&ref3d) || !Reference_get(ref3d,l3dview) ||
      l3dview->isdefault || l3dview->list)
      return FunctionPublish_returnError(pstate,"1 l2dview3d 1 l3dview (unlinked) required");


    Reference_releaseCheck(l2d->l2dview->viewref);
    Reference_ref(l3dview->reference);

    l2d->l2dview->viewref = l3dview->reference;
    return 0;

  case PLVIEW_SUN:
    if (n==1) return l2d->l2dview->sunref ? FunctionPublish_returnType(pstate,LUXI_CLASS_L3D_LIGHT,REF2VOID(l2d->l2dview->sunref)) : 0;
    else if (!FunctionPublish_getNArg(pstate,1,LUXI_CLASS_L3D_LIGHT,(void*)&ref3d) || !Reference_get(ref3d,l3d))
      return FunctionPublish_returnError(pstate,"1 l2dview3d 1 l3dlight required");

    Reference_releaseCheck(l2d->l2dview->sunref);
    Reference_ref(l3d->reference);

    l2d->l2dview->sunref = l3d->reference;
    return 0;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// LUXI_CLASS_L2D_FLAG

static int PubL2DFlag_new(PState pstate,PubFunction_t *f,int n)
{
  List2DNode_t *l2d;
  char *name;

  if (n < 1 || !FunctionPublish_getNArg(pstate,0,LUXI_CLASS_STRING,(void*)&name))
    return FunctionPublish_returnError(pstate,"1 string required");

  l2d = List2DNode_newFlag(name);

  Reference_makeVolatile(l2d->reference);
  return FunctionPublish_returnType(pstate,LUXI_CLASS_L2D_FLAG,REF2VOID(l2d->reference));
}

enum PubL2DList_Funcs_e{
  PL2DLIST_CLEAR,
};

static int PubList2D_listfuncs (PState pstate,PubFunction_t *fn, int n)
{
  switch((int)fn->upvalue)
  {
  case PL2DLIST_CLEAR:
    return FunctionPublish_returnType(pstate,LUXI_CLASS_RCMD_CLEAR,REF2VOID(List2D_get()->cmdclear->cmd.reference));
  }
  return 0;
}


void PubClass_List2D_init()
{
  FunctionPublish_initClass(LUXI_CLASS_L2D_LIST,"l2dlist",
    "The List2D is drawn after the List3D and orthogonally. It is mainly used for HUD rendering text and buttons.\
    The origin 0,0 is top left, the width and height is independently of the current resolution, but defined in window.refsize. New nodes start unlinked and wont be drawn, unless linked to root or another root child. Drawing of l2dnodes is also possible via the rcmddrawl2d part of l3dview rendercommand system."
    ,NULL,LUX_TRUE);
  FunctionPublish_addFunction(LUXI_CLASS_L2D_LIST,PubList2D_listfuncs,(void*)PL2DLIST_CLEAR,"getrcmdclear",
    "(rcmdclear):() - returns rcmdclear of the List2D.");
  FunctionPublish_setFunctionInherition(LUXI_CLASS_L2D_LIST,"getrcmdclear",LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_L2D_LIST,PubList2D_getroot,(void*)NULL,"getroot",
    "(l2dnode):() - returns l2dlist master root node. Root doesn't inherit its position info.");
  FunctionPublish_setFunctionInherition(LUXI_CLASS_L2D_LIST,"getroot",LUX_FALSE);

  FunctionPublish_initClass(LUXI_CLASS_L2D_NODE,"l2dnode","A node meant for 2d drawing"
    ,NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L2D_NODE,LUXI_CLASS_L2D_LIST);
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_free,NULL,"delete",
    "():(l2dnode) - deletes node.");

  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_SORTID,"sortid",
    "([int]):(l2dnode,[int]) - returns or sets l2dnode's sortid (lower gets rendered first). Setting a value will result into the parent's children list being resorted.");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_COLOR,"color",
    "([float r,g,b,a]):(l2dnode,[float r,g,b,a]) - returns or sets l2dnode's color");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_SCISSOR,"scissor",
    "([boolean]):(l2dnode,[boolean]) - returns or sets if scissoring should be used. If scissoring is enabled only pixels within scissor rectangle are drawn. Makes sure proper scissorsize and scissorstart are set.");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_SCISSORSIZE,"scissorsize",
    "([float x,y]):(l2dnode,[float x,y]) - returns or sets scissor rectangle size. ");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_SCISSORSTART,"scissorstart",
    "([float x,y]):(l2dnode,[float x,y]) - returns or sets scissor rectangle startpoint. ");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_SCISSORPARENT,"scissorparent",
    "([boolean]):(l2dnode,[boolean]) - returns or sets if parent's scissor information is used as well. If own node has no scissor set we will use parents, else we cap own with parent's ");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_SCISSORLOCAL,"scissorlocal",
    "([boolean]):(l2dnode,[boolean]) - returns or sets if own scissorstart is transformed with the node's matrix. The size is not transformed.");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_LINK,"parent",
    "([l2dnode]):(l2dnode,[l2dnode parent]) - returns or sets parent node. Passing a non l2dnode will unlink from parent. If linked will inherit transforms of parents and also won't be drawn if parent is not drawn. ==parent prevents gc of self, unless parent is root.");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_SWAPCHILDS,"swapinchildlist",
    "():(l2dnode,l2dnode) - swaps the children within the parent's childlist.");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_FIRSTCHILD,"firstchild",
    "([l2dnode]):(l2dnode,[l2dnode]) - returns or sets first child of the children list. First child is rendered first. Useful for manual sorting. ");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_prs,(void*)PL2D_POS,"pos",
    "([float x,y,z]):(l2dnode,[float x,y,z]) - returns or sets l2dnode's position");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_prs,(void*)PL2D_ROTDEG,"rotdeg",
    "([float x,y,z]):(l2dnode,[float x,y,z]) - returns or sets l2dnode's rotation in degrees");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_prs,(void*)PL2D_ROTRAD,"rotrad",
    "([float x,y,z]):(l2dnode,[float x,y,z]) - returns or sets l2dnode's rotation in radians");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_prs,(void*)PL2D_SCALE,"scale",
    "([float x,y,z]):(l2dnode,[float x,y,z]) - returns or sets l2dnode's scale");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_prs,(void*)PL2D_ROTCENTER,"rotcenter",
    "([float x,y,z]):(l2dnode,[float x,y,z]) - returns or sets l2dnode's center of rotation");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_2WORLD,"local2world",
    "(float x,y,z):(l2dnode,float x,y,z) - returns coordinates after transforms. l3dtargets will not be taken in account.");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubList2D_attributes,(void*)PL2D_2LOCAL,"world2local",
    "(float x,y,z):(l2dnode,float x,y,z) - returns coordinates in localspace. l3dtargets will not be taken in account.");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_NODE,PubL2DNode_new,NULL,"new",
    "(l2dnode):(string name)  - returns a new l2dnode, that can be used for hierarchy and organisation");
  FunctionPublish_setFunctionInherition(LUXI_CLASS_L2D_NODE,"new",LUX_FALSE);

  FunctionPublish_initClass(LUXI_CLASS_L2D_TEXT,"l2dtext",
    "Displays text on the screen. The Text can be formatted in different ways "
    "in order to change its color or textposition: \n"
    "* \\vrgb - where r,g,b is replaced by a number between 0 and 9:  "
    "The textprint color is replaced by the specified color. For example, "
    "this given string would print out different colors: "
    "\"\\v900red\\v090green\\v009blue\\v909mangenta\\v990yellow\"\n"
    "* \\vc - resets the color to the original color value\n"
    "* \\n - starts a new line\n"
    "* \\t - inserts a tab character\n"
    "* \\vxn; - replace n with a number that specifies an absolute distance in pixels from the left boundary\n"
    "* \\vR - aligns the current line to the RIGHT. The widest line of the printed text sets the total width\n"
    "* \\vC - aligns the current line centered, works as \\vR\n"
    "* \\vs - puts a shadow below the text, used to improve readability. Note that the used font texture can also implement a shadow.\n",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L2D_TEXT,LUXI_CLASS_L2D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L2D_TEXT,PubL2DText_new,NULL,"new",
    "(l2dtext):(string name,string text,[fontset])  - returns a new l2dtext");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_TEXT,PubL2DText_attributes,(void*)PL2DTEXT_FS,"fontset",
    "(fontset):(l2dtext,[fontset])  - returns or sets fontset.");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_TEXT,PubL2DText_attributes,(void*)PL2DTEXT_TEX,"font",
    "(texture):(l2dtext,[texture])  - returns or sets font texture");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_TEXT,PubL2DText_attributes,(void*)PL2DTEXT_SPACING,"spacing",
    "(float):(l2dtext,[float])  - returns or sets font spacing, default is 16");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_TEXT,PubL2DText_attributes,(void*)PL2DTEXT_TABWIDTH,"tabwidth",
    "(float):(l2dtext,[float])  - returns or sets tab width spacing, default is 0. 0 means spacing * 4 is used, otherwise values will be directly applied.");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_TEXT,PubL2DText_attributes,(void*)PL2DTEXT_SIZE,"size",
    "(float):(l2dtext,[float])  - returns or sets font size, default is 16");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_TEXT,PubL2DText_attributes,(void*)PL2DTEXT_TEXT,"text",
    "(string):(l2dtext,[string])  - returns or sets text");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_TEXT,PubL2DText_attributes,(void*)PL2DTEXT_DIM,"dimensions",
    "(float x,y,z):(l2dtext)  - returns dimension of space it would take when printed");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_TEXT,PubL2DText_attributes,(void*)PL2DTEXT_CHARAT,"charatpos",
    "(boolean inside, int charpos):(l2dtext, float x, float y)  - returns character index at given x,y. Returns true / false wether it x,y was inside or not.");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_TEXT,PubL2DText_attributes,(void*)PL2DTEXT_POSAT,"posatchar",
    "(float x,y):(l2dtext, int pos)  - returns the character's position offset. Behaviour undefined if pos>length of text.");


  FunctionPublish_initClass(LUXI_CLASS_L2D_IMAGE,"l2dimage","a quad with the given material (or plain color) will be rendered on screen.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L2D_IMAGE,LUXI_CLASS_L2D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L2D_IMAGE,PubL2DImage_new,NULL,"new",
    "(l2dimage):(string name,matsurface)  - returns a new l2dimage");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_IMAGE,PubL2DImage_prop,(void*)L2DIMAGE_FULLSCREEN,"fullscreen",
    "():(l2dimage)  - sets pos and scale for fullscreen");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_IMAGE,PubL2DImage_material,NULL,"matsurface",
    "([material/texture]):(l2dimage,[matsurface])  - returns or sets matsurface");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_IMAGE,PubL2DImage_prop,(void*)L2DIMAGE_QUADMESH,"quadmesh",
    "():(l2dimage)  - unrefs the rendermesh and uses the quadmesh again (default).");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_IMAGE,PubL2DImage_prop,(void*)L2DIMAGE_QUADMESHCENTERED,"quadcenteredmesh",
    "():(l2dimage)  - deletes the usermesh and uses the centered quadmesh again.");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_IMAGE,PubL2DImage_prop,(void*)L2DIMAGE_USERMESH,"usermesh",
    "():(l2dimage, vertextype, int numverts, numindices, [vidbuffer vbo], [int vbooffset], [vidbuffer ibo], [int ibooffset]) - creates inplace custom rendermesh (see rendermesh for details) Polygonwinding is CW (not CCW like the others!).");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_IMAGE,PubL2DImage_prop,(void*)L2DIMAGE_RENDERMESH,
    "rendermesh","([rendermesh]):(l2dimage,[rendermesh]) - gets or sets rendermesh. Get only works if a usermesh was created before or another rendermesh passed for useage.");

  FunctionPublish_initClass(LUXI_CLASS_L2D_L3DLINK,"l2dnode3d","list2d node that links to a l3d node (l3dmodel,l3dprimitive,l3dtrail). ==The l3dnode is prevented from gc, as long as l2dnode is not gc'ed.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L2D_L3DLINK,LUXI_CLASS_L2D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L2D_L3DLINK,PubL3DLinkL3D_new,NULL,"new",
    "(l2dnode3d):(string name,l3dnode)  - returns a new l2dlinkedl3d");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_L3DLINK,PubL3DLinkL3D_node,NULL,"l3d",
    "([l3dnode]):(l2dnode3d,[l3dnode])  - returns or sets l3dnode");

  FunctionPublish_initClass(LUXI_CLASS_L2D_L3DVIEW,"l2dview3d","list2d node that executes a direct l3dview draw (see l3dview.drawnow limitations). Never use recursive processing of l2d and l3d viewing, i.e. it will cause crashes if the linked l3dview contains an rcmddrawl2d. ==The l3dview (optional l3dlight) is prevented from gc, as long as l2dnode is not gc'ed.",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L2D_L3DVIEW,LUXI_CLASS_L2D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L2D_L3DVIEW,PubL3DLinkL3D_new,NULL,"new",
    "(l2dview3d):(string name,l3dview)  - returns a new l2dview3d");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_L3DVIEW,PubL3DLinkL3D_node,NULL,"view",
    "([l3dview]):(l2dnode3d,[l3dview])  - returns or sets l3dview");
  FunctionPublish_addFunction(LUXI_CLASS_L2D_L3DVIEW,PubL3DLinkL3D_node,NULL,"sun",
    "([l3dlight]):(l2dnode3d,[l3dlight])  - returns or sets l3dlight used as sun");

  FunctionPublish_initClass(LUXI_CLASS_L2D_FLAG,"l2dflag","list2d node that does only set some rendering parameters",NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_L2D_FLAG,LUXI_CLASS_L2D_NODE);
  FunctionPublish_addFunction(LUXI_CLASS_L2D_FLAG,PubL2DFlag_new,NULL,"new",
    "(l2dflag):(string name) - returns a new l2dflag");

}
