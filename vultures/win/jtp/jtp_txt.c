/*	SCCS Id: @(#)jtp_txt.c	1.0	2000/9/10	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

/*---------------------------------------------------------------------------
 Text displaying
---------------------------------------------------------------------------*/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "jtp_gen.h"
#include "jtp_gra.h"
#include "jtp_gfl.h"
#include "jtp_txt.h"
#include "jtp_win.h"

jtp_font * jtp_fonts = NULL;

int txtw_xbegin = 0;
int txtw_ybegin = 0;
int txtw_xend = 639;
int txtw_yend = 479;

#define JTP_FONT_BACKGROUND 254
#define JTP_GLYPH_BACKGROUND  0
#define JTP_GLYPH_FOREGROUND 15

/*---------------------------------------------------------------------------
 PCX font loader
---------------------------------------------------------------------------*/
int jtp_load_font
(
  const char *nimi,   /* Filename of the PCX image of the font */
  int paikka    /* Font index in the font array */
)
{
  int         i,j,otax,otax2,otay,otay2,verty,verty2;
  int         fpicx,fpicy;
  static unsigned char const line1[]  = "abcdefghijklmnopqrstuvwxyz";
  static unsigned char const line2[]  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ@";
  static unsigned char const line3[]  = "0123456789.,:;\'\"?!()[]/\\#%+-= _*$^&<>{}|";
  static const unsigned char *const lines[3] = { line1, line2, line3 };
  unsigned char * fontbuffer;
  int tempheight1 = 0, tempheight2 = 0;

  for (i = 0; i < 256; i++)
  {
    jtp_fonts[paikka].fontpics[i].kuva = NULL;
    jtp_fonts[paikka].fontpics[i].blmod = 0;
  }
 
  if ((fontbuffer = jtp_load_graphic(NULL, nimi, 0)) == NULL)
  {
    return 0;
  }
  jtp_get_dimensions(fontbuffer, &fpicx, &fpicy);
  if (fpicx <= 2 || fpicy <= 2)
  {
    jtp_write_log_message(JTP_LOG_NOTE, __FILE__, __LINE__, "font graphic too small in %s: %dx%d\n", nimi, fpicx, fpicy);
    return 0;
  }
  verty2 = -1;
  for (j = 0; j < 3; j++)
  {
    verty = verty2+1;
    while (verty < fpicy && fontbuffer[verty * fpicx + 4] == JTP_FONT_BACKGROUND)
      verty++;
  
    verty2 = verty+1;
    while (verty2 < fpicy && fontbuffer[verty2 * fpicx + 4] == JTP_FONT_BACKGROUND)
      verty2++;
    if (verty2 >= fpicy)
    {
      jtp_write_log_message(JTP_LOG_NOTE, __FILE__, __LINE__, "no baseline marker found in %s\n", nimi);
      return 0;
    }
    otax = 1;
    if (verty2 - verty + 1 > tempheight1)
      tempheight1 = verty2 - verty + 1;

    for (i = 0; lines[j][i] != '\0'; i++)
    {
      otay = verty;
      while (otay < fpicy && otax < fpicx && fontbuffer[otay * fpicx + otax + 4] == JTP_FONT_BACKGROUND)
        otay++;
      if (otay >= fpicy)
        otay = fpicy - 1;
      otay2 = otay;

      while ((otay2 + 1) < fpicy && otax < fpicx && fontbuffer[(otay2 + 1) * fpicx + otax + 4] != JTP_FONT_BACKGROUND)
        otay2++;
      if ((otay2 + 1) >= fpicy)
        otay2 = fpicy - 1;
      jtp_fonts[paikka].fontpics[lines[j][i]].blmod = otay2 - verty2;
      if (otay2 - verty2 > tempheight2)
        tempheight2 = otay2 - verty2;

      otax2 = otax;
      while ((otax2 + 1) < fpicx && fontbuffer[otay * fpicx + otax2 + 1 + 4] != JTP_FONT_BACKGROUND)
        otax2++;

      if (otax2 >= fpicx)
        otax2 = fpicx - 1;
      jtp_fonts[paikka].fontpics[lines[j][i]].kuva = jtp_get_img_src(otax, otay, otax2, otay2, fontbuffer);
#ifdef SANITY_CHECKS
      if (jtp_fonts[paikka].fontpics[lines[j][i]].kuva == NULL)
      {
        fprintf(stderr, "glyph for character %c not found in %s\n",
            lines[j][i], nimi);
      } else
      {
        int x, y, w, h;
        unsigned char *image;
        
        image = jtp_fonts[paikka].fontpics[lines[j][i]].kuva;
        jtp_get_dimensions(image, &w, &h);
        for (y = 0; y < h; y++)
          for (x = 0; x < w; x++)
            if (image[y * w + x + 4] != JTP_GLYPH_BACKGROUND &&
              image[y * w + x + 4] != JTP_GLYPH_FOREGROUND)
            {
              fprintf(stderr, "pixel %d,%d for glyph %c in font %s is neither foreground nor background\n",
                x, y, lines[j][i], nimi);
            }
      }
#endif  /* SANITY_CHECKS */
      /* printf("%d %c %3d.%3d - %3d.%3d %3d %3d\n", paikka, lines[j][i], otax, otay, otax2, otay2, tempheight1, tempheight2); */
      otax = otax2 + 2;
    }
  }
  free(fontbuffer);

  jtp_fonts[paikka].spacing = 2;
 /* 
  jtp_fonts[paikka].lineheight=jtp_fonts[paikka].fontpics['g'].kuva[1];
  jtp_fonts[paikka].lineheight+=jtp_fonts[paikka].fontpics['h'].kuva[1];
  jtp_fonts[paikka].lineheight-=jtp_fonts[paikka].fontpics['c'].kuva[1];
  jtp_fonts[paikka].baseline=jtp_fonts[paikka].fontpics['h'].kuva[1];
 */
  jtp_fonts[paikka].lineheight = tempheight1 + tempheight2;
  jtp_fonts[paikka].baseline = tempheight1 - 1;
  /* printf("Lineheight %d, baseline %d\n", jtp_fonts[paikka].lineheight, jtp_fonts[paikka].baseline); */
  return 1;
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
  int Xsize,Ysize,i,j,yalku,yloppu,xalku,xloppu;
  int dplus;
  unsigned char vari;
 
  if ((a==NULL) || (x>jtp_screen.drx2)) return(0);
  Ysize = a[1]; /* Assumes that character height is < 256 pixels */
  Xsize = a[3]; /* Assumes that character width is < 256 pixels */
  y -= Ysize-1;
 
  if (x<=jtp_screen.drx1-Xsize) return(0);
  if ((y<=jtp_screen.dry1-Ysize) || (y>jtp_screen.dry2)) return(Xsize);
 
  if (y+Ysize-1>jtp_screen.dry2) yloppu=jtp_screen.dry2-y; else yloppu=Ysize-1;
  if (y<jtp_screen.dry1) {yalku=jtp_screen.dry1-y;} else yalku=0;
 
  if (x+Xsize-1>jtp_screen.drx2) xloppu=jtp_screen.drx2-x; else xloppu=Xsize-1;
  if (x<jtp_screen.drx1) {xalku=jtp_screen.drx1-x;} else xalku=0;
 
  a += 4+yalku*Xsize;
  if (destin != jtp_screen.vpage)
  {
    dplus = jtp_get_img_width(destin);
    destin += 4;
  }
  else dplus = jtp_screen.width;
    
  destin += (y+yalku)*dplus+x;
 
  for (j = yalku; j <= yloppu; j++)
  {
    for (i = xalku; i <= xloppu; i++)
    {
      vari = a[i];
      if (vari == JTP_GLYPH_FOREGROUND)
        destin[i] = textcol;
#if 0
      else if (vari != JTP_GLYPH_BACKGROUND)
        destin[i] = vari;
#endif
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
  const char *jono,
  unsigned char *destin
)
{
  int chx = ax,pituus = strlen(jono),i;
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
               textcol,jtp_fonts[fontti].fontpics[(unsigned char)kirjain].kuva,destin) +
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

int jtp_text_length(const char *str, int font)
{
    int text_ln, max_ln;
    int str_ln, i;
    unsigned char current;
 
    if (!str) return(0);
 
    text_ln = 0;
    max_ln = 0;
    
    str_ln = strlen(str);
    for (i = 0; i < str_ln; i++)
    {
        current = str[i];
        switch(current)
        {
            case '\n':
                if (text_ln > max_ln)
                    max_ln = text_ln-jtp_fonts[font].spacing;
                text_ln = 0;
                break;

            default:
                if (jtp_fonts[font].fontpics[current].kuva != NULL)
                {
                    /* Assume that character width is < 256 pixels */
                    text_ln += jtp_fonts[font].fontpics[current].kuva[3];
                    text_ln += jtp_fonts[font].spacing;
                }
                break;
        }
    }
    if (text_ln > jtp_fonts[font].spacing)
        text_ln -= jtp_fonts[font].spacing;
    
    if (text_ln > max_ln)
        return text_ln;
    return max_ln;
}


int jtp_text_height(const char *str, int font)
{
    int height=0, len, i;

    if (!str) return(0);

    len = strlen(str);
    for (i = 0; i < len; i++)
        if (str[i] == '\n')
            height += jtp_fonts[font].lineheight;

    height += jtp_fonts[font].lineheight;
    return height;
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
