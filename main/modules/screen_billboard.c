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

#define nbmessages   9
#define sizenickname 64
#define sizemessages 64

static char const * TAG = "billboard";

char nicknamearray[nbmessages][sizenickname];
char messagearray[nbmessages][sizemessages];
int  messagecursor = 0;

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
        ESP_LOGI(TAG, "Got a string: %d \n", ts->messagestr);

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

void send_str(void) {
    // first we create a struct with the data, as we would like to receive on the other side
    badge_message_str data = {
        .messagestr = 'd'  // add the system timestamp to the message
    };

    // then we wrap the data in something to send over the comms bus
    badge_comms_message_t message = {0};
    message.message_type          = MESSAGE_TYPE_STRING;
    message.data_len_to_send      = sizeof(data);
    memcpy(message.data, &data, message.data_len_to_send);

    // send the message over the comms bus
    badge_comms_send_message(&message);
}


static esp_err_t nvs_get_str_wrapped(char const * namespace, char const * key, char* buffer, size_t buffer_size) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res == ESP_OK) {
        size_t size = 0;
        res         = nvs_get_str(handle, key, NULL, &size);
        if ((res == ESP_OK) && (size <= buffer_size - 1)) {
            res = nvs_get_str(handle, key, buffer, &size);
            if (res != ESP_OK) {
                buffer[0] = '\0';
            }
        }
    } else {
        buffer[0] = '\0';
    }
    nvs_close(handle);
    return res;
}

void DisplayBillboard(int _addmessageflag, char* _nickname, char* _message) {
    // set screen font and buffer
    pax_font_t const * font = pax_font_sky;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();

    // set infrastructure
    pax_background(gfx, WHITE);
    pax_draw_text(gfx, BLACK, font, 18, 80, 0, "The billboard");
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

    char billboardline[64];

    // if the add message flag is raised
    if (_addmessageflag) {
        // clear board if full and reset cursor
        if (messagecursor >= nbmessages) {
            for (int i = 0; i < nbmessages; i++) {
                strcpy(nicknamearray[i], "");
                strcpy(messagearray[i], "");
                messagecursor = 0;
            }
        }
        // add message to the board
        strcpy(nicknamearray[messagecursor], _nickname);
        strcpy(messagearray[messagecursor], _message);
        messagecursor++;
    }


    for (int i = 0; i < nbmessages; i++) {
        strcpy(billboardline, nicknamearray[i]);
        if (strlen(nicknamearray[i]) != 0)
            strcat(billboardline, ": ");
        strcat(billboardline, messagearray[i]);

        pax_draw_text(gfx, BLACK, font, fontsize, messageoffsetx, messageoffsety + textboxheight * i, billboardline);
        pax_outline_rect(gfx, BLACK, textboxoffestx, textboxoffesty + textboxheight * i, textboxwidth, textboxheight);
    }
    bsp_display_flush();
}

screen_t screen_billboard_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {

    // update the keyboard event handler settings
    event_t kbsettings = {
        .type                                     = event_control_keyboard,
        .args_control_keyboard.enable_typing      = false,
        .args_control_keyboard.enable_actions     = {true, true, true, true, true},
        .args_control_keyboard.enable_leds        = true,
        .args_control_keyboard.enable_relay       = true,
        kbsettings.args_control_keyboard.capslock = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    // set screen font and buffer
    // pax_font_t const * font = pax_font_sky;
    // pax_buf_t*         gfx  = bsp_get_gfx_buffer();

    // // memory
    // nvs_handle_t handle;
    // esp_err_t    res = nvs_open("owner", NVS_READWRITE, &handle);

    // // read nickname from memory
    // char   nickname[64] = {0};
    // size_t size         = 0;
    // res                 = nvs_get_str(handle, "nickname", NULL, &size);
    // if ((res == ESP_OK) && (size <= sizeof(nickname) - 1)) {
    //     res = nvs_get_str(handle, "nickname", nickname, &size);
    //     if (res != ESP_OK || strlen(nickname) < 1) {
    //         sprintf(nickname, "No nickname configured");
    //     }
    // }

    // nvs_close(handle);

    // // scale the nickame to fit the screen and display(I think)
    // pax_vec1_t dims = {
    //     .x = 999,
    //     .y = 999,
    // };
    // float scale = 100;
    // while (scale > 1) {
    //     dims = pax_text_size(font, scale, nickname);
    //     if (dims.x <= 296 && dims.y <= 100)
    //         break;
    //     scale -= 0.2;
    // }
    // ESP_LOGW(TAG, "Scale: %f", scale);
    // pax_draw_text(gfx, BLACK, font, 18, 80, 50, "The cake is a lie The cake is a lie The cake is a lie");
    //  pax_insert_png_buf(gfx, homescreen_png_start, homescreen_png_end - homescreen_png_start, 0, gfx->width - 24, 0);


    // pax_draw_text(gfx, BLACK, font, fontsize, messageoffsetx, messageoffsety, billboardline);
    // pax_draw_text(gfx, BLACK, font, fontsize, messageoffsetx, messageoffsety + textboxheight, nicknamearray[1]);
    // pax_draw_text(gfx, BLACK, font, fontsize, messageoffsetx, messageoffsety + textboxheight * 2, nicknamearray[2]);

    // pax_outline_rect(gfx, BLACK, textboxoffestx, textboxoffesty + textboxheight * 0, textboxwidth, textboxheight);
    // pax_outline_rect(gfx, BLACK, textboxoffestx, textboxoffesty + textboxheight * 1, textboxwidth, textboxheight);
    // pax_outline_rect(gfx, BLACK, textboxoffestx, textboxoffesty + textboxheight * 2, textboxwidth, textboxheight);

    receive_str();
    for (int i = 0; i < nbmessages; i++) {
        strcpy(nicknamearray[i], "");
        strcpy(messagearray[i], "");
    }
    DisplayBillboard(0, "", "");
    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;

                        case SWITCH_2:
                            char dummynickname[3][sizenickname] = {"Zero Cool", "Acid Burn", "Cereal Killer"};
                            char dummymessage[3][sizemessages]  = {
                                "Hack the Planet!",
                                "Mess with the best, die like the rest",
                                "1337"
                            };
                            int i = esp_random() % 3;
                            DisplayBillboard(1, dummynickname[i], dummymessage[i]);

                            break;
                        case SWITCH_3: break;
                        case SWITCH_4: send_str(); break;
                        case SWITCH_5:
                            char playermessage[64];
                            strcpy(playermessage, "Banana bread");
                            // Using textedit here works until you press "ok", then crashes
                            // textedit(
                            //     "Type your message to broadcast:",
                            //     application_event_queue,
                            //     keyboard_event_queue,
                            //     playermessage,
                            //     sizeof(playermessage)
                            // );
                            char nickname[64] = "";
                            nvs_get_str_wrapped("owner", "nickname", nickname, sizeof(nickname));
                            DisplayBillboard(1, nickname, playermessage);
                            break;
                        default: break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
