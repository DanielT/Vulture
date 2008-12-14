#if 0

#ifndef _inventory_h_
#define _inventory_h_

#include "menuwin.h"


enum invactions {
	V_INVACTION_APPLY = 1,
	V_INVACTION_DRINK,
	V_INVACTION_EAT,
	V_INVACTION_READ,
	V_INVACTION_ZAP,
	V_INVACTION_WEAR,
	V_INVACTION_PUT_ON,
	V_INVACTION_WIELD,
	V_INVACTION_REMOVE,
	V_INVACTION_DROP,
	V_INVACTION_NAME
};


class inventory : public menuwin
{
public:
	inventory() {};
	inventory(window *p);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	virtual window* replace_win(window* win);
	virtual void layout();
	
private:
	int ow_ncols, ow_vcols, ow_firstcol;
};


#endif

#endif //0