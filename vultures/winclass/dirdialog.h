#ifndef _dirdialog_h_
#define _dirdialog_h_

#include "mainwin.h"

class hotspot;

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
