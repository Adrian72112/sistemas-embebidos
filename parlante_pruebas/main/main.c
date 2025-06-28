/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "audio_controler.h"

static const char *TAG = "main";

// External references to embedded audio data
extern const uint8_t music_pcm_start[] asm("_binary_victory8bit_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_victory8bit_pcm_end");

extern const uint8_t music2_pcm_start[] asm("_binary_8bit_pcm_start");
extern const uint8_t music2_pcm_end[]   asm("_binary_8bit_pcm_end");

// Task control variables
static TaskHandle_t audio_task_handle = NULL;
static bool stop_audio_task = false;

// Audio data structure
typedef struct {
    const uint8_t *data;
    size_t size;
    const char *name;
} audio_track_t;

// Audio task that plays a single track
static void audio_play_task(void *args)
{
    audio_track_t *track = (audio_track_t *)args;
    size_t bytes_written = 0;
    uint8_t *data_ptr = (uint8_t *)track->data;
    
    ESP_LOGI(TAG, "🎵 Audio task started for: %s", track->name);
    
    // Simple preload - just start writing
    esp_err_t ret = audio_controller_preload(data_ptr, track->size, &bytes_written);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to preload %s", track->name);
        goto cleanup;
    }
    
    if (bytes_written < track->size) {
        data_ptr += bytes_written;
        size_t remaining = track->size - bytes_written;
        
        while (!stop_audio_task && remaining > 0) {
            ret = audio_controller_write(data_ptr, remaining, &bytes_written);
            if (ret != ESP_OK || bytes_written == 0) {
                ESP_LOGE(TAG, "Failed to write audio data for %s", track->name);
                break;
            }
            
            data_ptr += bytes_written;
            remaining -= bytes_written;
            
            // Small delay to avoid overwhelming the I2S
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    
cleanup:
    ESP_LOGI(TAG, "🎵 Audio task ended for: %s", track->name);
    audio_task_handle = NULL;
    vTaskDelete(NULL);
}

// Function to start playing a track
static esp_err_t play_track(const audio_track_t *track)
{
    // Stop any existing playback
    if (audio_task_handle) {
        stop_audio_task = true;
        
        // Wait for task to finish
        int timeout = 100; // 1 second timeout
        while (audio_task_handle && timeout-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        // Force delete if needed
        if (audio_task_handle) {
            vTaskDelete(audio_task_handle);
            audio_task_handle = NULL;
        }
    }
    
    // Reset stop flag
    stop_audio_task = false;
    
    // Create new audio task
    BaseType_t result = xTaskCreate(audio_play_task, "audio_play", 4096, (void*)track, 5, &audio_task_handle);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio task for %s", track->name);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

void app_main(void)
{
    printf("ESP32-S2 Kaluga Kit - Audio Player (Refactored)\n");
    printf("===============================================\n");
    
    // Configure audio controller
    audio_controller_config_t audio_config = AUDIO_CONTROLLER_DEFAULT_CONFIG();
    
    ESP_LOGI(TAG, "Initializing audio controller...");
    
    // Initialize audio controller
    esp_err_t ret = audio_controller_init(&audio_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio controller: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "Audio controller initialized successfully");
    
    // Prepare audio tracks
    audio_track_t tracks[] = {
        {
            .data = music_pcm_start,
            .size = music_pcm_end - music_pcm_start,
            .name = "Victory"
        },
        {
            .data = music2_pcm_start,
            .size = music2_pcm_end - music2_pcm_start,
            .name = "8bit Classic"
        }
    };
    
    const int num_tracks = sizeof(tracks) / sizeof(tracks[0]);
    
    // Verify audio data
    for (int i = 0; i < num_tracks; i++) {
        ESP_LOGI(TAG, "Track %d: %s (%zu bytes)", i, tracks[i].name, tracks[i].size);
        if (tracks[i].size == 0) {
            ESP_LOGE(TAG, "No audio data found for %s!", tracks[i].name);
            audio_controller_deinit();
            return;
        }
    }
    
    ESP_LOGI(TAG, "🎵 Starting track alternation every 4 seconds...");
    
    int current_track = 0;
    
    while (1) {
        // Play current track
        ESP_LOGI(TAG, "🎶 Now playing: %s", tracks[current_track].name);
        
        ret = play_track(&tracks[current_track]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start playback for %s", tracks[current_track].name);
        }
        
        // Wait 4 seconds
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Stop current track
        ESP_LOGI(TAG, "🔄 Switching tracks...");
        stop_audio_task = true;
        
        // Wait for task to stop
        int timeout = 50;
        while (audio_task_handle && timeout-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        // Force stop if needed
        if (audio_task_handle) {
            vTaskDelete(audio_task_handle);
            audio_task_handle = NULL;
        }
        
        // Move to next track
        current_track = (current_track + 1) % num_tracks;
        
        // Small delay for clean transition
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
