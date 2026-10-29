/* =========================================================================
 * CST-405  ·  TOPIC 3  ·  Compiling Complex Variables and Functions
 * FILE: scanner.l   —   Phase 1 — Lexical analysis
 * -------------------------------------------------------------------------
 * THE PIPELINE, AND WHERE THIS FILE SITS IN IT
 *   scanner -> parser -> ast -> semantic -> tac -> codegen
 *   ^^^^^^^  this file
 *
 * WHAT IS NEW IN TOPIC 3
 *   • Tokens for `[`, `]`, `,`, `{`, `}` and the arithmetic operators
 *   • `return` keyword for functions
 *
 * WHAT COMES NEXT
 *   Topic 4 adds loops (while, for, break) and the label/jump machinery they need.
 *
 * YOUR TASK
 *   Everything below marked TODO (Topic 3) is yours to write.
 *   Everything else already works — it is the Topic 2 compiler you
 *   have already built and tested. Do not rewrite it; extend it.
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