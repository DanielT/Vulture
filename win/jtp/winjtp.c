/*	SCCS Id: @(#)winjtp.c	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif

#include "func_tab.h" /* For extended commands list */
#include "display.h"  /* For identifying glyph types */

#include "jtp_keys.h"
#include "jtp_win.h"
#include "jtp_gra.h"
#include "winjtp.h"

/* Interface definition, for windows.c */
struct window_procs jtp_procs = {
    "jtp",
    WC_COLOR |
    WC_HILITE_PET |
    WC_TILED_MAP |
    WC_PRELOAD_TILES |
    WC_TILE_WIDTH |
    WC_TILE_HEIGHT |
    WC_TILE_FILE |
    WC_MAP_MODE |
    WC_TILED_MAP |
    WC_PLAYER_SELECTION,
    WC2_FULLSCREEN,
    jtp_init_nhwindows,
    jtp_player_selection,
    jtp_askname,
    jtp_get_nh_event,
    jtp_exit_nhwindows,
    jtp_suspend_nhwindows,
    jtp_resume_nhwindows,
    jtp_create_nhwindow,
    jtp_clear_nhwindow,
    jtp_display_nhwindow,
    jtp_destroy_nhwindow,
    jtp_curs,
    jtp_putstr,
    jtp_display_file,
    jtp_start_menu,
    jtp_add_menu,
    jtp_end_menu,
    jtp_select_menu,
    jtp_message_menu,
    jtp_update_inventory,
    jtp_mark_synch,
    jtp_wait_synch,
#ifdef CLIPPING
    jtp_cliparound,
#endif
#ifdef POSITIONBAR
    jtp_update_positionbar,
#endif
    jtp_print_glyph,
    jtp_raw_print,
    jtp_raw_print_bold,
    jtp_nhgetch,
    jtp_nh_poskey,
    jtp_nhbell,
    jtp_doprev_message,
    jtp_yn_function,
    jtp_getlin,
    jtp_get_ext_cmd,
    jtp_number_pad,
    jtp_delay_output,
#ifdef CHANGE_COLOR     /* the Mac uses a palette device */
    jtp_change_colour,
#ifdef MAC
    jtp_change_background,
    jtp_set_font_name,
#endif
    jtp_get_colour_string,
#endif
    jtp_start_screen,
    jtp_end_screen,
    jtp_outrip,
    donull /* TODO jtp_preference_update */
};

int debug_putstr = 0;
/* Is the player using a 'what is' command? */
int jtp_whatis_active = 0;
/* Has the map window been drawn on? (eg. is it empty or not) */
int jtp_map_is_empty = 1;


void
win_jtp_init()
{
  jtp_init_graphics();
}

void
jtp_start_screen()
{
}

void
jtp_end_screen()
{
}


void
jtp_outrip(window, how)
winid window;
int how;
{
  jtp_window * tempwindow;
  
  tempwindow = jtp_find_window(window);
  if (!tempwindow) return;
  
  tempwindow->ending_type = how;
}

/* clean up and quit */
void
bail(mesg)
const char *mesg;
{
    clearlocks();
    jtp_exit_nhwindows(mesg);
    terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}



/* Low level routines */

void jtp_raw_print(str)
const char *str;
{
  jtp_write_log_message("[NetHack]: ");
  jtp_write_log_message(str);
  jtp_write_log_message("\n");
}


void jtp_raw_print_bold(str)
const char *str;
{
  jtp_write_log_message("[NetHack]: ");
  jtp_write_log_message(str);
  jtp_write_log_message("\n");
}


void 
jtp_curs(window, x, y)
winid window;
register int x, y;
{
  jtp_window * tempwindow;

  tempwindow = jtp_find_window(window);
  if (!tempwindow) return;
  tempwindow->curs_x = x;
  tempwindow->curs_y = y;
}


void
jtp_putstr(window, attr, str)
winid window;
int attr;
const char *str;
{
  int i, j;
  jtp_window * tempwindow;
  char tempbuffer[1024];
  anything tempid;

/* JTP_DEBUG
  if (debug_putstr)
  {
    printf("%s\n", str);
    getch();
  }  
JTP_DEBUG */

  /* Display error messages immediately */
  if (window == WIN_ERR)
  {
    jtp_messagebox(str);
    return;    
  }
  
  tempwindow = jtp_find_window(window);
  if (!tempwindow)
  {
    jtp_write_log_message("ERROR: can't find window!\n");
    jtp_exit_graphics(); exit(1);
  }  

  /*
   * Each window type handles putstr separately.
   * Sometimes it's for convenience, sometimes for game or graphical reasons.
   */


  /* 
   * For windows of type NHW_MESSAGE, both the messages
   * and their send time are stored.
   * Cursor placement (curs) is not supported.
   */
  if (tempwindow->wintype == NHW_MESSAGE)
  {
    /* Stop autopilot, except during a 'what is' command */
    if ((jtp_move_length > 0) &&
        (jtp_autopilot_type == JTP_AUTOPILOT_MOVEMENT))
    {
      jtp_move_length = 0;
    }  

    /* Stop inventory (backpack) shortcut action */
    jtp_is_backpack_shortcut_active = 0;
    /* Stop spellbook viewing */
    jtp_is_spellbook_being_viewed = 0;

    /*
     * If the "what's this"-type autopilot is active,
     * then skip the help messages associated with NetHack's
     * lookat command.
     */
    if ((jtp_move_length > 0) &&
        (jtp_autopilot_type == JTP_AUTOPILOT_WHATSTHIS))
    {
      /* Skip help line [Please move the cursor to an unknown object.] */
      if ((str) && (strncmp(str, "Please move", 11) == 0)) return;
      /* Skip help line [(For instructions type a ?)] */
      if ((str) && (strncmp(str, "(For instru", 11) == 0)) return;
      /* Skip help line [Pick an object.] */
      if ((str) && (strncmp(str, "Pick an obj", 11) == 0)) return;
    }

    if (str)
    {
      /* If the message is blank (whitespace), it will not be stored. */
      j = 0;
      for (i = 0; i < strlen(str); i++)
        if ((str[i] != ' ') && (str[i] != 10) && (str[i] != 13))
        {
          j = 1; break;
        }            
      if (j)
      {
        /* Discard oldest message, shift others forward */
        free(tempwindow->rows[tempwindow->curs_rows-1]);
        for (i = tempwindow->curs_rows-1; i > 0; i--)
          tempwindow->rows[i] = tempwindow->rows[i-1];

        /* Remove leading and trailing whitespace */
        j = 0; while (str[j] == ' ') j++;
        i = strlen(str)-1; while ((str[i]==' ')||(str[i]==10)||(str[i]==13)) i--;
                
        /* Allocate space for new message. Store string and send time. */
        tempwindow->rows[0] = (int *)malloc(sizeof(int) + (i-j+2)*sizeof(char));
        memcpy((char *)(tempwindow->rows[0]) + sizeof(int), str+j, i-j+1);
        ((char *)(tempwindow->rows[0]) + sizeof(int))[i-j+1] = '\0';
        tempwindow->rows[0][0] = moves; /* Store send time in moves */
 
        /* Make sure the new message is shown */
        jtp_first_shown_message = 0;

        /* Play any event sounds associated with this message */
        jtp_play_event_sound(str+j);
      }
    }
    /* Copy message to toplines[] */
    strcpy(toplines, str);
    /* Redisplay message window */
    jtp_display_nhwindow(window, FALSE);
  }

  /*
   * If this is a NHW_MENU window, the window must first be initialized to display 
   * text data. If that has been done, then the text lines will be added as menu items.
   */
  else if (tempwindow->wintype == NHW_MENU)
  {
    /* If the window is already set to menu data, it can't show text data. */
    if ((tempwindow->menu) && (!tempwindow->menu->content_is_text)) return;
    
    /* Initialize menu structure, if necessary */
    if (!tempwindow->menu)
    {
      tempwindow->menu = (jtp_menu *)malloc(sizeof(jtp_menu));    
      tempwindow->menu->items = jtp_list_new();
      tempwindow->menu->prompt = NULL;
      tempwindow->menu->content_is_text = 1;    /* Text content */
    }
        
    /* Remove extra newline from the end of the string */
    strncpy(tempbuffer, str, 1023);
    i = strlen(tempbuffer)-1; 
    while ((i > 0) && ((tempbuffer[i] == 10) || (tempbuffer[i] == 13)))
      i--;
    tempbuffer[i+1] = '\0';  

    /* Add the new text line(s) as menu item(s) */    
    tempid.a_int = 1; /* Since text lines can't be selected anyway, these can be the same */    
    jtp_add_menu(window, NO_GLYPH, &tempid, 0, 0, ATR_NONE, tempbuffer, FALSE);
  }

  /*
   * For now, NHW_TEXT windows are handled almost like NHW_MENU windows. 
   * This may change in the future.
   */
  else if (tempwindow->wintype == NHW_TEXT)
  {    
    /* Initialize menu structure, if necessary */
    if (!tempwindow->menu)
    {
      tempwindow->menu = (jtp_menu *)malloc(sizeof(jtp_menu));    
      tempwindow->menu->items = jtp_list_new();
      tempwindow->menu->prompt = NULL;
      tempwindow->menu->content_is_text = 1;    /* Text content */
    }
        
    /* Remove extra newline from the end of the string */
    strncpy(tempbuffer, str, 1023);
    i = strlen(tempbuffer)-1; 
    while ((i > 0) && ((tempbuffer[i] == 10) || (tempbuffer[i] == 13)))
      i--;
    tempbuffer[i+1] = '\0';  

    /* Add the new text line(s) as menu item(s) */    
    tempid.a_int = 1; /* Since text lines can't be selected anyway, these can be the same */    
    jtp_add_menu(window, NO_GLYPH, &tempid, 0, 0, ATR_NONE, tempbuffer, FALSE);
  }

  /*
   * The status window is the only window that supports cursor placement (curs),
   * but theoretically this could be extended to the map window and NHW_TEXT or
   * NHW_MENU windows.
   */  
  else if (tempwindow->wintype == NHW_STATUS)
  {
    /* Clear window, if necessary */
    if (tempwindow->curs_y >= tempwindow->curs_rows) 
    {
      for (i = 0; i < tempwindow->curs_rows; i++)
        for (j = 1; j < tempwindow->curs_cols; j++)
          tempwindow->rows[i][j] = JTP_WINCONTENT_EMPTY;
      tempwindow->curs_y = 0;
      tempwindow->curs_x = 1;
    }
    
    /* Output message */
    for (i = 0; i < strlen(str); i++)
    {
      if ((str[i] == 10) || (str[i] == 13))
      {
        for (j = tempwindow->curs_x; j < tempwindow->curs_cols; j++)
          tempwindow->rows[tempwindow->curs_y][j] = JTP_WINCONTENT_EMPTY;
        tempwindow->curs_y++;
        tempwindow->curs_x = 1;
      }
      else
      {
        tempwindow->rows[tempwindow->curs_y][tempwindow->curs_x] = str[i];
        tempwindow->curs_x++;
        if (tempwindow->curs_x >= tempwindow->curs_cols)
        {
          tempwindow->curs_y++;
          tempwindow->curs_x = 1;
        }    
      }
      if (tempwindow->curs_y >= tempwindow->curs_rows)
        i = strlen(str);
    }
  
    /* Make sure next output starts on the next line */
    if (tempwindow->curs_x > 1)
    {
      for (j = tempwindow->curs_x; j < tempwindow->curs_cols; j++)
        tempwindow->rows[tempwindow->curs_y][j] = JTP_WINCONTENT_EMPTY;
      tempwindow->curs_y++;
      tempwindow->curs_x = 1;
    }    
  }
  
  /* The status window is always redisplayed after an update */
  if (tempwindow->wintype == NHW_STATUS)
    jtp_display_nhwindow(window, FALSE);
}


void
jtp_get_nh_event()
{
}


int
jtp_nhgetch()
{
  char result_key;
  int  i, j;
  char tempbuffer[1024];

  /*
   * I searched through the code, but nhgetch()
   * seems to be only used if the windowing system is
   * _not_ active (checked by !windowinited). So it doesn't
   * need to use map scrolling or other niceties.
   */
  result_key = 0;
  while (result_key == 0)
    result_key = jtp_getch();
  return(result_key);
}


int
jtp_nh_poskey(x, y, mod)
int *x, *y, *mod;
{
  char result_key;
  int i, j;

  /* Play ambient music or sound effects */
  jtp_play_ambient_sound(0);

  /* 
   * If this call is the result of a 'what is' command,
   * don't use or initiate autopilot. Also, a mouse
   * click on the map is now an acceptable response,
   * since it isn't passed to the autopilot code.
   */
  if (jtp_whatis_active)
  {
    result_key = 0;
    while (!result_key)
    { 
      /* 
       * Use autopilot, if active. For a 'what is' command,
       * the autopilot is just a series of locations to look at.
       */
      if ((jtp_move_length > 0) && (jtp_autopilot_type == JTP_AUTOPILOT_WHATSTHIS))
      {
        if (jtp_movebuffer[0] > 0)
        {
          *y = jtp_movebuffer[0]/jtp_map_width;
          *x = jtp_movebuffer[0]%jtp_map_width;
          for (i = 0; i < jtp_move_length-1; i++)
            jtp_movebuffer[i] = jtp_movebuffer[i+1];
          jtp_move_length--;
          return(0);
        }
        else
        {
          jtp_move_length = 0;
          return(' ');
        }
      }

      jtp_get_map_input();
      
      /* Process mouse click */
      result_key = jtp_whatis_mouseclick(&i, &j);
      if (result_key)
        return(result_key);
      else if (isok(i, j))
      {
        *x = i;
        *y = j;
        return(0);
      }
  
      /* Process key press */
      if (jtp_kbhit())
      {
        result_key = jtp_getch();
        result_key = jtp_translate_key(result_key);      
        return(result_key);
      }
    
      if (!result_key)
      {
        jtp_draw_all_windows();
        jtp_show_screen();      
      }
    }    
  }
  
  /*
   * If this call is not the result of 'what is',
   * then allow autopilot. This handles the
   * default GUI interface.
   */  
  else
  {
#ifdef JTP_INVISIBILITY_CODE
    if ((!jtp_map_is_empty) && (canseeself() == FALSE))
    {
      jtp_messagebox("Can't see self!");
      /* Don't use autopilot if the hero can't see self */
      jtp_you_x = -1;
      jtp_you_y = -1;
      if ((jtp_move_length > 0) && (jtp_autopilot_type == JTP_AUTOPILOT_MOVEMENT))
      {
        jtp_move_length = 0;
      }  
    }
#endif
    if (jtp_kbhit())
    {
      /* Disable autopilot because the player has made a command */
      if ((jtp_move_length > 0) && (jtp_autopilot_type == JTP_AUTOPILOT_MOVEMENT))
      {
        jtp_move_length = 0;
      }  
    }

    result_key = 0;
    while (!result_key)
    { 
      /* 
       * Use autopilot, if active. The 'autopilot' is just a series
       * of adjacent locations. If the hero is at one location,
       * the autopilot supplies the necessary command to move to
       * the next.
       */
      if ((jtp_move_length > 0) && (jtp_autopilot_type == JTP_AUTOPILOT_MOVEMENT))
      {
        /* Show a 'please wait'-style cursor */
        jtp_show_wait_cursor();
      
        i = jtp_movebuffer[0]/jtp_map_width;
        j = jtp_movebuffer[0]%jtp_map_width;
        
        if ((i == jtp_you_y-1) && (j == jtp_you_x-1))
          result_key = jtp_translate_command(JTP_NHCMD_NORTHWEST);
        else if ((i == jtp_you_y-1) && (j == jtp_you_x))
          result_key = jtp_translate_command(JTP_NHCMD_NORTH);
        else if ((i == jtp_you_y-1) && (j == jtp_you_x+1))
          result_key = jtp_translate_command(JTP_NHCMD_NORTHEAST);
        else if ((i == jtp_you_y) && (j == jtp_you_x-1))
          result_key = jtp_translate_command(JTP_NHCMD_WEST);
        else if ((i == jtp_you_y) && (j == jtp_you_x+1))
          result_key = jtp_translate_command(JTP_NHCMD_EAST);
        else if ((i == jtp_you_y+1) && (j == jtp_you_x-1))
          result_key = jtp_translate_command(JTP_NHCMD_SOUTHWEST);
        else if ((i == jtp_you_y+1) && (j == jtp_you_x))
          result_key = jtp_translate_command(JTP_NHCMD_SOUTH);
        else if ((i == jtp_you_y+1) && (j == jtp_you_x+1))
          result_key = jtp_translate_command(JTP_NHCMD_SOUTHEAST);
        else result_key = 0;
        if (result_key)
        {
          for (i = 0; i < jtp_move_length-1; i++)
            jtp_movebuffer[i] = jtp_movebuffer[i+1];
          jtp_move_length--;
          jtp_play_command_sound(JTP_NHCMD_EAST);
          return(result_key);
        }
        else /* Something strange has happened along the way. Stop moving. */
        {
          if (jtp_move_length > 0)
          {
            jtp_move_length = 0;
          }
        }
      }

      /* If the user selected a shortcut from the backpack screen, use that. */
      if (jtp_is_backpack_shortcut_active)
      {
        result_key = jtp_backpack_shortcut_action;
        jtp_is_backpack_shortcut_active = 0;
        jtp_play_command_sound(jtp_find_command_nhkey_index(result_key));
        return(result_key);
      }

      /*
       * If we got here, there was no autopilot or shortcut command.
       * Next, we read a mouse or keyboard command from the user.
       */
      jtp_get_map_input();

      /* Process key press */
      if (jtp_kbhit())
      {        
        result_key = jtp_getch();
        result_key = jtp_translate_key(result_key);
        jtp_play_command_sound(jtp_find_command_nhkey_index(result_key));
        switch(result_key)
        {        
          case '|': 
            jtp_messagebox("Saving screenshot");
            jtp_save_screenshot("scree000.pcx"); 
            break;
          default: break;
        }
        return(result_key);
      }
      
      /* Process mouse click */
      result_key = jtp_process_mouseclick();
      if (result_key)
      {
        jtp_play_command_sound(jtp_find_command_nhkey_index(result_key));
        return(result_key);
      }      
      else
      {
        jtp_draw_all_windows();
        jtp_show_screen();      
      }
    }
  }
}

/* High level routines */

void
jtp_print_glyph(window, x, y, glyph)
winid window;
xchar x, y;
int glyph;
{
  int k;

  /*
  I hope glyphs are ONLY used in the map window and menus.
  jtp_window * tempwindow;  
  tempwindow = jtp_find_window(window);
  if (!tempwindow) return;
  */
  
  if ((y >= jtp_map_height) || (y < 0) ||
      (x >= jtp_map_width) || (x < 1))
    return;

  /* By this time we need to initialize tile conversion */
  if (!jtp_tile_conversion_initialized)
    jtp_init_glyph_tiles();

  if (glyph_is_cmap(glyph))
  {
    k = glyph_to_cmap(glyph);

    /* S_stone glyphs need care to distinguish stone from unexplored area */
    if (k == S_stone)
    {
      if ((levl[x][y].typ == STONE) ||     /* An actual rough stone wall */
          (levl[x][y].typ == SCORR) ||     /* A secret passage */
          (levl[x][y].typ == SDOOR))       /* A secret door */
      {
        if (levl[x][y].seenv)
          jtp_mapglyph_cmap[y][x] = S_stone;
        else jtp_mapglyph_cmap[y][x] = JTP_WINCONTENT_GLYPH_UNEXPLORED; /* Unexplored area */
      }
      else if ((levl[x][y].seenv) &&       /* Unfinished parts of room walls */
          (levl[x][y].typ >= VWALL) &&     /* Assumes walls are in this order in rm.h */
          (levl[x][y].typ <= DBWALL))
        jtp_mapglyph_cmap[y][x] = S_stone;
      else if (levl[x][y].seenv)
        jtp_mapglyph_cmap[y][x] = JTP_WINCONTENT_GLYPH_NOT_VISIBLE; /* Areas that are explored but not visible (eg. dark) */
      else jtp_mapglyph_cmap[y][x] = JTP_WINCONTENT_GLYPH_UNEXPLORED; /* Unexplored area */
    }
    else
    {
      jtp_mapglyph_cmap[y][x] = k;
      if ((k >= S_vbeam) && (k <= S_ss4))
        printf("Printed effect %d\n");
      jtp_map_is_empty = 0;
    }

    /* Erase objects and monsters - hope they also get redrawn! */
    jtp_mapglyph_obj[y][x] = JTP_WINCONTENT_EMPTY;
    jtp_mapglyph_mon[y][x] = JTP_WINCONTENT_EMPTY;
  }  
  else if (glyph_is_object(glyph))
  {
    k = glyph_to_obj(glyph);
    jtp_mapglyph_obj[y][x] = k;
    jtp_map_is_empty = 0;

    /* Erase monsters - hope they also get redrawn! */
    jtp_mapglyph_mon[y][x] = JTP_WINCONTENT_EMPTY;
  }
  else if (glyph_is_monster(glyph))
  {
    k = glyph_to_mon(glyph);
    jtp_mapglyph_mon[y][x] = k;
    jtp_map_is_empty = 0;
  }
  else if ((glyph >= GLYPH_ZAP_OFF) && (glyph <= GLYPH_ZAP_OFF + NUM_ZAP))
  {
    /* A zap effect */
    k = S_vbeam + ((glyph - GLYPH_ZAP_OFF) & 0x3);
    printf("Printing zap (%d)\n", k);
    jtp_mapglyph_cmap[y][x] = k;
    jtp_map_is_empty = 0;
  }

  /* Notify the graphics code that the map has changed */
  jtp_map_changed = 1;
}


char
jtp_yn_function(ques, choices, defchoice)
const char *ques;
const char *choices;
char defchoice;
{
  int  i;
  char tempbuffer[1024];
  char tempbuffer2[64];
  int  nanswers;
  int  nchoice;

/*  jtp_messagebox("jtp_yn_function"); */
  
  /* Separate answers string into discrete choice strings */
  tempbuffer[0] = '\0';
  nanswers = 0;
  if (choices)
  {
    for (i = 0; i < strlen(choices); i++)
    {
      if (choices[i] == '#') sprintf(tempbuffer2, "Number_");
      else if (choices[i] == 27) {sprintf(tempbuffer2, "Other_"); i = strlen(choices);}
      else sprintf(tempbuffer2, "%c_", choices[i]);
      
      strcat(tempbuffer, tempbuffer2);
      nanswers++;
    }
    /* Get rid of extra underscore */
    if (strlen(tempbuffer) > 0)
      tempbuffer[strlen(tempbuffer)-1] = '\0';
  }  
  
  nchoice = jtp_query(-1, -1, ques, nanswers, tempbuffer, 0);
  if (nanswers == 0) return(nchoice);
  else return(choices[nchoice]);
}


void
jtp_getlin(ques, input)
const char *ques;
char *input;
{
  jtp_get_input(-1, -1, ques, input);
}


int
jtp_get_ext_cmd()
{
  int i, j, k;
  int tempwin;
  int nselected;
  anything id;
  menu_item * selected;
  char used_accelerators[512];
  char cur_accelerator, acc_found;

  tempwin = jtp_create_nhwindow(NHW_MENU);
  jtp_start_menu(tempwin);
  used_accelerators[0] = '\0';

  /* Add extended commands as menu items */
  for (i = 0; extcmdlist[i].ef_txt != NULL; i++)
  {
    /* Find an unused accelerator */
    acc_found = 0;
    for (k = 0; k < strlen(extcmdlist[i].ef_txt); k++)
    {
      cur_accelerator = tolower(extcmdlist[i].ef_txt[k]);
      acc_found = 1;
      for (j = 0; j < strlen(used_accelerators); j++)
        if (used_accelerators[j] == cur_accelerator) acc_found = 0;
      if (!acc_found)
      {
        cur_accelerator = toupper(extcmdlist[i].ef_txt[k]);
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
    /* Add found accelerator to list of used ones */
    j = strlen(used_accelerators);
    used_accelerators[j] = cur_accelerator;
    used_accelerators[j+1] = '\0';
    
    /* 
     * All extended commands are selectable, so make sure that a_void is not zero
     * (it is used for 'title line' checking)
     */
    id.a_int = i + 1;
    jtp_add_menu(tempwin, NO_GLYPH, &id, cur_accelerator, 0, ATR_NONE, extcmdlist[i].ef_txt, FALSE);
  }
  jtp_end_menu(tempwin, "Select a command");  
  nselected = jtp_select_menu(tempwin, PICK_ONE, &selected);
  jtp_destroy_nhwindow(tempwin);
  
  if (nselected <= 0) 
  {
    free(selected);
    return(-1);
  }  
  else
  {
    id = selected[0].item;
    free(selected);
    return(id.a_int - 1);
  }
}




void
jtp_display_file(fname, complain)
const char *fname;
boolean complain;
{
  dlb * f;                /* Data librarian */
  int  nlines;
  char ** textlines;
  char tempbuffer[1024];
  int  i;
  winid window;
  anything tempid;
  menu_item * selected;

/*
  jtp_messagebox("jtp_display_file");
  jtp_messagebox(fname);
*/

  /* Read the file */
  f = dlb_fopen(fname, "r");
  if (!f)
  {
    if (complain == TRUE)
    {
      sprintf(tempbuffer, "Can't open file [%s].\n", fname);
      jtp_messagebox(tempbuffer);      
    }
    return;
  }  

  nlines = 0;
  textlines = NULL;
  while (dlb_fgets(tempbuffer, 1024, f))
  {
    nlines++;
    textlines = (char **)realloc(textlines, nlines*sizeof(char *));
    if (!textlines)
    {
      jtp_write_log_message("ERROR: Out of memory! - location 31\n");
      jtp_exit_graphics(); exit(1);
    }
    
    textlines[nlines-1] = (char *)malloc(strlen(tempbuffer)+1);
    if (!textlines[nlines-1])
    {
      jtp_write_log_message("ERROR: Out of memory! - location 32\n");
      jtp_exit_graphics(); exit(1);
    }    
    strcpy(textlines[nlines-1], tempbuffer);
  }
  (void) dlb_fclose(f);  

  /* 
   * Make a NHW_MENU window, add each text line as a (textual) menu item.
   * This is a very unoptimized way of showing a text file ...
   */
  window = jtp_create_nhwindow(NHW_MENU);
  for (i = 0; i < nlines; i++)
    jtp_putstr(window, ATR_NONE, textlines[i]);
  
  /* Display the file */
  jtp_display_nhwindow(window, TRUE);
  
  /* Clean up */
  jtp_destroy_nhwindow(window);
  for (i = 0; i < nlines; i++)
    free(textlines[i]);
  free(textlines);  
}


void
jtp_update_inventory()
{
}


void
jtp_player_selection()
{
  jtp_select_player();
}


int
jtp_doprev_message()
{
  /* jtp_messagebox("jtp_doprev_message"); */
  jtp_first_shown_message++;
  if (jtp_first_shown_message > JTP_MAX_OLD_MESSAGES-JTP_MAX_SHOWN_MESSAGES)
    jtp_first_shown_message = JTP_MAX_OLD_MESSAGES-JTP_MAX_SHOWN_MESSAGES;
  jtp_draw_messages(jtp_find_window(WIN_MESSAGE));
  jtp_show_screen();
  return(0);
}


/* Window utility routines */

void
jtp_init_nhwindows(argcp, argv)
int *argcp;
char **argv;
{
  jtp_window * tempwindow;
  winid tempwinid;

  jtp_max_window_id = -1;
  jtp_windowlist = jtp_list_new();
  
  jtp_show_logo_screen();
  
  /* Success! */
  iflags.window_inited = TRUE;
}


void
jtp_exit_nhwindows(str)
const char *str;
{
  jtp_exit_graphics();
  if (str)
    printf("%s\n", str);
}


winid
jtp_create_nhwindow(type)
int type;
{
  int i, j;
  jtp_window * tempwindow;
  
  tempwindow = (jtp_window *)malloc(sizeof(jtp_window));
  if (!tempwindow)
  {
    jtp_write_log_message("ERROR: Out of memory! - location 1\n");
    jtp_exit_graphics(); exit(1);
  }
    
  jtp_max_window_id++;
  tempwindow->id = jtp_max_window_id;
  tempwindow->curs_x = 1;
  tempwindow->curs_y = 0;
  tempwindow->active = 0; /* Not active at creation time */
  switch(type)
  {
/*  
    case NHW_BASE:
      tempwindow->x = 0;
      tempwindow->y = 0;
      tempwindow->width = 800;
      tempwindow->height = 600;
      tempwindow->curs_rows = 50;
      tempwindow->curs_cols = 80;
      tempwindow->rows = (int **)malloc(tempwindow->curs_rows*sizeof(int *));
      for (i = 0; i < tempwindow->curs_rows; i++)
        tempwindow->rows[i] = (int *)malloc(tempwindow->curs_cols*sizeof(int));
      tempwindow->wintype = NHW_BASE;
      tempwindow->menu = NULL;
      tempwindow->buttons = NULL;      
      break;
*/      
    case NHW_MESSAGE:
      tempwindow->x = 0;
      tempwindow->y = 0;
      tempwindow->width = -1;
      tempwindow->height = -1;
      tempwindow->curs_rows = JTP_MAX_OLD_MESSAGES;
      tempwindow->curs_cols = -1;
      tempwindow->rows = (int **)malloc(tempwindow->curs_rows*sizeof(int *));
      /* Message memory is allocated when the message is received */
      for (i = 0; i < tempwindow->curs_rows; i++)
        tempwindow->rows[i] = NULL;
      tempwindow->wintype = NHW_MESSAGE;
      tempwindow->menu = NULL;      
      tempwindow->buttons = NULL;      
      break;
    case NHW_STATUS:
      tempwindow->x = -1;
      tempwindow->y = -1;
      tempwindow->width = 800;
      tempwindow->height = 100;
      tempwindow->curs_rows = 2;
      tempwindow->curs_cols = 80;
      tempwindow->rows = (int **)malloc(tempwindow->curs_rows*sizeof(int *));
      for (i = 0; i < tempwindow->curs_rows; i++)
      {
        tempwindow->rows[i] = (int *)malloc(tempwindow->curs_cols*sizeof(int));
        for (j = 0; j < tempwindow->curs_cols; j++)
          tempwindow->rows[i][j] = JTP_WINCONTENT_EMPTY;
      }
      tempwindow->curs_x = 1;
      tempwindow->curs_y = 0;      
      tempwindow->wintype = NHW_STATUS;
      tempwindow->menu = NULL;      
      tempwindow->buttons = NULL;      
      break;
    case NHW_MAP:
      tempwindow->x = 0;
      tempwindow->y = 0;
      tempwindow->width = -1;
      tempwindow->height = -1;
      tempwindow->curs_rows = jtp_map_height;
      tempwindow->curs_cols = jtp_map_width;            
      tempwindow->curs_x = 1;
      tempwindow->curs_y = 0;
      tempwindow->rows = NULL;
      tempwindow->wintype = NHW_MAP;
      tempwindow->menu = NULL;      
      tempwindow->buttons = NULL;      
      break;
    case NHW_MENU:
      tempwindow->x = -1;
      tempwindow->y = -1;
      tempwindow->width = -1;
      tempwindow->height = -1;
      tempwindow->curs_rows = 50;
      tempwindow->curs_cols = 80;
      tempwindow->rows = (int **)malloc(tempwindow->curs_rows*sizeof(int *));
      for (i = 0; i < tempwindow->curs_rows; i++)
        tempwindow->rows[i] = (int *)malloc(tempwindow->curs_cols*sizeof(int));
      tempwindow->wintype = NHW_MENU;
      tempwindow->menu = NULL;      
      tempwindow->buttons = NULL;      
      break;
    case NHW_TEXT:
      tempwindow->x = -1;
      tempwindow->y = -1;
      tempwindow->width = -1;
      tempwindow->height = -1;
      tempwindow->curs_rows = 50;
      tempwindow->curs_cols = 80;
      tempwindow->rows = (int **)malloc(tempwindow->curs_rows*sizeof(int *));
      if (!tempwindow->rows)
      {
        jtp_write_log_message("ERROR: Out of memory! - location 2\n");
        jtp_exit_graphics(); exit(1);
      }      
      for (i = 0; i < tempwindow->curs_rows; i++)
      {
        tempwindow->rows[i] = (int *)malloc(tempwindow->curs_cols*sizeof(int));
        if (!tempwindow->rows[i])
        {
          jtp_write_log_message("ERROR: Out of memory! - location 3\n");
          jtp_exit_graphics(); exit(1);
        }        
        for (j = 0; j < tempwindow->curs_cols; j++)
          tempwindow->rows[i][j] = JTP_WINCONTENT_EMPTY;
      }  
        
      tempwindow->curs_x = 1;
      tempwindow->curs_y = 0;
      tempwindow->wintype = NHW_TEXT;
      tempwindow->menu = NULL;      
      tempwindow->buttons = NULL;      
      tempwindow->ending_type = -1;
      break;
    default:
      return(WIN_ERR);
      break;
  }

  jtp_list_reset(jtp_windowlist);
  jtp_list_add(jtp_windowlist, tempwindow);
  return(tempwindow->id);
}


void
jtp_clear_nhwindow(window)
winid window;
{
  int i, j;
  jtp_window * tempwindow;

/*  jtp_messagebox("jtp_clear_nhwindow"); */

  tempwindow = jtp_find_window(window);
  if (!tempwindow) return;
  if (tempwindow->wintype == NHW_MAP)
  {
    for (i = 0; i < jtp_map_height; i++)
      for (j = 0; j < jtp_map_width; j++)
      {
        jtp_print_glyph(NHW_MAP, j, i, S_stone + GLYPH_CMAP_OFF);
        /* jtp_mapglyph_cmap[i][j] = JTP_WINCONTENT_EMPTY; */
        jtp_mapglyph_obj[i][j] = JTP_WINCONTENT_EMPTY;
        jtp_mapglyph_mon[i][j] = JTP_WINCONTENT_EMPTY;
      }
  }
  else
  {
    for (i = 0; i < tempwindow->curs_rows; i++)
      for (j = 0; j < tempwindow->curs_cols; j++)
        tempwindow->rows[i][j] = JTP_WINCONTENT_EMPTY;
  }    
}

void
jtp_display_nhwindow(window, blocking)
winid window;
boolean blocking;
{
  jtp_window * tempwindow;
  char tempbuffer[200];
  int  messages_height;
  menu_item * menu_list;    /* Dummy pointer for displaying NHW_MENU windows */

    
  tempwindow = jtp_find_window(window);
  if (!tempwindow) 
  {  
    jtp_write_log_message("ERROR: Invalid window!\n");
    jtp_exit_graphics(); exit(1); 
  }
  
  switch(tempwindow->wintype)
  {
    case NHW_MENU:
      jtp_end_menu(window, NULL);
      jtp_select_menu(window, PICK_NONE, &menu_list);
/*      
      if (tempwindow->width <= 0) return;
      jtp_draw_window(tempwindow->x, tempwindow->y, tempwindow->width, tempwindow->height);
      jtp_draw_menu(tempwindow->menu);
      jtp_draw_buttons(tempwindow->buttons);
*/      
      break;
    case NHW_TEXT:
      if (tempwindow->ending_type >= 0) 
        jtp_show_ending(tempwindow);
      else
      {
        jtp_end_menu(window, NULL);
        jtp_select_menu(window, PICK_NONE, &menu_list);
      }
      break;
    case NHW_MAP:
      if ((jtp_map_changed) && (!jtp_map_is_empty))
      {
        /* Update the location of the player */
        jtp_old_you_x = jtp_you_x;
        jtp_old_you_y = jtp_you_y;
        jtp_you_x = u.ux;
        jtp_you_y = u.uy;

        /* If the hero has moved, center the view on him/her (optional) */
        if (jtp_recenter_after_movement)
          if ((jtp_you_x != jtp_old_you_x) || (jtp_you_y != jtp_old_you_y))
          {
            jtp_map_x = jtp_you_x;
            jtp_map_y = jtp_you_y;
          }
 
        /* If the hero has moved off-screen, center the view on him/her */
        if ((!jtp_map_is_empty) && (!jtp_is_onscreen(jtp_you_x, jtp_you_y)))
        {
          jtp_map_x = jtp_you_x;
          jtp_map_y = jtp_you_y;   
        }    

        jtp_draw_map(tempwindow, -1, -1);
        tempwindow = jtp_find_window(WIN_MESSAGE);
        if (tempwindow) jtp_draw_messages(tempwindow);
        jtp_show_map_area();
      }
      break;
    case NHW_STATUS:
      jtp_draw_status(tempwindow);
      /* jtp_show_screen(); */
      jtp_show_status_area();
      break;
    case NHW_MESSAGE:
      messages_height = jtp_draw_messages(tempwindow);
      jtp_show_message_area(messages_height);
      break;
    default: 
      return;
      break;
  }
  /* TO DO */
}

void 
jtp_destroy_nhwindow(window)
winid window;
{
  int i;
  jtp_window * tempwindow;

/*  jtp_messagebox("jtp_destroy_nhwindow"); */

  tempwindow = jtp_find_window(window);
  if (!tempwindow) return;
  
  jtp_free_menu(tempwindow->menu);
  jtp_free_buttons(tempwindow->buttons);
  if (tempwindow->rows)
  {
    for (i = 0; i < tempwindow->curs_rows; i++)
      free(tempwindow->rows[i]);
    free(tempwindow->rows);
  }
  jtp_list_remove(jtp_windowlist, tempwindow);
  if (tempwindow->wintype == NHW_MAP)
    jtp_fade_out(0.2);
  free(tempwindow);    
}

void
jtp_start_menu(window)
winid window;
{
  jtp_window * tempwindow;
  
  tempwindow = jtp_find_window(window);
  if ((!tempwindow) || (tempwindow->wintype != NHW_MENU)) return;
  
  /* If the window is set to textual data, it can't display menu data */
  if ((tempwindow->menu) && (tempwindow->menu->content_is_text)) return;

  /* Clean up previous menu */
  if (tempwindow->menu) jtp_free_menu(tempwindow->menu);
  tempwindow->menu = (jtp_menu *)malloc(sizeof(jtp_menu));
  if (!tempwindow->menu)
  {
    jtp_write_log_message("ERROR: Out of memory! - location 4\n");
    jtp_exit_graphics(); exit(1);
  }
  
  tempwindow->menu->items = jtp_list_new();
  tempwindow->menu->prompt = NULL;
  tempwindow->menu->content_is_text = 0;    /* True menu content */  
}

void
jtp_add_menu(window, glyph, identifier, accelerator, groupacc, attr, str, preselected)
winid window;
int glyph;
const anything *identifier;
char accelerator;
char groupacc;
int attr;
const char *str;
boolean preselected;
{
  jtp_window * tempwindow;
  jtp_menuitem * tempmenuitem;
  char tempbuffer[1024];
  
  tempwindow = jtp_find_window(window);
  if (!tempwindow)
  {
    jtp_messagebox("Error: Window ID not found!\n");
    return;
  }  
  if (!tempwindow->menu)
  {
    jtp_messagebox("Error: Window menu not found!\n");
    return;
  }  
  
  tempmenuitem = (jtp_menuitem *)malloc(sizeof(jtp_menuitem));
  if (!tempmenuitem)
  {
    jtp_write_log_message("ERROR: Out of memory! - location 5\n");
    jtp_exit_graphics(); exit(1);
  }
  
  if ((!identifier) || (!identifier->a_void))
    tempmenuitem->count = JTP_NOT_SELECTABLE;
  else 
  {
    tempmenuitem->id = *identifier; 
    tempmenuitem->count = -1; 
  }
  
  if (preselected) tempmenuitem->selected = TRUE;
  else tempmenuitem->selected = FALSE;
  tempmenuitem->accelerator = accelerator;
  tempmenuitem->glyph = glyph;

  if (accelerator) sprintf(tempbuffer, "[%c] - %s", accelerator, str);
  else sprintf(tempbuffer, "%s", str);
  
  tempmenuitem->text = (char *)malloc(strlen(tempbuffer)+1);
  if (!tempmenuitem->text)
  {
    jtp_write_log_message("ERROR: Out of memory! - location 6\n");
    jtp_exit_graphics(); exit(1); 
  }
  memcpy(tempmenuitem->text, tempbuffer, strlen(tempbuffer)+1);
  

  jtp_list_reset(tempwindow->menu->items);
  while (jtp_list_current(tempwindow->menu->items))
    jtp_list_advance(tempwindow->menu->items);
  jtp_list_add(tempwindow->menu->items, tempmenuitem);

/*  jtp_messagebox(tempmenuitem->text);  */
}

void
jtp_end_menu(window, prompt)
winid window;
const char *prompt;
{
  jtp_window * tempwindow;
  jtp_button * tempbutton;
  jtp_menuitem * tempmenuitem;
  int n_accelerators = 0;
  int temp_accelerator;
  int used_accelerators[1024];
  char tempbuffer[1024];
  
  tempwindow = jtp_find_window(window);
  if (!tempwindow) return;
  if (!tempwindow->menu) return;

  if (prompt)
  {
    tempwindow->menu->prompt = (char *)malloc(strlen(prompt)+1);
    if (!tempwindow->menu->prompt)
    {
      jtp_write_log_message("ERROR: Out of memory! - location 7\n");
      jtp_exit_graphics(); exit(1);
    }
    
    memcpy(tempwindow->menu->prompt, prompt, strlen(prompt)+1);
  }
  else tempwindow->menu->prompt = NULL;

  /* Clean up previous buttons */
  if (tempwindow->buttons)
    jtp_free_buttons(tempwindow->buttons);
  tempwindow->buttons = jtp_list_new();

  /* If the content is textual, add a Continue button */
  if (tempwindow->menu->content_is_text)
  {
    tempbutton = (jtp_button *)malloc(sizeof(jtp_button));
    if (!tempbutton) 
    { 
      jtp_write_log_message("ERROR: Out of memory! - location 8\n");
      jtp_exit_graphics(); exit(1); 
    }

    tempbutton->text = (char *)malloc(strlen("Continue")+1);
    if (!tempbutton->text)
    { 
      jtp_write_log_message("ERROR: Out of memory! - location 9\n");
      jtp_exit_graphics(); exit(1); 
    }

    strcpy(tempbutton->text, "Continue");
    tempbutton->id = 0;
    tempbutton->accelerator = '\r'; /* Enter */
    jtp_list_reset(tempwindow->buttons);
    jtp_list_add(tempwindow->buttons, tempbutton);
  }
  else /* For non-textual (possibly selectable) content, add Accept and Cancel buttons */
  {
    /* Add a Cancel button */
    tempbutton = (jtp_button *)malloc(sizeof(jtp_button));
    if (!tempbutton)
    {
      jtp_write_log_message("ERROR: Out of memory! - location 8\n");
      jtp_exit_graphics(); exit(1); 
    }
    
    tempbutton->text = (char *)malloc(strlen("Continue")+1);
    if (!tempbutton->text)
    { 
      jtp_write_log_message("ERROR: Out of memory! - location 9\n");
      jtp_exit_graphics(); exit(1); 
    }

    strcpy(tempbutton->text, "Cancel");
    tempbutton->id = 0;
    tempbutton->accelerator = 27; /* ESC */
    jtp_list_reset(tempwindow->buttons);
    jtp_list_add(tempwindow->buttons, tempbutton);

    /* Add an Accept button */
    tempbutton = (jtp_button *)malloc(sizeof(jtp_button));
    if (!tempbutton)
    {
      jtp_write_log_message("ERROR: Out of memory! - location 10\n");
      jtp_exit_graphics(); exit(1); 
    }
    
    tempbutton->text = (char *)malloc(strlen("Accept")+1);
    if (!tempbutton->text)
    {
      jtp_write_log_message("ERROR: Out of memory! - location 11\n");
      jtp_exit_graphics(); exit(1); 
    }
    
    strcpy(tempbutton->text, "Accept");
    tempbutton->id = 1;
    tempbutton->accelerator = '\r'; /* Enter */    
    jtp_list_reset(tempwindow->buttons);
    jtp_list_add(tempwindow->buttons, tempbutton);
  }

  /*
   * If the content is not textual, make sure that each selectable 
   * item has a keyboard accelerator 
   */
  if (!tempwindow->menu->content_is_text)
  {
    n_accelerators = 0;
    used_accelerators[0]='\0';
    jtp_list_reset(tempwindow->menu->items);
    tempmenuitem = (jtp_menuitem *)jtp_list_current(tempwindow->menu->items);
    while (tempmenuitem)
    {
      if (tempmenuitem->count != JTP_NOT_SELECTABLE)
      {
        temp_accelerator = tempmenuitem->accelerator;
        if (temp_accelerator <= 0)
        {
          temp_accelerator = jtp_find_menu_accelerator(tempmenuitem->text, used_accelerators);
          if (temp_accelerator > 0)
          {
            tempmenuitem->accelerator = temp_accelerator;
            sprintf(tempbuffer, "[%c] - %s", temp_accelerator, tempmenuitem->text);
            /*
              jtp_write_log_message(tempbuffer);
              jtp_write_log_message("\n");
            */
            free(tempmenuitem->text);
            tempmenuitem->text = (char *)malloc(strlen(tempbuffer)+1);
            strcpy(tempmenuitem->text, tempbuffer);
          }
        }
        if (temp_accelerator > 0)
        {
          used_accelerators[n_accelerators] = temp_accelerator;
          used_accelerators[n_accelerators+1] = '\0';
          n_accelerators++;
        }
      }
      jtp_list_advance(tempwindow->menu->items);
      tempmenuitem = (jtp_menuitem *)jtp_list_current(tempwindow->menu->items);
    }
  }

/*  jtp_messagebox("Menu ready.");    */
}


int
jtp_select_menu(window, how, menu_list)
winid window;
int how;
menu_item **menu_list;
{
  jtp_window * tempwindow;
  jtp_menuitem * tempmenuitem;
  int selectedbutton, n_selected;

  if ((window == WIN_INVEN) && (how == PICK_NONE))
  {
    jtp_view_inventory();
    *menu_list = NULL;
    return(-1);
  }

  tempwindow = jtp_find_window(window);
  if (!tempwindow)
  {
    jtp_messagebox("ERROR: Can't find window for menu selection!");
    return;
  }  
  if (!tempwindow->menu)
  { 
    jtp_messagebox("ERROR: Window does not have a menu!");
    return;
  }  

  tempwindow->menu->selectiontype = how;

/*  jtp_messagebox("Entering menu selection");*/

  jtp_get_menu_coordinates(tempwindow);

/*  jtp_messagebox("Coordinates found");*/

  selectedbutton = jtp_get_menu_selection(tempwindow);

  if (selectedbutton == 0) /* Selected cancel */
  {
    /* jtp_messagebox("Selected cancel"); */
    *menu_list = NULL;
    return(-1);
  }
  else
  {
    n_selected = 0;
    
    jtp_list_reset(tempwindow->menu->items);
    tempmenuitem = (jtp_menuitem *)jtp_list_current(tempwindow->menu->items);
    *menu_list = NULL;
    while (tempmenuitem)
    {
      if (tempmenuitem->selected == TRUE)
      {
        n_selected++;
        *menu_list = (menu_item *)realloc(*menu_list, n_selected*sizeof(menu_item));
        (*menu_list)[n_selected-1].item = tempmenuitem->id;
        (*menu_list)[n_selected-1].count = tempmenuitem->count;
      }
      jtp_list_advance(tempwindow->menu->items);
      tempmenuitem = (jtp_menuitem *)jtp_list_current(tempwindow->menu->items);
    }
    
    return(n_selected);
  }  
}

void
jtp_suspend_nhwindows(str)
const char *str;
{
  jtp_messagebox("Suspending the game.");
  if (str)
    jtp_messagebox(str);
}

void
jtp_resume_nhwindows()
{
  jtp_messagebox("Resuming the game.");
}

char
jtp_message_menu(let, how, mesg)
char let;
int how;
const char *mesg;
{
  genl_message_menu(let, how, mesg);
}

void
jtp_mark_synch()
{
}

void
jtp_wait_synch()
{
}

void
jtp_cliparound(x, y)
int x;
int y;
{
}

void
jtp_update_positionbar(features)
char *features;
{
}

void
jtp_nhbell()
{
}

void
jtp_number_pad(state)
int state;
{
}

void
jtp_delay_output()
{
}



/* End of winjtp.c */
