
/**
 * Copyright (c) 2023 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "epaper.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hextools.h"
#include "include/epaper.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/spi_reg.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char* TAG = "epaper";

static uint8_t lut_buffer[HINK_LUT_SIZE];

static void bitbang_tx(int pin_data, int pin_clk, uint8_t data) {
    gpio_set_direction(pin_data, GPIO_MODE_OUTPUT);
    for (int i = 0; i < 8; i++) {
        gpio_set_level(pin_data, data & 0x80);
        data <<= 1;
        esp_rom_delay_us(1);
        gpio_set_level(pin_clk, 1);
        esp_rom_delay_us(1);
        gpio_set_level(pin_clk, 0);
    }
}

static uint8_t bitbang_rx(int pin_data, int pin_clk) {
    gpio_set_direction(pin_data, GPIO_MODE_INPUT);
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        esp_rom_delay_us(1);
        gpio_set_level(pin_clk, 1);
        data <<= 1;
        data  |= !!gpio_get_level(pin_data);
        esp_rom_delay_us(1);
        gpio_set_level(pin_clk, 0);
    }
    return data;
}

static void bitbang_write_cmd(int pin_data, int pin_clk, int pin_dc, uint8_t data) {
    gpio_set_level(pin_dc, false);
    bitbang_tx(pin_data, pin_clk, data);
}

static void bitbang_write_data(int pin_data, int pin_clk, int pin_dc, uint8_t data) {
    gpio_set_level(pin_dc, true);
    bitbang_tx(pin_data, pin_clk, data);
}

esp_err_t hink_read_lut(int pin_data, int pin_clk, int pin_cs, int pin_dc, int pin_reset, int pin_busy) {
    esp_err_t res;

    // Initialize pins
    gpio_config_t output_cfg = {
        .pin_bit_mask = BIT64(pin_reset) | BIT64(pin_cs) | BIT64(pin_dc) | BIT64(pin_clk),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = false,
        .pull_down_en = false,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    res = gpio_config(&output_cfg);
    if (res != ESP_OK) {
        return res;
    }

    res = gpio_set_level(pin_reset, false);
    if (res != ESP_OK) {
        return res;
    }

    res = gpio_set_level(pin_cs, true);
    if (res != ESP_OK) {
        return res;
    }

    res = gpio_set_level(pin_dc, false);
    if (res != ESP_OK) {
        return res;
    }

    res = gpio_set_level(pin_clk, false);
    if (res != ESP_OK) {
        return res;
    }

    gpio_config_t input_cfg = {
        .pin_bit_mask = BIT64(pin_busy) | BIT64(pin_data),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = true,
        .pull_down_en = false,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    res = gpio_config(&input_cfg);
    if (res != ESP_OK) {
        return res;
    }

    // Reset display
    ESP_LOGI(TAG, "reset display");
    vTaskDelay(10 / portTICK_PERIOD_MS);
    res = gpio_set_level(pin_reset, true);
    if (res != ESP_OK) {
        return res;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_SW_RESET);
    gpio_set_level(pin_cs, true);

    uint16_t timeout = 100;

    while (gpio_get_level(pin_busy)) {
        timeout--;
        if (timeout < 1) {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // Configure
    ESP_LOGI(TAG, "configure display");

    uint16_t screen_width  = 18;
    uint16_t screen_height = 152;

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_SET_ANALOG_BLOCK_CONTROL);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x54);
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_SET_DIGITAL_BLOCK_CONTROL);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x3B);
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_UNKNOWN_1);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x04);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x63);
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_BOOSTER_SOFT_START_CONTROL);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x8F);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x8F);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x8F);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x3F);
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_DRIVER_OUTPUT_CONTROL);
    bitbang_write_data(pin_data, pin_clk, pin_dc, (screen_height - 1) & 0xFF);
    bitbang_write_data(pin_data, pin_clk, pin_dc, ((screen_height - 1) >> 8));
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_DATA_ENTRY_MODE_SETTING);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x01);
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_SET_RAM_X_ADDRESS_LIMITS);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x00);
    bitbang_write_data(pin_data, pin_clk, pin_dc, screen_width);
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_SET_RAM_Y_ADDRESS_LIMITS);
    bitbang_write_data(pin_data, pin_clk, pin_dc, (screen_height - 1) & 0xFF);
    bitbang_write_data(pin_data, pin_clk, pin_dc, ((screen_height - 1) >> 8));
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_BORDER_WAVEFORM_CONTROL);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x01);
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_DISPLAY_UPDATE_CONTROL_1);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0b00001000);
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_DISPLAY_UPDATE_CONTROL_2);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0xf8);
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "activate config");
    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_MASTER_ACTIVATION);
    gpio_set_level(pin_cs, true);

    timeout = 100;
    while (gpio_get_level(pin_busy)) {
        timeout--;
        if (timeout < 1) {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_TEMPERATURE_SENSOR_CONTROL);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x48);
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    nvs_handle_t nvs_handle;
    res = nvs_open("epaper", NVS_READWRITE, &nvs_handle);
    if (res != ESP_OK) {
        return res;
    }

    int8_t temperatures[] = {0, 4, 8, 12, 16, 18, 20, 24, 26, 28, 30};

    for (int temperature_index = 0; temperature_index < sizeof(temperatures); temperature_index++) {
        int8_t temperature = temperatures[temperature_index];

        gpio_set_level(pin_cs, false);
        bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_TEMPERATURE_SENSOR_WRITE);
        bitbang_write_data(pin_data, pin_clk, pin_dc, (uint8_t)temperature);
        bitbang_write_data(pin_data, pin_clk, pin_dc, 0x00);
        gpio_set_level(pin_cs, true);
        vTaskDelay(10 / portTICK_PERIOD_MS);

        gpio_set_level(pin_cs, false);
        bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_DISPLAY_UPDATE_CONTROL_2);
        bitbang_write_data(pin_data, pin_clk, pin_dc, 0xB1);
        gpio_set_level(pin_cs, true);
        vTaskDelay(10 / portTICK_PERIOD_MS);

        // ESP_LOGI(TAG, "activate temperature setting");
        gpio_set_level(pin_cs, false);
        bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_MASTER_ACTIVATION);
        gpio_set_level(pin_cs, true);

        timeout = 100;
        while (gpio_get_level(pin_busy)) {
            timeout--;
            if (timeout < 1) {
                nvs_close(nvs_handle);
                return ESP_ERR_TIMEOUT;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        // ESP_LOGI(TAG, "read lut data");
        gpio_set_level(pin_cs, false);
        bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_READ_LUT_REGISTER);
        gpio_set_level(pin_dc, true);

        for (int i = 0; i < sizeof(lut_buffer); i++) {
            lut_buffer[i] = bitbang_rx(pin_data, pin_clk);
        }
        gpio_set_level(pin_cs, true);

        char lutname[16];
        sprintf(lutname, "lut.%u", temperature);

        res = nvs_set_blob(nvs_handle, lutname, lut_buffer, HINK_LUT_SIZE);
        if (res != ESP_OK) {
            nvs_close(nvs_handle);
            return res;
        }

        printf("Epaper LUT for %d degrees:\n", temperature);
        for (int i = 0; i < sizeof(lut_buffer); i++) {
            printf("%02x ", lut_buffer[i]);
        }
        printf("\n");
    }

    res = nvs_set_u8(nvs_handle, "lut_populated", 1);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return res;
}

esp_err_t hink_reset_lut() {
    nvs_handle_t nvs_handle;

    esp_err_t res = nvs_open("epaper", NVS_READWRITE, &nvs_handle);
    if (res != ESP_OK) {
        return res;
    }

    res = nvs_set_u8(nvs_handle, "lut_populated", 0);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return res;
}

bool hink_get_lut_populated() {
    nvs_handle_t nvs_handle;
    esp_err_t    res = nvs_open("epaper", NVS_READWRITE, &nvs_handle);
    if (res != ESP_OK) {
        return 0;
    }
    uint8_t value = 0;
    res           = nvs_get_u8(nvs_handle, "lut_populated", &value);
    if (res != ESP_OK) {
        value = 0;
    }
    nvs_close(nvs_handle);
    return (value == 1);
}

esp_err_t hink_get_lut(uint8_t temperature, uint8_t* target_buffer) {
    nvs_handle_t nvs_handle;

    esp_err_t res = nvs_open("epaper", NVS_READWRITE, &nvs_handle);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open epaper NVS namespace");
        return res;
    }

    char lutname[16];
    sprintf(lutname, "lut.%u", temperature);

    size_t stored_length = 0;
    res                  = nvs_get_blob(nvs_handle, lutname, NULL, &stored_length);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read length of %u degree LUT from NVS", temperature);
        return res;
    }

    if (stored_length != HINK_LUT_SIZE) {
        ESP_LOGE(TAG, "Stored length for %u degree LUT is invalid", temperature);
        return ESP_FAIL;
    }

    res = nvs_get_blob(nvs_handle, lutname, target_buffer, &stored_length);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read %u degree LUT from NVS", temperature);
    }

    nvs_close(nvs_handle);

    return res;
}

esp_err_t hink_apply_lut(hink_t* epaper, epaper_lut_t lut_type) {
    esp_err_t res;

    res = hink_get_lut(20, lut_buffer);
    if (res != ESP_OK) {
        return res;
    }

    lut7_t* lut = (lut7_t*)lut_buffer;

    switch (lut_type) {
        case lut_8s:
            lut->groups[0].repeat = 0;
            lut->groups[1].repeat = 0;
            lut->groups[2].repeat = 0;
            lut->groups[3].repeat = 0;
            lut->groups[4].repeat = 0;
            lut->groups[5].repeat = 0;
            break;
        case lut_4s:  // uses groups 3,4 and 5 without repeat
            lut->groups[0].repeat = 0;
            lut->groups[1].repeat = 0;
            lut->groups[2].repeat = 0;
            lut->groups[3].repeat = 0;
            lut->groups[4].repeat = 0;
            lut->groups[5].repeat = 0;

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
            break;

        case lut_1s:  // use group 3 and 4 without repeat, takes ~1400ms
            lut->groups[0].repeat = 0;
            lut->groups[1].repeat = 0;
            lut->groups[2].repeat = 0;
            lut->groups[3].repeat = 0;
            lut->groups[4].repeat = 0;
            lut->groups[5].repeat = 0;

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

            lut->groups[4].tp[0] = 0;
            lut->groups[4].tp[1] = 0;
            lut->groups[4].tp[2] = 0;
            lut->groups[4].tp[3] = 0;

            lut->groups[5].tp[0] = 0;
            lut->groups[5].tp[1] = 0;
            lut->groups[5].tp[2] = 0;
            lut->groups[5].tp[3] = 0;
            break;

        case lut_white:  // NOT WORKING ATM - uses groups 3,4 and 5 without repeat
            lut->groups[0].repeat = 0;
            lut->groups[1].repeat = 0;
            lut->groups[2].repeat = 0;
            lut->groups[3].repeat = 0;
            lut->groups[4].repeat = 0;
            lut->groups[5].repeat = 0;

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

        case lut_black:  // use the second half of group 3, takes ~2s
            lut->groups[0].repeat = 0;
            lut->groups[1].repeat = 0;
            lut->groups[2].repeat = 0;
            lut->groups[3].repeat = 0x03;
            lut->groups[4].repeat = 0;
            lut->groups[5].repeat = 0;

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

            lut->groups[3].tp[0] = 0;
            lut->groups[3].tp[1] = 0;

            lut->groups[4].tp[0] = 0;
            lut->groups[4].tp[1] = 0;
            lut->groups[4].tp[2] = 0;
            lut->groups[4].tp[3] = 0;

            lut->groups[5].tp[0] = 0;
            lut->groups[5].tp[1] = 0;
            lut->groups[5].tp[2] = 0;
            lut->groups[5].tp[3] = 0;
            break;

        case lut_red:  // use the second half of group 3, and full group 4 and 5, takes ~13s
            lut->groups[0].repeat = 0;
            lut->groups[1].repeat = 0;
            lut->groups[2].repeat = 0;
            lut->groups[3].repeat = 0x03;
            lut->groups[4].repeat = 0x04;
            lut->groups[5].repeat = 0x03;

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

            lut->groups[3].tp[0] = 0;
            lut->groups[3].tp[1] = 0;
            break;

        case lut_900ms:  // Fastest found, 900ms, but doesn't refresh the whole screen
            lut->groups[0].repeat = 0;
            lut->groups[1].repeat = 0;
            lut->groups[2].repeat = 0;
            lut->groups[3].repeat = 0;
            lut->groups[4].repeat = 0;
            lut->groups[5].repeat = 0;

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

            lut->groups[4].tp[0] = 0;
            lut->groups[4].tp[1] = 0;
            lut->groups[4].tp[2] = 0;
            lut->groups[4].tp[3] = 0;

            lut->groups[5].tp[0] = 0;
            lut->groups[5].tp[1] = 0;
            lut->groups[5].tp[2] = 0;
            lut->groups[5].tp[3] = 0;
            break;
        case lut_full: break;
        default:
            {
                ESP_LOGW(TAG, "Unknown LUT type selected, using default");
                break;
            }
    }
    return hink_set_lut(epaper, (uint8_t*)lut);
}
