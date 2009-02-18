/* NetHack may be freely redistributed.  See license for details. */

#ifndef _window_h_
#define _window_h_

#include <string>
#include <SDL.h>
#include "vultures_types.h"

#define V_LISTITEM_WIDTH  300
#define V_LISTITEM_HEIGHT  52


typedef enum {
	V_WINTYPE_NONE, /* only the root window has this type */
	V_WINTYPE_LEVEL,
	V_WINTYPE_MAP,
	V_WINTYPE_MAIN,
	V_WINTYPE_MENU,
	V_WINTYPE_BUTTON,
	V_WINTYPE_OPTION,
	V_WINTYPE_SCROLLBAR,
	V_WINTYPE_TEXT,
	V_WINTYPE_CONTEXTMENU,
	V_WINTYPE_SCROLLAREA,
	
	/* new-style inventory and object lists (ie multidrop, pickup, loot) */
	V_WINTYPE_OBJWIN,
	V_WINTYPE_OBJITEM,
	V_WINTYPE_OBJITEMHEADER,
	
	V_WINTYPE_ENDING,
	V_WINTYPE_CUSTOM
} window_type;


typedef enum {
	V_EVENT_UNHANDLED,
	V_EVENT_UNHANDLED_REDRAW, /* pass the event on to a parent win and redraw */
	V_EVENT_HANDLED_NOREDRAW, /* don't pass it on and don't redraw */
	V_EVENT_HANDLED_REDRAW,   /* don't pass it on and redraw */
	V_EVENT_HANDLED_FINAL     /* redraw and leave the event dispatcher */
} eventresult;



enum hotspots {
	V_HOTSPOT_NONE = 0,
	/* child windows of the map */
	V_HOTSPOT_SCROLL_UPLEFT,
	V_HOTSPOT_SCROLL_UP,
	V_HOTSPOT_SCROLL_UPRIGHT,
	V_HOTSPOT_SCROLL_LEFT,
	V_HOTSPOT_SCROLL_RIGHT,
	V_HOTSPOT_SCROLL_DOWNLEFT,
	V_HOTSPOT_SCROLL_DOWN,
	V_HOTSPOT_SCROLL_DOWNRIGHT,
	V_WIN_MINIMAP,
	V_WIN_STATUSBAR,
	V_WIN_TOOLBAR1,
	V_WIN_TOOLBAR2,
	V_WIN_ENHANCE,

	/* child windows of the statusbar */
	V_HOTSPOT_BUTTON_LOOK,
	V_HOTSPOT_BUTTON_EXTENDED,
	V_HOTSPOT_BUTTON_MAP,
	V_HOTSPOT_BUTTON_SPELLBOOK,
	V_HOTSPOT_BUTTON_INVENTORY,
	V_HOTSPOT_BUTTON_DISCOVERIES,
	V_HOTSPOT_BUTTON_MESSAGES,
	V_HOTSPOT_BUTTON_OPTIONS,
	V_HOTSPOT_BUTTON_IFOPTIONS,
	V_HOTSPOT_BUTTON_HELP,

	V_MENU_ACCEPT,
	V_MENU_CANCEL = -1,

	V_INV_NEXTPAGE,
	V_INV_PREVPAGE,
	V_INV_CLOSE
};



class window
{

public:
	window(window *p);
	virtual ~window();

	virtual bool draw() = 0;
	virtual void set_caption(std::string str);
	virtual void hide();
	virtual void layout() {};
	virtual void update_background(void);
	
	eventresult event_handler(window* target, void* result, SDL_Event* event);
	virtual eventresult handle_timer_event(window* target, void* result, int time);
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_keydown_event(window* target, void* result,
	                                         int sym, int mod, int unicode);
	virtual eventresult handle_resize_event(window* target, void* result, int res_w, int res_h);
	virtual eventresult handle_other_event(window* target, void* result, SDL_Event* event);
	
	window_type get_wintype() { return v_type; };
	void *get_menu_id() { return menu_id_v; };
	
	void draw_windows();
	window *walk_winlist(bool *descend);
	window *get_window_from_point(point mouse);
	bool intersects_invalid();

	int get_w() { return w; }
	int get_h() { return h; }

	bool need_redraw;
	bool visible;
	
	virtual window* find_accel(char accel);


// protected:
	window_type v_type;
  std::string caption;
	char accelerator;


	/* absolute coords; calculated before drawing */
	int abs_x, abs_y;

	/* coords relative to parent */
	int x, y;
	int w, h;

	
	union {
		void *menu_id_v;
		int menu_id;
	};
	
	window *first_child, *last_child;
	window *parent;
	window *sib_next, *sib_prev;
	
protected:
	bool autobg;
	SDL_Surface *background;
};

extern window *ROOTWIN;

#endif
