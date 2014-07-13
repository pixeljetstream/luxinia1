// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "gl_print.h"
#include "../common/console.h"


// LOCALS
static GLPrintData_t l_printdata;
uint PText_initDisplayList(int chars, int sidelength);

//////////////////////////////////////////////////////////////////////////
// FontSet

void FontSet_defaults(FontSet_t *fs)
{
  int i,size,fixumlaut = 0;

  fs->linespacing = GLPRINT_DEFAULT_WIDTH;
  fs->offsetX = 0;
  fs->offsetY = 0;
    size = fs->size;
  switch(fs->size)
  {
  case FONTSET_SIZE_16X16:
    fixumlaut = LUX_TRUE;
    break;
  case FONTSET_SIZE_8X8:
    fixumlaut = LUX_FALSE;
    break;
  case FONTSET_SIZE_6X6:
    fixumlaut = LUX_FALSE;
    break;
  }

  fs->numchars = size*size;

  for (i = 0; i < size*size; i++)
  {
    fs->charwidth[i] = 1.0f;
    fs->charlookup[i] = (i - 32);
  }

  if (fixumlaut){
    fs->charlookup[(uchar)'ä'] = 100;
    fs->charlookup[(uchar)'ü'] = 97;
    fs->charlookup[(uchar)'ö'] = 116;
    fs->charlookup[(uchar)'Ä'] = 110;
    fs->charlookup[(uchar)'Ü'] = 122;
    fs->charlookup[(uchar)'Ö'] = 121;
  }
}

FontSet_t* FontSet_new(FontSetSize_t size)
{
  FontSet_t *fs = lxMemGenZalloc(sizeof(FontSet_t));

  if (!l_printdata.baseID[size])
        l_printdata.baseID[size] = PText_initDisplayList(size*size,size);
  fs->size = size;
  FontSet_defaults(fs);

  fs->reference = Reference_new(LUXI_CLASS_FONTSET,fs);

  return fs;
}

static void FontSet_free(FontSet_t *fs)
{
  if (fs == &l_printdata.defaultFontSet)
    return;

  Reference_invalidate(fs->reference);
  lxMemGenFree(fs,sizeof(FontSet_t));
}
void RFontSet_free (lxRfontset ref)
{
  FontSet_free((FontSet_t*)Reference_value(ref));
}
//////////////////////////////////////////////////////////////////////////
// PText system

enum32 PText_getDefaultRenderFlag()
{
  return l_printdata.defaultRenderFlag;
}

FontSet_t*  PText_getDefaultFontSet()
{
  return &l_printdata.defaultFontSet;
}

uint PText_initDisplayList(int chars, int sidelength)
{
  uint base;
  int i;
    float uvsize = 1.0f/(float)sidelength;
    float eps = 0;

  base=glGenLists(chars);
  for (i=0; i<chars; i++)
  {
    float cx=(int)(((float)(i%sidelength)/(float)(sidelength))*256)/256.0f;
    float cy=(int)(((float)(i/sidelength)/(float)(sidelength))*256)/256.0f;

    glNewList(base+i,GL_COMPILE);
      glBegin(GL_QUADS);
      glNormal3f(0.0f,0.0f,-16.0f);
      glTexCoord2f(cx,1.0f-cy-eps);       glVertex2f(0.0f,0.0f);
      glTexCoord2f(cx+uvsize,1.0f-cy-eps);  glVertex2f(256.0f/(float)sidelength,0.0f);
      glTexCoord2f(cx+uvsize,1.0f-cy-uvsize); glVertex2f(256.0f/(float)sidelength,256/(float)sidelength);
      glTexCoord2f(cx,1.0f-cy-uvsize);    glVertex2f(0.0f,256.0f/(float)sidelength);
      glEnd();
    glEndList();
  }

  LUX_PROFILING_OP (g_Profiling.global.memory.viddlmem += chars * (64+12));

  return base;
}


void PText_init()                 // Build Our Font Display List
{
  FontSet_t *fs;

  Reference_registerType(LUXI_CLASS_FONTSET,"fontset",RFontSet_free,NULL);

  // clear all
  memset(&l_printdata,0,sizeof(GLPrintData_t));


  // set up defaults
  l_printdata.defaultRenderFlag = RENDER_NODEPTHTEST | RENDER_BLEND | RENDER_NOVERTEXCOLOR;

  fs = &l_printdata.defaultFontSet;
  fs->size = FONTSET_SIZE_16X16;
  fs->reference = Reference_new(LUXI_CLASS_FONTSET,fs);
  Reference_makeStrong(fs->reference);

  FontSet_defaults(fs);

  PText_initGL();

}

void PText_deinit()
{

  Reference_invalidate(l_printdata.defaultFontSet.reference);
  Reference_release(l_printdata.defaultFontSet.reference);

  PText_clearGL();
}

void PText_initGL()
{
    int i=0;
  ResData_CoreChunk(LUX_TRUE);
  l_printdata.defaultTexRID = ResData_addTexture("font.tga",TEX_ALPHA,TEX_ATTR_MIPMAP);
  ResData_CoreChunk(LUX_FALSE);

  // create display lists
  l_printdata.baseID[FONTSET_SIZE_16X16] = PText_initDisplayList(256,16);
  for (i=0;i<16;i++)
        if (l_printdata.baseID[i])
            l_printdata.baseID[i] = PText_initDisplayList(i*i,i);
  //l_printdata.baseRID[FONTSET_SIZE_8X8] = PText_initDisplayList(64,8);
  //l_printdata.baseRID[FONTSET_SIZE_6X6] = PText_initDisplayList(36,6);
}

void PText_clearGL()
{
    int i;
    for (i=0;i<16;i++)
    if (l_printdata.baseID[i]){
            glDeleteLists(l_printdata.baseID[i],  i*i);
      LUX_PROFILING_OP (g_Profiling.global.memory.viddlmem += i*i * (64+12));
    }
}

//////////////////////////////////////////////////////////////////////////
// PText
PText_t *PText_new(const char *text)
{
  PText_t *ptext = lxMemGenZalloc(sizeof(PText_t));
  PText_setDefaults(ptext);
  PText_setText(ptext,text);
  return ptext;
}
void PText_free(PText_t *ptext)
{
  PText_setText(ptext,NULL);
  lxMemGenFree(ptext,sizeof(PText_t));
}


void PText_setFontSetRef(PText_t *ptext, Reference ref)
{
  ref = Reference_isValid(ref) ? ref : l_printdata.defaultFontSet.reference;

  Reference_refWeak(ref);
  Reference_releaseWeakCheck(ptext->fontsetref);

  ptext->fontsetref = ref;
}


void PText_setFontTexRID(PText_t *ptext, int texRID)
{
  ptext->texRID = texRID < 0 ? l_printdata.defaultTexRID : texRID;
}

void PText_setWidth(PText_t *ptext, float width)
{
  if (width <= 0)
    ptext->width = GLPRINT_DEFAULT_WIDTH;
  else
    ptext->width = width;
}

void PText_setTabWidth(PText_t *ptext, float width)
{
  ptext->tabwidth = width;
}
void PText_setSize(PText_t *ptext, float size)
{
  ptext->size = size;
}

void PText_setDefaults(PText_t *ptext)
{
  ptext->fontsetref = l_printdata.defaultFontSet.reference;

  Reference_refWeak(ptext->fontsetref);

  ptext->size = 16.0f;
  ptext->tabwidth = 0.0f;
  ptext->width = GLPRINT_DEFAULT_WIDTH;
  ptext->texRID = l_printdata.defaultTexRID;
  ptext->text = NULL;
}
void PText_setText(PText_t *ptext,const char *text)
{
  if (ptext->text){
    genfreestr(ptext->text);
  }
  if (text){
    gennewstrcpy(ptext->text,text);
  }
  else{
    ptext->text = NULL;
  }
}

void PText_getCharPoint(const PText_t *ptext, int charidx, lxVector3 outdim)
{
  float size = ptext->size;
  float width = ptext->width;
  float tabwidth = ptext->tabwidth;
  const char *text = ptext->text;

  const uchar *tx;
  const uchar *to;
  float linemove;
  FontSet_t *fs = &l_printdata.defaultFontSet;

  Reference_get( ptext->fontsetref,fs);
  tabwidth = tabwidth ? tabwidth : width * 4;


  lxVector3Set(outdim,0,0,0);
  size*=0.0625f;
  linemove = 0;
  tx = text;
  to = &text[charidx];
  outdim[1] += fs->linespacing;
  while (*tx && (tx<=to)){
    if (!fs->ignorespecials){
      if (*tx=='\v') {
        tx++;
        if (*tx=='u' || *tx=='b' || *tx=='s' || *tx=='c') tx++;
        else if (*tx=='x'||*tx=='t') {
          while (*tx && *tx!=';' && tx<=to) tx++;
        }
        else tx+=3;
        continue;
      }
      if (*tx=='\n') {
        tx++;
        outdim[0] = 0;
        outdim[1] += fs->linespacing;
        linemove = 0;
        continue;
      }
      if (*tx=='\t') {
        float rest = tabwidth- fmod(linemove,tabwidth);
        if (rest==0) rest+=tabwidth;
        linemove += rest;
        tx++;
        continue;
      }
    }

    linemove +=  width*fs->charwidth[*tx];
    outdim[0] = linemove;
    tx++;
  }
  outdim[2] = 1.0;
  lxVector3Scale(outdim,outdim,size);
}

int PText_getCharindexAt(const PText_t *ptext, float x, float y, int *outidx)
{
  float size = ptext->size;
  float width = ptext->width;
  float tabwidth = ptext->tabwidth;
  const char *text = ptext->text;

  const uchar *tx;
  float linemove,py;
  FontSet_t *fs = &l_printdata.defaultFontSet;

  Reference_get(ptext->fontsetref,fs);
  tabwidth = tabwidth ? tabwidth : width * 4;

  size*=0.0625f;
  x /=size;
  y /=size;
  linemove = 0;
  tx = text;
  py = 0;
  py += fs->linespacing;
  while (*tx){
    if (!fs->ignorespecials){
      if (*tx=='\v') {
        tx++;
        if (*tx=='u' || *tx=='b' || *tx=='s' || *tx=='c') tx++;
        else if (*tx=='x'||*tx=='t') {
          while (*tx && *tx!=';') tx++;
        }
        else tx+=3;
        continue;
      }
      if (*tx=='\n') {
        tx++;
        if (py>=y) {
          *outidx = (int)tx-(int)text-1;
          return 0; // the line above is not long enough
        }
        py += fs->linespacing;
        linemove = 0;
        continue;
      }
      if (*tx=='\t') {
        float rest = tabwidth - fmod(linemove,tabwidth);
        if (rest==0) rest+=tabwidth;
        linemove += rest;
        tx++;
        continue;
      }
    }

    linemove +=  width*fs->charwidth[*tx];
    if (py>=y && linemove>x) //hit
    {
      *outidx = (int)tx-(int)text;
      return x>=0 && y>=0; // if it is in bound, true
    }
    tx++;
  }
  *outidx = (int)tx-(int)text;

  return 0;
}

void PText_getDimensions(const PText_t *ptext, const char *overridetxt, lxVector3 outdim)
{
  float size = ptext->size;
  float width = ptext->width;
  float tabwidth = ptext->tabwidth;
  const char *text = overridetxt ? overridetxt : ptext->text;

  const uchar *tx;
  float linemove;
  FontSet_t *fs;

  Reference_get(ptext->fontsetref,fs);
  LUX_ASSERT(fs);

  tabwidth = tabwidth ? tabwidth : width * 4;

  lxVector3Set(outdim,0,0,0);

  size*=0.0625f;
  linemove = 0;
  tx = text;
  outdim[1] += fs->linespacing;
  while (*tx){
      if (*tx=='\v') {
          tx++;
          if (*tx>='0' && *tx<='9') {
            tx+=3;
          }
          if (*tx=='u' || *tx=='b' || *tx=='s' || *tx=='c' || *tx=='C' || *tx=='R')
        tx++;
      if (*tx=='x') {
        int px = 0,dec = 1;
                int r,jump = 0,l;

                for (r=0;r<5;r++) {
                    if (tx[r+1]<='9' && tx[r+1]>='0') {

                    }
                    else if (tx[r+1]==';') {
                        for (l=r;l>0;l--) {
                            px+=dec*(tx[l]-'0');
                            dec*=10;
                        }
                        jump = 1;
                        tx+=r+2;
                    } else break;
                }
                if (jump) {
                    linemove = (float)px;
                }
      }
      continue;
      }
    if (!fs->ignorespecials){
      if (*tx=='\n') {
        tx++;
        outdim[0] = LUX_MAX(outdim[0],linemove);
        outdim[1] += fs->linespacing;
        linemove = 0;
        continue;
      }
      if (*tx=='\t') {
        float rest = tabwidth - fmod(linemove,tabwidth);
        if (rest==0) rest+=tabwidth;
        linemove += rest;
        tx++;
        continue;
      }
    }

    linemove +=  width*fs->charwidth[*tx];
    tx++;
  }
  outdim[0] = LUX_MAX(outdim[0],linemove);
  outdim[2] = 1.0;
  lxVector3Scale(outdim,outdim,size);
}

//////////////////////////////////////////////////////////////////////////
// Drawing

void Draw_PText(float x, float y, float z, const float *textbounds,
        const PText_t *ptext, const char *overridetxt)
{
  const uchar *tx;
  int     bold,
          underline,
          shadow;
  uchar curcolorub[4],origcolorub[4];

  int i;
  uint c;
  float linemove;
  float move;
  float spacing;
  const float *color = g_VID.drawsetup.rendercolor;

  float tabwidth = ptext->tabwidth;
  float width = ptext->width;
  float scale = ptext->size;
  float *textdim = NULL;
  const char *text = overridetxt ? overridetxt : ptext->text;
  FontSet_t *fs;

  if (text == NULL)
    return;

  Reference_get(ptext->fontsetref,fs);
  LUX_ASSERT(fs);

  bold = 0;
  underline = 0;
  shadow = 0;
  scale*=0.0625f;
  tabwidth = tabwidth ? tabwidth :  width * 4;
  spacing = fs->linespacing/scale;


  // set default state for printing
  vidSelectTexture(0);
  vidTexBlendDefault(VID_TEXBLEND_MOD_MOD);
  vidBindTexture(ptext->texRID);
  vidTexturing(GL_TEXTURE_2D);
  vidClearTexStages(1,i);

  lxVector4ubyte_FROM_float(origcolorub,color);
  LUX_ARRAY4COPY(curcolorub,origcolorub);
  glLineWidth(2);

  // matrices
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  x = x-fs->offsetX;
  y = y-fs->offsetY;

  glTranslatef(x,y,z);
  glScalef(scale,scale,1.0f);


  tx = text;
  linemove = 0;
  while (*tx) {
    // special chars
    if (!fs->ignorespecials){
    // fast line skipping
    if (textbounds && linemove == 0){
      if (y > textbounds[1]+textbounds[3]){
        // finished
        break;
      }

      while (y+spacing < textbounds[1]){
        // skip lines until visible again
        while(*tx && *tx != '\n')
          tx++;

        if (*tx == 0)
          break;
        else
          tx++;

        glTranslatef(0,fs->linespacing,0);
        y += fs->linespacing;
      }

    }
    if (*tx=='\n') {
      tx++;

      glTranslatef(-linemove,fs->linespacing,0);
      y += fs->linespacing;

      linemove = 0;
      continue;
    }
    if (*tx=='\t') {
      float rest = tabwidth - fmod(linemove,tabwidth);
      if (rest==0) rest+=tabwidth;
      linemove += rest;

      glTranslatef(rest,0,0);

      tx++;
      continue;
    }
    if (*tx=='\v') {
      uchar rgb[4];
      float dims[3],linedim[3];

      tx++;
      if (*tx=='u') {     underline = !underline; tx++; continue;}
      else if (*tx == 'R' || *tx=='C') { // alignment
        int to = 0;
        int cen = *tx=='C';
        char prev;
        float offy;

        if (!textdim) {
          textdim = dims;
          PText_getDimensions(ptext,text,dims);
        }
        tx++;
        while (tx[to]!=0 && tx[to]!='\n') to++;
        prev = tx[to];
        ((char*)tx)[to] = 0; // let s get the length of the line
        PText_getDimensions(ptext,tx,linedim);
        ((char*)tx)[to] = prev;
        offy = cen?floor((textdim[0]-linedim[0])/2):textdim[0]-linedim[0];

        glTranslatef(offy,0,0);
        linemove+=offy;

        continue;
      }
      else if (*tx == 'b') {  bold = !bold; tx++; continue;}
      else if (*tx == 's') {  shadow = !shadow; tx++; continue;}
      else if (*tx == 'c') {  glColor3ubv(origcolorub); LUX_ARRAY3COPY(curcolorub,origcolorub); tx++; continue;}
      else if (*tx == 'x') {
                int px = 0,dec = 1;
                int r,jump = 0,l;

                for (r=0;r<5;r++) {
                    if (tx[r+1]<='9' && tx[r+1]>='0') {

                    }
                    else if (tx[r+1]==';') {
                        for (l=r;l>0;l--) {
                            px+=dec*(tx[l]-'0');
                            dec*=10;
                        }
                        jump = 1;
                        tx+=r+2;
                    } else break;
                }
                if (jump) {
                    glTranslatef((float)px-linemove,0.0f,0.0f);
                    linemove = (float)px;
                    continue;
                }

      }
      else
            {
        if (!*(tx) || !*(tx+1) || !*(tx+2)) break;
        rgb[0] = ((*tx-'0')%10)*25+30;
        rgb[1] = ((*(tx+1)-'0')%10)*25+30;
        rgb[2] = ((*(tx+2)-'0')%10)*25+30;
        glColor3ubv(rgb);
        LUX_ARRAY3COPY(curcolorub,rgb);
        tx+=3;
        continue;
      }
    }
    }


    c = fs->charlookup[*tx]+l_printdata.baseID[fs->size];

    if (bold)     {
      //glTranslatef(1,1,0);
      glCallList(c);
      //glTranslatef(-2,-2,0);
      glCallList(c);
      //glTranslatef(1,1,0);
    }
    if (shadow)   {
      glTranslatef(2,2,0);
      glColor4f(0,0,0,0.9);
      glCallList(c);
      glColor4ubv(curcolorub);
      glTranslatef(-2,-2,0);
    }
    if (underline)  {
      glTranslatef(-4,0,0);
      glCallList(fs->charlookup['_']+l_printdata.baseID[fs->size]);
      glTranslatef(+8,0,0);
      glCallList(fs->charlookup['_']+l_printdata.baseID[fs->size]);
      glTranslatef(-4,0,0);
    }

        //glTranslatef(-2,0,0);
        glCallList(c);

    move = (width*fs->charwidth[*tx]);
    linemove += move ;
    glTranslatef(move,0,0);
    tx++;
  }

  vidCheckError();

  glPopMatrix();
  //glColor4fv(g_VID.drawsetup.rendercolor);

  //vidTexturing(FALSE);


}

void Draw_PText_shadowedf(float x, float y, float z, const float *scissorrect,const PText_t *ptext, const char *fmt, ...)
{
  static char   text[1024];
  const float   *color = g_VID.drawsetup.rendercolor;
  va_list   ap;

  if (fmt == NULL)
    return;

  va_start(ap, fmt);                    // Parses The String For Variables
      vsprintf(text, fmt, ap);              // And Converts Symbols To Actual Numbers
  va_end(ap);

  glColor3f(0,0,0);
  Draw_PText(x+1,y+1,z-1,scissorrect,ptext,text);
  glColor4fv(color);
  Draw_PText(x,y,z,scissorrect,ptext,text);
}

void Draw_PText_f(float x, float y, float z, const float *scissorrect,const PText_t *ptext, const char *fmt, ...)
{
  static char   text[1024];
  va_list   ap;

  if (fmt == NULL)
    return;

  va_start(ap, fmt);                    // Parses The String For Variables
  vsprintf(text, fmt, ap);              // And Converts Symbols To Actual Numbers
  va_end(ap);

  Draw_PText(x,y,z,scissorrect,ptext,text);
}

void Draw_RawText_start ()
{
  int i;

  vidSelectTexture(0);
  vidTexBlendDefault(VID_TEXBLEND_MOD_MOD);
  vidBindTexture(l_printdata.defaultTexRID);    // Select Our Font Texture_t
  vidTexturing(GL_TEXTURE_2D);
  vidClearTexStages(1,i);

  VIDRenderFlag_setGL(l_printdata.defaultRenderFlag);
  vidWireframe(LUX_FALSE);
  vidBlend(VID_DECAL,LUX_FALSE);

  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);            // Select The Projection Matrix44
  glPushMatrix();
  glLoadIdentity();                 // Reset The Projection Matrix44
  glOrtho(0,VID_REF_WIDTH ,VID_REF_HEIGHT,0,-128,128);  // Set Up An Ortho Screen

  // needed as top/bottom is swapped
  glFrontFace(GL_CW);

  glMatrixMode(GL_MODELVIEW);             // Select The Modelview Matrix44
  glPushMatrix();                   // Store The Modelview Matrix44
  glLoadIdentity();                 // Reset The Modelview Matrix44

}

void Draw_RawText_end ()
{
  vidTexturing(LUX_FALSE);                // Disable Texture_t Mapping
  glMatrixMode(GL_PROJECTION);            // Select The Projection Matrix44
  glPopMatrix();                    // Restore The Old Projection Matrix44
  glMatrixMode(GL_MODELVIEW);             // Select The Modelview Matrix44
  glPopMatrix();
  vidBlending(LUX_FALSE);

  // needed as top/bottom is swapped
  glFrontFace(GL_CCW);

}

void Draw_RawText_char (const unsigned char chr)
{
  // printing

  glCallList( (uint)l_printdata.defaultFontSet.charlookup[chr] + l_printdata.baseID[FONTSET_SIZE_16X16]);
}



