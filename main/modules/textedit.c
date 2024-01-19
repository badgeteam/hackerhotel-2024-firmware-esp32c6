#include "textedit.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static char const * TAG = "textedit";

extern uint8_t const textedit_png_start[] asm("_binary_textedit_png_start");
extern uint8_t const textedit_png_end[] asm("_binary_textedit_png_end");
extern uint8_t const capslock_png_start[] asm("_binary_capslock_png_start");
extern uint8_t const capslock_png_end[] asm("_binary_capslock_png_end");
extern uint8_t const special_png_start[] asm("_binary_special_png_start");
extern uint8_t const special_png_end[] asm("_binary_special_png_end");

void flush_event_queue(QueueHandle_t queue) {
    event_t event = {0};
    while (xQueueReceive(queue, &event, 0) == pdTRUE) {
        // empty.
    }
}

void disable_keyboard(QueueHandle_t keyboard_event_queue) {
    event_t kbsettings = {
        .type                                 = event_control_keyboard,
        .args_control_keyboard.enable_typing  = false,
        .args_control_keyboard.enable_actions = {false, false, false, false, false},
        .args_control_keyboard.enable_leds    = true,
        .args_control_keyboard.enable_relay   = true,
        .args_control_keyboard.capslock       = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void configure_keyboard(QueueHandle_t keyboard_event_queue, bool capslock) {
    event_t kbsettings = {
        .type                                 = event_control_keyboard,
        .args_control_keyboard.enable_typing  = true,
        .args_control_keyboard.enable_actions = {true, true, true, true, true},
        .args_control_keyboard.enable_leds    = true,
        .args_control_keyboard.enable_relay   = true,
    };
    kbsettings.args_control_keyboard.capslock = capslock;
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void textedit_draw(char const * title, char* value, bool capslock) {
    pax_font_t const * font = pax_font_saira_regular;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_draw_text(gfx, RED, font, 18, 5, 5, title);
    pax_draw_text(gfx, BLACK, font, 12, 5, 25, value);
    pax_insert_png_buf(gfx, textedit_png_start, textedit_png_end - textedit_png_start, 0, gfx->width - 24, 0);
    if (capslock) {
        pax_insert_png_buf(gfx, capslock_png_start, capslock_png_end - capslock_png_start, 135, gfx->width - 24, 0);
    }
    bsp_display_flush();
}

void special_draw(char const * title, char* value) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_insert_png_buf(gfx, special_png_start, special_png_end - special_png_start, 0, 0, 0);
    bsp_display_flush();
}

void special_character(
    char const *  title,
    QueueHandle_t application_event_queue,
    QueueHandle_t keyboard_event_queue,
    char*         output,
    size_t        output_size
) {

    // THIS FUNCTION IS WORK IN PROGRESS

    event_t kbsettings = {
        .type                                 = event_control_keyboard,
        .args_control_keyboard.enable_typing  = true,
        .args_control_keyboard.enable_actions = {false, true, true, true, false},
        .args_control_keyboard.enable_leds    = true,
        .args_control_keyboard.enable_relay   = true,
        .args_control_keyboard.capslock       = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    special_draw(title, output);

    while (1) {
        event_t event = {0};
        // flush_event_queue(application_event_queue);
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_5:
                        case SWITCH_1: break;
                        case SWITCH_2: return; break;
                        case SWITCH_3:
                            {
                                break;
                            }
                        case SWITCH_4:
                            {
                                break;
                            }
                        default: break;
                    }
                    if (event.args_input_keyboard.character != '\0') {
                        printf("Keyboard input:  %c\n", event.args_input_keyboard.character);
                        size_t length = strlen(output) + 1;
                        if (length < output_size) {
                            output[length - 1] = event.args_input_keyboard.character;
                            output[length]     = '\0';
                        }
                    }
                    special_draw(title, output);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}


bool textedit(
    char const *  title,
    QueueHandle_t application_event_queue,
    QueueHandle_t keyboard_event_queue,
    char*         output,
    size_t        output_size
) {
    bool capslock = false;
    textedit_draw(title, output, capslock);
    configure_keyboard(keyboard_event_queue, capslock);
    while (1) {
        event_t event = {0};
        // flush_event_queue(application_event_queue);
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_5:
                            disable_keyboard(keyboard_event_queue);
                            return true;
                            break;
                        case SWITCH_4:
                            disable_keyboard(keyboard_event_queue);
                            return false;
                            break;
                        case SWITCH_3:
                            {
                                capslock = !capslock;
                                configure_keyboard(keyboard_event_queue, capslock);
                                break;
                            }
                        case SWITCH_2:
                            // Backspace
                            size_t length = strlen(output);
                            if (length > 0) {
                                output[length - 1] = '\0';
                            }
                            break;
                        case SWITCH_1:
                            // Special character menu
                            special_character(
                                title,
                                application_event_queue,
                                keyboard_event_queue,
                                output,
                                output_size
                            );
                            break;
                        default: break;
                    }
                    if (event.args_input_keyboard.character != '\0') {
                        printf("Keyboard input:  %c\n", event.args_input_keyboard.character);
                        size_t length = strlen(output) + 1;
                        if (length < output_size) {
                            output[length - 1] = event.args_input_keyboard.character;
                            output[length]     = '\0';
                        }
                    }
                    textedit_draw(title, output, capslock);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
