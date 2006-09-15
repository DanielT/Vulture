/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* Copyright (c) Daniel Thaler, 2005				  */

#ifndef _jtp_map_h_
#define _jtp_map_h_

#include "jtp_tile.h"

/* global map-related constants */
#define JTP_AMBIENT_LIGHT 32 /*ambient light on the map = minimum lighting level*/

/* Map dimensions in glyphs */
#define JTP_MAP_WIDTH COLNO
#define JTP_MAP_HEIGHT ROWNO

/* 
 * Tile drawing: pixel coordinate difference from a square to
 * the one next to it in the map. Because of isometry,
 * this is not the same as the width/height of a tile!
 */
#ifdef JTP_USE_SMALL_MAP_TILES
#define JTP_MAP_XMOD 46
#define JTP_MAP_YMOD 18
#else
#define JTP_MAP_XMOD 56
#define JTP_MAP_YMOD 22
#endif


/* Map symbols in the 'view map' display */
enum jtp_map_symbol {
  JTP_MAP_SYMBOL_WALL = 0,
  JTP_MAP_SYMBOL_FLOOR,
  JTP_MAP_SYMBOL_UP,
  JTP_MAP_SYMBOL_DOWN,
  JTP_MAP_SYMBOL_DOOR,
  JTP_MAP_SYMBOL_CMAP,
  JTP_MAP_SYMBOL_TRAP,
  JTP_MAP_SYMBOL_OBJECT,
  JTP_MAP_SYMBOL_MONSTER,
  JTP_MAX_MAP_SYMBOLS
};


/* exported variables */
extern jtp_tilenumber ** jtp_map_back;
extern jtp_tilenumber ** jtp_map_obj;
extern jtp_tilenumber ** jtp_map_trap;
extern jtp_tilenumber ** jtp_map_furniture;
extern jtp_tilenumber ** jtp_map_specialeff;
extern jtp_tilenumber ** jtp_map_mon;
extern unsigned int ** jtp_map_specialattr;
extern unsigned char ** jtp_map_deco;
extern jtp_tilenumber jtp_map_swallow;
extern char ** jtp_map_tile_is_dark;
extern char ** jtp_room_indices;
extern int jtp_map_center_x;
extern int jtp_map_center_y;
extern int jtp_map_changed;
extern jtp_tile ** jtp_tiles;
extern jtp_wall_style ** jtp_maptile_wall;
extern jtp_floor_edge_style ** jtp_maptile_floor_edge;
extern unsigned char * jtp_map_parchment_center;
extern unsigned char * jtp_map_parchment_top;
extern unsigned char * jtp_map_parchment_bottom;
extern unsigned char * jtp_map_parchment_left;
extern unsigned char * jtp_map_parchment_right;
extern int jtp_game_palette_set;
extern jtp_floor_style * jtp_floors;
extern jtp_floor_edge_style * jtp_floor_edges;
extern jtp_wall_style * jtp_walls;
extern int ** jtp_map_light;
extern int jtp_nlights;
extern unsigned char * jtp_map_symbols[JTP_MAX_MAP_SYMBOLS];

/* exported functions */
extern void jtp_map_to_screen(int map_x, int map_y, int *screen_x, int *screen_y);
extern char *jtp_map_square_description(int tgt_x, int tgt_y, int include_seen);
extern void jtp_put_tile(int x, int y, int shade, unsigned char *a);
extern void jtp_draw_level(jtp_window *, int, int);
extern void jtp_view_map(void);

#endif
