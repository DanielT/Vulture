#ifndef _objheaderwin_h_
#define _objheaderwin_h_

#include "window.h"


class objheaderwin : public window
{
public:
	objheaderwin(window *parent, string cap);
	virtual bool draw();
	virtual eventresult event_handler(window *target, void *result, SDL_Event *event);
};


#endif
