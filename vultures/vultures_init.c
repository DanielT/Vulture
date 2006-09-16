/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000				  */

#include <unistd.h>
#include "vultures_mou.h"
#include "vultures_win.h"
#include "vultures_map.h"
#include "vultures_gametiles.h"
#include "vultures_gra.h"
#include "vultures_gen.h"
#include "vultures_sdl.h"
#include "vultures_gfl.h"
#include "vultures_keys.h"
#include "vultures_txt.h"
#include "vultures_init.h"
#include "vultures_nhplayerselection.h"
#include "vultures_main.h"

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
#define JTP_FILENAME_NETHACK_LOGO         "nh_jtp_1"
#define JTP_FILENAME_MAP_SYMBOLS          "nh_tiles"
#endif
#ifdef VULTURESCLAW
#define JTP_FILENAME_NETHACK_LOGO         "se_jtp_1"
#define JTP_FILENAME_MAP_SYMBOLS          "se_tiles"
#endif
#define JTP_FILENAME_CHARACTER_GENERATION "chargen2"
#define JTP_FILENAME_FONT_SMALL           "ttipchr1"
#define JTP_FILENAME_FONT_LARGE           "menuchr2"
#define JTP_FILENAME_WINDOW_STYLE         "jtp_win1"
#define JTP_FILENAME_STATUS_BAR           "jtp_st10"
#define JTP_FILENAME_MAP_PARCHMENT        "jtp_mwi4"
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
#define JTP_FILENAME_ENDING_ESCAPED       "quitgame"


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
          jtp_external_midi_player_command = malloc(strlen(tok + 1) + 1);
          strcpy(jtp_external_midi_player_command, tok + 1);
          /* Remove end-of-line from the string */
          trimright(jtp_external_midi_player_command);
        }
        else if (!strncmp(tempbuffer, "mp3_player", tok - tempbuffer) ||
          !strncmp(tempbuffer, "linux_mp3_player", tok - tempbuffer))
        {
          jtp_external_mp3_player_command = malloc(strlen(tok + 1) + 1);
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
        jtp_event_sounds =  realloc(jtp_event_sounds, jtp_n_event_sounds*sizeof(jtp_event_sound *));
        jtp_event_sounds[jtp_n_event_sounds-1] =  malloc(sizeof(jtp_event_sound));
        (jtp_event_sounds[jtp_n_event_sounds-1])->filename = malloc(JTP_MAX_FILENAME_LENGTH*sizeof(char));

        (jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern = malloc(JTP_MAX_FILENAME_LENGTH*sizeof(char));
        memcpy((jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern, tempbuffer+1, tok-tempbuffer-1);
        (jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern[tok-tempbuffer-1] = '\0';

        /* Check for a background music event */
        if (!strcmp((jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern, "nhfe_music_background"))
	{
          jtp_n_background_songs++;
          free((jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern);
          (jtp_event_sounds[jtp_n_event_sounds-1])->searchpattern = malloc(strlen("nhfe_music_background")+4);
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

  vultures_player_selection();
  
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
  tempbuffer = malloc(1024);
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
          scene_images[nScenes-1] = malloc((i + 1) * sizeof(*tempbuffer));
          if (!scene_images[nScenes - 1])
          {
            OOM(1);
          }
          strcpy(scene_images[nScenes - 1], tempbuffer + 1);
          dot = strrchr(scene_images[nScenes - 1], '.');
          if (dot != NULL)
          {
            if (strcmp(dot, ".png") == 0)
            {
              *dot = '\0';
            } else
            {
              jtp_write_log_message(JTP_LOG_NOTE, NULL, 0, "scene image %s not png?", scene_images[nScenes - 1]);
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
          subtitles[nScenes - 1][subtitle_rows[nScenes - 1] - 1] = malloc(strlen(tempbuffer + i) + 1);
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



void jtp_init_glyph_tiles(void)
{
    int i;
    jtp_cmaptiles = malloc(MAXPCHARS*sizeof(int));
    jtp_engulfmap = malloc(NUMMONS*sizeof(int));

    if (!jtp_cmaptiles || !jtp_engulfmap)
        OOM(1);


    for (i = 0; i < NUMMONS; i++)
        jtp_engulfmap[i] = V_TILE_NONE;

    jtp_engulfmap[PM_OCHRE_JELLY]   = V_TILE_ENGULF_OCHRE_JELLY;
    jtp_engulfmap[PM_LURKER_ABOVE]  = V_TILE_ENGULF_LURKER_ABOVE;
    jtp_engulfmap[PM_TRAPPER]       = V_TILE_ENGULF_TRAPPER;
    jtp_engulfmap[PM_PURPLE_WORM]   = V_TILE_ENGULF_PURPLE_WORM;
    jtp_engulfmap[PM_DUST_VORTEX]   = V_TILE_ENGULF_DUST_VORTEX;
    jtp_engulfmap[PM_ICE_VORTEX]    = V_TILE_ENGULF_ICE_VORTEX;
    jtp_engulfmap[PM_ENERGY_VORTEX] = V_TILE_ENGULF_ENERGY_VORTEX;
    jtp_engulfmap[PM_STEAM_VORTEX]  = V_TILE_ENGULF_STEAM_VORTEX;
    jtp_engulfmap[PM_FIRE_VORTEX]   = V_TILE_ENGULF_FIRE_VORTEX;
    jtp_engulfmap[PM_FOG_CLOUD]     = V_TILE_ENGULF_FOG_CLOUD;
    jtp_engulfmap[PM_AIR_ELEMENTAL] = V_TILE_ENGULF_AIR_ELEMENTAL;
    jtp_engulfmap[PM_JUIBLEX]      = V_TILE_ENGULF_JUIBLEX;


    jtp_cmaptiles[S_stone] = V_TILE_UNMAPPED_AREA;
    jtp_cmaptiles[S_vwall] = V_TILE_WALL_GENERIC;
    jtp_cmaptiles[S_hwall] = V_TILE_WALL_GENERIC;
    jtp_cmaptiles[S_tlcorn] = V_TILE_WALL_GENERIC;
    jtp_cmaptiles[S_trcorn] = V_TILE_WALL_GENERIC;
    jtp_cmaptiles[S_blcorn] = V_TILE_WALL_GENERIC;
    jtp_cmaptiles[S_brcorn] = V_TILE_WALL_GENERIC;
    jtp_cmaptiles[S_crwall] = V_TILE_WALL_GENERIC;
    jtp_cmaptiles[S_tuwall] = V_TILE_WALL_GENERIC;
    jtp_cmaptiles[S_tdwall] = V_TILE_WALL_GENERIC;
    jtp_cmaptiles[S_tlwall] = V_TILE_WALL_GENERIC;
    jtp_cmaptiles[S_trwall] = V_TILE_WALL_GENERIC;
    jtp_cmaptiles[S_ndoor] = V_TILE_DOOR_WOOD_BROKEN;
    jtp_cmaptiles[S_vodoor] = V_TILE_VDOOR_WOOD_OPEN;
    jtp_cmaptiles[S_hodoor] = V_TILE_HDOOR_WOOD_OPEN;
    jtp_cmaptiles[S_vcdoor] = V_TILE_VDOOR_WOOD_CLOSED;
    jtp_cmaptiles[S_hcdoor] = V_TILE_HDOOR_WOOD_CLOSED;
    jtp_cmaptiles[S_room] = V_TILE_FLOOR_COBBLESTONE;
    jtp_cmaptiles[S_corr] = V_TILE_FLOOR_ROUGH;
    jtp_cmaptiles[S_upstair] = V_TILE_STAIRS_UP;
    jtp_cmaptiles[S_dnstair] = V_TILE_STAIRS_DOWN;
    jtp_cmaptiles[S_fountain] = V_TILE_FOUNTAIN;
    jtp_cmaptiles[S_altar] = V_TILE_ALTAR;
    jtp_cmaptiles[S_teleportation_trap] = V_TILE_TRAP_TELEPORTER;
    jtp_cmaptiles[S_tree] = V_TILE_TREE;
    jtp_cmaptiles[S_cloud] = V_TILE_CLOUD;
    jtp_cmaptiles[S_air] = V_TILE_FLOOR_AIR;
    jtp_cmaptiles[S_grave] = V_TILE_GRAVE;
    jtp_cmaptiles[S_sink] = V_TILE_SINK;
    jtp_cmaptiles[S_bear_trap] = V_TILE_TRAP_BEAR;
    jtp_cmaptiles[S_rust_trap] = V_TILE_TRAP_WATER;
    jtp_cmaptiles[S_pit] = V_TILE_TRAP_PIT;
    jtp_cmaptiles[S_hole] = V_TILE_TRAP_PIT;
    jtp_cmaptiles[S_trap_door] = V_TILE_TRAP_DOOR;
    jtp_cmaptiles[S_water] = V_TILE_FLOOR_WATER;
    jtp_cmaptiles[S_pool] = V_TILE_FLOOR_WATER;
    jtp_cmaptiles[S_ice] = V_TILE_FLOOR_ICE;
    jtp_cmaptiles[S_lava] = V_TILE_FLOOR_LAVA;
    jtp_cmaptiles[S_throne] = V_TILE_THRONE;
    jtp_cmaptiles[S_bars] = V_TILE_BARS;
    jtp_cmaptiles[S_upladder] = V_TILE_LADDER_UP;
    jtp_cmaptiles[S_dnladder] = V_TILE_LADDER_DOWN;
    jtp_cmaptiles[S_arrow_trap] = V_TILE_TRAP_ARROW;
    jtp_cmaptiles[S_rolling_boulder_trap] = V_TILE_ROLLING_BOULDER_TRAP;
    jtp_cmaptiles[S_sleeping_gas_trap] = V_TILE_GAS_TRAP;
    jtp_cmaptiles[S_fire_trap] = V_TILE_TRAP_FIRE;
    jtp_cmaptiles[S_web] = V_TILE_WEB_TRAP;
    jtp_cmaptiles[S_statue_trap] = OBJECT_TO_VTILE(STATUE);
    jtp_cmaptiles[S_anti_magic_trap] = V_TILE_TRAP_ANTI_MAGIC;
    jtp_cmaptiles[S_polymorph_trap] = V_TILE_TRAP_POLYMORPH;
    jtp_cmaptiles[S_vbeam] = V_TILE_ZAP_VERTICAL;
    jtp_cmaptiles[S_hbeam] = V_TILE_ZAP_HORIZONTAL;
    jtp_cmaptiles[S_lslant] = V_TILE_ZAP_SLANT_LEFT;
    jtp_cmaptiles[S_rslant] = V_TILE_ZAP_SLANT_RIGHT;
    jtp_cmaptiles[S_explode1] = V_TILE_EXPLOSION_NORTHWEST;
    jtp_cmaptiles[S_explode2] = V_TILE_EXPLOSION_NORTH;
    jtp_cmaptiles[S_explode3] = V_TILE_EXPLOSION_NORTHEAST;
    jtp_cmaptiles[S_explode4] = V_TILE_EXPLOSION_WEST;
    jtp_cmaptiles[S_explode5] = V_TILE_EXPLOSION_CENTER;
    jtp_cmaptiles[S_explode6] = V_TILE_EXPLOSION_EAST;
    jtp_cmaptiles[S_explode7] = V_TILE_EXPLOSION_SOUTHWEST;
    jtp_cmaptiles[S_explode8] = V_TILE_EXPLOSION_SOUTH;
    jtp_cmaptiles[S_explode9] = V_TILE_EXPLOSION_SOUTHEAST;
    jtp_cmaptiles[S_litcorr] = V_TILE_FLOOR_ROUGH_LIT;
    jtp_cmaptiles[S_ss1] = V_TILE_RESIST_SPELL_1;
    jtp_cmaptiles[S_ss2] = V_TILE_RESIST_SPELL_2;
    jtp_cmaptiles[S_ss3] = V_TILE_RESIST_SPELL_3;
    jtp_cmaptiles[S_ss4] = V_TILE_RESIST_SPELL_4;
    jtp_cmaptiles[S_dart_trap] = V_TILE_DART_TRAP;
    jtp_cmaptiles[S_falling_rock_trap] = V_TILE_FALLING_ROCK_TRAP;
    jtp_cmaptiles[S_squeaky_board] = V_TILE_SQUEAKY_BOARD;
    jtp_cmaptiles[S_land_mine] = V_TILE_LAND_MINE;
    jtp_cmaptiles[S_magic_portal] = V_TILE_MAGIC_PORTAL;
    jtp_cmaptiles[S_spiked_pit] = V_TILE_SPIKED_PIT;
    jtp_cmaptiles[S_hole] = V_TILE_HOLE;
    jtp_cmaptiles[S_level_teleporter] = V_TILE_LEVEL_TELEPORTER;
    jtp_cmaptiles[S_magic_trap] = V_TILE_MAGIC_TRAP;
    jtp_cmaptiles[S_digbeam] = V_TILE_DIGBEAM;
    jtp_cmaptiles[S_flashbeam] = V_TILE_FLASHBEAM;
    jtp_cmaptiles[S_boomleft] = V_TILE_BOOMLEFT;
    jtp_cmaptiles[S_boomright] = V_TILE_BOOMRIGHT;
    jtp_cmaptiles[S_hcdbridge] = V_TILE_HCDBRIDGE;
    jtp_cmaptiles[S_vcdbridge] = V_TILE_VCDBRIDGE;
    jtp_cmaptiles[S_hodbridge] = V_TILE_HODBRIDGE;
    jtp_cmaptiles[S_vodbridge] = V_TILE_VODBRIDGE;


    /* We can now shuffle object tiles to match their descriptions
     * because savegame loading should have happened when we reach here
     * This prevents visual identification of objects
     * For example a potion of healing will no longer be purple-red every time*/
    jtp_tile ** obj_by_description = malloc(NUM_OBJECTS * sizeof(jtp_tile*));
    for (i = 0; i < NUM_OBJECTS; i++)
        obj_by_description[objects[i].oc_descr_idx] = jtp_tiles[OBJECT_TO_VTILE(objects[i].oc_name_idx)];

    for (i = 0; i < NUM_OBJECTS; i++)
        jtp_tiles[OBJECT_TO_VTILE(i)] = obj_by_description[i];

    free(obj_by_description);


    jtp_tile_conversion_initialized = 1;
}


char *jtp_make_filename(const char *subdir1, const char *subdir2, const char *name)
{
    char *filename;

    filename = malloc(strlen(jtp_game_path) + 1 +
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
 * Returns the buffer from jtp_load_image_buf
 * (width & height encoded in first 4 bytes)
 */
unsigned char *jtp_load_graphic(const char *subdir, const char *name, int load_palette)
{
    unsigned char *image;
    char namebuf[128];
    int fsize;
    char * filename;
    FILE * fp;
    char * srcbuf;

    strcat(strcpy(namebuf, name), ".png");
    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, subdir, namebuf);
    if (filename == NULL)
        OOM(1);

    fp = fopen(filename, "rb");
    free(filename);
    if (!fp)
    	return NULL;

    // obtain file size.
    fseek(fp , 0 , SEEK_END);
    fsize = ftell(fp);
    rewind(fp);

    srcbuf = malloc(fsize);
    if (!srcbuf)
        return 0;

    fread(srcbuf, fsize, 1, fp);
    fclose(fp);

    image = vultures_load_image(srcbuf, fsize, load_palette);

    free(srcbuf);

    return image;
}



static unsigned char *jtp_init_shades(char *fname)
{
 int i, j;
 FILE * f;
 unsigned char *temp1;

 f = fopen(fname, "rb");
 if (!f) return(NULL);
 
 temp1 = malloc(JTP_MAX_SHADES*256*sizeof(unsigned char));
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



static int jtp_load_gametiles(void)
{
    int i, fsize;
    char * filename;
    FILE * fp;
    char * srcbuf;

    filename = jtp_make_filename(JTP_GRAPHICS_DIRECTORY, NULL, "gametiles.bin");
    fp = fopen(filename, "rb");
    free(filename);
    
    if (!fp)
        panic("FATAL: cannot open gametiles.bin\n");

    // obtain file size.
    fseek(fp , 0 , SEEK_END);
    fsize = ftell(fp);
    rewind(fp);

    srcbuf = malloc(fsize);
    if (!srcbuf)
        return 0;

    fread(srcbuf, fsize, 1, fp);

    jtp_tiles = malloc(GAMETILECOUNT * sizeof(jtp_tile *));
    if (!jtp_tiles)
    {
        free(srcbuf);
        return 0;
    }

    /* load tiles */
    for (i = 0; i < GAMETILECOUNT; i++)
    {
        jtp_tiles[i] = NULL;

        if (vultures_gametiles[i].data_len == 0)
            continue;

        jtp_tiles[i] = malloc(sizeof(jtp_tile));
        if (!jtp_tiles[i])
            continue;

        jtp_tiles[i]->graphic = vultures_load_image(&srcbuf[vultures_gametiles[i].file_offset],
                                                    vultures_gametiles[i].data_len, (i==CURTILEOFFSET));

        jtp_tiles[i]->xmod = vultures_gametiles[i].hs_x;
        jtp_tiles[i]->ymod = vultures_gametiles[i].hs_y;
    }

    /* set pointers */
    for (i = 0; i < GAMETILECOUNT; i++)
    {
        if (jtp_tiles[i] || vultures_gametiles[i].ptr == -1)
            continue;

        jtp_tiles[i] = jtp_tiles[vultures_gametiles[i].ptr];
    }

    /* Mouse cursors are usually accessed via jtp_mcursor... */
    jtp_mcursor = &jtp_tiles[CURTILEOFFSET];

    /* we loaded the tile palette onto the screen with the first tile; save it */
    memcpy(jtp_game_colors, jtp_colors, sizeof(jtp_colors));

    free(srcbuf);
    return TRUE;
}



static int jtp_init_tilegraphics(void)
{
  int i, j;
  int all_ok = TRUE;
  char *filename;

  filename = jtp_make_filename(JTP_CONFIG_DIRECTORY, NULL, JTP_FILENAME_SHADING_TABLE);
  jtp_shade = jtp_init_shades(filename);
  free(filename);
  all_ok &= jtp_shade != NULL;
  
  /* select wall style */
  switch (jtp_wall_display_style)
  {
    case JTP_WALL_DISPLAY_STYLE_FULL:
      walltiles = (struct walls*)walls_full; break;
    case JTP_WALL_DISPLAY_STYLE_HALF_HEIGHT:
      walltiles = (struct walls*)walls_half; break;
    case JTP_WALL_DISPLAY_STYLE_TRANSPARENT:
      walltiles = (struct walls*)walls_trans; break;
    default:
      walltiles = (struct walls*)walls_half; break;
  }
  
  /* load game tiles */
  all_ok &= jtp_load_gametiles();

  
  /* Initialize map */

  jtp_map_width = JTP_MAP_WIDTH;
  jtp_map_height = JTP_MAP_HEIGHT;

  jtp_map_glyph       = malloc(JTP_MAP_HEIGHT * sizeof(int *));
  jtp_map_back        = malloc(JTP_MAP_HEIGHT * sizeof(int *));
  jtp_map_obj         = malloc(JTP_MAP_HEIGHT * sizeof(int *));
  jtp_map_trap        = malloc(JTP_MAP_HEIGHT * sizeof(int *));
  jtp_map_furniture   = malloc(JTP_MAP_HEIGHT * sizeof(int *));
  jtp_map_specialeff  = malloc(JTP_MAP_HEIGHT * sizeof(int *));
  jtp_map_mon         = malloc(JTP_MAP_HEIGHT * sizeof(int *));
  jtp_map_specialattr = malloc(JTP_MAP_HEIGHT * sizeof(unsigned int**));
  jtp_map_deco        = malloc(JTP_MAP_HEIGHT * sizeof(unsigned char *));
  
  jtp_maptile_wall       = malloc(JTP_MAP_HEIGHT*sizeof(jtp_wall_style *));
  jtp_maptile_floor_edge = malloc(JTP_MAP_HEIGHT*sizeof(jtp_floor_edge_style *));

  jtp_map_light        = malloc(JTP_MAP_HEIGHT*sizeof(int *));
  jtp_map_tile_is_dark = malloc(JTP_MAP_HEIGHT*sizeof(char *));
  jtp_room_indices     = malloc(JTP_MAP_HEIGHT*sizeof(char *));

  if ((!jtp_map_back) || (!jtp_map_obj) || (!jtp_map_trap) || (!jtp_map_glyph) ||
      (!jtp_map_furniture) || (!jtp_map_mon) || (!jtp_map_specialeff) ||
      (!jtp_maptile_wall) || (!jtp_maptile_floor_edge) || (!jtp_map_deco) ||
      (!jtp_map_light) || (!jtp_map_specialattr) || (!jtp_room_indices))
  {
    OOM(1);
  }
  
  for (i = 0; i < JTP_MAP_HEIGHT; i++)
  {
    jtp_map_glyph[i]       = malloc(JTP_MAP_WIDTH * sizeof(int));
    jtp_map_back[i]        = malloc(JTP_MAP_WIDTH * sizeof(int));
    jtp_map_obj[i]         = malloc(JTP_MAP_WIDTH * sizeof(int));
    jtp_map_trap[i]        = malloc(JTP_MAP_WIDTH * sizeof(int));
    jtp_map_furniture[i]   = malloc(JTP_MAP_WIDTH * sizeof(int));
    jtp_map_specialeff[i]  = malloc(JTP_MAP_WIDTH * sizeof(int));
    jtp_map_mon[i]         = malloc(JTP_MAP_WIDTH * sizeof(int));
    jtp_map_specialattr[i] = malloc(JTP_MAP_WIDTH * sizeof(unsigned int));
    jtp_map_deco[i]        = malloc(JTP_MAP_WIDTH * sizeof(unsigned char));

    jtp_maptile_wall[i]       = malloc(JTP_MAP_WIDTH*sizeof(jtp_wall_style));
    jtp_maptile_floor_edge[i] = malloc(JTP_MAP_WIDTH*sizeof(jtp_floor_edge_style));

    jtp_map_light[i] = malloc(JTP_MAP_WIDTH*sizeof(int));
    jtp_map_tile_is_dark[i] = malloc(JTP_MAP_WIDTH*sizeof(char));
    jtp_room_indices[i] = malloc(JTP_MAP_WIDTH*sizeof(char));

    if ((!jtp_map_back[i]) || (!jtp_map_obj[i]) || (!jtp_map_trap[i]) ||
        (!jtp_map_furniture[i]) || (!jtp_map_mon[i]) || (!jtp_map_specialeff[i]) ||
        (!jtp_maptile_wall[i]) || (!jtp_maptile_floor_edge[i]) || (!jtp_map_deco[i]) ||
        (!jtp_map_light[i]) || (!jtp_map_specialattr[i]) || (!jtp_room_indices[i]))
    {
      OOM(1);
    }
    
    for (j = 0; j < JTP_MAP_WIDTH; j++)
    {
      jtp_map_glyph[i][j] = NO_GLYPH;  
      jtp_map_back[i][j] = V_TILE_UNMAPPED_AREA;
      jtp_map_obj[i][j] = V_TILE_NONE;
      jtp_map_trap[i][j] = V_TILE_NONE;
      jtp_map_furniture[i][j] = V_TILE_NONE;
      jtp_map_specialeff[i][j] = V_TILE_NONE;
      jtp_map_mon[i][j] = V_TILE_NONE;
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
      //jtp_map_symbols[i] = jtp_get_img_src(1 + 10 * i, 1, 7 + 10 * i, 14, image);
      jtp_map_symbols[i] = jtp_get_img_src(
              (i%40)*VULTURES_MAP_SYMBOL_WIDTH,
              (i/40)*VULTURES_MAP_SYMBOL_HEIGHT,
              (i%40)*VULTURES_MAP_SYMBOL_WIDTH+(VULTURES_MAP_SYMBOL_WIDTH-1),
              (i/40)*VULTURES_MAP_SYMBOL_HEIGHT+(VULTURES_MAP_SYMBOL_HEIGHT-1),
              image);
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
