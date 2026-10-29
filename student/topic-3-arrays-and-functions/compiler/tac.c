/* =========================================================================
 * CST-405  ·  TOPIC 3  ·  Compiling Complex Variables and Functions
 * FILE: tac.c   —   Phases 4 & 5 — Intermediate code and optimization
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tac.h"
#include "trace.h"


TACList tacList;

TACList optimizedList;

TempAllocator tempAlloc;


/* ---------------------------------------------------------
 * TEMPORARIES
 * --------------------------------------------------------- */

void initTAC() {
    tacList.head = NULL;
    tacList.tail = NULL;

    tacList.tempCount = 0;
    tacList.labelCount = 0;

    optimizedList.head = NULL;
    optimizedList.tail = NULL;

    optimizedList.tempCount = 0;
    optimizedList.labelCount = 0;

    for (int i = 0;
         i < MAX_TEMPS;
         i++) {

        tempAlloc.allocated[i] = 0;
    }

    tempAlloc.maxUsed = 0;

    tempAlloc.freeCount = 0;

    trace(
        "TAC: Temporary allocator initialized\n");
}


char* newTemp() {
    char* temp =
        malloc(16);

    sprintf(
        temp,
        "t%d",
        tacList.tempCount++);

    return temp;
}


char* allocTemp() {
    int id;

    if (tempAlloc.freeCount >
        0) {

        id =
            tempAlloc.freeList[
                --tempAlloc.freeCount];

    } else {
        id =
            tempAlloc.maxUsed++;

        if (id >= MAX_TEMPS) {
            fprintf(
                stderr,
                "ERROR: Exceeded maximum temporaries\n");

            exit(1);
        }
    }

    tempAlloc.allocated[id] =
        1;

    char* temp =
        malloc(16);

    sprintf(
        temp,
        "t%d",
        id);

    return temp;
}


void freeTemp(char* temp) {
    if (!temp ||
        temp[0] != 't')

        return;

    int id =
        atoi(temp + 1);

    if (id < 0 ||
        id >= MAX_TEMPS)

        return;

    if (!tempAlloc.allocated[id])
        return;

    tempAlloc.allocated[id] =
        0;

    if (tempAlloc.freeCount <
        MAX_TEMPS) {

        tempAlloc.freeList[
            tempAlloc.freeCount++] =
            id;
    }
}


char* newLabel() {
    char* label =
        malloc(16);

    sprintf(
        label,
        "L%d",
        tacList.labelCount++);

    return label;
}


void printTempAllocatorState() {
    trace(
        "\nTemporary allocator:\n");

    trace(
        "  Maximum temporaries used: %d\n",
        tempAlloc.maxUsed);

    trace(
        "  Available for reuse: %d\n\n",
        tempAlloc.freeCount);
}


/* ---------------------------------------------------------
 * TAC LIST
 * --------------------------------------------------------- */

TACInstr* createTAC(
    TACOp op,
    char* arg1,
    char* arg2,
    char* result) {

    TACInstr* i =
        malloc(sizeof(TACInstr));

    i->op = op;

    i->arg1 =
        arg1
        ? strdup(arg1)
        : NULL;

    i->arg2 =
        arg2
        ? strdup(arg2)
        : NULL;

    i->result =
        result
        ? strdup(result)
        : NULL;

    i->next = NULL;

    return i;
}


void appendTAC(
    TACInstr* instr) {

    if (!tacList.head) {
        tacList.head =
            instr;

        tacList.tail =
            instr;

    } else {
        tacList.tail->next =
            instr;

        tacList.tail =
            instr;
    }
}


static void appendOptimized(
    TACInstr* instr) {

    if (!optimizedList.head) {
        optimizedList.head =
            instr;

        optimizedList.tail =
            instr;

    } else {
        optimizedList.tail->next =
            instr;

        optimizedList.tail =
            instr;
    }
}


/* ---------------------------------------------------------
 * EXPRESSIONS
 * --------------------------------------------------------- */

static int generateArgs(
    ASTNode* node);


char* generateTACExpr(
    ASTNode* node) {

    if (!node)
        return NULL;

    switch(node->type) {

        case NODE_NUM: {
            char* value =
                malloc(32);

            sprintf(
                value,
                "%d",
                node->data.num);

            return value;
        }


        case NODE_VAR:
            return strdup(
                node->data.name);


        case NODE_BINOP: {
            char* left =
                generateTACExpr(
                    node->data.binop
                        .left);

            char* right =
                NULL;

            if (node->data.binop.op !=
                'u') {

                right =
                    generateTACExpr(
                        node->data.binop
                            .right);
            }

            char* temp =
                allocTemp();

            TACOp op =
                TAC_ADD;

            switch (
                node->data.binop.op) {

                case '+':
                    op = TAC_ADD;
                    break;

                case '-':
                    op = TAC_SUB;
                    break;

                case '*':
                    op = TAC_MUL;
                    break;

                case '/':
                    op = TAC_DIV;
                    break;

                case 'u':
                    op = TAC_NEG;
                    break;

                default:
                    break;
            }

            appendTAC(
                createTAC(
                    op,
                    left,
                    right,
                    temp));

            freeTemp(left);

            freeTemp(right);

            return temp;
        }


        case NODE_FUNC_CALL: {
            int count =
                generateArgs(
                    node->data.func_call
                        .args);

            char argCount[16];

            sprintf(
                argCount,
                "%d",
                count);

            char* result =
                allocTemp();

            appendTAC(
                createTAC(
                    TAC_CALL,
                    node->data.func_call
                        .name,
                    argCount,
                    result));

            return result;
        }


        case NODE_ARRAY_INDEX: {
            char* index =
                generateTACExpr(
                    node->data.array_index
                        .index);

            char* result =
                allocTemp();

            appendTAC(
                createTAC(
                    TAC_ARRAY_LOAD,
                    node->data.array_index
                        .name,
                    index,
                    result));

            freeTemp(index);

            return result;
        }


        case NODE_ARG_LIST:
            return NULL;


        default:
            return NULL;
    }
}


/* ---------------------------------------------------------
 * ARGUMENTS
 * --------------------------------------------------------- */

static int generateArgs(
    ASTNode* node) {

    if (!node)
        return 0;

    if (node->type ==
        NODE_ARG_LIST) {

        int left =
            generateArgs(
                node->data.arg_list
                    .expr);

        int right =
            generateArgs(
                node->data.arg_list
                    .next);

        return left + right;
    }

    char* value =
        generateTACExpr(node);

    appendTAC(
        createTAC(
            TAC_ARG,
            value,
            NULL,
            NULL));

    freeTemp(value);

    return 1;
}


/* ---------------------------------------------------------
 * PARAMETERS
 * --------------------------------------------------------- */

static void generateParams(
    ASTNode* node) {

    if (!node)
        return;

    if (node->type ==
        NODE_PARAM) {

        appendTAC(
            createTAC(
                TAC_PARAM,
                "int",
                NULL,
                node->data.param.name));

        return;
    }

    if (node->type ==
            NODE_ARRAY_DECL &&
        node->data.array_decl
            .isParam) {

        appendTAC(
            createTAC(
                TAC_PARAM,
                "int[]",
                NULL,
                node->data.array_decl
                    .name));

        return;
    }

    if (node->type ==
        NODE_PARAM_LIST) {

        generateParams(
            node->data.param_list
                .param);

        generateParams(
            node->data.param_list
                .next);
    }
}


/* ---------------------------------------------------------
 * STATEMENTS
 * --------------------------------------------------------- */

static void generateStmt(
    ASTNode* node);

static void generateStmtList(
    ASTNode* node) {

    if (!node)
        return;

    if (node->type ==
        NODE_STMT_LIST) {

        generateStmt(
            node->data.stmtlist
                .stmt);

        generateStmtList(
            node->data.stmtlist
                .next);

    } else {
        generateStmt(node);
    }
}


static void generateStmt(
    ASTNode* node) {

    if (!node)
        return;

    switch(node->type) {

        case NODE_DECL:
            appendTAC(
                createTAC(
                    TAC_DECL,
                    node->data.decl
                        .varType,
                    NULL,
                    node->data.decl
                        .name));

            break;


        case NODE_ARRAY_DECL:
            if (node->data.array_decl
                    .isParam) {

                appendTAC(
                    createTAC(
                        TAC_PARAM,
                        "int[]",
                        NULL,
                        node->data.array_decl
                            .name));

            } else {
                char size[32];

                sprintf(
                    size,
                    "%d",
                    node->data.array_decl
                        .size);

                appendTAC(
                    createTAC(
                        TAC_ARRAY_DECL,
                        size,
                        NULL,
                        node->data.array_decl
                            .name));
            }

            break;


        case NODE_ASSIGN: {
            char* value =
                generateTACExpr(
                    node->data.assign
                        .value);

            if (node->data.assign
                    .arrayLHS) {

                ASTNode* lhs =
                    node->data.assign
                        .arrayLHS;

                char* index =
                    generateTACExpr(
                        lhs->data.array_index
                            .index);

                appendTAC(
                    createTAC(
                        TAC_ARRAY_STORE,
                        lhs->data.array_index
                            .name,
                        index,
                        value));

                freeTemp(index);

            } else {
                appendTAC(
                    createTAC(
                        TAC_ASSIGN,
                        value,
                        NULL,
                        node->data.assign
                            .var));
            }

            freeTemp(value);

            break;
        }


        case NODE_PRINT: {
            char* value =
                generateTACExpr(
                    node->data.expr);

            appendTAC(
                createTAC(
                    TAC_PRINT,
                    value,
                    NULL,
                    NULL));

            freeTemp(value);

            break;
        }


        case NODE_RETURN: {
            if (node->data.ret.expr) {
                char* value =
                    generateTACExpr(
                        node->data.ret
                            .expr);

                appendTAC(
                    createTAC(
                        TAC_RETURN,
                        value,
                        NULL,
                        NULL));

                freeTemp(value);

            } else {
                appendTAC(
                    createTAC(
                        TAC_RETURN,
                        NULL,
                        NULL,
                        NULL));
            }

            break;
        }


        case NODE_FUNC_CALL: {
            char* value =
                generateTACExpr(node);

            freeTemp(value);

            break;
        }


        case NODE_BLOCK:
            generateStmtList(
                node->data.block
                    .stmt_list);

            break;


        case NODE_STMT_LIST:
            generateStmtList(node);

            break;


        default:
            break;
    }
}


/* ---------------------------------------------------------
 * FUNCTIONS / TOP LEVEL
 * --------------------------------------------------------- */

static void generateFunction(
    ASTNode* node) {

    appendTAC(
        createTAC(
            TAC_FUNC_BEGIN,
            NULL,
            NULL,
            node->data.func_def.name));

    generateParams(
        node->data.func_def.params);

    generateStmt(
        node->data.func_def.body);

    appendTAC(
        createTAC(
            TAC_FUNC_END,
            NULL,
            NULL,
            node->data.func_def.name));
}


static void generateTopLevel(
    ASTNode* node) {

    if (!node)
        return;

    if (node->type ==
        NODE_STMT_LIST) {

        generateTopLevel(
            node->data.stmtlist
                .stmt);

        generateTopLevel(
            node->data.stmtlist
                .next);

        return;
    }

    if (node->type ==
        NODE_FUNC_DEF) {

        generateFunction(node);

        return;
    }

    generateStmt(node);
}


void generateTAC(
    ASTNode* node) {

    generateTopLevel(node);
}


/* ---------------------------------------------------------
 * TAC PRINTING
 * --------------------------------------------------------- */

static const char* opTextTAC(
    TACOp op) {

    switch(op) {
        case TAC_ADD:
            return "+";

        case TAC_SUB:
            return "-";

        case TAC_MUL:
            return "*";

        case TAC_DIV:
            return "/";

        default:
            return "?";
    }
}


void formatTAC(
    const TACInstr* i,
    char* buf,
    size_t n) {

    if (!i) {
        snprintf(
            buf,
            n,
            "(null)");

        return;
    }

    switch(i->op) {

        case TAC_FUNC_BEGIN:
            snprintf(
                buf,
                n,
                "FUNC_BEGIN %s",
                i->result);
            break;


        case TAC_FUNC_END:
            snprintf(
                buf,
                n,
                "FUNC_END %s",
                i->result);
            break;


        case TAC_PARAM:
            snprintf(
                buf,
                n,
                "PARAM %s %s",
                i->arg1
                ? i->arg1
                : "int",
                i->result);
            break;


        case TAC_DECL:
            snprintf(
                buf,
                n,
                "DECL %s %s",
                i->arg1
                ? i->arg1
                : "int",
                i->result);
            break;


        case TAC_ADD:
        case TAC_SUB:
        case TAC_MUL:
        case TAC_DIV:
            snprintf(
                buf,
                n,
                "%s = %s %s %s",
                i->result,
                i->arg1,
                opTextTAC(i->op),
                i->arg2);
            break;


        case TAC_NEG:
            snprintf(
                buf,
                n,
                "%s = -%s",
                i->result,
                i->arg1);
            break;


        case TAC_ASSIGN:
            snprintf(
                buf,
                n,
                "%s = %s",
                i->result,
                i->arg1);
            break;


        case TAC_PRINT:
            snprintf(
                buf,
                n,
                "PRINT %s",
                i->arg1);
            break;


        case TAC_ARG:
            snprintf(
                buf,
                n,
                "ARG %s",
                i->arg1);
            break;


        case TAC_CALL:
            snprintf(
                buf,
                n,
                "%s = CALL %s, %s",
                i->result,
                i->arg1,
                i->arg2);
            break;


        case TAC_RETURN:
            if (i->arg1) {
                snprintf(
                    buf,
                    n,
                    "RETURN %s",
                    i->arg1);

            } else {
                snprintf(
                    buf,
                    n,
                    "RETURN");
            }

            break;


        case TAC_ARRAY_DECL:
            snprintf(
                buf,
                n,
                "ARRAY_DECL %s[%s]",
                i->result,
                i->arg1);
            break;


        case TAC_ARRAY_LOAD:
            snprintf(
                buf,
                n,
                "%s = %s[%s]",
                i->result,
                i->arg1,
                i->arg2);
            break;


        case TAC_ARRAY_STORE:
            snprintf(
                buf,
                n,
                "%s[%s] = %s",
                i->arg1,
                i->arg2,
                i->result);
            break;


        case TAC_LABEL:
            snprintf(
                buf,
                n,
                "%s:",
                i->result);
            break;


        case TAC_GOTO:
            snprintf(
                buf,
                n,
                "GOTO %s",
                i->result);
            break;


        case TAC_IF_FALSE:
            snprintf(
                buf,
                n,
                "IF_FALSE %s GOTO %s",
                i->arg1,
                i->result);
            break;


        case TAC_IF_TRUE:
            snprintf(
                buf,
                n,
                "IF_TRUE %s GOTO %s",
                i->arg1,
                i->result);
            break;


        default:
            snprintf(
                buf,
                n,
                "UNKNOWN");
            break;
    }
}


int countTAC(
    const TACList* list) {

    int count = 0;

    for (TACInstr* i =
             list->head;
         i;
         i = i->next) {

        count++;
    }

    return count;
}


static void printList(
    TACList* list,
    const char* title) {

    trace(
        "%s\n",
        title);

    trace(
        "─────────────────────────────────────────────\n");

    char buf[256];

    int line = 1;

    for (TACInstr* i =
             list->head;
         i;
         i = i->next) {

        formatTAC(
            i,
            buf,
            sizeof(buf));

        trace(
            "%3d: %s\n",
            line++,
            buf);
    }

    trace(
        "─────────────────────────────────────────────\n");

    trace(
        "%d instruction(s)\n\n",
        countTAC(list));
}


void printTAC() {
    printList(
        &tacList,
        "UNOPTIMIZED THREE-ADDRESS CODE");
}


void printOptimizedTAC() {
    printList(
        &optimizedList,
        "OPTIMIZED THREE-ADDRESS CODE");
}


/* ---------------------------------------------------------
 * SIMPLE OPTIMIZER
 * --------------------------------------------------------- */

static int isNumber(
    const char* s) {

    if (!s ||
        !*s)
        return 0;

    int i = 0;

    if (s[0] == '-' ||
        s[0] == '+')
        i++;

    if (!s[i])
        return 0;

    for (;
         s[i];
         i++) {

        if (!isdigit(
                (unsigned char)s[i]))

            return 0;
    }

    return 1;
}


static void clearOptimized() {
    optimizedList.head = NULL;

    optimizedList.tail = NULL;

    optimizedList.tempCount =
        tacList.tempCount;

    optimizedList.labelCount =
        tacList.labelCount;
}


static void copyInstruction(
    TACInstr* i) {

    appendOptimized(
        createTAC(
            i->op,
            i->arg1,
            i->arg2,
            i->result));
}


static OptStats stats;


void optimizeTAC() {
    memset(
        &stats,
        0,
        sizeof(stats));

    clearOptimized();

    stats.passes = 1;

    stats.instructionsBefore =
        countTAC(
            &tacList);

    for (TACInstr* i =
             tacList.head;
         i;
         i = i->next) {

        /*
         * Constant folding for simple arithmetic.
         */

        if ((i->op ==
                TAC_ADD ||
             i->op ==
                TAC_SUB ||
             i->op ==
                TAC_MUL ||
             i->op ==
                TAC_DIV) &&
            isNumber(i->arg1) &&
            isNumber(i->arg2)) {

            long a =
                atol(i->arg1);

            long b =
                atol(i->arg2);

            long result = 0;

            int valid = 1;

            switch(i->op) {
                case TAC_ADD:
                    result = a + b;
                    break;

                case TAC_SUB:
                    result = a - b;
                    break;

                case TAC_MUL:
                    result = a * b;
                    break;

                case TAC_DIV:
                    if (b == 0) {
                        valid = 0;
                    } else {
                        result = a / b;
                    }
                    break;

                default:
                    valid = 0;
            }

            if (valid) {
                char value[64];

                sprintf(
                    value,
                    "%ld",
                    result);

                appendOptimized(
                    createTAC(
                        TAC_ASSIGN,
                        value,
                        NULL,
                        i->result));

                stats.constFold++;

                continue;
            }
        }

        /*
         * x + 0 / x - 0
         */

        if ((i->op ==
                TAC_ADD ||
             i->op ==
                TAC_SUB) &&
            i->arg2 &&
            strcmp(
                i->arg2,
                "0") == 0) {

            appendOptimized(
                createTAC(
                    TAC_ASSIGN,
                    i->arg1,
                    NULL,
                    i->result));

            stats.algebraic++;

            continue;
        }

        /*
         * x * 1 / x / 1
         */

        if ((i->op ==
                TAC_MUL ||
             i->op ==
                TAC_DIV) &&
            i->arg2 &&
            strcmp(
                i->arg2,
                "1") == 0) {

            appendOptimized(
                createTAC(
                    TAC_ASSIGN,
                    i->arg1,
                    NULL,
                    i->result));

            stats.algebraic++;

            continue;
        }

        /*
         * x * 0
         */

        if (i->op ==
                TAC_MUL &&
            i->arg2 &&
            strcmp(
                i->arg2,
                "0") == 0) {

            appendOptimized(
                createTAC(
                    TAC_ASSIGN,
                    "0",
                    NULL,
                    i->result));

            stats.algebraic++;

            continue;
        }

        copyInstruction(i);
    }

    stats.instructionsAfter =
        countTAC(
            &optimizedList);

    trace(
        "Optimization finished: %d -> %d TAC instruction(s)\n",
        stats.instructionsBefore,
        stats.instructionsAfter);
}


TACList* getOptimizedTAC() {
    return &optimizedList;
}


TACList* getUnoptimizedTAC() {
    return &tacList;
}


OptStats getOptStats() {
    return stats;
}


/* ---------------------------------------------------------
 * SAVE TAC
 * --------------------------------------------------------- */

static void saveList(
    TACList* list,
    const char* filename) {

    FILE* f =
        fopen(
            filename,
            "w");

    if (!f) {
        fprintf(
            stderr,
            "Could not write TAC file %s\n",
            filename);

        return;
    }

    char buf[256];

    for (TACInstr* i =
             list->head;
         i;
         i = i->next) {

        formatTAC(
            i,
            buf,
            sizeof(buf));

        fprintf(
            f,
            "%s\n",
            buf);
    }

    fclose(f);
}


void saveTACToFile(
    const char* filename) {

    saveList(
        &tacList,
        filename);
}


void saveOptimizedTACToFile(
    const char* filename) {

    saveList(
        &optimizedList,
        filename);
}