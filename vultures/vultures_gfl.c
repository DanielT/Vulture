/*  SCCS Id: @(#)jtp_gfl.c  3.0 2000/9/10 */
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#if defined(__GNUC__)
#include <unistd.h>
#endif

#include <SDL.h>
#include <SDL_image.h>

#include "vultures_gra.h"
#include "vultures_gfl.h"
#include "vultures_gen.h"
#include "vultures_win.h"
#include "vultures_sdl.h"
#include "vultures_init.h"

/*--------------------------------------------------------------------------
 256-color Image  palette loader (just the palette, not the actual image)
 surface     : an SDL_Surface from which the palette is to  be extracted
--------------------------------------------------------------------------*/
void jtp_load_palette_from_surface(SDL_Surface *surface)
{
    int colourCount;

    for (colourCount=0; colourCount<surface->format->palette->ncolors; colourCount++)
    {
      jtp_colors[colourCount][0]=surface->format->palette->colors[colourCount].r>>2;
      jtp_colors[colourCount][1]=surface->format->palette->colors[colourCount].g>>2;
      jtp_colors[colourCount][2]=surface->format->palette->colors[colourCount].b>>2;
      jtp_correct_gamma(&jtp_colors[colourCount][0],
                        &jtp_colors[colourCount][1],
                        &jtp_colors[colourCount][2],
                        jtp_gamma_correction);
    }
}


/*--------------------------------------------------------------------------
 256-color/truecolor tile loader
 srcbuf       : [in]  buffer containing a complete image file
 buflen       : [in]  length of the input buffer
 loadpalette  : [in]  use the image palette to set the screen palette
--------------------------------------------------------------------------*/
char * vultures_load_image(char *srcbuf, unsigned int buflen, char loadpalette)
{
	SDL_RWops *rw;
	SDL_Surface *img;
	unsigned char * destbuf;
	int y, linelen;
	unsigned char * graphic;

	rw = SDL_RWFromMem(srcbuf, buflen);
	img = IMG_Load_RW(rw, 1);
	if (!img)
		return NULL;

	linelen = (img->w * img->format->BytesPerPixel);

	
	graphic = malloc(img->h * linelen + 4);
	if (!graphic)
	{
		SDL_FreeSurface(img);
		return NULL;
	}
	jtp_put_dimensions(graphic, img->w, img->h);

	
    if (loadpalette)
        jtp_load_palette_from_surface(img);

	
	destbuf = graphic + 4;
	for (y = 0; y < img->h; y++)
		memcpy(destbuf + y * linelen, img->pixels + y * img->pitch, linelen);

	SDL_FreeSurface(img);

	return graphic;
}


/*-------------------------------------
 Save the contents of the screen (ie.
 back buffer) into a BMP file.
---------------------------------------*/
void jtp_save_screenshot(void)
{
	char *filename;
	int i;
	char *msg;
	char namebuf[20];
	
	for (i = 0; i < 1000; i++)
	{
		sprintf(namebuf, "scree%03d.bmp", i);
		filename = jtp_make_filename(NULL, NULL, namebuf);
		if (access(filename, R_OK) != 0)
		{
			SDL_SaveBMP(jtp_sdl_screen,filename);
			msg = malloc(sizeof("Screenshot saved as ") + strlen(filename) + 2);
			strcpy(msg, "Screenshot saved as ");
			strcat(msg, namebuf);
			strcat(msg, ".");
			jtp_messagebox(msg);
			free(msg);
			free(filename);
			return;
		}
		free(filename);
	}
	jtp_messagebox("Too many screenshots already saved.");
}
