/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#define VIAL_KEYBOARD_UID {0x3E, 0x15, 0xC1, 0x32, 0xC3, 0xDE, 0xCD, 0x83}

#define VIAL_UNLOCK_COMBO_ROWS { 4, 4 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 3 }

/* Key-reactive strip effects exposed by the on-device RGB animation menu. */
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE
#define ENABLE_RGB_MATRIX_SPLASH
#define ENABLE_RGB_MATRIX_MULTISPLASH
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_CROSS
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_WIDE

/* Keep device-specific metadata out of VIA's dynamic keymap and macro data.
 * Reserving the tail preserves the existing dynamic-keymap start address. */
#define REPLICAZERON_METADATA_EEPROM_SIZE 288
#define DYNAMIC_KEYMAP_EEPROM_MAX_ADDR (TOTAL_EEPROM_BYTE_COUNT - REPLICAZERON_METADATA_EEPROM_SIZE - 1)
