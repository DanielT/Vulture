#ifndef _messagebox_h_
#define _messagebox_h_

#include "mainwin.h"


class messagebox : public mainwin
{
public:
	messagebox(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
