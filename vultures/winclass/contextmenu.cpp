
#include "contextmenu.h"


contextmenu::contextmenu(window *p, int nh_wt, window_type wt) : window(p, nh_wt, wt)
{

}


int contextmenu::draw()
{
	return 0;
}

eventresult contextmenu::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}