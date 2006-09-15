/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000 */
/* Copyright (c) Daniel Thaler, 2005 */

#include "vultures_mou.h"
#include "vultures_win.h"
#include "vultures_map.h"
#include "vultures_init.h"
#include "vultures_gfl.h"
#include "vultures_gametiles.h"
#include "vultures_gra.h"
#include "vultures_sdl.h"

/* Lighting constants */
#define JTP_MAX_LIGHTS 10
#define JTP_BRILLIANCE 65.0


/*----------------------------
 * global variables
 *---------------------------- */
/* Map window contents, as Vulture's tile IDs */
int ** jtp_map_glyph = NULL;                /* real glyph representation of map */
int ** jtp_map_back = NULL;      /* background (floors, walls, pools, moats, ...) */
int ** jtp_map_furniture = NULL; /* furniture (stairs, altars, fountains, ...) */
int ** jtp_map_trap = NULL;      /* traps */
int ** jtp_map_obj = NULL;       /* topmost object */
int ** jtp_map_specialeff = NULL;   /* special effects: zap, engulf, explode */
int ** jtp_map_mon = NULL;       /* monster tile ID */
unsigned int ** jtp_map_specialattr = NULL; /* special attributes, we use them to highlight the pet */
unsigned char ** jtp_map_deco = NULL;     /* positions of murals and carpets */
int jtp_map_swallow = V_TILE_NONE; /* the engulf tile, if any */

unsigned char * jtp_map_parchment_center = NULL; /* Map parchment graphic, center area */
unsigned char * jtp_map_parchment_top = NULL;    /* Map parchment graphic, top border */
unsigned char * jtp_map_parchment_bottom = NULL; /* Map parchment graphic, bottom border */
unsigned char * jtp_map_parchment_left = NULL;   /* Map parchment graphic, left border */
unsigned char * jtp_map_parchment_right = NULL;  /* Map parchment graphic, right border */
unsigned char * jtp_map_symbols[JTP_MAX_MAP_SYMBOLS]; /* Map parchment symbols */

/* Light sources */
int jtp_nlights;           /* Number of light sources in use */
struct jtp_lights_struct {
  int x;
  int y;
  double radius;
} jtp_lights[JTP_MAX_LIGHTS]; /* Light source parameters (x,y,radius) */

int jtp_map_changed;       /* Has the map changed since the last glyph->tile conversion? */
int jtp_game_palette_set;  /* Has the in-game palette been set already? */

/* main tile array */
jtp_tile ** jtp_tiles = NULL;

/* pointer to full height, half height or transparent walltile array */
struct walls * walltiles = NULL;

/* Center coordinates of centermost displayed map tile.
 * Used for quantizing target crosshairs location. */
int jtp_map_center_x, jtp_map_center_y;

/* Map window contents, as Vulture's tiles */
jtp_wall_style ** jtp_maptile_wall = NULL; /* Custom (combination) wall style for each square */
jtp_floor_edge_style ** jtp_maptile_floor_edge = NULL; /* Custom floor edge style for each square */

int ** jtp_map_light = NULL;               /* Light levels */
char ** jtp_map_tile_is_dark = NULL;
char ** jtp_room_indices = NULL;           /* packed room numbers and deco ids */


/*----------------------------
 * pre-declared functions
 *---------------------------- */
static void jtp_init_floor_decors(int num_decors);
static void jtp_init_lights(int how_many);
static void jtp_calculate_lights(void);
static jtp_tile *jtp_get_floor_tile(int tile, int y, int x);
static void jtp_get_floor_edges(int y, int x);
static int jtp_get_room_index(int x, int y);
static int jtp_get_wall_decor(int floortype, int wally, int wallx, int floory, int floorx);
static int jtp_get_floor_decor(int floorstyle, int floory, int floorx);
static void jtp_clear_floor_edges(int y, int x);
static void jtp_get_wall_tiles(int y, int x);
static jtp_tile *jtp_make_invisible_player_tile(int monnum);
static jtp_tile *jtp_make_transparent_player_tile(int monnum);
static void jtp_draw_mini_map(void);


/*----------------------------
 * function implementations
 *---------------------------- */
void jtp_draw_level(
  jtp_window * mapwindow,
  int xc, int yc
)
{
    int i, j;
    jtp_tile * cur_tile;
    int lightlevel;
    int x, y;
    int tile_id;

    static int cur_dlevel = -1;

    if (!mapwindow) return;
    if (xc < 0) xc = jtp_map_x;
    if (yc < 0) yc = jtp_map_y;

    /* Check if we need to restore the game palette */
    if ((!jtp_game_palette_set) && (jtp_map_changed))
    {
        memcpy(jtp_colors, jtp_game_colors, sizeof(jtp_colors));
        jtp_refresh(&jtp_screen);
        jtp_updatepal(0, 255);
        jtp_game_palette_set = 1;
    }

    if (jtp_map_changed)
    {
        if (cur_dlevel != u.uz.dlevel)
        {
            jtp_init_floor_decors(10);
            jtp_init_lights(JTP_MAX_LIGHTS);
            cur_dlevel = u.uz.dlevel;
        }
        jtp_calculate_lights();
        jtp_map_changed = 0;
    }
  
    if (Invis)
    {
        if (!canseeself())
            jtp_tiles[V_TILE_PLAYER_INVIS] = jtp_make_invisible_player_tile(u.umonnum);
        else
            jtp_tiles[V_TILE_PLAYER_INVIS] = jtp_make_transparent_player_tile(u.umonnum);
    }

    /* Clear map area */
    memset(jtp_screen.vpage, JTP_COLOR_BACKGROUND, jtp_screen.width*(jtp_screen.height-JTP_STATUSBAR_HEIGHT));

    /* Only draw on map area */
    jtp_set_draw_region(0, 0, jtp_screen.width-1, jtp_screen.height-1-JTP_STATUSBAR_HEIGHT);

    /* If swallowed draw ONLY the engulf tile and the player! */
    if (u.uswallow && jtp_map_swallow != V_TILE_NONE)
    {
        lightlevel = jtp_map_light[u.uy][u.ux];
        x = jtp_map_center_x + JTP_MAP_XMOD*(u.ux - u.uy + yc - xc);
        y = jtp_map_center_y + JTP_MAP_YMOD*(u.ux + u.uy - yc - xc);

        /* engulf tile */
        cur_tile = jtp_tiles[jtp_map_swallow];
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);

        /* player */
        cur_tile = jtp_tiles[jtp_map_mon[u.uy][u.ux]];
        jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                     lightlevel, cur_tile->graphic);

        /* message background is now outdated */
        free(jtp_messages_background);
        jtp_messages_background = NULL;
        return;
    }
    else
        jtp_map_swallow = V_TILE_NONE;


    for (i = 0; i < JTP_MAP_HEIGHT; i++)
    {
        for (j = 1; j < JTP_MAP_WIDTH; j++)
        {
            if (jtp_map_tile_is_dark[i][j] || Blind)
                lightlevel = 15; /* JTP_AMBIENT is 32 ... */
            else
                lightlevel = jtp_map_light[i][j];

            if (jtp_map_back[i][j] == V_TILE_UNMAPPED_AREA)
                lightlevel = JTP_AMBIENT_LIGHT;


            /* Find position of tile center */
            x = jtp_map_center_x + JTP_MAP_XMOD*(j - i + yc - xc);
            y = jtp_map_center_y + JTP_MAP_YMOD*(j + i - yc - xc);

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
            if (jtp_map_back[i][j] == V_TILE_WALL_GENERIC ||
                jtp_map_back[i][j] == V_TILE_UNMAPPED_AREA)
                jtp_get_wall_tiles(i, j);
            else
                /* certain events (amnesia or a secret door being discovered) 
                 * require us to clear walls again. since we cannot check for those cases
                 * we simply clear walls whenever we don't set any... */
                jtp_clear_walls(i, j);

            if (jtp_map_back[i][j] == V_TILE_FLOOR_WATER ||
                jtp_map_back[i][j] == V_TILE_FLOOR_ICE ||
                jtp_map_back[i][j] == V_TILE_FLOOR_LAVA ||
                jtp_map_back[i][j] == V_TILE_FLOOR_AIR)
                /* these tiles get edges */
                jtp_get_floor_edges(i, j);
            else
                /* everything else doesn't. However we can't just assume a tile that doesn't need egdes doesn't have any:
                 * pools may be dried out by fireballs, and suddenly we have a pit with edges :(
                 * Therefore we always clear them explicitly */
                jtp_clear_floor_edges(i ,j);


            /* 2. Floor */
            tile_id = jtp_map_back[i][j];

            if ((tile_id >= V_TILE_FLOOR_COBBLESTONE) &&
                (tile_id <= V_TILE_FLOOR_DARK))		   
                cur_tile = jtp_get_floor_tile(tile_id, i, j);
            else if (tile_id == V_TILE_WALL_GENERIC)
            {
                /* Add the correct backgound color under wall tiles */
                cur_tile = jtp_tiles[V_TILE_UNMAPPED_AREA];
                lightlevel = JTP_AMBIENT_LIGHT;
            }
            else
                cur_tile = jtp_tiles[tile_id];

            if (cur_tile != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);

            /* 3. Floor edges */
            if ((cur_tile = jtp_maptile_floor_edge[i][j].west) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);
            if ((cur_tile = jtp_maptile_floor_edge[i][j].north) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);
            if ((cur_tile = jtp_maptile_floor_edge[i][j].east) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);
            if ((cur_tile = jtp_maptile_floor_edge[i][j].south) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);

            if ((cur_tile = jtp_maptile_floor_edge[i][j].northwest) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);
            if ((cur_tile = jtp_maptile_floor_edge[i][j].northeast) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                            lightlevel, cur_tile->graphic);
            if ((cur_tile = jtp_maptile_floor_edge[i][j].southeast) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                            lightlevel, cur_tile->graphic);
            if ((cur_tile = jtp_maptile_floor_edge[i][j].southwest) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                            lightlevel, cur_tile->graphic);

            if ((cur_tile = jtp_maptile_floor_edge[i][j].northwest_bank) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);
            if ((cur_tile = jtp_maptile_floor_edge[i][j].northeast_bank) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);
            if ((cur_tile = jtp_maptile_floor_edge[i][j].southeast_bank) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);
            if ((cur_tile = jtp_maptile_floor_edge[i][j].southwest_bank) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);

        }
    }

    for (i = 0; i < JTP_MAP_HEIGHT; i++)
    {
        for(j = 1; j < JTP_MAP_WIDTH; j++)
        {
            lightlevel = jtp_map_light[i][j];

            /* Find position of tile center */
            x = jtp_map_center_x + JTP_MAP_XMOD*(j - i + yc - xc);
            y = jtp_map_center_y + JTP_MAP_YMOD*(j + i - yc - xc);


            /* 1. West and north walls */
            if ((cur_tile = jtp_maptile_wall[i][j].west) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             jtp_map_light[i][j-1], cur_tile->graphic);
            if ((cur_tile = jtp_maptile_wall[i][j].north) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             jtp_map_light[i-1][j], cur_tile->graphic);


            /* 2. Furniture*/
            if ((cur_tile = jtp_tiles[jtp_map_furniture[i][j]]) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);


            /* 3. Traps */
            if ((cur_tile = jtp_tiles[jtp_map_trap[i][j]]) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);


            /* 4. Objects */
            if ((cur_tile = jtp_tiles[jtp_map_obj[i][j]]) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);


            /* 5. Monsters */
            if ((cur_tile = jtp_tiles[jtp_map_mon[i][j]]) != NULL)
            {
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);
                if (iflags.hilite_pet && (jtp_map_specialattr[i][j] & MG_PET))
                    jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                                 lightlevel, jtp_tiles[V_TILE_HILITE_PET]->graphic);
            }

            /* 6. Effects */
            if ((cur_tile = jtp_tiles[jtp_map_specialeff[i][j]]) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             lightlevel, cur_tile->graphic);

            /* 7. South & East walls */
            if ((cur_tile = jtp_maptile_wall[i][j].south) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             jtp_map_light[i+1][j], cur_tile->graphic);
            if ((cur_tile = jtp_maptile_wall[i][j].east) != NULL)
                jtp_put_tile(x + cur_tile->xmod, y + cur_tile->ymod,
                             jtp_map_light[i][j+1], cur_tile->graphic);
        }
    }

    /* Restore drawing region */
    jtp_set_draw_region(0, 0, jtp_screen.width-1, jtp_screen.height-1);

    /* Draw mini-map to the left of the status area */
    jtp_draw_mini_map();

    /* The old message background area is now invalid, so make sure it isn't used. */
    free(jtp_messages_background);
    jtp_messages_background = NULL;
}



static void jtp_init_floor_decors(int num_decors)
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
    for (i = 0; i < JTP_MAP_HEIGHT; i++)
    {
        memset(jtp_room_indices[i], 0, JTP_MAP_WIDTH);
        memset(jtp_map_deco[i], 0, JTP_MAP_WIDTH);
    }

    /* init room indices */
    for (i = 0; i < nroom; i++)
        for (j = rooms[i].ly; j <= rooms[i].hy; j++)
            memset(&jtp_room_indices[j][rooms[i].lx], (i+1), (rooms[i].hx-rooms[i].lx+1));

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
            while (ly <= rooms[roomno].hy && (old_deco = (jtp_map_deco[ly][lx] >> 4)) != 0)
            {
                while (lx <= rooms[roomno].hx && (old_deco2 = (jtp_map_deco[ly][lx] >> 4)) != 0)
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
                        jtp_map_deco[ly+j][lx+k] = ((current_deco+1) << 4) + (j*deco_width+k);
            }
        }

        if (!retries && !placed)
            /* placing this one failed, so trying to place others is futile  */
            break;

        roomno += 5;
        if (roomno > nroom)
            roomno = (roomno % nroom) + wrapadd;
    }

    /* center the decorations in the rooms, as the previous code always plces them in the top corner*/
    for (i = 0; i < nroom; i++)
    {
        lx = rooms[i].lx;
        ly = rooms[i].ly;

        while (jtp_map_deco[ly][lx] != 0 && lx <= rooms[i].hx)
            lx++;

        deco_width = lx - rooms[i].lx;
        lx = rooms[i].lx;

        while (jtp_map_deco[ly][lx] != 0 && ly <= rooms[i].hy)
            ly++;

        deco_height = ly - rooms[i].ly;
        xoffset = (int)((rooms[i].lx + rooms[i].hx + 1)/2.0 - deco_width/2.0);
        yoffset = (int)((rooms[i].ly + rooms[i].hy + 1)/2.0 - deco_height/2.0);

        for (j = deco_height-1; j >= 0; j--)
            for (k = deco_width-1; k >= 0; k--)
                jtp_map_deco[yoffset + j][xoffset + k] = jtp_map_deco[rooms[i].ly + j][rooms[i].lx + k];

        for (j = rooms[i].ly; j < yoffset; j++)
            memset(&jtp_map_deco[j][rooms[i].lx], 0, (rooms[i].hx-rooms[i].lx+1));

        for (j = rooms[i].ly; j <= rooms[i].hy; j++)
            memset(&jtp_map_deco[j][rooms[i].lx], 0, (xoffset - rooms[i].lx));
    }
}



static void jtp_init_lights(int how_many)
{
    int i;

    jtp_nlights = how_many;

    for (i = 0; i < jtp_nlights; i++)
    {
        jtp_lights[i].x = rand()%(JTP_MAP_WIDTH - 1) + 1;  /* X location */
        jtp_lights[i].y = rand()%JTP_MAP_HEIGHT;           /* Y location */
        jtp_lights[i].radius = (rand()%20 + 1) / 10.0;         /* radius */
    }
}



static void jtp_calculate_lights(void)
{
    int i, j, k, dx, dy;
    double temp_dist;
    double temp_lightlevel;
    double lightlevel;

    /* The hero carries a small light */
    jtp_lights[0].x = u.ux;
    jtp_lights[0].y = u.uy;
    jtp_lights[0].radius = 1.0;

    for (i = JTP_MAP_HEIGHT-1; i >= 0; i--)
        for (j = JTP_MAP_WIDTH-1; j >= 1; j--)
        {
            lightlevel = JTP_AMBIENT_LIGHT;
            for (k = jtp_nlights-1; k >= 0; k--)
            {
                dx = j - jtp_lights[k].x;
                if (dx > 16 || dx < -16)
                    continue;
                dy = i - jtp_lights[k].y;
                if (dy > 16 || dy < -16)
                    continue;

                /* temp_dist is the square of the straight line distance between i,j and the
                 * light. We subtract temp_dist directly without taking the root, because
                 * that gives us exponential dropoff.
                 * The scaling factor radius*5 brings temp_lightlevel down to ambient light level
                 * after about 16 squares */
                temp_dist = ((dx * dx) + (dy * dy));
                temp_lightlevel = JTP_MAX_SHADES - temp_dist/(jtp_lights[k].radius*5);
                lightlevel = (lightlevel > temp_lightlevel) ? lightlevel : temp_lightlevel;
            }

            if (lightlevel > JTP_MAX_SHADES-1)
                lightlevel = JTP_MAX_SHADES-1;
            jtp_map_light[i][j] = lightlevel;
        }
}



void jtp_map_to_screen(int map_x, int map_y, int *screen_x, int *screen_y)
{
    map_x -= jtp_map_x;
    map_y -= jtp_map_y;
    *screen_x = jtp_map_center_x + JTP_MAP_XMOD*(map_x - map_y);
    *screen_y = jtp_map_center_y + JTP_MAP_YMOD*(map_x + map_y);
}


char *jtp_map_square_description(
    int tgt_x, int tgt_y,
    int include_seen
)
{
    struct permonst *pm;
    char   *out_str, look_buf[BUFSZ];
    char   temp_buf[BUFSZ], coybuf[QBUFSZ];  
    char   monbuf[BUFSZ];
    struct monst *mtmp = (struct monst *) 0;
    const char *firstmatch;

    if ((tgt_x < 1) || (tgt_x >= JTP_MAP_WIDTH) || (tgt_y < 0) || (tgt_y >= JTP_MAP_HEIGHT))
       return(NULL);

    /* All of monsters, objects, traps and furniture get descriptions */
    if ((jtp_map_mon[tgt_y][tgt_x] != V_TILE_NONE) ||
        (jtp_map_obj[tgt_y][tgt_x] != V_TILE_NONE) ||
        (jtp_map_trap[tgt_y][tgt_x] != V_TILE_NONE) ||
        (jtp_map_furniture[tgt_y][tgt_x] != V_TILE_NONE))
    {
        out_str = malloc(BUFSZ);
        out_str[0] = '\0';

        look_buf[0] = '\0';
        monbuf[0] = '\0';
        pm = (struct permonst *)lookat(tgt_x, tgt_y, look_buf, monbuf);
        firstmatch = look_buf;
        if (*firstmatch)
        {
            mtmp = m_at(tgt_x,tgt_y);
            Sprintf(temp_buf, "%s", (pm == &mons[PM_COYOTE]) ? coyotename(mtmp,coybuf) : firstmatch);
            strncat(out_str, temp_buf, BUFSZ-strlen(out_str)-1);
        }
        if (include_seen)
        {
            if (monbuf[0])
            {
                Sprintf(temp_buf, " [seen: %s]", monbuf);
                strncat(out_str, temp_buf, BUFSZ-strlen(out_str)-1);
            }
        }
        return(out_str);
    }
    return(NULL);
}



static void jtp_get_wall_tiles(int y, int x)
{
    int style;

    if (!level.locations[x][y].seenv)
        return;

    /* x - 1: west wall  */
    if (x > 0 && jtp_map_back[y][x - 1] != V_TILE_WALL_GENERIC && jtp_map_back[y][x - 1] != V_TILE_UNMAPPED_AREA)
    {
        style = jtp_get_wall_decor(jtp_map_back[y][x - 1], y, x, y, x-1);
        jtp_maptile_wall[y][x].west = jtp_tiles[walltiles[style].west];
    }
    else
        jtp_maptile_wall[y][x].west = NULL;

    /* y - 1: north wall  */
    if (y > 0 && jtp_map_back[y - 1][x] != V_TILE_WALL_GENERIC && jtp_map_back[y - 1][x] != V_TILE_UNMAPPED_AREA)
    {
        style = jtp_get_wall_decor(jtp_map_back[y - 1][x], y, x, y - 1, x);
        jtp_maptile_wall[y][x].north = jtp_tiles[walltiles[style].north];
    }
    else
        jtp_maptile_wall[y][x].north = NULL;

    /* x + 1: east wall  */
    if (x < JTP_MAP_WIDTH - 1 && jtp_map_back[y][x + 1] != V_TILE_WALL_GENERIC &&
        jtp_map_back[y][x + 1] != V_TILE_UNMAPPED_AREA)
    {
        style = jtp_get_wall_decor(jtp_map_back[y][x + 1], y, x, y, x + 1);
        jtp_maptile_wall[y][x].east = jtp_tiles[walltiles[style].east];
    }
    else
        jtp_maptile_wall[y][x].east = NULL;

    /* y + 1: south wall  */
    if (y < JTP_MAP_HEIGHT - 1 && jtp_map_back[y + 1][x] != V_TILE_WALL_GENERIC &&
        jtp_map_back[y + 1][x] != V_TILE_UNMAPPED_AREA)
    {
        style = jtp_get_wall_decor(jtp_map_back[y + 1][x], y, x, y + 1, x);
        jtp_maptile_wall[y][x].south = jtp_tiles[walltiles[style].south];
    }
    else
        jtp_maptile_wall[y][x].south = NULL;
}


void jtp_clear_walls(int y, int x)
{
    jtp_maptile_wall[y][x].north = NULL;
    jtp_maptile_wall[y][x].east = NULL;
    jtp_maptile_wall[y][x].south = NULL;
    jtp_maptile_wall[y][x].west = NULL;
}


static jtp_tile *jtp_get_floor_tile(int tile, int y, int x)
{
    int style;
    int deco_pos;
    unsigned char deco = jtp_map_deco[y][x];

    if (deco && tile == V_TILE_FLOOR_COBBLESTONE)
    {
        style = (deco >> 4) - 1;
        deco_pos = (int)(deco & 0x0F);
        return jtp_tiles[floorstyles[style].array[deco_pos]];
    }

    style = jtp_get_floor_decor(tile, y, x);
    deco_pos = floorstyles[style].x * (y % floorstyles[style].y) + (x % floorstyles[style].x);
    return jtp_tiles[floorstyles[style].array[deco_pos]];
}



static void jtp_get_floor_edges(int y, int x)
{
    int tile = jtp_map_back[y][x];
    int style = V_FLOOR_EDGE_COBBLESTONE;

   /* Default: no floor edges around tile */
    jtp_clear_floor_edges(y, x);

    /* straight sections */
    if (x > 0 && tile != jtp_map_back[y][x-1] &&
       (tile + jtp_map_back[y][x-1]) != (V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE)) /* this prevents borders between water and ice*/
        jtp_maptile_floor_edge[y][x].west = jtp_tiles[flooredges[style].west];

    if (y > 0 && tile != jtp_map_back[y-1][x] &&
       (tile + jtp_map_back[y-1][x]) != (V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE))
        jtp_maptile_floor_edge[y][x].north = jtp_tiles[flooredges[style].north];

    if (x < JTP_MAP_WIDTH - 1 && tile != jtp_map_back[y][x+1] &&
       (tile + jtp_map_back[y][x+1]) != (V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE))
        jtp_maptile_floor_edge[y][x].east = jtp_tiles[flooredges[style].east];

    if (y < JTP_MAP_HEIGHT - 1 && tile != jtp_map_back[y+1][x] &&
       (tile + jtp_map_back[y+1][x]) != (V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE))
        jtp_maptile_floor_edge[y][x].south = jtp_tiles[flooredges[style].south];

    /* "inward pointing" corners */
    if ((jtp_maptile_floor_edge[y][x].west) && (jtp_maptile_floor_edge[y][x].north))
        jtp_maptile_floor_edge[y][x].northwest = jtp_tiles[flooredges[style].northwest];

    if ((jtp_maptile_floor_edge[y][x].west) && (jtp_maptile_floor_edge[y][x].south))
        jtp_maptile_floor_edge[y][x].southwest = jtp_tiles[flooredges[style].southwest];

    if ((jtp_maptile_floor_edge[y][x].east) && (jtp_maptile_floor_edge[y][x].south))
        jtp_maptile_floor_edge[y][x].southeast = jtp_tiles[flooredges[style].southeast];

    if ((jtp_maptile_floor_edge[y][x].east) && (jtp_maptile_floor_edge[y][x].north))
        jtp_maptile_floor_edge[y][x].northeast = jtp_tiles[flooredges[style].northeast];

    /* "outward pointing" corners */
    if ((!jtp_maptile_floor_edge[y][x].west) && (!jtp_maptile_floor_edge[y][x].north) &&
        x > 0 && y > 0 && tile != jtp_map_back[y-1][x-1] &&
        (tile + jtp_map_back[y-1][x-1]) != (V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE))
        jtp_maptile_floor_edge[y][x].northwest_bank = jtp_tiles[flooredges[style].northwest_bank];

    if ((!jtp_maptile_floor_edge[y][x].east) && (!jtp_maptile_floor_edge[y][x].north) &&
        x < JTP_MAP_WIDTH-1 && y > 0 && tile != jtp_map_back[y-1][x+1] &&
        (tile + jtp_map_back[y-1][x+1]) != (V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE))
        jtp_maptile_floor_edge[y][x].northeast_bank = jtp_tiles[flooredges[style].northeast_bank];

    if ((!jtp_maptile_floor_edge[y][x].west) && (!jtp_maptile_floor_edge[y][x].south) &&
        x > 0 && y < JTP_MAP_HEIGHT-1 && tile != jtp_map_back[y+1][x-1] &&
        (tile + jtp_map_back[y+1][x-1]) != (V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE))
        jtp_maptile_floor_edge[y][x].southwest_bank = jtp_tiles[flooredges[style].southwest_bank];

    if ((!jtp_maptile_floor_edge[y][x].east) && (!jtp_maptile_floor_edge[y][x].south) &&
        x < JTP_MAP_WIDTH-1 && y < JTP_MAP_HEIGHT-1 && tile != jtp_map_back[y+1][x+1] &&
        (tile + jtp_map_back[y+1][x+1]) != (V_TILE_FLOOR_WATER + V_TILE_FLOOR_ICE))
        jtp_maptile_floor_edge[y][x].southeast_bank = jtp_tiles[flooredges[style].southeast_bank];
}



static void jtp_clear_floor_edges(int y, int x)
{
    jtp_maptile_floor_edge[y][x].west = NULL;
    jtp_maptile_floor_edge[y][x].north = NULL;
    jtp_maptile_floor_edge[y][x].east = NULL;
    jtp_maptile_floor_edge[y][x].south = NULL;
    jtp_maptile_floor_edge[y][x].northwest = NULL;
    jtp_maptile_floor_edge[y][x].northeast = NULL;
    jtp_maptile_floor_edge[y][x].southwest = NULL;
    jtp_maptile_floor_edge[y][x].southeast = NULL;
    jtp_maptile_floor_edge[y][x].northwest_bank = NULL;
    jtp_maptile_floor_edge[y][x].northeast_bank = NULL;
    jtp_maptile_floor_edge[y][x].southwest_bank = NULL;
    jtp_maptile_floor_edge[y][x].southeast_bank = NULL;						
}



/* get room index is only used to semi-randomly select room decorations
 * therefore the number we return can be as bogus as we want, so long as
 * it's consistently the same. Using the current depth will provide a bit of
 * variety on the maze levels...*/
static int jtp_get_room_index(int x, int y)
{
    int rindex;
    if (nroom == 0) /* maze levels */
        return u.uz.dlevel;

    if (In_mines(&u.uz)) /* THe mies are a patchwork dungeon otherwise :( */
        return (u.uz.dlevel + nroom); /* cleverly prevent a repetitive sequence :P */

    rindex = jtp_room_indices[y][x];
    if (!rindex)
        return (u.uz.dlevel + nroom);

    return rindex;
}


/*
 * Convert wall tile index (ie. wall type) to an associated decoration style.
 */
static int jtp_get_wall_decor(
    int floortype,
    int wally, int wallx,
    int floory, int floorx
)
{
    int roomid;

#if defined(VULTURESEYE) || (defined(VULTURESCLAW) && defined(REINCARNATION))
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
            roomid = jtp_get_room_index(floorx, floory);
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
static int jtp_get_floor_decor(
  int floorstyle,
  int floory, int floorx
)
{
    int roomid;
#if defined(VULTURESEYE) || (defined(VULTURESCLAW) && defined(REINCARNATION))
    if (Is_rogue_level(&u.uz))
        return V_FLOOR_DARK;
#endif

    switch (floorstyle)
    {
        case V_TILE_FLOOR_ROUGH:     return V_FLOOR_ROUGH; 
        case V_TILE_FLOOR_ROUGH_LIT: return V_FLOOR_ROUGH_LIT;
        case V_TILE_FLOOR_COBBLESTONE:
        {
            roomid = jtp_get_room_index(floorx, floory);
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



static jtp_tile *jtp_make_transparent_player_tile(int monnum)
{
    static jtp_tile *tile;
    static int tilenum = -1;
    jtp_tile *montile;
    int x, y, w, h;
    unsigned char *dst;

    /*
     * monnum may change if player polymorphs...
     */
    if (monnum != tilenum)
    {
        if (tile != NULL)
        {
            free(tile->graphic);
            free(tile);
        }
        montile = jtp_tiles[MONSTER_TO_VTILE(monnum)];
        jtp_get_dimensions(montile->graphic, &w, &h);
        tile = malloc(sizeof(*tile));
        if (tile == NULL)
            return montile;
        *tile = *montile;
        tile->graphic = malloc((4 + w * h) * sizeof(unsigned char));
        if (tile->graphic == NULL)
        {
            free(tile);
            tile = NULL;
            return montile;
        }
        dst = tile->graphic;
        memcpy(dst, montile->graphic, (4 + w * h) * sizeof(unsigned char));
        dst += 4;
        for (y = 0; y < h; y++)
            for (x = y & 1; x < w; x += 2)
                dst[y * w + x] = JTP_COLOR_BACKGROUND;
        tilenum = monnum;
    }
    return tile;
}



static jtp_tile *jtp_make_invisible_player_tile(int monnum)
{
    static jtp_tile *tile;
    static int tilenum = -1;
    jtp_tile *montile;
    int x, y, w, h;
    unsigned char *dst;
    unsigned char *src;

    /*
     * monnum may change if player polymorphs...
     */
    if (monnum != tilenum)
    {
        if (tile != NULL)
        {
            free(tile->graphic);
            free(tile);
        }
        montile = jtp_tiles[MONSTER_TO_VTILE(monnum)];
        jtp_get_dimensions(montile->graphic, &w, &h);
        tile = malloc(sizeof(*tile));
        if (tile == NULL)
            return montile;
        *tile = *montile;
        tile->graphic = malloc((4 + w * h) * sizeof(unsigned char));
        if (tile->graphic == NULL)
        {
            free(tile);
            tile = NULL;
            return montile;
        }
        dst = tile->graphic;
        memcpy(dst, montile->graphic, 4 * sizeof(unsigned char));
        dst += 4;
        memset(dst, JTP_COLOR_BACKGROUND, w * h * sizeof(unsigned char));
        src = montile->graphic + 4;
        for (y = 0; y < h; y += 2)
            for (x = (y >> 1) & 1; x < w; x += 2)
                dst[y * w + x] = src[y * w + x];
        tilenum = monnum;
    }
    return tile;
}



void jtp_put_tile(
    int x, int y,
    int shade,
    unsigned char *a
)
{
    int srcXsize, srcYsize, j, start_y, end_y, start_x, end_x;
    int dplus;
    unsigned char *destin, *a_end, *shades;

    if ((!a) || (x > jtp_screen.drx2) || (y > jtp_screen.dry2))
       return;

    jtp_get_dimensions(a, &srcXsize, &srcYsize);

    if ((x + srcXsize <= jtp_screen.drx1) || (y + srcYsize <= jtp_screen.dry1))
        return;

    if (y < jtp_screen.dry1)
        start_y = jtp_screen.dry1 - y;
    else
        start_y = 0;

    if (y + srcYsize - 1 > jtp_screen.dry2)
        end_y = jtp_screen.dry2 - y;
    else
        end_y = srcYsize - 1;

    if (x < jtp_screen.drx1)
        start_x = jtp_screen.drx1 - x;
    else
        start_x = 0;

    if (x + srcXsize - 1 > jtp_screen.drx2)
        end_x = jtp_screen.drx2 - x;
    else
        end_x = srcXsize - 1;

    a += start_y * srcXsize + 4;
    a_end = a + (end_y - start_y + 1) * srcXsize;
    destin = jtp_screen.vpage + (start_y + y) * jtp_screen.width + x;
    dplus = jtp_screen.width;

    a += start_x;
    a_end += start_x;
    destin += start_x;
    end_x -= start_x;
    start_x = jtp_screen.width;
    shades = jtp_shade + 256*shade;

    while (a < a_end)
    {
        for (j = end_x; j >= 0; j--)
        {
            if (a[j] != JTP_COLOR_BACKGROUND)
                destin[j] = shades[a[j]];
        }
        a += srcXsize;  
        destin += dplus;
    }
}


/* Mini-map */
static void jtp_draw_mini_map(void)
{
    int i, j, k, l, m, n, n_start, n_end;
    int token_x, token_y;

    /* Draw mini-map in lower left corner */
    jtp_set_draw_region(jtp_statusbar_x, jtp_statusbar_y,
                        jtp_statusbar_x+193, jtp_screen.height-1);
    jtp_put_img(jtp_statusbar_x, jtp_statusbar_y, jtp_statusbar);
    jtp_set_draw_region(0, 0, jtp_screen.width-1, jtp_screen.height-1);


    for (i = 0; i < JTP_MAP_HEIGHT; i++)
        for (j = 1; j < JTP_MAP_WIDTH; j++)
        {
            l = JTP_COLOR_BACKGROUND;
            k = jtp_map_back[i][j];
            /* Select a color for this glyph */
            switch(k)
            {
                case V_TILE_WALL_GENERIC:
                case V_TILE_UNMAPPED_AREA:
                case V_TILE_NONE:
                    l = JTP_COLOR_BACKGROUND; break;
                case V_TILE_FLOOR_ROUGH: case V_TILE_FLOOR_ROUGH_LIT:
                    l = JTP_COLOR_MINI_CORRIDOR; break;
                case V_TILE_STAIRS_UP: case V_TILE_STAIRS_DOWN:
                    l = JTP_COLOR_MINI_STAIRS; break;
                case V_TILE_VDOOR_WOOD_OPEN: case V_TILE_VDOOR_WOOD_CLOSED:
                case V_TILE_HDOOR_WOOD_OPEN: case V_TILE_HDOOR_WOOD_CLOSED:
                case V_TILE_VODBRIDGE: case V_TILE_HODBRIDGE: case V_TILE_VCDBRIDGE: case V_TILE_HCDBRIDGE:
                    l = JTP_COLOR_MINI_DOOR; break;
                default:
                    l = JTP_COLOR_MINI_FLOOR; break;
            }
            if ((i == u.uy) && (j == u.ux)) l = JTP_COLOR_MINI_YOU;

            if (l != JTP_COLOR_BACKGROUND)
            {
                token_x = jtp_statusbar_x + 94 + 2*(j-jtp_map_x) - 2*(i-jtp_map_y);
                token_y = jtp_statusbar_y + 51 + 1*(j-jtp_map_x) + 1*(i-jtp_map_y);
                for (m = 0; m < 2; m++)
                {
                    if (m == 0) { n_start = 0; n_end = 0; }
                    else { n_start = -1; n_end = 1; }
                    for (n = n_start; n <= n_end; n++)
                    {
                        if ((token_x + n > jtp_statusbar_x + 4) &&
                            (token_x + n < jtp_statusbar_x + 193) &&
                            (token_y + m > jtp_statusbar_y + 4) &&
                            (token_y + m < jtp_statusbar_y + 96) &&
                            (jtp_screen.vpage[(token_y+m)*jtp_screen.width+(token_x+n)] <= 188) &&
                            (jtp_screen.vpage[(token_y+m)*jtp_screen.width+(token_x+n)] >= 180))
                            jtp_screen.vpage[(token_y+m)*jtp_screen.width+(token_x+n)] = l;
                    }
                }
            }
        }
}


/* Map drawing functions */
void jtp_view_map(void)
{
    int x, y;
    int map_x, map_y;
    int i, j;
    unsigned char * parchment_bg;
    char * temp_tooltip;
    int n_hotspots;
    jtp_hotspot ** map_hotspots;

    /* Find upper left corner of parchment */
    x = (jtp_screen.width - 640)/2;
    y = (jtp_screen.height - 480)/2;

    parchment_bg = jtp_get_img(x, y, x + 640-1, y + 480-1);

    /* Draw parchment */
    jtp_put_stencil(x, y, jtp_map_parchment_top);
    jtp_put_stencil(x, y + 480 - jtp_map_parchment_bottom[1], jtp_map_parchment_bottom);
    jtp_put_stencil(x, y, jtp_map_parchment_left);
    jtp_put_stencil(x + 640 - jtp_map_parchment_right[3], y, jtp_map_parchment_right);
    jtp_put_img(x + jtp_map_parchment_left[3], y + jtp_map_parchment_top[1], jtp_map_parchment_center);

    /* Find upper left corner of map on parchment */
    map_x = x + 39;
    map_y = y + 91;

    /* Draw map on parchment, and create hotspots */
    for (i = 0; i < JTP_MAP_HEIGHT; i++)
        for (j = 1; j < JTP_MAP_WIDTH; j++)
        {
            if (jtp_map_glyph[i][j] != NO_GLYPH && levl[j][i].typ != STONE && (jtp_map_glyph[i][j] != cmap_to_glyph(S_stone) || (levl[j][i].seenv && jtp_map_tile_is_dark[i][j]))) {
                jtp_put_tile(
                        map_x+VULTURES_MAP_SYMBOL_WIDTH*j,
                        map_y+VULTURES_MAP_SYMBOL_HEIGHT*i,
                        63, jtp_map_symbols[glyph2tile[jtp_map_glyph[i][j]]]); 
            }
        }

    /* Create hotspots */
    n_hotspots = 0;
    map_hotspots = NULL;
    for (i = 0; i < JTP_MAP_HEIGHT; i++)
        for (j = 1; j < JTP_MAP_WIDTH; j++)
        {
            temp_tooltip = jtp_choose_target_tooltip(j, i);
            if (temp_tooltip)
            {
                n_hotspots++;
                map_hotspots = (jtp_hotspot **)realloc(map_hotspots, n_hotspots*sizeof(jtp_hotspot *));
                map_hotspots[n_hotspots-1] = malloc(sizeof(jtp_hotspot));
                (map_hotspots[n_hotspots-1])->mcursor = jtp_mcursor[V_CURSOR_NORMAL];
                (map_hotspots[n_hotspots-1])->tooltip = temp_tooltip;
                (map_hotspots[n_hotspots-1])->x1 = map_x + 7*j;
                (map_hotspots[n_hotspots-1])->x2 = map_x + 7*j + 6;
                (map_hotspots[n_hotspots-1])->y1 = map_y + 14*i;
                (map_hotspots[n_hotspots-1])->y2 = map_y + 14*i + 13;
                (map_hotspots[n_hotspots-1])->accelerator = 0;
            }
        }

    jtp_refresh(&jtp_screen);
    /* Wait for mouse button release, then wait for another mouse button click */
    jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);
    jtp_get_mouse_inventory_input(jtp_mcursor[V_CURSOR_NORMAL], map_hotspots, n_hotspots, JTP_MBUTTON_LEFT);
    if (jtp_kbhit())
        jtp_getch();

    jtp_repeatmouse(jtp_mcursor[V_CURSOR_NORMAL], JTP_MBUTTON_NONE);

    /* Restore background and clean up */
    jtp_put_img(x, y, parchment_bg);
    jtp_refresh(&jtp_screen);
    free(parchment_bg);
    if (n_hotspots > 0)
    {
        for (i = 0; i < n_hotspots; i++)
        {
            free((map_hotspots[i])->tooltip);
            free(map_hotspots[i]);
        }
        free(map_hotspots);
    }
}

