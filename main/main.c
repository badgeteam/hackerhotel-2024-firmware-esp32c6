#include "driver/gpio.h"
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
#include "pax_gfx.h"
#include "sdkconfig.h"
#include "sdmmc_cmd.h"
#include "driver/i2c.h"
#include <inttypes.h>
#include <stdio.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>
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

#include "pax_codecs.h"

#define AUTOMATIC_SLEEP 0

#ifndef AUTOMATIC_SLEEP
#define AUTOMATIC_SLEEP 1
#endif

#define I2C_BUS     0
#define I2C_SPEED   400000  // 400 kHz
#define I2C_TIMEOUT 250     // us

#define GPIO_I2C_SCL 7
#define GPIO_I2C_SDA 6

extern const uint8_t renze_png_start[] asm("_binary_renze_png_start");
extern const uint8_t renze_png_end[] asm("_binary_renze_png_end");

static char const *TAG = "main";

pax_buf_t gfx;
pax_col_t palette[] = {0xffffffff, 0xff000000, 0xffff0000}; // white, black, red

hink_t epaper = {
    .spi_bus               = SPI2_HOST,
    .pin_cs                = 8,
    .pin_dcx               = 5,
    .pin_reset             = 16,
    .pin_busy              = 10,
    .spi_speed             = 10000000,
    .spi_max_transfer_size = SOC_SPI_MAXIMUM_BUFFER_SIZE,
    .screen_width          = 128,
    .screen_height         = 296,
};

sdmmc_card_t *card = NULL;

static esp_err_t initialize_nvs(void) {
    esp_err_t res = nvs_flash_init();
    if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        res = nvs_flash_erase();
        if (res != ESP_OK)
            return res;
        res = nvs_flash_init();
    }
    return res;
}

static esp_err_t initialize_system() {
    esp_err_t res;

    // Non-volatile storage
    res = initialize_nvs();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing NVS failed");
        return res;
    }

    if (0) {
        hink_read_lut(19, 21, epaper.pin_cs, epaper.pin_dcx, epaper.pin_reset, epaper.pin_busy);
    }

    // I2C bus
    i2c_config_t i2c_config = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = GPIO_I2C_SDA,
        .scl_io_num       = GPIO_I2C_SCL,
        .master.clk_speed = I2C_SPEED,
        .sda_pullup_en    = false,
        .scl_pullup_en    = false,
        .clk_flags        = 0};

    res = i2c_param_config(I2C_BUS, &i2c_config);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Configuring I2C bus parameters failed");
        return res;
    }

    res = i2c_set_timeout(I2C_BUS, I2C_TIMEOUT * 80);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Configuring I2C bus timeout failed");
        // return res;
    }

    res = i2c_driver_install(I2C_BUS, i2c_config.mode, 0, 0, 0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing I2C bus failed");
        return res;
    }

    // SPI bus
    spi_bus_config_t busConfiguration = {0};
    busConfiguration.mosi_io_num      = 19;
    busConfiguration.miso_io_num      = 20;
    busConfiguration.sclk_io_num      = 21;
    busConfiguration.quadwp_io_num    = -1;
    busConfiguration.quadhd_io_num    = -1;
    busConfiguration.max_transfer_sz  = SOC_SPI_MAXIMUM_BUFFER_SIZE;

    res = spi_bus_initialize(SPI2_HOST, &busConfiguration, SPI_DMA_CH_AUTO);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing SPI bus failed");
        return res;
    }

    // Epaper display
    res = hink_init(&epaper);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing epaper display failed");
        return res;
    }

    // Graphics stack
    ESP_LOGI(TAG, "Creating graphics...");
    pax_buf_init(&gfx, NULL, epaper.screen_height, epaper.screen_width, PAX_BUF_2_PAL);
    pax_buf_set_orientation(&gfx, PAX_O_ROT_CCW);
    gfx.palette      = palette;
    gfx.palette_size = sizeof(palette) / sizeof(pax_col_t);

    // SD card
    sdmmc_host_t          host        = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs               = 18;
    slot_config.host_id               = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files              = 5,
        .allocation_unit_size   = 16 * 1024};

    res = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (res != ESP_OK) {
        ESP_LOGW(TAG, "Initializing SD card failed");
        card = NULL;
    }
    res = ESP_OK; // Continue starting without SD card

    return res;
}

uint8_t lut_normal_20deg[] = {
0x08, 0x99, 0x21, 0x44, 0x40, 0x01, 0x00, 0x10, 0x99, 0x21, 0xa0, 0x20, 0x20, 0x00, 0x88, 0x99, 0x21, 0x44, 0x2b, 0x2f, 0x00, 0x88, 0x99, 0x21, 0x44, 0x2b, 0x2f, 0x00, 0x00, 0x00, 0x12, 0x40, 0x00, 0x00, 0x00, 0x36, 0x30, 0x33, 0x00, 0x01, 0x02, 0x02, 0x02, 0x02, 0x13, 0x01, 0x16, 0x01, 0x16, 0x04, 0x02, 0x0b, 0x10, 0x00, 0x03, 0x06, 0x04, 0x02, 0x2b, 0x04, 0x14, 0x04, 0x23, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x3c, 0xc0, 0x2e, 0x30, 0x0a, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61
};

void app_main(void) {
    esp_err_t res = initialize_system();
    if (res != ESP_OK) {
        // Device init failed, stop.
        ESP_LOGE(TAG, "System init failed, bailing out.");
        return;
    }

    if (card) {
        sdmmc_card_print_info(stdout, card);
    } else {
        ESP_LOGI(TAG, "No SD card found");
    }

    // Clear screen
    pax_background(&gfx, 0);
    pax_draw_text(&gfx, 1, pax_font_marker, 18, 1, 0, "Hackerhotel 2024");
    pax_draw_text(&gfx, 1, pax_font_sky, 12, 1, 50, "Hardware bringup testing firmware");
    pax_draw_text(&gfx, 1, pax_font_sky, 12, 2, 65, "Hardware bringup testing firmware");
    hink_set_lut_ext(&epaper, lut_normal_20deg);
    hink_write(&epaper, gfx.buf, false);
}
