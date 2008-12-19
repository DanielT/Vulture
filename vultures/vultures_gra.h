/* Copyright (c) Jaakko Peltonen, 2000				  */
/* Copyright (c) Daniel Thaler, 2006				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_gra_h_
#define _vultures_gra_h_

#include <SDL.h>

#define vultures_rect(x1,y1,x2,y2,cindex)      vultures_rect_surface(vultures_screen,x1,y1,x2,y2,cindex)
#define vultures_fill_rect(x1,y1,x2,y2,cindex) vultures_fill_rect_surface(vultures_screen,x1,y1,x2,y2,cindex)
#define vultures_line(x1,y1,x2,y2,cindex)      vultures_rect_surface(vultures_screen,x1,y1,x2,y2,cindex)
#define vultures_get_img(x1,y1,x2,y2)          vultures_get_img_src(x1,y1,x2,y2,vultures_screen)

extern void vultures_fade_out(double);
extern void vultures_fade_in(double);
extern void vultures_set_draw_region(int, int, int, int);
extern void vultures_rect_surface(SDL_Surface *,int, int, int, int, Uint32);
extern void vultures_fill_rect_surface(SDL_Surface *,int, int, int, int, Uint32);
extern SDL_Surface *vultures_get_img_src(int, int, int, int, SDL_Surface *);
extern void vultures_put_img(int, int, SDL_Surface *);
extern void vultures_draw_raised_frame(int x1, int y1, int x2, int y2);
extern void vultures_draw_lowered_frame(int x1, int y1, int x2, int y2);

extern SDL_PixelFormat * vultures_px_format;


#define DEF_AMASK 0xff000000

extern Uint32 CLR32_BLACK;
extern Uint32 CLR32_BLACK_A30;
extern Uint32 CLR32_BLACK_A50;
extern Uint32 CLR32_BLACK_A70;
extern Uint32 CLR32_GREEN;
extern Uint32 CLR32_YELLOW;
extern Uint32 CLR32_ORANGE;
extern Uint32 CLR32_RED;
extern Uint32 CLR32_GRAY20;
extern Uint32 CLR32_GRAY70;
extern Uint32 CLR32_GRAY77;
extern Uint32 CLR32_PURPLE44;
extern Uint32 CLR32_LIGHTPINK;
extern Uint32 CLR32_LIGHTGREEN;
extern Uint32 CLR32_BROWN;
extern Uint32 CLR32_WHITE;
extern Uint32 CLR32_BLESS_BLUE;
extern Uint32 CLR32_CURSE_RED;
extern Uint32 CLR32_GOLD_SHADE;

#define V_COLOR_BACKGROUND CLR32_BLACK

#endif /* _vultures_gra_h_ */
