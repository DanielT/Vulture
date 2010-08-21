/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>

#include "SDL.h"

#include "vulture_types.h"
#include "vulture_mou.h"
#include "vulture_gra.h"
#include "vulture_sdl.h"
#include "vulture_txt.h"
#include "vulture_tile.h"


static struct {
	point cur, prev;
	SDL_Surface *background;
	int cursor, prevcursor;
} vulture_mouse = {{0, 0}, {0, 0}, NULL, 0, 0};


static struct {
	point cur;
	SDL_Surface *background, *tip;
  std::string text;
	int valid;
	SDL_Rect refresh;
} vulture_tooltip = {{0, 0}, NULL, NULL, "", 0, {0,0,0,0}};

static Uint32 tooltipcolor;


static void vulture_mouse_get_bg(void);
static void vulture_mouse_get_tt_bg(void);



void vulture_mouse_init(void)
{
	vulture_mouse.prevcursor = CURTILEOFFSET;
	vulture_mouse.cursor = CURTILEOFFSET;

	/* slightly transparent off-white */
	tooltipcolor = SDL_MapRGBA(vulture_px_format, 0xff, 0xff, 0xf0, 0xd0);
}



void vulture_mouse_destroy(void)
{
	if (vulture_mouse.background)
		SDL_FreeSurface(vulture_mouse.background);

	if (vulture_tooltip.tip)
		SDL_FreeSurface(vulture_tooltip.tip);

	if (vulture_tooltip.background)
		SDL_FreeSurface(vulture_tooltip.background);
}



void vulture_set_mcursor(int cursornum)
{
	vulture_tile * prevcursor;

	if (cursornum < 0 || cursornum >= V_CURSOR_MAX)
		return;

	vulture_mouse.prevcursor = vulture_mouse.cursor;
	vulture_mouse.cursor = (CURTILEOFFSET + cursornum);

	prevcursor = vulture_get_tile(vulture_mouse.prevcursor);

	/* refresh region of previous cursor */
	if (vulture_mouse.cursor != vulture_mouse.prevcursor)
		vulture_refresh_region(vulture_mouse.cur.x + prevcursor->xmod,
						vulture_mouse.cur.y + prevcursor->ymod,
						vulture_mouse.cur.x + prevcursor->xmod +
												prevcursor->graphic->w,
						vulture_mouse.cur.y + prevcursor->ymod +
												prevcursor->graphic->h);
}



point vulture_get_mouse_pos(void)
{
	return vulture_mouse.cur;
}


point vulture_get_mouse_prev_pos(void)
{
	return vulture_mouse.prev;
}


void vulture_set_mouse_pos(int x, int y)
{
	vulture_mouse.prev = vulture_mouse.cur;
	vulture_mouse.cur.x = x;
	vulture_mouse.cur.y = y;
}


void vulture_mouse_draw(void)
{
	int new_x, new_y;
	vulture_tile *cursor;

	vulture_mouse_get_bg();

	cursor = vulture_get_tile(vulture_mouse.cursor);

	new_x = vulture_mouse.cur.x + cursor->xmod;
	new_y = vulture_mouse.cur.y + cursor->ymod;

	/* draw the mouse */
	vulture_put_tile(vulture_mouse.cur.x, vulture_mouse.cur.y, vulture_mouse.cursor);


	/* draw a new tooltip, if necessary */
	if (vulture_tooltip.valid && vulture_tooltip.tip) {
		if (vulture_tooltip.cur.x == -1) {
			if (new_x + vulture_tooltip.tip->w > vulture_screen->w)
				vulture_tooltip.cur.x = vulture_screen->w - vulture_tooltip.tip->w;
			else if (new_x + cursor->xmod < 0)
				vulture_tooltip.cur.x = 0;
			else
				vulture_tooltip.cur.x = new_x;

			if (new_y + cursor->graphic->h + vulture_tooltip.tip->h > vulture_screen->h)
				vulture_tooltip.cur.y = vulture_screen->h - vulture_tooltip.tip->h;
			else if (new_y + cursor->ymod < 0)
				vulture_tooltip.cur.y = 0;
			else
				vulture_tooltip.cur.y = new_y + cursor->graphic->h;
		}

		vulture_mouse_get_tt_bg();

		vulture_put_img(vulture_tooltip.cur.x,vulture_tooltip.cur.y,vulture_tooltip.tip);
		vulture_refresh_region(vulture_tooltip.cur.x, vulture_tooltip.cur.y,
						vulture_tooltip.cur.x + vulture_tooltip.tip->w,
						vulture_tooltip.cur.y + vulture_tooltip.tip->h);
	}
}



void vulture_mouse_refresh(void)
{
	int old_x, old_y, new_x, new_y;
	vulture_tile *cursor, *prevcursor;

	cursor = vulture_get_tile(vulture_mouse.cursor);
	prevcursor = vulture_get_tile(vulture_mouse.prevcursor);

	old_x = vulture_mouse.prev.x + prevcursor->xmod;
	old_y = vulture_mouse.prev.y + prevcursor->ymod;
	new_x = vulture_mouse.cur.x + cursor->xmod;
	new_y = vulture_mouse.cur.y + cursor->ymod;

	vulture_refresh_region(old_x-1, old_y-1, old_x + prevcursor->graphic->w,
							old_y + prevcursor->graphic->h);
	if (old_x != new_x || old_y != new_y)
		vulture_refresh_region(new_x-1, new_y-1, new_x + cursor->graphic->w,
								new_y + cursor->graphic->h);

	/* refresh the position of the previous tooltip */
	if (vulture_tooltip.refresh.x != -1) {
		vulture_refresh_region(vulture_tooltip.refresh.x, vulture_tooltip.refresh.y,
						vulture_tooltip.refresh.x + vulture_tooltip.refresh.w,
						vulture_tooltip.refresh.y + vulture_tooltip.refresh.h);
		vulture_tooltip.refresh.x = -1;
	}
}



void vulture_mouse_restore_bg()
{
	vulture_tile * cursor;

	if (vulture_tooltip.background) {
		vulture_put_img(vulture_tooltip.cur.x, vulture_tooltip.cur.y, vulture_tooltip.background);
		SDL_FreeSurface(vulture_tooltip.background);
		vulture_tooltip.background = NULL;

		/* remember where to refresh later */
		vulture_tooltip.refresh.x = vulture_tooltip.cur.x;
		vulture_tooltip.refresh.y = vulture_tooltip.cur.y;
		vulture_tooltip.refresh.w = vulture_tooltip.tip->w;
		vulture_tooltip.refresh.h = vulture_tooltip.tip->h;
	}

	if (vulture_mouse.background) {
		cursor = vulture_get_tile(vulture_mouse.cursor);

		vulture_put_img(vulture_mouse.cur.x + cursor->xmod,
					vulture_mouse.cur.y + cursor->ymod,
					vulture_mouse.background);
		SDL_FreeSurface(vulture_mouse.background);
		vulture_mouse.background = NULL;
	}
}



static void vulture_mouse_get_bg(void)
{
	vulture_tile * cursor;

	if (vulture_mouse.background)
		SDL_FreeSurface(vulture_mouse.background);

	cursor = vulture_get_tile(vulture_mouse.cursor);

	vulture_mouse.background = 
		vulture_get_img(vulture_mouse.cur.x + cursor->xmod,
					vulture_mouse.cur.y + cursor->ymod,
					vulture_mouse.cur.x + cursor->xmod + cursor->graphic->w,
					vulture_mouse.cur.y + cursor->ymod + cursor->graphic->h);
}


static void vulture_mouse_get_tt_bg(void)
{
	if (vulture_tooltip.tip && vulture_tooltip.valid) {
		if (vulture_tooltip.background)
			SDL_FreeSurface(vulture_tooltip.background);

		vulture_tooltip.background = vulture_get_img(vulture_tooltip.cur.x, vulture_tooltip.cur.y,
												vulture_tooltip.cur.x + vulture_tooltip.tip->w,
												vulture_tooltip.cur.y + vulture_tooltip.tip->h);
	}
}



void vulture_mouse_invalidate_tooltip(int force)
{
	vulture_tile * cursor = vulture_get_tile(vulture_mouse.cursor);

	/* invalidate only for "significant" movement. (significant == more than 4 px ...) */
	if (force ||
		abs(vulture_mouse.cur.x - (vulture_tooltip.cur.x -
									cursor->xmod)) > 4 ||
		abs(vulture_mouse.cur.y - (vulture_tooltip.cur.y -
									cursor->ymod - cursor->graphic->h)) > 4) {
		vulture_tooltip.valid = 0;
		vulture_tooltip.cur.x = -1;
		vulture_tooltip.cur.x = -1;
	}
}



void vulture_mouse_set_tooltip(std::string str)
{
	int length, height;

	if (!str.empty() && str != vulture_tooltip.text) {
		vulture_tooltip.text = str;

		if (vulture_tooltip.background)
			SDL_FreeSurface(vulture_tooltip.background);
		vulture_tooltip.background = NULL;

		if (vulture_tooltip.tip)
			SDL_FreeSurface(vulture_tooltip.tip);
		length = vulture_text_length(V_FONT_TOOLTIP, str) + 6;
		height = vulture_text_height(V_FONT_TOOLTIP, str) + 6;

		vulture_tooltip.tip = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA,
				length, height, vulture_px_format->BitsPerPixel,
				vulture_px_format->Rmask,
				vulture_px_format->Gmask,
				vulture_px_format->Bmask,
				vulture_px_format->Amask);

		SDL_FillRect(vulture_tooltip.tip, NULL, tooltipcolor);
		vulture_rect_surface(vulture_tooltip.tip, 0, 0, length-1, height-1, V_COLOR_BACKGROUND);
		vulture_put_text(V_FONT_TOOLTIP, str, vulture_tooltip.tip,
						3, 3, V_COLOR_BACKGROUND);
	}

	if (vulture_tooltip.tip)
		vulture_tooltip.valid = 1;
	else
		vulture_tooltip.valid = 0;

}


