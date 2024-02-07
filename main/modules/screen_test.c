#include "screen_test.h"
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
#include <esp_random.h>
#include <string.h>

extern uint8_t const mascots_png_start[] asm("_binary_mascots_png_start");
extern uint8_t const mascots_png_end[] asm("_binary_mascots_png_end");

static char const * TAG = "test";


char const team[11][32] = {
    "Nikolett", "Guru-san", "Renze", "Tom Clement", "CH23", "Norbert", "Zac", "SqyD", "Martijn", "Julian", "Dimitri"
};

void Display_credits_entry(int cursor) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    switch (cursor) {
        case badgeteam_mascot:
            pax_insert_png_buf(gfx, mascots_png_start, mascots_png_end - mascots_png_start, 0, 0, 0);
            break;
        case hh2024_team:
            Addborder2toBuffer();
            pax_center_text(gfx, BLACK, font1, fontsizeS * 1.5, gfx->height / 2, 10, "Brought to you by");
            for (int i = 0; i < 11; i++)
                pax_center_text(
                    gfx,
                    BLACK,
                    font1,
                    fontsizeS * 1.5,
                    gfx->height / 4 * (1 + ((i % 2) * 2)),  // X
                    fontsizeS * 1.5 * (i / 2) + 30,         // Y
                    team[i]
                );
            break;
        case production_sponsor:
            Addborder2toBuffer();
            WallofText(20, "espressif LOGO & allnet", 1, 10);
            break;
        case thankyou:
            Addborder2toBuffer();
            WallofText(
                20,
                "Thank you to the Hacker Hotel team for making this event happen, and thank you for attending and "
                "playing~ \n\n\n\n With love, badge.team",
                1,
                10
            );
            break;

        default: break;
    }
    bsp_display_flush();
}

screen_t screen_credits_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    int cursor = 0;
    Display_credits_entry(cursor);
    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        // case SWITCH_1: return screen_home; break;
                        // case SWITCH_2: break;
                        // case SWITCH_3: break;
                        // case SWITCH_4: break;
                        // case SWITCH_5: break;
                        default:
                            cursor++;
                            if (cursor == showsover)
                                return screen_home;
                            Display_credits_entry(cursor);
                            break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
