/* NetHack may be freely redistributed.  See license for details. */

#ifndef _toolbar_h_
#define _toolbar_h_

#include "window.h"


typedef struct {
	int menu_id;
	const char *name;
} tb_buttondesc;


class toolbar : public window
{
public:
	toolbar(window *p, int menuid, bool visible, int x, int y,
	        string imgfile, const tb_buttondesc buttons[5]);
	virtual ~toolbar();
	virtual bool draw();
	virtual eventresult handle_timer_event(window* target, void* result, int time);
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_resize_event(window* target, void* result, int res_w, int res_h);

private:
	SDL_Surface *bgimage;
};


#endif
