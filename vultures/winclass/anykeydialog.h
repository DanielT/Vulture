#ifndef _anykeydialog_h_
#define _anykeydialog_h_

#include "mainwin.h"


class anykeydialog : public mainwin
{
public:
	anykeydialog(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
