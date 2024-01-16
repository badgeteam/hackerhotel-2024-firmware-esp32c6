#pragma once

#include <stdbool.h>

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_eap_client.h"

// Camp WiFi settings.
#define WIFI_DEFAULT_SSID     "Hackerhotel"
#define WIFI_DEFAULT_USER     "badge"
#define WIFI_DEFAULT_IDENT    "badge"
#define WIFI_DEFAULT_PASSWORD "badge"
#define WIFI_DEFAULT_AUTH     WIFI_AUTH_WPA2_ENTERPRISE
#define WIFI_DEFAULT_PHASE2   ESP_EAP_TTLS_PHASE2_PAP

bool wifi_set_defaults();
bool wifi_check_configured();
