/* Copyright (c) Daniel Thaler, 2006				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <ctype.h>

#include "SDL.h"

#include "hack.h"
#include "skills.h"

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


#define ROOTWIN vultures_windows[0]

#define V_MESSAGEBUF_SIZE 512
#define V_EVENTSTACK_SIZE 32



/******************************************************************************
 * globals vars
 ******************************************************************************/


static struct window ** vultures_windows = NULL;
static int windowcount_cur = 0;
static int windowcount_max = 0;

static SDL_Rect * vultures_invrects = NULL;
static int vultures_invrects_num = 0;
static int vultures_invrects_max = 0;


/*********************************/

Uint32 vultures_message_colors[V_MAX_MESSAGE_COLORS];
Uint32 vultures_warn_colors[V_MAX_WARN];

vultures_window_graphics vultures_winelem; /* contains borders, background, etc */

int vultures_windows_inited = 0;
int vultures_suppress_helpmsg;
int vultures_winid_map = 0;
int vultures_winid_minimap = 0;
int vultures_winid_enhance = 0;
SDL_Surface * vultures_statusbar = NULL;
int vultures_whatis_singleshot = 0;

static int * vultures_messages_ages = NULL;
static char ** vultures_messages_buf = NULL;
static int vultures_messages_cur, vultures_messages_top;

vultures_event * vultures_eventstack = NULL;
int vultures_eventstack_top;


/******************************************************************************
 * function pre-declarations
 ******************************************************************************/

static int vultures_draw_button(struct window * win);
static int vultures_draw_mainwin(struct window * win);
static int vultures_draw_option(struct window * win);
static int vultures_draw_scrollbar(struct window * win);
static int vultures_draw_text(struct window * win);
static int vultures_draw_dropdown(struct window * win);
static int vultures_draw_objitem(struct window * win);
static int vultures_draw_objitemheader(struct window * win);

struct window * vultures_walk_winlist(struct window * win, int * descend);

static void vultures_status_add_cond(const char * str, int warnno, int color, struct window * tarray[5][5]);

void vultures_update_background(struct window * win);
int vultures_intersects_invalid(struct window * win);

static int vultures_event_dispatcher_core(SDL_Event * event, void * result, struct window * topwin);
static int vultures_handle_event(struct window * topwin, struct window * win,
                                 void * result, SDL_Event * event, int * redraw);
static void vultures_check_enhance(void);

extern boolean can_advance(int skill, int speedy);

/******************************
 * window management functions
 ******************************/

struct window * vultures_create_window_internal(int nh_type, struct window * parent, int wintype)
{
    int winid = 1;
    struct window * newwin;

    /* if necessary make space in the vultures_windows array */
    if (windowcount_cur == windowcount_max)
    {
        vultures_windows = (struct window **)realloc(vultures_windows, (windowcount_max + 16) * sizeof(struct window*));
        memset(&vultures_windows[windowcount_max], 0, 16 * sizeof(struct window*));
        windowcount_max += 16;

        /* no need to search through the first windowcount_cur ids, they're definitely taken */
        winid = windowcount_cur;
    }

    if (!parent)
        parent = ROOTWIN;

    /* find an id for the new window */
    while (vultures_windows[winid] && winid < windowcount_max)
        winid++;

    if (winid == windowcount_max)
        panic("could not find a free winarray entry, even though there should have been one!\n");

    /* alloc the new window */
    newwin = (struct window *)malloc(sizeof(struct window));
    if (!newwin)
        panic("alloc failed!\n");

    memset(newwin, 0, (sizeof(struct window)));
    vultures_windows[winid] = newwin;
    windowcount_cur++;

    /* initialize basic struct fields */
    newwin->id = winid;
    newwin->parent = parent;
    newwin->nh_type = nh_type;
    newwin->v_type = wintype;
    newwin->need_redraw = 1;
    newwin->visible = 1;

    /* add it to the parent's ll of children */
    newwin->sib_prev = parent->last_child;
    if (parent->first_child)
        parent->last_child->sib_next = newwin;
    else
        parent->first_child = newwin;
    parent->last_child = newwin;

    /* set up a draw() function for the window, if it doesn't want to draw itself (V_WINTYPE_CUSTOM) */
    vultures_init_wintype(newwin, wintype);

    return newwin;
}


/* set the correct values for various window vars depending on the window type,
 * in particular draw and event_handler */
void vultures_init_wintype(struct window * win, int wintype)
{
    win->v_type = wintype;

    switch(wintype)
    {
        case V_WINTYPE_MAIN:
            win->draw = vultures_draw_mainwin;
            win->autobg = 1;
            break;

        case V_WINTYPE_BUTTON:
            win->draw = vultures_draw_button;
            win->event_handler = vultures_eventh_button;
            win->autobg = 1;
            break;

        case V_WINTYPE_OPTION:
            win->draw = vultures_draw_option;
            win->pd.count = 0;
            break;

        case V_WINTYPE_SCROLLBAR:
            win->draw = vultures_draw_scrollbar; break;

        case V_WINTYPE_TEXT:
            win->draw = vultures_draw_text;
            win->pd.textcolor = V_COLOR_TEXT;
            break;

        case V_WINTYPE_DROPDOWN:
            win->draw = vultures_draw_dropdown;
            win->event_handler = vultures_eventh_dropdown;
            win->autobg = 1;
            break;

        case V_WINTYPE_OBJITEM:
            win->draw = vultures_draw_objitem;
            win->w = V_LISTITEM_WIDTH;
            win->h = V_LISTITEM_HEIGHT;
            win->scrollable = 0;
            win->autobg = 1;
            break;

        case V_WINTYPE_OBJITEMHEADER:
            win->draw = vultures_draw_objitemheader;
            win->event_handler = NULL;
            win->w = V_LISTITEM_WIDTH;
            win->h = V_LISTITEM_HEIGHT;
            win->scrollable = 0;
            break;

        default:
            win->draw = NULL;
    }
}



/* remove a window from all lists of  */
static void vultures_unlink_window(struct window * win)
{
    /* bemove from the global id list */
    vultures_windows[win->id] = NULL;
    windowcount_cur--;

    /* the root window has no parent */
    if (win->parent)
    {
        /* unlink the window everywhere */
        if (win->parent->first_child == win)
            win->parent->first_child = win->sib_next;

        if (win->parent->last_child == win)
            win->parent->last_child = win->sib_prev;
    }

    /* remove from the linked list of siblings */
    if (win->sib_prev)
        win->sib_prev->sib_next = win->sib_next;

    if (win->sib_next)
        win->sib_next->sib_prev = win->sib_prev;
}


void vultures_destroy_window_internal(struct window * win)
{
    if (!win)
        /* already destroyed */
        return;

    vultures_unlink_window(win);

    /* destroy it's children. note that the child's destroy function will manipulate ->first_child */
    while (win->first_child)
    {
        if (!win->visible)
            win->first_child->visible = 0;
        vultures_destroy_window_internal(win->first_child);
    }

    /* free up alloced resources */
    if (win->caption)
        free(win->caption);

    /* we may want to restore the background before deleting it */
    if (win->visible && win->background && win->autobg)
    {
        vultures_put_img(win->abs_x, win->abs_y, win->background);
        vultures_refresh_region(win->abs_x, win->abs_y, win->abs_x + win->w, win->abs_y + win->h);
    }

    /* make sure the background gets freed even if it doesn't get restored */
    if (win->background)
        SDL_FreeSurface(win->background);

    /* if there is a stored image we free that too */
    if (win->image)
        SDL_FreeSurface(win->image);

    free(win);
}


/* hide a window; if possible blit a stored background back onto the screen */
void vultures_hide_window(struct window * win)
{
    if (!win)
        return;

    if (win->background && win->autobg)
    {
        vultures_put_img(win->abs_x, win->abs_y, win->background);
        vultures_refresh_region(win->abs_x, win->abs_y, win->abs_x + win->w, win->abs_y + win->h);
        SDL_FreeSurface(win->background);
        win->background = NULL;
    }

    win->visible = 0;
    win->need_redraw = 0;
}


void vultures_create_root_window(void)
{
    /* create window array */
    vultures_windows = (struct window **)malloc(16 * sizeof(struct window*));
    memset(vultures_windows, 0, 16 * sizeof(struct window*));
    windowcount_max = 16;
    struct window * win, *subwin;

    /* create base window */
    win = (struct window *)malloc(sizeof(struct window));
    ROOTWIN = win;
    memset(win, 0, (sizeof(struct window)));
    /* root MUST have an event handler, because unhandled event get
     * passed up and might end up being given to the root window */
    win->draw = vultures_draw_level;
    win->event_handler = vultures_eventh_level;
    win->nh_type = NHW_MAP;
    win->x = 0;
    win->y = 0;
    win->w = vultures_screen->w;
    win->h = vultures_screen->h;
    win->need_redraw = 1;
    windowcount_cur = 1;


        /* upper left scroll hotspot */
    vultures_create_hotspot(0        ,0        ,20       , 20,
        V_HOTSPOT_SCROLL_UPLEFT, win, "scroll diagonally");
    /* upper right scroll hotspot */
    vultures_create_hotspot(win->w-20,0        ,20       , 20,
        V_HOTSPOT_SCROLL_UPRIGHT, win, "scroll diagonally");
    /* left scroll hotspot */
    vultures_create_hotspot(0        ,20       ,20       , win->h-40,
        V_HOTSPOT_SCROLL_LEFT, win, "scroll left");
    /* right scroll hotspot */
    vultures_create_hotspot(win->w-20,20       ,20       , win->h-40,
        V_HOTSPOT_SCROLL_RIGHT, win, "scroll right");
    /* bottom left scroll hotspot */
    vultures_create_hotspot(0        ,win->h-20,20       , 20,
        V_HOTSPOT_SCROLL_DOWNLEFT, win, "scroll diagonally");
    /* bottom scroll hotspot */
    vultures_create_hotspot(20       ,win->h-20,win->w-40, 20,
        V_HOTSPOT_SCROLL_DOWN, win, "scroll down");
    /* upper left scroll hotspot */
    vultures_create_hotspot(win->w-20,win->h-20,20       , 20,
        V_HOTSPOT_SCROLL_DOWNRIGHT, win, "scroll diagonally");

    /* create the minimap now, so that it's drawn _under_ the message window */
    subwin = vultures_create_window_internal(0, win, V_WINTYPE_CUSTOM);
    subwin->draw = vultures_draw_minimap;
    subwin->event_handler = vultures_eventh_minimap;
    subwin->image = vultures_load_graphic(NULL, V_FILENAME_MINIMAPBG);
    subwin->w = subwin->image->w;
    subwin->h = subwin->image->h;
    subwin->x = win->w - (subwin->w + 6);
    subwin->y = 6;
    subwin->visible = vultures_opts.show_minimap;
    subwin->menu_id = V_WIN_MINIMAP;
    subwin->autobg = 1;
    vultures_winid_minimap = subwin->id;


    /*NOTE: the upper scroll hotspot gets created later in vultures_create_nhwindow,
     * so that it covers the message window */

     vultures_windows_inited = 1;
}


void vultures_cleanup_windows(void)
{
    if (!vultures_windows)
        return;

    /* destroy any surviving windows */
    while (ROOTWIN->first_child)
        vultures_destroy_window_internal(ROOTWIN->first_child);

    free(ROOTWIN);
    free(vultures_windows);
}



/* encapsulate the vultures_windows array, so that it doesn't need to be global */
struct window * vultures_get_window(int winid)
{
    return vultures_windows[winid];
}


/* create a hotspot. a hotspot is an invisible window, whose purpose is recieving events */
int vultures_create_hotspot(int x, int y, int w, int h, int menu_id, struct window * parent, char * name)
{
    struct window * win;

    win = vultures_create_window_internal(0, parent, V_WINTYPE_CUSTOM);
    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;
    win->abs_x = parent->abs_x + win->x;
    win->abs_y = parent->abs_y + win->y;
    win->menu_id = menu_id;
    win->caption = strdup(name);

    return win->id;
}


/* create a button */
struct window * vultures_create_button(struct window * parent, const char * caption, int menu_id)
{
    struct window * win;

    win = vultures_create_window_internal(0, parent, V_WINTYPE_BUTTON);

    if (caption)
        win->caption = strdup(caption);
    else
        win->caption = NULL;

    win->menu_id = menu_id;

    return win;
}



/******************************
 * High-level window functions
 ******************************/

void vultures_messagebox(const char * message)
{
    struct window * win;
    int dummy;

    win = vultures_create_window_internal(0, NULL, V_WINTYPE_MAIN);
    win->event_handler = vultures_eventh_messagebox;
    win->caption = strdup(message);

    vultures_create_button(win, "Continue", 1);

    vultures_layout_menu(win);

    win->abs_x = win->parent->x + win->x;
    win->abs_y = win->parent->y + win->y;

    vultures_event_dispatcher(&dummy, V_RESPOND_INT, win);

    vultures_destroy_window_internal(win);
}


int vultures_get_input(int force_x, int force_y, const char *ques, char *input)
{
    struct window * win, *subwin;
    int response;

    win = vultures_create_window_internal(0, NULL, V_WINTYPE_MAIN);
    win->event_handler = vultures_eventh_input;
    win->caption = strdup(ques);

    subwin = vultures_create_window_internal(0, win, V_WINTYPE_TEXT);
    subwin->is_input = 1;
    subwin->menu_id = 1;
    subwin->autobg = 1;
    /* duplicate input. DON'T point to input directly, that causes segfaults
     * if the game is quit while an input is open */
    subwin->caption = (char *)malloc(256);
    subwin->caption[0] = '\0';

    /* calc sizes and positions */
    win->w = vultures_text_length(V_FONT_HEADLINE, (char *)ques);
    win->w = (win->w < 500) ? 500 : win->w;

    subwin->w = win->w;
    win->w += vultures_winelem.border_left->w + vultures_winelem.border_right->w;
    win->h = vultures_winelem.border_top->h + vultures_get_lineheight(V_FONT_HEADLINE) +
             3 * vultures_get_lineheight(V_FONT_INPUT) + vultures_winelem.border_bottom->h;
    subwin->h = vultures_get_lineheight(V_FONT_INPUT) + 1;

    subwin->x = (win->w - subwin->w) / 2;
    subwin->y = vultures_winelem.border_top->h + vultures_get_lineheight(V_FONT_HEADLINE) +
                vultures_get_lineheight(V_FONT_INPUT);

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
    strncpy(input, subwin->caption, 256);

    /* clean up */
    vultures_destroy_window_internal(win);

    return response;
}


/******************************
 * window builders
 ******************************/
struct window * vultures_query_choices(const char * ques, const char *choices, char defchoice)
{
    struct window *win, *button;
    int nbuttons = 0, longdesc = 0;
    int i, len;
    char * str = (char*)choices;

    win = vultures_create_window_internal(0, NULL, V_WINTYPE_MAIN);
    win->event_handler = vultures_eventh_query_choices;
    win->caption = strdup(ques);

    nbuttons = strlen(choices);

    /* a very common case is "yn" queries. Improve that to a yes/no query*/
    if (strncmp(choices, "yn", 3) == 0)
    {
        str = "yes\0no";
        longdesc = 1;
    }
    else if (strncmp(choices, "ynq", 4) == 0)
    {
        str = "yes\0no\0quit";
        longdesc = 1;
    }

    for(i = 0; i < nbuttons; i++)
    {
        len = 1;
        if (longdesc)
            len = strlen(str);

        button = vultures_create_window_internal(0, win, V_WINTYPE_BUTTON);

        button->caption = (char *)malloc(len+1);
        strncpy(button->caption, str, len);
        button->caption[len] = '\0';

        button->accelerator = str[0];
        button->is_default = (str[0] == defchoice);
        button->menu_id = i;

        if (longdesc)
            str += len;

        str++;
    }

    vultures_layout_menu(win);

    return win;
}



struct window * vultures_query_direction(const char * ques)
{
    struct window *win, *subwin;

    win = vultures_create_window_internal(0, NULL, V_WINTYPE_MAIN);
    win->event_handler = vultures_eventh_query_direction;
    win->caption = strdup(ques);

    subwin = vultures_create_window_internal(0, win, V_WINTYPE_CUSTOM);
    subwin->draw = vultures_draw_dirarrows;
    subwin->event_handler = NULL;
    subwin->w = vultures_winelem.direction_arrows->w;
    subwin->h = vultures_winelem.direction_arrows->h;


    /* calculate window layout */
    if (ques)
        win->w = vultures_text_length(V_FONT_HEADLINE, ques);
    win->w = (win->w > subwin->w) ? win->w : subwin->w;
    win->w += vultures_winelem.border_left->w + vultures_winelem.border_right->w;

    subwin->y = vultures_winelem.border_top->h + vultures_get_lineheight(V_FONT_HEADLINE) * 1.5;
    subwin->x = (win->w - subwin->w) / 2;

    win->h = subwin->y + subwin->h + vultures_winelem.border_bottom->h;

    win->x = (win->parent->w - win->w) / 2;
    win->y = (win->parent->h - win->h) / 2;

    return win;
}



struct window * vultures_query_anykey(const char * ques)
{
    struct window *win, *subwin;

    win = vultures_create_window_internal(0, NULL, V_WINTYPE_MAIN);
    win->event_handler = vultures_eventh_query_anykey;
    win->caption = strdup(ques);

    /* this gets a menu id, because it will store and display the count, if necessary */
    subwin = vultures_create_window_internal(0, win, V_WINTYPE_TEXT);
    subwin->caption = strdup("(type any key)");
    subwin->menu_id = 1;
    subwin->autobg = 1;


    /* create buttons */
    subwin = vultures_create_button(win, "Show choices", 2);
    subwin->accelerator = '?';

    subwin = vultures_create_button(win, "Show inventory", 3);
    subwin->accelerator = '*';

    subwin = vultures_create_button(win, "Cancel", 4);
    subwin->accelerator = '\033';

    vultures_layout_menu(win);

    return win;
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
    vultures_check_enhance();

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

    if (event->type == SDL_TIMEREVENT)
    {
        hovertime_prev = hovertime;
        hovertime += 150; /* 100 ms event timeout + 20 ms delay after the last event */
        event->user.code = hovertime;
    }
    else
    {
        hovertime = 0;
        event->user.code = 0;
    }

    /* keyboard events are always given to topwin, because mouse cursor
        * position has nothing to do with keyboard input */
    if (event->type == SDL_KEYDOWN && topwin->event_handler)
    {
        event_result = topwin->event_handler(topwin, topwin, result, event);
        if (event_result == V_EVENT_HANDLED_REDRAW || event_result == V_EVENT_UNHANDLED_REDRAW)
            redraw = 1;
    }
    else
    {
        /* find out what window the mouse is over now */
        mouse = vultures_get_mouse_pos();
        win = vultures_get_window_from_point(topwin, mouse);

        /* All other events belong to the window under the mouse cursor */
        if (event->type == SDL_MOUSEMOTION)
        {
            /* delete tooltip; if the mouse gets moved somewhere interesting
                * the window it is over will set up a new tooltip */
            vultures_mouse_invalidate_tooltip(0);

            /* notify the window the mouse got moved out of */
            win_old = vultures_get_window_from_point(topwin, mouse_old);
            if (win_old && win != win_old && win_old != win->parent)
            {
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
static int vultures_handle_event(struct window * topwin, struct window * win,
                                 void * result, SDL_Event * event, int * redraw)
{
    int event_result = V_EVENT_UNHANDLED;
    struct window * winptr = win;

    while (event_result < V_EVENT_HANDLED_NOREDRAW)
    {
        /* ascend past windows that don't have an event handler */
        while (!winptr->event_handler && winptr != topwin)
            winptr = winptr->parent;

        event_result = winptr->event_handler(winptr, win, result, event);
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
void vultures_draw_windows(struct window * topwin)
{
    int descend = 1;
    int invalid = 0;
    struct window * current = topwin;

    vultures_update_background(current);

    if (topwin->need_redraw && !topwin->draw(topwin))
    {
        /* topwin->draw() == 0 means that the window took care of
         * redrawing it's children itself */
        topwin->need_redraw = 0;
        return;
    }

    topwin->need_redraw = 0;

    do
    {
        current = vultures_walk_winlist(current, &descend);

        if (!current->visible || current->scrollable)
        {
            descend = 0;
            continue;
        }

        /* recalc absolute position */
        if (current->v_type != V_WINTYPE_NONE)
        {
            current->abs_x = current->parent->abs_x + current->x;
            current->abs_y = current->parent->abs_y + current->y;
        }

        /* if the window intersects an invalid region, some window "underneath" it
         * painted over part of it; we need to refresh the saved background and redraw, then */
        invalid = vultures_intersects_invalid(current);

        if (current->need_redraw || (invalid && descend))
        {
            vultures_update_background(current);

            /* setting descend = 0 will prevent the next call to vultures_walk_winlist
             * from descending to a child window. The window's draw() function can choose
             * to let us redraw it's children from here by returning 1 */
            if (current->draw)
                descend = current->draw(current);

            current->need_redraw = 0;
        }
    }
    /* vultures_walk_winlist will eventually arive back at the top window */
    while (current != topwin);
}


/* 1) "generic" drawing functions that apply to entire classes of windows */

static int vultures_draw_button(struct window * win)
{
    int x = win->abs_x;
    int y = win->abs_y;

    if (win->background)
        /* re-draw background: if the button just became un-pressed
         * we get messed up graphics otherwise */
        vultures_put_img(x, y, win->background);

    int text_start_x, text_start_y;

    /* Black edge */
    vultures_rect(x+1, y+1, x+win->w-2, y+win->h-2, V_COLOR_BACKGROUND);

    /* Outer edge (lowered) */
    vultures_draw_lowered_frame(x, y, x + win->w - 1, y + win->h - 1);
    /* Inner edge (raised) */
    vultures_draw_raised_frame(x + 2, y + 2, x + win->w - 3, y + win->h - 3);

    if (win->caption)
    {
        text_start_x = x + (win->w - vultures_text_length(V_FONT_BUTTON, win->caption))/2;
        text_start_y = y + 5;

        vultures_put_text_shadow(V_FONT_BUTTON, win->caption, vultures_screen, text_start_x,
                                text_start_y, V_COLOR_TEXT, V_COLOR_BACKGROUND);
    } else if (win->image) {
        vultures_put_img(x + (win->w - win->image->w) / 2, y + (win->h - win->image->h)/2, win->image);
    }

    if (win->selected)
    {
        /* shift the *entire* image of the button (including borders)
         * 2px left and down */
        SDL_Surface *buttonimage = vultures_get_img(x, y, x+win->w-3, y+win->h-3);

        vultures_fill_rect(x, y, x + win->w - 1, y + 1, CLR32_BLACK);
        vultures_fill_rect(x, y, x + 1, y + win->h - 1, CLR32_BLACK);      
        vultures_put_img(x + 2, y + 2, buttonimage);

        SDL_FreeSurface(buttonimage);
    }

    vultures_invalidate_region(x, y, win->w, win->h);

    return 0;
}



static int vultures_draw_mainwin(struct window * win)
{
    int x = win->abs_x;
    int y = win->abs_y;
    int pos_x, pos_y;

    /* Draw corners */
    vultures_put_img(x, y, vultures_winelem.corner_tl);
    vultures_put_img(x+win->w-vultures_winelem.corner_tr->w, y, vultures_winelem.corner_tr);
    vultures_put_img(x, y+win->h-vultures_winelem.corner_bl->h, vultures_winelem.corner_bl);
    vultures_put_img(x+win->w-vultures_winelem.corner_br->w,
                y+win->h-vultures_winelem.corner_br->h, vultures_winelem.corner_br);

    /* Draw top border */
    vultures_set_draw_region(x+vultures_winelem.border_left->w, y,
                        x+win->w-vultures_winelem.border_right->w,
                        y+vultures_winelem.border_top->h);
    pos_x = x + vultures_winelem.border_left->w;

    while (pos_x <= x+win->w-vultures_winelem.border_right->w)
    {
        vultures_put_img(pos_x, y, vultures_winelem.border_top);
        pos_x += vultures_winelem.border_top->w;
    }

    /* Draw bottom border */
    vultures_set_draw_region(x+vultures_winelem.border_left->w,
                        y+win->h-vultures_winelem.border_bottom->h,
                        x+win->w-vultures_winelem.border_right->w, y+win->h);
    pos_x = x + vultures_winelem.border_left->w;

    while (pos_x <= x+win->w-vultures_winelem.border_right->w)
    {
        vultures_put_img(pos_x, y+win->h-vultures_winelem.border_bottom->h,
                    vultures_winelem.border_bottom);
        pos_x += vultures_winelem.border_bottom->w;
    }

    /* Draw left border */
    vultures_set_draw_region(x, y+vultures_winelem.border_top->h,
                        x+vultures_winelem.border_left->w,
                        y+win->h-vultures_winelem.border_bottom->h);
    pos_y = y + vultures_winelem.border_top->h;

    while (pos_y <= y+win->h-vultures_winelem.border_bottom->h)
    {
        vultures_put_img(x, pos_y, vultures_winelem.border_left);
        pos_y += vultures_winelem.border_left->h;
    }

    /* Draw right border */
    vultures_set_draw_region(x+win->w-vultures_winelem.border_right->w,
                        y+vultures_winelem.border_top->h,
                        x+win->w, y+win->h-vultures_winelem.border_bottom->h);
    pos_y = y + vultures_winelem.border_top->h;

    while (pos_y <= y+win->h-vultures_winelem.border_bottom->h)
    {
        vultures_put_img(x+win->w-vultures_winelem.border_right->w,
                    pos_y, vultures_winelem.border_right);
        pos_y += vultures_winelem.border_right->h;
    }

    /* Draw center area */
    vultures_set_draw_region(x+vultures_winelem.border_left->w,
                        y+vultures_winelem.border_top->h,
                        x+win->w-vultures_winelem.border_right->w,
                        y+win->h-vultures_winelem.border_bottom->h);
    pos_y = y + vultures_winelem.border_top->h;
    while (pos_y <= y+win->h-vultures_winelem.border_bottom->h)
    {
        pos_x = x + vultures_winelem.border_left->w;

        while (pos_x <= x+win->w-vultures_winelem.border_right->w)
        {
            vultures_put_img(pos_x, pos_y, vultures_winelem.center);
            pos_x += vultures_winelem.center->w;
        }
        pos_y += vultures_winelem.center->h;
    }
    vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);


    /* draw title */
    pos_x = win->abs_x + vultures_winelem.border_left->w;
    pos_y = win->abs_y + vultures_winelem.border_top->h;

    if (win->caption)
        vultures_put_text_shadow(V_FONT_HEADLINE, win->caption, vultures_screen, pos_x, 
                                 pos_y, V_COLOR_TEXT, V_COLOR_BACKGROUND);


    vultures_invalidate_region(x, y, win->w, win->h);

    return 1;
}



static int vultures_draw_dropdown(struct window * win)
{
    int pos_x, pos_y;
    int x, y;

    x = win->abs_x;
    y = win->abs_y;

    /* Draw center area */
    vultures_set_draw_region(win->abs_x, win->abs_y, win->abs_x + win->w - 1, win->abs_y + win->h - 1);
    pos_y = y;
    while (pos_y <= y + win->h)
    {
        pos_x = x;
        while (pos_x <= x + win->w)
        {
            vultures_put_img(pos_x, pos_y, vultures_winelem.center);
            pos_x += vultures_winelem.center->w;
        }
        pos_y += vultures_winelem.center->h;
    }

    /* Draw black border */
    vultures_rect(x, y, x+win->w-1, y+win->h-1, 0);

    /* Draw edges (raised) */
    vultures_line(x+1,        y+1,        x+win->w-3, y+1,        CLR32_GRAY20);
    vultures_line(x+1,        y+win->h-2, x+win->w-2, y+win->h-2, CLR32_GRAY70);
    vultures_line(x+win->w-2, y+1,        x+win->w-2, y+win->h-2, CLR32_GRAY77);
    vultures_line(x+1,        y+1,        x+1,        y+win->h-3, CLR32_GRAY20);

    vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);

    vultures_invalidate_region(x, y, win->w, win->h);

    return 1;
}



/* draw a checkbox or radio button, depending on the menu type */
static int vultures_draw_option(struct window * win)
{
    /* PICK_ONE -> draw radio butttons */
    if (win->parent->select_how == PICK_ONE)
    {
        if (win->selected)
            vultures_put_img(win->abs_x, win->abs_y, vultures_winelem.radiobutton_on);
        else
            vultures_put_img(win->abs_x, win->abs_y, vultures_winelem.radiobutton_off);
    }
    /* otherwise we want checkboxes */
    else
    {
        if (win->selected)
        {
            /* selected items can be drawn with either an 'x' (count <= 0) or an '#' otherwise */
            if (win->pd.count <= 0)
                vultures_put_img(win->abs_x, win->abs_y, vultures_winelem.checkbox_on);
            else
                vultures_put_img(win->abs_x, win->abs_y, vultures_winelem.checkbox_count);
        }
        else
            vultures_put_img(win->abs_x, win->abs_y, vultures_winelem.checkbox_off);
    }

    /* draw the option description */
    vultures_put_text_shadow(V_FONT_MENU, win->caption, vultures_screen,
                             win->abs_x + vultures_winelem.radiobutton_off->w + 4,
                             win->abs_y + 2, V_COLOR_TEXT, V_COLOR_BACKGROUND);

    vultures_invalidate_region(win->abs_x, win->abs_y, win->w, win->h);

    return 0;
}


static int vultures_draw_scrollbar(struct window * win)
{
    int pos_y, scrollpos;
    int scrollarea_top = win->abs_y + vultures_winelem.scrollbutton_up->h;
    int scrollarea_bottom = win->abs_y + win->h -
                            vultures_winelem.scrollbutton_down->h;

    /* draw top & bottom buttons */
    vultures_put_img(win->abs_x, win->abs_y, vultures_winelem.scrollbutton_up);
    vultures_put_img(win->abs_x, scrollarea_bottom, vultures_winelem.scrollbutton_down);

    /* draw the scrollbar backgound */
    vultures_set_draw_region(win->abs_x, scrollarea_top, win->abs_x +
                        vultures_winelem.scrollbar->w - 1, scrollarea_bottom - 1);
    for (pos_y = scrollarea_top; pos_y < scrollarea_bottom; pos_y += vultures_winelem.scrollbar->h)
        vultures_put_img(win->abs_x, pos_y, vultures_winelem.scrollbar);

    vultures_set_draw_region(0, 0, vultures_screen->w - 1, vultures_screen->h - 1);

    /* draw scroll indicator */

    /* for menus that are only _very_ slightly longer than V_MENU_MAXHEIGHT
     * failing to consider spacing between menu elements will lead to scrollps valies
     * that are substantially larger than 8192 (100%). That's OK for the main
     * menu area (desired even), but not for the scroll indicator */
    scrollpos = (win->pd.scrollpos <= 8192) ? win->pd.scrollpos : 8192;

    pos_y = scrollarea_top + ((scrollarea_bottom - scrollarea_top -
            vultures_winelem.scroll_indicator->h) * scrollpos) / 8192.0;
    vultures_put_img(win->abs_x, pos_y, vultures_winelem.scroll_indicator);

    vultures_invalidate_region(win->abs_x, win->abs_y, win->w, win->h);

    return 0;
}


static int vultures_draw_text(struct window * win)
{
    int textlen = 0;

    if (win->background)
        /* the text might have changed, so redraw the background if there is one */
        vultures_put_img(win->abs_x, win->abs_y, win->background);

    if (win->caption)
    {
        vultures_put_text_shadow(V_FONT_MENU, win->caption, vultures_screen,
                 win->abs_x, win->abs_y, win->pd.textcolor, V_COLOR_BACKGROUND);

        textlen = vultures_text_length(V_FONT_MENU, win->caption);
    }


    if (win->is_input)
        /* draw prompt */
        vultures_rect(win->abs_x + textlen + 1, win->abs_y, win->abs_x + textlen + 1,
                 win->abs_y + win->h - 2, V_COLOR_TEXT);

    vultures_invalidate_region(win->abs_x-2, win->abs_y, win->w+4, win->h);

    return 0;
}


int vultures_draw_img(struct window * win)
{
    vultures_set_draw_region(win->abs_x, win->abs_y,
                        win->abs_x + win->w - 1, win->abs_y + win->h - 1);
    vultures_put_img(win->abs_x, win->abs_y, win->image);
    vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);

    vultures_invalidate_region(win->abs_x, win->abs_y, win->w, win->h);

    return 1;
}


/* 2) specialized window drawing funtions that apply to a single window */
int vultures_draw_messages(struct window * win)
{
    int age, textlen, num_messages;
    int pos_x, pos_y, i;
    char * message;
    int refresh_x, refresh_y, refresh_h, refresh_w;
    SDL_Surface * shade = win->image;
    int time_cur = moves;

    if (vultures_messages_getshown() != 0)
        /* set time_cur to the age of the first shown message */
        vultures_messages_get(0, &time_cur);

    refresh_x = refresh_y = 99999;
    refresh_h = refresh_w = 0;

    /* repaint background and free it */
    if (win->background)
    {
        vultures_put_img(win->abs_x, win->abs_y, win->background);
        SDL_FreeSurface(win->background);
        win->background = NULL;

        /* we need these values, so that we can refresh the larger
         * even if the window shrinks during the redraw */
        refresh_x = win->abs_x;
        refresh_y = win->abs_y;
        refresh_w = win->w;
        refresh_h = win->h;
    }

    num_messages = 0;
    win->h = 0;
    win->w = 0;

    /* calculate height & width of new message area */
    while((message = vultures_messages_get(num_messages, &age)) &&
          (time_cur-age) < V_MAX_MESSAGE_COLORS && num_messages < vultures_opts.messagelines)
    {
        win->h += (vultures_get_lineheight(V_FONT_MESSAGE) + 1);
        textlen = vultures_text_length(V_FONT_MESSAGE, message);
        win->w = (win->w < textlen) ? textlen : win->w;
        num_messages++;
    }

    /* add a bit of padding around the text */
    win->h += 2;
    win->w += 4;

    win->x = (win->parent->w - win->w) / 2;
    win->abs_x = win->parent->abs_x + win->x;
    win->abs_y = win->parent->abs_y;

    /* save new background */
    win->background = vultures_get_img(win->abs_x, win->abs_y,
                          win->abs_x + win->w-1, win->abs_y + win->h-1);


    /* shade the message area */
    vultures_set_draw_region(win->abs_x, win->abs_y,
                        win->abs_x + win->w-1, win->abs_y + win->h-1);
    pos_y = win->abs_y;
    while (pos_y <= win->abs_y + win->h)
    {
        pos_x = win->abs_x;

        while (pos_x <= win->abs_x+win->w)
        {
            vultures_put_img(pos_x, pos_y, shade);
            pos_x += shade->w;
        }
        pos_y += shade->h;
    }
    vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);


    /* draw the messages */
    for (i = 0; i < num_messages; i++)
    {
        message = vultures_messages_get(num_messages - i - 1, &age);

        pos_x = win->abs_x + (win->w - vultures_text_length(V_FONT_MESSAGE, message)) / 2;
        pos_y = win->abs_y + i * (vultures_get_lineheight(V_FONT_MESSAGE) + 1);
        vultures_put_text(V_FONT_MESSAGE, message, vultures_screen,
                          pos_x, pos_y, vultures_message_colors[time_cur-age]);
    }

    refresh_w = (refresh_w > win->w) ? refresh_w : win->w;
    refresh_h = (refresh_h > win->h) ? refresh_h : win->h;

    if (refresh_x > win->abs_x)
    {
        refresh_w += refresh_x - win->abs_x;
        refresh_x = win->abs_x;
    }
    refresh_y = (refresh_y < win->abs_y) ? refresh_y : win->abs_y;

    vultures_invalidate_region(refresh_x, refresh_y, refresh_w, refresh_h);

    return 0;
}


int vultures_draw_dirarrows(struct window * win)
{
    vultures_set_draw_region(win->abs_x, win->abs_y,
                        win->abs_x + win->w - 1, win->abs_y + win->h - 1);
    vultures_put_img(win->abs_x, win->abs_y, vultures_winelem.direction_arrows);
    vultures_set_draw_region(0, 0, vultures_screen->w - 1, vultures_screen->h - 1);

    vultures_invalidate_region(win->abs_x, win->abs_y, win->w, win->h);

    return 0;
}



int vultures_draw_menu(struct window * win)
{
    struct window * winelem;
    int scrolloffset = 0;
    int clip_top;
    int clip_bottom;
    int totalheight; /* viewable and total height of *scrolled elements* */
    int scrollpos = 0;
    int min_y = 99999, max_y = 0;

    /* draw the window + title */
    vultures_draw_mainwin(win);

    /* draw the unscrollable elements and get upper & lower limit of scrollable elements */
    winelem = win->first_child;
    while (winelem)
    {
        winelem->abs_x = win->abs_x + winelem->x;
        winelem->abs_y = win->abs_y + winelem->y;

        if (winelem->v_type == V_WINTYPE_BUTTON ||
            winelem->v_type == V_WINTYPE_SCROLLBAR)
            /* draw unscrollable elements now */
            winelem->draw(winelem);
        else
        {
            /* scrollable elements */
            min_y = (min_y < winelem->y) ? min_y : winelem->y;
            max_y = (max_y > (winelem->y + winelem->h)) ? max_y : (winelem->y + winelem->h);
        }

        if (winelem->v_type == V_WINTYPE_SCROLLBAR)
            scrollpos = winelem->pd.scrollpos;

        winelem = winelem->sib_next;
    }


    /* calc the size of the viewport available to scrollable elements */
    clip_top = win->abs_y + vultures_winelem.border_top->h;
    if (win->caption)
        clip_top += 2 * vultures_text_height(V_FONT_HEADLINE, win->caption);

    clip_bottom = win->abs_y + win->h - 
                  (15 + vultures_get_lineheight(V_FONT_MENU) +
                   vultures_winelem.border_bottom->h);

    totalheight = max_y - min_y +
    /* add one line extra space, to make sure the last element can be scrolled into view */
                 vultures_get_lineheight(V_FONT_MENU); 

    scrolloffset = (scrollpos * (totalheight - V_MENU_MAXHEIGHT)) / 8192.0;


    winelem = win->first_child;
    while (winelem)
    {

        if (winelem->v_type != V_WINTYPE_BUTTON &&
            winelem->v_type != V_WINTYPE_SCROLLBAR)
        {
            winelem->abs_y -= scrolloffset;
            if (winelem->abs_y >= clip_top && winelem->abs_y + winelem->h <= clip_bottom)
            {
                winelem->visible = 1;
                winelem->draw(winelem);
                winelem->need_redraw = 0;
            }
            else
                winelem->visible = 0;
        }

        winelem = winelem->sib_next;
    }

    /* no need to invalidate the draw region, the call to draw mainwin did that for us */
    return 0;
}


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



static int vultures_draw_objwin(struct window * win)
{
    char *stored_caption;
    char label[32];
    int x ,y, w, h, labelwidth, buttonspace;

    buttonspace = 0;
    if (win->select_how != PICK_NONE)
        buttonspace = vultures_get_lineheight(V_FONT_LARGE) + 14;

    /* draw the window, but prevent draw_mainwin from drawing the caption */
    stored_caption = win->caption;
    win->caption = NULL;
    vultures_draw_mainwin(win);
    win->caption = stored_caption;

    x = win->abs_x + vultures_winelem.border_left->w;
    y = win->abs_y + vultures_winelem.border_top->h;
    w = win->w - vultures_winelem.border_left->w - vultures_winelem.border_right->w;
    h = win->h - vultures_winelem.border_top->h - vultures_winelem.border_bottom->h;

    int headline_height = vultures_get_lineheight(V_FONT_HEADLINE);
    int headline_width = vultures_text_length(V_FONT_HEADLINE, win->caption);

    vultures_fill_rect(x, y, x + w - 1, y + headline_height*2, CLR32_BLACK_A50);

    vultures_line(x, y, x + w - 1, y, CLR32_GRAY20);
    vultures_line(x, y + headline_height*2, x + w - 1, y + headline_height * 2, CLR32_GRAY20);

    vultures_put_text_shadow(V_FONT_HEADLINE, win->caption, vultures_screen, x+(w-headline_width)/2,
                            y+headline_height/2+2, CLR32_WHITE, CLR32_GRAY20);

    if (win->pd.ow_ncols > win->pd.ow_vcols) {
        vultures_line(x, y+h-buttonspace-25, x+w-1, y+h-buttonspace-25, CLR32_GRAY77);
        
        snprintf(label, 32, "%d - %d / %d", win->pd.ow_firstcol + 1, win->pd.ow_firstcol + win->pd.ow_vcols, win->pd.ow_ncols);
        labelwidth = vultures_text_length(V_FONT_MENU, label);

        vultures_put_text_shadow(V_FONT_MENU, label, vultures_screen, x+(w-labelwidth)/2,
                                y+h-buttonspace-18, CLR32_BLACK, CLR32_GRAY20);
    }

    return 1;
}


void vultures_layout_itemwin(struct window *win)
{
    struct window *winelem, *tmpwin;
    struct obj * invitem;
    int itemcount = 0;
    int ncols, nrows;
    char * newcaption;
    int maxitems_per_col, maxitems_per_row;
    int textheight;

    if (!win->caption)
        win->caption = strdup("Inventory");

    int leftoffset = vultures_winelem.border_left->w;
    int topoffset = vultures_winelem.border_top->h;
    int rightoffset = vultures_winelem.border_right->w;
    int bottomoffset = 60; /* guesstimate for bottom border + page arrows + minimal spacing */

    textheight = vultures_get_lineheight(V_FONT_LARGE);
    topoffset += textheight*2 + 2;

    if (win->select_how != PICK_NONE)
        bottomoffset += (textheight + 14);

    win->v_type = V_WINTYPE_OBJWIN;
    win->draw = vultures_draw_objwin;

    if (win->select_how == PICK_NONE)
        win->event_handler = vultures_eventh_inventory;
    else
        win->event_handler = vultures_eventh_objwin;


    winelem = win->first_child;
    while (winelem)
    {
        if (winelem->v_type == V_WINTYPE_OPTION)
        {
            itemcount++;
            vultures_init_wintype(winelem, V_WINTYPE_OBJITEM);
            if (win->select_how != PICK_NONE)
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

    /* how many items will fit on the screen vertically */
    maxitems_per_col = (win->parent->h - topoffset - bottomoffset) / V_LISTITEM_HEIGHT;

    maxitems_per_row = (win->parent->w - 2*leftoffset - 10) / (V_LISTITEM_WIDTH + 4);

    /* calc number of rows to contain itemcount items */
    ncols = itemcount / maxitems_per_col + (itemcount % maxitems_per_col != 0 ? 1 : 0);

    /* distribute items evenly among the rows */
    nrows = itemcount / ncols + (itemcount % ncols != 0 ? 1 : 0);

    /* if there are more columns than can be displayed, prefer to maximize
     * space usage over even distribution of the items */
    if (ncols > maxitems_per_row)
        nrows = maxitems_per_col;

    win->pd.ow_vcols = ncols > maxitems_per_row ? maxitems_per_row : ncols;
    win->pd.ow_vrows = nrows > maxitems_per_col ? maxitems_per_col : nrows;
    win->pd.ow_ncols = ncols;

    win->pd.ow_firstcol = 0;

    itemcount = 0;
    winelem = win->first_child;
    while (winelem)
    {
        if (winelem->v_type == V_WINTYPE_OBJITEM || winelem->v_type == V_WINTYPE_OBJITEMHEADER)
        {
            winelem->x = (itemcount / win->pd.ow_vrows) * (V_LISTITEM_WIDTH + 4) + leftoffset;
            winelem->y = (itemcount % win->pd.ow_vrows) * V_LISTITEM_HEIGHT + topoffset;
            winelem->visible = ((itemcount / win->pd.ow_vrows) < win->pd.ow_vcols);


            /* find the objects associated with the items */
            if (winelem->v_type == V_WINTYPE_OBJITEM)
            {
                /* nethack may have been nice and passed an object pointer in menu_id_v
                 * Unforunately, we need this ugly hack to try to discern between
                 * chars, small ints and pointers */
                if (winelem->menu_id_v > (void*)0x10000)
                    winelem->pd.obj = (struct obj *)winelem->menu_id_v; 
                else if (win->id == WIN_INVEN)
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

    win->w = win->pd.ow_vcols * (V_LISTITEM_WIDTH + 4) - 4 + leftoffset + rightoffset;
    win->h = win->pd.ow_vrows * V_LISTITEM_HEIGHT + topoffset + vultures_winelem.border_bottom->h;

    if (ncols > win->pd.ow_vcols)
    {
        win->h += 25;

        winelem = vultures_create_button(win, NULL, V_INV_PREVPAGE);
        winelem->x = leftoffset;
        winelem->y = win->h - vultures_winelem.border_bottom->h - 23;
        winelem->w = 100;
        winelem->h = 24;
        winelem->image = vultures_get_img_src(0, 0, vultures_winelem.invarrow_left->w,
                                vultures_winelem.invarrow_left->h, vultures_winelem.invarrow_left);
        winelem->visible = 0;

        winelem = vultures_create_button(win, NULL, V_INV_NEXTPAGE);
        winelem->x = win->w - rightoffset - 101;
        winelem->y = win->h - vultures_winelem.border_bottom->h - 23;
        winelem->w = 100;
        winelem->h = 24;
        winelem->image = vultures_get_img_src(0, 0, vultures_winelem.invarrow_right->w,
                                vultures_winelem.invarrow_right->h, vultures_winelem.invarrow_right);
        winelem->visible = 1;
    }

    if (win->select_how != PICK_NONE)
    {
        int btn1_width = vultures_text_length(V_FONT_MENU, "Accept") + 14;
        int btn2_width = vultures_text_length(V_FONT_MENU, "Cancel") + 14;
        int max_width = (btn1_width > btn2_width) ? btn1_width : btn2_width;
        int total_width = 2 * max_width + 10;

        win->h += textheight + 14;

        winelem = vultures_create_button(win, "Accept", V_MENU_ACCEPT);
        winelem->h = textheight + 10;
        winelem->y = win->h - vultures_winelem.border_bottom->h - (textheight + 12);
        winelem->w = max_width;
        winelem->x = (win->w - rightoffset - leftoffset - total_width) / 2 + leftoffset;

        winelem = vultures_create_button(win, "Cancel", V_MENU_CANCEL);
        winelem->h = textheight + 10;
        winelem->y = win->h - vultures_winelem.border_bottom->h - (textheight + 12);
        winelem->w = max_width;
        winelem->x = (win->w - rightoffset - leftoffset - total_width) / 2 + leftoffset + max_width + 10;
    }
    else
    {
        winelem = vultures_create_button(win, NULL, V_INV_CLOSE);
        winelem->visible = 1;
        winelem->x = win->w - vultures_winelem.border_right->w - 28;
        winelem->y = vultures_winelem.border_top->h + 2;
        winelem->w = 26;
        winelem->h = 26;
        winelem->image = vultures_get_img_src(0, 0, vultures_winelem.closebutton->w,
                                vultures_winelem.closebutton->h, vultures_winelem.closebutton);
    }

    win->x = (win->parent->w - win->w) / 2;
    win->y = (win->parent->h - win->h) / 2;

    win->abs_x = win->parent->x + win->x;
    win->abs_y = win->parent->y + win->y;

}



/* arrange the buttons on a dropdown (context menu) */
void vultures_layout_dropdown(struct window *win)
{
    struct window * button;
    point mouse;
    int lineheight;
    int btn_maxwidth = 0;

    mouse = vultures_get_mouse_pos();

    win->h = 3;
    lineheight = vultures_get_lineheight(V_FONT_BUTTON);

    /* calculate the hight and find the max button width */
    button = win->first_child;
    while (button)
    {
        button->h = lineheight + 10;
        button->y = win->h;
        button->x = 3;
        win->h += button->h + 2;

        button->w = vultures_text_length(V_FONT_BUTTON, button->caption) + 11;
        btn_maxwidth = (btn_maxwidth > button->w) ? btn_maxwidth : button->w;

        button = button->sib_next;
    }
    win->h += 2;

    /* set the width of every button to the max button width */
    button = win->first_child;
    while (button)
    {
        button->w = btn_maxwidth;
        button = button->sib_next;
    }

    win->w = btn_maxwidth + 6;

    /* set dropdown position */
    if (mouse.x < win->parent->w - win->w)
        win->x = mouse.x;
    else
        win->x = win->parent->w - win->w;

    if (mouse.y < win->parent->h - win->h)
        win->y = mouse.y;
    else
        win->y = win->parent->h - win->h;

    win->abs_x = win->x + win->parent->abs_x;
    win->abs_y = win->y + win->parent->abs_y;
}


/* calc the with of a menu item */
int vultures_get_menuitem_width(struct window *item, int colwidths[8])
{
    int width, i, thiscol, btnwidth;
    char *sep, *start, *pos;
    char buf[256];

    /* get the width of the radiobutton/checkbox + 4px space */
    btnwidth = 0;
    if (item->v_type == V_WINTYPE_OPTION)
        switch (item->parent->select_how)
        {
            case PICK_NONE: btnwidth = vultures_winelem.radiobutton_off->w + 4; break;
            case PICK_ONE:  btnwidth = vultures_winelem.radiobutton_off->w + 4; break;
            case PICK_ANY:  btnwidth = vultures_winelem.checkbox_off->w + 4;    break;
        }

    /* if the caption contains a tab character we have items for multiple columns */
    if (strchr(item->caption, '\t'))
    {
        width = 0;
        i = 0;
        start = item->caption;
        sep = strchr(start, '\t');

        /* for each item  */
        while (sep && i < 7)
        {
            strncpy(buf, start, sep-start);
            buf[sep-start] = '\0';
            pos = &buf[sep-start-1];

            /* strip spaces */
            while (isspace(*pos))
                *pos-- = '\0';

            /* get the item's width */
            thiscol = vultures_text_length(V_FONT_MENU, buf) + 5;

            if (i == 0)
                thiscol += btnwidth;

            /* adjust the column with to fit, if necessary */
            if (thiscol > colwidths[i])
                colwidths[i] = thiscol;

            width += colwidths[i] + 20;
            start = sep+1;
            sep = strchr(start, '\t');
            i++;
        }
        width += vultures_text_length(V_FONT_MENU, start);
    }
    else
        width = vultures_text_length(V_FONT_MENU, item->caption) + btnwidth;


    return width;
}



void vultures_layout_menu(struct window *win)
{
    struct window *winelem, *tmp, *coltxt;
    char *start, *sep;
    int i;

    int btncount = 0, btn_maxwidth = 0, btn_totalwidth = 0;
    int need_scrollbar = 0, enlarge_buttons = 0;
    int elem_maxwidth = 0;
    int saved_scrollpos = 0;
    int pos_x = 0;

    int height = 0;
    int width = 0;
    int offset_left = vultures_winelem.border_left->w;
    int offset_top = vultures_winelem.border_top->h;
    int colwidths[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int colstart[8] = {0, 0, 0, 0, 0, 0, 0, 0};


    if (win->caption)
    {
        offset_top += 2 * vultures_text_height(V_FONT_HEADLINE, win->caption);
        width = vultures_text_length(V_FONT_HEADLINE, win->caption);
    }

    winelem = win->first_child;
    while (winelem)
    {
        if (winelem->v_type == V_WINTYPE_SCROLLBAR)
        {
            saved_scrollpos = winelem->pd.scrollpos;
            tmp = winelem;
            winelem = winelem->sib_next;
            vultures_destroy_window_internal(tmp);
            continue;
        }

        winelem->w = vultures_get_menuitem_width(winelem, colwidths);
        winelem->h = vultures_text_height(V_FONT_MENU, winelem->caption);

        switch (winelem->v_type)
        {
            case V_WINTYPE_OPTION:
            case V_WINTYPE_TEXT:
                if (!win->content_is_text)
                    winelem->h += 10;

                winelem->x = offset_left;
                winelem->y = height + offset_top;

                height += winelem->h + 4;
                elem_maxwidth = (elem_maxwidth > winelem->w) ? elem_maxwidth : winelem->w;

                break;

            /* count buttons and calculate widths; dont assign positions yet */
            case V_WINTYPE_BUTTON:
                btncount++;

                winelem->w += 14;
                winelem->h += 10;

                btn_maxwidth = (btn_maxwidth > winelem->w) ? btn_maxwidth : winelem->w;
                btn_totalwidth += winelem->w + 10;

                break;

            default: break;
        }

        if (height > V_MENU_MAXHEIGHT)
            need_scrollbar = 1;

        winelem = winelem->sib_next;
    }


    /* add up the widths of all the columns and determine the starting position of each column */
    for (i = 1; i < 8; i++)
    {
        colstart[i] = colstart[i-1] + colwidths[i-1];
        if (colstart[i] != colstart[i-1])
            colstart[i] += 16;
    }
    elem_maxwidth = (elem_maxwidth > (colstart[7] + colwidths[7])) ? 
                     elem_maxwidth : (colstart[7] + colwidths[7]);
    elem_maxwidth += 5;

    /* no need for spacing after the last button */
    btn_totalwidth -= 10;

    width = (width > elem_maxwidth) ? width : elem_maxwidth;
    elem_maxwidth = width;

    if (width < btn_totalwidth)
        width = btn_totalwidth;

    /* add the scrollbar */
    if (need_scrollbar)
    {
        winelem = vultures_create_window_internal(0, win, V_WINTYPE_SCROLLBAR);
        winelem->w = vultures_winelem.scrollbar->w;
        winelem->h = V_MENU_MAXHEIGHT;
        winelem->x = offset_left + width + 5;
        winelem->y = offset_top;
        winelem->pd.scrollpos = saved_scrollpos;

        width += winelem->w + 5;
        height = V_MENU_MAXHEIGHT;
    }

    /* if the menu elements are large enough, we enlarge all the
     * buttons to be equally wide */
    if (width >= (btn_maxwidth + 10) * btncount - 10)
    {
        enlarge_buttons = 1;
        btn_totalwidth = (btn_maxwidth + 10) * btncount - 10;
    }

    /* assign positions to all the buttons */
    pos_x = (width - btn_totalwidth) / 2;
    winelem = win->first_child;
    while (winelem)
    {
        if (winelem->v_type == V_WINTYPE_BUTTON)
        {
            if (enlarge_buttons)
                winelem->w = btn_maxwidth;

            winelem->x = pos_x + offset_left;
            pos_x += winelem->w + 10;
            winelem->y = height + offset_top + 1;
        }
        else if (winelem->v_type == V_WINTYPE_OPTION)
            winelem->w = elem_maxwidth;

        if (winelem->caption)
            start = strchr(winelem->caption, '\t');
        else
            start = NULL;

        if (start && winelem->scrollable)
        {
            *start++ = '\0';
            sep = strchr(start, '\t');
            i = 1;
            
            while (sep && i < 7)
            {
                coltxt = vultures_create_window_internal(0, win, V_WINTYPE_TEXT);
                coltxt->caption = (char *)malloc(sep-start+1);
                strncpy(coltxt->caption, start, sep-start);
                coltxt->caption[sep-start] = '\0';
                coltxt->w = 1;
                coltxt->h = winelem->h;
                coltxt->x = colstart[i] + winelem->x;
                coltxt->y = winelem->y;
                coltxt->scrollable = 1;

                start = sep;
                *start++ = '\0';
                sep = strchr(start, '\t');
                i++;
            }

            coltxt = vultures_create_window_internal(0, win, V_WINTYPE_TEXT);
            coltxt->caption = (char *)malloc(strlen(start)+1);
            strcpy(coltxt->caption, start);
            coltxt->w = 1;
            coltxt->h = winelem->h;
            coltxt->x = colstart[i] + winelem->x;
            coltxt->y = winelem->y;
            coltxt->scrollable = 1;
        }

        winelem = winelem->sib_next;
    }

    height += 15 + vultures_get_lineheight(V_FONT_MENU);
    
    win->h = height + offset_top + vultures_winelem.border_bottom->h;
    win->w = width + offset_left + vultures_winelem.border_right->w;
    
    win->x = (win->parent->w - win->w) / 2;
    win->y = (win->parent->h - win->h) / 2;
}



/*********************************
 * misc utility functions
 *********************************/

struct window * vultures_walk_winlist(struct window * win, int * descend)
{
    /* 1) try to descend to child */
    if (*descend && win->first_child)
        return win->first_child;

    /* 2) try sibling*/
    if (win->sib_next)
    {
        *descend = 1;
        return win->sib_next;
    }

    /* 3) ascend to parent and set *descend = 0 to prevent infinite loops */
    *descend = 0;
    return win->parent;
}


/* find the window under the mouse, starting from topwin */
struct window * vultures_get_window_from_point(struct window * topwin, point mouse)
{
    struct window *winptr, *nextwin;

    winptr = topwin;

    /* as each child window is completely contained by its parent we can descend
     * into every child window that is under the cursor until no further descent
     * is possible. The child lists are traversed in reverse because newer child
     * windows are considered to be on top if child windows overlap */
    while (winptr->last_child)
    {
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


/* check whether the map needs recentering */
int vultures_need_recenter(int map_x, int map_y)
{
    int screen_x, screen_y;

    map_x -= vultures_view_x;
    map_y -= vultures_view_y;
    screen_x = vultures_screen->w/2 + V_MAP_XMOD*(map_x - map_y);
    screen_y = vultures_screen->h/2 + V_MAP_YMOD*(map_x + map_y);

    /* if the player is within the outer 1/4 of the screen, the map needs recentering  */
    if ((screen_x >= vultures_screen->w/4) && (screen_x < vultures_screen->w * 3 / 4) &&
        (screen_y >= vultures_screen->h/4) && (screen_y < vultures_screen->h * 3 / 4))
        return 1;

    return 0;
}


extern const char *hu_stat[];           /* defined in eat.c */
extern const char * const enc_stat[];   /* defined in botl.c */

void vultures_parse_statusline(struct window * statuswin, const char * str)
{
    int i, j, len, hp, hpmax, nconds;
    long val;
    static struct window * tokenarray[5][5];
    static int array_inited = 0;
    static int const status_xpos[5] = { 0, 60, 100, 180, 250};
    char * ptr;
    int cap = near_capacity();

    if (!array_inited)
    {
        for (i = 0; i < 5; i++)
            for (j = 0; j < 5; j++)
            {
                tokenarray[i][j] = vultures_create_window_internal(0, statuswin, V_WINTYPE_TEXT);
                tokenarray[i][j]->autobg = 1;
                tokenarray[i][j]->x = 3 + status_xpos[i];
                tokenarray[i][j]->y = 2 + j*vultures_get_lineheight(V_FONT_STATUS);
                tokenarray[i][j]->w = 100;
                tokenarray[i][j]->h = vultures_get_lineheight(V_FONT_STATUS);
                tokenarray[i][j]->caption = (char *)malloc(64);
                tokenarray[i][j]->caption[0] = '\0';
            }

        /* the player is longer than everything else */
        tokenarray[0][0]->w = 250;
    }

    array_inited = 1;


    /* get player name + title */
    ptr = strstr(str, "St:");
    if (ptr)
    {
        len = ptr - str;
        strncpy(tokenarray[0][0]->caption, str, len);
        tokenarray[0][0]->caption[len] = '\0';
    }

    /* strength needs special treatment */
    ptr = tokenarray[0][1]->caption;
    if (ACURR(A_STR) > 18) 
    {
        if (ACURR(A_STR) > STR18(100))
            sprintf(ptr,"St:%2d", ACURR(A_STR)-100);
        else if (ACURR(A_STR) < STR18(100))
            sprintf(ptr, "St:18/%02d", ACURR(A_STR)-18);
        else
            sprintf(ptr,"St:18/**");
    }
    else
        sprintf(ptr, "St:%-1d", ACURR(A_STR));

    /* the other stats */
    sprintf(tokenarray[0][2]->caption, "Dx:%-1d", ACURR(A_DEX));
    sprintf(tokenarray[0][3]->caption, "Co:%-1d", ACURR(A_CON));
    sprintf(tokenarray[1][1]->caption, "In:%-1d", ACURR(A_INT));
    sprintf(tokenarray[1][2]->caption, "Wi:%-1d", ACURR(A_WIS));
    sprintf(tokenarray[1][3]->caption, "Ch:%-1d", ACURR(A_CHA));

    /* alignment */
    tokenarray[4][0]->visible = 1;
    sprintf(tokenarray[4][0]->caption, (u.ualign.type == A_CHAOTIC) ? "Chaotic" :
            (u.ualign.type == A_NEUTRAL) ? "Neutral" : "Lawful");

    /* score */
#ifdef SCORE_ON_BOTL
    if (flags.showscore)
        sprintf(tokenarray[3][4]->caption, "S:%ld", botl_score());
    else
        tokenarray[3][4]->caption[0] = '\0';
#endif

    /* money */
#ifndef GOLDOBJ
        val = u.ugold;
#else
        val = money_cnt(invent);
#endif
    if (val >= 100000)
        sprintf(tokenarray[3][2]->caption, "%c:%-2ldk", oc_syms[COIN_CLASS], val / 1000);
    else
        sprintf(tokenarray[3][2]->caption, "%c:%-2ld", oc_syms[COIN_CLASS], val);

    /* Experience */
    if (Upolyd)
        Sprintf(tokenarray[3][0]->caption, "HD:%d", mons[u.umonnum].mlevel);
#ifdef EXP_ON_BOTL
    else if(flags.showexp)
    {
        Sprintf(tokenarray[3][0]->caption, "Xp:%u/%-1ld", u.ulevel,u.uexp);
        /* if the exp gets too long, suppress displaying the alignment */
        if (strlen(tokenarray[3][0]->caption) > 10)
            tokenarray[4][0]->visible = 0;
    }
#endif
    else
        Sprintf(tokenarray[3][0]->caption, "Exp:%u", u.ulevel);


    /* HP, energy, armor */
    hp = Upolyd ? u.mh : u.uhp;
    hpmax = Upolyd ? u.mhmax : u.uhpmax;
    sprintf(tokenarray[2][1]->caption, "HP:%d(%d)", hp, hpmax);
    if (hp >= ((hpmax * 90) / 100))
        tokenarray[2][1]->pd.textcolor = vultures_warn_colors[V_WARN_NONE];
    else if (hp >= ((hpmax * 70) / 100))
        tokenarray[2][1]->pd.textcolor = vultures_warn_colors[V_WARN_NORMAL];
    else if (hp >= ((hpmax * 50) / 100))
        tokenarray[2][1]->pd.textcolor = vultures_warn_colors[V_WARN_MORE];
    else if (hp >= ((hpmax * 25) / 100))
        tokenarray[2][1]->pd.textcolor = vultures_warn_colors[V_WARN_ALERT];
    else
        tokenarray[2][1]->pd.textcolor = vultures_warn_colors[V_WARN_CRITICAL];
    sprintf(tokenarray[2][2]->caption, "Pw:%d(%d)", u.uen, u.uenmax);
    sprintf(tokenarray[2][3]->caption, "AC:%-2d", u.uac);

    /* time */
    if(flags.time)
        sprintf(tokenarray[3][3]->caption, "T:%ld", moves);
    else
        tokenarray[3][3]->caption[0] = '\0';

    /* depth again (numeric) */
    sprintf(tokenarray[3][1]->caption, "Dlvl:%-2d ", depth(&u.uz));

    /* conditions (hunger, confusion, etc) */
    nconds = 0;

    if (u.uhs > 1) /* hunger */
        vultures_status_add_cond(hu_stat[u.uhs], nconds++, u.uhs-1, tokenarray);
    else if (u.uhs < 1) /* satiated */
        vultures_status_add_cond(hu_stat[u.uhs], nconds++, 0, tokenarray);

    if(Confusion)
        vultures_status_add_cond("Conf", nconds++, V_WARN_MORE, tokenarray);

    if(Sick)
    {
        if (u.usick_type & SICK_VOMITABLE)
            vultures_status_add_cond("FoodPois", nconds++, V_WARN_ALERT, tokenarray);
        if (u.usick_type & SICK_NONVOMITABLE)
            vultures_status_add_cond("Ill", nconds++, V_WARN_ALERT, tokenarray);
    }

    if(Blind)          vultures_status_add_cond("Blind", nconds++, V_WARN_MORE, tokenarray);
    if(Stunned)        vultures_status_add_cond("Stun",  nconds++, V_WARN_MORE, tokenarray);
    if(Hallucination)  vultures_status_add_cond("Hallu", nconds++, V_WARN_MORE, tokenarray);
    if(Slimed)         vultures_status_add_cond("Slime", nconds++, V_WARN_ALERT, tokenarray);

    if(cap > UNENCUMBERED)
        vultures_status_add_cond(enc_stat[cap], nconds++, cap, tokenarray);

    /* reset the empty positions */
    for ( ;nconds < 8; nconds++)
        vultures_status_add_cond("", nconds, 0, tokenarray);

#ifdef SHOW_WEIGHT
    if (flags.showweight && !tokenarray[0][4]->caption[0])
        sprintf(tokenarray[0][4]->caption, "Wt:%ld/%ld", (long)(inv_weight()+weight_cap()),
            (long)weight_cap());
#endif

}


static void vultures_status_add_cond(const char * str, int warnno, int color, struct window * tarray[5][5])
{
    static const point pos[9] = {{4,1}, {4,2}, {4,3}, {4,4}, {3,4}, {2,4}, {1,4}, {0,4}, {4,0}};
    if (warnno >= 9)
        return;

    strcpy(tarray[pos[warnno].x][pos[warnno].y]->caption, str);
    tarray[pos[warnno].x][pos[warnno].y]->pd.textcolor = vultures_warn_colors[color];
}


/* enable and display the enhance icon if enhance is possible */
void vultures_check_enhance(void)
{
    int to_advance = 0, i, prev_vis;
    struct window * win;

    /* check every skill */
    for (i = 0; i < P_NUM_SKILLS; i++)
    {
        if (P_RESTRICTED(i))
            continue;
        if (can_advance(i, FALSE))
            to_advance++;
    }

    win = vultures_get_window(vultures_winid_enhance);
    prev_vis = win->visible;

    if (!to_advance && prev_vis)
        vultures_hide_window(win);
    else if (!win->visible && to_advance)
        win->visible = 1;
}



/* windows which have autobg set store an image of the screen behind them, so
 * that the screen can easily be restored when the window is hidden or destroyed.
 * if a window behind the current window is updated, the current window must
 * refresh its stored background */
void vultures_update_background(struct window * win)
{
    int i;
    int x1, y1, x2, y2;
    SDL_Rect src, dst;

    /* no stored background and autobg is off: do nothing */
    if (!win->background && !win->autobg)
        return;

    /* no background stored yet: copy a surface that is as large as the window */
    if (!win->background)
    {
        win->background = vultures_get_img(win->abs_x, win->abs_y,
                              win->abs_x + win->w - 1, win->abs_y + win->h - 1);
        return;
    }

    /* find the intersection between all invalid regions and the window so that
     * only actually invalid parts of the background get updated */
    for (i = 0; i < vultures_invrects_num; i++)
    {
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
        {
            src.x = x1;
            src.y = y1;
            src.w = x2 - x1;
            src.h = y2 - y1;

            dst.x = x1 - win->abs_x;
            dst.y = y1 - win->abs_y;
            dst.w = src.w;
            dst.h = src.h;

            SDL_BlitSurface(vultures_screen, &src, win->background, &dst);
        }
    }
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
int vultures_intersects_invalid(struct window * win)
{
    int i;
    int x1, y1, x2, y2;
    for (i = 0; i < vultures_invrects_num; i++)
    {
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



void vultures_messages_add(const char * str)
{
    if (!vultures_messages_buf)
    {
        /* init if necessary */
        vultures_messages_buf = (char **)malloc(V_MESSAGEBUF_SIZE * sizeof(char*));
        memset(vultures_messages_buf, 0, V_MESSAGEBUF_SIZE * sizeof(char*));

        vultures_messages_ages = (int *)malloc(V_MESSAGEBUF_SIZE * sizeof(int));
        vultures_messages_top = -1;
        vultures_messages_cur = -1;
    }

    vultures_messages_top = (vultures_messages_top + 1) % V_MESSAGEBUF_SIZE;
    vultures_messages_cur = vultures_messages_top;

    /* if we just wrapped around, free the old entry */
    if(vultures_messages_buf[vultures_messages_top])
        free(vultures_messages_buf[vultures_messages_top]);

    vultures_messages_buf[vultures_messages_top] = strdup(str);
    vultures_messages_ages[vultures_messages_top] = moves;
}


void vultures_messages_setshown(int first)
{
    vultures_messages_cur = (vultures_messages_top - first + V_MESSAGEBUF_SIZE) % V_MESSAGEBUF_SIZE;
}


int vultures_messages_getshown()
{
    return (vultures_messages_top - vultures_messages_cur + V_MESSAGEBUF_SIZE) % V_MESSAGEBUF_SIZE;
}


/* retrieve a message from the message buffer, offset messages before the current one */
char * vultures_messages_get(int offset, int * age)
{
    if (!vultures_messages_buf || offset >= V_MESSAGEBUF_SIZE)
        return NULL;

    int msg_id = (vultures_messages_cur - offset + V_MESSAGEBUF_SIZE) % V_MESSAGEBUF_SIZE;

    if (vultures_messages_buf[msg_id])
    {
        *age = vultures_messages_ages[msg_id];
        return vultures_messages_buf[msg_id];
    }

    *age = 0;
    return NULL;
}


void vultures_messages_destroy(void)
{
    int i;
    if (!vultures_messages_buf)
        return;

    for (i = 0; i < V_MESSAGEBUF_SIZE; i++)
        if (vultures_messages_buf[i])
            free(vultures_messages_buf[i]);

    free(vultures_messages_buf);
    free(vultures_messages_ages);
}


void vultures_messages_view(void)
{
    int offset, time, winid;
    char * message;
    char menuline[256];

    winid = vultures_create_nhwindow(NHW_MENU);

    offset = - vultures_messages_getshown();
    while ( (message = vultures_messages_get(offset, &time)) )
    {
        sprintf(menuline, "T:%d %s", time, message);
        vultures_putstr(winid, ATR_NONE, menuline);
        offset++;
    }

    /* Display the messages */
    vultures_display_nhwindow(winid, TRUE);

    /* Clean up */
    vultures_destroy_nhwindow(winid);
}



/* find the window that has the accelerator "accel" */
struct window * vultures_accel_to_win(struct window * parent, char accel)
{
    struct window * child = parent->first_child;

    while (child)
    {
        if (child->accelerator == accel)
            return child;

        child = child->sib_next;
    }

    return NULL;
}


/* resize the vultures application window to the given width and height */
void vultures_win_resize(int width, int height)
{
    struct window *current, *topwin;
    SDL_Event event;
    vultures_event dummy;
    int descend = 1;

    current = topwin = vultures_get_window(0);

    event.type = SDL_VIDEORESIZE;
    event.resize.w = width;
    event.resize.h = height;

    current->event_handler(current, current, &dummy, &event);

    do
    {
        current = vultures_walk_winlist(current, &descend);

        if (current->event_handler)
            current->event_handler(current, current, &dummy, &event);
        else if (current->parent->event_handler)
            current->parent->event_handler(current->parent, current, &dummy, &event);

        /* recalc absolute position */
        if (current->v_type != V_WINTYPE_NONE)
        {
            current->abs_x = current->parent->abs_x + current->x;
            current->abs_y = current->parent->abs_y + current->y;
        }
    }
    while (current != topwin);

    /* redraw everything */
    vultures_map_force_redraw();
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
