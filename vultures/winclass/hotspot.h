/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _hotspot_h_
#define _hotspot_h_

#include "window.h"


class hotspot : public window
{
public:
	hotspot(window *parent, int x, int y, int w, int h, int menu_id, string name);
	virtual bool draw();
};


#endif
