#include "screen_repertoire.h"
#include "application.h"
#include "badge_comms.h"
#include "badge_messages.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_ieee802154.h"
#include "esp_log.h"
#include "esp_mac.h"
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

static const char* TAG = "repertoire";

extern const uint8_t b_arrow1_png_start[] asm("_binary_b_arrow1_png_start");
extern const uint8_t b_arrow1_png_end[] asm("_binary_b_arrow1_png_end");
extern const uint8_t b_arrow2_png_start[] asm("_binary_b_arrow2_png_start");
extern const uint8_t b_arrow2_png_end[] asm("_binary_b_arrow2_png_end");

bool SettNBrepertoryID(uint16_t _ID) {
    bool res = nvs_set_u16_wrapped("Repertoire", "IDcount", _ID);
    if (log)
        ESP_LOGE(TAG, "set NBrepertoryID: %d", _ID);
    return res;
}

uint16_t GetNBrepertoryID(void) {
    uint16_t value = 0;
    bool     res   = nvs_get_u16_wrapped("Repertoire", "IDcount", &value);
    if (log)
        ESP_LOGE(TAG, "Get NBrepertoryID: %d", value);
    return value;
}

bool StoreRepertoire(char _repertoryIDlist[nicknamelength], uint8_t mac[8], uint16_t _ID) {
    char strnick[15] = "nickname";
    char strmac[15]  = "MAC";
    char nb[15];
    snprintf(nb, 15, "%d", _ID);
    strcat(strnick, nb);
    strcat(strmac, nb);
    if (log) {
        ESP_LOGE(TAG, "StoreRepertoire: ");
        ESP_LOGE(TAG, "set _ID: %d", _ID);
        ESP_LOGE(TAG, "nickname key: %s", strnick);
        ESP_LOGE(TAG, "nickname write: %s", _repertoryIDlist);
        ESP_LOGE(TAG, "MAC key: %s", strmac);
        for (int y = 0; y < 8; y++) ESP_LOGE(TAG, "MAC: %d", mac[y]);
    }
    bool res = nvs_set_str_wrapped("Repertoire", strnick, _repertoryIDlist);
    if (res == ESP_OK) {
        res = nvs_set_u8_blob_wrapped("Repertoire", strmac, mac, 8);
    }
    return res;
}

bool GetRepertoire(char _repertoryIDlist[nicknamelength], uint8_t mac[8], uint16_t _ID) {
    char strnick[15] = "nickname";
    char strmac[15]  = "MAC";
    char nb[15];
    // This is used as I can't get nvs_get_str_wrapped to populate a 1 dimension char array
    char value[2][nicknamelength] = {"", ""};
    snprintf(nb, 15, "%d", _ID);
    strcat(strnick, nb);
    strcat(strmac, nb);
    bool res = nvs_get_str_wrapped("Repertoire", strnick, value[0], sizeof(value[0]));
    if (res == ESP_OK) {
        res = nvs_get_u8_blob_wrapped("Repertoire", strmac, mac, 8);
    }
    strcpy(_repertoryIDlist, value[0]);
    if (log) {
        ESP_LOGE(TAG, "GetRepertoire: ");
        ESP_LOGE(TAG, "ID: %d", _ID);
        ESP_LOGE(TAG, "nickname key: %s", strnick);
        ESP_LOGE(TAG, "nickname read: %s", _repertoryIDlist);
        ESP_LOGE(TAG, "MAC key: %s", strmac);
    }
    for (int y = 0; y < 8; y++) ESP_LOGE(TAG, "MAC: %02x", mac[y]);
    return res;
}

void receive_repertoire(void) {
    // get a queue to listen on, for message type MESSAGE_TYPE_TIMESTAMP, and size badge_message_time_t
    QueueHandle_t repertoire_queue =
        badge_comms_add_listener(MESSAGE_TYPE_REPERTOIRE, sizeof(badge_message_repertoire_t));
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
        badge_message_repertoire_t* ts = (badge_message_repertoire_t*)message.data;

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
    badge_message_repertoire_t data;
    char                       _nickname[nicknamelength] = "Error get owner nickname";
    nvs_get_str_wrapped("owner", "nickname", _nickname, sizeof(_nickname));

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
    if (log)
        ESP_LOGE(TAG, "message sent to: %04x", TargetAddress);
}

void Display_repertoire(
    uint16_t        _nbrepertoryID,
    int             _nbsurroundingID,
    uint16_t        nb_item_rep,
    int             nb_item_sur,
    char            _repertoryIDlist[maxperpage][nicknamelength],
    char            _surroundingIDlist[maxIDsurrounding][nicknamelength],
    uint8_t         repertory_mac[maxperpage][8],
    uint8_t         surrounding_mac[maxIDsurrounding][8],
    struct cursor_t cursor,
    int             show_name_or_mac,
    int             page,
    int             max_page,
    int             addrmflag,
    int             _app
) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();

    int title_o_y = 4;

    // int page    = cursor.y / maxperpage;
    int box_y   = 11;
    int box_o_y = 20;

    int pagefooter_o_x = 21;

    int text_rep_x    = gfx->height / 4;
    int text_sur_x    = gfx->height * 3 / 4;
    int text_cursor_x = 0;

    if (log) {
        ESP_LOGE(TAG, "Display: Page: %d", page);
        ESP_LOGE(TAG, "Display: Cursor x: %d", cursor.x);
        ESP_LOGE(TAG, "Display: Cursor y: %d", cursor.y);
    }

    pax_background(gfx, WHITE);
    AddSWtoBuffer("", "", "", "", "");
    pax_insert_png_buf(gfx, b_arrow1_png_start, b_arrow1_png_end - b_arrow1_png_start, 66, 118, 0);
    pax_insert_png_buf(gfx, b_arrow2_png_start, b_arrow2_png_end - b_arrow2_png_start, 127, 118, 0);

    if (cursor.x == remove)
        text_cursor_x = text_rep_x;
    else
        text_cursor_x = text_sur_x;

    if (_app)
        AddOneTextSWtoBuffer(SWITCH_1, "Select");
    else
        AddOneTextSWtoBuffer(SWITCH_1, "Exit");

    if (cursor.x == remove && addrmflag == 1)
        AddOneTextSWtoBuffer(SWITCH_5, "Remove");
    if (cursor.x == add && addrmflag == 1)
        AddOneTextSWtoBuffer(SWITCH_5, "Add");

    if (!show_name_or_mac) {
        AddOneTextSWtoBuffer(SWITCH_4, "MAC");
    } else {
        AddOneTextSWtoBuffer(SWITCH_4, "Name");
    }
    pax_center_text(gfx, BLACK, font1, fontsizeS * 1.5, gfx->height / 4, title_o_y, "Repertoire");
    pax_center_text(gfx, BLACK, font1, fontsizeS * 1.5, gfx->height * 3 / 4, title_o_y, "In proximity");

    // display repertoire and surrounding name/mac content
    for (int i = 0; i < maxperpage; i++) {
        int  text_IDs_y                 = box_o_y + box_y * i;
        char leftfield[nicknamelength]  = "";
        char rightfield[nicknamelength] = "";
        char buf[10]                    = "";
        if (!show_name_or_mac) {
            strcpy(leftfield, _repertoryIDlist[i]);
            if (i <= nb_item_sur - 1)
                strcpy(rightfield, _surroundingIDlist[i + page * maxperpage]);
        } else {
            for (int y = 0; y < 8; y++) {
                snprintf(buf, 10, "%02x", repertory_mac[i][y]);
                strcat(leftfield, strcat(buf, ":"));
                if (i <= nb_item_sur - 1)
                    snprintf(buf, 10, "%02x", surrounding_mac[i + page * maxperpage][y]);
                strcat(rightfield, strcat(buf, ":"));
            }
            leftfield[strlen(leftfield) - 1]   = '\0';  // remove last :
            rightfield[strlen(rightfield) - 1] = '\0';  // remove last :
        }

        if (nb_item_rep > (i)) {
            pax_center_text(gfx, BLACK, font1, fontsizeS, text_rep_x, text_IDs_y, leftfield);
        }
        if (nb_item_sur > (i)) {
            pax_center_text(gfx, BLACK, font1, fontsizeS, text_sur_x, text_IDs_y, rightfield);
        }

        // display cursor and page number
        if (cursor.y == (i)) {
            pax_vec1_t dims = {
                .x = 999,
                .y = 999,
            };
            if (cursor.x == 0)
                dims = pax_text_size(font1, fontsizeS, leftfield);
            if (cursor.x == 1)
                dims = pax_text_size(font1, fontsizeS, rightfield);
            AddDiamondSelecttoBuf(text_cursor_x, text_IDs_y, dims.x);
            char pagefooter[10] = "Page";
            char str[20];
            // page
            snprintf(str, 12, "%d", page + 1);
            strcat(pagefooter, str);
            strcat(pagefooter, "/");
            // max page
            snprintf(str, 12, "%d", max_page + 1);
            strcat(pagefooter, str);
            pax_center_text(gfx, BLACK, font1, fontsizeS, gfx->height / 2, gfx->width - pagefooter_o_x, pagefooter);
        }
    }
    bsp_display_flush();
}

screen_t screen_repertoire_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int _app) {
    event_t tempkbsettings = kbsettings;

    char    repertoryIDlist[maxperpage][nicknamelength];
    char    surroundingIDlist[maxIDsurrounding][nicknamelength];
    uint8_t repertory_mac[maxperpage][8];
    uint8_t surrounding_mac[maxIDsurrounding][8];

    for (int i = 0; i < maxperpage; i++) {

        strcpy(repertoryIDlist[i], "");
        for (int y = 0; y < 8; y++) {
            repertory_mac[i][y] = 0;
        }
    }
    for (int i = 0; i < maxIDsurrounding; i++) {
        strcpy(surroundingIDlist[i], "");
        for (int y = 0; y < 8; y++) {
            surrounding_mac[i][y] = 0;
        }
    }

    // debug
    // strcpy(surroundingIDlist[0], "Florian");
    // strcpy(surroundingIDlist[1], "Dog");
    // strcpy(surroundingIDlist[2], "Ryan");
    // strcpy(surroundingIDlist[3], "dwadw");
    // strcpy(surroundingIDlist[4], "gap");
    // strcpy(surroundingIDlist[5], "stupid");
    // strcpy(surroundingIDlist[6], "telegraph");
    // strcpy(surroundingIDlist[7], "banana");

    // strcpy(surroundingIDlist[8], "bread");
    // strcpy(surroundingIDlist[9], "template");
    // strcpy(surroundingIDlist[10], "steve");
    // strcpy(surroundingIDlist[11], "nya");

    // surrounding_mac[0][0] = 0x69;
    // surrounding_mac[1][0] = 0x42;
    // surrounding_mac[2][0] = 0x18;

    // total numbers of IDs
    uint16_t nbrepertoryID   = GetNBrepertoryID();
    int      nbsurroundingID = 0;

    // numbers of IDs shown on screen
    uint16_t nb_item_rep = 0;
    int      nb_item_sur = 0;

    int             page             = 0;
    int             max_page         = 0;
    int             show_name_or_mac = 0;
    int             addrmflag        = 0;
    int             displayflag      = 1;
    int             timer_track      = esp_timer_get_time() / 5000000;
    struct cursor_t cursor           = {.x = 0, .y = 0, .yabs = 0};

    InitKeyboard(keyboard_event_queue);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_2, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_3, true);
    configure_keyboard_presses(keyboard_event_queue, true, false, false, true, true);

    // init broadcast receive
    // get a queue to listen on, for message type MESSAGE_TYPE_TIMESTAMP, and size badge_message_time_t
    QueueHandle_t repertoire_queue =
        badge_comms_add_listener(MESSAGE_TYPE_REPERTOIRE, sizeof(badge_message_repertoire_t));
    // check if an error occurred (check logs for the reason)
    if (repertoire_queue == NULL) {
        ESP_LOGE(TAG, "Failed to add listener");
    } else
        ESP_LOGI(TAG, "listening");
    badge_comms_message_t message;

    while (1) {
        if ((esp_timer_get_time() / 1000000) > (timer_track * BroadcastInterval)) {
            send_repertoire();
            timer_track++;
        }
        event_t event = {0};
        if (displayflag) {
            // check if the nb of pages has changed
            int max_y = nbrepertoryID;
            if (nbrepertoryID < nbsurroundingID)
                max_y = nbsurroundingID;
            max_page = (max_y - 1) / maxperpage;

            // update cursor and pages
            // check for x over/underflow
            if (cursor.x < 0) {
                cursor.x = 1;
                page     = Decrement(page, max_page);
                ESP_LOGE(TAG, "underflow x");
            }
            if (cursor.x > 1) {
                cursor.x = 0;
                page     = Increment(page, max_page);
                ESP_LOGE(TAG, "overflow x");
            }

            // update the number of entries shown on screen
            // if there are some stored IDs on the page
            if (nbrepertoryID - page * maxperpage > 0) {
                // if there are more than can fit on the page, page is full
                if (nbrepertoryID - page * maxperpage > maxperpage)
                    nb_item_rep = maxperpage;
                else  // show the nb left on the page
                    nb_item_rep = nbrepertoryID - page * maxperpage;
            } else
                nb_item_rep = 0;

            if (nbsurroundingID - page * maxperpage > 0) {
                if (nbsurroundingID - page * maxperpage > maxperpage)
                    nb_item_sur = maxperpage;
                else
                    nb_item_sur = nbsurroundingID - page * maxperpage;
            } else
                nb_item_sur = 0;

            // check for y over/underflow
            if (cursor.x == 0 && cursor.y >= nb_item_rep) {
                cursor.y = 0;
                ESP_LOGE(TAG, "repertory overflow y");
            }
            if (cursor.x == 1 && cursor.y >= nb_item_sur) {
                cursor.y = 0;
                ESP_LOGE(TAG, "surroundingID overflow y");
            }
            if (cursor.x == 0 && cursor.y < 0) {
                cursor.y = nb_item_rep - 1;
                ESP_LOGE(TAG, "repertory underflow y");
            }
            if (cursor.x == 1 && cursor.y < 0) {
                cursor.y = nb_item_sur - 1;
                ESP_LOGE(TAG, "surroundingID underflow y");
            }

            cursor.yabs = cursor.y + page * maxperpage;

            if (log) {
                ESP_LOGE(TAG, "after logic");
                ESP_LOGE(TAG, "cursor.x: %d", cursor.x);
                ESP_LOGE(TAG, "cursor.y: %d", cursor.y);
                ESP_LOGE(TAG, "nbrepertoryID: %d", nbrepertoryID);
                ESP_LOGE(TAG, "nbsurroundingID: %d", nbsurroundingID);
                ESP_LOGE(TAG, "nb_item_rep: %d", nb_item_rep);
                ESP_LOGE(TAG, "nb_item_sur: %d", nb_item_sur);
                ESP_LOGE(TAG, "page: %d", page);
                ESP_LOGE(TAG, "max_page: %d", max_page);
                ESP_LOGE(TAG, "cursor.yabs: %d", cursor.yabs);
            }

            // get content to display
            for (int i = page * maxperpage; i < nb_item_rep + page * maxperpage; i++)
                GetRepertoire(repertoryIDlist[i], repertory_mac[i], i);

            if (log) {
                for (int i = page * maxperpage; i < nb_item_rep + page * maxperpage; i++) {
                    ESP_LOGE(TAG, "Display after get: repertory name: %s", repertoryIDlist[i]);
                    ESP_LOGE(
                        TAG,
                        "Display after get: repertory MAC: %d:%d:%d:%d:%d:%d:%d:%d",
                        repertory_mac[i][0],
                        repertory_mac[i][1],
                        repertory_mac[i][2],
                        repertory_mac[i][3],
                        repertory_mac[i][4],
                        repertory_mac[i][5],
                        repertory_mac[i][6],
                        repertory_mac[i][7]
                    );
                }
                for (int i = page * maxperpage; i < nb_item_sur + page * maxperpage; i++) {
                    ESP_LOGE(TAG, "Display after get: surrounding name: %s", surroundingIDlist[i]);
                    ESP_LOGE(
                        TAG,
                        "Display after get: surrounding MAC: %d:%d:%d:%d:%d:%d:%d:%d",
                        surrounding_mac[i][0],
                        surrounding_mac[i][1],
                        surrounding_mac[i][2],
                        surrounding_mac[i][3],
                        surrounding_mac[i][4],
                        surrounding_mac[i][5],
                        surrounding_mac[i][6],
                        surrounding_mac[i][7]
                    );
                }
            }
            // disable add / remove
            if ((cursor.x == 0 && cursor.y > (nb_item_rep - 1)) || (cursor.x == 1 && cursor.y > (nb_item_sur - 1))) {
                configure_keyboard_press(keyboard_event_queue, SWITCH_5, false);
                addrmflag = 0;
            } else {
                configure_keyboard_press(keyboard_event_queue, SWITCH_5, true);
                addrmflag = 1;
            }

            Display_repertoire(
                nbrepertoryID,
                nbsurroundingID,
                nb_item_rep,
                nb_item_sur,
                repertoryIDlist,
                surroundingIDlist,
                repertory_mac,
                surrounding_mac,
                cursor,
                show_name_or_mac,
                page,
                max_page,
                addrmflag,
                _app
            );
            displayflag = 0;
        }

        // upon receiving a message
        if (xQueueReceive(repertoire_queue, &message, pdMS_TO_TICKS(1)) == pdTRUE) {
            badge_message_repertoire_t* ts                          = (badge_message_repertoire_t*)message.data;
            char                        inboundnick[nicknamelength] = "";
            uint8_t                     _inbound_mac[8];
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

            int     flag_already_exist    = 0;
            int     flag_line_repertory   = 0;
            int     flag_line_surrounding = 0;
            char    stored_nick[nicknamelength];
            uint8_t stored_mac[8];

            for (int i = 0; i < nbrepertoryID; i++) {
                GetRepertoire(stored_nick, stored_mac, i);
                for (int y = 0; y < 8; y++) {
                    if (repertory_mac[i][y] == _inbound_mac[y])
                        flag_line_repertory++;
                }
                // if known, update nickname in case it changed
                if (flag_line_repertory == 8) {
                    flag_already_exist = 1;
                    if (strcmp(stored_nick, inboundnick) != 0) {
                        StoreRepertoire(inboundnick, _inbound_mac, i);
                        if (log)
                            ESP_LOGE(TAG, "replaced nickanme %s with %s", stored_nick, inboundnick);
                    }
                }
                flag_line_repertory = 0;
            }

            for (int i = 0; i < nbsurroundingID; i++) {
                for (int y = 0; y < 8; y++) {
                    if (surrounding_mac[i][y] == _inbound_mac[y])
                        flag_line_surrounding++;
                }
                if (flag_line_surrounding == 8) {
                    flag_already_exist = 1;
                    strcpy(surroundingIDlist[i], inboundnick);
                }
                flag_line_surrounding = 0;
            }

            // add incoming message to surrounding
            if ((!flag_already_exist) && (nbsurroundingID < maxIDsurrounding)) {
                strcpy(surroundingIDlist[nbsurroundingID], inboundnick);
                for (int i = 0; i < 8; i++) surrounding_mac[nbsurroundingID][i] = _inbound_mac[i];
                nbsurroundingID++;
            }
            displayflag = 1;
        }

        if ((xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(9)) == pdTRUE)) {
            ESP_LOGE(TAG, "loop");
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1:
                            configure_keyboard_rotate_disable(keyboard_event_queue);
                            esp_err_t err = badge_comms_remove_listener(repertoire_queue);
                            if (log) {
                                ESP_LOGI(TAG, "unsubscription result: %s", esp_err_to_name(err));
                                ESP_LOGI(TAG, "Exit");
                            }
                            vTaskDelay(pdMS_TO_TICKS(100));
                            switch (_app) {
                                case 1:
                                    configure_keyboard_kb(keyboard_event_queue, tempkbsettings);
                                    char    nameget[32] = "";
                                    uint8_t macget[8]   = {0, 0, 0, 0, 0, 0, 0, 0};
                                    GetRepertoire(nameget, macget, cursor.yabs);
                                    TargetAddress = (macget[6] << 8) | macget[7];
                                    if (log) {
                                        ESP_LOGE(TAG, "repertory ID of target: %d", cursor.yabs);
                                        ESP_LOGE(TAG, "MAC 6: %02x", macget[6]);
                                        ESP_LOGE(TAG, "MAC 7: %02x", macget[7]);
                                        ESP_LOGE(TAG, "TargetAddress: %04x", TargetAddress);
                                    }
                                    return screen_home;
                                    break;
                                default: return screen_home; break;
                            }
                            break;
                        case SWITCH_2:
                            // debug reset 10 first field repertory
                            // SettNBrepertoryID(0);
                            // nbrepertoryID = 0;
                            // for (int i = 0; i < 10; i++) {
                            //     char    name[32] = "";
                            //     uint8_t maca[8];
                            //     for (int y = 0; y < 8; y++) maca[y] = 0;

                            //     StoreRepertoire(name, maca, i);
                            // }
                            break;
                        case SWITCH_L2: cursor.x--; break;
                        case SWITCH_R2: cursor.x++; break;
                        case SWITCH_L3: cursor.y++; break;
                        case SWITCH_R3: cursor.y--; break;
                        case SWITCH_3: break;
                        case SWITCH_4:
                            if (!show_name_or_mac)
                                show_name_or_mac = 1;
                            else
                                show_name_or_mac = 0;
                            break;
                        case SWITCH_5:
                            switch (cursor.x) {
                                case remove:
                                    if (nbrepertoryID == 0)
                                        break;
                                    char promt[128] = "Are you sure you want to remove ";
                                    strcat(promt, repertoryIDlist[cursor.yabs]);
                                    strcat(promt, "?");
                                    if (Screen_Confirmation(promt, application_event_queue, keyboard_event_queue)) {
                                        for (int i = cursor.yabs; i < nbrepertoryID; i++) {
                                            char    nameget[32] = "";
                                            uint8_t macget[8]   = {0, 0, 0, 0, 0, 0, 0, 0};
                                            GetRepertoire(nameget, macget, i + 1);
                                            StoreRepertoire(nameget, macget, i);
                                        }
                                        nbrepertoryID--;
                                        SettNBrepertoryID(nbrepertoryID);
                                        // If you remove the first surrouding contact, no underflow on the last page
                                        if (cursor.yabs)
                                            cursor.y--;
                                    }
                                    break;
                                case add:
                                    StoreRepertoire(
                                        surroundingIDlist[cursor.yabs],
                                        surrounding_mac[cursor.yabs],
                                        nbrepertoryID
                                    );
                                    for (int i = 0; i < 8; i++) surrounding_mac[cursor.yabs][i] = 0;
                                    strcpy(surroundingIDlist[cursor.yabs], "");
                                    nbsurroundingID--;
                                    for (int i = cursor.yabs; i < nbsurroundingID; i++) {
                                        strcpy(surroundingIDlist[i], surroundingIDlist[i + 1]);
                                        for (int y = 0; y < 8; y++) surrounding_mac[i][y] = surrounding_mac[i + 1][y];
                                    }
                                    nbrepertoryID++;
                                    SettNBrepertoryID(nbrepertoryID);
                                    // If you add the first surrouding contact, doesn't underflow on the last page
                                    if (cursor.yabs)
                                        cursor.y--;
                                    break;
                            }
                            break;
                        default: break;
                    }
                    displayflag = 1;
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type); break;
            }
        }
    }
}
