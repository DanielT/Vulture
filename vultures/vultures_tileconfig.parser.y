/* NetHack may be freely redistributed.  See license for details. */

/*
 * this is the parser for the tile configuration file; it recognizes the following
 * constructs and calls an associated function in vultures_tileconfig.c for each
 *  - tile specification (vultures_add_tile)
 *  - tile redirection (vultures_redirect_tile)
 *  - wall style specification (vultures_setup_wallstyle)
 *  - edge style specification (vultures_setup_wallstyle)
 *  - floor style specification (vultures_setup_wallstyle)
 */
%{
#include <stdio.h>
#include "vultures_types.h"
#include "vultures_tile.h"
#include "vultures_tileconfig.h"

extern int line_number;

void yyerror(const char *str)
{
    fprintf(stderr,"error: %s on line %d\n", str, line_number);
    exit(1);
}

int yywrap()
{
    return 1;
}

#define YYERROR_VERBOSE 1

%}


%union
{
        int number;
        char *string;
        struct {
            int type;
            char *name;
        } tileinfo;
        floortilerow row;
        struct {
            int rowcount;
            floortilerow *rows;
        } floorarray;
}

%token EOL REFERENCE WALLSTYLE EDGESTYLE FLOORSTYLE
%token <number> NUMBER
%token <string> STRING
%token <string> IDENTIFIER
%type <tileinfo> tile_id
%type <row> floortilerow
%type <floorarray> floortilearray


%start lines

%%
lines           : /* nothing */
                | lines line
                ;

line            : EOL
                | specline
                | redirline
                | wallsline
                | edgeline
                | floorline
                ;

                /* normal tile specification */
specline        : tile_id '=' STRING NUMBER NUMBER EOL 
                {
                    vultures_add_tile($1.type, $1.name, $3, $4, $5);
                    free($1.name); free($3);
                }
                ;

                /* redirection for a tile that does not exist */
redirline       : tile_id REFERENCE tile_id EOL
                {
                    vultures_redirect_tile($1.type, $1.name, $3.type, $3.name);
                    free($1.name); free($3.name);
                }
                ;

                /* wallstyle specification: 2 groups of 4 tiles as parameters (full height and half height) */
wallsline       : WALLSTYLE '(' STRING ')' '=' '(' IDENTIFIER IDENTIFIER IDENTIFIER IDENTIFIER ')'
                                               '(' IDENTIFIER IDENTIFIER IDENTIFIER IDENTIFIER ')' EOL
                {
                    vultures_setup_wallstyle(vultures_get_wallstyle_from_name($3), $7,  $8,  $9,  $10, $13, $14, $15, $16);
                    free($3);
                    free($7); free($8); free($9); free($10); free($13); free($14); free($15); free($16);
                }
                ;

                /* edgestyle specification: takes 12 tiles as parameters */
edgeline        : EDGESTYLE '(' STRING ')' '=' '(' IDENTIFIER IDENTIFIER IDENTIFIER IDENTIFIER IDENTIFIER IDENTIFIER
                                                   IDENTIFIER IDENTIFIER IDENTIFIER IDENTIFIER IDENTIFIER IDENTIFIER')' EOL
                {
                    vultures_setup_edgestyle(vultures_get_edgestyle_from_name($3), $7,  $8,  $9,  $10, $11, $12, $13, $14, $15, $16, $17, $18);
                    free($3);
                    free($7); free($8); free($9); free($10); free($11); free($12); free($13); free($14); free($15); free($16); free($17); free($18);
                }
                ;

                /* define a floorstyle, ie an X*Y pattern of tiles */
floorline       : FLOORSTYLE '(' STRING ')' '=' '(' NUMBER NUMBER floortilearray ')' EOL
                {
                    int i, j, style_id;

                    style_id = vultures_get_floorstyle_from_name($3);

                    if ($9.rowcount != $8)
                        YYERROR;

                    for (i = 0; i < $8; i++)
                        if ($9.rows[i].length != $7)
                            YYERROR;

                    int *tilearray = (int *)malloc($7 * $8 * sizeof(int));
                    for (i = 0; i < $8; i++)
                    {
                        for (j = 0; j < $7; j++)
                            tilearray[i * $7 + j] = $9.rows[i].tiles[j];
                        free($9.rows[i].tiles);
                    }
                    free($9.rows);

                    vultures_setup_floorstyle(style_id, $7, $8, tilearray);

                    free($3);
                }
                ;

floortilearray  : '(' floortilerow ')' /* first or only row of a floor array */
                {
                    $$.rowcount = 1;
                    $$.rows = (floortilerow *)malloc(sizeof(floortilerow));
                    $$.rows[0] = $2;
                }
                | floortilearray '(' floortilerow ')' /* following rows */
                {
                    $$.rows = $1.rows;
                    $$.rowcount = $1.rowcount;

                    $$.rowcount++;
                    $$.rows = (floortilerow *)realloc($$.rows, $$.rowcount * sizeof(floortilerow));
                    $$.rows[$$.rowcount - 1] = $3;
                }
                ;

floortilerow    : IDENTIFIER
                {
                    $$.length = 1;
                    $$.tiles = (int *)malloc(sizeof(int));
                    $$.tiles[0] = vultures_get_tile_index(TT_FLOOR, $1, 0);
                    free($1);

                    /* get tile index returns -1 for unknown identifiers */
                    if ($$.tiles[$$.length - 1] == -1)
                        YYERROR;
                }
                | floortilerow IDENTIFIER
                {
                    $$.length = $1.length;
                    $$.tiles = $1.tiles;

                    $$.length++;
                    $$.tiles = (int *)realloc($$.tiles, $$.length * sizeof(int));
                    $$.tiles[$$.length - 1] = vultures_get_tile_index(TT_FLOOR, $2, 0);
                    free($2);
                    if ($$.tiles[$$.length - 1] == -1)
                        YYERROR;
                }
                ;

                /* tiles are referred to in the form foo.bar (object.SPEAR, monster.PM_MASTER_LICH, etc) */
tile_id         : IDENTIFIER '.' IDENTIFIER
                {
                    $$.type = vultures_get_tiletype_from_name($1);
                    $$.name = $3;
                    free($1);
                }
                ;

%%
