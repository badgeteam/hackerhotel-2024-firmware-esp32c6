#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

#define screen_PC_n 0
#define screen_PC_e 1
#define screen_PC_s 2
#define screen_PC_w 3

screen_t screen_pointclick_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
