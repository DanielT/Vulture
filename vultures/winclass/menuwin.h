#ifndef _menuwin_h_
#define _menuwin_h_

#include "mainwin.h"


class menuwin : public mainwin
{
public:
	menuwin(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
