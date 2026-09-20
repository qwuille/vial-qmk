# Replicazeron firmware hardware guide

This document describes the hardware assumptions made by Qwuille's complete
STM32F103 Vial firmware. Replicazeron builds are hand-wired and commonly vary,
so verify every connection against both your controller board and the firmware
configuration before applying power.

This repository does not distribute printable models. See the separate
[Incedius](https://github.com/incedius/replicazeron) and
[9R](https://github.com/9R/replicazeron) hardware projects for existing model
work and earlier build documentation.

## Component checklist

| Component | Maintained firmware expectation | Notes and alternatives |
|---|---|---|
| Controller | STM32F103C8/CB Blue Pill-class board | Genuine and clone boards differ. The complete release is tested on an AliExpress clone. Check flash size, USB pull-up behavior, pin labels, and bootloader before assembly. |
| Bootloader | STM32duino `boot20`, application at `0x08002000` | Required for the documented `1EAF:0003` DFU path. Installing the bootloader initially normally requires ST-Link/SWD or another external programmer. |
| Display | SSD1306-compatible 128x32 monochrome OLED, I2C | A 128x64 module needs housing and firmware/UI changes and is not the maintained display target. Confirm module voltage and pin order; OLED breakout pin orders are not universal. |
| Main lighting | 1-32 three-channel WS2812-compatible addressable RGB pixels | Default 11. Firmware signaling is 800 kHz in GRB order. WS2812B and compatible SK6812 RGB pixels are typical. RGBW, different byte orders, or different signaling rates need a matching build. |
| Side indicators | 2 monochrome LEDs or compatible LED modules | Both are on the controller's right side: PB13 drives the physically left indicator and PB12 drives the physically right indicator. Runtime polarity supports active-high and active-low wiring. Bare LEDs require suitable series resistors. |
| Thumbstick | Two-axis analog joystick with optional push switch | Analog outputs must remain within the STM32 ADC input range. Do not feed a 5 V analog output into the MCU. The push switch is wired as a matrix key. |
| Switches | Momentary switches for the finger keys, D-pad, and auxiliary positions | The firmware matrix provides 30 positions. The number physically populated depends on the build. |
| Matrix diodes | One signal diode per populated switch, commonly 1N4148 | Firmware diode direction is `COL2ROW`; diode orientation must match the wiring. |
| Wiring and interconnects | Insulated wire, connectors or headers, strain relief, and insulation | Dupont-style connections and small protoboard are common, but permanent builds need mechanically secure, insulated connections. |
| USB cable/connector | Data-capable USB connection suitable for the selected Blue Pill | Charge-only cables cannot configure or flash the device. Some clones need a physical reconnect after DFU flashing. |
| Power | Regulated supply appropriate for the MCU, OLED, joystick, and LEDs | Addressable LEDs can exceed ordinary USB current budgets. See the power section before selecting pixel count or brightness. |

Mechanical fasteners, printed parts, switch hardware, caps, adhesives, and
cable routing depend on the model used and are intentionally outside the
firmware repository's bill of materials.

## STM32F103 pin map

### Switch matrix

The matrix is 6 rows by 5 columns with `COL2ROW` diodes.

| Function | MCU pins |
|---|---|
| Rows 0-5 | PB15, PA8, PA9, PA10, PA15, PB3 |
| Columns 0-4 | PA7, PA6, PA5, PA4, PB4 |

PA15, PB3, and PB4 are normally JTAG pins on STM32F1 devices. QMK's board
configuration must leave them available as GPIO; do not attach a debugger that
continues to claim those pins while testing the matrix.

### Analog stick

| Signal | MCU pin |
|---|---|
| Firmware X axis | PB0 / ADC input |
| Firmware Y axis | PB1 / ADC input |
| Stick push switch | A selected row/column matrix position |
| Ground | GND |
| Supply | Use a voltage that keeps both analog outputs within the STM32 ADC range |

Some older harness diagrams name the potentiometer outputs or axes in the
opposite order. The authoritative mapping for this build is `PB0 = X` and
`PB1 = Y`; orientation can then be corrected through the firmware rotation
setting.

### OLED

| I2C signal | MCU pin |
|---|---|
| SCL | PB10 |
| SDA | PB11 |
| Ground | GND |
| Supply | Match the OLED module's supported voltage |

The firmware uses ChibiOS `I2CD2`. Check the silkscreen on the actual OLED;
modules frequently use different VCC/GND/SCL/SDA connector orders.

### Lighting and indicators

| Function | MCU pin | Electrical behavior |
|---|---|---|
| Addressable RGB data | PB14 | TIM1 complementary PWM output with DMA, 800 kHz WS2812-compatible stream |
| Left side indicator | PB13 | Independent software-PWM GPIO; runtime active-high/active-low setting |
| Right side indicator | PB12 | Independent software-PWM GPIO; runtime active-high/active-low setting |

"Left" and "right" identify the two adjacent indicators as viewed on the
right side of the assembled controller. They replace the earlier ambiguous
upper/lower terminology.

If a bare monochrome LED is connected directly, calculate and fit an
appropriate current-limiting resistor. If a transistor or LED module is used,
the active-low setting may be required. Never use the firmware polarity option
as a substitute for checking the circuit.

### USB and bootloader-related pins

| Function | MCU pin or identifier |
|---|---|
| USB D- | PA11 |
| USB D+ / reconnect assist | PA12 |
| Replicazeron USB VID:PID | `4142:2305` |
| STM32duino DFU VID:PID | `1EAF:0003` |
| DFU application interface | Alternate interface 2 |
| Firmware application origin | `0x08002000` |

The WebHID reset option called **Blue Pill USB reconnect assist** temporarily
manipulates PA12 before reset. It does not change BOOT0/BOOT1 wiring and cannot
install a missing bootloader.

## Addressable RGB selection

The saved **Addressable LEDs** setting accepts 1-32 pixels and defaults to 11.
It changes how many physical strip pixels firmware effects and VialRGB expose.
Two additional virtual VialRGB endpoints follow the strip and drive the left
and right monochrome indicators; they are not addressable pixels on the strip.

Changing the runtime count cannot change the electrical protocol. Recompile if
the installed pixels require RGBW data, a color order other than GRB, or a rate
other than 800 kHz. Rescan OpenRGB after changing the count because it reads
the topology when the device connects.

For reliable WS2812 signaling:

- Keep data wiring short and routed away from noisy switch or power wiring.
- Connect the LED supply ground and controller ground together.
- Follow the pixel manufacturer's recommendation for data-line resistance and
  local supply decoupling.
- A 5 V pixel may not reliably recognize a 3.3 V data-high level in every
  electrical environment. Use an appropriate logic-level shifter when needed.
- Do not inject external LED power in a way that backfeeds the computer's USB
  port or an unpowered controller.

## Power budget

Do not size the supply from the normal animation appearance. A conventional
RGB pixel can approach roughly 60 mA at full-white maximum output. Eleven such
pixels can therefore approach 660 mA before adding the controller, OLED,
joystick, and side LEDs; 32 pixels can approach 1.92 A. Actual pixels vary, but
those figures are useful worst-case planning values.

Limit brightness and use a properly rated supply, wiring, connector, fuse, and
common ground. Larger strips or unrestricted full-white OpenRGB output may
require a separate regulated LED supply with safe power-injection wiring. If
you are not confident about preventing USB backfeed, obtain help with the power
circuit before connecting it to a computer.

## Bootloader compatibility

The development board is an AliExpress Blue Pill clone whose original
bootloader did not work with the required DFU workflow. These STM32duino
`boot20` variants were tried during testing:

- `generic_boot20_pb2.bin`
- `generic_boot20_pb12.bin`
- `generic_boot20_pc13.bin`

The suffix selects the bootloader's status-LED pin; it does not select the USB
reconnect pin. Board clones can use different onboard LEDs, USB circuitry,
flash devices, or MCU-compatible parts, so no one bootloader image can be
promised for every board sold as a Blue Pill.

Both firmware reset choices successfully entered DFU on the development board.
After browser flashing, that clone may still need its reset button or a USB
disconnect/reconnect to start the application. A successful WebDFU progress bar
does not prove that the bootloader can perform the final USB re-enumeration.

## Feature support by controller

| Controller definition | Status in this repository |
|---|---|
| `handwired/replicazeron/stm32f103` | Maintained target for Vial, VialRGB/OpenRGB, OLED, WebHID configuration, PWM/DMA strip output, and side indicators |
| Pro Micro / ATmega32U4 | Legacy definition; limited flash and GPIO prevent assuming feature parity |
| RP2040 | Experimental definition; pinout and lighting configuration differ and the complete documented firmware has not been validated on it |

Do not flash an STM32F103 binary onto another controller family. Builders who
change pins, display type, LED protocol, matrix wiring, or controller must make
and test a corresponding firmware target.

## Firmware references

The authoritative configuration files are:

- [`stm32f103/keyboard.json`](stm32f103/keyboard.json) for matrix, WS2812, and
  device-version configuration.
- [`stm32f103/config.h`](stm32f103/config.h) for OLED, side LED, analog, and
  PWM/DMA pins.
- [`info.json`](info.json) for USB identity and physical layout.
- [`keymaps/vial/rules.mk`](keymaps/vial/rules.mk) for the enabled Vial and
  VialRGB feature set.

When this guide and an older external wiring diagram disagree, check the source
for the firmware version actually being built and verify the hardware with a
meter before applying power.
