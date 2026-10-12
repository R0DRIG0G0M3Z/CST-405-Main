/* CST-405 Project 2: lexical analysis.
 * Input: source characters. Output: tokens, values, and exact locations.
 * All matches advance the location, including ignored whitespace/comments.
 * Invalid characters and unterminated comments increment lexicalErrors,
 * so they can never be silently skipped and certified as valid source.
 */
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "parser.tab.h"
int lexicalErrors=0;
static int line=1,column=1,commentLine,commentColumn;
#define YY_USER_ACTION updateLocation(yytext,yyleng);
static void updateLocation(const char* text,int length) {
    yylloc.first_line=line; yylloc.first_column=column;
    for(int i=0;i<length;i++) {
        yylloc.last_line=line; yylloc.last_column=column;
        if(text[i]=='\n') { line++; column=1; } else column++;
    }
    yylineno=line;
}
static void lexicalError(const char* message) {
    lexicalErrors++;
    fprintf(stderr,"Lexical error at line %d, column %d: %s\n",
            yylloc.first_line,yylloc.first_column,message);
}
%}
%option noyywrap nounput noinput nodefault
%x COMMENT
%%
"int" { return INT; }
"print" { return PRINT; }
[a-zA-Z_][a-zA-Z0-9_]* {
    /* The supplied register allocator stores names in a 64-byte field. */
    if(yyleng>63) { lexicalError("identifier exceeds 63 characters"); return INVALID; }
    yylval.str=strdup(yytext); return ID;
}
[0-9]+ {
    errno=0; long value=strtol(yytext,NULL,10);
    if(errno==ERANGE || value>INT_MAX) {
        lexicalError("integer literal exceeds 2147483647"); return INVALID;
    }
    yylval.num=(int)value; return NUM;
}
[+=;()] { return (unsigned char)yytext[0]; }
"//"[^\n]* { /* line comment */ }
"/*" { commentLine=yylloc.first_line; commentColumn=yylloc.first_column; BEGIN(COMMENT); }
<COMMENT>"*/" { BEGIN(INITIAL); }
<COMMENT>[^*\n]+ { }
<COMMENT>"*" { }
<COMMENT>\n { }
<COMMENT><<EOF>> {
    lexicalErrors++;
    fprintf(stderr,"Lexical error at line %d, column %d: unterminated block comment\n",commentLine,commentColumn);
    BEGIN(INITIAL); return 0;
}
[ \t\r\n]+ { }
. { char message[64]; snprintf(message,sizeof message,"unknown character '%c'",yytext[0]);
    lexicalError(message); return INVALID; }
<<EOF>> { yylloc.first_line=line; yylloc.first_column=column; return 0; }
%%
