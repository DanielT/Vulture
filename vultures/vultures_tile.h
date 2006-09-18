/* Copyright (c) Daniel Thaler, 2006                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_tile_h_
#define _vultures_tile_h_


extern void vultures_put_tile(int x, int y, int tile_id);
extern int vultures_load_gametiles(void);
extern void vultures_unload_gametiles(void);
extern vultures_tile * vultures_get_tile(int tile_id);
extern void vultures_flip_tile_arrays(void);


#endif
