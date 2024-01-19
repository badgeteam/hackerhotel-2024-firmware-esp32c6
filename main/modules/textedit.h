#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

bool textedit(
    char const *  title,
    QueueHandle_t application_event_queue,
    QueueHandle_t keyboard_event_queue,
    char*         output,
    size_t        output_size
);