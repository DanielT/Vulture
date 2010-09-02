/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vulture_win_h_
#define _vulture_win_h_

#include "vulture_types.h"
#include <string>

class window;

/* 
* External files used by the GUI.
* Note: The intro sequence may use other
* external images, listed in the script file.
*/
#define V_MAX_FILENAME_LENGTH 1024

#define V_FILENAME_INTRO_SCRIPT  "vulture_intro.txt"
#define V_FILENAME_OPTIONS       "vulture.conf"
#define V_FILENAME_SOUNDS_CONFIG "vulture_sounds.conf"

/*
* graphics files, without ".png" extension
*/
#define V_FILENAME_LOGO                 "logo"
#ifdef VULTURE_NETHACK
#define V_FILENAME_MAP_SYMBOLS          "nethack_tiles"
#endif
#ifdef VULTURE_SLASHEM
#define V_FILENAME_MAP_SYMBOLS          "slashem_tiles"
#endif
#ifdef VULTURE_UNNETHACK
#define V_FILENAME_MAP_SYMBOLS          "unnethack_tiles"
#endif
#ifndef V_FILENAME_MAP_SYMBOLS
#error Variant?
#endif
#define V_FILENAME_CHARACTER_GENERATION "chargen2"
#define V_FILENAME_FONT                 "VeraSe.ttf"
#define V_FILENAME_WINDOW_STYLE         "winelem"
#define V_FILENAME_MAP_PARCHMENT        "parchment"

/* how many ms must the mouse remain stationary until a tooltip is displayed */
#define HOVERTIMEOUT 400


enum responses {
	V_RESPOND_ANY,
	V_RESPOND_CHARACTER,
	V_RESPOND_INT,
	V_RESPOND_POSKEY
};


typedef struct {
	SDL_Surface *border_top;
	SDL_Surface *border_bottom;
	SDL_Surface *border_left;
	SDL_Surface *border_right;
	SDL_Surface *corner_tl;
	SDL_Surface *corner_tr;
	SDL_Surface *corner_bl;
	SDL_Surface *corner_br;
	SDL_Surface *center;
	SDL_Surface *radiobutton_on;
	SDL_Surface *radiobutton_off;
	SDL_Surface *checkbox_on;
	SDL_Surface *checkbox_count;
	SDL_Surface *checkbox_off;
	SDL_Surface *scrollbar;
	SDL_Surface *scrollbutton_up;
	SDL_Surface *scrollbutton_down;
	SDL_Surface *scroll_indicator;
	SDL_Surface *direction_arrows;
	SDL_Surface *invarrow_left;
	SDL_Surface *invarrow_right;
	SDL_Surface *closebutton;
} vulture_window_graphics;


typedef struct event {
	int x, y;
	int num;
	int rtype;
} vulture_event;


/* exported functions */

/* high-level window functions */
extern void vulture_messagebox(std::string message);
extern int vulture_get_input(int x, int y, const char *ques, char *input);


/* drawing functions */
extern void vulture_refresh_window_region(void);
extern void vulture_invalidate_region(int, int, int, int);


/* eventstack handling */
extern void vulture_eventstack_add(int, int, int, int);
extern vulture_event * vulture_eventstack_get(void);
extern void vulture_eventstack_destroy(void);


/* event handling */
extern void vulture_event_dispatcher(void * result, int resulttype, window * topwin);
extern int vulture_event_dispatcher_nonblocking(void * result, window * topwin);

/* misc functions */
extern void vulture_win_resize(int width, int height);
extern void vulture_show_mainmenu(void);


/* exported variables */
extern vulture_window_graphics vulture_winelem;
extern int vulture_suppress_helpmsg;
extern int vulture_whatis_singleshot;
extern int vulture_windows_inited;



extern SDL_Rect *vulture_invrects;
extern int vulture_invrects_num;
extern int vulture_invrects_max;


#endif
/* End of vulture_win.h */
