/* Copyright (c) Jaakko Peltonen, 2000				  */
/* Copyright (c) Daniel Thaler, 2006				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#if defined(__GNUC__)
#include <unistd.h>
#endif

#include <SDL.h>

#include "png.h"

#include "vultures_gfl.h"
#include "vultures_gen.h"
#include "vultures_win.h"
#include "vultures_sdl.h"

#include "winclass/messagewin.h"

#define PNG_BYTES_TO_CHECK 4


static void vultures_png_read_callback(png_structp png_ptr, png_bytep area, png_size_t size)
{
	char *mem = (char *)png_ptr->io_ptr;
	memcpy(area, mem, size);
	png_ptr->io_ptr = (mem + size);
}

/*--------------------------------------------------------------------------
truecolor png image loader (backend)
takes a buffer containing raw image data and returns an SDL_Surface
srcbuf       : [in]  buffer containing a complete image file
buflen       : [in]  length of the input buffer
--------------------------------------------------------------------------*/
SDL_Surface *vultures_load_surface(char *srcbuf, unsigned int buflen)
{
	png_structp png_ptr;
	png_infop info_ptr;
	Uint32 Rmask, Gmask, Bmask, Amask;
	png_uint_32 width, height;
	int bit_depth, color_type, row;
	png_bytep * row_pointers = NULL;
	SDL_Surface *img, *convert;

	/* vultures_load_surface converts everything to screen format for faster blitting
	* so we can't continue if we don't have a screen yet */
	if (!vultures_screen)
		return NULL;

	/* no NULL pointers, please */
	if (!srcbuf)
		return NULL;

	img = NULL;

	/* memory region must contain a png file */
	if (png_sig_cmp((unsigned char *)srcbuf, 0, PNG_BYTES_TO_CHECK))
		return NULL;

	/* Create the PNG loading context structure */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return NULL;

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto out;

	/* Set up error handling */
	if (setjmp(png_ptr->jmpbuf))
		goto out;

	png_set_read_fn(png_ptr, (char *)srcbuf, vultures_png_read_callback);

	/* Read PNG header info */
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
				&color_type, NULL, NULL, NULL);

	/* reduce 16 bit per channel to 8 bit per channel */
	png_set_strip_16(png_ptr);

	/* expand 1,2,4 bit grayscale to 8 bit grayscale; expand paletted images to rgb,
	* expand tRNS to true alpha channel */
	png_set_expand(png_ptr);

	/* expand grayscale to full rgb */
	png_set_gray_to_rgb(png_ptr);

	/* add an opaque alpha channel to anything that doesn't have one yet */
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	info_ptr->color_type |= PNG_COLOR_MASK_ALPHA;

	/* get the component mask for the surface */
	if ( SDL_BYTEORDER == SDL_LIL_ENDIAN )
	{
		Rmask = 0x000000FF;
		Gmask = 0x0000FF00;
		Bmask = 0x00FF0000;
		Amask = 0xFF000000;
	} else {
		Rmask = 0xFF000000;
		Gmask = 0x00FF0000;
		Bmask = 0x0000FF00;
		Amask = 0x000000FF;
	}

	img = SDL_AllocSurface(SDL_SWSURFACE | SDL_SRCALPHA, width, height,
						32, Rmask, Gmask, Bmask, Amask);
	if (!img)
		goto out;

	/* Create the array of pointers to image data */
	row_pointers = new png_bytep[height];
	if (!row_pointers) {
		SDL_FreeSurface(img);
		img = NULL;
		goto out;
	}

	for (row = 0; row < (int)height; row++)
		row_pointers[row] = (png_bytep)(Uint8 *)img->pixels + row * img->pitch;

	/* read the image */
	png_read_image(png_ptr, row_pointers);

out:
	png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : NULL, NULL);

	if (row_pointers)
		delete row_pointers;

	if (!img)
		return NULL;

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
SDL_Surface *vultures_load_graphic(string name)
{
	SDL_Surface *image;
	int fsize;
	string filename;
	FILE * fp;
	char * srcbuf;

	name += ".png";
	filename = vultures_make_filename(V_GRAPHICS_DIRECTORY, "", name);

	fp = fopen(filename.c_str(), "rb");
	if (!fp)
		return NULL;

	/* obtain file size. */
	fseek(fp , 0 , SEEK_END);
	fsize = ftell(fp);
	rewind(fp);

	srcbuf = new char[fsize];
	if (!srcbuf)
		return 0;

	fread(srcbuf, fsize, 1, fp);
	fclose(fp);

	image = vultures_load_surface(srcbuf, fsize);

	delete srcbuf;

	return image;
}



/*--------------------------------------------
Save the contents of the surface to a png file
--------------------------------------------*/
void vultures_save_png(SDL_Surface *surface, string filename, int with_alpha)
{
	png_structp png_ptr;
	png_infop info_ptr;
	FILE * fp;
	int i, j;
	unsigned int *in_pixels = (unsigned int*)surface->pixels;
	unsigned char *output = new unsigned char[surface->w * surface->h * (with_alpha ? 4 : 3)];
	png_bytep *image = new png_bytep[surface->h];

	/* strip out alpha bytes if neccessary and reorder the image bytes to RGB */
	for (j = 0; j < surface->h; j++)
	{
		image[j] = output;
		for (i = 0; i < surface->w; i++)
		{
			*output++ = (((*in_pixels) & surface->format->Rmask) >> surface->format->Rshift);       /* red   */
			*output++ = (((*in_pixels) & surface->format->Gmask) >> surface->format->Gshift);       /* green */
			*output++ = (((*in_pixels) & surface->format->Bmask) >> surface->format->Bshift);     /* blue  */
			if ( with_alpha )
				*output++ = (((*in_pixels) & surface->format->Amask) >> surface->format->Ashift); /* alpha */
			in_pixels++;
		}
	}

	/* open the file */
	if (!(fp = fopen(filename.c_str(), "wb")))
		goto cleanup;

	/* create png image data structures */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	if (setjmp(png_jmpbuf(png_ptr)))
		goto cleanup;

	/* write the image */
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, surface->w, surface->h, 8, with_alpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
				PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_set_rows(png_ptr, info_ptr, image);
	png_write_png(png_ptr, info_ptr, 0, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);

cleanup: /* <- */
	fclose(fp);
	/* output has been modified, but the value is still present in image[0] */
	delete image[0];
	delete image;
}


/*-------------------------------------
Save the contents of the screen (ie.
back buffer) into a BMP file.
---------------------------------------*/
void vultures_save_screenshot(void)
{
	string filename;
	int i;
	string msg;
	char namebuf[20];

	for (i = 0; i < 1000; i++)
	{
		sprintf(namebuf, "scree%03d.png", i);
		filename = vultures_make_filename(NULL, NULL, namebuf);
		if (access(filename.c_str(), R_OK) != 0)
		{
			vultures_save_png(vultures_screen, filename, 0);
			msg = string("Screenshot saved as ") + namebuf + ".";

			if (vultures_windows_inited)
				vultures_messagebox(msg);
			else
				msgwin->add_message(msg);

			return;
		}
	}
	vultures_messagebox("Too many screenshots already saved.");
}
