#include "screen_repertoire.h"
#include "application.h"
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

void Display_repertoire(
    int  _cursor_x,
    int  _cursor_y,
    int  _nbrepertoryID,
    int  _nbsurroundingID,
    int  _max_y,
    char _repertoryIDlist[maxIDrepertoire][nicknamelenght],
    char _surroundingIDlist[maxIDrepertoire][nicknamelenght]
) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();

    int title_o_y = 4;

    int maxperpage = 8;
    int box_y      = 11;
    int box_x      = 120;
    int box_o_y    = 20;

    int pagefooter_o_x = 21;

    int text_rep_x = gfx->height / 4;
    int text_sur_x = gfx->height * 3 / 4;
    int text_cursor_x;


    pax_background(gfx, WHITE);
    AddSWtoBuffer("Exit", "", "", "", "");
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
            dims = pax_text_size(font1, fontsizeS, _repertoryIDlist[i + page * maxperpage]);
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

screen_t screen_repertoire_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    char repertoryIDlist[maxIDrepertoire][nicknamelenght];
    for (int i = 0; i < maxIDrepertoire; i++) strcpy(repertoryIDlist[i], "");
    char surroundingIDlist[maxIDrepertoire][nicknamelenght];
    for (int i = 0; i < maxIDrepertoire; i++) strcpy(surroundingIDlist[i], "");

    strcpy(repertoryIDlist[0], "George");
    strcpy(repertoryIDlist[1], "Michael");
    strcpy(surroundingIDlist[0], "Florian");
    strcpy(surroundingIDlist[1], "Clown");
    int nbrepertoryID   = 2;
    int nbsurroundingID = 1;
    int max_y           = 2;
    int cursor_x        = 0;
    int cursor_y        = 0;
    int exit            = 0;


    if (nbsurroundingID)
        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_2, true);
    else
        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_2, false);

    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, false, false, false, true);
    if (nbsurroundingID)
        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_2, true);
    else
        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_2, false);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_3, true);
    DebugKeyboardSettings();
    Display_repertoire(cursor_x, cursor_y, nbrepertoryID, nbsurroundingID, max_y, repertoryIDlist, surroundingIDlist);

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1:
                            configure_keyboard_rotate_disable(keyboard_event_queue);
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
                        case SWITCH_4: break;
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
                                        }
                                        if (cursor_y)
                                            cursor_y--;
                                    }

                                    break;
                                case add:
                                    strcpy(repertoryIDlist[nbrepertoryID], surroundingIDlist[cursor_y]);
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
                    if (nbsurroundingID)
                        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_2, true);
                    else
                        configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_2, false);
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
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
