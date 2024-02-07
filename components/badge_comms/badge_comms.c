#include "badge_comms.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_ieee802154.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"
#include "ieee802154_header.h"
#include "memory.h"

static char const * TAG = "badge_comms";

uint16_t TargetAddress = 0xFFFF;

static QueueHandle_t badge_comms_receive = NULL;

typedef struct {
    badge_comms_message_type_t message_type;
    QueueHandle_t              queue;
    uint8_t                    expected_message_len;
    bool                       in_use;
} badge_comms_listener_t;

#define BADGE_COMMS_DEFAULT_QUEUE_DEPTH (5)
#define BADGE_COMMS_MAX_LISTENERS       (20)
static badge_comms_listener_t badge_comms_listeners[BADGE_COMMS_MAX_LISTENERS] = {0};
static SemaphoreHandle_t      badge_comms_listener_semaphore                   = NULL;

QueueHandle_t badge_comms_add_listener(badge_comms_message_type_t message_type, uint8_t expected_message_len) {
    if (badge_comms_listener_semaphore == NULL) {
        ESP_LOGE(TAG, "Listener semaphore has not been initialised");
        return NULL;
    }

    xSemaphoreTake(badge_comms_listener_semaphore, portMAX_DELAY);
    QueueHandle_t           queue              = NULL;
    // find the first entry in the badge_comms_listeners which is not in use
    badge_comms_listener_t* available_listener = NULL;
    for (uint32_t i = 0; i < BADGE_COMMS_MAX_LISTENERS; i++) {
        if (!badge_comms_listeners[i].in_use) {
            available_listener = &badge_comms_listeners[i];
            break;
        }
    }

    // if no listener space is available, return NO_MEM
    if (available_listener == NULL) {
        ESP_LOGE(TAG, "Unable to find a free listener spot");
        goto error;
    }

    // we only create a queue after we found a place for the queue to live
    queue = xQueueCreate(BADGE_COMMS_DEFAULT_QUEUE_DEPTH, sizeof(badge_comms_message_t));
    if (queue == NULL) {
        ESP_LOGE(TAG, "Failed to allocate queue");
        goto error;
    }

    available_listener->message_type         = message_type;
    available_listener->queue                = queue;
    available_listener->expected_message_len = expected_message_len;
    available_listener->in_use               = true;

error:
    xSemaphoreGive(badge_comms_listener_semaphore);
    return queue;
}

esp_err_t badge_comms_remove_listener(QueueHandle_t queue) {
    if (queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (badge_comms_listener_semaphore == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = ESP_ERR_NOT_FOUND;

    xSemaphoreTake(badge_comms_listener_semaphore, portMAX_DELAY);

    // find all listeners matching the message type and queue, and mark them as not in use
    for (uint32_t i = 0; i < BADGE_COMMS_MAX_LISTENERS; i++) {
        if (badge_comms_listeners[i].in_use && badge_comms_listeners[i].queue == queue) {
            badge_comms_listeners[i].in_use = false;
            err                             = ESP_OK;
            vQueueDelete(queue);
        }
    }

    xSemaphoreGive(badge_comms_listener_semaphore);
    return err;
}

void esp_ieee802154_receive_done(uint8_t* frame, esp_ieee802154_frame_info_t* frame_info) {
    BaseType_t xHigherPriorityTaskWoken = false;
    xQueueSendToBackFromISR(badge_comms_receive, frame, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
        taskYIELD();
}

void badge_comms_send_message(badge_comms_message_t* comms_message) {
    uint16_t pan_id = 0x1234;
    uint8_t  buffer[256];

    uint8_t eui64[8];
    esp_ieee802154_get_extended_address(eui64);

    ieee802154_address_t src = {
        .mode         = ADDR_MODE_LONG,
        .long_address = {eui64[0], eui64[1], eui64[2], eui64[3], eui64[4], eui64[5], eui64[6], eui64[7]}
    };

    ieee802154_address_t dst = {.mode = ADDR_MODE_SHORT, .short_address = TargetAddress};

    //     ieee802154_address_t dst = {
    //     .mode            = ADDR_MODE_LONG,
    //     .long_address[0] = 0x40,
    //     .long_address[1] = 0x4c,
    //     .long_address[2] = 0xca,
    //     .long_address[3] = 0xff,
    //     .long_address[4] = 0xfe,
    //     .long_address[5] = 0x49,
    //     .long_address[6] = 0x5c,
    //     .long_address[7] = 0x5c
    // };

    // ESP_LOGE(TAG, "Mac sent %02x", dst.long_address[0]);
    // ESP_LOGE(TAG, "Mac sent %02x", dst.long_address[1]);
    // ESP_LOGE(TAG, "Mac sent %02x", dst.long_address[2]);
    // ESP_LOGE(TAG, "Mac sent %02x", dst.long_address[3]);
    // ESP_LOGE(TAG, "Mac sent %02x", dst.long_address[4]);
    // ESP_LOGE(TAG, "Mac sent %02x", dst.long_address[5]);
    // ESP_LOGE(TAG, "Mac sent %02x", dst.long_address[6]);
    // ESP_LOGE(TAG, "Mac sent %02x", dst.long_address[7]);

    uint8_t hdr_len = ieee802154_header(&pan_id, &src, &pan_id, &dst, false, &buffer[1], sizeof(buffer) - 1);

    buffer[hdr_len + 1] = comms_message->message_type & 0xFF;
    memcpy(buffer + hdr_len + 2, comms_message->data, comms_message->data_len_to_send);

    hdr_len += comms_message->data_len_to_send + 1;  // set userdata buffersize, and one for the message type
    hdr_len += 2;                                    // save space for preceding protocol bytes

    buffer[0] = hdr_len;  // packet length

    esp_ieee802154_transmit(buffer, false);
}

void parse_message(
    uint8_t  message[BADGE_COMMS_MAX_MESSAGE_SIZE],
    uint8_t  userdata[BADGE_COMMS_USER_DATA_MAX_LEN],
    uint8_t  from_mac[8],
    uint8_t* userdata_len
) {

    uint8_t len = message[0] + 1;

    //    uint16_t channel = message[5] <<8| message[4];
    //    uint16_t to_address = message[7] <<8| message[6];

    for (uint8_t i = 0; i < 8; i++) {
        // parse and reverse the sender mac address
        from_mac[7 - i] = message[i + 8];
    }

    *userdata_len =
        MIN(len - 16 - 2,
            BADGE_COMMS_USER_DATA_MAX_LEN);  // lenth - bytes till the beginning of userdata - proceeding protocol bytes
    memcpy(userdata, message + 16, *userdata_len);

    //    uint8_t rssi = message[len-2];
    //    uint8_t lqi = message[len-1] & 0xF;
}

void badge_comms_receiver(void* param) {
    (void)param;

    uint8_t               received_message[BADGE_COMMS_MAX_MESSAGE_SIZE];
    uint8_t               parsed_userdata[BADGE_COMMS_USER_DATA_MAX_LEN];
    uint8_t               userdata_len;
    badge_comms_message_t message;
    uint8_t               from_mac[8];

    while (true) {
        xQueueReceive(badge_comms_receive, received_message, portMAX_DELAY);
        parse_message(received_message, parsed_userdata, from_mac, &userdata_len);

        message.message_type = parsed_userdata[0];
        uint8_t data_len     = userdata_len - 1;
        if (data_len >= BADGE_COMMS_USER_DATA_MAX_LEN) {
            data_len = BADGE_COMMS_USER_DATA_MAX_LEN;
        }
        memcpy(message.data, parsed_userdata + 1, data_len);  // -1 for the message type
        memcpy(message.from_mac, from_mac, 8);

        xSemaphoreTake(badge_comms_listener_semaphore, portMAX_DELAY);
        for (uint32_t i = 0; i < BADGE_COMMS_MAX_LISTENERS; i++) {
            if (badge_comms_listeners[i].in_use == true &&
                badge_comms_listeners[i].message_type == message.message_type &&
                badge_comms_listeners[i].expected_message_len == data_len) {
                xQueueSend(badge_comms_listeners[i].queue, &message, 0);
            }
        }
        xSemaphoreGive(badge_comms_listener_semaphore);
    }
}

esp_err_t init_badge_comms(void) {
    badge_comms_receive = xQueueCreate(5, BADGE_COMMS_MAX_MESSAGE_SIZE);
    if (badge_comms_receive == NULL) {
        return ESP_ERR_NO_MEM;
    }

    badge_comms_listener_semaphore = xSemaphoreCreateMutex();
    if (badge_comms_listener_semaphore == NULL) {
        ESP_LOGE(TAG, "Failed to create listener semaphore");
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(esp_ieee802154_enable(), TAG, "Failed to enable antenna");
    esp_ieee802154_set_promiscuous(false);
    esp_ieee802154_set_rx_when_idle(true);
    esp_ieee802154_set_panid(0x1234);
    uint8_t mac[8] = {0};
    ESP_RETURN_ON_ERROR(esp_read_mac(mac, ESP_MAC_IEEE802154), TAG, "Failed to read mac");
    esp_ieee802154_set_extended_address(mac);
    ESP_RETURN_ON_ERROR(esp_ieee802154_receive(), TAG, "Failed to enable badge_comms_receive");

    if (!xTaskCreate(badge_comms_receiver, "badge_comms", 2048, NULL, 8, NULL)) {
        return ESP_FAIL;
    }

    return ESP_OK;
}
