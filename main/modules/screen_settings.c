#include "screen_settings.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screens.h"
#include "textedit.h"
#include "wifi_ota.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

extern uint8_t const settings_png_start[] asm("_binary_settings_png_start");
extern uint8_t const settings_png_end[] asm("_binary_settings_png_end");

static char const * TAG = "settings";

static void configure_keyboard(QueueHandle_t keyboard_event_queue) {
    event_t kbsettings = {
        .type                                     = event_control_keyboard,
        .args_control_keyboard.enable_typing      = false,
        .args_control_keyboard.enable_actions     = {true, true, true, true, true},
        .args_control_keyboard.enable_leds        = true,
        .args_control_keyboard.enable_relay       = true,
        kbsettings.args_control_keyboard.capslock = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

static void ota_update_wrapped(QueueHandle_t keyboard_event_queue, bool nightly) {
    event_t kbsettings = {
        .type                                     = event_control_keyboard,
        .args_control_keyboard.enable_typing      = false,
        .args_control_keyboard.enable_actions     = {false, false, false, false, false},
        .args_control_keyboard.enable_leds        = true,
        .args_control_keyboard.enable_relay       = false,
        kbsettings.args_control_keyboard.capslock = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    ota_update(nightly);

    configure_keyboard(keyboard_event_queue);
}

static esp_err_t nvs_get_str_wrapped(char const * namespace, char const * key, char* buffer, size_t buffer_size) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res == ESP_OK) {
        size_t size = 0;
        res         = nvs_get_str(handle, key, NULL, &size);
        if ((res == ESP_OK) && (size <= buffer_size - 1)) {
            res = nvs_get_str(handle, key, buffer, &size);
            if (res != ESP_OK) {
                buffer[0] = '\0';
            }
        }
    } else {
        buffer[0] = '\0';
    }
    nvs_close(handle);
    return res;
}

static esp_err_t nvs_set_str_wrapped(char const * namespace, char const * key, char* buffer) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res == ESP_OK) {
        res = nvs_set_str(handle, key, buffer);
    }
    nvs_close(handle);
    return res;
}

static void edit_nickname(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    char nickname[64] = {0};
    nvs_get_str_wrapped("owner", "nickname", nickname, sizeof(nickname));
    bool res =
        textedit("What is your name?", application_event_queue, keyboard_event_queue, nickname, sizeof(nickname));
    if (res) {
        nvs_set_str_wrapped("owner", "nickname", nickname);
    }

    configure_keyboard(keyboard_event_queue);
}

static void edit_wifi(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    char ssid[64] = {0};
    bool res      = textedit("Enter SSID", application_event_queue, keyboard_event_queue, ssid, sizeof(ssid));

    char password[64] = {0};
    res = textedit("Enter password", application_event_queue, keyboard_event_queue, password, sizeof(password));

    configure_keyboard(keyboard_event_queue);
}

static void draw() {
    pax_font_t const * font = pax_font_saira_regular;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_draw_text(gfx, RED, font, 18, 5, 5, "Settings & OTA update");
    pax_insert_png_buf(gfx, settings_png_start, settings_png_end - settings_png_start, 0, gfx->width - 24, 0);
    bsp_display_flush();
}

screen_t screen_settings_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    configure_keyboard(keyboard_event_queue);
    draw();

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: edit_nickname(application_event_queue, keyboard_event_queue); break;
                        case SWITCH_3: edit_wifi(application_event_queue, keyboard_event_queue); break;
                        case SWITCH_4: ota_update_wrapped(keyboard_event_queue, false); break;
                        case SWITCH_5: ota_update_wrapped(keyboard_event_queue, true); break;
                        default: break;
                    }
                    draw();
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
