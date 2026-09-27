/* Copyright 2023 9R
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/* VIA enables Bootmagic globally; do not treat matrix (0,0) as Pico BOOTSEL. */
#define REPLICAZERON_DISABLE_BOOTMAGIC

/* GP20/GP21 use RP2040 I2C0. */
#define I2C_DRIVER I2CD0
#define I2C0_SDA_PIN GP20
#define I2C0_SCL_PIN GP21

#define STATUS_LED_A_PIN GP11
#define STATUS_LED_B_PIN GP12

#define ANALOG_AXIS_PIN_X GP26
#define ANALOG_AXIS_PIN_Y GP27
