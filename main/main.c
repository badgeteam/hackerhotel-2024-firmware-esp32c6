#include "application.h"
#include "bsp.h"
#include "driver/dedic_gpio.h"
#include "driver/gpio.h"
#include "epaper.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hextools.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "resources.h"
#include "riscv/rv_utils.h"
#include "sdkconfig.h"
#include "sdmmc_cmd.h"
#include "soc/gpio_struct.h"
#include "wifi_cert.h"
#include "wifi_connection.h"
#include "wifi_defaults.h"
#include "wifi_ota.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_vfs_fat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <sdkconfig.h>
#include <sdmmc_cmd.h>
#include <string.h>
#include <time.h>

static char const * TAG = "main";

void app_main(void) {
    esp_app_desc_t const * app_description = esp_ota_get_app_description();
    printf("BADGE.TEAM %s firmware v%s\r\n", app_description->project_name, app_description->version);

    esp_err_t res = bsp_init();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Hardware initialization failed, bailing out.");
        // vTaskDelay(pdMS_TO_TICKS(2000));
        // bsp_restart();
        return;
    }

    wifi_init();

    if (!wifi_check_configured()) {
        ESP_LOGW(TAG, "WiFi not configured");
        if (wifi_set_defaults()) {
            bsp_display_message("Notice", "WiFi configuration reset\nto defaults");
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            bsp_display_error("Failed to configure WiFi!\nFlash may be corrupted\nContact support");
            return;
        }
    }

    res = init_ca_store();
    if (res != ESP_OK) {
        bsp_display_error("Failed to initialize\nTLS certificate storage");
        return;
    }

    /*nvs_handle_t nvs_handle;
    nvs_open("system", NVS_READWRITE, &nvs_handle);
    nvs_set_str(nvs_handle, "wifi.ssid", "ssid");
    nvs_set_str(nvs_handle, "wifi.password", "password");
    nvs_set_u8(nvs_handle, "wifi.authmode", WIFI_AUTH_WPA2_PSK);
    nvs_close(nvs_handle);*/

    /*while (1) {
        bool charging = bsp_battery_charging();
        float voltage = bsp_battery_voltage();
        char buffer[64];
        sprintf(buffer, "Charging: %s, voltage %f\n", charging ? "yes" : "no", voltage);
        printf(buffer);
        bsp_display_message("Battery measurements", buffer);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }*/

    /*QueueHandle_t queue              = bsp_get_button_queue();
    uint8_t       button_state_left  = 0x00;
    uint8_t       button_state_right = 0x00;
    uint8_t       button_state_press = 0x00;
    bool          wait_for_release   = false;
    while (1) {
        coprocessor_input_message_t buttonMessage = {0};
        if (xQueueReceive(queue, &buttonMessage, portMAX_DELAY) == pdTRUE) {
            button_state_left  &= ~(1 << buttonMessage.button);
            button_state_right &= ~(1 << buttonMessage.button);
            button_state_press &= ~(1 << buttonMessage.button);
            switch (buttonMessage.state) {
                case SWITCH_LEFT: button_state_left |= 1 << buttonMessage.button; break;
                case SWITCH_RIGHT: button_state_right |= 1 << buttonMessage.button; break;
                case SWITCH_PRESS: button_state_press |= 1 << buttonMessage.button; break;
            }

            printf("LEFT: %02x RIGHT: %02x PRESS: %02x\n", button_state_left, button_state_right, button_state_press);

            uint32_t leds = 0;

            bool caps = true;

            if (button_state_press) {
                // Action
            }
            if (button_state_left && button_state_right) {
                // Select character
                char character = '\0';
                if (button_state_left == 0x01 && button_state_right == 0x10) {
                    character  = 'A';
                    leds      |= LED_A;
                }
                if (button_state_left == 0x01 && button_state_right == 0x08) {
                    character  = 'B';
                    leds      |= LED_B;
                }
                // C
                if (button_state_left == 0x02 && button_state_right == 0x10) {
                    character  = 'D';
                    leds      |= LED_D;
                }
                if (button_state_left == 0x01 && button_state_right == 0x04) {
                    character  = 'E';
                    leds      |= LED_E;
                }
                if (button_state_left == 0x02 && button_state_right == 0x08) {
                    character  = 'F';
                    leds      |= LED_F;
                }
                if (button_state_left == 0x04 && button_state_right == 0x10) {
                    character  = 'G';
                    leds      |= LED_G;
                }
                if (button_state_left == 0x01 && button_state_right == 0x02) {
                    character  = 'H';
                    leds      |= LED_H;
                }
                if (button_state_left == 0x02 && button_state_right == 0x04) {
                    character  = 'I';
                    leds      |= LED_I;
                }
                // J
                if (button_state_left == 0x04 && button_state_right == 0x08) {
                    character  = 'K';
                    leds      |= LED_K;
                }
                if (button_state_left == 0x08 && button_state_right == 0x10) {
                    character  = 'L';
                    leds      |= LED_L;
                }
                if (button_state_left == 0x02 && button_state_right == 0x01) {
                    character  = 'M';
                    leds      |= LED_M;
                }
                if (button_state_left == 0x04 && button_state_right == 0x02) {
                    character  = 'N';
                    leds      |= LED_N;
                }
                if (button_state_left == 0x08 && button_state_right == 0x04) {
                    character  = 'O';
                    leds      |= LED_O;
                }
                if (button_state_left == 0x10 && button_state_right == 0x08) {
                    character  = 'P';
                    leds      |= LED_P;
                }
                // Q
                if (button_state_left == 0x04 && button_state_right == 0x01) {
                    character  = 'R';
                    leds      |= LED_R;
                }
                if (button_state_left == 0x08 && button_state_right == 0x02) {
                    character  = 'S';
                    leds      |= LED_S;
                }
                if (button_state_left == 0x10 && button_state_right == 0x04) {
                    character  = 'T';
                    leds      |= LED_T;
                }
                // U
                if (button_state_left == 0x08 && button_state_right == 0x01) {
                    character  = 'V';
                    leds      |= LED_V;
                }
                if (button_state_left == 0x10 && button_state_right == 0x02) {
                    character  = 'W';
                    leds      |= LED_W;
                }
                // X
                if (button_state_left == 0x10 && button_state_right == 0x01) {
                    character  = 'Y';
                    leds      |= LED_Y;
                }
                // Z

                if (character != '\0' && !caps) {
                    character += 32;  // replace uppercase character with lowercase character
                }

                if (character != 0) {
                    printf("Selected character: %c\n", character);
                }
            } else {
                // Select row
                if (button_state_left & 0x01)
                    leds |= LED_A | LED_B | LED_E | LED_H;
                if (button_state_left & 0x02)
                    leds |= LED_M | LED_I | LED_F | LED_D;
                if (button_state_left & 0x04)
                    leds |= LED_R | LED_N | LED_K | LED_G;
                if (button_state_left & 0x08)
                    leds |= LED_V | LED_S | LED_O | LED_L;
                if (button_state_left & 0x10)
                    leds |= LED_Y | LED_W | LED_T | LED_P;
                if (button_state_right & 0x01)
                    leds |= LED_M | LED_R | LED_V | LED_Y;
                if (button_state_right & 0x02)
                    leds |= LED_H | LED_N | LED_S | LED_W;
                if (button_state_right & 0x04)
                    leds |= LED_E | LED_I | LED_O | LED_T;
                if (button_state_right & 0x08)
                    leds |= LED_B | LED_F | LED_K | LED_P;
                if (button_state_right & 0x10)
                    leds |= LED_A | LED_D | LED_G | LED_L;
            }

            esp_err_t _res = bsp_set_leds(leds);
            if (_res != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set LEDs (%d)\n", _res);
            }
        }
    }*/

    app_thread_entry();
}