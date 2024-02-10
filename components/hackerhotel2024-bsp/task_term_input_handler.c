#include "task_term_input_handler.h"
#include "bsp.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include <stdint.h>
#include <string.h>

void task_term_input_handler(void* pvParameters) {
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    char          prev  = 0;
    char          c     = 0;
    while (1) {
        // Try to get a character.
        prev    = c;
        int len = usb_serial_jtag_read_bytes((void*)&c, 1, portMAX_DELAY);
        if (len != 1) {
            vTaskDelay(1);
            continue;
        }

        // Ignore unprintable characters.
        if (c <= 0 || c > 0x7f) {
            continue;
        }
        // De-duplicate newlines.
        if (c == '\r') {
            event_t event = {
                .type            = event_term_input,
                .args_term_input = '\n',
            };
            xQueueSend(queue, &event, 1);
        } else if (c == '\n' && prev == '\r') {
            continue;
        } else {
            event_t event = {
                .type            = event_term_input,
                .args_term_input = c,
            };
            xQueueSend(queue, &event, 1);
        }
    }
}

esp_err_t start_task_term_input_handler(QueueHandle_t queue) {
    usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    esp_err_t                       ec  = usb_serial_jtag_driver_install(&cfg);
    if (ec != ESP_OK)
        return ESP_FAIL;
    BaseType_t res = xTaskCreate(task_term_input_handler, "terminal input handler", 4096, (void*)queue, 1, NULL);
    return (res == pdPASS) ? ESP_OK : ESP_FAIL;
}
