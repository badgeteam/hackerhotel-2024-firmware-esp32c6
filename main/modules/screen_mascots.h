#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

#define splash_duration 2000  // splash duration in MS

screen_t screen_mascots_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
