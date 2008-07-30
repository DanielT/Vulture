
#include "minimap.h"



minimap::minimap(window *p, int nh_wt) : window(p, nh_wt, V_WINTYPE_MAP)
{

}

int minimap::draw()
{
	return 0;
}

eventresult minimap::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}
