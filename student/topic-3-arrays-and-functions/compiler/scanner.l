/* =========================================================================
 * CST-405  ·  TOPIC 3  ·  Compiling Complex Variables and Functions
 * FILE: scanner.l   —   Phase 1 — Lexical analysis
 * INPUT: C-Minus source characters
 * OUTPUT: tokens and token values consumed by parser.y
 * ========================================================================= */

%{
#include <stdio.h>
#include <stdlib.h>
#include "parser.tab.h"
%}

%option nounput
%option noinput
%option yylineno

%%

"int"           { return INT; }
"print"         { return PRINT; }
"return"        { return RETURN; }
"if"            { return IF; }

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str = strdup(yytext);
    return ID;
}

[0-9]+ {
    yylval.num = atoi(yytext);
    return NUM;
}

"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }

"="             { return '='; }

";"             { return ';'; }
"("             { return '('; }
")"             { return ')'; }
"{"             { return '{'; }
"}"             { return '}'; }
"["             { return '['; }
"]"             { return ']'; }
","             { return ','; }

"//".*          { /* Skip single-line comments */ }
"/*"([^*]|\*+[^*/])*\*+"/"  { /* Skip multi-line comments */ }

[ \t\n]         { }

. {
    printf("Lexical Error: Unknown character '%c'\n", *yytext);
}

%%

int yywrap() {
    return 1;
}
