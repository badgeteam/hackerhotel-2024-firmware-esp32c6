#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

screen_t screen_shades_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
