"""Launch the packaged Replicazeron CAD bridge without showing a console."""

from __future__ import annotations

import os
import subprocess
import sys

EXECUTABLE_NAME = "ReplicazeronCadBridge.exe"
_started = False


def _candidates(addon_directory):
    configured = os.environ.get("REPLICAZERON_CAD_BRIDGE")
    if configured:
        yield configured
    yield os.path.join(addon_directory, EXECUTABLE_NAME)
    local_app_data = os.environ.get("LOCALAPPDATA")
    if local_app_data:
        yield os.path.join(local_app_data, "Replicazeron", EXECUTABLE_NAME)


def ensure_bridge_started(addon_directory):
    global _started
    if _started:
        return True, "Replicazeron CAD bridge launch already requested"
    if sys.platform != "win32":
        return False, "Automatic bridge startup is currently available on Windows only"

    executable = next((path for path in _candidates(addon_directory) if os.path.isfile(path)), None)
    if executable is None:
        return False, "ReplicazeronCadBridge.exe was not found beside the add-on"

    creation_flags = subprocess.CREATE_NO_WINDOW | subprocess.DETACHED_PROCESS
    try:
        subprocess.Popen(
            [executable, "--parent-pid", str(os.getpid())],
            cwd=os.path.dirname(executable),
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            close_fds=True,
            creationflags=creation_flags,
        )
    except OSError as error:
        return False, "Could not start Replicazeron CAD bridge: {}".format(error)

    _started = True
    return True, "Replicazeron CAD bridge started automatically"
