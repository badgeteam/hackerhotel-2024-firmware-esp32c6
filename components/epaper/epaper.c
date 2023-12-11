/**
 * Copyright (c) 2023 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#include "include/epaper.h"

#include "hextools.h"

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

static char const *TAG = "epaper";

static void IRAM_ATTR hink_spi_pre_transfer_callback(spi_transaction_t *t) {
    hink_t *device = ((hink_t *)t->user);
    gpio_set_level(device->pin_dcx, device->dc_level);
}

static esp_err_t hink_send(hink_t *device, uint8_t const *data, int const len, bool const dc_level) {
    if (device->spi_device == NULL) {
        return ESP_FAIL;
    }

    if (len == 0) {
        return ESP_OK;
    }

    device->dc_level              = dc_level;
    spi_transaction_t transaction = {
        .length    = len * 8,  // transaction length is in bits
        .rxlength  = 0,
        .tx_buffer = data,
        .rx_buffer = NULL,
        .user      = (void *)device,
    };

    return spi_device_transmit(device->spi_device, &transaction);
}

static esp_err_t hink_write_init_data(hink_t *device, uint8_t const *data) {
    esp_err_t res;
    uint8_t   cmd, len;

    if (device->spi_device == NULL) {
        return ESP_FAIL;
    }

    while (true) {
        cmd = *data++;
        if (!cmd) {
            return ESP_OK;  // END
        }
        len = *data++;
        res = hink_send(device, &cmd, 1, false);
        if (res != ESP_OK) {
            break;
        }
        res = hink_send(device, data, len, true);
        if (res != ESP_OK) {
            break;
        }
        data += len;
    }
    return res;
}

static esp_err_t hink_send_command(hink_t *device, uint8_t const cmd) {
    return hink_send(device, &cmd, 1, false);
}

static esp_err_t hink_send_data(hink_t *device, uint8_t const *data, uint16_t const length) {
    return hink_send(device, data, length, true);
}

static esp_err_t hink_send_u32(hink_t *device, uint32_t const data) {
    uint8_t buffer[4];
    buffer[0] = (data >> 24) & 0xFF;
    buffer[1] = (data >> 16) & 0xFF;
    buffer[2] = (data >> 8) & 0xFF;
    buffer[3] = data & 0xFF;
    return hink_send(device, buffer, 4, true);
}

static esp_err_t hink_send_u16(hink_t *device, uint32_t const data) {
    uint8_t buffer[2];
    buffer[0] = (data >> 8) & 0xFF;
    buffer[1] = data & 0xFF;
    return hink_send(device, buffer, 2, true);
}

static esp_err_t hink_send_u8(hink_t *device, uint8_t const data) {
    return hink_send(device, &data, 1, true);
}

static esp_err_t hink_reset(hink_t *device) {
    if (device->pin_reset >= 0) {
        ESP_LOGI(TAG, "epaper display reset");
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

bool hink_busy(hink_t *device) {
    return gpio_get_level(device->pin_busy);
}

esp_err_t hink_wait(hink_t *device) {
    while (hink_busy(device)) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    return ESP_OK;
}

static esp_err_t hink_select(hink_t *device, bool const state) {
    if (device->spi_device != NULL) {
        return ESP_FAIL;
    }
    return gpio_set_level(device->pin_cs, !state);
}

static esp_err_t hink_write_lut(hink_t *device) {
    esp_err_t res;
    if (device->lut == NULL) {
        return ESP_FAIL;
    }
    if (device->spi_device == NULL) {
        return ESP_FAIL;
    }
    res = hink_send_command(device, HINK_CMD_WRITE_LUT_REGISTER);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_send_data(device, device->lut, 70);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_set_gate_driving_voltage(device, device->lut[70]);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_set_source_driving_voltage(device, device->lut[71], device->lut[72], device->lut[73]);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_set_dummy_line_period(device, device->lut[74]);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_set_gate_line_width(device, device->lut[75]);
    return res;
}

esp_err_t hink_set_gate_driving_voltage(hink_t *device, uint8_t value) {
    esp_err_t res;
    if (device->spi_device == NULL) {
        return ESP_FAIL;
    }
    res = hink_send_command(device, HINK_CMD_GATE_DRIVING_VOLTAGE);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_send_u8(device, value & 0x1F);
    if (res == ESP_OK) {
        ESP_LOGI(TAG, "Gate driving voltage set to 0x%02x", value & 0x1F);
    }
    return res;
}

esp_err_t hink_set_source_driving_voltage(hink_t *device, uint8_t vsh1, uint8_t vsh2, uint8_t vsl) {
    esp_err_t res;
    if (device->spi_device == NULL) {
        return ESP_FAIL;
    }
    res = hink_send_command(device, HINK_CMD_SOURCE_DRIVING_VOLTAGE_CONTROL);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_send_u8(device, vsh1);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_send_u8(device, vsh2);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_send_u8(device, vsl);
    if (res == ESP_OK) {
        ESP_LOGI(TAG, "Source driving voltage set to 0x%02x 0x%02x 0x%02x", vsh1, vsh2, vsl);
    }
    return res;
}

esp_err_t hink_set_dummy_line_period(hink_t *device, uint8_t period) {
    esp_err_t res;
    if (device->spi_device == NULL) {
        return ESP_FAIL;
    }
    res = hink_send_command(device, HINK_CMD_SET_DUMMY_LINE_PERIOD);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_send_u8(device, period & 0x7F);
    if (res == ESP_OK) {
        ESP_LOGI(TAG, "Dummy line period set to 0x%02x", period & 0x7F);
    }
    return res;
}

esp_err_t hink_set_gate_line_width(hink_t *device, uint8_t width) {
    esp_err_t res;
    if (device->spi_device == NULL) {
        return ESP_FAIL;
    }
    res = hink_send_command(device, HINK_CMD_SET_GATE_LINE_WIDTH);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_send_u8(device, width & 0x0F);
    if (res == ESP_OK) {
        ESP_LOGI(TAG, "Gate line width set to 0x%02x", width & 0x0F);
    }
    return res;
}

esp_err_t hink_init(hink_t *device) {
    esp_err_t res;

    if (device->pin_dcx < 0)
        return ESP_FAIL;
    if (device->pin_cs < 0)
        return ESP_FAIL;

    device->lut = NULL;

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
            .mode             = 0,  // SPI mode 0
            .duty_cycle_pos   = 128,
            .cs_ena_pretrans  = 0,
            .cs_ena_posttrans = 0,
            .clock_speed_hz   = device->spi_speed,
            .input_delay_ns   = 0,
            .spics_io_num     = device->pin_cs,
            .flags            = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE,
            .queue_size       = 1,
            .pre_cb           = hink_spi_pre_transfer_callback,  // Handles D/C line
            .post_cb          = NULL};
        res = spi_bus_add_device(device->spi_bus, &devcfg, &device->spi_device);
        if (res != ESP_OK)
            return res;
    }

    hink_reset(device);

    return ESP_OK;
}

esp_err_t hink_deinit(hink_t *device) {
    esp_err_t res;
    if (device->spi_device != NULL) {
        res                = spi_bus_remove_device(device->spi_device);
        device->spi_device = NULL;
        if (res != ESP_OK) {
            return res;
        }
    }
    res = gpio_set_direction(device->pin_dcx, GPIO_MODE_INPUT);
    if (res != ESP_OK)
        return res;
    res = gpio_set_direction(device->pin_cs, GPIO_MODE_INPUT);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_reset(device);
    if (res != ESP_OK) {
        return res;
    }
    return res;
}

esp_err_t hink_write(hink_t *device, uint8_t const *buffer) {
    if (device->spi_device == NULL) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Reset");
    hink_reset(device);
    hink_send_command(device, HINK_CMD_SW_RESET);
    hink_wait(device);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Configure");
    hink_send_command(device, HINK_CMD_SET_ANALOG_BLOCK_CONTROL);
    hink_send_u8(device, 0x54);

    hink_send_command(device, HINK_CMD_SET_DIGITAL_BLOCK_CONTROL);
    hink_send_u8(device, 0x3B);

    hink_send_command(device, HINK_CMD_UNKNOWN_1);  // ACVCOM setting
    hink_send_u8(device, 0x04);
    hink_send_u8(device, 0x63);

    hink_send_command(device, HINK_CMD_BOOSTER_SOFT_START_CONTROL);
    hink_send_u8(device, 0x8F);
    hink_send_u8(device, 0x8F);
    hink_send_u8(device, 0x8F);
    hink_send_u8(device, 0x3F);

    hink_send_command(device, HINK_CMD_DRIVER_OUTPUT_CONTROL);
    hink_send_u8(device, ((device->screen_height - 1) & 0xFF));
    hink_send_u8(device, ((device->screen_height - 1) >> 8) && 0x01);
    hink_send_u8(device, 0x00);

    hink_send_command(device, HINK_CMD_DATA_ENTRY_MODE_SETTING);
    hink_send_u8(device, 0x01);

    hink_send_command(device, HINK_CMD_SET_RAM_X_ADDRESS_LIMITS);
    hink_send_u8(device, 0x00);
    hink_send_u8(device, (device->screen_width / 8) - 1);

    hink_send_command(device, HINK_CMD_SET_RAM_Y_ADDRESS_LIMITS);
    hink_send_u8(device, (device->screen_height - 1) & 0xFF);
    hink_send_u8(device, ((device->screen_height - 1) >> 8));
    hink_send_u8(device, 0x00);
    hink_send_u8(device, 0x00);

    hink_send_command(device, HINK_CMD_BORDER_WAVEFORM_CONTROL);
    hink_send_u8(device, 0x01);  // 0 = black,1 = white,2 = Red

    hink_send_command(device, HINK_CMD_TEMPERATURE_SENSOR_CONTROL);
    hink_send_u8(device, 0x80);  // 0x48 = External,0x80 = Internal

    hink_send_command(device, HINK_CMD_DISPLAY_UPDATE_CONTROL_1);
    hink_send_u8(device, 0b00001000);  // inverse or ignore ram content

    hink_send_command(device, HINK_CMD_DISPLAY_UPDATE_CONTROL_2);
    hink_send_u8(
        device,
        HINK_DISPLAY_UPDATE_CONTROL_2_CLOCK_ON | HINK_DISPLAY_UPDATE_CONTROL_2_LATCH_TEMPERATURE_VAL |
            HINK_DISPLAY_UPDATE_CONTROL_2_LOAD_LUT | HINK_DISPLAY_UPDATE_CONTROL_2_CLOCK_OFF
    );  // 0xB1

    hink_send_command(device, HINK_CMD_MASTER_ACTIVATION);
    hink_wait(device);

    // Red framebuffer

    hink_send_command(device, HINK_CMD_SET_RAM_X_ADDRESS_COUNTER);
    hink_send_u8(device, 0x00);

    hink_send_command(device, HINK_CMD_SET_RAM_Y_ADDRESS_COUNTER);
    hink_send_u8(device, (device->screen_height - 1) & 0xFF);
    hink_send_u8(device, ((device->screen_height - 1) >> 8));

    hink_send_command(device, HINK_CMD_WRITE_RAM_RED);

    for (int y = 0; y < device->screen_height; y++) {
        for (int x = (device->screen_width / 8) - 1; x >= 0; x--) {
            uint32_t position   = y * ((device->screen_width / 8) * 2) + x * 2;
            uint16_t pixels     = buffer[position] | (buffer[position + 1] << 8);
            uint8_t  out        = 0;
            pixels            >>= 1;
            for (int bit = 0; bit < 8; bit++) {
                out      = (out >> 1) | ((pixels & 1) << 7);
                pixels >>= 2;
            }
            hink_send_u8(device, out);
        }
    }

    // Black framebuffer

    hink_send_command(device, HINK_CMD_SET_RAM_X_ADDRESS_COUNTER);
    hink_send_u8(device, 0x00);

    hink_send_command(device, HINK_CMD_SET_RAM_Y_ADDRESS_COUNTER);
    hink_send_u8(device, (device->screen_height - 1) & 0xFF);
    hink_send_u8(device, ((device->screen_height - 1) >> 8));

    hink_send_command(device, HINK_CMD_WRITE_RAM_BLACK);

    for (int y = 0; y < device->screen_height; y++) {
        for (int x = (device->screen_width / 8) - 1; x >= 0; x--) {
            uint32_t position = y * ((device->screen_width / 8) * 2) + x * 2;
            uint16_t pixels   = buffer[position] | (buffer[position + 1] << 8);
            uint8_t  out      = 0;
            for (int bit = 0; bit < 8; bit++) {
                out      = (out >> 1) | ((pixels & 1) << 7);
                pixels >>= 2;
            }
            hink_send_u8(device, out);
        }
    }

    if (device->lut) {
        hink_write_lut(device);
    }

    hink_send_command(device, HINK_CMD_DISPLAY_UPDATE_CONTROL_2);
    hink_send_u8(
        device,
        HINK_DISPLAY_UPDATE_CONTROL_2_CLOCK_ON | HINK_DISPLAY_UPDATE_CONTROL_2_ANALOG_ON |
            HINK_DISPLAY_UPDATE_CONTROL_2_USE_MODE_1 | HINK_DISPLAY_UPDATE_CONTROL_2_USE_MODE_2 |
            HINK_DISPLAY_UPDATE_CONTROL_2_ANALOG_OFF | HINK_DISPLAY_UPDATE_CONTROL_2_CLOCK_OFF
    );

    hink_send_command(device, HINK_CMD_MASTER_ACTIVATION);
    return ESP_OK;
}

esp_err_t hink_sleep(hink_t *device) {
    esp_err_t res;
    ESP_LOGI(TAG, "Set display to deep sleep mode");
    res = hink_send_command(device, HINK_CMD_DEEP_SLEEP_MODE);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_send_u8(device, 1);  // Enter deep sleep mode 1
    return res;
}

// Set the active LUT.
// Does not create a copy of the LUT.
esp_err_t hink_set_lut(hink_t *device, uint8_t const *lut) {
    device->lut = lut;
    return ESP_OK;
}
