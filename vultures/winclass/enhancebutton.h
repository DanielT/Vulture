#ifndef _enhancebutton_h_
#define _enhancebutton_h_

#include "window.h"


class enhancebutton : public window
{
public:
	enhancebutton(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
