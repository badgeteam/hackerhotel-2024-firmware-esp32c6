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


extern const uint8_t diamondl_png_start[] asm("_binary_diamondl_png_start");
extern const uint8_t diamondl_png_end[] asm("_binary_diamondl_png_end");
extern const uint8_t diamondr_png_start[] asm("_binary_diamondr_png_start");
extern const uint8_t diamondr_png_end[] asm("_binary_diamondr_png_end");
extern const uint8_t diamondt_png_start[] asm("_binary_diamondt_png_start");
extern const uint8_t diamondt_png_end[] asm("_binary_diamondt_png_end");
extern const uint8_t diamondb_png_start[] asm("_binary_diamondb_png_start");
extern const uint8_t diamondb_png_end[] asm("_binary_diamondb_png_end");

extern const uint8_t library_png_start[] asm("_binary_library_png_start");
extern const uint8_t library_png_end[] asm("_binary_library_png_end");

static const char* TAG = "library";

void DisplayLibraryEntry(int cursor);

const char library_items_name[Nb_item_library][60] = {
    "Samuel Morse",
    "Queen Victoria's Message",
    "Transatlantic Telegraph Cables",
    "SS Carondelet",
    "Anatole Deibler",

    "The First Telegraph Post Office",
    "Entry 7",
    "Entry 8",
    "Entry 9",
    "Entry 10",

    "Entry 11",
    "Entry 12",
    "Entry 13",
};

const uint8_t library_pos[Nb_item_library][2] = {
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

const char library_item_content[Nb_item_library][5][600] = {
    // entry 1
    {"Samuel F.B. Morse developed an electric telegraph (1832–35) and then invented, with his friend Alfred Vail, the "
     "Morse Code (1838). The latter is a system for representing letters of the alphabet, numerals, and punctuation "
     "marks by arranging dots, dashes, and spaces. The codes are transmitted through either a telegraph machine or "
     "visual signals."},
    // entry 2
    {"To the President of the United States, Washington:—The Queen desires to congratulate the President upon the "
     "successful completion of this great international work, in which the Queen has taken the deepest interest.",

     "The "
     "Queen is convinced that the President will join with her in fervently hoping that the electric cable which now "
     "connects Great Britain with the United States will prove an additional link between the nations, whose "
     "friendship is founded upon their common interest and reciprocal esteem. The Queen has much pleasure in thus "
     "communicating with the President, and renewing to him her wishes for the prosperity of the United States."},
    // entry 3
    {"In an 1838 letter to Francis O.J. Smith in 1838, Morse wrote: ‘This mode of instantaneous communication must "
     "inevitably become an instrument of immense power, to be wielded for good or for evil, as it shall be properly or "
     "improperly directed.’ Transatlantic Telegraph Cables were undersea cables running under the Atlantic Ocean for "
     "telegraph communications.",

     " The first communications occurred on August 16th 1858, but the line speed was poor, "
     "and efforts to improve it caused the cable to fail after three weeks."
     "In September 1858, after several days of progressive deterioration of the insulation, the cable failed "
     "altogether. The reaction to the news was tremendous. Some writers even hinted that the line was a mere hoax; "
     "others pronounced it a stock-exchange speculation. "},
    // entry 4
    {"SS Carondelet was an immigrant ship, active in 1877 and 1878, that transported immigrants from Havana to New "
     "York City.The declaration has been preserved: “I, A. C. Burrows, Master of the S.S. Carondelet do solemnly, "
     "sincerely, and truly swear that the following List or Manifest, subscribed by me, and now delivered by me to the "
     "Collector of the Customs of the Collection District of New York,",

     "is a full and perfect list of all the "
     "passengers taken on board of the said S.S. Carondelet at Nassau NP from which port said S.S. Carondelet has now "
     "arrived, and that on said list is truly designated the age, the sex, and the occupation of each of said "
     "passengers, the part of the vessel occupied by each during the passage,",

     "the country to which each belongs, and "
     "also the country of which it is intended by each to become an inhabitant,",

     "and that said List or Manifest truly "
     "sets forth the number of said passengers who have died on said voyage, and the names and ages of those who "
     "died.\n\n So help me God. \n\n A. C. Burrows. \n\n 3rd of February, 1878."},
    // entry 5
    {"In April 1844 Morse set up a small laboratory in a first floor committee room in the Senate wing of the Capitol "
     "across from the Old Supreme Court chamber. On May 24, 1844, after weeks of testing, Morse gathered a small "
     "group—reportedly in the Supreme Court chamber, but more likely in the committee room—to send the first message "
     "all the way to Baltimore.",

     "Morse tapped out the message suggested to him by Ellsworth’s daughter Annie: “What "
     "Hath God Wrought.” Moments later an identical message was returned from Morse’s partner Alfred Vail in "
     "Baltimore. The experiment was a success.",

     "The Post Office assumed control of the Washington-Baltimore telegraph "
     "line in October and opened it to the public on a fee-basis, but Congress declined to fund the line’s extension "
     "or to purchase Morse’s patents as he had hoped. The tenacious Morse instead secured private investment and "
     "licensing, and by 1850, more than 10,000 miles of telegraph wire stretched across the nation."},
    // entry 6
    {"In April 1844 Morse set up a small laboratory in a first floor committee room in the Senate wing of the Capitol "
     "across from the Old Supreme Court chamber. On May 24, 1844, after weeks of testing, Morse gathered a small "
     "group—reportedly in the Supreme Court chamber, but more likely in the committee room—to send the first message "
     "all the way to Baltimore.",

     "Morse tapped out the message suggested to him by Ellsworth’s daughter Annie: “What "
     "Hath God Wrought.” Moments later an identical message was returned from Morse’s partner Alfred Vail in "
     "Baltimore. The experiment was a success.",

     "The Post Office assumed control of the Washington-Baltimore telegraph "
     "line in October and opened it to the public on a fee-basis, but Congress declined to fund the line’s extension "
     "or to purchase Morse’s patents as he had hoped. The tenacious Morse instead secured private investment and "
     "licensing, and by 1850, more than 10,000 miles of telegraph wire stretched across the nation."},
    {"Entry 7"},
    {"Entry 8"},
    {"Entry 9"},
    {"Entry 10"},

    {"Entry 11"},
    {"Entry 12"},
    {"Entry 13"},
};

const uint8_t library_item_nbpage[Nb_item_library] = {0, 1, 1, 3, 2, 2, 0, 0, 0, 0, 0, 0, 0};

pax_vec1_t DrawLibraryContent(int _yoffset, const char* _message) {
    pax_buf_t* gfx  = bsp_get_gfx_buffer();
    pax_vec1_t dims = {
        .x = 999,
        .y = 999,
    };
    pax_vec1_t cursorloc = {
        .x = 0,
        .y = 0,
    };

    int _centered  = 1;
    int _cursor    = 1;
    int maxnblines = 12;

    ESP_LOGE(TAG, "Unhandled event type %s", _message);
    ESP_LOGE(TAG, "Unhandled event type %d", strlen(_message));
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
                if (nblines >= maxnblines) {
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
    ESP_LOGE(TAG, "screen_library_content");
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, false, false, true);
    int page = 0;
    Display_library_content(keyboard_event_queue, cursor, page);
    while (1) {
        event_t event = {0};
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
    configure_keyboard_presses(keyboard_event_queue, true, false, false, false, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_3, true);
    DisplayLibraryEntry(cursor);

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case ROTATION_L3: cursor = Decrement(cursor, Nb_item_library); break;
                        case ROTATION_R3: cursor = Increment(cursor, Nb_item_library); break;
                        case SWITCH_4: break;
                        case SWITCH_5:
                            screen_library_content(application_event_queue, keyboard_event_queue, cursor);
                            break;
                        default: break;
                    }
                    ESP_LOGE(TAG, "Cursor %d", cursor);
                    DisplayLibraryEntry(cursor);
                    configure_keyboard_presses(keyboard_event_queue, true, false, false, false, true);
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
    // pax_center_text(gfx, BLACK, font1, fontsizeS, gfx->height / 2, 87, library_items_name[cursor]);
    WallofTextnb_line(87, library_items_name[cursor], 1, 10, 140);
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
