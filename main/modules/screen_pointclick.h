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

#define screen_PC_n2 0
#define screen_PC_s2 1

#define screen_PC_road1_n 0
#define screen_PC_road1_e 1
#define screen_PC_road1_w 2

#define screen_PC_dune3_s    0
#define screen_PC_dune3_gate 1
#define screen_PC_dune3_up   2

#define nb_state 10

#define state_cursor         0
#define state_locklighthouse 1

#define locklighthouse_locked   0
#define locklighthouse_unlocked 1

screen_t screen_pointclick_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
