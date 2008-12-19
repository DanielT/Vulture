#ifndef _contextmenu_h_
#define _contextmenu_h_

#include <vector>
#include <utility>

#include "window.h"


class contextmenu : public window
{
public:
	typedef std::vector<std::pair<char*, int> >  itemlist;

	contextmenu(window *p);
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	virtual void layout(void);
	void add_item(const char *label, int menuid);

private:
	itemlist items;
	bool layout_done;
};


#endif
