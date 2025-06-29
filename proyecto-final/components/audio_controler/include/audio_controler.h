/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio track structure
 */
typedef struct {
    const uint8_t *data;        /*!< Pointer to audio data */
    size_t size;                /*!< Size of audio data in bytes */
    const char *name;           /*!< Track name */
} audio_track_t;

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
    .sample_rate = 8000, \
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
 * @brief Load playlist with audio tracks
 * 
 * @param tracks Array of audio tracks
 * @param num_tracks Number of tracks in the playlist
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_load_playlist(const audio_track_t *tracks, size_t num_tracks);

/**
 * @brief Play current track or resume playback
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_play(void);

/**
 * @brief Pause current track
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_pause(void);

/**
 * @brief Skip to next track
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_next(void);

/**
 * @brief Go to previous track
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t audio_controller_previous(void);

#ifdef __cplusplus
}
#endif
