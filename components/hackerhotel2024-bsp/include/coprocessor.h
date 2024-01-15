#pragma once

#include "esp_err.h"

// Registers
#define COPROCESSOR_REG_FW_VERSION 0
#define COPROCESSOR_REG_LED        4
#define COPROCESSOR_REG_BTN        8

// LEDs
#define LED_A (1 << 0)
#define LED_B (1 << 1)
#define LED_D (1 << 2)
#define LED_E (1 << 3)
#define LED_F (1 << 4)
#define LED_G (1 << 5)
#define LED_L (1 << 6)
#define LED_I (1 << 7)
#define LED_K (1 << 8)
#define LED_H (1 << 9)
#define LED_M (1 << 10)
#define LED_N (1 << 11)
#define LED_O (1 << 12)
#define LED_P (1 << 13)
#define LED_R (1 << 14)
#define LED_S (1 << 15)
#define LED_T (1 << 16)
#define LED_V (1 << 17)
#define LED_W (1 << 18)
#define LED_Y (1 << 19)

// Buttons
#define SWITCH_IDLE  0
#define SWITCH_LEFT  (1 << 0)
#define SWITCH_PRESS (1 << 1)
#define SWITCH_RIGHT (1 << 2)

#define SWITCH_1 0
#define SWITCH_2 1
#define SWITCH_3 2
#define SWITCH_4 3
#define SWITCH_5 4

// Types
typedef struct _coprocessor_input_message {
    uint8_t button;
    uint8_t state;
} coprocessor_input_message_t;

// Functions
