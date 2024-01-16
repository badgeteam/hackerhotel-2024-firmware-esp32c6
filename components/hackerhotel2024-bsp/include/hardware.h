/*
 * Hackerhotel 2024 badge
 *
 * This file describes the connections from the ESP32 to the other
 * components on the Hackerhotel 2024 badge.
 *
 * More information can be found in the documentation at
 * https://www.badge.team/docs/badges/hackerhotel2024/pinout
 *
 */

#pragma once

// GPIO
#define GPIO_SAO_IO1      0
#define GPIO_SAO_IO2      1
#define GPIO_BATT_ADC     2  // analog input
#define GPIO_BATT_CHRG    3  // digital input, active low
#define GPIO_CH32_INT     4  // digital input, active low
#define GPIO_EPAPER_DCX   5  // digital output, epaper data/command pin
#define GPIO_I2C_SDA      6  // I2C bus SDA
#define GPIO_I2C_SCL      7  // I2C bus SCL
#define GPIO_EPAPER_CS    8  // digital output, epaper cs pin, active low
#define GPIO_UNUSED1      9  // On header
#define GPIO_EPAPER_BUSY  10  // digital input, epaper busy pin
#define GPIO_USB_DN       12  // USB
#define GPIO_USB_DP       13  // USB
#define GPIO_RELAY        15  // digital output, active high
#define GPIO_EPAPER_RESET 16  // digtial output, epaper reset pin
#define GPIO_CH32_SWD     17  // CH32V003 programming pin
#define GPIO_SDCARD_CS    18  // digital output, sdcard cs pin, active low
#define GPIO_SPI_MOSI     19  // SPI
#define GPIO_SPI_MISO     20  // SPI
#define GPIO_SPI_CLK      21  // SPI
#define GPIO_LED_DATA     22  // WS2812 LED data
#define GPIO_UNUSED2      23  // On header

// I2C bus
#define I2C_BUS     0
#define I2C_SPEED   400000  // 400 kHz
#define I2C_TIMEOUT 250     // us

#define COPROCESSOR_ADDR 0x42

// SPI bus
#define SPI_BUS SPI2_HOST

// SPI device: epaper
#define EPAPER_SPEED  10000000
#define EPAPER_WIDTH  128
#define EPAPER_HEIGHT 296

#define WHITE 0
#define BLACK 1
#define RED   2

// I2C device: CH32V003
#define COPROCESSOR_REG_FW_VERSION 0
#define COPROCESSOR_REG_LED        4
#define COPROCESSOR_REG_BTN        8