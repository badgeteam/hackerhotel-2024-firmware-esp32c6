#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

#define Nb_screen           9   // until lighthouse is fixed
#define Home_screen_timeout 30  // in seconds before the screen change to the nametag

screen_t screen_home_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
screen_t screen_Nametag(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
