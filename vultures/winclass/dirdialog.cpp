
#include "dirdialog.h"


dirdialog::dirdialog(window *p, int nh_wt) : mainwin(p, nh_wt, V_WINTYPE_MENU)
{

}

int dirdialog::draw()
{
	mainwin::draw();
	return 0;
}

eventresult dirdialog::event_handler(window* target, void* result, SDL_Event* event)
{
	return V_EVENT_HANDLED_FINAL;
}