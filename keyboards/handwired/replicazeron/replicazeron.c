/* Copyright 2023 9R
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "replicazeron.h"
#include "usb_util.h"
#include "eeconfig.h"
#include "raw_hid.h"
#ifdef JOYSTICK_ENABLE
#    include "analog.h"
#endif
#ifdef RGB_MATRIX_ENABLE
#    include "rgb_matrix.h"
#    include "vialrgb.h"
#endif
#ifdef VIA_ENABLE
#    include "eeprom.h"
#    include "via.h"
#    include "quantum/nvm/eeprom/nvm_eeprom_eeconfig_internal.h"
#    include "quantum/nvm/eeprom/nvm_eeprom_via_internal.h"
#endif

controller_state_t controller_state;

#ifdef RGB_MATRIX_ENABLE
static uint8_t rgb_animation_mode(rgb_animation_id_t animation);
static rgb_animation_id_t rgb_animation_from_mode(uint8_t mode);
static rgb_animation_id_t current_rgb_animation(void);
#endif

#define REPLICAZERON_USER_CONFIG_MAGIC 0x53
#define REPLICAZERON_USER_CONFIG_LEGACY_MAGIC 0x52
#define REPLICAZERON_USER_CONFIG_MAGIC_SHIFT 24
#define REPLICAZERON_USER_CONFIG_OPENRGB_SHIFT 21
#define REPLICAZERON_USER_CONFIG_SIDE_LEDS_DISABLED_SHIFT 22
#define REPLICAZERON_USER_CONFIG_STATIC_SHIFT 20
#define REPLICAZERON_USER_CONFIG_ANIMATION_SHIFT 17
#define REPLICAZERON_USER_CONFIG_ANIMATION_HIGH_SHIFT 23
#define REPLICAZERON_USER_CONFIG_FILTER_SHIFT 10
#define REPLICAZERON_USER_CONFIG_DEADZONE_MASK 0x03FF
#define REPLICAZERON_USER_CONFIG_FILTER_MASK 0x7F
#define REPLICAZERON_USER_CONFIG_ANIMATION_MASK 0x07
#define REPLICAZERON_USER_CONFIG_LEGACY_STATIC_MASK 0x80
#define REPLICAZERON_MODE_CONFIG_LEGACY_MAGIC 0xA7
#define REPLICAZERON_MODE_CONFIG_FORMAT 0xC0
#define REPLICAZERON_MODE_CONFIG_FORMAT_MASK 0xE0
#define REPLICAZERON_MODE_CONFIG_LED_COUNT_MASK 0x1F
#define REPLICAZERON_MODE_CONFIG_MAGIC_SHIFT 24
#define REPLICAZERON_MODE_CONFIG_SIDE_LED_SHIFT 20
#define REPLICAZERON_MODE_CONFIG_SIDE_LED_MASK 0x0F
#define REPLICAZERON_LAYOUT_KEY_ROW 5
#define REPLICAZERON_LAYOUT_KEY_COL 1
#define REPLICAZERON_LAYOUT_KEY_HOLD_MS 500
#define REPLICAZERON_RESET_LITTLE_ROW 0
#define REPLICAZERON_RESET_LITTLE_COL 0
#define REPLICAZERON_RESET_INDEX_ROW 0
#define REPLICAZERON_RESET_INDEX_COL 3
#define REPLICAZERON_DPAD_RIGHT_ROW 0
#define REPLICAZERON_DPAD_RIGHT_COL 4
#define REPLICAZERON_DPAD_SELECT_ROW 1
#define REPLICAZERON_DPAD_SELECT_COL 4
#define REPLICAZERON_DPAD_DOWN_ROW 2
#define REPLICAZERON_DPAD_DOWN_COL 4
#define REPLICAZERON_DPAD_LEFT_ROW 3
#define REPLICAZERON_DPAD_LEFT_COL 4
#define REPLICAZERON_DPAD_UP_ROW 4
#define REPLICAZERON_DPAD_UP_COL 4
#define REPLICAZERON_SIDE_LED_PREVIEW_DISTANCE 724
#define REPLICAZERON_SIDE_LED_MIN_LEVEL 20
#define REPLICAZERON_SIDE_LED_SOURCE_SHIFT 2

static uint16_t layout_key_timer;
static bool reset_little_held;
static bool reset_index_held;
static bool reset_combo_armed;
static uint32_t reset_combo_timer;
static uint32_t side_led_preview_started;
static bool side_led_preview_requested;
static bool bootloader_requested;
static bool bootloader_usb_disconnect_assist;
static uint32_t bootloader_request_started;

#define REPLICAZERON_FACTORY_RESET_HOLD_MS 2000
#define REPLICAZERON_SIDE_LED_PREVIEW_MS 1200
#define REPLICAZERON_BOOTLOADER_DELAY_MS 500

#ifdef VIA_ENABLE
#    define REPLICAZERON_TITLE_SIGNATURE_SIZE 2
#    define REPLICAZERON_TITLE_STORAGE_OFFSET REPLICAZERON_TITLE_SIGNATURE_SIZE
#    define REPLICAZERON_TITLE_STORAGE_SIZE (LAYOUT_COUNT * REPLICAZERON_TITLE_LENGTH)
#    define REPLICAZERON_MODE_STORAGE_OFFSET (REPLICAZERON_TITLE_STORAGE_OFFSET + REPLICAZERON_TITLE_STORAGE_SIZE)
#    define REPLICAZERON_MODE_STORAGE_SIZE LAYOUT_COUNT
#    define REPLICAZERON_MODE_SIGNATURE_OFFSET (REPLICAZERON_MODE_STORAGE_OFFSET + REPLICAZERON_MODE_STORAGE_SIZE)
#    define REPLICAZERON_MODE_SIGNATURE_SIZE 2
#    define REPLICAZERON_SIDE_LED_POLARITY_OFFSET (REPLICAZERON_MODE_SIGNATURE_OFFSET + REPLICAZERON_MODE_SIGNATURE_SIZE)
#endif

static void release_wasd_keys(void) {
    unregister_code(KC_W);
    unregister_code(KC_A);
    unregister_code(KC_S);
    unregister_code(KC_D);
    unregister_code(KC_LSFT);
#ifdef THUMBSTICK_ENABLE
    init_wasd_state();
#endif
}

static void apply_layout_mode(uint8_t layout) {
    if (layout >= LAYOUT_COUNT) {
        return;
    }

    release_wasd_keys();
    controller_state.wasdMode = controller_state.layoutModes[layout] != JOYSTICK_MODE_ANALOG;
    controller_state.wasdShiftMode = controller_state.layoutModes[layout] == JOYSTICK_MODE_WASD_SHIFT;
}

static void write_layout_modes(void) {
    uint8_t side_led_level = MAX(2, (controller_state.sideLedBrightness + 8) / 17);
    uint8_t hardware_config = REPLICAZERON_MODE_CONFIG_FORMAT | (controller_state.rgbLedCount - 1);
    uint32_t config = ((uint32_t)hardware_config << REPLICAZERON_MODE_CONFIG_MAGIC_SHIFT) |
                      ((uint32_t)side_led_level << REPLICAZERON_MODE_CONFIG_SIDE_LED_SHIFT);
    for (uint8_t layout = 0; layout < LAYOUT_COUNT; ++layout) {
        config |= (uint32_t)controller_state.layoutModes[layout] << (layout * 2);
    }
    eeconfig_update_kb(config);
#ifdef VIA_ENABLE
    uint8_t stored_modes[REPLICAZERON_MODE_STORAGE_SIZE];
    for (uint8_t layout = 0; layout < LAYOUT_COUNT; ++layout) {
        stored_modes[layout] = controller_state.layoutModes[layout];
    }
    stored_modes[0] |= controller_state.sideLedSourceA << REPLICAZERON_SIDE_LED_SOURCE_SHIFT;
    stored_modes[1] |= controller_state.sideLedSourceB << REPLICAZERON_SIDE_LED_SOURCE_SHIFT;
    eeprom_update_block(stored_modes, (void *)(uintptr_t)(VIA_EEPROM_CUSTOM_CONFIG_ADDR + REPLICAZERON_MODE_STORAGE_OFFSET),
                        REPLICAZERON_MODE_STORAGE_SIZE);
    static const uint8_t mode_signature[REPLICAZERON_MODE_SIGNATURE_SIZE] = {'J', '3'};
    eeprom_update_block(mode_signature, (void *)(uintptr_t)(VIA_EEPROM_CUSTOM_CONFIG_ADDR + REPLICAZERON_MODE_SIGNATURE_OFFSET),
                        REPLICAZERON_MODE_SIGNATURE_SIZE);
    eeprom_update_byte((uint8_t *)(uintptr_t)(VIA_EEPROM_CUSTOM_CONFIG_ADDR + REPLICAZERON_SIDE_LED_POLARITY_OFFSET), controller_state.sideLedsActiveLow);
#endif
}

static void set_layout_mode(uint8_t layout, joystick_mode_t mode) {
    if (layout >= LAYOUT_COUNT || mode >= JOYSTICK_MODE_COUNT) {
        return;
    }
    if (controller_state.layoutModes[layout] == mode) {
        return;
    }

    controller_state.layoutModes[layout] = mode;
    write_layout_modes();
    if (layout == controller_state.activeLayout) {
        apply_layout_mode(layout);
    }
}

static void load_layout_modes(void) {
    uint32_t config = eeconfig_read_kb();
    uint8_t hardware_config = config >> REPLICAZERON_MODE_CONFIG_MAGIC_SHIFT;
    bool current_hardware_config = (hardware_config & REPLICAZERON_MODE_CONFIG_FORMAT_MASK) == REPLICAZERON_MODE_CONFIG_FORMAT;
    bool legacy_hardware_config = hardware_config == REPLICAZERON_MODE_CONFIG_LEGACY_MAGIC;
    bool modes_loaded = false;
    bool modes_need_write = false;
#ifdef VIA_ENABLE
    uint8_t stored_mode_signature[REPLICAZERON_MODE_SIGNATURE_SIZE];
    eeprom_read_block(stored_mode_signature, (void *)(uintptr_t)(VIA_EEPROM_CUSTOM_CONFIG_ADDR + REPLICAZERON_MODE_SIGNATURE_OFFSET),
                      REPLICAZERON_MODE_SIGNATURE_SIZE);
    bool encoded_sources = stored_mode_signature[0] == 'J' && stored_mode_signature[1] == '3';
    if (encoded_sources || (stored_mode_signature[0] == 'J' && stored_mode_signature[1] == '2')) {
        uint8_t stored_modes[REPLICAZERON_MODE_STORAGE_SIZE];
        eeprom_read_block(stored_modes, (void *)(uintptr_t)(VIA_EEPROM_CUSTOM_CONFIG_ADDR + REPLICAZERON_MODE_STORAGE_OFFSET),
                          REPLICAZERON_MODE_STORAGE_SIZE);
        modes_loaded = true;
        for (uint8_t layout = 0; layout < LAYOUT_COUNT; ++layout) {
            controller_state.layoutModes[layout] = encoded_sources ? stored_modes[layout] & 0x03 : stored_modes[layout];
            if (controller_state.layoutModes[layout] >= JOYSTICK_MODE_COUNT) {
                controller_state.layoutModes[layout] = JOYSTICK_MODE_ANALOG;
                modes_loaded = false;
            }
        }
        if (encoded_sources) {
            controller_state.sideLedSourceA = stored_modes[0] >> REPLICAZERON_SIDE_LED_SOURCE_SHIFT;
            controller_state.sideLedSourceB = stored_modes[1] >> REPLICAZERON_SIDE_LED_SOURCE_SHIFT;
            if (controller_state.sideLedSourceA >= SIDE_LED_SOURCE_COUNT) {
                controller_state.sideLedSourceA = SIDE_LED_SOURCE_STICK;
                modes_need_write = true;
            }
            if (controller_state.sideLedSourceB >= SIDE_LED_SOURCE_COUNT) {
                controller_state.sideLedSourceB = SIDE_LED_SOURCE_BUTTONS;
                modes_need_write = true;
            }
        } else {
            modes_need_write = true;
        }
        uint8_t stored_polarity = eeprom_read_byte((uint8_t *)(uintptr_t)(VIA_EEPROM_CUSTOM_CONFIG_ADDR + REPLICAZERON_SIDE_LED_POLARITY_OFFSET));
        if (stored_polarity <= 1) {
            controller_state.sideLedsActiveLow = stored_polarity != 0;
        }
    }
#endif
    if (!modes_loaded && (current_hardware_config || legacy_hardware_config)) {
        for (uint8_t layout = 0; layout < LAYOUT_COUNT; ++layout) {
            uint8_t mode = (config >> (layout * 2)) & 0x03;
            controller_state.layoutModes[layout] = mode < JOYSTICK_MODE_COUNT ? mode : JOYSTICK_MODE_ANALOG;
        }
    }
    uint8_t side_led_level = (config >> REPLICAZERON_MODE_CONFIG_SIDE_LED_SHIFT) & REPLICAZERON_MODE_CONFIG_SIDE_LED_MASK;
    controller_state.sideLedBrightness = (side_led_level == 0 ? 15 : MAX(2, side_led_level)) * 17;
    if (current_hardware_config) {
        controller_state.rgbLedCount = MIN((hardware_config & REPLICAZERON_MODE_CONFIG_LED_COUNT_MASK) + 1, RGB_MATRIX_LED_COUNT);
    } else if (legacy_hardware_config) {
        modes_need_write = true;
    }
    if (!modes_loaded || modes_need_write) {
        write_layout_modes();
    }
}

static void write_user_config(void) {
    uint32_t config = ((uint32_t)REPLICAZERON_USER_CONFIG_MAGIC << REPLICAZERON_USER_CONFIG_MAGIC_SHIFT) |
                      ((uint32_t)controller_state.openrgbEnabled << REPLICAZERON_USER_CONFIG_OPENRGB_SHIFT) |
                      ((uint32_t)!controller_state.sideLedsEnabled << REPLICAZERON_USER_CONFIG_SIDE_LEDS_DISABLED_SHIFT) |
                      ((uint32_t)controller_state.rgbStaticSelected << REPLICAZERON_USER_CONFIG_STATIC_SHIFT) |
                      ((uint32_t)(controller_state.rgbAnimationSelection & REPLICAZERON_USER_CONFIG_ANIMATION_MASK) << REPLICAZERON_USER_CONFIG_ANIMATION_SHIFT) |
                      ((uint32_t)(controller_state.rgbAnimationSelection >> 3) << REPLICAZERON_USER_CONFIG_ANIMATION_HIGH_SHIFT) |
                      ((uint32_t)controller_state.filterStrength << REPLICAZERON_USER_CONFIG_FILTER_SHIFT) |
                      (controller_state.deadzone & REPLICAZERON_USER_CONFIG_DEADZONE_MASK);
    eeconfig_update_user(config);
}

static void set_openrgb_enabled(bool enabled) {
    controller_state.openrgbEnabled = enabled;
    write_user_config();
#ifdef RGB_MATRIX_ENABLE
    if (!enabled) {
        rgb_matrix_reload_from_eeprom();
    }
#endif
}

#ifdef RGB_MATRIX_ENABLE
uint8_t replicazeron_rgb_led_count(void) {
    return controller_state.rgbLedCount;
}

static void apply_rgb_led_count(void) {
    for (uint8_t led = 0; led < RGB_MATRIX_LED_COUNT; ++led) {
        g_led_config.flags[led] = led < controller_state.rgbLedCount ? LED_FLAG_UNDERGLOW : 0;
        g_led_config.point[led].x = controller_state.rgbLedCount <= 1 ? 112 :
                                    ((uint32_t)MIN(led, controller_state.rgbLedCount - 1) * 224) /
                                        (controller_state.rgbLedCount - 1);
        g_led_config.point[led].y = 32;
    }
}

static void set_rgb_led_count(uint8_t count) {
    controller_state.rgbLedCount = MIN(MIN(MAX(count, 1), REPLICAZERON_RGB_LED_COUNT_MAX), RGB_MATRIX_LED_COUNT);
    apply_rgb_led_count();
    write_layout_modes();
}
#endif

#ifdef VIALRGB_ENABLE
bool vialrgb_allow_write_kb(uint8_t command) {
    (void)command;
    return controller_state.openrgbEnabled;
}

uint16_t vialrgb_get_number_leds_kb(void) {
    return controller_state.rgbLedCount;
}

bool vialrgb_get_led_info_kb(uint16_t led, uint8_t *output) {
    uint8_t strip_count = controller_state.rgbLedCount;
    if (led >= strip_count) {
        return false;
    }
    output[0] = strip_count <= 1 ? 112 : ((uint32_t)led * 224) / (strip_count - 1);
    output[1] = 16;
    output[2] = LED_FLAG_UNDERGLOW;
    output[3] = 0;
    output[4] = led;
    return true;
}
#endif

static void set_filter_strength(uint8_t strength) {
    controller_state.filterStrength = MIN(strength, 100);
    controller_state.filterCandidate = controller_state.filterStrength;
    write_user_config();
}

static void set_side_leds_enabled(bool enabled) {
    controller_state.sideLedsEnabled = enabled;
    write_user_config();
#ifdef LEDS_ENABLE
    if (enabled) {
        side_led_preview_requested = true;
        side_led_preview_started = timer_read32();
    } else {
        side_led_preview_requested = false;
        suspend_leds(controller_state.sideLedsActiveLow);
    }
#endif
}

static void set_side_led_brightness(uint8_t brightness) {
    uint8_t level = MAX(2, (brightness + 8) / 17);
    controller_state.sideLedBrightness = level * 17;
    write_layout_modes();
#ifdef LEDS_ENABLE
    if (controller_state.sideLedsEnabled) {
        side_led_preview_requested = true;
        side_led_preview_started = timer_read32();
    }
#endif
}

static void set_side_led_polarity(bool active_low) {
    controller_state.sideLedsActiveLow = active_low;
    write_layout_modes();
#ifdef LEDS_ENABLE
    side_led_preview_requested = controller_state.sideLedsEnabled;
    side_led_preview_started = timer_read32();
    if (!controller_state.sideLedsEnabled) {
        suspend_leds(controller_state.sideLedsActiveLow);
    }
#endif
}

static void set_side_led_source(bool led_a, uint8_t source) {
    if (source >= SIDE_LED_SOURCE_COUNT) {
        return;
    }
    if (led_a) {
        controller_state.sideLedSourceA = source;
    } else {
        controller_state.sideLedSourceB = source;
    }
    write_layout_modes();
#ifdef LEDS_ENABLE
    side_led_preview_requested = controller_state.sideLedsEnabled;
    side_led_preview_started = timer_read32();
#endif
}

static void change_side_led_source(bool led_a, bool increase) {
    uint8_t source = led_a ? controller_state.sideLedSourceA : controller_state.sideLedSourceB;
    source = increase ? (source + 1) % SIDE_LED_SOURCE_COUNT : (source == 0 ? SIDE_LED_SOURCE_COUNT - 1 : source - 1);
    set_side_led_source(led_a, source);
}

static void change_side_led_brightness(bool increase) {
    uint8_t brightness = controller_state.sideLedBrightness;
    if (increase) {
        brightness = brightness > UINT8_MAX - 17 ? UINT8_MAX : brightness + 17;
    } else {
        brightness = brightness <= 34 ? 34 : brightness - 17;
    }
    set_side_led_brightness(brightness);
}

static void set_deadzone(uint16_t deadzone) {
    if (deadzone == 0) {
        deadzone = 1;
    } else if (deadzone >= _SHIFTZONE) {
        deadzone = _SHIFTZONE - 1;
    }
    controller_state.deadzone = deadzone;
    write_user_config();
}

#ifdef VIA_ENABLE
#    define REPLICAZERON_TITLE_COMMAND 0x70
#    define REPLICAZERON_TITLE_GET 0x00
#    define REPLICAZERON_TITLE_SET 0x01
#    define REPLICAZERON_DEADZONE_GET 0x02
#    define REPLICAZERON_DEADZONE_SET 0x03
#    define REPLICAZERON_RGB_GET 0x04
#    define REPLICAZERON_RGB_SET 0x05
#    define REPLICAZERON_FILTER_GET 0x06
#    define REPLICAZERON_FILTER_SET 0x07
#    define REPLICAZERON_MODE_GET 0x08
#    define REPLICAZERON_MODE_SET 0x09
#    define REPLICAZERON_SIDE_LEDS_GET 0x0A
#    define REPLICAZERON_SIDE_LEDS_SET 0x0B
#    define REPLICAZERON_BOOTLOADER_REQUEST 0x0C

char replicazeron_titles[REPLICAZERON_TITLE_COUNT][REPLICAZERON_TITLE_LENGTH] = {
    "Casual       ",
    "Shooter      ",
    "Misc         ",
    "Empty 4      ",
    "Empty 5      ",
    "Empty 6      ",
    "Empty 7      ",
    "Empty 8      ",
    "Empty 9      ",
    "Empty 10     ",
    "Settings     ",
};
static const uint8_t title_signature[REPLICAZERON_TITLE_SIGNATURE_SIZE] = {'R', 'T'};

static void sanitize_title(char *title) {
    for (uint8_t index = 0; index < REPLICAZERON_TITLE_LENGTH; index++) {
        if (title[index] < ' ' || title[index] > '~') {
            title[index] = ' ';
        }
    }
}

static void read_titles_from_eeprom(void *buffer, uint32_t offset, uint32_t length) {
    eeprom_read_block(buffer, (void *)(uintptr_t)(VIA_EEPROM_CUSTOM_CONFIG_ADDR + offset), length);
}

static void write_titles_to_eeprom(const void *buffer, uint32_t offset, uint32_t length) {
    eeprom_update_block(buffer, (void *)(uintptr_t)(VIA_EEPROM_CUSTOM_CONFIG_ADDR + offset), length);
}

void via_init_kb(void) {
    for (uint8_t layout = 0; layout < LAYOUT_COUNT; layout++) {
        sanitize_title(replicazeron_titles[layout]);
    }

    uint8_t stored_signature[REPLICAZERON_TITLE_SIGNATURE_SIZE];
    read_titles_from_eeprom(stored_signature, 0, REPLICAZERON_TITLE_SIGNATURE_SIZE);
    bool title_storage_valid = stored_signature[0] == title_signature[0] && stored_signature[1] == title_signature[1];
    if (title_storage_valid) {
        /* Vial normally derives its EEPROM marker from a random BUILD_ID, so
         * every newly compiled firmware would reset the saved dynamic keymap
         * to the hardcoded defaults. Our custom-data signature lives after
         * the dynamic keymap and therefore also verifies that the EEPROM
         * layout is compatible. Migrate the marker to this build before
         * Vial decides whether it should reset mappings and macros. */
        if (!via_eeprom_is_valid()) {
            via_eeprom_set_valid(true);
        }
        read_titles_from_eeprom(replicazeron_titles, REPLICAZERON_TITLE_STORAGE_OFFSET, REPLICAZERON_TITLE_STORAGE_SIZE);
        for (uint8_t layout = 0; layout < LAYOUT_COUNT; layout++) {
            sanitize_title(replicazeron_titles[layout]);
        }
    } else {
        write_titles_to_eeprom(title_signature, 0, REPLICAZERON_TITLE_SIGNATURE_SIZE);
        write_titles_to_eeprom(replicazeron_titles, REPLICAZERON_TITLE_STORAGE_OFFSET, REPLICAZERON_TITLE_STORAGE_SIZE);
    }
}

void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    if (length != 32 || data[0] != REPLICAZERON_TITLE_COMMAND) {
        data[0] = id_unhandled;
        return;
    }

    if (data[1] == REPLICAZERON_DEADZONE_GET) {
        data[3] = controller_state.deadzone >> 8;
        data[4] = controller_state.deadzone & 0xFF;
        uprintf("DEADZONE_GET -> %u\n",
        controller_state.deadzone);
    } else if (data[1] == REPLICAZERON_DEADZONE_SET) {
        set_deadzone((data[3] << 8) | data[4]);
    } else if (data[1] == REPLICAZERON_RGB_GET) {
#ifdef RGB_MATRIX_ENABLE
        if (controller_state.openrgbEnabled) {
            rgb_config_t firmware_rgb;
            eeconfig_read_rgb_matrix(&firmware_rgb);
            controller_state.rgbStaticSelected = firmware_rgb.mode == RGB_MATRIX_SOLID_COLOR;
            if (!controller_state.rgbStaticSelected) {
                controller_state.rgbAnimationSelection = rgb_animation_from_mode(firmware_rgb.mode);
            }
            data[3] = firmware_rgb.enable;
            data[6] = firmware_rgb.hsv.v;
            data[7] = MIN(3, firmware_rgb.speed / 64);
            data[8] = firmware_rgb.hsv.h;
        } else {
            if (rgb_matrix_is_enabled()) {
                controller_state.rgbStaticSelected = rgb_matrix_get_mode() == RGB_MATRIX_SOLID_COLOR;
                if (!controller_state.rgbStaticSelected) {
                    controller_state.rgbAnimationSelection = current_rgb_animation();
                }
            }
            data[3] = rgb_matrix_is_enabled();
            data[6] = rgb_matrix_get_val();
            data[7] = replicazeron_rgb_speed_level();
            data[8] = rgb_matrix_get_hue();
        }
        data[4] = controller_state.rgbStaticSelected;
        data[5] = controller_state.rgbAnimationSelection;
        data[9] = controller_state.openrgbEnabled;
        data[10] = controller_state.rgbLedCount;
#else
        data[0] = id_unhandled;
#endif
    } else if (data[1] == REPLICAZERON_RGB_SET) {
#ifdef RGB_MATRIX_ENABLE
        if (data[4] > 1 || data[5] >= RGB_ANIMATION_COUNT || data[7] > 3 || data[9] > 1 || data[10] > REPLICAZERON_RGB_LED_COUNT_MAX) {
            data[0] = id_unhandled;
        } else {
            bool enabled = data[3] != 0;
            controller_state.rgbStaticSelected = data[4] != 0;
            controller_state.rgbAnimationSelection = data[5];
            if (data[10] != 0) {
                set_rgb_led_count(data[10]);
            }
            set_openrgb_enabled(data[9] != 0);

            if (!controller_state.openrgbEnabled) {
                /* Set HSV while static: QMK's breathing effect deliberately
                 * ignores live brightness writes. Apply the final mode last. */
                rgb_matrix_enable();
                rgb_matrix_mode(RGB_MATRIX_SOLID_COLOR);
                rgb_matrix_set_speed((data[7] * 64) + 31);
                rgb_matrix_sethsv(data[8], rgb_matrix_get_sat(), data[6]);
                if (!controller_state.rgbStaticSelected) {
                    rgb_matrix_mode(rgb_animation_mode(controller_state.rgbAnimationSelection));
                }
                if (!enabled) {
                    rgb_matrix_disable();
                }
            }
        }
#else
        data[0] = id_unhandled;
#endif
    } else if (data[1] == REPLICAZERON_FILTER_GET) {
        data[3] = controller_state.filterStrength;
    } else if (data[1] == REPLICAZERON_FILTER_SET) {
        if (data[3] > 100) {
            data[0] = id_unhandled;
        } else {
            set_filter_strength(data[3]);
        }
    } else if (data[1] == REPLICAZERON_MODE_GET) {
        if (data[2] >= LAYOUT_COUNT) {
            data[0] = id_unhandled;
        } else {
            data[3] = controller_state.layoutModes[data[2]];
        }
    } else if (data[1] == REPLICAZERON_MODE_SET) {
        if (data[2] >= LAYOUT_COUNT || data[3] >= JOYSTICK_MODE_COUNT) {
            data[0] = id_unhandled;
        } else {
            set_layout_mode(data[2], data[3]);
        }
    } else if (data[1] == REPLICAZERON_SIDE_LEDS_GET) {
        data[3] = controller_state.sideLedsEnabled;
        data[4] = controller_state.sideLedBrightness;
        data[5] = controller_state.sideLedsActiveLow;
        data[6] = controller_state.sideLedSourceA;
        data[7] = controller_state.sideLedSourceB;
    } else if (data[1] == REPLICAZERON_SIDE_LEDS_SET) {
        if (data[3] > 1 || data[5] > 1 || data[6] >= SIDE_LED_SOURCE_COUNT || data[7] >= SIDE_LED_SOURCE_COUNT) {
            data[0] = id_unhandled;
        } else {
            set_side_led_polarity(data[5] != 0);
            set_side_led_source(true, data[6]);
            set_side_led_source(false, data[7]);
            set_side_led_brightness(data[4]);
            set_side_leds_enabled(data[3] != 0);
        }
    } else if (data[1] == REPLICAZERON_BOOTLOADER_REQUEST) {
        /* Require an explicit DFU signature. Delay the reset so the Raw HID
         * acknowledgement reaches the browser before USB disconnects. */
        if (data[3] != 'D' || data[4] != 'F' || data[5] != 'U' || data[6] != '!' || data[7] > 1) {
            data[0] = id_unhandled;
        } else {
            bootloader_usb_disconnect_assist = data[7] != 0;
            bootloader_requested = true;
            bootloader_request_started = timer_read32();
            data[3] = 1;
        }
    } else if (data[2] >= REPLICAZERON_TITLE_COUNT) {
        data[0] = id_unhandled;
    } else if (data[1] == REPLICAZERON_TITLE_GET) {
        uint8_t layout = data[2];
        for (uint8_t index = 0; index < REPLICAZERON_TITLE_LENGTH; index++) {
            data[3 + index] = replicazeron_titles[layout][index];
        }
    } else if (data[1] == REPLICAZERON_TITLE_SET) {
        uint8_t layout = data[2];
        if (layout >= LAYOUT_COUNT) {
            data[0] = id_unhandled;
            raw_hid_send(data, length);
            return;
        }
        for (uint8_t index = 0; index < REPLICAZERON_TITLE_LENGTH; index++) {
            replicazeron_titles[layout][index] = data[3 + index];
        }
        sanitize_title(replicazeron_titles[layout]);
        write_titles_to_eeprom(replicazeron_titles, REPLICAZERON_TITLE_STORAGE_OFFSET, REPLICAZERON_TITLE_STORAGE_SIZE);
    } else {
        data[0] = id_unhandled;
    }
    raw_hid_send(data, length);
}
#endif

#ifdef JOYSTICK_ENABLE
joystick_config_t joystick_axes[JOYSTICK_AXIS_COUNT] = {
    JOYSTICK_AXIS_IN(ANALOG_AXIS_PIN_X , 0, 512, 1023),
    JOYSTICK_AXIS_IN(ANALOG_AXIS_PIN_Y , 0, 512, 1023)
};

static int16_t filtered_axes[JOYSTICK_AXIS_COUNT];

static uint16_t axis_magnitude(int16_t value) {
    return value < 0 ? -value : value;
}

static void apply_axis_filter(int16_t *x, int16_t *y, uint8_t strength) {
    if (strength == 0) {
        return;
    }

    uint16_t abs_x = axis_magnitude(*x);
    uint16_t abs_y = axis_magnitude(*y);
    int16_t *minor_axis;
    uint16_t dominant;
    uint16_t minor;

    if (abs_x >= abs_y) {
        dominant = abs_x;
        minor = abs_y;
        minor_axis = y;
    } else {
        dominant = abs_y;
        minor = abs_x;
        minor_axis = x;
    }

    uint16_t threshold = ((uint32_t)dominant * strength) / 100;
    if (minor <= threshold || threshold >= dominant) {
        *minor_axis = 0;
        return;
    }

    uint16_t filtered_minor = ((uint32_t)(minor - threshold) * dominant) / (dominant - threshold);
    *minor_axis = *minor_axis < 0 ? -(int16_t)filtered_minor : (int16_t)filtered_minor;
}

uint16_t joystick_axis_sample(uint8_t axis) {
    static uint16_t filtered_samples[JOYSTICK_AXIS_COUNT] = {512, 512};

    if (axis == 0) {
        int16_t x = (int16_t)analogReadPin(ANALOG_AXIS_PIN_X) - 512;
        int16_t y = (int16_t)analogReadPin(ANALOG_AXIS_PIN_Y) - 512;

#ifdef THUMBSTICK_ENABLE
        thumbstick_unfiltered_position = get_thumbstick_polar_position(x, y);
#endif
        apply_axis_filter(&x, &y, controller_state.filterStrength);
        filtered_axes[0] = x;
        filtered_axes[1] = y;
        filtered_samples[0] = (uint16_t)(filtered_axes[0] + 512);
        filtered_samples[1] = (uint16_t)(filtered_axes[1] + 512);
    }

    /* WASD profiles must not also expose a moving analog stick to games. The
     * physical values remain cached for the keyboard-emulation path below. */
    return controller_state.wasdMode ? 512 : filtered_samples[axis];
}
#endif

#ifdef THUMBSTICK_ENABLE
static bool any_matrix_key_pressed(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        if (matrix_get_row(row) != 0) {
            return true;
        }
    }
    return false;
}

static uint8_t side_led_stick_level(uint16_t joystick_distance) {
    uint16_t distance = MIN(joystick_distance, REPLICAZERON_SIDE_LED_PREVIEW_DISTANCE);
    return REPLICAZERON_SIDE_LED_MIN_LEVEL +
           ((uint32_t)distance * (UINT8_MAX - REPLICAZERON_SIDE_LED_MIN_LEVEL)) / REPLICAZERON_SIDE_LED_PREVIEW_DISTANCE;
}

static uint8_t side_led_source_level(uint8_t source, uint16_t joystick_distance, bool key_pressed) {
    switch (source) {
        case SIDE_LED_SOURCE_STICK:
            return side_led_stick_level(joystick_distance);
        case SIDE_LED_SOURCE_BUTTONS:
            return key_pressed ? UINT8_MAX : 0;
        case SIDE_LED_SOURCE_ACTIVITY:
            return key_pressed ? UINT8_MAX : side_led_stick_level(joystick_distance);
        case SIDE_LED_SOURCE_ALWAYS_ON:
            return UINT8_MAX;
        case SIDE_LED_SOURCE_CAPS_LOCK:
            return host_keyboard_led_state().caps_lock ? UINT8_MAX : 0;
        case SIDE_LED_SOURCE_NUM_LOCK:
            return host_keyboard_led_state().num_lock ? UINT8_MAX : 0;
        case SIDE_LED_SOURCE_SCROLL_LOCK:
            return host_keyboard_led_state().scroll_lock ? UINT8_MAX : 0;
        default:
            return 0;
    }
}

void housekeeping_task_kb(void) {
    if (bootloader_requested && timer_elapsed32(bootloader_request_started) >= REPLICAZERON_BOOTLOADER_DELAY_MS) {
        bootloader_requested = false;
        if (bootloader_usb_disconnect_assist) {
            /* STM32F103 clone boards often need a host-visible detach before
             * reset. The STM32duino board definition implements this by
             * holding USB D+ (PA12) low, which emulates unplugging the cable. */
            usb_disconnect();
            wait_ms(250);
        }
        reset_keyboard();
    }

    if (controller_state.menuState == MENU_FACTORY_RESET && reset_little_held && reset_index_held) {
        if (!reset_combo_armed) {
            reset_combo_armed = true;
            reset_combo_timer = timer_read32();
        } else if (timer_elapsed32(reset_combo_timer) >= REPLICAZERON_FACTORY_RESET_HOLD_MS) {
            eeconfig_disable();
            soft_reset_keyboard();
        }
    } else {
        reset_combo_armed = false;
    }

    update_thumbstick_position(filtered_axes[0], filtered_axes[1]);
#    ifdef RGB_MATRIX_ENABLE
    replicazeron_rgb_set_joystick_activity(filtered_axes[0], filtered_axes[1]);
#    endif
    if (controller_state.wasdMode) {
        thumbstick(controller_state, filtered_axes[0], filtered_axes[1]);
    }
#    ifdef LEDS_ENABLE
    if (side_led_preview_requested && timer_elapsed32(side_led_preview_started) >= REPLICAZERON_SIDE_LED_PREVIEW_MS) {
        side_led_preview_requested = false;
    }
    bool side_led_preview = controller_state.menuState == MENU_SIDE_LEDS || side_led_preview_requested;
    bool key_pressed = any_matrix_key_pressed();
    uint8_t level_a = side_led_preview ? UINT8_MAX : side_led_source_level(controller_state.sideLedSourceA, thumbstick_unfiltered_position.distance, key_pressed);
    uint8_t level_b = side_led_preview ? UINT8_MAX : side_led_source_level(controller_state.sideLedSourceB, thumbstick_unfiltered_position.distance, key_pressed);
    update_leds(level_a, level_b, controller_state.sideLedsEnabled, controller_state.sideLedsActiveLow, controller_state.sideLedBrightness);
#    endif
}
#endif

static void menu_close(void) {
    controller_state.menuState = MENU_NONE;
}

static void menu_open(void) {
    controller_state.menuState = MENU_MAIN;
    controller_state.menuSelection = 0;
}

static void cycle_layout_mode(uint8_t layout) {
    if (layout < LAYOUT_COUNT) {
        set_layout_mode(layout, (controller_state.layoutModes[layout] + 1) % JOYSTICK_MODE_COUNT);
    }
}

#ifdef RGB_MATRIX_ENABLE
static uint8_t rgb_animation_mode(rgb_animation_id_t animation) {
    switch (animation) {
        case RGB_ANIMATION_RAINBOW_MOOD:
            return RGB_MATRIX_CYCLE_ALL;
        case RGB_ANIMATION_RAINBOW_SWIRL:
            return RGB_MATRIX_CYCLE_SPIRAL;
        case RGB_ANIMATION_KNIGHT:
            return RGB_MATRIX_CUSTOM_KNIGHT_RIDER;
        case RGB_ANIMATION_TWINKLE:
            return RGB_MATRIX_JELLYBEAN_RAINDROPS;
        case RGB_ANIMATION_MOVING_RAINBOW:
            return RGB_MATRIX_RAINBOW_MOVING_CHEVRON;
        case RGB_ANIMATION_HUE_WAVE:
            return RGB_MATRIX_HUE_WAVE;
        case RGB_ANIMATION_HUE_PENDULUM:
            return RGB_MATRIX_HUE_PENDULUM;
        case RGB_ANIMATION_CYLON:
            return RGB_MATRIX_CUSTOM_CYLON;
        case RGB_ANIMATION_PULSE:
            return RGB_MATRIX_CUSTOM_PULSE;
        case RGB_ANIMATION_REACTIVE_PULSE:
            return RGB_MATRIX_CUSTOM_REACTIVE_PULSE;
        case RGB_ANIMATION_BREATHING:
        default:
            return RGB_MATRIX_BREATHING;
    }
}

static rgb_animation_id_t rgb_animation_from_mode(uint8_t mode) {
    switch (mode) {
        case RGB_MATRIX_CYCLE_ALL:
            return RGB_ANIMATION_RAINBOW_MOOD;
        case RGB_MATRIX_CYCLE_SPIRAL:
            return RGB_ANIMATION_RAINBOW_SWIRL;
        case RGB_MATRIX_CUSTOM_KNIGHT_RIDER:
            return RGB_ANIMATION_KNIGHT;
        case RGB_MATRIX_JELLYBEAN_RAINDROPS:
            return RGB_ANIMATION_TWINKLE;
        case RGB_MATRIX_RAINBOW_MOVING_CHEVRON:
            return RGB_ANIMATION_MOVING_RAINBOW;
        case RGB_MATRIX_HUE_WAVE:
            return RGB_ANIMATION_HUE_WAVE;
        case RGB_MATRIX_HUE_PENDULUM:
            return RGB_ANIMATION_HUE_PENDULUM;
        case RGB_MATRIX_CUSTOM_CYLON:
            return RGB_ANIMATION_CYLON;
        case RGB_MATRIX_CUSTOM_PULSE:
            return RGB_ANIMATION_PULSE;
        case RGB_MATRIX_CUSTOM_REACTIVE_PULSE:
            return RGB_ANIMATION_REACTIVE_PULSE;
        default:
            return RGB_ANIMATION_BREATHING;
    }
}

static rgb_animation_id_t current_rgb_animation(void) {
    return rgb_animation_from_mode(rgb_matrix_get_mode());
}

static void apply_rgb_animation(rgb_animation_id_t animation) {
    rgb_matrix_enable();
    rgb_matrix_mode(rgb_animation_mode(animation));
}

static void change_rgb_animation_value(bool increase) {
    uint8_t value = rgb_matrix_get_val();
    if (increase) {
        value = value > UINT8_MAX - RGB_MATRIX_VAL_STEP ? UINT8_MAX : value + RGB_MATRIX_VAL_STEP;
    } else {
        value = value < RGB_MATRIX_VAL_STEP ? 0 : value - RGB_MATRIX_VAL_STEP;
    }

    rgb_matrix_sethsv(rgb_matrix_get_hue(), rgb_matrix_get_sat(), value);
    apply_rgb_animation(controller_state.rgbAnimationSelection);
}

uint8_t replicazeron_rgb_speed_level(void) {
    return MIN(3, rgb_matrix_get_speed() / 64);
}

bool replicazeron_rgb_animation_uses_hue(rgb_animation_id_t animation) {
    switch (animation) {
        case RGB_ANIMATION_BREATHING:
        case RGB_ANIMATION_KNIGHT:
        case RGB_ANIMATION_HUE_WAVE:
        case RGB_ANIMATION_HUE_PENDULUM:
        case RGB_ANIMATION_CYLON:
        case RGB_ANIMATION_PULSE:
        case RGB_ANIMATION_REACTIVE_PULSE:
            return true;
        default:
            return false;
    }
}

static void change_rgb_animation_speed(bool increase) {
    uint8_t speed = replicazeron_rgb_speed_level();
    if (increase) {
        speed = MIN(3, speed + 1);
    } else if (speed > 0) {
        speed--;
    }
    rgb_matrix_set_speed((speed * 64) + 31);
    apply_rgb_animation(controller_state.rgbAnimationSelection);
}

static void change_rgb_animation_hue(bool increase) {
    if (increase) {
        rgb_matrix_increase_hue();
    } else {
        rgb_matrix_decrease_hue();
    }
    apply_rgb_animation(controller_state.rgbAnimationSelection);
}
#endif

static void menu_move(int8_t delta) {
    if (controller_state.menuState == MENU_MAIN) {
        int8_t next = controller_state.menuSelection + delta;
        if (next < 0) {
            next = MAIN_MENU_ITEM_COUNT - 1;
        } else if (next >= MAIN_MENU_ITEM_COUNT) {
            next = 0;
        }
        controller_state.menuSelection = next;
    } else if (controller_state.menuState == MENU_LAYOUT_BROWSER) {
        int8_t next = controller_state.menuSelection + delta;
        if (next < 0) {
            next = LAYOUT_COUNT - 1;
        } else if (next >= LAYOUT_COUNT) {
            next = 0;
        }
        controller_state.menuSelection = next;
    } else if (controller_state.menuState == MENU_MODE_LAYOUT) {
        int8_t next = controller_state.menuSelection + delta;
        if (next < 0) {
            next = LAYOUT_COUNT - 1;
        } else if (next >= LAYOUT_COUNT) {
            next = 0;
        }
        controller_state.menuSelection = next;
    } else if (controller_state.menuState == MENU_RGB_TYPE) {
        int8_t next = controller_state.menuSelection + delta;
        if (next < 0) {
            next = RGB_TYPE_COUNT - 1;
        } else if (next >= RGB_TYPE_COUNT) {
            next = 0;
        }
        controller_state.menuSelection = next;
    } else if (controller_state.menuState == MENU_RGB_CONTROL) {
        controller_state.menuSelection = controller_state.menuSelection == 0 ? 1 : 0;
    } else if (controller_state.menuState == MENU_SIDE_LEDS) {
        int8_t next = controller_state.menuSelection + delta;
        if (next < 0) {
            next = SIDE_LED_MENU_ITEM_COUNT - 1;
        } else if (next >= SIDE_LED_MENU_ITEM_COUNT) {
            next = 0;
        }
        controller_state.menuSelection = next;
    } else if (controller_state.menuState == MENU_RGB_ANIMATION_BROWSER) {
        int8_t next = controller_state.menuSelection + delta;
        if (next < 0) {
            next = RGB_ANIMATION_COUNT - 1;
        } else if (next >= RGB_ANIMATION_COUNT) {
            next = 0;
        }
        controller_state.menuSelection = next;
#ifdef RGB_MATRIX_ENABLE
        if (rgb_matrix_is_enabled()) {
            rgb_matrix_mode_noeeprom(rgb_animation_mode(next));
        }
#endif
    } else if (controller_state.menuState == MENU_CALIB) {
        int8_t next = controller_state.menuSelection + delta;
        if (next < 0) {
            next = CALIBRATION_ITEM_COUNT - 1;
        } else if (next >= CALIBRATION_ITEM_COUNT) {
            next = 0;
        }
        controller_state.menuSelection = next;
    }
}

static void activate_layout(uint8_t layout) {
    if (layout >= LAYOUT_COUNT) {
        layout = LAYOUT_1;
    }
    controller_state.activeLayout = layout;
    controller_state.layoutSelection = layout;
    layer_move(layout);
}

static void advance_layout_key(void) {
    uint8_t layer = controller_state.highestActiveLayer;
    if (layer > _SETTINGS) {
        layer = _LAYOUT_1;
    } else {
        layer = (layer + 1) % (_SETTINGS + 1);
    }

    if (layer < LAYOUT_COUNT) {
        controller_state.activeLayout = layer;
        controller_state.layoutSelection = layer;
        apply_layout_mode(layer);
    }
    layer_move(layer);
}

static void menu_select(void) {
    switch (controller_state.menuState) {
        case MENU_MAIN:
            switch (controller_state.menuSelection) {
                case 0:
                    controller_state.menuState = MENU_LAYOUT_BROWSER;
                    controller_state.menuSelection = controller_state.activeLayout;
                    break;
                case 1:
                    controller_state.menuState = MENU_RGB_TYPE;
#ifdef RGB_MATRIX_ENABLE
                    uint8_t rgb_mode = rgb_matrix_get_mode();
                    if (rgb_mode == RGB_MATRIX_SOLID_COLOR) {
                        controller_state.rgbStaticSelected = true;
                        controller_state.menuSelection = 2;
                    } else if (rgb_mode > RGB_MATRIX_SOLID_COLOR) {
                        controller_state.rgbStaticSelected = false;
                        controller_state.rgbAnimationSelection = current_rgb_animation();
                        controller_state.menuSelection = 1;
                    } else {
                        controller_state.menuSelection = controller_state.rgbStaticSelected ? 2 : 1;
                    }
#else
                    controller_state.menuSelection = 0;
#endif
                    break;
                case 2:
                    controller_state.menuState = MENU_SIDE_LEDS;
                    controller_state.menuSelection = 0;
                    break;
                case 3:
                    controller_state.menuState = MENU_MODE_LAYOUT;
                    controller_state.menuSelection = controller_state.activeLayout;
                    break;
                case 4:
                    controller_state.menuState = MENU_CALIB;
                    controller_state.menuSelection = 0;
                    break;
                case 5:
                    controller_state.menuState = MENU_FACTORY_RESET;
                    reset_little_held = false;
                    reset_index_held = false;
                    reset_combo_armed = false;
                    break;
                default:
                    controller_state.menuState = MENU_NONE;
                    break;
            }
            break;
        case MENU_LAYOUT_BROWSER:
            activate_layout(controller_state.menuSelection);
            menu_close();
            break;
        case MENU_RGB_TYPE:
            if (controller_state.menuSelection == 0) {
                controller_state.menuState = MENU_RGB_CONTROL;
                controller_state.menuSelection = controller_state.openrgbEnabled ? 1 : 0;
            } else if (controller_state.menuSelection == 1) {
                set_openrgb_enabled(false);
                controller_state.rgbStaticSelected = false;
                write_user_config();
                controller_state.menuState = MENU_RGB_ANIMATION_BROWSER;
                controller_state.menuSelection = controller_state.rgbAnimationSelection;
#ifdef RGB_MATRIX_ENABLE
                apply_rgb_animation(controller_state.rgbAnimationSelection);
#endif
            } else {
                set_openrgb_enabled(false);
                controller_state.rgbStaticSelected = true;
                write_user_config();
#ifdef RGB_MATRIX_ENABLE
                rgb_matrix_enable();
                rgb_matrix_mode(RGB_MATRIX_SOLID_COLOR);
#endif
                controller_state.menuState = MENU_RGB_STATIC;
            }
            break;
        case MENU_RGB_CONTROL:
            set_openrgb_enabled(controller_state.menuSelection == 1);
            controller_state.menuState = controller_state.openrgbEnabled ? MENU_RGB_OPENRGB_INFO : MENU_RGB_TYPE;
            controller_state.menuSelection = 0;
            break;
        case MENU_RGB_OPENRGB_INFO:
            controller_state.menuState = MENU_RGB_TYPE;
            controller_state.menuSelection = 0;
            break;
        case MENU_RGB_ANIMATION_BROWSER:
            controller_state.rgbAnimationSelection = controller_state.menuSelection;
            write_user_config();
#ifdef RGB_MATRIX_ENABLE
            apply_rgb_animation(controller_state.rgbAnimationSelection);
#endif
            controller_state.menuState = MENU_RGB_ANIMATION_SETTINGS;
            controller_state.menuSelection = 0;
            break;
        case MENU_RGB_ANIMATION_SETTINGS:
            controller_state.menuState = MENU_RGB_ANIMATION_BROWSER;
            controller_state.menuSelection = controller_state.rgbAnimationSelection;
            break;
        case MENU_SIDE_LEDS:
            if (controller_state.menuSelection == 0) {
                set_side_leds_enabled(!controller_state.sideLedsEnabled);
            }
            break;
        case MENU_MODE_LAYOUT:
            controller_state.layoutSelection = controller_state.menuSelection;
            controller_state.menuState = MENU_MODE;
            break;
        case MENU_CALIB:
            if (controller_state.menuSelection == 0) {
                controller_state.menuState = MENU_CALIB_DEADZONE;
            } else {
                controller_state.filterCandidate = controller_state.filterStrength;
                controller_state.menuState = MENU_CALIB_FILTER;
            }
            break;
        default:
            menu_close();
            break;
    }
}

void keyboard_post_init_kb(void) {
    // Customise these values to desired behaviour
#ifdef VIAL_ENABLE
    debug_enable = false;
    debug_matrix = false;
#else
    debug_enable = true;
    debug_matrix = true;
#endif
    // debug_keyboard = true;
    // debug_mouse = true;

    controller_state = init_state();
    load_layout_modes();
#ifdef RGB_MATRIX_ENABLE
    apply_rgb_led_count();
#endif

#ifdef LEDS_ENABLE
    init_leds(controller_state.sideLedsActiveLow);
#endif // LEDS_ENABLE

#ifdef THUMBSTICK_ENABLE
    init_wasd_state();
#endif // THUMBSTICK_ENABLE

    apply_layout_mode(controller_state.activeLayout);

    uint32_t stored_config = eeconfig_read_user();
    uint32_t stored_deadzone = stored_config;
    uint8_t stored_magic = stored_config >> REPLICAZERON_USER_CONFIG_MAGIC_SHIFT;
    if (stored_magic == REPLICAZERON_USER_CONFIG_MAGIC) {
        stored_deadzone = stored_config & REPLICAZERON_USER_CONFIG_DEADZONE_MASK;
        controller_state.filterStrength = MIN(100, (stored_config >> REPLICAZERON_USER_CONFIG_FILTER_SHIFT) & REPLICAZERON_USER_CONFIG_FILTER_MASK);
        controller_state.filterCandidate = controller_state.filterStrength;
        controller_state.openrgbEnabled = ((stored_config >> REPLICAZERON_USER_CONFIG_OPENRGB_SHIFT) & 1) != 0;
        controller_state.sideLedsEnabled = ((stored_config >> REPLICAZERON_USER_CONFIG_SIDE_LEDS_DISABLED_SHIFT) & 1) == 0;
        controller_state.rgbStaticSelected = ((stored_config >> REPLICAZERON_USER_CONFIG_STATIC_SHIFT) & 1) != 0;
        uint8_t stored_animation = ((stored_config >> REPLICAZERON_USER_CONFIG_ANIMATION_SHIFT) & REPLICAZERON_USER_CONFIG_ANIMATION_MASK) |
                                   (((stored_config >> REPLICAZERON_USER_CONFIG_ANIMATION_HIGH_SHIFT) & 1) << 3);
        if (stored_animation < RGB_ANIMATION_COUNT) {
            controller_state.rgbAnimationSelection = stored_animation;
        }
    } else if (stored_magic == REPLICAZERON_USER_CONFIG_LEGACY_MAGIC) {
        stored_deadzone = stored_config & 0xFFFF;
        uint8_t stored_rgb = (stored_config >> 16) & 0xFF;
        controller_state.rgbStaticSelected = (stored_rgb & REPLICAZERON_USER_CONFIG_LEGACY_STATIC_MASK) != 0;
        uint8_t stored_animation = stored_rgb & ~REPLICAZERON_USER_CONFIG_LEGACY_STATIC_MASK;
        if (stored_animation < RGB_ANIMATION_COUNT) {
            controller_state.rgbAnimationSelection = stored_animation;
        }
    }
    if (stored_deadzone > 0 && stored_deadzone < _SHIFTZONE) {
        controller_state.deadzone = (uint16_t)stored_deadzone;
    }

#ifdef RGB_MATRIX_ENABLE
    rgb_config_t stored_rgb_matrix;
    eeconfig_read_rgb_matrix(&stored_rgb_matrix);
    if (stored_rgb_matrix.enable > 1 || stored_rgb_matrix.mode == RGB_MATRIX_NONE || stored_rgb_matrix.mode >= RGB_MATRIX_EFFECT_MAX) {
        /* Existing RGBLIGHT builds never initialized the RGB Matrix EEPROM
         * slot. Seed it without touching Vial mappings or custom settings. */
        eeconfig_update_rgb_matrix_default();
        rgb_matrix_reload_from_eeprom();
    }

    uint8_t rgb_mode = rgb_matrix_get_mode();
    if (rgb_mode == RGB_MATRIX_SOLID_COLOR) {
        controller_state.rgbStaticSelected = true;
    } else if (rgb_mode > RGB_MATRIX_SOLID_COLOR) {
        controller_state.rgbStaticSelected = false;
        controller_state.rgbAnimationSelection = current_rgb_animation();
    }
#endif

    keyboard_post_init_user();
}

#ifdef RGB_MATRIX_ENABLE
bool rgb_matrix_indicators_advanced_kb(uint8_t led_min, uint8_t led_max) {
    bool result = rgb_matrix_indicators_advanced_user(led_min, led_max);
    uint8_t first_inactive = MAX(led_min, controller_state.rgbLedCount);
    for (uint8_t led = first_inactive; led < led_max; ++led) {
        rgb_matrix_set_color(led, 0, 0, 0);
    }
    return result;
}
#endif

void suspend_power_down_kb(void) {
#ifdef LEDS_ENABLE
    suspend_leds(controller_state.sideLedsActiveLow);
#endif
    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
#ifdef LEDS_ENABLE
    init_leds(controller_state.sideLedsActiveLow);
#endif
    suspend_wakeup_init_user();
}

#ifdef OLED_ENABLE
bool oled_task_kb(void) {
    if (!oled_task_user()) {
        return false;
    }

    draw_oled(controller_state);
    return false;
}
#endif

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
#ifdef RGB_MATRIX_ENABLE
    if (record->event.pressed) {
        replicazeron_rgb_reactive_trigger();
    }
#endif

    /* The layout key is firmware-owned so Vial remapping cannot remove either
     * its tap action or its per-profile mode shortcut. */
    if (record->event.key.row == REPLICAZERON_LAYOUT_KEY_ROW && record->event.key.col == REPLICAZERON_LAYOUT_KEY_COL) {
        if (record->event.pressed) {
            layout_key_timer = timer_read();
        } else if (timer_elapsed(layout_key_timer) >= REPLICAZERON_LAYOUT_KEY_HOLD_MS) {
            cycle_layout_mode(controller_state.activeLayout);
        } else {
            advance_layout_key();
        }
        return false;
    }

    /* Never execute EEPROM clear from one key. Existing Vial mappings that
     * still contain EE_CLR are redirected to the guarded confirmation screen. */
    if (keycode == QK_CLEAR_EEPROM) {
        if (record->event.pressed) {
            controller_state.menuState = MENU_FACTORY_RESET;
            reset_little_held = false;
            reset_index_held = false;
            reset_combo_armed = false;
        }
        return false;
    }

    /* OLED navigation belongs to the physical D-pad while a menu is open, so
     * Vial remapping cannot make a settings screen impossible to operate. */
    if (controller_state.menuState != MENU_NONE) {
        uint8_t row = record->event.key.row;
        uint8_t col = record->event.key.col;
        if (row == REPLICAZERON_DPAD_RIGHT_ROW && col == REPLICAZERON_DPAD_RIGHT_COL) {
            keycode = KC_RIGHT;
        } else if (row == REPLICAZERON_DPAD_SELECT_ROW && col == REPLICAZERON_DPAD_SELECT_COL) {
            keycode = MENU_TOGGLE;
        } else if (row == REPLICAZERON_DPAD_DOWN_ROW && col == REPLICAZERON_DPAD_DOWN_COL) {
            keycode = KC_DOWN;
        } else if (row == REPLICAZERON_DPAD_LEFT_ROW && col == REPLICAZERON_DPAD_LEFT_COL) {
            keycode = KC_LEFT;
        } else if (row == REPLICAZERON_DPAD_UP_ROW && col == REPLICAZERON_DPAD_UP_COL) {
            keycode = KC_UP;
        }
    }

    if (!process_record_user(keycode, record)) {
        return false;
    }

#ifdef RGB_MATRIX_ENABLE
    if (controller_state.openrgbEnabled &&
        ((keycode >= QK_UNDERGLOW_TOGGLE && keycode <= RGB_MODE_TWINKLE) || IS_RGB_MATRIX_KEYCODE(keycode))) {
        return false;
    }
#endif

    if (controller_state.menuState == MENU_CALIB_DEADZONE && record->event.pressed) {
        set_deadzone((uint16_t)thumbstick_unfiltered_position.distance);
        controller_state.menuState = MENU_CALIB;
        controller_state.menuSelection = 0;
        return false;
    }

    if (keycode == MENU_TOGGLE && record->event.pressed) {
        if (controller_state.menuState == MENU_NONE) {
            menu_open();
        } else if (controller_state.menuState == MENU_RGB_STATIC) {
#ifdef RGB_MATRIX_ENABLE
            rgb_matrix_toggle();
#endif
        } else if (controller_state.menuState == MENU_RGB_ANIMATION_SETTINGS) {
            controller_state.menuState = MENU_RGB_ANIMATION_BROWSER;
            controller_state.menuSelection = controller_state.rgbAnimationSelection;
        } else if (controller_state.menuState == MENU_SIDE_LEDS || controller_state.menuState == MENU_FACTORY_RESET) {
            menu_close();
        } else if (controller_state.menuState == MENU_MODE) {
            cycle_layout_mode(controller_state.layoutSelection);
        } else if (controller_state.menuState == MENU_CALIB_FILTER) {
            set_filter_strength(controller_state.filterCandidate);
            controller_state.menuState = MENU_CALIB;
            controller_state.menuSelection = 1;
        } else {
            menu_select();
        }
        return false;
    }

    if (controller_state.menuState != MENU_NONE) {
        bool reset_little_key = record->event.key.row == REPLICAZERON_RESET_LITTLE_ROW && record->event.key.col == REPLICAZERON_RESET_LITTLE_COL;
        bool reset_index_key = record->event.key.row == REPLICAZERON_RESET_INDEX_ROW && record->event.key.col == REPLICAZERON_RESET_INDEX_COL;
        if (controller_state.menuState == MENU_FACTORY_RESET && (reset_little_key || reset_index_key)) {
            if (reset_little_key) {
                reset_little_held = record->event.pressed;
            } else {
                reset_index_held = record->event.pressed;
            }
            return false;
        }

        if (!record->event.pressed) {
            return false;
        }

        switch (keycode) {
            case KC_UP:
                if (controller_state.menuState == MENU_RGB_ANIMATION_SETTINGS) {
                    uint8_t field_count = replicazeron_rgb_animation_uses_hue(controller_state.rgbAnimationSelection) ? 3 : 2;
                    controller_state.menuSelection = controller_state.menuSelection == 0 ? field_count - 1 : controller_state.menuSelection - 1;
                } else if (controller_state.menuState == MENU_SIDE_LEDS) {
                    menu_move(-1);
                } else if (controller_state.menuState == MENU_RGB_STATIC) {
#ifdef RGB_MATRIX_ENABLE
                    rgb_matrix_increase_val();
#endif
                } else if (controller_state.menuState == MENU_CALIB_FILTER) {
                    controller_state.filterCandidate = MIN(100, controller_state.filterCandidate + 5);
                } else {
                    menu_move(-1);
                }
                return false;
            case KC_DOWN:
                if (controller_state.menuState == MENU_RGB_ANIMATION_SETTINGS) {
                    uint8_t field_count = replicazeron_rgb_animation_uses_hue(controller_state.rgbAnimationSelection) ? 3 : 2;
                    controller_state.menuSelection = (controller_state.menuSelection + 1) % field_count;
                } else if (controller_state.menuState == MENU_SIDE_LEDS) {
                    menu_move(1);
                } else if (controller_state.menuState == MENU_RGB_STATIC) {
#ifdef RGB_MATRIX_ENABLE
                    rgb_matrix_decrease_val();
#endif
                } else if (controller_state.menuState == MENU_CALIB_FILTER) {
                    controller_state.filterCandidate = controller_state.filterCandidate >= 5 ? controller_state.filterCandidate - 5 : 0;
                } else {
                    menu_move(1);
                }
                return false;
            case KC_LEFT:
                if (controller_state.menuState == MENU_LAYOUT_BROWSER) {
                    controller_state.menuState = MENU_MAIN;
                    controller_state.menuSelection = 0;
                } else if (controller_state.menuState == MENU_RGB_TYPE) {
                    controller_state.menuState = MENU_MAIN;
                    controller_state.menuSelection = 1;
                } else if (controller_state.menuState == MENU_RGB_CONTROL) {
                    controller_state.menuState = MENU_RGB_TYPE;
                    controller_state.menuSelection = 0;
                } else if (controller_state.menuState == MENU_RGB_OPENRGB_INFO) {
                    controller_state.menuState = MENU_RGB_TYPE;
                    controller_state.menuSelection = 0;
                } else if (controller_state.menuState == MENU_RGB_ANIMATION_BROWSER) {
#ifdef RGB_MATRIX_ENABLE
                    apply_rgb_animation(controller_state.rgbAnimationSelection);
#endif
                    controller_state.menuState = MENU_RGB_TYPE;
                    controller_state.menuSelection = 1;
                } else if (controller_state.menuState == MENU_RGB_STATIC) {
                    controller_state.menuState = MENU_RGB_TYPE;
                    controller_state.menuSelection = 2;
                } else if (controller_state.menuState == MENU_RGB_ANIMATION_SETTINGS) {
#ifdef RGB_MATRIX_ENABLE
                    if (controller_state.menuSelection == 0) {
                        change_rgb_animation_value(false);
                    } else if (controller_state.menuSelection == 1) {
                        change_rgb_animation_speed(false);
                    } else {
                        change_rgb_animation_hue(false);
                    }
#endif
                } else if (controller_state.menuState == MENU_SIDE_LEDS) {
                    if (controller_state.menuSelection == 0) {
                        set_side_leds_enabled(!controller_state.sideLedsEnabled);
                    } else if (controller_state.menuSelection == 1) {
                        change_side_led_brightness(false);
                    } else if (controller_state.menuSelection == 2) {
                        set_side_led_polarity(!controller_state.sideLedsActiveLow);
                    } else if (controller_state.menuSelection == 3) {
                        change_side_led_source(true, false);
                    } else {
                        change_side_led_source(false, false);
                    }
                } else if (controller_state.menuState == MENU_MODE_LAYOUT) {
                    controller_state.menuState = MENU_MAIN;
                    controller_state.menuSelection = 3;
                } else if (controller_state.menuState == MENU_MODE) {
                    controller_state.menuState = MENU_MODE_LAYOUT;
                    controller_state.menuSelection = controller_state.layoutSelection;
                } else if (controller_state.menuState == MENU_CALIB) {
                    controller_state.menuState = MENU_MAIN;
                    controller_state.menuSelection = 4;
                } else if (controller_state.menuState == MENU_CALIB_FILTER) {
                    controller_state.filterCandidate = controller_state.filterStrength;
                    controller_state.menuState = MENU_CALIB;
                    controller_state.menuSelection = 1;
                } else {
                    menu_close();
                }
                return false;
            case KC_RIGHT:
                if (controller_state.menuState == MENU_RGB_STATIC) {
#ifdef RGB_MATRIX_ENABLE
                    rgb_matrix_increase_hue();
#endif
                } else if (controller_state.menuState == MENU_RGB_ANIMATION_SETTINGS) {
#ifdef RGB_MATRIX_ENABLE
                    if (controller_state.menuSelection == 0) {
                        change_rgb_animation_value(true);
                    } else if (controller_state.menuSelection == 1) {
                        change_rgb_animation_speed(true);
                    } else {
                        change_rgb_animation_hue(true);
                    }
#endif
                } else if (controller_state.menuState == MENU_SIDE_LEDS) {
                    if (controller_state.menuSelection == 0) {
                        set_side_leds_enabled(!controller_state.sideLedsEnabled);
                    } else if (controller_state.menuSelection == 1) {
                        change_side_led_brightness(true);
                    } else if (controller_state.menuSelection == 2) {
                        set_side_led_polarity(!controller_state.sideLedsActiveLow);
                    } else if (controller_state.menuSelection == 3) {
                        change_side_led_source(true, true);
                    } else {
                        change_side_led_source(false, true);
                    }
                }
                return false;
            case KC_ENT:
                if (controller_state.menuState == MENU_RGB_STATIC) {
#ifdef RGB_MATRIX_ENABLE
                    rgb_matrix_toggle();
#endif
                } else if (controller_state.menuState == MENU_RGB_ANIMATION_SETTINGS) {
                    controller_state.menuState = MENU_RGB_ANIMATION_BROWSER;
                    controller_state.menuSelection = controller_state.rgbAnimationSelection;
                } else if (controller_state.menuState == MENU_MODE) {
                    cycle_layout_mode(controller_state.layoutSelection);
                } else if (controller_state.menuState == MENU_CALIB_FILTER) {
                    set_filter_strength(controller_state.filterCandidate);
                    controller_state.menuState = MENU_CALIB;
                    controller_state.menuSelection = 1;
                } else {
                    menu_select();
                }
                return false;
            default:
                return false;
        }
    }

    if (keycode == JOYMODE && record->event.pressed) {
        cycle_layout_mode(controller_state.activeLayout);
    } else if (keycode == AUTORUN && record->event.pressed) {
      if (!controller_state.autoRun) {
        controller_state.autoRun = true;
        register_code(KC_W);
      } else {
        controller_state.autoRun = false;
        unregister_code(KC_W);
      }
    }
    return true;
};

layer_state_t layer_state_set_kb(layer_state_t state) {
    state = layer_state_set_user(state);
    controller_state.highestActiveLayer = get_highest_layer(state) ;
    if (controller_state.highestActiveLayer < LAYOUT_COUNT) {
        controller_state.activeLayout = controller_state.highestActiveLayer;
        controller_state.layoutSelection = controller_state.activeLayout;
        apply_layout_mode(controller_state.activeLayout);
    }

    return state;
}
