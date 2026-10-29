/* =========================================================================
 * CST-405  ·  TOPIC 3  ·  Compiling Complex Variables and Functions
 * FILE: ast.h   —   Phase 2 — Syntax analysis (the tree it builds)
 * ========================================================================= */

#ifndef AST_H
#define AST_H

typedef enum {
    NODE_NUM,
    NODE_VAR,
    NODE_BINOP,
    NODE_DECL,
    NODE_ASSIGN,
    NODE_PRINT,
    NODE_STMT_LIST,

    NODE_FUNC_DEF,
    NODE_PARAM,
    NODE_PARAM_LIST,
    NODE_FUNC_CALL,
    NODE_ARG_LIST,
    NODE_RETURN,

    NODE_BLOCK,
    NODE_ARRAY_DECL,
    NODE_ARRAY_INDEX

} NodeType;

typedef struct ASTNode {
    NodeType type;
    int lineno;

    union {
        int num;

        char* name;

        struct {
            char* name;
            char* varType;
        } decl;

        struct {
            char op;
            struct ASTNode* left;
            struct ASTNode* right;
        } binop;

        struct {
            char* var;
            struct ASTNode* value;
            struct ASTNode* arrayLHS;
        } assign;

        struct ASTNode* expr;

        struct {
            struct ASTNode* stmt;
            struct ASTNode* next;
        } stmtlist;

        struct {
            char* name;
            struct ASTNode* params;
            struct ASTNode* body;
        } func_def;

        struct {
            char* name;
        } param;

        struct {
            struct ASTNode* param;
            struct ASTNode* next;
        } param_list;

        struct {
            char* name;
            struct ASTNode* args;
        } func_call;

        struct {
            struct ASTNode* expr;
            struct ASTNode* next;
        } arg_list;

        struct {
            struct ASTNode* expr;
        } ret;

        struct {
            struct ASTNode* stmt_list;
        } block;

        struct {
            char* name;
            int size;
            int isParam;
        } array_decl;

        struct {
            char* name;
            struct ASTNode* index;
        } array_index;

    } data;

} ASTNode;


/* Basic nodes */

ASTNode* createNum(int value);

ASTNode* createVar(char* name);

ASTNode* createBinOp(
    char op,
    ASTNode* left,
    ASTNode* right);

ASTNode* createDecl(
    char* type,
    char* name);

ASTNode* createAssign(
    char* var,
    ASTNode* value);

ASTNode* createPrint(
    ASTNode* expr);

ASTNode* createStmtList(
    ASTNode* stmt1,
    ASTNode* stmt2);


/* Functions */

ASTNode* createFuncDef(
    char* name,
    ASTNode* params,
    ASTNode* body);

ASTNode* createParam(
    char* name);

ASTNode* createParamList(
    ASTNode* param,
    ASTNode* next);

ASTNode* createFuncCall(
    char* name,
    ASTNode* args);

ASTNode* createArgList(
    ASTNode* expr,
    ASTNode* next);

ASTNode* createReturn(
    ASTNode* expr);


/* Blocks */

ASTNode* createBlock(
    ASTNode* stmt_list);


/* Arrays */

ASTNode* createArrayDecl(
    char* name,
    int size);

ASTNode* createArrayParam(
    char* name);

ASTNode* createArrayIndex(
    char* name,
    ASTNode* index);


/* Utilities */

const char* opText(char op);

void printAST(
    ASTNode* node,
    int level);

#endif