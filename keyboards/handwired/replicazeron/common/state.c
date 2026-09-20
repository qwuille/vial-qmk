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

#include "state.h"

controller_state_t init_state(void) {
    controller_state_t controller_state = {
        .wasdMode = false,
        .wasdShiftMode = false,
        .autoRun = false,
        .highestActiveLayer = 0,
        .menuState = MENU_NONE,
        .menuSelection = 0,
        .activeLayout = LAYOUT_1,
        .layoutSelection = LAYOUT_1,
        .layoutModes = {JOYSTICK_MODE_ANALOG},
        .rgbAnimationSelection = RGB_ANIMATION_BREATHING,
        .rgbStaticSelected = false,
        .openrgbEnabled = false,
        .sideLedsEnabled = true,
        .sideLedsActiveLow = true,
        .sideLedBrightness = UINT8_MAX,
        .sideLedSourceA = SIDE_LED_SOURCE_STICK,
        .sideLedSourceB = SIDE_LED_SOURCE_BUTTONS,
        .rgbLedCount = REPLICAZERON_RGB_LED_COUNT_DEFAULT,
        .deadzone = _DEADZONE,
        .filterStrength = _FILTER_STRENGTH,
        .filterCandidate = _FILTER_STRENGTH,
    };

    return controller_state;
}
