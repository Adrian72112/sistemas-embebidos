/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "i2s_driver.h"
#include "audio_config.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "i2s_driver";

esp_err_t i2s_driver_init(uint32_t sample_rate, i2s_chan_handle_t *tx_handle, i2s_chan_handle_t *rx_handle)
{
    ESP_LOGI(TAG, "Initializing I2S driver with sample rate: %ld Hz", sample_rate);
    
    // Channel configuration
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, tx_handle, rx_handle), TAG, "Failed to create I2S channel");

    // Standard mode configuration
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;

    // Initialize channels
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(*tx_handle, &std_cfg), TAG, "Failed to init TX channel");
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(*rx_handle, &std_cfg), TAG, "Failed to init RX channel");
    
    // Enable channels
    ESP_RETURN_ON_ERROR(i2s_channel_enable(*tx_handle), TAG, "Failed to enable TX channel");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(*rx_handle), TAG, "Failed to enable RX channel");

    ESP_LOGI(TAG, "I2S driver initialized successfully");
    return ESP_OK;
}

esp_err_t i2s_driver_deinit(i2s_chan_handle_t tx_handle, i2s_chan_handle_t rx_handle)
{
    ESP_LOGI(TAG, "Deinitializing I2S driver");
    
    if (tx_handle) {
        ESP_RETURN_ON_ERROR(i2s_channel_disable(tx_handle), TAG, "Failed to disable TX channel");
        ESP_RETURN_ON_ERROR(i2s_del_channel(tx_handle), TAG, "Failed to delete TX channel");
    }
    
    if (rx_handle) {
        ESP_RETURN_ON_ERROR(i2s_channel_disable(rx_handle), TAG, "Failed to disable RX channel");
        ESP_RETURN_ON_ERROR(i2s_del_channel(rx_handle), TAG, "Failed to delete RX channel");
    }

    ESP_LOGI(TAG, "I2S driver deinitialized successfully");
    return ESP_OK;
}

esp_err_t i2s_driver_write(i2s_chan_handle_t tx_handle, const uint8_t *data, size_t size, size_t *bytes_written)
{
    return i2s_channel_write(tx_handle, data, size, bytes_written, portMAX_DELAY);
}

esp_err_t i2s_driver_preload(i2s_chan_handle_t tx_handle, const uint8_t *data, size_t size, size_t *bytes_written)
{
    ESP_RETURN_ON_ERROR(i2s_channel_disable(tx_handle), TAG, "Failed to disable TX channel for preload");
    ESP_RETURN_ON_ERROR(i2s_channel_preload_data(tx_handle, data, size, bytes_written), TAG, "Failed to preload data");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx_handle), TAG, "Failed to enable TX channel after preload");
    return ESP_OK;
}
