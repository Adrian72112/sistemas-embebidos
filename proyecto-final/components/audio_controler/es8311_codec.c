/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "sdkconfig.h"
#include "es8311_codec.h"
#include "audio_config.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "es8311.h"

static const char *TAG = "es8311_codec";
static es8311_handle_t es_handle = NULL;

esp_err_t es8311_codec_init(uint32_t sample_rate, uint8_t volume, bool microphone_enabled)
{
    ESP_LOGI(TAG, "Initializing ES8311 codec");
    
    // Configure I2C for ES8311
    const i2c_config_t es_i2c_cfg = {
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .mode = I2C_MODE_MASTER,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM, &es_i2c_cfg), TAG, "Failed to configure I2C");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER, 0, 0, 0), TAG, "Failed to install I2C driver");

    // Configure and enable Power Amplifier
    esp_err_t ret = es8311_codec_enable_pa();
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to enable PA");

    // Create ES8311 handle
    es_handle = es8311_create(I2C_NUM, ES8311_ADDRRES_0);
    ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, TAG, "Failed to create ES8311 handle");

    // Configure ES8311 clock
    const es8311_clock_config_t es_clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = sample_rate * EXAMPLE_MCLK_MULTIPLE,
        .sample_frequency = sample_rate
    };

    // Initialize ES8311
    ESP_RETURN_ON_ERROR(es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16), 
                       TAG, "Failed to initialize ES8311");
    
    // Configure sample frequency
    ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(es_handle, sample_rate * EXAMPLE_MCLK_MULTIPLE, sample_rate), 
                       TAG, "Failed to configure ES8311 sample frequency");
    
    // Set volume
    ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, volume, NULL), 
                       TAG, "Failed to set ES8311 volume");
    
    // Configure microphone
    ESP_RETURN_ON_ERROR(es8311_microphone_config(es_handle, microphone_enabled), 
                       TAG, "Failed to configure ES8311 microphone");

    ESP_LOGI(TAG, "ES8311 codec initialized successfully (Sample Rate: %ld Hz, Volume: %d, Mic: %s)", 
             sample_rate, volume, microphone_enabled ? "enabled" : "disabled");
    return ESP_OK;
}

esp_err_t es8311_codec_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing ES8311 codec");
    
    if (es_handle) {
        // Disable PA before deinit
        es8311_codec_disable_pa();
        
        // Note: es8311 library doesn't provide a destroy function
        // so we just set handle to NULL
        es_handle = NULL;
    }
    
    // Uninstall I2C driver
    esp_err_t ret = i2c_driver_delete(I2C_NUM);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to delete I2C driver: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "ES8311 codec deinitialized");
    return ESP_OK;
}

esp_err_t es8311_codec_set_volume(uint8_t volume)
{
    ESP_RETURN_ON_FALSE(es_handle, ESP_ERR_INVALID_STATE, TAG, "ES8311 not initialized");
    ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, volume, NULL), TAG, "Failed to set volume");
    ESP_LOGI(TAG, "Volume set to %d", volume);
    return ESP_OK;
}

esp_err_t es8311_codec_enable_pa(void)
{
    ESP_LOGI(TAG, "Enabling Power Amplifier (PA)");
    
    // Configure PA control GPIO
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = (1ULL << EXAMPLE_PA_CTRL_IO),
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&gpio_cfg), TAG, "Failed to configure PA GPIO");
    ESP_RETURN_ON_ERROR(gpio_set_level(EXAMPLE_PA_CTRL_IO, 1), TAG, "Failed to enable PA");
    
    ESP_LOGI(TAG, "Power Amplifier enabled");
    return ESP_OK;
}

esp_err_t es8311_codec_disable_pa(void)
{
    ESP_LOGI(TAG, "Disabling Power Amplifier (PA)");
    esp_err_t ret = gpio_set_level(EXAMPLE_PA_CTRL_IO, 0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to disable PA: %s", esp_err_to_name(ret));
    }
    return ret;
}

es8311_handle_t es8311_codec_get_handle(void)
{
    return es_handle;
}
