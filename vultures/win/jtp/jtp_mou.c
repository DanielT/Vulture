/*	SCCS Id: @(#)jtp_mou.c	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>
#include "jtp_sdl.h"
#include "jtp_gra.h"
#include "jtp_mou.h"
#include "jtp_gen.h"
#include "jtp_gfl.h"
#include "jtp_tile.h"
#include "jtp_win.h"

short int jtp_mousex = 100, jtp_mousey = 100, jtp_mouseb = 0;
short int jtp_oldmx = 100, jtp_oldmy = 100, jtp_oldmb = 0;

jtp_mouse_cursor * jtp_mcursor[JTP_MAX_MCURSOR];

int jtp_readmouse(void)
{
  jtp_oldmx = jtp_mousex; jtp_oldmy = jtp_mousey; jtp_oldmb = jtp_mouseb;
  jtp_SDLProcessEvents();
  jtp_mousex = jtp_sdl_mousex; 
  jtp_mousey = jtp_sdl_mousey; 
  jtp_mouseb = jtp_sdl_mouseb;
  return jtp_mouseb;
}


void jtp_setmouse(int tempx,int tempy)
{
  jtp_mousex=tempx;
  jtp_mousey=tempy;
}


char jtp_mouse_area(int x1,int y1,int x2,int y2)
{
  if ((jtp_mousex>=x1)&&(jtp_mousex<=x2)&&(jtp_mousey>=y1)&&(jtp_mousey<=y2))
    return(1); 
  return(0);  
}

char jtp_oldm_area(int x1,int y1,int x2,int y2)
{
  if ((jtp_oldmx>=x1)&&(jtp_oldmx<=x2)&&(jtp_oldmy>=y1)&&(jtp_oldmy<=y2))
   return(1); 
  return(0);  
}


void jtp_repeatmouse
(
  jtp_mouse_cursor *m_cursor,
  int whenstop
)
{
  unsigned char * m_bg;
 
  m_bg = jtp_get_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                     jtp_mousex + m_cursor->xmod + m_cursor->graphic[3], 
                     jtp_mousey + m_cursor->ymod + m_cursor->graphic[1]);
  jtp_put_stencil(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_cursor->graphic);
  jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                     jtp_mousex + m_cursor->xmod + m_cursor->graphic[3],
                     jtp_mousey + m_cursor->ymod + m_cursor->graphic[1], &jtp_screen);
  do
  {
    jtp_readmouse();

    if ((jtp_oldmx!=jtp_mousex) || (jtp_oldmy!=jtp_mousey))
    {
       jtp_put_img(jtp_oldmx + m_cursor->xmod, jtp_oldmy + m_cursor->ymod, m_bg);
       free(m_bg);
       
       m_bg = jtp_get_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                          jtp_mousex + m_cursor->xmod + m_cursor->graphic[3],
                          jtp_mousey + m_cursor->ymod + m_cursor->graphic[1]);
       jtp_put_stencil(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_cursor->graphic);

       jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                          jtp_mousex + m_cursor->xmod + m_cursor->graphic[3],
                          jtp_mousey + m_cursor->ymod + m_cursor->graphic[1], &jtp_screen);
       jtp_refresh_region(jtp_oldmx + m_cursor->xmod, jtp_oldmy + m_cursor->ymod,
                          jtp_oldmx + m_cursor->xmod + m_cursor->graphic[3],
                          jtp_oldmy + m_cursor->ymod + m_cursor->graphic[1], &jtp_screen);
    }
  }
  while (jtp_mouseb != whenstop);

  jtp_put_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_bg);
  /* jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                     jtp_mousex + m_cursor->xmod + m_cursor->graphic[3],
                     jtp_mousey + m_cursor->ymod + m_cursor->graphic[1], &jtp_screen); */
  free(m_bg);
}


void jtp_keymouse
(
  jtp_mouse_cursor *m_cursor,
  int whenstop
)
{
  unsigned char * m_bg;
 
  m_bg = jtp_get_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                     jtp_mousex + m_cursor->xmod + m_cursor->graphic[3], 
                     jtp_mousey + m_cursor->ymod + m_cursor->graphic[1]);
  jtp_put_stencil(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_cursor->graphic);
  jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                     jtp_mousex + m_cursor->xmod + m_cursor->graphic[3],
                     jtp_mousey + m_cursor->ymod + m_cursor->graphic[1], &jtp_screen);
  do
  {
    jtp_readmouse();

    if ((jtp_oldmx!=jtp_mousex) || (jtp_oldmy!=jtp_mousey))
    {
       jtp_put_img(jtp_oldmx + m_cursor->xmod, jtp_oldmy + m_cursor->ymod, m_bg);
       free(m_bg);
       
       m_bg = jtp_get_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                          jtp_mousex + m_cursor->xmod + m_cursor->graphic[3],
                          jtp_mousey + m_cursor->ymod + m_cursor->graphic[1]);
       jtp_put_stencil(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_cursor->graphic);

       jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                          jtp_mousex + m_cursor->xmod + m_cursor->graphic[3],
                          jtp_mousey + m_cursor->ymod + m_cursor->graphic[1], &jtp_screen);
       jtp_refresh_region(jtp_oldmx + m_cursor->xmod, jtp_oldmy + m_cursor->ymod,
                          jtp_oldmx + m_cursor->xmod + m_cursor->graphic[3],
                          jtp_oldmy + m_cursor->ymod + m_cursor->graphic[1], &jtp_screen);
    }
    else
      jtp_msleep(5);
  }
  while ((jtp_mouseb != whenstop) && (!jtp_kbhit()));

  jtp_put_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_bg);
  /* jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                     jtp_mousex + m_cursor->xmod + m_cursor->graphic[3],
                     jtp_mousey + m_cursor->ymod + m_cursor->graphic[1], &jtp_screen); */
  free(m_bg);
}


void jtp_press_button
(
  int x1, int y1, int x2, int y2, 
  jtp_mouse_cursor * m_cursor
)
{
 unsigned char *button_bg = jtp_get_img(x1, y1, x2, y2);
 unsigned char *temp_button = jtp_get_img(x1, y1, x2-2,y2-2);

 jtp_fill_rect(x1, y1, x2, y1 + 1, 0);
 jtp_fill_rect(x1, y1, x1 + 1, y2, 0);      
 jtp_put_img(x1 + 2, y1 + 2, temp_button);
 free(temp_button);
 jtp_refresh_region(x1, y1, x2, y2, &jtp_screen);
 jtp_repeatmouse(m_cursor, JTP_MBUTTON_NONE);
 jtp_put_img(x1, y1, button_bg);
 jtp_refresh_region(x1, y1, x2, y2, &jtp_screen);
 free(button_bg); 
}


jtp_mouse_cursor *jtp_get_mcursor(unsigned char *image, int x1, int y1, int x2, int y2)
{
  jtp_mouse_cursor *temp1 = (jtp_mouse_cursor *)malloc(sizeof(jtp_mouse_cursor));
  int i, j, tx1 = 0, ty1 = 0, tx2 = -1, ty2 = -1;
  unsigned char *vpage;
  int xsize, ysize;

  if (temp1 == NULL)
    OOM(1);
  jtp_get_dimensions(image, &xsize, &ysize);
  if (xsize < 2 || ysize < 2)
  {
    free(image);
    return NULL;
  }
  /*
   * FIXME: same as jtp_get_tile
   */
  vpage = image + 4;
  for (i = y1; i <= y2; i++)
    for (j = x1; j <= x2; j++)
      if (vpage[i * xsize + j] != JTP_COLOR_BACKGROUND)
      {
        ty1 = i;
        i = y2 + 1;
        j = x2 + 1;
      }
 
  for (i = y2; i >= y1; i--)
    for (j = x1; j <= x2; j++)
      if (vpage[i * xsize + j] != JTP_COLOR_BACKGROUND)
      {
        ty2 = i;
        i = y1 - 1;
        j = x2 + 1;
      }
  for (j = x1; j <= x2; j++)
    for (i = ty1; i <= ty2; i++)
      if (vpage[i * xsize + j] != JTP_COLOR_BACKGROUND)
      {
        tx1 = j;
        i = ty2 + 1;
        j = x2 + 1;
      }
  for (j = x2; j >= x1; j--)
    for (i = ty1; i <= ty2; i++)
      if (vpage[i * xsize + j] != JTP_COLOR_BACKGROUND)
      {
        tx2 = j;
        i = ty2 + 1;
        j = x1 - 1;
      }
 
  temp1->xmod = 9999;
  for (i = x1; i <= x2; i++)
    if (vpage[(y1 - 1) * xsize + i] == JTP_COLOR_HOTSPOT)
      temp1->xmod = tx1 - i;

  temp1->ymod = 9999;
  for (i = y1; i <= y2; i++)
    if (vpage[i * xsize + x1 - 1] == JTP_COLOR_HOTSPOT)
      temp1->ymod = ty1 - i;

#ifdef SANITY_CHECKS
  if (temp1->xmod == 9999 || temp1->ymod == 9999)
  {
    fprintf(stderr, "no hotspot found in cursor file\n");
  }
  for (i = x1 - 1; i <= x2 + 1; i++)
  {
    if ((vpage[(y1 - 1) * xsize + i] != JTP_COLOR_HOTSPOT || i == (x1 - 1) || i == (x2 + 1)) &&
      vpage[(y1 - 1) * xsize + i] != JTP_COLOR_BORDER)
      fprintf(stderr, "warning: pixel at %d,%d in cursor file is neither border nor hotspot, wrong dimensions?\n",
        i, y1 - 1);
    if ((vpage[(y2 + 1) * xsize + i] != JTP_COLOR_HOTSPOT || i == (x1 - 1) || i == (x2 + 1)) &&
      vpage[(y2 + 1) * xsize + i] != JTP_COLOR_BORDER)
      fprintf(stderr, "warning: pixel at %d,%d in cursor file is neither border nor hotspot, wrong dimensions?\n",
        i, y2 + 1);
  }

  for (i = y1 - 1; i <= y2 + 1; i++)
  {
    if ((vpage[i * xsize + x1 - 1] != JTP_COLOR_HOTSPOT || i == (y1 - 1) || i == (y2 + 1)) &&
      vpage[i * xsize + x1 - 1] != JTP_COLOR_BORDER)
      fprintf(stderr, "warning: pixel at %d,%d in cursor file is neither border nor hotspot, wrong dimensions?\n",
        x1 - 1, i);
    if ((vpage[i * xsize + x2 + 1] != JTP_COLOR_HOTSPOT || i == (y1 - 1) || i == (y2 + 1)) &&
      vpage[i * xsize + x2 + 1] != JTP_COLOR_BORDER)
      fprintf(stderr, "warning: pixel at %d,%d in cursor file is neither border nor hotspot, wrong dimensions?\n",
        x2 + 1, i);
  }
#endif

  if (temp1->xmod == 9999 || temp1->ymod == 9999)
  {
    jtp_write_log_message(JTP_LOG_NOTE, __FILE__, __LINE__, "no hotspot found in cursor file\n");
  }

  if (temp1->xmod == 9999)
    temp1->xmod = 0;
  if (temp1->ymod == 9999)
    temp1->ymod = 0;
  temp1->graphic = jtp_get_img_src(tx1, ty1, tx2, ty2, image);
  return temp1;
}
