#ifndef _map_h_
#define _map_h_

#include "window.h"


class map : public window
{
public:
	map(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
