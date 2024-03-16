#pragma once

#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "pax_gfx.h"
#include "screens.h"

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

#define font1     (&PRIVATE_pax_font_sky)
#define fontsizeS 9

#define dev        0
#define production 1

// SET PRODUCTION OR DEV HERE
#define release_type dev
//

#define log 1

extern const int telegraph_X[20];
extern const int telegraph_Y[20];
extern event_t   kbsettings;

void DisplayError(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, const char* errorstr);
void draw_squi1(int y);
void draw_squi2(int y);
void DrawArrowVertical(int _sw);
void DrawArrowHorizontal(int _sw);
void Addborder1toBuffer(void);
void Addborder2toBuffer(void);
void AddSWborder1toBuffer(void);
void AddSWborder2toBuffer(void);

void AddSWtoBuffer(const char* SW1str, const char* SW2str, const char* SW3str, const char* SW4str, const char* SW5str);
void AddSWtoBufferL(const char* SW1str);
void AddSWtoBufferLR(const char* SW1str, const char* SW5str);

void AddBlocktoBuffer(int _x, int _y);

int  countwords(const char* text);
void getword(const char* text, char* word, int word_nb);
void drawParagraph(int x_offset, int y_offset, const char* text, bool centered);

pax_vec1_t WallofText(int _yoffset, const char* _message, int _centered, int _cursor);

void Justify_right_text(
    pax_buf_t* buf, pax_col_t color, const pax_font_t* font, float font_size, float x, float y, const char* text
);

int Screen_Information(const char* _prompt, QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
int Screen_Confirmation(const char* _prompt, QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
int WaitingforOpponent(const char* _prompt, QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
void AddSwitchesBoxtoBuffer(int _switch);
void AddOneTextSWtoBuffer(int _SW, const char* SWstr);
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

int      Increment(int _num, int _max);
int      Decrement(int _num, int _max);
void     AddDiamondSelecttoBuf(int _x, int _y, int _gap);
uint32_t ChartoLED(char _letter);

esp_err_t nvs_get_str_wrapped(const char* namespace, const char* key, char* buffer, size_t buffer_size);
esp_err_t nvs_set_str_wrapped(const char* namespace, const char* key, char* buffer);
esp_err_t nvs_get_u8_wrapped(const char* namespace, const char* key, uint8_t* value);
esp_err_t nvs_set_u8_wrapped(const char* namespace, const char* key, uint8_t value);
esp_err_t nvs_get_u16_wrapped(const char* namespace, const char* key, uint16_t* value);
esp_err_t nvs_set_u16_wrapped(const char* namespace, const char* key, uint16_t value);
esp_err_t nvs_get_u8_blob_wrapped(const char* namespace, const char* key, uint8_t* value, size_t length);
esp_err_t nvs_set_u8_blob_wrapped(const char* namespace, const char* key, uint8_t* value, size_t length);

screen_t screen_welcome_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);

/*  USEFUL bit of code
Delay:
 vTaskDelay(pdMS_TO_TICKS(100));






*/
