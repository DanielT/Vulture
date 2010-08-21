/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vulture_opt_h_
#define _vulture_opt_h_



/* Wall display styles (game option) */
enum vulture_wall_display_styles {
	V_WALL_DISPLAY_STYLE_FULL,
	V_WALL_DISPLAY_STYLE_HALF_HEIGHT,
};


/* this is only a struct to keep config options together;
* hopefully this will prevent them from being defined all over the place  */
struct vulture_optstruct {
	int recenter;
	int play_music;
	int play_effects;
	int wall_style;
	int height;
	int width;
	int fullscreen;
	int show_helptb;
	int show_actiontb;
	int show_minimap;
	int messagelines;
	int no_key_translation;
	int use_standard_inventory;
	int use_standard_object_menus;
	int highlight_cursor_square;
	int debug;
	char macro[6][10];
	double wall_opacity;
};


extern struct vulture_optstruct vulture_opts;


extern void vulture_read_options(void);
extern "C" {
extern int vulture_iface_opts(void);
}
extern void vulture_write_userconfig(void);

#endif
