#ifndef _objitemwin_h_
#define _objitemwin_h_

#include "optionwin.h"

struct obj;

class objitemwin : public optionwin
{
public:
	objitemwin(window *p, menuitem* mi, string cap,
	          char accel, int glyph, bool selected);
	virtual bool draw();
	virtual eventresult event_handler(window *target, void *result, SDL_Event *event);

	struct obj *obj;
	bool last_toggled;
	
private:
	bool hover;
};


#endif
