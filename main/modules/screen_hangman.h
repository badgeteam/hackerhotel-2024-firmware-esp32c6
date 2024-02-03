#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

#define longestword     10
#define nbwords         10
#define mistakesallowed 9

#define nbletters 26

screen_t screen_hangman_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);