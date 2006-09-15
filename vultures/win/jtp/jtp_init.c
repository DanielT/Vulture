/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000				  */

#include <unistd.h>
#include "jtp_mou.h"
#include "jtp_win.h"
#include "jtp_map.h"
#include "jtp_tile.h"
#include "jtp_gra.h"
#include "jtp_gen.h"
#include "jtp_sdl.h"
#include "jtp_gfl.h"
#include "jtp_keys.h"
#include "jtp_txt.h"
#include "jtp_init.h"
#include "winjtp.h"

/*----------------------------
 * constants
 *---------------------------- */
/* 
 * External files used by the GUI.
 * Note: The intro sequence may use other
 * external images, listed in the script file.
 * For DOS compatibility, use short filenames.
 */
#define JTP_MAX_FILENAME_LENGTH 1024

#define JTP_FILENAME_INTRO_SCRIPT  "jtp_intr.txt"
#define JTP_FILENAME_OPTIONS       "jtp_opts.txt"
#define JTP_FILENAME_SOUNDS_CONFIG "jtp_snds.txt"
#define JTP_FILENAME_KEYS_CONFIG   "jtp_keys.txt"
#define JTP_FILENAME_SHADING_TABLE "jtp_lit1.dat"

/*
 * graphics files, without ".pcx" extension
 */
#ifdef VULTURESEYE
#define JTP_FILENAME_NETHACK_LOGO         "jtp_nh_1"
#endif
#ifdef VULTURESCLAW
#define JTP_FILENAME_NETHACK_LOGO         "jtp_se_1"
#endif
#define JTP_FILENAME_CHARACTER_GENERATION "chargen2"
#define JTP_FILENAME_FONT_SMALL           "ttipchr1"
#define JTP_FILENAME_FONT_LARGE           "menuchr2"
#define JTP_FILENAME_WINDOW_STYLE         "jtp_win1"
#define JTP_FILENAME_STATUS_BAR           "jtp_st10"
#define JTP_FILENAME_MAP_PARCHMENT        "jtp_mwi4"
#define JTP_FILENAME_MAP_SYMBOLS          "micons2"
#define JTP_FILENAME_BACKPACK             "backpac5"
#define JTP_FILENAME_SPELLBOOK            "book1"
#define JTP_FILENAME_SPELL_SYMBOLS        "book2"
#define JTP_FILENAME_MOUSE_CURSORS        "jtp_mou5"

/*
 * Ending scenes. Eventually these could
 * be made configurable.
 */
#define JTP_FILENAME_ENDING_DIED          "cairn"
#define JTP_FILENAME_ENDING_ASCENDED      "night"
#define JTP_FILENAME_ENDING_QUIT          "quitgame"
#define JTP_FILENAME_ENDING_ESCAPED       "escaped"


#ifdef SANITY_CHECKS
    static char *lastpcx;
    #define setlastpcx(filename) if (lastpcx) free(lastpcx); lastpcx = jtp_strdup(filename)
#else
    #define setlastpcx(filename)
#endif

/* why is this here? It should ALWAYS be defined by unistd.h */
#ifndef R_OK
#  define R_OK 4
#endif

/*----------------------------
 * global variables
 *---------------------------- */
/* Wall display styles (game option) */
enum jtp_wall_display_styles {
  JTP_WALL_DISPLAY_STYLE_FULL=0,
  JTP_WALL_DISPLAY_STYLE_HALF_HEIGHT,
  JTP_WALL_DISPLAY_STYLE_TRANSPARENT
};

/* wall display style in use */
static int jtp_wall_display_style;


/* Translation tables: NetHack glyph to Vulture's tile */
int * jtp_montiles = NULL;        /* Monster glyph to monster tile */
int * jtp_objtiles = NULL;        /* Object glyph to object tile */
int * jtp_cmaptiles = NULL;       /* Cmap glyph to cmap tile (walls and so on) */
int * jtp_engulfmap = NULL;       /* Maps the engulfing monster type to its engulf tile */

int jtp_tile_conversion_initialized = 0; /* Have the conversion tables been set up? */

int jtp_map_width, jtp_map_height; /* Map dimensions */
int jtp_map_x, jtp_map_y;  /* Center of displayed map area */


/*----------------------------
 * pre-declared functions
 *---------------------------- */
static void trimright(char *buf);
static void jtp_show_intro(const char *introscript_name);


/*----------------------------
 * function implementaions
 *---------------------------- */
static void jtp_read_options(int *screen_width, int *screen_height)
{
  FILE * f;
  char tempbuffer[1024];
  char *tok;
  char *tok2;
  int soundtype;
  char *filename;

  /* printf("DEBUG[jtp_win.c/30]: Reading options from [%s]\n", optfile); */

  *screen_width = 800;
  *screen_height = 600;

  /* Read interface options */
  filename = jtp_make_filename(JTP_CONFIG_DIRECTORY, NULL, JTP_FILENAME_OPTIONS);
  f = fopen(filename, "rb");
  free(filename);
  if (f == NULL)
  {
    /* Use defaults */
  } else
  {
    while (fgets(tempbuffer, sizeof(tempbuffer), f))
    {
      tok = NULL;
      if (tempbuffer[0] != '%')
        tok = strstr(tempbuffer, "=");

      if (tok)
      {
        if (!strncmp(tempbuffer, "screen_xsize", tok - tempbuffer))
          *screen_width = atoi(tok + 1);
        else if (!strncmp(tempbuffer, "screen_ysize", tok - tempbuffer))
          *screen_height = atoi(tok + 1);
        else if (!strncmp(tempbuffer, "command_delay", tok - tempbuffer))
          jtp_min_command_delay = atof(tok + 1);
        else if (!strncmp(tempbuffer, "scrolling_delay", tok - tempbuffer))
          jtp_min_scroll_delay = atof(tok + 1);
        else if (!strncmp(tempbuffer, "wall_style", tok - tempbuffer))
        {
          if (!strncmp(tok + 1, "full", 4))
            jtp_wall_display_style = JTP_WALL_DISPLAY_STYLE_FULL;
          else if (!strncmp(tok + 1, "half_height", 11))
            jtp_wall_display_style = JTP_WALL_DISPLAY_STYLE_HALF_HEIGHT;
          else if (!strncmp(tok + 1, "transparent", 11))
            jtp_wall_display_style = JTP_WALL_DISPLAY_STYLE_TRANSPARENT;
        }
        else if (!strncmp(tempbuffer, "recenter_after_movement", tok - tempbuffer))
          jtp_recenter_after_movement = atoi(tok + 1);
        else if (!strncmp(tempbuffer, "play_music", tok - tempbuffer))
          jtp_play_music = atoi(tok + 1);
        else if (!strncmp(tempbuffer, "play_effects", tok - tempbuffer))
          jtp_play_effects = atoi(tok + 1);
        else if (!strncmp(tempbuffer, "one_command_per_click", tok - tempbuffer))
          jtp_one_command_per_click = atoi(tok + 1);
        else if (!strncmp(tempbuffer, "fullscreen", tok - tempbuffer))
          iflags.wc2_fullscreen = atoi(tok + 1);
        else if (!strncmp(tempbuffer, "gamma_correction", tok - tempbuffer))
          jtp_gamma_correction = atof(tok + 1);
        else if (!strncmp(tempbuffer, "midi_player", tok - tempbuffer) ||
          !strncmp(tempbuffer, "linux_midi_player", tok - tempbuffer))
        {
          jtp_external_midi_player_command = (char *)malloc(strlen(tok + 1) + 1);
          strcpy(jtp_external_midi_player_command, tok + 1);
          /* Remove end-of-line from the string */
          trimright(jtp_external_midi_player_command);
        }
        else if (!strncmp(tempbuffer, "mp3_player", tok - tempbuffer) ||
          !strncmp(tempbuffer, "linux_mp3_player", tok - tempbuffer))
        {
          jtp_external_mp3_player_command = (char *)malloc(strlen(tok + 1) + 1);
          strcpy(jtp_external_mp3_player_command, tok + 1);
          /* Remove end-of-line from the string */
          trimright(jtp_external_mp3_player_command);
        }
      }
    }
    fclose(f);
  }
  
  /* Read key mapping options */
  /* Create the key binding table */
  jtp_keymaps = (jtp_command *)calloc(sizeof(jtp_command), JTP_MAX_NETHACK_COMMANDS);
  jtp_set_nethack_keys();
  /* jtp_set_default_keymaps(); */ /* Use whatever NetHack has */
  filename = jtp_make_filename(JTP_CONFIG_DIRECTORY, NULL, JTP_FILENAME_KEYS_CONFIG);
  jtp_read_key_configuration(filename);
  free(filename);

  /* Read event sounds options */
  jtp_event_sounds = NULL;
  jtp_n_event_sounds = 0;

  filename = jtp_make_filename(JTP_CONFIG_DIRECTORY, NULL, JTP_FILENAME_SOUNDS_CONFIG);
  f = fopen(filename, "rb");
  free(filename);
  if (f == NULL) return;
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
      if ((tok = strstr(tempbuffer, "],SND,[")) != NULL) /* sound found */
        soundtype = JTP_EVENT_SOUND_TYPE_SND;      
      else if ((tok = strstr(tempbuffer, "],MUS,[")) != NULL) /* music found */      
        soundtype = JTP_EVENT_SOUND_TYPE_MUS;      
      else if ((tok = strstr(tempbuffer, "],RSNG,[")) != NULL) /* Random song file found */
        soundtype = JTP_EVENT_SOUND_TYPE_RANDOM_SONG;
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

        if (soundtype == JTP_EVENT_SOUND_TYPE_SND||soundtype == JTP_EVENT_SOUND_TYPE_MUS) tok = tok + 7;
        else tok = tok + 8;

        tok2 = strstr(tok, "]");
        memcpy((jtp_event_sounds[jtp_n_event_sounds-1])->filename, tok, tok2-tok);
        (jtp_event_sounds[jtp_n_event_sounds-1])->filename[tok2-tok] = '\0';

        (jtp_event_sounds[jtp_n_event_sounds-1])->soundtype = soundtype;


        /* If this isn't a CD track, add path to sounds subdirectory before filename */
        if (soundtype != JTP_EVENT_SOUND_TYPE_CD_AUDIO)
        {
          char *tmp = jtp_event_sounds[jtp_n_event_sounds-1]->filename;
          jtp_event_sounds[jtp_n_event_sounds-1]->filename = jtp_make_filename(soundtype == JTP_EVENT_SOUND_TYPE_SND ? JTP_SOUND_DIRECTORY : JTP_MUSIC_DIRECTORY, NULL, tmp);
          free(tmp);
        }

        jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "Mapped [%s] to [%s]\n", (jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern, (jtp_event_sounds[jtp_n_event_sounds-1])->filename);
      }
    }
  }
  fclose(f);
}



static void trimright(char *buf)
{
    int i;

    i = strlen(buf) - 1;
    while (i >= 0 && (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\r'))
        i--;
    buf[i + 1] = '\0';
}


void jtp_show_logo_screen(void)
{
  unsigned char *logo;
  
  jtp_play_event_sound("nhfe_music_main_title");

  if (iflags.wc_splash_screen)
  {
    logo = jtp_load_graphic(NULL, JTP_FILENAME_NETHACK_LOGO, TRUE);
    if (logo != NULL)
    {
      int w, h;
      
      jtp_get_dimensions(logo, &w, &h);
      /*
       * should stretch the image to fit screen,
       * but there is no such function, so just fill screen
       * with color in upper left corner.
       */
      jtp_rect(0, 0, jtp_screen.width - 1, jtp_screen.height - 1, logo[4]);
      jtp_put_img((jtp_screen.width - w) / 2, (jtp_screen.height - h) / 2, logo);
      jtp_blankpal(0, 255);
      jtp_refresh(&jtp_screen);
      jtp_game_palette_set = 0;
  
      jtp_fade_in(1.0);
      jtp_refresh(&jtp_screen);
      while (jtp_getch() == 0 && jtp_readmouse() == JTP_MBUTTON_NONE)
        jtp_msleep(5);
      jtp_fade_out(0.3);
      
      free(logo);
    }
  }
      
  /* Restore regular game palette */
  memcpy(jtp_colors, jtp_game_colors, sizeof(jtp_colors));
  jtp_clear_screen();
  jtp_refresh(&jtp_screen);
  jtp_game_palette_set = 1;
}



void jtp_select_player(void)
{
  int i, j, k, n;
  winid win;
  anything any;
  menu_item *selected = 0;
  char thisch, lastch = 0;
  char pbuf[1024];
  char *logo;
  int w, h;
  char *filename;
  
  jtp_clear_screen();
  /*
   * FIXME: loading the palette here has to make
   * sure that it contains the colors needed for displaying menus.
   */
  logo = jtp_load_graphic(NULL, JTP_FILENAME_CHARACTER_GENERATION, TRUE);
  if (logo != NULL)
  {
    jtp_get_dimensions(logo, &w, &h);
    jtp_put_img((jtp_screen.width - w) / 2, (jtp_screen.height - h) / 2, logo);
    free(logo);
  }
  jtp_blankpal(0, 255);
  jtp_refresh(&jtp_screen);
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
/*
 * WE made the choice, not the player, so don't bother complaining.
 * BTW: if pick_role() returns < 0, no valid role could be found,
 * and we should better go back picking another race/gender/alignment.
 */
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
        any.a_int = i + 1;    /* must be non-zero */
        thisch = lowc(roles[i].name.m[0]);
        if (thisch == lastch)
          thisch = highc(thisch);
        add_menu(win, NO_GLYPH, &any, thisch, 0, ATR_NONE, an(roles[i].name.m), MENU_UNSELECTED);
        lastch = thisch;
      }
      any.a_int = randrole() + 1;  /* must be non-zero */
      add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random", MENU_SELECTED);
      any.a_int = i + 1;      /* must be non-zero */
      add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit", MENU_UNSELECTED);
      end_menu(win, "Pick a role");
      n = select_menu(win, PICK_ONE, &selected);
      destroy_nhwindow(win);

      /* Process the choice */
      if (n != 1 || selected[0].item.a_int == any.a_int)
      {
        /* Selected quit */
        if (selected)
          free((genericptr_t) selected);
        bail((char *) 0);
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
        if (validrace(flags.initrole, i))
          n++;
        else if ((i == k) && (!races[++k].noun))
          k = 0;

        if (!races[++i].noun)
          i = 0;
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
            any.a_int = i + 1;  /* must be non-zero */
            add_menu(win, NO_GLYPH, &any, races[i].noun[0], 0, ATR_NONE, races[i].noun, MENU_UNSELECTED);
          }
        any.a_int = randrace(flags.initrole) + 1;
        add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random", MENU_SELECTED);
        any.a_int = i + 1;      /* must be non-zero */
        add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit", MENU_UNSELECTED);
        Sprintf(pbuf, "Pick the race of your %s", roles[flags.initrole].name.m);
        end_menu(win, pbuf);
        n = select_menu(win, PICK_ONE, &selected);
        destroy_nhwindow(win);
        if (n != 1 || selected[0].item.a_int == any.a_int)
        {
          /* Selected quit */
          if (selected)
            free((genericptr_t) selected);
          bail((char *) 0);
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
        if (validgend(flags.initrole, flags.initrace, i))
          n++;
        else if ((i == k) && (++k >= ROLE_GENDERS))
          k = 0;

        if (++i >= ROLE_GENDERS)
          i = 0;
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
            any.a_int = i + 1;  /* must be non-zero */
            add_menu(win, NO_GLYPH, &any, genders[i].adj[0], 0, ATR_NONE, genders[i].adj, MENU_UNSELECTED);
          }
        any.a_int = randgend(flags.initrole, flags.initrace) + 1;
        add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random", MENU_SELECTED);
        any.a_int = i + 1;      /* must be non-zero */
        add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit", MENU_UNSELECTED);
        sprintf(pbuf, "Pick the gender of your %s %s", races[flags.initrace].adj, roles[flags.initrole].name.m);
        end_menu(win, pbuf);
        n = select_menu(win, PICK_ONE, &selected);
        destroy_nhwindow(win);
        if (n != 1 || selected[0].item.a_int == any.a_int)
        {
          /* Selected quit */
          if (selected)
            free((genericptr_t) selected);
          bail((char *) 0);
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
        if (validalign(flags.initrole, flags.initrace, i))
          n++;
        else if ((i == k) && (++k >= ROLE_ALIGNS))
          k = 0;

        if (++i >= ROLE_ALIGNS)
          i = 0;
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
            any.a_int = i + 1;  /* must be non-zero */
            add_menu(win, NO_GLYPH, &any, aligns[i].adj[0], 0, ATR_NONE, aligns[i].adj, MENU_UNSELECTED);
          }
        any.a_int = randalign(flags.initrole, flags.initrace) + 1;
        add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random", MENU_SELECTED);
        any.a_int = i + 1;      /* must be non-zero */
        add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit", MENU_UNSELECTED);
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
          if (selected)
            free((genericptr_t) selected);
          bail((char *) 0);
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
  filename = jtp_make_filename(JTP_CONFIG_DIRECTORY, NULL, JTP_FILENAME_INTRO_SCRIPT);
  jtp_show_intro(filename);
  free(filename);
  
  /* Restore regular game palette */
  memcpy(jtp_colors, jtp_game_colors, sizeof(jtp_colors));
  jtp_clear_screen();
  jtp_refresh(&jtp_screen);
  jtp_updatepal(0, 255);
  jtp_game_palette_set = 1;  
}



void jtp_askname(void)
{
	int done;
	
	done = jtp_get_input(-1, jtp_screen.height - 170,
				  "What is your name?", plname);
	if (!done)
	{
		/* Player pressed ESC during the name query, so quit the game */
		bail((char *) 0);
	}
	
	/* do not fade out if user didn't enter anything, we will come back here */
	
	if (plname[0])
	{
		jtp_fade_out(0.2);
	}
}



static void jtp_show_intro(const char *introscript_name)
{
  FILE   * f;
  int      i, j;
  int      nScenes;
  int    * subtitle_rows;
  char *** subtitles;
  char  ** scene_images;
  char   * tempbuffer;
  double  start_clock, cur_clock;
  int      lineno;
  unsigned char *image;
  int image_width = 0, image_height = 0;
  
  nScenes = 0;
  scene_images = NULL;
  subtitle_rows = NULL;
  subtitles = NULL;
  tempbuffer = (char *)malloc(1024);
  if (!tempbuffer)
  {
    OOM(1);
  }

  jtp_play_event_sound("nhfe_music_introduction");
  
  f = fopen(introscript_name, "rb");
  if (f == NULL)
  {
    jtp_write_log_message(JTP_LOG_NOTE, NULL, 0, "intro script %s not found\n", introscript_name);
  } else
  {
    lineno = 1;
    while (fgets(tempbuffer, 1024, f))
    {
      if (tempbuffer[0] == '%') /* Start of new scene */
      {
        trimright(&tempbuffer[1]);
        i = strlen(&tempbuffer[1]);
        if (i > 0)
        {
          char *dot;
          
          nScenes++;
          scene_images = (char **)realloc(scene_images, nScenes*sizeof(char *));
          scene_images[nScenes-1] = (char *)malloc((i + 1) * sizeof(*tempbuffer));
          if (!scene_images[nScenes - 1])
          {
            OOM(1);
          }
          strcpy(scene_images[nScenes - 1], tempbuffer + 1);
          dot = strrchr(scene_images[nScenes - 1], '.');
          if (dot != NULL)
          {
            if (strcmp(dot, ".pcx") == 0)
            {
              *dot = '\0';
            } else
            {
              jtp_write_log_message(JTP_LOG_NOTE, NULL, 0, "scene image %s not pcx?", scene_images[nScenes - 1]);
            }
          }
          subtitle_rows = (int *)realloc(subtitle_rows, nScenes * sizeof(int));

          subtitle_rows[nScenes - 1] = 0;
          subtitles = (char ***)realloc(subtitles, nScenes * sizeof(char **));

          subtitles[nScenes - 1] = NULL;
        }
      }
      else /* New subtitle line for latest scene */
      {
        if (nScenes > 0)
        {
          subtitle_rows[nScenes - 1]++;
          subtitles[nScenes - 1] = (char **)realloc(subtitles[nScenes - 1],
            subtitle_rows[nScenes - 1] * sizeof(char *));
          
          /* Remove extra whitespace from line */
          trimright(tempbuffer);
          i = 0;
          while (tempbuffer[i] == ' ')
            i++;
          trimright(&tempbuffer[i]);
          /* Copy line to subtitle array */
          subtitles[nScenes - 1][subtitle_rows[nScenes - 1] - 1] = (char *)malloc(strlen(tempbuffer + i) + 1);
          if (!subtitles[nScenes - 1][subtitle_rows[nScenes - 1] - 1])
          {
            OOM(1);
          }
          strcpy(subtitles[nScenes - 1][subtitle_rows[nScenes - 1] - 1], tempbuffer + i);
          /* DEBUG printf("%s", subtitles[nScenes-1][subtitle_rows[nScenes-1]-1]); DEBUG */
        } else
        {
          jtp_write_log_message(JTP_LOG_NOTE, NULL, 0, "subtitle without a preceding scene in line %d of intro script %s\n",
            lineno, introscript_name);
        }
      }
    }
    fclose(f);

    if (nScenes == 0)
    {
      jtp_write_log_message(JTP_LOG_NOTE, NULL, 0, "no scenes found in intro script %s\n", introscript_name);
    } else
    {
      /*
       * Show each scene of the introduction in four steps:
       * - Erase previous image, load and fade in new image
       * - Print the subtitles
       * - Wait out a set delay
       * - Erase subtitles, Fade out 
       */
      jtp_set_draw_region(0, 0, jtp_screen.width - 1, jtp_screen.height - 1);
      for (i = 0; i < nScenes; i++)
      {
        /* If we are starting, or the previous image was different, fade in the current image */
        if ((i <= 0) || (strcmp(scene_images[i], scene_images[i - 1]) != 0))
        {
          image = jtp_load_graphic(NULL, scene_images[i], TRUE);
          jtp_get_dimensions(image, &image_width, &image_height);
          jtp_clear_screen();
          jtp_put_img((jtp_screen.width - image_width) / 2, (jtp_screen.height - image_height) / 6, image);
          jtp_refresh(&jtp_screen);
          jtp_fade_in(0.2);
        }
        /* Show subtitles */
        for (j = 0; j < subtitle_rows[i]; j++)
        {
          jtp_put_text((jtp_screen.width - jtp_text_length(subtitles[i][j], JTP_FONT_INTRO)) / 2,
                 2 * (jtp_screen.height - image_height) / 6 + image_height +
                 j * jtp_fonts[JTP_FONT_INTRO].lineheight,
                 JTP_FONT_INTRO, JTP_COLOR_INTRO_TEXT,
                 subtitles[i][j],
                 jtp_screen.vpage);
        }
        jtp_refresh(&jtp_screen);
    
        /* Wait until scene is over or player pressed a key */
        /* FIXME: this is a busy loop with event polling;
           also should make the timeout depend on amount of text */
        start_clock = jtp_clocktick();
        cur_clock = start_clock;
        while (cur_clock - start_clock < 5)
        {
          cur_clock = jtp_clocktick();
          if (jtp_kbhit() || jtp_readmouse() != JTP_MBUTTON_NONE)
          {
            jtp_getch();
            cur_clock = start_clock + 7;
            i = nScenes;
          } else
          {
          	jtp_msleep(10);
          }
        }
    
        /* Erase subtitles */
    
        jtp_fill_rect(0, (jtp_screen.height - image_height) / 6 + image_height,
                jtp_screen.width - 1, jtp_screen.height - 1, 0);
        jtp_refresh(&jtp_screen);
    
        /* If we are at the end, or the next image is different, fade out the current image */
        if ((i >= nScenes - 1) || (strcmp(scene_images[i], scene_images[i + 1]) != 0))
          jtp_fade_out(0.2);
      }
      
      /* Clean up */
      for (i = 0; i < nScenes; i++)
      {
        free(scene_images[i]);
        for (j = 0; j < subtitle_rows[i]; j++)
          free(subtitles[i][j]);
        if (subtitles[i] != NULL)
          free(subtitles[i]);
      }
      if (subtitle_rows != NULL)
        free(subtitle_rows);
      if (scene_images != NULL)
        free(scene_images);
    }
  }
  jtp_blankpal(0, 255);
  jtp_game_palette_set = 0;
}



void jtp_show_ending(jtp_window *tempwindow)
{
  int i, j, k, totallines;
  jtp_menuitem * tempmenuitem;
  char tempbuffer[1024];
  unsigned char *image;
  int image_width, image_height;
  
  /* 
   * Assume that the screen has been faded out by now.
   * Load the appropriate 'ending' image.
   */
  jtp_clear_screen();
  jtp_game_palette_set = 0;
  if (tempwindow->ending_type == QUIT)
  {
    image = jtp_load_graphic(NULL, JTP_FILENAME_ENDING_QUIT, TRUE);
    jtp_play_event_sound("nhfe_music_end_quit");
  }
  else if (tempwindow->ending_type == ASCENDED)
  {
    image = jtp_load_graphic(NULL, JTP_FILENAME_ENDING_ASCENDED, TRUE);
    jtp_play_event_sound("nhfe_music_end_ascended");
  }
  else if (tempwindow->ending_type == ESCAPED)
  {
    image = jtp_load_graphic(NULL, JTP_FILENAME_ENDING_ESCAPED, TRUE);
    jtp_play_event_sound("nhfe_music_end_ascended");
  }
  else if (tempwindow->ending_type < PANICKED)
  {
    image = jtp_load_graphic(NULL, JTP_FILENAME_ENDING_DIED, TRUE);
    jtp_play_event_sound("nhfe_music_end_died");
  }
  else
  {
    jtp_clear_screen();  /* No image associated with this ending */
    image = NULL;
  }
  if (image != NULL)
  {
    jtp_get_dimensions(image, &image_width, &image_height);
    jtp_put_img((jtp_screen.width - image_width) / 2, (jtp_screen.height - image_height) / 2, image);
    free(image);
  }
  
  jtp_refresh(&jtp_screen);
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
                   JTP_FONT_INTRO, JTP_COLOR_BACKGROUND,
                   tempbuffer,
                   jtp_screen.vpage);
      jtp_put_text(j, 
                   k + i*jtp_fonts[JTP_FONT_INTRO].lineheight + jtp_fonts[JTP_FONT_INTRO].baseline,
                   JTP_FONT_INTRO, JTP_COLOR_INTRO_TEXT,
                   tempbuffer,
                   jtp_screen.vpage);
      jtp_list_advance(tempwindow->menu->items);             
    }               
  }
  
  jtp_refresh(&jtp_screen);
  while (jtp_getch() == 0 && jtp_readmouse() == JTP_MBUTTON_NONE)
    ;
  jtp_fade_out(0.5);  

  /* Restore the regular game palette */  
  memcpy(jtp_colors, jtp_game_colors, sizeof(jtp_colors));
  jtp_clear_screen();
  jtp_refresh(&jtp_screen);
  jtp_updatepal(0, 255);    
  jtp_game_palette_set = 1;
}


static void jtp_init_monster_tile_table(void)
{
  int i, k;
  int * monclasstiles;

  jtp_montiles = (int *)malloc(NUMMONS*sizeof(int));
  monclasstiles = (int *)malloc(MAXMCLASSES*sizeof(int));
  if ((!jtp_montiles) || (!monclasstiles))
  {
    OOM(1);
  }

  /* Initialize class default tiles */
  for (i = 0; i < MAXMCLASSES; i++)
    monclasstiles[i] = JTP_TILE_KNIGHT;
  monclasstiles[S_DOG] = JTP_TILE_WOLF;
  monclasstiles[S_FELINE] = JTP_TILE_LARGE_CAT;
  monclasstiles[S_BLOB] = JTP_TILE_GREEN_SLIME;
  monclasstiles[S_JELLY] = JTP_TILE_GREEN_SLIME;
  monclasstiles[S_PUDDING] = JTP_TILE_GREEN_SLIME;
  monclasstiles[S_FUNGUS] = JTP_TILE_YELLOW_MOLD;
  monclasstiles[S_SNAKE] = JTP_TILE_WATER_MOCCASIN;
  monclasstiles[S_LIZARD] = JTP_TILE_LIZARD;
  monclasstiles[S_SPIDER] = JTP_TILE_GIANT_SPIDER;
  monclasstiles[S_XAN] = JTP_TILE_XAN;
  monclasstiles[S_RUSTMONST] = JTP_TILE_RUST_MONSTER;
  monclasstiles[S_KOBOLD] = JTP_TILE_KOBOLD;
  monclasstiles[S_GREMLIN] = JTP_TILE_GREMLIN;
  monclasstiles[S_HUMANOID] = JTP_TILE_GOBLIN;
  monclasstiles[S_EYE] = JTP_TILE_EYE;
  monclasstiles[S_COCKATRICE] = JTP_TILE_EYE;
  monclasstiles[S_JABBERWOCK] = JTP_TILE_EYE;
  monclasstiles[S_GNOME] = JTP_TILE_GNOME;
  monclasstiles[S_LEPRECHAUN] = JTP_TILE_GNOME;
  monclasstiles[S_ELEMENTAL] = JTP_TILE_FIRE_ELEMENTAL;
  monclasstiles[S_OGRE] = JTP_TILE_OGRE;
  monclasstiles[S_GIANT] = JTP_TILE_GIANT;
  monclasstiles[S_NAGA] = JTP_TILE_GUARDIAN_NAGA;
  monclasstiles[S_DRAGON] = JTP_TILE_BLACK_DRAGON;
  monclasstiles[S_WRAITH] = JTP_TILE_WRAITH;
  monclasstiles[S_GHOST] = JTP_TILE_GHOST;
  monclasstiles[S_ZOMBIE] = JTP_TILE_HUMAN_ZOMBIE;
  monclasstiles[S_GOLEM] = JTP_TILE_STONE_GOLEM;
  monclasstiles[S_RODENT] = JTP_TILE_RAT;
  monclasstiles[S_TROLL] = JTP_TILE_TROLL;
  monclasstiles[S_BAT] = JTP_TILE_GIANT_BAT;
  monclasstiles[S_QUADRUPED] = JTP_TILE_ROTHE;
  monclasstiles[S_UNICORN] = JTP_TILE_GRAY_UNICORN;
  monclasstiles[S_CENTAUR] = JTP_TILE_MOUNTAIN_CENTAUR;
  monclasstiles[S_ANT] = JTP_TILE_SOLDIER_ANT;
  monclasstiles[S_MIMIC] = JTP_TILE_LARGE_MIMIC;

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
  jtp_montiles[PM_HOBBIT] = JTP_TILE_HOBBIT;
  jtp_montiles[PM_DWARF] = JTP_TILE_DWARF;
  jtp_montiles[PM_DWARF_LORD] = JTP_TILE_DWARF_LORD;
  jtp_montiles[PM_DWARF_KING] = JTP_TILE_DWARF_KING;
  jtp_montiles[PM_GNOME] = JTP_TILE_GNOME;
  jtp_montiles[PM_GNOME_LORD] = JTP_TILE_GNOME_LORD;
  jtp_montiles[PM_GNOME_KING] = JTP_TILE_GNOME_KING;
  jtp_montiles[PM_GNOMISH_WIZARD] = JTP_TILE_GNOMISH_WIZARD;
  jtp_montiles[PM_ELF] = JTP_TILE_ELF;
  jtp_montiles[PM_WOODLAND_ELF] = JTP_TILE_WOODLAND_ELF;
  jtp_montiles[PM_GREEN_ELF] = JTP_TILE_GREEN_ELF;
  jtp_montiles[PM_GREY_ELF] = JTP_TILE_GREY_ELF;
  jtp_montiles[PM_ELF_LORD] = JTP_TILE_ELF_LORD;
  jtp_montiles[PM_ELVENKING] = JTP_TILE_ELVENKING;
  jtp_montiles[PM_BUGBEAR] = JTP_TILE_BUGBEAR;
  jtp_montiles[PM_GOBLIN] = JTP_TILE_GOBLIN;
  jtp_montiles[PM_HOBGOBLIN] = JTP_TILE_HOBGOBLIN;
  jtp_montiles[PM_ORC] = JTP_TILE_ORC;
  jtp_montiles[PM_HILL_ORC] = JTP_TILE_HILL_ORC;
  jtp_montiles[PM_MORDOR_ORC] = JTP_TILE_MORDOR_ORC;
  jtp_montiles[PM_URUK_HAI] = JTP_TILE_URUK_HAI;
  jtp_montiles[PM_ORC_SHAMAN] = JTP_TILE_URUK_HAI;
  jtp_montiles[PM_ORC_CAPTAIN] = JTP_TILE_URUK_HAI;
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
  jtp_montiles[PM_WOOD_NYMPH] = JTP_TILE_WOOD_NYMPH;
  jtp_montiles[PM_WATER_NYMPH] = JTP_TILE_WATER_NYMPH;
  jtp_montiles[PM_MOUNTAIN_NYMPH] = JTP_TILE_MOUNTAIN_NYMPH;
  jtp_montiles[PM_SUCCUBUS] = JTP_TILE_SUCCUBUS;
  jtp_montiles[PM_SKELETON] = JTP_TILE_SKELETON;
  jtp_montiles[PM_KILLER_BEE] = JTP_TILE_KILLER_BEE;
  jtp_montiles[PM_QUEEN_BEE] = JTP_TILE_QUEEN_BEE;
  jtp_montiles[PM_GIANT_BEETLE] = JTP_TILE_GIANT_BEETLE;
  jtp_montiles[PM_CENTIPEDE] = JTP_TILE_CENTIPEDE;
  jtp_montiles[PM_SCORPION] = JTP_TILE_SCORPION;
  jtp_montiles[PM_CAVE_SPIDER] = JTP_TILE_CAVE_SPIDER;
  jtp_montiles[PM_GIANT_SPIDER] = JTP_TILE_GIANT_SPIDER;
  jtp_montiles[PM_TOURIST] = JTP_TILE_TOURIST;
  jtp_montiles[PM_YELLOW_LIGHT] = JTP_TILE_YELLOW_LIGHT;
  jtp_montiles[PM_BLACK_LIGHT] = JTP_TILE_BLACK_LIGHT;
  jtp_montiles[PM_YELLOW_MOLD] = JTP_TILE_YELLOW_MOLD;
  jtp_montiles[PM_RED_MOLD] = JTP_TILE_RED_MOLD;
  jtp_montiles[PM_VIOLET_FUNGUS] = JTP_TILE_VIOLET_FUNGUS;
  jtp_montiles[PM_BROWN_MOLD] = JTP_TILE_BROWN_MOLD;
  jtp_montiles[PM_SHRIEKER] = JTP_TILE_SHRIEKER;
  jtp_montiles[PM_COCKATRICE] = JTP_TILE_COCKATRICE;
  jtp_montiles[PM_KRAKEN] = JTP_TILE_KRAKEN;
  jtp_montiles[PM_GIANT_EEL] = JTP_TILE_GIANT_EEL;
  jtp_montiles[PM_SHARK] = JTP_TILE_SHARK;
  jtp_montiles[PM_ACID_BLOB] = JTP_TILE_ACID_BLOB;
  jtp_montiles[PM_QUIVERING_BLOB] = JTP_TILE_QUIVERING_BLOB;
  jtp_montiles[PM_GELATINOUS_CUBE] = JTP_TILE_GELATINOUS_CUBE;
  jtp_montiles[PM_LICHEN] = JTP_TILE_LICHEN;
  jtp_montiles[PM_GREEN_MOLD] = JTP_TILE_GREEN_MOLD;
  jtp_montiles[PM_STALKER] = JTP_TILE_STALKER;
  jtp_montiles[PM_AIR_ELEMENTAL] = JTP_TILE_AIR_ELEMENTAL;
  jtp_montiles[PM_EARTH_ELEMENTAL] = JTP_TILE_EARTH_ELEMENTAL;
  jtp_montiles[PM_WATER_ELEMENTAL] = JTP_TILE_WATER_ELEMENTAL;
  jtp_montiles[PM_CHICKATRICE] = JTP_TILE_CHICKATRICE;
  jtp_montiles[PM_PYROLISK] = JTP_TILE_PYROLISK;
  jtp_montiles[PM_SMALL_MIMIC] = JTP_TILE_SMALL_MIMIC;
  jtp_montiles[PM_LARGE_MIMIC] = JTP_TILE_LARGE_MIMIC;
  jtp_montiles[PM_GIANT_MIMIC] = JTP_TILE_GIANT_MIMIC;
  jtp_montiles[PM_FOX] = JTP_TILE_FOX;
  jtp_montiles[PM_COYOTE] = JTP_TILE_COYOTE;
  jtp_montiles[PM_JACKAL] = JTP_TILE_JACKAL;
  jtp_montiles[PM_LITTLE_DOG] = JTP_TILE_LITTLE_DOG;
  jtp_montiles[PM_DOG] = JTP_TILE_DOG;
  jtp_montiles[PM_LARGE_DOG] = JTP_TILE_LARGE_DOG;
  jtp_montiles[PM_ROCK_PIERCER] = JTP_TILE_ROCK_PIERCER;
  jtp_montiles[PM_IRON_PIERCER] = JTP_TILE_IRON_PIERCER;
  jtp_montiles[PM_GLASS_PIERCER] = JTP_TILE_GLASS_PIERCER;
  jtp_montiles[PM_KITTEN] = JTP_TILE_KITTEN;
  jtp_montiles[PM_WATER_DEMON] = JTP_TILE_WATER_DEMON;
  jtp_montiles[PM_GECKO] = JTP_TILE_GECKO;
  jtp_montiles[PM_NEWT] = JTP_TILE_NEWT;
  jtp_montiles[PM_SHOPKEEPER] = JTP_TILE_SHOPKEEPER;
  jtp_montiles[PM_GAS_SPORE] = JTP_TILE_GAS_SPORE;
  jtp_montiles[PM_HOUSECAT] = JTP_TILE_HOUSECAT;
  jtp_montiles[PM_LARGE_CAT] = JTP_TILE_LARGE_CAT;
  jtp_montiles[PM_FREEZING_SPHERE] = JTP_TILE_FREEZING_SPHERE;
  jtp_montiles[PM_FLAMING_SPHERE] = JTP_TILE_FLAMING_SPHERE;
  jtp_montiles[PM_SHOCKING_SPHERE] = JTP_TILE_SHOCKING_SPHERE;
  jtp_montiles[PM_PONY] = JTP_TILE_PONY;
  jtp_montiles[PM_HORSE] = JTP_TILE_HORSE;
  jtp_montiles[PM_WARHORSE] = JTP_TILE_WARHORSE;
  jtp_montiles[PM_ROTHE] = JTP_TILE_ROTHE;
  jtp_montiles[PM_WATER_MOCCASIN] = JTP_TILE_WATER_MOCCASIN;
  jtp_montiles[PM_MANES] = JTP_TILE_MANES;
  jtp_montiles[PM_BLACK_UNICORN] = JTP_TILE_BLACK_UNICORN;
  jtp_montiles[PM_GRAY_UNICORN] = JTP_TILE_GRAY_UNICORN;
  jtp_montiles[PM_WHITE_UNICORN] = JTP_TILE_WHITE_UNICORN;
  jtp_montiles[PM_GIANT_ANT] = JTP_TILE_GIANT_ANT;
  jtp_montiles[PM_SOLDIER_ANT] = JTP_TILE_SOLDIER_ANT;
  jtp_montiles[PM_FIRE_ANT] = JTP_TILE_FIRE_ANT;
  jtp_montiles[PM_ROCK_MOLE] = JTP_TILE_ROCK_MOLE;
  jtp_montiles[PM_KOBOLD] = JTP_TILE_KOBOLD;
  jtp_montiles[PM_LARGE_KOBOLD] = JTP_TILE_LARGE_KOBOLD;
  jtp_montiles[PM_KOBOLD_LORD] = JTP_TILE_KOBOLD_LORD;
  jtp_montiles[PM_KOBOLD_SHAMAN] = JTP_TILE_KOBOLD_SHAMAN;
  jtp_montiles[PM_ORC_CAPTAIN] = JTP_TILE_ORC_CAPTAIN;
  jtp_montiles[PM_ORC_SHAMAN] = JTP_TILE_ORC_SHAMAN;
  jtp_montiles[PM_LEPRECHAUN] = JTP_TILE_LEPRECHAUN;
  jtp_montiles[PM_GARTER_SNAKE] = JTP_TILE_GARTER_SNAKE;
  jtp_montiles[PM_SNAKE] = JTP_TILE_SNAKE;
  jtp_montiles[PM_PIT_VIPER] = JTP_TILE_PIT_VIPER;
  jtp_montiles[PM_PYTHON] = JTP_TILE_PYTHON;
  jtp_montiles[PM_COBRA] = JTP_TILE_COBRA;
  jtp_montiles[PM_GIANT] = JTP_TILE_GIANT;
  jtp_montiles[PM_ETTIN] = JTP_TILE_ETTIN;
  jtp_montiles[PM_KOBOLD_ZOMBIE] = JTP_TILE_KOBOLD_ZOMBIE;
  jtp_montiles[PM_GNOME_ZOMBIE] = JTP_TILE_GNOME_ZOMBIE;
  jtp_montiles[PM_DWARF_ZOMBIE] = JTP_TILE_DWARF_ZOMBIE;
  jtp_montiles[PM_ORC_ZOMBIE] = JTP_TILE_ORC_ZOMBIE;
  jtp_montiles[PM_ELF_ZOMBIE] = JTP_TILE_ELF_ZOMBIE;
  jtp_montiles[PM_GIANT_ZOMBIE] = JTP_TILE_GIANT_ZOMBIE;
  jtp_montiles[PM_ETTIN_ZOMBIE] = JTP_TILE_ETTIN_ZOMBIE;
  jtp_montiles[PM_KOBOLD_MUMMY] = JTP_TILE_KOBOLD_MUMMY;
  jtp_montiles[PM_GNOME_MUMMY] = JTP_TILE_GNOME_MUMMY;
  jtp_montiles[PM_DWARF_MUMMY] = JTP_TILE_DWARF_MUMMY;
  jtp_montiles[PM_ORC_MUMMY] = JTP_TILE_ORC_MUMMY;
  jtp_montiles[PM_HUMAN_MUMMY] = JTP_TILE_HUMAN_MUMMY;
  jtp_montiles[PM_ELF_MUMMY] = JTP_TILE_ELF_MUMMY;
  jtp_montiles[PM_GIANT_MUMMY] = JTP_TILE_GIANT_MUMMY;
  jtp_montiles[PM_ETTIN_MUMMY] = JTP_TILE_ETTIN_MUMMY;
  jtp_montiles[PM_HOMUNCULUS] = JTP_TILE_HOMUNCULUS;
  jtp_montiles[PM_IMP] = JTP_TILE_IMP;
  jtp_montiles[PM_LEMURE] = JTP_TILE_LEMURE;
  jtp_montiles[PM_QUASIT] = JTP_TILE_QUASIT;
  jtp_montiles[PM_TENGU] = JTP_TILE_TENGU;
  jtp_montiles[PM_WUMPUS] = JTP_TILE_WUMPUS;
  jtp_montiles[PM_ORACLE] = JTP_TILE_ORACLE;
  jtp_montiles[PM_ALIGNED_PRIEST] = JTP_TILE_NEUTRAL_PRIEST;
  jtp_montiles[PM_WATCHMAN] = JTP_TILE_WATCHMAN;
  jtp_montiles[PM_WATCH_CAPTAIN] = JTP_TILE_WATCH_CAPTAIN;
  jtp_montiles[PM_SOLDIER] = JTP_TILE_SOLDIER;
  jtp_montiles[PM_SERGEANT] = JTP_TILE_SERGEANT;
  jtp_montiles[PM_LIEUTENANT] = JTP_TILE_LIEUTENANT;
  jtp_montiles[PM_CAPTAIN] = JTP_TILE_CAPTAIN;
  jtp_montiles[PM_GRID_BUG] = JTP_TILE_GRID_BUG;
  jtp_montiles[PM_XAN] = JTP_TILE_XAN;
  jtp_montiles[PM_GARGOYLE] = JTP_TILE_GARGOYLE;
  jtp_montiles[PM_WINGED_GARGOYLE] = JTP_TILE_WINGED_GARGOYLE;
  jtp_montiles[PM_DINGO] = JTP_TILE_DINGO;
  jtp_montiles[PM_WOLF] = JTP_TILE_WOLF;
  jtp_montiles[PM_WEREWOLF] = JTP_TILE_WEREWOLF;
  jtp_montiles[PM_WARG] = JTP_TILE_WARG;
  jtp_montiles[PM_WINTER_WOLF_CUB] = JTP_TILE_WINTER_WOLF_CUB;
  jtp_montiles[PM_WINTER_WOLF] = JTP_TILE_WINTER_WOLF;
  jtp_montiles[PM_HELL_HOUND_PUP] = JTP_TILE_HELL_HOUND_PUP;
  jtp_montiles[PM_HELL_HOUND] = JTP_TILE_HELL_HOUND;
  jtp_montiles[PM_JAGUAR] = JTP_TILE_JAGUAR;
  jtp_montiles[PM_LYNX] = JTP_TILE_LYNX;
  jtp_montiles[PM_PANTHER] = JTP_TILE_PANTHER;
  jtp_montiles[PM_TIGER] = JTP_TILE_TIGER;
  jtp_montiles[PM_MIND_FLAYER] = JTP_TILE_MIND_FLAYER;
  jtp_montiles[PM_MASTER_MIND_FLAYER] = JTP_TILE_MASTER_MIND_FLAYER;
  jtp_montiles[PM_LICH] = JTP_TILE_LICH;
  jtp_montiles[PM_DEMILICH] = JTP_TILE_DEMILICH;
  jtp_montiles[PM_MASTER_LICH] = JTP_TILE_MASTER_LICH;
  jtp_montiles[PM_ARCH_LICH] = JTP_TILE_ARCH_LICH;
  jtp_montiles[PM_SUCCUBUS] = JTP_TILE_SUCCUBUS;
  jtp_montiles[PM_INCUBUS] = JTP_TILE_INCUBUS;
  jtp_montiles[PM_GUARDIAN_NAGA] = JTP_TILE_GUARDIAN_NAGA;
  jtp_montiles[PM_GUARDIAN_NAGA_HATCHLING] = JTP_TILE_GUARDIAN_NAGA_HATCHLING;
  jtp_montiles[PM_GOLDEN_NAGA] = JTP_TILE_GOLDEN_NAGA;
  jtp_montiles[PM_GOLDEN_NAGA_HATCHLING] = JTP_TILE_GOLDEN_NAGA_HATCHLING;
  jtp_montiles[PM_RED_NAGA] = JTP_TILE_RED_NAGA;
  jtp_montiles[PM_RED_NAGA_HATCHLING] = JTP_TILE_RED_NAGA_HATCHLING;
  jtp_montiles[PM_BLACK_NAGA] = JTP_TILE_BLACK_NAGA;
  jtp_montiles[PM_BLACK_NAGA_HATCHLING] = JTP_TILE_BLACK_NAGA_HATCHLING;
  jtp_montiles[PM_STONE_GIANT] = JTP_TILE_STONE_GIANT;
  jtp_montiles[PM_HILL_GIANT] = JTP_TILE_HILL_GIANT;
  jtp_montiles[PM_FROST_GIANT] = JTP_TILE_FROST_GIANT;
  jtp_montiles[PM_FIRE_GIANT] = JTP_TILE_FIRE_GIANT;
  jtp_montiles[PM_STORM_GIANT] = JTP_TILE_STORM_GIANT;
  jtp_montiles[PM_TITAN] = JTP_TILE_TITAN;
  jtp_montiles[PM_MINOTAUR] = JTP_TILE_MINOTAUR;
  jtp_montiles[PM_OGRE] = JTP_TILE_OGRE;
  jtp_montiles[PM_OGRE_LORD] = JTP_TILE_OGRE_LORD;
  jtp_montiles[PM_OGRE_KING] = JTP_TILE_OGRE_KING;
  jtp_montiles[PM_RED_DRAGON] = JTP_TILE_RED_DRAGON;
  jtp_montiles[PM_BABY_RED_DRAGON] = JTP_TILE_BABY_RED_DRAGON;
  jtp_montiles[PM_GREEN_DRAGON] = JTP_TILE_GREEN_DRAGON;
  jtp_montiles[PM_BABY_GREEN_DRAGON] = JTP_TILE_BABY_GREEN_DRAGON;
  jtp_montiles[PM_ORANGE_DRAGON] = JTP_TILE_ORANGE_DRAGON;
  jtp_montiles[PM_BABY_ORANGE_DRAGON] = JTP_TILE_BABY_ORANGE_DRAGON;
  jtp_montiles[PM_YELLOW_DRAGON] = JTP_TILE_YELLOW_DRAGON;
  jtp_montiles[PM_BABY_YELLOW_DRAGON] = JTP_TILE_BABY_YELLOW_DRAGON;
  jtp_montiles[PM_BLUE_DRAGON] = JTP_TILE_BLUE_DRAGON;
  jtp_montiles[PM_BABY_BLUE_DRAGON] = JTP_TILE_BABY_BLUE_DRAGON;
  jtp_montiles[PM_GRAY_DRAGON] = JTP_TILE_GRAY_DRAGON;
  jtp_montiles[PM_BABY_GRAY_DRAGON] = JTP_TILE_BABY_GRAY_DRAGON;
  jtp_montiles[PM_SILVER_DRAGON] = JTP_TILE_SILVER_DRAGON;
  jtp_montiles[PM_BABY_SILVER_DRAGON] = JTP_TILE_BABY_SILVER_DRAGON;
  jtp_montiles[PM_BLACK_DRAGON] = JTP_TILE_BLACK_DRAGON;
  jtp_montiles[PM_BABY_BLACK_DRAGON] = JTP_TILE_BABY_BLACK_DRAGON;
  jtp_montiles[PM_WHITE_DRAGON] = JTP_TILE_WHITE_DRAGON;
  jtp_montiles[PM_BABY_WHITE_DRAGON] = JTP_TILE_BABY_WHITE_DRAGON;
  jtp_montiles[PM_GREMLIN] = JTP_TILE_GREMLIN;
  jtp_montiles[PM_RUST_MONSTER] = JTP_TILE_RUST_MONSTER;
  jtp_montiles[PM_DISENCHANTER] = JTP_TILE_DISENCHANTER;
  jtp_montiles[PM_BABY_PURPLE_WORM] = JTP_TILE_BABY_PURPLE_WORM;
  jtp_montiles[PM_PURPLE_WORM] = JTP_TILE_PURPLE_WORM;
  jtp_montiles[PM_BABY_LONG_WORM] = JTP_TILE_BABY_LONG_WORM;
  jtp_montiles[PM_LONG_WORM] = JTP_TILE_LONG_WORM;
  jtp_montiles[PM_LONG_WORM_TAIL] = JTP_TILE_LONG_WORM_TAIL;
  jtp_montiles[PM_MONKEY] = JTP_TILE_MONKEY;
  jtp_montiles[PM_APE] = JTP_TILE_APE;
  jtp_montiles[PM_OWLBEAR] = JTP_TILE_OWLBEAR;
  jtp_montiles[PM_YETI] = JTP_TILE_YETI;
  jtp_montiles[PM_CARNIVOROUS_APE] = JTP_TILE_CARNIVOROUS_APE;
  jtp_montiles[PM_SASQUATCH] = JTP_TILE_SASQUATCH;
  jtp_montiles[PM_MUMAK] = JTP_TILE_MUMAK;
  jtp_montiles[PM_LEOCROTTA] = JTP_TILE_LEOCROTTA;
  jtp_montiles[PM_TITANOTHERE] = JTP_TILE_TITANOTHERE;
  jtp_montiles[PM_BALUCHITHERIUM] = JTP_TILE_BALUCHITHERIUM;
  jtp_montiles[PM_MASTODON] = JTP_TILE_MASTADON;
  jtp_montiles[PM_ZRUTY] = JTP_TILE_ZRUTY;
  jtp_montiles[PM_WIZARD_OF_YENDOR] = JTP_TILE_WIZARD_OF_YENDOR;
  jtp_montiles[PM_XORN] = JTP_TILE_XORN;
  jtp_montiles[PM_STRAW_GOLEM] = JTP_TILE_STRAW_GOLEM;
  jtp_montiles[PM_PAPER_GOLEM] = JTP_TILE_PAPER_GOLEM;
  jtp_montiles[PM_ROPE_GOLEM] = JTP_TILE_ROPE_GOLEM;
  jtp_montiles[PM_GOLD_GOLEM] = JTP_TILE_GOLD_GOLEM;
  jtp_montiles[PM_LEATHER_GOLEM] = JTP_TILE_LEATHER_GOLEM;
  jtp_montiles[PM_WOOD_GOLEM] = JTP_TILE_WOOD_GOLEM;
  jtp_montiles[PM_FLESH_GOLEM] = JTP_TILE_FLESH_GOLEM;
  jtp_montiles[PM_CLAY_GOLEM] = JTP_TILE_CLAY_GOLEM;
  jtp_montiles[PM_STONE_GOLEM] = JTP_TILE_STONE_GOLEM;
  jtp_montiles[PM_GLASS_GOLEM] = JTP_TILE_GLASS_GOLEM;
  jtp_montiles[PM_IRON_GOLEM] = JTP_TILE_IRON_GOLEM;
  jtp_montiles[PM_BAT] = JTP_TILE_BAT;
  jtp_montiles[PM_GIANT_BAT] = JTP_TILE_GIANT_BAT;
  jtp_montiles[PM_RAVEN] = JTP_TILE_RAVEN;
  jtp_montiles[PM_VAMPIRE_BAT] = JTP_TILE_VAMPIRE_BAT;
  jtp_montiles[PM_QUANTUM_MECHANIC] = JTP_TILE_QUANTUM_MECHANIC;
  jtp_montiles[PM_DJINNI] = JTP_TILE_DJINNI;
  jtp_montiles[PM_LURKER_ABOVE] = JTP_TILE_LURKER_ABOVE;
  jtp_montiles[PM_TRAPPER] = JTP_TILE_TRAPPER;

  /* Clean up */
  free(monclasstiles);
}



static void jtp_init_engulf_tile_table(void)
{
    int i;
    jtp_engulfmap = (int *)malloc(NUMMONS*sizeof(int));
    
    for (i = 0; i < NUMMONS; i++)
        jtp_engulfmap[i] = JTP_TILE_NONE;
        
    jtp_engulfmap[PM_OCHRE_JELLY]   = JTP_TILE_ENGULF_OCHRE_JELLY;
    jtp_engulfmap[PM_LURKER_ABOVE]  = JTP_TILE_ENGULF_LURKER_ABOVE;
    jtp_engulfmap[PM_TRAPPER]       = JTP_TILE_ENGULF_TRAPPER;
    jtp_engulfmap[PM_PURPLE_WORM]   = JTP_TILE_ENGULF_PURPLE_WORM;
    jtp_engulfmap[PM_DUST_VORTEX]   = JTP_TILE_ENGULF_DUST_VORTEX;
    jtp_engulfmap[PM_ICE_VORTEX]    = JTP_TILE_ENGULF_ICE_VORTEX;
    jtp_engulfmap[PM_ENERGY_VORTEX] = JTP_TILE_ENGULF_ENERGY_VORTEX;
    jtp_engulfmap[PM_STEAM_VORTEX]  = JTP_TILE_ENGULF_STEAM_VORTEX;
    jtp_engulfmap[PM_FIRE_VORTEX]   = JTP_TILE_ENGULF_FIRE_VORTEX;
    jtp_engulfmap[PM_FOG_CLOUD]     = JTP_TILE_ENGULF_FOG_CLOUD;
    jtp_engulfmap[PM_AIR_ELEMENTAL] = JTP_TILE_ENGULF_AIR_ELEMENTAL;
    jtp_engulfmap[PM_JUIBLEX]      = JTP_TILE_ENGULF_JUIBLEX;
}


static void jtp_init_object_tile_table(void)
{
  int i, k;
  int * objclasstiles;
  const char * temp_descr;

  jtp_objtiles = (int *)malloc(NUM_OBJECTS*sizeof(int));
  objclasstiles = (int *)malloc(MAXOCLASSES*sizeof(int));
  if ((!jtp_objtiles) || (!objclasstiles))
  {
    OOM(1);
  }

  /* Initialize class default tiles */
#ifdef SANITY_CHECKS
  for (i = 0; i < MAXOCLASSES; i++)
    objclasstiles[i] = JTP_TILE_NONE;
#else
  for (i = 0; i < MAXOCLASSES; i++)
    objclasstiles[i] = JTP_TILE_DEFAULT_OBJECT;
#endif
  objclasstiles[ROCK_CLASS] = JTP_TILE_BOULDER;
  objclasstiles[COIN_CLASS] = JTP_TILE_COINS;
  objclasstiles[SPBOOK_CLASS] = JTP_TILE_BOOK;
  objclasstiles[SCROLL_CLASS] = JTP_TILE_SCROLL;
  objclasstiles[POTION_CLASS] = JTP_TILE_WATER;
  objclasstiles[WEAPON_CLASS] = JTP_TILE_SPEAR;
  objclasstiles[ARMOR_CLASS] = JTP_TILE_HELMET;
  objclasstiles[WAND_CLASS] = JTP_TILE_WAND;
  objclasstiles[GEM_CLASS] = JTP_TILE_BLUE_GLASS;
  objclasstiles[RING_CLASS] = JTP_TILE_RING;
  objclasstiles[AMULET_CLASS] = JTP_TILE_AMULET;
  objclasstiles[FOOD_CLASS] = JTP_TILE_FOOD_RATION;

#ifdef SANITY_CHECKS
  /* start with 1, because 0 is random class */
  for (i = 1; i < MAXOCLASSES; i++)
  {
    if (objclasstiles[i] == JTP_TILE_NONE)
    {
      objclasstiles[i] = JTP_TILE_DEFAULT_OBJECT;
      fprintf(stderr, "object class %d (%c) has no tile assigned\n",
        i,
        def_oc_syms[i]);
    }
  }
  /*
   * current reports:
   * 1 (ILLOBJS_CLASS)
   * 6 (TOOL_CLASS)
   * 15 (BALL_CLASS)
   * 16 (CHAIN_CLASS)
   * 17 (VENOM_CLASS)
   */
#endif

  /* Assign class based object tiles first */
  for (i = 0; i < NUM_OBJECTS; i++)
  {
    /* If the object class is recognizable, use that */
    k = objects[i].oc_class;
    if (k >= 0 && k < MAXOCLASSES)
      jtp_objtiles[i] = objclasstiles[k];
    else
#ifdef SANITY_CHECKS
      jtp_objtiles[i] = JTP_TILE_NONE;
#else
      jtp_objtiles[i] = JTP_TILE_DEFAULT_OBJECT;
#endif
  }

  /* Assign gems by color */
  for (i = 0; i < NUM_OBJECTS; i++)
  {
    if (objects[i].oc_class == GEM_CLASS)
    {
      switch (objects[i].oc_color)
      {
        case CLR_WHITE:  jtp_objtiles[i] = JTP_TILE_WHITE_GLASS; break;
        case CLR_BLUE:   jtp_objtiles[i] = JTP_TILE_BLUE_GLASS;  break;
        case CLR_RED:    jtp_objtiles[i] = JTP_TILE_RED_GLASS;   break;
        case CLR_BROWN:  jtp_objtiles[i] = JTP_TILE_BROWN_GLASS; break;
        case CLR_ORANGE: jtp_objtiles[i] = JTP_TILE_ORANGE_GLASS;break;
        case CLR_YELLOW: jtp_objtiles[i] = JTP_TILE_YELLOW_GLASS;break;
        case CLR_BLACK:  jtp_objtiles[i] = JTP_TILE_BLACK_GLASS; break;
        case CLR_GREEN:  jtp_objtiles[i] = JTP_TILE_GREEN_GLASS; break;
        case CLR_MAGENTA:jtp_objtiles[i] = JTP_TILE_VIOLET_GLASS;break;
      }
    }
  }

  /* Assign potions by color */
  for (i = 0; i < NUM_OBJECTS; i++)
    if (objects[i].oc_class == POTION_CLASS)
    {
      temp_descr = OBJ_DESCR(objects[i]);
      if (temp_descr)
      {
        if (strstr(temp_descr, "ruby")) jtp_objtiles[i] = JTP_TILE_RUBY_POTION;
        else if (strstr(temp_descr, "pink")) jtp_objtiles[i] = JTP_TILE_PINK_POTION;
        else if (strstr(temp_descr, "orange")) jtp_objtiles[i] = JTP_TILE_ORANGE_POTION;
        else if (strstr(temp_descr, "yellow")) jtp_objtiles[i] = JTP_TILE_YELLOW_POTION;
        else if (strstr(temp_descr, "emerald")) jtp_objtiles[i] = JTP_TILE_EMERALD_POTION;
        else if (strstr(temp_descr, "dark green")) jtp_objtiles[i] = JTP_TILE_DARK_GREEN_POTION;
        else if (strstr(temp_descr, "cyan")) jtp_objtiles[i] = JTP_TILE_CYAN_POTION;
        else if (strstr(temp_descr, "sky blue")) jtp_objtiles[i] = JTP_TILE_SKY_BLUE_POTION;
        else if (strstr(temp_descr, "brilliant blue")) jtp_objtiles[i] = JTP_TILE_BRILLIANT_BLUE_POTION;
        else if (strstr(temp_descr, "magenta")) jtp_objtiles[i] = JTP_TILE_MAGENTA_POTION;
        else if (strstr(temp_descr, "purple-red")) jtp_objtiles[i] = JTP_TILE_PURPLE_RED_POTION;
        else if (strstr(temp_descr, "puce")) jtp_objtiles[i] = JTP_TILE_PUCE_POTION;
        else if (strstr(temp_descr, "milky")) jtp_objtiles[i] = JTP_TILE_MILKY_POTION;
        else if (strstr(temp_descr, "swirly")) jtp_objtiles[i] = JTP_TILE_SWIRLY_POTION;
        else if (strstr(temp_descr, "bubbly")) jtp_objtiles[i] = JTP_TILE_BUBBLY_POTION;
        else if (strstr(temp_descr, "smoky")) jtp_objtiles[i] = JTP_TILE_SMOKY_POTION;
        else if (strstr(temp_descr, "cloudy")) jtp_objtiles[i] = JTP_TILE_CLOUDY_POTION;
        else if (strstr(temp_descr, "effervescent")) jtp_objtiles[i] = JTP_TILE_EFFERVESCENT_POTION;
        else if (strstr(temp_descr, "black")) jtp_objtiles[i] = JTP_TILE_BLACK_POTION;
        else if (strstr(temp_descr, "golden")) jtp_objtiles[i] = JTP_TILE_GOLDEN_POTION;
        else if (strstr(temp_descr, "brown")) jtp_objtiles[i] = JTP_TILE_BROWN_POTION;
        else if (strstr(temp_descr, "fizzy")) jtp_objtiles[i] = JTP_TILE_FIZZY_POTION;
        else if (strstr(temp_descr, "dark")) jtp_objtiles[i] = JTP_TILE_DARK_POTION;
        else if (strstr(temp_descr, "white")) jtp_objtiles[i] = JTP_TILE_WHITE_POTION;
        else if (strstr(temp_descr, "murky")) jtp_objtiles[i] = JTP_TILE_MURKY_POTION;
        else if (strstr(temp_descr, "water")) jtp_objtiles[i] = JTP_TILE_WATER;
        else if (strstr(temp_descr, "clear potion")) jtp_objtiles[i] = JTP_TILE_WATER;
        else if (strstr(temp_descr, "holy water")) jtp_objtiles[i] = JTP_TILE_HOLY_WATER;
        else if (strstr(temp_descr, "unholy water")) jtp_objtiles[i] = JTP_TILE_UNHOLY_WATER;
      }
    }

  /* Adjust for individual object types and exceptions */
  jtp_objtiles[STATUE] = JTP_TILE_STATUE;
  jtp_objtiles[LARGE_BOX] = JTP_TILE_LARGE_BOX;
  jtp_objtiles[ICE_BOX] = JTP_TILE_ICE_BOX;
  jtp_objtiles[CHEST] = JTP_TILE_CHEST;

  jtp_objtiles[SMALL_SHIELD] = JTP_TILE_SHIELD;
  jtp_objtiles[ELVEN_SHIELD] = JTP_TILE_SHIELD;
  jtp_objtiles[URUK_HAI_SHIELD] = JTP_TILE_SHIELD;
  jtp_objtiles[ORCISH_SHIELD] = JTP_TILE_SHIELD;
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
#ifdef DARK_ELVEN_MITHRIL_COAT
  jtp_objtiles[DARK_ELVEN_MITHRIL_COAT] = JTP_TILE_CHAIN_MAIL;
#endif
  jtp_objtiles[ELVEN_MITHRIL_COAT] = JTP_TILE_CHAIN_MAIL;
  jtp_objtiles[CHAIN_MAIL] = JTP_TILE_CHAIN_MAIL;
  jtp_objtiles[ORCISH_CHAIN_MAIL] = JTP_TILE_CHAIN_MAIL;
  jtp_objtiles[SCALE_MAIL] = JTP_TILE_SCALE_MAIL;
  jtp_objtiles[RING_MAIL] = JTP_TILE_RING_MAIL;
  jtp_objtiles[ORCISH_RING_MAIL] = JTP_TILE_RING_MAIL;

  jtp_objtiles[LEATHER_GLOVES] = JTP_TILE_GLOVES;
  jtp_objtiles[GAUNTLETS_OF_FUMBLING] = JTP_TILE_GLOVES;
  jtp_objtiles[GAUNTLETS_OF_POWER] = JTP_TILE_GLOVES;
#ifdef GAUNTLETS_OF_SWIMMING
  jtp_objtiles[GAUNTLETS_OF_SWIMMING] = JTP_TILE_GLOVES;
#endif
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
#ifdef ALCHEMY_SMOCK
  jtp_objtiles[ALCHEMY_SMOCK] = JTP_TILE_CLOAK;  /* not in Slash'EM */
#endif
  jtp_objtiles[LEATHER_CLOAK] = JTP_TILE_CLOAK;
  jtp_objtiles[CLOAK_OF_PROTECTION] = JTP_TILE_CLOAK;
  jtp_objtiles[CLOAK_OF_INVISIBILITY] = JTP_TILE_CLOAK;
  jtp_objtiles[CLOAK_OF_MAGIC_RESISTANCE] = JTP_TILE_CLOAK;
  jtp_objtiles[CLOAK_OF_DISPLACEMENT] = JTP_TILE_CLOAK;
#ifdef POISONOUS_CLOAK
  jtp_objtiles[POISONOUS_CLOAK] = JTP_TILE_CLOAK;
#endif

  jtp_objtiles[HAWAIIAN_SHIRT] = JTP_TILE_HAWAIIAN_SHIRT;
  jtp_objtiles[T_SHIRT] = JTP_TILE_HAWAIIAN_SHIRT;

  jtp_objtiles[CORNUTHAUM] = JTP_TILE_CONICAL_HAT;
  jtp_objtiles[DUNCE_CAP] = JTP_TILE_CONICAL_HAT;

  jtp_objtiles[SKELETON_KEY] = JTP_TILE_KEY;
  jtp_objtiles[LOCK_PICK] = JTP_TILE_KEY;

  jtp_objtiles[CLUB] = JTP_TILE_CLUB;
  jtp_objtiles[PICK_AXE] = JTP_TILE_PICKAXE;
  jtp_objtiles[GRAPPLING_HOOK] = JTP_TILE_PICKAXE;

  jtp_objtiles[KELP_FROND] = JTP_TILE_KELP_FROND;
  jtp_objtiles[EUCALYPTUS_LEAF] = JTP_TILE_EUCALYPTUS_LEAF;
  jtp_objtiles[APPLE] = JTP_TILE_APPLE;
  jtp_objtiles[ORANGE] = JTP_TILE_ORANGE;
  jtp_objtiles[PEAR] = JTP_TILE_PEAR;
  jtp_objtiles[MELON] = JTP_TILE_MELON;
  jtp_objtiles[BANANA] = JTP_TILE_BANANA;
  jtp_objtiles[CARROT] = JTP_TILE_CARROT;
  jtp_objtiles[SPRIG_OF_WOLFSBANE] = JTP_TILE_SPRIG_OF_WOLFSBANE;
  jtp_objtiles[CLOVE_OF_GARLIC] = JTP_TILE_CLOVE_OF_GARLIC;
  jtp_objtiles[SLIME_MOLD] = JTP_TILE_SLIME_MOLD;
  jtp_objtiles[LUMP_OF_ROYAL_JELLY] = JTP_TILE_LUMP_OF_ROYAL_JELLY;
  jtp_objtiles[EGG] = JTP_TILE_EGG;

  jtp_objtiles[FOOD_RATION] = JTP_TILE_FOOD_RATION;
  jtp_objtiles[CRAM_RATION] = JTP_TILE_CRAM_RATION;
  jtp_objtiles[TRIPE_RATION] = JTP_TILE_TRIPE_RATION;
  jtp_objtiles[K_RATION] = JTP_TILE_K_RATION;
  jtp_objtiles[C_RATION] = JTP_TILE_C_RATION;
  jtp_objtiles[MEATBALL] = JTP_TILE_MEATBALL;
  jtp_objtiles[MEAT_STICK] = JTP_TILE_MEAT_STICK;
  jtp_objtiles[HUGE_CHUNK_OF_MEAT] = JTP_TILE_HUGE_CHUNK_OF_MEAT;
  jtp_objtiles[MEAT_RING] = JTP_TILE_MEAT_RING;
  jtp_objtiles[CREAM_PIE] = JTP_TILE_CREAM_PIE;
  jtp_objtiles[CANDY_BAR] = JTP_TILE_CANDY_BAR;
  jtp_objtiles[FORTUNE_COOKIE] = JTP_TILE_FORTUNE_COOKIE;
  jtp_objtiles[PANCAKE] = JTP_TILE_PANCAKE;
  jtp_objtiles[LEMBAS_WAFER] = JTP_TILE_LEMBAS_WAFER;
  jtp_objtiles[TIN] = JTP_TILE_TIN;
  jtp_objtiles[TINNING_KIT] = JTP_TILE_TINNING_KIT;
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
  jtp_objtiles[TOOLED_HORN] = JTP_TILE_TOOLED_HORN;
  jtp_objtiles[HORN_OF_PLENTY] = JTP_TILE_FROST_HORN;
  jtp_objtiles[BUGLE] = JTP_TILE_FROST_HORN;
  jtp_objtiles[FROST_HORN] = JTP_TILE_FROST_HORN;
  jtp_objtiles[FIRE_HORN] = JTP_TILE_FIRE_HORN;
  jtp_objtiles[UNICORN_HORN] = JTP_TILE_UNICORN_HORN;

  jtp_objtiles[DILITHIUM_CRYSTAL] = JTP_TILE_DILITHIUM_CRYSTAL;
  jtp_objtiles[DIAMOND] = JTP_TILE_DIAMOND;
  jtp_objtiles[RUBY] = JTP_TILE_RUBY;
  jtp_objtiles[JACINTH] = JTP_TILE_JACINTH;
  jtp_objtiles[SAPPHIRE] = JTP_TILE_SAPPHIRE;
  jtp_objtiles[BLACK_OPAL] = JTP_TILE_BLACK_OPAL;
  jtp_objtiles[EMERALD] = JTP_TILE_EMERALD;
  jtp_objtiles[TURQUOISE] = JTP_TILE_TURQUOISE;
  jtp_objtiles[CITRINE] = JTP_TILE_CITRINE;
  jtp_objtiles[AQUAMARINE] = JTP_TILE_AQUAMARINE;
  jtp_objtiles[AMBER] = JTP_TILE_AMBER;
  jtp_objtiles[TOPAZ] = JTP_TILE_TOPAZ;
  jtp_objtiles[JET] = JTP_TILE_JET;
  jtp_objtiles[OPAL] = JTP_TILE_OPAL;
  jtp_objtiles[CHRYSOBERYL] = JTP_TILE_CHRYSOBERYL;
  jtp_objtiles[GARNET] = JTP_TILE_GARNET;
  jtp_objtiles[AMETHYST] = JTP_TILE_AMETHYST;
  jtp_objtiles[JASPER] = JTP_TILE_JASPER;
  jtp_objtiles[FLUORITE] = JTP_TILE_FLUORITE;
  jtp_objtiles[OBSIDIAN] = JTP_TILE_OBSIDIAN;
  jtp_objtiles[AGATE] = JTP_TILE_AGATE;
  jtp_objtiles[JADE] = JTP_TILE_JADE;

  jtp_objtiles[LUCKSTONE] = JTP_TILE_LUCKSTONE;
  jtp_objtiles[LOADSTONE] = JTP_TILE_LOADSTONE;
  jtp_objtiles[FLINT] = JTP_TILE_FLINT;
  jtp_objtiles[ROCK] = JTP_TILE_ROCKS;
  jtp_objtiles[TOUCHSTONE] = JTP_TILE_TOUCHSTONE;
#ifdef HEALTHSTONE /* only in SlashEM */
  jtp_objtiles[HEALTHSTONE] = JTP_TILE_HEALTHSTONE;
#endif
#ifdef WHETSTONE /* only in SlashEM */
  jtp_objtiles[WHETSTONE] = JTP_TILE_WHETSTONE;
#endif

  jtp_objtiles[SACK] = JTP_TILE_SACK;
  jtp_objtiles[BAG_OF_HOLDING] = JTP_TILE_BAG_OF_HOLDING;
  jtp_objtiles[OILSKIN_SACK] = JTP_TILE_OILSKIN_SACK;
  jtp_objtiles[BAG_OF_TRICKS] = JTP_TILE_BAG_OF_TRICKS;

#ifdef SANITY_CHECKS
  for (i = 0; i < NUM_OBJECTS; i++)
  {
    if (jtp_objtiles[i] == JTP_TILE_NONE)
    {
      jtp_objtiles[i] = JTP_TILE_DEFAULT_OBJECT;
      temp_descr = OBJ_NAME(objects[i]);
      fprintf(stderr, "object %d (%s) has no tile assigned\n",
        i,
        temp_descr ? temp_descr : "(nil)");
    }
  }
  /*
   * current reports:
   * none
   */
#endif

  /* Clean up */
  free(objclasstiles);
}


void jtp_init_glyph_tiles(void)
{
  int i;
  jtp_cmaptiles = (int *)malloc(MAXPCHARS*sizeof(int));
  
  if (!jtp_cmaptiles)
    OOM(1);

  jtp_init_monster_tile_table();
  jtp_init_object_tile_table();
  jtp_init_engulf_tile_table();

  for (i = 0; i < MAXPCHARS; i++)
    jtp_cmaptiles[i] = JTP_TILE_VDOOR_WOOD_CLOSED;
  
  jtp_cmaptiles[S_stone] = JTP_TILE_UNMAPPED_AREA;
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
  jtp_cmaptiles[S_teleportation_trap] = JTP_TILE_TRAP_TELEPORTER;
  jtp_cmaptiles[S_tree] = JTP_TILE_TREE;
  jtp_cmaptiles[S_cloud] = JTP_TILE_CLOUD;
  jtp_cmaptiles[S_air] = JTP_TILE_FLOOR_AIR;
  jtp_cmaptiles[S_grave] = JTP_TILE_GRAVE;
  jtp_cmaptiles[S_sink] = JTP_TILE_SINK;
  jtp_cmaptiles[S_bear_trap] = JTP_TILE_TRAP_BEAR;
  jtp_cmaptiles[S_rust_trap] = JTP_TILE_TRAP_WATER;
  jtp_cmaptiles[S_pit] = JTP_TILE_TRAP_PIT;
  jtp_cmaptiles[S_hole] = JTP_TILE_TRAP_PIT;
  jtp_cmaptiles[S_trap_door] = JTP_TILE_TRAP_DOOR;
  jtp_cmaptiles[S_water] = JTP_TILE_FLOOR_WATER;
  jtp_cmaptiles[S_pool] = JTP_TILE_FLOOR_WATER;
  jtp_cmaptiles[S_ice] = JTP_TILE_FLOOR_ICE;
  jtp_cmaptiles[S_lava] = JTP_TILE_FLOOR_LAVA;
  jtp_cmaptiles[S_throne] = JTP_TILE_THRONE;
  jtp_cmaptiles[S_bars] = JTP_TILE_BARS;
  jtp_cmaptiles[S_upladder] = JTP_TILE_LADDER_UP;
  jtp_cmaptiles[S_dnladder] = JTP_TILE_LADDER_DOWN;
  jtp_cmaptiles[S_arrow_trap] = JTP_TILE_TRAP_ARROW;
  jtp_cmaptiles[S_rolling_boulder_trap] = JTP_TILE_ROLLING_BOULDER_TRAP;
  jtp_cmaptiles[S_sleeping_gas_trap] = JTP_TILE_GAS_TRAP;
  jtp_cmaptiles[S_fire_trap] = JTP_TILE_TRAP_FIRE;
  jtp_cmaptiles[S_web] = JTP_TILE_WEB_TRAP;
  jtp_cmaptiles[S_statue_trap] = JTP_TILE_STATUE;
  jtp_cmaptiles[S_anti_magic_trap] = JTP_TILE_TRAP_ANTI_MAGIC;
  jtp_cmaptiles[S_polymorph_trap] = JTP_TILE_TRAP_POLYMORPH;
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
  jtp_cmaptiles[S_ss1] = JTP_TILE_RESIST_SPELL_1;
  jtp_cmaptiles[S_ss2] = JTP_TILE_RESIST_SPELL_2;
  jtp_cmaptiles[S_ss3] = JTP_TILE_RESIST_SPELL_3;
  jtp_cmaptiles[S_ss4] = JTP_TILE_RESIST_SPELL_4;
  jtp_cmaptiles[S_dart_trap] = JTP_TILE_DART_TRAP;
  jtp_cmaptiles[S_falling_rock_trap] = JTP_TILE_FALLING_ROCK_TRAP;
  jtp_cmaptiles[S_squeaky_board] = JTP_TILE_SQUEAKY_BOARD;
  jtp_cmaptiles[S_land_mine] = JTP_TILE_LAND_MINE;
  jtp_cmaptiles[S_magic_portal] = JTP_TILE_MAGIC_PORTAL;
  jtp_cmaptiles[S_spiked_pit] = JTP_TILE_TILE_SPIKED_PIT;
  jtp_cmaptiles[S_hole] = JTP_TILE_HOLE;
  jtp_cmaptiles[S_level_teleporter] = JTP_TILE_LEVEL_TELEPORTER;
  jtp_cmaptiles[S_magic_trap] = JTP_TILE_MAGIC_TRAP;
  jtp_cmaptiles[S_digbeam] = JTP_TILE_DIGBEAM;
  jtp_cmaptiles[S_flashbeam] = JTP_TILE_FLASHBEAM;
  jtp_cmaptiles[S_boomleft] = JTP_TILE_BOOMLEFT;
  jtp_cmaptiles[S_boomright] = JTP_TILE_BOOMRIGHT;
  jtp_cmaptiles[S_hcdbridge] = JTP_TILE_HCDBRIDGE;
  jtp_cmaptiles[S_vcdbridge] = JTP_TILE_VCDBRIDGE;
  jtp_cmaptiles[S_hodbridge] = JTP_TILE_HODBRIDGE;
  jtp_cmaptiles[S_vodbridge] = JTP_TILE_VODBRIDGE;

  jtp_tile_conversion_initialized = 1;
}


char *jtp_make_filename(const char *subdir1, const char *subdir2, const char *name)
{
	char *filename;
	
	filename = (char *)malloc(strlen(jtp_game_path) + 1 +
		(subdir1 ? strlen(subdir1) + 2 : 0) +
		(subdir2 ? strlen(subdir2) + 2 : 0) +
		strlen(name) + 1);
	if (filename == NULL)
		OOM(1);
		
	/*
	 * may need to be fixed on OSses like VMS
	 */
	strcpy(filename, jtp_game_path);
	append_slash(filename);
	if (subdir1)
	{
		strcat(filename, subdir1);
		append_slash(filename);
	}
	if (subdir2)
	{
		strcat(filename, subdir2);
		append_slash(filename);
	}
	strcat(filename, name);
	return filename;
}


/*
 * Load a PCX file from the graphics directory.
 * Returns the buffer from jtp_load_PCX_buf
 * (width & height encoded in first 4 bytes)
 */
unsigned char *jtp_load_graphic(const char *subdir, const char *name, int load_palette)
{
	unsigned char *image;
	char *filename;
	char namebuf[128];
	
	strcat(strcpy(namebuf, name), ".pcx");
	filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, subdir, namebuf);
	if (filename == NULL)
		OOM(1);
	if (access(filename, R_OK) != 0)
	{
		/*
		 * try main directory if not found in sub-directory
		 */
		if (subdir != NULL)
		{
			free(filename);
			filename = 	jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, namebuf);
		}
	}
	if (jtp_load_PCX_buf(0, 0, filename, &image, load_palette) == JTP_PCX_FAILURE)
	{
		image = NULL;
	}
	setlastpcx(filename);
	free(filename);
	return image;
}


static jtp_tile *jtp_get_tile_src(
  int x1, int y1,
  int x2, int y2,
  unsigned char *image
)
{
  jtp_tile * temp1;
  int i, j, tx1 = 0, ty1 = 0, tx2 = -1, ty2 = -1;
  int image_width, image_height;
 
  temp1 =(jtp_tile *)malloc(sizeof(jtp_tile));
  if (!temp1)
  {
    OOM(1);
  }
  jtp_get_dimensions(image, &image_width, &image_height);
  image += 4;
 
  for (i = y1; i <= y2; i++)
    for (j = x1; j <= x2; j++)
      if (image[i * image_width + j] != JTP_COLOR_BACKGROUND)
      {
        ty1 = i;
        i = y2 + 1;
        j = x2 + 1;
      }
 
  for (i = y2; i >= y1; i--)
    for (j = x1; j <= x2; j++)
      if (image[i * image_width + j] != JTP_COLOR_BACKGROUND)
      {
        ty2 = i;
        i = y1 - 1;
        j = x2 + 1;
      }
 
  for (j = x1; j <= x2; j++)
    for (i = ty1; i <= ty2; i++)
      if (image[i * image_width + j] != JTP_COLOR_BACKGROUND)
      {
        tx1 = j;
        i = ty2 + 1;
        j = x2 + 1;
      }

  for (j = x2; j >= x1; j--)
    for (i = ty1; i <= ty2; i++)
      if (image[i * image_width + j] != JTP_COLOR_BACKGROUND)
      {
        tx2 = j;
        i = ty2 + 1;
        j = x1 - 1;
      }
 
  temp1->xmod = 9999;
  for (i = x1; i <= x2; i++)
  {
    if (image[(y1 - 1) * image_width + i] == JTP_COLOR_HOTSPOT)
      temp1->xmod = tx1 - i;
  }
  
  temp1->ymod = 9999;
  for (i = y1; i <= y2; i++)
  {
    if (image[i * image_width + x1 - 1] == JTP_COLOR_HOTSPOT)
      temp1->ymod = ty1 - i;
  }
  
  temp1->graphic = jtp_get_img_src(tx1, ty1, tx2, ty2, image - 4);

#ifdef SANITY_CHECKS
  if (temp1->xmod == 9999 || temp1->ymod == 9999)
  {
    fprintf(stderr, "no hotspot found in file %s\n", lastpcx);
  }
  for (i = x1 - 1; i <= x2 + 1; i++)
  {
    if ((image[(y1 - 1) * image_width + i] != JTP_COLOR_HOTSPOT || i == (x1 - 1) || i == (x2 + 1)) &&
      image[(y1 - 1) * image_width + i] != JTP_COLOR_BORDER)
      fprintf(stderr, "warning: pixel at %d,%d in file %s is neither border nor hotspot, wrong dimensions?\n",
        i, y1 - 1, lastpcx);
    if ((image[(y2 + 1) * image_width + i] != JTP_COLOR_HOTSPOT || i == (x1 - 1) || i == (x2 + 1)) &&
      image[(y2 + 1) * image_width + i] != JTP_COLOR_BORDER)
      fprintf(stderr, "warning: pixel at %d,%d in file %s is neither border nor hotspot, wrong dimensions?\n",
        i, y2 + 1, lastpcx);
  }

  for (i = y1 - 1; i <= y2 + 1; i++)
  {
    if ((image[i * image_width + x1 - 1] != JTP_COLOR_HOTSPOT || i == (y1 - 1) || i == (y2 + 1)) &&
      image[i * image_width + x1 - 1] != JTP_COLOR_BORDER)
      fprintf(stderr, "warning: pixel at %d,%d in file %s is neither border nor hotspot, wrong dimensions?\n",
        x1 - 1, i, lastpcx);
    if ((image[i * image_width + x2 + 1] != JTP_COLOR_HOTSPOT || i == (y1 - 1) || i == (y2 + 1)) &&
      image[i * image_width + x2 + 1] != JTP_COLOR_BORDER)
      fprintf(stderr, "warning: pixel at %d,%d in file %s is neither border nor hotspot, wrong dimensions?\n",
        x2 + 1, i, lastpcx);
  }

  if (temp1->graphic == NULL)
  {
    fprintf(stderr, "error loading a tile of size %dx%d, from %d.%d-%d.%d in file %s of size %dx%d\n",
      tx2 - tx1 + 1,
      ty2 - ty1 + 1,
      x1, y1, x2, y2,
      lastpcx,
      image_width, image_height);
  }
#endif

  if (temp1->xmod == 9999)
    temp1->xmod = 0;
  if (temp1->ymod == 9999)
    temp1->ymod = 0;

  return temp1;
}


static jtp_tile *jtp_get_tile(int x1, int y1, int x2, int y2)
{
  return jtp_get_tile_src(x1, y1, x2, y2, jtp_screen.vpage - 4);
}


static unsigned char *jtp_init_shades(char *fname)
{
 int i, j;
 FILE * f;
 unsigned char *temp1;

 f = fopen(fname, "rb");
 if (!f) return(NULL);
 
 temp1 = (unsigned char *)malloc(JTP_MAX_SHADES*256*sizeof(unsigned char));
 if (!temp1) 
 {
   OOM(1);
 }  
 
 fread(temp1,1,JTP_MAX_SHADES*256,f);
 fclose(f);
 
 /* 
  * Make sure that the background color is not used as a shade.
  * Replace any occurences with color 47 (which is currently black)
  */
 for (i = 1; i < JTP_MAX_SHADES; i++)
   for (j = 0; j < 256; j++)
     if (temp1[i*256+j] == JTP_COLOR_BACKGROUND)
       temp1[i*256+j] = 47;
 
 return(temp1);
}



static void jtp_get_floor_style
(
  int style_index,
  int xspan, int yspan,
  int x1, int y1
)
{
  int i, j;

  jtp_floors[style_index].xspan = xspan;
  jtp_floors[style_index].yspan = yspan;
  jtp_floors[style_index].pattern = (jtp_tile **)malloc(xspan*yspan*sizeof(jtp_tile *));

#define FS_DX 116
#define FS_DY 53
#define FS_EX 112
#define FS_EY 49
  for (i = 0; i < yspan; i++)
    for (j = 0; j < xspan; j++)
      jtp_floors[style_index].pattern[i*xspan+j] = jtp_get_tile(x1 + FS_DX*j, y1 + FS_DY*i, x1 + FS_DX*j + FS_EX, y1 + FS_DY*i + FS_EY);
}


static void jtp_get_floor_edge_style
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


static void jtp_get_wall_style
(
  int style_index,
  int x1, int y1
)
{
#define WS_DX 60
#define WS_EX 56
#define WS_EY 122
  jtp_walls[style_index].west = jtp_get_tile(x1, y1, x1 + WS_EX, y1 + WS_EY);
  jtp_walls[style_index].north = jtp_get_tile(x1 + WS_DX, y1, x1 + WS_DX + WS_EX, y1 + WS_EY);
  jtp_walls[style_index].south = jtp_get_tile(x1 + 2*WS_DX, y1, x1 + 2*WS_DX + WS_EX, y1 + WS_EY);
  jtp_walls[style_index].east = jtp_get_tile(x1 + 3*WS_DX, y1, x1 + 3*WS_DX + WS_EX, y1 + WS_EY);
}


static void jtp_get_tile_group
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


static int jtp_init_tilegraphics(void)
{
  int i, j;
  int **tileloc;
  int all_ok = TRUE;
  char *filename;

  filename = jtp_make_filename(JTP_CONFIG_DIRECTORY, NULL, JTP_FILENAME_SHADING_TABLE);
  jtp_shade = jtp_init_shades(filename);
  free(filename);
  all_ok &= jtp_shade != NULL;
  
  tileloc = (int **)malloc(20*sizeof(int *));
  for (i = 0; i < 20; i++) 
    tileloc[i] = (int *)malloc(20*sizeof(int));

  jtp_tiles = (jtp_tile **)calloc(JTP_MAX_TILES, sizeof(jtp_tile *));
  jtp_walls = (jtp_wall_style *)calloc(JTP_MAX_WALL_STYLES, sizeof(jtp_wall_style));
  jtp_floors = (jtp_floor_style *)calloc(JTP_MAX_FLOOR_STYLES, sizeof(jtp_floor_style));
  jtp_floor_edges = (jtp_floor_edge_style *)calloc(JTP_MAX_FLOOR_EDGE_STYLES, sizeof(jtp_floor_edge_style));
  if (jtp_tiles == NULL || jtp_walls == NULL || jtp_floors == NULL || jtp_floor_edges == NULL)
    OOM(1);
  
  /* Load wall tiles */
  if (jtp_wall_display_style == JTP_WALL_DISPLAY_STYLE_FULL)
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls02a.pcx");
  else if (jtp_wall_display_style == JTP_WALL_DISPLAY_STYLE_HALF_HEIGHT)
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls02c.pcx");
  else if (jtp_wall_display_style == JTP_WALL_DISPLAY_STYLE_TRANSPARENT)
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls02b.pcx");
  else
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls02a.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  
  jtp_get_wall_style(JTP_WALL_STYLE_BRICK, 1, 1);
  jtp_get_wall_style(JTP_WALL_STYLE_BRICK_BANNER, 241, 1);
  jtp_get_wall_style(JTP_WALL_STYLE_BRICK_PAINTING, 1, 127);
  jtp_get_wall_style(JTP_WALL_STYLE_BRICK_POCKET, 481, 1);
  jtp_get_wall_style(JTP_WALL_STYLE_BRICK_PILLAR, 241, 127);
  jtp_get_wall_style(JTP_WALL_STYLE_MARBLE, 1, 253);

  /* Load more wall tiles */
  if (jtp_wall_display_style == JTP_WALL_DISPLAY_STYLE_FULL)
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls03a.pcx");
  else if (jtp_wall_display_style == JTP_WALL_DISPLAY_STYLE_HALF_HEIGHT)
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls03c.pcx");
  else if (jtp_wall_display_style == JTP_WALL_DISPLAY_STYLE_TRANSPARENT)
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls03b.pcx");
  else
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls03a.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);

  jtp_get_wall_style(JTP_WALL_STYLE_VINE_COVERED, 1, 1);
  jtp_get_wall_style(JTP_WALL_STYLE_STUCCO, 1, 127);
  jtp_get_wall_style(JTP_WALL_STYLE_ROUGH, 1, 253);

  /* Load even more wall tiles */
  if (jtp_wall_display_style == JTP_WALL_DISPLAY_STYLE_FULL)
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls04a.pcx");
  else if (jtp_wall_display_style == JTP_WALL_DISPLAY_STYLE_HALF_HEIGHT)
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls04c.pcx");
  else if (jtp_wall_display_style == JTP_WALL_DISPLAY_STYLE_TRANSPARENT)
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls04b.pcx");
  else
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "walls04a.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);

  jtp_get_wall_style(JTP_WALL_STYLE_DARK, 1, 1);
  jtp_get_wall_style(JTP_WALL_STYLE_LIGHT, 1, 127);

  /* Load floor pattern tiles */
  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_cm12.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
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

  /* Load more floor pattern tiles */

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_cm13.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
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
  /* Lit rough floor tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_ROUGH_LIT, 3, 3, 1, 372);
  /* Air tiles (3x3 pattern) */
  jtp_get_floor_style(JTP_FLOOR_STYLE_AIR, 3, 3, 349, 372);
  /* Rogue tile (1x1 pattern) */
/*  jtp_get_floor_style(JTP_FLOOR_STYLE_DARK, 1, 1, 349, 372); */
  jtp_get_floor_style(JTP_FLOOR_STYLE_DARK, 1, 1, 1, 531);

#define JTP_CMAP_TILE_DX 116
#define JTP_CMAP_TILE_DY 126

  /* Load miscellaneous cmap tiles */

  tileloc[0][0] = JTP_TILE_DOOR_WOOD_BROKEN; tileloc[0][1] = JTP_TILE_HDOOR_WOOD_CLOSED;
  tileloc[0][2] = JTP_TILE_VDOOR_WOOD_CLOSED; tileloc[0][3] = JTP_TILE_VDOOR_WOOD_OPEN;
  tileloc[0][4] = JTP_TILE_HDOOR_WOOD_OPEN; tileloc[0][5] = JTP_TILE_TRAP_BEAR;
  tileloc[1][0] = JTP_TILE_GRAVE; tileloc[1][1] = JTP_TILE_ALTAR;
  tileloc[1][2] = JTP_TILE_FOUNTAIN; tileloc[1][3] = JTP_TILE_STAIRS_UP;
  tileloc[1][4] = JTP_TILE_STAIRS_DOWN; tileloc[1][5] = JTP_TILE_SINK;
  tileloc[2][0] = JTP_TILE_GAS_TRAP; tileloc[2][1] = JTP_TILE_TRAP_PIT;
  tileloc[2][2] = JTP_TILE_TRAP_POLYMORPH; tileloc[2][3] = JTP_TILE_TREE;
  tileloc[2][4] = -1; tileloc[2][5] = JTP_TILE_TRAP_MAGIC;
  tileloc[3][0] = JTP_TILE_TRAP_DOOR; tileloc[3][1] = JTP_TILE_TRAP_WATER;
  tileloc[3][2] = JTP_TILE_TRAP_TELEPORTER; tileloc[3][3] = JTP_TILE_FLOOR_NOT_VISIBLE;
  tileloc[3][4] = JTP_TILE_UNMAPPED_AREA; tileloc[3][5] =  JTP_TILE_HILITE_PET;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_cm07.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_CMAP_TILE_DX, JTP_CMAP_TILE_DY, tileloc);

  /* Load more miscellaneous cmap tiles */

  tileloc[0][0] = JTP_TILE_BARS; tileloc[0][1] = JTP_TILE_THRONE;
  tileloc[0][2] = -1; tileloc[0][3] = JTP_TILE_EXPLOSION_NORTHWEST;
  tileloc[0][4] = JTP_TILE_EXPLOSION_NORTH; tileloc[0][5] = JTP_TILE_EXPLOSION_NORTHEAST;
  tileloc[1][0] = JTP_TILE_TRAP_ANTI_MAGIC; tileloc[1][1] = JTP_TILE_TRAP_ARROW;
  tileloc[1][2] = -1; tileloc[1][3] = JTP_TILE_EXPLOSION_WEST;
  tileloc[1][4] = JTP_TILE_EXPLOSION_CENTER; tileloc[1][5] = JTP_TILE_EXPLOSION_EAST;
  tileloc[2][0] = JTP_TILE_TRAP_FIRE; tileloc[2][1] = JTP_TILE_ROLLING_BOULDER_TRAP;
  tileloc[2][2] = JTP_TILE_TRAP_SLEEPGAS; tileloc[2][3] = JTP_TILE_EXPLOSION_SOUTHWEST;
  tileloc[2][4] = JTP_TILE_EXPLOSION_SOUTH; tileloc[2][5] = JTP_TILE_EXPLOSION_SOUTHEAST;
  tileloc[3][0] = JTP_TILE_ZAP_SLANT_RIGHT; tileloc[3][1] = JTP_TILE_ZAP_SLANT_LEFT;
  tileloc[3][2] = JTP_TILE_ZAP_HORIZONTAL; tileloc[3][3] = JTP_TILE_ZAP_VERTICAL;
  tileloc[3][4] = JTP_TILE_LADDER_UP; tileloc[3][5] = JTP_TILE_LADDER_DOWN;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_cmc7.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_CMAP_TILE_DX, JTP_CMAP_TILE_DY, tileloc);

  /* Load some special effect cmap tiles */

  tileloc[0][0] = JTP_TILE_RESIST_SPELL_1; tileloc[0][1] = JTP_TILE_RESIST_SPELL_2;
  tileloc[0][2] = JTP_TILE_RESIST_SPELL_3; tileloc[0][3] = JTP_TILE_RESIST_SPELL_4;
  tileloc[0][4] = JTP_TILE_WEB_TRAP; tileloc[0][5] = JTP_TILE_DART_TRAP;
  tileloc[1][0] = JTP_TILE_FALLING_ROCK_TRAP; tileloc[1][1] = JTP_TILE_SQUEAKY_BOARD;
  tileloc[1][2] = JTP_TILE_LAND_MINE; tileloc[1][3] = JTP_TILE_MAGIC_PORTAL;
  tileloc[1][4] = JTP_TILE_TILE_SPIKED_PIT; tileloc[1][5] = JTP_TILE_HOLE;
  tileloc[2][0] = JTP_TILE_LEVEL_TELEPORTER; tileloc[2][1] = JTP_TILE_MAGIC_TRAP;
  tileloc[2][2] = JTP_TILE_DIGBEAM; tileloc[2][3] = JTP_TILE_FLASHBEAM;
  tileloc[2][4] = JTP_TILE_BOOMLEFT; tileloc[2][5] = JTP_TILE_BOOMRIGHT;
  tileloc[3][0] = JTP_TILE_HCDBRIDGE; tileloc[3][1] = JTP_TILE_VCDBRIDGE;
  tileloc[3][2] = JTP_TILE_VODBRIDGE; tileloc[3][3] = JTP_TILE_HODBRIDGE;
  tileloc[3][4] = JTP_TILE_CLOUD; tileloc[3][5] = -1;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_cmc1.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_CMAP_TILE_DX, JTP_CMAP_TILE_DY, tileloc);

#define JTP_OBJ_TILE_DX 96
#define JTP_OBJ_TILE_DY 102

  /* Load object tiles */

  tileloc[0][0] = JTP_TILE_MISC; tileloc[0][1] = JTP_TILE_BOULDER;
  tileloc[0][2] = JTP_TILE_BONES; tileloc[0][3] = -1;
  tileloc[0][4] = -1; tileloc[0][5] = -1;
  tileloc[0][6] = JTP_TILE_BOW; tileloc[0][7] = JTP_TILE_KEY;
  tileloc[1][0] = JTP_TILE_STATUE; tileloc[1][1] = JTP_TILE_CHEST;
  tileloc[1][2] = JTP_TILE_COINS; tileloc[1][3] = JTP_TILE_BOOK;
  tileloc[1][4] = JTP_TILE_HELMET; tileloc[1][5] = JTP_TILE_SHIELD;
  tileloc[1][6] = JTP_TILE_AMULET; tileloc[1][7] = JTP_TILE_DAGGER;
  tileloc[2][0] = JTP_TILE_BOOTS; tileloc[2][1] = JTP_TILE_SPEAR;
  tileloc[2][2] = -1; tileloc[2][3] = JTP_TILE_SCROLL;
  tileloc[2][4] = JTP_TILE_WAND; tileloc[2][5] = JTP_TILE_SWORD;
  tileloc[2][6] = JTP_TILE_RING; tileloc[2][7] = JTP_TILE_APPLE;
  tileloc[3][0] = JTP_TILE_BLUE_GLASS; tileloc[3][1] = JTP_TILE_RING_MAIL;
  tileloc[3][2] = JTP_TILE_LEATHER_ARMOR; tileloc[3][3] = JTP_TILE_PLATE_MAIL;
  tileloc[3][4] = JTP_TILE_HAMMER; tileloc[3][5] = JTP_TILE_AXE;
  tileloc[3][6] = JTP_TILE_LANTERN; tileloc[3][7] = JTP_TILE_PEAR;
  tileloc[4][0] = JTP_TILE_SCALE_MAIL; tileloc[4][1] = JTP_TILE_CHAIN_MAIL;
  tileloc[4][2] = JTP_TILE_CLOAK; tileloc[4][3] = JTP_TILE_TRIDENT;
  tileloc[4][4] = JTP_TILE_CAMERA; tileloc[4][5] = JTP_TILE_FEDORA;
  tileloc[4][6] = JTP_TILE_CLUB; tileloc[4][7] = JTP_TILE_ARROW;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_obj1.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(5, 8, 1, 1, JTP_OBJ_TILE_DX, JTP_OBJ_TILE_DY, tileloc);

  /* Load more object tiles */

  tileloc[0][0] = JTP_TILE_EGG; tileloc[0][1] = JTP_TILE_GLOVES;
  tileloc[0][2] = JTP_TILE_BELL; tileloc[0][3] = JTP_TILE_RED_GLASS;
  tileloc[0][4] = JTP_TILE_GREEN_GLASS; tileloc[0][5] = JTP_TILE_CANDLE;
  tileloc[0][6] = JTP_TILE_CANDY_BAR; tileloc[0][7] = JTP_TILE_CARROT;
  tileloc[1][0] = JTP_TILE_YELLOW_GLASS; tileloc[1][1] = JTP_TILE_WHITE_GLASS;
  tileloc[1][2] = JTP_TILE_BLACK_GLASS; tileloc[1][3] = JTP_TILE_WHIP;
  tileloc[1][4] = JTP_TILE_MACE; tileloc[1][5] = JTP_TILE_TOOLED_HORN;
  tileloc[1][6] = JTP_TILE_FORTUNE_COOKIE; tileloc[1][7] = JTP_TILE_BANANA;
  tileloc[2][0] = JTP_TILE_CRYSTAL_BALL; tileloc[2][1] = JTP_TILE_FROST_HORN;
  tileloc[2][2] = JTP_TILE_UNICORN_HORN; tileloc[2][3] = JTP_TILE_HAWAIIAN_SHIRT;
  tileloc[2][4] = JTP_TILE_CREDIT_CARD; tileloc[2][5] = JTP_TILE_MIRROR;
  tileloc[2][6] = JTP_TILE_CREAM_PIE; tileloc[2][7] = JTP_TILE_ORANGE;
  tileloc[3][0] = JTP_TILE_CROSSBOW; tileloc[3][1] = JTP_TILE_CONICAL_HAT;
  tileloc[3][2] = JTP_TILE_MAGIC_MARKER; tileloc[3][3] = JTP_TILE_STAFF;
  tileloc[3][4] = JTP_TILE_FOOD_RATION; tileloc[3][5] = JTP_TILE_PANCAKE;
  tileloc[3][6] = JTP_TILE_LEMBAS_WAFER; tileloc[3][7] = JTP_TILE_KELP_FROND;
  tileloc[4][0] = JTP_TILE_TRIPE_RATION; tileloc[4][1] = JTP_TILE_MEAT_STICK;
  tileloc[4][2] = JTP_TILE_SHURIKEN; tileloc[4][3] = JTP_TILE_STONE;
  tileloc[4][4] = JTP_TILE_PICKAXE; tileloc[4][5] = JTP_TILE_TIN;
  tileloc[4][6] = JTP_TILE_CLOVE_OF_GARLIC; tileloc[4][7] = JTP_TILE_EUCALYPTUS_LEAF;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_obj2.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(5, 8, 1, 1, JTP_OBJ_TILE_DX, JTP_OBJ_TILE_DY, tileloc);

  /* Load even more object tiles */

  tileloc[0][0] = JTP_TILE_MELON; tileloc[0][1] = JTP_TILE_SLIME_MOLD;
  tileloc[0][2] = JTP_TILE_SPRIG_OF_WOLFSBANE; tileloc[0][3] = JTP_TILE_CRAM_RATION;
  tileloc[0][4] = JTP_TILE_HUGE_CHUNK_OF_MEAT; tileloc[0][5] = JTP_TILE_MEATBALL;
  tileloc[0][6] = JTP_TILE_K_RATION; tileloc[0][7] = JTP_TILE_C_RATION;
  tileloc[1][0] = JTP_TILE_BOOMERANG; tileloc[1][1] = JTP_TILE_LUMP_OF_ROYAL_JELLY;
  tileloc[1][2] = JTP_TILE_MEAT_RING; tileloc[1][3] = JTP_TILE_DILITHIUM_CRYSTAL;
  tileloc[1][4] = JTP_TILE_DIAMOND; tileloc[1][5] = JTP_TILE_RUBY;
  tileloc[1][6] = JTP_TILE_JACINTH; tileloc[1][7] = JTP_TILE_SAPPHIRE;
  tileloc[2][0] = JTP_TILE_EMERALD; tileloc[2][1] = JTP_TILE_BLACK_OPAL;
  tileloc[2][2] = JTP_TILE_TURQUOISE; tileloc[2][3] = JTP_TILE_CITRINE;
  tileloc[2][4] = JTP_TILE_AQUAMARINE; tileloc[2][5] = JTP_TILE_AMBER;
  tileloc[2][6] = JTP_TILE_TOPAZ; tileloc[2][7] = JTP_TILE_JET;
  tileloc[3][0] = JTP_TILE_OPAL; tileloc[3][1] = JTP_TILE_CHRYSOBERYL;
  tileloc[3][2] = JTP_TILE_GARNET; tileloc[3][3] = JTP_TILE_AMETHYST;
  tileloc[3][4] = JTP_TILE_JASPER; tileloc[3][5] = JTP_TILE_FLUORITE;
  tileloc[3][6] = JTP_TILE_OBSIDIAN; tileloc[3][7] = JTP_TILE_AGATE;
  tileloc[4][0] = JTP_TILE_JADE; tileloc[4][1] = JTP_TILE_LUCKSTONE;
  tileloc[4][2] = JTP_TILE_LOADSTONE; tileloc[4][3] = JTP_TILE_FLINT;
  tileloc[4][4] = JTP_TILE_VIOLET_GLASS; tileloc[4][5] = JTP_TILE_ORANGE_GLASS;
  tileloc[4][6] = JTP_TILE_TINNING_KIT; tileloc[4][7] = JTP_TILE_FIRE_HORN;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_obj3.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(5, 8, 1, 1, JTP_OBJ_TILE_DX, JTP_OBJ_TILE_DY, tileloc);

  /* Load potion/misc object tiles */

  tileloc[0][0] = JTP_TILE_RUBY_POTION; tileloc[0][1] = JTP_TILE_PINK_POTION;
  tileloc[0][2] = JTP_TILE_ORANGE_POTION; tileloc[0][3] = JTP_TILE_YELLOW_POTION;
  tileloc[0][4] = JTP_TILE_EMERALD_POTION; tileloc[0][5] = JTP_TILE_DARK_GREEN_POTION;
  tileloc[0][6] = JTP_TILE_CYAN_POTION; tileloc[0][7] = JTP_TILE_SKY_BLUE_POTION;
  tileloc[1][0] = JTP_TILE_BRILLIANT_BLUE_POTION; tileloc[1][1] = JTP_TILE_MAGENTA_POTION;
  tileloc[1][2] = JTP_TILE_PURPLE_RED_POTION; tileloc[1][3] = JTP_TILE_PUCE_POTION;
  tileloc[1][4] = JTP_TILE_MILKY_POTION; tileloc[1][5] = JTP_TILE_SWIRLY_POTION;
  tileloc[1][6] = JTP_TILE_BUBBLY_POTION; tileloc[1][7] = JTP_TILE_SMOKY_POTION;
  tileloc[2][0] = JTP_TILE_CLOUDY_POTION; tileloc[2][1] = JTP_TILE_EFFERVESCENT_POTION;
  tileloc[2][2] = JTP_TILE_BLACK_POTION; tileloc[2][3] = JTP_TILE_GOLDEN_POTION;
  tileloc[2][4] = JTP_TILE_BROWN_POTION; tileloc[2][5] = JTP_TILE_FIZZY_POTION;
  tileloc[2][6] = JTP_TILE_DARK_POTION; tileloc[2][7] = JTP_TILE_WHITE_POTION;
  tileloc[3][0] = JTP_TILE_MURKY_POTION; tileloc[3][1] = JTP_TILE_WATER;
  tileloc[3][2] = JTP_TILE_HOLY_WATER; tileloc[3][3] = JTP_TILE_UNHOLY_WATER;
  tileloc[3][4] = JTP_TILE_SACK; tileloc[3][5] = JTP_TILE_BAG_OF_HOLDING;
  tileloc[3][6] = JTP_TILE_OILSKIN_SACK; tileloc[3][7] = JTP_TILE_BAG_OF_TRICKS;
  tileloc[4][0] = JTP_TILE_ICE_BOX; tileloc[4][1] = JTP_TILE_LARGE_BOX;
  tileloc[4][2] = JTP_TILE_UNIDENTIFIED_BAG; tileloc[4][3] = JTP_TILE_TOUCHSTONE;
  tileloc[4][4] = JTP_TILE_ROCKS; tileloc[4][5] = JTP_TILE_BROWN_GLASS;
  tileloc[4][6] = JTP_TILE_HEALTHSTONE; tileloc[4][7] = JTP_TILE_WHETSTONE;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_obj4.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(5, 8, 1, 1, JTP_OBJ_TILE_DX, JTP_OBJ_TILE_DY, tileloc);

#define JTP_MON_TILE_DX 116
#define JTP_MON_TILE_DY 126
#define JTP_MON_LARGE_TILE_DX 154
#define JTP_MON_LARGE_TILE_DY 165
#define JTP_MON_HUGE_TILE_DX 232
#define JTP_MON_HUGE_TILE_DY 252

  /* Load monster tiles */

  tileloc[0][0] = JTP_TILE_KNIGHT; tileloc[0][1] = JTP_TILE_GOBLIN;
  tileloc[0][2] = JTP_TILE_QUEEN_BEE; tileloc[0][3] = JTP_TILE_FIRE_ELEMENTAL;
  tileloc[0][4] = JTP_TILE_GREEN_SLIME; tileloc[0][5] = JTP_TILE_SKELETON;
  tileloc[1][0] = JTP_TILE_EYE; tileloc[1][1] = JTP_TILE_QUANTUM_MECHANIC;
  tileloc[1][2] = JTP_TILE_HOUSECAT; tileloc[1][3] = JTP_TILE_LIZARD;
  tileloc[1][4] = JTP_TILE_LITTLE_DOG; tileloc[1][5] = JTP_TILE_TROLL;
  tileloc[2][0] = JTP_TILE_WOOD_NYMPH; tileloc[2][1] = JTP_TILE_MOUNTAIN_NYMPH;
  tileloc[2][2] = JTP_TILE_WIZARD; tileloc[2][3] = JTP_TILE_VALKYRIE;
  tileloc[2][4] = JTP_TILE_RANGER; tileloc[2][5] = JTP_TILE_WATER_NYMPH;
  tileloc[3][0] = JTP_TILE_GHOST; tileloc[3][1] = JTP_TILE_WRAITH;
  tileloc[3][2] = JTP_TILE_GIANT_BEETLE; tileloc[3][3] = JTP_TILE_HUMAN_ZOMBIE;
  tileloc[3][4] = JTP_TILE_CENTIPEDE; tileloc[3][5] = JTP_TILE_RAT;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon6.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);

  /* Load more monster tiles */

  tileloc[0][0] = JTP_TILE_ARCHEOLOGIST; tileloc[0][1] = JTP_TILE_TOURIST;
  tileloc[0][2] = JTP_TILE_ROGUE; tileloc[0][3] = JTP_TILE_PRIEST;
  tileloc[0][4] = JTP_TILE_COYOTE; tileloc[0][5] = JTP_TILE_JACKAL;
  tileloc[1][0] = JTP_TILE_VAMPIRE_BAT; tileloc[1][1] = JTP_TILE_MOUNTAIN_CENTAUR;
  tileloc[1][2] = JTP_TILE_SOLDIER_ANT; tileloc[1][3] = JTP_TILE_KILLER_BEE;
  tileloc[1][4] = JTP_TILE_YELLOW_LIGHT; tileloc[1][5] = JTP_TILE_FOX;
  tileloc[2][0] = JTP_TILE_COCKATRICE; tileloc[2][1] = JTP_TILE_KRAKEN;
  tileloc[2][2] = JTP_TILE_GIANT_EEL; tileloc[2][3] = JTP_TILE_PYROLISK;
  tileloc[2][4] = JTP_TILE_YELLOW_MOLD; tileloc[2][5] = JTP_TILE_SHARK;
  tileloc[3][0] = JTP_TILE_CHICKATRICE; tileloc[3][1] = JTP_TILE_SMALL_MIMIC;
  tileloc[3][2] = JTP_TILE_LARGE_MIMIC; tileloc[3][3] = JTP_TILE_GIANT_MIMIC;
  tileloc[3][4] = JTP_TILE_RED_MOLD; tileloc[3][5] = JTP_TILE_VIOLET_FUNGUS;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon7.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);

  /* Load even more monster tiles */

  tileloc[0][0] = JTP_TILE_ACID_BLOB; tileloc[0][1] = JTP_TILE_QUIVERING_BLOB;
  tileloc[0][2] = JTP_TILE_GELATINOUS_CUBE; tileloc[0][3] = JTP_TILE_BROWN_MOLD;
  tileloc[0][4] = JTP_TILE_LICHEN; tileloc[0][5] = JTP_TILE_GREEN_MOLD;
  tileloc[1][0] = JTP_TILE_SHRIEKER; tileloc[1][1] = JTP_TILE_STALKER;
  tileloc[1][2] = JTP_TILE_AIR_ELEMENTAL; tileloc[1][3] = JTP_TILE_EARTH_ELEMENTAL;
  tileloc[1][4] = JTP_TILE_WATER_ELEMENTAL; tileloc[1][5] = JTP_TILE_BLACK_LIGHT;
  tileloc[2][0] = JTP_TILE_ROCK_PIERCER; tileloc[2][1] = JTP_TILE_IRON_PIERCER;
  tileloc[2][2] = JTP_TILE_GLASS_PIERCER; tileloc[2][3] = JTP_TILE_DOG;
  tileloc[2][4] = JTP_TILE_LARGE_DOG; tileloc[2][5] = JTP_TILE_WATER_DEMON;
  tileloc[3][0] = JTP_TILE_GECKO; tileloc[3][1] = JTP_TILE_NEWT;
  tileloc[3][2] = JTP_TILE_SHOPKEEPER; tileloc[3][3] = JTP_TILE_GAS_SPORE;
  tileloc[3][4] = JTP_TILE_KITTEN; tileloc[3][5] = JTP_TILE_LARGE_CAT;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon8.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);

  /* Load yet more monster tiles */

  tileloc[0][0] = JTP_TILE_FREEZING_SPHERE; tileloc[0][1] = JTP_TILE_FLAMING_SPHERE;
  tileloc[0][2] = JTP_TILE_SHOCKING_SPHERE; tileloc[0][3] = JTP_TILE_PONY;
  tileloc[0][4] = JTP_TILE_HORSE; tileloc[0][5] = JTP_TILE_WARHORSE;
  tileloc[1][0] = JTP_TILE_ROTHE; tileloc[1][1] = JTP_TILE_GNOME;
  tileloc[1][2] = JTP_TILE_GREEN_ELF; tileloc[1][3] = JTP_TILE_DWARF;
  tileloc[1][4] = JTP_TILE_WATER_MOCCASIN; tileloc[1][5] = JTP_TILE_MANES;
  tileloc[2][0] = JTP_TILE_HOBBIT; tileloc[2][1] = JTP_TILE_BLACK_UNICORN;
  tileloc[2][2] = JTP_TILE_GRAY_UNICORN; tileloc[2][3] = JTP_TILE_WHITE_UNICORN;
  tileloc[2][4] = JTP_TILE_GIANT_ANT; tileloc[2][5] = JTP_TILE_FIRE_ANT;
  tileloc[3][0] = JTP_TILE_ELVENKING; tileloc[3][1] = JTP_TILE_ELF_LORD;
  tileloc[3][2] = JTP_TILE_GREY_ELF; tileloc[3][3] = JTP_TILE_WOODLAND_ELF;
  tileloc[3][4] = JTP_TILE_ELF; tileloc[3][5] = JTP_TILE_ROCK_MOLE;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon9.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);

  /* Load still more monster tiles */

  tileloc[0][0] = JTP_TILE_DWARF_LORD; tileloc[0][1] = JTP_TILE_DWARF_KING;
  tileloc[0][2] = JTP_TILE_GNOME_LORD; tileloc[0][3] = JTP_TILE_GNOME_KING;
  tileloc[0][4] = JTP_TILE_GNOMISH_WIZARD; tileloc[0][5] = JTP_TILE_ORC;
  tileloc[1][0] = JTP_TILE_HILL_ORC; tileloc[1][1] = JTP_TILE_MORDOR_ORC;
  tileloc[1][2] = JTP_TILE_URUK_HAI; tileloc[1][3] = JTP_TILE_HOBGOBLIN;
  tileloc[1][4] = JTP_TILE_BUGBEAR; tileloc[1][5] = JTP_TILE_KOBOLD;
  tileloc[2][0] = JTP_TILE_LARGE_KOBOLD; tileloc[2][1] = JTP_TILE_KOBOLD_SHAMAN;
  tileloc[2][2] = JTP_TILE_KOBOLD_LORD; tileloc[2][3] = JTP_TILE_ORC_CAPTAIN;
  tileloc[2][4] = JTP_TILE_ORC_SHAMAN; tileloc[2][5] = JTP_TILE_LEPRECHAUN;
  tileloc[3][0] = JTP_TILE_GARTER_SNAKE; tileloc[3][1] = JTP_TILE_SNAKE;
  tileloc[3][2] = JTP_TILE_PIT_VIPER; tileloc[3][3] = JTP_TILE_PYTHON;
  tileloc[3][4] = JTP_TILE_COBRA; tileloc[3][5] = JTP_TILE_WUMPUS;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon0.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);

  /* Still loading more monster tiles */

  tileloc[0][0] = JTP_TILE_KOBOLD_ZOMBIE; tileloc[0][1] = JTP_TILE_GNOME_ZOMBIE;
  tileloc[0][2] = JTP_TILE_DWARF_ZOMBIE; tileloc[0][3] = JTP_TILE_ORC_ZOMBIE;
  tileloc[0][4] = JTP_TILE_ELF_ZOMBIE; tileloc[0][5] = JTP_TILE_KOBOLD_MUMMY;
  tileloc[1][0] = JTP_TILE_GNOME_MUMMY; tileloc[1][1] = JTP_TILE_DWARF_MUMMY;
  tileloc[1][2] = JTP_TILE_ORC_MUMMY; tileloc[1][3] = JTP_TILE_HUMAN_MUMMY;
  tileloc[1][4] = JTP_TILE_ELF_MUMMY; tileloc[1][5] = JTP_TILE_HOMUNCULUS;
  tileloc[2][0] = JTP_TILE_IMP; tileloc[2][1] = JTP_TILE_LEMURE;
  tileloc[2][2] = JTP_TILE_QUASIT; tileloc[2][3] = JTP_TILE_TENGU;
  tileloc[2][4] = JTP_TILE_ORACLE; tileloc[2][5] = JTP_TILE_LAWFUL_PRIEST;
  tileloc[3][0] = JTP_TILE_CHAOTIC_PRIEST; tileloc[3][1] = JTP_TILE_NEUTRAL_PRIEST;
  tileloc[3][2] = JTP_TILE_UNALIGNED_PRIEST; tileloc[3][3] = JTP_TILE_WATCHMAN;
  tileloc[3][4] = JTP_TILE_WATCH_CAPTAIN; tileloc[3][5] = JTP_TILE_SOLDIER;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon1.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);

  /* Wow, this is a lot of monster tiles! */

  tileloc[0][0] = JTP_TILE_SERGEANT; tileloc[0][1] = JTP_TILE_LIEUTENANT;
  tileloc[0][2] = JTP_TILE_CAPTAIN; tileloc[0][3] = JTP_TILE_GRID_BUG;
  tileloc[0][4] = JTP_TILE_XAN; tileloc[0][5] = JTP_TILE_GARGOYLE;
  tileloc[1][0] = JTP_TILE_WINGED_GARGOYLE; tileloc[1][1] = JTP_TILE_DINGO;
  tileloc[1][2] = JTP_TILE_WOLF; tileloc[1][3] = JTP_TILE_WEREWOLF;
  tileloc[1][4] = JTP_TILE_WARG; tileloc[1][5] = JTP_TILE_WINTER_WOLF_CUB;
  tileloc[2][0] = JTP_TILE_WINTER_WOLF; tileloc[2][1] = JTP_TILE_HELL_HOUND_PUP;
  tileloc[2][2] = JTP_TILE_HELL_HOUND; tileloc[2][3] = JTP_TILE_JAGUAR;
  tileloc[2][4] = JTP_TILE_LYNX; tileloc[2][5] = JTP_TILE_PANTHER;
  tileloc[3][0] = JTP_TILE_TIGER; tileloc[3][1] = JTP_TILE_MIND_FLAYER;
  tileloc[3][2] = JTP_TILE_MASTER_MIND_FLAYER; tileloc[3][3] = JTP_TILE_LICH;
  tileloc[3][4] = JTP_TILE_DEMILICH; tileloc[3][5] = JTP_TILE_MASTER_LICH;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon2.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);

  /* Haven't we had enough monster tiles yet? */

  tileloc[0][0] = JTP_TILE_ARCH_LICH; tileloc[0][1] = JTP_TILE_GUARDIAN_NAGA;
  tileloc[0][2] = JTP_TILE_GUARDIAN_NAGA_HATCHLING; tileloc[0][3] = JTP_TILE_GOLDEN_NAGA;
  tileloc[0][4] = JTP_TILE_GOLDEN_NAGA_HATCHLING; tileloc[0][5] = JTP_TILE_RED_NAGA;
  tileloc[1][0] = JTP_TILE_RED_NAGA_HATCHLING; tileloc[1][1] = JTP_TILE_BLACK_NAGA;
  tileloc[1][2] = JTP_TILE_BLACK_NAGA_HATCHLING; tileloc[1][3] = JTP_TILE_RUST_MONSTER;
  tileloc[1][4] = JTP_TILE_GREMLIN; tileloc[1][5] = JTP_TILE_BABY_PURPLE_WORM;
  tileloc[2][0] = JTP_TILE_MONKEY; tileloc[2][1] = JTP_TILE_APE;
  tileloc[2][2] = JTP_TILE_OWLBEAR; tileloc[2][3] = JTP_TILE_YETI;
  tileloc[2][4] = JTP_TILE_CARNIVOROUS_APE; tileloc[2][5] = JTP_TILE_STRAW_GOLEM;
  tileloc[3][0] = JTP_TILE_PAPER_GOLEM; tileloc[3][1] = JTP_TILE_ROPE_GOLEM;
  tileloc[3][2] = JTP_TILE_GOLD_GOLEM; tileloc[3][3] = JTP_TILE_LEATHER_GOLEM;
  tileloc[3][4] = JTP_TILE_WOOD_GOLEM; tileloc[3][5] = JTP_TILE_FLESH_GOLEM;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon3.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);

  /* Nope, apparently not! */

  tileloc[0][0] = JTP_TILE_SCORPION; tileloc[0][1] = JTP_TILE_CAVE_SPIDER;
  tileloc[0][2] = JTP_TILE_GIANT_SPIDER; tileloc[0][3] = JTP_TILE_BAT;
  tileloc[0][4] = JTP_TILE_GIANT_BAT; tileloc[0][5] = JTP_TILE_RAVEN;
  tileloc[1][0] = JTP_TILE_DJINNI; tileloc[1][1] = -1;
  tileloc[1][2] = -1; tileloc[1][3] = -1;
  tileloc[1][4] = -1; tileloc[1][5] = -1;
  tileloc[2][0] = -1; tileloc[2][1] = -1;
  tileloc[2][2] = -1; tileloc[2][3] = -1;
  tileloc[2][4] = -1; tileloc[2][5] = -1;
  tileloc[3][0] = -1; tileloc[3][1] = -1;
  tileloc[3][2] = -1; tileloc[3][3] = -1;
  tileloc[3][4] = -1; tileloc[3][5] = -1;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon10.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);

  /* Load tall monster tiles */

  tileloc[0][0] = JTP_TILE_GIANT; tileloc[0][1] = JTP_TILE_STONE_GIANT;
  tileloc[0][2] = JTP_TILE_HILL_GIANT; tileloc[0][3] = JTP_TILE_FROST_GIANT;
  tileloc[0][4] = JTP_TILE_FIRE_GIANT; tileloc[0][5] = JTP_TILE_STORM_GIANT;
  tileloc[1][0] = JTP_TILE_ETTIN; tileloc[1][1] = JTP_TILE_TITAN;
  tileloc[1][2] = JTP_TILE_MINOTAUR; tileloc[1][3] = JTP_TILE_GIANT_ZOMBIE;
  tileloc[1][4] = JTP_TILE_ETTIN_ZOMBIE; tileloc[1][5] = JTP_TILE_GIANT_MUMMY;
  tileloc[2][0] = JTP_TILE_ETTIN_MUMMY; tileloc[2][1] = JTP_TILE_OGRE;
  tileloc[2][2] = JTP_TILE_OGRE_LORD; tileloc[2][3] = JTP_TILE_OGRE_KING;
  tileloc[2][4] = JTP_TILE_DISENCHANTER; tileloc[2][5] = JTP_TILE_SASQUATCH;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon4.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(3, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_LARGE_TILE_DY, tileloc);

  /* Load tall and wide monster tiles */

  tileloc[0][0] = JTP_TILE_SUCCUBUS; tileloc[0][1] = JTP_TILE_INCUBUS;
  tileloc[0][2] = JTP_TILE_BABY_YELLOW_DRAGON; tileloc[0][3] = JTP_TILE_BABY_WHITE_DRAGON;
  tileloc[0][4] = JTP_TILE_BABY_SILVER_DRAGON; 
  tileloc[1][0] = JTP_TILE_BABY_RED_DRAGON; tileloc[1][1] = JTP_TILE_BABY_BLACK_DRAGON;
  tileloc[1][2] = JTP_TILE_BABY_GRAY_DRAGON; tileloc[1][3] = JTP_TILE_BABY_GREEN_DRAGON;
  tileloc[1][4] = JTP_TILE_BABY_ORANGE_DRAGON; 
  tileloc[2][0] = JTP_TILE_BABY_BLUE_DRAGON; tileloc[2][1] = JTP_TILE_PURPLE_WORM;
  tileloc[2][2] = JTP_TILE_LONG_WORM_TAIL; tileloc[2][3] = JTP_TILE_LONG_WORM;
  tileloc[2][4] = JTP_TILE_BABY_LONG_WORM; 

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon5.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(3, 5, 1, 1, JTP_MON_LARGE_TILE_DX, JTP_MON_LARGE_TILE_DY, tileloc);

  /* Load more tall monster tiles */

  tileloc[0][0] = JTP_TILE_MUMAK; tileloc[0][1] = JTP_TILE_CLAY_GOLEM;
  tileloc[0][2] = JTP_TILE_STONE_GOLEM; tileloc[0][3] = JTP_TILE_GLASS_GOLEM;
  tileloc[0][4] = JTP_TILE_IRON_GOLEM; tileloc[0][5] = JTP_TILE_LURKER_ABOVE;
  tileloc[1][0] = JTP_TILE_TRAPPER; tileloc[1][1] = -1;
  tileloc[1][2] = -1; tileloc[1][3] = -1;
  tileloc[1][4] = -1; tileloc[1][5] = -1;
  tileloc[2][0] = -1; tileloc[2][1] = -1;
  tileloc[2][2] = -1; tileloc[2][3] = -1;
  tileloc[2][4] = -1; tileloc[2][5] = -1;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon16.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(3, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_LARGE_TILE_DY, tileloc);

  /* Load more tall and wide monster tiles */

  tileloc[0][0] = JTP_TILE_ZRUTY; tileloc[0][1] = JTP_TILE_LEOCROTTA;
  tileloc[0][2] = JTP_TILE_TITANOTHERE; tileloc[0][3] = JTP_TILE_XORN;
  tileloc[0][4] = -1; 
  tileloc[1][0] = -1; tileloc[1][1] = -1;
  tileloc[1][2] = -1; tileloc[1][3] = -1;
  tileloc[1][4] = -1; 
  tileloc[2][0] = -1; tileloc[2][1] = -1;
  tileloc[2][2] = -1; tileloc[2][3] = -1;
  tileloc[2][4] = -1; 

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon18.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(3, 5, 1, 1, JTP_MON_LARGE_TILE_DX, JTP_MON_LARGE_TILE_DY, tileloc);

  /* Load huge monster tiles */

  tileloc[0][0] = JTP_TILE_SILVER_DRAGON; tileloc[0][1] = JTP_TILE_BLACK_DRAGON;
  tileloc[0][2] = JTP_TILE_WHITE_DRAGON; 
  tileloc[1][0] = JTP_TILE_BALUCHITHERIUM; tileloc[1][1] = JTP_TILE_MASTADON;
  tileloc[1][2] = JTP_TILE_WIZARD_OF_YENDOR; 

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon14.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(2, 3, 1, 1, JTP_MON_HUGE_TILE_DX, JTP_MON_HUGE_TILE_DY, tileloc);

  /* Load huge monster tiles */

  tileloc[0][0] = JTP_TILE_RED_DRAGON; tileloc[0][1] = JTP_TILE_GREEN_DRAGON;
  tileloc[0][2] = JTP_TILE_ORANGE_DRAGON; 
  tileloc[1][0] = JTP_TILE_YELLOW_DRAGON; tileloc[1][1] = JTP_TILE_BLUE_DRAGON;
  tileloc[1][2] = JTP_TILE_GRAY_DRAGON; 

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_mon15.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(2, 3, 1, 1, JTP_MON_HUGE_TILE_DX, JTP_MON_HUGE_TILE_DY, tileloc);

  /* Tiles for the rogue level... hehehe  :)  */

  tileloc[0][0] = JTP_TILE_ROGUE_LEVEL_A; tileloc[0][1] = JTP_TILE_ROGUE_LEVEL_B;
  tileloc[0][2] = JTP_TILE_ROGUE_LEVEL_C; tileloc[0][3] = JTP_TILE_ROGUE_LEVEL_D;
  tileloc[0][4] = JTP_TILE_ROGUE_LEVEL_E; tileloc[0][5] = JTP_TILE_ROGUE_LEVEL_F;
  tileloc[1][0] = JTP_TILE_ROGUE_LEVEL_G; tileloc[1][1] = JTP_TILE_ROGUE_LEVEL_H;
  tileloc[1][2] = JTP_TILE_ROGUE_LEVEL_I; tileloc[1][3] = JTP_TILE_ROGUE_LEVEL_J;
  tileloc[1][4] = JTP_TILE_ROGUE_LEVEL_K; tileloc[1][5] = JTP_TILE_ROGUE_LEVEL_L;
  tileloc[2][0] = JTP_TILE_ROGUE_LEVEL_M; tileloc[2][1] = JTP_TILE_ROGUE_LEVEL_N;
  tileloc[2][2] = JTP_TILE_ROGUE_LEVEL_O; tileloc[2][3] = JTP_TILE_ROGUE_LEVEL_P;
  tileloc[2][4] = JTP_TILE_ROGUE_LEVEL_Q; tileloc[2][5] = JTP_TILE_ROGUE_LEVEL_R;
  tileloc[3][0] = JTP_TILE_ROGUE_LEVEL_S; tileloc[3][1] = JTP_TILE_ROGUE_LEVEL_T;
  tileloc[3][2] = JTP_TILE_ROGUE_LEVEL_U; tileloc[3][3] = JTP_TILE_ROGUE_LEVEL_V;
  tileloc[3][4] = JTP_TILE_ROGUE_LEVEL_W; tileloc[3][5] = JTP_TILE_ROGUE_LEVEL_X;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_monmisc1.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);

  /* And, of course, the last two.  */

  tileloc[0][0] = JTP_TILE_ROGUE_LEVEL_Y; tileloc[0][1] = JTP_TILE_ROGUE_LEVEL_Z;
  tileloc[0][2] = -1; tileloc[0][3] = -1;
  tileloc[0][4] = -1; tileloc[0][5] = -1;
  tileloc[1][0] = -1; tileloc[1][1] = -1;
  tileloc[1][2] = -1; tileloc[1][3] = -1;
  tileloc[1][4] = -1; tileloc[1][5] = -1;
  tileloc[2][0] = -1; tileloc[2][1] = -1;
  tileloc[2][2] = -1; tileloc[2][3] = -1;
  tileloc[2][4] = -1; tileloc[2][5] = -1;
  tileloc[3][0] = -1; tileloc[3][1] = -1;
  tileloc[3][2] = -1; tileloc[3][3] = -1;
  tileloc[3][4] = -1; tileloc[3][5] = -1;
  
  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "jtp_monmisc2.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_MON_TILE_DX, JTP_MON_TILE_DY, tileloc);
  
  /* Load the "engulfed" graphics */

  tileloc[0][0] = JTP_TILE_ENGULF_FIRE_VORTEX; tileloc[0][1] = JTP_TILE_ENGULF_FOG_CLOUD;
  tileloc[0][2] = JTP_TILE_ENGULF_AIR_ELEMENTAL;
  tileloc[1][0] = JTP_TILE_ENGULF_STEAM_VORTEX; tileloc[1][1] = JTP_TILE_ENGULF_PURPLE_WORM;
  tileloc[1][2] = JTP_TILE_ENGULF_JUIBLEX;
  
  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "engulf.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(2, 3, 1, 1, 218, 234, tileloc);

  /* Load the other engulfed graphics */

  tileloc[0][0] = JTP_TILE_ENGULF_OCHRE_JELLY; tileloc[0][1] = JTP_TILE_ENGULF_LURKER_ABOVE;
  tileloc[0][2] = JTP_TILE_ENGULF_TRAPPER;
  tileloc[1][0] = JTP_TILE_ENGULF_DUST_VORTEX; tileloc[1][1] = JTP_TILE_ENGULF_ICE_VORTEX;
  tileloc[1][2] = JTP_TILE_ENGULF_ENERGY_VORTEX;
  
  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "engulf2.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(2, 3, 1, 1, 218, 234, tileloc);
  
  
  /* questionmark tiles for warning and unseen monsters */

  tileloc[0][0] = JTP_TILE_WARNLEV_6; tileloc[0][1] = JTP_TILE_WARNLEV_5;
  tileloc[0][2] = JTP_TILE_WARNLEV_4; tileloc[0][3] = JTP_TILE_WARNLEV_3;
  tileloc[0][4] = JTP_TILE_WARNLEV_2; tileloc[0][5] = JTP_TILE_WARNLEV_1;
  tileloc[1][0] = -1; tileloc[1][1] = -1;
  tileloc[1][2] = -1; tileloc[1][3] = -1;
  tileloc[1][4] = -1; tileloc[1][5] = -1;
  tileloc[2][0] = -1; tileloc[2][1] = -1;
  tileloc[2][2] = -1; tileloc[2][3] = JTP_TILE_INVISIBLE_MONSTER;
  tileloc[2][4] = -1; tileloc[2][5] = -1;
  tileloc[3][0] = -1; tileloc[3][1] = -1;
  tileloc[3][2] = -1; tileloc[3][3] = -1;
  tileloc[3][4] = -1; tileloc[3][5] = -1;

  filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "unknown.pcx");
  all_ok &= jtp_load_PCX(0, 0, filename, FALSE) != JTP_PCX_FAILURE;
  setlastpcx(filename);
  free(filename);
  jtp_get_tile_group(4, 6, 1, 1, JTP_CMAP_TILE_DX, JTP_CMAP_TILE_DY, tileloc);

  /* We autogenerate this as needed in jtp_draw_level */
  jtp_tiles[JTP_TILE_PLAYER_INVIS] = NULL;

  jtp_tiles[JTP_TILE_NONE] = NULL;
  
  /* Initialize map */

  jtp_map_width = JTP_MAP_WIDTH;
  jtp_map_height = JTP_MAP_HEIGHT;

  jtp_map_back = (jtp_tilenumber **)malloc(JTP_MAP_HEIGHT * sizeof(jtp_tilenumber *));
  jtp_map_obj  = (jtp_tilenumber **)malloc(JTP_MAP_HEIGHT * sizeof(jtp_tilenumber *));
  jtp_map_trap = (jtp_tilenumber **)malloc(JTP_MAP_HEIGHT * sizeof(jtp_tilenumber *));
  jtp_map_furniture = (jtp_tilenumber **)malloc(JTP_MAP_HEIGHT * sizeof(jtp_tilenumber *));
  jtp_map_specialeff = (jtp_tilenumber **)malloc(JTP_MAP_HEIGHT * sizeof(jtp_tilenumber *));
  jtp_map_mon  = (jtp_tilenumber **)malloc(JTP_MAP_HEIGHT * sizeof(jtp_tilenumber *));
  jtp_map_specialattr = (unsigned int **)malloc(JTP_MAP_HEIGHT * sizeof(unsigned int**));
  jtp_map_deco = (unsigned char **)malloc(JTP_MAP_HEIGHT*sizeof(unsigned char *));
  
  jtp_maptile_wall = (jtp_wall_style **)malloc(JTP_MAP_HEIGHT*sizeof(jtp_wall_style *));
  jtp_maptile_floor_edge = (jtp_floor_edge_style **)malloc(JTP_MAP_HEIGHT*sizeof(jtp_floor_edge_style *));

  jtp_map_light = (int **)malloc(JTP_MAP_HEIGHT*sizeof(int *));
  jtp_map_tile_is_dark = (char **)malloc(JTP_MAP_HEIGHT*sizeof(char *));
  jtp_room_indices = (char **)malloc(JTP_MAP_HEIGHT*sizeof(char *));

  if ((!jtp_map_back) || (!jtp_map_obj) || (!jtp_map_trap) ||
      (!jtp_map_furniture) || (!jtp_map_mon) || (!jtp_map_specialeff) ||
      (!jtp_maptile_wall) || (!jtp_maptile_floor_edge) || (!jtp_map_deco) ||
      (!jtp_map_light) || (!jtp_map_specialattr) || (!jtp_room_indices))
  {
    OOM(1);
  }
  
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
  {
    jtp_map_back[i] = (jtp_tilenumber *)malloc(JTP_MAP_WIDTH * sizeof(jtp_tilenumber));
    jtp_map_obj[i]  = (jtp_tilenumber *)malloc(JTP_MAP_WIDTH * sizeof(jtp_tilenumber));
    jtp_map_trap[i] = (jtp_tilenumber *)malloc(JTP_MAP_WIDTH * sizeof(jtp_tilenumber));
    jtp_map_furniture[i] = (jtp_tilenumber *)malloc(JTP_MAP_WIDTH * sizeof(jtp_tilenumber));
    jtp_map_specialeff[i] = (jtp_tilenumber *)malloc(JTP_MAP_WIDTH * sizeof(jtp_tilenumber));
    jtp_map_mon[i]  = (jtp_tilenumber *)malloc(JTP_MAP_WIDTH * sizeof(jtp_tilenumber));
    jtp_map_specialattr[i] = (unsigned int*)malloc(JTP_MAP_WIDTH * sizeof(unsigned int));
    jtp_map_deco[i] = (unsigned char *)malloc(JTP_MAP_WIDTH * sizeof(unsigned char));

    jtp_maptile_wall[i] = (jtp_wall_style *)malloc(JTP_MAP_WIDTH*sizeof(jtp_wall_style));
    jtp_maptile_floor_edge[i] = (jtp_floor_edge_style *)malloc(JTP_MAP_WIDTH*sizeof(jtp_floor_edge_style));

    jtp_map_light[i] = (int *)malloc(JTP_MAP_WIDTH*sizeof(int));
    jtp_map_tile_is_dark[i] = (char *)malloc(JTP_MAP_WIDTH*sizeof(char));
    jtp_room_indices[i] = (char *)malloc(JTP_MAP_WIDTH*sizeof(char));

    if ((!jtp_map_back[i]) || (!jtp_map_obj[i]) || (!jtp_map_trap[i]) ||
        (!jtp_map_furniture[i]) || (!jtp_map_mon[i]) || (!jtp_map_specialeff[i]) ||
        (!jtp_maptile_wall[i]) || (!jtp_maptile_floor_edge[i]) || (!jtp_map_deco[i]) ||
        (!jtp_map_light[i]) || (!jtp_map_specialattr[i]) || (!jtp_room_indices[i]))
    {
      OOM(1);
    }
    
    for (j = 0; j < JTP_MAP_WIDTH; j++)
    {
      jtp_map_back[i][j] = JTP_TILE_UNMAPPED_AREA;
      jtp_map_obj[i][j] = JTP_TILE_NONE;
      jtp_map_trap[i][j] = JTP_TILE_NONE;
      jtp_map_furniture[i][j] = JTP_TILE_NONE;
      jtp_map_specialeff[i][j] = JTP_TILE_NONE;
      jtp_map_mon[i][j] = JTP_TILE_NONE;
      jtp_map_specialattr[i][j] = 0;
      jtp_map_deco[i][j] = 0;
      jtp_map_light[i][j] = JTP_AMBIENT_LIGHT;
      jtp_room_indices[i][j] = 0;
      jtp_map_tile_is_dark[i][j] = 1;
      jtp_clear_walls(i,j);
    }
  }

  /* Initialize random number generator */
  srand(time(NULL));

  /* Initialize light sources */
  jtp_nlights = 1;  /* Hero carries a small light */
  
  jtp_move_length = 0;
  jtp_is_backpack_shortcut_active = 0;

  jtp_map_x = JTP_MAP_WIDTH/2;
  jtp_map_y = JTP_MAP_HEIGHT/2;
  jtp_map_changed = 1;

  /* Clean up */
  for (i = 0; i < 20; i++)
    free(tileloc[i]);
  free(tileloc);
  
  return all_ok;
}


static void jtp_init_filenames(void)
{
  /* Get starting directory, and save it for reference */
#ifdef CHDIR
	char hackdir[1024];
	getcwd(hackdir, sizeof(hackdir));
	jtp_game_path = jtp_strdup(hackdir);
#else
	jtp_game_path = jtp_strdup(hackdir);
#endif
}


int jtp_init_graphics(void)
{
  int i;
  int screen_width, screen_height;
  int all_ok = TRUE;
  unsigned char *image;
  
  jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "Initializing filenames\n");
  jtp_init_filenames();

  jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "Reading Vulture's options\n");
  /* Read options file */
  jtp_read_options(&screen_width, &screen_height);
  /*
   * the size of the logo bitmap;
   * bad things happen if screen size is less than this; FIXME
   */
  if (screen_width < 800)
    screen_width = 800;
  if (screen_height < 600)
    screen_height = 600;

  jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "Initializing screen buffer\n");
  /* Initialize screen and calculate some often used coordinates */
  jtp_init_screen(screen_width, screen_height);
  jtp_statusbar_x = (jtp_screen.width-JTP_STATUSBAR_WIDTH)/2;
  jtp_statusbar_y = jtp_screen.height-JTP_STATUSBAR_HEIGHT;
  jtp_map_center_x = jtp_screen.width/2;
  jtp_map_center_y = (jtp_screen.height-JTP_STATUSBAR_HEIGHT)/2;

  jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "Initializing fonts\n");
  /* Load fonts */
  jtp_fonts = (jtp_font *)calloc(JTP_MAX_FONTS, sizeof(jtp_font));
  all_ok &= jtp_load_font(JTP_FILENAME_FONT_SMALL, JTP_FONT_SMALL);
  all_ok &= jtp_load_font(JTP_FILENAME_FONT_LARGE, JTP_FONT_LARGE);

  /* Load window style graphics */
  image = jtp_load_graphic(NULL, JTP_FILENAME_WINDOW_STYLE, FALSE);
  if (image == NULL)
  {
    all_ok = FALSE;
  } else
  {
    jtp_defwin.corner_tl = jtp_get_img_src(1, 1, 23, 23, image);
    jtp_defwin.border_top = jtp_get_img_src(27, 1, 57, 23, image);
    jtp_defwin.corner_tr = jtp_get_img_src(61, 1, 84, 23, image);
    jtp_defwin.border_left = jtp_get_img_src(1, 27, 23, 54, image);
    /* jtp_defwin.center = jtp_get_img_src(27, 27, 57, 54, image); */ /* This is the small graphic... */
    jtp_defwin.center = jtp_get_img_src(141, 1, 238, 168, image);  /* But let's use this big one. */
    jtp_defwin.border_right = jtp_get_img_src(61, 27, 84, 54, image);
    jtp_defwin.corner_bl = jtp_get_img_src(1, 58, 23, 82, image);
    jtp_defwin.border_bottom = jtp_get_img_src(27, 58, 57, 82, image);
    jtp_defwin.corner_br = jtp_get_img_src(61, 58, 84, 82, image);
    jtp_defwin.checkbox_off = jtp_get_img_src(1, 107, 17, 123, image);
    jtp_defwin.checkbox_on = jtp_get_img_src(21, 107, 37, 123, image);
    jtp_defwin.radiobutton_off = jtp_get_img_src(41, 107, 57, 123, image);
    jtp_defwin.radiobutton_on = jtp_get_img_src(61, 107, 77, 123, image);
    jtp_defwin.scrollbar = jtp_get_img_src(81, 107, 97, 123, image);
    jtp_defwin.scrollbutton_down = jtp_get_img_src(101, 107, 117, 123, image);
    jtp_defwin.scrollbutton_up = jtp_get_img_src(121, 107, 137, 123, image);
    jtp_defwin.scroll_indicator = jtp_get_img_src(1, 127, 17, 154, image);
    jtp_defwin.direction_arrows = jtp_get_img_src(242, 1, 576, 134, image);
    jtp_defwin.invarrow_left = jtp_get_img_src(242, 138, 364, 175, image);
    jtp_defwin.invarrow_right = jtp_get_img_src(368, 138, 490, 175, image);
    free(image);
  }
  
  /* Load status bar */
  image = jtp_load_graphic(NULL, JTP_FILENAME_STATUS_BAR, FALSE);
  if (image == NULL)
  {
    all_ok = FALSE;
  } else
  {
    jtp_statusbar = image;
  }
  
  /* Load map parchment */
  image = jtp_load_graphic(NULL, JTP_FILENAME_MAP_PARCHMENT, FALSE);
  if (image == NULL)
  {
    all_ok = FALSE;
  } else
  {
    jtp_map_parchment_center = jtp_get_img_src(21, 18, 606, 463, image);
    jtp_map_parchment_top = jtp_get_img_src(0, 0, 639, 17, image);
    jtp_map_parchment_bottom = jtp_get_img_src(0, 464, 639, 479, image);
    jtp_map_parchment_left = jtp_get_img_src(0, 0, 20, 479, image);
    jtp_map_parchment_right = jtp_get_img_src(601, 0, 639, 479, image);
    free(image);
  }

  /* Load backpack */
  image = jtp_load_graphic(NULL, JTP_FILENAME_BACKPACK, FALSE);
  if (image == NULL)
  {
    all_ok = FALSE;
  } else
  {
    jtp_backpack_center = jtp_get_img_src(23, 15, 620, 405, image);
    jtp_backpack_top = jtp_get_img_src(0, 0, 639, 14, image);
    jtp_backpack_bottom = jtp_get_img_src(0, 406, 639, 479, image);
    jtp_backpack_left = jtp_get_img_src(0, 0, 22, 479, image);
    jtp_backpack_right = jtp_get_img_src(621, 0, 639, 479, image);
    free(image);
  }

  /* Load map symbols */
  image = jtp_load_graphic(NULL, JTP_FILENAME_MAP_SYMBOLS, FALSE);
  if (image == NULL)
  {
    all_ok = FALSE;
  } else
  {
    for (i = 0; i < JTP_MAX_MAP_SYMBOLS; i++)
      jtp_map_symbols[i] = jtp_get_img_src(1 + 10 * i, 1, 7 + 10 * i, 14, image);
    free(image);
  }

  /* Load mouse cursors */
  image = jtp_load_graphic(NULL, JTP_FILENAME_MOUSE_CURSORS, TRUE);
  if (image == NULL)
  {
    all_ok = FALSE;
  } else
  {
    jtp_mcursor[JTP_CURSOR_NORMAL] = jtp_get_mcursor(image, 1, 1, 17, 24);
    jtp_mcursor[JTP_CURSOR_SCROLLRIGHT] = jtp_get_mcursor(image, 21, 1, 40, 23);
    jtp_mcursor[JTP_CURSOR_SCROLLLEFT] = jtp_get_mcursor(image, 44, 1, 63, 23);
    jtp_mcursor[JTP_CURSOR_SCROLLUP] = jtp_get_mcursor(image, 67, 1, 89, 20);
    jtp_mcursor[JTP_CURSOR_SCROLLDOWN] = jtp_get_mcursor(image, 93, 1, 115, 20);
    jtp_mcursor[JTP_CURSOR_TARGET_GREEN] = jtp_get_mcursor(image, 119, 1, 169, 26);
    jtp_mcursor[JTP_CURSOR_TARGET_RED] = jtp_get_mcursor(image, 173, 1, 223, 26);
    jtp_mcursor[JTP_CURSOR_TARGET_INVALID] = jtp_get_mcursor(image, 227, 1, 273, 26);
    jtp_mcursor[JTP_CURSOR_TARGET_HELP] = jtp_get_mcursor(image, 1, 30, 51, 79);
    jtp_mcursor[JTP_CURSOR_HOURGLASS] = jtp_get_mcursor(image, 277, 1, 306, 33);
    jtp_mcursor[JTP_CURSOR_OPENDOOR] = jtp_get_mcursor(image, 310, 1, 342, 30);
    jtp_mcursor[JTP_CURSOR_STAIRS] = jtp_get_mcursor(image, 346, 1, 383, 35);
    jtp_mcursor[JTP_CURSOR_GOBLET] = jtp_get_mcursor(image, 312, 34, 336, 68);
    /* these also contain the regular game palette */
    memcpy(jtp_game_colors, jtp_colors, sizeof(jtp_colors));
    free(image);
  }

  /* Set message shading */
  for (i = 0; i < JTP_MAX_MESSAGE_COLORS; i++)
    jtp_message_colors[i] = 20 + i/2;
  jtp_message_colors[0] = JTP_COLOR_TEXT;
  jtp_message_colors[1] = 17;

  /* Set warning colors */
  jtp_warn_colors[JTP_WARN_NONE] = JTP_COLOR_TEXT;
  jtp_warn_colors[JTP_WARN_NORMAL] = 10;
  jtp_warn_colors[JTP_WARN_MORE] = 84;
  jtp_warn_colors[JTP_WARN_ALERT] = 215;
  jtp_warn_colors[JTP_WARN_CRITICAL] = 222;
  
  /* Initialize the isometric tiles */
#if 0
	/* can't be done now, because the object descriptions are not yet set up */
	jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "Initializing tile conversion.\n");
	jtp_init_glyph_tiles(); /* Initialize glyph-tile correspondence tables just before game starts */
#endif
  jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "Loading tile graphics.\n");
  all_ok &= jtp_init_tilegraphics();  /* Initialize tile bitmaps */
  jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "Tile graphics loaded.\n");
 
  /* jtp_template_conversion(); */
 
  
  jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "Vulture's window system ready.\n");

  jtp_clear_screen();  /* Clear the screen buffer */
  /* Enter graphics mode */
  jtp_enter_graphics_mode(&jtp_screen); 
  jtp_blankpal(0,255); /* Set palette to all black */
  jtp_set_draw_region(0, 0, jtp_screen.width, jtp_screen.height);

  /* 
   * Set regular game palette. If any function changes the palette
   * (fade-ins, etc.), it is expected to restore the palette before
   * it returns.
   */
  memcpy(jtp_colors, jtp_game_colors, sizeof(jtp_colors));
  jtp_clear_screen();
  jtp_refresh(&jtp_screen);
  jtp_updatepal(0, 255);
  jtp_game_palette_set = 1;

  return all_ok;
}
