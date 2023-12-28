/**
 * Copyright (c) 2023 Nicolai Electronics
 * Copyright (c) 2022 CNLohr
 *
 * SPDX-License-Identifier: MIT
 */

#include "ch32.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include <inttypes.h>
#include <stdio.h>
#include "soc/gpio_struct.h"
#include "freertos/portmacro.h"
#include "riscv/rv_utils.h"
#include "driver/dedic_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

void ch32_init_debug() {
    uint32_t value = CH32_CFGR_KEY | CH32_CFGR_OUTEN; // Enable output from slave
    ch32_sdi_write(CH32_REG_DEBUG_SHDWCFGR, value); // First write the shadow register
    ch32_sdi_write(CH32_REG_DEBUG_CFGR, value); // Then the configuration register
    ch32_sdi_write(CH32_REG_DEBUG_CFGR, value); // Then the configuration register (again)
    ch32_sdi_read(CH32_REG_DEBUG_CPBR, &value);
    printf("CPBR: %08lx\n", value);
}

void ch32_stop_cpu() IRAM_ATTR __attribute__((optimize("O2")));
void ch32_stop_cpu() {
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request
        vTaskDelay(pdMS_TO_TICKS(100));
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request bit
}

void ch32_reset_cpu() IRAM_ATTR __attribute__((optimize("O2")));
void ch32_reset_cpu() {
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request
        vTaskDelay(pdMS_TO_TICKS(100));
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request bit
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000003); // Initiate a core reset request
        vTaskDelay(pdMS_TO_TICKS(100));
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the core reset request bit
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x10000001); // Clear the core reset status signal
        vTaskDelay(pdMS_TO_TICKS(100));
}

void ch32_start_cpu() IRAM_ATTR __attribute__((optimize("O2")));
void ch32_start_cpu() {
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Make the debug module work properly
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x80000001); // Initiate a halt request
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x00000001); // Clear the halt request bit
        ch32_sdi_write(CH32_REG_DEBUG_DMCONTROL, 0x40000001); // Initiate a resume request
}

void ch32_read_dmstatus() IRAM_ATTR __attribute__((optimize("O2")));
void ch32_read_dmstatus() {
        uint32_t value = 0;
        ch32_sdi_read(CH32_REG_DEBUG_DMCONTROL, &value);
        printf("Dmstatus: %08lx\n", value);
}

#define FLASH_STATR_WRPRTERR       ((uint8_t)0x10) 
#define CR_PAGE_PG                 ((uint32_t)0x00010000)
#define CR_BUF_LOAD                ((uint32_t)0x00040000)
#define FLASH_CTLR_MER             ((uint16_t)0x0004)     /* Mass Erase */
#define CR_STRT_Set                ((uint32_t)0x00000040)
#define CR_PAGE_ER                 ((uint32_t)0x00020000)
#define CR_BUF_RST                 ((uint32_t)0x00080000)

void ch32_update_progbuf_regs() {
	ch32_sdi_write(CH32_REG_DEBUG_DATA0, 0xe00000f4 );   // DATA0's location in memory.
	ch32_sdi_write(CH32_REG_DEBUG_COMMAND, 0x0023100a ); // Copy data to x10
	ch32_sdi_write(CH32_REG_DEBUG_DATA0, 0xe00000f8 );   // DATA1's location in memory.
	ch32_sdi_write(CH32_REG_DEBUG_COMMAND, 0x0023100b ); // Copy data to x11
	ch32_sdi_write(CH32_REG_DEBUG_DATA0, 0x40022010 ); //FLASH->CTLR
	ch32_sdi_write(CH32_REG_DEBUG_COMMAND, 0x0023100c ); // Copy data to x12
	ch32_sdi_write(CH32_REG_DEBUG_DATA0, CR_PAGE_PG|CR_BUF_LOAD);
	ch32_sdi_write(CH32_REG_DEBUG_COMMAND, 0x0023100d ); // Copy data to x13
}

void ch32_write_word(uint32_t address, uint32_t value) {
    bool flash = false;
    if ((address & 0xff000000) == 0x08000000) {
        // Code FLASH (16KB)
        flash = true;
    } else if ((address & 0x1FFFF800) == 0x1FFFF000) {
        // System FLASH (BOOT_1920B)
        flash = true;
    }

    ch32_sdi_write(CH32_REG_DEBUG_ABSTRACTAUTO, 0x00000000); // Disable autoexec

    // Different address, so we don't need to re-write all the program regs.
	// c.lw x9,0(x11) // Get the address to write to. 
	// c.sw x8,0(x9)  // Write to the address.
    ch32_sdi_write(CH32_REG_DEBUG_PROGBUF0, 0xc0804184);
	// c.addi x9, 4
	// c.sw x9,0(x11)
    ch32_sdi_write(CH32_REG_DEBUG_PROGBUF1, 0xc1840491);

    ch32_update_progbuf_regs();

    // Not finished.
}