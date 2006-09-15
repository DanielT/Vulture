/*	SCCS Id: @(#)winjtp.c	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "rm.h"
#include "display.h"
#include "dlb.h"
#include <ctype.h>
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
#include "jtp_gen.h"
#include "jtp_gfl.h"
#include "jtp_sdl.h"
#include "winjtp.h"
#include "jtp_tile.h"
#include "jtp_init.h"
#include "jtp_map.h"


/* the only other cmaps beside furniture that have significant height and
 * therefore need to be drawn as part of the second pass with the furniture. */
#define JTP_EXTRA_FURNITURE(typ) ((typ) == DOOR || \
                                 (typ) == DRAWBRIDGE_UP || \
                                 (typ) == DRAWBRIDGE_DOWN || \
                                 (typ) == IRONBARS || \
                                 (typ) == TREE)


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
    WC_PLAYER_SELECTION |
    WC_SPLASH_SCREEN |
    WC_POPUP_DIALOG,
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
	jtp_preference_update
};

/* Is the player using a 'what is' command? */
int jtp_whatis_active = 0;


void win_jtp_init(void)
{
	/* Initialize the function pointer that points to
	 * the kbhit() equivalent, in this case jtp_kbhit()
	 */
#if defined(WIN32)
	nt_kbhit = jtp_kbhit;
#endif	/* defined(WIN32) */
	/*
	 * this function used to call jtp_init_graphics(),
	 * which is too early however, since hackdir[] hasn't been
	 * setup yet, and we might not need to enter graphics mode at all
	 * if only displaying a usage.
	 */
}

void jtp_start_screen(void)
{
}

void jtp_end_screen(void)
{
}


void jtp_outrip(winid window, int how)
{
  jtp_window * tempwindow;
  
  tempwindow = jtp_find_window(window);
  if (!tempwindow) return;
  
  tempwindow->ending_type = how;
}

/* handle options updates here */
void jtp_preference_update(const char *pref)
{
    if (strcmpi(pref, "scroll_amount") == 0)
    {
        jtp_cliparound(u.ux, u.uy);
        return;
    }
    if (strcmpi(pref, "scroll_margin") == 0)
    {
        jtp_cliparound(u.ux, u.uy);
        return;
    }
    if (strcmpi(pref, "hilite_pet") == 0)
    {
        jtp_display_nhwindow(WIN_MAP, TRUE);
        jtp_show_screen();
        return;
    }
    if (strcmpi(pref, "align_message") == 0 ||
        strcmpi(pref, "align_status") == 0)
    {
        jtp_draw_all_windows();
        jtp_show_screen();
        return;
    }
    if (strcmpi(pref, "vary_msgcount") == 0)
    {
        jtp_draw_all_windows();
        jtp_show_screen();
        return;
    }
}

/* clean up and quit */
void bail(const char *mesg)
{
    clearlocks();
    jtp_exit_nhwindows(mesg);
    terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}



/* Low level routines */

void jtp_raw_print(const char *str)
{
	if (str == NULL || *str == '\0')
		return;
	jtp_write_log_message(JTP_LOG_NETHACK, NULL, 0, "%s\n", str);
	
	/* also print to stdout, this allows nethack topten to be displayed */
	printf("%s\n", str);
}


void jtp_raw_print_bold(const char *str)
{
	if (str == NULL || *str == '\0')
		return;
	jtp_write_log_message(JTP_LOG_NETHACK, NULL, 0, "%s\n", str);

	/* also print to stdout, this allows nethack topten to be displayed */
	printf("%s\n", str);	
}


void jtp_curs(winid window, int x, int y)
{
  jtp_window * tempwindow;

  tempwindow = jtp_find_window(window);
  if (!tempwindow) return;
  tempwindow->curs_x = x;
  tempwindow->curs_y = y;
}


void jtp_putstr(winid window, int attr, const char *str)
{
  int i, j;
  jtp_window * tempwindow;
  char tempbuffer[1024];
  anything tempid;

  /* Display error messages immediately */
  if (window == WIN_ERR)
  {
    jtp_messagebox(str);
    return;
  }
  
  tempwindow = jtp_find_window(window);
  if (!tempwindow)
  {
    jtp_write_log_message(JTP_LOG_ERROR, __FILE__, __LINE__, "can't find window!\n");
    jtp_exit_graphics_mode();
    exit(1);
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
    /* Stop inventory (backpack) shortcut action */
    jtp_is_backpack_shortcut_active = 0;

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
      for (i = 0; str[i] != '\0'; i++)
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
        tempwindow->rows[i][0] = 0;
      tempwindow->curs_y = 0;
      tempwindow->curs_x = 0;
    }
    
    strncpy((char *)tempwindow->rows[tempwindow->curs_y], str, tempwindow->curs_cols);
    if ((char *)tempwindow->rows[tempwindow->curs_y] != '\0')
      tempwindow->curs_y++;
    
    /* The status window is redisplayed after both lines have been printed */
    if (tempwindow->curs_y >= tempwindow->curs_rows)
      jtp_display_nhwindow(window, FALSE);
  }
}


void jtp_get_nh_event(void)
{
}


int jtp_nhgetch(void)
{
  int result_key;

  /*
   * I searched through the code, but nhgetch()
   * seems to be only used if the windowing system is
   * _not_ active (checked by !windowinited). So it doesn't
   * need to use map scrolling or other niceties.
   */
  result_key = 0;
  while (result_key == 0 || result_key >= 0x100)
    result_key = jtp_getch();
  return result_key;
}


int jtp_nh_poskey(int *x, int *y, int *mod)
{
  int result_key, nethack_key;
  int i, j;

  /* Play ambient music or sound effects */
  jtp_play_ambient_sound(0);

  /* If the pallette is still faded from the intro sequence, restore it */
  jtp_updatepal(0, 255);

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
      result_key = jtp_getch();
      if (result_key != 0)
         result_key = jtp_translate_key(result_key);
      if (result_key != 0)
        return (result_key);

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
    if (canseeself() == FALSE)
      jtp_messagebox("Can't see self!");
#endif

    result_key = 0;
    while (!result_key)
    {
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
      result_key = jtp_getch();
      if (result_key != 0)
      {
        nethack_key = jtp_translate_key(result_key);
        jtp_play_command_sound(jtp_find_command_nhkey_index(nethack_key));
        if (nethack_key == 0)
        {
          int cmd_index;
          /*
           * There may be key mappings, for there
           * no nethack key binding exists.
           * Currently, this is only true for the 'm' (show map) command,
           * for saving screenshots (which is handled on a lower level,
           * because it otherwise wouldn't work in the inventory menu),
           * and for some wizard commands (which are not handled here).
           */
          cmd_index = jtp_find_command_key_index(result_key);
          switch (cmd_index)
          {
          case JTP_NHCMD_SHOW_MAP:
            jtp_view_map();
            break;
          default:
            break;
          }
          result_key = 0;
        } else
        {
            result_key = nethack_key;
        }
        if (result_key != 0)
           return result_key;
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
  return 0;
}

/* High level routines */

void jtp_print_glyph(winid window, XCHAR_P x, XCHAR_P y, int glyph)
{
    struct obj  *obj;
    struct trap *trap;
    int character, colour;

    /* By this time we need to initialize tile conversion */
    if (!jtp_tile_conversion_initialized)
      jtp_init_glyph_tiles();

    /* check wether we are swallowed, if so, return, because nothing but the swallow graphic will be drawn anyway */
    if (glyph_is_swallow(glyph))
    {
        /* we only SET jtp_map_swallow here; we expect jtp_draw_level to reset it */
        jtp_map_swallow = jtp_engulfmap[(glyph-GLYPH_SWALLOW_OFF) >> 3];
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
            jtp_map_specialeff[y][x] = jtp_cmaptiles[S_vbeam + ((glyph - GLYPH_ZAP_OFF) & 0x03)];
        else if (glyph >= GLYPH_EXPLODE_OFF && glyph < GLYPH_ZAP_OFF)
            /* explosions; the (% 9) causes the fireball animation to be used for all
             * of them, which is rather painful with cone of cold... */
            jtp_map_specialeff[y][x] = jtp_cmaptiles[S_explode1 + (glyph - GLYPH_EXPLODE_OFF) % 9];
        else if (glyph_to_cmap(glyph) >= S_digbeam && glyph_to_cmap(glyph) <= S_ss4)
            /* digbeam, camera flash, boomerang, magic resistance: these are not floor tiles ... */
            jtp_map_specialeff[y][x] = jtp_cmaptiles[glyph_to_cmap(glyph)];
        else
            jtp_map_specialeff[y][x] = JTP_TILE_NONE;
	
        /* We rely on the glyph for monsters, as they are never covered by anything
         * at the start of the turn and dealing with them manually is ugly */
        if (glyph_is_monster(glyph))
        {
            /* we need special attributes, so that we can highlight the pet */
            mapglyph(glyph, &character, &colour, &jtp_map_specialattr[y][x], x, y);
	    
            /* storing monsters like this rather than as monster[x][y] allows us to optimize
             * drawing later on: monsters need to be drawn in a separate pass after everything
             * else, so that their lower edges don't get covered */
            jtp_map_mon[y][x] =  jtp_monster_to_tile(glyph_to_mon(glyph), x, y);
        }
        /* however they may be temporarily obscured by magic effects... */
        else if (!jtp_map_specialeff[y][x])
            jtp_map_mon[y][x] = JTP_TILE_NONE;

        /* visible objects that are not covered by lava */
        if ((obj = vobj_at(x,y)) && !covers_objects(x,y))
        {
            /* glyph_to_obj(obj_to_glyph(foo)) looks like nonsense, but is actually an elegant
             * way of handling hallucination, especially the complicated matter of hallucinated
             * corpses... */
            jtp_map_obj[y][x] = jtp_object_to_tile(glyph_to_obj(obj_to_glyph( obj )));
        }
        /* just to make things intersting, the above does not handle thrown/kicked objects... */
        else if (glyph_is_object(glyph))
        {
            jtp_map_obj[y][x] = jtp_object_to_tile(glyph_to_obj(glyph));
        }
        else
            jtp_map_obj[y][x] = JTP_TILE_NONE;

        /* traps that are not covered by lava and have been seen */
        if ((trap = t_at(x,y)) && !covers_traps(x,y) && trap->tseen)
        {
            /* what_trap handles hallucination */
            jtp_map_trap[y][x] = jtp_cmaptiles[what_trap(trap->ttyp) + S_arrow_trap - 1];
        }
        else
            jtp_map_trap[y][x] = JTP_TILE_NONE;
        
        /* handle furniture: altars, stairs,...
         * furniture is separated out from walls and floors, so that it can be used on any
         * type of floor, rather than producing a discolored blob*/
        if (IS_FURNITURE(level.locations[x][y].typ))
        {
            jtp_map_furniture[y][x] = jtp_cmaptiles[glyph_to_cmap(back_to_glyph(x,y))];
            /* furniture is only found in rooms, so set the background type */
            jtp_map_back[y][x] = JTP_TILE_FLOOR_COBBLESTONE;
        }
        else if (JTP_EXTRA_FURNITURE(level.locations[x][y].typ))
        {
            /* this stuff is not furniture, but we need to draw it at the same time,
             * so we pack it in with the furniture ...*/
            jtp_map_furniture[y][x] = jtp_cmaptiles[glyph_to_cmap(back_to_glyph(x,y))];
            jtp_map_back[y][x] = JTP_TILE_FLOOR_ROUGH;
        }
        else
        {
            jtp_map_furniture[y][x] = JTP_TILE_NONE;
            jtp_map_back[y][x] = jtp_cmaptiles[glyph_to_cmap(back_to_glyph(x,y))];
        }

        /* physically seen tiles cannot be dark */
        jtp_map_tile_is_dark[y][x] = 0;
    }

    /* location seen via some form of magical vision OR
     * when a save is restored*/
    else
    {
        /* if we can see a monster here, it will be re-shown explicitly */
        jtp_map_mon[y][x] = JTP_TILE_NONE;

        /* need to clear special effect explicitly here:
         * if we just got blinded by lightnig, the beam will remain onscreen otherwise */
        jtp_map_specialeff[y][x] = JTP_TILE_NONE;

        /* monsters */
        if (glyph_is_monster(glyph))
        {
            mapglyph(glyph, &character, &colour, &jtp_map_specialattr[y][x], x, y);

            jtp_map_mon[y][x] = jtp_monster_to_tile(glyph_to_mon(glyph), x, y);
	    
            /* if seen telepathically in an unexplored area, it might not have a floor */
            if (jtp_map_back[y][x] == JTP_TILE_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
                jtp_map_back[y][x] = JTP_TILE_FLOOR_NOT_VISIBLE;
        }

        /* handle invisible monsters */
        else if (glyph_is_invisible(glyph))
        {
            mapglyph(glyph, &character, &colour, &jtp_map_specialattr[y][x], x, y);
            jtp_map_mon[y][x] = JTP_TILE_INVISIBLE_MONSTER;
        }

        /* handle monsters you are warned of */
        else if (glyph_is_warning(glyph))
        {
            jtp_map_mon[y][x] = JTP_TILE_WARNLEV_1 + glyph_to_warning(glyph);
            if (jtp_map_back[y][x] == JTP_TILE_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
                jtp_map_back[y][x] = JTP_TILE_FLOOR_NOT_VISIBLE;
        }
	
        /* same as above, for objects */
        else if (glyph_is_object(glyph))
        {
            jtp_map_obj[y][x] = jtp_object_to_tile(glyph_to_obj(glyph));
            if (jtp_map_back[y][x] == JTP_TILE_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
                jtp_map_back[y][x] = JTP_TILE_FLOOR_NOT_VISIBLE;
        }

        /* traps are not seen magically (I think?), so this only triggers when loading a level */
        else if (glyph_is_trap(glyph))
        {
            jtp_map_trap[y][x] = jtp_cmaptiles[glyph_to_cmap(glyph)];
            if (jtp_map_back[y][x] == JTP_TILE_UNMAPPED_AREA)
                jtp_map_back[y][x] = JTP_TILE_FLOOR_NOT_VISIBLE;	    
        }
	
        else if (glyph_is_cmap(glyph))
        {
            /* Nethack shows us the cmaps, therfore there are no traps or objects here */
            jtp_map_obj[y][x] = JTP_TILE_NONE;
            jtp_map_trap[y][x] = JTP_TILE_NONE;

            /* IS_FURNITURE() may be true while the cmap is S_stone for dark rooms  */
            if (IS_FURNITURE(level.locations[x][y].typ) && glyph_to_cmap(glyph) != S_stone)
            {
                jtp_map_furniture[y][x] = jtp_cmaptiles[glyph_to_cmap(glyph)];
                jtp_map_back[y][x] = JTP_TILE_FLOOR_COBBLESTONE;
            }
            else if (JTP_EXTRA_FURNITURE(level.locations[x][y].typ) && glyph_to_cmap(glyph) != S_stone)
            {
                /* JTP_EXTRA_FURNITURE = doors, drawbridges, iron bars
                 * that stuff is not furniture, but we need to draw it at the same time,
                 * so we pack it in with the furniture ...*/
                jtp_map_furniture[y][x] = jtp_cmaptiles[glyph_to_cmap(back_to_glyph(x,y))];
                jtp_map_back[y][x] = JTP_TILE_FLOOR_ROUGH;
            }
            else
            {
                jtp_map_furniture[y][x] = JTP_TILE_NONE;
                jtp_map_back[y][x] = jtp_cmaptiles[glyph_to_cmap(glyph)];
            }
        }

        /* When a save is restored, we are shown a number of glyphs for objects, traps, etc
         * whose background we actually know and can display, even though we can't physically see it*/
        if (level.locations[x][y].seenv != 0 && jtp_map_back[y][x] == JTP_TILE_FLOOR_NOT_VISIBLE)
        {
            if (IS_FURNITURE(level.locations[x][y].typ))
            {
                jtp_map_furniture[y][x] = jtp_cmaptiles[glyph_to_cmap(back_to_glyph(x,y))];
                /* furniture is only found in rooms, so set the background type */
                jtp_map_back[y][x] = JTP_TILE_FLOOR_COBBLESTONE;
            }
            else if (JTP_EXTRA_FURNITURE(level.locations[x][y].typ))
            {
                /* this stuff is not furniture, but we need to draw it at the same time,
                 * so we pack it in with the furniture ...*/
                jtp_map_furniture[y][x] = jtp_cmaptiles[glyph_to_cmap(back_to_glyph(x,y))];
                jtp_map_back[y][x] = JTP_TILE_FLOOR_ROUGH;
            }
            else
            {
                jtp_map_furniture[y][x] = JTP_TILE_NONE;
                jtp_map_back[y][x] = jtp_cmaptiles[glyph_to_cmap(back_to_glyph(x,y))];
            }
        }

        /* handle unlit room tiles; until now we were assuming them to be lit; whereas tiles
         * converted via jtp_cmaptiles are currently JTP_TILE_NONE */
        if ( ((!level.locations[x][y].waslit) && jtp_map_back[y][x] == JTP_TILE_FLOOR_COBBLESTONE) ||
           (level.locations[x][y].typ == ROOM && jtp_map_back[y][x] == JTP_TILE_UNMAPPED_AREA &&
           level.locations[x][y].seenv))
        {
            jtp_map_back[y][x] = JTP_TILE_FLOOR_COBBLESTONE;
            jtp_map_tile_is_dark[y][x] = 1;
        }
        else
            jtp_map_tile_is_dark[y][x] = 0;
    }

    /* Notify the graphics code that the map has changed */
    jtp_map_changed = 1;  
}



char jtp_yn_function(const char *ques, const char *choices, CHAR_P defchoice)
{
    /* Remove keys in buffer */
    jtp_sdl_keybuf_reset();

    if (jtp_is_shortcut_active)
    {
        jtp_is_shortcut_active = 0;
	return(jtp_shortcut_query_response);
    }				
	  
    /* a save: yes/no or ring: right/left question  */
    if (choices)
        return jtp_query_choices(ques, choices, 0);

    /* An "In what direction ..." question */
    if (strncmp(ques,"In what dir", 11) == 0)
        return jtp_query_direction(ques);

    /* default case: What do you want to <foo>, where any key is a valid response */
    return jtp_query_anykey(ques);
}	  


void jtp_getlin(const char *ques, char *input)
{
	if (!jtp_get_input(-1, -1, ques, input))
		strcpy(input, "\033");
}


int jtp_get_ext_cmd(void)
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
    cur_accelerator = 0;
    for (k = 0; extcmdlist[i].ef_txt[k] != '\0'; k++)
    {
      cur_accelerator = tolower(extcmdlist[i].ef_txt[k]);
      acc_found = 1;
      for (j = 0; used_accelerators[j] != '\0'; j++)
        if (used_accelerators[j] == cur_accelerator) acc_found = 0;
      if (!acc_found)
      {
        cur_accelerator = toupper(extcmdlist[i].ef_txt[k]);
        acc_found = 1;
        for (j = 0; used_accelerators[j] != '\0'; j++)
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
        for (j = 0; used_accelerators[j] != '\0'; j++)
          if (used_accelerators[j] == cur_accelerator) acc_found = 0;
        if (acc_found) break;
      }
      /* Pick any available uppercase letter in the alphabet */
      if (!acc_found)
        for (cur_accelerator = 'A'; cur_accelerator <= 'Z'; cur_accelerator++)
        {
          acc_found = 1;
          for (j = 0; used_accelerators[j] != '\0'; j++)
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




void jtp_display_file(const char *fname, BOOLEAN_P complain)
{
  dlb * f;                /* Data librarian */
  int  nlines;
  char ** textlines;
  char tempbuffer[1024];
  int  i;
  winid window;

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
    	OOM(1);
    }

    textlines[nlines-1] = (char *)malloc(strlen(tempbuffer)+1);
    if (!textlines[nlines-1])
    {
    	OOM(1);
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


void jtp_update_inventory(void)
{
}


void jtp_player_selection(void)
{
  jtp_select_player();
}


int jtp_doprev_message(void)
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

void jtp_init_nhwindows(int *argcp, char **argv)
{
  jtp_max_window_id = -1;
  jtp_windowlist = jtp_list_new();

  if (!jtp_init_graphics())
  {
    panic("could not initalize graphic mode");
  }
  /*
   * hide some menu options that are not relevant for us
   */
  set_option_mod_status("altkeyhandler", SET_IN_FILE);
  set_option_mod_status("eight_bit_tty", SET_IN_FILE);
  set_option_mod_status("DECgraphics", SET_IN_FILE);
  set_option_mod_status("IBMgraphics", SET_IN_FILE);

  jtp_show_logo_screen();

  /* Success! */
  iflags.window_inited = TRUE;
}


void jtp_exit_nhwindows(const char *str)
{
  jtp_exit_graphics_mode();
  if (str)
    printf("%s\n", str);
}


winid jtp_create_nhwindow(int type)
{
  int i, j;
  jtp_window * tempwindow;

  tempwindow = (jtp_window *)malloc(sizeof(jtp_window));
  if (!tempwindow)
  {
    OOM(1);
  }

  jtp_max_window_id++;
  tempwindow->id = jtp_max_window_id;
  tempwindow->curs_x = 1;
  tempwindow->curs_y = 0;
  tempwindow->active = 0; /* Not active at creation time */
  switch(type)
  {
#if 0
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
#endif
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
      tempwindow->curs_cols = 100;
      tempwindow->rows = (int **)malloc(tempwindow->curs_rows*sizeof(char *));
      for (i = 0; i < tempwindow->curs_rows; i++)
      {
        tempwindow->rows[i] = (int *)malloc(tempwindow->curs_cols*sizeof(char));
	tempwindow->rows[i][0] = 0;
      }
      tempwindow->curs_x = 0;
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
        OOM(1);
      }
      for (i = 0; i < tempwindow->curs_rows; i++)
      {
        tempwindow->rows[i] = (int *)malloc(tempwindow->curs_cols*sizeof(int));
        if (!tempwindow->rows[i])
        {
          OOM(1);
        }
        for (j = 0; j < tempwindow->curs_cols; j++)
          tempwindow->rows[i][j] = JTP_TILE_NONE;
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


void jtp_clear_nhwindow(winid window)
{
  int i, j;
  jtp_window * tempwindow;

  tempwindow = jtp_find_window(window);
  if (!tempwindow) return;
  if (tempwindow->wintype == NHW_MAP)
  {
    for (i = 0; i < jtp_map_height; i++)
      for (j = 0; j < jtp_map_width; j++)
      {
        jtp_print_glyph(NHW_MAP, j, i, S_stone + GLYPH_CMAP_OFF);
        /* jtp_map_back[i][j] = JTP_TILE_NONE; */
        jtp_map_trap[i][j] = JTP_TILE_NONE;
        jtp_map_furniture[i][j] = JTP_TILE_NONE;
        jtp_map_obj[i][j] = JTP_TILE_NONE;
        jtp_map_mon[i][j] = JTP_TILE_NONE;
        jtp_map_specialeff[i][j] = JTP_TILE_NONE;
        jtp_map_specialattr[i][j] = 0;
        jtp_map_tile_is_dark[i][j] = 1;

	jtp_clear_walls(i, j);
      }
  }
  else
  {
    for (i = 0; i < tempwindow->curs_rows; i++)
      for (j = 0; j < tempwindow->curs_cols; j++)
        tempwindow->rows[i][j] = JTP_TILE_NONE;
  }
}

void jtp_display_nhwindow(winid window, BOOLEAN_P blocking)
{
  jtp_window * tempwindow;
  int  messages_height;
  menu_item * menu_list;    /* Dummy pointer for displaying NHW_MENU windows */


  tempwindow = jtp_find_window(window);
  if (!tempwindow)
  {
    jtp_write_log_message(JTP_LOG_ERROR, __FILE__, __LINE__, "Invalid window!\n");
    jtp_exit_graphics_mode();
    exit(1);
  }

  switch(tempwindow->wintype)
  {
    case NHW_MENU:
      jtp_end_menu(window, NULL);
      jtp_select_menu(window, PICK_NONE, &menu_list);
#if 0
      if (tempwindow->width <= 0) return;
      jtp_draw_window(tempwindow->x, tempwindow->y, tempwindow->width, tempwindow->height);
      jtp_draw_menu(tempwindow->menu);
      jtp_draw_buttons(tempwindow->buttons);
#endif
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
      if ((jtp_map_changed))
      {
        /* If the hero has moved, center the view on him/her (optional) */
        if (jtp_recenter_after_movement)
          if ((jtp_map_x != u.ux) || (jtp_map_y != u.uy))
          {
            jtp_map_x = u.ux;
            jtp_map_y = u.uy;
          }
 
        /* If the hero has moved off-screen, center the view on him/her */
        if ((!jtp_is_onscreen(u.ux, u.uy)))
        {
          jtp_map_x = u.ux;
          jtp_map_y = u.uy;
        }

        jtp_draw_level(tempwindow, -1, -1);
        tempwindow = jtp_find_window(WIN_MESSAGE);
        if (tempwindow) jtp_draw_messages(tempwindow);
        jtp_show_map_area();
      }
      if (blocking)
      {
          /* wait for a keypress, this is monster detection or similar */
          jtp_get_map_input();
          while (!jtp_process_mouseclick() && !jtp_getch())
          {
              jtp_draw_all_windows();
              jtp_show_screen();
              jtp_get_map_input();
          }

          /* process_mouseclick might have tried to initiate travel, but we don't want that here */
          u.tx = u.ux;
          u.ty = u.uy;
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
      break;
  }
  /* TO DO */
}

void jtp_destroy_nhwindow(winid window)
{
  int i;
  jtp_window * tempwindow;

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

void jtp_start_menu(winid window)
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
    OOM(1);
  }
  
  tempwindow->menu->items = jtp_list_new();
  tempwindow->menu->prompt = NULL;
  tempwindow->menu->content_is_text = 0;    /* True menu content */
}

void jtp_add_menu(
					 winid window,
					 int glyph,
					 const ANY_P * identifier,
					 CHAR_P accelerator,
					 CHAR_P groupacc,
					 int attr,
					 const char *str,
					 BOOLEAN_P preselected
)
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
    OOM(1);
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
    OOM(1);
  }
  memcpy(tempmenuitem->text, tempbuffer, strlen(tempbuffer)+1);
  

  jtp_list_reset(tempwindow->menu->items);
  while (jtp_list_current(tempwindow->menu->items))
    jtp_list_advance(tempwindow->menu->items);
  jtp_list_add(tempwindow->menu->items, tempmenuitem);
}

void jtp_end_menu(winid window, const char *prompt)
{
  jtp_window * tempwindow;
  jtp_button * tempbutton;
  jtp_menuitem * tempmenuitem;
  int n_accelerators = 0;
  int temp_accelerator;
  char used_accelerators[1024];
  char tempbuffer[1024];
  
  tempwindow = jtp_find_window(window);
  if (!tempwindow) return;
  if (!tempwindow->menu) return;

  if (prompt)
  {
    tempwindow->menu->prompt = (char *)malloc(strlen(prompt)+1);
    if (!tempwindow->menu->prompt)
    {
      OOM(1);
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
      OOM(1);
    }

    tempbutton->text = (char *)malloc(strlen("Continue")+1);
    if (!tempbutton->text)
    {
      OOM(1);
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
      OOM(1);
    }
    
    tempbutton->text = (char *)malloc(strlen("Continue")+1);
    if (!tempbutton->text)
    {
      OOM(1);
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
      OOM(1);
    }
    
    tempbutton->text = (char *)malloc(strlen("Accept")+1);
    if (!tempbutton->text)
    {
      OOM(1);
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
}


int jtp_select_menu(winid window, int how, menu_item ** menu_list)
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
    return -1;
  }
  if (!tempwindow->menu)
  {
    jtp_messagebox("ERROR: Window does not have a menu!");
    return -1;
  }

  tempwindow->menu->selectiontype = how;

  jtp_get_menu_coordinates(tempwindow);

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

void jtp_suspend_nhwindows(const char *str)
{
  /* FIXME: should switch back from fullscreen, minimize window or whatever */
  jtp_messagebox("Suspending the game.");
  if (str)
    jtp_messagebox(str);
}

void jtp_resume_nhwindows(void)
{
  /* FIXME: undo what was done in suspend */
  jtp_messagebox("Resuming the game.");
}

char jtp_message_menu(CHAR_P let, int how, const char *mesg)
{
  return genl_message_menu(let, how, mesg);
}

void jtp_mark_synch(void)
{
}

void jtp_wait_synch(void)
{
}

#ifdef CLIPPING
void jtp_cliparound(int x, int y)
{
}
#endif	/* CLIPPING */

#ifdef POSITIONBAR
void jtp_update_positionbar(char *features)
{
}
#endif	/* POSITIONBAR */

void jtp_nhbell(void)
{
	/* FIXME */
}

void jtp_number_pad(int state)
{
}

void jtp_delay_output(void)
{
    jtp_msleep(50);
}
