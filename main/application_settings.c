#include <inttypes.h>
#include <stdio.h>

#include "application_settings.h"
#include "application.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "bsp.h"

void menu_settings() {
    pax_buf_t *gfx  = bsp_get_gfx_buffer();
    QueueHandle_t queue = bsp_get_button_queue();

    bsp_apply_lut(lut_4s);
    pax_background(gfx, 0);
    pax_draw_text(gfx, 1, pax_font_marker, 18, 1, 0, "Settings menu");
    DisplaySwitchesBox(SWITCH_1);
    pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
    bsp_display_flush();

    bsp_flush_button_queue();
    while (1) {
        coprocessor_input_message_t buttonMessage = {0};
        if (xQueueReceive(queue, &buttonMessage, portMAX_DELAY) == pdTRUE) {
            printf("In settings: button %u state changed to %u\n", buttonMessage.button, buttonMessage.state);
            if (buttonMessage.state == SWITCH_PRESS) {
                if (buttonMessage.button == SWITCH_1) {
                    printf("Exiting settings!\n");
                    break; // Exit
                }
            }
        }
    }
    bsp_flush_button_queue();
    printf("Return to main!\n");
}