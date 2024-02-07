#include "screen_library.h"
#include "application.h"
#include "bsp.h"
#include "coprocessor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screen_library.h"
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>


extern uint8_t const diamondl_png_start[] asm("_binary_diamondl_png_start");
extern uint8_t const diamondl_png_end[] asm("_binary_diamondl_png_end");
extern uint8_t const diamondr_png_start[] asm("_binary_diamondr_png_start");
extern uint8_t const diamondr_png_end[] asm("_binary_diamondr_png_end");
extern uint8_t const diamondt_png_start[] asm("_binary_diamondt_png_start");
extern uint8_t const diamondt_png_end[] asm("_binary_diamondt_png_end");
extern uint8_t const diamondb_png_start[] asm("_binary_diamondb_png_start");
extern uint8_t const diamondb_png_end[] asm("_binary_diamondb_png_end");

extern uint8_t const library_png_start[] asm("_binary_library_png_start");
extern uint8_t const library_png_end[] asm("_binary_library_png_end");

static char const * TAG = "library";

void DisplayLibraryEntry(int cursor);

char const library_items_name[Nb_item_library][30] = {
    "Entry 1",
    "Entry 2",
    "Entry 3",
    "Entry 4",
    "Entry 5",

    "Entry 6",
    "Entry 7",
    "Entry 8",
    "Entry 9",
    "Entry 10",

    "Entry 11",
    "Entry 12",
    "Entry 13",
};

uint8_t const library_pos[Nb_item_library][2] = {
    {74, 38},
    {86, 34},
    {98, 37},
    {111, 38},
    {122, 34},

    {136, 37},
    {149, 34},
    {160, 38},
    {172, 38},
    {184, 38},

    {196, 38},
    {210, 37},
    {222, 37},
};

char const library_item_content[Nb_item_library][5][300] = {
    {"Entry 1.1", "Entry 1.2", "Entry 1.3"},

    {"Entry 2"},

    {"Entry 3.1", "Entry 3.2"},

    {"Entry 4"},
    {"Entry 5"},

    {"Entry 6"},
    {"Entry 7"},
    {"Entry 8"},
    {"Entry 9"},
    {"Entry 10"},

    {"Entry 11"},
    {"Entry 12"},
    {"Entry 13"},
};

uint8_t const library_item_nbpage[Nb_item_library] = {2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

pax_vec1_t DrawLibraryContent(int _yoffset, char const * _message) {
    pax_buf_t* gfx  = bsp_get_gfx_buffer();
    pax_vec1_t dims = {
        .x = 999,
        .y = 999,
    };
    pax_vec1_t cursorloc = {
        .x = 0,
        .y = 0,
    };

    int _centered = 1;
    int _cursor   = 1;

    ESP_LOGE(TAG, "Unhandled event type %s", _message);
    ESP_LOGE(TAG, "Unhandled event type %d", strlen(_message));
    // char message[128]       = "The quick brown fox jumps over the lazy dog, The quick brie da";  // message to parse
    char linetodisplay[128] = "";
    char Words[64];  // Parsed Word
    int  _xoffset       = 6;
    int  _maxlinelength = gfx->height - _xoffset * 2;

    // counters
    int nblines     = 0;
    int j           = 0;
    int wordcount   = 0;
    int cursorfound = 0;

    if (_cursor == 0) {
        cursorloc.x = _xoffset;
        cursorloc.y = _yoffset;
        cursorfound = 1;
    }

    // pax_background(gfx, WHITE);

    // goes through each character until the end of the string (unless nblines >= maxnblines)
    for (int i = 0; i <= (strlen(_message)); i++) {
        // If space or end of string found, process word/lines
        if (_message[i] == ' ' || _message[i] == '\0') {
            Words[j] = '\0';  // end of string terminate the word
            j        = 0;
            strcat(linetodisplay, Words);  // add word to linetodisplay
            strcat(linetodisplay, " ");    // and a space that was not parsed

            // if longer than maxlinelenght, go to the next line
            dims = pax_text_size(font1, fontsizeS, linetodisplay);
            if ((int)dims.x > _maxlinelength || _message[i] == '\0') {
                if (_message[i] != '\0')
                    linetodisplay[strlen(linetodisplay) - (strlen(Words) + 2)] =
                        '\0';  // remove the last word and 2 spaces

                // center the text
                if (_centered) {
                    dims     = pax_text_size(font1, fontsizeS, linetodisplay);
                    _xoffset = gfx->height / 2 - (int)(dims.x / 2);
                }

                ESP_LOGE(TAG, "line to display length: %d", strlen(linetodisplay));

                pax_draw_text(
                    gfx,
                    BLACK,
                    font1,
                    fontsizeS,
                    _xoffset,
                    _yoffset + nblines * fontsizeS,
                    linetodisplay
                );  // displays the line

                // calculate the cursor position

                if ((_cursor < strlen(linetodisplay)) && !cursorfound) {
                    ESP_LOGE(TAG, "cursor on: %c", linetodisplay[_cursor]);
                    linetodisplay[_cursor] = '\0';
                    cursorloc              = pax_text_size(font1, fontsizeS, linetodisplay);
                    cursorloc.x            = cursorloc.x + _xoffset;
                    cursorloc.y            = _yoffset + nblines * fontsizeS;
                    cursorfound            = 1;
                } else {
                    _cursor = _cursor - strlen(linetodisplay);
                    if (!_cursor) {
                        cursorloc.x = _xoffset;
                        cursorloc.y = _yoffset + (nblines + 1) * fontsizeS;
                        cursorfound = 1;
                    }
                }
                ESP_LOGE(TAG, "cursor x: %f", cursorloc.x);
                ESP_LOGE(TAG, "cursor y: %f", cursorloc.y);


                strcpy(linetodisplay, Words);  // Add the latest word that was parsed to the next line
                strcat(linetodisplay, " ");
                nblines++;
                if (nblines >= 8) {
                    break;
                }
            }

            // If it is the last word of the string
            // if (_message[i] == '\0') {
            //     pax_draw_text(gfx, BLACK, font1, fontsizeS, _xoffset, _yoffset + nblines * fontsizeS, linetodisplay);
            // }
            wordcount++;  // Move to the next word

        } else {
            Words[j] = _message[i];  // Store the character into newString
            j++;                     // Move to the next character within the word
        }
    }
    return cursorloc;
}

void Display_library_content(QueueHandle_t keyboard_event_queue, int cursor, int page) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    AddSWtoBufferLR("", "");
    if (page > 0) {
        AddOneTextSWtoBuffer(SWITCH_1, "Previous");
        configure_keyboard_press(keyboard_event_queue, SWITCH_1, true);
    } else
        configure_keyboard_press(keyboard_event_queue, SWITCH_1, false);
    if (library_item_nbpage[cursor] > page) {
        AddOneTextSWtoBuffer(SWITCH_5, "Next");
        // configure_keyboard_press(keyboard_event_queue, SWITCH_5, true);
    } else {
        AddOneTextSWtoBuffer(SWITCH_5, "Continue");
    }
    DrawLibraryContent(Y_offset_library, library_item_content[cursor][page]);
    bsp_display_flush();
}

screen_t screen_library_content(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int cursor) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, false, false, true);
    int page = 0;
    Display_library_content(keyboard_event_queue, cursor, page);
    while (1) {
        event_t event = {0};
        ESP_LOGE(TAG, "screen_library_content");
        if (xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1:
                            if (page)
                                page--;
                            break;
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5:
                            if (page < library_item_nbpage[cursor])
                                page++;
                            else
                                return screen_library;
                            break;
                        default: break;
                    }
                    Display_library_content(keyboard_event_queue, cursor, page);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}


screen_t screen_library_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    ESP_LOGE(TAG, "Enter screen_home_entry");
    bsp_apply_lut(lut_4s);
    int cursor = 0;
    // update the keyboard event handler settings
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, false, false, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_3, true);
    DisplayLibraryEntry(cursor);

    while (1) {
        event_t event = {0};
        ESP_LOGE(TAG, "test screen_home_entry");
        if (xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return 0; break;
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_L3: cursor = Decrement(cursor, Nb_item_library); break;
                        case SWITCH_R3: cursor = Increment(cursor, Nb_item_library); break;
                        case SWITCH_4: break;
                        case SWITCH_5:
                            screen_library_content(application_event_queue, keyboard_event_queue, cursor);
                            break;
                        default: break;
                    }
                    ESP_LOGE(TAG, "Cursor %d", cursor);
                    DisplayLibraryEntry(cursor);
                    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_3, true);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

void DisplayLibraryEntry(int cursor) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_insert_png_buf(gfx, library_png_start, library_png_end - library_png_start, 0, 0, 0);
    AddOneTextSWtoBuffer(SWITCH_1, "Exit");
    DrawArrowHorizontal(SWITCH_3);
    AddOneTextSWtoBuffer(SWITCH_5, "Select");
    pax_center_text(gfx, BLACK, font1, fontsizeS * 2, gfx->height / 2, 87, library_items_name[cursor]);
    // pax_insert_png_buf(
    //     gfx,
    //     diamondb_png_start,
    //     diamondb_png_end - diamondb_png_start,
    //     library_pos[cursor][0] - 4,
    //     library_pos[cursor][1] - 8,
    //     0
    // );
    pax_insert_png_buf(
        gfx,
        diamondt_png_start,
        diamondt_png_end - diamondt_png_start,
        library_pos[cursor][0] - 4,
        65 + 8,
        0
    );
    bsp_display_flush();
}
