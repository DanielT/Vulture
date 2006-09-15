/*	SCCS Id: @(#)jtp_keys.h	1.0	2001/6/10	*/
/* Copyright (c) Jaakko Peltonen, 2001				  */
/* NetHack may be freely redistributed.  See license for details. */



#define JTP_NHCMD_UNKNOWN -1

/* Movement commands */
#define JTP_NHCMD_NORTH 0
#define JTP_NHCMD_SOUTH 1
#define JTP_NHCMD_EAST 2
#define JTP_NHCMD_WEST 3
#define JTP_NHCMD_NORTHEAST 4
#define JTP_NHCMD_NORTHWEST 5
#define JTP_NHCMD_SOUTHEAST 6
#define JTP_NHCMD_SOUTHWEST 7

#define JTP_NHCMD_MOVEPREFIX_NO_FIGHT_PICKUP 8
#define JTP_NHCMD_MOVEPREFIX_FIGHT 9
#define JTP_NHCMD_MOVEPREFIX_FAR_NOPICKUP 10
#define JTP_NHCMD_MOVEPREFIX_INTERESTING 11
#define JTP_NHCMD_MOVEPREFIX_INTERESTING_NONFORK 12

#define JTP_NHCMD_UP 13
#define JTP_NHCMD_DOWN 14
#define JTP_NHCMD_JUMP 15
#define JTP_NHCMD_SIT 16
#define JTP_NHCMD_TELEPORT 17
#define JTP_NHCMD_TRAVEL 115

/* Exploring */

#define JTP_NHCMD_KICK 18
#define JTP_NHCMD_CLOSE_DOOR 19
#define JTP_NHCMD_OPEN_DOOR 20
#define JTP_NHCMD_LOOK_HERE 21
#define JTP_NHCMD_SEARCH 22
#define JTP_NHCMD_UNTRAP 23

/* Getting new items */

#define JTP_NHCMD_FORCE_LOCK 24
#define JTP_NHCMD_LOOT 25
#define JTP_NHCMD_PICKUP 26
#define JTP_NHCMD_TOGGLE_AUTOPICKUP 27

/* Browsing your inventory */

#define JTP_NHCMD_INVENTORY_LETTERS 28
#define JTP_NHCMD_COUNT_GOLD 29
#define JTP_NHCMD_LIST_INVENTORY 30
#define JTP_NHCMD_LIST_PARTIAL_INVENTORY 31
#define JTP_NHCMD_TELL_WEAPON 32
#define JTP_NHCMD_TELL_ARMOR 33
#define JTP_NHCMD_TELL_RINGS 34
#define JTP_NHCMD_TELL_AMULET 35
#define JTP_NHCMD_TELL_TOOLS 36
#define JTP_NHCMD_TELL_EQUIPMENT 37

/* Wearing and wielding items */

#define JTP_NHCMD_PUT_ON_ACCESSORY 38
#define JTP_NHCMD_SELECT_QUIVER 39
#define JTP_NHCMD_WEAR_ARMOR 40
#define JTP_NHCMD_WIELD_WEAPON 41

/* Using items */

#define JTP_NHCMD_APPLY 42
#define JTP_NHCMD_DIP 43
#define JTP_NHCMD_EAT 44
#define JTP_NHCMD_FIRE_QUIVER 45
#define JTP_NHCMD_INVOKE 46
#define JTP_NHCMD_OFFER 47
#define JTP_NHCMD_QUAFF 48
#define JTP_NHCMD_READ 49
#define JTP_NHCMD_RUB 50
#define JTP_NHCMD_THROW 51
#define JTP_NHCMD_ZAP 52

/* Getting rid of items */

#define JTP_NHCMD_REMOVE_ACCESSORY 53
#define JTP_NHCMD_REMOVE_ITEM 54
#define JTP_NHCMD_TAKE_OFF_ARMOR 55
#define JTP_NHCMD_DROP 56
#define JTP_NHCMD_DROP_SEVERAL 57

/* Other actions */

#define JTP_NHCMD_CAST_SPELL 58
#define JTP_NHCMD_CHAT 59
#define JTP_NHCMD_ENGRAVE 60
#define JTP_NHCMD_ENHANCE_WEAPON_SKILL 61
#define JTP_NHCMD_EXCHANGE_WEAPON 62
#define JTP_NHCMD_LIST_SPELLS 63
#define JTP_NHCMD_MONSTER_ABILITY 64
#define JTP_NHCMD_PAY_BILL 65
#define JTP_NHCMD_PRAY 66
#define JTP_NHCMD_REST 67
#define JTP_NHCMD_TURN_UNDEAD 68
#define JTP_NHCMD_TWO_WEAPON_MODE 69
#define JTP_NHCMD_WIPE_FACE 70

#define JTP_NHCMD_DISPLAY_ROLE 71
#define JTP_NHCMD_NAME_MONSTER 72
#define JTP_NHCMD_NAME_OBJECT 73
#define JTP_NHCMD_PLAYERSTEAL 93
#define JTP_NHCMD_POLYATWILL 111
#define JTP_NHCMD_BORG_TOGGLE 112
#define JTP_NHCMD_LIST_VANQUISHED 113
#define JTP_NHCMD_USE_TECHNIQUES 114

/* Help commands */

#define JTP_NHCMD_HELP_MENU 74
#define JTP_NHCMD_EXPLAIN_COMMAND 75
#define JTP_NHCMD_EXPLAIN_ONSCREEN_SYMBOL 76
#define JTP_NHCMD_EXPLAIN_SYMBOL 77
#define JTP_NHCMD_EXPLAIN_TRAP 78
#define JTP_NHCMD_LIST_DISCOVERED_OBJECTS 79
#define JTP_NHCMD_MAIN_MENU 116

/* Miscellaneous commands */

#define JTP_NHCMD_QUIT_GAME 80
#define JTP_NHCMD_SAVE_GAME 81
#define JTP_NHCMD_SET_OPTIONS 82
#define JTP_NHCMD_DISPLAY_VERSION 83
#define JTP_NHCMD_DISPLAY_VERSION_EXTENDED 84
#define JTP_NHCMD_DISPLAY_HISTORY 85

#define JTP_NHCMD_ESCAPE_TO_SHELL 86
#define JTP_NHCMD_PREVIOUS_MESSAGE 87
#define JTP_NHCMD_SUSPEND 88
#define JTP_NHCMD_REDRAW_SCREEN 89
#define JTP_NHCMD_EXPLORE_MODE 90

#define JTP_NHCMD_EXTENDED_COMMAND 91
#define JTP_NHCMD_REPEATED_COMMAND 92
#define JTP_NHCMD_SHOW_MAP 117
#define JTP_NHCMD_SAVE_SCREENSHOT 118

/* Wizard commands */

#define JTP_NHCMD_WIZ_GAIN_AC 94
#define JTP_NHCMD_WIZ_DETECT 95
#define JTP_NHCMD_WIZ_MAGIC_MAPPING 96
#define JTP_NHCMD_WIZ_GENESIS 97
#define JTP_NHCMD_WIZ_IDENTIFY 98
#define JTP_NHCMD_WIZ_GAIN_LEVEL 99
#define JTP_NHCMD_WIZ_INVULNERABILITY 100
#define JTP_NHCMD_WIZ_WHERE 101
#define JTP_NHCMD_WIZ_LEVEL_TELEPORT 102
#define JTP_NHCMD_WIZ_MAKE_WISH 103
#define JTP_NHCMD_WIZ_LIGHT_SOURCES 104
#define JTP_NHCMD_WIZ_SHOW_STATS 105
#define JTP_NHCMD_WIZ_SHOW_SEEN_VECTORS 106
#define JTP_NHCMD_WIZ_TIMEOUT_QUEUE 107
#define JTP_NHCMD_WIZ_SHOW_VISION 108
#define JTP_NHCMD_WIZ_WALL_MODES 109
#define JTP_NHCMD_WIZ_DEBUG 110

/* That's all! (Some commands don't have keyboard shortcuts; they're not listed here.) */
#define JTP_MAX_NETHACK_COMMANDS 119

/* Command key binding structure */

typedef struct {
  const char * description;
  int nhkey;          /* NetHack key without number_pad option */
  int nhkey_numpad;   /* NetHack key with number_pad option */
  int key;            /* Vulture's customizable key */
  unsigned int flags;
} jtp_command;
#define JTP_KEYFLAG_WIZARD  0x01

/* Command table */
extern jtp_command * jtp_keymaps;

/*
 * Key constants for non-ascii keys; the platform-dependent 
 * code must make sure these are assigned correctly
 * tho: this used to define negative key values for function
 * keys like cursor up/down etc, but those might conflict
 * with keys where the meta-bit is set.
 * Also, make sure to not pass this keys to the nethack core,
 * as these values have meanings only for the window port.
 */
#define JTP_KEY_MENU_SCROLLUP 0x100
#define JTP_KEY_MENU_SCROLLDOWN 0x101
#define JTP_KEY_MENU_SCROLLLEFT 0x102
#define JTP_KEY_MENU_SCROLLRIGHT 0x103
#define JTP_KEY_MENU_SCROLLPAGEUP 0x104
#define JTP_KEY_MENU_SCROLLPAGEDOWN 0x105
#define JTP_KEY_PAUSE 0x106
#define JTP_KEY_INSERT 0x107
#define JTP_KEY_HOME 0x108
#define JTP_KEY_END 0x109
#define JTP_KEY_PRINT_SCREEN 0x110

/*
 * Special symbols that aren't passed to the core nethack functions.
 * Nethack uses ascii keys (0-0x7f), plus keys in range 0x80-0xff
 * for Meta-Keys.
 */
#define JTP_MOUSEWHEEL_UP   0x110
#define JTP_MOUSEWHEEL_DOWN 0x111

/* Function definitions */

int jtp_find_command_name_index(const char * cmd_name);
int jtp_find_command_key_index(int cmd_key);
int jtp_find_command_nhkey_index(int cmd_key);

int jtp_translate_key(int cmd_key);
int jtp_translate_command(int cmd_index);
int jtp_getkey(int cmd_index);

void jtp_read_key_configuration(char * key_config_file);
void jtp_set_nethack_keys(void);
void jtp_set_default_keymaps(void);





