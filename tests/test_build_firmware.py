import contextlib
import io
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from scripts import build_firmware


class BuildFirmwareTest(unittest.TestCase):
    def setUp(self):
        build_dir = build_firmware.REPO_ROOT / "build"
        build_dir.mkdir(exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="build-test-", dir=build_dir)
        self.addCleanup(self.temp.cleanup)
        self.repo = Path(self.temp.name)
        self.qmk = self.repo / "build" / "qmk"
        self.make_qmk(self.qmk)
        keyboard_dir = self.repo / "qmk_firmware" / "keyboards" / "keyball"
        (keyboard_dir / "keyball39" / "keymaps" / "emacs").mkdir(parents=True)
        self.filename = "keyball_keyball39_emacs.hex"
        self.output = self.repo / "build" / self.filename
        self.stdout = io.StringIO()
        self.returncode = 0
        self.emit_firmware = True
        self.start_patch(patch.object(build_firmware, "REPO_ROOT", self.repo))
        self.start_patch(patch.dict(os.environ, {}, clear=True))
        self.start_patch(
            patch.object(build_firmware.shutil, "which", side_effect=lambda name: name)
        )
        self.run_mock = self.start_patch(
            patch.object(build_firmware.subprocess, "run", side_effect=self.compile)
        )

    def start_patch(self, patcher):
        result = patcher.start()
        self.addCleanup(patcher.stop)
        return result

    def make_qmk(self, path):
        (path / "quantum").mkdir(parents=True)
        (path / "Makefile").touch()
        (path / "quantum" / "action.c").touch()

    def compile(self, command, *, cwd, env, check):
        if not self.returncode and self.emit_firmware:
            (cwd / self.filename).write_text("new firmware", encoding="ascii")
        return subprocess.CompletedProcess(command, self.returncode)

    def run_build(self, *args):
        with (
            patch("sys.argv", ["build_firmware.py", *args]),
            contextlib.redirect_stdout(self.stdout),
            contextlib.redirect_stderr(io.StringIO()),
        ):
            return build_firmware.main()

    def test_docker_uses_local_image_and_publishes_under_build(self):
        self.assertEqual(self.run_build("--docker"), 0)
        command = self.run_mock.call_args.args[0]
        self.assertIn("--pull=never", command)
        self.assertIn(build_firmware.QMK_IMAGE, command)
        self.assertIn("--clean", command)
        self.assertIn("BUILD_DIR=/out/.build", command)
        self.assertIn(f"type=bind,source={self.repo / 'build'},target=/out", command)
        self.assertEqual(self.output.read_text(), "new firmware")
        self.assertFalse((self.qmk / self.filename).exists())

    def test_native_build_has_the_same_output_contract(self):
        qmk = self.repo / "qmk_firmware"
        self.make_qmk(qmk)
        with patch.dict(os.environ, {"QMK_HOME": str(self.repo / "unused-qmk")}):
            self.assertEqual(self.run_build("--qmk-home", str(qmk)), 0)
        command = self.run_mock.call_args.args[0]
        self.assertEqual(command[0], "qmk")
        self.assertEqual(self.run_mock.call_args.kwargs["env"]["QMK_HOME"], str(qmk))
        self.assertIn(f"BUILD_DIR={self.repo / 'build' / '.build'}", command)
        self.assertTrue(self.output.is_file())
        self.assertFalse((qmk / self.filename).exists())

    def test_build_failure_preserves_last_successful_firmware(self):
        self.output.write_text("previous firmware", encoding="ascii")
        self.returncode = 7
        self.assertEqual(self.run_build("--docker"), 7)
        self.assertEqual(self.output.read_text(), "previous firmware")
        self.assertNotIn("Firmware:", self.stdout.getvalue())

    def test_missing_output_is_an_error(self):
        self.emit_firmware = False
        with self.assertRaises(SystemExit) as error:
            self.run_build("--docker")
        self.assertEqual(error.exception.code, 2)
        self.assertFalse(self.output.exists())

    def test_invalid_arguments_do_not_start_a_build(self):
        for args in (
            ("--jobs", "0"),
            ("--keyboard", "../keyball39"),
            ("--keymap", "missing"),
            ("--qmk-home", str(self.repo / "missing")),
        ):
            with self.subTest(args=args), self.assertRaises(SystemExit):
                self.run_build("--docker", *args)
        self.run_mock.assert_not_called()

    def test_qmk_home_environment_is_respected(self):
        qmk = self.repo / "external-qmk"
        self.make_qmk(qmk)
        with patch.dict(os.environ, {"QMK_HOME": str(qmk)}):
            self.assertEqual(self.run_build("--docker"), 0)
        self.assertEqual(self.run_mock.call_args.kwargs["cwd"], qmk)


if __name__ == "__main__":
    unittest.main()
