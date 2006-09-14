/*	SCCS Id: @(#)jtp_keys.c	1.0	2001/6/05	*/
/* Copyright (c) Jaakko Peltonen, 2001				  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "jtp_keys.h"


/* Copied from 'cmd.c' */

#ifndef META
# ifndef NHSTDC
#  define META(c) (0x80 | (c))
# else
#  define META(c) ((c) - 128)
# endif /* NHSTDC */
#endif

#ifndef CTRL
#define CTRL(c) (0x1f & (c))
#endif


/* Local variables */

jtp_command * jtp_keymaps = NULL;
int jtp_last_translated_key;


/* Function implementations */

/* Find the index of a command by its description */
int jtp_find_command_name_index(char * cmd_name)
{
  int i;
 
  if (!cmd_name) return(JTP_NHCMD_UNKNOWN);
 
  for (i = 0; i < JTP_MAX_NETHACK_COMMANDS; i++)
    if (!strcmp(jtp_keymaps[i].description, cmd_name))
      return(i);

  /* Command name is not listed */
  return(JTP_NHCMD_UNKNOWN);
}


/* Find the index of a command by its customized key */
int jtp_find_command_key_index(int cmd_key)
{
  int i;
  
  for (i = 0; i < JTP_MAX_NETHACK_COMMANDS; i++)
    if (jtp_keymaps[i].key == cmd_key)
      return(i);

  /* Command key is not listed */
  return(JTP_NHCMD_UNKNOWN);
}


/* Find the index of a command by its original NetHack key */
int jtp_find_command_nhkey_index(int cmd_key)
{
  int i;
  
  for (i = 0; i < JTP_MAX_NETHACK_COMMANDS; i++)
  {
    if (iflags.num_pad)
    { 
      if (jtp_keymaps[i].nhkey_numpad == cmd_key)
        return(i);
    }
    else if (jtp_keymaps[i].nhkey == cmd_key)
      return(i);
  }

  /* Command key is not listed */
  return(JTP_NHCMD_UNKNOWN);
}


/* Translate a customized key to the original key */
int jtp_translate_key(int cmd_key)
{
  int i;

  /* Count keys aren't translated */
  if ((iflags.num_pad) && 
      (jtp_last_translated_key == jtp_keymaps[JTP_NHCMD_REPEATED_COMMAND].nhkey_numpad) &&
      (cmd_key > '0') && (cmd_key < '9'))
  {
    i = cmd_key;
    /* Since this is a count, don't update jtp_last_translated_key */
  }
  else
  {   
    i = jtp_find_command_key_index(cmd_key);
    if (i == JTP_NHCMD_UNKNOWN) i = cmd_key;
    else
    { 
      if (iflags.num_pad) i = jtp_keymaps[i].nhkey_numpad;
      else i = jtp_keymaps[i].nhkey;
    }
    jtp_last_translated_key = i;
  }
  return(i);  
}


/* Translate a command index into a NetHack key */
int jtp_translate_command(int cmd_index)
{
  if (iflags.num_pad) return(jtp_keymaps[cmd_index].nhkey_numpad);
  else return(jtp_keymaps[cmd_index].nhkey);
}


/* Read command key options from a file */
void jtp_read_key_configuration(char * key_config_file)
{
  FILE * f;
  char line_buffer[1024], cmd_name[1024];
  char * tok;
  int cmd_index, new_key;

  f = fopen(key_config_file, "rb");
  while (fgets(line_buffer, 1024, f))
  {
    if ((line_buffer[0] == '%') || (line_buffer[0] == '\0') ||
        (line_buffer[0] == 10) || (line_buffer[0] == 13))
    {
      /* Comment or empty line. Skip. */
    }
    else
    {
      /* Find separating "]=[" between command name and key setting */
      tok = strstr(line_buffer, "]=[");
      if (tok) /* Valid separator found */
      {
        /* Find out which NetHack command we're configuring */
        memcpy(cmd_name, line_buffer+1, tok-line_buffer-1);
        cmd_name[tok-line_buffer-1] = 0;

        cmd_index = jtp_find_command_name_index(cmd_name);
        if (cmd_index >= 0)
	{
          tok = tok + strlen("]=[") - 1;

          /* Parse the key setting */
          /* The key is one of four types: [+] [a] [META+a] [CTRL+a] */
          if (tok[1] == '+') new_key = '+';
          else if (tok[2] == ']') new_key = tok[1];
          else if (!strncmp(tok, "[META+", 6)) new_key = tok[6] + 128;
          else if (!strncmp(tok, "[CTRL+", 6)) new_key = (0x1f & tok[6]);

          /* Create the mapping */
          jtp_keymaps[cmd_index].key = new_key;

          /* 
          sprintf(line_buffer, "Mapped [%s] (index %d) to %c\n", 
                  cmd_name, cmd_index, new_key);
          jtp_write_log_message(line_buffer); 
          */
	}
      }
    }
  }

  /* File processed */
  fclose(f);
}


/* 
 * Assign NetHack keys, and also the default key mappings ("none"). 
 * Here we have to check the "number pad" option.
 */
void jtp_set_nethack_keys()
{
  int cmd_index;

  for (cmd_index = 0; cmd_index < JTP_MAX_NETHACK_COMMANDS; cmd_index++)
  {
    jtp_keymaps[cmd_index].nhkey_numpad = 0; 
    jtp_keymaps[cmd_index].key = 0;
  }

  /* Movement commands */

  cmd_index = JTP_NHCMD_NORTH;
  jtp_keymaps[cmd_index].description = "north";
  jtp_keymaps[cmd_index].nhkey_numpad = '8'; 
  jtp_keymaps[cmd_index].nhkey = 'k';

  cmd_index = JTP_NHCMD_SOUTH;
  jtp_keymaps[cmd_index].description = "south";
  jtp_keymaps[cmd_index].nhkey_numpad = '2'; 
  jtp_keymaps[cmd_index].nhkey = 'j';

  cmd_index = JTP_NHCMD_EAST;
  jtp_keymaps[cmd_index].description = "east";
  jtp_keymaps[cmd_index].nhkey_numpad = '6'; 
  jtp_keymaps[cmd_index].nhkey = 'l';

  cmd_index = JTP_NHCMD_WEST;
  jtp_keymaps[cmd_index].description = "west";
  jtp_keymaps[cmd_index].nhkey_numpad = '4'; 
  jtp_keymaps[cmd_index].nhkey = 'h';

  cmd_index = JTP_NHCMD_NORTHEAST;
  jtp_keymaps[cmd_index].description = "northeast";
  jtp_keymaps[cmd_index].nhkey_numpad = '9'; 
  jtp_keymaps[cmd_index].nhkey = 'u';

  cmd_index = JTP_NHCMD_NORTHWEST;
  jtp_keymaps[cmd_index].description = "northwest";
  jtp_keymaps[cmd_index].nhkey_numpad = '7'; 
  jtp_keymaps[cmd_index].nhkey = 'y';

  cmd_index = JTP_NHCMD_SOUTHEAST;
  jtp_keymaps[cmd_index].description = "southeast";
  jtp_keymaps[cmd_index].nhkey_numpad = '3'; 
  jtp_keymaps[cmd_index].nhkey = 'n';

  cmd_index = JTP_NHCMD_SOUTHWEST;
  jtp_keymaps[cmd_index].description = "southwest";
  jtp_keymaps[cmd_index].nhkey_numpad = '1'; 
  jtp_keymaps[cmd_index].nhkey = 'b';

  cmd_index = JTP_NHCMD_MOVEPREFIX_NO_FIGHT_PICKUP;
  jtp_keymaps[cmd_index].description = "move prefix: no fight/pickup";
  jtp_keymaps[cmd_index].nhkey = 'm'; 

  cmd_index = JTP_NHCMD_MOVEPREFIX_FIGHT;
  jtp_keymaps[cmd_index].description = "move prefix: fight";
  jtp_keymaps[cmd_index].nhkey = 'F'; 

  cmd_index = JTP_NHCMD_MOVEPREFIX_FAR_NOPICKUP;
  jtp_keymaps[cmd_index].description = "move prefix: far, no pickup";
  jtp_keymaps[cmd_index].nhkey = 'M'; 

  cmd_index = JTP_NHCMD_MOVEPREFIX_INTERESTING;
  jtp_keymaps[cmd_index].description = "move prefix: until interesting";
  jtp_keymaps[cmd_index].nhkey = 'g'; 

  cmd_index = JTP_NHCMD_MOVEPREFIX_INTERESTING_NONFORK;
  jtp_keymaps[cmd_index].description = "move prefix: until interesting nonfork";
  jtp_keymaps[cmd_index].nhkey = 'G'; 

  cmd_index = JTP_NHCMD_UP;
  jtp_keymaps[cmd_index].description = "up";
  jtp_keymaps[cmd_index].nhkey = '<'; 

  cmd_index = JTP_NHCMD_DOWN;
  jtp_keymaps[cmd_index].description = "down";
  jtp_keymaps[cmd_index].nhkey = '>'; 

  cmd_index = JTP_NHCMD_JUMP;
  jtp_keymaps[cmd_index].description = "jump";
  jtp_keymaps[cmd_index].nhkey_numpad = 'j'; 
  jtp_keymaps[cmd_index].nhkey = META('j');

  cmd_index = JTP_NHCMD_SIT;
  jtp_keymaps[cmd_index].description = "sit down";
  jtp_keymaps[cmd_index].nhkey = META('s'); 

  cmd_index = JTP_NHCMD_TELEPORT;
  jtp_keymaps[cmd_index].description = "teleport";
  jtp_keymaps[cmd_index].nhkey = CTRL('t'); 

  /* Exploring */

  cmd_index = JTP_NHCMD_KICK;
  jtp_keymaps[cmd_index].description = "kick";
  jtp_keymaps[cmd_index].nhkey_numpad = 'k'; 
  jtp_keymaps[cmd_index].nhkey = CTRL('d');

  cmd_index = JTP_NHCMD_CLOSE_DOOR;
  jtp_keymaps[cmd_index].description = "close door";
  jtp_keymaps[cmd_index].nhkey = 'c'; 

  cmd_index = JTP_NHCMD_OPEN_DOOR;
  jtp_keymaps[cmd_index].description = "open door";
  jtp_keymaps[cmd_index].nhkey = 'o'; 

  cmd_index = JTP_NHCMD_LOOK_HERE;
  jtp_keymaps[cmd_index].description = "look here";
  jtp_keymaps[cmd_index].nhkey = ':'; 

  cmd_index = JTP_NHCMD_SEARCH;
  jtp_keymaps[cmd_index].description = "search";
  jtp_keymaps[cmd_index].nhkey = 's'; 

  cmd_index = JTP_NHCMD_UNTRAP;
  jtp_keymaps[cmd_index].description = "untrap";
  jtp_keymaps[cmd_index].nhkey_numpad = 'u'; 
  jtp_keymaps[cmd_index].nhkey = META('u');

  /* Getting new items */

  cmd_index = JTP_NHCMD_FORCE_LOCK;
  jtp_keymaps[cmd_index].description = "force lock";
  jtp_keymaps[cmd_index].nhkey = META('f'); 

  cmd_index = JTP_NHCMD_LOOT;
  jtp_keymaps[cmd_index].description = "loot chest";
  jtp_keymaps[cmd_index].nhkey_numpad = 'l'; 
  jtp_keymaps[cmd_index].nhkey = META('l');

  cmd_index = JTP_NHCMD_PICKUP;
  jtp_keymaps[cmd_index].description = "pick up";
  jtp_keymaps[cmd_index].nhkey = ','; 

  cmd_index = JTP_NHCMD_TOGGLE_AUTOPICKUP;
  jtp_keymaps[cmd_index].description = "toggle autopickup";
  jtp_keymaps[cmd_index].nhkey = '@'; 

  /* Browsing your inventory */

  cmd_index = JTP_NHCMD_INVENTORY_LETTERS;
  jtp_keymaps[cmd_index].description = "assign inventory letters";
  jtp_keymaps[cmd_index].nhkey = META('a'); 

  cmd_index = JTP_NHCMD_COUNT_GOLD;
  jtp_keymaps[cmd_index].description = "count gold";
  jtp_keymaps[cmd_index].nhkey = GOLD_SYM; 

  cmd_index = JTP_NHCMD_LIST_INVENTORY;
  jtp_keymaps[cmd_index].description = "list inventory";
  jtp_keymaps[cmd_index].nhkey = 'i'; 

  cmd_index = JTP_NHCMD_LIST_PARTIAL_INVENTORY;
  jtp_keymaps[cmd_index].description = "list partial inventory";
  jtp_keymaps[cmd_index].nhkey = 'I'; 

  cmd_index = JTP_NHCMD_TELL_WEAPON;
  jtp_keymaps[cmd_index].description = "tell weapon";
  jtp_keymaps[cmd_index].nhkey = WEAPON_SYM; 

  cmd_index = JTP_NHCMD_TELL_ARMOR;
  jtp_keymaps[cmd_index].description = "tell armor";
  jtp_keymaps[cmd_index].nhkey = ARMOR_SYM; 

  cmd_index = JTP_NHCMD_TELL_RINGS;
  jtp_keymaps[cmd_index].description = "tell rings";
  jtp_keymaps[cmd_index].nhkey = RING_SYM; 

  cmd_index = JTP_NHCMD_TELL_AMULET;
  jtp_keymaps[cmd_index].description = "tell amulet";
  jtp_keymaps[cmd_index].nhkey = WEAPON_SYM; 

  cmd_index = JTP_NHCMD_TELL_TOOLS;
  jtp_keymaps[cmd_index].description = "tell tools";
  jtp_keymaps[cmd_index].nhkey = TOOL_SYM; 

  cmd_index = JTP_NHCMD_TELL_EQUIPMENT;
  jtp_keymaps[cmd_index].description = "tell equipment";
  jtp_keymaps[cmd_index].nhkey = '*'; 

  /* Wearing and wielding items */

  cmd_index = JTP_NHCMD_PUT_ON_ACCESSORY;
  jtp_keymaps[cmd_index].description = "put on accessory";
  jtp_keymaps[cmd_index].nhkey = 'P'; 

  cmd_index = JTP_NHCMD_SELECT_QUIVER;
  jtp_keymaps[cmd_index].description = "select quiver";
  jtp_keymaps[cmd_index].nhkey = 'Q'; 

  cmd_index = JTP_NHCMD_WEAR_ARMOR;
  jtp_keymaps[cmd_index].description = "wear armor";
  jtp_keymaps[cmd_index].nhkey = 'W'; 

  cmd_index = JTP_NHCMD_WIELD_WEAPON;
  jtp_keymaps[cmd_index].description = "wield weapon";
  jtp_keymaps[cmd_index].nhkey = 'w'; 

  /* Using items */

  cmd_index = JTP_NHCMD_APPLY;
  jtp_keymaps[cmd_index].description = "apply";
  jtp_keymaps[cmd_index].nhkey = 'a'; 

  cmd_index = JTP_NHCMD_DIP;
  jtp_keymaps[cmd_index].description = "dip object";
  jtp_keymaps[cmd_index].nhkey = META('d'); 

  cmd_index = JTP_NHCMD_EAT;
  jtp_keymaps[cmd_index].description = "eat";
  jtp_keymaps[cmd_index].nhkey = 'e'; 

  cmd_index = JTP_NHCMD_FIRE_QUIVER;
  jtp_keymaps[cmd_index].description = "fire quiver";
  jtp_keymaps[cmd_index].nhkey = 'f'; 

  cmd_index = JTP_NHCMD_INVOKE;
  jtp_keymaps[cmd_index].description = "invoke object";
  jtp_keymaps[cmd_index].nhkey = META('i'); 

  cmd_index = JTP_NHCMD_OFFER;
  jtp_keymaps[cmd_index].description = "offer object";
  jtp_keymaps[cmd_index].nhkey = META('o'); 

  cmd_index = JTP_NHCMD_QUAFF;
  jtp_keymaps[cmd_index].description = "quaff";
  jtp_keymaps[cmd_index].nhkey = 'q'; 

  cmd_index = JTP_NHCMD_READ;
  jtp_keymaps[cmd_index].description = "read";
  jtp_keymaps[cmd_index].nhkey = 'r'; 

  cmd_index = JTP_NHCMD_RUB;
  jtp_keymaps[cmd_index].description = "rub object";
  jtp_keymaps[cmd_index].nhkey = META('r'); 

  cmd_index = JTP_NHCMD_THROW;
  jtp_keymaps[cmd_index].description = "throw";
  jtp_keymaps[cmd_index].nhkey = 't'; 

  cmd_index = JTP_NHCMD_ZAP;
  jtp_keymaps[cmd_index].description = "zap wand";
  jtp_keymaps[cmd_index].nhkey = 'z'; 

  /* Getting rid of items */

  cmd_index = JTP_NHCMD_REMOVE_ACCESSORY;
  jtp_keymaps[cmd_index].description = "remove accessory";
  jtp_keymaps[cmd_index].nhkey = 'R'; 

  cmd_index = JTP_NHCMD_REMOVE_ITEM;
  jtp_keymaps[cmd_index].description = "remove worn item";
  jtp_keymaps[cmd_index].nhkey = 'A'; 

  cmd_index = JTP_NHCMD_TAKE_OFF_ARMOR;
  jtp_keymaps[cmd_index].description = "take off armor";
  jtp_keymaps[cmd_index].nhkey = 'T'; 

  cmd_index = JTP_NHCMD_DROP;
  jtp_keymaps[cmd_index].description = "drop";
  jtp_keymaps[cmd_index].nhkey = 'd'; 

  cmd_index = JTP_NHCMD_DROP_SEVERAL;
  jtp_keymaps[cmd_index].description = "drop several";
  jtp_keymaps[cmd_index].nhkey = 'D'; 

  /* Other actions */

  cmd_index = JTP_NHCMD_CAST_SPELL;
  jtp_keymaps[cmd_index].description = "cast spell";
  jtp_keymaps[cmd_index].nhkey = 'Z'; 

  cmd_index = JTP_NHCMD_CHAT;
  jtp_keymaps[cmd_index].description = "chat with monster";
  jtp_keymaps[cmd_index].nhkey = META('c'); 

  cmd_index = JTP_NHCMD_ENGRAVE;
  jtp_keymaps[cmd_index].description = "engrave";
  jtp_keymaps[cmd_index].nhkey = 'E'; 

  cmd_index = JTP_NHCMD_ENHANCE_WEAPON_SKILL;
  jtp_keymaps[cmd_index].description = "enhance weapon skill";
  jtp_keymaps[cmd_index].nhkey = META('e'); 

  cmd_index = JTP_NHCMD_EXCHANGE_WEAPON;
  jtp_keymaps[cmd_index].description = "exchange weapon";
  jtp_keymaps[cmd_index].nhkey = 'x'; 

  cmd_index = JTP_NHCMD_LIST_SPELLS;
  jtp_keymaps[cmd_index].description = "list spells";
  jtp_keymaps[cmd_index].nhkey = SPBOOK_SYM; 

  cmd_index = JTP_NHCMD_MONSTER_ABILITY;
  jtp_keymaps[cmd_index].description = "monster ability";
  jtp_keymaps[cmd_index].nhkey = META('m'); 

  cmd_index = JTP_NHCMD_PAY_BILL;
  jtp_keymaps[cmd_index].description = "pay bill";
  jtp_keymaps[cmd_index].nhkey = 'p'; 

  cmd_index = JTP_NHCMD_PRAY;
  jtp_keymaps[cmd_index].description = "pray";
  jtp_keymaps[cmd_index].nhkey = META('p'); 

  cmd_index = JTP_NHCMD_REST;
  jtp_keymaps[cmd_index].description = "rest";
  jtp_keymaps[cmd_index].nhkey = '.'; 

  cmd_index = JTP_NHCMD_TURN_UNDEAD;
  jtp_keymaps[cmd_index].description = "turn undead";
  jtp_keymaps[cmd_index].nhkey = META('t'); 

  cmd_index = JTP_NHCMD_TWO_WEAPON_MODE;
  jtp_keymaps[cmd_index].description = "two-weapon mode";
  jtp_keymaps[cmd_index].nhkey = META('2'); 

  cmd_index = JTP_NHCMD_WIPE_FACE;
  jtp_keymaps[cmd_index].description = "wipe face";
  jtp_keymaps[cmd_index].nhkey = META('w'); 

  cmd_index = JTP_NHCMD_DISPLAY_ROLE;
  jtp_keymaps[cmd_index].description = "display role";
  jtp_keymaps[cmd_index].nhkey = CTRL('x'); 

  cmd_index = JTP_NHCMD_NAME_MONSTER;
  jtp_keymaps[cmd_index].description = "name monster";
  jtp_keymaps[cmd_index].nhkey = 'C'; 

  cmd_index = JTP_NHCMD_NAME_OBJECT;
  jtp_keymaps[cmd_index].description = "name object";
  jtp_keymaps[cmd_index].nhkey_numpad = 'N'; 
  jtp_keymaps[cmd_index].nhkey = META('n');

  /* Help commands */

  cmd_index = JTP_NHCMD_HELP_MENU;
  jtp_keymaps[cmd_index].description = "help menu";
  jtp_keymaps[cmd_index].nhkey = '?'; 

  cmd_index = JTP_NHCMD_EXPLAIN_COMMAND;
  jtp_keymaps[cmd_index].description = "explain command";
  jtp_keymaps[cmd_index].nhkey = '&'; 

  cmd_index = JTP_NHCMD_EXPLAIN_ONSCREEN_SYMBOL;
  jtp_keymaps[cmd_index].description = "explain onscreen symbol";
  jtp_keymaps[cmd_index].nhkey = ';'; 

  cmd_index = JTP_NHCMD_EXPLAIN_SYMBOL;
  jtp_keymaps[cmd_index].description = "explain symbol";
  jtp_keymaps[cmd_index].nhkey = '/'; 

  cmd_index = JTP_NHCMD_EXPLAIN_TRAP;
  jtp_keymaps[cmd_index].description = "explain trap";
  jtp_keymaps[cmd_index].nhkey = '^'; 

  cmd_index = JTP_NHCMD_LIST_DISCOVERED_OBJECTS;
  jtp_keymaps[cmd_index].description = "list discovered objects";
  jtp_keymaps[cmd_index].nhkey = '\\'; 

  /* Miscellaneous commands */

  cmd_index = JTP_NHCMD_QUIT_GAME;
  jtp_keymaps[cmd_index].description = "quit game";
  jtp_keymaps[cmd_index].nhkey = META('q'); 

  cmd_index = JTP_NHCMD_SAVE_GAME;
  jtp_keymaps[cmd_index].description = "save game";
  jtp_keymaps[cmd_index].nhkey = 'S'; 

  cmd_index = JTP_NHCMD_SET_OPTIONS;
  jtp_keymaps[cmd_index].description = "set options";
  jtp_keymaps[cmd_index].nhkey = 'O'; 

  cmd_index = JTP_NHCMD_DISPLAY_VERSION;
  jtp_keymaps[cmd_index].description = "display version";
  jtp_keymaps[cmd_index].nhkey = 'v'; 

  cmd_index = JTP_NHCMD_DISPLAY_VERSION_EXTENDED;
  jtp_keymaps[cmd_index].description = "display extended version";
  jtp_keymaps[cmd_index].nhkey = META('v'); 

  cmd_index = JTP_NHCMD_DISPLAY_HISTORY;
  jtp_keymaps[cmd_index].description = "display history";
  jtp_keymaps[cmd_index].nhkey = 'V'; 

  cmd_index = JTP_NHCMD_ESCAPE_TO_SHELL;
  jtp_keymaps[cmd_index].description = "escape to shell";
  jtp_keymaps[cmd_index].nhkey = '!'; 

  cmd_index = JTP_NHCMD_PREVIOUS_MESSAGE;
  jtp_keymaps[cmd_index].description = "previous message";
  jtp_keymaps[cmd_index].nhkey = CTRL('p'); 

  cmd_index = JTP_NHCMD_SUSPEND;
  jtp_keymaps[cmd_index].description = "suspend";
  jtp_keymaps[cmd_index].nhkey = CTRL('z'); 

  cmd_index = JTP_NHCMD_REDRAW_SCREEN;
  jtp_keymaps[cmd_index].description = "redraw screen";
  jtp_keymaps[cmd_index].nhkey_numpad = CTRL('l'); 
  jtp_keymaps[cmd_index].nhkey = CTRL('r');

  cmd_index = JTP_NHCMD_EXPLORE_MODE;
  jtp_keymaps[cmd_index].description = "explore mode";
  jtp_keymaps[cmd_index].nhkey = 'X'; 

  cmd_index = JTP_NHCMD_EXTENDED_COMMAND;
  jtp_keymaps[cmd_index].description = "prefix: extended command";
  jtp_keymaps[cmd_index].nhkey = '#'; 

  cmd_index = JTP_NHCMD_REPEATED_COMMAND;
  jtp_keymaps[cmd_index].description = "prefix: repeated command";
  jtp_keymaps[cmd_index].nhkey = 'n'; 

  for (cmd_index = 0; cmd_index < JTP_MAX_NETHACK_COMMANDS; cmd_index++)
    if (!jtp_keymaps[cmd_index].nhkey_numpad)
      jtp_keymaps[cmd_index].nhkey_numpad = jtp_keymaps[cmd_index].nhkey;
}


void jtp_set_default_keymaps()
{
  int cmd_index;

  /* 
   * Assign the original NetHack keys
   * as the default Vulture's Eye keys.
   */   
  for (cmd_index = 0; cmd_index < JTP_MAX_NETHACK_COMMANDS; cmd_index++)
  {
    if (iflags.num_pad)
      jtp_keymaps[cmd_index].key = jtp_keymaps[cmd_index].nhkey_numpad;
    else
      jtp_keymaps[cmd_index].key = jtp_keymaps[cmd_index].nhkey;
  }
}

/* End of jtp_keys.c */
