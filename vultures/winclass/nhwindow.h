/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _nhwindow_h_
#define _nhwindow_h_

#include "menuitem.h"
#include <string>
#include <list>
using std::string;
using std::list;

class window;

class nhwindow {
public:
	typedef list<menuitem>::iterator item_iterator;

	nhwindow(int type);
	~nhwindow();
	void add_menuitem(string str, bool preselected, void *identifier, char accelerator, int glyph);
	void reset();
	
	int id;
	int type;
	int ending_type;
	string caption;
	window *impl;
	list<menuitem> items;
	bool has_objects;
};

nhwindow * vultures_get_nhwindow(int winid);

#endif
