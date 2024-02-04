#include "screen_pointclick.h"
#include "application.h"
#include "badge_messages.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screen_home.h"
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_random.h>
#include <string.h>

static char const * TAG = "point & click";

extern uint8_t const dock1n_png_start[] asm("_binary_dock1n_png_start");
extern uint8_t const dock1n_png_end[] asm("_binary_dock1n_png_end");
extern uint8_t const dock1e_png_start[] asm("_binary_dock1e_png_start");
extern uint8_t const dock1e_png_end[] asm("_binary_dock1e_png_end");
extern uint8_t const dock1s_png_start[] asm("_binary_dock1s_png_start");
extern uint8_t const dock1s_png_end[] asm("_binary_dock1s_png_end");
extern uint8_t const dock1w_png_start[] asm("_binary_dock1w_png_start");
extern uint8_t const dock1w_png_end[] asm("_binary_dock1w_png_end");

extern uint8_t const dock2n_png_start[] asm("_binary_dock2n_png_start");
extern uint8_t const dock2n_png_end[] asm("_binary_dock2n_png_end");
extern uint8_t const dock2e_png_start[] asm("_binary_dock2e_png_start");
extern uint8_t const dock2e_png_end[] asm("_binary_dock2e_png_end");
extern uint8_t const dock2s_png_start[] asm("_binary_dock2s_png_start");
extern uint8_t const dock2s_png_end[] asm("_binary_dock2s_png_end");
extern uint8_t const dock2w_png_start[] asm("_binary_dock2w_png_start");
extern uint8_t const dock2w_png_end[] asm("_binary_dock2w_png_end");

extern uint8_t const dune1n_png_start[] asm("_binary_dune1n_png_start");
extern uint8_t const dune1n_png_end[] asm("_binary_dune1n_png_end");
extern uint8_t const dune1s_png_start[] asm("_binary_dune1s_png_start");
extern uint8_t const dune1s_png_end[] asm("_binary_dune1s_png_end");

extern uint8_t const dune2n_png_start[] asm("_binary_dune2n_png_start");
extern uint8_t const dune2n_png_end[] asm("_binary_dune2n_png_end");
extern uint8_t const dune2s_png_start[] asm("_binary_dune2s_png_start");
extern uint8_t const dune2s_png_end[] asm("_binary_dune2s_png_end");

screen_t screen_pointclick_dock1(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
screen_t screen_pointclick_dock2(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
screen_t screen_pointclick_dune1(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
screen_t screen_pointclick_dune2(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);

screen_t screen_pointclick_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    screen_t current_screen_PC = screen_PC_dock1;
    while (1) {
        switch (current_screen_PC) {
            case screen_PC_dock1:
                {
                    current_screen_PC = screen_pointclick_dock1(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_PC_dock2:
                {
                    current_screen_PC = screen_pointclick_dock2(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_PC_dune1:
                {
                    current_screen_PC = screen_pointclick_dune1(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_PC_dune2:
                {
                    current_screen_PC = screen_pointclick_dune2(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_home:
            default: return screen_home; break;
        }
    }
}

screen_t screen_pointclick_dock1(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);

    int cursor      = screen_PC_s;
    int nbdirection = 4;
    ESP_LOGE(TAG, "dock 1");

    bsp_apply_lut(lut_4s);

    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_insert_png_buf(bsp_get_gfx_buffer(), dock1s_png_start, dock1s_png_end - dock1s_png_start, 0, 0, 0);
    bsp_display_flush();

    while (1) {
        int     flagchangescreen = 0;
        event_t event            = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: break;
                        case SWITCH_3: flagchangescreen++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    switch (cursor) {
                        case screen_PC_n:
                            {
                                pax_insert_png_buf(gfx, dock1n_png_start, dock1n_png_end - dock1n_png_start, 0, 0, 0);
                                break;
                            }
                        case screen_PC_e:
                            {
                                pax_insert_png_buf(gfx, dock1e_png_start, dock1e_png_end - dock1e_png_start, 0, 0, 0);
                                break;
                            }
                        case screen_PC_s:
                            {
                                pax_insert_png_buf(gfx, dock1s_png_start, dock1s_png_end - dock1s_png_start, 0, 0, 0);
                                break;
                            }
                        case screen_PC_w:
                            {
                                if (flagchangescreen)
                                    return screen_PC_dock2;
                                pax_insert_png_buf(gfx, dock1w_png_start, dock1w_png_end - dock1w_png_start, 0, 0, 0);
                                break;
                            }
                        case 10: return screen_home; break;
                    }
                    bsp_display_flush();
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_dock2(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);

    int cursor      = screen_PC_w;
    int nbdirection = 4;
    ESP_LOGE(TAG, "dock 2");

    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_insert_png_buf(bsp_get_gfx_buffer(), dock2w_png_start, dock2w_png_end - dock2w_png_start, 0, 0, 0);
    bsp_display_flush();

    while (1) {
        int     flagchangescreen = 0;
        event_t event            = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: break;
                        case SWITCH_3: flagchangescreen++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }


                    ESP_LOGE(TAG, "cursor %d", cursor);
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    switch (cursor) {
                        case screen_PC_n:
                            {
                                pax_insert_png_buf(gfx, dock2n_png_start, dock2n_png_end - dock2n_png_start, 0, 0, 0);
                                break;
                            }
                        case screen_PC_e:
                            {
                                pax_insert_png_buf(gfx, dock2e_png_start, dock2e_png_end - dock2e_png_start, 0, 0, 0);
                                break;
                            }
                        case screen_PC_s:
                            {
                                pax_insert_png_buf(gfx, dock2s_png_start, dock2s_png_end - dock2s_png_start, 0, 0, 0);
                                break;
                            }
                        case screen_PC_w:
                            {
                                if (flagchangescreen)
                                    return screen_PC_dune1;
                                pax_insert_png_buf(gfx, dock2w_png_start, dock2w_png_end - dock2w_png_start, 0, 0, 0);
                                break;
                            }
                        case 10: return screen_home; break;
                    }
                    bsp_display_flush();
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_dune1(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);

    int cursor      = screen_PC_n;
    int nbdirection = 2;
    ESP_LOGE(TAG, "dune 1");

    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_insert_png_buf(bsp_get_gfx_buffer(), dune1n_png_start, dune1n_png_end - dune1n_png_start, 0, 0, 0);
    bsp_display_flush();

    while (1) {
        int     flagchangescreen = 0;
        event_t event            = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: break;
                        case SWITCH_3: flagchangescreen++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }


                    ESP_LOGE(TAG, "cursor %d", cursor);
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    switch (cursor) {
                        case 0:  // North
                            {
                                pax_insert_png_buf(gfx, dune1n_png_start, dune1n_png_end - dune1n_png_start, 0, 0, 0);
                                break;
                            }
                        case 1:  // South
                            {
                                if (flagchangescreen)
                                    return screen_PC_dune2;
                                pax_insert_png_buf(gfx, dune1s_png_start, dune1s_png_end - dune1s_png_start, 0, 0, 0);
                                break;
                            }
                        case 10: return screen_home; break;
                    }
                    bsp_display_flush();
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_dune2(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);

    int cursor      = screen_PC_n;
    int nbdirection = 2;
    ESP_LOGE(TAG, "dune 2");

    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_insert_png_buf(bsp_get_gfx_buffer(), dune2n_png_start, dune2n_png_end - dune2n_png_start, 0, 0, 0);
    bsp_display_flush();

    while (1) {
        int     flagchangescreen = 0;
        event_t event            = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: break;
                        case SWITCH_3: flagchangescreen++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    switch (cursor) {
                        case 0:  // North
                            {
                                pax_insert_png_buf(gfx, dune2n_png_start, dune2n_png_end - dune2n_png_start, 0, 0, 0);
                                break;
                            }
                        case 1:  // South
                            {
                                pax_insert_png_buf(gfx, dune2s_png_start, dune2s_png_end - dune2s_png_start, 0, 0, 0);
                                break;
                            }
                        case 10: return screen_home; break;
                    }
                    bsp_display_flush();
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}