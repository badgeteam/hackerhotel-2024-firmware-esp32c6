#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

#define no_ship         0
#define small_ship      1
#define medium_front    2
#define medium_whole    3
#define long_front      4
#define long_whole      5
#define all_ship_placed 6

screen_t screen_battleship_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
