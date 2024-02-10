#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct _task_keyboard_parameters {
    QueueHandle_t input_queue;
    QueueHandle_t output_queue;
} task_keyboard_parameters_t;

#define ROTATION_L1 5
#define ROTATION_L2 6
#define ROTATION_L3 7
#define ROTATION_L4 8
#define ROTATION_L5 9

#define ROTATION_R1 10
#define ROTATION_R2 11
#define ROTATION_R3 12
#define ROTATION_R4 13
#define ROTATION_R5 14

#define NUM_ROTATION   10
#define NUM_CHARACTERS 37
// first 26 are letters,
// next 10 are numbers 0-9
// last character is space

esp_err_t start_task_keyboard(QueueHandle_t input_queue, QueueHandle_t output_queue);
