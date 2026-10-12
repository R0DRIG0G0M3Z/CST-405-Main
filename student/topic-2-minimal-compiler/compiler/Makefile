# ============================================================================
# CST-405 mini compiler — build rules
# ----------------------------------------------------------------------------
#   make            build ./minicompiler
#   make test       compile and run every program in tests/ under SPIM
#   make clean      remove everything that `make` produced
#
# The two generated files are worth knowing about:
#   lex.yy.c        written by flex from scanner.l
#   parser.tab.c/.h written by bison from parser.y
# Never edit those two — edit the .l and .y files and rebuild.
# ============================================================================
CC     = gcc
LEX    = flex
YACC   = bison
CFLAGS ?= -g -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L

TARGET = minicompiler
OBJS   = lex.yy.o parser.tab.o main.o ast.o parse_tree.o symtab.o semantic.o codegen.o tac.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# The scanner needs the token numbers that bison assigns, so parser.tab.h
# must exist before flex output is compiled.
lex.yy.c: scanner.l parser.tab.h
	$(LEX) scanner.l

parser.tab.c parser.tab.h &: parser.y ast.h parse_tree.h
	$(YACC) -Wall -d parser.y

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -c lex.yy.c

parser.tab.o: parser.tab.c
	$(CC) $(CFLAGS) -c parser.tab.c

main.o:     main.c ast.h semantic.h codegen.h tac.h trace.h parse_tree.h
	$(CC) $(CFLAGS) -c main.c
ast.o:      ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c
parse_tree.o: parse_tree.c parse_tree.h
	$(CC) $(CFLAGS) -c parse_tree.c
symtab.o:   symtab.c symtab.h trace.h
	$(CC) $(CFLAGS) -c symtab.c
semantic.o: semantic.c semantic.h ast.h symtab.h trace.h
	$(CC) $(CFLAGS) -c semantic.c
codegen.o:  codegen.c codegen.h ast.h symtab.h tac.h trace.h
	$(CC) $(CFLAGS) -c codegen.c
tac.o:      tac.c tac.h ast.h trace.h
	$(CC) $(CFLAGS) -c tac.c

# ----------------------------------------------------------------------------
# Compile every tests/*.cm file and run the result in SPIM.  Each test file
# states its expected output in a header comment; compare and you have a
# regression suite.
# ----------------------------------------------------------------------------
test: $(TARGET)
	python3 tests/run_tests.py

clean:
	rm -f $(TARGET) $(OBJS) lex.yy.c parser.tab.c parser.tab.h parser.output
	rm -f tests/*.s tests/*.tac *.s *.tac

.PHONY: all clean test
