#pragma once

#include "esp_eap_client.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Simpler interpretation of WiFi signal strength.
typedef enum {
    WIFI_STRENGTH_VERY_BAD,
    WIFI_STRENGTH_BAD,
    WIFI_STRENGTH_GOOD,
    WIFI_STRENGTH_VERY_GOOD,
} wifi_strength_t;

// Thresholds for aforementioned signal strength definitions.
#define WIFI_THRESH_BAD       -80
#define WIFI_THRESH_GOOD      -70
#define WIFI_THRESH_VERY_GOOD -67

// Retry forever.
#define WIFI_INFINITE_RETRIES 255

esp_netif_ip_info_t* wifi_get_ip_info();

// First time initialisation of the WiFi stack.
// Initialises internal resources and ESP32 WiFi.
// Use this if nothing else initialises ESP32 WiFi.
void wifi_init();

// First time initialisation of the WiFi stack.
// Initialises internal resources only.
// Use this if ESP32 WiFi is already initialised.
void wifi_init_no_hardware();

// Connect to a traditional username/password WiFi network.
// Will wait for the connection to be established.
// Will try at most `aRetryMax` times, or forever if it's WIFI_INFINITE_RETRIES.
// Returns whether WiFi has successfully connected.
bool wifi_connect(const char* aSsid, const char* aPassword, wifi_auth_mode_t aAuthmode, uint8_t aRetryMax);

// Connect to a WPA2 enterprise WiFi network.
// Will wait for the connection to be established.
// Will try at most `aRetryMax` times, or forever if it's WIFI_INFINITE_RETRIES.
// Returns whether WiFi has successfully connected.
bool wifi_connect_ent(
    const char*               aSsid,
    const char*               aIdent,
    const char*               aAnonIdent,
    const char*               aPassword,
    esp_eap_ttls_phase2_types phase2,
    uint8_t                   aRetryMax
);

// Connect to a traditional username/password WiFi network.
// Will return right away.
// Will try at most `aRetryMax` times, or forever if it's WIFI_INFINITE_RETRIES.
void wifi_connect_async(const char* aSsid, const char* aPassword, wifi_auth_mode_t aAuthmode, uint8_t aRetryMax);

// Connect to a WPA2 enterprise WiFi network.
// Will return right away.
// Will try at most `aRetryMax` times, or forever if it's WIFI_INFINITE_RETRIES.
void wifi_connect_ent_async(
    const char*               aSsid,
    const char*               aIdent,
    const char*               aAnonIdent,
    const char*               aPassword,
    esp_eap_ttls_phase2_types phase2,
    uint8_t                   aRetryMax
);

// Disconnect from WiFi and do not attempt to reconnect.
void wifi_disconnect();

// Awaits WiFi to be connected for at most `max_delay_millis` milliseconds or forever if it's 0.
// Returns whether WiFi has successfully connected.
bool wifi_await(uint64_t max_delay_millis);

// Test whether WiFi is currently connected.
bool wifi_is_connected();

// Scan for WiFi networks.
// Updates the APs pointer if non-null.
// Returns the number of APs found.
size_t wifi_scan(wifi_ap_record_t** aps);

// Get the strength value for a given RSSI.
wifi_strength_t wifi_rssi_to_strength(int8_t rssi);
