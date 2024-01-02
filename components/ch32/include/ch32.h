/**
 * Copyright (c) 2023 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include <esp_err.h>

#define CH32_REG_DEBUG_DATA0        0x04 // Data register 0, can be used for temporary storage of data
#define CH32_REG_DEBUG_DATA1        0x05 // Data register 1, can be used for temporary storage of data
#define CH32_REG_DEBUG_DMCONTROL    0x10 // Debug module control register
#define CH32_REG_DEBUG_DMSTATUS     0x11 // Debug module status register
#define CH32_REG_DEBUG_HARTINFO     0x12 // Microprocessor status register
#define CH32_REG_DEBUG_ABSTRACTCS   0x16 // Abstract command status register
#define CH32_REG_DEBUG_COMMAND      0x17 // Astract command register
#define CH32_REG_DEBUG_ABSTRACTAUTO 0x18 // Abstract command auto-executtion
#define CH32_REG_DEBUG_PROGBUF0     0x20 // Instruction cache register 0
#define CH32_REG_DEBUG_PROGBUF1     0x21 // Instruction cache register 1
#define CH32_REG_DEBUG_PROGBUF2     0x22 // Instruction cache register 2
#define CH32_REG_DEBUG_PROGBUF3     0x23 // Instruction cache register 3
#define CH32_REG_DEBUG_PROGBUF4     0x24 // Instruction cache register 4
#define CH32_REG_DEBUG_PROGBUF5     0x25 // Instruction cache register 5
#define CH32_REG_DEBUG_PROGBUF6     0x26 // Instruction cache register 6
#define CH32_REG_DEBUG_PROGBUF7     0x27 // Instruction cache register 7
#define CH32_REG_DEBUG_HALTSUM0     0x40 // Halt status register
#define CH32_REG_DEBUG_CPBR         0x7C // Capability register
#define CH32_REG_DEBUG_CFGR         0x7D // Configuration register
#define CH32_REG_DEBUG_SHDWCFGR     0x7E // Shadow configuration register

#define CH32_CFGR_KEY 0x5aa50000
#define CH32_CFGR_OUTEN (1 << 10)

// Low level functions
esp_err_t ch32_init(uint8_t pin);
void ch32_sdi_read(uint8_t address, uint32_t* value);
void ch32_sdi_write(uint8_t address, uint32_t value);
void ch32_sdi_write_bypass(uint32_t value);
void ch32_sdi_reset();

// High level functions
void ch32_enable_slave_output();
bool ch32_check_link();
bool ch32_halt_microprocessor();
bool ch32_resume_microprocessor();
bool ch32_reset_microprocessor_and_run();
bool ch32_reset_microprocessor_and_halt();
bool ch32_reset_debug_module();
void ch32_programmer();

#ifdef __cplusplus
}
#endif  //__cplusplus
