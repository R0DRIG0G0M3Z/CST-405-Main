/* =========================================================================
 * CST-405  ·  TOPIC 2  ·  Compiler for a Starter Language
 * FILE: semantic.h   —   Phase 3 — Semantic analysis
 * -------------------------------------------------------------------------
 * THE PIPELINE, AND WHERE THIS FILE SITS IN IT
 *   scanner -> parser -> ast -> semantic -> tac -> codegen
 *                               ^^^^^^^^  this file
 *
 * UNCHANGED SINCE TOPIC 1 — the interfaces held, which is the point
 *
 * WHAT COMES NEXT
 *   Topic 3 adds functions, arrays and the rest of arithmetic — and with them, real activation records.
 *
 * PROJECT 2 IMPLEMENTATION
 *   Interfaces and register/storage foundations are adapted from the course
 *   starter by Isac Artzi. Completed implementation is described in README.
 * ========================================================================= */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/* SEMANTIC ANALYSIS
 * This phase checks the semantic correctness of the program
 * Ensures variables are declared before use, no redeclarations, etc.
 * Topic 2 uses one scope and integer expressions
 */

/* SEMANTIC ERROR TRACKING */
typedef struct {
    int errorCount;           /* Number of semantic errors found */
    int warningCount;         /* Number of warnings issued */
} SemanticInfo;

/* SEMANTIC ANALYSIS FUNCTIONS */
void initSemantic();                     /* Initialize semantic analyzer */
int performSemanticAnalysis(ASTNode* root);  /* Run semantic checks on AST, returns 0 if successful */
void printSemanticSummary();             /* Print summary of semantic analysis results */

#endif
