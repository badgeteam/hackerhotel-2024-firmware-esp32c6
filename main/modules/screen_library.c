#include "screen_library.h"
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
#include "screen_library.h"
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

extern uint8_t const library_png_start[] asm("_binary_library_png_start");
extern uint8_t const library_png_end[] asm("_binary_library_png_end");

static char const * TAG = "library";

char const library_items_name[10][30] = {
    "Entry 1",
    "Entry 2",
    "Entry 3",
    "Name tag",
    "Library",

    "Deibler",
    "Repertoire",
    "template",
    "test",
    "Carondelet",
};

uint8_t const library_pos[10][2] = {
    {74, 38},
    {86, 34},
    {98, 37},
    {50, 50},
    {50, 50},

    {50, 50},
    {50, 50},
    {50, 50},
    {50, 50},
    {50, 50},
};

void DisplayLibrarytest(int cursor) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    Addborder2toBuffer();
    pax_center_text(gfx, BLACK, font1, fontsizeS * 2, gfx->height / 2, 64, "Get stickbugged lol");
    bsp_display_flush();
    vTaskDelay(pdMS_TO_TICKS(10000));
}

void DisplayLibraryEntry(int cursor);

screen_t screen_library_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    ESP_LOGE(TAG, "Enter screen_home_entry");
    bsp_apply_lut(lut_4s);
    // update the keyboard event handler settings
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, false, false, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_3, true);

    int cursor = 0;
    DisplayLibraryEntry(cursor);

    while (1) {
        event_t event = {0};
        ESP_LOGE(TAG, "test screen_home_entry");
        if (xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return 0; break;
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_L3: cursor = Decrement(cursor, Nb_item_library); break;
                        case SWITCH_R3: cursor = Increment(cursor, Nb_item_library); break;
                        case SWITCH_4: break;
                        case SWITCH_5: DisplayLibrarytest(cursor); break;
                        default: break;
                    }
                    ESP_LOGE(TAG, "Cursor %d", cursor);
                    DisplayLibraryEntry(cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

void DisplayLibraryEntry(int cursor) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_insert_png_buf(gfx, library_png_start, library_png_end - library_png_start, 0, 0, 0);
    AddOneTextSWtoBuffer(SWITCH_1, "Exit");
    DrawArrowHorizontal(SWITCH_3);
    AddOneTextSWtoBuffer(SWITCH_5, "Select");
    pax_center_text(gfx, BLACK, font1, fontsizeS * 2, gfx->height / 2, 87, library_items_name[cursor]);
    pax_insert_png_buf(
        gfx,
        diamondb_png_start,
        diamondb_png_end - diamondb_png_start,
        library_pos[cursor][0] - 4,
        library_pos[cursor][1] - 8,
        0
    );
    bsp_display_flush();
}
