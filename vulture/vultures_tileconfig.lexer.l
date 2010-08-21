/* NetHack may be freely redistributed.  See license for details. */

%option nounput

%{

#include "vultures_types.h"
#include "vultures_tile.h"
#include "vultures_tileconfig.h"
#include "vultures_tileconfig.parser.h"

#define YY_SKIP_YYWRAP
extern int yywrap();
int line_number = 1;

%}

%%
=>                      { return REFERENCE; }
WALLSTYLE               { return WALLSTYLE; }
EDGESTYLE               { return EDGESTYLE; }
FLOORSTYLE              { return FLOORSTYLE; }

#[^\n]*                 ; /* a comment (discarded) */
([+]|[-])?[0-9]+        { yylval.number = atoi(yytext); return NUMBER; }
[a-zA-Z][a-zA-Z0-9_]+   { yylval.string = strdup(yytext); return IDENTIFIER; }
\"[^"\n]*\"             { yylval.string = strdup(yytext+1); yylval.string[yyleng-2] = '\0'; return STRING; }
[ \t]                   ; /* whitespace (discarded) */
\\\r?\n                 { line_number++; } /* escaped line break */
\r?\n                   { line_number++; return EOL; }
.                       { return (int) yytext[0]; } /* everything else is passed to the parser character-by-character */
%%
