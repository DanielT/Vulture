/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _inventory_h_
#define _inventory_h_

#include "menuwin.h"

class objitemwin;

enum invactions {
	V_INVACTION_APPLY = 1,
	V_INVACTION_DRINK,
	V_INVACTION_EAT,
	V_INVACTION_READ,
	V_INVACTION_ZAP,
	V_INVACTION_WEAR,
	V_INVACTION_PUT_ON,
	V_INVACTION_WIELD,
	V_INVACTION_REMOVE,
	V_INVACTION_DROP,
	V_INVACTION_NAME
};


class inventory : public menuwin
{
public:
	inventory();
	inventory(window *p);
	virtual bool draw();
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_keydown_event(window* target, void* result, SDL_keysym keysym);
	virtual eventresult handle_resize_event(window* target, void* result, int res_w, int res_h);
	virtual menuwin* replace_win(menuwin* win);
	virtual void layout();
	
private:
	void update_invscroll(int newpos);
	eventresult context_menu(objitemwin *target);
	int ow_ncols, ow_vcols, ow_firstcol, ow_vrows;
	objitemwin *ow_lasttoggled;
};


#endif
