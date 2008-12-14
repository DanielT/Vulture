#ifndef _inputdialog_h_
#define _inputdialog_h_

#include "mainwin.h"
#include "textwin.h"


class inputdialog : public mainwin
{
public:
	inputdialog(window *p, const char *caption, int size);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	void copy_input(char *dest);

private:
	textwin *input;
	int destsize;
};


#endif
