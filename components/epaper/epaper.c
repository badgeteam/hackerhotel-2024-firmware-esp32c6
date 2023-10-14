/**
 * Copyright (c) 2023 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

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

static char const *TAG = "HINK";

static uint8_t const default_lut[HINK_LUT_SIZE] = {
    0x80, 0x66, 0x96, 0x51, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x10, 0x66, 0x96, 0x88, 0x20, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x8A, 0x66, 0x96, 0x51, 0x0B, 0x2F, 0x00, 0x00, 0x00, 0x00, 0x8A, 0x66, 0x96, 0x51, 0x0B, 0x2F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x55, 0x28, 0x25,
    0x00, 0x02, 0x03, 0x01, 0x02, 0x0C, 0x12, 0x01, 0x12, 0x01, 0x03, 0x05, 0x05, 0x02, 0x05, 0x02,
};



static void IRAM_ATTR hink_spi_pre_transfer_callback(spi_transaction_t *t) {
    HINK *device = ((HINK *)t->user);
    gpio_set_level(device->pin_dcx, device->dc_level);
}

static esp_err_t hink_send(HINK *device, uint8_t const *data, int const len, bool const dc_level) {
    if (device->spi_device == NULL)
        return ESP_FAIL;
    if (len == 0)
        return ESP_OK;

    device->dc_level              = dc_level;
    spi_transaction_t transaction = {
        .length    = len * 8, // transaction length is in bits
        .rxlength  = 0,
        .tx_buffer = data,
        .rx_buffer = NULL,
        .user      = (void *)device,
    };

    return spi_device_transmit(device->spi_device, &transaction);
}

static esp_err_t hink_write_init_data(HINK *device, uint8_t const *data) {
    if (device->spi_device == NULL)
        return ESP_FAIL;
    esp_err_t res = ESP_OK;
    uint8_t   cmd, len;
    while (true) {
        cmd = *data++;
        if (!cmd)
            return ESP_OK; // END
        len = *data++;
        res = hink_send(device, &cmd, 1, false);
        if (res != ESP_OK)
            break;
        res = hink_send(device, data, len, true);
        if (res != ESP_OK)
            break;
        data += len;
    }
    return res;
}

static esp_err_t hink_send_command(HINK *device, const uint8_t cmd) {
    return hink_send(device, &cmd, 1, false);
}

static esp_err_t hink_send_data(HINK *device, uint8_t const *data, const uint16_t length) {
    return hink_send(device, data, length, true);
}

static esp_err_t hink_send_u32(HINK *device, const uint32_t data) {
    uint8_t buffer[4];
    buffer[0] = (data >> 24) & 0xFF;
    buffer[1] = (data >> 16) & 0xFF;
    buffer[2] = (data >> 8) & 0xFF;
    buffer[3] = data & 0xFF;
    return hink_send(device, buffer, 4, true);
}

static esp_err_t hink_send_u16(HINK *device, const uint32_t data) {
    uint8_t buffer[2];
    buffer[0] = (data >> 8) & 0xFF;
    buffer[1] = data & 0xFF;
    return hink_send(device, buffer, 2, true);
}

static esp_err_t hink_send_u8(HINK *device, const uint8_t data) {
    return hink_send(device, &data, 1, true);
}

static esp_err_t hink_reset(HINK *device) {
    if (device->pin_reset >= 0) {
        ESP_LOGI(TAG, "reset");
        esp_err_t res = gpio_set_level(device->pin_reset, false);
        if (res != ESP_OK)
            return res;
        vTaskDelay(50 / portTICK_PERIOD_MS);
        res = gpio_set_level(device->pin_reset, true);
        if (res != ESP_OK)
            return res;
        vTaskDelay(50 / portTICK_PERIOD_MS);
    } else {
        ESP_LOGI(TAG, "(no reset pin available)");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    return ESP_OK;
}

static esp_err_t hink_wait(HINK *device) {
    while (gpio_get_level(device->pin_busy)) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    return ESP_OK;
}

static esp_err_t hink_select(HINK *device, bool const state) {
    if (device->spi_device != NULL)
        return ESP_FAIL;
    return gpio_set_level(device->pin_cs, !state);
}

static esp_err_t hink_write_lut(HINK *device, uint8_t const lut[HINK_LUT_SIZE]) {
    if (device->spi_device == NULL) {
        return ESP_FAIL;
    }

    hink_send_command(device, 0x32); // Write LUT register
    hink_send_data(device, lut, HINK_LUT_SIZE);

    return ESP_OK;
}



esp_err_t hink_init(HINK *device) {
    esp_err_t res;

    if (device->pin_dcx < 0)
        return ESP_FAIL;
    if (device->pin_cs < 0)
        return ESP_FAIL;

    device->lut = default_lut;

    res = gpio_reset_pin(device->pin_dcx);
    if (res != ESP_OK)
        return res;
    res = gpio_reset_pin(device->pin_cs);
    if (res != ESP_OK)
        return res;

    if (device->pin_reset >= 0) {
        res = gpio_reset_pin(device->pin_reset);
        if (res != ESP_OK)
            return res;
        res = gpio_set_direction(device->pin_reset, GPIO_MODE_OUTPUT);
        if (res != ESP_OK)
            return res;
    }

    // Initialize data/clock select GPIO pin
    res = gpio_set_direction(device->pin_dcx, GPIO_MODE_OUTPUT);
    if (res != ESP_OK)
        return res;

    res = gpio_set_direction(device->pin_busy, GPIO_MODE_INPUT);
    if (res != ESP_OK)
        return res;

    if (device->spi_device == NULL) {
        spi_device_interface_config_t devcfg = {
            .command_bits     = 0,
            .address_bits     = 0,
            .dummy_bits       = 0,
            .mode             = 0, // SPI mode 0
            .duty_cycle_pos   = 128,
            .cs_ena_pretrans  = 0,
            .cs_ena_posttrans = 0,
            .clock_speed_hz   = device->spi_speed,
            .input_delay_ns   = 0,
            .spics_io_num     = device->pin_cs,
            .flags            = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE,
            .queue_size       = 1,
            .pre_cb           = hink_spi_pre_transfer_callback, // Handles D/C line
            .post_cb          = NULL};
        res = spi_bus_add_device(device->spi_bus, &devcfg, &device->spi_device);
        if (res != ESP_OK)
            return res;
    }

    hink_reset(device);

    return ESP_OK;
}

esp_err_t hink_deinit(HINK *device) {
    esp_err_t res;
    if (device->spi_device != NULL) {
        res                = spi_bus_remove_device(device->spi_device);
        device->spi_device = NULL;
        if (res != ESP_OK)
            return res;
    }
    res = gpio_set_direction(device->pin_dcx, GPIO_MODE_INPUT);
    if (res != ESP_OK)
        return res;
    res = gpio_set_direction(device->pin_cs, GPIO_MODE_INPUT);
    if (res != ESP_OK)
        return res;
    res = hink_reset(device);
    if (res != ESP_OK)
        return res;
    return res;
}

esp_err_t hink_write(HINK *device, uint8_t const *buffer) {
    if (device->spi_device == NULL) {
        return ESP_FAIL;
    }

    uint16_t eink_x = 18;
    uint16_t eink_y = 151;

    ESP_LOGI(TAG, "Reset");
    hink_reset(device);
    hink_send_command(device, 0x12); // Software reset
    hink_wait(device);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Configure");
    hink_send_command(device, 0x74); // Set Analog Block Control
    hink_send_u8(device, 0x54);

    hink_send_command(device, 0x7E); // Set Digital Block Control
    hink_send_u8(device, 0x3B);

    hink_send_command(device, 0x2B); // ACVCOM setting
    hink_send_u8(device, 0x04);
    hink_send_u8(device, 0x63);

    hink_send_command(device, 0x0C); // Softstart Control
    hink_send_u8(device, 0x8F);
    hink_send_u8(device, 0x8F);
    hink_send_u8(device, 0x8F);
    hink_send_u8(device, 0x3F);

    hink_send_command(device, 0x01); // Driver Output control
    hink_send_u8(device, eink_y);
    hink_send_u8(device, (eink_y >> 8));
    hink_send_u8(device, 0x00);

    hink_send_command(device, 0x11); // Data Entry mode setting
    hink_send_u8(device, 0x01);

    hink_send_command(device, 0x44); // Set RAM X - address Start/End position
    hink_send_u8(device, 0x00);
    hink_send_u8(device, eink_x);

    hink_send_command(device, 0x45); // Set RAM Y - address Start/End position
    hink_send_u8(device, eink_y);
    hink_send_u8(device, (eink_y >> 8));
    hink_send_u8(device, 0x00);
    hink_send_u8(device, 0x00);

    hink_send_command(device, 0x3C); // Border Waveform Control
    hink_send_u8(device, 0x01);      // 0 = black,1 = white,2 = Red

    hink_send_command(device, 0x18); // Temperature sensor control
    hink_send_u8(device, 0x80);      // 0x48 = External,0x80 = Internal

    hink_send_command(device, 0x21);  // Display Update Control 1
    hink_send_u8(device, 0b00001000); // inverse or ignore ram content

    hink_send_command(device, 0x22); // Display Update Control 2
    hink_send_u8(device, 0xB1);

    hink_send_command(device, 0x20); // Master Activation
    hink_wait(device);

    ESP_LOGI(TAG, "Writing RED buffer");

    hink_send_command(device, 0x4E); // Set RAM X address counter
    hink_send_u8(device, 0x00);

    hink_send_command(device, 0x4F); // Set RAM Y address counter
    hink_send_u8(device, eink_y);
    hink_send_u8(device, (eink_y >> 8));

    hink_send_command(device, 0x26); // Write RAM RED

    for (int y = 0; y < 152; y++) {
        for (int x = 18; x >= 0; x--) {
            uint16_t pixels = buffer[y * 38 + x * 2] | (buffer[y * 38 + x * 2 + 1] << 8);
            uint8_t  out    = 0;
            for (int bit = 0; bit < 8; bit++) {
                out      = (out >> 1) | ((pixels & 1) << 7);
                pixels >>= 2;
            }
            hink_send_u8(device, out);
        }
    }

    ESP_LOGI(TAG, "Writing BLACK buffer");

    hink_send_command(device, 0x4E); // Set RAM X address counter
    hink_send_u8(device, 0x00);

    hink_send_command(device, 0x4F); // Set RAM Y address counter
    hink_send_u8(device, eink_y);
    hink_send_u8(device, (eink_y >> 8));

    hink_send_command(device, 0x24); // WRITE RAM BLACK

    for (int y = 0; y < 152; y++) {
        for (int x = 18; x >= 0; x--) {
            uint16_t pixels   = buffer[y * 38 + x * 2] | (buffer[y * 38 + x * 2 + 1] << 8);
            uint8_t  out      = 0;
            pixels          >>= 1;
            for (int bit = 0; bit < 8; bit++) {
                out      = (out >> 1) | ((pixels & 1) << 7);
                pixels >>= 2;
            }
            hink_send_u8(device, out);
        }
    }

    ESP_LOGI(TAG, "Setting LUT");

    hink_write_lut(device, device->lut);

    ESP_LOGI(TAG, "Update");

    hink_send_command(device, 0x22); // Display Update Control 2
    hink_send_u8(device, 0xC7);

    hink_send_command(device, 0x20); // Master Activation
    hink_wait(device);

    ESP_LOGI(TAG, "Sleep");

    // hink_send_command(device, 0x10); // Deep Sleep mode
    // hink_send_u8(device, 1);

    ESP_LOGI(TAG, "--- DONE ---");

    return ESP_OK;
}

// Set the active LUT.
// Does not create a copy of the LUT.
esp_err_t hink_set_lut(HINK *device, uint8_t const lut[HINK_LUT_SIZE]) {
    device->lut = lut;
    return ESP_OK;
}
