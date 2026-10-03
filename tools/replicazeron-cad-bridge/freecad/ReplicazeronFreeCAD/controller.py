"""FreeCAD camera adapter for the local Replicazeron CAD bridge."""

from __future__ import annotations

import json
import math
import os
import re
import socket
import time

import FreeCAD as App
import FreeCADGui as Gui

try:
    from .bridge_launcher import ensure_bridge_started
except ImportError:
    from bridge_launcher import ensure_bridge_started

try:
    from PySide import QtCore
except ImportError:
    from PySide2 import QtCore

UDP_PORT = 28462
_controller = None
_NUMBER = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"


def _load_settings():
    values = {
        "rotationDegreesPerSecond": 90.0,
        "panScalePerSecond": 0.75,
        "invertHorizontal": False,
        "invertVertical": False,
        "togglePanMode": False,
        "toggleRotateMode": False,
        "startBridgeAutomatically": True,
    }
    path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "settings.json")
    try:
        with open(path, "r", encoding="utf-8") as stream:
            values.update(json.load(stream))
    except (OSError, ValueError):
        pass
    return values


def _camera_position_and_focal_distance(camera_text):
    position_match = re.search(
        r"(?m)^\s*position\s+(%s)\s+(%s)\s+(%s)\s*$" % (_NUMBER, _NUMBER, _NUMBER),
        camera_text,
    )
    focal_match = re.search(r"(?m)^\s*focalDistance\s+(%s)\s*$" % _NUMBER, camera_text)
    if position_match is None or focal_match is None:
        raise ValueError("FreeCAD camera text has no position or focalDistance")
    position = App.Vector(*(float(value) for value in position_match.groups()))
    return position, float(focal_match.group(1))


def _camera_with_position(camera_text, position):
    replacement = "position {:.12g} {:.12g} {:.12g}".format(position.x, position.y, position.z)
    updated, count = re.subn(
        r"(?m)^\s*position\s+%s\s+%s\s+%s\s*$" % (_NUMBER, _NUMBER, _NUMBER),
        replacement,
        camera_text,
        count=1,
    )
    if count != 1:
        raise ValueError("FreeCAD camera text has no position")
    return updated


def _set_camera_position(view, camera_text, position):
    view.setCamera(_camera_with_position(camera_text, position))


def _camera_with_orientation(camera_text, rotation):
    x, y, z, w = (float(value) for value in rotation.Q)
    length = math.sqrt(x * x + y * y + z * z + w * w)
    if length < 1e-12:
        raise ValueError("FreeCAD camera rotation has zero length")
    x, y, z, w = x / length, y / length, z / length, w / length
    # q and -q describe the same rotation. Keeping w non-negative selects the
    # shorter, numerically stable axis-angle representation Coin expects.
    if w < 0.0:
        x, y, z, w = -x, -y, -z, -w
    w = max(-1.0, min(1.0, w))
    angle = 2.0 * math.acos(w)
    scale = math.sqrt(max(0.0, 1.0 - w * w))
    if scale < 1e-9:
        axis = (0.0, 0.0, 1.0)
        angle = 0.0
    else:
        axis = (x / scale, y / scale, z / scale)
    replacement = "orientation {:.12g} {:.12g} {:.12g} {:.12g}".format(
        axis[0], axis[1], axis[2], angle
    )
    updated, count = re.subn(
        r"(?m)^\s*orientation\s+%s\s+%s\s+%s\s+%s\s*$"
        % (_NUMBER, _NUMBER, _NUMBER, _NUMBER),
        replacement,
        camera_text,
        count=1,
    )
    if count != 1:
        raise ValueError("FreeCAD camera text has no orientation")
    return updated


def _set_camera_orientation(view, camera_text, rotation):
    view.setCamera(_camera_with_orientation(camera_text, rotation))


def _set_camera_pose(view, camera_text, position, rotation):
    updated = _camera_with_position(camera_text, position)
    updated = _camera_with_orientation(updated, rotation)
    view.setCamera(updated)


def save_settings(values):
    path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "settings.json")
    with open(path, "w", encoding="utf-8") as stream:
        json.dump(values, stream, indent=2)
        stream.write("\n")
    if _controller is not None:
        _controller.settings = dict(values)


def get_settings():
    return dict(_controller.settings) if _controller is not None else _load_settings()


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
        self.pan_button_down = False
        self.rotate_button_down = False
        self.pan_latched = False
        self.rotate_latched = False

    def _navigation_mode(self, sample):
        if not sample or not sample.get("active"):
            self.pan_button_down = False
            self.rotate_button_down = False
            self.pan_latched = False
            self.rotate_latched = False
            return "orbit"
        pan_down = bool(sample.get("pan"))
        rotate_down = bool(sample.get("rotate"))
        if self.settings["togglePanMode"]:
            if pan_down and not self.pan_button_down:
                self.pan_latched = not self.pan_latched
                if self.pan_latched:
                    self.rotate_latched = False
        else:
            self.pan_latched = False
        if self.settings["toggleRotateMode"]:
            if rotate_down and not self.rotate_button_down:
                self.rotate_latched = not self.rotate_latched
                if self.rotate_latched:
                    self.pan_latched = False
        else:
            self.rotate_latched = False
        self.pan_button_down = pan_down
        self.rotate_button_down = rotate_down
        pan_active = self.pan_latched if self.settings["togglePanMode"] else pan_down
        rotate_active = self.rotate_latched if self.settings["toggleRotateMode"] else rotate_down
        if rotate_active:
            return "rotate"
        if pan_active:
            return "pan"
        return "orbit"

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
                if sample.get("version") in (1, 2):
                    latest = sample
            except BlockingIOError:
                return latest
            except (ValueError, UnicodeDecodeError):
                continue

    def _tick(self):
        try:
            self._tick_camera()
        except Exception as error:
            # A Qt timer exception is otherwise reported every 16 ms and can
            # overwhelm FreeCAD's notification area. Pause until the user
            # toggles navigation back on after reviewing the single error.
            self.timer.stop()
            App.Console.PrintError(
                "Replicazeron navigation paused after a camera error: {}\n".format(error)
            )

    def _tick_camera(self):
        sample = self._latest_sample()
        now = time.monotonic()
        main_window = Gui.getMainWindow()
        foreground = main_window.isActiveWindow()
        navigation_mode = self._navigation_mode(sample if foreground else None)
        if sample is None:
            return
        if not sample.get("active") or not foreground or Gui.ActiveDocument is None:
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

        view = Gui.ActiveDocument.ActiveView
        # FreeCAD otherwise interpolates each tiny orientation change while
        # another one arrives, producing severe lag at the bridge report rate.
        view.setAnimationEnabled(False)
        rotation = view.getCameraOrientation()
        direction = rotation.multVec(App.Vector(0, 0, -1))

        up = rotation.multVec(App.Vector(0, 1, 0))
        right = rotation.multVec(App.Vector(1, 0, 0))
        if navigation_mode == "pan":
            camera_text = view.getCamera()
            position, focal_distance = _camera_position_and_focal_distance(camera_text)
            rate = float(self.settings["panScalePerSecond"])
            position += (right * horizontal + up * vertical) * focal_distance * rate * delta_time
            _set_camera_position(view, camera_text, position)
        elif navigation_mode == "rotate":
            rate = float(self.settings["rotationDegreesPerSecond"])
            # Roll around the viewing axis while keeping both camera position
            # and focal target fixed.  The horizontal stick axis controls roll.
            rotation = App.Rotation(direction, horizontal * rate * delta_time).multiply(rotation)
            _set_camera_orientation(view, view.getCamera(), rotation)
        else:
            camera_text = view.getCamera()
            position, focal_distance = _camera_position_and_focal_distance(camera_text)
            target = position + direction * focal_distance
            rate = float(self.settings["rotationDegreesPerSecond"])
            yaw = horizontal * rate * delta_time
            pitch = -vertical * rate * delta_time
            rotation = App.Rotation(up, yaw).multiply(rotation)
            right = rotation.multVec(App.Vector(1, 0, 0))
            rotation = App.Rotation(right, pitch).multiply(rotation)
            direction = rotation.multVec(App.Vector(0, 0, -1))
            position = target - direction * focal_distance
            _set_camera_pose(view, camera_text, position, rotation)
        # We are already executing on FreeCAD's GUI thread.  Processing a
        # nested GUI event loop here stalls input and can recursively fire the
        # controller timer; request only a viewport redraw instead.
        view.redraw()


def start():
    global _controller
    if _controller is None:
        _controller = CadController()
        if _controller.settings["startBridgeAutomatically"]:
            started, message = ensure_bridge_started(os.path.dirname(os.path.realpath(__file__)))
            if started:
                App.Console.PrintMessage(message + "\n")
            else:
                App.Console.PrintWarning(message + "\n")
        _controller.start()
    else:
        _controller.start()


def stop():
    global _controller
    if _controller is not None:
        _controller.stop()
        _controller = None


def toggle():
    stop() if is_running() else start()


def is_running():
    return _controller is not None and _controller.timer.isActive()
