# ReplicazEron roadmap

## Current state
- The device is working well in daily use.
- The menu system is functional.
- The OLED UI is functional.
- The HID activity LED works well.
- The joystick LED works, but calibration and scaling still need refinement.
- Layer persistence works.
- Input mode persistence works.

## Working approach
For now, keep the firmware stable and collect real-world issues before making larger architectural changes.

## Prioritized improvements

### 1. Profile browser first
This should be the first user-visible improvement.

Goals:
- Keep the existing Settings layer as a reserved configuration layer.
- Make the Profile action the first item in the menu flow.
- Replace repeated profile cycling with a browser-style selection flow.

Implementation notes:
- Treat the existing layer structure as the base profile system.
- Reserve the Settings layer as the last layer and keep it out of profile browsing.
- Add a simple menu state machine for browsing profiles and activating a selection.

### 2. Profile naming
After the browser is working:
- Add editable profile names.
- Store names in a Vial-friendly way.
- Keep a fallback naming system for unnamed profiles.

### 3. Calibration menu
After browser and naming are in place:
- Add a Calib menu entry.
- Capture center, left, right, up, and down stick positions.
- Store calibration values for use in joystick scaling.

## Code areas to update
- [replicazeron.c](replicazeron.c): layer change handling and custom keycode processing
- [common/state.h](common/state.h) and [common/state.c](common/state.c): persistent runtime state
- [common/oled.c](common/oled.c): display the profile browser and menu flow
- [common/thumbstick.c](common/thumbstick.c): joystick scaling and calibration hooks
- [keymaps/default/keymap.c](keymaps/default/keymap.c): menu and profile navigation key assignments

## Suggested implementation order
1. Collect real-world bugs and usability issues.
2. Implement a profile browser with a simple menu.
3. Add profile naming support.
4. Add the calibration menu.
5. Refine joystick LED behavior and scaling after the menu flow is stable.
