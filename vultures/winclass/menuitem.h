/* NetHack may be freely redistributed.  See license for details. */

#ifndef _menuitem_h_
#define _menuitem_h_

#include <string>
using std::string;

class menuitem {
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

#endif