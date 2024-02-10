#include "application.h"
#include "application_settings.h"
#include "badge-communication-protocol.h"
#include "bsp.h"
#include "driver/dedic_gpio.h"
#include "driver/gpio.h"
#include "epaper.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hextools.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "pax_types.h"
#include "resources.h"
#include "riscv/rv_utils.h"
#include "screen_home.h"
#include "screen_pointclick.h"
#include "screen_repertoire.h"
#include "screens.h"
#include "sdkconfig.h"
#include "sdmmc_cmd.h"
#include "soc/gpio_struct.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_random.h>
#include <esp_system.h>
#include <esp_vfs_fat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <sdkconfig.h>
#include <sdmmc_cmd.h>
#include <string.h>
#include <time.h>

extern const uint8_t border1_png_start[] asm("_binary_border1_png_start");
extern const uint8_t border1_png_end[] asm("_binary_border1_png_end");
extern const uint8_t border2_png_start[] asm("_binary_border2_png_start");
extern const uint8_t border2_png_end[] asm("_binary_border2_png_end");
extern const uint8_t switchframe1_png_start[] asm("_binary_switchframe1_png_start");
extern const uint8_t switchframe1_png_end[] asm("_binary_switchframe1_png_end");
extern const uint8_t switchframe2_png_start[] asm("_binary_switchframe2_png_start");
extern const uint8_t switchframe2_png_end[] asm("_binary_switchframe2_png_end");
extern const uint8_t border2lsw_png_start[] asm("_binary_border2lsw_png_start");
extern const uint8_t border2lsw_png_end[] asm("_binary_border2lsw_png_end");
extern const uint8_t border2lrsw_png_start[] asm("_binary_border2lrsw_png_start");
extern const uint8_t border2lrsw_png_end[] asm("_binary_border2lrsw_png_end");

extern const uint8_t diamondl_png_start[] asm("_binary_diamondl_png_start");
extern const uint8_t diamondl_png_end[] asm("_binary_diamondl_png_end");
extern const uint8_t diamondr_png_start[] asm("_binary_diamondr_png_start");
extern const uint8_t diamondr_png_end[] asm("_binary_diamondr_png_end");

extern const uint8_t b_arrow1_png_start[] asm("_binary_b_arrow1_png_start");
extern const uint8_t b_arrow1_png_end[] asm("_binary_b_arrow1_png_end");
extern const uint8_t b_arrow2_png_start[] asm("_binary_b_arrow2_png_start");
extern const uint8_t b_arrow2_png_end[] asm("_binary_b_arrow2_png_end");

// 272 x 9
extern const uint8_t squi1_png_start[] asm("_binary_squi1_png_start");
extern const uint8_t squi1_png_end[] asm("_binary_squi1_png_end");
// 286 x 13
extern const uint8_t squi2_png_start[] asm("_binary_squi2_png_start");
extern const uint8_t squi2_png_end[] asm("_binary_squi2_png_end");

event_t kbsettings;

const int telegraph_X[20] = {0, -8, 8, -16, 0, 16, -24, -8, 8, 24, -24, -8, 8, 24, -16, 0, 16, -8, 8, 0};
const int telegraph_Y[20] = {12, 27, 27, 42, 42, 42, 57, 57, 57, 57, 71, 71, 71, 71, 86, 86, 86, 101, 101, 116};

static const char* TAG = "application utilities";

void DisplayError(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, const char* errorstr) {
    event_t    tempkbsettings = kbsettings;
    pax_buf_t* gfx            = bsp_get_gfx_buffer();

    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);

    int text_x = gfx->height / 2;
    int text_y = 40;

    pax_background(gfx, WHITE);
    AddSWtoBuffer("", "", "", "", "");
    pax_center_text(gfx, BLACK, font1, fontsizeS, text_x, text_y, errorstr);
    bsp_display_flush();

    InitKeyboard(keyboard_event_queue);

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        default:
                            configure_keyboard_kb(keyboard_event_queue, tempkbsettings);
                            return;
                            break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type); break;
            }
        }
    }
}

void draw_squi1(int y) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    if (y < 4)
        y = 0;
    else
        y = y - 4;
    pax_insert_png_buf(gfx, squi1_png_start, squi1_png_end - squi1_png_start, (296 - 272) / 2, y, 0);
}

void draw_squi2(int y) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    if (y < 6)
        y = 0;
    else
        y = y - 6;
    pax_insert_png_buf(gfx, squi2_png_start, squi2_png_end - squi2_png_start, (296 - 286) / 2, y, 0);
}

void DrawArrowVertical(int _sw) {
    pax_insert_png_buf(
        bsp_get_gfx_buffer(),
        b_arrow2_png_start,
        b_arrow2_png_end - b_arrow2_png_start,
        60 * _sw + 7,
        118,
        0
    );
}

void DrawArrowHorizontal(int _sw) {
    pax_insert_png_buf(
        bsp_get_gfx_buffer(),
        b_arrow1_png_start,
        b_arrow1_png_end - b_arrow1_png_start,
        60 * _sw + 7,
        118,
        0
    );
}

void Addborder1toBuffer(void) {

    pax_insert_png_buf(bsp_get_gfx_buffer(), border1_png_start, border1_png_end - border1_png_start, 0, 0, 0);
}

void Addborder2toBuffer(void) {

    pax_insert_png_buf(bsp_get_gfx_buffer(), border2_png_start, border2_png_end - border2_png_start, 0, 0, 0);
}

void AddSWborder1toBuffer(void) {

    Addborder1toBuffer();
    pax_insert_png_buf(
        bsp_get_gfx_buffer(),
        switchframe1_png_start,
        switchframe1_png_end - switchframe1_png_start,
        0,
        127 - 10,
        0
    );
}

void AddSWborder2toBuffer(void) {

    Addborder2toBuffer();
    pax_insert_png_buf(
        bsp_get_gfx_buffer(),
        switchframe2_png_start,
        switchframe2_png_end - switchframe2_png_start,
        0,
        127 - 11,
        0
    );
}

void AddSWtoBuffer(const char* SW1str, const char* SW2str, const char* SW3str, const char* SW4str, const char* SW5str) {
    Addborder2toBuffer();
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_insert_png_buf(gfx, switchframe2_png_start, switchframe2_png_end - switchframe2_png_start, 0, 127 - 12, 0);
    int gapx = 60;
    int o_x  = 28;
    int o_y  = 118;
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 0, o_y, SW1str);
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 1, o_y, SW2str);
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 2, o_y, SW3str);
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 3, o_y, SW4str);
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 4, o_y, SW5str);
}

void AddSWtoBufferL(const char* SW1str) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_insert_png_buf(gfx, border2lsw_png_start, border2lsw_png_end - border2lsw_png_start, 0, 0, 0);
    int gapx = 60;
    int o_x  = 28;
    int o_y  = 118;
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 0, o_y, SW1str);
}

void AddSWtoBufferLR(const char* SW1str, const char* SW5str) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_insert_png_buf(gfx, border2lrsw_png_start, border2lrsw_png_end - border2lrsw_png_start, 0, 0, 0);
    int gapx = 60;
    int o_x  = 28;
    int o_y  = 118;
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 0, o_y, SW1str);
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 4, o_y, SW5str);
}

void AddOneTextSWtoBuffer(int _SW, const char* SWstr) {

    pax_buf_t* gfx  = bsp_get_gfx_buffer();
    int        gapx = 60;
    int        o_x  = 28;
    int        o_y  = 118;
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * _SW, o_y, SWstr);
}

void Justify_right_text(
    pax_buf_t* buf, pax_col_t color, const pax_font_t* font, float font_size, float x, float y, const char* text
) {
    pax_vec1_t dims = {
        .x = 999,
        .y = 999,
    };

    dims         = pax_text_size(font, font_size, text);
    int _xoffset = buf->height - (int)dims.x - x;
    pax_draw_text(buf, color, font, font_size, _xoffset, y, text);
}

// screen that stop the game loop from running and display information until the player press any inputs
int Screen_Information(const char* _prompt, QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    event_t    tempkbsettings = kbsettings;
    pax_buf_t* gfx            = bsp_get_gfx_buffer();

    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);

    int text_x = gfx->height / 2;
    int text_y = 40;

    pax_background(gfx, WHITE);
    AddSWtoBuffer("", "", "", "", "");
    pax_center_text(gfx, BLACK, font1, fontsizeS, text_x, text_y, _prompt);
    bsp_display_flush();

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        // case SWITCH_1:
                        // case SWITCH_2:
                        // case SWITCH_3:
                        // case SWITCH_4:
                        // case SWITCH_5:
                        default:
                            configure_keyboard_kb(keyboard_event_queue, tempkbsettings);
                            return 0;
                            break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type); break;
            }
        }
    }
}

// screen that stop the game loop from running, asking for a prompt and return 1 (yes) or 0 (no)
int Screen_Confirmation(
    const char* _prompt, QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue
) {
    event_t    tempkbsettings = kbsettings;
    pax_buf_t* gfx            = bsp_get_gfx_buffer();

    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, false, false, false, true);

    int text_x = gfx->height / 2;
    int text_y = 40;

    pax_background(gfx, WHITE);
    AddSWtoBuffer("yes", "", "", "", "no");
    pax_center_text(gfx, BLACK, font1, fontsizeS, text_x, text_y, _prompt);
    bsp_display_flush();

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1:
                            configure_keyboard_kb(keyboard_event_queue, tempkbsettings);
                            return 1;
                            // [[fallthrough]];
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5:
                            configure_keyboard_kb(keyboard_event_queue, tempkbsettings);
                            return 0;
                            // [[fallthrough]];
                        default: break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type); break;
            }
        }
    }
}

int WaitingforOpponent(const char* _prompt, QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();

    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, false, false, false, false);

    int text_x = gfx->height / 2;
    int text_y = 40;

    AddSWtoBufferL("Abort");
    pax_center_text(gfx, BLACK, font1, fontsizeS * 2, text_x, text_y, _prompt);
    bsp_display_flush();
    bsp_set_addressable_led(LED_YELLOW);
    return 1;
}
// Parse _message[] into an array of _nbwords
// and makes them into up to _maxnblines which are _maxlinelength pixel long
// can be centered if the _centered flag is high
void DisplayWallofTextWords(
    int  _fontsize,
    int  _maxlinelength,
    int  _maxnblines,
    int  _nbwords,
    int  _xoffset,
    int  _yoffset,
    char _message[200],
    int  _centered
) {
    // set screen font and buffer
    const pax_font_t* font = pax_font_sky;
    pax_buf_t*        gfx  = bsp_get_gfx_buffer();

    // to verify text length on buffer
    pax_vec1_t dims = {
        .x = 999,
        .y = 999,
    };

    // char message[128]       = "The quick brown fox jumps over the lazy dog, The quick brie da";  // message to parse
    char linetodisplay[128] = "";
    char Words[50][32];  // Array of parsed words

    // counters
    int nblines   = 0;
    int j         = 0;
    int wordcount = 0;

    pax_background(gfx, WHITE);

    // goes through each character until the end of the string (unless nblines >= maxnblines)
    for (int i = 0; i <= (strlen(_message)); i++) {
        // If space or end of string found, process word/lines
        if (_message[i] == ' ' || _message[i] == '\0') {
            Words[wordcount][j] = '\0';  // end of string terminate the word
            j                   = 0;
            // pax_draw_text(gfx, BLACK, font, fontsize, xoffset, wordcount * fontsize, Words[wordcount]);//use to
            // display words
            strcat(linetodisplay, Words[wordcount]);  // add word to linetodisplay
            strcat(linetodisplay, " ");               // and a space that was not parsed

            // if longer than maxlinelength, go to the next line
            dims = pax_text_size(font, _fontsize, linetodisplay);
            if ((int)dims.x > _maxlinelength) {
                linetodisplay[strlen(linetodisplay) - (strlen(Words[wordcount]) + 2)] =
                    '\0';  // remove the last word and 2 spaces

                // center the text
                if (_centered) {
                    dims     = pax_text_size(font, _fontsize, linetodisplay);
                    _xoffset = gfx->height / 2 - (int)(dims.x / 2);
                }

                pax_draw_text(
                    gfx,
                    BLACK,
                    font,
                    _fontsize,
                    _xoffset,
                    _yoffset + nblines * _fontsize,
                    linetodisplay
                );  // displays
                    // the line
                strcpy(
                    linetodisplay,
                    Words[wordcount]
                );  // Add the latest word that was parsed and removed to the next line
                nblines++;
                if (nblines >= _maxnblines) {
                    break;
                }
            }

            // If it is the last word of the string
            if (_message[i] == '\0') {
                pax_draw_text(gfx, BLACK, font, _fontsize, _xoffset, _yoffset + nblines * _fontsize, linetodisplay);
            }
            wordcount++;  // Move to the next word

        } else {
            Words[wordcount][j] = _message[i];  // Store the character into newString
            j++;                                // Move to the next character within the word
        }
    }

    bsp_display_flush();
}

// Parse _message[] into lines
// and makes them into up to _maxnblines which are _maxlinelength pixel long
// can be centered if the _centered flag is high
void DisplayWallofText(
    int _fontsize, int _maxlinelength, int _maxnblines, int _xoffset, int _yoffset, char _message[500], int _centered
) {
    // set screen font and buffer
    const pax_font_t* font = pax_font_sky;
    pax_buf_t*        gfx  = bsp_get_gfx_buffer();

    // to verify text length on buffer
    pax_vec1_t dims = {
        .x = 999,
        .y = 999,
    };

    // char message[128]       = "The quick brown fox jumps over the lazy dog, The quick brie da";  // message to parse
    char linetodisplay[128] = "";
    char Words[64];  // Array of parsed words

    // counters
    int nblines   = 0;
    int j         = 0;
    int wordcount = 0;

    // pax_background(gfx, WHITE);

    // goes through each character until the end of the string (unless nblines >= maxnblines)
    for (int i = 0; i <= (strlen(_message)); i++) {
        // If space or end of string found, process word/lines
        if (_message[i] == ' ' || _message[i] == '\0') {
            Words[j] = '\0';  // end of string terminate the word
            j        = 0;
            // pax_draw_text(gfx, BLACK, font, fontsize, xoffset, wordcount * fontsize, Words[wordcount]);//use to
            // display words
            strcat(linetodisplay, Words);  // add word to linetodisplay
            strcat(linetodisplay, " ");    // and a space that was not parsed

            // if longer than maxlinelength, go to the next line
            dims = pax_text_size(font, _fontsize, linetodisplay);
            if ((int)dims.x > _maxlinelength) {
                linetodisplay[strlen(linetodisplay) - (strlen(Words) + 2)] = '\0';  // remove the last word and 2 spaces

                // center the text
                if (_centered) {
                    dims     = pax_text_size(font, _fontsize, linetodisplay);
                    _xoffset = gfx->height / 2 - (int)(dims.x / 2);
                }

                pax_draw_text(
                    gfx,
                    BLACK,
                    font,
                    _fontsize,
                    _xoffset,
                    _yoffset + nblines * _fontsize,
                    linetodisplay
                );  // displays
                    // the line
                strcpy(linetodisplay,
                       Words);  // Add the latest word that was parsed and removed to the next line
                nblines++;
                if (nblines >= _maxnblines) {
                    break;
                }
            }

            // If it is the last word of the string
            if (_message[i] == '\0') {
                pax_draw_text(gfx, BLACK, font, _fontsize, _xoffset, _yoffset + nblines * _fontsize, linetodisplay);
            }
            wordcount++;  // Move to the next word

        } else {
            Words[j] = _message[i];  // Store the character into newString
            j++;                     // Move to the next character within the word
        }
    }

    // bsp_display_flush();
}

pax_vec1_t WallofText(int _yoffset, const char* _message, int _centered, int _cursor) {
    pax_buf_t* gfx  = bsp_get_gfx_buffer();
    pax_vec1_t dims = {
        .x = 999,
        .y = 999,
    };
    pax_vec1_t cursorloc = {
        .x = 0,
        .y = 0,
    };

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

pax_vec1_t WallofTextnb_line(int _yoffset, const char* _message, int _centered, int _cursor, int _maxlinelength) {
    pax_buf_t* gfx  = bsp_get_gfx_buffer();
    pax_vec1_t dims = {
        .x = 999,
        .y = 999,
    };
    pax_vec1_t cursorloc = {
        .x = 0,
        .y = 0,
    };

    ESP_LOGE(TAG, "Unhandled event type %s", _message);
    ESP_LOGE(TAG, "Unhandled event type %d", strlen(_message));
    // char message[128]       = "The quick brown fox jumps over the lazy dog, The quick brie da";  // message to parse
    char linetodisplay[128] = "";
    char Words[64];  // Parsed Word
    int  _xoffset = 6;
    // int  _maxlinelength = gfx->height - _xoffset * 2;

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

void AddSwitchesBoxtoBuffer(int _switch)  // in black
{
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_outline_rect(gfx, 1, 61 * _switch, 114, 50, 12);
}

void AddBlocktoBuffer(int _x, int _y) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    _y--;
    pax_draw_line(gfx, BLACK, _x + 5, _y + -1, _x + 5, _y + 2);
    pax_draw_line(gfx, BLACK, _x + 5, _y + 2, _x + 2, _y + 5);

    pax_draw_line(gfx, BLACK, _x + 2, _y + 5, _x + -1, _y + 5);
    pax_draw_line(gfx, BLACK, _x + -1, _y + 5, _x + -4, _y + 2);

    pax_draw_line(gfx, BLACK, _x + -4, _y + 2, _x + -4, _y + -1);
    pax_draw_line(gfx, BLACK, _x + -4, _y + -1, _x + -1, _y + -4);

    pax_draw_line(gfx, BLACK, _x + -1, _y + -4, _x + 2, _y + -4);
    pax_draw_line(gfx, BLACK, _x + 2, _y + -4, _x + 5, _y + -1);
}
// Position is the X coordinate of the center/left (since it's even) pixel
void DisplayTelegraph(int _colour, int _position) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    if (_position < 36)
        _position = 36;  // prevent to draw outside of buffer

    // Diamond
    pax_draw_line(gfx, WHITE, _position - 3, 1, _position + 4, 1);        // white over the border
    pax_draw_line(gfx, WHITE, _position - 4, 4, _position + 5, 4);        // white over the border
    pax_draw_line(gfx, WHITE, _position - 4, 123, _position + 5, 123);    // white over the border
    pax_draw_line(gfx, WHITE, _position - 3, 126, _position + 4, 126);    // white over the border
    pax_draw_line(gfx, _colour, _position - 36, 64, _position - 3, 127);  // Left to top
    pax_draw_line(gfx, _colour, _position - 36, 63, _position - 3, 0);    // Left to bottom
    pax_draw_line(gfx, _colour, _position + 37, 64, _position + 4, 127);  // Right to top
    pax_draw_line(gfx, _colour, _position + 37, 63, _position + 4, 0);    // Right to bottom
    pax_draw_line(gfx, _colour, _position - 3, 0, _position + 4, 0);      // horizontal top
    pax_draw_line(gfx, _colour, _position - 3, 127, _position + 4, 127);  // horizontal bottom

    // Letter circles

    for (int i = 0; i < 20; i++) AddBlocktoBuffer(_position + telegraph_X[i], telegraph_Y[i]);
}

int InputtoNum(char _inputletter) {
    switch (_inputletter) {
        case 'a': return 0; break;
        case 'b': return 1; break;
        case 'c': return 2; break;
        case 'd': return 3; break;
        case 'e': return 4; break;
        case 'f': return 5; break;
        case 'g': return 6; break;
        case 'h': return 7; break;
        case 'i': return 8; break;
        case 'j': return 9; break;
        case 'k': return 10; break;
        case 'l': return 11; break;
        case 'm': return 12; break;
        case 'n': return 13; break;
        case 'o': return 14; break;
        case 'p': return 15; break;
        case 'q': return 16; break;
        case 'r': return 17; break;
        case 's': return 18; break;
        case 't': return 19; break;
        case 'u': return 20; break;
        case 'v': return 21; break;
        case 'w': return 22; break;
        case 'x': return 23; break;
        case 'y': return 24; break;
        case 'z': return 25; break;
        default: return 0; break;
    }
}

// debug to refactor
void configure_keyboard_guru(QueueHandle_t keyboard_event_queue, bool SW1, bool SW2, bool SW3, bool SW4, bool SW5) {
    // update the keyboard event handler settings
    event_t kbsettings = {
        .type                                = event_control_keyboard,
        .args_control_keyboard.enable_typing = true,
        .args_control_keyboard
            .enable_rotations = {false, false, false, false, false, false, false, false, false, false},
        .args_control_keyboard.enable_characters  = {true,  true,  true,  true,  true,  true,  true,  true,  true,
                                                     true,  true,  false, false, false, false, false, false, false,
                                                     false, false, false, false, false, false, false, false},
        .args_control_keyboard.enable_actions     = {SW1, SW2, SW3, SW4, SW5},
        .args_control_keyboard.enable_leds        = true,
        .args_control_keyboard.enable_relay       = true,
        kbsettings.args_control_keyboard.capslock = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void InitKeyboard(QueueHandle_t keyboard_event_queue) {
    kbsettings.type                                = event_control_keyboard;
    kbsettings.args_control_keyboard.enable_typing = false;
    for (int i = 0; i < NUM_ROTATION; i++) kbsettings.args_control_keyboard.enable_rotations[i] = false;
    for (int i = 0; i < NUM_CHARACTERS; i++) kbsettings.args_control_keyboard.enable_characters[i] = true;
    for (int i = 0; i < NUM_SWITCHES; i++) kbsettings.args_control_keyboard.enable_actions[i] = false;
    kbsettings.args_control_keyboard.enable_leds  = true;
    kbsettings.args_control_keyboard.enable_relay = true;
    kbsettings.args_control_keyboard.capslock     = false;
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void configure_keyboard_kb(QueueHandle_t keyboard_event_queue, event_t _kbsettings) {
    kbsettings = _kbsettings;
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void configure_keyboard_typing(QueueHandle_t keyboard_event_queue, bool _typing) {
    kbsettings.args_control_keyboard.enable_typing = _typing;
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void configure_keyboard_character(QueueHandle_t keyboard_event_queue, int _SW, bool _character) {
    kbsettings.args_control_keyboard.enable_characters[_SW] = _character;
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void configure_keyboard_press(QueueHandle_t keyboard_event_queue, int _SW, bool _state) {
    kbsettings.args_control_keyboard.enable_actions[_SW] = _state;
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void configure_keyboard_presses(QueueHandle_t keyboard_event_queue, bool SW1, bool SW2, bool SW3, bool SW4, bool SW5) {
    kbsettings.args_control_keyboard.enable_actions[0] = SW1;
    kbsettings.args_control_keyboard.enable_actions[1] = SW2;
    kbsettings.args_control_keyboard.enable_actions[2] = SW3;
    kbsettings.args_control_keyboard.enable_actions[3] = SW4;
    kbsettings.args_control_keyboard.enable_actions[4] = SW5;
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void configure_keyboard_rotate(QueueHandle_t keyboard_event_queue, int _SW, int _LR, bool _state) {
    kbsettings.args_control_keyboard.enable_rotations[_SW + _LR] = _state;
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void configure_keyboard_rotate_both(QueueHandle_t keyboard_event_queue, int _SW, bool _state) {
    kbsettings.args_control_keyboard.enable_rotations[_SW]                = _state;  // left
    kbsettings.args_control_keyboard.enable_rotations[_SW + NUM_SWITCHES] = _state;  // right
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void configure_keyboard_rotate_disable(QueueHandle_t keyboard_event_queue) {
    for (int i = 0; i < NUM_ROTATION; i++) kbsettings.args_control_keyboard.enable_rotations[i] = false;
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void configure_keyboard_relay(QueueHandle_t keyboard_event_queue, bool _relay) {
    kbsettings.args_control_keyboard.enable_relay = _relay;
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void configure_keyboard_caps(QueueHandle_t keyboard_event_queue, bool _caps) {
    kbsettings.args_control_keyboard.capslock = _caps;
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

void DebugKeyboardSettings(void) {
    ESP_LOGE(TAG, "enable_typing: %d", kbsettings.args_control_keyboard.enable_typing);
    ESP_LOGE(TAG, "enable_rotations:");
    for (int i = 0; i < NUM_ROTATION; i++)
        ESP_LOGE(TAG, "%d: %d", i, kbsettings.args_control_keyboard.enable_rotations[i]);
    ESP_LOGE(TAG, "enable_characters:");
    for (int i = 0; i < NUM_CHARACTERS; i++)
        ESP_LOGE(TAG, "%d: %d", i, kbsettings.args_control_keyboard.enable_characters[i]);
    ESP_LOGE(TAG, "enable_actions:");
    for (int i = 0; i < NUM_SWITCHES; i++)
        ESP_LOGE(TAG, "%d: %d", i, kbsettings.args_control_keyboard.enable_actions[i]);
    ESP_LOGE(TAG, "enable_leds: %d", kbsettings.args_control_keyboard.enable_leds);
    ESP_LOGE(TAG, "enable_relay: %d", kbsettings.args_control_keyboard.enable_relay);
    ESP_LOGE(TAG, "capslock: %d", kbsettings.args_control_keyboard.capslock);
}

int Increment(int _num, int _max) {
    _num++;
    if (_num >= _max)
        _num = 0;
    return _num;
}
int Decrement(int _num, int _max) {
    _num--;
    if (_num < 0)
        _num = _max - 1;
    return _num;
}

void AddDiamondSelecttoBuf(int _x, int _y, int _gap) {
    int x_offset   = 6;
    _x             = _x - 3;
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_insert_png_buf(gfx, diamondl_png_start, diamondl_png_end - diamondl_png_start, _x - _gap / 2 - x_offset, _y, 0);
    pax_insert_png_buf(gfx, diamondr_png_start, diamondr_png_end - diamondr_png_start, _x + _gap / 2 + x_offset, _y, 0);

    // // horizontal ship coordonates orientation east
    // int of[13][2] = {
    //     {3, 0, 4, 0},
    //     {2, 1, 3, 1},
    //     {1, 2, 2, 2},
    //     {0, 3, 1, 3},
    //     {4, 0, 5, 0},
    //     {4, 0, 5, 0},
    //     {-8, 5},
    //     {-7, -5},
    //     {-7, 6},
    //     {-6, -6},
    //     {5 + 16 * (_shiplength - 1), -6},
    //     {-6, 7},
    //     {5 + 16 * (_shiplength - 1), 7},
    //     {5 + 16 * (_shiplength - 1), -6},
    //     {11 + 16 * (_shiplength - 1), 0},
    //     {5 + 16 * (_shiplength - 1), 7},
    //     {11 + 16 * (_shiplength - 1), 1},
    //     {16, 0}
    // };

    //         for (int i = 1; i < 9; i++)
    //             pax_draw_line(gfx, BLACK, _x + od[i][0], _y + od[i][1], _x + od[i + 1][0], _y + od[i + 1][1]);
}

uint32_t ChartoLED(char _letter) {
    switch (_letter) {
        case 'a': return LED_A; break;
        case 'b': return LED_B; break;
        // case 'c': return LED_; break;
        case 'd': return LED_D; break;
        case 'e': return LED_E; break;
        case 'f': return LED_F; break;
        case 'g': return LED_G; break;
        case 'h': return LED_H; break;
        case 'i': return LED_I; break;
        // case 'j': return LED_; break;
        case 'k': return LED_K; break;
        case 'l': return LED_L; break;
        case 'm': return LED_M; break;
        case 'n': return LED_N; break;
        case 'o': return LED_O; break;
        case 'p': return LED_P; break;
        // case 'q': return LED_; break;
        case 'r': return LED_R; break;
        case 's': return LED_S; break;
        case 't': return LED_T; break;
        // case 'u': return LED_; break;
        case 'v': return LED_V; break;
        case 'w': return LED_W; break;
        // case 'x': return LED_; break;
        case 'y': return LED_Y; break;
        // case 'z': return LED_; break;
        default: return 0; break;
    }
}

esp_err_t nvs_get_str_wrapped(const char* namespace, const char* key, char* buffer, size_t buffer_size) {

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

esp_err_t nvs_set_str_wrapped(const char* namespace, const char* key, char* buffer) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res == ESP_OK) {
        res = nvs_set_str(handle, key, buffer);
    }
    nvs_commit(handle);
    nvs_close(handle);
    return res;
}

esp_err_t nvs_get_u8_wrapped(const char* namespace, const char* key, uint8_t* value) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res != ESP_OK) {
        return res;
    }
    res = nvs_get_u8(handle, key, value);
    nvs_close(handle);
    return res;
}

esp_err_t nvs_set_u8_wrapped(const char* namespace, const char* key, uint8_t value) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res != ESP_OK) {
        return res;
    }
    res = nvs_set_u8(handle, key, value);
    nvs_commit(handle);
    nvs_close(handle);
    return res;
}

esp_err_t nvs_get_u16_wrapped(const char* namespace, const char* key, uint16_t* value) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res != ESP_OK) {
        return res;
    }
    res = nvs_get_u16(handle, key, value);
    nvs_close(handle);
    return res;
}

esp_err_t nvs_set_u16_wrapped(const char* namespace, const char* key, uint16_t value) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res != ESP_OK) {
        return res;
    }
    res = nvs_set_u16(handle, key, value);
    nvs_commit(handle);
    nvs_close(handle);
    return res;
}

esp_err_t nvs_get_u8_blob_wrapped(const char* namespace, const char* key, uint8_t* value, size_t length) {

    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res != ESP_OK) {
        return res;
    }
    res = nvs_get_blob(handle, key, value, &length);
    if (res != ESP_OK) {
        value[0] = NULL;
        return res;
    }
    nvs_close(handle);
    return res;
}

esp_err_t nvs_set_u8_blob_wrapped(const char* namespace, const char* key, uint8_t* value, size_t length) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res != ESP_OK) {
        return res;
    }
    res = nvs_set_blob(handle, key, value, length);
    nvs_commit(handle);
    nvs_close(handle);
    return res;
}

screen_t screen_welcome_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    ESP_LOGE(TAG, "enter welcome");
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_typing(keyboard_event_queue, true);
    int        characterEntered = 0;
    char       word[10]         = "hack3r";
    int        displayflag      = 1;
    pax_buf_t* gfx              = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            AddSWborder2toBuffer();
            switch (characterEntered) {
                case 0:
                    {
                        pax_center_text(
                            gfx,
                            BLACK,
                            font1,
                            fontsizeS,
                            gfx->height / 2,
                            gfx->width / 2 - 5,
                            "Welcome to Hacker Hotel!"
                        );
                        pax_center_text(
                            gfx,
                            BLACK,
                            font1,
                            fontsizeS,
                            gfx->height / 2,
                            gfx->width / 2 + 5,
                            "You have been granted access to the portable Telegraph device,"
                        );
                        pax_center_text(
                            gfx,
                            BLACK,
                            font1,
                            fontsizeS,
                            gfx->height / 2,
                            gfx->width / 2 + 15,
                            "courtesy of badge.team~"
                        );

                        pax_center_text(
                            gfx,
                            BLACK,
                            font1,
                            fontsizeS,
                            gfx->height / 2,
                            gfx->width / 2 + 25,
                            "Let us show you how to use the peculiar input system:"
                        );

                        break;
                    }
                case 1:
                    {
                        pax_center_text(gfx, BLACK, font1, fontsizeS * 3, gfx->height / 2, gfx->width / 2, "h");
                        break;
                    }
                case 2:
                    {
                        pax_center_text(gfx, BLACK, font1, fontsizeS * 3, gfx->height / 2, gfx->width / 2, "ha");
                        break;
                    }
                case 3:
                    {
                        pax_center_text(gfx, BLACK, font1, fontsizeS * 3, gfx->height / 2, gfx->width / 2, "hac");
                        break;
                    }
                case 4:
                    {
                        pax_center_text(gfx, BLACK, font1, fontsizeS * 3, gfx->height / 2, gfx->width / 2, "hack");
                        break;
                    }
                case 5:
                    {
                        pax_center_text(gfx, BLACK, font1, fontsizeS * 3, gfx->height / 2, gfx->width / 2, "hack3");
                        break;
                    }
                case 6:
                    {
                        pax_center_text(gfx, BLACK, font1, fontsizeS * 3, gfx->height / 2, gfx->width / 2, "hack3r");
                        break;
                    }
                case 7:
                    {
                        return screen_nametag;
                        break;
                    }

                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: break;
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (event.args_input_keyboard.character != '\0' &&
                        event.args_input_keyboard.character == word[characterEntered]) {
                        characterEntered++;
                    }
                    displayflag = 1;
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
