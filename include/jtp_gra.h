/*	SCCS Id: @(#)jtp_gra.h	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_gra_h_
#define _jtp_gra_h_

#include "jtp_def.h"

typedef struct{
  unsigned char * vpage;
  int width;
  int height;
  int drx1,dry1,drx2,dry2;
} jtp_screen_t;

void jtp_init_screen(int, int, int);
void jtp_enter_graphics_mode();
void jtp_exit_graphics_mode();
void jtp_correct_gamma(unsigned char *, unsigned char *, unsigned char *, double);
void jtp_palch(unsigned char, unsigned char, unsigned char, unsigned char);
void jtp_updatepal(unsigned char, unsigned char);
void jtp_blankpal(unsigned char, unsigned char);
void jtp_fade_out(double);
void jtp_fade_in(double);
void jtp_set_draw_region(int, int, int, int);
void jtp_pixelput(int, int, unsigned char);
unsigned char jtp_pixelget(int, int);
void jtp_rect(int, int, int, int, unsigned char);
void jtp_fill_rect(int, int, int, int, unsigned char);
void jtp_bres_line(int, int, int, int, unsigned char);
void jtp_bres_circle(int, int, int, unsigned char);
void jtp_bres_fill_circle(int, int, int, unsigned char);
unsigned char *jtp_get_img(int, int, int, int);
unsigned char *jtp_get_img_src(int, int, int, int, unsigned char *);
void jtp_put_img(int, int, unsigned char *);
void jtp_put_stencil(int, int, unsigned char *);
void jtp_refresh();
void jtp_refresh_region(int, int, int, int);
void jtp_free_screen();


extern unsigned char jtp_colors[][3];
extern jtp_screen_t jtp_screen;
extern double jtp_gamma_correction;

void jtp_GoBackToTextMode();
void jtp_SetGDisplayMode(jtp_uint4);


#endif
