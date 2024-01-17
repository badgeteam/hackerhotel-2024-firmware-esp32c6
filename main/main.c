#include "application.h"
#include "bsp.h"
#include "driver/dedic_gpio.h"
#include "driver/gpio.h"
#include "epaper.h"
#include "esp_err.h"
#include "esp_log.h"
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

static char const *TAG = "main";

void app_main(void) {
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

    //ota_update(false);

    app_thread_entry();
}