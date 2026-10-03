import importlib.util
import pathlib
import sys
import unittest
from unittest import mock


def load_launcher(relative_path, name):
    path = pathlib.Path(__file__).parents[1] / relative_path / "bridge_launcher.py"
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


class BridgeLauncherTests(unittest.TestCase):
    def setUp(self):
        self.launchers = (
            load_launcher(pathlib.Path("fusion/ReplicazeronFusion"), "fusion_bridge_launcher_test"),
            load_launcher(pathlib.Path("freecad/ReplicazeronFreeCAD"), "freecad_bridge_launcher_test"),
        )

    def test_non_windows_reports_that_auto_start_is_unavailable(self):
        for launcher in self.launchers:
            with self.subTest(launcher=launcher.__name__), mock.patch.object(launcher.sys, "platform", "linux"):
                started, message = launcher.ensure_bridge_started("/tmp/addin")
                self.assertFalse(started)
                self.assertIn("Windows only", message)

    def test_packaged_executable_is_started_hidden(self):
        for launcher in self.launchers:
            launcher._started = False
            with self.subTest(launcher=launcher.__name__), \
                 mock.patch.object(launcher.sys, "platform", "win32"), \
                 mock.patch.object(launcher.os.path, "isfile", side_effect=lambda path: path.endswith(launcher.EXECUTABLE_NAME)), \
                 mock.patch.object(launcher.subprocess, "CREATE_NO_WINDOW", 0x08000000, create=True), \
                 mock.patch.object(launcher.subprocess, "DETACHED_PROCESS", 0x00000008, create=True), \
                 mock.patch.object(launcher.subprocess, "Popen") as popen:
                started, _message = launcher.ensure_bridge_started("C:/Replicazeron")
                self.assertTrue(started)
                command = popen.call_args.args[0]
                expected = launcher.os.path.join("C:/Replicazeron", launcher.EXECUTABLE_NAME)
                self.assertEqual(command, [expected, "--parent-pid", str(launcher.os.getpid())])
                self.assertEqual(popen.call_args.kwargs["creationflags"], 0x08000008)
