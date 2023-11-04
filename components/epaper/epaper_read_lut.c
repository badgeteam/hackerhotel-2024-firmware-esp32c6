
/**
 * Copyright (c) 2023 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#include "hextools.h"
#include "include/epaper.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <sdkconfig.h>
#include <soc/gpio_reg.h>
#include <soc/gpio_sig_map.h>
#include <soc/gpio_struct.h>
#include <soc/spi_reg.h>
#include <string.h>

static char const *TAG = "epaper read lut";

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

void hink_read_lut(int pin_data, int pin_clk, int pin_cs, int pin_dc, int pin_reset, int pin_busy) {
    // Initialize pins
    gpio_reset_pin(pin_reset);
    gpio_set_direction(pin_reset, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_reset, false);

    gpio_reset_pin(pin_cs);
    gpio_set_direction(pin_cs, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_cs, true);

    gpio_reset_pin(pin_dc);
    gpio_set_direction(pin_dc, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_dc, false);

    gpio_reset_pin(pin_clk);
    gpio_set_direction(pin_clk, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_clk, false);

    gpio_reset_pin(pin_data);
    gpio_set_direction(pin_data, GPIO_MODE_INPUT);

    // Reset display
    ESP_LOGI(TAG, "reset display");
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(pin_reset, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_SW_RESET);
    gpio_set_level(pin_cs, true);
    while (gpio_get_level(pin_busy)) {
        ESP_LOGI(TAG, "waiting for display");
        vTaskDelay(100 / portTICK_PERIOD_MS);
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
    while (gpio_get_level(pin_busy)) {
        ESP_LOGI(TAG, "waiting for display");
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }



    ESP_LOGI(TAG, "set temperature");
    gpio_set_level(pin_cs, false);
    bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_TEMPERATURE_SENSOR_CONTROL);
    bitbang_write_data(pin_data, pin_clk, pin_dc, 0x48);
    gpio_set_level(pin_cs, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    int8_t temperatures[] = {-20, -10, 0,  2,  4,  6,  8,  10, 12, 14, 16, 18,
                             20,  22,  24, 26, 28, 30, 40, 50, 60, 70, 80, 90};

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
        while (gpio_get_level(pin_busy)) {
            // ESP_LOGI(TAG, "waiting for display");
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        // ESP_LOGI(TAG, "read lut data");
        gpio_set_level(pin_cs, false);
        bitbang_write_cmd(pin_data, pin_clk, pin_dc, HINK_CMD_READ_LUT_REGISTER);
        gpio_set_level(pin_dc, true);

        uint8_t lut[128] = {0};

        for (int i = 0; i < sizeof(lut); i++) {
            lut[i] = bitbang_rx(pin_data, pin_clk);
        }
        gpio_set_level(pin_cs, true);

        printf("%05d: ", temperature);
        for (int i = 0; i < sizeof(lut); i++) {
            printf("%02x ", lut[i]);
        }
        printf("\n");
    }
}
