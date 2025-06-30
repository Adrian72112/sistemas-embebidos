/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "esp_err.h"
#include "driver/i2s_std.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize I2S driver
 * 
 * @param sample_rate Sample rate in Hz
 * @param tx_handle Pointer to store TX handle
 * @param rx_handle Pointer to store RX handle
 * @return esp_err_t ESP_OK on success
 */
esp_err_t i2s_driver_init(uint32_t sample_rate, i2s_chan_handle_t *tx_handle, i2s_chan_handle_t *rx_handle);

/**
 * @brief Deinitialize I2S driver
 * 
 * @param tx_handle TX channel handle
 * @param rx_handle RX channel handle
 * @return esp_err_t ESP_OK on success
 */
esp_err_t i2s_driver_deinit(i2s_chan_handle_t tx_handle, i2s_chan_handle_t rx_handle);

/**
 * @brief Write data to I2S
 * 
 * @param tx_handle TX channel handle
 * @param data Data to write
 * @param size Size of data
 * @param bytes_written Pointer to store bytes written
 * @return esp_err_t ESP_OK on success
 */
esp_err_t i2s_driver_write(i2s_chan_handle_t tx_handle, const uint8_t *data, size_t size, size_t *bytes_written);

/**
 * @brief Preload data to I2S buffer
 * 
 * @param tx_handle TX channel handle
 * @param data Data to preload
 * @param size Size of data
 * @param bytes_written Pointer to store bytes written
 * @return esp_err_t ESP_OK on success
 */
esp_err_t i2s_driver_preload(i2s_chan_handle_t tx_handle, const uint8_t *data, size_t size, size_t *bytes_written);

#ifdef __cplusplus
}
#endif
