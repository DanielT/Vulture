/* NetHack may be freely redistributed.  See license for details. */

#ifndef _optionwin_h_
#define _optionwin_h_

#include "window.h"


class menuitem;


class optionwin : public window
{
public:
	optionwin(window *p, menuitem* mi, string cap,
	          char accel, int glyph, bool selected, bool is_checkbox);
	virtual bool draw();
	
	int glyph;
	menuitem *item;
	bool is_checkbox; // otherwise it's an option
};


#endif
