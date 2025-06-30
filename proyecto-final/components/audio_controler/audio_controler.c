/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "audio_controler.h"
#include "i2s_driver.h"
#include "es8311_codec.h" 
#include "logger.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "audio_controller";

// Private variables
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;
static bool is_initialized = false;

// Playlist variables
static audio_track_t *playlist = NULL;
static size_t playlist_size = 0;
static int current_track_index = 0;

// Task control
static TaskHandle_t audio_task_handle = NULL;
static SemaphoreHandle_t player_mutex = NULL;
static bool stop_current_track = false;
static bool is_paused = false;

// Forward declarations
static void audio_play_task(void *args);

esp_err_t audio_controller_init(const audio_controller_config_t *config)
{
    ESP_LOGI(TAG, "Initializing minimal audio controller");
    
    if (is_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "Config cannot be NULL");
    
    // Create mutex
    player_mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(player_mutex, ESP_ERR_NO_MEM, TAG, "Failed to create mutex");
    
    // Initialize logger
    esp_err_t ret = logger_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Logger init failed: %s", esp_err_to_name(ret));
    }
    
    // Initialize I2S driver
    ESP_RETURN_ON_ERROR(i2s_driver_init(config->sample_rate, &tx_handle, &rx_handle), 
                       TAG, "Failed to initialize I2S driver");
    
    // Initialize ES8311 codec
    ESP_RETURN_ON_ERROR(es8311_codec_init(config->sample_rate, config->volume, config->microphone_enabled),
                       TAG, "Failed to initialize ES8311 codec");
    
    is_initialized = true;
    ESP_LOGI(TAG, "Audio controller initialized successfully");
    return ESP_OK;
}

esp_err_t audio_controller_load_playlist(const audio_track_t *tracks, size_t num_tracks)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    ESP_RETURN_ON_FALSE(tracks && num_tracks > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid tracks");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Free existing playlist
    if (playlist) {
        free(playlist);
    }
    
    // Allocate and copy new playlist
    playlist = malloc(sizeof(audio_track_t) * num_tracks);
    if (!playlist) {
        xSemaphoreGive(player_mutex);
        return ESP_ERR_NO_MEM;
    }
    
    memcpy(playlist, tracks, sizeof(audio_track_t) * num_tracks);
    playlist_size = num_tracks;
    current_track_index = 0;
    
    xSemaphoreGive(player_mutex);
    
    ESP_LOGI(TAG, "Playlist loaded: %zu tracks", num_tracks);
    return ESP_OK;
}

esp_err_t audio_controller_play(void)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    ESP_RETURN_ON_FALSE(playlist && playlist_size > 0, ESP_ERR_INVALID_STATE, TAG, "No playlist");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Stop current track if playing
    stop_current_track = true;
    if (audio_task_handle) {
        xSemaphoreGive(player_mutex);
        
        // Wait for task to finish
        int timeout = 50;
        while (audio_task_handle && timeout-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        if (audio_task_handle) {
            vTaskDelete(audio_task_handle);
            audio_task_handle = NULL;
        }
        
        if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
    }
    
    // Start new track
    stop_current_track = false;
    is_paused = false;
    
    BaseType_t result = xTaskCreate(
        audio_play_task,
        "audio_play",
        4096,
        &playlist[current_track_index],
        1,
        &audio_task_handle
    );
    
    xSemaphoreGive(player_mutex);
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio task");
        return ESP_FAIL;
    }
    
    logger_log_event(LOGGER_EVENT_PLAY);
    ESP_LOGI(TAG, "▶️ Playing: %s", playlist[current_track_index].name);
    return ESP_OK;
}

esp_err_t audio_controller_pause(void)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    if (!is_paused && audio_task_handle) {
        is_paused = true;
        stop_current_track = true;
        
        xSemaphoreGive(player_mutex);
        
        // Wait for task to finish
        int timeout = 50;
        while (audio_task_handle && timeout-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        if (audio_task_handle) {
            vTaskDelete(audio_task_handle);
            audio_task_handle = NULL;
        }
        
        logger_log_event(LOGGER_EVENT_PAUSE);
        ESP_LOGI(TAG, "⏸️ Paused: %s", playlist[current_track_index].name);
    } else {
        xSemaphoreGive(player_mutex);
    }
    
    return ESP_OK;
}

esp_err_t audio_controller_next(void)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    ESP_RETURN_ON_FALSE(playlist && playlist_size > 0, ESP_ERR_INVALID_STATE, TAG, "No playlist");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Stop current track
    stop_current_track = true;
    bool was_playing = (audio_task_handle != NULL && !is_paused);
    is_paused = false;
    
    // Wait for current task to finish if it exists
    if (audio_task_handle) {
        xSemaphoreGive(player_mutex);
        
        int timeout = 50;
        while (audio_task_handle && timeout-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        if (audio_task_handle) {
            vTaskDelete(audio_task_handle);
            audio_task_handle = NULL;
        }
        
        if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
    }
    
    // Move to next track
    current_track_index = (current_track_index + 1) % playlist_size;
    
    // Si estaba reproduciendo, iniciar la nueva canción automáticamente
    if (was_playing) {
        stop_current_track = false;
        
        BaseType_t result = xTaskCreate(
            audio_play_task,
            "audio_play",
            4096,
            &playlist[current_track_index],
            1,
            &audio_task_handle
        );
        
        if (result != pdPASS) {
            xSemaphoreGive(player_mutex);
            ESP_LOGE(TAG, "Failed to create audio task for next track");
            return ESP_FAIL;
        }
    }
    
    xSemaphoreGive(player_mutex);
    
    logger_log_event(LOGGER_EVENT_NEXT);
    ESP_LOGI(TAG, "⏭️ Next: %s%s", playlist[current_track_index].name, 
             was_playing ? " (playing)" : " (ready)");
    
    return ESP_OK;
}

esp_err_t audio_controller_previous(void)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    ESP_RETURN_ON_FALSE(playlist && playlist_size > 0, ESP_ERR_INVALID_STATE, TAG, "No playlist");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Stop current track
    stop_current_track = true;
    bool was_playing = (audio_task_handle != NULL && !is_paused);
    is_paused = false;
    
    // Wait for current task to finish if it exists
    if (audio_task_handle) {
        xSemaphoreGive(player_mutex);
        
        int timeout = 50;
        while (audio_task_handle && timeout-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        if (audio_task_handle) {
            vTaskDelete(audio_task_handle);
            audio_task_handle = NULL;
        }
        
        if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
    }
    
    // Move to previous track
    current_track_index = (current_track_index - 1 + playlist_size) % playlist_size;
    
    // Si estaba reproduciendo, iniciar la nueva canción automáticamente
    if (was_playing) {
        stop_current_track = false;
        
        BaseType_t result = xTaskCreate(
            audio_play_task,
            "audio_play",
            4096,
            &playlist[current_track_index],
            1,
            &audio_task_handle
        );
        
        if (result != pdPASS) {
            xSemaphoreGive(player_mutex);
            ESP_LOGE(TAG, "Failed to create audio task for previous track");
            return ESP_FAIL;
        }
    }
    
    xSemaphoreGive(player_mutex);
    
    logger_log_event(LOGGER_EVENT_PREVIOUS);
    ESP_LOGI(TAG, "⏮️ Previous: %s%s", playlist[current_track_index].name,
             was_playing ? " (playing)" : " (ready)");
    
    return ESP_OK;
}

// =============================================================================
// PRIVATE TASK IMPLEMENTATION
// =============================================================================

static void audio_play_task(void *args)
{
    audio_track_t *track = (audio_track_t *)args;
    if (!track || !track->data || track->size == 0) {
        ESP_LOGE(TAG, "Invalid track data");
        audio_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "🎵 Playing: %s (%zu bytes) - LOOP MODE", track->name, track->size);
    
    // Loop infinito hasta que se pare la canción
    while (!stop_current_track && !is_paused) {
        size_t bytes_written = 0;
        const uint8_t *data_ptr = track->data;
        size_t remaining = track->size;
        
        // Write audio data in chunks
        while (!stop_current_track && !is_paused && remaining > 0) {
            size_t chunk_size = (remaining > 1024) ? 1024 : remaining;
            
            esp_err_t ret = i2s_driver_write(tx_handle, data_ptr, chunk_size, &bytes_written);
            if (ret != ESP_OK || bytes_written == 0) {
                ESP_LOGE(TAG, "Failed to write audio data");
                break;
            }
            
            data_ptr += bytes_written;
            remaining -= bytes_written;
            
            // Small delay to prevent overwhelming I2S
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        
        // Si terminó la canción completa y no se pidió parar, reiniciar
        if (remaining == 0 && !stop_current_track && !is_paused) {
            ESP_LOGD(TAG, "🔄 Looping: %s", track->name);
            // Pequeña pausa entre loops para evitar clicks
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    
    ESP_LOGI(TAG, "🎵 Stopped: %s", track->name);
    
    audio_task_handle = NULL;
    vTaskDelete(NULL);
}
