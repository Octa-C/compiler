"""
Helpers for the pytest suite in test_lexer.py.

Layout:
    tests/cases/<name>.oc                         lexer input
    tests/expected/<name>.toks                    expected token stream, one <TYPE, value> per line
    tests/expected/diagnostics/<name>.stderr      expected diagnostics (optional, empty when absent)

Note: This file was written by Claude (Anthropic)
"""

import difflib
import re
import subprocess
import sys
from collections import namedtuple
from itertools import zip_longest
from pathlib import Path

TESTS_DIR = Path(__file__).resolve().parent
ROOT = TESTS_DIR.parents[1]
CASES_DIR = TESTS_DIR / "cases"
EXPECTED_DIR = TESTS_DIR / "expected"
DEFAULT_LEXER = ROOT / "build" / ("lexer.exe" if sys.platform == "win32" else "lexer")

# line is the 1-based line the token was read from
Token = namedtuple("Token", "type value line")

# kind is value | type | missing | extra. at is the line in the expected file.
Diff = namedtuple("Diff", "kind expected actual at")

# <TYPE, value> or <TYPE,> for a token without a value part
TOKEN_LINE = re.compile(r"^<([A-Z][A-Z0-9_]*),\s?(.*)>$")


def parse_tokens(lines):
    """
    Return (tokens, malformed) where malformed is a list of (line, text).
    """
    tokens, malformed = [], []
    for number, text in enumerate(lines, start=1):
        if not text.strip():
            continue
        match = TOKEN_LINE.match(text.strip())
        if match:
            tokens.append(Token(match.group(1), match.group(2), number))
        else:
            malformed.append((number, text))
    return tokens, malformed


def format_token(token):
    if token.value == "":
        return f"<{token.type},>"
    return f"<{token.type}, {token.value}>"


def run_lexer(lexer, case_path):
    """
    Run the lexer on a case from inside its directory.

    The file is passed by bare name so diagnostics read "name.oc:line:col: ...".
    Returns (exit code, stdout lines, stderr lines).
    """
    proc = subprocess.run(
        [str(lexer), case_path.name],
        cwd=case_path.parent,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return proc.returncode, proc.stdout.splitlines(), proc.stderr.splitlines()


def expected_tokens_path(name):
    return EXPECTED_DIR / f"{name}.toks"


def expected_stderr_path(name):
    return EXPECTED_DIR / "diagnostics" / f"{name}.stderr"


def read_lines(path):
    return path.read_text(encoding="utf-8").splitlines()


def compare_tokens(expected, actual):
    """
    Align the two streams on token type, then classify every difference.
    """
    diffs = []
    matcher = difflib.SequenceMatcher(
        None, [t.type for t in expected], [t.type for t in actual], autojunk=False
    )
    for op, i1, i2, j1, j2 in matcher.get_opcodes():
        if op == "equal":
            for e, a in zip(expected[i1:i2], actual[j1:j2]):
                if e.value != a.value:
                    diffs.append(Diff("value", e, a, e.line))
            continue
        # tokens present on both sides of a replaced block are wrong-type pairs,
        # the leftovers are missing or extra
        paired = min(i2 - i1, j2 - j1) if op == "replace" else 0
        for k in range(paired):
            diffs.append(Diff("type", expected[i1 + k], actual[j1 + k], expected[i1 + k].line))
        for e in expected[i1 + paired : i2]:
            diffs.append(Diff("missing", e, None, e.line))
        after = expected[i1].line if i1 < len(expected) else (expected[-1].line + 1 if expected else 1)
        for a in actual[j1 + paired : j2]:
            diffs.append(Diff("extra", None, a, after))
    return diffs


def describe(diff):
    want = format_token(diff.expected) if diff.expected else "(nothing)"
    got = format_token(diff.actual) if diff.actual else "(nothing)"
    return f"Expected: {want}, Got {got}"


def compare_lines(expected, actual):
    """
    Return the differing lines as (line number, expected line, got line).
    A missing line is None.
    """
    matcher = difflib.SequenceMatcher(None, expected, actual, autojunk=False)
    rows = []
    for op, i1, i2, j1, j2 in matcher.get_opcodes():
        if op == "equal":
            continue
        for k, (want, got) in enumerate(zip_longest(expected[i1:i2], actual[j1:j2])):
            rows.append((i1 + k + 1, want, got))
    return rows
