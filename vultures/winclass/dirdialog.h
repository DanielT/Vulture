/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _dirdialog_h_
#define _dirdialog_h_

#include "mainwin.h"

class hotspot;

class dirdialog : public mainwin
{
public:
	dirdialog(window *p, string ques);
	virtual bool draw();
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_keydown_event(window* target, void* result, int sym, int mod, int unicode);
	virtual eventresult handle_resize_event(window* target, void* result, int res_w, int res_h);
	
private:
	hotspot *arr;
	int arroffset_x, arroffset_y;
};


#endif
