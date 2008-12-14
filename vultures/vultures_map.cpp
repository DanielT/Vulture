/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000 */
/* Copyright (c) Daniel Thaler, 2005 */


#include "vultures_win.h"
#include "vultures_map.h"
#include "vultures_gfl.h"
#include "vultures_tile.h"
#include "vultures_tileconfig.h"
#include "vultures_gra.h"
#include "vultures_sdl.h"
#include "vultures_main.h"
#include "vultures_tile.h"
#include "vultures_txt.h"
#include "vultures_opt.h"
#include "vultures_gen.h"
#include "vultures_mou.h"


#include "window_types.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#include "epri.h"

/* lookat isn't in the nethack headers, but we use it here ... */
extern "C" struct permonst * lookat(int, int, char *, char *);
extern "C" short glyph2tile[];

#define CMD_TRAVEL (char)0x90
#define META(c) (0x80 | (c))
#define CTRL(c) (0x1f & (c))


/* the only other cmaps beside furniture that have significant height and
* therefore need to be drawn as part of the second pass with the furniture. */
#define V_EXTRA_FURNITURE(typ) ((typ) == DOOR || \
								(typ) == DBWALL || \
								(typ) == IRONBARS || \
								(typ) == TREE)




/*----------------------------
* global variables
*---------------------------- */


static int * vultures_tilemap_engulf;


int vultures_map_draw_msecs = 0;
int vultures_map_draw_lastmove = 0;
point vultures_map_highlight = {-1, -1};

int vultures_map_highlight_objects = 0;


static int vultures_tilemap_misc[MAXPCHARS];


/*----------------------------
* pre-declared functions
*---------------------------- */
static int vultures_monster_to_tile(int mon_id, XCHAR_P x, XCHAR_P y);
static char vultures_mappos_to_dirkey(point mappos);
static void vultures_build_tilemap(void);




/*****************************************************************************
* 1) Initializer and destructor
*****************************************************************************/

int vultures_init_map(void)
{

	/* build vultures_tilemap_misc and vultures_tilemap_engulf */
	vultures_build_tilemap();

	return 1;
}



void vultures_destroy_map(void)
{

	/* free nethack to vultures translation tables */
	free (vultures_tilemap_engulf);

}


/*****************************************************************************
* 3) Setting map data
*****************************************************************************/



void vultures_print_glyph(winid window, XCHAR_P x, XCHAR_P y, int glyph)
{
	struct obj  *obj;
	struct trap *trap;
	int character, colour;
	int memglyph;

	levelwin *map;
	map = static_cast<levelwin*>(vultures_get_window(0));

	/* if we're blind, we also need to print what we remember under us */
	if (!cansee(x,y) && x == u.ux && y == u.uy)
	{
#ifdef DISPLAY_LAYERS
		memglyph = memory_glyph(x,y);
#else
		memglyph = level.locations[x][y].glyph;
#endif
		if (glyph != memglyph)
			vultures_print_glyph(window, x, y, memglyph);
	}

	int map_mon = map->get_glyph(MAP_MON, x, y);
	int map_obj = map->get_glyph(MAP_OBJ, x, y);
	int map_trap = map->get_glyph(MAP_TRAP, x, y);
	int map_back = map->get_glyph(MAP_BACK, x, y);
	int map_special = V_TILE_NONE;
	int map_furniture = map->get_glyph(MAP_FURNITURE, x, y);
	int darkness = map->get_glyph(MAP_DARKNESS, x, y);
	int prev_darkness = darkness;
	int is_pet = 0;
	unsigned int attr;

	map->set_map_data(MAP_GLYPH, x, y, glyph, false);

	/* check wether we are swallowed, if so, return, because nothing but the swallow graphic will be drawn anyway */
	if (glyph_is_swallow(glyph))
	{
		/* we only SET vultures_map_swallow here; we expect vultures_draw_level to reset it */
		map->set_swallowed(vultures_tilemap_engulf[(glyph-GLYPH_SWALLOW_OFF) >> 3]);
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
		if (x == u.ux && y == u.uy && !canseeself())
		{
			map->set_map_data(MAP_GLYPH, x, y, monnum_to_glyph(u.umonnum), false);
			is_pet = 0;
			map_mon = V_MISC_PLAYER_INVIS;
		}
		/* We rely on the glyph for monsters, as they are never covered by anything
		* at the start of the turn and dealing with them manually is ugly */
		else if (glyph_is_monster(glyph))
		{
			/* we need special attributes, so that we can highlight the pet */
			mapglyph(glyph, &character, &colour, &attr, x, y);
			is_pet = attr & MG_PET;
			map_mon =  vultures_monster_to_tile(glyph_to_mon(glyph), x, y);
		}
		/* handle invisible monsters */
		else if (glyph_is_invisible(glyph))
		{
			mapglyph(glyph, &character, &colour, &attr, x, y);
			is_pet = attr & MG_PET;
			map_mon = V_MISC_INVISIBLE_MONSTER;
		}
		/* handle monsters you are warned of */
		else if (glyph_is_warning(glyph))
		{
			map_mon = V_MISC_WARNLEV_1 + glyph_to_warning(glyph);
			mapglyph(glyph, &character, &colour, &attr, x, y);
			is_pet = attr & MG_PET;
		}
		/* however they may be temporarily obscured by magic effects... */
		else if (map_special == V_TILE_NONE)
			map_mon = V_TILE_NONE;

		/* visible objects that are not covered by lava */
		if ((obj = vobj_at(x,y)) && !covers_objects(x,y))
		{
			/* glyph_to_obj(obj_to_glyph(foo)) looks like nonsense, but is actually an elegant
			* way of handling hallucination, especially the complicated matter of hallucinated
			* corpses... */
			map_obj = vultures_object_to_tile(glyph_to_obj(obj_to_glyph( obj)), x, y, NULL);
		}
		/* just to make things interesting, the above does not handle thrown/kicked objects... */
		else if (glyph_is_object(glyph))
		{
			map_obj = vultures_object_to_tile(glyph_to_obj(glyph), x, y, NULL);
		}
		else
			map_obj = V_TILE_NONE;

		/* traps that are not covered by lava and have been seen */
		if ((trap = t_at(x,y)) && !covers_traps(x,y) && trap->tseen)
		{
			/* what_trap handles hallucination */
			map_trap = vultures_tilemap_misc[what_trap(trap->ttyp) + S_arrow_trap - 1];
		}
		else
			map_trap = V_TILE_NONE;

		/* handle furniture: altars, stairs,...
		* furniture is separated out from walls and floors, so that it can be used on any
		* type of floor, rather than producing a discolored blob*/
		if (glyph_to_cmap(glyph) >= S_upstair && glyph_to_cmap(glyph) <= S_fountain)
		{
			/* this is a mimic pretending to be part of the dungeon */
			map_furniture = vultures_tilemap_misc[glyph_to_cmap(glyph)];
			map_back = V_TILE_FLOOR_COBBLESTONE;
		}
		else if (IS_FURNITURE(level.locations[x][y].typ))
		{
			map_furniture = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
			/* furniture is only found in rooms, so set the background type */
			map_back = V_TILE_FLOOR_COBBLESTONE;
		}
		else if (glyph_to_cmap(glyph) >= S_ndoor && glyph_to_cmap(glyph) <= S_tree)
		{
			/* this is a mimic pretending to be part of the dungeon */
			map_furniture = vultures_tilemap_misc[glyph_to_cmap(glyph)];
			map_back = V_TILE_FLOOR_ROUGH;
		}
		else if (V_EXTRA_FURNITURE(level.locations[x][y].typ))
		{
			/* this stuff is not furniture, but we need to draw it at the same time,
			* so we pack it in with the furniture ...*/
			map_furniture = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
			map_back = V_TILE_FLOOR_ROUGH;
		}
		else
		{
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
	else
	{
		/* if we can see a monster here, it will be re-shown explicitly */
		map_mon = V_TILE_NONE;

		/* need to clear special effect explicitly here:
		* if we just got blinded by lightnig, the beam will remain onscreen otherwise */
		map_special = V_TILE_NONE;

		/* monsters */
		if (glyph_is_monster(glyph))
		{
			mapglyph(glyph, &character, &colour, &attr, x, y);
			is_pet = attr & MG_PET;
			map_mon = vultures_monster_to_tile(glyph_to_mon(glyph), x, y);

			/* if seen telepathically in an unexplored area, it might not have a floor */
			if (map_back == V_MISC_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
				map_back = V_MISC_FLOOR_NOT_VISIBLE;
		}

		/* handle invisible monsters */
		else if (glyph_is_invisible(glyph))
		{
			mapglyph(glyph, &character, &colour, &attr, x, y);
			is_pet = attr & MG_PET;
			map_mon = V_MISC_INVISIBLE_MONSTER;
		}

		/* handle monsters you are warned of */
		else if (glyph_is_warning(glyph))
		{
			map_mon = V_MISC_WARNLEV_1 + glyph_to_warning(glyph);
			if (map_back == V_MISC_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
				map_back = V_MISC_FLOOR_NOT_VISIBLE;
		}

		/* same as above, for objects */
		else if (glyph_is_object(glyph))
		{
			map_obj = vultures_object_to_tile(glyph_to_obj(glyph), x, y, NULL);
			if (map_back == V_MISC_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
				map_back = V_MISC_FLOOR_NOT_VISIBLE;
		}

		/* traps are not seen magically (I think?), so this only triggers when loading a level */
		else if (glyph_is_trap(glyph))
		{
			map_trap = vultures_tilemap_misc[glyph_to_cmap(glyph)];
			if (map_back == V_MISC_UNMAPPED_AREA)
				map_back = V_MISC_FLOOR_NOT_VISIBLE;
		}
	
		else if (glyph_is_cmap(glyph))
		{
			/* Nethack shows us the cmaps, therefore there are no traps or objects here */
			map_obj = V_TILE_NONE;
			map_trap = V_TILE_NONE;

			/* IS_FURNITURE() may be true while the cmap is S_stone for dark rooms  */
			if (IS_FURNITURE(level.locations[x][y].typ) && glyph_to_cmap(glyph) != S_stone)
			{
				map_furniture = vultures_tilemap_misc[glyph_to_cmap(glyph)];
				map_back = V_TILE_FLOOR_COBBLESTONE;
			}
			else if (V_EXTRA_FURNITURE(level.locations[x][y].typ) && glyph_to_cmap(glyph) != S_stone)
			{
				/* V_EXTRA_FURNITURE = doors, drawbridges, iron bars
				* that stuff is not furniture, but we need to draw it at the same time,
				* so we pack it in with the furniture ...*/
				map_furniture = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
				map_back = V_TILE_FLOOR_ROUGH;
			}
			else
			{
				map_furniture = V_TILE_NONE;
				map_back = vultures_tilemap_misc[glyph_to_cmap(glyph)];
			}
		}

		/* When a save is restored, we are shown a number of glyphs for objects, traps, etc
		* whose background we actually know and can display, even though we can't physically see it*/
		if (level.locations[x][y].seenv != 0 && map_back == V_MISC_FLOOR_NOT_VISIBLE)
		{
			if (IS_FURNITURE(level.locations[x][y].typ))
			{
				map_furniture = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
				/* furniture is only found in rooms, so set the background type */
				map_back = V_TILE_FLOOR_COBBLESTONE;
			}
			else if (V_EXTRA_FURNITURE(level.locations[x][y].typ))
			{
				/* this stuff is not furniture, but we need to draw it at the same time,
				* so we pack it in with the furniture ...*/
				map_furniture = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
				map_back = V_TILE_FLOOR_ROUGH;
			}
			else
			{
				map_furniture = V_TILE_NONE;
				map_back = vultures_tilemap_misc[glyph_to_cmap(back_to_glyph(x,y))];
			}
		}

		/* handle unlit room tiles; until now we were assuming them to be lit; whereas tiles
		* converted via vultures_tilemap_misc are currently V_TILE_NONE */
		if ( ((!level.locations[x][y].waslit) && map_back == V_TILE_FLOOR_COBBLESTONE) ||
		(level.locations[x][y].typ == ROOM && map_back == V_MISC_UNMAPPED_AREA &&
		level.locations[x][y].seenv))
		{
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
	level.locations[x-1][y].typ == DRAWBRIDGE_DOWN && level.locations[x-1][y].seenv)
	{
		map_furniture = V_MISC_HODBRIDGE;
		switch (level.locations[x][y].drawbridgemask & DB_UNDER)
		{
			case DB_MOAT:
				map->set_map_data(MAP_BACK, x-1, y, vultures_tilemap_misc[S_pool], force_update);
				break;

			case DB_LAVA:
				map->set_map_data(MAP_BACK, x-1, y, vultures_tilemap_misc[S_lava], force_update);
				break;

			case DB_ICE:
				map->set_map_data(MAP_BACK, x-1, y, vultures_tilemap_misc[S_ice], force_update);
				break;

			case DB_FLOOR:
				map->set_map_data(MAP_BACK, x-1, y, vultures_tilemap_misc[S_room], force_update);
				break;
		}
	}


	/* vertical drawbridges */
	if (level.locations[x][y].typ == 22 &&
	level.locations[x][y-1].typ == DRAWBRIDGE_DOWN && level.locations[x][y-1].seenv)
	{
		map_furniture = V_MISC_VODBRIDGE;
		switch (level.locations[x][y].drawbridgemask & DB_UNDER)
		{
			case DB_MOAT:
				map->set_map_data(MAP_BACK, x, y-1, vultures_tilemap_misc[S_pool], force_update);
				break;

			case DB_LAVA:
				map->set_map_data(MAP_BACK, x, y-1, vultures_tilemap_misc[S_lava], force_update);
				break;

			case DB_ICE:
				map->set_map_data(MAP_BACK, x, y-1, vultures_tilemap_misc[S_ice], force_update);
				break;

			case DB_FLOOR:
				map->set_map_data(MAP_BACK, x, y-1, vultures_tilemap_misc[S_room], force_update);
				break;
		}
	}


	/* write new map data */
	map->set_map_data(MAP_MON, x, y, map_mon, force_update);
	map->set_map_data(MAP_OBJ, x, y, map_obj, force_update);
	map->set_map_data(MAP_TRAP, x, y, map_trap, force_update);
	map->set_map_data(MAP_SPECIAL, x, y, map_special, force_update);
	map->set_map_data(MAP_FURNITURE, x, y, map_furniture, force_update);
	map->set_map_data(MAP_BACK, x, y, map_back, force_update);
	map->set_map_data(MAP_DARKNESS, x, y, darkness, force_update);
	map->set_map_data(MAP_PET, x, y, is_pet, false); /* you can't be your own pet */
}



/*****************************************************************************
* 5) Interface functions for windowing
*****************************************************************************/


/* convert an action_id into an actual key to be passed to nethack to perform the action */
int vultures_perform_map_action(int action_id, point mappos)
{
	switch (action_id)
	{
		case V_ACTION_TRAVEL:
			u.tx = mappos.x;
			u.ty = mappos.y;
			return CMD_TRAVEL;

		case V_ACTION_MOVE_HERE:
			return vultures_mappos_to_dirkey(mappos);

		case V_ACTION_LOOT:        return META('l');
		case V_ACTION_PICK_UP:     return ',';
		case V_ACTION_GO_DOWN:     return '>';
		case V_ACTION_GO_UP:       return '<';
		case V_ACTION_DRINK:       return 'q';
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

		case V_ACTION_KICK:
			vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
			return CTRL('d');

		case V_ACTION_OPEN_DOOR:
			vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
			return 'o';

		case V_ACTION_UNTRAP:
			vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
			return META('u');

		case V_ACTION_CLOSE_DOOR:
			vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
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
			vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
			return META('c');

		case V_ACTION_FIGHT:
			vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_POSKEY);
			return 'F';

		case V_ACTION_NAMEMON:
			vultures_eventstack_add(0, mappos.x, mappos.y, V_RESPOND_POSKEY);
			return 'C';


		default:
			break;
	}
	return 0;
}




/*****************************************************************************
* 6) misc utility functions
*****************************************************************************/

int vultures_object_to_tile(int obj_id, int x, int y, struct obj *in_obj)
{
	struct obj *obj;
	int tile;
	static int lastfigurine = PM_KNIGHT;
	static int laststatue = PM_KNIGHT;


	if (obj_id < 0 || obj_id > NUM_OBJECTS)
		/* we get *seriously weird* input for hallucinated corpses */
		obj_id = CORPSE;


	/* try to find the actual object corresponding to the given obj_id */
	if (in_obj)
		obj = in_obj;
	else
	{
		if (x >= 0)
			obj = level.objects[x][y];
		else
			obj = invent;

		while (obj && !(obj->otyp == obj_id && (x >= 0 || obj->invlet == y)))
			obj = (x >= 0) ? obj->nexthere : obj->nobj;
	}

	/* all amulets, potions, etc look the same when the player is blind */
	if (obj && Blind && !obj->dknown)
	{
		switch (obj->oclass)
		{
			case AMULET_CLASS: return OBJECT_TO_VTILE(AMULET_OF_ESP);
			case POTION_CLASS: return OBJECT_TO_VTILE(POT_WATER);
			case SCROLL_CLASS: return OBJECT_TO_VTILE(SCR_BLANK_PAPER);
			case WAND_CLASS:   return OBJECT_TO_VTILE(WAN_NOTHING);
			case SPBOOK_CLASS: return OBJECT_TO_VTILE(SPE_BLANK_PAPER);
			case RING_CLASS:   return OBJECT_TO_VTILE(RIN_ADORNMENT);
			case GEM_CLASS:
				if (objects[obj_id].oc_material == MINERAL)
					return OBJECT_TO_VTILE(ROCK);
				else
					return OBJECT_TO_VTILE(glassgems[CLR_BLUE]);
		}
	}


	/* figurines and statues get different tiles depending on which monster they represent */
	if (obj_id == STATUE || obj_id == FIGURINE)
	{
		if (obj_id == FIGURINE)
		{
			tile = lastfigurine;
			if (obj)
			{
				tile = obj->corpsenm;
				lastfigurine = tile;
			}

			tile = FIGURINE_TO_VTILE(tile);
		}
		else /* obj_id == STATUE */
		{
			tile = laststatue;
			if (obj)
			{
				tile = obj->corpsenm;
				laststatue = tile;
			}

			tile = STATUE_TO_VTILE(tile);
		}
		return tile;
	}

	/* prevent visual identification of unknown objects */
	return OBJECT_TO_VTILE(vultures_obfuscate_object(obj_id));
}


/* prevent visual identification of unknown objects */
int vultures_obfuscate_object(int obj_id)
{
	/* catch known objects */
	if (objects[obj_id].oc_name_known)
		return obj_id;

	/* revert objects that could be identified by their tiles to a generic
	* representation */
	switch (obj_id)
	{
		case SACK:
		case OILSKIN_SACK:
		case BAG_OF_TRICKS:
		case BAG_OF_HOLDING:
			return SACK;

		case LOADSTONE:
		case LUCKSTONE:
		case FLINT:
		case TOUCHSTONE: 
#ifdef HEALTHSTONE /* only in SlashEM */
		case HEALTHSTONE:
#endif
#ifdef WHETSTONE /* only in SlashEM */
		case WHETSTONE:
#endif
			return FLINT;

		case OIL_LAMP:
		case MAGIC_LAMP:
			return OIL_LAMP;

		case TIN_WHISTLE:
		case MAGIC_WHISTLE:
			return TIN_WHISTLE;

		/* all gems initially look like pieces of glass */
		case DILITHIUM_CRYSTAL:
		case DIAMOND:
		case RUBY:
		case JACINTH:
		case SAPPHIRE:
		case BLACK_OPAL:
		case EMERALD:
		case TURQUOISE:
		case CITRINE:
		case AQUAMARINE:
		case AMBER:
		case TOPAZ:
		case JET:
		case OPAL:
		case CHRYSOBERYL:
		case GARNET:
		case AMETHYST:
		case JASPER:
		case FLUORITE:
		case OBSIDIAN:
		case AGATE:
		case JADE:
			/* select the glass tile at runtime: gem colors get randomized */
			switch (objects[obj_id].oc_color)
			{
				case CLR_RED:     return glassgems[CLR_RED]; break;
				case CLR_BLACK:   return glassgems[CLR_BLACK]; break;
				case CLR_GREEN:   return glassgems[CLR_GREEN]; break;
				case CLR_BROWN:   return glassgems[CLR_BROWN]; break;
				case CLR_MAGENTA: return glassgems[CLR_MAGENTA]; break;
				case CLR_ORANGE:  return glassgems[CLR_ORANGE]; break;
				case CLR_YELLOW:  return glassgems[CLR_YELLOW]; break;
				case CLR_WHITE:   return glassgems[CLR_WHITE]; break;
				case CLR_BLUE:    return glassgems[CLR_BLUE]; break;
				default:          return glassgems[CLR_BLACK]; break;
			}
	}
	/* the vast majority of objects needs no special treatment */
	return obj_id;
}


/* returns the tile for a given monster id */
static int vultures_monster_to_tile(int mon_id, XCHAR_P x, XCHAR_P y)
{
	if (Invis && u.ux == x && u.uy == y)
		return V_MISC_PLAYER_INVIS;

#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz))
	{
		/* convert all monster tiles to 3d-letters on the rogue level */
		switch (mon_id)
		{
			case PM_COUATL : case PM_ALEAX : case PM_ANGEL :
			case PM_KI_RIN : case PM_ARCHON :
				return V_MISC_ROGUE_LEVEL_A;

			case PM_GIANT_BAT : case PM_RAVEN :
			case PM_VAMPIRE_BAT : case PM_BAT :
				return V_MISC_ROGUE_LEVEL_B;

			case PM_PLAINS_CENTAUR : case PM_FOREST_CENTAUR :
			case PM_MOUNTAIN_CENTAUR :
				return V_MISC_ROGUE_LEVEL_C;

			case PM_DOG:
			case PM_BABY_GRAY_DRAGON : case PM_BABY_SILVER_DRAGON :
			case PM_BABY_RED_DRAGON :
			case PM_BABY_WHITE_DRAGON : case PM_BABY_ORANGE_DRAGON :
			case PM_BABY_BLACK_DRAGON : case PM_BABY_BLUE_DRAGON :
			case PM_BABY_GREEN_DRAGON : case PM_BABY_YELLOW_DRAGON :
			case PM_GRAY_DRAGON : case PM_SILVER_DRAGON :
			case PM_RED_DRAGON :
			case PM_WHITE_DRAGON : case PM_ORANGE_DRAGON :
			case PM_BLACK_DRAGON : case PM_BLUE_DRAGON :
			case PM_GREEN_DRAGON : case PM_YELLOW_DRAGON :
				return V_MISC_ROGUE_LEVEL_D;

			case PM_STALKER : case PM_AIR_ELEMENTAL :
			case PM_FIRE_ELEMENTAL: case PM_EARTH_ELEMENTAL :
			case PM_WATER_ELEMENTAL :
				return V_MISC_ROGUE_LEVEL_E;

			case PM_LICHEN : case PM_BROWN_MOLD :
			case PM_YELLOW_MOLD : case PM_GREEN_MOLD :
			case PM_RED_MOLD : case PM_SHRIEKER :
			case PM_VIOLET_FUNGUS :
				return V_MISC_ROGUE_LEVEL_F;

			case PM_GNOME : case PM_GNOME_LORD :
			case PM_GNOMISH_WIZARD : case PM_GNOME_KING :
				return V_MISC_ROGUE_LEVEL_G;

			case PM_GIANT : case PM_STONE_GIANT :
			case PM_HILL_GIANT : case PM_FIRE_GIANT :
			case PM_FROST_GIANT : case PM_STORM_GIANT :
			case PM_ETTIN : case PM_TITAN : case PM_MINOTAUR :
				return V_MISC_ROGUE_LEVEL_H;

			case 999990 :	//None
				return V_MISC_ROGUE_LEVEL_I;

			case PM_JABBERWOCK :
				return V_MISC_ROGUE_LEVEL_J;

			case PM_KEYSTONE_KOP : case PM_KOP_SERGEANT :
			case PM_KOP_LIEUTENANT : case PM_KOP_KAPTAIN :
				return V_MISC_ROGUE_LEVEL_K;

			case PM_LICH : case PM_DEMILICH :
			case PM_MASTER_LICH : case PM_ARCH_LICH :
				return V_MISC_ROGUE_LEVEL_L;

			case PM_KOBOLD_MUMMY : case PM_GNOME_MUMMY :
			case PM_ORC_MUMMY : case PM_DWARF_MUMMY :
			case PM_ELF_MUMMY : case PM_HUMAN_MUMMY :
			case PM_ETTIN_MUMMY : case PM_GIANT_MUMMY :
				return V_MISC_ROGUE_LEVEL_M;

			case PM_RED_NAGA_HATCHLING :
			case PM_BLACK_NAGA_HATCHLING :
			case PM_GOLDEN_NAGA_HATCHLING :
			case PM_GUARDIAN_NAGA_HATCHLING :
			case PM_RED_NAGA : case PM_BLACK_NAGA :
			case PM_GOLDEN_NAGA : case PM_GUARDIAN_NAGA :
				return V_MISC_ROGUE_LEVEL_N;

			case PM_OGRE : case PM_OGRE_LORD :
			case PM_OGRE_KING :
				return V_MISC_ROGUE_LEVEL_O;

			case PM_GRAY_OOZE : case PM_BROWN_PUDDING :
			case PM_BLACK_PUDDING : case PM_GREEN_SLIME :
				return V_MISC_ROGUE_LEVEL_P;

			case PM_QUANTUM_MECHANIC :
				return V_MISC_ROGUE_LEVEL_Q;

			case PM_RUST_MONSTER : case PM_DISENCHANTER :
				return V_MISC_ROGUE_LEVEL_R;

			case PM_GARTER_SNAKE : case PM_SNAKE :
			case PM_WATER_MOCCASIN : case PM_PIT_VIPER :
			case PM_PYTHON : case PM_COBRA :
				return V_MISC_ROGUE_LEVEL_S;

			case PM_TROLL : case PM_ICE_TROLL :
			case PM_ROCK_TROLL : case PM_WATER_TROLL :
			case PM_OLOG_HAI :
				return V_MISC_ROGUE_LEVEL_T;

			case PM_UMBER_HULK :
				return V_MISC_ROGUE_LEVEL_U;

			case PM_VAMPIRE : case PM_VAMPIRE_LORD :
				return V_MISC_ROGUE_LEVEL_V;

			case PM_BARROW_WIGHT : case PM_WRAITH :
			case PM_NAZGUL :
				return V_MISC_ROGUE_LEVEL_W;

			case PM_XORN :
				return V_MISC_ROGUE_LEVEL_X;

			case PM_MONKEY : case PM_APE : case PM_OWLBEAR :
			case PM_YETI : case PM_CARNIVOROUS_APE :
			case PM_SASQUATCH :
				return V_MISC_ROGUE_LEVEL_Y;

			case PM_GHOUL:
			case PM_KOBOLD_ZOMBIE : case PM_GNOME_ZOMBIE :
			case PM_ORC_ZOMBIE : case PM_DWARF_ZOMBIE :
			case PM_ELF_ZOMBIE : case PM_HUMAN_ZOMBIE :
			case PM_ETTIN_ZOMBIE : case PM_GIANT_ZOMBIE :
				return V_MISC_ROGUE_LEVEL_Z;

			default:
			{
				if ((mon_id >= 0) && (mon_id < NUMMONS))
					return MONSTER_TO_VTILE(mon_id);
				else
					return V_TILE_NONE;
			}
		}
	}
#endif
	/* aleaxes are angelic doppelgangers and always look like the player */
	if (mon_id == PM_ALEAX)
		return MONSTER_TO_VTILE(u.umonnum);

	/* we have different tiles for priests depending on their alignment */
	if (mon_id == PM_ALIGNED_PRIEST)
	{
		register struct monst *mtmp = m_at(x, y);

		switch (EPRI(mtmp)->shralign)
		{
			case A_LAWFUL:  return V_MISC_LAWFUL_PRIEST;
			case A_CHAOTIC: return V_MISC_CHAOTIC_PRIEST;
			case A_NEUTRAL: return V_MISC_NEUTRAL_PRIEST;
			default:        return V_MISC_UNALIGNED_PRIEST;
		}
	}

	if ((mon_id >= 0) && (mon_id < NUMMONS))
		return MONSTER_TO_VTILE(mon_id);

	return V_TILE_NONE;
}


/* converts a mappos adjacent to the player to the dirkey pointing in that direction */
static char vultures_mappos_to_dirkey(point mappos)
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


/* certain tile ids need to be acessed via arrays; these are built here */
static void vultures_build_tilemap(void)
{
	int i;
	vultures_tilemap_engulf = (int *)malloc(NUMMONS*sizeof(int));

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
