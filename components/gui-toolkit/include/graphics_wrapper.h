#pragma once

#include "pax_gfx.h"
#include <stdint.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <sdkconfig.h>

void render_outline(
    float position_x, float position_y, float width, float height, pax_col_t border_color, pax_col_t background_color
);
void render_message(char* message);
