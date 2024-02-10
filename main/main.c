#include "application.h"
#include "badge-communication-protocol.h"
#include "badge-communication.h"
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
#include "printer.h"
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
#include "task_term_input_handler.h"
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

static const char* TAG = "main";

static QueueHandle_t keyboard_event_queue                 = NULL;
static QueueHandle_t application_event_queue              = NULL;
static QueueHandle_t background_communication_event_queue = NULL;
static QueueHandle_t input_handler_queues[3]              = {NULL};
static QueueHandle_t communication_handler_queues[3]      = {NULL};

esp_err_t setup(void) {
    const esp_app_desc_t* app_description = esp_app_get_description();
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
    keyboard_event_queue                 = xQueueCreate(8, sizeof(event_t));
    application_event_queue              = xQueueCreate(8, sizeof(event_t));
    background_communication_event_queue = xQueueCreate(8, sizeof(event_t));

    // Set up the input handler
    input_handler_queues[0] = keyboard_event_queue;
    input_handler_queues[1] = application_event_queue;
    input_handler_queues[2] = NULL;  // Terminate the list
    if ((res = start_task_button_input_handler(input_handler_queues)) != ESP_OK) {
        bsp_display_error("Failed to start button input handler task");
        return;
    }

    // Set up the keyboard
    if ((res = start_task_keyboard(keyboard_event_queue, application_event_queue)) != ESP_OK) {
        bsp_display_error("Failed to start keyboard task");
        return;
    }

    // Set up the terminal input handler.
    if ((res = start_task_term_input_handler(application_event_queue)) != ESP_OK) {
        bsp_display_error("Failed to start terminal input handler task");
        return;
    }

    // Wireless communication
    communication_handler_queues[0] = application_event_queue;
    communication_handler_queues[1] = background_communication_event_queue;
    communication_handler_queues[2] = NULL;  // Terminate the list

    if ((res = badge_communication_init(communication_handler_queues)) != ESP_OK) {
        bsp_display_error("Failed to initialize\nBodge comms failed to init");
        return;
    }

    if ((res = badge_communication_start()) != ESP_OK) {
        bsp_display_error("Failed to initialize\nBodge comms failed to start");
        return;
    }

    // Printer
    if (printer_initialize()) {
        pax_buf_t* gfx = bsp_get_gfx_buffer();
        pax_center_text(gfx, BLACK, pax_font_marker, 12, gfx->height / 2, gfx->width - 12, "Printer connected");
        bsp_display_flush_with_lut(lut_4s);
        char outstring[256] = {0};
        while (1) {
            event_t event;
            if (xQueueReceive(background_communication_event_queue, &event, portMAX_DELAY) == pdTRUE) {
                switch (event.type) {
                    case event_communication:
                        {
                            switch (event.args_communication.type) {
                                case MESSAGE_TYPE_CHAT:
                                    char* nickname = event.args_communication.data_chat.nickname;
                                    char* payload  = event.args_communication.data_chat.payload;
                                    sprintf(outstring, "<%s> %s\r\n", nickname, payload);
                                    printf("%s", outstring);
                                    res = printer_print(outstring);
                                    if (res != ESP_OK) {
                                        ESP_LOGE(TAG, "Failed to print");
                                    }
                                    break;
                                default:
                                    {
                                        ESP_LOGI(
                                            TAG,
                                            "Received comms messsage type %u, ignored",
                                            event.args_communication.type
                                        );
                                        break;
                                    }
                            }
                        }
                    default: break;
                }
            }
        }
    }

    // Comms test

    /*char outstring[256] = {0};
    while (1) {
        event_t event;
        if (xQueueReceive(background_communication_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_communication:
                    {
                        switch (event.args_communication.type) {
                            case MESSAGE_TYPE_CHAT:
                                char* nickname = event.args_communication.data_chat.nickname;
                                char* payload  = event.args_communication.data_chat.payload;
                                sprintf(outstring, "<%s> %s\r\n", nickname, payload);
                                printf("%s", outstring);
                                break;
                            default:
                                {
                                    ESP_LOGI(
                                        TAG,
                                        "Received comms messsage type %u, ignored",
                                        event.args_communication.type
                                    );
                                    break;
                                }
                        }
                    }
                default: break;
            }
        }
    }*/

    /*while (1) {
        badge_message_chat_t message = {
            .nickname = "Test",
            .payload  = "Hello world",
        };
        badge_communication_send_chat(&message);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }*/

    // SAO leds test
    bsp_sao_addressable_led_enable();
    uint8_t saoleddata[] = {
        0x10,
        0x00,
        0x00,
        0x10,
        0x00,
        0x00,
        0x10,
        0x00,
        0x00,
        0x10,
        0x00,
        0x00,
        0x10,
        0x00,
        0x00,
        0x10,
        0x00,
        0x00,
    };
    bsp_sao_addressable_led_set(saoleddata, sizeof(saoleddata));

    enum _epaper_lut lut = lut_1s;
    esp_err_t        err = nvs_get_u8_wrapped("system", "lut", (uint8_t*)&lut);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Restoring active LUT to %d", lut);
        bsp_apply_lut(lut);
    } else {
        ESP_LOGE(TAG, "Failed to restore LUT: %s", esp_err_to_name(err));
    }

    // Main application
    screen_t current_screen = screen_pointclick;
    if (release_type == production)
        current_screen = screen_mascots;
    while (1) {
        ESP_LOGE(TAG, "Screen: %d", current_screen);
        switch (current_screen) {

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
            case screen_mascots:
                {
                    current_screen = screen_mascots_entry(application_event_queue, keyboard_event_queue);
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
