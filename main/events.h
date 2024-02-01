#pragma once

#include "bsp.h"

typedef enum _event_type {
    event_input_button,
    event_input_keyboard,
    event_control_keyboard,
} event_type_t;

typedef struct _event_input_keyboard_args {
    char character;
    int  action;
} event_input_keyboard_args_t;

typedef struct _event_control_keyboard_args {
    bool enable_typing;
    bool enable_actions[NUM_SWITCHES];
    bool enable_rotations[NUM_ROTATION];
    bool enable_characters[NUM_LETTER];
    bool enable_leds;
    bool enable_relay;
    bool capslock;
} event_control_keyboard_args_t;

typedef struct _event {
    event_type_t type;
    union {
        coprocessor_input_message_t   args_input_button;
        event_input_keyboard_args_t   args_input_keyboard;
        event_control_keyboard_args_t args_control_keyboard;
    };
} event_t;
