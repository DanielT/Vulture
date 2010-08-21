/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vulture_txt_h_
#define _vulture_txt_h_

#include <string>
#include "SDL_ttf.h"


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
extern int vulture_load_font (int font_id, const char *ttf_filename,
                               int fontindex, int pointsize);

extern int vulture_put_text (int font_id, std::string str, SDL_Surface *dest,
                              int x, int y, Uint32 color);

extern int vulture_put_text_shadow (int font_id, std::string str, SDL_Surface *dest,
                                     int x, int y, Uint32 textcolor, Uint32 shadowcolor);
extern void vulture_put_text_multiline(int font_id, std::string str, SDL_Surface *dest,
                                       int x, int y, Uint32 color, Uint32 shadowcolor, int maxlen);

extern int vulture_text_length (int font_id, std::string str);

extern int vulture_text_height (int font_id, std::string str);

extern int vulture_get_lineheight(int font_id);

extern void vulture_free_fonts();

#endif
