#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screens.h"

screen_t screen_credits_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);

typedef enum _credits {
    badgeteam_mascot,
    hh2024_team,
    production_sponsor,
    thankyou,
    showsover,
} credits_t;