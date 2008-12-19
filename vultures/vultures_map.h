/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* Copyright (c) Daniel Thaler, 2005				  */

#ifndef _vultures_map_h_
#define _vultures_map_h_

extern "C" {
	#include "hack.h"
}

/* exported functions */
extern void vultures_print_glyph(winid, XCHAR_P, XCHAR_P, int);
extern int vultures_object_to_tile(int mon_id, int x, int y, struct obj *in_obj);
extern int vultures_obfuscate_object(int obj_id);
extern int vultures_perform_map_action(int action_id, point mappos);

extern void vultures_destroy_map(void);
extern int vultures_init_map(void);

#endif
