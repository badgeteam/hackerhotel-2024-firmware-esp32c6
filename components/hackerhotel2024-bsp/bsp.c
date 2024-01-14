#include "bsp.h"

#include <string.h>

#include "ch32.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/sdmmc_defs.h"
#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "managed_i2c.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "pax_gfx.h"
#include "sdmmc_cmd.h"

static const char* TAG = "bsp";

static hink_t epaper = {
    .spi_bus               = SPI_BUS,
    .pin_cs                = GPIO_EPAPER_CS,
    .pin_dcx               = GPIO_EPAPER_DCX,
    .pin_reset             = GPIO_EPAPER_RESET,
    .pin_busy              = GPIO_EPAPER_BUSY,
    .spi_speed             = EPAPER_SPEED,
    .spi_max_transfer_size = SOC_SPI_MAXIMUM_BUFFER_SIZE,
    .screen_width          = EPAPER_WIDTH,
    .screen_height         = EPAPER_HEIGHT,
};

static sdmmc_card_t* card = NULL;

static uint16_t coprocessor_fw_version = 0;

static bool bsp_ready      = false;
static bool ch32v003_ready = false;
static bool epaper_ready   = false;

static pax_buf_t pax_buffer;
static pax_col_t palette[] = {0xffffffff, 0xff000000, 0xffff0000};  // white, black, red

static uint8_t lut_buffer[76] = {0};

extern const uint8_t ch32_firmware_start[] asm("_binary_ch32_firmware_bin_start");
extern const uint8_t ch32_firmware_end[] asm("_binary_ch32_firmware_bin_end");

static esp_err_t flash_coprocessor() {
    bool res;

    ch32_init(GPIO_CH32_SWD);
    ch32_sdi_reset();
    ch32_enable_slave_output();

    res = ch32_check_link();
    if (!res) {
        ESP_LOGE(TAG, "CH32V003 link check failed");
        return ESP_FAIL;
    }

    res = ch32_halt_microprocessor();
    if (!res) {
        ESP_LOGE(TAG, "CH32V003 failed to halt microcontroller");
        return ESP_FAIL;
    }

    ch32_unlock_flash();
    ch32_set_nrst_mode(false);  // Use NRST as GPIO

    res = ch32_write_flash(0x08000000, ch32_firmware_start, ch32_firmware_end - ch32_firmware_start);
    if (!res) {
        return ESP_FAIL;
    }

    res = ch32_reset_microprocessor_and_run();
    if (!res) {
        ESP_LOGE(TAG, "CH32V003 failed to reset and run");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t initialize_nvs() {
    esp_err_t res = nvs_flash_init();
    if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        res = nvs_flash_erase();
        if (res != ESP_OK) return res;
        res = nvs_flash_init();
    }
    return res;
}

static esp_err_t initialize_gpio() {
    // Interrupt service
    esp_err_t res = gpio_install_isr_service(0);
    if (res != ESP_OK) {
        return res;
    }

    // Relay
    gpio_config_t relay_cfg = {
        .pin_bit_mask = BIT64(GPIO_RELAY),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = false,
        .pull_down_en = false,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    res = gpio_config(&relay_cfg);
    if (res != ESP_OK) {
        return res;
    }

    res = gpio_set_level(GPIO_RELAY, false);
    if (res != ESP_OK) {
        return res;
    }

    // SAO
    gpio_config_t sao_cfg = {
        .pin_bit_mask = BIT64(GPIO_SAO_IO1) | BIT64(GPIO_SAO_IO2),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = false,
        .pull_down_en = false,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    res = gpio_config(&sao_cfg);
    if (res != ESP_OK) {
        return res;
    }

    return ESP_OK;
}

static esp_err_t initialize_epaper_lut() {
    if (0) {
        hink_read_lut(GPIO_SPI_MOSI, GPIO_SPI_CLK, epaper.pin_cs, epaper.pin_dcx, epaper.pin_reset, epaper.pin_busy);
    }

    const uint8_t temporary_lut_initializer[76] = {
        0x08, 0x99, 0x21, 0x44, 0x40, 0x01, 0x00, 0x10, 0x99, 0x21, 0xa0, 0x20, 0x20, 0x00, 0x88, 0x99, 0x21, 0x44, 0x2b,
        0x2f, 0x00, 0x88, 0x99, 0x21, 0x44, 0x2b, 0x2f, 0x00, 0x00, 0x00, 0x12, 0x40, 0x00, 0x00, 0x00, 0x36, 0x30, 0x33,
        0x00, 0x01, 0x02, 0x02, 0x02, 0x02, 0x13, 0x01, 0x16, 0x01, 0x16, 0x04, 0x02, 0x0b, 0x10, 0x00, 0x03, 0x06, 0x04,
        0x02, 0x2b, 0x04, 0x14, 0x04, 0x23, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x3c, 0xc0, 0x2e, 0x30, 0x0a,
    };
    memcpy(lut_buffer, temporary_lut_initializer, sizeof(lut_buffer));

    lut7_t* lut = (lut7_t*) lut_buffer;

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
    return ESP_OK;
}

static esp_err_t initialize_spi_bus() {
    spi_bus_config_t busConfiguration = {0};
    busConfiguration.mosi_io_num      = GPIO_SPI_MOSI;
    busConfiguration.miso_io_num      = GPIO_SPI_MISO;
    busConfiguration.sclk_io_num      = GPIO_SPI_CLK;
    busConfiguration.quadwp_io_num    = -1;
    busConfiguration.quadhd_io_num    = -1;
    busConfiguration.max_transfer_sz  = SOC_SPI_MAXIMUM_BUFFER_SIZE;
    return spi_bus_initialize(SPI2_HOST, &busConfiguration, SPI_DMA_CH_AUTO);
}

static esp_err_t initialize_epaper() { return hink_init(&epaper); }

static esp_err_t initialize_graphics() {
    pax_buf_init(&pax_buffer, NULL, epaper.screen_width, epaper.screen_height, PAX_BUF_2_PAL);
    pax_buf_set_orientation(&pax_buffer, PAX_O_ROT_CCW);
    pax_buffer.palette      = palette;
    pax_buffer.palette_size = sizeof(palette) / sizeof(pax_col_t);
    return ESP_OK;
}

static esp_err_t initialize_i2c_bus() {
    esp_err_t res;

    i2c_config_t i2c_config = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = GPIO_I2C_SDA,
        .scl_io_num       = GPIO_I2C_SCL,
        .master.clk_speed = I2C_SPEED,
        .sda_pullup_en    = false,
        .scl_pullup_en    = false,
        .clk_flags        = 0,
    };

    res = i2c_param_config(I2C_BUS, &i2c_config);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Configuring I2C bus parameters failed");
        return res;
    }

    res = i2c_driver_install(I2C_BUS, i2c_config.mode, 0, 0, 0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing I2C bus failed");
        return res;
    }

    return ESP_OK;
}

esp_err_t initialize_coprocessor() {
    coprocessor_fw_version = 0;
    esp_err_t res          = i2c_read_reg(I2C_BUS, COPROCESSOR_ADDR, COPROCESSOR_REG_FW_VERSION, (uint8_t*) &coprocessor_fw_version, sizeof(uint16_t));
    if (res != ESP_OK) {
        coprocessor_fw_version = 0;
        ESP_LOGW(TAG, "Failed to read from CH32V003 via I2C");
    }

    if (coprocessor_fw_version != 1) {
        ESP_LOGW(TAG, "Programming CH32V003 firmware, previous version was %u", coprocessor_fw_version);
        bool ch32_result = flash_coprocessor();

        if (!ch32_result) {
            ESP_LOGE(TAG, "Failed to program the CH32V003 co-processor");
        } else {
            ESP_LOGW(TAG, "Succesfully programmed the CH32V003 co-processor");
        }

        res = i2c_read_reg(I2C_BUS, COPROCESSOR_ADDR, COPROCESSOR_REG_FW_VERSION, (uint8_t*) &coprocessor_fw_version, sizeof(uint16_t));
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read from CH32V003 via I2C after flashing");
            return ESP_FAIL;
        }

        if (coprocessor_fw_version != 1) {
            ESP_LOGE(TAG, "CH32V003 reports invalid version %u via I2C after flashing", coprocessor_fw_version);
            return ESP_FAIL;
        }
    }

    ESP_LOGW(TAG, "CH32V003 firmware version %u", coprocessor_fw_version);

    return ESP_OK;
}

static esp_err_t initialize_sdcard() {
    sdmmc_host_t          host        = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs               = GPIO_SDCARD_CS;
    slot_config.host_id               = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {.format_if_mount_failed = false, .max_files = 5, .allocation_unit_size = 16 * 1024};

    esp_err_t res = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (res != ESP_OK) {
        ESP_LOGW(TAG, "Initializing SD card failed");
        card = NULL;
    } else {
        sdmmc_card_print_info(stdout, card);
    }

    return ESP_OK;
}

esp_err_t bsp_init() {
    if (bsp_ready) {
        return ESP_OK;
    }

    esp_err_t res;

    // Non volatile storage
    res = initialize_nvs();
    if (res != ESP_OK) {
        bsp_display_error("Initializing NVS failed");
        return res;
    }

    // GPIO
    res = initialize_gpio();
    if (res != ESP_OK) {
        bsp_display_error("Initializing GPIO failed");
        return res;
    }

    // E-paper lookup table
    res = initialize_epaper_lut();
    if (res != ESP_OK) {
        bsp_display_error("Initializing epaper LUT failed");
        return res;
    }

    // SPI bus
    res = initialize_spi_bus();
    if (res != ESP_OK) {
        bsp_display_error("Initializing SPI failed");
        return res;
    }

    // Epaper display
    res = initialize_epaper();
    if (res != ESP_OK) {
        bsp_display_error("Initializing epaper display failed");
        return res;
    }

    // Graphics stack
    res = initialize_graphics();
    if (res != ESP_OK) {
        bsp_display_error("Initializing graphics stack failed");
        return res;
    }

    // Signal that epaper display is ready to be used
    epaper_ready = true;

    // I2C bus
    res = initialize_i2c_bus();
    if (res != ESP_OK) {
        bsp_display_error("Initializing I2C bus failed");
        return res;
    }

    // CH32V003 coprocessor
    res = initialize_coprocessor();
    if (res != ESP_OK) {
        bsp_display_error("Initializing coprocessor failed");
        return res;
    }

    // Signal that ch32v003 coprocessor is ready to be used
    ch32v003_ready = true;

    // SD card
    res = initialize_sdcard();
    if (res != ESP_OK) {
        bsp_display_error("Initializing SD card failed");
        return res;
    }

    // Signal that all hardware is ready to be used
    bsp_ready = true;
    return ESP_OK;
}

esp_err_t bsp_display_error(const char* error) {
    ESP_LOGE(TAG, "%s", error);
    if (epaper_ready) {
        const pax_font_t* font = pax_font_saira_regular;
        pax_background(&pax_buffer, WHITE);
        pax_draw_text(&pax_buffer, BLACK, font, 18, 5, 5, error);
        bsp_display_flush();
    }
    return ESP_OK;
}

hink_t* bsp_get_epaper() {
    if (!epaper_ready) return NULL;
    return &epaper;
}

esp_err_t bsp_display_flush() {
    if (!bsp_ready) return ESP_FAIL;
    if (!epaper_ready) return ESP_FAIL;
    if (!pax_is_dirty(&pax_buffer)) return ESP_OK;
    hink_set_lut(&epaper, lut_buffer);
    hink_write(&epaper, pax_buffer.buf);
    pax_mark_clean(&pax_buffer);
    return ESP_OK;
}

pax_buf_t* bsp_get_gfx_buffer() {
    if (!bsp_ready) return NULL;
    return &pax_buffer;
}

void bsp_restart() {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    fflush(stdout);
    esp_restart();
}