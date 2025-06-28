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
    .sample_rate = 16000, \
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
 * @brief Play audio data
 * 
 * @param data Pointer to audio data
 * @param size Size of audio data in bytes
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_play(const uint8_t *data, size_t size);

/**
 * @brief Play audio data in loop
 * 
 * @param data Pointer to audio data
 * @param size Size of audio data in bytes
 * @param loop_delay_ms Delay between loops in milliseconds
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_play_loop(const uint8_t *data, size_t size, uint32_t loop_delay_ms);

/**
 * @brief Set volume level
 * 
 * @param volume Volume level 0-100
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_set_volume(uint8_t volume);

/**
 * @brief Stop audio playback
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_stop(void);

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
