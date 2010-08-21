/* NetHack may be freely redistributed.  See license for details. */

#include <ctype.h>
#include "SDL.h"

extern "C" {
#include "hack.h"
}

#include "vulture_win.h"
#include "vulture_sdl.h"
#include "vulture_mou.h"
#include "vulture_main.h"
#include "vulture_opt.h"

#include "winclass/button.h"
#include "winclass/enhancebutton.h"
#include "winclass/inputdialog.h"
#include "winclass/levelwin.h"

#define V_EVENTSTACK_SIZE 32


/******************************************************************************
* globals vars
******************************************************************************/

SDL_Rect *vulture_invrects = NULL;
int vulture_invrects_num = 0;
int vulture_invrects_max = 0;


/*********************************/

vulture_window_graphics vulture_winelem; /* contains borders, background, etc */

int vulture_windows_inited = 0;
int vulture_suppress_helpmsg;
int vulture_winid_map = 0;
int vulture_winid_minimap = 0;
int vulture_whatis_singleshot = 0;

vulture_event * vulture_eventstack = NULL;
int vulture_eventstack_top;


/******************************************************************************
* function pre-declarations
******************************************************************************/

static int vulture_event_dispatcher_core(SDL_Event * event, void * result, struct window * topwin);
static int vulture_handle_event(struct window * topwin, struct window * win,
								void * result, SDL_Event * event, int * redraw);


/******************************
* High-level window functions
******************************/
void vulture_messagebox(std::string message)
{
    mainwin *win;
    int dummy;

    win = new mainwin(ROOTWIN);
    win->set_caption(message);

	new button(win, "Continue", 1, '\0');

    win->layout();

    vulture_event_dispatcher(&dummy, V_RESPOND_INT, win);

	delete win;
}


int vulture_get_input(int force_x, int force_y, const char *ques, char *input)
{
	int response;
	inputdialog *win = new inputdialog(ROOTWIN, ques, 256, force_x, force_y);

	/* get input */
	vulture_event_dispatcher(&response, V_RESPOND_INT, win);

	/* copy result into input */
	win->copy_input(input);

	/* clean up */
	delete win;

	return response;
}



/******************************
* Event handling functions
******************************/

void vulture_event_dispatcher(void * result, int resulttype, window * topwin)
{
	vulture_event * queued_event;
	int event_result = V_EVENT_UNHANDLED;
	point mouse;
	SDL_Event event;
	int redraw;
	window *win;

	/* first, check whether we have an autoresponse queued */
	while ( (queued_event = vulture_eventstack_get()))
	{
		if (queued_event->rtype == resulttype || queued_event->rtype == V_RESPOND_ANY)
		{
			/* suppress some messages during automatic actions */
			vulture_suppress_helpmsg = 1;

			if (resulttype == V_RESPOND_POSKEY)
				*(vulture_event*)result = *queued_event;
			else if (resulttype == V_RESPOND_CHARACTER)
				*(char*)result = (char)queued_event->num;
			else
				*(int*)result = queued_event->num;
			return;
		}
	}

	/* this block will take us out of singleshot-whatis mode (triggered by the context menu) */
	if (vulture_whatis_singleshot && resulttype == V_RESPOND_POSKEY)
	{
		((vulture_event*)result)->num = ' ';
		vulture_whatis_singleshot = 0;
		return;
	}

	/* check whether we want to draw the "enhance" icon */
	if (enhancebtn)
		enhancebtn->check_enhance();

	/* nothing queued, do normal event processing */
	if (!topwin)
		topwin = ROOTWIN;

	if (!vulture_whatis_singleshot)
		/* no need to suppress messages now... */
		vulture_suppress_helpmsg = 0;

	/* kill the tooltip */
	vulture_mouse_invalidate_tooltip(1);

	/* fake a mousemotion event, to make the window set its preferred cursor before we draw */
	memset(&event, 0, sizeof(event));
	event.type = SDL_MOUSEMOTION;
	mouse = vulture_get_mouse_pos();
	win = topwin->get_window_from_point(mouse);
	vulture_handle_event(topwin, win, result, &event, &redraw);

	/* draw windows, if necessary */
	topwin->draw_windows();
	vulture_mouse_draw();
	vulture_refresh_window_region();
	vulture_mouse_refresh();
	vulture_mouse_restore_bg();

	while (event_result != V_EVENT_HANDLED_FINAL)
	{
		/* Get next event OR wait 100ms */
		vulture_wait_event(&event, 100);

		event_result = vulture_event_dispatcher_core(&event, result, topwin);

		SDL_Delay(20);
	}
}



int vulture_event_dispatcher_nonblocking(void * result, window * topwin)
{
	SDL_Event event;
	int event_result = V_EVENT_UNHANDLED;

	if (!topwin)
		topwin = ROOTWIN;

	/* kill the tooltip */
	vulture_mouse_invalidate_tooltip(1);

	/* draw windows, if necessary */
	topwin->draw_windows();
	vulture_mouse_draw();
	vulture_refresh_window_region();
	vulture_mouse_refresh();
	vulture_mouse_restore_bg();

	while (event_result != V_EVENT_HANDLED_FINAL)
	{
		if (!vulture_poll_event(&event))
			return 0;

		event_result = vulture_event_dispatcher_core(&event, result, topwin);
	}

	return 1;
}


static int vulture_event_dispatcher_core(SDL_Event * event, void * result, window * topwin)
{
	window *win, *win_old;
	int event_result = V_EVENT_UNHANDLED;
	int redraw = 0, hovertime_prev = 0;
	static int hovertime = 0;
	point mouse, mouse_old = vulture_get_mouse_prev_pos();

	if (event->type == SDL_TIMEREVENT) {
		hovertime_prev = hovertime;
		hovertime += 150; /* 100 ms event timeout + 20 ms delay after the last event */
		event->user.code = hovertime;
	} else {
		hovertime = 0;
		event->user.code = 0;
	}

	/* keyboard events are always given to topwin, because mouse cursor
		* position has nothing to do with keyboard input */
	if (event->type == SDL_KEYDOWN) {
		event_result = topwin->event_handler(topwin, result, event);
		if (event_result == V_EVENT_HANDLED_REDRAW || event_result == V_EVENT_UNHANDLED_REDRAW)
			redraw = 1;
	}
	else {
		/* find out what window the mouse is over now */
		mouse = vulture_get_mouse_pos();
		win = topwin->get_window_from_point(mouse);

		/* All other events belong to the window under the mouse cursor */
		if (event->type == SDL_MOUSEMOTION) {
			/* delete tooltip; if the mouse gets moved somewhere interesting
				* the window it is over will set up a new tooltip */
			vulture_mouse_invalidate_tooltip(0);

			/* notify the window the mouse got moved out of */
			win_old = topwin->get_window_from_point(mouse_old);
			if (win_old && win != win_old && win_old != win->parent) {
				event->type = SDL_MOUSEMOVEOUT;
				event_result = vulture_handle_event(topwin, win_old, result, event, &redraw);
				event->type = SDL_MOUSEMOTION;
			}
		}

		/* the mouse might be outside the window of interest (topwin).
			* if so win is NULL and we can go wait for the next event now... */
		if (!win)
			return V_EVENT_UNHANDLED;

		event_result = vulture_handle_event(topwin, win, result, event, &redraw);
	}

	if (redraw)
		topwin->draw_windows();

	if (redraw || event->type != SDL_TIMEREVENT ||
		(hovertime > HOVERTIMEOUT && hovertime_prev < HOVERTIMEOUT))
		vulture_mouse_draw();

	/* refresh all regions (except mouse & tt) needing a refresh here,
		* do NOT do so in draw() functions */
	if (redraw || vulture_invrects_num)
		vulture_refresh_window_region();

	if (redraw || event->type != SDL_TIMEREVENT ||
		(hovertime > HOVERTIMEOUT && hovertime_prev < HOVERTIMEOUT))
	{
		vulture_mouse_refresh();
		vulture_mouse_restore_bg();
	}

	return event_result;
}


/* takes an event and passes it each window in the win->parent->...->topwin
* chain until one of the windows handles the event or until the event is
* rejected by topwin */
static int vulture_handle_event(window *topwin, window *win,
								void *result, SDL_Event *event, int *redraw)
{
	int event_result = V_EVENT_UNHANDLED;
	struct window * winptr = win;

	while (event_result < V_EVENT_HANDLED_NOREDRAW)
	{
		event_result = winptr->event_handler(win, result, event);
		if (event_result == V_EVENT_HANDLED_REDRAW ||
			event_result == V_EVENT_UNHANDLED_REDRAW)
			*redraw = 1;

		if (winptr == topwin)
			break;

		/* try this window's parent next */
		winptr = winptr->parent;
	}

	return event_result;
}



/* push an event onto the eventstack */
void vulture_eventstack_add(int num, int x, int y, int rtype)
{
	if (!vulture_eventstack)
	{
		vulture_eventstack = (vulture_event *)malloc(V_EVENTSTACK_SIZE * sizeof(vulture_event));
		vulture_eventstack_top = -1;
	}

	vulture_eventstack_top++;
	if (vulture_eventstack_top >= V_EVENTSTACK_SIZE)
	{
		printf("WARNING: eventstack full.\n");
		vulture_eventstack_top = V_EVENTSTACK_SIZE - 1;
		return;
	}

	vulture_eventstack[vulture_eventstack_top].num = num;
	vulture_eventstack[vulture_eventstack_top].x = x;
	vulture_eventstack[vulture_eventstack_top].y = y; 
	vulture_eventstack[vulture_eventstack_top].rtype = rtype;
}


/* pop an event off the eventstack */
vulture_event * vulture_eventstack_get(void)
{
	if (!vulture_eventstack)
		return NULL;

	/* stack empty? */
	if (vulture_eventstack_top >= V_EVENTSTACK_SIZE || vulture_eventstack_top < 0)
		return NULL;

	vulture_eventstack_top--;

	return &vulture_eventstack[vulture_eventstack_top+1];
}


void vulture_eventstack_destroy(void)
{
	if (vulture_eventstack)
		free(vulture_eventstack);
}


void vulture_invalidate_region(int x , int y, int w, int h)
{
	int i;

	/* look at known invalid rects */
	for (i = 0; i < vulture_invrects_num; i++)
	{
		if (x > vulture_invrects[i].x &&
			y > vulture_invrects[i].y &&
			x + w < vulture_invrects[i].x + vulture_invrects[i].w &&
			y + h < vulture_invrects[i].y + vulture_invrects[i].h)
			/* new invalid region is fully inside an already known one */
			return;

		else if (x < vulture_invrects[i].x &&
				y < vulture_invrects[i].y &&
				x + w > vulture_invrects[i].x + vulture_invrects[i].w &&
				y + h > vulture_invrects[i].y + vulture_invrects[i].h)
				/* old invalid region is fully inside new one;
				* stop searching here and reuse this entry in the array */
			break;
	}

	if (i >= vulture_invrects_max)
	{
		vulture_invrects = (SDL_Rect *)realloc(vulture_invrects, (vulture_invrects_max + 16) * sizeof(SDL_Rect));
		vulture_invrects_max += 16;
	}

	if (i == vulture_invrects_num)
		vulture_invrects_num++;

	vulture_invrects[i].x = x;
	vulture_invrects[i].y = y;
	vulture_invrects[i].w = w;
	vulture_invrects[i].h = h;
}


/* refresh invalid regions */
void vulture_refresh_window_region(void)
{
	int x1 = 9999, y1 = 9999, x2 = 0, y2 = 0;
	int i;

	/* find the bounding rectangla around all invalid rects */
	for (i = 0; i < vulture_invrects_num; i++)
	{
		x1 = (x1 < vulture_invrects[i].x) ? x1 : vulture_invrects[i].x;
		y1 = (y1 < vulture_invrects[i].y) ? y1 : vulture_invrects[i].y;

		x2 = (x2 > (vulture_invrects[i].x + vulture_invrects[i].w)) ?
			x2 : (vulture_invrects[i].x + vulture_invrects[i].w);
		y2 = (y2 > (vulture_invrects[i].y + vulture_invrects[i].h)) ?
			y2 : (vulture_invrects[i].y + vulture_invrects[i].h);
	}

	/* refresh the bounding rect */
	if (x1 < x2 && y1 < y2)
		vulture_refresh_region(x1, y1, x2, y2);

	/* there are now 0 invalid rects */
	if (vulture_invrects)
		free(vulture_invrects);

	vulture_invrects = NULL;
	vulture_invrects_num = 0;
	vulture_invrects_max = 0;
}


/* resize the vulture application window to the given width and height */
void vulture_win_resize(int width, int height)
{
	struct window *current, *topwin;
	SDL_Event event;
	vulture_event dummy;
	bool descend = true;

  if ( !ROOTWIN ) return;

	current = topwin = ROOTWIN;

	event.type = SDL_VIDEORESIZE;
	event.resize.w = width;
	event.resize.h = height;

	current->event_handler(current, &dummy, &event);

	do {
		current = current->walk_winlist(&descend);

		current->event_handler(current, &dummy, &event);

		/* recalc absolute position */
		if (current->v_type != V_WINTYPE_NONE && current->parent) {
			current->abs_x = current->parent->abs_x + current->x;
			current->abs_y = current->parent->abs_y + current->y;
		}
	}
	while (current != topwin);

	/* redraw everything */
	levwin->force_redraw();

	topwin->draw_windows();
	vulture_refresh_window_region();
}


#ifdef VULTURE_NETHACK
/* show a main menu with common options when the user presses esc */
void vulture_show_mainmenu()
{
	int winid, n;
	anything any;
	menu_item *selected;

	winid = vulture_create_nhwindow(NHW_MENU);
	vulture_start_menu(winid);
	
	any.a_int = 1;
	vulture_add_menu(winid, NO_GLYPH, &any, 'h', 0, ATR_BOLD,
					"Help", MENU_UNSELECTED);
	any.a_int = 2;
	vulture_add_menu(winid, NO_GLYPH, &any, 'O', 0, ATR_BOLD,
					"Options", MENU_UNSELECTED);
	any.a_int = 3;
	vulture_add_menu(winid, NO_GLYPH, &any, 'I', 0, ATR_BOLD,
					"Interface options", MENU_UNSELECTED);
	any.a_int = 4;
	vulture_add_menu(winid, NO_GLYPH, &any, 'S', 0, ATR_BOLD,
					"Save & Quit", MENU_UNSELECTED);
	any.a_int = 5;
	vulture_add_menu(winid, NO_GLYPH, &any, 'Q', 0, ATR_BOLD,
					"Quit", MENU_UNSELECTED);

	vulture_end_menu(winid, "Main menu");
	n = vulture_select_menu(winid, PICK_ONE, &selected);
	vulture_destroy_nhwindow(winid);

	if (n < 1)
		return;

	switch(selected[0].item.a_int)
	{
		case 1: dohelp(); break;
		case 2: doset(); break;
		case 3: vulture_iface_opts(); break;
		case 4: dosave(); break;
		case 5: done2(); break;
	}
}
#endif
