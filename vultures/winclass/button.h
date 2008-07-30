#ifndef _button_h_
#define _button_h_

#include "window.h"


class button : public window
{
public:
	button(window *p, int nh_wt, window_type wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
