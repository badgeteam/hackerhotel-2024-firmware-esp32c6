#include "application.h"
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
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screen_home.h"
#include "screen_pointclick.h"
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_random.h>
#include <string.h>

static char const * TAG = "repertoire";

void Display_repertoire(void) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    AddSWtoBuffer("Exit", "application", "dog", "bananana", "kami");
    bsp_display_flush();
}

screen_t screen_repertoire_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {

    configure_keyboard_guru(keyboard_event_queue, true, true, true, false, true);

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_L2:
                            Display_repertoire();
                            ESP_LOGE(TAG, "L2");
                            break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        case SWITCH_L5: ESP_LOGE(TAG, "L5"); break;
                        case SWITCH_R5: ESP_LOGE(TAG, "R5"); break;
                        case SWITCH_L1: ESP_LOGE(TAG, "L1"); break;
                        case SWITCH_R1: ESP_LOGE(TAG, "R1"); break;
                        default: break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
