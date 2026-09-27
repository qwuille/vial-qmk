# Replicazeron CAD bridge

This prototype gives Fusion and FreeCAD direct analog camera rotation without
moving the system pointer, holding mouse buttons, or sending keyboard keys.
The normal mouse remains available for selection and editing.

The bridge is intentionally separate from the CAD programs:

```text
Replicazeron Raw HID -> bridge.py -> Fusion add-in (UDP 28461)
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

The implementation is source-verified but still needs testing with a physical
controller in both applications.

## Firmware and WebHID

Flash firmware containing CAD report format 1, then use WebHID to set the
Settings L11 stick mode to **CAD bridge · Fusion and FreeCAD add-ons**. The old
middle-drag, Shift+middle-drag, and right-drag modes have been removed. Stored
mode 1 migrates to CAD bridge; obsolete stored modes 2 and 3 fall back to
scroll/cursor.

## Start the bridge on Windows

From this directory:

```powershell
py -m venv .venv
.venv\Scripts\python -m pip install -r requirements.txt
.venv\Scripts\python bridge.py
```

Use `bridge.py --demo` to send a circular test signal without a controller.
Stop it with Ctrl+C. Packaging it as a startup application can wait until the
camera direction and sensitivity have been verified on real hardware.

## Install the Fusion add-in

Copy the complete `fusion\ReplicazeronFusion` directory to:

```text
%APPDATA%\Autodesk\Autodesk Fusion 360\API\AddIns\ReplicazeronFusion
```

Open Fusion's **Scripts and Add-Ins** dialog, select `ReplicazeronFusion`, enable
**Run on Startup**, and run it. Edit its `settings.json` to change rotation
speed or invert an axis.

The add-in receives UDP on localhost port 28461 and updates Fusion's active
viewport camera on Fusion's UI thread through a custom application event.

## Install the FreeCAD add-on

Copy the complete `freecad\ReplicazeronFreeCAD` directory to:

```text
%APPDATA%\FreeCAD\Mod\ReplicazeronFreeCAD
```

Restart FreeCAD and select the **Replicazeron CAD** workbench once. Its toolbar
command toggles navigation; after initialization it can remain active while
another workbench is selected. Edit its `settings.json` to change rotation
speed or invert an axis.

The add-on receives UDP on localhost port 28462 using a Qt timer and updates
the active Coin camera while retaining the existing focal point.

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
