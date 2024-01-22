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

#define MainMenuhub          0
#define MainMenubattleship   1
#define MainMenuBadgetag     2
#define MainMenuSettings     3
#define MainMenuCredits      4
#define Gamescreenbattleship 5
#define Gameendbattleship    6

#define BSplaceboat  0
#define playerturn   1
#define opponentturn 2

#define menuinputdelay 500

#define water         0
#define boat          1
#define missedshot    2
#define boathit       3
#define boatdestroyed 4

static char const * TAG = "app";

uint8_t buttons[5];
int     delaySM                      = 1;
int     delaySMflag                  = 0;
int     delayLED                     = 0;
int     delayLEDflag                 = 0;
int     delayMISC                    = 0;
int     delayMISCflag                = 0;
int     MainMenustatemachine         = MainMenubattleship;
int     Battleshipstatemachine       = BSplaceboat;
int     MainMenuchangeflag           = 1;
char    inputletter                  = NULL;
char    playername[30]               = "";
char    opponent[30]                 = "";
int     specialcharacterselect       = 0;
char    specialcharactersicon[4][20] = {"A/!", "CAPS", "!?", "<|>"};
int     BSplayerboard[20]            = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int     BSopponentboard[20]          = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int     telegraphX[20]               = {0, -8, 8, -16, 0, 16, -24, -8, 8, 24, -24, -8, 8, 24, -16, 0, 16, -8, 8, 0};
int     telegraphY[20]   = {12, 27, 27, 42, 42, 42, 57, 57, 57, 57, 71, 71, 71, 71, 86, 86, 86, 101, 101, 116};
int     BSvictory        = 0;
int     AIshotsfired[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int     playerboats[6]   = {0, 0, 0, 0, 0, 0};
int     opponentboats[6] = {0, 0, 0, 0, 0, 0};
int     popboat          = 0;


esp_err_t TextInputTelegraph(void);

int DisplaySelectedLetter(int _selectedletter) {
    delayLED     = 500;
    delayLEDflag = 1;
    return _selectedletter;
}

void DisplayDebugCenterlines(void)  // in black
{
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_draw_line(gfx, 1, 147, 0, 147, 127);
    pax_draw_line(gfx, 1, 148, 0, 148, 127);
    pax_draw_line(gfx, 1, 0, 63, 295, 63);
    pax_draw_line(gfx, 1, 0, 64, 295, 64);
}

void DisplayDebugSwitchesBoxes(void)  // in black
{
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_outline_rect(gfx, 1, 0, 114, 52, 14);  // Switch 1
    // pax_draw_rect(gfx, 0, 1, 115, 50, 12);

    pax_outline_rect(gfx, 1, 61, 114, 52, 14);  // Switch 2
    // pax_draw_rect(gfx, 0, 62, 115, 50, 12);

    pax_outline_rect(gfx, 1, 61 * 2, 114, 52, 14);  // Switch 3
    // pax_draw_rect(gfx, 0, 1 + 61 * 2, 115, 50, 12);

    pax_outline_rect(gfx, 1, 61 * 3, 114, 52, 14);  // Switch 4
    // pax_draw_rect(gfx, 0, 1 + 61 * 3, 115, 50, 12);

    pax_outline_rect(gfx, 1, 61 * 4, 114, 52, 14);  // Switch 5
    // pax_draw_rect(gfx, 0, 1 + 61 * 4, 115, 50, 12);
}

void DisplaySwitchesBox(int _switch)  // in black
{
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_outline_rect(gfx, 1, 61 * _switch, 114, 50, 12);
}

void DisplayblockstatusBS(int _position, int _block, int _status) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    switch (_status) {
        case water:
            // do nothing
            break;

        case missedshot: pax_outline_circle(gfx, BLACK, _position + telegraphX[_block], telegraphY[_block], 3); break;
        case boat: pax_draw_circle(gfx, BLACK, _position + telegraphX[_block], telegraphY[_block], 3); break;
        case boathit: pax_draw_circle(gfx, RED, _position + telegraphX[_block], telegraphY[_block], 3); break;
        case boatdestroyed: pax_draw_circle(gfx, RED, _position + telegraphX[_block], telegraphY[_block], 5); break;

        default: break;
    }
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

    for (int i = 0; i < 20; i++) pax_outline_circle(gfx, _colour, _position + telegraphX[i], telegraphY[i], 6);

    // pax_outline_circle(gfx, _colour, _position, 12, 6);

    // pax_outline_circle(gfx, _colour, _position - 8, 27, 6);
    // pax_outline_circle(gfx, _colour, _position + 8, 27, 6);

    // pax_outline_circle(gfx, _colour, _position - 16, 42, 6);
    // pax_outline_circle(gfx, _colour, _position, 42, 6);
    // pax_outline_circle(gfx, _colour, _position + 16, 42, 6);

    // pax_outline_circle(gfx, _colour, _position - 24, 57, 6);
    // pax_outline_circle(gfx, _colour, _position - 8, 57, 6);
    // pax_outline_circle(gfx, _colour, _position + 8, 57, 6);
    // pax_outline_circle(gfx, _colour, _position + 24, 57, 6);

    // pax_outline_circle(gfx, _colour, _position - 24, 71, 6);
    // pax_outline_circle(gfx, _colour, _position - 8, 71, 6);
    // pax_outline_circle(gfx, _colour, _position + 8, 71, 6);
    // pax_outline_circle(gfx, _colour, _position + 24, 71, 6);

    // pax_outline_circle(gfx, _colour, _position - 16, 86, 6);
    // pax_outline_circle(gfx, _colour, _position, 86, 6);
    // pax_outline_circle(gfx, _colour, _position + 16, 86, 6);

    // pax_outline_circle(gfx, _colour, _position - 8, 101, 6);
    // pax_outline_circle(gfx, _colour, _position + 8, 101, 6);

    // pax_outline_circle(gfx, _colour, _position, 116, 6);
}

// bodge timer and debouncing, to fix
void testaa(void* param) {
    (void)param;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (delaySMflag == 1) {
            vTaskDelay(pdMS_TO_TICKS(delaySM));
            delaySMflag = 0;
        }

        if (delayLEDflag == 1) {
            vTaskDelay(pdMS_TO_TICKS(delayLED));
            delayLEDflag = 0;
        }
        if (delayMISCflag == 1) {
            vTaskDelay(pdMS_TO_TICKS(delayLED));
            delayMISCflag = 0;
        }
    }
}

void framenametag(void) {
    bsp_apply_lut(lut_1s);
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_background(gfx, 0);
    if (specialcharacterselect == 0 || specialcharacterselect == 1)
        pax_draw_text(gfx, 1, pax_font_saira_regular, 30, 80, 50, playername);
    DisplaySwitchesBox(SWITCH_1);
    pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
    DisplaySwitchesBox(SWITCH_3);
    pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 133, 116, specialcharactersicon[specialcharacterselect]);
    DisplaySwitchesBox(SWITCH_5);
    pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "delete");
    if (specialcharacterselect == 2) {
        int _position = 90;
        DisplayTelegraph(BLACK, _position);
        int _offsetX = 5;
        int _offsetY = 5;
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - _offsetX, 12 - _offsetY, "@");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 8 - _offsetX, 27 - _offsetY, "#");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 8 - _offsetX, 27 - _offsetY, "$");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 16 - _offsetX, 42 - _offsetY, "_");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - _offsetX, 42 - _offsetY, "&");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 16 - _offsetX, 42 - _offsetY, "-");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 24 - _offsetX, 57 - _offsetY, "+");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 8 - _offsetX, 57 - _offsetY, "(");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 8 - _offsetX, 57 - _offsetY, ")");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 24 - _offsetX, 57 - _offsetY, "/");

        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - _offsetX, 116 - _offsetY, "|");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 8 - _offsetX, 101 - _offsetY, "~");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 8 - _offsetX, 101 - _offsetY, "`");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 16 - _offsetX, 86 - _offsetY, ";");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - _offsetX, 86 - _offsetY, "!");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 16 - _offsetX, 86 - _offsetY, "?");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 24 - _offsetX, 71 - _offsetY, "*");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 8 - _offsetX, 71 - _offsetY, "\"");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 8 - _offsetX, 71 - _offsetY, "'");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 24 - _offsetX, 71 - _offsetY, ":");
    }

    if (specialcharacterselect == 3) {
        int _position = 90;
        DisplayTelegraph(BLACK, _position);
        int _offsetX = 5;
        int _offsetY = 5;
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - _offsetX, 12 - _offsetY, "=");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 8 - _offsetX, 27 - _offsetY, "[");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 8 - _offsetX, 27 - _offsetY, "]");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 16 - _offsetX, 42 - _offsetY, "\\");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - _offsetX, 42 - _offsetY, "{");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 16 - _offsetX, 42 - _offsetY, "}");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 24 - _offsetX, 57 - _offsetY, "<");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 8 - _offsetX, 57 - _offsetY, ">");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 8 - _offsetX, 57 - _offsetY, "");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 24 - _offsetX, 57 - _offsetY, "");

        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - _offsetX, 116 - _offsetY, "");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 8 - _offsetX, 101 - _offsetY, "");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 8 - _offsetX, 101 - _offsetY, "");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 16 - _offsetX, 86 - _offsetY, "");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - _offsetX, 86 - _offsetY, "");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 16 - _offsetX, 86 - _offsetY, "");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 24 - _offsetX, 71 - _offsetY, "");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position - 8 - _offsetX, 71 - _offsetY, "");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 8 - _offsetX, 71 - _offsetY, "");
        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 10, _position + 24 - _offsetX, 71 - _offsetY, "");
    }
    bsp_display_flush();
}

int SelectBlock(void) {
    switch (inputletter) {
        case 'a': return 0; break;
        case 'b': return 1; break;
        case 'd': return 2; break;
        case 'e': return 3; break;
        case 'f': return 4; break;
        case 'g': return 5; break;
        case 'h': return 6; break;
        case 'i': return 7; break;
        case 'k': return 8; break;
        case 'l': return 9; break;
        case 'm': return 10; break;
        case 'n': return 11; break;
        case 'o': return 12; break;
        case 'p': return 13; break;
        case 'r': return 14; break;
        case 's': return 15; break;
        case 't': return 16; break;
        case 'v': return 17; break;
        case 'w': return 18; break;
        case 'y': return 19; break;
        default: return 0; break;
    }
}

unsigned long millis() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void reset_buttons() {
    for (int i = 0; i < 5; i++) {
        buttons[i] = SWITCH_IDLE;
    }
}

TaskHandle_t testaa_handle = NULL;

void app_thread_entry(QueueHandle_t event_queue) {
    MainMenustatemachine = MainMenubattleship;
    reset_buttons();

    bsp_apply_lut(lut_4s);

    if (testaa_handle == NULL) {
        xTaskCreate(testaa, "testaa", 1024, NULL, 1, &testaa_handle);
    }


    while (1) {
        pax_buf_t* gfx  = bsp_get_gfx_buffer();
        bool       busy = hink_busy(bsp_get_epaper());

        // Quick hack to convert the new button queue back into the old polling method
        event_t event;
        if (xQueueReceive(event_queue, &event, 0) == pdTRUE) {
            switch (event.type) {
                case event_input_button:
                    printf(
                        "Application: button %u state changed to %u\n",
                        event.args_input_button.button,
                        event.args_input_button.state
                    );
                    buttons[event.args_input_button.button] = event.args_input_button.state;
                    break;
                case event_input_keyboard:
                    if (event.args_input_keyboard.action >= 0) {
                        printf("Keyboard action: %u\n", event.args_input_keyboard.action);
                    } else {
                        printf("Keyboard input:  %c\n", event.args_input_keyboard.character);
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }

        if (!busy) {
            switch (MainMenustatemachine) {
                case MainMenuhub:
                    return;  // Exit to main fw

                    if (buttons[SWITCH_2] == SWITCH_PRESS) {
                        MainMenustatemachine = MainMenuBadgetag;
                        MainMenuchangeflag   = 1;
                        break;
                    }

                    if (buttons[SWITCH_3] == SWITCH_PRESS) {
                        MainMenustatemachine = MainMenuCredits;
                        MainMenuchangeflag   = 1;
                        break;
                    }

                    if (buttons[SWITCH_4] == SWITCH_PRESS) {
                        MainMenustatemachine = MainMenuSettings;
                        MainMenuchangeflag   = 1;
                        break;
                    }

                    if (buttons[SWITCH_5] == SWITCH_PRESS) {
                        MainMenustatemachine = MainMenubattleship;
                        MainMenuchangeflag   = 1;
                        delaySMflag          = 1;
                        delaySM              = menuinputdelay;
                        break;
                    }
                    if (MainMenuchangeflag == 1) {
                        bsp_apply_lut(lut_4s);
                        pax_background(gfx, 0);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 12, 50, 50, "Main hub");
                        DisplaySwitchesBox(SWITCH_1);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, " ");
                        DisplaySwitchesBox(SWITCH_2);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 65, 116, "Nametag");
                        DisplaySwitchesBox(SWITCH_3);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 125, 116, "Credits");
                        DisplaySwitchesBox(SWITCH_4);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 187, 116, "Settings");
                        DisplaySwitchesBox(SWITCH_5);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "battle");
                        bsp_display_flush();
                        MainMenuchangeflag = 0;
                    }
                    break;

                case MainMenuBadgetag:

                    if (buttons[SWITCH_1] == SWITCH_PRESS) {
                        MainMenustatemachine = MainMenuhub;
                        MainMenuchangeflag   = 1;
                        break;
                    }
                    if (buttons[SWITCH_3] == SWITCH_PRESS) {
                        specialcharacterselect++;
                        if (specialcharacterselect > 3)
                            specialcharacterselect = 0;
                        framenametag();
                    }
                    // specialcharacterselect

                    if (buttons[SWITCH_5] == SWITCH_PRESS) {
                        playername[strlen(playername) - 1] = '\0';
                        framenametag();
                    }

                    if (MainMenuchangeflag == 1) {
                        inputletter = NULL;
                        framenametag();
                        MainMenuchangeflag = 0;
                    }

                    if (inputletter != NULL) {
                        strncat(playername, &inputletter, 1);
                        specialcharacterselect = 0;
                        framenametag();
                        inputletter = NULL;
                    }
                    break;
                case MainMenuSettings:
                    menu_settings();
                    MainMenustatemachine = MainMenuhub;
                    MainMenuchangeflag   = 1;
                    reset_buttons();
                    /*if (buttons[SWITCH_1] == SWITCH_PRESS) {
                        MainMenustatemachine = MainMenuhub;
                        MainMenuchangeflag   = 1;
                        break;
                    }
                    if (MainMenuchangeflag == 1) {
                        bsp_apply_lut(lut_4s);
                        pax_background(gfx, 0);
                        pax_draw_text(gfx, 1, pax_font_marker, 18, 1, 0, "Settings placeholder");
                        DisplaySwitchesBox(SWITCH_1);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
                        // DisplaySwitchesBox(SWITCH_3);
                        // pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 125, 116, "Online");
                        // DisplaySwitchesBox(SWITCH_5);
                        // pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "Offline");
                        bsp_display_flush();
                        MainMenuchangeflag = 0;
                    }*/
                    break;

                case MainMenuCredits:
                    if (buttons[SWITCH_1] == SWITCH_PRESS) {
                        MainMenustatemachine = MainMenuhub;
                        MainMenuchangeflag   = 1;
                        break;
                    }
                    if (MainMenuchangeflag == 1) {
                        bsp_apply_lut(lut_4s);
                        pax_background(gfx, 0);
                        pax_draw_text(gfx, 1, pax_font_marker, 18, 1, 0, "Credits placeholder");
                        DisplaySwitchesBox(SWITCH_1);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
                        // DisplaySwitchesBox(SWITCH_3);
                        // pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 125, 116, "Online");
                        // DisplaySwitchesBox(SWITCH_5);
                        // pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "Offline");
                        bsp_display_flush();
                        MainMenuchangeflag = 0;
                    }
                    break;

                case MainMenubattleship:

                    if (delaySMflag == 0) {
                        if (buttons[SWITCH_1] == SWITCH_PRESS) {
                            MainMenustatemachine = MainMenuhub;
                            MainMenuchangeflag   = 1;
                            delaySMflag          = 1;
                            delaySM              = menuinputdelay;
                            break;
                        }

                        if (buttons[SWITCH_3] == SWITCH_PRESS) {
                            // Placeholder for multiplayer
                            // MainMenustatemachine = MainMenuhub;
                            // MainMenuchangeflag   = 1;
                            // break;
                        }

                        if (buttons[SWITCH_5] == SWITCH_PRESS) {
                            MainMenustatemachine = Gamescreenbattleship;
                            strcpy(opponent, "AI");
                            MainMenuchangeflag = 1;
                            delaySMflag        = 1;
                            delaySM            = menuinputdelay;
                            break;
                        }
                    }

                    if (MainMenuchangeflag == 1) {
                        bsp_apply_lut(lut_4s);
                        pax_background(gfx, 0);
                        pax_draw_text(gfx, 1, pax_font_marker, 18, 1, 0, "Battleship");
                        DisplaySwitchesBox(SWITCH_1);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
                        DisplaySwitchesBox(SWITCH_3);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 125, 116, "Online");
                        DisplaySwitchesBox(SWITCH_5);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "Offline");
                        bsp_display_flush();
                        MainMenuchangeflag = 0;
                    }

                case Gamescreenbattleship:


                    if (buttons[SWITCH_1] == SWITCH_PRESS) {
                        MainMenustatemachine = MainMenuhub;
                        MainMenuchangeflag   = 1;
                        break;
                    }

                    switch (Battleshipstatemachine) {
                        case BSplaceboat:
                            if (inputletter != NULL && delaySMflag == 0) {
                                playerboats[popboat]         = SelectBlock();
                                BSplayerboard[SelectBlock()] = boat;
                                inputletter                  = NULL;
                                popboat++;
                                if (popboat > 5) {
                                    Battleshipstatemachine = playerturn;
                                    // generate AI boats
                                    for (int i = 0; i < 6; i++) {
                                        int _flagduplicate = 1;
                                        int tempplacement  = 0;
                                        while (_flagduplicate) {
                                            _flagduplicate = 0;
                                            tempplacement  = esp_random() % 20;
                                            for (int y = 0; y < 6; y++) {
                                                if (tempplacement == opponentboats[y])
                                                    _flagduplicate = 1;
                                            }
                                        }
                                        opponentboats[i]               = tempplacement;
                                        BSopponentboard[tempplacement] = boat;
                                    }
                                }
                                MainMenuchangeflag = 1;
                                delaySMflag        = 1;
                                delaySM            = menuinputdelay;
                            }
                            break;

                        case playerturn:
                            if (inputletter != NULL && delaySMflag == 0) {
                                printf("player turn");
                                if (BSopponentboard[SelectBlock()] == boat)
                                    BSopponentboard[SelectBlock()] = boathit;
                                else if (BSopponentboard[SelectBlock()] == water)
                                    BSopponentboard[SelectBlock()] = missedshot;
                                inputletter            = NULL;
                                Battleshipstatemachine = opponentturn;
                                MainMenuchangeflag     = 1;
                                delaySMflag            = 1;
                                delaySM                = menuinputdelay;

                                // check for victory - ie no boats left
                                BSvictory = 1;
                                for (int i = 0; i < 20; i++) {
                                    if (BSopponentboard[i] == boat)
                                        BSvictory = 0;
                                }
                            }
                            if (buttons[SWITCH_4] == SWITCH_PRESS && delaySMflag == 0) {
                                Battleshipstatemachine = opponentturn;
                                MainMenuchangeflag     = 1;
                                delaySMflag            = 1;
                                delaySM                = menuinputdelay;
                            }
                            break;

                        case opponentturn:
                            // if (inputletter != NULL && delaySMflag == 0) {
                            printf("opponent turn");

                            int shotturn = esp_random() % 20;
                            while (AIshotsfired[shotturn] == 1) {
                                shotturn = esp_random() % 20;
                            }
                            AIshotsfired[shotturn] = 1;
                            if (BSplayerboard[shotturn] == boat)
                                BSplayerboard[shotturn] = boathit;
                            else if (BSplayerboard[shotturn] == water)
                                BSplayerboard[shotturn] = missedshot;
                            inputletter            = NULL;
                            Battleshipstatemachine = playerturn;
                            MainMenuchangeflag     = 1;
                            delaySMflag            = 1;
                            delaySM                = menuinputdelay;

                            // check for victory - ie no boats left
                            BSvictory = 2;
                            for (int i = 0; i < 20; i++) {
                                if (BSplayerboard[i] == boat)
                                    BSvictory = 0;
                            }
                            // }
                            if (buttons[SWITCH_4] == SWITCH_PRESS && delaySMflag == 0) {
                                Battleshipstatemachine = playerturn;
                                MainMenuchangeflag     = 1;
                                delaySMflag            = 1;
                                delaySM                = menuinputdelay;
                            }
                            break;
                            // case opponentturn:
                            //     if (buttons[SWITCH_4] == SWITCH_PRESS && delaySMflag == 0) {
                            //         Battleshipstatemachine = playerturn;
                            //         MainMenuchangeflag     = 1;
                            //         delaySMflag            = 1;
                            //         delaySM                = menuinputdelay;
                            //         printf("opponent turn");
                            //     }
                            //     break;

                        default: break;
                    }

                    if (BSplayerboard[playerboats[0]] == boathit)
                        BSplayerboard[playerboats[0]] = boatdestroyed;
                    if (BSplayerboard[playerboats[1]] == boathit && BSplayerboard[playerboats[2]] == boathit) {
                        BSplayerboard[playerboats[1]] = boatdestroyed;
                        BSplayerboard[playerboats[2]] = boatdestroyed;
                    }
                    if (BSplayerboard[playerboats[3]] == boathit && BSplayerboard[playerboats[4]] == boathit &&
                        BSplayerboard[playerboats[5]] == boathit) {
                        BSplayerboard[playerboats[3]] = boatdestroyed;
                        BSplayerboard[playerboats[4]] = boatdestroyed;
                        BSplayerboard[playerboats[5]] = boatdestroyed;
                    }

                    if (BSopponentboard[opponentboats[0]] == boathit)
                        BSopponentboard[opponentboats[0]] = boatdestroyed;
                    if (BSopponentboard[opponentboats[1]] == boathit && BSopponentboard[opponentboats[2]] == boathit) {
                        BSopponentboard[opponentboats[1]] = boatdestroyed;
                        BSopponentboard[opponentboats[2]] = boatdestroyed;
                    }
                    if (BSopponentboard[opponentboats[3]] == boathit && BSopponentboard[opponentboats[4]] == boathit &&
                        BSopponentboard[opponentboats[5]] == boathit) {
                        BSopponentboard[opponentboats[3]] = boatdestroyed;
                        BSopponentboard[opponentboats[4]] = boatdestroyed;
                        BSopponentboard[opponentboats[5]] = boatdestroyed;
                    }



                    if (buttons[SWITCH_2] == SWITCH_PRESS || BSvictory != 0) {
                        MainMenustatemachine = Gameendbattleship;
                        MainMenuchangeflag   = 1;
                        delaySMflag          = 1;
                        delaySM              = menuinputdelay;
                        break;
                    }

                    if (MainMenuchangeflag == 1) {
                        bsp_apply_lut(lut_4s);
                        pax_background(gfx, 0);
                        int telegraphpos = 100;
                        DisplayTelegraph(BLACK, telegraphpos);
                        DisplayTelegraph(BLACK, pax_buf_get_width(gfx) - telegraphpos);
                        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 12, 8, 0, "Player");
                        pax_draw_text(gfx, BLACK, pax_font_sky_mono, 12, 275, 0, opponent);
                        for (int i = 0; i < 20; i++) {
                            DisplayblockstatusBS(telegraphpos, i, BSplayerboard[i]);
                            if (BSopponentboard[i] != boat)
                                DisplayblockstatusBS(pax_buf_get_width(gfx) - telegraphpos, i, BSopponentboard[i]);
                        }
                        // pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
                        DisplaySwitchesBox(SWITCH_1);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
                        // DisplaySwitchesBox(SWITCH_3);
                        // pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 125, 116, "Online");
                        // DisplaySwitchesBox(SWITCH_5);
                        // pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "Offline");
                        bsp_display_flush();
                        MainMenuchangeflag = 0;
                    }

                case Gameendbattleship:

                    if (buttons[SWITCH_1] == SWITCH_PRESS) {
                        MainMenustatemachine = MainMenuhub;
                        MainMenuchangeflag   = 1;
                        break;
                    }

                    if (MainMenuchangeflag == 1) {
                        // reset game
                        for (int i = 0; i < 20; i++) {
                            AIshotsfired[i]    = 0;
                            BSopponentboard[i] = 0;
                            BSplayerboard[i]   = 0;
                        }
                        for (int i = 0; i < 20; i++) {
                            playerboats[i]   = 0;
                            opponentboats[i] = NULL;
                        }


                        bsp_apply_lut(lut_4s);
                        pax_background(gfx, 0);
                        if (BSvictory == 1)
                            pax_draw_text(gfx, 1, pax_font_marker, 36, 80, 50, "Victory");
                        if (BSvictory == 2)
                            pax_draw_text(gfx, 1, pax_font_marker, 36, 80, 50, "Defeat");
                        DisplaySwitchesBox(SWITCH_1);
                        pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
                        bsp_display_flush();
                        BSvictory          = 0;
                        MainMenuchangeflag = 0;
                    }

                    break;
            }
        }

        if (delayLEDflag == 0)
            TextInputTelegraph();
    }
}

esp_err_t TextInputTelegraph(void) {
    uint32_t led = 0;
    switch (buttons[SWITCH_1]) {
        case SWITCH_LEFT: led |= LED_A | LED_B | LED_E | LED_H; break;
        case SWITCH_RIGHT: led |= LED_M | LED_R | LED_V | LED_Y; break;
        case SWITCH_PRESS: break;
        default: break;
    }

    switch (buttons[SWITCH_2]) {
        case SWITCH_LEFT: led |= LED_M | LED_I | LED_F | LED_D; break;
        case SWITCH_RIGHT: led |= LED_H | LED_N | LED_S | LED_W; break;
        case SWITCH_PRESS: break;
        default: break;
    }

    switch (buttons[SWITCH_3]) {
        case SWITCH_LEFT: led |= LED_R | LED_N | LED_K | LED_G; break;
        case SWITCH_RIGHT: led |= LED_E | LED_I | LED_O | LED_T; break;
        case SWITCH_PRESS: break;
        default: break;
    }

    switch (buttons[SWITCH_4]) {
        case SWITCH_LEFT: led |= LED_V | LED_S | LED_O | LED_L; break;
        case SWITCH_RIGHT: led |= LED_B | LED_F | LED_K | LED_P; break;
        case SWITCH_PRESS: break;
        default: break;
    }

    switch (buttons[SWITCH_5]) {
        case SWITCH_LEFT: led |= LED_Y | LED_W | LED_T | LED_P; break;
        case SWITCH_RIGHT: led |= LED_A | LED_D | LED_G | LED_L; break;
        case SWITCH_PRESS: break;
        default: break;
    }

    switch (specialcharacterselect) {
        case 0:
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_A);
                inputletter = 'a';
            }

            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_B);
                inputletter = 'b';
            }

            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_D);
                inputletter = 'd';
            }
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_E);
                inputletter = 'e';
            }
            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_F);
                inputletter = 'f';
            }
            if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_G);
                inputletter = 'g';
            }
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_2] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_H);
                inputletter = 'h';
            }
            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_I);
                inputletter = 'i';
            }
            if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_K);
                inputletter = 'k';
            }
            if (buttons[SWITCH_4] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_L);
                inputletter = 'l';
            }

            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_2] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_M);
                inputletter = 'm';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_N);
                inputletter = 'n';
            }
            if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_O);
                inputletter = 'o';
            }
            if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_P);
                inputletter = 'p';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_R);
                inputletter = 'r';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_S);
                inputletter = 's';
            }
            if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_T);
                inputletter = 't';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_V);
                inputletter = 'v';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_W);
                inputletter = 'w';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_Y);
                inputletter = 'y';
            }

            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_LEFT)  // C
            {
                led         = DisplaySelectedLetter(LED_E) + LED_G + LED_F + LED_H + LED_M + LED_R + LED_S + LED_T;
                inputletter = 'c';
            }
            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_LEFT)  // J
            {
                led         = DisplaySelectedLetter(LED_E) + LED_F + LED_G + LED_K + LED_O + LED_R + LED_S + LED_M;
                inputletter = 'j';
            }
            if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT)  // Q
            {
                led = DisplaySelectedLetter(LED_E) + LED_H + LED_M + LED_R + LED_S + LED_O + LED_K + LED_F + LED_T;
                inputletter = 'q';
            }
            if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT)  // U
            {
                led         = DisplaySelectedLetter(LED_H) + LED_M + LED_R + LED_S + LED_T + LED_P + LED_L;
                inputletter = 'u';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT)  // X
            {
                led         = DisplaySelectedLetter(LED_E) + LED_I + LED_G + LED_K + LED_N + LED_R + LED_O + LED_T;
                inputletter = 'x';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT)  // Z
            {
                led         = DisplaySelectedLetter(LED_E) + LED_F + LED_G + LED_K + LED_N + LED_R + LED_S + LED_T;
                inputletter = 'z';
            }
            break;

        case 1:  // CAPS letter
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_A);
                inputletter = 'A';
            }

            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_B);
                inputletter = 'B';
            }

            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_D);
                inputletter = 'D';
            }
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_E);
                inputletter = 'E';
            }
            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_F);
                inputletter = 'F';
            }
            if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_G);
                inputletter = 'G';
            }
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_2] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_H);
                inputletter = 'H';
            }
            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_I);
                inputletter = 'I';
            }
            if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_K);
                inputletter = 'K';
            }
            if (buttons[SWITCH_4] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_L);
                inputletter = 'L';
            }

            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_2] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_M);
                inputletter = 'M';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_N);
                inputletter = 'N';
            }
            if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_O);
                inputletter = 'O';
            }
            if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_P);
                inputletter = 'P';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_R);
                inputletter = 'R';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_S);
                inputletter = 'S';
            }
            if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_T);
                inputletter = 'T';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_V);
                inputletter = 'V';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_W);
                inputletter = 'W';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_Y);
                inputletter = 'Y';
            }

            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_LEFT)  // C
            {
                led         = DisplaySelectedLetter(LED_E) + LED_G + LED_F + LED_H + LED_M + LED_R + LED_S + LED_T;
                inputletter = 'C';
            }
            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_LEFT)  // J
            {
                led         = DisplaySelectedLetter(LED_E) + LED_F + LED_G + LED_K + LED_O + LED_R + LED_S + LED_M;
                inputletter = 'J';
            }
            if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT)  // Q
            {
                led = DisplaySelectedLetter(LED_E) + LED_H + LED_M + LED_R + LED_S + LED_O + LED_K + LED_F + LED_T;
                inputletter = 'Q';
            }
            if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT)  // U
            {
                led         = DisplaySelectedLetter(LED_H) + LED_M + LED_R + LED_S + LED_T + LED_P + LED_L;
                inputletter = 'U';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT)  // X
            {
                led         = DisplaySelectedLetter(LED_E) + LED_I + LED_G + LED_K + LED_N + LED_R + LED_O + LED_T;
                inputletter = 'X';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT)  // Z
            {
                led         = DisplaySelectedLetter(LED_E) + LED_F + LED_G + LED_K + LED_N + LED_R + LED_S + LED_T;
                inputletter = 'Z';
            }
            break;
        case 2:  // symbols 1
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_A);
                inputletter = '@';
            }

            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_B);
                inputletter = '#';
            }

            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_D);
                inputletter = '$';
            }
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_E);
                inputletter = '_';
            }
            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_F);
                inputletter = '&';
            }
            if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_G);
                inputletter = '-';
            }
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_2] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_H);
                inputletter = '+';
            }
            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_I);
                inputletter = '(';
            }
            if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_K);
                inputletter = ')';
            }
            if (buttons[SWITCH_4] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_L);
                inputletter = '/';
            }

            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_2] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_M);
                inputletter = '*';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_N);
                inputletter = '"';
            }
            if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_O);
                inputletter = '\'';
            }
            if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_P);
                inputletter = ':';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_R);
                inputletter = ';';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_S);
                inputletter = '!';
            }
            if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_T);
                inputletter = '?';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_V);
                inputletter = '~';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_W);
                inputletter = '`';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_Y);
                inputletter = '|';
            }
            break;
        case 3:  // symbols 2
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_A);
                inputletter = '=';
            }

            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_B);
                inputletter = '[';
            }

            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_D);
                inputletter = ']';
            }
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_E);
                inputletter = '\\';
            }
            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_F);
                inputletter = '{';
            }
            if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_G);
                inputletter = '}';
            }
            if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_2] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_H);
                inputletter = '<';
            }
            if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_I);
                inputletter = '>';
            }
            if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_K);
                inputletter = ' ';
            }
            if (buttons[SWITCH_4] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT) {
                led         = DisplaySelectedLetter(LED_L);
                inputletter = ' ';
            }

            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_2] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_M);
                inputletter = ' ';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_N);
                inputletter = ' ';
            }
            if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_O);
                inputletter = ' ';
            }
            if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_P);
                inputletter = ' ';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_R);
                inputletter = ' ';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_S);
                inputletter = ' ';
            }
            if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_T);
                inputletter = ' ';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_V);
                inputletter = ' ';
            }
            if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_W);
                inputletter = ' ';
            }
            if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT) {
                led         = DisplaySelectedLetter(LED_Y);
                inputletter = ' ';
            }
            break;
    }

    esp_err_t _res = bsp_set_leds(led);
    if (_res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LEDs (%d)\n", _res);
    }
    return _res;
}
