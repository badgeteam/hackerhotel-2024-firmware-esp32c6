// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 Hugo Trippaers

#pragma once

#include "badge-communication-protocol.h"
#include "esp_err.h"
#include "esp_ieee802154.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define IEEE802154_FRAME_SIZE      (128)  // 1 byte for the length + 127 bytes of data
#define BADGE_COMMS_RX_QUEUE_DEPTH (5)

esp_err_t badge_communication_init(QueueHandle_t* output_queues);

typedef struct _ieee802154_message {
    esp_ieee802154_frame_info_t frame_info;
    uint8_t                     frame[IEEE802154_FRAME_SIZE];
} ieee802154_message_t;

typedef struct {
    badge_comms_message_type_t message_type;
    uint8_t                    from_mac[8];
    uint8_t                    data[BADGE_COMMS_USER_DATA_MAX_LEN];
    uint8_t                    data_len_to_send;
} badge_comms_message_t;

/**
 * broadcast a message to other badges around
 * @param comms_message the message to be sent
 */
esp_err_t badge_communication_send(badge_comms_message_type_t type, uint8_t* content, uint8_t content_length);

esp_err_t badge_communication_send_time(badge_message_time_t* data);
esp_err_t badge_communication_send_chat(badge_message_chat_t* data);
esp_err_t badge_communication_send_repertoire(badge_message_chat_t* data);
esp_err_t badge_communication_send_battleship(badge_message_chat_t* data);

esp_err_t badge_communication_start();
esp_err_t badge_communication_stop();
