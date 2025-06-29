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
 * @brief Estructura de pista de audio
 */
typedef struct {
    const uint8_t *data;        /*!< Puntero a los datos de audio */
    size_t size;                /*!< Tamaño de los datos de audio en bytes */
    const char *name;           /*!< Nombre de la pista */
} audio_track_t;

/**
 * @brief Configuración del controlador de audio
 */
typedef struct {
    uint32_t sample_rate;           /*!< Frecuencia de muestreo en Hz */
    uint8_t volume;                 /*!< Nivel de volumen 0-100 */
    bool microphone_enabled;        /*!< Habilitar micrófono */
} audio_controller_config_t;

/**
 * @brief Configuración predeterminada del controlador de audio
 */
#define AUDIO_CONTROLLER_DEFAULT_CONFIG() { \
    .sample_rate = 8000, \
    .volume = 50, \
    .microphone_enabled = false \
}

/**
 * @brief Inicializar el controlador de audio
 * 
 * @param config Configuración del controlador de audio
 * @return esp_err_t ESP_OK en caso de éxito
 */
esp_err_t audio_controller_init(const audio_controller_config_t *config);

/**
 * @brief Cargar lista de reproducción con pistas de audio
 * 
 * @param tracks Array de pistas de audio
 * @param num_tracks Número de pistas en la lista de reproducción
 * @return esp_err_t ESP_OK en caso de éxito
 */
esp_err_t audio_controller_load_playlist(const audio_track_t *tracks, size_t num_tracks);

/**
 * @brief Reproducir pista actual o reanudar reproducción
 * 
 * @return esp_err_t ESP_OK en caso de éxito
 */
esp_err_t audio_controller_play(void);

/**
 * @brief Pausar pista actual
 * 
 * @return esp_err_t ESP_OK en caso de éxito
 */
esp_err_t audio_controller_pause(void);

/**
 * @brief Saltar a la siguiente pista
 * 
 * @return esp_err_t ESP_OK en caso de éxito
 */
esp_err_t audio_controller_next(void);

/**
 * @brief Ir a la pista anterior
 * 
 * @return esp_err_t ESP_OK en caso de éxito
 */
esp_err_t audio_controller_previous(void);

#ifdef __cplusplus
}
#endif
