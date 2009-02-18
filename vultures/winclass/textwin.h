/* NetHack may be freely redistributed.  See license for details. */

#ifndef _textwin_h_
#define _textwin_h_

#include "window.h"


class textwin : public window
{
public:
	textwin(window *p, std::string cap);
	textwin(window *p, int destsize);
	virtual void set_caption(std::string str);
	virtual bool draw();
	
	int textcolor;

private:
	bool is_input;
};


#endif
