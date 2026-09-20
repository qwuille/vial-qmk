
# Replicazeron

Firmware lineage and software credits include **9R** and **Incedius**. This fork
builds on Replicazeron firmware work by
[9R](https://github.com/9R/qmk_firmware) and
[Incedius](https://github.com/incedius/vial-qmk), with the current firmware,
WebHID, lighting, and documentation additions maintained by
[the current firmware repository](https://github.com/qwuille/vial-qmk). These credits describe software
stewardship and community additions; they are not a claim that these
maintainers designed the original commercial hardware that inspired the
project. Printable models are not distributed by this firmware repository.

See [HARDWARE.md](HARDWARE.md) for the complete component, pin, power, and
controller-compatibility guide.

This is a config to run a 3dprinted keyboard controller with qmk based on this project:

https://sites.google.com/view/alvaro-rosati/azeron-keypad-diy-tutorial

Some additional and redesigned files (i.e. for the OLED-display) can be found in [this repo](https://github.com/9R/replicazeron).

![Replicazeron](https://i.imgur.com/WTys4SM.jpg)

[Gallery](https://imgur.com/a/2qlEPVl)

## Features

 * 23 keys
 * analog stick with WASD emulation
 * joystick mode stored separately for each of the 10 layouts
 * 5way dpad
 * 2 status LEDS
 * 11x WS2812 RGB-LED lighting

RGB lighting and both side status LEDs turn off when the USB host suspends
during PC sleep or shutdown, then restore their normal behavior when the PC
wakes.

## Vial and OpenRGB

This update combines the complete Replicazeron configuration with VialRGB and
OpenRGB support. It keeps all 11 Vial layout slots, macros, editable layout
titles, per-layout joystick modes, OLED settings, WebHID configuration, and
normal Vial remapping.

OpenRGB communicates through VialRGB on the existing 32-byte Vial Raw HID
interface. There is no bridge program and no second OpenRGB HID interface, so
the keyboard appears only once in Vial.

Build the combined firmware with:

```sh
qmk compile -c -kb handwired/replicazeron/stm32f103 -km vial
```

### Experimental browser firmware update

The WebHID configurator includes a guarded WebUSB DFU installer for the
STM32duino bootloader (`1EAF:0003`, alternate interface 2). Version 0.1.8 or
later must first be installed with the normal command-line method; later builds
can then request bootloader mode directly from the page.

Open the standalone
[Replicazeron WebHID Control Deck](https://qwuille.github.io/replicazeron_webhid/)
in Chrome or Edge, connect the running Replicazeron, and use the numbered controls under **Firmware
update**. Select a Replicazeron `.bin`, enter bootloader mode, explicitly grant
access to the bootloader, and confirm the flash. The page validates the image
size and STM32 vector table before enabling the write button. This experimental
path is based on the same browser DFU approach demonstrated by
[WebDFU](https://github.com/devanlai/webdfu); keep the command-line flasher
available as the recovery path while testing.

The bootloader control offers two reset methods. **Blue Pill USB reconnect
assist (PA12)** first disconnects USB D+ in software for 250 ms, then resets;
this emulates the cable reconnect needed by some inexpensive STM32F103 clone
boards. **Standard software reset** preserves the previous behavior for boards
that enumerate correctly without assistance. The choice applies only to that
bootloader request and is not saved in EEPROM.

This USB workaround does not change the boot-selection wiring. On common Blue
Pills, PB2 is BOOT1 and may also be the STM32duino bootloader button, while
PC13 is the onboard LED. PB13 is a Replicazeron side-status LED in this build.
The reconnect workaround deliberately uses PA12 (USB D+) and leaves all three
of those pins alone.

#### Blue Pill clone bootloader compatibility note

The development controller is an AliExpress Blue Pill clone. Its original
bootloader did not provide working DFU behavior, so STM32duino `boot20`
bootloaders were tested during diagnosis. The three images tried were:

* `generic_boot20_pb2.bin`
* `generic_boot20_pb12.bin`
* `generic_boot20_pc13.bin`

These filenames describe the status-LED GPIO used by each bootloader build;
they do not select the USB disconnect pin. Generic STM32F103 bootloaders use
PA12 (USB D+) for USB re-enumeration. Bootloader selection remains specific to
the controller hardware, and a compatible `boot20` bootloader must already be
installed before the browser updater can use `1EAF:0003`, alternate interface
2. Installing or replacing that bootloader normally requires ST-Link/SWD or
another external programming method; flashing Replicazeron firmware through
alternate interface 2 does not overwrite the bootloader.

### OpenRGB setup

Use OpenRGB 1.0 or a current Pipeline build containing the QMK VialRGB
controller. In OpenRGB, open **Settings > QMK VialRGB Devices** and add:

| Name | USB VID | USB PID |
|------|---------|---------|
| Replicazeron | `4142` | `2305` |

Save the OpenRGB configuration and rescan devices. Registering the VID/PID in
OpenRGB and granting it control in the firmware are separate steps.
OpenRGB currently auto-registers only the VialRGB devices compiled into its
device list, so an unmodified installation needs this one-time manual entry.
After OpenRGB is confirmed under **RGB > Controller**, the OLED shows an
OpenRGB-active confirmation page with the VID and PID. Merely highlighting the
OpenRGB choice leaves the selector visible. The WebHID RGB card includes the
same setup micro-guide.

OpenRGB only takes ownership of the LEDs after **OpenRGB** is selected under
**RGB > Controller** on the OLED, or under **RGB lighting > Controller** in the
[WebHID Control Deck](https://qwuille.github.io/replicazeron_webhid/). The selection is stored in EEPROM:

* **Firmware** runs the saved onboard RGB Matrix effect and ignores VialRGB
  lighting writes. Vial remapping and configuration continue to work.
* **OpenRGB** accepts VialRGB lighting writes. OpenRGB is expected to remain
  running; if it closes, the LEDs intentionally retain the last frame it sent.

Switching back to **Firmware** restores the saved onboard effect. There is no
application detection, heartbeat, automatic timeout, or controller handoff.
The user explicitly chooses which controller owns the LEDs.

Vial's HID protocol remains available in both controller states. Mapping and
WebHID configuration traffic temporarily receives priority over VialRGB:
lighting packets are ignored without a reply, current colors remain visible,
and OpenRGB communication resumes after five seconds without configuration
traffic. This prevents most cross-application reply collisions. Closing
OpenRGB before opening Vial remains the most conservative option because both
applications still share one Raw HID interface. OpenRGB can discover the
device in either state, although its lighting writes are accepted only in
OpenRGB mode.

### Firmware RGB effects

Firmware mode includes Breathing, Rainbow, Swirl, Knight Rider, Twinkle,
Moving Rainbow, Hue Wave, Hue Pendulum, Cylon, Pulse, and Reactive Pulse.
Knight Rider uses a constant-speed KITT-style scanner with a trailing fade;
Cylon uses a compact eye that eases at each end of its travel.

Reactive Pulse rests at a very dim base color. One isolated button press sends
one wave from the center toward both ends. A burst of distinct presses reverses
the current wave without resetting its position, then repeats the inward and
outward pattern until the presses become idle. Its speed is based on the saved
base speed plus a decaying average of press frequency and actual joystick
movement. Holding a button or holding the stick still does not add activity.

On the animation settings screen, use Up/Down to select Brightness, Speed, or
Hue, and Left/Right to change the selected value. Hue is offered only for
effects that use a chosen base color. Predefined rainbow and random-color
effects show `Hue: automatic`. The WebHID page follows the same rule.

The two side status LEDs have a separate persistent `SIDELED` menu. `Enabled`
turns both indicators on or off, and `Bright` sets their shared maximum
brightness in 15 steps. `Left LED` and `Right LED` independently select Off, stick
strength, buttons held, combined activity, always on, Caps Lock, Num Lock,
or Scroll Lock. The defaults retain
the original behavior: the left LED shows stick strength and the right LED shows buttons.
The same controls are available under **Side indicators** in the WebHID Control
Deck; they do not change the addressable RGB strip. A short
full-brightness preview follows a setting change so its result is visible even
when the selected source is inactive.

Factory reset is guarded against accidental presses. Select `RESET` in the
OLED settings menu and hold the top little-finger and top index-finger keys
together for two seconds. A legacy `EE_CLR` mapping only opens this
confirmation screen; it never clears EEPROM by itself.

OpenRGB application effects are rendered by OpenRGB and streamed through
VialRGB Direct mode; adding those effects does not require a firmware change.

The two monochrome side indicators remain separate GPIO LEDs controlled by
their firmware-selected activity sources. They are not exposed to VialRGB or
OpenRGB. OpenRGB controls only the addressable strip, and its writes are
accepted only while the firmware RGB controller is set to **OpenRGB**.

The WebHID RGB card stores an addressable LED count from 1 to 32; the default
is 11. Firmware effects and OpenRGB expose only that many strip pixels. Restart
or rescan OpenRGB after changing
the count because it reads the device topology when connecting. This build is
for 800 kHz, GRB-order, three-channel WS2812-compatible pixels, including most
WS2812B and compatible SK6812 RGB strips. RGBW pixels, another byte order, or a
different signaling rate require a matching firmware build rather than a safe
runtime switch.

### What this update adds

* VialRGB and OpenRGB control for 1-32 WS2812-compatible LEDs over one HID
  interface.
* A persistent Firmware/OpenRGB ownership setting on the OLED and WebHID page.
* Ten independently remappable layouts plus an editable Settings tool layer.
* Per-layout Analog, WASD, and WASD + Shift thumbstick modes.
* Editable OLED layout titles, names for Macro 0-15, deadzone, and axis filtering.
* Vial-compatible recorded macro sequences plus optional firmware-side fixed
  or randomized automatic delays between their key actions.
* Persistent enable, brightness, active-low/active-high wiring, and independent
  signal-source controls for both side LEDs on the OLED and WebHID page.
* A two-button, two-second factory-reset confirmation on the OLED.
* RGB and both side status LEDs shut down with USB suspend and restore on wake.

The side-indicator brightness range starts at 34/255. Lower values do not
retain enough PWM resolution to show useful stick-strength variation. Wiring
polarity is stored per device so original active-low and modified active-high
builds can use the same firmware.

## Layout key

The physical `lyr` key is owned by the firmware, independent of its Vial
mapping. Tap it to advance to the next layout. Hold it for at least 500 ms to
cycle the current layout through Joystick, WASD, and WASD + Shift modes. Each
layout's selection is saved in EEPROM.

The OLED `MODE` menu first asks which layout to edit. The same per-layout modes
can be read and written with the standalone WebHID Control Deck.

Joystick profiles report the two analog HID axes. WASD and WASD + Shift
profiles center those HID axes and translate the physical stick into keyboard
input instead, avoiding simultaneous joystick movement in games that listen to
both device types.

On Settings, the thumbstick scrolls horizontally and vertically by default and
its analog game-controller axes are centered. Scroll rate follows stick
strength. The default Settings mouse-toggle key switches between scrolling and
regular cursor movement until Settings is left; cursor speed also follows stick
strength. The default finger keys provide cut/copy/paste, undo/redo, save,
find, browser back/forward, tabs, mouse buttons, and common navigation.
Those finger keys remain remappable. The five-way D-pad is reserved for OLED
navigation while the menu is open, and the physical layout key remains owned
by firmware.

## Persistent configuration

Vial mappings, macros, layout titles, per-layout joystick modes, calibration,
and RGB settings are stored in EEPROM. Compiled entries are used only when the
EEPROM is new, explicitly cleared, or its storage layout is incompatible. A
normal firmware reflash keeps the configured values instead of replacing them
with the hardcoded defaults. A full-chip erase performed by an external
programmer will still erase the STM32's flash-backed EEPROM.

Only the ten playable layout titles are editable. The Settings tool layer
always keeps the fixed name `Settings`, but its non-navigation keys remain
remappable. Per-layout joystick modes, side LED
wiring metadata, and the 16 macro names use a reserved EEPROM tail area so
they cannot overlap Vial's dynamic keymap. Layout titles remain in Vial's
custom EEPROM area, and WebHID selections persist
across reconnects and firmware restarts.

The first firmware containing the reserved-tail fix detects the legacy `J2` or
`J3` metadata signature. Older builds placed those 13 metadata bytes over the
start of Layout 0, so migration restores the affected first seven key
positions from the compiled defaults while preserving the remaining layouts,
macros, and configuration. Reapply custom mappings for those seven positions
from a previous Vial backup if necessary.

The WebHID control deck reads the device immediately after connecting and uses
a responsive, image-free CSS preview that animates the selected firmware
effect across the 11-LED strip. Individual changes are written live
after a short debounce while the device is connected. **Write all** remains
available for imported settings and recovery.

OLED menus always use the physical D-pad directions regardless of Vial
remapping. While the `SIDELED` screen is open, both indicators illuminate as a
live brightness preview; Left/Right edits the selected value and Menu exits.

## Supported MCUs

Currently configs for STM32F103 and atmega32U4 are available, but STM32 is recommended, since the atmega may run out of flash with all features enabled.

The maintained STM32F103 Vial build disables QMK's Caps Word, Magic, Layer
Lock, Grave Escape, and Space Cadet subsystems to preserve flash for the
controller-specific features. Grave Escape and Space Cadet are compact-keyboard
conveniences rather than game-controller functions: Grave Escape combines
Escape with grave/tilde behavior, while Space Cadet makes tapped Shift keys
produce parentheses. Their Vial keycodes are therefore unavailable in this
firmware. Ordinary Escape, grave, Shift, and parenthesis keycodes continue to
work normally.

With minor adjustments to Pinconfig it should be possible to use other MCUs that are supported by QMK


## Wirering

Full schematics can be found in this repo:

https://github.com/9R/replicazeron_schematics

### Rows
|row| promiro gpios | promicro pin | stm32F103 gpios |   color |
|---|---------------|--------------|-----------------|---------|
| 0 |          B1   |          15  |          B15    |  red    |
| 1 |          B3   |          14  |           A8    |  blue   |
| 2 |          B2   |          16  |           A9    |  yellow |
| 3 |          B6   |          10  |          A10    |  brown  |
| 4 |          B5   |           9  |          A15    |  orange |
| 5 |          B4   |           8  |           B3    |  green  |

### Columns
|col| promiro gpios | promicro pin | stm32F103 gpios |  color  |
|---|---------------|--------------|-----------------|---------|
| 0 |         C6    |            5 |          A7     |  white  |
| 1 |         D4    |            4 |          A6     |  grey   |
| 2 |         D7    |            6 |          A5     |  violet |
| 3 |         E6    |            7 |          A4     |  grey   |
| 4 |         F7    |           A0 |          B4     |  white  |

### Analog
| promicro gpio | stm32F103 gpio | pin | color |
|---------------|----------------|-----|-------|
|          GND  |           GND  | GND | white |
|          VCC  |           VCC  | VCC | red   |
|          F4   |           B1   | VRx | brown |
|          F5   |           B0   | VRy | yellow|
|               |                | SW  | blue  |

### OLED
| promicro gpio | stm32F103 gpio | pin | color |
|---------------|----------------|-----|-------|
|          GND  |           GND  | GND | white |
|          VCC  |           VCC  | VCC | red   |
|           D4  |           B10  | SDA | green |
|           C6  |           B11  | SCL | yellow|
