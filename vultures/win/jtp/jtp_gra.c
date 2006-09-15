/*	SCCS Id: @(#)jtp_gra.c	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "jtp_sdl.h"
#include "jtp_gen.h"
#include "jtp_gra.h"
#include "jtp_gfl.h"
#include "jtp_tile.h"

unsigned char jtp_colors[256][3];
unsigned char jtp_game_colors[256][3];
jtp_screen_t jtp_screen;
double jtp_gamma_correction = 1.0;

/*-----------------------------------------
 Graphics initialization
------------------------------------------*/
void jtp_init_screen
(
  int screen_width, 
  int screen_height
)
{
  jtp_screen.width = screen_width;
  jtp_screen.height = screen_height;
  jtp_screen.vpage = (unsigned char *)calloc(jtp_screen.width*jtp_screen.height + 4,sizeof(unsigned char));
  if (jtp_screen.vpage == NULL)
    OOM(1);
  jtp_put_dimensions(jtp_screen.vpage, jtp_screen.width, jtp_screen.height);
  jtp_screen.vpage += 4;
  jtp_screen.drx1 = 0;
  jtp_screen.dry1 = 0;
  jtp_screen.drx2 = jtp_screen.width-1;
  jtp_screen.dry2 = jtp_screen.height-1;
}


void jtp_free_screen(void)
{
  free(jtp_screen.vpage - 4);
}


/*--------------------------------------------------------------------------
 Palette handling
--------------------------------------------------------------------------*/

void jtp_correct_gamma
(
  unsigned char *r,  /* Pointers to the color components to be corrected */
  unsigned char *g, 
  unsigned char *b, 
  double alpha       /* Gamma value, alpha > 0. Values below 1 darken. */
)
{
  double l;  /* Original color luminosity */
  double m;  /* Gamma correction multiplier */

  /* 
   * A simple "L1" or "city-block" norm is used here
   * to calculate luminosity. Other norms could also
   * be used, for example the "L2" norm, or some 
   * weighted version of these. Also the relative
   * importance of the red/green/blue components
   * could be weighted.
   */
  l = ((double)*r + (double)*g + (double)*b) / (3.0*63.0);
  if (l == 0) return;

  /* Gamma correct luminosity value */
  if (alpha <= 0) return;
  if (alpha > 0) alpha = 1.0/alpha;

  m = 1/((1-alpha*alpha)*l+alpha*alpha);

  /* Correct the red, green and blue values */
  l = *r * m;
  if (l > 63)
    l = 63;
  *r = (unsigned char) l;
  l = *g * m;
  if (l > 63)
    l = 63;
  *g = (unsigned char) l;
  l = *b * m;
  if (l > 63)
    l = 63;
  *b = (unsigned char) l;

  return;
}

void jtp_updatepal
(
  unsigned char index1, 
  unsigned char index2
)
{
  int i;

  for (i = index1; i <= index2; i++)
    jtp_SDLRecordColor(i, jtp_colors[i][0], jtp_colors[i][1], jtp_colors[i][2]);
  jtp_SDLSetPalette();   
  jtp_SDLProcessEvents();
}

void jtp_blankpal
(
  unsigned char index1, 
  unsigned char index2
)
{
  int i;

  for (i = index1; i <= index2; i++)
    jtp_SDLRecordColor(i, 0, 0, 0);
  jtp_SDLSetPalette();
  jtp_SDLProcessEvents();
}

void jtp_fade_out(double n_secs)
{
  double start_clock, cur_clock, old_clock; 
  int i;
 
  start_clock = jtp_clocktick();
  cur_clock = start_clock;
  old_clock = cur_clock;
  
  while (cur_clock < n_secs + start_clock)
  {
    if (cur_clock != old_clock)
    {
      for (i = 0; i < 256; i++)
        jtp_SDLRecordColor(i, (jtp_colors[i][0]*(n_secs+start_clock-cur_clock))/n_secs,
                             (jtp_colors[i][1]*(n_secs+start_clock-cur_clock))/n_secs,
                             (jtp_colors[i][2]*(n_secs+start_clock-cur_clock))/n_secs);
      jtp_SDLSetPalette();
      jtp_SDLProcessEvents();
    }
    old_clock = cur_clock;
    jtp_msleep(10);
    cur_clock = jtp_clocktick();
  }
  jtp_blankpal(0,255);
}

void jtp_fade_in(double n_secs)
{
  double start_clock, cur_clock, old_clock;
  int i;

  start_clock = jtp_clocktick();
  cur_clock = start_clock;
  old_clock = cur_clock;
 
  while (cur_clock < n_secs + start_clock)
  {
    if (cur_clock != old_clock)
    {
      for (i = 0; i < 256; i++)
        jtp_SDLRecordColor(i, (jtp_colors[i][0]*(cur_clock-start_clock))/n_secs,
                             (jtp_colors[i][1]*(cur_clock-start_clock))/n_secs,
                             (jtp_colors[i][2]*(cur_clock-start_clock))/n_secs);
      jtp_SDLSetPalette();
      jtp_SDLProcessEvents();
    }
    old_clock = cur_clock;
    jtp_msleep(10);
    cur_clock = jtp_clocktick();
  }
  jtp_updatepal(0,255);
}


/*--------------------------------------------------------------------------
 Basic graphics plotting
--------------------------------------------------------------------------*/
void jtp_set_draw_region
(
  int _drx1, int _dry1,
  int _drx2, int _dry2
)
{
  if (_drx1 >= 0) jtp_screen.drx1=_drx1; else jtp_screen.drx1 = 0;
  if (_dry1 >= 0) jtp_screen.dry1=_dry1; else jtp_screen.dry1 = 0;
  if (_drx2 < jtp_screen.width) jtp_screen.drx2=_drx2; else jtp_screen.drx2 = jtp_screen.width-1;
  if (_dry2 < jtp_screen.height) jtp_screen.dry2=_dry2; else jtp_screen.dry2 = jtp_screen.height-1;
}


void jtp_pixelput
(
  int x, int y,
  unsigned char vari
)
{
  jtp_screen.vpage[y*jtp_screen.width+x]=vari;
}

void jtp_rect
(
  int x1, int y1,
  int x2, int y2,
  unsigned char cindex
)
{
  int i,j; 
  for (i = x1; i <= x2; i++)
  {
    jtp_screen.vpage[y1*jtp_screen.width+i] = cindex;
    jtp_screen.vpage[y2*jtp_screen.width+i] = cindex;
  }
  for (j = y1; j <= y2; j++)
  {
    jtp_screen.vpage[j*jtp_screen.width+x1] = cindex;
    jtp_screen.vpage[j*jtp_screen.width+x2] = cindex;
  }
}

void jtp_fill_rect
(
  int x1, int y1,
  int x2, int y2,
  unsigned char cindex
)
{
 int i,j;
 
 if (x1 < jtp_screen.drx1) x1 = jtp_screen.drx1;
 if (y1 < jtp_screen.dry1) y1 = jtp_screen.dry1; 
 if (x2 > jtp_screen.drx2) x2 = jtp_screen.drx2;
 if (y2 > jtp_screen.dry2) y2 = jtp_screen.dry2;
 
 for (i = x1; i <= x2; i++)
   for (j = y1; j <= y2; j++)
     jtp_screen.vpage[j*jtp_screen.width+i] = cindex;
}


void jtp_bres_line
(
  int x1, int y1, 
  int x2, int y2, 
  unsigned char cindex
)
{
  int dx, dy, x, y, tmpplus, endx, endy, p, const1, const2;
 
  dx = abs(x1-x2); dy = abs(y1-y2);
 
  if (dx > dy)
  {
    p = 2*dy-dx; const1 = 2*dy; const2 = 2*(dy-dx);

    if (x1 > x2)
    {
      x = x2;
      y = y2;
      endx = x1;
      if (y1 > y2)
        tmpplus = 1;
      else
        tmpplus = -1;
    }
    else
    {
      x = x1;
      y = y1;
      endx = x2;
      if (y2 > y1)
        tmpplus = 1;
      else
        tmpplus = -1;
    }
    jtp_screen.vpage[jtp_screen.width*y+x] = cindex;

    while (x < endx)
    {
      if (p < 0)
        p += const1;
      else
      {
        y += tmpplus;
        p += const2;
      }
      x++;
      jtp_screen.vpage[jtp_screen.width * y + x] = cindex;
    }
  }
  else
  {
    p = 2*dx-dy; const1 = 2*dx; const2 = 2*(dx-dy);

    if (y1 > y2)
    {
      y = y2;
      x = x2;
      endy = y1;
      if (x1 > x2)
        tmpplus = 1;
      else
        tmpplus = -1;
    }
    else
    {
      y = y1;
      x = x1;
      endy = y2;
      if (x2 > x1)
        tmpplus = 1;
      else
        tmpplus = -1;
    }
    jtp_screen.vpage[jtp_screen.width*y+x] = cindex;

    while (y < endy)
    {
      if (p < 0)
        p += const1;
      else
      {
        x += tmpplus;
        p += const2;
      }
      y++;
      jtp_screen.vpage[jtp_screen.width * y + x] = cindex;
    }
  }
}


/*--------------------------------------------------------------------------
 Bitmap handling
--------------------------------------------------------------------------*/
unsigned char *jtp_get_img
(
  int x1, int y1,
  int x2, int y2
)
{
  int Xsize,Ysize,i;
  int Xbegin, Ybegin;
  unsigned char *a;

  if (x1>jtp_screen.width-1)  return(NULL);
  if (y1>jtp_screen.height-1) return(NULL);
 
  if (x2<0) return(NULL);
  if (y2<0) return(NULL);
 
  if (x1<0) Xbegin = -x1; else Xbegin = 0;
  if (y1<0) Ybegin = -y1; else Ybegin = 0;
  if (x2>jtp_screen.width-1) x2=jtp_screen.width-1;
  if (y2>jtp_screen.height-1) y2=jtp_screen.height-1;

  Ysize=y2-y1+1;
  Xsize=x2-x1+1;
  a=(unsigned char *)malloc((Ysize*Xsize+4)*sizeof(unsigned char));
  if (a==NULL) return(NULL);
 
  jtp_put_dimensions(a, Xsize, Ysize);
  for (i=Ybegin;i<=Ysize-1;i++)
    memcpy(&(a[i*Xsize+4+Xbegin]),&(jtp_screen.vpage[(y1+i)*jtp_screen.width+x1+Xbegin]),Xsize-Xbegin);
  return(a);
}

unsigned char *jtp_get_img_src
(
  int x1, int y1,
  int x2, int y2,
  unsigned char *img_source
)
{
  int Xsize,Ysize,i;
  int imgXsize,imgYsize;
  unsigned char *a;

  if (!img_source) return(NULL);
  jtp_get_dimensions(img_source, &imgXsize, &imgYsize);

  if (x1>imgXsize-1) return(NULL);
  if (y1>imgYsize-1) return(NULL);

  if (x2<0) return(NULL);
  if (y2<0) return(NULL);

  if (x1<0) x1=0;
  if (y1<0) y1=0;
  if (x2>imgXsize-1) x2=imgXsize-1;
  if (y2>imgYsize-1) y2=imgYsize-1;

  Ysize=y2-y1+1;
  Xsize=x2-x1+1;
  a=(unsigned char *)malloc((Ysize*Xsize+4)*sizeof(unsigned char));
  if (a==NULL) return(NULL);
 
  jtp_put_dimensions(a, Xsize, Ysize);

  img_source+=y1*imgXsize + x1 + 4;
  for (i=0;i<=Ysize-1;i++)
  {
    memcpy(&(a[i*Xsize+4]),img_source,Xsize);
    img_source+=imgXsize;
  }
  return(a);
}



void jtp_put_img
(
  int x, int y,
  unsigned char *a
)
{
  int srcXsize,cutXsize,srcYsize,i,y1,y2,x1;
  unsigned char *destin;

  if ((a==NULL) || (x>jtp_screen.drx2) || (y>jtp_screen.dry2)) return;

  jtp_get_dimensions(a, &srcXsize, &srcYsize);

  if ((x+srcXsize<=jtp_screen.drx1) || (y+srcYsize<=jtp_screen.dry1)) return;
  if (y<jtp_screen.dry1) y1=jtp_screen.dry1-y; else y1=0;
  if (y+srcYsize-1>jtp_screen.dry2) y2=jtp_screen.dry2-y; else y2=srcYsize-1;
  if (x+srcXsize-1>jtp_screen.drx2) cutXsize=jtp_screen.drx2-x+1; else cutXsize=srcXsize;
  if (x<jtp_screen.drx1)
  {
    x1=jtp_screen.drx1-x; 
    cutXsize-=x1;
  }
  else x1=0;

  a += x1 + 4 + y1*srcXsize;
  destin = jtp_screen.vpage+(y+y1)*jtp_screen.width+x+x1;

  for (i = y1; i <= y2; i++)
  {
    memcpy(destin,a,cutXsize);
    a+=srcXsize;
    destin+=jtp_screen.width;
  }
}


void jtp_put_stencil
(
  int x, int y,
  unsigned char *a
)
{
  int srcXsize,srcYsize,j,y1,y2,x1,x2;
  int dplus;
  unsigned char *destin, *a_end;

  if ((a==NULL) || (x>jtp_screen.drx2) || (y>jtp_screen.dry2)) return;

  jtp_get_dimensions(a, &srcXsize, &srcYsize);

  if ((x+srcXsize<=jtp_screen.drx1) || (y+srcYsize<=jtp_screen.dry1)) return;
  if (y<jtp_screen.dry1) y1=jtp_screen.dry1-y; else y1=0;
  if (y+srcYsize-1>jtp_screen.dry2) y2=jtp_screen.dry2-y; else y2=srcYsize-1;
  if (x<jtp_screen.drx1) x1=jtp_screen.drx1-x; else x1=0; 
  if (x+srcXsize-1>jtp_screen.drx2) x2=jtp_screen.drx2-x; else x2=srcXsize-1;

  a+=y1*srcXsize+4;
  a_end = a + (y2-y1+1)*srcXsize;
  destin=jtp_screen.vpage+(y1+y)*jtp_screen.width+x;
  dplus = jtp_screen.width;

  a += x1;
  a_end += x1;
  destin += x1;
  x2 -= x1;
  x1 = jtp_screen.width;

  while (a < a_end)
  {
     for (j = x2; j >= 0; j--)
     {
       if (a[j] != JTP_COLOR_BACKGROUND)
        destin[j] = a[j];
     }
     a += srcXsize;  
     destin += dplus;
  }
}
