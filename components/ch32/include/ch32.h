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

#define CH32_REGS_CSR   0x0000 // Offsets for accessing CSRs.
#define CH32_REGS_GPR   0x1000 // Offsets for accessing general-purpose (x)registers.

#define CH32_CFGR_KEY 0x5aa50000
#define CH32_CFGR_OUTEN (1 << 10)

// The start of CH32 CODE FLASH region.
#define CH32_CODE_BEGIN     0x08000000
// the end of the CH32 CODE FLASH region.
#define CH32_CODE_END       0x08004000

// FLASH status register.
#define CH32_FLASH_STATR    0x4002200C
// FLASH configuration register.
#define CH32_FLASH_CTLR     0x40022010
// FLASH address register.
#define CH32_FLASH_ADDR     0x40022014

// FLASH is busy writing or erasing.
#define CH32_FLASH_STATR_BUSY   (1 << 0)
// FLASH is finished with the operation.
#define CH32_FLASH_STATR_EOP    (1 << 5)

// Perform standard programming operation.
#define CH32_FLASH_CTLR_PG      (1 << 0)
// Perform 1K sector erase.
#define CH32_FLASH_CTLR_PER     (1 << 1)
// Perform full FLASH erase.
#define CH32_FLASH_CTLR_MER     (1 << 2)
// Perform user-selected word program.
#define CH32_FLASH_CTLR_OBG     (1 << 4)
// Perform user-selected word erasure.
#define CH32_FLASH_CTLR_OBER    (1 << 5)
// Start an erase operation.
#define CH32_FLASH_CTLR_STRT    (1 << 6)
// Lock the FLASH.
#define CH32_FLASH_CTLR_LOCK    (1 << 7)
// Start a fast page programming operation (64 bytes).
#define CH32_FLASH_CTLR_FTPG    (1 << 16)
// Start a fast page erase operation (64 bytes).
#define CH32_FLASH_CTLR_FTER    (1 << 17)
// "Cache data unto BUF"
#define CH32_FLASH_CTLR_BUFLOAD (1 << 18)
// "BUF reset operation"
#define CH32_FLASH_CTLR_BUFRST  (1 << 19)


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
bool ch32_write_cpu_reg(uint16_t regno, uint32_t value);
bool ch32_read_cpu_reg(uint16_t regno, uint32_t *value_out);
bool ch32_run_debug_code(const void *code, size_t code_size);
// If halted, overwrite a0 and a1 in order to write memory.
bool ch32_write_memory_word(uint32_t address, uint32_t value);
// If halted, overwrite a0 and a1 in order to read memory.
bool ch32_read_memory_word(uint32_t address, uint32_t *value_out);
// Unlock the FLASH if not already unlocked.
bool ch32_unlock_flash();
// Lock the FLASH if not already locked.
bool ch32_lock_flash();
// If unlocked: Erase the entire FLASH.
bool ch32_erase_flash();
// If unlocked: Erase a 64-byte block of FLASH.
bool ch32_erase_flash_block(uint32_t addr);
// If unlocked: Write the FLASH.
bool ch32_write_flash(uint32_t addr, const void *data, size_t data_len);
// If unlocked: Write a 64-byte block of FLASH.
bool ch32_write_flash_block(uint32_t addr, const void *data);
// If unlocked: Set the NRST mode on startup.
// If true: use as reset pit, false: use as GPIO pin.
bool ch32_set_nrst_mode(bool use_as_reset);

#ifdef __cplusplus
}
#endif  //__cplusplus
