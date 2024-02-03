#include "screen_repertoire.h"
#include "application.h"
#include "badge_comms.h"
#include "badge_messages.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
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
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_random.h>
#include <string.h>

static char const * TAG = "repertoire";

extern uint8_t const b_arrow1_png_start[] asm("_binary_b_arrow1_png_start");
extern uint8_t const b_arrow1_png_end[] asm("_binary_b_arrow1_png_end");
extern uint8_t const b_arrow2_png_start[] asm("_binary_b_arrow2_png_start");
extern uint8_t const b_arrow2_png_end[] asm("_binary_b_arrow2_png_end");

bool StoreRepertoire(
    char _repertoryIDlist[maxIDrepertoire][nicknamelenght], uint8_t mac[maxIDrepertoire][8], uint8_t _nbrepertoryID
) {
    bool res = nvs_set_u8_wrapped("Repertoire", "IDcount", _nbrepertoryID);
    ESP_LOGE(TAG, "set _nbrepertoryID: %d", _nbrepertoryID);
    for (int i = 0; i < _nbrepertoryID; i++) {
        char strnick[15] = "nickname";
        char strmac[15]  = "MAC";
        char nb[15];
        snprintf(nb, 15, "%d", i);
        strcat(strnick, nb);
        strcat(strmac, nb);
        ESP_LOGE(TAG, "nickname key: %s", strnick);
        ESP_LOGE(TAG, "nickname write: %s", _repertoryIDlist[i]);
        ESP_LOGE(TAG, "MAC key: %s", strmac);
        for (int y = 0; y < 8; y++) ESP_LOGE(TAG, "MAC: %d", mac[i][y]);
        nvs_set_str_wrapped("Repertoire", strnick, _repertoryIDlist[i]);
        nvs_set_u8_blob_wrapped("Repertoire", strmac, mac[i], 8);
    }
    return res;
}

int GetRepertoire(char _repertoryIDlist[maxIDrepertoire][nicknamelenght], uint8_t mac[maxIDrepertoire][8]) {
    uint8_t value = 0;
    bool    res   = nvs_get_u8_wrapped("Repertoire", "IDcount", &value);
    ESP_LOGE(TAG, "read _nbrepertoryID: %d", value);
    for (int i = 0; i < value; i++) {
        char strnick[15] = "nickname";
        char strmac[15]  = "MAC";
        char nb[15];
        snprintf(nb, 15, "%d", i);
        strcat(strnick, nb);
        strcat(strmac, nb);
        nvs_get_str_wrapped("Repertoire", strnick, _repertoryIDlist[i], sizeof(_repertoryIDlist[i]));
        nvs_get_u8_blob_wrapped("Repertoire", strmac, mac[i], 8);
        ESP_LOGE(TAG, "nickname key: %s", strnick);
        ESP_LOGE(TAG, "nickname read: %s", _repertoryIDlist[i]);
        ESP_LOGE(TAG, "MAC key: %s", strmac);
        for (int y = 0; y < 8; y++) ESP_LOGE(TAG, "MAC: %d", mac[i][y]);
    }
    return value;
}

void receive_repertoire(void) {
    // get a queue to listen on, for message type MESSAGE_TYPE_TIMESTAMP, and size badge_message_timestamp_t
    QueueHandle_t repertoire_queue =
        badge_comms_add_listener(MESSAGE_TYPE_REPERTOIRE, sizeof(badge_message_repertoire));
    // check if an error occurred (check logs for the reason)
    if (repertoire_queue == NULL) {
        ESP_LOGE(TAG, "Failed to add listener");
        return;
    }

    uint32_t i = 0;

    while (true) {
        // variable for the queue to store the message in
        ESP_LOGI(TAG, "listening");
        badge_comms_message_t message;
        xQueueReceive(repertoire_queue, &message, portMAX_DELAY);

        // typecast the message data to the expected message type
        badge_message_repertoire* ts = (badge_message_repertoire*)message.data;

        // show we got a message, and its contents
        ESP_LOGI(TAG, "Got a string: %s \n", ts->nickname);

        // receive 3 timestamps
        i++;
        if (i >= 3) {

            // to clean up a listener, call the remove listener
            // this free's the queue from heap
            esp_err_t err = badge_comms_remove_listener(repertoire_queue);

            // show the result of the listener removal
            ESP_LOGI(TAG, "unsubscription result: %s", esp_err_to_name(err));
            return;
        }
    }
}

void send_repertoire(void) {
    // first we create a struct with the data, as we would like to receive on the other side
    badge_message_repertoire data;
    char                     _nickname[nicknamelenght] = "Guru-san";
    // nvs_get_str_wrapped("owner", "nickname", _nickname, sizeof(_nickname));

    strcpy(data.nickname, _nickname);

    // then we wrap the data in something to send over the comms bus
    badge_comms_message_t message = {0};
    message.message_type          = MESSAGE_TYPE_REPERTOIRE;
    message.data_len_to_send      = sizeof(data);
    memcpy(message.data, &data, message.data_len_to_send);

    // send the message over the comms bus
    badge_comms_send_message(&message);
    vTaskDelay(pdMS_TO_TICKS(100));
    bsp_set_addressable_led(LED_GREEN);
    vTaskDelay(pdMS_TO_TICKS(100));
    bsp_set_addressable_led(LED_OFF);
}

void SendRegularBr(void) {
    while (1) {
        send_repertoire();
        vTaskDelay(pdMS_TO_TICKS(BroadcastInterval));
    }
}


void Display_repertoire(
    int     _cursor_x,
    int     _cursor_y,
    uint8_t _nbrepertoryID,
    int     _nbsurroundingID,
    int     _max_y,
    char    _repertoryIDlist[maxIDrepertoire][nicknamelenght],
    char    _surroundingIDlist[maxIDrepertoire][nicknamelenght]
) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();

    int title_o_y = 4;

    int maxperpage = 8;
    int box_y      = 11;
    // int box_x      = 120;
    int box_o_y    = 20;

    int pagefooter_o_x = 21;

    int text_rep_x = gfx->height / 4;
    int text_sur_x = gfx->height * 3 / 4;
    int text_cursor_x;


    pax_background(gfx, WHITE);
    AddSWtoBuffer("Exit", "", "", "", "");
    pax_insert_png_buf(gfx, b_arrow1_png_start, b_arrow1_png_end - b_arrow1_png_start, 66, 118, 0);
    pax_insert_png_buf(gfx, b_arrow2_png_start, b_arrow2_png_end - b_arrow2_png_start, 127, 118, 0);
    if (_cursor_x == remove) {
        text_cursor_x = text_rep_x;
        AddOneTextSWtoBuffer(SWITCH_5, "Remove");
    } else {
        text_cursor_x = text_sur_x;
        AddOneTextSWtoBuffer(SWITCH_5, "Add");
    }

    pax_center_text(gfx, BLACK, font1, fontsizeS * 1.5, gfx->height / 4, title_o_y, "Repertoire");
    pax_center_text(gfx, BLACK, font1, fontsizeS * 1.5, gfx->height * 3 / 4, title_o_y, "In proximity");
    // for (int page = 0; page < ((_max_y / maxperpage) + 1); page++) {
    int page = _cursor_y / maxperpage;
    ESP_LOGE(TAG, "Page: %d", page);
    ESP_LOGE(TAG, "Cursor x: %d", _cursor_x);
    ESP_LOGE(TAG, "Cursor y: %d", _cursor_y);

    for (int i = 0; i < maxperpage; i++) {
        int text_IDs_y = box_o_y + box_y * i;
        if (_nbrepertoryID > (i + page * maxperpage)) {
            // pax_outline_rect(gfx, BLACK, gfx->height / 2 - box_x, box_o_y + box_y * i, box_x, box_y);
            pax_center_text(
                gfx,
                BLACK,
                font1,
                fontsizeS,
                text_rep_x,
                text_IDs_y,
                _repertoryIDlist[i + page * maxperpage]
            );
        }
        if (_nbsurroundingID > (i + page * maxperpage)) {
            // pax_outline_rect(gfx, BLACK, gfx->height / 2, text_IDs_y, box_x, box_y);
            pax_center_text(
                gfx,
                BLACK,
                font1,
                fontsizeS,
                text_sur_x,
                text_IDs_y,
                _surroundingIDlist[i + page * maxperpage]
            );
        }
        // display cursor and page number
        if (_cursor_y == (i + page * maxperpage)) {
            pax_vec1_t dims = {
                .x = 999,
                .y = 999,
            };
            if (_cursor_x == 0)
                dims = pax_text_size(font1, fontsizeS, _repertoryIDlist[i + page * maxperpage]);
            if (_cursor_x == 1)
                dims = pax_text_size(font1, fontsizeS, _surroundingIDlist[i + page * maxperpage]);
            AddDiamondSelecttoBuf(text_cursor_x, text_IDs_y, dims.x);
            char pagefooter[10] = "Page";
            char str[20];
            snprintf(str, 12, "%d", page + 1);
            strcat(pagefooter, str);
            strcat(pagefooter, "/");
            snprintf(str, 12, "%d", (_max_y / maxperpage) + 1);
            strcat(pagefooter, str);
            pax_center_text(gfx, BLACK, font1, fontsizeS, gfx->height / 2, gfx->width - pagefooter_o_x, pagefooter);
        }
    }

    bsp_display_flush();
}

void AddSurroundingRepertoire(char _inboundnick[nicknamelenght], uint8_t _inbound_mac[8]) {
}

screen_t screen_repertoire_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    char    repertoryIDlist[maxIDrepertoire][nicknamelenght];
    char    surroundingIDlist[maxIDrepertoire][nicknamelenght];
    uint8_t repertory_mac[maxIDrepertoire][8];
    uint8_t surrounding_mac[maxIDrepertoire][8];
    for (int i = 0; i < maxIDrepertoire; i++) {
        strcpy(repertoryIDlist[i], "");
        strcpy(surroundingIDlist[i], "");
        for (int y = 0; y < 8; y++) repertory_mac[i][y] = 0;
    }

    uint8_t nbrepertoryID   = 0;
    int     nbsurroundingID = 0;
    int     max_y;
    int     cursor_x = 0;
    int     cursor_y = 0;

    nbrepertoryID = GetRepertoire(repertoryIDlist, repertory_mac);
    ESP_LOGE(TAG, "nbrepertoryID main loop: %d", nbrepertoryID);

    // insert scan

    // udpate max_y to be the biggest y
    max_y = nbrepertoryID;
    if (nbrepertoryID < nbsurroundingID)
        max_y = nbsurroundingID;

    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, false, false, true, true);
    if (nbsurroundingID > 0 && nbsurroundingID > 0)
        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_2, true);
    else
        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_2, false);
    if (nbrepertoryID == 0)
        cursor_x = 1;
    if (max_y)
        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_3, true);
    else
        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_3, false);

    // DebugKeyboardSettings();

    Display_repertoire(cursor_x, cursor_y, nbrepertoryID, nbsurroundingID, max_y, repertoryIDlist, surroundingIDlist);
    // receive_stra();


    // init broadcast receive
    // get a queue to listen on, for message type MESSAGE_TYPE_TIMESTAMP, and size badge_message_timestamp_t
    QueueHandle_t repertoire_queue =
        badge_comms_add_listener(MESSAGE_TYPE_REPERTOIRE, sizeof(badge_message_repertoire));
    // check if an error occurred (check logs for the reason)
    if (repertoire_queue == NULL) {
        ESP_LOGE(TAG, "Failed to add listener");
    } else
        ESP_LOGI(TAG, "listening");
    badge_comms_message_t message;

    BaseType_t   xReturned;
    TaskHandle_t SendRegularBr_handle = NULL;
    if (SendRegularBr_handle == NULL) {
        xReturned = xTaskCreate(SendRegularBr, "SendRegularBr", 1024, NULL, 1, SendRegularBr_handle);
    } else
        ESP_LOGI(TAG, "Error");

    while (1) {
        event_t event = {0};

        // upon receiving a message
        if (xQueueReceive(repertoire_queue, &message, pdMS_TO_TICKS(1)) == pdTRUE) {
            badge_message_repertoire* ts                          = (badge_message_repertoire*)message.data;
            char                      inboundnick[nicknamelenght] = "";
            uint8_t                   _inbound_mac[8];
            strcpy(inboundnick, ts->nickname);
            for (int i = 0; i < 8; i++) {
                _inbound_mac[i] = message.from_mac[i];
                ESP_LOGI(TAG, "MAC: %d \n", _inbound_mac[i]);
            }
            ESP_LOGI(TAG, "Got a string: %s \n", ts->nickname);
            // DisplayBillboard(1, ts->nickname, ts->payload);
            vTaskDelay(pdMS_TO_TICKS(100));
            bsp_set_addressable_led(LED_PURPLE);
            vTaskDelay(pdMS_TO_TICKS(100));
            bsp_set_addressable_led(LED_OFF);

            // check if already know the inbound message

            int flag_already_exist    = 0;
            int flag_line_repertory   = 0;
            int flag_line_surrounding = 0;
            for (int i = 0; i < maxIDrepertoire; i++) {
                for (int y = 0; y < 8; y++) {
                    if (repertory_mac[i][y] == _inbound_mac[y])
                        flag_line_repertory++;
                    if (surrounding_mac[i][y] == _inbound_mac[y])
                        flag_line_surrounding++;
                }
                if (flag_line_repertory == 8) {
                    flag_already_exist = 1;
                    strcpy(repertoryIDlist[i], inboundnick);
                }

                if (flag_line_surrounding == 8) {
                    flag_already_exist = 1;
                    strcpy(surroundingIDlist[i], inboundnick);
                }
                flag_line_repertory   = 0;
                flag_line_surrounding = 0;
            }

            // add incoming message to surrounding
            if ((!flag_already_exist) && (nbsurroundingID < maxIDrepertoire)) {
                strcpy(surroundingIDlist[nbsurroundingID], inboundnick);
                for (int i = 0; i < 8; i++) surrounding_mac[nbsurroundingID][i] = _inbound_mac[i];
                nbsurroundingID++;
            }


            // to refactor
            Display_repertoire(
                cursor_x,
                cursor_y,
                nbrepertoryID,
                nbsurroundingID,
                max_y,
                repertoryIDlist,
                surroundingIDlist
            );
        }

        if ((xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE)) {
            ESP_LOGE(TAG, "loop");
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1:
                            StoreRepertoire(repertoryIDlist, repertory_mac, nbrepertoryID);
                            configure_keyboard_rotate_disable(keyboard_event_queue);
                            ESP_LOGI(TAG, "Exit");
                            if (xReturned == pdPASS) {
                                // vTaskDelete(SendRegularBr_handle);
                            }
                            vTaskDelay(pdMS_TO_TICKS(100));
                            return screen_home;
                            break;
                        case SWITCH_L2:
                            cursor_x = Decrement(cursor_x, 1);
                            if (cursor_x == 0 && cursor_y > (nbrepertoryID - 1))
                                cursor_y = nbrepertoryID - 1;
                            if (cursor_x == 1 && cursor_y > (nbsurroundingID - 1))
                                cursor_y = nbsurroundingID - 1;
                            break;
                        case SWITCH_R2:
                            cursor_x = Increment(cursor_x, 1);
                            if (cursor_x == 0 && cursor_y > (nbrepertoryID - 1))
                                cursor_y = nbrepertoryID - 1;
                            if (cursor_x == 1 && cursor_y > (nbsurroundingID - 1))
                                cursor_y = nbsurroundingID - 1;
                            break;

                        case SWITCH_L3:
                            if (cursor_x == 0)
                                cursor_y = Decrement(cursor_y, nbrepertoryID - 1);
                            if (cursor_x == 1)
                                cursor_y = Decrement(cursor_y, nbsurroundingID - 1);
                            break;
                        case SWITCH_R3:
                            if (cursor_x == 0)
                                cursor_y = Increment(cursor_y, nbrepertoryID - 1);
                            if (cursor_x == 1)
                                cursor_y = Increment(cursor_y, nbsurroundingID - 1);
                            break;

                        case SWITCH_3: break;
                        case SWITCH_4: send_repertoire(); break;
                        case SWITCH_5:
                            switch (cursor_x) {
                                case remove:
                                    char promt[128] = "Are you sure you want to remove ";
                                    strcat(promt, repertoryIDlist[cursor_y]);
                                    strcat(promt, "?");
                                    if (Screen_Confirmation(promt, application_event_queue, keyboard_event_queue)) {
                                        strcpy(repertoryIDlist[cursor_y], "");
                                        nbrepertoryID--;
                                        for (int i = cursor_y; i < nbrepertoryID; i++) {
                                            strcpy(repertoryIDlist[i], repertoryIDlist[i + 1]);
                                            for (int y = 0; y < 8; y++) repertory_mac[i][y] = repertory_mac[i + 1][y];
                                        }
                                        if (cursor_y)
                                            cursor_y--;
                                    }

                                    break;
                                case add:
                                    strcpy(repertoryIDlist[nbrepertoryID], surroundingIDlist[cursor_y]);
                                    for (int i = 0; i < 8; i++)
                                        repertory_mac[nbrepertoryID][i] = surrounding_mac[cursor_y][i];
                                    cursor_y = nbrepertoryID;
                                    cursor_x = 0;
                                    nbrepertoryID++;
                                    nbsurroundingID--;
                                    break;
                            }
                            break;
                        default: break;
                    }
                    // udpate max_y to be the biggest y
                    max_y = nbrepertoryID;
                    if (nbrepertoryID < nbsurroundingID)
                        max_y = nbsurroundingID;
                    // disable x navigation if nothing in range
                    if (nbsurroundingID > 0 && nbsurroundingID > 0)
                        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_2, true);
                    else
                        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_2, false);
                    if (max_y)
                        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_3, true);
                    else
                        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_3, false);
                    if (nbrepertoryID == 0)
                        cursor_x = 1;
                    if (nbsurroundingID == 0)
                        cursor_x = 0;
                    Display_repertoire(
                        cursor_x,
                        cursor_y,
                        nbrepertoryID,
                        nbsurroundingID,
                        max_y,
                        repertoryIDlist,
                        surroundingIDlist
                    );
                    break;

                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type); break;
            }
        }
    }
}
