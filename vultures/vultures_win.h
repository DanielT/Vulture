/*	SCCS Id: @(#)jtp_win.h	1.0	2000/9/10	*/
/* Copyright (c) Jaakko Peltonen, 2000 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_win_h_
#define _vultures_win_h_

#include "hack.h"
#include "vultures_types.h"

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

/* Menu handling constants */
#define JTP_NOT_SELECTABLE -12345

/* Maximum number of old messages stored by message window */
#define JTP_MAX_OLD_MESSAGES 50

/* Maximum number of messages displayed on-screen */
#define JTP_MAX_SHOWN_MESSAGES 3

/* Autopilot types */
#define JTP_AUTOPILOT_WHATSTHIS 1

/* Location and dimensions of the status window graphic, in pixels */
#define JTP_STATUSBAR_WIDTH 800
#define JTP_STATUSBAR_HEIGHT 100

#define JTP_MAX_SPELL_SYMBOLS 10

/* Font indices. Currently, there're only 2 fonts (large & small). */
#define JTP_FONT_SMALL 0
#define JTP_FONT_LARGE 1
#define JTP_MAX_FONTS  2
#define JTP_FONT_INTRO     JTP_FONT_LARGE
#define JTP_FONT_MENU      JTP_FONT_SMALL
#define JTP_FONT_HEADLINE  JTP_FONT_LARGE
#define JTP_FONT_BUTTON    JTP_FONT_LARGE
#define JTP_FONT_TOOLTIP   JTP_FONT_SMALL
#define JTP_FONT_STATUS    JTP_FONT_SMALL
#define JTP_FONT_MESSAGE   JTP_FONT_SMALL
#define JTP_FONT_INPUT     JTP_FONT_SMALL

/* Message shading: old messages grow darker */
#define JTP_MAX_MESSAGE_COLORS 16

/*
 * colors used to draw text
 */
#define JTP_COLOR_TEXT 15
#define JTP_COLOR_INTRO_TEXT 255

#define JTP_COLOR_BACKGROUND 0
#define JTP_COLOR_HOTSPOT 16
#define JTP_COLOR_BORDER 79


/* Indices into warning colors */
enum jtp_warn_type {
    JTP_WARN_NONE = 0,
    JTP_WARN_NORMAL,
    JTP_WARN_MORE,
    JTP_WARN_ALERT,
    JTP_WARN_CRITICAL,
    JTP_MAX_WARN
};

/* Event sound types */
enum jtp_event_sound {
    JTP_EVENT_SOUND_TYPE_NONE = 0,
    JTP_EVENT_SOUND_TYPE_SND,
    JTP_EVENT_SOUND_TYPE_MUS,
    JTP_EVENT_SOUND_TYPE_RANDOM_SONG,
    JTP_EVENT_SOUND_TYPE_CD_AUDIO,
};

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
    unsigned char * invarrow_left;
    unsigned char * invarrow_right;
} jtp_window_graphics;


typedef struct {
    jtp_tile * west;
    jtp_tile * north;
    jtp_tile * east;
    jtp_tile * south;
} jtp_wall_style;


typedef struct {
    jtp_tile * west;
    jtp_tile * north;
    jtp_tile * east;
    jtp_tile * south;
    jtp_tile * northwest;
    jtp_tile * northeast;
    jtp_tile * southwest;
    jtp_tile * southeast;
    jtp_tile * northwest_bank;
    jtp_tile * northeast_bank;
    jtp_tile * southwest_bank;
    jtp_tile * southeast_bank;
} jtp_floor_edge_style;


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
    int x;
    int y;
    int tile;
    unsigned int special;
} jtp_map_monster;


typedef struct {
    int tindex;
    unsigned char elevation;
} jtp_maptile;

typedef struct {
    char * searchpattern;
    int soundtype;
    char * filename;
} jtp_event_sound;

/* Variables provided by the Vulture's interface */

extern int jtp_statusbar_x;
extern int jtp_statusbar_y;
extern unsigned char * jtp_statusbar;
extern unsigned char * jtp_shade;
extern unsigned char * jtp_messages_background;

extern jtp_list * jtp_windowlist;
extern int jtp_max_window_id;
extern int jtp_first_shown_message;
extern char *jtp_game_path;

extern int jtp_movebuffer[];         /* Target squares for 'autopilot' */
extern int jtp_move_length;          /* Length of autopilot sequence */
extern int jtp_autopilot_type;       /* Autopilot type */

extern int jtp_is_backpack_shortcut_active; /* Has the user selected an action from the backpack screen? */
extern int jtp_backpack_shortcut_action;    /* The selected action */

extern int jtp_map_x, jtp_map_y;     /* Center of displayed map area */

extern int jtp_recenter_after_movement; /* Interface option */
extern int jtp_play_music;              /* Interface option */
extern int jtp_play_effects;            /* Interface option */
extern int jtp_quit_nethack;            /* Interface option: quit after game ends? */
extern char * jtp_external_midi_player_command; /* Interface option */
extern char * jtp_external_mp3_player_command;  /* Interface option */

extern int jtp_is_shortcut_active;
extern int jtp_shortcut_query_response;

extern unsigned char * jtp_backpack_center;
extern unsigned char * jtp_backpack_top;
extern unsigned char * jtp_backpack_bottom;
extern unsigned char * jtp_backpack_left;
extern unsigned char * jtp_backpack_right;

extern double jtp_min_command_delay;
extern double jtp_min_scroll_delay;
extern int jtp_one_command_per_click;

extern jtp_event_sound ** jtp_event_sounds;
extern int jtp_n_event_sounds;
extern int jtp_n_background_songs;

extern jtp_window_graphics jtp_defwin;
extern unsigned char jtp_message_colors[JTP_MAX_MESSAGE_COLORS];
extern unsigned char jtp_warn_colors[JTP_MAX_WARN];



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
extern unsigned char * jtp_draw_query_window(char * str[], int nstr, jtp_button * buttons, int nbuttons, int *win_x, int *win_y);
extern void        jtp_messagebox(const char *message);
extern char        jtp_whatis_mouseclick(int *tx, int *ty);
extern int         jtp_get_input(int, int, const char *ques, char *input);
extern int         jtp_get_menu_selection(jtp_window *menuwindow);
extern void        jtp_get_menu_coordinates(jtp_window *menuwindow);
extern void        jtp_get_mouse_input(jtp_tile * m_cursor, int whenstop);
extern void        jtp_draw_status(jtp_window *);
extern int         jtp_draw_messages(jtp_window *);
extern int         jtp_is_onscreen(int x, int y);
extern int         jtp_swap_key(int);
extern void        jtp_play_ambient_sound(int);
extern void        jtp_play_event_sound(const char *);
extern void        jtp_play_command_sound(int);
extern char        jtp_process_mouseclick(void);
extern void        jtp_view_inventory(void);
extern void        jtp_draw_menu(int x, int y, jtp_menu *menu, jtp_menuitem * firstitem);
extern void        jtp_draw_buttons(int x, int y, jtp_list * buttons);
extern int         jtp_find_menu_accelerator(const char *description, char * used_accelerators);
extern int         jtp_monster_to_tile(int cur_glyph, XCHAR_P x, XCHAR_P y);
extern int         jtp_object_to_tile(int obj_id, int x, int y);
extern void        jtp_clear_walls(int y, int x);
extern unsigned char *jtp_choose_target_tooltip(int tgt_x, int tgt_y);
extern int jtp_get_mouse_inventory_input(jtp_tile * m_cursor, jtp_hotspot ** hotspots, int n_hotspots, int whenstop);

extern struct permonst * lookat(int, int, char *, char *);
extern void append_slash(char *);
#endif
/* End of jtp_win.h */
