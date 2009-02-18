/* NetHack may be freely redistributed.  See license for details. */

#ifndef _inputdialog_h_
#define _inputdialog_h_

#include "mainwin.h"

class textwin;

class inputdialog : public mainwin
{
public:
	inputdialog(window *p, string caption, int size,
	            int force_x, int force_y);
	virtual bool draw();
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_keydown_event(window* target, void* result, int sym, int mod, int unicode);
	virtual eventresult handle_resize_event(window* target, void* result, int res_w, int res_h);
	void copy_input(char *dest);

private:
	textwin *input;
	int destsize;
};


#endif
