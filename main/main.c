#include "board.h"
#include "driver/dedic_gpio.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
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
#include "managed_i2c.h"
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

#include "ch32.h"

#define I2C_REG_FW_VERSION 0
#define I2C_REG_LED        4
#define I2C_REG_BTN        8

static char const *TAG = "main";

extern pax_buf_t     gfx;
extern sdmmc_card_t *card;
extern hink_t        epaper;


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

uint8_t buttons[5];

void frame1(void) {
    pax_background(&gfx, 0);
    pax_draw_text(&gfx, 1, pax_font_marker, 18, 60, 0, "Welcome to Hackerhotel");
    hink_write(&epaper, gfx.buf);
}

void app_main(void) {
    esp_err_t res = initialize_system();
    if (res != ESP_OK) {
        // Device init failed, stop.
        ESP_LOGE(TAG, "System init failed, bailing out.");
        return;
    }

    if (card) {
        sdmmc_card_print_info(stdout, card);
    } else {
        ESP_LOGI(TAG, "No SD card found");
    }

    // //  TPxA  TPxB  TPxC  TPxD  RPx
    // 0x36, 0x30, 0x33, 0x00, 0x01,
    // 0x02, 0x02, 0x02, 0x02, 0x13,
    // 0x01, 0x16, 0x01, 0x16, 0x04,
    // 0x02, 0x0b, 0x10, 0x00, 0x03,
    // 0x06, 0x04, 0x02, 0x2b, 0x04,
    // 0x14, 0x04, 0x23, 0x02, 0x03,
    // 0x00, 0x00, 0x00, 0x00, 0x00,

    lut7_t *lut = (lut7_t *)lut_test;

    lut->groups[0].repeat = 0;
    lut->groups[1].repeat = 0;
    lut->groups[2].repeat = 0;
    lut->groups[3].repeat = 0;
    lut->groups[4].repeat = 0;
    // lut->groups[5].repeat = 0;
    // lut->groups[6].repeat = 0;

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

    lut->groups[5].tp[0] = 0;
    lut->groups[5].tp[1] = 0;
    lut->groups[5].tp[2] = 0;
    lut->groups[5].tp[3] = 0;

    // lut->groups[6].tp[0] = 0x14;
    // lut->groups[6].tp[1] = 0x04;
    // lut->groups[6].tp[2] = 0x23;
    // lut->groups[6].tp[3] = 0;

    hink_set_lut(&epaper, (uint8_t *)lut);
    // hink_write(&epaper, gfx.buf);

    bool          state           = false;
    unsigned long previous_millis = 0;

    //xTaskCreate(testaa, "testaa", 1024, NULL, 1, NULL);

    while (1) {
        bool busy = hink_busy(&epaper);

        res = i2c_read_reg(I2C_BUS, 0x42, I2C_REG_BTN, buttons, 5);
        if (res == ESP_OK) {
            if (buttons[0] | buttons[1] | buttons[2] | buttons[3] | buttons[4]) {
                printf("BUTTONS %x %x %x %x %x\n", buttons[0], buttons[1], buttons[2], buttons[3], buttons[4]);
            }
        }

        if (!busy) {
            unsigned long current_millis = millis();
            unsigned long time_passed    = current_millis - previous_millis;
            if (previous_millis) {
                printf("Updating took %lu ms\n", time_passed);
            }
            previous_millis = millis();
            state           = !state;
            if (state) {
                pax_background(&gfx, 0);
                pax_draw_text(&gfx, 1, pax_font_marker, 18, 1, 0, "Hackerhotel 2024");
                pax_draw_text(&gfx, 1, pax_font_sky_mono, 12, 1, 50, "This text is black");
                pax_draw_text(&gfx, 2, pax_font_sky_mono, 12, 1, 65, "This text is red");
                pax_draw_rect(&gfx, 0, 0, 95, 100, 32);   // White
                pax_draw_rect(&gfx, 1, 100, 95, 98, 32);  // Black
                pax_draw_rect(&gfx, 2, 198, 95, 98, 32);  // Red
            } else {
                pax_background(&gfx, 0);
                pax_draw_text(&gfx, 1, pax_font_marker, 18, 1, 0, "Blah blah blah blah blah blah blah blah");
                pax_draw_text(&gfx, 2, pax_font_sky_mono, 12, 2, 50, "This text is red");
                pax_draw_text(&gfx, 1, pax_font_sky_mono, 12, 1, 65, "This text is black");
                pax_draw_rect(&gfx, 2, 0, 95, 100, 32);   // Red
                pax_draw_rect(&gfx, 0, 100, 95, 98, 32);  // White
                pax_draw_rect(&gfx, 1, 198, 95, 98, 32);  // Black
            }
            hink_write(&epaper, gfx.buf);
        }

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

        res = i2c_write_reg_n(I2C_BUS, 0x42, I2C_REG_LED, (uint8_t *)&led, sizeof(uint32_t));
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write to CH32 coprocessor (%d)\n", res);
        }

        gpio_set_level(
            GPIO_RELAY,
            buttons[0] | buttons[1] | buttons[2] | buttons[3] | buttons[4]
        );  // Turn on relay if one of the buttons is activated
    }
}
