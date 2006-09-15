/*  SCCS Id: @(#)jtp_txt.h  1.0 2000/9/10 */
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_txt_h_
#define _jtp_txt_h_

/*---------------------------------------------------------------------------
 Text displaying
---------------------------------------------------------------------------*/


typedef struct
           {
             unsigned char *kuva;
             short int blmod;
           } jtp_chrtype;

typedef struct
           {
             jtp_chrtype fontpics[256];
             unsigned char spacing;
             char baseline;
             char lineheight;
           } jtp_font;

extern jtp_font * jtp_fonts;

int jtp_load_font(const char *nimi,int paikka);
int  jtp_put_char(int x,int y,unsigned char textcol,unsigned char *a,unsigned char *destin);

void jtp_put_text(int ax,int ay,int fontti,unsigned char textcol,const char *jono,unsigned char *destin);
int  jtp_text_length(const char *str,int font);
int  jtp_text_height(const char *str,int font);
void jtp_set_text_window(int xalku,int yalku,int xloppu,int yloppu);
void jtp_free_fonts(int n_of_fonts);

#endif
