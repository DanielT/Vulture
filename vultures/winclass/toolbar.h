/* Copyright (c) Daniel Thaler, 2008                              */
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
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);

private:
	SDL_Surface *bgimage;
};


#endif
