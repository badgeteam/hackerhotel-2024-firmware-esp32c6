#include "application.h"
#include "badge_comms.h"
#include "badge_messages.h"
#include "bsp.h"
#include "driver/dedic_gpio.h"
#include "driver/gpio.h"
#include "epaper.h"
#include "esp_err.h"
#include "esp_ieee802154.h"
#include "esp_log.h"
#include "esp_mac.h"
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
#include "screen_battleship.h"
#include "screen_billboard.h"
#include "screen_hangman.h"
#include "screen_home.h"
#include "screen_library.h"
#include "screen_mascots.h"
#include "screen_pointclick.h"
#include "screen_repertoire.h"
#include "screen_scrambled.h"
#include "screen_settings.h"
#include "screen_shades.h"
#include "screen_template.h"
#include "screen_test.h"
#include "screens.h"
#include "sdkconfig.h"
#include "sdmmc_cmd.h"
#include "soc/gpio_struct.h"
#include "task_button_input_handler.h"
#include "task_keyboard.h"
#include "textedit.h"
#include "wifi_cert.h"
#include "wifi_connection.h"
#include "wifi_defaults.h"
#include "wifi_ota.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char const * TAG = "main";

static QueueHandle_t keyboard_event_queue    = NULL;
static QueueHandle_t application_event_queue = NULL;
static QueueHandle_t input_handler_queues[3] = {NULL};

esp_err_t setup(void) {
    esp_app_desc_t const * app_description = esp_app_get_description();
    printf("BADGE.TEAM %s firmware v%s\r\n", app_description->project_name, app_description->version);

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

    if ((res = init_badge_comms()) != ESP_OK) {
        bsp_display_error("Failed to initialize\nBodge comms failed to start");
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
    keyboard_event_queue    = xQueueCreate(8, sizeof(event_t));
    application_event_queue = xQueueCreate(8, sizeof(event_t));

    // Set up the input handler
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

    // LONG ADDRESS OWNER
    uint8_t mac_owner[8];
    for (int y = 0; y < 8; y++) {
        esp_read_mac(mac_owner, ESP_MAC_IEEE802154);
    }
    // SET SHORT ADDRESS OWNER
    uint16_t short_address_owner = (mac_owner[6] << 8) | mac_owner[7];
    if (esp_ieee802154_get_short_address() != short_address_owner) {
        esp_ieee802154_set_short_address(short_address_owner);
    }

    if (log) {
        for (int y = 0; y < 8; y++) {
            ESP_LOGE(TAG, "MAC address owner: %02x", mac_owner[y]);
        }
        ESP_LOGE(TAG, "short address owner: %04x", short_address_owner);
        ESP_LOGE(TAG, "stored owner short address: %04x", esp_ieee802154_get_short_address());
    }

    // Main application
    screen_t current_screen = screen_credits;
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
            case screen_shades:
                {
                    current_screen = screen_shades_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_billboard:
                {
                    current_screen = screen_billboard_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_battleship:
                {
                    current_screen = screen_battleship_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_pointclick:
                {
                    current_screen = screen_pointclick_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_repertoire:
                {
                    current_screen = screen_repertoire_entry(application_event_queue, keyboard_event_queue, 0);
                    break;
                }
            case screen_template:
                {
                    current_screen = screen_template_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_hangman:
                {
                    current_screen = screen_hangman_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_scrambled:
                {
                    current_screen = screen_scrambled_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_nametag:
                {
                    current_screen = screen_Nametag(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_library:
                {
                    current_screen = screen_library_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            case screen_credits:
                {
                    current_screen = screen_credits_entry(application_event_queue, keyboard_event_queue);
                    break;
                }
            default: current_screen = screen_home;
        }
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

    char buffer[64] = {0};
    bool editres =
        textedit("What is your name?", application_event_queue, keyboard_event_queue, buffer, sizeof(buffer));

    ESP_LOGW(
        TAG,
        "Text edit test: user %s the prompt and the resulting string is '%s'",
        editres ? "accepted" : "canceled",
        buffer
    );

    // return;

    // Configure keyboard
    event_t kbsettings = {
        .type                                 = event_control_keyboard,
        .args_control_keyboard.enable_typing  = true,
        .args_control_keyboard.enable_actions = {true, true, true, true, true},
        .args_control_keyboard.enable_leds    = true,
        .args_control_keyboard.enable_relay   = true,
        .args_control_keyboard.capslock       = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);


    // Test application
    /*while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button:
                    printf("Button input: %u / %u\n", event.args_input_button.button, event.args_input_button.state);
                    break;
                case event_input_keyboard:
                    if (event.args_input_keyboard.action >= 0) {
                        printf("Keyboard action: %u\n", event.args_input_keyboard.action);
                    } else {
                        printf("Keyboard input:  %c\n", event.args_input_keyboard.character);
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }*/

    // Configure keyboard
    event_t kbsettings2 = {
        .type                                 = event_control_keyboard,
        .args_control_keyboard.enable_typing  = true,
        .args_control_keyboard.enable_actions = {true, true, true, true, true},
        .args_control_keyboard.enable_leds    = true,
        .args_control_keyboard.enable_relay   = true,
        .args_control_keyboard.capslock       = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings2, portMAX_DELAY);


    // Start main app
    // app_thread_entry(application_event_queue);
}
