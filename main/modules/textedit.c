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
extern uint8_t const page1_png_start[] asm("_binary_page1_png_start");
extern uint8_t const page1_png_end[] asm("_binary_page1_png_end");
extern uint8_t const page2_png_start[] asm("_binary_page2_png_start");
extern uint8_t const page2_png_end[] asm("_binary_page2_png_end");
extern uint8_t const page3_png_start[] asm("_binary_page3_png_start");
extern uint8_t const page3_png_end[] asm("_binary_page3_png_end");

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
    pax_draw_line(gfx, BLACK, 0, 20, 295, 20);
    pax_draw_text(gfx, BLACK, font, 18, 5, 25, value);
    pax_insert_png_buf(gfx, textedit_png_start, textedit_png_end - textedit_png_start, 0, gfx->width - 24, 0);
    if (capslock) {
        pax_insert_png_buf(gfx, capslock_png_start, capslock_png_end - capslock_png_start, 135, gfx->width - 24, 0);
    }
    bsp_display_flush();
}

void special_draw(char const * title, char* value, uint8_t page) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_insert_png_buf(gfx, special_png_start, special_png_end - special_png_start, 0, 0, 0);
    if (page == 0) {
        pax_insert_png_buf(gfx, page1_png_start, page1_png_end - page1_png_start, 135, gfx->width - 24, 0);
    }
    if (page == 1) {
        pax_insert_png_buf(gfx, page2_png_start, page2_png_end - page2_png_start, 193, gfx->width - 24, 0);
    }
    if (page == 2) {
        pax_insert_png_buf(gfx, page3_png_start, page3_png_end - page3_png_start, 251, gfx->width - 24, 0);
    }
    bsp_display_flush();
}

char translate_special_character(char input, uint8_t page) {
    switch (page) {
        case 0:
            {
                switch (input) {
                    case 'a': return ' '; break;
                    case 'b': return 'c'; break;
                    case 'd': return 'C'; break;
                    case 'e': return 'j'; break;
                    case 'f': return 'J'; break;
                    case 'g': return 'q'; break;
                    case 'h': return 'Q'; break;
                    case 'i': return 'u'; break;
                    case 'k': return 'U'; break;
                    case 'l': return 'x'; break;
                    case 'm': return 'X'; break;
                    case 'n': return 'z'; break;
                    case 'o': return 'Z'; break;
                    case 'p': return '!'; break;
                    case 'r': return '@'; break;
                    case 's': return '#'; break;
                    case 't': return '$'; break;
                    case 'v': return '%'; break;
                    case 'w': return '^'; break;
                    case 'y': return '&'; break;
                    default: break;
                }
                break;
            }
        case 1:
            {
                switch (input) {
                    case 'a': return '0'; break;
                    case 'b': return '1'; break;
                    case 'd': return '2'; break;
                    case 'e': return '3'; break;
                    case 'f': return '4'; break;
                    case 'g': return '5'; break;
                    case 'h': return '6'; break;
                    case 'i': return '7'; break;
                    case 'k': return '8'; break;
                    case 'l': return '9'; break;
                    case 'm': return '('; break;
                    case 'n': return ')'; break;
                    case 'o': return '-'; break;
                    case 'p': return '_'; break;
                    case 'r': return '='; break;
                    case 's': return '+'; break;
                    case 't': return '"'; break;
                    case 'v': return '\''; break;
                    case 'w': return ':'; break;
                    case 'y': return ';'; break;
                    default: break;
                }
                break;
            }
        case 2:
            {
                switch (input) {
                    case 'a': return '~'; break;
                    case 'b': return '`'; break;
                    case 'd': return '*'; break;
                    case 'e': return 'e'; break;
                    case 'f': return '.'; break;
                    case 'g': return ','; break;
                    case 'h': return '{'; break;
                    case 'i': return '}'; break;
                    case 'k': return '['; break;
                    case 'l': return ']'; break;
                    case 'm': return '\\'; break;
                    case 'n': return '/'; break;
                    case 'o': return '|'; break;
                    case 'p': return '?'; break;
                    case 'r': return '<'; break;
                    case 's': return '>'; break;
                    case 't': return ' '; break;
                    case 'v': return ' '; break;
                    case 'w': return ' '; break;
                    case 'y': return ' '; break;
                    default: break;
                }
                break;
            }
        default: break;
    }

    return ' ';
}

void special_character(
    char const *  title,
    QueueHandle_t application_event_queue,
    QueueHandle_t keyboard_event_queue,
    char*         output,
    size_t        output_size
) {

    event_t kbsettings = {
        .type                                 = event_control_keyboard,
        .args_control_keyboard.enable_typing  = true,
        .args_control_keyboard.enable_actions = {false, true, true, true, true},
        .args_control_keyboard.enable_leds    = true,
        .args_control_keyboard.enable_relay   = true,
        .args_control_keyboard.capslock       = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    uint8_t page = 0;
    special_draw(title, output, page);

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: break;
                        case SWITCH_2: return; break;
                        case SWITCH_3:
                            {
                                page = 0;
                                break;
                            }
                        case SWITCH_4:
                            {
                                page = 1;
                                break;
                            }
                        case SWITCH_5:
                            {
                                page = 2;
                                break;
                            }
                        default: break;
                    }
                    if (event.args_input_keyboard.character != '\0') {
                        printf("Keyboard input:  %c\n", event.args_input_keyboard.character);
                        size_t length = strlen(output) + 1;
                        if (length < output_size) {
                            output[length - 1] = translate_special_character(event.args_input_keyboard.character, page);
                            output[length]     = '\0';
                        }
                        return;
                    }
                    special_draw(title, output, page);
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
                            configure_keyboard(keyboard_event_queue, capslock);
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
