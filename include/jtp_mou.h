/*	SCCS Id: @(#)jtp_mou.h	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_mou_h_
#define _jtp_mou_h_

#include "jtp_def.h"

#define JTP_MBUTTON_NONE 0
#define JTP_MBUTTON_LEFT 1
#define JTP_MBUTTON_RIGHT 2
#define JTP_MAX_MCURSOR 20

typedef struct {
  unsigned char * graphic;
  int xmod, ymod;
} jtp_mouse_cursor;

typedef struct {
  int x1, x2, y1, y2;
  jtp_mouse_cursor * mcursor;
  unsigned char * tooltip;
  char accelerator;
} jtp_hotspot;

extern short int jtp_mousex, jtp_mousey, jtp_mouseb;
extern short int jtp_oldmx, jtp_oldmy, jtp_oldmb;
extern jtp_mouse_cursor *jtp_mcursor[];

void jtp_readmouse();
void jtp_setmouse(int tempx, int tempy);
char jtp_mouse_area(int xalku, int yalku, int xloppu, int yloppu);
char jtp_oldm_area(int xalku, int yalku, int xloppu, int yloppu);
void jtp_repeatmouse(jtp_mouse_cursor *m_cursor, int whenstop);
void jtp_keymouse(jtp_mouse_cursor *m_cursor, int whenstop);
void jtp_press_button(int xalku, int yalku, int xloppu, int yloppu, jtp_mouse_cursor *m_cursor);
jtp_mouse_cursor * jtp_get_mcursor(int x1, int y1, int x2, int y2);


#endif
