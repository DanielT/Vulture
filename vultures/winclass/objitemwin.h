/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _objitemwin_h_
#define _objitemwin_h_

#include "optionwin.h"

struct obj;

class objitemwin : public optionwin
{
public:
	objitemwin(window *p, menuitem* mi, string cap,
	          char accel, int glyph, bool selected, bool multiselect);
	virtual bool draw();
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_other_event(window* target, void* result, SDL_Event* event);

	struct obj *obj;
	bool last_toggled;
	
private:
	bool hover;
};


#endif
