
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


toolbar::toolbar(window *p, int menuid, bool visible, int x, int y, const char *imgfile, const tb_buttondesc buttons[5]) : window(p)
{
	this->bgimage = vultures_load_graphic(NULL, imgfile);
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


eventresult toolbar::event_handler(window* target, void* result, SDL_Event* event)
{
	switch (event->type)
	{
		/* mousemotion sets the correct cursor */
		case SDL_MOUSEMOTION:
			vultures_set_mcursor(V_CURSOR_NORMAL);
			break;

		/* timer: draw tooltips */
		case SDL_TIMEREVENT:
			if (event->user.code > HOVERTIMEOUT)
				if (target != this  && target->caption)
					vultures_mouse_set_tooltip(target->caption);
			break;

		/* click events */
		case SDL_MOUSEBUTTONUP:
			/* throw away uninterseting clicks */
			if (event->button.button != SDL_BUTTON_LEFT ||
				target == this || !target->menu_id)
				break;

			/* one of the buttons was clicked */
			switch (target->menu_id)
			{
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


		case SDL_VIDEORESIZE:
			this->x = this->parent->w - (this->w + 6);
			if (this->menu_id == V_WIN_TOOLBAR1)
				this->y = this->parent->h - (this->h*2 + 8);
			else
				this->y = this->parent->h - (this->h + 6);
			break;

		default: break;
	}
	
	return V_EVENT_HANDLED_NOREDRAW;
}
