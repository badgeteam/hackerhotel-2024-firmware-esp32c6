#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

#define screen_PC_n 0
#define screen_PC_e 1
#define screen_PC_s 2
#define screen_PC_w 3

#define screen_PC_n2 0
#define screen_PC_s2 1

#define screen_PC_road1_n 0
#define screen_PC_road1_e 1
#define screen_PC_road1_w 2

#define screen_PC_dune3_s    0
#define screen_PC_dune3_gate 1
#define screen_PC_dune3_up   2

#define nb_state 10

#define state_cursor               0
#define state_tutorial_dock        1
#define state_tutorial_dune2       2
#define state_tutorial_town1_north 3
#define state_tutorial_town1_east  4
#define state_tutorial_town1_south 5
#define state_tutorial_town1_west  6
#define state_tutorial_dune3       7
#define state_locklighthouse       8

#define tutorial_uncomplete 0
#define tutorial_complete   1

#define locklighthouse_locked               0
#define locklighthouse_lighthouse_seen_sign 1
#define locklighthouse_meet_lady            2
#define locklighthouse_key_obtained         3
#define locklighthouse_unlocked             4

screen_t screen_pointclick_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
