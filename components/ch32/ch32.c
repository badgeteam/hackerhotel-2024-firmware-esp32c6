/**
 * Copyright (c) 2024 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#include "ch32.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include <inttypes.h>
#include <stdio.h>
#include "soc/gpio_struct.h"
#include "freertos/portmacro.h"
#include "riscv/rv_utils.h"
#include "driver/dedic_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static char const *TAG = "CH32";

// Initialization functions

void ch32_enable_slave_output() {
    uint32_t value = CH32_CFGR_KEY | CH32_CFGR_OUTEN; // Enable output from slave
    ch32_sdi_write(CH32_REG_DEBUG_SHDWCFGR, value); // First write the shadow register
    ch32_sdi_write(CH32_REG_DEBUG_CFGR, value); // Then the configuration register
    ch32_sdi_write(CH32_REG_DEBUG_CFGR, value); // Then the configuration register (again)
    ch32_sdi_read(CH32_REG_DEBUG_CPBR, &value); // Dummy read, for some reason the first read fails
}

bool ch32_check_link() {
    uint32_t value;
    ch32_sdi_read(CH32_REG_DEBUG_CPBR, &value);
    ESP_LOGW(TAG, "Link check raw result: %08" PRIx32, value);
    if (value == 0xFFFFFFFF || value == 0x00000000) {
        ESP_LOGE(TAG, "Link check failed: no chip");
        return false;
    }
    if (!((value >> 10) & 1)) {
        ESP_LOGE(TAG, "Link check failed: output not enabled");
        return false;
    }
    ESP_LOGI(TAG, "Link check succesful, version %" PRIu32, value >> 16);
    return true;
}

// Functions based on operation examples in debug spec document

bool ch32_halt_microprocessor() {
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request

    // Get the debug module status information, check rdata[9:8], if the value is 0b11,
    // it means the processor enters the halt state normally. Otherwise try again.
    uint8_t timeout = 5;
    while (1) {
        uint32_t value;
        ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
        if (((value >> 8) & 0b11) == 0b11) { // Check that processor has entered halted state
            break;
        }
        if (timeout == 0) {
            ESP_LOGE(TAG, "Failed to halt microprocessor");
            return false;
        }
        timeout--;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request
    ESP_LOGI(TAG, "Microprocessor halted");
    return true;
}

bool ch32_resume_microprocessor() {
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x40000001); // Initiate a resume request

    // Get the debug module status information, check rdata[17:16],
    // if the value is 0b11, it means the processor has recovered.
    uint32_t value;
    ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
    if (!(((value >> 16) & 0b11) == 0b11)) {
        ESP_LOGE(TAG, "Failed to resume microprocessor");
        return false;
    }
    return true;
}

bool ch32_reset_microprocessor_and_run() {
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000003); // Initiate a core reset request

    uint8_t timeout = 5;
    while (1) {
        uint32_t value;
        ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
        if (((value >> 18) & 0b11) == 0b11) { // Check that processor has been reset
            break;
        }
        if (timeout == 0) {
            ESP_LOGE(TAG, "Failed to reset microprocessor");
            return false;
        }
        timeout--;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the core reset request
    vTaskDelay(pdMS_TO_TICKS(10));
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x10000001); // Clear the core reset status signal
    vTaskDelay(pdMS_TO_TICKS(10));
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the core reset status signal clear request
    vTaskDelay(pdMS_TO_TICKS(10));

    timeout = 100;
    while (1) {
        uint32_t value;
        ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
        if (((value >> 18) & 0b11) == 0b00) { // Check that processor reset status has been cleared
            break;
        }
        if (timeout == 0) {
            ESP_LOGE(TAG, "Failed to resume microprocessor after reset, DMSTATUS=%08" PRIx32, value);
            return false;
        }
        timeout--;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return true;
}

bool ch32_reset_microprocessor_and_halt() {
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000003); // Initiate a core reset request

    uint8_t timeout = 100;
    while (1) {
        uint32_t value;
        ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
        if (((value >> 18) & 0b11) == 0b11) { // Check that processor has been reset
            break;
        }
        if (timeout == 0) {
            ESP_LOGE(TAG, "Failed to reset microprocessor");
            return false;
        }
        timeout--;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Clear the core reset request
    vTaskDelay(pdMS_TO_TICKS(10));
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x90000001); // Clear the core reset status signal
    vTaskDelay(pdMS_TO_TICKS(10));
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Clear the core reset status signal clear request
    vTaskDelay(pdMS_TO_TICKS(10));


    timeout = 5;
    while (1) {
        uint32_t value;
        ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
        if (((value >> 18) & 0b11) == 0b00) { // Check that processor reset status has been cleared
            break;
        }
        if (timeout == 0) {
            ESP_LOGE(TAG, "Failed to resume microprocessor after reset, DMSTATUS=%08" PRIx32, value);
            return false;
        }
        timeout--;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request

    return true;
}

bool ch32_reset_debug_module() {
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000003); // Write command
    uint32_t value;
    ch32_sdi_read(CH32_REG_DEBUG_DMCONTROL, &value);
    if (value != 0x00000003) {
        ESP_LOGE(TAG, "Failed to reset debug module");
        return false;
    }
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000002); // Write debug module reset command
    ch32_sdi_read(CH32_REG_DEBUG_DMCONTROL, &value);
    if (((value >> 1) & 1) != 0) { // Check whether rdata[1] is 0b0, if it is the reset is succesful
        ESP_LOGE(TAG, "Failed to reset debug module");
        return false;
    }
    return true;
}
