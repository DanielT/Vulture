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
extern void jtp_SDLEnterGraphicsMode(jtp_screen_t *);
extern void jtp_SDLExitGraphicsMode();

/* Palette handling */
extern void jtp_SDLRecordColor(int, int, int, int);
extern void jtp_SDLSetPalette();

/* Display updaters */
extern void jtp_SDLRefresh(jtp_screen_t *);
extern void jtp_SDLRefreshRegion(int, int, int, int, jtp_screen_t *);

/* Mouse handling */
extern void jtp_SDLReadMouse();

/* Keyboard handling */
extern int jtp_SDLGetch();
extern int jtp_SDLKbHit();

/* Sound playing */
extern void jtp_SDLPlayCDTrack(char *);
extern void jtp_SDLPlaySong(char *);
extern void jtp_SDLStopMusic();
extern int jtp_SDLIsMusicPlaying();
extern void jtp_SDLPlaySound(char *);

/* Miscellaneous */
extern void jtp_SDLProcessEvents();
#ifdef __cplusplus
}
#endif

extern int jtp_sdl_mousex, jtp_sdl_mousey, jtp_sdl_mouseb;

#endif
