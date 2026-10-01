/*
 * CST-405 Project 1 Token definitions
 */

#ifndef TOKENS_H
#define TOKENS_H

/* Single-character tokens such as '+' and ';' use their ASCII value. */
enum TokenKind {
    TOKEN_ERROR = 256,
    ID,
    NUM,
    INT,
    PRINT,
    RETURN,
    IF,
    ELSE,
    WHILE,
    FOR,
    SWITCH,
    CASE,
    DEFAULT,
    BREAK,
    LE,
    GE,
    EQ,
    NE,
    AND,
    OR
};

/* The driver uses these values to print the scanner result. */
extern int token_line;
extern int token_column;
extern int lexical_errors;
extern char scanner_error_message[100];
extern char scanner_error_lexeme[20];

#endif
