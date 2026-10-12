/* CST-405 Project 2: concrete parse tree creation, display, and cleanup.
 * Input: Bison grammar reductions. Output: the full derivation tree.
 * Each parent owns its children; leaves own copies of their token text.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "parse_tree.h"
ParseTree* createParseTree(const char* label, int count, ...) {
    if(count<0 || count>PT_MAX_CHILDREN) { fprintf(stderr,"Invalid parse tree arity\n"); exit(1); }
    ParseTree* n=calloc(1,sizeof *n);
    if(!n) { perror("Parse tree allocation"); exit(1); }
    n->label=strdup(label); n->count=count;
    va_list args; va_start(args,count);
    for(int i=0;i<count;i++) n->children[i]=va_arg(args,ParseTree*);
    va_end(args); return n;
}
void printParseTree(const ParseTree* n, int level) {
    if(!n) return;
    printf("%*s%s\n",2*level,"",n->label);
    for(int i=0;i<n->count;i++) printParseTree(n->children[i],level+1);
}
void freeParseTree(ParseTree* n) {
    if(!n) return;
    for(int i=0;i<n->count;i++) freeParseTree(n->children[i]);
    free(n->label); free(n);
}
