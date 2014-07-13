// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../common/common.h"

#include "gl_drawmesh.h"
#include "gl_draw2d.h"

// LOCALS
static lxVector3 l_scalevec;

// draws a screen image (automaticly with alpha blending)
void Draw_Texture_screen( float x, float y, float z, float w, float h, int texid, int blend, int setproj, enum32 forceflags, enum32 ignoreflags)
{
  int renderflag = 0;
  if (setproj){
    glFrontFace(GL_CW);
    vidOrthoOn(0,VID_REF_WIDTH ,VID_REF_HEIGHT,0,-128,128);
  }

  lxMatrix44IdentitySIMD(g_VID.drawsetup.worldMatrixCache);
  lxVector3Set(&g_VID.drawsetup.worldMatrixCache[12],x,y,z);

  lxVector3Set(l_scalevec,w,h,1);
  vidWorldMatrix(g_VID.drawsetup.worldMatrixCache);
  vidWorldScale(l_scalevec);

  if (vidMaterial(texid)){
    Material_update(ResData_getMaterial(texid),NULL);
    renderflag |= ResData_getMaterialShaderUse(texid)->renderflag;
  }
  else if (blend){
    renderflag |= RENDER_BLEND;
  }

  renderflag = (renderflag & ignoreflags) | forceflags;

  vidCheckError();
  VIDRenderFlag_setGL(renderflag);
  // draw
  glColor4f(1,1,1,1);
  Draw_Mesh_auto(g_VID.drw_meshquad,texid,NULL,0,NULL);


  if (setproj){
    glFrontFace(GL_CCW);
    vidOrthoOff();
  }

  vidWorldScale(NULL);
}


void Draw_Texture_cubemap(int texid, float inx, float iny, float size)
{
  int i;
  float x;
  float y;
  float width = size/4.0f;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(inx,iny,0);


  vidBlending(LUX_FALSE);
  vidAlphaTest(LUX_FALSE);
  vidNoDepthTest(LUX_TRUE);
  vidSelectTexture(0);
  vidBindTexture(texid);
  vidTexturing(GL_TEXTURE_CUBE_MAP_ARB);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  vidTexBlendDefault(VID_TEXBLEND_MOD_MOD);
  vidClearTexStages(1,i);

  glColor3f(1,1,1);

  //    -z
  //  -x  +y  +x  -y
  //    +z


  glBegin(GL_QUADS);
  x = 0;
  y = width;
  // -x
  glTexCoord3f(-1,-1,1);
  glVertex3f(x,y,0);
  glTexCoord3f(-1,1,1);
  glVertex3f(x+width,y,0);
  glTexCoord3f(-1,1,-1);
  glVertex3f(x+width,y+width,0);
  glTexCoord3f(-1,-1,-1);
  glVertex3f(x,y+width,0);
  x+= width;
  // +y
  glTexCoord3f(-1,1,1);
  glVertex3f(x,y,0);
  glTexCoord3f(1,1,1);
  glVertex3f(x+width,y,0);
  glTexCoord3f(1,1,-1);
  glVertex3f(x+width,y+width,0);
  glTexCoord3f(-1,1,-1);
  glVertex3f(x,y+width,0);
  x+=width;
  // +x
  glTexCoord3f(1,1,1);
  glVertex3f(x,y,0);
  glTexCoord3f(1,-1,1);
  glVertex3f(x+width,y,0);
  glTexCoord3f(1,-1,-1);
  glVertex3f(x+width,y+width,0);
  glTexCoord3f(1,1,-1);
  glVertex3f(x,y+width,0);
  x+=width;
  // -y
  glTexCoord3f(1,-1,1);
  glVertex3f(x,y,0);
  glTexCoord3f(-1,-1,1);
  glVertex3f(x+width,y,0);
  glTexCoord3f(-1,-1,-1);
  glVertex3f(x+width,y+width,0);
  glTexCoord3f(1,-1,-1);
  glVertex3f(x,y+width,0);

  x = width;
  y = 2*width;
  // -z
  glTexCoord3f(-1,1,-1);
  glVertex3f(x,y,0);
  glTexCoord3f(1,1,-1);
  glVertex3f(x+width,y,0);
  glTexCoord3f(1,-1,-1);
  glVertex3f(x+width,y+width,0);
  glTexCoord3f(-1,-1,-1);
  glVertex3f(x,y+width,0);
  y = 0;
  // +z
  glTexCoord3f(-1,-1,1);
  glVertex3f(x,y,0);
  glTexCoord3f(1,-1,1);
  glVertex3f(x+width,y,0);
  glTexCoord3f(1,1,1);
  glVertex3f(x+width,y+width,0);
  glTexCoord3f(-1,1,1);
  glVertex3f(x,y+width,0);

  glEnd();
}

