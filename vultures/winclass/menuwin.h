#ifndef _menuwin_h_
#define _menuwin_h_

#include <list>

#include "mainwin.h"

#define MAX_MENU_HEIGHT 800

class optionwin;
class scrollwin;

class menuitem
{
public:
	menuitem(string str, bool sel, void *ident, char accel, int glyph) : 
	         identifier(ident), str(str), glyph(glyph), preselected(sel),
             accelerator(accel), selected(false), count(-1) {};
	const void *identifier;
	string str;
	int glyph;
	bool preselected;
	char accelerator;
	bool selected;
	int count;
};


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
	menuwin(window *p);
	virtual ~menuwin();
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	virtual menuwin* replace_win(menuwin* win);
	virtual void layout();
	void reset();
	void assign_accelerators();
	void set_selection_type(int how);
	window *find_accel(char accel);
	selection_iterator selection_begin() { return selection_iterator(items.begin(), items.end()); }
	selection_iterator selection_end() { return selection_iterator(items.end(), items.end()); }
	
	virtual void add_menuitem(string str, bool preselected, void *identifier, char accelerator, int glyph);
	
protected:
	void select_option(optionwin *target, int count);
	
	scrollwin *scrollarea;
	int select_how;
	std::list<menuitem> items;
	int count;
};


#endif
