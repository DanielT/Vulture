/*	SCCS Id: @(#)jtp_mou.c	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>
#include "vultures_sdl.h"
#include "vultures_mou.h"
#include "vultures_gen.h"
#include "vultures_gfl.h"

short int jtp_mousex = 100, jtp_mousey = 100, jtp_mouseb = 0;
short int jtp_oldmx = 100, jtp_oldmy = 100, jtp_oldmb = 0;

jtp_tile ** jtp_mcursor;

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
  jtp_tile *m_cursor,
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
  jtp_tile *m_cursor,
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
  jtp_tile * m_cursor
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

