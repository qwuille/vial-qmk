"""FreeCAD camera adapter for the local Replicazeron CAD bridge."""

from __future__ import annotations

import json
import os
import socket
import time

import FreeCAD as App
import FreeCADGui as Gui

try:
    from PySide import QtCore
except ImportError:
    from PySide2 import QtCore

UDP_PORT = 28462
_controller = None


def _load_settings():
    values = {
        "rotationDegreesPerSecond": 90.0,
        "invertHorizontal": False,
        "invertVertical": False,
    }
    path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "settings.json")
    try:
        with open(path, "r", encoding="utf-8") as stream:
            values.update(json.load(stream))
    except (OSError, ValueError):
        pass
    return values


class CadController:
    def __init__(self):
        self.settings = _load_settings()
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(("127.0.0.1", UDP_PORT))
        self.socket.setblocking(False)
        self.timer = QtCore.QTimer()
        self.timer.setInterval(16)
        self.timer.timeout.connect(self._tick)
        self.last_update = None

    def start(self):
        self.timer.start()

    def stop(self):
        self.timer.stop()
        self.socket.close()

    def _latest_sample(self):
        latest = None
        while True:
            try:
                payload, _source = self.socket.recvfrom(2048)
                sample = json.loads(payload.decode("utf-8"))
                if sample.get("version") == 1:
                    latest = sample
            except BlockingIOError:
                return latest
            except (ValueError, UnicodeDecodeError):
                continue

    def _tick(self):
        sample = self._latest_sample()
        now = time.monotonic()
        main_window = Gui.getMainWindow()
        if sample is None:
            return
        if not sample.get("active") or not main_window.isActiveWindow() or Gui.ActiveDocument is None:
            self.last_update = None
            return
        if self.last_update is None:
            self.last_update = now
            return

        delta_time = min(0.05, max(0.0, now - self.last_update))
        self.last_update = now
        horizontal = float(sample.get("x", 0.0))
        vertical = float(sample.get("y", 0.0))
        if self.settings["invertHorizontal"]:
            horizontal = -horizontal
        if self.settings["invertVertical"]:
            vertical = -vertical
        if abs(horizontal) + abs(vertical) < 1e-5:
            return

        rate = float(self.settings["rotationDegreesPerSecond"])
        yaw = horizontal * rate * delta_time
        pitch = -vertical * rate * delta_time
        view = Gui.ActiveDocument.ActiveView
        camera = view.getCameraNode()
        quaternion = camera.orientation.getValue().getValue()
        rotation = App.Rotation(*quaternion)
        position_values = camera.position.getValue().getValue()
        position = App.Vector(*position_values)
        focal_distance = float(camera.focalDistance.getValue())
        direction = rotation.multVec(App.Vector(0, 0, -1))
        target = position + direction * focal_distance

        up = rotation.multVec(App.Vector(0, 1, 0))
        rotation = App.Rotation(up, yaw).multiply(rotation)
        right = rotation.multVec(App.Vector(1, 0, 0))
        rotation = App.Rotation(right, pitch).multiply(rotation)
        direction = rotation.multVec(App.Vector(0, 0, -1))
        position = target - direction * focal_distance

        camera.orientation.setValue(*rotation.Q)
        camera.position.setValue(position.x, position.y, position.z)
        Gui.updateGui()


def start():
    global _controller
    if _controller is None:
        _controller = CadController()
        _controller.start()


def stop():
    global _controller
    if _controller is not None:
        _controller.stop()
        _controller = None


def toggle():
    stop() if is_running() else start()


def is_running():
    return _controller is not None
