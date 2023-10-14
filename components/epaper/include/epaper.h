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

#include <driver/spi_master.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#define HINK_LUT_SIZE 70

typedef struct HINK {
    // Configuration
    int                 spi_bus;
    int                 pin_cs;
    int                 pin_dcx;
    int                 pin_reset;
    int                 pin_busy;
    uint32_t            spi_speed;
    uint32_t            spi_max_transfer_size;
    // Internal state
    spi_device_handle_t spi_device;
    bool                dc_level;
    SemaphoreHandle_t   mutex;
    uint8_t const      *lut;
} HINK;

esp_err_t hink_init(HINK *device);
esp_err_t hink_deinit(HINK *device);
// Write data to the display.
esp_err_t hink_write(HINK *device, uint8_t const *data);
// Set the active LUT.
// Does not create a copy of the LUT.
esp_err_t hink_set_lut(HINK *device, uint8_t const lut[HINK_LUT_SIZE]);

#ifdef __cplusplus
}
#endif //__cplusplus
