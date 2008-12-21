/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _map_h_
#define _map_h_

#include "window.h"

#ifdef VULTURESEYE
# define V_MAX_MAP_SYMBOLS  (40*30)
#endif
#ifdef VULTURESCLAW
# define V_MAX_MAP_SYMBOLS  (40*36)
#endif

class levelwin;
class mapdata;

class map : public window
{
public:
	map(levelwin *p, mapdata *data);
	~map();
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	static void toggle(void);

private:
	SDL_Surface *mapbg;
	SDL_Surface *map_symbols[V_MAX_MAP_SYMBOLS];
	mapdata *map_data;
};

#endif
