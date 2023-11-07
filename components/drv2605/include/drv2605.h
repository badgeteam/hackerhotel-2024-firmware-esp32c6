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

// I2C address
#define DRV2605_ADDR 0x5A

// Registers
#define DRV2605_REG_STATUS      0x00
#define DRV2605_REG_MODE        0x01
#define DRV2605_REG_RTPIN       0x02
#define DRV2605_REG_LIBRARY     0x03
#define DRV2605_REG_WAVESEQ1    0x04
#define DRV2605_REG_WAVESEQ2    0x05
#define DRV2605_REG_WAVESEQ3    0x06
#define DRV2605_REG_WAVESEQ4    0x07
#define DRV2605_REG_WAVESEQ5    0x08
#define DRV2605_REG_WAVESEQ6    0x09
#define DRV2605_REG_WAVESEQ7    0x0A
#define DRV2605_REG_WAVESEQ8    0x0B
#define DRV2605_REG_GO          0x0C
#define DRV2605_REG_OVERDRIVE   0x0D
#define DRV2605_REG_SUSTAINPOS  0x0E
#define DRV2605_REG_SUSTAINNEG  0x0F
#define DRV2605_REG_BREAK       0x10
#define DRV2605_REG_AUDIOCTRL   0x11
#define DRV2605_REG_AUDIOLVL    0x12
#define DRV2605_REG_AUDIOMAX    0x13
#define DRV2605_REG_AUDIOOUTMIN 0x14
#define DRV2605_REG_AUDIOOUTMAX 0x15
#define DRV2605_REG_RATEDV      0x16
#define DRV2605_REG_CLAMPV      0x17
#define DRV2605_REG_AUTOCALCOMP 0x18
#define DRV2605_REG_AUTOCALEMP  0x19
#define DRV2605_REG_FEEDBACK    0x1A
#define DRV2605_REG_CONTROL1    0x1B
#define DRV2605_REG_CONTROL2    0x1C
#define DRV2605_REG_CONTROL3    0x1D
#define DRV2605_REG_CONTROL4    0x1E
#define DRV2605_REG_VBAT        0x21
#define DRV2605_REG_LRARESON    0x22

// Mode register values (0x01)
#define DRV2605_MODE_INTTRIG     0x00
#define DRV2605_MODE_EXTTRIGEDGE 0x01
#define DRV2605_MODE_EXTTRIGLVL  0x02
#define DRV2605_MODE_PWMANALOG   0x03
#define DRV2605_MODE_AUDIOVIBE   0x04
#define DRV2605_MODE_REALTIME    0x05
#define DRV2605_MODE_DIAGNOS     0x06
#define DRV2605_MODE_AUTOCAL     0x07

typedef struct DRV2605 {
    // Configuration
    int     i2c_bus;
    uint8_t i2c_address;
} drv2605_t;

esp_err_t drv2605_init(drv2605_t *device);
esp_err_t drv2605_sleep(drv2605_t *device);

#ifdef __cplusplus
}
#endif //__cplusplus
