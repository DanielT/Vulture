/*	SCCS Id: @(#)jtp_dos.h	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_dos_h_
#define _jtp_dos_h_

#include "jtp_def.h"
#include "jtp_gra.h"

/* SVGA initialization and closing */
void jtp_DOSGoBackToTextMode();
void jtp_DOSSetMode(int screen_width, int screen_height, int screen_bitdepth);

/* Display updaters */
void jtp_DOSRefresh(jtp_screen_t *);
void jtp_DOSRefreshRegion(int x1,int y1,int x2,int y2, jtp_screen_t *);

/* Palette handling */
void jtp_DOSSetColor
(
  unsigned char palchvari,
  unsigned char palchrv,
  unsigned char palchgv,
  unsigned char palchbv
);

/* Mouse handling */
void jtp_DOSReadMouse(jtp_screen_t *);

/* Keyboard handling */
int jtp_DOSGetch();
int jtp_DOSKbHit();

extern short int jtp_dos_mousex, jtp_dos_mousey, jtp_dos_mouseb;

#endif
