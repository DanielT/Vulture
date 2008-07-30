
#include "hotspot.h"


hotspot::hotspot(window *p, int nh_wt, window_type wt) : window(p, nh_wt, wt)
{

}


int hotspot::draw()
{
	return 0;
}

eventresult hotspot::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}