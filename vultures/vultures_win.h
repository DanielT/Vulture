/* Copyright (c) Daniel Thaler, 2006, 2008				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_win_h_
#define _vultures_win_h_

#include "vultures_types.h"
#include <string>
using std::string;

class window;

/* 
* External files used by the GUI.
* Note: The intro sequence may use other
* external images, listed in the script file.
*/
#define V_MAX_FILENAME_LENGTH 1024

#define V_FILENAME_INTRO_SCRIPT  "vultures_intro.txt"
#define V_FILENAME_OPTIONS       "vultures.conf"
#define V_FILENAME_SOUNDS_CONFIG "vultures_sounds.conf"

/*
* graphics files, without ".png" extension
*/
#ifdef VULTURESEYE
#define V_FILENAME_NETHACK_LOGO         "nh_ve_1"
#define V_FILENAME_MAP_SYMBOLS          "nh_tiles"
#endif
#ifdef VULTURESCLAW
#define V_FILENAME_NETHACK_LOGO         "se_vc_1"
#define V_FILENAME_MAP_SYMBOLS          "se_tiles"
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
} vultures_window_graphics;


typedef struct event {
	int x, y;
	int num;
	int rtype;
} vultures_event;


/* exported functions */

/* high-level window functions */
extern void vultures_messagebox(string message);
extern int vultures_get_input(int x, int y, const char *ques, char *input);


/* drawing functions */
extern void vultures_refresh_window_region(void);
extern void vultures_invalidate_region(int, int, int, int);


/* eventstack handling */
extern void vultures_eventstack_add(int, int, int, int);
extern vultures_event * vultures_eventstack_get(void);
extern void vultures_eventstack_destroy(void);


/* event handling */
extern void vultures_event_dispatcher(void * result, int resulttype, window * topwin);
extern int vultures_event_dispatcher_nonblocking(void * result, window * topwin);

/* misc functions */
extern void vultures_win_resize(int width, int height);
extern void vultures_show_mainmenu(void);


/* exported variables */
extern vultures_window_graphics vultures_winelem;
extern int vultures_suppress_helpmsg;
extern int vultures_whatis_singleshot;
extern int vultures_windows_inited;



extern SDL_Rect *vultures_invrects;
extern int vultures_invrects_num;
extern int vultures_invrects_max;


#endif
/* End of vultures_win.h */
