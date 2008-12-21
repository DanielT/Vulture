#ifndef _levelwin_h_
#define _levelwin_h_

#include "vultures_tile.h"
#include "vultures_types.h"
#include "window.h"

/* 
 * Tile drawing: pixel coordinate difference from a square to
 * the one next to it in the map. Because of isometry,
 * this is not the same as the width/height of a tile!
 */
#define V_MAP_XMOD 56
#define V_MAP_YMOD 22

#define V_FILENAME_TOOLBAR1             "tb1"
#define V_FILENAME_TOOLBAR2             "tb2"

typedef enum {
	MAP_MON,
	MAP_OBJ,
	MAP_TRAP,
	MAP_BACK,
	MAP_SPECIAL,
	MAP_FURNITURE,
	MAP_DARKNESS,
	MAP_PET,
	
	MAP_GLYPH // the actual glyph
} glyph_type;


typedef enum {
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
} map_action;


class levelwin : public window
{
public:
	levelwin();
	~levelwin();
	void init();
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	void set_map_data(glyph_type type, int x, int y, int newval, bool force);
	void clear_map();
	void set_swallowed(int swglyph) { map_swallow = swglyph; };
	
	int get_glyph(glyph_type type, int x, int y);
	
	point mouse_to_map(point mouse);
	point map_to_mouse(point mappos);
	
	void toggle_uiwin(int menuid, bool enabled);
	void set_view(int x, int y);
	bool need_recenter(int map_x, int map_y);
	void force_redraw(void);
	void set_wall_style(int style);
	
	eventresult handle_click(void* result, int button, point mappos);
	string map_square_description(point target, int include_seen);

private:
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
	map_action get_map_action(point mappos);
	map_action get_map_contextmenu(point mappos);
	int get_map_cursor(point mappos);

	int view_x, view_y;  /* Center of displayed map area */
	
	/* Map window contents, as Vulture's tile IDs */
	int map_glyph[ROWNO][COLNO];     /* real glyph representation of map */
	int map_back[ROWNO][COLNO];      /* background (floors, walls, pools, moats, ...) */
	int map_furniture[ROWNO][COLNO]; /* furniture (stairs, altars, fountains, ...) */
	int map_trap[ROWNO][COLNO];      /* traps */
	int map_obj[ROWNO][COLNO];       /* topmost object */
	int map_specialeff[ROWNO][COLNO];   /* special effects: zap, engulf, explode */
	int map_mon[ROWNO][COLNO];       /* monster tile ID */
	int map_darkness[ROWNO][COLNO];
	int map_pet[ROWNO][COLNO]; /* special attributes, we use them to highlight the pet */
	unsigned char map_deco[ROWNO][COLNO];     /* positions of murals and carpets */
	int map_swallow; /* the engulf tile, if any */
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
