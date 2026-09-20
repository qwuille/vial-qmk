# Replicazeron firmware

This repository contains maintained Vial/QMK firmware for the Replicazeron
one-handed game controller. It contains the firmware source, WebHID protocol
integration, build instructions, and firmware-specific hardware information.

Printable models are deliberately not distributed here. Builders looking for
existing models and the earlier hardware documentation should use the separate
[Incedius Replicazeron](https://github.com/incedius/replicazeron) and
[9R Replicazeron](https://github.com/9R/replicazeron) projects.

## Highlights

- Vial remapping, 16 named macros, and ten playable layouts plus a protected Settings
  layer.
- Per-layout Joystick, WASD, and WASD + Shift modes.
- Settings-layer proportional page scrolling with a cursor toggle, plus CAD
  middle-drag, Shift+middle-drag, and right-drag thumbstick modes.
- VialRGB/OpenRGB control through the existing Vial Raw HID interface.
- Firmware-controlled lighting with eleven effects, including KITT-style
  Knight Rider, Cylon, Pulse, and activity-driven Reactive Pulse.
- Runtime addressable-strip length from 1 to 32 pixels; default 11.
- Two independently configurable monochrome side LEDs, physically located
  together on the right side of the controller. The left LED is PB13 and the
  right LED is PB12.
- OLED menus and a standalone WebHID control deck.
- Browser-assisted STM32duino DFU updates, with a PA12 reconnect workaround
  for affected Blue Pill clones.
- Addressable and side LEDs sleep when the USB host suspends.

## Supported target

The maintained and tested target is:

```text
Keyboard: handwired/replicazeron/stm32f103
Keymap:   vial
MCU:      STM32F103 Blue Pill or compatible clone
```

Build it with:

```sh
qmk compile -kb handwired/replicazeron/stm32f103 -km vial
```

The legacy Pro Micro and experimental RP2040 definitions are not release
targets for the complete feature set documented here.

## Branches and maintenance scope

- **`vial`** is the current default maintained Replicazeron firmware.
- **`vial-incedius`** preserves the Incedius fork state from which this work
  was developed; it is retained for history and comparison.

The repository retains the wider Vial-QMK source tree so upstream updates stay
mergeable. This project maintains the Replicazeron target and its explicitly
documented core integrations, not every inherited keyboard definition.

## Documentation

### Firmware documentation

- [Complete component, pin, power, and compatibility guide](keyboards/handwired/replicazeron/HARDWARE.md)
- [Firmware features, OpenRGB, WebHID, DFU, wiring, and usage](keyboards/handwired/replicazeron/readme.md)
- [Vial layout implementation notes](keyboards/handwired/replicazeron/VIAL_LAYOUT_NOTES.md)

### Hardware, models, and earlier build resources

- [Incedius Replicazeron](https://github.com/incedius/replicazeron) for
  Incedius's modified parts, build notes, bill of materials, and assembly
  information.
- [9R Replicazeron](https://github.com/9R/replicazeron) for 9R's modified
  model files and earlier project documentation.
- [9R Replicazeron schematics](https://github.com/9R/replicazeron_schematics)
  for the earlier controller wiring diagrams.

Those external resources describe their respective builds. Pin assignments,
LED count, bootloader behavior, and other electrical details can differ from
the current firmware target, so use the firmware hardware guide above for
the configuration implemented by this branch.

The standalone [Replicazeron WebHID Control Deck](https://github.com/qwuille/replicazeron_webhid)
is maintained in its own repository and can be opened at
**https://qwuille.github.io/replicazeron_webhid/**. Use a WebHID/WebUSB-capable
Chromium browser such as Chrome or Edge.

## OpenRGB registration

Until the device is included in OpenRGB's built-in VialRGB list, add it under
**Settings > QMK VialRGB Devices**:

| Field | Value |
|---|---|
| Name | `Replicazeron` |
| USB VID | `4142` |
| USB PID | `2305` |

Select **OpenRGB** as the RGB controller on the Replicazeron's OLED or WebHID
page. Configuration traffic from Vial or WebHID temporarily suppresses
VialRGB lighting replies; OpenRGB resumes after five seconds without
configuration traffic. Closing OpenRGB before a Vial session remains the most
conservative option because both programs still share one Raw HID interface.

## Project lineage and scope

This firmware follows work by
[9R](https://github.com/9R/qmk_firmware) and
[Incedius](https://github.com/incedius/vial-qmk), and is built on
[Vial-QMK](https://github.com/vial-kb/vial-qmk) and
[QMK](https://github.com/qmk/qmk_firmware). These software-lineage credits are
not a claim that any contributor designed the original commercial hardware
that inspired the community project. Incedius and 9R maintain their own
repositories, models, and documentation independently.

The inherited licenses and individual source-file copyright notices remain in
effect. See [LICENSE](LICENSE) and the other license files in the repository.
