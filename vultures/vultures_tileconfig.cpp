/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

extern "C" {
#include "hack.h"
}

#include "vultures_types.h"
#include "vultures_tile.h"
#include "vultures_tileconfig.h"
#include "vultures_gen.h"
#include "vultures_opt.h"


typedef struct {
    char *filename;
    int ptr_type, ptr_num;
    int hs_x, hs_y;
} tmp_tile;


char ** tilenames[NUM_TILETYPES];
int tmp_wallstyles[V_WALL_STYLE_MAX][8];
int tmp_flooredges[V_FLOOR_EDGE_MAX][12];
tmp_tile * tmp_gametiles[NUM_TILETYPES];
tmp_tile deftiles[NUM_TILETYPES];

/* things that get used by vultures later */
struct walls walls_full[V_WALL_STYLE_MAX];
struct walls walls_half[V_WALL_STYLE_MAX];
struct fedges flooredges[V_FLOOR_EDGE_MAX];
struct fstyles floorstyles[V_FLOOR_STYLE_MAX];
int vultures_typecount[NUM_TILETYPES];
int glassgems[CLR_MAX];

/* names for the various tiletypes */
static const char * typenames[NUM_TILETYPES];
static const char * floorstylenames[V_FLOOR_STYLE_MAX];
static const char * wallstylenames[V_WALL_STYLE_MAX];

static const char * edgestylenames[V_FLOOR_EDGE_MAX] = {
    /* [V_FLOOR_EDGE_COBBLESTONE] */ "COBBLESTONE"
};

/* the enum for the misc tiles starts at MISCTILEOFFSET so that the enumerated
 * names can be used diretly in the code; here however this means we need to
 * subtract MISCTILEOFFSET everywhere to get usable array indices */
static const char * miscnames[MISCTILECOUNT];

static const char * cursornames[V_CURSOR_MAX];


static void init_typenames();
static void init_floortilenames();
static void init_wallstylenames();
static void init_miscnames();
static void init_curnames();
static void init_objnames();
static void init_monnames();
static void init_explnames();


/* parse the configuration file and prepare the data arrays needed by the rest of vultures 
 * This function is the only one that should be called from outside */
void vultures_parse_tileconf(FILE *fp, struct gametiles **gt_ptr)
{
    int i, j, tilenum, loadnum;
    int typeoffset[NUM_TILETYPES];
    vultures_tile *tile;
    char messagebuf[512];
    struct gametiles * gametiles;



    /* init the names for all types of tiles */
    init_typenames();
    init_floortilenames();
    init_wallstylenames();
    init_miscnames();
    init_curnames();
    init_objnames(); /* init TT_OBJECT and TT_OBJICON */
    init_monnames(); /* init TT_MONSTER, TT_STATUE and TT_FIGURINE */
    init_explnames();/* TT_EXPL */

    vultures_typecount[TT_MISC] = MISCTILECOUNT;
    tilenames[TT_MISC] = (char**)miscnames;
    vultures_typecount[TT_CURSOR] = V_CURSOR_MAX;
    tilenames[TT_CURSOR] = (char**)cursornames;

    vultures_typecount[TT_WALL] = 0;
    tilenames[TT_WALL] = (char **)malloc(0);
    vultures_typecount[TT_FLOOR] = 0;
    tilenames[TT_FLOOR] = (char **)malloc(0);
    vultures_typecount[TT_EDGE] = 0;
    tilenames[TT_EDGE] = (char **)malloc(0);

    for (i = 0; i < NUM_TILETYPES; i++)
    {
        tmp_gametiles[i] = (tmp_tile *)malloc(vultures_typecount[i] * sizeof(tmp_tile));
        memset(tmp_gametiles[i], 0, vultures_typecount[i] * sizeof(tmp_tile));
    }

    /* tell the lexer about the input file and start the parser */
    yyrestart(fp);
    yyparse();

    /* the config has now been read and we know how many tiles of each type
     * there are, so offsets into the final tile array can be computed */
    typeoffset[0] = 0;
    for (i = 1; i < NUM_TILETYPES; i++)
        typeoffset[i] = typeoffset[i-1] + vultures_typecount[i-1];


    *gt_ptr = (struct gametiles *)malloc(GAMETILECOUNT * sizeof(struct gametiles));
    gametiles = *gt_ptr;
    memset(gametiles, 0, GAMETILECOUNT * sizeof(struct gametiles));

    /* build gametiles from the info in tmp_gametiles */
    for (i = 0; i < NUM_TILETYPES; i++)
    {
        for (j = 0; j < vultures_typecount[i]; j++)
        {
            tilenum = typeoffset[i] + j;

            /* use given tile */
            if (tmp_gametiles[i][j].filename && tmp_gametiles[i][j].ptr_type == -1 && tmp_gametiles[i][j].ptr_num == -1)
            {
                gametiles[tilenum].filename = strdup(tmp_gametiles[i][j].filename);
                gametiles[tilenum].ptr = -1;
                gametiles[tilenum].hs_x = tmp_gametiles[i][j].hs_x;
                gametiles[tilenum].hs_y = tmp_gametiles[i][j].hs_y;
            }
            /* redirect to another tile */
            else if (tmp_gametiles[i][j].ptr_type != 0 || tmp_gametiles[i][j].ptr_num != 0)
            {
                gametiles[tilenum].filename = NULL;
                gametiles[tilenum].ptr = typeoffset[tmp_gametiles[i][j].ptr_type] + tmp_gametiles[i][j].ptr_num;
                gametiles[tilenum].hs_x = 0;
                gametiles[tilenum].hs_y = 0;
            }
            /* use default tile */
            else if (!tmp_gametiles[i][j].filename &&
                      tmp_gametiles[i][j].ptr_type == 0 &&
                      tmp_gametiles[i][j].ptr_num == 0)
            {
                gametiles[tilenum].ptr = typeoffset[deftiles[i].ptr_type] + deftiles[i].ptr_num;
                gametiles[tilenum].hs_x = deftiles[i].hs_x;
                gametiles[tilenum].hs_y = deftiles[i].hs_y;
            }
        }
    }


    if (vultures_opts.debug)
    {
        /* validate all the tiles. this must be done in a separate loop as there may be
        * forward references which won't work until gametiles is fully initialized */
        for (i = 0; i < NUM_TILETYPES; i++)
        {
            for (j = 0; j < vultures_typecount[i]; j++)
            {
                tilenum = typeoffset[i] + j;
                loadnum = tilenum;
                if (!gametiles[tilenum].filename && gametiles[tilenum].ptr != -1)
                    loadnum = gametiles[tilenum].ptr;

                tile = vultures_load_tile(loadnum);
                if (!tile)
                {
                    if (gametiles[tilenum].filename)
                        snprintf(messagebuf, sizeof(messagebuf), "%s.%s: \"%s\" cannot be loaded",
                                typenames[i], tilenames[i][j], gametiles[tilenum].filename);
                    else
                        snprintf(messagebuf, sizeof(messagebuf), "%s.%s: invalid redirection",
                                typenames[i], tilenames[i][j]);

                    if (iflags.window_inited == TRUE)
                        pline(messagebuf);
                    else
                        printf("Tile config: %s\n", messagebuf);

                    vultures_write_log(V_LOG_NOTE, __FILE__, __LINE__, "Tile config: %s\n", messagebuf);
                }
                else
                {
                    SDL_FreeSurface(tile->graphic);
                    free(tile);
                }
            }
        }
    }

    /* special fixup for V_MISC_PLAYER_INVIS: it does not have a tile image,
     * nor should it redirect as it is handled specially */
    gametiles[V_MISC_PLAYER_INVIS].ptr = -1;


    /* free tilenames etc */
    for (i = 0; i < NUM_TILETYPES; i++)
    {
        for (j = 0; j < vultures_typecount[i]; j++)
        {
            if (i != TT_MISC && i != TT_CURSOR)
            {
                if (tilenames[i][j])
                    free(tilenames[i][j]);
                tilenames[i][j] = NULL;
            }

            if (tmp_gametiles[i][j].filename)
                free(tmp_gametiles[i][j].filename);
            tmp_gametiles[i][j].filename = NULL;
        }
        free(tmp_gametiles[i]);
    }

    free(tilenames[TT_OBJECT]);
    free(tilenames[TT_MONSTER]);
    free(tilenames[TT_EXPL]);
    free(tilenames[TT_WALL]);
    free(tilenames[TT_FLOOR]);
    free(tilenames[TT_EDGE]);


    /* build wall arrays */
    for (i = 0; i < V_WALL_STYLE_MAX; i++)
    {
        walls_full[i].west = tmp_wallstyles[i][0] + typeoffset[TT_WALL];
        walls_full[i].north = tmp_wallstyles[i][1] + typeoffset[TT_WALL];
        walls_full[i].south = tmp_wallstyles[i][2] + typeoffset[TT_WALL];
        walls_full[i].east = tmp_wallstyles[i][3] + typeoffset[TT_WALL];

        walls_half[i].west = tmp_wallstyles[i][4] + typeoffset[TT_WALL];
        walls_half[i].north = tmp_wallstyles[i][5] + typeoffset[TT_WALL];
        walls_half[i].south = tmp_wallstyles[i][6] + typeoffset[TT_WALL];
        walls_half[i].east = tmp_wallstyles[i][7] + typeoffset[TT_WALL];
    }


    for (i = 0; i < V_FLOOR_EDGE_MAX; i++)
        for (j = 0; j < 12; j++)
            flooredges[i].dir[j] = tmp_flooredges[i][j] + typeoffset[TT_EDGE];


    for (i = 0; i < V_FLOOR_STYLE_MAX; i++)
        for (j = 0; j < (floorstyles[i].x * floorstyles[i].y); j++)
            floorstyles[i].array[j] = floorstyles[i].array[j] + typeoffset[TT_FLOOR];
}


/* the following functions are called by the parser */

int vultures_get_tiletype_from_name(char* name)
{
    int i;
    for (i = 0; i < NUM_TILETYPES; i++)
        if (strcmp(name, typenames[i]) == 0)
            return i;

    yyerror("invalid tile type");
    return -1;
}


int vultures_get_wallstyle_from_name(char *name)
{
    int i;
    for (i = 0; i < V_WALL_STYLE_MAX; i++)
        if (strcmp(name, wallstylenames[i]) == 0)
            return i;

    yyerror("invalid wall style name");
    return -1;
}


int vultures_get_edgestyle_from_name(char *name)
{
    int i;
    for (i = 0; i < V_FLOOR_EDGE_MAX; i++)
        if (strcmp(name, edgestylenames[i]) == 0)
            return i;

    yyerror("invalid edge style name");
    return -1;
}


int vultures_get_floorstyle_from_name(char *name)
{
    int i;
    for (i = 0; i < V_FLOOR_STYLE_MAX; i++)
        if (strcmp(name, floorstylenames[i]) == 0)
            return i;

    yyerror("invalid floor style name");
    return -1;
}


int vultures_get_tile_index(int type, char * name, int allow_expand)
{
    int i, index;

    if (type >= NUM_TILETYPES)
        yyerror("invalid tile type");

    index = -1;
    for (i = 0; i < vultures_typecount[type]; i++)
        if (strcmp(name, tilenames[type][i]) == 0)
            index = i;

    if (index == -1) {
        if ((type == TT_WALL || type == TT_FLOOR || type == TT_EDGE) && allow_expand)
        {
            index = vultures_typecount[type];

            vultures_typecount[type]++;
            tilenames[type] = (char **)realloc(tilenames[type], vultures_typecount[type] * sizeof(char*));
            tmp_gametiles[type] = (tmp_tile *)realloc(tmp_gametiles[type], vultures_typecount[type] * sizeof(tmp_tile));
            memset(&tmp_gametiles[type][index], 0, sizeof(tmp_tile));

            tilenames[type][index] = strdup(name);
        }
    }

    return index;
}


void vultures_add_tile(int tiletype, char *tilename, char *filename, int xoffset, int yoffset)
{
    int tilenum = vultures_get_tile_index(tiletype, tilename, 1);

    if (tilenum == -1)
        return;

    tmp_gametiles[tiletype][tilenum].filename = strdup(filename);
    tmp_gametiles[tiletype][tilenum].ptr_type = -1;
    tmp_gametiles[tiletype][tilenum].ptr_num = -1;
    tmp_gametiles[tiletype][tilenum].hs_x = xoffset;
    tmp_gametiles[tiletype][tilenum].hs_y = yoffset;
}


void vultures_redirect_tile(int tiletype, char *tilename, int redir_tiletype, char *redir_tilename)
{
    int src, dst;

    dst = vultures_get_tile_index(redir_tiletype, redir_tilename, 1);
    if (dst == -1)
        return;

    if (strcmp("default", tilename) == 0)
    {
        deftiles[tiletype].filename = NULL;
        deftiles[tiletype].ptr_type = redir_tiletype;
        deftiles[tiletype].ptr_num = dst;
        deftiles[tiletype].hs_x = 0;
        deftiles[tiletype].hs_y = 0;
    }
    else
    {
        src = vultures_get_tile_index(tiletype, tilename, 1);
        if (src == -1)
            return;

        tmp_gametiles[tiletype][src].filename = NULL;
        tmp_gametiles[tiletype][src].ptr_type = redir_tiletype;
        tmp_gametiles[tiletype][src].ptr_num = dst;
        tmp_gametiles[tiletype][src].hs_x = 0;
        tmp_gametiles[tiletype][src].hs_y = 0;
    }
}


void vultures_setup_wallstyle(int style_id, char * full_w, char * full_n, char * full_s, char * full_e,
                                   char * half_w, char * half_n, char * half_s, char * half_e)
{
    int i;

    tmp_wallstyles[style_id][0] = vultures_get_tile_index(TT_WALL, full_w, 0);
    tmp_wallstyles[style_id][1] = vultures_get_tile_index(TT_WALL, full_n, 0);
    tmp_wallstyles[style_id][2] = vultures_get_tile_index(TT_WALL, full_s, 0);
    tmp_wallstyles[style_id][3] = vultures_get_tile_index(TT_WALL, full_e, 0);
    tmp_wallstyles[style_id][4] = vultures_get_tile_index(TT_WALL, half_w, 0);
    tmp_wallstyles[style_id][5] = vultures_get_tile_index(TT_WALL, half_n, 0);
    tmp_wallstyles[style_id][6] = vultures_get_tile_index(TT_WALL, half_s, 0);
    tmp_wallstyles[style_id][7] = vultures_get_tile_index(TT_WALL, half_e, 0);

    for (i = 0; i < 8; i++)
        if (tmp_wallstyles[style_id][i] == -1)
            yyerror("all given walltiles must be defined");
}


void vultures_setup_edgestyle(int style_id, char *edge01, char *edge02, char *edge03, char *edge04,
                                   char *edge05, char *edge06, char *edge07, char *edge08,
                                   char *edge09, char *edge10, char *edge11, char *edge12)
{
    int i;

    tmp_flooredges[style_id][0] = vultures_get_tile_index(TT_EDGE, edge01, 0);
    tmp_flooredges[style_id][1] = vultures_get_tile_index(TT_EDGE, edge02, 0);
    tmp_flooredges[style_id][2] = vultures_get_tile_index(TT_EDGE, edge03, 0);
    tmp_flooredges[style_id][3] = vultures_get_tile_index(TT_EDGE, edge04, 0);
    tmp_flooredges[style_id][4] = vultures_get_tile_index(TT_EDGE, edge05, 0);
    tmp_flooredges[style_id][5] = vultures_get_tile_index(TT_EDGE, edge06, 0);
    tmp_flooredges[style_id][6] = vultures_get_tile_index(TT_EDGE, edge07, 0);
    tmp_flooredges[style_id][7] = vultures_get_tile_index(TT_EDGE, edge08, 0);
    tmp_flooredges[style_id][8] = vultures_get_tile_index(TT_EDGE, edge09, 0);
    tmp_flooredges[style_id][9] = vultures_get_tile_index(TT_EDGE, edge10, 0);
    tmp_flooredges[style_id][10] = vultures_get_tile_index(TT_EDGE, edge11, 0);
    tmp_flooredges[style_id][11] = vultures_get_tile_index(TT_EDGE, edge12, 0);

    for (i = 0; i < 12; i++)
        if (tmp_flooredges[style_id][i] == -1)
            yyerror("all given edge tiles must be defined");
}


void vultures_setup_floorstyle(int style_id, int x, int y, int *tilearray)
{
    floorstyles[style_id].x = x;
    floorstyles[style_id].y = y;
    floorstyles[style_id].array = tilearray;
}





static void init_typenames()
{
    typenames[TT_OBJECT] = "object";
    typenames[TT_OBJICON] = "objicon";
    typenames[TT_MONSTER] = "monster";
    typenames[TT_STATUE] = "statue";
    typenames[TT_FIGURINE] = "figurine";
    typenames[TT_MISC] = "misc";
    typenames[TT_EXPL] = "explosion";
    typenames[TT_EDGE] = "edge";
    typenames[TT_FLOOR] = "floor";
    typenames[TT_WALL] = "wall";
    typenames[TT_CURSOR] = "cursor";
}


static void init_floortilenames()
{
    floorstylenames[V_FLOOR_COBBLESTONE] = "COBBLESTONE";
    floorstylenames[V_FLOOR_ROUGH] = "ROUGH";
    floorstylenames[V_FLOOR_CERAMIC] = "CERAMIC";
    floorstylenames[V_FLOOR_LAVA] = "LAVA";
    floorstylenames[V_FLOOR_WATER] = "WATER";
    floorstylenames[V_FLOOR_ICE] = "ICE";
    floorstylenames[V_FLOOR_MURAL] = "MURAL";
    floorstylenames[V_FLOOR_MURAL2] = "MURAL2";
    floorstylenames[V_FLOOR_CARPET] = "CARPET";
    floorstylenames[V_FLOOR_MOSS_COVERED] = "MOSS_COVERED";
    floorstylenames[V_FLOOR_MARBLE] = "MARBLE";
    floorstylenames[V_FLOOR_ROUGH_LIT] = "ROUGH_LIT";
    floorstylenames[V_FLOOR_AIR] = "AIR";
    floorstylenames[V_FLOOR_DARK] = "DARK";
}


static void init_wallstylenames()
{
    wallstylenames[V_WALL_BRICK] = "BRICK";
    wallstylenames[V_WALL_BRICK_BANNER] = "BRICK_BANNER";
    wallstylenames[V_WALL_BRICK_PAINTING] = "BRICK_PAINTING";
    wallstylenames[V_WALL_BRICK_POCKET] = "BRICK_POCKET";
    wallstylenames[V_WALL_BRICK_PILLAR] = "BRICK_PILLAR";
    wallstylenames[V_WALL_MARBLE] = "MARBLE";
    wallstylenames[V_WALL_VINE_COVERED] = "VINE_COVERED";
    wallstylenames[V_WALL_STUCCO] = "STUCCO";
    wallstylenames[V_WALL_ROUGH] = "ROUGH";
    wallstylenames[V_WALL_DARK] = "DARK";
    wallstylenames[V_WALL_LIGHT] = "LIGHT";
}


static void init_curnames()
{
    cursornames[V_CURSOR_NORMAL] = "NORMAL";
    cursornames[V_CURSOR_SCROLLRIGHT] = "SCROLLRIGHT";
    cursornames[V_CURSOR_SCROLLLEFT] = "SCROLLLEFT";
    cursornames[V_CURSOR_SCROLLUP] = "SCROLLUP";
    cursornames[V_CURSOR_SCROLLDOWN] = "SCROLLDOWN";
    cursornames[V_CURSOR_SCROLLUPLEFT] = "SCROLLUPLEFT";
    cursornames[V_CURSOR_SCROLLUPRIGHT] = "SCROLLUPRIGHT";
    cursornames[V_CURSOR_SCROLLDOWNLEFT] = "SCROLLDOWNLEFT";
    cursornames[V_CURSOR_SCROLLDOWNRIGHT] = "SCROLLDOWNRIGHT";
    cursornames[V_CURSOR_TARGET_GREEN] = "TARGET_GREEN";
    cursornames[V_CURSOR_TARGET_RED] = "TARGET_RED";
    cursornames[V_CURSOR_TARGET_INVALID] = "TARGET_INVALID";
    cursornames[V_CURSOR_TARGET_HELP] = "TARGET_HELP";
    cursornames[V_CURSOR_HOURGLASS] = "HOURGLASS";
    cursornames[V_CURSOR_OPENDOOR] = "OPENDOOR";
    cursornames[V_CURSOR_STAIRS] = "STAIRS";
    cursornames[V_CURSOR_GOBLET] = "GOBLET";
}


static void init_miscnames()
{
    miscnames[V_MISC_PLAYER_INVIS - MISCTILEOFFSET] = ""; /* make it impossible to assign a tile to V_MISC_PLAYER_INVIS */
    miscnames[V_MISC_FLOOR_NOT_VISIBLE - MISCTILEOFFSET] = "FLOOR_NOT_VISIBLE";
    miscnames[V_MISC_DOOR_WOOD_BROKEN - MISCTILEOFFSET] = "DOOR_WOOD_BROKEN";
    miscnames[V_MISC_HDOOR_WOOD_CLOSED - MISCTILEOFFSET] = "HDOOR_WOOD_CLOSED";
    miscnames[V_MISC_VDOOR_WOOD_CLOSED - MISCTILEOFFSET] = "VDOOR_WOOD_CLOSED";
    miscnames[V_MISC_VDOOR_WOOD_OPEN - MISCTILEOFFSET] = "VDOOR_WOOD_OPEN";
    miscnames[V_MISC_HDOOR_WOOD_OPEN - MISCTILEOFFSET] = "HDOOR_WOOD_OPEN";
    miscnames[V_MISC_TRAP_BEAR - MISCTILEOFFSET] = "TRAP_BEAR";
    miscnames[V_MISC_GRAVE - MISCTILEOFFSET] = "GRAVE";
    miscnames[V_MISC_ALTAR - MISCTILEOFFSET] = "ALTAR";
    miscnames[V_MISC_FOUNTAIN - MISCTILEOFFSET] = "FOUNTAIN";
    miscnames[V_MISC_STAIRS_UP - MISCTILEOFFSET] = "STAIRS_UP";
    miscnames[V_MISC_STAIRS_DOWN - MISCTILEOFFSET] = "STAIRS_DOWN";
    miscnames[V_MISC_SINK - MISCTILEOFFSET] = "SINK";
    miscnames[V_MISC_GAS_TRAP - MISCTILEOFFSET] = "GAS_TRAP";
    miscnames[V_MISC_TRAP_PIT - MISCTILEOFFSET] = "TRAP_PIT";
    miscnames[V_MISC_TRAP_POLYMORPH - MISCTILEOFFSET] = "TRAP_POLYMORPH";
    miscnames[V_MISC_TREE - MISCTILEOFFSET] = "TREE";
    miscnames[V_MISC_TRAP_MAGIC - MISCTILEOFFSET] = "TRAP_MAGIC";
    miscnames[V_MISC_TRAP_DOOR - MISCTILEOFFSET] = "TRAP_DOOR";
    miscnames[V_MISC_TRAP_WATER - MISCTILEOFFSET] = "TRAP_WATER";
    miscnames[V_MISC_TRAP_TELEPORTER - MISCTILEOFFSET] = "TRAP_TELEPORTER";
    miscnames[V_MISC_UNMAPPED_AREA - MISCTILEOFFSET] = "UNMAPPED_AREA";
    miscnames[V_MISC_HILITE_PET - MISCTILEOFFSET] = "HILITE_PET";
    miscnames[V_MISC_BARS - MISCTILEOFFSET] = "BARS";
    miscnames[V_MISC_THRONE - MISCTILEOFFSET] = "THRONE";
    miscnames[V_MISC_TRAP_ANTI_MAGIC - MISCTILEOFFSET] = "TRAP_ANTI_MAGIC";
    miscnames[V_MISC_TRAP_ARROW - MISCTILEOFFSET] = "TRAP_ARROW";
    miscnames[V_MISC_TRAP_FIRE - MISCTILEOFFSET] = "TRAP_FIRE";
    miscnames[V_MISC_ROLLING_BOULDER_TRAP - MISCTILEOFFSET] = "ROLLING_BOULDER_TRAP";
    miscnames[V_MISC_TRAP_SLEEPGAS - MISCTILEOFFSET] = "TRAP_SLEEPGAS";
    miscnames[V_MISC_ZAP_SLANT_RIGHT - MISCTILEOFFSET] = "ZAP_SLANT_RIGHT";
    miscnames[V_MISC_ZAP_SLANT_LEFT - MISCTILEOFFSET] = "ZAP_SLANT_LEFT";
    miscnames[V_MISC_ZAP_HORIZONTAL - MISCTILEOFFSET] = "ZAP_HORIZONTAL";
    miscnames[V_MISC_ZAP_VERTICAL - MISCTILEOFFSET] = "ZAP_VERTICAL";
    miscnames[V_MISC_LADDER_UP - MISCTILEOFFSET] = "LADDER_UP";
    miscnames[V_MISC_LADDER_DOWN - MISCTILEOFFSET] = "LADDER_DOWN";
    miscnames[V_MISC_RESIST_SPELL_1 - MISCTILEOFFSET] = "RESIST_SPELL_1";
    miscnames[V_MISC_RESIST_SPELL_2 - MISCTILEOFFSET] = "RESIST_SPELL_2";
    miscnames[V_MISC_RESIST_SPELL_3 - MISCTILEOFFSET] = "RESIST_SPELL_3";
    miscnames[V_MISC_RESIST_SPELL_4 - MISCTILEOFFSET] = "RESIST_SPELL_4";
    miscnames[V_MISC_WEB_TRAP - MISCTILEOFFSET] = "WEB_TRAP";
    miscnames[V_MISC_DART_TRAP - MISCTILEOFFSET] = "DART_TRAP";
    miscnames[V_MISC_FALLING_ROCK_TRAP - MISCTILEOFFSET] = "FALLING_ROCK_TRAP";
    miscnames[V_MISC_SQUEAKY_BOARD - MISCTILEOFFSET] = "SQUEAKY_BOARD";
    miscnames[V_MISC_MAGIC_PORTAL - MISCTILEOFFSET] = "MAGIC_PORTAL";
    miscnames[V_MISC_SPIKED_PIT - MISCTILEOFFSET] = "SPIKED_PIT";
    miscnames[V_MISC_HOLE - MISCTILEOFFSET] = "HOLE";
    miscnames[V_MISC_LEVEL_TELEPORTER - MISCTILEOFFSET] = "LEVEL_TELEPORTER";
    miscnames[V_MISC_MAGIC_TRAP - MISCTILEOFFSET] = "MAGIC_TRAP";
    miscnames[V_MISC_DIGBEAM - MISCTILEOFFSET] = "DIGBEAM";
    miscnames[V_MISC_FLASHBEAM - MISCTILEOFFSET] = "FLASHBEAM";
    miscnames[V_MISC_BOOMLEFT - MISCTILEOFFSET] = "BOOMLEFT";
    miscnames[V_MISC_BOOMRIGHT - MISCTILEOFFSET] = "BOOMRIGHT";
    miscnames[V_MISC_HCDBRIDGE - MISCTILEOFFSET] = "HCDBRIDGE";
    miscnames[V_MISC_VCDBRIDGE - MISCTILEOFFSET] = "VCDBRIDGE";
    miscnames[V_MISC_VODBRIDGE - MISCTILEOFFSET] = "VODBRIDGE";
    miscnames[V_MISC_HODBRIDGE - MISCTILEOFFSET] = "HODBRIDGE";
    miscnames[V_MISC_CLOUD - MISCTILEOFFSET] = "CLOUD";
    miscnames[V_MISC_OFF_MAP - MISCTILEOFFSET] = "OFF_MAP";
    miscnames[V_MISC_FLOOR_HIGHLIGHT - MISCTILEOFFSET] = "FLOOR_HIGHLIGHT";
    miscnames[V_MISC_LAND_MINE - MISCTILEOFFSET] = "LAND_MINE";
    miscnames[V_MISC_LAWFUL_PRIEST - MISCTILEOFFSET] = "LAWFUL_PRIEST";
    miscnames[V_MISC_CHAOTIC_PRIEST - MISCTILEOFFSET] = "CHAOTIC_PRIEST";
    miscnames[V_MISC_NEUTRAL_PRIEST - MISCTILEOFFSET] = "NEUTRAL_PRIEST";
    miscnames[V_MISC_UNALIGNED_PRIEST - MISCTILEOFFSET] = "UNALIGNED_PRIEST";
#if defined(REINCARNATION)
    miscnames[V_MISC_ROGUE_LEVEL_A - MISCTILEOFFSET] = "ROGUE_LEVEL_A";
    miscnames[V_MISC_ROGUE_LEVEL_B - MISCTILEOFFSET] = "ROGUE_LEVEL_B";
    miscnames[V_MISC_ROGUE_LEVEL_C - MISCTILEOFFSET] = "ROGUE_LEVEL_C";
    miscnames[V_MISC_ROGUE_LEVEL_D - MISCTILEOFFSET] = "ROGUE_LEVEL_D";
    miscnames[V_MISC_ROGUE_LEVEL_E - MISCTILEOFFSET] = "ROGUE_LEVEL_E";
    miscnames[V_MISC_ROGUE_LEVEL_F - MISCTILEOFFSET] = "ROGUE_LEVEL_F";
    miscnames[V_MISC_ROGUE_LEVEL_G - MISCTILEOFFSET] = "ROGUE_LEVEL_G";
    miscnames[V_MISC_ROGUE_LEVEL_H - MISCTILEOFFSET] = "ROGUE_LEVEL_H";
    miscnames[V_MISC_ROGUE_LEVEL_I - MISCTILEOFFSET] = "ROGUE_LEVEL_I";
    miscnames[V_MISC_ROGUE_LEVEL_J - MISCTILEOFFSET] = "ROGUE_LEVEL_J";
    miscnames[V_MISC_ROGUE_LEVEL_K - MISCTILEOFFSET] = "ROGUE_LEVEL_K";
    miscnames[V_MISC_ROGUE_LEVEL_L - MISCTILEOFFSET] = "ROGUE_LEVEL_L";
    miscnames[V_MISC_ROGUE_LEVEL_M - MISCTILEOFFSET] = "ROGUE_LEVEL_M";
    miscnames[V_MISC_ROGUE_LEVEL_N - MISCTILEOFFSET] = "ROGUE_LEVEL_N";
    miscnames[V_MISC_ROGUE_LEVEL_O - MISCTILEOFFSET] = "ROGUE_LEVEL_O";
    miscnames[V_MISC_ROGUE_LEVEL_P - MISCTILEOFFSET] = "ROGUE_LEVEL_P";
    miscnames[V_MISC_ROGUE_LEVEL_Q - MISCTILEOFFSET] = "ROGUE_LEVEL_Q";
    miscnames[V_MISC_ROGUE_LEVEL_R - MISCTILEOFFSET] = "ROGUE_LEVEL_R";
    miscnames[V_MISC_ROGUE_LEVEL_S - MISCTILEOFFSET] = "ROGUE_LEVEL_S";
    miscnames[V_MISC_ROGUE_LEVEL_T - MISCTILEOFFSET] = "ROGUE_LEVEL_T";
    miscnames[V_MISC_ROGUE_LEVEL_U - MISCTILEOFFSET] = "ROGUE_LEVEL_U";
    miscnames[V_MISC_ROGUE_LEVEL_V - MISCTILEOFFSET] = "ROGUE_LEVEL_V";
    miscnames[V_MISC_ROGUE_LEVEL_W - MISCTILEOFFSET] = "ROGUE_LEVEL_W";
    miscnames[V_MISC_ROGUE_LEVEL_X - MISCTILEOFFSET] = "ROGUE_LEVEL_X";
    miscnames[V_MISC_ROGUE_LEVEL_Y - MISCTILEOFFSET] = "ROGUE_LEVEL_Y";
    miscnames[V_MISC_ROGUE_LEVEL_Z - MISCTILEOFFSET] = "ROGUE_LEVEL_Z";
#endif
    miscnames[V_MISC_ENGULF_FIRE_VORTEX - MISCTILEOFFSET] = "ENGULF_FIRE_VORTEX";
    miscnames[V_MISC_ENGULF_FOG_CLOUD - MISCTILEOFFSET] = "ENGULF_FOG_CLOUD";
    miscnames[V_MISC_ENGULF_AIR_ELEMENTAL - MISCTILEOFFSET] = "ENGULF_AIR_ELEMENTAL";
    miscnames[V_MISC_ENGULF_STEAM_VORTEX - MISCTILEOFFSET] = "ENGULF_STEAM_VORTEX";
    miscnames[V_MISC_ENGULF_PURPLE_WORM - MISCTILEOFFSET] = "ENGULF_PURPLE_WORM";
    miscnames[V_MISC_ENGULF_JUIBLEX - MISCTILEOFFSET] = "ENGULF_JUIBLEX";
    miscnames[V_MISC_ENGULF_OCHRE_JELLY - MISCTILEOFFSET] = "ENGULF_OCHRE_JELLY";
    miscnames[V_MISC_ENGULF_LURKER_ABOVE - MISCTILEOFFSET] = "ENGULF_LURKER_ABOVE";
    miscnames[V_MISC_ENGULF_TRAPPER - MISCTILEOFFSET] = "ENGULF_TRAPPER";
    miscnames[V_MISC_ENGULF_DUST_VORTEX - MISCTILEOFFSET] = "ENGULF_DUST_VORTEX";
    miscnames[V_MISC_ENGULF_ICE_VORTEX - MISCTILEOFFSET] = "ENGULF_ICE_VORTEX";
    miscnames[V_MISC_ENGULF_ENERGY_VORTEX - MISCTILEOFFSET] = "ENGULF_ENERGY_VORTEX";
    miscnames[V_MISC_WARNLEV_1 - MISCTILEOFFSET] = "WARNLEV_1";
    miscnames[V_MISC_WARNLEV_2 - MISCTILEOFFSET] = "WARNLEV_2";
    miscnames[V_MISC_WARNLEV_3 - MISCTILEOFFSET] = "WARNLEV_3";
    miscnames[V_MISC_WARNLEV_4 - MISCTILEOFFSET] = "WARNLEV_4";
    miscnames[V_MISC_WARNLEV_5 - MISCTILEOFFSET] = "WARNLEV_5";
    miscnames[V_MISC_WARNLEV_6 - MISCTILEOFFSET] = "WARNLEV_6";
    miscnames[V_MISC_INVISIBLE_MONSTER - MISCTILEOFFSET] = "INVISIBLE_MONSTER";
    miscnames[V_MISC_STINKING_CLOUD - MISCTILEOFFSET] = "STINKING_CLOUD";
#ifdef VULTURESCLAW
    miscnames[V_MISC_TOILET - MISCTILEOFFSET] = "TOILET";
#endif
}


static void init_objnames()
{
    int i;
    char *c, *nameptr;
    int unnamed_cnt[MAXOCLASSES];

    static const char *objclassnames[] = { 0,
            "Illegal objects", "Weapons", "Armor", "Rings", "Amulets",
            "Tools", "Comestibles", "Potions", "Scrolls", "Spellbooks",
            "Wands", "Coins", "Gems", "Boulders/Statues", "Iron balls",
            "Chains", "Venoms"
    };

    memset(unnamed_cnt, 0, MAXOCLASSES * sizeof(int));

    vultures_typecount[TT_OBJECT] = OBJTILECOUNT;

    tilenames[TT_OBJECT] = (char **)malloc(vultures_typecount[TT_OBJECT] * sizeof(char*));

    for(i = 0; !i || objects[i].oc_class != ILLOBJ_CLASS; i++)
    {
        tilenames[TT_OBJECT][i] = (char *)malloc(41 * sizeof(char)); /* makedefs uses max 30 chars + '\0' */
        tilenames[TT_OBJECT][i][0] = '\0';
        tilenames[TT_OBJECT][i][40] = '\0';

        if (!obj_descr[i].oc_name)
        {
            unnamed_cnt[(int)objects[i].oc_class]++;
            snprintf(tilenames[TT_OBJECT][i], 40, "%3.3s unnamed %d",
                     objclassnames[(int)objects[i].oc_class], unnamed_cnt[(int)objects[i].oc_class]);
        }
        else
        {
            nameptr = (char*)obj_descr[i].oc_name;

            switch (objects[i].oc_class)
            {
                case WAND_CLASS:
                    snprintf(tilenames[TT_OBJECT][i], 40, "WAN_%s", nameptr); break;
                case RING_CLASS:
                    snprintf(tilenames[TT_OBJECT][i], 40, "RIN_%s", nameptr); break;
                case POTION_CLASS:
                    snprintf(tilenames[TT_OBJECT][i], 40, "POT_%s", nameptr); break;
                case SPBOOK_CLASS:
                    snprintf(tilenames[TT_OBJECT][i], 40, "SPE_%s", nameptr); break;
                case SCROLL_CLASS:
                    snprintf(tilenames[TT_OBJECT][i], 40, "SCR_%s", nameptr); break;
                case AMULET_CLASS:
                    if(objects[i].oc_material == PLASTIC)
                        snprintf(tilenames[TT_OBJECT][i], 40, "FAKE_AMULET_OF_YENDOR");
                    else
                        snprintf(tilenames[TT_OBJECT][i], 40, "%s", obj_descr[i].oc_name); break;
                case GEM_CLASS:
                    if (objects[i].oc_material == GLASS)
                    {
                        switch (objects[i].oc_color)
                        {
                            case CLR_WHITE:  snprintf(tilenames[TT_OBJECT][i], 40, "GEM_WHITE_GLASS"); break;
                            case CLR_BLUE:   snprintf(tilenames[TT_OBJECT][i], 40, "GEM_BLUE_GLASS");  break;
                            case CLR_RED:    snprintf(tilenames[TT_OBJECT][i], 40, "GEM_RED_GLASS");   break;
                            case CLR_BROWN:  snprintf(tilenames[TT_OBJECT][i], 40, "GEM_BROWN_GLASS"); break;
                            case CLR_ORANGE: snprintf(tilenames[TT_OBJECT][i], 40, "GEM_ORANGE_GLASS");break;
                            case CLR_YELLOW: snprintf(tilenames[TT_OBJECT][i], 40, "GEM_YELLOW_GLASS");break;
                            case CLR_BLACK:  snprintf(tilenames[TT_OBJECT][i], 40, "GEM_BLACK_GLASS"); break;
                            case CLR_GREEN:  snprintf(tilenames[TT_OBJECT][i], 40, "GEM_GREEN_GLASS"); break;
                            case CLR_MAGENTA:snprintf(tilenames[TT_OBJECT][i], 40, "GEM_VIOLET_GLASS");break;
                        }
                        glassgems[objects[i].oc_color] = i;
                    }
                    else
                        snprintf(tilenames[TT_OBJECT][i], 40, "%s", nameptr); break;
                    break;
                default:
                    snprintf(tilenames[TT_OBJECT][i], 40, "%s", nameptr); break;
            }
        }

        for (c = tilenames[TT_OBJECT][i]; *c; c++)
        {
            if (*c >= 'a' && *c <= 'z') *c -= (char)('a' - 'A');
            else if ((*c < 'A' || *c > 'Z') && !isdigit(*c)) *c = '_';
        }

    }

    /* fix up the slime mold as it's name varies and we can't count on it being called "slime mold" */
    free(tilenames[TT_OBJECT][SLIME_MOLD]);
    tilenames[TT_OBJECT][SLIME_MOLD] = strdup("SLIME_MOLD");

    vultures_typecount[TT_OBJICON] = vultures_typecount[TT_OBJECT];
    tilenames[TT_OBJICON] = tilenames[TT_OBJECT];
}



static void init_monnames()
{
    char * c;
    int i;

    vultures_typecount[TT_MONSTER] = MONTILECOUNT;

    tilenames[TT_MONSTER] = (char **)malloc(vultures_typecount[TT_MONSTER] * sizeof(char*));

    for (i = 0; mons[i].mlet; i++)
    {
        tilenames[TT_MONSTER][i] = (char *)malloc(64 * sizeof(char));
        if (mons[i].mlet == S_HUMAN && !strncmp(mons[i].mname, "were", 4))
            snprintf(tilenames[TT_MONSTER][i], 64, "PM_HUMAN_%s", mons[i].mname);
        else
            snprintf(tilenames[TT_MONSTER][i], 64, "PM_%s", mons[i].mname);

        for (c = tilenames[TT_MONSTER][i]; *c; c++)
        {
            if (*c >= 'a' && *c <= 'z') *c -= (char)('a' - 'A');
            else if (*c < 'A' || *c > 'Z') *c = '_';
        }
    }

    vultures_typecount[TT_STATUE] = vultures_typecount[TT_MONSTER];
    tilenames[TT_STATUE] = tilenames[TT_MONSTER];

    vultures_typecount[TT_FIGURINE] = vultures_typecount[TT_MONSTER];
    tilenames[TT_FIGURINE] = tilenames[TT_MONSTER];
}



static void init_explnames()
{
    int i, j;
    const char * explosion_names[] = {"DARK", "NOXIOUS", "MUDDY", "WET", "MAGICAL", "FIERY", "FROSTY"};

    vultures_typecount[TT_EXPL] = EXPL_MAX * 9;

    tilenames[TT_EXPL] = (char **)malloc(vultures_typecount[TT_EXPL] * sizeof(char*));

    for (i = 0; i < EXPL_MAX; i++)
    {
        for (j = 0; j < 9; j++)
        {
            tilenames[TT_EXPL][i*9+j] = (char *)malloc(64 * sizeof(char));
            snprintf(tilenames[TT_EXPL][i*9+j], 64, "%s_%d", explosion_names[i], j+1);
        }
    }
}

