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

#include "quantum.h"

#include "state.h"

extern controller_state_t controller_state;

#ifdef LEDS_ENABLE
#    include "leds.h"
#endif

#ifdef OLED_ENABLE
#    include "oled.h"
#endif

#ifdef THUMBSTICK_ENABLE
#    include "thumbstick.h"
#endif

enum kb_layers {
    _LAYOUT_1,
    _LAYOUT_2,
    _LAYOUT_3,
    _LAYOUT_4,
    _LAYOUT_5,
    _LAYOUT_6,
    _LAYOUT_7,
    _LAYOUT_8,
    _LAYOUT_9,
    _LAYOUT_10,
    _SETTINGS,
};

enum kb_keycodes {
    MENU_TOGGLE = QK_USER,
    JOYMODE,
    AUTORUN,
    MENU_SELECT,
    MENU_BACK,
    MENU_UP,
    MENU_DOWN,
    M_UP,
    M_DWN,
    M_L,
    M_R,
    M_SEL,
    SETTINGS_MOUSE_TOGGLE
};

#ifdef VIA_ENABLE
#    define REPLICAZERON_TITLE_COUNT 11
#    define REPLICAZERON_TITLE_LENGTH 13
extern char replicazeron_titles[REPLICAZERON_TITLE_COUNT][REPLICAZERON_TITLE_LENGTH];
#endif

#ifdef RGB_MATRIX_ENABLE
uint8_t replicazeron_rgb_led_count(void);
uint8_t replicazeron_rgb_speed_level(void);
bool replicazeron_rgb_animation_uses_hue(rgb_animation_id_t animation);
void replicazeron_rgb_reactive_trigger(void);
void replicazeron_rgb_set_joystick_activity(int16_t x, int16_t y);
#endif
