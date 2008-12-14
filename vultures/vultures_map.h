/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* Copyright (c) Daniel Thaler, 2005				  */

#ifndef _vultures_map_h_
#define _vultures_map_h_

#include <SDL.h>

extern "C" {
	#include "hack.h"
}

#include "vultures_win.h"


#define VULTURES_CLIPMARGIN 200


/* exported variables */

extern int vultures_map_draw_lastmove;
extern int vultures_map_draw_msecs;
extern point vultures_map_highlight;
extern int vultures_map_highlight_objects;

/* exported functions */
// extern char * vultures_map_square_description(point target, int include_seen);
// extern int vultures_draw_level(struct window * win);
// extern int vultures_draw_map(struct window * win);
// extern int vultures_draw_minimap(struct window * win);
extern void vultures_print_glyph(winid, XCHAR_P, XCHAR_P, int);
extern int vultures_object_to_tile(int mon_id, int x, int y, struct obj *in_obj);
extern int vultures_obfuscate_object(int obj_id);

// extern point vultures_mouse_to_map(point mouse);
// extern point vultures_map_to_mouse(point mappos);
extern int vultures_perform_map_action(int action_id, point mappos);
// extern void vultures_map_force_redraw(void);

void vultures_destroy_map(void);
extern int vultures_init_map(void);

// extern void vultures_add_to_clipregion(int tl_x, int tl_y, int br_x, int br_y);

#endif
