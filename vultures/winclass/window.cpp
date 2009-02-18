/* NetHack may be freely redistributed.  See license for details. */

#include "window.h"

#include "vultures_gra.h"
#include "vultures_win.h"
#include "vultures_sdl.h"


window *ROOTWIN = NULL;

window::window(window *p) : parent(p)
{
	if (!parent && ROOTWIN)
		parent = ROOTWIN;

	first_child = last_child = NULL;
	sib_next = sib_prev = NULL;
	abs_x = abs_y = x = y = w = h = 0;
	caption = "";
	accelerator = '\0';
	background = NULL;
	autobg = false;
	menu_id_v = NULL;
	
	need_redraw = true;
	visible = true;
	
	if (parent != NULL)
	{
		/* add win to the parent's ll of children */
		sib_prev = parent->last_child;
		if (parent->first_child)
			parent->last_child->sib_next = this;
		else
			parent->first_child = this;
		parent->last_child = this;
	} else
		ROOTWIN = this;
	
	v_type = V_WINTYPE_CUSTOM;
}


window::~window()
{
	/* the root window has no parent */
	if (parent) {
		/* unlink the window everywhere */
		if (parent->first_child == this)
			parent->first_child = sib_next;

		if (parent->last_child == this)
			parent->last_child = sib_prev;
	}

	/* remove from the linked list of siblings */
	if (sib_prev)
		sib_prev->sib_next = sib_next;

	if (sib_next)
		sib_next->sib_prev = sib_prev;


	/* destroy it's children. note that the child's destroy function will manipulate ->first_child */
	while (first_child) {
		if (!visible)
			first_child->visible = 0;
		delete first_child; // deleting a child will unlink it; eventually first_child will be NULL
	}

	/* we may want to restore the background before deleting it */
	if (visible && background != NULL && autobg && (!parent || parent->visible)) {
		vultures_put_img(abs_x, abs_y, background);
		vultures_refresh_region(abs_x, abs_y,abs_x + w, abs_y + h);
	}

	/* make sure the background gets freed even if it doesn't get restored */
	if (background)
		SDL_FreeSurface(background);
	
	if (ROOTWIN == this)
		ROOTWIN = NULL;
}


void window::set_caption(std::string str)
{
	caption = str;
}


void window::hide()
{
	if (background && autobg)
	{
		vultures_put_img(abs_x, abs_y, background);
		vultures_refresh_region(abs_x, abs_y, abs_x + w, abs_y + h);
		SDL_FreeSurface(background);
		background = NULL;
	}

	visible = 0;
	need_redraw = 0;
}


/* windows which have autobg set store an image of the screen behind them, so
* that the screen can easily be restored when the window is hidden or destroyed.
* if a window behind the current window is updated, the current window must
* refresh its stored background */
void window::update_background(void)
{
	int i;
	int x1, y1, x2, y2;
	SDL_Rect src, dst;

	/* no stored background and autobg is off: do nothing */
	if (!background && !autobg)
		return;

	/* no background stored yet: copy a surface that is as large as the window */
	if (!background) {
		background = vultures_get_img(abs_x, abs_y, (abs_x + w - 1), (abs_y + h - 1));
		return;
	}

	/* find the intersection between all invalid regions and the window so that
	* only actually invalid parts of the background get updated */
	for (i = 0; i < vultures_invrects_num; i++)
	{
		if (abs_x > (vultures_invrects[i].x + vultures_invrects[i].w) ||
			abs_y > (vultures_invrects[i].y + vultures_invrects[i].h) ||
			(abs_x + w) < vultures_invrects[i].x ||
			(abs_y + h) < vultures_invrects[i].y)
			continue;

		x1 = (abs_x > vultures_invrects[i].x) ? abs_x : vultures_invrects[i].x;
		y1 = (abs_y > vultures_invrects[i].y) ? abs_y : vultures_invrects[i].y;
		x2 = (abs_x + w > vultures_invrects[i].x + vultures_invrects[i].w) ?
				vultures_invrects[i].x + vultures_invrects[i].w : abs_x + w;
		y2 = (abs_y + h > vultures_invrects[i].y + vultures_invrects[i].h) ?
				vultures_invrects[i].y + vultures_invrects[i].h : abs_y + h;

		if (x1 < x2 && y1 < y2)
		{
			src.x = x1;
			src.y = y1;
			src.w = x2 - x1;
			src.h = y2 - y1;

			dst.x = x1 - abs_x;
			dst.y = y1 - abs_y;
			dst.w = src.w;
			dst.h = src.h;

			SDL_BlitSurface(vultures_screen, &src, background, &dst);
		}
	}
}


/* find the window that has the accelerator "accel" */
window* window::find_accel(char accel)
{
    struct window *child;

    for (child = first_child; child; child = child->sib_next)
        if (child->accelerator == accel)
            return child;

    return NULL;
}


/****************************
* window drawing functions
****************************/
/* walks the list of windows and draws all of them, depending on their type and status */
void window::draw_windows()
{
	bool descend = true;
	bool invalid = false;
	window *current = this;

	update_background();

	if (need_redraw && !draw()) {
		/* draw() == 0 means that the window took care of
		 * redrawing it's children itself */
		need_redraw = false;
		return;
	}

	need_redraw = false;

	do {
		current = current->walk_winlist(&descend);

		if (!current->visible) {
			descend = false;
			continue;
		}

		/* recalc absolute position */
		if (current->parent) {
			current->abs_x = current->parent->abs_x + current->x;
			current->abs_y = current->parent->abs_y + current->y;
		}

		/* if the window intersects an invalid region, some window "underneath" it
		 * painted over part of it; we need to refresh the saved background and redraw */
		invalid = current->intersects_invalid();

		if (current->need_redraw || (invalid && descend)) {
			current->update_background();

			/* setting descend = 0 will prevent the next call to vultures_walk_winlist
			 * from descending to a child window. The window's draw() function can choose
			 * to let us redraw it's children from here by returning 1 */
			descend = current->draw();

			current->need_redraw = 0;
		}
	/* vultures_walk_winlist will eventually arive back at the top window */
	} while (current != this);
}


window *window::walk_winlist(bool *descend)
{
	/* 1) try to descend to child */
	if (*descend && first_child)
		return first_child;

	/* 2) try sibling*/
	if (sib_next) {
		*descend = true;
		return sib_next;
	}

	/* 3) ascend to parent and set *descend = false to prevent infinite loops */
	*descend = false;
	return parent;
}


/* find the window under the mouse, starting from here */
window *window::get_window_from_point(point mouse)
{
	window *winptr, *nextwin;

	winptr = this;

	/* as each child window is completely contained by its parent we can descend
	* into every child window that is under the cursor until no further descent
	* is possible. The child lists are traversed in reverse because newer child
	* windows are considered to be on top if child windows overlap */
	while (winptr->last_child) {
		nextwin = winptr->last_child;

		while (nextwin && (nextwin->abs_x > mouse.x || nextwin->abs_y > mouse.y ||
						(nextwin->abs_x + nextwin->w) < mouse.x ||
						(nextwin->abs_y + nextwin->h) < mouse.y || !nextwin->visible))
			nextwin = nextwin->sib_prev;

		if (nextwin)
			winptr = nextwin;
		else
			return winptr;
	}
	return winptr;
}


/* determine whether a window intersects an invalid region */
bool window::intersects_invalid()
{
	int i;
	int x1, y1, x2, y2;

	for (i = 0; i < vultures_invrects_num; i++) {
		/* check intersection with each invalid rect */
		if (abs_x > (vultures_invrects[i].x + vultures_invrects[i].w) ||
			abs_y > (vultures_invrects[i].y + vultures_invrects[i].h) ||
			(abs_x + w) < vultures_invrects[i].x ||
			(abs_y + h) < vultures_invrects[i].y)
			continue;

		x1 = (abs_x > vultures_invrects[i].x) ? abs_x : vultures_invrects[i].x;
		y1 = (abs_y > vultures_invrects[i].y) ? abs_y : vultures_invrects[i].y;
		x2 = (abs_x + w > vultures_invrects[i].x + vultures_invrects[i].w) ?
				vultures_invrects[i].x + vultures_invrects[i].w : abs_x + w;
		y2 = (abs_y + h > vultures_invrects[i].y + vultures_invrects[i].h) ?
				vultures_invrects[i].y + vultures_invrects[i].h : abs_y + h;

		if (x1 < x2 && y1 < y2)
			return true;
	}

	return false;
}


eventresult window::event_handler(window *target, void *result, SDL_Event *event)
{
	switch (event->type)
	{
		case SDL_TIMEREVENT:
			return handle_timer_event(target, result, event->user.code);

		case SDL_MOUSEMOTION:
			return handle_mousemotion_event(target, result, event->motion.xrel,
			                           event->motion.yrel, event->motion.state);

		case SDL_MOUSEBUTTONUP:
			return handle_mousebuttonup_event(target, result, event->button.x,
			        event->button.y, event->button.button, event->button.state);

		case SDL_KEYDOWN:
			return handle_keydown_event(target, result, event->key.keysym.sym,
			                  event->key.keysym.mod, event->key.keysym.unicode);

		case SDL_VIDEORESIZE:
			return handle_resize_event(target, result, event->resize.w, event->resize.h);
		
		default:
			return handle_other_event(target, result, event);
	}

	return V_EVENT_UNHANDLED;
}


/* default event handling functions which do nothing */
eventresult window::handle_timer_event(window* target, void* result, int time)
{
	return V_EVENT_UNHANDLED;
}


eventresult window::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	return V_EVENT_UNHANDLED;
}


eventresult window::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	return V_EVENT_UNHANDLED;
}


eventresult window::handle_keydown_event(window* target, void* result, int sym, int mod, int unicode)
{
	return V_EVENT_UNHANDLED;
}


eventresult window::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	return V_EVENT_UNHANDLED;
}


eventresult window::handle_other_event(window* target, void* result, SDL_Event *event)
{
	return V_EVENT_UNHANDLED;
}

