#pragma once

#include "coprocessor.h"
#include "driver/spi_master.h"
#include "epaper.h"
#include "esp_err.h"
#include "hardware.h"
#include "pax_gfx.h"
#include "sdkconfig.h"

/** \brief Initialize basic board support
 *
 * \details This function installs the GPIO ISR (interrupt service routine) service
 *          which allows for per-pin GPIO interrupt handlers. Then it initializes the
 *          I2C and SPI communication busses and the LCD display driver. Returns ESP_OK
 *          on success and a
 *
 * \retval ESP_OK   The function succesfully executed
 * \retval ESP_FAIL The function failed, possibly indicating hardware failure
 *
 * Check the esp_err header file from the ESP-IDF for a complete list of error codes
 * returned by SDK functions.
 */

esp_err_t bsp_init();

/** \brief Prints an error message to stdout and if possible to the display
 *
 * \details Prints an error message to stdout and if possible to the display.
 *          Used to display fatal errors to the user.
 *
 * \retval ESP_OK   The function succesfully executed
 * \retval ESP_FAIL The function failed, possibly indicating hardware failure
 *
 * Check the esp_err header file from the ESP-IDF for a complete list of error codes
 * returned by SDK functions.
 */

esp_err_t bsp_display_error();

/** \brief Fetch a handle for the epaper display
 *
 * \details Fetch a handle for the epaper display, returns NULL if the epaper display has not been
 *          initialized.
 *
 * \retval ESP_OK   The function succesfully executed
 * \retval ESP_FAIL The function failed, possibly indicating hardware failure
 *
 * Check the esp_err header file from the ESP-IDF for a complete list of error codes
 * returned by SDK functions.
 */

hink_t* bsp_get_epaper();

/** \brief Flushes the framebuffer to the epaper display
 *
 * \details Flushes the framebuffer to the epaper display.
 *
 * \retval ESP_OK   The function succesfully executed
 * \retval ESP_FAIL The function failed, possibly indicating hardware failure
 *
 * Check the esp_err header file from the ESP-IDF for a complete list of error codes
 * returned by SDK functions.
 */

esp_err_t bsp_display_flush();

/** \brief Fetch a handle for the framebuffer
 *
 * \details Fetch a handle for the framebuffer.
 *
 * \retval struct:pax_buf_t Framebuffer
 * \retval NULL             Framebuffer not available
 */

pax_buf_t* bsp_get_gfx_buffer();

/** \brief Restart the board
 *
 * \details This function resets the ESP32
 */

void bsp_restart();
