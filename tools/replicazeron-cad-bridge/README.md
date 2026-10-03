# Replicazeron CAD bridge

This bridge gives Fusion and FreeCAD direct analog camera rotation without
moving the system pointer, holding mouse buttons, or sending keyboard keys.
The normal mouse remains available for selection and editing.

The bridge is a self-contained Windows executable that the packaged CAD add-ins
start automatically:

```text
Replicazeron Raw HID -> ReplicazeronCadBridge.exe -> Fusion add-in (UDP 28461)
                                                  -> FreeCAD add-on (UDP 28462)
```

All UDP traffic stays on `127.0.0.1`. The adapters discard movement while
their application is not the foreground window. The firmware only marks the
report active while Settings L11 and its **CAD bridge** stick mode are active.
The Settings five-way switch remains permanently firmware-owned; neither the
report nor either add-on assigns any CAD action to it.

## Current scope

- Two-axis, strength-sensitive orbit around the current camera target.
- Existing firmware deadzone, directional filtering, rotation correction, and
  smoothing are applied before the bridge receives movement.
- Fusion and FreeCAD have independent sensitivity and inversion files.
- No pan, zoom, view shortcuts, stick-button actions, or five-way actions are
  assigned yet. Those controls can be designed after orbit is hardware-tested.

Only one bridge process runs at a time, even when both CAD applications start
their bundled copy. Each add-in registers its CAD host process with the shared
bridge. The bridge stays available while either application is running and
closes automatically after the last registered CAD application exits. The
implementation still needs testing with a physical controller in both
applications.

## Firmware and WebHID

Flash firmware containing CAD report format 1, then use WebHID to set the
Settings L11 stick mode to **CAD bridge · Fusion and FreeCAD add-ons**. The old
middle-drag, Shift+middle-drag, and right-drag modes have been removed. Stored
mode 1 migrates to CAD bridge; obsolete stored modes 2 and 3 fall back to
scroll/cursor.

## Install on Windows

Download the add-in ZIP for the CAD application from the matching GitHub
release:

- `ReplicazeronFusion-windows.zip`
- `ReplicazeronFreeCAD-windows.zip`

Each package contains `ReplicazeronCadBridge.exe`; Python and pip are not
required. When the add-in starts, it launches the bridge as a notification-area
application without a console window. Use the **Replicazeron CAD Bridge** system
tray icon to confirm it is running or exit it manually. It normally exits by
itself when the last Fusion or FreeCAD process using it closes. A rotating
diagnostic log is written to `%LOCALAPPDATA%\Replicazeron\cad-bridge.log`.

Set `startBridgeAutomatically` to `false` in `settings.json` to disable this.
For a custom shared executable location, set the `REPLICAZERON_CAD_BRIDGE`
environment variable. The bridge also checks
`%LOCALAPPDATA%\Replicazeron\ReplicazeronCadBridge.exe`.

## Install the Fusion add-in

Extract the complete `ReplicazeronFusion` directory to:

```text
%APPDATA%\Autodesk\Autodesk Fusion 360\API\AddIns\ReplicazeronFusion
```

Open Fusion's **Scripts and Add-Ins** dialog, select `ReplicazeronFusion`, enable
**Run on Startup**, and run it. Edit its `settings.json` to change rotation
speed or invert an axis.

The add-in receives UDP on localhost port 28461 and updates Fusion's active
viewport camera on Fusion's UI thread through a custom application event.

## Install the FreeCAD add-on

Extract the complete `ReplicazeronFreeCAD` directory to:

```text
%APPDATA%\FreeCAD\Mod\ReplicazeronFreeCAD
```

Restart FreeCAD. The add-on and bridge start automatically; its toolbar command
in the **Replicazeron CAD** workbench toggles navigation. It can remain active
while another workbench is selected. Edit its `settings.json` to change
rotation speed, invert an axis, or disable automatic bridge startup.

The add-on receives UDP on localhost port 28462 using a Qt timer and updates
the active Coin camera while retaining the existing focal point.

## Development

Running directly from source still requires Python and the HID dependency:

```powershell
py -m venv .venv
.venv\Scripts\python -m pip install -r requirements.txt
.venv\Scripts\python bridge.py
```

Use `bridge.py --demo` to send a circular test signal without a controller.
Release builds use PyInstaller to produce the single-file Windows executable.

## CAD report format 1

The bridge polls Raw HID command `0x70`, operation `0x1C`. The 32-byte reply is:

| Bytes | Meaning |
|---|---|
| 0 | `0x70` command |
| 1 | `0x1C` CAD-stick reply |
| 3 | report format, currently `1` |
| 4 | active only on Settings L11 + CAD bridge mode |
| 5-6 | corrected polar angle, big-endian, 0-359 degrees |
| 7-8 | filtered and smoothed distance, big-endian |
| 9-10 | configured deadzone, big-endian |

Polling is excluded from the firmware's configuration-activity lock, so the
bridge does not continuously suppress OpenRGB traffic.
