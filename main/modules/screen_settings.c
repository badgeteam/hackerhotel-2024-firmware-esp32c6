#include "screen_settings.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi_types.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "menu.h"
#include "nvs.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screens.h"
#include "textedit.h"
#include "wifi_defaults.h"
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

static esp_err_t nvs_get_u8_wrapped(char const * namespace, char const * key, uint8_t* value) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res != ESP_OK) {
        return res;
    }
    res = nvs_get_u8(handle, key, value);
    nvs_close(handle);
    return res;
}

static esp_err_t nvs_set_u8_wrapped(char const * namespace, char const * key, uint8_t value) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res != ESP_OK) {
        return res;
    }
    res = nvs_set_u8(handle, key, value);
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

typedef enum _menu_wifi_action {
    /* ==== GENERIC ACTIONS ==== */
    // Nothing happens.
    ACTION_NONE,
    // Go back to the parent menu.
    ACTION_BACK,

    /* ==== MAIN MENU ACTIONS ==== */
    // Show the current WiFi settings.
    ACTION_SHOW,
    // Scan for networks and pick one to connect to.
    ACTION_SCAN,
    // Manually edit the current WiFi settings.
    ACTION_MANUAL,
    // Reset WiFi settings to default.
    ACTION_DEFAULTS,

    /* ==== AUTH MODES ==== */
    ACTION_AUTH_OPEN,
    ACTION_AUTH_WEP,
    ACTION_AUTH_WPA_PSK,
    ACTION_AUTH_WPA2_PSK,
    ACTION_AUTH_WPA_WPA2_PSK,
    ACTION_AUTH_WPA2_ENTERPRISE,
    ACTION_AUTH_WPA3_PSK,
    ACTION_AUTH_WPA2_WPA3_PSK,
    ACTION_AUTH_WAPI_PSK,

    /* ==== PHASE2 AUTH MODES ==== */
    ACTION_PHASE2_EAP,
    ACTION_PHASE2_MSCHAPV2,
    ACTION_PHASE2_MSCHAP,
    ACTION_PHASE2_PAP,
    ACTION_PHASE2_CHAP,
} menu_wifi_action_t;

static bool edit_network_type(wifi_auth_mode_t* network_type) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();

    menu_t* menu = menu_alloc("Authentication mode", 14, 12);

    menu->fgColor           = BLACK;
    menu->bgColor           = WHITE;
    menu->selectedItemColor = BLACK;
    menu->bgTextColor       = WHITE;
    menu->borderColor       = BLACK;
    menu->titleColor        = WHITE;
    menu->titleBgColor      = BLACK;
    menu->scrollbarBgColor  = WHITE;
    menu->scrollbarFgColor  = BLACK;

    menu_insert_item(menu, "Insecure", NULL, (void*)ACTION_AUTH_OPEN, -1);
    menu_insert_item(menu, "WEP", NULL, (void*)ACTION_AUTH_WEP, -1);
    menu_insert_item(menu, "WPA PSK", NULL, (void*)ACTION_AUTH_WPA_PSK, -1);
    menu_insert_item(menu, "WPA2 PSK", NULL, (void*)ACTION_AUTH_WPA2_PSK, -1);
    // menu_insert_item(menu, "QQQQQQQQQQQQ", NULL, (void*) ACTION_AUTH_WPA_WPA2_PSK, -1);
    menu_insert_item(menu, "WPA2 Enterprise", NULL, (void*)ACTION_AUTH_WPA2_ENTERPRISE, -1);
    menu_insert_item(menu, "WPA3 PSK", NULL, (void*)ACTION_AUTH_WPA3_PSK, -1);
    // menu_insert_item(menu, "QQQQQQQQQQQQ", NULL, (void*) ACTION_AUTH_WPA2_WPA3_PSK, -1);
    // menu_insert_item(menu, "WAPI PSK", NULL, (void*) ACTION_AUTH_WAPI_PSK, -1);

    // Pre-select default authmode.
    for (int i = 0; i < menu_get_length(menu); i++) {
        if ((int)menu_get_callback_args(menu, i) - (int)ACTION_AUTH_OPEN == (int)*network_type) {
            menu_navigate_to(menu, i);
        }
    }

    bool               render = true;
    menu_wifi_action_t action = ACTION_NONE;
    wifi_auth_mode_t   pick   = *network_type;

    // action = (menu_wifi_action_t)menu_get_callback_args(menu, menu_get_position(menu));
    // menu_render(pax_buffer, menu, 0, 0, pax_buffer->width, 220);
    // menu_navigate_previous(menu);
    // menu_navigate_next(menu);

    /*if (action != ACTION_NONE) {
        if (action == ACTION_BACK) {
            pick = -1;
            break;
        } else {
            pick = (wifi_auth_mode_t)(action - ACTION_AUTH_OPEN);
            break;
        }
        render = true;
        action = ACTION_NONE;
        render_wifi_help(pax_buffer);
    }
}*/

    pax_background(gfx, WHITE);
    menu_render(gfx, menu, 0, 0, 296, 128);
    bsp_display_flush();
    ESP_LOGE(TAG, "MENU TEST NOW VISIBLE");

    vTaskDelay(pdMS_TO_TICKS(10000));

    menu_free(menu);
    *network_type = pick;
    return true;

    pax_font_t const * font = pax_font_saira_regular;
    pax_background(gfx, WHITE);
    pax_draw_text(gfx, RED, font, 18, 0, 0, "Select network type:\n1. Cancel\n3. Move to select\n5. Save and continue");

    char typename_open[]                   = "Open";
    char typename_wep[]                    = "WEP";
    char typename_wpa1_psk[]               = "WPA-PSK";
    char typename_wpa2_psk[]               = "WPA2-PSK";
    char typename_wpa12_psk[]              = "WPA-PSK / WPA2-PSK";
    char typename_wpa_enterprise[]         = "WPA / WPA2 enterprise";
    char typename_wpa3_psk[]               = "WPA3-PSK";
    char typename_wpa23_psk[]              = "WPA2-PSK / WPA3-PSK";
    char typename_wapi_psk[]               = "WAPI-PSK";
    char typename_owe[]                    = "OWE";
    char typename_wpa3_enterprise_ent192[] = "WPA3 enterprise ENT 192";

    char* typename = typename_open;
    switch (*network_type) {
        case WIFI_AUTH_OPEN: typename = typename_open; break;
        case WIFI_AUTH_WEP: typename = typename_wep; break;
        case WIFI_AUTH_WPA_PSK: typename = typename_wpa1_psk; break;
        case WIFI_AUTH_WPA2_PSK: typename = typename_wpa2_psk; break;
        case WIFI_AUTH_WPA_WPA2_PSK: typename = typename_wpa12_psk; break;
        case WIFI_AUTH_ENTERPRISE: typename = typename_wpa_enterprise; break;
        case WIFI_AUTH_WPA3_PSK: typename = typename_wpa3_psk; break;
        case WIFI_AUTH_WPA2_WPA3_PSK: typename = typename_wpa23_psk; break;
        case WIFI_AUTH_WAPI_PSK: typename = typename_wapi_psk; break;
        case WIFI_AUTH_OWE: typename = typename_owe; break;
        case WIFI_AUTH_WPA3_ENT_192: typename = typename_wpa3_enterprise_ent192; break;
        default:
            typename      = typename_open;
            *network_type = WIFI_AUTH_OPEN;
            break;
    }

    pax_draw_text(gfx, BLACK, font, 18, 0, 70, typename);

    bsp_display_flush();

    return false;
}

static void draw_wifi_defaults() {
    pax_font_t const * font = pax_font_saira_regular;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_draw_text(
        gfx,
        BLACK,
        font,
        18,
        5,
        5,
        "WiFi:\nButton 1: Cancel\nButton 4: Default settings\nButton 5: Manual WPA2-PSK configuration"
    );
    bsp_display_flush();
}

int wifi_defaults(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    configure_keyboard(keyboard_event_queue);
    draw_wifi_defaults();

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return 0; break;
                        case SWITCH_2: return 0; break;
                        case SWITCH_3: return 0; break;
                        case SWITCH_4: return 1; break;
                        case SWITCH_5: return 2; break;
                        default: break;
                    }
                    draw_wifi_defaults();
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}


static void edit_wifi(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    char             ssid[33]     = {0};
    char             password[65] = {0};
    wifi_auth_mode_t network_type = WIFI_AUTH_WPA2_PSK;

    int d = wifi_defaults(application_event_queue, keyboard_event_queue);

    if (d == 0) {
        return;
    }

    if (d == 1) {
        wifi_set_defaults();
        return;
    }

    nvs_get_str_wrapped("system", "wifi.ssid", ssid, sizeof(ssid));
    nvs_get_str_wrapped("system", "wifi.password", password, sizeof(password));

    bool res;

    res = textedit("Enter SSID", application_event_queue, keyboard_event_queue, ssid, sizeof(ssid));
    if (!res) {
        return;
    }

    res = textedit("Enter password", application_event_queue, keyboard_event_queue, password, sizeof(password));
    if (!res) {
        return;
    }

    nvs_set_str_wrapped("system", "wifi.ssid", ssid);
    nvs_set_str_wrapped("system", "wifi.password", password);
    nvs_set_u8_wrapped("system", "wifi.authmode", WIFI_AUTH_WPA2_PSK);

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
