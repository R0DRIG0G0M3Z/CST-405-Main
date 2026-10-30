# CST-405 Project 3 - Complex Variables and Functions

This project extends the Topic 2 compiler with full integer arithmetic, arrays,
functions, scope, and MIPS activation records. The implementation keeps the same
six-phase pipeline used in the earlier project:

`scanner -> parser/AST -> semantic analysis -> TAC -> optimization -> MIPS`

## Implemented features

- Arithmetic with `+`, `-`, `*`, `/`, parentheses, unary minus, and the required
  precedence and associativity.
- Global and local integer arrays, array reads and writes, and computed indexes.
- Function definitions, zero to four parameters, calls, return values, forward
  references, nested calls, and recursion.
- Lexical scope. Parameters and local variables can shadow globals without
  changing the global value.
- A separate activation record for every call. The prologue reserves the frame
  and saves `$ra`; every return jumps to one epilogue, which restores `$ra` and
  `$sp`.
- Two-pass semantic analysis. The first pass records every function signature,
  and the second pass checks bodies and calls.
- Compatibility with the original flat Topic 2 programs. When a source file has
  no function definition, the parser places its statements in an implicit
  `main` function so the previous tests still run unchanged.

The small `if (expression) statement` rule is included so the required recursive
factorial test has a base case. Full `if/else` support is still reserved for the
later control-flow topic.

## Arrays and storage

A local array reserves `size * 4` bytes in its function frame. A global array is
placed in the `.data` section. These two cases use different base addresses:

- local array: `$sp + frame offset`
- global array: the address of its data label

Arrays are passed by reference. An `int values[]` parameter uses one four-byte
slot containing the caller's base address instead of copying every element. The
parameter has no size because only that address is passed; the callee would need
a separate count parameter if it needed the length.

The compiler intentionally uses two different symbol tables. `semantic.c` keeps
a stack of scopes to answer whether a name is visible. `symtab.c` assigns storage
locations to globals, parameters, locals, arrays, and temporary values during
code generation. Keeping those jobs separate made it possible to add functions
without changing the boundary between the AST, TAC, and MIPS phases.

## Build and run

Requirements: `gcc`, `make`, `flex`, `bison`, and SPIM or QtSPIM.

```bash
cd student/topic-3-arrays-and-functions/compiler
make clean
make
make test
```

To compile and run one program with the full trace:

```bash
./minicompiler tests/t3_06_recursion.cm factorial.s
spim -file factorial.s
```

Use `-q` after the output filename for a shorter compiler report. Compilation
also creates `.tac` and `.optimized.tac` files beside the requested assembly
file. These generated files are ignored by Git.

## Test coverage

The test runner compiles and executes all successful tests and compares their
SPIM output with the expected values. It also verifies that the intentional
syntax and semantic error programs are rejected.

Important Project 3 checks include:

- required arithmetic results, including `2 + 3 * 4 = 14` and
  `17 - 5 - 3 = 9`
- local and global array access
- array modification through a reference parameter
- nested calls on either side of an argument list
- a call to a function defined later in the file
- `factorial(5) = 120`
- a return in the middle of a function jumping to its epilogue
- all five original Project 2 tests

## Known limitations

- Only the `int` type is supported.
- Calls are limited to four arguments because the compiler uses `$a0-$a3`.
- Array bounds are not checked at run time.
- Arrays are one-dimensional.
- Full conditionals and loops belong to later course topics.

These limits are outside the required Project 3 functionality and are reported
instead of silently generating incorrect code.
