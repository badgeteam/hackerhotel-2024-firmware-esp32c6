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
    uint8_t                    data[BADGE_COMMS_USER_DATA_MAX_LEN];
    uint8_t                    data_len_to_send;
} badge_comms_message_t;

/**
 * broadcast a message to other badges around
 * @param comms_message the message to be sent
 */
void badge_comms_send_message(badge_comms_message_t* comms_message);

/**
 * 'subscribe' to a message type
 * @param message_type the message type to subscribe to
 * @param expected_message_len the sizeof the datastruct the receiver is expecting
 * @return a queue handle, where received messages are sent to.
 * return NULL if something went wrong, check logs for more info
 */
QueueHandle_t badge_comms_add_listener(badge_comms_message_type_t message_type, uint8_t expected_message_len);

/**
 * 'unsubscribe' from a message type
 * @param queue the queue handle received when calling badge_comms_add_listener
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG when no queue was provided
 *      - ESP_ERR_INVALID_STATE when the badge comms component has not been initialised
 *      - ESP_ERR_NOT_FOUND if the provided queue was not found in the list of listeners
 */
esp_err_t badge_comms_remove_listener(QueueHandle_t queue);
