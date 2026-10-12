/* =========================================================================
 * CST-405  ·  TOPIC 2  ·  Compiler for a Starter Language
 * FILE: tac.c   —   Phases 4 & 5 — Intermediate code and optimization
 * -------------------------------------------------------------------------
 * THE PIPELINE, AND WHERE THIS FILE SITS IN IT
 *   scanner -> parser -> ast -> semantic -> tac -> codegen
 *                                           ^^^  this file
 *
 * WHAT IS NEW IN TOPIC 2
 *   • AST -> three-address code, constant folding and propagation
 *
 * WHAT COMES NEXT
 *   Topic 3 adds functions, arrays and the rest of arithmetic — and with them, real activation records.
 *
 * PROJECT 2 IMPLEMENTATION
 *   Interfaces and register/storage foundations are adapted from the course
 *   starter by Isac Artzi. Completed implementation is described in README.
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

void initTAC() {
    tacList.head = NULL;
    tacList.tail = NULL;
    tacList.tempCount = 0;
    tacList.labelCount = 0;
    optimizedList.head = NULL;
    optimizedList.tail = NULL;

    /* Initialize temporary allocator */
    for (int i = 0; i < MAX_TEMPS; i++) {
        tempAlloc.allocated[i] = 0;
    }
    tempAlloc.maxUsed = 0;
    tempAlloc.freeCount = 0;
    trace("TAC: Temporary allocator initialized\n");
}

char* newTemp() {
    char* temp = malloc(10);
    sprintf(temp, "t%d", tacList.tempCount++);
    return temp;
}

char* allocTemp() {
    int tempNum;

    if (tempAlloc.freeCount > 0) {
        tempNum = tempAlloc.freeList[--tempAlloc.freeCount];
        trace("    [TAC ALLOC] Reusing temporary t%d\n", tempNum);
    } else {
        tempNum = tempAlloc.maxUsed++;
        tacList.tempCount = tempAlloc.maxUsed;
        if (tempNum >= MAX_TEMPS) {
            fprintf(stderr, "ERROR: Exceeded maximum temporaries\n");
            exit(1);
        }
        trace("    [TAC ALLOC] Allocating new temporary t%d\n", tempNum);
    }

    tempAlloc.allocated[tempNum] = 1;
    char* temp = malloc(10);
    sprintf(temp, "t%d", tempNum);
    return temp;
}

void freeTemp(char* temp) {
    if (!temp || temp[0] != 't' || !isdigit((unsigned char)temp[1])) return;
    for (int i=1;temp[i];i++) if (!isdigit((unsigned char)temp[i])) return;

    int tempNum = atoi(temp + 1);
    if (tempNum<0 || tempNum>=MAX_TEMPS || !tempAlloc.allocated[tempNum]) return;

    tempAlloc.allocated[tempNum] = 0;
    if (tempAlloc.freeCount < MAX_TEMPS) {
        tempAlloc.freeList[tempAlloc.freeCount++] = tempNum;
        trace("    [TAC FREE] Released temporary %s\n", temp);
    }
}


void printTempAllocatorState() {
    trace("\n┌──────────────────────────────────────────────────────────┐\n");
    trace("│ TEMPORARY ALLOCATOR STATISTICS                           │\n");
    trace("├──────────────────────────────────────────────────────────┤\n");
    trace("│ Total temporaries used:      %3d                         │\n", tempAlloc.maxUsed);
    trace("│ Currently allocated:         %3d                         │\n",
           tempAlloc.maxUsed - tempAlloc.freeCount);
    trace("│ Available for reuse:         %3d                         │\n", tempAlloc.freeCount);
    trace("└──────────────────────────────────────────────────────────┘\n\n");
}

TACInstr* createTAC(TACOp op, char* arg1, char* arg2, char* result) {
    TACInstr* instr = malloc(sizeof(TACInstr));
    instr->op = op;
    instr->arg1 = arg1 ? strdup(arg1) : NULL;
    instr->arg2 = arg2 ? strdup(arg2) : NULL;
    instr->result = result ? strdup(result) : NULL;
    instr->next = NULL;
    return instr;
}

void appendTAC(TACInstr* instr) {
    if (!tacList.head) {
        tacList.head = tacList.tail = instr;
    } else {
        tacList.tail->next = instr;
        tacList.tail = instr;
    }
}

void appendOptimizedTAC(TACInstr* instr) {
    if (!optimizedList.head) {
        optimizedList.head = optimizedList.tail = instr;
    } else {
        optimizedList.tail->next = instr;
        optimizedList.tail = instr;
    }
}

/* ── BREAK LABEL STACK ──────────────────────────────────────────────────────
 * When entering a switch, for, or while, push the "end" label so that any
 * nested NODE_BREAK can emit GOTO to the right exit point.
 */

/* Forward declarations */
static void generateTACStmt(ASTNode* node);


/* Generate TAC for expression - returns the temp/var holding result */
char* generateTACExpr(ASTNode* node) {
    if (!node) return NULL;
    if (node->type==NODE_NUM) {
        char value[32]; snprintf(value,sizeof value,"%d",node->data.num);
        return strdup(value);
    }
    if (node->type==NODE_VAR) return strdup(node->data.name);
    if (node->type==NODE_BINOP) {
        /* Postorder traversal: children must be computed before their sum.
         * The TAC instruction owns copies, so local strings can be freed. */
        char* left=generateTACExpr(node->data.binop.left);
        char* right=generateTACExpr(node->data.binop.right);
        char* result=allocTemp();
        appendTAC(createTAC(TAC_ADD,left,right,result));
        freeTemp(left); freeTemp(right); free(left); free(right);
        return result;
    }
    fprintf(stderr,"Internal TAC error: expression expected\n"); exit(1);
}

/* Generate TAC for statement list */
static void generateTACStmtList(ASTNode* node) {
    if (!node) return;

    if (node->type == NODE_STMT_LIST) {
        generateTACStmt(node->data.stmtlist.stmt);
        generateTACStmtList(node->data.stmtlist.next);
    } else {
        generateTACStmt(node);
    }
}

/* Generate TAC for a statement */
static void generateTACStmt(ASTNode* node) {
    if (!node) return;
    switch (node->type) {
    case NODE_DECL:
        appendTAC(createTAC(TAC_DECL,node->data.decl.varType,NULL,node->data.decl.name));
        break;
    case NODE_ASSIGN: {
        char* value=generateTACExpr(node->data.assign.value);
        appendTAC(createTAC(TAC_ASSIGN,value,NULL,node->data.assign.var));
        freeTemp(value); free(value); break;
    }
    case NODE_PRINT: {
        char* value=generateTACExpr(node->data.expr);
        appendTAC(createTAC(TAC_PRINT,value,NULL,NULL));
        freeTemp(value); free(value); break;
    }
    case NODE_STMT_LIST: generateTACStmtList(node); break;
    default: break;
    }
}

void generateTAC(ASTNode* node) {
    if (!node) return;
    /* The starter language has no function syntax, but the code generator
     * still emits code into MIPS functions — every program needs an entry
     * point called `main`.  So the whole statement list becomes the body of
     * an implicit main().  Topic 3 makes that function explicit in the
     * source, and this wrapper goes away. */
    appendTAC(createTAC(TAC_FUNC_BEGIN, NULL, NULL, "main"));
    generateTACStmt(node);
    appendTAC(createTAC(TAC_RETURN, "0", NULL, NULL));
    appendTAC(createTAC(TAC_FUNC_END, NULL, NULL, "main"));
}

/* Get operation name for printing */
static const char* getOpName(TACOp op) {
    switch(op) {
        case TAC_ADD: return "+";
        case TAC_SUB: return "-";
        case TAC_MUL: return "*";
        case TAC_DIV: return "/";
        case TAC_NEG: return "NEG";
        case TAC_LT: return "<";
        case TAC_GT: return ">";
        case TAC_LE: return "<=";
        case TAC_GE: return ">=";
        case TAC_EQ: return "==";
        case TAC_NE: return "!=";
        case TAC_AND: return "&&";
        case TAC_OR: return "||";
        case TAC_NOT: return "!";
        default: return "?";
    }
}


/* ==========================================================================
 * FORMATTING
 * --------------------------------------------------------------------------
 * One function renders a TAC instruction as text.  Everything that displays
 * or saves three-address code goes through it, so the listing you read on
 * screen and the listing saved to the .tac file can never drift apart.
 * ========================================================================*/
void formatTAC(const TACInstr* i, char* buf, size_t n) {
    if (!i) { snprintf(buf, n, "(null)"); return; }
    switch (i->op) {
        case TAC_FUNC_BEGIN: snprintf(buf, n, "FUNC_BEGIN %s", i->result); break;
        case TAC_FUNC_END:   snprintf(buf, n, "FUNC_END %s", i->result); break;
        case TAC_PARAM:      snprintf(buf, n, "PARAM %s %s",
                                      i->arg1 ? i->arg1 : "int", i->result); break;
        case TAC_DECL:       snprintf(buf, n, "DECL %s %s",
                                      i->arg1 ? i->arg1 : "int", i->result); break;

        case TAC_ADD: case TAC_SUB: case TAC_MUL: case TAC_DIV:
        case TAC_LT:  case TAC_GT:  case TAC_LE:  case TAC_GE:
        case TAC_EQ:  case TAC_NE:  case TAC_AND: case TAC_OR:
            snprintf(buf, n, "%s = %s %s %s", i->result, i->arg1,
                     getOpName(i->op), i->arg2); break;

        case TAC_NEG:        snprintf(buf, n, "%s = -%s", i->result, i->arg1); break;
        case TAC_NOT:        snprintf(buf, n, "%s = !%s", i->result, i->arg1); break;
        case TAC_ASSIGN:     snprintf(buf, n, "%s = %s", i->result, i->arg1); break;
        case TAC_PRINT:      snprintf(buf, n, "PRINT %s", i->arg1); break;
        case TAC_ARG:        snprintf(buf, n, "ARG %s", i->arg1); break;
        case TAC_CALL:       snprintf(buf, n, "%s = CALL %s, %s",
                                      i->result, i->arg1, i->arg2); break;
        case TAC_RETURN:     if (i->arg1) snprintf(buf, n, "RETURN %s", i->arg1);
                             else         snprintf(buf, n, "RETURN");
                             break;
        case TAC_LABEL:      snprintf(buf, n, "%s:", i->result); break;
        case TAC_GOTO:       snprintf(buf, n, "GOTO %s", i->arg1); break;
        case TAC_IF_FALSE:   snprintf(buf, n, "IF_FALSE %s GOTO %s", i->arg1, i->arg2); break;
        case TAC_IF_TRUE:    snprintf(buf, n, "IF_TRUE %s GOTO %s", i->arg1, i->arg2); break;
        case TAC_ARRAY_DECL: snprintf(buf, n, "ARRAY_DECL %s[%s]", i->result, i->arg1); break;
        case TAC_ARRAY_LOAD: snprintf(buf, n, "%s = %s[%s]", i->result, i->arg1, i->arg2); break;
        case TAC_ARRAY_STORE:snprintf(buf, n, "%s[%s] = %s", i->arg1, i->arg2, i->result); break;
        default:             snprintf(buf, n, "UNKNOWN"); break;
    }
}

/* Count instructions in a list — the crudest possible code-size metric, and
 * the one Topic 4 uses to quantify what optimization bought us. */
int countTAC(const TACList* list) {
    int n = 0;
    for (TACInstr* c = list->head; c; c = c->next) n++;
    return n;
}

static void dumpList(const TACList* list, const char* title) {
    char buf[256];
    trace("%s\n", title);
    trace("─────────────────────────────────────────────\n");
    int line = 1;
    for (TACInstr* c = list->head; c; c = c->next) {
        formatTAC(c, buf, sizeof buf);
        if (c->op == TAC_LABEL || c->op == TAC_FUNC_BEGIN || c->op == TAC_FUNC_END)
            trace("%3d: %s\n", line++, buf);
        else
            trace("%3d:     %s\n", line++, buf);
    }
    trace("─────────────────────────────────────────────\n");
    trace("     %d instructions\n\n", countTAC(list));
}

void printTAC(void)          { dumpList(&tacList,       "UNOPTIMIZED THREE-ADDRESS CODE"); }
void printOptimizedTAC(void) { dumpList(&optimizedList, "OPTIMIZED THREE-ADDRESS CODE"); }

TACList* getOptimizedTAC(void) { return &optimizedList; }
TACList* getUnoptimizedTAC(void) { return &tacList; }

static void saveList(const TACList* list, const char* filename, const char* banner) {
    FILE* f = fopen(filename, "w");
    if (!f) { fprintf(stderr, "Cannot write %s\n", filename); exit(1); }
    char buf[256];
    fprintf(f, "; %s\n", banner);
    fprintf(f, "; Generated by the CST-405 mini compiler\n;\n");
    int line = 1;
    for (TACInstr* c = list->head; c; c = c->next) {
        formatTAC(c, buf, sizeof buf);
        if (c->op == TAC_LABEL || c->op == TAC_FUNC_BEGIN || c->op == TAC_FUNC_END)
            fprintf(f, "%3d: %s\n", line++, buf);
        else
            fprintf(f, "%3d:     %s\n", line++, buf);
    }
    fprintf(f, ";\n; %d instructions\n", countTAC(list));
    int failed=ferror(f);
    if (fclose(f)!=0) failed=1;
    if (failed) { fprintf(stderr,"Failed to save TAC file %s\n",filename); exit(1); }
}

void saveTACToFile(const char* filename)          { saveList(&tacList,       filename, "Unoptimized three-address code"); }
void saveOptimizedTACToFile(const char* filename) { saveList(&optimizedList, filename, "Optimized three-address code"); }

/* ==========================================================================
 * PHASE 5 — OPTIMIZATION
 * --------------------------------------------------------------------------
 * The optimizer rewrites the TAC list into a shorter, cheaper list that
 * computes the same thing.  It runs the same set of transformations REPEATEDLY
 * until a pass changes nothing, because optimizations feed each other:
 *
 *      t0 = 2 * 3        constant folding  ->  t0 = 6
 *      x  = t0                                  x  = 6      (copy propagation)
 *      y  = x + 0                               y  = 6      (algebraic + folding)
 *      t0 = ...                                 (t0 now dead -> removed)
 *
 * No single pass finds all of that; three passes do.  Reaching a fixed point
 * is the standard way real optimizers are structured.
 *
 * Topic 2 implements constant propagation/folding, addition by zero, and
 * removal of unused compiler temporaries. There are no branches or loops,
 * so control-flow transformations have no opportunities in this grammar.
 * Facts are cleared at labels/functions for conservative future extension.
 * ========================================================================*/

#define MAX_FACTS 256

typedef struct {
    char* name;      /* The variable or temporary the fact is about */
    char* value;     /* The constant, or the name it is a copy of   */
    int   isConst;   /* 1 = value is a literal, 0 = value is a name */
} Fact;

static Fact  facts[MAX_FACTS];
static int   factCount = 0;
static int   changesThisPass = 0;

/* Optimization bookkeeping, reported to the user and used by Topic 4 to
 * quantify the gain from each technique. */
static OptStats optStats;

OptStats getOptStats(void) { return optStats; }

static void clearFacts(void) { factCount = 0; }

static void dropFactsAbout(const char* name) {
    for (int i = 0; i < factCount; i++) {
        if (strcmp(facts[i].name, name) == 0 ||
            (!facts[i].isConst && strcmp(facts[i].value, name) == 0)) {
            facts[i] = facts[--factCount];
            i--;
        }
    }
}

/* A call can change any global or array element, so only facts about
 * compiler temporaries survive it. */
static void dropNonTempFacts(void) {
    for (int i = 0; i < factCount; i++) {
        const char* n = facts[i].name;
        int temp = (n[0] == 't' && n[1] >= '0' && n[1] <= '9');
        if (!temp) { facts[i] = facts[--factCount]; i--; }
    }
}

static void recordFact(const char* name, const char* value, int isConst) {
    dropFactsAbout(name);
    if (factCount >= MAX_FACTS) return;
    facts[factCount].name    = (char*)name;
    facts[factCount].value   = (char*)value;
    facts[factCount].isConst = isConst;
    factCount++;
}

static const char* lookupFact(const char* name, int wantConst) {
    if (!name) return NULL;
    for (int i = 0; i < factCount; i++)
        if (strcmp(facts[i].name, name) == 0 && (!wantConst || facts[i].isConst))
            return facts[i].value;
    return NULL;
}

/* True for opcodes that only compute a value into `result` and have no other
 * effect.  Only these are candidates for dead-code elimination: a CALL may
 * print or modify globals, a STORE writes memory, a branch changes control. */
static int mnemonicIsPure(TACOp op) {
    switch (op) {
        case TAC_ADD: case TAC_SUB: case TAC_MUL: case TAC_DIV: case TAC_NEG:
        case TAC_LT:  case TAC_GT:  case TAC_LE:  case TAC_GE:
        case TAC_EQ:  case TAC_NE:  case TAC_AND: case TAC_OR: case TAC_NOT:
        case TAC_ARRAY_LOAD:
            return 1;
        default:
            return 0;
    }
}

/* Is this operand a literal integer? */
static int isConstantNumber(const char* s) {
    if (!s || !*s) return 0;
    int i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (!s[i]) return 0;
    for (; s[i]; i++) if (s[i] < '0' || s[i] > '9') return 0;
    return 1;
}

/* Evaluate an operation whose operands are both literals. */
static char* foldConstants(TACOp op, const char* a1, const char* a2, int* folded) {
    *folded = 0;
    if (!isConstantNumber(a1) || !isConstantNumber(a2)) return NULL;
    long a = atol(a1), b = atol(a2), r;
    switch (op) {
        case TAC_ADD:
            /* The language uses wrapping signed 32-bit integers, matching
             * MIPS addu; folding must preserve overflow behavior. */
            r = a + b;
            if (r > 2147483647L) r -= 4294967296L;
            if (r < -2147483648L) r += 4294967296L;
            break;
        case TAC_SUB: r = a - b; break;
        case TAC_MUL: r = a * b; break;
        case TAC_DIV: if (b == 0) return NULL;   /* leave division by zero
                                                  * alone: folding it would
                                                  * change a run-time trap
                                                  * into a compile-time one */
                      r = a / b; break;
        case TAC_LT:  r = a <  b; break;
        case TAC_GT:  r = a >  b; break;
        case TAC_LE:  r = a <= b; break;
        case TAC_GE:  r = a >= b; break;
        case TAC_EQ:  r = a == b; break;
        case TAC_NE:  r = a != b; break;
        case TAC_AND: r = (a != 0) && (b != 0); break;
        case TAC_OR:  r = (a != 0) || (b != 0); break;
        default: return NULL;
    }
    char* s = malloc(24);
    snprintf(s, 24, "%ld", r);
    *folded = 1;
    return s;
}

/* Does `name` get read anywhere at or after instruction `from`?
 * A conservative liveness test: it stops being conservative only for
 * compiler temporaries, which is exactly where dead code piles up. */
static int isReadLater(TACInstr* from, const char* name) {
    for (TACInstr* c = from; c; c = c->next) {
        if (c->arg1 && strcmp(c->arg1, name) == 0) return 1;
        if (c->arg2 && strcmp(c->arg2, name) == 0) return 1;
        /* ARRAY_STORE reads its `result` field (the value being stored) */
        if (c->op == TAC_ARRAY_STORE && c->result && strcmp(c->result, name) == 0) return 1;
        if (c->op == TAC_RETURN && c->arg1 && strcmp(c->arg1, name) == 0) return 1;
    }
    return 0;
}

/* Copy a TAC list so a pass can rewrite it without destroying the original. */
static TACList copyList(const TACList* src) {
    TACList d = { NULL, NULL, src->tempCount, src->labelCount };
    for (TACInstr* c = src->head; c; c = c->next) {
        TACInstr* n = createTAC(c->op, c->arg1, c->arg2, c->result);
        if (!d.head) d.head = d.tail = n;
        else { d.tail->next = n; d.tail = n; }
    }
    return d;
}

/* -------------------------------------------------------------------------
 * One optimization pass over `in`, producing `out`.
 * Returns the number of changes made.
 * -----------------------------------------------------------------------*/
static void freeList(TACList* list);

static TACList optimizePass(TACList* in) {
    TACList out = { NULL, NULL, in->tempCount, in->labelCount };
    clearFacts();
    for (TACInstr* c=in->head; c; c=c->next) {
        TACInstr* n=createTAC(c->op,c->arg1,c->arg2,c->result);
        /* Topic 2 has a single straight-line block. Clear facts at control
         * boundaries defensively; later topics need full flow analysis. */
        if(n->op==TAC_LABEL || n->op==TAC_FUNC_BEGIN || n->op==TAC_FUNC_END)
            clearFacts();
        if(n->op==TAC_CALL) dropNonTempFacts();
        if(n->op==TAC_ADD || n->op==TAC_ASSIGN || n->op==TAC_PRINT || n->op==TAC_RETURN) {
            char** args[2]={&n->arg1,&n->arg2};
            for(int k=0;k<2;k++) {
                const char* value=lookupFact(*args[k],1);
                if(value) {
                    char* copy=strdup(value); free(*args[k]); *args[k]=copy;
                    changesThisPass++; optStats.constProp++;
                }
            }
        }
        if(n->op==TAC_ADD) {
            int folded=0;
            char* value=foldConstants(n->op,n->arg1,n->arg2,&folded);
            if(folded) {
                free(n->arg1); free(n->arg2); n->arg1=value; n->arg2=NULL;
                n->op=TAC_ASSIGN; changesThisPass++; optStats.constFold++;
            } else if(strcmp(n->arg1,"0")==0 || strcmp(n->arg2,"0")==0) {
                /* Local peephole: x+0 and 0+x require only a move. */
                char* keep=strdup(strcmp(n->arg1,"0")==0 ? n->arg2:n->arg1);
                free(n->arg1); free(n->arg2); n->arg1=keep; n->arg2=NULL;
                n->op=TAC_ASSIGN; changesThisPass++; optStats.algebraic++;
            }
        }
        /* RHS substitution happens BEFORE invalidating the destination.
         * Thus x=x+1 reads the old x and records the newly computed x. */
        if(n->op==TAC_ASSIGN || mnemonicIsPure(n->op)) {
            dropFactsAbout(n->result);
            if(n->op==TAC_ASSIGN && isConstantNumber(n->arg1))
                recordFact(n->result,n->arg1,1);
        } else if(n->op==TAC_DECL) recordFact(n->result,"0",1);
        if(!out.head) out.head=out.tail=n;
        else { out.tail->next=n; out.tail=n; }
    }
    /* A second walk removes unused compiler temporaries only. User storage
     * declarations and print operations always survive. Facts are no longer
     * used after this walk, so freeing a dead instruction is safe. */
    TACInstr** link=&out.head;
    out.tail=NULL;
    while(*link) {
        TACInstr* n=*link;
        int temp=n->result && n->result[0]=='t' && isConstantNumber(n->result+1);
        if(temp && (n->op==TAC_ASSIGN || mnemonicIsPure(n->op)) &&
           !isReadLater(n->next,n->result)) {
            *link=n->next;
            free(n->arg1); free(n->arg2); free(n->result); free(n);
            changesThisPass++; optStats.deadCode++;
        } else { out.tail=n; link=&n->next; }
    }
    clearFacts();
    return out;
}

void optimizeTAC(void) {
    memset(&optStats, 0, sizeof optStats);
    optStats.instructionsBefore = countTAC(&tacList);

    TACList work = copyList(&tacList);

    int pass = 0;
    trace("Running optimization passes until no further change:\n");
    do {
        changesThisPass = 0;
        TACList next = optimizePass(&work);
        freeList(&work);
        work = next;
        pass++;
        trace("  pass %d: %3d change(s), %3d instructions\n",
               pass, changesThisPass, countTAC(&work));
    } while (changesThisPass > 0 && pass < 20);

    optimizedList = work;
    optStats.passes = pass;
    optStats.instructionsAfter = countTAC(&optimizedList);

    int removed = optStats.instructionsBefore - optStats.instructionsAfter;
    double pct = optStats.instructionsBefore
               ? (100.0 * removed / optStats.instructionsBefore) : 0.0;

    trace("\n  Technique                     Applications\n");
    trace("  ─────────────────────────────────────────\n");
    trace("  Algebraic simplification      %6d\n", optStats.algebraic);
    trace("  Constant folding              %6d\n", optStats.constFold);
    trace("  Constant propagation          %6d\n", optStats.constProp);
    trace("  Copy propagation              %6d\n", optStats.copyProp);
    trace("  Dead code elimination         %6d\n", optStats.deadCode);
    trace("  Unreachable code removal      %6d\n", optStats.unreachable);
    trace("  Branch simplification         %6d\n", optStats.branch);
    trace("  ─────────────────────────────────────────\n");
    trace("  TAC instructions   %d -> %d  (%d removed, %.1f%% smaller)\n\n",
           optStats.instructionsBefore, optStats.instructionsAfter, removed, pct);
}

/* -O0 keeps the IR unchanged so the MIPS arithmetic path can be tested and
 * shown in a video even when constant propagation removes every addition. */
void copyTACWithoutOptimization(void) {
    memset(&optStats,0,sizeof optStats);
    optimizedList=copyList(&tacList);
    optStats.instructionsBefore=countTAC(&tacList);
    optStats.instructionsAfter=countTAC(&optimizedList);
}
static void freeList(TACList* list) {
    TACInstr* n=list->head;
    while(n) {
        TACInstr* next=n->next;
        free(n->arg1); free(n->arg2); free(n->result); free(n); n=next;
    }
    list->head=list->tail=NULL;
}
void freeTAC(void) { freeList(&tacList); freeList(&optimizedList); }
