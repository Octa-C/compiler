# OctaC Lexer

The lexical analyser for the OctaC language. It reads a `.oc` source file 
and prints one token per line.

## Build

Requires `g++` and `make`. Run all commands from the repository root.

```sh
make            # builds build/lexer
make test       # builds, then runs the pytest suite
make clean      # removes the build/ directory
```

To use another compiler or flags: `make CXX=clang++ CXXFLAGS="-O0 -g"`.

## Usage

```sh
build/lexer <file.oc>              # print the token stream
build/lexer <file.oc> -o <out>     # write the token stream to <out>
build/lexer --dump-table           # print the DFA transition table (markdown)
```

`-o <out>` can also come before the file. Diagnostics go to stderr in either
case.

### Output

Tokens go to stdout as `<TYPE, value>`, one per line, ending with
`<COMPILER_EOF,>`. Only some tokens have a value part: identifiers, scalar
type names, integer, float and string literals, bool constants, arithmetic
operators and assignment operators. Every other token is written with an
empty value part, as `<TYPE,>`:

```
<FUNCTION,>
<IDENTIFIER, dot>
<LPAREN,>
<VECTOR,>
<SCALAR, f32>
```

Whitespace is skipped and produces no token.

### Errors

An unrecognized lexeme is emitted as a `<COMPILER_ERROR,>` token on stdout, and
a diagnostic naming the bad lexeme is written to stderr in `file:line:col`
form:

```
tests/lexer/cases/errors.oc:3:10: error: unrecognized lexeme '..'
```

Scanning continues after an error, so one run reports every problem in the
file. Line and column numbers are 1-based.

Exit codes: `0` when the file was scanned (even if it contained lexical
errors, so check stderr or look for `COMPILER_ERROR` tokens), `2` for a usage
error, an unreadable input file or an output file that cannot be written.

## Testing

The tests use pytest. From the repository root:

```sh
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
```

Run them with:

```sh
make test                       # build, then run every case
make test ARGS=-v               # list every error in each failing case
make test ARGS="-k matmul"      # selected cases
```

Each `tests/lexer/cases/<name>.oc` is run through the lexer as one test, which checks:

- the token stream against `tests/lexer/expected/<name>.toks`
- the diagnostics printed to stderr against
  `tests/lexer/expected/diagnostics/<name>.stderr`. When that file does not exist, no
  diagnostics are expected.

Each line of a `.toks` file is one token, written as `<TYPE, value>`, for
example `<IDENTIFIER, main>`. A token without a value has an empty value part,
for example `<LBRACE,>`.

A failure gives the line in the expected file, what was expected and what the
lexer produced:

```
matmul.toks:3 Expected: <LPAREN,>, Got <RBRACKET,>
```

Only the first error of a case is shown by default. With `-v` every error is
listed.

## Lexical structure

| Class | Lexemes |
|---|---|
| Keywords | `for` `do` `until` `continue` `function` `return` `if` `else` `elseif` `caseof` `case` `default` `and` `or` `not` `is` `in` `break` |
| Scalar types (`SCALAR`) | `i64` `i32` `i16` `f64` `f32` `f16` `bool` |
| Other types | `vector` `matrix` `string` |
| Identifier | `[a-zA-Z][a-zA-Z0-9_]*` (cannot start with `_`) |
| Integer literal | `[0-9]+` |
| Float literal | `[0-9]+\.[0-9]+` or `[0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+` (`1.` and `.5` are errors) |
| String literal | `"[^"\n]*"` (no escapes, single line) |
| Arithmetic | `+` `-` `*` `/` `%` |
| Matrix | `'` (transpose), `.*`, `./` |
| Comparison | `<` `>` `<=` `>=` |
| Assignment | `=`, `+=`, `-=`, `*=`, `/=` (each is a single token) |
| Punctuation | `{` `}` `(` `)` `[` `]` `;` `,` `:` `->` `...` |
| Comment | `//` to the end of the line, skipped and produces no token |

`BOOL` exists in the token enum but is never produced by
the lexer; `0` and `1` scan as integer literals and the parser is expected to
interpret them.

## How it works

The scanner is a DFA driven by a transition table.

1. **Character classes.** Each input byte maps to one of 17 classes
   (`CC_LETTER`, `CC_DIGIT`, `CC_DOT`, ...) via `charClassOf`.
2. **Transition table.** `TRANSITION[state][class]` in `src/dfa.cpp` gives the
   next state, with 19 states in total. `ACTION[state]` says what an accepting
   state produces.
3. **Maximal munch with backtracking.** `Scanner::next` walks the table as far
   as it can, remembering the last accepting state. When it hits `S_DEAD` it
   rewinds to that point. So `0...2` scans as `0`, `...`, `2`.
4. **Lookup after acceptance.** The DFA only recognizes shapes (word, number,
   operator). Keywords and operators are then resolved by lexeme lookup in
   `src/token.cpp`, which keeps the table small.
5. **Errors.** If no accepting state was ever reached, the consumed text (at
   least one character) becomes a `COMPILER_ERROR` token and scanning resumes
   right after it.

Run `build/lexer --dump-table` to print the full table.

## Project layout

Source lives in `lexer/`. The Makefile, `requirements.txt`, `.venv` and the
`build/` output are in the repository root, and the tests are in `tests/lexer/`.

```
lexer
├── include
│   ├── dfa.hpp                       # CharClass, State, Action enums and table declarations
│   ├── scanner.hpp                   # Scanner class
│   └── token.hpp                     # TokenType enum, Token struct, lookup declarations
├── src
│   ├── dfa.cpp                       # character classes, TRANSITION and ACTION tables
│   ├── scanner.cpp                   # the scanning loop and line/column tracking
│   └── token.cpp                     # token names, keyword and operator lookup tables
├── main.cpp                          # command line driver and --dump-table
└── README.md

tests/lexer
├── cases                             # .oc inputs (errors, sample, matmul, ...)
├── expected                          # <name>.toks, and diagnostics/<name>.stderr
├── conftest.py                       # the --lexer option and the lexer fixture
├── test_lexer.py                     # the tests that run every case
└── testlib.py                        # helpers for the tests
```
