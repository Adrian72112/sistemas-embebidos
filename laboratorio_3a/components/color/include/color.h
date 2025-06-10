/**
 * @file color.h
 * @brief Módulo para manejar el color actual del LED con sincronización mediante semáforo.
 * 
 * Este módulo encapsula el acceso a una variable compartida de tipo `color_t`, que representa
 * el color actual del LED RGB. El acceso es seguro entre tareas mediante un semáforo mutex.
 */

#pragma once

#include <stdint.h>

/**
 * @brief Enumeración de colores posibles para el LED RGB
 */
typedef enum {
    COLOR_NONE = 0, /**< Ningún color (apagado) */
    COLOR_RED,      /**< Color rojo */
    COLOR_GREEN,    /**< Color verde */
    COLOR_BLUE      /**< Color azul */
} color_t;

/**
 * @brief Inicializa el módulo de color
 * 
 * Crea el semáforo mutex necesario para proteger el acceso a la variable de color.
 * Esta función debe llamarse una vez al inicio del programa.
 */
void color_init(void);

/**
 * @brief Establece el color actual del LED
 * 
 * Esta función actualiza el valor del color compartido de forma segura mediante un semáforo.
 * 
 * @param[in] color Color que se desea establecer
 */
void color_set(color_t color);

/**
 * @brief Obtiene el color actual del LED
 * 
 * Esta función devuelve el color actualmente configurado, accediendo de forma segura
 * mediante un semáforo.
 * 
 * @return color_t El color actualmente activo
 */
color_t color_get(void);
