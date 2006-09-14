/*	SCCS Id: @(#)jtp_win.c	3.0	2000/9/10	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include "jtp_def.h"
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include "jtp_gen.h"
#include "jtp_gra.h"
#include "jtp_mou.h"
#include "jtp_txt.h"
#include "jtp_gfl.h"
#include "jtp_keys.h"
#include "jtp_win.h"
#include "winjtp.h"
#ifdef USE_DIRECTX_SYSCALLS
#include "jtp_dirx.h"
#endif
#ifdef USE_DOS_SYSCALLS
#include "jtp_dos.h"
#endif
#ifdef USE_SDL_SYSCALLS
#include "jtp_sdl.h"
#endif

#include "global.h"
#include "rm.h"
#include "display.h"
#include "patchlevel.h"

/*----------------------------------------------------------------
  Defines (constants)
-----------------------------------------------------------------*/
/* Directory separator: DOS-style (backslash) or Unix-style (slash) */
#ifdef UNIX
#define JTP_USE_UNIX_STYLE_DIRECTORY_SEPARATOR
#endif
#ifdef __BEOS__
#define JTP_USE_UNIX_STYLE_DIRECTORY_SEPARATOR
#endif


/* Wall display styles (game option) */
#define JTP_WALL_DISPLAY_STYLE_FULL 0
#define JTP_WALL_DISPLAY_STYLE_HALF_HEIGHT 1
#define JTP_WALL_DISPLAY_STYLE_TRANSPARENT 2

/* Maximum height (in pixels) of displayed menu items */
#define JTP_MAX_MENUITEMS_HEIGHT 450

/* Location and dimensions of the status window graphic, in pixels */
#define JTP_STATUSBAR_WIDTH 800
#define JTP_STATUSBAR_HEIGHT 100

/* Dimensions of intro slideshow graphics */
#define JTP_INTRO_SLIDE_WIDTH 800
#define JTP_INTRO_SLIDE_HEIGHT 450

/* Dimensions of ending graphics */
#define JTP_ENDING_SLIDE_WIDTH 800
#define JTP_ENDING_SLIDE_HEIGHT 600

/* Dimensions of NetHack logo screen & character generation graphic */
#define JTP_NH_LOGO_WIDTH 800
#define JTP_NH_LOGO_HEIGHT 600

/* Map dimensions in glyphs */
#define JTP_MAP_WIDTH COLNO
#define JTP_MAP_HEIGHT ROWNO

/* Message shading: old messages grow darker */
#define JTP_MAX_MESSAGE_COLORS 16

/* Length of an unknown path (for comparison). Must be greater than any real path! */
#define JTP_UNKNOWN_PATH_LENGTH (3*JTP_MAP_WIDTH*JTP_MAP_HEIGHT)

/* Maximum length of an autopilot move. */
#define JTP_MAX_MOVE_LENGTH (COLNO*ROWNO)

/* Lighting constants */
#define JTP_LIGHTING1 6.0
#define JTP_LIGHTING2 7.0
#define JTP_LIGHTING3 (JTP_MAX_SHADES-1+JTP_LIGHTING1*log(JTP_LIGHTING2))


/* Font indices. Currently, there're only 2 fonts (large & small). */
#define JTP_FONT_INTRO 1
#define JTP_FONT_MENU 0
#define JTP_FONT_HEADLINE 1
#define JTP_FONT_BUTTON 1
#define JTP_FONT_TOOLTIP 0
#define JTP_FONT_STATUS 0
#define JTP_FONT_MESSAGE 0
#define JTP_FONT_INPUT 0

/* Event sound types */
#define JTP_EVENT_SOUND_TYPE_WAVE 0
#define JTP_EVENT_SOUND_TYPE_LONG_WAVE 1
#define JTP_EVENT_SOUND_TYPE_MIDI 2
#define JTP_EVENT_SOUND_TYPE_RANDOM_SONG 3
#define JTP_EVENT_SOUND_TYPE_CD_AUDIO 4
#define JTP_EVENT_SOUND_TYPE_MP3 5
#define JTP_EVENT_SOUND_TYPE_NONE 6

/*
 * Mouse cursor shapes. Note: there's an implicit assumption
 * that mouse cursors will be less than 256 pixels wide/tall. 
 * Similar assumptions are made for many other small graphics.
 */
#define JTP_CURSOR_NORMAL 0
#define JTP_CURSOR_SCROLLLEFT 1
#define JTP_CURSOR_SCROLLRIGHT 2
#define JTP_CURSOR_SCROLLUP 3
#define JTP_CURSOR_SCROLLDOWN 4
#define JTP_CURSOR_TARGET_GREEN 5
#define JTP_CURSOR_TARGET_RED 6
#define JTP_CURSOR_TARGET_INVALID 7
#define JTP_CURSOR_TARGET_HELP 8
#define JTP_CURSOR_HOURGLASS 9
#define JTP_CURSOR_OPENDOOR 10
#define JTP_CURSOR_STAIRS 11
#define JTP_CURSOR_GOBLET 12

/* On-screen hotspots */
#define JTP_HOTSPOT_NONE 0
#define JTP_HOTSPOT_SCROLL_UP 1
#define JTP_HOTSPOT_SCROLL_DOWN 2
#define JTP_HOTSPOT_SCROLL_LEFT 3
#define JTP_HOTSPOT_SCROLL_RIGHT 4
#define JTP_HOTSPOT_MAP 5
#define JTP_HOTSPOT_MINI_MAP 6
#define JTP_HOTSPOT_STATUSBAR 7
#define JTP_HOTSPOT_BUTTON_LOOK 8
#define JTP_HOTSPOT_BUTTON_EXTENDED 9
#define JTP_HOTSPOT_BUTTON_MAP 10
#define JTP_HOTSPOT_BUTTON_SPELLBOOK 11
#define JTP_HOTSPOT_BUTTON_INVENTORY 12
#define JTP_HOTSPOT_BUTTON_MESSAGES 13
#define JTP_HOTSPOT_BUTTON_OPTIONS 14
#define JTP_HOTSPOT_BUTTON_HELP 15

/* Lighting constants */
#define JTP_MAX_LIGHTS 10
#define JTP_BRILLIANCE 65.0

/* 
 * Tile drawing: pixel coordinate difference from a square to
 * the one next to it in the map. Because of isometry,
 * this is not the same as the width/height of a tile!
 */
#ifdef JTP_USE_SMALL_MAP_TILES
#define JTP_MAP_XMOD 46
#define JTP_MAP_YMOD 18
#else
#define JTP_MAP_XMOD 56
#define JTP_MAP_YMOD 22
#endif

/* Floor pattern dimensions (eg. 3x3) */
#define JTP_FLOOR_PATTERN_WIDTH 3
#define JTP_FLOOR_PATTERN_HEIGHT 3

/* Carpet dimensions (eg. 3x2) */
#define JTP_CARPET_WIDTH 3
#define JTP_CARPET_HEIGHT 3

/* Wall styles */
#define JTP_WALL_STYLE_BRICK 0
#define JTP_WALL_STYLE_BRICK_BANNER 1
#define JTP_WALL_STYLE_BRICK_PAINTING 2
#define JTP_WALL_STYLE_BRICK_POCKET 3
#define JTP_WALL_STYLE_BRICK_PILLAR 4
#define JTP_WALL_STYLE_ROUGH 5
#define JTP_WALL_STYLE_STUCCO 6
#define JTP_WALL_STYLE_VINE_COVERED 7
#define JTP_WALL_STYLE_MARBLE 8
#define JTP_MAX_WALL_STYLES 9
/* Floor styles */
#define JTP_FLOOR_STYLE_COBBLESTONE 0
#define JTP_FLOOR_STYLE_ROUGH 1
#define JTP_FLOOR_STYLE_CERAMIC 2
#define JTP_FLOOR_STYLE_MOSS_COVERED 3
#define JTP_FLOOR_STYLE_MARBLE 4
#define JTP_FLOOR_STYLE_LAVA 5
#define JTP_FLOOR_STYLE_WATER 6
#define JTP_FLOOR_STYLE_ICE 7
#define JTP_FLOOR_STYLE_MURAL 8
#define JTP_FLOOR_STYLE_MURAL_2 9
#define JTP_FLOOR_STYLE_CARPET 10
#define JTP_FLOOR_STYLE_ROUGH_LIT 11
#define JTP_FLOOR_STYLE_AIR 12
#define JTP_MAX_FLOOR_STYLES 13

/* Floor edge styles */
#define JTP_FLOOR_EDGE_STYLE_COBBLESTONE 0
#define JTP_MAX_FLOOR_EDGE_STYLES 1

/* Tile graphic indices, cmap tiles */
#define JTP_TILE_INVALID -1
/* Wall tiles */
#define JTP_TILE_WALL_GENERIC 0
#define JTP_TILE_WALL_BRICK 1
#define JTP_TILE_WALL_ROUGH 2
/* Floor tiles */
#define JTP_TILE_FLOOR_COBBLESTONE 3
#define JTP_TILE_FLOOR_ROUGH 4
#define JTP_TILE_FLOOR_LAVA 72
#define JTP_TILE_FLOOR_WATER 73
#define JTP_TILE_FLOOR_ICE 74
#define JTP_TILE_FLOOR_CARPETED 5
#define JTP_TILE_FLOOR_ROUGH_LIT 121
#define JTP_TILE_FLOOR_AIR 133
/* Door tiles */
#define JTP_TILE_VDOOR_WOOD_OPEN 6
#define JTP_TILE_VDOOR_WOOD_CLOSED 7
#define JTP_TILE_HDOOR_WOOD_OPEN 8
#define JTP_TILE_HDOOR_WOOD_CLOSED 9
#define JTP_TILE_DOOR_WOOD_BROKEN 10
/* Other cmap tiles */
#define JTP_TILE_STAIRS_UP 11
#define JTP_TILE_STAIRS_DOWN 12
#define JTP_TILE_FOUNTAIN 13
#define JTP_TILE_ALTAR 14
#define JTP_TILE_TRAP_TELEPORTER 15
#define JTP_TILE_TREE 16
#define JTP_TILE_TRAP_PIT 17
#define JTP_TILE_CLOUD 18
#define JTP_TILE_GRAVE 19
#define JTP_TILE_SINK 20
#define JTP_TILE_TRAP_BEAR 21
#define JTP_TILE_TRAP_MAGIC 22
#define JTP_TILE_TRAP_WATER 23
#define JTP_TILE_TRAP_DOOR 24
#define JTP_TILE_FLOOR_NOT_VISIBLE 25
#define JTP_TILE_TRAP_ANTI_MAGIC 99
#define JTP_TILE_TRAP_ARROW 100
#define JTP_TILE_TRAP_SLEEPGAS 101
#define JTP_TILE_TRAP_FALLING_ROCK 102
#define JTP_TILE_TRAP_FIRE 103
#define JTP_TILE_ZAP_HORIZONTAL 104
#define JTP_TILE_ZAP_VERTICAL 105
#define JTP_TILE_ZAP_SLANT_LEFT 106
#define JTP_TILE_ZAP_SLANT_RIGHT 107
#define JTP_TILE_LADDER_DOWN 108
#define JTP_TILE_LADDER_UP 109
#define JTP_TILE_EXPLOSION_NORTHWEST 110
#define JTP_TILE_EXPLOSION_NORTH 111
#define JTP_TILE_EXPLOSION_NORTHEAST 112
#define JTP_TILE_EXPLOSION_WEST 113
#define JTP_TILE_EXPLOSION_CENTER 114
#define JTP_TILE_EXPLOSION_EAST 115
#define JTP_TILE_EXPLOSION_SOUTHWEST 116
#define JTP_TILE_EXPLOSION_SOUTH 117
#define JTP_TILE_EXPLOSION_SOUTHEAST 118
#define JTP_TILE_THRONE 119
#define JTP_TILE_BARS 120

/* Tile graphic indices, object tiles */
#define JTP_TILE_BAG 26
#define JTP_TILE_BOULDER 27
#define JTP_TILE_BONES 28
#define JTP_TILE_STATUE 29
#define JTP_TILE_CHEST 30
#define JTP_TILE_COINS 31
#define JTP_TILE_BOOK 32
#define JTP_TILE_HELMET 33
#define JTP_TILE_SHIELD 34
#define JTP_TILE_BOOTS 35
#define JTP_TILE_SPEAR 36
#define JTP_TILE_BOTTLE 37
#define JTP_TILE_SCROLL 38
#define JTP_TILE_WAND 39
#define JTP_TILE_SWORD 40
#define JTP_TILE_GEM_BLUE 41
#define JTP_TILE_GEM_GREEN 122
#define JTP_TILE_GEM_RED 123
#define JTP_TILE_GEM_YELLOW 124
#define JTP_TILE_GEM_WHITE 125
#define JTP_TILE_GEM_BLACK 126
#define JTP_TILE_HAMMER 75
#define JTP_TILE_AXE 76
#define JTP_TILE_LANTERN 77
#define JTP_TILE_RING 78
#define JTP_TILE_AMULET 79
#define JTP_TILE_LEATHER_ARMOR 80
#define JTP_TILE_PLATE_MAIL 81
#define JTP_TILE_SCALE_MAIL 82
#define JTP_TILE_RING_MAIL 83
#define JTP_TILE_CHAIN_MAIL 84
#define JTP_TILE_CLOAK 86
#define JTP_TILE_TRIDENT 87
#define JTP_TILE_CAMERA 88
#define JTP_TILE_FEDORA 89
#define JTP_TILE_CLUB 90
#define JTP_TILE_ARROW 91
#define JTP_TILE_PEAR 92
#define JTP_TILE_APPLE 93
#define JTP_TILE_DAGGER 94
#define JTP_TILE_KEY 95
#define JTP_TILE_BOW 96
#define JTP_TILE_WHIP 127
#define JTP_TILE_CANDLE 128
#define JTP_TILE_EGG 129
#define JTP_TILE_GLOVES 130
#define JTP_TILE_BELL 131
#define JTP_TILE_MACE 132
#define JTP_TILE_GRAND_HORN 134
#define JTP_TILE_CRYSTAL_BALL 135
#define JTP_TILE_HORN 136
#define JTP_TILE_UNICORN_HORN 137
#define JTP_TILE_HAWAIIAN_SHIRT 138
#define JTP_TILE_CREDIT_CARD 139
#define JTP_TILE_MIRROR 140
#define JTP_TILE_CROSSBOW 141
#define JTP_TILE_CONICAL_HAT 142
#define JTP_TILE_MAGIC_MARKER 143
#define JTP_TILE_STAFF 144
#define JTP_TILE_FOOD_RATION 145
#define JTP_TILE_COOKIE 146
#define JTP_TILE_TRIPE_RATION 147
#define JTP_TILE_MEAT_CHUNK 148
#define JTP_TILE_SHURIKEN 149
#define JTP_TILE_ROCK 150
#define JTP_TILE_PICKAXE 151
#define JTP_TILE_TIN 152

/* Tile graphic indices, monster tiles */
#define JTP_TILE_KNIGHT 42
#define JTP_TILE_DOG 43
#define JTP_TILE_JELLY 44
#define JTP_TILE_LIZARD 45
#define JTP_TILE_SPIDER 46
#define JTP_TILE_GOBLIN 47
#define JTP_TILE_EYE 48
#define JTP_TILE_GNOME 49
#define JTP_TILE_ELEMENTAL 50
#define JTP_TILE_OGRE 51
#define JTP_TILE_NAGA 52
#define JTP_TILE_CAT 53
#define JTP_TILE_WIZARD 54
#define JTP_TILE_VALKYRIE 55
#define JTP_TILE_RANGER 56
#define JTP_TILE_NYMPH 57
#define JTP_TILE_WATER_NYMPH 58
#define JTP_TILE_SKELETON 59
#define JTP_TILE_GHOST 60
#define JTP_TILE_WRAITH 61
#define JTP_TILE_GREEN_DRAGON 62
#define JTP_TILE_ZOMBIE 63
#define JTP_TILE_STONE_GOLEM 64
#define JTP_TILE_RAT 65
#define JTP_TILE_TROLL 66
#define JTP_TILE_BAT 67
#define JTP_TILE_HORSE 68
#define JTP_TILE_ANT 69
#define JTP_TILE_ARCHEOLOGIST 70
#define JTP_TILE_BEE 71
#define JTP_TILE_TOURIST 85
#define JTP_TILE_ROGUE 97
#define JTP_TILE_PRIEST 98

#define JTP_MAX_TILES 153

/* Map symbols in the 'view map' display */
#define JTP_MAP_SYMBOL_WALL 0
#define JTP_MAP_SYMBOL_FLOOR 1
#define JTP_MAP_SYMBOL_UP 2
#define JTP_MAP_SYMBOL_DOWN 3
#define JTP_MAP_SYMBOL_DOOR 4
#define JTP_MAP_SYMBOL_CMAP 5
#define JTP_MAP_SYMBOL_TRAP 6
#define JTP_MAP_SYMBOL_OBJECT 7
#define JTP_MAP_SYMBOL_MONSTER 8

#define JTP_MAX_MAP_SYMBOLS 9
#define JTP_MAX_SPELL_SYMBOLS 10

/*
 * Subdirectories used by Vulture's Eye. 
 * These should be under the main directory.
 */
#ifdef JTP_USE_UNIX_STYLE_DIRECTORY_SEPARATOR
  #define JTP_CONFIG_DIRECTORY "config/"
  #define JTP_GRAPHICS_DIRECTORY "graphics/"
  #define JTP_SOUND_DIRECTORY "sound/"
#endif
#ifndef JTP_USE_UNIX_STYLE_DIRECTORY_SEPARATOR
  #define JTP_CONFIG_DIRECTORY "config\\"
  #define JTP_GRAPHICS_DIRECTORY "graphics\\"
  #define JTP_SOUND_DIRECTORY "sound\\"
#endif

/* 
 * External files used by the GUI (indices into filename table).
 * Note: The intro sequence may use other
 * external images, listed in the script file.
 * For DOS compatibility, use short filenames.
 */
#define JTP_FILE_INTRO_SCRIPT 0
#define JTP_FILE_OPTIONS 1
#define JTP_FILE_SOUNDS_CONFIG 2
#define JTP_FILE_KEYS_CONFIG 3
#define JTP_FILE_SHADING_TABLE 4

#define JTP_FILE_CMAP_TILES 5
#define JTP_FILE_CMAP_TILES_2 63
#define JTP_FILE_CMAP_WALL_TILES 6
#define JTP_FILE_CMAP_WALL_TILES_2 7
#define JTP_FILE_CMAP_TRANSPARENT_WALL_TILES 55
#define JTP_FILE_CMAP_TRANSPARENT_WALL_TILES_2 56
#define JTP_FILE_CMAP_HALF_HEIGHT_WALL_TILES 58
#define JTP_FILE_CMAP_HALF_HEIGHT_WALL_TILES_2 59
#define JTP_FILE_CMAP_FLOOR_TILES 8
#define JTP_FILE_CMAP_FLOOR_TILES_2 9
#define JTP_FILE_OBJ_TILES_1 10
#define JTP_FILE_OBJ_TILES_2 62
#define JTP_FILE_MON_TILES_1 11
#define JTP_FILE_MON_TILES_2 54

#define JTP_FILE_MOUSE_CURSORS 12
#define JTP_FILE_NETHACK_LOGO 13
#define JTP_FILE_CHARACTER_GENERATION 14
#define JTP_FILE_FONT_SMALL 15
#define JTP_FILE_FONT_LARGE 16
#define JTP_FILE_WINDOW_STYLE 17
#define JTP_FILE_STATUS_BAR 18
#define JTP_FILE_MAP_PARCHMENT 19
#define JTP_FILE_MAP_SYMBOLS 20
#define JTP_FILE_BACKPACK 21
#define JTP_FILE_ENDING_DIED 22
#define JTP_FILE_ENDING_ASCENDED 23
#define JTP_FILE_ENDING_QUIT 24
#define JTP_FILE_SPELLBOOK 60
#define JTP_FILE_SPELL_SYMBOLS 61

#define JTP_MAX_GAME_FILES 64
#define JTP_MAX_FILENAME_LENGTH 512

typedef struct{
  char * str;
  int    action_id;
} jtp_dropdown_action;

/* Shortcut actions */
#define JTP_ACTION_CHAT 1
#define JTP_ACTION_CLOSE_DOOR 2
#define JTP_ACTION_ENGRAVE 3
#define JTP_ACTION_FORCE_LOCK 4
#define JTP_ACTION_GO_DOWN 5
#define JTP_ACTION_GO_UP 6
#define JTP_ACTION_KICK 7
#define JTP_ACTION_LOOT 8
#define JTP_ACTION_MONSTER_ABILITY 9
#define JTP_ACTION_MOVE_HERE 10
#define JTP_ACTION_OPEN_DOOR 11
#define JTP_ACTION_PAY_BILL 12
#define JTP_ACTION_PICK_UP 13
#define JTP_ACTION_PRAY 14
#define JTP_ACTION_PUSH_BOULDER 15
#define JTP_ACTION_REST 16
#define JTP_ACTION_RIDE 17
#define JTP_ACTION_SEARCH 18
#define JTP_ACTION_SIT 19
#define JTP_ACTION_TURN_UNDEAD 20
#define JTP_ACTION_UNTRAP 21
#define JTP_ACTION_WIPE_FACE 22
#define JTP_ACTION_DRINK 23
#define JTP_ACTION_LOOK_AROUND 24
#define JTP_ACTION_WHATS_THIS 25
#define JTP_ACTION_APPLY_ITEM 26
#define JTP_ACTION_DRINK_ITEM 27
#define JTP_ACTION_DROP_ITEM 28
#define JTP_ACTION_EAT_ITEM 29
#define JTP_ACTION_READ_ITEM 30
#define JTP_ACTION_REMOVE_ITEM 31
#define JTP_ACTION_WEAR_ITEM 32
#define JTP_ACTION_PUT_ON_ITEM 33
#define JTP_ACTION_WIELD_ITEM 34
#define JTP_ACTION_ZAP_ITEM 35
#define JTP_ACTION_ENTER_TRAP 36
#define JTP_ACTION_CAST_SPELL 37

/* Predefined background music names */
#define JTP_SONG_TITLE 0
#define JTP_SONG_INTRO 1
/*
define JTP_SONG_WATER_CAVES 27
define JTP_SONG_AIR_CAVES 28
define JTP_SONG_EARTH_CAVES 29
define JTP_SONG_FIRE_CAVES 30
define JTP_SONG_GNOMISH_MINES 31
define JTP_SONG_MINETOWN 32
define JTP_SONG_ORACLE 33
define JTP_SONG_LAMENT_1 34
define JTP_SONG_LAMENT_2 57
define JTP_SONG_AMBIENT_1 35
define JTP_SONG_AMBIENT_2 36
define JTP_SONG_BATTLE_1 37
define JTP_SONG_BATTLE_2 38
define JTP_SONG_SHOPPING 39
*/
#define JTP_SONG_ENDING_DIED 2
#define JTP_SONG_ENDING_ASCENDED 3
#define JTP_SONG_ENDING_QUIT 4
#define JTP_SONG_BACKGROUND 5

/* Predefined sound effect names */
#define JTP_SOUND_WALK 0
/*
define JTP_SOUND_CLOSE_DOOR 44
define JTP_SOUND_OPEN_DOOR 45
define JTP_SOUND_SWORD_HIT 46
define JTP_SOUND_SWORD_MISS 47
define JTP_SOUND_LEVEL_UP 48
define JTP_SOUND_CAT_MEOW 49
define JTP_SOUND_DOG_BARK 50
define JTP_SOUND_HORSE_WHINNY 51
define JTP_SOUND_FOUNTAIN 52
define JTP_SOUND_COUNTING 53
*/

/*---------------------------------------------------------
Local variables 
----------------------------------------------------------*/

/* Game directory (game files are expected to be under this) */
char jtp_game_path[JTP_MAX_FILENAME_LENGTH];

/* Filenames used by Vulture's Eye. Set during initialization */
char * jtp_filenames[JTP_MAX_GAME_FILES];


/* Interface options */
int jtp_recenter_after_movement = 0;
int jtp_play_music = 0;
int jtp_play_effects = 0;
int jtp_one_command_per_click = 0;
int jtp_fullscreen = 0;
/* Minimum scrolling delay (min time between scrolls, in seconds) */
double jtp_min_scroll_delay = 0.05;
double jtp_min_command_delay = 0.05;
char * jtp_external_midi_player_command = NULL;
char * jtp_external_mp3_player_command = NULL;

/* Event sounds */
jtp_event_sound ** jtp_event_sounds = NULL;
int jtp_n_event_sounds = 0;
int jtp_n_background_songs = 0;

/* Time of last scrolling event. Used to prevent overfast scrolling. */
clock_t jtp_last_scroll_time = 0;

/* Window list */
jtp_list * jtp_windowlist = NULL; /* Linked list of all windows */
int jtp_max_window_id = -1;     /* Largest window id given */

/* Autopilot */
int jtp_movebuffer[JTP_MAX_MOVE_LENGTH]; /* Target squares for 'autopilot' */
int jtp_move_length = 0;       /* Length of autopilot sequence */
int jtp_autopilot_type;    /* Autopilot type (movement or whatis) */

/* Query response for shortcut-activated commands */
int jtp_is_shortcut_active = 0;
int jtp_shortcut_query_response;

/* Is the spellbook being viewed? */
int jtp_is_spellbook_being_viewed = 0;

/* Shortcut actions selected from the backpack or spellbook screen */
int jtp_is_backpack_shortcut_active = 0;
int jtp_backpack_shortcut_action;

/* Map window contents, as NetHack glyphs */
int ** jtp_mapglyph_cmap = NULL;  /* Topmost cmap glyphs */
int ** jtp_mapglyph_obj = NULL;   /* Topmost object glyphs */
int ** jtp_mapglyph_mon = NULL;   /* Topmost monster glyphs */

/* Translation tables: NetHack glyph to Vulture's Eye tile */
int * jtp_montiles = NULL;        /* Monster glyph to monster tile */
int * jtp_objtiles = NULL;        /* Object glyph to object tile */
int * jtp_traptiles = NULL;       /* Trap glyph to trap tile */
int * jtp_cmaptiles = NULL;       /* Cmap glyph to cmap tile (walls and so on) */

/* Map window contents, as Vulture's Eye tiles */
jtp_tilestats *** jtp_maptile_cmap = NULL; /* Cmap tiles (incl. floor, excluding walls) */
jtp_tilestats *** jtp_maptile_obj = NULL;  /* Object tiles */
jtp_tilestats *** jtp_maptile_mon = NULL;  /* Monster tiles */
jtp_wall_style ** jtp_maptile_wall = NULL; /* Custom (combination) wall style for each square */
jtp_floor_edge_style ** jtp_maptile_floor_edge = NULL; /* Custom floor edge style for each square */

int ** jtp_room_indices = NULL;            /* Room indices */
int ** jtp_map_light = NULL;               /* Light levels */
double ** jtp_map_light_distances = NULL;  /* Temp array used to calculate light levels */
double ** jtp_map_temp_distances = NULL;   /* Temp array used to calculate light levels */
char ** jtp_map_accessibles = NULL;        /* Temp array used to calculate light levels */

/* General map info */
int jtp_map_width, jtp_map_height; /* Map dimensions */
int jtp_map_x, jtp_map_y;  /* Center of displayed map area */
int jtp_map_tgtx, jtp_map_tgty; /* Selected square on map */
int jtp_you_x, jtp_you_y;  /* Location of player character on the map */
int jtp_old_you_x, jtp_old_you_y;  /* Previous location of player character on the map */

int jtp_map_changed;       /* Has the map changed since the last glyph->tile conversion? */
int jtp_game_palette_set;  /* Has the in-game palette been set already? */
int jtp_cur_dlevel;        /* Current dungeon level (used for initialization) */
int jtp_prev_dlevel;       /* Previous dungeon level */
int jtp_tile_conversion_initialized = 0; /* Have the conversion tables been set up? */

/* Light sources */
int jtp_nlights;           /* Number of light sources in use */
int jtp_ambient;           /* Ambient light level in the map */
int jtp_lights[JTP_MAX_LIGHTS*3]; /* Light source parameters (x,y,radius) */

/* Floor decorations */
int jtp_n_floor_decors = 0;    /* Number of floor decorations in use */
jtp_floor_decor * jtp_floor_decors = NULL;  /* Floor decorations table */

/* Status bar location */
int jtp_statusbar_x, jtp_statusbar_y;

/*
  Center coordinates of centermost displayed map tile.
  Used for quantizing target crosshairs location.
*/
int jtp_map_center_x, jtp_map_center_y;

/* Message window */
int jtp_messages_height;   /* height of messages shown on-screen */
int jtp_first_shown_message = 0; /* Index of newest shown message (>= 0), used by the "previous message" command */
unsigned char * jtp_messages_background = NULL; /* Messages overlap this part of map window */
char jtp_message_colors[JTP_MAX_MESSAGE_COLORS]; /* Message age shading */

/* Bitmap graphics */
jtp_window_graphics jtp_defwin; /* Basic window graphics */
jtp_tilestats ** jtp_tiles = NULL;     /* Isometric map tile set */
jtp_wall_style * jtp_walls = NULL;     /* Wall tiles for different wall styles (rough/brick/etc.) */
jtp_floor_style * jtp_floors = NULL;   /* Floor tiles for different floor styles (rough/stone/etc.) */
jtp_floor_edge_style * jtp_floor_edges = NULL; /* Floor edge tiles for different floor styles */
unsigned char * jtp_statusbar = NULL; /* Status bar graphic */

unsigned char * jtp_map_parchment_center = NULL; /* Map parchment graphic, center area */
unsigned char * jtp_map_parchment_top = NULL;    /* Map parchment graphic, top border */
unsigned char * jtp_map_parchment_bottom = NULL; /* Map parchment graphic, bottom border */
unsigned char * jtp_map_parchment_left = NULL;   /* Map parchment graphic, left border */
unsigned char * jtp_map_parchment_right = NULL;  /* Map parchment graphic, right border */
unsigned char * jtp_map_symbols[JTP_MAX_MAP_SYMBOLS]; /* Map parchment symbols */

unsigned char * jtp_backpack_center = NULL; /* Backpack graphic, center area */
unsigned char * jtp_backpack_top = NULL;    /* Backpack graphic, top border */
unsigned char * jtp_backpack_bottom = NULL; /* Backpack graphic, bottom border */
unsigned char * jtp_backpack_left = NULL;   /* Backpack graphic, left border */
unsigned char * jtp_backpack_right = NULL;  /* Backpack graphic, right border */

unsigned char * jtp_spellbook_center = NULL; /* Spellbook graphic, center area */
unsigned char * jtp_spellbook_top = NULL;    /* Spellbook graphic, top border */
unsigned char * jtp_spellbook_bottom = NULL; /* Spellbook graphic, bottom border */
unsigned char * jtp_spellbook_left = NULL;   /* Spellbook graphic, left border */
unsigned char * jtp_spellbook_right = NULL;  /* Spellbook graphic, right border */
unsigned char * jtp_spell_symbols[JTP_MAX_SPELL_SYMBOLS]; /* Spell symbols */

/* Lighting table (color conversion) */
unsigned char * jtp_shade = NULL;     /* Light shading table */


/*----------------------------------------------------
  Function implementations 
-----------------------------------------------------*/

jtp_window *
jtp_find_window(window)
winid window;
{
  jtp_window * tempwindow;

  jtp_list_reset(jtp_windowlist);
  tempwindow = (jtp_window *)jtp_list_current(jtp_windowlist);
  while ((tempwindow) && (tempwindow->id != window))
  {
    jtp_list_advance(jtp_windowlist);
    tempwindow = (jtp_window *)jtp_list_current(jtp_windowlist);
  }
  return(tempwindow);
}


void
jtp_map_to_screen(map_x, map_y, screen_x, screen_y)
int map_x, map_y;
int *screen_x, *screen_y;
{
  map_x -= jtp_map_x;
  map_y -= jtp_map_y;
  *screen_x = jtp_map_center_x + JTP_MAP_XMOD*(map_x - map_y);
  *screen_y = jtp_map_center_y + JTP_MAP_YMOD*(map_x + map_y);
}


int
jtp_find_menu_accelerator(char * description, char * used_accelerators)
{
  char acc_found;
  int cur_accelerator;
  int j, k;

  /* Find an unused accelerator */

  /* Try a letter from the description */
  acc_found = 0;
  for (k = 0; k < strlen(description); k++)
  {
    cur_accelerator = tolower(description[k]);
    acc_found = 1;
    for (j = 0; j < strlen(used_accelerators); j++)
      if (used_accelerators[j] == cur_accelerator) acc_found = 0;
    if (!acc_found)
    {
      cur_accelerator = toupper(description[k]);
      acc_found = 1;
      for (j = 0; j < strlen(used_accelerators); j++)
        if (used_accelerators[j] == cur_accelerator) acc_found = 0;
    }
    if (acc_found) break;
  }
  if (!acc_found)
  {
    /* Pick any available lowercase letter in the alphabet */
    for (cur_accelerator = 'a'; cur_accelerator <= 'z'; cur_accelerator++)
    {
      acc_found = 1;
      for (j = 0; j < strlen(used_accelerators); j++)
        if (used_accelerators[j] == cur_accelerator) acc_found = 0;
      if (acc_found) break;
    }
    /* Pick any available uppercase letter in the alphabet */
    if (!acc_found)
      for (cur_accelerator = 'A'; cur_accelerator <= 'Z'; cur_accelerator++)
      {
        acc_found = 1;
        for (j = 0; j < strlen(used_accelerators); j++)
          if (used_accelerators[j] == cur_accelerator) acc_found = 0;
        if (acc_found) break;
      }
  }

  if (acc_found)
  {
    /* Add found accelerator to string of used ones (assume there's enough room) */
    j = strlen(used_accelerators);
    used_accelerators[j] = cur_accelerator;
    used_accelerators[j+1] = '\0';
    return(cur_accelerator);
  }
  else return(-1);
}


void
jtp_show_ending(tempwindow)
jtp_window * tempwindow;
{
  int i, j, k, l, empty_line, totallines;
  char ** templines;
  jtp_menuitem * tempmenuitem;
  char tempbuffer[1024];

  /* 
   * Assume that the screen has been faded out by now.
   * Load the appropriate 'ending' image.
   */
  jtp_clear_screen();
  jtp_game_palette_set = 0;
  if (tempwindow->ending_type == QUIT)
  {
    jtp_load_PCX((jtp_screen.width-JTP_ENDING_SLIDE_WIDTH)/2, 0,
                 jtp_filenames[JTP_FILE_ENDING_QUIT], 1);
    jtp_play_event_sound("nhfe_music_end_quit");
    /* jtp_play_midi_song(jtp_filenames[JTP_SONG_ENDING_QUIT]); */
  }
  else if (tempwindow->ending_type == ASCENDED)
  {
    jtp_load_PCX((jtp_screen.width-JTP_ENDING_SLIDE_WIDTH)/2, 0,
                 jtp_filenames[JTP_FILE_ENDING_ASCENDED], 1);
    jtp_play_event_sound("nhfe_music_end_ascended");
    /* jtp_play_midi_song(jtp_filenames[JTP_SONG_ENDING_ASCENDED]); */
  }
  else if (tempwindow->ending_type < GENOCIDED)
  {
    jtp_load_PCX((jtp_screen.width-JTP_ENDING_SLIDE_WIDTH)/2, 0,
                 jtp_filenames[JTP_FILE_ENDING_DIED], 1);
    jtp_play_event_sound("nhfe_music_end_died");
    /* jtp_play_midi_song(jtp_filenames[JTP_SONG_ENDING_DIED]); */
  }
  else
    jtp_clear_screen();  /* No image associated with this ending */
   
  jtp_refresh();
  jtp_fade_in(0.5);
  
  /* Count n. of rows to display */
  totallines = 0;
  if (tempwindow->menu)
  {
    jtp_list_reset(tempwindow->menu->items);
    while (jtp_list_current(tempwindow->menu->items))
    {
      totallines++;
      jtp_list_advance(tempwindow->menu->items);
    }
  }  
  /* Add prompt line */
  totallines++;

  /* Display the rows */
  if (totallines > 0)
  {
    k = jtp_screen.height - (totallines+1)*jtp_fonts[JTP_FONT_INTRO].lineheight;
    jtp_list_reset(tempwindow->menu->items);
    for (i = 0; i < totallines; i++)
    { 
      if (i == totallines-1) 
        strcpy(tempbuffer, "(press any key)");
      else 
      {
        tempmenuitem = (jtp_menuitem *)jtp_list_current(tempwindow->menu->items);
        strcpy(tempbuffer, tempmenuitem->text);
      }  
      j = (jtp_screen.width - jtp_text_length(tempbuffer, JTP_FONT_INTRO))/2;
      jtp_put_text(j, 
                   k + i*jtp_fonts[JTP_FONT_INTRO].lineheight + jtp_fonts[JTP_FONT_INTRO].baseline + 1,
                   JTP_FONT_INTRO, 0,
                   tempbuffer,
                   jtp_screen.vpage);
      jtp_put_text(j, 
                   k + i*jtp_fonts[JTP_FONT_INTRO].lineheight + jtp_fonts[JTP_FONT_INTRO].baseline,
                   JTP_FONT_INTRO, 255,
                   tempbuffer,
                   jtp_screen.vpage);
      jtp_list_advance(tempwindow->menu->items);             
    }               
  }
  
  jtp_refresh();
  jtp_getch();
  jtp_fade_out(0.5);  

  /* Restore the regular game palette */  
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_MOUSE_CURSORS], 1);
  jtp_clear_screen();
  jtp_refresh();
  jtp_updatepal(0, 255);    
  jtp_game_palette_set = 1;
}


int 
jtp_is_onscreen(x, y)
int x, y;
{
  int i, j;
  jtp_map_to_screen(x, y, &i, &j);

  if ((i >= 50) && (i < jtp_screen.width-50) &&
      (j >= 50) && (j < jtp_screen.height-JTP_STATUSBAR_HEIGHT-50))
    return(1);
  return(0);  
}

/*
 * jtp_play_ambient_sound
 *
 * Play an ambient sound effect or background music.
 */
void jtp_play_ambient_sound(int force_play)
{
  int k;
  char tempbuffer[256];

  if ((!force_play) && (jtp_is_music_playing())) return;
  if (force_play) jtp_stop_music();

  k = (rand() >> 4)%jtp_n_background_songs;
  sprintf(tempbuffer, "nhfe_music_background%03d", k);
  jtp_play_event_sound(tempbuffer);
  /*
  switch(k)
  {
    case 0: jtp_play_midi_song(jtp_filenames[JTP_SONG_AIR_CAVES]); break;
    case 1: jtp_play_midi_song(jtp_filenames[JTP_SONG_WATER_CAVES]); break;
    case 2: jtp_play_midi_song(jtp_filenames[JTP_SONG_FIRE_CAVES]); break;
    case 3: jtp_play_midi_song(jtp_filenames[JTP_SONG_EARTH_CAVES]); break;
    case 4: jtp_play_midi_song(jtp_filenames[JTP_SONG_GNOMISH_MINES]); break;
    case 5: jtp_play_midi_song(jtp_filenames[JTP_SONG_MINETOWN]); break;
    case 6: jtp_play_midi_song(jtp_filenames[JTP_SONG_ORACLE]); break;
    case 7: jtp_play_midi_song(jtp_filenames[JTP_SONG_LAMENT_1]); break;
    case 8: jtp_play_midi_song(jtp_filenames[JTP_SONG_LAMENT_2]); break;
    case 9: jtp_play_midi_song(jtp_filenames[JTP_SONG_AMBIENT_1]); break;
    case 10: jtp_play_midi_song(jtp_filenames[JTP_SONG_AMBIENT_2]); break;
    case 11: jtp_play_midi_song(jtp_filenames[JTP_SONG_BATTLE_1]); break;
    case 12: jtp_play_midi_song(jtp_filenames[JTP_SONG_BATTLE_2]); break;
    default: break;
  }
  */
}

/*
 * jtp_play_event_sound
 *
 * Play a sound effect or song associated with a NetHack event.
 */
void jtp_play_event_sound(const char * str)
{
  int i;

  for (i = 0; i < jtp_n_event_sounds; i++)
    if (strstr(str, (jtp_event_sounds[i])->searchpattern))
    {
      if ((jtp_event_sounds[i])->soundtype == JTP_EVENT_SOUND_TYPE_WAVE)      
        jtp_play_wave_sound((jtp_event_sounds[i])->filename, 44100, 16, 1);
      else if ((jtp_event_sounds[i])->soundtype == JTP_EVENT_SOUND_TYPE_LONG_WAVE)
        jtp_play_wave_sound((jtp_event_sounds[i])->filename, 22050, 8, 1);
      else if ((jtp_event_sounds[i])->soundtype == JTP_EVENT_SOUND_TYPE_MIDI)
        jtp_play_midi_song((jtp_event_sounds[i])->filename);
      else if ((jtp_event_sounds[i])->soundtype == JTP_EVENT_SOUND_TYPE_CD_AUDIO)
        jtp_play_cd_track((jtp_event_sounds[i])->filename);
      else if ((jtp_event_sounds[i])->soundtype == JTP_EVENT_SOUND_TYPE_MP3)
        jtp_play_mp3_song((jtp_event_sounds[i])->filename);
      else if ((jtp_event_sounds[i])->soundtype == JTP_EVENT_SOUND_TYPE_RANDOM_SONG)
        jtp_play_ambient_sound(1);

      i = jtp_n_event_sounds;
    }
}

/*
 * jtp_play_command_sound
 *
 * Play a sound effect associated with a NetHack command.
 */
void jtp_play_command_sound(int cur_cmd)
{
  switch(cur_cmd)
  {
    case JTP_NHCMD_NORTH: case JTP_NHCMD_SOUTH: 
    case JTP_NHCMD_WEST: case JTP_NHCMD_EAST:
    case JTP_NHCMD_NORTHWEST: case JTP_NHCMD_NORTHEAST:
    case JTP_NHCMD_SOUTHWEST: case JTP_NHCMD_SOUTHEAST: 
      jtp_play_event_sound("nhfe_sound_walk"); 
      /* jtp_play_wave_sound(jtp_filenames[JTP_SOUND_WALK], 44100, 16, 1); */
      break;
    default: break;
  }
}


/* 
 * jtp_find_path
 *
 * Find shortest path from map square [row1,col1] to square [row2,col2] 
 * with Dijkstra's algorithm. Store path in jtp_movebuffer.
 */
void 
jtp_find_path(row1, col1, row2, col2)
int row1, col1; /* Start map square */
int row2, col2; /* End map square */
{
  double **pathlengths;
  double cur_biased_length, delta_length;
  int **knowns;
  int **isdoors;
  int ***paths;
  int cur_row, cur_col, cur_tile, cur_glyph, cur_length;
  int all_known;
  int i, j;
  int y1, y2, x1, x2;
  char tempbuffer[1024];

  /*jtp_write_log_message("[jtp_win.c/jtp_find_path/Debug1]\n");*/

  /* If the start and end squares are the same, don't bother */
  if ((row1 == row2) && (col1 == col2)) return;

  /* Don't try to move off the map */
  if ((row1 < 0) || (row1 >= JTP_MAP_HEIGHT) || (col1 < 1) || (col1 >= JTP_MAP_WIDTH))
  {
    /* sprintf(tempbuffer, "Start square x=%d y=%d is invalid\0", col1, row1);
    jtp_messagebox(tempbuffer); */
    return;
  }
  if ((row2 < 0) || (row2 >= JTP_MAP_HEIGHT) || (col2 < 1) || (col2 >= JTP_MAP_WIDTH))
  {
    /* sprintf(tempbuffer, "End square x=%d y=%d is invalid\0", col2, row2);
    jtp_messagebox(tempbuffer); */
    return;
  }
   
  pathlengths = (double **)malloc(JTP_MAP_HEIGHT*sizeof(double *));
  knowns = (int **)malloc(JTP_MAP_HEIGHT*sizeof(int *));
  isdoors = (int **)malloc(JTP_MAP_HEIGHT*sizeof(int *));
  paths = (int ***)malloc(JTP_MAP_HEIGHT*sizeof(int **));
  if ((!pathlengths) || (!knowns) || (!isdoors) || (!paths))
  {
    jtp_write_log_message("[jtp_win.c/jtp_find_path/Check1] Out of memory!\n");
    jtp_exit_graphics(); exit(1);
  }
  
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
  {
    pathlengths[i] = (double *)malloc(JTP_MAP_WIDTH*sizeof(double));
    knowns[i] = (int *)malloc(JTP_MAP_WIDTH*sizeof(int));
    isdoors[i] = (int *)malloc(JTP_MAP_WIDTH*sizeof(int));
    paths[i] = (int **)calloc(JTP_MAP_WIDTH, sizeof(int *));
    if ((!pathlengths[i]) || (!knowns[i]) || (!isdoors[i]) || (!paths[i]))
    {
      jtp_write_log_message("[jtp_win.c/jtp_find_path/Check2] Out of memory!\n");
      jtp_exit_graphics(); exit(1);
    }
        
    for (j = 1; j < JTP_MAP_WIDTH; j++)
    {
      cur_glyph = jtp_mapglyph_cmap[i][j];
      cur_tile = jtp_cmap_to_tile(cur_glyph);
         
      /* 
       * If the square is inaccessible (wall, closed door, unknown) we don't go through it.
       * Also, we don't go into boulder spaces or traps.
       */
      if ((cur_tile == JTP_TILE_INVALID) || 
          (cur_tile == JTP_TILE_WALL_GENERIC) ||
          (cur_glyph == S_vcdoor) || 
          (cur_glyph == S_hcdoor))
        knowns[i][j] = 2;	/* Vertex is a 'no go' */
      else if ((jtp_mapglyph_obj[i][j] == BOULDER) ||
               (glyph_is_trap(jtp_mapglyph_cmap[i][j] + GLYPH_CMAP_OFF)))
        knowns[i][j] = 2;	/* Vertex contains trap or boulder */  
      else
        knowns[i][j] = 0;	/* Vertex is eligible for a path */

      /* 
       * If the vertex has an open door, we need to remember it, 
       * since doors can't be entered/exited diagonally.
       */
      if ((cur_glyph == S_vodoor) || (cur_glyph == S_hodoor))
        isdoors[i][j] = 1;	/* Vertex is an open door */
      else 
        isdoors[i][j] = 0;	/* Vertex is not a door */
      
      /*
       * Initially, we set all pathlengths to 'unknown', eg. a big value.
       * Later, we lower them if a path is found.
       */
      pathlengths[i][j] = JTP_UNKNOWN_PATH_LENGTH; /* All true paths are shorter */
    }
  }
  /* Length from the starting point to itself is trivially zero. */
  pathlengths[row1][col1] = 0;
  /* Starting square is valid for a path (even e.g. when in a trap) */
  knowns[row1][col1] = 0;

  /*jtp_write_log_message("[jtp_win.c/jtp_find_path/Debug2]\n");*/

  all_known = 0;
  while (!all_known)
  {
    /* Find unknown vertex (map square) with lowest cost */
    cur_row = -1; cur_col = -1; cur_length = JTP_UNKNOWN_PATH_LENGTH;
    for (i = 0; i < JTP_MAP_HEIGHT; i++)
      for (j = 1; j < JTP_MAP_WIDTH; j++)
        if ((!knowns[i][j]) && (pathlengths[i][j] < cur_length))
        {        
          cur_row = i; cur_col = j; 
          cur_biased_length = pathlengths[i][j];
          cur_length = (int)cur_biased_length; /* Total bias must be < 1 */
        }
    
    if (cur_row < 0)
    {
      /* All accessible vertices searched. Exit. */
      all_known = 1;
    }
    else
    {    
      /* Update neighbouring vertices */
      if (cur_row > 0) y1 = cur_row-1; else y1 = 0;
      if (cur_row < JTP_MAP_HEIGHT-1) y2 = cur_row+1; else y2 = JTP_MAP_HEIGHT-1;
      if (cur_col > 1) x1 = cur_col-1; else x1 = 1;
      if (cur_col < JTP_MAP_WIDTH-1) x2 = cur_col+1; else x2 = JTP_MAP_WIDTH-1;

      for (i = y1; i <= y2; i++)
        for (j = x1; j <= x2; j++)
        {
          /* Don't remap center square */
          if ((i == cur_row) && (j == cur_col)) continue;

          /* Open door squares can't be entered/exited diagonally */
          if ((isdoors[cur_row][cur_col]) || (isdoors[i][j]))
            if ((i != cur_row) && (j != cur_col))
              continue;

          /* Search is slightly biased against diagonal movement */
          if ((abs(i-cur_row) == 1) && (abs(j-cur_col) == 1))
            delta_length = 1.00001;
          else delta_length = 1;
                      
          if (pathlengths[i][j] > cur_biased_length + delta_length)
          {
            pathlengths[i][j] = cur_length + delta_length;
  
            free(paths[i][j]);
            paths[i][j] = (int *)malloc((cur_length+1)*sizeof(int));
            if (!paths[i][j])
            {
              jtp_write_log_message("[jtp_win.c/jtp_find_path/Check3] Out ot memory!\n");
              jtp_exit_graphics(); exit(1);
            }
            if (cur_length > 0)
              memcpy(paths[i][j], paths[cur_row][cur_col], cur_length*sizeof(int));
            paths[i][j][cur_length] = cur_row*JTP_MAP_WIDTH + cur_col;
          }
        }
      knowns[cur_row][cur_col] = 1;    
    }
  }

  /*jtp_write_log_message("[jtp_win.c/jtp_find_path/Debug3]\n");*/

  if ((knowns[row2][col2] != 1) ||
      (pathlengths[row2][col2] >= JTP_MAP_WIDTH*JTP_MAP_HEIGHT))
  {
    /*
    The target square is not accessible. Choose a new,
    accessible target square as close as possible to the
    original.
    */
    cur_row = row2; cur_col = col2;
    cur_biased_length = JTP_MAP_WIDTH*JTP_MAP_WIDTH;
    for (i = 0; i < JTP_MAP_HEIGHT; i++)
      for (j = 1; j < JTP_MAP_WIDTH; j++)
      {
        if ((knowns[i][j] == 1) &&
            (pathlengths[i][j] < JTP_MAP_WIDTH*JTP_MAP_HEIGHT))
        {
          delta_length = (i-row2)*(i-row2)+(j-col2)*(j-col2);
          if (delta_length < cur_biased_length)
          {
            cur_row = i; cur_col = j;
            cur_biased_length = delta_length;
          }
        }
      }
    row2 = cur_row; col2 = cur_col;
  }

  /*jtp_write_log_message("[jtp_win.c/jtp_find_path/Debug4]\n");*/

  /* If the new end square is the same as the start square, don't bother */
  if ((row1 == row2) && (col1 == col2)) return;

  if ((knowns[row2][col2] == 1) &&
      (pathlengths[row2][col2] < JTP_MAP_WIDTH*JTP_MAP_HEIGHT))
  {
    /*
    The target square is accessible (a path exists).
    Store the path in the autopilot buffer.
    */
    jtp_move_length = pathlengths[row2][col2];
    jtp_autopilot_type = JTP_AUTOPILOT_MOVEMENT;
    for (i = 1; i < jtp_move_length; i++)
      jtp_movebuffer[i-1] = paths[row2][col2][i];
    jtp_movebuffer[jtp_move_length-1] = row2*JTP_MAP_WIDTH + col2;  
  }

  /*jtp_write_log_message("[jtp_win.c/jtp_find_path/Debug5]\n");*/

  /* jtp_messagebox("Clean up\n"); */

  /* Clean up */
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
  {
    free(pathlengths[i]);
    free(knowns[i]);
    free(isdoors[i]);
    for (j = 0; j < JTP_MAP_WIDTH; j++)
      free(paths[i][j]);
    free(paths[i]);
  }
  free(pathlengths);
  free(knowns);
  free(isdoors);
  free(paths);

  /*jtp_write_log_message("[jtp_win.c/jtp_find_path/Debug6]\n");*/
}

/*
 * jtp_find_passable_squares
 * 
 * Find squares that can be walked on. This is
 * an initial step to finding paths from one
 * square to another.
 */
void jtp_find_passable_squares
(
  char ** knowns       /* Preallocated storage array for 'known' flag */
)
{
  int i, j;
  int cur_glyph, cur_tile;

  for (i = JTP_MAP_HEIGHT-1; i >= 0; i--)
    for (j = JTP_MAP_WIDTH-1; j >= 1; j--)
    {
      cur_glyph = jtp_mapglyph_cmap[i][j];
      cur_tile = jtp_cmap_to_tile(cur_glyph);
         
      /* 
       * If the square is inaccessible (wall, closed door, unknown)
       * we don't go through it. Also, we don't go into boulder
       * spaces.
       */
      if ((cur_tile == JTP_TILE_INVALID) ||
          (cur_tile == JTP_TILE_WALL_GENERIC) ||
          (cur_glyph == S_vcdoor) ||
          (cur_glyph == S_hcdoor))
        knowns[i][j] = 2;	/* Vertex is a 'no go' */
      else if (jtp_mapglyph_obj[i][j] == BOULDER)
        knowns[i][j] = 2; /* Vertex contains a shadow-casting object */
      else
        knowns[i][j] = 0;	/* Vertex is eligible for a path */      
    } 
}

/* 
 * jtp_find_distances
 *
 * Find shortest distances from map square [row1,col1] to
 * other squares with Dijkstra's algorithm. 
 */
void jtp_find_distances
(
  int row1, int col1,  /* Start map square */
  double ** distances, /* Preallocated storage array for distances) */
  char ** knowns       /* Preallocated storage array for 'known' flag */
)
{
  double cur_biased_length, delta_length;
  int cur_row, cur_col;
  int cur_tile, cur_glyph;
  int all_known;
  int i, j;
  int x1, y1, x2, y2;
  int vertices_searched = 0;
  char tempbuffer[1024];

  /* Starting square must be valid */
  if ((row1 < 0) || (row1 >= JTP_MAP_HEIGHT) ||
      (col1 < 1) || (col1 >= JTP_MAP_WIDTH))
  {
    /*
    sprintf(tempbuffer, "Start square x=%d y=%d is invalid\0", col1, row1);
    jtp_messagebox(tempbuffer);
    */
    return;
  }

  /*
   * Reset 'knowns' and 'distances' arrays, in case this path
   * algorithm has been applied to them earlier.
   */
  for (i = JTP_MAP_HEIGHT-1; i >= 0; i--)
    for (j = JTP_MAP_WIDTH-1; j >= 1; j--)
    {
      if (knowns[i][j] == 1) knowns[i][j] = 0;
      /*
       * Initially, we set all distances to 'unknown', eg. a big value.
       * All true paths are shorter than this.
       * Later, we lower the distances if a path is found.
       */
      distances[i][j] = JTP_UNKNOWN_PATH_LENGTH;
    }

  /* Length from the starting point to itself is trivially zero. */
  distances[row1][col1] = 0;
  
  all_known = 0;
  while (!all_known)
  {
    /* Find unknown vertex (map square) with lowest cost */
    cur_row = -1; cur_col = -1; cur_biased_length = JTP_UNKNOWN_PATH_LENGTH;
    for (i = JTP_MAP_HEIGHT-1; i >= 0; i--)
      for (j = JTP_MAP_WIDTH-1; j >= 1; j--)
        if ((knowns[i][j] == 0) && (distances[i][j] < cur_biased_length))
        {
          cur_row = i; cur_col = j; 
          cur_biased_length = distances[i][j];
        }
    
    if (cur_row < 0)
    {
      /* All accessible vertices searched. Exit. */
      all_known = 1;
    }
    else
    {
      /* Update neighbouring vertices */
      if (cur_row > 0) y1 = cur_row-1; else y1 = 0;
      if (cur_row < JTP_MAP_HEIGHT-1) y2 = cur_row+1; else y2 = JTP_MAP_HEIGHT-1;
      if (cur_col > 1) x1 = cur_col-1; else x1 = 1;
      if (cur_col < JTP_MAP_WIDTH-1) x2 = cur_col+1; else x2 = JTP_MAP_WIDTH-1;

      for (i = y1; i <= y2; i++)
        for (j = x1; j <= x2; j++)
        {
          vertices_searched++;

          /* Don't remap center square */
          if ((i == cur_row) && (j == cur_col)) continue;
          /*
           * Search is slightly biased against diagonal movement.
           * From Pythagoras' theorem, diagonal distance is sqrt(2).
           */
          if ((i != cur_row) && (j != cur_col))
            delta_length = 1.414213562;
          else delta_length = 1;
                     
          if (distances[i][j] > cur_biased_length + delta_length)          
            distances[i][j] = cur_biased_length + delta_length;          
        }
      knowns[cur_row][cur_col] = 1;
    }
  }

  /* Change distances to squared distances */
  for (i = JTP_MAP_HEIGHT-1; i >= 0; i--)
    for (j = JTP_MAP_WIDTH-1; j >= 1; j--)
      distances[i][j] *= distances[i][j];
}

/*
 * jtp_flood_fill_room
 *
 * Assigns room index to all pixels within a room,
 * using a recursive Flood Fill algorithm.
 */
void jtp_flood_fill_room
(
  int row1, int col1,
  int roomindex,
  int ** roomindices
)
{
  int i, j;

  /* Basic case: nothing to do */
  if ((roomindices[row1][col1] < 0) ||
      (roomindices[row1][col1] == roomindex))
    return;

  roomindices[row1][col1] = roomindex;
  if (col1 > 1) jtp_flood_fill_room(row1, col1-1, roomindex, roomindices);
  if (row1 > 0) jtp_flood_fill_room(row1-1, col1, roomindex, roomindices);
  if (col1 < JTP_MAP_WIDTH-1) jtp_flood_fill_room(row1, col1+1, roomindex, roomindices);
  if (row1 < JTP_MAP_HEIGHT-1) jtp_flood_fill_room(row1+1, col1, roomindex, roomindices);
}

/* 
 * jtp_find_room_indices
 *
 * Assign room indices to room squares.
 */
void jtp_find_room_indices
(
  int ** roomindices  /* Preallocated storage array for room indices) */
)
{
  int cur_glyph;
  int all_known;
  int roomindex;
  int i, j;
  char tempbuffer[1024];

  /*
   * Create initial indices array
   */
  for (i = JTP_MAP_HEIGHT-1; i >= 0; i--)
    for (j = JTP_MAP_WIDTH-1; j >= 1; j--)
    {
      cur_glyph = levl[j][i].typ;
         
      /* 
       * If the square is not a room square,
       * we don't give it an index.
       */
      if ((cur_glyph == STONE) ||
          (cur_glyph == VWALL) ||
          (cur_glyph == HWALL) ||
          (cur_glyph == TLCORNER) ||
          (cur_glyph == TRCORNER) ||
          (cur_glyph == BLCORNER) ||
          (cur_glyph == BRCORNER) ||
          (cur_glyph == CROSSWALL) ||
          (cur_glyph == TUWALL) ||
          (cur_glyph == TDWALL) ||
          (cur_glyph == TRWALL) ||
          (cur_glyph == DBWALL) ||
          (cur_glyph == SDOOR) ||
          (cur_glyph == SCORR) ||
          (cur_glyph == MOAT) ||
          (cur_glyph == POOL) ||
          (cur_glyph == WATER) ||
          (cur_glyph == DRAWBRIDGE_UP) ||
          (cur_glyph == LAVAPOOL) ||
          (cur_glyph == IRONBARS) ||
          (cur_glyph == DOOR) ||
          (cur_glyph == CORR) ||
          (cur_glyph == DRAWBRIDGE_DOWN))
        roomindices[i][j] = -1; /* Square is not part of a room */
      else                                   
        roomindices[i][j] = 0;  /* Square is part of a room */
    }

  all_known = 0;
  roomindex = 1;
  while (!all_known)
  {
    /* Find a room square with zero (unknown) room index */
    all_known = 1;
    for (i = JTP_MAP_HEIGHT-1; i >= 0; i--)
      for (j = JTP_MAP_WIDTH-1; j >= 1; j--)
        if (roomindices[i][j] == 0)
        {
          jtp_flood_fill_room(i, j, roomindex, roomindices);
          roomindex++;
          all_known = 0;

          /* Exit loop */
          i = -1; j = -1; 
        }
  }
}



void jtp_tooltip_location
(
  int moux,
  int mouy, 
  jtp_mouse_cursor *m_cursor, 
  int *ttip_x, 
  int *ttip_y, 
  unsigned char *ttip
)
{
 if (!ttip)
 {
   *ttip_x = moux; *ttip_y = mouy; return;
 }

 if (moux + m_cursor->xmod + 256*(int)ttip[2] + ttip[3] > jtp_screen.width)
   *ttip_x = jtp_screen.width - 256*(int)ttip[2] - ttip[3];
 else if (moux + m_cursor->xmod < 0)
   *ttip_x = 0;
 else  
   *ttip_x = moux + m_cursor->xmod;

 if (mouy + m_cursor->ymod + m_cursor->graphic[1] + ttip[1] > jtp_screen.height)
   *ttip_y = mouy - ttip[1];
 else if (mouy + m_cursor->ymod + m_cursor->graphic[1] < 0)
   *ttip_y = 0;
 else  
   *ttip_y = mouy + m_cursor->ymod + m_cursor->graphic[1];
}


unsigned char * jtp_make_tooltip
(
  char *tip_text
)
{
 unsigned char *temp;
 int i,j,xsize,ysize;

 xsize = jtp_text_length(tip_text, JTP_FONT_TOOLTIP) + 6;
 ysize = jtp_text_height(tip_text, JTP_FONT_TOOLTIP) + 6;

 temp = (unsigned char *)malloc((xsize*ysize+4)*sizeof(unsigned char));
 if (!temp)
 {
   jtp_write_log_message("[jtp_win.c/jtp_make_tooltip/Check1] Out of memory!\n");
   jtp_exit_graphics(); exit(1);
 }
 temp[0] = ysize/256;
 temp[1] = ysize&255;
 temp[2] = xsize/256;
 temp[3] = xsize&255;
 memset(temp+4,255,xsize*ysize);
 for (i = 0; i < xsize; i++)
 {
   temp[i+4] = 0;
   temp[(ysize-1)*xsize+i+4] = 0;
 }
 for (i = 0; i < ysize; i++)
 {
    temp[i*xsize+4] = 0;
    temp[i*xsize+xsize-1+4] = 0;
 }
 jtp_put_text(3, 3+jtp_fonts[JTP_FONT_TOOLTIP].baseline, JTP_FONT_TOOLTIP, 0, tip_text, temp);
 return(temp);
}

int jtp_mouse_hotspot()
{
  int hotspot;
  
  if (jtp_mouse_area(0, 0, jtp_screen.width-1, 20)) 
    hotspot = JTP_HOTSPOT_SCROLL_UP;
  else if (jtp_mouse_area(0, jtp_statusbar_y-1-20, 
                          jtp_screen.width-1, jtp_statusbar_y-1)) 
    hotspot = JTP_HOTSPOT_SCROLL_DOWN;
  else if (jtp_mouse_area(0, 0, 20, jtp_statusbar_y-1))
    hotspot = JTP_HOTSPOT_SCROLL_LEFT;
  else if (jtp_mouse_area(jtp_screen.width-1-20, 0, 
                          jtp_screen.width-1, jtp_statusbar_y-1))
    hotspot = JTP_HOTSPOT_SCROLL_RIGHT;
  else if (jtp_mouse_area(jtp_statusbar_x + 625, jtp_statusbar_y + 3, 
                          jtp_statusbar_x + 675, jtp_statusbar_y + 46)) 
    hotspot = JTP_HOTSPOT_BUTTON_MAP;
  else if (jtp_mouse_area(jtp_statusbar_x + 680, jtp_statusbar_y + 0, 
                          jtp_statusbar_x + 731, jtp_statusbar_y + 47))
    hotspot = JTP_HOTSPOT_BUTTON_MESSAGES;
  else if (jtp_mouse_area(jtp_statusbar_x + 737, jtp_statusbar_y + 53, 
                          jtp_statusbar_x + 787, jtp_statusbar_y + 91))
    hotspot = JTP_HOTSPOT_BUTTON_HELP;
  else if (jtp_mouse_area(jtp_statusbar_x + 625, jtp_statusbar_y + 50, 
                          jtp_statusbar_x + 675, jtp_statusbar_y + 91))
    hotspot = JTP_HOTSPOT_BUTTON_SPELLBOOK;
  else if (jtp_mouse_area(jtp_statusbar_x + 681, jtp_statusbar_y + 47, 
                          jtp_statusbar_x + 731, jtp_statusbar_y + 91)) 
    hotspot = JTP_HOTSPOT_BUTTON_INVENTORY;
  else if (jtp_mouse_area(jtp_statusbar_x + 568, jtp_statusbar_y + 8, 
                          jtp_statusbar_x + 620, jtp_statusbar_y + 46)) 
    hotspot = JTP_HOTSPOT_BUTTON_LOOK;
  else if (jtp_mouse_area(jtp_statusbar_x + 568, jtp_statusbar_y + 48, 
                          jtp_statusbar_x + 619, jtp_statusbar_y + 91)) 
    hotspot = JTP_HOTSPOT_BUTTON_EXTENDED;
  else if (jtp_mouse_area(jtp_statusbar_x + 737, jtp_statusbar_y + 2, 
                          jtp_statusbar_x + 787, jtp_statusbar_y + 46)) 
    hotspot = JTP_HOTSPOT_BUTTON_OPTIONS;
  else if (jtp_mouse_area(jtp_statusbar_x + 4, jtp_statusbar_y + 4,
                          jtp_statusbar_x + 193, jtp_statusbar_y + 96))
    hotspot = JTP_HOTSPOT_MINI_MAP;
  else if (jtp_mouse_area(0, jtp_statusbar_y, 
                          jtp_screen.width-1, jtp_screen.height-1))
    hotspot = JTP_HOTSPOT_STATUSBAR;
  else hotspot = JTP_HOTSPOT_MAP;
  
  return(hotspot);
}

char * jtp_map_square_description
(
  int tgt_x, int tgt_y,
  int include_seen
)
{
  /*
   * Choose an explanatory tooltip for the target square.
   * This code is based on do_look (in pager.c).
   * It would be better to make it a separate function.
   */
  struct permonst *pm = 0;
  char   *out_str, look_buf[BUFSZ];
  char   temp_buf[BUFSZ], coybuf[QBUFSZ];  
  char   monbuf[BUFSZ];
  register struct monst *mtmp = (struct monst *) 0;
  const char *firstmatch = 0;

  if ((tgt_x < 1) || (tgt_x >= JTP_MAP_WIDTH) || (tgt_y < 0) || (tgt_y >= JTP_MAP_HEIGHT))
    return(NULL);
  
  /* 
   * Only objects and monsters get a tooltip.
   * Traps and some cmap objects should also (on the 'to do' list).
   */
  if ((jtp_mapglyph_mon[tgt_y][tgt_x] >= 0) ||
      (jtp_mapglyph_obj[tgt_y][tgt_x] >= 0))
  {
    out_str = (char *)malloc(BUFSZ);
    out_str[0] = '\0';
  
    pm = jtp_do_lookat(tgt_x, tgt_y, look_buf, monbuf);
    firstmatch = look_buf;
    if (*firstmatch) {
      mtmp = m_at(tgt_x,tgt_y);
      Sprintf(temp_buf, "%s",
  		(pm == &mons[PM_COYOTE]) ?
  		coyotename(mtmp,coybuf) : firstmatch);
      (void)strncat(out_str, temp_buf, BUFSZ-strlen(out_str)-1);
      /* found = 1;	we have something to look up */
    }
    if (include_seen)
    {
      if (monbuf[0]) {
        Sprintf(temp_buf, " [seen: %s]", monbuf);
        (void)strncat(out_str, temp_buf, BUFSZ-strlen(out_str)-1);
      }
    }
    return(out_str);
  }  
  return(NULL);
}


unsigned char * jtp_choose_target_tooltip
(
  int tgt_x, int tgt_y
)
{
  unsigned char * new_tip;
  char * out_str;

  out_str = jtp_map_square_description(tgt_x, tgt_y, 1);
  if (out_str)
  {
    new_tip = jtp_make_tooltip(out_str);
    return(new_tip);
  }
  return(NULL);
}


unsigned char *jtp_choose_tooltip
(
  int hotspot
)
{
  unsigned char * new_tip;

  switch (hotspot)
  {
    case JTP_HOTSPOT_NONE:
      new_tip = NULL;
      break;
    case JTP_HOTSPOT_MAP:
      new_tip = NULL;
      break;
    case JTP_HOTSPOT_SCROLL_UP:
      new_tip = jtp_make_tooltip("Scroll Up");
      break;
    case JTP_HOTSPOT_SCROLL_DOWN:
      new_tip = jtp_make_tooltip("Scroll Down");
      break;
    case JTP_HOTSPOT_SCROLL_LEFT:
      new_tip = jtp_make_tooltip("Scroll Left");
      break;
    case JTP_HOTSPOT_SCROLL_RIGHT:
      new_tip = jtp_make_tooltip("Scroll Right");
      break;
    case JTP_HOTSPOT_BUTTON_MAP:
      new_tip = jtp_make_tooltip("View Map");
      break;
    case JTP_HOTSPOT_BUTTON_MESSAGES:
      new_tip = jtp_make_tooltip("Old Messages");
      break;
    case JTP_HOTSPOT_BUTTON_HELP:
      new_tip = jtp_make_tooltip("Help");
      break;
    case JTP_HOTSPOT_BUTTON_SPELLBOOK:
      new_tip = jtp_make_tooltip("View Spells");
      break;
    case JTP_HOTSPOT_BUTTON_INVENTORY:
      new_tip = jtp_make_tooltip("Inventory");
      break;
    case JTP_HOTSPOT_BUTTON_LOOK:
      new_tip = jtp_make_tooltip("Look");
      break;
    case JTP_HOTSPOT_BUTTON_EXTENDED:
      new_tip = jtp_make_tooltip("Extended Commands");
      break;
    case JTP_HOTSPOT_BUTTON_OPTIONS:
      new_tip = jtp_make_tooltip("Options");
      break;      
    default:
      new_tip = NULL;
      break;
  }
  return(new_tip);
}


jtp_mouse_cursor * jtp_choose_target_cursor
(
  int tgt_x, int tgt_y
)
{
  int cur_tile;

  if ((tgt_x < 1) || (tgt_x >= JTP_MAP_WIDTH) || (tgt_y < 0) || (tgt_y >= JTP_MAP_HEIGHT))
    return(jtp_mcursor[JTP_CURSOR_TARGET_INVALID]);

  if (jtp_whatis_active)
    return(jtp_mcursor[JTP_CURSOR_TARGET_HELP]);
    
  if (jtp_mapglyph_mon[tgt_y][tgt_x] >= 0)
    if ((tgt_x != jtp_you_x) || (tgt_y != jtp_you_y))
      return(jtp_mcursor[JTP_CURSOR_TARGET_RED]);
  if (jtp_mapglyph_obj[tgt_y][tgt_x] >= 0)  
    return(jtp_mcursor[JTP_CURSOR_TARGET_RED]);
  if (jtp_mapglyph_cmap[tgt_y][tgt_x] >= 0)
  {
    /* Closed doors get an 'open door' cursor */
    if ((jtp_mapglyph_cmap[tgt_y][tgt_x] == S_vcdoor) ||
        (jtp_mapglyph_cmap[tgt_y][tgt_x] == S_hcdoor))
      return(jtp_mcursor[JTP_CURSOR_OPENDOOR]);

    /* Stairs and ladders get a 'stairs' cursor */
    if ((jtp_mapglyph_cmap[tgt_y][tgt_x] == S_upstair) ||
        (jtp_mapglyph_cmap[tgt_y][tgt_x] == S_dnstair) ||
        (jtp_mapglyph_cmap[tgt_y][tgt_x] == S_upladder) ||
        (jtp_mapglyph_cmap[tgt_y][tgt_x] == S_dnladder))
      return(jtp_mcursor[JTP_CURSOR_STAIRS]);

    /* Fountains get a 'goblet' cursor */
    if (jtp_mapglyph_cmap[tgt_y][tgt_x] == S_fountain)
      return(jtp_mcursor[JTP_CURSOR_GOBLET]);

    cur_tile = jtp_cmap_to_tile(jtp_mapglyph_cmap[tgt_y][tgt_x]);
    if (cur_tile != JTP_TILE_WALL_GENERIC)
      return(jtp_mcursor[JTP_CURSOR_TARGET_GREEN]);    
    return(jtp_mcursor[JTP_CURSOR_TARGET_INVALID]);  
  }  
  return(jtp_mcursor[JTP_CURSOR_TARGET_INVALID]);
}




void jtp_get_mouse_input
(
  jtp_mouse_cursor * m_cursor,
  int whenstop
)
{
  int        target_x = -1, target_y = -1;
  int        target_old_x = -1, target_old_y = -1;

  int        hotspot = JTP_HOTSPOT_NONE;
  int        old_hotspot = JTP_HOTSPOT_NONE;

  unsigned char *m_bg = NULL;
  jtp_mouse_cursor *m_old_cursor = m_cursor;

  int        ttip_x = -1, ttip_y = -1;
  int        ttip_old_x = -1, ttip_old_y = -1;
  unsigned char *tooltip = NULL;
  unsigned char *ttip_bg = NULL;
  unsigned char *ttip_old_bg = NULL;
  unsigned char tempkey;


  int         i, j;
  char        forcedraw = 1;
  char        stopmouse = 0;

  jtp_readmouse();

  do
  {
    /* 
       At this point the pointer status is:
       tooptip:     not NULL, may change on mouse movement
       ttip_bg:     not NULL, changes on mouse movement
       ttip_old_bg: NULL
       m_bg:        NULL
    */
  
    if ((jtp_oldmx!=jtp_mousex) || (jtp_oldmy!=jtp_mousey) || forcedraw)
    {
      /* 
       * Find the map square under the mouse cursor.
       * The isometric mapping is a matrix operation, y = Ax+b, where the y are screen 
       * coordinates, the x are map square indices and A, b are constant. 
       * Here we use an inverse mapping to find map indices from pixel coordinates.
       */
      target_old_x = target_x; target_old_y = target_y;
      i = jtp_mousex - jtp_map_center_x + (jtp_map_x-jtp_map_y)*JTP_MAP_XMOD;
      j = jtp_mousey - jtp_map_center_y + (jtp_map_x+jtp_map_y)*JTP_MAP_YMOD;
      target_x = (JTP_MAP_YMOD*i + JTP_MAP_XMOD*j + JTP_MAP_XMOD*JTP_MAP_YMOD)/(2*JTP_MAP_XMOD*JTP_MAP_YMOD);
      target_y = (-JTP_MAP_YMOD*i + JTP_MAP_XMOD*j + JTP_MAP_XMOD*JTP_MAP_YMOD)/(2*JTP_MAP_XMOD*JTP_MAP_YMOD);

      /* Store map square indices */
      jtp_map_tgtx = target_x;
      jtp_map_tgty = target_y;
      
      /* Find the current hotspot */
      old_hotspot = hotspot;
      hotspot = jtp_mouse_hotspot();
                              
      /* Choose a mouse cursor for current hotspot */
      m_old_cursor = m_cursor; /* Necessary to get correct refresh area */
      switch (hotspot)
      {
        case JTP_HOTSPOT_MAP:
          m_cursor = jtp_choose_target_cursor(target_x, target_y);
          break;
        case JTP_HOTSPOT_SCROLL_UP:
          m_cursor = jtp_mcursor[JTP_CURSOR_SCROLLUP];
          break;
        case JTP_HOTSPOT_SCROLL_DOWN:
          m_cursor = jtp_mcursor[JTP_CURSOR_SCROLLDOWN];
          break;
        case JTP_HOTSPOT_SCROLL_LEFT:
          m_cursor = jtp_mcursor[JTP_CURSOR_SCROLLLEFT];
          break;
        case JTP_HOTSPOT_SCROLL_RIGHT:
          m_cursor = jtp_mcursor[JTP_CURSOR_SCROLLRIGHT];
          break;
        default:
          m_cursor = jtp_mcursor[JTP_CURSOR_NORMAL];
          break;
      }

      /* DEBUG */ if (!m_cursor) m_cursor = jtp_mcursor[JTP_CURSOR_NORMAL]; /* DEBUG */

      /* Store the background of the new cursor */
      m_bg = jtp_get_img(jtp_mousex+m_cursor->xmod,jtp_mousey+m_cursor->ymod,
                         jtp_mousex+m_cursor->xmod+m_cursor->graphic[3],
                         jtp_mousey+m_cursor->ymod+m_cursor->graphic[1]);

      /* Choose a new tooltip for current hotspot, if changed */
      ttip_old_bg = ttip_bg;
      ttip_old_x = ttip_x;
      ttip_old_y = ttip_y;
      if (hotspot == JTP_HOTSPOT_MAP)
      {
        if ((old_hotspot != JTP_HOTSPOT_MAP) ||
            (target_x != target_old_x) || (target_y != target_old_y))
        {
          free(tooltip);
          tooltip = jtp_choose_target_tooltip(target_x, target_y);
          jtp_tooltip_location(jtp_mousex, jtp_mousey, m_cursor, &ttip_x, &ttip_y, tooltip);          
        }  
      }
      else if (hotspot != old_hotspot)
      {
        free(tooltip);
        tooltip = jtp_choose_tooltip(hotspot);
        jtp_tooltip_location(jtp_mousex, jtp_mousey, m_cursor, &ttip_x, &ttip_y, tooltip);        
      }

      /* Store the background of the new tooltip */
      if (tooltip) 
      {
        ttip_bg = jtp_get_img(ttip_x, ttip_y, 
                              ttip_x + tooltip[2]*256 + tooltip[3] - 1, 
                              ttip_y + tooltip[0]*256 + tooltip[1] - 1);
      }
      else ttip_bg = NULL;

      /* Draw mouse cursor and tooltip */
      jtp_put_img(ttip_x, ttip_y, tooltip);
      jtp_put_stencil(jtp_mousex+m_cursor->xmod, jtp_mousey+m_cursor->ymod, m_cursor->graphic);
 
      if (forcedraw)
      {
        /* Refresh the entire screen */
        jtp_refresh();
        forcedraw = 0;
      }
      else
      {
        /* Refresh old and new areas of mouse cursor */
        jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                jtp_mousex + m_cursor->xmod + m_cursor->graphic[3] - 1, 
                jtp_mousey + m_cursor->ymod + m_cursor->graphic[1] - 1);
        jtp_refresh_region(jtp_oldmx + m_old_cursor->xmod, jtp_oldmy + m_old_cursor->ymod,
                jtp_oldmx + m_old_cursor->xmod + m_old_cursor->graphic[3] - 1, 
                jtp_oldmy + m_old_cursor->ymod + m_old_cursor->graphic[1] - 1);
 
        /* Refresh old and new areas of tooltip */
        if (ttip_bg)
          jtp_refresh_region(ttip_x, ttip_y,
                  ttip_x + ttip_bg[2]*256 + ttip_bg[3] - 1, 
                  ttip_y + ttip_bg[0]*256 + ttip_bg[1] - 1);
        if (ttip_old_bg)
          jtp_refresh_region(ttip_old_x, ttip_old_y,
                  ttip_old_x + ttip_old_bg[2]*256 + ttip_old_bg[3] - 1, 
                  ttip_old_y + ttip_old_bg[0]*256 + ttip_old_bg[1] - 1);
      }

      /* Restore mouse background */
      jtp_put_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_bg);
 
      /* Restore tooltip background */
      if (ttip_bg) jtp_put_img(ttip_x, ttip_y, ttip_bg);

      /* Now that we've used them in a screen refresh, the old backgrounds can be freed */
      free(ttip_old_bg); ttip_old_bg = NULL;
      free(m_bg); m_bg = NULL;
    }
    
    jtp_readmouse(); 
    if ((whenstop >= 0) && (jtp_mouseb == whenstop)) stopmouse = 1;
    else if (jtp_mouseb > 0) stopmouse = 1;
  }
  while ((!stopmouse) && (!jtp_kbhit()));

  /* Erase leftover tooltip from screen (keep mouse cursor to avoid flicker) */
  if (ttip_bg)
    jtp_refresh_region(ttip_x, ttip_y,
                       ttip_x + ttip_bg[2]*256 + ttip_bg[3],
                       ttip_y + ttip_bg[0]*256 + ttip_bg[1]);
  free(ttip_bg);
  free(tooltip);
}


void jtp_get_mouse_appearance
(
  jtp_mouse_cursor ** m_cursor,
  unsigned char ** tooltip,
  int * hotspot,
  jtp_hotspot ** hotspots,
  int n_hotspots
)
{
  int i;

  for (i = 0; i < n_hotspots; i++)
  {
    if ((jtp_mousex >= (hotspots[i])->x1) && (jtp_mousex <= (hotspots[i])->x2) &&
        (jtp_mousey >= (hotspots[i])->y1) && (jtp_mousey <= (hotspots[i])->y2))
    {
      *hotspot = i;
      *m_cursor = (hotspots[i])->mcursor;
      *tooltip = (hotspots[i])->tooltip;
      return;
    }
  }
  *m_cursor = jtp_mcursor[JTP_CURSOR_NORMAL];
  *tooltip = NULL;
  *hotspot = -1;
}


int jtp_get_mouse_inventory_input
(
  jtp_mouse_cursor * m_cursor,
  jtp_hotspot ** hotspots,
  int n_hotspots,
  int whenstop
)
{
  int        target_x = -1, target_y = -1;
  int        target_old_x = -1, target_old_y = -1;

  int        hotspot = -1;
  int        old_hotspot = -1;

  unsigned char *m_bg = NULL;
  jtp_mouse_cursor *m_old_cursor = m_cursor;

  int        ttip_x = -1, ttip_y = -1;
  int        ttip_old_x = -1, ttip_old_y = -1;
  unsigned char *tooltip = NULL;
  unsigned char *ttip_bg = NULL;
  unsigned char *ttip_old_bg = NULL;
  unsigned char tempkey;

  int         i, j;
  char        forcedraw = 1;
  char        stopmouse = 0;

  jtp_readmouse();

  do
  {
    /* 
       At this point the pointer status is:
       tooptip:     not NULL, may change on mouse movement
       ttip_bg:     not NULL, changes on mouse movement
       ttip_old_bg: NULL
       m_bg:        NULL
    */
  
    if ((jtp_oldmx!=jtp_mousex) || (jtp_oldmy!=jtp_mousey) || forcedraw)
    {      
      /* Find a mouse cursor and tooltip for the current location */
      m_old_cursor = m_cursor; /* Necessary to get correct refresh area */
      ttip_old_bg = ttip_bg;
      ttip_old_x = ttip_x;
      ttip_old_y = ttip_y;

      old_hotspot = hotspot;
      jtp_get_mouse_appearance(&m_cursor, &tooltip, &hotspot, hotspots, n_hotspots);
      /* DEBUG */ if (!m_cursor) m_cursor = jtp_mcursor[JTP_CURSOR_NORMAL]; /* DEBUG */

      /* Store the background of the new cursor */
      m_bg = jtp_get_img(jtp_mousex+m_cursor->xmod,jtp_mousey+m_cursor->ymod,
                         jtp_mousex+m_cursor->xmod+m_cursor->graphic[3],
                         jtp_mousey+m_cursor->ymod+m_cursor->graphic[1]);

      /* Store the background of the new tooltip */
      if (hotspot != old_hotspot)
        jtp_tooltip_location(jtp_mousex, jtp_mousey, m_cursor, &ttip_x, &ttip_y, tooltip);
      if (tooltip) 
      {
        ttip_bg = jtp_get_img(ttip_x, ttip_y, 
                              ttip_x + tooltip[2]*256 + tooltip[3] - 1, 
                              ttip_y + tooltip[0]*256 + tooltip[1] - 1);
      }
      else ttip_bg = NULL;

      /* Draw mouse cursor and tooltip */
      jtp_put_img(ttip_x, ttip_y, tooltip);
      jtp_put_stencil(jtp_mousex+m_cursor->xmod, jtp_mousey+m_cursor->ymod, m_cursor->graphic);
 
      if (forcedraw)
      {
        /* Refresh the entire screen */
        jtp_refresh();
        forcedraw = 0;
      }
      else
      {
        /* Refresh old and new areas of mouse cursor */
        jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                jtp_mousex + m_cursor->xmod + m_cursor->graphic[3] - 1, 
                jtp_mousey + m_cursor->ymod + m_cursor->graphic[1] - 1);
        jtp_refresh_region(jtp_oldmx + m_old_cursor->xmod, jtp_oldmy + m_old_cursor->ymod,
                jtp_oldmx + m_old_cursor->xmod + m_old_cursor->graphic[3] - 1, 
                jtp_oldmy + m_old_cursor->ymod + m_old_cursor->graphic[1] - 1);
 
        /* Refresh old and new areas of tooltip */
        if (ttip_bg)
          jtp_refresh_region(ttip_x, ttip_y,
                  ttip_x + ttip_bg[2]*256 + ttip_bg[3] - 1, 
                  ttip_y + ttip_bg[0]*256 + ttip_bg[1] - 1);
        if (ttip_old_bg)
          jtp_refresh_region(ttip_old_x, ttip_old_y,
                  ttip_old_x + ttip_old_bg[2]*256 + ttip_old_bg[3] - 1, 
                  ttip_old_y + ttip_old_bg[0]*256 + ttip_old_bg[1] - 1);
      }

      /* Restore mouse background */
      jtp_put_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_bg);
 
      /* Restore tooltip background */
      if (ttip_bg) jtp_put_img(ttip_x, ttip_y, ttip_bg);

      /* Now that we've used them in a screen refresh, the old backgrounds can be freed */
      free(ttip_old_bg); ttip_old_bg = NULL;
      free(m_bg); m_bg = NULL;
    }
    
    jtp_readmouse(); 
    if ((whenstop >= 0) && (jtp_mouseb == whenstop)) stopmouse = 1;
    else if (jtp_mouseb > 0) stopmouse = 1;
  }
  while ((!stopmouse) && (!jtp_kbhit()));

  /* Erase leftover tooltip from screen (keep mouse cursor to avoid flicker) */
  if (ttip_bg)
    jtp_refresh_region(ttip_x, ttip_y,
                       ttip_x + ttip_bg[2]*256 + ttip_bg[3],
                       ttip_y + ttip_bg[0]*256 + ttip_bg[1]);
  free(ttip_bg);
  return(hotspot);
}




void jtp_show_wait_cursor()
{
  jtp_mouse_cursor * m_cursor = jtp_mcursor[JTP_CURSOR_HOURGLASS];

  jtp_readmouse();
  
  jtp_put_stencil(jtp_mousex + m_cursor->xmod, 
                  jtp_mousey + m_cursor->ymod, 
                  m_cursor->graphic);
  jtp_refresh_region(jtp_mousex + m_cursor->xmod, 
                     jtp_mousey + m_cursor->ymod,
                     jtp_mousex + m_cursor->xmod + m_cursor->graphic[3] - 1, 
                     jtp_mousey + m_cursor->ymod + m_cursor->graphic[1] - 1);
}


void jtp_get_map_input()
{
  /* 
   * Remove any key presses in the key buffer (this is a design choice:
   * since the response isn't instantaneous, buffered keys usually result
   * in unwanted movement. This could be made into an option.
   */
  while (jtp_kbhit()) jtp_getch(); 

  jtp_get_mouse_input(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
}

void jtp_init_monster_tile_table()
{
  int i, j, k;
  int * monclasstiles;

  jtp_montiles = (int *)malloc(NUMMONS*sizeof(int));
  monclasstiles = (int *)malloc(MAXMCLASSES*sizeof(int));
  if ((!jtp_montiles) || (!monclasstiles))
  {
    jtp_write_log_message("[jtp_win.c/jtp_init_monster_tile_table/Check1] ERROR: Out of memory\n");
    jtp_exit_graphics(); exit(1);
  }

  /* Initialize class default tiles */
  for (i = 0; i < MAXMCLASSES; i++)
    monclasstiles[i] = JTP_TILE_KNIGHT;
  monclasstiles[S_DOG] = JTP_TILE_DOG;
  monclasstiles[S_FELINE] = JTP_TILE_CAT;
  monclasstiles[S_BLOB] = JTP_TILE_JELLY;
  monclasstiles[S_JELLY] = JTP_TILE_JELLY;
  monclasstiles[S_PUDDING] = JTP_TILE_JELLY;
  monclasstiles[S_FUNGUS] = JTP_TILE_JELLY;
  monclasstiles[S_SNAKE] = JTP_TILE_LIZARD;
  monclasstiles[S_LIZARD] = JTP_TILE_LIZARD;
  monclasstiles[S_SPIDER] = JTP_TILE_SPIDER;
  monclasstiles[S_XAN] = JTP_TILE_SPIDER;
  monclasstiles[S_ANT] = JTP_TILE_SPIDER;
  monclasstiles[S_RUSTMONST] = JTP_TILE_SPIDER;
  monclasstiles[S_KOBOLD] = JTP_TILE_GOBLIN;
  monclasstiles[S_GREMLIN] = JTP_TILE_GOBLIN;
  monclasstiles[S_HUMANOID] = JTP_TILE_GOBLIN;
  monclasstiles[S_EYE] = JTP_TILE_EYE;
  monclasstiles[S_COCKATRICE] = JTP_TILE_EYE;
  monclasstiles[S_JABBERWOCK] = JTP_TILE_EYE;
  monclasstiles[S_GNOME] = JTP_TILE_GNOME;
  monclasstiles[S_LEPRECHAUN] = JTP_TILE_GNOME;
  monclasstiles[S_ELEMENTAL] = JTP_TILE_ELEMENTAL;
  monclasstiles[S_OGRE] = JTP_TILE_OGRE;
  monclasstiles[S_GIANT] = JTP_TILE_OGRE;
  monclasstiles[S_NAGA] = JTP_TILE_NAGA;
  monclasstiles[S_DRAGON] = JTP_TILE_GREEN_DRAGON;
  monclasstiles[S_WRAITH] = JTP_TILE_WRAITH;
  monclasstiles[S_GHOST] = JTP_TILE_GHOST;
  monclasstiles[S_ZOMBIE] = JTP_TILE_ZOMBIE;
  monclasstiles[S_GOLEM] = JTP_TILE_STONE_GOLEM;
  monclasstiles[S_RODENT] = JTP_TILE_RAT;
  monclasstiles[S_TROLL] = JTP_TILE_TROLL;
  monclasstiles[S_BAT] = JTP_TILE_BAT;
  monclasstiles[S_QUADRUPED] = JTP_TILE_HORSE;
  monclasstiles[S_UNICORN] = JTP_TILE_HORSE;
  monclasstiles[S_CENTAUR] = JTP_TILE_HORSE;
  monclasstiles[S_ANT] = JTP_TILE_ANT;

  /* Assign class based monster tiles first */
  for (i = 0; i < NUMMONS; i++)
  {
    /* If the class is recognizable, use that */
    k = (int)mons[i].mlet;
    if ((k >= 0) && (k < MAXMCLASSES))
      jtp_montiles[i] = monclasstiles[k];
    else jtp_montiles[i] = JTP_TILE_KNIGHT;
  }

  /* Adjust for individual monster types and exceptions */
  jtp_montiles[PM_HOBBIT] = JTP_TILE_GNOME;
  jtp_montiles[PM_DWARF] = JTP_TILE_GNOME;
  jtp_montiles[PM_DWARF_LORD] = JTP_TILE_GNOME;
  jtp_montiles[PM_DWARF_KING] = JTP_TILE_GNOME;
  jtp_montiles[PM_ELF] = JTP_TILE_RANGER;
  jtp_montiles[PM_WOODLAND_ELF] = JTP_TILE_RANGER;
  jtp_montiles[PM_GREEN_ELF] = JTP_TILE_RANGER;
  jtp_montiles[PM_GREY_ELF] = JTP_TILE_RANGER;
  jtp_montiles[PM_ELF_LORD] = JTP_TILE_RANGER;
  jtp_montiles[PM_ELVENKING] = JTP_TILE_RANGER;
  jtp_montiles[PM_BUGBEAR] = JTP_TILE_GOBLIN;
  jtp_montiles[PM_GOBLIN] = JTP_TILE_GOBLIN;
  jtp_montiles[PM_HOBGOBLIN] = JTP_TILE_GOBLIN;
  jtp_montiles[PM_ORC] = JTP_TILE_GOBLIN;
  jtp_montiles[PM_HILL_ORC] = JTP_TILE_GOBLIN;
  jtp_montiles[PM_MORDOR_ORC] = JTP_TILE_GOBLIN;
  jtp_montiles[PM_URUK_HAI] = JTP_TILE_GOBLIN;
  jtp_montiles[PM_ORC_SHAMAN] = JTP_TILE_GOBLIN;
  jtp_montiles[PM_ORC_CAPTAIN] = JTP_TILE_GOBLIN;
  jtp_montiles[PM_KNIGHT] = JTP_TILE_KNIGHT;
  jtp_montiles[PM_SAMURAI] = JTP_TILE_KNIGHT;
  jtp_montiles[PM_MONK] = JTP_TILE_WIZARD;
  jtp_montiles[PM_HEALER] = JTP_TILE_PRIEST;
  jtp_montiles[PM_PRIEST] = JTP_TILE_PRIEST;
  jtp_montiles[PM_PRIESTESS] = JTP_TILE_PRIEST;
  jtp_montiles[PM_WIZARD] = JTP_TILE_WIZARD;
  jtp_montiles[PM_VALKYRIE] = JTP_TILE_VALKYRIE;
  jtp_montiles[PM_RANGER] = JTP_TILE_RANGER;
  jtp_montiles[PM_ROGUE] = JTP_TILE_ROGUE;
  jtp_montiles[PM_ARCHEOLOGIST] = JTP_TILE_ARCHEOLOGIST;
  jtp_montiles[PM_WOOD_NYMPH] = JTP_TILE_NYMPH;
  jtp_montiles[PM_WATER_NYMPH] = JTP_TILE_WATER_NYMPH;
  jtp_montiles[PM_MOUNTAIN_NYMPH] = JTP_TILE_NYMPH;
  jtp_montiles[PM_SUCCUBUS] = JTP_TILE_NYMPH;
  jtp_montiles[PM_LICH] = JTP_TILE_SKELETON;
  jtp_montiles[PM_DEMILICH] = JTP_TILE_SKELETON;
  jtp_montiles[PM_MASTER_LICH] = JTP_TILE_SKELETON;
  jtp_montiles[PM_ARCH_LICH] = JTP_TILE_SKELETON;
  jtp_montiles[PM_SKELETON] = JTP_TILE_SKELETON;
  jtp_montiles[PM_KILLER_BEE] = JTP_TILE_BEE;
  jtp_montiles[PM_QUEEN_BEE] = JTP_TILE_BEE;
  jtp_montiles[PM_TOURIST] = JTP_TILE_TOURIST;

  /* Clean up */
  free(monclasstiles);
}


void jtp_init_object_tile_table()
{
  int i, j, k;
  int * objclasstiles;
  char * temp_descr;

  jtp_objtiles = (int *)malloc(NUM_OBJECTS*sizeof(int));
  objclasstiles = (int *)malloc(MAXOCLASSES*sizeof(int));
  if ((!jtp_objtiles) || (!objclasstiles))
  {
    jtp_write_log_message("[jtp_win.c/jtp_init_object_tile_table/Check1] ERROR: Out of memory\n");
    jtp_exit_graphics(); exit(1);
  }

  /* Initialize class default tiles */
  for (i = 0; i < MAXOCLASSES; i++)
    objclasstiles[i] = JTP_TILE_BAG;
  objclasstiles[ROCK_CLASS] = JTP_TILE_BOULDER;
  objclasstiles[COIN_CLASS] = JTP_TILE_COINS;
  objclasstiles[SPBOOK_CLASS] = JTP_TILE_BOOK;
  objclasstiles[SCROLL_CLASS] = JTP_TILE_SCROLL;
  objclasstiles[POTION_CLASS] = JTP_TILE_BOTTLE;
  objclasstiles[WEAPON_CLASS] = JTP_TILE_SPEAR;
  objclasstiles[ARMOR_CLASS] = JTP_TILE_HELMET;
  objclasstiles[WAND_CLASS] = JTP_TILE_WAND;
  objclasstiles[GEM_CLASS] = JTP_TILE_GEM_BLUE;
  objclasstiles[RING_CLASS] = JTP_TILE_RING;
  objclasstiles[AMULET_CLASS] = JTP_TILE_AMULET;
  objclasstiles[FOOD_CLASS] = JTP_TILE_FOOD_RATION;

  /* Assign class based object tiles first */
  for (i = 0; i < NUM_OBJECTS; i++)
  {
    /* If the object class is recognizable, use that */
    k = (int)objects[i].oc_class;
    if ((k >= 0) && (k < MAXOCLASSES))
      jtp_objtiles[i] = objclasstiles[k];
    else jtp_objtiles[i] = JTP_TILE_BAG;
  }

  /* Assign gems by color */
  for (i = 0; i < NUM_OBJECTS; i++)
    if ((int)objects[i].oc_class == GEM_CLASS)
    {
      temp_descr = (char *)(OBJ_NAME(objects[i]));
      if (temp_descr)
      {
        if (strstr(temp_descr, "red")) jtp_objtiles[i] = JTP_TILE_GEM_RED;          
        else if (strstr(temp_descr, "orange")) jtp_objtiles[i] = JTP_TILE_GEM_YELLOW;
        else if (strstr(temp_descr, "yellow")) jtp_objtiles[i] = JTP_TILE_GEM_YELLOW;
        else if (strstr(temp_descr, "green")) jtp_objtiles[i] = JTP_TILE_GEM_GREEN;
        else if (strstr(temp_descr, "blue")) jtp_objtiles[i] = JTP_TILE_GEM_BLUE;
        else if (strstr(temp_descr, "black")) jtp_objtiles[i] = JTP_TILE_GEM_BLACK;
        else if (strstr(temp_descr, "white")) jtp_objtiles[i] = JTP_TILE_GEM_WHITE;
        else if (strstr(temp_descr, "violet")) jtp_objtiles[i] = JTP_TILE_GEM_RED;
        else if (strstr(temp_descr, "orange")) jtp_objtiles[i] = JTP_TILE_GEM_YELLOW;
      }
    }

  /* Adjust for individual object types and exceptions */
  jtp_objtiles[STATUE] = JTP_TILE_STATUE;
  jtp_objtiles[LARGE_BOX] = JTP_TILE_CHEST;
  jtp_objtiles[ICE_BOX] = JTP_TILE_CHEST;
  jtp_objtiles[CHEST] = JTP_TILE_CHEST;

  jtp_objtiles[SMALL_SHIELD] = JTP_TILE_SHIELD;
  jtp_objtiles[ELVEN_SHIELD] = JTP_TILE_SHIELD;
  jtp_objtiles[URUK_HAI_SHIELD] = JTP_TILE_SHIELD;
  jtp_objtiles[LARGE_SHIELD] = JTP_TILE_SHIELD;
  jtp_objtiles[DWARVISH_ROUNDSHIELD] = JTP_TILE_SHIELD;
  jtp_objtiles[SHIELD_OF_REFLECTION] = JTP_TILE_SHIELD;

  jtp_objtiles[ELVEN_LEATHER_HELM] = JTP_TILE_HELMET;
  jtp_objtiles[ORCISH_HELM] = JTP_TILE_HELMET;
  jtp_objtiles[DWARVISH_IRON_HELM] = JTP_TILE_HELMET;
  jtp_objtiles[DENTED_POT] = JTP_TILE_HELMET;
  jtp_objtiles[HELMET] = JTP_TILE_HELMET;
  jtp_objtiles[HELM_OF_BRILLIANCE] = JTP_TILE_HELMET;
  jtp_objtiles[HELM_OF_OPPOSITE_ALIGNMENT] = JTP_TILE_HELMET;
  jtp_objtiles[HELM_OF_TELEPATHY] = JTP_TILE_HELMET;

  jtp_objtiles[SHORT_SWORD] = JTP_TILE_SWORD;
  jtp_objtiles[ELVEN_SHORT_SWORD] = JTP_TILE_SWORD;
  jtp_objtiles[ORCISH_SHORT_SWORD] = JTP_TILE_SWORD;
  jtp_objtiles[DWARVISH_SHORT_SWORD] = JTP_TILE_SWORD;
  jtp_objtiles[SCIMITAR] = JTP_TILE_SWORD;
  jtp_objtiles[SILVER_SABER] = JTP_TILE_SWORD;
  jtp_objtiles[BROADSWORD] = JTP_TILE_SWORD;
  jtp_objtiles[ELVEN_BROADSWORD] = JTP_TILE_SWORD;
  jtp_objtiles[LONG_SWORD] = JTP_TILE_SWORD;
  jtp_objtiles[TWO_HANDED_SWORD] = JTP_TILE_SWORD;
  jtp_objtiles[KATANA] = JTP_TILE_SWORD;
  jtp_objtiles[TSURUGI] = JTP_TILE_SWORD;
  jtp_objtiles[RUNESWORD] = JTP_TILE_SWORD;

  jtp_objtiles[SPEAR] = JTP_TILE_SPEAR;
  jtp_objtiles[ELVEN_SPEAR] = JTP_TILE_SPEAR;
  jtp_objtiles[ORCISH_SPEAR] = JTP_TILE_SPEAR;
  jtp_objtiles[DWARVISH_SPEAR] = JTP_TILE_SPEAR;
  jtp_objtiles[SILVER_SPEAR] = JTP_TILE_SPEAR;
  jtp_objtiles[JAVELIN] = JTP_TILE_SPEAR;
  jtp_objtiles[TRIDENT] = JTP_TILE_SPEAR;

  jtp_objtiles[QUARTERSTAFF] = JTP_TILE_STAFF;
  jtp_objtiles[SHURIKEN] = JTP_TILE_SHURIKEN;
  jtp_objtiles[BOOMERANG] = JTP_TILE_SHURIKEN;

  jtp_objtiles[LOW_BOOTS] = JTP_TILE_BOOTS;
  jtp_objtiles[IRON_SHOES] = JTP_TILE_BOOTS;
  jtp_objtiles[HIGH_BOOTS] = JTP_TILE_BOOTS;
  jtp_objtiles[SPEED_BOOTS] = JTP_TILE_BOOTS;
  jtp_objtiles[WATER_WALKING_BOOTS] = JTP_TILE_BOOTS;
  jtp_objtiles[JUMPING_BOOTS] = JTP_TILE_BOOTS;
  jtp_objtiles[ELVEN_BOOTS] = JTP_TILE_BOOTS;
  jtp_objtiles[KICKING_BOOTS] = JTP_TILE_BOOTS;
  jtp_objtiles[FUMBLE_BOOTS] = JTP_TILE_BOOTS;
  jtp_objtiles[LEVITATION_BOOTS] = JTP_TILE_BOOTS;

  jtp_objtiles[AXE] = JTP_TILE_AXE;
  jtp_objtiles[BATTLE_AXE] = JTP_TILE_AXE;

  jtp_objtiles[LUCERN_HAMMER] = JTP_TILE_HAMMER;
  jtp_objtiles[WAR_HAMMER] = JTP_TILE_HAMMER;

  jtp_objtiles[MACE] = JTP_TILE_MACE;
  jtp_objtiles[MORNING_STAR] = JTP_TILE_MACE;
  jtp_objtiles[FLAIL] = JTP_TILE_MACE;

  jtp_objtiles[BULLWHIP] = JTP_TILE_WHIP;

  jtp_objtiles[STUDDED_LEATHER_ARMOR] = JTP_TILE_LEATHER_ARMOR;
  jtp_objtiles[LEATHER_ARMOR] = JTP_TILE_LEATHER_ARMOR;
  jtp_objtiles[LEATHER_JACKET] = JTP_TILE_LEATHER_ARMOR;

  jtp_objtiles[GRAY_DRAGON_SCALE_MAIL] = JTP_TILE_SCALE_MAIL;
  jtp_objtiles[SILVER_DRAGON_SCALE_MAIL] = JTP_TILE_SCALE_MAIL;
  jtp_objtiles[RED_DRAGON_SCALE_MAIL] = JTP_TILE_SCALE_MAIL;
  jtp_objtiles[WHITE_DRAGON_SCALE_MAIL] = JTP_TILE_SCALE_MAIL;
  jtp_objtiles[ORANGE_DRAGON_SCALE_MAIL] = JTP_TILE_SCALE_MAIL;
  jtp_objtiles[BLACK_DRAGON_SCALE_MAIL] = JTP_TILE_SCALE_MAIL;
  jtp_objtiles[BLUE_DRAGON_SCALE_MAIL] = JTP_TILE_SCALE_MAIL;
  jtp_objtiles[GREEN_DRAGON_SCALE_MAIL] = JTP_TILE_SCALE_MAIL;
  jtp_objtiles[YELLOW_DRAGON_SCALE_MAIL] = JTP_TILE_SCALE_MAIL;
  jtp_objtiles[PLATE_MAIL] = JTP_TILE_PLATE_MAIL;
  jtp_objtiles[CRYSTAL_PLATE_MAIL] = JTP_TILE_PLATE_MAIL;
  jtp_objtiles[BRONZE_PLATE_MAIL] = JTP_TILE_PLATE_MAIL;
  jtp_objtiles[SPLINT_MAIL] = JTP_TILE_PLATE_MAIL;
  jtp_objtiles[BANDED_MAIL] = JTP_TILE_PLATE_MAIL;
  jtp_objtiles[DWARVISH_MITHRIL_COAT] = JTP_TILE_CHAIN_MAIL;
  jtp_objtiles[ELVEN_MITHRIL_COAT] = JTP_TILE_CHAIN_MAIL;
  jtp_objtiles[CHAIN_MAIL] = JTP_TILE_CHAIN_MAIL;
  jtp_objtiles[ORCISH_CHAIN_MAIL] = JTP_TILE_CHAIN_MAIL;
  jtp_objtiles[SCALE_MAIL] = JTP_TILE_SCALE_MAIL;
  jtp_objtiles[RING_MAIL] = JTP_TILE_RING_MAIL;
  jtp_objtiles[ORCISH_RING_MAIL] = JTP_TILE_RING_MAIL;

  jtp_objtiles[LEATHER_GLOVES] = JTP_TILE_GLOVES;
  jtp_objtiles[GAUNTLETS_OF_FUMBLING] = JTP_TILE_GLOVES;
  jtp_objtiles[GAUNTLETS_OF_POWER] = JTP_TILE_GLOVES;
  jtp_objtiles[GAUNTLETS_OF_DEXTERITY] = JTP_TILE_GLOVES;

  jtp_objtiles[BRASS_LANTERN] = JTP_TILE_LANTERN;
  jtp_objtiles[OIL_LAMP] = JTP_TILE_LANTERN;
  jtp_objtiles[MAGIC_LAMP] = JTP_TILE_LANTERN;
  jtp_objtiles[TALLOW_CANDLE] = JTP_TILE_CANDLE;
  jtp_objtiles[WAX_CANDLE] = JTP_TILE_CANDLE;

  jtp_objtiles[ARROW] = JTP_TILE_ARROW;
  jtp_objtiles[ELVEN_ARROW] = JTP_TILE_ARROW;
  jtp_objtiles[ORCISH_ARROW] = JTP_TILE_ARROW;
  jtp_objtiles[SILVER_ARROW] = JTP_TILE_ARROW;
  jtp_objtiles[YA] = JTP_TILE_ARROW;
  jtp_objtiles[CROSSBOW_BOLT] = JTP_TILE_ARROW;
  jtp_objtiles[DART] = JTP_TILE_ARROW;

  jtp_objtiles[DAGGER] = JTP_TILE_DAGGER;
  jtp_objtiles[ELVEN_DAGGER] = JTP_TILE_DAGGER;
  jtp_objtiles[ORCISH_DAGGER] = JTP_TILE_DAGGER;
  jtp_objtiles[SILVER_DAGGER] = JTP_TILE_DAGGER;
  jtp_objtiles[ATHAME] = JTP_TILE_DAGGER;
  jtp_objtiles[SCALPEL] = JTP_TILE_DAGGER;
  jtp_objtiles[KNIFE] = JTP_TILE_DAGGER;
  jtp_objtiles[STILETTO] = JTP_TILE_DAGGER;
  jtp_objtiles[WORM_TOOTH] = JTP_TILE_DAGGER;
  jtp_objtiles[CRYSKNIFE] = JTP_TILE_DAGGER;

  jtp_objtiles[TRIDENT] = JTP_TILE_TRIDENT;
  jtp_objtiles[FEDORA] = JTP_TILE_FEDORA;  

  jtp_objtiles[BOW] = JTP_TILE_BOW;
  jtp_objtiles[ELVEN_BOW] = JTP_TILE_BOW;
  jtp_objtiles[ORCISH_BOW] = JTP_TILE_BOW;
  jtp_objtiles[YUMI] = JTP_TILE_BOW;
  jtp_objtiles[SLING] = JTP_TILE_BOW;
  jtp_objtiles[CROSSBOW] = JTP_TILE_CROSSBOW;

  jtp_objtiles[MUMMY_WRAPPING] = JTP_TILE_CLOAK;
  jtp_objtiles[ELVEN_CLOAK] = JTP_TILE_CLOAK;
  jtp_objtiles[ORCISH_CLOAK] = JTP_TILE_CLOAK;
  jtp_objtiles[DWARVISH_CLOAK] = JTP_TILE_CLOAK;
  jtp_objtiles[OILSKIN_CLOAK] = JTP_TILE_CLOAK;
  jtp_objtiles[ROBE] = JTP_TILE_CLOAK;
  jtp_objtiles[ALCHEMY_SMOCK] = JTP_TILE_CLOAK;
  jtp_objtiles[CLOAK_OF_PROTECTION] = JTP_TILE_CLOAK;
  jtp_objtiles[CLOAK_OF_INVISIBILITY] = JTP_TILE_CLOAK;
  jtp_objtiles[CLOAK_OF_MAGIC_RESISTANCE] = JTP_TILE_CLOAK;
  jtp_objtiles[CLOAK_OF_DISPLACEMENT] = JTP_TILE_CLOAK;  

  jtp_objtiles[HAWAIIAN_SHIRT] = JTP_TILE_HAWAIIAN_SHIRT;
  jtp_objtiles[T_SHIRT] = JTP_TILE_HAWAIIAN_SHIRT;

  jtp_objtiles[CORNUTHAUM] = JTP_TILE_CONICAL_HAT;
  jtp_objtiles[DUNCE_CAP] = JTP_TILE_CONICAL_HAT;  

  jtp_objtiles[SKELETON_KEY] = JTP_TILE_KEY;
  jtp_objtiles[LOCK_PICK] = JTP_TILE_KEY;

  jtp_objtiles[CLUB] = JTP_TILE_CLUB;
  jtp_objtiles[PICK_AXE] = JTP_TILE_PICKAXE;
  jtp_objtiles[GRAPPLING_HOOK] = JTP_TILE_PICKAXE;

  jtp_objtiles[KELP_FROND] = JTP_TILE_PEAR;
  jtp_objtiles[EUCALYPTUS_LEAF] = JTP_TILE_PEAR;
  jtp_objtiles[APPLE] = JTP_TILE_APPLE;
  jtp_objtiles[ORANGE] = JTP_TILE_APPLE;
  jtp_objtiles[PEAR] = JTP_TILE_PEAR;
  jtp_objtiles[MELON] = JTP_TILE_PEAR;
  jtp_objtiles[BANANA] = JTP_TILE_APPLE;
  jtp_objtiles[CARROT] = JTP_TILE_APPLE;
  jtp_objtiles[SPRIG_OF_WOLFSBANE] = JTP_TILE_PEAR;
  jtp_objtiles[CLOVE_OF_GARLIC] = JTP_TILE_APPLE;
  jtp_objtiles[SLIME_MOLD] = JTP_TILE_PEAR;
  jtp_objtiles[LUMP_OF_ROYAL_JELLY] = JTP_TILE_PEAR;
  jtp_objtiles[EGG] = JTP_TILE_EGG;

  jtp_objtiles[FOOD_RATION] = JTP_TILE_FOOD_RATION;
  jtp_objtiles[CRAM_RATION] = JTP_TILE_FOOD_RATION;
  jtp_objtiles[TRIPE_RATION] = JTP_TILE_TRIPE_RATION;
  jtp_objtiles[K_RATION] = JTP_TILE_TRIPE_RATION;
  jtp_objtiles[C_RATION] = JTP_TILE_TRIPE_RATION;
  jtp_objtiles[MEATBALL] = JTP_TILE_MEAT_CHUNK;
  jtp_objtiles[MEAT_STICK] = JTP_TILE_MEAT_CHUNK;
  jtp_objtiles[HUGE_CHUNK_OF_MEAT] = JTP_TILE_MEAT_CHUNK;
  jtp_objtiles[MEAT_RING] = JTP_TILE_MEAT_CHUNK;
  jtp_objtiles[CREAM_PIE] = JTP_TILE_COOKIE;
  jtp_objtiles[CANDY_BAR] = JTP_TILE_COOKIE;
  jtp_objtiles[FORTUNE_COOKIE] = JTP_TILE_COOKIE;
  jtp_objtiles[PANCAKE] = JTP_TILE_COOKIE;
  jtp_objtiles[LEMBAS_WAFER] = JTP_TILE_COOKIE;
  jtp_objtiles[TIN] = JTP_TILE_TIN;
  jtp_objtiles[TINNING_KIT] = JTP_TILE_TIN;
  jtp_objtiles[TIN_OPENER] = JTP_TILE_KEY;
  jtp_objtiles[CAN_OF_GREASE] = JTP_TILE_TIN;

  jtp_objtiles[EXPENSIVE_CAMERA] = JTP_TILE_CAMERA;
  jtp_objtiles[MIRROR] = JTP_TILE_MIRROR;
  jtp_objtiles[CRYSTAL_BALL] = JTP_TILE_CRYSTAL_BALL;
  jtp_objtiles[LENSES] = JTP_TILE_CAMERA;
  jtp_objtiles[MAGIC_MARKER] = JTP_TILE_MAGIC_MARKER;
  jtp_objtiles[CREDIT_CARD] = JTP_TILE_CREDIT_CARD;

  jtp_objtiles[BELL] = JTP_TILE_BELL;
  jtp_objtiles[BELL_OF_OPENING] = JTP_TILE_BELL;
  jtp_objtiles[TOOLED_HORN] = JTP_TILE_HORN;
  jtp_objtiles[HORN_OF_PLENTY] = JTP_TILE_HORN;
  jtp_objtiles[BUGLE] = JTP_TILE_HORN;
  jtp_objtiles[FROST_HORN] = JTP_TILE_GRAND_HORN;
  jtp_objtiles[FIRE_HORN] = JTP_TILE_GRAND_HORN;
  jtp_objtiles[UNICORN_HORN] = JTP_TILE_UNICORN_HORN;

  jtp_objtiles[DILITHIUM_CRYSTAL] = JTP_TILE_GEM_BLUE;
  jtp_objtiles[DIAMOND] = JTP_TILE_GEM_WHITE;
  jtp_objtiles[RUBY] = JTP_TILE_GEM_RED;
  jtp_objtiles[JACINTH] = JTP_TILE_GEM_YELLOW;
  jtp_objtiles[SAPPHIRE] = JTP_TILE_GEM_BLUE;
  jtp_objtiles[BLACK_OPAL] = JTP_TILE_GEM_BLACK;
  jtp_objtiles[EMERALD] = JTP_TILE_GEM_GREEN;
  jtp_objtiles[TURQUOISE] = JTP_TILE_GEM_GREEN;
  jtp_objtiles[CITRINE] = JTP_TILE_GEM_YELLOW;
  jtp_objtiles[AQUAMARINE] = JTP_TILE_GEM_BLUE;
  jtp_objtiles[AMBER] = JTP_TILE_GEM_YELLOW;
  jtp_objtiles[TOPAZ] = JTP_TILE_GEM_YELLOW;
  jtp_objtiles[JET] = JTP_TILE_GEM_BLACK;
  jtp_objtiles[OPAL] = JTP_TILE_GEM_WHITE;
  jtp_objtiles[CHRYSOBERYL] = JTP_TILE_GEM_YELLOW;
  jtp_objtiles[GARNET] = JTP_TILE_GEM_RED;
  jtp_objtiles[AMETHYST] = JTP_TILE_GEM_RED;
  jtp_objtiles[JASPER] = JTP_TILE_GEM_RED;
  jtp_objtiles[FLUORITE] = JTP_TILE_GEM_GREEN;
  jtp_objtiles[OBSIDIAN] = JTP_TILE_GEM_BLACK;
  jtp_objtiles[AGATE] = JTP_TILE_GEM_YELLOW;
  jtp_objtiles[JADE] = JTP_TILE_GEM_GREEN;

  jtp_objtiles[LUCKSTONE] = JTP_TILE_ROCK;
  jtp_objtiles[LOADSTONE] = JTP_TILE_ROCK;
  jtp_objtiles[FLINT] = JTP_TILE_ROCK;
  jtp_objtiles[ROCK] = JTP_TILE_ROCK;

  /* Clean up */
  free(objclasstiles);
}

void jtp_init_glyph_tiles()
{
  int i;
  jtp_traptiles = (int *)malloc(MAXPCHARS*sizeof(int));
  jtp_cmaptiles = (int *)malloc(MAXPCHARS*sizeof(int));
  if ((!jtp_traptiles) || (!jtp_cmaptiles))
  {
    jtp_write_log_message("[jtp_win.c/jtp_init_glyph_tiles/Check1] Out of memory!\n");
    jtp_exit_graphics(); exit(1);
  }

  jtp_init_monster_tile_table();
  jtp_init_object_tile_table();

/*
  jtp_montiles[S_ANT] = 0;
  jtp_montiles[S_BLOB] = 0;
  jtp_montiles[S_COCKATRICE] = 0;
  jtp_montiles[S_EYE] = 0;
  jtp_montiles[S_FELINE] = 0;
  jtp_montiles[S_GREMLIN] = 0;
  jtp_montiles[S_HUMANOID] = 0;
  jtp_montiles[S_IMP] = 0;
  jtp_montiles[S_JELLY] = 0;
  jtp_montiles[S_KOBOLD] = 0;
  jtp_montiles[S_LEPRECHAUN] = 0;
  jtp_montiles[S_MIMIC] = 0;
  jtp_montiles[S_NYMPH] = 0;
  jtp_montiles[S_PIERCER] = 0;
  jtp_montiles[S_QUADRUPED] = 0;
  jtp_montiles[S_RODENT] = 0;
  jtp_montiles[S_SPIDER] = 0;
  jtp_montiles[S_TRAPPER] = 0;
  jtp_montiles[S_UNICORN] = 0;
  jtp_montiles[S_VORTEX] = 0;
  jtp_montiles[S_WORM] = 0;
  jtp_montiles[S_XAN] = 0;
  jtp_montiles[S_LIGHT] = 0;
  jtp_montiles[S_ZRUTY] = 0;
  jtp_montiles[S_ANGEL] = 0;
  jtp_montiles[S_BAT] = 0;
  jtp_montiles[S_CENTAUR] = 0;
  jtp_montiles[S_DRAGON] = 0;
  jtp_montiles[S_ELEMENTAL] = 0;
  jtp_montiles[S_FUNGUS] = 0;
  jtp_montiles[S_GNOME] = 0;
  jtp_montiles[S_GIANT] = 0;
  jtp_montiles[S_JABBERWOCK] = 0;
  jtp_montiles[S_KOP] = 0;
  jtp_montiles[S_LICH] = 0;
  jtp_montiles[S_MUMMY] = 0;
  jtp_montiles[S_NAGA] = 0;
  jtp_montiles[S_OGRE] = 0;
  jtp_montiles[S_PUDDING] = 0;
  jtp_montiles[S_QUANTMECH] = 0;
  jtp_montiles[S_RUSTMONST] = 0;
  jtp_montiles[S_SNAKE] = 0;
  jtp_montiles[S_TROLL] = 0;
  jtp_montiles[S_UMBER] = 0;
  jtp_montiles[S_VAMPIRE] = 0;
  jtp_montiles[S_WRAITH] = 0;
  jtp_montiles[S_XORN] = 0;
  jtp_montiles[S_YETI] = 0;
  jtp_montiles[S_ZOMBIE] = 0;
  jtp_montiles[S_HUMAN] = 0;
  jtp_montiles[S_GHOST] = 0;
  jtp_montiles[S_GOLEM] = 0;
  jtp_montiles[S_DEMON] = 0;
  jtp_montiles[S_EEL] = 0;
  jtp_montiles[S_LIZARD] = 0;
  jtp_montiles[S_WORM_TAIL] = 0;
  jtp_montiles[S_MIMIC_DEF] = 0;
*/  


  for (i = 0; i < MAXPCHARS; i++)
    jtp_traptiles[i] = JTP_TILE_FLOOR_COBBLESTONE;
  
  for (i = 0; i < MAXPCHARS; i++)
    jtp_cmaptiles[i] = JTP_TILE_VDOOR_WOOD_CLOSED;
  jtp_cmaptiles[S_stone] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_vwall] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_hwall] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_tlcorn] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_trcorn] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_blcorn] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_brcorn] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_crwall] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_tuwall] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_tdwall] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_tlwall] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_trwall] = JTP_TILE_WALL_GENERIC;
  jtp_cmaptiles[S_ndoor] = JTP_TILE_DOOR_WOOD_BROKEN;
  jtp_cmaptiles[S_vodoor] = JTP_TILE_VDOOR_WOOD_OPEN;
  jtp_cmaptiles[S_hodoor] = JTP_TILE_HDOOR_WOOD_OPEN;
  jtp_cmaptiles[S_vcdoor] = JTP_TILE_VDOOR_WOOD_CLOSED;
  jtp_cmaptiles[S_hcdoor] = JTP_TILE_HDOOR_WOOD_CLOSED;
  jtp_cmaptiles[S_room] = JTP_TILE_FLOOR_COBBLESTONE;
  jtp_cmaptiles[S_corr] = JTP_TILE_FLOOR_ROUGH;
  jtp_cmaptiles[S_upstair] = JTP_TILE_STAIRS_UP;
  jtp_cmaptiles[S_dnstair] = JTP_TILE_STAIRS_DOWN;
  jtp_cmaptiles[S_fountain] = JTP_TILE_FOUNTAIN;
  jtp_cmaptiles[S_altar] = JTP_TILE_ALTAR;
  jtp_cmaptiles[S_level_teleporter] = JTP_TILE_TRAP_TELEPORTER;
  jtp_cmaptiles[S_magic_portal] = JTP_TILE_TRAP_TELEPORTER;
  jtp_cmaptiles[S_teleportation_trap] = JTP_TILE_TRAP_TELEPORTER;
  jtp_cmaptiles[S_tree] = JTP_TILE_TREE;
  jtp_cmaptiles[S_cloud] = JTP_TILE_CLOUD;
  jtp_cmaptiles[S_air] = JTP_TILE_FLOOR_AIR;
  jtp_cmaptiles[S_grave] = JTP_TILE_GRAVE;
  jtp_cmaptiles[S_sink] = JTP_TILE_SINK;
  jtp_cmaptiles[S_bear_trap] = JTP_TILE_TRAP_BEAR;
  jtp_cmaptiles[S_rust_trap] = JTP_TILE_TRAP_WATER;
  jtp_cmaptiles[S_pit] = JTP_TILE_TRAP_PIT;
  jtp_cmaptiles[S_spiked_pit] = JTP_TILE_TRAP_PIT;
  jtp_cmaptiles[S_hole] = JTP_TILE_TRAP_PIT;
  jtp_cmaptiles[S_trap_door] = JTP_TILE_TRAP_DOOR;
  jtp_cmaptiles[S_magic_trap] = JTP_TILE_TRAP_MAGIC;
  jtp_cmaptiles[S_water] = JTP_TILE_FLOOR_WATER;
  jtp_cmaptiles[S_pool] = JTP_TILE_FLOOR_WATER;
  jtp_cmaptiles[S_lava] = JTP_TILE_FLOOR_LAVA;
  jtp_cmaptiles[S_throne] = JTP_TILE_THRONE;
  jtp_cmaptiles[S_bars] = JTP_TILE_BARS;
  jtp_cmaptiles[S_upladder] = JTP_TILE_LADDER_UP;
  jtp_cmaptiles[S_dnladder] = JTP_TILE_LADDER_DOWN;
  jtp_cmaptiles[S_arrow_trap] = JTP_TILE_TRAP_ARROW;
  jtp_cmaptiles[S_squeaky_board] = JTP_TILE_DOOR_WOOD_BROKEN;
  jtp_cmaptiles[S_dart_trap] = JTP_TILE_TRAP_ARROW;
  jtp_cmaptiles[S_falling_rock_trap] = JTP_TILE_TRAP_FALLING_ROCK;
  jtp_cmaptiles[S_rolling_boulder_trap] = JTP_TILE_TRAP_FALLING_ROCK;
  jtp_cmaptiles[S_land_mine] = JTP_TILE_TRAP_FIRE;
  jtp_cmaptiles[S_sleeping_gas_trap] = JTP_TILE_TRAP_SLEEPGAS;
  jtp_cmaptiles[S_fire_trap] = JTP_TILE_TRAP_FIRE;
  jtp_cmaptiles[S_web] = JTP_TILE_TRAP_BEAR;
  jtp_cmaptiles[S_statue_trap] = JTP_TILE_TRAP_SLEEPGAS;
  jtp_cmaptiles[S_anti_magic_trap] = JTP_TILE_TRAP_ANTI_MAGIC;
  jtp_cmaptiles[S_polymorph_trap] = JTP_TILE_TRAP_MAGIC;
  jtp_cmaptiles[S_vbeam] = JTP_TILE_ZAP_VERTICAL;
  jtp_cmaptiles[S_hbeam] = JTP_TILE_ZAP_HORIZONTAL;
  jtp_cmaptiles[S_lslant] = JTP_TILE_ZAP_SLANT_LEFT;
  jtp_cmaptiles[S_rslant] = JTP_TILE_ZAP_SLANT_RIGHT;
  jtp_cmaptiles[S_explode1] = JTP_TILE_EXPLOSION_NORTHWEST;
  jtp_cmaptiles[S_explode2] = JTP_TILE_EXPLOSION_NORTH;
  jtp_cmaptiles[S_explode3] = JTP_TILE_EXPLOSION_NORTHEAST;
  jtp_cmaptiles[S_explode4] = JTP_TILE_EXPLOSION_WEST;
  jtp_cmaptiles[S_explode5] = JTP_TILE_EXPLOSION_CENTER;
  jtp_cmaptiles[S_explode6] = JTP_TILE_EXPLOSION_EAST;
  jtp_cmaptiles[S_explode7] = JTP_TILE_EXPLOSION_SOUTHWEST;
  jtp_cmaptiles[S_explode8] = JTP_TILE_EXPLOSION_SOUTH;
  jtp_cmaptiles[S_explode9] = JTP_TILE_EXPLOSION_SOUTHEAST;
  jtp_cmaptiles[S_litcorr] = JTP_TILE_FLOOR_ROUGH_LIT;

  jtp_tile_conversion_initialized = 1;
    
/*    
#define S_stone		0
#define S_vwall		1
#define S_hwall		2
#define S_tlcorn	3
#define S_trcorn	4
#define S_blcorn	5
#define S_brcorn	6
#define S_crwall	7
#define S_tuwall	8
#define S_tdwall	9
#define S_tlwall	10
#define S_trwall	11
#define S_ndoor		12
#define S_vodoor	13
#define S_hodoor	14
#define S_vcdoor	15	// closed door, vertical wall
#define S_hcdoor	16	// closed door, horizontal wall
#define S_bars		17	// KMH -- iron bars 
#define S_tree		18	// KMH 
#define S_room		19
#define S_corr		20
#define S_litcorr	21
#define S_upstair	22
#define S_dnstair	23
#define S_upladder	24
#define S_dnladder	25
#define S_altar		26
#define S_grave		27
#define S_throne	28
#define S_sink		29
#define S_fountain	30
#define S_pool		31
#define S_ice		32
#define S_lava		33
#define S_vodbridge	34
#define S_hodbridge	35
#define S_vcdbridge	36	// closed drawbridge, vertical wall
#define S_hcdbridge	37	// closed drawbridge, horizontal wall
#define S_air		38
#define S_cloud		39
#define S_water		40
*/
}


jtp_tilestats * jtp_get_tile
(
  int x1, int y1,
  int x2, int y2
)
{
  jtp_tilestats * temp1;
  int i, j, tx1, ty1, tx2, ty2;
  unsigned char * tempg;
 
  temp1 =(jtp_tilestats *)malloc(sizeof(jtp_tilestats));
  if (!temp1)
  {
    jtp_write_log_message("[jtp_win.c/jtp_get_tile/Check1] Out of memory!\n");
    jtp_exit_graphics(); exit(1);
  }
 
  for (i = y1; i <= y2; i++)
    for (j = x1; j <= x2; j++)
      if (jtp_screen.vpage[i*jtp_screen.width+j])
      {
        ty1 = i; i = y2+1; j = x2+1;
      }
 
  for (i = y2; i >= y1; i--)
    for (j = x1; j <= x2; j++)
      if (jtp_screen.vpage[i*jtp_screen.width+j])
      {
        ty2 = i; i = y1-1; j = x2+1;
      }
 
  for (j = x1; j <= x2; j++)
    for (i = ty1; i <= ty2; i++)   
      if (jtp_screen.vpage[i*jtp_screen.width+j])
      {
        tx1 = j; i = ty2+1; j = x2+1;
      }

  for (j = x2; j >= x1; j--)
    for (i = ty1; i <= ty2; i++)   
      if (jtp_screen.vpage[i*jtp_screen.width+j])
      {
        tx2 = j; i = ty2+1; j = x1-1;
      }

  temp1->xmod=0;
  for (i=x1;i<=x2;i++)
    if (jtp_screen.vpage[(y1-1)*jtp_screen.width+i]==16)
      temp1->xmod=tx1-i;
 
  temp1->ymod=0;
  for (i=y1;i<=y2;i++)
    if (jtp_screen.vpage[i*jtp_screen.width+x1-1]==16)
      temp1->ymod=ty1-i;

  temp1->graphic = jtp_get_img(tx1,ty1,tx2,ty2);

  return(temp1);
}

void jtp_put_tile
(
  int x, int y,
  int shade,
  unsigned char *a
)
{
  int srcXsize,srcYsize,j,yalku,yloppu,xalku,xloppu;
  int dplus;
  unsigned char vari;
  unsigned char *destin, *a_end, *shades;

  if ((!a) || (x>jtp_screen.drx2) || (y>jtp_screen.dry2)) return;

  srcYsize=a[0]*256+a[1];
  srcXsize=a[2]*256+a[3];

  if ((x+srcXsize<=jtp_screen.drx1) || (y+srcYsize<=jtp_screen.dry1)) return;
  if (y<jtp_screen.dry1) yalku=jtp_screen.dry1-y; else yalku=0;
  if (y+srcYsize-1>jtp_screen.dry2) yloppu=jtp_screen.dry2-y; else yloppu=srcYsize-1;
  if (x<jtp_screen.drx1) xalku=jtp_screen.drx1-x; else xalku=0; 
  if (x+srcXsize-1>jtp_screen.drx2) xloppu=jtp_screen.drx2-x; else xloppu=srcXsize-1;

  a+=yalku*srcXsize+4;
  a_end = a + (yloppu-yalku+1)*srcXsize;
  destin=jtp_screen.vpage+(yalku+y)*jtp_screen.width+x;
  dplus = jtp_screen.width;

  a += xalku;
  a_end += xalku;
  destin += xalku;
  xloppu -= xalku;
  xalku = jtp_screen.width;
  shades = jtp_shade + 256*shade;

  while (a < a_end)
  {
     for (j = xloppu; j >= 0; j--)
     {
/*
       vari=a[j];
       if (vari!=0) destin[j]=vari;
*/
       if (a[j]) destin[j] = shades[a[j]];
     }
     a += srcXsize;  
     destin += dplus;
  }
}


unsigned char *jtp_init_shades
(
  char *fname
)
{
 int i, j;
 FILE * f;
 unsigned char *temp1;

 f = fopen(fname, "rb");
 if (!f) return(NULL);
 
 temp1 = (unsigned char *)malloc(JTP_MAX_SHADES*256*sizeof(unsigned char));
 if (!temp1) 
 {
   jtp_write_log_message("[jtp_win.c/jtp_init_shades/Check1] Out of memory!\n");
   jtp_exit_graphics(); exit(1);
 }  
 
 fread(temp1,1,64*256,f); 
 fclose(f);
 
 /* 
  * Make sure that the background color is not used as a shade.
  * Replace any occurences with color 47 (which is currently black)
  */
 for (i = 1; i < JTP_MAX_SHADES; i++)
   for (j = 0; j < 256; j++)
     if (temp1[i*256+j] == 0)
       temp1[i*256+j] = 47;
 
 return(temp1);
}

void jtp_init_lights(int how_many)
{
  int i;

  jtp_nlights = how_many;
  jtp_ambient = 20;

  for (i = 0; i < jtp_nlights; i++)
  {
    jtp_lights[i*3] = rand()%(JTP_MAP_WIDTH - 1) + 1;  /* X location */
    jtp_lights[i*3+1] = rand()%JTP_MAP_HEIGHT;         /* Y location */
    jtp_lights[i*3+2] = rand()%20;                     /* radius*10 */
  }
}

void jtp_init_floor_decors
(
  int how_many
)
{
  int i, j, k, l, m, n;
  int pos_found;
  int cur_style, x, y;
  int **temp_mask;
  
  /* Initialize floor decorations */
  jtp_n_floor_decors = 0;  
  if (jtp_floor_decors) free(jtp_floor_decors);
  jtp_floor_decors = NULL;

  /*
    Create a temporary conversion of the actual dungeon level. 
    This is to ensure that decorations are in rooms (eg. not partly inside walls).
    I do this by making a mask with undesirable locations zeroed out.
  */
  temp_mask = (int **)malloc(JTP_MAP_HEIGHT*sizeof(int *));
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
  {
    temp_mask[i] = (int *)malloc(JTP_MAP_WIDTH*sizeof(int));
    for (j = 1; j < JTP_MAP_WIDTH; j++)
    {
      k = back_to_glyph(j, i) - GLYPH_CMAP_OFF;
      switch(k)
      {
        case S_stone: case S_vwall: case S_hwall:
        case S_tlcorn: case S_trcorn: case S_blcorn: case S_brcorn:
        case S_crwall: case S_tuwall: case S_tdwall: case S_tlwall: case S_trwall:
          temp_mask[i][j] = 0;
          break;
        default:
          /* 
            Technically, we should also worry about moats etc., but this will do
            for a start.
          */          
          temp_mask[i][j] = 1;
          break;
      }      
    }
  }
  
  /* This rather messy piece of code places the decorations onto the map */
  for (i = 0; i < how_many; i++)
  {
    switch(rand()%3)
    {
      case 0: cur_style = JTP_FLOOR_STYLE_CARPET; break;
      case 1: cur_style = JTP_FLOOR_STYLE_MURAL; break;
      case 2: cur_style = JTP_FLOOR_STYLE_MURAL_2; break;
      default: cur_style = JTP_FLOOR_STYLE_CARPET; break;
    }
    
    j = jtp_n_floor_decors;
    j += jtp_floors[cur_style].xspan*jtp_floors[cur_style].yspan;
    jtp_floor_decors = (jtp_floor_decor *)realloc(jtp_floor_decors, j*sizeof(jtp_floor_decor));
    
    /* Find location for decoration */
    for (k = 0; k <= JTP_MAP_HEIGHT-jtp_floors[cur_style].yspan; k++)
      for (l = 1; l <= JTP_MAP_WIDTH-jtp_floors[cur_style].xspan; l++)
      {
        pos_found = 1;
        for (m = jtp_floors[cur_style].yspan-1; m >= 0; m--)
          for (n = jtp_floors[cur_style].xspan-1; n >= 0; n--)
          {
            if (!temp_mask[k+m][l+n])
            {
              pos_found = 0; m = -1; n = -1;
            }
          }
        if (pos_found)
        {
          y = k; x = l;
          k = JTP_MAP_HEIGHT-jtp_floors[cur_style].yspan+1;
          l = JTP_MAP_WIDTH-jtp_floors[cur_style].xspan+1;
        }
      }
    if (!pos_found)
    {
      x = rand()%(JTP_MAP_WIDTH-jtp_floors[cur_style].xspan) + 1;
      y = rand()%(JTP_MAP_HEIGHT-jtp_floors[cur_style].yspan+1);
    }
    
    /* Remove chosen area from mask */
    for (k = 0; k < jtp_floors[cur_style].yspan; k++)
      for (l = 0; l < jtp_floors[cur_style].xspan; l++)      
        temp_mask[y+k][x+l] = 0;

    /* Place floor decor on the map */      
    for (k = 0; k < jtp_floors[cur_style].yspan; k++)
      for (l = 0; l < jtp_floors[cur_style].xspan; l++)
      {
        m = k*jtp_floors[cur_style].xspan + l;
        jtp_floor_decors[jtp_n_floor_decors+m].x = x + l;
        jtp_floor_decors[jtp_n_floor_decors+m].y = y + k;
        jtp_floor_decors[jtp_n_floor_decors+m].style = cur_style;
        jtp_floor_decors[jtp_n_floor_decors+m].pos = m;
      }
    jtp_n_floor_decors = j;
  }
  
  /* Clean up */
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
    free(temp_mask[i]);
  free(temp_mask);  
}

void jtp_get_floor_style
(
  int style_index,
  int xspan, int yspan,
  int x1, int y1
)
{
  int i, j;

  jtp_floors[style_index].xspan = xspan;
  jtp_floors[style_index].yspan = yspan;
  jtp_floors[style_index].pattern = (jtp_tilestats **)malloc(xspan*yspan*sizeof(jtp_tilestats *));

#define FS_DX 116
#define FS_DY 53
#define FS_EX 112
#define FS_EY 49
  for (i = 0; i < yspan; i++)
    for (j = 0; j < xspan; j++)
      jtp_floors[style_index].pattern[i*xspan+j] = jtp_get_tile(x1 + FS_DX*j, y1 + FS_DY*i, x1 + FS_DX*j + FS_EX, y1 + FS_DY*i + FS_EY);
}

void jtp_get_floor_edge_style
(
  int style_index,
  int x1, int y1
)
{
#define FE_DX 116
#define FE_DY 53
#define FE_EX 112
#define FE_EY 49
  jtp_floor_edges[style_index].west = jtp_get_tile(x1, y1, x1 + FE_EX, y1 + FE_EY);
  jtp_floor_edges[style_index].north = jtp_get_tile(x1 + FE_DX, y1, x1 + FE_EX + FE_DX, y1 + FE_EY);
  jtp_floor_edges[style_index].south = jtp_get_tile(x1, y1 + FE_DY, x1 + FE_EX, y1 + FE_EY + FE_DY);
  jtp_floor_edges[style_index].east = jtp_get_tile(x1 + FE_DX, y1 + FE_DY, x1 + FE_EX + FE_DX, y1 + FE_EY + FE_DY);
  jtp_floor_edges[style_index].southwest = jtp_get_tile(x1 + 2*FE_DX, y1, x1 + FE_EX + 2*FE_DX, y1 + FE_EY);
  jtp_floor_edges[style_index].northwest = jtp_get_tile(x1 + 3*FE_DX, y1, x1 + FE_EX + 3*FE_DX, y1 + FE_EY);
  jtp_floor_edges[style_index].southeast = jtp_get_tile(x1 + 2*FE_DX, y1 + FE_DY, x1 + FE_EX + 2*FE_DX, y1 + FE_EY + FE_DY);
  jtp_floor_edges[style_index].northeast = jtp_get_tile(x1 + 3*FE_DX, y1 + FE_DY, x1 + FE_EX + 3*FE_DX, y1 + FE_EY + FE_DY);
  jtp_floor_edges[style_index].southwest_bank = jtp_get_tile(x1 + 4*FE_DX, y1, x1 + FE_EX + 4*FE_DX, y1 + FE_EY);
  jtp_floor_edges[style_index].northwest_bank = jtp_get_tile(x1 + 5*FE_DX, y1, x1 + FE_EX + 5*FE_DX, y1 + FE_EY);
  jtp_floor_edges[style_index].southeast_bank = jtp_get_tile(x1 + 4*FE_DX, y1 + FE_DY, x1 + FE_EX + 4*FE_DX, y1 + FE_EY + FE_DY);
  jtp_floor_edges[style_index].northeast_bank = jtp_get_tile(x1 + 5*FE_DX, y1 + FE_DY, x1 + FE_EX + 5*FE_DX, y1 + FE_EY + FE_DY);
}

void jtp_get_wall_style
(
  int style_index,
  int x1, int y1
)
{
  int i, j;

#define WS_DX 60
#define WS_EX 56
#define WS_EY 122
  jtp_walls[style_index].west = jtp_get_tile(x1, y1, x1 + WS_EX, y1 + WS_EY);
  jtp_walls[style_index].north = jtp_get_tile(x1 + WS_DX, y1, x1 + WS_DX + WS_EX, y1 + WS_EY);
  jtp_walls[style_index].south = jtp_get_tile(x1 + 2*WS_DX, y1, x1 + 2*WS_DX + WS_EX, y1 + WS_EY);
  jtp_walls[style_index].east = jtp_get_tile(x1 + 3*WS_DX, y1, x1 + 3*WS_DX + WS_EX, y1 + WS_EY);
}


void jtp_get_tile_group
(
  int group_rows, 
  int group_cols,
  int x,
  int y,
  int tile_dx,
  int tile_dy,
  int ** indices
)
{
  int i, j;

  for (i = 0; i < group_rows; i++)
    for (j = 0; j < group_cols; j++)
      {
        if (indices[i][j] >= 0)
	  {
            jtp_tiles[indices[i][j]] =
	      jtp_get_tile(x + j*tile_dx, y + i*tile_dy, 
                           x + (j+1)*tile_dx - 4, y + (i+1)*tile_dy - 4);
	  }
      }
}


void jtp_init_tilegraphics(int wall_display_style)
{
  int i, j, k, l;
  int **tileloc;

  jtp_shade = jtp_init_shades(jtp_filenames[JTP_FILE_SHADING_TABLE]);
  printf("."); fflush(stdout);
  
  tileloc = (int **)malloc(20*sizeof(int *));
  for (i = 0; i < 20; i++) 
    tileloc[i] = (int *)malloc(20*sizeof(int));

  jtp_tiles = (jtp_tilestats **)calloc(JTP_MAX_TILES, sizeof(jtp_tilestats *));
  jtp_walls = (jtp_wall_style *)calloc(JTP_MAX_WALL_STYLES, sizeof(jtp_wall_style));
  jtp_floors = (jtp_floor_style *)calloc(JTP_MAX_FLOOR_STYLES, sizeof(jtp_floor_style));
  jtp_floor_edges = (jtp_floor_edge_style *)calloc(JTP_MAX_FLOOR_EDGE_STYLES, sizeof(jtp_floor_edge_style));

  /* Load wall tiles */
  if (wall_display_style == JTP_WALL_DISPLAY_STYLE_FULL)
    jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_WALL_TILES], 1);
  else if (wall_display_style == JTP_WALL_DISPLAY_STYLE_HALF_HEIGHT)
    jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_HALF_HEIGHT_WALL_TILES], 1);
  else if (wall_display_style == JTP_WALL_DISPLAY_STYLE_TRANSPARENT)
    jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_TRANSPARENT_WALL_TILES], 1);
  else
    jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_WALL_TILES], 1);

  jtp_get_wall_style(JTP_WALL_STYLE_BRICK, 1, 1);
  jtp_get_wall_style(JTP_WALL_STYLE_BRICK_BANNER, 241, 1);
  jtp_get_wall_style(JTP_WALL_STYLE_BRICK_PAINTING, 1, 127);
  jtp_get_wall_style(JTP_WALL_STYLE_BRICK_POCKET, 481, 1);
  jtp_get_wall_style(JTP_WALL_STYLE_BRICK_PILLAR, 241, 127);
  jtp_get_wall_style(JTP_WALL_STYLE_MARBLE, 1, 253);

  /* Load more wall tiles */
  if (wall_display_style == JTP_WALL_DISPLAY_STYLE_FULL)
    jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_WALL_TILES_2], 1);
  else if (wall_display_style == JTP_WALL_DISPLAY_STYLE_HALF_HEIGHT)
    jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_HALF_HEIGHT_WALL_TILES_2], 1);
  else if (wall_display_style == JTP_WALL_DISPLAY_STYLE_TRANSPARENT)
    jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_TRANSPARENT_WALL_TILES_2], 1);
  else
    jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_WALL_TILES_2], 1);

  jtp_get_wall_style(JTP_WALL_STYLE_VINE_COVERED, 1, 1);
  jtp_get_wall_style(JTP_WALL_STYLE_STUCCO, 1, 127);
  jtp_get_wall_style(JTP_WALL_STYLE_ROUGH, 1, 253);
  printf("."); fflush(stdout);

  /* Load floor pattern tiles */
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_FLOOR_TILES], 1);
  /* Cobblestone floor tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_COBBLESTONE, 3, 3, 1, 1);
  /* Rough floor tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_ROUGH, 3, 3, 1, 160);
  /* Ceramic floor tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_CERAMIC, 3, 3, 1, 319);
  /* Lava floor tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_LAVA, 3, 3, 349, 1);
  /* Water floor tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_WATER, 3, 3, 349, 160);
  /* Ice floor tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_ICE, 3, 3, 349, 319);

  /* Cobblestone floor edges (12 orientations) */
  jtp_get_floor_edge_style(JTP_FLOOR_EDGE_STYLE_COBBLESTONE, 1, 478);
  printf("."); fflush(stdout);

  /* Load more floor pattern tiles */
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_FLOOR_TILES_2], 1);
  /* Mural tiles (3x2 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_MURAL, 3, 2, 1, 1);
  /* Mural 2 tiles (3x2 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_MURAL_2, 3, 2, 349, 1);
  /* Carpet tiles (3x2 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_CARPET, 3, 2, 1, 107);
  /* Moss-covered floor tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_MOSS_COVERED, 3, 3, 1, 213);
  /* Marble floor tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_MARBLE, 3, 3, 349, 213);
  printf("."); fflush(stdout);
  /* Lit rough floor tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_ROUGH_LIT, 3, 3, 1, 372);
  /* Air tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_AIR, 3, 3, 349, 372);

#define JTP_CMAP_TILE_DX 116
#define JTP_CMAP_TILE_DY 126

  /* Load miscellaneous cmap tiles */

  tileloc[0][0] = JTP_TILE_DOOR_WOOD_BROKEN; tileloc[0][1] = JTP_TILE_HDOOR_WOOD_CLOSED;
  tileloc[0][2] = JTP_TILE_VDOOR_WOOD_CLOSED; tileloc[0][3] = JTP_TILE_VDOOR_WOOD_OPEN;
  tileloc[0][4] = JTP_TILE_HDOOR_WOOD_OPEN; tileloc[0][5] = JTP_TILE_TRAP_BEAR;
  tileloc[1][0] = JTP_TILE_GRAVE; tileloc[1][1] = JTP_TILE_ALTAR;
  tileloc[1][2] = JTP_TILE_FOUNTAIN; tileloc[1][3] = JTP_TILE_STAIRS_UP;
  tileloc[1][4] = JTP_TILE_STAIRS_DOWN; tileloc[1][5] = JTP_TILE_SINK;
  tileloc[2][0] = JTP_TILE_CLOUD; tileloc[2][1] = JTP_TILE_TRAP_PIT;
  tileloc[2][2] = JTP_TILE_TRAP_TELEPORTER; tileloc[2][3] = JTP_TILE_TREE;
  tileloc[2][4] = JTP_TILE_TRAP_MAGIC; tileloc[2][5] = -1;
  tileloc[3][0] = JTP_TILE_TRAP_DOOR; tileloc[3][1] = JTP_TILE_TRAP_WATER;
  tileloc[3][2] = JTP_TILE_TRAP_TELEPORTER; tileloc[3][3] = JTP_TILE_FLOOR_NOT_VISIBLE;
  tileloc[3][4] = -1; tileloc[3][5] = -1;

  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_TILES], 1);  
  jtp_get_tile_group(4, 6, 1, 1, JTP_CMAP_TILE_DX, JTP_CMAP_TILE_DY, tileloc);
  printf("."); fflush(stdout);

  /* Load more miscellaneous cmap tiles */

  tileloc[0][0] = JTP_TILE_BARS; tileloc[0][1] = JTP_TILE_THRONE;
  tileloc[0][2] = -1; tileloc[0][3] = JTP_TILE_EXPLOSION_NORTHWEST;
  tileloc[0][4] = JTP_TILE_EXPLOSION_NORTH; tileloc[0][5] = JTP_TILE_EXPLOSION_NORTHEAST;
  tileloc[1][0] = JTP_TILE_TRAP_ANTI_MAGIC; tileloc[1][1] = JTP_TILE_TRAP_ARROW;
  tileloc[1][2] = -1; tileloc[1][3] = JTP_TILE_EXPLOSION_WEST;
  tileloc[1][4] = JTP_TILE_EXPLOSION_CENTER; tileloc[1][5] = JTP_TILE_EXPLOSION_EAST;
  tileloc[2][0] = JTP_TILE_TRAP_FIRE; tileloc[2][1] = JTP_TILE_TRAP_FALLING_ROCK;
  tileloc[2][2] = JTP_TILE_TRAP_SLEEPGAS; tileloc[2][3] = JTP_TILE_EXPLOSION_SOUTHWEST;
  tileloc[2][4] = JTP_TILE_EXPLOSION_SOUTH; tileloc[2][5] = JTP_TILE_EXPLOSION_SOUTHEAST;
  tileloc[3][0] = JTP_TILE_ZAP_SLANT_RIGHT; tileloc[3][1] = JTP_TILE_ZAP_SLANT_LEFT;
  tileloc[3][2] = JTP_TILE_ZAP_HORIZONTAL; tileloc[3][3] = JTP_TILE_ZAP_VERTICAL;
  tileloc[3][4] = JTP_TILE_LADDER_UP; tileloc[3][5] = JTP_TILE_LADDER_DOWN;

  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_CMAP_TILES_2], 1);
  jtp_get_tile_group(4, 6, 1, 1, JTP_CMAP_TILE_DX, JTP_CMAP_TILE_DY, tileloc);
  printf("."); fflush(stdout);

#define JTP_OBJ_TILE_DX 96
#define JTP_OBJ_TILE_DY 102

  /* Load object tiles */

  tileloc[0][0] = JTP_TILE_BAG; tileloc[0][1] = JTP_TILE_BOULDER;
  tileloc[0][2] = JTP_TILE_BONES; tileloc[0][3] = -1;
  tileloc[0][4] = -1; tileloc[0][5] = -1;
  tileloc[0][6] = JTP_TILE_BOW; tileloc[0][7] = JTP_TILE_KEY;
  tileloc[1][0] = JTP_TILE_STATUE; tileloc[1][1] = JTP_TILE_CHEST;
  tileloc[1][2] = JTP_TILE_COINS; tileloc[1][3] = JTP_TILE_BOOK;
  tileloc[1][4] = JTP_TILE_HELMET; tileloc[1][5] = JTP_TILE_SHIELD;
  tileloc[1][6] = JTP_TILE_AMULET; tileloc[1][7] = JTP_TILE_DAGGER;
  tileloc[2][0] = JTP_TILE_BOOTS; tileloc[2][1] = JTP_TILE_SPEAR;
  tileloc[2][2] = JTP_TILE_BOTTLE; tileloc[2][3] = JTP_TILE_SCROLL;
  tileloc[2][4] = JTP_TILE_WAND; tileloc[2][5] = JTP_TILE_SWORD;
  tileloc[2][6] = JTP_TILE_RING; tileloc[2][7] = JTP_TILE_APPLE;
  tileloc[3][0] = JTP_TILE_GEM_BLUE; tileloc[3][1] = JTP_TILE_RING_MAIL;
  tileloc[3][2] = JTP_TILE_LEATHER_ARMOR; tileloc[3][3] = JTP_TILE_PLATE_MAIL;
  tileloc[3][4] = JTP_TILE_HAMMER; tileloc[3][5] = JTP_TILE_AXE;
  tileloc[3][6] = JTP_TILE_LANTERN; tileloc[3][7] = JTP_TILE_PEAR;
  tileloc[4][0] = JTP_TILE_SCALE_MAIL; tileloc[4][1] = JTP_TILE_CHAIN_MAIL;
  tileloc[4][2] = JTP_TILE_CLOAK; tileloc[4][3] = JTP_TILE_TRIDENT;
  tileloc[4][4] = JTP_TILE_CAMERA; tileloc[4][5] = JTP_TILE_FEDORA;
  tileloc[4][6] = JTP_TILE_CLUB; tileloc[4][7] = JTP_TILE_ARROW;

  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_OBJ_TILES_1], 1);
  jtp_get_tile_group(5, 8, 1, 1, JTP_OBJ_TILE_DX, JTP_OBJ_TILE_DY, tileloc);
  printf("."); fflush(stdout);

  /* Load more object tiles */

  tileloc[0][0] = JTP_TILE_EGG; tileloc[0][1] = JTP_TILE_GLOVES;
  tileloc[0][2] = JTP_TILE_BELL; tileloc[0][3] = JTP_TILE_GEM_RED;
  tileloc[0][4] = JTP_TILE_GEM_GREEN; tileloc[0][5] = JTP_TILE_CANDLE;
  tileloc[1][0] = JTP_TILE_GEM_YELLOW; tileloc[1][1] = JTP_TILE_GEM_WHITE;
  tileloc[1][2] = JTP_TILE_GEM_BLACK; tileloc[1][3] = JTP_TILE_WHIP;
  tileloc[1][4] = JTP_TILE_MACE; tileloc[1][5] = JTP_TILE_GRAND_HORN;
  tileloc[2][0] = JTP_TILE_CRYSTAL_BALL; tileloc[2][1] = JTP_TILE_HORN;
  tileloc[2][2] = JTP_TILE_UNICORN_HORN; tileloc[2][3] = JTP_TILE_HAWAIIAN_SHIRT;
  tileloc[2][4] = JTP_TILE_CREDIT_CARD; tileloc[2][5] = JTP_TILE_MIRROR;
  tileloc[3][0] = JTP_TILE_CROSSBOW; tileloc[3][1] = JTP_TILE_CONICAL_HAT;
  tileloc[3][2] = JTP_TILE_MAGIC_MARKER; tileloc[3][3] = JTP_TILE_STAFF;
  tileloc[3][4] = JTP_TILE_FOOD_RATION; tileloc[3][5] = JTP_TILE_COOKIE;
  tileloc[4][0] = JTP_TILE_TRIPE_RATION; tileloc[4][1] = JTP_TILE_MEAT_CHUNK;
  tileloc[4][2] = JTP_TILE_SHURIKEN; tileloc[4][3] = JTP_TILE_ROCK;
  tileloc[4][4] = JTP_TILE_PICKAXE; tileloc[4][5] = JTP_TILE_TIN;

  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_OBJ_TILES_2], 1);
  jtp_get_tile_group(5, 6, 1, 1, JTP_OBJ_TILE_DX, JTP_OBJ_TILE_DY, tileloc);
  printf("."); fflush(stdout);

#define JTP_MON_TILE_DX 116
#define JTP_MON_TILE_DY 126

  /* Load monster tiles */

  tileloc[0][0] = JTP_TILE_KNIGHT; tileloc[0][1] = JTP_TILE_GOBLIN;
  tileloc[0][2] = JTP_TILE_GNOME; tileloc[0][3] = JTP_TILE_ELEMENTAL;
  tileloc[0][4] = JTP_TILE_JELLY; tileloc[0][5] = JTP_TILE_SKELETON;
  tileloc[1][0] = JTP_TILE_EYE; tileloc[1][1] = JTP_TILE_SPIDER;
  tileloc[1][2] = JTP_TILE_CAT; tileloc[1][3] = JTP_TILE_LIZARD;
  tileloc[1][4] = JTP_TILE_DOG; tileloc[1][5] = JTP_TILE_TROLL;
  tileloc[2][0] = JTP_TILE_OGRE; tileloc[2][1] = JTP_TILE_NYMPH;
  tileloc[2][2] = JTP_TILE_WIZARD; tileloc[2][3] = JTP_TILE_VALKYRIE;
  tileloc[2][4] = JTP_TILE_RANGER; tileloc[2][5] = JTP_TILE_WATER_NYMPH;
  tileloc[3][0] = JTP_TILE_GHOST; tileloc[3][1] = JTP_TILE_WRAITH;
  tileloc[3][2] = JTP_TILE_GREEN_DRAGON; tileloc[3][3] = JTP_TILE_ZOMBIE;
  tileloc[3][4] = JTP_TILE_STONE_GOLEM; tileloc[3][5] = JTP_TILE_RAT;

  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_MON_TILES_1], 1);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);
  printf("."); fflush(stdout);

  /* Load more monster tiles */

  tileloc[0][0] = JTP_TILE_ARCHEOLOGIST; tileloc[0][1] = JTP_TILE_TOURIST;
  tileloc[0][2] = JTP_TILE_ROGUE; tileloc[0][3] = JTP_TILE_PRIEST;
  tileloc[1][0] = JTP_TILE_BAT; tileloc[1][1] = JTP_TILE_HORSE;
  tileloc[1][2] = JTP_TILE_ANT; tileloc[1][3] = JTP_TILE_BEE;

  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_MON_TILES_2], 1);
  jtp_get_tile_group(2, 4, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);
  printf("."); fflush(stdout);

  /* Initialize map */

  jtp_map_width = JTP_MAP_WIDTH;
  jtp_map_height = JTP_MAP_HEIGHT;

  jtp_mapglyph_cmap = (int **)malloc(JTP_MAP_HEIGHT*sizeof(int *));
  jtp_mapglyph_obj = (int **)malloc(JTP_MAP_HEIGHT*sizeof(int *));
  jtp_mapglyph_mon = (int **)malloc(JTP_MAP_HEIGHT*sizeof(int *));

  jtp_maptile_cmap = (jtp_tilestats ***)malloc(JTP_MAP_HEIGHT*sizeof(jtp_tilestats **));
  jtp_maptile_obj = (jtp_tilestats ***)malloc(JTP_MAP_HEIGHT*sizeof(jtp_tilestats **));
  jtp_maptile_mon = (jtp_tilestats ***)malloc(JTP_MAP_HEIGHT*sizeof(jtp_tilestats **));
  jtp_maptile_wall = (jtp_wall_style **)malloc(JTP_MAP_HEIGHT*sizeof(jtp_wall_style *));
  jtp_maptile_floor_edge = (jtp_floor_edge_style **)malloc(JTP_MAP_HEIGHT*sizeof(jtp_floor_edge_style *));

  jtp_map_light = (int **)malloc(JTP_MAP_HEIGHT*sizeof(int *));
  jtp_map_light_distances = (double **)malloc(JTP_MAP_HEIGHT*sizeof(double *));
  jtp_map_temp_distances = (double **)malloc(JTP_MAP_HEIGHT*sizeof(double *));
  jtp_map_accessibles = (char **)malloc(JTP_MAP_HEIGHT*sizeof(char *));
  jtp_room_indices = (int **)malloc(JTP_MAP_HEIGHT*sizeof(int *));

  if ((!jtp_mapglyph_cmap) || (!jtp_mapglyph_obj) || (!jtp_mapglyph_mon) ||
      (!jtp_maptile_cmap) || (!jtp_maptile_obj) || (!jtp_maptile_mon) ||
      (!jtp_maptile_wall) || (!jtp_maptile_floor_edge) ||
      (!jtp_map_light) || (!jtp_map_light_distances) ||
      (!jtp_map_temp_distances) || (!jtp_map_accessibles) ||
      (!jtp_room_indices))
  {
    jtp_write_log_message("[jtp_win.c/jtp_init_tilegraphics/Check1] Out of memory!\n");
    jtp_exit_graphics(); exit(1);
  }
  
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
  {
    jtp_mapglyph_cmap[i] = (int *)malloc(JTP_MAP_WIDTH*sizeof(int));
    jtp_mapglyph_obj[i] = (int *)malloc(JTP_MAP_WIDTH*sizeof(int));
    jtp_mapglyph_mon[i] = (int *)malloc(JTP_MAP_WIDTH*sizeof(int));

    jtp_maptile_cmap[i] = (jtp_tilestats **)malloc(JTP_MAP_WIDTH*sizeof(jtp_tilestats *));
    jtp_maptile_obj[i] = (jtp_tilestats **)malloc(JTP_MAP_WIDTH*sizeof(jtp_tilestats *));
    jtp_maptile_mon[i] = (jtp_tilestats **)malloc(JTP_MAP_WIDTH*sizeof(jtp_tilestats *));
    jtp_maptile_wall[i] = (jtp_wall_style *)malloc(JTP_MAP_WIDTH*sizeof(jtp_wall_style));
    jtp_maptile_floor_edge[i] = (jtp_floor_edge_style *)malloc(JTP_MAP_WIDTH*sizeof(jtp_floor_edge_style));

    jtp_map_light[i] = (int *)malloc(JTP_MAP_WIDTH*sizeof(int));
    jtp_map_light_distances[i] = (double *)malloc(JTP_MAP_WIDTH*sizeof(double));
    jtp_map_temp_distances[i] = (double *)malloc(JTP_MAP_WIDTH*sizeof(double));
    jtp_map_accessibles[i] = (char *)malloc(JTP_MAP_WIDTH*sizeof(char));
    jtp_room_indices[i] = (int *)malloc(JTP_MAP_WIDTH*sizeof(int));

    if ((!jtp_mapglyph_cmap[i]) || (!jtp_mapglyph_obj[i]) || (!jtp_mapglyph_mon[i]) ||
        (!jtp_maptile_cmap[i]) || (!jtp_maptile_obj[i]) || (!jtp_maptile_mon[i]) ||
        (!jtp_maptile_wall[i]) || (!jtp_maptile_floor_edge[i]) ||
        (!jtp_map_light[i]) || (!jtp_map_light_distances[i]) ||
        (!jtp_map_temp_distances[i]) || (!jtp_map_accessibles) ||
        (!jtp_room_indices[i]))
    {
      jtp_write_log_message("[jtp_win.c/jtp_init_tilegraphics/Check2] Out of memory!\n");
      jtp_exit_graphics(); exit(1);
    }
    
    for (j = 0; j < JTP_MAP_WIDTH; j++)
    {
      jtp_mapglyph_cmap[i][j] = JTP_WINCONTENT_GLYPH_UNEXPLORED;
      jtp_mapglyph_obj[i][j] = JTP_WINCONTENT_GLYPH_UNEXPLORED;
      jtp_mapglyph_mon[i][j] = JTP_WINCONTENT_GLYPH_UNEXPLORED;
      jtp_map_light[i][j] = 0;
    }
  }
  printf("."); fflush(stdout);

  /* Initialize random number generator */
  srand(time(NULL));

  /* Initialize light sources */
  jtp_nlights = 1;  /* Hero carries a small light */

  jtp_n_floor_decors = 0;
  jtp_floor_decors = NULL;
  jtp_cur_dlevel = -1;
  jtp_prev_dlevel = -1;
  
  jtp_move_length = 0;
  jtp_is_backpack_shortcut_active = 0;

  jtp_you_x = -1;
  jtp_you_y = -1;
  jtp_old_you_x = -1;
  jtp_old_you_y = -1;
  jtp_map_x = JTP_MAP_WIDTH/2;
  jtp_map_y = JTP_MAP_HEIGHT/2;
  jtp_map_changed = 1;

  /* Clean up */
  for (i = 0; i < 20; i++)
    free(tileloc[i]);
  free(tileloc);
}

int jtp_cmap_to_map_symbol
(
  int cur_glyph
)
{
  int cur_symbol = -1;

  if (glyph_is_trap(cur_glyph + GLYPH_CMAP_OFF)) /* trap */
    cur_symbol = JTP_MAP_SYMBOL_TRAP;
  else switch (cur_glyph)
  {
    case JTP_WINCONTENT_GLYPH_UNEXPLORED:
    case JTP_WINCONTENT_GLYPH_NOT_VISIBLE:
    case S_stone:
      cur_symbol = -1; break;
    case S_vwall: case S_hwall:
    case S_tlcorn: case S_trcorn: case S_blcorn: case S_brcorn:
    case S_crwall: case S_tuwall: case S_tdwall: case S_tlwall: case S_trwall:
      cur_symbol = JTP_MAP_SYMBOL_WALL; break;
    case S_room: case S_corr: case S_litcorr: 
      cur_symbol = JTP_MAP_SYMBOL_FLOOR; break;
    case S_upstair:
      cur_symbol = JTP_MAP_SYMBOL_UP; break;
    case S_dnstair:
      cur_symbol = JTP_MAP_SYMBOL_DOWN; break;
    case S_ndoor:
      cur_symbol = JTP_MAP_SYMBOL_FLOOR; break;
    case S_vodoor: case S_hodoor: case S_vcdoor: case S_hcdoor:
      cur_symbol = JTP_MAP_SYMBOL_DOOR; break;
    default:
      cur_symbol = JTP_MAP_SYMBOL_CMAP; break;
  }
  return(cur_symbol);
}

int jtp_object_to_map_symbol
(
  int cur_glyph
)
{
  if (cur_glyph == CORPSE)
  {
    /* Dead monster, currently not differentiated by monster type */
    return(JTP_MAP_SYMBOL_OBJECT);
  }
  else if ((cur_glyph >= 0) && (cur_glyph < NUM_OBJECTS))
    return(JTP_MAP_SYMBOL_OBJECT);  
  return(-1);
}

int jtp_monster_to_map_symbol
(
  int cur_glyph
)
{
  if ((cur_glyph >= 0) && (cur_glyph < NUMMONS))
    return(JTP_MAP_SYMBOL_MONSTER);
  return(-1);
}



int jtp_monster_to_tile
(
  int cur_glyph
)
{
  if ((cur_glyph >= 0) && (cur_glyph < NUMMONS))
    return(jtp_montiles[cur_glyph]);
  else return(JTP_TILE_INVALID);
}

int jtp_object_to_tile
(
  int cur_glyph
)
{
  if (cur_glyph == CORPSE)
  {
    /* Dead monster, currently not differentiated by monster type */
    return(JTP_TILE_BONES);
  }
  else if ((cur_glyph >= 0) && (cur_glyph < NUM_OBJECTS))
    return(jtp_objtiles[cur_glyph]);  
  return(JTP_TILE_INVALID);
}

int jtp_cmap_to_tile
(
  int cur_glyph
)
{
  if ((cur_glyph >= 0) && (cur_glyph < MAXPCHARS))
    return(jtp_cmaptiles[cur_glyph]);
  else if (cur_glyph == JTP_WINCONTENT_GLYPH_NOT_VISIBLE)
    return(JTP_TILE_FLOOR_NOT_VISIBLE);
  return(JTP_TILE_INVALID);
}


/*
 * Convert wall tile index (ie. wall type) to an associated decoration style.
 */
int jtp_get_wall_decor
(
  int walltile,
  int wally, int wallx,
  int floory, int floorx
)
{
  switch (walltile)
  {
    case JTP_TILE_WALL_ROUGH: 
      return(JTP_WALL_STYLE_ROUGH); 
      break;
    case JTP_TILE_WALL_BRICK:
      switch(jtp_room_indices[floory][floorx] % 4)
      {
        case 0: return(JTP_WALL_STYLE_STUCCO); break;
        case 1: return(JTP_WALL_STYLE_BRICK + ((wally*wallx+wally+wallx)%5)); break;
        case 2: return(JTP_WALL_STYLE_VINE_COVERED); break;
        case 3: return(JTP_WALL_STYLE_MARBLE); break;
        default: return(JTP_WALL_STYLE_BRICK); break;
      }
      break;
    default:
      return(JTP_WALL_STYLE_BRICK); 
      break;
  }
}

/*
 * Convert floor tile index (ie. floor type) to an associated decoration style.
 */
int jtp_get_floor_decor
(
  int floorstyle,
  int floory, int floorx
)
{
  switch (floorstyle)
  {
    case JTP_TILE_FLOOR_ROUGH: 
      return(JTP_FLOOR_STYLE_ROUGH); 
      break;
    case JTP_TILE_FLOOR_ROUGH_LIT:
      return(JTP_FLOOR_STYLE_ROUGH_LIT);
      break;
    case JTP_TILE_FLOOR_COBBLESTONE:
      switch(jtp_room_indices[floory][floorx] % 4)
      {
        case 0: return(JTP_FLOOR_STYLE_CERAMIC); break;
        case 1: return(JTP_FLOOR_STYLE_COBBLESTONE); break;
        case 2: return(JTP_FLOOR_STYLE_MOSS_COVERED); break;
        case 3: return(JTP_FLOOR_STYLE_MARBLE); break;
        default: return(JTP_FLOOR_STYLE_COBBLESTONE); break;
      }
      break;
    case JTP_TILE_FLOOR_WATER:
      return(JTP_FLOOR_STYLE_WATER);
      break;
    case JTP_TILE_FLOOR_ICE:
      return(JTP_FLOOR_STYLE_ICE);
      break;
    case JTP_TILE_FLOOR_AIR:
      return(JTP_FLOOR_STYLE_AIR);
      break;
    case JTP_TILE_FLOOR_LAVA:
      return(JTP_FLOOR_STYLE_LAVA);
      break;
    default:
      return(JTP_FLOOR_STYLE_COBBLESTONE); 
      break;
  }
}

void jtp_calculate_lights()
{
  int i, j, k;
  int temp1, temp2;
  double temp_lightlevel;
  double lightlevel;


  /* The hero carries a small light */
  jtp_lights[0] = jtp_you_x;
  jtp_lights[1] = jtp_you_y;
  jtp_lights[2] = 10;
  jtp_find_passable_squares(jtp_map_accessibles);
  jtp_find_distances(jtp_lights[1], jtp_lights[0], jtp_map_light_distances, jtp_map_accessibles);

  for (k = jtp_nlights-1; k >= 1; k--)
  {
    jtp_find_distances(jtp_lights[k*3+1], jtp_lights[k*3], jtp_map_temp_distances, jtp_map_accessibles);
    for (i = JTP_MAP_HEIGHT-1; i >= 0; i--)
      for (j = JTP_MAP_WIDTH-1; j >= 1; j--)
      {
        temp_lightlevel = jtp_map_temp_distances[i][j]*jtp_lights[k*3+2]/10;
        if (jtp_map_light_distances[i][j] > temp_lightlevel)
          jtp_map_light_distances[i][j] = temp_lightlevel;
      }
  }

  for (i = JTP_MAP_HEIGHT-1; i >= 0; i--)
    for (j = JTP_MAP_WIDTH-1; j >= 1; j--)
    {
      lightlevel = jtp_map_light_distances[i][j];
      /* 
       * Calculate illumination level from distance.
       * This should be converted to a table lookup.
       * The constants have been chosen empirically
       * (ie. what looked best). These rules apply:
       *
       *   distance is 0 --> lightlevel is JTP_MAX_SHADES-1
       * 
       */
      lightlevel = JTP_LIGHTING3 - JTP_LIGHTING1*log(lightlevel+JTP_LIGHTING2);
      if (lightlevel > JTP_MAX_SHADES-1) lightlevel = JTP_MAX_SHADES-1;
      else if (lightlevel < jtp_ambient) lightlevel = jtp_ambient;
      jtp_map_light[i][j] = lightlevel;
    }
}

void jtp_convert_map_objects()
{
  int i, j;
  int cur_glyph, cur_tile;
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
    for (j = 1; j < JTP_MAP_WIDTH; j++)
    {
      cur_glyph = jtp_mapglyph_obj[i][j];
      cur_tile = jtp_object_to_tile(cur_glyph);
      if (cur_tile != JTP_TILE_INVALID)
        jtp_maptile_obj[i][j] = jtp_tiles[cur_tile];
      else jtp_maptile_obj[i][j] = NULL;      
    }
}

void jtp_convert_map_monsters()
{
  int i, j;
  int cur_glyph, cur_tile;
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
    for (j = 1; j < JTP_MAP_WIDTH; j++)
    {
      cur_glyph = jtp_mapglyph_mon[i][j];
      cur_tile = jtp_monster_to_tile(cur_glyph);
      if (cur_tile != JTP_TILE_INVALID)
        jtp_maptile_mon[i][j] = jtp_tiles[cur_tile];
      else jtp_maptile_mon[i][j] = NULL;      
    }
}

void jtp_convert_map_cmaps()
{
  int i, j, k, l;
  int cur_glyph, cur_tile;
  int temp_glyph, temp_tile;
  int iswall, isfloor, isdecor;
  int isedge;
  
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
    for (j = 1; j < JTP_MAP_WIDTH; j++)
    {
      /* Default: no walls around tile */ 
      jtp_maptile_wall[i][j].west = NULL;
      jtp_maptile_wall[i][j].north = NULL;
      jtp_maptile_wall[i][j].east = NULL;
      jtp_maptile_wall[i][j].south = NULL;

      /* Default: no floor edges around tile */
      jtp_maptile_floor_edge[i][j].west = NULL;
      jtp_maptile_floor_edge[i][j].north = NULL;
      jtp_maptile_floor_edge[i][j].east = NULL;
      jtp_maptile_floor_edge[i][j].south = NULL;
      jtp_maptile_floor_edge[i][j].northwest = NULL;
      jtp_maptile_floor_edge[i][j].northeast = NULL;
      jtp_maptile_floor_edge[i][j].southwest = NULL;
      jtp_maptile_floor_edge[i][j].southeast = NULL;
      jtp_maptile_floor_edge[i][j].northwest_bank = NULL;
      jtp_maptile_floor_edge[i][j].northeast_bank = NULL;
      jtp_maptile_floor_edge[i][j].southwest_bank = NULL;
      jtp_maptile_floor_edge[i][j].southeast_bank = NULL;

      cur_glyph = jtp_mapglyph_cmap[i][j];
      cur_tile = jtp_cmap_to_tile(cur_glyph);
      if (cur_tile == JTP_TILE_INVALID)
      {
        /* Unknown glyph or nothing to draw */
        jtp_maptile_cmap[i][j] = NULL;
        continue;
      }

      /* Is this tile a floor tile that we have to check edges for? */
      isfloor = 0;      
      if ((cur_tile == JTP_TILE_FLOOR_LAVA) ||
          (cur_tile == JTP_TILE_FLOOR_WATER) ||
          (cur_tile == JTP_TILE_FLOOR_ICE) ||
          (cur_tile == JTP_TILE_FLOOR_AIR))
      {
        isfloor = 1;
      }

      /* If this tile is a floor tile, check for floor edges */
      if (isfloor)
      {
        /* The tile is a floor tile, we need to check the surrounding area */
        isedge = 0;
        if (j > 1)
        {
          temp_glyph = jtp_mapglyph_cmap[i][j-1];
          temp_tile = jtp_cmap_to_tile(temp_glyph);
          if (temp_tile != cur_tile)
          {
            /* Floor style ends to the west */
            k = JTP_FLOOR_EDGE_STYLE_COBBLESTONE; /* No others made so far */
            jtp_maptile_floor_edge[i][j].west = jtp_floor_edges[k].west;
          }
          /* Make a naive check for edge banks, clear unwanted ones later */
          if (i > 0)
          {
            temp_glyph = jtp_mapglyph_cmap[i-1][j-1];
            temp_tile = jtp_cmap_to_tile(temp_glyph);
            if (temp_tile != cur_tile)
            {
              /* Floor style ends to the northwest */
              k = JTP_FLOOR_EDGE_STYLE_COBBLESTONE; /* No others made so far */
              jtp_maptile_floor_edge[i][j].northwest_bank = jtp_floor_edges[k].northwest_bank;
            }
          }        
          if (i < JTP_MAP_HEIGHT-1)
          {
            temp_glyph = jtp_mapglyph_cmap[i+1][j-1];
            temp_tile = jtp_cmap_to_tile(temp_glyph);
            if (temp_tile != cur_tile)
            {         
              /* Floor style ends to the southwest */
              k = JTP_FLOOR_EDGE_STYLE_COBBLESTONE; /* No others made so far */
              jtp_maptile_floor_edge[i][j].southwest_bank = jtp_floor_edges[k].southwest_bank;
            }
          }          
        }
        if (i > 0)
        {
          temp_glyph = jtp_mapglyph_cmap[i-1][j];
          temp_tile = jtp_cmap_to_tile(temp_glyph);
          if (temp_tile != cur_tile)
          {
            /* Floor style ends to the north */
            k = JTP_FLOOR_EDGE_STYLE_COBBLESTONE; /* No others made so far */
            jtp_maptile_floor_edge[i][j].north = jtp_floor_edges[k].north;
          }
        }        
        if (i < JTP_MAP_HEIGHT-1)
        {
          temp_glyph = jtp_mapglyph_cmap[i+1][j];
          temp_tile = jtp_cmap_to_tile(temp_glyph);
          if (temp_tile != cur_tile)
          {         
            /* Floor style ends to the south */
            k = JTP_FLOOR_EDGE_STYLE_COBBLESTONE; /* No others made so far */
            jtp_maptile_floor_edge[i][j].south = jtp_floor_edges[k].south;
          }
        }
        if (j < JTP_MAP_WIDTH-1)
        {
          temp_glyph = jtp_mapglyph_cmap[i][j+1];
          temp_tile = jtp_cmap_to_tile(temp_glyph);
          if (temp_tile != cur_tile)
          {
            /* Floor style ends to the east */
            k = JTP_FLOOR_EDGE_STYLE_COBBLESTONE; /* No others made so far */
            jtp_maptile_floor_edge[i][j].east = jtp_floor_edges[k].east;
          }
          /* Make a naive check for edge banks, clear unwanted ones later */
          if (i > 0)
          {
            temp_glyph = jtp_mapglyph_cmap[i-1][j+1];
            temp_tile = jtp_cmap_to_tile(temp_glyph);
            if (temp_tile != cur_tile)
            {
              /* Floor style ends to the northeast */
              k = JTP_FLOOR_EDGE_STYLE_COBBLESTONE; /* No others made so far */
              jtp_maptile_floor_edge[i][j].northeast_bank = jtp_floor_edges[k].northeast_bank;
            }
          }        
          if (i < JTP_MAP_HEIGHT-1)
          {
            temp_glyph = jtp_mapglyph_cmap[i+1][j+1];
            temp_tile = jtp_cmap_to_tile(temp_glyph);
            if (temp_tile != cur_tile)
            {         
              /* Floor style ends to the southeast */
              k = JTP_FLOOR_EDGE_STYLE_COBBLESTONE; /* No others made so far */
              jtp_maptile_floor_edge[i][j].southeast_bank = jtp_floor_edges[k].southeast_bank;
            }
          }          
        }

        /* Check for edge corners */
        k = JTP_FLOOR_EDGE_STYLE_COBBLESTONE; /* No others made so far */
        if (jtp_maptile_floor_edge[i][j].south)
        {
          if (jtp_maptile_floor_edge[i][j].east)
            jtp_maptile_floor_edge[i][j].southeast = jtp_floor_edges[k].southeast;
          if (jtp_maptile_floor_edge[i][j].west)
            jtp_maptile_floor_edge[i][j].southwest = jtp_floor_edges[k].southwest;
        }
        if (jtp_maptile_floor_edge[i][j].north)
        {
          if (jtp_maptile_floor_edge[i][j].east)
            jtp_maptile_floor_edge[i][j].northeast = jtp_floor_edges[k].northeast;
          if (jtp_maptile_floor_edge[i][j].west)
            jtp_maptile_floor_edge[i][j].northwest = jtp_floor_edges[k].northwest;
        }

        /* Erase unnecessary edge banks */
        if (jtp_maptile_floor_edge[i][j].south)
        {
          jtp_maptile_floor_edge[i][j].southeast_bank = NULL;
          jtp_maptile_floor_edge[i][j].southwest_bank = NULL;
        }
        if (jtp_maptile_floor_edge[i][j].north)
        {
          jtp_maptile_floor_edge[i][j].northeast_bank = NULL;
          jtp_maptile_floor_edge[i][j].northwest_bank = NULL;
        }
        if (jtp_maptile_floor_edge[i][j].west)
        {
          jtp_maptile_floor_edge[i][j].southwest_bank = NULL;
          jtp_maptile_floor_edge[i][j].northwest_bank = NULL;
        }
        if (jtp_maptile_floor_edge[i][j].east)
        {
          jtp_maptile_floor_edge[i][j].southeast_bank = NULL;
          jtp_maptile_floor_edge[i][j].northeast_bank = NULL;
        }
      }

      /* If this tile is a floor tile, check for a floor decoration */
      if (cur_tile == JTP_TILE_FLOOR_COBBLESTONE)
      {
        isdecor = 0;
        for (k = 0; k < jtp_n_floor_decors; k++)
        {
          if ((jtp_floor_decors[k].x == j) && (jtp_floor_decors[k].y == i))
          {
            l = jtp_floor_decors[k].style;
            jtp_maptile_cmap[i][j] = jtp_floors[l].pattern[jtp_floor_decors[k].pos];
            isdecor = 1;
            break;
          }          
        }
        if (isdecor) continue;
      }
            
      /* For normal floor tiles (non-decoration), set position in the floor pattern */
      if ((cur_tile == JTP_TILE_FLOOR_COBBLESTONE) ||
          (cur_tile == JTP_TILE_FLOOR_ROUGH) ||
          (cur_tile == JTP_TILE_FLOOR_ROUGH_LIT) ||
        /*  (cur_tile == JTP_TILE_FLOOR_CERAMIC) || */
          (cur_tile == JTP_TILE_FLOOR_LAVA) ||
          (cur_tile == JTP_TILE_FLOOR_ICE) ||
          (cur_tile == JTP_TILE_FLOOR_WATER) ||
          (cur_tile == JTP_TILE_FLOOR_AIR))
      {
        k = jtp_get_floor_decor(cur_tile, i, j);
        l = jtp_floors[k].xspan*(i%jtp_floors[k].yspan) + (j%jtp_floors[k].xspan);
        jtp_maptile_cmap[i][j] = jtp_floors[k].pattern[l];
        continue;
      }

      /* Is this tile a known, seen wall ? */
      iswall = 0;      
      if (cur_tile == JTP_TILE_WALL_GENERIC)
      {
        /* if (levl[j][i].seenv) */ iswall = 1;
      }

      if (!iswall)
      {
        jtp_maptile_cmap[i][j] = jtp_tiles[cur_tile];
      }
      else
      {
        /* The tile is a wall, we need to check the surrounding area */
        jtp_maptile_cmap[i][j] = NULL;
        if (j > 1)
        {
          temp_glyph = jtp_mapglyph_cmap[i][j-1];
          temp_tile = jtp_cmap_to_tile(temp_glyph);
          if ((temp_tile != JTP_TILE_WALL_GENERIC) && (temp_tile != JTP_TILE_INVALID)) 
          {
            /* Wall ends to the west */
            if ((temp_glyph == S_corr) || 
                (temp_glyph == S_litcorr))            
              k = jtp_get_wall_decor(JTP_TILE_WALL_ROUGH, i, j, i, j-1);
            else
              k = jtp_get_wall_decor(JTP_TILE_WALL_BRICK, i, j, i, j-1);
            jtp_maptile_wall[i][j].west = jtp_walls[k].west;
          }
        }
        if (i > 0)
        {
          temp_glyph = jtp_mapglyph_cmap[i-1][j];
          temp_tile = jtp_cmap_to_tile(temp_glyph);
          if ((temp_tile != JTP_TILE_WALL_GENERIC) && (temp_tile != JTP_TILE_INVALID)) 
          {
            /* Wall ends to the north */
            if ((temp_glyph == S_corr) || 
                (temp_glyph == S_litcorr))
              k = jtp_get_wall_decor(JTP_TILE_WALL_ROUGH, i, j, i-1, j);
            else
              k = jtp_get_wall_decor(JTP_TILE_WALL_BRICK, i, j, i-1, j);
            jtp_maptile_wall[i][j].north = jtp_walls[k].north;
          }
        }        
        if (i < JTP_MAP_HEIGHT-1)
        {
          temp_glyph = jtp_mapglyph_cmap[i+1][j];
          temp_tile = jtp_cmap_to_tile(temp_glyph);
          if ((temp_tile != JTP_TILE_WALL_GENERIC) && (temp_tile != JTP_TILE_INVALID)) 
          {         
            /* Wall ends to the south */
            if ((temp_glyph == S_corr) || 
                (temp_glyph == S_litcorr))
              k = jtp_get_wall_decor(JTP_TILE_WALL_ROUGH, i, j, i+1, j);
            else
              k = jtp_get_wall_decor(JTP_TILE_WALL_BRICK, i, j, i+1, j);
            jtp_maptile_wall[i][j].south = jtp_walls[k].south;
          }
        }
        if (j < JTP_MAP_WIDTH-1)
        {
          temp_glyph = jtp_mapglyph_cmap[i][j+1];
          temp_tile = jtp_cmap_to_tile(temp_glyph);
          if ((temp_tile != JTP_TILE_WALL_GENERIC) && (temp_tile != JTP_TILE_INVALID))  
          {
            /* Wall ends to the east */
            if ((temp_glyph == S_corr) || 
                (temp_glyph == S_litcorr))
              k = jtp_get_wall_decor(JTP_TILE_WALL_ROUGH, i, j, i, j+1);
            else
              k = jtp_get_wall_decor(JTP_TILE_WALL_BRICK, i, j, i, j+1);
            jtp_maptile_wall[i][j].east = jtp_walls[k].east;
          }                  
        }
      }
    }
}


void jtp_draw_mini_map()
{
  int i, j, k, l, m, n, n_start, n_end;
  int token_x, token_y;

  /* Draw mini-map in lower left corner */
  jtp_set_draw_region(jtp_statusbar_x, jtp_statusbar_y,
                      jtp_statusbar_x+193, jtp_screen.height-1);
  jtp_put_img(jtp_statusbar_x, jtp_statusbar_y, jtp_statusbar);
  jtp_set_draw_region(0, 0, jtp_screen.width-1, jtp_screen.height-1);


  for (i = 0; i < JTP_MAP_HEIGHT; i++)
    for (j = 1; j < JTP_MAP_WIDTH; j++)
    {
      l = 0;
      k = jtp_mapglyph_cmap[i][j];
      /* Select a color for this glyph */
      switch(k)
      {
        case S_stone: case S_vwall: case S_hwall:
        case S_tlcorn: case S_trcorn: case S_blcorn: case S_brcorn:
        case S_tuwall: case S_tdwall: case S_tlwall: case S_trwall:
        case JTP_WINCONTENT_GLYPH_UNEXPLORED:
        case JTP_WINCONTENT_GLYPH_NOT_VISIBLE:
          l = 0; break;
        case S_corr: case S_litcorr:
          l = 238; break;
        case S_upstair: case S_dnstair:
          l = 165; break;
        case S_vodoor: case S_vcdoor: case S_hodoor: case S_hcdoor:
          l = 96; break;        
        default:
          l = 236; break;
      }
      if ((i == jtp_you_y) && (j == jtp_you_x)) l = 15;
      
      if (l > 0)
      {
        token_x = jtp_statusbar_x + 94 + 2*(j-jtp_map_x) - 2*(i-jtp_map_y);
        token_y = jtp_statusbar_y + 51 + 1*(j-jtp_map_x) + 1*(i-jtp_map_y);
        for (m = 0; m < 2; m++)
        {
          if (m == 0) { n_start = 0; n_end = 0; }
          else { n_start = -1; n_end = 1; }
          for (n = n_start; n <= n_end; n++)
          {
            if ((token_x + n > jtp_statusbar_x + 4) &&
                (token_x + n < jtp_statusbar_x + 193) &&
                (token_y + m > jtp_statusbar_y + 4) &&
                (token_y + m < jtp_statusbar_y + 96) &&
                (jtp_screen.vpage[(token_y+m)*jtp_screen.width+(token_x+n)] <= 188) &&
                (jtp_screen.vpage[(token_y+m)*jtp_screen.width+(token_x+n)] >= 180))
              jtp_screen.vpage[(token_y+m)*jtp_screen.width+(token_x+n)] = l;
          }
        }
      }
    }
}


void jtp_draw_map
(
  jtp_window * mapwindow,
  int xc, int yc
)
{
  int i, j, k, l;
  jtp_tilestats * cur_tile;
  int lightlevel;
  int x, y;

  if (!mapwindow) return;
  if (xc < 0) xc = jtp_map_x;
  if (yc < 0) yc = jtp_map_y;

  /* Check if we need to restore the game palette */
  if ((!jtp_game_palette_set) && (jtp_map_changed))
  {
    jtp_load_palette(jtp_filenames[JTP_FILE_MOUSE_CURSORS]);
    jtp_refresh();
    jtp_updatepal(0, 255);
    jtp_game_palette_set = 1;
  }

  if (jtp_map_changed)
  {
    jtp_prev_dlevel = jtp_cur_dlevel;
    jtp_cur_dlevel = u.uz.dlevel;
    if (jtp_cur_dlevel != jtp_prev_dlevel)
    {
      jtp_find_room_indices(jtp_room_indices);
      jtp_init_floor_decors(10);
      jtp_init_lights(JTP_MAX_LIGHTS);
    }
    jtp_calculate_lights();
    jtp_convert_map_objects();
    jtp_convert_map_monsters();
    jtp_convert_map_cmaps();
    jtp_map_changed = 0;
  }

  /* Clear map area */
  memset(jtp_screen.vpage, 0, jtp_screen.width*(jtp_screen.height-JTP_STATUSBAR_HEIGHT));

  /* Only draw on map area */
  jtp_set_draw_region(0, 0, jtp_screen.width-1, jtp_screen.height-1-JTP_STATUSBAR_HEIGHT);

  for (i = 0; i < JTP_MAP_HEIGHT; i++)
    for (j = 1; j < JTP_MAP_WIDTH; j++)
    {
      lightlevel = jtp_map_light[i][j];

      /* Find position of tile center */
      x = jtp_map_center_x + JTP_MAP_XMOD*(j - i + yc - xc);
      y = jtp_map_center_y + JTP_MAP_YMOD*(j + i - yc - xc);

      /* 
         Draw Vulture's Eye tiles, in order:
         1. West and north walls
         2. Floor (cmap)
         3. Floor edges
         4. Object
         5. Monster
         6. South and east walls
       */
      if ((cur_tile = jtp_maptile_wall[i][j].west) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     jtp_map_light[i][j-1], cur_tile->graphic);
      if ((cur_tile = jtp_maptile_wall[i][j].north) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     jtp_map_light[i-1][j], cur_tile->graphic);

      if ((cur_tile = jtp_maptile_cmap[i][j]) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);

      if ((cur_tile = jtp_maptile_floor_edge[i][j].west) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);
      if ((cur_tile = jtp_maptile_floor_edge[i][j].north) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);
      if ((cur_tile = jtp_maptile_floor_edge[i][j].east) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);
      if ((cur_tile = jtp_maptile_floor_edge[i][j].south) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);

      if ((cur_tile = jtp_maptile_floor_edge[i][j].northwest) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);
      if ((cur_tile = jtp_maptile_floor_edge[i][j].northeast) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);
      if ((cur_tile = jtp_maptile_floor_edge[i][j].southeast) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);
      if ((cur_tile = jtp_maptile_floor_edge[i][j].southwest) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);
      if ((cur_tile = jtp_maptile_floor_edge[i][j].northwest_bank) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);
      if ((cur_tile = jtp_maptile_floor_edge[i][j].northeast_bank) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);
      if ((cur_tile = jtp_maptile_floor_edge[i][j].southeast_bank) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);
      if ((cur_tile = jtp_maptile_floor_edge[i][j].southwest_bank) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);

      if ((cur_tile = jtp_maptile_obj[i][j]) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);

      if ((cur_tile = jtp_maptile_mon[i][j]) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);

      if ((cur_tile = jtp_maptile_wall[i][j].south) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     jtp_map_light[i+1][j], cur_tile->graphic);
      if ((cur_tile = jtp_maptile_wall[i][j].east) != NULL)
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     jtp_map_light[i][j+1], cur_tile->graphic);
    }

  /* Restore drawing region */
  jtp_set_draw_region(0, 0, jtp_screen.width-1, jtp_screen.height-1);

  /* Draw mini-map to the left of the status area */
  jtp_draw_mini_map();

  /* The old message background area is now invalid, so make sure it isn't used. */
  free(jtp_messages_background);
  jtp_messages_background = NULL;
}



jtp_list * jtp_list_new()
{
  jtp_list * list_temp;
  
  list_temp = (jtp_list *)malloc(sizeof(jtp_list));
  if (!list_temp)
  {
    jtp_write_log_message("[jtp_win.c/jtp_list_new/Check1] Out of memory!\n");
    jtp_exit_graphics(); exit(1);
  }
  list_temp->header = (jtp_listitem *)malloc(sizeof(jtp_listitem));
  if (!list_temp->header)
  {
    jtp_write_log_message("[jtp_win.c/jtp_list_new/Check2] Out of memory!\n");
    jtp_exit_graphics(); exit(1);
  }
  list_temp->header->previous = list_temp->header;
  list_temp->header->next = NULL;
  list_temp->header->itemdata = NULL;
  list_temp->previous = list_temp->header;
  list_temp->length = 0;
  return(list_temp);
}


void jtp_list_reset
(
  jtp_list *list_to_reset
)
{
  list_to_reset->previous = list_to_reset->header;
}

void jtp_list_advance
(
  jtp_list *list_to_advance
)
{
  if (list_to_advance->previous->next)
  {
    list_to_advance->previous = (jtp_listitem *)(list_to_advance->previous->next);
  }  
}

void jtp_list_retreat
(
  jtp_list *list_to_retreat
)
{
  list_to_retreat->previous = (jtp_listitem *)(list_to_retreat->previous->previous);  
}


void * jtp_list_current
(
  jtp_list *list_to_access
)
{
  if (!(list_to_access->previous->next)) 
    return(NULL);
  else 
    return(((jtp_listitem *)list_to_access->previous->next)->itemdata);
}

void jtp_list_add
(
  jtp_list *list_to_access,
  void * item_to_add
)
{
  jtp_listitem * tempitem;
  
  tempitem = (jtp_listitem *)malloc(sizeof(jtp_listitem));
  if (!tempitem)
  {
    jtp_write_log_message("[jtp_win.c/jtp_list_add/Check1] Out of memory!\n");
    jtp_exit_graphics(); exit(1);
  }
  tempitem->itemdata = item_to_add;
  tempitem->previous = list_to_access->previous;
  tempitem->next = list_to_access->previous->next;
  if (tempitem->next)
    ((jtp_listitem *)tempitem->next)->previous = tempitem;
  list_to_access->previous->next = tempitem;  
  list_to_access->length++;
}


void jtp_list_remove
(
  jtp_list *list_to_access,
  void * item_to_remove
)
{
  jtp_listitem * tempitem;
  
  jtp_list_reset(list_to_access);
  
  while ((list_to_access->previous->next) && 
         (((jtp_listitem *)list_to_access->previous->next)->itemdata != item_to_remove))
    jtp_list_advance(list_to_access);
  if (list_to_access->previous->next)
  {
    tempitem = (jtp_listitem *)(list_to_access->previous->next);
    list_to_access->previous->next = tempitem->next;
    if (tempitem->next) 
      ((jtp_listitem *)tempitem->next)->previous = list_to_access->previous;
    free(tempitem);
  }
  list_to_access->length--;
}


int jtp_list_length
(
  jtp_list *list_to_access
)
{
  return(list_to_access->length);
}  


void jtp_free_menu
(
  jtp_menu * menu_to_free
)
{
  jtp_menuitem * menuitemtemp;
  
  if (!menu_to_free) return;
  
  jtp_list_reset(menu_to_free->items);
  menuitemtemp = (jtp_menuitem *)jtp_list_current(menu_to_free->items);
  
  while (menuitemtemp)
  {
    jtp_list_remove(menu_to_free->items, menuitemtemp);
    free(menuitemtemp->text);
    free(menuitemtemp);
    
    jtp_list_reset(menu_to_free->items);
    menuitemtemp = (jtp_menuitem *)jtp_list_current(menu_to_free->items);
  }
  
  free(menu_to_free->items->header);
  free(menu_to_free->items);
  free(menu_to_free->prompt);
  free(menu_to_free);
}

void jtp_free_buttons
(
  jtp_list * buttonlist
)
{
  jtp_button * buttontemp;
  
  if (!buttonlist) return;
  
  jtp_list_reset(buttonlist);
  buttontemp = jtp_list_current(buttonlist);
  
  while (buttontemp)
  {
    jtp_list_remove(buttonlist, buttontemp);
    free(buttontemp->text);
    free(buttontemp);
    
    jtp_list_reset(buttonlist);
    buttontemp = jtp_list_current(buttonlist);
  }
  
  free(buttonlist->header);
  free(buttonlist);
}


void jtp_clear_screen()
{
  memset(jtp_screen.vpage, 0, jtp_screen.width*jtp_screen.height);
}

unsigned char * jtp_draw_window
(
  int x, int y,
  int width, int height
)
{
  int i, j;
  unsigned char * window_background;
  
  window_background = jtp_get_img(x, y, x+width, y+height);

  /* Draw corners */
  jtp_put_stencil(x, y, jtp_defwin.corner_tl);
  jtp_put_stencil(x+width-jtp_defwin.corner_tr[3], y, jtp_defwin.corner_tr);
  jtp_put_stencil(x, y+height-jtp_defwin.corner_bl[1], jtp_defwin.corner_bl);
  jtp_put_stencil(x+width-jtp_defwin.corner_br[3], y+height-jtp_defwin.corner_br[1], 
                  jtp_defwin.corner_br);

  /* Draw top border */
  jtp_set_draw_region(x+jtp_defwin.border_left[3], y,
                      x+width-jtp_defwin.border_right[3],
                      y+jtp_defwin.border_top[1]);
  i = x + jtp_defwin.border_left[3];
  while (i <= x+width-jtp_defwin.border_right[3])
  {
    jtp_put_stencil(i, y, jtp_defwin.border_top);
    i += jtp_defwin.border_top[3];
  }

  /* Draw bottom border */
  jtp_set_draw_region(x+jtp_defwin.border_left[3],
                      y+height-jtp_defwin.border_bottom[1],
                      x+width-jtp_defwin.border_right[3],
                      y+height);
  i = x + jtp_defwin.border_left[3];
  while (i <= x+width-jtp_defwin.border_right[3])
  {
    jtp_put_stencil(i, y+height-jtp_defwin.border_bottom[1], jtp_defwin.border_bottom);
    i += jtp_defwin.border_bottom[3];
  }

  /* Draw left border */
  jtp_set_draw_region(x, y+jtp_defwin.border_top[1],
                      x+jtp_defwin.border_left[3],
                      y+height-jtp_defwin.border_bottom[1]);
  i = y + jtp_defwin.border_top[1];
  while (i <= y+height-jtp_defwin.border_bottom[1])
  {
    jtp_put_stencil(x, i, jtp_defwin.border_left);
    i += jtp_defwin.border_left[1];
  }

  /* Draw right border */
  jtp_set_draw_region(x+width-jtp_defwin.border_right[3],
                      y+jtp_defwin.border_top[1],
                      x+width,
                      y+height-jtp_defwin.border_bottom[1]);
  i = y + jtp_defwin.border_top[1];
  while (i <= y+height-jtp_defwin.border_bottom[1])
  {
    jtp_put_stencil(x+width-jtp_defwin.border_right[3], i, jtp_defwin.border_right);
    i += jtp_defwin.border_right[1];
  }

  /* Draw center area */
  jtp_set_draw_region(x+jtp_defwin.border_left[3],
                      y+jtp_defwin.border_top[1],
                      x+width-jtp_defwin.border_right[3],
                      y+height-jtp_defwin.border_bottom[1]);
  i = y + jtp_defwin.border_top[1];
  while (i <= y+height-jtp_defwin.border_bottom[1])
  {
    j = x + jtp_defwin.border_left[3];
    while (j <= x+width-jtp_defwin.border_right[3])
    {
      jtp_put_img(j, i, jtp_defwin.center);
      j += jtp_defwin.center[3];
    }
    i += jtp_defwin.center[1];
  }

  jtp_set_draw_region(0, 0, jtp_screen.width-1, jtp_screen.height-1);
  return(window_background);
}

unsigned char * jtp_draw_dropdown_window
(
  int x, int y,
  int width, int height
)
{
  int i, j;
  unsigned char * window_background;
  
  window_background = jtp_get_img(x, y, x+width, y+height);

  /* Draw center area */
  jtp_set_draw_region(x, y, x+width-1, y+height-1);
  i = y;
  while (i <= y+height)
  {
    j = x;
    while (j <= x+width)
    {
      jtp_put_img(j, i, jtp_defwin.center);
      j += jtp_defwin.center[3];
    }
    i += jtp_defwin.center[1];
  }

  /* Draw black border */
  jtp_rect(x, y, x+width-1, y+height-1, 0);

  /* Draw edges (raised) */
  jtp_bres_line(x+1, y+1, x+width-3, y+1, 26);
  jtp_bres_line(x+1, y+height-2, x+width-2, y+height-2, 37);
  jtp_bres_line(x+width-2, y+1, x+width-2, y+height-2, 40);
  jtp_bres_line(x+1, y+1, x+1, y+height-3, 26);

  jtp_set_draw_region(0, 0, jtp_screen.width-1, jtp_screen.height-1);
  return(window_background);
}


void jtp_draw_button
(
  int x, int y,
  int width, int height,
  char *str
)
{
  int i;
  
  i = (width - jtp_text_length(str, JTP_FONT_BUTTON))/2;

  /* Black edge */
  jtp_rect(x+1, y+1, x+width-2, y+height-2, 0);
  /* Outer edge (lowered) */
  jtp_bres_line(x, y, x+width-2, y, 37);
  jtp_bres_line(x, y+1, x, y+height-2, 40);
  jtp_bres_line(x+1, y+height-1, x+width-1, y+height-1, 26);
  jtp_bres_line(x+width-1, y+1, x+width-1, y+height-1, 26);
  /* Inner edge (raised) */
  jtp_bres_line(x+3, y+height-3, x+width-3, y+height-3, 37);
  jtp_bres_line(x+2, y+2, x+width-4, y+2, 26);
  jtp_bres_line(x+width-3, y+2, x+width-3, y+height-3, 40);
  jtp_bres_line(x+2, y+2, x+2, y+height-4, 26);
  jtp_put_text(x+i+1, y+jtp_fonts[JTP_FONT_BUTTON].baseline+6,
               JTP_FONT_BUTTON, 0,
               str,
               jtp_screen.vpage);
  jtp_put_text(x+i,
               y+jtp_fonts[JTP_FONT_BUTTON].baseline+5,
               JTP_FONT_BUTTON, 15,
               str,
               jtp_screen.vpage);
}

void jtp_draw_buttons
(
  int x, int y,
  jtp_list * buttons
)
{
  jtp_button * tempbutton;
  
  if (!buttons) return;
  
  jtp_list_reset(buttons);
  tempbutton = jtp_list_current(buttons);  
  while (tempbutton)
  {
    jtp_draw_button(x + tempbutton->x, y + tempbutton->y, 
                    tempbutton->width, tempbutton->height, 
                    tempbutton->text);
    jtp_list_advance(buttons);
    tempbutton = (jtp_button *)jtp_list_current(buttons);  
  }
}

void jtp_draw_menu
(
  int x, int y,
  jtp_menu * menu,
  jtp_menuitem * firstitem
)
{
  int i, j;
  int firstitem_y;
  int firstitem_index;
  int menu_item_count;
  jtp_menuitem * tempmenuitem;

  if (!menu) return;

  if (menu->prompt)
  {
    jtp_put_text(x + menu->prompt_x, 
                 y + menu->prompt_y + jtp_fonts[JTP_FONT_HEADLINE].baseline + 1,
                 JTP_FONT_HEADLINE, 0,
                 menu->prompt,
                 jtp_screen.vpage);
    jtp_put_text(x+ menu->prompt_x,
                 y + menu->prompt_y + jtp_fonts[JTP_FONT_HEADLINE].baseline,
                 JTP_FONT_HEADLINE, 15,
                 menu->prompt,
                 jtp_screen.vpage);
  }
  
  if (menu->items)
  {
    firstitem_index = 0;  
    jtp_list_reset(menu->items);
    tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
    while ((tempmenuitem) && (tempmenuitem != firstitem))
    {
      jtp_list_advance(menu->items);
      tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
      firstitem_index++;
    }
    if (tempmenuitem) firstitem_y = tempmenuitem->y;    
    
    while ((tempmenuitem) &&
           (tempmenuitem->y - firstitem->y + tempmenuitem->height < JTP_MAX_MENUITEMS_HEIGHT))
    {
      i = x + tempmenuitem->x;
      if (tempmenuitem->count == JTP_NOT_SELECTABLE) j = 0;
      else
      {
        switch(menu->selectiontype)
        {
          case PICK_NONE:
            j = 0;
            i += jtp_defwin.radiobutton_off[3] + 4;            
            break;
          case PICK_ONE:
            j = (jtp_defwin.radiobutton_off[1] - jtp_fonts[JTP_FONT_MENU].baseline)/2;
            if (j < 0) j = 0;
            if (tempmenuitem->selected == TRUE)
              jtp_put_img(i, y + menu->items_y + tempmenuitem->y - firstitem_y, jtp_defwin.radiobutton_on);
            else  
              jtp_put_img(i, y + menu->items_y + tempmenuitem->y - firstitem_y, jtp_defwin.radiobutton_off);
            i += jtp_defwin.radiobutton_off[3] + 4;
            break;
          case PICK_ANY:
            j = (jtp_defwin.checkbox_off[1] - jtp_fonts[JTP_FONT_MENU].baseline)/2;
            if (j < 0) j = 0;
            if (tempmenuitem->selected == TRUE)
              jtp_put_img(i, y + menu->items_y + tempmenuitem->y - firstitem_y, jtp_defwin.checkbox_on);
            else  
              jtp_put_img(i, y + menu->items_y + tempmenuitem->y - firstitem_y, jtp_defwin.checkbox_off);
            i += jtp_defwin.checkbox_off[3] + 4;
            break;
          default:
            j = 0;
            break;
        }
      }
      jtp_put_text(i, y + menu->items_y + tempmenuitem->y - firstitem_y + jtp_fonts[JTP_FONT_MENU].baseline + j + 1,
                   JTP_FONT_MENU, 0, tempmenuitem->text, jtp_screen.vpage);
      jtp_put_text(i, y + menu->items_y + tempmenuitem->y - firstitem_y + jtp_fonts[JTP_FONT_MENU].baseline + j,
                   JTP_FONT_MENU, 15, tempmenuitem->text, jtp_screen.vpage);
      jtp_list_advance(menu->items);
      tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
    }
  }

  if (menu->need_scrollbar)
  {  
    jtp_put_img(x + menu->scrollbar_x, y + menu->scrollup_y, jtp_defwin.scrollbutton_up);
    jtp_put_img(x + menu->scrollbar_x, y + menu->scrolldown_y, jtp_defwin.scrollbutton_down);
    
    jtp_set_draw_region(x + menu->scrollbar_x, 
                        y + menu->scrollup_y + jtp_defwin.scrollbutton_up[1],
                        x + menu->scrollbar_x + jtp_defwin.scrollbar[3] - 1,
                        y + menu->scrolldown_y - 1);
    for (i = y + menu->scrollup_y + jtp_defwin.scrollbutton_up[1];
         i < y + menu->scrolldown_y; i+= jtp_defwin.scrollbar[1])
      jtp_put_img(x + menu->scrollbar_x, i, jtp_defwin.scrollbar);   
    jtp_set_draw_region(0, 0, jtp_screen.width - 1, jtp_screen.height - 1);

    /* Count n. of items in menu and draw scroll indicator */
    menu_item_count = jtp_list_length(menu->items);
    if (menu_item_count > 1)
    {
      i = menu->scrollup_y + jtp_defwin.scrollbutton_up[1] + 
          (firstitem_index*(menu->scrolldown_y - jtp_defwin.scroll_indicator[1] - 
          menu->scrollup_y - jtp_defwin.scrollbutton_up[1]))/(menu_item_count - 1);    
      jtp_put_img(x + menu->scrollbar_x, y + i, jtp_defwin.scroll_indicator);
    }  
  }
}


void jtp_draw_status
(
  jtp_window * tempwindow
)
{
  int i, j, k, l;
  char tempbuffer[1024];
  char namebuffer[1024];
  char * tok;
  int  token_ok, token_x, token_y;
  int  n_conditions, name_passed;
  int tokentype;

  /* Draw background */
  jtp_set_draw_region(jtp_statusbar_x+193, jtp_statusbar_y,
                      jtp_screen.width-1, jtp_screen.height-1);
  jtp_put_img(jtp_statusbar_x, jtp_statusbar_y, jtp_statusbar);
  jtp_set_draw_region(0, 0, jtp_screen.width-1,jtp_screen.height-1);

  /* If the screen is wider than the statusbar, clear the edge areas */
  if (jtp_screen.width > JTP_STATUSBAR_WIDTH)
  {
    jtp_fill_rect(0, jtp_statusbar_y, jtp_statusbar_x-1, jtp_screen.height-1, 0);
    jtp_fill_rect(jtp_statusbar_x + JTP_STATUSBAR_WIDTH, jtp_statusbar_y, 
                  jtp_screen.width-1, jtp_screen.height-1, 0);
  }

#ifdef PLAIN_STATUS_PRINTING
  /* Draw status lines */
  for (i = 0; i < tempwindow->curs_rows; i++)
  {
    for (j = 0; j < tempwindow->curs_cols; j++)
    {
      if (tempwindow->rows[i][j] > 0)
        tempbuffer[j] = tempwindow->rows[i][j];
      else tempbuffer[j] = ' ';
    }
    tempbuffer[tempwindow->curs_cols] = '\0';
    jtp_put_text(100, 
                 jtp_screen.height-JTP_STATUSBAR_HEIGHT + 10 + 
                   i*jtp_fonts[JTP_FONT_STATUS].lineheight + 
                   jtp_fonts[JTP_FONT_STATUS].baseline + 1,
                 JTP_FONT_STATUS, 0,
                 tempbuffer,
                 jtp_screen.vpage);
    jtp_put_text(100, 
                 jtp_screen.height-JTP_STATUSBAR_HEIGHT + 10 + 
                   i*jtp_fonts[JTP_FONT_STATUS].lineheight + 
                   jtp_fonts[JTP_FONT_STATUS].baseline,
                 JTP_FONT_STATUS, 15,
                 tempbuffer,
                 jtp_screen.vpage);
  }
#else
  /* 
   * Parse status lines into 'status tokens'. Assign location of tokens on-screen.
   * Note: this section contains lots of 'magic numbers', but they're all indidivual
   * coordinates, so it doesn't seem worth it to make a separate define for each.
   * They are chosen to match the graphic file used.
   */
  n_conditions = 0;
  namebuffer[0] = '\0';
  name_passed = 0;
  for (i = 0; i < tempwindow->curs_rows; i++)
  {
    for (j = 0; j < tempwindow->curs_cols; j++)
    {
      if (tempwindow->rows[i][j] > 0)
        tempbuffer[j] = tempwindow->rows[i][j];
      else tempbuffer[j] = ' ';
    }
    tempbuffer[tempwindow->curs_cols] = '\0';
    
    tok = strtok(tempbuffer, " ");
    while (tok)
    {
      token_ok = 0;
      if (strncmp(tok, "St:", 3) == 0) /* Strength token */
      {
        token_ok = 1; name_passed = 1; token_x = 226; 
        token_y = 15 + 1*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;
      }  
      else if (strncmp(tok, "Dx:", 3) == 0) /* Dexterity token */
      {
        token_ok = 1; name_passed = 1; token_x = 226;
        token_y = 15 + 2*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;
      }
      else if (strncmp(tok, "Co:", 3) == 0) /* Constitution token */
      {
        token_ok = 1; name_passed = 1; token_x = 226;
        token_y = 15 + 3*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;        
      }
      else if (strncmp(tok, "In:", 3) == 0) /* Intelligence token */
      {
        token_ok = 1; name_passed = 1; token_x = 286;
        token_y = 15 + 1*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;        
      }
      else if (strncmp(tok, "Wi:", 3) == 0) /* Wisdom token */
      {
        token_ok = 1; name_passed = 1; token_x = 286;
        token_y = 15 + 2*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;        
      }
      else if (strncmp(tok, "Ch:", 3) == 0) /* Charisma token */
      {
        token_ok = 1; name_passed = 1; token_x = 286;
        token_y = 15 + 3*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;
      }
      else if (strncmp(tok, "HP:", 3) == 0) /* Hit points token */
      {
        token_ok = 1; name_passed = 1; token_x = 346;
        token_y = 15 + 1*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;        
      }
      else if (strncmp(tok, "Pw:", 3) == 0) /* Power token */
      {
        token_ok = 1; name_passed = 1; token_x = 346;
        token_y = 15 + 2*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;        
      }
      else if (strncmp(tok, "AC:", 3) == 0) /* Armor class token */
      {
        token_ok = 1; name_passed = 1; token_x = 346;
        token_y = 15 + 3*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;        
      }
      else if (strncmp(tok, "Exp:", 4) == 0) /* Experience token */
      {
        token_ok = 1; name_passed = 1; token_x = 406;
        token_y = 15 + jtp_fonts[JTP_FONT_STATUS].baseline;        
      }
      else if (strncmp(tok, "Dlvl:", 5) == 0) /* Dungeon level token */
      {
        token_ok = 1; name_passed = 1; token_x = 406;
        token_y = 15 + 1*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;        
      }
      else if (strncmp(tok, "$:", 2) == 0) /* Gold token */
      {
        token_ok = 1; name_passed = 1; token_x = 406;
        token_y = 15 + 2*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;        
      }
      else if (strncmp(tok, "T:", 2) == 0) /* Time token */
      {
        token_ok = 1; name_passed = 1; token_x = 406;
        token_y = 15 + 3*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;
      }
      /* Other non-empty tokens are parts of name & title, conditions or alignment */
      else
      {
        if (name_passed)
        {
          token_ok = 1; token_x = 466; 
          token_y = 15 + n_conditions*jtp_fonts[JTP_FONT_STATUS].lineheight + jtp_fonts[JTP_FONT_STATUS].baseline;
          n_conditions++;
        }
        else
        {
          strcat(namebuffer, tok);
          strcat(namebuffer, " ");
        }  
      }
      /* If the token was recognized, display it */
      if (token_ok)
      {
        jtp_put_text(jtp_statusbar_x + token_x, jtp_statusbar_y + token_y + 1,
                     JTP_FONT_STATUS, 0, tok, jtp_screen.vpage);
        jtp_put_text(jtp_statusbar_x + token_x, jtp_statusbar_y + token_y,
                     JTP_FONT_STATUS, 15, tok, jtp_screen.vpage);        
      }
      /* Get the next token */
      tok = strtok(NULL, " ");
    }
    /* Show character name */
    jtp_put_text(jtp_statusbar_x + 226, 
                 jtp_statusbar_y + 15 + jtp_fonts[JTP_FONT_STATUS].baseline + 1,
                 JTP_FONT_STATUS, 0, namebuffer, jtp_screen.vpage);
    jtp_put_text(jtp_statusbar_x + 226, 
                 jtp_statusbar_y + 15 + jtp_fonts[JTP_FONT_STATUS].baseline,
                 JTP_FONT_STATUS, 15, namebuffer, jtp_screen.vpage);
  }
#endif
}



int jtp_draw_messages
(
  jtp_window * tempwindow
)
{
  int i, j, k, l;
  int tempwidth;
  char * tempstring;
  
  jtp_put_img(0, 0, jtp_messages_background);
  free(jtp_messages_background);
  
  /* Calculate width and height of messages to be shown */
  jtp_messages_height = 0;
  tempwidth = 0;
  for (i = JTP_MAX_SHOWN_MESSAGES-1+jtp_first_shown_message; i >= jtp_first_shown_message; i--)
    if (tempwindow->rows[i])
    {
      /* Check message age */
      k = moves; /* Current time in moves */
      k -= tempwindow->rows[i][0];
      if (k < 0) k = 0; /* This shouldn't happen, unless there's time travel etc. */
     
      /* If we're viewing previous messages, make sure they all get shown */
      if ((jtp_first_shown_message > 0) && (k >= JTP_MAX_MESSAGE_COLORS))
        k = JTP_MAX_MESSAGE_COLORS-1;

      if (k < JTP_MAX_MESSAGE_COLORS)
      {
        /* Message is new enough to be shown */
        tempstring = (char *)(tempwindow->rows[i]) + sizeof(int);
        jtp_messages_height += jtp_text_height(tempstring, JTP_FONT_MESSAGE);
        j = jtp_text_length(tempstring, JTP_FONT_MESSAGE);
        if (j > tempwidth) tempwidth = j;
      }
    }

  if (jtp_messages_height > 0) 
  {
    jtp_messages_height += 4;
    tempwidth += 8;
    jtp_messages_background = jtp_get_img(0, 0, jtp_screen.width-1, jtp_messages_height-1);
  }
  else jtp_messages_background = NULL;

  /* Shade the message area */
  for (i = 0; i < jtp_messages_height; i++)
    for (j = (jtp_screen.width - tempwidth)/2; j <= (jtp_screen.width - tempwidth)/2 + tempwidth; j++)
    {
      k = jtp_screen.vpage[i*jtp_screen.width + j];
      jtp_screen.vpage[i*jtp_screen.width + j] = jtp_shade[(JTP_MAX_SHADES/2)*256+k];
    }

  j = jtp_fonts[JTP_FONT_MESSAGE].baseline;
  for (i = JTP_MAX_SHOWN_MESSAGES-1+jtp_first_shown_message; i >= jtp_first_shown_message; i--)
    if (tempwindow->rows[i])
    {
      /* Shade message according to its age */
      k = moves; /* Current time in moves */
      k -= tempwindow->rows[i][0];
      if (k < 0) k = 0; /* This shouldn't happen, unless there's time travel etc. */
      /* else if (k >= JTP_MAX_MESSAGE_COLORS) k = JTP_MAX_MESSAGE_COLORS-1; */

      /* If we're viewing previous messages, make sure they all get shown */
      if ((jtp_first_shown_message > 0) && (k >= JTP_MAX_MESSAGE_COLORS))
        k = JTP_MAX_MESSAGE_COLORS-1;

      if (k < JTP_MAX_MESSAGE_COLORS)
      {
        /* Center message on-screen */
        tempstring = (char *)(tempwindow->rows[i]) + sizeof(int);      
        l = (jtp_screen.width - jtp_text_length(tempstring, JTP_FONT_MESSAGE))/2;
  /*      
        printf("Text: [%s], length %d, coord: %d\n", tempstring, jtp_text_length(tempstring, JTP_FONT_MESSAGE), l);
        getch();
  */      
        /* Draw message */
        jtp_put_text(l, j + 1,
                     JTP_FONT_MESSAGE, 0,
                     tempstring,
                     jtp_screen.vpage);
        jtp_put_text(l, j,
                     JTP_FONT_MESSAGE, jtp_message_colors[k],
                     tempstring,
                     jtp_screen.vpage);
        j += jtp_text_height(tempstring, JTP_FONT_MESSAGE);
      }
    }

  return(jtp_messages_height); /* Return height of drawn messages */
}

void jtp_draw_all_windows()
{
  jtp_window * tempwindow;
  unsigned char * tempimage;
  char tempbuffer[256];
  char * tempstring;
  int i, j, k;

 /* jtp_messagebox("Drawing all windows"); */
  
  /* Draw basic windows in correct order: map, messages, status */
  tempwindow = jtp_find_window(WIN_MAP);
  jtp_draw_map(tempwindow, -1, -1);
  tempwindow = jtp_find_window(WIN_MESSAGE);
  jtp_draw_messages(tempwindow);
  tempwindow = jtp_find_window(WIN_STATUS);
  jtp_draw_status(tempwindow);
  
  /* Draw the other windows in order of appearance */  
  jtp_list_reset(jtp_windowlist);
  tempwindow = (jtp_window *)jtp_list_current(jtp_windowlist);
  while (tempwindow)
  {
    switch(tempwindow->wintype)
    {
      case NHW_MAP: break;
      case NHW_STATUS: break;
      case NHW_MESSAGE: break;
      default: break;
    }        
    jtp_list_advance(jtp_windowlist);
    tempwindow = (jtp_window *)jtp_list_current(jtp_windowlist);
  }
}


void jtp_show_screen()
{
  jtp_refresh();
}

void jtp_show_map_area()
{
  jtp_refresh_region(0, 0, jtp_screen.width-1, jtp_statusbar_y-1);
}

void jtp_show_status_area()
{
  jtp_refresh_region(0, jtp_statusbar_y, jtp_screen.width-1, jtp_screen.height-1);
}

void jtp_show_message_area
(
  int messages_height /* Height of messages in message area */
)
{
  jtp_refresh_region(0, 0, jtp_screen.width-1, jtp_messages_height-1);
}

void jtp_read_mouse_input()
{
  /* If the palette is faded out for some reason, restore it */
  jtp_updatepal(0, 255);
  
  jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
}


int jtp_query
(
  int qx, int qy,        /* Query window position */
  const char * qmessage, /* Message to show player */
  int nanswers,          /* Number of possible answers */
  char * panswers,       /* Answer strings separated by underscores ('_') */
  int is_dropdown        /* Is the window a dropdown menu? */
)
{
  int i, j, k;
  int query_x, query_y;

  int is_direction_query = 0;              /* Is this a "what direction" query? */
  int direction_x, direction_y;            /* Selected direction */
  int directions_x, directions_y;          /* Start coordinates of direction graphic */
  int directions_width, directions_height; /* Dimensions of direction graphic */

  int totalwidth, totalheight;
  int buttons_width, buttons_height;
  jtp_button * pchoices;
  int nbuttons;                            /* In case of a 'key query', nbuttons != nanswers */

  int nchoice = -1;
  unsigned char * query_background;
  char pressedkey;

  char tempbuffer[256];

  /* Check for a direction query */
  if (strstr(qmessage, "direction"))
    is_direction_query = 1;

  /* Remove keys in buffer */
  while (jtp_kbhit()) jtp_getch();

  /* Check for a shortcut response */
  if (jtp_is_shortcut_active)
  {
    jtp_is_shortcut_active = 0;
    if (nanswers == 0)
      return(jtp_shortcut_query_response);
    else
    {
      /* Find the index of the shortcut response */
      i = 0; j = 0; k = 0;
      while (i < nanswers)
      {
        while (panswers[j] == '_') j++;
        if (panswers[j] == jtp_shortcut_query_response)
          return(i);
        while ((j < strlen(panswers)) && (panswers[j] != '_')) j++;
        i++;
      }
    }
  }

  pchoices = NULL;
  nbuttons = 0;

  /* Create choice buttons for the possible answers */
  if (nanswers > 0)
  {
    nbuttons = nanswers;
    pchoices = (jtp_button *)malloc(nbuttons*sizeof(jtp_button));
    if (!pchoices)
    {
      jtp_write_log_message("[jtp_win.c/jtp_query/Check1] Out of memory!\n");
      jtp_exit_graphics(); exit(1);
    }
  
    j = 0; k = 0;
    for (i = 0; i < nbuttons; i++)
    {
      while (panswers[j] == '_') j++;
      k = j; while ((k < strlen(panswers)) && (panswers[k] != '_')) k++;
      pchoices[i].text = (char *)malloc((k-j+1)*sizeof(char));
      if (!pchoices[i].text)
      {
        jtp_write_log_message("[jtp_win.c/jtp_query/Check2] Out of memory!\n");
        jtp_exit_graphics(); exit(1);
      }    
      memcpy(pchoices[i].text, panswers+j, k-j);
      pchoices[i].text[k-j] = '\0';
      pchoices[i].width = jtp_text_length(pchoices[i].text, JTP_FONT_BUTTON) + 11;
      pchoices[i].height = jtp_fonts[JTP_FONT_BUTTON].lineheight + 10;
      pchoices[i].id = i;
      if (nanswers == 1) pchoices[i].accelerator = '\r'; /* Enter */
      else pchoices[i].accelerator = pchoices[i].text[0];
      j = k;
    }
  }
  
  /* If this is a key query with "or ?*]", add 'show choices/inventory' and 'cancel' buttons */
  if ((nanswers == 0) && (strstr(qmessage, "or ?*]")))
  {
    nbuttons = 3;
    pchoices = (jtp_button *)malloc(nbuttons*sizeof(jtp_button));
    if ((nbuttons > 0) && (!pchoices))
    {
      jtp_write_log_message("[jtp_win.c/jtp_query/Check1] Out of memory!\n");
      jtp_exit_graphics(); exit(1);
    }  

    pchoices[0].accelerator = '?';
    pchoices[0].text = (char *)malloc(strlen("Show choices")+1);
    strcpy(pchoices[0].text, "Show choices");    

    pchoices[1].accelerator = '*';
    pchoices[1].text = (char *)malloc(strlen("Show inventory")+1);
    strcpy(pchoices[1].text, "Show inventory");

    pchoices[2].accelerator = 27; /* ESC */
    pchoices[2].text = (char *)malloc(strlen("Cancel")+1);
    strcpy(pchoices[2].text, "Cancel");

    for (i = 0; i < nbuttons; i++)
    {
      pchoices[i].width = jtp_text_length(pchoices[i].text, JTP_FONT_BUTTON) + 11;
      pchoices[i].height = jtp_fonts[JTP_FONT_BUTTON].lineheight + 10;
      pchoices[i].id = i;
    }  
  }


  /* Calculate width, height and position of query window */
  totalwidth = jtp_text_length((char *)qmessage, JTP_FONT_HEADLINE);
  totalheight = jtp_text_height((char *)qmessage, JTP_FONT_HEADLINE);
  totalheight += jtp_fonts[JTP_FONT_HEADLINE].lineheight;

  if (is_direction_query)
  {
    /* Add direction arrows */
    directions_width = jtp_defwin.direction_arrows[2]*256 + jtp_defwin.direction_arrows[3];
    directions_height = jtp_defwin.direction_arrows[0]*256 + jtp_defwin.direction_arrows[1];

    if (totalwidth < directions_width)
      totalwidth = directions_width;
    totalheight += directions_height;
  }
  else
  {
    /* If all answers are valid, add explanatory note */
    if (nanswers == 0) totalheight += 2*jtp_fonts[JTP_FONT_MENU].lineheight;
  }


  buttons_width = 0; buttons_height = 0;
  if (is_dropdown)
  {
    for (i = 0; i < nbuttons; i++)
    {
      buttons_height += pchoices[i].height + 10;
      if (pchoices[i].width > buttons_width) buttons_width = pchoices[i].width;
    }
  }
  else
  {
    for (i = 0; i < nbuttons; i++)
    {
      buttons_width += pchoices[i].width + 10;
      if (pchoices[i].height > buttons_height) buttons_height = pchoices[i].height;
    }
    buttons_width -= 10; /* Remove extra spacing */
  }
  if (buttons_width > totalwidth) totalwidth = buttons_width;
  totalheight += buttons_height;

  totalwidth += jtp_defwin.border_left[3];
  totalwidth += jtp_defwin.border_right[3];
  totalheight += jtp_defwin.border_top[1];
  totalheight += jtp_defwin.border_bottom[1];

  if (qx < 0) query_x = (jtp_screen.width - totalwidth) / 2;
  else if (qx < jtp_screen.width - totalwidth) query_x = qx;
  else query_x = jtp_screen.width - totalwidth;

  if (qy < 0) query_y = (jtp_screen.height - totalheight) / 2;
  else if (qy < jtp_screen.height - totalheight) query_y = qy;
  else query_y = jtp_screen.height - totalheight;
  
  /* Calculate direction arrows position */
  if (is_direction_query)
  {
    directions_x = query_x + jtp_defwin.border_left[3] + 
                   (totalwidth-jtp_defwin.border_left[3]-jtp_defwin.border_right[3] 
                    - directions_width)/2;
    directions_y = query_y + jtp_defwin.border_top[1];
    directions_y += jtp_text_height((char *)qmessage, JTP_FONT_HEADLINE);
    directions_y += jtp_fonts[JTP_FONT_HEADLINE].lineheight;
  }

  /* Calculate button positions */
  k = query_x + jtp_defwin.border_left[3] + 
      (totalwidth-jtp_defwin.border_left[3]-jtp_defwin.border_right[3] - buttons_width)/2;
  j = query_y + jtp_defwin.border_top[1];
  j += jtp_text_height((char *)qmessage, JTP_FONT_HEADLINE);
  j += jtp_fonts[JTP_FONT_HEADLINE].lineheight;
  if (nanswers == 0) j += 2*jtp_fonts[JTP_FONT_MENU].lineheight;

  if (is_dropdown)
  {
    for (i = 0; i < nbuttons; i++)
    {
      pchoices[i].x = k;
      pchoices[i].y = j;
      j += pchoices[i].height+10;
    }
  }
  else
  {
    for (i = 0; i < nbuttons; i++)
    {
      pchoices[i].x = k;
      pchoices[i].y = j;
      k += pchoices[i].width+10;
    }
  }

  /* Store background graphics */
  query_background = jtp_draw_window(query_x, query_y, totalwidth, totalheight);

  /* Draw buttons */
  for (i = 0; i < nbuttons; i++)
  {
    jtp_draw_button(pchoices[i].x, pchoices[i].y,
                    pchoices[i].width, pchoices[i].height,
                    pchoices[i].text);
  }

  /* If this was a direction query, draw direction arrows */
  if (is_direction_query)  
  {
    jtp_put_stencil(directions_x, directions_y, jtp_defwin.direction_arrows);  
  }

  /* Draw query message */
  jtp_put_text(jtp_defwin.border_left[3] + query_x+1,
               jtp_defwin.border_top[1] + query_y + jtp_fonts[JTP_FONT_HEADLINE].baseline+1,
               JTP_FONT_HEADLINE,
               0,
               (char *)qmessage,
               jtp_screen.vpage);
  jtp_put_text(jtp_defwin.border_left[3] + query_x,
               jtp_defwin.border_top[1] + query_y + jtp_fonts[JTP_FONT_HEADLINE].baseline,
               JTP_FONT_HEADLINE,
               15,
               (char *)qmessage,
               jtp_screen.vpage);

  /* Draw explanatory notice */
  if ((nanswers == 0) && (!is_direction_query))
  {
    jtp_put_text(jtp_defwin.border_left[3] + query_x+1,
                 jtp_defwin.border_top[1] + query_y + 
                   jtp_text_height((char *)qmessage, JTP_FONT_HEADLINE) + 
                   jtp_fonts[JTP_FONT_HEADLINE].lineheight + 
                   jtp_fonts[JTP_FONT_MENU].baseline + 1,
                 JTP_FONT_HEADLINE,
                 0,
                 "(type any key)",
                 jtp_screen.vpage);
    jtp_put_text(jtp_defwin.border_left[3] + query_x+1,
                 jtp_defwin.border_top[1] + query_y + 
                   jtp_text_height((char *)qmessage, JTP_FONT_HEADLINE) + 
                   jtp_fonts[JTP_FONT_HEADLINE].lineheight + 
                   jtp_fonts[JTP_FONT_MENU].baseline,
                 JTP_FONT_HEADLINE,
                 15,
                 "(type any key)",
                 jtp_screen.vpage);
  }

  /* Display window */
  jtp_refresh();
  /* If the palette is faded out for some reason, restore it */
  jtp_updatepal(0, 255);

  /* Non-direction query, any key accepted */
  if ((nanswers == 0) && (!is_direction_query))
  {
    while (nchoice < 0)
    {
      /* Wait for mouse click or key press */
      jtp_keymouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
      if (jtp_kbhit())      
        nchoice = jtp_getch();
      else if (jtp_mouseb != JTP_MBUTTON_NONE)
      {
        /* Find out if the mouse was clicked on a button */
        for (i = 0; i < nbuttons; i++)
          if (jtp_mouse_area(pchoices[i].x, pchoices[i].y,
                             pchoices[i].x+pchoices[i].width-1,
                             pchoices[i].y+pchoices[i].height-1))
          {
            /* Wait until mouse button is released */
            jtp_press_button(pchoices[i].x+1, pchoices[i].y+1,
                             pchoices[i].x+pchoices[i].width-2,
                             pchoices[i].y+pchoices[i].height-2,
                             jtp_mcursor[JTP_CURSOR_NORMAL]);
            nchoice = pchoices[i].accelerator;
          }
      }
    }
  }

  /* Direction query */
  else if ((nanswers == 0) && (is_direction_query))
  {
    /* Ask for input */
    while (nchoice < 0)
    {
      /* Wait for mouse click or key press */
      jtp_keymouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
      if (jtp_kbhit())
      {
        nchoice = jtp_getch();
        /* swap the keys to match the isometric viewpoint. */
        nchoice = jtp_translate_key(nchoice);
      }
      else if (jtp_mouseb != JTP_MBUTTON_NONE)
      {
        /* Find out if the mouse was clicked on a direction arrow */

        /* 
         * Find the direction square under the mouse cursor.
         * The isometric mapping is a matrix operation, y = Ax+b, where the y are screen 
         * coordinates, the x are map square indices and A, b are constant. 
         * Here we use an inverse mapping to find map indices from pixel coordinates.
         */
        i = jtp_mousex - directions_x - directions_width/2;
        j = jtp_mousey - directions_y - directions_height/2;
        direction_x = JTP_MAP_YMOD*i + JTP_MAP_XMOD*j + JTP_MAP_XMOD*JTP_MAP_YMOD;
        direction_x = direction_x/(2*JTP_MAP_XMOD*JTP_MAP_YMOD) - (direction_x < 0);
        direction_y = -JTP_MAP_YMOD*i + JTP_MAP_XMOD*j + JTP_MAP_XMOD*JTP_MAP_YMOD;
        direction_y = direction_y/(2*JTP_MAP_XMOD*JTP_MAP_YMOD) - (direction_y < 0);

        /*         
           sprintf(tempbuffer, "mx=%d my=%d i=%d j=%d dx=%d dy=%d\n", jtp_mousex, jtp_mousey, i, j, direction_x, direction_y);
           jtp_write_log_message(tempbuffer); 
        */

        /* Select a direction command */
        nchoice = -1;
        if (direction_y == -1)
        { 
          if (direction_x == -1) nchoice = jtp_translate_command(JTP_NHCMD_NORTHWEST);
          else if (direction_x == 0) nchoice = jtp_translate_command(JTP_NHCMD_NORTH);
          else if (direction_x == 1) nchoice = jtp_translate_command(JTP_NHCMD_NORTHEAST);
        }
        else if (direction_y == 0)
        { 
          if (direction_x == -1) nchoice = jtp_translate_command(JTP_NHCMD_WEST);
          else if (direction_x == 0) nchoice = jtp_translate_command(JTP_NHCMD_REST);
          else if (direction_x == 1) nchoice = jtp_translate_command(JTP_NHCMD_EAST);
        }
        if (direction_y == 1)
        { 
          if (direction_x == -1) nchoice = jtp_translate_command(JTP_NHCMD_SOUTHWEST);
          else if (direction_x == 0) nchoice = jtp_translate_command(JTP_NHCMD_SOUTH);
          else if (direction_x == 1) nchoice = jtp_translate_command(JTP_NHCMD_SOUTHEAST);
        }

        /* Wait until mouse button is released */
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      }
    }
  }

  /* Normal query */
  else
  {
    /* Ask for input */
    while (nchoice < 0)
    {
      /* Wait for mouse click or key press */
      jtp_keymouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
      if (jtp_kbhit())
      {
        pressedkey = jtp_getch();
        if (nanswers == 1) nchoice = 0;
        else
        {
          for (i = 0; i < nbuttons; i++)
            if (pchoices[i].accelerator == pressedkey)
            {
              /* Wait until mouse button is released */
              jtp_press_button(pchoices[i].x+1, pchoices[i].y+1,
                               pchoices[i].x+pchoices[i].width-2,
                               pchoices[i].y+pchoices[i].height-2,
                               jtp_mcursor[JTP_CURSOR_NORMAL]);
              nchoice = i;        
            }
        }    
      }
      else if (jtp_mouseb != JTP_MBUTTON_NONE)
      {
        for (i = 0; i < nbuttons; i++)
          if (jtp_mouse_area(pchoices[i].x, pchoices[i].y,
                             pchoices[i].x+pchoices[i].width-1,
                             pchoices[i].y+pchoices[i].height-1))
          {
            /* Wait until mouse button is released */
            jtp_press_button(pchoices[i].x+1, pchoices[i].y+1,
                             pchoices[i].x+pchoices[i].width-2,
                             pchoices[i].y+pchoices[i].height-2,
                             jtp_mcursor[JTP_CURSOR_NORMAL]);
            nchoice = i;
          }
        if ((nchoice < 0) && (is_dropdown))
        {
          /* Clean up */
          free(query_background);
          for (i = 0; i < nbuttons; i++)
            free(pchoices[i].text);
          free(pchoices);
          return(-1);
        }     
      }
    }
  }

  /* Restore background */
  jtp_put_img(query_x, query_y, query_background);
  jtp_refresh();

  /* Clean up */
  free(query_background);
  for (i = 0; i < nbuttons; i++)
    free(pchoices[i].text);
  free(pchoices);

  /* Return the chosen answer */
  return(nchoice);
}


int jtp_dropdown
(
  int qx, int qy,                 /* Dropdown menu position */
  int nanswers,                   /* Number of menu items */
  jtp_dropdown_action ** panswers /* Menu item details */
)
{
  int i, j, k, l;
  int query_x, query_y;
  int totalwidth, totalheight;
  int ncolumns, column_height;
  int nbuttons, buttons_width, buttons_height;
  int nchoice = -1;
  jtp_button * pchoices;
  unsigned char * query_background;
  char pressedkey;

  while (jtp_kbhit()) jtp_getch();


  /* Find out how many of the menu items are buttons (nonzero id) */
  nbuttons = 0;
  for (i = 0; i < nanswers; i++)
    if ((panswers[i])->action_id != 0)
      nbuttons++;
  if (nbuttons <= 0) return(0);


  /* Create button table */
  pchoices = (jtp_button *)malloc(nbuttons*sizeof(jtp_button));
  if (pchoices == NULL)
  {
    jtp_write_log_message("[jtp_win.c/jtp_dropdown/Check1] ERROR: Out of memory!\n");
    return(0);
  }


  /* Fill button table */  
  j = 0; 
  for (i = 0; i < nanswers; i++)
  {
    if ((panswers[i])->action_id != 0)
    {
      pchoices[j].id = (panswers[i])->action_id;
      pchoices[j].text = (panswers[i])->str;
      pchoices[j].width = jtp_text_length(pchoices[j].text, JTP_FONT_BUTTON) + 11;
      pchoices[j].height = jtp_fonts[JTP_FONT_BUTTON].lineheight + 10;
      if (nbuttons == 1) pchoices[j].accelerator = '\r'; /* Enter */
      else pchoices[j].accelerator = pchoices[j].text[0];
      j++;
    }
  }


  /* Calculate width and height of dropdown menu */
  totalwidth = 0;
  totalheight = 0;
  j = 0;
  ncolumns = 1; column_height = 0;
  for (i = 0; i < nanswers; i++)
  {
    if ((panswers[i])->action_id != 0)
    {
      if (pchoices[j].width > totalwidth) totalwidth = pchoices[j].width;
      l = pchoices[j].height;
      if (column_height + l < jtp_screen.height)
        column_height += l;
      else
      {
        ncolumns++;
        if (column_height > totalheight) totalheight = column_height;
        column_height = l;
      }
      j++;
    }
    else
    {
      k = jtp_text_length((panswers[i])->str, JTP_FONT_HEADLINE);
      if (k > totalwidth) totalwidth = k;
      l = jtp_text_height((panswers[i])->str, JTP_FONT_HEADLINE);
      if (column_height + l < jtp_screen.height)
        column_height += l;
      else
      {
        ncolumns++;
        if (column_height > totalheight) totalheight = column_height;
        column_height = l;
      }
    }
  }


  buttons_width = totalwidth;
  totalwidth *= ncolumns;
  totalwidth += 6;
  if (column_height > totalheight) totalheight = column_height;
  totalheight += 6;
  
  /* Calculate position of dropdown menu */
  if (qx < 0) query_x = (jtp_screen.width - totalwidth) / 2;
  else if (qx < jtp_screen.width - totalwidth) query_x = qx;
  else query_x = jtp_screen.width - totalwidth;

  if (qy < 0) query_y = (jtp_screen.height - totalheight) / 2;
  else if (qy < jtp_screen.height - totalheight) query_y = qy;
  else query_y = jtp_screen.height - totalheight;

  /* Store background graphics */
  query_background = jtp_draw_dropdown_window(query_x, query_y, totalwidth, totalheight);

  /* Calculate menuitem positions and draw them */
  j = 0;
  k = query_x + 3;
  l = query_y + 3;  
  for (i = 0; i < nanswers; i++)
  {
    if ((panswers[i])->action_id != 0)
    {
      pchoices[j].x = k;
      pchoices[j].y = l;
      pchoices[j].width = buttons_width;
      jtp_draw_button(pchoices[j].x, pchoices[j].y,
                      pchoices[j].width, pchoices[j].height,
                      pchoices[j].text);
      l += pchoices[j].height;
      if (l >= query_y + totalheight - 3)
      {
        l = query_y + 3;
        k += buttons_width;
      }
      j++;
    }
    else
    {
      /* Draw query message */
      jtp_put_text(k + 1,
                   l + jtp_fonts[JTP_FONT_HEADLINE].baseline + 1,
                   JTP_FONT_HEADLINE,
                   0,
                   (panswers[i])->str,
                   jtp_screen.vpage);
      jtp_put_text(k,
                   l + jtp_fonts[JTP_FONT_HEADLINE].baseline,
                   JTP_FONT_HEADLINE,
                   15,
                   (panswers[i])->str,
                   jtp_screen.vpage);
      l += jtp_text_height((panswers[i])->str, JTP_FONT_HEADLINE);
      if (l >= query_y + totalheight - 3)
      {
        l = query_y + 3;
        k += buttons_width;
      }
    }
  }

  /* Display window */
  jtp_refresh();
  /* If the palette is faded out for some reason, restore it */
  jtp_updatepal(0, 255);

  if (nanswers == 0)
  {
    nchoice = jtp_getch();
  }  
  else
  {
    /* Ask for input */
    while (nchoice < 0)
    {
      /* Wait for mouse click or key press */
      jtp_keymouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
      if (jtp_kbhit())
      {
        pressedkey = jtp_getch();
        if (nanswers == 1) nchoice = 0;
        else
        {
          for (i = 0; i < nbuttons; i++)
            if (pchoices[i].accelerator == pressedkey)
            {
              /* Wait until mouse button is released */
              jtp_press_button(pchoices[i].x+1, pchoices[i].y+1,
                               pchoices[i].x+pchoices[i].width-2,
                               pchoices[i].y+pchoices[i].height-2,
                               jtp_mcursor[JTP_CURSOR_NORMAL]);
              nchoice = i;        
            }
        }    
      }
      else if (jtp_mouseb != JTP_MBUTTON_NONE)
      {
        for (i = 0; i < nbuttons; i++)
          if (jtp_mouse_area(pchoices[i].x, pchoices[i].y,
                             pchoices[i].x+pchoices[i].width-1,
                             pchoices[i].y+pchoices[i].height-1))
          {
            /* Wait until mouse button is released */
            jtp_press_button(pchoices[i].x+1, pchoices[i].y+1,
                             pchoices[i].x+pchoices[i].width-2,
                             pchoices[i].y+pchoices[i].height-2,
                             jtp_mcursor[JTP_CURSOR_NORMAL]);
            nchoice = i;
          }
        if (nchoice < 0) nchoice = nbuttons+1;
      }
    }
  }

  /* Restore background */
  jtp_put_img(query_x, query_y, query_background);
  jtp_refresh();

  if (nchoice < nbuttons) i = pchoices[nchoice].id;
  else i = 0;
  
  /* Clean up */
  free(query_background);
  free(pchoices);

  /* Return the chosen answer */
  return(i);
}


void jtp_messagebox
(
  const char *message /* Message to show player */
)
{
  jtp_query(-1, -1, message, 1, "Continue", 0);
}


void jtp_show_logo_screen()
{
  int banner_height, banner_width;
  int banner_y;

  jtp_load_PCX((jtp_screen.width-JTP_NH_LOGO_WIDTH)/2, 
               (jtp_screen.height-JTP_NH_LOGO_HEIGHT)/2, 
               jtp_filenames[JTP_FILE_NETHACK_LOGO], 1);
  jtp_blankpal(0, 255);
  jtp_refresh();
  jtp_game_palette_set = 0; 

  jtp_play_event_sound("nhfe_music_main_title");
  /* jtp_play_midi_song(jtp_filenames[JTP_SONG_TITLE]); */
 
  jtp_fade_in(1.0);
  jtp_getch();
  jtp_fade_out(0.3);
}


void jtp_view_messages()
{
  char tempbuffer[1024];
  winid window;
  jtp_window * msgwindow;
  int i, msgtime;
  
  msgwindow = jtp_find_window(WIN_MESSAGE);
  if (!msgwindow) return;
  
  /* 
   * Make a NHW_MENU window, add each message as a (textual) menu item.
   * This is a very unoptimized way of showing a text file ...
   */
  window = jtp_create_nhwindow(NHW_MENU);
  for (i = 0; i < msgwindow->curs_rows; i++)
  {
    if (msgwindow->rows[i])
    {
      msgtime = ((int *)msgwindow->rows[i])[0];
      sprintf(tempbuffer, "T:%d %s", msgtime, (char *)(msgwindow->rows[i]) + sizeof(int));
      jtp_putstr(window, ATR_NONE, tempbuffer);
    }  
  }  
  
  /* Display the messages */
  jtp_display_nhwindow(window, TRUE);
  
  /* Clean up */
  jtp_destroy_nhwindow(window);
}


void jtp_view_map()
{
  int x, y;
  int map_x, map_y;
  int i, j;
  int cur_glyph, cur_symbol;
  unsigned char * parchment_bg;
  char * temp_tooltip;
  int n_hotspots;
  jtp_hotspot ** map_hotspots;
  
  /* Find upper left corner of parchment */
  x = (jtp_screen.width - 640)/2;
  y = (jtp_screen.height - 480)/2;

  parchment_bg = jtp_get_img(x, y, x + 640-1, y + 480-1);

  /* Draw parchment */
  jtp_put_stencil(x, y, jtp_map_parchment_top);
  jtp_put_stencil(x, y + 480 - jtp_map_parchment_bottom[1], jtp_map_parchment_bottom);
  jtp_put_stencil(x, y, jtp_map_parchment_left);
  jtp_put_stencil(x + 640 - jtp_map_parchment_right[3], y, jtp_map_parchment_right);
  jtp_put_img(x + jtp_map_parchment_left[3], y + jtp_map_parchment_top[1], jtp_map_parchment_center);

  /* Find upper left corner of map on parchment */
  map_x = x + 39;
  map_y = y + 91;
  
  /* Draw map on parchment, and create hotspots */
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
    for (j = 1; j < JTP_MAP_WIDTH; j++)
    {
      cur_glyph = jtp_mapglyph_cmap[i][j];
      cur_symbol = jtp_cmap_to_map_symbol(cur_glyph);
/*      
      if (cur_symbol == JTP_MAP_SYMBOL_WALL)
      {
        if (i > 0)
        {
          cur_glyph = jtp_glyph_to_map_symbol[i][j]
        }
      }
      else
*/ 
      if (cur_symbol >= 0)       
        jtp_put_img(map_x + 7*j, map_y + 14*i, jtp_map_symbols[cur_symbol]);
      
      cur_glyph = jtp_mapglyph_obj[i][j];
      cur_symbol = jtp_object_to_map_symbol(cur_glyph);
      if (cur_symbol >= 0) jtp_put_img(map_x + 7*j, map_y + 14*i, jtp_map_symbols[cur_symbol]);
      
      cur_glyph = jtp_mapglyph_mon[i][j];
      cur_symbol = jtp_monster_to_map_symbol(cur_glyph);
      if (cur_symbol >= 0) jtp_put_img(map_x + 7*j, map_y + 14*i, jtp_map_symbols[cur_symbol]);
    }

  /* DEBUG: show center of map window */
  jtp_fill_rect(map_x + 7*jtp_map_x, map_y + 14*jtp_map_y,
                map_x + 7*jtp_map_x + 6, map_y + 14*jtp_map_y + 13,
                15);

  /* Create hotspots */
  n_hotspots = 0;
  map_hotspots = NULL;
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
    for (j = 1; j < JTP_MAP_WIDTH; j++)
    {
      temp_tooltip = jtp_choose_target_tooltip(j, i);
      if (temp_tooltip)
      {
        n_hotspots++;
        map_hotspots = (jtp_hotspot **)realloc(map_hotspots, n_hotspots*sizeof(jtp_hotspot *));
        map_hotspots[n_hotspots-1] = (jtp_hotspot *)malloc(sizeof(jtp_hotspot));
        (map_hotspots[n_hotspots-1])->mcursor = jtp_mcursor[JTP_CURSOR_NORMAL];
        (map_hotspots[n_hotspots-1])->tooltip = temp_tooltip;
        (map_hotspots[n_hotspots-1])->x1 = map_x + 7*j;
        (map_hotspots[n_hotspots-1])->x2 = map_x + 7*j + 6;
        (map_hotspots[n_hotspots-1])->y1 = map_y + 14*i;
        (map_hotspots[n_hotspots-1])->y2 = map_y + 14*i + 13;
        (map_hotspots[n_hotspots-1])->accelerator = 0;
      }
    }

  
  jtp_refresh();
  /* Wait for mouse button release, then wait for another mouse button click */
  jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
  jtp_get_mouse_inventory_input(jtp_mcursor[JTP_CURSOR_NORMAL], map_hotspots, n_hotspots, JTP_MBUTTON_LEFT);
  jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);

/*  if (jtp_kbhit()) { x=jtp_getch(); jtp_save_screenshot("scree000.pcx"); } */

  /* Restore background and clean up */
  jtp_put_img(x, y, parchment_bg);
  jtp_refresh();
  free(parchment_bg);
  if (n_hotspots > 0)
  {
    for (i = 0; i < n_hotspots; i++)
    {
      free((map_hotspots[i])->tooltip);
      free(map_hotspots[i]);
    }
    free(map_hotspots);
  }
}


void jtp_show_intro(char * introscript_name)
{
  FILE   * f;
  int      i, j;
  int      nScenes;
  int    * subtitle_rows;
  char *** subtitles;
  char  ** scene_images;
  char   * tempbuffer;
  clock_t  start_clock, cur_clock;
  
  nScenes = 0;
  scene_images = NULL;
  subtitle_rows = NULL;
  subtitles = NULL;
  tempbuffer = (char *)malloc(1024);
  if (!tempbuffer)
  {
    jtp_write_log_message("[jtp_win.c/jtp_show_intro/Check1] Out of memory!\n");
    jtp_exit_graphics(); exit(1);
  }

  jtp_play_event_sound("nhfe_music_introduction");
  /* jtp_play_midi_song(jtp_filenames[JTP_SONG_INTRO]); */
  
  f = fopen(introscript_name, "rb");
  while (fgets(tempbuffer, 1024, f))
  {
    if (tempbuffer[0] == '%') /* Start of new scene */
    {
      nScenes++;
      scene_images = (char **)realloc(scene_images, nScenes*sizeof(char *));
      i = 1; 
      while((tempbuffer[i] != '\0') && (tempbuffer[i] != 10) && 
            (tempbuffer[i] != 13) && (tempbuffer[i] != ' ')) i++;                    
      scene_images[nScenes-1] = (char *)malloc(i);
      if (!scene_images[nScenes-1])
      {
        jtp_write_log_message("[jtp_win.c/jtp_show_intro/Check2] Out of memory!\n");
        jtp_exit_graphics(); exit(1);
      }      
      memcpy(scene_images[nScenes-1], tempbuffer+1, i-1);
      scene_images[nScenes-1][i-1] = '\0';
      /* DEBUG printf("%s %s", tempbuffer, scene_images[nScenes-1]); DEBUG */
      
      subtitle_rows = (int *)realloc(subtitle_rows, nScenes*sizeof(int));
      subtitle_rows[nScenes-1] = 0;
      subtitles = (char ***)realloc(subtitles, nScenes*sizeof(char **));
      subtitles[nScenes-1] = NULL;
    }
    else /* New subtitle line for latest scene */
    {
      subtitle_rows[nScenes-1]++;
      subtitles[nScenes-1] = (char **)realloc(subtitles[nScenes-1],
           subtitle_rows[nScenes-1]*sizeof(char *));
      /* Remove extra whitespace from line */
      i = strlen(tempbuffer)-1; 
      while ((i >= 0) && ((tempbuffer[i] == ' ')||(tempbuffer[i] == '\n')||(tempbuffer[i]=='\r')))
        i--;
      tempbuffer[i+1] = '\0';
      i = 0; while (tempbuffer[i] == ' ') i++;
      /* Copy line to subtitle array */
      subtitles[nScenes-1][subtitle_rows[nScenes-1]-1] = (char *)malloc(strlen(tempbuffer+i)+1);
      if (!subtitles[nScenes-1][subtitle_rows[nScenes-1]-1])
      {
        jtp_write_log_message("[jtp_win.c/jtp_show_intro/Check3] Out of memory!\n");
        jtp_exit_graphics(); exit(1); 
      }
      strcpy(subtitles[nScenes-1][subtitle_rows[nScenes-1]-1], tempbuffer+i);
      /* DEBUG printf("%s", subtitles[nScenes-1][subtitle_rows[nScenes-1]-1]); DEBUG */
    }
  }
  fclose(f);
  jtp_blankpal(0, 255);
  jtp_game_palette_set = 0; 
 
  /*
   * Show each scene of the introduction in four steps:
   * - Erase previous image, load and fade in new image
   * - Print the subtitles
   * - Wait out a set delay
   * - Erase subtitles, Fade out 
   */
  jtp_set_draw_region(0, 0, jtp_screen.width-1, jtp_screen.height-1); 
  for (i = 0; i < nScenes; i++)
  {
    /* If we are starting, or the previous image was different, fade in the current image */
    if ((i <= 0) || (strcmp(scene_images[i], scene_images[i-1]) != 0))
    {  
      sprintf(tempbuffer, "%s%s", JTP_GRAPHICS_DIRECTORY, scene_images[i]);
      jtp_clear_screen();
      jtp_load_PCX((jtp_screen.width-JTP_INTRO_SLIDE_WIDTH)/2, 
                   (jtp_screen.height-JTP_INTRO_SLIDE_HEIGHT)/6, 
                   tempbuffer, 1);
      jtp_refresh();
      jtp_fade_in(0.2);
    }  
    
    /* Show subtitles */
    for (j = 0; j < subtitle_rows[i]; j++)
    {
      jtp_put_text((jtp_screen.width-jtp_text_length(subtitles[i][j], JTP_FONT_INTRO))/2,
                      2*(jtp_screen.height-JTP_INTRO_SLIDE_HEIGHT)/6 + JTP_INTRO_SLIDE_HEIGHT +
                      j*jtp_fonts[JTP_FONT_INTRO].lineheight,
                   JTP_FONT_INTRO, 255,
                   subtitles[i][j],
                   jtp_screen.vpage);
    }
    jtp_refresh();
    
    /* Wait until scene is over or player pressed a key */
    start_clock = clock();
    cur_clock = start_clock;
    while (cur_clock-start_clock < 5*CLOCKS_PER_SEC)
    {
      cur_clock = clock();
      if (jtp_kbhit()) 
      {
        jtp_getch();
        cur_clock = start_clock + 7*CLOCKS_PER_SEC;
        i = nScenes;
      }  
    }

    /* Erase subtitles */
                      2*(jtp_screen.height-JTP_INTRO_SLIDE_HEIGHT)/6 + JTP_INTRO_SLIDE_HEIGHT +
                      j*jtp_fonts[JTP_FONT_INTRO].lineheight,
    
    jtp_fill_rect(0, (jtp_screen.height-JTP_INTRO_SLIDE_HEIGHT)/6 + JTP_INTRO_SLIDE_HEIGHT, 
                  jtp_screen.width-1, jtp_screen.height-1, 0);
    jtp_refresh();

    /* If we are at the end, or the next image is different, fade out the current image */
    if ((i >= nScenes-1) || (strcmp(scene_images[i], scene_images[i+1]) != 0))
      jtp_fade_out(0.2);
  }
  
  /* Clean up */

  /* Create "NetHack: continue game after dying - marker file" */
/*
  f = fopen("nhcgadmf.tmp", "wb");
  fprintf(f, "Temporary file for NetHack - Vulture's Eye.\nYou can safely delete this file.\n");
  fclose(f);
*/
}


void jtp_select_player()
{
  int i, j, k, n;
  winid win;
  anything any;
  menu_item *selected = 0;
  char thisch, lastch;
  char pbuf[1024];


  jtp_clear_screen();
  jtp_load_PCX((jtp_screen.width-JTP_NH_LOGO_WIDTH)/2, 
               (jtp_screen.height-JTP_NH_LOGO_HEIGHT)/2, 
               jtp_filenames[JTP_FILE_CHARACTER_GENERATION], 1);
  jtp_blankpal(0, 255);
  jtp_refresh();
  jtp_game_palette_set = 0;

  jtp_fade_in(0.2);

  /* Select a role, if necessary */
  /* we'll try to be compatible with pre-selected race/gender/alignment,
   * but may not succeed */
  if (flags.initrole < 0) 
  {
    /* Process the choice */
    if (flags.initrole == ROLE_RANDOM) 
    {
	/* Pick a random role */
	flags.initrole = pick_role(flags.initrace, flags.initgend, flags.initalign, PICK_RANDOM);
	if (flags.initrole < 0) 
      {
	  tty_putstr(BASE_WINDOW, 0, "Incompatible role!");
	  flags.initrole = randrole();
	}
    } 
    else 
    {
      /* Prompt for a role */
      win = create_nhwindow(NHW_MENU);
      start_menu(win);
      any.a_void = 0;         /* zero out all bits */
      for (i = 0; roles[i].name.m; i++) 
      {
        any.a_int = i+1;	/* must be non-zero */
        thisch = lowc(roles[i].name.m[0]);
        if (thisch == lastch) thisch = highc(thisch);
        add_menu(win, NO_GLYPH, &any, thisch, 0, ATR_NONE, an(roles[i].name.m), MENU_UNSELECTED);
        lastch = thisch;
      }
      any.a_int = randrole()+1; /* must be non-zero */
      add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE, "Random", MENU_SELECTED);
      any.a_int = i+1; /* must be non-zero */
      add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE, "Quit", MENU_UNSELECTED);
      end_menu(win, "Pick a role");
      n = select_menu(win, PICK_ONE, &selected);
      destroy_nhwindow(win);

      /* Process the choice */
      if (n != 1 || selected[0].item.a_int == any.a_int)
      {
        /* Selected quit */
        if (selected) free((genericptr_t) selected);
        bail((char *)0);
        /*NOTREACHED*/
      }  
      flags.initrole = selected[0].item.a_int - 1;
      free((genericptr_t) selected);
      selected = 0;
    }
  }

  /* Select a race, if necessary */
  /* force compatibility with role, try for compatibility with
   * pre-selected gender/alignment */
  if (flags.initrace < 0 || !validrace(flags.initrole, flags.initrace)) 
  {
    /* pre-selected race not valid */
    if (flags.initrace == ROLE_RANDOM) 
    {
	flags.initrace = pick_race(flags.initrole, flags.initgend, flags.initalign, PICK_RANDOM);
	if (flags.initrace < 0) 
      {
	  jtp_messagebox("The selected race is incompatible with other options.\nChoosing a random race.\n");
        flags.initrace = randrace(flags.initrole);
	}
    }
    else
    {
      /* Select a race */
      if (!validrace(flags.initrole, flags.initrace))
        i = j = k = randrace(flags.initrole);
      else
        i = j = k = flags.initrace;

      /* Count the number of valid races */
      n = 0;
      do 
      {
        if (validrace(flags.initrole, i)) n++;
        else if ((i == k) && (!races[++k].noun)) k = 0;
    
        if (!races[++i].noun) i = 0;
      } while (i != j);
  
      /* Permit the user to pick a race */
      if (!validrace(flags.initrole, flags.initrace)) 
      {
        win = create_nhwindow(NHW_MENU);
        start_menu(win);
       any.a_void = 0;         /* zero out all bits */
        for (i = 0; races[i].noun; i++)
          if (validrace(flags.initrole, i)) 
          {
            any.a_int = i+1; /* must be non-zero */
            add_menu(win, NO_GLYPH, &any, races[i].noun[0], 0, ATR_NONE, races[i].noun, MENU_UNSELECTED);
          }
        any.a_int = randrace(flags.initrole)+1;
        add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE, "Random", MENU_SELECTED);
        any.a_int = i+1; /* must be non-zero */
        add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE, "Quit", MENU_UNSELECTED);
        Sprintf(pbuf, "Pick the race of your %s", roles[flags.initrole].name.m);
        end_menu(win, pbuf);
        n = select_menu(win, PICK_ONE, &selected);
        destroy_nhwindow(win);
        if (n != 1 || selected[0].item.a_int == any.a_int)
        {
          /* Selected quit */
          if (selected) free((genericptr_t) selected);
          bail((char *)0);
          /*NOTREACHED*/
        }

        k = selected[0].item.a_int - 1;
        free((genericptr_t) selected);
        selected = 0;
      }
      flags.initrace = k;
    }
  }


  /* Select a gender, if necessary */
  /* force compatibility with role/race, try for compatibility with
   * pre-selected alignment */
  if (flags.initgend < 0 || !validgend(flags.initrole, flags.initrace, flags.initgend)) 
  {
    if (flags.initgend == ROLE_RANDOM) 
    {
	flags.initgend = pick_gend(flags.initrole, flags.initrace, flags.initalign, PICK_RANDOM);
	if (flags.initgend < 0) 
      { 
	  jtp_messagebox("The selected gender is incompatible with other options.\nChoosing a random gender.\n");
        flags.initgend = randgend(flags.initrole, flags.initrace);
	}
    }
    else
    {
      /* Select a gender */
      if (!validgend(flags.initrole, flags.initrace, flags.initgend))
        /* Pick a random valid gender */
        i = j = k = randgend(flags.initrole, flags.initrace);
      else
        i = j = k = flags.initgend;
    
      /* Count the number of valid genders */
      n = 0;
      do 
      {
        if (validgend(flags.initrole, flags.initrace, i)) n++;
        else if ((i == k) && (++k >= ROLE_GENDERS)) k = 0;
        
        if (++i >= ROLE_GENDERS) i = 0;
      } while (i != j);
    
      /* Permit the user to pick a gender */
      if (!validgend(flags.initrole, flags.initrace, flags.initgend)) 
      {
        win = create_nhwindow(NHW_MENU);
        start_menu(win);
        any.a_void = 0;         /* zero out all bits */
        for (i = 0; i < ROLE_GENDERS; i++)
          if (validgend(flags.initrole, flags.initrace, i)) 
          {
            any.a_int = i+1; /* must be non-zero */
            add_menu(win, NO_GLYPH, &any, genders[i].adj[0], 0, ATR_NONE, genders[i].adj, MENU_UNSELECTED);
          }
        any.a_int = randgend(flags.initrole, flags.initrace)+1;
        add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE, "Random", MENU_SELECTED);
        any.a_int = i+1; /* must be non-zero */
        add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE, "Quit", MENU_UNSELECTED);
        sprintf(pbuf, "Pick the gender of your %s %s", races[flags.initrace].adj, roles[flags.initrole].name.m);
        end_menu(win, pbuf);
        n = select_menu(win, PICK_ONE, &selected);
        destroy_nhwindow(win);
        if (n != 1 || selected[0].item.a_int == any.a_int)
        {
          /* Selected quit */
          if (selected) free((genericptr_t) selected);
          bail((char *)0);
          /*NOTREACHED*/
        }
  
        k = selected[0].item.a_int - 1;
        free((genericptr_t) selected);
        selected = 0;
      }
      flags.initgend = k;
    }
  }  

  /* Select an alignment, if necessary */
  /* force compatibility with role/race/gender */
  if (flags.initalign < 0 || !validalign(flags.initrole, flags.initrace, flags.initalign)) 
  {
    if (flags.initalign == ROLE_RANDOM) 
    {
	flags.initalign = pick_align(flags.initrole, flags.initrace, flags.initgend, PICK_RANDOM);
	if (flags.initalign < 0) 
      {
	  jtp_messagebox("The selected alignment is incompatible with other options.\nChoosing a random alignment.\n");
        flags.initalign = randalign(flags.initrole, flags.initrace);
	} 
    }
    else
    {
      /* Select an alignment */
      if (!validalign(flags.initrole, flags.initrace, flags.initalign))
        /* Pick a random valid alignment */
        i = j = k = randalign(flags.initrole, flags.initrace);
      else
        i = j = k = flags.initalign;
      /* Count the number of valid alignments */
      n = 0;
      do 
      {
        if (validalign(flags.initrole, flags.initrace, i)) n++;
        else if ((i == k) && (++k >= ROLE_ALIGNS)) k = 0;
       
        if (++i >= ROLE_ALIGNS) i = 0;
      } while (i != j);
      
      /* Permit the user to pick, if there is more than one */
      if (!validalign(flags.initrole, flags.initrace, flags.initalign)) 
      {
        win = create_nhwindow(NHW_MENU);
        start_menu(win);
        any.a_void = 0;         /* zero out all bits */
        for (i = 0; i < ROLE_ALIGNS; i++)
          if (validalign(flags.initrole, flags.initrace, i)) 
          {
            any.a_int = i+1; /* must be non-zero */
            add_menu(win, NO_GLYPH, &any, aligns[i].adj[0], 0, ATR_NONE, aligns[i].adj, MENU_UNSELECTED);
          }
        any.a_int = randalign(flags.initrole, flags.initrace)+1;
        add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE, "Random", MENU_SELECTED);
        any.a_int = i+1; /* must be non-zero */
        add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE, "Quit", MENU_UNSELECTED);
        sprintf(pbuf, "Pick the alignment of your %s %s %s", 
            genders[flags.initgend].adj,
            races[flags.initrace].adj,
            (flags.initgend && roles[flags.initrole].name.f) ?
            roles[flags.initrole].name.f :
            roles[flags.initrole].name.m);
        end_menu(win, pbuf);
        n = select_menu(win, PICK_ONE, &selected);
        destroy_nhwindow(win);
        if (n != 1 || selected[0].item.a_int == any.a_int)
        {
          /* Selected quit */
          if (selected) free((genericptr_t) selected);
          bail((char *)0);
          /*NOTREACHED*/
        }

        k = selected[0].item.a_int - 1;
        free((genericptr_t) selected);
        selected = 0;
      }
      flags.initalign = k;
    }
  }
  
  
  /* Success! Show introduction. */
  jtp_fade_out(0.2);
  jtp_show_intro(jtp_filenames[JTP_FILE_INTRO_SCRIPT]);
  
  /* Restore regular game palette */  
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_MOUSE_CURSORS], 1);
  jtp_clear_screen();
  jtp_refresh();
  jtp_updatepal(0, 255);
  jtp_game_palette_set = 1;  

/*  jtp_display_nhwindow(BASE_WINDOW, FALSE);  */
}

int jtp_construct_shortcut_action
(
  int tgtx, int tgty, /* Target square, or item accelerator in tgtx */
  int action_id       /* Shortcut action type */
)
{
  /* jtp_write_log_message("[jtp_win.c/jtp_construct_shortcut_action/Debug1]\n"); */

  switch(action_id)
  {
    case JTP_ACTION_CHAT:
    case JTP_ACTION_KICK:
    case JTP_ACTION_CLOSE_DOOR:
    case JTP_ACTION_OPEN_DOOR:
    case JTP_ACTION_FORCE_LOCK:
    case JTP_ACTION_UNTRAP:
      jtp_is_shortcut_active = 1;
      if (tgtx == jtp_you_x-1)
      {
        if (tgty == jtp_you_y-1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_NORTHWEST);
        if (tgty == jtp_you_y) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_WEST);
        if (tgty == jtp_you_y+1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_SOUTHWEST);

      }
      else if (tgtx == jtp_you_x)
      {
        if (tgty == jtp_you_y-1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_NORTH);
        if (tgty == jtp_you_y) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_REST);
        if (tgty == jtp_you_y+1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_SOUTH);
      }
      if (tgtx == jtp_you_x+1)
      {
        if (tgty == jtp_you_y-1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_NORTHEAST);
        if (tgty == jtp_you_y) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_EAST);
        if (tgty == jtp_you_y+1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_SOUTHEAST);
      }
      break;
    case JTP_ACTION_WHATS_THIS:
      jtp_is_shortcut_active = 1;
      jtp_shortcut_query_response = 'y';
      break;
    case JTP_ACTION_EAT_ITEM:
    case JTP_ACTION_DRINK_ITEM:
    case JTP_ACTION_READ_ITEM:
    case JTP_ACTION_DROP_ITEM:
    case JTP_ACTION_WEAR_ITEM:
    case JTP_ACTION_PUT_ON_ITEM:
    case JTP_ACTION_WIELD_ITEM:
    case JTP_ACTION_APPLY_ITEM:
    case JTP_ACTION_ZAP_ITEM:
    case JTP_ACTION_REMOVE_ITEM:
    case JTP_ACTION_CAST_SPELL:
      jtp_is_shortcut_active = 1;
      jtp_shortcut_query_response = tgtx;
      break;
    default:
      jtp_is_shortcut_active = 0;
      break;
  }
  switch(action_id)
  {
    case JTP_ACTION_CLOSE_DOOR: return(jtp_translate_command(JTP_NHCMD_CLOSE_DOOR)); break;
    case JTP_ACTION_DRINK: return(jtp_translate_command(JTP_NHCMD_QUAFF)); break;
    case JTP_ACTION_ENGRAVE: return(jtp_translate_command(JTP_NHCMD_ENGRAVE)); break;
    case JTP_ACTION_GO_DOWN: return(jtp_translate_command(JTP_NHCMD_DOWN)); break;
    case JTP_ACTION_GO_UP: return(jtp_translate_command(JTP_NHCMD_UP)); break;
    case JTP_ACTION_KICK: return(jtp_translate_command(JTP_NHCMD_KICK)); break;
    case JTP_ACTION_OPEN_DOOR: return(jtp_translate_command(JTP_NHCMD_OPEN_DOOR)); break;
    case JTP_ACTION_PAY_BILL: return(jtp_translate_command(JTP_NHCMD_PAY_BILL)); break;
    case JTP_ACTION_PICK_UP: return(jtp_translate_command(JTP_NHCMD_PICKUP)); break;
    case JTP_ACTION_REST: return(jtp_translate_command(JTP_NHCMD_REST)); break;
    case JTP_ACTION_SEARCH: return(jtp_translate_command(JTP_NHCMD_SEARCH)); break;
    case JTP_ACTION_LOOK_AROUND: return(jtp_translate_command(JTP_NHCMD_LOOK_HERE)); break;

    case JTP_ACTION_CHAT: return(jtp_translate_command(JTP_NHCMD_CHAT)); break;
    case JTP_ACTION_FORCE_LOCK: return(jtp_translate_command(JTP_NHCMD_FORCE_LOCK)); break;
    case JTP_ACTION_LOOT: return(jtp_translate_command(JTP_NHCMD_LOOT)); break;
    case JTP_ACTION_MONSTER_ABILITY: return(jtp_translate_command(JTP_NHCMD_MONSTER_ABILITY)); break;
    case JTP_ACTION_PRAY: return(jtp_translate_command(JTP_NHCMD_PRAY)); break;
    case JTP_ACTION_SIT: return(jtp_translate_command(JTP_NHCMD_SIT)); break;
    case JTP_ACTION_TURN_UNDEAD: return(jtp_translate_command(JTP_NHCMD_TURN_UNDEAD)); break;
    case JTP_ACTION_UNTRAP: return(jtp_translate_command(JTP_NHCMD_UNTRAP)); break;
    case JTP_ACTION_WIPE_FACE: return(jtp_translate_command(JTP_NHCMD_WIPE_FACE)); break;

    case JTP_ACTION_ENTER_TRAP:
    case JTP_ACTION_PUSH_BOULDER:
      if (tgtx == jtp_you_x-1)
      {
        if (tgty == jtp_you_y-1) return(jtp_translate_command(JTP_NHCMD_NORTHWEST));
        if (tgty == jtp_you_y) return(jtp_translate_command(JTP_NHCMD_WEST));
        if (tgty == jtp_you_y+1) return(jtp_translate_command(JTP_NHCMD_SOUTHWEST));
      }
      else if (tgtx == jtp_you_x)
      {
        if (tgty == jtp_you_y-1) return(jtp_translate_command(JTP_NHCMD_NORTH));
        if (tgty == jtp_you_y) return(jtp_translate_command(JTP_NHCMD_REST));
        if (tgty == jtp_you_y+1) return(jtp_translate_command(JTP_NHCMD_SOUTH));
      }
      if (tgtx == jtp_you_x+1)
      {
        if (tgty == jtp_you_y-1) return(jtp_translate_command(JTP_NHCMD_NORTHEAST));
        if (tgty == jtp_you_y) return(jtp_translate_command(JTP_NHCMD_EAST));
        if (tgty == jtp_you_y+1) return(jtp_translate_command(JTP_NHCMD_SOUTHEAST));
      }
      break;

    case JTP_ACTION_MOVE_HERE:
        jtp_find_path(jtp_you_y, jtp_you_x, tgty, tgtx);
        return(0);
        break;

    case JTP_ACTION_WHATS_THIS:
        jtp_move_length = 2;
        jtp_autopilot_type = JTP_AUTOPILOT_WHATSTHIS;
        jtp_movebuffer[0] = tgty*JTP_MAP_WIDTH+tgtx;
        jtp_movebuffer[1] = -1;
        return(jtp_translate_command(JTP_NHCMD_EXPLAIN_SYMBOL));
        break;

    case JTP_ACTION_EAT_ITEM: return(jtp_translate_command(JTP_NHCMD_EAT)); break;
    case JTP_ACTION_DRINK_ITEM: return(jtp_translate_command(JTP_NHCMD_QUAFF)); break;
    case JTP_ACTION_READ_ITEM: return(jtp_translate_command(JTP_NHCMD_READ)); break;
    case JTP_ACTION_DROP_ITEM: return(jtp_translate_command(JTP_NHCMD_DROP)); break;
    case JTP_ACTION_WEAR_ITEM: return(jtp_translate_command(JTP_NHCMD_WEAR_ARMOR)); break;
    case JTP_ACTION_PUT_ON_ITEM: return(jtp_translate_command(JTP_NHCMD_PUT_ON_ACCESSORY)); break;
    case JTP_ACTION_WIELD_ITEM: return(jtp_translate_command(JTP_NHCMD_WIELD_WEAPON)); break;
    case JTP_ACTION_APPLY_ITEM: return(jtp_translate_command(JTP_NHCMD_APPLY)); break;
    case JTP_ACTION_ZAP_ITEM: return(jtp_translate_command(JTP_NHCMD_ZAP)); break;
    case JTP_ACTION_REMOVE_ITEM: return(jtp_translate_command(JTP_NHCMD_REMOVE_ITEM)); break;
    case JTP_ACTION_CAST_SPELL: return(jtp_translate_command(JTP_NHCMD_CAST_SPELL)); break;

    default: return(0); break;
  }
}


void jtp_add_dropdown_action
(
  int *n_actions,
  jtp_dropdown_action *** dropdown_actions,
  int action_id,
  char * action_str
)
{
  int i, j;
  jtp_dropdown_action * temp_action;


  i = *n_actions;
  *dropdown_actions = (jtp_dropdown_action **)realloc(*dropdown_actions, (i+1)*sizeof(jtp_dropdown_action *));
  if (*dropdown_actions == NULL)
  {
    jtp_write_log_message("[jtp_win.c/jtp_add_dropdown_action/Check1] ERROR: could not allocate memory.\n");
    return;
  }
  temp_action = (jtp_dropdown_action *)malloc(sizeof(jtp_dropdown_action));
  if (temp_action == NULL)
  {
    jtp_write_log_message("[jtp_win.c/jtp_add_dropdown_action/Check2] ERROR: could not allocate memory.\n");
    return;
  }

  temp_action->action_id = action_id;
  if (!action_str) temp_action->str = NULL;
  else
  {
    j = strlen(action_str);
    temp_action->str = malloc(j+1);
    if (temp_action->str == NULL)
    {
      jtp_write_log_message("[jtp_win.c/jtp_add_dropdown_action/Check3] ERROR: could not allocate memory.\n");
      return;
    }
    memcpy(temp_action->str, action_str, j+1);
  }
  (*dropdown_actions)[i] = temp_action;
  *n_actions = i+1;
}
  

int jtp_get_dropdown_command
(
  int mx, int my,
  int tgtx, int tgty
)
{
  jtp_dropdown_action ** dropdown_actions;
  int n_actions, selected_action;
  int mapglyph_offset;
  int i;
  char * mapsquare_descr;


  /* Dropdown commands are shown only for valid squares */
  if ((tgtx < 1) || (tgtx >= JTP_MAP_WIDTH) || (tgty < 0) || (tgty >= JTP_MAP_HEIGHT))
    return(0);

  /* Construct a context-sensitive drop-down menu */
  dropdown_actions = NULL;
  n_actions = 0;

  if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, 0, plname);
  else
  {
    mapsquare_descr = jtp_map_square_description(tgtx, tgty, 0);
    if (mapsquare_descr)
    {
      jtp_add_dropdown_action(&n_actions, &dropdown_actions, 0, mapsquare_descr);
      free(mapsquare_descr);
    }
  }

  if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))
  {
    /* Add personal options: */
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_ENGRAVE, "Engrave");
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_LOOK_AROUND, "Look around");
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_MONSTER_ABILITY, "Monster ability");
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_PAY_BILL, "Pay bill");
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_PRAY, "Pray");
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_REST, "Rest");
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_SEARCH, "Search");
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_SIT, "Sit");
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_TURN_UNDEAD, "Turn undead");
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_WIPE_FACE, "Wipe face");
  }
  else if (jtp_mapglyph_mon[tgty][tgtx] >= 0)
  {
    /* Add monster options: */
    if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
      if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
        {
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_CHAT, "Chat");
          /* jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_RIDE, "Ride"); */
        }
  }

  if (jtp_mapglyph_obj[tgty][tgtx] >= 0)
  {
    /* Add object options: */
    mapglyph_offset =  jtp_mapglyph_obj[tgty][tgtx];
    switch(mapglyph_offset)
    {
      case LARGE_BOX: case ICE_BOX: case CHEST:
        if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))
        {
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_FORCE_LOCK, "Force lock");
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_LOOT, "Loot");
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_PICK_UP, "Pick up");
        }
        if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_UNTRAP, "Untrap");
        break;
      case SACK: case OILSKIN_SACK: case BAG_OF_HOLDING: case BAG_OF_TRICKS:
        if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))
        {
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_LOOT, "Loot");
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_PICK_UP, "Pick up");
        }
        break;
      case BOULDER:
        if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
          if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_PUSH_BOULDER, "Push");
        break;
      default:
        if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))        
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_PICK_UP, "Pick up");
        break;      
    }
  }
  if (jtp_mapglyph_cmap[tgty][tgtx] >= 0)
  {
    /* Add cmap options: */
    mapglyph_offset =  jtp_mapglyph_cmap[tgty][tgtx];
    switch(mapglyph_offset)
    {
      case S_vodoor: case S_hodoor:
        if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
          if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
          {
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_CLOSE_DOOR, "Close door");
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_UNTRAP, "Untrap");
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
          }
        break;
      case S_vcdoor: case S_hcdoor:
        if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
          if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
          {
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_OPEN_DOOR, "Open door");
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_UNTRAP, "Untrap");
            /* jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_FORCE_LOCK, "Force lock"); */
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
          }
        break;
      case S_dnstair: case S_dnladder:
        if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_GO_DOWN, "Go down");
        if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
          if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
        break;
      case S_upstair: case S_upladder:
        if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_GO_UP, "Go up");
        if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
          if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
        break;
      case S_fountain:
        if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_DRINK, "Drink");
        if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
          if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
        break;
      default:
        if (glyph_is_trap(jtp_mapglyph_cmap[tgty][tgtx] + GLYPH_CMAP_OFF))
        {
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_UNTRAP, "Untrap");
          if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
            if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
              jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_ENTER_TRAP, "Enter trap");
        }
        if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
          if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
        break;
    }
    if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))    
      jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_MOVE_HERE, "Move here");
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_WHATS_THIS, "What's this?");
  }


  if (n_actions > 0)
  {
    selected_action = jtp_dropdown(mx, my, n_actions, dropdown_actions);
    for (i = 0; i < n_actions; i++)
    {
      free(dropdown_actions[i]->str);
      free(dropdown_actions[i]);
    }
    free(dropdown_actions);
  }


  if (selected_action == 0) return(0);
  else return(jtp_construct_shortcut_action(tgtx, tgty, selected_action));
}

int jtp_get_default_command
(
  int tgtx, int tgty
)
{
  int selected_action;
  int mapglyph_offset;
  char * mapsquare_descr;

  /* jtp_write_log_message("[jtp_win.c/jtp_get_default_command/Debug1]\n"); */

  /* Off-map squares have no default action */
  if ((tgtx < 1) || (tgtx >= JTP_MAP_WIDTH) ||
      (tgty < 0) || (tgty >= JTP_MAP_HEIGHT))
    return(0);

  /* Select a default command */
  selected_action = 0;

  if ((abs(jtp_you_x-tgtx) >= 2) || (abs(jtp_you_y-tgty) >= 2))
    selected_action = JTP_ACTION_MOVE_HERE;
  if ((!selected_action) && (jtp_mapglyph_mon[tgty][tgtx] >= 0))
  {
    if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
      selected_action = JTP_ACTION_MOVE_HERE;
  }
  if ((!selected_action) && (jtp_mapglyph_obj[tgty][tgtx] >= 0))
  {
    mapglyph_offset = jtp_mapglyph_cmap[tgty][tgtx];
    if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))
      switch(mapglyph_offset)
      {
        case LARGE_BOX: case ICE_BOX: case CHEST:
          selected_action = JTP_ACTION_LOOT; break;
        default:
          selected_action = JTP_ACTION_PICK_UP; break;
      }
    else selected_action = JTP_ACTION_MOVE_HERE;     
  }
  if ((!selected_action) && (jtp_mapglyph_cmap[tgty][tgtx] >= 0))
  {
    /* Add cmap options: */
    mapglyph_offset =  jtp_mapglyph_cmap[tgty][tgtx];
    if ((mapglyph_offset == S_dnstair) || (mapglyph_offset == S_dnladder))
      if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))
        selected_action = JTP_ACTION_GO_DOWN;
    if ((mapglyph_offset == S_upstair) || (mapglyph_offset == S_upladder))
      if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))
        selected_action = JTP_ACTION_GO_UP;
    if (mapglyph_offset == S_sink)
      if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
        if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
          selected_action = JTP_ACTION_KICK;
    if ((mapglyph_offset == S_vcdoor) || (mapglyph_offset == S_hcdoor))
      if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))
        if ((abs(jtp_you_x-tgtx) <= 1) && (abs(jtp_you_y-tgty) <= 1))
          selected_action = JTP_ACTION_OPEN_DOOR;
    if (mapglyph_offset == S_fountain)
      if ((jtp_you_x == tgtx) && (jtp_you_y == tgty))        
        selected_action = JTP_ACTION_DRINK;
    if (!selected_action)
      if ((jtp_you_x != tgtx) || (jtp_you_y != tgty))        
        selected_action = JTP_ACTION_MOVE_HERE;
  }
  if ((!selected_action) && (jtp_you_x == tgtx) && (jtp_you_y == tgty))
    selected_action = JTP_ACTION_SEARCH;

  if (!selected_action) return(0);
  else return(jtp_construct_shortcut_action(tgtx, tgty, selected_action));
}


void jtp_view_inventory()
{
  int x, y;
  int inven_x, inven_y;
  int total_selectable_items;
  int selectable_items;
  int i, j;
  int cur_glyph, cur_symbol;
  int quit_viewing_inventory;
  int inventory_page_changed;
  int pressedkey;
  unsigned char * backpack_bg;

  jtp_window * menuwindow;
  jtp_menu * menu;
  int firstitem_index;
  jtp_menuitem * tempmenuitem;
  int item_x, item_y, item_index, item_tile;

  jtp_hotspot ** inven_hotspots;
  int n_hotspots;
  int selected_hotspot;

  jtp_dropdown_action ** dropdown_actions;
  int n_actions;
  int selected_action;

  /* Find upper left corner of backpack */
  x = (jtp_screen.width - 640)/2;
  y = (jtp_screen.height - 480)/2;

  backpack_bg = jtp_get_img(x, y, x + 640-1, y + 480-1);

  /* Draw parchment */
  jtp_put_stencil(x, y, jtp_backpack_top);
  jtp_put_stencil(x, y + 480 - jtp_backpack_bottom[1], jtp_backpack_bottom);
  jtp_put_stencil(x, y, jtp_backpack_left);
  jtp_put_stencil(x + 640 - jtp_backpack_right[3], y, jtp_backpack_right);

  /* Find upper left corner of inventory on backpack */
  inven_x = x + 113;
  inven_y = y + 106;

  menuwindow = jtp_find_window(WIN_INVEN);
  if (!menuwindow) return;
  menu = menuwindow->menu;
  if ((!menu) || (!menu->items)) return;

  /* Find out total number of selectable items in inventory */
  total_selectable_items = 0;
  jtp_list_reset(menu->items);
  tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
  while (tempmenuitem)
  {
    if (tempmenuitem->count != JTP_NOT_SELECTABLE)     
      total_selectable_items++;
    jtp_list_advance(menu->items);
    tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
  }

  quit_viewing_inventory = 0;
  inventory_page_changed = 1;
  firstitem_index = 0;
  inven_hotspots = NULL;
  n_hotspots = 0;

  /* Wait for mouse button release, then display the inventory */
  jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);

  while (!quit_viewing_inventory)
  {
    if (inventory_page_changed)
    {
      /* Clean up previous hotspots */
      if (n_hotspots > 0)
      {
        for (i = 0; i < n_hotspots; i++)
        {
          free((inven_hotspots[i])->tooltip);
          free(inven_hotspots[i]);
        }
        free(inven_hotspots);
        inven_hotspots = NULL;
        n_hotspots = 0;
      }

      /* Draw inventory on backpack, and create hotspots */
      jtp_put_img(x + jtp_backpack_left[3], y + jtp_backpack_top[1], jtp_backpack_center);

      /* Find the first selectable item to be displayed */
      item_index = -1;
      jtp_list_reset(menu->items);
      tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
      while ((tempmenuitem) && (item_index < firstitem_index))
      {
        if (tempmenuitem->count != JTP_NOT_SELECTABLE)
          item_index++;
        if (item_index < firstitem_index)
        {
          jtp_list_advance(menu->items);
          tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
        }
      }
      
      item_x = inven_x;
      item_y = inven_y;
      selectable_items = 0; /* None shown so far */
      while ((tempmenuitem) && (selectable_items < 25))
      {
        if (tempmenuitem->count != JTP_NOT_SELECTABLE)     
        {
          item_tile = jtp_object_to_tile(glyph_to_obj(tempmenuitem->glyph));
          if ((item_tile != JTP_TILE_INVALID) && (jtp_tiles[item_tile]))
            jtp_put_tile(item_x + jtp_tiles[item_tile]->xmod,
                         item_y + jtp_tiles[item_tile]->ymod,
                         JTP_MAX_SHADES-1,
                         jtp_tiles[item_tile]->graphic);

          n_hotspots++;
          inven_hotspots = (jtp_hotspot **)realloc(inven_hotspots, n_hotspots*sizeof(jtp_hotspot *));
          inven_hotspots[n_hotspots-1] = (jtp_hotspot *)malloc(sizeof(jtp_hotspot));
          (inven_hotspots[n_hotspots-1])->mcursor = jtp_mcursor[JTP_CURSOR_NORMAL];
          (inven_hotspots[n_hotspots-1])->tooltip = jtp_make_tooltip(tempmenuitem->text);
          (inven_hotspots[n_hotspots-1])->x1 = item_x - 49;
          (inven_hotspots[n_hotspots-1])->x2 = item_x + 48;
          (inven_hotspots[n_hotspots-1])->y1 = item_y - 17;
          (inven_hotspots[n_hotspots-1])->y2 = item_y + 18;
          (inven_hotspots[n_hotspots-1])->accelerator = tempmenuitem->accelerator;

          selectable_items++;
          item_x = inven_x + (selectable_items%5)*100;
          item_y = inven_y + (selectable_items/5)*60;
        }
        jtp_list_advance(menu->items);
        tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
      }
      /* Add the 'flip page'-arrow hotspots */
      if (firstitem_index > 0) /* Add 'previous page' arrow */
      {
        n_hotspots++;
        inven_hotspots = (jtp_hotspot **)realloc(inven_hotspots, n_hotspots*sizeof(jtp_hotspot *));
        inven_hotspots[n_hotspots-1] = (jtp_hotspot *)malloc(sizeof(jtp_hotspot));
        (inven_hotspots[n_hotspots-1])->mcursor = jtp_mcursor[JTP_CURSOR_NORMAL];
        (inven_hotspots[n_hotspots-1])->tooltip = jtp_make_tooltip("Previous page");
        (inven_hotspots[n_hotspots-1])->x1 = x + 71;
        (inven_hotspots[n_hotspots-1])->x2 = x + 193;
        (inven_hotspots[n_hotspots-1])->y1 = y + 410;
        (inven_hotspots[n_hotspots-1])->y2 = y + 443;
        (inven_hotspots[n_hotspots-1])->accelerator = JTP_KEY_MENU_SCROLLPAGEUP;
      }
      if (total_selectable_items - firstitem_index > 25) /* Add 'next page' arrow */
      {
        n_hotspots++;
        inven_hotspots = (jtp_hotspot **)realloc(inven_hotspots, n_hotspots*sizeof(jtp_hotspot *));
        inven_hotspots[n_hotspots-1] = (jtp_hotspot *)malloc(sizeof(jtp_hotspot));
        (inven_hotspots[n_hotspots-1])->mcursor = jtp_mcursor[JTP_CURSOR_NORMAL];
        (inven_hotspots[n_hotspots-1])->tooltip = jtp_make_tooltip("Next page");
        (inven_hotspots[n_hotspots-1])->x1 = x + 448;
        (inven_hotspots[n_hotspots-1])->x2 = x + 570;
        (inven_hotspots[n_hotspots-1])->y1 = y + 410;
        (inven_hotspots[n_hotspots-1])->y2 = y + 443;
        (inven_hotspots[n_hotspots-1])->accelerator = JTP_KEY_MENU_SCROLLPAGEDOWN;
      }
      jtp_refresh();
      inventory_page_changed = 0;
    }

    selected_hotspot = -1;
    selected_hotspot = jtp_get_mouse_inventory_input(jtp_mcursor[JTP_CURSOR_NORMAL], inven_hotspots, n_hotspots, JTP_MBUTTON_LEFT);
    if (jtp_kbhit())
    {
      pressedkey = jtp_getch();
      if (pressedkey == 27) /* ESC */
        quit_viewing_inventory = 1;
      else if ((pressedkey == JTP_KEY_MENU_SCROLLPAGEDOWN) || 
               (pressedkey == JTP_KEY_MENU_SCROLLPAGEUP))
      {
/*        jtp_write_log_message("Received PgUp/PgDn\n"); */
        /* If the 'flip page' hotspot exists, select it */
        for (i = 0; i < n_hotspots; i++)        
          if ((inven_hotspots[i])->accelerator == pressedkey)
          {
/*            jtp_write_log_message("Found hotspot\n"); */
            selected_hotspot = i;
            jtp_mouseb = JTP_MBUTTON_LEFT;
          }
      }
    }
    if ((jtp_mouseb == JTP_MBUTTON_RIGHT) && (selected_hotspot >= 0))
    {
      /* If the hotspot accelerator is nonnegative, then this is an inventory item */
      if ((inven_hotspots[selected_hotspot])->accelerator >= 0)
      {
        /* Select a shortcut action for this inventory item */
        dropdown_actions = NULL;
        n_actions = 0;  
        /* Add personal options: */
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_APPLY_ITEM, "Apply");
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_DRINK_ITEM, "Drink");
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_EAT_ITEM, "Eat");
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_READ_ITEM, "Read");
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_ZAP_ITEM, "Zap");
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_WEAR_ITEM, "Wear");
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_PUT_ON_ITEM, "Put on");
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_WIELD_ITEM, "Wield");
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_REMOVE_ITEM, "Remove");
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_DROP_ITEM, "Drop");
        selected_action = jtp_dropdown(jtp_mousex, jtp_mousey, n_actions, dropdown_actions);
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
        if (selected_action > 0)
        {
          jtp_is_backpack_shortcut_active = 1;
          jtp_backpack_shortcut_action = jtp_construct_shortcut_action((inven_hotspots[selected_hotspot])->accelerator, 0, selected_action);
          quit_viewing_inventory = 1;
        }
        /* Clean up the dropdown menu */
        for (i = 0; i < n_actions; i++)
        {
          free(dropdown_actions[i]->str);
          free(dropdown_actions[i]);
        }
        free(dropdown_actions);
      }
      else
      {
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      }
    }
    else if ((jtp_mouseb == JTP_MBUTTON_LEFT) && (selected_hotspot >= 0))
    {
      if ((inven_hotspots[selected_hotspot])->accelerator == JTP_KEY_MENU_SCROLLPAGEUP)
      {
        /* Wait for mouse button release, then redisplay the inventory */
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
        firstitem_index = firstitem_index - 25;
        inventory_page_changed = 1;
      }
      else if ((inven_hotspots[selected_hotspot])->accelerator == JTP_KEY_MENU_SCROLLPAGEDOWN)
      {
        /* Wait for mouse button release, then redisplay the inventory */
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
        firstitem_index = firstitem_index + 25;
        inventory_page_changed = 1;
      }
      else
      {
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      }
    }
    else if ((jtp_mouseb == JTP_MBUTTON_LEFT) && (selected_hotspot < 0))
    {
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      quit_viewing_inventory = 1;
    }
    else
    {
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
    }
  }

  /* Restore background and clean up */
  jtp_put_img(x, y, backpack_bg);
  jtp_refresh();
  free(backpack_bg);

  if (n_hotspots > 0)
  {
    for (i = 0; i < n_hotspots; i++)
    {
      free((inven_hotspots[i])->tooltip);
      free(inven_hotspots[i]);
    }
    free(inven_hotspots);
  }
}

int jtp_text_to_spell_symbol(char * spelltext)
{
  int spellglyph = 0;
  int i;

  if (!spelltext) return(0);

  for (i = 6; (i < 26) & (spelltext[i] != '\0'); i++)
    spellglyph += (unsigned char)spelltext[i];
  spellglyph = spellglyph % 10;
  return(spellglyph);
}

void jtp_view_spellbook(jtp_window *menuwindow)
{
  int x, y;
  int spells_x, spells_y;
  int total_selectable_items;
  int selectable_items;
  int i, j;
  int cur_glyph, cur_symbol;
  int quit_viewing_spellbook;
  int spellbook_page_changed;
  unsigned char * spellbook_bg;

  jtp_menu * menu;
  int firstitem_index;
  jtp_menuitem * tempmenuitem;
  int item_x, item_y, item_index;
  int item_tile;

  jtp_hotspot ** spells_hotspots;
  int n_hotspots;
  int selected_hotspot;
  int pressedkey;

  jtp_dropdown_action ** dropdown_actions;
  int n_actions;
  int selected_action;

  if (!menuwindow) return;

  /* Find upper left corner of backpack */
  x = (jtp_screen.width - 640)/2;
  y = (jtp_screen.height - 480)/2;

  spellbook_bg = jtp_get_img(x, y, x + 640-1, y + 480-1);

  /* Draw spellbook */
  jtp_put_stencil(x, y, jtp_spellbook_top);
  jtp_put_stencil(x, y + 480 - jtp_spellbook_bottom[1], jtp_spellbook_bottom);
  jtp_put_stencil(x, y, jtp_spellbook_left);
  jtp_put_stencil(x + 640 - jtp_spellbook_right[3], y, jtp_spellbook_right);

  /* Find upper left corner of spells on spellbook */
  spells_x = x + 73;
  spells_y = y + 60;

  menu = menuwindow->menu;
  if ((!menu) || (!menu->items)) return;

  /* Find out total number of selectable items in spellbook */
  total_selectable_items = 0;
  jtp_list_reset(menu->items);
  tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
  while (tempmenuitem)
  {
    if (tempmenuitem->count != JTP_NOT_SELECTABLE)     
      total_selectable_items++;
    jtp_list_advance(menu->items);
    tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
  }

  quit_viewing_spellbook = 0;
  spellbook_page_changed = 1;
  firstitem_index = 0;
  spells_hotspots = NULL;
  n_hotspots = 0;

  /* Wait for mouse button release, then display the spellbook */
  jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);

  while (!quit_viewing_spellbook)
  {
    if (spellbook_page_changed)
    {
      /* Clean up previous hotspots */
      if (n_hotspots > 0)
      {
        for (i = 0; i < n_hotspots; i++)
        {
          free((spells_hotspots[i])->tooltip);
          free(spells_hotspots[i]);
        }
        free(spells_hotspots);
        spells_hotspots = NULL;
        n_hotspots = 0;
      }

      /* Draw spells on backpack, and create hotspots */
      jtp_put_img(x + jtp_spellbook_left[3], y + jtp_spellbook_top[1], jtp_spellbook_center);

      /* Find the first selectable item to be displayed */
      item_index = -1;
      jtp_list_reset(menu->items);
      tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
      while ((tempmenuitem) && (item_index < firstitem_index))
      {
        if (tempmenuitem->count != JTP_NOT_SELECTABLE)
          item_index++;
        if (item_index < firstitem_index)
        {
          jtp_list_advance(menu->items);
          tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
        }
      }
      
      item_x = spells_x;
      item_y = spells_y;
      selectable_items = 0; /* None shown so far */
      while ((tempmenuitem) && (selectable_items < 12))
      {
        if (tempmenuitem->count != JTP_NOT_SELECTABLE)     
        {
          item_tile = jtp_text_to_spell_symbol(tempmenuitem->text);
          jtp_put_stencil(item_x, item_y, jtp_spell_symbols[item_tile]);

          n_hotspots++;
          spells_hotspots = (jtp_hotspot **)realloc(spells_hotspots, n_hotspots*sizeof(jtp_hotspot *));
          spells_hotspots[n_hotspots-1] = (jtp_hotspot *)malloc(sizeof(jtp_hotspot));
          (spells_hotspots[n_hotspots-1])->mcursor = jtp_mcursor[JTP_CURSOR_NORMAL];
          (spells_hotspots[n_hotspots-1])->tooltip = jtp_make_tooltip(tempmenuitem->text);
          (spells_hotspots[n_hotspots-1])->x1 = item_x;
          (spells_hotspots[n_hotspots-1])->x2 = item_x + 97;
          (spells_hotspots[n_hotspots-1])->y1 = item_y;
          (spells_hotspots[n_hotspots-1])->y2 = item_y + 97;
          (spells_hotspots[n_hotspots-1])->accelerator = tempmenuitem->accelerator;

          selectable_items++;
          item_x = spells_x + (selectable_items%4)*100;
          if ((selectable_items%4) > 1) item_x += 100;
          item_y = spells_y + (selectable_items/4)*110;
        }
        jtp_list_advance(menu->items);
        tempmenuitem = (jtp_menuitem *)jtp_list_current(menu->items);
      }
      /* Add the 'flip page'-arrow hotspots */
      if (firstitem_index > 0) /* Add 'previous page' arrow */
      {
        n_hotspots++;
        spells_hotspots = (jtp_hotspot **)realloc(spells_hotspots, n_hotspots*sizeof(jtp_hotspot *));
        spells_hotspots[n_hotspots-1] = (jtp_hotspot *)malloc(sizeof(jtp_hotspot));
        (spells_hotspots[n_hotspots-1])->mcursor = jtp_mcursor[JTP_CURSOR_NORMAL];
        (spells_hotspots[n_hotspots-1])->tooltip = jtp_make_tooltip("Previous page");
        (spells_hotspots[n_hotspots-1])->x1 = x + 41;
        (spells_hotspots[n_hotspots-1])->x2 = x + 91;
        (spells_hotspots[n_hotspots-1])->y1 = y + 397;
        (spells_hotspots[n_hotspots-1])->y2 = y + 424;
        (spells_hotspots[n_hotspots-1])->accelerator = JTP_KEY_MENU_SCROLLPAGEUP;
      }
      if (total_selectable_items - firstitem_index > 12) /* Add 'next page' arrow */
      {
        n_hotspots++;
        spells_hotspots = (jtp_hotspot **)realloc(spells_hotspots, n_hotspots*sizeof(jtp_hotspot *));
        spells_hotspots[n_hotspots-1] = (jtp_hotspot *)malloc(sizeof(jtp_hotspot));
        (spells_hotspots[n_hotspots-1])->mcursor = jtp_mcursor[JTP_CURSOR_NORMAL];
        (spells_hotspots[n_hotspots-1])->tooltip = jtp_make_tooltip("Next page");
        (spells_hotspots[n_hotspots-1])->x1 = x + 549;
        (spells_hotspots[n_hotspots-1])->x2 = x + 599;
        (spells_hotspots[n_hotspots-1])->y1 = y + 397;
        (spells_hotspots[n_hotspots-1])->y2 = y + 424;
        (spells_hotspots[n_hotspots-1])->accelerator = JTP_KEY_MENU_SCROLLPAGEDOWN;
      }
      jtp_refresh();
      spellbook_page_changed = 0;
    }

    selected_hotspot = -1;
    selected_hotspot = jtp_get_mouse_inventory_input(jtp_mcursor[JTP_CURSOR_NORMAL], spells_hotspots, n_hotspots, JTP_MBUTTON_LEFT);
    if (jtp_kbhit())
    {
      pressedkey = jtp_getch();
      if (pressedkey == 27) /* ESC */
        quit_viewing_spellbook = 1;
      else if ((pressedkey == JTP_KEY_MENU_SCROLLPAGEDOWN) || 
               (pressedkey == JTP_KEY_MENU_SCROLLPAGEUP))
      {
        /* If the 'flip page' hotspot exists, select it */
        for (i = 0; i < n_hotspots; i++)        
          if ((spells_hotspots[i])->accelerator == pressedkey)
          {
            selected_hotspot = i;
            jtp_mouseb = JTP_MBUTTON_LEFT;
          }
      }
    }
    if ((jtp_mouseb == JTP_MBUTTON_RIGHT) && (selected_hotspot >= 0))
    {
      /* If the hotspot accelerator is nonnegative, then this is an inventory item */
      if ((spells_hotspots[selected_hotspot])->accelerator >= 0)
      {
        /* Select a shortcut action for this inventory item */
        dropdown_actions = NULL;
        n_actions = 0;  
        /* Add personal options: */
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_CAST_SPELL, "Cast Spell");
        selected_action = jtp_dropdown(jtp_mousex, jtp_mousey, n_actions, dropdown_actions);
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
        if (selected_action > 0)
        {
          jtp_is_backpack_shortcut_active = 1;
          jtp_backpack_shortcut_action = jtp_construct_shortcut_action((spells_hotspots[selected_hotspot])->accelerator, 0, selected_action);
          quit_viewing_spellbook = 1;
        }
        /* Clean up the dropdown menu */
        for (i = 0; i < n_actions; i++)
        {
          free(dropdown_actions[i]->str);
          free(dropdown_actions[i]);
        }
        free(dropdown_actions);
      }
      else
      {
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      }
    }
    else if ((jtp_mouseb == JTP_MBUTTON_LEFT) && (selected_hotspot >= 0))
    {
      if ((spells_hotspots[selected_hotspot])->accelerator == JTP_KEY_MENU_SCROLLPAGEUP)
      {
        /* Wait for mouse button release, then redisplay the spells */
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
        firstitem_index = firstitem_index - 12;
        spellbook_page_changed = 1;
      }
      else if ((spells_hotspots[selected_hotspot])->accelerator == JTP_KEY_MENU_SCROLLPAGEDOWN)
      {
        /* Wait for mouse button release, then redisplay the spells */
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
        firstitem_index = firstitem_index + 12;
        spellbook_page_changed = 1;
      }
      else
      {
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      }
    }
    else if ((jtp_mouseb == JTP_MBUTTON_LEFT) && (selected_hotspot < 0))
    {
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      quit_viewing_spellbook = 1;
    }
    else
    {
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
    }
  }

  /* Restore background and clean up */
  jtp_put_img(x, y, spellbook_bg);
  jtp_refresh();
  free(spellbook_bg);

  if (n_hotspots > 0)
  {
    for (i = 0; i < n_hotspots; i++)
    {
      free((spells_hotspots[i])->tooltip);
      free(spells_hotspots[i]);
    }
    free(spells_hotspots);
  }

  /* Not viewing spellbook anymore. The next menu will be normal-style. */
  jtp_is_spellbook_being_viewed = 0;
}


void
jtp_recenter_from_minimap()
{
  int i, j;
  i = jtp_mousey - (jtp_statusbar_y + 51);
  j = jtp_mousex - (jtp_statusbar_x + 98);
  jtp_map_x = jtp_map_x + (j+2*i)/4;
  jtp_map_y = jtp_map_y + (2*i-j)/4;
  if (jtp_map_x < 1) jtp_map_x = 1;
  if (jtp_map_x >= JTP_MAP_WIDTH) jtp_map_x = JTP_MAP_WIDTH-1;
  if (jtp_map_y < 0) jtp_map_y = 0;
  if (jtp_map_y >= JTP_MAP_HEIGHT) jtp_map_y = JTP_MAP_HEIGHT-1;
  jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
}

void
jtp_autopilot_from_minimap()
{
  int i, j, tx, ty;
  i = jtp_mousey - (jtp_statusbar_y + 51);
  j = jtp_mousex - (jtp_statusbar_x + 98);
  tx = jtp_map_x + (j+2*i)/4;
  ty = jtp_map_y + (2*i-j)/4;
  if (tx < 1) tx = 1;
  if (tx >= JTP_MAP_WIDTH) tx = JTP_MAP_WIDTH-1;
  if (ty < 0) ty = 0;
  if (ty >= JTP_MAP_HEIGHT) ty = JTP_MAP_HEIGHT-1;
  jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
  jtp_find_path(jtp_you_y, jtp_you_x, ty, tx);
}


char
jtp_process_mouseclick()
{
  int hotspot;
  char selected_command;
  clock_t cur_time;
  
  if (jtp_mouseb == JTP_MBUTTON_NONE) 
    return(0);

  /* jtp_write_log_message("[jtp_win.c/jtp_process_mouseclick/Debug1]\n"); */

  hotspot = jtp_mouse_hotspot();
  cur_time = clock();
  switch (hotspot)
  {
    /* Scroll commands */
    case JTP_HOTSPOT_SCROLL_UP: 
       if (cur_time-jtp_last_scroll_time > jtp_min_scroll_delay*CLOCKS_PER_SEC)
       {
         jtp_last_scroll_time = cur_time;
         jtp_map_x--; jtp_map_y--; 
       }
       break;
    case JTP_HOTSPOT_SCROLL_DOWN: 
       if (cur_time-jtp_last_scroll_time > jtp_min_scroll_delay*CLOCKS_PER_SEC)
       {
         jtp_last_scroll_time = cur_time;
         jtp_map_x++; jtp_map_y++; 
       }
       break;
    case JTP_HOTSPOT_SCROLL_LEFT: 
       if (cur_time-jtp_last_scroll_time > jtp_min_scroll_delay*CLOCKS_PER_SEC)
       {
         jtp_last_scroll_time = cur_time;
         jtp_map_x--; jtp_map_y++; 
       }
       break;
    case JTP_HOTSPOT_SCROLL_RIGHT: 
       if (cur_time-jtp_last_scroll_time > jtp_min_scroll_delay*CLOCKS_PER_SEC)
       {
         jtp_last_scroll_time = cur_time;
         jtp_map_x++; jtp_map_y--; 
       }
       break;
    /* Autopilot or recenter from mini-map */
    case JTP_HOTSPOT_MINI_MAP:
      if (jtp_mouseb == JTP_MBUTTON_LEFT)
      {
        jtp_autopilot_from_minimap();
        return(0);
      }
      else
      {
        jtp_recenter_from_minimap();
        return(0);
      }
      break;
    /* Autopilot */
    case JTP_HOTSPOT_MAP:
      if (jtp_mouseb == JTP_MBUTTON_LEFT)
      {
        if (cur_time-jtp_last_scroll_time > jtp_min_command_delay*CLOCKS_PER_SEC)
        {      
          /* Use default command for target square */
          jtp_last_scroll_time = cur_time;
          selected_command = jtp_get_default_command(jtp_map_tgtx, jtp_map_tgty);
          if (jtp_one_command_per_click)
            while(jtp_mouseb != JTP_MBUTTON_NONE) 
              jtp_readmouse();
          return(selected_command);
        }
      }
      else
      {
        /* Select command from context-sensitive menu */
        return(jtp_get_dropdown_command(jtp_mousex, jtp_mousey, jtp_map_tgtx, jtp_map_tgty));
      }
      break;
    /* Command shortcuts */  
    case JTP_HOTSPOT_BUTTON_MAP:
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      jtp_view_map();
      break;
    case JTP_HOTSPOT_BUTTON_MESSAGES:
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      jtp_view_messages();
      break;
    case JTP_HOTSPOT_BUTTON_HELP:
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      return(jtp_translate_command(JTP_NHCMD_HELP_MENU));
      break;
    case JTP_HOTSPOT_BUTTON_SPELLBOOK:
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      jtp_is_spellbook_being_viewed = 1;
      return(jtp_translate_command(JTP_NHCMD_LIST_SPELLS));
      break;
    case JTP_HOTSPOT_BUTTON_INVENTORY:
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      return(jtp_translate_command(JTP_NHCMD_LIST_INVENTORY));
      break;
    case JTP_HOTSPOT_BUTTON_OPTIONS:
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      return(jtp_translate_command(JTP_NHCMD_SET_OPTIONS));
      break;
    case JTP_HOTSPOT_BUTTON_LOOK:
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      jtp_is_shortcut_active = 1;
      jtp_shortcut_query_response = 'y';
      return(jtp_translate_command(JTP_NHCMD_EXPLAIN_SYMBOL)); 
      break;
    case JTP_HOTSPOT_BUTTON_EXTENDED:
      jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      return(jtp_translate_command(JTP_NHCMD_EXTENDED_COMMAND)); 
      break;
    default: break;    
  }
  
  if (jtp_map_x >= JTP_MAP_WIDTH) jtp_map_x = JTP_MAP_WIDTH-1;
  if (jtp_map_x < 0) jtp_map_x = 0;
  if (jtp_map_y >= JTP_MAP_HEIGHT) jtp_map_y = JTP_MAP_HEIGHT-1;
  if (jtp_map_y < 0) jtp_map_y = 0;
  return(0);
}


char
jtp_whatis_mouseclick(tx, ty)
int *tx, *ty;
{
  int hotspot;
  
  *tx = -1; *ty = -1;
  if (jtp_mouseb == JTP_MBUTTON_NONE) 
    return(0);
  
  hotspot = jtp_mouse_hotspot();
  switch (hotspot)
  {
    /* Scroll commands */
    case JTP_HOTSPOT_SCROLL_UP: jtp_map_x--; jtp_map_y--; break;
    case JTP_HOTSPOT_SCROLL_DOWN: jtp_map_x++; jtp_map_y++; break;
    case JTP_HOTSPOT_SCROLL_LEFT: jtp_map_x--; jtp_map_y++; break;
    case JTP_HOTSPOT_SCROLL_RIGHT: jtp_map_x++; jtp_map_y--; break;
    /* Autopilot */
    case JTP_HOTSPOT_MAP:
      if (isok(jtp_map_tgtx, jtp_map_tgty))
      {
        *tx = jtp_map_tgtx;
        *ty = jtp_map_tgty;
        return(0);
      }
      break;
    /* Command shortcuts */  
    case JTP_HOTSPOT_BUTTON_MAP:
      jtp_view_map();
      break;      
    case JTP_HOTSPOT_BUTTON_MESSAGES:
      jtp_view_messages();
      break;
    case JTP_HOTSPOT_BUTTON_HELP:
      return(jtp_translate_command(JTP_NHCMD_EXPLAIN_SYMBOL));
      break;
    case JTP_HOTSPOT_BUTTON_SPELLBOOK:
    case JTP_HOTSPOT_BUTTON_INVENTORY:
    case JTP_HOTSPOT_BUTTON_LOOK:
    case JTP_HOTSPOT_BUTTON_EXTENDED:
    case JTP_HOTSPOT_BUTTON_OPTIONS:    
      return(' ');   /* Stop whatis first */
      break;
    default: break;    
  }
  
  if (jtp_map_x > 80) jtp_map_x = 80;
  if (jtp_map_x < 0) jtp_map_x = 0;
  if (jtp_map_y > 50) jtp_map_y = 50;
  if (jtp_map_y < 0) jtp_map_y = 0;
  return(0);
}


void jtp_get_input
(
  int forced_x, int forced_y, /* Forced location of prompt window (-1 = not forced) */
  const char * ques,          /* Message to show player */
  char * input                /* Answer string */
)
{
  int i, j, k;
  int totalwidth, totalheight;
  int query_x, query_y;
  int nchoice = 0;
  unsigned char * query_background;
  unsigned char * input_background;
  unsigned char key_result;

  /* Calculate width, height and position of query window */
  totalwidth = 500;
  i = jtp_text_length((char *)ques, JTP_FONT_HEADLINE) + 
      jtp_defwin.border_left[3] + 
      jtp_defwin.border_right[3];
  if (i > totalwidth) totalwidth = i;
  
  totalheight = jtp_text_height((char *)ques, JTP_FONT_HEADLINE);
  totalheight += 3*jtp_fonts[JTP_FONT_INPUT].lineheight;

  totalwidth += jtp_defwin.border_left[3];
  totalwidth += jtp_defwin.border_right[3];
  totalheight += jtp_defwin.border_top[1];
  totalheight += jtp_defwin.border_bottom[1];

  if (forced_x >= 0) query_x = forced_x;
  else query_x = (jtp_screen.width - totalwidth) / 2;

  if (forced_y >= 0) query_y = forced_y;
  else query_y = (jtp_screen.height - totalheight) / 2;

  /* Store background graphics */
  query_background = jtp_draw_window(query_x, query_y, totalwidth, totalheight);
  input_background = jtp_get_img(query_x + jtp_defwin.border_left[3],
                                 query_y + jtp_defwin.border_top[1] + 
                                   jtp_text_height((char *)ques, JTP_FONT_HEADLINE) + 
                                   jtp_fonts[JTP_FONT_INPUT].lineheight,
                                 query_x + totalwidth - jtp_defwin.border_right[3],
                                 query_y + jtp_defwin.border_top[1] + 
                                   jtp_text_height((char *)ques, JTP_FONT_HEADLINE) + 
                                   2*jtp_fonts[JTP_FONT_INPUT].lineheight);
  /* Draw query message */
  jtp_put_text(jtp_defwin.border_left[3] + query_x + 1,
               jtp_defwin.border_top[1] + query_y + 
                 jtp_fonts[JTP_FONT_HEADLINE].baseline + 1,
               JTP_FONT_HEADLINE,
               0,
               (char *)ques,
               jtp_screen.vpage);
  jtp_put_text(jtp_defwin.border_left[3] + query_x,
               jtp_defwin.border_top[1] + query_y + 
                 jtp_fonts[JTP_FONT_HEADLINE].baseline,
               JTP_FONT_HEADLINE,
               15,
               (char *)ques,
               jtp_screen.vpage);

  /* Display window */
  jtp_refresh();
  /* In case the palette is faded out for some reason, restore it */
  jtp_updatepal(0, 255);
    
  /* Wait for input */
  i = 0;
  input[0] = '\0';
  
  do
  {
    /* Redraw text */  
    jtp_put_img(query_x + jtp_defwin.border_left[3], 
                query_y + jtp_defwin.border_top[1] + 
                  jtp_text_height((char *)ques, JTP_FONT_HEADLINE) +                
                  jtp_fonts[JTP_FONT_INPUT].lineheight,
                input_background); 

    
    
    jtp_put_text(jtp_defwin.border_left[3]+query_x+1, 
                 query_y + jtp_defwin.border_top[1] + 
                   jtp_text_height((char *)ques, JTP_FONT_HEADLINE) + 
                   jtp_fonts[JTP_FONT_INPUT].lineheight +                   
                   jtp_fonts[JTP_FONT_INPUT].baseline + 1,
                 JTP_FONT_INPUT, 0,
                 input,
                 jtp_screen.vpage);
    jtp_put_text(jtp_defwin.border_left[3]+query_x+1, 
                 query_y + jtp_defwin.border_top[1] + 
                   jtp_text_height((char *)ques, JTP_FONT_HEADLINE) + 
                   jtp_fonts[JTP_FONT_INPUT].lineheight +                   
                   jtp_fonts[JTP_FONT_INPUT].baseline,
                 JTP_FONT_INPUT, 15,
                 input,
                 jtp_screen.vpage);
    /* Draw prompt */
    jtp_rect(query_x + jtp_defwin.border_left[3] + jtp_text_length(input, JTP_FONT_INPUT) + 2,
             query_y + jtp_defwin.border_top[1] + 
               jtp_text_height((char *)ques, JTP_FONT_HEADLINE) +
               jtp_fonts[JTP_FONT_INPUT].lineheight,
             query_x + jtp_defwin.border_left[3] + jtp_text_length(input, JTP_FONT_INPUT) + 2,
             query_y + jtp_defwin.border_top[1] +
               jtp_text_height((char *)ques, JTP_FONT_HEADLINE) +
               jtp_fonts[JTP_FONT_INPUT].lineheight +
               jtp_fonts[JTP_FONT_INPUT].baseline,
             15);
    /* Display window */
    jtp_refresh();
  
    key_result = jtp_getch();

    /* if ((key_result < 'a') || (key_result > 'z')) key_result = '?'; */
    if ((jtp_fonts[JTP_FONT_INPUT].fontpics[key_result].kuva != NULL) && (i < 80))
    {
      input[i] = key_result;
      input[i+1] = '\0';
      i++;
    }

    /* Backspace key */
    if ((key_result == 8)  || (jtp_text_length(input, JTP_FONT_INPUT)>290))
      if (i>0)
      {
        input[i-1]='\0';
        i--;
      }

    if (key_result == 27) /* ESC */
    {
      if (strcmp(ques, "What is your name?") == 0)
      {
        strcpy(input, "NHFEPLAYERPRESSEDESC");
        key_result = 13;
      }
    }
    
  } while (key_result != 13);


  /* Restore background */
  jtp_put_img(query_x, query_y, query_background);
  jtp_refresh();

  /* Clean up */
  free(query_background);
  free(input_background);
}

void
jtp_askname()
{
/*  jtp_write_log_message("DEBUGMESSAGE askname\n"); */

  jtp_get_input(-1, (jtp_screen.height-JTP_NH_LOGO_HEIGHT)/2 + 430,
                "What is your name?", plname);
  if (strcmp(plname, "NHFEPLAYERPRESSEDESC") == 0) 
  {
    /* Player pressed ESC during the name query, so quit the game */
    bail((char *)0);
  }

  jtp_fade_out(0.2);
  
  /* Restore the regular game palette */  
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_MOUSE_CURSORS], 1);
  jtp_clear_screen();
  jtp_refresh();
  jtp_updatepal(0, 255);  
  jtp_game_palette_set = 1;
}

void
jtp_read_options
(
  char * optfile,                /* File for interface options */
  char * soundsfile,             /* File for event sound options */
  char * keymapfile,             /* File for key mapping options */
  int * screen_width,
  int * screen_height,
  int * wall_display_style
)
{
  FILE * f;
  char tempbuffer[1024];
  char *tok;
  char *tok2;
  int i;
  int old_key, new_key;
  int soundtype;


  /* printf("DEBUG[jtp_win.c/30]: Reading options from [%s]\n", optfile); */

  *screen_width = 800;
  *screen_height = 600;

  /* Read interface options */
  f = fopen(optfile, "rb");
  if (!f) return; /* Use defaults */

  while (fgets(tempbuffer, 1024, f))
  {
    tok = NULL;
    if (tempbuffer[0] != '%') 
      tok = strstr(tempbuffer, "=");

    if (tok)
    {
      if (!strncmp(tempbuffer, "screen_xsize", tok - tempbuffer))
        *screen_width = atoi(tok+1);
      else if (!strncmp(tempbuffer, "screen_ysize", tok - tempbuffer))
        *screen_height = atoi(tok+1);
      else if (!strncmp(tempbuffer, "command_delay", tok - tempbuffer))
        jtp_min_command_delay = atof(tok+1);
      else if (!strncmp(tempbuffer, "scrolling_delay", tok - tempbuffer))
        jtp_min_scroll_delay = atof(tok+1);
      else if (!strncmp(tempbuffer, "wall_style", tok - tempbuffer))
      {
        if (!strncmp(tok+1, "full", 4))
          *wall_display_style = JTP_WALL_DISPLAY_STYLE_FULL;
        else if (!strncmp(tok+1, "half_height", 11))
          *wall_display_style = JTP_WALL_DISPLAY_STYLE_HALF_HEIGHT;
        else if (!strncmp(tok+1, "transparent", 11))
          *wall_display_style = JTP_WALL_DISPLAY_STYLE_TRANSPARENT;
      }
      else if (!strncmp(tempbuffer, "recenter_after_movement", tok - tempbuffer))
        jtp_recenter_after_movement = atoi(tok+1);
      else if (!strncmp(tempbuffer, "play_music", tok - tempbuffer))
        jtp_play_music = atoi(tok+1);
      else if (!strncmp(tempbuffer, "play_effects", tok - tempbuffer))
        jtp_play_effects = atoi(tok+1);
      else if (!strncmp(tempbuffer, "one_command_per_click", tok - tempbuffer))
        jtp_one_command_per_click = atoi(tok+1);
      else if (!strncmp(tempbuffer, "fullscreen", tok - tempbuffer))
        jtp_fullscreen = atoi(tok+1);
      else if (!strncmp(tempbuffer, "gamma_correction", tok - tempbuffer))
        jtp_gamma_correction = atof(tok+1);
      else if (!strncmp(tempbuffer, "linux_midi_player", tok - tempbuffer))
	{
        jtp_external_midi_player_command = (char *)malloc(strlen(tok+1)+1);
        strcpy(jtp_external_midi_player_command, tok+1);
        /* Remove end-of-line from the string */
        i = strlen(jtp_external_midi_player_command)-1;
        while ((jtp_external_midi_player_command[i] == 10) ||
               (jtp_external_midi_player_command[i] == 13))
          i--;
        jtp_external_midi_player_command[i+1] = '\0';
	}
      else if (!strncmp(tempbuffer, "linux_mp3_player", tok - tempbuffer))
	{
        jtp_external_mp3_player_command = (char *)malloc(strlen(tok+1)+1);
        strcpy(jtp_external_mp3_player_command, tok+1);
        /* Remove end-of-line from the string */
        i = strlen(jtp_external_mp3_player_command)-1;
        while ((jtp_external_mp3_player_command[i] == 10) ||
               (jtp_external_mp3_player_command[i] == 13))
          i--;
        jtp_external_mp3_player_command[i+1] = '\0';
	}
    }
  }
  fclose(f);

  /* Read key mapping options */
  /* Create the key binding table */
  jtp_keymaps = (jtp_command *)calloc(sizeof(jtp_command), JTP_MAX_NETHACK_COMMANDS);  
  jtp_set_nethack_keys();
  /* jtp_set_default_keymaps(); */ /* Use whatever NetHack has */
  jtp_read_key_configuration(keymapfile);

  /* Read event sounds options */
  jtp_event_sounds = NULL;
  jtp_n_event_sounds = 0;

  f = fopen(soundsfile, "rb");
  while (fgets(tempbuffer, 1024, f))
  {
    if ((tempbuffer[0] == '%') || (tempbuffer[0] == '\0') ||
        (tempbuffer[0] == 10) || (tempbuffer[0] == 13))
    {
      /* Comment or empty line. Skip. */
    }
    else
    {
      /* Check for sound type */

      soundtype = -1;
      if ((tok = strstr(tempbuffer, "],WAVE,[")) != NULL) /* WAVE sound found */
        soundtype = JTP_EVENT_SOUND_TYPE_WAVE;      
      else if ((tok = strstr(tempbuffer, "],LWAV,[")) != NULL) /* Long WAVE sound found */
        soundtype = JTP_EVENT_SOUND_TYPE_LONG_WAVE;      
      else if ((tok = strstr(tempbuffer, "],MIDI,[")) != NULL) /* MIDI file found */      
        soundtype = JTP_EVENT_SOUND_TYPE_MIDI;      
      else if ((tok = strstr(tempbuffer, "],RSNG,[")) != NULL) /* Random song file found */
        soundtype = JTP_EVENT_SOUND_TYPE_RANDOM_SONG;
      else if ((tok = strstr(tempbuffer, "],MP3,[")) != NULL)
        soundtype = JTP_EVENT_SOUND_TYPE_MP3;
      else if ((tok = strstr(tempbuffer, "],CDAU,[")) != NULL) /* CD audio track found */
        soundtype = JTP_EVENT_SOUND_TYPE_CD_AUDIO;      
      else if ((tok = strstr(tempbuffer, "],NONE,[")) != NULL) /* NONE placeholder found */
        soundtype = JTP_EVENT_SOUND_TYPE_NONE;      

      if (soundtype >= 0) /* Valid sound found */
      {
        jtp_n_event_sounds++;
        jtp_event_sounds = (jtp_event_sound **)realloc(jtp_event_sounds, jtp_n_event_sounds*sizeof(jtp_event_sound *));
        jtp_event_sounds[jtp_n_event_sounds-1] = (jtp_event_sound *)malloc(sizeof(jtp_event_sound));
        (jtp_event_sounds[jtp_n_event_sounds-1])->filename = (char *)malloc(JTP_MAX_FILENAME_LENGTH*sizeof(char));

        (jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern = (char *)malloc(JTP_MAX_FILENAME_LENGTH*sizeof(char));
        memcpy((jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern, tempbuffer+1, tok-tempbuffer-1);
        (jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern[tok-tempbuffer-1] = '\0';

        /* Check for a background music event */
        if (!strcmp((jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern, "nhfe_music_background"))
	{
          jtp_n_background_songs++;
          free((jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern);
          (jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern = (char *)malloc(strlen("nhfe_music_background")+4);
          sprintf((jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern, "nhfe_music_background%03d", jtp_n_background_songs-1);
	}

        if (soundtype == JTP_EVENT_SOUND_TYPE_MP3) tok = tok + 7;
        else tok = tok + 8;

        tok2 = strstr(tok, "]");
        memcpy((jtp_event_sounds[jtp_n_event_sounds-1])->filename, tok, tok2-tok);
        (jtp_event_sounds[jtp_n_event_sounds-1])->filename[tok2-tok] = '\0';

        (jtp_event_sounds[jtp_n_event_sounds-1])->soundtype = soundtype;


        /* If this isn't a CD track, add path to sounds subdirectory before filename */
        if (soundtype != JTP_EVENT_SOUND_TYPE_CD_AUDIO)
        {
          sprintf(tempbuffer,"%s%s%s\0", jtp_game_path, JTP_SOUND_DIRECTORY, (jtp_event_sounds[jtp_n_event_sounds-1])->filename);
          strcpy((jtp_event_sounds[jtp_n_event_sounds-1])->filename, tempbuffer);
        }

        /*sprintf(tempbuffer, "Mapped [%s] to [%s]\n", (jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern, (jtp_event_sounds[jtp_n_event_sounds-1])->filename);
        jtp_write_log_message(tempbuffer);*/
      }
    }
  }
  fclose(f);
}

void
jtp_init_filenames()
{
  int gplength;
  int i;

  /* Get starting directory, and save it for reference */
  getcwd(jtp_game_path, JTP_MAX_FILENAME_LENGTH);
  gplength = strlen(jtp_game_path);
  /* DOS/Windows use backslashed and Linux/Unix/BeOS uses forward slashes */
#ifdef JTP_USE_UNIX_STYLE_DIRECTORY_SEPARATOR
  jtp_game_path[gplength] = '/';
#endif
#ifndef JTP_USE_UNIX_STYLE_DIRECTORY_SEPARATOR
  jtp_game_path[gplength] = '\\';
#endif
  jtp_game_path[gplength+1] = '\0';
  gplength++;

  /* printf("DEBUG[jtp_win.c/10]: Game directory is [%s]\n", jtp_game_path); */

  /* Initialize filename table */
  for (i = 0; i < JTP_MAX_GAME_FILES; i++)
  {
    jtp_filenames[i] = malloc(JTP_MAX_FILENAME_LENGTH);
    strcpy(jtp_filenames[i], jtp_game_path);
  }

  /* Data filenames */
  strcat(jtp_filenames[JTP_FILE_INTRO_SCRIPT], JTP_CONFIG_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_INTRO_SCRIPT], "jtp_intr.txt");
  strcat(jtp_filenames[JTP_FILE_OPTIONS], JTP_CONFIG_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_OPTIONS], "jtp_opts.txt");
  strcat(jtp_filenames[JTP_FILE_SOUNDS_CONFIG], JTP_CONFIG_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_SOUNDS_CONFIG], "jtp_snds.txt");
  strcat(jtp_filenames[JTP_FILE_KEYS_CONFIG], JTP_CONFIG_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_KEYS_CONFIG], "jtp_keys.txt");
  strcat(jtp_filenames[JTP_FILE_SHADING_TABLE], JTP_CONFIG_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_SHADING_TABLE], "jtp_lit1.dat");

  /* Graphics filenames */
  strcat(jtp_filenames[JTP_FILE_CMAP_TILES], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_CMAP_TILES], "jtp_cm07.pcx");
    strcat(jtp_filenames[JTP_FILE_CMAP_TILES_2], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_CMAP_TILES_2], "jtp_cmc7.pcx");
  strcat(jtp_filenames[JTP_FILE_CMAP_WALL_TILES], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_CMAP_WALL_TILES], "walls02a.pcx");
  strcat(jtp_filenames[JTP_FILE_CMAP_WALL_TILES_2], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_CMAP_WALL_TILES_2], "walls03a.pcx");
  strcat(jtp_filenames[JTP_FILE_CMAP_TRANSPARENT_WALL_TILES], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_CMAP_TRANSPARENT_WALL_TILES], "walls02b.pcx");
  strcat(jtp_filenames[JTP_FILE_CMAP_TRANSPARENT_WALL_TILES_2], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_CMAP_TRANSPARENT_WALL_TILES_2], "walls03b.pcx");
  strcat(jtp_filenames[JTP_FILE_CMAP_HALF_HEIGHT_WALL_TILES], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_CMAP_HALF_HEIGHT_WALL_TILES], "walls02c.pcx");
  strcat(jtp_filenames[JTP_FILE_CMAP_HALF_HEIGHT_WALL_TILES_2], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_CMAP_HALF_HEIGHT_WALL_TILES_2], "walls03c.pcx");
  strcat(jtp_filenames[JTP_FILE_CMAP_FLOOR_TILES], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_CMAP_FLOOR_TILES], "jtp_cm12.pcx");
  strcat(jtp_filenames[JTP_FILE_CMAP_FLOOR_TILES_2], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_CMAP_FLOOR_TILES_2], "jtp_cm13.pcx");
  strcat(jtp_filenames[JTP_FILE_OBJ_TILES_1], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_OBJ_TILES_1], "jtp_obj1.pcx");
  strcat(jtp_filenames[JTP_FILE_OBJ_TILES_2], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_OBJ_TILES_2], "jtp_obj2.pcx");
  strcat(jtp_filenames[JTP_FILE_MON_TILES_1], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_MON_TILES_1], "jtp_mon6.pcx");
  strcat(jtp_filenames[JTP_FILE_MON_TILES_2], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_MON_TILES_2], "jtp_mon7.pcx");
  strcat(jtp_filenames[JTP_FILE_MOUSE_CURSORS], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_MOUSE_CURSORS], "jtp_mou5.pcx");
  strcat(jtp_filenames[JTP_FILE_NETHACK_LOGO], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_NETHACK_LOGO], "jtp_nh_1.pcx");
  strcat(jtp_filenames[JTP_FILE_CHARACTER_GENERATION], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_CHARACTER_GENERATION], "chargen2.pcx");
  strcat(jtp_filenames[JTP_FILE_FONT_SMALL], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_FONT_SMALL], "ttipchr1.pcx");
  strcat(jtp_filenames[JTP_FILE_FONT_LARGE], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_FONT_LARGE], "menuchr2.pcx");
  strcat(jtp_filenames[JTP_FILE_WINDOW_STYLE], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_WINDOW_STYLE], "jtp_win1.pcx");
  strcat(jtp_filenames[JTP_FILE_STATUS_BAR], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_STATUS_BAR], "jtp_st10.pcx");
  strcat(jtp_filenames[JTP_FILE_MAP_PARCHMENT], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_MAP_PARCHMENT], "jtp_mwi4.pcx");
  strcat(jtp_filenames[JTP_FILE_MAP_SYMBOLS], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_MAP_SYMBOLS], "micons2.pcx");
  strcat(jtp_filenames[JTP_FILE_BACKPACK], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_BACKPACK], "backpac5.pcx");
  strcat(jtp_filenames[JTP_FILE_SPELLBOOK], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_SPELLBOOK], "book1.pcx");
  strcat(jtp_filenames[JTP_FILE_SPELL_SYMBOLS], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_SPELL_SYMBOLS], "book2.pcx");
  strcat(jtp_filenames[JTP_FILE_ENDING_DIED], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_ENDING_DIED], "cairn.pcx");
  strcat(jtp_filenames[JTP_FILE_ENDING_ASCENDED], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_ENDING_ASCENDED], "night.pcx");
  strcat(jtp_filenames[JTP_FILE_ENDING_QUIT], JTP_GRAPHICS_DIRECTORY);
  strcat(jtp_filenames[JTP_FILE_ENDING_QUIT], "quitgame.pcx");

  /* Background music filenames */
  /*
  strcat(jtp_filenames[JTP_SONG_TITLE], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_TITLE], "nethack.mid");
  strcat(jtp_filenames[JTP_SONG_INTRO], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_INTRO], "intro.mid");
  strcat(jtp_filenames[JTP_SONG_WATER_CAVES], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_WATER_CAVES], "watercav.mid");
  strcat(jtp_filenames[JTP_SONG_AIR_CAVES], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_AIR_CAVES], "aircaves.mid");
  strcat(jtp_filenames[JTP_SONG_EARTH_CAVES], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_EARTH_CAVES], "earthcav.mid");
  strcat(jtp_filenames[JTP_SONG_FIRE_CAVES], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_FIRE_CAVES], "firecave.mid");
  strcat(jtp_filenames[JTP_SONG_GNOMISH_MINES], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_GNOMISH_MINES], "mines.mid");
  strcat(jtp_filenames[JTP_SONG_MINETOWN], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_MINETOWN], "town.mid");
  strcat(jtp_filenames[JTP_SONG_ORACLE], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_ORACLE], "oracle.mid");
  strcat(jtp_filenames[JTP_SONG_LAMENT_1], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_LAMENT_1], "lament1.mid");
  strcat(jtp_filenames[JTP_SONG_LAMENT_2], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_LAMENT_2], "lament2.mid");
  strcat(jtp_filenames[JTP_SONG_AMBIENT_1], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_AMBIENT_1], "ambient1.mid");
  strcat(jtp_filenames[JTP_SONG_AMBIENT_2], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_AMBIENT_2], "ambient2.mid");
  strcat(jtp_filenames[JTP_SONG_BATTLE_1], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_BATTLE_1], "battle1.mid");
  strcat(jtp_filenames[JTP_SONG_BATTLE_2], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_BATTLE_2], "battle2.mid");
  strcat(jtp_filenames[JTP_SONG_SHOPPING], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_SHOPPING], "shopping.mid");
  strcat(jtp_filenames[JTP_SONG_ENDING_DIED], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_ENDING_DIED], "died.mid");
  strcat(jtp_filenames[JTP_SONG_ENDING_ASCENDED], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_ENDING_ASCENDED], "ascended.mid");
  strcat(jtp_filenames[JTP_SONG_ENDING_QUIT], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SONG_ENDING_QUIT], "quit.mid");
  */

  /* Sound effect filenames */
  /*
  strcat(jtp_filenames[JTP_SOUND_WALK], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SOUND_WALK], "walk.raw");
  strcat(jtp_filenames[JTP_SOUND_CLOSE_DOOR], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SOUND_CLOSE_DOOR], "cldoor.raw");
  strcat(jtp_filenames[JTP_SOUND_OPEN_DOOR], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SOUND_OPEN_DOOR], "opdoor.raw");
  strcat(jtp_filenames[JTP_SOUND_SWORD_HIT], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SOUND_SWORD_HIT], "swordhit.raw");
  strcat(jtp_filenames[JTP_SOUND_SWORD_MISS], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SOUND_SWORD_MISS], "swordmis.raw");
  strcat(jtp_filenames[JTP_SOUND_LEVEL_UP], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SOUND_LEVEL_UP], "levelup.raw");
  strcat(jtp_filenames[JTP_SOUND_CAT_MEOW], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SOUND_CAT_MEOW], "cat_meow.raw");
  strcat(jtp_filenames[JTP_SOUND_DOG_BARK], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SOUND_DOG_BARK], "dog_bark.raw");
  strcat(jtp_filenames[JTP_SOUND_HORSE_WHINNY], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SOUND_HORSE_WHINNY], "horse_wh.raw");
  strcat(jtp_filenames[JTP_SOUND_FOUNTAIN], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SOUND_FOUNTAIN], "fountain.raw");
  strcat(jtp_filenames[JTP_SOUND_COUNTING], JTP_SOUND_DIRECTORY);
  strcat(jtp_filenames[JTP_SOUND_COUNTING], "counting.raw");
  */
  /* Change backslashes to slashes in UNIX/LINUX. Not implemented yet. */
}

void
jtp_init_graphics()
{
  int i;
  int screen_width, screen_height;
  int wall_display_style;

  /* jtp_write_log_message("DEBUG[jtp_win.c/1]: Initializing filenames\n"); */
  jtp_init_filenames();

  /* jtp_write_log_message("DEBUG[jtp_win.c/2]: Reading Vulture's Eye options\n"); */
  /* Read options file */
  jtp_read_options(jtp_filenames[JTP_FILE_OPTIONS],
                   jtp_filenames[JTP_FILE_SOUNDS_CONFIG],
                   jtp_filenames[JTP_FILE_KEYS_CONFIG],
                   &screen_width,
                   &screen_height,
                   &wall_display_style);
  if (screen_width < 800) screen_width = 800;

  /* jtp_write_log_message("DEBUG[jtp_win.c/3]: Initializing screen buffer\n"); */
  /* Initialize screen and calculate some often used coordinates */  
  jtp_init_screen(screen_width, screen_height, 8);
  jtp_statusbar_x = (jtp_screen.width-JTP_STATUSBAR_WIDTH)/2;
  jtp_statusbar_y = jtp_screen.height-JTP_STATUSBAR_HEIGHT;
  jtp_map_center_x = jtp_screen.width/2;
  jtp_map_center_y = (jtp_screen.height-JTP_STATUSBAR_HEIGHT)/2;
  printf("."); fflush(stdout);

  /* jtp_write_log_message("DEBUG[jtp_win.c/3]: Initializing fonts\n"); */
  /* Load fonts */
  jtp_fonts = (jtp_font *)calloc(2, sizeof(jtp_font));
  jtp_load_font(jtp_filenames[JTP_FILE_FONT_SMALL], 0);
  jtp_load_font(jtp_filenames[JTP_FILE_FONT_LARGE], 1);
  printf("."); fflush(stdout);

  /* Load window style graphics */
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_WINDOW_STYLE], 1);
  jtp_defwin.corner_tl = jtp_get_img(1, 1, 23, 23);
  jtp_defwin.border_top = jtp_get_img(27, 1, 57, 23);
  jtp_defwin.corner_tr = jtp_get_img(61, 1, 84, 23);
  jtp_defwin.border_left = jtp_get_img(1, 27, 23, 54);
  /* jtp_defwin.center = jtp_get_img(27, 27, 57, 54); */ /* This is the small graphic... */
  jtp_defwin.center = jtp_get_img(141, 1, 238, 168);     /* But let's use this big one. */
  jtp_defwin.border_right = jtp_get_img(61, 27, 84, 54);
  jtp_defwin.corner_bl = jtp_get_img(1, 58, 23, 82);
  jtp_defwin.border_bottom = jtp_get_img(27, 58, 57, 82);
  jtp_defwin.corner_br = jtp_get_img(61, 58, 84, 82);
  jtp_defwin.checkbox_off = jtp_get_img(1, 107, 17, 123);
  jtp_defwin.checkbox_on = jtp_get_img(21, 107, 37, 123);
  jtp_defwin.radiobutton_off = jtp_get_img(41, 107, 57, 123);
  jtp_defwin.radiobutton_on = jtp_get_img(61, 107, 77, 123);
  jtp_defwin.scrollbar = jtp_get_img(81, 107, 97, 123);
  jtp_defwin.scrollbutton_down = jtp_get_img(101, 107, 117, 123);
  jtp_defwin.scrollbutton_up = jtp_get_img(121, 107, 137, 123);
  jtp_defwin.scroll_indicator = jtp_get_img(1, 127, 17, 154);
  jtp_defwin.direction_arrows = jtp_get_img(242, 1, 576, 134);
  printf("."); fflush(stdout);
  
  /* Load status bar */
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_STATUS_BAR], 1);
  jtp_statusbar = jtp_get_img(0, 0, 799, 99);  
  printf("."); fflush(stdout);
  
  /* Load map parchment */
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_MAP_PARCHMENT], 1);
  jtp_map_parchment_center = jtp_get_img(21, 18, 606, 463);
  jtp_map_parchment_top = jtp_get_img(0, 0, 639, 17);
  jtp_map_parchment_bottom = jtp_get_img(0, 464, 639, 479);
  jtp_map_parchment_left = jtp_get_img(0, 0, 20, 479);
  jtp_map_parchment_right = jtp_get_img(601, 0, 639, 479);
  printf("."); fflush(stdout);

  /* Load backpack */
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_BACKPACK], 1);
  jtp_backpack_center = jtp_get_img(23, 15, 620, 405);
  jtp_backpack_top = jtp_get_img(0, 0, 639, 14);
  jtp_backpack_bottom = jtp_get_img(0, 406, 639, 479);
  jtp_backpack_left = jtp_get_img(0, 0, 22, 479);
  jtp_backpack_right = jtp_get_img(621, 0, 639, 479);
  printf("."); fflush(stdout);

  /* Load spellbook */
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_SPELLBOOK], 1);
  jtp_spellbook_center = jtp_get_img(2, 7, 628, 430);
  jtp_spellbook_top = jtp_get_img(0, 0, 639, 6);
  jtp_spellbook_bottom = jtp_get_img(0, 431, 639, 479);
  jtp_spellbook_left = jtp_get_img(0, 0, 1, 479);
  jtp_spellbook_right = jtp_get_img(629, 0, 639, 479);
  printf("."); fflush(stdout);

  /* Load spell symbols */
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_SPELL_SYMBOLS], 1);
  for (i = 0; i < 10; i++)
    jtp_spell_symbols[i] = jtp_get_img(1 + 101*(i%6), 1 + 101*(i/6), 98 + 101*(i%6), 98 + 101*(i/6));
  printf("."); fflush(stdout);

  /* Load map symbols */
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_MAP_SYMBOLS], 1);
  for (i = 0; i < JTP_MAX_MAP_SYMBOLS; i++)
    jtp_map_symbols[i] = jtp_get_img(1 + 10*i, 1, 7 + 10*i, 14);
  printf("."); fflush(stdout);

  /* Load mouse cursors */
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_MOUSE_CURSORS], 1);
  jtp_mcursor[JTP_CURSOR_NORMAL] = jtp_get_mcursor(1, 1, 17, 24);
  jtp_mcursor[JTP_CURSOR_SCROLLRIGHT] = jtp_get_mcursor(21, 1, 40, 23);
  jtp_mcursor[JTP_CURSOR_SCROLLLEFT] = jtp_get_mcursor(44, 1, 63, 23);
  jtp_mcursor[JTP_CURSOR_SCROLLUP] = jtp_get_mcursor(67, 1, 89, 20);
  jtp_mcursor[JTP_CURSOR_SCROLLDOWN] = jtp_get_mcursor(93, 1, 115, 20);
  jtp_mcursor[JTP_CURSOR_TARGET_GREEN] = jtp_get_mcursor(119, 1, 169, 26);
  jtp_mcursor[JTP_CURSOR_TARGET_RED] = jtp_get_mcursor(173, 1, 223, 26);
  jtp_mcursor[JTP_CURSOR_TARGET_INVALID] = jtp_get_mcursor(227, 1, 273, 26);
  jtp_mcursor[JTP_CURSOR_TARGET_HELP] = jtp_get_mcursor(1, 30, 51, 79);
  jtp_mcursor[JTP_CURSOR_HOURGLASS] = jtp_get_mcursor(277, 1, 306, 33);
  jtp_mcursor[JTP_CURSOR_OPENDOOR] = jtp_get_mcursor(310, 1, 342, 30);
  jtp_mcursor[JTP_CURSOR_STAIRS] = jtp_get_mcursor(346, 1, 383, 35);
  jtp_mcursor[JTP_CURSOR_GOBLET] = jtp_get_mcursor(312, 34, 336, 68);
  printf("."); fflush(stdout);

  /* Set message shading */
  for (i = 0; i < JTP_MAX_MESSAGE_COLORS; i++)
    jtp_message_colors[i] = 20 + i/2;
  jtp_message_colors[0] = 15;  
  jtp_message_colors[1] = 17;

  /* Initialize the isometric tiles */
  /* jtp_write_log_message("[jtp_win.c/jtp_init_graphics/Debug8] Initializing tile conversion.\n"); */
 /* jtp_init_glyph_tiles(); */     /* Initialize glyph-tile correspondence tables just before game starts */
  /* jtp_write_log_message("[jtp_win.c/jtp_init_graphics/Debug9] Loading tile graphics.\n"); */
  jtp_init_tilegraphics(wall_display_style);  /* Initialize tile bitmaps */
  /* jtp_write_log_message("[jtp_win.c/jtp_init_graphics/Debug9b] Tile graphics loaded.\n"); */
  printf("."); fflush(stdout);
 
  /* jtp_template_conversion(); */
 
  
  /* jtp_write_log_message("[jtp_win.c/jtp_init_graphics/Debug10] Vulture's Eye window system ready.\n"); */

  jtp_clear_screen();  /* Clear the screen buffer */
  /* Enter graphics mode */
  jtp_enter_graphics_mode(); 
  jtp_blankpal(0,255); /* Set palette to all black */
  jtp_set_draw_region(0, 0, jtp_screen.width, jtp_screen.height);

  /* 
   * Set regular game palette. If any function changes the palette
   * (fade-ins, etc.), it is expected to restore the palette before
   * it returns.
   */
  jtp_load_PCX(0, 0, jtp_filenames[JTP_FILE_MOUSE_CURSORS], 1);
  jtp_clear_screen();
  jtp_refresh();
  jtp_updatepal(0, 255);
  jtp_game_palette_set = 1;

/*
  jtp_write_log_message("DEBUGMESSAGE window system ready\n");  
  jtp_messagebox("Vulture's Eye windowing system is active.");
*/
}

void
jtp_get_menu_coordinates(menuwindow)
jtp_window * menuwindow;
{
  int i, j, k;
  int totalwidth, totalheight;
  int menuitems_width, menuitems_height;
  int prompt_width, prompt_height;
  int buttons_width, buttons_height;
  int top_separator_height, bottom_separator_height;
  jtp_menuitem * tempmenuitem;
  jtp_button * tempbutton;
  char tempbuffer[1024];

  if (!menuwindow) return;
    
  i = jtp_defwin.border_top[1];
  j = jtp_defwin.border_left[3];

  /* Calculate width and height of prompt */
  prompt_width = jtp_text_length(menuwindow->menu->prompt, JTP_FONT_HEADLINE);
  prompt_height = jtp_text_height(menuwindow->menu->prompt, JTP_FONT_HEADLINE); 
  menuwindow->menu->prompt_x = j;
  menuwindow->menu->prompt_y = i;

  /* If there is a prompt, add a small separator between it and the window content. */
  if (menuwindow->menu->prompt)
    top_separator_height = jtp_fonts[JTP_FONT_HEADLINE].lineheight;
  else top_separator_height = 0;
  
  /* Calculate width and height of each menu item */
  menuitems_width = 0;
  menuitems_height = 0;
  menuwindow->menu->items_y = jtp_defwin.border_top[1] + prompt_height + top_separator_height;
  menuwindow->menu->need_scrollbar = 0;
  i = 0; /* Menuitem y-coordinates are offsets from menuwindow->menu->items_y */

  if (menuwindow->menu)
  {
    jtp_list_reset(menuwindow->menu->items);
    tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
    while (tempmenuitem)
    {
      tempmenuitem->width = jtp_text_length(tempmenuitem->text, JTP_FONT_MENU);
      if (tempmenuitem->count != JTP_NOT_SELECTABLE)
        switch (menuwindow->menu->selectiontype)
        {
          case PICK_NONE: tempmenuitem->width += jtp_defwin.radiobutton_off[3] + 4; break;
          case PICK_ONE: tempmenuitem->width += jtp_defwin.radiobutton_off[3] + 4; break;
          case PICK_ANY: tempmenuitem->width += jtp_defwin.checkbox_off[3] + 4; break;
          default: break;
        }
      tempmenuitem->height = jtp_fonts[JTP_FONT_MENU].lineheight;
      /* If the content is not textual, add some extra space between the items */
      if (!menuwindow->menu->content_is_text) tempmenuitem->height += 10;
      
      tempmenuitem->x = j;
      tempmenuitem->y = i;
      if (menuitems_width < tempmenuitem->width) menuitems_width = tempmenuitem->width;
      
      i += tempmenuitem->height + 4;
      
      if (menuitems_height + tempmenuitem->height + 4 < JTP_MAX_MENUITEMS_HEIGHT)
        menuitems_height += tempmenuitem->height + 4;
      else
        menuwindow->menu->need_scrollbar = 1;
    
      jtp_list_advance(menuwindow->menu->items);
      tempmenuitem = jtp_list_current(menuwindow->menu->items);
    }
  }  

  /* If the scrollbar is needed, add its width to total width of menuitems */
  if (menuwindow->menu->need_scrollbar)
    menuitems_width += jtp_defwin.scrollbar[3];

  /* Add a small separator between the menu items and the buttons */
  if (menuwindow->menu->content_is_text)
    bottom_separator_height = jtp_fonts[JTP_FONT_MENU].lineheight;
  else bottom_separator_height = 0;    
    
  /* Calculate width and height of buttons */
  buttons_width = 0;
  buttons_height = 0;
  i = jtp_defwin.border_top[1] + prompt_height + top_separator_height + 
      menuitems_height + bottom_separator_height;
  j = 0; /* Buttons will be centered when the total window width is known */
  if (menuwindow->buttons)
  {
    jtp_list_reset(menuwindow->buttons);
    tempbutton = (jtp_button *)jtp_list_current(menuwindow->buttons);
    while (tempbutton)
    {
      tempbutton->width = jtp_text_length(tempbutton->text, JTP_FONT_BUTTON) + 11;
      tempbutton->height = jtp_text_height(tempbutton->text, JTP_FONT_BUTTON) + 10;
      tempbutton->x = j;
      tempbutton->y = i;
      if (buttons_height < tempbutton->height) buttons_height = tempbutton->height;
      buttons_width += tempbutton->width + 4;
      j += tempbutton->width + 4;
      
      jtp_list_advance(menuwindow->buttons);
      tempbutton = (jtp_button *)jtp_list_current(menuwindow->buttons);
    }
  }  
  
  /* Calculate total width and height from components */
  totalheight = prompt_height + top_separator_height + menuitems_height + 
                bottom_separator_height + buttons_height;
  totalwidth = prompt_width;
  if (menuitems_width > totalwidth) totalwidth = menuitems_width;
  if (buttons_width > totalwidth) totalwidth = buttons_width;
  totalwidth += jtp_defwin.border_left[3] + jtp_defwin.border_right[3];
  totalheight += jtp_defwin.border_top[1] + jtp_defwin.border_bottom[1];
  
  menuwindow->width = totalwidth;
  menuwindow->height = totalheight;

  /* If the scrollbar is needed, calculate its position */
  if (menuwindow->menu->need_scrollbar)
  {
    menuwindow->menu->scrollup_y = menuwindow->menu->items_y;
    menuwindow->menu->scrolldown_y = menuwindow->menu->items_y + menuitems_height - jtp_defwin.scrollbutton_down[1];
    menuwindow->menu->scrollbar_x = totalwidth - jtp_defwin.border_right[3] - jtp_defwin.scrollbar[3];
  }

  /* Center the buttons in the available space */
  j = (totalwidth - jtp_defwin.border_left[3] - jtp_defwin.border_right[3] - buttons_width)/ 2
      + jtp_defwin.border_left[3];
  if (menuwindow->buttons)
  {
    jtp_list_reset(menuwindow->buttons);
    tempbutton = (jtp_button *)jtp_list_current(menuwindow->buttons);
    while (tempbutton)
    {
      tempbutton->x += j;
      jtp_list_advance(menuwindow->buttons);
      tempbutton = (jtp_button *)jtp_list_current(menuwindow->buttons);
    }
  }  
  

  /* Calculate window position */  
  menuwindow->x = (jtp_screen.width - totalwidth)/2;
  menuwindow->y = (jtp_screen.height - totalheight)/2;

  /*
  sprintf(tempbuffer, "Width: %d, height: %d\nX %d, Y %d", menuwindow->width, menuwindow->height, menuwindow->x, menuwindow->y);
  jtp_messagebox(tempbuffer);
  */
}




int
jtp_get_menu_selection(menuwindow)
jtp_window * menuwindow;
{
  int selectedbutton;
  jtp_menuitem * tempmenuitem, * tempmenuitem2, * firstitem;
  jtp_button * tempbutton;
  unsigned char * menubackground;
  unsigned char * tempimage;
  int pressedkey;
  char widget_found;
  clock_t cur_time;
  int backpack_type_selection = 0;
  int n_menuitems, target_item;
  char tempbuffer[1024];
  char used_accelerators[1024];
  char n_accelerators;
  int temp_accelerator;

  if ((!menuwindow) || (!menuwindow->menu) || (!menuwindow->buttons))
    return(1);

  if (jtp_is_spellbook_being_viewed)
  {
    jtp_view_spellbook(menuwindow);
    return(1);
  }

  /*
   * If this is a 'Remove what types' menu, we need to support the backpack
   * screen shortcuts, so we need to automatically select which types of
   * items to remove. This is a really awkward solution.
   */
  if ((menuwindow->menu->prompt) &&
      (jtp_backpack_shortcut_action == jtp_translate_command(JTP_NHCMD_REMOVE_ITEM)) &&  /* 'Inactive' allowed. This is an exception to the normal rule. */
      (!strcmp(menuwindow->menu->prompt, "What type of things do you want to take off?")))
  {
    selectedbutton = 1;
    /* Set all menu items to 'not selected' */
    jtp_list_reset(menuwindow->menu->items);
    tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
    while (tempmenuitem2)
    {
      tempmenuitem2->selected = FALSE;
      jtp_list_advance(menuwindow->menu->items);
      tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);              
    }

    /* Set 'all types' to selected */
    jtp_list_reset(menuwindow->menu->items);
    tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
    tempmenuitem->selected = TRUE;
    tempmenuitem->count = -1;

    return(selectedbutton);
  }

  /*
   * If this is a 'Remove what' or 'Cast what' menu, we need to support
   * the backpack/spellbook screen shortcuts, so we need to automatically
   * select which item to remove/spell to cast.
   * This is a really awkward solution.
   */
  if ((menuwindow->menu->prompt) &&
      (jtp_backpack_shortcut_action == jtp_translate_command(JTP_NHCMD_REMOVE_ITEM)) &&  /* 'Inactive' allowed. This is an exception to the normal rule. */
      (!strcmp(menuwindow->menu->prompt, "What do you want to take off?")))
    backpack_type_selection = 1;

  if ((menuwindow->menu->prompt) &&
      (jtp_backpack_shortcut_action == jtp_translate_command(JTP_NHCMD_CAST_SPELL)) &&  /* 'Inactive' allowed. This is an exception to the normal rule. */
      (!strcmp(menuwindow->menu->prompt, "Choose which spell to cast")))
    backpack_type_selection = 1;

  if (backpack_type_selection)
  {
    /* Set all menu items to 'not selected' */
    jtp_list_reset(menuwindow->menu->items);
    tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
    while (tempmenuitem2)
    {
      tempmenuitem2->selected = FALSE;
      jtp_list_advance(menuwindow->menu->items);
      tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
    }

    /* Find the correct item by its shortcut accelerator, and select it */
    jtp_list_reset(menuwindow->menu->items);
    tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
    while (tempmenuitem2)
    {
      if (tempmenuitem2->accelerator == jtp_shortcut_query_response)
      {
        tempmenuitem2->selected = TRUE;
        tempmenuitem2->count = -1;
      }
      jtp_list_advance(menuwindow->menu->items);
      tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
    }

    /* The shortcut has now been used, so make sure it is not used twice */
    jtp_is_backpack_shortcut_active = 0;
    jtp_is_shortcut_active = 0;
    jtp_shortcut_query_response = 0;
    jtp_backpack_shortcut_action = 0;

    selectedbutton = 1;
    return(selectedbutton);
  }
  
  /* Find number of menu items in the menu */
  n_menuitems = 0;
  jtp_list_reset(menuwindow->menu->items);
  tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
  while (tempmenuitem)
  {
    n_menuitems++;
    jtp_list_advance(menuwindow->menu->items);
    tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
  }

  /* Store background graphics and draw window components */
  menubackground = jtp_draw_window(menuwindow->x, menuwindow->y, 
                                   menuwindow->width, menuwindow->height);
  if (!menubackground)
    jtp_messagebox("Error: Could not get menu background");

  jtp_list_reset(menuwindow->menu->items);
  firstitem = jtp_list_current(menuwindow->menu->items);
  jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);

  jtp_draw_buttons(menuwindow->x, menuwindow->y, menuwindow->buttons);

  jtp_refresh();
  /* If the palette is faded out for some reason, restore it */
  jtp_updatepal(0, 255);

  /*
  jtp_messagebox("Ready for input.");
  */

  selectedbutton = -1;
  while (selectedbutton < 0)
  {
    jtp_keymouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
    if (jtp_kbhit()) pressedkey = jtp_getch();
    else pressedkey = 0;

    cur_time = clock();

    widget_found = 0;
    jtp_list_reset(menuwindow->buttons);
    tempbutton = (jtp_button *)jtp_list_current(menuwindow->buttons);
    while ((tempbutton) && (selectedbutton < 0))
    {
      if ((jtp_mouseb == JTP_MBUTTON_LEFT) &&
          (jtp_in_area(jtp_mousex, jtp_mousey, 
                       menuwindow->x + tempbutton->x, 
                       menuwindow->y + tempbutton->y,
                       menuwindow->x + tempbutton->x + tempbutton->width - 1,
                       menuwindow->y + tempbutton->y + tempbutton->height - 1)))
        widget_found = 1;
      else if ((pressedkey) && (pressedkey == tempbutton->accelerator))
        widget_found = 1;
      if (widget_found)
      {
        /* Wait until mouse button is released */
        jtp_press_button(menuwindow->x + tempbutton->x + 1, 
                         menuwindow->y + tempbutton->y + 1,
                         menuwindow->x + tempbutton->x + tempbutton->width-2,
                         menuwindow->y + tempbutton->y + tempbutton->height-2,
                         jtp_mcursor[JTP_CURSOR_NORMAL]);
        selectedbutton = tempbutton->id;
      }        
      
      jtp_list_advance(menuwindow->buttons);
      tempbutton = (jtp_button *)jtp_list_current(menuwindow->buttons);
    }
    
    
    jtp_list_reset(menuwindow->menu->items);
    tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
    while ((tempmenuitem) && (tempmenuitem != firstitem))
    {
      jtp_list_advance(menuwindow->menu->items);
      tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
    }  
    
    widget_found = 0;
    while ((selectedbutton < 0) && (!widget_found) && (tempmenuitem) && 
           (tempmenuitem->y - firstitem->y + tempmenuitem->height < JTP_MAX_MENUITEMS_HEIGHT))
    {
      
      if ((jtp_mouseb == JTP_MBUTTON_LEFT) && 
          (jtp_in_area(jtp_mousex, jtp_mousey, 
                       menuwindow->x + tempmenuitem->x, 
                       menuwindow->y + menuwindow->menu->items_y + 
                         tempmenuitem->y - firstitem->y,
                       menuwindow->x + tempmenuitem->x + tempmenuitem->width - 1,
                       menuwindow->y + menuwindow->menu->items_y + 
                         tempmenuitem->y - firstitem->y + tempmenuitem->height - 1)))
        widget_found = 1;
      else if ((pressedkey) && (pressedkey == tempmenuitem->accelerator))
        widget_found = 1;
        
      if (widget_found)
      {
        /* Wait until mouse button is released */
        jtp_repeatmouse(jtp_mcursor[JTP_CURSOR_NORMAL], JTP_MBUTTON_NONE);

        if (tempmenuitem->count != JTP_NOT_SELECTABLE)
        {        
          switch (menuwindow->menu->selectiontype)
          {
            case PICK_NONE: break;
            case PICK_ONE:
              /* Set all menu items to 'not selected' */
              jtp_list_reset(menuwindow->menu->items);
              tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
              while (tempmenuitem2)
              {
                tempmenuitem2->selected = FALSE;
                jtp_list_advance(menuwindow->menu->items);
                tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);              
              }
              tempmenuitem->selected = TRUE;
              tempmenuitem->count = -1;
              break;
            case PICK_ANY: 
              if (tempmenuitem->selected == FALSE) tempmenuitem->selected = TRUE;
              else tempmenuitem->selected = FALSE;
              tempmenuitem->count = -1;
              break;
            default: break;
          }
        
          /* Redraw changed menu (background not redrawn since menuitem position is constant) */
          jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);
          jtp_refresh();
        }
                
      }
      
      jtp_list_advance(menuwindow->menu->items);
      tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
    }

    if (cur_time-jtp_last_scroll_time > jtp_min_scroll_delay*CLOCKS_PER_SEC)
    {
      widget_found = 0;

      /* Click on the scroll up button */
      if ((selectedbutton < 0) && (menuwindow->menu->need_scrollbar) &&
          (jtp_in_area(jtp_mousex, jtp_mousey, 
                       menuwindow->x + menuwindow->menu->scrollbar_x,
                       menuwindow->y + menuwindow->menu->scrollup_y,
                       menuwindow->x + menuwindow->menu->scrollbar_x +
                         jtp_defwin.scrollbutton_up[3] - 1,
                       menuwindow->y + menuwindow->menu->scrollup_y +
                         jtp_defwin.scrollbutton_up[1] - 1)))
      {
        widget_found = 1; target_item = 1;
      }
      else if (pressedkey == JTP_KEY_MENU_SCROLLUP)
      {
        widget_found = 1; target_item = 1;
      }
      else if (pressedkey == JTP_KEY_MENU_SCROLLPAGEUP)
      {
        widget_found = 1; target_item = 10;
      }

      if (widget_found == 1)
      {
        /* Find the menuitem that comes 'target_item' steps before 'firstitem' */
        jtp_list_reset(menuwindow->menu->items);
        tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        while ((tempmenuitem) && (tempmenuitem != firstitem))
        {
          jtp_list_advance(menuwindow->menu->items);
          tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        }
        while (target_item > 0)
        {
          jtp_list_retreat(menuwindow->menu->items);
          target_item--;
        }
        firstitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        
        /* Redraw whole window */
        tempimage = jtp_draw_window(menuwindow->x, menuwindow->y, 
                                    menuwindow->width, menuwindow->height);
        free(tempimage);
        if (!menubackground) jtp_messagebox("Error: Could not get menu background");
        jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);    
        jtp_draw_buttons(menuwindow->x, menuwindow->y, menuwindow->buttons);
        jtp_refresh();
        jtp_last_scroll_time = cur_time;
        widget_found = 2; /* Found and processed */
      }

      /* Click on the scroll down button */
      if ((selectedbutton < 0) && (!widget_found) && (menuwindow->menu->need_scrollbar) &&
          (jtp_in_area(jtp_mousex, jtp_mousey, 
                       menuwindow->x + menuwindow->menu->scrollbar_x,
                       menuwindow->y + menuwindow->menu->scrolldown_y,
                       menuwindow->x + menuwindow->menu->scrollbar_x +
                         jtp_defwin.scrollbutton_down[3] - 1,
                       menuwindow->y + menuwindow->menu->scrolldown_y +
                         jtp_defwin.scrollbutton_down[1] - 1)))
      {
        widget_found = 1; target_item = 1;
      }
      else if (pressedkey == JTP_KEY_MENU_SCROLLDOWN)
      {
        widget_found = 1; target_item = 1;
      }
      else if (pressedkey == JTP_KEY_MENU_SCROLLPAGEDOWN)
      {
        widget_found = 1; target_item = 10;
      }

      if (widget_found == 1)
      {
        /* Find the menuitem that comes 'target_item' steps after 'firstitem' */
        jtp_list_reset(menuwindow->menu->items);
        tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        while ((tempmenuitem) && (tempmenuitem != firstitem))
        {
          jtp_list_advance(menuwindow->menu->items);
          tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        }
        while (target_item > 0)
        {
          jtp_list_advance(menuwindow->menu->items);
          /* Don't go past the end */
          if (!(jtp_menuitem *)jtp_list_current(menuwindow->menu->items))
          {
            jtp_list_retreat(menuwindow->menu->items);
            target_item = 0;
          }
          target_item--;
        }
        if ((jtp_menuitem *)jtp_list_current(menuwindow->menu->items))
          firstitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
  
        /* Redraw whole window */
        tempimage = jtp_draw_window(menuwindow->x, menuwindow->y, 
                                    menuwindow->width, menuwindow->height);
        free(tempimage);                             
        jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);    
        jtp_draw_buttons(menuwindow->x, menuwindow->y, menuwindow->buttons);
        jtp_refresh();
        jtp_last_scroll_time = cur_time;
        widget_found = 2; /* Found and processed */
      }

      /* Click on the scrollbar itself */
      if ((selectedbutton < 0) && (!widget_found) && (menuwindow->menu->need_scrollbar) &&
          (jtp_in_area(jtp_mousex, jtp_mousey, 
                       menuwindow->x + menuwindow->menu->scrollbar_x,
                       menuwindow->y + menuwindow->menu->scrollup_y +
                         jtp_defwin.scrollbutton_up[1],
                       menuwindow->x + menuwindow->menu->scrollbar_x +
                         jtp_defwin.scrollbutton_down[3] - 1,
                       menuwindow->y + menuwindow->menu->scrolldown_y - 1)))
        widget_found = 1;

      if (widget_found == 1)
      {
        target_item = jtp_mousey - menuwindow->y - menuwindow->menu->scrollup_y -
                      jtp_defwin.scrollbutton_up[1];
        target_item *= (n_menuitems-1);
        target_item /= menuwindow->menu->scrolldown_y - 1 - menuwindow->menu->scrollup_y -
                       jtp_defwin.scrollbutton_up[1];
/*
        sprintf(tempbuffer,"mouse y: %d, y1: %d, y2: %d, nitems: %d, target item: %d\n\0",
                jtp_mousey,
                  
        jtp_write_log_message("[jtp_win.c/jtp_show_intro/Check2] Out of memory!\n");
*/ 
        /* Find the target menuitem */
        jtp_list_reset(menuwindow->menu->items);
        tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        while ((tempmenuitem) && (target_item > 0))
        {
          jtp_list_advance(menuwindow->menu->items);
          tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
          target_item--;
        }
        if ((jtp_menuitem *)jtp_list_current(menuwindow->menu->items))
          firstitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
  
        /* Redraw whole window */
        tempimage = jtp_draw_window(menuwindow->x, menuwindow->y, 
                                    menuwindow->width, menuwindow->height);
        free(tempimage);                             
        jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);    
        jtp_draw_buttons(menuwindow->x, menuwindow->y, menuwindow->buttons);
        jtp_refresh();
        jtp_last_scroll_time = cur_time;
        widget_found = 2; /* Found and processed */
      }
    }
  }

  /* Restore the window background */
  jtp_set_draw_region(0, 0, jtp_screen.width-1, jtp_screen.height-1);
  jtp_put_img(menuwindow->x, menuwindow->y, menubackground);
  jtp_refresh_region(menuwindow->x, menuwindow->y, 
                     menuwindow->x + menuwindow->width - 1,
                     menuwindow->y + menuwindow->height - 1);
  
  /* Clean up and exit */
  free(menubackground);  
  return(selectedbutton);
}

void
jtp_exit_graphics()
{
  jtp_exit_graphics_mode();  
}

/* End of jtp_win.c */
