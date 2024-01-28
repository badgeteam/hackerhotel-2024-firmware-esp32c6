#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "pax_types.h"

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

void debug_(void);
void Justify_right_text(
    pax_buf_t* buf, pax_col_t color, pax_font_t const * font, float font_size, float x, float y, char const * text
);
int  DisplayExitConfirmation(char _prompt[128], QueueHandle_t keyboard_event_queue);
void app_thread_entry(QueueHandle_t event_queue);
void AddSwitchesBoxtoBuffer(int _switch);
void DisplayTelegraph(int _colour, int _position);
