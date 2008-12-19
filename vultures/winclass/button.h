#ifndef _button_h_
#define _button_h_

#include "window.h"


class button : public window
{
public:
	button(window *p, const char *caption, int menuid, char accel);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);

	SDL_Surface *image;
private:
	bool selected;
};


#endif
