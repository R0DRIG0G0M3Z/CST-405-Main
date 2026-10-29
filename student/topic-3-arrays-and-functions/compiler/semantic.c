/* =========================================================================
 * CST-405  ·  TOPIC 3  ·  Compiling Complex Variables and Functions
 * FILE: semantic.c   —   Phase 3 — Semantic analysis
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semantic.h"
#include "symtab.h"
#include "trace.h"

#define MAX_FUNCTIONS 100
#define MAX_PARAMS 20
#define MAX_SCOPE_DEPTH 10


typedef struct {
    char* name;

    int paramCount;

    char* params[MAX_PARAMS];

    int paramIsArray[MAX_PARAMS];

    int isDefined;

} FunctionSymbol;


typedef struct {
    char* names[MAX_VARS];

    int count;

} Scope;


static SemanticInfo semInfo;

static FunctionSymbol
    functions[MAX_FUNCTIONS];

static int functionCount = 0;

static Scope
    scopes[MAX_SCOPE_DEPTH];

static int scopeDepth = 0;

static char*
    currentFunction = NULL;

static int
    inFunction = 0;


/* ---------------------------------------------------------
 * INITIALIZATION
 * --------------------------------------------------------- */

void initSemantic() {
    semInfo.errorCount = 0;
    semInfo.warningCount = 0;

    scopeDepth = 0;

    functionCount = 0;

    currentFunction = NULL;

    inFunction = 0;

    /* Built-in print function */

    functions[functionCount].name =
        strdup("print");

    functions[functionCount].paramCount =
        1;

    functions[functionCount].isDefined =
        1;

    functionCount++;

    trace(
        "SEMANTIC ANALYZER: initialized\n\n");
}


/* ---------------------------------------------------------
 * SCOPES
 * --------------------------------------------------------- */

static void enterScope() {
    if (scopeDepth >=
        MAX_SCOPE_DEPTH) {

        fprintf(
            stderr,
            "SEMANTIC ERROR: Maximum scope depth exceeded\n");

        semInfo.errorCount++;

        return;
    }

    scopes[scopeDepth].count =
        0;

    scopeDepth++;
}


static void exitScope() {
    if (scopeDepth <= 0)
        return;

    for (int i = 0;
         i <
         scopes[scopeDepth - 1].count;
         i++) {

        free(
            scopes[scopeDepth - 1]
                .names[i]);
    }

    scopeDepth--;
}


static void printSemanticScopes() {
    trace(
        "\n┌─────────────────────────────────────────────────────────┐\n");

    trace(
        "│ SEMANTIC SCOPE STACK (Depth: %d)\n",
        scopeDepth);

    trace(
        "├─────────────────────────────────────────────────────────┤\n");

    if (scopeDepth == 0) {
        trace(
            "│ (no active scopes)\n");

    } else {
        for (int depth = 0;
             depth < scopeDepth;
             depth++) {

            if (depth == 0) {
                trace(
                    "│ Scope[%d] GLOBAL (%d variables)\n",
                    depth,
                    scopes[depth].count);

            } else if (
                inFunction &&
                depth ==
                    scopeDepth - 1) {

                trace(
                    "│ Scope[%d] Function '%s' (%d variables)\n",
                    depth,
                    currentFunction
                    ? currentFunction
                    : "unknown",
                    scopes[depth].count);

            } else {
                trace(
                    "│ Scope[%d] LOCAL (%d variables)\n",
                    depth,
                    scopes[depth].count);
            }

            if (scopes[depth].count >
                0) {

                trace(
                    "│   Variables: ");

                for (int i = 0;
                     i <
                     scopes[depth].count;
                     i++) {

                    trace(
                        "%s",
                        scopes[depth]
                            .names[i]);

                    if (i <
                        scopes[depth]
                            .count - 1) {

                        trace(", ");
                    }
                }

                trace("\n");

            } else {
                trace(
                    "│   (empty)\n");
            }
        }
    }

    trace(
        "└─────────────────────────────────────────────────────────┘\n\n");
}


/* ---------------------------------------------------------
 * VARIABLE SCOPE
 * --------------------------------------------------------- */

static int isReservedName(
    const char* name) {

    if (!name ||
        !name[0])
        return 0;

    if (strncmp(
            name,
            "__sw",
            4) == 0)

        return 1;

    if (name[0] != 't' &&
        name[0] != 'L')

        return 0;

    if (!name[1])
        return 0;

    for (int i = 1;
         name[i];
         i++) {

        if (name[i] < '0' ||
            name[i] > '9')

            return 0;
    }

    return 1;
}


static void reportReserved(
    const char* name,
    int lineno) {

    fprintf(
        stderr,
        "\nSEMANTIC ERROR - Reserved Identifier\n");

    fprintf(
        stderr,
        "Location: Line %d\n",
        lineno);

    fprintf(
        stderr,
        "'%s' is reserved for compiler use\n\n",
        name);
}


static int addVarToScope(
    char* name) {

    if (scopeDepth == 0) {
        fprintf(
            stderr,
            "SEMANTIC ERROR: No active scope\n");

        return -1;
    }

    Scope* current =
        &scopes[scopeDepth - 1];

    for (int i = 0;
         i < current->count;
         i++) {

        if (strcmp(
                current->names[i],
                name) == 0) {

            return -1;
        }
    }

    if (current->count >=
        MAX_VARS) {

        fprintf(
            stderr,
            "SEMANTIC ERROR: Too many variables in scope\n");

        return -1;
    }

    current->names[
        current->count++] =
        strdup(name);

    return 0;
}


static int addArrayToScope(
    char* name,
    int size) {

    (void)size;

    return addVarToScope(
        name);
}


static int isVarDeclaredInScope(
    char* name) {

    for (int depth =
             scopeDepth - 1;
         depth >= 0;
         depth--) {

        for (int i = 0;
             i <
             scopes[depth].count;
             i++) {

            if (strcmp(
                    scopes[depth]
                        .names[i],
                    name) == 0) {

                return 1;
            }
        }
    }

    return 0;
}


/* ---------------------------------------------------------
 * FUNCTIONS
 * --------------------------------------------------------- */

static FunctionSymbol*
findFunction(char* name) {

    for (int i = 0;
         i < functionCount;
         i++) {

        if (strcmp(
                functions[i].name,
                name) == 0) {

            return &functions[i];
        }
    }

    return NULL;
}


static int addFunction(
    char* name,
    int paramCount,
    char** params,
    int* paramIsArray) {

    if (findFunction(name)) {
        fprintf(
            stderr,
            "SEMANTIC ERROR: Function '%s' already defined\n",
            name);

        semInfo.errorCount++;

        return -1;
    }

    if (functionCount >=
        MAX_FUNCTIONS) {

        fprintf(
            stderr,
            "SEMANTIC ERROR: Too many functions\n");

        semInfo.errorCount++;

        return -1;
    }

    FunctionSymbol* f =
        &functions[functionCount++];

    f->name =
        strdup(name);

    f->paramCount =
        paramCount;

    f->isDefined =
        1;

    for (int i = 0;
         i < paramCount;
         i++) {

        f->params[i] =
            strdup(params[i]);

        f->paramIsArray[i] =
            paramIsArray
            ? paramIsArray[i]
            : 0;
    }

    trace(
        "  ✓ Function '%s' defined with %d parameter(s)\n",
        name,
        paramCount);

    return 0;
}


static int countParams(
    ASTNode* params,
    char** paramNames,
    int* paramIsArray) {

    if (!params)
        return 0;

    if (params->type ==
        NODE_PARAM) {

        paramNames[0] =
            params->data.param.name;

        paramIsArray[0] =
            0;

        return 1;
    }

    if (params->type ==
            NODE_ARRAY_DECL &&
        params->data.array_decl
            .isParam) {

        paramNames[0] =
            params->data.array_decl
                .name;

        paramIsArray[0] =
            1;

        return 1;
    }

    if (params->type ==
        NODE_PARAM_LIST) {

        int left =
            countParams(
                params->data.param_list
                    .param,
                paramNames,
                paramIsArray);

        int right =
            countParams(
                params->data.param_list
                    .next,
                &paramNames[left],
                &paramIsArray[left]);

        return left + right;
    }

    return 0;
}


static int countArgs(
    ASTNode* args) {

    if (!args)
        return 0;

    if (args->type ==
        NODE_ARG_LIST) {

        return
            countArgs(
                args->data.arg_list
                    .expr)
            +
            countArgs(
                args->data.arg_list
                    .next);
    }

    return 1;
}


/* ---------------------------------------------------------
 * EXPRESSIONS
 * --------------------------------------------------------- */

static void checkExpr(
    ASTNode* node);

static void checkStmt(
    ASTNode* node);

static void checkStmtList(
    ASTNode* node);


static void checkExpr(
    ASTNode* node) {

    if (!node)
        return;

    switch(node->type) {

        case NODE_NUM:
            break;


        case NODE_VAR:
            if (!isVarDeclaredInScope(
                    node->data.name)) {

                fprintf(
                    stderr,
                    "SEMANTIC ERROR line %d: variable '%s' is not declared\n",
                    node->lineno,
                    node->data.name);

                semInfo.errorCount++;
            }

            break;


        case NODE_BINOP:
            checkExpr(
                node->data.binop.left);

            checkExpr(
                node->data.binop.right);

            break;


        case NODE_FUNC_CALL: {
            FunctionSymbol* f =
                findFunction(
                    node->data.func_call
                        .name);

            if (!f) {
                fprintf(
                    stderr,
                    "SEMANTIC ERROR line %d: function '%s' is not declared\n",
                    node->lineno,
                    node->data.func_call
                        .name);

                semInfo.errorCount++;

            } else {
                int count =
                    countArgs(
                        node->data.func_call
                            .args);

                if (count !=
                    f->paramCount) {

                    fprintf(
                        stderr,
                        "SEMANTIC ERROR line %d: function '%s' expects %d argument(s), got %d\n",
                        node->lineno,
                        node->data.func_call
                            .name,
                        f->paramCount,
                        count);

                    semInfo.errorCount++;

                } else {
                    trace(
                        "  ✓ Function call '%s' has correct argument count\n",
                        node->data.func_call
                            .name);
                }
            }

            ASTNode* args =
                node->data.func_call
                    .args;

            if (args)
                checkExpr(args);

            break;
        }


        case NODE_ARG_LIST:
            checkExpr(
                node->data.arg_list.expr);

            checkExpr(
                node->data.arg_list.next);

            break;


        case NODE_ARRAY_INDEX:
            if (!isVarDeclaredInScope(
                    node->data.array_index
                        .name)) {

                fprintf(
                    stderr,
                    "SEMANTIC ERROR line %d: array '%s' is not declared\n",
                    node->lineno,
                    node->data.array_index
                        .name);

                semInfo.errorCount++;
            }

            checkExpr(
                node->data.array_index
                    .index);

            break;


        default:
            break;
    }
}


/* ---------------------------------------------------------
 * STATEMENTS
 * --------------------------------------------------------- */

static void checkStmt(
    ASTNode* node) {

    if (!node)
        return;

    switch(node->type) {

        case NODE_DECL:
            if (isReservedName(
                    node->data.decl.name)) {

                reportReserved(
                    node->data.decl.name,
                    node->lineno);

                semInfo.errorCount++;

                break;
            }

            if (addVarToScope(
                    node->data.decl.name) ==
                -1) {

                fprintf(
                    stderr,
                    "SEMANTIC ERROR line %d: variable '%s' already declared in this scope\n",
                    node->lineno,
                    node->data.decl.name);

                semInfo.errorCount++;

            } else {
                trace(
                    "  ✓ Variable '%s' declared (line %d)\n",
                    node->data.decl.name,
                    node->lineno);
            }

            break;


        case NODE_ARRAY_DECL:
            if (isReservedName(
                    node->data.array_decl
                        .name)) {

                reportReserved(
                    node->data.array_decl
                        .name,
                    node->lineno);

                semInfo.errorCount++;

                break;
            }

            if (!node->data.array_decl
                    .isParam &&
                node->data.array_decl
                    .size <= 0) {

                fprintf(
                    stderr,
                    "SEMANTIC ERROR line %d: array '%s' must have positive size\n",
                    node->lineno,
                    node->data.array_decl
                        .name);

                semInfo.errorCount++;

                break;
            }

            if (addArrayToScope(
                    node->data.array_decl
                        .name,
                    node->data.array_decl
                        .size) == -1) {

                fprintf(
                    stderr,
                    "SEMANTIC ERROR line %d: array '%s' already declared\n",
                    node->lineno,
                    node->data.array_decl
                        .name);

                semInfo.errorCount++;

            } else {
                trace(
                    "  ✓ Array '%s' declared\n",
                    node->data.array_decl
                        .name);
            }

            break;


        case NODE_ASSIGN:
            if (node->data.assign
                    .arrayLHS) {

                checkExpr(
                    node->data.assign
                        .arrayLHS);

            } else if (
                !isVarDeclaredInScope(
                    node->data.assign.var)) {

                fprintf(
                    stderr,
                    "SEMANTIC ERROR line %d: assignment to undeclared variable '%s'\n",
                    node->lineno,
                    node->data.assign.var);

                semInfo.errorCount++;
            }

            checkExpr(
                node->data.assign.value);

            break;


        case NODE_PRINT:
            checkExpr(
                node->data.expr);

            trace(
                "  ✓ Print statement is valid\n");

            break;


        case NODE_RETURN:
            if (!inFunction) {
                fprintf(
                    stderr,
                    "SEMANTIC ERROR line %d: return outside a function\n",
                    node->lineno);

                semInfo.errorCount++;

            } else {
                trace(
                    "  ✓ Return statement in function '%s'\n",
                    currentFunction);

                checkExpr(
                    node->data.ret.expr);
            }

            break;


        case NODE_FUNC_CALL:
            checkExpr(node);

            break;


        case NODE_BLOCK:
            enterScope();

            checkStmtList(
                node->data.block
                    .stmt_list);

            exitScope();

            break;


        case NODE_STMT_LIST:
            checkStmtList(node);

            break;


        default:
            break;
    }
}


static void checkStmtList(
    ASTNode* node) {

    if (!node)
        return;

    if (node->type ==
        NODE_STMT_LIST) {

        checkStmt(
            node->data.stmtlist
                .stmt);

        checkStmtList(
            node->data.stmtlist
                .next);

    } else {
        checkStmt(node);
    }
}


/* ---------------------------------------------------------
 * FUNCTIONS / TWO PASSES
 * --------------------------------------------------------- */

static void addParamsToScope(
    ASTNode* node) {

    if (!node)
        return;

    if (node->type ==
        NODE_PARAM) {

        if (addVarToScope(
                node->data.param.name) ==
            -1) {

            fprintf(
                stderr,
                "SEMANTIC ERROR: duplicate parameter '%s'\n",
                node->data.param.name);

            semInfo.errorCount++;

        } else {
            trace(
                "  ✓ Parameter '%s' added to function scope\n",
                node->data.param.name);
        }

        return;
    }

    if (node->type ==
            NODE_ARRAY_DECL &&
        node->data.array_decl
            .isParam) {

        if (addArrayToScope(
                node->data.array_decl
                    .name,
                0) == -1) {

            fprintf(
                stderr,
                "SEMANTIC ERROR: duplicate array parameter '%s'\n",
                node->data.array_decl
                    .name);

            semInfo.errorCount++;

        } else {
            trace(
                "  ✓ Array parameter '%s[]' added to function scope\n",
                node->data.array_decl
                    .name);
        }

        return;
    }

    if (node->type ==
        NODE_PARAM_LIST) {

        addParamsToScope(
            node->data.param_list
                .param);

        addParamsToScope(
            node->data.param_list
                .next);
    }
}


static void checkFuncDef(
    ASTNode* node) {

    if (!node ||
        node->type !=
            NODE_FUNC_DEF)

        return;

    currentFunction =
        node->data.func_def.name;

    inFunction =
        1;

    enterScope();

    trace(
        "  Entered function '%s' scope\n",
        currentFunction);

    printSemanticScopes();

    addParamsToScope(
        node->data.func_def.params);

    printSemanticScopes();

    /*
     * Function body is a BLOCK.
     * The block creates another nested scope
     * for local declarations.
     */

    checkStmt(
        node->data.func_def.body);

    trace(
        "  Exiting function '%s' scope\n",
        currentFunction);

    exitScope();

    printSemanticScopes();

    inFunction =
        0;

    currentFunction =
        NULL;
}


static void registerFunction(
    ASTNode* node) {

    if (!node ||
        node->type !=
            NODE_FUNC_DEF)

        return;

    char* paramNames[MAX_PARAMS];

    int paramIsArray[MAX_PARAMS];

    int count =
        countParams(
            node->data.func_def.params,
            paramNames,
            paramIsArray);

    addFunction(
        node->data.func_def.name,
        count,
        paramNames,
        paramIsArray);
}


static void registerFunctions(
    ASTNode* node) {

    if (!node)
        return;

    if (node->type ==
        NODE_STMT_LIST) {

        registerFunctions(
            node->data.stmtlist
                .stmt);

        registerFunctions(
            node->data.stmtlist
                .next);

    } else if (
        node->type ==
        NODE_FUNC_DEF) {

        registerFunction(node);
    }
}


static void checkFunctions(
    ASTNode* node) {

    if (!node)
        return;

    if (node->type ==
        NODE_STMT_LIST) {

        checkFunctions(
            node->data.stmtlist
                .stmt);

        checkFunctions(
            node->data.stmtlist
                .next);

        return;
    }

    if (node->type ==
        NODE_FUNC_DEF) {

        trace(
            "─── Checking function: %s ───\n",
            node->data.func_def
                .name);

        checkFuncDef(node);

        trace("\n");

        return;
    }

    if (node->type ==
        NODE_DECL) {

        if (addVarToScope(
                node->data.decl.name) ==
            -1) {

            fprintf(
                stderr,
                "SEMANTIC ERROR: global variable '%s' already declared\n",
                node->data.decl.name);

            semInfo.errorCount++;

        } else {
            trace(
                "  ✓ Global variable '%s' declared\n",
                node->data.decl.name);
        }

        return;
    }

    if (node->type ==
        NODE_ARRAY_DECL) {

        checkStmt(node);

        return;
    }

    checkStmt(node);
}


int performSemanticAnalysis(
    ASTNode* root) {

    if (!root) {
        fprintf(
            stderr,
            "SEMANTIC ERROR: No AST to analyze\n");

        return -1;
    }

    trace(
        "Running semantic analysis with function support...\n\n");

    enterScope();

    trace(
        "Entered global scope\n");

    printSemanticScopes();


    trace(
        "Pass 1: Registering all functions\n");

    trace(
        "───────────────────────────────\n");

    registerFunctions(root);

    trace("\n");


    trace(
        "Pass 2: Checking function bodies\n");

    trace(
        "─────────────────────────────────\n");

    checkFunctions(root);


    exitScope();

    return
        semInfo.errorCount > 0
        ? -1
        : 0;
}


void printSemanticSummary() {
    trace(
        "═══════════════════════════════════════════\n");

    trace(
        "SEMANTIC ANALYSIS SUMMARY\n");

    trace(
        "═══════════════════════════════════════════\n");

    trace(
        "Functions defined:  %d\n",
        functionCount);

    trace(
        "Errors found:       %d\n",
        semInfo.errorCount);

    trace(
        "Warnings found:     %d\n",
        semInfo.warningCount);

    trace("\n");

    if (semInfo.errorCount == 0) {
        trace(
            "✓ Semantic analysis passed - program is semantically correct!\n");

    } else {
        trace(
            "✗ Semantic analysis failed - fix errors before proceeding\n");
    }

    trace(
        "═══════════════════════════════════════════\n\n");
}