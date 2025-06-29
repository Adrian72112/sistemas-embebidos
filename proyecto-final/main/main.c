#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_controler.h"

static const char *TAG = "main";

// External references to embedded audio data
extern const uint8_t music_pcm_start[] asm("_binary_victory8bit_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_victory8bit_pcm_end");

extern const uint8_t music2_pcm_start[] asm("_binary_8bit_pcm_start");
extern const uint8_t music2_pcm_end[]   asm("_binary_8bit_pcm_end");

extern const uint8_t music3_pcm_start[] asm("_binary_start_pcm_start");
extern const uint8_t music3_pcm_end[]   asm("_binary_start_pcm_end");

extern const uint8_t music4_pcm_start[] asm("_binary_whistle_pcm_start");
extern const uint8_t music4_pcm_end[]   asm("_binary_whistle_pcm_end");

extern const uint8_t music5_pcm_start[] asm("_binary_lose_pcm_start");
extern const uint8_t music5_pcm_end[]   asm("_binary_lose_pcm_end");

extern const uint8_t music6_pcm_start[] asm("_binary_buenass_pcm_start");
extern const uint8_t music6_pcm_end[]   asm("_binary_buenass_pcm_end");

void app_main(void)
{
    printf("ESP32-S2 Kaluga Kit - Minimal Audio Player\n");
    printf("==========================================\n");
    
    // Initialize audio controller
    audio_controller_config_t config = AUDIO_CONTROLLER_DEFAULT_CONFIG();
    config.volume = 50;
    
    esp_err_t ret = audio_controller_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Init failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Prepare playlist
    audio_track_t tracks[] = {
        { music_pcm_start, music_pcm_end - music_pcm_start, "Victory" },
        { music2_pcm_start, music2_pcm_end - music2_pcm_start, "8bit Classic" },
        { music3_pcm_start, music3_pcm_end - music3_pcm_start, "Game Start" },
        {
            .data = music4_pcm_start,
            .size = music4_pcm_end - music4_pcm_start,
            .name = "Whistle"
        },
        {
            .data = music5_pcm_start,
            .size = music5_pcm_end - music5_pcm_start,
            .name = "Lose"
        },
        {
            .data = music6_pcm_start,
            .size = music6_pcm_end - music6_pcm_start,
            .name = "Bueenass"
        }
    };
    
    // Load playlist
    ret = audio_controller_load_playlist(tracks, 6);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Load playlist failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "🎵 Starting minimal demo...");
    
    // Simple demo loop
    while (1) {
        // Resume
        ESP_LOGI(TAG, "▶️ Resuming...");
        audio_controller_play();
        vTaskDelay(pdMS_TO_TICKS(5000));  // Play for 2 seconds
        
        // Next track
        ESP_LOGI(TAG, "⏭️ Next track...");
        audio_controller_next();
        vTaskDelay(pdMS_TO_TICKS(500));   // Small delay
    }
}