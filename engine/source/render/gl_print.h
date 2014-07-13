// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_PRINT_H__
#define __GL_PRINT_H__

#include "../common/common.h"
#include "../common/reflib.h"
#include "../resource/texture.h"

#define GLPRINT_DEFAULT_WIDTH 16.0f

typedef enum FontSetSize_e{
    FONTSET_SIZE_0X0, // no sense - but required for simpler arithmetic
    FONTSET_SIZE_1X1,
    FONTSET_SIZE_2X2,
    FONTSET_SIZE_3X3,
    FONTSET_SIZE_4X4,
    FONTSET_SIZE_5X5,
  FONTSET_SIZE_6X6,
  FONTSET_SIZE_7X7,
  FONTSET_SIZE_8X8,
  FONTSET_SIZE_9X9,
  FONTSET_SIZE_10X10,
  FONTSET_SIZE_11X11,
  FONTSET_SIZE_12X12,
  FONTSET_SIZE_13X13,
  FONTSET_SIZE_14X14,
  FONTSET_SIZE_15X15,
  FONTSET_SIZE_16X16,
  FONTSET_SIZES,
} FontSetSize_t;

typedef struct FontSet_s {
  Reference   reference;
  FontSetSize_t size;
  int       numchars;
  int       ignorespecials;
  unsigned char charlookup[256];    // which char maps to which display list
  float     charwidth[256];   // width per char
  float     linespacing;
  float           offsetX;            // glyph offset
  float           offsetY;
} FontSet_t;

typedef struct PText_s{
  char        *text;

  Reference     fontsetref;
  int         texRID;

  float       width;
  float       tabwidth;
  float       size;
}PText_t;

typedef struct GLPrintData_s{
  uint    baseID[FONTSET_SIZES];  // display list bases
  //float   width;
  //float   tabwidth;
  //int     texRID;
  //FontSet_t *fontSet;

  FontSet_t defaultFontSet;
  enum32    defaultRenderFlag;
  int     defaultTexRID;
}GLPrintData_t;




//////////////////////////////////////////////////////////////////////////
// FontSet
FontSet_t* FontSet_new(FontSetSize_t size);
//void FontSet_free(FontSet_t *fs);
void RFontSet_free (lxRfontset ref);

//////////////////////////////////////////////////////////////////////////
// PText system
enum32    PText_getDefaultRenderFlag();
FontSet_t*  PText_getDefaultFontSet();

void PText_init();
void PText_deinit();
void PText_initGL();
void PText_clearGL();


//////////////////////////////////////////////////////////////////////////
// PText
PText_t *PText_new(const char *text);
void PText_free(PText_t *ptext);

  // warning dont call on "new" created ptexts
void PText_setDefaults(PText_t *ptext);
void PText_setFontSetRef(PText_t *ptext, Reference ref);
void PText_setFontTexRID(PText_t *ptext,int texRID);
void PText_setWidth(PText_t *ptext,float width);
void PText_setTabWidth(PText_t *ptext,float width);
void PText_setSize(PText_t *ptext, float size);
  // sets text string
void PText_setText(PText_t *ptext,const char *text);
  // gets space bounds
void PText_getDimensions(const PText_t *ptext, const char *overridetxt, lxVector3 outdim);
  // writes the position of the character at charidx in the outdim vector
void PText_getCharPoint(const PText_t *ptext, int charidx, lxVector3 outdim);
  // writes the character position that is next to the given coordinate
  // where 0,0 is top left position. Returns 1 on success, 0 on false hit
  // Sets *outidx to the last valid position
int PText_getCharindexAt(const PText_t *ptext, float x, float y, int *outidx);



//////////////////////////////////////////////////////////////////////////
// Drawing functions
//    they all require g_VID.drawsetup.rendercolor to be set
//    use vidColor4f(r,g,b,a) or vidColor4fv(colorvec) to set this value before drawing text

//  textbounds[x,y,w,h] must be in same space as x,y,z and when passed
//  will cause line skipping of text outside the bounds
void Draw_PText(float x, float y, float z, const float *textbounds, const PText_t *ptext, const char*overridetxt);
  // prints shadowed
void Draw_PText_shadowedf(float x, float y, float z,const float *textbounds,const PText_t *ptext, const char *fmt, ...);
void Draw_PText_f(float x, float y, float z,const float *textbounds,const PText_t *ptext, const char *fmt, ...);

//////////////////////////////////////////////////////////////////////////
// Raw Drawing for console
void Draw_RawText_start();
void Draw_RawText_end();
void Draw_RawText_char(const unsigned char chr);

#endif

