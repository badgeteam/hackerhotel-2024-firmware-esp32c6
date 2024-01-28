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
#include "events.h"
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
#include "task_button_input_handler.h"
#include "task_keyboard.h"
#include "wifi_cert.h"
#include "wifi_connection.h"
#include "wifi_defaults.h"
#include "wifi_ota.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static char const * TAG = "main";

esp_err_t setup() {
    esp_app_desc_t const * app_description = esp_app_get_description();
    printf("BADGE.TEAM %s firmware v%s\r\n", app_description->project_name, app_description->version);
    // banana
    esp_err_t res = bsp_init();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Hardware initialization failed, bailing out.");
        // vTaskDelay(pdMS_TO_TICKS(2000));
        // bsp_restart();
        return res;
    }

    wifi_init();

    if (!wifi_check_configured()) {
        ESP_LOGW(TAG, "WiFi not configured");
        if (!wifi_set_defaults()) {
            bsp_display_error("Failed to configure WiFi!\nFlash may be corrupted\nContact support");
            return ESP_FAIL;
        }
    }

    res = init_ca_store();
    if (res != ESP_OK) {
        bsp_display_error("Failed to initialize\nTLS certificate storage");
        return res;
    }

    return res;
}

void app_main(void) {
    esp_err_t res;

    // Set up the hardware and basic services
    res = setup();
    if (res != ESP_OK) {
        return;
    }

    // Create event queues
    QueueHandle_t keyboard_event_queue    = xQueueCreate(8, sizeof(event_t));
    QueueHandle_t application_event_queue = xQueueCreate(8, sizeof(event_t));

    // Set up the input handler
    QueueHandle_t input_handler_queues[3];
    input_handler_queues[0] = keyboard_event_queue;
    input_handler_queues[1] = application_event_queue;
    input_handler_queues[2] = NULL;  // Terminate the list
    res                     = start_task_button_input_handler(input_handler_queues);
    if (res != ESP_OK) {
        bsp_display_error("Failed to start button input handler task");
        return;
    }

    // Set up the keyboard
    res = start_task_keyboard(keyboard_event_queue, application_event_queue);
    if (res != ESP_OK) {
        bsp_display_error("Failed to start keyboard task");
        return;
    }

<<<<<<< Updated upstream
    // Test application
    /*while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button:
                    printf("Button input: %u / %u\n", event.args_input_button.button, event.args_input_button.state);
=======
    // Main application
    screen_t current_screen = screen_test;
    while (1) {
        switch (current_screen) {
            case screen_mascots:
                {
                    current_screen = screen_mascots_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_home:
                {
                    current_screen = screen_home_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_settings:
                {
                    current_screen = screen_settings_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_battleship:
                {
                    current_screen = screen_battleship_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_shades:
                {
                    current_screen = screen_shades_entry(application_event_queue, keyboard_event_queue);
>>>>>>> Stashed changes
                    break;
                case event_input_keyboard:
                    if (event.args_input_keyboard.action >= 0) {
                        printf("Keyboard action: %u\n", event.args_input_keyboard.action);
                    } else {
                        printf("Keyboard input:  %c\n", event.args_input_keyboard.character);
                    }
                    break;
<<<<<<< Updated upstream
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
=======
                }
            case screen_test:
                {
                    current_screen = screen_test_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            default: current_screen = screen_home;
>>>>>>> Stashed changes
        }
    }*/

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

    // Disable some of the keyboard functions for compatibility
    event_t kbsettings = {
        .type                                 = event_control_keyboard,
        .args_control_keyboard.enable_typing  = true,
        .args_control_keyboard.enable_actions = {true, true, true, true, true},
        .args_control_keyboard.enable_leds    = false,
        .args_control_keyboard.enable_relay   = true,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    // Start main app
    app_thread_entry(application_event_queue);
}