import importlib.util
import pathlib
import sys
import types
import unittest
from unittest import mock


MODULE_PATH = pathlib.Path(__file__).parents[1] / "bridge.py"
SPEC = importlib.util.spec_from_file_location("replicazeron_cad_bridge", MODULE_PATH)
BRIDGE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = BRIDGE
SPEC.loader.exec_module(BRIDGE)


def report(angle=0, distance=512, deadzone=100, active=True, pan=False, rotate=False, version=2):
    data = bytearray(32)
    data[0] = BRIDGE.COMMAND
    data[1] = BRIDGE.CAD_STICK_GET
    data[3] = version
    data[4] = int(active)
    data[5:7] = angle.to_bytes(2, "big")
    data[7:9] = distance.to_bytes(2, "big")
    data[9:11] = deadzone.to_bytes(2, "big")
    data[11] = int(pan)
    data[12] = int(rotate)
    return data


class DecodeReportTests(unittest.TestCase):
    def test_full_up(self):
        sample = BRIDGE.decode_report(report(angle=0), 7, 1.0)
        self.assertTrue(sample.active)
        self.assertAlmostEqual(sample.x, 0.0)
        self.assertAlmostEqual(sample.y, 1.0)
        self.assertAlmostEqual(sample.strength, 1.0)

    def test_full_left(self):
        sample = BRIDGE.decode_report(report(angle=90), 8, 2.0)
        self.assertAlmostEqual(sample.x, -1.0)
        self.assertAlmostEqual(sample.y, 0.0, places=7)

    def test_pan_mode(self):
        self.assertTrue(BRIDGE.decode_report(report(pan=True), 9, 3.0).pan)
        self.assertFalse(BRIDGE.decode_report(report(), 10, 4.0).pan)

    def test_rotate_mode(self):
        self.assertTrue(BRIDGE.decode_report(report(rotate=True), 9, 3.0).rotate)
        self.assertFalse(BRIDGE.decode_report(report(), 10, 4.0).rotate)

    def test_format_one_remains_compatible(self):
        sample = BRIDGE.decode_report(report(pan=True, rotate=True, version=1), 9, 3.0)
        self.assertTrue(sample.pan)
        self.assertFalse(sample.rotate)

    def test_deadzone_and_inactive_reports_are_zero(self):
        centered = BRIDGE.decode_report(report(distance=100), 9, 3.0)
        inactive = BRIDGE.decode_report(report(active=False), 10, 4.0)
        self.assertEqual((centered.x, centered.y, centered.strength), (0.0, 0.0, 0.0))
        self.assertEqual((inactive.x, inactive.y, inactive.strength), (0.0, 0.0, 0.0))

    def test_rejects_wrong_operation(self):
        data = report()
        data[1] = 0
        with self.assertRaises(ValueError):
            BRIDGE.decode_report(data, 0, 0.0)

    def test_packaged_self_test_does_not_require_a_device(self):
        with mock.patch.dict(sys.modules, {"hid": types.ModuleType("hid")}):
            BRIDGE.self_test()

    def test_single_instance_is_unrestricted_off_windows(self):
        with mock.patch.object(BRIDGE.sys, "platform", "linux"):
            self.assertTrue(BRIDGE.acquire_single_instance())

    def test_parent_registry_keeps_bridge_alive_until_last_host_closes(self):
        running = {101, 202}
        parents = BRIDGE.ParentRegistry(lambda pid: pid in running)
        self.assertTrue(parents.add(101))
        self.assertTrue(parents.add(202))
        self.assertTrue(parents.configured)
        self.assertTrue(parents.any_running())

        running.remove(101)
        self.assertTrue(parents.any_running())
        running.remove(202)
        self.assertFalse(parents.any_running())

    def test_parent_registry_rejects_invalid_pid(self):
        parents = BRIDGE.ParentRegistry(lambda _pid: True)
        self.assertFalse(parents.add(0))
        self.assertFalse(parents.configured)
        self.assertFalse(parents.any_running())

    def test_vial_window_detection_uses_title_or_executable(self):
        self.assertTrue(BRIDGE.is_vial_window("Vial", ""))
        self.assertTrue(BRIDGE.is_vial_window("Keyboard settings", "C:/Tools/Vial.exe"))
        self.assertTrue(BRIDGE.is_vial_window("Replicazeron - Vial", "C:/Browser/chrome.exe"))
        self.assertFalse(BRIDGE.is_vial_window("FreeCAD", "C:/Tools/FreeCAD.exe"))
        self.assertFalse(BRIDGE.is_vial_window("Trivial notes", "C:/Tools/notes.exe"))


if __name__ == "__main__":
    unittest.main()
