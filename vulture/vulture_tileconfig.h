/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vulture_tileconfig_h_
#define _vulture_tileconfig_h_

void vulture_add_tile(int tiletype, char *tilename, char *filename, int xoffset, int yoffset);

void vulture_redirect_tile(int tiletype, char *tilename, int redir_tiletype, char *redir_tilename);

void vulture_setup_wallstyle(int style_id, char * full_w, char * full_n, char * full_s, char * full_e,
                                   char * half_w, char * half_n, char * half_s, char * half_e);
void vulture_setup_edgestyle(int style_id, char *edge01, char *edge02, char *edge03, char *edge04,
                                   char *edge05, char *edge06, char *edge07, char *edge08,
                                   char *edge09, char *edge10, char *edge11, char *edge12);
void vulture_setup_floorstyle(int style_id, int x, int y, int *tilearray);

int vulture_get_floorstyle_from_name(char* name);
int vulture_get_wallstyle_from_name(char *name);
int vulture_get_edgestyle_from_name(char *name);

int vulture_get_tiletype_from_name(char* name);

int vulture_get_tile_index(int type, char * name, int allow_expand);

void vulture_parse_tileconf(FILE *fp, struct gametiles **gt_ptr);

extern void yyerror(const char *str);
extern int yyparse();
extern void yyrestart(FILE *fp);
extern int yyparse();
extern int yylex();


typedef struct  {
            int length;
            int *tiles;
        } floortilerow;

#endif
