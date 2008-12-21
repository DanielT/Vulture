/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _scrollwin_h_
#define _scrollwin_h_

#include "window.h"

class scrollbar;

enum scrolltypes {
	V_SCROLL_LINE_REL,
	V_SCROLL_PAGE_REL,
	V_SCROLL_PAGE_ABS,
	V_SCROLL_PIXEL_ABS
};


class scrollwin : public window
{
public:
	scrollwin() {};
	scrollwin(window *p);
	virtual bool draw(void);
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	int get_scrollheight();
	void set_height(int newheight);
	void layout(void);

	eventresult scrollto(int scrolltype, int scrolldir);

private:
	eventresult mousescroll(scrollbar *target, int is_drag);
	int get_menuitem_width(window *item, int colwidths[8]);
	bool layout_done;
	int scrollpos;
	scrollbar *scroll;
};


#endif
