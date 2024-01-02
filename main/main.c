#include "driver/gpio.h"
#include "driver/i2c.h"
#include "epaper.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hextools.h"
#include "managed_i2c.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "sdkconfig.h"
#include "sdmmc_cmd.h"
#include "ch32.h"

#include <inttypes.h>
#include <stdio.h>

#include <esp_vfs_fat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <sdkconfig.h>
#include <sdmmc_cmd.h>
#include <string.h>
#include <time.h>

#define AUTOMATIC_SLEEP 0

#ifndef AUTOMATIC_SLEEP
#define AUTOMATIC_SLEEP 1
#endif

#define I2C_BUS 0
#define I2C_SPEED 400000 // 400 kHz
#define I2C_TIMEOUT 250  // us

#define GPIO_I2C_SCL 7
#define GPIO_I2C_SDA 6

#define GPIO_RELAY 15

#define A 1
#define B 2
#define D 4
#define E 8
#define F 16
#define G 32
#define L 64
#define I 128
#define K 256
#define H 512

#define M 1024
#define N 2048
#define O 4096
#define P 8192
#define R 16384
#define S 32768
#define T 65536
#define V 131072
#define W 262144
#define Y 524288

#define SWidle 0
#define SWleft 1
#define SWbutton 2
#define SWright 4

#define SW1 0
#define SW2 1
#define SW3 2
#define SW4 3
#define SW5 4

extern const uint8_t renze_png_start[] asm("_binary_renze_png_start");
extern const uint8_t renze_png_end[] asm("_binary_renze_png_end");

static char const *TAG = "main";

pax_buf_t gfx;
pax_col_t palette[] = {0xffffffff, 0xff000000, 0xffff0000}; // white, black, red

hink_t epaper = {
    .spi_bus = SPI2_HOST,
    .pin_cs = 8,
    .pin_dcx = 5,
    .pin_reset = 16,
    .pin_busy = 10,
    .spi_speed = 10000000,
    .spi_max_transfer_size = SOC_SPI_MAXIMUM_BUFFER_SIZE,
    .screen_width = 128,
    .screen_height = 296,
};

sdmmc_card_t *card = NULL;

static esp_err_t initialize_nvs(void)
{
    esp_err_t res = nvs_flash_init();
    if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        res = nvs_flash_erase();
        if (res != ESP_OK)
            return res;
        res = nvs_flash_init();
    }
    return res;
}

static esp_err_t initialize_system()
{
    esp_err_t res;

    // Non-volatile storage
    res = initialize_nvs();
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Initializing NVS failed");
        return res;
    }

    // Relay

    gpio_config_t cfg = {
        .pin_bit_mask = BIT64(GPIO_RELAY),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    res = gpio_set_level(GPIO_RELAY, false);
    if (res != ESP_OK)
    {
        return res;
    }

    // Epaper (LUT)

    if (0)
    {
        hink_read_lut(19, 21, epaper.pin_cs, epaper.pin_dcx, epaper.pin_reset, epaper.pin_busy);
    }

    // I2C bus
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_I2C_SDA,
        .scl_io_num = GPIO_I2C_SCL,
        .master.clk_speed = I2C_SPEED,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .clk_flags = 0};

    res = i2c_param_config(I2C_BUS, &i2c_config);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Configuring I2C bus parameters failed");
        return res;
    }

    res = i2c_set_timeout(I2C_BUS, I2C_TIMEOUT * 80);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Configuring I2C bus timeout failed");
        // return res;
    }

    res = i2c_driver_install(I2C_BUS, i2c_config.mode, 0, 0, 0);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Initializing I2C bus failed");
        return res;
    }

    // SPI bus
    spi_bus_config_t busConfiguration = {0};
    busConfiguration.mosi_io_num = 19;
    busConfiguration.miso_io_num = 20;
    busConfiguration.sclk_io_num = 21;
    busConfiguration.quadwp_io_num = -1;
    busConfiguration.quadhd_io_num = -1;
    busConfiguration.max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE;

    res = spi_bus_initialize(SPI2_HOST, &busConfiguration, SPI_DMA_CH_AUTO);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Initializing SPI bus failed");
        return res;
    }

    // Epaper display
    res = hink_init(&epaper);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Initializing epaper display failed");
        return res;
    }

    // Graphics stack
    ESP_LOGI(TAG, "Creating graphics...");
    pax_buf_init(&gfx, NULL, epaper.screen_width, epaper.screen_height, PAX_BUF_2_PAL);
    pax_buf_set_orientation(&gfx, PAX_O_ROT_CCW);
    gfx.palette = palette;
    gfx.palette_size = sizeof(palette) / sizeof(pax_col_t);

    // SD card
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = 18;
    slot_config.host_id = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    res = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (res != ESP_OK)
    {
        ESP_LOGW(TAG, "Initializing SD card failed");
        card = NULL;
    }
    res = ESP_OK; // Continue starting without SD card

    return res;
}

uint8_t lut_normal_20deg[] = {
    0x08,
    0x99,
    0x21,
    0x44,
    0x40,
    0x01,
    0x00,
    0x10,
    0x99,
    0x21,
    0xa0,
    0x20,
    0x20,
    0x00,
    0x88,
    0x99,
    0x21,
    0x44,
    0x2b,
    0x2f,
    0x00,
    0x88,
    0x99,
    0x21,
    0x44,
    0x2b,
    0x2f,
    0x00,
    0x00,
    0x00,
    0x12,
    0x40,
    0x00,
    0x00,
    0x00,
    0x36,
    0x30,
    0x33,
    0x00,
    0x01,
    0x02,
    0x02,
    0x02,
    0x02,
    0x13,
    0x01,
    0x16,
    0x01,
    0x16,
    0x04,
    0x02,
    0x0b,
    0x10,
    0x00,
    0x03,
    0x06,
    0x04,
    0x02,
    0x2b,
    0x04,
    0x14,
    0x04,
    0x23,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x13,
    0x3c,
    0xc0,
    0x2e,
    0x30,
    0x0a,
};

// All LUTs

// 0  = -20 degrees
// 1  = -10 degrees
// 2  =   0 degrees
// 3  =   2 degrees
// 4  =   4 degrees
// 5  =   6 degrees
// 6  =   8 degrees
// 7  =  10 degrees
// 8  =  12 degrees
// 9  =  14 degrees
// 10 =  16 degrees
// 11 =  18 degrees
// 12 =  20 degrees
// 13 =  22 degrees
// 14 =  24 degrees
// 15 =  26 degrees
// 16 =  28 degrees
// 17 =  30 degrees
// 18 =  40 degrees
// 19 =  50 degrees
// 20 =  60 degrees
// 21 =  70 degrees
// 22 =  80 degrees
// 23 =  90 degrees

uint8_t luts[24][128] = {
    {
        0x20,
        0x99,
        0x21,
        0x44,
        0x00,
        0x00,
        0x04,
        0x04,
        0x99,
        0x21,
        0x8a,
        0x00,
        0x00,
        0x20,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0x00,
        0x00,
        0x12,
        0x84,
        0x00,
        0x00,
        0x00,
        0x37,
        0x39,
        0x3b,
        0x38,
        0x01,
        0x03,
        0x03,
        0x03,
        0x03,
        0x13,
        0x01,
        0x23,
        0x01,
        0x23,
        0x04,
        0x0c,
        0x00,
        0x04,
        0x0e,
        0x03,
        0x02,
        0x03,
        0x02,
        0x04,
        0x16,
        0x0f,
        0x07,
        0x05,
        0x2b,
        0x06,
        0x2d,
        0x05,
        0x03,
        0x29,
        0x05,
        0x17,
        0x46,
        0x27,
        0x36,
        0x30,
        0x0a,
    },
    {
        0x20,
        0x99,
        0x21,
        0x44,
        0x00,
        0x00,
        0x04,
        0x04,
        0x99,
        0x21,
        0x8a,
        0x00,
        0x00,
        0x20,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0x00,
        0x00,
        0x12,
        0x84,
        0x00,
        0x00,
        0x00,
        0x37,
        0x39,
        0x3b,
        0x38,
        0x01,
        0x03,
        0x03,
        0x03,
        0x03,
        0x13,
        0x01,
        0x23,
        0x01,
        0x23,
        0x04,
        0x0c,
        0x00,
        0x04,
        0x0e,
        0x03,
        0x02,
        0x03,
        0x02,
        0x04,
        0x16,
        0x0f,
        0x07,
        0x05,
        0x2b,
        0x06,
        0x2d,
        0x05,
        0x03,
        0x29,
        0x05,
        0x17,
        0x46,
        0x27,
        0x36,
        0x30,
        0x0a,
    },
    {
        0x20,
        0x99,
        0x21,
        0x44,
        0x00,
        0x00,
        0x04,
        0x04,
        0x99,
        0x21,
        0x8a,
        0x00,
        0x00,
        0x20,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0x00,
        0x00,
        0x12,
        0x84,
        0x00,
        0x00,
        0x00,
        0x37,
        0x39,
        0x3b,
        0x38,
        0x01,
        0x03,
        0x03,
        0x03,
        0x03,
        0x13,
        0x01,
        0x23,
        0x01,
        0x23,
        0x04,
        0x0c,
        0x00,
        0x04,
        0x0e,
        0x03,
        0x02,
        0x03,
        0x02,
        0x04,
        0x16,
        0x0f,
        0x07,
        0x05,
        0x2b,
        0x06,
        0x2d,
        0x05,
        0x03,
        0x29,
        0x05,
        0x17,
        0x46,
        0x27,
        0x36,
        0x30,
        0x0a,
    },
    {
        0x20,
        0x99,
        0x21,
        0x44,
        0x00,
        0x00,
        0x04,
        0x04,
        0x99,
        0x21,
        0x8a,
        0x00,
        0x00,
        0x20,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0x00,
        0x00,
        0x12,
        0x84,
        0x00,
        0x00,
        0x00,
        0x37,
        0x39,
        0x3b,
        0x38,
        0x01,
        0x03,
        0x03,
        0x03,
        0x03,
        0x13,
        0x01,
        0x23,
        0x01,
        0x23,
        0x04,
        0x0c,
        0x00,
        0x04,
        0x0e,
        0x03,
        0x02,
        0x03,
        0x02,
        0x04,
        0x16,
        0x0f,
        0x07,
        0x05,
        0x2b,
        0x06,
        0x2d,
        0x05,
        0x03,
        0x29,
        0x05,
        0x17,
        0x46,
        0x27,
        0x36,
        0x30,
        0x0a,
    },
    {
        0x20,
        0x99,
        0x21,
        0x44,
        0x00,
        0x00,
        0x04,
        0x04,
        0x99,
        0x21,
        0x8a,
        0x00,
        0x00,
        0x20,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0x00,
        0x00,
        0x12,
        0x84,
        0x00,
        0x00,
        0x00,
        0x37,
        0x31,
        0x33,
        0x3f,
        0x01,
        0x03,
        0x03,
        0x03,
        0x03,
        0x13,
        0x01,
        0x20,
        0x01,
        0x20,
        0x04,
        0x0a,
        0x00,
        0x03,
        0x0c,
        0x03,
        0x02,
        0x04,
        0x02,
        0x04,
        0x0f,
        0x0f,
        0x07,
        0x05,
        0x2a,
        0x06,
        0x2d,
        0x05,
        0x03,
        0x29,
        0x05,
        0x15,
        0x41,
        0x25,
        0x32,
        0x30,
        0x0a,
    },
    {
        0x20,
        0x99,
        0x21,
        0x44,
        0x00,
        0x00,
        0x04,
        0x04,
        0x99,
        0x21,
        0x8a,
        0x00,
        0x00,
        0x20,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0x00,
        0x00,
        0x12,
        0x84,
        0x00,
        0x00,
        0x00,
        0x37,
        0x31,
        0x33,
        0x3f,
        0x01,
        0x03,
        0x03,
        0x03,
        0x03,
        0x13,
        0x01,
        0x20,
        0x01,
        0x20,
        0x04,
        0x0a,
        0x00,
        0x03,
        0x0c,
        0x03,
        0x02,
        0x04,
        0x02,
        0x04,
        0x0f,
        0x0f,
        0x07,
        0x05,
        0x2a,
        0x06,
        0x2d,
        0x05,
        0x03,
        0x29,
        0x05,
        0x15,
        0x41,
        0x25,
        0x32,
        0x30,
        0x0a,
    },
    {
        0x20,
        0x99,
        0x21,
        0x44,
        0x00,
        0x00,
        0x04,
        0x04,
        0x99,
        0x21,
        0xa8,
        0x00,
        0x00,
        0x20,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0x00,
        0x00,
        0x12,
        0x48,
        0x00,
        0x00,
        0x00,
        0x28,
        0x31,
        0x33,
        0x2e,
        0x01,
        0x03,
        0x03,
        0x03,
        0x03,
        0x0f,
        0x01,
        0x1e,
        0x01,
        0x1e,
        0x04,
        0x03,
        0x0c,
        0x0a,
        0x00,
        0x03,
        0x02,
        0x04,
        0x02,
        0x04,
        0x0b,
        0x0f,
        0x06,
        0x04,
        0x28,
        0x05,
        0x2d,
        0x05,
        0x03,
        0x29,
        0x05,
        0x15,
        0x41,
        0xce,
        0x32,
        0x30,
        0x0a,
    },
    {
        0x20,
        0x99,
        0x21,
        0x44,
        0x00,
        0x00,
        0x04,
        0x04,
        0x99,
        0x21,
        0xa8,
        0x00,
        0x00,
        0x20,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0xaa,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0x00,
        0x00,
        0x12,
        0x48,
        0x00,
        0x00,
        0x00,
        0x28,
        0x31,
        0x33,
        0x2e,
        0x01,
        0x03,
        0x03,
        0x03,
        0x03,
        0x0f,
        0x01,
        0x1e,
        0x01,
        0x1e,
        0x04,
        0x03,
        0x0c,
        0x0a,
        0x00,
        0x03,
        0x02,
        0x04,
        0x02,
        0x04,
        0x0b,
        0x0f,
        0x06,
        0x04,
        0x28,
        0x05,
        0x2d,
        0x05,
        0x03,
        0x29,
        0x05,
        0x15,
        0x41,
        0xce,
        0x32,
        0x30,
        0x0a,
    },
    {
        0x20,
        0x99,
        0x21,
        0x44,
        0x10,
        0x00,
        0x04,
        0x04,
        0x99,
        0x21,
        0x8a,
        0x00,
        0x00,
        0x20,
        0xa2,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0xa2,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0x00,
        0x00,
        0x12,
        0x84,
        0x00,
        0x00,
        0x00,
        0x22,
        0x2a,
        0x28,
        0x22,
        0x01,
        0x03,
        0x03,
        0x03,
        0x03,
        0x0e,
        0x01,
        0x19,
        0x01,
        0x19,
        0x04,
        0x07,
        0x00,
        0x03,
        0x0a,
        0x03,
        0x02,
        0x04,
        0x03,
        0x04,
        0x05,
        0x0f,
        0x06,
        0x05,
        0x23,
        0x04,
        0x23,
        0x04,
        0x01,
        0x27,
        0x03,
        0x15,
        0x41,
        0xc6,
        0x32,
        0x30,
        0x0a,
    },
    {
        0x20,
        0x99,
        0x21,
        0x44,
        0x10,
        0x00,
        0x04,
        0x04,
        0x99,
        0x21,
        0x8a,
        0x00,
        0x00,
        0x20,
        0xa2,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0xa2,
        0x99,
        0x21,
        0x44,
        0x99,
        0x2f,
        0x2f,
        0x00,
        0x00,
        0x12,
        0x84,
        0x00,
        0x00,
        0x00,
        0x22,
        0x2a,
        0x28,
        0x22,
        0x01,
        0x03,
        0x03,
        0x03,
        0x03,
        0x0e,
        0x01,
        0x19,
        0x01,
        0x19,
        0x04,
        0x07,
        0x00,
        0x03,
        0x0a,
        0x03,
        0x02,
        0x04,
        0x03,
        0x04,
        0x05,
        0x0f,
        0x06,
        0x05,
        0x23,
        0x04,
        0x23,
        0x04,
        0x01,
        0x27,
        0x03,
        0x15,
        0x41,
        0xc6,
        0x32,
        0x30,
        0x0a,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x01,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x20,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x36,
        0x30,
        0x33,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x16,
        0x01,
        0x16,
        0x04,
        0x02,
        0x0b,
        0x10,
        0x00,
        0x03,
        0x06,
        0x04,
        0x02,
        0x2b,
        0x04,
        0x14,
        0x04,
        0x23,
        0x02,
        0x03,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x13,
        0x3c,
        0xc0,
        0x2e,
        0x30,
        0x0a,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x01,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x20,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x36,
        0x30,
        0x33,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x16,
        0x01,
        0x16,
        0x04,
        0x02,
        0x0b,
        0x10,
        0x00,
        0x03,
        0x06,
        0x04,
        0x02,
        0x2b,
        0x04,
        0x14,
        0x04,
        0x23,
        0x02,
        0x03,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x13,
        0x3c,
        0xc0,
        0x2e,
        0x30,
        0x0a,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x01,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x20,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x36,
        0x30,
        0x33,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x16,
        0x01,
        0x16,
        0x04,
        0x02,
        0x0b,
        0x10,
        0x00,
        0x03,
        0x06,
        0x04,
        0x02,
        0x2b,
        0x04,
        0x14,
        0x04,
        0x23,
        0x02,
        0x03,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x13,
        0x3c,
        0xc0,
        0x2e,
        0x30,
        0x0a,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x04,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x20,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x24,
        0x2a,
        0x24,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x10,
        0x01,
        0x10,
        0x04,
        0x02,
        0x08,
        0x0a,
        0x00,
        0x03,
        0x07,
        0x05,
        0x01,
        0x23,
        0x03,
        0x0a,
        0x04,
        0x01,
        0x21,
        0x03,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x11,
        0x37,
        0xb9,
        0x2a,
        0x30,
        0x0a,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x04,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x20,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x24,
        0x2a,
        0x24,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x10,
        0x01,
        0x10,
        0x04,
        0x02,
        0x08,
        0x0a,
        0x00,
        0x03,
        0x07,
        0x05,
        0x01,
        0x23,
        0x03,
        0x0a,
        0x04,
        0x01,
        0x21,
        0x03,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x11,
        0x37,
        0xb9,
        0x2a,
        0x30,
        0x0a,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x04,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x80,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x13,
        0x2e,
        0x2e,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x14,
        0x01,
        0x14,
        0x04,
        0x02,
        0x0a,
        0x0c,
        0x00,
        0x03,
        0x07,
        0x06,
        0x02,
        0x24,
        0x04,
        0x02,
        0x04,
        0x03,
        0x1f,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x11,
        0x37,
        0xb4,
        0x2a,
        0x2c,
        0x08,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x04,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x80,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x13,
        0x2e,
        0x2e,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x14,
        0x01,
        0x14,
        0x04,
        0x02,
        0x0a,
        0x0c,
        0x00,
        0x03,
        0x07,
        0x06,
        0x02,
        0x24,
        0x04,
        0x02,
        0x04,
        0x03,
        0x1f,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x11,
        0x37,
        0xb4,
        0x2a,
        0x2c,
        0x08,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x04,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x80,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x13,
        0x2e,
        0x2e,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x14,
        0x01,
        0x14,
        0x04,
        0x02,
        0x0a,
        0x0c,
        0x00,
        0x03,
        0x07,
        0x06,
        0x02,
        0x24,
        0x04,
        0x02,
        0x04,
        0x03,
        0x1f,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x11,
        0x37,
        0xb4,
        0x2a,
        0x2c,
        0x08,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x04,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x80,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x13,
        0x2e,
        0x2e,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x14,
        0x01,
        0x14,
        0x04,
        0x02,
        0x0a,
        0x0c,
        0x00,
        0x03,
        0x07,
        0x06,
        0x02,
        0x24,
        0x04,
        0x02,
        0x04,
        0x03,
        0x1f,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x11,
        0x37,
        0xb4,
        0x2a,
        0x2c,
        0x08,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x04,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x80,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x13,
        0x2e,
        0x2e,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x14,
        0x01,
        0x14,
        0x04,
        0x02,
        0x0a,
        0x0c,
        0x00,
        0x03,
        0x07,
        0x06,
        0x02,
        0x24,
        0x04,
        0x02,
        0x04,
        0x03,
        0x1f,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x11,
        0x37,
        0xb4,
        0x2a,
        0x2c,
        0x08,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x04,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x80,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x13,
        0x2e,
        0x2e,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x14,
        0x01,
        0x14,
        0x04,
        0x02,
        0x0a,
        0x0c,
        0x00,
        0x03,
        0x07,
        0x06,
        0x02,
        0x24,
        0x04,
        0x02,
        0x04,
        0x03,
        0x1f,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x11,
        0x37,
        0xb4,
        0x2a,
        0x2c,
        0x08,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x04,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x80,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x13,
        0x2e,
        0x2e,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x14,
        0x01,
        0x14,
        0x04,
        0x02,
        0x0a,
        0x0c,
        0x00,
        0x03,
        0x07,
        0x06,
        0x02,
        0x24,
        0x04,
        0x02,
        0x04,
        0x03,
        0x1f,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x11,
        0x37,
        0xb4,
        0x2a,
        0x2c,
        0x08,
    },
    {
        0x08,
        0x99,
        0x21,
        0x44,
        0x40,
        0x04,
        0x00,
        0x10,
        0x99,
        0x21,
        0xa0,
        0x20,
        0x80,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x88,
        0x99,
        0x21,
        0x44,
        0x2b,
        0x2f,
        0x00,
        0x00,
        0x00,
        0x12,
        0x40,
        0x00,
        0x00,
        0x00,
        0x13,
        0x2e,
        0x2e,
        0x00,
        0x01,
        0x02,
        0x02,
        0x02,
        0x02,
        0x13,
        0x01,
        0x14,
        0x01,
        0x14,
        0x04,
        0x02,
        0x0a,
        0x0c,
        0x00,
        0x03,
        0x07,
        0x06,
        0x02,
        0x24,
        0x04,
        0x02,
        0x04,
        0x03,
        0x1f,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x11,
        0x37,
        0xb4,
        0x2a,
        0x2c,
        0x08,
    },
    {
        0x80,
        0x10,
        0x10,
        0x00,
        0x00,
        0x00,
        0x00,
        0x40,
        0x80,
        0x80,
        0x00,
        0x00,
        0x00,
        0x00,
        0x80,
        0x90,
        0x0b,
        0xc0,
        0x00,
        0x00,
        0x00,
        0x80,
        0x90,
        0x0b,
        0xc0,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x0c,
        0x0c,
        0x00,
        0x00,
        0x02,
        0x05,
        0x05,
        0x05,
        0x1e,
        0x0c,
        0x05,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x15,
        0x41,
        0xa8,
        0x32,
        0x24,
        0x04,
    }};

uint8_t lut_test[] = {
    0x08, // 0
    0x99, // 1
    0x21, // 2
    0x44, // 3
    0x40, // 4
    0x01, // 5
    0x00, // 6
    0x10, // 7
    0x99, // 8
    0x21, // 9
    0xa0, // 10
    0x20, // 11
    0x20, // 12
    0x00, // 13
    0x88, // 14
    0x99, // 15
    0x21, // 16
    0x44, // 17
    0x2b, // 18
    0x2f, // 19
    0x00, // 20
    0x88, // 21
    0x99, // 22
    0x21, // 23
    0x44, // 24
    0x2b, // 25
    0x2f, // 26
    0x00, // 27
    0x00, // 28
    0x00, // 29
    0x12, // 30
    0x40, // 31
    0x00, // 32
    0x00, // 33
    0x00, // 34

    //  TPxA  TPxB  TPxC  TPxD  RPx
    0x36, 0x30, 0x33, 0x00, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x13,
    0x01, 0x16, 0x01, 0x16, 0x04,
    0x02, 0x0b, 0x10, 0x00, 0x03,
    0x06, 0x04, 0x02, 0x2b, 0x04,
    0x14, 0x04, 0x23, 0x02, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00,

    0x13, // VGH
    0x3c, // VSH1
    0xc0, // VSH2
    0x2e, // VSL
    0x30, // Frame 1
    0x0a, // Frame 2
};

unsigned long millis() {
    return (unsigned long) (esp_timer_get_time() / 1000ULL);
}

uint32_t led = 1;
uint8_t buttons[5];
int delaySM = 1;
int delaySMflag = 0;
int statemachine = 0;

void frame1(void)
{
    pax_background(&gfx, 0);
    pax_draw_text(&gfx, 1, pax_font_marker, 18, 60, 0, "Welcome to Hackerhotel");
    hink_write(&epaper, gfx.buf);
}

void testaa(void)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (delaySMflag == 1)
        {
            vTaskDelay(pdMS_TO_TICKS(delaySM));
            delaySMflag = 0;
        }
    }
}

void app_main(void)
{
    esp_err_t res = initialize_system();
    if (res != ESP_OK)
    {
        // Device init failed, stop.
        ESP_LOGE(TAG, "System init failed, bailing out.");
        return;
    }

    if (card)
    {
        sdmmc_card_print_info(stdout, card);
    }
    else
    {
        ESP_LOGI(TAG, "No SD card found");
    }

    uint32_t xxx = 0xFFFFFFFF;
    res = i2c_write_reg_n(I2C_BUS, 0x42, 0, (uint8_t *)&xxx, sizeof(uint32_t));
    if (res != ESP_OK) {
        printf("FAILED %d\n", res);
    }

    ch32_init(17);
    ch32_sdi_reset();
    ch32_enable_slave_output();
    bool ch32res = ch32_check_link();
    if (!ch32res) {
        printf("Failed to initialize debug interface of CH32V003\n");
        return;
    }
    ch32res = ch32_reset_microprocessor_and_run();
    if (!ch32res) {
        printf("CH32V003 reset failed\n");
        return;
    }

    lut7_t* lut = (lut7_t*) lut_test;

    lut->groups[0].repeat = 0;
    lut->groups[1].repeat = 0;
    lut->groups[2].repeat = 0;
    lut->groups[3].repeat = 0;
    lut->groups[4].repeat = 0;
    // lut->groups[5].repeat = 0;
    // lut->groups[6].repeat = 0;

    lut->groups[0].tp[0] = 0;
    lut->groups[0].tp[1] = 0;
    lut->groups[0].tp[2] = 0;
    lut->groups[0].tp[3] = 0;

    lut->groups[1].tp[0] = 0;
    lut->groups[1].tp[1] = 0;
    lut->groups[1].tp[2] = 0;
    lut->groups[1].tp[3] = 0;

    lut->groups[2].tp[0] = 0;
    lut->groups[2].tp[1] = 0;
    lut->groups[2].tp[2] = 0;
    lut->groups[2].tp[3] = 0;

    lut->groups[5].tp[0] = 0;
    lut->groups[5].tp[1] = 0;
    lut->groups[5].tp[2] = 0;
    lut->groups[5].tp[3] = 0;

    // lut->groups[6].tp[0] = 0x14;
    // lut->groups[6].tp[1] = 0x04;
    // lut->groups[6].tp[2] = 0x23;
    // lut->groups[6].tp[3] = 0;

    hink_set_lut(&epaper, (uint8_t *)lut);
    // hink_write(&epaper, gfx.buf);

    bool state = false;
    unsigned long previous_millis = 0;

    xTaskCreate(testaa, "testaa", 1024, NULL, 1, NULL);

    while (1)
    {
        // bool busy = hink_busy(&epaper);
        //  printf("Epaper: %s\n", busy ? "busy" : "idle");

        res = i2c_read_reg(I2C_BUS, 0x42, 5, buttons, 5);
        if (res == ESP_OK)
        {
            if (buttons[0] | buttons[1] | buttons[2] | buttons[3] | buttons[4])
            {
                printf("BUTTONS %x %x %x %x %x\n", buttons[0], buttons[1], buttons[2], buttons[3], buttons[4]);
            }
        }

        // gpio_set_level(
        //     GPIO_RELAY,
        //     buttons[0] | buttons[1] | buttons[2] | buttons[3] | buttons[4]); // Turn on relay if one of the buttons is activated

        // if (!busy)
        // {

        //     unsigned long current_millis = millis();
        //     unsigned long time_passed = current_millis - previous_millis;
        //     if (previous_millis)
        //     {
        //         printf("Updating took %lu ms\n", time_passed);
        //     }
        //     previous_millis = millis();
        //     state = !state;
        //     testaa();

        // if (state)
        // {
        //     pax_background(&gfx, 0);
        //     pax_draw_text(&gfx, 1, pax_font_marker, 18, 1, 0, "Hackerhotel 2024");
        //     pax_draw_text(&gfx, 1, pax_font_sky_mono, 12, 1, 50, "This text is black");
        //     pax_draw_text(&gfx, 2, pax_font_sky_mono, 12, 1, 65, "This text is red");
        //     pax_draw_rect(&gfx, 0, 0, 95, 100, 32);  // White
        //     pax_draw_rect(&gfx, 1, 100, 95, 98, 32); // Black
        //     pax_draw_rect(&gfx, 2, 198, 95, 98, 32); // Red
        // }
        // else
        // {
        //     pax_background(&gfx, 0);
        //     pax_draw_text(&gfx, 1, pax_font_marker, 18, 1, 0, "Blah blah blah blah blah blah blah blah");
        //     pax_draw_text(&gfx, 2, pax_font_sky_mono, 12, 2, 50, "This text is red");
        //     pax_draw_text(&gfx, 1, pax_font_sky_mono, 12, 1, 65, "This text is black");
        //     pax_draw_rect(&gfx, 2, 0, 95, 100, 32);  // Red
        //     pax_draw_rect(&gfx, 0, 100, 95, 98, 32); // White
        //     pax_draw_rect(&gfx, 1, 198, 95, 98, 32); // Black
        // }
        //
        // hink_write(&epaper, gfx.buf);
        // }

        res = i2c_write_reg_n(I2C_BUS, 0x42, 0, (uint8_t *)&led, sizeof(uint32_t));
        if (res != ESP_OK)
        {
            printf("FAILED %d\n", res);
        }

        // vTaskDelay(pdMS_TO_TICKS(500));
        // led++;

        // pax_background(&gfx, 0);
        // pax_draw_text(&gfx, 1, pax_font_marker, 18, 60, 0, "Welcome to Hackerhotel");
        // hink_write(&epaper, gfx.buf);

        // vTaskDelay(pdMS_TO_TICKS(2000));
        // pax_background(&gfx, 0);
        // pax_draw_text(&gfx, 1, pax_font_marker, 18, 60, 0, "Frame2");
        // hink_write(&epaper, gfx.buf);

        // vTaskDelay(pdMS_TO_TICKS(2000));
        led = 0;
        if (buttons[SW1] == SWleft)
            led = led | A | B | E | H;
        if (buttons[SW1] == SWright)
            led = led | M | R | V | Y;
        if (buttons[SW2] == SWleft)
            led = led | M | I | F | D;
        if (buttons[SW2] == SWright)
            led = led | H | N | S | W;
        if (buttons[SW3] == SWleft)
            led = led | R | N | K | G;
        if (buttons[SW3] == SWright)
            led = led | E | I | O | T;
        if (buttons[SW4] == SWleft)
            led = led | V | S | O | L;
        if (buttons[SW4] == SWright)
            led = led | B | F | K | P;
        if (buttons[SW5] == SWleft)
            led = led | Y | W | T | P;
        if (buttons[SW5] == SWright)
            led = led | A | D | G | L;
        if (buttons[SW5] == SWright && buttons[SW3] == SWleft)
        {
            led = G;
            //                                          X    Y
            pax_draw_text(&gfx, 1, pax_font_marker, 30, 100, 50, "G");
            gpio_set_level(GPIO_RELAY, 1);
            hink_write(&epaper, gfx.buf);
        }

        else if (buttons[SW1] == SWright && buttons[SW4] == SWleft)
        {
            led = V;
            //                                          X    Y
            pax_draw_text(&gfx, 1, pax_font_marker, 30, 125, 50, "U");
            gpio_set_level(GPIO_RELAY, 1);
            hink_write(&epaper, gfx.buf);
        }

        else if (buttons[SW1] == SWright && buttons[SW3] == SWleft)
        {
            led = R;
            //                                          X    Y
            pax_draw_text(&gfx, 1, pax_font_marker, 30, 150, 50, "R");
            gpio_set_level(GPIO_RELAY, 1);
            hink_write(&epaper, gfx.buf);
        }

        else if (buttons[SW1] == SWright && buttons[SW5] == SWleft)
        {
            led = V;
            //                                          X    Y
            pax_draw_text(&gfx, 1, pax_font_marker, 30, 175, 50, "U");
            gpio_set_level(GPIO_RELAY, 1);
            hink_write(&epaper, gfx.buf);
        }

        else
            gpio_set_level(GPIO_RELAY, 0);

        if (buttons[SW5] == SWbutton)
        {
            delaySMflag = 0;
            statemachine = 3;
        }

        if (delaySMflag == 0)
        {
            switch (statemachine)
            {
            case 0:
                pax_background(&gfx, 0);
                pax_draw_text(&gfx, 1, pax_font_marker, 20, 20, 55, "Welcome to Hackerhotel");
                hink_write(&epaper, gfx.buf);
                delaySM = 5000;
                delaySMflag = 1;
                statemachine = 1;
                break;
            case 1:
                pax_background(&gfx, 0);
                pax_draw_text(&gfx, 1, pax_font_marker, 20, 40, 50, "Please, dear guest,");
                pax_draw_text(&gfx, 1, pax_font_marker, 20, 50, 70, "enter your name");
                hink_write(&epaper, gfx.buf);
                delaySM = 5000;
                delaySMflag = 1;
                statemachine = 2;
                break;
            case 2:
                pax_background(&gfx, 0);
                hink_write(&epaper, gfx.buf);
                delaySMflag = 2;
                break;
            case 3:
                pax_background(&gfx, 0);
                pax_draw_text(&gfx, 1, pax_font_marker, 20, 40, 40, "Wonderful! please");
                pax_draw_text(&gfx, 1, pax_font_marker, 20, 50, 60, "follow us to...");
                pax_draw_text(&gfx, 1, pax_font_marker, 20, 50, 80, "[end of tutorial]");
                hink_write(&epaper, gfx.buf);
                delaySMflag = 2;
                break;
            }
        }
    }
}
