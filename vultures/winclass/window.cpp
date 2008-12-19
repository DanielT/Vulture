#include <cstring>

#include "window.h"

#include "vultures_gra.h"
#include "vultures_win.h"
#include "vultures_sdl.h"


window **vultures_windows = NULL;
int windowcount = 0;
int windowcount_max = 0;

window* vultures_get_window(int winid)
{
	return vultures_windows[winid];
}


window::window()
{
}

window::window(window *p) : parent(p)
{
	id = 0;
	
	if (windowcount != 0 && !parent)
		printf("Treason uncloaked! New window is not parented to rootwin\n");
	
	/* if necessary make space in the vultures_windows array */
	if (windowcount == windowcount_max)
	{
		vultures_windows = (window**)realloc(vultures_windows, (windowcount_max + 16) * sizeof(window*));
		memset(&vultures_windows[windowcount_max], 0, 16 * sizeof(window*));
		windowcount_max += 16;

		/* no need to search through the first windowcount_cur ids, they're definitely taken */
		id = windowcount;
	}
	else
		while (vultures_windows[id] != NULL && id < windowcount_max)
			id++;
	
	windowcount++;
	vultures_windows[id] = this;
	
	first_child = last_child = NULL;
	sib_next = sib_prev = NULL;
	abs_x = abs_y = x = y = w = h = 0;
	caption = NULL;
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
	}
	
	nh_type = 0;
	v_type = V_WINTYPE_CUSTOM;
}


window::~window()
{
	/* id == -1 if the window is being replaced */
	if (id > -1) {
		vultures_windows[id] = NULL;
		windowcount--;
	}
	
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
	
	/* clean up after the last window is gone */
	if (windowcount == 0) {
		free(vultures_windows);
		vultures_windows = NULL;
		windowcount_max = 0;
	}

	/* free up alloced resources */
	if (caption)
		free(caption);

	/* we may want to restore the background before deleting it */
	if (visible && background != NULL && autobg) {
		vultures_put_img(abs_x, abs_y, background);
		vultures_refresh_region(abs_x, abs_y,abs_x + w, abs_y + h);
	}

	/* make sure the background gets freed even if it doesn't get restored */
	if (background)
		SDL_FreeSurface(background);
}


window* window::replace_win(window *win)
{
	*this = *win;
	
	vultures_windows[id] = this;

	if (parent) {
		if (parent->first_child == win)
			parent->first_child = this;

		if (parent->last_child == win)
			parent->last_child = this;
	}

	if (sib_prev)
		sib_prev->sib_next = this;

	if (sib_next)
		sib_next->sib_prev = this;
		
	win->first_child = win->last_child = win->parent = NULL;
	win->caption = NULL;
	win->background = NULL;
	win->id = -1;
	
	delete win;
	
	return this;
}


void window::set_caption(const char *str)
{
	if (caption)
		free(caption);
	
	caption = strdup(str);
}


void window::hide()
{
	if (background && autobg)
	{
		vultures_put_img(abs_x, abs_y, background);
// 		vultures_invalidate_region(abs_x, abs_y, w, h);
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

