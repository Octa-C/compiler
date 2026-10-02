# OctaC Lexer

The lexical analyser for the OctaC language. It splits a `.oc` source file into
tokens, which the compiler driver prints with `--emit-tokens` and the parser
will consume.

## Usage

The lexer is built into the compiler driver, which the Makefile in the repository
root builds as `build/octacc`. The `--emit-tokens` flag stops the compiler after
lexical analysis and prints the token stream.

```sh
build/octacc --emit-tokens <file.oc>              # print the token stream
build/octacc --emit-tokens <file.oc> -o <out>     # write the token stream to <out>
build/octacc <file.oc>                            # lex the file and report errors only
```

Options can come before or after the file, and `-o` requires `--emit-tokens`.
Diagnostics go to stderr in every case.

### Output

Tokens go to stdout as `<TYPE, value>`, one per line, ending with
`<COMPILER_EOF,>`. Only some tokens have a value part: identifiers, scalar
type names, integer, float, string and bool literals, arithmetic operators and
assignment operators. Every other token is written with an empty value part, as
`<TYPE,>`:

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
a diagnostic naming the bad lexeme is written to stderr. It gives the position
in `file:line:col` form, the source line and a marker under the lexeme:

```
tests/lexer/cases/errors.oc:3:10: error: unrecognized lexeme '..'
 3 | i32 y = 3..4;
   |          ^~
```

Diagnostics are coloured when stderr is a terminal. Setting `NO_COLOR` turns the
colour off. Scanning continues after an error, so one run reports every problem
in the file. Line and column numbers are 1-based.

Exit codes: `0` when the file was scanned without errors, `1` when it contained
lexical errors (the token stream is still produced), `2` for a usage error, an
unreadable input file or an output file that cannot be written.

## Testing

The lexer tests live in `tests/lexer` and run with `make test` from the
repository root. Each `tests/lexer/cases/<name>.oc` is run through
`octacc --emit-tokens` as one test, which checks:

- the token stream against `tests/lexer/expected/<name>.toks`
- the diagnostics printed to stderr against
  `tests/lexer/expected/diagnostics/<name>.stderr`. When that file does not
  exist, no diagnostics are expected.
- the exit code, which should be `1` when diagnostics are expected and `0`
  otherwise.

Each line of a `.toks` file is one token in the output format above.

A failure gives the line in the expected file, what was expected and what the
lexer produced:

```
matmul.toks:3 Expected: <LPAREN,>, Got <RBRACKET,>
```

Only the first error of a case is shown by default. With `make test ARGS=-v`
every error is listed.

## Lexical structure

| Class | Lexemes |
|---|---|
| Keywords | `for` `do` `until` `continue` `function` `return` `if` `else` `elseif` `caseof` `case` `default` `and` `or` `not` `is` `in` `break` |
| Scalar types (`SCALAR`) | `i64` `i32` `i16` `f64` `f32` `f16` `bool` |
| Vector type (`VECTOR`) | `vector` |
| Matrix type (`MATRIX`) | `matrix` |
| String type (`STRING`) | `string` |
| Bool literal (`BOOL_LIT`) | `true` `false` |
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

## How it works

The lexer is a DFA driven by a transition table.

1. **Character classes.** Each input byte maps to one of 17 classes
   (`CharClass::Letter`, `CharClass::Digit`, `CharClass::Dot`, ...) via
   `charClassOf`.
2. **Transition table.** `nextState(state, class)` in `src/dfa.cpp` gives the
   next state, with 19 states in total. `actionOf(state)` says what an
   accepting state produces.
3. **Maximal munch with backtracking.** `Scanner::next` walks the table as far
   as it can, remembering the last accepting state. When it reaches
   `State::Dead` it rewinds to that point. So `0...2` scans as `0`, `...`, `2`.
4. **Lookup after acceptance.** The DFA only recognizes shapes (word, number,
   operator). Keywords and operators are then resolved by lexeme lookup in
   `src/token.cpp`, which keeps the table small.
5. **Errors.** If no accepting state was ever reached, the consumed text (at
   least one character) becomes a `COMPILER_ERROR` token, is reported to the
   `DiagnosticReporter`, and scanning resumes right after it.

`Scanner::tokenize` runs `Scanner::next` to the end of the input and returns
every token, ending with `COMPILER_EOF`.

## Layout

```
lexer
├── include
│   ├── dfa.hpp         # CharClass, State and Action enums, DFA lookups
│   ├── scanner.hpp     # Scanner class
│   └── token.hpp       # TokenType enum, Token struct, lookup declarations
├── src
│   ├── dfa.cpp         # character classes and the transition table
│   ├── scanner.cpp     # the scanning loop and line/column tracking
│   └── token.cpp       # token names, keyword and operator lookup tables
└── README.md
```
