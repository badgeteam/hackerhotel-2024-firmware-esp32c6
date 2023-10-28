#include "drv2605.h"

#include "managed_i2c.h"

esp_err_t drv2605_init(drv2605_t *device) {
    esp_err_t res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_MODE, DRV2605_MODE_AUDIOVIBE);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_RTPIN, 0x00);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_LIBRARY, 0x01);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_WAVESEQ1, 0x01);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_WAVESEQ2, 0x00);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_OVERDRIVE, 0x00);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_SUSTAINPOS, 0x00);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_SUSTAINNEG, 0x00);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_BREAK, 0x00);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_AUDIOMAX, 0x32);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_CONTROL1, 0x20);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_CONTROL3, 0xA2);
    if (res != ESP_OK)
        return res;
    res = i2c_write_reg(device->i2c_bus, device->i2c_address, DRV2605_REG_FEEDBACK, 0x7F);
    return res;
}
