/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "sdkconfig.h"
#include "audio_controler.h"
#include "i2s_driver.h"
#include "es8311_codec.h"
#include "audio_config.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "audio_controller";
static const char err_reason[][30] = {"input param is invalid",
                                      "operation timeout"};

// Private variables
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;
static bool is_initialized = false;
static uint8_t current_volume = 50;
static TaskHandle_t play_task_handle = NULL;

// Private structure for play loop task
typedef struct {
    const uint8_t *data;
    size_t size;
    uint32_t loop_delay_ms;
} play_loop_params_t;

// Private functions
static void audio_play_loop_task(void *args);

esp_err_t audio_controller_init(const audio_controller_config_t *config)
{
    ESP_LOGI(TAG, "Initializing audio controller");
    
    if (is_initialized) {
        ESP_LOGW(TAG, "Audio controller already initialized");
        return ESP_OK;
    }
    
    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "Config cannot be NULL");
    
    // Initialize I2S driver
    ESP_RETURN_ON_ERROR(i2s_driver_init(config->sample_rate, &tx_handle, &rx_handle), 
                       TAG, "Failed to initialize I2S driver");
    
    // Initialize ES8311 codec
    ESP_RETURN_ON_ERROR(es8311_codec_init(config->sample_rate, config->volume, config->microphone_enabled),
                       TAG, "Failed to initialize ES8311 codec");
    
    current_volume = config->volume;
    is_initialized = true;
    
    ESP_LOGI(TAG, "Audio controller initialized successfully");
    return ESP_OK;
}

esp_err_t audio_controller_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing audio controller");
    
    if (!is_initialized) {
        ESP_LOGW(TAG, "Audio controller not initialized");
        return ESP_OK;
    }
    
    // Stop any running playback
    audio_controller_stop();
    
    // Deinitialize codec
    es8311_codec_deinit();
    
    // Deinitialize I2S driver
    i2s_driver_deinit(tx_handle, rx_handle);
    
    tx_handle = NULL;
    rx_handle = NULL;
    is_initialized = false;
    
    ESP_LOGI(TAG, "Audio controller deinitialized");
    return ESP_OK;
}

esp_err_t audio_controller_play(const uint8_t *data, size_t size)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Audio controller not initialized");
    ESP_RETURN_ON_FALSE(data && size > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid data or size");
    
    size_t bytes_written = 0;
    esp_err_t ret = i2s_driver_write(tx_handle, data, size, &bytes_written);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write audio data: %s", 
                ret == ESP_ERR_TIMEOUT ? err_reason[1] : err_reason[0]);
        return ret;
    }
    
    if (bytes_written > 0) {
        ESP_LOGD(TAG, "Audio played successfully, %zu bytes written", bytes_written);
    } else {
        ESP_LOGE(TAG, "No bytes were written");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

static void audio_play_loop_task(void *args)
{
    play_loop_params_t *params = (play_loop_params_t *)args;
    esp_err_t ret = ESP_OK;
    size_t bytes_written = 0;
    uint8_t *data_ptr = (uint8_t *)params->data;
    
    ESP_LOGI(TAG, "Starting audio loop playback");
    
    // Preload data for immediate playback
    ret = i2s_driver_preload(tx_handle, data_ptr, params->size, &bytes_written);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to preload audio data");
        goto cleanup;
    }
    data_ptr += bytes_written;
    
    while (1) {
        // Check if task should terminate
        if (eTaskGetState(play_task_handle) == eDeleted) {
            break;
        }
        
        ret = i2s_driver_write(tx_handle, data_ptr, params->data + params->size - data_ptr, &bytes_written);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Audio loop write failed: %s", 
                    ret == ESP_ERR_TIMEOUT ? err_reason[1] : err_reason[0]);
            break;
        }
        
        if (bytes_written > 0) {
            ESP_LOGD(TAG, "Audio loop: %zu bytes written", bytes_written);
        } else {
            ESP_LOGE(TAG, "Audio loop: no bytes written");
            break;
        }
        
        // Reset data pointer for loop
        data_ptr = (uint8_t *)params->data;
        
        // Delay between loops
        if (params->loop_delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(params->loop_delay_ms));
        }
    }
    
cleanup:
    free(params);
    play_task_handle = NULL;
    ESP_LOGI(TAG, "Audio loop playback task ended");
    vTaskDelete(NULL);
}

esp_err_t audio_controller_play_loop(const uint8_t *data, size_t size, uint32_t loop_delay_ms)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Audio controller not initialized");
    ESP_RETURN_ON_FALSE(data && size > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid data or size");
    
    // Stop any existing playback
    audio_controller_stop();
    
    // Allocate parameters for the task
    play_loop_params_t *params = malloc(sizeof(play_loop_params_t));
    ESP_RETURN_ON_FALSE(params, ESP_ERR_NO_MEM, TAG, "Failed to allocate memory for play parameters");
    
    params->data = data;
    params->size = size;
    params->loop_delay_ms = loop_delay_ms;
    
    // Create playback task
    BaseType_t result = xTaskCreate(audio_play_loop_task, "audio_loop", 4096, params, 5, &play_task_handle);
    if (result != pdPASS) {
        free(params);
        ESP_LOGE(TAG, "Failed to create audio loop task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Audio loop playback started");
    return ESP_OK;
}

esp_err_t audio_controller_set_volume(uint8_t volume)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Audio controller not initialized");
    ESP_RETURN_ON_FALSE(volume <= 100, ESP_ERR_INVALID_ARG, TAG, "Volume must be 0-100");
    
    ESP_RETURN_ON_ERROR(es8311_codec_set_volume(volume), TAG, "Failed to set volume");
    current_volume = volume;
    
    return ESP_OK;
}

esp_err_t audio_controller_stop(void)
{
    if (play_task_handle) {
        ESP_LOGI(TAG, "Stopping audio playback");
        vTaskDelete(play_task_handle);
        play_task_handle = NULL;
    }
    return ESP_OK;
}

i2s_chan_handle_t audio_controller_get_tx_handle(void)
{
    return tx_handle;
}

i2s_chan_handle_t audio_controller_get_rx_handle(void)
{
    return rx_handle;
}
