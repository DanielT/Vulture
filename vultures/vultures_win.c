/*	SCCS Id: @(#)jtp_win.c	3.0	2000/9/10	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdio.h>
#include <math.h>
#if defined(__GNUC__)
#include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>
#include "vultures_gen.h"
#include "vultures_mou.h"
#include "vultures_txt.h"
#include "vultures_gfl.h"
#include "vultures_keys.h"
#include "vultures_win.h"
#include "vultures_gametiles.h"
#include "vultures_sdl.h"
#include "vultures_map.h"
#include "vultures_init.h"
#include "vultures_main.h"

#include "epri.h"
#include "global.h"
#include "rm.h"
#include "display.h"
#include "patchlevel.h"

/*----------------------------------------------------------------
  Defines (constants)
-----------------------------------------------------------------*/
/* Command used by NetHack for travel (from src/cmd.c) */
#define CMD_TRAVEL (char)0x90

/* Directory separator: DOS-style (backslash) or Unix-style (slash) */
#ifdef UNIX
#define JTP_USE_UNIX_STYLE_DIRECTORY_SEPARATOR
#endif
#ifdef __BEOS__
#define JTP_USE_UNIX_STYLE_DIRECTORY_SEPARATOR
#endif



/* Maximum height (in pixels) of displayed menu items */
#define JTP_MAX_MENUITEMS_HEIGHT 450

/* Magic pixel positions within the statusbar graphic  */
#define STATUS_XPOS 220
#define STATUS_YPOS  15

/* Dimensions of NetHack logo screen & character generation graphic */
#define JTP_NH_LOGO_WIDTH 800
#define JTP_NH_LOGO_HEIGHT 600

	
/* On-screen hotspots */
enum jtp_hotspot {
  JTP_HOTSPOT_NONE = 0,
  JTP_HOTSPOT_SCROLL_UP,
  JTP_HOTSPOT_SCROLL_DOWN,
  JTP_HOTSPOT_SCROLL_LEFT,
  JTP_HOTSPOT_SCROLL_RIGHT,
  JTP_HOTSPOT_MAP,
  JTP_HOTSPOT_MINI_MAP,
  JTP_HOTSPOT_STATUSBAR,
  JTP_HOTSPOT_BUTTON_LOOK,
  JTP_HOTSPOT_BUTTON_EXTENDED,
  JTP_HOTSPOT_BUTTON_MAP,
  JTP_HOTSPOT_BUTTON_SPELLBOOK,
  JTP_HOTSPOT_BUTTON_INVENTORY,
  JTP_HOTSPOT_BUTTON_MESSAGES,
  JTP_HOTSPOT_BUTTON_OPTIONS,
  JTP_HOTSPOT_BUTTON_HELP
};

/* Floor pattern dimensions (eg. 3x3) */
#define JTP_FLOOR_PATTERN_WIDTH 3
#define JTP_FLOOR_PATTERN_HEIGHT 3

/* Carpet dimensions (eg. 3x2) */
#define JTP_CARPET_WIDTH 3
#define JTP_CARPET_HEIGHT 3


typedef struct{
  char * str;
  int    action_id;
} jtp_dropdown_action;

/* Shortcut actions */
enum jtp_action {
  JTP_ACTION_NONE = 0,
  JTP_ACTION_CHAT,
  JTP_ACTION_CLOSE_DOOR,
  JTP_ACTION_ENGRAVE,
  JTP_ACTION_FORCE_LOCK,
  JTP_ACTION_GO_DOWN,
  JTP_ACTION_GO_UP,
  JTP_ACTION_KICK,
  JTP_ACTION_LOOT,
  JTP_ACTION_MONSTER_ABILITY,
  JTP_ACTION_MOVE_HERE,
  JTP_ACTION_OPEN_DOOR,
  JTP_ACTION_PAY_BILL,
  JTP_ACTION_PICK_UP,
  JTP_ACTION_PRAY,
  JTP_ACTION_PUSH_BOULDER,
  JTP_ACTION_REST,
  JTP_ACTION_RIDE,
  JTP_ACTION_SEARCH,
  JTP_ACTION_SIT,
  JTP_ACTION_TURN_UNDEAD,
  JTP_ACTION_UNTRAP,
  JTP_ACTION_WIPE_FACE,
  JTP_ACTION_DRINK,
  JTP_ACTION_LOOK_AROUND,
  JTP_ACTION_WHATS_THIS,
  JTP_ACTION_APPLY_ITEM,
  JTP_ACTION_DRINK_ITEM,
  JTP_ACTION_DROP_ITEM,
  JTP_ACTION_EAT_ITEM,
  JTP_ACTION_READ_ITEM,
  JTP_ACTION_REMOVE_ITEM,
  JTP_ACTION_WEAR_ITEM,
  JTP_ACTION_PUT_ON_ITEM,
  JTP_ACTION_WIELD_ITEM,
  JTP_ACTION_ZAP_ITEM,
  JTP_ACTION_ENTER_TRAP,
  JTP_ACTION_CAST_SPELL,
  JTP_ACTION_ATTACK,
};

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
char *jtp_game_path;


/* Interface options */
int jtp_recenter_after_movement = 0;
int jtp_play_music = 0;
int jtp_play_effects = 0;
int jtp_one_command_per_click = 0;
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
double jtp_last_scroll_time = 0;

/* Window list */
jtp_list * jtp_windowlist = NULL; /* Linked list of all windows */
int jtp_max_window_id = -1;     /* Largest window id given */

/* Autopilot */
int jtp_movebuffer[2];         /* Target squares for 'autopilot' */
int jtp_move_length = 0;       /* Length of autopilot sequence */
int jtp_autopilot_type;    /* Autopilot type (movement or whatis) */

/* Query response for shortcut-activated commands */
int jtp_is_shortcut_active = 0;
int jtp_shortcut_query_response;

/* Shortcut actions selected from the backpack or spellbook screen */
int jtp_is_backpack_shortcut_active = 0;
int jtp_backpack_shortcut_action;

/* General map info */
int jtp_map_tgtx, jtp_map_tgty; /* Selected square on map */


/* Status bar location */
int jtp_statusbar_x, jtp_statusbar_y;

/* Message window */
int jtp_messages_height;   /* height of messages shown on-screen */
int jtp_first_shown_message = 0; /* Index of newest shown message (>= 0), used by the "previous message" command */
unsigned char * jtp_messages_background = NULL; /* Messages overlap this part of map window */
unsigned char jtp_message_colors[JTP_MAX_MESSAGE_COLORS]; /* Message age shading */
unsigned char jtp_warn_colors[JTP_MAX_WARN];

/* Bitmap graphics */
jtp_window_graphics jtp_defwin; /* Basic window graphics */
unsigned char * jtp_statusbar = NULL; /* Status bar graphic */

unsigned char * jtp_backpack_center = NULL; /* Backpack graphic, center area */
unsigned char * jtp_backpack_top = NULL;    /* Backpack graphic, top border */
unsigned char * jtp_backpack_bottom = NULL; /* Backpack graphic, bottom border */
unsigned char * jtp_backpack_left = NULL;   /* Backpack graphic, left border */
unsigned char * jtp_backpack_right = NULL;  /* Backpack graphic, right border */

/* Lighting table (color conversion) */
unsigned char * jtp_shade = NULL;     /* Light shading table */

static struct {
	const char *str;
	int level;
} const jtp_condition_alerts[] = {
	/* encumberance */
	{ "Burdened", JTP_WARN_NORMAL },
	{ "Brd", JTP_WARN_NORMAL },
	{ "Stressed", JTP_WARN_MORE },
	{ "Ssd", JTP_WARN_NORMAL },
	{ "Strained", JTP_WARN_MORE },
	{ "Snd", JTP_WARN_NORMAL },
	{ "Overtaxed", JTP_WARN_ALERT },
	{ "Otd", JTP_WARN_ALERT },
	{ "Overloaded", JTP_WARN_CRITICAL },
	{ "Old", JTP_WARN_CRITICAL },
	/* hungry */
	{ "Satiated", JTP_WARN_NONE },
	{ "Sat", JTP_WARN_NONE },
	{ "Hungry", JTP_WARN_NORMAL },
	{ "Hun", JTP_WARN_NORMAL },
	{ "Weak", JTP_WARN_MORE },
	{ "Wea", JTP_WARN_MORE },
	{ "Fainting", JTP_WARN_CRITICAL },
	{ "Ftg", JTP_WARN_CRITICAL },
	{ "Fainted", JTP_WARN_ALERT },
	{ "Ftd", JTP_WARN_ALERT },
	{ "Starved", JTP_WARN_ALERT }, /* too late to warn about this ;) */
	{ "Sta", JTP_WARN_ALERT },
	/* Confusion */
	{ "Conf", JTP_WARN_MORE },
	{ "Cnf", JTP_WARN_MORE },
	/* Sick */
	{ "FoodPois", JTP_WARN_MORE },
	{ "FPs", JTP_WARN_MORE },
	{ "Ill", JTP_WARN_MORE },
	/* others */
	{ "Blind", JTP_WARN_MORE },
	{ "Bnd", JTP_WARN_MORE },
	{ "Stun", JTP_WARN_MORE },
	{ "Stn", JTP_WARN_MORE },
	{ "Hallu", JTP_WARN_MORE },
	{ "Hal", JTP_WARN_MORE },
	{ "Slime", JTP_WARN_MORE },
	{ "Slm", JTP_WARN_MORE },
	{ "Held", JTP_WARN_MORE },
};


/*----------------------------------------------------
  Function implementations 
-----------------------------------------------------*/

jtp_window *jtp_find_window(winid window)
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


int jtp_find_menu_accelerator(const char *description, char *used_accelerators)
{
    char acc_found;
    int cur_accelerator;
    int i, j;
    char acclist[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";

    /* Find an unused accelerator */
    acc_found = 0;
    cur_accelerator = 0;

    /* Pick any available letter from [a-zA-Z0-9] */
    for (i = 0; i < strlen(acclist); i++)
    {
        cur_accelerator = acclist[i];

        acc_found = 1;
        for (j = 0; used_accelerators[j] != '\0'; j++)
            if (used_accelerators[j] == cur_accelerator)
                acc_found = 0;
        if (acc_found)
            break;
    }

    if (acc_found)
    {
        /* Add found accelerator to string of used ones (assume there's enough room) */
        j = strlen(used_accelerators);
        used_accelerators[j] = cur_accelerator;
        used_accelerators[j+1] = '\0';
        return(cur_accelerator);
    }

    return(-1);
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
      if ((jtp_event_sounds[i])->soundtype == JTP_EVENT_SOUND_TYPE_SND)      
        jtp_play_sound((jtp_event_sounds[i])->filename);
      else if ((jtp_event_sounds[i])->soundtype == JTP_EVENT_SOUND_TYPE_MUS)
        jtp_play_song((jtp_event_sounds[i])->filename);
      else if ((jtp_event_sounds[i])->soundtype == JTP_EVENT_SOUND_TYPE_CD_AUDIO)
        jtp_play_cd_track((jtp_event_sounds[i])->filename);
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



static void jtp_tooltip_location(
  int moux,
  int mouy, 
  jtp_tile *m_cursor, 
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



static unsigned char *jtp_make_tooltip(const char *tip_text)
{
 unsigned char *temp;
 int i,xsize,ysize;

 xsize = jtp_text_length(tip_text, JTP_FONT_TOOLTIP) + 6;
 ysize = jtp_text_height(tip_text, JTP_FONT_TOOLTIP) + 6;

 temp = malloc((xsize*ysize+4)*sizeof(unsigned char));
 if (!temp)
 {
   OOM(1);
 }
 jtp_put_dimensions(temp, xsize, ysize);
 memset(temp + 4, JTP_COLOR_TEXT, xsize * ysize);
 for (i = 0; i < xsize; i++)
 {
   temp[i+4] = JTP_COLOR_BACKGROUND;
   temp[(ysize-1)*xsize+i+4] = JTP_COLOR_BACKGROUND;
 }
 for (i = 0; i < ysize; i++)
 {
    temp[i*xsize+4] = JTP_COLOR_BACKGROUND;
    temp[i*xsize+xsize-1+4] = JTP_COLOR_BACKGROUND;
 }
 jtp_put_text(3, 3+jtp_fonts[JTP_FONT_TOOLTIP].baseline, JTP_FONT_TOOLTIP, JTP_COLOR_BACKGROUND, tip_text, temp);
 return(temp);
}



static int jtp_mouse_hotspot(void)
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



unsigned char *jtp_choose_target_tooltip(int tgt_x, int tgt_y)
{
  unsigned char * new_tip;
  char * out_str;

  out_str = jtp_map_square_description(tgt_x, tgt_y, 1);
  if (out_str)
  {
    new_tip = jtp_make_tooltip(out_str);
    free(out_str);
    return(new_tip);
  }
  return(NULL);
}



static unsigned char *jtp_choose_tooltip(int hotspot)
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
      new_tip = jtp_make_tooltip("Cast Spells");
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



static jtp_tile *jtp_choose_target_cursor(int tgt_x, int tgt_y)
{
  if ((tgt_x < 1) || (tgt_x >= JTP_MAP_WIDTH) || (tgt_y < 0) || (tgt_y >= JTP_MAP_HEIGHT))
    return(jtp_mcursor[V_CURSOR_TARGET_INVALID]);

  if (jtp_whatis_active)
    return(jtp_mcursor[V_CURSOR_TARGET_HELP]);
    
  if (jtp_map_mon[tgt_y][tgt_x] != V_TILE_NONE)
    if ((tgt_x != u.ux) || (tgt_y != u.uy))
      return(jtp_mcursor[V_CURSOR_TARGET_RED]);
  
  if (jtp_map_obj[tgt_y][tgt_x] != V_TILE_NONE)  
    return(jtp_mcursor[V_CURSOR_TARGET_RED]);
  
  if (jtp_map_back[tgt_y][tgt_x] != V_TILE_NONE)  /* valid visible location  */
  {
    /* Closed doors get an 'open door' cursor */
    if ((jtp_map_furniture[tgt_y][tgt_x] == V_TILE_VDOOR_WOOD_CLOSED) ||
        (jtp_map_furniture[tgt_y][tgt_x] == V_TILE_HDOOR_WOOD_CLOSED))
      return(jtp_mcursor[V_CURSOR_OPENDOOR]);

    /* Stairs and ladders get a 'stairs' cursor */
    if ((jtp_map_furniture[tgt_y][tgt_x] == V_TILE_STAIRS_UP) ||
        (jtp_map_furniture[tgt_y][tgt_x] == V_TILE_STAIRS_DOWN) ||
        (jtp_map_furniture[tgt_y][tgt_x] == V_TILE_LADDER_UP) ||
        (jtp_map_furniture[tgt_y][tgt_x] == V_TILE_LADDER_DOWN))
      return(jtp_mcursor[V_CURSOR_STAIRS]);

    /* Fountains get a 'goblet' cursor */
    if (jtp_map_furniture[tgt_y][tgt_x] == V_TILE_FOUNTAIN)
      return(jtp_mcursor[V_CURSOR_GOBLET]);

    if (jtp_map_back[tgt_y][tgt_x] != V_TILE_WALL_GENERIC)
      return(jtp_mcursor[V_CURSOR_TARGET_GREEN]);
  }

  return(jtp_mcursor[V_CURSOR_TARGET_INVALID]);
}



void jtp_get_mouse_input(jtp_tile * m_cursor, int whenstop)
{
  int        target_x = -1, target_y = -1;
  int        target_old_x = -1, target_old_y = -1;

  int        hotspot = JTP_HOTSPOT_NONE;
  int        old_hotspot = JTP_HOTSPOT_NONE;

  unsigned char *m_bg = NULL;
  jtp_tile *m_old_cursor = m_cursor;

  int        ttip_x = -1, ttip_y = -1;
  int        ttip_old_x = -1, ttip_old_y = -1;
  unsigned char *tooltip = NULL;
  unsigned char *ttip_bg = NULL;
  unsigned char *ttip_old_bg = NULL;

  int         i, j;
  char        forcedraw = 1;
  char        stopmouse = 0;

  jtp_readmouse();

  do
  {
    /* 
       At this point the pointer status is:
       tooltip:     not NULL, may change on mouse movement
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
          m_cursor = jtp_mcursor[V_CURSOR_SCROLLUP];
          break;
        case JTP_HOTSPOT_SCROLL_DOWN:
          m_cursor = jtp_mcursor[V_CURSOR_SCROLLDOWN];
          break;
        case JTP_HOTSPOT_SCROLL_LEFT:
          m_cursor = jtp_mcursor[V_CURSOR_SCROLLLEFT];
          break;
        case JTP_HOTSPOT_SCROLL_RIGHT:
          m_cursor = jtp_mcursor[V_CURSOR_SCROLLRIGHT];
          break;
        default:
          m_cursor = jtp_mcursor[V_CURSOR_NORMAL];
          break;
      }

      /* DEBUG */ if (!m_cursor) m_cursor = jtp_mcursor[V_CURSOR_NORMAL]; /* DEBUG */

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
        jtp_refresh(&jtp_screen);
        forcedraw = 0;
      }
      else
      {
        /* Refresh old and new areas of mouse cursor */
        jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                jtp_mousex + m_cursor->xmod + m_cursor->graphic[3] - 1, 
                jtp_mousey + m_cursor->ymod + m_cursor->graphic[1] - 1, &jtp_screen);
        jtp_refresh_region(jtp_oldmx + m_old_cursor->xmod, jtp_oldmy + m_old_cursor->ymod,
                jtp_oldmx + m_old_cursor->xmod + m_old_cursor->graphic[3] - 1, 
                jtp_oldmy + m_old_cursor->ymod + m_old_cursor->graphic[1] - 1, &jtp_screen);
 
        /* Refresh old and new areas of tooltip */
        if (ttip_bg)
          jtp_refresh_region(ttip_x, ttip_y,
                  ttip_x + ttip_bg[2]*256 + ttip_bg[3] - 1, 
                  ttip_y + ttip_bg[0]*256 + ttip_bg[1] - 1, &jtp_screen);
        if (ttip_old_bg)
          jtp_refresh_region(ttip_old_x, ttip_old_y,
                  ttip_old_x + ttip_old_bg[2]*256 + ttip_old_bg[3] - 1, 
                  ttip_old_y + ttip_old_bg[0]*256 + ttip_old_bg[1] - 1, &jtp_screen);
      }

      /* Restore mouse background */
      jtp_put_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_bg);
 
      /* Restore tooltip background */
      if (ttip_bg) jtp_put_img(ttip_x, ttip_y, ttip_bg);

      /* Now that we've used them in a screen refresh, the old backgrounds can be freed */
      free(ttip_old_bg); ttip_old_bg = NULL;
      free(m_bg); m_bg = NULL;
    }
    jtp_get_event();
    jtp_readmouse();
    if ((whenstop >= 0) && (jtp_mouseb == whenstop)) stopmouse = 1;
    else if (jtp_mouseb > 0) stopmouse = 1;
  }
  while ((!stopmouse) && (!jtp_kbhit()));

  /* Erase leftover tooltip from screen (keep mouse cursor to avoid flicker) */
  if (ttip_bg)
    jtp_refresh_region(ttip_x, ttip_y,
                       ttip_x + ttip_bg[2]*256 + ttip_bg[3],
                       ttip_y + ttip_bg[0]*256 + ttip_bg[1], &jtp_screen);
  free(ttip_bg);
  free(tooltip);
}



static void jtp_get_mouse_appearance(
  jtp_tile ** m_cursor,
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
  *m_cursor = jtp_mcursor[V_CURSOR_NORMAL];
  *tooltip = NULL;
  *hotspot = -1;
}



int jtp_get_mouse_inventory_input(
  jtp_tile * m_cursor,
  jtp_hotspot ** hotspots,
  int n_hotspots,
  int whenstop
)
{
  int        hotspot = -1;
  int        old_hotspot = -1;

  unsigned char *m_bg = NULL;
  jtp_tile *m_old_cursor = m_cursor;

  int        ttip_x = -1, ttip_y = -1;
  int        ttip_old_x = -1, ttip_old_y = -1;
  unsigned char *tooltip = NULL;
  unsigned char *ttip_bg = NULL;
  unsigned char *ttip_old_bg = NULL;

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
      /* DEBUG */ if (!m_cursor) m_cursor = jtp_mcursor[V_CURSOR_NORMAL]; /* DEBUG */

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
        jtp_refresh(&jtp_screen);
        forcedraw = 0;
      }
      else
      {
        /* Refresh old and new areas of mouse cursor */
        jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                jtp_mousex + m_cursor->xmod + m_cursor->graphic[3] - 1, 
                jtp_mousey + m_cursor->ymod + m_cursor->graphic[1] - 1, &jtp_screen);
        jtp_refresh_region(jtp_oldmx + m_old_cursor->xmod, jtp_oldmy + m_old_cursor->ymod,
                jtp_oldmx + m_old_cursor->xmod + m_old_cursor->graphic[3] - 1, 
                jtp_oldmy + m_old_cursor->ymod + m_old_cursor->graphic[1] - 1, &jtp_screen);
 
        /* Refresh old and new areas of tooltip */
        if (ttip_bg)
          jtp_refresh_region(ttip_x, ttip_y,
                  ttip_x + ttip_bg[2]*256 + ttip_bg[3] - 1, 
                  ttip_y + ttip_bg[0]*256 + ttip_bg[1] - 1, &jtp_screen);
        if (ttip_old_bg)
          jtp_refresh_region(ttip_old_x, ttip_old_y,
                  ttip_old_x + ttip_old_bg[2]*256 + ttip_old_bg[3] - 1, 
                  ttip_old_y + ttip_old_bg[0]*256 + ttip_old_bg[1] - 1, &jtp_screen);
      }

      /* Restore mouse background */
      jtp_put_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_bg);
 
      /* Restore tooltip background */
      if (ttip_bg) jtp_put_img(ttip_x, ttip_y, ttip_bg);

      /* Now that we've used them in a screen refresh, the old backgrounds can be freed */
      free(ttip_old_bg); ttip_old_bg = NULL;
      free(m_bg); m_bg = NULL;
    }
    jtp_get_event();
    jtp_readmouse();
    if ((whenstop >= 0) && (jtp_mouseb == whenstop)) stopmouse = 1;
    else if (jtp_mouseb > 0) stopmouse = 1;
  }
  while ((!stopmouse) && (!jtp_kbhit()));

  /* Erase leftover tooltip from screen (keep mouse cursor to avoid flicker) */
  if (ttip_bg)
    jtp_refresh_region(ttip_x, ttip_y,
                       ttip_x + ttip_bg[2]*256 + ttip_bg[3],
                       ttip_y + ttip_bg[0]*256 + ttip_bg[1], &jtp_screen);
  free(ttip_bg);
  return(hotspot);
}



int jtp_object_to_tile(int obj_id, int x, int y)
{
    int tile;

    if (obj_id == STATUE || obj_id == FIGURINE)
    {
        struct obj *obj;
        static int lastfigurine = PM_KNIGHT;
 
        if (x >= 0)
            obj = level.objects[x][y];
        else
            obj = invent;

        while (obj && !(obj->otyp == obj_id && (x >= 0 || obj->invlet == y)))
            obj = (x >= 0) ? obj->nexthere : obj->nobj;

        if (obj && obj->corpsenm != 0)
        {
            tile = obj->corpsenm;
            lastfigurine = tile;
        }
        else
        {
            if (obj_id == FIGURINE)
                tile = lastfigurine;
            else
                tile = PM_KNIGHT;
        }
        
        if (obj_id == STATUE)
        {
            tile = STATUE_TO_VTILE(tile);
            if (jtp_tiles[tile] == NULL)
                tile = OBJECT_TO_VTILE(STATUE);
        }
        else
        {
            tile = FIGURINE_TO_VTILE(tile);
            if (jtp_tiles[tile] == NULL)
                tile = OBJECT_TO_VTILE(STRANGE_OBJECT);
        }
        return tile;
    }

    if (!objects[obj_id].oc_name_known)
    {
        switch (obj_id)
        {
            case SACK:
            case OILSKIN_SACK:
            case BAG_OF_TRICKS:
            case BAG_OF_HOLDING:
                return V_TILE_UNIDENTIFIED_BAG;
		
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
                return V_TILE_STONE;

            case TIN_WHISTLE:
            case MAGIC_WHISTLE:
                return OBJECT_TO_VTILE(TIN_WHISTLE);
		
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
                switch (objects[obj_id].oc_color)
                {
                    case CLR_RED:     tile = GEM_RED_GLASS; break;
                    case CLR_BLACK:   tile = GEM_BLACK_GLASS; break;
                    case CLR_GREEN:   tile = GEM_GREEN_GLASS; break;
                    case CLR_BROWN:   tile = GEM_BROWN_GLASS; break;
                    case CLR_MAGENTA: tile = GEM_VIOLET_GLASS; break;
                    case CLR_ORANGE:  tile = GEM_ORANGE_GLASS; break;
                    case CLR_YELLOW:  tile = GEM_YELLOW_GLASS; break;
                    case CLR_WHITE:   tile = GEM_WHITE_GLASS; break;
                    case CLR_BLUE:    tile = GEM_BLUE_GLASS; break;
                    default:          tile = GEM_BLACK_GLASS; break;
                }
                return OBJECT_TO_VTILE(tile);
        }
    }
    
    return OBJECT_TO_VTILE(obj_id);
}



int jtp_monster_to_tile(int mon_id, XCHAR_P x, XCHAR_P y)
{
    if (Invis && u.ux == x && u.uy == y)
        return V_TILE_PLAYER_INVIS;

#if defined(VULTURESEYE) || (defined(VULTURESCLAW) && defined(REINCARNATION))
    if (Is_rogue_level(&u.uz))
    {
        switch (mon_id)
        {
            case PM_COUATL : case PM_ALEAX : case PM_ANGEL :
            case PM_KI_RIN : case PM_ARCHON :
                return V_TILE_ROGUE_LEVEL_A;
              
            case PM_GIANT_BAT : case PM_RAVEN :
            case PM_VAMPIRE_BAT : case PM_BAT :
                return V_TILE_ROGUE_LEVEL_B;
              
            case PM_PLAINS_CENTAUR : case PM_FOREST_CENTAUR :
            case PM_MOUNTAIN_CENTAUR :
                return V_TILE_ROGUE_LEVEL_C;

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
                return V_TILE_ROGUE_LEVEL_D;

            case PM_STALKER : case PM_AIR_ELEMENTAL :
            case PM_FIRE_ELEMENTAL: case PM_EARTH_ELEMENTAL :
            case PM_WATER_ELEMENTAL :
                return V_TILE_ROGUE_LEVEL_E;

            case PM_LICHEN : case PM_BROWN_MOLD :
            case PM_YELLOW_MOLD : case PM_GREEN_MOLD :
            case PM_RED_MOLD : case PM_SHRIEKER :
            case PM_VIOLET_FUNGUS :
                return V_TILE_ROGUE_LEVEL_F;

            case PM_GNOME : case PM_GNOME_LORD :
            case PM_GNOMISH_WIZARD : case PM_GNOME_KING :
                return V_TILE_ROGUE_LEVEL_G;

            case PM_GIANT : case PM_STONE_GIANT :
            case PM_HILL_GIANT : case PM_FIRE_GIANT :
            case PM_FROST_GIANT : case PM_STORM_GIANT :
            case PM_ETTIN : case PM_TITAN : case PM_MINOTAUR :
                return V_TILE_ROGUE_LEVEL_H;

            case 999990 :	//None
                return V_TILE_ROGUE_LEVEL_I;

            case PM_JABBERWOCK :
                return V_TILE_ROGUE_LEVEL_J;

            case PM_KEYSTONE_KOP : case PM_KOP_SERGEANT :
            case PM_KOP_LIEUTENANT : case PM_KOP_KAPTAIN :
                return V_TILE_ROGUE_LEVEL_K;

            case PM_LICH : case PM_DEMILICH :
            case PM_MASTER_LICH : case PM_ARCH_LICH :
                return V_TILE_ROGUE_LEVEL_L;

            case PM_KOBOLD_MUMMY : case PM_GNOME_MUMMY :
            case PM_ORC_MUMMY : case PM_DWARF_MUMMY :
            case PM_ELF_MUMMY : case PM_HUMAN_MUMMY :
            case PM_ETTIN_MUMMY : case PM_GIANT_MUMMY :
                return V_TILE_ROGUE_LEVEL_M;

            case PM_RED_NAGA_HATCHLING :
            case PM_BLACK_NAGA_HATCHLING :
            case PM_GOLDEN_NAGA_HATCHLING :
            case PM_GUARDIAN_NAGA_HATCHLING :
            case PM_RED_NAGA : case PM_BLACK_NAGA :
            case PM_GOLDEN_NAGA : case PM_GUARDIAN_NAGA :
                return V_TILE_ROGUE_LEVEL_N;

            case PM_OGRE : case PM_OGRE_LORD :
            case PM_OGRE_KING :
                return V_TILE_ROGUE_LEVEL_O;

            case PM_GRAY_OOZE : case PM_BROWN_PUDDING :
            case PM_BLACK_PUDDING : case PM_GREEN_SLIME :
                return V_TILE_ROGUE_LEVEL_P;

            case PM_QUANTUM_MECHANIC :
                return V_TILE_ROGUE_LEVEL_Q;

            case PM_RUST_MONSTER : case PM_DISENCHANTER :
                return V_TILE_ROGUE_LEVEL_R;

            case PM_GARTER_SNAKE : case PM_SNAKE :
            case PM_WATER_MOCCASIN : case PM_PIT_VIPER :
            case PM_PYTHON : case PM_COBRA :
                return V_TILE_ROGUE_LEVEL_S;

            case PM_TROLL : case PM_ICE_TROLL :
            case PM_ROCK_TROLL : case PM_WATER_TROLL :
            case PM_OLOG_HAI :
                return V_TILE_ROGUE_LEVEL_T;

            case PM_UMBER_HULK :
                return V_TILE_ROGUE_LEVEL_U;

            case PM_VAMPIRE : case PM_VAMPIRE_LORD :
                return V_TILE_ROGUE_LEVEL_V;

            case PM_BARROW_WIGHT : case PM_WRAITH :
            case PM_NAZGUL :
                return V_TILE_ROGUE_LEVEL_W;

            case PM_XORN :
                return V_TILE_ROGUE_LEVEL_X;

            case PM_MONKEY : case PM_APE : case PM_OWLBEAR :
            case PM_YETI : case PM_CARNIVOROUS_APE :
            case PM_SASQUATCH :
                return V_TILE_ROGUE_LEVEL_Y;

            case PM_GHOUL:
            case PM_KOBOLD_ZOMBIE : case PM_GNOME_ZOMBIE :
            case PM_ORC_ZOMBIE : case PM_DWARF_ZOMBIE :
            case PM_ELF_ZOMBIE : case PM_HUMAN_ZOMBIE :
            case PM_ETTIN_ZOMBIE : case PM_GIANT_ZOMBIE :
                return V_TILE_ROGUE_LEVEL_Z;

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
    if (mon_id == PM_ALEAX)
        return MONSTER_TO_VTILE(u.umonnum);

    if (mon_id == PM_ALIGNED_PRIEST)
    {
        register struct monst *mtmp = m_at(x, y);

        switch (EPRI(mtmp)->shralign)
        {
            case A_LAWFUL:  return V_TILE_LAWFUL_PRIEST;
            case A_CHAOTIC: return V_TILE_CHAOTIC_PRIEST;
            case A_NEUTRAL: return V_TILE_NEUTRAL_PRIEST;
            default:        return V_TILE_UNALIGNED_PRIEST;
        }
    }

    if ((mon_id >= 0) && (mon_id < NUMMONS))
        return MONSTER_TO_VTILE(mon_id);

    return V_TILE_NONE;
}



jtp_list * jtp_list_new(void)
{
  jtp_list * list_temp;
  
  list_temp = malloc(sizeof(jtp_list));
  if (!list_temp)
  {
    OOM(1);
  }
  list_temp->header = malloc(sizeof(jtp_listitem));
  if (!list_temp->header)
  {
    OOM(1);
  }
  list_temp->header->previous = list_temp->header;
  list_temp->header->next = NULL;
  list_temp->header->itemdata = NULL;
  list_temp->previous = list_temp->header;
  list_temp->length = 0;
  return(list_temp);
}


void jtp_list_reset(jtp_list * list_to_reset)
{
  list_to_reset->previous = list_to_reset->header;
}

void jtp_list_advance(jtp_list * list_to_advance)
{
  if (list_to_advance->previous->next)
  {
    list_to_advance->previous = (jtp_listitem *)(list_to_advance->previous->next);
  }
}

static void jtp_list_retreat(jtp_list * list_to_retreat)
{
  list_to_retreat->previous = (jtp_listitem *)(list_to_retreat->previous->previous);
}


void *jtp_list_current(jtp_list * list_to_access)
{
  if (!(list_to_access->previous->next))
    return(NULL);
  else
    return(((jtp_listitem *)list_to_access->previous->next)->itemdata);
}

void jtp_list_add(jtp_list * list_to_access, void *item_to_add)
{
  jtp_listitem * tempitem;
  
  tempitem = malloc(sizeof(jtp_listitem));
  if (!tempitem)
  {
    OOM(1);
  }
  tempitem->itemdata = item_to_add;
  tempitem->previous = list_to_access->previous;
  tempitem->next = list_to_access->previous->next;
  if (tempitem->next)
    ((jtp_listitem *)tempitem->next)->previous = tempitem;
  list_to_access->previous->next = tempitem;
  list_to_access->length++;
}


void jtp_list_remove(jtp_list * list_to_access, void *item_to_remove)
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



int jtp_list_length(jtp_list *list_to_access)
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



void jtp_free_buttons(jtp_list *buttonlist)
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



void jtp_clear_screen(void)
{
  memset(jtp_screen.vpage, JTP_COLOR_BACKGROUND, jtp_screen.width*jtp_screen.height);
}



unsigned char *jtp_draw_window(int x, int y, int width, int height)
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



static unsigned char *jtp_draw_dropdown_window(int x, int y, int width, int height)
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



void jtp_draw_button(int x, int y, int width, int height, const char *str)
{
  int i;
  
  i = (width - jtp_text_length(str, JTP_FONT_BUTTON))/2;

  /* Black edge */
  jtp_rect(x+1, y+1, x+width-2, y+height-2, JTP_COLOR_BACKGROUND);
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
               JTP_FONT_BUTTON, JTP_COLOR_BACKGROUND,
               str,
               jtp_screen.vpage);
  jtp_put_text(x+i,
               y+jtp_fonts[JTP_FONT_BUTTON].baseline+5,
               JTP_FONT_BUTTON, JTP_COLOR_TEXT,
               str,
               jtp_screen.vpage);
}



void jtp_draw_buttons(int x, int y, jtp_list *buttons)
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



void jtp_draw_menu(int x, int y, jtp_menu *menu, jtp_menuitem *firstitem)
{
  int i, j;
  int firstitem_y;
  int firstitem_index = 0;
  int menu_item_count;
  jtp_menuitem * tempmenuitem;

  if (!menu) return;

  if (menu->prompt)
  {
    jtp_put_text(x + menu->prompt_x, 
                 y + menu->prompt_y + jtp_fonts[JTP_FONT_HEADLINE].baseline + 1,
                 JTP_FONT_HEADLINE, JTP_COLOR_BACKGROUND,
                 menu->prompt,
                 jtp_screen.vpage);
    jtp_put_text(x+ menu->prompt_x,
                 y + menu->prompt_y + jtp_fonts[JTP_FONT_HEADLINE].baseline,
                 JTP_FONT_HEADLINE, JTP_COLOR_TEXT,
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
                   JTP_FONT_MENU, JTP_COLOR_BACKGROUND, tempmenuitem->text, jtp_screen.vpage);
      jtp_put_text(i, y + menu->items_y + tempmenuitem->y - firstitem_y + jtp_fonts[JTP_FONT_MENU].baseline + j,
                   JTP_FONT_MENU, JTP_COLOR_TEXT, tempmenuitem->text, jtp_screen.vpage);
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



void jtp_draw_status(jtp_window *tempwindow)
{
  int i;
  unsigned int k;
  char * strptr;
  int len;
  char buffer[1024];
  char * tok;
  int  token_ok, token_x = 0, token_y = 0;
  int  n_conditions, name_passed;
  int  lineheight;
  int  warn;
  static int const status_xpos[5] = { 0, 60, 100, 180, 255 };
  
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
  lineheight = jtp_fonts[JTP_FONT_STATUS].lineheight;

  /* 
   * Parse status lines into 'status tokens'. Assign location of tokens on-screen.
   * Note: this section contains lots of 'magic numbers', but they're all indidivual
   * coordinates, so it doesn't seem worth it to make a separate define for each.
   * They are chosen to match the graphic file used.
   */

  n_conditions = 0;
  buffer[0] = '\0';
  name_passed = 0;
  for (i = 0; i < tempwindow->curs_rows; i++)
  {
    strptr = (char *)tempwindow->rows[i];
    len = strlen(strptr);
    
    tok = strtok(strptr, " ");
    while (tok)
    {
      token_ok = 0;
      warn = JTP_WARN_NONE;
      if (strncmp(tok, "St:", 3) == 0) /* Strength token */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 0;
        token_y = 1;
      }
      else if (strncmp(tok, "Dx:", 3) == 0) /* Dexterity token */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 0;
        token_y = 2;
      }
      else if (strncmp(tok, "Co:", 3) == 0) /* Constitution token */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 0;
        token_y = 3;
      }
      else if (strncmp(tok, "Wt:", 3) == 0) /* Weight token, only possible with SHOW_WEIGHT defined */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 0;
        token_y = 4;
      }
      else if (strncmp(tok, "In:", 3) == 0) /* Intelligence token */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 1;
        token_y = 1;
      }
      else if (strncmp(tok, "Wi:", 3) == 0) /* Wisdom token */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 1;
        token_y = 2;
      }
      else if (strncmp(tok, "Ch:", 3) == 0) /* Charisma token */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 1;
        token_y = 3;
      }
      else if (strncmp(tok, "HP:", 3) == 0) /* Hit points token */
      {
        int hp, hpmax;
        
        token_ok = 1;
        name_passed = 1;
        token_x = 2;
        token_y = 1;
        /*
         * FIXME: the thresholds used here as well as the colors
         * should be made configurable
         */
        hp = Upolyd ? u.mh : u.uhp;
        hpmax = Upolyd ? u.mhmax : u.uhpmax;
        if (hp >= ((hpmax * 90) / 100))
          warn = JTP_WARN_NONE;
        else if (hp >= ((hpmax * 70) / 100))
          warn = JTP_WARN_NORMAL;
        else if (hp >= ((hpmax * 50) / 100))
          warn = JTP_WARN_MORE;
        else if (hp >= ((hpmax * 25) / 100))
          warn = JTP_WARN_ALERT;
        else
          warn = JTP_WARN_CRITICAL;
      }
      else if (strncmp(tok, "Pw:", 3) == 0) /* Power token */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 2;
        token_y = 2;
      }
      else if (strncmp(tok, "AC:", 3) == 0) /* Armor class token */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 2;
        token_y = 3;
      }
      else if (strncmp(tok, "HD:", 3) == 0) /* Polymorphed token */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 2;
        token_y = 4;
      }
      else if (strncmp(tok, "Exp:", 4) == 0 ||  /* Experience token */
        strncmp(tok, "Xp:", 3) == 0)      /* Experience token, only possible with EXP_ON_BOTL defined */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 3;
        token_y = 0;
      }
      else if (strncmp(tok, "Dlvl:", 5) == 0) /* Dungeon level token */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 3;
        token_y = 1;
      }
      else if (tok[0] == oc_syms[COIN_CLASS] && /* Gold token */
        tok[1] == ':')
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 3;
        token_y = 2;
      }
      else if (strncmp(tok, "T:", 2) == 0) /* Time token */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 3;
        token_y = 3;
      }
      else if (strncmp(tok, "S:", 2) == 0) /* Score token, only possible with SCORE_ON_BOTL defined */
      {
        token_ok = 1;
        name_passed = 1;
        token_x = 3;
        token_y = 4;
      }
      /* Other non-empty tokens are parts of name & title, conditions or alignment */
      else
      {
        if (name_passed)
        {
          token_ok = 1;
          token_x = 4;
          token_y = n_conditions;
          n_conditions++;
          for (k = 0; k < sizeof(jtp_condition_alerts) / sizeof(jtp_condition_alerts[0]); k++)
          {
            if (strcmp(tok, jtp_condition_alerts[k].str) == 0)
            {
              warn = jtp_condition_alerts[k].level;
              break;
            }
          }
        }
        else
        {
          strcat(buffer, tok);
          strcat(buffer, " ");
        }
      }
      /* If the token was recognized, display it */
      if (token_ok)
      {
        token_x = STATUS_XPOS + status_xpos[token_x];
        token_y = STATUS_YPOS + lineheight * token_y + jtp_fonts[JTP_FONT_STATUS].baseline;
        jtp_put_text(jtp_statusbar_x + token_x, jtp_statusbar_y + token_y + 1,
                     JTP_FONT_STATUS, JTP_COLOR_BACKGROUND, tok, jtp_screen.vpage);
        jtp_put_text(jtp_statusbar_x + token_x, jtp_statusbar_y + token_y,
                     JTP_FONT_STATUS, jtp_warn_colors[warn], tok, jtp_screen.vpage);
      }
      /* Get the next token */
      tok = strtok(NULL, " ");
    }
    /* Show character name */
    jtp_put_text(jtp_statusbar_x + STATUS_XPOS,
                 jtp_statusbar_y + STATUS_YPOS + jtp_fonts[JTP_FONT_STATUS].baseline + 1,
                 JTP_FONT_STATUS, JTP_COLOR_BACKGROUND, buffer, jtp_screen.vpage);
    jtp_put_text(jtp_statusbar_x + STATUS_XPOS,
                 jtp_statusbar_y + STATUS_YPOS + jtp_fonts[JTP_FONT_STATUS].baseline,
                 JTP_FONT_STATUS, JTP_COLOR_TEXT, buffer, jtp_screen.vpage);

    /* undo the damage strtok does to our source string */
    for (k = 0; k < len; k++)
      if (!strptr[k])
        strptr[k] = ' ';
  }
  /*
   * show dungeon name
   */
  buffer[0] = '\0';
#ifdef VULTURESCLAW
  describe_level(buffer, TRUE);
#else
  if (!describe_level(buffer))
  {
    sprintf(buffer, "%s, level %d ", dungeons[u.uz.dnum].dname, depth(&u.uz));
  }
#endif
  jtp_put_text(jtp_statusbar_x + STATUS_XPOS,
    jtp_statusbar_y + STATUS_YPOS + 5 * lineheight + jtp_fonts[JTP_FONT_STATUS].baseline + 1,
         JTP_FONT_STATUS, JTP_COLOR_BACKGROUND, buffer, jtp_screen.vpage);
  jtp_put_text(jtp_statusbar_x + STATUS_XPOS,
    jtp_statusbar_y + STATUS_YPOS + 5 * lineheight + jtp_fonts[JTP_FONT_STATUS].baseline,
         JTP_FONT_STATUS, JTP_COLOR_TEXT, buffer, jtp_screen.vpage);
}



int jtp_draw_messages(jtp_window *tempwindow)
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
        /* Draw message */
        jtp_put_text(l, j + 1,
                     JTP_FONT_MESSAGE, JTP_COLOR_BACKGROUND,
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



void jtp_draw_all_windows(void)
{
  jtp_window * tempwindow;

  /* Draw basic windows in correct order: map, messages, status */
  tempwindow = jtp_find_window(WIN_MAP);
  jtp_draw_level(tempwindow, -1, -1);
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



void jtp_show_screen(void)
{
  jtp_refresh(&jtp_screen);
}



void jtp_show_map_area(void)
{
  jtp_refresh_region(0, 0, jtp_screen.width-1, jtp_statusbar_y-1, &jtp_screen);
}



void jtp_show_status_area(void)
{
  jtp_refresh_region(0, jtp_statusbar_y, jtp_screen.width-1, jtp_screen.height-1, &jtp_screen);
}



void jtp_show_message_area(int messages_height)		/* Height of messages in message area */
{
  jtp_refresh_region(0, 0, jtp_screen.width-1, jtp_messages_height-1, &jtp_screen);
}



void jtp_read_mouse_input(void)
{
  /* If the palette is faded out for some reason, restore it */
  jtp_updatepal(0, 255);
  
  jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
}



int jtp_query_choices(const char *ques, const char *choices, int nchoices)
{
    jtp_button * pchoices = NULL;
    int nbuttons, i, len, longdesc = 0;
    int query_x, query_y;
    char *str = (char*)choices;
    char * pstr[1];
    unsigned char * background;
    int pressedkey, selected;
    
    /* there are two modes of operation:
     * - basic mode, where each button is labeled with a single character which are stored in choices
     *   and nchoices is zero (as the number of buttons is trivially found via strlen(choices)
     * - fancy mode, where choices contains exactly nchoices strings which are separated by '\0' */

    nbuttons = nchoices;
    if (!nchoices)
        nbuttons = strlen(choices);
    else
	longdesc = 1;
    
    /* a very common case is "yn" queries. Improve that to a yes/no query*/
    if (strncmp(choices, "yn", 3) == 0)
    {
        str = "yes\0no";
	longdesc = 1;;
    }
    
    pchoices = malloc(nbuttons * sizeof(jtp_button));

    /* str always points to the start of the next button label, len holds the length of the label */
    for(i = 0; i < nbuttons; i++)
    {
        len = 1;
        if (longdesc)
            len = strlen(str);

        pchoices[i].text = malloc(len+1);
        strncpy(pchoices[i].text, str, len);
        pchoices[i].text[len] = '\0';
        pchoices[i].accelerator = str[0];
        pchoices[i].width = jtp_text_length(pchoices[i].text, JTP_FONT_BUTTON) + 15;
        pchoices[i].height = jtp_fonts[JTP_FONT_BUTTON].lineheight + 10;
        pchoices[i].id = i;

        if (longdesc)
            str += len;

    str++;
    }

    /* if we have only one button the accelerator is the ENTER key  */
    if (nchoices == 1)
        pchoices[0].accelerator = '\n';

    /* set up the headline string */
    pstr[0] = (char*)ques;

    /* draw the window & save the background */
    background = jtp_draw_query_window(pstr, 1, pchoices, nbuttons, &query_x, &query_y);

    /* get input */
    selected = -1;
    while (selected < 0)
    {
        /* Wait for mouse click or key press */
        jtp_keymouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
        pressedkey = jtp_getch();
        if (pressedkey != 0)
        {
            if (nbuttons == 1)
                selected = 0;
            else
            {
                for (i = 0; i < nbuttons; i++)
                    if (pchoices[i].accelerator == pressedkey)
                        selected = i;
            }
        }
        else if (jtp_mouseb != JTP_MBUTTON_NONE)
        {
            for (i = 0; i < nbuttons; i++)
                if (jtp_mouse_area(pchoices[i].x, pchoices[i].y, 
                                   pchoices[i].x + pchoices[i].width-1,
                                   pchoices[i].y + pchoices[i].height-1))
                    selected = i;
        }
    }
    if (!nchoices)
        selected = choices[selected];

    /* Restore background */
    jtp_put_img(query_x, query_y, background);
    jtp_refresh(&jtp_screen);

    /* Clean up */
    free(background);
    for (i = 0; i < nbuttons; i++)
        free(pchoices[i].text);
    free(pchoices);

    /* Return the chosen answer */
    return selected;
}



int jtp_query_direction(const char *ques)
{
    int height, width;
    int directions_x, directions_y;
    int directions_height, directions_width;
    int query_x, query_y;
    int text_x, text_y;
    int click_x, click_y;
    unsigned char * background;
    int nchoice = -1;
    int i, j;

    /* Calculate width, height and position of query window */
    width = jtp_text_length(ques, JTP_FONT_HEADLINE);
    height = jtp_text_height(ques, JTP_FONT_HEADLINE) + jtp_fonts[JTP_FONT_HEADLINE].lineheight/2;

    /* Add direction arrows */
    directions_width = jtp_defwin.direction_arrows[2]*256 + jtp_defwin.direction_arrows[3];
    directions_height = jtp_defwin.direction_arrows[0]*256 + jtp_defwin.direction_arrows[1];

    width = (width > directions_width) ? width : directions_width;
    height += directions_height;

    /* calculate window size and position */
    width  += (jtp_defwin.border_left[3] + jtp_defwin.border_right[3]);
    height += (jtp_defwin.border_top[1] + jtp_defwin.border_bottom[1]);

    query_x = (jtp_screen.width - width) / 2;
    query_y = (jtp_screen.height - height) / 2;

    /* calculate direction arrows position  */
    directions_x = query_x + (width - directions_width)/2;
    directions_y = query_y + jtp_defwin.border_top[1] +
                   jtp_text_height(ques, JTP_FONT_HEADLINE) + jtp_fonts[JTP_FONT_HEADLINE].lineheight/2;

    /* Store background graphics */
    background = jtp_draw_window(query_x, query_y, width, height);

    /* draw the direction arrows */
    jtp_put_stencil(directions_x, directions_y, jtp_defwin.direction_arrows);

    /* Draw query message */
    text_x = query_x + jtp_defwin.border_left[3] + 1;
    text_y = query_y + jtp_defwin.border_top[1] + 1;

    jtp_put_text(text_x + 1, text_y + jtp_fonts[JTP_FONT_HEADLINE].baseline + 1, JTP_FONT_HEADLINE,
                 JTP_COLOR_BACKGROUND, ques, jtp_screen.vpage);
    jtp_put_text(text_x, text_y + jtp_fonts[JTP_FONT_HEADLINE].baseline, JTP_FONT_HEADLINE,
                 JTP_COLOR_TEXT, ques, jtp_screen.vpage);

    /* Display window */
    jtp_refresh(&jtp_screen);	

    /* Ask for input */
    while (nchoice < 0)
    {
        /* Wait for mouse click or key press */
        jtp_keymouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
        nchoice = jtp_getch();
        if (nchoice != 0)
        {
            /* swap the keys to match the isometric viewpoint. */
            nchoice = jtp_translate_key(nchoice);
            if (nchoice == 0)
                nchoice = -1;
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
            click_x = JTP_MAP_YMOD*i + JTP_MAP_XMOD*j + JTP_MAP_XMOD*JTP_MAP_YMOD;
            click_x = click_x/(2*JTP_MAP_XMOD*JTP_MAP_YMOD) - (click_x < 0);
            click_y = -JTP_MAP_YMOD*i + JTP_MAP_XMOD*j + JTP_MAP_XMOD*JTP_MAP_YMOD;
            click_y = click_y/(2*JTP_MAP_XMOD*JTP_MAP_YMOD) - (click_y < 0);

      	    /* Select a direction command */
            nchoice = -1;
            if (click_y == -1)
            {
                if (click_x == -1) nchoice = jtp_translate_command(JTP_NHCMD_NORTHWEST);
                else if (click_x == 0) nchoice = jtp_translate_command(JTP_NHCMD_NORTH);
                else if (click_x == 1) nchoice = jtp_translate_command(JTP_NHCMD_NORTHEAST);
            }
            else if (click_y == 0)
            {
                if (click_x == -1) nchoice = jtp_translate_command(JTP_NHCMD_WEST);
                else if (click_x == 0) nchoice = jtp_translate_command(JTP_NHCMD_REST);
                else if (click_x == 1) nchoice = jtp_translate_command(JTP_NHCMD_EAST);
            }
            if (click_y == 1)
            {
                if (click_x == -1) nchoice = jtp_translate_command(JTP_NHCMD_SOUTHWEST);
                else if (click_x == 0) nchoice = jtp_translate_command(JTP_NHCMD_SOUTH);
                else if (click_x == 1) nchoice = jtp_translate_command(JTP_NHCMD_SOUTHEAST);
            }

            if (click_x >= 2 && i < directions_width/2 && j < directions_height/2)
                nchoice = jtp_translate_command(JTP_NHCMD_DOWN);

            if (click_x <= -2 && i > -directions_width/2 && j > -directions_height/2)
                nchoice = jtp_translate_command(JTP_NHCMD_UP);

            /* Wait until mouse button is released */
            jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
        }
        else
        {
            nchoice = -1;
        }
    }

    /* Restore background */
    jtp_put_img(query_x, query_y, background);
    jtp_refresh(&jtp_screen);

    /* Clean up */
    free(background);

    /* Return the chosen answer */
    return nchoice;
}



int jtp_query_anykey(const char *ques)
{
    jtp_button * pchoices = NULL;
    int query_x, query_y;
    int nbuttons, i;
    char * pstr[2];
    unsigned char * background;
    int nchoice = -1;
    int count = 0;
    char countstr[64];
    int keybuf[10];
    int keycount = 0;

    nbuttons = 3;
    pchoices = malloc(nbuttons*sizeof(jtp_button));
    if (!pchoices)
        OOM(1);

    pchoices[0].accelerator = '?';
    pchoices[0].text = "Show choices";

    pchoices[1].accelerator = '*';
    pchoices[1].text = "Show inventory";

    pchoices[2].accelerator = 27; /* ESC */
    pchoices[2].text = "Cancel";

    for (i = 0; i < 3; i++)
    {
        pchoices[i].width = jtp_text_length(pchoices[i].text, JTP_FONT_BUTTON) + 15;
        pchoices[i].height = jtp_fonts[JTP_FONT_BUTTON].lineheight + 10;
        pchoices[i].id = i;
    }

    /* set up the headline & helpstring */   
    pstr[0] = (char*)ques;
    pstr[1] = "(type any key)";

    /* draw the window and save the background */
    background = jtp_draw_query_window(pstr, 2, pchoices, 3, &query_x, &query_y);

    /* get input */
    while (nchoice < 0)
    {
        /* Wait for mouse click or key press */
        jtp_keymouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
        nchoice = jtp_getch();
        if (nchoice == 0 && jtp_mouseb != JTP_MBUTTON_NONE)
        {
            /* Find out if the mouse was clicked on a button */
            for (i = 0; i < nbuttons; i++)
                if (jtp_mouse_area(pchoices[i].x, pchoices[i].y,
                                   pchoices[i].x+pchoices[i].width-1,
                                   pchoices[i].y+pchoices[i].height-1))
                    nchoice = pchoices[i].accelerator;
        }
        /* entering a count */
        else if (isdigit((char)nchoice) && keycount < 10)
        {
            count = count * 10 + (nchoice - 0x30);
            keybuf[keycount++] = nchoice;
            snprintf(countstr, 64, "Count: %d", count);
            pstr[1] = countstr;
            jtp_put_img(query_x, query_y, background);
            free(background);
            background = jtp_draw_query_window(pstr, 2, pchoices, 3, &query_x, &query_y);
            jtp_refresh(&jtp_screen);
            nchoice = -1;
        }
        /* backspace on a count */
        else if (nchoice == 8 && keycount > 0)
        {
            count = count / 10;
            keycount--;
            snprintf(countstr, 64, "Count: %d", count);
            pstr[1] = countstr;
            jtp_put_img(query_x, query_y, background);
            free(background);
            background = jtp_draw_query_window(pstr, 2, pchoices, 3, &query_x, &query_y);
            jtp_refresh(&jtp_screen);
            nchoice = -1;
        }
    }

    /* Restore background */
    jtp_put_img(query_x, query_y, background);
    jtp_refresh(&jtp_screen);

    /* Clean up */
    free(background);
    free(pchoices);

    /* Return the chosen answer */
    if (keycount)
    {
        for(i = 1; i < keycount; i++)
            jtp_sdl_keybuf_add(keybuf[i]);
        jtp_sdl_keybuf_add(nchoice);
        return keybuf[0];
    }
    /*else*/
    return nchoice;
}



unsigned char * jtp_draw_query_window(char * str[], int nstr, jtp_button * buttons, int nbuttons, int *win_x, int *win_y)
{
    int i, temp;
    int text_width, text_height;
    int buttons_width = 0, buttons_height = 0;
    int height, width;
    int query_x, query_y, text_x, text_y;
    unsigned char * query_background;

    /* calculate height & width of the text  */
    text_width = jtp_text_length(str[0], JTP_FONT_HEADLINE);
    text_height = jtp_text_height(str[0], JTP_FONT_HEADLINE) + jtp_fonts[JTP_FONT_HEADLINE].lineheight/2;
    for (i = 1; i < nstr; i++)
    {
        temp = jtp_text_length(str[i], JTP_FONT_MENU);
	text_width = (text_width > temp) ? text_width : temp;

	text_height += jtp_text_height(str[i], JTP_FONT_MENU) + jtp_fonts[JTP_FONT_MENU].lineheight/2;
    }
    text_height += 3; /* pad slightly  */

    /* calculate height and width of the buttons*/
    temp = 0;
    for (i = 0; i < nbuttons; i++)
    {
        buttons_width += buttons[i].width + 10;
        if (buttons[i].height > buttons_height)
            buttons_height = buttons[i].height;

        /* find maximum button width */
        temp = (temp > buttons[i].width) ? temp : buttons[i].width;
    }
    buttons_width -= 10; /* Remove extra spacing */

    /* calculate window dimensions & position */
    height = text_height + buttons_height + 5; /* +5 to accommodate padding */
    width = buttons_width;

    if (text_width > buttons_width)
    {
        width = text_width;

        /* if the text is a lot wider than the buttons, make all buttons equally large  */
        if (width > ((temp+10)*nbuttons-10))
        {
            for (i = 0; i < nbuttons; i++)
                buttons[i].width = temp;
            buttons_width = (temp+10)*nbuttons-10;
        }
    }

    width  += (jtp_defwin.border_left[3] + jtp_defwin.border_right[3]);
    height += (jtp_defwin.border_top[1] + jtp_defwin.border_bottom[1]);

    query_x = (jtp_screen.width - width) / 2;
    query_y = (jtp_screen.height - height) / 2;

    text_x = query_x + jtp_defwin.border_left[3];
    text_y = query_y + jtp_defwin.border_top[1];

    /* calculate button positions */
    temp = query_x + (width - buttons_width) / 2;
    for (i = 0; i < nbuttons; i++)
    {
        buttons[i].x = temp;
        buttons[i].y = text_y + text_height;
        temp += buttons[i].width+10;
    }

    /* Draw the window and store the background */
    query_background = jtp_draw_window(query_x, query_y, width, height);

    /* Draw buttons */
    for (i = 0; i < nbuttons; i++)
    {
        jtp_draw_button(buttons[i].x, buttons[i].y,
                        buttons[i].width, buttons[i].height,
                        buttons[i].text);
    }
    
    /* Draw query message */
    jtp_put_text(text_x + 1, text_y + jtp_fonts[JTP_FONT_HEADLINE].baseline + 1, JTP_FONT_HEADLINE,
                 JTP_COLOR_BACKGROUND, str[0], jtp_screen.vpage);
    jtp_put_text(text_x, text_y + jtp_fonts[JTP_FONT_HEADLINE].baseline, JTP_FONT_HEADLINE,
                 JTP_COLOR_TEXT, str[0], jtp_screen.vpage);

    text_y += (jtp_text_height(str[0], JTP_FONT_HEADLINE) + jtp_fonts[JTP_FONT_HEADLINE].lineheight/2);

    for (i = 1; i < nstr; i++)
    {
        jtp_put_text(text_x + 1, text_y + jtp_fonts[JTP_FONT_MENU].baseline + 1,
                     JTP_FONT_MENU, JTP_COLOR_BACKGROUND, str[i], jtp_screen.vpage);
        jtp_put_text(text_x, text_y + jtp_fonts[JTP_FONT_MENU].baseline,
                    JTP_FONT_MENU, JTP_COLOR_TEXT, str[i], jtp_screen.vpage);

        text_y += (jtp_text_height(str[1], JTP_FONT_MENU) + jtp_fonts[JTP_FONT_MENU].lineheight/2);
    }

    /* Display window */
    jtp_refresh(&jtp_screen);
    
    /* If the palette is faded out for some reason, restore it */
    jtp_updatepal(0, 255);

    if (win_x)
        *win_x = query_x;
    if (win_y)
        *win_y = query_y;

    return query_background;
}



static int jtp_dropdown(
  int qx, int qy,                 /* Dropdown menu position */
  int nanswers,                   /* Number of menu items */
  jtp_dropdown_action ** panswers /* Menu item details */
)
{
  int i, j, k, l;
  int query_x, query_y;
  int totalwidth, totalheight;
  int ncolumns, column_height;
  int nbuttons, buttons_width;
  int nchoice = -1;
  jtp_button * pchoices;
  unsigned char * query_background;
  char pressedkey;

  /* discard all keys currently in the buffer to avoid unexpected results  */
  jtp_sdl_keybuf_reset();

  /* Find out how many of the menu items are buttons (nonzero id) */
  nbuttons = 0;
  for (i = 0; i < nanswers; i++)
    if ((panswers[i])->action_id != 0)
      nbuttons++;
  if (nbuttons <= 0) return(0);


  /* Create button table */
  pchoices = malloc(nbuttons*sizeof(jtp_button));
  if (pchoices == NULL)
  {
    OOM(0);
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
      if (nbuttons == 1) pchoices[j].accelerator = '\n'; /* Enter */
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
                   JTP_COLOR_BACKGROUND,
                   (panswers[i])->str,
                   jtp_screen.vpage);
      jtp_put_text(k,
                   l + jtp_fonts[JTP_FONT_HEADLINE].baseline,
                   JTP_FONT_HEADLINE,
                   JTP_COLOR_TEXT,
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
  jtp_refresh(&jtp_screen);
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
      jtp_keymouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
      pressedkey = jtp_getch();
      if (pressedkey != 0)
      {
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
                               jtp_mcursor[V_CURSOR_NORMAL]);
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
                             jtp_mcursor[V_CURSOR_NORMAL]);
            nchoice = i;
          }
        if (nchoice < 0) nchoice = nbuttons+1;
      }
    }
  }

  /* Restore background */
  jtp_put_img(query_x, query_y, query_background);
  jtp_refresh(&jtp_screen);

  if (nchoice < nbuttons) i = pchoices[nchoice].id;
  else i = 0;
  
  /* Clean up */
  free(query_background);
  free(pchoices);

  /* Return the chosen answer */
  return(i);
}



void jtp_messagebox(const char *message)	/* Message to show player */
{
    jtp_query_choices(message, "Continue", 1);
}



static void jtp_view_messages(void)
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



static int jtp_construct_shortcut_action(
  int tgtx, int tgty, /* Target square, or item accelerator in tgtx */
  int action_id       /* Shortcut action type */
)
{
  jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "jtp_construct_shortcut_action\n");

  switch(action_id)
  {
    case JTP_ACTION_CHAT:
    case JTP_ACTION_KICK:
    case JTP_ACTION_CLOSE_DOOR:
    case JTP_ACTION_OPEN_DOOR:
    case JTP_ACTION_FORCE_LOCK:
    case JTP_ACTION_UNTRAP:
      jtp_is_shortcut_active = 1;
      if (tgtx == u.ux-1)
      {
        if (tgty == u.uy-1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_NORTHWEST);
        if (tgty == u.uy) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_WEST);
        if (tgty == u.uy+1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_SOUTHWEST);

      }
      else if (tgtx == u.ux)
      {
        if (tgty == u.uy-1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_NORTH);
        if (tgty == u.uy) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_REST);
        if (tgty == u.uy+1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_SOUTH);
      }
      if (tgtx == u.ux+1)
      {
        if (tgty == u.uy-1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_NORTHEAST);
        if (tgty == u.uy) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_EAST);
        if (tgty == u.uy+1) jtp_shortcut_query_response = jtp_translate_command(JTP_NHCMD_SOUTHEAST);
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

    case JTP_ACTION_ATTACK:
    case JTP_ACTION_ENTER_TRAP:
    case JTP_ACTION_PUSH_BOULDER:
      if (tgtx == u.ux-1)
      {
        if (tgty == u.uy-1) return(jtp_translate_command(JTP_NHCMD_NORTHWEST));
        if (tgty == u.uy) return(jtp_translate_command(JTP_NHCMD_WEST));
        if (tgty == u.uy+1) return(jtp_translate_command(JTP_NHCMD_SOUTHWEST));
      }
      else if (tgtx == u.ux)
      {
        if (tgty == u.uy-1) return(jtp_translate_command(JTP_NHCMD_NORTH));
        if (tgty == u.uy) return(jtp_translate_command(JTP_NHCMD_REST));
        if (tgty == u.uy+1) return(jtp_translate_command(JTP_NHCMD_SOUTH));
      }
      if (tgtx == u.ux+1)
      {
        if (tgty == u.uy-1) return(jtp_translate_command(JTP_NHCMD_NORTHEAST));
        if (tgty == u.uy) return(jtp_translate_command(JTP_NHCMD_EAST));
        if (tgty == u.uy+1) return(jtp_translate_command(JTP_NHCMD_SOUTHEAST));
      }
      break;

    case JTP_ACTION_MOVE_HERE:
        u.tx = tgtx;
        u.ty = tgty;
        return(CMD_TRAVEL);
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

  }
  return 0;
}


static void jtp_add_dropdown_action(
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
    OOM(0);
    return;
  }
  temp_action = malloc(sizeof(jtp_dropdown_action));
  if (temp_action == NULL)
  {
    OOM(0);
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
      OOM(0);
      return;
    }
    memcpy(temp_action->str, action_str, j+1);
  }
  (*dropdown_actions)[i] = temp_action;
  *n_actions = i+1;
}



static int jtp_get_dropdown_command(int mx, int my, int tgtx, int tgty)
{
  jtp_dropdown_action ** dropdown_actions;
  int n_actions, selected_action = JTP_ACTION_NONE;
  int mapglyph_offset;
  int i;
  char * mapsquare_descr;


  /* Dropdown commands are shown only for valid squares */
  if ((tgtx < 1) || (tgtx >= JTP_MAP_WIDTH) || (tgty < 0) || (tgty >= JTP_MAP_HEIGHT))
    return(0);

  /* Construct a context-sensitive drop-down menu */
  dropdown_actions = NULL;
  n_actions = 0;

  if ((u.ux == tgtx) && (u.uy == tgty))
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

  if ((u.ux == tgtx) && (u.uy == tgty))
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
  else if (jtp_map_mon[tgty][tgtx] != V_TILE_NONE)
  {
    /* Add monster options: */
    if ((u.ux != tgtx) || (u.uy != tgty))
      if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
        {
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_CHAT, "Chat");
          /* jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_RIDE, "Ride"); */
        }
  }

  if (jtp_map_obj[tgty][tgtx] != V_TILE_NONE)
  {
    /* Add object options: */
    mapglyph_offset =  jtp_map_obj[tgty][tgtx];
    switch(mapglyph_offset - OBJTILEOFFSET)
    {
      case LARGE_BOX: case ICE_BOX: case CHEST:
        if ((u.ux == tgtx) && (u.uy == tgty))
        {
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_FORCE_LOCK, "Force lock");
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_LOOT, "Loot");
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_PICK_UP, "Pick up");
        }
        if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_UNTRAP, "Untrap");
        break;
      case SACK: case OILSKIN_SACK: case BAG_OF_HOLDING: case BAG_OF_TRICKS: case (V_TILE_UNIDENTIFIED_BAG + OBJTILEOFFSET):
        if ((u.ux == tgtx) && (u.uy == tgty))
        {
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_LOOT, "Loot");
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_PICK_UP, "Pick up");
        }
        break;
      case BOULDER:
        if ((u.ux != tgtx) || (u.uy != tgty))
          if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_PUSH_BOULDER, "Push");
        break;
      default:
        if ((u.ux == tgtx) && (u.uy == tgty))        
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_PICK_UP, "Pick up");
        break;      
    }
  }

  if (jtp_map_furniture[tgty][tgtx] != V_TILE_NONE)
  {
    /* Add cmap options: */
    mapglyph_offset =  jtp_map_furniture[tgty][tgtx];
    switch(mapglyph_offset)
    {
      case V_TILE_STAIRS_DOWN: case V_TILE_LADDER_DOWN:
        if ((u.ux == tgtx) && (u.uy == tgty))
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_GO_DOWN, "Go down");
        if ((u.ux != tgtx) || (u.uy != tgty))
          if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
        break;

      case V_TILE_STAIRS_UP: case V_TILE_LADDER_UP:
        if ((u.ux == tgtx) && (u.uy == tgty))
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_GO_UP, "Go up");
        if ((u.ux != tgtx) || (u.uy != tgty))
          if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
        break;

      case V_TILE_FOUNTAIN:
        if ((u.ux == tgtx) && (u.uy == tgty))
          jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_DRINK, "Drink");
        if ((u.ux != tgtx) || (u.uy != tgty))
          if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
        break;
	
      case V_TILE_VDOOR_WOOD_OPEN: case V_TILE_HDOOR_WOOD_OPEN:
        if ((u.ux != tgtx) || (u.uy != tgty))
          if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
          {
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_CLOSE_DOOR, "Close door");
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_UNTRAP, "Untrap");
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
          }
        break;
	
      case V_TILE_VDOOR_WOOD_CLOSED: case V_TILE_HDOOR_WOOD_CLOSED:
        if ((u.ux != tgtx) || (u.uy != tgty))
          if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
          {
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_OPEN_DOOR, "Open door");
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_UNTRAP, "Untrap");
            /* jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_FORCE_LOCK, "Force lock"); */
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
          }
        break;
	
      default:
        if ((u.ux != tgtx) || (u.uy != tgty))
          if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
            jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_KICK, "Kick");
        break;
    }
  }

  if (jtp_map_trap[tgty][tgtx] != V_TILE_NONE)
  {
    jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_UNTRAP, "Untrap");
    if ((u.ux != tgtx) || (u.uy != tgty))
      if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
        jtp_add_dropdown_action(&n_actions, &dropdown_actions, JTP_ACTION_ENTER_TRAP, "Enter trap");
  }
  
  if (jtp_map_back[tgty][tgtx] != V_TILE_NONE)
  {
    if ((u.ux != tgtx) || (u.uy != tgty))    
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


  if (selected_action == JTP_ACTION_NONE) return(0);
  else return(jtp_construct_shortcut_action(tgtx, tgty, selected_action));
}



static int jtp_get_default_command(int tgtx, int tgty)
{
  int selected_action;
  int mapglyph_offset;

  /* Off-map squares have no default action */
  if ((tgtx < 1) || (tgtx >= JTP_MAP_WIDTH) ||
      (tgty < 0) || (tgty >= JTP_MAP_HEIGHT))
    return(0);

  /* Select a default command */
  selected_action = 0;

  /* Target is at least 2 squares away */
  if ((abs(u.ux-tgtx) >= 2) || (abs(u.uy-tgty) >= 2))
    selected_action = JTP_ACTION_MOVE_HERE;
 
  /* Monster on target square */
  if ((!selected_action) && (jtp_map_mon[tgty][tgtx] != V_TILE_NONE))
  {
    if ((u.ux != tgtx) || (u.uy != tgty))
      selected_action = JTP_ACTION_ATTACK;
  }

  /* Object on target square */
  if ((!selected_action) && (jtp_map_obj[tgty][tgtx] != V_TILE_NONE))
  {
    mapglyph_offset = jtp_map_obj[tgty][tgtx];
    if ((u.ux == tgtx) && (u.uy == tgty))
      switch(mapglyph_offset - OBJTILEOFFSET)
      {
        case LARGE_BOX: case ICE_BOX: case CHEST:
          selected_action = JTP_ACTION_LOOT; break;
        default:
          selected_action = JTP_ACTION_PICK_UP; break;
      }
    else selected_action = JTP_ACTION_MOVE_HERE;     
  }

  /* map feature on target square  */
  if ((!selected_action) && (jtp_map_furniture[tgty][tgtx] != V_TILE_NONE))
  {
    /* Add cmap options: */
    mapglyph_offset =  jtp_map_furniture[tgty][tgtx];
    if ((mapglyph_offset == V_TILE_STAIRS_DOWN) || (mapglyph_offset == V_TILE_LADDER_DOWN))
      if ((u.ux == tgtx) && (u.uy == tgty))
        selected_action = JTP_ACTION_GO_DOWN;
    if ((mapglyph_offset == V_TILE_STAIRS_UP) || (mapglyph_offset == V_TILE_LADDER_UP))
      if ((u.ux == tgtx) && (u.uy == tgty))
        selected_action = JTP_ACTION_GO_UP;
    if (mapglyph_offset == V_TILE_SINK)
      if ((u.ux != tgtx) || (u.uy != tgty))
        if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
          selected_action = JTP_ACTION_KICK;
    if (mapglyph_offset == V_TILE_FOUNTAIN)
      if ((u.ux == tgtx) && (u.uy == tgty))        
        selected_action = JTP_ACTION_DRINK;
    if ((mapglyph_offset == V_TILE_VDOOR_WOOD_CLOSED) || (mapglyph_offset == V_TILE_HDOOR_WOOD_CLOSED))
      if ((u.ux != tgtx) || (u.uy != tgty))
        if ((abs(u.ux-tgtx) <= 1) && (abs(u.uy-tgty) <= 1))
          selected_action = JTP_ACTION_OPEN_DOOR;
  }

  /* default action for your own square */
  if ((!selected_action) && (u.ux == tgtx) && (u.uy == tgty))
    selected_action = JTP_ACTION_SEARCH;

  /* default action for adjacent squares (nonadjacent squares were handled further up)*/
  if (!selected_action)
    if ((u.ux != tgtx) || (u.uy != tgty))
      selected_action = JTP_ACTION_ATTACK;

  if (!selected_action) return(0);
  else return(jtp_construct_shortcut_action(tgtx, tgty, selected_action));
}


void jtp_view_inventory(void)
{
  int x, y;
  int inven_x, inven_y;
  int total_selectable_items;
  int selectable_items;
  int i;
  int quit_viewing_inventory;
  int inventory_page_changed;
  int pressedkey;
  unsigned char * backpack_bg;
  unsigned char * leftarrow_bg = NULL;
  unsigned char * rightarrow_bg = NULL;

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
  jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);

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
          int obj_id = glyph_to_obj(tempmenuitem->glyph);
          item_tile = jtp_object_to_tile(obj_id, -1, tempmenuitem->accelerator);
          
          if ((item_tile != V_TILE_NONE) && (jtp_tiles[item_tile]))
            jtp_put_tile(item_x + jtp_tiles[item_tile]->xmod,
                         item_y + jtp_tiles[item_tile]->ymod,
                         JTP_MAX_SHADES-1,
                         jtp_tiles[item_tile]->graphic);

          n_hotspots++;
          inven_hotspots = (jtp_hotspot **)realloc(inven_hotspots, n_hotspots*sizeof(jtp_hotspot *));
          inven_hotspots[n_hotspots-1] = malloc(sizeof(jtp_hotspot));
          (inven_hotspots[n_hotspots-1])->mcursor = jtp_mcursor[V_CURSOR_NORMAL];
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
        inven_hotspots[n_hotspots-1] = malloc(sizeof(jtp_hotspot));
        (inven_hotspots[n_hotspots-1])->mcursor = jtp_mcursor[V_CURSOR_NORMAL];
        (inven_hotspots[n_hotspots-1])->tooltip = jtp_make_tooltip("Previous page");
        (inven_hotspots[n_hotspots-1])->x1 = x + 71;
        (inven_hotspots[n_hotspots-1])->x2 = x + 193;
        (inven_hotspots[n_hotspots-1])->y1 = y + 410;
        (inven_hotspots[n_hotspots-1])->y2 = y + 443;
        (inven_hotspots[n_hotspots-1])->accelerator = MENU_PREVIOUS_PAGE;
        if (!leftarrow_bg)
          leftarrow_bg = jtp_get_img(x + 71, y + 408, x + 193, y + 445);
        jtp_put_stencil(x+71, y+408, jtp_defwin.invarrow_left);
      }
      else if (leftarrow_bg)
        jtp_put_img(x+71, y+408, leftarrow_bg);
        
      if (total_selectable_items - firstitem_index > 25) /* Add 'next page' arrow */
      {
        n_hotspots++;
        inven_hotspots = (jtp_hotspot **)realloc(inven_hotspots, n_hotspots*sizeof(jtp_hotspot *));
        inven_hotspots[n_hotspots-1] = malloc(sizeof(jtp_hotspot));
        (inven_hotspots[n_hotspots-1])->mcursor = jtp_mcursor[V_CURSOR_NORMAL];
        (inven_hotspots[n_hotspots-1])->tooltip = jtp_make_tooltip("Next page");
        (inven_hotspots[n_hotspots-1])->x1 = x + 448;
        (inven_hotspots[n_hotspots-1])->x2 = x + 570;
        (inven_hotspots[n_hotspots-1])->y1 = y + 410;
        (inven_hotspots[n_hotspots-1])->y2 = y + 443;
        (inven_hotspots[n_hotspots-1])->accelerator = MENU_NEXT_PAGE;
        if (!rightarrow_bg)
          rightarrow_bg = jtp_get_img(x + 448, y + 408, x + 570, y + 445);
        jtp_put_stencil(x+448, y+408, jtp_defwin.invarrow_right);
      }
      else if (rightarrow_bg)
        jtp_put_img(x+448, y+408, rightarrow_bg);

      jtp_refresh(&jtp_screen);
      inventory_page_changed = 0;
    }

    selected_hotspot = jtp_get_mouse_inventory_input(jtp_mcursor[V_CURSOR_NORMAL], inven_hotspots, n_hotspots, JTP_MBUTTON_LEFT);
    pressedkey = jtp_getch();
    if (pressedkey != 0)
    {
      if (pressedkey == JTP_MOUSEWHEEL_UP)
        pressedkey = MENU_PREVIOUS_PAGE;
      else if (pressedkey == JTP_MOUSEWHEEL_DOWN)
        pressedkey = MENU_NEXT_PAGE;
      if (pressedkey == '\033') /* ESC */
        quit_viewing_inventory = 1;
      else if ((pressedkey == MENU_NEXT_PAGE) || 
               (pressedkey == MENU_PREVIOUS_PAGE))
      {
        /* If the 'flip page' hotspot exists, select it */
        for (i = 0; i < n_hotspots; i++)        
          if ((inven_hotspots[i])->accelerator == pressedkey)
          {
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
        jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
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
        jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      }
    }
    else if ((jtp_mouseb == JTP_MBUTTON_LEFT) && (selected_hotspot >= 0))
    {
      if ((inven_hotspots[selected_hotspot])->accelerator == MENU_PREVIOUS_PAGE)
      {
        /* Wait for mouse button release, then redisplay the inventory */
        jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
        firstitem_index = firstitem_index - 25;
        inventory_page_changed = 1;
      }
      else if ((inven_hotspots[selected_hotspot])->accelerator == MENU_NEXT_PAGE)
      {
        /* Wait for mouse button release, then redisplay the inventory */
        jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
        firstitem_index = firstitem_index + 25;
        inventory_page_changed = 1;
      }
      else
      {
        jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      }
    }
    else if ((jtp_mouseb == JTP_MBUTTON_LEFT) && (selected_hotspot < 0))
    {
      jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      quit_viewing_inventory = 1;
    }
    else
    {
      jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
    }
  }

  /* Restore background and clean up */
  jtp_put_img(x, y, backpack_bg);
  jtp_refresh(&jtp_screen);
  free(backpack_bg);
  
  if (leftarrow_bg)
    free(leftarrow_bg);
  if (rightarrow_bg)
    free(rightarrow_bg);

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



static void jtp_recenter_from_minimap(void)
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
  jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
}



static void jtp_autopilot_from_minimap(void)
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
  jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
  u.tx = tx;
  u.ty = ty;  
}


char jtp_process_mouseclick(void)
{
  int hotspot;
  char selected_command;
  double cur_time;
  
  if (jtp_mouseb == JTP_MBUTTON_NONE) 
    return(0);

  hotspot = jtp_mouse_hotspot();
  cur_time = jtp_clocktick();
  switch (hotspot)
  {
    /* Scroll commands */
    case JTP_HOTSPOT_SCROLL_UP: 
       if (cur_time-jtp_last_scroll_time > jtp_min_scroll_delay)
       {
         jtp_last_scroll_time = cur_time;
         jtp_map_x--; jtp_map_y--; 
       }
       break;
    case JTP_HOTSPOT_SCROLL_DOWN: 
       if (cur_time-jtp_last_scroll_time > jtp_min_scroll_delay)
       {
         jtp_last_scroll_time = cur_time;
         jtp_map_x++; jtp_map_y++; 
       }
       break;
    case JTP_HOTSPOT_SCROLL_LEFT: 
       if (cur_time-jtp_last_scroll_time > jtp_min_scroll_delay)
       {
         jtp_last_scroll_time = cur_time;
         jtp_map_x--; jtp_map_y++; 
       }
       break;
    case JTP_HOTSPOT_SCROLL_RIGHT: 
       if (cur_time-jtp_last_scroll_time > jtp_min_scroll_delay)
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
        return(CMD_TRAVEL);
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
        if (cur_time-jtp_last_scroll_time > jtp_min_command_delay)
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
      jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      jtp_view_map();
      break;
    case JTP_HOTSPOT_BUTTON_MESSAGES:
      jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      jtp_view_messages();
      break;
    case JTP_HOTSPOT_BUTTON_HELP:
      jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      return(jtp_translate_command(JTP_NHCMD_HELP_MENU));
      break;
    case JTP_HOTSPOT_BUTTON_SPELLBOOK:
      jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      return(jtp_translate_command(JTP_NHCMD_CAST_SPELL));
      break;
    case JTP_HOTSPOT_BUTTON_INVENTORY:
      jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      return(jtp_translate_command(JTP_NHCMD_LIST_INVENTORY));
      break;
    case JTP_HOTSPOT_BUTTON_OPTIONS:
      jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      return(jtp_translate_command(JTP_NHCMD_SET_OPTIONS));
      break;
    case JTP_HOTSPOT_BUTTON_LOOK:
      jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
      jtp_is_shortcut_active = 1;
      jtp_shortcut_query_response = 'y';
      return(jtp_translate_command(JTP_NHCMD_EXPLAIN_SYMBOL)); 
      break;
    case JTP_HOTSPOT_BUTTON_EXTENDED:
      jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
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


char jtp_whatis_mouseclick(int *tx, int *ty)
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


int jtp_get_input
(
  int forced_x, int forced_y, /* Forced location of prompt window (-1 = not forced) */
  const char * ques,          /* Message to show player */
  char * input                /* Answer string */
)
{
  int i;
  int totalwidth, totalheight;
  int query_x, query_y;
  unsigned char * query_background;
  unsigned char * input_background;
  int key_result;

  /* Calculate width, height and position of query window */
  totalwidth = 500;
  i = jtp_text_length((char *)ques, JTP_FONT_HEADLINE) + 
      jtp_defwin.border_left[3] + 
      jtp_defwin.border_right[3];
  if (i > totalwidth) totalwidth = i;
  
  totalheight = jtp_text_height(ques, JTP_FONT_HEADLINE);
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
                                   jtp_text_height(ques, JTP_FONT_HEADLINE) + 
                                   jtp_fonts[JTP_FONT_INPUT].lineheight,
                                 query_x + totalwidth - jtp_defwin.border_right[3],
                                 query_y + jtp_defwin.border_top[1] + 
                                   jtp_text_height(ques, JTP_FONT_HEADLINE) + 
                                   2*jtp_fonts[JTP_FONT_INPUT].lineheight);
  /* Draw query message */
  jtp_put_text(jtp_defwin.border_left[3] + query_x + 1,
               jtp_defwin.border_top[1] + query_y + 
                 jtp_fonts[JTP_FONT_HEADLINE].baseline + 1,
               JTP_FONT_HEADLINE,
               JTP_COLOR_BACKGROUND,
               ques,
               jtp_screen.vpage);
  jtp_put_text(jtp_defwin.border_left[3] + query_x,
               jtp_defwin.border_top[1] + query_y + 
                 jtp_fonts[JTP_FONT_HEADLINE].baseline,
               JTP_FONT_HEADLINE,
               JTP_COLOR_TEXT,
               ques,
               jtp_screen.vpage);

  /* Display window */
  jtp_refresh(&jtp_screen);
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
                  jtp_text_height(ques, JTP_FONT_HEADLINE) +
                  jtp_fonts[JTP_FONT_INPUT].lineheight,
                input_background);

    
    
    jtp_put_text(jtp_defwin.border_left[3]+query_x+1, 
                 query_y + jtp_defwin.border_top[1] + 
                   jtp_text_height(ques, JTP_FONT_HEADLINE) + 
                   jtp_fonts[JTP_FONT_INPUT].lineheight +
                   jtp_fonts[JTP_FONT_INPUT].baseline + 1,
                 JTP_FONT_INPUT, JTP_COLOR_BACKGROUND,
                 input,
                 jtp_screen.vpage);
    jtp_put_text(jtp_defwin.border_left[3]+query_x+1, 
                 query_y + jtp_defwin.border_top[1] + 
                   jtp_text_height(ques, JTP_FONT_HEADLINE) + 
                   jtp_fonts[JTP_FONT_INPUT].lineheight +
                   jtp_fonts[JTP_FONT_INPUT].baseline,
                 JTP_FONT_INPUT, JTP_COLOR_TEXT,
                 input,
                 jtp_screen.vpage);
    /* Draw prompt */
    jtp_rect(query_x + jtp_defwin.border_left[3] + jtp_text_length(input, JTP_FONT_INPUT) + 2,
             query_y + jtp_defwin.border_top[1] + 
               jtp_text_height(ques, JTP_FONT_HEADLINE) +
               jtp_fonts[JTP_FONT_INPUT].lineheight,
             query_x + jtp_defwin.border_left[3] + jtp_text_length(input, JTP_FONT_INPUT) + 2,
             query_y + jtp_defwin.border_top[1] +
               jtp_text_height(ques, JTP_FONT_HEADLINE) +
               jtp_fonts[JTP_FONT_INPUT].lineheight +
               jtp_fonts[JTP_FONT_INPUT].baseline,
             15);
    /* Display window */
    jtp_refresh(&jtp_screen);
  
    key_result = jtp_getch();

    /* if ((key_result < 'a') || (key_result > 'z')) key_result = '?'; */
    if (key_result != 0 && key_result < 0x100 && (jtp_fonts[JTP_FONT_INPUT].fontpics[key_result].kuva != NULL) && (i < 80))
    {
      input[i] = key_result;
      input[i+1] = '\0';
      i++;
    }

    /* Backspace key */
    if ((key_result == 8)  || (jtp_text_length(input, JTP_FONT_INPUT) > 490))
      if (i>0)
      {
        input[i-1]='\0';
        i--;
      }

  } while (key_result != '\n'/*Enter*/ && key_result != '\033'/*Escape*/);


  /* Restore background */
  jtp_put_img(query_x, query_y, query_background);
  jtp_refresh(&jtp_screen);

  /* Clean up */
  free(query_background);
  free(input_background);
  return key_result == '\n'/*Enter*/;
}



void jtp_get_menu_coordinates(jtp_window *menuwindow)
{
  int i, j;
  int totalwidth, totalheight;
  int menuitems_width, menuitems_height;
  int prompt_width, prompt_height;
  int buttons_width, buttons_height;
  int top_separator_height, bottom_separator_height;
  jtp_menuitem * tempmenuitem;
  jtp_button * tempbutton;

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
  else
    top_separator_height = 0;

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
  else
    bottom_separator_height = 0;
    
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
  menuwindow->menu->items_height = menuitems_height;
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
}



int jtp_get_menu_selection(jtp_window *menuwindow)
{
  int selectedbutton;
  jtp_menuitem * tempmenuitem, * tempmenuitem2, * firstitem;
  jtp_menuitem * lastitem;
  jtp_button * tempbutton;
  unsigned char * menubackground;
  unsigned char * tempimage;
  int pressedkey;
  char widget_found;
  double cur_time;
  int backpack_type_selection = 0;
  int n_menuitems, target_item = 0;

  if ((!menuwindow) || (!menuwindow->menu) || (!menuwindow->buttons))
    return(1);

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

  jtp_refresh(&jtp_screen);
  /* If the palette is faded out for some reason, restore it */
  jtp_updatepal(0, 255);

  selectedbutton = -1;
  while (selectedbutton < 0)
  {
    jtp_keymouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_LEFT);
    pressedkey = jtp_getch();

    cur_time = jtp_clocktick();

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
                         jtp_mcursor[V_CURSOR_NORMAL]);
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

    switch (pressedkey)
    {
    case MENU_FIRST_PAGE:
    case MENU_LAST_PAGE:
    case MENU_NEXT_PAGE:
    case MENU_PREVIOUS_PAGE:
      /* handled below */
      break;
    case MENU_SELECT_ALL:
      if (menuwindow->menu->selectiontype == PICK_ANY)
      {
        /* Set all menu items to 'not selected' */
        jtp_list_reset(menuwindow->menu->items);
        tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        while (tempmenuitem2)
        {
          if (tempmenuitem2->count != JTP_NOT_SELECTABLE)
            tempmenuitem2->selected = TRUE;
          jtp_list_advance(menuwindow->menu->items);
          tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);              
        }
        jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);
        jtp_refresh(&jtp_screen);
        widget_found = 1;
      }
      break;
    case MENU_UNSELECT_ALL:
      if (menuwindow->menu->selectiontype == PICK_ANY)
      {
        /* Set all menu items to 'not selected' */
        jtp_list_reset(menuwindow->menu->items);
        tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        while (tempmenuitem2)
        {
          if (tempmenuitem2->count != JTP_NOT_SELECTABLE)
            tempmenuitem2->selected = FALSE;
          jtp_list_advance(menuwindow->menu->items);
          tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        }
        jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);
        jtp_refresh(&jtp_screen);
        widget_found = 1;
      }
      break;
    case MENU_INVERT_ALL:
      if (menuwindow->menu->selectiontype == PICK_ANY)
      {
        /* Set all menu items to 'not selected' */
        jtp_list_reset(menuwindow->menu->items);
        tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        while (tempmenuitem2)
        {
          if (tempmenuitem2->count != JTP_NOT_SELECTABLE)
            tempmenuitem2->selected = !tempmenuitem2->selected;
          jtp_list_advance(menuwindow->menu->items);
          tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        }
        jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);
        jtp_refresh(&jtp_screen);
        widget_found = 1;
      }
      break;
    case MENU_SELECT_PAGE:
      if (menuwindow->menu->selectiontype == PICK_ANY)
      {
        /* Set all menu items to 'not selected' */
        tempmenuitem2 = firstitem;
        while (tempmenuitem2)
        {
          if (tempmenuitem2->count != JTP_NOT_SELECTABLE)
            tempmenuitem2->selected = TRUE;
          jtp_list_advance(menuwindow->menu->items);
          tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
          if (tempmenuitem2 != NULL && (tempmenuitem2->y - firstitem->y) >= menuwindow->menu->items_height)
            break;
        }
        jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);
        jtp_refresh(&jtp_screen);
        widget_found = 1;
      }
      break;
    case MENU_UNSELECT_PAGE:
      if (menuwindow->menu->selectiontype == PICK_ANY)
      {
        /* Set all menu items to 'not selected' */
        tempmenuitem2 = firstitem;
        while (tempmenuitem2)
        {
          if (tempmenuitem2->count != JTP_NOT_SELECTABLE)
            tempmenuitem2->selected = FALSE;
          jtp_list_advance(menuwindow->menu->items);
          tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
          if (tempmenuitem2 != NULL && (tempmenuitem2->y - firstitem->y) >= menuwindow->menu->items_height)
            break;
        }
        jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);
        jtp_refresh(&jtp_screen);
        widget_found = 1;
      }
      break;
    case MENU_INVERT_PAGE:
      if (menuwindow->menu->selectiontype == PICK_ANY)
      {
        /* Set all menu items to 'not selected' */
        tempmenuitem2 = firstitem;
        while (tempmenuitem2)
        {
          if (tempmenuitem2->count != JTP_NOT_SELECTABLE)
            tempmenuitem2->selected = !tempmenuitem2->selected;
          jtp_list_advance(menuwindow->menu->items);
          tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
          if (tempmenuitem2 != NULL && (tempmenuitem2->y - firstitem->y) >= menuwindow->menu->items_height)
            break;
        }
        jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);
        jtp_refresh(&jtp_screen);
        widget_found = 1;
      }
      break;
    case MENU_SEARCH:
      if (menuwindow->menu->selectiontype == PICK_ANY || menuwindow->menu->selectiontype == PICK_ONE)
      {
      	char buf[BUFSZ];
      	
      	if (jtp_get_input(-1, -1, "Search for:", buf) && *buf)
      	{
      	  lastitem = NULL;
          jtp_list_reset(menuwindow->menu->items);
          tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
          while (tempmenuitem2)
          {
            if (tempmenuitem2->count != JTP_NOT_SELECTABLE)
            {
              if (strstr(tempmenuitem2->text, buf))
              {
                tempmenuitem2->selected = TRUE;
                if (lastitem == NULL)
                  lastitem = tempmenuitem2;
                if (menuwindow->menu->selectiontype == PICK_ONE)
                  break;
              } else if (menuwindow->menu->selectiontype == PICK_ANY)
              {
                tempmenuitem2->selected = FALSE;
              }
            }
            jtp_list_advance(menuwindow->menu->items);
            tempmenuitem2 = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
          }
          if (lastitem != NULL)
          {
            firstitem = lastitem;
            tempimage = jtp_draw_window(menuwindow->x, menuwindow->y, 
                                    menuwindow->width, menuwindow->height);
            free(tempimage);
          }
          jtp_draw_menu(menuwindow->x, menuwindow->y, menuwindow->menu, firstitem);
          jtp_refresh(&jtp_screen);
      	}
      	widget_found = 1;
      }
      break;
    }
    
    while ((selectedbutton < 0) && (!widget_found) && (tempmenuitem))
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
        jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);

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
          jtp_refresh(&jtp_screen);
        }
                
      }
      
      jtp_list_advance(menuwindow->menu->items);
      tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
    }

    if (cur_time-jtp_last_scroll_time > jtp_min_scroll_delay)
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
      else if (pressedkey == MENU_PREVIOUS_PAGE)
      {
        widget_found = 1; target_item = 1;
      }
      else if (pressedkey == JTP_MOUSEWHEEL_UP)
      {
        widget_found = 1; target_item = 3;
      }
      else if (pressedkey == MENU_PREVIOUS_PAGE)
      {
        widget_found = 1; target_item = 10;
      }
      else if (pressedkey == MENU_FIRST_PAGE)
      {
        widget_found = 1; target_item = 9999;
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
          if (menuwindow->menu->items->previous == menuwindow->menu->items->header)
            break;
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
        jtp_refresh(&jtp_screen);
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
      else if (pressedkey == MENU_NEXT_PAGE)
      {
        widget_found = 1; target_item = 1;
      }
      else if (pressedkey == JTP_MOUSEWHEEL_DOWN)
      {
        widget_found = 1; target_item = 3;
      }
      else if (pressedkey == MENU_NEXT_PAGE)
      {
        widget_found = 1; target_item = 10;
      }
      else if (pressedkey == MENU_LAST_PAGE)
      {
        widget_found = 1; target_item = 9999;
      }

      if (widget_found == 1)
      {
        jtp_list_reset(menuwindow->menu->items);
        tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        while (tempmenuitem)
        {
          jtp_list_advance(menuwindow->menu->items);
          tempmenuitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
        }
        jtp_list_retreat(menuwindow->menu->items);
        lastitem = (jtp_menuitem *)jtp_list_current(menuwindow->menu->items);
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
          if ((lastitem->y - ((jtp_menuitem *)jtp_list_current(menuwindow->menu->items))->y + lastitem->height) <= menuwindow->menu->items_height)
            target_item = 0;
          else
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
        jtp_refresh(&jtp_screen);
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
        jtp_refresh(&jtp_screen);
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
                     menuwindow->y + menuwindow->height - 1, &jtp_screen);
  
  /* Clean up and exit */
  free(menubackground);
  return(selectedbutton);
}
