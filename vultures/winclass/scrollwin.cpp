/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#include "scrollwin.h"
#include "scrollbar.h"
#include "textwin.h"

#include "vultures_gen.h"
#include "vultures_gra.h"
#include "vultures_win.h"
#include "vultures_sdl.h"
#include "vultures_txt.h"
#include "vultures_mou.h"
#include "vultures_tile.h"


scrollwin::scrollwin(window *p, bool txt) : window(p), is_text(txt)
{
	v_type = V_WINTYPE_SCROLLAREA;
	scrollpos = 0;
	layout_done = false;
	scroll = NULL;
}


bool scrollwin::draw()
{
	int scrolloffset = 0;

	scrolloffset = (scrollpos * (get_scrollheight() - this->h)) / 8192.0;

	/* ensure correct clipping */
	vultures_set_draw_region(abs_x, abs_y, abs_x + w, abs_y + h);

	for (struct window *winelem = first_child; winelem; winelem = winelem->sib_next) {
		winelem->abs_x = abs_x + winelem->x;
		winelem->abs_y = abs_y + winelem->y;

		if (winelem->v_type != V_WINTYPE_SCROLLBAR)
			winelem->abs_y -= scrolloffset;
			
		/* elements muss be at least half visible */
		if (winelem->abs_y + winelem->h/2 >= this->abs_y &&
		    winelem->abs_y + winelem->h/2 <= this->abs_y + this->h) {
			winelem->visible = 1;
			winelem->draw();
			winelem->need_redraw = 0;
		} else
			winelem->visible = 0;
	}
	
	/* unrestricted drawing */
	vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);
	
	return false;
}


int scrollwin::get_scrollheight(void)
{
	int max_y = 0;
	int lower_edge = 0;
	
	if (first_child == NULL)
		return 0;
	
	for (struct window *winelem = first_child; winelem; winelem = winelem->sib_next) {
		if (winelem != scroll) {
			lower_edge = winelem->y + winelem->h;
			max_y = (max_y > lower_edge) ? max_y : lower_edge;
		}
	}
	
	/* add one line of extra space, to make sure the last element can be scrolled into view */
	return max_y + vultures_get_lineheight(V_FONT_MENU); 
}


/* calc the with of a menu item */
int scrollwin::get_menuitem_width(window *item, int colwidths[8])
{
	int width, i, thiscol, btnwidth;
	size_t prevpos, pos;
	string coltxt;
	
	/* if this is an option leave space for the checkbox in the first column */
	btnwidth = 0;
	if (item->v_type == V_WINTYPE_OPTION)
		btnwidth = vultures_winelem.checkbox_off->w + 4;
		
	pos = item->caption.find_first_of('\t');
	if (pos == string::npos)
		return vultures_text_length(V_FONT_MENU, item->caption) + btnwidth + 5;

	width = prevpos = i = 0;
	do {
		/* get the string for this column */
		coltxt = item->caption.substr(prevpos, pos - prevpos);
		
		/* trim trailing whitespace */
		trim(coltxt);

		/* get the item's width */
		thiscol = vultures_text_length(V_FONT_MENU, coltxt) + 5;
		
		if (i == 0)
			thiscol += btnwidth;
	
		/* adjust the column with to fit, if necessary */
		if (thiscol > colwidths[i])
			colwidths[i] = thiscol;

		width += colwidths[i];
		prevpos = ++pos;
		i++;
		pos = item->caption.find_first_of('\t', prevpos);
	} while (prevpos-1 != string::npos && i <= 7);
	
	return width;
}


void scrollwin::layout(void)
{
	window *winelem, *coltxt;
	int i, elem_maxwidth, saved_scrollpos, height;
	int colwidths[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	int colstart[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	string txt, orig_caption;
	bool done = false;
	elem_maxwidth = height = 0;
	saved_scrollpos = scrollpos;
	size_t prevpos, pos;

	/* must not re-layout an already layouted scrollarea, as that would mangle
	 * multi-column text fields */
	if (layout_done)
		return;
	
	/* remove current scrollbar */
	if (scroll)
		delete scroll;

	for (winelem = first_child; winelem; winelem = winelem->sib_next) {
		winelem->w = get_menuitem_width(winelem, colwidths);
		winelem->h = vultures_text_height(V_FONT_MENU, winelem->caption);

		if (winelem->v_type == V_WINTYPE_OPTION || 
		    winelem->v_type == V_WINTYPE_TEXT) {
			if (!is_text)
				winelem->h += 10;

			winelem->x = 0;
			winelem->y = height;
			height += winelem->h + 4;
			/* this is just a guess at this point, as winelem->w may be wrong for multi-column */
			elem_maxwidth = (elem_maxwidth > winelem->w) ? elem_maxwidth : winelem->w;
		}
	}

	/* add up the widths of all the columns and determine the starting position of each column */
	for (i = 1; i < 8; i++) {
		colstart[i] = colstart[i-1] + colwidths[i-1];
		if (colstart[i] != colstart[i-1])
			colstart[i] += 16;
	}
	elem_maxwidth = (elem_maxwidth > (colstart[7] + colwidths[7])) ? 
					elem_maxwidth : (colstart[7] + colwidths[7]) + 5;

	w = elem_maxwidth;
	h = height;

	/* split up the captions into multiple labels: one for each column */
	for (winelem = first_child; winelem; winelem = winelem->sib_next) {
		winelem->w = elem_maxwidth;

		done = false;
		/* skip over elements without tab chars */
		pos = prevpos = winelem->caption.find_first_of('\t');
		if (pos == string::npos)
			continue;
		
		orig_caption = winelem->caption;
		winelem->caption = orig_caption.substr(0, pos);
		prevpos++;
		i = 1;
		do {
			pos = orig_caption.find_first_of('\t', prevpos);
			if (pos == string::npos)
				done = true;
			txt = orig_caption.substr(prevpos, pos - prevpos);
			trim(txt);
			
			coltxt = new textwin(this, txt);
			coltxt->w = 1;
			coltxt->h = winelem->h;
			coltxt->x = colstart[i];
			coltxt->y = winelem->y + 1;
			
			prevpos = ++pos;
			i++;
		} while (!done);
	}
	
	layout_done = true;
}


void scrollwin::set_height(int scrollheight)
{
	int inner_height = get_scrollheight();
	
	h = scrollheight;

	/* first remove an existing scrollbar - the window may have gotten larger */
	if (scroll)
		delete scroll;

	/* add the scrollbar */
	if (inner_height > scrollheight) {
		scroll = new scrollbar(this, scrollpos);
		scroll->w = vultures_winelem.scrollbar->w;
		scroll->h = h;
		scroll->x = w + 5;
		scroll->y = 0;

		this->w += scroll->w + 5;
	}
}


/* scroll to a position; depending on scrolltype this is either
 * a direction (+- 1 line), a page number, or a vertical pixel offset */
eventresult scrollwin::scrollto(int scrolltype, int scrolldir)
{
	window *child, *firstvisible;
	int height = 0;
	int scroll_top;

	scroll_top = 0;

	firstvisible = NULL;

	/* find the first visible child (menu item or text item),
	 * the maximum menu height and the menu's scrollbar */
	for (child = first_child; child; child = child->sib_next) {
		if (child->visible) {
			firstvisible = child;
			break;
		}
	}

	/* this function can be called by menus without a scrollbar if a
	 * scrolling hotkey is pressed or the mousewheel is used over the window */
	if (!scroll)
		return V_EVENT_HANDLED_NOREDRAW;

	height = get_scrollheight();


	switch (scrolltype)
	{
		case V_SCROLL_LINE_REL:
			if (scrolldir < 0) {
				if (firstvisible->sib_prev)
					scroll_top = firstvisible->sib_prev->y;
			} else {
				if (firstvisible->sib_next && firstvisible->sib_next != scroll &&
					(firstvisible->y < height - this->h))
					scroll_top = firstvisible->sib_next->y;
				else
					scroll_top = firstvisible->y;
			}
			break;

		case V_SCROLL_PAGE_REL:
			child = firstvisible;
			if (scrolldir < 0) {
				while (child && child->sib_prev &&
					child->y > (firstvisible->y - this->h))
					child = child->sib_prev;

				if (child->y < (firstvisible->y - this->h))
					scroll_top = child->sib_next->y;
			} else {
				while (child && child->sib_next && child != scroll &&
					child->y < (firstvisible->y + this->h) &&
					(child->y < height - this->h))
					child = child->sib_next;

				if (child->y > (firstvisible->y + this->h))
					scroll_top = child->sib_prev->y;
				else
					scroll_top = child->y;
			}
			break;

		case V_SCROLL_PAGE_ABS:
		case V_SCROLL_PIXEL_ABS:
			scroll_top = this->h * scrolldir;

			if (scrolltype == V_SCROLL_PIXEL_ABS)
				scroll_top = scrolldir;

			/* find the first menuitem */
			child = first_child;

			while (child->sib_next && child->y < scroll_top &&
				child != scroll && (child->y < height - this->h))
				child = child->sib_next;

			if (child->y > scroll_top)
				child = child->sib_prev;

			scroll_top = child->y;
			break;
	}

	scrollpos = (scroll_top * 8192.0 / (height - this->h));
	scroll->scrollpos = scrollpos;

	need_redraw = true;
	parent->need_redraw = true;

	return V_EVENT_HANDLED_REDRAW;
}


eventresult scrollwin::mousescroll(scrollbar *target, int is_drag)
{
	int scrollind_y;
	point mouse;
	int scrollarea_top = target->abs_y + vultures_winelem.scrollbutton_up->h;
	int scrollarea_bottom = target->abs_y + target->h - vultures_winelem.scrollbutton_down->h;

	scrollind_y = ((scrollarea_bottom - scrollarea_top -
					vultures_winelem.scroll_indicator->h) *
					(int)target->scrollpos) / 8192.0;

	mouse = vultures_get_mouse_pos();

	/* click on the scroll-up button */
	if (mouse.y <= scrollarea_top) {
		if (!is_drag)
			return scrollto(V_SCROLL_LINE_REL, -1);
	/* click on the scroll-down button */
	} else if (mouse.y >= scrollarea_bottom) {
		if (!is_drag)
			return scrollto(V_SCROLL_LINE_REL, 1);
	}

	/* click on the scrollbar above the indicator */
	else if (mouse.y <= scrollarea_top + scrollind_y) {
		if (!is_drag)
			return scrollto(V_SCROLL_PAGE_REL, -1);
	}

	/*click on the scrollbar below the indicator */
	else if (mouse.y >= scrollarea_top + vultures_winelem.scroll_indicator->h + scrollind_y) {
		if (!is_drag)
			return scrollto(V_SCROLL_PAGE_REL, 1);
	}

	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult scrollwin::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	window *winelem;
	point mouse, oldpos;

	vultures_set_mcursor(V_CURSOR_NORMAL);

	if (state == SDL_PRESSED && 
		(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))) {
		mouse = vultures_get_mouse_pos();
		oldpos.x = mouse.x + xrel;
		oldpos.y = mouse.y + yrel;
		winelem = get_window_from_point(oldpos);

		if (target->v_type == V_WINTYPE_SCROLLBAR && winelem == target)
			return mousescroll(static_cast<scrollbar*>(target), 1);
	}
	return V_EVENT_UNHANDLED;
}


eventresult scrollwin::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	/* wheel up/down: scroll by one line */
	if (button == SDL_BUTTON_WHEELUP)
		return scrollto(V_SCROLL_LINE_REL, -1);

	if (button == SDL_BUTTON_WHEELDOWN)
		return scrollto(V_SCROLL_LINE_REL, 1);

	if (target == this)
		/* clicks on the menu window itself are not interesting */
		return V_EVENT_HANDLED_NOREDRAW;

	/* a click on the scrollbar */
	else if (target->v_type == V_WINTYPE_SCROLLBAR)
		return mousescroll(static_cast<scrollbar*>(target), 0);
		
	return V_EVENT_UNHANDLED;
}
