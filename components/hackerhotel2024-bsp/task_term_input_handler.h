#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

esp_err_t start_task_term_input_handler(QueueHandle_t queue);
