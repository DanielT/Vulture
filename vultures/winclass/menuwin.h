#ifndef _menuwin_h_
#define _menuwin_h_

#include <vector>

#include "mainwin.h"

#define MAX_MENU_HEIGHT 800

class optionwin;
class scrollwin;

class menuitem
{
public:
	menuitem(const char *str, bool sel, void *ident, char accel, int glyph) : 
	         identifier(ident), str(str), glyph(glyph), preselected(sel),
             accelerator(accel), selected(false), count(-1) {};
	const void *identifier;
	const char *str;
	const int glyph;
	const bool preselected;
	char accelerator;
	bool selected;
	int count;
};


class menuwin : public mainwin
{
public:
	typedef std::vector<menuitem*>::iterator item_iterator;

	class selection_iterator
	{
		friend class menuwin;
	public:
		selection_iterator& operator++(void);
		bool operator!=(selection_iterator rhs) const;
		menuitem* operator*();
		
	private:
		selection_iterator(item_iterator start, item_iterator end);
		
		item_iterator iter;
		item_iterator end;
	};


	menuwin() {};
	menuwin(window *p);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	virtual window* replace_win(window* win);
	virtual void layout();
	void reset();
	void assign_accelerators();
	void set_selection_type(int how);
	window *find_accel(char accel);
	selection_iterator selection_begin() { return selection_iterator(items.begin(), items.end()); }
	selection_iterator selection_end() { return selection_iterator(items.end(), items.end()); }
	
	virtual void add_menuitem(const char *str, bool preselected, void *identifier, char accelerator, int glyph);
	
protected:
	void select_option(optionwin *target, int count);
	
	scrollwin *scrollarea;
	int select_how;
	std::vector<menuitem*> items;
	int count;
};


#endif
