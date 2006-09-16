/* tiletrans.c: tile transformer/compiler
 * Copyright (c) 2005 Daniel Thaler
 * This file may be freely distributed under the Nethack license*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tiletrans.h"

#include "png.h"
#include "SDL_endian.h"

#include "config.h"
#include "color.h"

#include "objclass.h"
#include "obj.h"

#include "monsym.h"
#include "permonst.h"

/* png file signature length */
#define PNG_BYTES_TO_CHECK 4

/* access the control data in tilespec.c */
extern const int num_tiledefs;
extern const int num_objclassmaps;
extern const int num_monclassmaps;
extern const int num_eqmapentries;
extern const struct tiledef tiledefinitions[];
extern const struct classdefmap objclassmap[];
extern const struct classdefmap monclassmap[];
extern const struct eqmap tilemap[];
extern struct objclass objects[];
extern struct objdescr obj_descr[];
extern struct permonst mons[];
extern const char * floor_edges[];
extern const char * wall_styles[];
extern const struct floorstyles fstyles[];
extern const int num_floor_edges;
extern const int num_fstyles;
extern const int num_wall_styles;


/* global vars */
int ** floor_edge_indices = NULL;
int * floor_style_indices = NULL;
int * fs_start = NULL;
int num_ftiles = 0;

struct wallstyle ** walls = NULL;

struct gametile * gametiles = NULL;

int typecount[MAX_TYPES];
int typeoffset[MAX_TYPES];
char ** typenames[MAX_TYPES];
char * type_to_name[MAX_TYPES];

int gametilecount = 0;

/* nethack does not #define the worthless pieces of glass,
 * so we remember what object id's they have and do it ourselves */
int glassgems[20];
int glassgemcount = 0;


/* this palette is our default palette for tiles. It was dumped from jtp_mon0.png and is the palette that truecolor
 * input will get mapped to, if 8-bit output is requested. components are {red, green, blue}*/
const static png_color def_palette[] = { 
{ 91,   0,   0}, {  0,   0, 171}, {  0, 171,   0}, {  0, 171, 171}, {171,   0,   0}, {171,   0, 171}, {171,  87,   0}, {171, 171, 171},
{ 87,  87,  87}, { 87,  87, 255}, { 87, 255,  87}, { 87, 255, 255}, {255,  87,  87}, {255,  87, 255}, {255, 255,  87}, {255, 255, 255},
{255, 163, 255}, {247, 243, 243}, {239, 235, 235}, {231, 227, 227}, {227, 219, 219}, {219, 211, 211}, {211, 203, 203}, {203, 195, 195},
{199, 187, 187}, {191, 179, 179}, {183, 171, 171}, {175, 163, 163}, {171, 155, 155}, {163, 147, 147}, {155, 139, 139}, {147, 135, 135},
{143, 127, 127}, {131, 115, 115}, {123, 107, 107}, {111,  99,  99}, {103,  91,  91}, { 95,  83,  83}, { 83,  75,  75}, { 75,  67,  67},
{ 67,  59,  59}, { 55,  51,  51}, { 47,  43,  43}, { 35,  31,  31}, { 27,  23,  23}, { 19,  15,  15}, {  7,   7,   7}, {  0,   0,   0},
{215,  43, 215}, {235, 235, 247}, {223, 223, 239}, {207, 207, 231}, {191, 191, 223}, {179, 179, 215}, {167, 167, 207}, {151, 151, 199},
{143, 143, 191}, {131, 131, 183}, {119, 119, 175}, {107, 107, 167}, { 99,  99, 159}, { 87,  87, 151}, { 79,  79, 143}, { 71,  71, 135},
{ 63,  63, 127}, { 63,  63, 123}, { 55,  55, 115}, { 51,  51, 107}, { 47,  47,  99}, { 43,  43,  87}, { 39,  39,  79}, { 35,  35,  71},
{ 31,  31,  63}, { 27,  27,  51}, { 19,  19,  43}, { 15,  15,  35}, { 11,  11,  27}, {  7,   7,  15}, {  0,   0,   7}, {135,  39, 151},
{  0,   0,  27}, {255, 255, 211}, {255, 255, 171}, {255, 255, 127}, {255, 255,  87}, {255, 231,  87}, {255, 211,  87}, {255, 187,  87},
{255, 167,  87}, {243, 159,  87}, {227, 151,  91}, {215, 143,  91}, {203, 139,  91}, {191, 131,  91}, {179, 123,  91}, {167, 119,  87},
{155, 111,  87}, {143, 103,  79}, {131,  95,  75}, {123,  87,  67}, {111,  79,  63}, {103,  71,  55}, { 91,  67,  51}, { 83,  59,  47},
{ 71,  51,  39}, { 59,  43,  35}, { 51,  35,  27}, { 39,  27,  23}, { 31,  19,  15}, { 19,  15,  11}, {  0,  64,   0}, {  0,   0,  91},
{207,  75,  75}, {191,  67,  67}, {179,  63,  63}, {163,  59,  59}, {151,  55,  55}, {135,  47,  47}, {123,  43,  43}, {107,  39,  39},
{ 95,  35,  35}, { 83,  27,  27}, { 67,  23,  23}, { 55,  19,  19}, { 39,  11,  11}, { 27,   7,   7}, { 11,   7,   7}, {  0,   0, 107},
{  0,   0,  51}, {231, 247, 239}, {207, 239, 227}, {187, 231, 215}, {167, 223, 203}, {147, 215, 191}, {127, 207, 179}, {111, 203, 167},
{ 95, 195, 155}, { 79, 187, 147}, { 67, 179, 135}, { 51, 171, 127}, { 39, 163, 115}, { 27, 155, 107}, { 15, 147,  99}, {  7, 143,  91},
{  0, 135,  83}, {  0, 123,  75}, {  0, 115,  71}, {  0, 107,  63}, {  0,  99,  59}, {  0,  87,  55}, {  0,  79,  47}, {  0,  71,  43},
{  0,  63,  39}, {  0,  51,  31}, {  0,  43,  27}, {  0,  35,  19}, {  0,  27,  15}, {  0,  15,  11}, {  0,   7,   7}, {  0,   0, 127},
{  0,   0,  71}, {243, 239, 247}, {235, 227, 239}, {223, 211, 227}, {215, 199, 219}, {207, 187, 211}, {195, 179, 203}, {187, 163, 195},
{179, 155, 187}, {171, 143, 179}, {159, 131, 171}, {151, 123, 163}, {143, 111, 155}, {135, 103, 147}, {127,  95, 139}, {119,  87, 131},
{111,  79, 123}, {103,  71, 115}, { 95,  67, 107}, { 87,  63,  95}, { 79,  55,  87}, { 71,  51,  79}, { 67,  47,  71}, { 59,  39,  63},
{ 51,  35,  55}, { 43,  31,  47}, { 35,  23,  39}, { 27,  19,  31}, { 23,  15,  23}, { 11,   7,  15}, {120, 120, 120}, {  0,   0, 151},
{243, 183, 103}, {231, 179, 115}, {223, 175, 127}, {215, 175, 135}, {207, 175, 143}, {199, 171, 151}, {191, 171, 159}, {183, 171, 163},
{147,  83, 175}, {139,  87, 163}, {135,  95, 151}, {127,  95, 143}, {119,  99, 131}, {115,  99, 119}, {107,  99, 111}, {207, 227,  59},
{175, 211, 115}, {143, 195, 107}, {111, 179,  99}, { 91, 163,  91}, { 83, 147,  99}, {255, 235,  75}, {255, 215,  67}, {255, 199,  59},
{255, 179,  51}, {255, 159,  43}, {255, 135,  35}, {255, 111,  27}, {255,  87,  19}, {255,  63,  15}, {255,  35,   7}, {223, 207, 211},
{215, 155, 171}, {207, 147, 167}, {199, 139, 167}, {191, 131, 167}, {183, 127, 167}, {179, 119, 163}, {171, 115, 163}, {163, 107, 163},
{151,  99, 155}, {139,  95, 147}, {127,  91, 143}, {115,  83, 135}, {103,  79, 127}, { 91,  71, 119}, { 79,  67, 111}, { 71,  63, 107},
{243,  43,  27}, {231,  55,  43}, {219,  63,  59}, {251, 239, 231}, {251, 223, 211}, {247, 207, 191}, {247, 195, 171}, {243, 179, 147},
{243, 163, 131}, {243, 151, 111}, {231, 139,  99}, {219, 127,  87}, {211, 115,  79}, {207,  95,  75}, {199,  79,  71}, {255, 255, 255}};




/**********************************************************
 * freeimg: free up rows, then the rowarray of a png image
 **********************************************************/
void freeimg(png_byte *** img, int height)
{
	int i;
	if (*img) {
		for (i = 0; i < height; i++)
			free((*img)[i]);
		free (*img);
		*img = NULL;
	}

}


/***********************************************************************
 * read_png_file: reads a png file, automatically converts it to RGBA
 * while doing so and also saves any palette used by the image
 ***********************************************************************/
int read_png_file(char * filename, png_byte *** image, png_uint_32 * width, png_uint_32 * height, png_color * palette, int * num_pal)
{
	char sigbuf[PNG_BYTES_TO_CHECK];
	FILE * fp = fopen(filename, "rb");
	
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int row;
	int bit_depth, color_type;
	png_colorp tmp_pal;
	
	if (!fp)
	{
		fprintf(stderr, "WARNING: could not open \"%s\" for reading!\n", filename);
		return 1;
	}
	
	fread(&sigbuf, 1, PNG_BYTES_TO_CHECK, fp);
    if (png_sig_cmp(sigbuf, 0, PNG_BYTES_TO_CHECK))
    {
		fprintf(stderr, "WARNING: file \"%s\" is not a png and will not be loaded!\n", filename);
		fclose(fp);
        return 1;
    }

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return 1;
	}

	/* Set error handling */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		/* Free all of the memory associated with the png_ptr and info_ptr */
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		fclose(fp);
		/* If we get here, we had a problem reading the file */
		return 1;
	}

	/* Set up the input control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	/* If we have already read some of the signature */
	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

	/* Read image info before the first IDAT (image data chunk). */
	png_read_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, width, height, &bit_depth, &color_type, NULL, NULL, NULL);
	
	/* read the palette, if present */
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_PLTE))
	{
		png_get_PLTE(png_ptr, info_ptr, &tmp_pal, num_pal);
		/* the palette is saved in info_ptr, so we need to copy it */
		memcpy(palette, tmp_pal, *num_pal * sizeof(png_color));
	}
	else
		*num_pal = 0;

	/* Expand paletted or RGB images */
	png_set_expand(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

	/* update the png_info to reflect the requested transformations so that png_rowbytes will return the correct number */
	png_read_update_info(png_ptr, info_ptr);
	
	*image = malloc(*height * sizeof(png_byte*));
	for (row = 0; row < *height; row++)
		(*image)[row] = malloc(png_get_rowbytes(png_ptr, info_ptr));

	png_read_image(png_ptr, *image);

	/* read rest of file, and get additional chunks in info_ptr */
	png_read_end(png_ptr, info_ptr);

	/* At this point you have read the entire image */

	/* clean up after the read, and free any memory allocated */
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

	/* close the file */
	fclose(fp);
	
	if (*num_pal) { /* in our paletted images we need to translate the "transparent" color to real transparency */
		unsigned int ** pxdata = (unsigned int **)*image;
		unsigned int transcolor = SDL_SwapLE32((0xff << 24) + (palette[0].blue << 16) + (palette[0].green << 8) + palette[0].red);
		int i, j;
		for (i = 0; i < *height; i ++) {
			for (j = 0; j < *width; j++)
			{
				if (pxdata[i][j] == transcolor)
					pxdata[i][j] = 0x00000000; /* fully transparent */
			}
		}
	}

	/* that's it */
	return 0;
}



/********************************************************************************
 * write_png_file: appends png image data to the given file pointer.
 * If num_pal is non-zero, the image is first converted to 8-bit paletted format
 * using either the default palette or the palette originally loaded from the image
 ********************************************************************************/
int write_png_file(FILE *outfile, png_byte ** tile, png_uint_32 width, png_uint_32 height, png_color * palette, int num_pal)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_color * usedpal = NULL;
	png_byte ** outtile;
	double dist, mindist;
	int i, j, k, bestcolor, colortype = PNG_COLOR_TYPE_RGB_ALPHA;
	
	/* if num_pal is non-zero, we want an 8-bit paletted image as output */
	if (num_pal)
	{
		colortype = PNG_COLOR_TYPE_PALETTE;
		
		/* if we were given a palette, use that, otherwise we have a default palette */
		if (palette)
			usedpal = palette;
		else
			usedpal = (png_color *)def_palette; /* shutting the compiler up with a pointless cast ... */
		
		/* map every truecolor value to a palette index */
		outtile = malloc(height * sizeof(png_byte*));
		for (i = 0; i < height; i++)
		{
			outtile[i] = malloc(width * sizeof(png_byte));
			for (j = 0; j < width; j++)
			{
				mindist = 10000.0;
				bestcolor = 0;
				
				if (tile[i][j*4+3]  != 0x00)
					/* we consider every color value to be a coordinate in 3D space and calculate the minimum
					 * distance between it and all palette colors, to find the best palette match for it */
					for (k = 1; k < num_pal; k++)
					{
						dist = sqrt( (tile[i][j*4  ] - usedpal[k].red  ) * (tile[i][j*4  ] - usedpal[k].red  ) +
						             (tile[i][j*4+1] - usedpal[k].green) * (tile[i][j*4+1] - usedpal[k].green) +
						             (tile[i][j*4+2] - usedpal[k].blue ) * (tile[i][j*4+2] - usedpal[k].blue ) );
						if (dist == 0)
						{
							bestcolor = k;
							k = num_pal;
						}
						else if (dist < mindist)
						{
							mindist = dist;
							bestcolor = k;
						}
					}
				
				outtile[i][j] = bestcolor;
			}
		}
	}
	else
		outtile = tile;
		
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		/* If we get here, we had a problem reading the file */
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return 1;
	}
	
	png_init_io(png_ptr, outfile);
	
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, colortype,
	             PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	if (num_pal)
		png_set_PLTE(png_ptr, info_ptr, usedpal, num_pal);
		
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, outtile);
	png_write_end(png_ptr, info_ptr);
	
	
	if (num_pal)
		freeimg(&outtile, height);
	
	return 0;
}


/*********************************************************************************
 * grab_tile: checks the border, finds the hotspot and cuts transparent rows/cols
 * off the tile. It returns an array of rowpointers into the original image rather
 * than copying data around
 *********************************************************************************/
png_byte ** grab_tile(png_byte ** img, png_uint_32 img_w, png_uint_32 img_h, int * tile_x,
                      int * tile_y, int * tile_w, int * tile_h, int * hs_x, int * hs_y)
{
	png_byte ** tile = NULL;
	unsigned int ** pximg = (unsigned int**)img; /* access the image as pixels rather than byte-wise */
	int i, j;
	int hs_y_tmp = *tile_y, hs_x_tmp = *tile_x;
	
	int hscolor = SDL_SwapLE32(0xffffa3ff);
	int bordercolor = SDL_SwapLE32(0xff972787);

	if (pximg[*tile_y-1][*tile_x-1] != bordercolor)
		fprintf(stderr, "topleft: no border pixel found at (%d,%d). Wrong image dimensions? ",
		        *tile_x-1, *tile_y-1);
	if (pximg[*tile_y+*tile_h+1][*tile_x+*tile_w+1] != bordercolor)
		fprintf(stderr, "bottom-right: no border pixel found at (%d,%d). Wrong image dimensions? ",
		        *tile_x+*tile_w+1, *tile_y+*tile_h+1);
	
	/* find left hotspot relative to containing image */
	for (i = *tile_y; i <= (*tile_y + *tile_h); i++)
		if (pximg[i][*tile_x-1] == hscolor)
		{
			hs_y_tmp = i;
			i = img_h;
		}

	/* find top hotspot relative to containing image */
	for (i = *tile_x; i <= (*tile_x + *tile_w); i++)
		if (pximg[*tile_y-1][i] == hscolor)
		{
			hs_x_tmp = i;
			i = img_w;
		}


	/* cut transparent rows off the top */
	for (i = 0; i < *tile_h; i++) {
		for (j = 0; j < *tile_w; j++) {
			if (img[i + *tile_y][(j + *tile_x)*4+3]  != 0x00) {
				*tile_y += i;
				*tile_h -= i;
				i = *tile_h;
				j = *tile_w;
			}
		}
	}
	
	/* cut transparent rows off the bottom */
	for (i = *tile_h; i > 0; i--) {
		for (j = 0; j < *tile_w; j++) {
			if (img[i + *tile_y][(j + *tile_x)*4+3]  != 0x00) {
				*tile_h = i + 1;
				i = 0;
				j = *tile_w;
			}
		}
	}
	
	/* cut transparent rows off the left side */
	for (i = 0; i < *tile_w; i++) {
		for (j = 0; j < *tile_h; j++) {
			if (img[j + *tile_y][(i + *tile_x)*4+3]  != 0x00) {
				*tile_x += i;
				*tile_w -= i;
				i = *tile_w;
				j = *tile_h;
			}
		}
	}
	
	/* cut transparent rows off the right side */
	for (i = *tile_w; i > 0; i--) {
		for (j = 0; j < *tile_h; j++) {
			if (img[j + *tile_y][(i + *tile_x)*4+3]  != 0x00) {
				*tile_w = i + 1;
				i = 0;
				j = *tile_h;
			}
		}
	}
	
	/* calc hotspot relative to the topleft corner of the shrunken tile */
	*hs_x = *tile_x - hs_x_tmp;
	*hs_y = *tile_y - hs_y_tmp;
	
	tile = malloc(*tile_h * sizeof(png_byte*));
	for (i = 0; i < *tile_h; i++)
		tile[i] = &(img[i+*tile_y][*tile_x*4]);
	
	return tile;
}



/********************************************************************************************************
 * getindex: looks up a name in the namearray for its type to determine its position in the output array
 ********************************************************************************************************/
int getindex(enum tiletype type, char * name)
{
	int i;
	
	if (type >= MAX_TYPES)
	{
		fprintf(stderr, "ERROR: Encountered invalid tile type %d. Quitting.\n", type);
		exit(1);
	}
	
	for (i = 0; i < typecount[type]; i++)
		if (strcmp(name, typenames[type][i]) == 0)
			return typeoffset[type] + i;
	
	return -1;
}



/***********************************************************************************
 * load_tiles: main control function for ... guess what ;)
 * load_tiles iterates over the location description array, appends the image data
 * to the output file and saves information about the tile
 ***********************************************************************************/
void load_tiles(char * outname, char * imgpath, int wantpal)
{
	int i, num_pal, loadcount = 0;
	png_uint_32 width, height;
	png_color * palette = malloc(256 * sizeof(png_color));
	png_byte ** img = NULL;
	png_byte ** tile = NULL;	
	int tile_x, tile_y, tile_w, tile_h, xmod, ymod, hs_x, hs_y;
	long filepos = 0, prev_pos = 0;
	
	char * lastimage = NULL;
	char fullname[1024];
	FILE *outfile = fopen(outname, "wb");
	
	for(i = 0; i < num_tiledefs; i++) {
		if (!tiledefinitions[i].filename)
			/* do nothing, it's a pseudotile */
			continue;

		if (!lastimage || strcmp(lastimage, tiledefinitions[i].filename) != 0) {
			if (lastimage) {
				printf("Loaded %3d tiles from %s\n", loadcount, lastimage);
				loadcount = 0;
			}
			freeimg(&img, height);
		
			lastimage = tiledefinitions[i].filename;
			strcpy(fullname, imgpath);
			strcat(fullname, tiledefinitions[i].filename);
			if (read_png_file(fullname, &img, &width, &height, palette, &num_pal)) {
				fprintf(stderr, "WARNING: could not read png %s!\n", fullname);
				freeimg(&img, height);
			}
		}
		
		if (!img)
			continue;
		
		loadcount++;
		
		tile_x = tiledefinitions[i].topleft_x;
		tile_y = tiledefinitions[i].topleft_y;
		tile_w = tiledefinitions[i].width;
		tile_h = tiledefinitions[i].height;
		xmod = 0;
		ymod = 0;
		
		tile = grab_tile(img, width, height, &tile_x, &tile_y, &tile_w, &tile_h, &hs_x, &hs_y);
		if(tile) {
			if (!wantpal)
				num_pal = 0; /* will cause the image not to be reduced to a palette */
		
			prev_pos = ftell(outfile);
			write_png_file(outfile, tile, tile_w, tile_h, palette, num_pal);
			filepos = ftell(outfile);
			
			int index = getindex(tiledefinitions[i].type, tiledefinitions[i].name);
			/* index == -1 means the object does not exist.
			 * That is not an error, because we might be seenig a
			 * slashem only object, while compiling nethack */
			if (prev_pos != filepos && index != -1)
			{
				gametiles[index].file_offset = prev_pos;
				gametiles[index].data_len = filepos - prev_pos;
				gametiles[index].ptr_to = index;
				gametiles[index].hot_x = hs_x;
				gametiles[index].hot_y = hs_y;				
			}
		
			free(tile);
		}
		
	}
	printf("Loaded %3d tiles from %s\n", loadcount, lastimage);
	printf("\n");
	
	freeimg(&img, height);
	free(palette);
	
	fclose(outfile);
	
}


/***********************************************************************************
 * the init_<foo>names functions create arrays of all valid names for that class
 *
 * for monsters and objects this means doing virtually the same as makedefs;
 * for other classes we only need a list of all the names so that getindex can work
 * correctly
 ***********************************************************************************/
void init_objnames()
{
	int i;
	char * c;
	
	for(i = 0; !i || objects[i].oc_class != ILLOBJ_CLASS; i++)
		typecount[T_OBJECT]++;
	
	typenames[T_OBJECT] = malloc(typecount[T_OBJECT] * sizeof(char*));
	
	for(i = 0; !i || objects[i].oc_class != ILLOBJ_CLASS; i++)
	{
		typenames[T_OBJECT][i] = malloc(31 * sizeof(char)); /* makedefs uses max 30 chars + '\0' */
		typenames[T_OBJECT][i][0] = '\0';
		typenames[T_OBJECT][i][30] = '\0';
		
		if (!obj_descr[i].oc_name)
			continue;
		
		switch (objects[i].oc_class)
		{
			case WAND_CLASS:
				snprintf(typenames[T_OBJECT][i], 30, "WAN_%s", obj_descr[i].oc_name); break;
			case RING_CLASS:
				snprintf(typenames[T_OBJECT][i], 30, "RIN_%s", obj_descr[i].oc_name); break;
			case POTION_CLASS:
				snprintf(typenames[T_OBJECT][i], 30, "POT_%s", obj_descr[i].oc_name); break;
			case SPBOOK_CLASS:
				snprintf(typenames[T_OBJECT][i], 30, "SPE_%s", obj_descr[i].oc_name); break;
			case SCROLL_CLASS:
				snprintf(typenames[T_OBJECT][i], 30, "SCR_%s", obj_descr[i].oc_name); break;
			case AMULET_CLASS:
				if(objects[i].oc_material == PLASTIC)
					snprintf(typenames[T_OBJECT][i], 30, "FAKE_AMULET_OF_YENDOR");
				else
					snprintf(typenames[T_OBJECT][i], 30, "%s", obj_descr[i].oc_name); break;
			case GEM_CLASS:
				if (objects[i].oc_material == GLASS)
				{
					switch (objects[i].oc_color)
					{
						case CLR_WHITE:  snprintf(typenames[T_OBJECT][i], 30, "GEM_WHITE_GLASS"); break;
						case CLR_BLUE:   snprintf(typenames[T_OBJECT][i], 30, "GEM_BLUE_GLASS");  break;
						case CLR_RED:    snprintf(typenames[T_OBJECT][i], 30, "GEM_RED_GLASS");   break;
						case CLR_BROWN:  snprintf(typenames[T_OBJECT][i], 30, "GEM_BROWN_GLASS"); break;
						case CLR_ORANGE: snprintf(typenames[T_OBJECT][i], 30, "GEM_ORANGE_GLASS");break;
						case CLR_YELLOW: snprintf(typenames[T_OBJECT][i], 30, "GEM_YELLOW_GLASS");break;
						case CLR_BLACK:  snprintf(typenames[T_OBJECT][i], 30, "GEM_BLACK_GLASS"); break;
						case CLR_GREEN:  snprintf(typenames[T_OBJECT][i], 30, "GEM_GREEN_GLASS"); break;
						case CLR_MAGENTA:snprintf(typenames[T_OBJECT][i], 30, "GEM_VIOLET_GLASS");break;
					}
					glassgems[glassgemcount++] = i;
				}
				else
					snprintf(typenames[T_OBJECT][i], 30, "%s", obj_descr[i].oc_name); break;
				break;
			default:
				snprintf(typenames[T_OBJECT][i], 30, "%s", obj_descr[i].oc_name); break;
		}
		
		for (c = typenames[T_OBJECT][i]; *c; c++)
		{
			if (*c >= 'a' && *c <= 'z') *c -= (char)('a' - 'A');
			else if (*c < 'A' || *c > 'Z') *c = '_';
		}
	}	
}


void init_monnames()
{
	char * c;
	int i;
	
	for (i = 0; mons[i].mlet; i++)
		typecount[T_MONSTER]++;
		
	typenames[T_MONSTER] = malloc(typecount[T_MONSTER] * sizeof(char*));
	
	for (i = 0; mons[i].mlet; i++)
	{
		typenames[T_MONSTER][i] = malloc(64 * sizeof(char));
		if (mons[i].mlet == S_HUMAN && !strncmp(mons[i].mname, "were", 4))
			snprintf(typenames[T_MONSTER][i], 64, "PM_HUMAN_%s", mons[i].mname);
		else
			snprintf(typenames[T_MONSTER][i], 64, "PM_%s", mons[i].mname);
		
		for (c = typenames[T_MONSTER][i]; *c; c++)
		{
			if (*c >= 'a' && *c <= 'z') *c -= (char)('a' - 'A');
			else if (*c < 'A' || *c > 'Z') *c = '_';
		}
	}
	
	typecount[T_STATUE] = typecount[T_MONSTER];
	typenames[T_STATUE] = typenames[T_MONSTER];
	
	typecount[T_FIGURINE] = typecount[T_MONSTER];
	typenames[T_FIGURINE] = typenames[T_MONSTER];
}


void init_gen_names(enum tiletype type)
{
	int i;
	int j = 0;

	for (i = 0; i < num_tiledefs; i++)
		if (tiledefinitions[i].type == type)
			typecount[type]++;
	
	typenames[type] = malloc(typecount[type] * sizeof(char*));
	
	for (i = 0; i < num_tiledefs; i++)
		if (tiledefinitions[i].type == type)
			typenames[type][j++] = tiledefinitions[i].name;
}


void init_types()
{
	int i;
	
	for (i = 0; i < MAX_TYPES; i++)
	{
		typecount[i] = 0;
		typeoffset[i] = 0;
		typenames[i] = NULL;
		type_to_name[i] = "UNNAMED TYPE!";
	}
	
	type_to_name[T_OBJECT]  = "OBJECTS";
	type_to_name[T_SPECIAL] = "SPECIAL TILES";
	type_to_name[T_DUNGEON] = "DUNGEON TILES";
	type_to_name[T_MONSTER] = "MONSTERS";
	type_to_name[T_STATUE]  = "STATUES";
	type_to_name[T_FIGURINE]= "FIGURINES";
	type_to_name[T_CURSOR]  = "CURSORS";
	
}


/**********************************************************************
 * do_eqmap uses the fallback mappings and the tilemap to create pointers
 * to tiles for monsters/objects that don't have one of their own
 **********************************************************************/
void do_eqmap()
{
	int i, j, x, y;
	
	int objfallback = getindex(T_OBJECT, "STRANGE_OBJECT");
	int * objclassidx = malloc(MAXOCLASSES * sizeof(int));
	
	/* point all object entries that don't have a tile to a suitable fallback  */
	memset(objclassidx, -1, MAXOCLASSES * sizeof(int));
	for (i = 0; i < num_objclassmaps; i++)
	{
		j = getindex(T_OBJECT, objclassmap[i].mapto);
		if (j != -1)
			objclassidx[objclassmap[i].class] = j;
	}
	
	for (i = typeoffset[T_OBJECT]; i < (typeoffset[T_OBJECT] + typecount[T_OBJECT]); i++)
	{
		if (gametiles[i].data_len == 0 && gametiles[i].ptr_to == -1)
			gametiles[i].ptr_to = objclassidx[(int)objects[i - typeoffset[T_OBJECT]].oc_class] + typeoffset[T_OBJECT];
		
		/* objclassidx[objects[i-objoffset].otyp] might give -1 if it was not set */
		if (gametiles[i].data_len == 0 && gametiles[i].ptr_to == -1)
			gametiles[i].ptr_to = objfallback + typeoffset[T_OBJECT];
	}
	free (objclassidx);


	int monfallback = getindex(T_MONSTER, "PM_KNIGHT");
	int statfallback = getindex(T_OBJECT, "STATUE");
	int figfallback = getindex(T_OBJECT, "FIGURINE");
	int mon_to_fig = typeoffset[T_FIGURINE] - typeoffset[T_MONSTER];
	int mon_to_sta = typeoffset[T_STATUE] - typeoffset[T_MONSTER];

	int * monclassidx = malloc(MAXMCLASSES * sizeof(int));
	memset(monclassidx, -1, MAXMCLASSES * sizeof(int));
	for (i = 0; i < num_monclassmaps; i++)
	{
		j = getindex(T_MONSTER, monclassmap[i].mapto);
		if (j != -1)
			monclassidx[monclassmap[i].class] = j;
	}
	
	for (i = 0; i < typecount[T_MONSTER]; i++)
	{
		if (gametiles[i+typeoffset[T_MONSTER]].ptr_to == -1)
			gametiles[i+typeoffset[T_MONSTER]].ptr_to = (monclassidx[(int)mons[i].mlet] != -1) ?
				monclassidx[(int)mons[i].mlet] : monfallback;
	
		if (gametiles[i+typeoffset[T_FIGURINE]].ptr_to == -1)
		{
			if ((monclassidx[(int)mons[i].mlet] != -1) && gametiles[monclassidx[(int)mons[i].mlet] + mon_to_fig].data_len != 0)
				gametiles[i+typeoffset[T_FIGURINE]].ptr_to = monclassidx[(int)mons[i].mlet] + mon_to_fig;
			else
				gametiles[i+typeoffset[T_FIGURINE]].ptr_to = figfallback;
		}

		if (gametiles[i+typeoffset[T_STATUE]].ptr_to == -1)
		{
			if ((monclassidx[(int)mons[i].mlet] != -1) && gametiles[monclassidx[(int)mons[i].mlet] + mon_to_sta].data_len != 0)
				gametiles[i+typeoffset[T_STATUE]].ptr_to = monclassidx[(int)mons[i].mlet] + mon_to_sta;
			else
				gametiles[i+typeoffset[T_STATUE]].ptr_to = statfallback;
		}
	}
	free(monclassidx);
	
	
	/* override generic mappings with explicit mappings */
	for (i = 0; i < num_eqmapentries; i++)
	{
		x = getindex(tilemap[i].type, tilemap[i].mapfrom);
		y = getindex(tilemap[i].type, tilemap[i].mapto);
		
		if (y == -1)
			printf ("WARNING: Could not map %s to %s because I don't know %s.\n",
			        tilemap[i].mapfrom, tilemap[i].mapto, tilemap[i].mapto);
		
		if (x != -1 && gametiles[x].data_len == 0)
		{
			if (gametiles[y].data_len != 0)
				gametiles[x].ptr_to = y;
			else
				printf("WARNING: Entry %d in tilemap maps %s to %s, which does not have a unique tile itself. Entry ignored\n", i, tilemap[i].mapfrom, tilemap[i].mapto);
		}
	}
}


/************************************************************************************
 * init_<foo> functions: walls, floors and floor edges all consist of groups of tiles
 * these functions use a basename + extensions system to put the groups together
 ************************************************************************************/
void init_floorstyles()
{
	int i, j, k;
	int cur_ftile = 0;
	char namebuf[128];
	
	for (i = 0; i < num_fstyles; i++)
		num_ftiles += fstyles[i].x * fstyles[i].y;
	
	fs_start = malloc(sizeof(int) * num_fstyles);
	floor_style_indices = malloc(sizeof(int) * num_ftiles);
	
	for (i = 0; i < num_fstyles; i++)
	{
		fs_start[i] = cur_ftile;
		for (j = 0; j < fstyles[i].y; j++)
		{
			for (k = 0; k < fstyles[i].x; k++)
			{
				snprintf(namebuf, 64, "%s_%d_%d", fstyles[i].basename, k, j);
				floor_style_indices[cur_ftile] = getindex(T_DUNGEON, namebuf);
				if (floor_style_indices[cur_ftile] == -1)
				{
					fprintf(stderr, "ERROR: Component tile %d,%d of floor edge style %s was not found\n", k,j, fstyles[i].basename);
					exit(1);	
				}
				cur_ftile++;
			}
		}
	}
}

void init_edgestyles()
{
	int i,j;
	char namebuf[128];
	
	floor_edge_indices = malloc (sizeof(int*) * num_floor_edges);
	
	for (i = 0; i < num_floor_edges; i++)
	{
		floor_edge_indices[i] = malloc(12 * sizeof(int));
		for (j = 0; j < 12; j++)
		{
			snprintf(namebuf, 64, "%s_%d", floor_edges[i], j+1);
			floor_edge_indices[i][j] = getindex(T_DUNGEON, namebuf);
			if (floor_edge_indices[i][j] == -1)
			{
				fprintf(stderr, "ERROR: Component tile %d of floor edge style %s was not found\n", j+1, floor_edges[i]);
				exit(1);
			}
		}
	}
}



void init_walltiles()
{
	int i, j;
	char tilename[128];
	char types[] = "FHT";
	walls = malloc(3 * sizeof(struct wallstyle*));
	
	for (i = 0; i < 3; i++)
	{
		walls[i] = malloc(num_wall_styles * sizeof(struct wallstyle));
		for (j = 0; j < num_wall_styles; j++)
		{
			snprintf(tilename, 128, "%s_%c_%c", wall_styles[j], types[i], 'N');
			walls[i][j].north = getindex(T_DUNGEON, tilename);
			if (walls[i][j].north == -1)
			{
				fprintf(stderr, "Expected to find tile %s for type %d, but it does not exist.\n", tilename, j);
				exit(1);
			}
			snprintf(tilename, 128, "%s_%c_%c", wall_styles[j], types[i], 'E');
			walls[i][j].east = getindex(T_DUNGEON, tilename);
			if (walls[i][j].east == -1)
			{
				fprintf(stderr, "Expected to find tile %s, but it does not exist.\n", tilename);
				exit(1);
			}
			snprintf(tilename, 128, "%s_%c_%c", wall_styles[j], types[i], 'S');
			walls[i][j].south = getindex(T_DUNGEON, tilename);
			if (walls[i][j].south == -1)
			{
				fprintf(stderr, "Expected to find tile %s, but it does not exist.\n", tilename);
				exit(1);
			}
			snprintf(tilename, 128, "%s_%c_%c", wall_styles[j], types[i], 'W');
			walls[i][j].west = getindex(T_DUNGEON, tilename);
			if (walls[i][j].west == -1)
			{
				fprintf(stderr, "Expected to find tile %s, but it does not exist.\n", tilename);
				exit(1);
			}
		}
		
	}
}



/********************************************************************************************
 * output: creates the c source output that describes the content of the binary image archive
 ********************************************************************************************/
void output()
{
	int i, j;
	/* Write vultures_gametiles.c */
	FILE * fp = fopen("vultures_gametiles.c", "w");
	
	fprintf(fp, "/* This file was generated by tiletrans. DO NOT EDIT THIS FILE */\n");
	fprintf(fp, "#include \"vultures_gametiles.h\"\n\n");
	
	
	/* ftilearry contains the indices of all the floor tiles in vultures_gametiles[] */
	fprintf(fp, "int ftilearray[] = {");
	for (i = 0; i < num_ftiles; i++)
		fprintf(fp, " %d%c", floor_style_indices[i], (i < num_ftiles-1) ? ',' : '}');
	fprintf(fp, ";\n\n");
	
	
	/* output infos about the floor stiles themselves: x, y and a pointer to the section of ftilearray[]
	 * which contains the floor tile indices for this floor style */
	fprintf(fp, "const struct fstyles floorstyles[] = {\n");
	for (i = 0; i < num_fstyles; i++)
		fprintf(fp, "/* V_%-18.18s */ { %d, %d, &ftilearray[%d] }%c\n", fstyles[i].basename, fstyles[i].x, fstyles[i].y, fs_start[i], (i < num_fstyles-1)?',' : ' ');
	fprintf(fp, "};\n\n");
	
	
	/* floor edges */
	fprintf(fp, "const struct fedges flooredges[] = {\n");
	for (i = 0; i < num_floor_edges; i++)
		fprintf(fp, "/* V_%-22.22s */ {%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d}%c\n", floor_edges[i],
		            floor_edge_indices[i][0], floor_edge_indices[i][1], floor_edge_indices[i][2], floor_edge_indices[i][3],
		            floor_edge_indices[i][4], floor_edge_indices[i][5], floor_edge_indices[i][6], floor_edge_indices[i][7],
		            floor_edge_indices[i][8], floor_edge_indices[i][9], floor_edge_indices[i][10], floor_edge_indices[i][11],
		            (i < num_floor_edges-1) ? ',' : ' ');
	fprintf(fp, "};\n\n");
	
	
	/* output wall tiles in 3 variants */
	fprintf(fp, "/* full height wall tiles */\n");
	fprintf(fp, "const struct walls walls_full[] = {\n");
	for (i = 0; i < num_wall_styles; i++)
		fprintf(fp, "/* V_%-22.22s */ {%3d, %3d, %3d, %3d}%c\n", wall_styles[i], walls[0][i].north, walls[0][i].east, walls[0][i].south, walls[0][i].west, (i < num_wall_styles-1)? ',' : ' ');
	fprintf(fp, "};\n");
	fprintf(fp, "/* half height wall tiles */\n");
	fprintf(fp, "const struct walls walls_half[] = {\n");
	for (i = 0; i < num_wall_styles; i++)
		fprintf(fp, "/* V_%-22.22s */ {%3d, %3d, %3d, %3d}%c\n", wall_styles[i], walls[1][i].north, walls[1][i].east, walls[1][i].south, walls[1][i].west, (i < num_wall_styles-1)? ',' : ' ');
	fprintf(fp, "};\n");
	fprintf(fp, "/* transparent wall tiles */\n");
	fprintf(fp, "const struct walls walls_trans[] = {\n");
	for (i = 0; i < num_wall_styles; i++)
		fprintf(fp, "/* V_%-22.22s */ {%3d, %3d, %3d, %3d}%c\n", wall_styles[i], walls[2][i].north, walls[2][i].east, walls[2][i].south, walls[2][i].west, (i < num_wall_styles-1)? ',' : ' ');
	fprintf(fp, "};\n\n");
	
	
	/* output the main game tile array */
	fprintf(fp, "const struct gametiles vultures_gametiles[] = {\n");
	
	for (j = 0; j < MAX_TYPES; j++)
	{
		fprintf(fp, "\n/* %s: */\n", type_to_name[j]);
		for(i = 0; i < typecount[j]; i++)
		{
			fprintf(fp, "/* %04d: %-25.25s */ {%8ld, %5ld, %4d, %4d, %4d},\n", i+typeoffset[j], typenames[j][i], gametiles[i+typeoffset[j]].file_offset, gametiles[i+typeoffset[j]].data_len, gametiles[i+typeoffset[j]].ptr_to, gametiles[i+typeoffset[j]].hot_x, gametiles[i+typeoffset[j]].hot_y );
		}
	}
	
	fprintf(fp, "};\n");
	
	fclose(fp);
	
	
	/* output vultures_gametiles.h */
	fp = fopen("vultures_gametiles.h", "w");
	fprintf(fp, "/* This file was generated by tiletrans. DO NOT EDIT THIS FILE */\n\n");
	
	fprintf(fp, "#ifndef _vultures_gametiles_h_\n#define _vultures_gametiles_h_\n\n");
	
	for (i = 0; i < MAX_TYPES; i++)
	{
		fprintf(fp, "#define %3.3sTILECOUNT  %d\n", type_to_name[i], typecount[i]);
	}
	fprintf(fp, "#define GAMETILECOUNT %d\n\n", gametilecount);

	for (i = 0; i < MAX_TYPES; i++)
		fprintf(fp, "#define %3.3sTILEOFFSET  %d\n", type_to_name[i], typeoffset[i]);
	
	fprintf(fp, "\n");

	
	fprintf(fp, "#define OBJECT_TO_VTILE(obj_id) ((obj_id) + OBJTILEOFFSET)\n");
	fprintf(fp, "#define MONSTER_TO_VTILE(mon_id) ((mon_id) + MONTILEOFFSET)\n");
	fprintf(fp, "#define STATUE_TO_VTILE(mon_id) ((mon_id) + STATILEOFFSET)\n");
	fprintf(fp, "#define FIGURINE_TO_VTILE(mon_id) ((mon_id) + FIGTILEOFFSET)\n\n");

	fprintf(fp, "struct gametiles {\n\tlong file_offset;\n\tlong data_len;\n\tint ptr;\n\tint hs_x;\n\tint hs_y;\n};\n\n");
	fprintf(fp, "struct fstyles {\n\tint x;\n\tint y;\n\tint * array;\n};\n\n");
	fprintf(fp, "struct walls {\n\tint north;\n\tint east;\n\tint south;\n\tint west;\n};\n\n");
	fprintf(fp, "struct fedges {\n\t int west, north, south, east;\n\tint southwest, northwest, southeast, northeast;\n\t");
	fprintf(fp, "int southwest_bank, northwest_bank, southeast_bank, northeast_bank;\n};\n\n");

	fprintf(fp, "enum sptiles {\n");
	fprintf(fp, "\tV_TILE_%s = SPETILEOFFSET,\n", typenames[T_SPECIAL][0]);
	for (i = 1; i < typecount[T_SPECIAL]; i++)
		fprintf(fp, "\tV_TILE_%s,\n", typenames[T_SPECIAL][i]);
	fprintf(fp, "\tV_TILE_MAX\n};\n\n");
	
	fprintf(fp, "enum floorstyles {\n");
	for (i = 0; i < num_fstyles; i++)
		fprintf(fp, "\tV_%s,\n", fstyles[i].basename);
	fprintf(fp, "\tV_FLOOR_STYLE_MAX\n};\n\n");
	
	fprintf(fp, "enum flooredges {\n");
	for (i = 0; i < num_floor_edges; i++)
		fprintf(fp, "\tV_%s,\n", floor_edges[i]);
	fprintf(fp, "\tV_FLOOR_EDGE_MAX\n};\n\n");
	
	fprintf(fp, "enum wallstyles {\n");
	for (i = 0; i < num_wall_styles; i++)
		fprintf(fp, "\tV_%s,\n", wall_styles[i]);
	fprintf(fp, "\tV_WALL_STYLE_MAX\n};\n\n");
	
	fprintf(fp, "enum mcursor {\n");
	for (i = 0; i < typecount[T_CURSOR]; i++)
		fprintf(fp, "\tV_%s,\n", typenames[T_CURSOR][i]);
	fprintf(fp, "\tV_CURSOR_MAX\n};\n\n");

	
	fprintf(fp, "extern const struct walls walls_full[];\n");
	fprintf(fp, "extern const struct walls walls_half[];\n");
	fprintf(fp, "extern const struct walls walls_trans[];\n");
	fprintf(fp, "extern const struct fedges flooredges[];\n");
	fprintf(fp, "extern const struct fstyles floorstyles[];\n");
	fprintf(fp, "extern const struct gametiles vultures_gametiles[];\n\n");
	
	fprintf(fp, "/* define glass gem names */\n");
	for (i = 0; i < glassgemcount; i++)
	{
		fprintf(fp, "#define ");
		switch (objects[glassgems[i]].oc_color)
		{
			case CLR_WHITE:  fprintf(fp, "GEM_WHITE_GLASS"); break;
			case CLR_BLUE:   fprintf(fp, "GEM_BLUE_GLASS");  break;
			case CLR_RED:    fprintf(fp, "GEM_RED_GLASS");   break;
			case CLR_BROWN:  fprintf(fp, "GEM_BROWN_GLASS"); break;
			case CLR_ORANGE: fprintf(fp, "GEM_ORANGE_GLASS");break;
			case CLR_YELLOW: fprintf(fp, "GEM_YELLOW_GLASS");break;
			case CLR_BLACK:  fprintf(fp, "GEM_BLACK_GLASS"); break;
			case CLR_GREEN:  fprintf(fp, "GEM_GREEN_GLASS"); break;
			case CLR_MAGENTA:fprintf(fp, "GEM_VIOLET_GLASS");break;
		}
		fprintf(fp, " %d\n", glassgems[i]);
	}
	
	fprintf(fp, "\n\n#endif /* defined _vultures_gametiles_h_*/\n");
	
	fclose(fp);
}


int main(int argc, char * argv[])
{
	int i;
	int wantpal = 0;
	
	if (argc != 4)
	{
		printf("usage: %s (pal|rgba) outfilename <path/to/images/>\n", argv[0]);
		return 1;
	}
	
	if (strncmp(argv[1], "pal", 3) == 0)
		wantpal = 1;
	
	init_types();
	
	init_objnames();
	init_monnames();
	init_gen_names(T_SPECIAL);
	init_gen_names(T_DUNGEON);
	init_gen_names(T_CURSOR);
	
	for (i = 1; i < MAX_TYPES; i++)
		typeoffset[i] = typeoffset[i-1] + typecount[i-1];

	gametilecount = typeoffset[MAX_TYPES-1] + typecount[MAX_TYPES-1];

	gametiles = malloc(gametilecount * (sizeof(struct gametile)));

	for (i = 0; i < gametilecount; i++)
	{
		gametiles[i].file_offset = 0;
		gametiles[i].data_len = 0;
		gametiles[i].ptr_to = -1;
		gametiles[i].hot_x = 0;
		gametiles[i].hot_y = 0;
	}
	
	load_tiles(argv[2], argv[3], wantpal);
	
	do_eqmap();
	
	init_floorstyles();
	init_edgestyles();
	init_walltiles();
	
	output();	
	
	printf("tiletrans: Wrote %d tiles in %s format to %s\n\n", num_tiledefs, wantpal ? "8-bit paletted" : "32-bit RGBA", argv[2]);

	return 0;
}

