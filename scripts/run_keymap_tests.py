"""Run the Keyball39 emacs regression tests in an existing QMK checkout."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
TEST_NAME = "keyball39_emacs_mark"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--qmk-home",
        type=Path,
        default=os.environ.get("QMK_HOME"),
        help="QMK 0.22.14 checkout with its test dependencies (or set QMK_HOME)",
    )
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--filter", help="Optional GoogleTest filter")
    args = parser.parse_args()

    if args.qmk_home is None:
        parser.error("--qmk-home or QMK_HOME is required")
    if args.jobs < 1:
        parser.error("--jobs must be positive")

    qmk_home = args.qmk_home.resolve()
    if (
        not (qmk_home / "Makefile").is_file()
        or not (qmk_home / "quantum" / "action.c").is_file()
    ):
        parser.error(f"Not a QMK firmware checkout: {qmk_home}")
    if not (qmk_home / "tests").is_dir():
        parser.error(f"QMK test infrastructure is missing: {qmk_home / 'tests'}")
    make = shutil.which("make")
    if make is None:
        parser.error("make is required; run inside the QMK build environment")

    source = REPO_ROOT / "tests" / TEST_NAME
    if not source.is_dir():
        parser.error(f"Regression tests are missing: {source}")
    target = qmk_home / "tests" / TEST_NAME
    if target.exists() or target.is_symlink():
        parser.error(f"Refusing to overwrite an existing test directory: {target}")

    environment = os.environ.copy()
    if args.filter:
        environment["GTEST_FILTER"] = args.filter
    keyboard_dir = REPO_ROOT / "qmk_firmware" / "keyboards" / "keyball"
    command = [
        make,
        f"-j{args.jobs}",
        f"test:{TEST_NAME}",
        f"KEYBALL_DIR={keyboard_dir}",
    ]

    target.mkdir()
    try:
        shutil.copytree(source, target, dirs_exist_ok=True)
        print(f"Running {TEST_NAME} in {qmk_home}", flush=True)
        return subprocess.run(
            command, cwd=qmk_home, env=environment, check=False
        ).returncode
    finally:
        shutil.rmtree(target)


if __name__ == "__main__":
    raise SystemExit(main())
