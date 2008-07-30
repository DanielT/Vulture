#ifndef _hotspot_h_
#define _hotspot_h_

#include "window.h"


class hotspot : public window
{
public:
	hotspot(window *p, int nh_wt, window_type wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
