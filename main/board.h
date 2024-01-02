#pragma once

#include "esp_err.h"

// Constants for the pinout of the ESP32C6

#define I2C_BUS     0
#define I2C_SPEED   400000  // 400 kHz
#define I2C_TIMEOUT 250     // us

#define SPI_BUS SPI2_HOST


#define EPAPER_SPEED  10000000
#define EPAPER_WIDTH  128
#define EPAPER_HEIGHT 296

#define GPIO_SAO_IO1      0
#define GPIO_SAO_IO2      1
#define GPIO_BATT_ADC     2
#define GPIO_BATT_CHRG    3
#define GPIO_CH32_INT     4
#define GPIO_EPAPER_DCX   5
#define GPIO_I2C_SDA      6
#define GPIO_I2C_SCL      7
#define GPIO_EPAPER_CS    8
#define GPIO_UNUSED1      9
#define GPIO_EPAPER_BUSY  10
#define GPIO_USB_DN       12
#define GPIO_USB_DP       13
#define GPIO_RELAY        15
#define GPIO_EPAPER_RESET 16
#define GPIO_CH32         17
#define GPIO_SDCARD_CS    18
#define GPIO_SPI_MOSI     19
#define GPIO_SPI_MISO     20
#define GPIO_SPI_CLK      21
#define GPIO_LED_DATA     22
#define GPIO_UNUSED2      23

// Constants for communicating with the firmware in the CH32V003

#define LED_A (1 << 0)
#define LED_B (1 << 1)
#define LED_D (1 << 2)
#define LED_E (1 << 3)
#define LED_F (1 << 4)
#define LED_G (1 << 5)
#define LED_L (1 << 6)
#define LED_I (1 << 7)
#define LED_K (1 << 8)
#define LED_H (1 << 9)
#define LED_M (1 << 10)
#define LED_N (1 << 11)
#define LED_O (1 << 12)
#define LED_P (1 << 13)
#define LED_R (1 << 14)
#define LED_S (1 << 15)
#define LED_T (1 << 16)
#define LED_V (1 << 17)
#define LED_W (1 << 18)
#define LED_Y (1 << 19)

#define SWITCH_IDLE  0
#define SWITCH_LEFT  (1 << 0)
#define SWITCH_PRESS (1 << 1)
#define SWITCH_RIGHT (1 << 2)

#define SWITCH_1 0
#define SWITCH_2 1
#define SWITCH_3 2
#define SWITCH_4 3
#define SWITCH_5 4

esp_err_t     initialize_system();
unsigned long millis();
