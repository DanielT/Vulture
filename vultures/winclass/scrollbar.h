/* NetHack may be freely redistributed.  See license for details. */

#ifndef _scrollbar_h_
#define _scrollbar_h_

#include "window.h"


class scrollbar : public window
{
public:
	scrollbar(window *p, int scrolloff);
	virtual bool draw();

	int scrollpos;
private:
};


#endif
