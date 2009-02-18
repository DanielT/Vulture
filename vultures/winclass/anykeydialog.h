/* NetHack may be freely redistributed.  See license for details. */

#ifndef _anykeydialog_h_
#define _anykeydialog_h_

#include "mainwin.h"

class textwin;

class anykeydialog : public mainwin
{
public:
	anykeydialog(window *p, string ques);
	virtual bool draw();
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_keydown_event(window* target, void* result, int sym, int mod, int unicode);
	virtual eventresult handle_resize_event(window* target, void* result, int w, int h);

private:
	int count;
	textwin *txt;
};


#endif
