// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __CONSOLE2_H__
#define __CONSOLE2_H__

#include <stdio.h>

/*
 * The Console is a simple textscreen. It displays only characters in different
 * formats. History, commandline input, etc. has to be implemented somewhere else.
 *
 */

#define rgb24to15(rgb) (unsigned short) ( (((rgb)>>16)     / 8 )<<10 | \
                      (((rgb)>>8&0xff) / 8 )<<5 | \
                      (((rgb)&0xff)    / 8 ))
#define rgb15torgb(rgb,r,g,b) { (r) = (((rgb)>>10 & 0x1f)*8*0xff/248);\
                (g) = (((rgb)>>5 &  0x1f)*8*0xff/248);\
                (b) = (((rgb) &     0x1f)*8*0xff/248);}
  // puts a string at position and sets the position to the new line
void Console_putln (char *chr, char style, short color);

  // prints the console to a filestream, stdout for example
void Console_print(FILE *stream);

  // moves lines up
void Console_shift(unsigned short lines);

  // sets the cursors position
void Console_setPos (unsigned short x, unsigned short y);

  // gets the cursor position
void Console_getPos (unsigned short *x, unsigned short *y);

  // puts a line to the current position
void Console_put (char *chr, unsigned char style, unsigned short color);

  // returns character information at x,y, writes style and color to pointers (if not null)
unsigned char Console_get (int x, int y, int *style, int *color);

  // initializes the console
void Console_init ();

  // draws the console
void Console_draw();

  // clears the console
void Console_clear();

  // returns 0 if the console is not active
int Console_isActive ();

  // activates or deactivates the console
void Console_setActive (int on);

  // simulates a keypress
void Console_keyIn(int key);

  // callbackfunction to be called on keypressedevent
void Console_setKeyIn (void (*keyIn)(int key));

  // prints a formated string to the console
void Console_printf (const char *key,...);

  // prints special error formated string
void Console_printferr (const char *fmt,...) ;

  // callback function for error printout wrapping
void Console_setPrintErr(void (*out)(char *));

  // callback function for std printout wrapping
void Console_setPrintStd(void (*out)(char *));

  // returns height of console
int Console_getHeight();

  // returns width of console
int Console_getWidth();

void Console_deinit();
#endif
