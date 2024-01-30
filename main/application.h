#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "pax_types.h"

#define nicknamelenght 32
#define messagelenght  67

#define LED_OFF       0x000000
#define LED_RED       0xFF0000
#define LED_GREEN     0x00FF00
#define LED_BLUE      0x0000FF
#define LED_YELLOW    0xFFFF00
#define LED_TURQUOISE 0x00FFFF
#define LED_PURPLE    0xFF00FF
#define LED_WHITE     0xFFFFFF


void Addborder1toBuffer(void);
void Addborder2toBuffer(void);

void DisplayWallofTextWords(
    int  _fontsize,
    int  _maxlinelenght,
    int  _maxnblines,
    int  _nbwords,
    int  _xoffset,
    int  _yoffset,
    char _message[200],
    int  _centered
);

void DisplayWallofText(
    int  _fontsize,
    int  _maxlinelenght,
    int  _maxnblines,
    int  _nbwords,
    int  _xoffset,
    int  _yoffset,
    char _message[500],
    int  _centered
);

void Justify_right_text(
    pax_buf_t* buf, pax_col_t color, pax_font_t const * font, float font_size, float x, float y, char const * text
);
int  DisplayExitConfirmation(char _prompt[128], QueueHandle_t keyboard_event_queue);
void app_thread_entry(QueueHandle_t event_queue);
void AddSwitchesBoxtoBuffer(int _switch);
void DisplayTelegraph(int _colour, int _position);
