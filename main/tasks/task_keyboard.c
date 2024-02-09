#include "task_keyboard.h"
#include "bsp.h"
#include "esp_log.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include <stdint.h>

#define SW1_CP 0x01
#define SW2_CP 0x02
#define SW3_CP 0x04
#define SW4_CP 0x08
#define SW5_CP 0x10

static const char* TAG = "keyboard";

typedef struct _keyboard_state {
    uint8_t button_state_left;
    uint8_t button_state_right;
    uint8_t button_state_press;
    bool    wait_for_release;
    bool    enable_typing;
    bool    enable_rot_action;
    bool    enable_rotations[NUM_ROTATION];
    bool    enable_actions[NUM_SWITCHES];
    bool    enable_characters[NUM_CHARACTERS];
    bool    enable_leds;
    bool    enable_relay;
    bool    capslock;
} keyboard_state_t;

void keyboard_handle_input(keyboard_state_t* state, coprocessor_input_message_t* input, QueueHandle_t output_queue) {
    state->button_state_left  &= ~(1 << input->button);
    state->button_state_right &= ~(1 << input->button);
    state->button_state_press &= ~(1 << input->button);
    switch (input->state) {
        case SWITCH_LEFT: state->button_state_left |= 1 << input->button; break;
        case SWITCH_RIGHT: state->button_state_right |= 1 << input->button; break;
        case SWITCH_PRESS: state->button_state_press |= 1 << input->button; break;
    }

    uint32_t leds          = 0;
    int      led_line_flag = 1;
    char     character     = '\0';

    if (state->wait_for_release) {
        if (((!state->button_state_left) || (!state->button_state_right)) && (!state->button_state_press)) {
            if (state->enable_relay) {
                bsp_set_relay(false);
            }
            state->wait_for_release = false;
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    } else if (state->enable_typing && (state->button_state_left || state->button_state_right) && state->button_state_press) {
        switch (state->button_state_left) {
            case SW1_CP:
                character  = '1';
                leds      |= LED_H + LED_H;
                break;
            case SW2_CP:
                character  = '2';
                leds      |= LED_M + LED_I;
                break;
            case SW3_CP:
                character  = '3';
                leds      |= LED_R + LED_N;
                break;
            case SW4_CP:
                character  = '4';
                leds      |= LED_V + LED_S;
                break;
            case SW5_CP:
                character  = '5';
                leds      |= LED_Y + LED_W;
                break;
            default: break;
        }
        switch (state->button_state_right) {
            case SW5_CP:
                character  = '0';
                leds      |= LED_L + LED_G;
                break;
            case SW4_CP:
                character  = '9';
                leds      |= LED_P + LED_K;
                break;
            case SW3_CP:
                character  = '8';
                leds      |= LED_T + LED_O;
                break;
            case SW2_CP:
                character  = '7';
                leds      |= LED_W + LED_S;
                break;
            case SW1_CP:
                character  = '6';
                leds      |= LED_Y + LED_V;
                break;
            default: break;
        }
    } else if (state->button_state_press || ((state->button_state_left || state->button_state_right) && state->enable_rot_action)) {
        // Action
        if (1) {
            int action    = -1;
            int rotation  = -1;
            led_line_flag = 0;
            switch (state->button_state_press) {
                case (1 << 0): action = SWITCH_1; break;
                case (1 << 1): action = SWITCH_2; break;
                case (1 << 2): action = SWITCH_3; break;
                case (1 << 3): action = SWITCH_4; break;
                case (1 << 4): action = SWITCH_5; break;
            }
            if (state->enable_rot_action) {
                switch (state->button_state_left) {
                    case (1 << 0): rotation = SWITCH_L1; break;
                    case (1 << 1): rotation = SWITCH_L2; break;
                    case (1 << 2): rotation = SWITCH_L3; break;
                    case (1 << 3): rotation = SWITCH_L4; break;
                    case (1 << 4): rotation = SWITCH_L5; break;
                }

                switch (state->button_state_right) {
                    case (1 << 0): rotation = SWITCH_R1; break;
                    case (1 << 1): rotation = SWITCH_R2; break;
                    case (1 << 2): rotation = SWITCH_R3; break;
                    case (1 << 3): rotation = SWITCH_R4; break;
                    case (1 << 4): rotation = SWITCH_R5; break;
                }
            }

            if ((action >= 0 && state->enable_actions[action]) ||
                (rotation >= 0 && state->enable_rotations[rotation - NUM_SWITCHES])) {
                if (state->enable_relay) {
                    bsp_set_relay(true);
                }
                state->wait_for_release = true;

                event_t event;
                event.type                          = event_input_keyboard;
                event.args_input_keyboard.character = '\0';
                if (action >= 0)
                    event.args_input_keyboard.action = action;
                if (rotation >= 0)
                    event.args_input_keyboard.action = rotation;
                xQueueSend(output_queue, &event, 0);
            }
        }
    }
else if ((state->button_state_left && state->button_state_right && state->enable_typing)||((state->button_state_left == 0x09) && state->enable_typing)||((state->button_state_left == 0x06) && state->enable_typing)||((state->button_state_right == 0x18) && state->enable_typing)||((state->button_state_right == 0x14) && state->enable_typing)||((state->button_state_right == 0x12) && state->enable_typing)||((state->button_state_right == 0x11) && state->enable_typing)) {
        // Select character
        if (state->button_state_left == SW1_CP && state->button_state_right == SW5_CP && state->enable_characters[0]) {
            character  = 'A';
            leds      |= LED_A;
        }
        if (state->button_state_left == SW1_CP && state->button_state_right == SW4_CP && state->enable_characters[1]) {
            character  = 'B';
            leds      |= LED_B;
        }
        // C
        if (state->button_state_left == 0x09 && state->enable_characters[2]) {
            character  = 'C';
            leds      |= LED_E + LED_G + LED_F + LED_H + LED_M + LED_R + LED_S + LED_T;
        }

        if (state->button_state_left == SW2_CP && state->button_state_right == SW5_CP && state->enable_characters[3]) {
            character  = 'D';
            leds      |= LED_D;
        }
        if (state->button_state_left == SW1_CP && state->button_state_right == SW3_CP && state->enable_characters[4]) {
            character  = 'E';
            leds      |= LED_E;
        }
        if (state->button_state_left == SW2_CP && state->button_state_right == SW4_CP && state->enable_characters[5]) {
            character  = 'F';
            leds      |= LED_F;
        }
        if (state->button_state_left == SW3_CP && state->button_state_right == SW5_CP && state->enable_characters[6]) {
            character  = 'G';
            leds      |= LED_G;
        }
        if (state->button_state_left == SW1_CP && state->button_state_right == SW2_CP && state->enable_characters[7]) {
            character  = 'H';
            leds      |= LED_H;
        }
        if (state->button_state_left == SW2_CP && state->button_state_right == SW3_CP && state->enable_characters[8]) {
            character  = 'I';
            leds      |= LED_I;
        }
        // J
        if (state->button_state_left == 0x06 && state->enable_characters[9]) {
            character  = 'J';
            leds      |= LED_E + LED_F + LED_G + LED_K + LED_O + LED_R + LED_S + LED_M;
        }
        if (state->button_state_left == SW3_CP && state->button_state_right == SW4_CP && state->enable_characters[10]) {
            character  = 'K';
            leds      |= LED_K;
        }
        if (state->button_state_left == SW4_CP && state->button_state_right == SW5_CP && state->enable_characters[11]) {
            character  = 'L';
            leds      |= LED_L;
        }
        if (state->button_state_left == SW2_CP && state->button_state_right == SW1_CP && state->enable_characters[12]) {
            character  = 'M';
            leds      |= LED_M;
        }
        if (state->button_state_left == SW3_CP && state->button_state_right == SW2_CP && state->enable_characters[13]) {
            character  = 'N';
            leds      |= LED_N;
        }
        if (state->button_state_left == SW4_CP && state->button_state_right == SW3_CP && state->enable_characters[14]) {
            character  = 'O';
            leds      |= LED_O;
        }
        if (state->button_state_left == SW5_CP && state->button_state_right == SW4_CP && state->enable_characters[15]) {
            character  = 'P';
            leds      |= LED_P;
        }
        // Q
        if (state->button_state_right == 0x18 && state->enable_characters[16]) {
            character  = 'Q';
            leds      |= LED_E + LED_H + LED_M + LED_R + LED_S + LED_O + LED_K + LED_F + LED_T;
        }
        if (state->button_state_left == SW3_CP && state->button_state_right == SW1_CP && state->enable_characters[17]) {
            character  = 'R';
            leds      |= LED_R;
        }
        if (state->button_state_left == SW4_CP && state->button_state_right == SW2_CP && state->enable_characters[18]) {
            character  = 'S';
            leds      |= LED_S;
        }
        if (state->button_state_left == SW5_CP && state->button_state_right == SW3_CP && state->enable_characters[19]) {
            character  = 'T';
            leds      |= LED_T;
        }
        // U
        if (state->button_state_right == 0x14 && state->enable_characters[20]) {
            character  = 'U';
            leds      |= LED_H + LED_M + LED_R + LED_S + LED_T + LED_P + LED_L;
        }
        if (state->button_state_left == SW4_CP && state->button_state_right == SW1_CP && state->enable_characters[21]) {
            character  = 'V';
            leds      |= LED_V;
        }
        if (state->button_state_left == SW5_CP && state->button_state_right == SW2_CP && state->enable_characters[22]) {
            character  = 'W';
            leds      |= LED_W;
        }
        // X
        if (state->button_state_right == 0x12 && state->enable_characters[23]) {
            character  = 'X';
            leds      |= LED_E + LED_I + LED_G + LED_K + LED_N + LED_R + LED_O + LED_T;
        }
        if (state->button_state_left == SW5_CP && state->button_state_right == SW1_CP && state->enable_characters[24]) {
            character  = 'Y';
            leds      |= LED_Y;
        }
        // Z
        if (state->button_state_right == 0x11 && state->enable_characters[25]) {
            character  = 'Z';
            leds      |= LED_E + LED_F + LED_G + LED_K + LED_N + LED_R + LED_S + LED_T;
        }

        if (character >= 'a' && character <= 'z' && !state->capslock) {
            character += 32;  // replace uppercase character with lowercase character
        }
    }

    if (character != 0 && state->enable_typing) {
        led_line_flag = 0;
        if (state->enable_relay) {
            bsp_set_relay(true);
        }
        state->wait_for_release = true;
        event_t event;
        event.type                          = event_input_keyboard;
        event.args_input_keyboard.character = character;
        event.args_input_keyboard.action    = -1;
        xQueueSend(output_queue, &event, 0);
    }

    if (led_line_flag) {
        // Select row
        if (state->button_state_left & SW1_CP)
            leds |= LED_A | LED_B | LED_E | LED_H;
        if (state->button_state_left & SW2_CP)
            leds |= LED_M | LED_I | LED_F | LED_D;
        if (state->button_state_left & SW3_CP)
            leds |= LED_R | LED_N | LED_K | LED_G;
        if (state->button_state_left & SW4_CP)
            leds |= LED_V | LED_S | LED_O | LED_L;
        if (state->button_state_left & SW5_CP)
            leds |= LED_Y | LED_W | LED_T | LED_P;
        if (state->button_state_right & SW1_CP)
            leds |= LED_M | LED_R | LED_V | LED_Y;
        if (state->button_state_right & SW2_CP)
            leds |= LED_H | LED_N | LED_S | LED_W;
        if (state->button_state_right & SW3_CP)
            leds |= LED_E | LED_I | LED_O | LED_T;
        if (state->button_state_right & SW4_CP)
            leds |= LED_B | LED_F | LED_K | LED_P;
        if (state->button_state_right & SW5_CP)
            leds |= LED_A | LED_D | LED_G | LED_L;
    }

    if (state->enable_leds) {
        esp_err_t _res = bsp_set_leds(leds);
        if (_res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set LEDs (%d)\n", _res);
        }
    }
}

void keyboard_handle_control(keyboard_state_t* state, event_control_keyboard_args_t* input) {
    state->enable_typing = input->enable_typing;
    for (uint32_t index = 0; index < NUM_SWITCHES; index++) {
        state->enable_actions[index] = input->enable_actions[index];
    }
    state->enable_rot_action = false;
    for (uint32_t index = 0; index < NUM_ROTATION; index++) {
        state->enable_rotations[index] = input->enable_rotations[index];
        if (input->enable_rotations[index])
            state->enable_rot_action = true;
    }
    for (uint32_t index = 0; index < NUM_CHARACTERS; index++) {
        state->enable_characters[index] = input->enable_characters[index];
    }
    state->enable_leds  = input->enable_leds;
    state->enable_relay = input->enable_relay;
    state->capslock     = input->capslock;
}

void task_keyboard(void* pvParameters) {
    task_keyboard_parameters_t* parameters = (task_keyboard_parameters_t*)pvParameters;

    QueueHandle_t input_queue  = parameters->input_queue;
    QueueHandle_t output_queue = parameters->output_queue;

    keyboard_state_t state = {0};
    state.enable_typing    = true;
    for (uint32_t index = 0; index < NUM_SWITCHES; index++) {
        state.enable_actions[index] = true;
    }
    for (uint32_t index = 0; index < NUM_ROTATION; index++) {
        state.enable_rotations[index] = false;
    }
    for (uint32_t index = 0; index < NUM_CHARACTERS; index++) {
        state.enable_characters[index] = true;
    }
    state.enable_leds  = true;
    state.enable_relay = true;

    while (1) {
        event_t event = {0};
        if (xQueueReceive(input_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: keyboard_handle_input(&state, &event.args_input_button, output_queue); break;
                case event_control_keyboard: keyboard_handle_control(&state, &event.args_control_keyboard); break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

esp_err_t start_task_keyboard(QueueHandle_t input_queue, QueueHandle_t output_queue) {
    task_keyboard_parameters_t parameters = {0};
    parameters.input_queue                = input_queue;
    parameters.output_queue               = output_queue;
    BaseType_t res                        = xTaskCreate(task_keyboard, "keyboard", 2048, (void*)&parameters, 1, NULL);
    return (res == pdPASS) ? ESP_OK : ESP_FAIL;
}
