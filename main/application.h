#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void app_thread_entry(QueueHandle_t event_queue);
void DisplaySwitchesBox(int _switch);
