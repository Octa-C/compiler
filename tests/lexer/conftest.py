"""
pytest configuration: the --lexer option and the lexer fixture.

Note: This file was written by Claude (Anthropic)
"""

from pathlib import Path

import pytest

from testlib import DEFAULT_LEXER


def pytest_addoption(parser):
    parser.addoption(
        "--lexer",
        default=None,
        help="path to the lexer binary, written as --lexer=PATH (default: build/lexer)",
    )


@pytest.fixture(scope="session")
def lexer(request):
    arg = request.config.getoption("--lexer")
    path = Path(arg).resolve() if arg else DEFAULT_LEXER
    if not path.exists():
        pytest.exit(f"lexer not found at {path}. Run `make` first or pass --lexer.", returncode=2)
    return path

