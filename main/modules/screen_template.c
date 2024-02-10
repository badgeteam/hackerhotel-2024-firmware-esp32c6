#include "application.h"
#include "badge-communication-protocol.h"
#include "badge-communication.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screen_home.h"
#include "screen_pointclick.h"
#include "screen_repertoire.h"
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_random.h>
#include <string.h>

static const char* TAG = "template";

extern const uint8_t border1_png_start[] asm("_binary_border1_png_start");
extern const uint8_t border1_png_end[] asm("_binary_border1_png_end");

void receive_strb(void) {
    // get a queue to listen on, for message type MESSAGE_TYPE_TIME, and size badge_message_timestamp_t
    QueueHandle_t BS_queue =
        NULL;  // badge_comms_add_listener(MESSAGE_TYPE_BATTLESHIP, sizeof(badge_message_battleship_t));

    // check if an error occurred (check logs for the reason)
    if (BS_queue == NULL) {
        ESP_LOGE(TAG, "Failed to add listener");
        return;
    }

    uint32_t i = 0;

    while (true) {
        // variable for the queue to store the message in
        ESP_LOGI(TAG, "listening");
        badge_comms_message_t message;
        xQueueReceive(BS_queue, &message, portMAX_DELAY);

        // typecast the message data to the expected message type
        badge_message_battleship_t* ts = (badge_message_battleship_t*)message.data;



        // show we got a message, and its contents
        for (int i = 0; i < BSpayload; i++) ESP_LOGI(TAG, "dataBS: %d is %d", i, ts->dataBS[i]);
        bsp_set_addressable_led(LED_PURPLE);
        vTaskDelay(pdMS_TO_TICKS(100));
        bsp_set_addressable_led(LED_OFF);
        // receive 3 timestamps
        i++;
        if (i >= 3) {

            // to clean up a listener, call the remove listener
            // this free's the queue from heap
            // esp_err_t err = badge_comms_remove_listener(BS_queue);

            // show the result of the listener removal
            // ESP_LOGI(TAG, "unsubscription result: %s", esp_err_to_name(err));
            return;
        }
    }
}

void send_strb(void) {
    // first we create a struct with the data, as we would like to receive on the other side
    badge_message_battleship_t data;
    uint8_t                    _dataBS[BSpayload];
    for (int i = 0; i < BSpayload; i++) _dataBS[i] = 69;
    for (int i = 0; i < BSpayload; i++) data.dataBS[i] = _dataBS[i];

    // then we wrap the data in something to send over the comms bus
    badge_comms_message_t message = {0};
    message.message_type          = MESSAGE_TYPE_BATTLESHIP;
    message.data_len_to_send      = sizeof(data);
    memcpy(message.data, &data, message.data_len_to_send);

    // send the message over the comms bus
    // badge_communication_send(&message);
    vTaskDelay(pdMS_TO_TICKS(100));  // SUPER DUPER IMPORTANT, OTHERWISE THE LEDS MESS WITH THE MESSAGE
    bsp_set_addressable_led(LED_GREEN);
    vTaskDelay(pdMS_TO_TICKS(100));
    bsp_set_addressable_led(LED_OFF);
}

void DisplayTemplate(void) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_draw_text(gfx, BLACK, font1, fontsizeS, 0, 0, "its coordinates");
    pax_outline_circle(gfx, BLACK, 0, 0, 3);
    pax_insert_png_buf(gfx, border1_png_start, border1_png_end - border1_png_start, 0, 127 - 11, 0);
    bsp_display_flush();
}

screen_t screen_template_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    reset_keyboard_settings(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);

    int displayflag = 1;

    QueueHandle_t BS_queue =
        NULL;  // badge_comms_add_listener(MESSAGE_TYPE_BATTLESHIP, sizeof(badge_message_battleship_t));
    if (BS_queue == NULL) {
        ESP_LOGE(TAG, "Failed to add listener");
    } else
        ESP_LOGI(TAG, "listening");
    badge_comms_message_t message;

    while (1) {
        event_t event = {0};
        if (xQueueReceive(BS_queue, &message, pdMS_TO_TICKS(10)) == pdTRUE) {
            ESP_LOGI(TAG, "listening");
            badge_comms_message_t message;
            xQueueReceive(BS_queue, &message, portMAX_DELAY);
            badge_message_battleship_t* ts = (badge_message_battleship_t*)message.data;

            for (int i = 0; i < BSpayload; i++) {
                // ennemy_data[i] = ts->dataBS[i];
                if (log)
                    ESP_LOGI(TAG, "dataBS: %d is %d", i, ts->dataBS[i]);
            }
            bsp_set_addressable_led(LED_PURPLE);
            vTaskDelay(pdMS_TO_TICKS(100));
            bsp_set_addressable_led(LED_OFF);
            // DisplayBillboard(1, ts->nickname, ts->payload);
        }
        if (displayflag) {
            DisplayTemplate();
            displayflag = 0;
        }
        if ((xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE)) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_3:
                            send_strb();
                            displayflag = 1;
                            break;
                        case SWITCH_4:
                            screen_repertoire_entry(application_event_queue, keyboard_event_queue, 1);

                            displayflag = 1;
                            break;
                        case SWITCH_5:
                            receive_strb();
                            displayflag = 1;
                            break;
                        default: break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
