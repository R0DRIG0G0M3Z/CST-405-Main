/* =========================================================================
 * CST-405  ·  TOPIC 3  ·  Compiling Complex Variables and Functions
 * FILE: parser.y   —   Phase 2 — Syntax analysis
 * INPUT: tokens from scanner.l
 * OUTPUT: an abstract syntax tree rooted at `root`
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

/* A legacy Topic 2 program has no function definition.  Keeping that form
 * valid lets the previous regression tests run unchanged; it is wrapped in
 * an implicit main function after parsing. */
static int containsFunction(ASTNode* node);
%}

%union {
    int num;
    char* str;
    struct ASTNode* node;
}

%token <num> NUM
%token <str> ID
%token INT PRINT RETURN IF

%type <node> program
%type <node> decl_or_func_list decl_or_func
%type <node> func_def params param_list param
%type <node> block
%type <node> stmt_list stmt
%type <node> decl assign return_stmt
%type <node> expr
%type <node> func_call args arg_list
%type <node> print_stmt
%type <node> if_stmt

%left '+' '-'
%left '*' '/'
%right UMINUS

%%

program:
    decl_or_func_list {
        root = containsFunction($1)
             ? $1
             : createFuncDef("main", NULL, createBlock($1));
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
    | stmt {
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
    | if_stmt
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

/* A small truthy/falsy branch is included here because Project 3 requires a
 * recursive factorial demonstration.  Full if/else syntax remains Topic 5. */
if_stmt:
    IF '(' expr ')' stmt {
        $$ = createIf($3, $5);
    }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr,
            "Syntax Error at line %d: %s\n",
            yylineno,
            s);
}

static int containsFunction(ASTNode* node) {
    if (!node) return 0;
    if (node->type == NODE_FUNC_DEF) return 1;
    if (node->type != NODE_STMT_LIST) return 0;
    return containsFunction(node->data.stmtlist.stmt) ||
           containsFunction(node->data.stmtlist.next);
}
