/* NetHack may be freely redistributed.  See license for details. */

#include <ctype.h>

#include <SDL.h>

/* nethack headers */
extern "C" {

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
}

#define TRAVEL_HACK /* XXX This is to be removed once Slash'EM (NetHack?)  fixes the problem of infinite loops */

/* vulture headers */
#include "vulture_main.h"
#include "vulture_gra.h"
#include "vulture_gen.h"
#include "vulture_win.h"
#include "vulture_sdl.h"
#include "vulture_init.h"
#include "vulture_pcmusic.h"
#include "vulture_sound.h"
#include "vulture_mou.h"
#include "vulture_opt.h"

#include "winclass/nhwindow.h"
#include "winclass/anykeydialog.h"
#include "winclass/button.h"
#include "winclass/choicedialog.h"
#include "winclass/dirdialog.h"
#include "winclass/endingwin.h"
#include "winclass/inventory.h"
#include "winclass/levelwin.h"
#include "winclass/messagewin.h"
#include "winclass/statuswin.h"
#include "winclass/mapdata.h"
#include "winclass/map.h"
#include "winclass/minimap.h"

/* Interface definition, for windows.c */
struct window_procs vulture_procs = {
	"vulture",
	WC_COLOR |
	WC_HILITE_PET |
	WC_PLAYER_SELECTION |
	WC_SPLASH_SCREEN |
	WC_POPUP_DIALOG |
	WC_FONT_TEXT,
	WC2_FULLSCREEN,
	vulture_init_nhwindows,
	vulture_player_selection,
	vulture_askname,
	vulture_get_nh_event,
	vulture_exit_nhwindows,
	vulture_suspend_nhwindows,
	vulture_resume_nhwindows,
	vulture_create_nhwindow,
	vulture_clear_nhwindow,
	vulture_display_nhwindow,
	vulture_destroy_nhwindow,
	vulture_curs,
	vulture_putstr,
	vulture_display_file,
	vulture_start_menu,
	vulture_add_menu,
	vulture_end_menu,
	vulture_select_menu,
	vulture_message_menu,
	vulture_update_inventory,
	vulture_mark_synch,
	vulture_wait_synch,
#ifdef CLIPPING
	vulture_cliparound,
#endif
#ifdef POSITIONBAR
	vulture_update_positionbar,
#endif
	vulture_print_glyph,
	vulture_raw_print,
	vulture_raw_print_bold,
	vulture_nhgetch,
	vulture_nh_poskey,
	vulture_nhbell,
	vulture_doprev_message,
	vulture_yn_function,
	vulture_getlin,
	vulture_get_ext_cmd,
	vulture_number_pad,
	vulture_delay_output,
#ifdef CHANGE_COLOR
	vulture_change_colour,
#ifdef MAC
	vulture_change_background,
	vulture_set_font_name,
#endif
	vulture_get_colour_string,
#endif
	vulture_start_screen,
	vulture_end_screen,
	vulture_outrip,
	vulture_preference_update
};


int vulture_whatis_active = 0;

static int vulture_stop_travelling = 0;



/***************************** 
* nethack interface api 
*****************************/

void win_vulture_init(void)
{
#if defined(WIN32)
	/*nt_kbhit = vulture_kbhit;*/ /* this should be unnecessary and we haven't implemented a "vulture_kbhit" anywhere */
#endif
}


void vulture_init_nhwindows(int *argcp, char **argv)
{
	unsigned int mask;

	/* try to chdir to our datadir */
	vulture_chdir_to_datadir(argv[0]);

	if (!vulture_init_graphics())
		panic("could not initalize graphic mode");

#ifdef PCMUSIC
	vulture_pcmusic_init();
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

	/* these _must_ have the right value for vulture to work correctly */
	set_option_mod_status("menu_tab_sep", SET_IN_FILE);

	/* Setting options here makes sure they have the right value
	* as this is done _after_ reading in .nethackrc and co */
	iflags.menu_tab_sep = 1;
	iflags.wc_hilite_pet = 1;

	vulture_show_logo_screen();

	map_data = new mapdata();

	new levelwin(map_data);
	vulture_windows_inited = 1;

	/* Success! */
	iflags.window_inited = TRUE;
}



void vulture_exit_nhwindows(const char * str)
{
	/* destroy any surviving windows */
	delete ROOTWIN;
	delete map_data;

	/* close the application window */
	vulture_exit_graphics_mode();
	if (str)
		printf("%s\n", str);

	/* clean up all the memory we allocated */
	vulture_destroy_graphics();
	
	vulture_write_userconfig();
}



winid vulture_create_nhwindow(int type)
{
	nhwindow *win = new nhwindow(type);

	switch(type) {
		case NHW_STATUS:
			win->impl = new statuswin(NULL);
			break;

		case NHW_MESSAGE:
			win->impl = new messagewin(NULL);
			break;

		case NHW_MAP:
			win->impl = ROOTWIN;
			static_cast<levelwin*>(ROOTWIN)->init();
			break;

		case NHW_MENU:
		case NHW_TEXT:
			break;

		default:
			return WIN_ERR;
	}
	return win->id;
}



void vulture_clear_nhwindow(int winid)
{
	if (!vulture_get_nhwindow(winid))
		return;

	/* this doesn't seem to be used for anything other than the map ... */
	if (winid == WIN_MAP)
		map_data->clear();

	/* nethack also wants to clear WIN_MESSAGE frequently, but we don't do that
	* because we have our own way of handling the message window... */
}


/* helper called by vulture_display_nhwindow */
static void vulture_display_nhmap(window *win, vulture_event *result, BOOLEAN_P blocking)
{
	if (u.uz.dlevel != 0) {
		/* u.uz.dlevel == 0 when the game hasn't been fully initialized yet
		* you can't actually go there, the astral levels have negative numbers */

		/* Center the view on the hero, if necessary */
		if (!vulture_whatis_active && 
		    (vulture_opts.recenter || !levwin->need_recenter(u.ux, u.uy)))
			levwin->set_view(u.ux, u.uy);

		if (mapwin)
			mapwin->need_redraw = 1;

		if (vulture_opts.show_minimap)
			minimapwin->need_redraw = 1;

		win->need_redraw = 1;

		if (flags.travel) {
			if (vulture_event_dispatcher_nonblocking(result, NULL))
				vulture_stop_travelling = 1;
		} else {
			win->draw_windows();
			vulture_refresh_window_region();
		}
	}

	/* blocking is set for things like monster detection, where we wait for an
	* event before returning to a normal state */
	if (blocking) {
		vulture_event dummy;
		/* go into the event+drawing loop until we get a response */
		vulture_event_dispatcher(&dummy, V_RESPOND_POSKEY, NULL);

		/* we may have tried to initiate travel in the event handler, but we don't want that here */
		u.tx = u.ux;
		u.ty = u.uy;
	}
}

void vulture_display_nhwindow(int winid, BOOLEAN_P blocking)
{
	nhwindow *win = vulture_get_nhwindow(winid);
	menu_item *menu_list;    /* Dummy pointer for displaying NHW_MENU windows */
	vulture_event result = {-1, -1, 0, 0};

	if (!win)
		return;

	switch(win->type) {
		case NHW_TEXT:
			if (win->ending_type > 0) {
				int response;

				win->impl = new endingwin(ROOTWIN, win->items, win->ending_type);
				win->impl->need_redraw = 1;
				win->impl->visible = 1;

				/* need to run the eventloop manually */
				vulture_event_dispatcher(&response, V_RESPOND_INT, win->impl);
				
				delete win->impl;
				win->impl = NULL;

				vulture_fade_out(0.5);
				return;
			}
			/* else fall through */

		case NHW_MENU:
			/* note that win->impl is NULL for menu windows! */
			vulture_end_menu(winid, NULL);
			vulture_select_menu(winid, PICK_NONE, &menu_list);
			break;

		case NHW_MAP:
			vulture_display_nhmap(win->impl, &result, blocking);
			break;

		case NHW_STATUS:
			/* status is always visible, so we consider this a request for redraw */
			win->impl->visible = 1;
			win->impl->need_redraw = 1;
			break;

		case NHW_MESSAGE:
			win->impl->need_redraw = 1;
			/* we need to actually draw here so that we get output even when the
			* message window isn't on the current drawing loop */
			win->impl->draw_windows();
			vulture_refresh_window_region();
			break;

		default:
			win->impl->need_redraw = 1;
	}
}


void vulture_destroy_nhwindow(int winid)
{
	nhwindow *win = vulture_get_nhwindow(winid);

	if (winid == WIN_MAP) {
		vulture_fade_out(0.2);
		return;
	}

	else if (win && winid == WIN_STATUS)
		win->impl->visible = 0;

	delete win;
}


void vulture_start_menu(int winid)
{
	nhwindow *win = vulture_get_nhwindow(winid);

	/* sanity checks */
	if(!win)
		return;

	if (win->type != NHW_MENU)
		return;
	
	win->reset();
}


/* add an item to a menu that was started with vulture_start_menu */
void vulture_add_menu(int winid, int glyph, const ANY_P * identifier,
					CHAR_P accelerator, CHAR_P groupacc, int attr,
					const char *str, BOOLEAN_P preselected)
{
	nhwindow *win = vulture_get_nhwindow(winid);
	if (!win)
		return;
	
	if (glyph_is_object(glyph))
		win->has_objects = true;
	
	win->add_menuitem(str, !!preselected, identifier->a_void, accelerator, glyph);
}


/* finalize a menu window and add the title and accelerators */
void vulture_end_menu(int winid, const char *prompt)
{
	nhwindow *win = vulture_get_nhwindow(winid);
	if(!win)
		return;

	/* we (ab)use the prompt as the window title */
	if (prompt)
		win->caption = prompt;
}



int vulture_select_menu(int winid, int how, menu_item **menu_list)
{
	nhwindow *nhwin;
	menuwin *win;
	int response, n_selected;
	vulture_event *queued_event;
	
    bool objwin_ok = ((winid == WIN_INVEN && !vulture_opts.use_standard_inventory) ||
                      (winid != WIN_INVEN && !vulture_opts.use_standard_object_menus));

	nhwin = vulture_get_nhwindow(winid);
	if(!nhwin)
		return -1;


	/* assign accelerators to menuitems, if necessary */
	*menu_list = NULL; /* realloc blows up if this contains a random memory location */

	/* check for an autoresponse to this menu */
	if (how != PICK_NONE && (queued_event = vulture_eventstack_get()) &&
		queued_event->rtype == V_RESPOND_ANY) {
		for (nhwindow::item_iterator iter = nhwin->items.begin();
		     iter != nhwin->items.end(); ++iter) {
			if (iter->accelerator == (char)queued_event->num) {
				*menu_list = (menu_item *)malloc(sizeof(menu_item));
				(*menu_list)[0].item.a_void = (void*)iter->identifier;
				(*menu_list)[0].count = -1;
				return -1;
			}
		}
	}

	if (objwin_ok && nhwin->has_objects) {
		win = new inventory(ROOTWIN, nhwin->items, how, nhwin->id);
	}else {
		win = new menuwin(ROOTWIN, nhwin->items, how);
		if (how == PICK_NONE )
			new button(win, "Continue", V_MENU_ACCEPT, 0);
		else {
      if (how == PICK_ANY )
        new button(win, "Accept", V_MENU_ACCEPT, 0);
			new button(win, "Cancel", V_MENU_CANCEL, 0);
		}
	}

	win->caption = nhwin->caption;
	win->visible = 1;
	win->need_redraw = 1;
	win->layout();

	vulture_event_dispatcher(&response, V_RESPOND_INT, win);

	/* nothing selected, because the window was canceled or no selection was requested */
	if (response == V_MENU_CANCEL || how == PICK_NONE) {
		delete win;
		return -1;
	}

	n_selected = 0;
	
	for (menuwin::selection_iterator iter = win->selection_begin();
	     iter != win->selection_end(); ++iter) {
			n_selected++;
			*menu_list = (menu_item *)realloc(*menu_list, n_selected*sizeof(menu_item));
			(*menu_list)[n_selected-1].item.a_void = (void*)(*iter).identifier;
			(*menu_list)[n_selected-1].count = (*iter).count;
	}

	delete win;
	return n_selected;
}



/***************************** 
* Output functions
*****************************/

void vulture_print_glyph(winid window, XCHAR_P x, XCHAR_P y, int glyph)
{
	map_data->set_glyph(x, y, glyph);
}


void vulture_raw_print(const char *str)
{
	if (str == NULL || *str == '\0')
		return;
	vulture_write_log(V_LOG_NETHACK, NULL, 0, "%s\n", str);

	/* also print to stdout, this allows nethack topten to be displayed */
	printf("%s\n", str);
}



void vulture_raw_print_bold(const char *str)
{
	if (str == NULL || *str == '\0')
		return;
	vulture_write_log(V_LOG_NETHACK, NULL, 0, "%s\n", str);

	/* also print to stdout, this allows nethack topten to be displayed */
	printf("%s\n", str);	
}



void vulture_putstr(int winid, int attr, const char *str)
{
	nhwindow *win;

	if (!str)
		return;

	/* Display error messages immediately */
	if (winid == WIN_ERR) {
		vulture_messagebox(str);
		return;
	}

	win = vulture_get_nhwindow(winid);

	/*
	* For windows of type NHW_MESSAGE, both the messages
	* and their send time are stored.
	*/
	switch (win->type) {
		case NHW_MESSAGE:
			/* If "what's this" is active,skip the help messages
			* associated with NetHack's lookat command. */
			if (vulture_suppress_helpmsg) {
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

			msgwin->add_message(std::string(str));

			/* Play any event sounds associated with this message */
			vulture_play_event_sound(str);

			/* Copy message to toplines[] */
			strcpy(toplines, str);

			/* Redisplay message window */
			vulture_display_nhwindow(winid, FALSE);
			break;

		case NHW_TEXT:
		case NHW_MENU:
			/* Add the new text line as a menu item */
			win->add_menuitem(str, false, NULL, '\0', 0);
			break;

		case NHW_STATUS:
			stwin->parse_statusline(str);
			stwin->need_redraw = 1;
			break;
	}
}



/********************************************
* functions that get input from the user
********************************************/

int vulture_nhgetch(void)
{
	vulture_event * queued_event;
	SDL_Event event;

	/* check eventstack */
	queued_event = vulture_eventstack_get();
	if (queued_event)
		return queued_event->num;

	vulture_wait_key(&event);

	return vulture_translate_key(vulture_make_nh_key(event.key.keysym.sym, event.key.keysym.mod, event.key.keysym.unicode));
}



int vulture_nh_poskey(int *x, int *y, int *mod)
{
	vulture_event result = {-1, -1, 0, 0};

	/* Play ambient music or sound effects */
	vulture_play_ambient_sound(0);

	vulture_event_dispatcher(&result, V_RESPOND_POSKEY, NULL);

	*x = result.x;
	*y = result.y;

	return result.num;
}



char vulture_yn_function(const char *ques, const char *choices, CHAR_P defchoice)
{
	window *win;
	char response;

	/* a save: yes/no or ring: right/left question  */
	if (choices)
		win = new choicedialog(ROOTWIN, ques, choices, 0);

	/* An "In what direction ..." question */
	else if (strstr(ques, "n what direc"))
		win = new dirdialog(ROOTWIN, ques);

	/* default case: What do you want to <foo>, where any key is a valid response */
	else
		win = new anykeydialog(ROOTWIN, ques);

	/* get input for our window */
	vulture_event_dispatcher(&response, V_RESPOND_CHARACTER, win);

	/* clean up */
	delete win;

	return response;
}



void vulture_getlin(const char *ques, char *input)
{
	if (!vulture_get_input(-1, -1, ques, input))
		strcpy(input, "\033");
}



/*************************************
* non windowing-related nh functions
*************************************/

void vulture_outrip(int winid, int how)
{
	nhwindow *win = vulture_get_nhwindow(winid);

	if (!win)
		return;

	win->ending_type = how;
}


/* handle options updates here */
void vulture_preference_update(const char *pref)
{
	if (strcmpi(pref, "hilite_pet") == 0)
		vulture_display_nhwindow(WIN_MAP, FALSE);

	return;
}


/* clean up and quit */
void vulture_bail(const char *mesg)
{
	clearlocks();
	vulture_exit_nhwindows(mesg);
	terminate(EXIT_SUCCESS);
	/*NOTREACHED*/
}


/* display a list of extended commands for the user to pick from */
int vulture_get_ext_cmd(void)
{
	int i, j, len;
	int win;
	int nselected;
	anything id;
	menu_item * selected = NULL;
	char used_accels[128];
	char cur_accelerator = '\0';

// TODO: use functions of class menuwin directly
	win = vulture_create_nhwindow(NHW_MENU);
	vulture_start_menu(win);
	used_accels[0] = '\0';

	/* Add extended commands as menu items */
	for (i = 0; extcmdlist[i].ef_txt != NULL; i++) {
		/* try to find an accelerator that fits the command name */
		cur_accelerator = tolower(extcmdlist[i].ef_txt[0]);

		/* check whether the cosen accel is already in use */
		j = 0;
		while (cur_accelerator != used_accels[j] && used_accels[j])
			j++;

		len = strlen(used_accels);
		if (j < len)
			/* cur_accelerator is already used, so find another */
			cur_accelerator = vulture_find_menu_accelerator(used_accels);
		else {
			/* cur_accelerator is not in use: claim it */
			used_accels[len] = cur_accelerator;
			used_accels[len+1] = '\0';
		}

		/* add the command with the chosen accelerator */
		id.a_int = i + 1;
		vulture_add_menu(win, NO_GLYPH, &id, cur_accelerator, 0,
						ATR_NONE, extcmdlist[i].ef_txt, FALSE);
	}

	vulture_end_menu(win, "Select a command");
	nselected = vulture_select_menu(win, PICK_ONE, &selected);
	vulture_destroy_nhwindow(win);

	id.a_int = 0;

	if (nselected > 0)
		id = selected[0].item;

	free(selected);

	return (id.a_int - 1);
}


/* display a file in a menu window */
void vulture_display_file(const char *fname, BOOLEAN_P complain)
{
	dlb * f;                /* Data librarian */
	char tempbuffer[1024];
	int window;

	/* Read the file */
	f = dlb_fopen(fname, "r");
	if (!f) {
		if (complain == TRUE) {
			sprintf(tempbuffer, "Can't open file [%s].\n", fname);
			vulture_messagebox(tempbuffer);
		}
		return;
	}

	window = vulture_create_nhwindow(NHW_MENU);

	while (dlb_fgets(tempbuffer, 1024, f))
		vulture_putstr(window, ATR_NONE, tempbuffer);

	dlb_fclose(f);


	/* Display the file */
	vulture_display_nhwindow(window, TRUE);

	/* Clean up */
	vulture_destroy_nhwindow(window);
}



int vulture_doprev_message(void)
{
	nhwindow *messages;

	int pos = msgwin->getshown();
	msgwin->setshown(pos+1);

	messages = vulture_get_nhwindow(WIN_MESSAGE);
	messages->impl->need_redraw = 1;
	messages->impl->draw_windows();
	vulture_refresh_window_region();

	return 0;
}


/* delay output is _supposed to_ simply pause for a bit; unfortunately that
* would cause the mouse to freeze, because the eventloop would also pause */
void vulture_delay_output(void)
{
	int delay = 50, origdelay;
	int elapsed;
	int starttick, endtick;
	vulture_event result;
	int funcstart, funcend;
	int loops = 0;

	funcstart = SDL_GetTicks();
	
	/* the time it took to draw the map is subtracted from the delay time;
	* this allows the game to adapt to slow-drawing systems (ie unaccelerated
	* software graphics) */
	if (vulture_map_draw_lastmove == moves) {
		delay -= vulture_map_draw_msecs;
		if (delay < 0)
			delay = 1;
		vulture_map_draw_msecs = 0;
	}
	origdelay = delay;

	/* delay in small increments and run the eventloop in between */
	while (delay > 10) {
		starttick = SDL_GetTicks();

		if (vulture_event_dispatcher_nonblocking(&result, NULL))
			vulture_stop_travelling = 1;

		endtick = SDL_GetTicks();
		elapsed = endtick - starttick;
		if (elapsed < 10) {
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


char vulture_message_menu(CHAR_P let, int how, const char *mesg)
{
	pline("%s", mesg);
	return '\0';
}



/* the textmode ui highlights one position onscreen, this is the cursor
* vulture has no real equivalent, however in some situations we allow
* the mose to be moved via keyboard input */
void vulture_curs(winid window, int x, int y)
{
	/* allow selecting a position via keyboard for look and teleport (both set vulture_whatis_active) */
	if (window == WIN_MAP && vulture_whatis_active) {
		point mappos, mouse;

		mappos.x = x; mappos.y = y;
		mouse = levwin->map_to_mouse( mappos );

		/* move the viewport when the mouse approaches the edge */
		if (mouse.x < 50 || mouse.x > (vulture_screen->w-50) ||
			mouse.y < 50 || mouse.y > (vulture_screen->h-50)) {
			levwin->set_view(x, y);

			/* calculate the new mouse position */
			mouse = levwin->map_to_mouse( mappos );
		}

		vulture_set_mouse_pos( mouse.x, mouse.y );
	}
}


/* called by the core, this function is intended to do window event processing
* we misuse it to stop travelling, either because this was requested, or
* because a loop in the travel algorithm was detected */
void vulture_get_nh_event(void)
{
	static point lastpos[2] = {{-1,-1}, {-1,-1}};

	if (flags.travel) {
		/* if the positon 2 turns ago is identical to the current position,
		* the travel algorithm has hung and travel needs to be canceled */
		if (vulture_stop_travelling ||
			((lastpos[1].x != lastpos[0].x || lastpos[1].y != lastpos[0].y) &&
			lastpos[1].x == u.ux && lastpos[1].y == u.uy)) {
			flags.travel = 0;
			flags.mv = 0;
			u.tx = u.ux;
			u.ty = u.uy;
			lastpos[0].x = lastpos[1].x = -1;
		} else {
			lastpos[1] = lastpos[0];
			lastpos[0].x = u.ux;
			lastpos[0].y = u.uy;
		}
	}
	else
		lastpos[0].x = lastpos[1].x = -1;

	vulture_stop_travelling = 0;
}


/*******************************************************
* utility functions
*******************************************************/

int vulture_find_menu_accelerator(char *used_accelerators)
{
	char acc_found;
	int cur_accelerator;
	unsigned int i, j;
	char acclist[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	/* Find an unused accelerator */
	acc_found = 0;
	cur_accelerator = 0;

	/* Pick any available letter from [a-zA-Z] */
	for (i = 0; i < strlen(acclist); i++) {
		cur_accelerator = acclist[i];

		acc_found = 1;
		for (j = 0; used_accelerators[j] != '\0'; j++)
			if (used_accelerators[j] == cur_accelerator)
				acc_found = 0;
		if (acc_found)
			break;
	}

	if (acc_found) {
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

/* suspend/resume is disabled in vulture, because 
* it makes no sense for a gui app to have */
void vulture_suspend_nhwindows(const char *str) {}
void vulture_resume_nhwindows(void) {}

void vulture_mark_synch(void){}
void vulture_wait_synch(void) {}
void vulture_nhbell(void) {}
void vulture_number_pad(int state) {}
void vulture_update_inventory(void) {}
void vulture_start_screen(void) {}
void vulture_end_screen(void) {}


#ifdef CLIPPING
void vulture_cliparound(int x, int y) {}
#endif

#ifdef POSITIONBAR
void vulture_update_positionbar(char *features) {}
#endif
