/* NetHack may be freely redistributed.  See license for details. */

#ifndef _mapdata_h_
#define _mapdata_h_

extern "C" {
#include "hack.h"
}

#include "window.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

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


class mapviewer {
public:
	virtual void map_update(glyph_type type, int prev_glyph, int new_glyph, int x, int y) = 0;
	virtual void map_clear() = 0;
};

class mapdata {
public:
	mapdata();
	
	void clear();
	void set_glyph(int x, int y, int glyph);
	int get_glyph(glyph_type type, int x, int y) const;
	string map_square_description(point target, int include_seen);
	eventresult handle_click(void* result, int button, point mappos);
	map_action get_map_action(point mappos);
	map_action get_map_contextmenu(point mappos);

	int perform_map_action(int action_id, point mappos);

	void add_viewer(mapviewer *v);
	void del_viewer(mapviewer *v);
	int map_swallow; /* the engulf tile, if any */
	
private:
	void set_map_data(glyph_type type, int x, int y, int newval, bool force);
	char mappos_to_dirkey(point mappos);

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

	int vultures_tilemap_engulf[NUMMONS];
	int vultures_tilemap_misc[MAXPCHARS];

	vector<mapviewer*> views;
};

extern mapdata *map_data;

#endif
