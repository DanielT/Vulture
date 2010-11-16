/* NetHack may be freely redistributed.  See license for details. */

#ifndef _menuitem_h_
#define _menuitem_h_

#include <string>

class menuitem {
public:
	menuitem(std::string str, bool sel, void *ident, char accel, char group_accel, int glyph) : 
	         identifier(ident), str(str), glyph(glyph), preselected(sel),
             accelerator(accel), group_accelerator(group_accel), selected(false), count(-1) {};
	const void *identifier;
  std::string str;
	int glyph;
	bool preselected;
	char accelerator;
	char group_accelerator;
	bool selected;
	int count;
};

#endif
