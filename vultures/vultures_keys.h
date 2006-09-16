/*	SCCS Id: @(#)jtp_keys.h	1.0	2001/6/10	*/
/* Copyright (c) Jaakko Peltonen, 2001				  */
/* NetHack may be freely redistributed.  See license for details. */


enum jtp_keycmd {
  JTP_NHCMD_UNKNOWN = -1,

/* Movement commands */
  JTP_NHCMD_NORTH,
  JTP_NHCMD_SOUTH,
  JTP_NHCMD_EAST,
  JTP_NHCMD_WEST,
  JTP_NHCMD_NORTHEAST,
  JTP_NHCMD_NORTHWEST,
  JTP_NHCMD_SOUTHEAST,
  JTP_NHCMD_SOUTHWEST,

  JTP_NHCMD_MOVEPREFIX_NO_FIGHT_PICKUP,
  JTP_NHCMD_MOVEPREFIX_FIGHT,
  JTP_NHCMD_MOVEPREFIX_FAR_NOPICKUP,
  JTP_NHCMD_MOVEPREFIX_INTERESTING,
  JTP_NHCMD_MOVEPREFIX_INTERESTING_NONFORK,

  JTP_NHCMD_UP,
  JTP_NHCMD_DOWN,
  JTP_NHCMD_JUMP,
  JTP_NHCMD_SIT,
  JTP_NHCMD_TELEPORT,
  JTP_NHCMD_TRAVEL,

/* Exploring */

  JTP_NHCMD_KICK,
  JTP_NHCMD_CLOSE_DOOR,
  JTP_NHCMD_OPEN_DOOR,
  JTP_NHCMD_LOOK_HERE,
  JTP_NHCMD_SEARCH,
  JTP_NHCMD_UNTRAP,

/* Getting new items */

  JTP_NHCMD_FORCE_LOCK,
  JTP_NHCMD_LOOT,
  JTP_NHCMD_PICKUP,
  JTP_NHCMD_TOGGLE_AUTOPICKUP,

/* Browsing your inventory */

  JTP_NHCMD_INVENTORY_LETTERS,
  JTP_NHCMD_COUNT_GOLD,
  JTP_NHCMD_LIST_INVENTORY,
  JTP_NHCMD_LIST_PARTIAL_INVENTORY,
  JTP_NHCMD_TELL_WEAPON,
  JTP_NHCMD_TELL_ARMOR,
  JTP_NHCMD_TELL_RINGS,
  JTP_NHCMD_TELL_AMULET,
  JTP_NHCMD_TELL_TOOLS,
  JTP_NHCMD_TELL_EQUIPMENT,

/* Wearing and wielding items */

  JTP_NHCMD_PUT_ON_ACCESSORY,
  JTP_NHCMD_SELECT_QUIVER,
  JTP_NHCMD_WEAR_ARMOR,
  JTP_NHCMD_WIELD_WEAPON,

/* Using items */

  JTP_NHCMD_APPLY,
  JTP_NHCMD_DIP,
  JTP_NHCMD_EAT,
  JTP_NHCMD_FIRE_QUIVER,
  JTP_NHCMD_INVOKE,
  JTP_NHCMD_OFFER,
  JTP_NHCMD_QUAFF,
  JTP_NHCMD_READ,
  JTP_NHCMD_RUB,
  JTP_NHCMD_THROW,
  JTP_NHCMD_ZAP,

/* Getting rid of items */

  JTP_NHCMD_REMOVE_ACCESSORY,
  JTP_NHCMD_REMOVE_ITEM,
  JTP_NHCMD_TAKE_OFF_ARMOR,
  JTP_NHCMD_DROP,
  JTP_NHCMD_DROP_SEVERAL,

/* Other actions */

  JTP_NHCMD_CAST_SPELL,
  JTP_NHCMD_CHAT,
  JTP_NHCMD_ENGRAVE,
  JTP_NHCMD_ENHANCE_WEAPON_SKILL,
  JTP_NHCMD_EXCHANGE_WEAPON,
  JTP_NHCMD_LIST_SPELLS,
  JTP_NHCMD_MONSTER_ABILITY,
  JTP_NHCMD_PAY_BILL,
  JTP_NHCMD_PRAY,
  JTP_NHCMD_REST,
  JTP_NHCMD_TURN_UNDEAD,
  JTP_NHCMD_TWO_WEAPON_MODE,
  JTP_NHCMD_WIPE_FACE,

  JTP_NHCMD_DISPLAY_ROLE,
  JTP_NHCMD_NAME_MONSTER,
  JTP_NHCMD_NAME_OBJECT,
  JTP_NHCMD_PLAYERSTEAL,
  JTP_NHCMD_POLYATWILL,
  JTP_NHCMD_BORG_TOGGLE,
  JTP_NHCMD_LIST_VANQUISHED,
  JTP_NHCMD_USE_TECHNIQUES,

/* Help commands */

  JTP_NHCMD_HELP_MENU,
  JTP_NHCMD_EXPLAIN_COMMAND,
  JTP_NHCMD_EXPLAIN_ONSCREEN_SYMBOL,
  JTP_NHCMD_EXPLAIN_SYMBOL,
  JTP_NHCMD_EXPLAIN_TRAP,
  JTP_NHCMD_LIST_DISCOVERED_OBJECTS,
  JTP_NHCMD_MAIN_MENU,

/* Miscellaneous commands */

  JTP_NHCMD_QUIT_GAME,
  JTP_NHCMD_SAVE_GAME,
  JTP_NHCMD_SET_OPTIONS,
  JTP_NHCMD_DISPLAY_VERSION,
  JTP_NHCMD_DISPLAY_VERSION_EXTENDED,
  JTP_NHCMD_DISPLAY_HISTORY,

  JTP_NHCMD_ESCAPE_TO_SHELL,
  JTP_NHCMD_PREVIOUS_MESSAGE,
  JTP_NHCMD_REDRAW_SCREEN,
  JTP_NHCMD_EXPLORE_MODE,

  JTP_NHCMD_EXTENDED_COMMAND,
  JTP_NHCMD_REPEATED_COMMAND,
  JTP_NHCMD_SHOW_MAP,
  JTP_NHCMD_SAVE_SCREENSHOT,

/* Wizard commands */

  JTP_NHCMD_WIZ_GAIN_AC,
  JTP_NHCMD_WIZ_DETECT,
  JTP_NHCMD_WIZ_MAGIC_MAPPING,
  JTP_NHCMD_WIZ_GENESIS,
  JTP_NHCMD_WIZ_IDENTIFY,
  JTP_NHCMD_WIZ_GAIN_LEVEL,
  JTP_NHCMD_WIZ_INVULNERABILITY,
  JTP_NHCMD_WIZ_WHERE,
  JTP_NHCMD_WIZ_LEVEL_TELEPORT,
  JTP_NHCMD_WIZ_MAKE_WISH,
  JTP_NHCMD_WIZ_LIGHT_SOURCES,
  JTP_NHCMD_WIZ_SHOW_STATS,
  JTP_NHCMD_WIZ_SHOW_SEEN_VECTORS,
  JTP_NHCMD_WIZ_TIMEOUT_QUEUE,
  JTP_NHCMD_WIZ_SHOW_VISION,
  JTP_NHCMD_WIZ_WALL_MODES,
  JTP_NHCMD_WIZ_DEBUG,

/* non-key that takes jtp_translate_key out of "counting mode" */
  JTP_NHCMD_PSEUDO_BARRIER,

/* That's all! (Some commands don't have keyboard shortcuts; they're not listed here.) */
  JTP_MAX_NETHACK_COMMANDS
};

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





