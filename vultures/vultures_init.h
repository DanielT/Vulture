/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000 */

#ifndef _jtp_init_h_
#define _jtp_init_h_


/* exported variables */
extern int jtp_map_width, jtp_map_height;
extern int jtp_tile_conversion_initialized;
extern int * jtp_cmaptiles;
extern int * jtp_engulfmap;


/* exported functions */
extern void          jtp_askname(void);
extern void          jtp_show_logo_screen(void);
extern void          jtp_show_ending(jtp_window *);
extern void          jtp_init_glyph_tiles(void);
extern void          jtp_select_player(void);
extern int           jtp_init_graphics(void);
extern char *        jtp_make_filename(const char *subdir1, const char *subdir2, const char *name);
extern unsigned char *jtp_load_graphic(const char *subdir, const char *name, int load_palette);

#endif
