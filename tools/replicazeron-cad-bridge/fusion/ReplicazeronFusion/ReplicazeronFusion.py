"""Fusion add-in for pointer-independent Replicazeron viewport rotation."""

from __future__ import annotations

import json
import math
import os
import socket
import sys
import threading
import time
import traceback

import adsk.core

EVENT_ID = "replicazeron.cad.stick"
UDP_PORT = 28461
_handlers = []
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


def _foreground_is_fusion():
    if sys.platform != "win32":
        return True
    try:
        import ctypes

        user32 = ctypes.windll.user32
        window = user32.GetForegroundWindow()
        length = user32.GetWindowTextLengthW(window)
        title = ctypes.create_unicode_buffer(length + 1)
        user32.GetWindowTextW(window, title, len(title))
        return "fusion" in title.value.lower()
    except Exception:
        return True


def _add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def _subtract(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _scale(value, factor):
    return (value[0] * factor, value[1] * factor, value[2] * factor)


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def _normalize(value):
    length = math.sqrt(_dot(value, value))
    if length < 1e-9:
        raise ValueError("zero-length camera vector")
    return _scale(value, 1.0 / length)


def _rotate(value, axis, radians):
    axis = _normalize(axis)
    cosine = math.cos(radians)
    sine = math.sin(radians)
    return _add(
        _add(_scale(value, cosine), _scale(_cross(axis, value), sine)),
        _scale(axis, _dot(axis, value) * (1.0 - cosine)),
    )


class _CadEventHandler(adsk.core.CustomEventHandler):
    def __init__(self, controller):
        super().__init__()
        self._controller = controller

    def notify(self, _args):
        try:
            self._controller.apply_latest()
        except Exception:
            adsk.core.Application.get().log(traceback.format_exc())


class CadController:
    def __init__(self, app):
        self.app = app
        self.settings = _load_settings()
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(("127.0.0.1", UDP_PORT))
        self.socket.settimeout(0.25)
        self.lock = threading.Lock()
        self.latest = None
        self.event_pending = False
        self.running = True
        self.last_update = None
        self.thread = threading.Thread(target=self._receive, name="ReplicazeronFusion", daemon=True)

    def start(self):
        self.thread.start()

    def stop(self):
        self.running = False
        self.socket.close()
        self.thread.join(timeout=1.0)

    def _receive(self):
        while self.running:
            try:
                payload, _source = self.socket.recvfrom(2048)
                sample = json.loads(payload.decode("utf-8"))
                if sample.get("version") != 1:
                    continue
                should_fire = False
                with self.lock:
                    self.latest = sample
                    if not self.event_pending:
                        self.event_pending = True
                        should_fire = True
                if should_fire:
                    self.app.fireCustomEvent(EVENT_ID)
            except socket.timeout:
                continue
            except (OSError, ValueError, UnicodeDecodeError):
                if self.running:
                    time.sleep(0.1)

    def apply_latest(self):
        with self.lock:
            sample = self.latest
            self.event_pending = False
        now = time.monotonic()
        if not sample or not sample.get("active") or not _foreground_is_fusion():
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
        yaw = math.radians(horizontal * rate * delta_time)
        pitch = math.radians(-vertical * rate * delta_time)
        viewport = self.app.activeViewport
        if viewport is None:
            return
        camera = viewport.camera
        eye = (camera.eye.x, camera.eye.y, camera.eye.z)
        target = (camera.target.x, camera.target.y, camera.target.z)
        up = _normalize((camera.upVector.x, camera.upVector.y, camera.upVector.z))
        offset = _subtract(eye, target)

        offset = _rotate(offset, up, yaw)
        right = _normalize(_cross(up, offset))
        offset = _rotate(offset, right, pitch)
        up = _rotate(up, right, pitch)
        next_eye = _add(target, offset)

        camera.eye = adsk.core.Point3D.create(*next_eye)
        camera.upVector = adsk.core.Vector3D.create(*_normalize(up))
        camera.isSmoothTransition = False
        viewport.camera = camera
        viewport.refresh()


def run(_context):
    global _controller
    app = adsk.core.Application.get()
    try:
        event = app.registerCustomEvent(EVENT_ID)
        _controller = CadController(app)
        handler = _CadEventHandler(_controller)
        event.add(handler)
        _handlers.append(handler)
        _controller.start()
        app.log("Replicazeron Fusion CAD navigation loaded")
    except Exception:
        app.log(traceback.format_exc())


def stop(_context):
    global _controller
    app = adsk.core.Application.get()
    if _controller is not None:
        _controller.stop()
        _controller = None
    try:
        app.unregisterCustomEvent(EVENT_ID)
    except Exception:
        pass
    _handlers.clear()
