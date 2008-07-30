#ifndef _inventory_h_
#define _inventory_h_

#include "mainwin.h"


class inventory : public mainwin
{
public:
	inventory(window *p, int nh_wt);
	virtual int draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
};


#endif
