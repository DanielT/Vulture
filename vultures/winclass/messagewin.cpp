
#include "messagewin.h"


messagewin::messagewin(window *p, int nh_wt) : window(p, nh_wt, V_WINTYPE_CUSTOM)
{

}

int messagewin::draw()
{
	return 0;
}

eventresult messagewin::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}