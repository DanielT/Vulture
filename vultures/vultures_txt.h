/* Copyright (c) Daniel Thaler, 2006                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_txt_h_
#define _vultures_txt_h_

#include <string>
#include "SDL_ttf.h"

using std::string;

/* Font indices. Currently, there're only 2 fonts (large & small). */
#define V_FONT_SMALL 0
#define V_FONT_LARGE 1
#define V_FONT_INTRO     V_FONT_LARGE
#define V_FONT_MENU      V_FONT_SMALL
#define V_FONT_HEADLINE  V_FONT_LARGE
#define V_FONT_BUTTON    V_FONT_LARGE
#define V_FONT_TOOLTIP   V_FONT_SMALL
#define V_FONT_STATUS    V_FONT_SMALL
#define V_FONT_MESSAGE   V_FONT_SMALL
#define V_FONT_INPUT     V_FONT_SMALL

/*
* colors used to draw text
*/
#define V_COLOR_TEXT       0xffffffff
#define V_COLOR_INTRO_TEXT 0xffffffff

/*---------------------------------------------------------------------------
 Text displaying
---------------------------------------------------------------------------*/

/* load a font from a ttf file */
extern int vultures_load_font (int font_id, const char *ttf_filename,
                               int fontindex, int pointsize);

extern int vultures_put_text (int font_id, string str, SDL_Surface *dest,
                              int x, int y, Uint32 color);

extern int vultures_put_text_shadow (int font_id, string str, SDL_Surface *dest,
                                     int x, int y, Uint32 textcolor, Uint32 shadowcolor);
extern void vultures_put_text_multiline(int font_id, string str, SDL_Surface *dest,
                                       int x, int y, Uint32 color, Uint32 shadowcolor, int maxlen);

extern int vultures_text_length (int font_id, string str);

extern int vultures_text_height (int font_id, string str);

extern int vultures_get_lineheight(int font_id);

extern void vultures_free_fonts();

#endif
