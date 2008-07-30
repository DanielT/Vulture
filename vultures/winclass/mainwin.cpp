
#include "mainwin.h"


mainwin::mainwin(window *p, int nh_wt, window_type wt) : window(p, nh_wt, wt)
{

}


int mainwin::draw()
{
	return 0;
}

eventresult mainwin::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}