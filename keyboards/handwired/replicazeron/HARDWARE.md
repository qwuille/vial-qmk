# Replicazeron firmware hardware guide

This document describes the hardware assumptions made by the complete
STM32F103 and Raspberry Pi Pico Vial firmware. Replicazeron builds are
hand-wired and commonly vary, so verify every connection against both your
controller board and the firmware configuration before applying power.

This repository does not distribute printable models. See the separate
[Incedius](https://github.com/incedius/replicazeron) and
[9R](https://github.com/9R/replicazeron) hardware projects for existing model
work and earlier build documentation.

The maintained three-page
[Raspberry Pi Pico wiring diagram](../../../docs/Replicazeron_RP2040_Wiring_Diagram.pdf)
shows the physical Pico pins, matrix diode direction, peripheral circuits,
power recommendations, signal labels, and staged bring-up checks. Wire colors
are intentionally left to each builder.

The browser-based
[interactive complete wiring guide](../../../docs/replicazeron-rp2040-wiring-diagram.html#interactive-matrix-guide)
combines the matrix, OLED, joystick and push switch, side indicators, layer
button, RGB strip, optional level shifter, and power paths in one node-flow
canvas. Every matrix cell and peripheral cable can be checked off and assigned
one or two wire colours. Progress persists locally and can be exported or
restored as JSON. The interactive page is intentionally excluded when the HTML
document is printed to PDF.

The validated STM32F103 wiring is available in the matching
[Blue Pill PDF](../../../docs/Replicazeron_STM32F103_Wiring_Diagram.pdf) and
[interactive HTML guide](../../../docs/replicazeron-stm32f103-wiring-diagram.html#interactive-matrix-guide).
Both HTML guides include light, dark, and system-theme choices plus direct PDF
downloads. The PDFs intentionally omit the interactive explorer so they remain
useful offline and on paper.

## Component checklist

| Component | Maintained firmware expectation | Notes and alternatives |
|---|---|---|
| Controller | STM32F103C8/CB Blue Pill-class board or Raspberry Pi Pico (RP2040) | The complete release is tested on an AliExpress STM32 clone. The Pico target compiles and awaits physical validation. Use the pin map for the selected target. |
| Bootloader | STM32duino `boot20` for Blue Pill; factory ROM bootloader for Pico | The STM32 application starts at `0x08002000` and uses the documented `1EAF:0003` DFU path. Pico uses BOOTSEL and a UF2 without installing another bootloader. |
| Display | SSD1306-compatible 128x32 monochrome OLED, I2C | A 128x64 module needs housing and firmware/UI changes and is not the maintained display target. Confirm module voltage and pin order; OLED breakout pin orders are not universal. |
| Main lighting | 1-32 three-channel WS2812-compatible addressable RGB pixels | Default 11. Firmware signaling is 800 kHz in GRB order. WS2812B and compatible SK6812 RGB pixels are typical. RGBW, different byte orders, or different signaling rates need a matching build. |
| Side indicators | 2 monochrome LEDs or compatible LED modules | Both are on the controller's right side. STM32 uses PB13/PB12 and Pico uses GP11/GP12 for the physically left/right indicators. Runtime polarity supports active-high and active-low wiring. Bare LEDs require suitable series resistors. |
| Thumbstick | Two-axis analog joystick with optional push switch | Analog outputs must remain within the selected MCU's ADC input range. Do not feed a 5 V analog output into either MCU. The push switch is wired as a matrix key. |
| Switches | Momentary switches for the finger keys, D-pad, and auxiliary positions | The firmware matrix provides 30 positions. The number physically populated depends on the build. |
| Matrix diodes | One signal diode per populated switch, commonly 1N4148 | Firmware diode direction is `COL2ROW`; diode orientation must match the wiring. |
| Wiring and interconnects | Insulated wire, connectors or headers, strain relief, and insulation | Dupont-style connections and small protoboard are common, but permanent builds need mechanically secure, insulated connections. |
| USB cable/connector | Data-capable USB connection suitable for the selected controller | Charge-only cables cannot configure or flash the device. Some Blue Pill clones need a physical reconnect after DFU flashing. |
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

## Raspberry Pi Pico pin map

This target is for the standard Raspberry Pi Pico with an RP2040. It uses the
following wiring supplied for the Pico-based Replicazeron build:

| Function | Pico pin |
|---|---|
| Column 0 | GP0 |
| Column 1 | GP1 |
| Column 2 | GP2 |
| Column 3 | GP3 |
| Column 4 | GP4 |
| Row 0 | GP5 |
| Row 1 | GP6 |
| Row 2 | GP7 |
| Row 3 | GP8 |
| Row 4 | GP9 |
| Row 5 | GP10 |
| Analog axis 0 / firmware X | GP26 / ADC0 |
| Analog axis 1 / firmware Y | GP27 / ADC1 |
| OLED SDA | GP20 / I2C0 SDA |
| OLED SCL | GP21 / I2C0 SCL |
| Left side indicator | GP11 |
| Right side indicator | GP12 |
| Addressable RGB data | GP13 |

The separate base button is the `COL_1` / `ROW_5` matrix cell. The 5-way thumb
switch uses column 4 as common: the supplied harness identifies
center as row 0 and contacts A-D as rows 1-4. 9R's STM32 schematic confirms the
same column/row topology but does not name the five physical directions. It
also connects the joystick's single push button between column 4 and row 5
through its own diode: `COL_4 -> button terminal 1`, then `button terminal 2 ->
1N4148 -> ROW_5`, with the diode band/bar toward `ROW_5`. These are two
terminals of one button, not two buttons, and both terminals must be isolated.
A module whose only `SW` output is internally referenced to GND cannot be wired
directly as this matrix cell. The separate five-way navigation switch is also
one physical component: its center press and four directions are five contacts
sharing `COL_4` on rows 0-4. The OLED harness is VCC, SCL, SDA, GND; confirm its
physical connector order before applying power.

The two side indicators are wired active-low in 9R's schematic and in the
firmware default: `3.3 V -> resistor -> LED anode`, with the LED cathode going
to GP11 or GP12. A low GPIO output turns the LED on. The runtime polarity option
still supports an intentionally active-high build.

**Do not power the thumbstick potentiometers from 5 V when their outputs are
connected directly to GP26 and GP27. RP2040 GPIO and ADC inputs are not 5 V
tolerant.** For an ordinary passive two-potentiometer joystick, connect its
supply lead to Pico `3V3(OUT)` and its ground lead to GND so the analog outputs
remain in the 0-3.3 V range. If the module truly requires
5 V, use correctly designed voltage translation rather than connecting its
outputs directly. Also ensure any OLED I2C pull-ups go to 3.3 V, or use a level
shifter; a breakout powered from 5 V may pull SDA/SCL up to 5 V.

Hold **BOOTSEL** while connecting USB to enter the Pico ROM bootloader, then
release it before copying the firmware. The Pico appears as an `RPI-RP2` drive;
copy the generated `.uf2` onto that drive. The firmware deliberately does not
enable the optional double-tap-reset detector. It can still enter the ROM
bootloader through an explicit QMK/WebHID firmware reset request. The Pico does
not require an STM32duino bootloader or the Blue Pill WebDFU procedure.

Bootmagic is disabled on the Pico target because it treats matrix row 0,
column 0 as a startup request to enter the bootloader. On the documented
hand-wired matrix that position can be asserted during power-up and cause an
otherwise valid application to return immediately to `RPI-RP2`. BOOTSEL and
the explicit firmware reset command remain available for recovery.

## Addressable RGB selection

The saved **Addressable LEDs** setting accepts 1-32 pixels and defaults to 11.
It changes how many physical strip pixels firmware effects and VialRGB expose.
The left and right monochrome side indicators are separate GPIO outputs and are
not exposed to VialRGB or OpenRGB.

Changing the runtime count cannot change the electrical protocol. Recompile if
the installed pixels require RGBW data, a color order other than GRB, or a rate
other than 800 kHz. Rescan OpenRGB after changing the count because it reads
the topology when the device connects.

For reliable WS2812 signaling:

- Keep data wiring short and routed away from noisy switch or power wiring.
- Connect the LED supply ground and controller ground together.
- Follow the pixel manufacturer's recommendation for data-line resistance and
  local supply decoupling.
- If the exact LED strip specification accepts a 3.3 V data input, connect the
  selected GPIO to DIN through the recommended series resistor and omit the
  level shifter.
- If the strip does not accept 3.3 V data, or its input requirement is unknown,
  use an appropriate logic-level shifter such as a 5 V-powered 74AHCT device.
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
| Pro Micro / ATmega32U4 | Not supported in this version. Flash and EEPROM are insufficient for the complete feature set; a heavily reduced variant would only be considered if there is enough demand. Migrate new builds to STM32F103 or RP2040 instead. |
| `handwired/replicazeron/rp2040` | Raspberry Pi Pico target using the shared Vial/OLED/WebHID/lighting feature code; compiles successfully and awaits physical validation on the documented wiring |

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
