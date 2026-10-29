/* =========================================================================
 * CST-405  ·  TOPIC 3  ·  Compiling Complex Variables and Functions
 * FILE: parser.y   —   Phase 2 — Syntax analysis
 * -------------------------------------------------------------------------
 * THE PIPELINE, AND WHERE THIS FILE SITS IN IT
 *   scanner -> parser -> ast -> semantic -> tac -> codegen
 *              ^^^^^^  this file
 *
 * WHAT IS NEW IN TOPIC 3
 *   • A program is now a list of declarations AND function definitions
 *   • Array declaration `int a[10];`, indexing `a[i]`, and array parameters `int a[]`
 *   • Full arithmetic with precedence: + - * / , parentheses, unary minus
 *   • Function definitions, parameter lists, calls, and `return`
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
#include <string.h>
#include "ast.h"

extern int yylex();
extern int yyparse();
extern FILE* yyin;
extern int yylineno;

void yyerror(const char* s);

ASTNode* root = NULL;
%}

%union {
    int num;
    char* str;
    struct ASTNode* node;
}

%token <num> NUM
%token <str> ID
%token INT PRINT RETURN

%type <node> program
%type <node> decl_or_func_list decl_or_func
%type <node> func_def params param_list param
%type <node> block
%type <node> stmt_list stmt
%type <node> decl assign return_stmt
%type <node> expr
%type <node> func_call args arg_list
%type <node> print_stmt

%left '+' '-'
%left '*' '/'
%right UMINUS

%%

program:
    decl_or_func_list {
        root = $1;
    }
    ;

decl_or_func_list:
    decl_or_func {
        $$ = $1;
    }
    | decl_or_func_list decl_or_func {
        $$ = createStmtList($1, $2);
    }
    ;

decl_or_func:
    func_def {
        $$ = $1;
    }
    | decl {
        $$ = $1;
    }
    ;

func_def:
    INT ID '(' params ')' block {
        $$ = createFuncDef($2, $4, $6);
        free($2);
    }
    | INT ID '(' ')' block {
        $$ = createFuncDef($2, NULL, $5);
        free($2);
    }
    ;

params:
    param_list {
        $$ = $1;
    }
    ;

param_list:
    param {
        $$ = $1;
    }
    | param_list ',' param {
        $$ = createParamList($1, $3);
    }
    ;

param:
    INT ID {
        $$ = createParam($2);
        free($2);
    }
    | INT ID '[' ']' {
        $$ = createArrayParam($2);
        free($2);
    }
    ;

block:
    '{' stmt_list '}' {
        $$ = createBlock($2);
    }
    | '{' '}' {
        $$ = createBlock(NULL);
    }
    ;

stmt_list:
    stmt {
        $$ = $1;
    }
    | stmt_list stmt {
        $$ = createStmtList($1, $2);
    }
    ;

stmt:
    decl
    | assign
    | print_stmt
    | return_stmt
    | block
    | func_call ';' {
        $$ = $1;
    }
    ;

decl:
    INT ID ';' {
        $$ = createDecl("int", $2);
        free($2);
    }
    | INT ID '[' NUM ']' ';' {
        $$ = createArrayDecl($2, $4);
        free($2);
    }
    ;

assign:
    ID '=' expr ';' {
        $$ = createAssign($1, $3);
        free($1);
    }
    | ID '[' expr ']' '=' expr ';' {
        ASTNode* lhs = createArrayIndex($1, $3);

        $$ = createAssign(NULL, $6);
        $$->data.assign.arrayLHS = lhs;

        free($1);
    }
    ;

return_stmt:
    RETURN expr ';' {
        $$ = createReturn($2);
    }
    | RETURN ';' {
        $$ = createReturn(NULL);
    }
    ;

expr:
    NUM {
        $$ = createNum($1);
    }
    | ID {
        $$ = createVar($1);
        free($1);
    }
    | ID '[' expr ']' {
        $$ = createArrayIndex($1, $3);
        free($1);
    }
    | func_call {
        $$ = $1;
    }
    | expr '+' expr {
        $$ = createBinOp('+', $1, $3);
    }
    | expr '-' expr {
        $$ = createBinOp('-', $1, $3);
    }
    | expr '*' expr {
        $$ = createBinOp('*', $1, $3);
    }
    | expr '/' expr {
        $$ = createBinOp('/', $1, $3);
    }
    | '-' expr %prec UMINUS {
        $$ = createBinOp('u', $2, NULL);
    }
    | '(' expr ')' {
        $$ = $2;
    }
    ;

func_call:
    ID '(' args ')' {
        $$ = createFuncCall($1, $3);
        free($1);
    }
    | ID '(' ')' {
        $$ = createFuncCall($1, NULL);
        free($1);
    }
    ;

args:
    arg_list {
        $$ = $1;
    }
    ;

arg_list:
    expr {
        $$ = $1;
    }
    | arg_list ',' expr {
        $$ = createArgList($1, $3);
    }
    ;

print_stmt:
    PRINT '(' expr ')' ';' {
        $$ = createPrint($3);
    }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr,
            "Syntax Error at line %d: %s\n",
            yylineno,
            s);
}