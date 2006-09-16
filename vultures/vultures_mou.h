/*	SCCS Id: @(#)jtp_mou.h	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_mou_h_
#define _jtp_mou_h_

#include "vultures_types.h"

#define JTP_MBUTTON_NONE 0
#define JTP_MBUTTON_LEFT 1
#define JTP_MBUTTON_RIGHT 2


extern short int jtp_mousex, jtp_mousey, jtp_mouseb;
extern short int jtp_oldmx, jtp_oldmy, jtp_oldmb;
extern jtp_tile ** jtp_mcursor;

int jtp_readmouse(void);
void jtp_setmouse(int tempx, int tempy);
char jtp_mouse_area(int xstart, int ystart, int xend, int yend);
char jtp_oldm_area(int xstart, int ystart, int xend, int yend);
void jtp_repeatmouse(jtp_tile *m_cursor, int whenstop);
void jtp_keymouse(jtp_tile *m_cursor, int whenstop);
void jtp_press_button(int xstart, int ystart, int xend, int yend, jtp_tile *m_cursor);


#endif
