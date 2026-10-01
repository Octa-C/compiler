# Octa C

OctaC is a statically typed, C-structured language for numerical and matrix computation. This repository holds the OctaC compiler, built as part of a Compiler Construction course. The language design is documented separately in  [octac-docs](https://github.com/Octa-C/octac-docs).

---

## Status

| Stage | State |
|---|---|
| Lexical analysis | Done (`lexer/`) |
| Parsing | Not started |
| Semantic analysis | Not started |
| Code generation | Not started |

---

## Repository layout

- `lexer/`: the lexical analyser (C++17). See [lexer/README.md](lexer/README.md) for usage, output format and how the DFA works.
- `specs/tokens.txt`: the token set. This is the source of truth for token names and lexemes, and the lexer follows it.
- `tests/lexer/`: pytest suite for the lexer (`cases/` inputs, `expected/` token streams and diagnostics).
- `Makefile`: builds the compiler binaries into `build/` and runs the tests.
- `requirements.txt`: Python packages needed by the tests.

---

## Build and test

Requires `g++`, `make` and Python 3.

```sh
make            # builds build/lexer
make test       # builds, then runs the pytest suite
make clean      # removes build/
```

The tests need pytest. Install it into a virtualenv in the repository root, which `make test` picks up automatically:

```sh
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
```

---

## Quick start

```sh
make
build/lexer tests/lexer/cases/sample.oc
```

Each token is printed as `<TYPE, value>`, one per line, ending with `<COMPILER_EOF,>`.

---

## Credits

Prepared by Syed Taha and Muhammad Usman as part of a Compiler Construction course, under the supervision of Miss Sadaf Alvi.
