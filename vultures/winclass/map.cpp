/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

extern "C" {
	#include "hack.h"
	
	extern "C" short glyph2tile[];
}

#include "vultures_gra.h"
#include "vultures_gfl.h"
#include "vultures_win.h"
#include "vultures_txt.h"
#include "vultures_sdl.h"
#include "vultures_mou.h"
#include "vultures_tile.h"

#include "map.h"
#include "hotspot.h"
#include "levelwin.h"
#include "textwin.h"
#include "mapdata.h"

#define SYMBOL_WIDTH   7
#define SYMBOL_HEIGHT 14

window *mapwin = NULL;


map::map(levelwin *p, mapdata *data) : window(p), map_data(data)
{
	int i;
	SDL_Surface *image;

	v_type = V_WINTYPE_MAP;
	autobg = 1;
	need_redraw = 1;
	w = 640;
	h = 480;
	x = (parent->w - w) / 2;
	y = (parent->h - h) / 2;
	abs_x = parent->abs_x + x;
	abs_y = parent->abs_y + y;

	/* Load map parchment */
	mapbg = vultures_load_graphic(V_FILENAME_MAP_PARCHMENT);

	/* level title */
	textwin *txt = new textwin(this, "");
	txt->x = w/2;
	txt->y = 48;

	/* close button */
	hotspot *close = new hotspot(this, 598, 11, 28, 24, 1, "Close map");
	close->abs_x = abs_x + close->x;
	close->abs_y = abs_y + close->y;

	/* Load map symbols */
	image = vultures_load_graphic(V_FILENAME_MAP_SYMBOLS);
	if (image == NULL)
		return;
	else {
		for (i = 0; i < V_MAX_MAP_SYMBOLS; i++)
			map_symbols[i] = vultures_get_img_src(
				(i%40)*SYMBOL_WIDTH,
				(i/40)*SYMBOL_HEIGHT,
				(i%40)*SYMBOL_WIDTH + (SYMBOL_WIDTH - 1),
				(i/40)*SYMBOL_HEIGHT+ (SYMBOL_HEIGHT - 1),
				image);
		SDL_FreeSurface(image);
	}
}

map::~map()
{
	int i;

	/* free the map symbols */
	for (i = 0; i < V_MAX_MAP_SYMBOLS; i++)
		SDL_FreeSurface(map_symbols[i]);
	
	SDL_FreeSurface(mapbg);
}

bool map::draw()
{
	int map_x, map_y;
	int i, j;
	window * txt = first_child;
	char buffer[256];

	/* Draw parchment */
	vultures_put_img(abs_x, abs_y, mapbg);

	/* Draw the level name */
#ifdef VULTURESCLAW
	describe_level(buffer, TRUE);
#else
	if (!describe_level(buffer))
		sprintf(buffer, "%s, level %d ", dungeons[u.uz.dnum].dname, depth(&u.uz));
#endif
	txt->set_caption(buffer);
	txt->x = (w - vultures_text_length(V_FONT_HEADLINE, txt->caption)) / 2;
	txt->abs_x = abs_x + txt->x;
	txt->abs_y = abs_y + txt->y;
	vultures_put_text_shadow(V_FONT_HEADLINE, txt->caption, vultures_screen,
	                      txt->abs_x, txt->abs_y, CLR32_BROWN, CLR32_BLACK_A50);

	/* Find upper left corner of map on parchment */
	map_x = abs_x + 39;
	map_y = abs_y + 91;

	/* Draw map on parchment, and create hotspots */
	for (i = 0; i < ROWNO; i++)
		for (j = 1; j < COLNO; j++) {
			int map_glyph = map_data->get_glyph(MAP_GLYPH, j, i);
			bool is_dark = (map_data->get_glyph(MAP_DARKNESS, j, i) == 2);
			
			if (map_glyph != NO_GLYPH &&
				(map_glyph != cmap_to_glyph(S_stone) ||
				(level.locations[j][i].seenv && is_dark))) {
				vultures_put_img(
						map_x + SYMBOL_WIDTH*j,
						map_y + SYMBOL_HEIGHT*i,
						map_symbols[glyph2tile[map_glyph]]); 
			}
		}

	vultures_invalidate_region(abs_x, abs_y, w, h);

	return false;
}


void map::toggle(void)
{
	if (!mapwin) {
		mapwin = new map(levwin, ::map_data);

		mapwin->x = (mapwin->parent->w - mapwin->w) / 2;
		mapwin->y = (mapwin->parent->h - mapwin->h) / 2;
	}
	else {
		mapwin->hide();
		delete mapwin;
		mapwin = NULL;
	}
}


eventresult map::handle_timer_event(window* target, void* result, int time)
{
	string ttext;
	point mappos = get_mouse_mappos();
	
	if (time < HOVERTIMEOUT)
		return V_EVENT_HANDLED_NOREDRAW;

	/* draw the tooltip for the close button */
	if (this != target && target->menu_id == 1)
		vultures_mouse_set_tooltip(target->caption);
	/* draw a tooltip for the map location */
	else if (mappos.x != -1) {
		ttext = map_data->map_square_description(mappos, 1);
		if(!ttext.empty())
			vultures_mouse_set_tooltip(ttext);
	}

	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult map::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	vultures_set_mcursor(V_CURSOR_NORMAL);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult map::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	point mappos = get_mouse_mappos();
	
	/* handler != target if the user clicked on the X in the upper right corner */
	if (this != target && target->menu_id == 1) {
		toggle();
		vultures_mouse_invalidate_tooltip(1);
		return V_EVENT_HANDLED_REDRAW;
	}

	/* only handle clicks on valid map locations */
	if (mappos.x != -1)
		return map_data->handle_click(result, button, mappos);
	
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult map::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	x = (parent->w - w) / 2;
	y = (parent->h - h) / 2;
	return V_EVENT_HANDLED_NOREDRAW;
}


point map::get_mouse_mappos(void)
{
	point mouse, mappos;

	mouse = vultures_get_mouse_pos();
	mappos.x = (mouse.x - abs_x - 39) / SYMBOL_WIDTH;
	mappos.y = (mouse.y - abs_y - 91) / SYMBOL_HEIGHT;

	if (mappos.x < 1 || mappos.x >= COLNO ||
		mappos.y < 0 || mappos.y >= ROWNO) {
		mappos.x = -1;
		mappos.y = -1;
	}
	
	return mappos;
}
