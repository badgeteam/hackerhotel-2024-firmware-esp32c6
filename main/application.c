#include "application.h"
#include "application_settings.h"
#include "badge_messages.h"
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

extern uint8_t const border1_png_start[] asm("_binary_border1_png_start");
extern uint8_t const border1_png_end[] asm("_binary_border1_png_end");
extern uint8_t const border2_png_start[] asm("_binary_border2_png_start");
extern uint8_t const border2_png_end[] asm("_binary_border2_png_end");
extern uint8_t const switchframe1_png_start[] asm("_binary_switchframe1_png_start");
extern uint8_t const switchframe1_png_end[] asm("_binary_switchframe1_png_end");
extern uint8_t const switchframe2_png_start[] asm("_binary_switchframe2_png_start");
extern uint8_t const switchframe2_png_end[] asm("_binary_switchframe2_png_end");

extern uint8_t const diamondl_png_start[] asm("_binary_diamondl_png_start");
extern uint8_t const diamondl_png_end[] asm("_binary_diamondl_png_end");
extern uint8_t const diamondr_png_start[] asm("_binary_diamondr_png_start");
extern uint8_t const diamondr_png_end[] asm("_binary_diamondr_png_end");


event_t kbsettings;

int const telegraph_X[20] = {0, -8, 8, -16, 0, 16, -24, -8, 8, 24, -24, -8, 8, 24, -16, 0, 16, -8, 8, 0};
int const telegraph_Y[20] = {12, 27, 27, 42, 42, 42, 57, 57, 57, 57, 71, 71, 71, 71, 86, 86, 86, 101, 101, 116};

static char const * TAG = "application utilities";

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

void AddSWtoBuffer(
    char const * SW1str, char const * SW2str, char const * SW3str, char const * SW4str, char const * SW5str
) {
    Addborder2toBuffer();
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_insert_png_buf(gfx, switchframe2_png_start, switchframe2_png_end - switchframe2_png_start, 0, 127 - 11, 0);
    int gapx = 60;
    int o_x  = 28;
    int o_y  = 118;
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 0, o_y, SW1str);
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 1, o_y, SW2str);
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 2, o_y, SW3str);
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 3, o_y, SW4str);
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * 4, o_y, SW5str);
}

void AddOneTextSWtoBuffer(int _SW, char const * SWstr) {
    pax_buf_t* gfx  = bsp_get_gfx_buffer();
    int        gapx = 60;
    int        o_x  = 28;
    int        o_y  = 118;
    pax_center_text(gfx, BLACK, font1, 9, o_x + gapx * _SW, o_y, SWstr);
}

void Justify_right_text(
    pax_buf_t* buf, pax_col_t color, pax_font_t const * font, float font_size, float x, float y, char const * text
) {
    pax_vec1_t dims = {
        .x = 999,
        .y = 999,
    };

    dims         = pax_text_size(font, font_size, text);
    int _xoffset = buf->height - (int)dims.x - x;
    pax_draw_text(buf, color, font, font_size, _xoffset, y, text);
}

int DisplayExitConfirmation(char _prompt[128], QueueHandle_t keyboard_event_queue) {
    event_t kbsettings = {
        .type                                     = event_control_keyboard,
        .args_control_keyboard.enable_typing      = true,
        .args_control_keyboard.enable_actions     = {true, false, false, false, true},
        .args_control_keyboard.enable_leds        = true,
        .args_control_keyboard.enable_relay       = true,
        kbsettings.args_control_keyboard.capslock = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    int text_x        = 50;
    int text_y        = 20;
    int text_fontsize = 18;

    pax_font_t const * font = pax_font_sky;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();
    bsp_apply_lut(lut_1s);
    pax_background(gfx, WHITE);
    Addborder2toBuffer();
    pax_draw_text(gfx, BLACK, font, 9, text_x, text_y, _prompt);
    AddSwitchesBoxtoBuffer(SWITCH_1);
    pax_draw_text(gfx, BLACK, font, 9, 8 + SWITCH_1 * 62, 116, "yes");
    AddSwitchesBoxtoBuffer(SWITCH_5);
    pax_draw_text(gfx, BLACK, font, 9, 8 + SWITCH_5 * 62, 116, "no");
    bsp_display_flush();
    return 1;
}

int Screen_Confirmation(char _prompt[128], QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    event_t tempkbsettings = kbsettings;
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, false, false, false, true);

    int text_x        = 50;
    int text_y        = 20;
    int text_fontsize = 18;

    pax_buf_t* gfx = bsp_get_gfx_buffer();
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
                            break;
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5:
                            configure_keyboard_kb(keyboard_event_queue, tempkbsettings);
                            return 0;
                            break;
                        default: break;
                    }
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
// Parse _message[] into an array of _nbwords
// and makes them into up to _maxnblines which are _maxlinelenght pixel long
// can be centered if the _centered flag is high
void DisplayWallofTextWords(
    int  _fontsize,
    int  _maxlinelenght,
    int  _maxnblines,
    int  _nbwords,
    int  _xoffset,
    int  _yoffset,
    char _message[200],
    int  _centered
) {
    // set screen font and buffer
    pax_font_t const * font = pax_font_sky;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();

    // to verify text lenght on buffer
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

            // if longer than maxlinelenght, go to the next line
            dims = pax_text_size(font, _fontsize, linetodisplay);
            if ((int)dims.x > _maxlinelenght) {
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
// and makes them into up to _maxnblines which are _maxlinelenght pixel long
// can be centered if the _centered flag is high
void DisplayWallofText(
    int  _fontsize,
    int  _maxlinelenght,
    int  _maxnblines,
    int  _nbwords,
    int  _xoffset,
    int  _yoffset,
    char _message[500],
    int  _centered
) {
    // set screen font and buffer
    pax_font_t const * font = pax_font_sky;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();

    // to verify text lenght on buffer
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

            // if longer than maxlinelenght, go to the next line
            dims = pax_text_size(font, _fontsize, linetodisplay);
            if ((int)dims.x > _maxlinelenght) {
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
    pax_draw_line(gfx, _colour, _position - 36, 64, _position - 3, 127);  // Left to top
    pax_draw_line(gfx, _colour, _position - 36, 63, _position - 3, 0);    // Left to bottom
    pax_draw_line(gfx, _colour, _position + 37, 64, _position + 4, 127);  // Right to top
    pax_draw_line(gfx, _colour, _position + 37, 63, _position + 4, 0);    // Right to bottom

    // Letter circles

    for (int i = 0; i < 20; i++) AddBlocktoBuffer(_position + telegraph_X[i], telegraph_Y[i]);
}
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
    for (int i = 0; i < NUM_LETTER; i++) kbsettings.args_control_keyboard.enable_characters[i] = true;
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
    for (int i = 0; i < NUM_LETTER; i++)
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
    if (_num > _max)
        _num = 0;
    return _num;
}
int Decrement(int _num, int _max) {
    _num--;
    if (_num < 0)
        _num = _max;
    return _num;
}

void AddDiamondSelecttoBuf(int _x, int _y, int _gap) {
    int x_offset   = 6;
    _x             = _x - 3;
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_insert_png_buf(
        bsp_get_gfx_buffer(),
        diamondl_png_start,
        diamondl_png_end - diamondl_png_start,
        _x - _gap / 2 - x_offset,
        _y,
        0
    );
    pax_insert_png_buf(
        bsp_get_gfx_buffer(),
        diamondr_png_start,
        diamondr_png_end - diamondr_png_start,
        _x + _gap / 2 + x_offset,
        _y,
        0
    );

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
    //     {5 + 16 * (_shiplenght - 1), -6},
    //     {-6, 7},
    //     {5 + 16 * (_shiplenght - 1), 7},
    //     {5 + 16 * (_shiplenght - 1), -6},
    //     {11 + 16 * (_shiplenght - 1), 0},
    //     {5 + 16 * (_shiplenght - 1), 7},
    //     {11 + 16 * (_shiplenght - 1), 1},
    //     {16, 0}
    // };

    //         for (int i = 1; i < 9; i++)
    //             pax_draw_line(gfx, BLACK, _x + od[i][0], _y + od[i][1], _x + od[i + 1][0], _y + od[i + 1][1]);
}

esp_err_t nvs_get_str_wrapped(char const * namespace, char const * key, char* buffer, size_t buffer_size) {
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

esp_err_t nvs_set_str_wrapped(char const * namespace, char const * key, char* buffer) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res == ESP_OK) {
        res = nvs_set_str(handle, key, buffer);
    }
    nvs_commit(handle);
    nvs_close(handle);
    return res;
}