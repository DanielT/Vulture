/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _dirdialog_h_
#define _dirdialog_h_

#include "mainwin.h"

class hotspot;

class dirdialog : public mainwin
{
public:
	dirdialog(window *p, string ques);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	
private:
	hotspot *arr;
	int arroffset_x, arroffset_y;
};


#endif
