ifeq ($(strip $(LEDS_ENABLE)), yes)
    OPT_DEFS += -DLEDS_ENABLE
    SRC += leds.c
endif

ifeq ($(strip $(OLED_ENABLE)), yes)
    SRC += oled.c
endif

ifeq ($(strip $(THUMBSTICK_ENABLE)), yes)
    OPT_DEFS += -DTHUMBSTICK_ENABLE
    SRC += thumbstick.c
    ANALOG_DRIVER_REQUIRED = yes
endif

# The RP2040 has ample flash for QMK conveniences that are intentionally
# removed from the flash-constrained STM32F103 build. This file is included
# after the Vial keymap rules, so these values override the shared conservative
# defaults without changing the Blue Pill firmware.
ifeq ($(strip $(KEYBOARD)),handwired/replicazeron/rp2040)
    OPT_DEFS += -DREPLICAZERON_XINPUT_ENABLE
    NKRO_ENABLE = yes
    REPEAT_KEY_ENABLE = yes
    CAPS_WORD_ENABLE = yes
    MAGIC_ENABLE = yes
    LAYER_LOCK_ENABLE = yes
    GRAVE_ESC_ENABLE = yes
    SPACE_CADET_ENABLE = yes
endif
