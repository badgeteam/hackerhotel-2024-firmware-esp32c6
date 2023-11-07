#include <esp_err.h>
#include <esp_log.h>

#include "mma8452q.h"
#include "managed_i2c.h"

static char const *TAG = "mma8452q";

esp_err_t mma8452q_init(mma8452q_t *device) {
    esp_err_t res;
    uint8_t value;
    res = i2c_read_reg(device->i2c_bus, device->i2c_address, MMA8452Q_REG_WHO_AM_I, &value, sizeof(value));
    if (res != ESP_OK)
        return res;
    if (value != MMA8452Q_WHO_AM_I_VALUE) {
        ESP_LOGI(TAG, "Invalid who am I register value %02x", value);
        return ESP_FAIL;
    }
    return res;
}
