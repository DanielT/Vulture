/*	SCCS Id: @(#)jtp_mou.c	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_mou_c_
#define _jtp_mou_c_

#include "jtp_def.h"
#ifdef USE_DIRECTX_SYSCALLS
#include "jtp_dirx.h"
#endif
#ifdef USE_DOS_SYSCALLS
#include "jtp_dos.h"
#endif
#ifdef USE_SDL_SYSCALLS
#include "jtp_sdl.h"
#endif
#include "jtp_gra.h"
#include "jtp_mou.h"

short int jtp_mousex = 100, jtp_mousey = 100, jtp_mouseb = 0;
short int jtp_oldmx = 100, jtp_oldmy = 100, jtp_oldmb = 0;

jtp_mouse_cursor * jtp_mcursor[JTP_MAX_MCURSOR];

void jtp_readmouse()
{
  jtp_oldmx = jtp_mousex; jtp_oldmy = jtp_mousey; jtp_oldmb = jtp_mouseb;
#ifdef USE_DIRECTX_SYSCALLS
  jtp_DXReadMouse();
  jtp_mousex = jtp_dx_mousex; 
  jtp_mousey = jtp_dx_mousey; 
  jtp_mouseb = jtp_dx_mouseb;
#endif
#ifdef USE_DOS_SYSCALLS
  jtp_DOSReadMouse(&jtp_screen);
  jtp_mousex = jtp_dos_mousex; 
  jtp_mousey = jtp_dos_mousey; 
  jtp_mouseb = jtp_dos_mouseb;
#endif
#ifdef USE_SDL_SYSCALLS
  jtp_SDLReadMouse();
  jtp_mousex = jtp_sdl_mousex; 
  jtp_mousey = jtp_sdl_mousey; 
  jtp_mouseb = jtp_sdl_mouseb;
#endif
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
                     jtp_mousey + m_cursor->ymod + m_cursor->graphic[1]);
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
                          jtp_mousey + m_cursor->ymod + m_cursor->graphic[1]);
       jtp_refresh_region(jtp_oldmx + m_cursor->xmod, jtp_oldmy + m_cursor->ymod,
                          jtp_oldmx + m_cursor->xmod + m_cursor->graphic[3],
                          jtp_oldmy + m_cursor->ymod + m_cursor->graphic[1]);
    }
  }
  while (jtp_mouseb != whenstop);

  jtp_put_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_bg);
  /* jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                     jtp_mousex + m_cursor->xmod + m_cursor->graphic[3],
                     jtp_mousey + m_cursor->ymod + m_cursor->graphic[1]); */
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
                     jtp_mousey + m_cursor->ymod + m_cursor->graphic[1]);
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
                          jtp_mousey + m_cursor->ymod + m_cursor->graphic[1]);
       jtp_refresh_region(jtp_oldmx + m_cursor->xmod, jtp_oldmy + m_cursor->ymod,
                          jtp_oldmx + m_cursor->xmod + m_cursor->graphic[3],
                          jtp_oldmy + m_cursor->ymod + m_cursor->graphic[1]);
    }
  }
  while ((jtp_mouseb != whenstop) && (!jtp_kbhit()));

  jtp_put_img(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod, m_bg);
  /* jtp_refresh_region(jtp_mousex + m_cursor->xmod, jtp_mousey + m_cursor->ymod,
                     jtp_mousex + m_cursor->xmod + m_cursor->graphic[3],
                     jtp_mousey + m_cursor->ymod + m_cursor->graphic[1]); */
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
 jtp_refresh_region(x1, y1, x2, y2);
 jtp_repeatmouse(m_cursor, JTP_MBUTTON_NONE);
 jtp_put_img(x1, y1, button_bg);
 jtp_refresh_region(x1, y1, x2, y2);
 free(button_bg); 
}


jtp_mouse_cursor *
jtp_get_mcursor(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
  jtp_mouse_cursor *temp1 = (jtp_mouse_cursor *)malloc(sizeof(jtp_mouse_cursor));
  int i, j, tx1, ty1, tx2, ty2;
  unsigned char * tempg;

  for (i = y1; i <= y2; i++)
    for (j = x1; j <= x2; j++)
      if (jtp_screen.vpage[i*jtp_screen.width+j])
      {
        ty1 = i; i = y2+1; j = x2+1;
      }
 // printf("Check1\n");   
 
  for (i = y2; i >= y1; i--)
    for (j = x1; j <= x2; j++)
      if (jtp_screen.vpage[i*jtp_screen.width+j])
      {
        ty2 = i; i = y1-1; j = x2+1;
      }
 // printf("Check2\n");
  for (j = x1; j <= x2; j++)
    for (i = ty1; i <= ty2; i++)   
      if (jtp_screen.vpage[i*jtp_screen.width+j])
      {
        tx1 = j; i = ty2+1; j = x2+1;
      }
 // printf("Check3\n");
  for (j = x2; j >= x1; j--)
    for (i = ty1; i <= ty2; i++)   
      if (jtp_screen.vpage[i*jtp_screen.width+j])
      {
        tx2 = j; i = ty2+1; j = x1-1;
      }
 // printf("Check4\n"); 
 
  temp1->xmod = 0;
  for (i = x1; i <= x2; i++)
    if (jtp_screen.vpage[(y1-1)*jtp_screen.width+i] == 16)
      temp1->xmod = tx1 - i; 

  temp1->ymod = 0;
  for (i = y1; i <= y2; i++)
    if (jtp_screen.vpage[i*jtp_screen.width+x1-1] == 16)
      temp1->ymod = ty1 - i;
 
  temp1->graphic = jtp_get_img(tx1, ty1, tx2, ty2);
  return(temp1);
}



#endif
