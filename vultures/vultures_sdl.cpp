/* NetHack may be freely redistributed.  See license for details. */

/*-------------------------------------------------------------------
vultures_sdl.c : SDL API calls for Vulture's windowing system.
Requires SDL 1.1 or newer.
-------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#ifdef __GNUC__
#include <unistd.h>
#endif
#include <stdarg.h>
#include "vultures_sound.h"
#include "SDL.h"
#include "SDL_video.h"
#include "SDL_error.h"
#include "SDL_audio.h"
#include "SDL_mixer.h"
#include "vultures_gen.h"
#include "vultures_gfl.h"
#include "vultures_win.h"
#include "vultures_sdl.h"
#include "vultures_main.h"
#include "vultures_mou.h"
#include "vultures_opt.h"
#include "vultures_tile.h"

#include "winclass/levelwin.h"

#include "date.h"


/* Definitions */


static int fs_message_printed = 0;
static int have_mouse_focus = 1;

/* Graphics objects */
SDL_Surface *vultures_screen;             /* Graphics surface */


Uint32 vultures_timer_callback(Uint32 interval, void * param);
int vultures_handle_global_event(SDL_Event * event);



void vultures_wait_event(SDL_Event * event, int wait_timeout)
{
	int done = 0;
	SDL_TimerID sleeptimer = NULL;

	if (wait_timeout > 0)
		sleeptimer = SDL_AddTimer(wait_timeout, vultures_timer_callback, NULL);


	while (!done)
	{
		event->type = 0;

		while (SDL_PollEvent(event) && event->type == SDL_MOUSEMOTION)
			/* do nothing, we're merely dequeueing unhandled mousemotion events */ ;

		if (!event->type && !SDL_WaitEvent(event))
			continue;

		vultures_handle_global_event(event);

		if (event->type == SDL_KEYDOWN ||
			event->type == SDL_MOUSEBUTTONDOWN ||
			event->type == SDL_MOUSEBUTTONUP ||
			/* send timer events only while the mouse is in the window */
			(event->type == SDL_TIMEREVENT && have_mouse_focus) ||
			event->type == SDL_MOUSEMOTION)
			done = 1;
	}

	SDL_RemoveTimer(sleeptimer);
}



int vultures_poll_event(SDL_Event * event)
{
	int done = 0;

	while (!done)
	{
		event->type = 0;

		while (SDL_PollEvent(event) && event->type == SDL_MOUSEMOTION)
			/* do nothing, we're merely dequeueing unhandled mousemotion events */ ;

		if (!event->type)
			/* no events queued */
			return 0;

		vultures_handle_global_event(event);

		if (event->type == SDL_KEYDOWN ||
			event->type == SDL_MOUSEBUTTONDOWN ||
			event->type == SDL_MOUSEBUTTONUP ||
			event->type == SDL_MOUSEMOTION)
			/* an interesting event, leave the loop */
			done = 1;

		/* else: an uninteresting event, like those handled in vultures_handle_global_event */
	}
	return 1;
}


/* wait for an input event, but stop waiting after wait_timeout milliseconds */
void vultures_wait_input(SDL_Event * event, int wait_timeout)
{
	int done = 0;
	SDL_TimerID sleeptimer = NULL;

	if (wait_timeout > 0)
		sleeptimer = SDL_AddTimer(wait_timeout, vultures_timer_callback, NULL);

	/* loop until we have an interesting event */
	while (!done)
	{
		if (!SDL_WaitEvent(event))
			continue;

		vultures_handle_global_event(event);

		if (event->type == SDL_KEYDOWN ||
			event->type == SDL_MOUSEBUTTONUP ||
			event->type == SDL_TIMEREVENT)
			done = 1;
	}

	SDL_RemoveTimer(sleeptimer);
}




void vultures_wait_key(SDL_Event * event)
{
	int done = 0;

	while (!done)
	{
		if (!SDL_WaitEvent(event))
			continue;

		vultures_handle_global_event(event);

		if (event->type == SDL_KEYDOWN)
			done = 1;
	}
}


Uint32 vultures_timer_callback(Uint32 interval, void * param)
{
	SDL_Event event;
	event.type = SDL_TIMEREVENT;
	event.user.data1 = param;

	SDL_PushEvent(&event);

	return interval;
}



static void vultures_set_fullscreen(void)
{
	SDL_Rect **modes;
	int newheight = 0;
	int newwidth = 0;
	int bestmode = 0;
	int i;

	/* Get available fullscreen/hardware modes */
	modes = SDL_ListModes(NULL, SDL_FULLSCREEN);

	/* Check if our resolution is restricted */
	if(modes == (SDL_Rect **)-1){
		newheight = vultures_opts.height;
		newwidth  = vultures_opts.width;
	}
	else{
		/* find a mode that is >= the dimensions in the config file */
		for (i = 0; modes[i]; i++)
		{
			if (modes[i]->h >= vultures_opts.height &&
				modes[i]->w >= vultures_opts.width)
				bestmode = i;
		}
		newheight = modes[bestmode]->h;
		newwidth  = modes[bestmode]->w;
	}

	vultures_screen = SDL_SetVideoMode(newwidth, newheight, 0, 
									SDL_SWSURFACE | SDL_FULLSCREEN | SDL_ASYNCBLIT);

	vultures_win_resize(newwidth, newheight);
}



static void vultures_set_windowed()
{
	vultures_screen = SDL_SetVideoMode(vultures_opts.width, vultures_opts.height, 0, 
									SDL_SWSURFACE |SDL_ASYNCBLIT | SDL_RESIZABLE);
	vultures_win_resize(vultures_opts.width, vultures_opts.height);
}



void vultures_set_screensize(void)
{
	if (vultures_opts.fullscreen)
		vultures_set_fullscreen();
	else
		vultures_set_windowed();
}



int vultures_handle_global_event(SDL_Event * event)
{
	static int quitting = 0;
	static int need_kludge = 0;
	static point actual_winsize = {0,0};
	
	/* blarg. see the SDL_VIDEORESIZE case */
	if (need_kludge &&
		(actual_winsize.x != vultures_opts.width ||
		actual_winsize.y != vultures_opts.height))
	{
		vultures_screen = SDL_SetVideoMode(vultures_opts.width, vultures_opts.height, 32,
										SDL_SWSURFACE | SDL_RESIZABLE | SDL_ASYNCBLIT);
		vultures_win_resize(vultures_opts.width, vultures_opts.height);
		actual_winsize.x = vultures_opts.width;
		actual_winsize.y = vultures_opts.height;
		need_kludge = 0;
	}

	switch (event->type)
	{
		case SDL_ACTIVEEVENT:
			if (event->active.gain && event->active.state == SDL_APPACTIVE)
				vultures_refresh();
			else if (event->active.state == SDL_APPMOUSEFOCUS)
				have_mouse_focus = event->active.gain;
			break;

		case SDL_VIDEOEXPOSE:
			vultures_refresh();
			break;

		case SDL_QUIT:
			if (quitting)
				break;

			/* prevent recursive quit dialogs... */
			quitting++;

			/* exit gracefully */
			if (program_state.gameover)
			{
				/* assume the user really meant this, as the game is already over... */
				/* to make sure we still save bones, just set stop printing flag */
				program_state.stopprint++;
				/* and send keyboard input as if user pressed ESC */
				vultures_eventstack_add('\033', -1, -1, V_RESPOND_POSKEY); 
			}
			else if (!program_state.something_worth_saving)
			{
				/* User exited before the game started, e.g. during splash display */
				/* Just get out. */
				vultures_bail(NULL);
			}
			else
			{
				switch (vultures_yn_function("Save and quit?", "yn", 'n'))
				{
					case 'y':
						vultures_eventstack_add('y', -1 , -1, V_RESPOND_CHARACTER);
						dosave();
						break;
					case 'n':
						vultures_eventstack_add('\033', -1, -1, V_RESPOND_CHARACTER);
						break;
					default:
						break;
				}
			}
			quitting--;
			break;

		case SDL_MOUSEMOTION:
			vultures_set_mouse_pos(event->motion.x, event->motion.y);
			break;

		case SDL_KEYDOWN:
			if (event->key.keysym.sym == SDLK_TAB && (event->key.keysym.mod & KMOD_ALT) && vultures_opts.fullscreen)
			{
				/* go out of fullscreen for alt + tab */
				vultures_opts.fullscreen = 0;
				vultures_set_windowed();
				if (!vultures_opts.fullscreen && !fs_message_printed++)
					/* This gets displayed only once */
					pline("You have left fullscreen mode. Press ALT+RETURN to reenter it.");

				event->type = 0; /* we don't want to leave the event loop for this */
				return 1;
			}
			else if(event->key.keysym.sym == SDLK_RETURN && (event->key.keysym.mod & KMOD_ALT))
			{
				/* toggle fullscreen with ctrl + enter */
				if (vultures_opts.fullscreen)
				{
					vultures_opts.fullscreen = 0;
					vultures_set_windowed();
				}
				else
				{
					vultures_opts.fullscreen = 1;
					vultures_set_fullscreen();
				}
				event->type = 0; /* we don't want to leave the event loop for this */
				return 1;
			}
			else if(event->key.keysym.sym == SDLK_SYSREQ ||
					event->key.keysym.sym == SDLK_PRINT ||
					event->key.keysym.sym == SDLK_F12)
			{
				vultures_save_screenshot();
				event->type = 0; /* we don't want to leave the event loop for this */
				return 1;
			}
			/* magic tileconfig reload key */
			else if (event->key.keysym.sym == SDLK_F11 &&
					(event->key.keysym.mod & KMOD_ALT) &&
					(event->key.keysym.mod & KMOD_SHIFT))
			{
				vultures_unload_gametiles();
				vultures_load_gametiles();
				levwin->force_redraw();
				ROOTWIN->draw_windows();
				pline("tileconfig reloaded!");
			}

			/* object highlight */
			if (vultures_map_highlight_objects == 0 &&
				((event->key.keysym.mod & KMOD_RCTRL) ||
				event->key.keysym.sym == SDLK_RCTRL))
			{
				/* enable highlighting */
				vultures_map_highlight_objects = 1;

				/* set the max clipping rect */
				levwin->force_redraw();

				/* redraw with the object highlight */
				ROOTWIN->draw_windows();

				/* the mouse got painted over, restore it */
				vultures_mouse_draw();

				/* bring the buffer onto the screen */
				vultures_refresh_window_region();

				/* make sure no after-images of the mouse are left over */
				vultures_mouse_restore_bg();
			}

			break;

		case SDL_KEYUP:
			if (vultures_map_highlight_objects == 1)
			{
				/* disable highlighting */
				vultures_map_highlight_objects = 0;

				/* set the max clipping rect */
				levwin->force_redraw();

				/* redraw without the object highlight */
				ROOTWIN->draw_windows();

				/* the mouse got painted over, restore it */
				vultures_mouse_draw();

				/* bring the buffer onto the screen */
				vultures_refresh_window_region();

				/* make sure no after-images of the mouse are left over */
				vultures_mouse_restore_bg();
			}
			break;

		case SDL_VIDEORESIZE:
			actual_winsize.x = event->resize.w;
			actual_winsize.y = event->resize.h;

			vultures_opts.width = event->resize.w;
			if (event->resize.w < 540)
			{
				need_kludge = 1;
				vultures_opts.width = 540;
			}
			vultures_opts.height = event->resize.h;
			if (event->resize.h < 510)
			{
				need_kludge = 1;
				vultures_opts.height = 510;
			}

			/* SDL will not actually change the size of the window here, only the size of the buffer.
			* therefore we may need_kludge to call SDL_SetVideoMode AGAIN to set the window size. */
			vultures_screen = SDL_SetVideoMode(vultures_opts.width, vultures_opts.height, 32,
										SDL_SWSURFACE | SDL_RESIZABLE | SDL_ASYNCBLIT);
			vultures_win_resize(vultures_opts.width, vultures_opts.height);
			break;
	}

	return 0;
}



static void vultures_sdl_error(char *file, int line, const char *what)
{
	vultures_write_log(V_LOG_ERROR, file, line, "%s: %s\n", what, SDL_GetError());
}


void vultures_enter_graphics_mode()
{
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) == -1)
	{
		vultures_sdl_error(__FILE__, __LINE__, "Could not initialize SDL");
		exit(1);
	}

	/* Initialize the event handlers */
	atexit(SDL_Quit);
	/* Filter key, mouse and quit events */
	/*  SDL_SetEventFilter(FilterEvents); */
	/* Enable key repeat */
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);  
	SDL_WM_SetCaption(VERSION_ID,NULL);

	if (vultures_opts.fullscreen)
		vultures_screen = SDL_SetVideoMode(vultures_opts.width, vultures_opts.height, 32,
										SDL_SWSURFACE | SDL_FULLSCREEN | SDL_ASYNCBLIT);
	else
		vultures_screen = SDL_SetVideoMode(vultures_opts.width, vultures_opts.height, 32,
										SDL_SWSURFACE | SDL_RESIZABLE | SDL_ASYNCBLIT);

	/* no screen: maybe the configured video mode didn't work */
	if (!vultures_screen) {
		vultures_sdl_error(__FILE__, __LINE__, "Failed to set configured video mode, trying to fall back to 800x600 windowed");
		vultures_screen = SDL_SetVideoMode(800, 600, 32, SDL_SWSURFACE | SDL_RESIZABLE | SDL_ASYNCBLIT);
		vultures_opts.fullscreen = 0;
	}

	/* still no screen: nothing more to be done */
	if (!vultures_screen)
	{
		vultures_sdl_error(__FILE__, __LINE__, "Could not initialize video mode");
		exit(1);
	}

	/* Don't show double cursor */
	SDL_ShowCursor(SDL_DISABLE);

	/* Enable Unicode translation. Necessary to match keypresses to characters */
	SDL_EnableUNICODE(1);

	if (vultures_opts.play_effects || vultures_opts.play_music)
		vultures_init_sound();
}



void vultures_exit_graphics_mode(void)
{
	SDL_ShowCursor(SDL_ENABLE);

	vultures_stop_music();
	if (vultures_cdrom) SDL_CDClose(vultures_cdrom);
	vultures_cdrom = NULL;
	SDL_Quit();
}



void vultures_refresh_region
(
	int x1, int y1, 
	int x2, int y2 
)
{
	SDL_Rect rect;

	/* resizing support makes this check necessary: we may try to refresh
	* a region that used to be inside the window, but isn't anymore */
	x1 = (x1 >= vultures_screen->w) ? vultures_screen->w - 1 : x1;
	y1 = (y1 >= vultures_screen->h) ? vultures_screen->h - 1 : y1;
	
	/* Clip edges (yes, really, otherwise we crash...) */
	rect.x = (x1 < 0) ? 0 : x1;
	rect.y = (y1 < 0) ? 0 : y1;
	rect.w = ((x2 >= vultures_screen->w) ? vultures_screen->w - 1 : x2) - rect.x + 1;
	rect.h = ((y2 >= vultures_screen->h) ? vultures_screen->h - 1 : y2) - rect.y + 1;

	SDL_UpdateRects(vultures_screen, 1, &rect);
}



void vultures_refresh(void)
{
	SDL_Flip( vultures_screen );
}

