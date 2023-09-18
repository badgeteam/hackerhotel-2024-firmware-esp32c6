/**
 * Copyright (c) 2021 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#include <sdkconfig.h>
#include <driver/i2c.h>
#include "include/managed_i2c.h"

esp_err_t i2c_init(int bus, int pin_sda, int pin_scl, int clk_speed, bool pullup_sda, bool pullup_scl) {
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = pin_sda,
        .scl_io_num = pin_scl,
        .master.clk_speed = clk_speed,
        .sda_pullup_en = pullup_sda ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .scl_pullup_en = pullup_scl ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
    };
    
    esp_err_t res = i2c_param_config(bus, &i2c_config);
    if (res != ESP_OK) return res;
    res = i2c_set_timeout(bus, 20000); // 250 us ( 20000 clock cycles @ APB freq = 80 MHz )
    if (res != ESP_OK) return res;
    return i2c_driver_install(bus, i2c_config.mode, 0, 0, 0);
}

esp_err_t i2c_read_bytes(int bus, uint8_t addr, uint8_t *value, size_t value_len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t res = i2c_master_start(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, ( addr << 1 ) | READ_BIT, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    if (value_len > 1) {
        res = i2c_master_read(cmd, value, value_len-1, ACK_VAL);
        if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    }
    res = i2c_master_read_byte(cmd, &value[value_len-1], NACK_VAL);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_stop(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }   

    res = i2c_master_cmd_begin(bus, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return res;
}

esp_err_t i2c_read_reg(int bus, uint8_t addr, uint8_t reg, uint8_t *value, size_t value_len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t res = i2c_master_start(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, ( addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_start(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, ( addr << 1 ) | READ_BIT, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    if (value_len > 1) {
        res = i2c_master_read(cmd, value, value_len-1, ACK_VAL);
        if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    }
    res = i2c_master_read_byte(cmd, &value[value_len-1], NACK_VAL);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_stop(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }

    res = i2c_master_cmd_begin(bus, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return res;
}

esp_err_t i2c_read_event(int bus, uint8_t addr, uint8_t *buf) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t res = i2c_master_start(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, ( addr << 1 ) | READ_BIT, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_read(cmd, buf, 2, ACK_VAL);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_read_byte(cmd, &buf[2], NACK_VAL);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_stop(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }

    res = i2c_master_cmd_begin(bus, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return res;
}

esp_err_t i2c_write_byte(int bus, uint8_t addr, uint8_t value) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t res = i2c_master_start(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, ( addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, value, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_stop(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }

    res = i2c_master_cmd_begin(bus, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return res;
}

esp_err_t i2c_write_reg(int bus, uint8_t addr, uint8_t reg, uint8_t value) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t res = i2c_master_start(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, ( addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, value, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_stop(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }

    res = i2c_master_cmd_begin(bus, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return res;
}

esp_err_t i2c_write_reg_n(int bus, uint8_t addr, uint8_t reg, const uint8_t *value, size_t value_len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t res = i2c_master_start(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, ( addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    for (size_t i = 0; i < value_len; i++) {
        res = i2c_master_write_byte(cmd, value[i], ACK_CHECK_EN);
        if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    }
    res = i2c_master_stop(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }

    res = i2c_master_cmd_begin(bus, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return res;
}

esp_err_t i2c_write_buffer(int bus, uint8_t addr, const uint8_t* buffer, uint16_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t res = i2c_master_start(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, ( addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    for (uint16_t i = 0; i < len; i++) {
        res = i2c_master_write_byte(cmd, buffer[i], ACK_CHECK_EN);
        if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    }
    res = i2c_master_stop(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }

    res = i2c_master_cmd_begin(bus, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return res;
}

esp_err_t i2c_write_buffer_reg(int bus, uint8_t addr, uint8_t reg, const uint8_t* buffer, uint16_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t res = i2c_master_start(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, ( addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    for (uint16_t i = 0; i < len; i++) {
        res = i2c_master_write_byte(cmd, buffer[i], ACK_CHECK_EN);
        if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    }
    res = i2c_master_stop(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }

    res = i2c_master_cmd_begin(bus, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return res;
}

esp_err_t i2c_write_reg32(int bus, uint8_t addr, uint8_t reg, uint32_t value) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t res = i2c_master_start(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, ( addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, (value)&0xFF, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, (value>>8)&0xFF, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, (value>>16)&0xFF, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_write_byte(cmd, (value>>24)&0xFF, ACK_CHECK_EN);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }
    res = i2c_master_stop(cmd);
    if (res != ESP_OK) { i2c_cmd_link_delete(cmd); return res; }

    res = i2c_master_cmd_begin(bus, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return res;
}
