/* Copyright (c) Daniel Thaler, 2006				  */
/* NetHack may be freely redistributed.  See license for details. */


#include <ctype.h>

#include "SDL.h"

extern "C" {
#include "hack.h"
}

#include "vultures_win.h"
#include "vultures_win_event.h"
#include "vultures_sdl.h"
#include "vultures_map.h"
#include "vultures_mou.h"
#include "vultures_txt.h"
#include "vultures_main.h"
#include "vultures_tile.h"
#include "vultures_opt.h"
#include "vultures_gfl.h"


#include "window_types.h"

#define V_EVENTSTACK_SIZE 32


#define ROOTWIN vultures_get_window(0)


/******************************************************************************
* globals vars
******************************************************************************/


// static struct window ** vultures_windows = NULL;
// static int windowcount_cur = 0;
// static int windowcount_max = 0;

SDL_Rect *vultures_invrects = NULL;
int vultures_invrects_num = 0;
int vultures_invrects_max = 0;


/*********************************/

Uint32 vultures_message_colors[V_MAX_MESSAGE_COLORS];
Uint32 vultures_warn_colors[V_MAX_WARN];

vultures_window_graphics vultures_winelem; /* contains borders, background, etc */

int vultures_windows_inited = 0;
int vultures_suppress_helpmsg;
int vultures_winid_map = 0;
int vultures_winid_minimap = 0;
SDL_Surface * vultures_statusbar = NULL;
int vultures_whatis_singleshot = 0;


vultures_event * vultures_eventstack = NULL;
int vultures_eventstack_top;


/******************************************************************************
* function pre-declarations
******************************************************************************/

struct window * vultures_walk_winlist(struct window *win, bool *descend);

int vultures_intersects_invalid(struct window * win);

static int vultures_event_dispatcher_core(SDL_Event * event, void * result, struct window * topwin);
static int vultures_handle_event(struct window * topwin, struct window * win,
								void * result, SDL_Event * event, int * redraw);


/******************************
* High-level window functions
******************************/
void vultures_messagebox(const char *message)
{
    mainwin *win;
    int dummy;

    win = new mainwin(vultures_get_window(0));
    win->set_caption(message);

	new button(win, "Continue", 1, '\0');

    win->layout();

    win->abs_x = win->parent->x + win->x;
    win->abs_y = win->parent->y + win->y;

    vultures_event_dispatcher(&dummy, V_RESPOND_INT, win);

	delete win;
}


int vultures_get_input(int force_x, int force_y, const char *ques, char *input)
{
	int response;
	inputdialog *win = new inputdialog(vultures_get_window(0), ques, 256);

	if (force_x > -1)
		win->x = force_x;
	else
		win->x = (win->parent->w - win->w) / 2;

	if (force_y > -1)
		win->y = force_y;
	else
		win->y = (win->parent->h - win->h) / 2;

	win->abs_x = win->parent->x + win->x;
	win->abs_y = win->parent->y + win->y;

	/* get input */
	vultures_event_dispatcher(&response, V_RESPOND_INT, win);

	/* copy result into input */
	win->copy_input(input);

	/* clean up */
	delete win;

	return response;
}



/******************************
* Event handling functions
******************************/

void vultures_event_dispatcher(void * result, int resulttype, struct window * topwin)
{
	vultures_event * queued_event;
	int event_result = V_EVENT_UNHANDLED;
	point mouse;
	SDL_Event event;
	int redraw;
	struct window *win;

	/* first, check whether we have an autoresponse queued */
	while ( (queued_event = vultures_eventstack_get()))
	{
		if (queued_event->rtype == resulttype || queued_event->rtype == V_RESPOND_ANY)
		{
			/* suppress some messages during automatic actions */
			vultures_suppress_helpmsg = 1;

			if (resulttype == V_RESPOND_POSKEY)
				*(vultures_event*)result = *queued_event;
			else if (resulttype == V_RESPOND_CHARACTER)
				*(char*)result = (char)queued_event->num;
			else
				*(int*)result = queued_event->num;
			return;
		}
	}

	/* this block will take us out of singleshot-whatis mode (triggered by the context menu) */
	if (vultures_whatis_singleshot && resulttype == V_RESPOND_POSKEY)
	{
		((vultures_event*)result)->num = ' ';
		vultures_whatis_singleshot = 0;
		return;
	}

	/* check whether we want to draw the "enhance" icon */
	if (enhancebtn)
		enhancebtn->check_enhance();

	/* nothing queued, do normal event processing */
	if (!topwin)
		topwin = ROOTWIN;

	if (!vultures_whatis_singleshot)
		/* no need to suppress messages now... */
		vultures_suppress_helpmsg = 0;

	/* kill the tooltip */
	vultures_mouse_invalidate_tooltip(1);

	/* fake a mousemotion event, to make the window set its preferred cursor before we draw */
	memset(&event, 0, sizeof(event));
	event.type = SDL_MOUSEMOTION;
	mouse = vultures_get_mouse_pos();
	win = vultures_get_window_from_point(topwin, mouse);
	vultures_handle_event(topwin, win, result, &event, &redraw);

	/* draw windows, if necessary */
	vultures_draw_windows(topwin);
	vultures_mouse_draw();
	vultures_refresh_window_region();
	vultures_mouse_refresh();
	vultures_mouse_restore_bg();

	while (event_result != V_EVENT_HANDLED_FINAL)
	{
		/* Get next event OR wait 100ms */
		vultures_wait_event(&event, 100);

		event_result = vultures_event_dispatcher_core(&event, result, topwin);

		SDL_Delay(20);
	}
}



int vultures_event_dispatcher_nonblocking(void * result, struct window * topwin)
{
	SDL_Event event;
	int event_result = V_EVENT_UNHANDLED;

	if (!topwin)
		topwin = ROOTWIN;

	/* kill the tooltip */
	vultures_mouse_invalidate_tooltip(1);

	/* draw windows, if necessary */
	vultures_draw_windows(topwin);
	vultures_mouse_draw();
	vultures_refresh_window_region();
	vultures_mouse_refresh();
	vultures_mouse_restore_bg();

	while (event_result != V_EVENT_HANDLED_FINAL)
	{
		if (!vultures_poll_event(&event))
			return 0;

		event_result = vultures_event_dispatcher_core(&event, result, topwin);
	}

	return 1;
}


static int vultures_event_dispatcher_core(SDL_Event * event, void * result, struct window * topwin)
{
	struct window *win, *win_old;
	int event_result = V_EVENT_UNHANDLED;
	int redraw = 0, hovertime_prev = 0;
	static int hovertime = 0;
	point mouse, mouse_old = vultures_get_mouse_prev_pos();

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
		mouse = vultures_get_mouse_pos();
		win = vultures_get_window_from_point(topwin, mouse);

		/* All other events belong to the window under the mouse cursor */
		if (event->type == SDL_MOUSEMOTION) {
			/* delete tooltip; if the mouse gets moved somewhere interesting
				* the window it is over will set up a new tooltip */
			vultures_mouse_invalidate_tooltip(0);

			/* notify the window the mouse got moved out of */
			win_old = vultures_get_window_from_point(topwin, mouse_old);
			if (win_old && win != win_old && win_old != win->parent) {
				event->type = SDL_MOUSEMOVEOUT;
				event_result = vultures_handle_event(topwin, win_old, result, event, &redraw);
				event->type = SDL_MOUSEMOTION;
			}
		}

		/* the mouse might be outside the window of interest (topwin).
			* if so win is NULL and we can go wait for the next event now... */
		if (!win)
			return V_EVENT_UNHANDLED;

		event_result = vultures_handle_event(topwin, win, result, event, &redraw);
	}

	if (redraw)
		vultures_draw_windows(topwin);

	if (redraw || event->type != SDL_TIMEREVENT ||
		(hovertime > HOVERTIMEOUT && hovertime_prev < HOVERTIMEOUT))
		vultures_mouse_draw();

	/* refresh all regions (except mouse & tt) needing a refresh here,
		* do NOT do so in draw() functions */
	if (redraw || vultures_invrects_num)
		vultures_refresh_window_region();

	if (redraw || event->type != SDL_TIMEREVENT ||
		(hovertime > HOVERTIMEOUT && hovertime_prev < HOVERTIMEOUT))
	{
		vultures_mouse_refresh();
		vultures_mouse_restore_bg();
	}

	return event_result;
}


/* takes an event and passes it each window in the win->parent->...->topwin
* chain until one of the windows handles the event or until the event is
* rejected by topwin */
static int vultures_handle_event(struct window *topwin, struct window *win,
								void * result, SDL_Event *event, int *redraw)
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
void vultures_eventstack_add(int num, int x, int y, int rtype)
{
	if (!vultures_eventstack)
	{
		vultures_eventstack = (vultures_event *)malloc(V_EVENTSTACK_SIZE * sizeof(vultures_event));
		vultures_eventstack_top = -1;
	}

	vultures_eventstack_top++;
	if (vultures_eventstack_top >= V_EVENTSTACK_SIZE)
	{
		printf("WARNING: eventstack full.\n");
		vultures_eventstack_top = V_EVENTSTACK_SIZE - 1;
		return;
	}

	vultures_eventstack[vultures_eventstack_top].num = num;
	vultures_eventstack[vultures_eventstack_top].x = x;
	vultures_eventstack[vultures_eventstack_top].y = y; 
	vultures_eventstack[vultures_eventstack_top].rtype = rtype;
}


/* pop an event off the eventstack */
vultures_event * vultures_eventstack_get(void)
{
	if (!vultures_eventstack)
		return NULL;

	/* stack empty? */
	if (vultures_eventstack_top >= V_EVENTSTACK_SIZE || vultures_eventstack_top < 0)
		return NULL;

	vultures_eventstack_top--;

	return &vultures_eventstack[vultures_eventstack_top+1];
}


void vultures_eventstack_destroy(void)
{
	if (vultures_eventstack)
		free(vultures_eventstack);
}



/****************************
* window drawing functions 
****************************/
/* walks the list of windows and draws all of them, depending on their type and status */
void vultures_draw_windows(window *topwin)
{
	bool descend = true;
	bool invalid = false;
	window *current = topwin;

	current->update_background();

	if (topwin->need_redraw && !topwin->draw()) {
		/* topwin->draw() == 0 means that the window took care of
		 * redrawing it's children itself */
		topwin->need_redraw = false;
		return;
	}

	topwin->need_redraw = 0;

	do {
		current = vultures_walk_winlist(current, &descend);

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
		invalid = vultures_intersects_invalid(current);

		if (current->need_redraw || (invalid && descend)) {
			current->update_background();

			/* setting descend = 0 will prevent the next call to vultures_walk_winlist
			 * from descending to a child window. The window's draw() function can choose
			 * to let us redraw it's children from here by returning 1 */
			descend = current->draw();

			current->need_redraw = 0;
		}
	/* vultures_walk_winlist will eventually arive back at the top window */
	} while (current != topwin);
}

#if 0
/* 2) specialized window drawing funtions that apply to a single window */


static int vultures_draw_objitem(struct window * win)
{
	char tmpstr[32];
	int text_start_x, text_start_y, txt_height;
	int tile_x, tile_y;
	int x = win->abs_x;
	int y = win->abs_y;
	int w = win->w;
	int h = win->h;
	int tile = 0;
	int weight = 0;
	Uint32 textcolor;

	if (win->pd.obj)
	{
		tile = vultures_object_to_tile(win->pd.obj->otyp, -1, -1, win->pd.obj);
		weight = win->pd.obj->owt;

		tile_x = x + h/2;
		tile_y = y + h * 3 / 4;

		if (TILE_IS_OBJECT(tile))
		{
			tile = tile - OBJTILEOFFSET + ICOTILEOFFSET;
			tile_x = x + 2;
			tile_y = y + 2;
		}
	}

	vultures_set_draw_region(x, y, x + w - 1, y + h - 1);

	/* re-set the background to prevent shadings from stacking repatedly until they become solid */
	if (win->background)
		vultures_put_img(x, y, win->background);


	/* hovering gives an item a light blue frame */
	if (win->hover)
		vultures_rect(x+1, y+1, x+w-2, y+h-2, CLR32_BLESS_BLUE);

	/* otherwise, if it is selected, the item has an orange frame */
	else if (win->selected)
		vultures_rect(x+1, y+1, x+w-2, y+h-2, CLR32_ORANGE);

	/* all other items appear etched */
	else
	{
		/* draw the outer edge of the frame */
		vultures_draw_lowered_frame(x, y, x+w-1, y+h-1);
		/* Inner edge */
		vultures_draw_raised_frame(x+1, y+1, x+w-2, y+h-2);
	}

	/* the item that was toggled last has a white outer frame to indicate it's special status */
	if (win->last_toggled)
		vultures_rect(x, y, x+w-1, y+h-1, CLR32_WHITE);


	/* selected items also have yellow background shading */
	if (win->selected)
		vultures_fill_rect(x+h-1, y+2, x+w-3, y+h-3, CLR32_GOLD_SHADE);


	/* use a different text color for worn objects */
	if (win->pd.obj && win->pd.obj->owornmask)
		textcolor = CLR32_LIGHTGREEN;
	else
		textcolor = CLR32_WHITE;


	/* draw text, leaving a h by h square on the left free for the object tile */
	/* line 1 and if necessary line 2 contain the item description */
	vultures_put_text_multiline(V_FONT_MENU, win->caption, vultures_screen, x + h,
								y + 3, textcolor, CLR32_BLACK, w - h - 6);

	/* weight is in line 3 */
	txt_height = vultures_text_height(V_FONT_MENU, win->caption);
	text_start_y = y + txt_height*2 + 4;

	/* draw the object weight */
	tmpstr[0] = '\0';
	if (weight)
		snprintf(tmpstr, 32, "w: %d", weight);
	text_start_x = x + (w - vultures_text_length(V_FONT_MENU, tmpstr))/2;
	vultures_put_text_shadow(V_FONT_MENU, tmpstr, vultures_screen, text_start_x,
								text_start_y, textcolor, CLR32_BLACK);

	if (win->selected)
	{
		tmpstr[0] = '\0';
		if (win->pd.count <= 0 || (win->pd.obj && win->pd.count > win->pd.obj->quan))
			snprintf(tmpstr, 32, "selected (all)");
		else
			snprintf(tmpstr, 32, "selected (%d)", win->pd.count);
		text_start_x = x + w - vultures_text_length(V_FONT_MENU, tmpstr) - 6;
		vultures_put_text_shadow(V_FONT_MENU, tmpstr, vultures_screen, text_start_x,
									text_start_y, textcolor, CLR32_BLACK);
	}

	/* draw the tile itself */
	/* constrain the drawing region to the box for the object tile, so that large
	* tiles don't overlap */
	vultures_set_draw_region(x + 2, y + 2, x + h - 3, y + h - 3);

	/* darken the background */
	vultures_fill_rect(x + 2, y + 2, x + h - 3, y + h - 3, CLR32_BLACK_A30);

	/* indicate blessed/cursed visually */
	if (win->pd.obj && win->pd.obj->bknown && win->pd.obj->blessed)
		vultures_fill_rect(x + 2, y + 2, x + h - 3, y + h - 3, CLR32_BLESS_BLUE);

	if (win->pd.obj && win->pd.obj->bknown && win->pd.obj->cursed)
		vultures_fill_rect(x + 2, y + 2, x + h - 3, y + h - 3, CLR32_CURSE_RED);

	/* draw the object tile */
	vultures_put_tile(tile_x, tile_y, tile);

	/* draw the item letter on the top left corner of the object tile */
	snprintf(tmpstr, 11, "%c", win->accelerator);
	vultures_put_text_shadow(V_FONT_MENU, tmpstr, vultures_screen, x + 2,
								y + 2, textcolor, CLR32_BLACK);

	/* draw the quantity on the tile */
	if (win->pd.obj && win->pd.obj->quan > 1)
	{
		snprintf(tmpstr, 11, "%ld", win->pd.obj->quan);
		txt_height = vultures_text_height(V_FONT_MENU, tmpstr);
		text_start_x = x + h - vultures_text_length(V_FONT_MENU, tmpstr) - 2;
		vultures_put_text_shadow(V_FONT_MENU, tmpstr, vultures_screen, text_start_x,
									y + h - txt_height, CLR32_WHITE, CLR32_BLACK);
	}

	/* restore the drawing region */
	vultures_set_draw_region(0, 0, vultures_screen->w - 1, vultures_screen->h - 1);

	vultures_invalidate_region(x, y, w, h);

	return 0;
}


static int vultures_draw_objitemheader(struct window * win)
{
	int x = win->abs_x;
	int y = win->abs_y;

	/* constrain drawing to this window */
	vultures_set_draw_region(x, y, x + win->w - 1, y + win->h - 1);

	vultures_fill_rect(x+2, y+2, x + win->w - 3, y + win->h - 3, CLR32_BLACK_A50);

	/* Outer edge */
	vultures_draw_lowered_frame(x, y, x+win->w-1, y+win->h-1);
	/* Inner edge */
	vultures_draw_raised_frame(x+1, y+1, x+win->w-2, y+win->h-2);

	/* draw the text centered in the window */
	int txt_width = vultures_text_length(V_FONT_MENU, win->caption);
	int txt_height = vultures_text_height(V_FONT_MENU, win->caption);
	int text_start_x = x + (win->w - txt_width)/2;
	int text_start_y = y + (win->h - txt_height)/2;

	vultures_put_text_shadow(V_FONT_MENU, win->caption, vultures_screen, text_start_x,
							text_start_y, CLR32_WHITE, CLR32_BLACK);

	vultures_invalidate_region(x, y, win->w, win->h);

	/* lift drawing region restriction */
	vultures_set_draw_region(0, 0, vultures_screen->w - 1, vultures_screen->h - 1);

	return 0;
}


#endif

/*********************************
* misc utility functions
*********************************/

window *vultures_walk_winlist(struct window *win, bool *descend)
{
	/* 1) try to descend to child */
	if (*descend && win->first_child)
		return win->first_child;

	/* 2) try sibling*/
	if (win->sib_next) {
		*descend = true;
		return win->sib_next;
	}

	/* 3) ascend to parent and set *descend = false to prevent infinite loops */
	*descend = false;
	return win->parent;
}


/* find the window under the mouse, starting from topwin */
window *vultures_get_window_from_point(window *topwin, point mouse)
{
	window *winptr, *nextwin;

	winptr = topwin;

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
		else return winptr;
	}
	return winptr;
}


void vultures_invalidate_region(int x , int y, int w, int h)
{
	int i;

	/* look at known invalid rects */
	for (i = 0; i < vultures_invrects_num; i++)
	{
		if (x > vultures_invrects[i].x &&
			y > vultures_invrects[i].y &&
			x + w < vultures_invrects[i].x + vultures_invrects[i].w &&
			y + h < vultures_invrects[i].y + vultures_invrects[i].h)
			/* new invalid region is fully inside an already known one */
			return;

		else if (x < vultures_invrects[i].x &&
				y < vultures_invrects[i].y &&
				x + w > vultures_invrects[i].x + vultures_invrects[i].w &&
				y + h > vultures_invrects[i].y + vultures_invrects[i].h)
				/* old invalid region is fully inside new one;
				* stop searching here and reuse this entry in the array */
			break;
	}

	if (i >= vultures_invrects_max)
	{
		vultures_invrects = (SDL_Rect *)realloc(vultures_invrects, (vultures_invrects_max + 16) * sizeof(SDL_Rect));
		vultures_invrects_max += 16;
	}

	if (i == vultures_invrects_num)
		vultures_invrects_num++;

	vultures_invrects[i].x = x;
	vultures_invrects[i].y = y;
	vultures_invrects[i].w = w;
	vultures_invrects[i].h = h;
}


/* refresh invalid regions */
void vultures_refresh_window_region(void)
{
	int x1 = 9999, y1 = 9999, x2 = 0, y2 = 0;
	int i;

	/* find the bounding rectangla around all invalid rects */
	for (i = 0; i < vultures_invrects_num; i++)
	{
		x1 = (x1 < vultures_invrects[i].x) ? x1 : vultures_invrects[i].x;
		y1 = (y1 < vultures_invrects[i].y) ? y1 : vultures_invrects[i].y;

		x2 = (x2 > (vultures_invrects[i].x + vultures_invrects[i].w)) ?
			x2 : (vultures_invrects[i].x + vultures_invrects[i].w);
		y2 = (y2 > (vultures_invrects[i].y + vultures_invrects[i].h)) ?
			y2 : (vultures_invrects[i].y + vultures_invrects[i].h);
	}

	/* refresh the bounding rect */
	if (x1 < x2 && y1 < y2)
		vultures_refresh_region(x1, y1, x2, y2);

	/* there are now 0 invalid rects */
	if (vultures_invrects)
		free(vultures_invrects);

	vultures_invrects = NULL;
	vultures_invrects_num = 0;
	vultures_invrects_max = 0;
}


/* determine whether a window intersects an invalid region */
int vultures_intersects_invalid(window *win)
{
	int i;
	int x1, y1, x2, y2;

	for (i = 0; i < vultures_invrects_num; i++) {
		/* check intersection with each invalid rect */
		if (win->abs_x > (vultures_invrects[i].x + vultures_invrects[i].w) ||
			win->abs_y > (vultures_invrects[i].y + vultures_invrects[i].h) ||
			(win->abs_x + win->w) < vultures_invrects[i].x ||
			(win->abs_y + win->h) < vultures_invrects[i].y)
			continue;

		x1 = (win->abs_x > vultures_invrects[i].x) ? win->abs_x : vultures_invrects[i].x;
		y1 = (win->abs_y > vultures_invrects[i].y) ? win->abs_y : vultures_invrects[i].y;
		x2 = (win->abs_x + win->w > vultures_invrects[i].x + vultures_invrects[i].w) ?
				vultures_invrects[i].x + vultures_invrects[i].w : win->abs_x + win->w;
		y2 = (win->abs_y + win->h > vultures_invrects[i].y + vultures_invrects[i].h) ?
				vultures_invrects[i].y + vultures_invrects[i].h : win->abs_y + win->h;

		if (x1 < x2 && y1 < y2)
			return 1;
	}

	return 0;
}


/* resize the vultures application window to the given width and height */
void vultures_win_resize(int width, int height)
{
	struct window *current, *topwin;
	SDL_Event event;
	vultures_event dummy;
	bool descend = true;

	current = topwin = vultures_get_window(0);

	event.type = SDL_VIDEORESIZE;
	event.resize.w = width;
	event.resize.h = height;

	current->event_handler(current, &dummy, &event);

	do
	{
		current = vultures_walk_winlist(current, &descend);

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

	vultures_draw_windows(topwin);
	vultures_refresh_window_region();
}


#ifdef VULTURESEYE
/* show a main menu with common options when the user presses esc */
void vultures_show_mainmenu()
{
	int winid, n;
	anything any;
	menu_item *selected;

	winid = vultures_create_nhwindow(NHW_MENU);
	vultures_start_menu(winid);
	
	any.a_int = 1;
	vultures_add_menu(winid, NO_GLYPH, &any, 'h', 0, ATR_BOLD,
					"Help", MENU_UNSELECTED);
	any.a_int = 2;
	vultures_add_menu(winid, NO_GLYPH, &any, 'O', 0, ATR_BOLD,
					"Options", MENU_UNSELECTED);
	any.a_int = 3;
	vultures_add_menu(winid, NO_GLYPH, &any, 'I', 0, ATR_BOLD,
					"Interface options", MENU_UNSELECTED);
	any.a_int = 4;
	vultures_add_menu(winid, NO_GLYPH, &any, 'S', 0, ATR_BOLD,
					"Save & Quit", MENU_UNSELECTED);
	any.a_int = 5;
	vultures_add_menu(winid, NO_GLYPH, &any, 'Q', 0, ATR_BOLD,
					"Quit", MENU_UNSELECTED);

	vultures_end_menu(winid, "Main menu");
	n = vultures_select_menu(winid, PICK_ONE, &selected);
	vultures_destroy_nhwindow(winid);

	if (n < 1)
		return;

	switch(selected[0].item.a_int)
	{
		case 1: dohelp(); break;
		case 2: doset(); break;
		case 3: vultures_iface_opts(); break;
		case 4: dosave(); break;
		case 5: done2(); break;
	}
}
#endif

