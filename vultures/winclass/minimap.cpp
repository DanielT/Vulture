/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

extern "C" {
	#include "hack.h"
}

#include "vultures_main.h"
#include "vultures_win.h"
#include "vultures_opt.h"
#include "vultures_gfl.h"
#include "vultures_gra.h"
#include "vultures_mou.h"
#include "vultures_tile.h"

#include "minimap.h"
#include "levelwin.h"

minimap *minimapwin = NULL;

minimap::minimap(levelwin *p, mapdata *data) : window(p), level(p), map_data(data)
{
	minimapbg = vultures_load_graphic(V_FILENAME_MINIMAPBG);
	w = minimapbg->w;
	h = minimapbg->h;
	x = parent->w - (w + 6);
	y = 6;
	visible = vultures_opts.show_minimap;
	menu_id = V_WIN_MINIMAP;
	autobg = 1;
	
	minimapwin = this;
}


minimap::~minimap()
{
	SDL_FreeSurface(minimapbg);
	minimapwin = NULL;
}


bool minimap::draw()
{
	int map_x, map_y, sym;
	Uint32 *pixels;
	SDL_Rect destrect = {0, 0, 3, 2};

	Uint32 minimap_colors[V_MMTILE_MAX] =
	    {CLR32_BLACK, V_COLOR_MINI_FLOOR, V_COLOR_MINI_STAIRS,
	     V_COLOR_MINI_DOOR, V_COLOR_MINI_YOU, CLR32_GREEN, CLR32_RED};
	
	if (!minimapbg)
		return false;
		
	if (this->background)
		vultures_put_img(abs_x, abs_y, background);


	for (map_y = 0; map_y < ROWNO; map_y++)
	{
		for (map_x = 1; map_x < COLNO; map_x++)
		{
			/* translate the contents of the map into a minimap symbol color */
			switch(map_data->get_glyph(MAP_BACK, map_x, map_y))
			{
				case V_TILE_WALL_GENERIC:
				case V_MISC_UNMAPPED_AREA:
					sym = V_MMTILE_NONE; break;

				default:
					sym = V_MMTILE_FLOOR; break;
			}

			switch(map_data->get_glyph(MAP_FURNITURE, map_x, map_y))
			{
				case V_MISC_STAIRS_UP:
				case V_MISC_STAIRS_DOWN:
					sym = V_MMTILE_STAIRS; break;

				case V_MISC_VDOOR_WOOD_OPEN:
				case V_MISC_VDOOR_WOOD_CLOSED:
				case V_MISC_HDOOR_WOOD_OPEN:
				case V_MISC_HDOOR_WOOD_CLOSED:
				case V_MISC_VODBRIDGE:
				case V_MISC_HODBRIDGE:
				case V_MISC_VCDBRIDGE:
				case V_MISC_HCDBRIDGE:
					sym = V_MMTILE_DOOR; break;
			}

			if (map_data->get_glyph(MAP_TRAP, map_x, map_y) == V_MISC_MAGIC_PORTAL)
				sym = V_MMTILE_STAIRS;

			if (map_data->get_glyph(MAP_MON, map_x, map_y) != V_TILE_NONE)
			{
				if (map_data->get_glyph(MAP_PET, map_x, map_y))
					sym = V_MMTILE_PET;
				else
					sym = V_MMTILE_MONSTER;
			}

			if ((map_y == u.uy) && (map_x == u.ux))
				sym = V_MMTILE_YOU;

			/* draw symbols that changed onto the "minimap surface" */
			if (sym != vultures_minimap_syms[map_y][map_x])
			{
				vultures_minimap_syms[map_y][map_x] = sym;

				destrect.x = 40 + 2*map_x - 2*map_y;
				destrect.y = map_x + map_y;

				pixels = (Uint32 *)((char*)minimapbg->pixels + 
						minimapbg->pitch * (destrect.y+6) + (destrect.x+6) * 4);

				/* A minimap symbol has this shape: _ C _
				*                                  C C C
				* where "_" is transparent and "C" is a colored pixel */

				/* row 1: */
				/* pixels[0] = transparent -> dont write */
				pixels[1] = minimap_colors[sym];
				/* pixels[2] = transparent -> dont write */

				/* row 2 */
				pixels = (Uint32 *)((char*)minimapbg->pixels + 
						minimapbg->pitch * (destrect.y+7) + (destrect.x+6) * 4);
				pixels[0] = minimap_colors[sym];
				pixels[1] = minimap_colors[sym];
				pixels[2] = minimap_colors[sym];
			}

		}
	}

	vultures_put_img(abs_x, abs_y, minimapbg);

	vultures_invalidate_region(abs_x, abs_y, w, h);

	return false;
}


eventresult minimap::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	vultures_set_mcursor(V_CURSOR_NORMAL);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult minimap::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	point mouse, mappos;
	int offs_x, offs_y;

	/* translate the mouse position to a map coordinate */
	mouse = vultures_get_mouse_pos();

	offs_x = mouse.x - abs_x - 6 - 40;
	offs_y = mouse.y - abs_y - 6;
	mappos.x = ( offs_x + 2*offs_y)/4;
	mappos.y = (-offs_x + 2*offs_y)/4;
	
	if (mappos.x < 1 || mappos.x > COLNO ||
		mappos.y < 0 || mappos.y > ROWNO)
		return V_EVENT_UNHANDLED;

	/* limited mouse event handling, due to the fact that the minimap
	* is too small for precise targeting */
	/* left button: direct selection of a location (for very imprecise teleport) */
	if (button == SDL_BUTTON_LEFT) {
		if (vultures_whatis_active) {
			((vultures_event*)result)->num = 0;
			((vultures_event*)result)->x = mappos.x;
			((vultures_event*)result)->y = mappos.y;
			return V_EVENT_HANDLED_FINAL;
		}

		((vultures_event*)result)->num = map_data->perform_map_action(V_ACTION_TRAVEL, mappos);
		return V_EVENT_HANDLED_FINAL;
	}
	/* right button: travel to location */
	else if(button == SDL_BUTTON_RIGHT) {
		levwin->set_view(mappos.x, mappos.y);
		levwin->need_redraw = 1;
		return V_EVENT_HANDLED_REDRAW;
	}

	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult minimap::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	x = parent->w - (w + 6);
	return V_EVENT_HANDLED_NOREDRAW;
}
