/* NetHack may be freely redistributed.  See license for details. */

#include "vultures_sdl.h" /* XXX this must be the first include,
                             no idea why but it won't compile otherwise */

#include "vultures_main.h"
#include "vultures_win.h"
#include "vultures_gen.h"
#include "vultures_mou.h"
#include "vultures_tile.h"

#include "mapdata.h"
#include "window.h"
#include "levelwin.h"
#include "contextmenu.h"

	
extern "C" struct permonst * lookat(int, int, char *, char *);


/* the only other cmaps beside furniture that have significant height and
* therefore need to be drawn as part of the second pass with the furniture. */
#define V_EXTRA_FURNITURE(typ) ((typ) == DOOR || \
								(typ) == DBWALL || \
								(typ) == IRONBARS || \
								(typ) == TREE)

#define CMD_TRAVEL (char)0x90
#define META(c) (0x80 | (c))
#define CTRL(c) (0x1f & (c))

mapdata *map_data;

mapdata::mapdata()
{
	int i, j;
	
	for (i = 0; i < ROWNO; i++) {
		for (j = 0; j < COLNO; j++) {
			map_glyph[i][j] = NO_GLYPH;  
			map_back[i][j] = V_MISC_UNMAPPED_AREA;
			map_obj[i][j] = V_TILE_NONE;
			map_trap[i][j] = V_TILE_NONE;
			map_furniture[i][j] = V_TILE_NONE;
			map_specialeff[i][j] = V_TILE_NONE;
			map_mon[i][j] = V_TILE_NONE;
			map_pet[i][j] = 0;
			map_darkness[i][j] = 2;
		}
	}
	
	map_swallow = V_TILE_NONE;
	
	/* build engulf tile array */
	for (i = 0; i < NUMMONS; i++)
		vultures_tilemap_engulf[i] = V_TILE_NONE;

	vultures_tilemap_engulf[PM_OCHRE_JELLY]   = V_MISC_ENGULF_OCHRE_JELLY;
	vultures_tilemap_engulf[PM_LURKER_ABOVE]  = V_MISC_ENGULF_LURKER_ABOVE;
	vultures_tilemap_engulf[PM_TRAPPER]       = V_MISC_ENGULF_TRAPPER;
	vultures_tilemap_engulf[PM_PURPLE_WORM]   = V_MISC_ENGULF_PURPLE_WORM;
	vultures_tilemap_engulf[PM_DUST_VORTEX]   = V_MISC_ENGULF_DUST_VORTEX;
	vultures_tilemap_engulf[PM_ICE_VORTEX]    = V_MISC_ENGULF_ICE_VORTEX;
	vultures_tilemap_engulf[PM_ENERGY_VORTEX] = V_MISC_ENGULF_ENERGY_VORTEX;
	vultures_tilemap_engulf[PM_STEAM_VORTEX]  = V_MISC_ENGULF_STEAM_VORTEX;
	vultures_tilemap_engulf[PM_FIRE_VORTEX]   = V_MISC_ENGULF_FIRE_VORTEX;
	vultures_tilemap_engulf[PM_FOG_CLOUD]     = V_MISC_ENGULF_FOG_CLOUD;
	vultures_tilemap_engulf[PM_AIR_ELEMENTAL] = V_MISC_ENGULF_AIR_ELEMENTAL;
	vultures_tilemap_engulf[PM_JUIBLEX]      = V_MISC_ENGULF_JUIBLEX;

	/* build "special tile" array: these are the tiles for dungeon glyphs */
#ifdef VULTURESCLAW
	vultures_tilemap_misc[S_toilet] = V_MISC_TOILET;
#endif

	vultures_tilemap_misc[S_stone] = V_MISC_UNMAPPED_AREA;
	vultures_tilemap_misc[S_vwall] = V_TILE_WALL_GENERIC;
	vultures_tilemap_misc[S_hwall] = V_TILE_WALL_GENERIC;
	vultures_tilemap_misc[S_tlcorn] = V_TILE_WALL_GENERIC;
	vultures_tilemap_misc[S_trcorn] = V_TILE_WALL_GENERIC;
	vultures_tilemap_misc[S_blcorn] = V_TILE_WALL_GENERIC;
	vultures_tilemap_misc[S_brcorn] = V_TILE_WALL_GENERIC;
	vultures_tilemap_misc[S_crwall] = V_TILE_WALL_GENERIC;
	vultures_tilemap_misc[S_tuwall] = V_TILE_WALL_GENERIC;
	vultures_tilemap_misc[S_tdwall] = V_TILE_WALL_GENERIC;
	vultures_tilemap_misc[S_tlwall] = V_TILE_WALL_GENERIC;
	vultures_tilemap_misc[S_trwall] = V_TILE_WALL_GENERIC;
	vultures_tilemap_misc[S_ndoor] = V_MISC_DOOR_WOOD_BROKEN;
	vultures_tilemap_misc[S_vodoor] = V_MISC_VDOOR_WOOD_OPEN;
	vultures_tilemap_misc[S_hodoor] = V_MISC_HDOOR_WOOD_OPEN;
	vultures_tilemap_misc[S_vcdoor] = V_MISC_VDOOR_WOOD_CLOSED;
	vultures_tilemap_misc[S_hcdoor] = V_MISC_HDOOR_WOOD_CLOSED;
	vultures_tilemap_misc[S_room] = V_TILE_FLOOR_COBBLESTONE;
	vultures_tilemap_misc[S_corr] = V_TILE_FLOOR_ROUGH;
	vultures_tilemap_misc[S_upstair] = V_MISC_STAIRS_UP;
	vultures_tilemap_misc[S_dnstair] = V_MISC_STAIRS_DOWN;
	vultures_tilemap_misc[S_fountain] = V_MISC_FOUNTAIN;
	vultures_tilemap_misc[S_altar] = V_MISC_ALTAR;
	vultures_tilemap_misc[S_teleportation_trap] = V_MISC_TRAP_TELEPORTER;
	vultures_tilemap_misc[S_tree] = V_MISC_TREE;
	vultures_tilemap_misc[S_cloud] = V_MISC_CLOUD;
	vultures_tilemap_misc[S_air] = V_TILE_FLOOR_AIR;
	vultures_tilemap_misc[S_grave] = V_MISC_GRAVE;
	vultures_tilemap_misc[S_sink] = V_MISC_SINK;
	vultures_tilemap_misc[S_bear_trap] = V_MISC_TRAP_BEAR;
	vultures_tilemap_misc[S_rust_trap] = V_MISC_TRAP_WATER;
	vultures_tilemap_misc[S_pit] = V_MISC_TRAP_PIT;
	vultures_tilemap_misc[S_hole] = V_MISC_TRAP_PIT;
	vultures_tilemap_misc[S_trap_door] = V_MISC_TRAP_DOOR;
	vultures_tilemap_misc[S_water] = V_TILE_FLOOR_WATER;
	vultures_tilemap_misc[S_pool] = V_TILE_FLOOR_WATER;
	vultures_tilemap_misc[S_ice] = V_TILE_FLOOR_ICE;
	vultures_tilemap_misc[S_lava] = V_TILE_FLOOR_LAVA;
	vultures_tilemap_misc[S_throne] = V_MISC_THRONE;
	vultures_tilemap_misc[S_bars] = V_MISC_BARS;
	vultures_tilemap_misc[S_upladder] = V_MISC_LADDER_UP;
	vultures_tilemap_misc[S_dnladder] = V_MISC_LADDER_DOWN;
	vultures_tilemap_misc[S_arrow_trap] = V_MISC_TRAP_ARROW;
	vultures_tilemap_misc[S_rolling_boulder_trap] = V_MISC_ROLLING_BOULDER_TRAP;
	vultures_tilemap_misc[S_sleeping_gas_trap] = V_MISC_GAS_TRAP;
	vultures_tilemap_misc[S_fire_trap] = V_MISC_TRAP_FIRE;
	vultures_tilemap_misc[S_web] = V_MISC_WEB_TRAP;
	vultures_tilemap_misc[S_statue_trap] = OBJECT_TO_VTILE(STATUE);
	vultures_tilemap_misc[S_anti_magic_trap] = V_MISC_TRAP_ANTI_MAGIC;
	vultures_tilemap_misc[S_polymorph_trap] = V_MISC_TRAP_POLYMORPH;
	vultures_tilemap_misc[S_vbeam] = V_MISC_ZAP_VERTICAL;
	vultures_tilemap_misc[S_hbeam] = V_MISC_ZAP_HORIZONTAL;
	vultures_tilemap_misc[S_lslant] = V_MISC_ZAP_SLANT_LEFT;
	vultures_tilemap_misc[S_rslant] = V_MISC_ZAP_SLANT_RIGHT;
	vultures_tilemap_misc[S_litcorr] = V_TILE_FLOOR_ROUGH_LIT;
	vultures_tilemap_misc[S_ss1] = V_MISC_RESIST_SPELL_1;
	vultures_tilemap_misc[S_ss2] = V_MISC_RESIST_SPELL_2;
	vultures_tilemap_misc[S_ss3] = V_MISC_RESIST_SPELL_3;
	vultures_tilemap_misc[S_ss4] = V_MISC_RESIST_SPELL_4;
	vultures_tilemap_misc[S_dart_trap] = V_MISC_DART_TRAP;
	vultures_tilemap_misc[S_falling_rock_trap] = V_MISC_FALLING_ROCK_TRAP;
	vultures_tilemap_misc[S_squeaky_board] = V_MISC_SQUEAKY_BOARD;
	vultures_tilemap_misc[S_land_mine] = V_MISC_LAND_MINE;
	vultures_tilemap_misc[S_magic_portal] = V_MISC_MAGIC_PORTAL;
	vultures_tilemap_misc[S_spiked_pit] = V_MISC_SPIKED_PIT;
	vultures_tilemap_misc[S_hole] = V_MISC_HOLE;
	vultures_tilemap_misc[S_level_teleporter] = V_MISC_LEVEL_TELEPORTER;
	vultures_tilemap_misc[S_magic_trap] = V_MISC_MAGIC_TRAP;
	vultures_tilemap_misc[S_digbeam] = V_MISC_DIGBEAM;
	vultures_tilemap_misc[S_flashbeam] = V_MISC_FLASHBEAM;
	vultures_tilemap_misc[S_boomleft] = V_MISC_BOOMLEFT;
	vultures_tilemap_misc[S_boomright] = V_MISC_BOOMRIGHT;
	vultures_tilemap_misc[S_hcdbridge] = V_MISC_HCDBRIDGE;
	vultures_tilemap_misc[S_vcdbridge] = V_MISC_VCDBRIDGE;
	vultures_tilemap_misc[S_hodbridge] = V_MISC_HODBRIDGE;
	vultures_tilemap_misc[S_vodbridge] = V_MISC_VODBRIDGE;
}


void mapdata::clear()
{
	int i, j;

	for (i = 0; i < ROWNO; i++)
		for (j = 0; j < COLNO; j++) {
			map_darkness[i][j] = 2;
			/* ideally this is what we'd do to clear background:
			 * map_back[i][j] = V_MISC_UNMAPPED_AREA;
			 * unfortunately doing so breaks dark tiles in rooms... */
			vultures_print_glyph(0, j, i, cmap_to_glyph(S_stone));
			map_trap[i][j] = V_TILE_NONE;
			map_furniture[i][j] = V_TILE_NONE;
			map_obj[i][j] = V_TILE_NONE;
			map_mon[i][j] = V_TILE_NONE;
			map_specialeff[i][j] = V_TILE_NONE;
			map_pet[i][j] = 0;
			map_glyph[i][j] = cmap_to_glyph(S_stone);
		}
	
	/* notify observers */
	for (std::vector<mapviewer*>::iterator i = views.begin(); i != views.end(); ++i)
		(*i)->map_clear();
}



void mapdata::set_glyph(int x, int y, int glyph)
{
	struct obj  *obj;
	struct trap *trap;
	int character, colour;
	int memglyph;

	/* if we're blind, we also need to print what we remember under us */
	if (!cansee(x,y) && x == u.ux && y == u.uy) {
#ifdef DISPLAY_LAYERS
		memglyph = memory_glyph(x,y);
#else
		memglyph = level.locations[x][y].glyph;
#endif
		if (glyph != memglyph)
			set_glyph(x, y, memglyph);
	}

	int map_mon = this->map_mon[y][x];
	int map_obj = this->map_obj[y][x];
	int map_trap = this->map_trap[y][x];
	int map_back = this->map_back[y][x];
	int map_special = V_TILE_NONE;
	int map_furniture = this->map_furniture[y][x];
	int darkness = this->map_darkness[y][x];
	int prev_darkness = darkness;
	int is_pet = 0;
	unsigned int attr;

	set_map_data(MAP_GLYPH, x, y, glyph, false);

	/* check wether we are swallowed, if so, return, because nothing but the swallow graphic will be drawn anyway */
	if (glyph_is_swallow(glyph)) {
		/* we only SET vultures_map_swallow here; we expect vultures_draw_level to reset it */
		map_swallow = (vultures_tilemap_engulf[(glyph-GLYPH_SWALLOW_OFF) >> 3]);
		return;
	}


	/* Nethack will only show us one glyph per position. We need up to 4 "things" per mapsquare:
	* monsters, objects, traps & furniture, floor & walls.
	* Therefore we rely on the glyph only for monsters (which are never covered) and magical vision
	* where we can't just display eg the object a telepathically seen monster is standing on,
	* because that would be cheating. */
	if (cansee(x,y))
	{
		/* various special effects, these occur only during the player's turn and should be
		* layered on top of everything else */
		if (glyph >= GLYPH_ZAP_OFF && glyph < GLYPH_WARNING_OFF)
			map_special = vultures_tilemap_misc[S_vbeam + ((glyph - GLYPH_ZAP_OFF) & 0x03)];
		else if (glyph >= GLYPH_EXPLODE_OFF && glyph < GLYPH_ZAP_OFF)
			map_special = EXPTILEOFFSET + (glyph - GLYPH_EXPLODE_OFF);
		else if (glyph_to_cmap(glyph) >= S_digbeam && glyph_to_cmap(glyph) <= S_ss4)
			/* digbeam, camera flash, boomerang, magic resistance: these are not floor tiles ... */
			map_special = vultures_tilemap_misc[glyph_to_cmap(glyph)];

		/* if the player is invisible and can't see himself, nethack does not print a
		* glyph for him; we need to insert it ourselves */
		if (x == u.ux && y == u.uy && !canseeself()) {
			set_map_data(MAP_GLYPH, x, y, monnum_to_glyph(u.umonnum), false);
			is_pet = 0;
			map_mon = V_MISC_PLAYER_INVIS;
		}
		/* We rely on the glyph for monsters, as they are never covered by anything
		* at the start of the turn and dealing with them manually is ugly */
		else if (glyph_is_monster(glyph)) {
			/* we need special attributes, so that we can highlight the pet */
			mapglyph(glyph, &character, &colour, &attr, x, y);
			is_pet = attr & MG_PET;
			map_mon =  vultures_monster_to_tile(glyph_to_mon(glyph), x, y);
		}
		/* handle invisible monsters */
		else if (glyph_is_invisible(glyph)) {
			mapglyph(glyph, &character, &colour, &attr, x, y);
			is_pet = attr & MG_PET;
			map_mon = V_MISC_INVISIBLE_MONSTER;
		}
		/* handle monsters you are warned of */
		else if (glyph_is_warning(glyph)) {
			map_mon = V_MISC_WARNLEV_1 + glyph_to_warning(glyph);
			mapglyph(glyph, &character, &colour, &attr, x, y);
			is_pet = attr & MG_PET;
		}
		/* however they may be temporarily obscured by magic effects... */
		else if (map_special == V_TILE_NONE)
			map_mon = V_TILE_NONE;

		/* visible objects that are not covered by lava */
		if ((obj = vobj_at(x,y)) && !covers_objects(x,y)) {
			/* glyph_to_obj(obj_to_glyph(foo)) looks like nonsense, but is actually an elegant
			* way of handling hallucination, especially the complicated matter of hallucinated
			* corpses... */
			map_obj = vultures_object_to_tile(glyph_to_obj(obj_to_glyph( obj)), x, y, NULL);
		}
		/* just to make things interesting, the above does not handle thrown/kicked objects... */
		else if (glyph_is_object(glyph))
			map_obj = vultures_object_to_tile(glyph_to_obj(glyph), x, y, NULL);
		else
			map_obj = V_TILE_NONE;

		/* traps that are not covered by lava and have been seen */
		if ((trap = t_at(x,y)) && !covers_traps(x,y) && trap->tseen)
			/* what_trap handles hallucination */
			map_trap = vultures_tilemap_misc[what_trap(trap->ttyp) + S_arrow_trap - 1];
		else
			map_trap = V_TILE_NONE;

		/* handle furniture: altars, stairs,...
		* furniture is separated out from walls and floors, so that it can be used on any
		* type of floor, rather than producing a discolored blob*/
		if (glyph_to_cmap(glyph) >= S_upstair && glyph_to_cmap(glyph) <= S_fountain) {
			/* this is a mimic pretending to be part of the dungeon */
			map_furniture = vultures_tilemap_misc[glyph_to_cmap(glyph)];
			map_back = V_TILE_FLOOR_COBBLESTONE;
		}
		else if (IS_FURNITURE(level.locations[x][y].typ)) {
			map_furniture = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
			/* furniture is only found in rooms, so set the background type */
			map_back = V_TILE_FLOOR_COBBLESTONE;
		}
		else if (glyph_to_cmap(glyph) >= S_ndoor && glyph_to_cmap(glyph) <= S_tree) {
			/* this is a mimic pretending to be part of the dungeon */
			map_furniture = vultures_tilemap_misc[glyph_to_cmap(glyph)];
			map_back = V_TILE_FLOOR_ROUGH;
		}
		else if (V_EXTRA_FURNITURE(level.locations[x][y].typ)) {
			/* this stuff is not furniture, but we need to draw it at the same time,
			* so we pack it in with the furniture ...*/
			map_furniture = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
			map_back = V_TILE_FLOOR_ROUGH;
		}
		else {
			map_furniture = V_TILE_NONE;
			map_back = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
			if (map_special == V_TILE_NONE && glyph_to_cmap(glyph) == S_cloud && back_to_glyph(x,y) != glyph)
				map_special = V_MISC_STINKING_CLOUD;
		}

		/* physically seen tiles cannot be dark */
		darkness = 0;
	}

	/* location seen via some form of magical vision OR
	* when a save is restored*/
	else {
		/* if we can see a monster here, it will be re-shown explicitly */
		map_mon = V_TILE_NONE;

		/* need to clear special effect explicitly here:
		* if we just got blinded by lightnig, the beam will remain onscreen otherwise */
		map_special = V_TILE_NONE;

		/* monsters */
		if (glyph_is_monster(glyph)) {
			mapglyph(glyph, &character, &colour, &attr, x, y);
			is_pet = attr & MG_PET;
			map_mon = vultures_monster_to_tile(glyph_to_mon(glyph), x, y);

			/* if seen telepathically in an unexplored area, it might not have a floor */
			if (map_back == V_MISC_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
				map_back = V_MISC_FLOOR_NOT_VISIBLE;
		}

		/* handle invisible monsters */
		else if (glyph_is_invisible(glyph)) {
			mapglyph(glyph, &character, &colour, &attr, x, y);
			is_pet = attr & MG_PET;
			map_mon = V_MISC_INVISIBLE_MONSTER;
		}

		/* handle monsters you are warned of */
		else if (glyph_is_warning(glyph)) {
			map_mon = V_MISC_WARNLEV_1 + glyph_to_warning(glyph);
			if (map_back == V_MISC_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
				map_back = V_MISC_FLOOR_NOT_VISIBLE;
		}

		/* same as above, for objects */
		else if (glyph_is_object(glyph)) {
			map_obj = vultures_object_to_tile(glyph_to_obj(glyph), x, y, NULL);
			if (map_back == V_MISC_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
				map_back = V_MISC_FLOOR_NOT_VISIBLE;
		}

		/* traps are not seen magically (I think?), so this only triggers when loading a level */
		else if (glyph_is_trap(glyph)) {
			map_trap = vultures_tilemap_misc[glyph_to_cmap(glyph)];
			if (map_back == V_MISC_UNMAPPED_AREA)
				map_back = V_MISC_FLOOR_NOT_VISIBLE;
		}
	
		else if (glyph_is_cmap(glyph)) {
			/* Nethack shows us the cmaps, therefore there are no traps or objects here */
			map_obj = V_TILE_NONE;
			map_trap = V_TILE_NONE;

			/* IS_FURNITURE() may be true while the cmap is S_stone for dark rooms  */
			if (IS_FURNITURE(level.locations[x][y].typ) && glyph_to_cmap(glyph) != S_stone) {
				map_furniture = vultures_tilemap_misc[glyph_to_cmap(glyph)];
				map_back = V_TILE_FLOOR_COBBLESTONE;
			}
			else if (V_EXTRA_FURNITURE(level.locations[x][y].typ) &&
				     glyph_to_cmap(glyph) != S_stone) {
				/* V_EXTRA_FURNITURE = doors, drawbridges, iron bars
				* that stuff is not furniture, but we need to draw it at the same time,
				* so we pack it in with the furniture ...*/
				map_furniture = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
				map_back = V_TILE_FLOOR_ROUGH;
			}
			else {
				map_furniture = V_TILE_NONE;
				map_back = vultures_tilemap_misc[glyph_to_cmap(glyph)];
			}
		}

		/* When a save is restored, we are shown a number of glyphs for objects, traps, etc
		* whose background we actually know and can display, even though we can't physically see it*/
		if (level.locations[x][y].seenv != 0 && map_back == V_MISC_FLOOR_NOT_VISIBLE) {
			if (IS_FURNITURE(level.locations[x][y].typ)) {
				map_furniture = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
				/* furniture is only found in rooms, so set the background type */
				map_back = V_TILE_FLOOR_COBBLESTONE;
			}
			else if (V_EXTRA_FURNITURE(level.locations[x][y].typ)) {
				/* this stuff is not furniture, but we need to draw it at the same time,
				* so we pack it in with the furniture ...*/
				map_furniture = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
				map_back = V_TILE_FLOOR_ROUGH;
			}
			else {
				map_furniture = V_TILE_NONE;
				map_back = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
			}
		}

		/* handle unlit room tiles; until now we were assuming them to be lit; whereas tiles
		* converted via vultures_tilemap_misc are currently V_TILE_NONE */
		if ( ((!level.locations[x][y].waslit) && map_back == V_TILE_FLOOR_COBBLESTONE) ||
		      (level.locations[x][y].typ == ROOM && map_back == V_MISC_UNMAPPED_AREA &&
		       level.locations[x][y].seenv)) {
			map_back = V_TILE_FLOOR_COBBLESTONE;
			darkness = 2;
		}
		else
			darkness = 1;
	}

	/* if the lightlevel changed, we need to force the tile to be updated */
	int force_update = (prev_darkness != darkness);


	/* fix drawbriges, so they don't start in the middle of the moat when they're open */
	/* horizontal drawbridges */
	if (level.locations[x][y].typ == 22 && 
	    level.locations[x-1][y].typ == DRAWBRIDGE_DOWN && level.locations[x-1][y].seenv) {
		map_furniture = V_MISC_HODBRIDGE;
		switch (level.locations[x][y].drawbridgemask & DB_UNDER)
		{
			case DB_MOAT:
				set_map_data(MAP_BACK, x-1, y, vultures_tilemap_misc[S_pool], force_update);
				break;

			case DB_LAVA:
				set_map_data(MAP_BACK, x-1, y, vultures_tilemap_misc[S_lava], force_update);
				break;

			case DB_ICE:
				set_map_data(MAP_BACK, x-1, y, vultures_tilemap_misc[S_ice], force_update);
				break;

			case DB_FLOOR:
				set_map_data(MAP_BACK, x-1, y, vultures_tilemap_misc[S_room], force_update);
				break;
		}
	}


	/* vertical drawbridges */
	if (level.locations[x][y].typ == 22 &&
	    level.locations[x][y-1].typ == DRAWBRIDGE_DOWN && level.locations[x][y-1].seenv) {
		map_furniture = V_MISC_VODBRIDGE;
		switch (level.locations[x][y].drawbridgemask & DB_UNDER)
		{
			case DB_MOAT:
				set_map_data(MAP_BACK, x, y-1, vultures_tilemap_misc[S_pool], force_update);
				break;

			case DB_LAVA:
				set_map_data(MAP_BACK, x, y-1, vultures_tilemap_misc[S_lava], force_update);
				break;

			case DB_ICE:
				set_map_data(MAP_BACK, x, y-1, vultures_tilemap_misc[S_ice], force_update);
				break;

			case DB_FLOOR:
				set_map_data(MAP_BACK, x, y-1, vultures_tilemap_misc[S_room], force_update);
				break;
		}
	}


	/* write new map data */
	set_map_data(MAP_MON, x, y, map_mon, force_update);
	set_map_data(MAP_OBJ, x, y, map_obj, force_update);
	set_map_data(MAP_TRAP, x, y, map_trap, force_update);
	set_map_data(MAP_SPECIAL, x, y, map_special, force_update);
	set_map_data(MAP_FURNITURE, x, y, map_furniture, force_update);
	set_map_data(MAP_BACK, x, y, map_back, force_update);
	set_map_data(MAP_DARKNESS, x, y, darkness, force_update);
	set_map_data(MAP_PET, x, y, is_pet, false); /* you can't be your own pet */
}



/* convert an action_id into an actual key to be passed to nethack to perform the action */
int mapdata::perform_map_action(int action_id, point mappos)
{
	switch (action_id)
	{
		case V_ACTION_TRAVEL:
			u.tx = mappos.x;
			u.ty = mappos.y;
			return CMD_TRAVEL;

		case V_ACTION_MOVE_HERE:
			return mappos_to_dirkey(mappos);

		case V_ACTION_LOOT:        return META('l');
		case V_ACTION_PICK_UP:     return ',';
		case V_ACTION_GO_DOWN:     return '>';
		case V_ACTION_GO_UP:       return '<';
		case V_ACTION_SEARCH:      return 's';
		case V_ACTION_ENGRAVE:     return 'E';
		case V_ACTION_LOOK_AROUND: return ':';
		case V_ACTION_PAY_BILL:    return 'p';
		case V_ACTION_OFFER:       return META('o');
		case V_ACTION_PRAY:        return META('p');
		case V_ACTION_REST:        return '.';
		case V_ACTION_SIT:         return META('s');
		case V_ACTION_TURN_UNDEAD: return META('t');
		case V_ACTION_WIPE_FACE:   return META('w');
		case V_ACTION_FORCE_LOCK:  return META('f');
		case V_ACTION_MONSTER_ABILITY: return META('m');

		case V_ACTION_DRINK:
			vultures_eventstack_add(mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
      return 'q';

		case V_ACTION_READ:
			vultures_eventstack_add(mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
			return 'r';

		case V_ACTION_KICK:
			vultures_eventstack_add(mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
			return CTRL('d');

		case V_ACTION_OPEN_DOOR:
			vultures_eventstack_add(mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
			return 'o';

		case V_ACTION_UNTRAP:
			vultures_eventstack_add(mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
			return META('u');

		case V_ACTION_CLOSE_DOOR:
			vultures_eventstack_add(mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
			return 'c';

		case V_ACTION_WHATS_THIS:
			/* events get stored on a STACK, so calls to eventstack add 
			* need to be done in revers order */
			vultures_whatis_singleshot = 1;

			/* select the mapsquare */
			vultures_eventstack_add(0, mappos.x, mappos.y, V_RESPOND_POSKEY);

			/* yes, we _really do_ want more info */
			vultures_eventstack_add('y', -1, -1, V_RESPOND_CHARACTER);

			return '/';

		case V_ACTION_CHAT:
			vultures_eventstack_add(mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
			return META('c');

		case V_ACTION_FIGHT:
			vultures_eventstack_add(mappos_to_dirkey(mappos),-1,-1, V_RESPOND_POSKEY);
			return 'F';

		case V_ACTION_FIRE:
			return 'f';

		case V_ACTION_NAMEMON:
			vultures_eventstack_add(0, mappos.x, mappos.y, V_RESPOND_POSKEY);
			return 'C';


		default:
			break;
	}
	return 0;
}


/* converts a mappos adjacent to the player to the dirkey pointing in that direction */
char mapdata::mappos_to_dirkey(point mappos)
{
	const char chartable[3][3] = {{'7', '8', '9'},
								{'4', '.', '6'}, 
								{'1', '2', '3'}};
	int dx = mappos.x - u.ux;
	int dy = mappos.y - u.uy;

	if (dx < -1 || dx > 1 || dy < -1 || dy > 1)
		return 0;

	return vultures_numpad_to_hjkl(chartable[dy+1][dx+1], 0);
}


eventresult mapdata::handle_click(void* result, int button, point mappos)
{
	int retval, action_id = 0;

	/* if vultures_whatis_active is set, we want a location (for look or teleport) */
	if (vultures_whatis_active)
	{
		((vultures_event*)result)->num = 0;
		((vultures_event*)result)->x = mappos.x;
		((vultures_event*)result)->y = mappos.y;
		return V_EVENT_HANDLED_FINAL;
	}

	/* else  */
	/* right click: try to resolve the click on the map to a default action */
	if (button == SDL_BUTTON_LEFT)
		action_id = get_map_action(mappos);
	/* left click: allow the user to choose from a context menu */
	else if (button == SDL_BUTTON_RIGHT)
		action_id = get_map_contextmenu(mappos);

	/* if an action was chosen, return it and leave the event loop */
	if (action_id)
	{
		retval = perform_map_action(action_id, mappos);
		if (retval)
		{
			((vultures_event*)result)->num = retval;
			return V_EVENT_HANDLED_FINAL;
		}
	}

	return V_EVENT_HANDLED_NOREDRAW;
}


void mapdata::set_map_data(glyph_type type, int x, int y, int newval, bool force)
{
	int (*data_array)[ROWNO][COLNO], prevtile;
	
	switch (type) {
		case MAP_MON: data_array = &map_mon; break;
		case MAP_OBJ: data_array = &map_obj; break;
		case MAP_TRAP: data_array = &map_trap; break;
		case MAP_BACK: data_array = &map_back; break;
		case MAP_SPECIAL: data_array = &map_specialeff; break;
		case MAP_FURNITURE: data_array = &map_furniture; break;
		case MAP_DARKNESS: data_array = &map_darkness; break;
		case MAP_PET: data_array = &map_pet; break;
		case MAP_GLYPH:
			/* raw glyph values are only stored, not printed */
			map_glyph[y][x] = newval;
			return;
    default:
      throw "Invalid type for map data";
	}

	if ((*data_array)[y][x] != newval || force) {
		prevtile = (*data_array)[y][x];

		for (std::vector<mapviewer*>::iterator i = views.begin(); i != views.end(); ++i)
			(*i)->map_update(type, prevtile, newval, x, y);
		
		(*data_array)[y][x] = newval;
	}
}


int mapdata::get_glyph(glyph_type type, int x, int y) const
{
	switch (type) {
		case MAP_MON: return map_mon[y][x];
		case MAP_OBJ: return map_obj[y][x];
		case MAP_TRAP: return map_trap[y][x];
		case MAP_BACK: return map_back[y][x];
		case MAP_SPECIAL: return map_specialeff[y][x];
		case MAP_FURNITURE: return map_furniture[y][x];
		case MAP_DARKNESS: return map_darkness[y][x];
		case MAP_PET: return map_pet[y][x];
		case MAP_GLYPH: return map_glyph[y][x];
		
		default: return -1;
	}
}


std::string mapdata::map_square_description(point target, int include_seen)
{
	struct permonst *pm;
  std::string out_str = "";
	char monbuf[BUFSZ], temp_buf[BUFSZ], coybuf[BUFSZ], look_buf[BUFSZ];
	struct monst *mtmp = (struct monst *) 0;
	const char *firstmatch;
	int n_objs;
	struct obj * obj;

	if ((target.x < 1) || (target.x >= COLNO) ||
	    (target.y < 0) || (target.y >= ROWNO))
		return out_str;

	/* All of monsters, objects, traps and furniture get descriptions */
	if ((map_mon[target.y][target.x] != V_TILE_NONE)) {
		look_buf[0] = '\0';
		monbuf[0] = '\0';
		pm = lookat(target.x, target.y, look_buf, monbuf);
		firstmatch = look_buf;
		if (look_buf[0]) {
			mtmp = m_at(target.x, target.y);
			Sprintf(temp_buf, "%s", (pm == &mons[PM_COYOTE]) ? coyotename(mtmp,coybuf) : firstmatch);
			out_str = temp_buf;
		}
		if (include_seen) {
			if (monbuf[0]) {
				sprintf(temp_buf, " [seen: %s]", monbuf);
				out_str += temp_buf;
			}
		}
	}
	else if (map_obj[target.y][target.x] != V_TILE_NONE) {
		look_buf[0] = '\0';
		monbuf[0] = '\0';
		lookat(target.x, target.y, look_buf, monbuf);
		
		n_objs = 0;
		obj = level.objects[target.x][target.y];
		while(obj) {
			n_objs++;
			obj = obj->nexthere;
		}
		
		if (n_objs > 1) {
			snprintf(temp_buf, BUFSZ, "%s (+%d other object%s)", look_buf, n_objs - 1, (n_objs > 2) ? "s" : "");
			out_str = temp_buf;
		} else
			out_str = look_buf;
	}
	else if ((map_trap[target.y][target.x] != V_TILE_NONE) ||
			(map_furniture[target.y][target.x] != V_TILE_NONE)) {
		lookat(target.x, target.y, look_buf, monbuf);
		out_str = look_buf;
	}

	return out_str;
}


map_action mapdata::get_map_action(point mappos)
{
	int mapglyph_offset;

	/* Off-map squares have no default action */
	if ((mappos.x < 1) || (mappos.x >= COLNO) ||
		(mappos.y < 0) || (mappos.y >= ROWNO))
		return V_ACTION_NONE;


	/* Target is at least 2 squares away */
	if ((abs(u.ux-mappos.x) >= 2) || (abs(u.uy-mappos.y) >= 2))
		return V_ACTION_TRAVEL;

	/* Monster on target square */
	if (map_mon[mappos.y][mappos.x] != V_TILE_NONE &&
		(u.ux != mappos.x || u.uy != mappos.y))
		return V_ACTION_MOVE_HERE;

	/* Object on target square */
	if (map_obj[mappos.y][mappos.x] != V_TILE_NONE &&
		u.ux == mappos.x && u.uy == mappos.y)
	{
		mapglyph_offset = map_obj[mappos.y][mappos.x];
		switch(mapglyph_offset - OBJTILEOFFSET)
		{
			case LARGE_BOX:
			case ICE_BOX:
			case CHEST:
				return V_ACTION_LOOT;

			default:
				return V_ACTION_PICK_UP;
		}
	}

	/* map feature on target square */
	if (map_furniture[mappos.y][mappos.x] != V_TILE_NONE)
	{
		if ((u.ux == mappos.x) && (u.uy == mappos.y))
			switch (map_furniture[mappos.y][mappos.x])
			{
				case V_MISC_STAIRS_DOWN:
				case V_MISC_LADDER_DOWN:
					return V_ACTION_GO_DOWN;

				case V_MISC_STAIRS_UP:
				case V_MISC_LADDER_UP:
					return V_ACTION_GO_UP;

				case V_MISC_FOUNTAIN:
				case V_MISC_SINK:
#ifdef VULTURESCLAW
				case V_MISC_TOILET:
#endif
					return V_ACTION_DRINK;
			}

		else
			switch (map_furniture[mappos.y][mappos.x])
			{
				case V_MISC_VDOOR_WOOD_CLOSED:
				case V_MISC_HDOOR_WOOD_CLOSED:
					return V_ACTION_OPEN_DOOR;
			}
	}

	/* default action for your own square */
	if (u.ux == mappos.x && u.uy == mappos.y)
		return V_ACTION_SEARCH;

	/* default action for adjacent squares (nonadjacent squares were handled further up)*/
	/* if the square contains an object and there is no monster ther, use trvel after all
	* to suppress the messegebox listing the objects */
	if (map_obj[mappos.y][mappos.x] && !level.monsters[mappos.x][mappos.y])
		return V_ACTION_TRAVEL;

	if (u.ux != mappos.x || u.uy != mappos.y)
		return V_ACTION_MOVE_HERE;

	return V_ACTION_NONE;
}


/* display a context menu for the given map location and return the chosen action */
map_action mapdata::get_map_contextmenu(point mappos)
{
	contextmenu *menu;
	int mapglyph_offset;
	int result;
	point mouse_pos = vultures_get_mouse_pos();


	/* Dropdown commands are shown only for valid squares */
	if ((mappos. x < 1) || (mappos. x >= COLNO) || (mappos. y < 0) || (mappos. y >= ROWNO))
		return V_ACTION_NONE;

	/* Construct a context-sensitive drop-down menu */
	menu = new contextmenu(levwin /*, mouse_pos */);

	if ((u.ux == mappos. x) && (u.uy == mappos. y))
	{
		/* Add personal options: */
		menu->add_item("Fire", V_ACTION_FIRE);

		menu->add_item("Engrave", V_ACTION_ENGRAVE);
		menu->add_item("Look around", V_ACTION_LOOK_AROUND);
		menu->add_item("Monster ability", V_ACTION_MONSTER_ABILITY);

		if (*u.ushops)
			menu->add_item("Pay bill", V_ACTION_PAY_BILL);

		menu->add_item("Pray", V_ACTION_PRAY);
		menu->add_item("Rest", V_ACTION_REST);
		menu->add_item("Search", V_ACTION_SEARCH);
		menu->add_item("Sit", V_ACTION_SIT);

		/* do a minimum check to leave turn undead out for those who _definitely_ can't do it */
#ifdef VULTURESEYE
		if (Role_if(PM_PRIEST) || Role_if(PM_KNIGHT) ||
#else /* VULTURESCLAW */
		if (tech_known(T_TURN_UNDEAD) ||
#endif
			objects[SPE_TURN_UNDEAD].oc_name_known)
			menu->add_item("Turn undead", V_ACTION_TURN_UNDEAD);

		menu->add_item("Wipe face", V_ACTION_WIPE_FACE);
	}

	/* monster options */
	else if (map_mon[mappos. y][mappos. x] != V_TILE_NONE)
		if ((abs(u.ux-mappos. x) <= 1) && (abs(u.uy-mappos. y) <= 1))
		{
			menu->add_item("Chat", V_ACTION_CHAT);
			menu->add_item("Fight", V_ACTION_FIGHT);
			menu->add_item("Name", V_ACTION_NAMEMON);
		}


	/* Add object options: */
	if (map_obj[mappos. y][mappos. x] != V_TILE_NONE &&
		(abs(u.ux-mappos. x) <= 1 && abs(u.uy-mappos. y) <= 1))
	{
		mapglyph_offset =  map_obj[mappos. y][mappos. x];
		switch(mapglyph_offset - OBJTILEOFFSET)
		{
			/* containers have special options */
			case LARGE_BOX:
			case ICE_BOX:
			case CHEST:
				if ((u.ux == mappos. x) && (u.uy == mappos. y))
				{
					menu->add_item("Force lock", V_ACTION_FORCE_LOCK);
					menu->add_item("Loot", V_ACTION_LOOT);
					menu->add_item("Pick up", V_ACTION_PICK_UP);
				}
				menu->add_item("Untrap", V_ACTION_UNTRAP);
				break;

			case SACK:
			case OILSKIN_SACK:
			case BAG_OF_HOLDING:
			case BAG_OF_TRICKS:
				if ((u.ux == mappos. x) && (u.uy == mappos. y))
				{
					menu->add_item("Loot", V_ACTION_LOOT);
					menu->add_item("Pick up", V_ACTION_PICK_UP);
				}
				break;

			/* all other objects can merly be picked up */
			default:
				if ((u.ux == mappos. x) && (u.uy == mappos. y))
					menu->add_item("Pick up", V_ACTION_PICK_UP);
				break;
		}
	}


	/* Add location options: */
	if (map_furniture[mappos. y][mappos. x] != V_TILE_NONE &&
		abs(u.ux-mappos. x) <= 1 && abs(u.uy-mappos. y) <= 1)
	{
		switch(map_furniture[mappos. y][mappos. x])
		{
			case V_MISC_STAIRS_DOWN: case V_MISC_LADDER_DOWN:
				if ((u.ux == mappos. x) && (u.uy == mappos. y))
					menu->add_item("Go down", V_ACTION_GO_DOWN);
				break;

			case V_MISC_STAIRS_UP: case V_MISC_LADDER_UP:
				if ((u.ux == mappos. x) && (u.uy == mappos. y))
					menu->add_item("Go up", V_ACTION_GO_UP);
				break;

			case V_MISC_FOUNTAIN: case V_MISC_SINK:
#ifdef VULTURESCLAW
			case V_MISC_TOILET:
#endif
				if ((u.ux == mappos. x) && (u.uy == mappos. y))
					menu->add_item("Drink", V_ACTION_DRINK);
				break;

			case V_MISC_VDOOR_WOOD_OPEN: case V_MISC_HDOOR_WOOD_OPEN:
				if ((u.ux != mappos. x) || (u.uy != mappos. y))
				{
					menu->add_item("Close door", V_ACTION_CLOSE_DOOR);
					menu->add_item("Untrap", V_ACTION_UNTRAP);
					menu->add_item("Kick", V_ACTION_KICK);
				}
				break;

			case V_MISC_VDOOR_WOOD_CLOSED: case V_MISC_HDOOR_WOOD_CLOSED:
				if ((u.ux != mappos. x) || (u.uy != mappos. y))
				{
					menu->add_item("Open door", V_ACTION_OPEN_DOOR);
					menu->add_item("Untrap", V_ACTION_UNTRAP);
					menu->add_item("Kick", V_ACTION_KICK);
				}
				break;

			case V_MISC_ALTAR:
				if ((u.ux == mappos. x) && (u.uy == mappos. y))
					menu->add_item("Offer", V_ACTION_OFFER);
				else
					menu->add_item("Kick", V_ACTION_KICK);
				break;

      case V_MISC_GRAVE:
				if ((u.ux == mappos. x) && (u.uy == mappos. y))
					menu->add_item("Read", V_ACTION_READ);
        break;

			default:
				if ((u.ux != mappos. x) || (u.uy != mappos. y))
					menu->add_item("Kick", V_ACTION_KICK);
				break;
		}
	}

	/* (known) traps */
	if (map_trap[mappos. y][mappos. x] != V_TILE_NONE)
	{
		menu->add_item("Untrap", V_ACTION_UNTRAP);
		if ((u.ux != mappos. x) || (u.uy != mappos. y))
			if ((abs(u.ux-mappos. x) <= 1) && (abs(u.uy-mappos. y) <= 1))
				menu->add_item("Enter trap", V_ACTION_MOVE_HERE);
	}

	/* move to and look will work for every (mapped) square */
	if (map_back[mappos. y][mappos. x] != V_TILE_NONE)
	{
		if ((u.ux != mappos. x) || (u.uy != mappos. y))    
			menu->add_item("Move here", V_ACTION_TRAVEL);
		menu->add_item("What's this?", V_ACTION_WHATS_THIS);
	}

	menu->layout();
	vultures_event_dispatcher(&result, V_RESPOND_INT, menu);

	delete menu;

	return (map_action)result;
}


void mapdata::add_viewer(mapviewer *v)
{
	views.push_back(v);
}

void mapdata::del_viewer(mapviewer *v)
{
	for (std::vector<mapviewer*>::iterator i = views.begin(); i != views.end(); ++i) {
		if (*i == v) {
			views.erase(i);
			return;
		}
	}
}
