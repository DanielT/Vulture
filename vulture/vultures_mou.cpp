/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>

#include "SDL.h"

#include "vultures_types.h"
#include "vultures_mou.h"
#include "vultures_gra.h"
#include "vultures_sdl.h"
#include "vultures_txt.h"
#include "vultures_tile.h"


static struct {
	point cur, prev;
	SDL_Surface *background;
	int cursor, prevcursor;
} vultures_mouse = {{0, 0}, {0, 0}, NULL, 0, 0};


static struct {
	point cur;
	SDL_Surface *background, *tip;
  std::string text;
	int valid;
	SDL_Rect refresh;
} vultures_tooltip = {{0, 0}, NULL, NULL, "", 0, {0,0,0,0}};

static Uint32 tooltipcolor;


static void vultures_mouse_get_bg(void);
static void vultures_mouse_get_tt_bg(void);



void vultures_mouse_init(void)
{
	vultures_mouse.prevcursor = CURTILEOFFSET;
	vultures_mouse.cursor = CURTILEOFFSET;

	/* slightly transparent off-white */
	tooltipcolor = SDL_MapRGBA(vultures_px_format, 0xff, 0xff, 0xf0, 0xd0);
}



void vultures_mouse_destroy(void)
{
	if (vultures_mouse.background)
		SDL_FreeSurface(vultures_mouse.background);

	if (vultures_tooltip.tip)
		SDL_FreeSurface(vultures_tooltip.tip);

	if (vultures_tooltip.background)
		SDL_FreeSurface(vultures_tooltip.background);
}



void vultures_set_mcursor(int cursornum)
{
	vultures_tile * prevcursor;

	if (cursornum < 0 || cursornum >= V_CURSOR_MAX)
		return;

	vultures_mouse.prevcursor = vultures_mouse.cursor;
	vultures_mouse.cursor = (CURTILEOFFSET + cursornum);

	prevcursor = vultures_get_tile(vultures_mouse.prevcursor);

	/* refresh region of previous cursor */
	if (vultures_mouse.cursor != vultures_mouse.prevcursor)
		vultures_refresh_region(vultures_mouse.cur.x + prevcursor->xmod,
						vultures_mouse.cur.y + prevcursor->ymod,
						vultures_mouse.cur.x + prevcursor->xmod +
												prevcursor->graphic->w,
						vultures_mouse.cur.y + prevcursor->ymod +
												prevcursor->graphic->h);
}



point vultures_get_mouse_pos(void)
{
	return vultures_mouse.cur;
}


point vultures_get_mouse_prev_pos(void)
{
	return vultures_mouse.prev;
}


void vultures_set_mouse_pos(int x, int y)
{
	vultures_mouse.prev = vultures_mouse.cur;
	vultures_mouse.cur.x = x;
	vultures_mouse.cur.y = y;
}


void vultures_mouse_draw(void)
{
	int new_x, new_y;
	vultures_tile *cursor;

	vultures_mouse_get_bg();

	cursor = vultures_get_tile(vultures_mouse.cursor);

	new_x = vultures_mouse.cur.x + cursor->xmod;
	new_y = vultures_mouse.cur.y + cursor->ymod;

	/* draw the mouse */
	vultures_put_tile(vultures_mouse.cur.x, vultures_mouse.cur.y, vultures_mouse.cursor);


	/* draw a new tooltip, if necessary */
	if (vultures_tooltip.valid && vultures_tooltip.tip) {
		if (vultures_tooltip.cur.x == -1) {
			if (new_x + vultures_tooltip.tip->w > vultures_screen->w)
				vultures_tooltip.cur.x = vultures_screen->w - vultures_tooltip.tip->w;
			else if (new_x + cursor->xmod < 0)
				vultures_tooltip.cur.x = 0;
			else
				vultures_tooltip.cur.x = new_x;

			if (new_y + cursor->graphic->h + vultures_tooltip.tip->h > vultures_screen->h)
				vultures_tooltip.cur.y = vultures_screen->h - vultures_tooltip.tip->h;
			else if (new_y + cursor->ymod < 0)
				vultures_tooltip.cur.y = 0;
			else
				vultures_tooltip.cur.y = new_y + cursor->graphic->h;
		}

		vultures_mouse_get_tt_bg();

		vultures_put_img(vultures_tooltip.cur.x,vultures_tooltip.cur.y,vultures_tooltip.tip);
		vultures_refresh_region(vultures_tooltip.cur.x, vultures_tooltip.cur.y,
						vultures_tooltip.cur.x + vultures_tooltip.tip->w,
						vultures_tooltip.cur.y + vultures_tooltip.tip->h);
	}
}



void vultures_mouse_refresh(void)
{
	int old_x, old_y, new_x, new_y;
	vultures_tile *cursor, *prevcursor;

	cursor = vultures_get_tile(vultures_mouse.cursor);
	prevcursor = vultures_get_tile(vultures_mouse.prevcursor);

	old_x = vultures_mouse.prev.x + prevcursor->xmod;
	old_y = vultures_mouse.prev.y + prevcursor->ymod;
	new_x = vultures_mouse.cur.x + cursor->xmod;
	new_y = vultures_mouse.cur.y + cursor->ymod;

	vultures_refresh_region(old_x-1, old_y-1, old_x + prevcursor->graphic->w,
							old_y + prevcursor->graphic->h);
	if (old_x != new_x || old_y != new_y)
		vultures_refresh_region(new_x-1, new_y-1, new_x + cursor->graphic->w,
								new_y + cursor->graphic->h);

	/* refresh the position of the previous tooltip */
	if (vultures_tooltip.refresh.x != -1) {
		vultures_refresh_region(vultures_tooltip.refresh.x, vultures_tooltip.refresh.y,
						vultures_tooltip.refresh.x + vultures_tooltip.refresh.w,
						vultures_tooltip.refresh.y + vultures_tooltip.refresh.h);
		vultures_tooltip.refresh.x = -1;
	}
}



void vultures_mouse_restore_bg()
{
	vultures_tile * cursor;

	if (vultures_tooltip.background) {
		vultures_put_img(vultures_tooltip.cur.x, vultures_tooltip.cur.y, vultures_tooltip.background);
		SDL_FreeSurface(vultures_tooltip.background);
		vultures_tooltip.background = NULL;

		/* remember where to refresh later */
		vultures_tooltip.refresh.x = vultures_tooltip.cur.x;
		vultures_tooltip.refresh.y = vultures_tooltip.cur.y;
		vultures_tooltip.refresh.w = vultures_tooltip.tip->w;
		vultures_tooltip.refresh.h = vultures_tooltip.tip->h;
	}

	if (vultures_mouse.background) {
		cursor = vultures_get_tile(vultures_mouse.cursor);

		vultures_put_img(vultures_mouse.cur.x + cursor->xmod,
					vultures_mouse.cur.y + cursor->ymod,
					vultures_mouse.background);
		SDL_FreeSurface(vultures_mouse.background);
		vultures_mouse.background = NULL;
	}
}



static void vultures_mouse_get_bg(void)
{
	vultures_tile * cursor;

	if (vultures_mouse.background)
		SDL_FreeSurface(vultures_mouse.background);

	cursor = vultures_get_tile(vultures_mouse.cursor);

	vultures_mouse.background = 
		vultures_get_img(vultures_mouse.cur.x + cursor->xmod,
					vultures_mouse.cur.y + cursor->ymod,
					vultures_mouse.cur.x + cursor->xmod + cursor->graphic->w,
					vultures_mouse.cur.y + cursor->ymod + cursor->graphic->h);
}


static void vultures_mouse_get_tt_bg(void)
{
	if (vultures_tooltip.tip && vultures_tooltip.valid) {
		if (vultures_tooltip.background)
			SDL_FreeSurface(vultures_tooltip.background);

		vultures_tooltip.background = vultures_get_img(vultures_tooltip.cur.x, vultures_tooltip.cur.y,
												vultures_tooltip.cur.x + vultures_tooltip.tip->w,
												vultures_tooltip.cur.y + vultures_tooltip.tip->h);
	}
}



void vultures_mouse_invalidate_tooltip(int force)
{
	vultures_tile * cursor = vultures_get_tile(vultures_mouse.cursor);

	/* invalidate only for "significant" movement. (significant == more than 4 px ...) */
	if (force ||
		abs(vultures_mouse.cur.x - (vultures_tooltip.cur.x -
									cursor->xmod)) > 4 ||
		abs(vultures_mouse.cur.y - (vultures_tooltip.cur.y -
									cursor->ymod - cursor->graphic->h)) > 4) {
		vultures_tooltip.valid = 0;
		vultures_tooltip.cur.x = -1;
		vultures_tooltip.cur.x = -1;
	}
}



void vultures_mouse_set_tooltip(std::string str)
{
	int length, height;

	if (!str.empty() && str != vultures_tooltip.text) {
		vultures_tooltip.text = str;

		if (vultures_tooltip.background)
			SDL_FreeSurface(vultures_tooltip.background);
		vultures_tooltip.background = NULL;

		if (vultures_tooltip.tip)
			SDL_FreeSurface(vultures_tooltip.tip);
		length = vultures_text_length(V_FONT_TOOLTIP, str) + 6;
		height = vultures_text_height(V_FONT_TOOLTIP, str) + 6;

		vultures_tooltip.tip = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA,
				length, height, vultures_px_format->BitsPerPixel,
				vultures_px_format->Rmask,
				vultures_px_format->Gmask,
				vultures_px_format->Bmask,
				vultures_px_format->Amask);

		SDL_FillRect(vultures_tooltip.tip, NULL, tooltipcolor);
		vultures_rect_surface(vultures_tooltip.tip, 0, 0, length-1, height-1, V_COLOR_BACKGROUND);
		vultures_put_text(V_FONT_TOOLTIP, str, vultures_tooltip.tip,
						3, 3, V_COLOR_BACKGROUND);
	}

	if (vultures_tooltip.tip)
		vultures_tooltip.valid = 1;
	else
		vultures_tooltip.valid = 0;

}


