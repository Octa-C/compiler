# OctaC Parser

The syntax analyser for the OctaC language. It reads the token stream produced by
the [lexer](../lexer/README.md) and builds a parse tree according to the LL(1)
grammar of the language.

## Usage

The parser is built into the compiler driver, which the Makefile in the repository
root builds as `build/octacc`. The `--emit-parse-tree` flag stops the compiler
after parsing and prints the parse tree.

```sh
build/octacc --emit-parse-tree <file.oc>              # print the parse tree
build/octacc --emit-parse-tree <file.oc> -o <out>     # write the parse tree to <out>
build/octacc <file.oc>                                # parse the file and report errors only
```

`--emit-parse-tree` cannot be combined with `--emit-tokens`. Parsing starts only
when the lexer found no errors.

### Output

The tree is printed one node per line, indented two spaces per level. A nonterminal
is written as its name in the grammar, and a terminal as its token in the form the
lexer prints. A nonterminal that derived the empty string is marked `(empty)`:

```
Program
  <FUNCTION,>
  <IDENTIFIER, main>
  <LPAREN,>
  Params (empty)
  <RPAREN,>
```

The tree mirrors the LL(1) grammar exactly, with one node per nonterminal and one child
per symbol of the production. A nonterminal with primes in the grammar is named with `P`
instead, followed by the number of primes from two upwards: `Stmt'` is `StmtP`,
`Stmt''` is `StmtP2` and `Stmt'''''` is `StmtP5`.

### Drawing the tree

`parser/tools/visualize_parse_tree.py` draws the tree as a diagram. It uses only the Python
standard library. The input is a `.oc` file, which it compiles with `build/octacc`, a
file with the tree text, or `-` for the tree text on standard input.

```sh
python3 parser/tools/visualize_parse_tree.py <file.oc>                    # writes <file>.html
python3 parser/tools/visualize_parse_tree.py <file.oc> --compact          # shorter tree
python3 parser/tools/visualize_parse_tree.py <file.oc> -o <tree.svg>      # a plain SVG
python3 parser/tools/visualize_parse_tree.py <file.oc> --layout horizontal  # left to right
```

The tree is drawn from top to bottom by default. `--layout horizontal` draws it from left to
right, which takes less space for large trees.
An `.html` output is interactive. A click on a node with children collapses or expands
it, and a collapsed node shows how many nodes it hides, for example `Stmts +64`. The
buttons at the top expand all nodes, collapse all nodes, fit the tree to the window and
switch between the two layouts. The mouse pans and the wheel zooms. `--expand N` sets how
many levels below the root are open when the page loads, 3 by default, and 0 opens all of
them. An `.svg` output is a static drawing of the whole tree. Nonterminals are drawn blue,
tokens green, and empty nonterminals grey and dashed. `--compact` leaves out the
empty nodes and merges each chain of single-child nonterminals into one node labelled
`Top > Bottom`, with the whole chain in its tooltip. `--hide-empty` only leaves out the
empty nodes.

### Errors

A syntax error is written to stderr in the same form as a lexical error, naming the
token the grammar allows and the token found:

```
main.oc:2:13: error: expected an expression but found ';'
 2 |     i32 x = ;
   |             ^
```

After an error the parser skips to a point where parsing can continue, so one run
reports many errors. Inside a block it skips to the end of the statement. Elsewhere
it skips to the next `function`. No tree is produced when there was an error.

Exit codes are the ones of the compiler driver: `0` for no errors, `1` when the file
had lexical or syntax errors, `2` for a usage error or an unreadable file.

## Testing

The parser tests live in `tests/parser` and run with `make test` from the repository
root. Each `tests/parser/cases/<name>.oc` is run through `octacc --emit-parse-tree`
as one test, which checks:

- the tree against `tests/parser/expected/<name>.tree`. When that file does not
  exist, no tree is expected.
- the diagnostics printed to stderr against
  `tests/parser/expected/diagnostics/<name>.stderr`. When that file does not exist,
  no diagnostics are expected.
- the exit code, which should be `1` when diagnostics are expected and `0` otherwise.

## How it works

The parser is a recursive descent parser. The grammar is LL(1), so one token of
lookahead picks every production.

1. **One function per nonterminal.** `Parser` has a private function for each
   nonterminal in the grammar, named after it (`stmts`, `exprP`, `stmtP3`, ...). Each
   one returns a `ParseNode`. Two helpers add symbols that several productions share:
   `function` for `FUNCTION ID ( Params ) -> Type { Stmts }` and `bracedStmts` for
   `{ Stmts }`.
2. **Lookahead.** A function looks at the next token with `peek` and `check`, and
   chooses the production from it. An empty production is taken when the next token
   is not the start of the non-empty one.
3. **Matching terminals.** `expect` consumes a token of the required type and adds
   it to the node as a leaf. `shift` does the same for the token already known to
   be there.
4. **Errors and recovery.** `expect` and the failing branches of a function throw
   `SyntaxError` after reporting. `stmtsP` catches it and skips the rest of the
   statement, and `program` and `programP` catch it and skip to the next function.
   Brace depth is tracked, so skipping a broken statement leaves the enclosing
   block intact. Two errors at the same token are reported once.

## Layout

```
parser
├── include
│   ├── parse_tree.hpp            # NonTerminal enum, ParseNode struct, tree printer
│   └── parser.hpp                # Parser class
├── src
│   ├── parse_tree.cpp            # nonterminal names and the tree printer
│   └── parser.cpp                # one function per nonterminal, error recovery
├── tools
│   ├── js
│   │   ├── parse_tree_layout.js  # tree layout used by the HTML page
│   │   └── parse_tree_view.js    # expand, collapse, pan and zoom in the HTML page
│   └── visualize_parse_tree.py   # draws a parse tree as SVG or HTML
└── README.md
```
