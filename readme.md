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
- Per-layout WASD and configurable Faux analog modes. RP2040 additionally has
  Joystick and XInput + keyboard layouts with freely assignable gamepad buttons.
- Settings-layer proportional page scrolling with a cursor toggle, plus a
  pointer-independent analog CAD bridge for Fusion and FreeCAD. Entering
  Settings releases and suppresses the playable layer's WASD/Faux keys.
- VialRGB/OpenRGB control through the existing Vial Raw HID interface.
- Firmware-controlled lighting with reactive, splash, multisplash,
  reactive-cross, and reactive-wide effects. The RP2040 build provides sixteen
  effects in total; the flash-limited Blue Pill provides twelve.
- Runtime addressable-strip length from 1 to 32 pixels; default 11.
- Two independently configurable monochrome side LEDs, physically located
  together on the right side of the controller.
- OLED menus and a standalone WebHID control deck.
- Browser-assisted STM32duino DFU and RP2040 PICOBOOT updates, with automatic
  target selection and a PA12 reconnect workaround for affected Blue Pill
  clones.
- Addressable and side LEDs sleep when the USB host suspends.

## Supported targets

The maintained and tested STM32 target is:

```text
Keyboard: handwired/replicazeron/stm32f103
Keymap:   vial
MCU:      STM32F103 Blue Pill or compatible clone
```

The STM32 USB HID/DirectInput joystick gaming mode was removed from the
recommended Standard release after
testing showed that mixed joystick and keyboard input made games repeatedly
switch their HUD and active input prompts. That switching was not smooth and
could introduce visible hesitation during play. STM32 therefore presents its
gaming controls as keyboard input only. Its analog thumbstick sampling is still
used internally for proportional scrolling, cursor control, and the optional
Fusion/FreeCAD camera bridge.
The separately published, feature-frozen DirectInput compatibility release is
available for programs and games where the original interface works correctly.

Build it with:

```sh
qmk compile -kb handwired/replicazeron/stm32f103 -km vial
```

Two STM32 release variants are published:

- **Standard** (`handwired_replicazeron_stm32f103_vial.bin`) is recommended.
  It uses keyboard-only gaming modes and remains eligible for future feature
  additions.
- **DirectInput compatibility**
  (`handwired_replicazeron_stm32f103_directinput_vial.bin`) restores the USB
  HID joystick and bindable gamepad buttons for software where that interface
  is useful. It is feature-frozen because the STM32 flash is almost full; it
  receives maintenance fixes but no new features.

Build the compatibility variant with:

```sh
qmk compile -kb handwired/replicazeron/stm32f103 -km vial -e REPLICAZERON_STM32_DIRECTINPUT=yes
```

The Raspberry Pi Pico target uses the same Vial keymap and Replicazeron feature
code and compiles successfully to a UF2.

> [!WARNING]
> The RP2040 target, including its XInput interface and browser-flashing path,
> has not yet been verified on physical hardware. Treat the current UF2 as an
> experimental build and keep BOOTSEL recovery available. The STM32F103 target
> is the validated release target.

Build the experimental RP2040 firmware with:

```sh
qmk compile -kb handwired/replicazeron/rp2040 -km vial
```

Hold **BOOTSEL** while connecting the Pico, then copy
`handwired_replicazeron_rp2040_vial.uf2` to the `RPI-RP2` drive.

### Pro Micro

This version does **not** support the standard ATmega32U4 Pro Micro, and its
obsolete build target has been removed. A previous build probe found that the
current compact firmware exceeds the Pro Micro's
Caterina application space by approximately 12.5 KB, and its 1 KB EEPROM
cannot hold the present Vial layers, macro data, profile names, and
Replicazeron metadata together.

A Pro Micro build would have to lose most of the features that distinguish
this project, including the OLED interface and addressable RGB/OpenRGB support,
and would require redesigned persistent configuration. New builds should use
the supported STM32F103 Blue Pill or RP2040 target instead.

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
- [Fusion and FreeCAD analog camera bridge](tools/replicazeron-cad-bridge/README.md)
- [Interactive RP2040 wiring guide](docs/replicazeron-rp2040-wiring-diagram.html) and
  [offline PDF](docs/Replicazeron_RP2040_Wiring_Diagram.pdf)
- [Interactive STM32F103 wiring guide](docs/replicazeron-stm32f103-wiring-diagram.html) and
  [offline PDF](docs/Replicazeron_STM32F103_Wiring_Diagram.pdf)

### Image and diagram credits

The interactive wiring diagrams, their SVG artwork, and the downloadable PDF
versions were created specifically for this firmware repository and are
maintained by the Replicazeron project contributors. The WebHID controller
preview and `RZ` mark are code-generated artwork maintained in the
[Replicazeron WebHID repository](https://github.com/qwuille/replicazeron_webhid).
No third-party controller photographs are embedded in this project's pages.

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
