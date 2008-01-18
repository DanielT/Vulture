/* Copyright (c) Daniel Thaler, 2006				  */
/* NetHack may be freely redistributed.  See license for details. */

/* system headers */
#include <ctype.h>

/* SDL headers */
#include "SDL.h"
#include "SDL_video.h"

/* nethack headers */
#include "hack.h"
#include "rm.h"
#include "display.h"
#include "dlb.h"
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif

#include "func_tab.h" /* For extended commands list */

#define TRAVEL_HACK /* XXX This is to be removed once Slash'EM (NetHack?)  fixes the problem of infinite loops */

/* vultures headers */
#include "vultures_types.h"
#include "vultures_gametiles.h"
#include "vultures_gra.h"
#include "vultures_gfl.h"
#include "vultures_gen.h"
#include "vultures_win.h"
#include "vultures_win_event.h"
#include "vultures_map.h"
#include "vultures_sdl.h"
#include "vultures_init.h"
#include "vultures_main.h"
#include "vultures_pcmusic.h"
#include "vultures_sound.h"
#include "vultures_tile.h"
#include "vultures_txt.h"
#include "vultures_mou.h"
#include "vultures_opt.h"

#ifdef EXPORT_TILES
# include "vultures_conf.h"
# include "vultures_export_tiles.h"
#endif


/* Interface definition, for windows.c */
struct window_procs vultures_procs = {
    "vultures",
    WC_COLOR |
    WC_HILITE_PET |
    WC_PLAYER_SELECTION |
    WC_SPLASH_SCREEN |
    WC_POPUP_DIALOG |
    WC_FONT_TEXT,
    WC2_FULLSCREEN,
    vultures_init_nhwindows,
    vultures_player_selection,
    vultures_askname,
    vultures_get_nh_event,
    vultures_exit_nhwindows,
    vultures_suspend_nhwindows,
    vultures_resume_nhwindows,
    vultures_create_nhwindow,
    vultures_clear_nhwindow,
    vultures_display_nhwindow,
    vultures_destroy_nhwindow,
    vultures_curs,
    vultures_putstr,
    vultures_display_file,
    vultures_start_menu,
    vultures_add_menu,
    vultures_end_menu,
    vultures_select_menu,
    vultures_message_menu,
    vultures_update_inventory,
    vultures_mark_synch,
    vultures_wait_synch,
#ifdef CLIPPING
    vultures_cliparound,
#endif
#ifdef POSITIONBAR
    vultures_update_positionbar,
#endif
    vultures_print_glyph,
    vultures_raw_print,
    vultures_raw_print_bold,
    vultures_nhgetch,
    vultures_nh_poskey,
    vultures_nhbell,
    vultures_doprev_message,
    vultures_yn_function,
    vultures_getlin,
    vultures_get_ext_cmd,
    vultures_number_pad,
    vultures_delay_output,
#ifdef CHANGE_COLOR
    vultures_change_colour,
#ifdef MAC
    vultures_change_background,
    vultures_set_font_name,
#endif
    vultures_get_colour_string,
#endif
    vultures_start_screen,
    vultures_end_screen,
    vultures_outrip,
	vultures_preference_update
};


int vultures_whatis_active = 0;

static int vultures_stop_travelling = 0;
static int vultures_find_menu_accelerator(char *used_accelerators);



/***************************** 
 * nethack interface api 
 *****************************/

void win_vultures_init(void)
{
#if defined(WIN32)
    /*nt_kbhit = vultures_kbhit;*/ /* this should be unnecessary and we haven't implemented a "vultures_kbhit" anywhere */
#endif
}


void vultures_init_nhwindows(int *argcp, char **argv)
{
    unsigned int mask;

    /* try to chdir to our datadir */
    vultures_chdir_to_datadir(argv[0]);


    if (!vultures_init_graphics())
        panic("could not initalize graphic mode");

#ifdef PCMUSIC
    vultures_pcmusic_init();
#endif

    /*
     * hide some menu options that are not relevant for us
     */
    set_option_mod_status("altkeyhandler", SET_IN_FILE);
    set_option_mod_status("DECgraphics", SET_IN_FILE);
    set_option_mod_status("IBMgraphics", SET_IN_FILE);
    set_option_mod_status("menu_headings", SET_IN_FILE);
    set_option_mod_status("null", SET_IN_FILE);
    set_option_mod_status("boulder", SET_IN_FILE);
    set_option_mod_status("sound", SET_IN_FILE);
    set_option_mod_status("silent", SET_IN_FILE);
    set_option_mod_status("lit_corridor", SET_IN_FILE);
    set_option_mod_status("msghistory", SET_IN_FILE);
    set_option_mod_status("windowtype", SET_IN_FILE);
    set_option_mod_status("perm_invent", SET_IN_FILE);

    mask = WC_ALIGN_MESSAGE | WC_ALIGN_STATUS | WC_ASCII_MAP | WC_COLOR | WC_EIGHT_BIT_IN |
           WC_FONTSIZ_TEXT | WC_MAP_MODE | WC_PLAYER_SELECTION | WC_POPUP_DIALOG |
           WC_PRELOAD_TILES | WC_SCROLL_AMOUNT | WC_SCROLL_MARGIN | WC_TILED_MAP |
           WC_TILE_WIDTH | WC_TILE_HEIGHT | WC_TILE_FILE | WC_INVERSE | WC_VARY_MSGCOUNT |
           WC_WINDOWCOLORS | WC_MOUSE_SUPPORT;

    set_wc_option_mod_status(mask, SET_IN_FILE);


    /* these _must_ have the right value for vultures to work correctly */
    set_option_mod_status("menu_tab_sep", SET_IN_FILE);
//     set_option_mod_status("number_pad", SET_IN_FILE);


    /* Setting options here makes sure they have the right value
     * as this is done _after_ reading in .nethackrc and co */
    iflags.menu_tab_sep = 1;
//     iflags.num_pad = 1;
    iflags.wc_hilite_pet = 1;

#ifdef EXPORT_TILES
    vultures_export_tiles();
#endif

    vultures_show_logo_screen();

    vultures_create_root_window();

    /* Success! */
    iflags.window_inited = TRUE;
}



void vultures_exit_nhwindows(const char * str)
{
    /* destroy any surviving windows */
    vultures_cleanup_windows();

    /* close the application window */
    vultures_exit_graphics_mode();
    if (str)
        printf("%s\n", str);

    /* clean up all the memory we allocated */
    vultures_destroy_graphics();
    
    vultures_write_userconfig();
}



winid vultures_create_nhwindow(int type)
{
    struct window * win, *subwin;

    switch(type)
    {
        case NHW_STATUS:
            win = vultures_create_window_internal(NHW_STATUS, NULL, V_WINTYPE_CUSTOM);
            win->draw = vultures_draw_img;
            win->event_handler = vultures_eventh_status;
            win->pd_type = 1;
            win->pd.image = vultures_load_graphic(NULL, V_FILENAME_STATUS_BAR);
            win->w = win->pd.image->w;
            win->h = win->pd.image->h;
            win->x = 6;
            win->y = vultures_screen->h - (win->h + 6);
            win->menu_id = V_WIN_STATUSBAR;

             /* The enhance symbol: usually invisible, it is shown only when skill enhancement is possible */
            subwin = vultures_create_window_internal(0, NULL, V_WINTYPE_CUSTOM);
            subwin->draw = vultures_draw_img;
            subwin->event_handler = vultures_eventh_enhance;
            subwin->pd_type = 1;
            subwin->pd.image = vultures_load_graphic(NULL, V_FILENAME_ENHANCE);
            subwin->w = subwin->pd.image->w;
            subwin->h = subwin->pd.image->h;
            subwin->x = win->x + win->w;
            subwin->y = win->y - subwin->h;
            subwin->visible = 0;
            subwin->autobg = 1;
            subwin->menu_id = V_WIN_ENHANCE;

            vultures_winid_enhance = subwin->id;
           break;

        case NHW_MESSAGE:
            win = vultures_create_window_internal(NHW_MESSAGE, NULL, V_WINTYPE_CUSTOM);
            win->draw = vultures_draw_messages;
            win->event_handler = vultures_eventh_messages;

            win->pd_type = 1;
            win->pd.image = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                            40, 20, 32,
                                            vultures_px_format->Rmask,
                                            vultures_px_format->Gmask,
                                            vultures_px_format->Bmask, 0);
            SDL_FillRect(win->pd.image, NULL, CLR32_BLACK);
            SDL_SetAlpha(win->pd.image, SDL_SRCALPHA, 128);

            break;

        case NHW_MAP:
            win = vultures_get_window(0);
            win->w = vultures_screen->w;
            win->h = vultures_screen->h;

            /* create the upper scroll hotspot. NOTE that all other scroll hotspots
             * are created earlier in vultures_create_root_window */
            vultures_create_hotspot(20       ,0        ,win->w-40, 20,
                V_HOTSPOT_SCROLL_UP, win, "scroll up");

            /* Toolbar1: inventory, map, cast spell, extended commands */
            subwin = vultures_create_window_internal(0, NULL, V_WINTYPE_CUSTOM);
            subwin->draw = vultures_draw_img;
            subwin->event_handler = vultures_eventh_toolbar;
            subwin->pd_type = 1;
            subwin->pd.image = vultures_load_graphic(NULL, V_FILENAME_TOOLBAR1);
            subwin->w = subwin->pd.image->w;
            subwin->h = subwin->pd.image->h;
            subwin->x = win->w - (subwin->w + 6);
            subwin->y = win->h - (subwin->h*2 + 8);
            subwin->visible = vultures_opts.show_actiontb;
            subwin->menu_id = V_WIN_TOOLBAR1;

            /* Inventory button */
            vultures_create_hotspot(4, 0, 38, 39, V_HOTSPOT_BUTTON_INVENTORY, subwin, "Inventory");
            /* Map button */
            vultures_create_hotspot(44, 0, 38, 39, V_HOTSPOT_BUTTON_MAP, subwin, "Map");
            /* Cast spell button */
            vultures_create_hotspot(84, 0, 38, 39, V_HOTSPOT_BUTTON_SPELLBOOK, subwin, "Cast spell");
            /* Extended commands */
            vultures_create_hotspot(124, 0, 38, 39, V_HOTSPOT_BUTTON_EXTENDED, subwin, "Extended commands");
            /* Show Discoveries */
            vultures_create_hotspot(164, 0, 38, 39, V_HOTSPOT_BUTTON_DISCOVERIES, subwin, "Show discoveries");


            /* Toolbar 2: look at, previous messages, options, help */
            subwin = vultures_create_window_internal(0, NULL, V_WINTYPE_CUSTOM);
            subwin->draw = vultures_draw_img;
            subwin->event_handler = vultures_eventh_toolbar;
            subwin->pd_type = 1;
            subwin->pd.image = vultures_load_graphic(NULL, V_FILENAME_TOOLBAR2);
            subwin->w = subwin->pd.image->w;
            subwin->h = subwin->pd.image->h;
            subwin->x = win->w - (subwin->w + 6);
            subwin->y = win->h - (subwin->h + 6);
            subwin->visible = vultures_opts.show_helptb;
            subwin->menu_id = V_WIN_TOOLBAR2;

            /* Look at button */
            vultures_create_hotspot(4, 0, 38, 39, V_HOTSPOT_BUTTON_LOOK, subwin, "Look");
            /* Messages button */
            vultures_create_hotspot(44, 0, 38, 39, V_HOTSPOT_BUTTON_MESSAGES, subwin, "Old messages");
            /* Options button */
            vultures_create_hotspot(84, 0, 38, 39, V_HOTSPOT_BUTTON_OPTIONS, subwin, "Options");
            /* Options button */
            vultures_create_hotspot(124, 0, 38, 39, V_HOTSPOT_BUTTON_IFOPTIONS, subwin, "Interface Options");
            /* Help button */
            vultures_create_hotspot(164, 0, 38, 39, V_HOTSPOT_BUTTON_HELP, subwin, "Help");

            break;

        case NHW_MENU:
        case NHW_TEXT:
            win = vultures_create_window_internal(type, NULL, V_WINTYPE_MAIN);
            win->visible = 0;
            win->draw = vultures_draw_menu;
            break;

        default:
            return WIN_ERR;
    }
    return win->id;
}



void vultures_clear_nhwindow(int winid)
{
    if (!vultures_get_window(winid))
        return;

    /* this doesn't seem to be used for anything other than the map ... */
    if (winid == WIN_MAP)
        vultures_clear_map();

    /* nethack also wants to clear WIN_MESSAGE frequently, but we don't do that
     * because we have our own way of handling the message window... */
}



void vultures_display_nhwindow(int winid, BOOLEAN_P blocking)
{
    struct window * win = vultures_get_window(winid);
    menu_item * menu_list;    /* Dummy pointer for displaying NHW_MENU windows */
    vultures_event result = {-1,-1,0};

    if (!win)
        return;

    switch(win->nh_type)
    {
        case NHW_TEXT:
            if (win->pd.ending_type > 0)
            {
                vultures_show_ending(win);
                return;
            }
            /* else fall through */

        case NHW_MENU:
            win->need_redraw = 1;
            win->visible = 1;
            vultures_end_menu(winid, NULL);
            vultures_select_menu(winid, PICK_NONE, &menu_list);
            break;

        case NHW_MAP:
            if (u.uz.dlevel != 0)
            {
               /* u.uz.dlevel == 0 when the game hasn't been fully initialized yet
                * you can't actually go there, the astral levels have negative numbers */

               /* Center the view on the hero, if necessary */
                if (!vultures_whatis_active && (vultures_opts.recenter || !vultures_need_recenter(u.ux, u.uy)))
                {
                    vultures_view_x = u.ux;
                    vultures_view_y = u.uy;
                }

                if (vultures_winid_map)
                    vultures_get_window(vultures_winid_map)->need_redraw = 1;

                if (vultures_opts.show_minimap)
                    vultures_get_window(vultures_winid_minimap)->need_redraw = 1;

                win->need_redraw = 1;

                if (flags.travel)
                {
                    if (vultures_event_dispatcher_nonblocking(&result, NULL))
                        vultures_stop_travelling = 1;
                }
                else
                {
                    vultures_draw_windows(win);
                    vultures_refresh_window_region();
                }
            }

            if (blocking)
            {
                vultures_event dummy;
                /* go into the event+drawing loop until we get a response */
                vultures_event_dispatcher(&dummy, V_RESPOND_POSKEY, NULL);

                /* we have tried to initiate travel in the event handler, but we don't want that here */
                u.tx = u.ux;
                u.ty = u.uy;
            }
            break;

        case NHW_STATUS:
            win->visible = 1;
            win->need_redraw = 1;
            break;

        case NHW_MESSAGE:
            win->need_redraw = 1;
            /* we need to actually draw here so that we get output even when the
             * message window isn't on the current drawing loop */
            vultures_draw_windows(win);
            vultures_refresh_window_region();
            break;

        default:
            win->need_redraw = 1;
    }
}


void vultures_destroy_nhwindow(int winid)
{
    struct window * win = vultures_get_window(winid);

    if (winid == WIN_MAP)
    {
        vultures_fade_out(0.2);
        return;
    }

    else if (win && winid == WIN_STATUS)
        win->visible = 0;

    vultures_destroy_window_internal(win);
}



void vultures_start_menu(int winid)
{
    struct window * win = vultures_get_window(winid);

    /* sanity checks */
    if(!win)
        return;

    if (win->nh_type != NHW_MENU)
        return;

    /* clean up previous contents */
    while (win->first_child)
    {
        win->first_child->visible = 0;
        vultures_destroy_window_internal(win->first_child);
    }

    /* we need a specialzed draw function, too
     * the standard "main window" function doesn't deal
     * with the complexity added by  scrollbars */
    win->draw = vultures_draw_menu;

    win->content_is_text = 0;
}



void vultures_add_menu(int winid, int glyph, const ANY_P * identifier,
                       CHAR_P accelerator, CHAR_P groupacc, int attr,
                       const char *str, BOOLEAN_P preselected)
{
    int type;
    struct window * win = vultures_get_window(winid);
    struct window * elem;

    if (!win)
        return;

    if (!identifier || !identifier->a_void)
        type = V_WINTYPE_TEXT;
    else
        type = V_WINTYPE_OPTION;

    elem = vultures_create_window_internal(0, win, type);

    if (identifier && identifier->a_void)
        elem->menu_id_v = identifier->a_void;

    if (preselected)
        elem->selected = 1;

    elem->accelerator = accelerator;
    elem->scrollable = 1;

    if (str)
        elem->caption = strdup(str);

}



void vultures_end_menu(int winid, const char *prompt)
{
    struct window * win, *win_elem;
    int new_accel;
    char used_accels[128]; /* 65 should be max # of accels, the other 63 bytes are safety :P */
    char *str;

    win = vultures_get_window(winid);

    if(!win)
        return;

    if (prompt)
    {
        win->caption = malloc(strlen(prompt)+1);
        strcpy(win->caption, prompt);
    }

    /* assign accelerators to menuitems, if necessary */
    if (!win->content_is_text)
    {
        used_accels[0] = '\0';
        win_elem = win->first_child;

        while (win_elem)
        {
            if (win_elem->accelerator == 0 && win_elem->v_type == V_WINTYPE_OPTION)
            {
                new_accel = vultures_find_menu_accelerator(used_accels);
                if (new_accel >= 0)
                    win_elem->accelerator = new_accel;
            }

            /* make the accelerator part of the caption */
            if (win_elem->caption && win_elem->accelerator)
            {
                str = malloc(strlen(win_elem->caption) + 7);
                sprintf(str, "[%c] - %s", win_elem->accelerator, win_elem->caption);
                free(win_elem->caption);
                win_elem->caption = str;
            }

            win_elem = win_elem->sib_next;
        }
    }
}



int vultures_select_menu(int winid, int how, menu_item ** menu_list)
{
    struct window *win, *win_elem;
    int response, n_selected;
    vultures_event * queued_event;

    win = vultures_get_window(winid);

    if(!win)
        return -1;

    win->select_how = how;
    *menu_list = NULL; /* realloc blows up if this contains a random memory location */

    if (winid == WIN_INVEN && how == PICK_NONE && !vultures_opts.use_standard_inventory)
    {
        win->visible = 1;
        win->need_redraw = 1;
        win->draw = vultures_draw_inventory;
        win->event_handler = vultures_eventh_inventory;
        win->pd.inv_page = 0; /* current page number */

        vultures_layout_inventory(win);

        vultures_event_dispatcher(&response, V_RESPOND_INT, win);

        vultures_hide_window(win);

        return -1;
    }

    /* check for an autoresponse to this menu */
    if ((queued_event = vultures_eventstack_get()) && queued_event->rtype == V_RESPOND_ANY)
    {
        win_elem = vultures_accel_to_win(win, (char)queued_event->num);
        if (win_elem)
        {
            *menu_list = malloc(sizeof(menu_item));
            (*menu_list)[0].item.a_void = win_elem->menu_id_v;
            (*menu_list)[0].count = -1; 
            return 1;
        }
    }

    if ( win->content_is_text || how == PICK_NONE )
    {
        win_elem = vultures_create_window_internal(0, win, V_WINTYPE_BUTTON);
        win_elem->caption = strdup("Continue");
        win_elem->menu_id = 1;
    }
    else
    {
        win_elem = vultures_create_window_internal(0, win, V_WINTYPE_BUTTON);
        win_elem->caption = strdup("Accept");
        win_elem->menu_id = 1;

        win_elem = vultures_create_window_internal(0, win, V_WINTYPE_BUTTON);
        win_elem->caption = strdup("Cancel");
        win_elem->menu_id = -1;
    }


    /* set up an event handler */
    win->event_handler = vultures_eventh_menu;

    /* assign sizes and positions; create scrollbar if necessary */
    vultures_layout_menu(win);

    win->visible = 1;
    win->need_redraw = 1;
    win->abs_x = win->parent->abs_x + win->x;
    win->abs_y = win->parent->abs_y + win->y;

    vultures_event_dispatcher(&response, V_RESPOND_INT, win);

    /* don't destroy the window here, nethack will either call destroy_nhwindow on it
     * or re-use it and call start_menu. However we do need to get it off the screen */
    vultures_hide_window(win);

    if (response == -1)
        /* canceled */
        return -1;

    n_selected = 0;
    win_elem = win->first_child;

    while (win_elem)
    {
        if (win_elem->selected)
        {
            n_selected++;
            *menu_list = realloc(*menu_list, n_selected*sizeof(menu_item));
            (*menu_list)[n_selected-1].item.a_void = win_elem->menu_id_v;
            (*menu_list)[n_selected-1].count = -1; /* TODO: we don't (never have actually) allow a
                                                    * count in menus; for example it should be possible
                                                    * to say put 5 (of a stack of 7) into the BoH */
        }

        win_elem = win_elem->sib_next;
    }

    return n_selected;
}



/***************************** 
 * Output functions
 *****************************/

void vultures_raw_print(const char *str)
{
    if (str == NULL || *str == '\0')
        return;
    vultures_write_log(V_LOG_NETHACK, NULL, 0, "%s\n", str);

    /* also print to stdout, this allows nethack topten to be displayed */
    printf("%s\n", str);
}



void vultures_raw_print_bold(const char *str)
{
    if (str == NULL || *str == '\0')
        return;
    vultures_write_log(V_LOG_NETHACK, NULL, 0, "%s\n", str);

    /* also print to stdout, this allows nethack topten to be displayed */
    printf("%s\n", str);	
}



void vultures_putstr(int winid, int attr, const char *str)
{
    struct window * win;
    ANY_P menuid;

    if (!str)
        return;

    /* Display error messages immediately */
    if (winid == WIN_ERR)
    {
        vultures_messagebox(str);
        return;
    }

    win = vultures_get_window(winid);

    /*
     * For windows of type NHW_MESSAGE, both the messages
     * and their send time are stored.
     */
    switch (win->nh_type)
    {
        case NHW_MESSAGE:
            /* If "what's this" is active,skip the help messages
            * associated with NetHack's lookat command. */
            if (vultures_suppress_helpmsg)
            {
                /* Skip help line [Please move the cursor to an unknown object.] */
                if (strncmp(str, "Please move", 11) == 0) return;
                /* Skip help line [(For instructions type a ?)] */
                if (strncmp(str, "(For instru", 11) == 0) return;
                /* Skip help line [Pick an object.] */
                if (strncmp(str, "Pick an obj", 11) == 0) return;
            }

            /* Skip line [Done.] */
            if (strncmp(str, "Done.", 5) == 0)
                return;

            vultures_messages_add(str);

            /* Play any event sounds associated with this message */
            vultures_play_event_sound(str);

            /* Copy message to toplines[] */
            strcpy(toplines, str);

            /* Redisplay message window */
            vultures_display_nhwindow(winid, FALSE);
            break;
            break;

        case NHW_TEXT:
        case NHW_MENU:
            win->content_is_text = 1;    /* Text content */

            /* Add the new text line as a menu item */
            menuid.a_void = 0; /* Since text lines can't be selected anyway, these can be the same */
            vultures_add_menu(winid, NO_GLYPH, &menuid, 0, 0, ATR_NONE, str, FALSE);
            break;

        case NHW_STATUS:
            vultures_parse_statusline(win, str);
            win->need_redraw = 1;
            break;
    }
}



/********************************************
 * functions that get input from the user
 ********************************************/

int vultures_nhgetch(void)
{
    vultures_event * queued_event;
    SDL_Event event;

    /* check eventstack */
    queued_event = vultures_eventstack_get();
    if (queued_event)
        return queued_event->num;

    vultures_wait_key(&event);

    return vultures_translate_key(vultures_convertkey_sdl2nh(&event.key.keysym));
}



int vultures_nh_poskey(int *x, int *y, int *mod)
{
    vultures_event result = {-1,-1,0};

    /* Play ambient music or sound effects */
    vultures_play_ambient_sound(0);

    vultures_event_dispatcher(&result, V_RESPOND_POSKEY, NULL);

    *x = result.x;
    *y = result.y;

    return result.num;
}



char vultures_yn_function(const char *ques, const char *choices, CHAR_P defchoice)
{
    struct window * win;
    char response;

    /* a save: yes/no or ring: right/left question  */
    if (choices)
        win = vultures_query_choices(ques, choices, 0);

    /* An "In what direction ..." question */
    else if (strstr(ques, "n what direc"))
        win = vultures_query_direction(ques);

    /* default case: What do you want to <foo>, where any key is a valid response */
    else
        win = vultures_query_anykey(ques);

    /* get input for our window */
    win->abs_x = win->parent->abs_x + win->x;
    win->abs_y = win->parent->abs_y + win->y;
    vultures_event_dispatcher(&response, V_RESPOND_CHARACTER, win);

    /* clean up */
    vultures_destroy_window_internal(win);

    return response;
}



void vultures_getlin(const char *ques, char *input)
{
    if (!vultures_get_input(-1, -1, ques, input))
        strcpy(input, "\033");
}



/*************************************
 * non windowing-related nh functions
 *************************************/

void vultures_outrip(int winid, int how)
{
    struct window * win = vultures_get_window(winid);

    if (!win)
        return;

    win->pd.ending_type = (how+1);
}


/* handle options updates here */
void vultures_preference_update(const char *pref)
{
    if (strcmpi(pref, "hilite_pet") == 0)
        vultures_display_nhwindow(WIN_MAP, FALSE);

    return;
}


/* clean up and quit */
void vultures_bail(const char *mesg)
{
    clearlocks();
    vultures_exit_nhwindows(mesg);
    terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}


int vultures_get_ext_cmd(void)
{
    int i, j, len;
    int win;
    int nselected;
    anything id;
    menu_item * selected = NULL;
    char used_accels[128];
    char cur_accelerator;

    win = vultures_create_nhwindow(NHW_MENU);
    vultures_start_menu(win);
    used_accels[0] = '\0';

    /* Add extended commands as menu items */
    for (i = 0; extcmdlist[i].ef_txt != NULL; i++)
    {
        cur_accelerator = tolower(extcmdlist[i].ef_txt[0]);

        j = 0;
        while (cur_accelerator != used_accels[j] && used_accels[j])
            j++;

        len = strlen(used_accels);
        if (j < len)
            cur_accelerator = vultures_find_menu_accelerator(used_accels);
        else
        {
            used_accels[len] = cur_accelerator;
            used_accels[len+1] = '\0';
        }

        id.a_int = i + 1;
        vultures_add_menu(win, NO_GLYPH, &id, cur_accelerator, 0,
                          ATR_NONE, extcmdlist[i].ef_txt, FALSE);
    }

    vultures_end_menu(win, "Select a command");
    nselected = vultures_select_menu(win, PICK_ONE, &selected);
    vultures_destroy_nhwindow(win);

    id.a_int = 0;

    if (nselected > 0)
        id = selected[0].item;

    free(selected);

    return (id.a_int - 1);
}




void vultures_display_file(const char *fname, BOOLEAN_P complain)
{
    dlb * f;                /* Data librarian */
    char tempbuffer[1024];
    int window;

    /* Read the file */
    f = dlb_fopen(fname, "r");
    if (!f)
    {
        if (complain == TRUE)
        {
            sprintf(tempbuffer, "Can't open file [%s].\n", fname);
            vultures_messagebox(tempbuffer);
        }
        return;
    }

    window = vultures_create_nhwindow(NHW_MENU);

    while (dlb_fgets(tempbuffer, 1024, f))
        vultures_putstr(window, ATR_NONE, tempbuffer);

    dlb_fclose(f);


    /* Display the file */
    vultures_display_nhwindow(window, TRUE);

    /* Clean up */
    vultures_destroy_nhwindow(window);
}



int vultures_doprev_message(void)
{
    struct window * messages;

    int pos = vultures_messages_getshown();
    vultures_messages_setshown(pos+1);

    messages = vultures_get_window(WIN_MESSAGE);
    messages->need_redraw = 1;
    vultures_draw_windows(messages);
    vultures_refresh_window_region();

    return 0;
}


void vultures_delay_output(void)
{
    int delay = 50, origdelay;
    int elapsed;
    int starttick, endtick;
    vultures_event result;
    int funcstart, funcend;
    int loops = 0;

    funcstart = SDL_GetTicks();

    if (vultures_map_draw_lastmove == moves)
    {
        delay -= vultures_map_draw_msecs;
        if (delay < 0)
            delay = 1;
        vultures_map_draw_msecs = 0;
    }
    origdelay = delay;

    while (delay > 10)
    {
        starttick = SDL_GetTicks();

        if (vultures_event_dispatcher_nonblocking(&result, NULL))
            vultures_stop_travelling = 1;

        endtick = SDL_GetTicks();
        elapsed = endtick - starttick;
        if (elapsed < 10)
        {
            SDL_Delay(10 - elapsed);
            elapsed = 10;
        }
        delay -= elapsed;
        loops++;
    }

    funcend = SDL_GetTicks();
    elapsed = funcend - funcstart;
    
    /* we use >5 in the condition rather than the theoretically more correct >0
     * because we expect a delay granularity or 10 ms. By using >5 we should get
     * the correct delay on average ...*/
    if (origdelay - elapsed > 5)
        SDL_Delay(origdelay - elapsed);
}


char vultures_message_menu(CHAR_P let, int how, const char *mesg)
{
    pline("%s", mesg);
    return '\0';
}



void vultures_curs(winid window, int x, int y)
{
    if ( window == WIN_MAP && vultures_whatis_active)
    {
        point mappos, mouse;

        mappos.x = x; mappos.y = y;
        mouse = vultures_map_to_mouse( mappos );

        if (mouse.x < 50 || mouse.x > (vultures_screen->w-50) ||
            mouse.y < 50 || mouse.y > (vultures_screen->h-50))
        {
            vultures_view_x = x;
            vultures_view_y = y;

            /* calculate the new mouse position */
            mouse = vultures_map_to_mouse( mappos );
        }

        vultures_set_mouse_pos( mouse.x, mouse.y );
    }
}



void vultures_get_nh_event(void)
{
    static point lastpos[2] = {{-1,-1}, {-1,-1}};

    if (flags.travel)
    {
        if (vultures_stop_travelling ||
            ((lastpos[1].x != lastpos[0].x || lastpos[1].y != lastpos[0].y) &&
            lastpos[1].x == u.ux && lastpos[1].y == u.uy))
        {
            flags.travel = 0;
            flags.mv = 0;
            u.tx = u.ux;
            u.ty = u.uy;
            lastpos[0].x = lastpos[1].x = -1;
        }
        else
        {
            lastpos[1] = lastpos[0];
            lastpos[0].x = u.ux;
            lastpos[0].y = u.uy;
        }
    }
    else
        lastpos[0].x = lastpos[1].x = -1;

    vultures_stop_travelling = 0;
}




/*******************************************************
 * utility functions
 *******************************************************/


static int vultures_find_menu_accelerator(char *used_accelerators)
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
        return cur_accelerator;
    }

    return -1;
}


/*******************************************************
 * unsupported/unnecessary functions of the nethack api
 *******************************************************/

/* suspend/resume is disabled in vultures, because 
 * it makes no sense for a gui app to have */
void vultures_suspend_nhwindows(const char *str) {}
void vultures_resume_nhwindows(void) {}

void vultures_mark_synch(void){}
void vultures_wait_synch(void) {}
void vultures_nhbell(void) {}
void vultures_number_pad(int state) {}
void vultures_update_inventory(void) {}
void vultures_start_screen(void) {}
void vultures_end_screen(void) {}


#ifdef CLIPPING
void vultures_cliparound(int x, int y) {}
#endif

#ifdef POSITIONBAR
void vultures_update_positionbar(char *features) {}
#endif


