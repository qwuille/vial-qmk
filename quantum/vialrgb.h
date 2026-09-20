/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <stdbool.h>
#include <inttypes.h>

#define VIALRGB_PROTOCOL_VERSION 1

/* Start at 0x40 in order to not conflict with existing "enum via_lighting_value",
   even though they likely wouldn't be enabled together with vialrgb */
enum {
    vialrgb_set_mode = 0x41,
    vialrgb_direct_fastset = 0x42,
};

enum {
    vialrgb_get_info = 0x40,
    vialrgb_get_mode = 0x41,
    vialrgb_get_supported = 0x42,
    vialrgb_get_number_leds = 0x43,
    vialrgb_get_led_info = 0x44,
};

void vialrgb_get_value(uint8_t *data, uint8_t length);
void vialrgb_set_value(uint8_t *data, uint8_t length);
void vialrgb_save(uint8_t *data, uint8_t length);

/* Keyboard-level policy hook. Return false to expose VialRGB information while
 * ignoring host lighting writes. Vial keymap commands are unaffected. */
bool vialrgb_allow_write_kb(uint8_t command);

/* Optional virtual LEDs supplied by a keyboard. The reported count may be
 * larger than RGB_MATRIX_LED_COUNT; virtual writes are delivered through the
 * hook without being passed to the physical RGB Matrix driver. */
uint16_t vialrgb_get_number_leds_kb(void);
bool vialrgb_get_led_info_kb(uint16_t led, uint8_t *output);
void vialrgb_set_led_kb(uint16_t led, uint8_t hue, uint8_t sat, uint8_t val);

#if defined(VIALRGB_ENABLE) && !defined(RGB_MATRIX_ENABLE)
#error VIALRGB_ENABLE=yes requires RGB_MATRIX_ENABLE=yes
#endif
