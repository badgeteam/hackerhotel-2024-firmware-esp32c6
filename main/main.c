#include <stdio.h>
#include <string.h>
#include <time.h>

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
#include "driver/i2c.h"
#include "epaper.h"
#include "libcsid.h"
#include "pax_gfx.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "managed_i2c.h"

#define I2C_BUS        0
#define I2C_SPEED      400000  // 400 kHz
#define I2C_TIMEOUT    250 // us

#define GPIO_I2C_SCL     7
#define GPIO_I2C_SDA     6

//static xSemaphoreHandle i2c_semaphore = NULL;

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

static char const *TAG = "main";

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
    for (unsigned int i = 0; i < NUM_SAMPLES; i++) {
        unsigned int sample_val = mono_samples_data[i];
        samples_data[i * 2 + 0] = (unsigned short) (((short)sample_val) * 1.0);
        samples_data[i * 2 + 1] = (unsigned short) (((short)sample_val) * 0.8);
    }

    int            pos  = 0;
    int            left = 2 * 2 * NUM_SAMPLES;
    unsigned char *ptr  = (unsigned char *)samples_data;

    while (left > 0) {
        size_t written = 0;
        i2s_write(I2S_NUM_0, (char const *)ptr, left, &written, 100 / portTICK_PERIOD_MS);
        pos  += written;
        ptr  += written;
        left -= written;
    }
}

#include "phantom.inc"

static void player_task(void *pvParameters) {
    while (1) {
        render_audio();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

pax_buf_t gfx;
pax_col_t palette[] = {0xffffffff, 0xffff0000, 0xff000000};

uint8_t lut[HINK_LUT_SIZE] = {1, 2, 3, 4};

uint8_t const lut_rom[HINK_LUT_SIZE] = {
    0x80, 0x66, 0x96, 0x51, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x10, 0x66, 0x96, 0x88, 0x20, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x8A, 0x66, 0x96, 0x51, 0x0B, 0x2F, 0x00, 0x00, 0x00, 0x00, 0x8A, 0x66, 0x96, 0x51, 0x0B, 0x2F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x55, 0x28, 0x25,
    0x00, 0x02, 0x03, 0x01, 0x02, 0x0C, 0x12, 0x01, 0x12, 0x01, 0x03, 0x05, 0x05, 0x02, 0x05, 0x02,
};

uint8_t const lut_alt[HINK_LUT_SIZE] = {
    0x80, 0x66, 0x96, 0x51, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x10, 0x66, 0x96, 0x88, 0x20, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x8a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x66, 0x96, 0x51, 0x0b, 0x2f,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2d, 0x55, 0x28, 0x25,
    0x00, 0x02, 0x03, 0x01, 0x02, 0x04, 0x24, 0x02, 0x24, 0x02, 0x11, 0x05, 0x05, 0x02, 0x05, 0x02};

uint8_t const zeroes[HINK_LUT_SIZE] = {0};

// Bin 2 hex printer.
void rawprinthex(uint64_t val, int digits) {
    char const hextab[] = "0123456789ABCDEF";
    for (; digits > 0; digits--) {
        putc(hextab[(val >> (digits * 4 - 4)) & 15], stdout);
    }
}

#define LOGK_HEXDUMP_COLS   16
#define LOGK_HEXDUMP_GROUPS 4
// Print a hexdump, override the address shown (usually for debug purposes).
static void hexdump_vaddr(char const *msg, void const *data, size_t size, size_t vaddr) {
    fputs(msg, stdout);
    putc('\r', stdout);
    putc('\n', stdout);

    uint8_t const *ptr = data;
    for (size_t y = 0; y * LOGK_HEXDUMP_COLS < size; y++) {
        rawprinthex((size_t)vaddr + y * LOGK_HEXDUMP_COLS, sizeof(size_t) * 2);
        putc(':', stdout);
        size_t x;
        for (x = 0; y * LOGK_HEXDUMP_COLS + x < size && x < LOGK_HEXDUMP_COLS; x++) {
            if ((x % LOGK_HEXDUMP_GROUPS) == 0) {
                putc(' ', stdout);
            }
            putc(' ', stdout);
            rawprinthex(ptr[y * LOGK_HEXDUMP_COLS + x], 2);
        }
        for (; x < LOGK_HEXDUMP_GROUPS; x++) {
            if ((x % LOGK_HEXDUMP_GROUPS) == 0) {
                putc(' ', stdout);
            }
            putc(' ', stdout);
            putc(' ', stdout);
            putc(' ', stdout);
        }
        putc(' ', stdout);
        putc(' ', stdout);
        for (x = 0; y * LOGK_HEXDUMP_COLS + x < size && x < LOGK_HEXDUMP_COLS; x++) {
            char c = (char)ptr[y * LOGK_HEXDUMP_COLS + x];
            if (c >= 0x20 && c <= 0x7e) {
                putc(c, stdout);
            } else {
                putc('.', stdout);
            }
        }
        putc('\r', stdout);
        putc('\n', stdout);
    }
    putc('\r', stdout);
    putc('\n', stdout);
}

static void hexdump(char const *msg, void const *data, size_t size) {
    hexdump_vaddr(msg, data, size, (size_t)data);
}

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
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
}

#define DRV2605_REG_STATUS 0x00       ///< Status register
#define DRV2605_REG_MODE 0x01         ///< Mode register
#define DRV2605_MODE_INTTRIG 0x00     ///< Internal trigger mode
#define DRV2605_MODE_EXTTRIGEDGE 0x01 ///< External edge trigger mode
#define DRV2605_MODE_EXTTRIGLVL 0x02  ///< External level trigger mode
#define DRV2605_MODE_PWMANALOG 0x03   ///< PWM/Analog input mode
#define DRV2605_MODE_AUDIOVIBE 0x04   ///< Audio-to-vibe mode
#define DRV2605_MODE_REALTIME 0x05    ///< Real-time playback (RTP) mode
#define DRV2605_MODE_DIAGNOS 0x06     ///< Diagnostics mode
#define DRV2605_MODE_AUTOCAL 0x07     ///< Auto calibration mode

#define DRV2605_REG_RTPIN 0x02    ///< Real-time playback input register
#define DRV2605_REG_LIBRARY 0x03  ///< Waveform library selection register
#define DRV2605_REG_WAVESEQ1 0x04 ///< Waveform sequence register 1
#define DRV2605_REG_WAVESEQ2 0x05 ///< Waveform sequence register 2
#define DRV2605_REG_WAVESEQ3 0x06 ///< Waveform sequence register 3
#define DRV2605_REG_WAVESEQ4 0x07 ///< Waveform sequence register 4
#define DRV2605_REG_WAVESEQ5 0x08 ///< Waveform sequence register 5
#define DRV2605_REG_WAVESEQ6 0x09 ///< Waveform sequence register 6
#define DRV2605_REG_WAVESEQ7 0x0A ///< Waveform sequence register 7
#define DRV2605_REG_WAVESEQ8 0x0B ///< Waveform sequence register 8

#define DRV2605_REG_GO 0x0C         ///< Go register
#define DRV2605_REG_OVERDRIVE 0x0D  ///< Overdrive time offset register
#define DRV2605_REG_SUSTAINPOS 0x0E ///< Sustain time offset, positive register
#define DRV2605_REG_SUSTAINNEG 0x0F ///< Sustain time offset, negative register
#define DRV2605_REG_BREAK 0x10      ///< Brake time offset register
#define DRV2605_REG_AUDIOCTRL 0x11  ///< Audio-to-vibe control register
#define DRV2605_REG_AUDIOLVL                                                   \
  0x12 ///< Audio-to-vibe minimum input level register
#define DRV2605_REG_AUDIOMAX                                                   \
  0x13 ///< Audio-to-vibe maximum input level register
#define DRV2605_REG_AUDIOOUTMIN                                                \
  0x14 ///< Audio-to-vibe minimum output drive register
#define DRV2605_REG_AUDIOOUTMAX                                                \
  0x15                          ///< Audio-to-vibe maximum output drive register
#define DRV2605_REG_RATEDV 0x16 ///< Rated voltage register
#define DRV2605_REG_CLAMPV 0x17 ///< Overdrive clamp voltage register
#define DRV2605_REG_AUTOCALCOMP                                                \
  0x18 ///< Auto-calibration compensation result register
#define DRV2605_REG_AUTOCALEMP                                                 \
  0x19                            ///< Auto-calibration back-EMF result register
#define DRV2605_REG_FEEDBACK 0x1A ///< Feedback control register
#define DRV2605_REG_CONTROL1 0x1B ///< Control1 Register
#define DRV2605_REG_CONTROL2 0x1C ///< Control2 Register
#define DRV2605_REG_CONTROL3 0x1D ///< Control3 Register
#define DRV2605_REG_CONTROL4 0x1E ///< Control4 Register
#define DRV2605_REG_VBAT 0x21     ///< Vbat voltage-monitor register
#define DRV2605_REG_LRARESON 0x22 ///< LRA resonance-period register

void app_main(void) {
    esp_err_t res;

    ESP_LOGI(TAG, "Starting NVS...");
    initialize_nvs();

    // I2C bus
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_I2C_SDA,
        .scl_io_num = GPIO_I2C_SCL,
        .master.clk_speed = I2C_SPEED,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .clk_flags = 0
    };

    res = i2c_param_config(I2C_BUS, &i2c_config);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Configuring I2C bus parameters failed");
    }

    res = i2c_set_timeout(I2C_BUS, I2C_TIMEOUT * 80);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Configuring I2C bus timeout failed");
    }

    res = i2c_driver_install(I2C_BUS, i2c_config.mode, 0, 0, 0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing system I2C bus failed");
    }

    //i2c_semaphore = xSemaphoreCreateBinary();
    //xSemaphoreGive( i2c_semaphore );

    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_MODE, DRV2605_MODE_AUDIOVIBE);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_RTPIN, 0x00);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_LIBRARY, 0x01);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_WAVESEQ1, 0x01);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_WAVESEQ2, 0x00);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_OVERDRIVE, 0x00);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_SUSTAINPOS, 0x00);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_SUSTAINNEG, 0x00);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_BREAK, 0x00);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_AUDIOMAX, 0x32);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_CONTROL1, 0x20);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_CONTROL3, 0xA2);
    res = i2c_write_reg(I2C_BUS, 0x5A, DRV2605_REG_FEEDBACK, 0x7F);


    ESP_LOGI(TAG, "Starting I2S...");
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
        .bits_per_chan        = I2S_BITS_PER_SAMPLE_16BIT};

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config =
        {.mck_io_num = -1, .bck_io_num = 23, .ws_io_num = 17, .data_out_num = 22, .data_in_num = I2S_PIN_NO_CHANGE};

    i2s_set_pin(I2S_NUM_0, &pin_config);

    gpio_config_t pin_amp_enable_cfg = {
        .pin_bit_mask = 1 << 1,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_set_level(1, true);

    gpio_config(&pin_amp_enable_cfg);

    libcsid_init(16000, SIDMODEL_6581);
    libcsid_load((unsigned char *)&phantom_of_the_opera_sid, phantom_of_the_opera_sid_len, 0);

    printf("SID Title: %s\n", libcsid_gettitle());
    printf("SID Author: %s\n", libcsid_getauthor());
    printf("SID Info: %s\n", libcsid_getinfo());

    TaskHandle_t player_handle = NULL;
    // xTaskCreate(player_task, "Audio player task", 2048, NULL, tskIDLE_PRIORITY+2, &player_handle);

    ESP_LOGI(TAG, "Starting SPI...");
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
        return;
    }

    ESP_LOGI(TAG, "Creating graphics...");
    pax_buf_init(&gfx, NULL, 152, 152, PAX_BUF_2_PAL);
    gfx.palette      = palette;
    gfx.palette_size = sizeof(palette) / sizeof(pax_col_t);
    // pax_buf_set_orientation(&gfx, PAX_O_FLIP_H);

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

    ESP_LOGI(TAG, "Starting epaper...");

    HINK epaper = {
        .spi_bus               = SPI2_HOST,
        .pin_cs                = 8,
        .pin_dcx               = 5,
        .pin_reset             = 16,
        .pin_busy              = 10,
        .spi_speed             = 10000000,
        .spi_max_transfer_size = SOC_SPI_MAXIMUM_BUFFER_SIZE,
    };

    hink_init(&epaper);


    // hink_set_lut(&epaper, lut_alt);
    hink_write(&epaper, gfx.buf);


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


    ESP_LOGI(TAG, "Starting SD card...");

    sdmmc_host_t          host        = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs               = 18;
    slot_config.host_id               = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files              = 5,
        .allocation_unit_size   = 16 * 1024};

    sdmmc_card_t *card = NULL;

    res = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing SD card failed");
        // return;
    } else {
        sdmmc_card_print_info(stdout, card);
    }

    ESP_LOGI(TAG, "Writing to epaper...");
    // while (1) {
    // ESP_LOGI(TAG, "Hello world!");
    hink_write(&epaper, gfx.buf);
    // sdmmc_card_print_info(stdout, card);
    //    vTaskDelay(10000 / portTICK_PERIOD_MS);
    //}

    while (1) {
        test_time();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
