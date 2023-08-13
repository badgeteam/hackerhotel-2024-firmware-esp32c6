/**
 * Copyright (c) 2023 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include <stdbool.h>
#include <stdint.h>
#include <esp_err.h>
#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

typedef struct HINK {
    // Configuration
    int spi_bus;
    int pin_cs;
    int pin_dcx;
    int pin_reset;
    int pin_busy;
    uint32_t spi_speed;
    uint32_t spi_max_transfer_size;
    // Internal state
    spi_device_handle_t spi_device;
    bool dc_level;
    SemaphoreHandle_t mutex;
} HINK;

esp_err_t hink_init(HINK* device);
esp_err_t hink_deinit(HINK* device);
esp_err_t hink_write(HINK* device, const uint8_t *data);

#ifdef __cplusplus
}
#endif //__cplusplus
