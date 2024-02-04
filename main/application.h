#pragma once

#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "pax_gfx.h"

#define LED_OFF       0x000000
#define LED_RED       0xFF0000
#define LED_GREEN     0x00FF00
#define LED_BLUE      0x0000FF
#define LED_YELLOW    0xFFFF00
#define LED_TURQUOISE 0x00FFFF
#define LED_PURPLE    0xFF00FF
#define LED_WHITE     0xFFFFFF

#define left  0
#define right 5

extern int const telegraph_X[20];
extern int const telegraph_Y[20];

#define font1     (&PRIVATE_pax_font_sky)
#define fontsizeS 9

void Addborder1toBuffer(void);
void Addborder2toBuffer(void);
void AddSWborder1toBuffer(void);
void AddSWborder2toBuffer(void);
void AddSWtoBuffer(
    char const * SW1str, char const * SW2str, char const * SW3str, char const * SW4str, char const * SW5str
);
void AddBlocktoBuffer(int _x, int _y);

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
);
pax_vec1_t WallofText(int _yoffset, char const * _message, int _centered, int _cursor);
// Parse _message[] into lines
// and makes them into up to _maxnblines which are _maxlinelenght pixel long
// can be centered if the _centered flag is high
void       DisplayWallofText(
          int _fontsize, int _maxlinelenght, int _maxnblines, int _xoffset, int _yoffset, char _message[500], int _centered
      );

void Justify_right_text(
    pax_buf_t* buf, pax_col_t color, pax_font_t const * font, float font_size, float x, float y, char const * text
);
int  DisplayExitConfirmation(char _prompt[128], QueueHandle_t keyboard_event_queue);
int  Screen_Confirmation(char _prompt[128], QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
void AddSwitchesBoxtoBuffer(int _switch);
void AddOneTextSWtoBuffer(int _SW, char const * SWstr);
void DisplayTelegraph(int _colour, int _position);
int  InputtoNum(char _inputletter);
void configure_keyboard_guru(QueueHandle_t keyboard_event_queue, bool SW1, bool SW2, bool SW3, bool SW4, bool SW5);
void InitKeyboard(QueueHandle_t keyboard_event_queue);
void configure_keyboard_kb(QueueHandle_t keyboard_event_queue, event_t _kbsettings);
void configure_keyboard_typing(QueueHandle_t keyboard_event_queue, bool _typing);
void configure_keyboard_character(QueueHandle_t keyboard_event_queue, int _pos, bool _character);
void configure_keyboard_press(QueueHandle_t keyboard_event_queue, int _pos, bool _state);
void configure_keyboard_presses(QueueHandle_t keyboard_event_queue, bool SW1, bool SW2, bool SW3, bool SW4, bool SW5);
void configure_keyboard_rotate(QueueHandle_t keyboard_event_queue, int _SW, int _LR, bool _state);
void configure_keyboard_rotate_both(QueueHandle_t keyboard_event_queue, int _SW, bool _state);
void configure_keyboard_rotate_disable(QueueHandle_t keyboard_event_queue);
void configure_keyboard_relay(QueueHandle_t keyboard_event_queue, bool _relay);
void configure_keyboard_caps(QueueHandle_t keyboard_event_queue, bool _caps);
void DebugKeyboardSettings(void);
int  Increment(int _num, int _max);
int  Decrement(int _num, int _max);
void AddDiamondSelecttoBuf(int _x, int _y, int _gap);
uint32_t  ChartoLED(char _letter);
esp_err_t nvs_get_str_wrapped(char const * namespace, char const * key, char* buffer, size_t buffer_size);
esp_err_t nvs_set_str_wrapped(char const * namespace, char const * key, char* buffer);
esp_err_t nvs_get_u8_wrapped(char const * namespace, char const * key, uint8_t* value);
esp_err_t nvs_set_u8_wrapped(char const * namespace, char const * key, uint8_t value);
esp_err_t nvs_get_u8_blob_wrapped(char const * namespace, char const * key, uint8_t* value, size_t length);
esp_err_t nvs_set_u8_blob_wrapped(char const * namespace, char const * key, uint8_t* value, size_t length);

/*  USEFUL bit of code
Delay:
 vTaskDelay(pdMS_TO_TICKS(100));






*/