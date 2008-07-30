#ifndef _statuswin_h_
#define _statuswin_h_

#include "window.h"


class statuswin : public window
{
public:
	statuswin(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
