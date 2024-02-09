#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

#define timer_battery_screen 60  // in seconds

screen_t screen_settings_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
screen_t screen_settings_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
