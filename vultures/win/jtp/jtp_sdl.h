/*	SCCS Id: @(#)jtp_sdl.h	3.0	2001/01/18	*/
/* Copyright (c) Jaakko Peltonen, 2001				  */
/* NetHack may be freely redistributed.  See license for details. */

/*-------------------------------------------------------------------
 jtp_sdl.h : SDL API calls for Vulture's windowing system.
 Requires SDL 1.2 or newer.
-------------------------------------------------------------------*/

#ifndef _jtp_sdl_h_
#define _jtp_sdl_h_

#include "jtp_gra.h"

#ifdef __cplusplus
extern "C"
{
#endif
/* Graphics initialization and closing */
extern void jtp_enter_graphics_mode(jtp_screen_t *);
extern void jtp_exit_graphics_mode(void);

/* Palette handling */
extern void jtp_SDLRecordColor(int, int, int, int);
extern void jtp_SDLSetPalette(void);

/* Display updaters */
extern void jtp_refresh(jtp_screen_t *);
extern void jtp_refresh_region(int, int, int, int, jtp_screen_t *);

/* Keyboard handling */
extern int jtp_getch(void);
extern int jtp_kbhit(void);
extern void jtp_get_event(void);

/* public keybuf manipulation  */
extern void jtp_sdl_keybuf_add(int key);
extern void jtp_sdl_keybuf_reset();

/* Sound playing */
extern void jtp_play_cd_track(char * cdtrackname);
extern void jtp_play_song(char * midifilename);
extern void jtp_stop_music(void);
extern int jtp_is_music_playing(void);
extern void jtp_play_sound(char * wavefilename);

/* Miscellaneous */
extern void jtp_SDLProcessEvents(void);
extern void jtp_msleep(unsigned long milliseconds);
extern double jtp_clocktick(void);

#ifdef __cplusplus
}
#endif

extern int jtp_sdl_mousex, jtp_sdl_mousey, jtp_sdl_mouseb;

#endif
