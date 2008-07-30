#ifndef _toolbar_h_
#define _toolbar_h_

#include "window.h"


class toolbar : public window
{
public:
	toolbar(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
