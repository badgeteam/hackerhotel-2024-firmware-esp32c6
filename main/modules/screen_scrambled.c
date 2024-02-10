#include "screen_scrambled.h"
#include "application.h"
#include "badge-communication-protocol.h"
#include "badge-communication.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_random.h>
#include <string.h>

static const char* TAG = "scrambled";

extern const uint8_t caronv_png_start[] asm("_binary_caronv_png_start");
extern const uint8_t caronv_png_end[] asm("_binary_caronv_png_end");
extern const uint8_t b_arrow1_png_start[] asm("_binary_b_arrow1_png_start");
extern const uint8_t b_arrow1_png_end[] asm("_binary_b_arrow1_png_end");
extern const uint8_t b_arrow1t_png_start[] asm("_binary_b_arrow1t_png_start");
extern const uint8_t b_arrow1t_png_end[] asm("_binary_b_arrow1t_png_end");

static const char* const scrambled[] = {"to the president of the united", ""};

screen_t screen_scrambled_victory(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);

void DisplayScrambled(char usertext[500], int cursor, int cursortoggle) {
    pax_buf_t* gfx       = bsp_get_gfx_buffer();
    pax_vec1_t cursorloc = {
        .x = 0,
        .y = 0,
    };
    pax_background(gfx, WHITE);
    AddSWtoBuffer("Exit", "Delete", "Space", "", "Finish");
    if (cursortoggle == 0)
        pax_insert_png_buf(gfx, b_arrow1_png_start, b_arrow1_png_end - b_arrow1_png_start, 64 + 61 + 61, 118, 0);
    else
        pax_insert_png_buf(gfx, b_arrow1t_png_start, b_arrow1t_png_end - b_arrow1t_png_start, 64 + 61 + 61, 118, 0);

    WallofText(30, scrambled[0], 0, cursor);

    // draw user taxt + cursor
    cursorloc = WallofText(75, usertext, 0, cursor);
    pax_draw_line(gfx, BLACK, cursorloc.x + 1, cursorloc.y, cursorloc.x - 1, cursorloc.y);
    pax_draw_line(gfx, BLACK, cursorloc.x, cursorloc.y, cursorloc.x, cursorloc.y + fontsizeS);
    pax_draw_line(gfx, BLACK, cursorloc.x + 1, cursorloc.y + fontsizeS, cursorloc.x - 1, cursorloc.y + fontsizeS);

    bsp_display_flush();
}

int Finish(char usertext[500], int _textID) {
    int correct = 1;
    for (int i = 0; i < strlen(scrambled[0]); i++) {
        uint32_t leds   = 0;
        leds           |= ChartoLED(usertext[i]);
        esp_err_t _res  = bsp_set_leds(leds);
        if (_res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set LEDs (%d)\n", _res);
        }
        if (usertext[i] != scrambled[_textID][i]) {
            correct = 0;
            bsp_set_addressable_led(LED_RED);
            bsp_set_relay(true);
        } else
            bsp_set_relay(false);
        vTaskDelay(pdMS_TO_TICKS(100));
        bsp_set_addressable_led(LED_OFF);
    }
    bsp_set_relay(false);
    esp_err_t _res = bsp_set_leds(0);
    if (_res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LEDs (%d)\n", _res);
    }
    return correct;
}

screen_t screen_scrambled_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    reset_keyboard_settings(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_typing(keyboard_event_queue, true);

    int  textID        = 0;
    char usertext[500] = "to the prqsident of tge united";
    int  cursor        = 0;
    int  displayflag   = 1;
    int  cursortoggle  = 0;

    while (1) {
        event_t event = {0};
        if (displayflag) {
            DisplayScrambled(usertext, cursor, cursortoggle);
            displayflag = 0;
        }
        if ((xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE)) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2:                         // delete
                            if (strlen(usertext) && cursor) {  // do nothing if empty
                                cursor--;
                                if (usertext[cursor] != '\0') {
                                    for (int i = cursor; i < strlen(usertext); i++) usertext[i] = usertext[i + 1];
                                } else
                                    usertext[cursor] = '\0';
                            }
                            break;
                        case SWITCH_3:  // space
                            usertext[cursor] = ' ';
                            cursor++;
                            break;
                        case SWITCH_4:
                            if (!cursortoggle) {
                                cursortoggle = 1;
                                configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_4, true);
                                configure_keyboard_typing(keyboard_event_queue, false);
                            } else {
                                cursortoggle = 0;
                                configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_4, false);
                                configure_keyboard_typing(keyboard_event_queue, true);
                            }
                            break;
                        case ROTATION_L4:
                            if (cursor > 0)
                                cursor--;
                            break;
                        case ROTATION_R4:
                            if (usertext[cursor] != '\0')
                                cursor++;
                            break;
                        case SWITCH_5:  // finish
                            if (Finish(usertext, textID))
                                return screen_scrambled_victory(application_event_queue, keyboard_event_queue);
                            break;
                        default: break;
                    }
                    if (event.args_input_keyboard.character != '\0') {
                        if (usertext[cursor] != '\0') {
                            for (int i = strlen(usertext); i >= cursor; i--) usertext[i + 1] = usertext[i];
                        }
                        usertext[cursor] = event.args_input_keyboard.character;
                        cursor++;
                    }
                    displayflag = 1;
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_scrambled_victory(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_insert_png_buf(gfx, caronv_png_start, caronv_png_end - caronv_png_start, 0, 0, 0);
    bsp_display_flush();

    reset_keyboard_settings(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);

    while (1) {
        event_t event = {0};
        if ((xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE)) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: return screen_home; break;
                        case SWITCH_3: return screen_home; break;
                        case SWITCH_4: return screen_home; break;
                        case SWITCH_5: return screen_home; break;
                        default: break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
