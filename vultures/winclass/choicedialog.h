#ifndef _choicedialog_h_
#define _choicedialog_h_

#include "mainwin.h"


class choicedialog : public mainwin
{
public:
	choicedialog(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
