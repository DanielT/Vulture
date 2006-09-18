/* tiletrans.c: tile transformer/compiler
 * Copyright (c) 2005 Daniel Thaler
 * This file may be freely distributed under the Nethack license*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
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

/* The definition of EXPL_MAX is taken from hack.h. Including hack.h here
 * gives a conflict with zlib, which is included via png.h */
#define EXPL_MAX 7

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

/* record the largest distance to the right/bottom from the hotspot for all tiles */
int tile_max_right = 0;
int tile_max_bottom = 0;

struct wallstyle ** walls = NULL;

struct gametile * gametiles = NULL;

int typecount[MAX_TYPES];
int typeoffset[MAX_TYPES];
char ** typenames[MAX_TYPES];
char * type_to_name[MAX_TYPES];

int gametilecount = 0;
int loaded_objects = 0;
int loaded_monsters = 0;

/* nethack does not #define the worthless pieces of glass,
 * so we remember what object id's they have and do it ourselves */
int glassgems[20];
int glassgemcount = 0;

int binfilesize = 0;


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
 * read_png_file: reads a png file and automatically converts it to RGBA
 * while doing so
 ***********************************************************************/
int read_png_file(char * filename, png_byte *** image, png_uint_32 * width, png_uint_32 * height)
{
	unsigned char sigbuf[PNG_BYTES_TO_CHECK];
	FILE * fp = fopen(filename, "rb");
	
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int row;
	int bit_depth, color_type;
	int pal_present;
	
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
		pal_present = 1;
	else
		pal_present = 0;

	/* Expand paletted or RGB images */
	png_set_expand(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	info_ptr->color_type |= PNG_COLOR_MASK_ALPHA;

	/* update the png_info to reflect the requested transformations so that png_rowbytes will return the correct number */
	png_read_update_info(png_ptr, info_ptr);
	
	*image = malloc(*height * sizeof(png_byte*));
	for (row = 0; row < *height; row++)
		(*image)[row] = malloc(png_get_rowbytes(png_ptr, info_ptr));

	png_read_image(png_ptr, *image);

	/* read rest of file, and get additional chunks in info_ptr */
	/* png_read_end(png_ptr, info_ptr); */ /* contains only comments and metadata, which we don't care about */

	/* At this point you have read the entire image */

	/* clean up after the read, and free any memory allocated */
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

	/* close the file */
	fclose(fp);
	
	if (pal_present) { /* in our images we need to translate the "transparent" color to real transparency */
		unsigned int ** pxdata = (unsigned int **)*image;
		unsigned int transcolor = SDL_SwapLE32((0xff << 24) + 91);
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
 ********************************************************************************/
int write_png_file(FILE *outfile, png_byte ** tile, png_uint_32 width, png_uint_32 height)
{
	png_structp png_ptr;
	png_infop info_ptr;
		
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		/* If we get here, we had a problem writing the file */
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return 1;
	}
	
	png_init_io(png_ptr, outfile);
	
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
	             PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, tile);
	png_write_end(png_ptr, info_ptr);
	
	png_destroy_write_struct(&png_ptr, &info_ptr);

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
void load_tiles(char * outname, char * imgpath)
{
	int i, loadcount = 0;
	png_uint_32 width, height;
	png_byte ** img = NULL;
	png_byte ** tile = NULL;	
	int tile_x, tile_y, tile_w, tile_h, xmod, ymod, hs_x, hs_y;
	long filepos = 0, prev_pos = 0;
	int index;
	
	char * lastimage = NULL;
	char fullname[1024];
	FILE *outfile = fopen(outname, "wb");
	
	for(i = 0; i < num_tiledefs; i++) {
        index = getindex(tiledefinitions[i].type, tiledefinitions[i].name);
		if (!tiledefinitions[i].filename || index == -1)
			/* do nothing if it's a pseudotile or */
			/* index == -1 which means the tile does not exist.
			 * That is not an error, because we might be seeing a
			 * slashem only tile, while compiling nethack */
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
			if (read_png_file(fullname, &img, &width, &height)) {
				fprintf(stderr, "WARNING: could not read png %s!\n", fullname);
				freeimg(&img, height);
			}
		}
		
		if (!img)
			continue;
		
		loadcount++;
		
		if (tiledefinitions[i].type == T_OBJECT)
            loaded_objects++;
        else if (tiledefinitions[i].type == T_MONSTER)
            loaded_monsters++;
		
		tile_x = tiledefinitions[i].topleft_x;
		tile_y = tiledefinitions[i].topleft_y;
		tile_w = tiledefinitions[i].width;
		tile_h = tiledefinitions[i].height;
		xmod = 0;
		ymod = 0;
		
		tile = grab_tile(img, width, height, &tile_x, &tile_y, &tile_w, &tile_h, &hs_x, &hs_y);
		if(tile) {
            tile_max_right = tile_max_right > (tile_w + hs_x) ? tile_max_right : (tile_w + hs_x);
            tile_max_bottom = tile_max_bottom > (tile_h + hs_y) ? tile_max_bottom : (tile_h + hs_y);
		
			prev_pos = ftell(outfile);
			write_png_file(outfile, tile, tile_w, tile_h);
			filepos = ftell(outfile);
			
			if (prev_pos != filepos)
			{
                if (gametiles[index].line)
                    printf("WARNING: Definition of %s on line %d overrides previous definition on line %d!\n",
                             tiledefinitions[i].name, tiledefinitions[i].line, gametiles[index].line);
				gametiles[index].file_offset = prev_pos;
				gametiles[index].data_len = filepos - prev_pos;
				gametiles[index].ptr_to = index;
				gametiles[index].hot_x = hs_x;
				gametiles[index].hot_y = hs_y;
				gametiles[index].line = tiledefinitions[i].line;
			}
		
			free(tile);
		}
		
	}
	printf("Loaded %3d tiles from %s\n", loadcount, lastimage);
	printf("\n");
	
	freeimg(&img, height);
	
	fclose(outfile);
	
	binfilesize = filepos;
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
	char *c, *nameptr, nonamebuf[16];
	int unnamed_cnt = 0;
	
	for(i = 0; !i || objects[i].oc_class != ILLOBJ_CLASS; i++)
		typecount[T_OBJECT]++;
	
	typenames[T_OBJECT] = malloc(typecount[T_OBJECT] * sizeof(char*));
	
	for(i = 0; !i || objects[i].oc_class != ILLOBJ_CLASS; i++)
	{
		typenames[T_OBJECT][i] = malloc(31 * sizeof(char)); /* makedefs uses max 30 chars + '\0' */
		typenames[T_OBJECT][i][0] = '\0';
		typenames[T_OBJECT][i][30] = '\0';
		
		if (!obj_descr[i].oc_name)
		{
            snprintf(nonamebuf, 16, "unnamed %d", ++unnamed_cnt);
            nameptr = nonamebuf;
		}
		else
            nameptr = (char*)obj_descr[i].oc_name;
		
		switch (objects[i].oc_class)
		{
			case WAND_CLASS:
				snprintf(typenames[T_OBJECT][i], 30, "WAN_%s", nameptr); break;
			case RING_CLASS:
				snprintf(typenames[T_OBJECT][i], 30, "RIN_%s", nameptr); break;
			case POTION_CLASS:
				snprintf(typenames[T_OBJECT][i], 30, "POT_%s", nameptr); break;
			case SPBOOK_CLASS:
				snprintf(typenames[T_OBJECT][i], 30, "SPE_%s", nameptr); break;
			case SCROLL_CLASS:
				snprintf(typenames[T_OBJECT][i], 30, "SCR_%s", nameptr); break;
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
					snprintf(typenames[T_OBJECT][i], 30, "%s", nameptr); break;
				break;
			default:
				snprintf(typenames[T_OBJECT][i], 30, "%s", nameptr); break;
		}
		
		for (c = typenames[T_OBJECT][i]; *c; c++)
		{
			if (*c >= 'a' && *c <= 'z') *c -= (char)('a' - 'A');
			else if ((*c < 'A' || *c > 'Z') && !isdigit(*c)) *c = '_';
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


void init_explnames()
{
    int i, j;
    const char * explosion_names[] = {"DARK", "NOXIOUS", "MUDDY", "WET", "MAGICAL", "FIERY", "FROSTY"};

    typecount[T_EXPL] = EXPL_MAX * 9;

    typenames[T_EXPL] = malloc(typecount[T_EXPL] * sizeof(char*));

    for (i = 0; i < EXPL_MAX; i++)
    {
        for (j = 0; j < 9; j++)
        {
            typenames[T_EXPL][i*9+j] = malloc(64 * sizeof(char));
            snprintf(typenames[T_EXPL][i*9+j], 64, "%s_%d", explosion_names[i], j+1);
        }
    }
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
	type_to_name[T_EXPL]    = "EXPLOSION TILES";
	type_to_name[T_DUNGEON] = "DUNGEON TILES";
	type_to_name[T_FLOOR]   = "FLOOR TILES";
	type_to_name[T_WALL]    = "WALL TILES";
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
	
	int expl_dir;
	char namebuf[64];
	for (i = 0; i < typecount[T_EXPL]; i++)
	{
	   expl_dir = i % 9;

	   if (gametiles[i+typeoffset[T_EXPL]].ptr_to == -1)
	   {
	       snprintf(namebuf, 64, "FIERY_%d", expl_dir+1);
	       gametiles[i+typeoffset[T_EXPL]].ptr_to = getindex(T_EXPL, namebuf);
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
				floor_style_indices[cur_ftile] = getindex(T_FLOOR, namebuf);
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
	char types[] = "FH";
	walls = malloc(2 * sizeof(struct wallstyle*));
	
	for (i = 0; i < 2; i++)
	{
		walls[i] = malloc(num_wall_styles * sizeof(struct wallstyle));
		for (j = 0; j < num_wall_styles; j++)
		{
			snprintf(tilename, 128, "%s_%c_%c", wall_styles[j], types[i], 'N');
			walls[i][j].north = getindex(T_WALL, tilename);
			if (walls[i][j].north == -1)
			{
				fprintf(stderr, "Expected to find tile %s for type %d, but it does not exist.\n", tilename, j);
				exit(1);
			}
			snprintf(tilename, 128, "%s_%c_%c", wall_styles[j], types[i], 'E');
			walls[i][j].east = getindex(T_WALL, tilename);
			if (walls[i][j].east == -1)
			{
				fprintf(stderr, "Expected to find tile %s, but it does not exist.\n", tilename);
				exit(1);
			}
			snprintf(tilename, 128, "%s_%c_%c", wall_styles[j], types[i], 'S');
			walls[i][j].south = getindex(T_WALL, tilename);
			if (walls[i][j].south == -1)
			{
				fprintf(stderr, "Expected to find tile %s, but it does not exist.\n", tilename);
				exit(1);
			}
			snprintf(tilename, 128, "%s_%c_%c", wall_styles[j], types[i], 'W');
			walls[i][j].west = getindex(T_WALL, tilename);
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
		fprintf(fp, "/* V_%-22.22s */ {{%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d}}%c\n", floor_edges[i],
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
	fprintf(fp, "struct fedges {\n\t int dir[12];\n");
	fprintf(fp, "};\n\n");

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
	
	fprintf(fp, "\n/* The maximum distance to the left/bottom from the hotspot in any tile */\n");
	fprintf(fp, "#define V_MAX_TILE_XOFFS %d\n", tile_max_right);
	fprintf(fp, "#define V_MAX_TILE_YOFFS %d\n", tile_max_bottom);
	
	fprintf(fp, "\n#define V_BINFILESIZE %d\n", binfilesize);
	
	fprintf(fp, "\n\n#endif /* defined _vultures_gametiles_h_*/\n");
	
	fclose(fp);
}


int main(int argc, char * argv[])
{
	int i;
	
	if (argc != 3)
	{
		printf("usage: %s outfilename <path/to/images/>\n", argv[0]);
		return 1;
	}
		
	init_types();
	
	init_objnames();
	init_monnames();
	init_explnames();
	init_gen_names(T_SPECIAL);
	init_gen_names(T_DUNGEON);
	init_gen_names(T_FLOOR);
	init_gen_names(T_WALL);
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
		gametiles[i].line = 0;
	}
	
	load_tiles(argv[1], argv[2]);
	
	do_eqmap();
	
	init_floorstyles();
	init_edgestyles();
	init_walltiles();
	
	output();	

    printf("tiletrans: Wrote %d tiles to %s\n\n", num_tiledefs, argv[1]);
    printf("%d of %d objects have unique tiles: %02.1f%% done\n",
           loaded_objects, typecount[T_OBJECT], ((double)(loaded_objects*100)/typecount[T_OBJECT]));
    printf("%d of %d monsters have unique tiles: %02.1f%% done\n",
           loaded_monsters, typecount[T_MONSTER], ((double)(loaded_monsters*100)/typecount[T_MONSTER]));
    printf("\n");

	return 0;
}

