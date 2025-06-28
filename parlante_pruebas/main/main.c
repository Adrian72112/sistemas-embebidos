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
extern const uint8_t music_pcm_start[] asm("_binary_8bit_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_8bit_pcm_end");

void app_main(void)
{
    printf("ESP32-S2 Kaluga Kit - Audio Player\n");
    printf("==================================\n");
    
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
    
    // Calculate audio data size
    size_t audio_data_size = music_pcm_end - music_pcm_start;
    ESP_LOGI(TAG, "Embedded audio data size: %zu bytes", audio_data_size);
    
    if (audio_data_size == 0) {
        ESP_LOGE(TAG, "No audio data found! Check if audio file is properly embedded.");
        audio_controller_deinit();
        return;
    }
    
    // Start loop playback with 1 second delay between loops
    ESP_LOGI(TAG, "Starting audio loop playback...");
    ret = audio_controller_play_loop(music_pcm_start, audio_data_size, 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start audio loop: %s", esp_err_to_name(ret));
        audio_controller_deinit();
        return;
    }
    
    ESP_LOGI(TAG, "Audio loop started successfully! 🎵");
    
    // Demonstrate volume control with different levels
    int volume_levels[] = {30, 45, 60, 75, 50}; // Various volume levels
    int num_levels = sizeof(volume_levels) / sizeof(volume_levels[0]);
    
    ESP_LOGI(TAG, "Starting volume demonstration...");
    
    while (1) {
        for (int i = 0; i < num_levels; i++) {
            ESP_LOGI(TAG, "🔊 Setting volume to %d%%", volume_levels[i]);
            ret = audio_controller_set_volume(volume_levels[i]);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to set volume to %d%%", volume_levels[i]);
            }
            vTaskDelay(pdMS_TO_TICKS(8000)); // Wait 8 seconds between volume changes
        }
    }
}
