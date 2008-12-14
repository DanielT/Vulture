#ifndef _dirdialog_h_
#define _dirdialog_h_

#include "mainwin.h"
#include "hotspot.h"

/* for defs of V_MAP_XMOD and V_MAP_YMOD */
#include "levelwin.h"


class dirdialog : public mainwin
{
public:
	dirdialog(window *p, const char *ques);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	
private:
	hotspot *arr;
	int arroffset_x, arroffset_y;
};


#endif
