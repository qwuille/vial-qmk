JOYSTICK_ENABLE = yes
OLED_ENABLE = yes

LEDS_ENABLE = yes
THUMBSTICK_ENABLE = yes

DYNAMIC_KEYMAP_LAYER_COUNT = 11

LTO_ENABLE = yes

SRC += state.c

VPATH += keyboards/handwired/replicazeron/common

# DEFAULT_FOLDER is no longer valid in modern QMK rules.mk. Use keyboard variant selection instead.
# DEFAULT_FOLDER = handwired/replicazeron/stm32f103
