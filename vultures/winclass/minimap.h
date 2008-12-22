/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _minimap_h_
#define _minimap_h_

#include "SDL.h"
#include "window.h"

#define V_FILENAME_MINIMAPBG            "minimapbg"

/*
 * colors used to draw the mini-map
 */
#define V_COLOR_MINI_CORRIDOR CLR32_PURPLE44
#define V_COLOR_MINI_STAIRS   CLR32_LIGHTPINK
#define V_COLOR_MINI_DOOR     CLR32_BROWN
#define V_COLOR_MINI_FLOOR    CLR32_PURPLE44
#define V_COLOR_MINI_YOU      CLR32_WHITE

/* minimap tile types */
enum {
	V_MMTILE_NONE,
	V_MMTILE_FLOOR,
	V_MMTILE_STAIRS,
	V_MMTILE_DOOR,
	V_MMTILE_YOU,
	V_MMTILE_PET,
	V_MMTILE_MONSTER,
	V_MMTILE_MAX
};


class levelwin;
class mapdata;

class minimap : public window
{
public:
	minimap(levelwin *p, mapdata *data);
	~minimap();
	virtual bool draw();
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_resize_event(window* target, void* result, int res_w, int res_h);
	
private:
	SDL_Surface *minimapbg;
	char vultures_minimap_syms[ROWNO][COLNO];
	levelwin *level;
	mapdata *map_data;
};

extern minimap *minimapwin;

#endif
