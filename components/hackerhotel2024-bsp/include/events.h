#pragma once

#include "badge-communication-protocol.h"
#include "bsp.h"
#include "task_keyboard.h"

typedef enum _event_type {
    event_input_button,
    event_input_keyboard,
    event_control_keyboard,
    event_communication,
    event_term_input,
} event_type_t;

typedef struct _event_input_keyboard_args {
    char character;
    int  action;
} event_input_keyboard_args_t;

typedef struct _event_control_keyboard_args {
    bool enable_typing;
    bool enable_actions[NUM_SWITCHES];
    bool enable_rotations[NUM_ROTATION];
    bool enable_characters[NUM_CHARACTERS];
    bool enable_leds;
    bool enable_relay;
    bool capslock;
} event_control_keyboard_args_t;

typedef struct _event_communication_args {
    badge_comms_message_type_t type;
    ieee802154_address_t       src;
    ieee802154_address_t       dst;
    union {
        uint8_t                    data[BADGE_COMMS_USER_DATA_MAX_LEN];
        badge_message_time_t       data_time;
        badge_message_chat_t       data_chat;
        badge_message_repertoire_t data_repertoire;
        badge_message_battleship_t data_battleship;
    };
} event_communication_args_t;

typedef struct _event {
    event_type_t type;
    union {
        coprocessor_input_message_t   args_input_button;
        event_input_keyboard_args_t   args_input_keyboard;
        event_control_keyboard_args_t args_control_keyboard;
        event_communication_args_t    args_communication;
        char                          args_term_input;
    };
} event_t;
