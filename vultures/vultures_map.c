/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000 */
/* Copyright (c) Daniel Thaler, 2005 */

#include "vultures_win.h"
#include "vultures_map.h"
#include "vultures_gfl.h"
#include "vultures_gametiles.h"
#include "vultures_gra.h"
#include "vultures_sdl.h"
#include "vultures_main.h"
#include "vultures_tile.h"
#include "vultures_txt.h"
#include "vultures_opt.h"

#include "epri.h"

/* lookat isn't in the nethack headers, but we use it here ... */
extern struct permonst * lookat(int, int, char *, char *);
extern short glyph2tile[];

#define CMD_TRAVEL (char)0x90
#define META(c) (0x80 | (c))
#define CTRL(c) (0x1f & (c))


/* the only other cmaps beside furniture that have significant height and
 * therefore need to be drawn as part of the second pass with the furniture. */
#define V_EXTRA_FURNITURE(typ) ((typ) == DOOR || \
                                 (typ) == DBWALL || \
                                 (typ) == IRONBARS || \
                                 (typ) == TREE)


/* minimap tile types */
enum {
    V_MMTILE_NONE,
    V_MMTILE_FLOOR,
    V_MMTILE_STAIRS,
    V_MMTILE_DOOR,
    V_MMTILE_YOU,
    V_MMTILE_PET,
    V_MMTILE_MONSTER,
    V_MMTILE_MAX
};


/*----------------------------
 * global variables
 *---------------------------- */

int vultures_view_x, vultures_view_y;  /* Centre of displayed map area */

/* Map window contents, as Vulture's tile IDs */
static int ** vultures_map_glyph = NULL;                /* real glyph representation of map */
static int ** vultures_map_back = NULL;      /* background (floors, walls, pools, moats, ...) */
static int ** vultures_map_furniture = NULL; /* furniture (stairs, altars, fountains, ...) */
static int ** vultures_map_trap = NULL;      /* traps */
static int ** vultures_map_obj = NULL;       /* topmost object */
static int ** vultures_map_specialeff = NULL;   /* special effects: zap, engulf, explode */
static int ** vultures_map_mon = NULL;       /* monster tile ID */
static unsigned int ** vultures_map_specialattr = NULL; /* special attributes, we use them to highlight the pet */
static unsigned char ** vultures_map_deco = NULL;     /* positions of murals and carpets */
static int vultures_map_swallow = V_TILE_NONE; /* the engulf tile, if any */

static SDL_Surface * vultures_map_symbols[V_MAX_MAP_SYMBOLS]; /* Map parchment symbols */

static char ** vultures_minimap_syms = NULL;


static int * vultures_tilemap_special;
static int * vultures_tilemap_engulf;

/* pointer to full height, half height or transparent walltile array */
struct walls * walltiles = NULL;

/* Map window contents, as Vulture's tiles */
static struct walls ** vultures_maptile_wall = NULL; /* Custom (combination) wall style for each square */
static struct fedges ** vultures_maptile_floor_edge = NULL; /* Custom floor edge style for each square */

static char ** vultures_map_tile_is_dark = NULL;
static char ** vultures_room_indices = NULL;           /* packed room numbers and deco ids */


static int vultures_map_clip_tl_x = 999999;
static int vultures_map_clip_tl_y = 999999;
static int vultures_map_clip_br_x = 0;
static int vultures_map_clip_br_y = 0;

int vultures_map_draw_msecs = 0;
int vultures_map_draw_lastmove = 0;
point vultures_map_highlight = {-1, -1};

/*----------------------------
 * pre-declared functions
 *---------------------------- */
static void vultures_init_floor_decors(int num_decors);
static int vultures_get_floor_tile(int tile, int y, int x);
static void vultures_get_floor_edges(int y, int x);
static int vultures_get_room_index(int x, int y);
static int vultures_get_wall_decor(int floortype, int wally, int wallx, int floory, int floorx);
static int vultures_get_floor_decor(int floorstyle, int floory, int floorx);
static void vultures_clear_floor_edges(int y, int x);
static void vultures_get_wall_tiles(int y, int x);
static int vultures_monster_to_tile(int mon_id, XCHAR_P x, XCHAR_P y);
static char vultures_mappos_to_dirkey(point mappos);
static void vultures_clear_walls(int y, int x);
static void vultures_build_tilemap(void);
static int vultures_remap_tile_id(int tile_id);



/*----------------------------
 * function implementations
 *---------------------------- */

/* Ordering of functions:
 * 1) initializer and destructor
 *    - vultures_init_map
 *    - vultures_destroy_map
 *
 * 2) drawing
 *    - vultures_draw_level
 *    - vultures_draw_map
 *    - vultures_draw_minimap
 *    - vultures_map_force_redraw
 *
 * 3) setting map data
 *    - vultures_clear_map
 *    - vultures_print_glyph
 *    - vultures_clear_walls
 *    - vultures_clear_floor_edges
 *
 * 4) Computing map data
 *    - vultures_init_floor_decors
 *    - vultures_get_wall_tiles
 *    - vultures_get_floor_tile
 *    - vultures_get_floor_edges
 *    - vultures_get_room_index
 *    - vultures_get_wall_decor
 *    - vultures_get_floor_decor
 *
 * 5) Interface functions for windowing
 *    - vultures_map_square_description
 *    - vultures_mouse_to_map
 *    - vultures_map_to_mouse
 *    - vultures_get_map_action
 *    - vultures_get_map_contextmenu
 *    - vultures_perform_map_action
 *    - vultures_get_map_cursor
 *
 * 6) misc utility functions
 *    - vultures_object_to_tile
 *    - vultures_monster_to_tile
 *    - vultures_mappos_to_dirkey
 *    - vultures_build_tilemap
 *    - vultures_remap_tile_id
 */



/*****************************************************************************
 * 1) Initializer and destructor
 *****************************************************************************/

int vultures_init_map(void)
{
    int i, j;
    SDL_Surface * image;

    /* select wall style */
    switch (vultures_opts.wall_style)
    {
        case V_WALL_DISPLAY_STYLE_FULL:
            walltiles = (struct walls*)walls_full; break;

        case V_WALL_DISPLAY_STYLE_HALF_HEIGHT:
            walltiles = (struct walls*)walls_half; break;

        default:
            walltiles = (struct walls*)walls_half; break;
    }

    /* Initialize map */
    vultures_map_glyph       = malloc(V_MAP_HEIGHT * sizeof(int *));
    vultures_map_back        = malloc(V_MAP_HEIGHT * sizeof(int *));
    vultures_map_obj         = malloc(V_MAP_HEIGHT * sizeof(int *));
    vultures_map_trap        = malloc(V_MAP_HEIGHT * sizeof(int *));
    vultures_map_furniture   = malloc(V_MAP_HEIGHT * sizeof(int *));
    vultures_map_specialeff  = malloc(V_MAP_HEIGHT * sizeof(int *));
    vultures_map_mon         = malloc(V_MAP_HEIGHT * sizeof(int *));
    vultures_map_specialattr = malloc(V_MAP_HEIGHT * sizeof(unsigned int**));
    vultures_map_deco        = malloc(V_MAP_HEIGHT * sizeof(unsigned char *));

    vultures_maptile_wall       = malloc(V_MAP_HEIGHT*sizeof(struct walls *));
    vultures_maptile_floor_edge = malloc(V_MAP_HEIGHT*sizeof(struct fedges *));

    vultures_map_tile_is_dark = malloc(V_MAP_HEIGHT*sizeof(char *));
    vultures_room_indices     = malloc(V_MAP_HEIGHT*sizeof(char *));

    vultures_minimap_syms     = malloc(V_MAP_HEIGHT*sizeof(char *));

    if ((!vultures_map_back) || (!vultures_map_obj) || (!vultures_map_trap) || (!vultures_map_glyph) ||
        (!vultures_map_furniture) || (!vultures_map_mon) || (!vultures_map_specialeff) ||
        (!vultures_maptile_wall) || (!vultures_maptile_floor_edge) || (!vultures_map_deco) ||
        (!vultures_map_specialattr) || (!vultures_room_indices))
        vultures_bail("Out of memory");

    for (i = 0; i < V_MAP_HEIGHT; i++)
    {
        vultures_map_glyph[i]       = malloc(V_MAP_WIDTH * sizeof(int));
        vultures_map_back[i]        = malloc(V_MAP_WIDTH * sizeof(int));
        vultures_map_obj[i]         = malloc(V_MAP_WIDTH * sizeof(int));
        vultures_map_trap[i]        = malloc(V_MAP_WIDTH * sizeof(int));
        vultures_map_furniture[i]   = malloc(V_MAP_WIDTH * sizeof(int));
        vultures_map_specialeff[i]  = malloc(V_MAP_WIDTH * sizeof(int));
        vultures_map_mon[i]         = malloc(V_MAP_WIDTH * sizeof(int));
        vultures_map_specialattr[i] = malloc(V_MAP_WIDTH * sizeof(unsigned int));
        vultures_map_deco[i]        = malloc(V_MAP_WIDTH * sizeof(unsigned char));

        vultures_maptile_wall[i]       = malloc(V_MAP_WIDTH*sizeof(struct walls));
        vultures_maptile_floor_edge[i] = malloc(V_MAP_WIDTH*sizeof(struct fedges));

        vultures_map_tile_is_dark[i] = malloc(V_MAP_WIDTH*sizeof(char));
        vultures_room_indices[i] = malloc(V_MAP_WIDTH*sizeof(char));

        vultures_minimap_syms[i] = malloc(V_MAP_WIDTH*sizeof(char));

        if ((!vultures_map_back[i]) || (!vultures_map_obj[i]) || (!vultures_map_trap[i]) ||
            (!vultures_map_furniture[i]) || (!vultures_map_mon[i]) || (!vultures_map_specialeff[i]) ||
            (!vultures_maptile_wall[i]) || (!vultures_maptile_floor_edge[i]) || (!vultures_map_deco[i]) ||
            (!vultures_map_specialattr[i]) || (!vultures_room_indices[i]))
            vultures_bail("Out of memory");

        for (j = 0; j < V_MAP_WIDTH; j++)
        {
            vultures_map_glyph[i][j] = NO_GLYPH;  
            vultures_map_back[i][j] = V_TILE_UNMAPPED_AREA;
            vultures_map_obj[i][j] = V_TILE_NONE;
            vultures_map_trap[i][j] = V_TILE_NONE;
            vultures_map_furniture[i][j] = V_TILE_NONE;
            vultures_map_specialeff[i][j] = V_TILE_NONE;
            vultures_map_mon[i][j] = V_TILE_NONE;
            vultures_map_specialattr[i][j] = 0;
            vultures_map_deco[i][j] = 0;
            vultures_room_indices[i][j] = 0;
            vultures_map_tile_is_dark[i][j] = 2;
            vultures_clear_walls(i,j);

            vultures_minimap_syms[i][j] = 0;
        }
    }

    vultures_view_x = V_MAP_WIDTH/2;
    vultures_view_y = V_MAP_HEIGHT/2;


    /* Load map symbols */
    image = vultures_load_graphic(NULL, V_FILENAME_MAP_SYMBOLS);
    if (image == NULL)
        return 0;
    else
    {
        for (i = 0; i < V_MAX_MAP_SYMBOLS; i++)
            vultures_map_symbols[i] = vultures_get_img_src(
                (i%40)*VULTURES_MAP_SYMBOL_WIDTH,
                (i/40)*VULTURES_MAP_SYMBOL_HEIGHT,
                (i%40)*VULTURES_MAP_SYMBOL_WIDTH + (VULTURES_MAP_SYMBOL_WIDTH - 1),
                (i/40)*VULTURES_MAP_SYMBOL_HEIGHT+ (VULTURES_MAP_SYMBOL_HEIGHT - 1),
                image);
        SDL_FreeSurface(image);
    }

    /* build vultures_tilemap_special and vultures_tilemap_engulf */
    vultures_build_tilemap();

    return 1;
}



void vultures_destroy_map(void)
{
    int i;

    for (i = 0; i < V_MAP_HEIGHT; i++)
    {
        free (vultures_map_glyph[i]);
        free (vultures_map_back[i]);
        free (vultures_map_obj[i]);
        free (vultures_map_trap[i]);
        free (vultures_map_furniture[i]);
        free (vultures_map_specialeff[i]);
        free (vultures_map_mon[i]);
        free (vultures_map_specialattr[i]);
        free (vultures_map_deco[i]);
        free (vultures_maptile_wall[i]);
        free (vultures_maptile_floor_edge[i]);
        free (vultures_map_tile_is_dark[i]);
        free (vultures_room_indices[i]);

        free(vultures_minimap_syms[i]);
    }

    free (vultures_map_glyph);
    free (vultures_map_back);
    free (vultures_map_obj);
    free (vultures_map_trap);
    free (vultures_map_furniture);
    free (vultures_map_specialeff);
    free (vultures_map_mon);
    free (vultures_map_specialattr);
    free (vultures_map_deco);
    free (vultures_maptile_wall);
    free (vultures_maptile_floor_edge);
    free (vultures_map_tile_is_dark);
    free (vultures_room_indices);
    free (vultures_minimap_syms);

    /* free the map symbols */
    for (i = 0; i < V_MAX_MAP_SYMBOLS; i++)
        SDL_FreeSurface(vultures_map_symbols[i]);

    /* free nethack to vultures translation tables */
    free (vultures_tilemap_special);
    free (vultures_tilemap_engulf);

}


/*****************************************************************************
 * 2) Drawing
 *****************************************************************************/

int vultures_draw_level(struct window * win)
{
    int i, j, dir_idx;
    vultures_tile * cur_tile;
    int x, y;
    int tile_id;
    int map_tr_x, map_tr_y, __i, __j, diff, sum;
    int map_centre_x = win->w / 2;
    int map_centre_y = win->h / 2;
    
    unsigned long startticks = SDL_GetTicks();

    static int cur_dlevel = -1;
    static int prev_cx = -1;
    static int prev_cy = -1;

    if (cur_dlevel != u.uz.dlevel)
    {
        vultures_init_floor_decors(10);
        cur_dlevel = u.uz.dlevel;
    }


    if (prev_cx != vultures_view_x ||
        prev_cy != vultures_view_y ||
        vultures_map_swallow != V_TILE_NONE)
    {
        vultures_map_clip_tl_x = win->abs_x;
        vultures_map_clip_tl_y = win->abs_y;
        vultures_map_clip_br_x = win->abs_x + win->w;
        vultures_map_clip_br_y = win->abs_y + win->h;
    }
    else
    {
        vultures_map_clip_tl_x = vultures_map_clip_tl_x >= 0 ? vultures_map_clip_tl_x : 0;
        vultures_map_clip_tl_y = vultures_map_clip_tl_y >= 0 ? vultures_map_clip_tl_y : 0;
        vultures_map_clip_br_x = (vultures_map_clip_br_x < win->abs_x + win->w) ?
                                  vultures_map_clip_br_x : win->abs_x + win->w - 1;
        vultures_map_clip_br_y = (vultures_map_clip_br_y < win->abs_y + win->h) ?
                                  vultures_map_clip_br_y : win->abs_y + win->h - 1;
    }

    if (vultures_map_clip_tl_x >= win->w + win->abs_x || vultures_map_clip_tl_y >= win->h + win->abs_y)
        /* nothing changed onscreen */
        return 1;

    prev_cx = vultures_view_x;
    prev_cy = vultures_view_y;

    /* Only draw on map area */
    vultures_set_draw_region(vultures_map_clip_tl_x, vultures_map_clip_tl_y,
                             vultures_map_clip_br_x, vultures_map_clip_br_y);

    /* If swallowed draw ONLY the engulf tile and the player! */
    if (u.uswallow && vultures_map_swallow != V_TILE_NONE)
    {
        /* Clear map area */
        SDL_FillRect(vultures_screen, NULL, CLR32_BLACK);

        x = map_centre_x + V_MAP_XMOD*(u.ux - u.uy + vultures_view_y - vultures_view_x);
        y = map_centre_y + V_MAP_YMOD*(u.ux + u.uy - vultures_view_y - vultures_view_x);

        /* engulf tile */
        vultures_put_tile(x, y, vultures_map_swallow);

        /* player */
        vultures_put_tile(x, y, vultures_map_mon[u.uy][u.ux]);

        vultures_invalidate_region(vultures_map_clip_tl_x, vultures_map_clip_tl_y,
                                   vultures_map_clip_br_x - vultures_map_clip_tl_x,
                                   vultures_map_clip_br_y - vultures_map_clip_tl_y);
        return 1;
    }
    else
        vultures_map_swallow = V_TILE_NONE;


    /* coords of the top right corner */
    map_tr_x = (-V_MAP_YMOD * (map_centre_x + 50) + V_MAP_XMOD * (map_centre_y + 50) +
                    V_MAP_XMOD * V_MAP_YMOD) / (2 * V_MAP_XMOD * V_MAP_YMOD);
    map_tr_y =  (V_MAP_YMOD * (map_centre_x + 50) + V_MAP_XMOD * (map_centre_y + 50) +
                    V_MAP_XMOD * V_MAP_YMOD) / (2 * V_MAP_XMOD * V_MAP_YMOD);

    /* without the +-1 small corners within the viewport are not drawn */
    diff = map_tr_x - map_tr_y - 1;
    sum  = map_tr_x + map_tr_y + 1;

    for (__i = - map_tr_y; __i <= map_tr_y; __i++)
    {
        i = vultures_view_y + __i;

        for (__j = diff + __i; __j + __i <= sum; __j++)
        {
            j = vultures_view_x + __j;

            x = map_centre_x + V_MAP_XMOD*(__j - __i);
            y = map_centre_y + V_MAP_YMOD*(__j + __i);

            if (j < 1 || j >= V_MAP_WIDTH || i < 0 || i >= V_MAP_HEIGHT)
            {
                vultures_put_tile(x, y, V_TILE_OFF_MAP);
                continue;
            }

            /* 
               Draw Vulture's tiles, in order:
               pass 1
                 1. Floor
                 2. Floor edges

               pass 2
                 1. North & west walls
                 2. Furniture
                 3. Traps
                 4. Objects
                 5. Monsters
                 6. Effects
                 7. South & east walls
             */

            /* 0. init walls and floor edges for this tile*/
            if (vultures_map_back[i][j] == V_TILE_WALL_GENERIC ||
                vultures_map_back[i][j] == V_TILE_UNMAPPED_AREA)
                vultures_get_wall_tiles(i, j);
            else
                /* certain events (amnesia or a secret door being discovered) 
                 * require us to clear walls again. since we cannot check for those cases
                 * we simply clear walls whenever we don't set any... */
                vultures_clear_walls(i, j);

            if (vultures_map_back[i][j] == V_TILE_FLOOR_WATER ||
                vultures_map_back[i][j] == V_TILE_FLOOR_ICE ||
                vultures_map_back[i][j] == V_TILE_FLOOR_LAVA ||
                vultures_map_back[i][j] == V_TILE_FLOOR_AIR)
                /* these tiles get edges */
                vultures_get_floor_edges(i, j);
            else
                /* everything else doesn't. However we can't just assume a tile that doesn't need egdes doesn't have any:
                 * pools may be dried out by fireballs, and suddenly we have a pit with edges :(
                 * Therefore we always clear them explicitly */
                vultures_clear_floor_edges(i ,j);


            /* 2. Floor */
            tile_id = vultures_map_back[i][j];

            if ((tile_id >= V_TILE_FLOOR_COBBLESTONE) &&
                (tile_id <= V_TILE_FLOOR_DARK))
                tile_id = vultures_get_floor_tile(tile_id, i, j) + 
                          vultures_map_tile_is_dark[i][j] * GAMETILECOUNT;
            else if (tile_id == V_TILE_WALL_GENERIC)
                tile_id = V_TILE_UNMAPPED_AREA;

            vultures_put_tile(x, y, tile_id);

            /* shortcut for unmapped case */
            if (tile_id == V_TILE_UNMAPPED_AREA)
                continue;

            if (vultures_opts.highlight_cursor_square && 
                (j == vultures_map_highlight.x && i == vultures_map_highlight.y))
                vultures_put_tile(x, y, V_TILE_FLOOR_HIGHLIGHT);

            /* 3. Floor edges */
            for (dir_idx = 0; dir_idx < 12; dir_idx++)
                vultures_put_tile(x, y, vultures_maptile_floor_edge[i][j].dir[dir_idx]);
        }
    }

    for (__i = - map_tr_y; __i <= map_tr_y; __i++)
    {
        i = vultures_view_y + __i;
        if (i < 0 || i >= V_MAP_HEIGHT)
            continue;

        for (__j = diff + __i; __j + __i <= sum; __j++)
        {
            j = vultures_view_x + __j;
            if (j < 1 || j >= V_MAP_WIDTH)
                continue;
            /* Find position of tile centre */
            x = map_centre_x + V_MAP_XMOD*(__j - __i);
            y = map_centre_y + V_MAP_YMOD*(__j + __i);


            /* 1. West and north walls */
            if (j > 1)
                vultures_put_tile(x, y, vultures_maptile_wall[i][j].west +
                                  vultures_map_tile_is_dark[i][j-1] * GAMETILECOUNT);
            if (i > 1)
                vultures_put_tile(x, y, vultures_maptile_wall[i][j].north +
                                  vultures_map_tile_is_dark[i-1][j] * GAMETILECOUNT);

            /* shortcut for unmapped case */
            if (vultures_map_back[i][j] != V_TILE_UNMAPPED_AREA ||
                vultures_map_obj[i][j] != V_TILE_NONE)
            {
                /* 2. Furniture*/
                vultures_put_tile(x, y, vultures_map_furniture[i][j]);


                /* 3. Traps */
                vultures_put_tile(x, y, vultures_map_trap[i][j]);


                /* 4. Objects */
                vultures_put_tile(x, y, vultures_map_obj[i][j]);


                /* 5. Monsters */
                if ((cur_tile = vultures_get_tile(vultures_map_mon[i][j])) != NULL)
                {
                    vultures_put_tile(x, y, vultures_map_mon[i][j]);
                    if (iflags.hilite_pet && (vultures_map_specialattr[i][j] & MG_PET))
                        vultures_put_img(x + cur_tile->xmod, y + cur_tile->ymod - 10,
                                    vultures_get_tile(V_TILE_HILITE_PET)->graphic);
                }

                /* 6. Effects */
                vultures_put_tile(x, y, vultures_map_specialeff[i][j]);
            }

            /* 7. South & East walls */
            if (i < V_MAP_HEIGHT - 1)
                vultures_put_tile(x, y, vultures_maptile_wall[i][j].south +
                                  vultures_map_tile_is_dark[i+1][j] * GAMETILECOUNT);
            if (j < V_MAP_WIDTH - 1)
                vultures_put_tile(x, y, vultures_maptile_wall[i][j].east +
                                  vultures_map_tile_is_dark[i][j+1] * GAMETILECOUNT);
        }
    }


    /* Restore drawing region */
    vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);

    vultures_invalidate_region(vultures_map_clip_tl_x, vultures_map_clip_tl_y,
                               vultures_map_clip_br_x - vultures_map_clip_tl_x,
                               vultures_map_clip_br_y - vultures_map_clip_tl_y);

    vultures_map_clip_tl_x = 999999;
    vultures_map_clip_tl_y = 999999;
    vultures_map_clip_br_x = 0;
    vultures_map_clip_br_y = 0;

    vultures_flip_tile_arrays();

    vultures_map_draw_msecs = SDL_GetTicks() - startticks;
    vultures_map_draw_lastmove = moves;

    return 1;
}



int vultures_draw_map(struct window * win)
{
    int map_x, map_y;
    int i, j;
    struct window * txt = win->first_child;

    /* Draw parchment */
    vultures_put_img(win->abs_x, win->abs_y, win->pd.image);

    /* Draw the level name */
#ifdef VULTURESCLAW
    describe_level(txt->caption, TRUE);
#else
    if (!describe_level(txt->caption))
        sprintf(txt->caption, "%s, level %d ", dungeons[u.uz.dnum].dname, depth(&u.uz));
#endif
    txt->x = (win->w - vultures_text_length(V_FONT_HEADLINE, txt->caption)) / 2;
    txt->abs_x = win->abs_x + txt->x;
    txt->abs_y = win->abs_y + txt->y;
    vultures_put_text_shadow(V_FONT_HEADLINE, txt->caption, vultures_screen, txt->abs_x, txt->abs_y, CLR32_BROWN, CLR32_BLACK_A50);

    /* Find upper left corner of map on parchment */
    map_x = win->abs_x + 39;
    map_y = win->abs_y + 91;

    /* Draw map on parchment, and create hotspots */
    for (i = 0; i < V_MAP_HEIGHT; i++)
        for (j = 1; j < V_MAP_WIDTH; j++)
        {
            if (vultures_map_glyph[i][j] != NO_GLYPH &&
                (vultures_map_glyph[i][j] != cmap_to_glyph(S_stone) ||
                (level.locations[j][i].seenv && vultures_map_tile_is_dark[i][j] == 2))) {
                vultures_put_img(
                        map_x+VULTURES_MAP_SYMBOL_WIDTH*j,
                        map_y+VULTURES_MAP_SYMBOL_HEIGHT*i,
                        vultures_map_symbols[glyph2tile[vultures_map_glyph[i][j]]]); 
            }
        }

    vultures_invalidate_region(win->abs_x, win->abs_y, win->w, win->h);

    return 0;
}



int vultures_draw_minimap(struct window * win)
{
    int map_x, map_y, sym;
    Uint32 * pixels;
    SDL_Rect destrect = {0, 0, 3, 2};

    Uint32 minimap_colors[V_MMTILE_MAX] = {CLR32_BLACK, V_COLOR_MINI_FLOOR, V_COLOR_MINI_STAIRS,
                                           V_COLOR_MINI_DOOR, V_COLOR_MINI_YOU, CLR32_GREEN, CLR32_RED};
    
    if (!win->pd.image)
        return 0;
        
    if (win->background)
        vultures_put_img(win->abs_x, win->abs_y, win->background);


    for (map_y = 0; map_y < V_MAP_HEIGHT; map_y++)
    {
        for (map_x = 1; map_x < V_MAP_WIDTH; map_x++)
        {
            /* translate the contents of the map into a minimap symbol color */
            switch(vultures_map_back[map_y][map_x])
            {
                case V_TILE_WALL_GENERIC:
                case V_TILE_UNMAPPED_AREA:
                    sym = V_MMTILE_NONE; break;

                default:
                    sym = V_MMTILE_FLOOR; break;
            }

            switch(vultures_map_furniture[map_y][map_x])
            {
                case V_TILE_STAIRS_UP:
                case V_TILE_STAIRS_DOWN:
                    sym = V_MMTILE_STAIRS; break;

                case V_TILE_VDOOR_WOOD_OPEN:
                case V_TILE_VDOOR_WOOD_CLOSED:
                case V_TILE_HDOOR_WOOD_OPEN:
                case V_TILE_HDOOR_WOOD_CLOSED:
                case V_TILE_VODBRIDGE:
                case V_TILE_HODBRIDGE:
                case V_TILE_VCDBRIDGE:
                case V_TILE_HCDBRIDGE:
                    sym = V_MMTILE_DOOR; break;
            }

            if (vultures_map_trap[map_y][map_x] == V_TILE_MAGIC_PORTAL)
                sym = V_MMTILE_STAIRS;

            if (vultures_map_mon[map_y][map_x] != V_TILE_NONE)
            {
                if (vultures_map_specialattr[map_y][map_x] & MG_PET)
                    sym = V_MMTILE_PET;
                else
                    sym = V_MMTILE_MONSTER;
            }

            if ((map_y == u.uy) && (map_x == u.ux))
                sym = V_MMTILE_YOU;

            /* draw symbols that changed onto the "minimap surface" */
            if (sym != vultures_minimap_syms[map_y][map_x])
            {
                vultures_minimap_syms[map_y][map_x] = sym;

                destrect.x = 40 + 2*map_x - 2*map_y;
                destrect.y = map_x + map_y;

                pixels = win->pd.image->pixels + 
                         win->pd.image->pitch * (destrect.y+6) + (destrect.x+6) * 4;

                /* A minimap symbol has this shape: _ C _
                 *                                  C C C
                 * where "_" is transparent and "C" is a colored pixel */

                /* row 1: */
                /* pixels[0] = transparent -> dont write */
                pixels[1] = minimap_colors[sym];
                 /* pixels[2] = transparent -> dont write */

                /* row 2 */
                pixels = win->pd.image->pixels + 
                         win->pd.image->pitch * (destrect.y+7) + (destrect.x+6) * 4;
                pixels[0] = minimap_colors[sym];
                pixels[1] = minimap_colors[sym];
                pixels[2] = minimap_colors[sym];
            }

        }
    }

    vultures_put_img(win->abs_x, win->abs_y, win->pd.image);

    vultures_invalidate_region(win->abs_x, win->abs_y, win->w, win->h);

    return 0;
}



void vultures_map_force_redraw(void)
{
    struct window * win = vultures_get_window(0);
    win->need_redraw = 1;

    vultures_map_clip_tl_x = 0;
    vultures_map_clip_tl_y = 0;
    vultures_map_clip_br_x = win->w;
    vultures_map_clip_br_y = win->h;
}


/*****************************************************************************
 * 3) Setting map data
 *****************************************************************************/

void vultures_clear_map()
{
    int i, j;

    for (i = 0; i < V_MAP_HEIGHT; i++)
        for (j = 0; j < V_MAP_WIDTH; j++)
        {
            vultures_map_tile_is_dark[i][j] = 2;
            /* ideally this is what we'd do to clear background:
            * vultures_map_back[i][j] = V_TILE_UNMAPPED_AREA;
            * unfortunately doing so breaks dark tiles in rooms... */
            vultures_print_glyph(0, j, i, cmap_to_glyph(S_stone));
            vultures_map_trap[i][j] = V_TILE_NONE;
            vultures_map_furniture[i][j] = V_TILE_NONE;
            vultures_map_obj[i][j] = V_TILE_NONE;
            vultures_map_mon[i][j] = V_TILE_NONE;
            vultures_map_specialeff[i][j] = V_TILE_NONE;
            vultures_map_specialattr[i][j] = 0;
            vultures_map_glyph[i][j] = cmap_to_glyph(S_stone);

            vultures_clear_floor_edges(i, j);
            vultures_clear_walls(i, j);
        }
}


void vultures_add_to_clipregion(int tl_x, int tl_y, int br_x, int br_y)
{
    vultures_map_clip_tl_x = (vultures_map_clip_tl_x < tl_x) ? vultures_map_clip_tl_x : tl_x;
    vultures_map_clip_tl_y = (vultures_map_clip_tl_y < tl_y) ? vultures_map_clip_tl_y : tl_y;
    vultures_map_clip_br_x = (vultures_map_clip_br_x > br_x) ? vultures_map_clip_br_x : br_x;
    vultures_map_clip_br_y = (vultures_map_clip_br_y > br_y) ? vultures_map_clip_br_y : br_y;
}


static void vultures_set_map_data(int ** data_array, int x, int y, int newval, int force)
{
    int pixel_x, pixel_y;
    int tl_x, tl_y, br_x, br_y;
    int oldval = vultures_remap_tile_id(data_array[y][x]);
    int tmp_newval = vultures_remap_tile_id(newval);
    struct window * map = vultures_get_window(0);

    if (data_array[y][x] != newval || force)
    {
        data_array[y][x] = newval;

        pixel_x = (map->w / 2) + V_MAP_XMOD*(x - y + vultures_view_y - vultures_view_x);
        pixel_y = (map->h / 2) + V_MAP_YMOD*(x + y - vultures_view_y - vultures_view_x);

        if (data_array != vultures_map_back)
        {
            tl_x = (vultures_gametiles[tmp_newval].hs_x < vultures_gametiles[oldval].hs_x) ? 
                    vultures_gametiles[tmp_newval].hs_x : vultures_gametiles[oldval].hs_x;
            tl_x += pixel_x;
            tl_y = (vultures_gametiles[tmp_newval].hs_y < vultures_gametiles[oldval].hs_y) ? 
                    vultures_gametiles[tmp_newval].hs_y : vultures_gametiles[oldval].hs_y;
            tl_y += pixel_y;
            br_x = pixel_x + V_MAX_TILE_XOFFS;
            br_y = pixel_y + V_MAX_TILE_YOFFS;

            if (data_array == vultures_map_mon)
                /* allow for the heart icon on pets */
                tl_y -= 10;
        }
        else
        {
            /* floor tiles tend to be placeholders until we reach draw_level, so we do this manually */
            tl_x = pixel_x - 56;
            tl_y = pixel_y - 100; /* 100 pixels accounts for possible walls, too */
            br_x = pixel_x + 56;
            br_y = pixel_y + 22;
        }

        vultures_add_to_clipregion(tl_x, tl_y, br_x, br_y);
    }
}



void vultures_print_glyph(winid window, XCHAR_P x, XCHAR_P y, int glyph)
{
    struct obj  *obj;
    struct trap *trap;
    int character, colour;
    int memglyph;

    /* if we're blind, we also need to print what we remember under us */
    if (!cansee(x,y) && x == u.ux && y == u.uy)
    {
#ifdef DISPLAY_LAYERS
        memglyph = memory_glyph(x,y);
#else
        memglyph = level.locations[x][y].glyph;
#endif
        if (glyph != memglyph)
            vultures_print_glyph(window, x, y, memglyph);
    }

    int map_mon = vultures_map_mon[y][x];
    int map_obj = vultures_map_obj[y][x];
    int map_trap = vultures_map_trap[y][x];
    int map_back = vultures_map_back[y][x];
    int map_special = V_TILE_NONE;
    int map_furniture = vultures_map_furniture[y][x];
    int darkness = vultures_map_tile_is_dark[y][x];

    vultures_map_glyph[y][x] = glyph;

    /* check wether we are swallowed, if so, return, because nothing but the swallow graphic will be drawn anyway */
    if (glyph_is_swallow(glyph))
    {
        /* we only SET vultures_map_swallow here; we expect vultures_draw_level to reset it */
        vultures_map_swallow = vultures_tilemap_engulf[(glyph-GLYPH_SWALLOW_OFF) >> 3];
        return;
    }


    /* Nethack will only show us one glyph per position. We need up to 4 "things" per mapsquare:
     * monsters, objects, traps & furniture, floor & walls.
     * Therefore we rely on the glyph only for monsters (which are never covered) and magical vision
     * where we can't just display eg the object a telepathically seen monster is standing on,
     * because that would be cheating. */
    if (cansee(x,y))
    {
        /* various special effects, these occur only during the player's turn and should be
         * layered on top of everything else */
        if (glyph >= GLYPH_ZAP_OFF && glyph < GLYPH_WARNING_OFF)
            map_special = vultures_tilemap_special[S_vbeam + ((glyph - GLYPH_ZAP_OFF) & 0x03)];
        else if (glyph >= GLYPH_EXPLODE_OFF && glyph < GLYPH_ZAP_OFF)
            map_special = EXPTILEOFFSET + (glyph - GLYPH_EXPLODE_OFF);
        else if (glyph_to_cmap(glyph) >= S_digbeam && glyph_to_cmap(glyph) <= S_ss4)
            /* digbeam, camera flash, boomerang, magic resistance: these are not floor tiles ... */
            map_special = vultures_tilemap_special[glyph_to_cmap(glyph)];

        /* if the player is invisible and can't see himself, nethack does not print a
         * glyph for him; we need to insert it ourselves */
        if (x == u.ux && y == u.uy && !canseeself())
        {
            vultures_map_glyph[y][x] = monnum_to_glyph(u.umonnum);
            map_mon = V_TILE_PLAYER_INVIS;
        }
        /* We rely on the glyph for monsters, as they are never covered by anything
         * at the start of the turn and dealing with them manually is ugly */
        else if (glyph_is_monster(glyph))
        {
            /* we need special attributes, so that we can highlight the pet */
            mapglyph(glyph, &character, &colour, &vultures_map_specialattr[y][x], x, y);

            map_mon =  vultures_monster_to_tile(glyph_to_mon(glyph), x, y);
        }
        /* handle invisible monsters */
        else if (glyph_is_invisible(glyph))
        {
            mapglyph(glyph, &character, &colour, &vultures_map_specialattr[y][x], x, y);
            map_mon = V_TILE_INVISIBLE_MONSTER;
        }
        /* handle monsters you are warned of */
        else if (glyph_is_warning(glyph))
        {
            map_mon = V_TILE_WARNLEV_1 + glyph_to_warning(glyph);
            mapglyph(glyph, &character, &colour, &vultures_map_specialattr[y][x], x, y);
        }
        /* however they may be temporarily obscured by magic effects... */
        else if (map_special == V_TILE_NONE)
            map_mon = V_TILE_NONE;

        /* visible objects that are not covered by lava */
        if ((obj = vobj_at(x,y)) && !covers_objects(x,y))
        {
            /* glyph_to_obj(obj_to_glyph(foo)) looks like nonsense, but is actually an elegant
             * way of handling hallucination, especially the complicated matter of hallucinated
             * corpses... */
            map_obj = vultures_object_to_tile(glyph_to_obj(obj_to_glyph( obj)), x, y);
        }
        /* just to make things interesting, the above does not handle thrown/kicked objects... */
        else if (glyph_is_object(glyph))
        {
            map_obj = vultures_object_to_tile(glyph_to_obj(glyph), x, y);
        }
        else
            map_obj = V_TILE_NONE;

        /* traps that are not covered by lava and have been seen */
        if ((trap = t_at(x,y)) && !covers_traps(x,y) && trap->tseen)
        {
            /* what_trap handles hallucination */
            map_trap = vultures_tilemap_special[what_trap(trap->ttyp) + S_arrow_trap - 1];
        }
        else
            map_trap = V_TILE_NONE;

        /* handle furniture: altars, stairs,...
         * furniture is separated out from walls and floors, so that it can be used on any
         * type of floor, rather than producing a discolored blob*/
        if (glyph_to_cmap(glyph) >= S_upstair && glyph_to_cmap(glyph) <= S_fountain)
        {
            /* this is a mimic pretending to be part of the dungeon */
            map_furniture = vultures_tilemap_special[glyph_to_cmap(glyph)];
            map_back = V_TILE_FLOOR_COBBLESTONE;
        }
        else if (IS_FURNITURE(level.locations[x][y].typ))
        {
            map_furniture = vultures_tilemap_special[glyph_to_cmap(back_to_glyph(x,y))];
            /* furniture is only found in rooms, so set the background type */
            map_back = V_TILE_FLOOR_COBBLESTONE;
        }
        else if (glyph_to_cmap(glyph) >= S_ndoor && glyph_to_cmap(glyph) <= S_tree)
        {
            /* this is a mimic pretending to be part of the dungeon */
            map_furniture = vultures_tilemap_special[glyph_to_cmap(glyph)];
            map_back = V_TILE_FLOOR_ROUGH;
        }
        else if (V_EXTRA_FURNITURE(level.locations[x][y].typ))
        {
            /* this stuff is not furniture, but we need to draw it at the same time,
             * so we pack it in with the furniture ...*/
            map_furniture = vultures_tilemap_special[glyph_to_cmap(back_to_glyph(x,y))];
            map_back = V_TILE_FLOOR_ROUGH;
        }
        else
        {
            map_furniture = V_TILE_NONE;
            map_back = vultures_tilemap_special[glyph_to_cmap(back_to_glyph(x,y))];
            if (map_special == V_TILE_NONE && glyph_to_cmap(glyph) == S_cloud && back_to_glyph(x,y) != glyph)
                map_special = V_TILE_STINKING_CLOUD;
        }

        /* physically seen tiles cannot be dark */
        vultures_map_tile_is_dark[y][x] = 0;
    }

    /* location seen via some form of magical vision OR
     * when a save is restored*/
    else
    {
        /* if we can see a monster here, it will be re-shown explicitly */
        map_mon = V_TILE_NONE;

        /* need to clear special effect explicitly here:
         * if we just got blinded by lightnig, the beam will remain onscreen otherwise */
        map_special = V_TILE_NONE;

        /* monsters */
        if (glyph_is_monster(glyph))
        {
            mapglyph(glyph, &character, &colour, &vultures_map_specialattr[y][x], x, y);

            map_mon = vultures_monster_to_tile(glyph_to_mon(glyph), x, y);

            /* if seen telepathically in an unexplored area, it might not have a floor */
            if (map_back == V_TILE_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
                map_back = V_TILE_FLOOR_NOT_VISIBLE;
        }

        /* handle invisible monsters */
        else if (glyph_is_invisible(glyph))
        {
            mapglyph(glyph, &character, &colour, &vultures_map_specialattr[y][x], x, y);
            map_mon = V_TILE_INVISIBLE_MONSTER;
        }

        /* handle monsters you are warned of */
        else if (glyph_is_warning(glyph))
        {
            map_mon = V_TILE_WARNLEV_1 + glyph_to_warning(glyph);
            if (map_back == V_TILE_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
                map_back = V_TILE_FLOOR_NOT_VISIBLE;
        }

        /* same as above, for objects */
        else if (glyph_is_object(glyph))
        {
            map_obj = vultures_object_to_tile(glyph_to_obj(glyph), x, y);
            if (map_back == V_TILE_UNMAPPED_AREA && level.locations[x][y].typ != STONE)
                map_back = V_TILE_FLOOR_NOT_VISIBLE;
        }

        /* traps are not seen magically (I think?), so this only triggers when loading a level */
        else if (glyph_is_trap(glyph))
        {
            map_trap = vultures_tilemap_special[glyph_to_cmap(glyph)];
            if (map_back == V_TILE_UNMAPPED_AREA)
                map_back = V_TILE_FLOOR_NOT_VISIBLE;
        }
	
        else if (glyph_is_cmap(glyph))
        {
            /* Nethack shows us the cmaps, therefore there are no traps or objects here */
            map_obj = V_TILE_NONE;
            map_trap = V_TILE_NONE;

            /* IS_FURNITURE() may be true while the cmap is S_stone for dark rooms  */
            if (IS_FURNITURE(level.locations[x][y].typ) && glyph_to_cmap(glyph) != S_stone)
            {
                map_furniture = vultures_tilemap_special[glyph_to_cmap(glyph)];
                map_back = V_TILE_FLOOR_COBBLESTONE;
            }
            else if (V_EXTRA_FURNITURE(level.locations[x][y].typ) && glyph_to_cmap(glyph) != S_stone)
            {
                /* V_EXTRA_FURNITURE = doors, drawbridges, iron bars
                 * that stuff is not furniture, but we need to draw it at the same time,
                 * so we pack it in with the furniture ...*/
                map_furniture = vultures_tilemap_special[glyph_to_cmap(back_to_glyph(x,y))];
                map_back = V_TILE_FLOOR_ROUGH;
            }
            else
            {
                map_furniture = V_TILE_NONE;
                map_back = vultures_tilemap_special[glyph_to_cmap(glyph)];
            }
        }

        /* When a save is restored, we are shown a number of glyphs for objects, traps, etc
         * whose background we actually know and can display, even though we can't physically see it*/
        if (level.locations[x][y].seenv != 0 && map_back == V_TILE_FLOOR_NOT_VISIBLE)
        {
            if (IS_FURNITURE(level.locations[x][y].typ))
            {
                map_furniture = vultures_tilemap_special[glyph_to_cmap(back_to_glyph(x,y))];
                /* furniture is only found in rooms, so set the background type */
                map_back = V_TILE_FLOOR_COBBLESTONE;
            }
            else if (V_EXTRA_FURNITURE(level.locations[x][y].typ))
            {
                /* this stuff is not furniture, but we need to draw it at the same time,
                 * so we pack it in with the furniture ...*/
                map_furniture = vultures_tilemap_special[glyph_to_cmap(back_to_glyph(x,y))];
                map_back = V_TILE_FLOOR_ROUGH;
            }
            else
            {
                map_furniture = V_TILE_NONE;
                map_back = vultures_tilemap_special[glyph_to_cmap(back_to_glyph(x,y))];
            }
        }

        /* handle unlit room tiles; until now we were assuming them to be lit; whereas tiles
         * converted via vultures_tilemap_special are currently V_TILE_NONE */
        if ( ((!level.locations[x][y].waslit) && map_back == V_TILE_FLOOR_COBBLESTONE) ||
           (level.locations[x][y].typ == ROOM && map_back == V_TILE_UNMAPPED_AREA &&
           level.locations[x][y].seenv))
        {
            map_back = V_TILE_FLOOR_COBBLESTONE;
            vultures_map_tile_is_dark[y][x] = 2;
        }
        else
            vultures_map_tile_is_dark[y][x] = 1;
    }

    /* if the lightlevel changed, we need to force the tile to be updated */
    int force_update = (darkness != vultures_map_tile_is_dark[y][x]);


    /* fix drawbriges, so they don't start in the middle of the moat when they're open */
    /* horizontal drawbridges */
    if (level.locations[x][y].typ == 22 &&
       level.locations[x-1][y].typ == DRAWBRIDGE_DOWN && level.locations[x-1][y].seenv)
    {
        map_furniture = V_TILE_HODBRIDGE;
        switch (level.locations[x][y].drawbridgemask & DB_UNDER)
        {
            case DB_MOAT:  
                vultures_set_map_data(vultures_map_back, x-1, y, vultures_tilemap_special[S_pool], force_update);
                break;

            case DB_LAVA:
                vultures_set_map_data(vultures_map_back, x-1, y, vultures_tilemap_special[S_lava], force_update);
                break;

            case DB_ICE:
                vultures_set_map_data(vultures_map_back, x-1, y, vultures_tilemap_special[S_ice], force_update);
                break;

            case DB_FLOOR:
                vultures_set_map_data(vultures_map_back, x-1, y, vultures_tilemap_special[S_room], force_update);
                break;
        }
    }


    /* vertical drawbridges */
    if (level.locations[x][y].typ == 22 &&
       level.locations[x][y-1].typ == DRAWBRIDGE_DOWN && level.locations[x][y-1].seenv)
    {
        map_furniture = V_TILE_VODBRIDGE;
        switch (level.locations[x][y].drawbridgemask & DB_UNDER)
        {
            case DB_MOAT:
                vultures_set_map_data(vultures_map_back, x, y-1, vultures_tilemap_special[S_pool], force_update);
                break;

            case DB_LAVA:
                vultures_set_map_data(vultures_map_back, x, y-1, vultures_tilemap_special[S_lava], force_update);
                break;

            case DB_ICE:
                vultures_set_map_data(vultures_map_back, x, y-1, vultures_tilemap_special[S_ice], force_update);
                break;

            case DB_FLOOR:
                vultures_set_map_data(vultures_map_back, x, y-1, vultures_tilemap_special[S_room], force_update);
                break;
        }
    }
    
    /* write new map data */

    vultures_set_map_data(vultures_map_mon, x, y, map_mon, force_update);
    vultures_set_map_data(vultures_map_obj, x, y, map_obj, force_update);
    vultures_set_map_data(vultures_map_trap, x, y, map_trap, force_update);
    vultures_set_map_data(vultures_map_specialeff, x, y, map_special, force_update);
    vultures_set_map_data(vultures_map_furniture, x, y, map_furniture, force_update);
    vultures_set_map_data(vultures_map_back, x, y, map_back, force_update);
}



static void vultures_clear_walls(int y, int x)
{
    vultures_maptile_wall[y][x].west = V_TILE_NONE;
    vultures_maptile_wall[y][x].north = V_TILE_NONE;
    vultures_maptile_wall[y][x].east = V_TILE_NONE;
    vultures_maptile_wall[y][x].south = V_TILE_NONE;
}



static void vultures_clear_floor_edges(int y, int x)
{
    int i;
    for (i = 0; i < 12; i++)
        vultures_maptile_floor_edge[y][x].dir[i] = V_TILE_NONE;
}



/*****************************************************************************
 * 4) Computing map data
 *****************************************************************************/

static void vultures_init_floor_decors(int num_decors)
{
    int i, j, k;
    int lx, ly;

    if (!nroom)
        return; /* the level doesn't have distinct rooms, so do nothing */

    /* after putting a decor in room roomno, we calculate (roomno + 5) modulo nroom.
     * (nroom is the global containing the number of rooms on the level) */
    int roomno = 0;
    /* when roomno wraps we also add wrapadd, to ensure that a different bunch of rooms gets the next few decorations
     * this will distibute decorations seemingly at random but repeatably throughout the level
     * (rooms are sorted from left to right, so a step of 1 would leave most decorations on the left side)*/
    int wrapadd = (nroom % 5) ? 0 : 1;

    /* if placing a decor fails, try at most retries other rooms */
    int retries;

    /* did we manage to place the deco?  */
    int placed;

    int old_deco, old_deco2;
    int deco_height;
    int deco_width;
    int xoffset, yoffset;

    int current_deco = V_FLOOR_CARPET;

    /* reset the room indices and map decor arrays */
    for (i = 0; i < V_MAP_HEIGHT; i++)
    {
        memset(vultures_room_indices[i], 0, V_MAP_WIDTH);
        memset(vultures_map_deco[i], 0, V_MAP_WIDTH);
    }

    /* init room indices */
    for (i = 0; i < nroom; i++)
        for (j = rooms[i].ly; j <= rooms[i].hy; j++)
            memset(&vultures_room_indices[j][rooms[i].lx], (i+1), (rooms[i].hx-rooms[i].lx+1));

    /* do no more if we're on the rogue level or in the mines, because decors there look dumb */
#ifdef REINCARNATION
    if (Is_rogue_level(&u.uz))
        return;
#endif
    s_level * lev;
    if (In_mines(&u.uz) && ((lev = Is_special(&u.uz)) == 0 || !lev->flags.town))
        return;

    for (i = 0; i < num_decors; i++)
    {
        retries = nroom;
        placed = 0;
        switch (roomno % 3)
        {
            case 0: current_deco = V_FLOOR_CARPET; break;
            case 1: current_deco = V_FLOOR_MURAL; break;
            case 2: current_deco = V_FLOOR_MURAL2; break;
        }

        deco_width = floorstyles[current_deco].x;
        deco_height = floorstyles[current_deco].y;

        while (retries-- && !placed)
        {
            lx = rooms[roomno].lx;
            ly = rooms[roomno].ly;
            while (ly <= rooms[roomno].hy && (old_deco = (vultures_map_deco[ly][lx] >> 4)) != 0)
            {
                while (lx <= rooms[roomno].hx && (old_deco2 = (vultures_map_deco[ly][lx] >> 4)) != 0)
                    lx += floorstyles[old_deco2-1].x;
                ly += floorstyles[old_deco-1].y;
                lx = rooms[roomno].lx;
            }

            if ((rooms[roomno].hx - lx + 1) >= deco_width &&
                (rooms[roomno].hy - ly + 1) >= deco_height)
            {
                placed = 1;
                for (j=0; j<deco_height; j++)
                    for (k=0; k<deco_width;k++)
                        vultures_map_deco[ly+j][lx+k] = ((current_deco+1) << 4) + (j*deco_width+k);
            }
        }

        if (!retries && !placed)
            /* placing this one failed, so trying to place others is futile  */
            break;

        roomno += 5;
        if (roomno > nroom)
            roomno = (roomno % nroom) + wrapadd;
    }

    /* centre the decorations in the rooms, as the previous code always plces them in the top corner*/
    for (i = 0; i < nroom; i++)
    {
        lx = rooms[i].lx;
        ly = rooms[i].ly;

        while (vultures_map_deco[ly][lx] != 0 && lx <= rooms[i].hx)
            lx++;

        deco_width = lx - rooms[i].lx;
        lx = rooms[i].lx;

        while (vultures_map_deco[ly][lx] != 0 && ly <= rooms[i].hy)
            ly++;

        deco_height = ly - rooms[i].ly;
        xoffset = (int)((rooms[i].lx + rooms[i].hx + 1)/2.0 - deco_width/2.0);
        yoffset = (int)((rooms[i].ly + rooms[i].hy + 1)/2.0 - deco_height/2.0);

        for (j = deco_height-1; j >= 0; j--)
            for (k = deco_width-1; k >= 0; k--)
                vultures_map_deco[yoffset + j][xoffset + k] = vultures_map_deco[rooms[i].ly + j][rooms[i].lx + k];

        for (j = rooms[i].ly; j < yoffset; j++)
            memset(&vultures_map_deco[j][rooms[i].lx], 0, (rooms[i].hx-rooms[i].lx+1));

        for (j = rooms[i].ly; j <= rooms[i].hy; j++)
            memset(&vultures_map_deco[j][rooms[i].lx], 0, (xoffset - rooms[i].lx));
    }
}



static void vultures_get_wall_tiles(int y, int x)
{
    int style;

    if (!level.locations[x][y].seenv)
        return;

    /* x - 1: west wall  */
    if (x > 0 && vultures_map_back[y][x - 1] != V_TILE_WALL_GENERIC && vultures_map_back[y][x - 1] != V_TILE_UNMAPPED_AREA)
    {
        style = vultures_get_wall_decor(vultures_map_back[y][x - 1], y, x, y, x-1);
        vultures_maptile_wall[y][x].west = walltiles[style].west;
    }
    else
        vultures_maptile_wall[y][x].west = V_TILE_NONE;

    /* y - 1: north wall  */
    if (y > 0 && vultures_map_back[y - 1][x] != V_TILE_WALL_GENERIC && vultures_map_back[y - 1][x] != V_TILE_UNMAPPED_AREA)
    {
        style = vultures_get_wall_decor(vultures_map_back[y - 1][x], y, x, y - 1, x);
        vultures_maptile_wall[y][x].north = walltiles[style].north;
    }
    else
        vultures_maptile_wall[y][x].north = V_TILE_NONE;

    /* x + 1: east wall  */
    if (x < V_MAP_WIDTH - 1 && vultures_map_back[y][x + 1] != V_TILE_WALL_GENERIC &&
        vultures_map_back[y][x + 1] != V_TILE_UNMAPPED_AREA)
    {
        style = vultures_get_wall_decor(vultures_map_back[y][x + 1], y, x, y, x + 1);
        vultures_maptile_wall[y][x].east = walltiles[style].east;
    }
    else
        vultures_maptile_wall[y][x].east = V_TILE_NONE;

    /* y + 1: south wall  */
    if (y < V_MAP_HEIGHT - 1 && vultures_map_back[y + 1][x] != V_TILE_WALL_GENERIC &&
        vultures_map_back[y + 1][x] != V_TILE_UNMAPPED_AREA)
    {
        style = vultures_get_wall_decor(vultures_map_back[y + 1][x], y, x, y + 1, x);
        vultures_maptile_wall[y][x].south = walltiles[style].south;
    }
    else
        vultures_maptile_wall[y][x].south = V_TILE_NONE;
}



static int vultures_get_floor_tile(int tile, int y, int x)
{
    int style;
    int deco_pos;
    unsigned char deco = vultures_map_deco[y][x];

    if (deco && tile == V_TILE_FLOOR_COBBLESTONE)
    {
        style = (deco >> 4) - 1;
        deco_pos = (int)(deco & 0x0F);
        return floorstyles[style].array[deco_pos];
    }

    style = vultures_get_floor_decor(tile, y, x);
    deco_pos = floorstyles[style].x * (y % floorstyles[style].y) + (x % floorstyles[style].x);
    return floorstyles[style].array[deco_pos];
}



static void vultures_get_floor_edges(int y, int x)
{
    int i;
    int tile = vultures_map_back[y][x];
    int style = V_FLOOR_EDGE_COBBLESTONE;

    point s_delta[4] = {{-1,0}, {0,-1}, {1,0}, {0,1}};
    point d_delta[4] = {{-1,1}, {-1,-1}, {1,-1}, {1,1}};

   /* Default: no floor edges around tile */
    vultures_clear_floor_edges(y, x);

    /* straight sections */
    for (i = 0; i < 4; i++)
    {
        if (x > 0 && x < V_MAP_WIDTH-s_delta[i].x && y > 0 && y < V_MAP_HEIGHT-s_delta[i].y && 
            tile != vultures_map_back[y+s_delta[i].y][x+s_delta[i].x] &&
            (tile + vultures_map_back[y+s_delta[i].y][x+s_delta[i].x]) !=
            (V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE)) /* this prevents borders between water and ice*/
                vultures_maptile_floor_edge[y][x].dir[i] = flooredges[style].dir[i];
    }


    /* "inward pointing" corners */
    for (i = 4; i < 8; i++)
    {
        if ((vultures_maptile_floor_edge[y][x].dir[(i+3)%4] != V_TILE_NONE) &&
            (vultures_maptile_floor_edge[y][x].dir[i%4]) != V_TILE_NONE)

            vultures_maptile_floor_edge[y][x].dir[i] = flooredges[style].dir[i];
    }


    /* "outward pointing" corners */
    for (i = 8; i < 12; i++)
    {
        if ((vultures_maptile_floor_edge[y][x].dir[(i+3)%4] == V_TILE_NONE) &&
            (vultures_maptile_floor_edge[y][x].dir[i%4] == V_TILE_NONE) &&
            x > 0 && x < V_MAP_WIDTH - 1 && y > 0 && y < V_MAP_HEIGHT - 1 && 
            tile != vultures_map_back[y+d_delta[i%4].y][x+d_delta[i%4].x] &&
            (tile + vultures_map_back[y+d_delta[i%4].y][x+d_delta[i%4].x]) != 
            (V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE))

            vultures_maptile_floor_edge[y][x].dir[i] = flooredges[style].dir[i];
    }

/*    for (i = 0; i < 12; i++)
        if (!vultures_maptile_floor_edge[y][x].dir[i])
            vultures_maptile_floor_edge[y][x].dir[i] = V_TILE_NONE;*/
}



/* get room index is only used to semi-randomly select room decorations
 * therefore the number we return can be as bogus as we want, so long as
 * it's consistently the same. Using the current depth will provide a bit of
 * variety on the maze levels...*/
static int vultures_get_room_index(int x, int y)
{
    int rindex;
    if (nroom == 0) /* maze levels */
        return u.uz.dlevel;

    if (In_mines(&u.uz)) /* The mines are a patchwork dungeon otherwise :( */
        return (u.uz.dlevel + nroom); /* cleverly prevent a repetitive sequence :P */

    rindex = vultures_room_indices[y][x];
    if (!rindex)
        return (u.uz.dlevel + nroom);

    return rindex;
}


/*
 * Convert wall tile index (ie. wall type) to an associated decoration style.
 */
static int vultures_get_wall_decor(
    int floortype,
    int wally, int wallx,
    int floory, int floorx
)
{
    int roomid;

#ifdef REINCARNATION
    if (Is_rogue_level(&u.uz))
        return V_WALL_LIGHT;
#endif

    switch (floortype)
    {
        case V_TILE_FLOOR_ROUGH:
        case V_TILE_FLOOR_ROUGH_LIT:
            return V_WALL_ROUGH; 
        case V_TILE_FLOOR_NOT_VISIBLE:
        case V_TILE_FLOOR_COBBLESTONE:
        {
            roomid = vultures_get_room_index(floorx, floory);
            switch(roomid % 4)
            {
                case 0: return V_WALL_STUCCO;
                case 1: return V_WALL_BRICK + ((wally*wallx+wally+wallx)%5);
                case 2: return V_WALL_VINE_COVERED;
                case 3: return V_WALL_MARBLE;
            }
        }
        default:
            return V_WALL_BRICK; 
    }
}


/*
 * Convert floor tile index (ie. floor type) to an associated decoration style.
 */
static int vultures_get_floor_decor(
  int floorstyle,
  int floory, int floorx
)
{
    int roomid;
#ifdef REINCARNATION
    if (Is_rogue_level(&u.uz))
        return V_FLOOR_DARK;
#endif

    switch (floorstyle)
    {
        case V_TILE_FLOOR_ROUGH:     return V_FLOOR_ROUGH; 
        case V_TILE_FLOOR_ROUGH_LIT: return V_FLOOR_ROUGH_LIT;
        case V_TILE_FLOOR_COBBLESTONE:
        {
            roomid = vultures_get_room_index(floorx, floory);
            switch(roomid % 4)
            {
                case 0: return V_FLOOR_CERAMIC;
                case 1: return V_FLOOR_COBBLESTONE;
                case 2: return V_FLOOR_MOSS_COVERED;
                case 3: return V_FLOOR_MARBLE;
            }
        }
        case V_TILE_FLOOR_WATER:     return V_FLOOR_WATER;
        case V_TILE_FLOOR_ICE:       return V_FLOOR_ICE;
        case V_TILE_FLOOR_AIR:       return V_FLOOR_AIR;
        case V_TILE_FLOOR_LAVA:      return V_FLOOR_LAVA;
        case V_TILE_FLOOR_DARK:      return V_FLOOR_DARK;
        default:                     return V_FLOOR_COBBLESTONE; 
    }
}



/*****************************************************************************
 * 5) Interface functions for windowing
 *****************************************************************************/

char * vultures_map_square_description(point target, int include_seen)
{
    struct permonst *pm;
    char   *out_str, look_buf[BUFSZ];
    char   temp_buf[BUFSZ], coybuf[QBUFSZ];  
    char   monbuf[BUFSZ];
    struct monst *mtmp = (struct monst *) 0;
    const char *firstmatch;
    int n_objs;
    struct obj * obj;

    if ((target.x < 1) || (target.x >= V_MAP_WIDTH) || (target.y < 0) || (target.y >= V_MAP_HEIGHT))
       return NULL;

    /* All of monsters, objects, traps and furniture get descriptions */
    if ((vultures_map_mon[target.y][target.x] != V_TILE_NONE))
    {
        out_str = malloc(BUFSZ);
        out_str[0] = '\0';

        look_buf[0] = '\0';
        monbuf[0] = '\0';
        pm = lookat(target.x, target.y, look_buf, monbuf);
        firstmatch = look_buf;
        if (*firstmatch)
        {
            mtmp = m_at(target.x, target.y);
            Sprintf(temp_buf, "%s", (pm == &mons[PM_COYOTE]) ? coyotename(mtmp,coybuf) : firstmatch);
            strncat(out_str, temp_buf, BUFSZ-strlen(out_str)-1);
        }
        if (include_seen)
        {
            if (monbuf[0])
            {
                sprintf(temp_buf, " [seen: %s]", monbuf);
                strncat(out_str, temp_buf, BUFSZ-strlen(out_str)-1);
            }
        }
        return out_str;
    }
    else if (vultures_map_obj[target.y][target.x] != V_TILE_NONE)
    {
        out_str = malloc(BUFSZ);
        look_buf[0] = '\0';
        monbuf[0] = '\0';
        lookat(target.x, target.y, look_buf, monbuf);
        
        n_objs = 0;
        obj = level.objects[target.x][target.y];
        while(obj)
        {
            n_objs++;
            obj = obj->nexthere;
        }
        
        if (n_objs > 1)
            snprintf(out_str, BUFSZ, "%s (+%d other object%s)", look_buf, n_objs - 1, (n_objs > 2) ? "s" : "");
        else
            strncpy(out_str, look_buf, BUFSZ);

        return out_str;
    }
    else if ((vultures_map_trap[target.y][target.x] != V_TILE_NONE) ||
             (vultures_map_furniture[target.y][target.x] != V_TILE_NONE))
    {
        out_str = malloc(BUFSZ);
        lookat(target.x, target.y, out_str, monbuf);

        return out_str;
    }

    return NULL;
}

point vultures_mouse_to_map(point mouse)
{
    point mappos, px_offset;
    struct window * map = vultures_get_window(0);

    px_offset.x = mouse.x - (map->w / 2) + (vultures_view_x - vultures_view_y) * V_MAP_XMOD;
    px_offset.y = mouse.y - (map->h / 2) + (vultures_view_x+vultures_view_y)*V_MAP_YMOD;

    mappos.x = ( V_MAP_YMOD * px_offset.x + V_MAP_XMOD * px_offset.y +
             V_MAP_XMOD*V_MAP_YMOD)/(2*V_MAP_XMOD*V_MAP_YMOD);
    mappos.y = (-V_MAP_YMOD * px_offset.x + V_MAP_XMOD * px_offset.y +
             V_MAP_XMOD*V_MAP_YMOD)/(2*V_MAP_XMOD*V_MAP_YMOD);

    return mappos;
}

point vultures_map_to_mouse(point mappos)
{
    struct window * map = vultures_get_window(WIN_MAP);
    int map_centre_x = map->w / 2;
    int map_centre_y = map->h / 2;
    point mouse;

    mouse.x = map_centre_x + V_MAP_XMOD*(mappos.x - mappos.y + vultures_view_y - vultures_view_x);
    mouse.y = map_centre_y + V_MAP_YMOD*(mappos.x + mappos.y - vultures_view_y - vultures_view_x);

    /* FIXME Why does this happen sometimes ? */
    if ( mouse.x < 0 ) mouse.x = 0;
    if ( mouse.y < 0 ) mouse.y = 0;

    return mouse;
}

int vultures_get_map_action(point mappos)
{
    int mapglyph_offset;

    /* Off-map squares have no default action */
    if ((mappos.x < 1) || (mappos.x >= V_MAP_WIDTH) ||
        (mappos.y < 0) || (mappos.y >= V_MAP_HEIGHT))
        return 0;


    /* Target is at least 2 squares away */
    if ((abs(u.ux-mappos.x) >= 2) || (abs(u.uy-mappos.y) >= 2))
        return V_ACTION_TRAVEL;

    /* Monster on target square */
    if (vultures_map_mon[mappos.y][mappos.x] != V_TILE_NONE &&
        (u.ux != mappos.x || u.uy != mappos.y))
        return V_ACTION_MOVE_HERE;

    /* Object on target square */
    if (vultures_map_obj[mappos.y][mappos.x] != V_TILE_NONE &&
        u.ux == mappos.x && u.uy == mappos.y)
    {
        mapglyph_offset = vultures_map_obj[mappos.y][mappos.x];
        switch(mapglyph_offset - OBJTILEOFFSET)
        {
            case LARGE_BOX:
            case ICE_BOX:
            case CHEST:
                return V_ACTION_LOOT;

            default:
                return V_ACTION_PICK_UP;
        }
    }

    /* map feature on target square */
    if (vultures_map_furniture[mappos.y][mappos.x] != V_TILE_NONE)
    {
        if ((u.ux == mappos.x) && (u.uy == mappos.y))
            switch (vultures_map_furniture[mappos.y][mappos.x])
            {
                case V_TILE_STAIRS_DOWN:
                case V_TILE_LADDER_DOWN:
                    return V_ACTION_GO_DOWN;

                case V_TILE_STAIRS_UP:
                case V_TILE_LADDER_UP:
                    return V_ACTION_GO_UP;

                case V_TILE_FOUNTAIN:
                    return V_ACTION_DRINK;
            }

        else
            switch (vultures_map_furniture[mappos.y][mappos.x])
            {
                case V_TILE_VDOOR_WOOD_CLOSED:
                case V_TILE_HDOOR_WOOD_CLOSED:
                    return V_ACTION_OPEN_DOOR;
            }
    }

    /* default action for your own square */
    if (u.ux == mappos.x && u.uy == mappos.y)
        return V_ACTION_SEARCH;

    /* default action for adjacent squares (nonadjacent squares were handled further up)*/
    /* if the square contains an object and there is no monster ther, use trvel after all
     * to suppress the messegebox listing the objects */
    if (vultures_map_obj[mappos.y][mappos.x] && !level.monsters[mappos.x][mappos.y])
        return V_ACTION_TRAVEL;

    if (u.ux != mappos.x || u.uy != mappos.y)
        return V_ACTION_MOVE_HERE;

    return 0;
}



int vultures_get_map_contextmenu(point mappos)
{
    struct window * menu;
    int mapglyph_offset;
    int result;


    /* Dropdown commands are shown only for valid squares */
    if ((mappos. x < 1) || (mappos. x >= V_MAP_WIDTH) || (mappos. y < 0) || (mappos. y >= V_MAP_HEIGHT))
        return(0);

    /* Construct a context-sensitive drop-down menu */
    menu = vultures_create_window_internal(0, NULL, V_WINTYPE_DROPDOWN);

    if ((u.ux == mappos. x) && (u.uy == mappos. y))
    {
        /* Add personal options: */
        vultures_create_button(menu, "Engrave", V_ACTION_ENGRAVE);
        vultures_create_button(menu, "Look around", V_ACTION_LOOK_AROUND);
        vultures_create_button(menu, "Monster ability", V_ACTION_MONSTER_ABILITY);

        if (*u.ushops)
            vultures_create_button(menu, "Pay bill", V_ACTION_PAY_BILL);

        vultures_create_button(menu, "Pray", V_ACTION_PRAY);
        vultures_create_button(menu, "Rest", V_ACTION_REST);
        vultures_create_button(menu, "Search", V_ACTION_SEARCH);
        vultures_create_button(menu, "Sit", V_ACTION_SIT);

        /* do a minimum check to leave turn undead out for those who _definitely_ can't do it */
#ifdef VULTURESEYE
        if (Role_if(PM_PRIEST) || Role_if(PM_KNIGHT) ||
#else /* VULTURESCLAW */
        if (tech_known(T_TURN_UNDEAD) ||
#endif
            objects[SPE_TURN_UNDEAD].oc_name_known)
            vultures_create_button(menu, "Turn undead", V_ACTION_TURN_UNDEAD);

        vultures_create_button(menu, "Wipe face", V_ACTION_WIPE_FACE);
    }

    /* monster options */
    else if (vultures_map_mon[mappos. y][mappos. x] != V_TILE_NONE)
        if ((abs(u.ux-mappos. x) <= 1) && (abs(u.uy-mappos. y) <= 1))
        {
            vultures_create_button(menu, "Chat", V_ACTION_CHAT);
            vultures_create_button(menu, "Fight", V_ACTION_FIGHT);
            vultures_create_button(menu, "Name", V_ACTION_NAMEMON);
        }


    /* Add object options: */
    if (vultures_map_obj[mappos. y][mappos. x] != V_TILE_NONE &&
        (abs(u.ux-mappos. x) <= 1 && abs(u.uy-mappos. y) <= 1))
    {
        mapglyph_offset =  vultures_map_obj[mappos. y][mappos. x];
        switch(mapglyph_offset - OBJTILEOFFSET)
        {
            case LARGE_BOX:
            case ICE_BOX:
            case CHEST:
                if ((u.ux == mappos. x) && (u.uy == mappos. y))
                {
                    vultures_create_button(menu, "Force lock", V_ACTION_FORCE_LOCK);
                    vultures_create_button(menu, "Loot", V_ACTION_LOOT);
                    vultures_create_button(menu, "Pick up", V_ACTION_PICK_UP);
                }
                vultures_create_button(menu, "Untrap", V_ACTION_UNTRAP);
                break;

            case SACK:
            case OILSKIN_SACK:
            case BAG_OF_HOLDING:
            case BAG_OF_TRICKS:
            case (V_TILE_UNIDENTIFIED_BAG + OBJTILEOFFSET):
                if ((u.ux == mappos. x) && (u.uy == mappos. y))
                {
                    vultures_create_button(menu, "Loot", V_ACTION_LOOT);
                    vultures_create_button(menu, "Pick up", V_ACTION_PICK_UP);
                }
                break;

            default:
                if ((u.ux == mappos. x) && (u.uy == mappos. y))
                    vultures_create_button(menu, "Pick up", V_ACTION_PICK_UP);
                break;
        }
    }


    /* Add location options: */
    if (vultures_map_furniture[mappos. y][mappos. x] != V_TILE_NONE &&
        abs(u.ux-mappos. x) <= 1 && abs(u.uy-mappos. y) <= 1)
    {
        switch(vultures_map_furniture[mappos. y][mappos. x])
        {
            case V_TILE_STAIRS_DOWN: case V_TILE_LADDER_DOWN:
                if ((u.ux == mappos. x) && (u.uy == mappos. y))
                    vultures_create_button(menu, "Go down", V_ACTION_GO_DOWN);
                break;

            case V_TILE_STAIRS_UP: case V_TILE_LADDER_UP:
                if ((u.ux == mappos. x) && (u.uy == mappos. y))
                    vultures_create_button(menu, "Go up", V_ACTION_GO_UP);
                break;

            case V_TILE_FOUNTAIN:
                if ((u.ux == mappos. x) && (u.uy == mappos. y))
                    vultures_create_button(menu, "Drink", V_ACTION_DRINK);
                break;

            case V_TILE_VDOOR_WOOD_OPEN: case V_TILE_HDOOR_WOOD_OPEN:
                if ((u.ux != mappos. x) || (u.uy != mappos. y))
                {
                    vultures_create_button(menu, "Close door", V_ACTION_CLOSE_DOOR);
                    vultures_create_button(menu, "Untrap", V_ACTION_UNTRAP);
                    vultures_create_button(menu, "Kick", V_ACTION_KICK);
                }
                break;

            case V_TILE_VDOOR_WOOD_CLOSED: case V_TILE_HDOOR_WOOD_CLOSED:
                if ((u.ux != mappos. x) || (u.uy != mappos. y))
                {
                    vultures_create_button(menu, "Open door", V_ACTION_OPEN_DOOR);
                    vultures_create_button(menu, "Untrap", V_ACTION_UNTRAP);
                    vultures_create_button(menu, "Kick", V_ACTION_KICK);
                }
                break;

            case V_TILE_ALTAR:
                if ((u.ux == mappos. x) && (u.uy == mappos. y))
                    vultures_create_button(menu, "Offer", V_ACTION_OFFER);
                else
                    vultures_create_button(menu, "Kick", V_ACTION_KICK);
                break;

            default:
                if ((u.ux != mappos. x) || (u.uy != mappos. y))
                    vultures_create_button(menu, "Kick", V_ACTION_KICK);
                break;
        }
    }

    if (vultures_map_trap[mappos. y][mappos. x] != V_TILE_NONE)
    {
        vultures_create_button(menu, "Untrap", V_ACTION_UNTRAP);
        if ((u.ux != mappos. x) || (u.uy != mappos. y))
            if ((abs(u.ux-mappos. x) <= 1) && (abs(u.uy-mappos. y) <= 1))
                vultures_create_button(menu, "Enter trap", V_ACTION_MOVE_HERE);
    }

    if (vultures_map_back[mappos. y][mappos. x] != V_TILE_NONE)
    {
        if ((u.ux != mappos. x) || (u.uy != mappos. y))    
            vultures_create_button(menu, "Move here", V_ACTION_TRAVEL);
        vultures_create_button(menu, "What's this?", V_ACTION_WHATS_THIS);
    }


    vultures_layout_dropdown(menu);

    vultures_event_dispatcher(&result, V_RESPOND_INT, menu);

    vultures_destroy_window_internal(menu);

    return result;
}



int vultures_perform_map_action(int action_id, point mappos)
{
    switch (action_id)
    {
        case V_ACTION_TRAVEL:
            u.tx = mappos.x;
            u.ty = mappos.y;
            return CMD_TRAVEL;

        case V_ACTION_MOVE_HERE:
            return vultures_mappos_to_dirkey(mappos);

        case V_ACTION_LOOT:        return META('l');
        case V_ACTION_PICK_UP:     return ',';
        case V_ACTION_GO_DOWN:     return '>';
        case V_ACTION_GO_UP:       return '<';
        case V_ACTION_DRINK:       return 'q';
        case V_ACTION_SEARCH:      return 's';
        case V_ACTION_ENGRAVE:     return 'E';
        case V_ACTION_LOOK_AROUND: return ':';
        case V_ACTION_PAY_BILL:    return 'p';
        case V_ACTION_OFFER:       return META('o');
        case V_ACTION_PRAY:        return META('p');
        case V_ACTION_REST:        return '.';
        case V_ACTION_SIT:         return META('s');
        case V_ACTION_TURN_UNDEAD: return META('t');
        case V_ACTION_WIPE_FACE:   return META('w');
        case V_ACTION_FORCE_LOCK:  return META('f');
        case V_ACTION_MONSTER_ABILITY: return META('m');

        case V_ACTION_KICK:
            vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
            return CTRL('d');

        case V_ACTION_OPEN_DOOR:
            vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
            return 'o';

        case V_ACTION_UNTRAP:
            vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
            return META('u');

        case V_ACTION_CLOSE_DOOR:
            vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
            return 'c';

        case V_ACTION_WHATS_THIS:
            /* events get stored on a STACK, so calls to eventstack add 
             * need to be done in revers order */
            vultures_whatis_singleshot = 1;

            /* select the mapsquare */
            vultures_eventstack_add(0, mappos.x, mappos.y, V_RESPOND_POSKEY);

            /* yes, we _really do_ want more info */
            vultures_eventstack_add('y', -1, -1, V_RESPOND_CHARACTER);

            return '/';

        case V_ACTION_CHAT:
            vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_CHARACTER);
            return META('c');

        case V_ACTION_FIGHT:
            vultures_eventstack_add(vultures_mappos_to_dirkey(mappos),-1,-1, V_RESPOND_POSKEY);
            return 'F';

        case V_ACTION_NAMEMON:
            vultures_eventstack_add(0, mappos.x, mappos.y, V_RESPOND_POSKEY);
            return 'C';


        default:
            break;
    }
    return 0;
}


int vultures_get_map_cursor(point mappos)
{
    if ((mappos.x < 1) || (mappos.x >= V_MAP_WIDTH) ||
        (mappos.y < 0) || (mappos.y >= V_MAP_HEIGHT))
        return V_CURSOR_TARGET_INVALID;

     if (vultures_whatis_active)
         return V_CURSOR_TARGET_HELP;

    if (vultures_map_mon[mappos.y][mappos.x] != V_TILE_NONE &&
        ((mappos.x != u.ux) || (mappos.y != u.uy)))
        return V_CURSOR_TARGET_RED;

    if (vultures_map_obj[mappos.y][mappos.x] != V_TILE_NONE)  
        return V_CURSOR_TARGET_RED;

    if (vultures_map_back[mappos.y][mappos.x] != V_TILE_NONE)  /* valid visible location  */
    {
        /* Closed doors get an 'open door' cursor */
        if ((vultures_map_furniture[mappos.y][mappos.x] == V_TILE_VDOOR_WOOD_CLOSED) ||
            (vultures_map_furniture[mappos.y][mappos.x] == V_TILE_HDOOR_WOOD_CLOSED))
            return V_CURSOR_OPENDOOR;

        /* Stairs and ladders get a 'stairs' cursor */
        if ((vultures_map_furniture[mappos.y][mappos.x] == V_TILE_STAIRS_UP) ||
            (vultures_map_furniture[mappos.y][mappos.x] == V_TILE_STAIRS_DOWN) ||
            (vultures_map_furniture[mappos.y][mappos.x] == V_TILE_LADDER_UP) ||
            (vultures_map_furniture[mappos.y][mappos.x] == V_TILE_LADDER_DOWN))
            return V_CURSOR_STAIRS;

        /* Fountains get a 'goblet' cursor */
        if (vultures_map_furniture[mappos.y][mappos.x] == V_TILE_FOUNTAIN)
            return V_CURSOR_GOBLET;

        if (vultures_map_back[mappos.y][mappos.x] != V_TILE_WALL_GENERIC)
            return V_CURSOR_TARGET_GREEN;
    }

    return V_CURSOR_TARGET_INVALID;
}



/*****************************************************************************
 * 6) misc utility functions
 *****************************************************************************/

int vultures_object_to_tile(int obj_id, int x, int y)
{
    struct obj *obj;
    int tile;
    static int lastfigurine = PM_KNIGHT;
    static int laststatue = PM_KNIGHT;


    if (obj_id < 0 || obj_id > NUM_OBJECTS)
        /* we get *seriously weird* input for hallucinated corpses */
        obj_id = CORPSE;


    if (x >= 0)
        obj = level.objects[x][y];
    else
        obj = invent;

    while (obj && !(obj->otyp == obj_id && (x >= 0 || obj->invlet == y)))
        obj = (x >= 0) ? obj->nexthere : obj->nobj;


    if (obj && Blind && !obj->dknown)
    {
        switch (obj->oclass)
        {
            case AMULET_CLASS: return OBJECT_TO_VTILE(AMULET_OF_ESP);
            case POTION_CLASS: return OBJECT_TO_VTILE(POT_WATER);
            case SCROLL_CLASS: return OBJECT_TO_VTILE(SCR_BLANK_PAPER);
            case WAND_CLASS:   return OBJECT_TO_VTILE(WAN_NOTHING);
            case SPBOOK_CLASS: return OBJECT_TO_VTILE(SPE_BLANK_PAPER);
            case RING_CLASS:   return OBJECT_TO_VTILE(RIN_ADORNMENT);
            case GEM_CLASS:
                if (objects[obj_id].oc_material == MINERAL)
                    return OBJECT_TO_VTILE(ROCK);
                else
                    return OBJECT_TO_VTILE(GEM_BLUE_GLASS);
        }
    }


    if (obj_id == STATUE || obj_id == FIGURINE)
    {
        if (obj_id == FIGURINE)
        {
            tile = lastfigurine;
            if (obj)
            {
                tile = obj->corpsenm;
                lastfigurine = tile;
            }

            tile = FIGURINE_TO_VTILE(tile);
        }
        else /* obj_id == STATUE */
        {
            tile = laststatue;
            if (obj)
            {
                tile = obj->corpsenm;
                laststatue = tile;
            }

            tile = STATUE_TO_VTILE(tile);
        }
        return tile;
    }

    if (!objects[obj_id].oc_name_known)
    {
        switch (obj_id)
        {
            case SACK:
            case OILSKIN_SACK:
            case BAG_OF_TRICKS:
            case BAG_OF_HOLDING:
                return V_TILE_UNIDENTIFIED_BAG;

            case LOADSTONE:
            case LUCKSTONE:
            case FLINT:
            case TOUCHSTONE: 
#ifdef HEALTHSTONE /* only in SlashEM */
            case HEALTHSTONE:
#endif
#ifdef WHETSTONE /* only in SlashEM */
            case WHETSTONE:
#endif
                return V_TILE_STONE;
            
            case OIL_LAMP:
            case MAGIC_LAMP:
                return OBJECT_TO_VTILE(OIL_LAMP);

            case TIN_WHISTLE:
            case MAGIC_WHISTLE:
                return OBJECT_TO_VTILE(TIN_WHISTLE);

            case DILITHIUM_CRYSTAL:
            case DIAMOND:
            case RUBY:
            case JACINTH:
            case SAPPHIRE:
            case BLACK_OPAL:
            case EMERALD:
            case TURQUOISE:
            case CITRINE:
            case AQUAMARINE:
            case AMBER:
            case TOPAZ:
            case JET:
            case OPAL:
            case CHRYSOBERYL:
            case GARNET:
            case AMETHYST:
            case JASPER:
            case FLUORITE:
            case OBSIDIAN:
            case AGATE:
            case JADE:
                switch (objects[obj_id].oc_color)
                {
                    case CLR_RED:     tile = GEM_RED_GLASS; break;
                    case CLR_BLACK:   tile = GEM_BLACK_GLASS; break;
                    case CLR_GREEN:   tile = GEM_GREEN_GLASS; break;
                    case CLR_BROWN:   tile = GEM_BROWN_GLASS; break;
                    case CLR_MAGENTA: tile = GEM_VIOLET_GLASS; break;
                    case CLR_ORANGE:  tile = GEM_ORANGE_GLASS; break;
                    case CLR_YELLOW:  tile = GEM_YELLOW_GLASS; break;
                    case CLR_WHITE:   tile = GEM_WHITE_GLASS; break;
                    case CLR_BLUE:    tile = GEM_BLUE_GLASS; break;
                    default:          tile = GEM_BLACK_GLASS; break;
                }
                return OBJECT_TO_VTILE(tile);
        }
    }

    return OBJECT_TO_VTILE(obj_id);
}



static int vultures_monster_to_tile(int mon_id, XCHAR_P x, XCHAR_P y)
{
    if (Invis && u.ux == x && u.uy == y)
        return V_TILE_PLAYER_INVIS;

#ifdef REINCARNATION
    if (Is_rogue_level(&u.uz))
    {
        switch (mon_id)
        {
            case PM_COUATL : case PM_ALEAX : case PM_ANGEL :
            case PM_KI_RIN : case PM_ARCHON :
                return V_TILE_ROGUE_LEVEL_A;

            case PM_GIANT_BAT : case PM_RAVEN :
            case PM_VAMPIRE_BAT : case PM_BAT :
                return V_TILE_ROGUE_LEVEL_B;

            case PM_PLAINS_CENTAUR : case PM_FOREST_CENTAUR :
            case PM_MOUNTAIN_CENTAUR :
                return V_TILE_ROGUE_LEVEL_C;

            case PM_DOG:
            case PM_BABY_GRAY_DRAGON : case PM_BABY_SILVER_DRAGON :
            case PM_BABY_RED_DRAGON :
            case PM_BABY_WHITE_DRAGON : case PM_BABY_ORANGE_DRAGON :
            case PM_BABY_BLACK_DRAGON : case PM_BABY_BLUE_DRAGON :
            case PM_BABY_GREEN_DRAGON : case PM_BABY_YELLOW_DRAGON :
            case PM_GRAY_DRAGON : case PM_SILVER_DRAGON :
            case PM_RED_DRAGON :
            case PM_WHITE_DRAGON : case PM_ORANGE_DRAGON :
            case PM_BLACK_DRAGON : case PM_BLUE_DRAGON :
            case PM_GREEN_DRAGON : case PM_YELLOW_DRAGON :
                return V_TILE_ROGUE_LEVEL_D;

            case PM_STALKER : case PM_AIR_ELEMENTAL :
            case PM_FIRE_ELEMENTAL: case PM_EARTH_ELEMENTAL :
            case PM_WATER_ELEMENTAL :
                return V_TILE_ROGUE_LEVEL_E;

            case PM_LICHEN : case PM_BROWN_MOLD :
            case PM_YELLOW_MOLD : case PM_GREEN_MOLD :
            case PM_RED_MOLD : case PM_SHRIEKER :
            case PM_VIOLET_FUNGUS :
                return V_TILE_ROGUE_LEVEL_F;

            case PM_GNOME : case PM_GNOME_LORD :
            case PM_GNOMISH_WIZARD : case PM_GNOME_KING :
                return V_TILE_ROGUE_LEVEL_G;

            case PM_GIANT : case PM_STONE_GIANT :
            case PM_HILL_GIANT : case PM_FIRE_GIANT :
            case PM_FROST_GIANT : case PM_STORM_GIANT :
            case PM_ETTIN : case PM_TITAN : case PM_MINOTAUR :
                return V_TILE_ROGUE_LEVEL_H;

            case 999990 :	//None
                return V_TILE_ROGUE_LEVEL_I;

            case PM_JABBERWOCK :
                return V_TILE_ROGUE_LEVEL_J;

            case PM_KEYSTONE_KOP : case PM_KOP_SERGEANT :
            case PM_KOP_LIEUTENANT : case PM_KOP_KAPTAIN :
                return V_TILE_ROGUE_LEVEL_K;

            case PM_LICH : case PM_DEMILICH :
            case PM_MASTER_LICH : case PM_ARCH_LICH :
                return V_TILE_ROGUE_LEVEL_L;

            case PM_KOBOLD_MUMMY : case PM_GNOME_MUMMY :
            case PM_ORC_MUMMY : case PM_DWARF_MUMMY :
            case PM_ELF_MUMMY : case PM_HUMAN_MUMMY :
            case PM_ETTIN_MUMMY : case PM_GIANT_MUMMY :
                return V_TILE_ROGUE_LEVEL_M;

            case PM_RED_NAGA_HATCHLING :
            case PM_BLACK_NAGA_HATCHLING :
            case PM_GOLDEN_NAGA_HATCHLING :
            case PM_GUARDIAN_NAGA_HATCHLING :
            case PM_RED_NAGA : case PM_BLACK_NAGA :
            case PM_GOLDEN_NAGA : case PM_GUARDIAN_NAGA :
                return V_TILE_ROGUE_LEVEL_N;

            case PM_OGRE : case PM_OGRE_LORD :
            case PM_OGRE_KING :
                return V_TILE_ROGUE_LEVEL_O;

            case PM_GRAY_OOZE : case PM_BROWN_PUDDING :
            case PM_BLACK_PUDDING : case PM_GREEN_SLIME :
                return V_TILE_ROGUE_LEVEL_P;

            case PM_QUANTUM_MECHANIC :
                return V_TILE_ROGUE_LEVEL_Q;

            case PM_RUST_MONSTER : case PM_DISENCHANTER :
                return V_TILE_ROGUE_LEVEL_R;

            case PM_GARTER_SNAKE : case PM_SNAKE :
            case PM_WATER_MOCCASIN : case PM_PIT_VIPER :
            case PM_PYTHON : case PM_COBRA :
                return V_TILE_ROGUE_LEVEL_S;

            case PM_TROLL : case PM_ICE_TROLL :
            case PM_ROCK_TROLL : case PM_WATER_TROLL :
            case PM_OLOG_HAI :
                return V_TILE_ROGUE_LEVEL_T;

            case PM_UMBER_HULK :
                return V_TILE_ROGUE_LEVEL_U;

            case PM_VAMPIRE : case PM_VAMPIRE_LORD :
                return V_TILE_ROGUE_LEVEL_V;

            case PM_BARROW_WIGHT : case PM_WRAITH :
            case PM_NAZGUL :
                return V_TILE_ROGUE_LEVEL_W;

            case PM_XORN :
                return V_TILE_ROGUE_LEVEL_X;

            case PM_MONKEY : case PM_APE : case PM_OWLBEAR :
            case PM_YETI : case PM_CARNIVOROUS_APE :
            case PM_SASQUATCH :
                return V_TILE_ROGUE_LEVEL_Y;

            case PM_GHOUL:
            case PM_KOBOLD_ZOMBIE : case PM_GNOME_ZOMBIE :
            case PM_ORC_ZOMBIE : case PM_DWARF_ZOMBIE :
            case PM_ELF_ZOMBIE : case PM_HUMAN_ZOMBIE :
            case PM_ETTIN_ZOMBIE : case PM_GIANT_ZOMBIE :
                return V_TILE_ROGUE_LEVEL_Z;

            default:
            {
                if ((mon_id >= 0) && (mon_id < NUMMONS))
                    return MONSTER_TO_VTILE(mon_id);
                else
                    return V_TILE_NONE;
            }
        }
    }
#endif
    if (mon_id == PM_ALEAX)
        return MONSTER_TO_VTILE(u.umonnum);

    if (mon_id == PM_ALIGNED_PRIEST)
    {
        register struct monst *mtmp = m_at(x, y);

        switch (EPRI(mtmp)->shralign)
        {
            case A_LAWFUL:  return V_TILE_LAWFUL_PRIEST;
            case A_CHAOTIC: return V_TILE_CHAOTIC_PRIEST;
            case A_NEUTRAL: return V_TILE_NEUTRAL_PRIEST;
            default:        return V_TILE_UNALIGNED_PRIEST;
        }
    }

    if ((mon_id >= 0) && (mon_id < NUMMONS))
        return MONSTER_TO_VTILE(mon_id);

    return V_TILE_NONE;
}



static char vultures_mappos_to_dirkey(point mappos)
{
    if (mappos.y == u.uy + 1)
    {
        if (mappos.x == u.ux - 1) return iflags.num_pad ? '1' : 'b';
        if (mappos.x == u.ux)     return iflags.num_pad ? '2' : 'j';
        if (mappos.x == u.ux + 1) return iflags.num_pad ? '3' : 'n';
    }
    else if (mappos.y == u.uy)
    {
        if (mappos.x == u.ux - 1) return iflags.num_pad ? '4' : 'h';
        if (mappos.x == u.ux)     return '.';
        if (mappos.x == u.ux + 1) return iflags.num_pad ? '6' : 'l';
    }
    else if (mappos.y == u.uy - 1)
    {
        if (mappos.x == u.ux - 1) return iflags.num_pad ? '7' : 'y';
        if (mappos.x == u.ux)     return iflags.num_pad ? '8' : 'k';
        if (mappos.x == u.ux + 1) return iflags.num_pad ? '9' : 'u';
    }

    /* the caller should make sure we don't get here */
    return 0;
}



static void vultures_build_tilemap(void)
{
    int i;
    vultures_tilemap_special = malloc(MAXPCHARS*sizeof(int));
    vultures_tilemap_engulf = malloc(NUMMONS*sizeof(int));

    for (i = 0; i < NUMMONS; i++)
        vultures_tilemap_engulf[i] = V_TILE_NONE;

    vultures_tilemap_engulf[PM_OCHRE_JELLY]   = V_TILE_ENGULF_OCHRE_JELLY;
    vultures_tilemap_engulf[PM_LURKER_ABOVE]  = V_TILE_ENGULF_LURKER_ABOVE;
    vultures_tilemap_engulf[PM_TRAPPER]       = V_TILE_ENGULF_TRAPPER;
    vultures_tilemap_engulf[PM_PURPLE_WORM]   = V_TILE_ENGULF_PURPLE_WORM;
    vultures_tilemap_engulf[PM_DUST_VORTEX]   = V_TILE_ENGULF_DUST_VORTEX;
    vultures_tilemap_engulf[PM_ICE_VORTEX]    = V_TILE_ENGULF_ICE_VORTEX;
    vultures_tilemap_engulf[PM_ENERGY_VORTEX] = V_TILE_ENGULF_ENERGY_VORTEX;
    vultures_tilemap_engulf[PM_STEAM_VORTEX]  = V_TILE_ENGULF_STEAM_VORTEX;
    vultures_tilemap_engulf[PM_FIRE_VORTEX]   = V_TILE_ENGULF_FIRE_VORTEX;
    vultures_tilemap_engulf[PM_FOG_CLOUD]     = V_TILE_ENGULF_FOG_CLOUD;
    vultures_tilemap_engulf[PM_AIR_ELEMENTAL] = V_TILE_ENGULF_AIR_ELEMENTAL;
    vultures_tilemap_engulf[PM_JUIBLEX]      = V_TILE_ENGULF_JUIBLEX;


    vultures_tilemap_special[S_stone] = V_TILE_UNMAPPED_AREA;
    vultures_tilemap_special[S_vwall] = V_TILE_WALL_GENERIC;
    vultures_tilemap_special[S_hwall] = V_TILE_WALL_GENERIC;
    vultures_tilemap_special[S_tlcorn] = V_TILE_WALL_GENERIC;
    vultures_tilemap_special[S_trcorn] = V_TILE_WALL_GENERIC;
    vultures_tilemap_special[S_blcorn] = V_TILE_WALL_GENERIC;
    vultures_tilemap_special[S_brcorn] = V_TILE_WALL_GENERIC;
    vultures_tilemap_special[S_crwall] = V_TILE_WALL_GENERIC;
    vultures_tilemap_special[S_tuwall] = V_TILE_WALL_GENERIC;
    vultures_tilemap_special[S_tdwall] = V_TILE_WALL_GENERIC;
    vultures_tilemap_special[S_tlwall] = V_TILE_WALL_GENERIC;
    vultures_tilemap_special[S_trwall] = V_TILE_WALL_GENERIC;
    vultures_tilemap_special[S_ndoor] = V_TILE_DOOR_WOOD_BROKEN;
    vultures_tilemap_special[S_vodoor] = V_TILE_VDOOR_WOOD_OPEN;
    vultures_tilemap_special[S_hodoor] = V_TILE_HDOOR_WOOD_OPEN;
    vultures_tilemap_special[S_vcdoor] = V_TILE_VDOOR_WOOD_CLOSED;
    vultures_tilemap_special[S_hcdoor] = V_TILE_HDOOR_WOOD_CLOSED;
    vultures_tilemap_special[S_room] = V_TILE_FLOOR_COBBLESTONE;
    vultures_tilemap_special[S_corr] = V_TILE_FLOOR_ROUGH;
    vultures_tilemap_special[S_upstair] = V_TILE_STAIRS_UP;
    vultures_tilemap_special[S_dnstair] = V_TILE_STAIRS_DOWN;
    vultures_tilemap_special[S_fountain] = V_TILE_FOUNTAIN;
    vultures_tilemap_special[S_altar] = V_TILE_ALTAR;
    vultures_tilemap_special[S_teleportation_trap] = V_TILE_TRAP_TELEPORTER;
    vultures_tilemap_special[S_tree] = V_TILE_TREE;
    vultures_tilemap_special[S_cloud] = V_TILE_CLOUD;
    vultures_tilemap_special[S_air] = V_TILE_FLOOR_AIR;
    vultures_tilemap_special[S_grave] = V_TILE_GRAVE;
    vultures_tilemap_special[S_sink] = V_TILE_SINK;
    vultures_tilemap_special[S_bear_trap] = V_TILE_TRAP_BEAR;
    vultures_tilemap_special[S_rust_trap] = V_TILE_TRAP_WATER;
    vultures_tilemap_special[S_pit] = V_TILE_TRAP_PIT;
    vultures_tilemap_special[S_hole] = V_TILE_TRAP_PIT;
    vultures_tilemap_special[S_trap_door] = V_TILE_TRAP_DOOR;
    vultures_tilemap_special[S_water] = V_TILE_FLOOR_WATER;
    vultures_tilemap_special[S_pool] = V_TILE_FLOOR_WATER;
    vultures_tilemap_special[S_ice] = V_TILE_FLOOR_ICE;
    vultures_tilemap_special[S_lava] = V_TILE_FLOOR_LAVA;
    vultures_tilemap_special[S_throne] = V_TILE_THRONE;
    vultures_tilemap_special[S_bars] = V_TILE_BARS;
    vultures_tilemap_special[S_upladder] = V_TILE_LADDER_UP;
    vultures_tilemap_special[S_dnladder] = V_TILE_LADDER_DOWN;
    vultures_tilemap_special[S_arrow_trap] = V_TILE_TRAP_ARROW;
    vultures_tilemap_special[S_rolling_boulder_trap] = V_TILE_ROLLING_BOULDER_TRAP;
    vultures_tilemap_special[S_sleeping_gas_trap] = V_TILE_GAS_TRAP;
    vultures_tilemap_special[S_fire_trap] = V_TILE_TRAP_FIRE;
    vultures_tilemap_special[S_web] = V_TILE_WEB_TRAP;
    vultures_tilemap_special[S_statue_trap] = OBJECT_TO_VTILE(STATUE);
    vultures_tilemap_special[S_anti_magic_trap] = V_TILE_TRAP_ANTI_MAGIC;
    vultures_tilemap_special[S_polymorph_trap] = V_TILE_TRAP_POLYMORPH;
    vultures_tilemap_special[S_vbeam] = V_TILE_ZAP_VERTICAL;
    vultures_tilemap_special[S_hbeam] = V_TILE_ZAP_HORIZONTAL;
    vultures_tilemap_special[S_lslant] = V_TILE_ZAP_SLANT_LEFT;
    vultures_tilemap_special[S_rslant] = V_TILE_ZAP_SLANT_RIGHT;
    vultures_tilemap_special[S_litcorr] = V_TILE_FLOOR_ROUGH_LIT;
    vultures_tilemap_special[S_ss1] = V_TILE_RESIST_SPELL_1;
    vultures_tilemap_special[S_ss2] = V_TILE_RESIST_SPELL_2;
    vultures_tilemap_special[S_ss3] = V_TILE_RESIST_SPELL_3;
    vultures_tilemap_special[S_ss4] = V_TILE_RESIST_SPELL_4;
    vultures_tilemap_special[S_dart_trap] = V_TILE_DART_TRAP;
    vultures_tilemap_special[S_falling_rock_trap] = V_TILE_FALLING_ROCK_TRAP;
    vultures_tilemap_special[S_squeaky_board] = V_TILE_SQUEAKY_BOARD;
    vultures_tilemap_special[S_land_mine] = V_TILE_LAND_MINE;
    vultures_tilemap_special[S_magic_portal] = V_TILE_MAGIC_PORTAL;
    vultures_tilemap_special[S_spiked_pit] = V_TILE_SPIKED_PIT;
    vultures_tilemap_special[S_hole] = V_TILE_HOLE;
    vultures_tilemap_special[S_level_teleporter] = V_TILE_LEVEL_TELEPORTER;
    vultures_tilemap_special[S_magic_trap] = V_TILE_MAGIC_TRAP;
    vultures_tilemap_special[S_digbeam] = V_TILE_DIGBEAM;
    vultures_tilemap_special[S_flashbeam] = V_TILE_FLASHBEAM;
    vultures_tilemap_special[S_boomleft] = V_TILE_BOOMLEFT;
    vultures_tilemap_special[S_boomright] = V_TILE_BOOMRIGHT;
    vultures_tilemap_special[S_hcdbridge] = V_TILE_HCDBRIDGE;
    vultures_tilemap_special[S_vcdbridge] = V_TILE_VCDBRIDGE;
    vultures_tilemap_special[S_hodbridge] = V_TILE_HODBRIDGE;
    vultures_tilemap_special[S_vodbridge] = V_TILE_VODBRIDGE;

#ifdef VULTURESCLAW
    vultures_tilemap_special[S_toilet] = V_TILE_TOILET;
#endif
}



static int vultures_remap_tile_id(int tile_id)
{
    if (tile_id == V_TILE_PLAYER_INVIS)
        tile_id = MONSTER_TO_VTILE(u.umonnum);

    if (tile_id < NUM_OBJECTS)
        tile_id = objects[tile_id].oc_descr_idx;

    /* if the tile is merely a pointer to another tile we modify the tile_id */
    if (!vultures_gametiles[tile_id].data_len &&
        vultures_gametiles[tile_id].ptr != -1)
    {
        tile_id = vultures_gametiles[tile_id].ptr;
    }
    
    return tile_id;
}

