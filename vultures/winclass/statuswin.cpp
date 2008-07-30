
#include "statuswin.h"


statuswin::statuswin(window *p, int nh_wt) : window(p, nh_wt, V_WINTYPE_CUSTOM)
{

}

int statuswin::draw()
{
	return 0;
}

eventresult statuswin::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}