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

static const char* const scrambled[] = {
    "to the president of the united states wgisahontn",
    "the qeeun dirsees to corntgtaulae the pdneresit upon the sscefuscul cteoplomin of this graet irttanaeinonl wrok "
    "in wcihh the qeuen has taekn the depeest itrsneet",
    "the eecitrlc clbae which now cncnetos great britain with the uentid staets will pvore aa aaoddiitnl link beewetn "
    "the nntiaos whose fhesndriip ii feonudd upon tiehr coommn itnerset and rapocriecl eseetm",
    "eth uenqe sah umhc elsepura ni tshu omnctianugcmi htiw teh sintprede and eenrignw ot ihm rhe shswei for eth "
    "iyosteprrp of eht etdnui tsstea"
};

static const char* const reference[] = {
    "to the president of the united states washington",
    "the queen desires to congratulate the president upon the successful completion of this great international work "
    "in which the queen has taken the deepest interest",
    "the electric cable which now connects great britain with the united states will prove an additional link between "
    "the nations whose friendship is founded upon their common interest and reciprocal esteem",
    "the queen has much pleasure in thus communicating with the president and renewing to him her wishes for the "
    "prosperity of the united states"
};

screen_t screen_scrambled_victory(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);

void DisplayScrambled(char usertext[250], int cursor, int cursortoggle, int textID) {
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

    WallofText(10, scrambled[textID], 0, cursor);


    // draw user taxt + cursor
    cursorloc = WallofText(65, usertext, 0, cursor);
    pax_draw_line(gfx, BLACK, cursorloc.x + 1, cursorloc.y, cursorloc.x - 1, cursorloc.y);
    pax_draw_line(gfx, BLACK, cursorloc.x, cursorloc.y, cursorloc.x, cursorloc.y + fontsizeS);
    pax_draw_line(gfx, BLACK, cursorloc.x + 1, cursorloc.y + fontsizeS, cursorloc.x - 1, cursorloc.y + fontsizeS);

    bsp_display_flush();
}

int Finish(char usertext[250], int _textID) {
    int correct = 1;
    for (int i = 0; i < strlen(reference[_textID]); i++) {
        uint32_t leds = 0;
        // highlight wrong letters and blink the status LED red
        if (usertext[i] != reference[_textID][i]) {
            correct  = 0;
            leds    |= ChartoLED(usertext[i]);
            bsp_set_addressable_led(LED_RED);
            bsp_set_relay(true);
        } else {
            bsp_set_addressable_led(LED_GREEN);
            bsp_set_relay(false);
        }
        esp_err_t _res = bsp_set_leds(leds);
        if (_res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set LEDs (%d)\n", _res);
        }
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

screen_t screen_scrambled_splash(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int* textID
) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    AddSWtoBuffer("Level 1", "Level 2", "Level 3", "Level 4", "Help");
    // pax_insert_png_buf(gfx, caronv_png_start, caronv_png_end - caronv_png_start, 0, 0, 0);
    bsp_display_flush();
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);

    while (1) {
        event_t event = {0};
        if ((xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE)) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1:
                            *textID = 0;
                            return screen_LH_playing;
                            break;
                        case SWITCH_2:
                            *textID = 1;
                            return screen_LH_playing;
                            break;
                        case SWITCH_3:
                            *textID = 2;
                            return screen_LH_playing;
                            break;
                        case SWITCH_4:
                            *textID = 3;
                            return screen_LH_playing;
                            break;
                        case SWITCH_5: return screen_home; break;
                        default: break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_scrambled_playing(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int* textID
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_typing(keyboard_event_queue, true);

    char usertext[250] = "to the president of the united states washington";
    int  cursor        = 0;
    int  displayflag   = 1;
    int  cursortoggle  = 0;

    while (1) {
        event_t event = {0};
        if (displayflag) {
            DisplayScrambled(usertext, cursor, cursortoggle, *textID);
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
                        case SWITCH_5:
                            if (Finish(usertext, *textID)) {
                                return screen_LH_victory;
                            }
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

    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    vTaskDelay(pdMS_TO_TICKS(3000));

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

screen_t screen_scrambled_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    screen_t current_screen = screen_LH_splash;
    int      textID         = 0;
    while (1) {
        switch (current_screen) {
            case screen_LH_splash:
                {
                    current_screen = screen_scrambled_splash(application_event_queue, keyboard_event_queue, &textID);
                    break;
                }
            case screen_LH_playing:
                {
                    current_screen = screen_scrambled_playing(application_event_queue, keyboard_event_queue, &textID);
                    break;
                }
            case screen_LH_victory:
                {
                    current_screen = screen_scrambled_victory(application_event_queue, keyboard_event_queue);
                    break;
                }
            default: return screen_home;
        }
    }
}