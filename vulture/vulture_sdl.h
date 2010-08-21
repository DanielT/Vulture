/* NetHack may be freely redistributed.  See license for details. */

/*-------------------------------------------------------------------
 vulture_sdl.h : SDL API calls for Vulture's windowing system.
 Requires SDL 1.2 or newer.
-------------------------------------------------------------------*/

#ifndef _vulture_sdl_h_
#define _vulture_sdl_h_

#include <SDL.h>

#define SDL_TIMEREVENT SDL_USEREVENT
#define SDL_MOUSEMOVEOUT (SDL_USEREVENT+1)

/* low-level event handling */
extern void vulture_wait_event(SDL_Event *event, int wait_timeout);
extern int vulture_poll_event(SDL_Event *event);
extern void vulture_wait_input(SDL_Event *event, int wait_timeout);
extern void vulture_wait_key(SDL_Event *event);

/* Graphics initialization and closing */
extern void vulture_enter_graphics_mode(void);
extern void vulture_exit_graphics_mode(void);

/* Display updaters */
extern void vulture_refresh(void);
extern void vulture_refresh_region(int, int, int, int);

/* Miscellaneous */
extern void vulture_set_screensize(void);


extern SDL_Surface *vulture_screen;

#endif
