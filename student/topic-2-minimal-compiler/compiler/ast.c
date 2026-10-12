/* CST-405 Project 2: AST construction and display.
 * Input: Bison reductions. Output: owned nodes consumed by semantics and TAC.
 * The AST discards punctuation; parse_tree.c keeps it for comparison.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
extern int yylineno;

static ASTNode* node(NodeType type) {
    ASTNode* n = calloc(1, sizeof *n);
    if (!n) { perror("AST allocation"); exit(1); }
    n->type = type; n->lineno = yylineno; n->colno = 1;
    return n;
}
ASTNode* createNum(int value) { ASTNode* n=node(NODE_NUM); n->data.num=value; return n; }
ASTNode* createVar(char* name) { ASTNode* n=node(NODE_VAR); n->data.name=strdup(name); return n; }
ASTNode* createBinOp(char op, ASTNode* left, ASTNode* right) {
    ASTNode* n=node(NODE_BINOP); n->data.binop.op=op;
    n->data.binop.left=left; n->data.binop.right=right; return n;
}
ASTNode* createDecl(char* type, char* name) {
    ASTNode* n=node(NODE_DECL); n->data.decl.varType=strdup(type);
    n->data.decl.name=strdup(name); return n;
}
ASTNode* createAssign(char* var, ASTNode* value) {
    ASTNode* n=node(NODE_ASSIGN); n->data.assign.var=strdup(var);
    n->data.assign.value=value; return n;
}
ASTNode* createPrint(ASTNode* expr) { ASTNode* n=node(NODE_PRINT); n->data.expr=expr; return n; }
ASTNode* createStmtList(ASTNode* first, ASTNode* second) {
    if (!first) return second;
    if (!second) return first;
    /* Left-recursive grammar: the left child is the preceding sequence.
     * Every visitor walks it before the newly appended right child. */
    ASTNode* n=node(NODE_STMT_LIST);
    n->data.stmtlist.stmt=first; n->data.stmtlist.next=second; return n;
}
const char* opText(char op) { return op=='+' ? "+" : "?"; }
void printAST(ASTNode* n, int level) {
    if (!n) return;
    if (n->type==NODE_STMT_LIST) {
        printAST(n->data.stmtlist.stmt, level);
        printAST(n->data.stmtlist.next, level); return;
    }
    printf("%*s", level*2, "");
    switch(n->type) {
    case NODE_NUM: printf("NUM %d",n->data.num); break;
    case NODE_VAR: printf("VAR %s",n->data.name); break;
    case NODE_BINOP: printf("ADD"); break;
    case NODE_DECL: printf("DECL %s %s",n->data.decl.varType,n->data.decl.name); break;
    case NODE_ASSIGN: printf("ASSIGN %s",n->data.assign.var); break;
    case NODE_PRINT: printf("PRINT"); break;
    default: break;
    }
    printf(" [line %d, column %d]\n",n->lineno,n->colno);
    if(n->type==NODE_BINOP) {
        printAST(n->data.binop.left,level+1); printAST(n->data.binop.right,level+1);
    } else if(n->type==NODE_ASSIGN) printAST(n->data.assign.value,level+1);
    else if(n->type==NODE_PRINT) printAST(n->data.expr,level+1);
}
/* Nodes own duplicated names and their child nodes. Bison also uses this
 * destructor to release partially built ASTs during error recovery. */
void freeAST(ASTNode* n) {
    if(!n) return;
    switch(n->type) {
    case NODE_VAR: free(n->data.name); break;
    case NODE_DECL: free(n->data.decl.name); free(n->data.decl.varType); break;
    case NODE_BINOP: freeAST(n->data.binop.left); freeAST(n->data.binop.right); break;
    case NODE_ASSIGN: free(n->data.assign.var); freeAST(n->data.assign.value); break;
    case NODE_PRINT: freeAST(n->data.expr); break;
    case NODE_STMT_LIST: freeAST(n->data.stmtlist.stmt); freeAST(n->data.stmtlist.next); break;
    default: break;
    }
    free(n);
}
