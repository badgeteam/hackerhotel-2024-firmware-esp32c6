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

#define BS_A_CK            0
#define BS_invite          1
#define BS_starter         2
#define BS_shipboard_start 3
#define BS_shipboard_end   8
#define BS_shotlocation    9
#define BS_shottypereport  10
#define BS_abort           11

#define invitation_sent     1
#define invitation_accepted 2
#define invitation_declined 3

#define BS_default 255

screen_t screen_battleship_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
