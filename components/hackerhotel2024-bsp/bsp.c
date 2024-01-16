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
#include "ledstrip.h"
#include "pax_codecs.h"

const uint8_t target_coprocessor_fw_version = 3;  // Must match the value in the ch32_firmware.bin resource

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

typedef void (*coprocessor_intr_t)();

static SemaphoreHandle_t coprocessor_intr_trigger     = NULL;
static TaskHandle_t      coprocessor_intr_task_handle = NULL;
static QueueHandle_t     coprocessor_button_queue     = NULL;
static SemaphoreHandle_t coprocessor_semaphore        = NULL;

extern const uint8_t ch32_firmware_start[] asm("_binary_ch32_firmware_bin_start");
extern const uint8_t ch32_firmware_end[] asm("_binary_ch32_firmware_bin_end");

extern const uint8_t hackerhotel_png_start[] asm("_binary_hackerhotel_png_start");
extern const uint8_t hackerhotel_png_end[] asm("_binary_hackerhotel_png_end");


esp_err_t bsp_display_error(const char* error) {
    ESP_LOGE(TAG, "%s", error);
    if (epaper_ready) {
        const pax_font_t* font = pax_font_sky_mono;
        pax_background(&pax_buffer, WHITE);
        pax_draw_text(&pax_buffer, RED, font, 18, 5, 5, "Error");
        pax_draw_text(&pax_buffer, BLACK, font, 12, 5, 25, error);
        bsp_display_flush();
    }
    return ESP_OK;
}

esp_err_t bsp_display_message(const char* title, const char* message) {
    ESP_LOGI(TAG, "%s", message);
    if (epaper_ready) {
        const pax_font_t* font = pax_font_sky_mono;
        pax_background(&pax_buffer, WHITE);
        pax_draw_text(&pax_buffer, RED, font, 18, 0, 0, title);
        pax_draw_text(&pax_buffer, BLACK, font, 12, 0, 25, message);
        bsp_display_flush();
    }
    return ESP_OK;
}

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

static void IRAM_ATTR coprocessor_intr_handler(void* arg) {
    /* in interrupt handler context */
    xSemaphoreGiveFromISR(coprocessor_intr_trigger, NULL);
    portYIELD_FROM_ISR();
}

static void coprocessor_intr_task(void* arg) {
    uint8_t prev_buttons[5] = {0};

    while (1) {
        if (xSemaphoreTake(coprocessor_intr_trigger, portMAX_DELAY)) {
            if (xSemaphoreTake(coprocessor_semaphore, portMAX_DELAY)) {
                uint8_t   buttons[5];
                esp_err_t res = i2c_read_reg(I2C_BUS, COPROCESSOR_ADDR, COPROCESSOR_REG_BTN, buttons, 5);
                xSemaphoreGive(coprocessor_semaphore);
                if (res != ESP_OK) {
                    ESP_LOGE(TAG, "Coprocessor interrupt task failed to read button states");
                    continue;
                }
                for (uint8_t index = 0; index < sizeof(buttons); index++) {
                    if (prev_buttons[index] != buttons[index]) {
                        prev_buttons[index] = buttons[index];
                        // ESP_LOGD(TAG, "Button state changed: %u = %u", index, buttons[index]);
                        coprocessor_input_message_t message;
                        message.button = index;
                        message.state  = buttons[index];
                        xQueueSend(coprocessor_button_queue, &message, 0);
                    }
                }
            }
        }
    }
}

static esp_err_t initialize_led() {
    esp_err_t res = ledstrip_init(GPIO_LED_DATA);
    if (res != ESP_OK) {
        return res;
    }
    const uint8_t data[3] = {0,0,0};
    res = ledstrip_send(data, 3);
    return res;
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
    esp_err_t res = ESP_OK;

    if (!hink_get_lut_populated()) {
        ESP_LOGW(TAG, "Epaper LUT table not initialized");
        res = hink_read_lut(GPIO_SPI_MOSI, GPIO_SPI_CLK, epaper.pin_cs, epaper.pin_dcx, epaper.pin_reset, epaper.pin_busy);
        if (res != ESP_OK) {
            return res;
        }
    }

    return res;
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

static esp_err_t initialize_epaper() {
    esp_err_t res = hink_init(&epaper);
    if (res != ESP_OK) {
        return res;
    }
    res = hink_apply_lut(&epaper, lut_1s);
    if (res != ESP_OK) {
        return res;
    }
    return res;
}

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
    bool invalid_version = false;

    coprocessor_fw_version = 0;
    esp_err_t res          = i2c_read_reg(I2C_BUS, COPROCESSOR_ADDR, COPROCESSOR_REG_FW_VERSION, (uint8_t*) &coprocessor_fw_version, sizeof(uint16_t));
    if (res != ESP_OK) {
        coprocessor_fw_version = 0;
        ESP_LOGW(TAG, "Failed to read from CH32V003 via I2C");
    }

    if (coprocessor_fw_version != target_coprocessor_fw_version) {
        ESP_LOGW(TAG, "Programming CH32V003 firmware, previous version was %u", coprocessor_fw_version);
        bsp_display_message("Please wait", "Updating coprocessor firmware...\n\nDo NOT power off the device");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_err_t ch32_result = flash_coprocessor();

        if (ch32_result != ESP_OK) {
            ESP_LOGE(TAG, "Failed to program the CH32V003 co-processor");
            return ESP_ERR_INVALID_STATE;
        } else {
            ESP_LOGW(TAG, "Succesfully programmed the CH32V003 co-processor");
        }

        res = i2c_read_reg(I2C_BUS, COPROCESSOR_ADDR, COPROCESSOR_REG_FW_VERSION, (uint8_t*) &coprocessor_fw_version, sizeof(uint16_t));
        if (res != ESP_OK || coprocessor_fw_version == 0) {
            ESP_LOGE(TAG, "Failed to read from CH32V003 via I2C after flashing");
            return ESP_ERR_INVALID_RESPONSE;
        }

        if (coprocessor_fw_version != target_coprocessor_fw_version) {
            ESP_LOGE(TAG, "CH32V003 reports invalid version %u via I2C after flashing", coprocessor_fw_version);
            invalid_version = true;
        }
    }

    ESP_LOGW(TAG, "CH32V003 firmware version %u", coprocessor_fw_version);

    // Initialize interrupt handling

    coprocessor_button_queue = xQueueCreate(8, sizeof(coprocessor_input_message_t));
    if (coprocessor_button_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    coprocessor_intr_trigger = xSemaphoreCreateBinary();
    if (coprocessor_intr_trigger == NULL) {
        return ESP_ERR_NO_MEM;
    }

    coprocessor_semaphore = xSemaphoreCreateBinary();
    if (coprocessor_semaphore == NULL) {
        return ESP_ERR_NO_MEM;
    }

    res = gpio_isr_handler_add(GPIO_CH32_INT, coprocessor_intr_handler, NULL);
    if (res != ESP_OK) return res;

    gpio_config_t sao_cfg = {
        .pin_bit_mask = BIT64(GPIO_CH32_INT),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = false,
        .pull_down_en = false,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    res = gpio_config(&sao_cfg);
    if (res != ESP_OK) {
        return res;
    }

    xTaskCreate(&coprocessor_intr_task, "Coprocessor interrupt", 4096, NULL, 10, coprocessor_intr_task_handle);
    xSemaphoreGive(coprocessor_intr_trigger);

    xSemaphoreGive(coprocessor_semaphore);

    if (invalid_version) {
        return ESP_ERR_INVALID_VERSION;
    }

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

    // Addressable LED
    res = initialize_led();
    if (res != ESP_OK) {
        bsp_display_error("Initializing LED failed");
        return res;
    }

    bsp_set_addressable_led(0xFF0000); // RED

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

    bsp_set_addressable_led(0x0000FF); // BLUE

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
    if (res == ESP_ERR_INVALID_STATE) {
        bsp_display_error("Flashing CH32V003 failed\nPlease contact support");
        return res;
    } else if (res == ESP_ERR_INVALID_RESPONSE) {
        bsp_display_error("No response from coprocessor\nRemove all addons and restart\nIf that doesn't help\nplease contact support");
        return res;
    } else if (res == ESP_ERR_INVALID_VERSION) {
        bsp_display_error("Coprocessor reports invalid\nversion. Please contact support\nBadge will attempt to continue starting");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    } else if (res != ESP_OK) {
        bsp_display_error("Initializing coprocessor failed\n\nPlease contact support");
        return res;
    }

    // Signal that ch32v003 coprocessor is ready to be used
    ch32v003_ready = true;

    bsp_set_addressable_led(0x00FF00); // GREEN

    // SD card
    res = initialize_sdcard();
    if (res != ESP_OK) {
        bsp_display_error("Initializing SD card failed");
        return res;
    }

    bsp_set_addressable_led(0x000000); // OFF

    // Signal that all hardware is ready to be used
    bsp_ready = true;

    // Factory test
    if (!bsp_passed_factory_test()) {
        return bsp_factory_test();
    }

    return ESP_OK;
}

hink_t* bsp_get_epaper() {
    if (!epaper_ready) return NULL;
    return &epaper;
}

esp_err_t bsp_display_flush() {
    if (!epaper_ready) return ESP_FAIL;
    // if (!pax_is_dirty(&pax_buffer)) return ESP_OK;
    hink_write(&epaper, pax_buffer.buf);
    pax_mark_clean(&pax_buffer);
    return ESP_OK;
}

void bsp_display_wait() {
    if (!epaper_ready) {
        return;
    }
    while (hink_busy(&epaper)) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool bsp_display_busy() {
    if (!epaper_ready) {
        return true;
    }
    return hink_busy(&epaper);
}

pax_buf_t* bsp_get_gfx_buffer() {
    if (!bsp_ready) return NULL;
    return &pax_buffer;
}

QueueHandle_t bsp_get_button_queue() { return coprocessor_button_queue; }

esp_err_t bsp_set_leds(uint32_t led) {
    if (xSemaphoreTake(coprocessor_semaphore, portMAX_DELAY)) {
        esp_err_t res = i2c_write_reg_n(I2C_BUS, COPROCESSOR_ADDR, COPROCESSOR_REG_LED, (uint8_t*) &led, sizeof(uint32_t));
        xSemaphoreGive(coprocessor_semaphore);
        return res;
    }
    return ESP_FAIL;
}

void bsp_restart() {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    fflush(stdout);
    esp_restart();
}

esp_err_t bsp_apply_lut(epaper_lut_t lut_type) { return hink_apply_lut(&epaper, lut_type); }

bool bsp_wait_for_button() {
    QueueHandle_t               queue         = bsp_get_button_queue();
    coprocessor_input_message_t buttonMessage = {0};
    bool                        result        = false;
    while (1) {
        if (xQueueReceive(queue, &buttonMessage, portMAX_DELAY) == pdTRUE) {
            if (buttonMessage.state == SWITCH_PRESS) {
                if (buttonMessage.button == SWITCH_5) {
                    result = true;
                }
                break;
            }
        }
    }
    return result;
}

uint8_t bsp_wait_for_button_number() {
    QueueHandle_t               queue         = bsp_get_button_queue();
    coprocessor_input_message_t buttonMessage = {0};
    bool                        result        = false;
    while (1) {
        if (xQueueReceive(queue, &buttonMessage, portMAX_DELAY) == pdTRUE) {
            if (buttonMessage.state == SWITCH_PRESS) {
                return buttonMessage.button;
            }
        }
    }
}

void bsp_flush_button_queue() {
    QueueHandle_t               queue         = bsp_get_button_queue();
    coprocessor_input_message_t buttonMessage = {0};
    while (xQueueReceive(queue, &buttonMessage, 0) == pdTRUE) {
        // empty.
    }
}

esp_err_t bsp_set_addressable_led(uint32_t color) {
    uint8_t data[3];
    data[0] = (color >>  8) & 0xFF; // R
    data[1] = (color >> 16) & 0xFF; // G
    data[2] = (color >>  0) & 0xFF; // B
    return ledstrip_send(data, 3);
}

esp_err_t bsp_set_addressable_leds(uint8_t data, int length) {
    return ledstrip_send(data, length);
}

bool bsp_passed_factory_test() {
    nvs_handle_t nvs_handle;
    esp_err_t res = nvs_open("system", NVS_READWRITE, &nvs_handle);
    if (res != ESP_OK) {
        return false;
    }
    uint8_t value;
    res = nvs_get_u8(nvs_handle, "factory_test", &value);
    nvs_close(nvs_handle);
    if (res != ESP_OK) {
        return false;
    }

    return (value == 1);
}

esp_err_t bsp_factory_test() {
    esp_err_t res;

    if (!bsp_ready) {
        return ESP_FAIL;
    }

    bsp_display_message("Factory test", "Testing...");

    bsp_set_addressable_led(0xFF00FF); // Purple

    bsp_set_relay(true);
    vTaskDelay(pdMS_TO_TICKS(500));
    bsp_set_relay(false);

    uint8_t   buttons[5];
    xSemaphoreTake(coprocessor_semaphore, portMAX_DELAY);
    res = i2c_read_reg(I2C_BUS, COPROCESSOR_ADDR, COPROCESSOR_REG_BTN, buttons, 5);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Factory test failed to read button states");
        bsp_display_message("Factory test", "Failed to read button states");
        xSemaphoreGive(coprocessor_semaphore);
        return ESP_FAIL;
    }
    xSemaphoreGive(coprocessor_semaphore);

    for (uint8_t index = 0; index < sizeof(buttons); index++) {
        if (buttons[index] != SWITCH_IDLE) {
            char message[40] = {0};
            snprintf(message, 39, " --- FAILED ---\nButton %u stuck (%u)", index, buttons[index]);
            bsp_display_message("Factory test", message);
            return ESP_FAIL;
        }
    }

    bsp_set_leds(0xFFFFFFFF);

    bsp_display_message("Factory test", "Check LEDs:\nAll diamond LEDs should be on\nAddressable LED should be pink\n\nOK?   Press button 5\nFAIL? Press button 1\nSKIP? Press button 2");

    bsp_flush_button_queue();
    uint8_t button = bsp_wait_for_button_number();

    bool pass = (button == SWITCH_5);

    if (button == SWITCH_2) {
        bsp_display_message("Factory test", " --- SKIPPED ---\nStarting application...");
        return ESP_OK;
    }

    if (!pass) {
        bsp_display_message("Factory test", " --- FAILED ---\nLEDs not okay");
        return ESP_FAIL;
    }

    bsp_display_message("Factory test", " --- PASS ---");

    nvs_handle_t nvs_handle;
    res = nvs_open("system", NVS_READWRITE, &nvs_handle);
    if (res != ESP_OK) {
        bsp_display_message("Factory test", " --- FAILED ---\nNVS open failed");
        return ESP_FAIL;
    }
    res = nvs_set_u8(nvs_handle, "factory_test", 1);
    nvs_close(nvs_handle);
    if (res != ESP_OK) {
        bsp_display_message("Factory test", " --- FAILED ---\nNVS store failed");
        return ESP_FAIL;
    }

    pax_background(&pax_buffer, WHITE);
    pax_insert_png_buf(&pax_buffer, hackerhotel_png_start, hackerhotel_png_end - hackerhotel_png_start, 0, 0, 0);
    bsp_apply_lut(lut_full);
    bsp_display_flush();
    bsp_display_wait();

    bsp_set_addressable_led(0x00FF00); // Green

    // Wait until power gets removed
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return ESP_OK;
}

esp_err_t bsp_set_relay(bool state) {
    return gpio_set_level(GPIO_RELAY, state);
}