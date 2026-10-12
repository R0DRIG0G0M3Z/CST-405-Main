/* CST-405 Project 2: syntax analysis.
 * Input: Flex tokens with locations. Output: AST and concrete parse tree.
 * This is exactly the Topic 2 grammar: declarations, assignments, addition,
 * and print. '+' is left associative; parentheses belong only to print.
 */
%code requires {
#include "ast.h"
#include "parse_tree.h"
}
%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "parse_tree.h"
extern int yylex(void);
extern int yylineno;
void yyerror(const char* message);
ASTNode* root=NULL;
ParseTree* parseTreeRoot=NULL;
int syntaxErrors=0;
static ASTNode* located(ASTNode* n,int line,int column) {
    if(n) { n->lineno=line; n->colno=column; } return n;
}
#define LEAF(text) createParseTree(text,0)
%}
%locations
%define parse.error detailed
%union {
    int num;
    char* str;
    struct { struct ASTNode* ast; struct ParseTree* tree; } parsed;
}
%token <num> NUM
%token <str> ID
%token INT PRINT INVALID
%type <parsed> stmt_list stmt decl assign expr print_stmt
%destructor { free($$); } <str>
%destructor { freeAST($$.ast); freeParseTree($$.tree); } <parsed>
%left '+'
%%
/* The start symbol is untyped: Bison must not destroy the completed trees
 * when it accepts. main.c owns them after this reduction. */
program: stmt_list { root=$1.ast; parseTreeRoot=createParseTree("program",1,$1.tree); };
stmt_list:
    stmt { $$.ast=$1.ast; $$.tree=createParseTree("stmt_list",1,$1.tree); }
  | stmt_list stmt { $$.ast=createStmtList($1.ast,$2.ast);
                    $$.tree=createParseTree("stmt_list",2,$1.tree,$2.tree); }
  ;
stmt:
    decl { $$.ast=$1.ast; $$.tree=createParseTree("stmt",1,$1.tree); }
  | assign { $$.ast=$1.ast; $$.tree=createParseTree("stmt",1,$1.tree); }
  | print_stmt { $$.ast=$1.ast; $$.tree=createParseTree("stmt",1,$1.tree); }
  /* Synchronize at a semicolon and continue reporting other bad statements.
   * syntaxErrors stays nonzero even if yyparse() eventually accepts. */
  | error ';' { $$.ast=NULL; $$.tree=LEAF("invalid statement"); yyerrok; }
  ;
decl: INT ID ';' {
    $$.ast=located(createDecl("int",$2),@2.first_line,@2.first_column);
    $$.tree=createParseTree("decl",3,LEAF("int"),LEAF($2),LEAF(";")); free($2);
};
assign: ID '=' expr ';' {
    $$.ast=located(createAssign($1,$3.ast),@1.first_line,@1.first_column);
    $$.tree=createParseTree("assign",4,LEAF($1),LEAF("="),$3.tree,LEAF(";")); free($1);
};
expr:
    NUM { char label[32]; snprintf(label,sizeof label,"%d",$1);
          $$.ast=located(createNum($1),@1.first_line,@1.first_column);
          $$.tree=createParseTree("expr",1,LEAF(label)); }
  | ID { $$.ast=located(createVar($1),@1.first_line,@1.first_column);
         $$.tree=createParseTree("expr",1,LEAF($1)); free($1); }
  | expr '+' expr {
        $$.ast=located(createBinOp('+',$1.ast,$3.ast),@2.first_line,@2.first_column);
        $$.tree=createParseTree("expr",3,$1.tree,LEAF("+"),$3.tree);
    }
  ;
print_stmt: PRINT '(' expr ')' ';' {
    $$.ast=located(createPrint($3.ast),@1.first_line,@1.first_column);
    $$.tree=createParseTree("print_stmt",5,LEAF("print"),LEAF("("),$3.tree,LEAF(")"),LEAF(";"));
};
%%
void yyerror(const char* message) {
    syntaxErrors++;
    fprintf(stderr,"Syntax error at line %d, column %d: %s\n",
        yylloc.first_line,yylloc.first_column,message);
    fprintf(stderr,"  Check the preceding statement for a missing ';' or the print expression for ')'.\n");
}
