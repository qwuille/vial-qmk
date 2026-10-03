"""Launch the packaged Replicazeron CAD bridge in the background."""

from __future__ import annotations

import os
import subprocess
import sys

WINDOWS_EXECUTABLE_NAME = "ReplicazeronCadBridge.exe"
LINUX_EXECUTABLE_NAME = "ReplicazeronCadBridge"
def _executable_name():
    return WINDOWS_EXECUTABLE_NAME if sys.platform == "win32" else LINUX_EXECUTABLE_NAME


def _candidates(addon_directory):
    configured = os.environ.get("REPLICAZERON_CAD_BRIDGE")
    if configured:
        yield configured
    yield os.path.join(addon_directory, _executable_name())
    local_app_data = os.environ.get("LOCALAPPDATA")
    if local_app_data:
        yield os.path.join(local_app_data, "Replicazeron", _executable_name())


def ensure_bridge_started(addon_directory):
    if sys.platform not in {"win32", "linux"}:
        return False, "Automatic bridge startup is currently available on Windows and Linux only"

    executable = next((path for path in _candidates(addon_directory) if os.path.isfile(path)), None)
    if executable is None:
        return False, "{} was not found beside the add-on".format(_executable_name())

    options = {
        "cwd": os.path.dirname(executable),
        "stdin": subprocess.DEVNULL,
        "stdout": subprocess.DEVNULL,
        "stderr": subprocess.DEVNULL,
        "close_fds": True,
    }
    if sys.platform == "win32":
        options["creationflags"] = subprocess.CREATE_NO_WINDOW | subprocess.DETACHED_PROCESS
    else:
        options["start_new_session"] = True
    try:
        subprocess.Popen(
            [executable, "--parent-pid", str(os.getpid())],
            **options,
        )
    except OSError as error:
        return False, "Could not start Replicazeron CAD bridge: {}".format(error)

    return True, "Replicazeron CAD bridge started automatically"
