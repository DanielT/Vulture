/* NetHack may be freely redistributed.  See license for details. */

#ifndef _choicedialog_h_
#define _choicedialog_h_

#include "menuwin.h"

class button;

class choicedialog : public mainwin
{
public:
	choicedialog(window *p, string question, const char *choices, char defchoice);
	virtual bool draw();
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_keydown_event(window* target, void* result, int sym, int mod, int unicode);
	virtual eventresult handle_resize_event(window* target, void* result, int res_w, int res_h);

private:
	char defchoice;
	button *defbutton;
};


#endif
