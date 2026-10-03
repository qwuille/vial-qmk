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

try:
    from .bridge_launcher import ensure_bridge_started
except ImportError:
    from bridge_launcher import ensure_bridge_started

EVENT_ID = "replicazeron.cad.stick"
UDP_PORT = 28461
SETTINGS_COMMAND_ID = "ReplicazeronCadSettingsCommand"
SETTINGS_PANEL_ID = "SolidScriptsAddinsPanel"
_handlers = []
_controller = None


def _user_settings_path():
    root = os.environ.get("APPDATA")
    if not root:
        root = os.path.join(os.path.expanduser("~"), ".config")
    return os.path.join(root, "Replicazeron", "Fusion", "settings.json")


def _legacy_settings_path():
    return os.path.join(os.path.dirname(os.path.realpath(__file__)), "settings.json")


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
    for path in (_user_settings_path(), _legacy_settings_path()):
        try:
            with open(path, "r", encoding="utf-8") as stream:
                values.update(json.load(stream))
            break
        except (OSError, ValueError):
            continue
    return values


def _save_settings(values):
    path = _user_settings_path()
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as stream:
        json.dump(values, stream, indent=2)
        stream.write("\n")


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


def _save_settings_from_inputs(inputs):
    if _controller is None:
        raise RuntimeError("Replicazeron controller is not running")

    values = dict(_controller.settings)
    values["invertHorizontal"] = inputs.itemById("invertHorizontal").value
    values["invertVertical"] = inputs.itemById("invertVertical").value
    values["rotationDegreesPerSecond"] = inputs.itemById("rotationRate").value
    values["panScalePerSecond"] = inputs.itemById("panRate").value
    values["togglePanMode"] = inputs.itemById("togglePanMode").value
    values["toggleRotateMode"] = inputs.itemById("toggleRotateMode").value
    values["startBridgeAutomatically"] = inputs.itemById("startBridge").value
    _save_settings(values)
    _controller.settings = values
    adsk.core.Application.get().log("Replicazeron settings saved and applied")


def _show_settings_error():
    app = adsk.core.Application.get()
    message = "Replicazeron settings could not be saved:\n\n" + traceback.format_exc()
    app.log(message)
    app.userInterface.messageBox(message)


class _SettingsExecuteHandler(adsk.core.CommandEventHandler):
    def __init__(self, inputs):
        super().__init__()
        self.inputs = inputs

    def notify(self, _args):
        try:
            _save_settings_from_inputs(self.inputs)
        except Exception:
            _show_settings_error()


class _SettingsInputChangedHandler(adsk.core.InputChangedEventHandler):
    def __init__(self, inputs):
        super().__init__()
        self.inputs = inputs

    def notify(self, _args):
        try:
            _save_settings_from_inputs(self.inputs)
        except Exception:
            _show_settings_error()


class _SettingsDestroyHandler(adsk.core.CommandEventHandler):
    def __init__(self, inputs):
        super().__init__()
        self.inputs = inputs

    def notify(self, _args):
        try:
            _save_settings_from_inputs(self.inputs)
        except Exception:
            _show_settings_error()


class _SettingsCreatedHandler(adsk.core.CommandCreatedEventHandler):
    def __init__(self):
        super().__init__()

    def notify(self, args):
        try:
            settings = _controller.settings
            inputs = args.command.commandInputs
            inputs.addBoolValueInput("invertHorizontal", "Reverse left / right", True, "", bool(settings["invertHorizontal"]))
            inputs.addBoolValueInput("invertVertical", "Reverse up / down", True, "", bool(settings["invertVertical"]))
            inputs.addFloatSpinnerCommandInput("rotationRate", "Orbit / rotate speed (degrees/second)", "", 5.0, 360.0, 5.0, float(settings["rotationDegreesPerSecond"]))
            inputs.addFloatSpinnerCommandInput("panRate", "Pan speed (view widths/second)", "", 0.05, 5.0, 0.05, float(settings["panScalePerSecond"]))
            inputs.addBoolValueInput("togglePanMode", "Toggle CAD Pan instead of hold", True, "", bool(settings["togglePanMode"]))
            inputs.addBoolValueInput("toggleRotateMode", "Toggle CAD Rotate instead of hold", True, "", bool(settings["toggleRotateMode"]))
            inputs.addBoolValueInput("startBridge", "Start bridge automatically", True, "", bool(settings["startBridgeAutomatically"]))
            inputs.addTextBoxCommandInput(
                "saveNotice",
                "",
                "Changes are applied and saved immediately.",
                1,
                True,
            )

            execute_handler = _SettingsExecuteHandler(inputs)
            args.command.execute.add(execute_handler)
            _handlers.append(execute_handler)

            changed_handler = _SettingsInputChangedHandler(inputs)
            args.command.inputChanged.add(changed_handler)
            _handlers.append(changed_handler)

            destroy_handler = _SettingsDestroyHandler(inputs)
            args.command.destroy.add(destroy_handler)
            _handlers.append(destroy_handler)

            # Create or migrate the per-user file while commandCreated is
            # known to be running, even before Fusion delivers later events.
            _save_settings_from_inputs(inputs)
        except Exception:
            _show_settings_error()


def _add_settings_command(app):
    ui = app.userInterface
    command = ui.commandDefinitions.itemById(SETTINGS_COMMAND_ID)
    if command is None:
        command = ui.commandDefinitions.addButtonDefinition(
            SETTINGS_COMMAND_ID,
            "Replicazeron settings",
            "Configure Replicazeron orbit, rotate, pan, and axis directions",
            "",
        )
    handler = _SettingsCreatedHandler()
    command.commandCreated.add(handler)
    _handlers.append(handler)

    panel = ui.allToolbarPanels.itemById(SETTINGS_PANEL_ID)
    if panel is None:
        raise RuntimeError("Fusion Add-Ins toolbar panel is unavailable")
    if panel.controls.itemById(SETTINGS_COMMAND_ID) is None:
        control = panel.controls.addCommand(command)
        if control is None:
            raise RuntimeError("Could not add Replicazeron settings to the Add-Ins panel")
        control.isPromotedByDefault = True
        control.isPromoted = True
    app.log("Replicazeron settings command added to Fusion Add-Ins panel")


def _remove_settings_command(app):
    ui = app.userInterface
    panel = ui.allToolbarPanels.itemById(SETTINGS_PANEL_ID)
    if panel is not None:
        control = panel.controls.itemById(SETTINGS_COMMAND_ID)
        if control is not None:
            control.deleteMe()
    command = ui.commandDefinitions.itemById(SETTINGS_COMMAND_ID)
    if command is not None:
        command.deleteMe()


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
        self.pan_button_down = False
        self.rotate_button_down = False
        self.pan_latched = False
        self.rotate_latched = False
        self.thread = threading.Thread(target=self._receive, name="ReplicazeronFusion", daemon=True)

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
                if sample.get("version") not in (1, 2):
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
        foreground = _foreground_is_fusion()
        navigation_mode = self._navigation_mode(sample if foreground else None)
        if not sample or not sample.get("active") or not foreground:
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

        viewport = self.app.activeViewport
        if viewport is None:
            return
        camera = viewport.camera
        eye = (camera.eye.x, camera.eye.y, camera.eye.z)
        target = (camera.target.x, camera.target.y, camera.target.z)
        up = _normalize((camera.upVector.x, camera.upVector.y, camera.upVector.z))
        offset = _subtract(eye, target)
        right = _normalize(_cross(up, offset))
        if navigation_mode == "pan":
            distance = math.sqrt(_dot(offset, offset))
            rate = float(self.settings["panScalePerSecond"])
            movement = _scale(_add(_scale(right, horizontal), _scale(up, vertical)), distance * rate * delta_time)
            next_eye = _add(eye, movement)
            next_target = _add(target, movement)
            camera.target = adsk.core.Point3D.create(*next_target)
        elif navigation_mode == "rotate":
            rate = float(self.settings["rotationDegreesPerSecond"])
            direction = _subtract(target, eye)
            # Keep both camera points fixed and roll around the viewing axis.
            # This is intentionally distinct from the normal two-axis orbit.
            roll = math.radians(horizontal * rate * delta_time)
            up = _rotate(up, _normalize(direction), roll)
            next_eye = eye
        else:
            rate = float(self.settings["rotationDegreesPerSecond"])
            yaw = math.radians(horizontal * rate * delta_time)
            pitch = math.radians(-vertical * rate * delta_time)
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
        settings = _load_settings()
        if settings["startBridgeAutomatically"]:
            _started, message = ensure_bridge_started(os.path.dirname(os.path.realpath(__file__)))
            app.log(message)
        event = app.registerCustomEvent(EVENT_ID)
        _controller = CadController(app)
        handler = _CadEventHandler(_controller)
        event.add(handler)
        _handlers.append(handler)
        _controller.start()
        _add_settings_command(app)
        app.log("Replicazeron Fusion CAD navigation loaded")
    except Exception:
        message = "Replicazeron Fusion add-in failed:\n\n" + traceback.format_exc()
        app.log(message)
        app.userInterface.messageBox(message)


def stop(_context):
    global _controller
    app = adsk.core.Application.get()
    if _controller is not None:
        _controller.stop()
        _controller = None
    try:
        _remove_settings_command(app)
        app.unregisterCustomEvent(EVENT_ID)
    except Exception:
        pass
    _handlers.clear()
