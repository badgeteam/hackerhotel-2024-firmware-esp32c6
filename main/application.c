#include "application.h"
#include "application_settings.h"
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
#include "sdkconfig.h"
#include "sdmmc_cmd.h"
#include "soc/gpio_struct.h"
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

int const telegraph_X[20] = {0, -8, 8, -16, 0, 16, -24, -8, 8, 24, -24, -8, 8, 24, -16, 0, 16, -8, 8, 0};
int const telegraph_Y[20] = {12, 27, 27, 42, 42, 42, 57, 57, 57, 57, 71, 71, 71, 71, 86, 86, 86, 101, 101, 116};

static char const * TAG = "application utilities";

void Addborder1toBuffer(void) {

    pax_insert_png_buf(bsp_get_gfx_buffer(), border1_png_start, border1_png_end - border1_png_start, 0, 0, 0);
}
void Addborder2toBuffer(void) {

    pax_insert_png_buf(bsp_get_gfx_buffer(), border2_png_start, border2_png_end - border2_png_start, 0, 0, 0);
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
        .type                                     = event_control_keyboard,
        .args_control_keyboard.enable_typing      = false,
        .args_control_keyboard.enable_actions     = {SW1, SW2, SW3, SW4, SW5},
        .args_control_keyboard.enable_leds        = true,
        .args_control_keyboard.enable_relay       = true,
        kbsettings.args_control_keyboard.capslock = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}
