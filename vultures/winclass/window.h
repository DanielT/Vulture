#ifndef _window_h_
#define _window_h_


#include <SDL.h>


typedef enum {
    V_WINTYPE_NONE, /* only the root window has this type */
    V_WINTYPE_MAIN,
    V_WINTYPE_BUTTON,
    V_WINTYPE_OPTION,
    V_WINTYPE_SCROLLBAR,
    V_WINTYPE_TEXT,
    V_WINTYPE_DROPDOWN,
    
    /* new-style inventory and object lists (ie multidrop, pickup, loot) */
    V_WINTYPE_OBJWIN,
    V_WINTYPE_OBJITEM,
    V_WINTYPE_OBJITEMHEADER,
    
    V_WINTYPE_CUSTOM
} window_type;


typedef enum {
    V_EVENT_UNHANDLED,
    V_EVENT_UNHANDLED_REDRAW, /* pass the event on to a parent win and redraw */
    V_EVENT_HANDLED_NOREDRAW, /* don't pass it on and don't redraw */
    V_EVENT_HANDLED_REDRAW,   /* don't pass it on and redraw */
    V_EVENT_HANDLED_FINAL     /* redraw and leave the event dispatcher */
} eventresult;




class window
{

public:
	window(window *p, int nh_wt, window_type wt);

	virtual int draw() = 0;
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event) = 0;

protected:
    int id;
    int nh_type;                   /* type assigned by nethack */
    window_type v_type;
    char * caption;
	
	window *parent;
	window *sib_next, *sib_prev;
	window *first_child, *last_child;

    /* absolute coords; calculated before drawing */
    int abs_x, abs_y;

    /* coords relative to parent */
    int x, y;
    int w, h;

	bool need_redraw : 1;
	bool visible : 1;

};

#endif
