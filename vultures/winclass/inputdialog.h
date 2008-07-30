#ifndef _inputdialog_h_
#define _inputdialog_h_

#include "mainwin.h"


class inputdialog : public mainwin
{
public:
	inputdialog(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
