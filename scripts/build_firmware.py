"""Build Keyball firmware into the repository's build directory."""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
QMK_IMAGE = (
    "ghcr.io/qmk/qmk_cli@sha256:"
    "2dc05fc9f32efebd6b05c2b8676ee548358bc7e151e9dbf4dac6b6eed4513b07"
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--qmk-home",
        type=Path,
        default=os.environ.get("QMK_HOME") or REPO_ROOT / "build" / "qmk",
        help="Existing QMK 0.22.14 checkout (default: QMK_HOME or build/qmk)",
    )
    parser.add_argument("--keyboard", default="keyball39")
    parser.add_argument("--keymap", default="emacs")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument(
        "--docker", action="store_true", help="Use the locally installed CI image"
    )
    args = parser.parse_args()

    if args.jobs < 1:
        parser.error("--jobs must be positive")
    for name in (args.keyboard, args.keymap):
        if not re.fullmatch(r"[A-Za-z0-9_-]+", name):
            parser.error(f"Expected a single keyboard/keymap directory name: {name}")

    qmk_home = args.qmk_home.resolve()
    if not (qmk_home / "Makefile").is_file() or not (
        qmk_home / "quantum" / "action.c"
    ).is_file():
        parser.error(
            f"Not a QMK checkout: {qmk_home}; prepare QMK 0.22.14 with its "
            "submodules, then set QMK_HOME or pass --qmk-home"
        )

    keyboard_dir = REPO_ROOT / "qmk_firmware" / "keyboards" / "keyball"
    if not (keyboard_dir / args.keyboard / "keymaps" / args.keymap).is_dir():
        parser.error(f"Keymap not found: {args.keyboard}:{args.keymap}")
    executable = shutil.which("docker" if args.docker else "qmk")
    if executable is None:
        parser.error(
            "docker is required"
            if args.docker
            else "qmk is required; use a QMK build environment or --docker"
        )
    if not args.docker:
        linked_keyboard = qmk_home / "keyboards" / "keyball"
        if linked_keyboard.resolve() != keyboard_dir.resolve():
            parser.error(f"Link {linked_keyboard} to {keyboard_dir} before building")

    build_dir = REPO_ROOT / "build"
    build_dir.mkdir(exist_ok=True)
    output_dir = "/out/.build" if args.docker else str(build_dir / ".build")
    compile_args = [
        "compile",
        "--clean",
        "-j",
        str(args.jobs),
        "-kb",
        f"keyball/{args.keyboard}",
        "-km",
        args.keymap,
        "-e",
        "SKIP_GIT=yes",
        "-e",
        f"BUILD_DIR={output_dir}",
    ]
    if args.docker:
        command = [
            executable,
            "run",
            "--rm",
            "--pull=never",
            "--mount",
            f"type=bind,source={qmk_home},target=/qmk",
            "--mount",
            f"type=bind,source={keyboard_dir},target=/qmk/keyboards/keyball,readonly",
            "--mount",
            f"type=bind,source={build_dir},target=/out",
            "--workdir",
            "/qmk",
            "--env",
            "QMK_HOME=/qmk",
            "--entrypoint",
            "qmk",
            QMK_IMAGE,
            *compile_args,
        ]
    else:
        command = [executable, *compile_args]
    print(f"Building {args.keyboard}:{args.keymap} into {build_dir}", flush=True)
    environment = dict(os.environ, QMK_HOME=str(qmk_home))
    result = subprocess.run(command, cwd=qmk_home, env=environment, check=False)
    if result.returncode:
        return result.returncode

    filename = f"keyball_{args.keyboard}_{args.keymap}.hex"
    source = qmk_home / filename
    destination = build_dir / filename
    if not source.is_file():
        parser.error(f"QMK completed without the expected firmware: {source}")
    # QMK also copies the HEX to its checkout root; keep that copy in build/.
    if source.resolve() != destination.resolve():
        shutil.move(str(source), str(destination))
    print(f"Firmware: {destination}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
