/**
 * Copyright (c) 2023 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

#include <stdbool.h>
#include <stdint.h>

#include <driver/spi_master.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

typedef struct _lut_group {
    uint8_t tp[4];
    uint8_t repeat;
} __attribute__((packed, aligned(1))) lut_group_t;

typedef struct _lut7 {
    uint8_t     vs[35];     // Register 0x32
    lut_group_t groups[7];  // Register 0x32
    uint8_t     vgh;        // Gate level (0x03)
    uint8_t     vsh1;       // Source level (0x04)
    uint8_t     vsh2;       // Source level (0x04)
    uint8_t     vsl;        // Source level (0x04)
    uint8_t     frame1;     // Dummy line (0x3A)
    uint8_t     frame2;     // Gate line width (0x3B)
} __attribute__((packed, aligned(1))) lut7_t;

typedef struct _hink {
    // Configuration
    int                 spi_bus;
    int                 pin_cs;
    int                 pin_dcx;
    int                 pin_reset;
    int                 pin_busy;
    uint32_t            spi_speed;
    uint32_t            spi_max_transfer_size;
    uint16_t            screen_width;
    uint16_t            screen_height;
    // Internal state
    spi_device_handle_t spi_device;
    bool                dc_level;
    SemaphoreHandle_t   mutex;
    uint8_t const      *lut;
} hink_t;

#define HINK_CMD_DRIVER_OUTPUT_CONTROL          0x01
#define HINK_CMD_GATE_DRIVING_VOLTAGE           0x03
#define HINK_CMD_SOURCE_DRIVING_VOLTAGE_CONTROL 0x04
#define HINK_CMD_BOOSTER_SOFT_START_CONTROL     0x0C
#define HINK_CMD_GATE_SCAN_START_POSITION       0x0F
#define HINK_CMD_DEEP_SLEEP_MODE                0x10
#define HINK_CMD_DATA_ENTRY_MODE_SETTING        0x11
#define HINK_CMD_SW_RESET                       0x12
#define HINK_CMD_SW_RESET_2                     0x13  // Not in datasheet
#define HINK_CMD_HV_READY_DETECTION             0x14
#define HINK_CMD_VCI_DETECTION                  0x15
#define HINK_CMD_TEMPERATURE_SENSOR_CONTROL     0x18
#define HINK_CMD_TEMPERATURE_SENSOR_WRITE       0x1A
#define HINK_CMD_TEMPERATURE_SENSOR_READ        0x1B
#define HINK_CMD_TEMPERATURE_SENSOR_WRITE_REG   0x1C
#define HINK_CMD_MASTER_ACTIVATION              0x20
#define HINK_CMD_DISPLAY_UPDATE_CONTROL_1       0x21
#define HINK_CMD_DISPLAY_UPDATE_CONTROL_2       0x22
#define HINK_CMD_WRITE_RAM_BLACK                0x24
#define HINK_CMD_WRITE_RAM_RED                  0x26
#define HINK_CMD_READ_RAM                       0x27
#define HINK_CMD_VCOM_SENSE                     0x28
#define HINK_CMD_VCOM_SENSE_DURATION            0x29
#define HINK_CMD_PROGRAM_VCOM_OTP               0x2A
#define HINK_CMD_UNKNOWN_1                      0x2B  // Not in datasheet
#define HINK_CMD_WRITE_VCOM_REGISTER            0x2C
#define HINK_CMD_OTP_REGISTER_READ              0x2D
#define HINK_CMD_USER_ID_READ                   0x2E
#define HINK_CMD_STATUS_BIT_READ                0x2F
#define HINK_CMD_PROGRAM_WS_OTP                 0x30
#define HINK_CMD_LOAD_WS_OTP                    0x31
#define HINK_CMD_WRITE_LUT_REGISTER             0x32
#define HINK_CMD_READ_LUT_REGISTER              0x33  // Not in datasheet
#define HINK_CMD_CRC_CALCULATION                0x34
#define HINK_CMD_CRC_STATUS_READ                0x35
#define HINK_CMD_PROGRAM_OTP_SELECTION          0x36
#define HINK_CMD_WRITE_OTP_SELECTION            0x37
#define HINK_CMD_WRITE_USER_ID                  0x38
#define HINK_CMD_OTP_PROGRAM_MODE               0x39
#define HINK_CMD_SET_DUMMY_LINE_PERIOD          0x3A
#define HINK_CMD_SET_GATE_LINE_WIDTH            0x3B
#define HINK_CMD_BORDER_WAVEFORM_CONTROL        0x3C
#define HINK_CMD_READ_RAM_OPTION                0x41
#define HINK_CMD_SET_RAM_X_ADDRESS_LIMITS       0x44
#define HINK_CMD_SET_RAM_Y_ADDRESS_LIMITS       0x45
#define HINK_CMD_AUTO_WRITE_RED_RAM             0x46
#define HINK_CMD_AUTO_WRITE_BW_RAM              0x47
#define HINK_CMD_SET_RAM_X_ADDRESS_COUNTER      0x4E
#define HINK_CMD_SET_RAM_Y_ADDRESS_COUNTER      0x4F
#define HINK_CMD_SET_ANALOG_BLOCK_CONTROL       0x74
#define HINK_CMD_SET_DIGITAL_BLOCK_CONTROL      0x7E
#define HINK_CMD_NOP                            0x7F

#define HINK_DISPLAY_UPDATE_CONTROL_2_CLOCK_OFF             0x01
#define HINK_DISPLAY_UPDATE_CONTROL_2_ANALOG_OFF            0x02
#define HINK_DISPLAY_UPDATE_CONTROL_2_USE_MODE_1            0x04
#define HINK_DISPLAY_UPDATE_CONTROL_2_USE_MODE_2            0x08
#define HINK_DISPLAY_UPDATE_CONTROL_2_LOAD_LUT              0x10
#define HINK_DISPLAY_UPDATE_CONTROL_2_LATCH_TEMPERATURE_VAL 0x20
#define HINK_DISPLAY_UPDATE_CONTROL_2_ANALOG_ON             0x40
#define HINK_DISPLAY_UPDATE_CONTROL_2_CLOCK_ON              0x80

esp_err_t hink_init(hink_t *device);
esp_err_t hink_deinit(hink_t *device);

bool      hink_busy(hink_t *device);
esp_err_t hink_wait(hink_t *device);

// Write data to the display.
esp_err_t hink_write(hink_t *device, uint8_t const *data);
// Set the active LUT.
// Does not create a copy of the LUT.
esp_err_t hink_set_lut(hink_t *device, uint8_t const *lut);

esp_err_t hink_set_gate_driving_voltage(hink_t *device, uint8_t value);
esp_err_t hink_set_source_driving_voltage(hink_t *device, uint8_t vsh1, uint8_t vsh2, uint8_t vsl);
esp_err_t hink_set_dummy_line_period(hink_t *device, uint8_t period);
esp_err_t hink_set_gate_line_width(hink_t *device, uint8_t width);

esp_err_t hink_sleep(hink_t *device);

void hink_read_lut(int pin_data, int pin_clk, int pin_cs, int pin_dc, int pin_reset, int pin_busy);

#ifdef __cplusplus
}
#endif  //__cplusplus
