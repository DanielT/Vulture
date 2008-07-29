
#include "mainwin.h"


mainwin::mainwin(window *p, int nh_wt) : window(p, nh_wt, V_WINTYPE_MAIN)
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