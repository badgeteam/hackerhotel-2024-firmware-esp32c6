#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "driver/i2s.h"
#include "epaper.h"
#include "libcsid.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

static const char* TAG = "main";

static void initialize_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

/*#define NUM_SAMPLES 300
unsigned short samples_data[2 * NUM_SAMPLES];

static void render_audio() {
    libcsid_render(samples_data, NUM_SAMPLES);

    int pos = 0;
    int left = 2 * NUM_SAMPLES;
    unsigned char *ptr = (unsigned char *)samples_data;

    while (left > 0) {
        size_t written = 0;
        i2s_write(I2S_NUM_0, (const char *)ptr, left, &written, 100 / portTICK_PERIOD_MS);
        pos += written;
        ptr += written;
        left -= written;
    }
}*/

#define NUM_SAMPLES 1200
unsigned short mono_samples_data[2 * NUM_SAMPLES];
unsigned short samples_data[2 * 2 * NUM_SAMPLES];

static void render_audio() {
    libcsid_render(mono_samples_data, NUM_SAMPLES);

    // Duplicate mono samples to create stereo buffer
    for(unsigned int i = 0; i < NUM_SAMPLES; i ++) {
        unsigned int sample_val = mono_samples_data[i];
        samples_data[i * 2 + 0] = (unsigned short) (((short)sample_val) * 1);
        samples_data[i * 2 + 1] = (unsigned short) (((short)sample_val) * 1);
    }

    int pos = 0;
    int left = 2 * 2 * NUM_SAMPLES;
    unsigned char *ptr = (unsigned char *)samples_data;

    while (left > 0) {
        size_t written = 0;
        i2s_write(I2S_NUM_0, (const char *)ptr, left, &written, 100 / portTICK_PERIOD_MS);
        pos += written;
        ptr += written;
        left -= written;
    }
}

#include "commando.inc"

static void player_task(void *pvParameters) {
    while (1) {
        render_audio();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting NVS...");
    initialize_nvs();

    ESP_LOGI(TAG, "Starting I2S...");
    i2s_config_t i2s_config = {.mode                 = I2S_MODE_MASTER | I2S_MODE_TX,
                               .sample_rate          = 16000,
                               .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
                               .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
                               .communication_format = I2S_COMM_FORMAT_I2S_LSB,
                               .dma_buf_count        = 32,
                               .dma_buf_len          = 64,
                               .intr_alloc_flags     = 0,
                               .use_apll             = false,
                               .bits_per_chan        = I2S_BITS_PER_SAMPLE_16BIT};

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config = {.mck_io_num = -1, .bck_io_num = 23, .ws_io_num = 17, .data_out_num = 22, .data_in_num = I2S_PIN_NO_CHANGE};

    i2s_set_pin(I2S_NUM_0, &pin_config);

    libcsid_init(16000, SIDMODEL_6581);
    libcsid_load((unsigned char *)&music_Commando_sid, music_Commando_sid_len, 0);

    printf("SID Title: %s\n", libcsid_gettitle());
    printf("SID Author: %s\n", libcsid_getauthor());
    printf("SID Info: %s\n", libcsid_getinfo());

    TaskHandle_t player_handle = NULL;
    xTaskCreate(player_task, "Audio player task", 2048, NULL, tskIDLE_PRIORITY+2, &player_handle);

    ESP_LOGI(TAG, "Starting SPI...");
    spi_bus_config_t busConfiguration = {0};
    busConfiguration.mosi_io_num      = 19;
    busConfiguration.miso_io_num      = 20;
    busConfiguration.sclk_io_num      = 21;
    busConfiguration.quadwp_io_num    = -1;
    busConfiguration.quadhd_io_num    = -1;
    busConfiguration.max_transfer_sz  = SOC_SPI_MAXIMUM_BUFFER_SIZE;

    esp_err_t res = spi_bus_initialize(SPI2_HOST, &busConfiguration, SPI_DMA_CH_AUTO);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing SPI bus failed");
        return;
    }

    ESP_LOGI(TAG, "Starting epaper...");

    HINK epaper = {
        .spi_bus = SPI2_HOST,
        .pin_cs = 8,
        .pin_dcx = 5,
        .pin_reset = 16,
        .pin_busy = 10,
        .spi_speed = 10000000,
        .spi_max_transfer_size = SOC_SPI_MAXIMUM_BUFFER_SIZE,
    };

    hink_init(&epaper);

    ESP_LOGI(TAG, "Starting SD card...");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = 18;
    slot_config.host_id = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card = NULL;

    res = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing SD card failed");
        //return;
    } else {
        sdmmc_card_print_info(stdout, card);
    }

    ESP_LOGI(TAG, "Writing to epaper...");
    //while (1) {
        //ESP_LOGI(TAG, "Hello world!");
        hink_write(&epaper, NULL);
        //sdmmc_card_print_info(stdout, card);
    //    vTaskDelay(10000 / portTICK_PERIOD_MS);
    //}
}
