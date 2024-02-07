#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

#define Nb_item_library 10
// #define Home_screen_timeout 5  // in seconds before the screen change to the nametag

screen_t screen_library_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
