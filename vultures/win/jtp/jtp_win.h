/*	SCCS Id: @(#)jtp_win.h	1.0	2000/9/10	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_win_h_
#define _jtp_win_h_

#include "hack.h"

/*
 * Subdirectories used by Vulture's. 
 * These should be under the main directory.
 */
#define JTP_CONFIG_DIRECTORY   "config"
#define JTP_GRAPHICS_DIRECTORY "graphics"
#define JTP_SOUND_DIRECTORY    "sound"
#define JTP_MUSIC_DIRECTORY    "music"
#define JTP_MANUAL_DIRECTORY   "manual"

/* Graphics constants - these should really not be here */
#define JTP_MAX_SHADES 64 

/* Special window content types */
#define JTP_WINCONTENT_EMPTY (-1)
#define JTP_WINCONTENT_GLYPH_UNEXPLORED (-1)
#define JTP_WINCONTENT_GLYPH_NOT_VISIBLE (-2)

/* Menu handling constants */
#define JTP_NOT_SELECTABLE -12345

/* Maximum number of old messages stored by message window */
#define JTP_MAX_OLD_MESSAGES 50

/* Maximum number of messages displayed on-screen */
#define JTP_MAX_SHOWN_MESSAGES 3

/* Autopilot types */
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
  int accelerator;                           /* Key accelerator for the item */
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
  int items_height;                          /* Height of items in window */
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
	/* image data (width & height encoded in first 4 bytes) */
	unsigned char *graphic;
	/* hotspot offsets;
	   difference between left/top most non-transparent pixel
	   and hotspot defined in the image
	 */
	int xmod,ymod;
} jtp_tilestats;

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
  int accelerator;
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

/* Variables provided by the Vulture's interface */
extern int ** jtp_mapglyph_cmap;
extern int ** jtp_mapglyph_obj;
extern int ** jtp_mapglyph_mon;
extern unsigned int ** jtp_mapglyph_special;
extern int jtp_map_width, jtp_map_height;
extern int jtp_map_changed;
extern jtp_list * jtp_windowlist;
extern int jtp_max_window_id;
extern int jtp_first_shown_message;
extern char *jtp_game_path;

extern int jtp_movebuffer[];         /* Target squares for 'autopilot' */
extern int jtp_move_length;          /* Length of autopilot sequence */
extern int jtp_autopilot_type;       /* Autopilot type */

extern int jtp_is_spellbook_being_viewed;   /* Is the user viewing the spellbook? */
extern int jtp_is_backpack_shortcut_active; /* Has the user selected an action from the backpack screen? */
extern int jtp_backpack_shortcut_action;    /* The selected action */

extern int jtp_map_x, jtp_map_y;     /* Center of displayed map area */

extern int jtp_recenter_after_movement; /* Interface option */
extern int jtp_play_music;              /* Interface option */
extern int jtp_play_effects;            /* Interface option */
extern int jtp_quit_nethack;            /* Interface option: quit after game ends? */
extern char * jtp_external_midi_player_command; /* Interface option */
extern char * jtp_external_mp3_player_command;  /* Interface option */

extern int jtp_tile_conversion_initialized; /* Have the tile conversion tables been set up? */

extern int jtp_is_shortcut_active;
extern int jtp_shortcut_query_response;

/* Function declarations */
extern jtp_list *  jtp_list_new(void);
extern void        jtp_list_reset(jtp_list *list_to_reset);
extern void        jtp_list_advance(jtp_list *list_to_advance);
extern void *      jtp_list_current(jtp_list *list_to_access);
extern void        jtp_list_add(jtp_list *list_to_access, void *item_to_add);
extern void        jtp_list_remove(jtp_list *list_to_access, void *item_to_remove);
extern int         jtp_list_length(jtp_list *list_to_access);
extern jtp_window *jtp_find_window(winid);
extern void        jtp_free_menu(jtp_menu *menu_to_free);
extern void        jtp_free_buttons(jtp_list *buttonlist);
extern void        jtp_clear_screen(void);
extern unsigned char * jtp_draw_window(int x, int y, int width, int height);
extern void        jtp_draw_button(int x, int y, int width, int height, const char *str);
extern void        jtp_draw_all_windows(void);
extern void        jtp_show_screen(void);
extern void        jtp_show_map_area(void);
extern void        jtp_show_status_area(void);
extern void        jtp_show_message_area(int messages_height);
extern void        jtp_read_mouse_input(void);
extern int         jtp_query_choices(const char *ques, const char *choices, int nchoices);
extern int         jtp_query_direction(const char *ques);
extern int         jtp_query_anykey(const char *ques);
unsigned char * jtp_draw_query_window(char * str[], int nstr, jtp_button * buttons, int nbuttons, int *win_x, int *win_y);

extern void        jtp_messagebox(const char *message);
extern char        jtp_whatis_mouseclick(int *tx, int *ty);
extern int         jtp_get_input(int, int, const char *ques, char *input);
extern void        jtp_askname(void);
extern int         jtp_get_menu_selection(jtp_window *menuwindow);
extern void        jtp_get_menu_coordinates(jtp_window *menuwindow);
extern void        jtp_get_map_input(void);
extern void        jtp_draw_map(jtp_window *, int, int);
extern void        jtp_draw_status(jtp_window *);
extern int         jtp_draw_messages(jtp_window *);
extern int         jtp_is_onscreen(int x, int y);
extern void        jtp_show_logo_screen(void);
extern void        jtp_show_ending(jtp_window *);
extern int         jtp_swap_key(int);
extern void        jtp_play_ambient_sound(int);
extern void        jtp_play_event_sound(const char *);
extern void        jtp_play_command_sound(int);
extern void        jtp_init_glyph_tiles(void);
extern void        jtp_select_player(void);
extern int         jtp_init_graphics(void);
extern char        jtp_process_mouseclick(void);
extern void        jtp_view_inventory(void);
extern void        jtp_draw_menu(int x, int y, jtp_menu *menu, jtp_menuitem * firstitem);
extern void        jtp_draw_buttons(int x, int y, jtp_list * buttons);
extern int         jtp_find_menu_accelerator(const char *description, char * used_accelerators);
extern void        jtp_view_map(void);
extern char *      jtp_make_filename(const char *subdir1, const char *subdir2, const char *name);
extern unsigned char *jtp_load_graphic(const char *subdir, const char *name, int load_palette);

struct permonst * FDECL(jtp_do_lookat, (int, int, char *, char *));

#endif
/* End of jtp_win.h */
