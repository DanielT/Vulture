#ifndef _levelwin_h_
#define _levelwin_h_

#include "window.h"


class levelwin : public window
{
public:
	levelwin();
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
