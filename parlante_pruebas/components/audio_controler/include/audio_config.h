/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Audio Configuration Constants */
#define EXAMPLE_RECV_BUF_SIZE       (2400)
#define EXAMPLE_SAMPLE_RATE         (8000) // 8kHz sample rate
#define EXAMPLE_MCLK_MULTIPLE       (384)  // If not using 24-bit data width, 256 should be enough
#define EXAMPLE_MCLK_FREQ_HZ        (EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE)
#define EXAMPLE_VOICE_VOLUME        (50)   // Volume level for ES8311 codec, range: 0-100
#define EXAMPLE_PA_CTRL_IO          (GPIO_NUM_10)

/* I2C Configuration */
#define I2C_NUM                     (0)
#define I2C_SCL_IO                  (GPIO_NUM_7)
#define I2C_SDA_IO                  (GPIO_NUM_8)

/* I2S Configuration - ESP32S2 Kaluga Kit v1.3 */
#define I2S_NUM                     (1)    // ESPECÍFICAMENTE I2S1 según el esquemático
#define I2S_MCK_IO                  (GPIO_NUM_35)
#define I2S_BCK_IO                  (GPIO_NUM_18)
#define I2S_WS_IO                   (GPIO_NUM_17)
#define I2S_DO_IO                   (GPIO_NUM_12)  // Speaker Out
#define I2S_DI_IO                   (GPIO_NUM_46)  // Mic In

#ifdef __cplusplus
}
#endif
