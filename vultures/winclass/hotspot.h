#ifndef _hotspot_h_
#define _hotspot_h_

#include "window.h"


class hotspot : public window
{
public:
	hotspot(window *parent, int x, int y, int w, int h, int menu_id, const char *name);
	virtual bool draw();
	virtual eventresult event_handler(window *target, void *result, SDL_Event *event);
};


#endif
