#pragma once

#include "esp_err.h"

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);
esp_err_t ledstrip_init(gpio_num_t pin);
esp_err_t ledstrip_send(uint8_t* data, int length);