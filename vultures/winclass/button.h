/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _button_h_
#define _button_h_

#include "window.h"


class button : public window
{
public:
	button(window *p, string caption, int menuid, char accel);
	virtual ~button();
	virtual bool draw();
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_other_event(window* target, void* result, SDL_Event* event);

	SDL_Surface *image;
private:
	bool selected;
};


#endif
