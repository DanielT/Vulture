#ifndef _levelwin_h_
#define _levelwin_h_

#include "vultures_tile.h"
#include "vultures_types.h"
#include "window.h"
#include "mapdata.h"

/* 
 * Tile drawing: pixel coordinate difference from a square to
 * the one next to it in the map. Because of isometry,
 * this is not the same as the width/height of a tile!
 */
#define V_MAP_XMOD 56
#define V_MAP_YMOD 22

#define V_FILENAME_TOOLBAR1             "tb1"
#define V_FILENAME_TOOLBAR2             "tb2"



class levelwin : public window, public mapviewer
{
public:
	levelwin(mapdata *data);
	~levelwin();
	void init();
	virtual bool draw();
	virtual eventresult handle_timer_event(window* target, void* result, int time);
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int mouse_x, int mouse_y, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_keydown_event(window* target, void* result, int sym, int mod, int unicode);
	virtual eventresult handle_resize_event(window* target, void* result, int res_w, int res_h);
	
	point mouse_to_map(point mouse);
	point map_to_mouse(point mappos);
	
	void toggle_uiwin(int menuid, bool enabled);
	void set_view(int x, int y);
	bool need_recenter(int map_x, int map_y);
	void force_redraw(void);
	void set_wall_style(int style);
	
	void map_update(glyph_type type, int prev_glyph, int new_glyph, int x, int y);
	void map_clear();

private:
	mapdata *map_data;

	void add_to_clipregion(int tl_x, int tl_y, int br_x, int br_y);
	int get_room_index(int x, int y);
	int get_wall_decor(int floortype, int wally, int wallx, int floory, int floorx);
	int get_floor_decor(int floorstyle, int floory, int floorx);
	void init_floor_decors(int num_decors);
	void get_wall_tiles(int y, int x);
	int get_floor_tile(int tile, int y, int x);
	void get_floor_edges(int y, int x);
	void clear_walls(int y, int x);
	void clear_floor_edges(int y, int x);
	int get_map_cursor(point mappos);

	int view_x, view_y;  /* Center of displayed map area */
	
	unsigned char map_deco[ROWNO][COLNO];     /* positions of murals and carpets */
	point map_highlight;

	/* pointer to full height, half height or transparent walltile array */
	struct walls *walltiles;

	struct walls maptile_wall[ROWNO][COLNO]; /* Custom (combination) wall style for each square */
	struct fedges maptile_floor_edge[ROWNO][COLNO]; /* Custom floor edge style for each square */

	char room_indices[ROWNO][COLNO]; /* packed room numbers and deco ids */

	int clip_tl_x;
	int clip_tl_y ;
	int clip_br_x;
	int clip_br_y;
};


extern levelwin *levwin;
extern int vultures_map_draw_lastmove;
extern int vultures_map_draw_msecs;
extern int vultures_map_highlight_objects;

#endif
