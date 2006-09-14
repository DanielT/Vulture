/*	SCCS Id: @(#)jtp_win.h	1.0	2000/9/10	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_win_h_
#define _jtp_win_h_

#include "hack.h"
#include "jtp_def.h"

/* Graphics constants - these should really not be here */
#define JTP_MAX_SHADES 64 

/* Special window content types */
#define JTP_WINCONTENT_EMPTY -1
#define JTP_WINCONTENT_GLYPH_UNEXPLORED -1
#define JTP_WINCONTENT_GLYPH_NOT_VISIBLE -2

/* Menu handling constants */
#define JTP_NOT_SELECTABLE -12345

/*
 * Key constants for non-ascii keys; the platform-dependent 
 * code must make sure these are assigned correctly
 */
#define JTP_KEY_MENU_SCROLLUP -10
#define JTP_KEY_MENU_SCROLLDOWN -11
#define JTP_KEY_MENU_SCROLLLEFT -12
#define JTP_KEY_MENU_SCROLLRIGHT -13
#define JTP_KEY_MENU_SCROLLPAGEUP -14
#define JTP_KEY_MENU_SCROLLPAGEDOWN -15
#define JTP_KEY_PAUSE -16
#define JTP_KEY_INSERT -17
#define JTP_KEY_HOME -18
#define JTP_KEY_END -19

/* Maximum number of old messages stored by message window */
#define JTP_MAX_OLD_MESSAGES 50

/* Maximum number of messages displayed on-screen */
#define JTP_MAX_SHOWN_MESSAGES 3

/* Autopilot types */
#define JTP_AUTOPILOT_MOVEMENT 0
#define JTP_AUTOPILOT_WHATSTHIS 1

/* JTP specific structure definitions */
typedef struct {
  void * itemdata;
  void * next;
  void * previous;
} jtp_listitem;

typedef struct {
  jtp_listitem * previous;
  jtp_listitem * header;
  int length;
} jtp_list;

typedef struct {
  char * text;                               /* Item description */
  anything id;                               /* Identifier of item */
  char accelerator;                          /* Key accelerator for the item */
  int glyph;                                 /* NetHack glyph for the item */
  int selected;                              /* Is the item selected? */
  int count;                                 /* Number of times selected */
  int x, y, width, height;                   /* Position and dimensions, in pixels */
} jtp_menuitem;

typedef struct {
  char * prompt;                             /* Prompt message */
  int prompt_x, prompt_y;                    /* Position of prompt message in window */
  int need_scrollbar;                        /* Is a scrollbar necessary? 1=yes, 0=no */
  int scrollbar_x, scrollup_y, scrolldown_y; /* Position of scrollbar, if applicable */
  int items_y;                               /* Position of content items in window */
  jtp_list * items;                          /* Menu items or text lines */
  int selectiontype;                         /* PICK_NONE, PICK_ONE, or PICK_MANY */
  int content_is_text;                       /* 1=Text display, 0=menu display */
} jtp_menu;

typedef struct {
  winid id;                  /* The window id, for example, WIN_STATUS */
  int active;                /* Is the window active or dormant? */
  int wintype;               /* The window type, for example, NHW_MENU */
  int ending_type;           /* Is the NHW_TEXT window an 'end screen' for some ending type? */
  int x, y;                  /* Position of upper left corner on-screen, in pixels */
  int width, height;         /* Window dimensions, in pixels */
  int curs_x, curs_y;        /* Location of text cursor in window content, in characters */
  int curs_rows, curs_cols;  /* Window content dimensions, in characters */
  int **rows;                /* Window content, indexed by row and column */
  jtp_menu * menu;           /* Possible menu content, only for NHW_MENU */
  jtp_list * buttons;        /* Possible button content, only for NHW_MENU */
} jtp_window;

typedef struct {
  unsigned char * border_top;
  unsigned char * border_bottom;
  unsigned char * border_left;
  unsigned char * border_right;
  unsigned char * corner_tl;
  unsigned char * corner_tr;
  unsigned char * corner_bl;
  unsigned char * corner_br;
  unsigned char * center;
  unsigned char * radiobutton_on;
  unsigned char * radiobutton_off;
  unsigned char * checkbox_on;
  unsigned char * checkbox_off;
  unsigned char * scrollbar;
  unsigned char * scrollbutton_up;
  unsigned char * scrollbutton_down;
  unsigned char * scroll_indicator;
  unsigned char * direction_arrows;
} jtp_window_graphics;

typedef struct { 
                unsigned char *graphic;
                int xmod,ymod;
               }jtp_tilestats;

typedef struct {
  jtp_tilestats * west;
  jtp_tilestats * north;
  jtp_tilestats * east;
  jtp_tilestats * south;
} jtp_wall_style;

typedef struct {
  int xspan, yspan;
  jtp_tilestats ** pattern;
} jtp_floor_style;

typedef struct {
  jtp_tilestats * west;
  jtp_tilestats * north;
  jtp_tilestats * east;
  jtp_tilestats * south;
  jtp_tilestats * northwest;
  jtp_tilestats * northeast;
  jtp_tilestats * southwest;
  jtp_tilestats * southeast;
  jtp_tilestats * northwest_bank;
  jtp_tilestats * northeast_bank;
  jtp_tilestats * southwest_bank;
  jtp_tilestats * southeast_bank;
} jtp_floor_edge_style;

typedef struct {
  int x, y;        /* Position in the map window */
  int style, pos;  /* Floor decor style (=floor style) and position in it */
} jtp_floor_decor;


typedef struct {
  int x;
  int y;
  int width;
  int height;
  int id;
  char accelerator;
  char * text;
} jtp_button;


typedef struct {
                int tindex;
                unsigned char elevation;
               }jtp_maptile;

typedef struct {
  char * searchpattern;
  int soundtype;
  char * filename;
} jtp_event_sound;

/* Variables provided by the Vulture's Eye interface */
extern int ** jtp_mapglyph_cmap;
extern int ** jtp_mapglyph_obj;
extern int ** jtp_mapglyph_mon;
extern int jtp_map_width, jtp_map_height;
extern int jtp_map_changed;
extern jtp_list * jtp_windowlist;
extern int jtp_max_window_id;
extern int jtp_first_shown_message;

extern int jtp_movebuffer[];         /* Target squares for 'autopilot' */
extern int jtp_move_length;          /* Length of autopilot sequence */
extern int jtp_autopilot_type;       /* Autopilot type */

extern int jtp_is_spellbook_being_viewed;   /* Is the user viewing the spellbook? */
extern int jtp_is_backpack_shortcut_active; /* Has the user selected an action from the backpack screen? */
extern int jtp_backpack_shortcut_action;    /* The selected action */

extern int jtp_you_x, jtp_you_y;     /* Location of player character on the map */
extern int jtp_old_you_x, jtp_old_you_y; /* Previous location of player character on the map */
extern int jtp_map_x, jtp_map_y;     /* Center of displayed map area */

extern int jtp_recenter_after_movement; /* Interface option */
extern int jtp_play_music;              /* Interface option */
extern int jtp_play_effects;            /* Interface option */
extern int jtp_quit_nethack;            /* Interface option: quit after game ends? */
extern int jtp_fullscreen;              /* Interface option: play in fullscreen mode? */
extern char * jtp_external_midi_player_command; /* Interface option */
extern char * jtp_external_mp3_player_command;  /* Interface option */

extern int jtp_tile_conversion_initialized; /* Have the tile conversion tables been set up? */

/* Function declarations */
extern jtp_list *  jtp_list_new();
extern void        jtp_list_reset(jtp_list *list_to_reset);
extern void        jtp_list_advance(jtp_list *list_to_advance);
extern void *      jtp_list_current(jtp_list *list_to_access);
extern void        jtp_list_add(jtp_list *list_to_access, void *item_to_add);
extern void        jtp_list_remove(jtp_list *list_to_access, void *item_to_remove);
extern int         jtp_list_length(jtp_list *list_to_access);
extern jtp_window *jtp_find_window(winid);
extern void        jtp_free_menu(jtp_menu *menu_to_free);
extern void        jtp_free_buttons(jtp_list *buttonlist);
extern void        jtp_clear_screen();
extern unsigned char * jtp_draw_window(int x, int y, int width, int height);
extern void        jtp_draw_button(int x, int y, int width, int height, char *str);
extern void        jtp_draw_all_windows();
extern void        jtp_show_screen();
extern void        jtp_show_map_area();
extern void        jtp_show_status_area();
extern void        jtp_show_message_area(int messages_height);
extern void        jtp_read_mouse_input();
extern int         jtp_query(int qx, int qy, const char *qmessage, int nanswers, char *panswers, int is_dropdown);
extern void        jtp_messagebox(const char *message);
extern char        jtp_process_mouseclick();
extern char        jtp_whatis_mouseclick();
extern void        jtp_get_input(int, int, const char *ques, char *input);
extern void        jtp_askname();
extern int         jtp_get_menu_selection(jtp_window *menuwindow);
extern void        jtp_get_menu_coordinates(jtp_window *menuwindow);
extern void        jtp_get_map_input();
extern void        jtp_draw_map(jtp_window *, int, int);
extern void        jtp_draw_status(jtp_window *);
extern int         jtp_draw_messages(jtp_window *);
extern void        jtp_exit_graphics();
extern int         jtp_is_onscreen(int x, int y);
extern void        jtp_show_logo_screen();
extern void        jtp_show_ending(jtp_window *);
extern int         jtp_swap_key(int);
extern void        jtp_play_ambient_sound(int);
extern void        jtp_play_event_sound(const char *);
extern void        jtp_play_command_sound(int);
extern void        jtp_init_glyph_tiles();

#endif
/* End of jtp_win.h */
