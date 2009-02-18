/* NetHack may be freely redistributed.  See license for details. */

#ifndef _contextmenu_h_
#define _contextmenu_h_

#include <vector>
#include <utility>

#include "window.h"


class contextmenu : public window
{
public:
	typedef std::vector<std::pair<std::string, int> >  itemlist;

	contextmenu(window *p);
	virtual bool draw();
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int mouse_x, int mouse_y, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_keydown_event(window* target, void* result, SDL_keysym keysym);
	virtual void layout(void);
	void add_item(const char *label, int menuid);

private:
	itemlist items;
	bool layout_done;
};


#endif
