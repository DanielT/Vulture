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

#include "png.h"

#include "vultures_gfl.h"
#include "vultures_gen.h"
#include "vultures_win.h"
#include "vultures_sdl.h"


/*--------------------------------------------------------------------------
truecolor image loader (backend)
takes a buffer containing raw image data and returns an SDL_Surface
srcbuf       : [in]  buffer containing a complete image file
buflen       : [in]  length of the input buffer
--------------------------------------------------------------------------*/
SDL_Surface *vultures_load_surface(char *srcbuf, unsigned int buflen)
{
    SDL_RWops *rw;
    SDL_Surface *img, *convert;

    /* vultures_load_surface converts everything to screen format for faster blitting
    * so we can't continue if we don't have a screen yet */
    if (!vultures_screen)
    return NULL;

    rw = SDL_RWFromMem(srcbuf, buflen);
    img = IMG_Load_RW(rw, 1);

    if (!img)
    return NULL;

    if (img->format->BitsPerPixel == 8)
        SDL_SetColorKey(img, SDL_SRCCOLORKEY, 0);

    convert = SDL_DisplayFormatAlpha(img);

    SDL_FreeSurface(img);
    img = convert;

    return img;
}



/*-------------------------------------
truecolor image loader (front end)
loads the given file into a buffer and calls vultures_load_surface
subdir:  [in] subdirectory in which to look
name:    [in] filename of the image
---------------------------------------*/
SDL_Surface *vultures_load_graphic(const char *subdir, const char *name)
{
    SDL_Surface *image;
    char namebuf[128];
    int fsize;
    char * filename;
    FILE * fp;
    char * srcbuf;

    strcat(strcpy(namebuf, name), ".png");
    filename = vultures_make_filename(V_GRAPHICS_DIRECTORY, subdir, namebuf);
    if (filename == NULL)
        OOM(1);

    fp = fopen(filename, "rb");
    free(filename);
    if (!fp)
    	return NULL;

    /* obtain file size. */
    fseek(fp , 0 , SEEK_END);
    fsize = ftell(fp);
    rewind(fp);

    srcbuf = malloc(fsize);
    if (!srcbuf)
        return 0;

    fread(srcbuf, fsize, 1, fp);
    fclose(fp);

    image = vultures_load_surface(srcbuf, fsize);

    free(srcbuf);

    return image;
}


/*--------------------------------------------
Save the contents of the surface to a png file
--------------------------------------------*/
void vultures_save_png(SDL_Surface * surface, char* filename)
{
    png_structp png_ptr;
    png_infop info_ptr;
    FILE * fp;
    unsigned int i, j, *in_pixels = surface->pixels;
    unsigned char *output = malloc(surface->w * surface->h * 3);
    png_byte ** image = malloc(surface->h * sizeof(png_byte*));

    /* strip out alpha bytes and reorder the image bytes to RGB */
    for (j = 0; j < surface->h; j++)
    {
        image[j] = output;
        for (i = 0; i < surface->w; i++)
        {
            *output++ = (((*in_pixels) & surface->format->Rmask) >> surface->format->Rshift);  /* red */
            *output++ = (((*in_pixels) & surface->format->Gmask) >> surface->format->Gshift);  /* green */
            *output++ = (((*in_pixels++) & surface->format->Bmask) >> surface->format->Bshift);/* blue */
        }
    }

    /* open the file */
    if (!(fp = fopen(filename, "wb")))
        goto cleanup;

    /* create png image data structures */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);
    if (setjmp(png_jmpbuf(png_ptr)))
        goto cleanup;

    /* write the image */
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, surface->w, surface->h, 8, PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_set_rows(png_ptr, info_ptr, image);
    png_write_png(png_ptr, info_ptr, 0, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);

cleanup: /* <- */
    fclose(fp);
    free(image[0]);
    free(image);
}


/*-------------------------------------
Save the contents of the screen (ie.
back buffer) into a BMP file.
---------------------------------------*/
void vultures_save_screenshot(void)
{
    char *filename;
    int i;
    char *msg;
    char namebuf[20];

    for (i = 0; i < 1000; i++)
    {
        sprintf(namebuf, "scree%03d.png", i);
        filename = vultures_make_filename(NULL, NULL, namebuf);
        if (access(filename, R_OK) != 0)
        {
            vultures_save_png(vultures_screen,filename);
            msg = malloc(256);
            snprintf(msg, 256, "Screenshot saved as %s.", namebuf);

            if (vultures_windows_inited)
                vultures_messagebox(msg);
            else
                vultures_messages_add(msg);

            free(msg);
            free(filename);
            return;
        }
        free(filename);
    }
    vultures_messagebox("Too many screenshots already saved.");
}
