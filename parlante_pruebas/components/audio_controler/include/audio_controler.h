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
 * @brief Audio controller configuration
 */
typedef struct {
    uint32_t sample_rate;           /*!< Sample rate in Hz */
    uint8_t volume;                 /*!< Volume level 0-100 */
    bool microphone_enabled;        /*!< Enable microphone */
} audio_controller_config_t;

/**
 * @brief Default audio controller configuration
 */
#define AUDIO_CONTROLLER_DEFAULT_CONFIG() { \
    .sample_rate = 10000, \
    .volume = 50, \
    .microphone_enabled = false \
}

/**
 * @brief Initialize audio controller
 * 
 * @param config Audio controller configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_init(const audio_controller_config_t *config);

/**
 * @brief Deinitialize audio controller
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_deinit(void);

/**
 * @brief Play audio data once (blocking call)
 * 
 * @param data Pointer to audio data
 * @param size Size of audio data in bytes
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_play(const uint8_t *data, size_t size);

/**
 * @brief Write audio data to I2S (non-blocking, for tasks)
 * 
 * @param data Pointer to audio data
 * @param size Size of audio data in bytes
 * @param bytes_written Pointer to store bytes written
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_write(const uint8_t *data, size_t size, size_t *bytes_written);

/**
 * @brief Preload audio data to I2S buffer
 * 
 * @param data Pointer to audio data
 * @param size Size of audio data in bytes
 * @param bytes_written Pointer to store bytes written
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_preload(const uint8_t *data, size_t size, size_t *bytes_written);

/**
 * @brief Set volume level
 * 
 * @param volume Volume level 0-100
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_set_volume(uint8_t volume);

/**
 * @brief Get TX channel handle (for advanced usage)
 * 
 * @return i2s_chan_handle_t TX channel handle
 */
i2s_chan_handle_t audio_controller_get_tx_handle(void);

/**
 * @brief Get RX channel handle (for advanced usage)
 * 
 * @return i2s_chan_handle_t RX channel handle
 */
i2s_chan_handle_t audio_controller_get_rx_handle(void);

#ifdef __cplusplus
}
#endif
