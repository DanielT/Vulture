/* NetHack may be freely redistributed.  See license for details. */

#include "nhwindow.h"
#include "window.h"

#include <vector>

static std::vector<nhwindow*> vulture_nhwindows(8);
int windowcount = 0;
int windowcount_max = 8;

nhwindow* vulture_get_nhwindow(int winid)
{
	return vulture_nhwindows[winid];
}


nhwindow::nhwindow(int type)
{
	id = 0;
	
	/* if necessary make space in the vulture_windows array */
	if (windowcount == windowcount_max) {
		vulture_nhwindows.resize(windowcount_max + 8, NULL);
		windowcount_max += 8;

		/* no need to search through the first windowcount_cur ids, they're definitely taken */
		id = windowcount;
	}
	else
		while (vulture_nhwindows[id] != NULL && id < windowcount_max)
			id++;
	
	windowcount++;
	vulture_nhwindows[id] = this;

	this->type = type;
	impl = NULL;
	ending_type = 0;
	caption.clear();
	has_objects = false;
}


nhwindow::~nhwindow()
{
	vulture_nhwindows[id] = NULL;
	windowcount--;
	
	if (impl)
		delete impl;
	
	/* clean up after the last window is gone */
	if (windowcount == 0) {
		vulture_nhwindows.clear();
		windowcount_max = 0;
	}
}


void nhwindow::add_menuitem(std::string str, bool preselected, void *identifier, char accelerator, char group_accelerator, int glyph)
{
	items.push_back(menuitem(str, preselected, identifier, accelerator, group_accelerator, glyph));
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

