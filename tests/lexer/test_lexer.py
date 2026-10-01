"""
Run the lexer on every tests/cases/<name>.oc and compare with tests/expected/.

Each case is one test. It checks the token stream against <name>.toks and the
diagnostics on stderr against <name>.stderr (none expected if that file is
absent). A failure gives the line in the expected file, what was expected and
what the lexer produced. By default only the first error is shown, and with
pytest -v every error is listed.

Note: This file was written by Claude (Anthropic)
"""

from collections import Counter

import pytest

from testlib import (
    CASES_DIR,
    compare_lines,
    compare_tokens,
    describe,
    expected_stderr_path,
    expected_tokens_path,
    parse_tokens,
    read_lines,
    run_lexer,
)

CASES = sorted(CASES_DIR.glob("*.oc"))


def fail(problems, verbose):
    """
    Fail with the first error, or with every error when verbose.
    """
    heading, lines = problems[0]
    if not verbose:
        problems = [(heading, lines[:1])]
    text = []
    for heading, lines in problems:
        text.append(heading)
        for line in lines:
            text.append("    " + line)
    pytest.fail("\n".join(text), pytrace=False)


@pytest.mark.parametrize("case", CASES, ids=[p.stem for p in CASES])
def test_lexer(case, lexer, request):
    verbose = request.config.getoption("verbose") > 0
    tokens_file = expected_tokens_path(case.stem)
    stderr_file = expected_stderr_path(case.stem)
    if not tokens_file.exists():
        pytest.fail(f"no expected file: {tokens_file.name}", pytrace=False)

    exp_tokens, _ = parse_tokens(read_lines(tokens_file))
    exp_stderr = read_lines(stderr_file) if stderr_file.exists() else []
    code, out, err = run_lexer(lexer, case)
    act_tokens, act_malformed = parse_tokens(out)

    problems = []  # (heading, [lines])
    if code != 0:
        problems.append(("lexer exit status", [f"exited with {code}"]))
    if act_malformed:
        problems.append(
            ("lexer output that is not a token", [f"line {n}: {t!r}" for n, t in act_malformed])
        )

    diffs = compare_tokens(exp_tokens, act_tokens)
    if diffs:
        counts = Counter(d.kind for d in diffs)
        heading = (
            f"expected={len(exp_tokens)}, got={len(act_tokens)}, "
            f"value={counts['value']}, type={counts['type']}, "
            f"missing={counts['missing']}, extra={counts['extra']}"
        )
        problems.append((heading, [f"{tokens_file.name}:{d.at} {describe(d)}" for d in diffs]))

    rows = compare_lines(exp_stderr, err)
    if rows:
        lines = [
            f"{stderr_file.name}:{n} Expected: {want or '(nothing)'}, Got {got or '(nothing)'}"
            for n, want, got in rows
        ]
        problems.append((f"stderr differs from {stderr_file.name}", lines))

    if problems:
        fail(problems, verbose)
