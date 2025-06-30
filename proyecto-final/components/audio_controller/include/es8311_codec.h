/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "esp_err.h"
#include "es8311.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize ES8311 codec
 * 
 * @param sample_rate Sample rate in Hz
 * @param volume Initial volume level (0-100)
 * @param microphone_enabled Enable microphone input
 * @return esp_err_t ESP_OK on success
 */
esp_err_t es8311_codec_init(uint32_t sample_rate, uint8_t volume, bool microphone_enabled);

/**
 * @brief Deinitialize ES8311 codec
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t es8311_codec_deinit(void);

/**
 * @brief Set codec volume
 * 
 * @param volume Volume level (0-100)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t es8311_codec_set_volume(uint8_t volume);

/**
 * @brief Enable Power Amplifier
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t es8311_codec_enable_pa(void);

/**
 * @brief Disable Power Amplifier
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t es8311_codec_disable_pa(void);

/**
 * @brief Get ES8311 handle for advanced operations
 * 
 * @return es8311_handle_t ES8311 handle
 */
es8311_handle_t es8311_codec_get_handle(void);

#ifdef __cplusplus
}
#endif
