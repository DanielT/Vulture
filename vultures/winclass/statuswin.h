/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _statuswin_h_
#define _statuswin_h_

#include "window.h"

#define V_FILENAME_STATUS_BAR           "statusbar"

/* Indices into warning colors */
enum vultures_warn_type {
	V_WARN_NONE = 0,
	V_WARN_NORMAL,
	V_WARN_MORE,
	V_WARN_ALERT,
	V_WARN_CRITICAL,
	V_MAX_WARN
};


class textwin;

class statuswin : public window
{
public:
	statuswin(window *p);
	virtual ~statuswin();
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	void parse_statusline(string str);

private:
	void add_cond(string str, int warnno, int color);
	SDL_Surface *statusbg;
	textwin *tokenarray[5][5];
};

extern statuswin *stwin;

#endif
