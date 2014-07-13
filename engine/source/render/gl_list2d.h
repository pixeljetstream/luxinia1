// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_LIST2D_H__
#define __GL_LIST2D_H__

/*
List2D
------

Similar to List3D just for orthographic stuff, like text, images...

Author: Christoph Kubisch

*/

#include <luxinia/luxcore/contmacrolinkedlist.h>
#include "../common/types.h"
#include "../common/reflib.h"
#include "gl_list3d.h"
#include "gl_print.h"

typedef struct List2DImage_s{
  int         texid;
  Mesh_t        *mesh;
  MaterialObject_t  *matobj;
  Reference     usermesh;
}List2DImage_t;

typedef struct List2DText_s{
  PText_t       ptext;
  // order must not change
  lxVector2       localscissorstart;
  lxVector2       localscissorsize;
}List2DText_t;

typedef struct List2DFlag_s
{
  VIDStencil_t    stencil;
}List2DFlag_t;

typedef struct List2DView_s{
  Reference     sunref;
  Reference     viewref;
}List2DView_t;

typedef struct List2DNode_s
{
  // 2 DW
  lxClassType     type;
  char        *name;

  // 2 DW
  uchar       usescissor;
  uchar       parentscissor;
  uchar       localscissor;
  uchar       dirtymat;
  short       dirtyscissor;
  short       sortchildren;

  // 32 DW - 4 DW aligned
  lxMatrix44      finalMatrix;
  lxMatrix44      finalScaled;

  // 12 DW
  lxVector3       position;
  lxVector3       scale;
  lxVector3       rotation;
  lxVector3       rotationcenter;

  // order must not change
  lxVector2       scissorstart;
  lxVector2       scissorsize;

  lxVector2       finalscissorstart;
  lxVector2       finalscissorsize;

  union{
    List2DText_t  *l2dtext;
    List2DImage_t *l2dimage;
    List3DNode_t  *l3dnode;
    List2DFlag_t  *l2dflag;
    List2DView_t  *l2dview;
  };


  enum32        renderflag;
  VIDBlend_t      blend;
  VIDAlpha_t      alpha;
  VIDLine_t     line;
  lxVector4       color;

  Reference     reference;
  int         sortkey;

  List3DNode_t    *l3dtarget;
  Reference     l3dtargetref;

  struct List2DNode_s *parent;
  struct List2DNode_s *childrenListHead;
  struct List2DNode_s LUX_LISTNODEVARS;
} List2DNode_t;

typedef struct List2D_s{
  List2DNode_t  *root;
  RenderCmdClear_t *cmdclear;

  int   fpsaccum;
  int   fps;

  int   *globalscissor;

  lxVector4 orthobounds;
}List2D_t;


///////////////////////////////////////////////////////////////////////////////
// List2D

void List2D_init();
void List2D_deinit();

List2D_t* List2D_get();

  // if root is passed we will use that
void List2D_draw(List2DNode_t *root,float refw,float refh,int dirtyscissor,float *parentscissor,int *globalscissor);

  // root can be null
List2DNode_t *List2DNode_new(const char *name);

//////////////////////////////////////////////////////////////////////////
// List2DText

List2DText_t* List2DText_new(const char *text);
void List2DText_free(List2DText_t *ltext);

///////////////////////////////////////////////////////////////////////////////
// List2DNode

  // functions to create and delete a node
List2DNode_t *List2DNode_newFunc(const char *name, Mesh_t* (*draw)  (void *),void *upvalue);
List2DNode_t *List2DNode_newImage(const char *name, int texid);
List2DNode_t *List2DNode_newL3DNode(const char *name, List3DNode_t *l3node);
List2DNode_t *List2DNode_newL3DView(const char*name, List3DView_t *l3dview);
List2DNode_t *List2DNode_newText(const char *name, char *text);
List2DNode_t *List2DNode_newFlag(const char *name);


void List2DNode_updateFinalMatrix_recursive(List2DNode_t *node);
//void List2DNode_free(List2DNode_t *node);
void RList2DNode_free(lxRl2dnode ref);

void List2DNode_link(List2DNode_t *node,List2DNode_t *parent);
void List2DNode_setSortKey(List2DNode_t *node, int sortRID);

List2DNode_t* List2DNode_getRoot();

#endif
