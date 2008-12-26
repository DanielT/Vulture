/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _menuwin_h_
#define _menuwin_h_

#include <list>
#include "menuitem.h"
#include "mainwin.h"

#define MAX_MENU_HEIGHT 800

class optionwin;
class scrollwin;


class menuwin : public mainwin
{
public:
	typedef std::list<menuitem>::iterator item_iterator;

	class selection_iterator
	{
		friend class menuwin;
	public:
		selection_iterator& operator++(void);
		bool operator!=(selection_iterator rhs) const;
		menuitem& operator*();
		
	private:
		selection_iterator(item_iterator start, item_iterator end);
		
		item_iterator iter;
		item_iterator end;
	};

	menuwin();
	menuwin(window *p, std::list<menuitem> &menuitems, int how);
	virtual ~menuwin();
	virtual bool draw();
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_keydown_event(window* target, void* result, int sym, int mod, int unicode);
	virtual eventresult handle_resize_event(window* target, void* result, int w, int h);

	
	virtual void layout();
	window *find_accel(char accel);
	selection_iterator selection_begin() { return selection_iterator(items.begin(), items.end()); }
	selection_iterator selection_end() { return selection_iterator(items.end(), items.end()); }
	
protected:
	void assign_accelerators();
	void select_option(optionwin *target, int count);
	
	scrollwin *scrollarea;
	std::list<menuitem> items;
	int select_how;
	int count;
};

#endif
