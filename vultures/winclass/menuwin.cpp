extern "C" {
	#include "hack.h"
}

#include "vultures_main.h"
#include "vultures_win.h"
#include "vultures_txt.h"
#include "vultures_mou.h"
#include "vultures_tile.h"

#include "menuwin.h"
#include "scrollwin.h"
#include "textwin.h"
#include "optionwin.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))


menuwin::menuwin()
{
	scrollarea = NULL;
	count = 0;
}

menuwin::menuwin(window *p) : mainwin(p)
{
	v_type = V_WINTYPE_MENU;
	nh_type = NHW_MENU;
	
	scrollarea = NULL;
	count = 0;
}


menuwin::~menuwin()
{
	items.clear();
}


menuwin* menuwin::replace_win(menuwin* win)
{
	*this = *win;
	
	window::replace_win(win);
	v_type = V_WINTYPE_MENU;
	nh_type = NHW_MENU;
	
	return this;
}


void menuwin::reset()
{
	while (first_child) {
		delete first_child;
	}
	
	items.clear();
	
	scrollarea = NULL;
	w = 0;
	
	if (background)
		SDL_FreeSurface(background);
	background = NULL;
}


bool menuwin::draw()
{
	/* draw the window + title */
	mainwin::draw();

	/* no need to invalidate the draw region, the call to draw mainwin did that for us */
	return true;
}



void menuwin::select_option(optionwin *target, int count)
{
	window *winelem;
	optionwin *opt;

	if (!target->is_checkbox) {
		/* unselect everything else */
		for (winelem = first_child; winelem; winelem = winelem->sib_next) {
			if (winelem->v_type == V_WINTYPE_OPTION ||
			    winelem->v_type == V_WINTYPE_OBJITEM) {
				opt = static_cast<optionwin*>(winelem);
				opt->item->selected = false;
				opt->item->count = -1;
				
			}
		}
		target->item->selected = true;
	}
	else
		target->item->selected = !target->item->selected;

	if (target->item->selected)
		target->item->count = count;
}


eventresult menuwin::event_handler(window* target, void* result, SDL_Event* event)
{
	window *winelem;
	optionwin *opt;
	int key;
	char * str_to_find;

	/* menus use the normal cursor */
	if (event->type == SDL_MOUSEMOTION)
		vultures_set_mcursor(V_CURSOR_NORMAL);

	/* handle clicks and wheel events */
	else if (event->type == SDL_MOUSEBUTTONUP)
	{
		if (target == this)
			/* clicks on the menu window itself are not interesting */
			return V_EVENT_HANDLED_NOREDRAW;


		/* click on an option / checkbox */
		if (target->v_type == V_WINTYPE_OPTION)
		{
			opt = static_cast<optionwin*>(target);
			select_option(opt, count ? count : -1);
			count = 0;
			if (select_how == PICK_ONE) {
				*(int*)result = V_MENU_ACCEPT;
				return V_EVENT_HANDLED_FINAL;
			}

			need_redraw = 1;
			return V_EVENT_HANDLED_REDRAW;
		}

		/* a click on a button */
		else if (target->v_type == V_WINTYPE_BUTTON && target->menu_id)
		{
			*(int*)result = target->menu_id;
			return V_EVENT_HANDLED_FINAL;
		}
	}


	/* handle keyboard events */
	else if (event->type == SDL_KEYDOWN)
	{
		need_redraw = 1;
		key = event->key.keysym.unicode;
		switch (event->key.keysym.sym)
		{
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				*(int*)result = V_MENU_ACCEPT;
				return V_EVENT_HANDLED_FINAL;

			case SDLK_SPACE:
			case SDLK_ESCAPE:
				*(int*)result = content_is_text ? V_MENU_ACCEPT : V_MENU_CANCEL;
				return V_EVENT_HANDLED_FINAL;

			/* handle menu control keys */
			case SDLK_PAGEUP:   key = MENU_PREVIOUS_PAGE; /* '<' */ break; 
			case SDLK_PAGEDOWN: key = MENU_NEXT_PAGE;     /* '>' */ break;
			case SDLK_HOME:     key = MENU_FIRST_PAGE;    /* '^' */ break;
			case SDLK_END:      key = MENU_LAST_PAGE;     /* '|' */ break;

			/* scroll via arrow keys */
			case SDLK_KP2:
			case SDLK_DOWN:
				return scrollarea->scrollto(V_SCROLL_LINE_REL, 1);

			case SDLK_KP8:
			case SDLK_UP:
				return scrollarea->scrollto(V_SCROLL_LINE_REL, -1);

			case SDLK_BACKSPACE:
				count = count / 10;

			default: break;
		}

		if (!key)
			/* a function key, but not one we recognize, wass pressed */
			return V_EVENT_HANDLED_NOREDRAW;

		switch (key)
		{
			case MENU_PREVIOUS_PAGE:
				return scrollarea->scrollto(V_SCROLL_PAGE_REL, -1);

			case MENU_NEXT_PAGE:
				return scrollarea->scrollto(V_SCROLL_PAGE_REL, 1);

			case MENU_FIRST_PAGE:
				return scrollarea->scrollto(V_SCROLL_PAGE_ABS, 0);

			case MENU_LAST_PAGE:
				return scrollarea->scrollto(V_SCROLL_PAGE_ABS, 9999);


			case MENU_SELECT_ALL:
			case MENU_UNSELECT_ALL:
				/* invalid for single selection menus */
				if (select_how == PICK_ONE)
					return V_EVENT_HANDLED_NOREDRAW;

				for (winelem = scrollarea->first_child; winelem; winelem = winelem->sib_next) {
					if (winelem->v_type == V_WINTYPE_OPTION) {
						opt = static_cast<optionwin*>(winelem);
						opt->item->selected = (key == MENU_SELECT_ALL);
						opt->item->count = -1;
					}
				}
				need_redraw = 1;
				return V_EVENT_HANDLED_REDRAW;


			case MENU_INVERT_ALL:
				/* invalid for single selection menus */
				if (select_how == PICK_ONE)
					return V_EVENT_HANDLED_NOREDRAW;

				for (winelem = scrollarea->first_child; winelem; winelem = winelem->sib_next) {
					if (winelem->v_type == V_WINTYPE_OPTION) {
						opt = static_cast<optionwin*>(winelem);
						opt->item->selected = !opt->item->selected;
						opt->item->count = -1;
					}
				}
				need_redraw = 1;
				return V_EVENT_HANDLED_REDRAW;


			case MENU_SELECT_PAGE:
			case MENU_UNSELECT_PAGE:
				/* invalid for single selection menus */
				if (select_how == PICK_ONE)
					return V_EVENT_HANDLED_NOREDRAW;

				for (winelem = scrollarea->first_child; winelem; winelem = winelem->sib_next) {
					if (winelem->v_type == V_WINTYPE_OPTION && winelem->visible) {
						opt = static_cast<optionwin*>(winelem);
						opt->item->selected = (key == MENU_SELECT_PAGE);
						opt->item->count = -1;
					}
				}
				need_redraw = 1;
				return V_EVENT_HANDLED_REDRAW;


			case MENU_INVERT_PAGE:
				/* invalid for single selection menus */
				if (select_how == PICK_ONE)
					return V_EVENT_HANDLED_NOREDRAW;

				for (winelem = scrollarea->first_child; winelem; winelem = winelem->sib_next) {
					if (winelem->v_type == V_WINTYPE_OPTION && winelem->visible) {
						opt = static_cast<optionwin*>(winelem);
						opt->item->selected = !opt->item->selected;
						opt->item->count = -1;
					}
				}
				need_redraw = 1;
				return V_EVENT_HANDLED_REDRAW;


			case MENU_SEARCH:
				str_to_find = (char *)malloc(512);
				str_to_find[0] = '\0';
				if (vultures_get_input(-1, -1, "What are you looking for?", str_to_find) != -1) {
					for (winelem = scrollarea->first_child; winelem; winelem = winelem->sib_next) {
						if (winelem->caption.find(str_to_find)) {
							scrollarea->scrollto(V_SCROLL_PIXEL_ABS, winelem->y);
							break;
						}
					}
				}
				free(str_to_find);
				need_redraw = 1;
				return V_EVENT_HANDLED_REDRAW;

			default:
				/* numbers are part of a count */
				if (key >= '0' && key <= '9') {
					count = count * 10 + (key - '0');
					break;
				}
			
				/* try to match the key to an accelerator */
				target = scrollarea->find_accel(key);
				if (target)
				{
					select_option(static_cast<optionwin*>(target), count ? count : -1);
					count = 0;
					if (select_how == PICK_ONE) {
						*(int*)result = V_MENU_ACCEPT;
						return V_EVENT_HANDLED_FINAL;
					}

					/* if the selected element isn't visible bring it into view */
					if (!target->visible)
						scrollarea->scrollto(V_SCROLL_PIXEL_ABS, target->y);

					need_redraw = 1;
					return V_EVENT_HANDLED_REDRAW;
				}
				break;
		}
	}
	else if (event->type == SDL_VIDEORESIZE)
	{
		layout();
		return V_EVENT_HANDLED_NOREDRAW;
	}

	return V_EVENT_HANDLED_NOREDRAW;
}


void menuwin::assign_accelerators()
{
	int new_accel;
	char used_accels[128]; /* 65 should be max # of accels, the other 63 bytes are safety :P */

	used_accels[0] = '\0';
	for (item_iterator i = items.begin(); i != items.end(); ++i) {
		if (i->accelerator == 0 && i->identifier) {
			new_accel = vultures_find_menu_accelerator(used_accels);
			if (new_accel >= 0)
				i->accelerator = new_accel;
		}
	}
}


void menuwin::set_selection_type(int how)
{
	select_how = how;
}


window * menuwin::find_accel(char accel)
{
	for (window *child = first_child; child; child = child->sib_next) {
		if (child->accelerator == accel)
			return child;
	}

	return NULL;
}


void menuwin::layout()
{
	int scrollheight = 0;
	int buttonheight = vultures_get_lineheight(V_FONT_MENU) + 15;
	int menu_height_limit;
	string newcaption;
	
	// remove existing scrollarea
	if (scrollarea)
		delete scrollarea;
	
	// create & populate new scrollarea
	scrollarea = new scrollwin(this);
	for (item_iterator i = items.begin(); i != items.end(); ++i) {
		newcaption = "";
		if (i->accelerator) {
			newcaption += "[ ] - ";
			newcaption[1] = i->accelerator;
		}
		newcaption += i->str;
	
		if (!i->identifier || select_how == PICK_NONE)
			new textwin(scrollarea, newcaption);
		else
			new optionwin(scrollarea, &(*i), newcaption, i->accelerator, 
			             i->glyph, i->preselected, select_how == PICK_ANY);
	}
	scrollarea->layout();
	scrollheight = scrollarea->get_scrollheight();
	
	/* set actual scroll area height */
	h = scrollheight + buttonheight;
	menu_height_limit = parent->get_h() - get_frameheight() - buttonheight - 30;
	if (h > menu_height_limit) {
		h = min(menu_height_limit, MAX_MENU_HEIGHT);
		scrollheight = h - buttonheight;
		scrollarea->set_height(scrollheight);
	}
	
	mainwin::layout();
	
	/* enlarge scroll area so that the scrollbar will be on the right edge */
	scrollarea->w = w - border_left - border_right;
}


void menuwin::add_menuitem(string str, bool preselected, void *identifier, char accelerator, int glyph)
{
	items.push_back(menuitem(str, preselected, identifier, accelerator, glyph));
}


/* selection iterator */
menuwin::selection_iterator::selection_iterator(item_iterator start, item_iterator end) : iter(start), end(end)
{
	while (iter != end && !iter->selected)
		iter++;
}

menuwin::selection_iterator& menuwin::selection_iterator::operator++(void)
{
	iter++;
	while (iter != end && !iter->selected)
		iter++;
	
	return *this;
}

bool menuwin::selection_iterator::operator!=(selection_iterator rhs) const
{
	return this->iter != rhs.iter;
}

menuitem& menuwin::selection_iterator::operator*()
{
	return *iter;
}
