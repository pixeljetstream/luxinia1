// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../controls/controls.h"
#include "../common/console.h"
#include "../common/common.h"

////////////////////////////////////////////////////////////////////////////////
// The Bufferobject is a struct that stores a queue of strings
#define BUFFERSIZE ((1<<16)-1)

typedef struct Buffer_s {
  char buffer[0xff][1024];
  unsigned int current;
  unsigned int read;
} Buffer_t;

////////////////////////////////////////////////////////////////////////////////
// local globals
static Buffer_t *l_std=NULL,*l_err=NULL;

////////////////////////////////////////////////////////////////////////////////
// The buffer implementation


static Buffer_t* Buffer_new() {
  Buffer_t *self = lxMemGenZalloc(sizeof(Buffer_t));
  self->buffer[0][0] = 0;
  self->read = 0;
  self->current = 0;

  return self;
}

static void Buffer_free(Buffer_t *self) {
  lxMemGenFree(self,sizeof(Buffer_t));
}

static void Buffer_push (Buffer_t *self, const char* str)
{
  strncpy(self->buffer[(self->current++)%0xff],str,1023);
  if (self->current == self->read) self->read++;
}

static const char *Buffer_pop (Buffer_t *self)
{
//  LUX_PRINTF("%i %i...\n",self->current,self->read);
  if (self->current <= self->read) return NULL;
  return self->buffer[(self->read++)%0xff];
}


////////////////////////////////////////////////////////////////////////////////
// pubfunctions
static int PubConsole_clear (PState state,PubFunction_t *f, int n)
{
  Console_clear();
  return 0;
}

static int PubConsole_put(PState state,PubFunction_t *f, int n)
{
  char *chr;
  unsigned int style,r,g,b,rgb;
  int x;

  x = FunctionPublish_getArg(state,5,LUXI_CLASS_STRING,&chr, LUXI_CLASS_INT,&style,
    LUXI_CLASS_INT, &r, LUXI_CLASS_INT, &g, LUXI_CLASS_INT, &b);
  if (x>n) x = n;
  if (x<1) return FunctionPublish_returnError(state,"first arg must be string");
  if (x<2) style = 0;
  if (x<5) {r = 0xff; g = 0xff; b=0xff;}
  rgb = (r&0xff)<<16|(g&0xff)<<8|(b&0xff);
  Console_put(chr,style,rgb24to15(rgb));
  return 0;
}

static int PubConsole_get(PState state,PubFunction_t *f, int n)
{
  unsigned char chr;
  int style,r,g,b,rgb;
  int x,y;

  if (FunctionPublish_getArg(state,2,LUXI_CLASS_INT, &x, LUXI_CLASS_INT, &y)!=2)
    return FunctionPublish_returnError(state,"2 ints expected");
  chr = Console_get (x,y, &style, &rgb);
  rgb15torgb(rgb,r,g,b);

  return FunctionPublish_setRet(state,5,LUXI_CLASS_CHAR,(int)chr,LUXI_CLASS_INT,r,LUXI_CLASS_INT,g,LUXI_CLASS_INT,b,LUXI_CLASS_INT,style);
}

static int PubConsole_pos(PState state,PubFunction_t *f, int n)
{
  if (n!=0 && n!=2) return FunctionPublish_returnError(state,"2 or 0 arguments required");
  if (n == 0) {
    unsigned short x,y;
    Console_getPos(&x,&y);
    return FunctionPublish_setRet(state,2,LUXI_CLASS_INT,(int)x,LUXI_CLASS_INT,(int)y);
  } else {
    static unsigned int x,y;
    if (FunctionPublish_getArg(state,2,LUXI_CLASS_INT,&x, LUXI_CLASS_INT,&y)!=2)
      return FunctionPublish_returnError(state,"arg1 and arg2 must be numbers");
    Console_setPos((unsigned short) x, (unsigned short) y);
    return 0;
  }
  return 0;
}

#define KEYIN_QUEUE_SIZE 42
static int l_keys[KEYIN_QUEUE_SIZE];
static unsigned int l_keyStart = 0, l_keyEnd = 0;

static void PubConsole_keyIn (int key)
{
//  LUX_PRINTF("Keypress: %i\n",key);

  switch(key) {
  case LUXI_KEY_BACKSPACE:
    key = 8;
    break;
  case LUXI_KEY_TAB:
    key = 9;
    break;
  case LUXI_KEY_ENTER:
    key = 13;
    break;
  default:
    break;
  }

  l_keys[l_keyEnd++%KEYIN_QUEUE_SIZE] = key;
  if (l_keyEnd==l_keyStart) l_keyStart++;
}

static int PubConsole_popKey (PState state,PubFunction_t *f, int n)
{
  int keycode;
  if (l_keyStart==l_keyEnd) return 0;
  keycode = l_keys[l_keyStart++%KEYIN_QUEUE_SIZE];
  return FunctionPublish_setRet(state,2,LUXI_CLASS_CHAR,keycode,LUXI_CLASS_INT,keycode);
}

static int PubConsole_active (PState state,PubFunction_t *f, int n)
{
  bool8 isactive;
  if (n==0) return FunctionPublish_returnBool(state,(char)Console_isActive());
  if (FunctionPublish_getArg(state,1,LUXI_CLASS_BOOLEAN,&isactive)!=1)
    return FunctionPublish_returnError(state,"arg1 must be boolean (or compatible)");
  Console_setActive(isactive);
  return 0;
}

//#define BUFFER_SIZE 4096
//static char l_errBuffer[BUFFER_SIZE], *l_errBufferPos = l_errBuffer;
//static char l_stdBuffer[BUFFER_SIZE], *l_stdBufferPos = l_errBuffer;

static int PubConsole_popStd (PState state,PubFunction_t *f, int n) {
  const char *std;

  std = Buffer_pop(l_std);
//  LUX_PRINTF("%.3s\n",std);
  if (std == NULL)
    return 0;

  return FunctionPublish_returnString(state,(char*)std);
}

static int PubConsole_popErr (PState state,PubFunction_t *f, int n) {
  const char *err;

  err = Buffer_pop(l_err);
  if (err == NULL) return 0;

  return FunctionPublish_returnString(state,(char*)err);
}

////////////////////////////////////////////////////////////////////////////////
// buffer access
static void PubConsole_printErr(char *out)
{
  Buffer_push(l_err,out);
/*  static int max;

  if (l_errBufferPos<l_errBuffer) l_errBufferPos = l_errBuffer;
  max = (int)l_errBufferPos-(int)l_errBuffer-1;
  if (max>BUFFER_SIZE) return;*/
//  strcpy(out,l_errBufferPos);
//  l_errBufferPos+=strlen(out);
}

static void PubConsole_printStd(char *out)
{
  Buffer_push(l_std,out);
/*  static int max;

  if (l_stdBufferPos<l_stdBuffer) l_stdBufferPos = l_stdBuffer;
  max = (int)l_stdBufferPos-(int)l_stdBuffer-1;
  if (max>BUFFER_SIZE) return;
  strcpy(out,l_stdBufferPos);
  l_stdBufferPos+=strlen(out);*/
}

static int PubConsole_width(PState state,PubFunction_t *f, int n)
{
  return FunctionPublish_returnInt(state,Console_getWidth());
}

static int PubConsole_height(PState state,PubFunction_t *f, int n)
{
  return FunctionPublish_returnInt(state,Console_getHeight());
}

void PubClass_Console_deInit()
{
  Buffer_free(l_std);
  Buffer_free(l_err);

  l_std = NULL;
  l_err = NULL;

  Console_setPrintErr(NULL);
  Console_setPrintStd(NULL);
}

void PubClass_Console_init()
{
  l_std = Buffer_new();
  l_err = Buffer_new();

  FunctionPublish_initClass(LUXI_CLASS_CONSOLE,
    "console","The console of luxinia is nothing more than \
 a screen containing lot's of letters. Each letter has a color and a style value that \
 can be set individually. The console has also a keyinput queue that represents the \
 keys that the user has pressed since the last frame. This queue is only filled if the \
 console is active.",
    PubClass_Console_deInit,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_CONSOLE,PubConsole_put,NULL,"put",
    "():(string text,[int style],[int r,g,b]) - writes the string given by text \
to the console. Stylecodes are: \n\
* 1: Bold\n\
* 2: Italic\n\
* 4: Underline\n\
* 8: Strikeout\n\
* 16: Smaller\n\
If you want to combine the values, just make the sum of the styles you want to use\
(i.e. 2+4+16).");
  FunctionPublish_addFunction(LUXI_CLASS_CONSOLE,PubConsole_get,NULL,"get",
    "(char c, int r,g,b,style):(int x,y) - Returns character information at given x,y");
  FunctionPublish_addFunction(LUXI_CLASS_CONSOLE,PubConsole_popKey,NULL,"popKey",
    "([char key],[int keycode]):() - returns a key and its keycode, that was pressed or nil. tab,backspace and enter get converted to regular keycodes, all other special keys stay as original keycode");
  FunctionPublish_addFunction(LUXI_CLASS_CONSOLE,PubConsole_pos,NULL,"pos",
    "([int x,y]):([int x,y]) - returns position or sets position");
  FunctionPublish_addFunction(LUXI_CLASS_CONSOLE,PubConsole_clear,NULL,"clear",
    "():() - clears the console screen");
  FunctionPublish_addFunction(LUXI_CLASS_CONSOLE,PubConsole_active,NULL,"active",
    "([boolean active]):([boolean active]) - returns activestatus if no arguments given and sets activestatus if arg is boolean");
  FunctionPublish_addFunction(LUXI_CLASS_CONSOLE,PubConsole_width,NULL,"width",
    "(int width):() - returns width of console");
  FunctionPublish_addFunction(LUXI_CLASS_CONSOLE,PubConsole_height,NULL,"height",
    "(int height):() - returns height of console");
  FunctionPublish_addFunction(LUXI_CLASS_CONSOLE,PubConsole_popErr,NULL,"poperr",
    "([string err]):() - returns (if available) a line that was printed to error stream.\
 If you like to provide an mechanism that prints out information that comes from the Luxinia C core,\
 you need to pop the messages from this queue.");
  FunctionPublish_addFunction(LUXI_CLASS_CONSOLE,PubConsole_popStd,NULL,"popstd",
    "([string err]):() - returns (if available) a line that was printed to standard stream.\
 If you like to provide an mechanism that prints out information that comes from the Luxinia C core,\
 you need to pop the messages from this queue.");

  Console_setKeyIn(PubConsole_keyIn);
  Console_setPrintErr(PubConsole_printErr);
  Console_setPrintStd(PubConsole_printStd);

  Console_printf(LUX_VERSION);
  //Console_printf("The Console is initialised");
}
