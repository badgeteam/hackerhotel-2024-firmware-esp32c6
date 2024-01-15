#include "driver/dedic_gpio.h"
#include "driver/gpio.h"
#include "epaper.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
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

#include "bsp.h"

#define LutFull 0
#define Lut8s 1
#define Lut4s 2
#define Lut1s 3
#define LutBlack 4
#define LutRed 5
#define LutWhite 6
#define LutRedfromBlack 7

#define MainMenuhub 0
#define MainMenubattleship 1
#define MainMenuBadgetag 2
#define MainMenuSettings 3
#define MainMenuCredits 4

static char const *TAG = "main";

uint8_t lut_test[] = {
    0x08,  // 0
    0x99,  // 1
    0x21,  // 2
    0x44,  // 3
    0x40,  // 4
    0x01,  // 5
    0x00,  // 6
    0x10,  // 7
    0x99,  // 8
    0x21,  // 9
    0xa0,  // 10
    0x20,  // 11
    0x20,  // 12
    0x00,  // 13
    0x88,  // 14
    0x99,  // 15
    0x21,  // 16
    0x44,  // 17
    0x2b,  // 18
    0x2f,  // 19
    0x00,  // 20
    0x88,  // 21
    0x99,  // 22
    0x21,  // 23
    0x44,  // 24
    0x2b,  // 25
    0x2f,  // 26
    0x00,  // 27
    0x00,  // 28
    0x00,  // 29
    0x12,  // 30
    0x40,  // 31
    0x00,  // 32
    0x00,  // 33
    0x00,  // 34

    //  TPxA  TPxB  TPxC  TPxD  RPx
    0x36,
    0x30,
    0x33,
    0x00,
    0x01,
    0x02,
    0x02,
    0x02,
    0x02,
    0x13,
    0x01,
    0x16,
    0x01,
    0x16,
    0x04,
    0x02,
    0x0b,
    0x10,
    0x00,
    0x03,
    0x06,
    0x04,
    0x02,
    0x2b,
    0x04,
    0x14,
    0x04,
    0x23,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,

    0x13,  // VGH
    0x3c,  // VSH1
    0xc0,  // VSH2
    0x2e,  // VSL
    0x30,  // Frame 1
    0x0a,  // Frame 2
};
lut7_t *lut = (lut7_t *)lut_test;

uint8_t buttons[5];
int delaySM = 1;
int delaySMflag = 0;
int delayLED = 0;
int delayLEDflag = 0;
int MainMenustatemachine = MainMenuhub;
int MainMenuchangeflag = 1;
char inputletter = NULL;
char playername[20] = "";
int specialcharacterselect = 0;
char specialcharactersicon[4][20] = {"A/!", "CAPS", "!?", "<|>"};

esp_err_t TextInputTelegraph(void);

int DisplaySelectedLetter(int _selectedletter)
{
    delayLED = 500;
    delayLEDflag = 1;
    return _selectedletter;
}

void DisplayDebugCenterlines(void) // in black
{
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_draw_line(gfx, 1, 147, 0, 147, 127);
    pax_draw_line(gfx, 1, 148, 0, 148, 127);
    pax_draw_line(gfx, 1, 0, 63, 295, 63);
    pax_draw_line(gfx, 1, 0, 64, 295, 64);
}

void DisplayDebugSwitchesBoxes(void) // in black
{
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_outline_rect(gfx, 1, 0, 114, 52, 14); // Switch 1
    // pax_draw_rect(gfx, 0, 1, 115, 50, 12);

    pax_outline_rect(gfx, 1, 61, 114, 52, 14); // Switch 2
    // pax_draw_rect(gfx, 0, 62, 115, 50, 12);

    pax_outline_rect(gfx, 1, 61 * 2, 114, 52, 14); // Switch 3
    // pax_draw_rect(gfx, 0, 1 + 61 * 2, 115, 50, 12);

    pax_outline_rect(gfx, 1, 61 * 3, 114, 52, 14); // Switch 4
    // pax_draw_rect(gfx, 0, 1 + 61 * 3, 115, 50, 12);

    pax_outline_rect(gfx, 1, 61 * 4, 114, 52, 14); // Switch 5
    // pax_draw_rect(gfx, 0, 1 + 61 * 4, 115, 50, 12);
}

void DisplaySwitchesBox(int _switch) // in black
{
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    pax_outline_rect(gfx, 1, 61 * _switch, 114, 50, 12);
}

// Position is the X coordinate of the center/left (since it's even) pixel
void DisplayTelegraph(int _colour, int _position)
{
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    if (_position < 36)
        _position = 36; // prevent to draw outside of buffer

    // Diamond
    pax_draw_line(gfx, _colour, _position - 36, 64, _position - 3, 127); // Left to top
    pax_draw_line(gfx, _colour, _position - 36, 63, _position - 3, 0);   // Left to bottom
    pax_draw_line(gfx, _colour, _position + 37, 64, _position + 4, 127); // Right to top
    pax_draw_line(gfx, _colour, _position + 37, 63, _position + 4, 0);   // Right to bottom

    // Letter circles

    pax_outline_circle(gfx, _colour, _position, 12, 6);

    pax_outline_circle(gfx, _colour, _position - 8, 27, 6);
    pax_outline_circle(gfx, _colour, _position + 8, 27, 6);

    pax_outline_circle(gfx, _colour, _position - 16, 42, 6);
    pax_outline_circle(gfx, _colour, _position, 42, 6);
    pax_outline_circle(gfx, _colour, _position + 16, 42, 6);

    pax_outline_circle(gfx, _colour, _position - 24, 57, 6);
    pax_outline_circle(gfx, _colour, _position - 8, 57, 6);
    pax_outline_circle(gfx, _colour, _position + 8, 57, 6);
    pax_outline_circle(gfx, _colour, _position + 24, 57, 6);

    pax_outline_circle(gfx, _colour, _position - 24, 71, 6);
    pax_outline_circle(gfx, _colour, _position - 8, 71, 6);
    pax_outline_circle(gfx, _colour, _position + 8, 71, 6);
    pax_outline_circle(gfx, _colour, _position + 24, 71, 6);

    pax_outline_circle(gfx, _colour, _position - 16, 86, 6);
    pax_outline_circle(gfx, _colour, _position, 86, 6);
    pax_outline_circle(gfx, _colour, _position + 16, 86, 6);

    pax_outline_circle(gfx, _colour, _position - 8, 101, 6);
    pax_outline_circle(gfx, _colour, _position + 8, 101, 6);

    pax_outline_circle(gfx, _colour, _position, 116, 6);
}

void testaa(void)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (delaySMflag == 1)
        {
            vTaskDelay(pdMS_TO_TICKS(delaySM));
            delaySMflag = 0;
        }

        if (delayLEDflag == 1)
        {
            vTaskDelay(pdMS_TO_TICKS(delayLED));
            delayLEDflag = 0;
        }
    }
}

void LUTset(int LUTconfiguration)
{
    switch (LUTconfiguration)
    {
    case LutFull: // uses all groups
        lut->groups[0].repeat = 0x01;
        lut->groups[1].repeat = 0x13;
        lut->groups[2].repeat = 0x04;
        lut->groups[3].repeat = 0x03;
        lut->groups[4].repeat = 0x04;
        lut->groups[5].repeat = 0x03;
        // lut->groups[6].repeat = 0x00;

        lut->groups[0].tp[0] = 0x36;
        lut->groups[0].tp[1] = 0x30;
        lut->groups[0].tp[2] = 0x33;
        lut->groups[0].tp[3] = 0x00;

        lut->groups[1].tp[0] = 0x02;
        lut->groups[1].tp[1] = 0x02;
        lut->groups[1].tp[2] = 0x02;
        lut->groups[1].tp[3] = 0x02;

        lut->groups[2].tp[0] = 0x01;
        lut->groups[2].tp[1] = 0x16;
        lut->groups[2].tp[2] = 0x01;
        lut->groups[2].tp[3] = 0x16;

        lut->groups[3].tp[0] = 0x02;
        lut->groups[3].tp[1] = 0x0b;
        lut->groups[3].tp[2] = 0x10;
        lut->groups[3].tp[3] = 0x00;

        lut->groups[4].tp[0] = 0x06;
        lut->groups[4].tp[1] = 0x04;
        lut->groups[4].tp[2] = 0x02;
        lut->groups[4].tp[3] = 0x2b;

        lut->groups[5].tp[0] = 0x14;
        lut->groups[5].tp[1] = 0x04;
        lut->groups[5].tp[2] = 0x23;
        lut->groups[5].tp[3] = 0x02;
        break;

    case Lut8s: // uses all groups without repeat
        lut->groups[0].repeat = 0;
        lut->groups[1].repeat = 0;
        lut->groups[2].repeat = 0;
        lut->groups[3].repeat = 0;
        lut->groups[4].repeat = 0;
        lut->groups[5].repeat = 0;
        // lut->groups[6].repeat = 0x00;

        lut->groups[0].tp[0] = 0x36;
        lut->groups[0].tp[1] = 0x30;
        lut->groups[0].tp[2] = 0x33;
        lut->groups[0].tp[3] = 0x00;

        lut->groups[1].tp[0] = 0x02;
        lut->groups[1].tp[1] = 0x02;
        lut->groups[1].tp[2] = 0x02;
        lut->groups[1].tp[3] = 0x02;

        lut->groups[2].tp[0] = 0x01;
        lut->groups[2].tp[1] = 0x16;
        lut->groups[2].tp[2] = 0x01;
        lut->groups[2].tp[3] = 0x16;

        lut->groups[3].tp[0] = 0x02;
        lut->groups[3].tp[1] = 0x0b;
        lut->groups[3].tp[2] = 0x10;
        lut->groups[3].tp[3] = 0x00;

        lut->groups[4].tp[0] = 0x06;
        lut->groups[4].tp[1] = 0x04;
        lut->groups[4].tp[2] = 0x02;
        lut->groups[4].tp[3] = 0x2b;

        lut->groups[5].tp[0] = 0x14;
        lut->groups[5].tp[1] = 0x04;
        lut->groups[5].tp[2] = 0x23;
        lut->groups[5].tp[3] = 0x02;
        break;

    case Lut4s: // uses groups 3,4 and 5 without repeat
        lut->groups[0].repeat = 0;
        lut->groups[1].repeat = 0;
        lut->groups[2].repeat = 0;
        lut->groups[3].repeat = 0;
        lut->groups[4].repeat = 0;
        lut->groups[5].repeat = 0;
        // lut->groups[6].repeat = 0x00;

        lut->groups[0].tp[0] = 0;
        lut->groups[0].tp[1] = 0;
        lut->groups[0].tp[2] = 0;
        lut->groups[0].tp[3] = 0;

        lut->groups[1].tp[0] = 0;
        lut->groups[1].tp[1] = 0;
        lut->groups[1].tp[2] = 0;
        lut->groups[1].tp[3] = 0;

        lut->groups[2].tp[0] = 0;
        lut->groups[2].tp[1] = 0;
        lut->groups[2].tp[2] = 0;
        lut->groups[2].tp[3] = 0;

        lut->groups[3].tp[0] = 0x02;
        lut->groups[3].tp[1] = 0x0b;
        lut->groups[3].tp[2] = 0x10;
        lut->groups[3].tp[3] = 0x00;

        lut->groups[4].tp[0] = 0x06;
        lut->groups[4].tp[1] = 0x04;
        lut->groups[4].tp[2] = 0x02;
        lut->groups[4].tp[3] = 0x2b;

        lut->groups[5].tp[0] = 0x14;
        lut->groups[5].tp[1] = 0x04;
        lut->groups[5].tp[2] = 0x23;
        lut->groups[5].tp[3] = 0x02;
        break;

    case Lut1s: // uses all groups
        lut->groups[0].repeat = 0;
        lut->groups[1].repeat = 0;
        lut->groups[2].repeat = 0;
        lut->groups[3].repeat = 0;
        lut->groups[4].repeat = 0;
        lut->groups[5].repeat = 0;
        // lut->groups[6].repeat = 0x00;

        lut->groups[0].tp[0] = 0;
        lut->groups[0].tp[1] = 0;
        lut->groups[0].tp[2] = 0;
        lut->groups[0].tp[3] = 0;

        lut->groups[1].tp[0] = 0;
        lut->groups[1].tp[1] = 0;
        lut->groups[1].tp[2] = 0;
        lut->groups[1].tp[3] = 0;

        lut->groups[2].tp[0] = 0;
        lut->groups[2].tp[1] = 0;
        lut->groups[2].tp[2] = 0;
        lut->groups[2].tp[3] = 0;

        lut->groups[3].tp[0] = 0x02;
        lut->groups[3].tp[1] = 0x0b;
        lut->groups[3].tp[2] = 0x10;
        lut->groups[3].tp[3] = 0x00;

        lut->groups[4].tp[0] = 0x06;
        lut->groups[4].tp[1] = 0x04;
        lut->groups[4].tp[2] = 0x02;
        lut->groups[4].tp[3] = 0x2b;

        lut->groups[5].tp[0] = 0;
        lut->groups[5].tp[1] = 0;
        lut->groups[5].tp[2] = 0;
        lut->groups[5].tp[3] = 0;
        break;

    case LutWhite: // uses groups 3,4 and 5 without repeat
        lut->groups[0].repeat = 0;
        lut->groups[1].repeat = 0;
        lut->groups[2].repeat = 0;
        lut->groups[3].repeat = 0;
        lut->groups[4].repeat = 0;
        lut->groups[5].repeat = 0;
        // lut->groups[6].repeat = 0x00;

        lut->groups[0].tp[0] = 0;
        lut->groups[0].tp[1] = 0;
        lut->groups[0].tp[2] = 0;
        lut->groups[0].tp[3] = 0;

        lut->groups[1].tp[0] = 0;
        lut->groups[1].tp[1] = 0;
        lut->groups[1].tp[2] = 0;
        lut->groups[1].tp[3] = 0;

        lut->groups[2].tp[0] = 0;
        lut->groups[2].tp[1] = 0;
        lut->groups[2].tp[2] = 0;
        lut->groups[2].tp[3] = 0;

        lut->groups[3].tp[0] = 0x02;
        lut->groups[3].tp[1] = 0x0b;
        lut->groups[3].tp[2] = 0;
        lut->groups[3].tp[3] = 0;

        lut->groups[4].tp[0] = 0;
        lut->groups[4].tp[1] = 0;
        lut->groups[4].tp[2] = 0;
        lut->groups[4].tp[3] = 0;

        lut->groups[5].tp[0] = 0;
        lut->groups[5].tp[1] = 0;
        lut->groups[5].tp[2] = 0;
        lut->groups[5].tp[3] = 0;
        break;
    }
    hink_set_lut(bsp_get_epaper(), (uint8_t *)lut);
}

void framenametag(void)
{
    LUTset(Lut1s);
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
    if (specialcharacterselect == 2)
    {
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

    if (specialcharacterselect == 3)
    {
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
    hink_write(bsp_get_epaper(), gfx->buf);
}

unsigned long millis() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void app_main(void) {
    esp_err_t res = bsp_init();
    if (res != ESP_OK) {
        // Device init failed, stop.
        ESP_LOGE(TAG, "Hardware initialization failed, bailing out.");
        //vTaskDelay(pdMS_TO_TICKS(2000));
        //bsp_restart();
        return;
    }

    QueueHandle_t queue = bsp_get_button_queue();

    /*
    // This example shows how to wait for button events
    while(1) {
        coprocessor_input_message_t buttonMessage = {0};
        if (xQueueReceive(queue, &buttonMessage, portMAX_DELAY) == pdTRUE) {
            printf("Button %u state changed to %u\n", buttonMessage.button, buttonMessage.state);
        }
    }
    */

    LUTset(Lut4s);

    xTaskCreate(testaa, "testaa", 1024, NULL, 1, NULL);

    while (1) {
        pax_buf_t* gfx = bsp_get_gfx_buffer();
        bool busy = hink_busy(bsp_get_epaper());

        // Quick hack to convert the new button queue back into the old polling method
        coprocessor_input_message_t buttonMessage = {0};
        if (xQueueReceive(queue, &buttonMessage, 0) == pdTRUE) {
            printf("Button %u state changed to %u\n", buttonMessage.button, buttonMessage.state);
            buttons[buttonMessage.button] = buttonMessage.state;
        }

        if (!busy) {
             switch (MainMenustatemachine) {
                case MainMenuhub:

                 if (buttons[SWITCH_2] == SWITCH_PRESS) {
                     MainMenustatemachine = MainMenuBadgetag;
                     MainMenuchangeflag = 1;
                     break;
                 }

                 if (buttons[SWITCH_3] == SWITCH_PRESS) {
                     MainMenustatemachine = MainMenuCredits;
                     MainMenuchangeflag = 1;
                     break;
                 }

                 if (buttons[SWITCH_4] == SWITCH_PRESS) {
                     MainMenustatemachine = MainMenuSettings;
                     MainMenuchangeflag = 1;
                     break;
                 }

                 if (buttons[SWITCH_5] == SWITCH_PRESS) {
                     MainMenustatemachine = MainMenubattleship;
                     MainMenuchangeflag = 1;
                     break;
                 }
                 if (MainMenuchangeflag == 1) {
                     LUTset(Lut4s);
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
                     hink_write(bsp_get_epaper(), gfx->buf);
                     MainMenuchangeflag = 0;
                 }
                 break;

             case MainMenuBadgetag:

                 if (buttons[SWITCH_1] == SWITCH_PRESS)
                 {
                     MainMenustatemachine = MainMenuhub;
                     MainMenuchangeflag = 1;
                     break;
                 }
                 if (buttons[SWITCH_3] == SWITCH_PRESS)
                 {
                     specialcharacterselect++;
                     if (specialcharacterselect > 3)
                         specialcharacterselect = 0;
                     framenametag();
                 }
                  //specialcharacterselect

                 if (buttons[SWITCH_5] == SWITCH_PRESS)
                 {
                     playername[strlen(playername) - 1] = '\0';
                     framenametag();
                 }

                 if (MainMenuchangeflag == 1)
                 {
                     inputletter = NULL;
                     framenametag();
                     MainMenuchangeflag = 0;
                 }

                 if (inputletter != NULL)
                 {
                     strncat(playername, &inputletter, 1);
                     specialcharacterselect = 0;
                     framenametag();
                     inputletter = NULL;
                 }

             case MainMenuSettings:
                 if (buttons[SWITCH_1] == SWITCH_PRESS)
                 {
                     MainMenustatemachine = MainMenuhub;
                     MainMenuchangeflag = 1;
                     break;
                 }
                 if (MainMenuchangeflag == 1)
                 {
                     LUTset(Lut4s);
                     pax_background(gfx, 0);
                     pax_draw_text(gfx, 1, pax_font_marker, 18, 1, 0, "Settings placeholder");
                     DisplaySwitchesBox(SWITCH_1);
                     pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
                     // DisplaySwitchesBox(SWITCH_3);
                     // pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 125, 116, "Online");
                     // DisplaySwitchesBox(SWITCH_5);
                     // pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "Offline");
                     hink_write(bsp_get_epaper(), gfx->buf);
                     MainMenuchangeflag = 0;
                 }
                 break;

             case MainMenuCredits:
                 if (buttons[SWITCH_1] == SWITCH_PRESS)
                 {
                     MainMenustatemachine = MainMenuhub;
                     MainMenuchangeflag = 1;
                     break;
                 }
                 if (MainMenuchangeflag == 1)
                 {
                     LUTset(Lut4s);
                     pax_background(gfx, 0);
                     pax_draw_text(gfx, 1, pax_font_marker, 18, 1, 0, "Credits placeholder");
                     DisplaySwitchesBox(SWITCH_1);
                     pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
                     // DisplaySwitchesBox(SWITCH_3);
                     // pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 125, 116, "Online");
                     // DisplaySwitchesBox(SWITCH_5);
                     // pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "Offline");
                     hink_write(bsp_get_epaper(), gfx->buf);
                     MainMenuchangeflag = 0;
                 }
                 break;

             case MainMenubattleship:
                 if (buttons[SWITCH_1] == SWITCH_PRESS)
                 {
                     MainMenustatemachine = MainMenuhub;
                     MainMenuchangeflag = 1;
                     break;
                 }
                 if (MainMenuchangeflag == 1)
                 {
                     LUTset(Lut4s);
                     pax_background(gfx, 0);
                     pax_draw_text(gfx, 1, pax_font_marker, 18, 1, 0, "Battleship");
                     DisplaySwitchesBox(SWITCH_1);
                     pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
                     DisplaySwitchesBox(SWITCH_3);
                     pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 125, 116, "Online");
                     DisplaySwitchesBox(SWITCH_5);
                     pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "Offline");
                     hink_write(bsp_get_epaper(), gfx->buf);
                     MainMenuchangeflag = 0;
                 }

                 break;
             }
         }

        if (delayLEDflag == 0) res = TextInputTelegraph();

        gpio_set_level(
            GPIO_RELAY,
            buttons[0] | buttons[1] | buttons[2] | buttons[3] | buttons[4]
        );  // Turn on relay if one of the buttons is activated
    }
}

esp_err_t TextInputTelegraph(void)
{
    uint32_t led = 0;
    switch (buttons[SWITCH_1])
    {
    case SWITCH_LEFT:
        led |= LED_A | LED_B | LED_E | LED_H;
        break;
    case SWITCH_RIGHT:
        led |= LED_M | LED_R | LED_V | LED_Y;
        break;
    case SWITCH_PRESS:
        break;
    default:
        break;
    }

    switch (buttons[SWITCH_2])
    {
    case SWITCH_LEFT:
        led |= LED_M | LED_I | LED_F | LED_D;
        break;
    case SWITCH_RIGHT:
        led |= LED_H | LED_N | LED_S | LED_W;
        break;
    case SWITCH_PRESS:
        break;
    default:
        break;
    }

    switch (buttons[SWITCH_3])
    {
    case SWITCH_LEFT:
        led |= LED_R | LED_N | LED_K | LED_G;
        break;
    case SWITCH_RIGHT:
        led |= LED_E | LED_I | LED_O | LED_T;
        break;
    case SWITCH_PRESS:
        break;
    default:
        break;
    }

    switch (buttons[SWITCH_4])
    {
    case SWITCH_LEFT:
        led |= LED_V | LED_S | LED_O | LED_L;
        break;
    case SWITCH_RIGHT:
        led |= LED_B | LED_F | LED_K | LED_P;
        break;
    case SWITCH_PRESS:
        break;
    default:
        break;
    }

    switch (buttons[SWITCH_5])
    {
    case SWITCH_LEFT:
        led |= LED_Y | LED_W | LED_T | LED_P;
        break;
    case SWITCH_RIGHT:
        led |= LED_A | LED_D | LED_G | LED_L;
        break;
    case SWITCH_PRESS:
        break;
    default:
        break;
    }

    switch (specialcharacterselect)
    {
    case 0:
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_A);
            inputletter = 'a';
        }

        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_B);
            inputletter = 'b';
        }

        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_D);
            inputletter = 'd';
        }
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_E);
            inputletter = 'e';
        }
        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_F);
            inputletter = 'f';
        }
        if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_G);
            inputletter = 'g';
        }
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_2] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_H);
            inputletter = 'h';
        }
        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_I);
            inputletter = 'i';
        }
        if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_K);
            inputletter = 'k';
        }
        if (buttons[SWITCH_4] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_L);
            inputletter = 'l';
        }

        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_2] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_M);
            inputletter = 'm';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_N);
            inputletter = 'n';
        }
        if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_O);
            inputletter = 'o';
        }
        if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_P);
            inputletter = 'p';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_R);
            inputletter = 'r';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_S);
            inputletter = 's';
        }
        if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_T);
            inputletter = 't';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_V);
            inputletter = 'v';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_W);
            inputletter = 'w';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_Y);
            inputletter = 'y';
        }

        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_LEFT) // C
        {
            led = DisplaySelectedLetter(LED_E) + LED_G + LED_F + LED_H + LED_M + LED_R + LED_S + LED_T;
            inputletter = 'c';
        }
        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_LEFT) // J
        {
            led = DisplaySelectedLetter(LED_E) + LED_F + LED_G + LED_K + LED_O + LED_R + LED_S + LED_M;
            inputletter = 'j';
        }
        if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT) // Q
        {
            led = DisplaySelectedLetter(LED_E) + LED_H + LED_M + LED_R + LED_S + LED_O + LED_K + LED_F + LED_T;
            inputletter = 'q';
        }
        if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT) // U
        {
            led = DisplaySelectedLetter(LED_H) + LED_M + LED_R + LED_S + LED_T + LED_P + LED_L;
            inputletter = 'u';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT) // X
        {
            led = DisplaySelectedLetter(LED_E) + LED_I + LED_G + LED_K + LED_N + LED_R + LED_O + LED_T;
            inputletter = 'x';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT) // Z
        {
            led = DisplaySelectedLetter(LED_E) + LED_F + LED_G + LED_K + LED_N + LED_R + LED_S + LED_T;
            inputletter = 'z';
        }
        break;

    case 1: // CAPS letter
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_A);
            inputletter = 'A';
        }

        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_B);
            inputletter = 'B';
        }

        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_D);
            inputletter = 'D';
        }
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_E);
            inputletter = 'E';
        }
        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_F);
            inputletter = 'F';
        }
        if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_G);
            inputletter = 'G';
        }
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_2] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_H);
            inputletter = 'H';
        }
        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_I);
            inputletter = 'I';
        }
        if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_K);
            inputletter = 'K';
        }
        if (buttons[SWITCH_4] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_L);
            inputletter = 'L';
        }

        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_2] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_M);
            inputletter = 'M';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_N);
            inputletter = 'N';
        }
        if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_O);
            inputletter = 'O';
        }
        if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_P);
            inputletter = 'P';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_R);
            inputletter = 'R';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_S);
            inputletter = 'S';
        }
        if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_T);
            inputletter = 'T';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_V);
            inputletter = 'V';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_W);
            inputletter = 'W';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_Y);
            inputletter = 'Y';
        }

        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_LEFT) // C
        {
            led = DisplaySelectedLetter(LED_E) + LED_G + LED_F + LED_H + LED_M + LED_R + LED_S + LED_T;
            inputletter = 'C';
        }
        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_LEFT) // J
        {
            led = DisplaySelectedLetter(LED_E) + LED_F + LED_G + LED_K + LED_O + LED_R + LED_S + LED_M;
            inputletter = 'J';
        }
        if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT) // Q
        {
            led = DisplaySelectedLetter(LED_E) + LED_H + LED_M + LED_R + LED_S + LED_O + LED_K + LED_F + LED_T;
            inputletter = 'Q';
        }
        if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT) // U
        {
            led = DisplaySelectedLetter(LED_H) + LED_M + LED_R + LED_S + LED_T + LED_P + LED_L;
            inputletter = 'U';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT) // X
        {
            led = DisplaySelectedLetter(LED_E) + LED_I + LED_G + LED_K + LED_N + LED_R + LED_O + LED_T;
            inputletter = 'X';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_RIGHT) // Z
        {
            led = DisplaySelectedLetter(LED_E) + LED_F + LED_G + LED_K + LED_N + LED_R + LED_S + LED_T;
            inputletter = 'Z';
        }
        break;
    case 2: // symbols 1
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_A);
            inputletter = '@';
        }

        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_B);
            inputletter = '#';
        }

        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_D);
            inputletter = '$';
        }
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_E);
            inputletter = '_';
        }
        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_F);
            inputletter = '&';
        }
        if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_G);
            inputletter = '-';
        }
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_2] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_H);
            inputletter = '+';
        }
        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_I);
            inputletter = '(';
        }
        if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_K);
            inputletter = ')';
        }
        if (buttons[SWITCH_4] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_L);
            inputletter = '/';
        }

        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_2] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_M);
            inputletter = '*';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_N);
            inputletter = '"';
        }
        if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_O);
            inputletter = '\'';
        }
        if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_P);
            inputletter = ':';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_R);
            inputletter = ';';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_S);
            inputletter = '!';
        }
        if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_T);
            inputletter = '?';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_V);
            inputletter = '~';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_W);
            inputletter = '`';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_Y);
            inputletter = '|';
        }
        break;
    case 3: // symbols 2
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_A);
            inputletter = '=';
        }

        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_B);
            inputletter = '[';
        }

        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_D);
            inputletter = ']';
        }
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_E);
            inputletter = '\\';
        }
        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_F);
            inputletter = '{';
        }
        if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_G);
            inputletter = '}';
        }
        if (buttons[SWITCH_1] == SWITCH_LEFT && buttons[SWITCH_2] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_H);
            inputletter = '<';
        }
        if (buttons[SWITCH_2] == SWITCH_LEFT && buttons[SWITCH_3] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_I);
            inputletter = '>';
        }
        if (buttons[SWITCH_3] == SWITCH_LEFT && buttons[SWITCH_4] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_K);
            inputletter = ' ';
        }
        if (buttons[SWITCH_4] == SWITCH_LEFT && buttons[SWITCH_5] == SWITCH_RIGHT)
        {
            led = DisplaySelectedLetter(LED_L);
            inputletter = ' ';
        }

        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_2] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_M);
            inputletter = ' ';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_N);
            inputletter = ' ';
        }
        if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_O);
            inputletter = ' ';
        }
        if (buttons[SWITCH_4] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_P);
            inputletter = ' ';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_3] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_R);
            inputletter = ' ';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_S);
            inputletter = ' ';
        }
        if (buttons[SWITCH_3] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_T);
            inputletter = ' ';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_4] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_V);
            inputletter = ' ';
        }
        if (buttons[SWITCH_2] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_W);
            inputletter = ' ';
        }
        if (buttons[SWITCH_1] == SWITCH_RIGHT && buttons[SWITCH_5] == SWITCH_LEFT)
        {
            led = DisplaySelectedLetter(LED_Y);
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