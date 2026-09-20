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

#define THUMBSTICK_DEBUG

/* Runtime-configurable idle handling is implemented in common/oled.c. */
#define OLED_TIMEOUT 0
/* Live input screens otherwise dirty the display on nearly every matrix loop,
 * starving the loop-timed side-LED PWM with continuous I2C transfers. */
#define OLED_UPDATE_INTERVAL 50

/* joystick configuration */
#define JOYSTICK_BUTTON_COUNT 0
#define JOYSTICK_AXIS_COUNT 2
#define JOYSTICK_AXIS_RESOLUTION 10

#define _DEADZONE  100  // 0 to _SHIFTZONE-1
#define _SHIFTZONE 350  // _DEADZONE+1 to 600
#define _FILTER_STRENGTH 20 // 0 disables axis filtering, 100 allows only the dominant axis
#define _THUMBSTICK_ROTATION 100 //degrees, adjusts forward direction

/* Locking resynchronize hack */
#define LOCKING_RESYNC_ENABLE
