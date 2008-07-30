#ifndef _messagewin_h_
#define _messagewin_h_

#include "window.h"


class messagewin : public window
{
public:
	messagewin(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
