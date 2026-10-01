/*
 * CST-405 Project 1 Lexer driver
 * Rodrigo Gomez
 */

#include <stdio.h>
#include <string.h>
#include "tokens.h"

extern FILE *yyin;
extern char *yytext;
int yylex(void);

/* Converts the numeric token value into the name shown in the table. */
static const char *tokenName(int token)
{
    switch (token) {
        case ID: return "ID";
        case NUM: return "NUM";
        case INT: return "INT";
        case PRINT: return "PRINT";
        case RETURN: return "RETURN";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case WHILE: return "WHILE";
        case FOR: return "FOR";
        case SWITCH: return "SWITCH";
        case CASE: return "CASE";
        case DEFAULT: return "DEFAULT";
        case BREAK: return "BREAK";
        case LE: return "LE";
        case GE: return "GE";
        case EQ: return "EQ";
        case NE: return "NE";
        case AND: return "AND";
        case OR: return "OR";
        case '+': return "PLUS";
        case '-': return "MINUS";
        case '*': return "TIMES";
        case '/': return "DIVIDE";
        case '<': return "LT";
        case '>': return "GT";
        case '!': return "NOT";
        case '=': return "ASSIGN";
        case ';': return "SEMICOLON";
        case ':': return "COLON";
        case ',': return "COMMA";
        case '(': return "LPAREN";
        case ')': return "RPAREN";
        case '{': return "LBRACE";
        case '}': return "RBRACE";
        case '[': return "LBRACKET";
        case ']': return "RBRACKET";
        default: return "UNKNOWN";
    }
}

/* Groups tokens so the output is easier to read and check. */
static const char *tokenCategory(int token)
{
    switch (token) {
        case INT: case PRINT: case RETURN: case IF: case ELSE:
        case WHILE: case FOR: case SWITCH: case CASE: case DEFAULT:
        case BREAK:
            return "KEYWORD";
        case ID:
            return "IDENTIFIER";
        case NUM:
            return "INTEGER";
        case LE: case GE: case EQ: case NE: case AND: case OR:
        case '+': case '-': case '*': case '/': case '<': case '>':
        case '!': case '=':
            return "OPERATOR";
        case ';': case ':': case ',': case '(': case ')':
        case '{': case '}': case '[': case ']':
            return "DELIMITER";
        default:
            return "ERROR";
    }
}

int main(int argc, char *argv[])
{
    int token;
    int counts_only = 0;
    int total_tokens = 0;
    int keyword_count = 0;
    int identifier_count = 0;
    int integer_count = 0;
    int operator_count = 0;
    int delimiter_count = 0;

    if (argc == 3 && strcmp(argv[2], "-c") == 0)
        counts_only = 1;
    else if (argc != 2) {
        fprintf(stderr, "Usage: %s <sourcefile.cm> [-c]\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        perror("Could not open input file");
        return 1;
    }

    if (!counts_only) {
        printf("%-14s %-12s %-18s %s\n",
               "TOKEN", "CATEGORY", "TEXT", "LOCATION");
        printf("---------------------------------------------------------------\n");
    }

    while ((token = yylex()) != 0) {
        const char *category;

        if (token == TOKEN_ERROR) {
            fprintf(stderr, "Lexical Error at line %d, column %d: '%s' (%s)\n",
                    token_line, token_column, scanner_error_lexeme,
                    scanner_error_message);
            continue;
        }

        category = tokenCategory(token);
        total_tokens++;

        if (strcmp(category, "KEYWORD") == 0) keyword_count++;
        else if (strcmp(category, "IDENTIFIER") == 0) identifier_count++;
        else if (strcmp(category, "INTEGER") == 0) integer_count++;
        else if (strcmp(category, "OPERATOR") == 0) operator_count++;
        else if (strcmp(category, "DELIMITER") == 0) delimiter_count++;

        if (!counts_only) {
            printf("%-14s %-12s %-18s line %d, column %d\n",
                   tokenName(token), category, yytext,
                   token_line, token_column);
        }
    }

    fclose(yyin);

    if (counts_only) {
        printf("Tokens: %d (keywords %d, identifiers %d, integers %d, "
               "operators %d, delimiters %d)\n",
               total_tokens, keyword_count, identifier_count, integer_count,
               operator_count, delimiter_count);
    }

    if (lexical_errors == 0) {
        printf("Source is lexically clean: 0 errors.\n");
        return 0;
    }

    printf("Lexical analysis finished with %d error(s).\n", lexical_errors);
    return 1;
}
