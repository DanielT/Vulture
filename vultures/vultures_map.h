/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* Copyright (c) Daniel Thaler, 2005				  */

#ifndef _vultures_map_h_
#define _vultures_map_h_

#include <SDL.h>
#include "hack.h"
#include "vultures_gametiles.h"
#include "vultures_win.h"

/* Map dimensions in glyphs */
#define V_MAP_WIDTH COLNO
#define V_MAP_HEIGHT ROWNO

/* 
 * Tile drawing: pixel coordinate difference from a square to
 * the one next to it in the map. Because of isometry,
 * this is not the same as the width/height of a tile!
 */
#define V_MAP_XMOD 56
#define V_MAP_YMOD 22

#define VULTURES_MAP_SYMBOL_WIDTH   7
#define VULTURES_MAP_SYMBOL_HEIGHT 14

#ifdef VULTURESEYE
# define V_MAX_MAP_SYMBOLS 40*30
#endif
#ifdef VULTURESCLAW
# define V_MAX_MAP_SYMBOLS 40*36
#endif

/*
 * colors used to draw the mini-map
 */
#define V_COLOR_MINI_CORRIDOR CLR32_PURPLE44
#define V_COLOR_MINI_STAIRS   CLR32_LIGHTPINK
#define V_COLOR_MINI_DOOR     CLR32_BROWN
#define V_COLOR_MINI_FLOOR    CLR32_PURPLE44
#define V_COLOR_MINI_YOU      CLR32_WHITE


enum actions {
    V_ACTION_NONE,
    V_ACTION_TRAVEL,
    V_ACTION_MOVE_HERE,
    V_ACTION_LOOT,
    V_ACTION_PICK_UP,
    V_ACTION_GO_DOWN,
    V_ACTION_GO_UP,
    V_ACTION_DRINK,
    V_ACTION_KICK,
    V_ACTION_OPEN_DOOR,
    V_ACTION_SEARCH,

    V_ACTION_ENGRAVE,
    V_ACTION_LOOK_AROUND,
    V_ACTION_PAY_BILL,
    V_ACTION_OFFER,
    V_ACTION_PRAY,
    V_ACTION_REST,
    V_ACTION_SIT,
    V_ACTION_TURN_UNDEAD,
    V_ACTION_WIPE_FACE,
    V_ACTION_FORCE_LOCK,
    V_ACTION_UNTRAP,
    V_ACTION_CLOSE_DOOR,
    V_ACTION_WHATS_THIS,
    V_ACTION_MONSTER_ABILITY,
    V_ACTION_CHAT,
    V_ACTION_FIGHT,
    V_ACTION_NAMEMON
};



/* exported variables */
extern int vultures_view_x, vultures_view_y;     /* Center of displayed map area */
extern struct walls * walltiles;

extern int vultures_map_draw_lastmove;
extern int vultures_map_draw_msecs;
extern point vultures_map_highlight;

/* exported functions */
extern char * vultures_map_square_description(point target, int include_seen);
extern int vultures_draw_level(struct window * win);
extern int vultures_draw_map(struct window * win);
extern int vultures_draw_minimap(struct window * win);
extern void vultures_print_glyph(winid, XCHAR_P, XCHAR_P, int);
extern int vultures_object_to_tile(int mon_id, int x, int y);

extern point vultures_mouse_to_map(point mouse);
extern point vultures_map_to_mouse(point mappos);
extern int vultures_get_map_action(point mappos);
extern int vultures_get_map_contextmenu(point mappos);
extern int vultures_perform_map_action(int action_id, point mappos);
extern int vultures_get_map_cursor(point mappos);
extern void vultures_map_force_redraw(void);

void vultures_destroy_map(void);
extern int vultures_init_map(void);
extern void vultures_clear_map(void);

extern void vultures_add_to_clipregion(int tl_x, int tl_y, int br_x, int br_y);

#endif
