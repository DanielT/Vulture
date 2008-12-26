/* Copyright (c) Jaakko Peltonen, 2001				  */
/* NetHack may be freely redistributed.  See license for details. */

/*-------------------------------------------------------------------
 vultures_sdl.h : SDL API calls for Vulture's windowing system.
 Requires SDL 1.2 or newer.
-------------------------------------------------------------------*/

#ifndef _vultures_sdl_h_
#define _vultures_sdl_h_

#include <SDL.h>

#define SDL_TIMEREVENT SDL_USEREVENT
#define SDL_MOUSEMOVEOUT (SDL_USEREVENT+1)

/* low-level event handling */
extern void vultures_wait_event(SDL_Event *event, int wait_timeout);
extern int vultures_poll_event(SDL_Event *event);
extern void vultures_wait_input(SDL_Event *event, int wait_timeout);
extern void vultures_wait_key(SDL_Event *event);

/* Graphics initialization and closing */
extern void vultures_enter_graphics_mode(void);
extern void vultures_exit_graphics_mode(void);

/* Display updaters */
extern void vultures_refresh(void);
extern void vultures_refresh_region(int, int, int, int);

/* Miscellaneous */
extern void vultures_set_screensize(void);


extern SDL_Surface *vultures_screen;

#endif
