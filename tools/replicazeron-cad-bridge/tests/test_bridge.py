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


def report(angle=0, distance=512, deadzone=100, active=True):
    data = bytearray(32)
    data[0] = BRIDGE.COMMAND
    data[1] = BRIDGE.CAD_STICK_GET
    data[3] = 1
    data[4] = int(active)
    data[5:7] = angle.to_bytes(2, "big")
    data[7:9] = distance.to_bytes(2, "big")
    data[9:11] = deadzone.to_bytes(2, "big")
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


if __name__ == "__main__":
    unittest.main()
