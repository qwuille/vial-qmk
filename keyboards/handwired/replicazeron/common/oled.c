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
#define IDLE_LOGO_MIN_DELAY_MS     120000UL
#define IDLE_LOGO_DELAY_RANGE_MS   180001UL

static bool     logo_timing_initialized;
static bool     boot_logo_active = true;
static bool     idle_logo_active;
static uint32_t logo_started;
static uint32_t last_activity_seen;
static uint32_t next_idle_logo;
static uint32_t logo_random_state = 0x5245504CUL;

static const char PROGMEM main_menu_labels[MAIN_MENU_ITEM_COUNT][8] = {
    "LAYOUT",
    "RGB",
    "SIDELED",
    "MODE",
    "CALIB",
    "RESET"
};

static uint32_t next_logo_delay(void) {
    /* A tiny PRNG is sufficient here; this only varies an animation interval. */
    logo_random_state ^= logo_random_state << 13;
    logo_random_state ^= logo_random_state >> 17;
    logo_random_state ^= logo_random_state << 5;
    logo_random_state ^= timer_read32();
    return IDLE_LOGO_MIN_DELAY_MS + (logo_random_state % IDLE_LOGO_DELAY_RANGE_MS);
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

static bool draw_logo_or_idle_blank(menu_state_t menu_state) {
    uint32_t now      = timer_read32();
    uint32_t activity = last_input_activity_time();

    if (!logo_timing_initialized) {
        logo_timing_initialized = true;
        logo_started            = now;
        last_activity_seen      = activity;
        next_idle_logo          = activity + OLED_TIMEOUT + next_logo_delay();
    }

    /* Menus and fresh input always take control of the display immediately. */
    if (menu_state != MENU_NONE || activity != last_activity_seen) {
        last_activity_seen = activity;
        boot_logo_active   = false;
        idle_logo_active   = false;
        next_idle_logo     = activity + OLED_TIMEOUT + next_logo_delay();
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

    if (last_input_activity_elapsed() < OLED_TIMEOUT) {
        return false;
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
            next_idle_logo   = now + next_logo_delay();
            oled_clear();
            oled_off();
        }
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

static const char *side_led_source_name(uint8_t source) {
    switch (source) {
        case SIDE_LED_SOURCE_STICK: return PSTR("Stick");
        case SIDE_LED_SOURCE_BUTTONS: return PSTR("Buttons");
        case SIDE_LED_SOURCE_ACTIVITY: return PSTR("Activity");
        case SIDE_LED_SOURCE_ALWAYS_ON: return PSTR("Always");
        case SIDE_LED_SOURCE_CAPS_LOCK: return PSTR("Caps Lock");
        case SIDE_LED_SOURCE_NUM_LOCK: return PSTR("Num Lock");
        case SIDE_LED_SOURCE_SCROLL_LOCK: return PSTR("Scroll Lock");
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

static void draw_mode_layout_browser(controller_state_t controller_state) {
    oled_write_ln_P(PSTR("MODE: LAYOUT"), false);
    oled_write_P(PSTR("> "), false);
    draw_layout_name(controller_state.menuSelection, true);
    draw_mode_value(controller_state.layoutModes[controller_state.menuSelection]);
    oled_write_ln_P(PSTR("Up/Dn then Menu"), false);
}

static void draw_mode_menu(controller_state_t controller_state) {
    oled_write_ln_P(PSTR("MODE"), false);
    draw_layout_name(controller_state.layoutSelection, true);
    draw_mode_value(controller_state.layoutModes[controller_state.layoutSelection]);
    oled_write_ln_P(PSTR("Menu Cycle Left Back"), false);
}

//////////// OLED output helpers //////////////
void draw_mode(controller_state_t controller_state) {
    //draw oled row showing thumbstick mode
    oled_write_P(PSTR("Mode: "), false);
    if (controller_state.wasdShiftMode) {
        oled_write_ln_P(PSTR("WASD + Shift"), false);
    } else if (controller_state.wasdMode) {
        oled_write_ln_P(PSTR("WASD"), false);
    } else {
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

//////////// draw OLED output //////////////
void draw_oled(controller_state_t controller_state) {
    if (draw_logo_or_idle_blank(controller_state.menuState)) {
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
            case MENU_CALIB:
                draw_calibration_menu(controller_state.menuSelection);
                return;
            case MENU_CALIB_DEADZONE:
                draw_deadzone_calibration();
                return;
            case MENU_CALIB_FILTER:
                draw_filter_calibration(controller_state.filterCandidate);
                return;
            case MENU_FACTORY_RESET:
                draw_factory_reset_menu();
                return;
            default:
                break;
        }
    }

    oled_write_P(PSTR("Layout:"), false);

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
            draw_layout_name(controller_state.highestActiveLayer, false);
            break;

        case _SETTINGS:
            draw_layout_name(controller_state.highestActiveLayer, false);
            break;

        default:
            oled_write_P(PSTR("Unknown"), false);
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
