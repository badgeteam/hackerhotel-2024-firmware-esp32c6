#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct _task_keyboard_parameters {
    QueueHandle_t input_queue;
    QueueHandle_t output_queue;
} task_keyboard_parameters_t;

esp_err_t start_task_keyboard(QueueHandle_t input_queue, QueueHandle_t output_queue);
