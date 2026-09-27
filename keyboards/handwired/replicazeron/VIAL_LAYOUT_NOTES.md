# Vial Layout Notes

## Replicazeron Files Changed

- `keymaps/vial/keymap.c`
  - Defines ten selectable layouts plus the settings layer.
  - Uses `TO()` for layout transitions so exactly one layout is active.
  - Assigns the physical 5-way D-pad to OLED menu navigation; its center opens or selects menu items.
  - Provides direct RGB, mode, autorun, reset, and boot controls.
  - RGB menu: choose Animated or Static.
  - RGB Controller selects either persistent onboard Firmware effects or
    OpenRGB control through VialRGB on the same Raw HID interface.
  - Static RGB: Up/Down changes brightness, Right changes hue, and center toggles RGB.
  - Animated RGB: browse Breathing, Rainbow, Swirl, Knight, and Twinkle. Selecting an effect opens its settings, where Up/Down changes brightness, Left/Right changes the actual QMK animation-speed variant, and center returns to the effect browser.
  - The standalone [WebHID Control Deck](https://github.com/qwuille/replicazeron_webhid) can read and write RGB enabled state, mode, animation, brightness, speed, and hue.
  - Calibration menu: Deadzone displays the live unfiltered stick distance on a center-to-outer gauge and saves on the next gamepad key press.
  - Axis Filter suppresses a small secondary axis relative to the dominant axis. Up/Down adjusts its persistent strength from 0 to 100%, center saves, and Left cancels.
  - Smoothing provides five persistent levels from Off through Maximum. The
    default Balanced level retains the previous low-pass behavior; stronger
    levels trade some response time for greater resistance to noisy sticks.
  - The WebHID Control Deck can read and write axis-filter strength and
    smoothing, and import/export both settings as JSON.
  - RP2040's HID Game Pad report exposes two analog axes and 32 buttons.
    Vial's **User** palette provides **Gamepad Button 1** through **Gamepad
    Button 32** for DirectInput assignments while other positions remain
    ordinary keyboard bindings. STM32 Standard deliberately exposes no USB gamepad: its
    former HID/DirectInput joystick gaming mode caused games to switch
    repeatedly and unsmoothly between gamepad and keyboard HUD/input prompts.
    Removing that USB report does not remove the internal analog sampling used
    for proportional scrolling, cursor control, or the Fusion/FreeCAD bridge.
  - The separately released STM32 DirectInput compatibility build restores the
    HID joystick and this gamepad palette. It retains the present feature set
    but is feature-frozen because only a few hundred application-flash bytes
    remain in the verified build.
  - On RP2040, Gamepad Buttons 1-17 are labelled with their additional XInput
    meanings. A saved binding emits a numbered DirectInput button in Analog
    mode or its labelled Xbox control in XInput mode.
  - Mode menu: STM32 Standard layouts cycle WASD and Faux analog. STM32
    DirectInput adds Joystick; RP2040 also adds XInput + keys. Faux analog uses configurable Walk and Run keys and
    strength thresholds without exposing a gamepad. Settings tools provide
    proportional scroll/cursor or the pointer-independent CAD bridge on both
    targets. The Settings five-way remains permanently firmware-owned.
  - Screen menu: choose one of the ten playable layouts and cycle Input
    monitor, Macro focus, Game status, Combined, or Minimal OLED designs.
    Combined uses its fourth row for the last macro instead of RGB status.
  - Display menu: sets the OLED shutdown delay in 30-second steps or Never,
    and the sleeping-logo interval in one-minute steps or Disabled.
  - Calibration continuously samples the thumbstick; any gamepad key saves the displayed deadzone.
- `keymaps/vial/rules.mk`
  - Sets `DYNAMIC_KEYMAP_LAYER_COUNT = 11`.
  - Reserves 145 bytes of VIA custom configuration for 11 fixed-width titles and a storage signature.
- `keymaps/vial/vial.json`
  - Defines the physical six-row, five-column keyboard matrix.
- `replicazeron.c`
  - Stores title data through a private Raw HID command.
  - Uses the custom-data signature to migrate Vial's per-build EEPROM marker so normal firmware reflashes preserve dynamic mappings, macros, and settings.
  - Reserves the EEPROM tail for joystick metadata and 16 macro names; this
    avoids overlapping Layout 0 while keeping the existing keymap start address.
  - Packs each layout's OLED design into unused bits of its existing joystick
    mode byte, so the feature consumes no Vial macro-buffer space.
  - Packs smoothing into unused bits of the Settings-stick byte, preserving
    the existing EEPROM and Vial macro capacity while migrating older `R9`
    metadata to the current `RA` format.
  - Migrates the legacy `J2`/`J3` metadata and restores its seven overwritten
    Layout 0 positions from compiled defaults.
  - Seeds default titles:
    `Casual`, `Shooter`, `Misc`, `Empty 3` through `Empty 9`, and `Settings`.
- `common/oled.c`
  - Renders title data stored in RAM.
  - Renders five selectable per-layout status designs and live key, stick, and
    macro information.
  - Shows a roughly 1.9-second Replicazeron ripple logo at boot.
  - Defaults to shutting down after one idle minute and waking for the logo
    every three minutes while asleep. Both timers are configurable from the
    OLED or WebHID, and key or menu activity wakes the display immediately.
- `common/leds.c`
  - Uses hardware PWM on the STM32 PB13 status LED to show unfiltered joystick distance: dim at center and smoothly brighter toward the outer edge.
  - Illuminates the PB12 status LED while any physical matrix switch is held, including switches currently mapped to `KC_NO`.
- [Replicazeron WebHID Control Deck](https://github.com/qwuille/replicazeron_webhid)
  - Provides the separately maintained browser editor for titles, joystick
    modes, graphical OLED designs, input tuning, lighting, side indicators,
    JSON import/export, and WebUSB DFU. Its macro editor uses an image-free CSS
    keyboard for quickly building sequences.

## Using Layer Titles

The physical keyboard is a six-row, five-column matrix. No virtual matrix row is used.

Open **https://qwuille.github.io/replicazeron_webhid/** in a compatible Chromium browser, connect the controller, and edit each title and the deadzone. The valid deadzone range is `1` through `349`. The editor uses private Raw HID command `0x70`; title data is stored separately from Vial keymaps. Export and import its JSON file alongside a Vial layout backup.

After flashing firmware that includes this feature, reset the Vial dynamic keymap once to initialize the default titles.
