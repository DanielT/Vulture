/*	SCCS Id: @(#)jtp_mou.h	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_mou_h_
#define _jtp_mou_h_


#define JTP_MBUTTON_NONE 0
#define JTP_MBUTTON_LEFT 1
#define JTP_MBUTTON_RIGHT 2

/*
 * Mouse cursor shapes. Note: there's an implicit assumption
 * that mouse cursors will be less than 256 pixels wide/tall. 
 * Similar assumptions are made for many other small graphics.
 */
#define JTP_CURSOR_NORMAL 0
#define JTP_CURSOR_SCROLLLEFT 1
#define JTP_CURSOR_SCROLLRIGHT 2
#define JTP_CURSOR_SCROLLUP 3
#define JTP_CURSOR_SCROLLDOWN 4
#define JTP_CURSOR_TARGET_GREEN 5
#define JTP_CURSOR_TARGET_RED 6
#define JTP_CURSOR_TARGET_INVALID 7
#define JTP_CURSOR_TARGET_HELP 8
#define JTP_CURSOR_HOURGLASS 9
#define JTP_CURSOR_OPENDOOR 10
#define JTP_CURSOR_STAIRS 11
#define JTP_CURSOR_GOBLET 12
#define JTP_MAX_MCURSOR 13

typedef struct {
  unsigned char * graphic;
  int xmod, ymod;
} jtp_mouse_cursor;

typedef struct {
  int x1, x2, y1, y2;
  jtp_mouse_cursor * mcursor;
  unsigned char * tooltip;
  int accelerator;
} jtp_hotspot;

extern short int jtp_mousex, jtp_mousey, jtp_mouseb;
extern short int jtp_oldmx, jtp_oldmy, jtp_oldmb;
extern jtp_mouse_cursor *jtp_mcursor[];

int jtp_readmouse(void);
void jtp_setmouse(int tempx, int tempy);
char jtp_mouse_area(int xstart, int ystart, int xend, int yend);
char jtp_oldm_area(int xstart, int ystart, int xend, int yend);
void jtp_repeatmouse(jtp_mouse_cursor *m_cursor, int whenstop);
void jtp_keymouse(jtp_mouse_cursor *m_cursor, int whenstop);
void jtp_press_button(int xstart, int ystart, int xend, int yend, jtp_mouse_cursor *m_cursor);
jtp_mouse_cursor * jtp_get_mcursor(unsigned char *image, int x1, int y1, int x2, int y2);


#endif
