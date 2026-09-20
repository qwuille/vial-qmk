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
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define LAYOUT_COUNT 10
#define MAIN_MENU_ITEM_COUNT 8
#define RGB_TYPE_COUNT 3
#define RGB_ANIMATION_COUNT 11
#define CALIBRATION_ITEM_COUNT 2
#define SIDE_LED_MENU_ITEM_COUNT 5
#define DISPLAY_MENU_ITEM_COUNT 2
#define REPLICAZERON_RGB_LED_COUNT_DEFAULT 11
#define REPLICAZERON_RGB_LED_COUNT_MAX 32
#define REPLICAZERON_OLED_OFF_DEFAULT 1
#define REPLICAZERON_LOGO_INTERVAL_DEFAULT 2
#define REPLICAZERON_DISPLAY_TIMER_DISABLED 15
#define REPLICAZERON_DISPLAY_TIMER_CONFIG(off, logo) (((logo) << 4) | (off))
#define REPLICAZERON_OLED_OFF_INDEX(config) ((config) & 0x0F)
#define REPLICAZERON_LOGO_INTERVAL_INDEX(config) ((config) >> 4)
#define OLED_LAYOUT_PRESET_COUNT 5
#define REPLICAZERON_LAST_MACRO_NONE UINT8_MAX

typedef enum {
    MENU_NONE = 0,
    MENU_MAIN,
    MENU_LAYOUT_BROWSER,
    MENU_RGB_TYPE,
    MENU_RGB_CONTROL,
    MENU_RGB_OPENRGB_INFO,
    MENU_RGB_STATIC,
    MENU_RGB_ANIMATION_BROWSER,
    MENU_RGB_ANIMATION_SETTINGS,
    MENU_SIDE_LEDS,
    MENU_MODE_LAYOUT,
    MENU_MODE,
    MENU_SCREEN_LAYOUT,
    MENU_SCREEN,
    MENU_CALIB,
    MENU_CALIB_DEADZONE,
    MENU_CALIB_FILTER,
    MENU_DISPLAY,
    MENU_FACTORY_RESET
} menu_state_t;

typedef enum {
    RGB_ANIMATION_BREATHING = 0,
    RGB_ANIMATION_RAINBOW_MOOD,
    RGB_ANIMATION_RAINBOW_SWIRL,
    RGB_ANIMATION_KNIGHT,
    RGB_ANIMATION_TWINKLE,
    RGB_ANIMATION_MOVING_RAINBOW,
    RGB_ANIMATION_HUE_WAVE,
    RGB_ANIMATION_HUE_PENDULUM,
    RGB_ANIMATION_CYLON,
    RGB_ANIMATION_PULSE,
    RGB_ANIMATION_REACTIVE_PULSE
} rgb_animation_id_t;

typedef enum {
    LAYOUT_1 = 0,
    LAYOUT_2,
    LAYOUT_3,
    LAYOUT_4,
    LAYOUT_5,
    LAYOUT_6,
    LAYOUT_7,
    LAYOUT_8,
    LAYOUT_9,
    LAYOUT_10
} layout_id_t;

typedef enum {
    JOYSTICK_MODE_ANALOG = 0,
    JOYSTICK_MODE_WASD,
    JOYSTICK_MODE_WASD_SHIFT,
    JOYSTICK_MODE_COUNT
} joystick_mode_t;

typedef enum {
    SETTINGS_STICK_MOUSE = 0,
    SETTINGS_STICK_MIDDLE_DRAG,
    SETTINGS_STICK_SHIFT_MIDDLE_DRAG,
    SETTINGS_STICK_RIGHT_DRAG,
    SETTINGS_STICK_MODE_COUNT
} settings_stick_mode_t;

typedef enum {
    OLED_LAYOUT_INPUT = 0,
    OLED_LAYOUT_MACRO,
    OLED_LAYOUT_GAME,
    OLED_LAYOUT_COMBINED,
    OLED_LAYOUT_MINIMAL
} oled_layout_preset_t;

typedef enum {
    SIDE_LED_SOURCE_OFF = 0,
    SIDE_LED_SOURCE_STICK,
    SIDE_LED_SOURCE_BUTTONS,
    SIDE_LED_SOURCE_ACTIVITY,
    SIDE_LED_SOURCE_ALWAYS_ON,
    SIDE_LED_SOURCE_CAPS_LOCK,
    SIDE_LED_SOURCE_NUM_LOCK,
    SIDE_LED_SOURCE_SCROLL_LOCK,
    SIDE_LED_SOURCE_COUNT
} side_led_source_t;

typedef struct {
    bool wasdMode;
    bool wasdShiftMode;
    bool autoRun;
    uint8_t highestActiveLayer;
    menu_state_t menuState;
    uint8_t menuSelection;
    layout_id_t activeLayout;
    layout_id_t layoutSelection;
    /* EEPROM stores exactly one byte per layout. Do not use the enum type
     * here: ARM enums are wider than one byte unless explicitly packed. */
    uint8_t layoutModes[LAYOUT_COUNT];
    uint8_t layoutDisplayPresets[LAYOUT_COUNT];
    uint8_t settingsStickMode;
    uint8_t displayTimerConfig;
    uint8_t lastMacro;
    uint8_t macroSlotsUsed;
    rgb_animation_id_t rgbAnimationSelection;
    bool rgbStaticSelected;
    bool openrgbEnabled;
    bool sideLedsEnabled;
    bool sideLedsActiveLow;
    uint8_t sideLedBrightness;
    uint8_t sideLedSourceA;
    uint8_t sideLedSourceB;
    uint8_t rgbLedCount;
    uint16_t deadzone;
    uint8_t filterStrength;
    uint8_t filterCandidate;
} controller_state_t;

controller_state_t init_state(void);
