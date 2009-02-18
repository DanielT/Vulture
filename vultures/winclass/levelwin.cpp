/* NetHack may be freely redistributed.  See license for details. */

extern "C" {
	#include "hack.h"
}

#include "levelwin.h"
#include "hotspot.h"
#include "minimap.h"
#include "toolbar.h"
#include "contextmenu.h"
#include "map.h"
#include "messagewin.h"


#include "vultures_win.h"
#include "vultures_sdl.h"
#include "vultures_gra.h"
#include "vultures_opt.h"
#include "vultures_tile.h"
#include "vultures_mou.h"
#include "vultures_gen.h"
#include "vultures_main.h"

/* ??????????
 * without this the file will occasionally fail to compile, seemingly at random
 */
#undef min
#undef max
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define VULTURES_CLIPMARGIN 200


levelwin *levwin;
int vultures_map_draw_msecs = 0;
int vultures_map_draw_lastmove = 0;
int vultures_map_highlight_objects = 0;


levelwin::levelwin(mapdata *data) : window(NULL), map_data(data)
{
	v_type = V_WINTYPE_LEVEL;
	
	x = 0;
	y = 0;
	w = vultures_screen->w;
	h = vultures_screen->h;
	need_redraw = 1;


	/* upper left scroll hotspot */
	new hotspot(this, 0,      0,      20,     20,     V_HOTSPOT_SCROLL_UPLEFT,    "scroll diagonally");
	/* upper right scroll hotspot */
	new hotspot(this, w - 20, 0,      20,     20,     V_HOTSPOT_SCROLL_UPRIGHT,   "scroll diagonally");
	/* left scroll hotspot */
	new hotspot(this, 0,      20,     20,     h - 40, V_HOTSPOT_SCROLL_LEFT,      "scroll left");
	/* right scroll hotspot */
	new hotspot(this, w - 20, 20,     20,     h - 40, V_HOTSPOT_SCROLL_RIGHT,     "scroll right");
	/* bottom left scroll hotspot */
	new hotspot(this, 0,      h - 20, 20,     20,     V_HOTSPOT_SCROLL_DOWNLEFT,  "scroll diagonally");
	/* bottom scroll hotspot */
	new hotspot(this, 20,     h - 20, w - 40, 20,     V_HOTSPOT_SCROLL_DOWN,      "scroll down");
	/* upper left scroll hotspot */
	new hotspot(this, w - 20, h - 20, 20,     20,     V_HOTSPOT_SCROLL_DOWNRIGHT, "scroll diagonally");

	/* create the minimap now, so that it's drawn _under_ the message window */
	new minimap(this, map_data);

	/*NOTE: the upper scroll hotspot gets created later in ::init(),
	* so that it covers the message window */

	clip_tl_x = 999999;
	clip_tl_y = 999999;
	clip_br_x = 0;
	clip_br_y = 0;

	levwin = this;
	
	map_data->add_viewer(this);
}


void levelwin::init()
{
	int i, j;
	
	/* Initialize map */
	for (i = 0; i < ROWNO; i++) {
		for (j = 0; j < COLNO; j++) {
			map_deco[i][j] = 0;
			room_indices[i][j] = 0;
			
			clear_walls(i,j);
		}
	}
	
	w = vultures_screen->w;
	h = vultures_screen->h;

	/* create the upper scroll hotspot. NOTE that all other scroll hotspots
	// 		* are created earlier in the ctor */
	new hotspot(this, 20, 0, w - 40, 20, V_HOTSPOT_SCROLL_UP, "scroll up");

	/* Toolbar1: inventory, map, cast spell, extended commands */
	const tb_buttondesc tb1_desc[5] = {
		{V_HOTSPOT_BUTTON_INVENTORY, "Inventory"},
		{V_HOTSPOT_BUTTON_MAP, "Map"},
		{V_HOTSPOT_BUTTON_SPELLBOOK, "Cast spell"},
		{V_HOTSPOT_BUTTON_EXTENDED, "Extended commands"},
		{V_HOTSPOT_BUTTON_DISCOVERIES, "Show discoveries"}
	};

	new toolbar(this, V_WIN_TOOLBAR1, vultures_opts.show_actiontb,
				w - 215, h - 84, V_FILENAME_TOOLBAR1, tb1_desc);


	/* Toolbar 2: look at, previous messages, options, help */
	const tb_buttondesc tb2_desc[5] = {
		{V_HOTSPOT_BUTTON_LOOK, "Look"},
		{V_HOTSPOT_BUTTON_MESSAGES, "Old messages"},
		{V_HOTSPOT_BUTTON_OPTIONS, "Options"},
		{V_HOTSPOT_BUTTON_IFOPTIONS, "Interface Options"},
		{V_HOTSPOT_BUTTON_HELP, "Help"}
	};

	new toolbar(this, V_WIN_TOOLBAR2, vultures_opts.show_helptb,
				w - 215, h - 45, V_FILENAME_TOOLBAR2, tb2_desc);
	
	/* select wall style */
	switch (vultures_opts.wall_style)
	{
		case V_WALL_DISPLAY_STYLE_FULL:
			walltiles = (struct walls*)walls_full; break;

		case V_WALL_DISPLAY_STYLE_HALF_HEIGHT:
			walltiles = (struct walls*)walls_half; break;

		default:
			walltiles = (struct walls*)walls_half; break;
	}

	view_x = COLNO/2;
	view_y = ROWNO/2;
}


levelwin::~levelwin()
{
	map_data->del_viewer(this);
}


void levelwin::set_view(int x, int y)
{
	view_x = x;
	view_y = y;
}


/* check whether the map needs recentering */
bool levelwin::need_recenter(int map_x, int map_y)
{
	int screen_x, screen_y;

	map_x -= view_x;
	map_y -= view_y;
	screen_x = vultures_screen->w/2 + V_MAP_XMOD*(map_x - map_y);
	screen_y = vultures_screen->h/2 + V_MAP_YMOD*(map_x + map_y);

	/* if the player is within the outer 1/4 of the screen, the map needs recentering  */
	if ((screen_x >= vultures_screen->w/4) && (screen_x < vultures_screen->w * 3 / 4) &&
		(screen_y >= vultures_screen->h/4) && (screen_y < vultures_screen->h * 3 / 4))
		return true;

	return false;
}


void levelwin::force_redraw(void)
{
    need_redraw = 1;

    clip_tl_x = 0;
    clip_tl_y = 0;
    clip_br_x = w;
    clip_br_y = h;
}



bool levelwin::draw()
{
	int i, j, dir_idx;
	vultures_tile * cur_tile;
	int x, y;
	int tile_id, shadelevel;
	int map_tr_x, map_tr_y, __i, __j, diff, sum;
	int map_centre_x = this->w / 2;
	int map_centre_y = this->h / 2;
	int map_back, map_darkness, map_obj, map_mon;
	
	unsigned long startticks = SDL_GetTicks();

	static int cur_dlevel = -1;
	static int prev_cx = -1;
	static int prev_cy = -1;

	if (cur_dlevel != u.uz.dlevel)
	{
		init_floor_decors(10);
		cur_dlevel = u.uz.dlevel;
	}


	if (prev_cx != view_x ||
		prev_cy != view_y ||
		map_data->map_swallow != V_TILE_NONE)
	{
		clip_tl_x = this->abs_x;
		clip_tl_y = this->abs_y;
		clip_br_x = this->abs_x + this->w;
		clip_br_y = this->abs_y + this->h;
	}
	else
	{
		clip_tl_x = min(clip_tl_x, 0);
		clip_tl_y = min(clip_tl_y, 0);
		clip_br_x = max(clip_br_x, this->abs_x + this->w - 1);
		clip_br_y = max(clip_br_y, this->abs_y + this->h - 1);
	}

	if (clip_tl_x >= this->w + this->abs_x ||
	    clip_tl_y >= this->h + this->abs_y)
		/* nothing changed onscreen */
		return 1;

	prev_cx = view_x;
	prev_cy = view_y;

	/* Only draw on map area */
	vultures_set_draw_region(clip_tl_x, clip_tl_y,
							clip_br_x, clip_br_y);

	/* If swallowed draw ONLY the engulf tile and the player! */
	if (u.uswallow && map_data->map_swallow != V_TILE_NONE) {
		/* Clear map area */
		SDL_FillRect(vultures_screen, NULL, CLR32_BLACK);

		x = map_centre_x + V_MAP_XMOD*(u.ux - u.uy + view_y - view_x);
		y = map_centre_y + V_MAP_YMOD*(u.ux + u.uy - view_y - view_x);

		/* engulf tile */
		vultures_put_tile(x, y, map_data->map_swallow);

		/* player */
		vultures_put_tile(x, y, map_data->get_glyph(MAP_MON, u.ux, u.uy));

		vultures_invalidate_region(clip_tl_x, clip_tl_y,
								clip_br_x - clip_tl_x,
								clip_br_y - clip_tl_y);
		return 1;
	}
	else
		map_data->map_swallow = V_TILE_NONE;


	/* prevent double redraws if the map view just moved under the mouse cursor */
	map_highlight = mouse_to_map(vultures_get_mouse_pos());

	/* coords of the top right corner */
	map_tr_x = (-V_MAP_YMOD * (map_centre_x + 50) + V_MAP_XMOD * (map_centre_y + 50) +
					V_MAP_XMOD * V_MAP_YMOD) / (2 * V_MAP_XMOD * V_MAP_YMOD);
	map_tr_y =  (V_MAP_YMOD * (map_centre_x + 50) + V_MAP_XMOD * (map_centre_y + 50) +
					V_MAP_XMOD * V_MAP_YMOD) / (2 * V_MAP_XMOD * V_MAP_YMOD);

	/* without the +-1 small corners within the viewport are not drawn */
	diff = map_tr_x - map_tr_y - 1;
	sum  = map_tr_x + map_tr_y + 1;

	for (__i = - map_tr_y; __i <= map_tr_y; __i++)
	{
		i = view_y + __i;

		for (__j = diff + __i; __j + __i <= sum; __j++)
		{
			j = view_x + __j;

			x = map_centre_x + V_MAP_XMOD*(__j - __i);
			y = map_centre_y + V_MAP_YMOD*(__j + __i);

			if (j < 1 || j >= COLNO || i < 0 || i >= ROWNO)
			{
				vultures_put_tile(x, y, V_MISC_OFF_MAP);
				continue;
			}
			
			map_back = map_data->get_glyph(MAP_BACK, j, i);
			map_darkness = map_data->get_glyph(MAP_DARKNESS, j, i);

			/* 
			Draw Vulture's tiles, in order:
			pass 1
				1. Floor
				2. Floor edges

			pass 2
				1. North & west walls
				2. Furniture
				3. Traps
				4. Objects
				5. Monsters
				6. Effects
				7. South & east walls
			*/

			/* 0. init walls and floor edges for this tile*/
			if (map_back == V_TILE_WALL_GENERIC ||
				map_back == V_MISC_UNMAPPED_AREA)
				get_wall_tiles(i, j);
			else
				/* certain events (amnesia or a secret door being discovered) 
				* require us to clear walls again. since we cannot check for those cases
				* we simply clear walls whenever we don't set any... */
				clear_walls(i, j);

			if (map_back == V_TILE_FLOOR_WATER ||
				map_back == V_TILE_FLOOR_ICE ||
				map_back == V_TILE_FLOOR_LAVA ||
				map_back == V_TILE_FLOOR_AIR)
				/* these tiles get edges */
				get_floor_edges(i, j);
			else
				/* everything else doesn't. However we can't just assume a tile that doesn't need egdes doesn't have any:
				* pools may be dried out by fireballs, and suddenly we have a pit with edges :(
				* Therefore we always clear them explicitly */
				clear_floor_edges(i ,j);


			/* 2. Floor */
			tile_id = map_back;
			shadelevel = 0;

			if ((tile_id >= V_TILE_FLOOR_COBBLESTONE) &&
				(tile_id <= V_TILE_FLOOR_DARK)) {
				tile_id = get_floor_tile(tile_id, i, j);
				shadelevel = map_darkness;
			}
			else if(tile_id == V_TILE_NONE || tile_id == V_TILE_WALL_GENERIC)
				tile_id = V_MISC_UNMAPPED_AREA;

			vultures_put_tile_shaded(x, y, tile_id, shadelevel);

			/* shortcut for unmapped case */
			if (tile_id == V_MISC_UNMAPPED_AREA)
				continue;

			if (vultures_opts.highlight_cursor_square && 
				(j == map_highlight.x && i == map_highlight.y))
				vultures_put_tile(x, y, V_MISC_FLOOR_HIGHLIGHT);

			/* 3. Floor edges */
			for (dir_idx = 0; dir_idx < 12; dir_idx++)
				vultures_put_tile(x, y, maptile_floor_edge[i][j].dir[dir_idx]);
		}
	}

	for (__i = - map_tr_y; __i <= map_tr_y; __i++)
	{
		i = view_y + __i;
		if (i < 0 || i >= ROWNO)
			continue;

		for (__j = diff + __i; __j + __i <= sum; __j++)
		{
			j = view_x + __j;
			if (j < 1 || j >= COLNO)
				continue;
			/* Find position of tile centre */
			x = map_centre_x + V_MAP_XMOD*(__j - __i);
			y = map_centre_y + V_MAP_YMOD*(__j + __i);

			map_back = map_data->get_glyph(MAP_BACK, j, i);
			map_obj = map_data->get_glyph(MAP_OBJ, j, i);
			map_mon = map_data->get_glyph(MAP_MON, j, i);

			/* 1. West and north walls */
			if (j > 1)
				vultures_put_tile_shaded(x, y, maptile_wall[i][j].west,
										map_data->get_glyph(MAP_DARKNESS, j-1, i));
			if (i > 1)
				vultures_put_tile_shaded(x, y, maptile_wall[i][j].north,
										map_data->get_glyph(MAP_DARKNESS, j, i-1));

			/* shortcut for unmapped case */
			if (map_back != V_MISC_UNMAPPED_AREA ||
				map_obj != V_TILE_NONE) {
				/* 2. Furniture*/
				vultures_put_tile(x, y, map_data->get_glyph(MAP_FURNITURE, j, i));


				/* 3. Traps */
				vultures_put_tile(x, y, map_data->get_glyph(MAP_TRAP, j, i));


				/* 4. Objects */
				vultures_put_tile(x, y, map_obj);


				/* 5. Monsters */
				if ((cur_tile = vultures_get_tile(map_mon)) != NULL) {
					vultures_put_tile(x, y, map_mon);
					if (iflags.hilite_pet && map_data->get_glyph(MAP_PET, j, i))
						vultures_put_img(x + cur_tile->xmod, y + cur_tile->ymod - 10,
									vultures_get_tile(V_MISC_HILITE_PET)->graphic);
				}

				/* 6. Effects */
				vultures_put_tile(x, y, map_data->get_glyph(MAP_SPECIAL, j, i));
			}

			/* 7. South & East walls */
			if (i < ROWNO - 1)
				vultures_put_tile_shaded(x, y, maptile_wall[i][j].south,
										map_data->get_glyph(MAP_DARKNESS, j, i+1));
			if (j < COLNO - 1)
				vultures_put_tile_shaded(x, y, maptile_wall[i][j].east,
										map_data->get_glyph(MAP_DARKNESS, j+1, i));
		}
	}

	/* draw object highlights if requested */
	if (vultures_map_highlight_objects)
	{
		for (__i = - map_tr_y; __i <= map_tr_y; __i++)
		{
			i = view_y + __i;
			if (i < 0 || i >= ROWNO)
				continue;

			for (__j = diff + __i; __j + __i <= sum; __j++)
			{
				j = view_x + __j;
				if (j < 1 || j >= COLNO)
					continue;

				x = map_centre_x + V_MAP_XMOD*(__j - __i);
				y = map_centre_y + V_MAP_YMOD*(__j + __i);

				vultures_put_tilehighlight(x, y, map_data->get_glyph(MAP_OBJ, j, i));
			}
		}
	}
	/* Restore drawing region */
	vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);

	vultures_invalidate_region(clip_tl_x, clip_tl_y,
							clip_br_x - clip_tl_x,
							clip_br_y - clip_tl_y);

	clip_tl_x = 999999;
	clip_tl_y = 999999;
	clip_br_x = 0;
	clip_br_y = 0;

	vultures_tilecache_age();

	vultures_map_draw_msecs = SDL_GetTicks() - startticks;
	vultures_map_draw_lastmove = moves;

	return true;
}


eventresult levelwin::handle_timer_event(window* target, void* result, int time)
{
  std::string ttext;
	point mappos, mouse;

	mouse = vultures_get_mouse_pos();
	mappos = mouse_to_map(mouse);

	/* hovering on a border: scroll */
	if (target != this && target->menu_id)
	{
		int increment = (time / 500) + 1;
		switch (target->menu_id)
		{
			case V_HOTSPOT_SCROLL_UPLEFT:
				view_x-=increment; break;
			case V_HOTSPOT_SCROLL_UP:
				view_x-=increment; view_y-=increment; break;
			case V_HOTSPOT_SCROLL_UPRIGHT:
				view_y-=increment; break;
			case V_HOTSPOT_SCROLL_LEFT:
				view_x-=increment; view_y+=increment; break;
			case V_HOTSPOT_SCROLL_RIGHT:
				view_x+=increment; view_y-=increment; break;
			case V_HOTSPOT_SCROLL_DOWNLEFT:
				view_y+=increment; break;
			case V_HOTSPOT_SCROLL_DOWN:
				view_x+=increment; view_y+=increment; break;
			case V_HOTSPOT_SCROLL_DOWNRIGHT:
				view_x+=increment; break;
		}

		view_x = (view_x < 0) ? 0 : view_x;
		view_y = (view_y < 0) ? 0 : view_y;
		view_x = (view_x > COLNO) ? COLNO : view_x;
		view_y = (view_y > ROWNO) ? ROWNO : view_y;

		need_redraw = 1;
		return V_EVENT_HANDLED_REDRAW;
	}

	/* hovering over the map: display a tooltip */
	if (time < HOVERTIMEOUT)
		return V_EVENT_HANDLED_NOREDRAW;

	if (target != this && !target->caption.empty())
		vultures_mouse_set_tooltip(target->caption);
	else {
		ttext = map_data->map_square_description(mappos, 1);
		if(!ttext.empty())
			vultures_mouse_set_tooltip(ttext);
	}
	
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult levelwin::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	point mouse, mappos;

	mouse = vultures_get_mouse_pos();
	mappos = mouse_to_map(mouse);

	if (target != this && target->menu_id)
	{
		/* show a map scroll cursor if the mouse is in the edge zone */
		switch (target->menu_id)
		{
			case V_HOTSPOT_SCROLL_UPLEFT:
				vultures_set_mcursor(V_CURSOR_SCROLLUPLEFT); break;
			case V_HOTSPOT_SCROLL_UP:
				vultures_set_mcursor(V_CURSOR_SCROLLUP); break;
			case V_HOTSPOT_SCROLL_UPRIGHT:
				vultures_set_mcursor(V_CURSOR_SCROLLUPRIGHT); break;
			case V_HOTSPOT_SCROLL_LEFT:
				vultures_set_mcursor(V_CURSOR_SCROLLLEFT); break;
			case V_HOTSPOT_SCROLL_RIGHT:
				vultures_set_mcursor(V_CURSOR_SCROLLRIGHT); break;
			case V_HOTSPOT_SCROLL_DOWNLEFT:
				vultures_set_mcursor(V_CURSOR_SCROLLDOWNLEFT); break;
			case V_HOTSPOT_SCROLL_DOWN:
				vultures_set_mcursor(V_CURSOR_SCROLLDOWN); break;
			case V_HOTSPOT_SCROLL_DOWNRIGHT:
				vultures_set_mcursor(V_CURSOR_SCROLLDOWNRIGHT); break;
			default:
				vultures_set_mcursor(get_map_cursor(mappos));
		}
	}
	else
		/* select a cursor for the current position */
		vultures_set_mcursor(get_map_cursor(mappos));

	/* if the highlight option is on, store the map position of the mouse
	* and refresh the current and previous positions */
	if (vultures_opts.highlight_cursor_square && 
		(map_highlight.x != mappos.x || map_highlight.y != mappos.y))
	{
		mouse = map_to_mouse(map_highlight);
		add_to_clipregion(mouse.x - V_MAP_XMOD, mouse.y - V_MAP_YMOD,
							mouse.x + V_MAP_XMOD, mouse.y + V_MAP_YMOD);
		mouse = map_to_mouse(mappos);
		add_to_clipregion(mouse.x - V_MAP_XMOD, mouse.y - V_MAP_YMOD,
							mouse.x + V_MAP_XMOD, mouse.y + V_MAP_YMOD);

		map_highlight = mappos;
		need_redraw = 1;
		return V_EVENT_HANDLED_REDRAW;
	}
	
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult levelwin::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	point mouse, mappos;

	mouse = vultures_get_mouse_pos();
	mappos = mouse_to_map(mouse);

	return map_data->handle_click(result, button, mappos);
}


eventresult levelwin::handle_keydown_event(window* target, void* result, int sym, int mod, int unicode)
{
	int translated_key, key;
	int macronum, i;

	switch (sym) {
		case SDLK_TAB:
			map::toggle();
			return V_EVENT_HANDLED_REDRAW;

		/* F1- F6 trigger macros */
		case SDLK_F1:
		case SDLK_F2:
		case SDLK_F3:
		case SDLK_F4:
		case SDLK_F5:
		case SDLK_F6:
			macronum = sym - SDLK_F1;
			if (!vultures_opts.macro[macronum][0])
				break;

			((vultures_event*)result)->num = vultures_opts.macro[macronum][0];
			for (i = strlen(vultures_opts.macro[macronum]) - 1; i > 0; i--)
				vultures_eventstack_add(vultures_opts.macro[macronum][i], -1, -1, V_RESPOND_ANY);
			return V_EVENT_HANDLED_FINAL;

		/* CTRL+SHIFT+o opens the interface options */
		case SDLK_o:
			if (!(mod & KMOD_SHIFT) || !(mod & KMOD_CTRL))
				break;
			vultures_iface_opts();
			return V_EVENT_HANDLED_REDRAW;

		/* CTRL+SHIFT+p shows the message log */
		case SDLK_p:
			if (!(mod & KMOD_SHIFT) || !(mod & KMOD_CTRL))
				break;
			msgwin->view_all();
			return V_EVENT_HANDLED_REDRAW;

		default:
			break;
	}

	/* all other keys are converted and passed to the core */
	key = vultures_make_nh_key(sym, mod, unicode);

	if (!key)
		return V_EVENT_HANDLED_NOREDRAW;

	if (mapwin && isdigit(key))
		translated_key = key;
	else
		translated_key = vultures_translate_key(key);

	if (translated_key) {
		((vultures_event*)result)->num = translated_key;
		return V_EVENT_HANDLED_FINAL;
	}
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult levelwin::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	if (this == target) {
		w = res_w;
		h = res_h;
	} else {
		switch (target->menu_id)
		{
			/* case V_HOTSPOT_SCROLL_UPLEFT: never needs updating on resize*/

			case V_HOTSPOT_SCROLL_UP:
				target->w = w - 40;
				break;

			case V_HOTSPOT_SCROLL_UPRIGHT:
				target->x = w - 20;
				break;

			case V_HOTSPOT_SCROLL_LEFT:
				target->h = h - 40;
				break;

			case V_HOTSPOT_SCROLL_RIGHT:
				target->h = h - 40;
				target->x = w - 20;
				break;

			case V_HOTSPOT_SCROLL_DOWNLEFT:
				target->y = h - 20;
				break;

			case V_HOTSPOT_SCROLL_DOWN:
				target->y = h - 20;
				target->w = w - 40;
				break;

			case V_HOTSPOT_SCROLL_DOWNRIGHT:
				target->x = w - 20;
				target->y = h - 20;
				break;
		}
	}
	return V_EVENT_HANDLED_NOREDRAW;
}


void levelwin::map_update(glyph_type type, int prev_glyph, int new_glyph, int x, int y)
{
	int pixel_x, pixel_y;
	int tl_x = 99999, tl_y = 99999, br_x = 0, br_y = 0;
	vultures_tile *oldtile, *newtile;

	pixel_x = (this->w / 2) + V_MAP_XMOD*(x - y + view_y - view_x);
	pixel_y = (this->h / 2) + V_MAP_YMOD*(x + y - view_y - view_x);

	if (pixel_x < -VULTURES_CLIPMARGIN ||
		pixel_y < -VULTURES_CLIPMARGIN ||
		pixel_x > vultures_screen->w + VULTURES_CLIPMARGIN ||
		pixel_y > vultures_screen->h + VULTURES_CLIPMARGIN)
		return;
		
	oldtile = vultures_get_tile(prev_glyph);
	newtile = vultures_get_tile(new_glyph);

	if (type != MAP_BACK) {
		if (oldtile) {
			tl_x = oldtile->xmod;
			tl_y = oldtile->ymod;

			br_x = oldtile->xmod + oldtile->graphic->w;
			br_y = oldtile->ymod + oldtile->graphic->h;
		}

		if (newtile) {
			tl_x = min(newtile->xmod, tl_x);
			tl_y = min(newtile->ymod, tl_y);

			br_x = max(br_x, newtile->xmod + newtile->graphic->w);
			br_y = max(br_y, newtile->ymod + newtile->graphic->h);
		}

		tl_x += pixel_x;
		tl_y += pixel_y;
		br_x += pixel_x;
		br_y += pixel_y;

		if (type == MAP_MON)
			/* allow for the heart icon on pets */
			tl_y -= 10;
	}
	else {
		/* floor tiles tend to be placeholders until we reach draw_level,
			* so we do this manually */
		tl_x = pixel_x - 56;
		tl_y = pixel_y - 100; /* 100 pixels accounts for possible walls, too */
		br_x = pixel_x + 56;
		br_y = pixel_y + 22;
	}

	add_to_clipregion(tl_x, tl_y, br_x, br_y);
}


void levelwin::map_clear()
{
	int i, j;

	for (i = 0; i < ROWNO; i++)
		for (j = 0; j < COLNO; j++) {
			clear_floor_edges(i, j);
			clear_walls(i, j);
		}
}

point levelwin::mouse_to_map(point mouse)
{
	point mappos, px_offset;

	px_offset.x = mouse.x - (w / 2) + (view_x - view_y) * V_MAP_XMOD;
	px_offset.y = mouse.y - (h / 2) + (view_x + view_y)*V_MAP_YMOD;

	mappos.x = ( V_MAP_YMOD * px_offset.x + V_MAP_XMOD * px_offset.y +
			V_MAP_XMOD*V_MAP_YMOD)/(2*V_MAP_XMOD*V_MAP_YMOD);
	mappos.y = (-V_MAP_YMOD * px_offset.x + V_MAP_XMOD * px_offset.y +
			V_MAP_XMOD*V_MAP_YMOD)/(2*V_MAP_XMOD*V_MAP_YMOD);

	return mappos;
}


point levelwin::map_to_mouse(point mappos)
{
	int map_centre_x = w / 2;
	int map_centre_y = h / 2;
	point mouse;

	mouse.x = map_centre_x + V_MAP_XMOD*(mappos.x - mappos.y + view_y - view_x);
	mouse.y = map_centre_y + V_MAP_YMOD*(mappos.x + mappos.y - view_y - view_x);

	/* FIXME Why does this happen sometimes ? */
	if ( mouse.x < 0 ) mouse.x = 0;
	if ( mouse.y < 0 ) mouse.y = 0;

	return mouse;
}


void levelwin::toggle_uiwin(int menuid, bool enabled)
{
	window *win = this->first_child;
	while (win)
	{
		if (win->menu_id == menuid)
			win->visible = enabled;
		win = win->sib_next;
	}
	
	force_redraw();
}


void levelwin::add_to_clipregion(int tl_x, int tl_y, int br_x, int br_y)
{
	clip_tl_x = min(clip_tl_x, tl_x);
	clip_tl_y = min(clip_tl_y, tl_y);
	clip_br_x = max(clip_br_x, br_x);
	clip_br_y = max(clip_br_y, br_y);
}


void levelwin::set_wall_style(int style)
{
	switch (style)
	{
		case V_WALL_DISPLAY_STYLE_FULL:
			walltiles = (struct walls*)walls_full; break;
		case V_WALL_DISPLAY_STYLE_HALF_HEIGHT:
			walltiles = (struct walls*)walls_half; break;
	}
}


/* get room index is only used to semi-randomly select room decorations
* therefore the number we return can be as bogus as we want, so long as
* it's consistently the same. Using the current depth will provide a bit of
* variety on the maze levels...*/
int levelwin::get_room_index(int x, int y)
{
	int rindex;
	if (nroom == 0) /* maze levels */
		return u.uz.dlevel;

	if (In_mines(&u.uz)) /* The mines are a patchwork dungeon otherwise :( */
		return (u.uz.dlevel + nroom); /* cleverly prevent a repetitive sequence :P */

	rindex = room_indices[y][x];
	if (!rindex)
		return (u.uz.dlevel + nroom);

	return rindex;
}



/*
* Convert wall tile index (ie. wall type) to an associated decoration style.
*/
int levelwin::get_wall_decor(
	int floortype,
	int wally, int wallx,
	int floory, int floorx
)
{
	int roomid;

#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz))
		return V_WALL_LIGHT;
#endif

	switch (floortype)
	{
		case V_TILE_FLOOR_ROUGH:
		case V_TILE_FLOOR_ROUGH_LIT:
			return V_WALL_ROUGH; 
		case V_MISC_FLOOR_NOT_VISIBLE:
		case V_TILE_FLOOR_COBBLESTONE:
		{
			roomid = get_room_index(floorx, floory);
			switch(roomid % 4)
			{
				case 0: return V_WALL_STUCCO;
				case 1: return V_WALL_BRICK + ((wally*wallx+wally+wallx)%5);
				case 2: return V_WALL_VINE_COVERED;
				case 3: return V_WALL_MARBLE;
			}
		}
		default:
			return V_WALL_BRICK; 
	}
}


/*
* Convert floor tile index (ie. floor type) to an associated decoration style.
*/
int levelwin::get_floor_decor(int floorstyle, int floory, int floorx)
{
	int roomid;
#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz))
		return V_FLOOR_DARK;
#endif

	switch (floorstyle)
	{
		case V_TILE_FLOOR_ROUGH:     return V_FLOOR_ROUGH; 
		case V_TILE_FLOOR_ROUGH_LIT: return V_FLOOR_ROUGH_LIT;
		case V_TILE_FLOOR_COBBLESTONE:
		{
			roomid = get_room_index(floorx, floory);
			switch(roomid % 4)
			{
				case 0: return V_FLOOR_CERAMIC;
				case 1: return V_FLOOR_COBBLESTONE;
				case 2: return V_FLOOR_MOSS_COVERED;
				case 3: return V_FLOOR_MARBLE;
			}
		}
		case V_TILE_FLOOR_WATER:     return V_FLOOR_WATER;
		case V_TILE_FLOOR_ICE:       return V_FLOOR_ICE;
		case V_TILE_FLOOR_AIR:       return V_FLOOR_AIR;
		case V_TILE_FLOOR_LAVA:      return V_FLOOR_LAVA;
		case V_TILE_FLOOR_DARK:      return V_FLOOR_DARK;
		default:                     return V_FLOOR_COBBLESTONE; 
	}
}



void levelwin::init_floor_decors(int num_decors)
{
	int i, j, k;
	int lx, ly;

	if (!nroom)
		return; /* the level doesn't have distinct rooms, so do nothing */

	/* after putting a decor in room roomno, we calculate (roomno + 5) modulo nroom.
	* (nroom is the global containing the number of rooms on the level) */
	int roomno = 0;
	/* when roomno wraps we also add wrapadd, to ensure that a different bunch of rooms gets the next few decorations
	* this will distibute decorations seemingly at random but repeatably throughout the level
	* (rooms are sorted from left to right, so a step of 1 would leave most decorations on the left side)*/
	int wrapadd = (nroom % 5) ? 0 : 1;

	/* if placing a decor fails, try at most retries other rooms */
	int retries;

	/* did we manage to place the deco?  */
	int placed;

	int old_deco, old_deco2;
	int deco_height;
	int deco_width;
	int xoffset, yoffset;

	int current_deco = V_FLOOR_CARPET;

	/* reset the room indices and map decor arrays */
	for (i = 0; i < ROWNO; i++)
	{
		memset(room_indices[i], 0, COLNO);
		memset(map_deco[i], 0, COLNO);
	}

	/* init room indices */
	for (i = 0; i < nroom; i++)
		for (j = rooms[i].ly; j <= rooms[i].hy; j++)
			memset(&room_indices[j][rooms[i].lx], (i+1), (rooms[i].hx-rooms[i].lx+1));

	/* do no more if we're on the rogue level or in the mines, because decors there look dumb */
#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz))
		return;
#endif
	s_level * lev;
	if (In_mines(&u.uz) && ((lev = Is_special(&u.uz)) == 0 || !lev->flags.town))
		return;

	for (i = 0; i < num_decors; i++)
	{
		retries = nroom;
		placed = 0;
		switch (roomno % 3)
		{
			case 0: current_deco = V_FLOOR_CARPET; break;
			case 1: current_deco = V_FLOOR_MURAL; break;
			case 2: current_deco = V_FLOOR_MURAL2; break;
		}

		deco_width = floorstyles[current_deco].x;
		deco_height = floorstyles[current_deco].y;

		while (retries-- && !placed)
		{
			lx = rooms[roomno].lx;
			ly = rooms[roomno].ly;
			while (ly <= rooms[roomno].hy && (old_deco = (map_deco[ly][lx] >> 4)) != 0)
			{
				while (lx <= rooms[roomno].hx && (old_deco2 = (map_deco[ly][lx] >> 4)) != 0)
					lx += floorstyles[old_deco2-1].x;
				ly += floorstyles[old_deco-1].y;
				lx = rooms[roomno].lx;
			}

			if ((rooms[roomno].hx - lx + 1) >= deco_width &&
				(rooms[roomno].hy - ly + 1) >= deco_height)
			{
				placed = 1;
				for (j=0; j<deco_height; j++)
					for (k=0; k<deco_width;k++)
						map_deco[ly+j][lx+k] = ((current_deco+1) << 4) + (j*deco_width+k);
			}
		}

		if (!retries && !placed)
			/* placing this one failed, so trying to place others is futile  */
			break;

		roomno += 5;
		if (roomno > nroom)
			roomno = (roomno % nroom) + wrapadd;
	}

	/* centre the decorations in the rooms, as the previous code always plces them in the top corner*/
	for (i = 0; i < nroom; i++)
	{
		lx = rooms[i].lx;
		ly = rooms[i].ly;

		while (map_deco[ly][lx] != 0 && lx <= rooms[i].hx)
			lx++;

		deco_width = lx - rooms[i].lx;
		lx = rooms[i].lx;

		while (map_deco[ly][lx] != 0 && ly <= rooms[i].hy)
			ly++;

		deco_height = ly - rooms[i].ly;
		xoffset = (int)((rooms[i].lx + rooms[i].hx + 1)/2.0 - deco_width/2.0);
		yoffset = (int)((rooms[i].ly + rooms[i].hy + 1)/2.0 - deco_height/2.0);

		for (j = deco_height-1; j >= 0; j--)
			for (k = deco_width-1; k >= 0; k--)
				map_deco[yoffset + j][xoffset + k] = map_deco[rooms[i].ly + j][rooms[i].lx + k];

		for (j = rooms[i].ly; j < yoffset; j++)
			memset(&map_deco[j][rooms[i].lx], 0, (rooms[i].hx-rooms[i].lx+1));

		for (j = rooms[i].ly; j <= rooms[i].hy; j++)
			memset(&map_deco[j][rooms[i].lx], 0, (xoffset - rooms[i].lx));
	}
}


void levelwin::get_wall_tiles(int y, int x)
{
	int style;
	int bg_west, bg_north, bg_east, bg_south;

	if (!level.locations[x][y].seenv)
		return;

	/* x - 1: west wall  */
	bg_west = map_data->get_glyph(MAP_BACK, x - 1, y);
	if (x > 0 && bg_west != V_TILE_WALL_GENERIC && bg_west != V_MISC_UNMAPPED_AREA) {
		style = get_wall_decor(bg_west, y, x, y, x-1);
		maptile_wall[y][x].west = walltiles[style].west;
	}
	else
		maptile_wall[y][x].west = V_TILE_NONE;

	/* y - 1: north wall  */
	bg_north = map_data->get_glyph(MAP_BACK, x, y - 1);
	if (y > 0 && bg_north != V_TILE_WALL_GENERIC && bg_north != V_MISC_UNMAPPED_AREA) {
		style = get_wall_decor(bg_north, y, x, y - 1, x);
		maptile_wall[y][x].north = walltiles[style].north;
	}
	else
		maptile_wall[y][x].north = V_TILE_NONE;

	/* x + 1: east wall  */
	bg_east = map_data->get_glyph(MAP_BACK, x + 1, y);
	if (x < COLNO - 1 && bg_east != V_TILE_WALL_GENERIC && bg_east != V_MISC_UNMAPPED_AREA) {
		style = get_wall_decor(bg_east, y, x, y, x + 1);
		maptile_wall[y][x].east = walltiles[style].east;
	}
	else
		maptile_wall[y][x].east = V_TILE_NONE;

	/* y + 1: south wall  */
	bg_south = map_data->get_glyph(MAP_BACK, x, y + 1);
	if (y < ROWNO - 1 && bg_south != V_TILE_WALL_GENERIC && bg_south != V_MISC_UNMAPPED_AREA) {
		style = get_wall_decor(bg_south, y, x, y + 1, x);
		maptile_wall[y][x].south = walltiles[style].south;
	}
	else
		maptile_wall[y][x].south = V_TILE_NONE;
}



int levelwin::get_floor_tile(int tile, int y, int x)
{
	int style;
	int deco_pos;
	unsigned char deco = map_deco[y][x];

	if (deco && tile == V_TILE_FLOOR_COBBLESTONE) {
		style = (deco >> 4) - 1;
		deco_pos = (int)(deco & 0x0F);
		return floorstyles[style].array[deco_pos];
	}

	style = get_floor_decor(tile, y, x);
	deco_pos = floorstyles[style].x * (y % floorstyles[style].y) + (x % floorstyles[style].x);
	return floorstyles[style].array[deco_pos];
}



void levelwin::get_floor_edges(int y, int x)
{
	int i, x2, y2;
	int tile = map_data->get_glyph(MAP_BACK, x, y);
	int style = V_FLOOR_EDGE_COBBLESTONE;

	point s_delta[4] = {{-1,0}, {0,-1}, {1,0}, {0,1}};
	point d_delta[4] = {{-1,1}, {-1,-1}, {1,-1}, {1,1}};

	/* Default: no floor edges around tile */
	clear_floor_edges(y, x);

	/* straight sections */
	for (i = 0; i < 4; i++) {
		x2 = x+s_delta[i].x;
		y2 = y+s_delta[i].y;
		if (x > 0 && x < COLNO-s_delta[i].x && y > 0 && y < ROWNO-s_delta[i].y && 
			tile != map_data->get_glyph(MAP_BACK, x2, y2) &&
			tile + map_data->get_glyph(MAP_BACK, x2, y2) !=
			V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE) /* this prevents borders between water and ice*/
				maptile_floor_edge[y][x].dir[i] = flooredges[style].dir[i];
	}


	/* "inward pointing" corners */
	for (i = 4; i < 8; i++) {
		if ((maptile_floor_edge[y][x].dir[(i+3)%4] != V_TILE_NONE) &&
			(maptile_floor_edge[y][x].dir[i%4]) != V_TILE_NONE)

			maptile_floor_edge[y][x].dir[i] = flooredges[style].dir[i];
	}


	/* "outward pointing" corners */
	for (i = 8; i < 12; i++) {
		x2 = x+d_delta[i%4].x;
		y2 = y+d_delta[i%4].y;
		if ((maptile_floor_edge[y][x].dir[(i+3)%4] == V_TILE_NONE) &&
			(maptile_floor_edge[y][x].dir[i%4] == V_TILE_NONE) &&
			x > 0 && x < COLNO - 1 && y > 0 && y < ROWNO - 1 && 
			tile != map_data->get_glyph(MAP_BACK, x2, y2) &&
			tile + map_data->get_glyph(MAP_BACK, x2, y2) != 
			V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE)
			maptile_floor_edge[y][x].dir[i] = flooredges[style].dir[i];
	}

/*    for (i = 0; i < 12; i++)
		if (!maptile_floor_edge[y][x].dir[i])
			maptile_floor_edge[y][x].dir[i] = V_TILE_NONE;*/
}

void levelwin::clear_walls(int y, int x)
{
	maptile_wall[y][x].west = V_TILE_NONE;
	maptile_wall[y][x].north = V_TILE_NONE;
	maptile_wall[y][x].east = V_TILE_NONE;
	maptile_wall[y][x].south = V_TILE_NONE;
}



void levelwin::clear_floor_edges(int y, int x)
{
	int i;
	for (i = 0; i < 12; i++)
		maptile_floor_edge[y][x].dir[i] = V_TILE_NONE;
}


/* select an appropriate cursor for the given location */
int levelwin::get_map_cursor(point mappos)
{
	if ((mappos.x < 1) || (mappos.x >= COLNO) ||
		(mappos.y < 0) || (mappos.y >= ROWNO))
		return V_CURSOR_TARGET_INVALID;

	/* whatis: look or teleport */
	if (vultures_whatis_active)
		return V_CURSOR_TARGET_HELP;

	/* monsters and objects get a red circle */
	if (map_data->get_glyph(MAP_MON, mappos.x, mappos.y) != V_TILE_NONE &&
		((mappos.x != u.ux) || (mappos.y != u.uy)))
		return V_CURSOR_TARGET_RED;

	if (map_data->get_glyph(MAP_OBJ, mappos.x, mappos.y) != V_TILE_NONE)  
		return V_CURSOR_TARGET_RED;

	/* other valid visible locations  */
	if (map_data->get_glyph(MAP_BACK, mappos.x, mappos.y) != V_TILE_NONE) {
		int furniture = map_data->get_glyph(MAP_FURNITURE, mappos.x, mappos.y);
		/* Closed doors get an 'open door' cursor */
		if ((furniture == V_MISC_VDOOR_WOOD_CLOSED) ||
			(furniture == V_MISC_HDOOR_WOOD_CLOSED))
			return V_CURSOR_OPENDOOR;

		/* Stairs and ladders get a 'stairs' cursor */
		if ((furniture == V_MISC_STAIRS_UP) || (furniture == V_MISC_STAIRS_DOWN) ||
			(furniture == V_MISC_LADDER_UP) || (furniture == V_MISC_LADDER_DOWN))
			return V_CURSOR_STAIRS;

		/* Fountains get a 'goblet' cursor */
		if (furniture == V_MISC_FOUNTAIN)
			return V_CURSOR_GOBLET;

		if (map_data->get_glyph(MAP_BACK, mappos.x, mappos.y) != V_TILE_WALL_GENERIC)
			return V_CURSOR_TARGET_GREEN;
	}

	return V_CURSOR_TARGET_INVALID;
}

