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

/* I2C Config */
#define I2C_DRIVER I2CD2
#define I2C1_SDA_PIN B11
#define I2C1_SCL_PIN B10

#define STATUS_LED_A_PIN B13
#define STATUS_LED_B_PIN B12

/*
 * PB14 is TIM1_CH2N. Drive the WS2812 chain from TIM1 update DMA instead
 * of timing-sensitive bit-banging. The two side LEDs use independent GPIO
 * software PWM so their settings cannot conflict with the strip timer.
 */
#define WS2812_PWM_DRIVER PWMD1
#define WS2812_PWM_CHANNEL 2
#define WS2812_PWM_DMA_STREAM STM32_DMA1_STREAM3
#define WS2812_PWM_DMA_CHANNEL 0
#define WS2812_PWM_DMA_REQUEST TIM_DIER_CC2DE
#define WS2812_PWM_COMPLEMENTARY_OUTPUT
#define RGB_MATRIX_LED_PROCESS_LIMIT 32
#define RGB_MATRIX_LED_FLUSH_LIMIT 8

#define ANALOG_AXIS_PIN_X B0
#define ANALOG_AXIS_PIN_Y B1
