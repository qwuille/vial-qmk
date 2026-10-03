
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

Some additional and redesigned files (i.e. for the OLED-display) can be found in [this repo](https://github.com/9R/replicazeron).

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

Live Input and Combined OLED pages refresh at 20 frames per second. This keeps
their changing stick and key values responsive without continuously occupying
the I2C bus and disturbing the loop-timed side-LED brightness control.

## Vial and OpenRGB

This update combines the complete Replicazeron configuration with VialRGB and
OpenRGB support. It keeps all 11 Vial layout slots, macros, editable layout
titles, per-layout joystick modes, OLED settings, WebHID configuration, and
normal Vial remapping.

The optional Vial Key Override engine and developer HID Console remain
disabled. On the STM32F103 build, NKRO and several additional QMK conveniences
are also disabled to leave flash space for the Replicazeron OLED, WebHID,
macro, joystick, lighting, and host-traffic indicator features. The standard
Blue Pill keyboard report still supports six simultaneous keyboard keys;
joystick and mouse reports remain separate. Ordinary remapping, layer keys,
the Vial macro engine, and all 11 layout slots remain available. Tap Dance is
disabled on the Blue Pill to provide enough flash for adjustable analog
smoothing. The RP2040 build restores Tap Dance, NKRO, Repeat Key, Caps Word,
Magic keycodes, Layer Lock, Grave Escape, and Space Cadet.

OpenRGB communicates through VialRGB on the existing 32-byte Vial Raw HID
interface. There is no bridge program and no second OpenRGB HID interface, so
the keyboard appears only once in Vial.

Build the combined firmware with:

```sh
qmk compile -c -kb handwired/replicazeron/stm32f103 -km vial
```

For a standard Raspberry Pi Pico wired according to
[HARDWARE.md](HARDWARE.md), build the RP2040 UF2 with:

```sh
qmk compile -c -kb handwired/replicazeron/rp2040 -km vial
```

> [!WARNING]
> The RP2040 firmware, XInput interface, and browser-flashing path compile but
> have not yet been verified on physical RP2040 hardware. Treat the UF2 as
> experimental and keep BOOTSEL recovery available. The STM32F103 build is the
> validated release target.

Hold BOOTSEL while connecting the Pico and copy the generated UF2 to its
`RPI-RP2` drive for manual recovery or the first update from older firmware.
Do not use the STM32 `.bin` or STM32duino DFU transport on the Pico.

### Browser firmware update

The WebHID configurator reads the running firmware's controller identity and
selects the appropriate WebUSB installer automatically: STM32duino DFU
(`1EAF:0003`, alternate interface 2) for Blue Pill, or the RP2040 ROM PICOBOOT
interface (`2E8A:0003`) for a Pico UF2.

Open the standalone
[Replicazeron WebHID Control Deck](https://qwuille.github.io/replicazeron_webhid/)
in Chrome or Edge, connect the running Replicazeron, and use the numbered controls under **Firmware
update**. Select the requested Replicazeron `.bin` or `.uf2`, enter bootloader
mode, explicitly grant access to the bootloader, and confirm the flash. The page
validates the STM32 image size/vector table or the RP2040 UF2 family,
completeness, and flash range before enabling the write button. Keep manual
DFU, SWD, or BOOTSEL flashing available as a recovery path.

After identifying the controller, the page loads the matching firmware sourced
from the latest GitHub release and verifies its published size and SHA-256
digest. The Pages deployment performs the same checks before placing the asset
beside the site, avoiding GitHub release-redirect CORS restrictions without
committing binaries to the WebHID repository. The verified file can also be
downloaded from the page for manual recovery; selecting a local file is no
longer required.

Older RP2040 firmware predates the controller-identity reply used for automatic
selection. Install the current UF2 once by holding BOOTSEL while connecting USB
and copying it to `RPI-RP2`. After that one manual update, the page recognizes
the Pico and uses PICOBOOT automatically. Older firmware without the identity
reply falls back to the historical Blue Pill updater for compatibility.

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

Firmware mode includes Solid Reactive, Splash, Multisplash, Solid Reactive
Cross, and Solid Reactive Wide. Key presses are mapped to the nearest point on
the horizontal LED strip, so these effects spread from the relevant finger or
thumb group rather than from an unrelated pixel. LED 1 is treated as the
index-finger/`COL_3` end of the strip; the chain then runs toward middle, ring,
and little finger. Thumb controls use that same nearest endpoint.

The RP2040 build also includes Breathing, Rainbow, Swirl, Knight Rider,
Twinkle, Moving Rainbow, Hue Wave, Hue Pendulum, Cylon, Pulse, and Reactive
Pulse. The flash-limited Blue Pill retains the standard effects but omits the
four larger Replicazeron-only effects: Knight Rider, Cylon, Pulse, and Reactive
Pulse.
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
Scroll Lock, OpenRGB traffic, Vial/WebHID traffic, or Host control. OpenRGB and
Vial/WebHID traffic can be monitored separately. Host control stays steadily
lit for active OpenRGB frames and blinks while Vial or WebHID temporarily has
configuration priority. It turns off when neither protocol is active. The defaults retain
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
their firmware-selected activity sources. They are not exposed as VialRGB or
OpenRGB lighting endpoints, although they can indicate current host-protocol
traffic. OpenRGB controls only the addressable strip, and its writes are
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
* Per-layout WASD and configurable faux-analog keyboard modes on both targets,
  plus Analog and XInput + keys modes on RP2040.
* Five independently selectable OLED designs for every playable layout: Input
  monitor, Macro focus, Game status, Combined, and Minimal.
* Editable OLED layout titles, names for Macro 0-15, deadzone, axis filtering,
  and analog smoothing.
* Persistent OLED shutdown and sleeping-logo interval controls without reducing
  Vial's macro buffer.
* Vial-compatible recorded macro sequences plus optional firmware-side fixed
  or randomized automatic delays between their key actions.
* Persistent enable, brightness, active-low/active-high wiring, and independent
  signal-source controls for both side LEDs on the OLED and WebHID page.
* A two-button, two-second factory-reset confirmation on the OLED.
* RGB and both side status LEDs shut down with USB suspend and restore on wake.

The configurable deadzone also applies to the analog HID report. Values inside
it are held at the exact center to eliminate small ADC drift. Values outside it
are linearly rescaled, so movement begins smoothly at zero and can still reach
the full axis range. The same setting is available in WebHID and under
**Calibration → Deadzone** on the OLED; no additional EEPROM field is needed.
The reported axes also use persistent, adjustable fixed-point smoothing to
reduce ADC jitter on sticks of varying quality. Off, Light, Balanced, Strong,
and Maximum trade progressively steadier output for additional response delay;
Balanced preserves the previous fixed smoothing behavior. The same setting is
available in WebHID and under **Calibration → Smoothing** on the OLED. **Axis
filter** remains a separate directional aid that suppresses unintended
diagonal movement. RP2040 XInput now receives the configured deadzone as well
as smoothing instead of forwarding center noise around the HID path.

The side-indicator brightness range starts at 34/255. Lower values do not
retain enough PWM resolution to show useful stick-strength variation. Wiring
polarity is stored per device so original active-low and modified active-high
builds can use the same firmware.

## Layout key

On the ten playable layers, the physical `lyr` key is owned by the firmware,
independent of its Vial mapping. Tap it to advance to the next layout. Hold it
for at least 500 ms to cycle the current layout through its available stick
modes. Blue Pill Standard
cycles between WASD and Faux analog; the DirectInput compatibility variant also
offers Joystick. RP2040 offers all three plus XInput + keys.
Each layout's selection is saved in EEPROM.

The OLED `MODE` menu first asks which layout to edit. Its `Settings tools`
entry controls only the Settings layer and cycles between scroll/cursor and the
pointer-independent Fusion/FreeCAD CAD bridge. The same choice is available in
the standalone WebHID Control Deck. Selecting Settings releases and suppresses
any WASD/Faux keys from the playable layer underneath it. The five-way remains
permanently firmware-owned on Settings and is not assigned a CAD action.
In CAD bridge mode, holding the default Settings mouse-mode key momentarily
switches the stick from orbit to pan; releasing it returns to orbit. Vial's User
tab also provides **CAD Pan** and **CAD Rotate**, which can be assigned to any
remappable keys. CAD Rotate keeps the camera position and target fixed while the
horizontal stick axis rolls the view. The Fusion and FreeCAD add-in settings independently select
momentary or toggle behavior for each key.

The OLED `SCREEN` menu similarly chooses a playable layout and cycles its
display design. Input monitor shows live stick direction, strength, and held
key count. Macro focus shows the last invoked macro and its assigned name. Game
status shows macro usage and the last macro. Combined puts live input and macro
status together; this replaces the less useful RGB-status concept. Minimal
keeps only the layout title and input mode. The WebHID layout rows provide CSS
previews for both the selected thumbstick mode and OLED design.

Faux analog remains entirely on the keyboard interface. A disabled Walk and
Run binding behaves like ordinary WASD. Defining only Walk produces Walk/Pace;
defining only Run produces Pace/Run; defining both produces Walk/Pace/Run.
WebHID selects each key and both stick-strength transition points. Games must
provide suitable keyboard bindings for the chosen Walk and Run keys.

The STM32 USB HID/DirectInput joystick-only gaming mode was removed from the
recommended Standard variant
after testing showed that combining its gamepad reports with the controller's
normal keyboard input made games repeatedly switch between gamepad and keyboard
HUDs. That transition was not smooth and could cause visible hesitation while
playing. STM32 now exposes keyboard gaming modes only, while retaining the
physical stick's internal analog sampling, deadzone, filtering, smoothing,
Settings-layer proportional scroll/cursor control, and the optional
Fusion/FreeCAD camera bridge.
Existing STM32 Joystick layouts migrate to WASD after this update.

A separately published DirectInput compatibility variant restores the HID
joystick and all 32 Vial-bindable gamepad buttons without removing any current
feature. Build it by adding `-e REPLICAZERON_STM32_DIRECTINPUT=yes` to the STM32
QMK command. It leaves only a few hundred application-flash bytes free in the
verified build, so it
is feature-frozen: maintenance corrections remain possible, but new features
target Standard and RP2040. WebHID identifies the running variant and hardware
revision, defaults to its matching update channel, and offers an explicit STM32
variant toggle before flashing.

Joystick profiles on STM32 DirectInput and RP2040 report two HID axes and 32
bindable DirectInput buttons. RP2040's XInput mode is the recommended controller path for games that
support simultaneous keyboard and controller input.

The RP2040-only **XInput + keys** mode sends the thumbstick as the XInput left
stick and centres the normal HID joystick report. Vial's User tab labels
Gamepad Buttons 1-17 with A, B, X, Y, D-pad, bumpers, triggers, Back, Start,
stick-click, and Guide meanings; no extra layer is used. The same assignments
emit numbered DirectInput buttons in Analog mode. Ordinary QMK keycodes on the
same layout remain keyboard keys, allowing either pure controller assignments
or an intentional hybrid. Gamepad assignments are inert in the two keyboard
emulation modes but remain stored, so changing modes never erases them.

On Settings, the thumbstick scrolls horizontally and vertically by default.
DirectInput and RP2040 Joystick builds also report their game-controller axes;
Standard keeps the same analog sampling internal. Scroll rate follows a gentle strength curve for precise
movement near center without excessive full-deflection scrolling. The default
Settings mouse-toggle key switches between scrolling and regular cursor movement
until Settings is left; cursor speed also follows stick strength. Thumbstick
movement counts as input activity and wakes the OLED. The default finger keys
provide cut/copy/paste, undo/redo, save, find, browser back/forward, tabs,
mouse buttons, and common navigation.
Those finger keys remain remappable. On Settings, every position except the
five-way D-pad is remappable, including the physical layout-key position. The
five-way centre opens the OLED menu from any layer; while the overlay is open,
all five directions are firmware-owned. Closing it restores the selected OLED
design for the active playable layer. The physical layout key remains
firmware-owned only on the ten playable layers.

## Persistent configuration

Vial mappings, macros, layout titles, per-layout joystick and OLED modes,
calibration, and RGB settings are stored in EEPROM. Compiled entries are used only when the
EEPROM is new, explicitly cleared, or its storage layout is incompatible. A
normal firmware reflash keeps the configured values instead of replacing them
with the hardcoded defaults. A full-chip erase performed by an external
programmer will still erase the STM32's flash-backed EEPROM.

Only the ten playable layout titles and OLED designs are editable. The Settings tool layer
always keeps the fixed name `Settings`, but its non-navigation keys remain
remappable. Per-layout joystick and OLED modes, side LED
wiring metadata, and the 16 macro names use a reserved EEPROM tail area so
they cannot overlap Vial's dynamic keymap. Layout titles remain in Vial's
custom EEPROM area, and WebHID selections persist
across reconnects and firmware restarts.

The OLED design uses otherwise unused bits in each existing per-layout mode
byte, and live macro statistics are calculated from Vial's existing macro
buffer. No macro slots or macro-buffer bytes are consumed by this feature.

The first firmware containing the reserved-tail fix detects the legacy `J2` or
`J3` metadata signature. Older builds placed those 13 metadata bytes over the
start of Layout 0, so migration restores the affected first seven key
positions from the compiled defaults while preserving the remaining layouts,
macros, and configuration. Reapply custom mappings for those seven positions
from a previous Vial backup if necessary.

The WebHID control deck reads the device immediately after connecting and uses
responsive, image-free CSS previews for the selected stick mode, OLED design,
and firmware effect across the 11-LED strip. Its visual macro editor includes
a clickable CSS keyboard for quickly appending familiar keys. Individual changes are written live
after a short debounce while the device is connected. **Write all** remains
available for imported settings and recovery.

OLED menus always use the physical D-pad directions regardless of Vial
remapping. While the `SIDELED` screen is open, both indicators illuminate as a
live brightness preview; Left/Right edits the selected value and Menu exits.

## Supported MCUs

The complete shared firmware has targets for STM32F103 and Raspberry Pi Pico
(RP2040). The Pico target has substantially more flash headroom but still needs
physical validation.

This version does **not** support the standard ATmega32U4 Pro Micro, and the
obsolete controller definition has been removed. The current compact firmware
exceeds the Pro Micro's Caterina application space by about
12.5 KB. Its 1 KB EEPROM also cannot simultaneously hold the current Vial
layers, macros, profile names, and Replicazeron metadata. A build that fits
would have to omit most of the project's distinctive features, including the
OLED interface and addressable RGB/OpenRGB, and reduce or redesign persistent
configuration. New and upgraded builds should use the STM32F103 Blue Pill or
RP2040 target instead.

The maintained STM32F103 Vial build disables QMK's Repeat Key, Caps Word,
Magic, Layer Lock, Grave Escape, Space Cadet, and NKRO subsystems to preserve
flash for the controller-specific features. The RP2040 Vial build enables all
of those features. Grave Escape and Space Cadet are compact-keyboard
conveniences: Grave Escape combines Escape with grave/tilde behavior, while
Space Cadet makes tapped Shift keys produce parentheses. Their Vial keycodes
are therefore available on RP2040 but not on the Blue Pill. Ordinary Escape,
grave, Shift, and parenthesis keycodes continue to work normally on both.

Other QMK-supported MCUs require their own pin configuration, resource review,
firmware target, and hardware validation; compatibility should not be assumed.


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
