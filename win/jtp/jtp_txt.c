/*	SCCS Id: @(#)jtp_txt.c	1.0	2000/9/10	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_txt_c_
#define _jtp_txt_c_

/*---------------------------------------------------------------------------
 Text displaying
---------------------------------------------------------------------------*/

#include <stdio.h>
#include <time.h>
#include "jtp_gen.h"
#include "jtp_gra.h"
#include "jtp_gfl.h"
#include "jtp_txt.h"

jtp_font * jtp_fonts = NULL;

int txtw_xbegin = 0;
int txtw_ybegin = 0;
int txtw_xend = 639;
int txtw_yend = 479;


/*---------------------------------------------------------------------------
 PCX font loader
---------------------------------------------------------------------------*/
void jtp_load_font
(
  char *nimi,   /* Filename of the PCX image of the font */
  int paikka    /* Font index in the font array */
)
{
  int         i,j,otax,otax2,otay,otay2,verty,verty2;
  int         fpicx,fpicy;
  unsigned char   line1[]  = "abcdefghijklmnopqrstuvwxyz\0";
  unsigned char   line2[]  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\0";
  unsigned char   line3[]  = "0123456789.,:;\'\"?!()[]/\\#%+-=„Ž”™ _*$\0";
  unsigned char * lines[3] = {line1,line2,line3};
  unsigned char * fontbuffer;
  int tempheight1 = 0, tempheight2 = 0;

  for (i = 0; i < 256; i++)
  {
    jtp_fonts[paikka].fontpics[i].kuva = NULL;
    jtp_fonts[paikka].fontpics[i].blmod = 0;
  }
 
  jtp_load_PCX_buf(0,0,nimi,&fontbuffer,0);
  fpicy = fontbuffer[0]*256+fontbuffer[1];
  fpicx = fontbuffer[2]*256+fontbuffer[3];
 
  verty2 = -1;
  for (j = 0; j < 3; j++)
  { 
    verty = verty2+1;
    while (fontbuffer[verty*fpicx+4] == 254) verty++;
  
    verty2 = verty+1;
    while (fontbuffer[verty2*fpicx+4] == 254) verty2++;
  
    otax = 1; otay = verty;
    if (verty2-verty+1 > tempheight1) tempheight1 = verty2-verty+1;
     
    for (i = 0; i < strlen((char *)lines[j]); i++)
    {
      while (fontbuffer[otay*fpicx+otax+4]==254) otay++;
      otay2=otay;
    
      while (fontbuffer[otay2*fpicx+otax+fpicx+4]!=254) otay2++;
      jtp_fonts[paikka].fontpics[(lines[j][i])].blmod=otay2-verty2;
      if (otay2 - verty2 > tempheight2) tempheight2 = otay2 - verty2;
    
      otax2=otax;
      while (fontbuffer[otay*fpicx+otax2+1+4]!=254) otax2++;
 
      jtp_fonts[paikka].fontpics[(lines[j][i])].kuva=jtp_get_img_src(otax,otay,otax2,otay2,fontbuffer);
               
      otax=otax2+2;
      otay=verty;
    }
  }
  free(fontbuffer);
 
  jtp_fonts[paikka].spacing=2;
 /* 
  jtp_fonts[paikka].lineheight=jtp_fonts[paikka].fontpics['g'].kuva[1];
  jtp_fonts[paikka].lineheight+=jtp_fonts[paikka].fontpics['h'].kuva[1];
  jtp_fonts[paikka].lineheight-=jtp_fonts[paikka].fontpics['c'].kuva[1];
  jtp_fonts[paikka].baseline=jtp_fonts[paikka].fontpics['h'].kuva[1];
 */
 /* printf("Lineheight %d, baseline %d\n", tempheight1+tempheight2,tempheight1-1); getch(); */
  jtp_fonts[paikka].lineheight=tempheight1 + tempheight2;
  jtp_fonts[paikka].baseline=tempheight1-1;
} 




/*---------------------------------------------------------------------------
 Text plotting into an image pointer
---------------------------------------------------------------------------*/
int jtp_put_char
(
  int x, int y,
  unsigned char textcol,
  unsigned char *a,
  unsigned char *destin
)
{
  int Xsize,Ysize,i,j,k,l,yalku,yloppu,xalku,xloppu;
  int dplus;
  unsigned char vari;
 
  if ((a==NULL) || (x>jtp_screen.drx2)) return(0);
  Ysize = a[1]; /* Assumes that character height is < 256 pixels */
  Xsize = a[3]; /* Assumes that character width is < 256 pixels */
  y-=Ysize-1;
 
  if (x<=jtp_screen.drx1-Xsize) return(0);
  if ((y<=jtp_screen.dry1-Ysize) || (y>jtp_screen.dry2)) return(Xsize);
 
  if (y+Ysize-1>jtp_screen.dry2) yloppu=jtp_screen.dry2-y; else yloppu=Ysize-1;
  if (y<jtp_screen.dry1) {yalku=jtp_screen.dry1-y;} else yalku=0;
 
  if (x+Xsize-1>jtp_screen.drx2) xloppu=jtp_screen.drx2-x; else xloppu=Xsize-1;
  if (x<jtp_screen.drx1) {xalku=jtp_screen.drx1-x;} else xalku=0;
 
  a += 4+yalku*Xsize;
  if (destin != jtp_screen.vpage)
  {
    dplus = destin[3]+256*destin[2];
    destin += 4;
  }
  else dplus = jtp_screen.width;
    
  destin += (y+yalku)*dplus+x;
 
  for (j = yalku; j <= yloppu; j++)
  {
    for (i = xalku; i <= xloppu; i++)
    {
      vari = a[i];
      if (vari == 15) destin[i] = textcol;
      else if (vari > 0) destin[i] = vari;
    }
    a += Xsize;
    destin += dplus;
  }
  return(Xsize);
}


void jtp_put_text
(
  int ax, int ay,
  int fontti, unsigned char textcol,
  char *jono,
  unsigned char *destin
)
{
  int chx = ax,pituus = strlen(jono),i;
  unsigned char curcol = textcol;
  char kirjain;
  
  for (i = 0; i < pituus; i++)
  {      
    kirjain = jono[i];
    switch (kirjain)
    {
      case '\n':
        ay += jtp_fonts[fontti].lineheight;
        chx=ax;
        break;
      default:
        if (jtp_fonts[fontti].fontpics[(unsigned char)kirjain].kuva != NULL)
          chx+=jtp_put_char(chx,ay+jtp_fonts[fontti].fontpics[(unsigned char)kirjain].blmod,
               curcol,jtp_fonts[fontti].fontpics[(unsigned char)kirjain].kuva,destin) +
               jtp_fonts[fontti].spacing;
        if (chx>jtp_screen.drx2)
        {
          ay += jtp_fonts[fontti].lineheight;
          chx = ax;
        }
        break;
    }   
  }
}

/*---------------------------------------------------------------------------
 Text characteristics
---------------------------------------------------------------------------*/

int jtp_text_length
(
  char *jono,
  int fontti
)
{
  int text_ln, old_ln;
  int str_ln, i;
  char kirjain;
 
  if (!jono) return(0);
 
  text_ln = 0;
  old_ln = 0;
  str_ln = strlen(jono);
  for (i = 0; i < str_ln; i++)
  {
    kirjain = jono[i];
    switch(kirjain)
    {
      case '\n':
        if (text_ln > old_ln)
          old_ln = text_ln-jtp_fonts[fontti].spacing;
        text_ln = 0;
        break;
      default:
        if (jtp_fonts[fontti].fontpics[(unsigned char)kirjain].kuva != NULL)
        {
          /* Assume that character width is < 256 pixels */
          text_ln += jtp_fonts[fontti].fontpics[(unsigned char)kirjain].kuva[3];
          text_ln += jtp_fonts[fontti].spacing;
        }
        break;
    }
  }
  if (text_ln > jtp_fonts[fontti].spacing)
    text_ln -= jtp_fonts[fontti].spacing;
  if (text_ln > old_ln) return(text_ln);
  return(old_ln);
}


int jtp_text_height
(
  char *jono,
  int fontti
)
{
  int ay=0,pituus,i;
  char kirjain;

  if (!jono) return(0);

// printf(jono); printf("\n\n"); getch();
  pituus = strlen(jono);
  for (i = 0; i < pituus; i++)
  {
    kirjain=jono[i];
    switch (kirjain)
    {
      case '\n': ay+=jtp_fonts[fontti].lineheight; break;//fflush(stdout); getch(); printf(" newline \n"); getch(); break;
      default:   break;//printf("%c",kirjain); break;  
    }
  }  
  ay += jtp_fonts[fontti].lineheight;
// printf("Total height is %d\n",ay); getch();
  return(ay);
}


void jtp_set_text_window(int xalku,int yalku,int xloppu,int yloppu)
{
  jtp_screen.drx1 = xalku;
  jtp_screen.dry1 = yalku;
  jtp_screen.drx2 = xloppu;
  jtp_screen.dry2 = yloppu;
}


void jtp_free_fonts(int n_of_fonts)
{
  int i, j;

  if (jtp_fonts == NULL) return;
  for (j = 0; j < n_of_fonts; j++)
  {
    for (i = 0; i < 256; i++)
      if (jtp_fonts[j].fontpics[i].kuva != NULL)
      {
        free(jtp_fonts[j].fontpics[i].kuva);
        jtp_fonts[j].fontpics[i].kuva = NULL;
      }
  }
  free(jtp_fonts);
  jtp_fonts = NULL;
}

#endif
