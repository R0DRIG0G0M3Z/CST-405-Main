# CST-405 Project 2: Minimalist Language Compiler

Complete implementation of the Topic 2 course starter from `isac-artzi/CST-405`.
The compiler translates a small integer language into MIPS assembly. Course
interfaces, driver, storage helpers and register-cache foundations are credited
to the instructor; completed sections and validation are described below.

## Build in Ubuntu / VS Code WSL

```bash
sudo apt update
sudo apt install build-essential flex bison spim python3
cd student/topic-2-minimal-compiler/compiler
make
make test
```

Run `cd ...` from the repository root. GNU Make 4.3 or newer is required for the
grouped Bison output rule. Generated `parser.tab.c`, `parser.tab.h` and `lex.yy.c`
are build products; edit `parser.y` and `scanner.l` instead.

## Demo

```bash
./minicompiler tests/t2_01_basics.cm demo.s
cat demo.tac
cat demo.optimized.tac
spim -file demo.s
./minicompiler tests/t2_01_basics.cm demo_raw.s -q -O0
spim -file demo_raw.s
./minicompiler tests/t2_04_errors_undeclared.cm bad_semantic.s -q
./minicompiler tests/t2_05_errors_syntax.cm bad_syntax.s -q
```

The expected program values for the first example are 5, 10 and 18, each on a
separate line. These are comparison values, not a substitute for execution
proof. `-q` hides educational traces; `-O0` preserves the original TAC so the
actual MIPS addition instructions can be inspected and executed.

For the assignment recording, use **MARS or QtSPIM** as requested in the
checklist. In MARS, open the generated `.s`, assemble it, then run it. The Run I/O
pane displays program output. MARS can run natively on Windows while the compiler
is built in WSL; locate the generated file through `explorer.exe .` in the WSL
compiler directory. The original MARS distribution is at
<https://github.com/dpetersanderson/MARS/releases>.

Optional command-line MARS validation (set the actual downloaded JAR path):

```bash
MARS_JAR=/path/to/Mars4_5.jar make test
java -jar /path/to/Mars4_5.jar nc sm ae1 se1 demo.s
```

## Exact language

```text
program    -> stmt_list
stmt_list  -> stmt | stmt_list stmt
stmt       -> decl | assign | print_stmt
decl       -> int ID ;
assign     -> ID = expr ;
expr       -> NUM | ID | expr + expr
print_stmt -> print ( expr ) ;
```

Addition is left associative. General expression parentheses, subtraction,
initializers such as `int x = 1;`, functions, arrays, loops and conditionals are
outside Topic 2. Both `//` and `/* ... */` comments are supported. Blank programs
are rejected. Integer variables start at zero; addition wraps in signed 32-bit
arithmetic, using `addu` so optimized and unoptimized behavior agrees at overflow.
Literals range from 0 to 2147483647. Identifiers are limited to 63 characters,
with at most 128 user variables. Names `t` followed only by digits, `L` followed
only by digits, and the prefix `__sw` are reserved by the course IR design.

## Checklist mapping

| Checklist requirement | Implementation / evidence |
| --- | --- |
| Lexical analysis | `scanner.l`: tokens, exact locations, invalid characters, integer bounds, unterminated block comments |
| Grammar compliance and syntax errors | `parser.y`: complete grammar, detailed line/column diagnostics, semicolon recovery; recovered errors still fail compilation |
| Symbol table | `semantic.c`: declaration visibility; `symtab.c`: insertion, local/global lookup and stack offsets; tables shown in verbose trace |
| Parse tree creation and display | `parse_tree.c/.h`, built in Bison reductions and displayed with punctuation |
| Separate AST functions | `ast.c/.h`: all seven node kinds, constructors, display and cleanup |
| Lexical/syntax certification | `main.c`: checks parse status plus lexical/syntax error counters before continuing |
| AST semantic traversal and errors | `semantic.c`: undeclared uses/targets, duplicates, reserved names, scope capacity; reports locations and multiple errors |
| Semantic certification | Semantic summary and driver gate; IR generation begins only after successful analysis |
| Three-address IR saved | `tac.c`: postorder expression traversal, statement emission, saved `.tac` |
| Optimization | `tac.c`: constant propagation/folding, local addition-by-zero simplification, removal of unused temporaries; saved `.optimized.tac` |
| Flow optimization where possible | No source branches or loops exist in this grammar, so no control-flow optimization opportunity exists |
| Additional optimizations with instructor consultation | Folding/propagation follow the starter's instructions; no claim of extra instructor-approved techniques |
| Separate assembly generator / file | `codegen.c`: optimized TAC to `.s`, stack storage, register cache/spilling, print/exit syscalls |
| MIPS execution | `make test` executes real generated assembly under SPIM; `MARS_JAR=... make test` uses MARS |

The code generator consumes the semantic-checked AST **through TAC**, matching
the course pipeline: AST -> semantic validation -> TAC -> optimized TAC -> MIPS.
Semantic analysis checks the existing AST; it does not create a second AST.
A parse tree retains grammar symbols and punctuation; an AST keeps the structure
needed by later phases. These are separate structures with separate destructors.

## Optimization example to explain

For `x = 5; y = 10; total = x + y + 3;`, the original IR uses temporary sums.
Propagation substitutes the known values 5 and 10. Folding computes 15 and then
18. Later passes remove temporary assignments that are no longer read. Each
assignment invalidates facts about its destination **after** reading the RHS, so
`x = x + 1;` uses the old value correctly. Declarations and print effects remain.

The target back end reserves homes for variables and temporary values, caches
values in `$t0` through `$t9`, spills them when needed, and pins operands during
instruction selection so allocating a destination cannot destroy an input.
TAC `PRINT` becomes integer-print syscall 1 followed by newline-print syscall 4.
`RETURN` jumps to the main epilogue, which uses exit syscall 10.

## Automated checks

`tests/run_tests.py` asserts compiler exit codes, diagnostic locations, absence
of output artifacts for new invalid inputs, and real simulator values. It covers
all five supplied files plus duplicates, unknown characters, comments, overflow,
reserved names, capacity limits, long expressions, 40 deterministic random
programs, zero initialization, reassignment, temporary reuse and register spills.
Both optimized and `-O0` paths are executed. There are 118 checks per simulator.
Compilation time statistics measure compiler CPU time, not generated-program
speed. Compilation errors return nonzero; failed source checks never proceed to
IR generation. Use fresh output names for error demos; existing files from a
previous successful compilation are not deleted on source errors.

## Individual video and submission

Every team member must record their **own** demonstration. Explain the real
assigned tasks you completed; do not claim the entire shared code as an
individual contribution. Keep a simple contributions record identifying each
person, their actual work and the relevant commits or files.

Recommended recording order:

1. Identify yourself and your actual assigned tasks.
2. Show the source language and the files for each compiler phase.
3. Build, run `make test`, and compile the basics example with the full trace.
4. Compare parse tree and AST; follow `total` through AST, TAC and optimized TAC.
5. Show `demo_raw.s` from `-O0` and explain a sum, storage and print syscalls.
6. Assemble/run `demo.s` in MARS or QtSPIM and show the real result.
7. Compile the two supplied invalid programs and explain the error gates.
8. Finish with your contribution and what the test comparison established.

Submit the private repository link **including the branch containing this
implementation**, and an individual video link for each team member as directed
by the instructor. Verify the instructor can access the private repo and each
video. Code and a recording guide do not replace the required recordings.
