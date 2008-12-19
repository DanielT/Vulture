#ifndef _inputdialog_h_
#define _inputdialog_h_

#include "mainwin.h"

class textwin;

class inputdialog : public mainwin
{
public:
	inputdialog(window *p, string caption, int size,
	            int force_x, int force_y);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	void copy_input(char *dest);

private:
	textwin *input;
	int destsize;
};


#endif
