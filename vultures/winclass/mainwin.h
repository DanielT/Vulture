/* NetHack may be freely redistributed.  See license for details. */

#ifndef _mainwin_h_
#define _mainwin_h_

#include "window.h"


class mainwin : public window
{
public:
	mainwin(window *p);
	virtual bool draw();
	virtual void layout();

protected:
	int border_left, border_right, border_top, border_bottom;
	int get_frameheight();
};


#endif
