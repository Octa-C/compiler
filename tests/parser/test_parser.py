"""
Run the compiler on every tests/parser/cases/<name>.oc and compare with tests/parser/expected/.

Each case is one test. It checks the parse tree printed by --emit-parse-tree against <name>.tree
(none expected if that file is absent), the diagnostics on stderr against <name>.stderr (none
expected if that file is absent) and the exit code, which should be 1 when diagnostics are
expected and 0 otherwise.
"""

import difflib
import subprocess
from pathlib import Path

import pytest

PARSER_DIR = Path(__file__).resolve().parent
CASES_DIR = PARSER_DIR / "cases"
EXPECTED_DIR = PARSER_DIR / "expected"
CASES = sorted(CASES_DIR.glob("*.oc"))


def read_lines(path):
    return path.read_text(encoding="utf-8").splitlines() if path.exists() else []


def first_difference(expected, actual):
    """
    Return a description of the first line where the two lists differ, or None.
    """
    for number, (want, got) in enumerate(zip(expected, actual), start=1):
        if want != got:
            return f"line {number}: expected {want!r}, got {got!r}"
    if len(expected) != len(actual):
        return f"expected {len(expected)} lines, got {len(actual)}"
    return None


def all_differences(expected, actual):
    return "\n".join(difflib.unified_diff(expected, actual, "expected", "actual", lineterm=""))


@pytest.mark.parametrize("case", CASES, ids=[p.stem for p in CASES])
def test_parser(case, compiler, request):
    verbose = request.config.getoption("verbose") > 0
    tree_file = EXPECTED_DIR / f"{case.stem}.tree"
    stderr_file = EXPECTED_DIR / "diagnostics" / f"{case.stem}.stderr"
    exp_tree = read_lines(tree_file)
    exp_stderr = read_lines(stderr_file)

    proc = subprocess.run(
        [str(compiler), "--emit-parse-tree", case.name],
        cwd=case.parent,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    problems = []
    want_code = 1 if exp_stderr else 0
    if proc.returncode != want_code:
        problems.append(f"exit status: expected {want_code}, exited with {proc.returncode}")
    for name, expected, actual in (
        (tree_file.name, exp_tree, proc.stdout.splitlines()),
        (stderr_file.name, exp_stderr, proc.stderr.splitlines()),
    ):
        diff = all_differences(expected, actual) if verbose else first_difference(expected, actual)
        if diff:
            problems.append(f"{name} differs:\n{diff}")
    if problems:
        pytest.fail("\n".join(problems), pytrace=False)
