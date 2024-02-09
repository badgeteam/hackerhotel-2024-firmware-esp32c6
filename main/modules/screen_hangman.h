#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

#define longestword 16

#define english   0
#define victorian 1

// #define nbwords         10
#define mistakesallowed 10

#define nbletters 26

screen_t screen_hangman_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);