
#include "map.h"


#define NHW_MAP 1


map::map(window *p, int nh_wt) : window(p, nh_wt, V_WINTYPE_MAP)
{

}

int map::draw()
{
	return 0;
}

eventresult map::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}
