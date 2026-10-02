"""
pytest configuration: the --compiler option and the compiler fixture.

Note: This file was written by Claude (Anthropic)
"""

import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_COMPILER = ROOT / "build" / ("octacc.exe" if sys.platform == "win32" else "octacc")


def pytest_addoption(parser):
    parser.addoption(
        "--compiler",
        default=None,
        help="path to the compiler binary, written as --compiler=PATH (default: build/octacc)",
    )


@pytest.fixture(scope="session")
def compiler(request):
    arg = request.config.getoption("--compiler")
    path = Path(arg).resolve() if arg else DEFAULT_COMPILER
    if not path.exists():
        pytest.exit(f"compiler not found at {path}. Run `make` first or pass --compiler.", returncode=2)
    return path

