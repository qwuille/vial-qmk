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

#include "oled.h"
#include "oled_driver.h"
#include "progmem.h"
#include "timer.h"
#include "util.h"

#include <string.h>

uint8_t shiftbits =32 ;

#define LOGO_FRAME_MS              160
#define LOGO_FRAME_COUNT           12
#define LOGO_DURATION_MS           (LOGO_FRAME_MS * LOGO_FRAME_COUNT)

static bool     logo_timing_initialized;
static bool     boot_logo_active = true;
static bool     idle_logo_active;
static uint32_t logo_started;
static uint32_t last_activity_seen;
static uint32_t next_idle_logo;

static const char PROGMEM main_menu_labels[MAIN_MENU_ITEM_COUNT][8] = {
    "LAYOUT",
    "RGB",
    "SIDELED",
    "MODE",
    "SCREEN",
    "CALIB",
    "DISPLAY",
    "RESET"
};

static uint32_t oled_off_delay(uint8_t config) {
    return (uint32_t)(REPLICAZERON_OLED_OFF_INDEX(config) + 1) * 30000UL;
}

static uint32_t logo_interval(uint8_t config) {
    return (uint32_t)(REPLICAZERON_LOGO_INTERVAL_INDEX(config) + 1) * 60000UL;
}

static void logo_put_centered(char line[21], const char *text) {
    uint8_t length = strlen(text);
    uint8_t start  = length < 20 ? (20 - length) / 2 : 0;

    for (uint8_t index = 0; index < length && start + index < 20; ++index) {
        line[start + index] = text[index];
    }
}

static void draw_logo_frame(uint8_t frame) {
    char rows[4][21];

    for (uint8_t row = 0; row < 4; ++row) {
        memset(rows[row], ' ', 20);
        rows[row][20] = '\0';
    }

    if (frame == 0) {
        logo_put_centered(rows[0], ".");
    } else if (frame == 1) {
        logo_put_centered(rows[0], "|");
        logo_put_centered(rows[2], ".");
    } else {
        if (frame <= 8) {
            logo_put_centered(rows[1], "replicazeron");
        } else if (frame == 9) {
            logo_put_centered(rows[1], "r p i a e o ");
        } else if (frame == 10) {
            logo_put_centered(rows[1], "  p   a   o ");
        }

        switch (frame) {
            case 2:
                logo_put_centered(rows[2], "o");
                break;
            case 3:
                logo_put_centered(rows[2], "(o)");
                break;
            case 4:
                logo_put_centered(rows[2], "((o))");
                break;
            case 5:
                logo_put_centered(rows[2], "((   ))");
                break;
            case 6:
                logo_put_centered(rows[2], "(       )");
                break;
            case 7:
                logo_put_centered(rows[2], ".         .");
                break;
            default:
                break;
        }
    }

    oled_set_cursor(0, 0);
    for (uint8_t row = 0; row < 4; ++row) {
        oled_write_ln(rows[row], false);
    }
}

static bool draw_logo_or_idle_blank(controller_state_t controller_state) {
    uint32_t now      = timer_read32();
    uint32_t activity = last_input_activity_time();
    uint8_t config    = controller_state.displayTimerConfig;
    uint8_t off_index = REPLICAZERON_OLED_OFF_INDEX(config);
    uint8_t logo_index = REPLICAZERON_LOGO_INTERVAL_INDEX(config);
    uint32_t off_delay = oled_off_delay(config);

    if (!logo_timing_initialized) {
        logo_timing_initialized = true;
        logo_started            = now;
        last_activity_seen      = activity;
        next_idle_logo          = activity + off_delay + logo_interval(config);
    }

    /* Menus and fresh input always take control of the display immediately. */
    if (controller_state.menuState != MENU_NONE || activity != last_activity_seen) {
        last_activity_seen = activity;
        boot_logo_active   = false;
        idle_logo_active   = false;
        next_idle_logo     = activity + off_delay + logo_interval(config);
        oled_on();
        return false;
    }

    if (boot_logo_active) {
        uint32_t elapsed = timer_elapsed32(logo_started);
        if (elapsed < LOGO_DURATION_MS) {
            draw_logo_frame(elapsed / LOGO_FRAME_MS);
            return true;
        }
        boot_logo_active = false;
    }

    if (off_index == REPLICAZERON_DISPLAY_TIMER_DISABLED || last_input_activity_elapsed() < off_delay) {
        return false;
    }

    if (logo_index == REPLICAZERON_DISPLAY_TIMER_DISABLED) {
        oled_off();
        return true;
    }

    if (!idle_logo_active && timer_expired32(now, next_idle_logo)) {
        idle_logo_active = true;
        logo_started     = now;
        oled_on();
    }

    if (idle_logo_active) {
        uint32_t elapsed = timer_elapsed32(logo_started);
        if (elapsed < LOGO_DURATION_MS) {
            draw_logo_frame(elapsed / LOGO_FRAME_MS);
        } else {
            idle_logo_active = false;
            next_idle_logo   = now + logo_interval(config);
            oled_clear();
            oled_off();
        }
    } else {
        oled_off();
    }

    /* Leave the normal status screen hidden throughout the idle period. */
    return true;
}

static const char *get_layout_name(uint8_t layout) {
    switch (layout) {
        case LAYOUT_1:
            return "Casual";
        case LAYOUT_2:
            return "Shooter";
        case LAYOUT_3:
            return "Misc";
        case LAYOUT_4:
            return "Empty 3";
        case LAYOUT_5:
            return "Empty 4";
        case LAYOUT_6:
            return "Empty 5";
        case LAYOUT_7:
            return "Empty 6";
        case LAYOUT_8:
            return "Empty 7";
        case LAYOUT_9:
            return "Empty 8";
        case LAYOUT_10:
            return "Empty 9";
        default:
            return "UNKNOWN";
    }
}

static const char *get_rgb_animation_name(rgb_animation_id_t animation) {
    switch (animation) {
        case RGB_ANIMATION_RAINBOW_MOOD:
            return "Rainbow";
        case RGB_ANIMATION_RAINBOW_SWIRL:
            return "Swirl";
        case RGB_ANIMATION_KNIGHT:
            return "Knight Rider";
        case RGB_ANIMATION_TWINKLE:
            return "Twinkle";
        case RGB_ANIMATION_MOVING_RAINBOW:
            return "Moving Rainbow";
        case RGB_ANIMATION_HUE_WAVE:
            return "Hue Wave";
        case RGB_ANIMATION_HUE_PENDULUM:
            return "Hue Pendulum";
        case RGB_ANIMATION_CYLON:
            return "Cylon";
        case RGB_ANIMATION_PULSE:
            return "Pulse";
        case RGB_ANIMATION_REACTIVE_PULSE:
            return "Reactive Pulse";
        case RGB_ANIMATION_BREATHING:
        default:
            return "Breathing";
    }
}

static void draw_rgb_type_menu(uint8_t selection) {
    oled_write_ln_P(PSTR("RGB"), false);
    oled_write_P(selection == 0 ? PSTR(">") : PSTR(" "), false);
    oled_write_ln_P(PSTR("Controller"), false);
    oled_write_P(selection == 1 ? PSTR(">") : PSTR(" "), false);
    oled_write_ln_P(PSTR("Animated"), false);
    oled_write_P(selection == 2 ? PSTR(">") : PSTR(" "), false);
    oled_write_ln_P(PSTR("Static"), false);
}

static void draw_rgb_control_menu(uint8_t selection) {
    oled_write_ln_P(PSTR("RGB CONTROL"), false);
    oled_write_P(selection == 0 ? PSTR(">") : PSTR(" "), false);
    oled_write_ln_P(PSTR("Firmware"), false);
    oled_write_P(selection == 1 ? PSTR(">") : PSTR(" "), false);
    oled_write_ln_P(PSTR("OpenRGB"), false);
    oled_write_ln_P(PSTR("Menu: Select"), false);
}

static void draw_openrgb_info(void) {
    oled_write_ln_P(PSTR("OPENRGB ACTIVE"), false);
    oled_write_ln_P(PSTR("VID 4142"), false);
    oled_write_ln_P(PSTR("PID 2305"), false);
    oled_write_ln_P(PSTR("Menu: Back"), false);
}

static void draw_rgb_animation_browser(uint8_t selection) {
    oled_write_ln_P(PSTR("ANIMATED"), false);
    oled_write_P(PSTR("> "), false);
    oled_write_ln(get_rgb_animation_name(selection), false);
    oled_write_ln_P(PSTR("Up/Dn: Browse"), false);
    oled_write_ln_P(PSTR("Menu: Select"), false);
}

static void draw_rgb_animation_settings(rgb_animation_id_t animation, uint8_t selection) {
    oled_write_ln(get_rgb_animation_name(animation), false);
#ifdef RGB_MATRIX_ENABLE
    oled_write_P(selection == 0 ? PSTR(">B: ") : PSTR(" B: "), false);
    oled_write_ln(get_u16_str(rgb_matrix_get_val(), ' '), false);
    oled_write_P(selection == 1 ? PSTR(">S: ") : PSTR(" S: "), false);
    oled_write_ln(get_u16_str(replicazeron_rgb_speed_level(), ' '), false);
    if (replicazeron_rgb_animation_uses_hue(animation)) {
        oled_write_P(selection == 2 ? PSTR(">H: ") : PSTR(" H: "), false);
        oled_write_ln(get_u16_str(rgb_matrix_get_hue(), ' '), false);
    } else {
        oled_write_ln_P(PSTR(" Hue: automatic"), false);
    }
#else
    oled_write_ln_P(PSTR("RGB unavailable"), false);
    oled_write_ln_P(PSTR(""), false);
    oled_write_ln_P(PSTR(""), false);
#endif
}

static void draw_layout_name(uint8_t layout, bool newline) {
#ifdef VIA_ENABLE
    if (layout < REPLICAZERON_TITLE_COUNT) {
        for (uint8_t index = 0; index < REPLICAZERON_TITLE_LENGTH; index++) {
            oled_write_char(replicazeron_titles[layout][index], false);
        }
    } else {
        oled_write(get_layout_name(layout), false);
    }
#else
    oled_write(get_layout_name(layout), false);
#endif
    if (newline) {
        oled_write_ln_P(PSTR(""), false);
    }
}

static void draw_main_menu(uint8_t selection) {
    uint8_t first = selection >= 4 ? selection - 3 : 0;
    for (uint8_t index = first; index < MIN(first + 4, MAIN_MENU_ITEM_COUNT); ++index) {
        oled_write_P(index == selection ? PSTR(">") : PSTR(" "), false);
        oled_write_P(main_menu_labels[index], false);
        oled_write_ln_P(PSTR(""), false);
    }
}

static void draw_calibration_menu(uint8_t selection) {
    oled_write_ln_P(PSTR("CALIBRATE"), false);
    oled_write_P(selection == 0 ? PSTR(">") : PSTR(" "), false);
    oled_write_ln_P(PSTR("Deadzone"), false);
    oled_write_P(selection == 1 ? PSTR(">") : PSTR(" "), false);
    oled_write_ln_P(PSTR("Axis Filter"), false);
    oled_write_ln_P(PSTR("Left: Back"), false);
}

static void draw_display_menu(controller_state_t controller_state) {
    uint8_t off_index  = REPLICAZERON_OLED_OFF_INDEX(controller_state.displayTimerConfig);
    uint8_t logo_index = REPLICAZERON_LOGO_INTERVAL_INDEX(controller_state.displayTimerConfig);
    oled_write_ln_P(PSTR("DISPLAY TIMERS"), false);
    oled_write_P(controller_state.menuSelection == 0 ? PSTR(">Off: ") : PSTR(" Off: "), false);
    if (off_index == REPLICAZERON_DISPLAY_TIMER_DISABLED) {
        oled_write_ln_P(PSTR("Never"), false);
    } else {
        oled_write(get_u16_str((off_index + 1) * 30, ' '), false);
        oled_write_ln_P(PSTR(" sec"), false);
    }
    oled_write_P(controller_state.menuSelection == 1 ? PSTR(">Logo: ") : PSTR(" Logo: "), false);
    if (logo_index == REPLICAZERON_DISPLAY_TIMER_DISABLED) {
        oled_write_ln_P(PSTR("Disabled"), false);
    } else {
        oled_write(get_u16_str(logo_index + 1, ' '), false);
        oled_write_ln_P(PSTR(" min"), false);
    }
    oled_write_ln_P(PSTR("L/R Change Menu Back"), false);
}

static const char *side_led_source_name(uint8_t source) {
    switch (source) {
        case SIDE_LED_SOURCE_STICK: return PSTR("Stick");
        case SIDE_LED_SOURCE_BUTTONS: return PSTR("Buttons");
        case SIDE_LED_SOURCE_ACTIVITY: return PSTR("Activity");
        case SIDE_LED_SOURCE_ALWAYS_ON: return PSTR("Always");
        case SIDE_LED_SOURCE_CAPS_LOCK: return PSTR("Caps Lock");
        case SIDE_LED_SOURCE_NUM_LOCK: return PSTR("Num Lock");
        case SIDE_LED_SOURCE_SCROLL_LOCK: return PSTR("Scroll Lock");
        case SIDE_LED_SOURCE_OPENRGB: return PSTR("OpenRGB");
        case SIDE_LED_SOURCE_CONFIGURATION: return PSTR("Vial/Web");
        case SIDE_LED_SOURCE_HOST_CONTROL: return PSTR("Host");
        default: return PSTR("Off");
    }
}

static void draw_side_led_menu(controller_state_t controller_state) {
    oled_write_ln_P(PSTR("SIDE LEDS"), false);
    uint8_t first_item = controller_state.menuSelection > 2 ? controller_state.menuSelection - 2 : 0;
    for (uint8_t item = first_item; item < MIN(first_item + 3, SIDE_LED_MENU_ITEM_COUNT); ++item) {
        oled_write_P(controller_state.menuSelection == item ? PSTR(">") : PSTR(" "), false);
        switch (item) {
            case 0:
                oled_write_P(PSTR("Enabled: "), false);
                oled_write_ln_P(controller_state.sideLedsEnabled ? PSTR("Yes") : PSTR("No"), false);
                break;
            case 1:
                oled_write_P(PSTR("Bright: "), false);
                oled_write_ln(get_u16_str(controller_state.sideLedBrightness, ' '), false);
                break;
            case 2:
                oled_write_P(PSTR("Wiring: "), false);
                oled_write_ln_P(controller_state.sideLedsActiveLow ? PSTR("Low") : PSTR("High"), false);
                break;
            case 3:
                oled_write_P(PSTR("Left LED: "), false);
                oled_write_ln_P(side_led_source_name(controller_state.sideLedSourceA), false);
                break;
            default:
                oled_write_P(PSTR("Right LED:"), false);
                oled_write_ln_P(side_led_source_name(controller_state.sideLedSourceB), false);
                break;
        }
    }
}

static void draw_factory_reset_menu(void) {
    oled_write_ln_P(PSTR("FACTORY RESET"), false);
    oled_write_ln_P(PSTR("Hold top outer keys"), false);
    oled_write_ln_P(PSTR("Little+Index 2 sec"), false);
    oled_write_ln_P(PSTR("Menu: Cancel"), false);
}

static void draw_horizontal_gauge(uint8_t cursor_pos) {
    /*
     * A 128px display has room for 21 six-pixel glyphs, but writing the 21st
     * glyph advances QMK's OLED cursor to the next row.  Calling write_ln()
     * after that skips a second row and makes these four-line screens wrap
     * over themselves.  Keep one glyph of headroom so the explicit newline
     * advances exactly once.
     */
    const uint8_t width = 20;
    for (uint8_t x = 0; x < width; ++x) {
        if (x == 0) {
            oled_write_P(PSTR("C"), false);
        } else if (x == width - 1) {
            oled_write_P(PSTR("|"), false);
        } else if (x == cursor_pos) {
            oled_write_P(PSTR("O"), false);
        } else {
            oled_write_P(PSTR("-"), false);
        }
    }
    oled_write_ln_P(PSTR(""), false);
}

static void draw_deadzone_calibration(void) {
    uint8_t cursor_pos = 1 + MIN(17, ((uint32_t)thumbstick_unfiltered_position.distance * 17) / 724);
    oled_write_P(PSTR("DEADZONE Now: "), false);
    oled_write_ln(get_u16_str(thumbstick_unfiltered_position.distance, ' '), false);
    draw_horizontal_gauge(cursor_pos);
    oled_write_ln_P(PSTR("C Center       Outer"), false);
    oled_write_ln_P(PSTR("Any key: Save"), false);
}

static void draw_filter_calibration(uint8_t strength) {
    uint8_t cursor_pos = 1 + ((uint16_t)strength * 17) / 100;
    oled_write_P(PSTR("AXIS FILTER: "), false);
    oled_write(get_u16_str(strength, ' '), false);
    oled_write_ln_P(PSTR("%"), false);
    draw_horizontal_gauge(cursor_pos);
    oled_write_ln_P(PSTR("Up/Dn: Strength"), false);
    oled_write_ln_P(PSTR("Menu Save Left Back"), false);
}

static void draw_layout_browser(controller_state_t controller_state) {
    oled_write_ln_P(PSTR("LAYOUT"), false);
    oled_write_P(PSTR("> "), false);
    draw_layout_name(controller_state.menuSelection, true);
    oled_write_P(PSTR("A: "), false);
    draw_layout_name(controller_state.activeLayout, true);
    oled_write_ln_P(PSTR("Up/Dn Browse"), false);
}

static void draw_mode_value(joystick_mode_t mode) {
    switch (mode) {
        case JOYSTICK_MODE_WASD_SHIFT:
            oled_write_ln_P(PSTR("WASD + Shift"), false);
            break;
        case JOYSTICK_MODE_WASD:
            oled_write_ln_P(PSTR("WASD"), false);
            break;
        case JOYSTICK_MODE_ANALOG:
        default:
            oled_write_ln_P(PSTR("JoyStick"), false);
            break;
    }
}

static void draw_settings_stick_value(uint8_t mode) {
    switch (mode) {
        case SETTINGS_STICK_MIDDLE_DRAG:
            oled_write_ln_P(PSTR("CAD middle pan"), false);
            break;
        case SETTINGS_STICK_SHIFT_MIDDLE_DRAG:
            oled_write_ln_P(PSTR("CAD Shift orbit"), false);
            break;
        case SETTINGS_STICK_RIGHT_DRAG:
            oled_write_ln_P(PSTR("CAD right orbit"), false);
            break;
        default:
            oled_write_ln_P(PSTR("Page scroll/cursor"), false);
            break;
    }
}

static const char *oled_layout_preset_name(uint8_t preset) {
    switch (preset) {
        case OLED_LAYOUT_MACRO: return PSTR("Macro");
        case OLED_LAYOUT_GAME: return PSTR("Game");
        case OLED_LAYOUT_COMBINED: return PSTR("Combined");
        case OLED_LAYOUT_MINIMAL: return PSTR("Minimal");
        default: return PSTR("Input monitor");
    }
}

static void draw_screen_layout_browser(controller_state_t controller_state) {
    oled_write_ln_P(PSTR("SCREEN: LAYOUT"), false);
    oled_write_P(PSTR("> "), false);
    draw_layout_name(controller_state.menuSelection, true);
    oled_write_ln_P(oled_layout_preset_name(controller_state.layoutDisplayPresets[controller_state.menuSelection]), false);
    oled_write_ln_P(PSTR("Up/Dn then Menu"), false);
}

static void draw_screen_menu(controller_state_t controller_state) {
    oled_write_ln_P(PSTR("OLED DESIGN"), false);
    draw_layout_name(controller_state.layoutSelection, true);
    oled_write_ln_P(oled_layout_preset_name(controller_state.layoutDisplayPresets[controller_state.layoutSelection]), false);
    oled_write_ln_P(PSTR("Menu Cycle Left Back"), false);
}

static void draw_mode_layout_browser(controller_state_t controller_state) {
    if (controller_state.menuSelection == LAYOUT_COUNT) {
        oled_write_ln_P(PSTR("SETTINGS TOOLS"), false);
        oled_write_ln_P(PSTR("> Settings layer"), false);
        draw_settings_stick_value(controller_state.settingsStickMode);
    } else {
        oled_write_ln_P(PSTR("MODE: LAYOUT"), false);
        oled_write_P(PSTR("> "), false);
        draw_layout_name(controller_state.menuSelection, true);
        draw_mode_value(controller_state.layoutModes[controller_state.menuSelection]);
    }
    oled_write_ln_P(PSTR("Up/Dn then Menu"), false);
}

static void draw_mode_menu(controller_state_t controller_state) {
    if (controller_state.layoutSelection == LAYOUT_COUNT) {
        oled_write_ln_P(PSTR("SETTINGS TOOLS"), false);
        oled_write_ln_P(PSTR("Settings layer"), false);
        draw_settings_stick_value(controller_state.settingsStickMode);
    } else {
        oled_write_ln_P(PSTR("MODE"), false);
        draw_layout_name(controller_state.layoutSelection, true);
        draw_mode_value(controller_state.layoutModes[controller_state.layoutSelection]);
    }
    oled_write_ln_P(PSTR("Menu Cycle Left Back"), false);
}

//////////// OLED output helpers //////////////
void draw_mode(controller_state_t controller_state) {
    //draw oled row showing thumbstick mode
    if (controller_state.highestActiveLayer == _SETTINGS) {
        oled_write_P(PSTR("Tool: "), false);
        draw_settings_stick_value(controller_state.settingsStickMode);
    } else if (controller_state.wasdShiftMode) {
        oled_write_P(PSTR("Mode: "), false);
        oled_write_ln_P(PSTR("WASD + Shift"), false);
    } else if (controller_state.wasdMode) {
        oled_write_P(PSTR("Mode: "), false);
        oled_write_ln_P(PSTR("WASD"), false);
    } else {
        oled_write_P(PSTR("Mode: "), false);
        oled_write_ln_P(PSTR("JoyStick"), false);
    }
}

void draw_wasd_key(wasd_state_t wasd_state) {
    //draw oled row showing active keypresses emulated from thumbstick
    const char* keys = "wasd";
    bool keystates [] = { wasd_state.w, wasd_state.a, wasd_state.s, wasd_state.d };
    // iterate over keystates
    for (uint8_t i = 0 ; i < ARRAY_SIZE(keystates); ++i) {
        if (keystates[i]) {
            char k = keys[i] ;
            //bitshift char to upper case
            if (wasd_state.shift) {
                k &= ~shiftbits;
            }
            oled_write_char(k, false);
        } else {
            oled_write_P(PSTR(" "), false);
        }
    }
}

void draw_thumb_debug(thumbstick_polar_position_t thumbstick_polar_position) {
    //draw oled row showing thumbstick direction and distance from center
    oled_write_P(PSTR("Dir:"), false);
    oled_write(get_u16_str(thumbstick_polar_position.angle, ' '), false);
    oled_write_P(PSTR(" Dist:"), false);
    oled_write_ln(get_u16_str(thumbstick_polar_position.distance, ' '), false);
    //print registered key codes
    oled_write_P(PSTR("Keycodes: "), false);
    draw_wasd_key(wasd_state);
}

static const char *stick_direction(uint16_t angle, uint16_t distance, uint16_t deadzone) {
    if (distance < deadzone) return PSTR("--");
    if (angle < 23 || angle >= 338) return PSTR("N");
    if (angle < 68) return PSTR("NW");
    if (angle < 113) return PSTR("W");
    if (angle < 158) return PSTR("SW");
    if (angle < 203) return PSTR("S");
    if (angle < 248) return PSTR("SE");
    if (angle < 293) return PSTR("E");
    return PSTR("NE");
}

static uint8_t held_key_count(void) {
    uint8_t count = 0;
    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        matrix_row_t keys = matrix_get_row(row);
        while (keys) {
            count += keys & 1;
            keys >>= 1;
        }
    }
    return count;
}

static void draw_macro_name(uint8_t macro, bool prefix) {
    if (prefix) {
        oled_write_P(PSTR("Last: "), false);
    }
    if (macro == REPLICAZERON_LAST_MACRO_NONE) {
        oled_write_ln_P(PSTR("None"), false);
        return;
    }
#ifdef VIA_ENABLE
    char name[REPLICAZERON_TITLE_LENGTH];
    replicazeron_read_macro_name(macro, name);
    for (uint8_t index = 0; index < REPLICAZERON_TITLE_LENGTH; ++index) {
        oled_write_char(name[index], false);
    }
    oled_write_ln_P(PSTR(""), false);
#else
    oled_write_P(PSTR("Macro "), false);
    oled_write_ln(get_u16_str(macro, ' '), false);
#endif
}

static void draw_stick_line(controller_state_t controller_state, bool include_keys) {
    oled_write_P(PSTR("Stick: "), false);
    oled_write_P(stick_direction(thumbstick_polar_position.angle, thumbstick_polar_position.distance, controller_state.deadzone), false);
    oled_write_P(PSTR(" "), false);
    oled_write(get_u16_str(MIN(100, ((uint32_t)thumbstick_polar_position.distance * 100) / 724), ' '), false);
    oled_write_P(PSTR("%"), false);
    if (include_keys) {
        oled_write_P(PSTR(" K"), false);
        oled_write(get_u16_str(held_key_count(), ' '), false);
    }
    oled_write_ln_P(PSTR(""), false);
}

static void draw_macro_count(controller_state_t controller_state) {
    oled_write_P(PSTR("Macros: "), false);
    oled_write(get_u16_str(controller_state.macroSlotsUsed, ' '), false);
    oled_write_ln_P(PSTR("/16"), false);
}

static void draw_playable_layout(controller_state_t controller_state) {
    uint8_t layout = controller_state.highestActiveLayer;
    uint8_t preset = controller_state.layoutDisplayPresets[layout];
    oled_write_P(PSTR("Layout:"), false);
    draw_layout_name(layout, true);

    if (preset == OLED_LAYOUT_COMBINED) {
        draw_stick_line(controller_state, true);
        draw_macro_count(controller_state);
        draw_macro_name(controller_state.lastMacro, true);
        return;
    }

    draw_mode(controller_state);
    switch (preset) {
        case OLED_LAYOUT_MACRO:
            if (controller_state.lastMacro == REPLICAZERON_LAST_MACRO_NONE) {
                oled_write_ln_P(PSTR("Macro: None"), false);
                oled_write_ln_P(PSTR("No macro used"), false);
            } else {
                oled_write_P(PSTR("Macro: "), false);
                oled_write_ln(get_u16_str(controller_state.lastMacro, ' '), false);
                draw_macro_name(controller_state.lastMacro, false);
            }
            break;
        case OLED_LAYOUT_GAME:
            draw_macro_count(controller_state);
            draw_macro_name(controller_state.lastMacro, true);
            break;
        case OLED_LAYOUT_MINIMAL:
            oled_write_ln_P(PSTR(""), false);
            oled_write_ln_P(PSTR(""), false);
            break;
        case OLED_LAYOUT_INPUT:
        default:
            draw_stick_line(controller_state, false);
            oled_write_P(PSTR("Keys held: "), false);
            oled_write_ln(get_u16_str(held_key_count(), ' '), false);
            break;
    }
}

//////////// draw OLED output //////////////
void draw_oled(controller_state_t controller_state) {
    if (draw_logo_or_idle_blank(controller_state)) {
        return;
    }

    /* Every screen is a complete four-row frame. */
    oled_set_cursor(0, 0);

    if (controller_state.menuState != MENU_NONE) {
        switch (controller_state.menuState) {
            case MENU_MAIN:
                draw_main_menu(controller_state.menuSelection);
                return;
            case MENU_LAYOUT_BROWSER:
                draw_layout_browser(controller_state);
                return;
            case MENU_RGB_TYPE:
                draw_rgb_type_menu(controller_state.menuSelection);
                return;
            case MENU_RGB_CONTROL:
                draw_rgb_control_menu(controller_state.menuSelection);
                return;
            case MENU_RGB_OPENRGB_INFO:
                draw_openrgb_info();
                return;
            case MENU_RGB_STATIC:
                oled_write_ln_P(PSTR("STATIC"), false);
                oled_write_ln_P(PSTR("Up/Dn: Bright"), false);
                oled_write_ln_P(PSTR("Right: Hue"), false);
                oled_write_ln_P(PSTR("Menu: Toggle"), false);
                return;
            case MENU_RGB_ANIMATION_BROWSER:
                draw_rgb_animation_browser(controller_state.menuSelection);
                return;
            case MENU_RGB_ANIMATION_SETTINGS:
                draw_rgb_animation_settings(controller_state.rgbAnimationSelection, controller_state.menuSelection);
                return;
            case MENU_SIDE_LEDS:
                draw_side_led_menu(controller_state);
                return;
            case MENU_MODE_LAYOUT:
                draw_mode_layout_browser(controller_state);
                return;
            case MENU_MODE:
                draw_mode_menu(controller_state);
                return;
            case MENU_SCREEN_LAYOUT:
                draw_screen_layout_browser(controller_state);
                return;
            case MENU_SCREEN:
                draw_screen_menu(controller_state);
                return;
            case MENU_CALIB:
                draw_calibration_menu(controller_state.menuSelection);
                return;
            case MENU_CALIB_DEADZONE:
                draw_deadzone_calibration();
                return;
            case MENU_CALIB_FILTER:
                draw_filter_calibration(controller_state.filterCandidate);
                return;
            case MENU_DISPLAY:
                draw_display_menu(controller_state);
                return;
            case MENU_FACTORY_RESET:
                draw_factory_reset_menu();
                return;
            default:
                break;
        }
    }

    switch (controller_state.highestActiveLayer) {
        case _LAYOUT_1:
        case _LAYOUT_2:
        case _LAYOUT_3:
        case _LAYOUT_4:
        case _LAYOUT_5:
        case _LAYOUT_6:
        case _LAYOUT_7:
        case _LAYOUT_8:
        case _LAYOUT_9:
        case _LAYOUT_10:
            draw_playable_layout(controller_state);
            return;

        case _SETTINGS:
            oled_write_P(PSTR("Layout:"), false);
            draw_layout_name(controller_state.highestActiveLayer, false);
            break;

        default:
            oled_write_P(PSTR("Layout:Unknown"), false);
    }
    oled_write_ln_P(PSTR(""), false);

    draw_mode(controller_state);
    if (controller_state.highestActiveLayer == _SETTINGS ) {
        draw_thumb_debug(thumbstick_polar_position);
    }
    else {
        oled_write_ln_P(PSTR(" "), false);
    }
    oled_write_ln_P(PSTR(" "), false);
}
