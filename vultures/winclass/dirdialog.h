#ifndef _dirdialog_h_
#define _dirdialog_h_

#include "mainwin.h"


class dirdialog : public mainwin
{
public:
	dirdialog(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
