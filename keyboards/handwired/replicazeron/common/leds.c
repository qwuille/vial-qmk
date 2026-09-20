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

#include "leds.h"
#include <stdbool.h>
#include "gpio.h"

#define STATUS_LED_PWM_STEPS 8

//////////// Status LEDs //////////////
static void write_led(pin_t pin, bool on, bool active_low) {
    gpio_write_pin(pin, active_low ? !on : on);
}

void init_leds(bool active_low) {
    gpio_set_pin_output(STATUS_LED_A_PIN);
    gpio_set_pin_output(STATUS_LED_B_PIN);
    write_led(STATUS_LED_A_PIN, false, active_low);
    write_led(STATUS_LED_B_PIN, false, active_low);
}

void update_leds(uint8_t level_a, uint8_t level_b, bool enabled, bool active_low, uint8_t brightness) {
    static uint8_t pwm_phase;

    if (!enabled || brightness == 0) {
        write_led(STATUS_LED_A_PIN, false, active_low);
        write_led(STATUS_LED_B_PIN, false, active_low);
        return;
    }

    level_a = ((uint16_t)level_a * brightness) / UINT8_MAX;
    level_b = ((uint16_t)level_b * brightness) / UINT8_MAX;
    /* Advance independently on each keyboard task. Using the millisecond
     * clock here can alias with the main loop and repeatedly hit one phase,
     * making every brightness setting look identical. */
    uint8_t phase = pwm_phase;
    pwm_phase = (pwm_phase + 1) & (STATUS_LED_PWM_STEPS - 1);
    uint8_t pwm_level_a = (level_a + 31) / 32;
    uint8_t pwm_level_b = (level_b + 31) / 32;
    bool led_a_on = pwm_level_a >= STATUS_LED_PWM_STEPS || phase < pwm_level_a;
    bool led_b_on = pwm_level_b >= STATUS_LED_PWM_STEPS || phase < pwm_level_b;
    write_led(STATUS_LED_A_PIN, led_a_on, active_low);
    write_led(STATUS_LED_B_PIN, led_b_on, active_low);
}

void suspend_leds(bool active_low) {
    write_led(STATUS_LED_A_PIN, false, active_low);
    write_led(STATUS_LED_B_PIN, false, active_low);
}
