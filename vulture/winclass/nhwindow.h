/* NetHack may be freely redistributed.  See license for details. */

#ifndef _nhwindow_h_
#define _nhwindow_h_

#include "menuitem.h"
#include <string>
#include <list>

class window;

class nhwindow {
public:
	typedef std::list<menuitem>::iterator item_iterator;

	nhwindow(int type);
	~nhwindow();
	void add_menuitem(std::string str, bool preselected, void *identifier, char accelerator, int glyph);
	void reset();
	
	int id;
	int type;
	int ending_type;
  std::string caption;
	window *impl;
  std::list<menuitem> items;
	bool has_objects;
};

nhwindow * vultures_get_nhwindow(int winid);

#endif
