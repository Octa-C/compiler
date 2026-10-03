"""
Tests for the command line of the compiler driver: help, usage errors, options and output files.
"""

import subprocess

import pytest

SOURCE = "i32 x = 1;\n"
SOURCE_TOKENS = [
    "<SCALAR, i32>",
    "<IDENTIFIER, x>",
    "<ASSIGNMENT, =>",
    "<INT, 1>",
    "<SEMICOLON,>",
    "<COMPILER_EOF,>",
]


def run(compiler, *args, cwd=None):
    return subprocess.run(
        [str(compiler), *args], cwd=cwd, capture_output=True, text=True, encoding="utf-8"
    )


@pytest.fixture
def source(tmp_path):
    path = tmp_path / "prog.oc"
    path.write_text(SOURCE, encoding="utf-8")
    return path


def test_help_lists_every_argument(compiler):
    for flag in ("--help", "-h"):
        proc = run(compiler, flag)
        assert proc.returncode == 0
        assert proc.stdout.startswith("usage: octacc [options] <input>")
        for text in ("--emit-tokens", "--emit-parse-tree", "-o, --output <file>", "<input>"):
            assert text in proc.stdout


def test_emit_tokens_prints_the_token_stream(compiler, source):
    proc = run(compiler, "--emit-tokens", str(source))
    assert proc.returncode == 0
    assert proc.stdout.splitlines() == SOURCE_TOKENS
    assert proc.stderr == ""


def test_without_an_emit_flag_nothing_is_printed(compiler, tmp_path):
    path = tmp_path / "prog.oc"
    path.write_text("function main() -> i32 {\n    return 0;\n}\n", encoding="utf-8")
    proc = run(compiler, str(path))
    assert proc.returncode == 0
    assert proc.stdout == ""
    assert proc.stderr == ""


@pytest.mark.parametrize("spelling", ["-o", "--output"])
def test_output_option_writes_to_a_file(compiler, source, tmp_path, spelling):
    out = tmp_path / "tokens.txt"
    proc = run(compiler, "--emit-tokens", spelling, str(out), str(source))
    assert proc.returncode == 0
    assert proc.stdout == ""
    assert out.read_text(encoding="utf-8").splitlines() == SOURCE_TOKENS


def test_options_may_follow_the_input_and_use_equals(compiler, source, tmp_path):
    out = tmp_path / "tokens.txt"
    proc = run(compiler, str(source), "--emit-tokens", f"--output={out}")
    assert proc.returncode == 0
    assert out.read_text(encoding="utf-8").splitlines() == SOURCE_TOKENS


def test_double_dash_ends_option_parsing(compiler, tmp_path):
    path = tmp_path / "-odd.oc"
    path.write_text(SOURCE, encoding="utf-8")
    proc = run(compiler, "--emit-tokens", "--", "-odd.oc", cwd=tmp_path)
    assert proc.returncode == 0
    assert proc.stdout.splitlines() == SOURCE_TOKENS


@pytest.mark.parametrize(
    "args, message",
    [
        ([], "missing argument '<input>'"),
        (["--bogus", "x.oc"], "unknown option '--bogus'"),
        (["-z", "x.oc"], "unknown option '-z'"),
        (["a.oc", "b.oc"], "unexpected argument 'b.oc'"),
        (["x.oc", "-o"], "option '-o' requires a value"),
        (["--emit-tokens=yes", "x.oc"], "option '--emit-tokens' does not take a value"),
        (["-o", "a", "-o", "b", "x.oc"], "option '-o' given more than once"),
        (["-o", "a", "x.oc"], "option '--output' requires '--emit-tokens' or '--emit-parse-tree'"),
    ],
)
def test_usage_errors_exit_with_2(compiler, args, message):
    proc = run(compiler, *args)
    assert proc.returncode == 2
    assert message in proc.stderr
    assert proc.stdout == ""


def test_missing_input_file(compiler, tmp_path):
    proc = run(compiler, "--emit-tokens", str(tmp_path / "absent.oc"))
    assert proc.returncode == 2
    assert "cannot read" in proc.stderr


def test_unwritable_output_file(compiler, source, tmp_path):
    proc = run(compiler, "--emit-tokens", "-o", str(tmp_path / "no" / "dir" / "t"), str(source))
    assert proc.returncode == 2
    assert "cannot open" in proc.stderr


def test_lexical_errors_exit_with_1_and_still_emit_tokens(compiler, tmp_path):
    path = tmp_path / "bad.oc"
    path.write_text("i32 x = 1 @ 2;\n", encoding="utf-8")
    proc = run(compiler, "--emit-tokens", str(path))
    assert proc.returncode == 1
    assert "<COMPILER_ERROR,>" in proc.stdout
    assert "unrecognized lexeme '@'" in proc.stderr


def test_lexical_errors_exit_with_1_without_emit_tokens(compiler, tmp_path):
    path = tmp_path / "bad.oc"
    path.write_text("@\n", encoding="utf-8")
    proc = run(compiler, str(path))
    assert proc.returncode == 1
    assert proc.stdout == ""


def test_emit_parse_tree_prints_the_tree(compiler, tmp_path):
    path = tmp_path / "prog.oc"
    path.write_text("function main() -> i32 {\n    return 0;\n}\n", encoding="utf-8")
    proc = run(compiler, "--emit-parse-tree", str(path))
    assert proc.returncode == 0
    lines = proc.stdout.splitlines()
    assert lines[0] == "Program"
    assert lines[1] == "  <FUNCTION,>"
    assert "  <IDENTIFIER, main>" in lines
    assert "  ProgramP (empty)" in lines
    assert proc.stderr == ""


def test_emit_parse_tree_writes_to_a_file(compiler, tmp_path):
    path = tmp_path / "prog.oc"
    path.write_text("function main() -> i32 {\n}\n", encoding="utf-8")
    out = tmp_path / "tree.txt"
    proc = run(compiler, "--emit-parse-tree", "-o", str(out), str(path))
    assert proc.returncode == 0
    assert proc.stdout == ""
    assert out.read_text(encoding="utf-8").startswith("Program\n")


def test_syntax_errors_exit_with_1_and_print_no_tree(compiler, tmp_path):
    path = tmp_path / "bad.oc"
    path.write_text("function main() -> i32 {\n    i32 x = ;\n}\n", encoding="utf-8")
    proc = run(compiler, "--emit-parse-tree", str(path))
    assert proc.returncode == 1
    assert proc.stdout == ""
    assert "expected an expression but found ';'" in proc.stderr


def test_without_an_emit_flag_syntax_errors_still_exit_with_1(compiler, tmp_path):
    path = tmp_path / "bad.oc"
    path.write_text("function main() -> i32 {\n", encoding="utf-8")
    proc = run(compiler, str(path))
    assert proc.returncode == 1
    assert proc.stdout == ""


def test_lexical_errors_stop_before_parsing(compiler, tmp_path):
    path = tmp_path / "bad.oc"
    path.write_text("function main() -> i32 { @ }\n", encoding="utf-8")
    proc = run(compiler, "--emit-parse-tree", str(path))
    assert proc.returncode == 1
    assert "unrecognized lexeme" in proc.stderr
    assert "expected" not in proc.stderr


def test_emit_flags_cannot_be_combined(compiler, source):
    proc = run(compiler, "--emit-tokens", "--emit-parse-tree", str(source))
    assert proc.returncode == 2
    assert "cannot be combined" in proc.stderr
