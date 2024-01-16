#pragma once

#include <stdbool.h>

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_eap_client.h"

bool wifi_connect_to_stored();
void wifi_disconnect_and_disable();
