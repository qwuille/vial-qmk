$ErrorActionPreference = 'Stop'

$docs = if ($PSScriptRoot) { $PSScriptRoot } else { (Resolve-Path '.\docs').Path }
$utf8 = [System.Text.UTF8Encoding]::new($false)

function Replace-Literal([string]$text, [System.Collections.IDictionary]$replacements) {
    foreach ($entry in $replacements.GetEnumerator()) {
        $text = $text.Replace($entry.Key, $entry.Value)
    }
    return $text
}

$svgSource = [IO.File]::ReadAllText((Join-Path $docs 'replicazeron-rp2040-wiring-diagram.svg'))
$controller = @'
  <!-- CONTROLLER -->
  <g id="controller" transform="translate(30 548)">
    <rect width="520" height="485" rx="8" class="zone"/>
    <rect width="520" height="38" rx="8" class="zone-head"/><path d="M0 38 H520" stroke="#a9c0c5"/>
    <text x="14" y="25" class="zone-title">Blue Pill header wiring</text>
    <rect x="100" y="58" width="320" height="386" rx="10" class="mcu"/>
    <text x="260" y="79" text-anchor="middle" class="label">U1 · STM32F103 BLUE PILL</text>
    <text x="260" y="94" text-anchor="middle" class="small">USB connector at top · header labels shown</text>
    <g class="pin-small">
      <text x="112" y="120">GND</text><text x="112" y="138">GND</text><text x="112" y="156">3V3</text><text x="112" y="174">NRST</text>
      <text x="112" y="192">PB11  OLED_SDA</text><text x="112" y="210">PB10  OLED_SCL</text><text x="112" y="228">PB1   JOY_VRY</text><text x="112" y="246">PB0   JOY_VRX</text>
      <text x="112" y="264">PA7   COL_0</text><text x="112" y="282">PA6   COL_1</text><text x="112" y="300">PA5   COL_2</text><text x="112" y="318">PA4   COL_3</text>
      <text x="112" y="336">PA3</text><text x="112" y="354">PA2</text><text x="112" y="372">PA1</text><text x="112" y="390">PA0</text><text x="112" y="408">PC15 · PC14 · PC13 · VBAT</text>
    </g>
    <g class="pin-small" text-anchor="end">
      <text x="408" y="120">5V</text><text x="408" y="138">GND</text><text x="408" y="156">3V3</text><text x="408" y="174">PB9</text><text x="408" y="192">PB8</text>
      <text x="408" y="210">PB7</text><text x="408" y="228">PB6</text><text x="408" y="246">PB5</text><text x="408" y="264">PB4   COL_4</text><text x="408" y="282">PB3   ROW_5</text>
      <text x="408" y="300">PA15  ROW_4</text><text x="408" y="318">PA12  USB D+</text><text x="408" y="336">PA11  USB D−</text><text x="408" y="354">PA10  ROW_3</text>
      <text x="408" y="372">PA9   ROW_2</text><text x="408" y="390">PA8   ROW_1</text><text x="408" y="408">PB15  ROW_0</text><text x="408" y="426">PB14 RGB · PB13 LED L · PB12 LED R</text>
    </g>
    <text x="260" y="463" text-anchor="middle" class="warning-text">PA15, PB3 AND PB4 MUST BE AVAILABLE AS GPIO (NOT JTAG)</text>
  </g>
'@
$svg = [regex]::Replace($svgSource, '(?s)  <!-- CONTROLLER -->.*?  </g>\r?\n\r?\n  <!-- PERIPHERALS -->', $controller + "`r`n  <!-- PERIPHERALS -->", 1)
$svg = Replace-Literal $svg ([ordered]@{
    'Replicazeron RP2040 wiring diagram' = 'Replicazeron STM32F103 wiring diagram'
    'Raspberry Pi Pico header' = 'STM32F103 Blue Pill header'
    'REPLICAZERON · RP2040 WIRING DIAGRAM' = 'REPLICAZERON · STM32F103 WIRING DIAGRAM'
    'standard Raspberry Pi Pico' = 'STM32F103 Blue Pill'
    'COL_3 · GP3 · P5' = 'COL_3 · PA4'
    'COL_2 · GP2 · P4' = 'COL_2 · PA5'
    'COL_1 · GP1 · P2' = 'COL_1 · PA6'
    'COL_0 · GP0 · P1' = 'COL_0 · PA7'
    'COL_4 · GP4 · P6' = 'COL_4 · PB4'
    'use GP13 → 330Ω → DIN' = 'use PB14 → 330Ω → DIN'
    'RP2040 INPUTS ARE NOT 5 V TOLERANT' = 'STM32F103 INPUTS ARE NOT 5 V TOLERANT'
    'Power joystick and OLED from Pico 3V3(OUT), pin 36.' = 'Power joystick and OLED from the Blue Pill 3V3 header.'
    'Keep GP0–GP28 and ADC signals within 0–3.3 V.' = 'Keep GPIO and ADC signals within 0–3.3 V.'
    'All supplies, Pico and peripherals require common ground.' = 'All supplies, Blue Pill and peripherals require common ground.'
    'Do not tie external +5 V to VBUS while USB is connected' = 'Do not backfeed the USB 5 V rail from an external supply'
    'REPLICAZERON RP2040' = 'REPLICAZERON STM32F103'
    'RP2040-WIRE-01' = 'STM32-WIRE-01'
})
[IO.File]::WriteAllText((Join-Path $docs 'replicazeron-stm32f103-wiring-diagram.svg'), $svg, $utf8)

$html = [IO.File]::ReadAllText((Join-Path $docs 'replicazeron-rp2040-wiring-diagram.html'))
$html = [regex]::Replace($html, '(?s)\r?\n  <svg viewBox="0 0 1120 610".*?</svg>', '', 1)
$html = Replace-Literal $html ([ordered]@{
    'Replicazeron — Raspberry Pi Pico (RP2040) wiring diagram' = 'Replicazeron — STM32F103 Blue Pill wiring diagram'
    '<strong>RP2040 wiring</strong>' = '<strong>STM32 / Blue Pill wiring</strong>'
    'Replicazeron_RP2040_Wiring_Diagram.pdf' = 'Replicazeron_STM32F103_Wiring_Diagram.pdf'
    'replicazeron-stm32f103-wiring-diagram.html">STM32 / Blue Pill diagram' = 'replicazeron-rp2040-wiring-diagram.html">RP2040 diagram'
    'Replicazeron — Raspberry Pi Pico wiring diagram' = 'Replicazeron — STM32F103 Blue Pill wiring diagram'
    'Standard Pico / RP2040' = 'STM32F103 Blue Pill'
    'handwired/replicazeron/rp2040' = 'handwired/replicazeron/stm32f103'
    'replicazeron-rp2040-wiring-diagram.svg' = 'replicazeron-stm32f103-wiring-diagram.svg'
    'Complete Replicazeron RP2040 assembly wiring diagram' = 'Complete Replicazeron STM32F103 assembly wiring diagram'
    'exact Pico pins' = 'exact Blue Pill GPIO labels'
    'Pico pins' = 'Blue Pill GPIO labels'
    'RP2040' = 'STM32F103'
    'Pico' = 'Blue Pill'
    'GP26 / pin 31' = 'PB0 / ADC'
    'GP27 / pin 32' = 'PB1 / ADC'
    'GP21/SCL, pin 27' = 'PB10 / SCL'
    'GP20/SDA, pin 26' = 'PB11 / SDA'
    'GP11 / pin 15' = 'PB13'
    'GP12 / pin 16' = 'PB12'
    'Blue Pill GP13' = 'Blue Pill PB14'
    'GP13 data' = 'PB14 data'
    'GP13 directly' = 'PB14 directly'
    'GP26/GP27' = 'PB0/PB1'
    '3V3(OUT), pin 36' = 'Blue Pill 3V3 header'
    'pin 36, 3V3(OUT)' = '3V3 header'
    'RPI-RP2' = 'STM32duino DFU'
    'BOOTSEL' = 'BOOT0'
    'Flash the UF2' = 'Flash the STM32 .bin through STM32duino DFU'
})

$gpioMap = [ordered]@{
    'GP27'='PB1'; 'GP26'='PB0'; 'GP21'='PB10'; 'GP20'='PB11'; 'GP13'='PB14'; 'GP12'='PB12'; 'GP11'='PB13'; 'GP10'='PB3';
    'GP9'='PA15'; 'GP8'='PA10'; 'GP7'='PA9'; 'GP6'='PA8'; 'GP5'='PB15'; 'GP4'='PB4'; 'GP3'='PA4'; 'GP2'='PA5'; 'GP1'='PA6'; 'GP0'='PA7'
}
foreach ($entry in $gpioMap.GetEnumerator()) {
    $html = [regex]::Replace($html, "\b$([regex]::Escape($entry.Key))\b", $entry.Value)
}

$html = [regex]::Replace($html, "(?s)\{ name: 'ROW_0'.*?\}\r?\n  \];", @'
{ name: 'ROW_0', gpio: 'PB15', pin: 'PB15' },
    { name: 'ROW_1', gpio: 'PA8', pin: 'PA8' },
    { name: 'ROW_2', gpio: 'PA9', pin: 'PA9' },
    { name: 'ROW_3', gpio: 'PA10', pin: 'PA10' },
    { name: 'ROW_4', gpio: 'PA15', pin: 'PA15' },
    { name: 'ROW_5', gpio: 'PB3', pin: 'PB3' }
  ];
'@, 1, [System.TimeSpan]::FromSeconds(2))
$html = [regex]::Replace($html, "(?s)\{ name: 'COL_0'.*?\}\r?\n  \];", @'
{ name: 'COL_0', gpio: 'PA7', pin: 'PA7', group: 'Little finger', functions: ['KEY 1', 'KEY 2', 'KEY 3', 'KEY 4', 'KEY 5', 'OUTER KEY'] },
    { name: 'COL_1', gpio: 'PA6', pin: 'PA6', group: 'Ring finger', functions: ['KEY 1', 'KEY 2', 'KEY 3', 'KEY 4', 'KEY 5', 'BASE BUTTON'] },
    { name: 'COL_2', gpio: 'PA5', pin: 'PA5', group: 'Middle finger', functions: ['KEY 1', 'KEY 2', 'KEY 3', 'KEY 4', 'KEY 5', 'THUMB-SIDE KEY'] },
    { name: 'COL_3', gpio: 'PA4', pin: 'PA4', group: 'Index finger', functions: ['KEY 1', 'KEY 2', 'KEY 3', 'KEY 4', 'KEY 5', 'INNER KEY'] },
    { name: 'COL_4', gpio: 'PB4', pin: 'PB4', group: '5-way button', functions: ['5-WAY CENTER', '5-WAY A', '5-WAY B', '5-WAY C', '5-WAY D', 'JOYSTICK BUTTON'] }
  ];
'@, 1, [System.TimeSpan]::FromSeconds(2))
$html = $html.Replace('/P${row.pin}', '').Replace('/P${column.pin}', '')
$html = $html.Replace('· pin ${row.pin}', '· header ${row.pin}')
$html = $html.Replace('physical pin <code>${column.pin}</code>', 'Blue Pill header label <code>${column.pin}</code>')
$html = $html.Replace('physical pin <code>${row.pin}</code>', 'Blue Pill header label <code>${row.pin}</code>')
$html = $html.Replace('physical pin', 'header label').Replace('Physical pin', 'Header label').Replace('Physical', 'Header label')
$html = $html.Replace('Power the module from 3.3 V unless its exact schematic proves that 5 V VCC cannot pull SDA/SCL above 3.3 V.', 'Power the module from the Blue Pill 3.3 V header unless its exact schematic proves that 5 V VCC cannot pull SDA/SCL above 3.3 V.')
$html = $html.Replace('Direction mapping to resolve:', 'Direction mapping to verify:')
$html = $html.Replace('>STM32F103 diagram</a>', '>RP2040 diagram</a>')
$html = $html.Replace('RASPBERRY PI PICO', 'STM32F103 BLUE PILL')
$html = $html.Replace('STM32F103 · physical header pins', 'STM32F103 · GPIO header labels')
$html = $html.Replace('ROW_0 · PB15/P7', 'ROW_0 · PB15').Replace('ROW_1 · PA8/P9', 'ROW_1 · PA8').Replace('ROW_2 · PA9/P10', 'ROW_2 · PA9').Replace('ROW_3 · PA10/P11', 'ROW_3 · PA10').Replace('ROW_4 · PA15/P12', 'ROW_4 · PA15').Replace('ROW_5 · PB3/P14', 'ROW_5 · PB3')
$html = $html.Replace('<tr><td>COL_0</td><td>PA7</td><td>1</td></tr>', '<tr><td>COL_0</td><td>PA7</td><td>PA7</td></tr>').Replace('<tr><td>COL_1</td><td>PA6</td><td>2</td></tr>', '<tr><td>COL_1</td><td>PA6</td><td>PA6</td></tr>').Replace('<tr><td>COL_2</td><td>PA5</td><td>4</td></tr>', '<tr><td>COL_2</td><td>PA5</td><td>PA5</td></tr>').Replace('<tr><td>COL_3</td><td>PA4</td><td>5</td></tr>', '<tr><td>COL_3</td><td>PA4</td><td>PA4</td></tr>').Replace('<tr><td>COL_4</td><td>PB4</td><td>6</td></tr>', '<tr><td>COL_4</td><td>PB4</td><td>PB4</td></tr>')
$html = $html.Replace('<tr><td>ROW_0</td><td>PB15</td><td>7</td></tr>', '<tr><td>ROW_0</td><td>PB15</td><td>PB15</td></tr>').Replace('<tr><td>ROW_1</td><td>PA8</td><td>9</td></tr>', '<tr><td>ROW_1</td><td>PA8</td><td>PA8</td></tr>').Replace('<tr><td>ROW_2</td><td>PA9</td><td>10</td></tr>', '<tr><td>ROW_2</td><td>PA9</td><td>PA9</td></tr>').Replace('<tr><td>ROW_3</td><td>PA10</td><td>11</td></tr>', '<tr><td>ROW_3</td><td>PA10</td><td>PA10</td></tr>').Replace('<tr><td>ROW_4</td><td>PA15</td><td>12</td></tr>', '<tr><td>ROW_4</td><td>PA15</td><td>PA15</td></tr>').Replace('<tr><td>ROW_5</td><td>PB3</td><td>14</td></tr>', '<tr><td>ROW_5</td><td>PB3</td><td>PB3</td></tr>')
$html = $html.Replace('<tr><td>Left side LED</td><td>PB13</td><td>15</td></tr>', '<tr><td>Left side LED</td><td>PB13</td><td>PB13</td></tr>').Replace('<tr><td>Right side LED</td><td>PB12</td><td>16</td></tr>', '<tr><td>Right side LED</td><td>PB12</td><td>PB12</td></tr>').Replace('<tr><td>WS2812 data</td><td>PB14</td><td>17</td></tr>', '<tr><td>WS2812 data</td><td>PB14</td><td>PB14</td></tr>').Replace('<tr><td>OLED SDA</td><td>PB11</td><td>26</td></tr>', '<tr><td>OLED SDA</td><td>PB11</td><td>PB11</td></tr>').Replace('<tr><td>OLED SCL</td><td>PB10</td><td>27</td></tr>', '<tr><td>OLED SCL</td><td>PB10</td><td>PB10</td></tr>').Replace('<tr><td>Joystick VRX</td><td>PB0/ADC0</td><td>31</td></tr>', '<tr><td>Joystick VRX</td><td>PB0 / ADC8</td><td>PB0</td></tr>').Replace('<tr><td>Joystick VRY</td><td>PB1/ADC1</td><td>32</td></tr>', '<tr><td>Joystick VRY</td><td>PB1 / ADC9</td><td>PB1</td></tr>')
$html = $html.Replace('<tr><td>Joystick ground</td><td>AGND</td><td>33</td></tr>', '<tr><td>Joystick ground</td><td>GND</td><td>GND</td></tr>').Replace('<tr><td>3.3 V peripherals</td><td>3V3(OUT)</td><td>36</td></tr>', '<tr><td>3.3 V peripherals</td><td>3V3</td><td>3V3</td></tr>').Replace('<tr><td>Logic ground</td><td>GND</td><td>3/8/13/18/23/28/38</td></tr>', '<tr><td>Logic ground</td><td>GND</td><td>Either GND header</td></tr>')
$html = $html.Replace('GP14, GP15, GP16–GP19, GP22, and GP28 remain spare in this firmware.', 'Unused GPIO remains available only where it does not conflict with USB, SWD, the bootloader, or the documented JTAG-remapped matrix pins.')
$html = $html.Replace('Do not tie an external 5 V supply to Blue Pill VBUS while USB is connected', 'Do not backfeed the Blue Pill 5 V/USB rail from an external supply')
$html = $html.Replace('Verify BOOT0 is not mechanically stuck. BOOT0 is onboard and must not be wired into the key matrix.', 'Verify the BOOT0 jumper is in its normal 0 position. BOOT0 and BOOT1/PB2 are not part of the key matrix.')
$html = $html.Replace('Continuity-check every harness wire to the Blue Pill header label—not only the GP label.', 'Continuity-check every harness wire to the correct Blue Pill GPIO header label.')
$html = $html.Replace('PB4 / pin 6 / COL_4', 'PB4 / COL_4').Replace('PB3 / pin 14 / ROW_5', 'PB3 / ROW_5')
$html = $html.Replace('COL_4 · PB4 / pin 6', 'COL_4 · PB4').Replace('ROW_5 · PB3 / pin 14', 'ROW_5 · PB3')
$html = $html.Replace('Flash the STM32 .bin through STM32duino DFU and confirm the <span class="mono">STM32duino DFU</span> drive disappears and the USB HID device enumerates.', 'Enter STM32duino DFU, flash the STM32 <span class="mono">.bin</span>, then reset or reconnect as required and confirm the USB HID device enumerates.')
$html = $html.Replace("compared net-by-net with 9R's STM32 KiCad schematic. It preserves the COL2ROW topology, COL_4 thumb common, ROW_5 stick click, 3.3 V analog/OLED supplies, active-low indicators, and 5 V WS2812 supply while translating MCU pins to Blue Pill.", "based on the validated 9R STM32 wiring and the maintained firmware pin map. It preserves the COL2ROW topology, COL_4 thumb common, ROW_5 stick click, 3.3 V analog/OLED supplies, active-low indicators, and 5 V WS2812 supply.")
[IO.File]::WriteAllText((Join-Path $docs 'replicazeron-stm32f103-wiring-diagram.html'), $html, $utf8)
