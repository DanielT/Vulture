/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_opt_h_
#define _vultures_opt_h_



/* Wall display styles (game option) */
enum vultures_wall_display_styles {
	V_WALL_DISPLAY_STYLE_FULL,
	V_WALL_DISPLAY_STYLE_HALF_HEIGHT,
};


/* this is only a struct to keep config options together;
* hopefully this will prevent them from being defined all over the place  */
struct vultures_optstruct {
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


extern struct vultures_optstruct vultures_opts;


extern void vultures_read_options(void);
extern "C" {
extern int vultures_iface_opts(void);
}
extern void vultures_write_userconfig(void);

#endif
