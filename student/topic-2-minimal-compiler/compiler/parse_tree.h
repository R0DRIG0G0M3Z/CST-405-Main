/* Concrete syntax tree interface: retains grammar symbols and punctuation. */
#ifndef PARSE_TREE_H
#define PARSE_TREE_H
#define PT_MAX_CHILDREN 5
typedef struct ParseTree {
    char* label;
    int count;
    struct ParseTree* children[PT_MAX_CHILDREN];
} ParseTree;
ParseTree* createParseTree(const char* label, int count, ...);
void printParseTree(const ParseTree* tree, int level);
void freeParseTree(ParseTree* tree);
#endif
