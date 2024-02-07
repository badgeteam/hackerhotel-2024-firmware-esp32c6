#include "screen_home.h"
#include "application.h"
#include "bsp.h"
#include "coprocessor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
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

extern uint8_t const diamondl_png_start[] asm("_binary_diamondl_png_start");
extern uint8_t const diamondl_png_end[] asm("_binary_diamondl_png_end");
extern uint8_t const diamondr_png_start[] asm("_binary_diamondr_png_start");
extern uint8_t const diamondr_png_end[] asm("_binary_diamondr_png_end");
extern uint8_t const diamondt_png_start[] asm("_binary_diamondt_png_start");
extern uint8_t const diamondt_png_end[] asm("_binary_diamondt_png_end");
extern uint8_t const diamondb_png_start[] asm("_binary_diamondb_png_start");
extern uint8_t const diamondb_png_end[] asm("_binary_diamondb_png_end");

static char const * TAG = "homescreen";

// THIS LIST NEED TO MATCH SCREEN.H
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

uint8_t const screen_pos[10][2] = {
    {48, 100},
    {200, 36},
    {50, 50},
    {50, 50},
    {50, 50},

    {50, 50},
    {50, 50},
    {50, 50},
    {50, 50},
    {50, 50},
};


void DisplayHomeEntry(int cursor);

screen_t screen_home_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    ESP_LOGE(TAG, "Enter screen_home_entry");
    // update the keyboard event handler settings
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, false, false, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);

    int cursor = screen_battleship;
    DisplayHomeEntry(cursor);
    int timer_track = (esp_timer_get_time() / (Home_screen_timeout * 1000000)) + 1;

    while (1) {
        event_t event = {0};
        // when timeout
        if ((esp_timer_get_time() / 1000000) > (timer_track * Home_screen_timeout)) {
            return screen_nametag;
        }
        ESP_LOGE(TAG, "test screen_home_entry");
        if (xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor = Decrement(cursor, Nb_screen); break;
                        case SWITCH_R1: cursor = Increment(cursor, Nb_screen); break;
                        case SWITCH_1: break;
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5: return cursor; break;
                        default: break;
                    }
                    ESP_LOGE(TAG, "Cursor %d", cursor);
                    timer_track = (esp_timer_get_time() / (Home_screen_timeout * 1000000)) + 1;
                    DisplayHomeEntry(cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

void DisplayHomeEntry(int cursor) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    AddSWtoBufferLR("", "Select");
    DrawArrowHorizontal(SWITCH_1);
    pax_center_text(gfx, BLACK, font1, fontsizeS * 2, gfx->height / 2, 100, screen_name[cursor]);
    pax_insert_png_buf(
        gfx,
        diamondb_png_start,
        diamondb_png_end - diamondb_png_start,
        screen_pos[cursor][0],
        screen_pos[cursor][1],
        0
    );
    bsp_display_flush();
}

screen_t screen_Nametag(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    ESP_LOGE(TAG, "Enter screen_Nametag");
    // set screen font and buffer
    pax_buf_t* gfx = bsp_get_gfx_buffer();
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
        dims = pax_text_size(font1, scale, nickname);
        if (dims.x <= 296 && dims.y <= 100)
            break;
        scale -= 0.2;
    }
    ESP_LOGW(TAG, "Scale: %f", scale);
    pax_draw_text(gfx, RED, font1, scale, (296 - dims.x) / 2, (100 - dims.y) / 2, nickname);
    bsp_display_flush();

    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);

    while (1) {
        event_t event = {0};
        ESP_LOGE(TAG, "test screen_home_entry");
        if (xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        default: return screen_home; break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
