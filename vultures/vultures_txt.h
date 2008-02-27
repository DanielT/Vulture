/* Copyright (c) Daniel Thaler, 2006                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_txt_h_
#define _vultures_txt_h_

#include "SDL_ttf.h"


/*---------------------------------------------------------------------------
 Text displaying
---------------------------------------------------------------------------*/

/* load a font from a ttf file */
extern int vultures_load_font (int font_id, const char * ttf_filename,
                               int fontindex, int pointsize);

extern int vultures_put_text (int font_id, const char *str, SDL_Surface *dest,
                              int x, int y, Uint32 color);

extern int vultures_put_text_shadow (int font_id, const char *str, SDL_Surface *dest,
                                     int x, int y, Uint32 textcolor, Uint32 shadowcolor);
extern void vultures_put_text_multiline(int font_id, const char *str, SDL_Surface *dest,
                                       int x, int y, Uint32 color, Uint32 shadowcolor, int maxlen);

extern int vultures_text_length (int font_id, const char *str);

extern int vultures_text_height (int font_id, const char *str);

extern int vultures_get_lineheight(int font_id);

extern void vultures_free_fonts();

#endif
