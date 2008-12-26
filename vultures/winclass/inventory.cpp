/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

extern "C" {
#include "hack.h"

extern int take_off();
extern int select_off(struct obj *);
extern long takeoff_mask;
extern const char *disrobing;
}

#include "vultures_gra.h"
#include "vultures_txt.h"
#include "vultures_win.h"
#include "vultures_sdl.h"
#include "vultures_mou.h"
#include "vultures_tile.h"

#include "inventory.h"
#include "objitemwin.h"
#include "objheaderwin.h"
#include "contextmenu.h"
#include "button.h"


#define META(c) (0x80 | (c))
#define CTRL(c) (0x1f & (c))


inventory::inventory(window *p, std::list<menuitem> &menuitems, int how, int id) :
                     menuwin(p, menuitems, how), nhid(id)
{
	v_type = V_WINTYPE_OBJWIN;

	ow_ncols = ow_vcols = ow_firstcol = ow_vrows = 0;
	ow_lasttoggled = NULL;
}


bool inventory::draw()
{
	string stored_caption;
	char label[32];
	int ix ,iy, iw, ih, labelwidth, buttonspace;

	buttonspace = 0;
	if (select_how != PICK_NONE)
		buttonspace = vultures_get_lineheight(V_FONT_LARGE) + 14;

	/* draw the window, but prevent draw_mainwin from drawing the caption */
	stored_caption = caption;
	caption.clear();
	
	mainwin::draw();
	
	caption = stored_caption;

	ix = abs_x + border_left;
	iy = abs_y + border_top;
	iw = w - border_left - border_right;
	ih = h - border_top - border_bottom;

	int headline_height = vultures_get_lineheight(V_FONT_HEADLINE);
	int headline_width = vultures_text_length(V_FONT_HEADLINE, caption);

	vultures_fill_rect(ix, iy, ix + iw - 1, iy + headline_height * 2, CLR32_BLACK_A50);

	vultures_line(ix, iy, ix + iw - 1, iy, CLR32_GRAY20);
	vultures_line(ix, iy + headline_height * 2, ix + iw - 1,
				iy + headline_height * 2, CLR32_GRAY20);

	vultures_put_text_shadow(V_FONT_HEADLINE, caption, vultures_screen, ix+(iw-headline_width)/2,
							iy+headline_height/2+2, CLR32_WHITE, CLR32_GRAY20);

	if (ow_ncols > ow_vcols) {
		vultures_line(x, y+h-buttonspace-25, x+w-1, y+h-buttonspace-25, CLR32_GRAY77);
		
		snprintf(label, 32, "%d - %d / %d", ow_firstcol + 1, ow_firstcol + ow_vcols, ow_ncols);
		labelwidth = vultures_text_length(V_FONT_MENU, label);

		vultures_put_text_shadow(V_FONT_MENU, label, vultures_screen, x+(w-labelwidth)/2,
								y+h-buttonspace-18, CLR32_BLACK, CLR32_GRAY20);
	}

	return 1;
}


void inventory::update_invscroll(int newpos)
{
	struct window * winelem;
	int itemcount = 0;
	int itemcol;
	int leftoffset = border_left;
	int topoffset = border_top;

	topoffset += vultures_get_lineheight(V_FONT_HEADLINE) * 2 + 2;

	if (newpos + ow_vcols > ow_ncols)
		newpos = ow_ncols - ow_vcols;
	else if (newpos < 0)
		newpos = 0;

	ow_firstcol = newpos;

	
	for (winelem = first_child; winelem; winelem = winelem->sib_next) {
		if (winelem->v_type == V_WINTYPE_OBJITEM ||
			winelem->v_type == V_WINTYPE_OBJITEMHEADER) {
			itemcol = (itemcount / ow_vrows);

			winelem->x = (itemcol - newpos) * (V_LISTITEM_WIDTH + 4) + leftoffset;
			winelem->y = (itemcount % ow_vrows) * V_LISTITEM_HEIGHT + topoffset;

			winelem->visible = (itemcol >= newpos && itemcol < newpos + ow_vcols);

			itemcount++;
		}

		else if (winelem->v_type == V_WINTYPE_BUTTON) {
			if (winelem->menu_id == V_INV_PREVPAGE)
				winelem->visible = (newpos != 0);
			else if (winelem->menu_id == V_INV_NEXTPAGE)
				winelem->visible = (newpos + ow_vcols < ow_ncols);
		}
	}
}


eventresult inventory::context_menu(objitemwin *target)
{
	int action = 0, key = 0;
	contextmenu *menu;
	
	menu = new contextmenu(ROOTWIN);
	menu->add_item("Apply", V_INVACTION_APPLY);

	if (!target->obj->owornmask)
	{
		/* if you can wear it there's no way you can eat or drink it */
		if (target->obj->oclass == POTION_CLASS)
			menu->add_item("Drink", V_INVACTION_DRINK);
		menu->add_item("Eat", V_INVACTION_EAT);
	}

	menu->add_item("Read", V_INVACTION_READ);

	if (target->obj->oclass == WAND_CLASS)
		menu->add_item("Zap", V_INVACTION_ZAP);

	/* you could already be wearing it, then you can't wear it again */
	if (!target->obj->owornmask && target->obj->oclass != WAND_CLASS) {
		if (target->obj->oclass != RING_CLASS && target->obj->oclass != AMULET_CLASS)
			menu->add_item("Wear", V_INVACTION_WEAR);

		if (target->obj->oclass != ARMOR_CLASS)
			menu->add_item("Put on", V_INVACTION_PUT_ON);
	}

	menu->add_item("Wield", V_INVACTION_WIELD);

	if (target->obj->owornmask)
		menu->add_item("Remove", V_INVACTION_REMOVE);

	if (!target->obj->owornmask)
		menu->add_item("Drop", V_INVACTION_DROP);

	if (!objects[target->obj->otyp].oc_name_known)
		menu->add_item("Name", V_INVACTION_NAME);

	menu->layout();
	vultures_event_dispatcher(&action, V_RESPOND_INT, menu);

	delete menu;


	if (action) {
		vultures_eventstack_add('i', -1, -1, V_RESPOND_POSKEY);

		switch (action)
		{
			case V_INVACTION_APPLY: key = 'a'; break;
			case V_INVACTION_DRINK: key = 'q'; break;
			case V_INVACTION_EAT:   key = 'e'; break;
			case V_INVACTION_READ:  key = 'r'; break;
			case V_INVACTION_ZAP:   key = 'z'; break;
			case V_INVACTION_WEAR:  key = 'W'; break;
			case V_INVACTION_PUT_ON:key = 'P'; break;
			case V_INVACTION_WIELD: key = 'w'; break;
			case V_INVACTION_REMOVE:
				/* we call a bunch of functions in do_wear.c directly here;
					* we can do so safely because take_off() directly accounts for
					* elapsed turns */
				select_off(target->obj); /* sets takoff_mask */
				if (takeoff_mask)
				{
					/* default activity for armor and/or accessories,
						* possibly combined with weapons */
					disrobing = "disrobing";

					/* specific activity when handling weapons only */
					if (!(takeoff_mask & ~(W_WEP|W_SWAPWEP|W_QUIVER)))
						disrobing = "disarming";

					(void) take_off();
				}
				/* having performed an action we need to return to the main game loop
					* so that thing like AC and vision (because of helmets & amulets of ESP)
					* get recalculated.
					* However we do not want to perform any more actions or cause messages
					* to be printed. CTRL+r (redraw) is a suitable NOP */
				key = CTRL('r');
				return V_EVENT_HANDLED_FINAL;

			case V_INVACTION_NAME:
				vultures_eventstack_add(target->menu_id, -1, -1, V_RESPOND_ANY);
				vultures_eventstack_add('n', -1,-1, V_RESPOND_ANY);
				vultures_eventstack_add(META('n'), -1, -1, V_RESPOND_POSKEY);
				return V_EVENT_HANDLED_FINAL;

			case V_INVACTION_DROP:  key = 'd'; break;
		}

		vultures_eventstack_add(target->menu_id, -1, -1, V_RESPOND_CHARACTER);
		vultures_eventstack_add(key, -1, -1, V_RESPOND_POSKEY);

		return V_EVENT_HANDLED_FINAL;
	}

	return V_EVENT_HANDLED_NOREDRAW;
}



eventresult inventory::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	vultures_set_mcursor(V_CURSOR_NORMAL);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult inventory::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	point mouse = vultures_get_mouse_pos();
	window *winelem;
	objitemwin *oitem;
	
	if (select_how == PICK_NONE) {
		/* close the window if the user clicks outside it */
		if (this == target && (mouse.x < abs_x || mouse.y < abs_y ||
							mouse.x > abs_x + w || mouse.y > abs_y + h))
			return V_EVENT_HANDLED_FINAL;

		/* left clicks on object items do nothing */
		if (button == SDL_BUTTON_LEFT &&
				target != this && target->v_type == V_WINTYPE_OBJITEM)
			return V_EVENT_HANDLED_NOREDRAW;

		/* right clicks on object items open a context menu */
		else if (button == SDL_BUTTON_RIGHT && target->v_type == V_WINTYPE_OBJITEM)
			return context_menu(static_cast<objitemwin*>(target));
	}
	
	if (button == SDL_BUTTON_WHEELUP) {
		if (ow_firstcol > 0) {
			/* scroll inventory backwards */
			update_invscroll(ow_firstcol - 1);
			need_redraw = 1;
			return V_EVENT_HANDLED_REDRAW;
		}
		return V_EVENT_HANDLED_NOREDRAW;
	}

	else if (button == SDL_BUTTON_WHEELDOWN) {
		if (ow_firstcol + ow_vcols < ow_ncols) {
			/* scroll inventory forwards */
			update_invscroll(ow_firstcol + 1);
			need_redraw = 1;
			return V_EVENT_HANDLED_REDRAW;
		}
		return V_EVENT_HANDLED_NOREDRAW;
	}

	else if (button == SDL_BUTTON_LEFT) {
		if (this == target)
			return V_EVENT_HANDLED_NOREDRAW;

		if (target->v_type == V_WINTYPE_BUTTON) {
			switch (target->menu_id)
			{
				case V_MENU_ACCEPT:
				case V_MENU_CANCEL:
				case V_INV_CLOSE:
					*(int*)result = target->menu_id;
					return V_EVENT_HANDLED_FINAL;

				case V_INV_PREVPAGE:
					update_invscroll(ow_firstcol - 1);
					need_redraw = 1;
					return V_EVENT_HANDLED_REDRAW;

				case V_INV_NEXTPAGE:
					update_invscroll(ow_firstcol + 1);
					need_redraw = 1;
					return V_EVENT_HANDLED_REDRAW;
			}
		}

		/* select a range of items from target (clicked item) to handler->pd.ow_lasttoggled (previously clicked item) */
		if (target->v_type == V_WINTYPE_OBJITEM && (SDL_GetModState() & KMOD_LSHIFT) && 
			select_how != PICK_ONE && ow_lasttoggled) {
			int selectme = 0;
			for (winelem = first_child; winelem; winelem = winelem->sib_next) {
				if (winelem == target || winelem == ow_lasttoggled) {
					selectme = !selectme;
					oitem = static_cast<objitemwin*>(winelem);
					oitem->item->selected = 1;
					oitem->item->count = -1;
				}

				if (selectme && winelem->v_type == V_WINTYPE_OBJITEM) {
					oitem = static_cast<objitemwin*>(winelem);
					oitem->item->selected = 1;
					oitem->item->count = -1;
				}
			}

			ow_lasttoggled->last_toggled = 0;
			ow_lasttoggled = static_cast<objitemwin*>(target);
			static_cast<objitemwin*>(target)->last_toggled = 1;

			need_redraw = 1;
			return V_EVENT_HANDLED_REDRAW;
		}
		else if (target->v_type == V_WINTYPE_OBJITEM) {
			select_option(static_cast<objitemwin*>(target), -1);

			if (ow_lasttoggled)
				ow_lasttoggled->last_toggled = false;
			ow_lasttoggled = static_cast<objitemwin*>(target);
			ow_lasttoggled->last_toggled = true;

			if (select_how == PICK_ONE) {
				*(int*)result = V_MENU_ACCEPT;
				return V_EVENT_HANDLED_FINAL;
			}

			need_redraw = 1;
			return V_EVENT_HANDLED_REDRAW;
		}
	}

	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult inventory::handle_keydown_event(window* target, void* result, int sym, int mod, int unicode)
{
	window *winelem;
	int itemcount, colno, key;

	need_redraw = 1;
	key = unicode;
	switch (sym) {
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			*(int*)result = V_MENU_ACCEPT;
			return V_EVENT_HANDLED_FINAL;

		case SDLK_SPACE:
		case SDLK_ESCAPE:
			*(int*)result = (select_how == PICK_NONE) ? V_MENU_ACCEPT : V_MENU_CANCEL;
			return V_EVENT_HANDLED_FINAL;

		/* handle menu control keys */
		case SDLK_HOME:     key = MENU_FIRST_PAGE;    /* '^' */ break;
		case SDLK_END:      key = MENU_LAST_PAGE;     /* '|' */ break;

		/* scroll via arrow keys */
		case SDLK_PAGEDOWN:
		case SDLK_KP2:
		case SDLK_DOWN:
		case SDLK_RIGHT:
			update_invscroll(ow_firstcol + 1);
			return V_EVENT_HANDLED_REDRAW;

		case SDLK_PAGEUP:
		case SDLK_KP8:
		case SDLK_UP:
		case SDLK_LEFT:
			update_invscroll(ow_firstcol - 1);
			return V_EVENT_HANDLED_REDRAW;

		case SDLK_BACKSPACE:
			if (ow_lasttoggled)
				ow_lasttoggled->item->count = ow_lasttoggled->item->count / 10;
			return V_EVENT_HANDLED_REDRAW;

		default: break;
	}

	if (!key)
		/* a function or modifier key, but not one we recognize, was pressed */
		return V_EVENT_HANDLED_NOREDRAW;

	switch (key) {
		case MENU_PREVIOUS_PAGE:
			update_invscroll(ow_firstcol - 1);
			return V_EVENT_HANDLED_REDRAW;

		case MENU_NEXT_PAGE:
			update_invscroll(ow_firstcol + 1);
			return V_EVENT_HANDLED_REDRAW;

		case MENU_FIRST_PAGE:
			update_invscroll(0);
			return V_EVENT_HANDLED_REDRAW;

		case MENU_LAST_PAGE:
			update_invscroll(999999);
			return V_EVENT_HANDLED_REDRAW;


		case MENU_SELECT_ALL:
		case MENU_UNSELECT_ALL:
			/* invalid for single selection menus */
			if (select_how != PICK_ANY)
				return V_EVENT_HANDLED_NOREDRAW;

			for (winelem = first_child; winelem; winelem = winelem->sib_next) {
				if (winelem->v_type == V_WINTYPE_OBJITEM) {
					static_cast<objitemwin*>(winelem)->item->selected =
											(key == MENU_SELECT_ALL);
					static_cast<objitemwin*>(winelem)->item->count = -1;
				}
			}
			return V_EVENT_HANDLED_REDRAW;


		case MENU_INVERT_ALL:
			/* invalid for single selection menus */
			if (select_how != PICK_ANY)
				return V_EVENT_HANDLED_NOREDRAW;

			for (winelem = first_child; winelem; winelem = winelem->sib_next) {
				if (winelem->v_type == V_WINTYPE_OBJITEM) {
					static_cast<objitemwin*>(winelem)->item->selected = 
					!static_cast<objitemwin*>(winelem)->item->selected;
					static_cast<objitemwin*>(winelem)->item->count = -1;
				}
			}
			return V_EVENT_HANDLED_REDRAW;


		case MENU_SELECT_PAGE:
		case MENU_UNSELECT_PAGE:
			/* invalid for single selection menus */
			if (select_how != PICK_ANY)
				return V_EVENT_HANDLED_NOREDRAW;

			for (winelem = first_child; winelem; winelem = winelem->sib_next) {
				if (winelem->v_type == V_WINTYPE_OBJITEM && winelem->visible) {
					static_cast<objitemwin*>(winelem)->item->selected = (key == MENU_SELECT_PAGE);
					static_cast<objitemwin*>(winelem)->item->count = -1;
				}
			}
			return V_EVENT_HANDLED_REDRAW;


		case MENU_INVERT_PAGE:
			/* invalid for single selection menus */
			if (select_how != PICK_ANY)
				return V_EVENT_HANDLED_NOREDRAW;

			for (winelem = first_child; winelem; winelem = winelem->sib_next) {
				if (winelem->v_type == V_WINTYPE_OBJITEM && winelem->visible) {
					static_cast<objitemwin*>(winelem)->item->selected =
						!static_cast<objitemwin*>(winelem)->item->selected;
					static_cast<objitemwin*>(winelem)->item->count = -1;
				}
			}
			return V_EVENT_HANDLED_REDRAW;


		case MENU_SEARCH:
			char str_to_find[512];
			str_to_find[0] = '\0';
			if (vultures_get_input(-1, -1, "What are you looking for?", str_to_find) != -1)
			{
				itemcount = 0;
				for (winelem = first_child; winelem; winelem = winelem->sib_next) {
					itemcount++;
					if (winelem->caption.find(str_to_find)) {
						colno = itemcount / ow_vrows;
						update_invscroll(colno);
						break;
					}
				}

				if (ow_lasttoggled)
					ow_lasttoggled->last_toggled = false;
				ow_lasttoggled = static_cast<objitemwin*>(winelem);
				if (ow_lasttoggled)
					ow_lasttoggled->last_toggled = true;
			}
			return V_EVENT_HANDLED_REDRAW;

		default:
			if (select_how == PICK_NONE)
				return V_EVENT_HANDLED_FINAL;
		
			/* numbers are part of a count */
			if (key >= '0' && key <= '9' && ow_lasttoggled && 
				ow_lasttoggled->item->count < 1000000) {
				if (ow_lasttoggled->item->count == -1)
					ow_lasttoggled->item->count = 0;
				ow_lasttoggled->item->count = ow_lasttoggled->item->count * 10 + (key - '0');

				return V_EVENT_HANDLED_REDRAW;
			}

			/* try to match the key to an accelerator */
			target = find_accel(key);
			if (target) {
				select_option(static_cast<objitemwin*>(target), -1);
				if (select_how == PICK_ONE) {
					*(int*)result = V_MENU_ACCEPT;
					return V_EVENT_HANDLED_FINAL;
				}

				if (ow_lasttoggled)
					ow_lasttoggled->last_toggled = 0;
				ow_lasttoggled = static_cast<objitemwin*>(target);
				ow_lasttoggled->last_toggled = 1;


				/* if the selected element isn't visible bring it into view */
				if (!target->visible) {
					itemcount = 0;
					for (winelem = first_child; winelem && winelem != target;
						winelem = winelem->sib_next)
						itemcount++;
					
					colno = itemcount / ow_vrows;
					update_invscroll(colno);
				}
				return V_EVENT_HANDLED_REDRAW;
			}
			break;
	}
	
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult inventory::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	if (visible) {
		/* hide_window takes care of the background */
		hide();

		/* resize */
		layout();

		/* redraw */
		visible = 1;
		need_redraw = 1;
		return V_EVENT_HANDLED_REDRAW;
	}
	
	return V_EVENT_HANDLED_NOREDRAW;
}


void inventory::layout()
{
	window *winelem;
	button *btn;
	struct obj * invitem;
	
	int itemcount = 0;
	int ncols, nrows;
	int maxitems_per_col, maxitems_per_row;
	int textheight;

	/* remove all child windows - they get (re-)created here */
	while (first_child)
		delete first_child;

	if (caption.empty())
		caption = "Inventory";

	int leftoffset = border_left;
	int topoffset = border_top;
	int rightoffset = border_right;
	int bottomoffset = 60; /* guesstimate for bottom border + page arrows + minimal spacing */

	textheight = vultures_get_lineheight(V_FONT_LARGE);
	topoffset += textheight*2 + 2;

	if (select_how != PICK_NONE)
		bottomoffset += (textheight + 14);

	for (item_iterator i = items.begin(); i != items.end(); ++i) {
		if (!i->identifier)
			new objheaderwin(this, i->str);
		else
			new objitemwin(this, &(*i), i->str, i->accelerator, 
						i->glyph, i->preselected, select_how == PICK_ANY);
		itemcount++;

	}

	/* how many items will fit on the screen vertically */
	maxitems_per_col = (parent->h - topoffset - bottomoffset) / V_LISTITEM_HEIGHT;

	maxitems_per_row = (parent->w - 2*leftoffset - 10) / (V_LISTITEM_WIDTH + 4);

	/* calc number of rows to contain itemcount items */
	ncols = itemcount / maxitems_per_col + (itemcount % maxitems_per_col != 0 ? 1 : 0);

	/* distribute items evenly among the rows */
	nrows = itemcount / ncols + (itemcount % ncols != 0 ? 1 : 0);

	/* if there are more columns than can be displayed, prefer to maximize
	* space usage over even distribution of the items */
	if (ncols > maxitems_per_row)
		nrows = maxitems_per_col;

	ow_vcols = ncols > maxitems_per_row ? maxitems_per_row : ncols;
	ow_vrows = nrows > maxitems_per_col ? maxitems_per_col : nrows;
	ow_ncols = ncols;

	ow_firstcol = 0;

	itemcount = 0;
	for (winelem = first_child; winelem; winelem = winelem->sib_next) {
		if (winelem->v_type == V_WINTYPE_OBJITEM ||
		    winelem->v_type == V_WINTYPE_OBJITEMHEADER) {
			winelem->x = (itemcount / ow_vrows) * (V_LISTITEM_WIDTH + 4) + leftoffset;
			winelem->y = (itemcount % ow_vrows) * V_LISTITEM_HEIGHT + topoffset;
			winelem->visible = ((itemcount / ow_vrows) < ow_vcols);

			/* find the objects associated with the items */
			if (winelem->v_type == V_WINTYPE_OBJITEM) {
				/* nethack may have been nice and passed an object pointer in menu_id_v
				 * Unforunately, we need this ugly hack to try to discern between
				 * chars, small ints and pointers */
				if (winelem->menu_id_v > (void*)0x10000)
					static_cast<objitemwin*>(winelem)->obj = (struct obj *)winelem->menu_id_v; 
				else if (nhid == WIN_INVEN) {
					invitem = invent;
					while (invitem && invitem->invlet != winelem->accelerator)
						invitem = invitem->nobj;
					static_cast<objitemwin*>(winelem)->obj = invitem;
				}
			}

			itemcount++;
		}
	}

	w = ow_vcols * (V_LISTITEM_WIDTH + 4) - 4 + leftoffset + rightoffset;
	h = ow_vrows * V_LISTITEM_HEIGHT + topoffset + border_bottom;

	if (ncols > ow_vcols) {
		h += 25;

		btn = new button(this, "", V_INV_PREVPAGE, '\0');
		btn->x = leftoffset;
		btn->y = h - border_bottom - 23;
		btn->w = 100;
		btn->h = 24;
		btn->image = vultures_get_img_src(0, 0, vultures_winelem.invarrow_left->w,
								vultures_winelem.invarrow_left->h, vultures_winelem.invarrow_left);
		btn->visible = 0;

		btn = new button(this, "", V_INV_NEXTPAGE, '\0');
		btn->x = w - rightoffset - 101;
		btn->y = h - border_bottom - 23;
		btn->w = 100;
		btn->h = 24;
		btn->image = vultures_get_img_src(0, 0, vultures_winelem.invarrow_right->w,
								vultures_winelem.invarrow_right->h, vultures_winelem.invarrow_right);
		btn->visible = 1;
	}

	if (select_how != PICK_NONE) {
		int btn1_width = vultures_text_length(V_FONT_MENU, "Accept") + 14;
		int btn2_width = vultures_text_length(V_FONT_MENU, "Cancel") + 14;
		int max_width = (btn1_width > btn2_width) ? btn1_width : btn2_width;
		int total_width = 2 * max_width + 10;

		h += textheight + 14;

		btn = new button(this, "Accept", V_MENU_ACCEPT, '\0');
		btn->h = textheight + 10;
		btn->y = h - border_bottom - (textheight + 12);
		btn->w = max_width;
		btn->x = (w - rightoffset - leftoffset - total_width) / 2 + leftoffset;

		btn = new button(this, "Cancel", V_MENU_CANCEL, '\0');
		btn->h = textheight + 10;
		btn->y = h - border_bottom - (textheight + 12);
		btn->w = max_width;
		btn->x = (w - rightoffset - leftoffset - total_width) / 2 + leftoffset + max_width + 10;
	} else {
		btn = new button(this, "", V_INV_CLOSE, '\0');
		btn->visible = 1;
		btn->x = w - border_right - 28;
		btn->y = border_top + 2;
		btn->w = 26;
		btn->h = 26;
		btn->image = vultures_get_img_src(0, 0, vultures_winelem.closebutton->w,
								vultures_winelem.closebutton->h, vultures_winelem.closebutton);
	}

	x = (parent->w - w) / 2;
	y = (parent->h - h) / 2;

	abs_x = parent->x + x;
	abs_y = parent->y + y;

}
