# Replicazeron CAD bridge

This bridge gives Fusion and FreeCAD direct analog camera orbit, rotate, and pan without
moving the system pointer, holding mouse buttons, or sending keyboard keys.
The normal mouse remains available for selection and editing.

The bridge is distributed as a self-contained executable that the packaged CAD
add-ins start automatically. The Windows Fusion and FreeCAD 1.1 add-ins have
been validated with physical Replicazeron hardware. The Linux x86_64 FreeCAD
package remains experimental:

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

- Two-axis, strength-sensitive orbit around the current camera target, with a
  momentary firmware control for proportional pan.
- Existing firmware deadzone, directional filtering, rotation correction, and
  smoothing are applied before the bridge receives movement.
- Fusion and FreeCAD expose independent orbit/rotate speed, pan speed, and horizontal
  and vertical reversal settings in their add-in UI.
- Hold the default Settings mouse-mode key to pan in CAD bridge mode and release
  it to return to orbit. **CAD Pan** and **CAD Rotate** can also be assigned to
  non-five-way keys from Vial's User tab. Each control has its own hold/toggle
  option in the CAD add-in settings.
- No zoom, view shortcuts, or five-way actions are assigned.

Only one bridge process runs at a time, even when both CAD applications start
their bundled copy. Each add-in registers its CAD host process with the shared
bridge. The bridge stays available while either application is running and
closes automatically after the last registered CAD application exits. The
FreeCAD adapter still needs application and physical-controller testing.

Changing away from Settings L11 is a normal inactive state: the bridge remains
available and resumes CAD reports when Settings and **CAD bridge** mode become
active again. On Windows, bringing Vial to the foreground makes the bridge
close its Raw HID handle so Vial has exclusive access to configuration replies.
Moving away from Vial makes the bridge re-enumerate the controller and resume
automatically.

## Add-in settings and pan control

Fusion and FreeCAD store their settings independently. Changes made in the
settings dialog apply immediately. Fusion stores its writable settings at
`%APPDATA%\Replicazeron\Fusion\settings.json`; FreeCAD keeps its own
`settings.json`.

| Setting | Effect |
| --- | --- |
| **Reverse left / right** | Reverses horizontal stick movement without changing vertical movement. |
| **Reverse up / down** | Reverses vertical stick movement without changing horizontal movement. |
| **Orbit / rotate speed** | Sets the maximum orbit and camera-roll rate. |
| **Pan speed** | Sets proportional camera translation speed. |
| **Toggle CAD Pan instead of hold** | Off: CAD Pan is active only while held. On: each CAD Pan press toggles its latched state. |
| **Toggle CAD Rotate instead of hold** | Off: camera roll is active only while CAD Rotate is held. On: each CAD Rotate press toggles it. |
| **Start bridge automatically** | Starts the bundled background bridge with the CAD application. |

Assign **CAD Pan** and optionally **CAD Rotate** to suitable remappable buttons
from Vial's **User** tab.
The default Settings mouse-mode button also becomes the pan control when the
Settings stick is in **CAD bridge** mode. Momentary hold-to-pan is the default:
press and hold the button while moving the stick to pan, then release it to
return to orbit. CAD Rotate keeps the camera position and target fixed and uses
the horizontal stick axis to roll the view. Each modifier can independently use momentary or toggle
behavior.

## Firmware and WebHID

Flash firmware containing CAD report format 2, then use WebHID to set the
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

Set `startBridgeAutomatically` to `false` in the add-in settings to disable
this.
For a custom shared executable location, set the `REPLICAZERON_CAD_BRIDGE`
environment variable. The bridge also checks
`%LOCALAPPDATA%\Replicazeron\ReplicazeronCadBridge.exe`.

## Install the Fusion add-in

Extract the complete `ReplicazeronFusion` directory to:

```text
%APPDATA%\Autodesk\Autodesk Fusion 360\API\AddIns\ReplicazeronFusion
```

Do not place the add-in directly in `C:\` for a normal installation. Open
Fusion's **Scripts and Add-Ins** dialog, select `ReplicazeronFusion`, enable
**Run on Startup**, and run it. Open the Design workspace's **Add-Ins** drop-down
and choose **Replicazeron settings** to reverse either axis, adjust orbit/rotate or pan
speed, and choose hold or toggle behavior independently for CAD Pan and CAD
Rotate.

The add-in receives UDP on localhost port 28461 and updates Fusion's active
viewport camera on Fusion's UI thread through a custom application event.

## Install the FreeCAD add-on

> **Tested:** this add-on has been validated with FreeCAD 1.1 on Windows and
> physical Replicazeron hardware.

Extract the complete `ReplicazeronFreeCAD` directory into FreeCAD's versioned
user `Mod` directory. For FreeCAD 1.1 on Windows, that is:

```text
%APPDATA%\FreeCAD\v1-1\Mod\ReplicazeronFreeCAD
```

Restart FreeCAD. The add-on and bridge start automatically. Select
**Replicazeron CAD** from the workbench selector to show its toolbar commands,
which toggle navigation and open settings. Navigation can
remain active while another workbench is selected. The settings dialog reverses
either axis, adjusts orbit/rotate and pan speed, and controls automatic bridge startup.
While the **Replicazeron CAD** workbench is selected, the same commands are
available from FreeCAD's top-level **Replicazeron** menu.

The add-on receives UDP on localhost port 28462 using a Qt timer and updates
the active Coin camera while retaining the existing focal point.

## Experimental Linux FreeCAD package

Download `ReplicazeronFreeCAD-linux-x86_64.tar.gz`, extract the contained
`ReplicazeronFreeCAD` directory into FreeCAD's user `Mod` directory, and restart
FreeCAD. The package includes a self-contained `ReplicazeronCadBridge` binary;
Python is not required. It starts and stops with FreeCAD in the same way as the
Windows package.

This build is supplied for testing and has not been validated with physical
Replicazeron hardware. A tray icon is attempted when the Linux desktop exposes
a compatible notification area; the bridge continues in the background when
the desktop or Wayland session does not provide one. Fusion is not packaged for
Linux because Autodesk does not provide a native Linux Fusion application.

## Development

Running directly from source still requires Python and the HID dependency:

```powershell
py -m venv .venv
.venv\Scripts\python -m pip install -r requirements.txt
.venv\Scripts\python bridge.py
```

Use `bridge.py --demo` to send a circular test signal without a controller.
Release builds use PyInstaller to produce single-file Windows and Linux x86_64
executables.

## CAD report format 2

The bridge polls Raw HID command `0x70`, operation `0x1C`. The 32-byte reply is:

| Bytes | Meaning |
|---|---|
| 0 | `0x70` command |
| 1 | `0x1C` CAD-stick reply |
| 3 | report format, currently `2` |
| 4 | active only on Settings L11 + CAD bridge mode |
| 5-6 | corrected polar angle, big-endian, 0-359 degrees |
| 7-8 | filtered and smoothed distance, big-endian |
| 9-10 | configured deadzone, big-endian |
| 11 | CAD Pan key state: `0` released, `1` pressed |
| 12 | CAD Rotate key state: `0` released, `1` pressed |

The bridge remains compatible with format 1 firmware, which has only the CAD
Pan state in byte 11.

Polling is excluded from the firmware's configuration-activity lock, so the
bridge does not continuously suppress OpenRGB traffic.
