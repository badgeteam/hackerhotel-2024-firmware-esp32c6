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
#include <stdio_ext.h>
#include <unistd.h>
#include <fcntl.h>
#include "soc/gpio_struct.h"
#include "freertos/portmacro.h"
#include "riscv/rv_utils.h"
#include "driver/dedic_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define PROTOCOL_START     '!'
#define PROTOCOL_ACK       '+'
#define PROTOCOL_TEST      '?'
#define PROTOCOL_POWER_ON  'p'
#define PROTOCOL_POWER_OFF 'P'
#define PROTOCOL_WRITE_REG 'w'
#define PROTOCOL_READ_REG  'r'

void handle_command(int c) {
    uint8_t reg;
    uint32_t val;
    switch (c) {
        case PROTOCOL_TEST:
            fputc(PROTOCOL_ACK, stdout);
            break;
        case PROTOCOL_POWER_ON:
            // Dummy, not supported
            fputc(PROTOCOL_ACK, stdout);
            break;
        case PROTOCOL_POWER_OFF:
            // Dummy, not supported
            fputc(PROTOCOL_ACK, stdout);
            break;
        case PROTOCOL_WRITE_REG:
            fread(&reg, sizeof(uint8_t), 1, stdin);
            fread(&val, sizeof(uint32_t), 1, stdin);
            ch32_sdi_write(reg, val);
            fputc(PROTOCOL_ACK, stdout);
            break;
        case PROTOCOL_READ_REG:
            fread(&reg, sizeof(uint8_t), 1, stdin);
            ch32_sdi_read(reg, &val);
            fwrite(&val, sizeof(uint32_t), 1, stdout);
            break;
    }
    fflush(stdout);
    fsync(fileno(stdout));
}

void ch32_programmer() {
    setvbuf(stdin, NULL, _IONBF, 0);
    fcntl(fileno(stdout), F_SETFL, 0);
    fcntl(fileno(stdin), F_SETFL, 0);
    fputc(PROTOCOL_START, stdout);
    fflush(stdout);
    fsync(fileno(stdout));
    while (1) {
        int c = fgetc(stdin);
        if (c != EOF) {
            handle_command(c);
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
