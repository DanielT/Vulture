%{

#include "vultures_types.h"
#include "vultures_tile.h"
#include "vultures_tileconfig.h"
#include "vultures_tileconfig.parser.h"

int line_number = 1;

%}

%%

=>                      { return REFERENCE; }
WALLSTYLE               { return WALLSTYLE; }
EDGESTYLE               { return EDGESTYLE; }
FLOORSTYLE              { return FLOORSTYLE; }

#[^\n]*                 ; /* a comment */
([+]|[-])?[0-9]+        { yylval.number = atoi(yytext); return NUMBER; }
[a-zA-Z][a-zA-Z0-9_]+   { yylval.string = strdup(yytext); return IDENTIFIER; }
\"[^"\n]*\"             { yylval.string = strdup(yytext+1); yylval.string[yyleng-2] = '\0'; return STRING; }
[ \t]                   ;
\\\r?\n                 { line_number++; } /* escaped line break */
\r?\n                   { line_number++; return EOL; }
.                       { return (int) yytext[0]; }
%%