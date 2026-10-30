/* =========================================================================
 * CST-405  ·  TOPIC 3  ·  Compiling Complex Variables and Functions
 * FILE: codegen.c   —   Phase 6 — MIPS code generation
 * INPUT: optimized three-address code from tac.c
 * OUTPUT: MIPS assembly that runs in SPIM or QtSPIM
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "symtab.h"
#include "trace.h"


static FILE* out;

RegisterAllocator regAlloc;


/* Arguments waiting for CALL */

static char*
    argBuf[16];

static int
    argBufCount = 0;


/* Current function */

static int
    paramIndex = 0;

static int
    frameSize = 0;

static const char*
    currentFunc = NULL;


/* ---------------------------------------------------------
 * LABELS
 * --------------------------------------------------------- */

static const char* funcLabel(
    const char* name) {

    static char buffers[2][MAX_VAR_NAME + 8];

    static int which = 0;

    if (strcmp(
            name,
            "main") == 0) {

        return "main";
    }

    which ^= 1;

    snprintf(
        buffers[which],
        sizeof(buffers[which]),
        "fn_%s",
        name);

    return buffers[which];
}


static const char* dataLabel(
    const char* name) {

    static char buffers[2][MAX_VAR_NAME + 8];

    static int which = 0;

    which ^= 1;

    snprintf(
        buffers[which],
        sizeof(buffers[which]),
        "g_%s",
        name);

    return buffers[which];
}


/* ---------------------------------------------------------
 * HELPERS
 * --------------------------------------------------------- */

static int isConstant(
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

        if (s[i] < '0' ||
            s[i] > '9')

            return 0;
    }

    return 1;
}


static int isTemp(
    const char* s) {

    if (!s ||
        s[0] != 't' ||
        !s[1])

        return 0;

    for (int i = 1;
         s[i];
         i++) {

        if (s[i] < '0' ||
            s[i] > '9')

            return 0;
    }

    return 1;
}


/* ---------------------------------------------------------
 * REGISTER ALLOCATOR
 * --------------------------------------------------------- */

void initRegAlloc() {
    for (int i = 0;
         i < NUM_TEMP_REGS;
         i++) {

        regAlloc.regs[i]
            .varName[0] =
            '\0';

        regAlloc.regs[i]
            .isDirty =
            0;

        regAlloc.regs[i]
            .inUse =
            0;

        regAlloc.regs[i]
            .hasHome =
            0;

        regAlloc.regs[i]
            .lastUsed =
            0;
    }

    regAlloc.timestamp =
        0;

    regAlloc.spillCount =
        0;

    regAlloc.loadCount =
        0;
}


void printRegAllocStats() {
    printf(
        "  Register spills (store to memory) : %d\n",
        regAlloc.spillCount);

    printf(
        "  Register reloads (load from memory): %d\n",
        regAlloc.loadCount);
}


static int findRegister(
    const char* name) {

    for (int i = 0;
         i < NUM_TEMP_REGS;
         i++) {

        if (regAlloc.regs[i]
                .inUse &&
            strcmp(
                regAlloc.regs[i]
                    .varName,
                name) == 0) {

            regAlloc.regs[i]
                .lastUsed =
                ++regAlloc.timestamp;

            return i;
        }
    }

    return -1;
}


static void emitStoreHome(
    int reg,
    const char* name);


static void emitLoadHome(
    int reg,
    const char* name);


static int chooseVictim() {
    int victim = 0;

    int oldest =
        regAlloc.regs[0]
            .lastUsed;

    for (int i = 0;
         i < NUM_TEMP_REGS;
         i++) {

        if (!regAlloc.regs[i]
                .inUse)

            return i;

        if (regAlloc.regs[i]
                .lastUsed <
            oldest) {

            oldest =
                regAlloc.regs[i]
                    .lastUsed;

            victim = i;
        }
    }

    return victim;
}


static void spillRegister(
    int reg) {

    RegisterDescriptor* d =
        &regAlloc.regs[reg];

    if (d->inUse &&
        d->isDirty &&
        d->hasHome) {

        emitStoreHome(
            reg,
            d->varName);

        regAlloc.spillCount++;
    }

    d->isDirty = 0;
}


static int getRegister(
    const char* name,
    int hasHome,
    int load) {

    int r =
        findRegister(name);

    if (r >= 0)
        return r;

    r =
        chooseVictim();

    if (regAlloc.regs[r]
            .inUse) {

        spillRegister(r);
    }

    RegisterDescriptor* d =
        &regAlloc.regs[r];

    strncpy(
        d->varName,
        name,
        MAX_VAR_NAME - 1);

    d->varName[
        MAX_VAR_NAME - 1] =
        '\0';

    d->inUse =
        1;

    d->hasHome =
        hasHome;

    d->isDirty =
        0;

    d->lastUsed =
        ++regAlloc.timestamp;

    if (load) {
        emitLoadHome(
            r,
            name);

        regAlloc.loadCount++;
    }

    return r;
}


static int ensureRegister(
    const char* name) {

    return getRegister(
        name,
        1,
        1);
}


static int definitionRegister(
    const char* name) {

    int r =
        getRegister(
            name,
            1,
            0);

    regAlloc.regs[r]
        .isDirty =
        1;

    return r;
}


static int scratchCount =
    0;


static int scratchRegister() {
    char name[32];

    sprintf(
        name,
        "#scratch%d",
        scratchCount++);

    return getRegister(
        name,
        0,
        0);
}


static void releaseRegister(
    int r) {

    if (r < 0 ||
        r >= NUM_TEMP_REGS)

        return;

    regAlloc.regs[r]
        .varName[0] =
        '\0';

    regAlloc.regs[r]
        .isDirty =
        0;

    regAlloc.regs[r]
        .inUse =
        0;

    regAlloc.regs[r]
        .hasHome =
        0;
}


static int operandRegister(
    const char* value) {

    if (isConstant(value)) {
        int r =
            scratchRegister();

        fprintf(
            out,
            "    li   $t%d, %s\n",
            r,
            value);

        return r;
    }

    return ensureRegister(
        value);
}


static void flushRegisters(
    const char* reason) {

    (void)reason;

    for (int i = 0;
         i < NUM_TEMP_REGS;
         i++) {

        if (regAlloc.regs[i]
                .inUse) {

            spillRegister(i);

            releaseRegister(i);
        }
    }
}


/* ---------------------------------------------------------
 * MEMORY HOMES
 * --------------------------------------------------------- */

static void emitLoadHome(
    int reg,
    const char* name) {

    Symbol* s =
        lookupSymbol(name);

    if (!s) {
        fprintf(
            stderr,
            "codegen: symbol '%s' not found\n",
            name);

        exit(1);
    }

    if (s->isGlobal) {
        fprintf(
            out,
            "    lw   $t%d, %s\n",
            reg,
            dataLabel(name));

    } else {
        fprintf(
            out,
            "    lw   $t%d, %d($sp)        # %s\n",
            reg,
            s->offset,
            name);
    }
}


static void emitStoreHome(
    int reg,
    const char* name) {

    Symbol* s =
        lookupSymbol(name);

    if (!s) {
        fprintf(
            stderr,
            "codegen: symbol '%s' not found\n",
            name);

        exit(1);
    }

    if (s->isGlobal) {
        fprintf(
            out,
            "    sw   $t%d, %s\n",
            reg,
            dataLabel(name));

    } else {
        fprintf(
            out,
            "    sw   $t%d, %d($sp)        # %s\n",
            reg,
            s->offset,
            name);
    }
}


/* ---------------------------------------------------------
 * ARRAY ADDRESS
 * --------------------------------------------------------- */

static int arrayBaseRegister(
    const char* name) {

    Symbol* s =
        lookupSymbol(name);

    if (!s ||
        !s->isArray) {

        fprintf(
            stderr,
            "codegen: '%s' is not an array\n",
            name);

        exit(1);
    }

    int reg =
        scratchRegister();

    if (s->isGlobal) {
        fprintf(
            out,
            "    la   $t%d, %s\n",
            reg,
            dataLabel(name));

    } else if (
        s->isParamArray) {

        fprintf(
            out,
            "    lw   $t%d, %d($sp)        # array parameter %s\n",
            reg,
            s->offset,
            name);

    } else {
        fprintf(
            out,
            "    addi $t%d, $sp, %d        # base of %s\n",
            reg,
            s->offset,
            name);
    }

    return reg;
}


static int elementAddressRegister(
    const char* arrayName,
    const char* indexValue) {

    int base =
        arrayBaseRegister(
            arrayName);

    int index =
        operandRegister(
            indexValue);

    int offset =
        scratchRegister();

    fprintf(
        out,
        "    sll  $t%d, $t%d, 2         # index * 4\n",
        offset,
        index);

    fprintf(
        out,
        "    add  $t%d, $t%d, $t%d\n",
        base,
        base,
        offset);

    releaseRegister(
        offset);

    return base;
}


/* ---------------------------------------------------------
 * FRAME LAYOUT
 * --------------------------------------------------------- */

static void reserveTemp(
    const char* name) {

    if (!name ||
        isConstant(name) ||
        !isTemp(name))

        return;

    if (lookupSymbol(name))
        return;

    addVar(
        (char*)name,
        "int");
}


static int layoutFrame(
    TACInstr* begin) {

    initSymTab();

    for (TACInstr* i =
             begin->next;
         i &&
         i->op !=
             TAC_FUNC_END;
         i = i->next) {

        switch(i->op) {

            case TAC_PARAM:
                if (i->arg1 &&
                    strcmp(
                        i->arg1,
                        "int[]") == 0) {

                    addArrayParam(
                        i->result);

                } else {
                    addVar(
                        i->result,
                        "int");
                }

                break;


            case TAC_DECL:
                addVar(
                    i->result,
                    i->arg1
                    ? i->arg1
                    : "int");

                break;


            case TAC_ARRAY_DECL:
                addArray(
                    i->result,
                    atoi(i->arg1));

                break;


            default:
                break;
        }

        reserveTemp(
            i->result);

        reserveTemp(
            i->arg1);

        reserveTemp(
            i->arg2);
    }

    int size =
        getLocalBytes()
        + 8;

    if (size % 8)
        size +=
            8 - (size % 8);

    return size;
}


/* ---------------------------------------------------------
 * PROLOGUE / EPILOGUE
 * --------------------------------------------------------- */

static void emitPrologue(
    const char* name,
    int size) {

    fprintf(
        out,
        "\n# ==== function %s ====\n",
        name);

    fprintf(
        out,
        "%s:\n",
        funcLabel(name));

    fprintf(
        out,
        "    addi $sp, $sp, -%d\n",
        size);

    fprintf(
        out,
        "    sw   $ra, %d($sp)\n",
        size - 4);
}


static void emitEpilogue(
    const char* name,
    int size) {

    fprintf(
        out,
        "%s__epilogue:\n",
        funcLabel(name));

    fprintf(
        out,
        "    lw   $ra, %d($sp)\n",
        size - 4);

    fprintf(
        out,
        "    addi $sp, $sp, %d\n",
        size);

    if (strcmp(
            name,
            "main") == 0) {

        fprintf(
            out,
            "    li   $v0, 10\n");

        fprintf(
            out,
            "    syscall\n");

    } else {
        fprintf(
            out,
            "    jr   $ra\n");
    }
}


/* ---------------------------------------------------------
 * GLOBAL DATA
 * --------------------------------------------------------- */

static void emitDataSection(
    TACInstr* head) {

    initGlobalScope();

    fprintf(
        out,
        ".data\n");

    fprintf(
        out,
        "__nl: .asciiz \"\\n\"\n");

    int insideFunction = 0;

    for (TACInstr* i = head;
         i;
         i = i->next) {

        if (i->op ==
            TAC_FUNC_BEGIN) {

            insideFunction = 1;
            continue;
        }

        if (i->op ==
            TAC_FUNC_END) {

            insideFunction = 0;
            continue;
        }

        if (insideFunction)
            continue;

        if (i->op ==
            TAC_DECL) {

            addGlobalVar(
                i->result,
                i->arg1
                ? i->arg1
                : "int");

            fprintf(
                out,
                ".align 2\n%s: .word 0\n",
                dataLabel(
                    i->result));

        } else if (
            i->op ==
            TAC_ARRAY_DECL) {

            int size =
                atoi(i->arg1);

            addGlobalArray(
                i->result,
                size);

            fprintf(
                out,
                ".align 2\n%s: .space %d\n",
                dataLabel(
                    i->result),
                size * 4);
        }
    }

    fprintf(
        out,
        "\n.text\n");

    fprintf(
        out,
        ".globl main\n");
}


/* ---------------------------------------------------------
 * MAIN CODE GENERATION
 * --------------------------------------------------------- */

void generateMIPSFromTAC(
    const char* filename) {

    out =
        fopen(
            filename,
            "w");

    if (!out) {
        fprintf(
            stderr,
            "Cannot open output file %s\n",
            filename);

        exit(1);
    }

    TACList* list =
        getOptimizedTAC();

    if (!list ||
        !list->head) {

        fprintf(
            stderr,
            "No TAC available for code generation\n");

        fclose(out);

        return;
    }

    emitDataSection(
        list->head);


    for (TACInstr* begin =
             list->head;
         begin;
         begin = begin->next) {

        if (begin->op !=
            TAC_FUNC_BEGIN)

            continue;

        currentFunc =
            begin->result;

        frameSize =
            layoutFrame(begin);

        trace(
            "\nActivation record for '%s': %d bytes\n",
            currentFunc,
            frameSize);

        printSymTab();

        paramIndex =
            0;

        argBufCount =
            0;

        initRegAlloc();

        emitPrologue(
            currentFunc,
            frameSize);


        for (TACInstr* i =
                 begin->next;
             i &&
             i->op !=
                 TAC_FUNC_END;
             i = i->next) {

            switch(i->op) {

                case TAC_PARAM: {
                    Symbol* s =
                        lookupSymbol(
                            i->result);

                    if (paramIndex >=
                        MAX_ARGS) {

                        fprintf(
                            stderr,
                            "codegen: more than %d function parameters are not supported\n",
                            MAX_ARGS);

                        exit(1);
                    }

                    fprintf(
                        out,
                        "    sw   $a%d, %d($sp)        # parameter %s\n",
                        paramIndex,
                        s->offset,
                        i->result);

                    paramIndex++;

                    break;
                }


                case TAC_DECL:
                case TAC_ARRAY_DECL:
                    /*
                     * Storage already assigned
                     * by layoutFrame().
                     */
                    break;


                case TAC_ADD:
                case TAC_SUB:
                case TAC_MUL:
                case TAC_DIV: {
                    int a =
                        operandRegister(
                            i->arg1);

                    int b =
                        operandRegister(
                            i->arg2);

                    int d =
                        definitionRegister(
                            i->result);

                    const char* op =
                        "add";

                    if (i->op ==
                        TAC_SUB)

                        op = "sub";

                    else if (
                        i->op ==
                        TAC_MUL)

                        op = "mul";

                    else if (
                        i->op ==
                        TAC_DIV)

                        op = "div";

                    fprintf(
                        out,
                        "    %s  $t%d, $t%d, $t%d\n",
                        op,
                        d,
                        a,
                        b);

                    break;
                }


                case TAC_NEG: {
                    int a =
                        operandRegister(
                            i->arg1);

                    int d =
                        definitionRegister(
                            i->result);

                    fprintf(
                        out,
                        "    sub  $t%d, $zero, $t%d\n",
                        d,
                        a);

                    break;
                }


                case TAC_ASSIGN: {
                    int a =
                        operandRegister(
                            i->arg1);

                    int d =
                        definitionRegister(
                            i->result);

                    fprintf(
                        out,
                        "    move $t%d, $t%d\n",
                        d,
                        a);

                    break;
                }


                case TAC_PRINT: {
                    int a =
                        operandRegister(
                            i->arg1);

                    fprintf(
                        out,
                        "    move $a0, $t%d\n",
                        a);

                    fprintf(
                        out,
                        "    li   $v0, 1\n");

                    fprintf(
                        out,
                        "    syscall\n");

                    fprintf(
                        out,
                        "    la   $a0, __nl\n");

                    fprintf(
                        out,
                        "    li   $v0, 4\n");

                    fprintf(
                        out,
                        "    syscall\n");

                    break;
                }


                case TAC_ARG:
                    if (argBufCount <
                        16) {

                        argBuf[
                            argBufCount++] =
                            i->arg1;
                    }

                    break;


                case TAC_CALL: {
                    int count =
                        atoi(
                            i->arg2
                            ? i->arg2
                            : "0");

                    if (count >
                        MAX_ARGS) {

                        fprintf(
                            stderr,
                            "codegen: call to '%s' has too many arguments\n",
                            i->arg1);

                        exit(1);
                    }

                    int first =
                        argBufCount -
                        count;

                    if (first < 0)
                        first = 0;

                    /*
                     * Put arguments into $a0-$a3
                     * before flushing caller-saved registers.
                     */

                    for (int k = 0;
                         k < count;
                         k++) {

                        const char* value =
                            argBuf[
                                first + k];

                        Symbol* s =
                            lookupSymbol(
                                value);

                        if (s &&
                            s->isArray) {

                            int r =
                                arrayBaseRegister(
                                    value);

                            fprintf(
                                out,
                                "    move $a%d, $t%d\n",
                                k,
                                r);

                            releaseRegister(
                                r);

                        } else {
                            int r =
                                operandRegister(
                                    value);

                            fprintf(
                                out,
                                "    move $a%d, $t%d\n",
                                k,
                                r);
                        }
                    }

                    argBufCount =
                        first;

                    flushRegisters(
                        "before call");

                    fprintf(
                        out,
                        "    jal  %s\n",
                        funcLabel(
                            i->arg1));

                    int d =
                        definitionRegister(
                            i->result);

                    fprintf(
                        out,
                        "    move $t%d, $v0\n",
                        d);

                    break;
                }


                case TAC_RETURN:
                    if (i->arg1) {
                        int r =
                            operandRegister(
                                i->arg1);

                        fprintf(
                            out,
                            "    move $v0, $t%d\n",
                            r);
                    }

                    flushRegisters(
                        "return");

                    fprintf(
                        out,
                        "    j    %s__epilogue\n",
                        funcLabel(
                            currentFunc));

                    break;

                case TAC_LABEL:
                    flushRegisters("control-flow label");
                    fprintf(out, "%s:\n", i->result);
                    break;

                case TAC_GOTO:
                    flushRegisters("before jump");
                    fprintf(out, "    j    %s\n", i->result);
                    break;

                case TAC_IF_FALSE: {
                    int condition = operandRegister(i->arg1);
                    /* Dirty values must reach memory on both branch paths. */
                    flushRegisters("before conditional branch");
                    fprintf(out, "    beq  $t%d, $zero, %s\n",
                            condition, i->result);
                    break;
                }


                case TAC_ARRAY_LOAD: {
                    int addr =
                        elementAddressRegister(
                            i->arg1,
                            i->arg2);

                    int dest =
                        definitionRegister(
                            i->result);

                    fprintf(
                        out,
                        "    lw   $t%d, 0($t%d)\n",
                        dest,
                        addr);

                    releaseRegister(
                        addr);

                    break;
                }


                case TAC_ARRAY_STORE: {
                    int value =
                        operandRegister(
                            i->result);

                    int addr =
                        elementAddressRegister(
                            i->arg1,
                            i->arg2);

                    fprintf(
                        out,
                        "    sw   $t%d, 0($t%d)\n",
                        value,
                        addr);

                    releaseRegister(
                        addr);

                    break;
                }


                default:
                    break;
            }
        }

        flushRegisters(
            "end of function");

        emitEpilogue(
            currentFunc,
            frameSize);
    }

    fclose(out);
}
