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

/* Help commands */

#define JTP_NHCMD_HELP_MENU 74
#define JTP_NHCMD_EXPLAIN_COMMAND 75
#define JTP_NHCMD_EXPLAIN_ONSCREEN_SYMBOL 76
#define JTP_NHCMD_EXPLAIN_SYMBOL 77
#define JTP_NHCMD_EXPLAIN_TRAP 78
#define JTP_NHCMD_LIST_DISCOVERED_OBJECTS 79

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

/* That's all! (Some commands don't have keyboard shortcuts; they're not listed here.) */
#define JTP_MAX_NETHACK_COMMANDS 93

/* Command key binding structure */

typedef struct {
  char * description;
  int nhkey;          /* NetHack key without number_pad option */
  int nhkey_numpad;   /* NetHack key with number_pad option */
  int key;            /* Vulture's Eye customizable key */
} jtp_command;

/* Command table */
extern jtp_command * jtp_keymaps;

/* Function definitions */

int jtp_find_command_name_index(char * cmd_name);
int jtp_find_command_key_index(int cmd_key);
int jtp_find_command_nhkey_index(int cmd_key);

int jtp_translate_key(int cmd_key);
int jtp_translate_command(int cmd_index);

void jtp_read_key_configuration(char * key_config_file);
void jtp_set_nethack_keys();
void jtp_set_default_keymaps();





