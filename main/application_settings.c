#include <inttypes.h>
#include <stdio.h>

#include "application_settings.h"
#include "application.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "bsp.h"
#include "wifi_ota.h"

void menu_settings() {
    pax_buf_t *gfx  = bsp_get_gfx_buffer();
    QueueHandle_t queue = bsp_get_button_queue();

    bool exit = false;
    while (!exit) {
        bsp_apply_lut(lut_4s);
        pax_background(gfx, 0);
        pax_draw_text(gfx, 1, pax_font_marker, 18, 0, 0, "Settings menu");
        pax_draw_text(gfx, 1, pax_font_marker, 18, 0, 20, "OTA update?");
        DisplaySwitchesBox(SWITCH_1);
        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "EXIT");
        DisplaySwitchesBox(SWITCH_4);
        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 187, 116, "DEV");
        DisplaySwitchesBox(SWITCH_5);
        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "STABLE");
        bsp_display_flush();

        bsp_flush_button_queue();
        while (1) {
            coprocessor_input_message_t buttonMessage = {0};
            if (xQueueReceive(queue, &buttonMessage, portMAX_DELAY) == pdTRUE) {
                printf("In settings: button %u state changed to %u\n", buttonMessage.button, buttonMessage.state);
                if (buttonMessage.state == SWITCH_PRESS) {
                    if (buttonMessage.button == SWITCH_1) {
                        printf("Exiting settings!\n");
                        exit = true;
                        break; // Exit button loop
                    } else if (buttonMessage.button == SWITCH_4) {
                        ota_update(true);
                        break; // Exit button loop
                    } else if (buttonMessage.button == SWITCH_5) {
                        ota_update(false);
                        break; // Exit button loop
                    }
                }
            }
        }
    }

    bsp_flush_button_queue();
    printf("Return to main!\n");
}