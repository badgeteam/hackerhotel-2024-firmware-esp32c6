#include "screen_hangman.h"
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
#include "screen_home.h"
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_random.h>
#include <string.h>

char const forfeitprompt[128] = "Do you want to exit and declare forfeit";

extern uint8_t const deibler1_png_start[] asm("_binary_deibler1_png_start");
extern uint8_t const deibler1_png_end[] asm("_binary_deibler1_png_end");

static char const * TAG = "hangman";

void DisplayHangman(
    char disabled_letters[nbletters], char letters_found[longestword], int word_length, int mistake_count
) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_insert_png_buf(gfx, deibler1_png_start, deibler1_png_end - deibler1_png_start, 0, 0, 0);

    int dblock_o_x = 100;
    int dblock_o_y = 20;
    int dblock_g_x = 10;

    int text_o_x = 150;
    int text_o_y = 60;
    int text_g_x = 14;

    char fuckme[5] = "";

    char debugtext[30] = "";
    snprintf(debugtext, 30, "%d mistakes", mistake_count);
    pax_draw_text(gfx, BLACK, font1, fontsizeS, 200, 40, debugtext);

    for (int i = 0; i < mistake_count; i++) {
        AddBlocktoBuffer(dblock_o_x + i * dblock_g_x, dblock_o_y);
        fuckme[0] = disabled_letters[i];
        pax_draw_text(gfx, BLACK, font1, fontsizeS, dblock_o_x - 4 + i * dblock_g_x, dblock_o_y - 5, fuckme);
    }

    ESP_LOGE(TAG, "word_length: %d", word_length);
    for (int i = 0; i < word_length; i++) {
        fuckme[0] = letters_found[i];
        ESP_LOGE(TAG, "i: %d", i);
        if (letters_found[i] == NULL)
            pax_center_text(gfx, BLACK, font1, fontsizeS * 2, text_o_x + text_g_x * i, text_o_y, "-");
        else
            pax_center_text(gfx, BLACK, font1, fontsizeS * 2, text_o_x + text_g_x * i, text_o_y, fuckme);
    }
    // pax_outline_circle(gfx, BLACK, 0, 0, 3);
    bsp_display_flush();
}

screen_t screen_hangman_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, false, false, false, false);
    configure_keyboard_typing(keyboard_event_queue, true);

    char choosen_word[longestword] = "test";
    char letters_found[longestword];
    for (int i = 0; i < longestword; i++) letters_found[i] = NULL;
    char disabled_letters[nbletters];
    for (int i = 0; i < nbletters; i++) disabled_letters[i] = NULL;

    // int word_length = sizeof(choosen_word);
    int word_length = 4;

    ESP_LOGE(TAG, "word_length: %d", word_length);
    int displayflag   = 1;
    int mistake_count = 0;

    for (int i = 0; i < nbletters; i++) disabled_letters[i] = NULL;


    while (1) {
        if (displayflag) {
            DisplayHangman(disabled_letters, letters_found, word_length, mistake_count);
            displayflag = 0;
        }
        event_t event = {0};
        if ((xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE)) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1:
                            if (Screen_Confirmation(forfeitprompt, application_event_queue, keyboard_event_queue))
                                InitKeyboard(keyboard_event_queue);
                            return screen_home;
                            break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (event.args_input_keyboard.character != '\0') {
                        char input = event.args_input_keyboard.character;
                        ESP_LOGE(TAG, "Input: %c", input);
                        int flagcorrectguess = 0;
                        for (int i = 0; i < sizeof(choosen_word); i++)
                            if (input == choosen_word[i]) {
                                letters_found[i] = input;
                                ESP_LOGE(TAG, "match location: %c", letters_found[i]);
                                ESP_LOGE(TAG, "match location: %d", i);
                                flagcorrectguess = 1;
                            }
                        ESP_LOGE(TAG, "flagcorrectguess: %d", flagcorrectguess);

                        if (!flagcorrectguess) {
                            ESP_LOGE(TAG, "inbcorrect guess");
                            ESP_LOGE(TAG, "letter nb: %d", InputtoNum(input));
                            disabled_letters[mistake_count] = input;
                            mistake_count++;
                        }
                        configure_keyboard_character(keyboard_event_queue, InputtoNum(input), false);
                        displayflag = 1;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
