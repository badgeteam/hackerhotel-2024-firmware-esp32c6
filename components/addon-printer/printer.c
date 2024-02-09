#include "printer.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hardware.h"
#include "managed_i2c.h"
#include <string.h>

static const char* TAG = "printer";

bool printer_addon_present() {
    uint16_t  version = 0;
    esp_err_t res     = i2c_read_reg(
        I2C_BUS,
        PRINTER_ADDON_ADDRESS,
        PRINTER_ADDON_REG_FW_VERSION,
        (uint8_t*)&version,
        sizeof(uint16_t)
    );
    if (res != ESP_OK)
        return false;
    ESP_LOGW(TAG, "Printer addon firmware version %u", version);
    return (version > 0);
}

esp_err_t printer_addon_control(bool select, bool init, bool autofd) {
    uint8_t value = (select ? (1 << 0) : 0) | (init ? (1 << 1) : 0) | (autofd ? (1 << 2) : 0);
    return i2c_write_reg(I2C_BUS, PRINTER_ADDON_ADDRESS, PRINTER_ADDON_REG_CONTROL, value);
}

esp_err_t printer_addon_status(bool* select, bool* pe, bool* busy, bool* ack, bool* error) {
    uint8_t   value;
    esp_err_t res = i2c_read_reg(I2C_BUS, PRINTER_ADDON_ADDRESS, PRINTER_ADDON_REG_STATUS, &value, sizeof(uint8_t));
    if (res != ESP_OK) {
        return res;
    }
    if (select != NULL) {
        *select = (value >> 0) & 1;
    }

    if (pe != NULL) {
        *pe = (value >> 1) & 1;
    }

    if (busy != NULL) {
        *busy = (value >> 2) & 1;
    }

    if (ack != NULL) {
        *ack = (value >> 3) & 1;
    }

    if (error != NULL) {
        *error = !((value >> 4) & 1);
    }

    return ESP_OK;
}

esp_err_t printer_addon_send(const uint8_t* data, uint8_t length) {
    esp_err_t res = i2c_write_reg_n(I2C_BUS, PRINTER_ADDON_ADDRESS, PRINTER_ADDON_REG_DATA, data, length);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "i2c write data: error %d", res);
        return res;
    }
    return res;
}

bool printer_initialize() {
    esp_err_t res;
    if (printer_addon_present()) {
        ESP_LOGI(TAG, "Printer addon detected, initializing printer...");
        res = printer_addon_control(true, true, true);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to control printer addon");
            return false;
        }

        bool select = false;
        bool pe     = false;
        bool busy   = false;
        bool ack    = false;
        bool error  = false;

        for (uint8_t timeout = 100; timeout > 0; timeout--) {
            res = printer_addon_status(&select, &pe, &busy, &ack, &error);
            if (res != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read printer addon status");
                return false;
            }
            // printf("Printer status: %u %u %u %u %u\r\n", select, pe, busy, ack, error);
            if (select && (!busy) && (!error)) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        if (error) {
            ESP_LOGE(TAG, "Printer reports error");
            return false;
        }

        if (!select) {
            ESP_LOGE(TAG, "No printer connected to addon");
            return false;
        }

        if (busy) {
            ESP_LOGE(TAG, "Printer took too long to initialize");
            return false;
        }

        return true;
    }

    return false;
}

esp_err_t printer_print(char* text) {
    esp_err_t res;

    res = printer_addon_send((uint8_t*)text, strlen(text));
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send data to printer addon");
        return res;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    bool select = false;
    bool pe     = false;
    bool busy   = false;
    bool ack    = false;
    bool error  = false;
    for (uint8_t timeout = 100; timeout > 0; timeout--) {
        res = printer_addon_status(&select, &pe, &busy, &ack, &error);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read printer addon status");
            return res;
        }
        printf("Printer status: %u %u %u %u %u\r\n", select, pe, busy, ack, error);
        if (select && (!busy) && (!error)) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (error) {
        ESP_LOGE(TAG, "Printer reports error");
        return ESP_FAIL;
    }

    if (!select) {
        ESP_LOGE(TAG, "No printer connected to addon");
        return ESP_FAIL;
    }

    if (busy) {
        ESP_LOGE(TAG, "Printer took too long to initialize");
        return ESP_FAIL;
    }

    if (pe) {
        printf("PE STATUS SET\r\n");
    } else {
        printf("NO PE\r\n");
    }

    return ESP_OK;
}
