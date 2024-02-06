#include "screen_home.h"
#include "application.h"
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
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

extern uint8_t const homescreen_png_start[] asm("_binary_homescreen_png_start");
extern uint8_t const homescreen_png_end[] asm("_binary_homescreen_png_end");

static char const * TAG = "homescreen";

char const screen_name[10][30] = {
    "Lighthouse",
    "Engine room",
    "Billboard",
    "Name tag",
    "Library",

    "Deibler",
    "Repertoire",
    "template",
    "test",
    "Carondelet",
};

screen_t screen_home_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {

    // update the keyboard event handler settings
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    // event_t kbsettings = {
    //     .type                                     = event_control_keyboard,
    //     .args_control_keyboard.enable_typing      = false,
    //     .args_control_keyboard.enable_actions     = {true, true, true, true, true},
    //     .args_control_keyboard.enable_leds        = true,
    //     .args_control_keyboard.enable_relay       = true,
    //     kbsettings.args_control_keyboard.capslock = false,
    // };
    // xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    // set screen font and buffer
    pax_font_t const * font = pax_font_marker;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);

    // memory
    nvs_handle_t handle;
    esp_err_t    res = nvs_open("owner", NVS_READWRITE, &handle);

    // read nickname from memory
    char   nickname[64] = {0};
    size_t size         = 0;
    res                 = nvs_get_str(handle, "nickname", NULL, &size);
    if ((res == ESP_OK) && (size <= sizeof(nickname) - 1)) {
        res = nvs_get_str(handle, "nickname", nickname, &size);
        if (res != ESP_OK || strlen(nickname) < 1) {
            sprintf(nickname, "No nickname configured");
        }
    }

    nvs_close(handle);

    // scale the nickame to fit the screen and display(I think)
    pax_vec1_t dims = {
        .x = 999,
        .y = 999,
    };
    float scale = 100;
    while (scale > 1) {
        dims = pax_text_size(font, scale, nickname);
        if (dims.x <= 296 && dims.y <= 100)
            break;
        scale -= 0.2;
    }
    ESP_LOGW(TAG, "Scale: %f", scale);
    pax_draw_text(gfx, RED, font, scale, (296 - dims.x) / 2, (100 - dims.y) / 2, nickname);
    pax_insert_png_buf(gfx, homescreen_png_start, homescreen_png_end - homescreen_png_start, 0, gfx->width - 24, 0);
    bsp_display_flush();
    uint8_t cursor = 0;

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {

                        case SWITCH_1: return screen_settings; break;
                        case SWITCH_2: return screen_billboard; break;
                        case SWITCH_3: return screen_battleship; break;
                        case SWITCH_4: return screen_repertoire; break;
                        case SWITCH_5: return screen_template; break;
                        default: break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

// screen_t screen_test_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {

//     // update the keyboard event handler settings
//     event_t kbsettings = {
//         .type                                     = event_control_keyboard,
//         .args_control_keyboard.enable_typing      = false,
//         .args_control_keyboard.enable_actions     = {true, false, false, true, true},
//         .args_control_keyboard.enable_leds        = true,
//         .args_control_keyboard.enable_relay       = true,
//         kbsettings.args_control_keyboard.capslock = false,
//     };
//     xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

//     // set screen font and buffer
//     pax_font_t const * font = pax_font_marker;
//     pax_buf_t*         gfx  = bsp_get_gfx_buffer();
//     pax_background(gfx, WHITE);

//     // // memory
//     // nvs_handle_t handle;
//     // esp_err_t    res = nvs_open("owner", NVS_READWRITE, &handle);

//     // // read nickname from memory
//     // char   nickname[64] = {0};
//     // size_t size         = 0;
//     // res                 = nvs_get_str(handle, "nickname", NULL, &size);
//     // if ((res == ESP_OK) && (size <= sizeof(nickname) - 1)) {
//     //     res = nvs_get_str(handle, "nickname", nickname, &size);
//     //     if (res != ESP_OK || strlen(nickname) < 1) {
//     //         sprintf(nickname, "No nickname configured");
//     //     }
//     // }

//     // nvs_close(handle);

//     // // scale the nickame to fit the screen and display(I think)
//     // pax_vec1_t dims = {
//     //     .x = 999,
//     //     .y = 999,
//     // };
//     // float scale = 100;
//     // while (scale > 1) {
//     //     dims = pax_text_size(font, scale, nickname);
//     //     if (dims.x <= 296 && dims.y <= 100)
//     //         break;
//     //     scale -= 0.2;
//     // }
//     // ESP_LOGW(TAG, "Scale: %f", scale);
//     pax_draw_text(gfx, BLACK, font, 30, 50, 40, "The cake is a lie");
//     // pax_insert_png_buf(gfx, homescreen_png_start, homescreen_png_end - homescreen_png_start, 0, gfx->width - 24,
//     0); bsp_display_flush();

//     while (1) {
//         event_t event = {0};
//         if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
//             switch (event.type) {
//                 case event_input_button: break;  // Ignore raw button input
//                 case event_input_keyboard:
//                     switch (event.args_input_keyboard.action) {
//                         case SWITCH_1: return screen_home; break;
//                         case SWITCH_2: break;
//                         case SWITCH_3: break;
//                         case SWITCH_4: break;
//                         case SWITCH_5: break;
//                         default: break;
//                     }
//                     break;
//                 default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
//             }
//         }
//     }
// }