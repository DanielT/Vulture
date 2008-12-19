#ifndef _textwin_h_
#define _textwin_h_

#include "window.h"


class textwin : public window
{
public:
// 	textwin() {};
	textwin(window *p, string cap);
	textwin(window *p, int destsize);
	virtual void set_caption(string str);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	
	int textcolor;

private:
	bool is_input;
};


#endif
