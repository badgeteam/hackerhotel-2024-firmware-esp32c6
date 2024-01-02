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

#define CH32T 4

dedic_gpio_bundle_handle_t ch32_dedic_gpio_handle = NULL;

__attribute__((always_inline))
static inline void ch32_delay(int delay) {
	asm volatile(
"1:	addi %[delay], %[delay], -1\n"
"	bgt %[delay], zero, 1b\n" : [delay]"+r"(delay));
}


extern gpio_dev_t GPIO;

__attribute__((always_inline))
static inline void ch32_tx1() {
    dedic_gpio_bundle_write(ch32_dedic_gpio_handle, 0x01, 0x00);
    ch32_delay(2 * CH32T);
    dedic_gpio_bundle_write(ch32_dedic_gpio_handle, 0x01, 0x01);
    ch32_delay(8 * CH32T);
}

__attribute__((always_inline))
static inline void ch32_tx0() {
    dedic_gpio_bundle_write(ch32_dedic_gpio_handle, 0x01, 0x00);
    ch32_delay(35 * CH32T);
    dedic_gpio_bundle_write(ch32_dedic_gpio_handle, 0x01, 0x01);
    ch32_delay(8 * CH32T);
}

static inline void ch32_tx_stop() {
    dedic_gpio_bundle_write(ch32_dedic_gpio_handle, 0x01, 0x01);
    ch32_delay(128 * CH32T);
}

static inline void ch32_tx32(uint32_t word) {
    for (uint8_t bit = 0; bit < 32; bit++) {
        if (word & 0x80000000) {
            ch32_tx1();
        } else {
            ch32_tx0();
        }
        word <<= 1;
    }
}

static inline void ch32_tx7(uint8_t word) {
    for (uint8_t bit = 0; bit < 7; bit++) {
        if (word & 0x40) {
            ch32_tx1();
        } else {
            ch32_tx0();
        }
        word <<= 1;
    }
}

__attribute__((always_inline))
static inline bool ch32_rx() {
    return dedic_gpio_bundle_read_in(ch32_dedic_gpio_handle) & 0x01;
}

static inline uint32_t ch32_rx32() {
    uint32_t word = 0;
    for (uint8_t bit = 0; bit < 32; bit++) {
        dedic_gpio_bundle_write(ch32_dedic_gpio_handle, 0x01, 0x00); // Drive low
        ch32_delay(2 * CH32T);
        dedic_gpio_bundle_write(ch32_dedic_gpio_handle, 0x01, 0x01); // Float high
        ch32_delay(8 * CH32T);
        // Read input to see if CH32 is holding low
        word <<= 1;
        word |= dedic_gpio_bundle_read_in(ch32_dedic_gpio_handle) & 0x01;
        ch32_delay(10 * CH32T);
    }
    return word;
}

void ch32_sdi_reset() {
    dedic_gpio_bundle_write(ch32_dedic_gpio_handle, 0x01, 0x00);
    vTaskDelay(pdMS_TO_TICKS(1));
    dedic_gpio_bundle_write(ch32_dedic_gpio_handle, 0x01, 0x01);
    vTaskDelay(pdMS_TO_TICKS(1));
}

esp_err_t ch32_init(uint8_t pin) {
    esp_err_t res;

    gpio_reset_pin(pin);

    gpio_config_t cfg = {
        .pin_bit_mask = BIT64(pin),
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en   = false,
        .pull_down_en = false,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    res = gpio_config(&cfg);
    if (res != ESP_OK) {
        return res;
    }

    res = gpio_set_level(pin, true);
    if (res != ESP_OK) {
        return res;
    }

    int gpios[1];
    gpios[0] = pin;

    dedic_gpio_bundle_config_t dedic_gpio_config = {
        .gpio_array = gpios,
        .array_size = sizeof(gpios) / sizeof(gpios[0]),
        .flags = {
            .out_en = 1,
            .in_en = 1,
        },
    };

    dedic_gpio_new_bundle(&dedic_gpio_config, &ch32_dedic_gpio_handle);

    return ESP_OK;
}

void ch32_sdi_read(uint8_t address, uint32_t* value) {
    portDISABLE_INTERRUPTS();
    rv_utils_intr_global_disable();
    ch32_tx1();           // start bit (always 1)
    ch32_tx7(address);    // 7 bit address
    ch32_tx0();           // tead/write control bit (0 for host read)
    *value = ch32_rx32(); // 32 bit data
    ch32_tx_stop();
    rv_utils_intr_global_enable();
    portENABLE_INTERRUPTS();
}

void ch32_sdi_write(uint8_t address, uint32_t value) {
    portDISABLE_INTERRUPTS();
    rv_utils_intr_global_disable();
    ch32_tx1();        // start bit (always 1)
    ch32_tx7(address); // 7 bit address
    ch32_tx1();        // tead/write control bit (1 for host write)
    ch32_tx32(value);
    ch32_tx_stop();
    rv_utils_intr_global_enable();
    portENABLE_INTERRUPTS();
}

void ch32_sdi_write_bypass(uint32_t value) {
    portDISABLE_INTERRUPTS();
    rv_utils_intr_global_disable();
    ch32_tx0(); // start bit (always 0)
    ch32_tx32(value);
    ch32_tx_stop();
    rv_utils_intr_global_enable();
    portENABLE_INTERRUPTS();
}
