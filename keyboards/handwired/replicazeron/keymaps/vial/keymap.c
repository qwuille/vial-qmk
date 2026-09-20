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

#define DYNAMIC_KEYMAP_LAYER_COUNT 11

#include QMK_KEYBOARD_H
#include "config.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_LAYOUT_1] = LAYOUT(
    //  little  | ring       | middle | index   | 5way-dpad | -finger
        KC_F,    KC_ESC,      KC_1,    KC_3,    KC_RIGHT,
        KC_T,    KC_3,        KC_4,    KC_C,    KC_ENT,
        KC_X,    KC_Q,        KC_R,    KC_E,    KC_DOWN,
        KC_LSFT, KC_LCTL,     KC_SPC,  KC_F,    KC_LEFT,
        KC_B,    KC_V,        KC_T,    KC_N,    KC_UP,
        KC_TAB,  TO(_LAYOUT_2),       KC_ESC,  KC_I,    KC_P
    ),

    [_LAYOUT_2] = LAYOUT(
        KC_K,    KC_NO,       KC_NO,   KC_NO,   KC_RIGHT,
        KC_NO,   KC_NO,       KC_X,    KC_G,    KC_ENT,
        KC_NO,   KC_Q,        KC_NO,   KC_E,    KC_DOWN,
        KC_C,    KC_LSFT,     KC_SPC,  KC_F,    KC_LEFT,
        KC_B,    KC_X,        KC_Z,    KC_G,    KC_UP,
        KC_TAB,  TO(_LAYOUT_3),       KC_ESC,  KC_R,    KC_P
    ),

    [_LAYOUT_3] = LAYOUT(
        KC_Q,    LALT(KC_D),  KC_P,    KC_B,    KC_1,
        LCTL(KC_X), LCTL(KC_C), LCTL(KC_V), KC_C, KC_3,
        KC_NO,    KC_Q,        KC_F,    KC_E,    KC_7,
        KC_LCTL, KC_LSFT,     KC_SPC,  KC_LALT, KC_2,
        KC_NO,    KC_X,        KC_V,    KC_H,    KC_8,
        KC_TAB,   TO(_LAYOUT_4),       KC_ESC,  KC_R,    KC_P
    ),

    [_LAYOUT_4] = LAYOUT(
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, TO(_LAYOUT_5), KC_NO, KC_NO, KC_NO
    ),
    [_LAYOUT_5] = LAYOUT(
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, TO(_LAYOUT_6), KC_NO, KC_NO, KC_NO
    ),
    [_LAYOUT_6] = LAYOUT(
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, TO(_LAYOUT_7), KC_NO, KC_NO, KC_NO
    ),
    [_LAYOUT_7] = LAYOUT(
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, TO(_LAYOUT_8), KC_NO, KC_NO, KC_NO
    ),
    [_LAYOUT_8] = LAYOUT(
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, TO(_LAYOUT_9), KC_NO, KC_NO, KC_NO
    ),
    [_LAYOUT_9] = LAYOUT(
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, TO(_LAYOUT_10), KC_NO, KC_NO, KC_NO
    ),
    [_LAYOUT_10] = LAYOUT(
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, TO(_SETTINGS), KC_NO, KC_NO, KC_NO
    ),

    [_SETTINGS] = LAYOUT(
        RGB_M_P,  RGB_M_B,   RGB_M_K, RGB_M_T,   KC_RIGHT,
        KC_NO,    RGB_SAI,   RGB_VAI, RGB_HUI,   MENU_TOGGLE,
        RGB_TOG,  KC_NO,     KC_NO,   KC_NO,     KC_DOWN,
        KC_NO,    RGB_SAD,   RGB_VAD, RGB_HUD,   KC_LEFT,
        QK_BOOT,  JOYMODE,   AUTORUN, KC_V,       KC_UP,
        RGB_MOD,  TO(_LAYOUT_1), KC_NO, RGB_RMOD, KC_P
    )
};
