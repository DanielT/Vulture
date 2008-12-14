#ifndef _statuswin_h_
#define _statuswin_h_

#include "window.h"
#include "textwin.h"

#define V_FILENAME_STATUS_BAR           "statusbar"


class statuswin : public window
{
public:
	statuswin(window *p);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	void parse_statusline(const char *str);

private:
	void add_cond(const char *str, int warnno, int color);
	SDL_Surface *statusbg;
	textwin *tokenarray[5][5];
// 	const int status_xpos[5];
};

extern statuswin *stwin;

#endif
