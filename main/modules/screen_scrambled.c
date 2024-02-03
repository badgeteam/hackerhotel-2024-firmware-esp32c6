#include "screen_scrambled.h"
#include "application.h"
#include "badge_comms.h"
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
#include "nvs_flash.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_random.h>
#include <string.h>

static char const * TAG = "scrambled";

// extern uint8_t const border1_png_start[] asm("_binary_border1_png_start");
// extern uint8_t const border1_png_end[] asm("_binary_border1_png_end");
static char const * const scrambled[] = {"to the president of the united states wgisahontn", ""};

// static char const * const correct[]   = {"To the President of the United States, Washington", ""};



// static char const * const scrambled1[] = {"To", "the", "President", "of", "the", "United", "States,", "Washington"};
// static char const * const text1[]      = {"To", "the", "President", "of", "the", "United", "States,", "Wgisahontn"};

void DisplayScrambled(char usertext[500]) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    // strcpy(scrambled1[0], "");
    pax_background(gfx, WHITE);
    AddSWtoBuffer("Exit", "Delete", "Space", "", "Finish");
    pax_draw_text(gfx, BLACK, font1, fontsizeS, 0, 0, "its coordinates");

    // pax_outline_circle(gfx, BLACK, 0, 0, 3);
    WallofText(30, scrambled[0], 0);
    WallofText(75, usertext, 0);

    // DisplayWallofText(fontsizeS, 285, 4, gfx->height, 30, scrambled, 0);
    // pax_insert_png_buf(gfx, border1_png_start, border1_png_end - border1_png_start, 0, 127 - 11, 0);
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
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_typing(keyboard_event_queue, true);
    // configure_keyboard_rotate_both(keyboard_event_queue, 4, true);

    int  textID        = 0;
    char usertext[500] = "to the prqsident of tge united stases washington";
    int  cursor        = 0;
    int  displayflag   = 1;
    DisplayScrambled(usertext);

    while (1) {
        event_t event = {0};
        if (displayflag) {
            DisplayScrambled(usertext);
            displayflag = 0;
        }
        if ((xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE)) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2:  // delete
                            cursor--;
                            usertext[cursor] = '\0';
                            break;
                        case SWITCH_3:  // space
                            usertext[cursor] = ' ';
                            cursor++;
                            break;
                        case SWITCH_4: break;
                        case SWITCH_5:  // finish
                            Finish(usertext, textID);
                            break;
                        default: break;
                    }
                    if (event.args_input_keyboard.character != '\0') {
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
