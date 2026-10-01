# CST-405 Project 1 Lexical Analyzer

This project is a lexical analyzer created with Flex. It recognizes the tokens used in the course language and prints their kind, text, line, and column. It also reports lexical errors and continues scanning the rest of the file.

## Build

```bash
make
```

## Run one test

```bash
./lexer tests/01_all_tokens.cm
```

Use `-c` to display only the token counts and final result:

```bash
./lexer tests/01_all_tokens.cm -c
```

## Run all tests

```bash
make test
```

Tests 01 through 04 and test 07 should finish without lexical errors. Test 05 contains four invalid characters and test 06 contains an unterminated block comment.

## Files

- `scanner.l` - Flex rules for recognizing tokens and errors
- `tokens.h` - token definitions used by the lexer
- `main.c` - runs the scanner and prints the results
- `Makefile` - builds the lexer and runs the tests
- `tests/` - test files for valid input, comments, operators, and lexical errors
- `CONTRIBUTIONS.md` - my contribution to the project
