/* Copyright (c) Daniel Thaler, 2006                              */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>

#include "hack.h"

#include "vultures_types.h"
#include "vultures_tile.h"
#include "vultures_gra.h"
#include "vultures_gfl.h"
#include "vultures_map.h"
#include "vultures_gen.h"
#include "vultures_opt.h"


#define TILEARRAYLEN (GAMETILECOUNT*3)

#define V_IS_FLOOR(x) (((x) >= FLOTILEOFFSET) &&((x) < (FLOTILEOFFSET + FLOTILECOUNT)))
#define V_IS_WALL(x)  (((x) >= WALTILEOFFSET) &&((x) < (WALTILEOFFSET + WALTILECOUNT)))


/* main tile arrays */
static vultures_tile ** vultures_tiles_cur;
static vultures_tile ** vultures_tiles_prev;

/* data buffer containing gametiles.bin */
static unsigned char * vultures_srcfile;

/* semi-transparent black areas used to shade floortiles */
static SDL_Surface * vultures_ftshade1;
static SDL_Surface * vultures_ftshade2;


static vultures_tile *vultures_make_alpha_player_tile(int monnum, double op_scale);
static vultures_tile * vultures_load_tile(int tile_id);
static inline vultures_tile * vultures_shade_tile(int tile_id);
static inline vultures_tile *vultures_set_tile_alpha(vultures_tile *tile, double opacity);



void vultures_put_tile(int x, int y, int tile_id)
{
    vultures_tile * tile = NULL;

    if (tile_id == V_TILE_NONE)
        return;

    tile = vultures_get_tile(tile_id);

    if (tile != NULL)
        vultures_put_img(x + tile->xmod, y + tile->ymod, tile->graphic);
}


/* vultures_get_tile is responsible for tile "administration"
 * if the tile is already loaded it will return a pointer to it
 * otherwise it loads the tile, stores the pointer and returns it */
vultures_tile * vultures_get_tile(int tile_id)
{
    int real_id = tile_id % GAMETILECOUNT;
    int shadelevel = tile_id - real_id; /* shadelevel is an even multiple of GAMETILECOUNT */

#ifndef EXPORT_TILES
    /* modifiying the tile_id: must come first */
    /* if we have an object, we manipulate the tile id to give shuffled objects */
    if (real_id < NUM_OBJECTS)
    {
        real_id = objects[real_id].oc_descr_idx;
        tile_id = real_id + shadelevel;
    }
#endif

    /* if the tile is merely a pointer to another tile we modify the tile_id */
    if (!vultures_gametiles[real_id].data_len &&
        vultures_gametiles[real_id].ptr != -1)
    {
        real_id = vultures_gametiles[real_id].ptr;
        tile_id = real_id + shadelevel;
    }


    /* specialized load functions: second */
    /* if you are invisible you have the V_TILE_PLAYER_INVIS. here we give that tile a meaning */
    if (tile_id == V_TILE_PLAYER_INVIS)
        vultures_tiles_cur[tile_id] = vultures_make_alpha_player_tile(u.umonnum, canseeself() ? 0.6 : 0.35);

    /* a shaded tile */
    else if (tile_id >= GAMETILECOUNT)
        return vultures_shade_tile(tile_id);

    /* check whether the tile was loaded last turn */
    if (!vultures_tiles_cur[tile_id] && vultures_tiles_prev[tile_id])
        vultures_tiles_cur[tile_id] = vultures_tiles_prev[tile_id];
    else
        vultures_tiles_cur[tile_id] = vultures_load_tile(tile_id);

    return vultures_tiles_cur[tile_id];
}


/* vultures_load_tile is the actual tile loader
 * it returns a pointer to the tile; the caller is expected to free it */
static vultures_tile * vultures_load_tile(int tile_id)
{
    int real_id = tile_id % GAMETILECOUNT;
    vultures_tile * newtile;
    char * data;

    /* check whether the tile was loaded last turn */
    if (vultures_tiles_prev[tile_id])
        return vultures_tiles_prev[tile_id];

    /* check whether the tile was loaded THIS turn (shouldn't be called then, though) */
    if (vultures_tiles_cur[tile_id])
        return vultures_tiles_cur[tile_id];

    /* if data_len is 0 the tile doesn't have a graphic */
    if (!vultures_gametiles[tile_id].data_len)
        return NULL;

    newtile = malloc(sizeof(vultures_tile));
    if (!newtile)
        return NULL;

    data = (char *)&vultures_srcfile[vultures_gametiles[real_id].file_offset];
    newtile->graphic = vultures_load_surface(data, vultures_gametiles[real_id].data_len);
    newtile->xmod = vultures_gametiles[real_id].hs_x;
    newtile->ymod = vultures_gametiles[real_id].hs_y;

    if (V_IS_WALL(real_id))
        vultures_set_tile_alpha(newtile, vultures_opts.wall_opacity);

    return newtile;
}



static inline vultures_tile * vultures_shade_tile(int tile_id)
{
    vultures_tile *tile, *orig;
    int real_id = tile_id % GAMETILECOUNT;
    SDL_Surface * blend = (tile_id/GAMETILECOUNT == 1) ? vultures_ftshade1 : vultures_ftshade2;

    if (vultures_gametiles[real_id].ptr == -1)
        return NULL;

    if (!vultures_tiles_cur[tile_id] && vultures_tiles_prev[tile_id])
        vultures_tiles_cur[tile_id] = vultures_tiles_prev[tile_id];

    if (!vultures_tiles_cur[tile_id])
    {
        orig = vultures_get_tile(real_id);

        tile = malloc(sizeof(vultures_tile));
        tile->xmod = orig->xmod;
        tile->ymod = orig->ymod;
        tile->graphic = vultures_get_img_src(0,0, orig->graphic->w-1, orig->graphic->h-1, orig->graphic);
        SDL_BlitSurface(blend, NULL, tile->graphic, NULL);

        vultures_tiles_cur[tile_id] = tile;
    }

    return vultures_tiles_cur[tile_id];
}



void vultures_flip_tile_arrays(void)
{
    int i;
    vultures_tile ** temp;

    for (i = 0; i < TILEARRAYLEN; i++)
    {
        if (vultures_tiles_prev[i] && !vultures_tiles_cur[i])
        {
            SDL_FreeSurface(vultures_tiles_prev[i]->graphic);
            free(vultures_tiles_prev[i]);
        }
    }

    temp = vultures_tiles_prev;
    vultures_tiles_prev = vultures_tiles_cur;
    vultures_tiles_cur = temp;
    memset(vultures_tiles_cur, 0, TILEARRAYLEN * sizeof(vultures_tile *));
}



int vultures_load_gametiles(void)
{
    int fsize;
    char * filename;
    FILE * fp;

    /* load gametiles.bin */
    filename = vultures_make_filename(V_GRAPHICS_DIRECTORY, NULL, "gametiles.bin");
    fp = fopen(filename, "rb");
    free(filename);

    if (!fp)
        panic("FATAL: cannot open gametiles.bin\n");

    /* obtain file size. */
    fseek(fp , 0 , SEEK_END);
    fsize = ftell(fp);
    rewind(fp);
    
    if (fsize != V_BINFILESIZE)
        panic("FATAL: Actual size of gametiles.bin does not match the expected size!\nAre you sure you are using the gametiles.bin that was built together with this executable?\n");

    vultures_srcfile = malloc(fsize);
    if (!vultures_srcfile)
        return 0;

    fread(vultures_srcfile, fsize, 1, fp);
    fclose(fp);

    /* initialize the two tile arrays */
    /* the extra 2*FLOORTILECOUNT elements will contain pre-shaded floor tiles */
    vultures_tiles_cur  = malloc(TILEARRAYLEN * sizeof(vultures_tile *));
    vultures_tiles_prev = malloc(TILEARRAYLEN * sizeof(vultures_tile *));
    if (!vultures_tiles_cur || !vultures_tiles_prev)
    {
        free(vultures_srcfile);
        return 0;
    }
    memset(vultures_tiles_cur, 0, TILEARRAYLEN * sizeof(vultures_tile *));
    memset(vultures_tiles_prev, 0, TILEARRAYLEN * sizeof(vultures_tile *));


    /* create the surfaces used to shade floor tiles */
    vultures_ftshade1 = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA, 112, 100, 32,
                                  vultures_px_format->Rmask, vultures_px_format->Gmask,
                                  vultures_px_format->Bmask, DEF_AMASK);
    vultures_ftshade2 = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA, 112, 100, 32,
                                  vultures_px_format->Rmask, vultures_px_format->Gmask,
                                  vultures_px_format->Bmask, DEF_AMASK);
    SDL_FillRect(vultures_ftshade1, NULL, CLR32_BLACK_A30);
    SDL_FillRect(vultures_ftshade2, NULL, CLR32_BLACK_A70);

    return 1;
}


void vultures_unload_gametiles(void)
{
    /* calling flip twice will unload all the tiles... */
    vultures_flip_tile_arrays();
    vultures_flip_tile_arrays();
    
    free(vultures_tiles_cur);
    free(vultures_tiles_prev);
    
    
    free(vultures_srcfile);
    
    SDL_FreeSurface(vultures_ftshade1);
    SDL_FreeSurface(vultures_ftshade2);    
}



static vultures_tile *vultures_make_alpha_player_tile(int monnum, double op_scale)
{
    static int tilenum = -1;
    static double lastscale = 0;
    vultures_tile *montile;
    vultures_tile *tile = NULL;
    int x, y;

    if (vultures_tiles_cur[V_TILE_PLAYER_INVIS])
        tile = vultures_tiles_cur[V_TILE_PLAYER_INVIS];
    else if (vultures_tiles_prev[V_TILE_PLAYER_INVIS])
        tile = vultures_tiles_prev[V_TILE_PLAYER_INVIS];

    /*
     * monnum may change if player polymorphs...
     */
    if (monnum != tilenum || lastscale != op_scale || !tile)
    {
        lastscale = op_scale;

        if (vultures_tiles_cur[V_TILE_PLAYER_INVIS] != NULL)
        {
            SDL_FreeSurface(vultures_tiles_cur[V_TILE_PLAYER_INVIS]->graphic);
            free(vultures_tiles_cur[V_TILE_PLAYER_INVIS]);
            vultures_tiles_cur[V_TILE_PLAYER_INVIS] = NULL;
        }

        montile = vultures_get_tile(MONSTER_TO_VTILE(monnum));
        tile = malloc(sizeof(*tile));
        if (tile == NULL)
            return montile;

        /* set tile->graphic to be an exact copy of montile->graphic */
        tile->graphic = vultures_get_img_src(0,0,montile->graphic->w-1, montile->graphic->h-1, montile->graphic);
        tile->xmod = montile->xmod;
        tile->ymod = montile->ymod;

        /* scale opacity of every pixel by op_scale. This works nicely, because
         * complete transparency has a numeric value of 0, so it will remain unchanged,
         * while all other pixels transparency will depend on their current transparency */
        unsigned char * rawdata = tile->graphic->pixels;
        for (y = 0; y < tile->graphic->h; y++)
            for (x = 0; x < tile->graphic->pitch; x += 4)
                rawdata[y*tile->graphic->pitch+x+3] *= op_scale;
        tilenum = monnum;
    }

    return tile;
}

static inline vultures_tile *vultures_set_tile_alpha(vultures_tile *tile, double opacity)
{
    unsigned char * rawdata;
    int x, y;

    rawdata = tile->graphic->pixels;
    for (y = 0; y < tile->graphic->h; y++)
        for (x = 0; x < tile->graphic->pitch; x += 4)
            rawdata[y*tile->graphic->pitch+x+3] *= opacity;

    return tile;
}



