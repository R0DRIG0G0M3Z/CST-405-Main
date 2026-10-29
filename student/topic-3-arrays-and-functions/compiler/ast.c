/* =========================================================================
 * CST-405  ·  TOPIC 3  ·  Compiling Complex Variables and Functions
 * FILE: ast.c   —   Phase 2 — AST implementation
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

extern int yylineno;


/* Basic nodes */

ASTNode* createNum(int value) {
    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_NUM;

    node->lineno =
        yylineno;

    node->data.num =
        value;

    return node;
}


ASTNode* createVar(char* name) {
    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_VAR;

    node->lineno =
        yylineno;

    node->data.name =
        strdup(name);

    return node;
}


ASTNode* createBinOp(
    char op,
    ASTNode* left,
    ASTNode* right) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_BINOP;

    node->lineno =
        yylineno;

    node->data.binop.op =
        op;

    node->data.binop.left =
        left;

    node->data.binop.right =
        right;

    return node;
}


ASTNode* createDecl(
    char* type,
    char* name) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_DECL;

    node->lineno =
        yylineno;

    node->data.decl.name =
        strdup(name);

    node->data.decl.varType =
        strdup(type);

    return node;
}


ASTNode* createAssign(
    char* var,
    ASTNode* value) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_ASSIGN;

    node->lineno =
        yylineno;

    node->data.assign.var =
        var ? strdup(var) : NULL;

    node->data.assign.value =
        value;

    node->data.assign.arrayLHS =
        NULL;

    return node;
}


ASTNode* createPrint(
    ASTNode* expr) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_PRINT;

    node->lineno =
        yylineno;

    node->data.expr =
        expr;

    return node;
}


ASTNode* createStmtList(
    ASTNode* stmt1,
    ASTNode* stmt2) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_STMT_LIST;

    node->lineno =
        stmt1
        ? stmt1->lineno
        : (stmt2
           ? stmt2->lineno
           : yylineno);

    node->data.stmtlist.stmt =
        stmt1;

    node->data.stmtlist.next =
        stmt2;

    return node;
}


/* Functions */

ASTNode* createFuncDef(
    char* name,
    ASTNode* params,
    ASTNode* body) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_FUNC_DEF;

    node->lineno =
        yylineno;

    node->data.func_def.name =
        strdup(name);

    node->data.func_def.params =
        params;

    node->data.func_def.body =
        body;

    return node;
}


ASTNode* createParam(
    char* name) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_PARAM;

    node->lineno =
        yylineno;

    node->data.param.name =
        strdup(name);

    return node;
}


ASTNode* createParamList(
    ASTNode* param,
    ASTNode* next) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_PARAM_LIST;

    node->lineno =
        param
        ? param->lineno
        : yylineno;

    node->data.param_list.param =
        param;

    node->data.param_list.next =
        next;

    return node;
}


ASTNode* createFuncCall(
    char* name,
    ASTNode* args) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_FUNC_CALL;

    node->lineno =
        yylineno;

    node->data.func_call.name =
        strdup(name);

    node->data.func_call.args =
        args;

    return node;
}


ASTNode* createArgList(
    ASTNode* expr,
    ASTNode* next) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_ARG_LIST;

    node->lineno =
        expr
        ? expr->lineno
        : yylineno;

    node->data.arg_list.expr =
        expr;

    node->data.arg_list.next =
        next;

    return node;
}


ASTNode* createReturn(
    ASTNode* expr) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_RETURN;

    node->lineno =
        yylineno;

    node->data.ret.expr =
        expr;

    return node;
}


/* Blocks */

ASTNode* createBlock(
    ASTNode* stmt_list) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_BLOCK;

    node->lineno =
        yylineno;

    node->data.block.stmt_list =
        stmt_list;

    return node;
}


/* Arrays */

ASTNode* createArrayDecl(
    char* name,
    int size) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_ARRAY_DECL;

    node->lineno =
        yylineno;

    node->data.array_decl.name =
        strdup(name);

    node->data.array_decl.size =
        size;

    node->data.array_decl.isParam =
        0;

    return node;
}


ASTNode* createArrayParam(
    char* name) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_ARRAY_DECL;

    node->lineno =
        yylineno;

    node->data.array_decl.name =
        strdup(name);

    node->data.array_decl.size =
        0;

    node->data.array_decl.isParam =
        1;

    return node;
}


ASTNode* createArrayIndex(
    char* name,
    ASTNode* index) {

    ASTNode* node =
        malloc(sizeof(ASTNode));

    node->type =
        NODE_ARRAY_INDEX;

    node->lineno =
        yylineno;

    node->data.array_index.name =
        strdup(name);

    node->data.array_index.index =
        index;

    return node;
}


/* Operator text */

const char* opText(char op) {
    switch (op) {
        case '+':
            return "+";

        case '-':
            return "-";

        case '*':
            return "*";

        case '/':
            return "/";

        case '<':
            return "<";

        case '>':
            return ">";

        case 'l':
            return "<=";

        case 'g':
            return ">=";

        case 'e':
            return "==";

        case 'n':
            return "!=";

        case 'u':
            return "unary -";

        case '&':
            return "&&";

        case '|':
            return "||";

        case '!':
            return "!";

        default:
            return "?";
    }
}


/* AST display */

void printAST(
    ASTNode* node,
    int level) {

    if (!node)
        return;

    for (int i = 0;
         i < level;
         i++) {

        printf("  ");
    }

    switch(node->type) {

        case NODE_NUM:
            printf(
                "NUM: %d\n",
                node->data.num);
            break;


        case NODE_VAR:
            printf(
                "VAR: %s\n",
                node->data.name);
            break;


        case NODE_BINOP:
            printf(
                "BINOP: %s\n",
                opText(
                    node->data.binop.op));

            printAST(
                node->data.binop.left,
                level + 1);

            printAST(
                node->data.binop.right,
                level + 1);

            break;


        case NODE_DECL:
            printf(
                "DECL: %s %s\n",
                node->data.decl.varType,
                node->data.decl.name);
            break;


        case NODE_ASSIGN:
            if (node->data.assign.arrayLHS) {
                printf(
                    "ASSIGN (array element)\n");

                for (int i = 0;
                     i < level + 1;
                     i++) {

                    printf("  ");
                }

                printf("LHS:\n");

                printAST(
                    node->data.assign.arrayLHS,
                    level + 2);

            } else {
                printf(
                    "ASSIGN: %s\n",
                    node->data.assign.var);
            }

            for (int i = 0;
                 i < level + 1;
                 i++) {

                printf("  ");
            }

            printf("VALUE:\n");

            printAST(
                node->data.assign.value,
                level + 2);

            break;


        case NODE_PRINT:
            printf("PRINT\n");

            printAST(
                node->data.expr,
                level + 1);

            break;


        case NODE_STMT_LIST:
            printAST(
                node->data.stmtlist.stmt,
                level);

            printAST(
                node->data.stmtlist.next,
                level);

            break;


        case NODE_FUNC_DEF:
            printf(
                "FUNC_DEF: %s\n",
                node->data.func_def.name);

            if (node->data.func_def.params) {
                for (int i = 0;
                     i < level + 1;
                     i++) {

                    printf("  ");
                }

                printf("PARAMS:\n");

                printAST(
                    node->data.func_def.params,
                    level + 2);
            }

            for (int i = 0;
                 i < level + 1;
                 i++) {

                printf("  ");
            }

            printf("BODY:\n");

            printAST(
                node->data.func_def.body,
                level + 2);

            break;


        case NODE_PARAM:
            printf(
                "PARAM: %s\n",
                node->data.param.name);
            break;


        case NODE_PARAM_LIST:
            printAST(
                node->data.param_list.param,
                level);

            printAST(
                node->data.param_list.next,
                level);

            break;


        case NODE_FUNC_CALL:
            printf(
                "FUNC_CALL: %s\n",
                node->data.func_call.name);

            if (node->data.func_call.args) {
                for (int i = 0;
                     i < level + 1;
                     i++) {

                    printf("  ");
                }

                printf("ARGS:\n");

                printAST(
                    node->data.func_call.args,
                    level + 2);
            }

            break;


        case NODE_ARG_LIST:
            printAST(
                node->data.arg_list.expr,
                level);

            printAST(
                node->data.arg_list.next,
                level);

            break;


        case NODE_RETURN:
            printf("RETURN\n");

            if (node->data.ret.expr) {
                printAST(
                    node->data.ret.expr,
                    level + 1);
            }

            break;


        case NODE_BLOCK:
            printf("BLOCK\n");

            printAST(
                node->data.block.stmt_list,
                level + 1);

            break;


        case NODE_ARRAY_DECL:
            if (node->data.array_decl.isParam) {
                printf(
                    "ARRAY_PARAM: %s[]\n",
                    node->data.array_decl.name);

            } else {
                printf(
                    "ARRAY_DECL: %s[%d]\n",
                    node->data.array_decl.name,
                    node->data.array_decl.size);
            }

            break;


        case NODE_ARRAY_INDEX:
            printf(
                "ARRAY_INDEX: %s\n",
                node->data.array_index.name);

            for (int i = 0;
                 i < level + 1;
                 i++) {

                printf("  ");
            }

            printf("INDEX:\n");

            printAST(
                node->data.array_index.index,
                level + 2);

            break;
    }
}