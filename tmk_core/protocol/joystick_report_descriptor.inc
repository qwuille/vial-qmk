    HID_RI_USAGE_PAGE(8, 0x01),          // Generic Desktop
    HID_RI_USAGE(8, JOYSTICK_HID_USAGE), // Joystick or Game Pad
    HID_RI_COLLECTION(8, 0x01),          // Application
#ifdef JOYSTICK_SHARED_EP
        HID_RI_REPORT_ID(8, REPORT_ID_JOYSTICK),
#endif
        HID_RI_COLLECTION(8, 0x00), // Physical
#if JOYSTICK_AXIS_COUNT > 0
            HID_RI_USAGE_PAGE(8, 0x01), // Generic Desktop
            HID_RI_USAGE(8, 0x30),      // X
#    if JOYSTICK_AXIS_COUNT > 1
            HID_RI_USAGE(8, 0x31),      // Y
#    endif
#    if JOYSTICK_AXIS_COUNT > 2
            HID_RI_USAGE(8, 0x32),      // Z
#    endif
#    if JOYSTICK_AXIS_COUNT > 3
            HID_RI_USAGE(8, 0x33),      // Rx
#    endif
#    if JOYSTICK_AXIS_COUNT > 4
            HID_RI_USAGE(8, 0x34),      // Ry
#    endif
#    if JOYSTICK_AXIS_COUNT > 5
            HID_RI_USAGE(8, 0x35),      // Rz
#    endif
#    if JOYSTICK_AXIS_RESOLUTION == 8
            HID_RI_LOGICAL_MINIMUM(8, -JOYSTICK_MAX_VALUE),
            HID_RI_LOGICAL_MAXIMUM(8, JOYSTICK_MAX_VALUE),
            HID_RI_REPORT_COUNT(8, JOYSTICK_AXIS_COUNT),
            HID_RI_REPORT_SIZE(8, 0x08),
#    else
            HID_RI_LOGICAL_MINIMUM(16, -JOYSTICK_MAX_VALUE),
            HID_RI_LOGICAL_MAXIMUM(16, JOYSTICK_MAX_VALUE),
            HID_RI_REPORT_COUNT(8, JOYSTICK_AXIS_COUNT),
            HID_RI_REPORT_SIZE(8, 0x10),
#    endif
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
#endif

#ifdef JOYSTICK_HAS_HAT
            // Hat Switch (4 bits)
            HID_RI_USAGE(8, 0x39), // Hat Switch
            HID_RI_LOGICAL_MINIMUM(8, 0x00),
            HID_RI_LOGICAL_MAXIMUM(8, 0x07),
            HID_RI_PHYSICAL_MINIMUM(8, 0),
            HID_RI_PHYSICAL_MAXIMUM(16, 315),
            HID_RI_UNIT(8, 0x14),  // Degree, English Rotation
            HID_RI_REPORT_COUNT(8, 1),
            HID_RI_REPORT_SIZE(8, 4),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE | HID_IOF_NULLSTATE),
            // Padding (4 bits)
            HID_RI_REPORT_COUNT(8, 0x04),
            HID_RI_REPORT_SIZE(8, 0x01),
            HID_RI_INPUT(8, HID_IOF_CONSTANT),
#endif

#if JOYSTICK_BUTTON_COUNT > 0
            HID_RI_USAGE_PAGE(8, 0x09), // Button
            HID_RI_USAGE_MINIMUM(8, 0x01),
            HID_RI_USAGE_MAXIMUM(8, JOYSTICK_BUTTON_COUNT),
            HID_RI_LOGICAL_MINIMUM(8, 0x00),
            HID_RI_LOGICAL_MAXIMUM(8, 0x01),
            HID_RI_REPORT_COUNT(8, JOYSTICK_BUTTON_COUNT),
            HID_RI_REPORT_SIZE(8, 0x01),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

#    if (JOYSTICK_BUTTON_COUNT % 8) != 0
            HID_RI_REPORT_COUNT(8, 8 - (JOYSTICK_BUTTON_COUNT % 8)),
            HID_RI_REPORT_SIZE(8, 0x01),
            HID_RI_INPUT(8, HID_IOF_CONSTANT),
#    endif
#endif
        HID_RI_END_COLLECTION(0),
    HID_RI_END_COLLECTION(0),
