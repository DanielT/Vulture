#ifndef _minimap_h_
#define _minimap_h_

#include "window.h"


class minimap : public window
{
public:
	minimap(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
