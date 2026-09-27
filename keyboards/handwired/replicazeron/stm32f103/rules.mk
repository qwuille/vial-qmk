WS2812_DRIVER = pwm

# The normal STM32 release deliberately omits the USB gamepad interface.  A
# separately named compatibility release can opt in without removing any of
# the standard firmware features:
#   qmk compile ... -e REPLICAZERON_STM32_DIRECTINPUT=yes
ifeq ($(strip $(REPLICAZERON_STM32_DIRECTINPUT)),yes)
    JOYSTICK_ENABLE = yes
    OPT_DEFS += -DREPLICAZERON_STM32_DIRECTINPUT
    VIAL_KEYBOARD_DEFINITION = keyboards/handwired/replicazeron/keymaps/vial/vial.json
else
    JOYSTICK_ENABLE = no
    VIAL_KEYBOARD_DEFINITION = keyboards/handwired/replicazeron/keymaps/vial/vial-stm32.json
endif
