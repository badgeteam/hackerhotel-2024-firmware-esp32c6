// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 Hugo Trippaers

#include "badge-communication.h"
#include "badge-communication-ieee802154.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_ieee802154.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "events.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"
#include "memory.h"

static const char* TAG = "communication";

static bool initialized = false;

static QueueHandle_t queue_raw_message_rx = NULL;

// Private functions

void print_addr(ieee802154_address_t* addr) {
    switch (addr->mode) {
        case ADDR_MODE_NONE: printf("NONE  ---\r\n"); break;
        case ADDR_MODE_SHORT: printf("SHORT %04x\r\n", addr->short_address); break;
        case ADDR_MODE_LONG:
            printf(
                "LONG  %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                addr->long_address[0],
                addr->long_address[1],
                addr->long_address[2],
                addr->long_address[3],
                addr->long_address[4],
                addr->long_address[5],
                addr->long_address[6],
                addr->long_address[7]
            );
            break;
    }
}

void badge_communication_rx_task(void* param) {
    QueueHandle_t* output_queues = (QueueHandle_t*)param;

    ieee802154_message_t raw_message = {0};

    while (true) {
        if (xQueueReceive(queue_raw_message_rx, &raw_message, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Received badge communication packet");
            uint8_t* raw_packet        = &raw_message.frame[1];
            uint8_t  raw_packet_length = raw_message.frame[0];

            if (raw_packet_length >= IEEE802154_FRAME_SIZE) {
                ESP_LOGE(TAG, "Frame too large (%u), ignored", raw_packet_length);
                continue;
            }

            /*printf("< ");
            for (uint8_t i = 0; i < raw_packet_length; i++) {
                printf("%02x", raw_packet[i]);
            }
            printf("\r\n");*/

            mac_fcs_t            fcs;
            uint16_t             src_pan, dst_pan;
            ieee802154_address_t src, dst;
            bool                 ack;
            uint8_t header_length = ieee802154_header_parse(raw_packet, &fcs, &src_pan, &src, &dst_pan, &dst, &ack);
            /*printf("dst: %04x ", dst_pan);
            print_addr(&dst);
            printf("src: %04x ", src_pan);
            print_addr(&src);
            printf("ack: %u\r\n", ack);*/

            if (fcs.frameType != FRAME_TYPE_DATA) {
                continue;
            }

            badge_communication_packet_t* packet = (badge_communication_packet_t*)&raw_packet[header_length];

            if (packet->magic != BADGE_COMMUNICATION_MAGIC) {
                ESP_LOGE(TAG, "Invalid magic (%02x)", packet->magic);
                continue;
            }

            if (packet->version != BADGE_COMMUNICATION_VERSION) {
                ESP_LOGE(TAG, "Invalid version (%02x)", packet->version);
                continue;
            }

            if (packet->repeat > 0) {
                packet->repeat -= 1;
                ESP_LOGW(TAG, "Repeating packet, repeat counter is %u", packet->repeat);
                esp_ieee802154_transmit(raw_message.frame, false);
            }

            uint8_t data_length = raw_packet_length - header_length - sizeof(badge_communication_packet_t);

            event_t event                 = {.type = event_communication};
            event.args_communication.type = packet->type;
            memcpy(&event.args_communication.src, &src, sizeof(ieee802154_address_t));
            memcpy(&event.args_communication.dst, &dst, sizeof(ieee802154_address_t));
            memcpy(event.args_communication.data, packet->content, data_length);

            uint32_t index = 0;
            while (output_queues[index] != NULL) {
                xQueueSend(output_queues[index], &event, 0);
                index++;
            }
        }
    }
}

// Public functions

esp_err_t badge_communication_init(QueueHandle_t* output_queues) {
    if (initialized) {
        ESP_LOGI(TAG, "Badge communication is already initialized");
        return ESP_OK;
    }

    queue_raw_message_rx = xQueueCreate(BADGE_COMMS_RX_QUEUE_DEPTH, sizeof(badge_comms_message_t));
    if (queue_raw_message_rx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate queue");
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(esp_ieee802154_enable(), TAG, "Failed to enable IEEE802154");
    ESP_RETURN_ON_ERROR(esp_ieee802154_set_promiscuous(false), TAG, "Failed to set radio mode");
    ESP_RETURN_ON_ERROR(esp_ieee802154_set_rx_when_idle(true), TAG, "Failed to set receive when idle");
    ESP_RETURN_ON_ERROR(esp_ieee802154_set_panid(BADGE_COMMUNICATION_PAN), TAG, "Failed to set PAN identifier");
    ESP_RETURN_ON_ERROR(esp_ieee802154_set_channel(BADGE_COMMUNICATION_CHANNEL), TAG, "Failed ot set channel");

    // ESP_RETURN_ON_ERROR(esp_ieee802154_set_panid(0x1234), TAG, "Failed to set PAN identifier");
    // ESP_RETURN_ON_ERROR(esp_ieee802154_set_channel(11), TAG, "Failed ot set channel");

    uint8_t mac[8] = {0};
    ESP_RETURN_ON_ERROR(esp_read_mac(mac, ESP_MAC_IEEE802154), TAG, "Failed to read mac address");
    ESP_LOGI(
        TAG,
        "MAC address: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5],
        mac[6],
        mac[7]
    );
    ESP_RETURN_ON_ERROR(esp_ieee802154_set_extended_address(mac), TAG, "Failed to set mac address");

    uint16_t short_address = (mac[6] << 8) | mac[7];
    ESP_RETURN_ON_ERROR(esp_ieee802154_set_short_address(short_address), TAG, "Failed to set short address");

    ESP_LOGI(TAG, "Short address: %04x", short_address);

    if (xTaskCreate(badge_communication_rx_task, "badge-communication", 2048, (void*)output_queues, 8, NULL) !=
        pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        return ESP_FAIL;
    }

    initialized = true;

    return ESP_OK;
}

esp_err_t badge_communication_start() {
    return esp_ieee802154_receive();
}

esp_err_t badge_communication_stop() {
    return esp_ieee802154_sleep();
}

esp_err_t badge_communication_send(
    badge_comms_message_type_t type, uint8_t* content, uint8_t content_length, ieee802154_address_t* arg_dst
) {
    uint16_t pan_id = BADGE_COMMUNICATION_PAN;

    if (content_length > BADGE_COMMS_USER_DATA_MAX_LEN) {
        ESP_LOGE(TAG, "Message too long");
        return ESP_FAIL;
    }

    ieee802154_address_t src = {.mode = ADDR_MODE_LONG};
    esp_ieee802154_get_extended_address(src.long_address);

    ieee802154_address_t dst = {.mode = ADDR_MODE_SHORT, .short_address = 0xFFFF};  // Broadcast

    if (arg_dst != NULL) {
        memcpy(&dst, arg_dst, sizeof(ieee802154_address_t));
    }

    uint8_t raw_packet[256] = {0};
    uint8_t raw_packet_length =
        ieee802154_header(&pan_id, &src, &pan_id, &dst, false, &raw_packet[1], sizeof(raw_packet) - 1);

    badge_communication_packet_t* packet = (badge_communication_packet_t*)&raw_packet[1 + raw_packet_length];
    packet->magic                        = BADGE_COMMUNICATION_MAGIC;
    packet->version                      = BADGE_COMMUNICATION_VERSION;
    packet->repeat                       = 0;
    packet->flags                        = 0;
    packet->type                         = (uint8_t)type;
    memcpy(packet->content, content, content_length);

    raw_packet_length += content_length;  // set userdata buffersize, and one for the message type
    raw_packet_length += 2;               // save space for preceding protocol bytes

    raw_packet[0] = raw_packet_length;  // packet length

    /*printf("> ");
    for (uint8_t i = 0; i < raw_packet[0]; i++) {
        printf("%02x", raw_packet[i + 1]);
    }
    printf("\r\n");*/

    return esp_ieee802154_transmit(raw_packet, false);
}

esp_err_t badge_communication_send_time(badge_message_time_t* data) {
    return badge_communication_send(MESSAGE_TYPE_TIME, (uint8_t*)data, sizeof(badge_message_time_t), NULL);
}

esp_err_t badge_communication_send_chat(badge_message_chat_t* data) {
    return badge_communication_send(MESSAGE_TYPE_CHAT, (uint8_t*)data, sizeof(badge_message_chat_t), NULL);
}

esp_err_t badge_communication_send_repertoire(badge_message_repertoire_t* data) {
    return badge_communication_send(MESSAGE_TYPE_REPERTOIRE, (uint8_t*)data, sizeof(badge_message_repertoire_t), NULL);
}

esp_err_t badge_communication_send_battleship(badge_message_battleship_t* data, ieee802154_address_t* dst) {
    return badge_communication_send(MESSAGE_TYPE_BATTLESHIP, (uint8_t*)data, sizeof(badge_message_battleship_t), dst);
}

// ESP-IDF callback functions

void esp_ieee802154_receive_done(uint8_t* frame, esp_ieee802154_frame_info_t* frame_info) {
    ieee802154_message_t message;
    memcpy(&message.frame_info, frame_info, sizeof(esp_ieee802154_frame_info_t));
    memcpy(message.frame, frame, IEEE802154_FRAME_SIZE);
    xQueueSendToBackFromISR(queue_raw_message_rx, &message, NULL);
}
