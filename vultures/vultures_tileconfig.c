/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#include "vultures_types.h"
#include "vultures_tile.h"
#include "vultures_tileconfig.h"


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
struct gametiles *vultures_gametiles;
struct fedges flooredges[V_FLOOR_EDGE_MAX];
struct fstyles floorstyles[V_FLOOR_STYLE_MAX];
int vultures_typecount[NUM_TILETYPES];
int glassgems[CLR_MAX];

/* names for the various tiletypes */
static const char *typenames[NUM_TILETYPES] = {
    [TT_OBJECT] = "object",
    [TT_OBJICON] = "objicon",
    [TT_MONSTER] = "monster",
    [TT_STATUE] = "statue",
    [TT_FIGURINE] = "figurine",
    [TT_MISC] = "misc",
    [TT_EXPL] = "explosion",
    [TT_EDGE] = "edge",
    [TT_FLOOR] = "floor",
    [TT_WALL] = "wall",
    [TT_CURSOR] = "cursor"
};

/* names for various types of tiles need to be given explicitly */

static const char * floorstylenames[V_FLOOR_STYLE_MAX] = {
    [V_FLOOR_COBBLESTONE] = "COBBLESTONE",
    [V_FLOOR_ROUGH] = "ROUGH",
    [V_FLOOR_CERAMIC] = "CERAMIC",
    [V_FLOOR_LAVA] = "LAVA",
    [V_FLOOR_WATER] = "WATER",
    [V_FLOOR_ICE] = "ICE",
    [V_FLOOR_MURAL] = "MURAL",
    [V_FLOOR_MURAL2] = "MURAL2",
    [V_FLOOR_CARPET] = "CARPET",
    [V_FLOOR_MOSS_COVERED] = "MOSS_COVERED",
    [V_FLOOR_MARBLE] = "MARBLE",
    [V_FLOOR_ROUGH_LIT] = "ROUGH_LIT",
    [V_FLOOR_AIR] = "AIR",
    [V_FLOOR_DARK] = "DARK"
};

static const char * wallstylenames[V_WALL_STYLE_MAX] = {
    [V_WALL_BRICK] = "BRICK",
    [V_WALL_BRICK_BANNER] = "BRICK_BANNER",
    [V_WALL_BRICK_PAINTING] = "BRICK_PAINTING",
    [V_WALL_BRICK_POCKET] = "BRICK_POCKET",
    [V_WALL_BRICK_PILLAR] = "BRICK_PILLAR",
    [V_WALL_MARBLE] = "MARBLE",
    [V_WALL_VINE_COVERED] = "VINE_COVERED",
    [V_WALL_STUCCO] = "STUCCO",
    [V_WALL_ROUGH] = "ROUGH",
    [V_WALL_DARK] = "DARK",
    [V_WALL_LIGHT] = "LIGHT"
};

static const char * edgestylenames[V_FLOOR_EDGE_MAX] = {
    [V_FLOOR_EDGE_COBBLESTONE] = "COBBLESTONE"
};

/* the enum for the misc tiles starts at MISCTILEOFFSET so that the enumerated
 * names can be used diretly in the code; here however this means we need to
 * subtract MISCTILEOFFSET everywhere to get usable array indices */
static char *miscnames[MISCTILECOUNT] = {
    [V_MISC_PLAYER_INVIS - MISCTILEOFFSET] = "", /* make it impossible to assign a tile to V_MISC_PLAYER_INVIS */
    [V_MISC_FLOOR_NOT_VISIBLE - MISCTILEOFFSET] = "FLOOR_NOT_VISIBLE",
    [V_MISC_DOOR_WOOD_BROKEN - MISCTILEOFFSET] = "DOOR_WOOD_BROKEN",
    [V_MISC_HDOOR_WOOD_CLOSED - MISCTILEOFFSET] = "HDOOR_WOOD_CLOSED",
    [V_MISC_VDOOR_WOOD_CLOSED - MISCTILEOFFSET] = "VDOOR_WOOD_CLOSED",
    [V_MISC_VDOOR_WOOD_OPEN - MISCTILEOFFSET] = "VDOOR_WOOD_OPEN",
    [V_MISC_HDOOR_WOOD_OPEN - MISCTILEOFFSET] = "HDOOR_WOOD_OPEN",
    [V_MISC_TRAP_BEAR - MISCTILEOFFSET] = "TRAP_BEAR",
    [V_MISC_GRAVE - MISCTILEOFFSET] = "GRAVE",
    [V_MISC_ALTAR - MISCTILEOFFSET] = "ALTAR",
    [V_MISC_FOUNTAIN - MISCTILEOFFSET] = "FOUNTAIN",
    [V_MISC_STAIRS_UP - MISCTILEOFFSET] = "STAIRS_UP",
    [V_MISC_STAIRS_DOWN - MISCTILEOFFSET] = "STAIRS_DOWN",
    [V_MISC_SINK - MISCTILEOFFSET] = "SINK",
    [V_MISC_GAS_TRAP - MISCTILEOFFSET] = "GAS_TRAP",
    [V_MISC_TRAP_PIT - MISCTILEOFFSET] = "TRAP_PIT",
    [V_MISC_TRAP_POLYMORPH - MISCTILEOFFSET] = "TRAP_POLYMORPH",
    [V_MISC_TREE - MISCTILEOFFSET] = "TREE",
    [V_MISC_TRAP_MAGIC - MISCTILEOFFSET] = "TRAP_MAGIC",
    [V_MISC_TRAP_DOOR - MISCTILEOFFSET] = "TRAP_DOOR",
    [V_MISC_TRAP_WATER - MISCTILEOFFSET] = "TRAP_WATER",
    [V_MISC_TRAP_TELEPORTER - MISCTILEOFFSET] = "TRAP_TELEPORTER",
    [V_MISC_UNMAPPED_AREA - MISCTILEOFFSET] = "UNMAPPED_AREA",
    [V_MISC_HILITE_PET - MISCTILEOFFSET] = "HILITE_PET",
    [V_MISC_BARS - MISCTILEOFFSET] = "BARS",
    [V_MISC_THRONE - MISCTILEOFFSET] = "THRONE",
    [V_MISC_TRAP_ANTI_MAGIC - MISCTILEOFFSET] = "TRAP_ANTI_MAGIC",
    [V_MISC_TRAP_ARROW - MISCTILEOFFSET] = "TRAP_ARROW",
    [V_MISC_TRAP_FIRE - MISCTILEOFFSET] = "TRAP_FIRE",
    [V_MISC_ROLLING_BOULDER_TRAP - MISCTILEOFFSET] = "ROLLING_BOULDER_TRAP",
    [V_MISC_TRAP_SLEEPGAS - MISCTILEOFFSET] = "TRAP_SLEEPGAS",
    [V_MISC_ZAP_SLANT_RIGHT - MISCTILEOFFSET] = "ZAP_SLANT_RIGHT",
    [V_MISC_ZAP_SLANT_LEFT - MISCTILEOFFSET] = "ZAP_SLANT_LEFT",
    [V_MISC_ZAP_HORIZONTAL - MISCTILEOFFSET] = "ZAP_HORIZONTAL",
    [V_MISC_ZAP_VERTICAL - MISCTILEOFFSET] = "ZAP_VERTICAL",
    [V_MISC_LADDER_UP - MISCTILEOFFSET] = "LADDER_UP",
    [V_MISC_LADDER_DOWN - MISCTILEOFFSET] = "LADDER_DOWN",
    [V_MISC_RESIST_SPELL_1 - MISCTILEOFFSET] = "RESIST_SPELL_1",
    [V_MISC_RESIST_SPELL_2 - MISCTILEOFFSET] = "RESIST_SPELL_2",
    [V_MISC_RESIST_SPELL_3 - MISCTILEOFFSET] = "RESIST_SPELL_3",
    [V_MISC_RESIST_SPELL_4 - MISCTILEOFFSET] = "RESIST_SPELL_4",
    [V_MISC_WEB_TRAP - MISCTILEOFFSET] = "WEB_TRAP",
    [V_MISC_DART_TRAP - MISCTILEOFFSET] = "DART_TRAP",
    [V_MISC_FALLING_ROCK_TRAP - MISCTILEOFFSET] = "FALLING_ROCK_TRAP",
    [V_MISC_SQUEAKY_BOARD - MISCTILEOFFSET] = "SQUEAKY_BOARD",
    [V_MISC_MAGIC_PORTAL - MISCTILEOFFSET] = "MAGIC_PORTAL",
    [V_MISC_SPIKED_PIT - MISCTILEOFFSET] = "SPIKED_PIT",
    [V_MISC_HOLE - MISCTILEOFFSET] = "HOLE",
    [V_MISC_LEVEL_TELEPORTER - MISCTILEOFFSET] = "LEVEL_TELEPORTER",
    [V_MISC_MAGIC_TRAP - MISCTILEOFFSET] = "MAGIC_TRAP",
    [V_MISC_DIGBEAM - MISCTILEOFFSET] = "DIGBEAM",
    [V_MISC_FLASHBEAM - MISCTILEOFFSET] = "FLASHBEAM",
    [V_MISC_BOOMLEFT - MISCTILEOFFSET] = "BOOMLEFT",
    [V_MISC_BOOMRIGHT - MISCTILEOFFSET] = "BOOMRIGHT",
    [V_MISC_HCDBRIDGE - MISCTILEOFFSET] = "HCDBRIDGE",
    [V_MISC_VCDBRIDGE - MISCTILEOFFSET] = "VCDBRIDGE",
    [V_MISC_VODBRIDGE - MISCTILEOFFSET] = "VODBRIDGE",
    [V_MISC_HODBRIDGE - MISCTILEOFFSET] = "HODBRIDGE",
    [V_MISC_CLOUD - MISCTILEOFFSET] = "CLOUD",
    [V_MISC_OFF_MAP - MISCTILEOFFSET] = "OFF_MAP",
    [V_MISC_FLOOR_HIGHLIGHT - MISCTILEOFFSET] = "FLOOR_HIGHLIGHT",
    [V_MISC_LAND_MINE - MISCTILEOFFSET] = "LAND_MINE",
    [V_MISC_LAWFUL_PRIEST - MISCTILEOFFSET] = "LAWFUL_PRIEST",
    [V_MISC_CHAOTIC_PRIEST - MISCTILEOFFSET] = "CHAOTIC_PRIEST",
    [V_MISC_NEUTRAL_PRIEST - MISCTILEOFFSET] = "NEUTRAL_PRIEST",
    [V_MISC_UNALIGNED_PRIEST - MISCTILEOFFSET] = "UNALIGNED_PRIEST",
#if defined(REINCARNATION)
    [V_MISC_ROGUE_LEVEL_A - MISCTILEOFFSET] = "ROGUE_LEVEL_A",
    [V_MISC_ROGUE_LEVEL_B - MISCTILEOFFSET] = "ROGUE_LEVEL_B",
    [V_MISC_ROGUE_LEVEL_C - MISCTILEOFFSET] = "ROGUE_LEVEL_C",
    [V_MISC_ROGUE_LEVEL_D - MISCTILEOFFSET] = "ROGUE_LEVEL_D",
    [V_MISC_ROGUE_LEVEL_E - MISCTILEOFFSET] = "ROGUE_LEVEL_E",
    [V_MISC_ROGUE_LEVEL_F - MISCTILEOFFSET] = "ROGUE_LEVEL_F",
    [V_MISC_ROGUE_LEVEL_G - MISCTILEOFFSET] = "ROGUE_LEVEL_G",
    [V_MISC_ROGUE_LEVEL_H - MISCTILEOFFSET] = "ROGUE_LEVEL_H",
    [V_MISC_ROGUE_LEVEL_I - MISCTILEOFFSET] = "ROGUE_LEVEL_I",
    [V_MISC_ROGUE_LEVEL_J - MISCTILEOFFSET] = "ROGUE_LEVEL_J",
    [V_MISC_ROGUE_LEVEL_K - MISCTILEOFFSET] = "ROGUE_LEVEL_K",
    [V_MISC_ROGUE_LEVEL_L - MISCTILEOFFSET] = "ROGUE_LEVEL_L",
    [V_MISC_ROGUE_LEVEL_M - MISCTILEOFFSET] = "ROGUE_LEVEL_M",
    [V_MISC_ROGUE_LEVEL_N - MISCTILEOFFSET] = "ROGUE_LEVEL_N",
    [V_MISC_ROGUE_LEVEL_O - MISCTILEOFFSET] = "ROGUE_LEVEL_O",
    [V_MISC_ROGUE_LEVEL_P - MISCTILEOFFSET] = "ROGUE_LEVEL_P",
    [V_MISC_ROGUE_LEVEL_Q - MISCTILEOFFSET] = "ROGUE_LEVEL_Q",
    [V_MISC_ROGUE_LEVEL_R - MISCTILEOFFSET] = "ROGUE_LEVEL_R",
    [V_MISC_ROGUE_LEVEL_S - MISCTILEOFFSET] = "ROGUE_LEVEL_S",
    [V_MISC_ROGUE_LEVEL_T - MISCTILEOFFSET] = "ROGUE_LEVEL_T",
    [V_MISC_ROGUE_LEVEL_U - MISCTILEOFFSET] = "ROGUE_LEVEL_U",
    [V_MISC_ROGUE_LEVEL_V - MISCTILEOFFSET] = "ROGUE_LEVEL_V",
    [V_MISC_ROGUE_LEVEL_W - MISCTILEOFFSET] = "ROGUE_LEVEL_W",
    [V_MISC_ROGUE_LEVEL_X - MISCTILEOFFSET] = "ROGUE_LEVEL_X",
    [V_MISC_ROGUE_LEVEL_Y - MISCTILEOFFSET] = "ROGUE_LEVEL_Y",
    [V_MISC_ROGUE_LEVEL_Z - MISCTILEOFFSET] = "ROGUE_LEVEL_Z",
#endif
    [V_MISC_ENGULF_FIRE_VORTEX - MISCTILEOFFSET] = "ENGULF_FIRE_VORTEX",
    [V_MISC_ENGULF_FOG_CLOUD - MISCTILEOFFSET] = "ENGULF_FOG_CLOUD",
    [V_MISC_ENGULF_AIR_ELEMENTAL - MISCTILEOFFSET] = "ENGULF_AIR_ELEMENTAL",
    [V_MISC_ENGULF_STEAM_VORTEX - MISCTILEOFFSET] = "ENGULF_STEAM_VORTEX",
    [V_MISC_ENGULF_PURPLE_WORM - MISCTILEOFFSET] = "ENGULF_PURPLE_WORM",
    [V_MISC_ENGULF_JUIBLEX - MISCTILEOFFSET] = "ENGULF_JUIBLEX",
    [V_MISC_ENGULF_OCHRE_JELLY - MISCTILEOFFSET] = "ENGULF_OCHRE_JELLY",
    [V_MISC_ENGULF_LURKER_ABOVE - MISCTILEOFFSET] = "ENGULF_LURKER_ABOVE",
    [V_MISC_ENGULF_TRAPPER - MISCTILEOFFSET] = "ENGULF_TRAPPER",
    [V_MISC_ENGULF_DUST_VORTEX - MISCTILEOFFSET] = "ENGULF_DUST_VORTEX",
    [V_MISC_ENGULF_ICE_VORTEX - MISCTILEOFFSET] = "ENGULF_ICE_VORTEX",
    [V_MISC_ENGULF_ENERGY_VORTEX - MISCTILEOFFSET] = "ENGULF_ENERGY_VORTEX",
    [V_MISC_WARNLEV_1 - MISCTILEOFFSET] = "WARNLEV_1",
    [V_MISC_WARNLEV_2 - MISCTILEOFFSET] = "WARNLEV_2",
    [V_MISC_WARNLEV_3 - MISCTILEOFFSET] = "WARNLEV_3",
    [V_MISC_WARNLEV_4 - MISCTILEOFFSET] = "WARNLEV_4",
    [V_MISC_WARNLEV_5 - MISCTILEOFFSET] = "WARNLEV_5",
    [V_MISC_WARNLEV_6 - MISCTILEOFFSET] = "WARNLEV_6",
    [V_MISC_INVISIBLE_MONSTER - MISCTILEOFFSET] = "INVISIBLE_MONSTER",
    [V_MISC_STINKING_CLOUD - MISCTILEOFFSET] = "STINKING_CLOUD",
    [V_MISC_TOILET - MISCTILEOFFSET] = "TOILET"
};

char *cursornames[V_CURSOR_MAX] = {
    [V_CURSOR_NORMAL] = "NORMAL",
    [V_CURSOR_SCROLLRIGHT] = "SCROLLRIGHT",
    [V_CURSOR_SCROLLLEFT] = "SCROLLLEFT",
    [V_CURSOR_SCROLLUP] = "SCROLLUP",
    [V_CURSOR_SCROLLDOWN] = "SCROLLDOWN",
    [V_CURSOR_SCROLLUPLEFT] = "SCROLLUPLEFT",
    [V_CURSOR_SCROLLUPRIGHT] = "SCROLLUPRIGHT",
    [V_CURSOR_SCROLLDOWNLEFT] = "SCROLLDOWNLEFT",
    [V_CURSOR_SCROLLDOWNRIGHT] = "SCROLLDOWNRIGHT",
    [V_CURSOR_TARGET_GREEN] = "TARGET_GREEN",
    [V_CURSOR_TARGET_RED] = "TARGET_RED",
    [V_CURSOR_TARGET_INVALID] = "TARGET_INVALID",
    [V_CURSOR_TARGET_HELP] = "TARGET_HELP",
    [V_CURSOR_HOURGLASS] = "HOURGLASS",
    [V_CURSOR_OPENDOOR] = "OPENDOOR",
    [V_CURSOR_STAIRS] = "STAIRS",
    [V_CURSOR_GOBLET] = "GOBLET"
};



static void init_objnames();
static void init_monnames();
static void init_explnames();


/* parse the configuration file and prepare the data arrays needed by the rest of vultures 
 * This function is the only one that should be called from outside */
void vultures_parse_tileconf(FILE *fp)
{
    int i, j, tilenum;
    int typeoffset[NUM_TILETYPES];

    /* init the names for all types of tiles except walls, floors and edges, where the names are dynamic */
    init_objnames(); /* init TT_OBJECT and TT_OBJICON */
    init_monnames(); /* init TT_MONSTER, TT_STATUE and TT_FIGURINE */
    init_explnames();/* TT_EXPL */

    vultures_typecount[TT_MISC] = MISCTILECOUNT;
    tilenames[TT_MISC] = miscnames;
    vultures_typecount[TT_CURSOR] = V_CURSOR_MAX;
    tilenames[TT_CURSOR] = cursornames;

    vultures_typecount[TT_WALL] = 0;
    tilenames[TT_WALL] = malloc(0);
    vultures_typecount[TT_FLOOR] = 0;
    tilenames[TT_FLOOR] = malloc(0);
    vultures_typecount[TT_EDGE] = 0;
    tilenames[TT_EDGE] = malloc(0);

    for (i = 0; i < NUM_TILETYPES; i++)
    {
        tmp_gametiles[i] = malloc(vultures_typecount[i] * sizeof(tmp_tile));
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


    vultures_gametiles = malloc(GAMETILECOUNT * sizeof(struct gametiles));
    memset(vultures_gametiles, 0, GAMETILECOUNT * sizeof(struct gametiles));

    /* build vultures_gametiles from the info in tmp_gametiles */
    for (i = 0; i < NUM_TILETYPES; i++)
    {
        for (j = 0; j < vultures_typecount[i]; j++)
        {
            tilenum = typeoffset[i] + j;

            /* use given tile */
            if (tmp_gametiles[i][j].filename && tmp_gametiles[i][j].ptr_type == -1 && tmp_gametiles[i][j].ptr_num == -1)
            {
                vultures_gametiles[tilenum].filename = strdup(tmp_gametiles[i][j].filename);
                vultures_gametiles[tilenum].ptr = -1;
                vultures_gametiles[tilenum].hs_x = tmp_gametiles[i][j].hs_x;
                vultures_gametiles[tilenum].hs_y = tmp_gametiles[i][j].hs_y;
            }
            /* redirect to another tile */
            else if (tmp_gametiles[i][j].ptr_type != -1 && tmp_gametiles[i][j].ptr_num != -1)
            {
                vultures_gametiles[tilenum].filename = NULL;
                vultures_gametiles[tilenum].ptr = typeoffset[tmp_gametiles[i][j].ptr_type] + tmp_gametiles[i][j].ptr_num;
                vultures_gametiles[tilenum].hs_x = 0;
                vultures_gametiles[tilenum].hs_y = 0;
            }
            /* use default tile */
            else if (!tmp_gametiles[i][j].filename && tmp_gametiles[i][j].ptr_type == 0 && tmp_gametiles[i][j].ptr_num == 0)
            {
                vultures_gametiles[tilenum].ptr = typeoffset[deftiles[i].ptr_type] + deftiles[i].ptr_num;
                vultures_gametiles[tilenum].hs_x = deftiles[i].hs_x;
                vultures_gametiles[tilenum].hs_y = deftiles[i].hs_y;
            }
        }
    }

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
            tilenames[type] = realloc(tilenames[type], vultures_typecount[type] * sizeof(char*));
            tmp_gametiles[type] = realloc(tmp_gametiles[type], vultures_typecount[type] * sizeof(tmp_tile));
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


static void init_objnames()
{
    int i;
    char *c, *nameptr, nonamebuf[16];
    int unnamed_cnt = 0;

    vultures_typecount[TT_OBJECT] = OBJTILECOUNT;

    tilenames[TT_OBJECT] = malloc(vultures_typecount[TT_OBJECT] * sizeof(char*));

    for(i = 0; !i || objects[i].oc_class != ILLOBJ_CLASS; i++)
    {
        tilenames[TT_OBJECT][i] = malloc(31 * sizeof(char)); /* makedefs uses max 30 chars + '\0' */
        tilenames[TT_OBJECT][i][0] = '\0';
        tilenames[TT_OBJECT][i][30] = '\0';

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
                snprintf(tilenames[TT_OBJECT][i], 30, "WAN_%s", nameptr); break;
            case RING_CLASS:
                snprintf(tilenames[TT_OBJECT][i], 30, "RIN_%s", nameptr); break;
            case POTION_CLASS:
                snprintf(tilenames[TT_OBJECT][i], 30, "POT_%s", nameptr); break;
            case SPBOOK_CLASS:
                snprintf(tilenames[TT_OBJECT][i], 30, "SPE_%s", nameptr); break;
            case SCROLL_CLASS:
                snprintf(tilenames[TT_OBJECT][i], 30, "SCR_%s", nameptr); break;
            case AMULET_CLASS:
                if(objects[i].oc_material == PLASTIC)
                    snprintf(tilenames[TT_OBJECT][i], 30, "FAKE_AMULET_OF_YENDOR");
                else
                    snprintf(tilenames[TT_OBJECT][i], 30, "%s", obj_descr[i].oc_name); break;
            case GEM_CLASS:
                if (objects[i].oc_material == GLASS)
                {
                    switch (objects[i].oc_color)
                    {
                        case CLR_WHITE:  snprintf(tilenames[TT_OBJECT][i], 30, "GEM_WHITE_GLASS"); break;
                        case CLR_BLUE:   snprintf(tilenames[TT_OBJECT][i], 30, "GEM_BLUE_GLASS");  break;
                        case CLR_RED:    snprintf(tilenames[TT_OBJECT][i], 30, "GEM_RED_GLASS");   break;
                        case CLR_BROWN:  snprintf(tilenames[TT_OBJECT][i], 30, "GEM_BROWN_GLASS"); break;
                        case CLR_ORANGE: snprintf(tilenames[TT_OBJECT][i], 30, "GEM_ORANGE_GLASS");break;
                        case CLR_YELLOW: snprintf(tilenames[TT_OBJECT][i], 30, "GEM_YELLOW_GLASS");break;
                        case CLR_BLACK:  snprintf(tilenames[TT_OBJECT][i], 30, "GEM_BLACK_GLASS"); break;
                        case CLR_GREEN:  snprintf(tilenames[TT_OBJECT][i], 30, "GEM_GREEN_GLASS"); break;
                        case CLR_MAGENTA:snprintf(tilenames[TT_OBJECT][i], 30, "GEM_VIOLET_GLASS");break;
                    }
                    glassgems[objects[i].oc_color] = i;
                }
                else
                    snprintf(tilenames[TT_OBJECT][i], 30, "%s", nameptr); break;
                break;
            default:
                snprintf(tilenames[TT_OBJECT][i], 30, "%s", nameptr); break;
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

    tilenames[TT_MONSTER] = malloc(vultures_typecount[TT_MONSTER] * sizeof(char*));

    for (i = 0; mons[i].mlet; i++)
    {
        tilenames[TT_MONSTER][i] = malloc(64 * sizeof(char));
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

    tilenames[TT_EXPL] = malloc(vultures_typecount[TT_EXPL] * sizeof(char*));

    for (i = 0; i < EXPL_MAX; i++)
    {
        for (j = 0; j < 9; j++)
        {
            tilenames[TT_EXPL][i*9+j] = malloc(64 * sizeof(char));
            snprintf(tilenames[TT_EXPL][i*9+j], 64, "%s_%d", explosion_names[i], j+1);
        }
    }
}

