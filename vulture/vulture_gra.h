/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vulture_gra_h_
#define _vulture_gra_h_

#include <SDL.h>

#define vulture_rect(x1,y1,x2,y2,cindex)      vulture_rect_surface(vulture_screen,x1,y1,x2,y2,cindex)
#define vulture_fill_rect(x1,y1,x2,y2,cindex) vulture_fill_rect_surface(vulture_screen,x1,y1,x2,y2,cindex)
#define vulture_line(x1,y1,x2,y2,cindex)      vulture_rect_surface(vulture_screen,x1,y1,x2,y2,cindex)
#define vulture_get_img(x1,y1,x2,y2)          vulture_get_img_src(x1,y1,x2,y2,vulture_screen)

extern void vulture_fade_out(double);
extern void vulture_fade_in(double);
extern void vulture_set_draw_region(int, int, int, int);
extern void vulture_rect_surface(SDL_Surface *,int, int, int, int, Uint32);
extern void vulture_fill_rect_surface(SDL_Surface *,int, int, int, int, Uint32);
extern SDL_Surface *vulture_get_img_src(int, int, int, int, SDL_Surface *);
extern void vulture_put_img(int, int, SDL_Surface *);
extern void vulture_draw_raised_frame(int x1, int y1, int x2, int y2);
extern void vulture_draw_lowered_frame(int x1, int y1, int x2, int y2);

extern SDL_PixelFormat * vulture_px_format;


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

#endif /* _vulture_gra_h_ */
