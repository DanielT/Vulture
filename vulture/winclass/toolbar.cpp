/* NetHack may be freely redistributed.  See license for details. */

#include "vultures_gfl.h"
#include "vultures_gra.h"
#include "vultures_sdl.h"
#include "vultures_win.h"
#include "vultures_mou.h"
#include "vultures_opt.h"
#include "vultures_tile.h"

#include "toolbar.h"
#include "hotspot.h"
#include "messagewin.h"
#include "map.h"
#include "levelwin.h"
#include "contextmenu.h"


toolbar::toolbar(window *p, int menuid, bool visible, int x, int y, std::string imgfile, const tb_buttondesc buttons[5]) : window(p)
{
	this->bgimage = vultures_load_graphic(imgfile);
	this->w = bgimage->w;
	this->h = bgimage->h;
	this->x = x;
	this->y = y;
	this->visible = visible;
	this->menu_id = menuid;

	/* Create buttons */
	new hotspot(this, 4, 0, 38, 39, buttons[0].menu_id, buttons[0].name);
	new hotspot(this, 44, 0, 38, 39, buttons[1].menu_id, buttons[1].name);
	new hotspot(this, 84, 0, 38, 39, buttons[2].menu_id, buttons[2].name);
	new hotspot(this, 124, 0, 38, 39, buttons[3].menu_id, buttons[3].name);
	new hotspot(this, 164, 0, 38, 39, buttons[4].menu_id, buttons[4].name);
	new hotspot(this, 184, 0, 38, 39, buttons[5].menu_id, buttons[5].name);

}

toolbar::~toolbar()
{
	SDL_FreeSurface(bgimage);
}

bool toolbar::draw()
{
	vultures_set_draw_region(abs_x, abs_y, abs_x + w - 1, abs_y + h - 1);
	vultures_put_img(abs_x, abs_y, bgimage);
	vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);

	vultures_invalidate_region(abs_x, abs_y, w, h);
	
	return true;
}


eventresult toolbar::handle_timer_event(window* target, void* result, int time)
{
	if (time > HOVERTIMEOUT)
		if (target != this  && !target->caption.empty())
			vultures_mouse_set_tooltip(target->caption);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult toolbar::handle_mousemotion_event(window* target, void* result, 
                                              int xrel, int yrel, int state)
{
	vultures_set_mcursor(V_CURSOR_NORMAL);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult toolbar::handle_mousebuttonup_event(window* target, void* result,
                                int mouse_x, int mouse_y, int button, int state)
{
	/* throw away uninteresting clicks */
	if (button != SDL_BUTTON_LEFT || target == this || !target->menu_id)
		return V_EVENT_HANDLED_NOREDRAW;

	/* one of the buttons was clicked */
	switch (target->menu_id) {
    case V_HOTSPOT_BUTTON_GOLD:
      {
        int action;
        contextmenu *menu = new contextmenu( levwin );

        menu->add_item("Check Wallet", V_ACTION_GOLD_QUERY);
        menu->add_item("Drop Gold", V_ACTION_GOLD_DROP);
        menu->add_item("Throw Gold", V_ACTION_GOLD_THROW);
        if (*u.ushops)
          menu->add_item("Pay shopkeeper", V_ACTION_GOLD_PAY);

        menu->layout();
        vultures_event_dispatcher(&action, V_RESPOND_INT, menu );

        switch (action) {

          case V_ACTION_GOLD_QUERY:
            ((vultures_event*)result)->num = '$';
            break;

          case V_ACTION_GOLD_DROP:
            vultures_eventstack_add('$', -1, -1, V_RESPOND_CHARACTER);
            ((vultures_event*)result)->num = 'd';
            break;

          case V_ACTION_GOLD_THROW:
            vultures_eventstack_add('$', -1, -1, V_RESPOND_CHARACTER);
            ((vultures_event*)result)->num = 't';
            break;

          case V_ACTION_GOLD_PAY:
            ((vultures_event*)result)->num = 'p';
            break;

        }

        delete menu;
      }
			return V_EVENT_HANDLED_FINAL;

		case V_HOTSPOT_BUTTON_LOOK:
			vultures_eventstack_add('y', -1, -1, V_RESPOND_CHARACTER);
			((vultures_event*)result)->num = '/';
			return V_EVENT_HANDLED_FINAL;

		case V_HOTSPOT_BUTTON_EXTENDED:
			((vultures_event*)result)->num = '#';
			return V_EVENT_HANDLED_FINAL;

		case V_HOTSPOT_BUTTON_MAP:
			map::toggle();
			return V_EVENT_HANDLED_REDRAW;

		case V_HOTSPOT_BUTTON_SPELLBOOK:
			((vultures_event*)result)->num = 'Z';
			return V_EVENT_HANDLED_FINAL;

		case V_HOTSPOT_BUTTON_INVENTORY:
			((vultures_event*)result)->num = 'i';
			return V_EVENT_HANDLED_FINAL;

		case V_HOTSPOT_BUTTON_DISCOVERIES:
			((vultures_event*)result)->num = '\\';
			return V_EVENT_HANDLED_FINAL;

		case V_HOTSPOT_BUTTON_MESSAGES:
			msgwin->view_all();
			break;

		case V_HOTSPOT_BUTTON_OPTIONS:
			((vultures_event*)result)->num = 'O';
			return V_EVENT_HANDLED_FINAL;

		case V_HOTSPOT_BUTTON_IFOPTIONS:
			vultures_iface_opts();
			break;

		case V_HOTSPOT_BUTTON_HELP:
			((vultures_event*)result)->num = '?';
			return V_EVENT_HANDLED_FINAL;
	}
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult toolbar::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	x = parent->w - (w + 6);
	if (menu_id == V_WIN_TOOLBAR1)
		y = parent->h - (h * 2 + 8);
	else
		y = parent->h - (h + 6);

	return V_EVENT_HANDLED_NOREDRAW;
}
