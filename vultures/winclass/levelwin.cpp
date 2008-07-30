
#include "levelwin.h"


#define NHW_MAP 1


levelwin::levelwin() : window(NULL, NHW_MAP, V_WINTYPE_MAP)
{

}

int levelwin::draw()
{
	return 0;
}

eventresult levelwin::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}
