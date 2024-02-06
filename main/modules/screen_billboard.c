#include "application.h"
#include "badge_comms.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screen_home.h"
#include "screens.h"
#include "textedit.h"
#include "time.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_random.h>
#include <string.h>

#define nbmessages 9

static const char* TAG = "billboard";

char nicknamearray[nbmessages][nicknamelenght];
char messagearray[nbmessages][messagelenght];
int  messagecursor = 0;
int  initflag      = 0;

// void receive_repertoire(void) {
//     // get a queue to listen on, for message type MESSAGE_TYPE_TIMESTAMP, and size badge_message_timestamp_t
//     QueueHandle_t str_queue = badge_comms_add_listener(MESSAGE_TYPE_STRING, sizeof(badge_message_str));
//     // check if an error occurred (check logs for the reason)
//     if (str_queue == NULL) {
//         ESP_LOGE(TAG, "Failed to add listener");
//         return;
//     }

//     uint32_t i = 0;

//     while (true) {
//         // variable for the queue to store the message in
//         ESP_LOGI(TAG, "listening");
//         badge_comms_message_t message;
//         xQueueReceive(str_queue, &message, portMAX_DELAY);

//         // typecast the message data to the expected message type
//         badge_message_str* ts = (badge_message_str*)message.data;

//         // show we got a message, and its contents
//         ESP_LOGI(TAG, "Got a string: %s \n", ts->nickname);
//         ESP_LOGI(TAG, "Got a string: %s \n", ts->payload);

//         // receive 3 timestamps
//         i++;
//         if (i >= 3) {

//             // to clean up a listener, call the remove listener
//             // this free's the queue from heap
//             esp_err_t err = badge_comms_remove_listener(str_queue);

//             // show the result of the listener removal
//             ESP_LOGI(TAG, "unsubscription result: %s", esp_err_to_name(err));
//             return;
//         }
//     }
// }

// void send_repertoire(void) {
//     // first we create a struct with the data, as we would like to receive on the other side
//     badge_message_str data;
//     char              nickname[nicknamelenght]     = "dwawdawda";
//     char              playermessage[messagelenght] = "babnannan";
//     strcpy(data.nickname, nickname);
//     strcpy(data.payload, playermessage);

//     // then we wrap the data in something to send over the comms bus
//     badge_comms_message_t message = {0};
//     message.message_type          = MESSAGE_TYPE_STRING;
//     message.data_len_to_send      = sizeof(data);
//     memcpy(message.data, &data, message.data_len_to_send);

//     // send the message over the comms bus
//     badge_comms_send_message(&message);
//     bsp_set_addressable_led(LED_GREEN);
//     vTaskDelay(pdMS_TO_TICKS(100));
//     bsp_set_addressable_led(LED_OFF);
// }

void receive_str(void) {
    // get a queue to listen on, for message type MESSAGE_TYPE_TIMESTAMP, and size badge_message_timestamp_t
    QueueHandle_t str_queue = badge_comms_add_listener(MESSAGE_TYPE_STRING, sizeof(badge_message_str));
    // check if an error occurred (check logs for the reason)
    if (str_queue == NULL) {
        ESP_LOGE(TAG, "Failed to add listener");
        return;
    }

    uint32_t i = 0;

    while (true) {
        // variable for the queue to store the message in
        ESP_LOGI(TAG, "listening");
        badge_comms_message_t message;
        xQueueReceive(str_queue, &message, portMAX_DELAY);

        // typecast the message data to the expected message type
        badge_message_str* ts = (badge_message_str*)message.data;

        // show we got a message, and its contents
        ESP_LOGI(TAG, "Got a string: %s \n", ts->nickname);
        ESP_LOGI(TAG, "Got a string: %s \n", ts->payload);

        // receive 3 timestamps
        i++;
        if (i >= 3) {

            // to clean up a listener, call the remove listener
            // this free's the queue from heap
            esp_err_t err = badge_comms_remove_listener(str_queue);

            // show the result of the listener removal
            ESP_LOGI(TAG, "unsubscription result: %s", esp_err_to_name(err));
            return;
        }
    }
}

void send_str(char _nickname[nicknamelenght], char _payload[messagelenght]) {
    // first we create a struct with the data, as we would like to receive on the other side
    badge_message_str data;
    strcpy(data.nickname, _nickname);
    strcpy(data.payload, _payload);

    // then we wrap the data in something to send over the comms bus
    badge_comms_message_t message = {0};
    message.message_type          = MESSAGE_TYPE_STRING;
    message.data_len_to_send      = sizeof(data);
    memcpy(message.data, &data, message.data_len_to_send);

    // send the message over the comms bus
    badge_comms_send_message(&message);
}

void DisplayBillboard(int _addmessageflag, char* _nickname, char* _message) {
    // set screen font and buffer
    const pax_font_t* font = pax_font_sky;
    pax_buf_t*        gfx  = bsp_get_gfx_buffer();

    // set infrastructure
    pax_background(gfx, WHITE);
    pax_draw_text(gfx, BLACK, font, 18, 70, 0, "The billboard");
    AddSwitchesBoxtoBuffer(SWITCH_1);
    pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
    AddSwitchesBoxtoBuffer(SWITCH_5);
    pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "Send");

    int messageoffsetx = 1;
    int messageoffsety = 21;

    int textboxoffestx = 1;
    int textboxoffesty = 20;

    int textboxheight = 10;
    int textboxwidth  = gfx->height - textboxoffestx;

    int fontsize = 9;

    char billboardline[128];

    // if the add message flag is raised
    if (_addmessageflag) {
        // add message to the board
        strcpy(nicknamearray[messagecursor], _nickname);
        strcpy(messagearray[messagecursor], _message);

        // increment or reset cursor
        if (messagecursor >= nbmessages) {
            messagecursor = 0;
        }
        messagecursor++;
    }

    for (int i = 0; i < nbmessages; i++) {
        int j = 0 - i;
        if (j < 0)
            j = nbmessages - i;
        strcpy(billboardline, nicknamearray[j]);
        if (strlen(nicknamearray[j]) != 0)  // don't put the colon if there is no message
            strcat(billboardline, ": ");
        strcat(billboardline, messagearray[j]);

        int shift = (i + messagecursor + nbmessages - 1) % nbmessages;

        pax_draw_text(
            gfx,
            BLACK,
            font,
            fontsize,
            messageoffsetx,
            messageoffsety + textboxheight * shift,
            billboardline
        );
        pax_outline_rect(
            gfx,
            BLACK,
            textboxoffestx,
            textboxoffesty + textboxheight * shift,
            textboxwidth,
            textboxheight
        );
    }
    bsp_display_flush();
}

screen_t screen_billboard_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, false, false, false, true);
    if (!initflag)
        for (int i = 0; i < nbmessages; i++) {
            strcpy(nicknamearray[i], "");
            strcpy(messagearray[i], "");
            initflag = 1;
        }

    char nickname[nicknamelenght]     = "";
    char playermessage[messagelenght] = "";

    // init broadcast receive
    // get a queue to listen on, for message type MESSAGE_TYPE_TIMESTAMP, and size badge_message_timestamp_t
    QueueHandle_t str_queue = badge_comms_add_listener(MESSAGE_TYPE_STRING, sizeof(badge_message_str));
    // check if an error occurred (check logs for the reason)
    if (str_queue == NULL) {
        ESP_LOGE(TAG, "Failed to add listener");
    }
    ESP_LOGI(TAG, "listening");
    badge_comms_message_t message;

    DisplayBillboard(0, "", "");  // Draw billboard without adding message
    // receive_repertoire();


    while (1) {
        event_t event = {0};

        // upon receiving a message
        if (xQueueReceive(str_queue, &message, pdMS_TO_TICKS(1)) == pdTRUE) {
            badge_message_str* ts = (badge_message_str*)message.data;
            ESP_LOGI(TAG, "Got a string: %s \n", ts->nickname);
            ESP_LOGI(TAG, "Got a string: %s \n", ts->payload);
            DisplayBillboard(1, ts->nickname, ts->payload);
            bsp_set_addressable_led(LED_PURPLE);
            vTaskDelay(pdMS_TO_TICKS(100));
            bsp_set_addressable_led(LED_OFF);
        }

        if (xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {

            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1:  // when exiting, remove the billboard channel listener
                            esp_err_t err = badge_comms_remove_listener(str_queue);
                            ESP_LOGI(TAG, "unsubscription result: %s", esp_err_to_name(err));
                            return screen_home;
                            break;

                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5:
                            // send_repertoire();
                            if (textedit(
                                    "Type your message to broadcast:",
                                    application_event_queue,
                                    keyboard_event_queue,
                                    playermessage,
                                    sizeof(playermessage)
                                )) {
                                nvs_get_str_wrapped("owner", "nickname", nickname, sizeof(nickname));
                                DisplayBillboard(1, nickname, playermessage);
                                send_str(nickname, playermessage);
                                strcpy(playermessage, "");

                                bsp_set_addressable_led(LED_GREEN);
                                vTaskDelay(pdMS_TO_TICKS(100));
                                bsp_set_addressable_led(LED_OFF);
                            } else
                                DisplayBillboard(0, nickname, playermessage);  // no new messages
                            // refactor : due to unknown state of keyboard coming back from textedit
                            InitKeyboard(keyboard_event_queue);
                            configure_keyboard_presses(keyboard_event_queue, true, false, false, false, true);
                            break;
                        default: break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
