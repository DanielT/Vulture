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
	endingwin(int how);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	virtual menuwin* replace_win(menuwin *win);

private:
	int ending_type;
};


#endif
