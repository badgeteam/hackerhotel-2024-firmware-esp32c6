#pragma once

#include "badge_messages.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define BADGE_COMMS_MAX_MESSAGE_SIZE (128)

esp_err_t init_badge_comms(void);

typedef struct {
    badge_comms_message_type_t message_type;
    uint8_t                    from_mac[8];
    uint8_t                    data[BADGE_COMMS_MAX_MESSAGE_SIZE];
    uint8_t                    data_len_to_send;
} badge_comms_message_t;

void badge_comms_send_message(badge_comms_message_t* comms_message);

QueueHandle_t badge_comms_add_listener(badge_comms_message_type_t message_type, uint8_t expected_message_len);
esp_err_t     badge_comms_remove_listener(QueueHandle_t queue);
