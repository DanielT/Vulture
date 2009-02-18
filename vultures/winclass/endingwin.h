/* NetHack may be freely redistributed.  See license for details. */

#ifndef _endingwin_h_
#define _endingwin_h_

#include "menuwin.h"


/*
 * Ending scenes. Eventually these could
 * be made configurable.
 */
#define V_FILENAME_ENDING_DIED          "cairn"
#define V_FILENAME_ENDING_ASCENDED      "night"
#define V_FILENAME_ENDING_QUIT          "quitgame"
#define V_FILENAME_ENDING_ESCAPED       "quitgame"


class endingwin : public menuwin
{
public:
	endingwin(window *p, std::list<menuitem> &menuitems, int how);
	virtual bool draw();
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_keydown_event(window* target, void* result, int sym, int mod, int unicode);

private:
	int ending_type;
};


#endif
