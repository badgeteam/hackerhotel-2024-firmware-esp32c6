#include "badge_comms.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "memory.h"
#include "time.h"

static char const * TAG = "bodge comms example";

void receive_timestamp(void) {
    // get a queue to listen on, for message type MESSAGE_TYPE_TIMESTAMP, and size badge_message_timestamp_t
    QueueHandle_t timestamp_queue = badge_comms_add_listener(MESSAGE_TYPE_TIMESTAMP, sizeof(badge_message_timestamp_t));
    // check if an error occurred (check logs for the reason)
    if (timestamp_queue == NULL) {
        ESP_LOGE(TAG, "Failed to add listener");
        return;
    }

    uint32_t i = 0;

    while (true) {
        // variable for the queue to store the message in
        badge_comms_message_t message;
        xQueueReceive(timestamp_queue, &message, portMAX_DELAY);

        // typecast the message data to the expected message type
        badge_message_timestamp_t* ts = (badge_message_timestamp_t*)message.data;

        // show we got a message, and its contents
        ESP_LOGI(TAG, "Got a timestamp: %lld (%08llX)\n", ts->unix_time, ts->unix_time);

        // receive 3 timestamps
        i++;
        if (i >= 3) {

            // to clean up a listener, call the remove listener
            // this free's the queue from heap
            esp_err_t err = badge_comms_remove_listener(timestamp_queue);

            // show the result of the listener removal
            ESP_LOGI(TAG, "unsubscription result: %s", esp_err_to_name(err));
            return;
        }
    }
}

void send_timestamp(void) {
    // first we create a struct with the data, as we would like to receive on the other side
    badge_message_timestamp_t data = {
        .unix_time = time(NULL)  // add the system timestamp to the message
    };

    // then we wrap the data in something to send over the comms bus
    badge_comms_message_t message = {0};
    message.message_type          = MESSAGE_TYPE_TIMESTAMP;
    message.data_len_to_send      = sizeof(data);
    memcpy(message.data, &data, message.data_len_to_send);

    // send the message over the comms bus
    badge_comms_send_message(&message);
}
