<div align="center">
  <img alt="The OctaC logo" src="assets/logo.svg" width="25%">

A statically typed, C-structured language for numerical and matrix computation.

[Documentation] | [Getting started] | [Lexer] | [Contributing]
</div>

This is the main source code repository for OctaC. It contains the compiler and
its test suite.

[Documentation]: https://github.com/Octa-C/octac-docs
[Getting started]: #quick-start
[Lexer]: lexer/README.md
[Contributing]: CONTRIBUTING.md

## Features

- **C-like structure:** functions with `-> type` return types, braces and
  semicolons, `//` line comments and compound assignment (`+=`, `-=`, `*=`, `/=`).

- **Sized scalar types:** `i16`, `i32`, `i64`, `f16`, `f32`, `f64` and `bool`,
  plus `string`.

- **Built-in `vector` and `matrix` types** with matrix operators: transpose
  (`'`), element-wise multiply (`.*`) and divide (`./`).

- **Rich control flow:** `if` / `elseif` / `else`, `caseof` / `case` /
  `default`, and three loops: `for`, `do ... until` and `until`, with `break`
  and `continue`.

- **Word-based logic:** `and`, `or`, `not`, `is` and `in` as keywords.

- **Static typing:** types are declared and checked at compile time.

## Status

| Stage | State |
|---|---|
| Lexical analysis | Done ([lexer/](lexer/README.md)) |
| Parsing | Not started |
| Semantic analysis | Not started |
| Code generation | Not started |

## Quick Start

Requires `g++`, `make` and Python 3.

```sh
make
build/lexer tests/lexer/cases/sample.oc
```

Each token is printed as `<TYPE, value>`, one per line, ending with
`<COMPILER_EOF,>`. See the [lexer README](lexer/README.md) for the output
format, error reporting and how the DFA works.

## Building and Testing

```sh
make            # builds build/lexer
make test       # builds, then runs the pytest suite
make clean      # removes build/
```

The tests need pytest. Install it into a virtualenv in the repository root,
which `make test` picks up automatically:

```sh
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
```

## Repository Layout

```
.
├── .github/
├── assets/
├── lexer
│   ├── include
│   │   ├── dfa.hpp                   # CharClass, State, Action enums and table declarations
│   │   ├── scanner.hpp               # Scanner class
│   │   └── token.hpp                 # TokenType enum, Token struct, lookup declarations
│   ├── src
│   │   ├── dfa.cpp                   # character classes, TRANSITION and ACTION tables
│   │   ├── scanner.cpp               # the scanning loop and line/column tracking
│   │   └── token.cpp                 # token names, keyword and operator lookup tables
│   ├── main.cpp                      # command line driver and --dump-table
│   └── README.md                     # lexer usage, output format and DFA design
├── specs
│   └── tokens.txt                    # the token set, source of truth for the lexer
├── tests
│   └── lexer/
├── CONTRIBUTING.md                   # how to contribute
├── Makefile                          # builds build/lexer, runs the tests
├── README.md
└── requirements.txt                  # Python packages for the tests
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).
