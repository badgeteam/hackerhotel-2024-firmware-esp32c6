#include "driver/i2c.h"
#include "driver/i2s.h"
#include "drv2605.h"
#include "epaper.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
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

#include <stdio.h>

#include <driver/i2s.h>
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

#define I2C_BUS     0
#define I2C_SPEED   400000 // 400 kHz
#define I2C_TIMEOUT 250    // us

#define GPIO_I2C_SCL 7
#define GPIO_I2C_SDA 6

static char const *TAG = "main";

pax_buf_t gfx;
pax_col_t palette[] = {0xffffffff, 0xffff0000, 0xff000000};

HINK epaper = {
    .spi_bus               = SPI2_HOST,
    .pin_cs                = 8,
    .pin_dcx               = 5,
    .pin_reset             = 16,
    .pin_busy              = 10,
    .spi_speed             = 10000000,
    .spi_max_transfer_size = SOC_SPI_MAXIMUM_BUFFER_SIZE,
};

drv2605_t drv2605_device = {
    .i2c_bus     = I2C_BUS,
    .i2c_address = DRV2605_ADDR,
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

    // I2C bus
    i2c_config_t i2c_config = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = GPIO_I2C_SDA,
        .scl_io_num       = GPIO_I2C_SCL,
        .master.clk_speed = I2C_SPEED,
        .sda_pullup_en    = false,
        .scl_pullup_en    = false,
        .clk_flags        = 0
    };

    res = i2c_param_config(I2C_BUS, &i2c_config);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Configuring I2C bus parameters failed");
        return res;
    }

    res = i2c_set_timeout(I2C_BUS, I2C_TIMEOUT * 80);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Configuring I2C bus timeout failed");
        return res;
    }

    res = i2c_driver_install(I2C_BUS, i2c_config.mode, 0, 0, 0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing I2C bus failed");
        return res;
    }

    // I2S audio
    i2s_config_t i2s_config = {
        .mode                 = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate          = 16000,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_LSB,
        .dma_buf_count        = 32,
        .dma_buf_len          = 64,
        .intr_alloc_flags     = 0,
        .use_apll             = false,
        .bits_per_chan        = I2S_BITS_PER_SAMPLE_16BIT
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config =
        {.mck_io_num = -1, .bck_io_num = 23, .ws_io_num = 17, .data_out_num = 22, .data_in_num = I2S_PIN_NO_CHANGE};
    i2s_set_pin(I2S_NUM_0, &pin_config);

    // GPIO for controlling power to the audio amplifier
    gpio_config_t pin_amp_enable_cfg = {
        .pin_bit_mask = 1 << 1,
        .mode         = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en   = false,
        .pull_down_en = false,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_set_level(1, true);
    gpio_config(&pin_amp_enable_cfg);

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

    // DRV2605 vibration motor driver
    res = drv2605_init(&drv2605_device);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing DRV2605 failed");
        return res;
    }

    // Graphics stack
    ESP_LOGI(TAG, "Creating graphics...");
    pax_buf_init(&gfx, NULL, 152, 152, PAX_BUF_2_PAL);
    gfx.palette      = palette;
    gfx.palette_size = sizeof(palette) / sizeof(pax_col_t);

    // SD card
    sdmmc_host_t          host        = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs               = 18;
    slot_config.host_id               = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config =
        {.format_if_mount_failed = false, .max_files = 5, .allocation_unit_size = 16 * 1024};

    res = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing SD card failed");
        card = NULL;
        return res;
    }

    return res;
}

uint8_t lut[HINK_LUT_SIZE] = {1, 2, 3, 4};

uint8_t const lut_rom[HINK_LUT_SIZE] = {
    0x80, 0x66, 0x96, 0x51, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x10, 0x66, 0x96, 0x88, 0x20, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x8A, 0x66, 0x96, 0x51, 0x0B, 0x2F, 0x00, 0x00, 0x00, 0x00, 0x8A, 0x66, 0x96, 0x51, 0x0B, 0x2F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x55, 0x28, 0x25,
    0x00, 0x02, 0x03, 0x01, 0x02, 0x0C, 0x12, 0x01, 0x12, 0x01, 0x03, 0x05, 0x05, 0x02, 0x05, 0x02,
};

uint8_t const lut_alt[HINK_LUT_SIZE] = {0x80, 0x66, 0x96, 0x51, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x10, 0x66,
                                        0x96, 0x88, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x66, 0x96, 0x51, 0x0b, 0x2f,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a, 0x40, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x2d, 0x55, 0x28, 0x25, 0x00, 0x02, 0x03, 0x01, 0x02, 0x04,
                                        0x24, 0x02, 0x24, 0x02, 0x11, 0x05, 0x05, 0x02, 0x05, 0x02};

uint8_t const zeroes[HINK_LUT_SIZE] = {0};

static void bitbang_tx(int tx_pin, int clk_pin, uint8_t byte) {
    gpio_set_direction(tx_pin, GPIO_MODE_OUTPUT);
    for (int i = 0; i < 8; i++) {
        gpio_set_level(tx_pin, byte & 0x80);
        byte <<= 1;
        esp_rom_delay_us(1);
        gpio_set_level(clk_pin, 1);
        esp_rom_delay_us(1);
        gpio_set_level(clk_pin, 0);
    }
}

static uint8_t bitbang_rx(int rx_pin, int clk_pin) {
    gpio_set_direction(rx_pin, GPIO_MODE_INPUT);
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        esp_rom_delay_us(1);
        gpio_set_level(clk_pin, 1);
        byte <<= 1;
        byte  |= !!gpio_get_level(rx_pin);
        esp_rom_delay_us(1);
        gpio_set_level(clk_pin, 0);
    }
    return byte;
}
void test_time() {
    time_t    now;
    char      strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
}

void read_lut() {
    spi_bus_remove_device(epaper.spi_device);
    spi_bus_free(SPI2_HOST);

    gpio_set_direction(21, GPIO_MODE_OUTPUT);
    gpio_set_level(21, 0);
    gpio_set_direction(5, GPIO_MODE_OUTPUT);
    gpio_set_level(5, 0);
    gpio_set_direction(8, GPIO_MODE_OUTPUT);
    gpio_set_level(8, 1);
    vTaskDelay(1);
    gpio_set_level(8, 0);

    bitbang_tx(19, 21, 0x33);
    gpio_set_level(5, 1);
    for (int i = 0; i < HINK_LUT_SIZE; i++) {
        lut[i] = bitbang_rx(19, 21);
    }

    hexdump_vaddr("LUT:", lut, sizeof(lut), 0);
    return;
}

void app_main(void) {
    esp_err_t res = initialize_system();
    if (res != ESP_OK) {
        // Device init failed, stop.
        return;
    }

    if (card) {
        sdmmc_card_print_info(stdout, card);
    }

    pax_background(&gfx, 0);
    pax_set_pixel(&gfx, 1, 5, 5);
    pax_set_pixel(&gfx, 2, 5, 10);

    pax_draw_rect(&gfx, 1, 0, 0, 50, 20);
    pax_draw_text(&gfx, 0, pax_font_sky, 18, 1, 1, "Test");
    pax_draw_rect(&gfx, 2, 50, 0, 50, 20);
    pax_draw_text(&gfx, 0, pax_font_sky, 18, 51, 1, "Test");

    pax_draw_rect(&gfx, 0, 0, 20, 50, 20);
    pax_draw_text(&gfx, 1, pax_font_sky, 18, 1, 21, "Test");
    pax_draw_rect(&gfx, 2, 50, 20, 50, 20);
    pax_draw_text(&gfx, 1, pax_font_sky, 18, 51, 21, "Test");

    pax_draw_rect(&gfx, 0, 0, 40, 50, 20);
    pax_draw_text(&gfx, 2, pax_font_sky, 18, 1, 41, "Test");
    pax_draw_rect(&gfx, 1, 50, 40, 50, 20);
    pax_draw_text(&gfx, 2, pax_font_sky, 18, 51, 41, "Test");


    // hink_set_lut(&epaper, lut_alt);
    hink_write(&epaper, gfx.buf);

    while (1) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
