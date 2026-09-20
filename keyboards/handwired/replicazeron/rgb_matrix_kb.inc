/* Copyright 2026 Qwuille
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

RGB_MATRIX_EFFECT(KNIGHT_RIDER)
RGB_MATRIX_EFFECT(CYLON)
RGB_MATRIX_EFFECT(PULSE)
RGB_MATRIX_EFFECT(REACTIVE_PULSE)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

uint8_t replicazeron_rgb_led_count(void);

#define REACTIVE_IDLE_LEVEL       7
#define REACTIVE_BURST_WINDOW_MS  600
#define REACTIVE_IDLE_TIMEOUT_MS  1000
#define REACTIVE_ACTIVITY_TICK_MS 12

static uint8_t reactive_key_activity;
static uint8_t reactive_joystick_activity;
static bool reactive_pulse_active;
static bool reactive_pulse_repeating;
static bool reactive_pulse_outward = true;
static bool reactive_joystick_initialized;
static int16_t reactive_last_joystick_x;
static int16_t reactive_last_joystick_y;
static uint16_t reactive_pulse_position;
static uint32_t reactive_last_press;
static uint32_t reactive_activity_updated;
static uint32_t reactive_animation_updated;
static uint32_t reactive_joystick_sampled;

static uint8_t replicazeron_effect_led_max(uint8_t led_max) {
    return MIN(led_max, replicazeron_rgb_led_count());
}

static void replicazeron_set_effect_color(uint8_t led, uint8_t level) {
    hsv_t hsv = rgb_matrix_config.hsv;
    hsv.v = scale8(hsv.v, level);
    rgb_t rgb = rgb_matrix_hsv_to_rgb(hsv);
    rgb_matrix_set_color(led, rgb.r, rgb.g, rgb.b);
}

static uint16_t replicazeron_scanner_position(bool *moving_right) {
    uint8_t led_count = replicazeron_rgb_led_count();
    uint16_t travel_ms = 1050 - ((uint16_t)rgb_matrix_config.speed * 3);
    uint32_t phase = g_rgb_timer % ((uint32_t)travel_ms * 2);
    *moving_right = phase < travel_ms;
    if (!*moving_right) {
        phase = ((uint32_t)travel_ms * 2) - phase;
    }
    return (phase * ((led_count - 1) * 256U)) / travel_ms;
}

bool KNIGHT_RIDER(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    uint8_t active_led_max = replicazeron_effect_led_max(led_max);
    bool moving_right;
    uint16_t position = replicazeron_scanner_position(&moving_right);

    for (uint8_t i = led_min; i < active_led_max; ++i) {
        int16_t signed_distance = moving_right ? (int16_t)position - ((int16_t)i * 256)
                                               : ((int16_t)i * 256) - (int16_t)position;
        uint8_t level = 0;
        if (signed_distance >= -128 && signed_distance < 1152) {
            uint16_t distance = signed_distance < 0 ? (uint16_t)-signed_distance : (uint16_t)signed_distance;
            level = distance >= 1152 ? 0 : 255 - ((uint32_t)distance * 255 / 1152);
        }
        replicazeron_set_effect_color(i, level);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

bool CYLON(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    uint8_t active_led_max = replicazeron_effect_led_max(led_max);
    uint8_t led_count = replicazeron_rgb_led_count();
    uint8_t time = scale16by8(g_rgb_timer, qadd8(rgb_matrix_config.speed / 4, 1));
    uint16_t position = ((uint32_t)sin8(time) * ((led_count - 1) * 256U)) / 255;

    for (uint8_t i = led_min; i < active_led_max; ++i) {
        int16_t delta = ((int16_t)i * 256) - (int16_t)position;
        uint16_t distance = delta < 0 ? (uint16_t)-delta : (uint16_t)delta;
        uint8_t level = distance >= 640 ? 0 : 255 - ((uint32_t)distance * 255 / 640);
        replicazeron_set_effect_color(i, level);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

bool PULSE(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    uint8_t active_led_max = replicazeron_effect_led_max(led_max);
    uint8_t time = scale16by8(g_rgb_timer, qadd8(rgb_matrix_config.speed / 4, 1));
    uint8_t envelope = abs8(sin8(time) - 128) * 2;
    envelope = scale8(envelope, envelope);

    for (uint8_t i = led_min; i < active_led_max; ++i) {
        replicazeron_set_effect_color(i, envelope);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

static void reactive_decay_activity(uint32_t now) {
    if (reactive_activity_updated == 0) {
        reactive_activity_updated = now;
        return;
    }

    uint32_t elapsed = now - reactive_activity_updated;
    uint8_t decay = MIN(elapsed / REACTIVE_ACTIVITY_TICK_MS, UINT8_MAX);
    if (decay == 0) {
        return;
    }
    reactive_key_activity = reactive_key_activity > decay ? reactive_key_activity - decay : 0;
    reactive_joystick_activity = reactive_joystick_activity > decay ? reactive_joystick_activity - decay : 0;
    reactive_activity_updated += (uint32_t)decay * REACTIVE_ACTIVITY_TICK_MS;
}

void replicazeron_rgb_reactive_trigger(void) {
    uint32_t now = timer_read32();
    reactive_decay_activity(now);
    reactive_key_activity = qadd8(reactive_key_activity, 64);

    if (!reactive_pulse_active) {
        reactive_pulse_active = true;
        reactive_pulse_repeating = false;
        reactive_pulse_outward = true;
        reactive_pulse_position = 0;
    } else if (now - reactive_last_press <= REACTIVE_BURST_WINDOW_MS) {
        /* Preserve the current position. A burst reverses the existing
         * outward wave, then it ping-pongs until the presses become idle. */
        if (!reactive_pulse_repeating && reactive_pulse_outward) {
            reactive_pulse_outward = false;
        }
        reactive_pulse_repeating = true;
    }
    reactive_last_press = now;
}

void replicazeron_rgb_set_joystick_activity(int16_t x, int16_t y) {
    uint32_t now = timer_read32();
    if (!reactive_joystick_initialized) {
        reactive_last_joystick_x = x;
        reactive_last_joystick_y = y;
        reactive_joystick_sampled = now;
        reactive_joystick_initialized = true;
        return;
    }
    if (now - reactive_joystick_sampled < 8) {
        return;
    }

    reactive_decay_activity(now);
    int16_t dx = x - reactive_last_joystick_x;
    int16_t dy = y - reactive_last_joystick_y;
    uint16_t movement = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
    if (movement > 8) {
        reactive_joystick_activity = qadd8(reactive_joystick_activity, MIN((movement - 8) / 2, 48));
    }
    reactive_last_joystick_x = x;
    reactive_last_joystick_y = y;
    reactive_joystick_sampled = now;
}

bool REACTIVE_PULSE(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    uint8_t active_led_max = replicazeron_effect_led_max(led_max);
    uint32_t now = timer_read32();
    if (params->init) {
        reactive_pulse_active = false;
        reactive_pulse_repeating = false;
        reactive_pulse_outward = true;
        reactive_pulse_position = 0;
        reactive_animation_updated = now;
    }
    reactive_decay_activity(now);

    if (reactive_pulse_repeating && now - reactive_last_press > REACTIVE_IDLE_TIMEOUT_MS) {
        reactive_pulse_repeating = false;
    }

    const uint16_t center = (replicazeron_rgb_led_count() - 1) * 128U;
    bool deactivate_after_frame = false;
    if (reactive_pulse_active) {
        uint32_t elapsed = MIN(now - reactive_animation_updated, 50);
        uint16_t base_duration = 1100 - ((uint16_t)rgb_matrix_config.speed * 3);
        uint16_t average_activity = ((uint16_t)reactive_key_activity + reactive_joystick_activity) / 2;
        uint16_t speed_scale = 256 + (average_activity * 2);
        uint16_t movement = ((uint32_t)elapsed * center * speed_scale) / ((uint32_t)base_duration * 256);
        movement = MAX(movement, 1);

        if (reactive_pulse_outward) {
            if (reactive_pulse_position + movement >= center) {
                reactive_pulse_position = center;
                if (reactive_pulse_repeating) {
                    reactive_pulse_outward = false;
                } else {
                    deactivate_after_frame = true;
                }
            } else {
                reactive_pulse_position += movement;
            }
        } else if (movement >= reactive_pulse_position) {
            reactive_pulse_position = 0;
            if (reactive_pulse_repeating) {
                reactive_pulse_outward = true;
            } else {
                deactivate_after_frame = true;
            }
        } else {
            reactive_pulse_position -= movement;
        }
    }
    reactive_animation_updated = now;

    uint8_t response_level = MAX(64, reactive_key_activity);
    for (uint8_t i = led_min; i < active_led_max; ++i) {
        uint8_t level = REACTIVE_IDLE_LEVEL;
        if (!reactive_pulse_active) {
            replicazeron_set_effect_color(i, level);
            continue;
        }
        int16_t from_center = ((int16_t)i * 256) - (int16_t)center;
        uint16_t distance = from_center < 0 ? (uint16_t)-from_center : (uint16_t)from_center;
        uint16_t difference = distance > reactive_pulse_position ? distance - reactive_pulse_position : reactive_pulse_position - distance;
        uint8_t band = difference >= 384 ? 0 : 255 - ((uint32_t)difference * 255 / 384);
        uint8_t pulse_level = scale8(response_level, band);
        level = MAX(level, pulse_level);
        replicazeron_set_effect_color(i, level);
    }
    if (deactivate_after_frame) {
        reactive_pulse_active = false;
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif
