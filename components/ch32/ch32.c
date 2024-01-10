/**
 * Copyright (c) 2024 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#include "ch32.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include "soc/gpio_struct.h"
#include "freertos/portmacro.h"
#include "riscv/rv_utils.h"
#include "driver/dedic_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static char const *TAG = "CH32";

// Initialization functions

void ch32_enable_slave_output() {
    uint32_t value = CH32_CFGR_KEY | CH32_CFGR_OUTEN; // Enable output from slave
    ch32_sdi_write(CH32_REG_DEBUG_SHDWCFGR, value); // First write the shadow register
    ch32_sdi_write(CH32_REG_DEBUG_CFGR, value); // Then the configuration register
    ch32_sdi_write(CH32_REG_DEBUG_CFGR, value); // Then the configuration register (again)
    ch32_sdi_read(CH32_REG_DEBUG_CPBR, &value); // Dummy read, for some reason the first read fails
}

bool ch32_check_link() {
    uint32_t value;
    ch32_sdi_read(CH32_REG_DEBUG_CPBR, &value);
    ESP_LOGW(TAG, "Link check raw result: %08" PRIx32, value);
    if (value == 0xFFFFFFFF || value == 0x00000000) {
        ESP_LOGE(TAG, "Link check failed: no chip");
        return false;
    }
    if (!((value >> 10) & 1)) {
        ESP_LOGE(TAG, "Link check failed: output not enabled");
        return false;
    }
    ESP_LOGI(TAG, "Link check succesful, version %" PRIu32, value >> 16);
    return true;
}

// Functions based on operation examples in debug spec document

bool ch32_halt_microprocessor() {
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request

    // Get the debug module status information, check rdata[9:8], if the value is 0b11,
    // it means the processor enters the halt state normally. Otherwise try again.
    uint8_t timeout = 5;
    while (1) {
        uint32_t value;
        ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
        if (((value >> 8) & 0b11) == 0b11) { // Check that processor has entered halted state
            break;
        }
        if (timeout == 0) {
            ESP_LOGE(TAG, "Failed to halt microprocessor, DMSTATUS=%" PRIx32, value);
            return false;
        }
        timeout--;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request
    ESP_LOGI(TAG, "Microprocessor halted");
    return true;
}

bool ch32_resume_microprocessor() {
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x40000001); // Initiate a resume request

    // Get the debug module status information, check rdata[17:16],
    // if the value is 0b11, it means the processor has recovered.
    uint8_t timeout = 5;
    while (1) {
        uint32_t value;
        ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
        if ((((value >> 10) & 0b11) == 0b11)) {
            break;
        }
        if (timeout == 0) {
            ESP_LOGE(TAG, "Failed to resume microprocessor, DMSTATUS=%" PRIx32, value);
            return false;
        }
        timeout --;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return true;
}

bool ch32_reset_microprocessor_and_run() {
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000003); // Initiate a core reset request

    uint8_t timeout = 5;
    while (1) {
        uint32_t value;
        ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
        if (((value >> 18) & 0b11) == 0b11) { // Check that processor has been reset
            break;
        }
        if (timeout == 0) {
            ESP_LOGE(TAG, "Failed to reset microprocessor");
            return false;
        }
        timeout--;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the core reset request
    vTaskDelay(pdMS_TO_TICKS(10));
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x10000001); // Clear the core reset status signal
    vTaskDelay(pdMS_TO_TICKS(10));

    // timeout = 100;
    // while (1) {
    //     uint32_t value;
    //     ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
    //     if (((value >> 18) & 0b11) == 0b00) { // Check that processor reset status has been cleared
    //         break;
    //     }
    //     if (timeout == 0) {
    //         ESP_LOGE(TAG, "Failed to resume microprocessor after reset, DMSTATUS=%08" PRIx32, value);
    //         return false;
    //     }
    //     timeout--;
    //     vTaskDelay(pdMS_TO_TICKS(10));
    // }
    
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the core reset status signal clear request
    vTaskDelay(pdMS_TO_TICKS(10));

    return true;
}

bool ch32_reset_microprocessor_and_halt() {
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000003); // Initiate a core reset request

    uint8_t timeout = 100;
    while (1) {
        uint32_t value;
        ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
        if (((value >> 18) & 0b11) == 0b11) { // Check that processor has been reset
            break;
        }
        if (timeout == 0) {
            ESP_LOGE(TAG, "Failed to reset microprocessor");
            return false;
        }
        timeout--;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Clear the core reset request
    vTaskDelay(pdMS_TO_TICKS(10));
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x90000001); // Clear the core reset status signal
    vTaskDelay(pdMS_TO_TICKS(10));
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Clear the core reset status signal clear request
    vTaskDelay(pdMS_TO_TICKS(10));


    timeout = 5;
    while (1) {
        uint32_t value;
        ch32_sdi_read(CH32_REG_DEBUG_DMSTATUS, &value);
        if (((value >> 18) & 0b11) == 0b00) { // Check that processor reset status has been cleared
            break;
        }
        if (timeout == 0) {
            ESP_LOGE(TAG, "Failed to resume microprocessor after reset, DMSTATUS=%08" PRIx32, value);
            return false;
        }
        timeout--;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request

    return true;
}

bool ch32_reset_debug_module() {
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000003); // Write command
    uint32_t value;
    ch32_sdi_read(CH32_REG_DEBUG_DMCONTROL, &value);
    if (value != 0x00000003) {
        ESP_LOGE(TAG, "Failed to reset debug module");
        return false;
    }
    ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000002); // Write debug module reset command
    ch32_sdi_read(CH32_REG_DEBUG_DMCONTROL, &value);
    if (((value >> 1) & 1) != 0) { // Check whether rdata[1] is 0b0, if it is the reset is succesful
        ESP_LOGE(TAG, "Failed to reset debug module");
        return false;
    }
    return true;
}

bool ch32_write_cpu_reg(uint16_t regno, uint32_t value) {
    uint32_t command =
        regno        // Register to access.
        | (1 << 16)  // Write access.
        | (1 << 17)  // Perform transfer.
        | (2 << 20)  // 32-bit register access.
        | (0 << 24); // Access register command.
    
    ch32_sdi_write(CH32_REG_DEBUG_DATA0, value);
    ch32_sdi_write(CH32_REG_DEBUG_COMMAND, command);
    return true;
}

bool ch32_read_cpu_reg(uint16_t regno, uint32_t *value_out) {
    uint32_t command =
        regno        // Register to access.
        | (0 << 16)  // Read access.
        | (1 << 17)  // Perform transfer.
        | (2 << 20)  // 32-bit register access.
        | (0 << 24); // Access register command.
    
    ch32_sdi_write(CH32_REG_DEBUG_COMMAND, command);
    ch32_sdi_read(CH32_REG_DEBUG_DATA0, value_out);
    return true;
}

bool ch32_run_debug_code(const void *code, size_t code_size) {
    if (code_size > 8*4) {
        ESP_LOGE(TAG, "Debug program is too long (%zd/%zd)", code_size, (size_t)8*4);
        return false;
    } else if (code_size & 1) {
        ESP_LOGE(TAG, "Debug program size must be a multiple of 2 (%zd)", code_size);
        return false;
    }
    
    // Copy into program buffer.
    uint32_t tmp[8] = {0};
    memcpy(tmp, code, code_size);
    for (size_t i = 0; i < 8; i++) {
        ch32_sdi_write(CH32_REG_DEBUG_PROGBUF0+i, tmp[i]);
    }
    
    // Run program buffer.
    uint32_t command =
          (0 << 17)  // Do not perform transfer.
        | (1 << 18)  // Run program buffer afterwards.
        | (2 << 20)  // 32-bit register access.
        | (0 << 24); // Access register command.
    ch32_sdi_write(CH32_REG_DEBUG_COMMAND, command);
    
    return true;
}

extern const uint8_t ch32_writemem_S_c[];
extern const size_t  ch32_writemem_S_c_len;
bool ch32_write_memory_word(uint32_t address, uint32_t value) {
    ch32_write_cpu_reg(CH32_REGS_GPR + 10, value);
    ch32_write_cpu_reg(CH32_REGS_GPR + 11, address);
    ch32_run_debug_code(ch32_writemem_S_c, ch32_writemem_S_c_len);
    return true;
}

extern const uint8_t ch32_readmem_S_c[];
extern const size_t  ch32_readmem_S_c_len;
bool ch32_read_memory_word(uint32_t address, uint32_t *value_out) {
    ch32_write_cpu_reg(CH32_REGS_GPR + 11, address);
    ch32_run_debug_code(ch32_readmem_S_c, ch32_readmem_S_c_len);
    ch32_read_cpu_reg(CH32_REGS_GPR + 10, value_out);
    return true;
}

// Wait for the FLASH chip to finish its current operation.
static void ch32_wait_flash() {
    uint32_t value = 0;
    ch32_read_memory_word(CH32_FLASH_STATR, &value);
    while(value & CH32_FLASH_STATR_BUSY) {
        vTaskDelay(0);
        ch32_read_memory_word(CH32_FLASH_STATR, &value);
    }
}

// Unlock the FLASH if not already unlocked.
bool ch32_unlock_flash() {
    uint32_t ctlr;
    ch32_read_memory_word(0x40022010, &ctlr);
    if (!(ctlr & 0x8080)) {
        // FLASH already unlocked.
        return true;
    }
    
    // Enter the unlock keys.
    ch32_write_memory_word(0x40022004, 0x45670123);
    ch32_write_memory_word(0x40022004, 0xCDEF89AB);
    ch32_write_memory_word(0x40022008, 0x45670123);
    ch32_write_memory_word(0x40022008, 0xCDEF89AB);
    ch32_write_memory_word(0x40022024, 0x45670123);
    ch32_write_memory_word(0x40022024, 0xCDEF89AB);
    
    // Check again if FLASH is unlocked.
    ch32_read_memory_word(0x40022010, &ctlr);
    return !(ctlr & 0x8080);
}

// Lock the FLASH if not already locked.
bool ch32_lock_flash();

// If unlocked: Erase the entire FLASH.
bool ch32_erase_flash() {
    // TODO.
    return false;
}

// If unlocked: Erase a 64-byte block of FLASH.
bool ch32_erase_flash_block(uint32_t addr) {
    if (addr % 64) return false;
    
    ch32_wait_flash();
    ch32_write_memory_word(CH32_FLASH_CTLR, CH32_FLASH_CTLR_FTER);
    ch32_write_memory_word(CH32_FLASH_ADDR, addr);
    ch32_write_memory_word(CH32_FLASH_CTLR, CH32_FLASH_CTLR_FTER | CH32_FLASH_CTLR_STRT);
    ch32_wait_flash();
    ch32_write_memory_word(CH32_FLASH_CTLR, 0);
    
    return true;
}

// If unlocked: Erase and write a range of FLASH memory.
bool ch32_write_flash(uint32_t addr, const void *_data, size_t data_len) {
    if (addr % 64) return false;
    const uint8_t *data = _data;
    
    for (size_t i = 0; i < data_len; i+=64) {
        if (!ch32_erase_flash_block(addr+i)) {
            ESP_LOGE(TAG, "Error: Failed to erase FLASH at %08"PRIx32, addr+i);
            return false;
        }
        if (!ch32_write_flash_block(addr+i, data+i)) {
            ESP_LOGE(TAG, "Error: Failed to write FLASH at %08"PRIx32, addr+i);
            return false;
        }
    }
    
    return true;
}

// If unlocked: Write a 64-byte block of FLASH.
bool ch32_write_flash_block(uint32_t addr, const void *data) {
    if (addr % 64) return false;
    
    ch32_wait_flash();
    ch32_write_memory_word(CH32_FLASH_CTLR, CH32_FLASH_CTLR_FTPG);
    ch32_write_memory_word(CH32_FLASH_CTLR, CH32_FLASH_CTLR_FTPG | CH32_FLASH_CTLR_BUFRST);
    ch32_wait_flash();
    
    uint32_t wdata[16];
    memcpy(wdata, data, sizeof(wdata));
    for (size_t i = 0; i < 16; i++) {
        ch32_write_memory_word(addr+i*4, wdata[i]);
        ch32_write_memory_word(CH32_FLASH_CTLR, CH32_FLASH_CTLR_FTPG | CH32_FLASH_CTLR_BUFLOAD);
        ch32_wait_flash();
    }
    
    ch32_write_memory_word(CH32_FLASH_ADDR, addr);
    ch32_write_memory_word(CH32_FLASH_CTLR, CH32_FLASH_CTLR_FTPG | CH32_FLASH_CTLR_STRT);
    ch32_wait_flash();
    ch32_write_memory_word(CH32_FLASH_CTLR, 0);
    
    uint32_t rdata[16];
    for (size_t i = 0; i < 16; i++) {
        ch32_read_memory_word(addr+i*4, &rdata[i]);
    }
    if (memcmp(wdata, rdata, sizeof(wdata))) {
        ESP_LOGE(TAG, "Write block mismatch at %08"PRIx32, addr);
        ESP_LOGE(TAG, "Write:");
        for (size_t i = 0; i < 16; i++) {
            ESP_LOGE(TAG, "%zx: %08"PRIx32, i, wdata[i]);
        }
        ESP_LOGE(TAG, "Read:");
        for (size_t i = 0; i < 16; i++) {
            ESP_LOGE(TAG, "%zx: %08"PRIx32, i, rdata[i]);
        }
        return false;
    }
    
    return true;
}

// If unlocked: Set the NRST mode on startup.
// If true: use as reset pit, false: use as GPIO pin.
bool ch32_set_nrst_mode(bool use_as_reset) {
    // User configuration address.
    const uint32_t addr = 0x1ffff800;
    
    // Read current value.
    uint32_t rdata[16];
    for (size_t i = 0; i < 16; i++) {
        ch32_read_memory_word(addr+i*4, &rdata[i]);
    }
    
    // Update the NRST mode.
    if (use_as_reset) {
        rdata[0] &= ~(0b11 << 27);
        rdata[0] |=  (0b01 << 27);
    } else {
        rdata[0] |=  (0b11 << 27);
    }
    
    // Write new value.
    ch32_erase_flash_block(addr);
    ch32_write_flash_block(addr, rdata);
    
    return true;
}
