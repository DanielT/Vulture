/*	SCCS Id: @(#)jtp_gra.c	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

/*-------------------------------------------------------------------
 jtp_dirx.h : DirextX API calls for Vulture's Eye windowing system.
 Requires DirextX 3.0 or newer.
-------------------------------------------------------------------*/

#ifndef _jtp_dirx_h_
#define _jtp_dirx_h_

#include "jtp_gra.h"

#define JTP_DX_ICON_NETHACK 100

#ifdef __cplusplus
extern "C"
{
#endif
/* Graphics initialization and closing */
extern void jtp_DXEnterGraphicsMode(jtp_screen_t *);
extern void jtp_DXExitGraphicsMode();

/* Palette handling */
extern void jtp_DXRecordColor(int, int, int, int);
extern void jtp_DXSetPalette();

/* Display updaters */
extern void jtp_DXRefresh(jtp_screen_t *);
extern void jtp_DXRefreshRegion(int, int, int, int, jtp_screen_t *);

/* Mouse handling */
extern void jtp_DXReadMouse();

/* Keyboard handling */
extern int jtp_DXGetch();
extern int jtp_DXKbHit();

/* Sound playing */
extern void jtp_DXPlayCDTrack(char *);
extern void jtp_DXPlayMIDISong(char *);
extern void jtp_DXStopMusic();
extern int jtp_DXIsMusicPlaying();
extern void jtp_DXPlayWaveSound(char *, int, int, int);

/* Miscellaneous */
extern void jtp_DXProcessEvents();
#ifdef __cplusplus
}
#endif
/* This function is only called from WinMain, so we hide it otherwise */
#ifdef __cplusplus
extern void jtp_DXSetInstance
(
  HINSTANCE hThisInstance, 
  HINSTANCE hPrevInstance, 
  LPSTR lpszCmdParam, 
  int nCmdShow
);
#endif

extern int jtp_dx_mousex, jtp_dx_mousey, jtp_dx_mouseb;

#endif
