#pragma once

#include "esp_err.h"
#include <stdbool.h>

#define PRINTER_ADDON_ADDRESS 0x43

// Registers
#define PRINTER_ADDON_REG_FW_VERSION 0
#define PRINTER_ADDON_REG_STATUS     4
#define PRINTER_ADDON_REG_CONTROL    5
#define PRINTER_ADDON_REG_DATA       6

// Types

// Functions
bool      printer_addon_present();
esp_err_t printer_addon_control(bool select, bool init, bool autofd);
esp_err_t printer_addon_status(bool* select, bool* pe, bool* busy, bool* ack, bool* error);
esp_err_t printer_addon_send(const uint8_t* data, uint8_t length);
bool      printer_initialize();
esp_err_t printer_print(char* text);
