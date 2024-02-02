#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

#define maxIDrepertoire   5
#define remove            0
#define add               1
#define BroadcastInterval 5000

screen_t screen_repertoire_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
