/*
 * CST-405 Project 1 Lexical Analyzer
 * Rodrigo Gomez
 */

%{
#include <stdio.h>
#include <string.h>
#include "tokens.h"

int current_line = 1;
int current_column = 1;
int token_line = 1;
int token_column = 1;
int lexical_errors = 0;
int comment_start_line = 1;
int comment_start_column = 1;
char scanner_error_message[100] = "";
char scanner_error_lexeme[20] = "";

/*
 * This saves where a match started, then moves the position forward.
 * Doing it for every match also counts newlines inside block comments.
 */
static void update_position(const char *text, int length)
{
    int i;

    token_line = current_line;
    token_column = current_column;

    for (i = 0; i < length; i++) {
        if (text[i] == '\n') {
            current_line++;
            current_column = 1;
        } else {
            current_column++;
        }
    }
}

#define YY_USER_ACTION update_position(yytext, yyleng);
%}

/* Input comes from a file, so interactive terminal checks are not needed. */
%option noyywrap nodefault nounput noinput never-interactive
%x COMMENT

DIGIT       [0-9]
ID_START    [A-Za-z_]
ID_CHAR     [A-Za-z0-9_]

%%

 /* Keywords stay above ID because first match decides equal-length matches. */
"int"           { return INT; }
"print"         { return PRINT; }
"return"        { return RETURN; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"for"           { return FOR; }
"switch"        { return SWITCH; }
"case"          { return CASE; }
"default"       { return DEFAULT; }
"break"         { return BREAK; }

{ID_START}{ID_CHAR}*  { return ID; }
{DIGIT}+              { return NUM; }

 /* Flex chooses these over the one-character rules by longest match. */
"<="            { return LE; }
">="            { return GE; }
"=="            { return EQ; }
"!="            { return NE; }
"&&"            { return AND; }
"||"            { return OR; }

"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"<"             { return '<'; }
">"             { return '>'; }
"!"             { return '!'; }
"="             { return '='; }

";"             { return ';'; }
":"             { return ':'; }
","             { return ','; }
"("             { return '('; }
")"             { return ')'; }
"{"             { return '{'; }
"}"             { return '}'; }
"["             { return '['; }
"]"             { return ']'; }

"//"[^\n]*      { /* A line comment does not produce a token. */ }

"/*"            {
                    comment_start_line = token_line;
                    comment_start_column = token_column;
                    BEGIN(COMMENT);
                }
<COMMENT>"*/"    { BEGIN(INITIAL); }
<COMMENT>\n      { /* Position was already updated by YY_USER_ACTION. */ }
<COMMENT>.       { /* Keep reading until the closing delimiter. */ }
<COMMENT><<EOF>> {
                    lexical_errors++;
                    token_line = comment_start_line;
                    token_column = comment_start_column;
                    strcpy(scanner_error_lexeme, "/*");
                    strcpy(scanner_error_message, "unterminated block comment");
                    BEGIN(INITIAL);
                    return TOKEN_ERROR;
                }

[ \t\r\n]+      { /* Whitespace is ignored but its position is counted. */ }

.               {
                    lexical_errors++;
                    snprintf(scanner_error_lexeme, sizeof(scanner_error_lexeme),
                             "%s", yytext);
                    strcpy(scanner_error_message, "unknown character");
                    return TOKEN_ERROR;
                }

%%
