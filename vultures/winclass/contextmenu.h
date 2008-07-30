#ifndef _contextmenu_h_
#define _contextmenu_h_

#include "window.h"


class contextmenu : public window
{
public:
	contextmenu(window *p, int nh_wt, window_type wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
