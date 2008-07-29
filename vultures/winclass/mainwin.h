#ifndef _mainwin_h_
#define _mainwin_h_

#include "window.h"


class mainwin : public window
{
public:
	mainwin(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
