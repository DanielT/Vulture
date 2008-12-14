#if 0


extern "C" {
#include "hack.h"
}

#include "vultures_gra.h"
#include "vultures_txt.h"
#include "vultures_win.h"
#include "vultures_sdl.h"

#include "inventory.h"


inventory::inventory(window *p) : menuwin(p)
{
	v_type = V_WINTYPE_OBJWIN;
}

bool inventory::draw()
{
    char *stored_caption;
    char label[32];
    int ix ,iy, iw, ih, labelwidth, buttonspace;

    buttonspace = 0;
    if (select_how != PICK_NONE)
        buttonspace = vultures_get_lineheight(V_FONT_LARGE) + 14;

    /* draw the window, but prevent draw_mainwin from drawing the caption */
    stored_caption = caption;
    caption = NULL;
    
    mainwin::draw();
    
    caption = stored_caption;

    ix = abs_x + vultures_winelem.border_left->w;
    iy = abs_y + vultures_winelem.border_top->h;
    iw = w - vultures_winelem.border_left->w - vultures_winelem.border_right->w;
    ih = h - vultures_winelem.border_top->h - vultures_winelem.border_bottom->h;

    int headline_height = vultures_get_lineheight(V_FONT_HEADLINE);
    int headline_width = vultures_text_length(V_FONT_HEADLINE, caption);

    vultures_fill_rect(ix, iy, ix + iw - 1, iy + headline_height*2, CLR32_BLACK_A50);

    vultures_line(ix, iy, ix + iw - 1, iy, CLR32_GRAY20);
    vultures_line(ix, iy + headline_height*2, ix + iw - 1, iy + headline_height * 2, CLR32_GRAY20);

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


void vultures_update_invscroll(struct window * win, int newpos)
{
	struct window * winelem;
	int itemcount = 0;
	int itemcol;
	int leftoffset = vultures_winelem.border_left->w;
	int topoffset = vultures_winelem.border_top->h;

	topoffset += vultures_get_lineheight(V_FONT_HEADLINE)*2 + 2;

	if (newpos+win->pd.ow_vcols > win->pd.ow_ncols)
		newpos = win->pd.ow_ncols - win->pd.ow_vcols;
	else if (newpos < 0)
		newpos = 0;

	win->pd.ow_firstcol = newpos;

	winelem = win->first_child;
	while (winelem)
	{
		if (winelem->v_type == V_WINTYPE_OBJITEM || winelem->v_type == V_WINTYPE_OBJITEMHEADER)
		{
			itemcol = (itemcount / win->pd.ow_vrows);

			winelem->x = (itemcol - newpos) * (V_LISTITEM_WIDTH + 4) + leftoffset;
			winelem->y = (itemcount % win->pd.ow_vrows) * V_LISTITEM_HEIGHT + topoffset;

			winelem->visible = (itemcol >= newpos && itemcol < newpos + win->pd.ow_vcols);

			itemcount++;
		}

		if (winelem->v_type == V_WINTYPE_BUTTON)
		{
			if (winelem->menu_id == V_INV_PREVPAGE)
				winelem->visible = (newpos != 0);
			else if (winelem->menu_id == V_INV_NEXTPAGE)
				winelem->visible = (newpos+win->pd.ow_vcols < win->pd.ow_ncols);
		}

		winelem = winelem->sib_next;
	}
}


int vultures_inventory_context_menu(struct window * target)
{
	int action = 0, key = 0;
	struct window * menu;

	menu = vultures_create_window_internal(0, NULL, V_WINTYPE_DROPDOWN);
	vultures_create_button(menu, "Apply", V_INVACTION_APPLY);

	if (!target->pd.obj->owornmask)
	{
		/* if you can wear it there's no way you can eat or drink it */
		if (target->pd.obj->oclass == POTION_CLASS)
			vultures_create_button(menu, "Drink", V_INVACTION_DRINK);
		vultures_create_button(menu, "Eat", V_INVACTION_EAT);
	}

	vultures_create_button(menu, "Read", V_INVACTION_READ);

	if (target->pd.obj->oclass == WAND_CLASS)
		vultures_create_button(menu, "Zap", V_INVACTION_ZAP);

	/* you could already be wearing it, then you can't wear it again */
	if (!target->pd.obj->owornmask && target->pd.obj->oclass != WAND_CLASS)
	{
		if (target->pd.obj->oclass != RING_CLASS && target->pd.obj->oclass != AMULET_CLASS)
			vultures_create_button(menu, "Wear", V_INVACTION_WEAR);

		if (target->pd.obj->oclass != ARMOR_CLASS)
			vultures_create_button(menu, "Put on", V_INVACTION_PUT_ON);
	}

	vultures_create_button(menu, "Wield", V_INVACTION_WIELD);

	if (target->pd.obj->owornmask)
		vultures_create_button(menu, "Remove", V_INVACTION_REMOVE);

	if (!target->pd.obj->owornmask)
		vultures_create_button(menu, "Drop", V_INVACTION_DROP);

	if (!objects[target->pd.obj->otyp].oc_name_known)
		vultures_create_button(menu, "Name", V_INVACTION_NAME);

	vultures_layout_dropdown(menu);

	vultures_event_dispatcher(&action, V_RESPOND_INT, menu);

	vultures_destroy_window_internal(menu);

	if (action)
	{
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
				select_off(target->pd.obj); /* sets takoff_mask */
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


eventresult inventory::event_handler(window* target, void* result, SDL_Event* event)
{
	point mouse;
	int key;
	int key_ok;

	switch (event->type)
	{
		case SDL_MOUSEMOTION:
			vultures_set_mcursor(V_CURSOR_NORMAL);
			break;

		case SDL_MOUSEBUTTONUP:
			mouse = vultures_get_mouse_pos();
			/* close the window if the user clicks outside it */
			if (handler == target && (mouse.x < handler->abs_x || mouse.y < handler->abs_y ||
									mouse.x > handler->abs_x + handler->w ||
									mouse.y > handler->abs_y + handler->h))
				return V_EVENT_HANDLED_FINAL;

			/* left clicks on object items do nothing */
			if (event->button.button == SDL_BUTTON_LEFT &&
					target != handler && target->v_type == V_WINTYPE_OBJITEM)
				return V_EVENT_HANDLED_NOREDRAW;

			/* right clicks on object items open a context menu */
			else if (event->button.button == SDL_BUTTON_RIGHT && target->v_type == V_WINTYPE_OBJITEM)
				return vultures_inventory_context_menu(target);

			/* other functions are outsourced... */
			else
				return vultures_eventh_objwin(handler, target, result, event);


		case SDL_KEYDOWN:
			key_ok = 0;
			key = event->key.keysym.unicode;
			switch (event->key.keysym.sym)
			{
				case SDLK_HOME:
				case SDLK_END:
				case SDLK_PAGEDOWN:
				case SDLK_KP2:
				case SDLK_DOWN:
				case SDLK_PAGEUP:
				case SDLK_KP8:
				case SDLK_UP:
				case SDLK_LEFT:
				case SDLK_RIGHT:
					key_ok = 1;
					break;

				default: break;
			}

			switch (key)
			{
				case MENU_PREVIOUS_PAGE:
				case MENU_NEXT_PAGE:
				case MENU_FIRST_PAGE:
				case MENU_LAST_PAGE:
				case MENU_SEARCH:
					key_ok = 1;

				default: break;
			}

			if (key_ok)
				return vultures_eventh_objwin(handler, target, result, event);

			else if (!key)
				return V_EVENT_HANDLED_NOREDRAW;

			else
				return V_EVENT_HANDLED_FINAL;

		case SDL_VIDEORESIZE:
			return vultures_eventh_objwin(handler, target, result, event);
	}

	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult inventory::objwin_event_handler(window* target, void* result, SDL_Event* event)
{
	struct window * winelem;
	char * str_to_find;
	int key, itemcount, colno;

	switch (event->type)
	{
		case SDL_MOUSEMOTION:
			vultures_set_mcursor(V_CURSOR_NORMAL);
			break;

		case SDL_MOUSEBUTTONUP:
			if (event->button.button == SDL_BUTTON_WHEELUP)
			{
				if (handler->pd.ow_firstcol > 0)
				{
					/* scroll inventory backwards */
					vultures_update_invscroll(handler, handler->pd.ow_firstcol - 1);
					handler->need_redraw = 1;
					return V_EVENT_HANDLED_REDRAW;
				}
				return V_EVENT_HANDLED_NOREDRAW;
			}

			else if (event->button.button == SDL_BUTTON_WHEELDOWN)
			{
				if (handler->pd.ow_firstcol + handler->pd.ow_vcols < handler->pd.ow_ncols)
				{
					/* scroll inventory forwards */
					vultures_update_invscroll(handler, handler->pd.ow_firstcol + 1);
					handler->need_redraw = 1;
					return V_EVENT_HANDLED_REDRAW;
				}
				return V_EVENT_HANDLED_NOREDRAW;
			}

			else if (event->button.button == SDL_BUTTON_LEFT)
			{
				if (handler == target)
					break;

				if (target->v_type == V_WINTYPE_BUTTON)
				{
					switch (target->menu_id)
					{
						case V_MENU_ACCEPT:
						case V_MENU_CANCEL:
						case V_INV_CLOSE:
							*(int*)result = target->menu_id;
							return V_EVENT_HANDLED_FINAL;

						case V_INV_PREVPAGE:
							vultures_update_invscroll(handler, handler->pd.ow_firstcol - 1);
							handler->need_redraw = 1;
							return V_EVENT_HANDLED_REDRAW;

						case V_INV_NEXTPAGE:
							vultures_update_invscroll(handler, handler->pd.ow_firstcol + 1);
							handler->need_redraw = 1;
							return V_EVENT_HANDLED_REDRAW;

					}
				}

				/* select a range of items from target (clicked item) to handler->pd.ow_lasttoggled (previously clicked item) */
				if (target->v_type == V_WINTYPE_OBJITEM && (SDL_GetModState() & KMOD_LSHIFT) && 
					handler->select_how != PICK_ONE && handler->pd.ow_lasttoggled)
				{
					int selectme = 0;
					for (winelem = handler->first_child; winelem; winelem = winelem->sib_next)
					{
						if (winelem == target || winelem == handler->pd.ow_lasttoggled)
						{
							selectme = !selectme;
							winelem->selected = 1;
							winelem->pd.count = -1;
						}

						if (selectme && winelem->v_type == V_WINTYPE_OBJITEM)
						{
							winelem->selected = 1;
							winelem->pd.count = -1;
						}
					}

					handler->pd.ow_lasttoggled->last_toggled = 0;
					handler->pd.ow_lasttoggled = target;
					target->last_toggled = 1;

					handler->need_redraw = 1;
					return V_EVENT_HANDLED_REDRAW;
				}
				else if (target->v_type == V_WINTYPE_OBJITEM)
				{
					select_option(handler, target, -1);

					if (handler->pd.ow_lasttoggled)
						handler->pd.ow_lasttoggled->last_toggled = 0;
					handler->pd.ow_lasttoggled = target;
					target->last_toggled = 1;

					if (handler->select_how == PICK_ONE)
					{
						*(int*)result = V_MENU_ACCEPT;
						return V_EVENT_HANDLED_FINAL;
					}

					handler->need_redraw = 1;
					return V_EVENT_HANDLED_REDRAW;
				}
			}
			break;

		case SDL_KEYDOWN:
			handler->need_redraw = 1;
			key = event->key.keysym.unicode;
			switch (event->key.keysym.sym)
			{
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					*(int*)result = V_MENU_ACCEPT;
					return V_EVENT_HANDLED_FINAL;

				case SDLK_SPACE:
				case SDLK_ESCAPE:
					*(int*)result = (handler->content_is_text) ? V_MENU_ACCEPT : V_MENU_CANCEL;
					return V_EVENT_HANDLED_FINAL;

				/* handle menu control keys */
				case SDLK_HOME:     key = MENU_FIRST_PAGE;    /* '^' */ break;
				case SDLK_END:      key = MENU_LAST_PAGE;     /* '|' */ break;

				/* scroll via arrow keys */
				case SDLK_PAGEDOWN:
				case SDLK_KP2:
				case SDLK_DOWN:
				case SDLK_RIGHT:
					vultures_update_invscroll(handler, handler->pd.ow_firstcol + 1);
					return V_EVENT_HANDLED_REDRAW;

				case SDLK_PAGEUP:
				case SDLK_KP8:
				case SDLK_UP:
				case SDLK_LEFT:
					vultures_update_invscroll(handler, handler->pd.ow_firstcol - 1);
					return V_EVENT_HANDLED_REDRAW;

				case SDLK_BACKSPACE:
					if (handler->pd.ow_lasttoggled)
						handler->pd.ow_lasttoggled->pd.count = handler->pd.ow_lasttoggled->pd.count / 10;
					return V_EVENT_HANDLED_REDRAW;

				default: break;
			}

			if (!key)
				/* a function or modifier key, but not one we recognize, was pressed */
				return V_EVENT_HANDLED_NOREDRAW;

			switch (key)
			{
				case MENU_PREVIOUS_PAGE:
					vultures_update_invscroll(handler, handler->pd.ow_firstcol - 1);
					return V_EVENT_HANDLED_REDRAW;

				case MENU_NEXT_PAGE:
					vultures_update_invscroll(handler, handler->pd.ow_firstcol + 1);
					return V_EVENT_HANDLED_REDRAW;

				case MENU_FIRST_PAGE:
					vultures_update_invscroll(handler, 0);
					return V_EVENT_HANDLED_REDRAW;

				case MENU_LAST_PAGE:
					vultures_update_invscroll(handler, 999999);
					return V_EVENT_HANDLED_REDRAW;


				case MENU_SELECT_ALL:
				case MENU_UNSELECT_ALL:
					/* invalid for single selection menus */
					if (handler->select_how == PICK_ONE)
						return V_EVENT_HANDLED_NOREDRAW;

					winelem = handler->first_child;
					while (winelem)
					{
						if (winelem->v_type == V_WINTYPE_OBJITEM)
						{
							winelem->selected = (key == MENU_SELECT_ALL);
							winelem->pd.count = -1;
						}
						winelem = winelem->sib_next;
					}
					return V_EVENT_HANDLED_REDRAW;


				case MENU_INVERT_ALL:
					/* invalid for single selection menus */
					if (handler->select_how == PICK_ONE)
						return V_EVENT_HANDLED_NOREDRAW;

					winelem = handler->first_child;
					while (winelem)
					{
						if (winelem->v_type == V_WINTYPE_OBJITEM)
						{
							winelem->selected = !winelem->selected;
							winelem->pd.count = -1;
						}
						winelem = winelem->sib_next;
					}
					return V_EVENT_HANDLED_REDRAW;


				case MENU_SELECT_PAGE:
				case MENU_UNSELECT_PAGE:
					/* invalid for single selection menus */
					if (handler->select_how == PICK_ONE)
						return V_EVENT_HANDLED_NOREDRAW;

					winelem = handler->first_child;
					while (winelem)
					{
						if (winelem->v_type == V_WINTYPE_OBJITEM && winelem->visible)
						{
							winelem->selected = (key == MENU_SELECT_PAGE);
							winelem->pd.count = -1;
						}
						winelem = winelem->sib_next;
					}
					return V_EVENT_HANDLED_REDRAW;


				case MENU_INVERT_PAGE:
					/* invalid for single selection menus */
					if (handler->select_how == PICK_ONE)
						return V_EVENT_HANDLED_NOREDRAW;

					winelem = handler->first_child;
					while (winelem)
					{
						if (winelem->v_type == V_WINTYPE_OBJITEM && winelem->visible)
						{
							winelem->selected = !winelem->selected;
							winelem->pd.count = -1;
						}
						winelem = winelem->sib_next;
					}
					return V_EVENT_HANDLED_REDRAW;


				case MENU_SEARCH:
					str_to_find = (char *)malloc(512);
					str_to_find[0] = '\0';
					if (vultures_get_input(-1, -1, "What are you looking for?", str_to_find) != -1)
					{
						itemcount = 0;
						winelem = handler->first_child;
						while (winelem)
						{
							itemcount++;
							if (winelem->caption && strstr(winelem->caption, str_to_find))
							{
								colno = itemcount / handler->pd.ow_vrows;
								vultures_update_invscroll(handler, colno);
								break;
							}

							winelem = winelem->sib_next;
						}

						if (handler->pd.ow_lasttoggled)
							handler->pd.ow_lasttoggled->last_toggled = 0;
						handler->pd.ow_lasttoggled = winelem;
						if (winelem)
							winelem->last_toggled = 1;
					}
					free(str_to_find);
					return V_EVENT_HANDLED_REDRAW;

				default:
					/* numbers are part of a count */
					if (key >= '0' && key <= '9' && handler->pd.ow_lasttoggled && 
						handler->pd.ow_lasttoggled->pd.count < 1000000)
					{
						if (handler->pd.ow_lasttoggled->pd.count == -1)
							handler->pd.ow_lasttoggled->pd.count = 0;
						handler->pd.ow_lasttoggled->pd.count = handler->pd.ow_lasttoggled->pd.count * 10 + (key - '0');

						return V_EVENT_HANDLED_REDRAW;
					}

					/* try to match the key to an accelerator */
					target = vultures_accel_to_win(handler, key);
					if (target)
					{
						select_option(handler, target, -1);
						handler->pd.count = 0;
						if (handler->select_how == PICK_ONE)
						{
							*(int*)result = V_MENU_ACCEPT;
							return V_EVENT_HANDLED_FINAL;
						}

						if (handler->pd.ow_lasttoggled)
							handler->pd.ow_lasttoggled->last_toggled = 0;
						handler->pd.ow_lasttoggled = target;
						target->last_toggled = 1;


						/* if the selected element isn't visible bring it into view */
						if (!target->visible)
						{
							itemcount = 0;
							winelem = handler->first_child;
							while (winelem)
							{
								itemcount++;
								winelem = winelem->sib_next;
								if (winelem == target)
									break;
							}
							colno = itemcount / handler->pd.ow_vrows;
							vultures_update_invscroll(handler, colno);
						}
						return V_EVENT_HANDLED_REDRAW;
					}
					break;
			}
			break;

		case SDL_VIDEORESIZE:
			if (handler->visible)
			{
				/* hide_window takes care of the background */
				vultures_hide_window(handler);

				/* resize */
				vultures_layout_itemwin(handler);

				/* redraw */
				handler->visible = 1;
				handler->need_redraw = 1;
			}
			return V_EVENT_HANDLED_NOREDRAW;
	}

	return V_EVENT_HANDLED_NOREDRAW;
}


window* inventory::replace_win(window* win)
{
	window::replace_win(win);
	v_type = V_WINTYPE_OBJWIN;
	nh_type = NHW_MENU;
	
	return this;
}



void inventory::layout()
{
    struct window *winelem, *tmpwin;
    struct obj * invitem;
    int itemcount = 0;
    int ncols, nrows;
    char * newcaption;
    int maxitems_per_col, maxitems_per_row;
    int textheight;
    
    reset();

    if (!caption)
        caption = strdup("Inventory");

    int leftoffset = vultures_winelem.border_left->w;
    int topoffset = vultures_winelem.border_top->h;
    int rightoffset = vultures_winelem.border_right->w;
    int bottomoffset = 60; /* guesstimate for bottom border + page arrows + minimal spacing */

    textheight = vultures_get_lineheight(V_FONT_LARGE);
    topoffset += textheight*2 + 2;

    if (select_how != PICK_NONE)
        bottomoffset += (textheight + 14);

    v_type = V_WINTYPE_OBJWIN;
    draw = vultures_draw_objwin;

    if (select_how == PICK_NONE)
        event_handler = vultures_eventh_inventory;
    else
        event_handler = vultures_eventh_objwin;


    winelem = first_child;
    while (winelem)
    {
        if (winelem->v_type == V_WINTYPE_OPTION)
        {
            itemcount++;
            vultures_init_wintype(winelem, V_WINTYPE_OBJITEM);
            if (select_how != PICK_NONE)
                winelem->event_handler = vultures_eventh_objitem;
        }
        else if (winelem->v_type == V_WINTYPE_TEXT)
        {
            itemcount++;
            vultures_init_wintype(winelem, V_WINTYPE_OBJITEMHEADER);
        }
        else if (winelem->v_type == V_WINTYPE_OBJITEM || winelem->v_type == V_WINTYPE_OBJITEMHEADER)
            itemcount++;
        else if (winelem->v_type == V_WINTYPE_BUTTON)
        {
            tmpwin = winelem;
            winelem = winelem->sib_next;
            vultures_destroy_window_internal(tmpwin);
            continue;
        }

        winelem = winelem->sib_next;
    }
    
    for (std::vector<menuitem*>::iterator iter = items.begin(); iter != items.end(); iter++)
    {
    	menuitem *item = *iter;
    	if (item->identifier != NULL)
    	{
    		new objitem(this, item->)
    	}
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

    pd.ow_vcols = ncols > maxitems_per_row ? maxitems_per_row : ncols;
    pd.ow_vrows = nrows > maxitems_per_col ? maxitems_per_col : nrows;
    pd.ow_ncols = ncols;

    pd.ow_firstcol = 0;

    itemcount = 0;
    winelem = first_child;
    while (winelem)
    {
        if (winelem->v_type == V_WINTYPE_OBJITEM || winelem->v_type == V_WINTYPE_OBJITEMHEADER)
        {
            winelem->x = (itemcount / pd.ow_vrows) * (V_LISTITEM_WIDTH + 4) + leftoffset;
            winelem->y = (itemcount % pd.ow_vrows) * V_LISTITEM_HEIGHT + topoffset;
            winelem->visible = ((itemcount / pd.ow_vrows) < pd.ow_vcols);


            /* find the objects associated with the items */
            if (winelem->v_type == V_WINTYPE_OBJITEM)
            {
                /* nethack may have been nice and passed an object pointer in menu_id_v
                 * Unforunately, we need this ugly hack to try to discern between
                 * chars, small ints and pointers */
                if (winelem->menu_id_v > (void*)0x10000)
                    winelem->pd.obj = (struct obj *)winelem->menu_id_v; 
                else if (id == WIN_INVEN)
                {
                    invitem = invent;
                    while (invitem && invitem->invlet != winelem->accelerator)
                        invitem = invitem->nobj;
                    winelem->pd.obj = invitem;
                }

                if (winelem->caption[0] == '[')
                {
                    newcaption = strdup(winelem->caption+6);
                    free(winelem->caption);
                    winelem->caption = newcaption;
                }
            }

            itemcount++;
        }
        winelem = winelem->sib_next;
    }

    w = pd.ow_vcols * (V_LISTITEM_WIDTH + 4) - 4 + leftoffset + rightoffset;
    h = pd.ow_vrows * V_LISTITEM_HEIGHT + topoffset + vultures_winelem.border_bottom->h;

    if (ncols > pd.ow_vcols)
    {
        h += 25;

        winelem = vultures_create_button(win, NULL, V_INV_PREVPAGE);
        winelem->x = leftoffset;
        winelem->y = h - vultures_winelem.border_bottom->h - 23;
        winelem->w = 100;
        winelem->h = 24;
        winelem->image = vultures_get_img_src(0, 0, vultures_winelem.invarrow_left->w,
                                vultures_winelem.invarrow_left->h, vultures_winelem.invarrow_left);
        winelem->visible = 0;

        winelem = vultures_create_button(win, NULL, V_INV_NEXTPAGE);
        winelem->x = w - rightoffset - 101;
        winelem->y = h - vultures_winelem.border_bottom->h - 23;
        winelem->w = 100;
        winelem->h = 24;
        winelem->image = vultures_get_img_src(0, 0, vultures_winelem.invarrow_right->w,
                                vultures_winelem.invarrow_right->h, vultures_winelem.invarrow_right);
        winelem->visible = 1;
    }

    if (select_how != PICK_NONE)
    {
        int btn1_width = vultures_text_length(V_FONT_MENU, "Accept") + 14;
        int btn2_width = vultures_text_length(V_FONT_MENU, "Cancel") + 14;
        int max_width = (btn1_width > btn2_width) ? btn1_width : btn2_width;
        int total_width = 2 * max_width + 10;

        h += textheight + 14;

        winelem = vultures_create_button(win, "Accept", V_MENU_ACCEPT);
        winelem->h = textheight + 10;
        winelem->y = h - vultures_winelem.border_bottom->h - (textheight + 12);
        winelem->w = max_width;
        winelem->x = (w - rightoffset - leftoffset - total_width) / 2 + leftoffset;

        winelem = vultures_create_button(win, "Cancel", V_MENU_CANCEL);
        winelem->h = textheight + 10;
        winelem->y = h - vultures_winelem.border_bottom->h - (textheight + 12);
        winelem->w = max_width;
        winelem->x = (w - rightoffset - leftoffset - total_width) / 2 + leftoffset + max_width + 10;
    }
    else
    {
        winelem = vultures_create_button(win, NULL, V_INV_CLOSE);
        winelem->visible = 1;
        winelem->x = w - vultures_winelem.border_right->w - 28;
        winelem->y = vultures_winelem.border_top->h + 2;
        winelem->w = 26;
        winelem->h = 26;
        winelem->image = vultures_get_img_src(0, 0, vultures_winelem.closebutton->w,
                                vultures_winelem.closebutton->h, vultures_winelem.closebutton);
    }

    x = (parent->w - w) / 2;
    y = (parent->h - h) / 2;

    abs_x = parent->x + x;
    abs_y = parent->y + y;

}

#endif // 0