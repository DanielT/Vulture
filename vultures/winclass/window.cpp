#include <vector>

#include "window.h"


std::vector<window*> vultures_windows(16);
static int windowcount = 0;


window::window(window *p, int nh_wt, window_type wt) : parent(p), nh_type(nh_wt), v_type(wt)
{
	int pos = 0;
	
	if (windowcount < vultures_windows.size())
		while (vultures_windows[pos] != NULL && pos < vultures_windows.size())
			pos++;
	else
		pos = windowcount;
	
	vultures_windows[pos] = this;
	windowcount = vultures_windows.size();
	
	id = pos;
	need_redraw = true;
	visible = true;
	
	if (parent != NULL)
	{
		/* add win to the parent's ll of children */
		sib_prev = parent->last_child;
		if (parent->first_child)
			parent->last_child->sib_next = this;
		else
			parent->first_child = this;
		parent->last_child = this;
	}
}
