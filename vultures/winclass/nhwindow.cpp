/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#include "nhwindow.h"
#include "window.h"

#include <vector>
using std::vector;

static vector<nhwindow*> vultures_nhwindows(8);
int windowcount = 0;
int windowcount_max = 8;

nhwindow* vultures_get_nhwindow(int winid)
{
	return vultures_nhwindows[winid];
}


nhwindow::nhwindow(int type)
{
	id = 0;
	
	/* if necessary make space in the vultures_windows array */
	if (windowcount == windowcount_max) {
		vultures_nhwindows.resize(windowcount_max + 8, NULL);
		windowcount_max += 8;

		/* no need to search through the first windowcount_cur ids, they're definitely taken */
		id = windowcount;
	}
	else
		while (vultures_nhwindows[id] != NULL && id < windowcount_max)
			id++;
	
	windowcount++;
	vultures_nhwindows[id] = this;

	this->type = type;
	impl = NULL;
	ending_type = 0;
	caption.clear();
	has_objects = false;
}


nhwindow::~nhwindow()
{
	vultures_nhwindows[id] = NULL;
	windowcount--;
	
	if (impl)
		delete impl;
	
	/* clean up after the last window is gone */
	if (windowcount == 0) {
		vultures_nhwindows.clear();
		windowcount_max = 0;
	}
}


void nhwindow::add_menuitem(string str, bool preselected, void *identifier, char accelerator, int glyph)
{
	items.push_back(menuitem(str, preselected, identifier, accelerator, glyph));
}


void nhwindow::reset()
{
	items.clear();
	ending_type = 0;
	caption.clear();
	has_objects = false;
	
	if (impl)
		delete impl;
	impl = NULL;
}

