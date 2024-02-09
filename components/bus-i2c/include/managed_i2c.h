#pragma once

#include <esp_err.h>

#define WRITE_BIT     I2C_MASTER_WRITE /* I2C master write */
#define READ_BIT      I2C_MASTER_READ  /* I2C master read */
#define ACK_CHECK_EN  0x1              /* I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0              /* I2C master will not check ack from slave */
#define ACK_VAL       0x0              /* I2C ack value */
#define NACK_VAL      0x1              /* I2C nack value */

esp_err_t i2c_init(int bus, int pin_sda, int pin_scl, int clk_speed, bool pullup_sda, bool pullup_scl);
esp_err_t i2c_read_bytes(int bus, uint8_t addr, uint8_t* value, size_t value_len);
esp_err_t i2c_read_reg(int bus, uint8_t addr, uint8_t reg, uint8_t* value, size_t value_len);
esp_err_t i2c_read_event(int bus, uint8_t addr, uint8_t* buf);
esp_err_t i2c_write_byte(int bus, uint8_t addr, uint8_t value);
esp_err_t i2c_write_reg(int bus, uint8_t addr, uint8_t reg, uint8_t value);
esp_err_t i2c_write_reg_n(int bus, uint8_t addr, uint8_t reg, const uint8_t* value, size_t value_len);
esp_err_t i2c_write_buffer(int bus, uint8_t addr, const uint8_t* buffer, uint16_t len);
esp_err_t i2c_write_buffer_reg(int bus, uint8_t addr, uint8_t reg, const uint8_t* buffer, uint16_t len);
esp_err_t i2c_write_reg32(int bus, uint8_t addr, uint8_t reg, uint32_t value);
