#include "screen_settings.h"
#include "application.h"
#include "badge-communication-protocol.h"
#include "badge-communication.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
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

extern const uint8_t settings_png_start[] asm("_binary_settings_png_start");
extern const uint8_t settings_png_end[] asm("_binary_settings_png_end");
extern const uint8_t battery1_png_start[] asm("_binary_battery1_png_start");
extern const uint8_t battery1_png_end[] asm("_binary_battery1_png_end");
extern const uint8_t battery2_png_start[] asm("_binary_battery2_png_start");
extern const uint8_t battery2_png_end[] asm("_binary_battery2_png_end");
extern const uint8_t lutdial_png_start[] asm("_binary_lutdial_png_start");
extern const uint8_t lutdial_png_end[] asm("_binary_lutdial_png_end");

static const char* TAG = "settings";

static void configure_keyboard(QueueHandle_t keyboard_event_queue) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
}

static void ota_update_wrapped(QueueHandle_t keyboard_event_queue, bool nightly) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_relay(keyboard_event_queue, false);
    bsp_set_relay(false);
    // event_t kbsettings = {
    //     .type                                     = event_control_keyboard,
    //     .args_control_keyboard.enable_typing      = false,
    //     .args_control_keyboard.enable_actions     = {false, false, false, false, false},
    //     .args_control_keyboard.enable_leds        = true,
    //     .args_control_keyboard.enable_relay       = false,
    //     kbsettings.args_control_keyboard.capslock = false,
    // };
    // xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    badge_communication_stop();

    ota_update(nightly);

    badge_communication_start();

    configure_keyboard(keyboard_event_queue);
}

// static esp_err_t nvs_get_str_wrapped(char const * namespace, char const * key, char* buffer, size_t buffer_size) {
//     nvs_handle_t handle;
//     esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
//     if (res == ESP_OK) {
//         size_t size = 0;
//         res         = nvs_get_str(handle, key, NULL, &size);
//         if ((res == ESP_OK) && (size <= buffer_size - 1)) {
//             res = nvs_get_str(handle, key, buffer, &size);
//             if (res != ESP_OK) {
//                 buffer[0] = '\0';
//             }
//         }
//     } else {
//         buffer[0] = '\0';
//     }
//     nvs_close(handle);
//     return res;
// }

// static esp_err_t nvs_set_str_wrapped(char const * namespace, char const * key, char* buffer) {
//     nvs_handle_t handle;
//     esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
//     if (res == ESP_OK) {
//         res = nvs_set_str(handle, key, buffer);
//     }
//     nvs_commit(handle);
//     nvs_close(handle);
//     return res;
// }

// static esp_err_t nvs_get_u8_wrapped(char const * namespace, char const * key, uint8_t* value) {
//     nvs_handle_t handle;
//     esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
//     if (res != ESP_OK) {
//         return res;
//     }
//     res = nvs_get_u8(handle, key, value);
//     nvs_close(handle);
//     return res;
// }

// static esp_err_t nvs_set_u8_wrapped(char const * namespace, char const * key, uint8_t value) {
//     nvs_handle_t handle;
//     esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
//     if (res != ESP_OK) {
//         return res;
//     }
//     res = nvs_set_u8(handle, key, value);
//     nvs_commit(handle);
//     nvs_close(handle);
//     return res;
// }

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

    //    bool               render = true;
    //    menu_wifi_action_t action = ACTION_NONE;
    wifi_auth_mode_t pick = *network_type;

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

    const pax_font_t* font = pax_font_saira_regular;
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
    const pax_font_t* font = pax_font_saira_regular;
    pax_buf_t*        gfx  = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_draw_text(
        gfx,
        BLACK,
        font,
        18,
        5,
        5,
        "WiFi:\nButton 1&2: Return\nButton 3: Manual enterprise\nButton 4: Default settings\nButton 5: Manual WPA2-PSK "
        "configuration"
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
                        case SWITCH_3: return 3; break;
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
    char ssid[33]     = {0};
    char password[65] = {0};
    char username[65] = {0};
    char identity[65] = {0};
    bool res;

    int d = wifi_defaults(application_event_queue, keyboard_event_queue);

    switch (d) {
        case 1:  // set hackerhotel defaults
            wifi_set_defaults();
            return;
        case 2:  // set custom WPA2_PSK network
            nvs_get_str_wrapped("system", "wifi.ssid", ssid, sizeof(ssid));
            nvs_get_str_wrapped("system", "wifi.password", password, sizeof(password));

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
            return;
        case 3:
            nvs_get_str_wrapped("system", "wifi.ssid", ssid, sizeof(ssid));
            nvs_get_str_wrapped("system", "wifi.password", password, sizeof(password));
            nvs_get_str_wrapped("system", "wifi.username", username, sizeof(username));
            nvs_get_str_wrapped("system", "wifi.anon_ident", identity, sizeof(identity));

            res = textedit("Enter SSID", application_event_queue, keyboard_event_queue, ssid, sizeof(ssid));
            if (!res) {
                return;
            }

            res = textedit("Enter password", application_event_queue, keyboard_event_queue, password, sizeof(password));
            if (!res) {
                return;
            }

            res = textedit("Enter username", application_event_queue, keyboard_event_queue, username, sizeof(username));
            if (!res) {
                return;
            }

            res = textedit("Enter identity", application_event_queue, keyboard_event_queue, identity, sizeof(identity));
            if (!res) {
                return;
            }

            nvs_set_u8_wrapped("system", "wifi.authmode", WIFI_AUTH_ENTERPRISE);
            nvs_set_u8_wrapped("system", "wifi.phase2", ESP_EAP_TTLS_PHASE2_PAP);
            nvs_set_str_wrapped("system", "wifi.ssid", ssid);
            nvs_set_str_wrapped("system", "wifi.username", username);
            nvs_set_str_wrapped("system", "wifi.anon_ident", identity);
            nvs_set_str_wrapped("system", "wifi.password", password);

            configure_keyboard(keyboard_event_queue);
            return;
        default:  // cancel
            return;
    }
}

static void draw() {
    const pax_font_t* font = pax_font_saira_regular;
    pax_buf_t*        gfx  = bsp_get_gfx_buffer();
    pax_background(gfx, WHITE);
    pax_draw_text(gfx, RED, font, 18, 5, 5, "Settings & OTA update");
    pax_insert_png_buf(gfx, settings_png_start, settings_png_end - settings_png_start, 0, gfx->width - 24, 0);
    bsp_display_flush();
}

// screen_t screen_settings_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
//     if (log)
//         ESP_LOGE(TAG, "Enter screen_settings_entry");
//     configure_keyboard(keyboard_event_queue);
//     draw();

//     while (1) {
//         event_t event = {0};
//         if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
//             switch (event.type) {
//                 case event_input_button: break;  // Ignore raw button input
//                 case event_input_keyboard:
//                     switch (event.args_input_keyboard.action) {
//                         case SWITCH_1: return screen_home; break;
//                         // case SWITCH_2: edit_nickname(application_event_queue, keyboard_event_queue); break;
//                         case SWITCH_3: edit_wifi(application_event_queue, keyboard_event_queue); break;
//                         case SWITCH_4: ota_update_wrapped(keyboard_event_queue, false); break;
//                         case SWITCH_5: ota_update_wrapped(keyboard_event_queue, true); break;
//                         default: break;
//                     }
//                     draw();
//                     break;
//                 default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
//             }
//         }
//     }
// }

const float OCVLut[32][2] = {{0, 2.7296},      {0.0164, 3.0857}, {0.0328, 3.2497}, {0.0492, 3.3247}, {0.0656, 3.3609},
                             {0.082, 3.3821},  {0.0984, 3.3991}, {0.1092, 3.41},   {0.12, 3.4212},   {0.1308, 3.4329},
                             {0.1416, 3.4448}, {0.1523, 3.457},  {0.188, 3.4963},  {0.2237, 3.5314}, {0.2594, 3.5611},
                             {0.2951, 3.5865}, {0.3308, 3.6099}, {0.3867, 3.6471}, {0.4426, 3.6893}, {0.4985, 3.7382},
                             {0.5544, 3.7931}, {0.6103, 3.8511}, {0.6709, 3.9139}, {0.7315, 3.9728}, {0.792, 4.025},
                             {0.8526, 4.0694}, {0.9132, 4.108},  {0.9306, 4.1187}, {0.9479, 4.1297}, {0.9653, 4.1412},
                             {0.9826, 4.1537}, {1, 4.1676}};

screen_t screen_lut_dial(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    if (log)
        ESP_LOGE(TAG, "Enter screen_home_entry");
    // update the keyboard event handler settings
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, false, false, false, false);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor = 1;

    while (1) {
        pax_buf_t* gfx = bsp_get_gfx_buffer();
        pax_insert_png_buf(gfx, lutdial_png_start, lutdial_png_end - lutdial_png_start, 0, 0, 0);
        AddOneTextSWtoBuffer(SWITCH_1, "Exit");
        DrawArrowHorizontal(SWITCH_5);
        int offX = 183;
        int offY = 15;
        int gapY = 15;
        pax_draw_text(gfx, BLACK, font1, fontsizeS, offX, offY, "1 sec refresh");
        pax_draw_text(gfx, BLACK, font1, fontsizeS, offX, offY + gapY, "4 sec refresh");
        pax_draw_text(gfx, BLACK, font1, fontsizeS, offX, offY + gapY * 2, "8 sec refresh");
        pax_draw_text(gfx, BLACK, font1, fontsizeS, offX, offY + gapY * 3, "24 sec refresh");
        switch (cursor) {
            case 0:
                bsp_apply_lut(lut_1s);
                pax_simple_tri(gfx, BLACK, 69, 71, 72, 69, 55, 57);
                break;
            case 1:
                bsp_apply_lut(lut_4s);
                pax_simple_tri(gfx, BLACK, 71, 69, 74, 69, 65, 48);
                break;
            case 2:
                bsp_apply_lut(lut_8s);
                pax_simple_tri(gfx, BLACK, 72, 68, 74, 69, 80, 48);
                break;
            case 3:
                bsp_apply_lut(lut_full);
                pax_simple_tri(gfx, BLACK, 73, 69, 76, 71, 90, 57);
                break;
            default: break;
        }
        bsp_display_flush();

        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_settings; break;
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        case SWITCH_L5:
                            if (cursor)
                                cursor--;
                            break;
                        case SWITCH_R5:
                            if (cursor < 4)
                                cursor++;
                            break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_settings_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    if (log)
        ESP_LOGE(TAG, "Enter screen_home_entry");
    // update the keyboard event handler settings
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    char voltage[15]      = "";
    char SoC[15]          = "";
    char oldvoltage[15]   = "";
    char oldSoC[15]       = "";
    int  timer_track      = 0;
    int  oldchargingstate = -1;

    while (1) {
        if ((esp_timer_get_time() / 1000000) > (timer_track * timer_battery_screen) ||
            (oldchargingstate != bsp_battery_charging())) {
            timer_track = (esp_timer_get_time() / (timer_battery_screen * 1000000)) + 1;

            pax_buf_t* gfx = bsp_get_gfx_buffer();
            if (bsp_battery_charging())
                pax_insert_png_buf(gfx, battery2_png_start, battery2_png_end - battery2_png_start, 0, 0, 0);
            else
                pax_insert_png_buf(gfx, battery1_png_start, battery1_png_end - battery1_png_start, 0, 0, 0);

            AddOneTextSWtoBuffer(SWITCH_1, "Exit");
            AddOneTextSWtoBuffer(SWITCH_2, "Screen");
            AddOneTextSWtoBuffer(SWITCH_3, "WiFi");
            AddOneTextSWtoBuffer(SWITCH_4, "OTA");
            AddOneTextSWtoBuffer(SWITCH_5, "OTA dev");


            float SoCfromlut = 0;
            for (int i = 0; i < 32; i++)
                if (OCVLut[i][1] < bsp_battery_voltage())
                    SoCfromlut = OCVLut[i][0] * 100;
            snprintf(SoC, 15, "%.0f", SoCfromlut);
            ESP_LOGE(TAG, "SoCfromlut %f", SoCfromlut);
            snprintf(voltage, 15, "%.2fV", bsp_battery_voltage());
            pax_center_text(gfx, BLACK, font1, fontsizeS, 32, 14, voltage);
            pax_center_text(gfx, BLACK, font1, fontsizeS, 31, 44, SoC);
            ESP_LOGE(TAG, "bsp_battery_charging %d", bsp_battery_charging());

            if (strcmp(voltage, oldvoltage) || strcmp(SoC, oldSoC) || (oldchargingstate != bsp_battery_charging())) {
                bsp_display_flush();
                strcpy(oldvoltage, voltage);
                strcpy(oldSoC, SoC);
                oldchargingstate = bsp_battery_charging();
            }
        }

        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: screen_lut_dial(application_event_queue, keyboard_event_queue); break;
                        case SWITCH_3: edit_wifi(application_event_queue, keyboard_event_queue); break;
                        case SWITCH_4: ota_update_wrapped(keyboard_event_queue, false); break;
                        case SWITCH_5: ota_update_wrapped(keyboard_event_queue, true); break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_lut_dial(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    if (log)
        ESP_LOGE(TAG, "Enter screen_home_entry");
    // update the keyboard event handler settings
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, false, false, false, false);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int          cursor    = 0;
    epaper_lut_t activeLut = lut_4s;

    while (1) {
        pax_buf_t* gfx = bsp_get_gfx_buffer();
        pax_insert_png_buf(gfx, lutdial_png_start, lutdial_png_end - lutdial_png_start, 0, 0, 0);
        AddOneTextSWtoBuffer(SWITCH_1, "Exit");
        DrawArrowHorizontal(SWITCH_5);

        switch (cursor) {
            case 0: activeLut = lut_1s; break;
            case 1: activeLut = lut_4s; break;
            case 2: activeLut = lut_8s; break;
            case 3: activeLut = lut_full; break;
            default: break;
        }

        bsp_apply_lut(activeLut);
        nvs_set_u8_wrapped("system", "lut", (uint8_t) activeLut);

        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        case SWITCH_L5:
                            if (cursor)
                                cursor--;
                            break;
                        case SWITCH_R5:
                            if (cursor)
                                cursor++;
                            break;
                    }


                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
