CC = gcc
FLEX = flex
CFLAGS = -Wall -Wextra -std=c11
TARGET = lexer

all: $(TARGET)

$(TARGET): main.o lex.yy.o
	$(CC) $(CFLAGS) main.o lex.yy.o -o $(TARGET)

main.o: main.c tokens.h
	$(CC) $(CFLAGS) -c main.c

lex.yy.o: lex.yy.c tokens.h
	$(CC) $(CFLAGS) -c lex.yy.c

lex.yy.c: scanner.l tokens.h
	$(FLEX) scanner.l

test: $(TARGET)
	@echo "Running clean tests"
	@./$(TARGET) tests/01_all_tokens.cm -c
	@./$(TARGET) tests/02_longest_match.cm -c
	@./$(TARGET) tests/03_keywords_vs_ids.cm -c
	@./$(TARGET) tests/04_comments.cm -c
	@./$(TARGET) tests/07_student_program.cm -c
	@echo "Running expected error tests"
	@./$(TARGET) tests/05_lexical_errors.cm -c; test $$? -eq 1
	@./$(TARGET) tests/06_unterminated_comment.cm -c; test $$? -eq 1
	@echo "All tests returned the expected status."

clean:
	rm -f $(TARGET) lex.yy.c *.o

.PHONY: all test clean
