// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include <stdio.h>
#include <stdarg.h>
#include "../common/common.h"
#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../common/timer.h"
#include "../render/gl_print.h"
#include "../fnpublish/fnpublish.h"
#include "../render/gl_render.h"

////////////////////////////////////////////////////////////////////////////////
// The Console: This does NOT interprete anything!!! it only displays chars on
// the screen
typedef struct Console_s {
  int *screen;
  int isActive;
  long transitionStart;
  unsigned short w, h;
  float r,g,b,a;
  int *position;
} Console_t;

////////////////////////////////////////////////////////////////////////////////
// local variables used by this c file
#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 40

static Console_t *l_console = NULL;

static void (*l_printStd)(const char *chr);
static void (*l_printErr)(const char *chr);
static void (*l_keyIn)( int key);



////////////////////////////////////////////////////////////////////////////////
// The Console implementation

static void Console_free(Console_t *self) {
  lxMemGenFree(self->screen,self->w*self->h*sizeof(int));
  lxMemGenFree(self,sizeof(Console_t));
}

static Console_t* Console_new (unsigned short w, unsigned short h)
{
  Console_t *self;

  self = (Console_t*) lxMemGenZalloc(sizeof(Console_t));
  self->isActive = 0;
  self->transitionStart = -100;
  self->w = w;
  self->h = h;
  self->r = 0.4; self->g = 0.4; self->b = 0.8; self->a = 0.5;
  self->screen = (int*)lxMemGenZalloc(w*h*sizeof(int));
  self->position = self->screen;

  return self;
}

void Console_setPos (unsigned short x, unsigned short y)
{
  Console_t *self = l_console;

  self->position = self->screen+((y*self->w + x)%(self->w*self->h));
}

void Console_getPos (unsigned short *x, unsigned short *y)
{
  Console_t *self = l_console;
  if (x) *x = (unsigned short)((int)(self->position-self->screen)%self->w);
  if (y) *y = (unsigned short)((int)(self->position-self->screen)/self->w);
}

void Console_put (char *chr, unsigned char style, unsigned short color)
{
  Console_t *self = l_console;
  unsigned char* uchr = (unsigned char*)chr;
  while (*uchr)
  {
    *self->position = *uchr|(style<<8)|(color<<16);
    uchr++;
    self->position++;
    if (self->position-self->screen >= self->w*self->h)
      self->position = self->screen;
  }
}

unsigned char Console_get (int x, int y, int *style, int *color)
{
  Console_t *self = l_console;
  int *i;

  i = self->screen+((y*self->w + x)%(self->w*self->h));
  if (style!=NULL) *style=(*i>>8)&0xff;
  if (color!=NULL) *color=*i>>16;
  return *i&0xff;
}

void Console_putln ( char *cchr, char style, short color) {
  Console_t *self = l_console;
  unsigned char *chr = (unsigned char *)cchr;

  Console_put(cchr,style,color);
  self->position+=(self->w-(int)(self->position-self->screen)%self->w);
  if (self->position >= self->screen + self->w * self->h)
    self->position = self->screen;
}

void Console_print(FILE *stream)
{
  Console_t *self = l_console;
  int *position;
  int rest;

  rest = self->w*self->h;
  position = self->screen;
  while (rest--)
  {
    fprintf(stream,"%c",(*position++)&0xff);
  }
}

void Console_clear()
{
  Console_t *self = l_console;
  int rest = self->w*self->h;

  self->position = self->screen;
  while (rest--) *(self->position++) = 0;
  self->position = self->screen;
}

void Console_shift(unsigned short lines)
{
  Console_t *self = l_console;
  int rest = self->w*self->h;
  int *position = self->screen;

  while (rest--)
  {
    if (position - self->screen + self->w*lines < self->w*self->h)
        *position = *(position + self->w*lines);
    else  *position = 0;
    position++;
  }
}

void Console_draw()
{
  Console_t *self = l_console;
  int *position;
  int rest;
  float transition,scf=0.96;
  int i;
  float vidrefh,vidrefw;
  float aspect;


  transition = (float)(g_LuxTimer.time - self->transitionStart);
  transition*=0.25;
  if (transition>100) transition = 100;
  if (!self->isActive) transition = 100-transition;

  if ((!self->isActive && transition==0) || g_LuxTimer.time<500) return;

  vidrefw = VID_REF_WIDTH;
  vidrefh = VID_REF_HEIGHT;

  // scale refwidth
  aspect = (float)g_VID.windowWidth/(float)g_VID.windowHeight;
  if (aspect > (640.0/480.0)){
    VID_REF_WIDTH = aspect*480.0;
    VID_REF_HEIGHT = 480.0;
  }
  else{
    VID_REF_WIDTH = 640;
    VID_REF_HEIGHT = 640.0/aspect;
  }

  rest = self->w*self->h;
  position = self->screen;

  glClear(GL_DEPTH_BUFFER_BIT);
  vidWireframe(LUX_FALSE);
  // <draw_background>
  vidClearTexStages(0,i);
  vidNoDepthTest(LUX_TRUE);
  vidNoDepthMask(LUX_TRUE);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(-1.0, 1.0, 1.0, -1.0);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  vidNoCullFace(LUX_TRUE);
  #define TRANSITITION \
    glRotatef(90-transition*0.9,0,1,0);\
    glScalef(transition*0.01,transition*0.01,transition*0.01);\

  glRotatef(180,0,1,0);
  TRANSITITION

  vidBlending(LUX_TRUE);
  glColor4f( self->r,self->g,self->b,self->a*(transition*0.01));
  vidBlend(VID_DECAL,LUX_FALSE);
  vidLighting(LUX_FALSE);
  glBegin(GL_QUADS);
    glVertex2f(-1.0,  1);
    glVertex2f(-1.0,  -1.0);
    glVertex2f( 1.0,  -1.0);
    glVertex2f( 1.0,  1);

    glVertex2f(-scf,  scf);
    glVertex2f(-scf,  -scf);
    glVertex2f( scf,  -scf);
    glVertex2f( scf,  scf);
  glEnd();
  //vidLighting(TRUE);
  vidBlending(LUX_FALSE);
  // </draw_background>
  glColor4f( 1.0, 1.0, 1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  vidNoDepthTest(LUX_FALSE);
  vidNoDepthMask(LUX_FALSE);

  Draw_RawText_start();
  glTranslatef(VID_REF_WIDTH/2,VID_REF_HEIGHT/2,0);
  TRANSITITION

  glScalef(scf,-scf,scf);

  glTranslatef(-VID_REF_WIDTH/2,-VID_REF_HEIGHT/2,0);
  glTranslatef(0,VID_REF_HEIGHT,0);
  glScalef(0.79,-0.75,1.0f);
  while (rest--)
  {
    unsigned char c;

    c = (char)((*position)&0xff);

    if (c <' ') glTranslatef(10,0,0);
    else {
      lxMatrix44 m;
      int color,r,g,b,style;

      style = (*position>>8)&0xff;
      color = (*position>>16);
      r = ((color>>10 & 0x1f)*8*0xff/248);
      g = ((color>>5 &  0x1f)*8*0xff/248);
      b = ((color &     0x1f)*8*0xff/248);
      glPushMatrix();
      glColor3ub(r,g,b);
      if (style & 16) { // smaller
        glScalef(0.8,0.8,1.0f);
      }
      if (style & 2) { // italic
        // very costly but doesnt matter as console isnt performance critical
        glGetFloatv(GL_MODELVIEW_MATRIX,m);
        m[4] = -0.4;
        glLoadMatrixf(m);
        glTranslatef(4,0,0);
      }
      Draw_RawText_char(c);
      if (style&1) { // bold
        //glTranslatef(-1,0,0);
        Draw_RawText_char(c);
        //glTranslatef(+2,0,0);
        Draw_RawText_char(c);
      }
      if (style&4) { // underline
        //glTranslatef(-16,2,0);
        glTranslatef(0,2,0);
        Draw_RawText_char('_');
        //glTranslatef(-14,0,0);
        glTranslatef(2,0,0);
        Draw_RawText_char('_');
        glTranslatef(-2,-2,0);
      }
      if (style&8) { // strikeout
        glTranslatef(+2,0,0);
        Draw_RawText_char('-');
        glTranslatef(+4,0,0);
        Draw_RawText_char('-');
        glTranslatef(+8,0,0);
      }
      glPopMatrix();
      glTranslatef(10,0,0);
    }
    //glTranslatef(10,0,0);
    if (rest%self->w==0) {
      glTranslatef(-10.0f*(float)self->w,16.0f,0.0f);
    }
    position++;
  }
  Draw_RawText_end();
  vidNoCullFace(LUX_FALSE);
  VID_REF_WIDTH = vidrefw;
  VID_REF_HEIGHT = vidrefh;
}

void Console_setActive (int on) {
  Console_t *self = l_console;
  long tstart = g_LuxTimer.time-self->transitionStart;

  self->isActive = (on!=0);
  if (tstart>250) tstart = 250;
  self->transitionStart = g_LuxTimer.time-(250-tstart);
}

int Console_isActive () {
  Console_t *self = l_console;

  if (self == NULL) return 0;
  return self->isActive;
}

void Console_printf (const char *fmt,...)
{
  static char   text[1024];
  va_list   ap;
  int textlen;

  if (fmt == NULL) return;
  va_start(ap, fmt);
      textlen = vsprintf(text, fmt, ap);
  va_end(ap);

  if (!l_printStd)
  {
    Console_put(text,0,0xffff);
    return;
  }
  l_printStd(text);
}

void Console_printferr (const char *fmt,...)
{
  static char   text[1024];
  va_list   ap;
  int textlen;

  if (fmt == NULL) return;
  va_start(ap, fmt);
      textlen = vsprintf(text, fmt, ap);
  va_end(ap);

  if (!l_printErr)
  {
    Console_put(text,0,0xf000);
    return;
  }
  l_printErr(text);
}

void Console_setPrintErr(void (*out)(const char *))
{
  l_printErr = out;
}

void Console_setPrintStd(void (*out)(const char *))
{
  l_printStd = out;
}

void Console_keyIn(int key) {
  if (!l_keyIn) return;
  l_keyIn(key);
}

void Console_setKeyIn (void (*keyIn)(int key))
{
  l_keyIn = keyIn;
}

void Console_deinit() {
  Console_free(l_console);
  l_console = NULL;
}

int Console_getHeight() {
  return CONSOLE_HEIGHT;
}

int Console_getWidth() {
  return CONSOLE_WIDTH;
}


void Console_init () {
  l_console = Console_new(CONSOLE_WIDTH,CONSOLE_HEIGHT);
}
