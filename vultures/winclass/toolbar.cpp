
#include "toolbar.h"


#define NHW_MAP 1


toolbar::toolbar(window *p, int nh_wt) : window(p, nh_wt, V_WINTYPE_MAP)
{

}

int toolbar::draw()
{
	return 0;
}

eventresult toolbar::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}
