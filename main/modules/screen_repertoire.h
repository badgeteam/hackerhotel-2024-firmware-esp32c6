#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

#define maxperpage        8
#define maxIDrepertoire   500
#define maxIDsurrounding  5
#define remove            0
#define add               1
#define BroadcastInterval 5

struct cursor_t {
    int x;
    int y;
    int yabs;
};

screen_t screen_repertoire_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queuem, int _app);
