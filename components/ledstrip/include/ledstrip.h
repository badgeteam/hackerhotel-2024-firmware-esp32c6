#pragma once

#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "esp_err.h"

typedef struct _ledstrip {
    // Configuration
    gpio_num_t pin;

    // Initialized by init function
    rmt_channel_handle_t led_chan;
    rmt_encoder_handle_t led_encoder;
} ledstrip_t;

void      led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t* r, uint32_t* g, uint32_t* b);
esp_err_t ledstrip_init(ledstrip_t* device);
esp_err_t ledstrip_deinit(ledstrip_t* device);
esp_err_t ledstrip_send(ledstrip_t* device, const uint8_t* data, int length);
