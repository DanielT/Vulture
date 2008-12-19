#ifndef _anykeydialog_h_
#define _anykeydialog_h_

#include "mainwin.h"

class textwin;

class anykeydialog : public mainwin
{
public:
	anykeydialog(window *p, string ques);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);

private:
	int count;
	textwin *txt;
};


#endif
