
#include "enhancebutton.h"



enhancebutton::enhancebutton(window *p, int nh_wt) : window(p, nh_wt, V_WINTYPE_MAP)
{

}

int enhancebutton::draw()
{
	return 0;
}

eventresult enhancebutton::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}
