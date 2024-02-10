#include "screen_mascots.h"
#include "application.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

extern const uint8_t mascots_png_start[] asm("_binary_mascots_png_start");
extern const uint8_t mascots_png_end[] asm("_binary_mascots_png_end");

static const char* TAG = "mascots";

screen_t screen_mascots_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    if (log)
        ESP_LOGE(TAG, "Enter screen_mascots_entry");
    reset_keyboard_settings(keyboard_event_queue);

    //    pax_font_t const * font = pax_font_saira_regular;
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_insert_png_buf(gfx, mascots_png_start, mascots_png_end - mascots_png_start, 0, 0, 0);
    bsp_display_flush();

    vTaskDelay(pdMS_TO_TICKS(splash_duration));

    return screen_nametag;
}
