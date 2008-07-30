
#include "button.h"


button::button(window *p, int nh_wt, window_type wt) : window(p, nh_wt, wt)
{

}


int button::draw()
{
	return 0;
}

eventresult button::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}
