#include "task_button_input_handler.h"
#include "bsp.h"
#include "esp_log.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include <stdint.h>
#include <string.h>

void task_button_input_handler(void* pvParameters) {
    QueueHandle_t  input_queue   = bsp_get_button_queue();
    QueueHandle_t* output_queues = (QueueHandle_t*)pvParameters;

    while (1) {
        coprocessor_input_message_t buttonMessage = {0};
        if (xQueueReceive(input_queue, &buttonMessage, portMAX_DELAY) == pdTRUE) {
            event_t event = {
                .type = event_input_button,
            };
            memcpy(&event.args_input_button, &buttonMessage, sizeof(coprocessor_input_message_t));
            uint32_t index = 0;
            while (output_queues[index] != NULL) {
                xQueueSend(output_queues[index], &event, 0);
                index++;
            }
        }
    }
}

esp_err_t start_task_button_input_handler(QueueHandle_t* queues) {
    BaseType_t res = xTaskCreate(task_button_input_handler, "button input handler", 1024, (void*)queues, 1, NULL);
    return (res == pdPASS) ? ESP_OK : ESP_FAIL;
}
