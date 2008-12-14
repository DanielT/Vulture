#ifndef _map_h_
#define _map_h_

#include "window.h"

class levelwin;

#define VULTURES_MAP_SYMBOL_WIDTH   7
#define VULTURES_MAP_SYMBOL_HEIGHT 14


#ifdef VULTURESEYE
# define V_MAX_MAP_SYMBOLS  (40*30)
#endif
#ifdef VULTURESCLAW
# define V_MAX_MAP_SYMBOLS  (40*36)
#endif


class map : public window
{
public:
	map(levelwin *p);
	~map();
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	static void toggle(void);

private:
	SDL_Surface *mapbg;
	SDL_Surface *vultures_map_symbols[V_MAX_MAP_SYMBOLS];
};


#endif
