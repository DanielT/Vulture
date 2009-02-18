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
	scrollwin(window *p, bool txt);
	virtual bool draw(void);
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);

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
	bool is_text;
};


#endif
